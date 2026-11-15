Status: FINAL / STABLE / HELD

# Alpha open points, batch 9: why mesas vanish

Code-free behavioural contract for the client terrain-formation builders. Supported image: `binary/Cube.exe`, SHA-256 `84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF`, image base 0x00400000. Every address below is an RVA; evidence paths are relative to the original Alpha evidence root, chiefly `decompiled/client/assembly.txt` and the image bytes. Everything stated outside the unresolved list was read instruction by instruction or from original constant bytes. Names such as "mesa", "mound", "sea" and "lake" are working labels for the two builders and the cell sources described, not proved gameplay meanings.

---

## Mesas

### 1. The answer

The mesa builder at RVA 0x1CB600 and the mound builder at RVA 0x1CBE00 apply different guards.

* Mound guard: a guard-shape cell whose type bits (byte 3 AND 0x1F) equal 2 is skipped without further test; any other cell aborts the mound if its byte 3 has bit 0x80.
* Mesa guard: any guard-shape cell whose byte 3 has bit 0x80 aborts the mesa. There is no type-2 exception. The bit test at 0x1CB8B4 is the only test applied to the read cell, whereas the mound builder compares the type with 2 (0x1CC0A3–0x1CC0AA) before reading the cell a second time.

The water placeholder returned by the cell-read rules lives at 0x36DDF4 and is initialized at run time by RVA 0x2FAE90 with byte 3 = 0x82, so it carries bit 0x80 and type 2. Consequently a mesa writes nothing at all if a single cell inside its guard shape reads as that water. A mound treats the same cell as type 2 and skips it.

Within the builder bodies themselves the only column data read are the bottom (+0x10), the stored count (+0x1C), the floats +0x04 and +0x08 (used only for the surface cap), and the stored cells. Type 9 and other land types receive no special treatment in the guard.

### 2. The exact rule, in order

Inputs, in argument order: x, y, z, rx, ry, rz, zone, with the world as the object operand. Neither builder calls the imported `rand`, and none of their noise, column-lookup or colour helpers inspected to three call levels does; an abort therefore leaves the random stream untouched.

N(a, b) below denotes the two-argument noise routine at RVA 0x0C0EF0, which receives two binary64 arguments and whose result is rounded to binary32.

Column top. For column (X, Y), the top height is obtained by multiplying N(0.01·X + 4394, 0.01·Y + 8974) by rz converted to binary32, adding the binary32 conversion of the integer sum z + rz, and truncating toward zero. Both phases recompute it identically (0x1CB6FF–0x1CB72B and 0x1CB9A2–0x1CB9D2). The scale and offsets are binary64 constants at 0x302A48 (0.01), 0x31DE60 (4394) and 0x31DE88 (8974).

Box. X runs from x − 2·rx to x + 2·rx in the outer loop; Y runs from y − 2·ry to y + 2·ry in the inner loop, both inclusive.

Phase 1, the guard (0x1CB696–0x1CB8E7). Columns that do not exist are skipped. For an existing column with bottom b0, Z runs downward from the column top to b0 inclusive; nothing is visited when the top is below b0. For each Z:

1. Two shape coordinates are formed in binary32. The first is 0.4 times N(0.05·Y, 0.05·Z) plus (X − x) / rx; the second is 0.4 times N(0.05·X + 4374, 0.05·Z + 9898) plus (Y − y) / ry. The integer differences and radii are converted to binary32 before the division. The factor 0.4 is the binary32 constant at 0x2FD318 (0.4000000059604645); 0.05, 4374 and 9898 are binary64 constants at 0x31DB48, 0x31DE58 and 0x31DEA8.
2. The squares of the second and first coordinates are added in binary32. A sum greater than 1 moves on to the next Z; a sum of exactly 1 counts as inside. An unordered comparison also moves on.
3. The cell is read by the ordinary rules: a missing column or Z below the bottom yields the fixed placeholder at 0x36DDFC (byte 3 = 1, initialized by RVA 0x2FAE60); Z at or above bottom + count yields the air placeholder at 0x36DDF8 (byte 3 = 0, initialized by RVA 0x2FAE30) when Z > 0 and the water placeholder when Z ≤ 0, so Z = 0 reads as water; a stored cell (read through RVA 0x02F730) whose type bits are 0, at Z ≤ 0 and with bit 0x40 clear also yields the water placeholder; any other stored cell is returned as stored.
4. If byte 3 of the result has bit 0x80, the routine leaves at once: 0x1CB8B8 branches to the common exit at 0x1CBDDE, past phase 2.

Phase 1 performs no writes, so an abort leaves the whole formation absent rather than partially built.

Phase 2, the fill (0x1CB980–0x1CBDB3). The same box and the same per-column Z range are used. For each Z:

1. The cell is read by the rules above. Processing continues only when its type bits are 0, 2, 4, 9 or 5 and bit 0x40 is clear (0x1CBA66–0x1CBA8E); bit 0x20 is ignored.
2. The shape coordinates now use factor 0.5 (binary32 at 0x345DA0) and vertical scale 0.02 (binary64 at 0x31DB38): 0.5 times N(0.05·Y, 0.02·Z) plus (X − x) / rx, and 0.5 times N(0.05·X + 4374, 0.02·Z + 9898) plus (Y − y) / ry. The same squared-sum test with limit 1 applies.
3. At Z equal to the column top, the block is the full four-byte result of the surface routine RVA 0x1C56E0 for (X, Y, Z), given column floats +0x04 and +0x08 (0x1CBB9A–0x1CBC30), including its byte 3. That routine sets bit 0x20 in byte 3 on its only exit.
4. Below the top, a blend weight s is (Z − b0 − 8) converted to binary32, multiplied by binary32 0.05 (0x2FCDB4, 0.05000000074505806), then clamped to the range 0 to 1. Colour routine RVA 0x1F9620 is evaluated first and scaled by s; colour routine RVA 0x1C7140 is evaluated second and scaled by 1 − s; per channel the second product plus the first is truncated toward zero and its low eight bits stored. Byte 3 is set to exactly 5. The evaluation order has no effect on the result.
5. The block is written through the ordinary writer RVA 0x04E7A0 into the supplied zone (0x1CBD82).

The guard shape (factor 0.4, vertical scale 0.05) and the fill shape (factor 0.5, vertical scale 0.02) differ, so a mesa that passes the guard can still meet water cells near its edge during the fill. The fill accepts type 2 and overwrites it.

### 3. What can and cannot abort a mesa

| Cell in the guard shape | Byte 3 as read | Aborts a mesa | Aborts a mound |
|---|---|---|---|
| Above the stored range at Z ≤ 0 | 0x82 | yes | no |
| Stored cell with type bits 0 at Z ≤ 0, bit 0x40 clear (for example zero-filled cells created when a column is extended upward) | 0x82 | yes | no |
| Cell written from the water placeholder by the pass at 0x1E6ACE | 0x82 | yes | no |
| Cell written at 0x1E5F79–0x1E5F98 (colour 0, byte 3 = 2) | 2 | no | no |
| Formation block stored over a cell that already had bit 0x80 (section 4) | type 5, 6 or surface type, plus 0x80 | yes | yes |
| Air above Z = 0, and any stored cell without bit 0x80, including cells below the bottom | no 0x80 | no | no |

The guard walks whole columns from the top to the bottom. A cell written at 0x1E5F79–0x1E5F98 therefore never stops a mesa, and the fill turns it into mesa body.

### 4. The block writer keeps bit 0x80

The stored-cell setter RVA 0x04E700 behaves as follows. A negative index first extends the column downward and then stores the block plainly. An index at or past the stored count first extends the column upward; the extension routine RVA 0x050C50 zero-fills new cells. Then, if the target cell already has bit 0x80, a new block whose type bits are 0 is discarded; otherwise the new block is stored and bit 0x80 is set on it. A cell without the bit receives the new block plainly.

The writer RVA 0x04E7A0 uses this setter for every cell it stores. It does nothing when the column is missing, or when the block's byte 3 is entirely zero and Z is at or above bottom + count. At Z ≤ 0 a block whose byte 3 has none of the bits in mask 0x5F is replaced by a second water placeholder at 0x36B010 (byte 3 = 0x82, initialized by RVA 0x2F99C0) before storing. When a write lands below the old bottom, the writer also fills the gap between the written cell and the old bottom with blocks of byte 3 = 6 coloured by RVA 0x1F9620.

Consequences for formations:

* A formation block written over a cell already carrying bit 0x80 keeps the bit: byte 3 becomes 0x86 for a mound body (byte 3 = 6), 0x85 for a mesa body, and the surface type with 0x20 and 0x80 for a cap. Their type bits are 6, 5 or the surface type, not 2.
* A later mound or mesa whose guard shape touches such a cell aborts, the mound included, because the cell is not type 2.
* Writing a block with type bits 0 over a cell carrying bit 0x80 leaves the cell unchanged.
* Cells above the stored range are not stored, so a formation filling them writes blocks without bit 0x80. Later formations then read that stored block rather than the water placeholder, which can admit a mesa that would otherwise have aborted. Formations act on the live column state in the order they are generated.

### 5. The call site (0x1E75E3–0x1E7676)

* Draw order before the tests: rx is a remainder by 40 plus 10, ry a remainder by 40 plus 10, rz a remainder by 25 plus 10 (so rz is 10 to 34), followed by the x and y position draws (0x1E7356–0x1E73D0).
* Tests in order: the squared distance from a world reference point (world +0x8000F0/+0x8000F4) below binary32 3600 (0x2FD334) skips (0x1E7436); a squared distance below binary32 6400 (0x31DF68) to any entry of a position list (entry fields +0x08 and +0x10, in fixed units scaled by 1/65536) skips; a value from RVA 0x1989D0 greater than binary32 0.6 (0x2FD8DC) skips; 1 − 50·R, with R from RVA 0x1F9340, at or above 0 skips; then M is computed by RVA 0x1EFFA0; then the centre column is read; then the coin is drawn.
* The top argument is the centre column's stored bottom + count (0x1E75E8–0x1E75EE), read once before the coin and passed unchanged to either builder. Stored air in the column contributes to the count; no offset is added. The column pointer is used without a null test.
* Coin: one draw at 0x1E75F6, made for every formation that reaches this point regardless of M; an odd result selects the mesa branch.
* M test: the mesa builder is called only when binary32 0.25 (0x345D98, exactly 0.25) is strictly greater than M (0x1E7612). An even coin or M ≥ 0.25 calls the mound builder. Nothing else selects between them.
* Mesa arguments: x, y, top, rx, ry, rz halved with truncation toward zero (5 to 17), zone (0x1E761B–0x1E7648). Mound arguments: x, y, top, rx, ry, rz, zone (0x1E764F–0x1E7676). Neither call exchanges rx and ry.

### 6. Pointers to other contracts

These findings bear on the sections of other contracts listed below. Those documents were not opened during this re-certification, so their present wording is not characterised here.

* `alpha-worldgen-caves.md` §1, §2 and §4.3 (mound and mesa builders and their call site).
* `alpha-worldgen-terrain.md` §1.3, §1.5, §2.1 and §5.5 (setter/writer bit-0x80 rule and the 0x1E5F79–0x1E5F98 write).

## Re-certification (2026-11-14)

Method: RVAs 0x1CB600 and 0x1CBE00, the setter 0x04E700, the writer 0x04E7A0, the extension routine 0x050C50 (decompile shape only), the call site 0x1E7356–0x1E767B, the writes at 0x1E5F79–0x1E5F98 and 0x1E6ACE, the end of the surface routine 0x1C56E0, and the placeholder initializers 0x2FAE30, 0x2FAE60, 0x2FAE90 and 0x2F99C0 were read in `decompiled/client/assembly.txt`. Every cited constant was read from `binary/Cube.exe`. Callee trees were searched for calls to the imported `rand`. No reconstruction was consulted.

Confirmed: argument order; column-top formula and constants; box and loop order; inclusive downward Z range; both shape formulas, factors and vertical scales; inclusive limit 1; cell-read rules; the abort branch and exit address; absence of writes in phase 1; the fill's type list and bit-0x40 test; surface cap, blend weight, clamp, evaluation order and byte 3 = 5; writer use; setter bit-0x80 rule; the call-site order, constants, coin, strict 0.25 test, argument lists and rz/2 range; that no draws occur in the builders.

Corrected:
- All addresses were virtual addresses; they are now RVAs (subtract 0x400000).
- Old: the mound guard skips cells that are "not water". New: it skips cells of type 2, which include read water and the type-2 cells written at 0x1E5F79–0x1E5F98.
- Old: the surface cap uses column "moisture (+0x04) and warmth (+0x08)". New: column floats +0x04 and +0x08; the names were unproved.
- Added: writer substitution of the 0x36B010 water placeholder at Z ≤ 0, the downward gap fill, the upward zero-filled extension, the missing-column and air-above-range no-ops, and the unchecked column pointer at the call site.
- Old: the call-site section endorsed another contract's section and the "draws as specified" there. New: the draw order and ranges are stated directly from original evidence.
- Removed: confidence labels, the recording-check procedure and the proposed edits to other contracts (replaced by pointers only), the claim that the "reach" of the guard is 1.4·rx/1.4·ry, the terrace-pool table row, and climate/desert explanations, all moved to the unresolved list below.

Unresolved:
- The value range of N at RVA 0x0C0EF0 is not established, so the guard's spatial reach (1.4·rx and 1.4·ry if |N| ≤ 1) is unproved.
- Whether the 0x1E6ACE pass is a lake fill, whether the 0x1E5F79–0x1E5F98 write is ravine water, and whether any later "terrace pool" pass writes type-2 water after formations are unconfirmed labels.
- Whether particular observed missing mesas, desert or otherwise, are explained by this guard is not established from code; it depends on terrain data.
- Whether centre columns over open water have a top of 0 or below is data-dependent and unproved.
- Which surface type, for example type 9, the cap receives in a given climate depends on RVA 0x1C56E0 branches not audited here.
- Absence of draws is established to three call levels from the builders; deeper callees were not exhaustively searched.
