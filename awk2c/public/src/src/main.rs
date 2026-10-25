//! awk2c — translate AWK source to C. Emits C to stdout; never invokes `cc`.

mod ast;
mod codegen;
mod parser;
#[cfg(test)]
mod testutil;

use anyhow::{bail, Context, Result};
use std::env;
use std::fs;

fn main() -> Result<()> {
    let args: Vec<String> = env::args().skip(1).collect();
    let [path] = args.as_slice() else {
        bail!("usage: awk2c progfile");
    };

    let program_text = fs::read_to_string(path).with_context(|| format!("reading {path}"))?;
    let program = parser::parse(&program_text)?;
    let c_source = codegen::generate(&program);
    print!("{c_source}");
    Ok(())
}
