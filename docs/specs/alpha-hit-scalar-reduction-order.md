Status: FINAL / STABLE / HELD
# Alpha hit scalar-reduction arithmetic order

Scope: one block of the client hit applier, RVA 0x195B57–0x195BD5, together with its entry test at 0x195B3E–0x195B55. Supported image: `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, image base 0x00400000. All addresses are RVAs; evidence paths are relative to the original Alpha evidence root. The facts below come from the original image and its instruction listing (`decompiled/client/assembly.txt`), whose bytes for the whole scoped range were compared with the image file and matched. No replacement code, tests, captures, other contracts or reconstruction supplied facts.

## Entry

The block runs only when the binary32 hit amount at hit+0x10 compares strictly greater than positive zero. A zero or negative amount skips everything below, including the action and timer writes; an unordered comparison (a NaN amount) also skips it. How the hit and its target were admitted before this point is outside scope. The value reduced here is the binary32 scalar at creature+0x1190, which is a different field from both health and the hit amount.

## Evaluation and store order

The precision names below describe operand and result widths at each step. They do not fix a global rounding mode, which this contract does not trace.

1. Integer exponent input. The byte at creature+0x1A8 is zero-extended to 32 bits, and the 32-bit word at creature+0x190 is read separately. One is subtracted from that word first, using 32-bit integer arithmetic; the zero-extended byte is then added to the result, again in 32-bit integer arithmetic. Both steps keep only the low 32 bits (wrap modulo 2^32). There is no wider sum and no floating-point step here. The final 32-bit pattern is treated as a signed integer by the next step.
2. Exponent. That signed integer is converted to binary32 and multiplied by binary32 0.25, giving a binary32 exponent, which is then widened to binary64. The whole integer is converted once, after the decrement and addition; the two inputs are never converted separately.
3. Power. The runtime's precise power routine is called with binary64 base 2.0 and the widened exponent, returning a binary64 result. This finishes before the hit amount is read again.
4. Numerator and denominator. The hit amount is read again as binary32 and multiplied by binary32 0.25 to form a binary32 numerator. Only afterwards is the binary64 power result narrowed to binary32 to form the denominator.
5. Quotient. The quartered numerator is divided by the narrowed denominator, producing binary32. The amount is quartered before division, not after, and no reciprocal of the power is formed.
6. Subtraction. Only now is the old binary32 scalar at creature+0x1190 read; the quotient is subtracted from it with a binary32 result. Nothing in steps 4–6 is carried in binary64. The hit amount itself is not modified.
7. Store and clamp. Positive zero is compared with the difference; the difference is then written to creature+0x1190 unconditionally, before the branch that uses the comparison. If the difference is ordered and strictly below zero, a second write replaces it with the all-zero 32-bit pattern (positive zero). A positive difference, either signed zero, or an unordered (NaN) difference keeps the first write. Because the test is strict, a negative-zero difference is not rewritten as positive zero.
8. Action and timer. After the scalar write and any clamp, the action byte at creature+0x68 is compared with 0x54 (decimal 84). When equal, that byte is first set to zero and then the 32-bit word at creature+0x12C is set to 3000 (bit pattern 0x00000BB8). When not equal, neither write happens. Processing then continues at 0x195BD5.

## Constants

Read directly from the image file:

- Binary64 base 2.0 at RVA 0x345E18, pattern 0x4000000000000000, bytes `00 00 00 00 00 00 00 40`.
- Binary32 0.25 at RVA 0x345D98, pattern 0x3E800000, bytes `00 00 80 3E`. The same constant serves both the exponent and the numerator multiplications.
- The clamp value is the all-zero 32-bit word.

## Instruction evidence

From `decompiled/client/assembly.txt`, each range re-checked against `binary/Cube.exe`:
- entry amount read, zero setup and comparison: 0x195B3E–0x195B55;
- byte and word reads, 32-bit decrement and addition: 0x195B57–0x195B6D (the base-2.0 load is interleaved at 0x195B64 and does not affect the integer order);
- signed integer to binary32 conversion, quarter multiplication and widening: 0x195B6F–0x195B7E;
- power call: 0x195B81;
- amount reread and quarter multiplication: 0x195B86–0x195B8B;
- narrowing of the power result and division: 0x195B93–0x195B97;
- scalar read and subtraction: 0x195B9B–0x195BA3;
- zero comparison, unconditional write and conditional overwrite: 0x195BA7–0x195BB7;
- action test and ordered byte and timer writes: 0x195BC1–0x195BCB.

The call target at RVA 0x28D8D4 is an import stub whose indirect jump reads import address slot VA 0x006FC454. In the image's import table that slot belongs to `MSVCR110.dll`, entry `_libm_sse2_pow_precise`. This contract fixes the binary64 call boundary and the conversions around it. It does not describe the DLL's internal power approximation.

## Limits

Not established here: global SIMD rounding control, denormal treatment, exception masks, recovery from nonfinite inputs other than the comparison outcomes stated above, concurrent mutation, and behaviour when the narrowed power result is zero or nonfinite. The integer widths and wrap order in step 1 hold as instruction facts regardless of those limits. The signedness with which other code interprets the timer word at +0x12C is not established by this block.

## Re-certification (2026-11-14)

Re-checked by a source-side author against the original client image, its instruction listing and its import table; listing bytes for 0x195B30–0x195BE0 and the import stub were compared with the file and matched. The Infinite-Cubes reconstruction was not consulted.

Confirmed:
- Entry test (strictly greater than zero), field offsets +0x10, +0x190, +0x1A8, +0x1190, +0x68, +0x12C.
- Integer order (decrement, then add the zero-extended byte; 32-bit wrap), signed conversion, quarter scaling, widening, power call, reread and quartering of the amount, narrowing after the numerator, binary32 division and subtraction.
- Unconditional provisional store before the strict-negative clamp; negative zero retained.
- Action 0x54 test, byte cleared before the timer is set to 0xBB8.
- Constants and their RVAs and byte patterns; the import slot resolves to `_libm_sse2_pow_precise`.

Corrected:
- The timer at +0x12C was called a "signed 32-bit timer"; the block only writes a 32-bit word, so its signedness is now left unestablished.
- The closing status line was at the end of the document; it is now the first line. The earlier provenance sentence about "held artifacts" has been replaced with this author's own evidence statement.
- Added (newly confirmed, not previously stated): a NaN amount also skips the block, and a NaN difference keeps the provisional write.
- Prose rewritten in independent wording.

Unresolved:
- DLL power-routine internals, rounding and exception state, denormals, zero or nonfinite power results, concurrency.
- Meaning of the fields at +0x190, +0x1A8 and +0x1190 beyond their arithmetic roles here.
