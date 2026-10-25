//! Nature inference over the AST: which names are scalars and which are arrays.
//! It fills in `Codegen`'s inferred state for [`super::emit`].
//!
//! This is a *representation* decision, not an optimization: a name's nature
//! picks `AwkValue` or `AwkArray`, two different C types with two different
//! calling conventions (arrays pass by reference), so without it even a valid
//! program emits C that does not compile.
//!
//! The lattice has two points — scalar below array — and a location only ever
//! moves upward, which is what makes the fixpoint below terminate. It is sound
//! only for programs the awk oracle itself accepts: one name used as both a
//! scalar and an array is out of scope, and raising it to array is this pass
//! making a choice, not diagnosing an error.
//!
//! Two steps. [`Walk`] visits every scope once and records the *syntactic*
//! evidence — `a[i]`, `in`, `delete`, `for..in`, `split` make a name an array,
//! any other mention defaults it to scalar — along with every call site. Then
//! [`Codegen::propagate_nature`] flows nature across those call sites to a
//! fixpoint, which is what resolves the names no single scope decides.
//!
//! The walk is flow-insensitive: it never prunes, so statements after a `next`
//! or `return` are visited like any other, since a name's nature holds there
//! just the same.

use crate::ast::{Expr, Pattern, Program, Stmt};

// This module is the analysis half of `codegen`, so it deliberately works
// against the parent's whole state and helper surface.
#[allow(clippy::wildcard_imports)]
use super::*;

impl Codegen {
    pub(super) fn infer<'p>(&mut self, program: &'p Program) {
        // Registered up front so a call can be resolved before its definition.
        for f in &program.functions {
            let params = f.params.iter().map(|p| (p.clone(), false)).collect();
            self.funcs.insert(f.name.clone(), params);
        }
        let mut calls: Vec<CallSite<'p>> = Vec::new();
        for f in &program.functions {
            Walk { cg: self, func: Some(&f.name), calls: &mut calls }.block(&f.body);
        }
        for item in &program.items {
            let mut w = Walk { cg: self, func: None, calls: &mut calls };
            match &item.pattern {
                Pattern::Expr(e) => w.expr(e),
                Pattern::Range(a, b) => {
                    w.expr(a);
                    w.expr(b);
                }
                Pattern::Begin | Pattern::End | Pattern::Always => {}
            }
            w.block(&item.action);
        }
        self.propagate_nature(&calls);
    }

    /// Record a global's nature. Array always wins, whichever order the two
    /// verdicts arrive in; the runtime's own variables are never declared here.
    fn mark_global(&mut self, name: &str, is_arr: bool) {
        if is_special_scalar(name) {
            return;
        }
        if is_arr {
            self.global_scalars.remove(name);
            self.global_arrays.insert(name.to_string());
        } else if !self.global_arrays.contains(name) {
            self.global_scalars.insert(name.to_string());
        }
    }

    /// Whether `name` is a parameter of `f`, and where.
    fn param_pos(&self, f: &str, name: &str) -> Option<usize> {
        self.funcs.get(f)?.iter().position(|(p, _)| p == name)
    }

    /// Resolve scalar-vs-array nature across call boundaries, to a fixpoint.
    ///
    /// Local use alone leaves a variable's nature undetermined in two common
    /// situations: the "extra params as locals" idiom, where a scratch array is
    /// declared as a trailing parameter and only the callee ever subscripts it,
    /// and a caller that does nothing with a variable but hand it to a function.
    /// Either side can be the one that knows, so passing a name to a parameter
    /// makes the two *share* a nature: whenever they disagree, both are raised
    /// to array. Only a bare name can carry nature — anything else is
    /// necessarily a scalar expression. It terminates because a location only
    /// ever moves scalar -> array, and each round that changes nothing is last.
    fn propagate_nature(&mut self, calls: &[CallSite]) {
        let mut changed = true;
        while changed {
            changed = false;
            for cs in calls {
                for (i, arg) in cs.args.iter().enumerate() {
                    // Only a bare name carries nature, and a special variable is
                    // a scalar whatever it is passed to.
                    let Some(name) = *arg else { continue };
                    if is_special_scalar(name) {
                        continue;
                    }
                    // No parameter to share with: an excess argument, or a call
                    // to a function that is not defined.
                    let Some(&(_, param_arr)) =
                        self.funcs.get(cs.callee).and_then(|ps| ps.get(i))
                    else {
                        continue;
                    };
                    // The argument's own nature lives with the caller's
                    // parameter if it is one, else with the global.
                    let pos = cs.caller.and_then(|f| self.param_pos(f, name));
                    let arg_arr = match pos {
                        Some(j) => self.funcs[cs.caller.unwrap()][j].1,
                        None => self.global_arrays.contains(name),
                    };
                    if param_arr == arg_arr {
                        continue;
                    }
                    // They disagree, so one side knows: raise both.
                    self.funcs.get_mut(cs.callee).unwrap()[i].1 = true;
                    match pos {
                        Some(j) => self.funcs.get_mut(cs.caller.unwrap()).unwrap()[j].1 = true,
                        None => self.mark_global(name, true),
                    }
                    changed = true;
                }
            }
        }
    }
}

/// One call to a user-defined function, kept for interprocedural propagation.
struct CallSite<'p> {
    callee: &'p str,
    /// Enclosing function, or `None` for a top-level rule.
    caller: Option<&'p str>,
    /// Per argument, the bare variable name when the argument is one. Only a
    /// bare name can carry array nature across the boundary.
    args: Vec<Option<&'p str>>,
}

/// The fact-collecting walk of one scope: a function body, or a rule's pattern
/// and action.
struct Walk<'a, 'p> {
    cg: &'a mut Codegen,
    /// The enclosing function, whose parameters shadow the globals.
    func: Option<&'p str>,
    /// Shared across scopes: the whole program's call sites land here.
    calls: &'a mut Vec<CallSite<'p>>,
}

impl<'p> Walk<'_, 'p> {
    /// Record `name`'s nature in the scope being walked.
    fn mark(&mut self, name: &str, is_arr: bool) {
        if let Some(f) = self.func
            && let Some(p) = self.cg.param_pos(f, name)
        {
            self.cg.funcs.get_mut(f).unwrap()[p].1 |= is_arr;
            return;
        }
        self.cg.mark_global(name, is_arr);
    }

    fn block(&mut self, stmts: &'p [Stmt]) {
        for s in stmts {
            self.stmt(s);
        }
    }

    fn stmt(&mut self, s: &'p Stmt) {
        match s {
            Stmt::Expr(e) => self.expr(e),
            Stmt::Print(es) | Stmt::Printf(es) => self.exprs(es),
            Stmt::Delete(name, subs) => {
                self.mark(name, true);
                self.exprs(subs);
            }
            Stmt::If(cond, then, els) => {
                self.expr(cond);
                self.stmt(then);
                if let Some(e) = els {
                    self.stmt(e);
                }
            }
            Stmt::While(cond, body) | Stmt::DoWhile(body, cond) => {
                self.expr(cond);
                self.stmt(body);
            }
            Stmt::For(init, cond, update, body) => {
                for s in [init, update].into_iter().flatten() {
                    self.stmt(s);
                }
                if let Some(c) = cond {
                    self.expr(c);
                }
                self.stmt(body);
            }
            Stmt::ForIn(var, array, body) => {
                self.mark(var, false);
                self.mark(array, true);
                self.stmt(body);
            }
            Stmt::Block(stmts) => self.block(stmts),
            Stmt::Exit(e) | Stmt::Return(e) => {
                if let Some(e) = e {
                    self.expr(e);
                }
            }
            Stmt::Next | Stmt::Break | Stmt::Continue => {}
        }
    }

    fn exprs(&mut self, es: &'p [Expr]) {
        for e in es {
            self.expr(e);
        }
    }

    fn expr(&mut self, e: &'p Expr) {
        match e {
            Expr::Var(name) => self.mark(name, false),
            Expr::ArrayRef(name, subs) | Expr::In(subs, name) => {
                self.mark(name, true);
                self.exprs(subs);
            }
            Expr::Call(name, args) => {
                // Record the site; `propagate_nature` resolves the argument
                // natures once every scope's local pass has run.
                self.calls.push(CallSite {
                    callee: name,
                    caller: self.func,
                    args: args
                        .iter()
                        .map(|a| match a {
                            Expr::Var(n) => Some(n.as_str()),
                            _ => None,
                        })
                        .collect(),
                });
                // A bare name is still walked: `mark` only defaults it to
                // scalar, so an array verdict from the call site overrides it.
                self.exprs(args);
            }
            // `split`'s second argument is the array it fills, not a value.
            Expr::Builtin(name, args) if name == "split" && args.len() >= 2 => {
                if let Expr::Var(arr) = &args[1] {
                    self.mark(arr, true);
                }
                self.expr(&args[0]);
                self.exprs(&args[2..]);
            }
            Expr::Builtin(_, args) => self.exprs(args),
            Expr::Assign(_, a, b) | Expr::Binary(_, a, b) | Expr::Concat(a, b)
            | Expr::Match(_, a, b) => {
                self.expr(a);
                self.expr(b);
            }
            Expr::Field(a) | Expr::Unary(_, a) | Expr::Incr { target: a, .. }
            | Expr::Getline(Some(a)) => self.expr(a),
            Expr::Ternary(cond, then, els) => {
                self.expr(cond);
                self.expr(then);
                self.expr(els);
            }
            Expr::Num(_) | Expr::Str(_) | Expr::Regex(_) | Expr::Getline(None) => {}
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn nature_flows_from_callee_to_caller() {
        // Only the callee subscripts the name, so the caller's `y` is an array.
        let cg = analyze("function f(a, b) { b[1] = 1 } BEGIN { f(x, y) }");
        assert_eq!(cg.funcs["f"], [("a".to_string(), false), ("b".to_string(), true)]);
        assert!(cg.global_arrays.contains("y"));
        assert!(cg.global_scalars.contains("x"));
    }

    #[test]
    fn nature_flows_from_caller_to_callee() {
        // Only the caller knows; the parameter must follow.
        let cg = analyze("function f(p) { return 1 } BEGIN { a[1] = 1; f(a) }");
        assert!(cg.funcs["f"][0].1, "{:?}", cg.funcs["f"]);
    }

    #[test]
    fn nature_chains_through_calls() {
        let cg = analyze(
            "function inner(p) { p[1] = 1 }
             function outer(q) { inner(q) }
             BEGIN { outer(z) }",
        );
        assert!(cg.funcs["outer"][0].1, "{:?}", cg.funcs["outer"]);
        assert!(cg.global_arrays.contains("z"), "nature reached the caller two levels up");
    }

    #[test]
    fn syntactic_array_uses_are_recognized() {
        let cg = analyze(
            "BEGIN { split(\"a:b\", parts, \":\"); delete d[1]; for (k in m) n = k; if (1 in q) x = 1 }",
        );
        for name in ["parts", "d", "m", "q"] {
            assert!(cg.global_arrays.contains(name), "{name} should be an array");
        }
        for name in ["k", "n", "x"] {
            assert!(cg.global_scalars.contains(name), "{name} should be a scalar");
        }
    }

    /// Every statement and expression position has to be walked, or a name used
    /// as an array only there would be declared an `AwkValue` and the emitted C
    /// would not compile.
    #[test]
    fn the_walk_reaches_every_position() {
        let cg = analyze(
            "$0 ~ pat1[1], $0 ~ pat2[1] { if (c[1]) t[1]; else e[1]
                 while (w[1]) wb[1]
                 do db[1]; while (dw[1])
                 for (fi[1]; fc[1]; fu[1]) fb[1]
                 for (k in fin) fib[1]
                 { blk[1] }
                 print p[1]; printf \"%s\", pf[1]
                 delete del[sub1[1]]
                 x = a[1] b[1] (c1[1] < c2[1]) (m1[1] ~ m2[1])
                 y = -u[1] + (t1[1] ? t2[1] : t3[1]) + $f[1] + i1[1]++ + length(bi[1]) + fn(ca[1])
                 getline gl[1]
                 return r[1] }",
        );
        for name in [
            "pat1", "pat2", "c", "t", "e", "w", "wb", "db", "dw", "fi", "fc", "fu", "fb", "fin",
            "fib", "blk", "p", "pf", "del", "sub1", "a", "b", "c1", "c2", "m1", "m2", "u", "t1",
            "t2", "t3", "f", "i1", "bi", "ca", "gl", "r",
        ] {
            assert!(cg.global_arrays.contains(name), "{name} was not walked");
        }
    }

    #[test]
    fn unreachable_code_is_walked_too() {
        // A flow-insensitive collector must still see statements it could prove
        // unreachable — nature facts hold there just the same.
        let cg = analyze("{ next; a[1] } { exit; b[1] }");
        assert!(cg.global_arrays.contains("a") && cg.global_arrays.contains("b"));
    }
}
