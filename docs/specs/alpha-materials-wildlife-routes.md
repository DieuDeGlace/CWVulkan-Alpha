# Alpha material colours, the wildlife picker and wandering-group behaviours

Behavioural specification of three pieces of Cube World **Alpha** that other specs cite but
did not write down:

1. the **material colour** routine (client 0x4c7250) and the **shine** test (client
   0x4c7be0): what colour each item material byte gives an attached cube, which items
   shimmer, and every place the two are used, including two spirit-coloured light sources;
2. the **wildlife species picker** (server 0x5290d0 / client 0x5f56c0, open point **M-2** of
   `alpha-missions.md`) and the **zone generator's wildlife pass** that is its main user;
3. the **behaviour objects** of the cell monster and the wandering groups
   (`alpha-missions.md` §3.1, §3.2, open point **M-5**): route walk, follow the leader,
   look at player, combat and random walk, with their per-update rules and their
   "handled" results (open point **A-1** of `alpha-creatures-behaviour.md`).

Written from the Alpha decompile (`disassembled alpha cube world/decompiled/client` and
`…/server`), the per-instruction listings, and fresh disassembly of `Cube.exe` and
`Server.exe` where the C output is missing (the zone generators client 0x5e4850 / server
0x518630 and the frame routine client 0x4ac260 have none). Behaviour class names come from
the run-time type names in `Server.exe` (`cube::WalkPathBehavior` and so on). No code was
copied. Routines are cited as "client 0x…" (`Cube.exe`) or "server 0x…" (`Server.exe`);
offsets are for cross-checking only. Readings marked *believed* rest on partial evidence;
*as read* marks behaviour that looks unintended but is what the code does.

This document **adds to** and does not repeat:

* `alpha-item-services.md` §3.7–§3.9 (customization bench, spirit colours 128–131);
* `alpha-world-items.md` §2.2 and §2.5 (world-item colour, attached sub-cubes; **W-5** is
  answered in §1.4 here);
* `alpha-rendering.md` §2, §6.2 step 5 (the shimmer in the pixel shader), §10 (point
  lights), §11.2 (creature material colour);
* `alpha-creatures-models.md` §6.4 (equipment slots), §6.5 (shimmer note), Appendix B (creature
  type names used below);
* `alpha-creatures-behaviour.md` §5 (hit resolution), §9 (spawn objects and activation),
  §12.1 (spirit bursts), §13 (behaviour layer);
* `alpha-missions.md` §2.3 step 5 (mission kind 4), §3.1, §3.2 (cell monster, wandering
  groups);
* `alpha-worldgen-terrain.md` §1.2–§1.4 (columns, blocks, block types), §3 (q, W, M);
  `alpha-worldgen-regions.md` §4 (climate blends), §7 (location distance f);
  `alpha-worldgen-caves.md` §3.1 (the zone's object list), §7 (ravine spawns);
  `alpha-worldgen-structures.md` §2 (camps), §5.5.10 (animals of open plots).

---

## 0. Conventions

* **fx** = fixed-point world unit, 65 536 per block (`alpha-worldgen-overview.md`).
* **draw** = one call of the C library `rand()` (0 … 32 767). Nothing here re-seeds; every
  draw continues the stream of its caller (the zone generator's stream, the server's
  stream).
* "d mod n" with a fresh draw d is the plain remainder (draws are never negative).
* Colours are (r, g, b, a) floats. **Input colour** C = (Cr, Cg, Cb, Ca) is the colour the
  caller hands to the material routine; **strength** s is its extra float argument.
* Creature type names are those of `alpha-creatures-models.md` Appendix B (model names).
* "Handled" is the boolean an AI behaviour's update returns; a sequential behaviour stops
  at the first behaviour that returns handled (`alpha-creatures-behaviour.md` §13.1, and
  §4.1 below).

---

## 1. Material colours and shine

### 1.1 The material colour routine (client 0x4c7250)

Inputs: material byte m (0 … 255, zero-extended by every caller), input colour C,
strength s. Output: a colour K. Two families:

**Ordinary materials** (everything below 128 and above 131). K = C multiplied
component-wise by the factors of the table; the alpha is passed through unchanged
(Ka = Ca); s is ignored.

| Material m | Name (`alpha-item-names.md` §1.2) | Factors (r, g, b) | White input gives |
|---|---|---|---|
| 1 | iron | (0.7, 0.7, 0.7) | light grey |
| 2 | wood | (0.4, 0.3, 0.2) | brown |
| 5 | obsidian | (0.1, 0.1, 0.1) | near black |
| 7 | bone | (0.9, 0.9, 0.9) | off-white |
| 11 | gold | (1.0, 0.7, 0.2) | gold |
| 12 | silver | (0.8, 0.8, 0.85) | blue-grey |
| **every other value**: 0, 3, 4, 6, 8, 9, 10 (copper), 13–16 (gems), 17 (sandstone), 18–21, 22 (ice), 23–27 (light, glass, silk, linen, cotton), 28–127, 132–255 | — | (0.5, 0.5, 0.5) | mid grey |

So only six materials have a colour of their own; copper, gems, ice crystal, cloth and the
rest all come out as the input darkened to one half.

**Spirit materials** 128–131. The input colour is ignored except its alpha. With
f = 1 + 0.5·s:

| m | Spirit | (Kr, Kg, Kb) | Ka |
|---|---|---|---|
| 128 | fire | (f, 0.5f, 0.1f) | (Ca + s)·f |
| 129 | unholy | (0.3f, f, 0.5f) | (Ca + s)·f |
| 130 | ice | (0.3f, 0.5f, f) | (Ca + s)·f |
| 131 | wind | (0.8f, 0.8f, f) | (Ca + s)·f |

(These agree with `alpha-item-services.md` §3.9, which only lists the spirits.) The
constants are single-precision floats (0.7 is 0.699999988…, 0.85 is 0.850000024…), and
each product is one single-precision multiplication.

The alpha channel matters because the world shader uses the material alpha as the sun/sky
weight (`alpha-rendering.md` §5 step 7): a spirit cube with s > 0 is drawn brighter than
its surroundings, and with Ca + s > 1 it is over-lit.

### 1.2 The shine test (client 0x4c7be0)

Input: an item. Result 1 when the item's type byte (+0x00) is non-zero **and** its
material byte (+0x0d) is 1 (iron), 11 (gold), 12 (silver) or 22 (ice); otherwise 0.

The result is written to the shader's shininess constant (0x448fe0). The world pixel
shader only tests its sign: with shininess > 0 every pixel is multiplied by
1 + 0.3·sin θ, θ a screen-position-and-normal angle (`alpha-rendering.md` §6.2 step 5).
This is the moving metallic **shimmer** band seen on iron, gold, silver and ice gear. It has
no other effect (no specular term exists).

### 1.3 Where the two routines are used

There are exactly five callers of 0x4c7250 and three of 0x4c7be0 in the client (the
server draws nothing and calls neither).

| Use | Routine | Material taken from | Input colour C | Strength s |
|---|---|---|---|---|
| Attached cubes of a **lying world item** (client 0x4b2c4b → 0x471b60) | 0x4c7250 per cube | the cube's own material byte | the item's lit colour of `alpha-world-items.md` §2.2 (incl. pulse and highlight) | 0 |
| Attached cubes of **equipped items on a creature** (0x4b5e1a, 0x4b5e5c, 0x4b5e9e, 0x4b5ee0 → 0x471b60): slot 7 main hand, slot 6 off hand, slot 2 chest, slot 5 shoulders, each only when that part has a model | 0x4c7250 per cube | the cube's material | the creature's material colour (`alpha-rendering.md` §11.2) | ρ = combo counter / combo maximum (`alpha-creatures-behaviour.md` §4.4; **not** clamped) |
| Attached cubes of an **interface icon** (client 0x4758c0 → 0x471b60) | 0x4c7250 per cube | the cube's material | (1, 1, 1, 1) | 0 |
| **Customization bench** preview (client 0x588500, twice) | 0x4c7250 | held / selected material | as `alpha-item-services.md` §3.7 | 0 |
| **Spirit bursts** on a hit (client 0x59a800) | 0x4c7250 per spirit | spirit material (≥ 128 only) | (1, 1, 1, 1) | 0.5·ρ (`alpha-creatures-behaviour.md` §12.1) |
| **Average spirit colour** of an item (client 0x4c6cc0, §1.5) | 0x4c7250 per spirit | spirit material (≥ 128 only) | (1, 1, 1, 1) | 1 |
| Shine for every **attached cube** (0x471b60) | 0x4c7be0 | the **item's** material, not the cube's | — | — |
| Shine for **equipment models on creatures** (client 0x4128f0, nine calls: chest, amulet, shoulders, gloves, both weapons, …) | 0x4c7be0 | the item | — | — |
| Shine for the **main model of an interface icon** (client 0x4758c0 at 0x4765c6) | 0x4c7be0 | the item | — | — |

What the routines are **not** used for:

* The voxels of item models themselves (swords, armour, coins…) keep the colours stored in
  their `.cub` models. No material tint is applied to a weapon in the hand, to armour on a
  body or to a lying item; the material byte only picks the model
  (`alpha-creatures-models.md` §6.2–§6.3) and switches the shimmer.
* The main model of a **lying world item** is drawn **without** shimmer: the item loop of
  the frame routine never sets shininess (it stays 0 from the frame set-up, and 0x471b60
  resets it to 0 after every item), so a gold sword on the ground does not shimmer, while
  its attached cubes do (shine follows the item's material inside 0x471b60) — *as read*.
* Terrain, creatures' bodies and static objects.

Each attached cube is therefore drawn with the material colour K of its own material, the
shimmer of its item's material, and the lighting of the frame.

### 1.4 The attached-cube model (answers W-5 of `alpha-world-items.md`)

At start-up the client builds one 1 × 1 × 1 voxel model whose only voxel is white
(255, 255, 255), meshes it and keeps it at G+0x800730 (client 0x461f7d … 0x461fd9).
0x471b60 draws this model once per entry of the item's attached list (+0x14, 8 bytes per
entry: signed x, y, z offsets in model voxels at bytes 0–2, material at byte 3; count at
+0x114), with the item's model matrix translated by (x, y, z) model voxels. Because the
voxel is white, **the cube's drawn colour is exactly K** (times the usual lighting and
ambient occlusion of a lone voxel). The shininess is set once before the first cube and set
back to 0 after the last.

### 1.5 Spirit glow lights (client 0x4ac260 and 0x488ee0)

**Average spirit colour** A(item) (client 0x4c6cc0): the sum of K(m, (1, 1, 1, 1), s = 1)
over the item's attached cubes whose material is ≥ 128, divided by their number; (0, 0, 0, 0)
when there are none. With s = 1, f = 1.5 and every spirit has Ka = 3, so A.a = 3 whenever
the item carries any spirit.

**Held-spirit light** (frame routine, client 0x4b0184 … 0x4b06b3; one block per slot). For
each creature drawn, with ρ = combo counter / combo maximum as above:

* if ρ = 0, no light at all;
* otherwise, for slot 7 (main hand) and slot 6 (off hand), when the slot holds a weapon
  (type 3) and A.a > 0: a point light of colour A.rgb × ρ × **0.3** and radius **10**, at
  the hand that holds the weapon (*believed*: built from the hand matrices at creature
  object +0x1a04 and +0x1b84);
* for slot 5 (shoulders) and slot 2 (chest), when the slot is not empty and A.a > 0: a point
  light of colour A.rgb × ρ × **0.1**, radius **10**, at the creature's position. Only
  weapons can receive cubes (`alpha-item-services.md` §3.3), so these two never fire in
  normal play — *as read*.

These lights join the frame-wide list of `alpha-rendering.md` §10.2 (they are not in the
source table of §10.1 there). A creature on a combo chain therefore glows in its spirit
colour, brighter as the chain grows, and stops glowing when the combo resets.

**Critical-hit flash** (client 0x494c00 … 0x494d65, in the per-frame routine). For each hit
record the client handles whose byte +0x44 (the critical flag of
`alpha-creatures-behaviour.md` §5 step 15) is set and whose attacker exists: for each
weapon of the attacker (0x43df60), if A.a > 0, a **timed light** is queued on the list
G+0x800748: position = the hit record's 24 bytes at +0x20 (three fx integers, *believed* the
hit position; this narrows **P-10** of `alpha-net-packets.md`), colour A.rgb, radius **20**,
elapsed 0, duration **400 ms**. Each frame every timed light's elapsed time grows by the
frame time and the light is removed when elapsed > duration (client 0x4963cb …); while it
lives the frame routine adds a point light of colour × max(1 − elapsed/duration, 0) and its
radius (client 0x4af486 …). (The same list also receives a (0.3, 0.3, 0.5) flash of radius
16 for hits whose byte +0x46 is set — not a material use and not specified here.)

### 1.6 Worked examples

1. **Icon of a gold longsword with one iron cube and one fire spirit.** Icon: shininess 1
   (gold). Iron cube: K = (1, 1, 1, 1) × (0.7, 0.7, 0.7) = (0.7, 0.7, 0.7, 1). Fire cube at
   s = 0: f = 1, K = (1, 0.5, 0.1, (1 + 0)·1 = 1).
2. **The same sword in an NPC's hand**, creature material colour (1, 1, 1, 0.8), combo
   10 of 20 (ρ = 0.5). Iron cube: (0.7, 0.7, 0.7, 0.8). Fire cube: f = 1.25,
   K = (1.25, 0.625, 0.125, (0.8 + 0.5)·1.25 = 1.625). Held-spirit light: A = (1.5, 0.75,
   0.15, 3); colour A.rgb × 0.5 × 0.3 = (0.225, 0.1125, 0.0225), radius 10 at the hand.
3. **A critical hit with that sword**: a 400 ms flash at the hit point, colour
   (1.5, 0.75, 0.15) fading linearly, radius 20; after 100 ms it is (1.125, 0.5625, 0.1125).
4. **Two spirits** (fire and ice) on one weapon: A = ((1.5 + 0.45)/2, (0.75 + 0.75)/2,
   (0.15 + 1.5)/2, 3) = (0.975, 0.75, 0.825, 3), a pale lilac glow.
5. **A copper item** (material 10) with an own-material copper cube, lying in daylight with
   colour (1, 1, 1, 0.9): the cube is (0.5, 0.5, 0.5, 0.9), no shimmer.

---

## 2. The wildlife species picker (server 0x5290d0, client 0x5f56c0)

Answers **M-2** of `alpha-missions.md`. The two programs are the same routine with the same
constants and lists.

### 2.1 Inputs and climate values

Inputs: block column (x, y) (integers), a **height** h (integer, blocks), a **mode** byte
(0 or 1). Before any test the routine reads four values at (x, y), all without draws:

| Value | Routine (client / server) | Spec |
|---|---|---|
| warmth w | 0x5c4dd0 / 0x4f8b40 | `alpha-worldgen-regions.md` §4 |
| moisture m | 0x5c4800 / 0x4f8570 | same (oasis rule included) |
| fire share F | 0x5ef040 / 0x522e20 | same |
| mountain mask M = M(x, y) with no zone | 0x5effa0 / 0x523d80 | `alpha-worldgen-terrain.md` §3.4 |

### 2.2 The decision

A list of creature types is built, then one is chosen. First rule that applies:

1. **h < 0** (under sea level): the **water list**, whatever the mode.
2. **mode ≠ 0**: the **people list**.
3. **F > 0.1**: the **fire list** (nothing else is added).
4. Otherwise start with the **base** entries 21, 46, and 47 when **M > 0.3**; then
   * **h ≤ 3**: add the **shore list**;
   * **h > 3**: add the climate list chosen by w and m:
     * 0.2 ≤ w < 0.8: **temperate**;
     * w ≥ 0.8 and m < 0.2: **hot and dry**;
     * w ≥ 0.8 and m ≥ 0.6: **hot and wet**;
     * w ≥ 0.8 and 0.2 ≤ m < 0.6: **nothing** (base entries only);
     * w < 0.2: **cold**.

All comparisons are single-precision against the float constants 0.1, 0.2, 0.3, 0.6, 0.8
(slightly off the decimal values, e.g. 0.2 is 0.200000003). "w ≥ 0.6 and m ≥ 0.6" is the
literal test of the hot-and-wet branch; it is only reached with w ≥ 0.8.

Lists, **in their exact order** (the order decides the result):

| List | Entries (type number and model name) | n |
|---|---|---|
| water | 145 fish1, 146 fish2, 147 seahorse, 150 shark, 152 lantern-fish, 153 mawfish, 155 blowfish, 154 piranha | 8 |
| people | 2 human ♂, 4 goblin ♂, 7 lizardman ♂, 9 dwarf ♂, 11 orc ♂, 15 undead ♂, 51 gnobold, 48 gnoll, 76 spike | 9 |
| fire | 73 devourer, 82 minotaur, 85 imp, 112 ember golem, 103 firebeetle, 62 mosquito, 110 hell-demon | 7 |
| base | 21 skull-bull, 46 ogre, (47 rockling if M > 0.3) | 2–3 |
| shore | 106 crab, 57 seagull, 86 squid, 25 turtle | 4 |
| temperate | 54 insect-guard, 74 duckbill, 28 wolf, 56 chicken, 25 turtle, 53 hornet, 55 crow, 60 fly, 63 plainrunner, 67 peacock, 69 plant-creature, 70 radish-creature, 71 onionling, 87 mole, 88 biter, 100 cow, 90 squirrel, 91 raccoon, 92 owl, 35 bunny, 102 barkbeetle, 104 snoutbeetle, 105 lemonbeetle, 34 sheep, 33 pig, 30 cat, 31 cat2, 32 cat3, 19 collie, 20 shepherd dog, 26 dog2, 27 scottish terrier, 98 pony, 22 alpaca, 23 brown alpaca, 151 bumblebee | 36 |
| hot and dry | 54 insect-guard, 61 midge, 72 desert-onionling, 42 sand-horror, 73 devourer, 66 desertrunner, 83 desert nomad ♂, 84 desert nomad ♀, 102 barkbeetle, 103 firebeetle, 88 biter, 62 mosquito, 36 porcupine, 99 camel | 14 |
| hot and wet | 54 insect-guard, 89 koala, 75 crocodile, 74 duckbill, 71 onionling, 64 leafrunner, 68 frog, 60 fly, 58 parrot, 67 peacock, 83 nomad ♂, 84 nomad ♀, 25 turtle, 102 barkbeetle, 104 snoutbeetle, 105 lemonbeetle, 88 biter, 62 mosquito, 36 porcupine, 50 monkey, 22 alpaca, 23 brown alpaca | 22 |
| cold | 28 wolf, 56 chicken, 65 snowrunner, 90 squirrel, 93 penguin, 21 skull-bull, 49 polar gnoll, 22 alpaca, 23 brown alpaca, 35 bunny | 10 |

### 2.3 The draw and the result

If the list is empty the result is **60** (fly) with no draw; this never happens, since every
branch adds at least two entries. Otherwise **one draw** d: result = list[d mod n]. The
routine always makes exactly one draw.

The appended lists repeat entries already present (21 in cold, 25 in shore and temperate),
which doubles their chance.

### 2.4 Callers

| Caller | Mode | (x, y) | h |
|---|---|---|---|
| Zone generator wildlife pass (client 0x5eb48c, server 0x51f26c; §3) | 0 | the candidate column | first open z above the ground (§3.1 step 9) |
| Mission kind 4 (server 0x50c550, client 0x5d8750; `alpha-missions.md` §2.3 step 5) | 0 | target zone centre | L's ground height (+0x14) |
| Dead mission kinds 12, 13 (same routines) | 1 | target zone centre | as above |

### 2.5 Examples

| w | m | F | M | h | List | n | Draw | Result |
|---|---|---|---|---|---|---|---|---|
| 0.5 | 0.5 | 0 | 0.1 | 40 | base + temperate | 38 | 1000 → 12 | 69 plant-creature |
| 0.5 | 0.5 | 0 | 0.4 | 40 | base (3) + temperate | 39 | 1000 → 25 | 105 lemonbeetle |
| 0.9 | 0.1 | 0 | 0 | 40 | base + hot and dry | 16 | 7 → 7 | 66 desertrunner |
| 0.9 | 0.4 | 0 | 0 | 40 | base only | 2 | 7 → 1 | 46 ogre |
| 0.1 | 0.5 | 0 | 0 | 2 | base + shore | 6 | 13 → 1 | 46 ogre |
| any | any | 0.3 | any | 40 | fire | 7 | 32767 → 0 | 73 devourer |
| any | any | any | any | −5 | water | 8 | 100 → 4 | 152 lantern-fish |

(Index 12 of the first list: base 21, 46, then temperate positions 0–10 are 54, 74, 28, 56,
25, 53, 55, 60, 63, 67, 69.)

---

## 3. The zone generator's wildlife pass

Client 0x5eaf75 … 0x5ebbcd, server around 0x51f26c (the same instructions shifted). It
runs after the voxel-tree grid (`alpha-worldgen-vegetation.md` §5) and the other static
passes, and before the plant-sprite pass (0x5ebc60 …). The zone outline of
`alpha-worldgen-overview.md` lists this range as "creatures (follow-up)", and none of the
worldgen specs, `alpha-creatures-behaviour.md` §9 or `alpha-missions.md` describe it:
**this section is its specification**. Lz is the zone's location record (the record of its
map cell, `alpha-worldgen-locations.md` §1.2); (zx, zy) the zone.

Preceding code in the same range, not part of the wildlife pass: a 64 × 64 scan of the
region's per-zone table that collects (zone, distance) entries and sorts them, then frees
the list without using it (client 0x5eae6a … 0x5eaf72; no draws, no effect — *as read*);
and, for an Lz of type 9 whose centre lies in this zone, one creature of type **107** at the
location centre with the location's level and sub-level (client 0x5ead31 … 0x5eae65;
*believed* a location guardian, not traced further).

### 3.1 Candidates

A 3 × 3 grid: outer counter i = 0, 1, 2, inner counter j = 0, 1, 2. For each (i, j), in that
order:

1. **draw**: if d mod 4 = 0, skip this candidate (one in four never tries).
2. **draw**: x = 256·zx + 24 + 85·i + d mod 10.
3. **draw**: y = 256·zy + 24 + 85·j + d mod 10.
4. **Location clearance**: if Lz's type is neither 0 nor 10: v = 1 − f(Lz, (x·65536,
   y·65536)) (0x5f8e10); if v > 0 and v² > 0.3, skip.
5. **Cold**: w = the column's stored warmth (column +0x08, or the blend if the column is
   missing). If w < 0.2: **draw**; if d mod 4 = 0, skip.
6. **Dry**: m = the column's moisture (+0x04, or the blend). If m < 0.2: **draw**; if
   d mod 4 = 0, skip.
7. **Camps and sites**: for every point P of the zone's object list
   (`alpha-worldgen-caves.md` §3.1: camp points and type-10 site objects), if the
   horizontal distance between (x, y) and P is under 20 blocks (d² < 400, in single
   precision), skip.
8. **Ground**: z = the first height at or above the column's +0x14 value whose block type
   (low five bits) is 0 (air) or 2 (water); G = the type of the block at (x, y, z − 1)
   (the ground under it).
9. **Towns**: if q(x, y) > 0 (`alpha-worldgen-terrain.md` §3.1), skip.
10. **Flatness**: if W(x, y) < 1 (§3.2 there), skip.
11. A **creature record** (0x10f0 bytes, client 0x5adb20) at fx position
    (x·65536 + 32768, y·65536 + 32768, z·65536); **draw**: yaw (+0x54) = d·360/32767
    (single precision); level (+0x34) := 1; type (+0x2c) := the **picker** (§2) with
    (x, y, z, mode 0) — **one draw**.

Then the ground decides between a plant object and an animal (§3.2, §3.3).

### 3.2 Ground plants (controller kind 6, "inert object")

Checked in this order; "plant" means: type := the value shown, controller kind (+0x28) := 6,
append to the zone's creature list (zone + 0x18), go to the next candidate. Where noted the
health multiplier (+0xf58) is set to 25.

1. **G = 12 (fire ground)**: **draw**: type = 126 (fire-scrub) if d is odd, else 130
   (thorn-tree). Plant (health multiplier left at its default).
2. **z < 0**: go to the animal (§3.3).
3. **G = 10 (snow)**: **draw**: if d mod 4 ≠ 0, animal. Otherwise **draw**, by d mod 4:
   0 → 121 snow-bush, 1 → 123 cotton-plant, 2 → 125 cobwebscrub, 3 → 122 berry-bush;
   health multiplier 25; plant.
4. **G ∈ {4, 5, 9}** (grass, type 5, sand): **draw**: if d mod 3 ≠ 0 **and** w > 0.8 **and**
   m < 0.1: **draw**: d even → 124 scrub, d odd → 128 cactus; plant (default health
   multiplier). Otherwise:
   * G = 4 only: **draw**: if d mod 3 = 0, animal. Else if m (re-read) > 0.1: **draw**, by
     d mod 4: 0 → 120 bush, 1 → 123 cotton-plant, 2 → 127 ginseng, 3 → 125 cobwebscrub;
     health multiplier 25; plant. Else animal.
   * G = 5 or 9: animal.
5. Any other G: animal.

The picker's type is thrown away for plants (its draw still counts).

### 3.3 Animals

The record keeps the picker's type and controller kind 1 (the constructor default).

1. **Boss**: if the controller kind is 1 and the appearance flag 0x1000 is clear (always,
   for a fresh record): **draw**; if d mod 100 = 0 the appearance gets the **boss flag
   0x200**.
2. Lc = the location record of the creature's map cell (client 0x487da0; the same record as
   Lz). (lo, hi) = the type's **level range** (client 0x444020, server 0x40f0a0; §3.4).
   **draw**: level := lo + d mod (hi − lo + 1). If Lc exists, lo ≤ Lc.level (+0x24) ≤ hi and
   the creature stands inside Lc (v = 1 − f(Lc, position) > 0): tier (+0x58) := Lc's
   sub-level byte (+0x28). Otherwise the tier stays 0.
3. **Active window**: (+0x40, +0x44) := (21 600 000, 86 400 000) ms, i.e. the creature only
   activates between **06:00 and 24:00** (`alpha-creatures-behaviour.md` §9.2 step 2).
4. Append to the zone's creature list.
5. **Group**: (glo, ghi) = the type's group range (client 0x443f40, server 0x40efc0;
   §3.4). **draw**: k = glo − 1 + d mod (ghi − glo + 1) **companions** (0 when the range is
   (1, 1), but the draw is still made). For c = 0 … k − 1:
   * a new creature record, controller kind 1;
   * θ = f32(c · 2π / k) (computed in double, rounded to single);
     position = the leader's position + (trunc(f32(cos θ)·8·65536),
     trunc(f32(sin θ)·8·65536), 0) fx: a ring of radius **8 blocks** around the leader,
     the first companion due "east" (+x);
   * type = the **variant** of the leader's type (client 0x5f8590, server 0x52bfa0; §3.4),
     which may draw;
   * boss flag cleared; **draw**: level within the companion type's level range; tier :=
     the leader's tier;
   * **draw**: yaw = d·360/32767;
   * no active window (companions are always active); append.

### 3.4 Tables

**Level ranges** (0x444020). Unlisted types: (2, 4).

| (lo, hi) | Types |
|---|---|
| (3, 6) | 19, 20, 25, 26, 30, 31, 32, 60, 63, 67, 69, 70 |
| (6, 9) | 98, 100 |
| (9, 14) | 37, 38, 39, 40, 53, 59, 88, 105, 106, 151 |
| (14, 21) | 28, 61, 90, 155 |
| (21, 31) | 36, 54, 64, 65, 66, 72, 73, 91, 99, 152, 153 |
| (31, 47) | 21, 46, 47, 50, 58, 75, 80, 86, 89, 102, 104, 150 |
| (47, 81) | 17, 49, 103, 113, 114, 154 |
| (81, 180) | 81, 83, 84, 94 |
| (180, 19981) | 62, 82, 85, 97, 110, 112 |

The last row is *as read*: mosquitoes, minotaurs, imps, ember golems and hell-demons in fire
areas get levels up to 19 981. Wildlife levels do not depend on the region; only the tier
can come from a surrounding location.

**Group ranges** (0x443f40). Unlisted types: (1, 1), no companions.

| (glo, ghi) | Companions | Types |
|---|---|---|
| (1, 3) | 0–2 | 21, 28, 42, 50, 55, 63, 69, 70, 87, 88 |
| (1, 5) | 0–4 | 22, 35, 36, 54, 56, 60, 71, 72, 73, 98, 99, 100, 102, 103, 104, 105, 154 |

**Variants** (0x5f8590): a companion of a paired type is redrawn within its pair, one draw;
other types are copied with no draw.

| Leader type | Companion type |
|---|---|
| 0 or 1 | d mod 2 (0 or 1) |
| 2/3, 4/5, 7/8, 9/10, 11/12, 13/14, 15/16, 22/23, 83/84 | first of the pair + d mod 2 |
| 37–40 (slimes) | 37 + d mod 4 |
| anything else | unchanged, no draw |

### 3.5 Draw count of one candidate

| Path | Draws |
|---|---|
| skipped at step 1 | 1 |
| skipped at step 4, 7, 9 or 10 | 3, + 1 for each of steps 5, 6 that ran |
| plant on fire ground | 3 + yaw 1 + picker 1 + 1 = 6 (+ 1 for each cold/dry test) |
| plant on snow | 3 + yaw + picker + 2 = 7 (+ cold/dry) |
| scrub or cactus (G 4, 5, 9, hot and dry) | 3 + 2 + 2 = 7 (+ cold/dry) |
| plant on grass (second test) | 3 + 2 + 3 = 8 (+ cold/dry) |
| animal | 3 (+ cold/dry) + yaw 1 + picker 1 + the ground tests (0 for z < 0 or another G; 1 on snow or G 5/9; 2 on grass) + boss 1 + level 1 + group 1 + per companion (variant 0 or 1, level 1, yaw 1) |

Since the pass continues the zone's random stream, every later pass (plant sprites,
`alpha-worldgen-vegetation.md` §2–§4) depends on these counts.

### 3.6 Worked example

Zone (100, 200), Lz of type 0, candidate i = 1, j = 2, column climate w = 0.5, m = 0.5,
no camps, q = 0, W ≥ 1, column +0x14 = 37 with air at z = 37 and grass at z = 36.

1. Draw 12 345 → mod 4 = 1, go on. Draw 7 → x = 25 600 + 24 + 85 + 7 = **25 716**.
   Draw 23 → y = 51 200 + 24 + 170 + 3 = **51 397**.
2. No cold or dry draw. z = 37, G = 4. Draw 16 384 → yaw 180.0055.
3. Picker with F = 0.05, M = 0.1, h = 37: base + temperate, n = 38; draw 1000 → **69
   plant-creature**.
4. G = 4: draw 5 → 5 mod 3 = 2 ≠ 0, but w ≤ 0.8 so no cactus; G = 4: draw 9 → 9 mod 3 = 0
   → animal.
5. Boss draw 250 → 50, no boss. Level range (3, 6): draw 20 001 → 20 001 mod 4 = 1 →
   level **4**. Lc level 12 is outside (3, 6): tier 0. Window 06:00–24:00.
6. Group (1, 3): draw 7 → k = 0 + 7 mod 3 = **1** companion at θ = 0: position + (8, 0, 0)
   blocks, type 69 (no variant draw); level draw 3 → 3 + 3 mod 4 = **6**; yaw draw.

Twelve draws in all.

---

## 4. Behaviour objects (answers M-5; A-1 for the server)

### 4.1 Classes and layouts (server)

| Constructor | Class | Object fields (offsets) | Update | Clone |
|---|---|---|---|---|
| 0x41cfc0 | SequentialBehavior | list of behaviours (+0x04), count (+0x08) | 0x41d080 | vtable slot 1 |
| 0x4029e0 (radius) | CombatBehavior | special timer +0x04 (20 000 ms), radius +0x08, chase time +0x0c, cooldown +0x10 | 0x402f40 | slot 1 |
| 0x414720 | LookAtPlayerBehavior | none | 0x414730 | 0x4149c0 |
| 0x4c5d50 (radius) | WalkPathBehavior | route (+0x04 … +0x0c, 24-byte fx points), index +0x10, wait +0x14 (ms), radius +0x18 | 0x4c5e00 | 0x4c63e0 |
| 0x4c5d10 (other) | WalkPathBehavior, copy | the other's route copied, its index and radius; wait 0 | as above | as above |
| 0x4055f0 (id) | CompanionBehavior | followed creature id (u64, +0x08) | 0x405780 | 0x405ef0 |
| 0x41cb90 | RandomWalkBehavior | timer +0x04 (starts 0) | 0x41cbb0 | 0x41ce70 |

Client counterparts: the update routines named in `alpha-creatures-behaviour.md` §13
(Combat 0x42cb20, LookAtPlayer 0x4c8140, RandomWalk 0x4dbf20, sequence 0x4dc390,
WalkPath 0x58d050, Companion 0x43ace0); the rules below are read from the server and
*believed* identical on the client, which runs them offline.

Each update receives the creature, the world and the accumulated time dt (≥ 32 ms,
`alpha-creatures-behaviour.md` §13.1). "Intent" is the creature's wanted-movement vector
(creature object +0x40) that the steering of §7 there and the physics turn into motion.

At activation the spawn object's behaviour is **cloned** for the new creature (§9.2 step 8
there). The prototype in the spawn object never runs, so every (re)activated creature
starts its route at the point index the prototype holds (0 for groups) with no wait, and a
copied route keeps the index the leader's prototype had when the copy was made (0).

### 4.2 SequentialBehavior

Calls its behaviours in list order and stops at the first that returns handled; returns
handled then. If none does, intent := 0 and it returns not handled. (As
`alpha-creatures-behaviour.md` §13.1 describes.)

### 4.3 What each behaviour returns

| Behaviour | Handled when | Not handled when |
|---|---|---|
| Combat | it has a target (chasing or in range); while dodging (dodge time ≠ 0); in the first 3000 ms of action 80; drinking at low health; any of the in-range actions | **no target**; the tick it **gives up** a chase (it then sets cooldown 3000 ms and chase time 0) |
| LookAtPlayer | a player is within 8 blocks and the action is not 83 or 84 | otherwise |
| WalkPath | it is **waiting** at a point; it is walking (a path search was started or is running) | empty route; the point could not be fitted (skipped); the tick it **arrives**; the point's zone is not loaded |
| Companion | it is moving toward the followed creature (straight or by path) | the followed creature is missing; it is within 4 blocks; the ground under the followed creature cannot be found; it is within 4 blocks of that ground point; that point's zone is not loaded |
| RandomWalk | **always** | never |

This settles **A-1** of `alpha-creatures-behaviour.md` for the server. With the default
activation sequence (Combat, LookAtPlayer, RandomWalk) the RandomWalk runs whenever the
creature has no target and no player is near.

### 4.4 Combat (server 0x402f40, radius 20)

The server routine is the one `alpha-creatures-behaviour.md` §13.2 describes from the
client: the radius argument is the **acquisition radius** (a candidate qualifies when its
effective squared distance is below radius² = 400), the special timer starts at 20 000 ms,
the chase gives up after 20 000 ms and then waits 3000 ms. Every Combat object built by the
spawn code (the cell monster, the wandering groups, the default sequence) uses 20.

### 4.5 LookAtPlayer (server 0x414730)

As `alpha-creatures-behaviour.md` §13.3: clear flag 0x04; the nearest creature with
controller kind 0 and health ≥ 0 within 8 blocks (3-D, d² < 64); unless the action is 83 or
84: set flag 0x04, look vector := player − self in blocks, intent := 0, **clear the path**,
handled. A wandering creature therefore stops in front of a player who comes within 8
blocks and resumes (with a fresh path search) when the player leaves.

### 4.6 WalkPath (server 0x4c5e00, radius r = 10)

Per update:

1. Empty route → not handled.
2. wait −= dt (floored at 0). If wait > 0 → handled (nothing else; the intent is left as it
   is).
3. index := index mod (number of points) (negative becomes 0). intent := 0. T := route[index].
4. **Fit T to the ground**, with h = the creature's box height (object +0x88) and the
   world's box-collision test (0x4d4f90) applied at T + (0, 0, h/2):
   * while the box there is free, lower T by one block (at most 21 tests); if it never
     collides, **skip the point** (index + 1, not handled);
   * then, while the box collides, raise T by one block (at most 21 tests); if it never
     becomes free, skip the point.
   T is now the lowest free standing place at or below the route point (within 20 blocks).
5. **Arrived?** With p = the creature's position − (0, 0, h/2), if |p − T| < r (3-D, blocks):
   clear the path, intent := 0, **wait := 2000 + d mod 4000 ms** (one draw), index + 1,
   **not handled**.
6. **Start walking** (only when the creature has no path nodes): clear the path; path
   start := creature position + (0, 0, 0.1 − h/2); goal := T. If the zone of T is not
   loaded (0x406290), stop here, not handled (the same point is tried again next update).
   Otherwise start the path search (0x4dd2e0), set its arrival radius to r, run one search
   round (0x4dde90 then 0x4dafe0), intent := 0.
7. **Search rounds**: while the search is open (open-list count ≠ 0) run up to 10 more
   rounds per update, stopping early when the path has more than 50 nodes or its newest node
   is within r blocks of the goal (block coordinates).
8. Clear mode flag 0x40 (running); handled.

So the creature walks (at the steering's walk speed, never running) to each route point in
turn, stops within 10 blocks of it, stands 2.0–6.0 s, then heads for the next; after the
last point it starts again at the first.

### 4.7 Companion (server 0x405780, follow the creature with the stored id)

Per update:

1. Look the followed creature up by id; if it does not exist → not handled.
2. D = its position − own position. **Running** (mode flag 0x40) := |D| > 8 blocks, else
   cleared.
3. (A branch for controller kind 5 regenerates health; it does not concern wandering groups
   — see `alpha-pets-bombs.md`.)
4. intent := 0. If |D|² < 16 (within **4 blocks**) → not handled.
5. **Straight**: if the world's walk test between the two positions with width = the sum of
   both box widths passes (0x52ef00, *believed* "the straight line is walkable"): clear the
   path; intent := 5 × (Dx, Dy, 0), and if its squared length exceeds 900, intent := 30 ×
   its unit vector. Handled.
6. **By path** otherwise: find the ground under the followed creature: from its position go
   down one block at a time (at most 21) to the first block that is neither air nor water,
   then up (at most 21) to the first air or water block: goal G. (Either search failing →
   not handled.) If the creature (position − h/2) is within 4 blocks of G: clear the path,
   intent 0, not handled. If it has no path: start a search to G with arrival radius **2**
   (zone not loaded → not handled), one round, intent 0. Then up to 10 rounds as WalkPath
   step 7. Handled.

### 4.8 RandomWalk (server 0x41cbb0)

As `alpha-creatures-behaviour.md` §13.4: clear running; timer −= dt (floor 0); when it is 0:
home distance from the creature's home (record +0x1b0) in the horizontal plane; d² ≤ 100 →
intent := 10·(2d₁/32767 − 1, 2d₂/32767 − 1, 0) (two draws, x first); otherwise intent :=
10 × the horizontal unit vector toward home; timer := 3000 + d mod 5000 ms (one draw);
action := 0. **Always handled.** The timer only runs down while RandomWalk is actually
called.

### 4.9 How the spawned creatures behave

**Cell monster** (`alpha-missions.md` §3.1): sequence (Combat 20, RandomWalk). Without a
target it ambles about its home (the zone centre at ground height) in random 3.0–8.0 s legs
of intent ≈ 10, and walks straight back whenever it is more than 10 blocks from home. With a
target within 20 blocks it fights (§13.2 there); after a lost chase it waits 3 s before it
can acquire again, and the leash brings it home. It never looks at players (no
LookAtPlayer).

**Wandering-group leader** (`alpha-missions.md` §3.2): sequence (Combat 20, LookAtPlayer,
WalkPath with its 20 points and r = 10, RandomWalk).

* Target → fight. Player within 8 blocks → stop and face him.
* Otherwise walk the route point to point, standing 2–6 s at each.
* RandomWalk only runs on the ticks WalkPath returns not handled (arrival, skipped point,
  unloaded zone). Its timer starts at 0, so on the **first** arrival it fires: the leader
  gets intent 10 toward the zone centre (or a random direction when within 10 blocks of it)
  and keeps that intent through the 2–6 s wait, since WalkPath leaves the intent alone while
  waiting. Afterwards its timer (3000–7999 ms) only shrinks by one dt per arrival, so it
  practically never fires again and later waits are spent standing still — *as read*.

**Wandering-group members**: sequence (Combat 20, LookAtPlayer, Companion of the leader,
WalkPath copy, RandomWalk).

* Farther than 4 blocks from the leader: follow it (straight at up to intent 30, running
  beyond 8 blocks, or by path to the ground under it).
* Within 4 blocks: Companion is not handled, so the member walks **its own copy of the
  route** (own index, own waits); it soon drifts more than 4 blocks from the leader and
  Companion takes over again. The visible result is a loose cluster that trails the leader
  — *as read*.
* Leader dead or despawned: Companion is never handled and the member walks the route
  alone.

All members and the leader fight independently (each acquires its own target within 20
blocks); there is no shared aggro.

### 4.10 Worked example

A leader with box height 1.8 is at (1000.5, 2000.5, 30.9) (blocks) next to route point 3 at
(1005, 2003, 40). The fit tests the box at (1005, 2003, 40.9): free → lower … the box first
collides at T.z = 30 (ground at 30), then T.z rises to 31, the first free place: T = (1005,
2003, 31). p = (1000.5, 2000.5, 30.0); |p − T|² = 20.25 + 6.25 + 1 = 27.5 < 100 → arrived:
draw 4321 → wait = 2000 + 321 = 2321 ms, index 4, not handled. RandomWalk then runs: home
(zone centre) is 60 blocks away → intent 10 toward home, timer e.g. 3000 + 1234 = 4234 ms
(if this is its first call). The next 2321 ms WalkPath returns handled at step 2 and the
leader drifts toward the zone centre; then it fits point 4 and starts a path search with
arrival radius 10.

---

## 5. Additions to other specs (not edited here)

* `alpha-world-items.md` **W-5**: answered by §1.2 and §1.4 (one white voxel model; shine =
  the shader shimmer of iron, gold, silver, ice items). §2.5 there: the main model of a
  lying item does not shimmer (§1.3).
* `alpha-rendering.md` §10.1: two more light sources, the held-spirit light and the
  critical-hit flash (§1.5).
* `alpha-net-packets.md` **P-10**: hit record bytes 0x20 … 0x37 are read as a position
  (three fx integers) by the critical-hit flash (*believed*).
* `alpha-creatures-behaviour.md` **A-1**: §4.3; **A-2**: Companion and WalkPath are §4.6,
  §4.7.
* `alpha-missions.md` **M-2**: §2; **M-5**: §4. In §3.2 there, the members' type variants
  follow the table of §3.4 here (the same routine, server 0x52bfa0).
* `alpha-worldgen-overview.md` §5 row "0x5ea7f4..0x5eb8xx": the wildlife part is §3 here.

---

## 6. Notes for AlphaVulkan

* Material colours: a 256-entry table (six coloured materials, the rest 0.5) plus the
  spirit rule; keep spirits an extensible list (as `alpha-item-services.md` §6 asks). The
  shimmer is one boolean per draw; keep it on equipment, icons and attached cubes, and off
  for the main model of lying items to match the original.
* The held-spirit light and the critical-hit flash count against the 10/16-light limits of
  `alpha-rendering.md` §10.2.
* The picker and the wildlife pass are pure functions of the world plus the zone's draw
  stream; implement them as data tables (lists, level and group ranges, variants) so a mod
  can add species without code.
* Behaviours: implement the five classes with exactly the "handled" results of §4.3; the
  odd interactions of §4.9 (leader drift on the first arrival, members walking their own
  route copy) come for free and should be kept for parity, with a flag if a "fixed" mode is
  wanted later.

---

## 7. Open points

* **MW-1** The held-spirit light's position: confirm it sits at the weapon hand (creature
  object +0x1a04 / +0x1b84 matrices). *Game check*: a level-10 NPC or player with a
  spirit-cubed weapon on a combo chain at night; the coloured light should follow the hand
  and vanish when the combo resets.
* **MW-2** Which hit list the critical-hit flash walks (incoming hits only, or every hit the
  client resolves), and the meaning of hit byte +0x46 (the blue-grey flash). *Game check*:
  land a critical hit with a spirit weapon in the dark and watch for the 0.4 s flash at the
  target; let an NPC crit the player and compare.
* **MW-3** Block type 5 (one of the ground types that allow cactus and scrub) is not named in
  `alpha-worldgen-terrain.md` §1.3. *Check*: find its writer in the surface routine.
* **MW-4** The 06:00–24:00 active window of wildlife leaders, against companions that have
  none: *game check*: in a meadow between 00:00 and 06:00 only companions should appear
  (leaders spawn at 06:00). Rest (action 84) to move the clock quickly.
* **MW-5** The level range (180, 19 981) of mosquitoes, minotaurs, imps, ember golems and
  hell-demons; *game check*: visit a fire area (fire share > 0.1) and read the levels shown
  on these creatures.
* **MW-6** The type-107 creature placed in location-type-9 zones (§3, preceding code): its
  role and z (it reads the column's +0x10 value) were not traced.
* **MW-7** The leader's drift on its first route arrival (§4.9) and the members' own route
  copies: *game check*: follow one wandering group for a few minutes and compare its path
  with its 20 route points (recordable with the mod DLL's creature log: positions of leader
  and members, action, mode flag 0x40).
* **MW-8** The world tests 0x4d4f90 (box collision), 0x52ef00 (straight walk) and the path
  search 0x4dd2e0 / 0x4dde90 / 0x4dafe0 are used here by name only; they belong to a
  path-finding specification.
