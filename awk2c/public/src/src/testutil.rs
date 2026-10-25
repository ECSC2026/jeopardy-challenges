//! Test-only helpers shared by the module test suites: each of them wants a
//! program, a rule body or a single expression out of a snippet of AWK.

use crate::ast::{Expr, Program, Stmt};

/// Parse `src`, panicking with the parser's diagnostic if it does not.
pub fn program(src: &str) -> Program {
    crate::parser::parse(src).unwrap_or_else(|e| panic!("must parse:\n{src}\n---\n{e}"))
}

/// The statements of the program's first rule.
pub fn action(p: &Program) -> &[Stmt] {
    &p.items[0].action
}

/// The expression of `BEGIN { <src> }`.
pub fn expr(src: &str) -> Expr {
    let p = program(&format!("BEGIN {{ {src} }}"));
    match &action(&p)[0] {
        Stmt::Expr(e) => e.clone(),
        other => panic!("not an expression statement: {other:?}"),
    }
}
