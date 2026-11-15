Status: FINAL / STABLE / HELD

# Alpha items: the random equipment generators

Behavioural specification of the four routines in Cube World **Alpha** `Cube.exe`
(SHA-256 `84a7a132…30bf`, image base 0x400000) that make random equipment. All addresses in
this document are **RVAs in that `Cube.exe` image**. The routines are the **loot generator**
RVA 0x1F7A60, the **recipe generator** RVA 0x1F6D50, the **armour generator** RVA 0x1F51E0 and
the **weapon generator** RVA 0x1F8AD0. Written from the machine code of all four routines;
the routines are described by behaviour, constants and draw order only.

Companion pointers (not restated here): [random](alpha-worldgen-random.md) (the draw source),
[structures](alpha-worldgen-structures.md) §1.10.2 and §1.10.6 (the dungeon callers),
[creatures](alpha-creatures-behaviour.md) §10 (item record fields and stat formulas) and §12.3
(mission rewards).

**Scope.** All four routines are specified to the draw: every draw, every branch, every value
written to the item record, and the draw count of every path. Besides the draw source they
call only `memset`, `operator delete`, the stack-cookie check RVA 0x28D788, the vector append
RVA 0x0C7830 (which reaches the vector growth routine RVA 0x0C6940 and `operator new`) and the
record copiers RVA 0x00EE70 / RVA 0x02C5E0; none of these makes a draw. Two meanings are
*inferred* (item type 2 = recipe, and the class numbering, §6); neither affects draws or bytes
written.

---

## 0. Conventions

* A **draw** is one call of the C runtime `rand` imported from MSVCR110 (import slot RVA
  0x2FC468), giving 0..32767 from the calling thread's stream. "d mod n" is the remainder of
  the signed division; draws are non-negative, so for positive n it is the ordinary remainder.
  "odd" / "even" test the low bit of the draw.
  There are **no floating-point operations** in any of the four routines.
* Draws are numbered **D1, D2, …** from the routine's entry.
* **Item record**: a 0x118-byte record (field names as in creatures spec §10.1). Fields used
  here:

  | Off | Width | Name here |
  |---|---|---|
  | +0x00 | 1 byte | **type** |
  | +0x01 | 1 byte | **subtype** |
  | +0x04 | 4 bytes | **seed** |
  | +0x08 | 4 bytes | **product** (only used by recipes, §3) |
  | +0x0c | 1 byte | **rarity** |
  | +0x0d | 1 byte | **material** |
  | +0x0e | 1 byte | (unnamed; always 0 here) |
  | +0x10 | 2 bytes | **level** |
  | +0x14 | 32 × 8 bytes | spirits (always all zero here) |
  | +0x114 | 4 bytes | spirit count (always 0 here) |

  The record copiers transfer exactly the fields above (each spirit entry as all eight of its
  bytes). Bytes +0x02, +0x03, +0x0f, +0x12 and +0x13 are never transferred: the destination
  keeps whatever those bytes held before.
* **Candidate list**: each generator builds a local growable list of complete item records
  (stride 0x118). "**push** (type, subtype, material, …)" means: set the named fields of a
  working record, then append a copy of the whole working record. Fields not named keep the
  value they had for the previous push. At the end one candidate is picked by a final draw:

  **pick**: draw d; n = number of candidates; index = d mod n computed as an **unsigned**
  division; the result is a copy of candidate[index] into the output record. With n = 0 the
  division is by zero and raises the processor's integer-divide fault (§5).

* **Rarity roll** ρ(m), written out inline 25 times in the loot generator, and the same
  arithmetic as the rarity routine RVA 0x1F8530 when that routine's force flag is clear and its
  argument is m − 1: draw, q = d mod m; draw, add 1 if that draw mod 100 is 0; draw, add 1 if
  that draw mod 1000 is 0; draw, add 1 if that draw mod 10000 is 0; result min(q, 4).
  **Always 4 draws.** With m = 0 the first remainder divides by zero (§5).
  (When the force flag of RVA 0x1F8530 is set, the same four draws are made but the result is
  min(argument + 1, 4).)

---

## 1. The loot generator (RVA 0x1F7A60)

### 1.1 Call

loot(out, **L**, **r**) → out. A member routine: the object in ECX is used only to pass it on
as ECX to the recipe generator. Three stack arguments (it returns with a 12-byte stack
release): the output record pointer, the **level** L (only its low 16 bits are stored) and the
**rarity cap** r. It returns the output pointer.

The immediately preceding push of −1 at the dungeon chest and boss call sites belongs to the
**following** call (RVA 0x04B560 after the chest, RVA 0x06EBE0 after the boss), interleaved by
the compiler; the loot generator itself takes only (out, L, r). Callers inside the dungeon
builder RVA 0x1CC510, where lv is the 32-bit value at +0x80 and sl the byte at +0x84 of that
routine's first stack argument (the zone; names as in the structures spec):

| Call site | L | r |
|---|---|---|
| Dungeon chest (structures §1.10.6), RVA 0x1D3154 | lv | sl + 1 |
| Dungeon table loot, case 4 of a draw mod 6, RVA 0x1D3423 | lv | sl |
| Dungeon boss, RVA 0x1D3C16 | the creature's +0x34, set to lv and then raised to at least 1 | RVA 0x1F8530 with argument sl and force set, i.e. min(sl + 1, 4) |

So r ≥ 0 for every caller in the image.

### 1.2 Working record

Before any draw the working record is: seed = 0, product = 0, +0x0e = 0, level = L, spirits
zero, spirit count 0. Type, subtype, rarity and material are written before the first push.
**The seed is never written again**, so every candidate of this generator has seed 0 except
the recipe (§3). Let **m** = r + 1.

### 1.3 Draws, in order

1. **D1–D4**: four draws, results discarded.
2. Material = 1, type = 3 (weapon). **D5**: subtype = D5 mod 3 (0, 1 or 2). Rarity = ρ(m)
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

   The subtype becomes 0 at candidate 6, where type and subtype are written together as one
   16-bit store. Candidate k ≥ 1 takes its rarity from D(11 + 4k) … D(14 + 4k).
5. **Recipe**: the working record itself is passed as the output of recipe(L, r) of §3 (note:
   r, **not** m), which overwrites it, starting at **D111**; this costs 17 or 25 draws. **Push**
   it as candidate 25.
6. **Pick** (§0) among n = **26** candidates with the next draw (D128 or D136).

### 1.4 Result and draw count

The output is a copy of the picked candidate. Its fields:

* candidates 0–24: type, subtype and material from the table; seed 0; product 0; rarity its
  own ρ(m) roll; level L.
* candidate 25: the recipe record of §3.

**Draw count: 128** (recipe wraps the weapon generator) or **136** (recipe wraps the armour
generator); the routine body contains 111 draw sites. The count never depends on L, and on r
only through the division-by-zero case.

Each candidate is equally likely up to the remainder bias: 32768 = 26 · 1260 + 8, so indices
0–7 have probability 1261/32768 and 8–25 have 1260/32768. Only the picked candidate's rarity
matters, but all 25 rolls are always drawn.

---

## 2. The building blocks: armour (RVA 0x1F51E0) and weapon (RVA 0x1F8AD0)

Both take (out, **L**, **R**, **c**) and return out, with four stack arguments (16-byte stack
release); neither reads ECX. L is stored as the 16-bit level, **R** is stored as the rarity
byte **unchanged** (no roll), and **c** selects a class. A class block runs when c equals its
number **or c is negative**. Each push takes a fresh seed draw: "push [d]" means draw d,
seed = d mod 100, then push.

Initial working record: subtype 0, product 0, +0x0e 0, spirits zero, spirit count 0, level
L, rarity R. The weapon generator additionally sets seed 0, type 3 and material 1 before any
draw. The armour generator does not pre-set seed, type or material; every push writes all
three first, so no candidate shows the difference.

### 2.1 Armour generator (RVA 0x1F51E0)

1. Class **1** block: material = 1; types 7, 4, 5, 6 in that order, each push [d].
   4 draws, 4 candidates.
2. Class **3** block: material = 25; types 7, 7, 4, 5, 6, each push [d]. 5 draws,
   5 candidates.
3. Class **2** block: material = 26; the same five types. 5 draws, 5 candidates.
4. Class **4** block: material = 27; the same five types. 5 draws, 5 candidates.
5. Always: draw; material = 12 if odd, 11 if even; type = 8; push [d].
6. Always: draw; material = 12 if odd, 11 if even; type = 9; push [d].
7. **Pick** with the next draw.

| c | Candidates | Draws |
|---|---|---|
| < 0 | 21 | **24** |
| 1 | 6 | 9 |
| 2, 3, 4 | 7 | 10 |
| 0 or ≥ 5 | 2 (types 8 and 9) | 5 |

All candidates have subtype 0.

### 2.2 Weapon generator (RVA 0x1F8AD0)

Type is 3 for every candidate; material starts at 1.

1. Class **1** block (material 1):
   * draw: subtype = d mod 3; push [d];
   * material = 1 again; draw: subtype = (d mod 3) + 15; push [d];
   * subtype = 13; push [d].
   5 draws, 3 candidates.
2. Class **4** block (material still 1): subtypes 3, 5, 4 in that order, each push [d].
   3 draws, 3 candidates.
3. Always (no draw): material = 2.
4. Class **2** block: subtype 6, push [d]; **one draw whose result is never used**; subtype 8,
   push [d]. 3 draws, 2 candidates.
5. Class **3** block (material 2): subtype 10, push [d]; subtype 11, push [d]; subtype 12:
   draw, material = **11 if odd, 12 if even**, then push [d]. 4 draws, 3 candidates.
   (The parity is the reverse of the armour generator's steps 5–6.)
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

## 3. The recipe generator (RVA 0x1F6D50)

recipe(out, **L**, **R**) → out, three stack arguments (12-byte stack release); the object in
ECX is passed through as ECX to the building block.

1. The output record is cleared with no draw: type = subtype = 0, seed = 0, product = 0,
   rarity = material = 0, +0x0e = 0, **level = 1**, spirit count 0, spirits zeroed.
2. **D1**: if even, tmp = armour(L, R, −1) (§2.1, 24 draws); if odd, tmp = weapon(L, R, −1)
   (§2.2, 16 draws).
3. tmp is copied over the output by the record copier (so level = L, rarity = R, and type,
   subtype, material and seed from the picked candidate).
4. product (+0x08, 4 bytes) = the output's type byte, zero-extended (3 from the weapon
   generator, 4–9 from the armour generator); then type = **2**.

**Draw count: 17** (odd D1) or **25** (even D1). Subtype, material, seed (0..99), level L and
rarity R are those of the wrapped item; the recipe's rarity is the caller's R as given, never
rolled.

Callers: the loot generator (§1.3 step 5, with its r); the dungeon table loot, case 5 of the
same draw mod 6 (RVA 0x1D343F, L = lv, R = sl); and RVA 0x19BC7A inside routine RVA 0x19B180,
which passes L = the 32-bit value at +0x190 of the object it handles and R = the result of the
rarity routine RVA 0x1F8530 with force clear and argument (the byte at +0x1A8 of that object)
+ 1. That caller is otherwise outside this document.

---

## 4. Value tables

Type and subtype names follow creatures spec §3.1 / §10.1; names in *italics* are *inferred*
labels and are not used by any arithmetic here.

| Type | Meaning | Subtypes produced |
|---|---|---|
| 2 | *recipe* (product type at +0x08) | of the product |
| 3 | weapon | 0, 1, 2 (*sword, axe, mace*), 3 (*dagger*), 4 (*fist*), 5 (*longsword*), 6 (*bow*), 7 (*crossbow*), 8 (*boomerang*), 10 (*staff*), 11 (*wand*), 12 (*bracelet*), 13 (*shield*), 15, 16, 17 (*greatsword, greataxe, greatmace*) |
| 4, 5, 6, 7 | *chest, gloves, boots, shoulders* | 0 |
| 8, 9 | *amulet, ring* | 0 |

| Material | Where | Note |
|---|---|---|
| 1 | class-1 armour, class-1 and class-4 weapons | *iron* |
| 2 | class-2 and class-3 weapons | *wood* |
| 11, 12 | types 8 and 9, subtype-12 weapons | see creatures §10.3 |
| 25, 26, 27 | armour of classes 3, 2, 4 | see creatures §10.1 |

The loot generator's 25 fixed candidates contain no weapon subtypes 3, 4, 5, 8, 12, 15, 16 or
17 and no types 8 or 9 (those reach its result only through the recipe). It does contain
weapon subtype 7, which the weapon generator never pushes.

---

## 5. Edge cases

* **loot with r = −1** (m = 0): the first rarity roll divides by zero. No caller in the image
  does this (§1.1). For r < −1 the signed remainder by a negative m lies in 0 … |m| − 1; not
  reachable from any caller.
* **weapon with c = 0 or c ≥ 5**: no candidate is pushed, and the pick divides by zero. The
  recipe generator always passes c = −1.
* The pick uses an **unsigned** division, so with n > 0 it can never index out of range.
* Large L is truncated to 16 bits when stored; large R and rarity results to 8 bits.

---

## 6. Inferred meanings

* **Class numbers** 1 warrior, 2 ranger, 3 mage, 4 rogue: inferred from the weapon blocks
  (class 1 subtypes 0–2, 13, 15–17; class 2 subtypes 6 and 8; class 3 subtypes 10–12; class 4
  subtypes 3–5) together with the inferred subtype names of §4 and the material-set naming of
  creatures §10.1. Draws do not depend on the names.
* **Type 2 = recipe**, with the product's type at +0x08: inferred from the wrapper's shape (a
  full item record re-typed as 2 with its original type kept at +0x08).

---

## 7. Worked examples

Draw stream after seeding the C runtime stream with 123 (state ← state · 214013 + 2531011 mod
2³², draw = bits 16–30 of the new state, initial state 123):

440, 19053, 23075, 13104, 32363, 3265, 30749, 32678, 9760, 28064, 20429, 750, 4480, 3863,
30429, 18586, 30758, 31595, 31567, 24630, 16059, 2734, 703, 3966, …

### 7.1 Weapon generator, weapon(L = 5, R = 1, c = −1), seed 123

| Draw | Value | Effect |
|---|---|---|
| D1 | 440 | subtype 440 mod 3 = 2 |
| D2 | 19053 | seed 53 → candidate 0 (3, 2, material 1) |
| D3 | 23075 | subtype 2 + 15 = 17 |
| D4 | 13104 | seed 4 → candidate 1 (3, 17) |
| D5 | 32363 | seed 63 → candidate 2 (3, 13) |
| D6–D8 | 3265, 30749, 32678 | seeds 65, 49, 78 → candidates 3–5 (3, 3), (3, 5), (3, 4) |
| D9 | 9760 | seed 60 → candidate 6 (3, 6, material 2) |
| D10 | 28064 | unused |
| D11 | 20429 | seed 29 → candidate 7 (3, 8, material 2) |
| D12, D13 | 750, 4480 | seeds 50, 80 → candidates 8, 9 (3, 10), (3, 11) |
| D14 | 3863 | odd → material 11 |
| D15 | 30429 | seed 29 → candidate 10 (3, 12, material 11) |
| D16 | 18586 | pick 18586 mod 11 = 7 |

Result: type 3, subtype 8, material 2, seed 29, rarity 1, level 5. **16 draws.**

### 7.2 Armour generator, armour(L = 5, R = 1, c = −1), seed 123

D1–D4 → seeds 40, 53, 75, 4 (types 7, 4, 5, 6, material 1); D5–D9 → material 25, seeds 63,
65, 49, 78, 60; D10–D14 → material 26, seeds 64, 29, 50, 80, 63; D15–D19 → material 27, seeds
29, 86, 58, 95, 67; D20 = 24630 even → material 11, D21 → seed 59 (type 8); D22 = 2734 even →
material 11, D23 → seed 3 (type 9); D24 = 3966, pick 3966 mod 21 = **18** = the material-27
type-6 candidate. Result: type 6, subtype 0, material 27, seed 67 (from D19 = 31567), rarity
1, level 5. **24 draws.**

### 7.3 Recipe generator, recipe(L = 5, R = 0), seed 123

D1 = 440 even → armour(5, 0, −1) on D2…D25. Result: type 2, product 7, subtype 0,
material 27, seed 86, rarity 0, level 5. **25 draws.**

### 7.4 Loot generator, loot(L = 5, r = 1), seed 123

* D1–D4 unused; D5 = 32363 → subtype 32363 mod 3 = 2; D6 = 3265 mod 2 = 1, D7–D9 add nothing
  (30749 mod 100 = 49, 32678 mod 1000 = 678, 9760 mod 10000 = 9760) → candidate 0 = (3, 2,
  material 1, rarity 1).
* Candidate rarities (0–24): 1 1 1 1 0 0 1 1 0 1 | 0 1 0 1 1 | 1 0 0 1 0 | 1 1 0 0 0.
* D111 = 16022 even → recipe wraps the armour generator on D112–D135: type 2, product 7,
  material 27, seed 71, rarity 1, level 5.
* D136 = 25363, pick 25363 mod 26 = **13** → candidate 13 = type 5, material 25.

Result: type 5, subtype 0, material 25, seed 0, product 0, rarity 1, level 5.
**136 draws.** With (L, r) = (1, 0) the same stream gives the same candidate (type 5,
material 25) with rarity 0 and level 1; with (7, 3), rarity 3 and level 7 (all 136 draws).

### 7.5 Other seeds, loot(5, 1)

| Seed | D5 / D111 | Final draw | Result | Draws |
|---|---|---|---|---|
| 1 | D5 = 19169 → subtype 2; D111 odd | 15574 mod 26 = 0 | type 3, subtype 2, material 1, seed 0, rarity 0 | 128 |
| 37 | D111 = 6119 odd (weapon recipe) | 9255 mod 26 = 25 | recipe: type 2, product 3, subtype 4, material 1, seed 55, rarity 1 | 128 |

---

## 8. Open questions

1. **Unused draws.** The loot generator makes D1–D4 and D10–D14 and the weapon generator one
   draw between subtypes 6 and 8 without storing their results. Why they exist (for example
   removed candidates) is not established; the draws themselves are certain.
2. **Seed 0 for fixed loot candidates.** The loot generator never writes the seed, so its 25
   fixed candidates all have seed 0, while recipe products carry seeds 0..99. How the stat
   formulas of creatures §10.2 then behave for these items is not established here.
3. **Type 2 and +0x08** (§6): not established from these routines alone that type 2 is the
   recipe / formula item and that +0x08 is read as its product type.
4. **Class numbering** (§6): not established from these routines alone.
5. **Other callers**: RVA 0x19B180 also calls the weapon and armour generators directly with
   c = −1 (RVA 0x19B4A9 / RVA 0x19B4C4); the routines RVA 0x07B600, RVA 0x195A60, RVA 0x1C9B30
   and RVA 0x1D9460 call the weapon and armour generators with their own L, R and c. Those call
   sites, including any claim that mission rewards pass the player's class (creatures §12.3),
   belong to the specifications of their systems and are not confirmed here.
6. **Divide fault outcome.** What happens after the integer-divide fault of §5 (process
   termination or an outer handler) is not established.
7. **Inferred labels.** The italic names of §4 are not derived from these routines.

---

## Re-certification (2026-11-14)

Re-checked against the machine code of `Cube.exe` (SHA-256 confirmed as above) disassembled
with the MSVC disassembler: the four routines, the rarity routine RVA 0x1F8530, both record
copiers, the vector append and growth routines, the MSVCR110 import slots for `rand` and
`memset`, and every call site of the five routines in the image. No reconstruction was
consulted.

**Confirmed.** Record offsets and widths; candidate order, types, subtypes and materials of
all four generators; class-block conditions (equal or negative); parity rules in both
directions; the rarity roll's divisors 100 / 1000 / 10000, its cap 4 and its use of r + 1 in
the loot generator; the recipe argument r (not r + 1); the level-1 clear and full copy-over in
the recipe generator; product = zero-extended type then type 2; draw counts of every path
(loot 128 / 136 with 111 draw sites; recipe 17 / 25; armour 24 / 9 / 10 / 5; weapon 16 / 6 /
4 / 4 / 5 / 1); unsigned pick over stride 0x118; stack-argument counts; the dungeon call sites
and their L / r (lv = +0x80 dword, sl = +0x84 byte of the zone argument); the case 4 / case 5
mapping of the table-loot jump table; the remainder bias; no floating point; and all worked
example values except the one noted below, recomputed independently.

**Corrected.**
1. Addresses: all were image VAs (e.g. 0x5f7a60) without an image-relative statement; now
   RVAs in `Cube.exe` (e.g. RVA 0x1F7A60).
2. §7.2 material-27 seeds: old "29, 86, 95, 67, 30"; new "29, 86, 58, 95, 67". Selected
   candidate and result were already right; no effect on draw order.
3. §2 initial working record: old said seed was pre-cleared in both building blocks; the
   armour generator does not pre-set seed (or type / material). No observable effect on
   candidates or draws.
4. Callee list: added the stack-cookie check and the indirect `operator new`; neither draws.
5. Divide by zero: old called it "a crash"; now stated only as the integer-divide fault,
   outcome unresolved.
6. RVA 0x19B180 caller: clarified that R is the rarity routine applied to (byte +0x1A8) + 1
   with force clear and L is the dword at +0x190; added its direct c = −1 building-block calls.
7. Following-call names "list insert" / "inventory append" replaced by their RVAs.

None of these changes reverses a behavioural claim or alters item generation or draw order.

**Removed.** Commentary about the decompiler's output; advice on padding bytes and on making
unused draws addressed to a reimplementer; the note about a task brief's differing stream and
the cross-document vector comparison; the suggestions to check loot stats in the running game
and to confirm meanings in the UI or crafting code (kept only as unresolved questions); the
"closes open question 2 of the structures spec" project note; the class-name-based material
wording (now by class number).

**Unresolved.** Open questions 1–7 of §8: purpose of the unused draws; stat behaviour of
seed-0 loot; type 2 / +0x08 meaning; class numbering; other callers' arguments and the mission
reward class claim; divide-fault outcome; italic labels of §4. Companion-spec section pointers
were not opened and are not re-certified here.
