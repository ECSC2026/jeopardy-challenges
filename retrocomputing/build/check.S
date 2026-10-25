/*
 * The main part of the flag checker is 26-bit ARM code.
 * This is mapped in the low 64 MB (26-bit space), and has to stay there.
 *
 * Two passes over the state, one word per round:
 *
 *     w[i] := w[i] + F(w[i+1], nzcv) + C,   i = round & 15
 *
 * Nothing in F depends on w[i], so the round is a translation and therefore a
 * permutation, and inverting it is a subtraction rather than a search.
 *
 * Two passes rather than one, deliberately.  With a single pass every word the
 * inverse recovers is immediately plaintext, so a wrong model announces itself
 * one round at a time.  With two, the first sixteen backward rounds recover
 * intermediate values that cannot be checked against anything -- you have to
 * have the whole machine right before the first legible byte appears.
 *
 * Register assignment:
 *  r0        a = w[i+1], which this round does not write
 *  r2        t = S-box word
 *  r3        i = round & 15, kept from the load to the store
 *  r4-r7     Scratch registers
 *  r8        State pointer
 *  r9        S-box
 *  r10       Round counter, running from -ROUNDS up to zero
 *  r11       a & 0xff: the S-box offset, reused as the shift amount
 *  r12       Loop displacement
 *
 * For instructions, use .inst instead of .word to avoid the $d symbols.
 */

#define ROUNDS 32

        .text
        .p2align    2
        .global     transform
        .type       transform, %function
transform:
        stmfd       sp!, {r4-r11, lr}
        @ Register setup
        mov         r8, r0
        ldr         r9, =sbox
        ldr         r12, .Ldisp                         @ With 26-bit wrapping, goes back to .Lround
        mvn         r10, #(ROUNDS - 1)                  @ = -ROUNDS: counts up to zero, low nibble counts 0..15
        swi         #0x9f0003                           @ __ARM_NR_usr26

        adds        r4, r10, #0                         @ NZCV = 1000

.Lround:
        @ Transform loop

        @ 1. Round state
        @    r10 runs from -ROUNDS up to zero, so the low nibble counts 0..15 twice
        @    and the loop below can test for the end with the same instruction that
        @    steps the counter.
        and         r3, r10, #15
        ldr         r7, [r8, r3, lsl #2]                @ u := r7 = w[round & 15]
        add         r11, r3, #1
        and         r11, r11, #15
        ldr         r0, [r8, r11, lsl #2]               @ a := r0 = w[(round + 1) & 15]

        @ 2. S-box lookup
        @    We address the table by the whole low byte of a, so the word we land on is
        @    S[(a & 0xff) >> 2] and we are off by (a & 3) bytes.
        @    This is allowed on Linux on the SA-110.
        and         r11, r0, #0xff
        ldr         r2, [r9, r11]                       @ t := r2 = ror(S[(a & 0xff) >> 2], a & 3)

        @ 3. NZCV flag folding
        @    In `mov r4, pc`, r15/pc is named in Rm, so we read the entire thing including the fused flags.
        @    That means r4 contains exactly the NZCV flag state in its top nibble. We do not have to mask
        @    it off, because everything else in r15 lives below bit 28.
        @    The TEQP instruction is a 26-bit exclusive (and objdump will silently drop the p suffix).
        @
        @    ARM Architecture Reference Manual (ARM DDI 0100B), §5.3:
        @     > On 26-bit architectures, the MSR and MRS instructions are not supported. Instead,
        @     > variants of the CMP, CMN, TST and TEQ instructions are used to write just the PSR part
        @     > of register 15. These variants are called CMPP, CMNP, TSTP and TEQP, and are
        @     > distinguished by having instruction bits[15:12] equal to 0b1111 (these bits are usually
        @     > set to zero for these instructions).
        @     > These instructions write their ALU result directly to the PSR part of register 15 (only N,
        @     > Z, C and V are affected in User mode).
        @
        @    In other words, this sets nzcv ^= t[31:28].
        @
        .inst       0xe1a0400f                          @ mov  r4, pc
        .inst       0xe134f002                          @ teqp r4, r2

        @ 4. PC writes
        @    On the other hand, if you write to pc without the S bit, only the PC part is kept
        @    (cf. ARM §5.2.2). We do the S-bit version below, and the non-S-bit version here.
        @    t << 26 is all zeroes below bit 26, so it adds nothing to pc[25:2] and we land on pc + 8,
        @    stepping over the next instruction. What it does add is junk in the top six bits.
        @    Emulating this as 32-bit will crash here.
        @    (or, as the LLM so nicely put it, "jump to hyperspace").
        .inst       0xe08ffd02                          @ add pc, pc, r2, lsl #26
        nop                                             @ Skipped

        @ 5. Compute something from the flags
        @    This is the dependency on the flag nibble we got from above.
        eor         r5, r0, r2                          @ F := r5 = r0 (a) ^ r2 (t)
        eorlt       r5, r5, r2                          @ LT [N != V]     r5 = r5 ^ r2
        subhi       r5, r5, r4, lsr #28                 @ HI [C and !Z]   r5 = r5 - nzcv

        @ 6. Register shift
        @    This is mostly just confusing.
        @    The ordinary reading of an LSL operand would be to shift by (imm % width).
        @    That is what AArch64 and x86 do, but 32-bit ARM has always done something else
        @    (ARM DDI 0100B, Addressing Mode 1 (Rm, LSL Rs)).
        @    The shift amount is Rs[7:0], so the result is zero if the amount is larger than the
        @    register width.
        eor         r5, r5, r2, lsl r11
        @    Similarly, ASR #32 is encoded by <shift_imm> = 0, which people will treat as "no shift".
        @    This is wrong, as is a naive ">> 32" (ASR #32 will broadcast the register's sign bit, i.e. this
        @    really is an ASR #31 with different flag behavior.
        .inst       0xe0255042                          @ eor r5, r5, r2, asr #32

        @ 7. Accumulate
        @    Nothing since the teqp has had the S bit set, so C is still the one it wrote.
        @    The movs{ne,eq} below pick whether it is reset to zero or not (their r4 result
        @    is discarded).
        tst         r0, #0x40                           @ Z = not (a & 0x40)
        .inst       0x13b04104                          @ movsne r4, #4, ROR #2  =>  if !Z then C = 0 end (and N = Z = 0)
        .inst       0x03b04001                          @ movseq r4, #1          =>  if  Z then C = C end (and N = Z = 0)
        adc         r6, r7, r5                          @ r6 = r7 (u) + r5 (F) + C

        @ 8. Write back
        str         r6, [r8, r3, lsl #2]                @ w[round & 15] = r6

        @ 9. Loop
        @    This is another "do maths with pc" trick. But this time, we have the S bit set.
        @    This means we write the PC from result[25:2], and the flags from result[31:28].
        @    This is both a jump _and_ a communication channel to the next round.
        and         r11, r6, #0xf0000000                @ Take only the flags
        adds        r10, r10, #1                        @ Step the round counter
        orrne       r11, r11, r12                       @ Update r11 (new pc) only if we have another round to go
.Lend:
        .inst       0xe09ff00b                          @ adds pc, pc, r11
                                                        @ Jumps to .Lround if we or'd into r11 (i.e., we have more rounds
                                                        @ to go), to nowhere if we don't have 26-bit wrapping, or falls
                                                        @ through to the next instruction otherwise.
        nop                                             @ Skipped

        @ 10. Cleanup
        @     Back out to 32-bit mode, the same way we came in.
        swi         #0x9f0004                           @ __ARM_NR_usr32
        ldmfd       sp!, {r4-r11, pc}

.Ldisp:
        .word       (.Lround - .Lend - 8) & 0x03fffffc
        .ltorg
        .size       transform, .-transform

        .section    .rodata
        .p2align    2
        .type       sbox, %object
sbox:
        .word       0x9614881d, 0x0294610f, 0xcf1ab333, 0x88e19501
        .word       0x14f0b706, 0x4c6b1b58, 0xdac9e49d, 0x1080b480
        .word       0x25164d5e, 0x76745c2b, 0x4fc83b7a, 0xb5c4ba4e
        .word       0xe4861666, 0x35794bcb, 0x565d2880, 0xbc3d3572
        .word       0xb3136f53, 0xb3ce7d25, 0xc6e99cb4, 0xf4d06ca3
        .word       0xbbb7f855, 0x21d3aefa, 0x3c27db7c, 0x7d493b8b
        .word       0xc56fa606, 0x4c2b01b4, 0x2804840e, 0xcad501a8
        .word       0x8005e5df, 0x75bb0891, 0xdd4e190e, 0x60355134
        .word       0x832ef727, 0x429900c4, 0x73a43bfa, 0xd0711b46
        .word       0x0edf74fa, 0xd2920dff, 0x9474a22c, 0x053e6683
        .word       0xacd79285, 0x39e2ca98, 0x4819869e, 0xe884e89c
        .word       0x96ffb245, 0xdb3435e6, 0x6c464789, 0x961d03d4
        .word       0x5cfbaabe, 0x943b1c0a, 0x2919dcef, 0x62aa804d
        .word       0xe30eff9c, 0x0ee152df, 0x5d2ebcb9, 0x13ecd101
        .word       0x208ba9e4, 0x325fd9ed, 0x87e6b466, 0x5c8f4886
        .word       0xb2353699, 0xa2839839, 0xaea670f2, 0x8a2e6297
        .size       sbox, .-sbox
