Status: FINAL / STABLE / HELD
# Kind-89 boundary of the original relationship predicate

Scope: the single client predicate at RVA 0x044680 in `binary/Cube.exe` (SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, image base 0x00400000), evaluated for an object whose kind is exactly decimal 89. All addresses are RVAs. Evidence paths are relative to the original Alpha evidence root (`Documents/disassembled alpha cube world`). The instruction listing `decompiled/client/assembly.txt` for RVA 0x044680–0x044759 was compared byte for byte with the image file and matched; the exported function file for this routine was used only to locate it. No other contract, reconstruction or replacement material is evidence here.

## Inputs and decision order

The object is passed by pointer in ECX. Two fields are read, in this order:

1. The 16-bit word at object+0x7E, zero-extended. If it has any bit of mask 0x1A00 set, the predicate answers false at once and never reads the kind. The mask consists of 0x0200, 0x0800 and 0x1000, which are bits 9, 11 and 12 when bit 0 is the least significant bit.
2. Only when all three of those bits are clear, the full 32-bit kind at object+0x64 is read and tested against the predicate's list of admitted kinds. Decimal 89 (0x59) is the last entry of that list, tested at RVA 0x04474F; equality there selects the true exit.

[Evidence: `binary/Cube.exe` and `decompiled/client/assembly.txt`, RVA 0x044680–0x04468F and 0x04474F–0x044759.]

## Results for kind 89

With every flag bit not named in a row equal to zero:

| Word at object+0x7E | Mask bits present | Boolean result (AL) | Whole EAX at return |
| --- | --- | --- | --- |
| 0x0000 | none | 1 (true) | 0x00000001 |
| 0x0200 | bit 9 | 0 (false) | 0x00000200 |
| 0x0800 | bit 11 | 0 (false) | 0x00000800 |
| 0x1000 | bit 12 | 0 (false) | 0x00001000 |
| 0x1A00 | bits 9, 11, 12 | 0 (false) | 0x00001A00 |

Any word containing at least one of the three mask bits yields false. Flag bits outside the mask have no influence on the decision, so kind 89 with the three mask bits clear yields true whatever the other bits are. Only the two bytes at +0x7E/+0x7F form the flag input. The kind test compares all 32 bits; a value that merely has low byte 0x59 is not kind 89.

## Return register

The answer is carried in AL only. The true exit loads AL with 1; the false exit clears AL. Neither exit clears the upper 24 bits of EAX. On the early flag rejection EAX therefore still holds the zero-extended flag word with its low byte zeroed, which is why the false rows above show nonzero whole-EAX values. On the true exit for kind 89, EAX held 0x59 before AL was replaced, so the whole register reads 0x00000001. The relationship callers inspected compare or save AL only and do not treat whole EAX as the boolean. [Evidence: `decompiled/client/assembly.txt`, RVA 0x044754–0x044759; call sequences at RVA 0x196CD7–0x196CE7, 0x196CF7–0x196D07, 0x1988D1–0x1988E1 and 0x1988F1–0x198901, each also checked against the image bytes.]

## Limits

The predicate writes no memory, calls nothing, draws no random number and performs no floating-point work. Its outcome depends only on the two fields named above; it does not consult a clock, health, map order, identifiers or the floating-point control state. It does not test the pointer for null before reading the flag word, so it presumes readable input storage. This contract does not establish what produces the flag bits or what they mean in gameplay, caller-level relationship admission, behaviour under concurrent writes, any actor role name, a server counterpart, or the predicate's full behaviour for any kind other than exactly 89.

## Re-certification (2026-11-14)

Re-checked by a source-side author directly against the original client image and its instruction listing (listing bytes for every cited range compared with the file and found identical). The Infinite-Cubes reconstruction was not consulted.

Confirmed:
- Image hash, image base and routine RVA 0x044680.
- Zero-extended 16-bit flag read at +0x7E; mask 0x1A00 (bits 9, 11, 12); flag rejection precedes the kind read.
- Full 32-bit kind read at +0x64; 0x59 is the final list comparison at 0x04474F.
- AL-only result; true exit at 0x044754, false exit at 0x044757; upper EAX bits not normalised; all table values.
- The four cited caller sequences compare or preserve AL.
- No writes, calls, random draws, floating-point work or null check.

Corrected:
- No factual corrections. The prose was rewritten in independent wording and reorganised into input, result and limit sections; the true-exit whole-EAX value is now explained explicitly.

Unresolved:
- Producers and gameplay meaning of the three mask bits.
- Predicate results for kinds other than 89, caller-level admission, concurrency and any server counterpart (outside scope).
