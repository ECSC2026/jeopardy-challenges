//! Parse AWK source (via pest) into the AST.

use std::sync::LazyLock;

use anyhow::{anyhow, Result};
use pest::iterators::{Pair, Pairs};
use pest::pratt_parser::{Assoc, Op, PrattParser};
use pest::Parser;
use pest_derive::Parser;

use crate::ast::{is_builtin, Program, Function, Item, Pattern, Stmt, Expr, BinOp, UnOp};

#[derive(Parser)]
#[grammar = "awk.pest"]
struct AwkParser;

pub fn parse(src: &str) -> Result<Program> {
    let mut pairs = AwkParser::parse(Rule::program, src)
        .map_err(|e| anyhow!("parse error:\n{e}"))?;

    let mut items = Vec::new();
    let mut functions = Vec::new();
    for pair in pairs.next().unwrap().into_inner() {
        match pair.as_rule() {
            Rule::function_def => functions.push(parse_function(pair)),
            Rule::rule => items.push(parse_rule(pair)),
            _ => {} // EOI
        }
    }
    Ok(Program { items, functions })
}

fn parse_function(pair: Pair<Rule>) -> Function {
    let mut inner = pair.into_inner();
    let name = inner.next().unwrap().as_str().to_string();
    let mut params = Vec::new();
    let mut body = Vec::new();
    for p in inner {
        match p.as_rule() {
            Rule::param_list => params = p.into_inner().map(|n| n.as_str().to_string()).collect(),
            _ => body = parse_block(p),
        }
    }
    Function { name, params, body }
}

fn parse_rule(pair: Pair<Rule>) -> Item {
    let mut it = pair.into_inner();
    let head = it.next().unwrap();
    let (pattern, action) = match head.as_rule() {
        Rule::special if head.as_str() == "BEGIN" => (Pattern::Begin, parse_block(it.next().unwrap())),
        Rule::special => (Pattern::End, parse_block(it.next().unwrap())),
        Rule::block => (Pattern::Always, parse_block(head)),
        // A pattern with no action prints $0.
        _ => (
            parse_pattern(head),
            it.next().map_or_else(|| vec![Stmt::Print(vec![])], parse_block),
        ),
    };
    Item { pattern, action }
}

fn parse_pattern(pair: Pair<Rule>) -> Pattern {
    let mut it = pair.into_inner();
    let first = expr_from(it.next().unwrap());
    match it.next() {
        Some(last) => Pattern::Range(first, expr_from(last)),
        None => Pattern::Expr(first),
    }
}

fn parse_block(pair: Pair<Rule>) -> Vec<Stmt> {
    pair.into_inner().map(parse_stmt).collect()
}

fn parse_stmt(pair: Pair<Rule>) -> Stmt {
    match pair.as_rule() {
        Rule::block => Stmt::Block(parse_block(pair)),
        Rule::if_stmt => {
            let mut it = pair.into_inner();
            Stmt::If(
                expr_from(it.next().unwrap()),
                Box::new(parse_stmt(it.next().unwrap())),
                it.next().map(|e| Box::new(parse_stmt(e))),
            )
        }
        Rule::while_stmt => {
            let mut it = pair.into_inner();
            Stmt::While(expr_from(it.next().unwrap()), Box::new(parse_stmt(it.next().unwrap())))
        }
        Rule::do_stmt => {
            let mut it = pair.into_inner();
            Stmt::DoWhile(Box::new(parse_stmt(it.next().unwrap())), expr_from(it.next().unwrap()))
        }
        Rule::for_stmt => parse_for(pair),
        Rule::break_stmt => Stmt::Break,
        Rule::continue_stmt => Stmt::Continue,
        Rule::next_stmt => Stmt::Next,
        Rule::exit_stmt => Stmt::Exit(pair.into_inner().next().map(expr_from)),
        Rule::return_stmt => Stmt::Return(pair.into_inner().next().map(expr_from)),
        Rule::delete_stmt => {
            let mut it = pair.into_inner();
            let name = it.next().unwrap().as_str().to_string();
            Stmt::Delete(name, it.next().map_or_else(Vec::new, list_from))
        }
        // `print_args` is silent, so the optional child is the expression list.
        Rule::print_stmt => Stmt::Print(pair.into_inner().next().map_or_else(Vec::new, list_from)),
        Rule::printf_stmt => Stmt::Printf(pair.into_inner().next().map_or_else(Vec::new, list_from)),
        Rule::expr_stmt => Stmt::Expr(expr_from(first(pair))),
        r => unreachable!("unexpected stmt {r:?}"),
    }
}

fn parse_for(pair: Pair<Rule>) -> Stmt {
    // The body is the last child in both forms; what precedes it says which.
    let mut parts: Vec<Pair<Rule>> = pair.into_inner().collect();
    let body = Box::new(parse_stmt(parts.pop().unwrap()));
    if parts.first().is_some_and(|p| p.as_rule() == Rule::name) {
        // for (var in array) body
        return Stmt::ForIn(parts[0].as_str().to_string(), parts[1].as_str().to_string(), body);
    }
    // C-style: any of for_init/for_cond/for_update may be absent.
    let mut init = None;
    let mut cond = None;
    let mut update = None;
    for p in parts {
        match p.as_rule() {
            Rule::for_init => init = Some(Box::new(Stmt::Expr(expr_from(p)))),
            Rule::for_cond => cond = Some(expr_from(p)),
            _ => update = Some(Box::new(Stmt::Expr(expr_from(p)))),
        }
    }
    Stmt::For(init, cond, update, body)
}

// ---- Expressions -----------------------------------------------------------

/// AWK's operator precedence, lowest first. Concatenation sits between the
/// comparisons and `+`/`-` and reads the zero-width `concat_op` the grammar
/// emits where no real operator appears, which is what lets a single Pratt table
/// cover an operatorless operator.
static PRATT: LazyLock<PrattParser<Rule>> = LazyLock::new(|| {
    use Assoc::{Left, Right};
    PrattParser::new()
        .op(Op::infix(Rule::or_op, Left))
        .op(Op::infix(Rule::and_op, Left))
        .op(Op::infix(Rule::in_op, Left))
        .op(Op::infix(Rule::match_op, Left))
        .op(Op::infix(Rule::comp_op, Left))
        .op(Op::infix(Rule::concat_op, Left))
        .op(Op::infix(Rule::add_op, Left))
        .op(Op::infix(Rule::mul_op, Left))
        // Between `*` and `^`, so `-a * b` is `(-a) * b` but `-2 ^ 2` is
        // `-(2 ^ 2)` — the one place awk's unary minus surprises people.
        .op(Op::prefix(Rule::un_op))
        .op(Op::infix(Rule::pow_op, Right))
        .op(Op::prefix(Rule::pre_op) | Op::postfix(Rule::post_op))
});

/// The rules that wrap exactly one child when their own syntax is absent, so
/// peeling them generically leaves every arm of `expr_from` a plain
/// construction. `pratt_expr` is not among them: a lone operand is a one-element
/// stream, which the Pratt fold already handles.
fn peelable(r: Rule) -> bool {
    matches!(r, Rule::assignment | Rule::ternary
        | Rule::for_init | Rule::for_cond | Rule::for_update)
}

/// Fold one `pratt_expr`'s flat operand/operator stream by the table above.
fn pratt(pair: Pair<Rule>) -> Expr {
    PRATT
        .map_primary(expr_from)
        // `pre_op` and `un_op` are distinct rules only because the table keys on
        // the rule to tell a prefix `++` from a postfix one; their token sets are
        // disjoint (`un_op` cannot match `++`/`--`), so one match on the text
        // covers both.
        .map_prefix(|op, e| {
            let target = Box::new(e);
            match op.as_str() {
                "++" => Expr::Incr { pre: true, dec: false, target },
                "--" => Expr::Incr { pre: true, dec: true, target },
                "!" => Expr::Unary(UnOp::Not, target),
                "-" => Expr::Unary(UnOp::Neg, target),
                _ => Expr::Unary(UnOp::Pos, target),
            }
        })
        .map_postfix(|e, op| {
            Expr::Incr { pre: false, dec: op.as_str() == "--", target: Box::new(e) }
        })
        .map_infix(|l, op, r| match op.as_rule() {
            Rule::concat_op => Expr::Concat(Box::new(l), Box::new(r)),
            Rule::match_op => Expr::Match(op.as_str() == "!~", Box::new(l), Box::new(r)),
            // `in`'s right operand is a bare array name, which the operand slot
            // parsed as a `var`.
            Rule::in_op => match r {
                Expr::Var(name) => Expr::In(vec![l], name),
                other => unreachable!("`in` applied to {other:?}"),
            },
            _ => Expr::Binary(bin_op(op.as_str()), Box::new(l), Box::new(r)),
        })
        .parse(pair.into_inner())
}

// A flat dispatch over grammar rules; splitting it would only scatter the
// one-to-one correspondence with awk.pest.
fn expr_from(mut pair: Pair<Rule>) -> Expr {
    while peelable(pair.as_rule()) && pair.clone().into_inner().count() == 1 {
        pair = first(pair);
    }
    match pair.as_rule() {
        Rule::pratt_expr => pratt(pair),
        Rule::assignment => {
            let mut it = pair.into_inner();
            let target = next_expr(&mut it);
            // `+=` is `+` then `=`; a bare `=` strips to nothing, the plain form.
            let op = it.next().unwrap().as_str();
            let op = op.strip_suffix('=').filter(|o| !o.is_empty()).map(bin_op);
            Expr::Assign(op, target, next_expr(&mut it))
        }
        Rule::ternary => {
            let mut it = pair.into_inner();
            Expr::Ternary(next_expr(&mut it), next_expr(&mut it), next_expr(&mut it))
        }
        // `(e)` grouping and `(subs) in arr` share a rule so the parenthesized
        // contents are parsed once; the trailing array name is what tells them
        // apart. Without it a valid program's list holds exactly one element.
        Rule::paren_expr => {
            let mut it = pair.into_inner();
            let mut subs = list_from(it.next().unwrap());
            // The `in_op` and the array name follow only in the membership form.
            match it.next_back() {
                Some(arr) => Expr::In(subs, arr.as_str().to_string()),
                None => subs.remove(0),
            }
        }
        Rule::field => Expr::Field(Box::new(expr_from(first(pair)))),
        Rule::getline_expr => {
            Expr::Getline(pair.into_inner().next().map(|p| Box::new(expr_from(p))))
        }
        Rule::call => {
            let mut it = pair.into_inner();
            let name = it.next().unwrap().as_str().to_string();
            let args = it.next().map_or_else(Vec::new, list_from);
            if is_builtin(&name) {
                Expr::Builtin(name, args)
            } else {
                Expr::Call(name, args)
            }
        }
        Rule::array_ref => {
            let mut it = pair.into_inner();
            let name = it.next().unwrap().as_str().to_string();
            Expr::ArrayRef(name, list_from(it.next().unwrap()))
        }
        // Bare `length` is the one built-in usable without parentheses.
        Rule::var if pair.as_str() == "length" => Expr::Builtin("length".to_string(), vec![]),
        Rule::var => Expr::Var(pair.as_str().to_string()),
        Rule::number => Expr::Num(pair.as_str().parse().unwrap()),
        Rule::string => Expr::Str(unescape(first(pair).as_str())),
        Rule::regex => Expr::Regex(first(pair).as_str().to_string()),
        r => unreachable!("unexpected expr {r:?}"),
    }
}

fn list_from(pair: Pair<Rule>) -> Vec<Expr> {
    pair.into_inner().map(expr_from).collect()
}

fn first(pair: Pair<Rule>) -> Pair<Rule> {
    pair.into_inner().next().unwrap()
}

/// The next child of `it` as a boxed expression — the shape most AST nodes want,
/// and what lets the fixed-arity rules be read straight off the iterator instead
/// of collected and indexed.
// Boxed on purpose: every caller stores the result in a `Box<Expr>` field, so
// returning it unboxed would only move the `Box::new` out to the call sites.
#[allow(clippy::unnecessary_box_returns)]
fn next_expr(it: &mut Pairs<Rule>) -> Box<Expr> {
    Box::new(expr_from(it.next().unwrap()))
}

fn bin_op(s: &str) -> BinOp {
    match s {
        "+" => BinOp::Add,
        "-" => BinOp::Sub,
        "*" => BinOp::Mul,
        "/" => BinOp::Div,
        "%" => BinOp::Mod,
        "^" => BinOp::Pow,
        "<" => BinOp::Lt,
        "<=" => BinOp::Le,
        ">" => BinOp::Gt,
        ">=" => BinOp::Ge,
        "==" => BinOp::Eq,
        "!=" => BinOp::Ne,
        "&&" => BinOp::And,
        "||" => BinOp::Or,
        other => unreachable!("bad operator {other:?}"),
    }
}

/// Process AWK string-literal escape sequences, yielding raw bytes.
///
/// Bytes rather than a `String` because an octal escape denotes one arbitrary
/// byte: `"\377"` is a single 0xFF, which is not valid UTF-8 on its own. Source
/// text outside escapes is passed through as its own UTF-8 bytes, matching how
/// a byte-oriented awk in the C locale sees the file.
fn unescape(s: &str) -> Vec<u8> {
    let mut out = Vec::with_capacity(s.len());
    let mut chars = s.chars().peekable();
    let push_char = |out: &mut Vec<u8>, c: char| {
        let mut buf = [0u8; 4];
        out.extend_from_slice(c.encode_utf8(&mut buf).as_bytes());
    };
    while let Some(c) = chars.next() {
        if c != '\\' {
            push_char(&mut out, c);
            continue;
        }
        match chars.next() {
            Some('n') => out.push(b'\n'),
            Some('t') => out.push(b'\t'),
            Some('r') => out.push(b'\r'),
            // An escaped backslash, or a stray one at end of input.
            Some('\\') | None => out.push(b'\\'),
            Some('"') => out.push(b'"'),
            Some('/') => out.push(b'/'),
            Some('a') => out.push(0x07),
            Some('b') => out.push(0x08),
            Some('f') => out.push(0x0C),
            Some('v') => out.push(0x0B),
            Some(d @ '0'..='7') => {
                // Octal escape: up to 3 digits total.
                let mut val = d.to_digit(8).unwrap();
                for _ in 0..2 {
                    if let Some(&n) = chars.peek()
                        && let Some(dd) = n.to_digit(8)
                    {
                        val = val * 8 + dd;
                        chars.next();
                        continue;
                    }
                    break;
                }
                // Three octal digits reach 0o777 (511); keeping only the low
                // byte matches gawk under LC_ALL=C (\400 -> 0, \777 -> 0xFF).
                #[allow(clippy::cast_possible_truncation)]
                out.push(val as u8);
            }
            Some(other) => {
                out.push(b'\\');
                push_char(&mut out, other);
            }
        }
    }
    out
}

#[cfg(test)]
mod tests {
    use super::*;

    fn ok(src: &str) {
        if let Err(e) = parse(src) {
            panic!("failed to parse:\n{src}\n---\n{e}");
        }
    }

    fn bad(src: &str) {
        if let Ok(p) = parse(src) {
            panic!("should not have parsed:\n{src}\n---\n{p:?}");
        }
    }

    #[test]
    fn programs_parse() {
        ok(r#"BEGIN { print "hello" }"#);
        ok("{ print }");
        ok("{ print $1, $2 }");
        ok("$1 > 3 { print $0 }");
        ok("/foo/ { print }");
        ok("NR==1, NR==5 { print }");
        ok("END { print NR }");
        ok("{ sum += $1 } END { print sum }");
        ok("{ a[$1]++ } END { for (k in a) print k, a[k] }");
        ok("BEGIN { for (i=0;i<10;i++) print i }");
        // Every clause of the head is optional, including all of them at once.
        ok("BEGIN { for (;;) break }");
        ok("BEGIN { i=0; for (; i<3;) i++ }");
        ok("BEGIN { x = 1; while (x < 5) { print x; x++ } }");
        ok("BEGIN { i=0; do { print i; i++ } while (i < 3) }");
        ok("BEGIN { if (1) print \"a\"; else print \"b\" }");
        ok("function f(a, b) { return a + b } BEGIN { print f(2,3) }");
        ok("BEGIN { print 1 2 - 3 }");
        ok("BEGIN { print 2^3^2 }");
        ok("BEGIN { print -2^2 }");
        ok("BEGIN { x = \"a\" \"b\" \"c\"; print x }");
        ok("BEGIN { print (1+2)*3 }");
        ok(r#"BEGIN { printf("%d %s\n", 5, "x") }"#);
        ok("BEGIN { n = split(\"a:b:c\", arr, \":\"); print n }");
        ok("BEGIN { print substr(\"hello\", 2, 3) }");
        ok("BEGIN { s=\"foo\"; gsub(/o/, \"0\", s); print s }");
        ok("{ if ($0 ~ /x/) print }");
        ok("BEGIN { print (\"a\" in b) }");
        ok("BEGIN { a[1,2]=3; if ((1,2) in a) print \"yes\" }");
        ok("BEGIN { delete a[1]; delete a }");
        ok("BEGIN { print x>0 ? \"pos\" : \"nonpos\" }");
        ok("BEGIN { print $NF }");
        ok("BEGIN { x=5; print x++ + ++x }");
        ok("BEGIN { print length(\"abc\") length }");
        ok("{ while ((getline line) > 0) print line }");
        ok("# comment\nBEGIN { print 1 } # trailing\n");
        ok("BEGIN {\n  print 1 &&\n  2\n}");
    }

    /// A statement ending in `}` terminates itself, so the next statement may
    /// follow with no `;` or newline in between (POSIX `terminated_statement`).
    #[test]
    fn a_statement_ending_in_a_brace_needs_no_separator() {
        ok(r#"BEGIN { {m=1} print "x" }"#);
        ok(r#"BEGIN { for (i=0;i<1;i++) {x=1} print "x" }"#);
        ok(r#"BEGIN { for (k in a) {x=1} print "x" }"#);
        ok(r#"BEGIN { while (0) {x=1} print "x" }"#);
        ok(r#"BEGIN { if (1) {a=1} print "x" }"#);
        ok(r#"BEGIN { if (1) {a=1} else {b=1} print "x" }"#);
        ok(r#"BEGIN { if (1) {a=1} else if (0) {b=1} print "x" }"#);
        ok(r#"BEGIN { if (1) if (0) {a=1} print "x" }"#);
        ok("BEGIN { {a=1}{b=2}{c=3} }");
        ok("function f() { {a=1} return 2 }");

        // One that does not end in `}` still needs the separator.
        bad("BEGIN { print 1 print 2 }");
        bad("BEGIN { x=1 y=2 }");
        bad("BEGIN { while (0) print 1 print 2 }");
        bad("BEGIN { if (1) {a=1} else print 2 print 3 }");
        bad("BEGIN { exit 1 print 2 }");
        // `do` ends in `)`, so POSIX has it terminated like any simple
        // statement (gawk is laxer here and accepts both of these).
        bad("BEGIN { do a=1 while (0) }");
        bad("BEGIN { do {a=1} while (0) print 2 }");

        // The separator is optional at the end of a block, and repeatable.
        ok("BEGIN { print 1 }");
        ok("BEGIN { ;;; x=1 ;;; }");
        ok("BEGIN {\n\n}");
    }

    /// `else` and the `while` of a `do` are matched where the body statement
    /// may already have ended, so they must not match the head of a name.
    #[test]
    fn else_and_while_do_not_swallow_an_identifier() {
        let program = parse("BEGIN { if (1) x = 1\nelsewhere = 2 }").expect("must parse");
        let action = &program.items[0].action;
        assert_eq!(action.len(), 2, "two statements, not an if/else: {action:?}");
        assert!(matches!(action[0], Stmt::If(_, _, None)), "{action:?}");

        let program = parse("BEGIN { i=0; do i++; while (i<2)\nwhilevar = 3 }")
            .expect("must parse");
        let action = &program.items[0].action;
        assert_eq!(action.len(), 3, "{action:?}");
        assert!(matches!(action[1], Stmt::DoWhile(..)), "{action:?}");

        // The keywords themselves still bind across separators and newlines.
        ok("BEGIN { if (1) print 1; else print 2 }");
        ok("BEGIN { if (1) print 1\nelse print 2 }");
        ok("BEGIN { if (1) { print 1 }\nelse { print 2 } }");
        ok("BEGIN { i=0; do { i++ } while (i<3); print i }");
        ok("BEGIN { i=0\ndo\n  i++\nwhile (i<3)\nprint i }");
    }

    // ---- AST shape (grouping, associativity, the `/` ambiguity) ------------

    /// A compact prefix rendering of an expression, so the tests below assert
    /// on *structure* rather than on the source they came from.
    fn shape(e: &Expr) -> String {
        let list = |es: &[Expr]| es.iter().map(shape).collect::<Vec<_>>().join(" ");
        match e {
            Expr::Num(n) => format!("{n}"),
            Expr::Str(s) => format!("{:?}", String::from_utf8_lossy(s)),
            Expr::Regex(r) => format!("/{r}/"),
            Expr::Var(n) => n.clone(),
            Expr::Field(i) => format!("($ {})", shape(i)),
            Expr::ArrayRef(n, subs) => format!("({n}[] {})", list(subs)),
            Expr::Assign(op, t, v) => {
                let op = op.map_or_else(|| "Assign".to_string(), |o| format!("{o:?}"));
                format!("({op}= {} {})", shape(t), shape(v))
            }
            Expr::Ternary(c, t, f) => format!("(?: {} {} {})", shape(c), shape(t), shape(f)),
            Expr::Binary(op, a, b) => format!("({op:?} {} {})", shape(a), shape(b)),
            Expr::Unary(op, a) => format!("({op:?} {})", shape(a)),
            Expr::Match(neg, a, b) => {
                format!("({} {} {})", if *neg { "!~" } else { "~" }, shape(a), shape(b))
            }
            Expr::In(subs, n) => format!("(in {n} {})", list(subs)),
            Expr::Concat(a, b) => format!("(cat {} {})", shape(a), shape(b)),
            Expr::Incr { pre, dec, target } => {
                let (p, d) = (if *pre { "pre" } else { "post" }, if *dec { "--" } else { "++" });
                format!("({p}{d} {})", shape(target))
            }
            Expr::Call(n, args) => format!("({n} {})", list(args)),
            Expr::Builtin(n, args) => format!("(@{n} {})", list(args)),
            Expr::Getline(None) => "(getline)".to_string(),
            Expr::Getline(Some(t)) => format!("(getline {})", shape(t)),
        }
    }

    /// Parse `src` as an expression statement and render its shape.
    fn ex(src: &str) -> String {
        shape(&crate::testutil::expr(src))
    }

    #[test]
    fn arithmetic_precedence_and_associativity() {
        assert_eq!(ex("1 - 2 - 3"), "(Sub (Sub 1 2) 3)", "left-associative");
        assert_eq!(ex("1 + 2 * 3"), "(Add 1 (Mul 2 3))");
        assert_eq!(ex("(1 + 2) * 3"), "(Mul (Add 1 2) 3)");
        assert_eq!(ex("2 ^ 3 ^ 2"), "(Pow 2 (Pow 3 2))", "right-associative");
        assert_eq!(ex("-2 ^ 2"), "(Neg (Pow 2 2))", "unary minus binds looser than ^");
        assert_eq!(ex("x = y = 1"), "(Assign= x (Assign= y 1))", "right-associative");
        assert_eq!(ex("a && b || c"), "(Or (And a b) c)");
        assert_eq!(ex("!a == b"), "(Eq (Not a) b)");
    }

    #[test]
    fn concatenation_binds_looser_than_arithmetic() {
        // The operatorless-concatenation hazard: `1 2 - 3` is 1 concatenated
        // with (2 - 3), not (1 2) - 3.
        assert_eq!(ex("1 2 - 3"), "(cat 1 (Sub 2 3))");
        assert_eq!(ex(r#""a" "b" "c""#), r#"(cat (cat "a" "b") "c")"#);
        assert_eq!(ex("x = 1 2"), "(Assign= x (cat 1 2))");
    }

    #[test]
    fn slash_is_division_after_a_value_and_a_regex_otherwise() {
        assert_eq!(ex("x = 4 / 2 / 1"), "(Assign= x (Div (Div 4 2) 1))");
        assert_eq!(ex("x ~ /a/"), "(~ x /a/)");
        assert_eq!(ex("x !~ /a/"), "(!~ x /a/)");
        assert_eq!(ex("x = /a/"), "(Assign= x /a/)", "a bare regex means ($0 ~ /a/)");
    }

    #[test]
    fn lvalues_and_calls() {
        assert_eq!(ex("$1"), "($ 1)");
        assert_eq!(ex("$NF"), "($ NF)");
        assert_eq!(ex("$(1 + 1)"), "($ (Add 1 1))");
        assert_eq!(ex("a[1, 2]"), "(a[] 1 2)");
        assert_eq!(ex("(1, 2) in a"), "(in a 1 2)");
        assert_eq!(ex("x++ + ++y"), "(Add (post++ x) (pre++ y))");
        assert_eq!(ex("f(1, 2)"), "(f 1 2)", "a user call");
        assert_eq!(ex(r#"length("a")"#), r#"(@length "a")"#, "a built-in");
        assert_eq!(ex("getline"), "(getline)");
        assert_eq!(ex("getline line"), "(getline line)");
        assert_eq!(ex("c ? 1 : 2"), "(?: c 1 2)");
    }

    /// The zero-width `concat_op` matches wherever no real operator does, so
    /// these are the places it must *not* reach.
    #[test]
    fn the_zero_width_concat_operator_stays_in_its_lane() {
        // A newline still terminates a statement. Were `nl` applied to `infix`
        // as a whole it would follow `concat_op` too, and this would parse as
        // the single statement `x = (1 print)`.
        let program = parse("BEGIN { x = 1\nprint 2 }").expect("must parse");
        let action = &program.items[0].action;
        assert_eq!(action.len(), 2, "assignment then print, not a concat: {action:?}");

        // `in` is tried before `concat_op`, so without a word boundary on the
        // operator `x inside` would read as `x in side`.
        assert_eq!(ex("x inside"), "(cat x inside)");
        assert_eq!(ex("x in side"), "(in side x)");

        // A newline *is* ignored after `&&`/`||`, which sit above concatenation.
        ok("BEGIN {\n  print 1 &&\n  2\n}");
    }

    /// `print (a, b)` is an argument list; `paren_expr` would otherwise claim it
    /// as one operand and silently drop everything after the first element.
    #[test]
    fn parenthesized_print_arguments_are_a_list_not_a_grouping() {
        let action = &parse("BEGIN { print (1, 2) }").expect("must parse").items[0].action;
        let Stmt::Print(args) = &action[0] else { panic!("{action:?}") };
        assert_eq!(args.len(), 2, "two print arguments: {args:?}");

        // A parenthesis that only *starts* the argument list falls through to
        // the bare form, so the lookahead must not claim it.
        let action = &parse("BEGIN { print (1+2)*3 }").expect("must parse").items[0].action;
        let Stmt::Print(args) = &action[0] else { panic!("{action:?}") };
        assert_eq!(args.len(), 1, "one argument: {args:?}");
        assert_eq!(shape(&args[0]), "(Mul (Add 1 2) 3)");
    }

    /// Grouping and `(subs) in arr` share one rule so the parenthesized contents
    /// are parsed once. Ordering them as two alternatives instead re-parses the
    /// inside on every failed membership match, which is exponential in depth —
    /// seconds at depth 20, and minutes not far beyond it.
    #[test]
    fn nested_parentheses_do_not_blow_up() {
        let src = format!("BEGIN {{ x = {}1{} }}", "(".repeat(40), ")".repeat(40));
        let start = std::time::Instant::now();
        ok(&src);
        assert!(start.elapsed().as_secs() < 5, "40-deep parens took {:?}", start.elapsed());
    }

    #[test]
    fn compound_assignment_carries_the_plain_binary_operator() {
        assert_eq!(ex("x = 1"), "(Assign= x 1)");
        assert_eq!(ex("x += 1"), "(Add= x 1)");
        assert_eq!(ex("x ^= 2"), "(Pow= x 2)");
        assert_eq!(ex("a[i] %= 2"), "(Mod= (a[] i) 2)");
        // `==` is not an assignment; `assign_op` must not claim its first `=`.
        assert_eq!(ex("x == 1"), "(Eq x 1)");
    }

    #[test]
    fn patterns_and_functions() {
        let program = parse("NR == 1, NR == 5 { print } function f(a, b) { return a } END { }")
            .expect("must parse");
        assert!(matches!(program.items[0].pattern, Pattern::Range(..)), "{program:?}");
        assert!(matches!(program.items[1].pattern, Pattern::End), "{program:?}");
        assert_eq!(program.functions[0].name, "f");
        assert_eq!(program.functions[0].params, ["a", "b"]);
    }

    // ---- Escape processing -------------------------------------------------

    #[test]
    fn escapes_become_raw_bytes() {
        assert_eq!(unescape("a\\nb"), b"a\nb");
        assert_eq!(unescape("\\t\\r\\\\\\\""), b"\t\r\\\"");
        assert_eq!(unescape("\\101\\60"), b"A0", "octal escapes");
        assert_eq!(unescape("\\377"), [0xff], "a high byte stays one byte");
        assert_eq!(unescape("\\400"), [0x00], "truncated to the low byte");
        assert_eq!(unescape("\\q"), b"\\q", "an unknown escape keeps the backslash");
        assert_eq!(unescape("plain"), b"plain");
    }
}
