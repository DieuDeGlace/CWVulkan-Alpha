Status: FINAL / STABLE / HELD

# Alpha loot: the draws of an NPC's death loot

Behavioural specification of the random parts of Cube World **Alpha**'s NPC death loot: the
**rarity roll** with the tier, the **level re-roll** with spread 0.05, the **tameable creature
list** behind pet-food drops, and the **exact order of every `rand()` draw** made by the loot
routine and by the drop routine it calls.

Written from the original Alpha images (`Server.exe`, `Cube.exe`), their disassembly listings
and decompile. Both images are 32-bit with image base 0x400000. Every routine is cited as an
**RVA** with the image named ("Server.exe RVA 0x…", "Cube.exe RVA 0x…").

| Routine | Server.exe RVA | Cube.exe RVA (twin) |
|---|---|---|
| NPC loot | 0xd2ae0 | 0x19b180 |
| Drop one item into the world | 0xd2810 | 0x19aeb0 |
| Rarity roll | 0x12bf40 | 0x1f8530 |
| Level re-roll | 0x14470 | 0xc7e80 |
| Pet food | 0x12b3f0 | 0x1f79e0 |
| Armour / weapon / recipe generator | 0x128bf0 / 0x12c4e0 / 0x12a760 | 0x1f51e0 / 0x1f8ad0 / 0x1f6d50 |
| Level curve *c* / its inverse | 0x7d60 / 0x11090 | 0x3ca60 / 0x462f0 |
| Coin base value *V* | 0x10f00 | 0x45f60 |
| Tameable list filled in | world constructor 0xc8570 (fill begins at 0xcd517) | world constructor 0x18eb00 (fill begins at 0x193aa7) |

**The two images make the same draws in the same order** in the loot and drop routines (the
call sequence of both listings matches site by site, including the equipment, random-item,
pet-food, type-drop, recipe, coin, leftovers and inventory paths), and their constants match.
The server's three generators have 24, 16 and 1 draw sites, as do the client's.

Related contracts (pointers only): `alpha-server-simulation.md` §6.2–6.4 (when loot runs),
`alpha-items-generator.md` (the armour, weapon and recipe generators), `alpha-world-items.md`
§1 and §3.2 (world-item record, the fall), `alpha-worldgen-random.md` (the `rand()`
generator), `alpha-item-names.md` (item and pet-food names).

---

## 0. Conventions

* A **draw** is one call of the C runtime `rand()` imported by both images: state ← state ·
  214013 + 2531011 (mod 2³²), result (state >> 16) & 0x7fff, so 0 … 32767. Seeded with 123
  the stream begins 440, 19053, 23075, 13104, 32363, 3265, 30749, 32678, 9760, 28064, 20429,
  750, 4480, 3863, 30429, 18586, 30758, …
* "*d* mod *n*" is the C remainder; draws are non-negative so it is the plain remainder. Where
  the routine tests "even/odd" it is the low bit of the draw.
* *r* = a fraction made from a draw. The loot's float arithmetic is **32-bit float** (SSE
  single precision), each operation rounded to float, in the order written here; the only
  exception is the two powers in *V* (§3.5). Conversions to integer **truncate toward zero**.
* The **victim** is the dying creature. Fields used: controller byte (+0x60; values 0, 1 and 3
  matter here — conventionally player, hostile NPC, merchant), creature type (+0x64, i32),
  appearance flags (+0x7e, u16; bits 0x08, 0x10, 0x200, 0x800 matter here — conventionally
  animal, animal, boss, no loot; the text below uses the bit values), position (+0x10, 3 × i64 in 1/65536 block),
  level *L* (+0x190, i32), tier *t* (+0x1a8, u8), equipment slot *k* at +0x300 + 0x118·*k*,
  inventory bags at +0x11dc.
* Item record fields (280 bytes): type +0x00, subtype +0x01, seed +0x04, +0x08 (used by
  recipes), rarity +0x0c, material +0x0d, level/amount +0x10 (i16), a 256-byte block at
  +0x14 and a 32-bit word at +0x114. A **fresh record** has every one of these zero except
  level +0x10 = 1.

---

## 1. The rarity roll ρ(*m*, force) (Server.exe RVA 0x12bf40, Cube.exe RVA 0x1f8530)

Inputs: an integer *m* (the callers pass the tier, or the tier + 1) and a force flag (every
loot caller passes false). **Always exactly 4 draws**, in this order:

1. *d*₁: *q* := *d*₁ mod (*m* + 1).
2. *d*₂: if *d*₂ mod 100 = 0, *q* := *q* + 1.
3. *d*₃: if *d*₃ mod 1000 = 0, *q* := *q* + 1.
4. *d*₄: if *d*₄ mod 10000 = 0, *q* := *q* + 1.
5. If force: *q* := *m* + 1 (the draws are still spent).
6. Result min(*q*, 4). There is no lower clamp; with *m* ≥ 0 the result is 0 … 4.

So the base is spread over 0 … *m* (up to the modulo bias of 32768 / (*m* + 1)), and three
independent bonuses add +1 with probabilities 328/32768 (≈ 1 %), 33/32768 (≈ 0.1 %) and
4/32768 (≈ 0.012 %). The tier is a u8, so *m* + 1 ≥ 1 and the first remainder never divides
by zero.

| Caller in the loot | *m* | Base range |
|---|---|---|
| random equipment item (§3.3) | *t* | 0 … *t* |
| pet food (§3.3; result **discarded**) | *t* | — |
| recipe (§3.5) | *t* + 1 | 0 … *t* + 1 |

A **tier-0** victim's random item is therefore rarity 0 unless a bonus fires (≈ 1.1 %).

**Worked values.** Seed 123: draws 440, 19053, 23075, 13104 — no bonus fires (53, 75, 3104
≠ 0) — so ρ(0) = 0, ρ(1) = 0, ρ(2) = 2, ρ(3) = 0, ρ(4) = 0 (440 mod 1, 2, 3, 4, 5 = 0, 0, 2,
0, 0). Seed 42: draws 175, 400, 17869, 30056 — the *d*₂ bonus fires — so ρ(0) = 1, ρ(1) = 2,
ρ(2) = 2, ρ(3) = 4, ρ(4) = 1. Each consumes 4 draws.

---

## 2. The level re-roll with spread *s* = 0.05 (Server.exe RVA 0x14470, Cube.exe RVA 0xc7e80)

It changes an item's level (+0x10, read as a signed 16-bit value) in place. The loot always
calls it with *s* = 0.05 and the "widen upward" flag set. It uses the level curve and its
inverse, in 32-bit float:

* *c*(*x*) = 1 − 1 / ((*x* − 1) · 0.05 + 1)
* *c*⁻¹(*y*) = (1 / (1 − *y*) − 1) · 20 + 1

Rule, with *N* = the item's level:

1. If *N* < 2: nothing changes, **no draw**.
2. *k* := *c*(*N*). If *k* − *s* ≤ 0: level := 1, **no draw**. (This happens for *N* = 2:
   *c*(2) = 0.047619 < 0.05. A level-2 item always becomes level 1.)
3. *lo* := trunc(*c*⁻¹(*k* − *s*)), at least 1.
4. *hi* := trunc(*c*⁻¹(min(*k* + *s*, 0.9999))) (flag set; without the flag *hi* would be *N*).
5. **One draw** *d*: level := *lo* + *d* mod (*hi* − *lo* + 1), stored as 16 bits.

The result can be **above or below** the original level. Bounds (exact float evaluation):

| *N* | *c*(*N*) | *lo* | *hi* | | *N* | *lo* | *hi* |
|---:|---:|---:|---:|---|---:|---:|---:|
| 1 | — | unchanged | | | 15 | 12 | 18 |
| 2 | 0.0476 | → 1 | | | 20 | 16 | 24 |
| 3 | 0.0909 | 1 | 4 | | 25 | 20 | 30 |
| 4 | 0.1304 | 2 | 5 | | 30 | 24 | 36 |
| 5 | 0.1667 | 3 | 6 | | 40 | 32 | 50 |
| 6 | 0.2000 | 4 | 7 | | 50 | 39 | 64 |
| 7 | 0.2308 | 5 | 8 | | 60 | 46 | 79 |
| 8 | 0.2593 | 6 | 9 | | 80 | 60 | 112 |
| 9 | 0.2857 | 7 | 11 | | 100 | 72 | 150 |
| 10 | 0.3103 | 8 | 12 | | 150 | 99 | 273 |
| 11 | 0.3333 | 8 | 13 | | 200 | 122 | 464 |
| 12 | 0.3548 | 9 | 14 | | 300 | 158 | 1556 |

From *N* = 381 on, *k* + *s* > 0.9999 and *hi* = trunc(*c*⁻¹(0.9999)) = 199,947 (float; the
real-number value would be 199,981). Whenever *hi* − *lo* + 1 exceeds 32,767 the result is
*lo* + *d*, and when that passes 32,767 it wraps negative in the 16-bit field. No ordinary
loot reaches this.

**Worked values** (seed 123, first draw 440): *N* = 1 → 1 (0 draws); 2 → 1 (0 draws);
3 → 1 + 440 mod 4 = 1; 5 → 3 + 440 mod 4 = 3; 10 → 8 + 440 mod 5 = 8; 20 → 16 + 440 mod 9 =
24; 50 → 39 + 440 mod 26 = 63; 100 → 72 + 440 mod 79 = 117 (1 draw each).

---

## 3. The loot routine, draw by draw (Server.exe RVA 0xd2ae0, Cube.exe RVA 0x19b180)

### 3.1 Placing a drop: the offset and the drop routine

Every item is dropped by the same two-part pattern.

**In the loot routine ("place")**, three draws:

* *a*: Δx := 1 − (*a* · 2) / 32767 (float, −1 … +1 block);
* *b*: Δy := 1 − (*b* · 2) / 32767; Δz := 0;
* each of Δx, Δy, Δz is multiplied by 65,536 (float) and truncated toward zero to an i64,
  then added to the victim's position;
* *e*: an angle (*e* · 360) / 32767 is passed to the drop routine — and **thrown away there**
  (see below). The draw is still spent.

Where the item record is built before placement (random item, pet food, coins, type drops,
leftovers) the offset draws come **after** the record's draws; for the recipe they come
**before** (§3.5). For the equipment item the slot pick sits between Δy and the angle (§3.2).

**In the drop routine** (Server.exe RVA 0xd2810, Cube.exe RVA 0x19aeb0; fuller behaviour in
`alpha-world-items.md` §3.2), one or two draws. It first builds a world-item record whose
scale (+0x134) starts at 1/14 and is multiplied by the caller's factor (float), then:

1. *T*: landing timer (+0x13c) := 500 + *T* mod 300. Always drawn.
2. The zone holding the position (block x and y, each divided by 256) is looked up. If it is
   **not loaded**, the item is not created and **no second draw** is made.
3. Otherwise the item record is copied in, and *A*: angle (+0x130) := (*A* · 360) / 32767,
   overwriting the argument; then the item falls from its block height down to the first
   block whose type is neither 0 nor 2 (air, water) and rests one block above it; the item is
   then inserted into the zone (into the container at zone +0x30).

So a placed drop costs **5 draws** when its zone is loaded (4 if not). Factors are listed per
step below.

### 3.2 Entry and the equipment drop

0. **No loot at all** (no draws) when the controller is 0 or 3, or the appearance has flag
   0x800.
1. **One draw, discarded.**
2. Build the list of the victim's **non-empty** equipment slots (type byte ≠ 0) in the order
   **6, 7, 2, 5, 4, 3, 1**. Slot 0 and slots numbered 8 or higher are never considered.
3. **Only if the list is not empty**: a draw *d*; the equipment drops when *d* mod 4 = 0
   **or** the victim has flag 0x200 (such a victim still spends the draw). If it drops:
   Δx draw, Δy draw, **slot pick** draw *p* → the slot list entry *p* mod *n* (unsigned, *n* =
   list length), angle draw, then the drop routine. The slot's item is dropped **as it is**
   (level, rarity, seed unchanged; no re-roll). Factor 1.0.

### 3.3 Random equipment item and pet food (controller 1 or creature type 143)

Only when the controller is 1 **or** the creature type is 143. Otherwise no draw here.

1. Draw *d*: continue only if *d* mod 10 = 0.
2. Draw *p*: level *N* := max(1, *L* − 1 + (*p* mod 2)).
3. Draw *g*: even → **armour** generator, odd → **weapon** generator.
4. Rarity ρ(*t*, false) — 4 draws (§1).
5. The generator, for **all classes** (class argument −1), with level *N* and rarity ρ:
   armour 24 draws, weapon 16 draws (`alpha-items-generator.md` §2).
6. Level re-roll (§2): 0 or 1 draw.
7. Place and drop (§3.1): 5 draws. Factor 1.0.
8. Draw *f*: a **pet food** follows only if *f* mod 5 = 0:
   1. ρ(*t*, false) — 4 draws, **result unused**.
   2. Pet food (Server.exe RVA 0x12b3f0, Cube.exe RVA 0x1f79e0). It is handed the record,
      the level *N* and that rarity, and **ignores the level and rarity**: the record becomes
      a fresh record with type 20 (level 1, rarity 0, material 0, seed 0, +0x14 block and
      +0x114 word cleared), then **one draw** *q*: subtype := low byte of entry *q* mod *n*
      (unsigned, *n* = the tameable list's length, 45) of the tameable list (§4).
   3. Level re-roll: the level is 1, so **no draw**.
   4. Place and drop: 5 draws. Factor 1.0.

Draw count of this step: 1 (no item); 1 + 2 + 4 + 24/16 + 0/1 + 5 + 1 (item, no pet food);
plus 4 + 1 + 5 = 10 more with pet food.

### 3.4 Type drops (every creature that got this far)

1. Draw *n*: count := 1 + *n* mod 3. **This draw is made for every looted victim**, whatever
   its controller, even when its type has no type drop.
2. For each of the *count* items: a fresh record (level 1, rarity 0 unless stated, seed 0),
   filled by the creature type; a type with no entry ends the step at once (no further
   draws). For types 120, 121, 124, 126 **one extra draw** decides the item (even / odd) before
   the placement. Then place and drop (5 draws).

| Creature type | Item (type 11): subtype, material, rarity | Factor |
|---|---|---|
| 58 | subtype 2, material 19 | 0.8 |
| 70 | subtype 18 | 0.8 |
| 71 | subtype 21 | 0.8 |
| 120, 121, 124, 126 | even: subtype 5, material 21; odd: subtype 1, material 2 | 0.8 |
| 122 | subtype 27 | 0.8 |
| 123 | subtype 11, material 27 | 0.8 |
| 125 | subtype 6 | 0.8 |
| 127 | subtype 20 | 0.8 |
| 128 | subtype 23 | 0.8 |
| 131 | subtype 0, material 11 | 3.0 |
| 132 | subtype 0, material 1 | 3.0 |
| 133 | subtype 0, material 12 | 3.0 |
| 134 | subtype 0, material 17 | 0.8 |
| 135 | subtype 0, material 13, rarity 1 | 3.0 |
| 136 | subtype 0, material 14, rarity 2 | 3.0 |
| 137 | subtype 0, material 15, rarity 3 | 3.0 |
| 138 | subtype 0, material 16, rarity 4 | 3.0 |
| 139 | subtype 8, material 22 | 0.8 |

Materials not listed are 0. (Table identical in both images.)

### 3.5 Recipe, coins or leftovers (controller 1 or creature type 143)

Only when the controller is 1 or the type is 143; otherwise skip to §3.6.

1. Draw *d*: a **recipe** drops when *d* mod 50 = 0. Its order differs from the other
   generated items: Δx draw, Δy draw, angle draw **first**; then ρ(*t* + 1, false) (4 draws;
   base 0 … *t* + 1); then the recipe generator with the victim's level *L* (taken as 16 bits)
   and that rarity; then the drop routine. The recipe generator starts from a fresh record,
   makes one draw (even → armour generator, odd → weapon generator, all classes), copies the
   generated item, stores the generated item's type in +0x08 and sets the type to 2 — in all
   1 + 24 or 1 + 16 = 25 or 17 draws (`alpha-items-generator.md` §3). No level re-roll.
   Factor 0.75.
2. If the appearance has **neither flag 0x08 nor 0x10**: **coins**.
   1. *V* := 2^(3 · *c*(*L*)) · 2^(0.25 · *t*), with *c* evaluated in float from *L*. Each
      power is evaluated in double precision from a float exponent and rounded to float; the
      product is a float.
   2. One draw *d* — the *r* of the formula: value := ((*d* · 2) / 32767 + 1) · (*V* · 10),
      i.e. *V* · 10 · (1 + 2*r*) with *r* = *d* / 32767; × 10 more (float) with flag 0x200;
      then truncated to an integer *v*. The draw is made even if *v* turns out 0.
   3. copper = *v* mod 100, silver = (*v* / 100) mod 100, gold = (*v* / 100) / 100 (integer
      division). In that order, each non-zero part is one item — a fresh record with type 12,
      subtype 0, material 10 (copper part) / 12 (silver part) / 11 (gold part), amount
      (+0x10) the part — and is placed and dropped (5 draws each). Factor 0.75.
3. Otherwise (flag 0x08 or 0x10 set): **leftovers** (item type 14).
   1. Draw *d*: *u* := *d* mod 1000 + 20 · *t*. Rarity 0 if *u* < 700; 1 if *u* < 950; 2 if
      *u* ≤ 998; 3 if *u* ≥ 999. With flag 0x200: + 1, at most 3.
   2. Draw *e* (always): an item drops only if *e* mod 20 = 0 **and** the rarity ≠ 0.
   3. If it drops: a fresh record with type 14; one draw is stored whole as the seed (+0x04,
      0 … 32767, not reduced); level (+0x10) := *L* (low 16 bits); rarity as above; subtype
      and material 0; then place and drop (5 draws). Factor 0.75.

| Tier *t* | rarity 1 from *d* mod 1000 = | 2 from | 3 from |
|---:|---:|---:|---:|
| 0 | 700 | 950 | 999 |
| 1 | 680 | 930 | 979 |
| 2 | 660 | 910 | 959 |
| 3 | 640 | 890 | 939 |
| 4 | 620 | 870 | 919 |

(With *t* ≥ 13, *u* ≥ 999 always and every non-zero roll is rarity 3.)

### 3.6 The inventory of a dying controller-1 victim

Only when the controller is 1 (type 143 alone does not qualify). The inventory is a list of
bags (begin/end at +0x11dc / +0x11e0, 12 bytes each); each bag a list of 284-byte slots (i32
count, then an item record). For every bag in order, every slot in order, when the slot's item
type ≠ 0 and its count > 0, repeat *count* times:

* if the item has type 1 and subtype 1 (conventionally a healing potion): nothing (no draws);
* otherwise place and drop the **slot's item record unchanged** (the drop routine copies it;
  5 draws). Factor 0.75.

So a stack of 3 drops as 3 separate world items, each a copy of the stored record (no
re-roll, no new seed).

### 3.7 Draw count summary

| Step | Condition | Draws |
|---|---|---|
| entry | loot runs | 1 |
| equipment chance | ≥ 1 non-empty slot in 6, 7, 2, 5, 4, 3, 1 | 1 |
| equipment item | chance hit or flag 0x200 | 4 (Δx, Δy, slot, angle) + drop |
| random item chance | controller 1 or type 143 | 1 |
| random item | hit | 2 + 4 + 24 or 16 + 0 or 1 + 3 + drop |
| pet-food chance | random item made | 1 |
| pet food | hit | 4 + 1 + 3 + drop |
| type-drop count | always | 1 |
| each type drop | type in the table | (1 for 120/121/124/126) + 3 + drop |
| recipe chance | controller 1 or type 143 | 1 |
| recipe | hit | 3 + 4 + 17 or 25 + drop |
| coin value | (controller 1 or type 143), neither 0x08 nor 0x10 | 1 |
| each coin kind | that part ≠ 0 | 3 + drop |
| leftovers rarity, chance | (controller 1 or type 143), 0x08 or 0x10 | 2 |
| leftovers | rarity ≠ 0 and chance hit | 1 + 3 + drop |
| inventory | controller 1, per unit not type 1 / subtype 1 | 3 + drop |

"drop" = 2 draws (1 if the zone under the item is not loaded).

---

## 4. The tameable creature list (pet-food subtypes)

The world object keeps a list of 45 creature types (begin/end at +0x88 / +0x8c, 4 bytes each;
the pet food takes the low byte as the subtype). It is filled once, near the end of the world
constructor (Server.exe RVA 0xc8570, fill beginning at RVA 0xcd517; Cube.exe RVA 0x18eb00,
fill beginning at RVA 0x193aa7). Both images hold the same list in the same order. Pet food
takes entry *q* mod 45 (unsigned) for one draw *q*. As 32768 = 45 · 728 + 8, entries 0 – 7
come up with probability 729/32768 and the rest 728/32768.

| # | type | creature | # | type | creature | # | type | creature |
|--:|--:|---|--:|--:|---|--:|--:|---|
| 0 | 35 | Bunny | 15 | 105 | LemonBeetle | 30 | 86 | Spitter |
| 1 | 87 | Mole | 16 | 19 | Collie | 31 | 99 | Camel |
| 2 | 60 | Fly | 17 | 40 | BlueSlime | 32 | 106 | Crab |
| 3 | 55 | Crow | 18 | 37 | GreenSlime | 33 | 63 | PlainRunner |
| 4 | 34 | Sheep | 19 | 38 | PinkSlime | 34 | 66 | DesertRunner |
| 5 | 23 | BrownAlpaca | 20 | 39 | YellowSlime | 35 | 64 | LeafRunner |
| 6 | 22 | Alpaca | 21 | 92 | Owl | 36 | 65 | SnowRunner |
| 7 | 30 | Cat | 22 | 93 | Penguin | 37 | 74 | Duckbill |
| 8 | 33 | Pig | 23 | 56 | Chicken | 38 | 36 | Porcupine |
| 9 | 98 | Horse | 24 | 90 | Squirrel | 39 | 57 | Seagull |
| 10 | 25 | Turtle | 25 | 91 | Raccoon | 40 | 58 | Parrot |
| 11 | 53 | Hornet | 26 | 50 | Monkey | 41 | 59 | Bat |
| 12 | 67 | Peacock | 27 | 27 | ScottishTerrier | 42 | 61 | Midge |
| 13 | 102 | BarkBeetle | 28 | 75 | Crocodile | 43 | 62 | Mosquito |
| 14 | 104 | SnoutBeetle | 29 | 26 | Terrier | 44 | 88 | Biter |

Creature names are those the client's world constructor (Cube.exe RVA 0x18eb00) registers
for each type number. Types not in the list (for example 103 FireBeetle and 151 Bumblebee)
never come from loot pet food. Pet-food item names: `alpha-item-names.md` §1.

---

## 5. Worked examples: a level-5, tier-0, controller-1 victim

The victim: controller 1, type 4 (no type drop), level 5, tier 0, appearance flags 0 (none of
0x08, 0x10, 0x200, 0x800). Equipment: items in slots 7, 6 and 2 (say a sword, a shield and a
chest piece); the non-empty list in loot order is **[slot 6, slot 7, slot 2]**. Inventory: one
bag holding a stack of 2 items of type 1 subtype 1 and a stack of 1 item of type 11 subtype
2. Every zone around it is loaded. Coins: *c*(5) = 0.1666667, *V* = 2^0.5 · 2^0 = 1.4142137,
*V* · 10 = 14.142137. Offsets below are in 1/65536 block, added to the victim's position.

### 5.1 Stream from seed 123 (16 draws)

| # | Draw | Use | Result |
|--:|--:|---|---|
| 1 | 440 | entry | discarded |
| 2 | 19053 | equipment chance | 19053 mod 4 = 1 → nothing |
| 3 | 23075 | random item chance | mod 10 = 5 → nothing |
| 4 | 13104 | type-drop count | 1 + 0 = 1, but type 4 has no entry → nothing |
| 5 | 32363 | recipe chance | mod 50 = 13 → nothing |
| 6 | 3265 | coin *r* | (6530 / 32767 + 1) · 14.142137 = 16.9605 → *v* = 16: **16 copper** |
| 7 | 30749 | copper Δx | 1 − 61498/32767 = −0.87683 → −57,463 |
| 8 | 32678 | copper Δy | −0.99457 → −65,179 |
| 9 | 9760 | copper angle argument | discarded by the drop routine |
| 10 | 28064 | copper landing timer | 500 + 164 = 664 |
| 11 | 20429 | copper angle | 224.45° |
| — | — | 2 items of type 1 / subtype 1 | skipped, no draws |
| 12 | 750 | type-11 item Δx | 0.95422 → 62,535 |
| 13 | 4480 | Δy | 0.72655 → 47,615 |
| 14 | 3863 | angle argument | discarded |
| 15 | 30429 | landing timer | 500 + 129 = 629 |
| 16 | 18586 | angle | 204.20° |

Result: two world items — 16 copper (type 12, material 10, amount 16, scale 0.75/14) and the
type-11 item (scale 0.75/14). The next draw of the stream is 30758.

### 5.2 Stream from seed 170 (61 draws, every branch but the recipe)

| # | Draw | Use | Result |
|--:|--:|---|---|
| 1 | 593 | entry | discarded |
| 2 | 32708 | equipment chance | mod 4 = 0 → drops |
| 3 | 10723 | Δx | 0.34550 → 22,642 |
| 4 | 29968 | Δy | −0.82916 → −54,339 |
| 5 | 25628 | slot pick | 25628 mod 3 = 2 → **slot 2** |
| 6 | 23981 | angle argument | discarded |
| 7 | 13194 | landing timer | 794 |
| 8 | 26169 | angle | 287.51°; slot-2 item dropped unchanged, factor 1.0 |
| 9 | 32140 | random item chance | mod 10 = 0 → drops |
| 10 | 1248 | level parity | *N* = 5 − 1 + 0 = 4 |
| 11 | 13477 | generator choice | odd → **weapon** |
| 12–15 | 2820, 29199, 27876, 24885 | ρ(0) | 0 (no bonus) |
| 16–31 | 14276 … 21087 | weapon generator (16 draws) | pick 21087 mod 11 = 0 → candidate 0: subtype 14276 mod 3 = **2**, material 1, seed 13251 mod 100 = **51** |
| 32 | 22124 | level re-roll *N* = 4 (lo 2, hi 5) | 2 + 22124 mod 4 = **2** |
| 33 | 9820 | Δx | 26,254 |
| 34 | 32691 | Δy | −65,231 |
| 35 | 16364 | angle argument | discarded |
| 36 | 25463 | landing timer | 763 |
| 37 | 12641 | angle | 138.88°; weapon type 3 / subtype 2, level 2, rarity 0 |
| 38 | 11895 | pet-food chance | mod 5 = 0 → drops |
| 39–42 | 4457, 28135, 19619, 1852 | ρ(0) | discarded |
| 43 | 30903 | tameable pick | 30903 mod 45 = 33 → type **63** (PlainRunner); level 1, no re-roll draw |
| 44 | 3937 | Δx | 49,787 |
| 45 | 23874 | Δy | −29,962 |
| 46 | 1138 | angle argument | discarded |
| 47 | 18714 | landing timer | 614 |
| 48 | 18032 | angle | 198.11° |
| 49 | 16048 | type-drop count | (no entry for type 4) |
| 50 | 20118 | recipe chance | mod 50 = 18 → nothing |
| 51 | 6308 | coin *r* | (12616/32767 + 1) · 14.142137 = 19.587 → **19 copper** |
| 52–56 | 25128, 4032, 9467, 5617, 20746 | copper Δx, Δy, angle arg, timer, angle | −34,979; 49,407; —; 717; 227.93° |
| — | — | type 1 / subtype 1 stack | skipped |
| 57–61 | 22016, 17355, 26133, 22312, 27465 | type-11 item Δx, Δy, angle arg, timer, angle | −22,530; −3,886; —; 612; 301.75° |

Five world items: the slot-2 item, a level-2 weapon of subtype 2 and rarity 0, a pet food of
subtype 63, 19 copper, the type-11 item.

---

## 6. Open points

* **L-1** Whether anything other than the world constructors writes the tameable list
  (+0x88 / +0x8c). No other writer has been identified; the list is *believed* constant
  after construction.
* **L-2** Inventory stacks whose item uses +0x10 as an amount (coins, type 11, …): the copy
  keeps +0x10, so a stack of count *n* with amount *a* would drop *n* items of amount *a*.
  Whether NPC inventories ever hold such stacks (or always count 1) is not established.
* **L-3** The meaning of the discarded first draw and of the discarded pet-food rarity is
  unknown; both draws are always made.
* **L-4** The armour and recipe generators were checked by draw-site count and branch
  structure only (24 and 1 + 24/16, all unconditional for class −1); the weapon generator's
  candidate list was checked draw by draw on the server image. A draw-by-draw comparison of
  the client generators with the server ones was not done.
* **L-5** The meanings of the slot numbers (in particular slot 1, the only slot outside 2–7
  in the list), of the controller values, of the flag bits and of the item / material numbers
  are labels taken from companion contracts; this contract defines behaviour by the numbers
  only.
* **L-6** Whether the client twin runs only when the client simulates NPCs itself (offline)
  is not established here.
* **L-7** The unit of the landing timer (+0x13c) and the drop routine's handling after the
  zone insertion (a further call at Server.exe RVA 0xda680) are left to
  `alpha-world-items.md`.

---

## Re-certification (2026-11-15)

Checked against `Server.exe` and `Cube.exe` (image base 0x400000 each) using their
disassembly listings, decompile and raw image bytes (float constants read from the images).

**Confirmed.** Entry conditions and the discarded first draw; slot order 6, 7, 2, 5, 4, 3, 1
and slot offsets; equipment chance (mod 4, flag 0x200) and the order Δx, Δy, slot pick,
angle; random-item chance, level parity, generator parity, rarity before the generator, class
−1, re-roll with 0.05 and flag set, factor 1.0; pet-food chance (mod 5), discarded rarity,
record contents, list pick; type-drop count draw, the whole type-drop table and its factors
(0.8 / 3.0); recipe chance (mod 50) with offsets before ρ(*t* + 1) and the generator, factor
0.75; coin formula, float order, boss × 10 before truncation, part order copper / silver /
gold and materials 10 / 12 / 11; leftovers thresholds, boss cap, 1-in-20 draw, unreduced seed
draw; inventory loop, bag and slot strides, type 1 / subtype 1 skip, factor 0.75; drop
routine timer before the zone check and the angle only when loaded, initial scale 1/14;
rarity roll; re-roll rule, curve constants (0.05, 20, 0.9999), the bounds table, the 381 /
199,947 cap; constants 1, 2, 32767, 360, 65536, 0.75, 0.8, 3, 10, 0.25, 2.0 (double); the
45-entry list in both images; every worked stream (seeds 123, 42, 170) recomputed in
single-precision arithmetic; client twin call order.

**Corrected** (no reversal; nothing changes a loot result or the random-draw order):
1. Addresses: old VAs "server 0x…/client 0x…" → RVAs with the image named.
2. Pet food: old "takes no rarity" → it receives the level and the rarity and ignores both.
3. Pet-food modulo: old "mod 45" → mod the list length (45).
4. Unconsidered slots: old "0 and 8 … 12" → slot 0 and slots ≥ 8 (slot count not asserted).
5. Recipe: added the generator's own parity draw and its type-2 / +0x08 conversion, which
   account for the 17 / 25 draws.
6. Drop routine: old "zone marked changed" → item inserted into the zone container; the rest
   left to the world-items contract (L-7).
7. Re-roll wrap: old "from N = 381 … the result is lo + d" → whenever *hi* − *lo* + 1 exceeds
   32,767.
8. Fresh-record contents, the +0x08 field and field widths stated explicitly.

**Removed.** Statements addressed to a reimplementation (draw-stream matching purpose,
"for cross-checking", "must be drawn to stay in step"); the former §6 list of corrections to
`alpha-server-simulation.md` and the cross-reference claims to that contract and to
`alpha-creatures-behaviour.md` (another contract's contents were not examined; the native
behaviour remains in §§1–3); pet-food item names, the claim that every list entry and
FireBeetle / Bumblebee have their own food names, and the "mace" weapon-name inference
(not established from the images); the slot-role names (off hand, main hand, …) and
material names (gold, iron, silver), now labels under L-5; unit "ms" on the landing timer; "test vectors" wording.

**Unresolved.** L-1 to L-7 above.
