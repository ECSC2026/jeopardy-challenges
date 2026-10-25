# /// script
# dependencies = [
#   "z3-solver"
# ]
# ///
import os, tempfile, subprocess, re, z3

# Michaelsoft Office 2006 is an MSI reversing challenge. It turns out that dialogs, controls,
# events and conditions within the MSI database are powerful enough to implement a simple
# CPU architecture, where button clicks perform calculations and variables are used to store
# internal state and emulate memory.
#
# The CPU implemented is an 8-bit architecture where every value is stored as 8 separate
# variables, each representing a single bit of the 8-bit value. For the values that carry
# over between dialogs (registers, memory), their names are suffixed with _0-_7 to make it
# easier to identify which bit each variable represents.
#
# The program is roughly the following:
# while (c := getchar()) != 0:
#  mem[i++] = c
# <a bunch of computations with add, xor, bit permutations, and 4x4 multiplication>
# if mem[0] != x: fail()
# if mem[1] != y: fail()
# ...
# success()
#
# The easiest way to approach this challenge is to print each event and condition for each
# dialog, and to look for patterns. There's only a couple of unique instructions and their
# encodings are consistent (except for the fact that MSI tables are sorted) so it becomes
# somewhat easy to pattern match on what a dialog does based on what it looks like. That
# lets us extract the computations that modify the input, and the final check against
# expected values.
#
# After we extract the computations, we only need to map the underlying register/memory
# variable names back to the input. We can do this by looking at the giant list of buttons
# in Step5, responsible for the indirect `mem[i++]` write. The first char is stored in the
# register for `idx=00000000`, the second char in `idx=00000001`, and so on.

msidb = "C:\\Program Files (x86)\\Windows Kits\\10\\bin\\10.0.19041.0\\x86\\MsiDb.exe"
input = "./out2.msi"

# extract to temp dir
tempdir = tempfile.mkdtemp()
subprocess.run([msidb, "-s", "-d", input, "-f", tempdir, "-e", "*"])

def parse_table(path):
    with open(path, "r", encoding="utf-8") as f:
        lines = f.readlines()
    headers = lines[0].strip().split("\t")
    rows = [line.strip().split("\t") for line in lines[1:]]
    rows = [dict(zip(headers, row)) for row in rows]
    return rows

control = parse_table(os.path.join(tempdir, "Control.idt"))
control_condition = parse_table(os.path.join(tempdir, "ControlC.idt"))
control_event = parse_table(os.path.join(tempdir, "ControlE.idt"))

all_controls = {}
for c in control:
    key = (c["Dialog_"], c["Control"])
    all_controls[key] = {
        "control": c,
        "conditions": [],
        "events": [],
    }

for cc in control_condition:
    key = (cc["Dialog_"], cc["Control_"])
    if key in all_controls:
        all_controls[key]["conditions"].append(cc)

for ce in control_event:
    key = (ce["Dialog_"], ce["Control_"])
    if key in all_controls:
        all_controls[key]["events"].append(ce)

def extract_vars(msg):
    return [(m[0], int(m[1])) for m in re.findall(r"([A-Za-z]+)_([0-9])", msg)]

# extract slot mapping mapping character index to the variable that contains it.
slot_mapping = {}
for (dialog, name), btn in all_controls.items():
    if dialog != "Step5" or not name.startswith("Button") or name == "Button0":
        continue
    show = next(c["Condition"] for c in btn["conditions"] if c["Action"] == "Show")
    idx = int(re.search(r'="([01]{8})"', show)[1], 2)
    slot_mapping[idx] = next(v for v, _ in extract_vars(" ".join(e["Event"] for e in btn["events"])))
slot_mapping = dict(sorted(slot_mapping.items()))

# now let's build a z3 model
s = z3.Solver()
z3_vars = {v: z3.BitVec(v, 8) for v in slot_mapping.values()}
flag_vars = {k: z3_vars[v] for k, v in slot_mapping.items()}

# limited only to the relevant steps
for step in range(9, 484):
    buttons = {k: v for k, v in all_controls.items() if k[0] == f"Step{step}" and (k[1].startswith("Button") or k[1] == "NextButton")}
    buttonl = list(buttons.values())

    all_reads = set()
    all_writes = set()
    for btn in buttonl:
        for event in btn["events"]:
            all_reads.update([v[0] for v in extract_vars(event["Argument"])])
            all_reads.update([v[0] for v in extract_vars(event["Condition"])])
            all_writes.update([v[0] for v in extract_vars(event["Event"])])
    all_reads = list(all_reads)
    all_writes = list(all_writes)

    # debugging
    if step in []:
        print(f"Step{step}:")
        for button in buttons:
            print(f"  {button[1]}:")
            for condition in buttons[button]["conditions"]:
                print(f"    {condition['Action']}: {condition['Condition']}")
            for event in buttons[button]["events"]:
                print(f"    {event['Event']} <- {event['Argument']} when {event['Condition']}")

    # x = immediate
    is_set = len(buttonl) == 2 and all([x["Argument"] in ("0", "1") and x["Condition"] == "1" for x in buttonl[0]["events"]])
    if is_set:
        x = all_writes[0]
        val = "".join(x["Argument"] for x in buttonl[0]["events"] if "_" in x["Event"])
        val = int(val[::-1], 2)
        print(f"  {step}: {x} = {val}")
        z3_vars[x] = z3.BitVecVal(val, 8)
        continue

    # x = a xor b
    is_xor = len(buttonl) == 2 and all([x["Argument"] in ("0", "1") and "XOR" in x["Condition"] for x in buttonl[0]["events"]])
    if is_xor:
        a, b = all_reads
        x = all_writes[0]
        print(f"  {step}: {x} = {a} XOR {b}")
        z3_vars[x] = z3_vars[a] ^ z3_vars[b]
        continue

    # x = permute(a, ...)
    is_permute = len(buttonl) == 2 and all(["[" in x["Argument"] and "[" in x["Event"] and "][" not in x["Argument"] and x["Condition"] == "1" for x in buttonl[0]["events"]])
    if is_permute:
        # alphabetically sorted so this is a bit of a mess
        intermediates = {}
        for ev in buttonl[0]["events"]:
            intermediates[ev["Event"]] = ev["Argument"]

        # order is lsb to msb
        order = []
        for i in range(8):
            write_entry = next((k for k in buttonl[1]["events"] if f"_{i}" in k["Event"]))
            order.append(int(intermediates[write_entry["Argument"]][-2:-1]))
        
        a = all_reads[0]
        x = all_writes[0]
        print(f"  {step}: {x} = permute({a}, {order})")
        z3_vars[x] = z3.Concat(*[z3.Extract(i, i, z3_vars[a]) for i in order[::-1]])
        continue

    # x = a + b
    is_add = len(buttonl) == 9
    if is_add:
        a, b = all_reads
        x = all_writes[0]
        print(f"  {step}: {x} = {a} + {b}")
        z3_vars[x] = z3_vars[a] + z3_vars[b]
        continue

    # multiply
    is_multiply = len(buttonl) == 4
    if is_multiply:
        a, b = all_reads
        x = all_writes[0]
        print(f"  {step}: {x} = {a} * {b}")
        z3_vars[x] = (z3_vars[a] & 0b0000_1111) * (z3_vars[b] & 0b0000_1111)
        continue

    # value check, a == cond
    is_check = len(buttonl) == 2 and any(["<>" in x["Condition"] for x in buttonl[1]["events"]])
    if is_check:
        a = all_reads[0]
        val = next((v["Condition"] for v in buttonl[1]["events"] if "<>" in v["Condition"]), None)
        val = int(val.split('"')[1], 2)
        print(f"  {step}: {a} == {val}")
        s.add(z3_vars[a] == val)
        continue

    # wtf
    raise Exception(f"weird form at step {step}")

print(s.check())
model = s.model()
flag = ""
for k, v in flag_vars.items():
    if model[v] is None:
        break
    flag += chr(model[v].as_long())
print(flag)