use eyre::Result;
use std::collections::HashMap;
use std::path::PathBuf;

use clap::Parser;

use crate::emit::emit_insn;
use crate::insn::{Insn, InsnBuilder, Loc, Reg};
use crate::table as t;
use crate::util::idents;

mod emit;
mod insn;
mod rw;
mod table;
mod tables;
mod util;

#[derive(Debug, Parser)]
struct Args {
    /// Input MSI file to graft things onto.
    input_msi: PathBuf,

    /// Output MSI file to write the grafted result to.
    output_msi: PathBuf,
}

const MSIDB: &str = "C:\\Program Files (x86)\\Windows Kits\\10\\bin\\10.0.19041.0\\x86\\MsiDb.exe";

fn program() -> Vec<Insn> {
    use crate::Reg::*;
    use Insn::*;
    use Loc::*;
    let mut i = InsnBuilder::new();

    // read input
    let counter = Reg(A);
    let one = Reg(B);
    let char = Reg(C);
    i.add(SetImm(counter, 0));
    i.add(SetImm(one, 1));
    i.add(Label("readchar"));
    i.add(Read(char));
    i.add(JeqImm("read_done", char, 0));
    i.add(WriteIndirect(counter, char));
    i.add(Add(counter, counter, one));
    i.add(Jmp("readchar"));

    i.add(Label("read_done"));
    // for (idx, c) in "ECSC{y0u_c4n_g0_ah3ad_4nd_un1nst4ll_n0w1814e85}"
    //     .chars()
    //     .enumerate()
    // {
    //     i.add(SetImm(Mem(idx as u8), c as u8));
    // }

    i.add(Add(Mem(32), Mem(32), Mem(10)));
    i.add(Swizzle(
        Mem(27),
        Mem(27),
        [
            Some(4),
            Some(1),
            Some(5),
            Some(2),
            Some(0),
            Some(6),
            Some(3),
            Some(7),
        ],
    ));
    i.add(SetImm(Reg(A), 190));
    i.add(Xor(Mem(10), Mem(10), Reg(A)));
    i.add(Swizzle(
        Mem(31),
        Mem(31),
        [
            Some(6),
            Some(3),
            Some(0),
            Some(2),
            Some(5),
            Some(4),
            Some(1),
            Some(7),
        ],
    ));
    i.add(Xor(Mem(11), Mem(11), Mem(22)));
    i.add(Add(Mem(10), Mem(10), Mem(37)));
    i.add(Xor(Mem(31), Mem(31), Mem(9)));
    i.add(Add(Mem(3), Mem(3), Mem(27)));
    i.add(Xor(Mem(19), Mem(19), Mem(16)));
    i.add(SetImm(Reg(A), 73));
    i.add(Xor(Mem(2), Mem(2), Reg(A)));
    i.add(Xor(Mem(26), Mem(26), Mem(14)));
    i.add(SetImm(Reg(A), 129));
    i.add(Add(Mem(25), Mem(25), Reg(A)));
    i.add(Xor(Mem(3), Mem(3), Mem(18)));
    i.add(SetImm(Reg(A), 229));
    i.add(Add(Mem(9), Mem(9), Reg(A)));
    i.add(Swizzle(
        Mem(32),
        Mem(32),
        [
            Some(7),
            Some(5),
            Some(2),
            Some(3),
            Some(0),
            Some(1),
            Some(4),
            Some(6),
        ],
    ));
    i.add(Xor(Mem(41), Mem(41), Mem(9)));
    i.add(SetImm(Reg(A), 111));
    i.add(Add(Mem(23), Mem(23), Reg(A)));
    i.add(Swizzle(
        Mem(4),
        Mem(4),
        [
            Some(5),
            Some(3),
            Some(6),
            Some(7),
            Some(0),
            Some(1),
            Some(2),
            Some(4),
        ],
    ));
    i.add(SetImm(Reg(A), 234));
    i.add(Add(Mem(36), Mem(36), Reg(A)));
    i.add(Xor(Mem(0), Mem(0), Mem(46)));
    i.add(Swizzle(
        Mem(8),
        Mem(8),
        [
            Some(7),
            Some(4),
            Some(3),
            Some(0),
            Some(1),
            Some(6),
            Some(5),
            Some(2),
        ],
    ));
    i.add(SetImm(Reg(A), 160));
    i.add(Xor(Mem(25), Mem(25), Reg(A)));
    i.add(Swizzle(
        Mem(5),
        Mem(5),
        [
            Some(6),
            Some(1),
            Some(2),
            Some(3),
            Some(5),
            Some(4),
            Some(0),
            Some(7),
        ],
    ));
    i.add(SetImm(Reg(A), 30));
    i.add(Xor(Mem(45), Mem(45), Reg(A)));
    i.add(Xor(Mem(34), Mem(34), Mem(14)));
    i.add(Add(Mem(7), Mem(7), Mem(40)));
    i.add(Xor(Mem(25), Mem(25), Mem(37)));
    i.add(Swizzle(
        Mem(5),
        Mem(5),
        [
            Some(5),
            Some(1),
            Some(3),
            Some(0),
            Some(4),
            Some(6),
            Some(2),
            Some(7),
        ],
    ));
    i.add(SetImm(Reg(A), 37));
    i.add(Add(Mem(4), Mem(4), Reg(A)));
    i.add(Xor(Mem(38), Mem(38), Mem(6)));
    i.add(Xor(Mem(41), Mem(41), Mem(27)));
    i.add(Add(Mem(21), Mem(21), Mem(39)));
    i.add(SetImm(Reg(A), 124));
    i.add(Xor(Mem(18), Mem(18), Reg(A)));
    i.add(Xor(Mem(2), Mem(2), Mem(41)));
    i.add(SetImm(Reg(A), 113));
    i.add(Add(Mem(34), Mem(34), Reg(A)));
    i.add(Xor(Mem(11), Mem(11), Mem(45)));
    i.add(Add(Mem(6), Mem(6), Mem(10)));
    i.add(SetImm(Reg(A), 126));
    i.add(Xor(Mem(1), Mem(1), Reg(A)));
    i.add(SetImm(Reg(A), 192));
    i.add(Xor(Mem(41), Mem(41), Reg(A)));
    i.add(Xor(Mem(6), Mem(6), Mem(13)));
    i.add(Swizzle(
        Mem(0),
        Mem(0),
        [
            Some(4),
            Some(0),
            Some(7),
            Some(5),
            Some(3),
            Some(1),
            Some(6),
            Some(2),
        ],
    ));
    i.add(SetImm(Reg(A), 221));
    i.add(Add(Mem(13), Mem(13), Reg(A)));
    i.add(Xor(Mem(36), Mem(36), Mem(33)));
    i.add(SetImm(Reg(A), 216));
    i.add(Add(Mem(15), Mem(15), Reg(A)));
    i.add(Xor(Mem(41), Mem(41), Mem(27)));
    i.add(FourBitMultiply(Mem(51), Mem(12), Mem(39)));
    i.add(Swizzle(
        Mem(40),
        Mem(40),
        [
            Some(4),
            Some(3),
            Some(5),
            Some(2),
            Some(6),
            Some(7),
            Some(0),
            Some(1),
        ],
    ));
    i.add(Xor(Mem(20), Mem(20), Mem(23)));
    i.add(Add(Mem(10), Mem(10), Mem(36)));
    i.add(SetImm(Reg(A), 173));
    i.add(Xor(Mem(6), Mem(6), Reg(A)));
    i.add(Add(Mem(41), Mem(41), Mem(29)));
    i.add(Add(Mem(36), Mem(36), Mem(7)));
    i.add(Add(Mem(6), Mem(6), Mem(26)));
    i.add(SetImm(Reg(A), 185));
    i.add(Add(Mem(33), Mem(33), Reg(A)));
    i.add(FourBitMultiply(Mem(58), Mem(8), Mem(36)));
    i.add(Xor(Mem(43), Mem(43), Mem(41)));
    i.add(Swizzle(
        Mem(22),
        Mem(22),
        [
            Some(5),
            Some(3),
            Some(2),
            Some(4),
            Some(0),
            Some(7),
            Some(6),
            Some(1),
        ],
    ));
    i.add(Xor(Mem(1), Mem(1), Mem(3)));
    i.add(SetImm(Reg(A), 214));
    i.add(Add(Mem(8), Mem(8), Reg(A)));
    i.add(Add(Mem(2), Mem(2), Mem(10)));
    i.add(Add(Mem(27), Mem(27), Mem(39)));
    i.add(SetImm(Reg(A), 94));
    i.add(Xor(Mem(30), Mem(30), Reg(A)));
    i.add(FourBitMultiply(Mem(60), Mem(12), Mem(39)));
    i.add(Add(Mem(22), Mem(22), Mem(10)));
    i.add(Xor(Mem(2), Mem(2), Mem(6)));
    i.add(Swizzle(
        Mem(45),
        Mem(45),
        [
            Some(2),
            Some(1),
            Some(6),
            Some(5),
            Some(7),
            Some(0),
            Some(4),
            Some(3),
        ],
    ));
    i.add(Add(Mem(46), Mem(46), Mem(17)));
    i.add(Swizzle(
        Mem(7),
        Mem(7),
        [
            Some(6),
            Some(4),
            Some(3),
            Some(2),
            Some(0),
            Some(1),
            Some(7),
            Some(5),
        ],
    ));
    i.add(Xor(Mem(27), Mem(27), Mem(22)));
    i.add(Add(Mem(6), Mem(6), Mem(27)));
    i.add(Xor(Mem(18), Mem(18), Mem(9)));
    i.add(Xor(Mem(32), Mem(32), Mem(44)));
    i.add(SetImm(Reg(A), 245));
    i.add(Add(Mem(20), Mem(20), Reg(A)));
    i.add(Add(Mem(13), Mem(13), Mem(28)));
    i.add(Swizzle(
        Mem(37),
        Mem(37),
        [
            Some(7),
            Some(1),
            Some(2),
            Some(6),
            Some(0),
            Some(4),
            Some(5),
            Some(3),
        ],
    ));
    i.add(FourBitMultiply(Mem(69), Mem(36), Mem(8)));
    i.add(SetImm(Reg(A), 191));
    i.add(Add(Mem(9), Mem(9), Reg(A)));
    i.add(Add(Mem(40), Mem(40), Mem(39)));
    i.add(Xor(Mem(33), Mem(33), Mem(5)));
    i.add(Xor(Mem(11), Mem(11), Mem(27)));
    i.add(Xor(Mem(40), Mem(40), Mem(45)));
    i.add(Xor(Mem(43), Mem(43), Mem(23)));
    i.add(SetImm(Reg(A), 133));
    i.add(Xor(Mem(9), Mem(9), Reg(A)));
    i.add(Add(Mem(33), Mem(33), Mem(30)));
    i.add(Swizzle(
        Mem(31),
        Mem(31),
        [
            Some(1),
            Some(5),
            Some(3),
            Some(2),
            Some(7),
            Some(4),
            Some(6),
            Some(0),
        ],
    ));
    i.add(FourBitMultiply(Mem(63), Mem(14), Mem(45)));
    i.add(SetImm(Reg(A), 119));
    i.add(Xor(Mem(38), Mem(38), Reg(A)));
    i.add(SetImm(Reg(A), 217));
    i.add(Xor(Mem(19), Mem(19), Reg(A)));
    i.add(Add(Mem(20), Mem(20), Mem(1)));
    i.add(Xor(Mem(6), Mem(6), Mem(8)));
    i.add(SetImm(Reg(A), 88));
    i.add(Add(Mem(29), Mem(29), Reg(A)));
    i.add(SetImm(Reg(A), 42));
    i.add(Xor(Mem(19), Mem(19), Reg(A)));
    i.add(FourBitMultiply(Mem(64), Mem(17), Mem(10)));
    i.add(Add(Mem(39), Mem(39), Mem(34)));
    i.add(Swizzle(
        Mem(29),
        Mem(29),
        [
            Some(3),
            Some(0),
            Some(1),
            Some(6),
            Some(2),
            Some(4),
            Some(5),
            Some(7),
        ],
    ));
    i.add(Swizzle(
        Mem(19),
        Mem(19),
        [
            Some(2),
            Some(3),
            Some(4),
            Some(0),
            Some(7),
            Some(5),
            Some(1),
            Some(6),
        ],
    ));
    i.add(SetImm(Reg(A), 34));
    i.add(Xor(Mem(11), Mem(11), Reg(A)));
    i.add(Add(Mem(21), Mem(21), Mem(31)));
    i.add(SetImm(Reg(A), 216));
    i.add(Xor(Mem(1), Mem(1), Reg(A)));
    i.add(Swizzle(
        Mem(10),
        Mem(10),
        [
            Some(7),
            Some(0),
            Some(4),
            Some(2),
            Some(1),
            Some(5),
            Some(3),
            Some(6),
        ],
    ));
    i.add(Swizzle(
        Mem(16),
        Mem(16),
        [
            Some(1),
            Some(5),
            Some(0),
            Some(6),
            Some(4),
            Some(7),
            Some(2),
            Some(3),
        ],
    ));
    i.add(Add(Mem(18), Mem(18), Mem(15)));
    i.add(Xor(Mem(26), Mem(26), Mem(40)));
    i.add(Swizzle(
        Mem(42),
        Mem(42),
        [
            Some(6),
            Some(3),
            Some(4),
            Some(5),
            Some(7),
            Some(0),
            Some(2),
            Some(1),
        ],
    ));
    i.add(Xor(Mem(12), Mem(12), Mem(41)));
    i.add(FourBitMultiply(Mem(59), Mem(0), Mem(15)));
    i.add(FourBitMultiply(Mem(57), Mem(43), Mem(36)));
    i.add(Add(Mem(31), Mem(31), Mem(30)));
    i.add(Xor(Mem(31), Mem(31), Mem(21)));
    i.add(SetImm(Reg(A), 29));
    i.add(Xor(Mem(44), Mem(44), Reg(A)));
    i.add(Swizzle(
        Mem(38),
        Mem(38),
        [
            Some(3),
            Some(6),
            Some(5),
            Some(2),
            Some(1),
            Some(4),
            Some(7),
            Some(0),
        ],
    ));
    i.add(SetImm(Reg(A), 215));
    i.add(Xor(Mem(23), Mem(23), Reg(A)));
    i.add(SetImm(Reg(A), 141));
    i.add(Xor(Mem(4), Mem(4), Reg(A)));
    i.add(Xor(Mem(4), Mem(4), Mem(14)));
    i.add(Swizzle(
        Mem(6),
        Mem(6),
        [
            Some(3),
            Some(7),
            Some(2),
            Some(5),
            Some(0),
            Some(6),
            Some(1),
            Some(4),
        ],
    ));
    i.add(SetImm(Reg(A), 134));
    i.add(Xor(Mem(1), Mem(1), Reg(A)));
    i.add(Swizzle(
        Mem(29),
        Mem(29),
        [
            Some(6),
            Some(4),
            Some(2),
            Some(1),
            Some(5),
            Some(7),
            Some(3),
            Some(0),
        ],
    ));
    i.add(Add(Mem(3), Mem(3), Mem(1)));
    i.add(Swizzle(
        Mem(29),
        Mem(29),
        [
            Some(1),
            Some(2),
            Some(7),
            Some(3),
            Some(4),
            Some(0),
            Some(6),
            Some(5),
        ],
    ));
    i.add(SetImm(Reg(A), 195));
    i.add(Add(Mem(37), Mem(37), Reg(A)));
    i.add(SetImm(Reg(A), 238));
    i.add(Add(Mem(30), Mem(30), Reg(A)));
    i.add(SetImm(Reg(A), 170));
    i.add(Add(Mem(32), Mem(32), Reg(A)));
    i.add(FourBitMultiply(Mem(65), Mem(32), Mem(18)));
    i.add(SetImm(Reg(A), 219));
    i.add(Xor(Mem(46), Mem(46), Reg(A)));
    i.add(Xor(Mem(23), Mem(23), Mem(24)));
    i.add(Add(Mem(46), Mem(46), Mem(40)));
    i.add(SetImm(Reg(A), 35));
    i.add(Add(Mem(45), Mem(45), Reg(A)));
    i.add(Xor(Mem(29), Mem(29), Mem(37)));
    i.add(Swizzle(
        Mem(11),
        Mem(11),
        [
            Some(7),
            Some(5),
            Some(2),
            Some(0),
            Some(4),
            Some(6),
            Some(1),
            Some(3),
        ],
    ));
    i.add(SetImm(Reg(A), 250));
    i.add(Add(Mem(24), Mem(24), Reg(A)));
    i.add(Swizzle(
        Mem(24),
        Mem(24),
        [
            Some(3),
            Some(0),
            Some(2),
            Some(6),
            Some(7),
            Some(1),
            Some(5),
            Some(4),
        ],
    ));
    i.add(Xor(Mem(43), Mem(43), Mem(37)));
    i.add(Xor(Mem(38), Mem(38), Mem(45)));
    i.add(Swizzle(
        Mem(15),
        Mem(15),
        [
            Some(5),
            Some(7),
            Some(6),
            Some(4),
            Some(3),
            Some(0),
            Some(2),
            Some(1),
        ],
    ));
    i.add(FourBitMultiply(Mem(68), Mem(41), Mem(25)));
    i.add(Xor(Mem(6), Mem(6), Mem(34)));
    i.add(Swizzle(
        Mem(10),
        Mem(10),
        [
            Some(2),
            Some(4),
            Some(5),
            Some(6),
            Some(3),
            Some(7),
            Some(1),
            Some(0),
        ],
    ));
    i.add(Add(Mem(36), Mem(36), Mem(43)));
    i.add(SetImm(Reg(A), 71));
    i.add(Add(Mem(4), Mem(4), Reg(A)));
    i.add(Add(Mem(35), Mem(35), Mem(1)));
    i.add(SetImm(Reg(A), 33));
    i.add(Xor(Mem(19), Mem(19), Reg(A)));
    i.add(Xor(Mem(25), Mem(25), Mem(18)));
    i.add(Xor(Mem(0), Mem(0), Mem(21)));
    i.add(SetImm(Reg(A), 118));
    i.add(Add(Mem(44), Mem(44), Reg(A)));
    i.add(Swizzle(
        Mem(24),
        Mem(24),
        [
            Some(4),
            Some(1),
            Some(3),
            Some(0),
            Some(7),
            Some(6),
            Some(5),
            Some(2),
        ],
    ));
    i.add(Add(Mem(33), Mem(33), Mem(16)));
    i.add(Add(Mem(14), Mem(14), Mem(16)));
    i.add(Xor(Mem(25), Mem(25), Mem(0)));
    i.add(Xor(Mem(28), Mem(28), Mem(44)));
    i.add(SetImm(Reg(A), 59));
    i.add(Xor(Mem(41), Mem(41), Reg(A)));
    i.add(Swizzle(
        Mem(24),
        Mem(24),
        [
            Some(5),
            Some(6),
            Some(0),
            Some(7),
            Some(4),
            Some(2),
            Some(3),
            Some(1),
        ],
    ));
    i.add(Xor(Mem(14), Mem(14), Mem(42)));
    i.add(Swizzle(
        Mem(46),
        Mem(46),
        [
            Some(4),
            Some(5),
            Some(0),
            Some(7),
            Some(6),
            Some(2),
            Some(3),
            Some(1),
        ],
    ));
    i.add(Swizzle(
        Mem(45),
        Mem(45),
        [
            Some(6),
            Some(2),
            Some(7),
            Some(3),
            Some(1),
            Some(5),
            Some(4),
            Some(0),
        ],
    ));
    i.add(SetImm(Reg(A), 24));
    i.add(Xor(Mem(27), Mem(27), Reg(A)));
    i.add(Swizzle(
        Mem(28),
        Mem(28),
        [
            Some(3),
            Some(4),
            Some(2),
            Some(5),
            Some(1),
            Some(7),
            Some(6),
            Some(0),
        ],
    ));
    i.add(Xor(Mem(27), Mem(27), Mem(22)));
    i.add(SetImm(Reg(A), 73));
    i.add(Xor(Mem(12), Mem(12), Reg(A)));
    i.add(SetImm(Reg(A), 58));
    i.add(Xor(Mem(21), Mem(21), Reg(A)));
    i.add(SetImm(Reg(A), 208));
    i.add(Xor(Mem(38), Mem(38), Reg(A)));
    i.add(SetImm(Reg(A), 55));
    i.add(Xor(Mem(44), Mem(44), Reg(A)));
    i.add(Swizzle(
        Mem(23),
        Mem(23),
        [
            Some(4),
            Some(7),
            Some(2),
            Some(0),
            Some(3),
            Some(5),
            Some(1),
            Some(6),
        ],
    ));
    i.add(SetImm(Reg(A), 192));
    i.add(Xor(Mem(34), Mem(34), Reg(A)));
    i.add(Xor(Mem(40), Mem(40), Mem(4)));
    i.add(Add(Mem(19), Mem(19), Mem(32)));
    i.add(Xor(Mem(26), Mem(26), Mem(2)));
    i.add(Swizzle(
        Mem(5),
        Mem(5),
        [
            Some(2),
            Some(4),
            Some(7),
            Some(6),
            Some(1),
            Some(0),
            Some(3),
            Some(5),
        ],
    ));
    i.add(Add(Mem(37), Mem(37), Mem(8)));
    i.add(Swizzle(
        Mem(5),
        Mem(5),
        [
            Some(1),
            Some(6),
            Some(5),
            Some(2),
            Some(4),
            Some(7),
            Some(3),
            Some(0),
        ],
    ));
    i.add(SetImm(Reg(A), 162));
    i.add(Add(Mem(29), Mem(29), Reg(A)));
    i.add(FourBitMultiply(Mem(67), Mem(5), Mem(0)));
    i.add(Add(Mem(1), Mem(1), Mem(13)));
    i.add(SetImm(Reg(A), 61));
    i.add(Xor(Mem(10), Mem(10), Reg(A)));
    i.add(Add(Mem(14), Mem(14), Mem(18)));
    i.add(Add(Mem(16), Mem(16), Mem(42)));
    i.add(SetImm(Reg(A), 18));
    i.add(Xor(Mem(7), Mem(7), Reg(A)));
    i.add(Xor(Mem(34), Mem(34), Mem(1)));
    i.add(SetImm(Reg(A), 246));
    i.add(Xor(Mem(28), Mem(28), Reg(A)));
    i.add(Xor(Mem(41), Mem(41), Mem(25)));
    i.add(SetImm(Reg(A), 183));
    i.add(Xor(Mem(45), Mem(45), Reg(A)));
    i.add(SetImm(Reg(A), 22));
    i.add(Add(Mem(31), Mem(31), Reg(A)));
    i.add(SetImm(Reg(A), 191));
    i.add(Add(Mem(8), Mem(8), Reg(A)));
    i.add(Swizzle(
        Mem(35),
        Mem(35),
        [
            Some(2),
            Some(0),
            Some(4),
            Some(6),
            Some(7),
            Some(1),
            Some(5),
            Some(3),
        ],
    ));
    i.add(Swizzle(
        Mem(42),
        Mem(42),
        [
            Some(5),
            Some(3),
            Some(6),
            Some(1),
            Some(7),
            Some(2),
            Some(0),
            Some(4),
        ],
    ));
    i.add(SetImm(Reg(A), 206));
    i.add(Add(Mem(0), Mem(0), Reg(A)));
    i.add(Xor(Mem(22), Mem(22), Mem(9)));
    i.add(SetImm(Reg(A), 47));
    i.add(Xor(Mem(22), Mem(22), Reg(A)));
    i.add(Swizzle(
        Mem(26),
        Mem(26),
        [
            Some(7),
            Some(2),
            Some(1),
            Some(0),
            Some(3),
            Some(4),
            Some(6),
            Some(5),
        ],
    ));
    i.add(SetImm(Reg(A), 33));
    i.add(Add(Mem(32), Mem(32), Reg(A)));
    i.add(Swizzle(
        Mem(17),
        Mem(17),
        [
            Some(7),
            Some(4),
            Some(6),
            Some(2),
            Some(0),
            Some(5),
            Some(3),
            Some(1),
        ],
    ));
    i.add(Add(Mem(24), Mem(24), Mem(45)));
    i.add(SetImm(Reg(A), 95));
    i.add(Xor(Mem(25), Mem(25), Reg(A)));
    i.add(Xor(Mem(10), Mem(10), Mem(44)));
    i.add(SetImm(Reg(A), 150));
    i.add(Add(Mem(22), Mem(22), Reg(A)));
    i.add(Xor(Mem(12), Mem(12), Mem(22)));
    i.add(SetImm(Reg(A), 192));
    i.add(Add(Mem(42), Mem(42), Reg(A)));
    i.add(FourBitMultiply(Mem(50), Mem(21), Mem(17)));
    i.add(SetImm(Reg(A), 241));
    i.add(Xor(Mem(29), Mem(29), Reg(A)));
    i.add(Xor(Mem(39), Mem(39), Mem(11)));
    i.add(FourBitMultiply(Mem(52), Mem(37), Mem(45)));
    i.add(Add(Mem(18), Mem(18), Mem(33)));
    i.add(Xor(Mem(39), Mem(39), Mem(38)));
    i.add(Add(Mem(10), Mem(10), Mem(38)));
    i.add(SetImm(Reg(A), 74));
    i.add(Xor(Mem(28), Mem(28), Reg(A)));
    i.add(Add(Mem(12), Mem(12), Mem(38)));
    i.add(Swizzle(
        Mem(3),
        Mem(3),
        [
            Some(1),
            Some(3),
            Some(6),
            Some(0),
            Some(5),
            Some(7),
            Some(4),
            Some(2),
        ],
    ));
    i.add(FourBitMultiply(Mem(48), Mem(41), Mem(24)));
    i.add(SetImm(Reg(A), 195));
    i.add(Add(Mem(45), Mem(45), Reg(A)));
    i.add(SetImm(Reg(A), 97));
    i.add(Xor(Mem(28), Mem(28), Reg(A)));
    i.add(Add(Mem(16), Mem(16), Mem(6)));
    i.add(FourBitMultiply(Mem(55), Mem(31), Mem(27)));
    i.add(SetImm(Reg(A), 188));
    i.add(Add(Mem(36), Mem(36), Reg(A)));
    i.add(Xor(Mem(3), Mem(3), Mem(40)));
    i.add(Xor(Mem(9), Mem(9), Mem(26)));
    i.add(Add(Mem(5), Mem(5), Mem(35)));
    i.add(Add(Mem(45), Mem(45), Mem(4)));
    i.add(Add(Mem(30), Mem(30), Mem(31)));
    i.add(Xor(Mem(0), Mem(0), Mem(15)));
    i.add(SetImm(Reg(A), 4));
    i.add(Xor(Mem(18), Mem(18), Reg(A)));
    i.add(Swizzle(
        Mem(25),
        Mem(25),
        [
            Some(7),
            Some(0),
            Some(3),
            Some(2),
            Some(1),
            Some(4),
            Some(6),
            Some(5),
        ],
    ));
    i.add(Add(Mem(41), Mem(41), Mem(20)));
    i.add(SetImm(Reg(A), 202));
    i.add(Xor(Mem(4), Mem(4), Reg(A)));
    i.add(SetImm(Reg(A), 155));
    i.add(Xor(Mem(3), Mem(3), Reg(A)));
    i.add(Swizzle(
        Mem(29),
        Mem(29),
        [
            Some(6),
            Some(4),
            Some(3),
            Some(0),
            Some(1),
            Some(5),
            Some(2),
            Some(7),
        ],
    ));
    i.add(Swizzle(
        Mem(16),
        Mem(16),
        [
            Some(7),
            Some(4),
            Some(5),
            Some(3),
            Some(2),
            Some(6),
            Some(0),
            Some(1),
        ],
    ));
    i.add(Xor(Mem(13), Mem(13), Mem(33)));
    i.add(Add(Mem(17), Mem(17), Mem(1)));
    i.add(SetImm(Reg(A), 139));
    i.add(Add(Mem(17), Mem(17), Reg(A)));
    i.add(SetImm(Reg(A), 214));
    i.add(Xor(Mem(2), Mem(2), Reg(A)));
    i.add(SetImm(Reg(A), 80));
    i.add(Add(Mem(18), Mem(18), Reg(A)));
    i.add(Swizzle(
        Mem(42),
        Mem(42),
        [
            Some(4),
            Some(6),
            Some(2),
            Some(5),
            Some(1),
            Some(0),
            Some(7),
            Some(3),
        ],
    ));
    i.add(SetImm(Reg(A), 223));
    i.add(Add(Mem(6), Mem(6), Reg(A)));
    i.add(SetImm(Reg(A), 99));
    i.add(Xor(Mem(10), Mem(10), Reg(A)));
    i.add(Add(Mem(39), Mem(39), Mem(29)));
    i.add(Add(Mem(17), Mem(17), Mem(26)));
    i.add(SetImm(Reg(A), 204));
    i.add(Add(Mem(38), Mem(38), Reg(A)));
    i.add(Add(Mem(16), Mem(16), Mem(39)));
    i.add(Swizzle(
        Mem(36),
        Mem(36),
        [
            Some(5),
            Some(6),
            Some(0),
            Some(4),
            Some(2),
            Some(1),
            Some(3),
            Some(7),
        ],
    ));
    i.add(FourBitMultiply(Mem(49), Mem(0), Mem(12)));
    i.add(SetImm(Reg(A), 135));
    i.add(Add(Mem(33), Mem(33), Reg(A)));
    i.add(SetImm(Reg(A), 246));
    i.add(Xor(Mem(0), Mem(0), Reg(A)));
    i.add(SetImm(Reg(A), 36));
    i.add(Add(Mem(6), Mem(6), Reg(A)));
    i.add(Xor(Mem(43), Mem(43), Mem(46)));
    i.add(Add(Mem(35), Mem(35), Mem(34)));
    i.add(SetImm(Reg(A), 115));
    i.add(Xor(Mem(40), Mem(40), Reg(A)));
    i.add(SetImm(Reg(A), 42));
    i.add(Add(Mem(38), Mem(38), Reg(A)));
    i.add(Add(Mem(20), Mem(20), Mem(2)));
    i.add(SetImm(Reg(A), 197));
    i.add(Add(Mem(35), Mem(35), Reg(A)));
    i.add(Add(Mem(17), Mem(17), Mem(46)));
    i.add(SetImm(Reg(A), 39));
    i.add(Add(Mem(35), Mem(35), Reg(A)));
    i.add(Add(Mem(45), Mem(45), Mem(38)));
    i.add(Add(Mem(42), Mem(42), Mem(38)));
    i.add(Xor(Mem(20), Mem(20), Mem(5)));
    i.add(SetImm(Reg(A), 170));
    i.add(Xor(Mem(25), Mem(25), Reg(A)));
    i.add(FourBitMultiply(Mem(56), Mem(36), Mem(17)));
    i.add(SetImm(Reg(A), 41));
    i.add(Add(Mem(29), Mem(29), Reg(A)));
    i.add(Add(Mem(18), Mem(18), Mem(2)));
    i.add(Add(Mem(5), Mem(5), Mem(23)));
    i.add(Swizzle(
        Mem(0),
        Mem(0),
        [
            Some(3),
            Some(6),
            Some(0),
            Some(2),
            Some(4),
            Some(5),
            Some(7),
            Some(1),
        ],
    ));
    i.add(SetImm(Reg(A), 148));
    i.add(Xor(Mem(15), Mem(15), Reg(A)));
    i.add(Xor(Mem(16), Mem(16), Mem(4)));
    i.add(FourBitMultiply(Mem(66), Mem(40), Mem(38)));
    i.add(FourBitMultiply(Mem(61), Mem(3), Mem(24)));
    i.add(FourBitMultiply(Mem(47), Mem(44), Mem(11)));
    i.add(SetImm(Reg(A), 225));
    i.add(Add(Mem(21), Mem(21), Reg(A)));
    i.add(SetImm(Reg(A), 29));
    i.add(Xor(Mem(34), Mem(34), Reg(A)));
    i.add(SetImm(Reg(A), 197));
    i.add(Xor(Mem(12), Mem(12), Reg(A)));
    i.add(Xor(Mem(2), Mem(2), Mem(35)));
    i.add(Swizzle(
        Mem(35),
        Mem(35),
        [
            Some(6),
            Some(0),
            Some(1),
            Some(7),
            Some(2),
            Some(3),
            Some(4),
            Some(5),
        ],
    ));
    i.add(Add(Mem(40), Mem(40), Mem(14)));
    i.add(FourBitMultiply(Mem(54), Mem(19), Mem(42)));
    i.add(Add(Mem(12), Mem(12), Mem(18)));
    i.add(Add(Mem(6), Mem(6), Mem(17)));
    i.add(SetImm(Reg(A), 204));
    i.add(Xor(Mem(18), Mem(18), Reg(A)));
    i.add(Xor(Mem(38), Mem(38), Mem(6)));
    i.add(Add(Mem(42), Mem(42), Mem(8)));
    i.add(Xor(Mem(44), Mem(44), Mem(41)));
    i.add(Swizzle(
        Mem(45),
        Mem(45),
        [
            Some(1),
            Some(4),
            Some(2),
            Some(6),
            Some(7),
            Some(5),
            Some(0),
            Some(3),
        ],
    ));
    i.add(SetImm(Reg(A), 152));
    i.add(Xor(Mem(45), Mem(45), Reg(A)));
    i.add(SetImm(Reg(A), 221));
    i.add(Xor(Mem(24), Mem(24), Reg(A)));
    i.add(SetImm(Reg(A), 4));
    i.add(Xor(Mem(38), Mem(38), Reg(A)));
    i.add(Add(Mem(26), Mem(26), Mem(40)));
    i.add(Xor(Mem(7), Mem(7), Mem(10)));
    i.add(SetImm(Reg(A), 159));
    i.add(Add(Mem(31), Mem(31), Reg(A)));
    i.add(Add(Mem(34), Mem(34), Mem(44)));
    i.add(FourBitMultiply(Mem(53), Mem(35), Mem(6)));
    i.add(SetImm(Reg(A), 199));
    i.add(Xor(Mem(12), Mem(12), Reg(A)));
    i.add(Xor(Mem(46), Mem(46), Mem(28)));
    i.add(Swizzle(
        Mem(0),
        Mem(0),
        [
            Some(6),
            Some(1),
            Some(0),
            Some(4),
            Some(2),
            Some(5),
            Some(7),
            Some(3),
        ],
    ));
    i.add(Xor(Mem(0), Mem(0), Mem(20)));
    i.add(Add(Mem(25), Mem(25), Mem(6)));
    i.add(SetImm(Reg(A), 66));
    i.add(Xor(Mem(16), Mem(16), Reg(A)));
    i.add(Swizzle(
        Mem(46),
        Mem(46),
        [
            Some(2),
            Some(1),
            Some(7),
            Some(3),
            Some(5),
            Some(6),
            Some(4),
            Some(0),
        ],
    ));
    i.add(SetImm(Reg(A), 234));
    i.add(Xor(Mem(32), Mem(32), Reg(A)));
    i.add(SetImm(Reg(A), 122));
    i.add(Xor(Mem(45), Mem(45), Reg(A)));
    i.add(Xor(Mem(20), Mem(20), Mem(23)));
    i.add(Xor(Mem(33), Mem(33), Mem(9)));
    i.add(SetImm(Reg(A), 26));
    i.add(Add(Mem(28), Mem(28), Reg(A)));
    i.add(Xor(Mem(21), Mem(21), Mem(33)));
    i.add(SetImm(Reg(A), 150));
    i.add(Xor(Mem(19), Mem(19), Reg(A)));
    i.add(SetImm(Reg(A), 134));
    i.add(Add(Mem(11), Mem(11), Reg(A)));
    i.add(Swizzle(
        Mem(44),
        Mem(44),
        [
            Some(6),
            Some(3),
            Some(5),
            Some(0),
            Some(7),
            Some(4),
            Some(2),
            Some(1),
        ],
    ));
    i.add(FourBitMultiply(Mem(62), Mem(45), Mem(15)));
    i.add(Swizzle(
        Mem(46),
        Mem(46),
        [
            Some(0),
            Some(4),
            Some(3),
            Some(7),
            Some(5),
            Some(1),
            Some(2),
            Some(6),
        ],
    ));
    i.add(Xor(Mem(46), Mem(46), Mem(14)));

    i.add(JneImm("reject", Mem(0), 171));
    i.add(JneImm("reject", Mem(1), 86));
    i.add(JneImm("reject", Mem(2), 7));
    i.add(JneImm("reject", Mem(3), 188));
    i.add(JneImm("reject", Mem(4), 98));
    i.add(JneImm("reject", Mem(5), 163));
    i.add(JneImm("reject", Mem(6), 49));
    i.add(JneImm("reject", Mem(7), 197));
    i.add(JneImm("reject", Mem(8), 83));
    i.add(JneImm("reject", Mem(9), 218));
    i.add(JneImm("reject", Mem(10), 11));
    i.add(JneImm("reject", Mem(11), 228));
    i.add(JneImm("reject", Mem(12), 199));
    i.add(JneImm("reject", Mem(13), 38));
    i.add(JneImm("reject", Mem(14), 176));
    i.add(JneImm("reject", Mem(15), 125));
    i.add(JneImm("reject", Mem(16), 87));
    i.add(JneImm("reject", Mem(17), 182));
    i.add(JneImm("reject", Mem(18), 225));
    i.add(JneImm("reject", Mem(19), 175));
    i.add(JneImm("reject", Mem(20), 238));
    i.add(JneImm("reject", Mem(21), 176));
    i.add(JneImm("reject", Mem(22), 236));
    i.add(JneImm("reject", Mem(23), 244));
    i.add(JneImm("reject", Mem(24), 111));
    i.add(JneImm("reject", Mem(25), 118));
    i.add(JneImm("reject", Mem(26), 81));
    i.add(JneImm("reject", Mem(27), 135));
    i.add(JneImm("reject", Mem(28), 254));
    i.add(JneImm("reject", Mem(29), 100));
    i.add(JneImm("reject", Mem(30), 152));
    i.add(JneImm("reject", Mem(31), 28));
    i.add(JneImm("reject", Mem(32), 151));
    i.add(JneImm("reject", Mem(33), 0));
    i.add(JneImm("reject", Mem(34), 205));
    i.add(JneImm("reject", Mem(35), 55));
    i.add(JneImm("reject", Mem(36), 78));
    i.add(JneImm("reject", Mem(37), 183));
    i.add(JneImm("reject", Mem(38), 26));
    i.add(JneImm("reject", Mem(39), 212));
    i.add(JneImm("reject", Mem(40), 249));
    i.add(JneImm("reject", Mem(41), 43));
    i.add(JneImm("reject", Mem(42), 224));
    i.add(JneImm("reject", Mem(43), 165));
    i.add(JneImm("reject", Mem(44), 216));
    i.add(JneImm("reject", Mem(45), 249));
    i.add(JneImm("reject", Mem(46), 111));
    i.add(JneImm("reject", Mem(47), 168));
    i.add(JneImm("reject", Mem(48), 28));
    i.add(JneImm("reject", Mem(49), 18));
    i.add(JneImm("reject", Mem(50), 60));
    i.add(JneImm("reject", Mem(51), 15));
    i.add(JneImm("reject", Mem(52), 56));
    i.add(JneImm("reject", Mem(53), 7));
    i.add(JneImm("reject", Mem(54), 117));
    i.add(JneImm("reject", Mem(55), 91));
    i.add(JneImm("reject", Mem(56), 84));
    i.add(JneImm("reject", Mem(57), 15));
    i.add(JneImm("reject", Mem(58), 14));
    i.add(JneImm("reject", Mem(59), 63));
    i.add(JneImm("reject", Mem(60), 15));
    i.add(JneImm("reject", Mem(61), 24));
    i.add(JneImm("reject", Mem(62), 117));
    i.add(JneImm("reject", Mem(63), 0));
    i.add(JneImm("reject", Mem(64), 112));
    i.add(JneImm("reject", Mem(65), 168));
    i.add(JneImm("reject", Mem(66), 135));
    i.add(JneImm("reject", Mem(67), 84));
    i.add(JneImm("reject", Mem(68), 0));
    i.add(JneImm("reject", Mem(69), 4));
    i.add(Exit(
        "Thanks for proving your mortality. You may now proceed with the installation.",
        true,
    ));
    i.add(Label("reject"));
    i.add(Exit("Your identity as a human could not be verified.", false));

    i.into_inner()
}

fn main() -> Result<()> {
    let args = Args::parse();

    let src_tempdir = tempfile::tempdir()?;
    println!(
        "Extracting {} to {}",
        args.input_msi.display(),
        src_tempdir.path().display()
    );
    std::fs::copy(&args.input_msi, src_tempdir.path().join("input.msi"))?;
    std::process::Command::new(MSIDB)
        .arg("-s")
        .arg("-d")
        .arg(src_tempdir.path().join("input.msi"))
        .arg("-f")
        .arg(&src_tempdir.path())
        .arg("-e")
        .arg("*")
        .current_dir(&src_tempdir)
        .status()?;

    let mut tm = tables::TableManager::new(src_tempdir.path().to_path_buf());

    let prog = program();

    // allocate labels for all instructions
    let mut labels = (0..prog.len())
        .map(|i| format!("Step{}", i))
        .collect::<Vec<_>>(); //; idents("insn_", prog.len());
    let mut named_labels = HashMap::new();
    for (i, insn) in prog.iter().enumerate() {
        if let Insn::Label(name) = insn {
            named_labels.insert(name.to_string(), labels[i + 1].clone());
            labels[i] = labels[i + 1].clone();
        }
    }

    // emit actual instructions
    for (i, insn) in prog.into_iter().enumerate() {
        let label = &labels[i];

        match insn {
            Insn::Label(_) => {}

            _ => {
                let next_label = if i + 1 < labels.len() {
                    &labels[i + 1]
                } else {
                    ""
                };
                emit_insn(&mut tm, label, &insn, next_label, &named_labels);
            }
        }
    }

    // hook up to form
    // tm.table_rows::<t::Property>()
    //     .iter_mut()
    //     .find(|x| x.Property == "WelcomeForm_NextArgs")
    //     .unwrap()
    //     .Value = labels[0].clone();
    tm.insert(t::Property {
        Property: format!("ConfirmInstallForm_NextArgs"),
        Value: labels[0].clone(),
    });

    let out_tempdir = tempfile::tempdir()?;
    tm.write(out_tempdir.path())?;

    println!(
        "Repacking {} to {}",
        out_tempdir.path().display(),
        args.output_msi.display()
    );
    std::fs::copy(&args.input_msi, &args.output_msi)?;
    std::process::Command::new(MSIDB)
        .arg("-d")
        .arg(&args.output_msi)
        .arg("-f")
        .arg(&out_tempdir.path())
        .arg("-i")
        .arg("*")
        .status()?;

    Ok(())
}
