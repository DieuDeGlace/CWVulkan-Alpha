# Alpha open points 12: the vegetation pass, draw by draw

Clean-room audit of `alpha-worldgen-vegetation.md` §2 to §4 against the Ghidra listing of the
Alpha client, instruction by instruction, range 0x5ebc60 .. 0x5ed2c1. Constants were read from
`binary/Cube.exe`. Addresses are citations only. AlphaVulkan source was not read, so nothing
here says what our code does.

The rules were then replayed against the recordings (§9): a small throwaway checker took the
stored blocks of each recorded zone, ran the rules below on the real `rand` stream of that
zone, and compared every sprite and item with the recording.

**This file replaces §2, §3 and §4 of the vegetation spec wherever they differ.**

## 1. Result in short

| Finding | Confidence |
|---|---|
| Sprites stand at (x + 0.5, y + 0.5, zt). Exact fixed point in §2. Lilies at zt − 0.1. Street lights keep the block corner. | high, listing and all recordings |
| **Temperate grass: the spec swapped the two moisture branches and inverted one test.** mo > 0.5 gives 4 / 12. mo ≤ 0.5 gives 3 / 2. (§4.4) | high, listing and replay |
| **Each use of a random value is its own draw.** Gate, rotation, kind and scale are separate calls. Reed 2, lily 3, street light 2 draws. (§5) | high, listing and replay |
| Kind 12 costs one extra draw that kind 3 does not. This is what moved the items near home. (§6) | high |
| The reed's mode is exactly 4, not default + flag. (§3) | high, listing and recordings |
| `above` comes from the stored array only: past the top it is plain type 0, never the implicit water. (§2) | high, listing |
| The 13 items in the fire land are **ravine items** (category 11, subtype 19), from `alpha-worldgen-caves.md` §7. They are not in vegetation §4. (§7) | high, listing and stream check |
| Vegetation §4 is correct as a rule. Only its list offset and the draw wording need care. (§7) | high |

## 2. Reading the column, and positions

**Loop.** x outer, y inner, each column's stored blocks from index 0 upwards (0x5ebc60 ..
0x5ec5e8). For each index i:

* If i < count − 1 and the stored block i + 1 is solid (type neither 0 nor 2): skip this i.
* here = type of block i, above = type of block i + 1, zt = bottom + i + 1.
* Type is byte 3 and 0x1f (0x434c10).

**Correction to §2: `above` is a raw array read** (0x59aaf0), not the block reader of terrain
§1.4. Index ≥ count returns a zeroed block, so the type is 0 at any height. A stored block of
type 0 at z ≤ 0 also stays 0. Nothing in the loop sees the implicit water of z ≤ 0. Two results:

* Under the sea, the top stored block still has above = 0, so a lake bed (here = 3) there can
  get a reed.
* Stored water (here = 2) with above = 0 is common at z ≤ 0 too. The lily's zt > 0 test is what
  stops those.

Only the street-light cell test (§5.1) uses the normal reader 0x42f7e0.

**Which case runs** (0x5ebd3b .. 0x5ec308):

* above = 0 and here = 11: street light (§5.1).
* above = 0 and here = 3: reed (§5.2).
* above = 0 and here = 2: lily (§5.3).
* Otherwise, here ∈ {4, 9, 10, 12}, with above 0 or 2: ground cover (§5.4).

Note the cases are exclusive and nothing else is tried for types 11, 3, 2.

**Positions** (fixed point, 1/65536 block). Confirmed on every sprite and item of all recordings:

| Record | x, y | z |
|---|---|---|
| Plant sprite, every kind but the lilies | x·65536 + 32768, y·65536 + 32768 | zt·65536 |
| Lily 31, 32 | same | zt·65536 − 6553, read as zt − 0.1 (fraction 58983) |
| Vegetation item | same | zt·65536, then the settle pass (§7.3) |
| Ravine item | same | zf·65536, then the settle pass |
| Street light, static record | x·65536, y·65536, **no 0.5** | zt·65536 |

Why: 0x468fc0 subtracts trunc(v · −65536), so it **adds** v, and it is called with 0.5. 0x42f5a0
subtracts trunc(v · 65536) and is called with 0.1. That closes vegetation open question 2. The
spec's "block corner" in §1.3 is wrong for sprites.

## 3. The sprite record

Constructor 0x58e9f0 sets +0x2c, +0x30, +0x34 = 1.0 and **+0x38 = 2**. So the flags word is the
"mode" field, default 2. "Flag 4" means OR 4, which gives **6**.

The reed is different: the code stores +0x38 = **4** outright (0x5ec0d3), so it is 4, not 6.

Modes seen in the recordings, all as the listing says:

| Mode | Kinds |
|---|---|
| 2 | 0, 1, 6, 10, 31, 32, and kind 4 when made by the hot branch |
| 4 | 22 |
| 6 | 3, 5, 7, 9, 11, 12, and kind 4 when made by the temperate branch |

Rotation arithmetic, each step rounded to float:

| Form | Used by |
|---|---|
| **R360** = (f32(d) · 360f) / 32767f (multiply first) | reed, lily, hot grass |
| **R90** = float((d and 3) · 90) | underwater, fire, temperate, desert flowers |
| **I360** = (f32(d) / 32767f) · 360f (divide first) | ground items |

## 4. Corrected kind rules

Write nA = N(0.01x + 9843, 0.01y + 8437), nB = N(0.01x + 34234, 0.01y + 234234) in double
arguments, and nF = N(f32(f32(x)·0.01f + 34234f), f32(f32(y)·0.01f + 234234f)) with
float arguments.

All compare with plain `>` on floats. Constants: 0.75, 0.25, 0.5 exact; 0.8 and 0.2 are floats.

### 4.1 Underwater, fire

As the spec says. Kind 7 if nA ≤ 0. Otherwise 5 + (d and 1). Fire: 9 if nA > 0, else 10.

### 4.2 Hot (warmth > 0.75), mo ≤ 0.25

Kind 28 if d even, else 27.

### 4.3 Hot, mo > 0.25

Unchanged from the spec: nA > 0: 4 if nF > 0, else 3. nA ≤ 0: 12 if nB ≤ 0.5, else 11.

### 4.4 Temperate (warmth ≤ 0.75): **replaces the spec**

0x5ece85 .. 0x5ed0c4. The moisture test jumps to the *lower* branch when mo ≤ 0.5, and the
flags survive to the jump, which is easy to misread.

| Moisture | nA | Test | Kind |
|---|---|---|---|
| mo > 0.5 | nA > 0 | nF > 0 | **12** |
| mo > 0.5 | nA > 0 | nF ≤ 0 | **4** |
| mo ≤ 0.5 | nA > 0 | nF > 0 | **2** |
| mo ≤ 0.5 | nA > 0 | nF ≤ 0 | **3** |
| either | nA ≤ 0 | nB > 0 | 0 |
| either | nA ≤ 0 | nB ≤ 0 | 1 |

The spec had the 2/3 pair under mo > 0.5 and the 4/12 pair under mo ≤ 0.5, and it also had 2 and
3 the wrong way round. This fits your counts exactly: your rule's kind 2 is Alpha's 4 (nF ≤ 0),
and your kind 3 is Alpha's 12 (nF > 0).

Kinds 2, 3, 4, 12 get OR 4. Kind 12 takes a scale draw (§5.4). Placed if here = 4, or the kind is
2, 3 or 4.

The recordings hold no temperate kind 2 or 3 at all (no recorded zone has temperate ground with
mo ≤ 0.5), so that row is from the listing alone.

## 5. Every draw, in order

Each **D** is one `rand()` call. Tests with no D make no draw. A failed test ends the column
entry unless it says otherwise.

### 5.1 Street light (here = 11, above = 0)

1. q(x, y) > 0.75, and (90y + x) mod 470 = 0.
2. **D1**: D1 and 15 = 0.
3. Cells zt .. zt + 6 all non-solid, through the normal reader 0x42f7e0.
4. warmth > 0.8 (no draw): kind 51, scale (1, 1, 8). Else kind 50, scale (2, 2, 8).
5. **D2**: rotation index = D2 and 3.

Static record: inserted in the list at zone + 0x0c. All six street lights of seed-41-town show
kind 50, whole-number positions, scale (2, 2, 8), rotation 0..3.

### 5.2 Reed (here = 3, above = 0): 1 or 2 draws

1. warmth > 0.2.
2. |N(f32(0.05x + 9843), f32(0.05y + 8437))| > 0.5 (0.05 is the float).
3. **D1**: D1 and 7 = 0.
4. **D2**: rotation R360(D2).

Kind 22, scale 0.09 (bits 0x3db851ec), mode 4. **Rotation is a separate draw.**

### 5.3 Lily (here = 2, above = 0): 1 or 3 draws

1. warmth > 0.2, and zt > 0.
2. |N(f32(0.05x + 24234), f32(0.05y + 53565))| > 0.7.
3. **D1**: D1 mod 10 = 0.
4. **D2**: rotation R360(D2).
5. **D3**: kind = 31 + (D3 and 1).

Scale 0.09, mode 2, z = zt − 0.1. Three separate draws. Taking the kind from D1 can only give
31, since a number divisible by 10 is always even.

### 5.4 Ground cover (here ∈ {4, 9, 10, 12})

Gate, for every case: |N(f32(0.05x + 9843), f32(0.05y + 8437))| > 0.6, then **G**: G and 7 = 0.

Then by zt and type. "[...]" means only in that case.

| Case | Draws after G, in order |
|---|---|
| zt ≤ −5, any type | **R**: R90. If nA > 0: **K**: kind 5 + (K and 1). **S**: scale |
| −4 ≤ zt ≤ 0 | none |
| zt ≥ 1, here = 12, nA > 0 | **R**: R90, kind 9 |
| zt ≥ 1, here = 12, nA ≤ 0 | **R**: R90, kind 10. **S**: f32(S · 0.02 / 32767 + 0.03) in double |
| zt ≥ 1, here = 10 | item step only (§7.1) |
| hot, mo ≤ 0.25 | **P**: P mod 100 = 0, else no plant. [**R**: R90. **K**: kind.] Then item step |
| hot, mo > 0.25 | **R**: R360. [Kind 11 or 12: **S**.] Then item step |
| temperate | **R**: R90. [Kind 12: **S**.] Then item step |

Scales:

* Underwater: base 0.1, kind 5 base 0.075. Final = base · (f32(S)/32767f + 1f) in float.
* Kind 11: (f32(S)·0.05f)/32767f + 0.05f.
* Kind 12: (f32(S)·0.02f)/32767f + 0.1f.
* All others 0.075.

**The S draw of kinds 11 and 12 is made even when the plant is then not placed** (for example
on sand, here = 9). Likewise R. Only the list append is skipped. "Placed" for hot: here = 4 or
kind 3. Temperate: §4.4. Desert flowers are always placed.

The item step runs for here = 4 and here = 10 only (§7.1). Sand and fire have none.

**Reading the spec.** Its wording "draw" reused for several values on one line is what caused
the trouble. In §3 and §4 of that spec, every word "draw" that is not the gate is a new call.

## 6. Why the wrong rules showed the way they did

* **Fire land matched fully** because that ground has none of the changed branches: it uses only
  R and thorn S, as the spec says.
* **Near home**, Alpha's kind 12 makes the S draw where a kind 3 makes none. Each such column
  shifts the stream by one. From then on every gate and item in the zone lands elsewhere, which
  is why only 3 of 148 items stayed in place, though the totals were close.
* **Reed and town counts** (344 / 366, 108 / 93) fit the same shift: the reed gate lands on other
  draws once the stream has moved.
* **Desert and styles 2, 4:** any reed, lily, or temperate column in the zone shifts the rest of
  it, which fits about 5% of plants moving. With the rules here those zones replay exactly (§9).

**Lilies in style 1 (Alpha 1, ours 14) are not explained by this.** The stored blocks of those
zones have only 19 columns that pass the lily's tests (273 stored water tops with zt = 1; none
higher), so about 2 lilies are expected, and Alpha has 1. Your 14 needs about 140 candidates. A
draw fix alone will not bring that down. Check the zt > 0 test and which columns have stored
water in our output. For scale: the same blocks hold 19 281 water tops with above = 0 at all
heights, and allowing zt ≥ −4 would give about 158 candidates, near your number. That is only a
lead, as I cannot see our code.

The same data corrects spec §7: **lilies do occur at sea level**, on stored water at z = 0
(zt = 1). The lily in style 1 is one (z = 0.9).

## 7. World items

### 7.1 Vegetation items: spec §4 is right

The list is **zone + 0x30** (0x44d590), not +0x24. The item step comes after the plant decision
in §5.4, whether or not a plant was placed.

* **here = 4:** **D**: D mod 150 = 0. **R**: rotation I360(R). **T**: T even → subtype 15, default
  scale 1/14. T odd → subtype 22, scale 0.1.
* **here = 10:** **D**: D mod 300 = 0. **R**: I360(R). **T**: made but unused; subtype 24, scale 0.1.

Category 11, count (+0x10) 1, tag (+0x138) 2, position as in §2.

### 7.2 All sources of generated items

A byte scan of the executable for calls to the append routine found seven generator sites. One
is in bytes Ghidra did not disassemble (jump-table cases of 0x5e4850, 0x5e646b .. 0x5e699f), so
`calls.csv` misses it.

| Site | Items | Category, subtype | Scale | Tag | Rotation | Spec |
|---|---|---|---|---|---|---|
| 0x5e65d2 ravine | 1 per hit | **11, 19** (+0x0d = 9) | 0.1 | **0** | **radians** | caves §7 |
| 0x5ed176 | grass | 11, 15 or 22 | 1/14 or 0.1 | 2 | I360 | §7.1 |
| 0x5ed2bc | snow | 11, 24 | 0.1 | 2 | I360 | §7.1 |
| 0x5b1ead | town crop plots | 11, 16 (pumpkin) or 17 (pineapple) | 0.1 | 2 | R360 | structures, field plots |
| 0x5bb34d, 0x5bb6bd | ruin plots, kinds 17, 19 | **type 25 mana cube** | 1/14 | 0 | none | open points 5 §3 |
| 0x5d3560 | cave and dungeon tables | various, see below | 0.06 | 1 | degrees | structures §1, loot |

Runtime drops (0x59aeb0) are the only other source. They never run in generation.

What the recordings hold, as a check:

| Recording | Items |
|---|---|
| style 5 (fire) | 11/19 × 13, nothing else |
| home | 11/15 × 76, 11/22 × 68, 11/19 × 1, tag 1 loot × 3 |
| town | 11/15 × 78, 11/22 × 79, 11/16 × 619, 11/17 × 476, 11/19 × 1 |
| style 3 (snow, dungeon) | 11/24 × 66, 11/19 × 4, tag 1 loot |
| styles 1, 2, 4 | 11/15, 11/22, 11/19, tag 1 loot |

**No recording holds a mana cube.** The item you may be thinking of, 11/19, is the glowing
"shimmer mushroom" from ravines, and the town item is the same. A mana cube is type 25, with
+0x04 = D mod 200, built by 0x5f77b0.

### 7.3 Ravine items: the 13 in the fire land

The spec is in caves §7. Listing and spec agree, and all 14 ravine items checked sit on the real
stream with exactly this order:

Condition: top − floor > 2 and (x + 3y) mod 7 = 0.

1. **D1**: D1 mod 30 = 0.
2. **D2**: case = D2 and 3. Case 0 needs top ≤ S.
3. **D3**: D3 and 3 = 0.
4. Record: category 11, subtype 19, +0x0d = 9, count 1, scale 0.1, tag 0, position
   (x + 0.5, y + 0.5, zf) with zf = trunc(floor).
5. **D4**: rotation = f32(D4 · 2π / 32767) in double, in **radians**.

Style 5 test values: zone (32706, 32684) D4 = 2053 at draw index 539; zone (32709, 32684) D4 =
10045 at index 205; dungeon zone (32795, 32798) D4 = 2187 at index 184.

The renderer takes +0x130 as degrees (`alpha-world-items.md` §1), so these items just show a
very small angle. Keep the value as stored for parity.

Fire land plants match in our code, so the four draws must already be made there. My guess is
that the record is drawn for but not stored, which would be one append. I cannot see our code.

### 7.4 Settling

After the generator's passes, every item with bit 0 of +0x138 clear sinks: while the cell at
(x, y, z − 0.5) is not solid, z −= 1 (`alpha-open-points-5.md` §4.1). So tags 0 and 2 settle, and
tag 1 does not. Water is not solid, so an item on a lake bed stays there. The pass makes no draws.

## 8. Other spec corrections

* §1.3: the plant list is **zone + 0x04** (`LEA ECX,[EDI+4]` at every append, and the tree
  records too, 0x5e20da, 0x5e47c3). This closes trees open question 4. Zone + 0x24 is another
  list (the id pass at 0x5ec5ee walks it).
* §1.3: the record fields are +0x20 scale and +0x24 rotation, with one scale, not a scale vector.
* §7: "Lilies appear only on raised lakes" is false (§6).
* Trees: records 60 and 61 agree with trees §5. Kind 61 has x + 0.5, y + 0.5 and whole z in every
  recording. +0x20..+0x2b hold stale stack values in kind 60 and 61, as that section says (the
  recordings show junk sizes), so a comparison must ignore size and rotation for these kinds.
* Vegetation open question 2 is closed (§2).

## 9. Replay against the recordings

The checker used the recorded blocks and the rules of §4, §5 and §7.1, seeded with `srand(A +
65536·zy + zx)` (A = 172 for seed 41). It knew warmth and moisture only by trying each branch
at each column where they mattered, so a wrong rule could only pass if no record contradicted it.

| Recording | Zones replayed to the last record | Not finished |
|---|---:|---|
| seed-41-dungeon (home) | 16 | 1, no start found |
| style 1 | 22 | |
| style 2 | 18 | 1, no start found |
| style 4 | 15 | |
| style 5 (fire) | 19 | |
| seed-41-town | 9 | 1 stopped at record 510 of 906; 1 no start found |
| **Total** | **99 of 103** | 4 |

That is about 115 000 sprites and 289 vegetation items, each matching in kind, x, y, z, scale,
rotation and order, including every reed and lily in those zones. Zones with a contents file but
no block file were skipped. Style 3 has no plant list; its snow items were checked on their own:
all 66 subtype 24 items sit on their zone's stream with the gate, D mod 300 = 0 and the rotation
in that order.

A "start" is the first record whose values can be located on the zone's stream. The checker
begins there, so a few records before it in each zone (3448 of 119 076 in all) were not
compared. "No start found" means this search failed, not that a rule failed. The one stopped
town zone (32804, 32802) ends at kind 0 at (140, 73), z 142. I did not find the cause. It may be a
town feature the checker does not model, as the street-light cell test was guessed there.

**Test vector.** Seed 41, zone (32644, 32820), with D[0] the first draw after the zone's `srand`:
D[2283456] = 17688 is the gate of the first plant, kind 12 at local (0, 3), z 43. D[2283457] = 9127
is its rotation (100.2753°) and D[2283458] = 2167 its scale (0.1013227). The state after the
last column of that zone is 1259950482.

The temperate rows for kinds 12, 4, 1, 0, all hot rows but desert flowers, and underwater and fire
are confirmed this way. Desert flowers (27, 28) and temperate kinds 2 and 3 do not occur in the
recordings, so those rest on the listing alone.

## 10. Open points

* **P12-1** Lily count in style 1 (§6): needs a look at our output, not the listing.
* **P12-2** Temperate kinds 2, 3 and desert flowers rest on the listing only. A recording of such
  ground would settle them.
* **P12-3** The stopped town zone (§9).
