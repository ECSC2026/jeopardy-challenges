//! Emit C23 source from the AST.
//!
//! Split in two: [`analysis`] infers each name's nature (scalar vs array), and
//! [`emit`] writes the C that decision selects the types for. This file keeps
//! the shared `Codegen` state and the small literal helpers.

use std::collections::{BTreeSet, HashMap};
// `write!` into a String cannot fail; the unwraps below are unreachable.
use std::fmt::Write as _;

use crate::ast::Program;

mod analysis;
mod emit;

/// The variables the runtime owns: always scalars, never declared here.
fn is_special_scalar(n: &str) -> bool {
    matches!(
        n,
        "NR" | "NF" | "FS" | "OFS" | "ORS" | "RS" | "SUBSEP" | "CONVFMT" | "OFMT"
            | "RSTART" | "RLENGTH"
    )
}

/// A function's parameters in declaration order, each with its inferred nature:
/// `true` is an array (passed by reference), `false` a scalar (passed by value).
type Params = Vec<(String, bool)>;

/// Where the code being emitted sits, which is what decides how `exit` leaves.
#[derive(Clone, Copy)]
enum Site {
    Rule,
    End,
    Func,
}

/// Per-scope context threaded through emission.
struct Ctx<'a> {
    /// The enclosing function's parameters, or empty at top level.
    params: &'a [(String, bool)],
    site: Site,
}

impl Ctx<'_> {
    fn top(site: Site) -> Self {
        Ctx { params: &[], site }
    }

    /// `name`'s nature as a parameter of the enclosing function, if it is one.
    fn local(&self, name: &str) -> Option<bool> {
        self.params.iter().find(|(p, _)| p == name).map(|&(_, arr)| arr)
    }

    /// The scalar parameters, which own the value they were passed and so have
    /// to release it before the function returns.
    fn owned(&self) -> impl Iterator<Item = &str> {
        self.params.iter().filter(|(_, arr)| !arr).map(|(p, _)| p.as_str())
    }
}

#[derive(Default)]
pub struct Codegen {
    global_scalars: BTreeSet<String>,
    global_arrays: BTreeSet<String>,
    funcs: HashMap<String, Params>,
}

pub fn generate(program: &Program) -> String {
    let mut cg = Codegen::default();
    cg.infer(program);
    cg.emit(program)
}

// ---- Literal helpers ------------------------------------------------------

/// Rust's `{}` for f64 is always plain decimal (no exponent), so an integral
/// value comes out as a bare integer form — `1.0` → `1`, `1e20` →
/// `100000000000000000000`. As a C *integer* constant the latter overflows every
/// integer type and is silently wrapped mod 2^64, so append `.0` and let it be
/// an (unconstrained) floating constant with the correct value.
fn fmt_double(n: f64) -> String {
    let s = format!("{n}");
    if s.bytes().all(|b| b.is_ascii_digit() || b == b'-') {
        format!("{s}.0")
    } else {
        s
    }
}

/// Parse `src` and run the inference passes on it, for the submodules' tests.
#[cfg(test)]
fn analyze(src: &str) -> Codegen {
    let program = crate::parser::parse(src).expect("test program must parse");
    let mut cg = Codegen::default();
    cg.infer(&program);
    cg
}

/// Emit C for `src`, for the submodules' tests.
#[cfg(test)]
fn compile(src: &str) -> String {
    let program = crate::parser::parse(src).expect("test program must parse");
    generate(&program)
}

fn c_string_literal(s: &[u8]) -> String {
    let mut out = String::from("\"");
    for b in s.iter().copied() {
        match b {
            b'"' => out.push_str("\\\""),
            b'\\' => out.push_str("\\\\"),
            b'\n' => out.push_str("\\n"),
            b'\t' => out.push_str("\\t"),
            b'\r' => out.push_str("\\r"),
            0x20..=0x7e => out.push(b as char),
            _ => write!(out, "\\{b:03o}").unwrap(),
        }
    }
    out.push('"');
    out
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn doubles_format_as_c_literals() {
        // An integral value needs a decimal point to stay a `double` in C — and
        // beyond the integral range that is what keeps it from becoming an
        // out-of-range C integer constant.
        assert_eq!(fmt_double(1.0), "1.0");
        assert_eq!(fmt_double(-2.0), "-2.0");
        assert_eq!(fmt_double(0.0), "0.0");
        assert_eq!(fmt_double(0.5), "0.5");
        assert_eq!(fmt_double(-0.25), "-0.25");
        assert_eq!(fmt_double(1e15), "1000000000000000.0");
        assert_eq!(fmt_double(1e20), "100000000000000000000.0");
        // A form that is already unambiguously floating is left alone.
        assert_eq!(fmt_double(1e-7), "0.0000001");
    }

    #[test]
    fn string_literals_are_escaped_for_c() {
        assert_eq!(c_string_literal(b"abc"), r#""abc""#);
        assert_eq!(c_string_literal(b"a\"b\\c"), r#""a\"b\\c""#);
        assert_eq!(c_string_literal(b"\n\t\r"), r#""\n\t\r""#);
        // Non-printable and high bytes go out as octal escapes, so the emitted
        // C stays ASCII and byte-exact.
        assert_eq!(c_string_literal(&[0x01, 0xff]), r#""\001\377""#);
    }
}
