# Alpha open points, batch 9: why mesas vanish

Clean-room behavioural specification. Written from the assembly and pseudocode of the Alpha
`Cube.exe`; addresses are citations only. Constants were read from the binary's data section.

Confidence labels: **certain** = read instruction by instruction; **likely** = follows from the
code but depends on data not checked here; **to check** = needs a recording.

---

## Mesas

### 1. The answer

The mesa builder 0x5cb600 and the mound builder 0x5cbe00 do **not** have the same guard.

* Mound guard: abort if an inside cell is **not water** and has bit 0x80 of byte 3 set.
* Mesa guard: abort if an inside cell has bit 0x80 of byte 3 set. **There is no water
  exception.** (0x5cb8b4: the only test after the read. Compare 0x5cbe00, which first compares
  the type with 2 before it reads the cell again.) **Certain.**

The water that the read rules return has byte 3 = 0x82 (the constant at 0x76ddf4, set up by
0x6fae90), so it has bit 0x80. Therefore **a mesa writes nothing at all if a single inside cell
of the guard shape reads as sea or lake water.** The mesa is a land-only formation. Mounds ignore
water and are built, which is why the mounds exist at the same places. **Certain** for the code.

That the desert in the recording fails for this reason is **likely, to check** (§6): it has ground
below sea level, and grass areas with mesas are on dry ground. Nothing in either builder reads
climate, zone fields or any column field other than bottom, count, moisture and warmth, and
nothing is special about sand.

### 2. The exact rule, in order

Inputs, in stack order: x, y, z, rx, ry, rz, zone (the world in ECX). No draws anywhere; an abort
does not change the random stream. **Certain.**

**Top of a column.** zt(X, Y) = trunc(f32(f32(N(0.01X + 4394, 0.01Y + 8974) · (float)rz)
+ (float)(z + rz))). The same in both phases (0x5cb6ff..0x5cb72b, 0x5cb9a2..0x5cb9d2).

**Box.** X from x − 2rx to x + 2rx (outer), Y from y − 2ry to y + 2ry (inner).

**Phase 1, the guard** (0x5cb696..0x5cb8e7). For each column that exists, with b0 = its bottom,
for Z from zt(X, Y) down to b0 inclusive (nothing if zt < b0):

1. u = f32(f32(N(0.05Y, 0.05Z) · 0.4) + f32((X − x) / rx));
   v = f32(f32(N(0.05X + 4374, 0.05Z + 9898) · 0.4) + f32((Y − y) / ry)).
   The multiplier is the float 0.4000000059604645 (0x6fd318).
2. If f32(v² + u²) > 1, next Z. (Exactly 1 counts as inside.)
3. Read the cell by the usual rules:
   * no column or Z < bottom: grey, type 1, byte 3 = 1;
   * Z ≥ bottom + count: air (byte 3 = 0) if Z > 0, water (byte 3 = 0x82) if Z ≤ 0. Z = 0 is water;
   * a stored block with type bits 0, Z ≤ 0 and bit 0x40 clear: water (byte 3 = 0x82);
   * otherwise the stored block.
4. If byte 3 of that result has bit 0x80 set: **return at once, nothing written** (0x5cb8b8 jumps
   to the function's exit 0x5cbdde, past phase 2).

Phase 1 writes nothing, so "abort" means the whole formation is absent, never partly built.

**Phase 2, the fill** (0x5cb980..0x5cbdb3). Same box. For each column that exists, b0 = its bottom,
for Z from zt(X, Y) down to b0 inclusive:

1. Read the cell as above. Continue only if the type bits (byte 3 AND 0x1f) are 0, 2, 4, 9 or 5
   **and** bit 0x40 is clear (0x5cba66..0x5cba8e). This is the same list as the mound's.
   Type 9 sand and type 4 are both accepted. Bit 0x20 does not matter.
2. u = f32(f32(N(0.05Y, 0.02Z) · 0.5) + f32((X − x) / rx));
   v = f32(f32(N(0.05X + 4374, 0.02Z + 9898) · 0.5) + f32((Y − y) / ry)).
   If f32(v² + u²) > 1, next Z.
3. If Z = zt(X, Y): block = surface colour of (X, Y, Z) with the column's moisture (+0x04) and
   warmth (+0x08), including its type byte (0x5cbb9a..0x5cbc30).
4. Otherwise: s = sat(f32((float)(Z − b0 − 8) · 0.05000000074505806)); per channel
   trunc(f32(Cl(X, Y, Z)·(1 − s) + Rk(X, Y, Z)·s)), low 8 bits; byte 3 = 5 exactly. Rk is
   computed first, then Cl (no effect on the result).
5. Write the block with the normal writer, into this zone (0x5cbd82).

The guard shape (0.4, vertical 0.05) and the fill shape (0.5, vertical 0.02) differ, so a mesa that
passes the guard can still reach a water cell at its edge. The fill accepts type 2 and replaces it.

Reach of the guard: since |N| ≤ 1, a cell can only be inside within 1.4·rx and 1.4·ry of the centre.
The rest of the box never counts.

### 3. What can and cannot abort a mesa

| Cell in the guard shape | Byte 3 as read | Aborts a mesa | Aborts a mound |
|---|---|---|---|
| Sea: above the stored range at Z ≤ 0 | 0x82 | **yes** | no |
| Stored air, Z ≤ 0, 0x40 clear (for example the gap a formation leaves when it extends a column) | 0x82 | **yes** | no |
| Lake fill water (0x5e6ace writes the 0x82 constant) | 0x82 | **yes** | no |
| Ravine water (0x5e5f79..0x5e5f98: colour 0, byte 3 = 2) | 2 | no | no |
| Terrace pool water (later pass) | 2 | no, runs after | no, runs after |
| Formation block written over stored 0x82 water (§4) | 0x80 kept | **yes** | **yes** |
| Air above Z = 0, ground, rock, sand, grass, cells below the bottom | no 0x80 | no | no |

So the guard walks whole columns, from zt to the bottom, and in practice one thing stops a mesa: a
column within reach whose ground lies below Z = 1, or a lake. The ravine case matters when
comparing against recordings: **explicit ravine water does not stop a mesa, and the fill turns it
into mesa rock.** A low point in a ravine is therefore not an explanation; a low point of open
ground is. **Certain** for the table, from the writes listed.

### 4. The block writer keeps bit 0x80

The stored-cell setter 0x44e700 behaves as follows. When the target stored cell already has bit
0x80: if the new block's type bits are 0 the write is dropped; otherwise the new block is stored and
bit 0x80 is set on it. New cells and cells without the bit are written plainly. **Certain.**

Consequences for formations:

* Rock written over stored 0x82 water (lake fill) has byte 3 = 0x86 for a mound, 0x85 for a mesa
  body, and t | 0x20 | 0x80 for a cap. These count as type 6 or 5 but carry the bit.
* A later mound **or** mesa in the same zone whose guard shape touches such a cell aborts, because
  it is not type 2. So the mound's guard does trigger in a fresh zone, though only after a formation
  has overlapped lake water.
* Writing air over a bit-0x80 cell does nothing.
* Implicit sea is not stored, so a formation filling the sea writes plain blocks with no bit.
  Later formations then see that rock and no longer see water there, which can allow a mesa that
  would otherwise have aborted. Formation order matters; build them in draw order on the live column.

### 5. The call site (0x5e75e3..0x5e7676): no change

§4.3 of the caves spec is right on all of these. **Certain.**

* Order of the tests: home 3600 (0x5e7436, skipped when d² < 3600), object list 6400, q > 0.6 skip,
  1 − 50R ≥ 0 skip, then M at 0x5effa0, then the column read, then the coin.
* The top argument is the stored **bottom + count** of the centre column (0x5e75e8..0x5e75ee), read
  once before the coin and passed unchanged to both builders. Stored air in the column counts.
  It is not H and has no offset. Over the sea it is ≤ 0.
* Coin: one draw; odd selects the mesa path (0x5e75fc). The draw happens for every formation that
  got this far, whatever M is.
* M test: mesa only if 0.25 > M, strict (0x5e7612; the float at 0x745d98 is exactly 0.25). An even
  draw or M ≥ 0.25 gives a mound. Nothing else selects between them.
* Mesa arguments: x, y, top, rx, ry, rz / 2, zone (0x5e761b..0x5e7648). rz / 2 is integer division;
  rz is 10..34, so 5..17. Mound arguments: x, y, top, rx, ry, rz, zone. Neither call swaps rx and ry.
* The draws rx, ry, rz, x, y are as specified.

### 6. Check against the recording

For each desert formation that took the mesa branch and left nothing, using the column data of the
recording (plain terrain before formations):

1. Evaluate the phase 1 shape. Some inside cell must read as water with bit 0x80, meaning a column
   within 1.4·r with ground top ≤ 0 (at a Z that is inside for that column), or lake water.
2. Formations that **do** exist must have no such cell, given the state left by earlier formations.

If a vanished mesa has no such cell, this rule is incomplete: report the formation (x, y, rx, ry, rz)
and the lowest ground around it. Check first whether the low columns are open ground or ravines,
since ravine water does not abort (§3). Mesas fully on land should appear as type 5 with a surface
cap, sand-coloured in a desert (type 9 | 0x20 from the surface routine), so that sandy mesas with
type 5 bodies are expected on dry desert ground. **To check.**

### 7. Corrections

**`alpha-worldgen-caves.md` §2.** Replace "Same inputs and loops as §1, with these differences" by
adding this difference: the mesa guard has no water exception; it aborts on any inside cell whose
byte 3 has bit 0x80, and read water has it. Everything else in §2 stands (top, both inside tests,
the colour mix, type 5, the surface cap, no draws).

**`alpha-worldgen-caves.md` §1.** "Terrain blocks never carry bit 0x80 ... the guard never triggers"
is wrong as written. Lake fill stores 0x82 water, read water carries the bit, and blocks written
over stored 0x82 keep it (§4 here). The mound's guard skips water, so it only triggers on such
overwritten cells. The type list and the fill are right.

**`alpha-worldgen-caves.md` §4.3.** No change to the rule. Add a note that the mesa branch can
produce nothing when water is within reach, and that draws are unaffected.

**`alpha-worldgen-terrain.md` §1.3 and §1.5.** The keep rule of bit 0x80 lives in the setter 0x44e700,
which the writer 0x44e7a0 uses for every write, so it applies to every pass and not only the inline
writes of the surface pass. "Plain writes happen in a fresh zone" holds only for cells without the
bit; stored 0x82 water has it.

**`alpha-worldgen-terrain.md` §2.1.** Ravine water is stored with colour 0 and byte 3 = 2, without
bit 0x80 (already stated in §5.5 step 8; §2.1 should not be read as meaning the 0x82 block).
