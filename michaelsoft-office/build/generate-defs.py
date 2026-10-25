# Given an msi file extracted by Orca, generate Rust definitions for the tables in the msi file
# to have some type safety when reading and writing msi files.
import json
import glob
import pathlib
import sys

def type_to_rust(ty):
    ty = ty.lower()
    if ty.startswith("s") or ty.startswith("l"):
        return "String"
    elif ty.startswith("i"):
        return "i" + str(int(ty[1:]) * 8)
    else:
        return "Binary"

def parse_table_descs(idt_path):
    content = open(idt_path, 'r').read()
    lines = content.splitlines()

    rows = [l.split("\t") for l in lines[3:]]
    return [{
        "table": meta[0],
        "column": meta[1],
        "description": meta[9],
    } for meta in rows]

def generate_defs(idt_path, descs):
    content = open(idt_path, 'r').read()
    lines = content.splitlines()

    columns = lines[0].split("\t")
    types = lines[1].split("\t")
    meta = lines[2].split("\t")

    has_binary = any(type_to_rust(ty) == "Binary" for ty in types)
    if has_binary:
        return  # Skip tables with binary columns

    table_name = meta[0]

    print(f"#[derive(Debug, Default, Clone)]")
    print(f"pub struct {table_name} {{")
    for col, ty in zip(columns, types):
        desc = next((d["description"] for d in descs if d["table"] == table_name and d["column"] == col), "")
        if desc:
            print(f"    /// {desc}")
        rust_type = type_to_rust(ty)
        print(f"    pub {col}: {rust_type},")
    print(f"}}\n")

    print(f"impl {table_name} {{")
    print(f"    #[rustfmt::skip]")
    print(f"    pub fn new(", end="")
    print(", ".join(f"{col}: impl Into<{type_to_rust(ty)}>" for col, ty in zip(columns, types)), end="")
    print(f") -> Self {{")
    print(f"        Self {{")
    for col in columns:
        print(f"            {col}: {col}.into(),")
    print(f"        }}")
    print(f"    }}")
    print(f"}}\n")

    print(f"impl TableRow for {table_name} {{")
    print(f"    const NAME: &'static str = {json.dumps(table_name)};")
    print(f"    const COLUMNS: &'static str = {json.dumps(lines[0])};")
    print(f"    const TYPES: &'static str = {json.dumps(lines[1])};")
    print(f"    const META: &'static str = {json.dumps(lines[2])};")
    print("")
    print(f"    #[rustfmt::skip]")
    print(f"    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {{")
    for i, (col, ty) in enumerate(zip(columns, types)):
        suff = "\\r\\n" if i == len(columns) - 1 else "\\t"
        if type_to_rust(ty) == "String":
            print(f"        write!(writer, \"{{}}{suff}\", escape(self.{col}.clone()))?;")
        else:
            print(f"        write!(writer, \"{{}}{suff}\", self.{col})?;")
    print(f"        Ok(())")
    print(f"    }}")
    print("")
    print(f"    #[rustfmt::skip]")
    print(f"    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {{")
    print(f"        let mut line = String::new();")
    print(f"        reader.read_line(&mut line)?;")
    print(f"        let parts: Vec<&str> = line.trim_end_matches(&['\\r', '\\n']).split('\\t').collect();")
    print(f"        Ok(Self {{")
    for i, (col, ty) in enumerate(zip(columns, types)):
        if type_to_rust(ty) == "String":
            print(f"            {col}: unescape(parts[{i}].to_string()),")
        else:
            print(f"            {col}: parts[{i}].parse().unwrap(),")
    print(f"        }})")
    print(f"    }}")
    print(f"}}\n")

print(f"#![allow(non_snake_case, unused, dead_code)]")
print(f"/// This code is automatically @generated. Do not edit.")
print(f"use crate::rw::{{TableRow, escape, unescape}};\n")

descs = parse_table_descs(pathlib.Path(sys.argv[1]) / "_Validation.idt")
for idt_file in glob.glob(str(pathlib.Path(sys.argv[1]) / "*.idt")):
    generate_defs(idt_file, descs)