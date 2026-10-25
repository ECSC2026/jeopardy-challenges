use std::collections::HashMap;

use crate::insn::{Insn, Loc, MEMORY_SIZE};
use crate::table as t;
use crate::util::{all, idents};
use crate::{tables::TableManager, util::ident};

fn xor(a: &str, b: &str) -> String {
    format!("(({a} AND NOT {b}) OR (NOT {a} AND {b}))")
}

pub static CONTROL_COUNTER: std::sync::atomic::AtomicI32 = std::sync::atomic::AtomicI32::new(0);

pub fn emit_insn(
    db: &mut TableManager,
    name: &str,
    insn: &Insn,
    fallthrough: &str,
    labels: &HashMap<String, String>,
) {
    CONTROL_COUNTER.store(0, std::sync::atomic::Ordering::SeqCst);
    let win = basic_window(db, name);

    match insn {
        Insn::SetImm(reg, imm) => {
            let clicked_prop = ident("imm_clicked_");

            default_btn(
                db,
                &win,
                9,
                90i16,
                &format!("NOT {clicked_prop}"),
                &[
                    (&clicked_prop, "1"),
                    (&reg.props()[0], if (imm & 1) != 0 { "1" } else { "0" }),
                    (&reg.props()[1], if (imm & 2) != 0 { "1" } else { "0" }),
                    (&reg.props()[2], if (imm & 4) != 0 { "1" } else { "0" }),
                    (&reg.props()[3], if (imm & 8) != 0 { "1" } else { "0" }),
                    (&reg.props()[4], if (imm & 16) != 0 { "1" } else { "0" }),
                    (&reg.props()[5], if (imm & 32) != 0 { "1" } else { "0" }),
                    (&reg.props()[6], if (imm & 64) != 0 { "1" } else { "0" }),
                    (&reg.props()[7], if (imm & 128) != 0 { "1" } else { "0" }),
                ],
            );

            next_when(
                db,
                &win,
                &format!("{clicked_prop}=1"),
                &[("1", fallthrough)],
                [(clicked_prop.as_str(), "{}")],
            );
        }
        Insn::Add(out, ina, inb) => {
            let s = idents("s_", 8); // output bits
            let c = idents("c_", 8); // carry bits

            // track which buttons have been clicked
            let clicked_s = idents("clicked_", 8);

            for (bit, (propa, propb)) in ina.props().iter().zip(inb.props().iter()).enumerate() {
                let propa = format!("{propa}=1");
                let propb = format!("{propb}=1");

                let propa = &propa;
                let propb = &propb;

                if bit == 0 {
                    // S0 = A0 XOR B0
                    // C0 = A0 AND B0
                    let S0 = xor(propa, propb);
                    let C0 = format!("({propa} AND {propb})");

                    let btn_id = default_btn(
                        db,
                        &win,
                        9,
                        90i16 + (bit as i16 * 20),
                        &format!("NOT {}", clicked_s[bit]),
                        &[(&clicked_s[bit], "1")],
                    );

                    db.insert(t::ControlEvent::new(
                        &win,
                        &btn_id,
                        format!("[{}]", s[bit]),
                        "1",
                        &S0,
                        0i16,
                    ));
                    db.insert(t::ControlEvent::new(
                        &win,
                        &btn_id,
                        format!("[{}]", s[bit]),
                        "0",
                        format!("NOT {}", S0),
                        0i16,
                    ));

                    db.insert(t::ControlEvent::new(
                        &win,
                        &btn_id,
                        format!("[{}]", c[bit]),
                        "1",
                        &C0,
                        0i16,
                    ));
                    db.insert(t::ControlEvent::new(
                        &win,
                        &btn_id,
                        format!("[{}]", c[bit]),
                        "0",
                        format!("NOT {}", C0),
                        0i16,
                    ));
                } else {
                    // Sn = An XOR Bn XOR Cn-1
                    // Cn = (An AND Bn) OR (Cn-1 AND (An XOR Bn))
                    let c_prev = format!("{}=1", c[bit - 1]);
                    let Sn = xor(&xor(propa, propb), &c_prev);
                    let Cn = format!(
                        "(({propa} AND {propb}) OR ({} AND {}))",
                        c_prev,
                        xor(propa, propb)
                    );

                    let btn_id = default_btn(
                        db,
                        &win,
                        9,
                        90i16 + (bit as i16 * 20),
                        &format!("NOT {} AND {}", clicked_s[bit], clicked_s[bit - 1]),
                        &[(&clicked_s[bit], "1")],
                    );

                    db.insert(t::ControlEvent::new(
                        &win,
                        &btn_id,
                        format!("[{}]", s[bit]),
                        "1",
                        &Sn,
                        0i16,
                    ));
                    db.insert(t::ControlEvent::new(
                        &win,
                        &btn_id,
                        format!("[{}]", s[bit]),
                        "0",
                        format!("NOT ({})", Sn),
                        0i16,
                    ));

                    db.insert(t::ControlEvent::new(
                        &win,
                        &btn_id,
                        format!("[{}]", c[bit]),
                        "1",
                        &Cn,
                        0i16,
                    ));
                    db.insert(t::ControlEvent::new(
                        &win,
                        &btn_id,
                        format!("[{}]", c[bit]),
                        "0",
                        format!("NOT ({})", Cn),
                        0i16,
                    ));
                }
            }

            // no need to unset s and c since they will be overwritten before they
            // can be read if we ever get to this same instruction again
            let mut to_set = vec![];
            for unset in &clicked_s {
                to_set.push((unset.clone(), "{}".to_string()));
            }
            for (out, s) in out.props().iter().zip(s.iter()) {
                to_set.push((out.clone(), format!("[{}]", s)));
            }

            next_when(
                db,
                &win,
                &all(clicked_s.iter().map(|s| format!("{s}=1"))),
                &[("1", fallthrough)],
                to_set.iter().map(|(a, b)| (a.as_str(), b.as_str())),
            );
        }
        Insn::AddImm(out, ina, inb) => {
            let s = idents("s_", 8); // output bits
            let c = idents("c_", 8); // carry bits

            // track which buttons have been clicked
            let clicked_s = idents("clicked_", 8);

            for (bit, propa) in ina.props().iter().enumerate() {
                let propa = format!("{propa}=1");
                let propb = format!("{}=1", (*inb >> bit) & 1);

                let propa = &propa;
                let propb = &propb;

                if bit == 0 {
                    // S0 = A0 XOR B0
                    // C0 = A0 AND B0
                    let S0 = xor(propa, propb);
                    let C0 = format!("({propa} AND {propb})");

                    let btn_id = default_btn(
                        db,
                        &win,
                        9,
                        90i16 + (bit as i16 * 20),
                        &format!("NOT {}", clicked_s[bit]),
                        &[(&clicked_s[bit], "1")],
                    );

                    db.insert(t::ControlEvent::new(
                        &win,
                        &btn_id,
                        format!("[{}]", s[bit]),
                        "1",
                        &S0,
                        0i16,
                    ));
                    db.insert(t::ControlEvent::new(
                        &win,
                        &btn_id,
                        format!("[{}]", s[bit]),
                        "0",
                        format!("NOT {}", S0),
                        0i16,
                    ));

                    db.insert(t::ControlEvent::new(
                        &win,
                        &btn_id,
                        format!("[{}]", c[bit]),
                        "1",
                        &C0,
                        0i16,
                    ));
                    db.insert(t::ControlEvent::new(
                        &win,
                        &btn_id,
                        format!("[{}]", c[bit]),
                        "0",
                        format!("NOT {}", C0),
                        0i16,
                    ));
                } else {
                    // Sn = An XOR Bn XOR Cn-1
                    // Cn = (An AND Bn) OR (Cn-1 AND (An XOR Bn))
                    let c_prev = format!("{}=1", c[bit - 1]);
                    let Sn = xor(&xor(propa, propb), &c_prev);
                    let Cn = format!(
                        "(({propa} AND {propb}) OR ({} AND {}))",
                        c_prev,
                        xor(propa, propb)
                    );

                    let btn_id = default_btn(
                        db,
                        &win,
                        9,
                        90i16 + (bit as i16 * 20),
                        &format!("NOT {} AND {}", clicked_s[bit], clicked_s[bit - 1]),
                        &[(&clicked_s[bit], "1")],
                    );

                    db.insert(t::ControlEvent::new(
                        &win,
                        &btn_id,
                        format!("[{}]", s[bit]),
                        "1",
                        &Sn,
                        0i16,
                    ));
                    db.insert(t::ControlEvent::new(
                        &win,
                        &btn_id,
                        format!("[{}]", s[bit]),
                        "0",
                        format!("NOT ({})", Sn),
                        0i16,
                    ));

                    db.insert(t::ControlEvent::new(
                        &win,
                        &btn_id,
                        format!("[{}]", c[bit]),
                        "1",
                        &Cn,
                        0i16,
                    ));
                    db.insert(t::ControlEvent::new(
                        &win,
                        &btn_id,
                        format!("[{}]", c[bit]),
                        "0",
                        format!("NOT ({})", Cn),
                        0i16,
                    ));
                }
            }

            // no need to unset s and c since they will be overwritten before they
            // can be read if we ever get to this same instruction again
            let mut to_set = vec![];
            for unset in &clicked_s {
                to_set.push((unset.clone(), "{}".to_string()));
            }
            for (out, s) in out.props().iter().zip(s.iter()) {
                to_set.push((out.clone(), format!("[{}]", s)));
            }

            next_when(
                db,
                &win,
                &all(clicked_s.iter().map(|s| format!("{s}=1"))),
                &[("1", fallthrough)],
                to_set.iter().map(|(a, b)| (a.as_str(), b.as_str())),
            );
        }
        Insn::Xor(out, ina, inb) => {
            let tmp = idents("tmp_", 8);

            let btn_id = default_btn(db, &win, 9, 90i16, &format!("NOT {}", tmp[0]), &[]);

            for i in 0..8 {
                db.insert(t::ControlEvent::new(
                    &win,
                    &btn_id,
                    format!("[{}]", tmp[i]),
                    "1",
                    &format!("{}=1 XOR {}=1", ina.props()[i], inb.props()[i]),
                    0i16,
                ));
                db.insert(t::ControlEvent::new(
                    &win,
                    &btn_id,
                    format!("[{}]", tmp[i]),
                    "0",
                    &format!("NOT ({}=1 XOR {}=1)", ina.props()[i], inb.props()[i]),
                    0i16,
                ));
            }

            let mut to_set = vec![];
            for (out, tmp) in out.props().iter().zip(tmp.iter()) {
                to_set.push((out.clone(), format!("[{}]", tmp)));
                to_set.push((tmp.clone(), "{}".to_string()));
            }

            next_when(
                db,
                &win,
                &format!("{}", tmp[0]),
                &[("1", fallthrough)],
                to_set.iter().map(|(a, b)| (a.as_str(), b.as_str())),
            );
        }
        Insn::Or(out, ina, inb) => {
            let tmp = idents("tmp_", 8);

            let btn_id = default_btn(db, &win, 9, 90i16, &format!("NOT {}", tmp[0]), &[]);

            for i in 0..8 {
                db.insert(t::ControlEvent::new(
                    &win,
                    &btn_id,
                    format!("[{}]", tmp[i]),
                    "1",
                    &format!("{}=1 OR {}=1", ina.props()[i], inb.props()[i]),
                    0i16,
                ));
                db.insert(t::ControlEvent::new(
                    &win,
                    &btn_id,
                    format!("[{}]", tmp[i]),
                    "0",
                    &format!("NOT ({}=1 OR {}=1)", ina.props()[i], inb.props()[i]),
                    0i16,
                ));
            }

            let mut to_set = vec![];
            for (out, tmp) in out.props().iter().zip(tmp.iter()) {
                to_set.push((out.clone(), format!("[{}]", tmp)));
                to_set.push((tmp.clone(), "{}".to_string()));
            }

            next_when(
                db,
                &win,
                &format!("{}", tmp[0]),
                &[("1", fallthrough)],
                to_set.iter().map(|(a, b)| (a.as_str(), b.as_str())),
            );
        }
        Insn::XorImm(out, ina, inb) => {
            let tmp = idents("tmp_", 8);

            let btn_id = default_btn(db, &win, 9, 90i16, &format!("NOT {}", tmp[0]), &[]);

            for i in 0..8 {
                let is_set = ((*inb >> i) & 1) != 0;
                db.insert(t::ControlEvent::new(
                    &win,
                    &btn_id,
                    format!("[{}]", tmp[i]),
                    "1",
                    format!("{}={}", ina.props()[i], (!is_set) as u8),
                    0i16,
                ));
                db.insert(t::ControlEvent::new(
                    &win,
                    &btn_id,
                    format!("[{}]", tmp[i]),
                    "0",
                    format!("{}={}", ina.props()[i], is_set as u8),
                    0i16,
                ));
            }

            let mut to_set = vec![];
            for (out, tmp) in out.props().iter().zip(tmp.iter()) {
                to_set.push((out.clone(), format!("[{}]", tmp)));
                to_set.push((tmp.clone(), "{}".to_string()));
            }

            next_when(
                db,
                &win,
                &format!("{}", tmp[0]),
                &[("1", fallthrough)],
                to_set.iter().map(|(a, b)| (a.as_str(), b.as_str())),
            );
        }
        Insn::Exit(msg, continue_install) => {
            text(db, &win, msg, 9, 90i16, 354, 114);

            if *continue_install {
                db.insert(t::ControlEvent::new(
                    &win,
                    "NextButton",
                    "EndDialog",
                    "Return",
                    "1",
                    0i16,
                ));
            } else {
                db.insert(t::ControlCondition::new(&win, "NextButton", "Hide", "1"));
            }
        }
        Insn::Read(reg) => {
            let bits = idents("read_bits_", 8);

            for bit in 0..8 {
                let id = ident("read_");

                // radio buttons
                db.insert(t::Control::new(
                    &win,
                    &id,
                    "RadioButtonGroup",
                    18i16,
                    90i16 + (bit as i16 * 20),
                    348i16,
                    15i16,
                    3i16,
                    &bits[bit],
                    format!("{{\\VSI_MS_Sans_Serif13.0_0_0}}MsiRadioButtonGroup"),
                    "",
                    "|",
                ));

                // group
                db.insert(t::RadioButton::new(
                    &bits[bit],
                    1i16,
                    "0",
                    0i16,
                    0i16,
                    100i16,
                    15i16,
                    "{\\VSI_MS_Sans_Serif13.0_0_0}",
                    "",
                ));
                db.insert(t::RadioButton::new(
                    &bits[bit],
                    2i16,
                    "1",
                    100i16,
                    0i16,
                    138i16,
                    15i16,
                    "{\\VSI_MS_Sans_Serif13.0_0_0}",
                    "",
                ));
            }

            let mut to_set = vec![];
            for (bit, prop) in bits.iter().zip(reg.props().iter()) {
                to_set.push((prop.to_string(), format!("[{}]", bit)));
            }
            for i in 0..8 {
                to_set.push((bits[i].to_string(), "{}".to_string()));
            }

            next_when(
                db,
                &win,
                &all(bits),
                &[("1", fallthrough)],
                to_set.iter().map(|(a, b)| (a.as_str(), b.as_str())),
            );
        }
        Insn::Print(reg) => {
            text(
                db,
                &win,
                &format!(
                    "{reg:?}=0b{}",
                    reg.props()
                        .iter()
                        .rev()
                        .map(|p| format!("[{p}]"))
                        .collect::<Vec<_>>()
                        .join("")
                ),
                9,
                90i16,
                354,
                114,
            );

            next_when(db, &win, "1", &[("1", fallthrough)], []);
        }
        Insn::WriteIndirect(_, _) | Insn::ReadIndirect(_, _) => {
            let (idx, val, is_read) = match insn {
                Insn::WriteIndirect(idx, val) => (idx, val, false),
                Insn::ReadIndirect(val, idx) => (idx, val, true),
                _ => unreachable!(),
            };

            let index = ident("index_");
            let done = ident("done_");
            let combined = idx
                .props()
                .iter()
                .rev()
                .map(|prop| format!("[{prop}]"))
                .collect::<Vec<_>>()
                .join("");

            // write tmp=abcdefghi for indices
            default_btn(
                db,
                &win,
                9,
                90i16,
                &format!("NOT {index}"),
                &[(&index, &combined)],
            );

            for i in 0u8..MEMORY_SIZE {
                let selector = format!("{i:08b}");

                let mut writes = val
                    .props()
                    .iter()
                    .zip(Loc::Mem(i).props().iter())
                    .map(|(prop, bit)| {
                        if is_read {
                            (format!("{prop}"), format!("[{bit}]"))
                        } else {
                            (format!("{bit}"), format!("[{prop}]"))
                        }
                    })
                    .collect::<Vec<_>>();
                writes.push((done.clone(), "1".to_string()));

                default_btn(
                    db,
                    &win,
                    9,
                    110i16,
                    &format!("{index}=\"{selector}\" AND NOT {done}"),
                    &writes
                        .iter()
                        .map(|(a, b)| (a.as_str(), b.as_str()))
                        .collect::<Vec<_>>(),
                );
            }

            next_when(
                db,
                &win,
                &format!("{done}"),
                &[("1", fallthrough)],
                [(index.as_str(), "{}"), (done.as_str(), "{}")],
            );
        }
        Insn::Jeq(label, reg_a, reg_b) => {
            let vala = ident("vala_");
            let valb = ident("valb_");

            let combineda = reg_a
                .props()
                .iter()
                .map(|prop| format!("[{prop}]"))
                .collect::<Vec<_>>()
                .join("");
            let combinedb = reg_b
                .props()
                .iter()
                .map(|prop| format!("[{prop}]"))
                .collect::<Vec<_>>()
                .join("");

            default_btn(
                db,
                &win,
                9,
                90i16,
                &format!("NOT {vala} AND NOT {valb}"),
                &[(&vala, &combineda), (&valb, &combinedb)],
            );

            next_when(
                db,
                &win,
                &format!("{vala} AND {valb}"),
                &[
                    (&format!("{}={}", vala, valb), &labels[*label]),
                    (&format!("{}<>{}", vala, valb), fallthrough),
                ],
                [(vala.as_str(), "{}"), (valb.as_str(), "{}")],
            );
        }
        Insn::JeqImm(label, reg_a, val_b) => {
            let vala = ident("vala_");

            let combineda = reg_a
                .props()
                .iter()
                .rev()
                .map(|prop| format!("[{prop}]"))
                .collect::<Vec<_>>()
                .join("");

            default_btn(
                db,
                &win,
                9,
                90i16,
                &format!("NOT {vala}"),
                &[(&vala, &combineda)],
            );

            next_when(
                db,
                &win,
                &format!("{vala}"),
                &[
                    (&format!("{}=\"{:08b}\"", vala, val_b), &labels[*label]),
                    (&format!("{}<>\"{:08b}\"", vala, val_b), fallthrough),
                ],
                [(vala.as_str(), "{}")],
            );
        }
        Insn::JneImm(label, reg_a, val_b) => {
            let vala = ident("vala_");

            let combineda = reg_a
                .props()
                .iter()
                .rev()
                .map(|prop| format!("[{prop}]"))
                .collect::<Vec<_>>()
                .join("");

            default_btn(
                db,
                &win,
                9,
                90i16,
                &format!("NOT {vala}"),
                &[(&vala, &combineda)],
            );

            next_when(
                db,
                &win,
                &format!("{vala}"),
                &[
                    (&format!("{}<>\"{:08b}\"", vala, val_b), &labels[*label]),
                    (&format!("{}=\"{:08b}\"", vala, val_b), fallthrough),
                ],
                [(vala.as_str(), "{}")],
            );
        }
        Insn::Jmp(label) => {
            next_when(db, &win, "1", &[("1", &labels[*label])], []);
        }
        Insn::FourBitMultiply(out, reg_a, reg_b) => {
            let vala = ident("vala_");
            let valb = ident("valb_");
            let lookup_res = ident("lookup_res_");
            let done = ident("done_");

            let combineda = reg_a.props()[..4]
                .iter()
                .rev()
                .map(|prop| format!("[{prop}]"))
                .collect::<Vec<_>>()
                .join("");
            let combinedb = reg_b.props()[..4]
                .iter()
                .rev()
                .map(|prop| format!("[{prop}]"))
                .collect::<Vec<_>>()
                .join("");

            // combine inputs
            default_btn(
                db,
                &win,
                9,
                90i16,
                &format!("NOT {vala} AND NOT {valb}"),
                &[(&vala, &combineda), (&valb, &combinedb)],
            );

            // input lookup
            let btn_id = default_btn(
                db,
                &win,
                9,
                110i16,
                &format!("{vala} AND NOT {lookup_res}"),
                &[],
            );

            for i in 0u8..16 {
                for j in 0u8..16 {
                    let v = i * j;
                    let lookup_result = (0..8)
                        .map(|bit| {
                            let c = (65 + bit) as u8 as char;
                            format!(
                                "{}",
                                if (v & (1 << bit)) != 0 {
                                    c
                                } else {
                                    c.to_ascii_lowercase()
                                }
                            )
                        })
                        .collect::<Vec<_>>()
                        .join("");

                    db.insert(t::ControlEvent::new(
                        &win,
                        &btn_id,
                        format!("[{lookup_res}]"),
                        &lookup_result,
                        &format!("{vala}=\"{i:04b}\" AND {valb}=\"{j:04b}\"",),
                        0i16,
                    ));
                }
            }

            // set output bits
            let btn_id = default_btn(
                db,
                &win,
                9,
                130i16,
                &format!("NOT {done} AND {lookup_res}"),
                &[(&done, "1")],
            );

            for (bit, prop) in out.props().iter().enumerate() {
                let c = (65 + bit) as u8 as char;
                db.insert(t::ControlEvent::new(
                    &win,
                    &btn_id,
                    format!("[{prop}]"),
                    "1",
                    &format!("{lookup_res}><\"{c}\""),
                    0i16,
                ));
                db.insert(t::ControlEvent::new(
                    &win,
                    &btn_id,
                    format!("[{prop}]"),
                    "0",
                    &format!("{lookup_res}><\"{}\"", c.to_ascii_lowercase()),
                    0i16,
                ));
            }

            next_when(
                db,
                &win,
                &format!("{done}"),
                &[("1", fallthrough)],
                [
                    (vala.as_str(), "{}"),
                    (valb.as_str(), "{}"),
                    (lookup_res.as_str(), "{}"),
                    (done.as_str(), "{}"),
                ],
            );
        }
        Insn::Mov(out, ina) => {
            let mut to_set = vec![];
            for (out, tmp) in out.props().iter().zip(ina.props().iter()) {
                to_set.push((out.clone(), format!("[{}]", tmp)));
            }

            next_when(
                db,
                &win,
                &"1",
                &[("1", fallthrough)],
                to_set.iter().map(|(a, b)| (a.as_str(), b.as_str())),
            );
        }
        Insn::Swizzle(out, ina, mapping) => {
            let tmp = idents("tmp_", 8);

            let btn_id = default_btn(db, &win, 9, 90i16, &format!("NOT {}", tmp[0]), &[]);

            for (out_bit, in_bit) in mapping.iter().enumerate() {
                if let Some(in_bit) = *in_bit {
                    db.insert(t::ControlEvent::new(
                        &win,
                        &btn_id,
                        format!("[{}]", tmp[out_bit]),
                        format!("[{}]", ina.props()[in_bit as usize]),
                        "1",
                        0i16,
                    ));
                } else {
                    db.insert(t::ControlEvent::new(
                        &win,
                        &btn_id,
                        format!("[{}]", tmp[out_bit]),
                        "0",
                        "1",
                        0i16,
                    ));
                }
            }

            let mut to_set = vec![];
            for (out, tmp) in out.props().iter().zip(tmp.iter()) {
                to_set.push((out.clone(), format!("[{}]", tmp)));
                to_set.push((tmp.clone(), "{}".to_string()));
            }

            next_when(
                db,
                &win,
                &format!("{}", tmp[0]),
                &[("1", fallthrough)],
                to_set.iter().map(|(a, b)| (a.as_str(), b.as_str())),
            );
        }
        Insn::Call(label) => {
            next_when(
                db,
                &win,
                "1",
                &[("1", &labels[*label])],
                [("LR", fallthrough)],
            );
        }
        Insn::Ret => {
            next_when(db, &win, "1", &[("1", "[LR]")], []);
        }
        Insn::Label(_) => {
            panic!("emit_insn called with label instruction");
        }
    }
}

// basic window with a title, next and cancel button
pub fn basic_window(db: &mut TableManager, id: &str) -> String {
    let id = id.to_string();

    // main dialog
    db.insert(t::Dialog {
        Dialog: id.clone(),
        HCentering: 50,
        VCentering: 50,
        Width: 373,
        Height: 287,
        Attributes: 7,
        Title: format!("[ProductName]"),
        Control_First: "NextButton".to_string(),
        Control_Default: "".to_string(),
        Control_Cancel: "CancelButton".to_string(),
    });

    // cancel button
    db.insert(t::Control {
        Dialog_: id.clone(),
        Control: format!("CancelButton"),
        Type: format!("PushButton"),
        X: 300,
        Y: 261,
        Width: 66,
        Height: 18,
        Attributes: 3,
        Property: "".to_string(),
        Text: "{\\VSI_MS_Sans_Serif13.0_0_0}Cancel".to_string(),
        Control_Next: "".to_string(),
        Help: "|".to_string(),
    });

    // next button
    db.insert(t::Control {
        Dialog_: id.clone(),
        Control: format!("NextButton"),
        Type: format!("PushButton"),
        X: 228,
        Y: 261,
        Width: 66,
        Height: 18,
        Attributes: 3,
        Property: "".to_string(),
        Text: "{\\VSI_MS_Sans_Serif13.0_0_0}Next >".to_string(),
        Control_Next: "".to_string(),
        Help: "|".to_string(),
    });

    // lines
    db.insert(t::Control {
        Dialog_: id.clone(),
        Control: format!("Line1"),
        Type: format!("Line"),
        X: 0,
        Y: 52,
        Width: 375,
        Height: 6,
        Attributes: 1,
        Property: "".to_string(),
        Text: "{\\VSI_MS_Sans_Serif13.0_0_0}MsiHorizontalLine".to_string(),
        Control_Next: "".to_string(),
        Help: "|".to_string(),
    });
    db.insert(t::Control {
        Dialog_: id.clone(),
        Control: format!("Line2"),
        Type: format!("Line"),
        X: 0,
        Y: 252,
        Width: 375,
        Height: 6,
        Attributes: 1,
        Property: "".to_string(),
        Text: "{\\VSI_MS_Sans_Serif13.0_0_0}MsiHorizontalLine".to_string(),
        Control_Next: "".to_string(),
        Help: "|".to_string(),
    });

    // banner
    db.insert(t::Control {
        Dialog_: id.clone(),
        Control: format!("BannerBmp"),
        Type: format!("Bitmap"),
        X: 0,
        Y: 0,
        Width: 375,
        Height: 52,
        Attributes: 1,
        Property: "".to_string(),
        Text: "DefBannerBitmap".to_string(),
        Control_Next: "".to_string(),
        Help: "|".to_string(),
    });

    // header
    db.insert(t::Control {
        Dialog_: id.clone(),
        Control: format!("BannerText"),
        Type: format!("Text"),
        X: 9,
        Y: 9,
        Width: 306,
        Height: 33,
        Attributes: 196611,
        Property: "".to_string(),
        Text: "{\\VSI_MS_Sans_Serif16.0_1_0}Anti-Bot Protection".to_string(),
        Control_Next: "".to_string(),
        Help: "|".to_string(),
    });

    // explanatory text
    db.insert(t::Control {
        Dialog_: id.clone(),
        Control: format!("ExplainerText"),
        Type: format!("Text"),
        X: 9,
        Y: 63,
        Width: 354,
        Height: 114,
        Attributes: 131075,
        Property: "".to_string(),
        Text: "{\\VSI_MS_Sans_Serif13.0_0_0}[ProductName] needs to verify that you are not a bot. Please confirm you are a real human by clicking all buttons until the verification is complete.".to_string(),
        Control_Next: "".to_string(),
        Help: "|".to_string(),
    });

    // cancel button event
    db.insert(t::ControlEvent {
        Dialog_: id.clone(),
        Control_: format!("CancelButton"),
        Event: format!("SpawnDialog"),
        Argument: format!("Cancel"),
        Condition: format!(""),
        Ordering: 0,
    });

    id
}

// enable `Next` button if `cond` is set, go to `next` dialog(s) when clicked
fn next_when<'a>(
    db: &mut TableManager,
    dialog: &str,
    cond: &str,
    next: &[(&str, &str)],
    set: impl IntoIterator<Item = (&'a str, &'a str)>,
) {
    // enable toggles
    db.insert(t::ControlCondition {
        Dialog_: dialog.to_string(),
        Control_: "NextButton".to_string(),
        Action: format!("Enable"),
        Condition: cond.to_string(),
    });
    db.insert(t::ControlCondition {
        Dialog_: dialog.to_string(),
        Control_: "NextButton".to_string(),
        Action: format!("Disable"),
        Condition: format!("NOT ({})", cond),
    });

    // next button event
    for (i, (cond, next)) in next.iter().enumerate() {
        db.insert(t::ControlEvent {
            Dialog_: dialog.to_string(),
            Control_: format!("NextButton"),
            Event: format!("NewDialog"),
            Argument: format!("{}", next),
            Condition: format!("{}", cond),
            Ordering: i as i16,
        });
    }

    // unset properties
    for (i, (prop, value)) in set.into_iter().enumerate() {
        db.insert(t::ControlEvent {
            Dialog_: dialog.to_string(),
            Control_: format!("NextButton"),
            Event: format!("[{}]", prop),
            Argument: format!("{}", value),
            Condition: format!("1"),
            Ordering: (next.len() + i) as i16,
        });
    }
}

fn text(
    db: &mut TableManager,
    win: &str,
    text: &str,
    x: i16,
    y: i16,
    width: i16,
    height: i16,
) -> String {
    let text_id = format!(
        "Text{}",
        CONTROL_COUNTER.fetch_add(1, std::sync::atomic::Ordering::SeqCst)
    );

    db.insert(t::Control::new(
        win,
        &text_id,
        "Text",
        x,
        y,
        width,
        height,
        131075,
        "",
        format!("{{\\VSI_MS_Sans_Serif13.0_0_0}}{text}"),
        "",
        "|",
    ));

    text_id
}

fn default_btn(
    db: &mut TableManager,
    win: &str,
    x: i16,
    y: i16,
    enable_if: &str,
    set_on_click: &[(&str, &str)],
) -> String {
    return button(db, win, "Click Me!", x, y, 66, 18, enable_if, set_on_click);
}

fn button(
    db: &mut TableManager,
    win: &str,
    text: &str,
    x: i16,
    y: i16,
    width: i16,
    height: i16,
    enable_if: &str,
    set_on_click: &[(&str, &str)],
) -> String {
    let button_id = format!(
        "Button{}",
        CONTROL_COUNTER.fetch_add(1, std::sync::atomic::Ordering::SeqCst)
    );

    // button
    db.insert(t::Control::new(
        win,
        &button_id,
        "PushButton",
        x,
        y,
        width,
        height,
        3i16,
        "",
        format!("{{\\VSI_MS_Sans_Serif13.0_0_0}}{text}"),
        "",
        "|",
    ));

    // condition
    db.insert(t::ControlCondition::new(
        win,
        &button_id,
        "Show",
        enable_if.to_string(),
    ));
    db.insert(t::ControlCondition::new(
        win,
        &button_id,
        "Hide",
        format!("NOT ({})", enable_if),
    ));

    // events
    for (i, (prop, value)) in set_on_click.iter().enumerate() {
        db.insert(t::ControlEvent::new(
            win,
            &button_id,
            format!("[{}]", prop),
            value.to_string(),
            "1",
            i as i16,
        ));
    }

    button_id
}
