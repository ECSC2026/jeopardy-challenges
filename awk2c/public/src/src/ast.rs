//! Lightweight AST for the AWK language.

#[derive(Debug, Clone)]
pub struct Program {
    pub items: Vec<Item>,
    pub functions: Vec<Function>,
}

/// One pattern-action rule.
#[derive(Debug, Clone)]
pub struct Item {
    pub pattern: Pattern,
    pub action: Vec<Stmt>,
}

#[derive(Debug, Clone)]
pub struct Function {
    pub name: String,
    pub params: Vec<String>,
    pub body: Vec<Stmt>,
}

#[derive(Debug, Clone)]
pub enum Pattern {
    Begin,
    End,
    Always,
    Expr(Expr),
    Range(Expr, Expr),
}

#[derive(Debug, Clone)]
pub enum Stmt {
    Expr(Expr),
    Print(Vec<Expr>),
    Printf(Vec<Expr>),
    If(Expr, Box<Stmt>, Option<Box<Stmt>>),
    While(Expr, Box<Stmt>),
    DoWhile(Box<Stmt>, Expr),
    /// C-style for: init, cond, update, body.
    For(Option<Box<Stmt>>, Option<Expr>, Option<Box<Stmt>>, Box<Stmt>),
    /// for (var in array) body
    ForIn(String, String, Box<Stmt>),
    Block(Vec<Stmt>),
    Next,
    Exit(Option<Expr>),
    Return(Option<Expr>),
    Break,
    Continue,
    /// delete arr[subs]  (empty subs => delete whole array)
    Delete(String, Vec<Expr>),
}

#[derive(Debug, Clone)]
pub enum Expr {
    Num(f64),
    /// String literal after escape processing. Held as raw bytes: octal escapes
    /// such as `\377` denote single bytes that are not valid UTF-8 on their own.
    Str(Vec<u8>),
    /// ERE literal /re/ — as a value it means ($0 ~ /re/).
    Regex(String),
    Var(String),
    Field(Box<Expr>),
    /// arr[i, j, ...]
    ArrayRef(String, Vec<Expr>),
    /// `target = value`, or a compound `target op= value` when the operator is
    /// present — `+=` *is* `+` then `=`, so it needs no enum of its own.
    Assign(Option<BinOp>, Box<Expr>, Box<Expr>),
    Ternary(Box<Expr>, Box<Expr>, Box<Expr>),
    Binary(BinOp, Box<Expr>, Box<Expr>),
    Unary(UnOp, Box<Expr>),
    /// `~` / `!~`; bool is true for `!~`.
    Match(bool, Box<Expr>, Box<Expr>),
    /// (subs) in arr
    In(Vec<Expr>, String),
    Concat(Box<Expr>, Box<Expr>),
    /// `++`/`--` on an lvalue: `pre` selects the prefix form (which yields the
    /// new value) over the postfix one (which yields the old); `dec` selects
    /// `--` over `++`. Every consumer but codegen treats the four alike.
    Incr { pre: bool, dec: bool, target: Box<Expr> },
    /// User-defined function call.
    Call(String, Vec<Expr>),
    /// Built-in function call (name is a recognized builtin).
    Builtin(String, Vec<Expr>),
    /// getline forms (in-scope: plain and into-var, from main input).
    Getline(Option<Box<Expr>>),
}

/// Whether `name` is an AWK built-in — which is how the parser tells a built-in
/// call from a user call, so the set of recognized names lives here only.
/// Unrecognized names are not a panic; codegen rejects them separately.
#[must_use]
pub fn is_builtin(name: &str) -> bool {
    matches!(
        name,
        "length" | "index" | "split" | "sub" | "gsub" | "match" | "sin" | "cos" | "atan2"
            | "exp" | "log" | "sqrt" | "int" | "rand" | "srand" | "system" | "close" | "fflush"
            | "substr" | "toupper" | "tolower" | "sprintf"
    )
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum BinOp {
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Pow,
    Lt,
    Le,
    Gt,
    Ge,
    Eq,
    Ne,
    And,
    Or,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum UnOp {
    Neg, // -
    Pos, // +
    Not, // !
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn builtins_are_known_by_name() {
        for name in ["length", "split", "substr", "sprintf"] {
            assert!(is_builtin(name), "{name}");
        }
        // A name that is not a built-in at all: the parser reads this as a user
        // call, and codegen rejects an unknown one separately.
        assert!(!is_builtin("nosuchfunction"));
    }
}
