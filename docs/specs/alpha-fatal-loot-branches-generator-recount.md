# Fatal-loot generators P and Q: draw recount

Status: FINAL / STABLE / HELD.

This independently authored, code-free note answers one question about the held contract `alpha-fatal-loot-branches.md`: whether the draw total stated for generator P in its section 11.2 is consistent with that section's itemised list. It corrects one statement of that contract and confirms the others named below. Where this note and the held contract differ, this note governs. The held contract was not edited.

Supported originals and conventions are those of the held contract: `binary/Cube.exe` (client), SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, and `binary/Server.exe` (server), SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`, preferred base `0x00400000`, addresses given as RVAs, evidence paths relative to `Documents/disassembled alpha cube world`. The Infinite-Cubes reconstruction was not consulted.

## Method

Generator P (client `0x1F8AD0–0x1F8E01`, server `0x12C4E0–0x12C814`) and generator Q (client `0x1F51E0–0x1F56B5`, server `0x128BF0–0x1290C8`) were re-read in `decompiled/client/assembly.txt` and `decompiled/server/assembly.txt`. The byte column of those listing ranges had already been checked against each image with no mismatch. For this recount, every call through the random import and every call to the candidate-append routine (client `0x0C7830`, server `0x128530`) inside each range was enumerated mechanically, in address order, together with the reduction applied to each draw and each write to the candidate's category, subtype and `+0x0D` bytes. This was done separately for the client and for the server; the two enumerations are identical for each generator. Every random call in either routine sits on the straight-line path taken when the selector is negative; none is inside a loop.

## Finding

**The itemised list in section 11.2 of the held contract is correct and complete. The stated total is wrong.** With selector −1, generator P makes **16** draws including the final selection draw, and builds 11 candidates. The figure "17 draws" in section 11.2 is an arithmetic error in the held contract; no draw is missing from the itemised order. The held contract's group 3 contributes four draws, not five.

**Section 11.3 is confirmed.** With selector −1, generator Q makes 24 draws including the final selection draw, and builds 21 candidates.

## Generator P, re-verified order (selector −1)

Each numbered line is one draw, in execution order. "Modifier" means the draw's signed remainder modulo 100, stored at candidate `+0x04`; a candidate is appended immediately after its modifier draw. Every candidate has category 3.

Group 1 (included when the selector is 1 or negative), byte `+0x0D` equal to 1:

1. signed remainder modulo 3 gives the subtype, 0 to 2;
2. modifier; first candidate appended;
3. signed remainder modulo 3, plus 15, gives the subtype, 15 to 17;
4. modifier; second candidate appended;
5. modifier for subtype 13; third candidate appended.

Group 4 (selector 4 or negative), byte `+0x0D` still 1:

6. modifier for subtype 3; fourth candidate appended;
7. modifier for subtype 5; fifth candidate appended;
8. modifier for subtype 4; sixth candidate appended.

Byte `+0x0D` is then set to 2 unconditionally, whatever the selector.

Group 2 (selector 2 or negative), byte `+0x0D` equal to 2:

9. modifier for subtype 6; seventh candidate appended;
10. a draw whose result is discarded;
11. modifier for subtype 8; eighth candidate appended.

Group 3 (selector 3 or negative):

12. modifier for subtype 10, byte `+0x0D` equal to 2; ninth candidate appended;
13. modifier for subtype 11, byte `+0x0D` equal to 2; tenth candidate appended;
14. parity draw for subtype 12: byte `+0x0D` becomes 11 when the draw's signed remainder modulo 2 is nonzero and 12 when it is zero;
15. modifier for subtype 12; eleventh candidate appended.

Final:

16. selection draw, reduced by unsigned 32-bit division by the candidate count; the remainder indexes the candidate copied to the destination.

Group totals: 5, 3, 3, 4 and 1, sum 16. The group order (1, 4, 2, 3), every subtype value, every `+0x0D` value and the parity rule for the last candidate are as the held contract's section 11.2 states them. [Client `0x1F8B79–0x1F8DD2`; server `0x12C589–0x12C7E2`.]

## Generator Q, re-verified order (selector −1)

Every candidate has subtype 0 and one modifier draw of the same form, after which it is appended.

- Selector 1 or negative, byte `+0x0D` equal to 1: categories 7, 4, 5, 6 — four draws.
- Selector 3 or negative, byte `+0x0D` equal to 25: categories 7, 7, 4, 5, 6 — five draws.
- Selector 2 or negative, byte `+0x0D` equal to 26: categories 7, 7, 4, 5, 6 — five draws.
- Selector 4 or negative, byte `+0x0D` equal to 27: categories 7, 7, 4, 5, 6 — five draws.
- Unconditional: one parity draw setting byte `+0x0D` to 12 when the signed remainder modulo 2 is nonzero and 11 when it is zero, then the modifier draw for category 8 — two draws.
- Unconditional: one parity draw with the same rule, then the modifier draw for category 9 — two draws.
- Final selection draw, reduced by unsigned 32-bit division by the candidate count — one draw.

Sum 24 draws, 21 candidates, as the held contract's section 11.3 states. [Client `0x1F5288–0x1F5686`; server `0x128C98–0x129096`.]

## Effect on the rest of the held contract

Sections 6, 9 and 11.6 of the held contract refer to "a generator P run" without restating a number; wherever a total for a selector −1 run of generator P is needed, it is 16, and for generator Q it is 24. No other statement of the held contract is changed by this note.

## Unresolved

Unchanged from the held contract: the candidate-list growth routine was not audited and is assumed draw-free; the game meaning of the category, subtype and `+0x0D` numbers is not asserted; behaviour of the random source itself is outside scope.
