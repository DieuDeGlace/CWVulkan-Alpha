# Alpha items: the random equipment generators

Behavioural specification of the four routines in Cube World **Alpha** `Cube.exe`
(SHA-256 `84a7a132…30bf`) that make random equipment: the **loot generator** 0x5f7a60, the
**recipe generator** 0x5f6d50, the **armour generator** 0x5f51e0 and the **weapon generator**
0x5f8ad0. Written from the assembly of all four routines (the decompiler output was used only
as a reading aid; it mis-places the argument slots and loses the final pick, see §1.1). No
code was copied; the routines are described by behaviour, constants and draw order.

Read first: [random](alpha-worldgen-random.md) (rand, draw idioms),
[structures](alpha-worldgen-structures.md) §1.10.2 items 6–8 and §1.10.6 (the dungeon callers),
[creatures](alpha-creatures-behaviour.md) §10 (item record fields and stat formulas) and §12.3
(mission rewards).

**Status.** All four routines are specified **to the draw**: every `rand()` call, every
branch, every value written to the item record, and the draw count of every path. They call
only the C runtime `rand`, `memset`, `operator delete`, the vector growth routine 0x4c6940,
the vector append 0x4c7830 and the record copiers 0x40ee70 / 0x42c5e0, none of which draw.
This closes open question 2 of the structures spec: with this document a dungeon's draw count
is exact through its chests, table loot and boss. Two meanings are *inferred* (item type 2 =
recipe, and the class numbering, §6); neither affects draws or bytes written.

---

## 0. Conventions

* As in the random spec §0 and §1.2: a **draw** is one MSVC `rand()` call (0..32767) from the
  calling thread's stream; "d % n" is the C remainder (draws are non-negative, so plain
  modulo); "odd(d)" / "even(d)" test the low bit (the code's sign fix-ups never trigger).
  There are **no floating-point operations** in any of the four routines.
* Draws are numbered **D1, D2, …** from the routine's entry.
* **Item record**: the 0x118-byte record of creatures spec §10.1. Fields used here:

  | Off | Type | Name here |
  |---|---|---|
  | +0x00 | u8 | **type** |
  | +0x01 | u8 | **subtype** |
  | +0x04 | u32 | **seed** |
  | +0x08 | u32 | **product** (only used by recipes, §3) |
  | +0x0c | u8 | **rarity** |
  | +0x0d | u8 | **material** |
  | +0x0e | u8 | (unnamed; always 0 here) |
  | +0x10 | i16 | **level** |
  | +0x14 | 32 × 8 bytes | spirits (always all zero here) |
  | +0x114 | i32 | spirit count (always 0 here) |

  Bytes +0x02, +0x03, +0x0f, +0x12, +0x13 are padding and are not copied by the record
  copiers; a reimplementation should leave them 0.
* **Candidate list**: each generator builds a local list of complete item records (a
  `std::vector` of 0x118-byte records). "**push** (type, subtype, material, …)" means: set the
  named fields of a working record, then append a copy of the whole working record. Fields
  not named keep the value they had for the previous push. At the end one candidate is
  picked by a final draw:

  **pick**: draw d; index = d mod n (unsigned), n = number of candidates; the result is a
  copy of candidate[index]. With n = 0 this is an integer division by zero (a crash in the
  original; §5).

* **Rarity roll** ρ(m) (inlined 25 times in 0x5f7a60; the same arithmetic as 0x5f8530 with
  force = false and m = r + 1): draw, q = d % m; draw, +1 if d % 100 = 0; draw, +1 if
  d % 1000 = 0; draw, +1 if d % 10000 = 0; result min(q, 4). **Always 4 draws.** With
  m = 0 the first remainder divides by zero (§5).

---

## 1. The loot generator (0x5f7a60)

### 1.1 Call

loot(out, **L**, **r**) → out. A member routine (ECX = the zone/generator object, used only to
pass on to 0x5f6d50); stack arguments, in push order reversed: the output record pointer,
the **level** L (only its low 16 bits are stored) and the **rarity cap** r; `RET 0x0C`, so
there are exactly three stack arguments. It returns the output pointer.

**Correction to the structures spec.** §1.10.2 item 8 and §1.10.6 write the chest and boss
calls as "0x5f7a60(lv, sl + 1, −1)". The `PUSH −1` that precedes those calls is an argument of
the **next** call (the list insert 0x44b560 for the chest, the inventory append for the boss),
interleaved by the compiler; 0x5f7a60 itself takes only (L, r). The callers are:

| Caller | L | r |
|---|---|---|
| Dungeon chest (structures §1.10.6 step 7, 0x5d3154) | zone level lv | sl + 1 |
| Dungeon table loot, d % 6 = 4 (0x5d3423) | lv | sl |
| Dungeon boss (0x5d3c16) | creature +0x34 = max(lv, 1) | rarity(sl, force) of 0x5f8530 = min(sl + 1, 4) |

(sl = the zone's sub-level byte.) So r ≥ 0 for every known caller.

### 1.2 Working record

Before any draw the working record is: subtype unset (it is written before the first push),
seed = 0, product = 0, +0x0e = 0, level = L, spirits zero, spirit count 0. **The seed is
never written again**, so every candidate of this generator has seed 0 except the recipe
(§3). Let **m** = r + 1.

### 1.3 Draws, in order

1. **D1–D4**: four draws, results discarded.
2. Type = 3 (weapon), material = 1. **D5**: subtype = D5 % 3 (0, 1 or 2). Rarity = ρ(m)
   (D6–D9). **Push** candidate 0.
3. **D10–D14**: five draws, results discarded.
4. Then, in this order, each with its own rarity ρ(m) (4 draws) followed by a push:

   | # | type | subtype | material | Draws |
   |---|---|---|---|---|
   | 1 | 3 | 13 | 1 | D15–D18 |
   | 2 | 3 | 6 | 2 | D19–D22 |
   | 3 | 3 | 7 | 2 | D23–D26 |
   | 4 | 3 | 10 | 2 | D27–D30 |
   | 5 | 3 | 11 | 2 | D31–D34 |
   | 6 | 7 | 0 | 1 | D35–D38 |
   | 7 | 4 | 0 | 1 | D39–D42 |
   | 8 | 5 | 0 | 1 | D43–D46 |
   | 9 | 6 | 0 | 1 | D47–D50 |
   | 10, 11 | 7, 7 | 0 | 25 | D51–D58 |
   | 12, 13, 14 | 4, 5, 6 | 0 | 25 | D59–D70 |
   | 15, 16 | 7, 7 | 0 | 26 | D71–D78 |
   | 17, 18, 19 | 4, 5, 6 | 0 | 26 | D79–D90 |
   | 20, 21 | 7, 7 | 0 | 27 | D91–D98 |
   | 22, 23, 24 | 4, 5, 6 | 0 | 27 | D99–D110 |

   (The subtype becomes 0 at candidate 6, where type and subtype are written together.)
   In general candidate k ≥ 1 takes its rarity from D(11 + 4k) … D(14 + 4k).
5. **Recipe**: the working record is overwritten by recipe(L, r) of §3 (note: r, **not** m),
   starting at **D111**; it costs 17 or 25 draws. **Push** it as candidate 25.
6. **Pick** (§0) among n = **26** candidates with the next draw (D128 or D136).

### 1.4 Result and draw count

The output is a copy of the picked candidate. Its fields:

* candidates 0–24: type, subtype and material from the table; seed 0; product 0; rarity its
  own ρ(m) roll; level L.
* candidate 25: the recipe record of §3.

**Draw count: 128** (recipe from the weapon generator) or **136** (recipe from the armour
generator); the call has 111 draw sites. It never depends on L, and on r only through the
division-by-zero case.

Each candidate is equally likely up to the modulo bias: 32768 = 26 · 1260 + 8, so indices
0–7 have probability 1261/32768 and 8–25 have 1260/32768. Only the picked candidate's
rarity matters, but all 25 rolls are always drawn.

---

## 2. The building blocks: armour (0x5f51e0) and weapon (0x5f8ad0)

Both have the form gen(out, **L**, **R**, **c**) → out, `RET 0x10` (four stack arguments):
L is stored as the 16-bit level, **R** is stored as the rarity byte **unchanged** (no roll),
and **c** selects a class; c < 0 means "all classes". The working record starts with
subtype 0 (armour) or type 3 / material 1 (weapon), seed / product / +0x0e = 0, spirits
zero. A class block runs when c equals its number **or c < 0**. Each push takes a fresh
seed draw: "push(…) [d]" means draw d, seed = d % 100, then push.

### 2.1 Armour generator (0x5f51e0)

1. Class **1** block: material = 1; types 7, 4, 5, 6 in that order, each push [d].
   4 draws, 4 candidates.
2. Class **3** block: material = 25; types 7, 7, 4, 5, 6, each push [d]. 5 draws, 5 cand.
3. Class **2** block: material = 26; the same five types. 5 draws, 5 cand.
4. Class **4** block: material = 27; the same five types. 5 draws, 5 cand.
5. Always: draw; material = 12 if odd, 11 if even; type = 8; push [d].
6. Always: draw; material = 12 if odd, 11 if even; type = 9; push [d].
7. **Pick** with the next draw.

| c | Candidates | Draws |
|---|---|---|
| < 0 | 21 | **24** |
| 1 | 6 | 9 |
| 2, 3, 4 | 7 | 10 |
| 0 or ≥ 5 | 2 (amulet, ring) | 5 |

All candidates have subtype 0.

### 2.2 Weapon generator (0x5f8ad0)

Type is 3 for every candidate; material starts at 1.

1. Class **1** block (material 1):
   * draw: subtype = d % 3; push [d];
   * draw: subtype = d % 3 + 15; push [d];
   * subtype = 13; push [d].
   5 draws, 3 candidates.
2. Class **4** block (material still 1): subtypes 3, 5, 4 in that order, each push [d].
   3 draws, 3 candidates.
3. Always (no draw): material = 2.
4. Class **2** block: subtype 6, push [d]; **one draw discarded**; subtype 8, push [d].
   3 draws, 2 candidates.
5. Class **3** block (material 2): subtype 10, push [d]; subtype 11, push [d]; subtype 12:
   draw, material = **11 if odd, 12 if even**, then push [d]. 4 draws, 3 candidates.
   (Note the parity is the reverse of the armour generator's step 5–6.)
6. **Pick** with the next draw.

| c | Candidates | Draws |
|---|---|---|
| < 0 | 11 | **16** |
| 1 | 3 | 6 |
| 4 | 3 | 4 |
| 2 | 2 | 4 |
| 3 | 3 | 5 |
| 0 or ≥ 5 | 0 | 1, then division by zero (§5) |

---

## 3. The recipe generator (0x5f6d50)

recipe(out, **L**, **R**) → out, `RET 0x0C`, ECX passed through to the building blocks.

1. The output record is cleared: type = subtype = 0, seed = 0, product = 0, rarity =
   material = 0, +0x0e = 0, **level = 1**, spirit count 0, spirits zeroed (no draws).
2. **D1**: if even, tmp = armour(L, R, −1) (§2.1, 24 draws); if odd, tmp = weapon(L, R, −1)
   (§2.2, 16 draws).
3. The whole of tmp is copied over the output (so level = L, rarity = R, and type, subtype,
   material, seed from the picked candidate).
4. product (+0x08, u32) = the output's type byte (3–9); then type = **2**.

**Draw count: 17** (odd D1) or **25** (even D1). Subtype, material, seed (0..99), level L and
rarity R are those of the wrapped item; the recipe's rarity is the caller's R as given,
never rolled.

Callers: the loot generator (§1.3 step 5, with its r), the dungeon table loot d % 6 = 5
(0x5d343f, R = sl), and 0x59b180, which calls recipe(+0x190, rarity(+0x1a8 + 1, force 0)) of
the object it handles (that caller is outside this document).

---

## 4. Value tables

Type and subtype names follow creatures spec §3.1 / §10.1; names in *italics* are
*inferred* from the Steam-era item tables and are not used by any formula here.

| Type | Meaning | Subtypes produced |
|---|---|---|
| 2 | *recipe* (product type at +0x08) | of the product |
| 3 | weapon | 0, 1, 2 (*sword, axe, mace*), 3 (*dagger*), 4 (*fist*), 5 (*longsword*), 6 (*bow*), 7 (*crossbow*), 8 (*boomerang*), 10 (*staff*), 11 (*wand*), 12 (*bracelet*), 13 (*shield*), 15, 16, 17 (*greatsword, greataxe, greatmace*) |
| 4, 5, 6, 7 | chest, gloves, boots, shoulders | 0 |
| 8, 9 | amulet, ring | 0 |

| Material | Where | Note |
|---|---|---|
| 1 | warrior armour, warrior and rogue weapons | *iron* |
| 2 | ranger and mage weapons | *wood* |
| 11, 12 | amulets, rings, subtype-12 weapons | crit / speed bonus (creatures §10.3) |
| 25, 26, 27 | armour of classes 3, 2, 4 | the three cloth/leather sets of creatures §10.1 |

Which of 0x5f7a60's 25 fixed candidates exist is a strict subset of the building blocks: it
has no daggers, fists, longswords, boomerangs, bracelets, greatweapons, amulets or rings
(those appear in its loot only through the recipe), and it has crossbows (subtype 7), which
the weapon generator never pushes (its draw is consumed and discarded, §2.2 step 4).

---

## 5. Edge cases

* **loot with r = −1** (m = 0): the first rarity roll divides by zero. No known caller does
  this (§1.1). For r < −1 the C remainder by a negative m gives 0 … |m| − 1 (sign of the
  dividend); not reachable.
* **weapon with c = 0 or c ≥ 5**: no candidate is pushed, and the pick divides by zero. The
  recipe generator always passes c = −1; mission rewards pass the player's class
  (creatures §12.3).
* The pick uses an **unsigned** division, so it can never index out of range.
* Large L is truncated to 16 bits when stored; large R / rarity results to 8 bits.

---

## 6. Inferred meanings

* **Class numbers** 1 warrior, 2 ranger, 3 mage, 4 rogue: inferred from the weapon blocks
  (class 1 swords/axes/maces/shields, class 2 bows/boomerangs, class 3 staffs/wands, class 4
  daggers/longswords/fists) and from creatures §10.1, which already calls 25, 26, 27 the
  mage, ranger and rogue sets. Draws do not depend on the names.
* **Type 2 = recipe**, with the product's type at +0x08: inferred from the wrapper's shape
  (a full item record re-typed as 2 and its original type kept). Confirm in the inventory
  UI or the crafting code.

---

## 7. Test vectors

Stream of srand(123), computed with state ← state · 214013 + 2531011 (mod 2³²), draw =
(state >> 16) & 0x7fff, initial state 123:

440, 19053, 23075, 13104, 32363, 3265, 30749, 32678, 9760, 28064, 20429, 750, 4480, 3863,
30429, 18586, 30758, 31595, 31567, 24630, 16059, 2734, 703, 3966, …

(The values from the sixth draw on differ from the "3992, 14217, 7353, …" quoted in the task
brief for this spec; the brief's continuation does not follow from the stated recurrence and
does not occur in the first 200 draws of seed 123. The three-and-four-draw vectors already in
the random, trees and structures specs agree with the list above.)

### 7.1 Weapon generator, weapon(L = 5, R = 1, c = −1), from srand(123)

| Draw | Value | Effect |
|---|---|---|
| D1 | 440 | subtype 440 % 3 = 2 |
| D2 | 19053 | seed 53 → cand 0 (3, 2, mat 1) |
| D3 | 23075 | subtype 2 + 15 = 17 |
| D4 | 13104 | seed 4 → cand 1 (3, 17) |
| D5 | 32363 | seed 63 → cand 2 (3, 13) |
| D6–D8 | 3265, 30749, 32678 | seeds 65, 49, 78 → cands 3–5 (3, 3), (3, 5), (3, 4) |
| D9 | 9760 | seed 60 → cand 6 (3, 6, mat 2) |
| D10 | 28064 | discarded |
| D11 | 20429 | seed 29 → cand 7 (3, 8, mat 2) |
| D12, D13 | 750, 4480 | seeds 50, 80 → cands 8, 9 (3, 10), (3, 11) |
| D14 | 3863 | odd → material 11 |
| D15 | 30429 | seed 29 → cand 10 (3, 12, mat 11) |
| D16 | 18586 | pick 18586 mod 11 = 7 |

Result: type 3, subtype 8, material 2, seed 29, rarity 1, level 5. **16 draws.**

### 7.2 Armour generator, armour(L = 5, R = 1, c = −1), from srand(123)

D1–D4 → seeds 40, 53, 75, 4 (types 7, 4, 5, 6, material 1); D5–D9 → material 25 seeds 63,
65, 49, 78, 60; D10–D14 → material 26 seeds 64, 29, 50, 80, 63; D15–D19 → material 27 seeds
29, 86, 95, 67, 30; D20 = 24630 even → material 11, D21 → seed 59 (amulet); D22 = 2734 even →
material 11, D23 → seed 3 (ring); D24 = 3966, pick 3966 mod 21 = **18** = the material-27
boots. Result: type 6, subtype 0, material 27, seed 67 (from D19 = 31567), rarity 1,
level 5. **24 draws.**

### 7.3 Recipe generator, recipe(L = 5, R = 0), from srand(123)

D1 = 440 even → armour(5, 0, −1) on D2…D25. Result: type 2, product 7 (shoulders),
subtype 0, material 27, seed 86, rarity 0, level 5. **25 draws.**

### 7.4 Loot generator, loot(L = 5, r = 1), from srand(123)

* D1–D4 discarded; D5 = 32363 → subtype 32363 % 3 = 2; D6 = 3265 % 2 = 1, D7–D9 no bonus
  (30749 % 100 = 49, 32678 % 1000 = 678, 9760 % 10000 = 9760) → candidate 0 = (3, 2, mat 1,
  rarity 1).
* Candidate rarities (0–24): 1 1 1 1 0 0 1 1 0 1 | 0 1 0 1 1 | 1 0 0 1 0 | 1 1 0 0 0.
* D111 = 16022 even → recipe from the armour generator on D112–D135: type 2, product 7,
  material 27, seed 71, rarity 1, level 5.
* D136 = 25363, pick 25363 mod 26 = **13** → candidate 13 = gloves (type 5), material 25.

Result: type 5, subtype 0, material 25, seed 0, product 0, rarity 1, level 5.
**136 draws.** With (L, r) = (1, 0) the same stream gives the same candidate (type 5, material
25) with rarity 0 and level 1; with (7, 3), rarity 3 and level 7 (all 136 draws).

### 7.5 Other seeds, loot(5, 1)

| srand | D5 / D111 | Final draw | Result | Draws |
|---|---|---|---|---|
| 1 | D5 = 19169 → subtype 2; D111 odd | 15574 mod 26 = 0 | weapon (3, 2), material 1, seed 0, rarity 0 | 128 |
| 37 | D111 = 6119 odd (weapon recipe) | 9255 mod 26 = 25 | recipe: type 2, product 3, subtype 4, material 1, seed 55, rarity 1 | 128 |

---

## 8. Open questions

1. **Discarded draws.** 0x5f7a60 discards D1–D4 and D10–D14, and 0x5f8ad0 discards one draw
   between the bow and the boomerang. They are plain `rand()` calls whose results are never
   stored (checked in the assembly). They are presumably left over from removed candidates;
   a reimplementation must still make them.
2. **Seed 0 for fixed loot candidates.** The loot generator never writes the seed, so the 25
   fixed candidates all have seed 0 (stats then use u₁ = u₂ = 0 in creatures §10.2), while
   recipe products carry seeds 0..99. Confirmed in the assembly; worth checking against a
   dungeon chest in the running game that loot stats really do not vary.
3. **Type 2 and +0x08** (§6): confirm that type 2 is the recipe / formula item and that
   +0x08 is read as its product type.
4. **Class numbering** (§6): confirm 1 warrior, 2 ranger, 3 mage, 4 rogue from the class
   selection screen or the creature class field.
5. **Callers outside the dungeon**: 0x59b180 (recipe with rarity(+0x1a8 + 1, 0)) and the
   class-specific callers of the building blocks (0x47b600, 0x595a60, 0x5c9b30, 0x5d9460)
   pass their own L, R and c; those call sites belong to the specs of their systems.
