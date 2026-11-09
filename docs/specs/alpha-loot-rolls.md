# Alpha loot: the draws of an NPC's death loot

Behavioural specification of the random parts of Cube World **Alpha**'s NPC death loot that
`alpha-server-simulation.md` §6.4 left open: the **rarity roll** with the tier, the **level
re-roll** with spread 0.05, the **tameable creature list** behind pet-food drops, and the
**exact order of every `rand()` draw** in the loot routine and in the drop routine it calls,
so that a reimplementation fed the same draw stream produces the same items at the same
places.

Written from the Alpha decompile (`disassembled alpha cube world/decompiled/server` and
`…/client`) and the per-instruction listings where the C output hides argument order. No code
was copied. Routines are cited by address in `Server.exe` ("server 0x…") or `Cube.exe`
("client 0x…"); object offsets are given only for cross-checking.

| Routine | Server | Client twin |
|---|---|---|
| NPC loot | 0x4d2ae0 | 0x59b180 |
| Drop one item into the world | 0x4d2810 | 0x59aeb0 |
| Rarity roll | 0x52bf40 | 0x5f8530 |
| Level re-roll | 0x414470 | 0x4c7e80 |
| Pet food | 0x52b3f0 | 0x5f79e0 |
| Armour / weapon / recipe generator | 0x528bf0 / 0x52c4e0 / 0x52a760 | 0x5f51e0 / 0x5f8ad0 / 0x5f6d50 |
| Level curve *c* / its inverse | 0x407d60 / 0x411090 | 0x43ca60 / 0x4462f0 |
| Coin base value *V* | 0x410f00 | 0x445f60 |
| Tameable list filled in | world constructor 0x4c8570 | world constructor 0x58eb00 |

The client twin runs only offline (the client then owns its NPCs); online, NPC deaths and
their loot happen on the server. **The two programs make the same draws in the same order**
(checked call by call in the client listing for the equipment, random-item and pet-food
paths; the server's three generators have the same draw sites as the client ones, 24 / 16 / 1,
and are *believed* identical in behaviour).

Related: `alpha-server-simulation.md` §6.2–6.4 (when loot runs), `alpha-items-generator.md`
(the armour, weapon and recipe generators, their draw counts), `alpha-world-items.md` §1 and
§3.2 (world-item record, the fall), `alpha-worldgen-random.md` (the `rand()` generator),
`alpha-item-names.md` (pet-food names).

---

## 0. Conventions

* A **draw** is one MSVC `rand()`: state ← state · 214013 + 2531011 (mod 2³²), result
  (state >> 16) & 0x7fff, so 0 … 32767 (`alpha-worldgen-random.md` §1). `srand(123)` gives
  440, 19053, 23075, 13104, 32363, 3265, 30749, 32678, 9760, 28064, 20429, 750, 4480, 3863,
  30429, 18586, …
* "*d* mod *n*" is the C remainder; draws are non-negative so it is the plain remainder. Where
  the code tests "even/odd" it is the low bit of the draw.
* *r* = a fraction made from a draw. All of the loot's float arithmetic is **32-bit float**
  (SSE single precision), each operation rounded to float, in the order written here.
  Conversions to integer **truncate toward zero**.
* The **victim** is the dying creature. Fields used: controller byte (+0x60: 0 player, 1
  hostile NPC, 3 merchant), creature type (+0x64), appearance flags (+0x7e: 0x18 animal,
  0x200 boss, 0x800 no loot), position (+0x10, 3 × i64 in 1/65536 block), level *L* (+0x190,
  i32), tier *t* (+0x1a8, u8), equipment slot *k* at +0x300 + 0x118·*k*, inventory bags at
  +0x11dc.
* Item record fields (280 bytes, `alpha-items-generator.md` §0): type +0x00, subtype +0x01,
  seed +0x04, rarity +0x0c, material +0x0d, level/amount +0x10 (i16).

---

## 1. The rarity roll ρ(*m*, force) (server 0x52bf40, client 0x5f8530)

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

**Test vectors.** `srand(123)`: draws 440, 19053, 23075, 13104 — no bonus fires (53, 75,
3104 ≠ 0) — so ρ(0) = 0, ρ(1) = 0, ρ(2) = 2, ρ(3) = 0, ρ(4) = 0 (440 mod 1, 2, 3, 4, 5 =
0, 0, 2, 0, 0). `srand(42)`: draws 175, 400, 17869, 30056 — *d*₂ bonus fires — so ρ(0) = 1,
ρ(1) = 2, ρ(2) = 2, ρ(3) = 4, ρ(4) = 1. Each consumes 4 draws.

---

## 2. The level re-roll with spread *s* = 0.05 (server 0x414470, client 0x4c7e80)

It changes an item's level (+0x10, i16) in place. The loot always calls it with *s* = 0.05
and the "widen upward" flag set. Uses the level curve and its inverse, in 32-bit float:

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
exact value would be 199,981); then the result is *lo* + *d*, which passes 32,767 and wraps
negative in the 16-bit field when *lo* + *d* > 32,767. No ordinary loot reaches this.

**Test vectors** (`srand(123)`, first draw 440): *N* = 1 → 1 (0 draws); 2 → 1 (0 draws);
3 → 1 + 440 mod 4 = 1; 5 → 3 + 440 mod 4 = 3; 10 → 8 + 440 mod 5 = 8; 20 → 16 + 440 mod 9 =
24; 50 → 39 + 440 mod 26 = 63; 100 → 72 + 440 mod 79 = 117 (1 draw each).

The same routine re-rolls mission bosses' equipment (`alpha-creatures-behaviour.md` §9.2
step 6), which resolves that spec's open point C-3.

---

## 3. The loot routine, draw by draw (server 0x4d2ae0, client 0x59b180)

### 3.1 Placing a drop: the offset and the drop routine

Every item is dropped by the same two-part pattern.

**In the loot routine ("place")**, three draws:

* *a*: Δx := 1 − (*a* · 2) / 32767 (float, −1 … +1 block);
* *b*: Δy := 1 − (*b* · 2) / 32767; Δz := 0;
* each of Δx, Δy, Δz is multiplied by 65,536 and truncated toward zero to an i64, then added
  to the victim's position;
* *e*: an angle 360 · *e* / 32767 is passed to the drop routine — and **thrown away there**
  (see below). The draw is still spent.

Where the item record is built by a generator (random item, pet food, coins, type 14) the
offset draws come **after** the generator's draws; for the recipe they come **before** (§3.5).
For the equipment item the slot pick sits between Δy and the angle (§3.2).

**In the drop routine** (server 0x4d2810, client 0x59aeb0; full behaviour in
`alpha-world-items.md` §3.2), one or two draws:

1. *T*: landing timer := 500 + *T* mod 300 ms (+0x13c). Always drawn.
2. If the zone under the position is **not loaded**, the item is lost and **no second draw**
   is made.
3. Otherwise *A*: angle := (*A* · 360) / 32767 degrees (+0x130), overwriting the argument;
   then the fall to the first block that is not air (0) or water (2), and the zone is marked
   changed.

So a placed drop costs **5 draws** when its zone is loaded (4 if not). The scale written is
(1/14) · factor; factors are listed per step below.

### 3.2 Entry and the equipment drop

0. **No loot at all** (no draws) when the controller is 0 (player) or 3 (merchant), or the
   appearance has flag 0x800.
1. **One draw, discarded.**
2. Build the list of the victim's **non-empty** equipment slots (type byte ≠ 0) in the order
   **6, 7, 2, 5, 4, 3, 1** (off hand, main hand, chest, shoulders, gloves, boots, slot 1).
   Slots 0 and 8 … 12 are never considered.
3. **Only if the list is not empty**: a draw *d*; the equipment drops when *d* mod 4 = 0
   **or** the victim has the boss flag 0x200 (a boss still spends the draw). If it drops:
   Δx draw, Δy draw, **slot pick** draw *p* → the slot list entry *p* mod *n* (unsigned, *n* =
   list length), angle draw, then the drop routine. The item is dropped **as it is** (level,
   rarity, seed unchanged; no re-roll). Factor 1.0.

### 3.3 Random equipment item and pet food (hostiles and type 143)

Only when the controller is 1 **or** the creature type is 143. Otherwise no draw here.

1. Draw *d*: continue only if *d* mod 10 = 0.
2. Draw *p*: level *N* := max(1, *L* − 1 + (*p* mod 2)).
3. Draw *g*: even → **armour** generator, odd → **weapon** generator.
4. Rarity ρ(*t*, false) — 4 draws (§1).
5. The generator, for **all classes** (class −1), with level *N* and rarity ρ: armour 24
   draws, weapon 16 draws (`alpha-items-generator.md` §2).
6. Level re-roll (§2): 0 or 1 draw.
7. Place and drop (§3.1): 5 draws. Factor 1.0.
8. Draw *f*: a **pet food** follows only if *f* mod 5 = 0:
   1. ρ(*t*, false) — 4 draws, **result unused** (the pet-food routine takes no rarity).
   2. Pet food (server 0x52b3f0, client 0x5f79e0): a fresh record — type 20, level 1,
      rarity 0, material 0, seed 0, no spirits — and **one draw** *q*: subtype := entry
      *q* mod 45 (unsigned) of the tameable list (§4).
   3. Level re-roll: the level is 1, so **no draw**.
   4. Place and drop: 5 draws. Factor 1.0.

Draw count of this step: 1 (no item); 1 + 2 + 4 + 24/16 + 0/1 + 5 + 1 (item, no pet food);
plus 4 + 1 + 5 = 10 more with pet food.

### 3.4 Type drops (every creature that got this far)

1. Draw *n*: count := 1 + *n* mod 3. **This draw is made for every victim**, hostile or not,
   even when its type has no type drop.
2. For each of the *count* items: a fresh record (level 1, rarity 0 unless stated, seed 0),
   filled by the creature type; a type with no entry ends the step at once (no further
   draws). For types 120, 121, 124, 126 **one extra draw** decides the item (even / odd) before
   the placement. Then place and drop (5 draws).

| Creature type | Item (type 11 unless stated): subtype, material, rarity | Factor |
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
| 131 | subtype 0, material 11 (gold) | 3.0 |
| 132 | subtype 0, material 1 (iron) | 3.0 |
| 133 | subtype 0, material 12 (silver) | 3.0 |
| 134 | subtype 0, material 17 | 0.8 |
| 135 | subtype 0, material 13, rarity 1 | 3.0 |
| 136 | subtype 0, material 14, rarity 2 | 3.0 |
| 137 | subtype 0, material 15, rarity 3 | 3.0 |
| 138 | subtype 0, material 16, rarity 4 | 3.0 |
| 139 | subtype 8, material 22 | 0.8 |

Materials not listed are 0. (Table confirmed against both programs; it matches
`alpha-server-simulation.md` §6.4 item 3.)

### 3.5 Recipe, coins or leftovers (hostiles and type 143)

Only when the controller is 1 or the type is 143; otherwise skip to §3.6.

1. Draw *d*: a **recipe** drops when *d* mod 50 = 0. Its order differs from the other
   generated items: Δx draw, Δy draw, angle draw **first**; then ρ(*t* + 1, false) (4 draws;
   base 0 … *t* + 1); then the recipe generator with level *L* (as an i16) and that rarity
   (17 or 25 draws, `alpha-items-generator.md` §3); then the drop routine. No level re-roll.
   Factor 0.75.
2. If the appearance has **neither flag 0x08 nor 0x10** (not an animal): **coins**.
   1. *V* := 2^(3 · *c*(*L*)) · 2^(0.25 · *t*). Each power is evaluated in double precision
      from a float exponent and rounded to float; the product is a float.
   2. One draw *d* — this is the *r* of the formula: value := ((*d* · 2) / 32767 + 1) ·
      (*V* · 10), i.e. *V* · 10 · (1 + 2*r*) with *r* = *d* / 32767; × 10 more with the boss
      flag; then truncated to an integer *v*. The draw is made even if *v* turns out 0.
   3. copper = *v* mod 100, silver = (*v* / 100) mod 100, gold = *v* / 10000 (integer
      division). In that order, each non-zero part is one item — type 12, subtype 0, material
      10 copper / 12 silver / 11 gold, amount (+0x10) the part, level otherwise default — and
      is placed and dropped (5 draws each). Factor 0.75.
3. Otherwise (**animals**, flag 0x08 or 0x10): **leftovers** (item type 14).
   1. Draw *d*: *u* := *d* mod 1000 + 20 · *t*. Rarity 0 if *u* < 700; 1 if *u* < 950; 2 if
      *u* ≤ 998; 3 if *u* ≥ 999. Boss: + 1, at most 3.
   2. Draw *e* (always): an item drops only if *e* mod 20 = 0 **and** the rarity ≠ 0.
   3. If it drops: one draw is stored whole as the seed (+0x04, 0 … 32767, not reduced);
      type 14, subtype 0, material 0, rarity as above, level (+0x10) := *L*; then place and
      drop (5 draws). Factor 0.75.

| Tier *t* | rarity 1 from *d* mod 1000 = | 2 from | 3 from |
|---:|---:|---:|---:|
| 0 | 700 | 950 | 999 |
| 1 | 680 | 930 | 979 |
| 2 | 660 | 910 | 959 |
| 3 | 640 | 890 | 939 |
| 4 | 620 | 870 | 919 |

(With *t* ≥ 13, *u* ≥ 999 always and every non-zero roll is rarity 3.)

### 3.6 The inventory of a dying hostile

Only when the controller is 1 (type 143 alone does not qualify). The inventory is a list of
bags (+0x11dc … +0x11e0, 12 bytes each); each bag a list of 284-byte slots (u32 count, then an
item record). For every bag in order, every slot in order, when the slot's item type ≠ 0 and
its count > 0, repeat *count* times:

* if the item is a **healing potion** (type 1, subtype 1): nothing (no draws);
* otherwise place and drop a **copy of the slot's item record unchanged** (5 draws). Factor
  0.75.

So a stack of 3 drops as 3 separate world items, each a copy of the stored record (no
re-roll, no new seed).

### 3.7 Draw count summary

| Step | Condition | Draws |
|---|---|---|
| entry | loot runs | 1 |
| equipment chance | ≥ 1 non-empty slot in 6, 7, 2, 5, 4, 3, 1 | 1 |
| equipment item | chance hit or boss | 4 (Δx, Δy, slot, angle) + drop |
| random item chance | controller 1 or type 143 | 1 |
| random item | hit | 2 + 4 + 24 or 16 + 0 or 1 + 3 + drop |
| pet-food chance | random item made | 1 |
| pet food | hit | 4 + 1 + 3 + drop |
| type-drop count | always | 1 |
| each type drop | type in the table | (1 for 120/121/124/126) + 3 + drop |
| recipe chance | controller 1 or type 143 | 1 |
| recipe | hit | 3 + 4 + 17 or 25 + drop |
| coin value | (ctrl 1 or type 143), not an animal | 1 |
| each coin kind | that part ≠ 0 | 3 + drop |
| leftovers rarity, chance | (ctrl 1 or type 143), animal | 2 |
| leftovers | rarity ≠ 0 and chance hit | 1 + 3 + drop |
| inventory | controller 1, per unit not a healing potion | 3 + drop |

"drop" = 2 draws (1 if the zone under the item is not loaded).

---

## 4. The tameable creature list (pet-food subtypes)

The world object keeps a list of 45 creature types (+0x88 … +0x8c, 4 bytes each; the low
byte is the subtype). It is filled once at the end of the world constructor (server 0x4c8570
from 0x4cd517; client 0x58eb00 from 0x593aa7) and **never changes** (*believed*: no other
writer was found). Both programs hold the same list in the same order. Pet food takes entry
*q* mod 45 (unsigned) for one draw *q*. As 32768 = 45 · 728 + 8, entries 0 – 7 come up with
probability 729/32768 and the rest 728/32768.

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

Creature names as in `alpha-item-names.md` §1 (pet food). Every entry has its own food name
there; the two named foods **not** reachable from loot are FireBeetle (103, Curry) and
Bumblebee (151, Biscuit Role).

---

## 5. Worked examples: a level-5, tier-0 hostile goblin

The victim: controller 1, type 4 (goblin, no type drop), level 5, tier 0, appearance flags
0 (not a boss, not an animal). Equipment: slot 7 an iron sword, slot 6 an iron shield, slot 2
an iron chest; the non-empty list in loot order is **[shield (6), sword (7), chest (2)]**.
Inventory: one bag holding a stack of 2 healing potions and a stack of 1 item of type 11
subtype 2. Every zone around it is loaded. Coins: *c*(5) = 0.1666667, *V* = 2^0.5 · 2^0 =
1.4142137, *V* · 10 = 14.142137. Offsets below are in 1/65536 block, added to the goblin's
position.

### 5.1 Stream from `srand(123)` (16 draws)

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
| 10 | 28064 | copper landing timer | 500 + 164 = 664 ms |
| 11 | 20429 | copper angle | 224.45° |
| — | — | 2 healing potions | skipped, no draws |
| 12 | 750 | type-11 item Δx | 0.95422 → 62,535 |
| 13 | 4480 | Δy | 0.72655 → 47,615 |
| 14 | 3863 | angle argument | discarded |
| 15 | 30429 | landing timer | 500 + 129 = 629 ms |
| 16 | 18586 | angle | 204.20° |

Result: two world items — 16 copper (type 12, material 10, amount 16, scale 0.75/14) and the
type-11 item (scale 0.75/14). The next draw of the stream is 30758.

### 5.2 Stream from `srand(170)` (61 draws, every branch but the recipe)

| # | Draw | Use | Result |
|--:|--:|---|---|
| 1 | 593 | entry | discarded |
| 2 | 32708 | equipment chance | mod 4 = 0 → drops |
| 3 | 10723 | Δx | 0.34550 → 22,642 |
| 4 | 29968 | Δy | −0.82916 → −54,339 |
| 5 | 25628 | slot pick | 25628 mod 3 = 2 → **chest** (slot 2) |
| 6 | 23981 | angle argument | discarded |
| 7 | 13194 | landing timer | 794 ms |
| 8 | 26169 | angle | 287.51°; chest dropped unchanged, factor 1.0 |
| 9 | 32140 | random item chance | mod 10 = 0 → drops |
| 10 | 1248 | level parity | *N* = 5 − 1 + 0 = 4 |
| 11 | 13477 | generator choice | odd → **weapon** |
| 12–15 | 2820, 29199, 27876, 24885 | ρ(0) | 0 (no bonus) |
| 16–31 | 14276 … 21087 | weapon generator (16 draws) | pick 21087 mod 11 = 0 → candidate 0: subtype 14276 mod 3 = **2**, material 1, seed 13251 mod 100 = **51** |
| 32 | 22124 | level re-roll *N* = 4 (lo 2, hi 5) | 2 + 22124 mod 4 = **2** |
| 33 | 9820 | Δx | 26,254 |
| 34 | 32691 | Δy | −65,231 |
| 35 | 16364 | angle argument | discarded |
| 36 | 25463 | landing timer | 763 ms |
| 37 | 12641 | angle | 138.88°; weapon type 3/2, level 2, rarity 0 |
| 38 | 11895 | pet-food chance | mod 5 = 0 → drops |
| 39–42 | 4457, 28135, 19619, 1852 | ρ(0) | discarded |
| 43 | 30903 | tameable pick | 30903 mod 45 = 33 → type **63 PlainRunner** ("Milk Chocolate Bar"); level 1, no re-roll draw |
| 44 | 3937 | Δx | 49,787 |
| 45 | 23874 | Δy | −29,962 |
| 46 | 1138 | angle argument | discarded |
| 47 | 18714 | landing timer | 614 ms |
| 48 | 18032 | angle | 198.11° |
| 49 | 16048 | type-drop count | (no entry for type 4) |
| 50 | 20118 | recipe chance | mod 50 = 18 → nothing |
| 51 | 6308 | coin *r* | (12616/32767 + 1) · 14.142137 = 19.587 → **19 copper** |
| 52–56 | 25128, 4032, 9467, 5617, 20746 | copper Δx, Δy, angle arg, timer, angle | −34,979; 49,407; —; 717 ms; 227.93° |
| — | — | healing potions | skipped |
| 57–61 | 22016, 17355, 26133, 22312, 27465 | type-11 item Δx, Δy, angle arg, timer, angle | −22,530; −3,886; —; 612 ms; 301.75° |

Five world items: the goblin's chest, a level-2 weapon of subtype 2 (*mace*, name
inferred) and rarity 0, a
PlainRunner pet food, 19 copper, the type-11 item.

---

## 6. Corrections to `alpha-server-simulation.md` §6.4

1. The equipment chance draw is made **only if** at least one of slots 6, 7, 2, 5, 4, 3, 1
   holds an item; a boss spends it too. After it come Δx, Δy, the slot pick (draw mod *n*,
   unsigned) and the angle argument, in that order.
2. "rarity from the rarity roll with the tier" = ρ(*t*) of §1 (base *d* mod (*t* + 1), plus
   three rare bonuses, cap 4); the recipe's "rarity from tier + 1" = ρ(*t* + 1), base
   *d* mod (*t* + 2).
3. Pet food: **level 1, rarity 0**; its rarity roll is drawn (4 draws) and discarded; its
   subtype is the 45-entry list of §4, draw mod 45; its re-roll spends **no** draw.
4. The type-drop count draw (1 + *d* mod 3) is made for **every** looted creature, also those
   with no type drop and non-hostiles.
5. The coin *r* is the single draw right after the recipe chance (or after the recipe's own
   draws); the formula *V* · 10 · (1 + 2*r*) is right, evaluated in float and truncated; the
   boss × 10 comes before truncation.
6. Leftovers (type 14): both the rarity draw and the 1-in-20 draw are always made; when an
   item drops it also takes a seed draw (stored unreduced) before its placement.
7. Inventory drops are for controller 1 only (not type 143) and copy each stack's item once
   per unit.
8. The drop routine spends one draw (timer) before it checks the zone and a second (the real
   angle) only when the zone is loaded; the caller's angle draw is always wasted.
9. The example in §6.4 should read "draw mod 4 = 0, then the slot pick chooses the shield
   (index 0 of [shield, sword, chest])"; its coin line (*r* = 0.5 → 28 copper) is consistent
   with §3.5.

---

## 7. Open points

* **L-1** Whether anything other than the world constructors writes the tameable list
  (+0x88). None was found by scanning both programs for writes to +0x8c; *believed* constant.
* **L-2** Inventory stacks whose item uses +0x10 as an amount (coins, type 11, …): the copy
  keeps +0x10, so a stack of count *n* with amount *a* would drop *n* items of amount *a*.
  Whether NPC inventories ever hold such stacks (or always count 1) is not checked.
* **L-3** The meaning of the discarded first draw and of the discarded pet-food rarity is
  unknown; both must be drawn to stay in step.
* **L-4** The server's armour, weapon and recipe generators are taken to behave like the
  client ones (same number of draw sites); a draw-by-draw comparison of the server listing
  was not done.
* **L-5** Slot 1's meaning (the only armour slot outside 2–5 in the list) is not named in the
  creature specs.
