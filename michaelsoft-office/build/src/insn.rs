use std::{collections::HashMap, sync::Mutex};

use crate::util::ident;

#[derive(Debug, Copy, Clone, Hash, PartialEq, Eq)]
pub enum Reg {
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
}

#[derive(Debug, Copy, Clone, Hash, PartialEq, Eq)]
pub enum Loc {
    Mem(u8),
    Reg(Reg),
}

static NAMES: Mutex<Option<HashMap<Loc, Vec<String>>>> = Mutex::new(None);

impl Loc {
    pub fn props(&self) -> Vec<String> {
        let mut guard = NAMES.lock().unwrap();
        let map = guard.get_or_insert_default();
        if let Some(names) = map.get(self) {
            names.clone()
        } else {
            let name = ident("loc_");
            let names = (0..8)
                .map(|i| format!("{}_{}", name, i))
                .collect::<Vec<_>>();
            map.insert(*self, names.clone());
            names
        }
    }
}

pub const MEMORY_SIZE: u8 = 128;

#[derive(Debug)]
pub enum Insn {
    Label(&'static str),
    Read(Loc),                          // 0 = input
    WriteIndirect(Loc, Loc),            // [0] = 1
    ReadIndirect(Loc, Loc),             // 0 = [1]
    Mov(Loc, Loc),                      // 0 = 1
    SetImm(Loc, u8),                    // 0 = 1
    Add(Loc, Loc, Loc),                 // 0 = 1 + 2
    AddImm(Loc, Loc, u8),               // 0 = 1 + 2
    Xor(Loc, Loc, Loc),                 // 0 = 1 ^ 2
    XorImm(Loc, Loc, u8),               // 0 = 1 ^ 2
    Or(Loc, Loc, Loc),                  // 0 = 1 | 2
    Swizzle(Loc, Loc, [Option<u8>; 8]), // 0 = 1[swizzle[0]] + 2*1[swizzle[1]] + ...
    Print(Loc),                         // print 0
    Jmp(&'static str),                  // jump to 0
    Jeq(&'static str, Loc, Loc),        // if 1 == 2 jump to 0
    JeqImm(&'static str, Loc, u8),      // if 1 == 2 jump to 0
    JneImm(&'static str, Loc, u8),      // if 1 != 2 jump to 0
    Exit(&'static str, bool), // show 0, then exit if bool is false, or continue install if bool is true
    FourBitMultiply(Loc, Loc, Loc), // 0 = 1 * 2 (4x4->8 bit multiply)
    Call(&'static str),       // call 0
    Ret,                      // return from call
}

pub struct InsnBuilder {
    insns: Vec<Insn>,
}

impl InsnBuilder {
    pub fn new() -> Self {
        Self { insns: Vec::new() }
    }

    pub fn add(&mut self, insn: Insn) {
        self.insns.push(insn);
    }

    pub fn into_inner(self) -> Vec<Insn> {
        self.insns
    }
}
