# Alpha creatures: models, appearance, equipment and poses

Status: FINAL / STABLE / HELD

Behavioural contract for how the Cube World **Alpha** client builds and draws a creature:
the model table, the 172-byte appearance record and the per-type generator that fills it,
colour handling, equipment models, and the procedural pose system. Supported image:
`Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`
(image base 0x400000). The Alpha `Server.exe` contains an appearance generator of the same
size at **server** 0x40a840 (0x40a840..0x40ecff, 17 600 bytes, as the client routine
0x43f7c0..0x443c7f); identity of its behaviour is not re-verified here (§8).

Related contracts: `alpha-net-packets.md` (appearance and creature record),
`alpha-rendering.md` §11 (creature lighting and material), `alpha-data-stores.md` (`.cub`
models), `alpha-saves.md` (saved character data), `alpha-player-control.md`,
`alpha-creatures-behaviour.md` (actions and their phase lengths).

---

## 0. Conventions

* Addresses are `Cube.exe` virtual addresses unless marked **server**.
* "Record offset" = offset in the 4456-byte creature record. The client creature object
  holds the record at object offset +0x10, so object offset = record offset + 0x10.
  Client-only state beyond the record is given as object offsets ("obj +0x1188").
  Record "field *n*" numbers follow `alpha-net-packets.md` and are given only alongside
  the offset.
* "Appearance offset" = offset inside the 172-byte appearance record, which is at record
  offset 0x68 (object +0x78).
* Lengths are **model units** unless stated: one voxel of an unscaled model; §7.4 step 7
  gives the conversion to blocks. Angles are degrees unless stated.
* "Draw" = one call of the C runtime `rand()` (MSVCR110, 15-bit result 0..32767) on the
  calling thread; the generator consumes the caller's random state. "Draw *k*" is the
  *k*-th draw made by the routine being described.
* Arithmetic is single precision unless a step says double.
* *inferred* marks a meaning deduced from use; *unconfirmed* marks a statement that could
  not be established from the image. Both are collected in §8.

---

## 1. Routines

| Routine | Role |
|---|---|
| 0x4ee230 | creates the model table and loads every voxel model from `data1.db` (§2) |
| 0x4e7290, 0x4e7650 | model load and key-colour scan (§5.1) |
| 0x4e71d0, 0x4e7870 | "voxel is empty" test and model mesher (§5.1) |
| 0x428750 | appearance record constructor: default values (§3.2) |
| 0x43f7c0 | **appearance generator**: creature type + optional character-choice record → appearance (§4) |
| 0x44be40 | character loader; reads the saved choice record and calls the generator (§4.1) |
| 0x42c080 | character-creation screen update; keeps choices in range and calls the generator (§4.1) |
| 0x444680 | per-type predicate gated by appearance flags 0x0200/0x0800/0x1000 (§3.3) |
| 0x4ec400 | item → model (§6.3) |
| 0x51be60 | weapon (item type 3) → model (§6.2) |
| 0x4ec370 | consumable (item type 1) → model (§6.3) |
| 0x4b4ef0..0x4b5342 | per frame: action id → pose id (§7.2) |
| 0x4128f0 | pose and draw of one creature (§6.5, §7) |
| 0x413ae9 | pose-case dispatch: 73-entry jump table at 0x423d4c, index = pose id − 1 (§7.8, Appendix C) |
| 0x41e660..0x41f147 | channel smoothing and hand composition (§7.7) |
| 0x41f147..0x423d33 | matrices and draws of every part, equipment included (§6.5, §7.9) |
| 0x60c510 (0x61e9b5..0x61f0f1) | per simulation step: riding, walk phase and amplitude, footsteps, decays (§7.3) |
| 0x60c510 (0x61f968..0x61fd54) | attach rule, displayed position and body rotation (§7.10) |

---

## 2. The model table

At start-up 0x4ee230 creates **2569** model slots (0xa09) and then makes 2557 load calls,
each naming a `.cub` key in `data1.db` (`alpha-data-stores.md`), a fixed slot and a key
flag (§5.1). Every number in this document that names a model ("model 430") is a slot of
this table; Appendix A lists every slot.

* 2550 slots receive a model. Nineteen slots are never loaded and stay empty: 1559–1563,
  1589–1593, 2060, 2061, 2071, 2353–2356, 2419, 2420.
* Seven slots are loaded twice and the later load wins: slot 1300 is loaded with
  `orc-head-m01` and then `orc-head`; slots 1224–1228 and 1230 load the same key twice.
* Some keys occupy several slots (body2 0/1232, body4 1/1234, body3 2/1233, foot 228/432,
  torch 842/2434, mermaid-hand 1350/1358, door 2076/2482, window 2077/2484, goddess2
  2078/2481, egg1 2164/2165/2166).

A part index is used only when, read as a signed 16-bit value, it lies in 0..2568; any
other value (65535 in particular) means "no model". An index that names an empty slot
selects an existing but empty model and draws nothing.

---

## 3. The appearance record

### 3.1 Layout (172 bytes)

| Off | Size | Type | Meaning |
|---|---|---|---|
| 0x00 | 2 | 2 × u8 | *unknown*; 0 by default and never written by the generator |
| 0x02 | 3 | 3 × u8 | **hair colour** R, G, B (§5.2) |
| 0x05 | 1 | — | padding (constructor leaves it unwritten) |
| 0x06 | 2 | u16 | **appearance flags** (§3.3) |
| 0x08 | 12 | 3 × f32 | **collision box** x, y, z in blocks; box.x also sets the model scale (§7.4 step 7) |
| 0x14 | 2 | u16 | model: **head** |
| 0x16 | 2 | u16 | model: **hair** layer (the eye layer for types 17, 41, 85, 97, 109, 152) |
| 0x18 | 2 | u16 | model: **hand** (drawn twice, the +x one mirrored; front legs of quadrupeds) |
| 0x1a | 2 | u16 | model: **foot** (drawn twice; hind legs of quadrupeds) |
| 0x1c | 2 | u16 | model: **body** |
| 0x1e | 2 | u16 | model: **tail** |
| 0x20 | 2 | u16 | model: **arm** (upper arms of large humanoids) |
| 0x22 | 2 | u16 | model: **wing** (drawn twice) |
| 0x24 | 4 | f32 | head scale (head and hair) |
| 0x28 | 4 | f32 | body scale (also multiplies tail, shoulder piece and body-worn items) |
| 0x2c | 4 | f32 | hand scale (hands, held item, pet item) |
| 0x30 | 4 | f32 | foot scale |
| 0x34 | 4 | f32 | arm scale |
| 0x38 | 4 | f32 | weapon scale (both weapon slots) |
| 0x3c | 4 | f32 | tail scale |
| 0x40 | 4 | f32 | shoulder-piece scale |
| 0x44 | 4 | f32 | wing scale |
| 0x48 | 4 | f32 | body pitch: added to the body frame's x rotation and subtracted in the head matrix (§7.9.3) |
| 0x4c | 12 | 3 × f32 | hand rotation at rest (pose 0 only): right hand (x, y, z), left hand (x, −y, −z) |
| 0x58 | 4 | f32 | foot angle (added to one x rotation of each foot, §7.9.5) |
| 0x5c | 4 | f32 | wing angle (x rotation of the wings) |
| 0x60 | 4 | f32 | tail angle (x rotation of the tail) |
| 0x64 | 12 | 3 × f32 | body offset |
| 0x70 | 12 | 3 × f32 | head offset |
| 0x7c | 12 | 3 × f32 | hand offset (+x hand; the −x hand mirrors x) |
| 0x88 | 12 | 3 × f32 | foot offset (+x foot; the −x foot mirrors x) |
| 0x94 | 12 | 3 × f32 | tail offset |
| 0xa0 | 12 | 3 × f32 | wing offset |

Offsets are model units relative to the creature pivot (§7.4); x is to the creature's
right, y forward, z up. The scale names come from the drawn part each multiplies in
0x4128f0 (§6.5, §7.9).

### 3.2 Defaults (0x428750)

A newly constructed appearance holds: bytes 0x00–0x01 = 0; hair colour (255, 255, 255);
flags 0; collision box (1, 1, 1); all eight model indices 65535 (none); scales head 1.01,
body 1, hand 1, foot 0.98, arm 1, weapon 0.95, tail 0.8, shoulder 1, wing 1; body pitch 0;
hand rotation (0, 0, 0); foot, wing and tail angles 0; body offset (0, 0, −5); head offset
(0, 0.5, 5); hand offset (6, 0, 0); foot offset (3, 1, −10.5); tail offset (0, −8, 2);
wing offset (0, 0, 0).

The constructor is called by, among others, 0x5adb20, 0x43c100, 0x4465c0 and 0x60c510,
and by the creation screen before each generator call (§4.1). That every creature's
appearance is constructed before the generator runs is *inferred* from these callers.
With the defaults in place, a part that a type does not set stays **none** and is not
drawn (a human has no arm, tail or wing model; a dog no hair, arm or wing). Model 0
(`body2`) is drawn only where a type sets it: the body of types 1, 4–6, 11, 12, 15, 16, 18
and 44.

### 3.3 Appearance flags (0x06)

The generator ORs its per-type value into the existing flags (Appendix B lists the value
per type). Bits and where they are set:

| Bit | Set by the generator for types | Effect found |
|---|---|---|
| 0x0001 | 19–23, 25–34, 37–41, 74, 75, 98–105, 116, 119, 145–147, 150, 152–155 | **quadruped**: hands become front legs (hand targets and leg swing of §7.4, §7.9.4); swing ×0.75; no running lean; knocked-down pose turns about y (§7.7) |
| 0x0002 | 41, 42, 53–62, 92, 97, 101, 151 | **flyer**: off the ground (contact bit 0 clear) the swing and the foot lifts are 0 (test at 0x412ef2); eligible for the walk-cycle increments off the ground (§7.3) |
| 0x0004 | 118 | suppresses pose 11's case (Appendix C) |
| 0x0008 | 17, 37–42, 50, 63–67, 69, 70, 85, 87–94, 96, 104, 106, 107, 118 | *unknown* (tested at 0x43dd3e and 0x446f5e outside the draw) |
| 0x0010 | 19–23, 25–36, 53–62, 71–75, 86, 98–103, 105, 116, 119, 145–147, 150–155 | *unconfirmed here*: natural-weapon behaviour described in `alpha-creatures-behaviour.md` |
| 0x0020 | every type with a non-zero value except 86 and 97 | *unknown* |
| 0x0100 | 145–147, 150, 152–155 (all water creatures) | *inferred* swimmer (tested in the world step at 0x61b68b, 0x61c4a3) |
| 0x0400 | 17, 41, 85, 97, 109, 152 | **glowing eyes**: the hair layer is drawn with its colour unmodulated by the frame material (§5.2) |
| 0x0200 | — (run time) | highlight (`alpha-rendering.md` §11.2) |
| 0x0800, 0x1000 | — (run time) | *unknown* |
| 0x2000 | — (run time) | *unknown* (tested at 0x60c9c6) |

0x444680 is not a flag test for a single bit: it returns true only when none of 0x0200,
0x0800, 0x1000 is set **and** the creature type is one of 19, 20, 22, 23, 25, 26, 27, 30,
31, 32, 33, 34, 35, 53, 55, 56, 57, 58, 67, 68, 74, 87, 89, 92, 93, 98, 100, 106, 107, 145,
146, 147 (meaning *unconfirmed*).

---

## 4. Creature types and the appearance generator

### 4.1 Inputs

The generator (0x43f7c0) receives a pointer to the creature type (record offset 0x54,
32-bit), the appearance to fill, and an optional pointer to a 20-byte **character-choice
record**:

| Off | Size | Meaning |
|---|---|---|
| 0x00 | i32 | race 0..7 |
| 0x04 | u8 | sex: bit 0 = 1 female, 0 male |
| 0x08 | i32 | head choice |
| 0x0c | i32 | hair choice |
| 0x10 | 3 × u8 | hair colour R, G, B |
| 0x13 | 1 | padding |

**Character loader** (0x44be40): after reading the creature from a save it reads the next
20 bytes of the save data into a choice record and calls the generator with the
creature's type, its appearance and that record (call at 0x44c549). Type and appearance
therefore follow from the saved choice record (`alpha-saves.md`).

**With a choice record**, the generator first sets the type from race and sex:

| Race | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|---|
| Base type | 2 (human) | 0 (elf) | 9 (dwarf) | 11 (orc) | 4 (goblin) | 7 (lizard) | 15 (undead) | 13 (frogman) |

type = base + (sex bit 0). A race outside 0..7 leaves the type unchanged.

**Without a choice record** (NPCs and spawned creatures) the generator builds one: draw 1
→ blue, draw 2 → green, draw 3 → red (each the low byte of the draw), draw 4 mod 100 →
head choice, draw 5 mod 100 → hair choice, race and sex 0. The type is **not** changed.
These five draws are always made in this case, before any per-type draw.

**Character-creation screen** (0x42c080): it computes the type from race and sex with the
same table, sets the class byte (object +0x140, i.e. record 0x130) to the class menu index
+ 1, and keeps the head and hair choices within the ranges of the resulting type: a value
below 0 becomes the last index of the range, a value past the last becomes 0. The ranges
are those of Appendix B for types 0, 1, 3, 4, 5, 7–16; any other type (including 2) uses
6 heads and 15 hair layers. It then constructs a default appearance and calls the
generator (0x42c3c2).

### 4.2 Order of work

1. Choice-record handling as in §4.1 (five draws when no record is given).
2. Collision box := (0.96, 0.96, 2.16) for every type.
3. Types 0..155 dispatch through the jump table at 0x443ca0; types ≥ 156 write nothing
   more.
4. The per-type step (Appendix B) writes some of: hair colour, flags (OR), box, part
   models, scales, angles and offsets. Anything not written keeps its previous value.

Choices are reduced as `choice mod n` with C semantics: the remainder has the sign of the
dividend, so a negative choice yields a lower model index (the creation screen never
produces one). Hair colour: types 0–5, 7–12 and 148–149 copy it from the choice record;
types 13 and 14 set it to (255, 255, 255); types 15 and 16 copy it from the choice record
through a 3-byte copy helper (0x428920); types 6 and 46 replace it with three further
draws (R = third, G = second, B = first of those draws, each the low byte); every other
type leaves it unchanged.

Some boxes are computed from the box of step 2 in single precision: types 4, 5, 9, 10, 48,
49, 87, 92, 93 divide it by 1.2 (→ 0.8, 0.8, 1.8); type 81 multiplies it by 1.5
(→ 1.44, 1.44, 3.2400002); types 88 and 89 by 1.1, type 90 by 0.7, type 91 by 0.9.

### 4.3 Worked example: male human, choices head 2, hair 7, colour (120, 80, 40)

Type = 2 + 0 = 2. Box (0.96, 0.96, 2.16). Head = 1246 + (2 mod 6) = 1248 `human-head-m03`;
hair = 1252 + (7 mod 15) = 1259 `human-hair-m08`; hand 430 `hand2` (1248 is not greater
than 1248); foot 432 `foot`; body 1 `body4`; hair colour (120, 80, 40). Everything else
keeps the §3.2 defaults. With head choice 3 the head is 1249 and the hand 431
`brown-hand`: heads m04–m06 get the darker hand. No draws are made.

A bull terrier (type 6) generated without a choice record consumes seven draws in this
order: draws 1–5 for the internal choice record (§4.1), draws 6–8 for the hair colour
(B, G, R) and draw 9 for the head (2379 + draw 9 mod 3).

---

## 5. Colouring

### 5.1 Key colours in models (0x4e7290, 0x4e7650, 0x4e71d0, 0x4e7870)

Each load call passes a **key flag** that the loader stores in model object byte +0x55.
0x4ee230 passes 0 for 489 load calls and 1 for the other 2068 (2061 distinct slots end
with flag 1, since the doubly loaded slots all use 1).

* **Key colours active** (flag 0): every hair layer (`*-hair-*`), the eye layers
  `skeleton-eyes`, `imp-eyes`, `dark-troll-eyes`, `vampire-eyes`, `frightener-eyes`
  (not `lantern-fish-eyes`), all wood staffs and obsidian staffs, all wood wands, all
  gold and silver bracelets, and the "random" variants (not the five plain models) of
  iron maces, wood bows, wood crossbows and wood boomerangs.
* **Key colours inactive** (flag 1): every other model.

With key colours active, three exact colours are **keys**:

| Key | R, G, B | Collected into (model object) |
|---|---|---|
| red | 255, 0, 0 | point list at +0x04 |
| green | 0, 255, 0 | point list at +0x10 |
| blue | 0, 0, 255 | point list at +0x1c |

Rules (only when the flag is 0):

1. At load (0x4e7650) the coordinates (x, y, z) of every key voxel are appended to that
   key's list, scanning x outermost, then y, then z.
2. The mesher's test of the voxel being meshed (0x4e71d0 called with the model's key flag)
   treats a key voxel as **empty**, so it emits no faces for it. The colour must match
   exactly.
3. The mesher's neighbour tests call 0x4e71d0 in strict mode (for example the call at
   0x4ea203), where only (0, 0, 0) counts as empty. A face of an ordinary voxel facing a key
   voxel is therefore not emitted, and key voxels count as solid for occlusion
   (`alpha-rendering.md` §11.1).
4. Key voxels are not recoloured. Whether anything reads the point lists is unresolved
   (§8).

Example: `human-hair-m01.cub` is 18 × 16 × 16 voxels with 298 red, 10 blue and 4 green key
voxels (a stand-in head and eye markers), 463 white hair voxels, light-grey shading
voxels and 286 skin-coloured (255, 211, 182) voxels (ears and face edges that belong to the
hair model). Only the non-key voxels are meshed.

With flag 1, pure red, green and blue are ordinary colours and are drawn (for example the
red eyes of `troll-head` and the red of `santa-body`).

**Skin tone** is not a key: body, hand, head and foot models carry their skin as ordinary
colours (`hand2` uses (255, 182, 182) and (255, 211, 182)). Different skin tones come from
choosing other models (Appendix B: hand 430 or 431, desert-nomad hands 188/189).

### 5.2 Tinting

* Body-part models are drawn with their own voxel colours under the frame material
  (`alpha-rendering.md` §11.1).
* The **hair layer** (appearance 0x16 model) is drawn with the material
  (R/255, G/255, B/255, 1) built from appearance bytes 0x02–0x04. Normally that colour is
  multiplied component by component (alpha included) with the frame material; when
  appearance flag 0x0400 (glowing eyes) is set the colour is used **as is**, so the layer
  is not darkened by night or shade. The frame material is restored after the hair draw.
  White hair voxels therefore take the hair colour exactly, and the skin-coloured voxels
  of a hair model are tinted as well.
* The head draw sets no material of its own: the head is drawn with the frame material
  and is **not** tinted by the hair colour (0x420433..0x4206c7).
* Skin tone is not a colour: it is a choice of hand or head model (§4, Appendix B).
* Equipment colour comes from its model. The shimmer switch is described in §6.5.

---

## 6. Equipment and items

### 6.1 Item fields that select a model

Evidence: item-to-model selector 0x4ec400, weapon selector 0x51be60, consumable
selector 0x4ec370, item stat helper 0x4c70b0 (reached from 0x444db0).

| Item offset | Width | Meaning |
|---|---|---|
| 0x00 | u8 | item type |
| 0x01 | u8 | subtype |
| 0x04 | 32-bit | variant seed; picks one of several look variants. The weapon selector reduces it as a **signed** value, every other selector as an **unsigned** value |
| 0x0c | u8 | rarity; every selector clamps it to at most 4 |
| 0x0d | u8 | material |
| 0x10 | 16-bit | level, read as a signed value by 0x4c70b0 (not used for model choice) |

An item occupies 0x118 (280) bytes inside the creature record.

Item types handled by the selector (names follow the model names they select):
1 consumable, 2 formula, 3 weapon, 4 chest armour, 5 gloves, 6 boots, 7 shoulder
armour, 8 amulet, 9 ring, 10 block cube, 11 resource / ingredient, 12 coin, 13 ice
crystal, 14 digested leftovers, 15 parrot beak, 16 painting, 17 vase, 18 candle,
19 pet box / egg, 20 pet food, 21 special (key, potion, plaster, quest items), 23
vehicle (glider, boat), 24 lantern, 25 mana cube. Types 0, 22 and 26 or above select
no model.

Material numbers, named from the models they select: 1 iron, 2 wood, 5 obsidian, 6
unique clothing (innkeeper chest and body1–body5), 7 bone, 11 gold, 12 silver, 18
saurian, 19 parrot, 20 mammoth, 22 ice crystal, 23 light, 25 silk, 26 linen, 27 wool.
Resource materials 13–17 are emerald, sapphire, ruby, diamond and sandstone, and
129–131 select the unholy, ice and wind cubes.

Notation below: *r* = rarity, clamped so that any value above 3 counts as 4;
*v* = seed reduced modulo the stated number. "No model" means the selector returns
nothing (a null model reference). A model index outside 0..2568 also yields no model.

### 6.2 Weapons (item type 3), 0x51be60

Here *v* = seed modulo 11 computed on the **signed** seed (a negative seed gives a
negative remainder and hence a lower index). The selector is keyed by subtype, then
by material where listed.

| Sub | Weapon | Model index |
|---|---|---|
| 0 | sword | material 5 → 2059; material 7 → 2072; otherwise 899 + 5·*v* + *r* |
| 1 | axe | material 5 → 2060 (an empty slot); material 7 → 2074; otherwise 1770 + 5·*v* + *r* |
| 2 | mace | material 2 → 664 + *r*; material 7 → 2073; otherwise 609 + 5·*v* + *r* |
| 3 | dagger | 954 + 5·*v* + *r* |
| 4 | fist weapon | 1009 + 5·*v* + *r* |
| 5 | longsword | 1880 + 5·*v* + *r* |
| 6 | bow | 674 + 5·*v* + *r* |
| 7 | crossbow | 729 + 5·*v* + *r* |
| 8 | boomerang | 784 + 5·*v* + *r* |
| 9 | arrow | 840 |
| 10 | staff | material 5 → 604 + *r*; otherwise 434 + 5·*v* + *r* |
| 11 | wand | 489 + 5·*v* + *r* |
| 12 | bracelet | uses *v* = signed seed modulo 6: material 11 → 544 + 5·*v* + *r*; otherwise 574 + 5·*v* + *r* |
| 13 | shield | material 2 → 1119 + *r*; otherwise 1064 + 5·*v* + *r* |
| 14 | quiver | 839 |
| 15 | greatsword | material 5 → 1936; material 7 → 1935; otherwise 1825 + 5·*v* + *r* |
| 16 | greataxe | material 5 → 1993; material 7 → 1992; material 18 → 1994; otherwise 1937 + 5·*v* + *r* |
| 17 | greatmace | material 2 → 1995; material 5 → 2053; material 7 → 2052; otherwise 1996 + 5·*v* + *r* |
| 18 | pitchfork | 2331 |
| 19 | pickaxe | 2051 |
| 20 | torch | 842 |
| 21 or more | — | 899 |

Each seed-varied family occupies 55 consecutive slots: five plain models followed by
ten groups of five "random" variants, so eleven variants times five rarities fill it.
The bracelet families occupy 30 slots each (six variants). Slot 2060 is never loaded,
so an obsidian axe refers to an existing but empty model and shows nothing.

### 6.3 Armour, jewellery and other items, 0x4ec400

For armour, amulets and rings *v* = **unsigned** seed modulo 5.

* **Chest (4)**, by material: 5 → 2054; 6 → by subtype 0..6 → 1229..1235
  (innkeeper-chest, backpack, body1 … body5); 6 with any subtype above 6 → handled as
  a shoulder piece of material 6, i.e. 1149 + 5·*v* + *r*; 7 → 2067; 11 → 2062;
  18 → 1224; 19 → 2369; 20 → 2375; 22 → 2360; 23 → 670; 25 → 1669 + 5·*v* + *r*;
  26 → 1359 + 5·*v* + *r*; 27 → 1459 + 5·*v* + *r*; any other → 1124 + 5·*v* + *r*.
* **Gloves (5)**: 1 → 1174 + 5·*v* + *r*; 5 → 2057; 7 → 2070; 11 → 2065; 18 → 1226;
  19 → 2368; 20 → 2374; 22 → 2362; 23 → 672; 25 → 1744 + 5·*v* + *r*;
  26 → 1434 + 5·*v* + *r*; 27 → 1534 + 5·*v* + *r*; any other → 7 (`glove`).
* **Boots (6)**: 1 → 1199 + 5·*v* + *r*; 5 → 2056; 7 → 2069; 11 → 2064; 18 → 1227;
  19 → 2370; 20 → 2376; 22 → 2361; 23 → 673; 25 → 1719 + 5·*v* + *r*;
  26 → 1409 + 5·*v* + *r*; 27 → 1509 + 5·*v* + *r*; any other → 433 (`boot`).
* **Shoulders (7)**: 5 → 2055; 7 → 2068; 11 → 2063; 18 → 1225; 19 → 2372;
  20 → 2378; 22 → 2359; 23 → 671; 25 → 1694 + 5·*v* + *r*; 26 → 1384 + 5·*v* + *r*;
  27 → 1484 + 5·*v* + *r*; any other → 1149 + 5·*v* + *r*.
* **Amulet (8)**: material 12 → 1594 + 5·*v* + *r*; otherwise 1564 + 5·*v* + *r*.
* **Ring (9)**: material 12 → 1644 + 5·*v* + *r*; otherwise 1619 + 5·*v* + *r*.
* **Consumable (1)**, by subtype (0x4ec370): 0 → 852, 1 → 849, 2 → 850, 3 → 851,
  4 → 2395, 5 → 2092, 6 → 2393, 7 → 2546, 8 → 2335, 9 → 2333, any other → 843.
* Single-model types: 2 → 2421 (`formula`); 10 → 2075 (`cube`); 13 → 2357; 15 → 2367;
  24 → 2433 (`lantern02`); 25 → 2566 (`manacube`).
* **Coin (12)**, by material: 11 → 2086 (gold), 12 → 2085 (silver), any other → 2084
  (copper).
* **Pet box (19)**, by subtype: 24 → 2164, 25 → 2161, any other → 2080 (`pet-box`).
* **Vehicle (23)**, by subtype: 1 → 2548 (`boat2`), any other → 2547 (`glider`).
* Types 11, 14, 16, 17, 18, 20 and 21 follow in §6.3.1. Any type or key not listed
  selects no model.

#### 6.3.1 Per-subtype tables (0x4ec400)

**Resource (11)**, by subtype; "m" = material:

| Sub | Model | Sub | Model |
|---|---|---|---|
| 0 | by m: 1 → 2337 `iron-nugget`, 2 → 2365 `wood-log`, 11 → 2339 `gold-nugget`, 12 → 2338 `silver-nugget`, 13 → 2340 `emerald-nugget`, 14 → 2341 `sapphire-nugget`, 15 → 2342 `ruby-nugget`, 16 → 2343 `diamond-nugget`, 17 → 2364 `sandstone`, other → none | 14 | by m: 129 → 2350 `unholy-cube`, 130 → 2349 `ice-cube`, 131 → 2351 `wind-cube`, other → 2348 `fire-cube` |
| 1 | 2365 `wood-log` | 15 | 843 `mushroom` |
| 2 | 2366 `parrot-feather` | 16 | 2332 `pumpkin` |
| 3, 4 | none | 17 | 2334 `pineapple` |
| 5 | 2394 `plant-fiber` | 18 | 2391 `radish-slice` |
| 6 | 2352 `cobweb` | 19 | 844 `shimmer-mushroom` |
| 7 | 2373 `mammoth-hair` | 20 | 2097 `ginseng-root` |
| 8 | 2357 `ice-crystal` | 21 | 2392 `onion-slice` |
| 9 | 2363 `ice-coated-yarn` | 22 | 845 `heartflower` |
| 10 | by m: 1 → 2344 `iron-cube`, 2 → 2347 `wood-cube`, 11 → 2346 `gold-cube`, 12 → 2345 `silver-cube`, other → none | 23 | 847 `prickly-pear` |
| 11 | 2160 `cotton` | 24 | 846 `heartflower-frozen` |
| 12 | 2389 `flask` | 25 | 848 `soulflower` |
| 13 | 2336 `spiribit` | 26 / 27 | 2390 `water-flask` / 2091 `snow-berry` |
| | | 28 or more | none |

**Digested leftovers (14)**: let *k* = rarity − 1 (rarity as an unsigned byte). If *k*
is greater than 3 the model is 2321; otherwise a negative *k* counts as 0 and the model
is 2318 + *k* (`digested-leftovers01` … `04`).
**Painting (16)**: 2313 + (seed AND 1) (`painting01`, `painting02`).
**Vase (17)**: 2113 + (seed AND 3) (`vase`, `vase2`, `vase3`, `vase4`).
**Candle (18)**: subtype 1 → 2120 + (unsigned seed mod 3) (`undead-candle1..3`); any
other subtype → 2117 + (unsigned seed mod 3) (`candle01..03`).

**Pet food (20)**, by subtype (any subtype not listed → 2334 `pineapple`):

| Sub → model |
|---|
| 19 → 875 `bubble-gum`; 22 → 868 `vanilla-cupcake`; 23 → 869 `chocolate-cupcake`; 25 → 877 `cinnamon-role`; 26 → 879 `waffle`; 27 → 871 `croissant`; 30 → 859 `candy`; 33 → 858 `pumpkin-mash`; 34 → 863 `cotton-candy`; 35 → 857 `carrot`; 36 → 889 `blackberry-marmelade`; 37 → 853 `jelly-green`; 38 → 854 `jelly-pink`; 39 → 856 `jelly-yellow`; 40 → 855 `jelly-blue` |
| 50 → 870 `banana-split`; 53 → 864 `popcorn`; 55 → 876 `licorice-candy`; 56 → 865 `cereal-bar`; 57 → 890 `salted-caramel`; 58 → 891 `ginger-tartlet`; 59 → 892 `mango-juice`; 60 → 893 `fruit-basket`; 61 → 894 `melon-icecream`; 62 → 895 `bloodorange-juice`; 63 → 884 `milk-chocolate-bar`; 64 → 886 `mint-chocolate-bar`; 65 → 887 `white-chocolate-bar`; 66 → 885 `caramel-chocolate-bar`; 67 → 874 `chocolate-cookie` |
| 74 → 888 `sugar-candy`; 75 → 878 `apple-ring`; 86 → 880 `water-ice`; 87 → 862 `donut-chocolate`; 88 → 896 `pancakes`; 90 → 866 `strawberry-cake`; 91 → 867 `chocolate-cake`; 92 → 860 `lollipop`; 93 → 861 `softice`; 98 → 882 `candied-apple`; 99 → 881 `date-cookie`; 102 → 2408 `bread`; 103 → 897 `curry`; 104 → 872 `lolly`; 105 → 873 `lemon-tart`; 106 → 883 `strawberry-cocktail`; 151 → 898 `biscuit-role` |

The pet-food subtype keys coincide with creature-type numbers of Appendix B
(*unconfirmed* that each names the creature that eats it).

**Special (21)**, by subtype: 3 → 2422 `key1`; 4 and 5 → 851 `mana-potion`;
6 → 2550 `plaster`; 7 → 609 `iron-mace1`; 8 → 2363 `ice-coated-yarn`;
9 → 2395 `soup-ginseng`; any other → 2081 + (subtype AND 3) (`quest-item-amulet01`,
`quest-item-amulet02`, `quest-item-jewel-case`, `copper-coin`).

### 6.4 Equipment slots

The creature record holds the **item in hand** at record offset 0x1d8, immediately
followed by 13 equipment items (0x118 bytes each) starting at record offset 0x2f0, so
slot *k* is at record offset 0x2f0 + 0x118·*k*. The pose and draw routine 0x4128f0
selects the model of each drawn slot through 0x4ec400 once per frame.

| Slot | Record offset | Usual item type | How the draw uses it |
|---|---|---|---|
| 0 | 0x2f0 | *unconfirmed* | never read by the draw |
| 1 | 0x408 | amulet (8) (*inferred*) | extra part on the body, body scale (§6.5) |
| 2 | 0x520 | chest (4) | replaces the appearance body model |
| 3 | 0x638 | boots (6) | replaces the appearance foot model |
| 4 | 0x750 | gloves (5) | replaces the appearance hand model |
| 5 | 0x868 | shoulders (7) | extra part on the body (§6.5) |
| 6 | 0x980 | weapon (3), off hand | in the left hand, or on the back (§6.5) |
| 7 | 0xa98 | weapon (3), main hand | in the right hand, or on the back (§6.5) |
| 8, 9 | 0xbb0, 0xcc8 | rings (9) (*inferred*) | never read by the draw |
| 10 | 0xde0 | lantern (24) (*inferred*) | drawn with the left hand while mode flag 0x200 is set |
| 11 | 0xef8 | vehicle (23) | while mode flag 0x10 is set (glider) and, separately, while the action id is 107 (boat) (§6.5) |
| 12 | 0x1010 | pet item (*inferred*) | drawn at the hands' midpoint during action 82 (§6.5) |

The weapon slot types 3 and the armour types of slots 2–7 are those the stat helper
0x444db0 requires; the item types named for slots 1, 8, 9, 10 and 12 are inferred from
the models drawn there and are not checked by the draw, which draws whatever model the
slot's item selects.

The **item in hand** (record 0x1d8) is drawn at the midpoint of the two hands with the
hand scale during actions 80 and 81 while the action time is below the item's use limit
(consumables: 10000 ms, consumable subtype 1: 3000 ms, any other item: 0) (§6.5).

Substitution: when slot 2, 3 or 4 selects no model (an empty item, type 0, or any
combination that selects nothing) the appearance body, foot or hand model is used. The
head and hair are always the appearance models; no equipment slot replaces them. The
tail, arm and wing are always appearance models. An appearance index is used only when
it lies in 0..2568 read as a signed 16-bit value; otherwise that part has no model. A
selected model that is an empty slot (for example 2060) suppresses the substitution and
shows nothing.

**Draw flags.** The caller passes a bit set that removes parts for this draw:

| Bit | Removes |
|---|---|
| 0x001 | head |
| 0x002 | hair layer |
| 0x004 | shoulder piece (slot 5) |
| 0x008 | body / chest |
| 0x040 | both weapons (slots 6 and 7) |
| 0x010 | the left hand, together with the lantern drawn with it |
| 0x020 | the right hand |
| 0x080 | the first foot drawn (the left foot, *unconfirmed* side) |
| 0x100 | the second foot drawn (the right foot, *unconfirmed* side) |

Bits 0x01–0x40 are applied before any matrix is built (0x41f0f4..0x41f147); bits
0x10, 0x20, 0x80 and 0x100 are tested at each hand and foot draw (0x423001, 0x4233b0,
0x4238d7, 0x423b11). When the hand model itself is absent neither hand nor the lantern
is drawn.

**Left and right.** Following §3.1, the **left** hand is the −x hand and the **right**
hand the +x hand. The right hand is the one drawn with a negative x scale (mirrored);
its rotation and position channels are the "R" channels of §7.7, and it carries the
main weapon (slot 7). The left hand carries the off-hand weapon (slot 6) and the
lantern (slot 10). Of the two hand draws the left one comes first.

### 6.5 How equipment is drawn (0x41f147..0x423d33)

Every equipment model is placed by the part rule of §7.4 step 8: a parent frame, then the
listed translations and rotations, each applied to the current matrix in the order written
(post-multiplied), then the scale, then a translation by fractions of the model's voxel
size (*sx*, *sy*, *sz*). "Centred" means T(−*sx*/2, −*sy*/2, −*sz*/2). The three parent
frames are the **base frame** (§7.4 step 7), the **body frame** (§7.4 step 8) and the
**arm frame** (§7.9.2). R*a*(θ) is a right-handed rotation about local axis *a* by θ
degrees. *s* is the walk swing of §7.4 step 1 after all of its factors; *s′* is the
swing as it stands right after the quadruped factor. *w* = appearance weapon scale (0x38),
*b* = body scale (0x28), *n* = hand scale (0x2c). Subtype tests in this section read only
the subtype byte of the slot; only the quiver tests also require item type 3.

**Shimmer switch.** Immediately before drawing the chest, amulet, shoulders, main weapon,
off-hand weapon, left hand and both feet, the shader's shimmer switch is set from the
corresponding slot (2, 1, 5, 7, 6, 4, 3): 1 when that slot's item type is non-zero and its
material is 1, 11, 12 or 22 (0x4c7be0), otherwise 0. After each of these draws it is set
back to 0. The right hand sets no value of its own (it inherits whatever the switch holds)
and resets it to 0 afterwards; the lantern draw resets it to 0 before drawing.

**Glider (slot 11, mode 0x10).** Body frame · T(body offset + (0, 0, 8·box.z/box.x)) ·
R*x*(−*B*.x) · S(2*b*) · centred (*B* = smoothed body tilt, §7.7).

**Boat (slot 11, action 107).** Body frame · T(body offset + (0, 0, −9·box.z/box.x)) ·
R*x*(−*B*.x) · R*z*(−*B*.z) · S(2*b*) · T(1, 2, 0) · T(−*sx*/2, −*sy*/2, 0).

**Mana cube** (model 2566) while the pose id is 72 and the pose-state time copy is below
5000: arm-frame matrix · T(midpoint of the two hand positions + (0, 0, 8)) ·
R*z*(0.02·*t*) · R*y*(45) · R*x*(45) · S(1) · centred. It is drawn before the arm frame of
the current frame is rebuilt, so the matrix it starts from is the arm frame kept in the
pose state from the previous call (*as read*, 0x41fa97..0x41fc52 versus 0x420a48).

**Chest (slot 2, else the appearance body).** Body frame · T(body offset) ·
R*z*(*s*·90/π + *Y*/2) · S(*b*) · centred, where *Y* is the smoothed aim yaw (§7.7).

**Plaster.** Only when a chest/body model is drawn and record field 20 (offset 0x12c, f32)
is > 0, model 2550 is drawn: body frame · T(body offset) · R*z*(*s*·90/π + *Y*/2) · S(*b*) ·
T(0, −½·(chest model *sy*), 0) · S(0.5) · R*y*(45) · R*z*(180) · T(−*sx*/2, 0, −*sz*/2).
No shimmer value is set for it. (*inferred*: a bandage after damage.)

**Amulet (slot 1).** Body frame · T(body offset + (0, 3, 6.05)) · R*z*(*s*·90/π + *Y*/2) ·
R*x*(0.7·*s*²·180/π) · S(*b*) · T(−*sx*/2, −1.5, −*sz*): it hangs from its top edge and
swings forward with the square of the swing.

**Shoulders (slot 5).** Body frame · T(0, 0, 4 + 2·*a*) · T(body offset) · R*z*(*s*·90/π) ·
S(shoulder scale) · S(*b* + 0.1) · centred (*a* = walk amplitude).

**Gloves (slot 4)** and **boots (slot 3)** only replace the hand and foot models; they are
placed exactly as hands (§7.9.4) and feet (§7.9.5).

**Held item (record field 43).** During actions 80 and 81, while the action time (record)
is below the held item's use limit (0x4c6b80: 10 000 ms for item type 1, 3 000 ms for type 1
subtype 1, otherwise 0): arm frame · T(midpoint of the two hand positions) · R*x*(λ.x) ·
S(*n*) · centred (λ = left-hand rotation, §7.7).

**Pet item (slot 12).** During action 82 (no time limit): the same matrix, ending with
T(−*sx*/2, −*sy*/2, 0) instead of the centring.

**Back quivers for bows.** If slot 6 holds a weapon (type 3) of subtype 6 or 7, model 839
is drawn on the back: body frame · R*z*(*s*·45/π) · T(0, −7.1, 0.1) · R*y*(45) ·
R*z*(90) · S(*w* + 0.01) · centred. Then, if slot 7 holds a weapon (type 3) of subtype 6
or 7, model 839 is drawn the same way with R*y*(−45). Both are drawn whatever the action.

**Main weapon (slot 7) glow.** While record field 23 (offset 0x134) is > 0 the material
for the main weapon is the frame material (all four components) multiplied by
1 + 0.25·(1 + cos(0.01·*t*)), *t* = pose-state time copy in ms; the frame material is
restored after the weapon draw. This applies whether the weapon is in the hand or on the
back. (*inferred*: a charge glow.)

**Main weapon, on the back (sheathed).** When the action is 0, 21, 79–85, 91, 96, 105,
106, 107 or 109, or mode 0x10 (gliding) is set — unless the main subtype is 4 (fist) or
12 (bracelet): body frame · R*z*(*s*·45/π) · T(0, −7.1, 0.1) · R*y*(−45 for a staff
(subtype 10), −135 for every other subtype) · R*z*(90) · S(*w*) · **centred**.

**Main weapon, in the hand.** Otherwise: arm frame · T(*R*) · R*z*(ρ.z) · R*y*(ρ.y) ·
R*x*(ρ.x) · R*z*(*W*.z) · R*y*(*W*.y) · R*x*(*W*.x + 0.5·*p*) · S(*w*) · grip translation,
where *R* is the right-hand position and ρ its rotation (§7.7), *W* the main grip rotation
(zero unless a pose case writes it) and *p* the head pitch (record field 5). The grip is
chosen in this order:

| Test | Grip translation |
|---|---|
| model is 840 (`arrow`) | (−*sx*/2 − 1, 0.5, 1) |
| subtype 10 (staff) | (−*sx*/2, −*sy*/2, −10) |
| subtype 12 (bracelet) | S(0.6) first, then (−*sx*/2, 1 − *sy*, −*sz*/2) |
| subtype 6 (bow) | (−*sx*/2, 1.5 − *sy*, −*sz*/2) |
| subtype 7 (crossbow) | (−*sx*/2, 0, −*sz*/2) |
| subtype 8 (boomerang) | (−*sx*/2, −2, −4) |
| subtype 4 (fist) | centred |
| anything else | (−*sx*/2, −*sy*/2, −4) |

So a sword's voxel 4 above the bottom of the model sits at the hand and the blade points
along the hand's +z.

**Off-hand weapon (slot 6), on the back.** Same action list as the main weapon, but mode
0x10 does **not** sheathe it; never sheathed when the off-hand subtype is 20 (torch), 4 or
12: body frame · T(0.5, −0.65, 0.5) · R*z*(*s*·45/π) · then by subtype — 13 (shield):
T(0, −8.2, 0) · R*z*(180); 6 or 7: T(0, −6.2, 0) · R*y*(−45) · R*z*(90); any other:
T(0, −7.2, 0) · R*y*(−225) · R*z*(90) — then S(*w*) · centred. No glow is applied in this
case.

**Off-hand weapon, in the hand.** Arm frame · T(*L*) (pose 71: T(midpoint of the hands +
(0, 0, 8))) · R*z*(λ.z) · R*y*(λ.y) · R*x*(λ.x) · R*z*(*W*′.z) · R*y*(*W*′.y) ·
R*x*(*W*′.x + 0.5·*p*) · S(*w*′) · grip, with *w*′ the off-hand copy of the weapon scale
(the pose-71 case multiplies it by 1.5 and puts model 2101 in the off hand). The glow
rule above also applies here. Grip, tested in this order:

| Test | Grip translation |
|---|---|
| model is 840 (`arrow`) | (1 − *sx*/2, 0.5, 1) |
| pose 71 | centred |
| off-hand subtype 6 | (−*sx*/2, 1.5 − *sy*, −*sz*/2) |
| off-hand subtype 7 | (−*sx*/2, 0, −*sz*/2) |
| off-hand subtype 12 | S(0.6) first, then (−*sx*/2, 1 − *sy*, −*sz*/2) |
| off-hand subtype 13 (shield) | (−*sx*/2, 3, −*sz*/2) |
| **main** subtype 8 (boomerang) | (−*sx*/2, −2, −4) |
| off-hand subtype 4 | R*y*(180) first, then centred |
| anything else | (−*sx*/2, −*sy*/2, −4) |

**Slot-6 quiver.** If slot 6 holds a quiver (type 3, subtype 14) its model (839) is taken
out of the off hand before posing (§7.7 step 4), so no off-hand weapon is drawn, and after
the off-hand step it is drawn on the back: body frame · T(0.65, −0.5, 0.3) · R*z*(*s*·45/π) ·
T(0, −6.2, 0) · R*y*(−45) · R*z*(90) · S(*w*) · centred.

**Weapon trail points.** After each main- or off-hand weapon draw (in hand or on back) the
model points (*sx*/2, *sy*/2, 0) and (*sx*/2, *sy*/2, *sz*) are transformed by that weapon's
matrix and kept in the pose state (main hand +0x62c / +0x56c, off hand +0x7ac / +0x6ec). A
16-entry trail history is then updated for poses 1–3, 8, 10, 12, 14, 28–32, 35, 48 and 49
(both hands), 15, 17 and 19 (off hand) and 16, 18, 20 and 50 (main hand), and a 16-step
circle is built for pose 69 (0x4225fd..0x422fe5). The exact time window of the history
and the trail rendering are *unconfirmed* (§8, U-9).

**Lantern (slot 10).** Drawn only inside the left-hand step (a hand model is present and
draw-flag bit 4 is clear) while mode flag 0x200 is set: arm frame · T(*L* + (0, 2, −2)) ·
S(0.8) · T(−*sx*/2, 1 − *sy*/2, 2 − *sz*). The shimmer switch is set to 0 and the material
to (1, 1, 1, 1) for the draw (so it is not darkened), then the frame material is restored.

---

## 7. Poses and animation

The creature has no skeleton file. Every frame 0x4128f0 computes the pose procedurally from the creature record (action id, action time, velocity, flags), client-only animation state, and smoothed pose channels kept per creature in the pose state (obj +0x1498).

### 7.1 Inputs per creature

| Source | Meaning |
|---|---|
| record 0x58 (field 9) | **action id** (u8) |
| record 0x5c (field 10) | **action time** in ms since the action began |
| record 0x24 (field 2) | velocity (blocks/s); x, y used for the running and lean tests, all three for the dodge axis |
| record 0x30 (field 3) | movement intent (`alpha-player-control.md` §3) |
| record 0x48 (field 5) | head pitch *p* (degrees) |
| record 0x4c (field 6) | contact flags: bit 0 on ground, bit 2 touching a wall |
| record 0x50 (u8) | value 5 suppresses the riding leg pose (§7.4 step 2) |
| record 0x54 (field 8) | creature type (type 101 special cases) |
| record 0x114 (field 14) | mode flags: 0x01 climbing allowed, 0x04 aiming, 0x10 gliding, 0x40 running, 0x200 lantern lit, 0x400 sitting pose (*names inferred from use*) |
| record 0x118 (field 15) | dodge-roll time left (ms; the roll angle is scaled by 600) |
| record 0x11c (field 16) | > 0: knocked-down pose |
| record 0x120 (field 17) | > 0: pose 56 is forced (§7.2) |
| record 0x12c, 0x134 (fields 20, 23) | plaster and weapon glow (§6.5) |
| record 0x130, 0x131 | class and specialisation (dodge exception, §7.4 step 7) |
| record 0x150 (field 26) | aim direction (*inferred*), §7.7 step 2 |
| record 0x1d8 (field 43) | held item |
| record 0x2f0 (field 44) | the 13 equipment items, 0x118 bytes each |
| obj +0x11c0 | **mount id** (client-only, u64): non-zero → riding |
| obj +0x1180 | landing bob (blocks) |
| obj +0x1188 | **walk amplitude** *a* 0..1 (§7.3) |
| obj +0x118c | **walk phase** (radians) |
| obj +0x1350 | displayed position (3 × i64, 1/65536 block), §7.10 |
| obj +0x1374 | displayed body rotation *D* (x, y, z degrees), §7.10 |

### 7.2 Action id → pose id (Cube.exe 0x4b4ef0..0x4b5342)

Each frame, before any pose work, the client copies the creature's action time (record
0x5c, object +0x6c) into the pose state (+0x04, object +0x149c) and derives a **pose id**
(pose state +0x08, object +0x14a0) from the action id (record 0x58, u8). The action id is
dispatched through a 110-entry table covering actions 1..110; action 0 and every action
above 110 fall to the default, pose 0. After the mapping, if record field 17 (offset 0x120,
signed 32-bit) is greater than 0 the pose id is overwritten with 56.

| Actions → pose | | | |
|---|---|---|---|
| 1→16 | 2→15 | 3→18 | 4→17 |
| 5→33 | 6→20 | 7→19 | 8→9 |
| 9→18 | 10→10 | 11→28 | 12→29 |
| 13→30 | 14→31 | 15→32 | 16, 17→35 |
| 18→17 | 19→18 | 20, 21→50 | 22, 23→21 |
| 24, 25→22 | 26→66 | 27→67 | 28→11 |
| 29→0 | 30–34→61 | 35→62 | 36→27 |
| 37→23 | 38→26 | 39→24 | 40→25 |
| 41→24 | 42→25 | 43→23 | 44–46→26 |
| 47→1 | 48→ see below | 49→61 | 50→22 |
| 51→70 | 52, 53→0 | 54→1 | 55→22 |
| 56→0 | 57→2 | 58→3 | 59→4 |
| 60→2 | 61→49 | 62→18 | 63→6 |
| 64→5 | 65→48 | 66→49 | 67→37 |
| 68→38 | 69→39 | 70→40 | 71, 72→41 |
| 73→42 | 74→43 | 75→44 | 76→45 |
| 77→46 | 78→47 | 79→54 | 80→51 |
| 81→52 | 82→53 | 83→57 | 84→55 |
| 85→0 | 86→60 | 87–90→61 | 91→63 |
| 92→61 | 93→ see below | 94, 95→26 | 96→65 |
| 97→0 | 98→8 | 99, 100→0 | 101→68 |
| 102→0 | 103→68 | 104→69 | 105→61 |
| 106→58 | 107→59 | 108→71 | 109→72 |
| 110→73 | 0 and ≥ 111 → 0 | | |

**Action 48** (0x4b51ca): if the main-hand item (equipment slot 7, object +0xaa8) has type 3
and subtype 6 or 7 (bow, crossbow), pose 22. Otherwise, if the off-hand item (slot 6,
object +0x990) has type 3: subtype 13 (shield) gives pose 9, any other subtype pose 35.
Otherwise pose 34.

**Action 93** (0x4b52dc): if the off-hand item type (object +0x990) is 0 (empty), pose 49 —
the same result as actions 61 and 66; otherwise pose 35.

The slot numbers follow §6.4; the two item records are 0x118 (280) bytes apart.

### 7.3 Walk-cycle state (simulation step, Cube.exe 0x61e9b5..0x61f0f1)

Per creature and per step of *dt* milliseconds (an integer; *dt*ₛ = *dt*·0.001):

* *speed²* = squared 3-D length of the velocity (record 0x24); *speed* its square root;
  *speed_xy* = length of the horizontal part of the velocity.
* *k* = sqrt(0.8 / *S*), where *S* is the creature's **collision box x** (appearance 0x08,
  object +0x80) (0x619137..0x619151, used at 0x61ea5b).
* "contact" = record 0x4c (bit 0 on ground, bit 1 in water, bit 2 touching a wall); "mode" =
  record 0x114; "flyer" = appearance flag 0x0002; "climbing" = mode bit 0x01 and contact
  bit 2.

**Riding** (tested first, 0x61e9b5). If the mount id (object +0x11c0, 64-bit) resolves to an
existing creature:

* if that creature's value at record offset 0x15c (health) is not greater than 0, the mount
  id is cleared to 0 and the normal walk-cycle step below runs;
* otherwise, if this creature's record byte 0x50 equals 5, the normal step runs (no riding);
* otherwise the creature rides: position := mount position + (0, 0, 1.5 − mount landing
  bob) blocks (converted to fixed point); velocity := 0; orientation := mount orientation
  + (20, 0, 0); movement intent (record 0x30) := 0; walk amplitude := mount amplitude × 0.5;
  walk phase := mount phase. The phase/amplitude increments below are skipped, but the
  equipment-flag checks, the amplitude clamp and all three relaxations at the end of this
  section still run.

If the mount id does not resolve to a creature, it is left unchanged and the normal step
runs.

**Normal step.** The creature is *eligible* when it is on ground, or a flyer, or climbing,
or in water.

* If eligible, *speed²* > 0.5 and the action id is not 107:
  * climbing: phase += *k*·*speed*·*dt*ₛ·6; amplitude += 0.2·*dt*ₛ (nothing else);
  * else on ground, in an attack wind-up — action 48 at any time; action 54 while the
    action time is below the action's full duration; actions 6, 7, 10, 17, 18, 19, 20 while
    the action time is below the action's first phase (`alpha-creatures-behaviour.md`
    §3.2): phase += *k*·(*dt*·0.0001·*speed_xy*·1.5); amplitude += *dt*ₛ;
  * else on ground: phase += *k*·(*speed_xy*·*dt*ₛ·1.5), doubled for action 79;
    amplitude += *dt*ₛ;
  * else (not on ground; water or air): phase += *k*·(*dt*·0.002·*speed_xy*·1.5);
    amplitude += *dt*ₛ.
* Otherwise (not eligible, or too slow, or action 107): if the creature is not on ground,
  not in water and not climbing, phase := 0 and amplitude += *k*·*dt*ₛ; else nothing changes.

**Footsteps** (0x61ed8f..0x61ef06), only in the two on-ground, non-climbing cases above,
and only when: the world has a local player creature (the reference is non-null;
*unconfirmed* whether this is the intended meaning), record field 15 (dodge time, 0x118) is
0, contact bit 1 (water) is clear, and *speed_xy* > 5. If trunc(old phase/π) <
trunc(new phase/π) (double-precision division, truncation), one sound event is emitted at
the **creature position**. The ground is probed at the creature position lowered by
0.5·box.z blocks and by a further 0.5 block (each converted to fixed point by ×65536 and
truncation); the block type there is its type byte AND 0x1F:

* type 3: one draw; sound id 33 + (draw mod 3), pitch 1;
* any other: sound id 32; one draw; pitch draw·0.2/32767 + 1.

Volume is 1 in both cases. Exactly one draw is made per footstep. What block type 3
represents is *unconfirmed*.

**After the increments** (0x61f008..0x61f0f1), every step, riding included:

1. Mode bit 0x10 (gliding) is cleared unless the item at object +0xf08 (equipment slot 11)
   has type 23 and subtype 0 (glider).
2. Mode bit 0x200 (lantern) is cleared unless the item at object +0xdf0 (slot 10) has
   type 24.
3. If amplitude > 1, amplitude := 1.
4. Amplitude relaxes toward 0 at rate 0.005 per ms.
5. Landing bob (object +0x1180) relaxes toward 0 at rate 0.01 per ms if it is negative,
   otherwise at rate 0.02.
6. Hit flash (object +0x1184) relaxes toward 0 at rate 0.0075 per ms.

"Relax *x* toward *t* at rate *r* per ms" (0x411d40) means: *dt* times in succession,
*x* ← *x* + (*t* − *x*)·*r*, each step in single precision.

### 7.4 From pose to matrices

All of this is per frame in 0x4128f0. Rotations, translations and scales are applied to
the current matrix in the order written (post-multiplied); angles are degrees.

1. **Swing.** *s₀* = sin(phase + π/2), where phase + π/2 is formed in double, rounded to
   single, and the sine is taken in double and rounded to single. *swing* =
   *a*·C·*s₀* in double, rounded to single, with C = 2.19911482006215 (0.7π rounded to
   single precision, stored as a double). Then, in this order: ×0.5 for action 79; ×0.1
   for actions 106 and 107; ×1.25 and the **running** flag set if (mode 0x40 set, not
   (mode 0x01 with wall contact), |intent|² > 0.1 (all three components) and
   |velocity_xy|² > 0.1) or the action is 48; set to 0 if appearance flag 0x0002 (flyer)
   and not on ground; ×0.75 if appearance flag 0x0001 (quadruped) — the value at this
   point is *s′*; ×0.25 if the main-slot subtype byte is 5 and the action is non-zero;
   ×0.1 for poses 51 and 52 while the action time is below the held item's use limit
   (§6.5), and ×0.1 for pose 53; ×0.5 for actions 22, 36, 63, 8, 28 or poses 2, 3, 48, 49,
   37; set to 0 if mode 0x10.
2. **Leg angles and body lean.** leg*L* = 1.5·(*s′*·180/π) and leg*R* = −leg*L* (the
   product *s′*·180 in single, the division by π in double). Body lean ℓ = 0. Overrides,
   in order: riding (mount id non-zero and record byte 0x50 ≠ 5) → both legs 20, ℓ = 10;
   poses 57, 52, 58, 59 → both legs 40, ℓ = 10; pose 55 → both legs −10.
3. **Hand base targets**, with *h* = appearance hand offset and *r* = *h*.x·(1 + *a*).
   Not quadruped: right *R* = (cos(swing)·*r*, *h*.y + sin(swing)·*r* + 2.1 − 5*a*,
   *h*.z − 4.1 + 6*a*); left *L* = (−cos(swing)·*r*, *h*.y − sin(swing)·*r* + 2.1 − 5*a*,
   *h*.z − 4.1 + 6*a*). Quadruped: *R* = (*h*.x, *h*.y + 2.1 − 5*a*, *h*.z) and
   *L* = (−*h*.x, *h*.y + 2.1 − 5*a*, *h*.z). Riding: both y += 2, both x × 0.5.
   Arm base targets (for the arm model): left = ((*L*.x − 0.3·*h*.x)/2, (*L*.y − 8)/4,
   (*L*.z + *h*.z + 12)/2), right = ((*R*.x + 0.3·*h*.x)/2, (*R*.y − 8)/4,
   (*R*.z + *h*.z + 12)/2).
4. **Foot base targets**: right = appearance foot offset, left = (−x, y, z). Tail target =
   appearance tail offset.
5. **Pose overrides**: the pose case (§7.8, Appendix C) replaces some targets and sets
   channel targets (§7.7).
6. **Smoothing.** The weight is *w* = 1 − (1 − *r*)^dt, obtained by relaxing 0 toward 1
   dt times at rate *r* (0x411d40; dt is the caller's frame time in ms, *r* = 0.01 unless
   the pose case sets another). Linear channels move by *p* ← *p* + (target − *p*)·*w*.
   Angle channels move by the shortest arc (0x423f70): with *a* and target *t* converted
   to radians, *d* = asin(clamp(sin *t*·cos *a* − cos *t*·sin *a*, −1, 1)), and
   *a* ← *a* + *d*·*w*·180/π.
7. **Base matrix** (0x41f147..0x41f5c7), in this order:
   * T(*X*, *Y*, *Z*) in blocks with *X* = (displayed x + caller x offset)/65536,
     *Y* = (displayed y + caller y offset)/65536 and *Z* = (displayed z − ⌊bob·65536⌋ +
     ⌊*E*.z·65536⌋)/65536, all sums in 64-bit fixed point (the conversions truncate toward
     zero). The two 64-bit horizontal offsets are passed in by the caller; no offset is
     added to z (*inferred*: the negated camera position).
   * **Dodge roll** when field 15 ≠ 0, unless class 4 specialisation 1 is performing
     action 17, 5 or 20 within its duration: axis = velocity × (0, 0, 1) = (*v*.y, −*v*.x, 0)
     (type 101: the velocity itself), applied only if the axis is longer than 0.01, by the
     angle (1 − field15/600)·(−360) about the normalised axis. In the same case (and also
     whenever mode 0x10 is set) the z of both composed hand positions *L* and *R* is raised
     by 5·box.z/box.x, whether or not the roll itself was applied; dodging while gliding
     raises them twice.
   * R*z*(*D*.z) · T(0, 0, *h*) · R*y*(*D*.y) · R*x*(*D*.x) · T(*E*.x, *E*.y, 0) ·
     R*z*(*B*.z) · R*y*(*B*.y) · R*x*(*B*.x) · T(0, 0, −*h*) · S(box.x/11.2).
   From here on lengths are model units (box 0.96 → one unit = 0.0857143 blocks). *h* is
   the smoothed pivot height whose target is box.z/2 while 0x444650 holds (mode 0x10,
   velocity z < 0, not on ground, field 16 < 1 and field 15 < 1) and 0 otherwise.
8. **Body matrix** = base · T(0, 0, bob) · T(*S*) · R*z*(*T*.z) · R*y*(*T*.y) ·
   R*x*(appearance body pitch + *T*.x), with bob = 4·|*s′*| (×4 once more for action 79).
   Each part is then drawn as parent frame · T(offsets) · rotations · S(scales) ·
   T(fraction of the model size), the fraction normally being one half on each axis.

### 7.5 Parts in drawing order

| # | Part (model source) | Frame | Condition |
|---|---|---|---|
| 1 | glider (slot 11) | body | mode 0x10 |
| 2 | boat (slot 11) | body | action 107 |
| 3 | mana cube (2566) | previous arm frame | pose 72, time < 5000 |
| 4 | body (slot 2, else appearance body) | body | model present, draw-flag bit 3 clear |
| 5 | plaster (2550) | body | with 4, field 20 > 0 |
| 6 | amulet (slot 1) | body | |
| 7 | shoulders (slot 5) | body | draw-flag bit 2 clear |
| 8 | tail (appearance) | body | |
| 9 | head | body | bit 0 clear |
| 10 | hair / eyes | body | bit 1 clear |
| 11 | arms: left, then right (appearance arm) | arm | |
| 12 | held item (field 43) | arm | actions 80, 81 within limit |
| 13 | pet item (slot 12) | arm | action 82 |
| 14 | back quiver for an off-hand bow/crossbow | body | |
| 15 | back quiver for a main-hand bow/crossbow | body | |
| 16 | main weapon (slot 7), hand or back | arm / body | bit 6 clear |
| 17 | off-hand weapon (slot 6), hand or back | arm / body | bit 6 clear |
| 18 | slot-6 quiver on the back | body | |
| 19 | left hand (slot 4, else appearance hand) | arm | bit 4 clear |
| 20 | lantern (slot 10) | arm | inside 19, mode 0x200 |
| 21 | right hand, mirrored | arm | bit 5 clear |
| 22 | wings: −x, then +x | arm | |
| 23 | left foot (slot 3, else appearance foot) | base | bit 7 clear |
| 24 | right foot | base | bit 8 clear |

The matrices of each part are in §6.5 (equipment) and §7.9.

### 7.6 Worked example: standing and walking human

Standing (amplitude 0, phase 0): *s₀* = 1, swing = 0, legs 0/0. Right hand target
(6, 2.1, −4.1), left (−6, 2.1, −4.1). Feet (3, 1, −10.5) and (−3, 1, −10.5). Body offset
(0, 0, −5), head offset (0, 0.5, 5), head scale 1.01. One model unit = 0.96/11.2 =
0.085714 blocks, so the head pivot is about 0.43 blocks above and the foot pivots about
0.90 blocks below the creature position.

Walking at amplitude 1, phase π/3: *s₀* = 0.5; swing = 1.0995574 rad; legs ±94.5°;
*r* = 12; right hand = (12·cos 1.0995574, 12·sin 1.0995574 + 2.1 − 5, −4.1 + 6) =
(5.448, 7.792, 1.9); left hand = (−5.448, −13.592, 1.9). These are base targets; the drawn
pose approaches them through the channels of §7.7.

### 7.7 Pose state, channels and the per-frame order

The pose function keeps per creature a **pose state** (obj +0x1498; offsets below are
pose-state offsets) holding the copied action time (+0x04), the pose id (+0x08), the
smoothed **channels**, the part model pointers (+0xbc..+0xe8) and the part matrices.
Pose cases set channel **targets**; the channels follow them (§7.4 step 6) and are then
added to the base targets.

| Channel | Symbol | Pose state | Kind | Used by |
|---|---|---|---|---|
| left-hand position offset | Δ*L* | +0x0c | linear | added to *L*; 0.4·Δ*L* to the left arm |
| right-hand position offset | Δ*R* | +0x18 | linear | added to *R*; 0.4·Δ*R* to the right arm |
| left-hand rotation offset | ρ*L* | +0x24 | linear | added to λ |
| right-hand rotation offset | ρ*R* | +0x30 | linear | added to ρ |
| torso angles | *T* | +0x54 | linear | body frame, arm frame, head counter-rotation |
| body tilt | *B* | +0x60 | angles | base frame; glider and boat |
| torso shift | *S* | +0x6c | linear | body and arm frames |
| head shift | *K* | +0x78 | linear | added to the head offset |
| head angles | *H* | +0x84 | angles | head and hair |
| tail angles | *Q* | +0x90 | angles | tail |
| shoulder roll | *U* | +0x9c | linear (only *U*.y read) | arm frame |
| aim yaw | *Y* | +0xa8 | linear scalar | chest, plaster, amulet (*Y*/2); feet (*Y*) |
| lean | *E* | +0xac | linear | base frame T(*E*.x, *E*.y, 0); *E*.z added to world z in blocks |
| pivot height | *h* | +0xb8 | linear scalar | base frame |

One weight *w* serves all channels in a frame. Rates seen in pose cases: 0.002, 0.005,
0.01, 0.02, 0.03, 0.04, 0.05, 0.1, 0.2.

Unsmoothed per-frame values: hand base targets *L*, *R*; arm, foot and tail base
targets; hand rotations λ (left) and ρ (right); leg*L*, leg*R*; a foot extra angle *F*
(starts at 0); the body lean ℓ; a **foot-turn flag** (starts off); grip rotations *W*
(main) and *W*′ (off hand), both starting at 0; and *r*.

**Order of work** (0x4128f0):

1. Part models: equipment models for slots 1–7, then the appearance model for each part
   left empty (an index that is negative as a signed 16-bit value or not below the table
   size gives no model). Swing *s* and *s′* (§7.4 step 1).
2. All channel targets := 0, except *h* := box.z/2 while 0x444650 holds. Aim yaw target:
   if mode 0x04, |intent_xy|² > 5 and |*v*_xy|² > 0 with *v* = record field 26: with
   *u* = intent_xy and *v* = *v*_xy both normalised, *Y* = asin(clamp(*v*.x·*u*.y −
   *v*.y·*u*.x, −1, 1))·180/π, negated when *u*·*v* < −0.2; otherwise 0.
3. Base targets (§7.4 steps 2–4). Hand rotations: pose 0 → ρ = appearance hand rotation
   (x, y, z) and λ = (x, −y, −z); any other pose → both 0.
4. A slot-6 quiver (type 3, subtype 14) is taken out of the off hand (§6.5).
5. **Sitting** (mode 0x400): *B* := (−70, 0, 0); *H*.x := 70; Δ*L* := (0, 3, 1); Δ*R* :=
   (0, 3, 1), with Δ*R*.y := 11 when the main-slot subtype byte is 7; *E*.z :=
   (box.y − box.z)/2; *K* := (0, −1, 0); leg*L* = leg*R* := −20. If an action is running
   (id ≠ 0 and time below its duration) Δ*L* and Δ*R* each get + (0, −5, 6).
6. The **pose case** (§7.8, Appendix C); it may overwrite any of the above.
7. If the pose id is 0, the copied action time := 0.
8. Arm targets: left arm := ½(left arm + *L*), right arm := ½(right arm + *R*).
9. **Knocked down** (field 16 > 0): not quadruped → *B* := (90, 0, 0), *E*.z :=
   (box.y − box.z)/2; quadruped → *B* := (0, 90, 0), *E*.z := (box.x − box.z)/2.
10. **Climbing** (mode 0x01 and wall contact): Δ*L*.z += 7, Δ*R*.z += 7, ρ*L*.x += 90,
    ρ*R*.x += 90.
11. **Torch in the off hand** (slot-6 subtype byte 20): ρ*L*.x := 0, Δ*L*.y += 4,
    Δ*L*.z += 4.
12. **Running lean** (not quadruped, action not 106/107): if the running flag of §7.4
    step 1 is set: *B*.x −= 25, *H*.x += 10; otherwise, if |velocity_xy|² > 0.1 and
    intent·velocity > 0 (three components): *B*.x −= 10, *H*.x += 5.
13. Weight *w* (§7.4 step 6), then smoothing in this order: ρ*R*, ρ*L*, Δ*R*, Δ*L*, *T*, *E*,
    *h*, *B*, *H*, *Q*, *Y*, *U*, *S*, *K*. During action 28 a further vector (pose state
    +0x86c) moves toward record field 26 normalised (left unnormalised when its length is
    not above 0) with its own weight at rate 0.01; for any other action it is set to 0. No
    reader of it was found in this routine.
14. Composition: ρ += ρ*R*, λ += ρ*L*, *R* += Δ*R*, *L* += Δ*L*, right arm += 0.4·Δ*R*,
    left arm += 0.4·Δ*L*.
15. Draw flags (caller argument): bit 0 removes the head, 1 the hair, 3 the body, 2 the
    shoulders, 6 both weapons; bits 4, 5, 7, 8 skip the left hand (with the lantern), right
    hand, left foot and right foot. Then the matrices and draws of §7.4 steps 7–8, §6.5
    and §7.9, in the order of §7.5.

### 7.8 Action poses: how a pose case works

Evidence: `Cube.exe` (SHA-256 `84a7a132…30bf`), pose dispatch at 0x413ae9 (pose id − 1 bounded
to 0..72, jump table of 73 entries at 0x423d4c), cases 0x413afd..0x41e660, shared write-back
tails at 0x41e64e / 0x41e653 / 0x41e65a.

Each frame the pose id held in pose state +0x08 selects at most one case. Poses 1..73 have a
table entry; poses 12, 13, 14, 36 and 64 point at the common exit (no case) and poses 44 and
73 share one case. Any other id (0, or above 73) takes no case.

A case reads the copied action time *t* (pose state +0x04, ms; a few cases read the record's
action time, which holds the same value this frame), the phase lengths of the current action,
the equipment, some record and object fields, and its own constants. The phase lengths come
from four per-action routines: P1 (0x43caa0), P2 (0x447310), P3 (0x444270) and the action
duration D (0x43d1a0); they are integers in ms (see `alpha-creatures-behaviour.md` §3.3).
Halved or thirded lengths ("P1/2", "P2/3") are integer divisions truncating toward zero.

Phase names used below:

| Name | Condition |
|---|---|
| W | *t* < P1 |
| A | P1 ≤ *t* < P1 + P2 |
| R | P1 + P2 ≤ *t* < D |
| E | *t* ≥ D |

Many cases use other boundaries (P1/2, P1 + P2/2, P1 + 300, 2·D, fixed millisecond values);
these are stated in the case. Branches are tested in the order written and the first that
matches is taken; a phase not mentioned writes nothing. Progress values are written
explicitly; "clamp" means limited to [0, 1] and "min(…, 1)" limits only from above.
lerp(*x*, *y*, *q*) = (1 − *q*)·*x* + *q*·*y*.

A case writes some of: channel targets of §7.7 (":=" replaces the value, "+=" / "−=" adjust
the value present at that moment, which is the §7.7 default unless an earlier step of the
same frame changed it), base targets (*L*, *R* hands, left/right arm bases, left/right foot
bases, head base = appearance head offset, tail base = tail offset), the smoothing rate *r*,
the foot extra angle *F*, the body lean ℓ, the foot-turn flag ("turn"), the pose id, and the
part model pointers of the pose state (+0xbc head, +0xcc hand, +0xd4 tail, +0xd8 foot,
+0xdc main-hand weapon, +0xe0 off-hand weapon). A case that sets the pose id to 0 also causes
the copied action time to be cleared after the switch (§7.7 step 7).

Arithmetic: products and sums are single precision. Every sine or cosine is evaluated in
double precision on a single-precision argument and rounded back to single; where a
constant π or π/2 is added to the argument, the addition is made in double and the sum is
rounded to single before the call.

Shared blocks:

* **Grip G(*g*)**: *L* := *R* + *g* (the right-hand base target as it stands at that moment,
  plus *g*). "G(*g*) with arm lift" additionally adds 10 to the left arm base y and subtracts
  4 from its z. Whether the arm lift is applied is stated per case.
* **STANCE** (end branch of poses 10, 15, 16, 17, 18, 29 and 35; common code at
  0x41952a..0x419748 with per-pose entry stubs): the pose id := 0 and *U* := 0. Then, except
  in pose 29, if the creature type (record 0x54) is 87 or 96 nothing further is written.
  Otherwise *T*.z := 20; ρ*R* := (−120, −90, 30); ρ*L* := (−80, 40, 60) — in poses 10 and 18,
  when the off-hand slot's subtype byte is 13, ρ*L* := (0, 0, 90) instead; Δ*L*.y −= 4,
  Δ*L*.z += 2; and if the main-hand model pointer is set and the off-hand model pointer is
  not: *R* += (0, 2, 3), G((−3, 0, −3)) with arm lift, ρ*R*.x := 0, ρ*R*.y := 20 (ρ*R*.z stays
  30), ρ*L* := ρ*R*. (Each entry stub first writes ρ*L*.x := −90, or −60 in poses 16 and 35;
  that value is always overwritten by the ρ*L* assignment above.)
* **Two-handed test** (0x444230): true when the main-hand slot (slot 7) holds item type 3 with
  subtype 5, 6, 7, 8, 10, 11, 15, 16, 17 or 18 (longsword, bow, crossbow, boomerang, staff,
  wand, greatsword, greataxe, greatmace, pitchfork).
* **Hand wobble**: "wobble (z)" adds 0.5·cos(0.5·*t*) to the z of both hand base targets
  *R* and *L*. "Wobble (Δ)" adds 0.5·sin(0.04·*t*) to Δ*R*.x and Δ*L*.x and 0.5·cos(0.05·*t*)
  to Δ*R*.z and Δ*L*.z.
* **Guard idle** (poses 19, 20 outside their active window, pose 50 after its active phase):
  *T*.z := 0; *B*.z := 0; ρ*R* := (60, −20, 20); Δ*R* := (0, 2 + sin(0.01·*t*),
  4 + cos(0.01·*t*)); ρ*L* := (60, 20, −20); Δ*L* := (0, 2 + sin(0.01·*t* + π/2),
  4 + cos(0.01·*t* + π/2)); *S*.z += 0.5·sin(*k*·*t*) with *k* = 0.005 (poses 19, 50) or 0.01
  (pose 20); *r* := 0.03 in poses 19 and 20 (pose 50 leaves *r* at its default).
* **Spin counter**: some sword cases read the object counter at obj +0x1314 (meaning
  *unresolved*) and use *c* = (counter / 2) mod 3, both operations with C integer semantics.

The arm-frame head-pitch tilt of §7.9.2 is omitted while the pose id is 2, 3 or 37 (test at
0x420ad7..0x420ae7, made after the case has run; the end branches of these cases set the pose
id to 0, so the tilt is present in those branches).

### 7.9 Part transforms in full

Frames (model units after the base scale S(box.x/11.2)):

* **Base frame** = §7.4 step 7.
* **Body frame** = §7.4 step 8: base · T(0, 0, bob) · T(*S*) · R*z*(*T*.z) · R*y*(*T*.y) ·
  R*x*(body pitch + *T*.x).

#### 7.9.1 Body-attached parts

Glider, boat, chest, plaster, amulet and shoulders: §6.5. **Tail**: body frame ·
T(body offset) · T(tail target) · R*x*(appearance tail angle) · R*z*(−*s*·180/π) ·
R*z*(*Q*.z) · R*y*(*Q*.y) · R*x*(*Q*.x) · S(tail scale) · S(*b*) · T(−*sx*/2, −*sy*, −*sz*/2):
the tail's root is its +y face.

#### 7.9.2 Arm frame

Arm frame = base · T(0, 0, bob) · T(*S*) · [R*x*(2·*p*) unless (mode 0x01 and wall contact)
or the pose is 2, 3 or 37] · [R*x*(60) while mode 0x400] · T(0, 0, −4) · R*y*(*U*.y) ·
T(0, 0, 4) · R*z*(*T*.z) · R*y*(*T*.y) · R*x*(*T*.x). The body pitch is **not** applied.
It is stored in the pose state (+0x12c). Arms, hands, held and pet items, both in-hand
weapons, the lantern and the wings hang from it.

#### 7.9.3 Head and hair

Body frame · T(head offset + *K* + (0, 0, −bob/2)) · T(0, 0, −3) · R*x*(−*T*.x − body
pitch) · R*y*(−*T*.y) · R*z*(*H*.z − *T*.z) · R*y*(*H*.y) · R*x*(*p* − *D*.x + *H*.x) ·
T(0, 0, 3) · S(head scale) · centred, except that type 101 uses −0.2·*sy* for the y
fraction. The head cancels the torso's x and y turn and half its bob, and pitches by the
look pitch less the displayed body pitch. The hair layer uses the same matrix and the
material of §5.2.

#### 7.9.4 Arms and hands

**Arms** (appearance arm model, not mirrored; left first): arm frame · T(arm target) ·
S(arm scale) · centred.
**Left hand** (−x): arm frame · [quadruped: R*x*(leg*R*)] · T(*L*) · R*z*(λ.z) · R*y*(λ.y) ·
[quadruped: R*x*(leg*R*)] · R*x*(λ.x) · S(*n*) · centred.
**Right hand** (+x): the same with *R*, ρ and leg*L*, scaled by (−*n*, *n*, *n*), i.e.
mirrored in x; for this one draw render state 22 is set to 3 and afterwards back to 2.
Glove models are placed the same way.

#### 7.9.5 Feet

From the **base** frame: base · R*z*(*Y*) · R*x*(*a*₁) · R*y*(±ℓ) ·
T(foot target + (0, 0, 3·|*s′*| + lift)) · [action 79: R*x*(−1.5·leg)] · S(foot scale) ·
[left foot with the foot-turn flag: R*z*(60); otherwise R*x*(*a*₂)] · centred.

| | *a*₁ | *a*₂ | R*y* | lift | leg |
|---|---|---|---|---|---|
| left foot (−x), drawn first | foot angle + *F* + leg*L* | *F* + leg*L* | +ℓ | 4·*a*·*s₀* | leg*L* |
| right foot (+x) | *F* + leg*R* | foot angle + *F* + leg*R* | −ℓ | 4·*a*·(1 − *s₀*) | leg*R* |

Both lifts are 0 for a flyer off the ground. The foot-turn flag affects only the left foot.
Quadrupeds use this rule for their hind legs; their front legs are the hands.

#### 7.9.6 Wings

Arm frame · T(∓wing offset.x, wing offset.y, wing offset.z) · R*x*(wing angle) · R*y*(±*f*) ·
R*z*(−90) · R*x*(90) · S(wing scale) · T(−*sx*/2, 0, −*sz*/2), with
*f* = cos(phase/2)·*a*·90 − 45 (cosine of a single-precision argument, rounded to single);
the −x wing is drawn first with +*f*, then the +x wing with −*f*.

### 7.10 Displayed position and body rotation (Cube.exe 0x61f968..0x61fd54)

Besides the record position (record field 0, object +0x10, 3 × i64 fixed point, 65536 per
block) and orientation (record field 1, object +0x28, degrees), every creature keeps a
**displayed position** (object +0x1350, 3 × i64) and **displayed body rotation** *D*
(object +0x1374, 3 × f32 degrees). Each step one of three rules applies.

**Attach rule** (checked first, 0x61f968). It applies when this creature's record byte 0x50
equals 5, the creature id at record offset 0x188 (object +0x198) resolves to an existing
creature, and that creature's action id is 106. With *z*₁ = this creature's box z and
*z*₂ = the other's box z, offset = (0, 0, 0.5·*z*₁ − 0.5·*z*₂ + 0.01) blocks (fixed point):

* displayed position := other displayed position + offset; *D* := other *D*;
* record position := other record position + offset; orientation := other orientation;
* landing bob, walk amplitude and walk phase are copied from the other creature;
* velocity := 0; movement intent := 0.

The smoothing rules below are then skipped. What this attachment represents is
*unconfirmed*.

**Smoothing** (object +0x1398 > 0, signed; 0x61fb80): *w* is obtained by relaxing 0 toward 1
at rate 0.015 per ms over *dt* (so *w* = 1 − 0.985^*dt* up to float rounding). Then:

1. *wf* = trunc(*w*·65536). Each displayed-position component *p* moves by
   trunc((*rec* − *p*)·*wf* / 65536) (64-bit product, truncating division).
2. The z component moves once more: *p*.z += trunc(float(*rec*.z − *p*.z)·*w*), using the
   already updated *p*.z.
3. Each component of *D* moves toward the matching orientation component by
   *D* ← *D* + asin(clamp(sin(target − *D*), −1, 1))·*w* (degrees in and out; the sine
   difference is formed from the sines and cosines of the two angles).
4. Landing bob := 0.

What sets or clears object +0x1398 is *unconfirmed* (zeroed by 0x43b690, 0x446330,
0x447110).

**Otherwise**: displayed position := record position; *D* := orientation.

*D* is what the base frame rotates by (§7.4 step 7) and what the head pitch subtracts
(§7.9.3).

### 7.11 Worked examples

1. **Smoothing weight.** dt = 16 ms: *r* = 0.01 → *w* = 1 − 0.99¹⁶ = 0.148542;
   *r* = 0.03 → 0.385746; *r* = 0.005 → 0.077069; *r* = 0.05 → 0.559873.
2. **Greatsword wind-up (pose 2, action 57)** — the pose-case values are those of
   Appendix C: P1 = 800 ms, *t* = 200: *q* = min(1.5·200/800, 1) = 0.375; ρ*R* target
   (78.75, 0, 78.75); Δ*R* = Δ*L* target (0, 0, 5.25); *r* = 0.01. From rest one 16 ms frame
   moves ρ*R*.x to 78.75·0.148542 = 11.6977 and Δ*R*.z to 0.779847, so the drawn right hand
   of a standing human is (6, 2.1, −3.320153); fully settled it would be (6, 2.1, 1.15),
   and the left hand, based on *R* + (2, 0, 0), (8, 2.1, 1.15).
3. **Sword in the hand.** A model 5 × 5 × 18 voxels, weapon scale 0.95: grip
   (−2.5, −2.5, −4), so voxel point (2.5, 2.5, 4) sits at the right hand's origin and the
   model extends 14·0.95 = 13.3 model units along the hand's +z.
4. **Feet while walking** (amplitude 1, phase π/3, human): *s′* = 1.0995574 rad,
   leg*L* = 94.5°, leg*R* = −94.5°, *s₀* = 0.5. Left foot *a*₁ = *a*₂ = 94.5°, right foot
   *a*₁ = *a*₂ = −94.5°; both z targets −10.5 + 3·1.0995574 + 2 = −5.2013278.
5. **Dodge roll.** Field 15 = 450: angle (1 − 450/600)·(−360) = −90° about
   (*v*.y, −*v*.x, 0) normalised; velocity (3, 4, 0) → axis (0.8, −0.6, 0).
6. **Displayed rotation** (rate from §7.10). *D*.z = 350°, target 10°, dt = 16:
   *w*ᴰ = 1 − 0.985¹⁶ = 0.214801; *D*.z ← 350 + 20·0.214801 = 354.29602°.
7. **Wing flap.** Amplitude 1, phase π: *f* = cos(π/2)·90 − 45 = −45°: the −x wing turns
   R*y*(−45), the +x wing R*y*(45).
8. **Lantern.** Idle human (left hand (−6, 2.1, −4.1)): the lantern frame origin is the
   arm-frame point (−6, 4.1, −6.1), followed by S(0.8) and T(−*sx*/2, 1 − *sy*/2, 2 − *sz*).
9. **Item models.** Type 11 subtype 0 material 11 → 2339 `gold-nugget`; type 17 seed 6 →
   2113 + 2 = 2115 `vase3`; type 18 subtype 1 seed 7 → 2120 + 1 = 2121; type 14 rarity 0
   → 2318, rarity 3 → 2320, rarity 4 → 2321; type 21 subtype 10 → 2081 + 2 = 2083
   `quest-item-jewel-case`; type 23 subtype 1 → 2548 `boat2`, subtype 0 → 2547 `glider`.

---

## 8. Unresolved

Statements below are either open or rest on inference; the rest of the document does not
depend on them for its numeric content.

* **U-1** Meaning of appearance bytes 0x00–0x01 (never written by the generator).
* **U-2** Meanings of appearance flags 0x0008, 0x0020, 0x0800, 0x1000 and 0x2000; the
  natural-weapon effect of 0x0010 (described in `alpha-creatures-behaviour.md`, not
  re-verified here); 0x0100 as "swimmer" (*inferred*); what the type list of 0x444680
  selects (§3.3).
* **U-3** Whether any routine reads the key-colour point lists at model object +0x04,
  +0x10, +0x1c (§5.1).
* **U-4** Behavioural identity of the server generator (**server** 0x40a840) with the
  client one beyond equal size; the server's new-player defaults (box (1.2, 1.2, 2.7) and
  type 44, from `alpha-net-packets.md`) are not re-verified here.
* **U-5** That every creature's appearance passes through the constructor before the
  generator runs (§3.2, *inferred* from the callers).
* **U-6** Item types of equipment slots 0, 1, 8, 9, 10 and 12 (*inferred* from the models
  drawn); which foot belongs to draw-flag bits 0x080 and 0x100 (§6.4).
* **U-7** That each pet-food subtype names the creature type that eats it; the material
  names (derived from model names); the default item level (§6.1, §6.3.1).
* **U-8** What the two caller-supplied 64-bit horizontal offsets of the base translation
  are (*inferred*: the negated camera position) (§7.4 step 7).
* **U-9** The time window of the weapon-trail history and how trails are rendered (§6.5).
* **U-10** What sets record field 20 (0x12c, plaster) and field 23 (0x134, weapon glow and
  pose 67); both labels are *inferred*.
* **U-11** What sets or clears obj +0x1398 (displayed-position smoothing); the meaning of
  record byte 0x50 = 5 (riding and attach exceptions); what the attach rule represents;
  footstep block type 3; the meaning of the footstep "local player present" gate (§7.3,
  §7.10).
* **U-12** Meanings of obj +0x1314 (spin counter), obj +0x13b4 (pose 66 branch) and the
  record vectors at 0x138 and 0x144 (shoulder roll sources); whether D (0x43d1a0) always
  equals P1 + P2 + P3; whether pose 7 is reachable (no action maps to it) (§7.8,
  Appendix C).
* **U-13** Contact bit 1 as "in water" and record field 26 (0x150) as "aim direction" are
  *inferred* from use; record field 17 (0x120) forcing pose 56 is labelled "stunned" by
  inference.
* **U-14** The mana-cube matrix starts from the arm frame stored by the previous call
  (*as read*, §6.5).

---

## Appendix A. Model table (slot: key without `.cub`, ten per row; "—" = empty slot)

```
   0: body2, body4, body3, wizard-head, wizard-body, witch-head, witch-body, glove, head2, girl-head3
  10: elf-head-female, old-head, gnoll-head, gnoll-body, gnoll-hand, gnoll-foot, polar-gnoll-head, polar-gnoll-body, polar-gnoll-hand, polar-gnoll-foot
  20: monkey-head, monkey-body, monkey-hand, monkey-foot, troll-head, troll-body, troll-arm, troll-hand, troll-foot, dark-troll-eyes
  30: dark-troll-head, dark-troll-body, dark-troll-arm, dark-troll-hand, dark-troll-foot, hell-demon-head, hell-demon-body, hell-demon-arm, hell-demon-hand, hell-demon-foot
  40: golem-head, golem-body, golem-arm, golem-hand, golem-foot, golem-ember-head, golem-ember-body, golem-ember-arm, golem-ember-hand, golem-ember-foot
  50: golem-snow-head, golem-snow-body, golem-snow-arm, golem-snow-hand, golem-snow-foot, yeti-head, yeti-body, yeti-arm, yeti-hand, yeti-foot
  60: ogre-head, ogre-body, ogre-hand, ogre-foot, rockling-head, rockling-hand, rockling-foot, cyclops-head, cyclops-body, cyclops-arm
  70: cyclops-hand, cyclops-foot, mammoth-head, mammoth-body, mammoth-foot, goblin-head-m01, goblin-head-m02, goblin-head-m03, goblin-head-m04, goblin-head-m05
  80: goblin-hair-m01, goblin-hair-m02, goblin-hair-m03, goblin-hair-m04, goblin-hair-m05, goblin-hair-m06, goblin-head-f01, goblin-head-f02, goblin-head-f03, goblin-head-f04
  90: goblin-head-f05, goblin-hair-f01, goblin-hair-f02, goblin-hair-f03, goblin-hair-f04, goblin-hair-f05, goblin-hair-f06, goblin-hand, lizard-head-m01, lizard-head-m02
 100: lizard-hair-m01, lizard-hair-m02, lizard-hair-m03, lizard-hair-m04, lizard-hair-m05, lizard-hair-m06, lizard-head-f01, lizard-head-f02, lizard-head-f03, lizard-head-f04
 110: lizard-head-f05, lizard-hand, lizard-body, lizard-foot, frog-body, frog-head, frog-foot, frog-hand, plant-creature-body, plant-creature-head
 120: plant-creature-foot, plant-creature-hand, radish-creature-head, radish-creature-foot, radish-creature-hand, onionling-head, onionling-foot, desert-onionling, desert-onionling-foot, devourer-head
 130: devourer-foot, mole-body, mole-head, mole-foot, mole-hand, biter-body, biter-head, biter-foot, biter-hand, koala-body
 140: koala-head, koala-foot, koala-hand, squirrel-body, squirrel-head, squirrel-foot, squirrel-hand, raccoon-body, raccoon-head, raccoon-foot
 150: raccoon-hand, owl-head, owl-foot, owl-hand, bunny-body, bunny-foot, porcupine-body, porcupine-foot, squid-body, squid-foot
 160: spike-head, spike-foot, spike-body, spike-hand, anubis-head, anubis-foot, anubis-body, anubis-hand, horus-head, horus-foot
 170: horus-body, horus-hand, jester-head, jester-foot, jester-body, jester-hand, spectrino-head, spectrino-foot, spectrino-body, spectrino-hand
 180: desert-nomad-head1, desert-nomad-head2, desert-nomad-head3, desert-nomad-female-head1, desert-nomad-female-head2, desert-nomad-female-head3, desert-nomad-foot, desert-nomad-body, desert-nomad-hand, desert-nomad-hand2
 190: djinn-head, djinn-foot, djinn-body, djinn-hand, minotaur-head, minotaur-foot, minotaur-body, minotaur-hand, minotaur-arm, imp-eyes
 200: imp-head, imp-foot, imp-body, imp-hand, penguin-body, penguin-head, penguin-foot, penguin-hand, crab-body, crab-head
 210: crab-foot, crab-hand, crab-body-blue, crab-head-blue, crab-foot-blue, crab-hand-blue, barkbeetle-head, barkbeetle-foot, firebeetle-head, firebeetle-foot
 220: snoutbeetle-head, snoutbeetle-foot, lemonbeetle-head, lemonbeetle-foot, santa-body, santa-head, zombie-body, zombie-head, foot, zombie-hand
 230: hornet-body, hornet-head, hornet-foot, hornet-hand, insect-guard-body, insect-guard-head, insect-guard-foot, insect-guard-hand, insect-guard-wing, fly-body
 240: fly-head, fly-foot, fly-hand, bumblebee, bumblebee-hand, bumblebee-foot, midge-body, midge-head, midge-foot, midge-hand
 250: mosquito-body, mosquito-head, mosquito-foot, mosquito-hand, crow-body, crow-head, crow-foot, crow-hand, chicken-body, chicken-head
 260: chicken-foot, chicken-hand, seagull-body, seagull-head, seagull-foot, seagull-hand, parrot-body, parrot-head, parrot-foot, parrot-hand
 270: parrot-blue-body, parrot-blue-head, parrot-blue-foot, parrot-blue-hand, bat-body, bat-head, bat-foot, bat-hand, lich-body, lich-head
 280: lich-hand, lich-arm, dwarf-head-m01, dwarf-head-m02, dwarf-head-m03, dwarf-head-m04, dwarf-head-m05, dwarf-hair-m01, dwarf-hair-m02, dwarf-hair-m03
 290: dwarf-head-f01, dwarf-head-f02, dwarf-head-f03, dwarf-head-f04, dwarf-head-f05, dwarf-hair-f01, dwarf-hair-f02, dwarf-hair-f03, dwarf-hair-f04, dwarf-hair-f05
 300: dwarf-body, dwarf-body-female, orc-hand, undead-head-m01, undead-head-m02, undead-head-m03, undead-head-m04, undead-head-m05, undead-head-m06, undead-hair-m01
 310: undead-hair-m02, undead-hair-m03, undead-hair-m04, undead-hair-m05, undead-hair-m06, undead-head-f01, undead-head-f02, undead-head-f03, undead-head-f04, undead-head-f05
 320: undead-head-f06, undead-hair-f01, undead-hair-f02, undead-hair-f03, undead-hair-f04, undead-hair-f05, undead-hair-f06, undead-hand, skeleton-head, skeleton-eyes
 330: skeleton-hand, skeleton-body, skeleton-foot, plainrunner-body, plainrunner-foot, leafrunner-body, leafrunner-foot, snowrunner-body, snowrunner-foot, desertrunner-body
 340: desertrunner-foot, peacock-body, peacock-foot, peacock-head, pony-body, pony-head, pony-foot, pony-tail, camel-body, camel-head
 350: camel-foot, cow-body, cow-head, cow-foot, collie-body, collie-head, collie-hand, collie-foot, collie-tail, shepherd-dog-body
 360: shepherd-dog-head, shepherd-dog-hand, shepherd-dog-foot, shepherd-dog-tail, skull-bull-body, skull-bull-head, skull-bull-hand, skull-bull-foot, skull-bull-tail, alpaca-body
 370: alpaca-head, alpaca-hand, alpaca-foot, alpaca-brown-body, alpaca-brown-head, alpaca-brown-hand, alpaca-brown-foot, dog-body2, dog-head2, dog-hand2
 380: dog-foot2, dog-tail2, scottish-terrier-body, scottish-terrier-head, scottish-terrier-hand, scottish-terrier-foot, scottish-terrier-tail, wolf-body, wolf-head, wolf-hand
 390: wolf-foot, wolf-tail, panther-body, panther-head, panther-hand, panther-foot, panther-tail, cat-body, cat-head, cat-hand
 400: cat-foot, cat-tail, cat-body2, cat-head2, cat-hand2, cat-foot2, cat-tail2, cat-body3, cat-head3, cat-hand3
 410: cat-foot3, cat-tail3, pig-body, pig-head, pig-foot, sheep-body, sheep-head, sheep-foot, duckbill-body, duckbill-head
 420: duckbill-foot, duckbill-tail, crocodile-body, crocodile-head, crocodile-foot, dragon-body, dragon-head, dragon-foot, dragon-wing, dragon-tail
 430: hand2, brown-hand, foot, boot, wood-staff1, wood-staff2, wood-staff3, wood-staff4, wood-staff6, wood-staff1-random1
 440: wood-staff2-random1, wood-staff3-random1, wood-staff4-random1, wood-staff5-random1, wood-staff1-random2, wood-staff2-random2, wood-staff3-random2, wood-staff4-random2, wood-staff5-random2, wood-staff1-random3
 450: wood-staff2-random3, wood-staff3-random3, wood-staff4-random3, wood-staff5-random3, wood-staff1-random4, wood-staff2-random4, wood-staff3-random4, wood-staff4-random4, wood-staff5-random4, wood-staff1-random5
 460: wood-staff2-random5, wood-staff3-random5, wood-staff4-random5, wood-staff5-random5, wood-staff1-random6, wood-staff2-random6, wood-staff3-random6, wood-staff4-random6, wood-staff5-random6, wood-staff1-random7
 470: wood-staff2-random7, wood-staff3-random7, wood-staff4-random7, wood-staff5-random7, wood-staff1-random8, wood-staff2-random8, wood-staff3-random8, wood-staff4-random8, wood-staff5-random8, wood-staff1-random9
 480: wood-staff2-random9, wood-staff3-random9, wood-staff4-random9, wood-staff5-random9, wood-staff1-random10, wood-staff2-random10, wood-staff3-random10, wood-staff4-random10, wood-staff5-random10, wood-wand1
 490: wood-wand2, wood-wand3, wood-wand4, wood-wand5, wood-wand1-random1, wood-wand2-random1, wood-wand3-random1, wood-wand4-random1, wood-wand5-random1, wood-wand1-random2
 500: wood-wand2-random2, wood-wand3-random2, wood-wand4-random2, wood-wand5-random2, wood-wand1-random3, wood-wand2-random3, wood-wand3-random3, wood-wand4-random3, wood-wand5-random3, wood-wand1-random4
 510: wood-wand2-random4, wood-wand3-random4, wood-wand4-random4, wood-wand5-random4, wood-wand1-random5, wood-wand2-random5, wood-wand3-random5, wood-wand4-random5, wood-wand5-random5, wood-wand1-random6
 520: wood-wand2-random6, wood-wand3-random6, wood-wand4-random6, wood-wand5-random6, wood-wand1-random7, wood-wand2-random7, wood-wand3-random7, wood-wand4-random7, wood-wand5-random7, wood-wand1-random8
 530: wood-wand2-random8, wood-wand3-random8, wood-wand4-random8, wood-wand5-random8, wood-wand1-random9, wood-wand2-random9, wood-wand3-random9, wood-wand4-random9, wood-wand5-random9, wood-wand1-random10
 540: wood-wand2-random10, wood-wand3-random10, wood-wand4-random10, wood-wand5-random10, gold-bracelet1, gold-bracelet2, gold-bracelet3, gold-bracelet4, gold-bracelet5, gold-bracelet1-random1
 550: gold-bracelet2-random1, gold-bracelet3-random1, gold-bracelet4-random1, gold-bracelet5-random1, gold-bracelet1-random2, gold-bracelet2-random2, gold-bracelet3-random2, gold-bracelet4-random2, gold-bracelet5-random2, gold-bracelet1-random3
 560: gold-bracelet2-random3, gold-bracelet3-random3, gold-bracelet4-random3, gold-bracelet5-random3, gold-bracelet1-random4, gold-bracelet2-random4, gold-bracelet3-random4, gold-bracelet4-random4, gold-bracelet5-random4, gold-bracelet1-random5
 570: gold-bracelet2-random5, gold-bracelet3-random5, gold-bracelet4-random5, gold-bracelet5-random5, silver-bracelet1, silver-bracelet2, silver-bracelet3, silver-bracelet4, silver-bracelet5, silver-bracelet1-random1
 580: silver-bracelet2-random1, silver-bracelet3-random1, silver-bracelet4-random1, silver-bracelet5-random1, silver-bracelet1-random2, silver-bracelet2-random2, silver-bracelet3-random2, silver-bracelet4-random2, silver-bracelet5-random2, silver-bracelet1-random3
 590: silver-bracelet2-random3, silver-bracelet3-random3, silver-bracelet4-random3, silver-bracelet5-random3, silver-bracelet1-random4, silver-bracelet2-random4, silver-bracelet3-random4, silver-bracelet4-random4, silver-bracelet5-random4, silver-bracelet1-random5
 600: silver-bracelet2-random5, silver-bracelet3-random5, silver-bracelet4-random5, silver-bracelet5-random5, obsidian-staff1, obsidian-staff2, obsidian-staff3, obsidian-staff4, obsidian-staff5, iron-mace1
 610: iron-mace2, iron-mace3, iron-mace4, iron-mace5, iron-mace1-random1, iron-mace2-random1, iron-mace3-random1, iron-mace4-random1, iron-mace5-random1, iron-mace1-random2
 620: iron-mace2-random2, iron-mace3-random2, iron-mace4-random2, iron-mace5-random2, iron-mace1-random3, iron-mace2-random3, iron-mace3-random3, iron-mace4-random3, iron-mace5-random3, iron-mace1-random4
 630: iron-mace2-random4, iron-mace3-random4, iron-mace4-random4, iron-mace5-random4, iron-mace1-random5, iron-mace2-random5, iron-mace3-random5, iron-mace4-random5, iron-mace5-random5, iron-mace1-random6
 640: iron-mace2-random6, iron-mace3-random6, iron-mace4-random6, iron-mace5-random6, iron-mace1-random7, iron-mace2-random7, iron-mace3-random7, iron-mace4-random7, iron-mace5-random7, iron-mace1-random8
 650: iron-mace2-random8, iron-mace3-random8, iron-mace4-random8, iron-mace5-random8, iron-mace1-random9, iron-mace2-random9, iron-mace3-random9, iron-mace4-random9, iron-mace5-random9, iron-mace1-random10
 660: iron-mace2-random10, iron-mace3-random10, iron-mace4-random10, iron-mace5-random10, wood-mace01, wood-mace02, wood-mace03, wood-mace04, wood-mace05, light-helmet
 670: light-chest, light-shoulder, light-glove, light-boot, bow, wood-bow2, wood-bow3, wood-bow4, wood-bow5, wood-bow1-random1
 680: wood-bow2-random1, wood-bow3-random1, wood-bow4-random1, wood-bow5-random1, wood-bow1-random2, wood-bow2-random2, wood-bow3-random2, wood-bow4-random2, wood-bow5-random2, wood-bow1-random3
 690: wood-bow2-random3, wood-bow3-random3, wood-bow4-random3, wood-bow5-random3, wood-bow1-random4, wood-bow2-random4, wood-bow3-random4, wood-bow4-random4, wood-bow5-random4, wood-bow1-random5
 700: wood-bow2-random5, wood-bow3-random5, wood-bow4-random5, wood-bow5-random5, wood-bow1-random6, wood-bow2-random6, wood-bow3-random6, wood-bow4-random6, wood-bow5-random6, wood-bow1-random7
 710: wood-bow2-random7, wood-bow3-random7, wood-bow4-random7, wood-bow5-random7, wood-bow1-random8, wood-bow2-random8, wood-bow3-random8, wood-bow4-random8, wood-bow5-random8, wood-bow1-random9
 720: wood-bow2-random9, wood-bow3-random9, wood-bow4-random9, wood-bow5-random9, wood-bow1-random10, wood-bow2-random10, wood-bow3-random10, wood-bow4-random10, wood-bow5-random10, wood-crossbow1
 730: wood-crossbow2, wood-crossbow3, wood-crossbow4, wood-crossbow5, wood-crossbow1-random1, wood-crossbow2-random1, wood-crossbow3-random1, wood-crossbow4-random1, wood-crossbow5-random1, wood-crossbow1-random2
 740: wood-crossbow2-random2, wood-crossbow3-random2, wood-crossbow4-random2, wood-crossbow5-random2, wood-crossbow1-random3, wood-crossbow2-random3, wood-crossbow3-random3, wood-crossbow4-random3, wood-crossbow5-random3, wood-crossbow1-random4
 750: wood-crossbow2-random4, wood-crossbow3-random4, wood-crossbow4-random4, wood-crossbow5-random4, wood-crossbow1-random5, wood-crossbow2-random5, wood-crossbow3-random5, wood-crossbow4-random5, wood-crossbow5-random5, wood-crossbow1-random6
 760: wood-crossbow2-random6, wood-crossbow3-random6, wood-crossbow4-random6, wood-crossbow5-random6, wood-crossbow1-random7, wood-crossbow2-random7, wood-crossbow3-random7, wood-crossbow4-random7, wood-crossbow5-random7, wood-crossbow1-random8
 770: wood-crossbow2-random8, wood-crossbow3-random8, wood-crossbow4-random8, wood-crossbow5-random8, wood-crossbow1-random9, wood-crossbow2-random9, wood-crossbow3-random9, wood-crossbow4-random9, wood-crossbow5-random9, wood-crossbow1-random10
 780: wood-crossbow2-random10, wood-crossbow3-random10, wood-crossbow4-random10, wood-crossbow5-random10, wood-boomerang1, wood-boomerang2, wood-boomerang3, wood-boomerang4, wood-boomerang5, wood-boomerang1-random1
 790: wood-boomerang2-random1, wood-boomerang3-random1, wood-boomerang4-random1, wood-boomerang5-random1, wood-boomerang1-random2, wood-boomerang2-random2, wood-boomerang3-random2, wood-boomerang4-random2, wood-boomerang5-random2, wood-boomerang1-random3
 800: wood-boomerang2-random3, wood-boomerang3-random3, wood-boomerang4-random3, wood-boomerang5-random3, wood-boomerang1-random4, wood-boomerang2-random4, wood-boomerang3-random4, wood-boomerang4-random4, wood-boomerang5-random4, wood-boomerang1-random5
 810: wood-boomerang2-random5, wood-boomerang3-random5, wood-boomerang4-random5, wood-boomerang5-random5, wood-boomerang1-random6, wood-boomerang2-random6, wood-boomerang3-random6, wood-boomerang4-random6, wood-boomerang5-random6, wood-boomerang1-random7
 820: wood-boomerang2-random7, wood-boomerang3-random7, wood-boomerang4-random7, wood-boomerang5-random7, wood-boomerang1-random8, wood-boomerang2-random8, wood-boomerang3-random8, wood-boomerang4-random8, wood-boomerang5-random8, wood-boomerang1-random9
 830: wood-boomerang2-random9, wood-boomerang3-random9, wood-boomerang4-random9, wood-boomerang5-random9, wood-boomerang1-random10, wood-boomerang2-random10, wood-boomerang3-random10, wood-boomerang4-random10, wood-boomerang5-random10, quiver
 840: arrow, fireball, torch, mushroom, shimmer-mushroom, heartflower, heartflower-frozen, prickly-pear, soulflower, life-potion
 850: cactus-potion, mana-potion, cookie, jelly-green, jelly-pink, jelly-blue, jelly-yellow, carrot, pumpkin-mash, candy
 860: lollipop, softice, donut-chocolate, cotton-candy, popcorn, cereal-bar, strawberry-cake, chocolate-cake, vanilla-cupcake, chocolate-cupcake
 870: banana-split, croissant, lolly, lemon-tart, chocolate-cookie, bubble-gum, licorice-candy, cinnamon-role, apple-ring, waffle
 880: water-ice, date-cookie, candied-apple, strawberry-cocktail, milk-chocolate-bar, caramel-chocolate-bar, mint-chocolate-bar, white-chocolate-bar, sugar-candy, blackberry-marmelade
 890: salted-caramel, ginger-tartlet, mango-juice, fruit-basket, melon-icecream, bloodorange-juice, pancakes, curry, biscuit-role, iron-sword1
 900: iron-sword2, iron-sword3, iron-sword4, iron-sword5, iron-sword1-random1, iron-sword2-random1, iron-sword3-random1, iron-sword4-random1, iron-sword5-random1, iron-sword1-random2
 910: iron-sword2-random2, iron-sword3-random2, iron-sword4-random2, iron-sword5-random2, iron-sword1-random3, iron-sword2-random3, iron-sword3-random3, iron-sword4-random3, iron-sword5-random3, iron-sword1-random4
 920: iron-sword2-random4, iron-sword3-random4, iron-sword4-random4, iron-sword5-random4, iron-sword1-random5, iron-sword2-random5, iron-sword3-random5, iron-sword4-random5, iron-sword5-random5, iron-sword1-random6
 930: iron-sword2-random6, iron-sword3-random6, iron-sword4-random6, iron-sword5-random6, iron-sword1-random7, iron-sword2-random7, iron-sword3-random7, iron-sword4-random7, iron-sword5-random7, iron-sword1-random8
 940: iron-sword2-random8, iron-sword3-random8, iron-sword4-random8, iron-sword5-random8, iron-sword1-random9, iron-sword2-random9, iron-sword3-random9, iron-sword4-random9, iron-sword5-random9, iron-sword1-random10
 950: iron-sword2-random10, iron-sword3-random10, iron-sword4-random10, iron-sword5-random10, iron-dagger1, iron-dagger2, iron-dagger3, iron-dagger4, iron-dagger5, iron-dagger1-random1
 960: iron-dagger2-random1, iron-dagger3-random1, iron-dagger4-random1, iron-dagger5-random1, iron-dagger1-random2, iron-dagger2-random2, iron-dagger3-random2, iron-dagger4-random2, iron-dagger5-random2, iron-dagger1-random3
 970: iron-dagger2-random3, iron-dagger3-random3, iron-dagger4-random3, iron-dagger5-random3, iron-dagger1-random4, iron-dagger2-random4, iron-dagger3-random4, iron-dagger4-random4, iron-dagger5-random4, iron-dagger1-random5
 980: iron-dagger2-random5, iron-dagger3-random5, iron-dagger4-random5, iron-dagger5-random5, iron-dagger1-random6, iron-dagger2-random6, iron-dagger3-random6, iron-dagger4-random6, iron-dagger5-random6, iron-dagger1-random7
 990: iron-dagger2-random7, iron-dagger3-random7, iron-dagger4-random7, iron-dagger5-random7, iron-dagger1-random8, iron-dagger2-random8, iron-dagger3-random8, iron-dagger4-random8, iron-dagger5-random8, iron-dagger1-random9
1000: iron-dagger2-random9, iron-dagger3-random9, iron-dagger4-random9, iron-dagger5-random9, iron-dagger1-random10, iron-dagger2-random10, iron-dagger3-random10, iron-dagger4-random10, iron-dagger5-random10, iron-fist1
1010: iron-fist2, iron-fist3, iron-fist4, iron-fist5, iron-fist1-random1, iron-fist2-random1, iron-fist3-random1, iron-fist4-random1, iron-fist5-random1, iron-fist1-random2
1020: iron-fist2-random2, iron-fist3-random2, iron-fist4-random2, iron-fist5-random2, iron-fist1-random3, iron-fist2-random3, iron-fist3-random3, iron-fist4-random3, iron-fist5-random3, iron-fist1-random4
1030: iron-fist2-random4, iron-fist3-random4, iron-fist4-random4, iron-fist5-random4, iron-fist1-random5, iron-fist2-random5, iron-fist3-random5, iron-fist4-random5, iron-fist5-random5, iron-fist1-random6
1040: iron-fist2-random6, iron-fist3-random6, iron-fist4-random6, iron-fist5-random6, iron-fist1-random7, iron-fist2-random7, iron-fist3-random7, iron-fist4-random7, iron-fist5-random7, iron-fist1-random8
1050: iron-fist2-random8, iron-fist3-random8, iron-fist4-random8, iron-fist5-random8, iron-fist1-random9, iron-fist2-random9, iron-fist3-random9, iron-fist4-random9, iron-fist5-random9, iron-fist1-random10
1060: iron-fist2-random10, iron-fist3-random10, iron-fist4-random10, iron-fist5-random10, iron-shield01, iron-shield02, iron-shield03, iron-shield04, iron-shield05, iron-shield1-random1
1070: iron-shield2-random1, iron-shield3-random1, iron-shield4-random1, iron-shield5-random1, iron-shield1-random2, iron-shield2-random2, iron-shield3-random2, iron-shield4-random2, iron-shield5-random2, iron-shield1-random3
1080: iron-shield2-random3, iron-shield3-random3, iron-shield4-random3, iron-shield5-random3, iron-shield1-random4, iron-shield2-random4, iron-shield3-random4, iron-shield4-random4, iron-shield5-random4, iron-shield1-random5
1090: iron-shield2-random5, iron-shield3-random5, iron-shield4-random5, iron-shield5-random5, iron-shield1-random6, iron-shield2-random6, iron-shield3-random6, iron-shield4-random6, iron-shield5-random6, iron-shield1-random7
1100: iron-shield2-random7, iron-shield3-random7, iron-shield4-random7, iron-shield5-random7, iron-shield1-random8, iron-shield2-random8, iron-shield3-random8, iron-shield4-random8, iron-shield5-random8, iron-shield1-random9
1110: iron-shield2-random9, iron-shield3-random9, iron-shield4-random9, iron-shield5-random9, iron-shield1-random10, iron-shield2-random10, iron-shield3-random10, iron-shield4-random10, iron-shield5-random10, wood-shield01
1120: wood-shield02, wood-shield03, wood-shield04, wood-shield05, iron-chest1, iron-chest2, iron-chest3, iron-chest4, iron-chest5, iron-chest1-random1
1130: iron-chest2-random1, iron-chest3-random1, iron-chest4-random1, iron-chest5-random1, iron-chest1-random2, iron-chest2-random2, iron-chest3-random2, iron-chest4-random2, iron-chest5-random2, iron-chest1-random3
1140: iron-chest2-random3, iron-chest3-random3, iron-chest4-random3, iron-chest5-random3, iron-chest1-random4, iron-chest2-random4, iron-chest3-random4, iron-chest4-random4, iron-chest5-random4, iron-shoulder1
1150: iron-shoulder2, iron-shoulder3, iron-shoulder4, iron-shoulder5, iron-shoulder1-random1, iron-shoulder2-random1, iron-shoulder3-random1, iron-shoulder4-random1, iron-shoulder5-random1, iron-shoulder1-random2
1160: iron-shoulder2-random2, iron-shoulder3-random2, iron-shoulder4-random2, iron-shoulder5-random2, iron-shoulder1-random3, iron-shoulder2-random3, iron-shoulder3-random3, iron-shoulder4-random3, iron-shoulder5-random3, iron-shoulder1-random4
1170: iron-shoulder2-random4, iron-shoulder3-random4, iron-shoulder4-random4, iron-shoulder5-random4, iron-hand1, iron-hand2, iron-hand3, iron-hand4, iron-hand5, iron-hand1-random1
1180: iron-hand2-random1, iron-hand3-random1, iron-hand4-random1, iron-hand5-random1, iron-hand1-random2, iron-hand2-random2, iron-hand3-random2, iron-hand4-random2, iron-hand5-random2, iron-hand1-random3
1190: iron-hand2-random3, iron-hand3-random3, iron-hand4-random3, iron-hand5-random3, iron-hand1-random4, iron-hand2-random4, iron-hand3-random4, iron-hand4-random4, iron-hand5-random4, iron-foot1
1200: iron-foot2, iron-foot3, iron-foot4, iron-foot5, iron-foot1-random1, iron-foot2-random1, iron-foot3-random1, iron-foot4-random1, iron-foot5-random1, iron-foot1-random2
1210: iron-foot2-random2, iron-foot3-random2, iron-foot4-random2, iron-foot5-random2, iron-foot1-random3, iron-foot2-random3, iron-foot3-random3, iron-foot4-random3, iron-foot5-random3, iron-foot1-random4
1220: iron-foot2-random4, iron-foot3-random4, iron-foot4-random4, iron-foot5-random4, saurian-chest, saurian-shoulder, saurian-glove, saurian-boot, saurian-helmet, innkeeper-chest
1230: backpack, body1, body2, body3, body4, body5, elf-head-m01, elf-head-m02, elf-head-m03, elf-head-m04
1240: elf-head-f01, elf-head-f02, elf-head-f03, elf-head-f04, elf-head-f05, elf-head-f06, human-head-m01, human-head-m02, human-head-m03, human-head-m04
1250: human-head-m05, human-head-m06, human-hair-m01, human-hair-m02, human-hair-m03, human-hair-m04, human-hair-m05, human-hair-m06, human-hair-m07, human-hair-m08
1260: human-hair-m09, human-hair-m10, human-hair-m11, human-hair-m12, human-hair-m13, human-hair-m14, human-hair-m15, human-head-f01, human-head-f02, human-head-f03
1270: human-head-f04, human-head-f05, human-head-f06, human-hair-f01, human-hair-f02, human-hair-f03, human-hair-f04, human-hair-f05, human-hair-f06, human-hair-f07
1280: elf-hair-m01, elf-hair-m02, elf-hair-m03, elf-hair-m04, elf-hair-m05, elf-hair-m06, elf-hair-m07, elf-hair-m08, elf-hair-m09, elf-hair-m10
1290: elf-hair-f01, elf-hair-f02, elf-hair-f03, elf-hair-f04, elf-hair-f05, elf-hair-f06, elf-hair-f07, elf-hair-f08, elf-hair-f09, elf-hair-f10
1300: orc-head, orc-head-m02, orc-head-m03, orc-head-m04, orc-head-m05, orc-head-f01, orc-head-f02, orc-head-f03, orc-head-f04, orc-head-f05
1310: orc-hair-m01, orc-hair-m02, orc-hair-m03, orc-hair-m04, orc-hair-m05, orc-hair-m06, orc-hair-m07, orc-hair-m08, orc-hair-m09, orc-hair-m10
1320: orc-hair-f01, orc-hair-f02, orc-hair-f03, orc-hair-f04, frogman-head-m01, frogman-head-m02, frogman-head-m03, frogman-head-m04, frogman-head-m05, frogman-hair-m01
1330: frogman-hair-m02, frogman-hair-m03, frogman-hair-m04, frogman-hair-m05, frogman-head-f01, frogman-head-f02, frogman-head-f03, frogman-head-f04, frogman-hair-f01, frogman-hair-f02
1340: frogman-hair-f03, frogman-hair-f04, frogman-hand, mermaid-head-f01, mermaid-head-f02, mermaid-head-f03, mermaid-hair-f01, mermaid-hair-f02, mermaid-hair-f03, mermaid-body
1350: mermaid-hand, merman-head-m01, merman-head-m02, merman-head-m03, merman-hair-m01, merman-hair-m02, merman-hair-m03, merman-body, mermaid-hand, linen-chest1
1360: linen-chest2, linen-chest3, linen-chest4, linen-chest5, linen-chest1-random1, linen-chest2-random1, linen-chest3-random1, linen-chest4-random1, linen-chest5-random1, linen-chest1-random2
1370: linen-chest2-random2, linen-chest3-random2, linen-chest4-random2, linen-chest5-random2, linen-chest1-random3, linen-chest2-random3, linen-chest3-random3, linen-chest4-random3, linen-chest5-random3, linen-chest1-random4
1380: linen-chest2-random4, linen-chest3-random4, linen-chest4-random4, linen-chest5-random4, linen-shoulder1, linen-shoulder2, linen-shoulder3, linen-shoulder4, linen-shoulder5, linen-shoulder1-random1
1390: linen-shoulder2-random1, linen-shoulder3-random1, linen-shoulder4-random1, linen-shoulder5-random1, linen-shoulder1-random2, linen-shoulder2-random2, linen-shoulder3-random2, linen-shoulder4-random2, linen-shoulder5-random2, linen-shoulder1-random3
1400: linen-shoulder2-random3, linen-shoulder3-random3, linen-shoulder4-random3, linen-shoulder5-random3, linen-shoulder1-random4, linen-shoulder2-random4, linen-shoulder3-random4, linen-shoulder4-random4, linen-shoulder5-random4, linen-foot1
1410: linen-foot2, linen-foot3, linen-foot4, linen-foot5, linen-foot1-random1, linen-foot2-random1, linen-foot3-random1, linen-foot4-random1, linen-foot5-random1, linen-foot1-random2
1420: linen-foot2-random2, linen-foot3-random2, linen-foot4-random2, linen-foot5-random2, linen-foot1-random3, linen-foot2-random3, linen-foot3-random3, linen-foot4-random3, linen-foot5-random3, linen-foot1-random4
1430: linen-foot2-random4, linen-foot3-random4, linen-foot4-random4, linen-foot5-random4, linen-hand1, linen-hand2, linen-hand3, linen-hand4, linen-hand5, linen-hand1-random1
1440: linen-hand2-random1, linen-hand3-random1, linen-hand4-random1, linen-hand5-random1, linen-hand1-random2, linen-hand2-random2, linen-hand3-random2, linen-hand4-random2, linen-hand5-random2, linen-hand1-random3
1450: linen-hand2-random3, linen-hand3-random3, linen-hand4-random3, linen-hand5-random3, linen-hand1-random4, linen-hand2-random4, linen-hand3-random4, linen-hand4-random4, linen-hand5-random4, wool-chest1
1460: wool-chest2, wool-chest3, wool-chest4, wool-chest5, wool-chest1-random1, wool-chest2-random1, wool-chest3-random1, wool-chest4-random1, wool-chest5-random1, wool-chest1-random2
1470: wool-chest2-random2, wool-chest3-random2, wool-chest4-random2, wool-chest5-random2, wool-chest1-random3, wool-chest2-random3, wool-chest3-random3, wool-chest4-random3, wool-chest5-random3, wool-chest1-random4
1480: wool-chest2-random4, wool-chest3-random4, wool-chest4-random4, wool-chest5-random4, wool-shoulder1, wool-shoulder2, wool-shoulder3, wool-shoulder4, wool-shoulder5, wool-shoulder1-random1
1490: wool-shoulder2-random1, wool-shoulder3-random1, wool-shoulder4-random1, wool-shoulder5-random1, wool-shoulder1-random2, wool-shoulder2-random2, wool-shoulder3-random2, wool-shoulder4-random2, wool-shoulder5-random2, wool-shoulder1-random3
1500: wool-shoulder2-random3, wool-shoulder3-random3, wool-shoulder4-random3, wool-shoulder5-random3, wool-shoulder1-random4, wool-shoulder2-random4, wool-shoulder3-random4, wool-shoulder4-random4, wool-shoulder5-random4, wool-foot1
1510: wool-foot2, wool-foot3, wool-foot4, wool-foot5, wool-foot1-random1, wool-foot2-random1, wool-foot3-random1, wool-foot4-random1, wool-foot5-random1, wool-foot1-random2
1520: wool-foot2-random2, wool-foot3-random2, wool-foot4-random2, wool-foot5-random2, wool-foot1-random3, wool-foot2-random3, wool-foot3-random3, wool-foot4-random3, wool-foot5-random3, wool-foot1-random4
1530: wool-foot2-random4, wool-foot3-random4, wool-foot4-random4, wool-foot5-random4, wool-hand1, wool-hand2, wool-hand3, wool-hand4, wool-hand5, wool-hand1-random1
1540: wool-hand2-random1, wool-hand3-random1, wool-hand4-random1, wool-hand5-random1, wool-hand1-random2, wool-hand2-random2, wool-hand3-random2, wool-hand4-random2, wool-hand5-random2, wool-hand1-random3
1550: wool-hand2-random3, wool-hand3-random3, wool-hand4-random3, wool-hand5-random3, wool-hand1-random4, wool-hand2-random4, wool-hand3-random4, wool-hand4-random4, wool-hand5-random4, —
1560: —, —, —, —, gold-amulet1-random1, gold-amulet2-random1, gold-amulet3-random1, gold-amulet4-random1, gold-amulet5-random1, gold-amulet1-random2
1570: gold-amulet2-random2, gold-amulet3-random2, gold-amulet4-random2, gold-amulet5-random2, gold-amulet1-random3, gold-amulet2-random3, gold-amulet3-random3, gold-amulet4-random3, gold-amulet5-random3, gold-amulet1-random4
1580: gold-amulet2-random4, gold-amulet3-random4, gold-amulet4-random4, gold-amulet5-random4, gold-amulet1-random5, gold-amulet2-random5, gold-amulet3-random5, gold-amulet4-random5, gold-amulet5-random5, —
1590: —, —, —, —, silver-amulet1-random1, silver-amulet2-random1, silver-amulet3-random1, silver-amulet4-random1, silver-amulet5-random1, silver-amulet1-random2
1600: silver-amulet2-random2, silver-amulet3-random2, silver-amulet4-random2, silver-amulet5-random2, silver-amulet1-random3, silver-amulet2-random3, silver-amulet3-random3, silver-amulet4-random3, silver-amulet5-random3, silver-amulet1-random4
1610: silver-amulet2-random4, silver-amulet3-random4, silver-amulet4-random4, silver-amulet5-random4, silver-amulet1-random5, silver-amulet2-random5, silver-amulet3-random5, silver-amulet4-random5, silver-amulet5-random5, gold-ring1-random1
1620: gold-ring2-random1, gold-ring3-random1, gold-ring4-random1, gold-ring5-random1, gold-ring1-random2, gold-ring2-random2, gold-ring3-random2, gold-ring4-random2, gold-ring5-random2, gold-ring1-random3
1630: gold-ring2-random3, gold-ring3-random3, gold-ring4-random3, gold-ring5-random3, gold-ring1-random4, gold-ring2-random4, gold-ring3-random4, gold-ring4-random4, gold-ring5-random4, gold-ring1-random5
1640: gold-ring2-random5, gold-ring3-random5, gold-ring4-random5, gold-ring5-random5, silver-ring1-random1, silver-ring2-random1, silver-ring3-random1, silver-ring4-random1, silver-ring5-random1, silver-ring1-random2
1650: silver-ring2-random2, silver-ring3-random2, silver-ring4-random2, silver-ring5-random2, silver-ring1-random3, silver-ring2-random3, silver-ring3-random3, silver-ring4-random3, silver-ring5-random3, silver-ring1-random4
1660: silver-ring2-random4, silver-ring3-random4, silver-ring4-random4, silver-ring5-random4, silver-ring1-random5, silver-ring2-random5, silver-ring3-random5, silver-ring4-random5, silver-ring5-random5, silk-chest1
1670: silk-chest2, silk-chest3, silk-chest4, silk-chest5, silk-chest1-random1, silk-chest2-random1, silk-chest3-random1, silk-chest4-random1, silk-chest5-random1, silk-chest1-random2
1680: silk-chest2-random2, silk-chest3-random2, silk-chest4-random2, silk-chest5-random2, silk-chest1-random3, silk-chest2-random3, silk-chest3-random3, silk-chest4-random3, silk-chest5-random3, silk-chest1-random4
1690: silk-chest2-random4, silk-chest3-random4, silk-chest4-random4, silk-chest5-random4, silk-shoulder1, silk-shoulder2, silk-shoulder3, silk-shoulder4, silk-shoulder5, silk-shoulder1-random1
1700: silk-shoulder2-random1, silk-shoulder3-random1, silk-shoulder4-random1, silk-shoulder5-random1, silk-shoulder1-random2, silk-shoulder2-random2, silk-shoulder3-random2, silk-shoulder4-random2, silk-shoulder5-random2, silk-shoulder1-random3
1710: silk-shoulder2-random3, silk-shoulder3-random3, silk-shoulder4-random3, silk-shoulder5-random3, silk-shoulder1-random4, silk-shoulder2-random4, silk-shoulder3-random4, silk-shoulder4-random4, silk-shoulder5-random4, silk-foot1
1720: silk-foot2, silk-foot3, silk-foot4, silk-foot5, silk-foot1-random1, silk-foot2-random1, silk-foot3-random1, silk-foot4-random1, silk-foot5-random1, silk-foot1-random2
1730: silk-foot2-random2, silk-foot3-random2, silk-foot4-random2, silk-foot5-random2, silk-foot1-random3, silk-foot2-random3, silk-foot3-random3, silk-foot4-random3, silk-foot5-random3, silk-foot1-random4
1740: silk-foot2-random4, silk-foot3-random4, silk-foot4-random4, silk-foot5-random4, silk-hand1, silk-hand2, silk-hand3, silk-hand4, silk-hand5, silk-hand1-random1
1750: silk-hand2-random1, silk-hand3-random1, silk-hand4-random1, silk-hand5-random1, silk-hand1-random2, silk-hand2-random2, silk-hand3-random2, silk-hand4-random2, silk-hand5-random2, silk-hand1-random3
1760: silk-hand2-random3, silk-hand3-random3, silk-hand4-random3, silk-hand5-random3, silk-hand1-random4, silk-hand2-random4, silk-hand3-random4, silk-hand4-random4, silk-hand5-random4, iron-helmet
1770: iron-axe1, iron-axe2, iron-axe3, iron-axe4, iron-axe5, iron-axe1-random1, iron-axe2-random1, iron-axe3-random1, iron-axe4-random1, iron-axe5-random1
1780: iron-axe1-random2, iron-axe2-random2, iron-axe3-random2, iron-axe4-random2, iron-axe5-random2, iron-axe1-random3, iron-axe2-random3, iron-axe3-random3, iron-axe4-random3, iron-axe5-random3
1790: iron-axe1-random4, iron-axe2-random4, iron-axe3-random4, iron-axe4-random4, iron-axe5-random4, iron-axe1-random5, iron-axe2-random5, iron-axe3-random5, iron-axe4-random5, iron-axe5-random5
1800: iron-axe1-random6, iron-axe2-random6, iron-axe3-random6, iron-axe4-random6, iron-axe5-random6, iron-axe1-random7, iron-axe2-random7, iron-axe3-random7, iron-axe4-random7, iron-axe5-random7
1810: iron-axe1-random8, iron-axe2-random8, iron-axe3-random8, iron-axe4-random8, iron-axe5-random8, iron-axe1-random9, iron-axe2-random9, iron-axe3-random9, iron-axe4-random9, iron-axe5-random9
1820: iron-axe1-random10, iron-axe2-random10, iron-axe3-random10, iron-axe4-random10, iron-axe5-random10, iron-greatsword1, iron-greatsword2, iron-greatsword3, iron-greatsword4, iron-greatsword5
1830: iron-greatsword1-random1, iron-greatsword2-random1, iron-greatsword3-random1, iron-greatsword4-random1, iron-greatsword5-random1, iron-greatsword1-random2, iron-greatsword2-random2, iron-greatsword3-random2, iron-greatsword4-random2, iron-greatsword5-random2
1840: iron-greatsword1-random3, iron-greatsword2-random3, iron-greatsword3-random3, iron-greatsword4-random3, iron-greatsword5-random3, iron-greatsword1-random4, iron-greatsword2-random4, iron-greatsword3-random4, iron-greatsword4-random4, iron-greatsword5-random4
1850: iron-greatsword1-random5, iron-greatsword2-random5, iron-greatsword3-random5, iron-greatsword4-random5, iron-greatsword5-random5, iron-greatsword1-random6, iron-greatsword2-random6, iron-greatsword3-random6, iron-greatsword4-random6, iron-greatsword5-random6
1860: iron-greatsword1-random7, iron-greatsword2-random7, iron-greatsword3-random7, iron-greatsword4-random7, iron-greatsword5-random7, iron-greatsword1-random8, iron-greatsword2-random8, iron-greatsword3-random8, iron-greatsword4-random8, iron-greatsword5-random8
1870: iron-greatsword1-random9, iron-greatsword2-random9, iron-greatsword3-random9, iron-greatsword4-random9, iron-greatsword5-random9, iron-greatsword1-random10, iron-greatsword2-random10, iron-greatsword3-random10, iron-greatsword4-random10, iron-greatsword5-random10
1880: iron-longsword1, iron-longsword2, iron-longsword3, iron-longsword4, iron-longsword5, iron-longsword1-random1, iron-longsword2-random1, iron-longsword3-random1, iron-longsword4-random1, iron-longsword5-random1
1890: iron-longsword1-random2, iron-longsword2-random2, iron-longsword3-random2, iron-longsword4-random2, iron-longsword5-random2, iron-longsword1-random3, iron-longsword2-random3, iron-longsword3-random3, iron-longsword4-random3, iron-longsword5-random3
1900: iron-longsword1-random4, iron-longsword2-random4, iron-longsword3-random4, iron-longsword4-random4, iron-longsword5-random4, iron-longsword1-random5, iron-longsword2-random5, iron-longsword3-random5, iron-longsword4-random5, iron-longsword5-random5
1910: iron-longsword1-random6, iron-longsword2-random6, iron-longsword3-random6, iron-longsword4-random6, iron-longsword5-random6, iron-longsword1-random7, iron-longsword2-random7, iron-longsword3-random7, iron-longsword4-random7, iron-longsword5-random7
1920: iron-longsword1-random8, iron-longsword2-random8, iron-longsword3-random8, iron-longsword4-random8, iron-longsword5-random8, iron-longsword1-random9, iron-longsword2-random9, iron-longsword3-random9, iron-longsword4-random9, iron-longsword5-random9
1930: iron-longsword1-random10, iron-longsword2-random10, iron-longsword3-random10, iron-longsword4-random10, iron-longsword5-random10, bone-greatsword, obsidian-greatsword, iron-greataxe1, iron-greataxe2, iron-greataxe3
1940: iron-greataxe4, iron-greataxe5, iron-greataxe1-random1, iron-greataxe2-random1, iron-greataxe3-random1, iron-greataxe4-random1, iron-greataxe5-random1, iron-greataxe1-random2, iron-greataxe2-random2, iron-greataxe3-random2
1950: iron-greataxe4-random2, iron-greataxe5-random2, iron-greataxe1-random3, iron-greataxe2-random3, iron-greataxe3-random3, iron-greataxe4-random3, iron-greataxe5-random3, iron-greataxe1-random4, iron-greataxe2-random4, iron-greataxe3-random4
1960: iron-greataxe4-random4, iron-greataxe5-random4, iron-greataxe1-random5, iron-greataxe2-random5, iron-greataxe3-random5, iron-greataxe4-random5, iron-greataxe5-random5, iron-greataxe1-random6, iron-greataxe2-random6, iron-greataxe3-random6
1970: iron-greataxe4-random6, iron-greataxe5-random6, iron-greataxe1-random7, iron-greataxe2-random7, iron-greataxe3-random7, iron-greataxe4-random7, iron-greataxe5-random7, iron-greataxe1-random8, iron-greataxe2-random8, iron-greataxe3-random8
1980: iron-greataxe4-random8, iron-greataxe5-random8, iron-greataxe1-random9, iron-greataxe2-random9, iron-greataxe3-random9, iron-greataxe4-random9, iron-greataxe5-random9, iron-greataxe1-random10, iron-greataxe2-random10, iron-greataxe3-random10
1990: iron-greataxe4-random10, iron-greataxe5-random10, bone-greataxe, obsidian-greataxe, saurian-greataxe, wood-greatmace02, iron-greatmace1, iron-greatmace2, iron-greatmace3, iron-greatmace4
2000: iron-greatmace5, iron-greatmace1-random1, iron-greatmace2-random1, iron-greatmace3-random1, iron-greatmace4-random1, iron-greatmace5-random1, iron-greatmace1-random2, iron-greatmace2-random2, iron-greatmace3-random2, iron-greatmace4-random2
2010: iron-greatmace5-random2, iron-greatmace1-random3, iron-greatmace2-random3, iron-greatmace3-random3, iron-greatmace4-random3, iron-greatmace5-random3, iron-greatmace1-random4, iron-greatmace2-random4, iron-greatmace3-random4, iron-greatmace4-random4
2020: iron-greatmace5-random4, iron-greatmace1-random5, iron-greatmace2-random5, iron-greatmace3-random5, iron-greatmace4-random5, iron-greatmace5-random5, iron-greatmace1-random6, iron-greatmace2-random6, iron-greatmace3-random6, iron-greatmace4-random6
2030: iron-greatmace5-random6, iron-greatmace1-random7, iron-greatmace2-random7, iron-greatmace3-random7, iron-greatmace4-random7, iron-greatmace5-random7, iron-greatmace1-random8, iron-greatmace2-random8, iron-greatmace3-random8, iron-greatmace4-random8
2040: iron-greatmace5-random8, iron-greatmace1-random9, iron-greatmace2-random9, iron-greatmace3-random9, iron-greatmace4-random9, iron-greatmace5-random9, iron-greatmace1-random10, iron-greatmace2-random10, iron-greatmace3-random10, iron-greatmace4-random10
2050: iron-greatmace5-random10, iron-pickaxe, bone-greatmace, obsidian-greatmace, obsidian-chest, obsidian-shoulders, obsidian-boot, obsidian-glove, obsidian-helmet, obsidian-sword
2060: —, —, gold-chest, gold-shoulder, gold-boot, gold-glove, gold-helmet, bone-chest, bone-shoulders, bone-boot
2070: bone-glove, —, bone-sword, bone-mace, bone-axe, cube, door, window, goddess2, artifact
2080: pet-box, quest-item-amulet01, quest-item-amulet02, quest-item-jewel-case, copper-coin, silver-coin, gold-coin, bush, snow-bush, cobwebscrub
2090: berry-bush, snow-berry, snow-berry-mash, scrub, scrub-green, fire-scrub, ginseng, ginseng-root, fir-tree, thorn-tree
2100: gold-deposit, iron-deposit, silver-deposit, sandstone-deposit, emerald-deposit, sapphire-deposit, ruby-deposit, diamond-deposit, ice-crystal-deposit, scarecrow
2110: aim, dummy, tree-leaves, vase, vase2, vase3, vase4, candle01, candle02, candle03
2120: undead-candle1, undead-candle2, undead-candle3, character-platform, antique-building1, antique-building2, antique-building3, antique-building4, entrance-crypt, entrance-barrow
2130: entrance-mine, entrance-antique, entrance-tomb, monster-body-reptile-shell, monster-body-reptile-shell-spiked, monster-body-reptile-crest1, monster-body-reptile-crest2, monster-body-reptile-crest3, monster-body-reptile-crest4, monster-body-reptile-crest5
2140: monster-body-reptile-spiked1, monster-body-reptile-spiked2, monster-foot-reptile-normal, monster-foot-reptile-claw1, monster-foot-reptile-claw2, monster-head-reptile-lizard, monster-head-reptile-turtle, monster-head-reptile-horn1, monster-head-reptile-horn2, monster-head-reptile-horn3
2150: monster-tail-reptile-normal, monster-tail-reptile-spike1, monster-tail-reptile-spike2, rune-giant-head-normal01, rune-giant-head-laser, rune-giant-body01, rune-giant-body02, rune-giant-hand01, rune-giant-foot01, cotton-plant
2160: cotton, turtle-body, turtle-head, turtle-foot, egg1, egg1, egg1, werewolf-head, werewolf-hand, werewolf-body
2170: werewolf-foot, vampire-head, vampire-eyes, vampire-hand, frame-house01, frame-inn, frame-shop, frame-tower, building-stone-well, building-stilt-inn1
2180: building-stilt-hut3, building-stilt-hut2, building-stilt-path, landingplace, stone-bridge, entrance-stairs, carpet1, carpet2, carpet3, framework-base
2190: framework-floor, framework-floor-stairs, framework-wall, framework-wall-window, framework-wall-door, framework-wall-indoor, framework-wall-balcony, framework-wall-lamp, framework-roof1, framework-roof2
2200: framework-roof3, framework-arc, stone-base, stone-floor, stone-floor-stairs, stone-wall, stone-wall-window, stone-wall-door, stone-wall-indoor, stone-wall-balcony
2210: stone-wall-lamp, stone-roof1, stone-roof2, stone-roof3, stone-arc, whitewood-base, whitewood-floor, whitewood-floor-stairs, whitewood-wall, whitewood-wall-window
2220: whitewood-wall-door, whitewood-wall-indoor, whitewood-wall-balcony, whitewood-wall-lamp, whitewood-roof1, whitewood-roof2, whitewood-roof3, whitewood-arc, clay-base, clay-floor
2230: clay-floor-stairs, clay-wall, clay-wall-window, clay-wall-door, clay-wall-indoor, clay-wall-balcony, clay-wall-lamp, clay-roof1, clay-roof2, clay-roof3
2240: clay-arc, clay-entrance-stairs, whiteclay-entrance-stairs, whiteclay-base, whiteclay-floor, whiteclay-floor-stairs, whiteclay-wall, whiteclay-wall-window, whiteclay-wall-door, whiteclay-wall-indoor
2250: whiteclay-wall-balcony, whiteclay-wall-lamp, whiteclay-roof1, whiteclay-roof2, whiteclay-roof3, whiteclay-arc, antiqueruin-entrance-stairs, antiqueruin-base, antiqueruin-floor, antiqueruin-floor-stairs
2260: antiqueruin-wall, antiqueruin-roof1, antiqueruin-roof2, antiqueruin-roof3, antiqueruin-arc, jungleruin-entrance-stairs, jungleruin-base, jungleruin-floor, jungleruin-floor-stairs, jungleruin-wall
2270: jungleruin-wall-door, jungleruin-wall-indoor, jungleruin-roof1, jungleruin-roof2, jungleruin-roof3, jungleruin-arc, desertruin-entrance-stairs, desertruin-base, desertruin-floor, desertruin-floor-stairs
2280: desertruin-wall, desertruin-wall-door, desertruin-wall-indoor, desertruin-roof1, desertruin-roof2, desertruin-roof3, desertruin-arc, wood-base, wood-floor, wood-floor-stairs
2290: wood-wall, wood-wall-window, wood-wall-door, wood-wall-indoor, wood-wall-balcony, wood-wall-lamp, wood-roof1, wood-roof2, wood-roof3, wood-arc
2300: palm-leaf, palm-leaf-diagonal, white-castle-round-tower, building-warrior, building-ranger, building-rogue, building-mage, building-smithy, building-carpentersshop, building-weavingmill
2310: cactus1, cactus2, cubequest4, painting01, painting02, icon-talk, icon-analyze, icon-vendor, digested-leftovers01, digested-leftovers02
2320: digested-leftovers03, digested-leftovers04, flower-vase1, flower-vase2, flower-vase3, flower-vase4, flower-vase5, flower-vase6, flower-vase7, flower-vase8
2330: flower-vase9, pitchfork, pumpkin, pumpkin-muffin, pineapple, pineapple-slice, spiribit, iron-nugget, silver-nugget, gold-nugget
2340: emerald-nugget, sapphire-nugget, ruby-nugget, diamond-nugget, iron-cube, silver-cube, gold-cube, wood-cube, fire-cube, ice-cube
2350: unholy-cube, wind-cube, cobweb, —, —, —, —, ice-crystal, ice-crystal-helmet, ice-crystal-shoulder
2360: ice-crystal-chest, ice-crystal-boot, ice-crystal-glove, ice-coated-yarn, sandstone, wood-log, parrot-feather, parrot-beak, parrot-glove, parrot-chest
2370: parrot-boot, parrot-helmet, parrot-shoulder, mammoth-hair, mammoth-glove, mammoth-chest, mammoth-boot, mammoth-helmet, mammoth-shoulder, bullterrier-head1
2380: bullterrier-head2, bullterrier-head3, gnobold-foot, gnobold-head, gnobold-hand, insectoid-foot, insectoid-head, insectoid-hand, insectoid-body, flask
2390: water-flask, radish-slice, onion-slice, mushroom-spit, plant-fiber, soup-ginseng, unknown, desert-rib, desert-skull, slime-green
2400: slime-pink, slime-yellow, slime-blue, frightener-head, frightener-eyes, sand-horror-head, sand-horror-hand, seastar, bread, sandwich
2410: fish1, fish2, shark, lantern-fish, lantern-fish-eyes, mawfish, piranha, blowfish, seahorse, —
2420: —, formula, key1, flowers2, flowers, grass, grass2, grass3, lava-flower, lava-grass
2430: thorn-plant, echinacea2, leaf, lantern02, torch, stone, stone2, tendril, tulips-colorful, cornflower
2440: reed, pumpkin-leaves, pineapple-leaves, sunflower, bean-tendril, desert-flower01, desert-flower02, wheat, corn, water-lily01
2450: water-lily02, inn-sign, shop-sign, weapon-shop-sign, armor-shop-sign, identifier-sign, smithy-sign, carpentersshop-sign, weavingmill-sign, ivy
2460: wall-roses-red, wall-roses-white, christmas-tree, underwater-plant, alga, coral, inca-art1, inca-art2, inca-art3, inca-art4
2470: crest1, castle-chain, wall-skull, torch-read, torch-green, torch-blue, torch-yellow, liana, chandelier, cobwebs
2480: cobwebs2, goddess2, door, big-door, window, castle-window, gate, spike-trap, stomp-trap, lever
2490: chest-base02, chest-top02, table, stone-table, sandstone-table, stone-stool, sandstone-stool, stool, bench, bed
2500: bedtable, cupboard, market-stand1, market-stand2, market-stand3, barrel, crate, open-crate, sack, shelter
2510: desktop, counter, shelf1, shelf2, shelf3, castle-shelf1, castle-shelf2, castle-shelf3, stone-shelf1, stone-shelf2
2520: stone-shelf3, sandstone-shelf1, sandstone-shelf2, sandstone-shelf3, corpse, runestone, flower-box01, flower-box02, flower-box03, street-light02
2530: street-light01, fence01, fence02, fence03, fence04, furnace, anvil, sawbench, workbench, customization-bench
2540: spinningwheel, loom, overground-dungeon01, underground-dungeon01, castle-arc, temple-arc, bomb1, glider, boat2, stars
2550: plaster, zzz, heart, angry, campfire, tent, beach-umbrella, beach-towel, wood-mat, compass2
2560: mission, city, skull, airship, sphinx01, obelisk, manacube, maptile, cloud02
```

## Appendix B. Creature types (per-type step of the generator 0x43f7c0)

Columns: models set; flags ORed into appearance 0x06; scalars set (names of §3.1,
"bodyPitch" is 0x48); vectors set as (x, y, z), "·" = component not written; "angles" =
(foot 0x58, wing 0x5c, tail 0x60). A type without a box entry keeps (0.96, 0.96, 2.16); a
part not listed keeps its previous value (65535 after the constructor). "head choice" and
"hair choice" are choice-record fields 0x08 and 0x0c, reduced with C `%` (§4.2). "Draw *k*"
counts the draws of the per-type step; when no choice record was given they follow the
five draws of §4.1. A model written as "*n* + (… mod *m*)" uses the remainder with C
semantics; "odd"/"even" tests use the signed remainder of division by 2. Keys that exist in
several slots are written with the slot this routine uses.

Hair colour per type: see §4.2. Computed boxes (single precision, from (0.96, 0.96, 2.16)):
"÷1.2" = (0.8, 0.8, 1.8); "×1.5" = (1.44, 1.44, 3.2400002); "×1.1" = (1.056, 1.056, 2.376);
"×0.7" = (0.672, 0.672, 1.512); "×0.9" = (0.864, 0.864, 1.944).

| Type | Models | Flags | Scalars | Vectors |
|---|---|---|---|---|
| 0 | head 1236 + (head choice mod 4) (elf-head-m01 … m04); hair 1280 + (hair choice mod 10) (elf-hair-m01 … m10); hand 430 hand2; foot 432 foot; body 1 body4 |  |  |  |
| 1 | head 1240 + (head choice mod 6) (elf-head-f01 … f06); hair 1290 + (hair choice mod 10) (elf-hair-f01 … f10); hand 430; foot 432; body 0 body2 |  |  |  |
| 2 | head 1246 + (head choice mod 6) (human-head-m01 … m06); hair 1252 + (hair choice mod 15) (human-hair-m01 … m15); hand 431 brown-hand if the head index > 1248, else 430; foot 432; body 1 |  |  |  |
| 3 | head 1267 + (head choice mod 6) (human-head-f01 … f06); hair 1273 + (hair choice mod 7) (human-hair-f01 … f07); hand 431 if the head index > 1269, else 430; foot 432; body 1 |  |  |  |
| 4 | head 75 + (head choice mod 5) (goblin-head-m01 … m05); hair 80 + (hair choice mod 6) (goblin-hair-m01 … m06); hand 97 goblin-hand; foot 432; body 0 |  | weapon 1.2 | box ÷1.2 |
| 5 | head 86 + (head choice mod 5) (goblin-head-f01 … f05); hair 91 + (hair choice mod 6) (goblin-hair-f01 … f06); hand 97; foot 432; body 0 |  | weapon 1.2 | box ÷1.2 |
| 6 | head 2379 + (draw 4 mod 3) (bullterrier-head1 … 3), after hair-colour draws 1–3; hand 431 brown-hand; foot 432; body 0 |  | head 0.9; weapon 1.2 | headOff (0, 3, 5) |
| 7 | head 98 + (head choice mod 2) (lizard-head-m01, m02); hair 100 + (hair choice mod 6) (lizard-hair-m01 … m06); hand 111 lizard-hand; foot 113 lizard-foot; body 112 lizard-body |  |  |  |
| 8 | head 106 + (head choice mod 5) (lizard-head-f01 … f05); hair 100 + (hair choice mod 6) (lizard-hair-m01 … m06); hand 111; foot 113; body 112 |  |  |  |
| 9 | head 282 + (head choice mod 5) (dwarf-head-m01 … m05); hair 287 + (hair choice mod 3) (dwarf-hair-m01 … m03); hand 430; foot 432; body 300 dwarf-body |  | head 0.9; weapon 1.2 | box ÷1.2 |
| 10 | head 290 + (head choice mod 5) (dwarf-head-f01 … f05); hair 295 + (hair choice mod 5) (dwarf-hair-f01 … f05); hand 430; foot 432; body 301 dwarf-body-female |  | head 0.9; weapon 1.2 | box ÷1.2 |
| 11 | head 1300 + (head choice mod 5) (orc-head, orc-head-m02 … m05); hair 1310 + (hair choice mod 10) (orc-hair-m01 … m10); hand 302 orc-hand; foot 432; body 0 |  | head 0.9; shoulder 1.2 | box (1.04, 1.04, 2.34) |
| 12 | head 1305 + (head choice mod 5) (orc-head-f01 … f05); hair 1320 + (hair choice mod 4) (orc-hair-f01 … f04); hand 302; foot 432; body 0 |  | head 0.8; body 0.95; shoulder 1.1 | box (1.04, 1.04, 2.34); headOff (0, 1.5, 4) |
| 13 | head 1324 + (head choice mod 5) (frogman-head-m01 … m05); hair 1329 + (hair choice mod 5) (frogman-hair-m01 … m05); hand 1342 frogman-hand; foot 432; body 1 |  |  |  |
| 14 | head 1334 + (head choice mod 4) (frogman-head-f01 … f04); hair 1338 + (hair choice mod 4) (frogman-hair-f01 … f04); hand 1342; foot 432; body 1 |  |  |  |
| 15 | head 303 + (head choice mod 6) (undead-head-m01 … m06); hair 309 + (hair choice mod 6) (undead-hair-m01 … m06); hand 327 undead-hand; foot 432; body 0 |  | head 0.9 |  |
| 16 | head 315 + (head choice mod 6) (undead-head-f01 … f06); hair 321 + (hair choice mod 6) (undead-hair-f01 … f06); hand 327; foot 432; body 0 |  | head 0.9 |  |
| 17 | head 328 skeleton-head; hair 329 skeleton-eyes; hand 330 skeleton-hand; foot 332 skeleton-foot; body 331 skeleton-body | 0x428 | head 0.8 | handRot (0, −90, 0); bodyOff (0, 0, −2); headOff (0, 4, 7); handOff (5, 3, 0) |
| 18 | head 11 old-head; hand 430; foot 432; body 0 |  |  |  |
| 19 | head 355 collie-head; hand 356 collie-hand; foot 357 collie-foot; body 354 collie-body; tail 358 collie-tail | 0x31 | head 0.9; hand 1; foot 1; tail 1 | box (0.8, 0.8, 1); angles (·, ·, −30); bodyOff (0, 0, 2); headOff (0, 9, 6); handOff (3, 4, −2.5); footOff (3, −4, −1); tailOff (0, −8, −1) |
| 20 | head 360 shepherd-dog-head; hand 361; foot 362; body 359; tail 363 (shepherd-dog parts) | 0x31 | head 0.9; hand 1; foot 1; tail 1 | as type 19 |
| 21 | head 365 skull-bull-head; hand 366; foot 367; body 364; tail 368 (skull-bull parts) | 0x31 | head 0.7; body 1.2; hand 0.8; foot 0.8; tail 1 | box (2.8, 2.8, 4); angles (·, ·, −30); bodyOff (0, 0, 1); headOff (0, 12.5, 3); handOff (3, 2.5, −4.5); footOff (3, −4, −4); tailOff (0, −8, −2.5) |
| 22 | head 370 alpaca-head; hand 371; foot 372; body 369 | 0x31 | head 0.8; hand 1; foot 1 | bodyOff (0, 0, −4); headOff (0, 9, 5); handOff (3, 4, −8.5); footOff (3, −4, −9) |
| 23 | head 374 alpaca-brown-head; hand 375; foot 376; body 373 | 0x31 | head 0.8; hand 1; foot 1 | as type 22 |
| 24 | body 2164 egg1 | 0x20 |  | box (0.8, 0.8, 1.2); bodyOff (0, 0, −1.5) |
| 25 | head 2162 turtle-head; hand 2163 turtle-foot; foot 2163; body 2161 turtle-body | 0x31 | head 0.65; hand 0.8; foot 0.8; tail 1 | box (0.8, 0.8, 1); angles (·, ·, −30); bodyOff (0, 0, 3.5); headOff (0, 13, 0.5); handOff (3, 4, −3.5); footOff (3, −4, −3.5); tailOff (0, −8, 4.5) |
| 26 | head 378 dog-head2; hand 379; foot 380; body 377; tail 381 | 0x31 | head 0.9; body 1.05; hand 1; foot 1; tail 0.8 | box (1.04, 1.04, 2.34); angles (·, ·, −120); bodyOff (0, 0, −5.5); headOff (0, 9, −1); handOff (3, 4, −8.5); footOff (3, −4.1, −8.5); tailOff (0, −10, 3) |
| 27 | head 383 scottish-terrier-head; hand 384; foot 385; body 382; tail 386 | 0x31 | head 0.9; body 1.05; hand 1; foot 1; tail 0.8 | box (0.8, 0.8, 0.7); angles (·, ·, −120); bodyOff (0, 0, 0.5); headOff (0, 9, 5); handOff (3, 4, −2.5); footOff (3, −4.1, −2.5); tailOff (0, −7, 2) |
| 28 | head 388 wolf-head; hand 389; foot 390; body 387; tail 391 | 0x31 | head 0.8; body 1.2; hand 0.9; foot 0.9 | box (1.52, 1.52, 3.23); bodyOff (0, 0, −3.5); headOff (0, 11, −1); handOff (3, 3, −7.5); footOff (3, −4.1, −7.5); tailOff (0, −5.5, −1) |
| 29 | head 393 panther-head; hand 394; foot 395; body 392; tail 396 | 0x31 | head 0.6; hand 1.1; foot 1; tail 1 | angles (·, ·, −30); bodyOff (0, 0, 0); headOff (0, 14, −1); handOff (3, 5, −8.5); footOff (3, −6, −9); tailOff (0, −11, −4) |
| 30 | head 398 cat-head; hand 399; foot 400; body 397; tail 401 | 0x31 | head 0.9; hand 1; foot 1; tail 1.1 | angles (·, ·, −30); bodyOff (0, 0, −4); headOff (0, 9, −1); handOff (3, 4, −8.5); footOff (3, −4.1, −8.5); tailOff (0, −8, −1) |
| 31 | head 403 cat-head2; hand 404; foot 405; body 402; tail 406 | 0x31 | as type 30 | as type 30 |
| 32 | head 408 cat-head3; hand 409; foot 410; body 407; tail 411 | 0x31 | as type 30 | as type 30 |
| 33 | head 413 pig-head; hand 414 pig-foot; foot 414; body 412 pig-body | 0x31 | head 0.85; body 1.2; hand 1; foot 1; tail 1 | box (0.8, 0.8, 0.8); angles (·, ·, −30); bodyOff (0, 0, 1); headOff (0, 9, 1.9); handOff (3, 4, −4.2); footOff (3, −4, −4.2); tailOff (0, −8, −1) |
| 34 | head 416 sheep-head; hand 417 sheep-foot; foot 417; body 415 sheep-body | 0x31 | head 0.85; body 1.2; hand 1; foot 1; tail 1 | box (1.2, 1.2, 1.2); angles (·, ·, −30); bodyOff (0, 0, 1); headOff (0, 4, 1.9); handOff (3, 4, −4.2); footOff (3, −4, −4.2); tailOff (0, −8, −1) |
| 35 | foot 155 bunny-foot; body 154 bunny-body | 0x30 | hand 0.9 | box (1, 1, 1.1); bodyOff (0, 0, 2); headOff (0, 1, 8); handOff (9, 0, 8); footOff (5, 1, −2.5) |
| 36 | foot 157 porcupine-foot; body 156 porcupine-body | 0x30 | hand 0.9 | as type 35 |
| 37–40 | body 2399 slime-green / 2400 slime-pink / 2401 slime-yellow / 2402 slime-blue | 0x29 | body 1.5 | box (0.8, 0.8, 0.8); bodyOff (0, 0, 0) |
| 41 | head 2403 frightener-head; hair 2404 frightener-eyes | 0x42b | head 0.8 | box (1.4, 1.4, 2.5); headOff (0, 0, 0) |
| 42 | head 2405 sand-horror-head; hand 2406 sand-horror-hand | 0x2a | head 0.8; hand 0.5 | box (1.4, 1.4, 2.5); headOff (0, 0, 0); handOff (6, 0, 2) |
| 43 | head 3 wizard-head; hand 430; foot 432; body 4 wizard-body | 0x20 |  |  |
| 44 | head 10 elf-head-female; hair none (65535); hand 430; foot 432; body 0 |  |  |  |
| 45 | head 5 witch-head; hand 430; foot 432; body 6 witch-body | 0x20 | head 0.8 | headOff (0, 3, 4) |
| 46 | hair colour from draws 1–3 (§4.2); head 60 ogre-head; hand 62 ogre-hand; foot 63 ogre-foot; body 61 ogre-body | 0x20 | head 0.61 | box (2.4, 2.4, 5.4); bodyOff (0, 0, −2); headOff (0, 4, 6); handOff (7.5, 0, 3) |
| 47 | head 64 rockling-head; hand 65 rockling-hand; foot 66 rockling-foot | 0x20 | head 0.8; hand 0.85; foot 0.85; weapon 0.65 | box (2.4, 2.4, 3.6); bodyOff (0, 0, −1); headOff (0, 0, 2); handOff (7.5, 0, 5); footOff (3, 1, −6.5) |
| 48 | head 12 gnoll-head; hand 14; foot 15; body 13 | 0x20 |  | box ÷1.2 |
| 49 | head 16 polar-gnoll-head; hand 18; foot 19; body 17 | 0x20 | head 1.2 | box ÷1.2; headOff (0, 1.5, 4) |
| 50 | head 20 monkey-head; hand 22; foot 23; body 21 | 0x28 | head 1.05; body 0.8 | box (0.64, 0.64, 1.36); bodyOff (0, 0, −4); headOff (0, 3.5, 2); footOff (3.5, 1, −8.5) |
| 51 | head 2383 gnobold-head; hand 2384 gnobold-hand; foot 2382 gnobold-foot; body 226 zombie-body | 0x20 |  |  |
| 52 | head 2386 insectoid-head; hand 2387; foot 2385; body 2388 | 0x20 |  | bodyOff (0, 0, −4); handOff (6, 0, 1) |
| 53 | head 231 hornet-head; foot 232 hornet-foot; body 230 hornet-body; wing 233 hornet-hand | 0x32 | bodyPitch −45 | angles (30, 45, ·); footOff (3, −1, −7); wingOff (1, −6, 1) |
| 54 | head 235 insect-guard-head; hand 237; foot 236; body 234; wing 238 insect-guard-wing | 0x32 | bodyPitch −45 | as type 53 |
| 55 | head 255 crow-head; foot 256; body 254; wing 257 crow-hand | 0x32 | bodyPitch −45 | angles (·, 45, ·); headOff (0, 3, 6); footOff (3, −4, −10); wingOff (3, −5, 0) |
| 56 | head 259 chicken-head; foot 260; body 258; wing 261 chicken-hand | 0x32 | body 1.3; bodyPitch 0 | box (0.8, 0.8, 1.65); angles (·, 45, ·); headOff (0, 3, 2); footOff (3, 0, −10); wingOff (3, −2, −5) |
| 57 | head 263 seagull-head; foot 264; body 262; wing 265 seagull-hand | 0x32 | bodyPitch −45 | angles (·, 45, ·); headOff (0, 5, 8); footOff (3, −4, −10); wingOff (3, −5, 0) |
| 58 | draw 1 odd → head 267, foot 268, body 266, wing 269 (parrot); even → head 271, foot 272, body 270, wing 273 (parrot-blue) | 0x32 | bodyPitch −45 | angles (·, 45, ·); headOff (0, 3, 6); footOff (3, −4, −10); wingOff (3, −5, 0) |
| 59 | head 275 bat-head; foot 276; body 274; wing 277 bat-hand | 0x32 | bodyPitch −45 | angles (·, 45, ·); footOff (3, −4, −10); wingOff (3, −5, 0) |
| 60 | head 240 fly-head; foot 241; body 239; wing 242 fly-hand | 0x32 | bodyPitch −45 | angles (30, 45, ·); headOff (0, 3, 4); footOff (3, −1, −7); wingOff (1, −6, 1) |
| 61 | head 247 midge-head; foot 248; body 246; wing 249 midge-hand | 0x32 | bodyPitch −45 | angles (30, 45, ·); headOff (0, 3, 4); footOff (3, −1, −5); wingOff (1, −5, −1) |
| 62 | head 251 mosquito-head; foot 252; body 250; wing 253 mosquito-hand | 0x32 | bodyPitch −45 | angles (30, 45, ·); headOff (0, 3, 4); footOff (3, −1, −7); wingOff (1, −6, 1) |
| 63–66 | hand none (65535); foot 334 / 336 / 338 / 340 and body 333 / 335 / 337 / 339 (plain-, leaf-, snow-, desert-runner) | 0x28 | foot 1.5 | box (1.2, 1.2, 2.7); bodyOff (0, 3, 6); footOff (4, 0, −1.5) |
| 67 | head 343 peacock-head; foot 342; body 341 | 0x28 | head 0.5; body 0.7; foot 1.5 | box (1.2, 1.2, 3.75); bodyOff (0, 0, 4); headOff (0, 4, 8); footOff (4, 0, −6) |
| 68 | head 115 frog-head; hand 117; foot 116; body 114 | 0x20 | hand 1.2 | bodyOff (0, 0, −6); headOff (0, 1, 2); handOff (8, 0, 2); footOff (5, 1, −8.5) |
| 69 | head 119 plant-creature-head; hand 121; foot 120; body 118 | 0x28 | hand 0.8; foot 0.7 | box (1.2, 1.2, 2.7); handRot (−45, 45, −45); bodyOff (0, 0, −4); headOff (0, 1, 7); handOff (6, 3, 0); footOff (3, 2, −10) |
| 70 | hand 124 radish-creature-hand; foot 123; body 122 radish-creature-head | 0x28 |  | box (1.2, 1.2, 2.7); handRot (−45, 45, −45); bodyOff (0, 0, 5); headOff (0, 1, 7); handOff (6, 3, 0); footOff (3, 2, −8) |
| 71 | foot 126 onionling-foot; body 125 onionling-head | 0x30 |  | box (1.2, 1.2, 2.7); bodyOff (0, 0, 1); headOff (0, 0, 0); footOff (3, 0, −12) |
| 72 | foot 128 desert-onionling-foot; body 127 desert-onionling | 0x30 |  | as type 71 |
| 73 | foot 130 devourer-foot; body 129 devourer-head | 0x30 |  | box (1.2, 1.2, 2.7); headOff (0, 0, −12); footOff (3, 0, −11) |
| 74 | head 419 duckbill-head; hand 420 duckbill-foot; foot 420; body 418; tail 421 | 0x31 | head 0.73; body 0.9; hand 1; foot 1; tail 0.8 | box (1, 1, 1); angles (·, ·, −80); bodyOff (0, 0, 0.6); headOff (0, 8.5, 1.5); handOff (4, 1, −3.7); footOff (4, −3, −3.7); tailOff (0, −8.5, −2.9) |
| 75 | head 423 crocodile-head; hand 424 crocodile-foot; foot 424; body 422 | 0x31 | head 1.1; body 2; hand 1.6; foot 1.6 | box (1, 1, 1); bodyOff (0, −9, 2.6); headOff (0, 17.5, 2.5); handOff (6, 9, −1.7); footOff (6, −9, −1.7) |
| 76–79 | head / foot / body / hand of spike (160, 161, 162, 163), anubis (164–167), horus (168–171), jester (172–175) | 0x20 | body 0.9 | handOff (6, 0, −2) |
| 80 | head 176 spectrino-head; foot 177; body 178; hand 179 | 0x20 |  |  |
| 81 | head 190 djinn-head; foot 191; body 192; hand 193 | 0x20 | head 0.8; foot 0.8 | box ×1.5; headOff (0, 1.8, 6); footOff (3.5, 1, −10.8) |
| 82 | head 194 minotaur-head; foot 195; body 196; hand 197; arm 198 minotaur-arm | 0x20 | head 0.5; body 0.9; hand 0.7; foot 0.9; arm 0.65; weapon 0.9 | box (3.2, 3.2, 5.8); bodyOff (0, 0, −2); headOff (0, 3, 4.5); handOff (7, 0, 1.5); footOff (3, 1, −7.5) |
| 83 | head 180 + (draw 1 mod 3) (desert-nomad-head1 … 3); hand 189 desert-nomad-hand2 if the head index ≠ 180, else 188 desert-nomad-hand; foot 186; body 187 | 0x20 | head 0.9 | headOff (0, 0.8, 6) |
| 84 | head 183 + (draw 1 mod 3) (desert-nomad-female-head1 … 3); hand 189 if the head index ≠ 183, else 188; foot 186; body 187 | 0x20 | head 0.9 | headOff (0, 0.8, 6) |
| 85 | head 200 imp-head; hair 199 imp-eyes; hand 203; foot 201; body 202 | 0x428 | body 0.9 | handRot (−90, 30, ·); handOff (8, 5, 0) |
| 86 | foot 159 squid-foot; body 158 squid-body | 0x10 |  | box (0.8, 0.8, 1); bodyOff (0, 0, 2); footOff (5, 1, −5.5) |
| 87 | head 132 mole-head; hand 134; foot 133; body 131 | 0x28 | hand 0.9 | box ÷1.2; handRot (·, −30, ·); bodyOff (0, 0, −6); headOff (0, 1, 2); handOff (9, 0, 2); footOff (5, 1, −8.5) |
| 88 | head 136 biter-head; hand 138; foot 137; body 135 | 0x28 | hand 0.9 | box ×1.1; handRot (·, −30, ·); bodyOff (0, 0, −5); headOff (0, 1, 3); handOff (9, 0, 2); footOff (5, 1, −8.5) |
| 89 | head 140 koala-head; hand 142; foot 141; body 139 | 0x28 | head 1.2; hand 0.9 | as type 88 |
| 90 | head 144 squirrel-head; hand 146; foot 145; body 143 | 0x28 | head 1.2; hand 0.75; foot 0.8 | box ×0.7; handRot (·, −30, ·); bodyOff (0, −5, −5); headOff (0, 1, 4); handOff (7, 0, 0); footOff (5, 1, −8.5) |
| 91 | head 148 raccoon-head; hand 150; foot 149; body 147 | 0x28 | head 1.2; hand 0.75; foot 0.8 | box ×0.9; otherwise as type 90 |
| 92 | head 151 owl-head; foot 152 owl-foot; wing 153 owl-hand | 0x2a | hand 0.9; bodyPitch −10 | box ÷1.2; angles (·, 80, ·); headOff (0, 0, −2); handOff (9, 0, 2); footOff (5, 0, −11.5); wingOff (3, −5, 0) |
| 93 | head 205 penguin-head; hand 207; foot 206; body 204 | 0x28 | hand 0.9 | box ÷1.2; handRot (−60, −45, 0); bodyOff (0, 0, −6); headOff (0, 1, 2); handOff (8, 0, 2); footOff (5, 1, −8.5) |
| 94 | head 2167 werewolf-head; hand 2168; foot 2170; body 2169 | 0x28 | head 0.8; hand 0.6; shoulder 1.2 | box (2.4, 2.4, 5.4); handRot (0, −45, 0); headOff (0, 3, 3); handOff (5, 4, 0) |
| 95 | head 225 santa-head; hand 430; foot 432; body 224 santa-body | 0x20 | head 0.8 |  |
| 96 | head 227 zombie-head; hair none (65535); hand 229 zombie-hand; foot 228 foot; body 226 zombie-body | 0x28 |  | handRot (0, −90, 0); handOff (6, 3, −2) |
| 97 | head 2171 vampire-head; hair 2172 vampire-eyes; hand 2173 vampire-hand; foot 432; body 1463 wool-chest5; wing 277 bat-hand | 0x402 | wing 2 | angles (·, 60, ·) |
| 98 | head 345 pony-head; hand 346 pony-foot; foot 346; body 344; tail 347 pony-tail | 0x31 | head 0.8; body 0.9; hand 0.8; foot 0.8; tail 1 | box (1.44, 1.44, 1.6); bodyOff (0, 0, 3); headOff (0, 8, 6); handOff (3, 3, −3.25); footOff (3, −5, −3.25); tailOff (0, −7, −4) |
| 99 | head 349 camel-head; hand 350 camel-foot; foot 350; body 348 | 0x31 | head 0.65; body 0.95; hand 0.8; foot 0.75 | box (1.8, 1.8, 3); bodyOff (0, 0, 1); headOff (0, 12, 4); handOff (3, 3, −5.75); footOff (3, −4, −5.75) |
| 100 | head 352 cow-head; hand 353 cow-foot; foot 353; body 351 | 0x31 | head 0.65; body 1.2; hand 0.8; foot 0.75 | box (1.8, 1.8, 3); bodyOff (0, 0, −2); headOff (0, 10, 1); handOff (3, 3, −6.75); footOff (3, −4, −6.75) |
| 101 | head 426 dragon-head; hand 427 dragon-foot; foot 427; body 425; tail 429 dragon-tail | 0x33 | head 0.51; hand 1.5; foot 1 | bodyOff (0, 0, −2); headOff (0, 12.5, 0.5); handOff (3, 4, 1); footOff (3, −4, 1); tailOff (0, −16, 4) |
| 102 | head 216 barkbeetle-head; foot 217 | 0x31 | head 0.75 | box (1.6, 1.6, 1.6); headOff (0, 1, 1); footOff (3.5, 1, −2.5) |
| 103 | head 218 firebeetle-head; foot 219 | 0x31 | head 0.9 | as type 102 |
| 104 | head 220 snoutbeetle-head; foot 221 | 0x29 | head 0.9 | as type 102 |
| 105 | head 222 lemonbeetle-head; foot 223 | 0x31 | head 0.9 | as type 102 |
| 106 | head 209 crab-head; hand 211; foot 210; body 208 | 0x28 | head 0.75; hand 1.2 | box (0.8, 0.8, 1.2); handRot (90, 30, 0); bodyOff (0, 0, 1); headOff (0, 1, 5); handOff (10, 0, 8); footOff (6, 1, −5.5) |
| 107 | head 213 crab-head-blue; hand 215; foot 214; body 212 | 0x28 | head 0.75; hand 1.2 | box (4.8, 4.8, 7.2); otherwise as type 106 |
| 108 | head 24 troll-head; hand 27; foot 28; body 25; arm 26 troll-arm | 0x20 | head 0.6; foot 0.75; arm 0.9; shoulder 1.2 | box (5.6, 5.6, 12.6); headOff (0, 3, 3); handOff (8, 3, 0) |
| 109 | head 30 dark-troll-head; hair 29 dark-troll-eyes; hand 33; foot 34; body 31; arm 32 | 0x420 | as type 108 | as type 108 |
| 110 | head 35 hell-demon-head; hand 38; foot 39; body 36; arm 37 | 0x20 | head 0.6; foot 0.75; arm 0.85; shoulder 1.1 | box (3.2, 3.2, 7.2); headOff (0, 3, 1); handOff (8, 3, 0) |
| 111 | head 40 golem-head; hand 43; foot 44; body 41; arm 42 | 0x20 | head 0.6; foot 0.755; arm 0.9; shoulder 1.2 | box (5.6, 5.6, 12.6); headOff (0, 4, 3); handOff (8, 3, 0) |
| 112 | head 45 golem-ember-head; hand 48; foot 49; body 46; arm 47 | 0x20 | head 0.61; foot 0.755; arm 0.9; shoulder 1.2 | as type 111 |
| 113 | head 50 golem-snow-head; hand 53; foot 54; body 51; arm 52 | 0x20 | head 0.61; foot 0.755; arm 0.9; shoulder 1.2 | as type 111 |
| 114 | head 55 yeti-head; hand 58; foot 59; body 56; arm 57 | 0x20 | as type 108 | as type 108 |
| 115 | head 67 cyclops-head; hand 70; foot 71; body 68; arm 69 | 0x20 | as type 108 | box (4.8, 4.8, 10.8); headOff (0, 3, 3); handOff (8, 3, 0) |
| 116 | head 72 mammoth-head; hand 74 mammoth-foot; foot 74; body 73 | 0x31 | head 0.4; body 0.7; hand 0.5; foot 0.5 | box (*s*, *s*, 1.12·*s*) with *s* = 3·draw 1/32767 + 7 (made before the models are written); bodyOff (0, 0, 1); headOff (0, 8, 2); handOff (3, 3, −4.1); footOff (2.9, −3, −4.1); tailOff (0, −4, −3) |
| 117 | head 279 lich-head; hand 280 lich-hand; foot none (65535); body 278 lich-body; arm 281 lich-arm | 0x20 | head 0.4; hand 0.75 | box (6.4, 6.4, 14.4); handRot (0, −90, 0); bodyOff (0, 0, 3); headOff (0, 1, 11); handOff (6, 5, 6) |
| 118 | head 2154 rune-giant-head-laser; body 2155 + (draw 1 mod 2) (rune-giant-body01, 02); hand 2157 rune-giant-hand01; foot 2158 rune-giant-foot01 | 0x2c | head 0.7; hand 1.5; foot 0.8; arm 0.9; shoulder 1.2 | box (4, 4, 10); bodyOff (0, 0, 2); headOff (0, 4, 8); handOff (8, 0, 5); footOff (3, 1, −9.5) |
| 119 | box first; then head 2145 + (draw 1 mod 5) (monster-head-reptile-lizard … horn3); body 2133 + (draw 2 mod 9) (monster-body-reptile-shell … spiked2); hand **and** foot both 2142 + (draw 3 mod 3) (monster-foot-reptile-normal … claw2; one draw for both); tail 2150 + (draw 4 mod 2) (monster-tail-reptile-normal, spike1) | 0x31 | tail 0.7; body 0.7; head 0.1·draw 5/32767 + 0.4; foot 0.5; hand 0.5 | box (9, 9, 10.08); headOff (0, 8, 0.5·draw 6/32767 − 1); bodyOff (0, 0, 1); tailOff (0, −4, −3); handOff (draw 7/32767 + 3, 3, −4.1); footOff (draw 8/32767 + 3, −3, −4.1) |
| 120 | body 2087 bush |  |  | box (2, 2, 2); bodyOff (0, 0, 0) |
| 121 | body 2088 snow-bush |  |  | box (2, 2, 2); bodyOff (0, 0, 0) |
| 122 | body 2090 berry-bush |  |  | box (2, 2, 2); bodyOff (0, 0, 0) |
| 123 | body 2159 cotton-plant |  |  | box (0.5, 0.5, 1.5); bodyOff (0, 0, 0) |
| 124 | body 2093 scrub if draw 1 is odd, else 2094 scrub-green |  |  | box (2, 2, 2); bodyOff (0, 0, 2) |
| 125 | body 2089 cobwebscrub |  | body 1.5 | box (0.5, 0.5, 1.7); bodyOff (0, 0, 0) |
| 126 | body 2095 fire-scrub |  |  | box (2, 2, 2); bodyOff (0, 0, 2) |
| 127 | body 2096 ginseng |  |  | box (1, 1, 1); bodyOff (0, 0, 0) |
| 128 | body 2310 cactus1 |  |  | box (1.5, 1.5, 4); bodyOff (0, 0, 0) |
| 129 | body 2098 fir-tree |  |  | bodyOff (0, 0, −4) |
| 130 | body 2099 thorn-tree |  |  | box (3, 3, 12); bodyOff (0, 0, −4) |
| 131–139 | body 2100 … 2108 (gold-, iron-, silver-, sandstone-, emerald-, sapphire-, ruby-, diamond-, ice-crystal-deposit) |  |  | box (2, 2, 2); bodyOff (0, 0, −4) |
| 140 | body 2109 scarecrow | 0x20 | body 2 | box (0.8, 0.8, 4); bodyOff (0, 0, 0) |
| 141 | body 2110 aim | 0x20 |  | box (1.5, 1.5, 2); bodyOff (0, 0, 0) |
| 142 | body 2111 dummy | 0x20 |  | box (1.5, 1.5, 2.5); bodyOff (0, 0, 0) |
| 143 | body 2113 + (draw 1 mod 4) (vase … vase4) |  |  | box (1.5, 1.5, 1.25); bodyOff (0, 0, 0) |
| 144 | body 2546 bomb1 |  | body 0.8 | box (0.8, 0.8, 0.8); bodyOff (0, 0, 1) |
| 145 | body 2410 fish1 | 0x131 | body 0.9 | box (*s*, *s*, *s*) with *s* = (0.5·draw 1/32767 + 1.2)·0.8; bodyOff (0, 0, 0) |
| 146 | body 2411 fish2 | 0x131 | body 0.9 | as type 145 |
| 147 | body 2418 seahorse | 0x131 |  | box (0.8, 0.8, 2); bodyOff (0, 0, 0) |
| 148 | head 1343 + (head choice mod 3) (mermaid-head-f01 … f03); hair 1346 + (hair choice mod 3) (mermaid-hair-f01 … f03); hand 1350 mermaid-hand; body 1349 mermaid-body |  |  |  |
| 149 | head 1351 + (head choice mod 3) (merman-head-m01 … m03); hair 1354 + (hair choice mod 3) (merman-hair-m01 … m03); hand 1350; body 1357 merman-body |  |  |  |
| 150 | body 2412 shark | 0x131 | body 1.2 | box (2.4, 2.4, 6); bodyOff (0, 0, 0) |
| 151 | head 243 bumblebee; foot 245 bumblebee-foot; wing 244 bumblebee-hand | 0x32 | head 1.2; foot 1; wing 0.6; bodyPitch −45 | box (1, 1, 1.875); angles (·, 45, ·); headOff (0, 3, 0); footOff (3, 4, −8); wingOff (6.5, 3, −1) |
| 152 | head 2413 lantern-fish; hair 2414 lantern-fish-eyes | 0x531 | head 1.2 | box (2.4, 2.4, 6); bodyOff (0, 0, 0); headOff (0, 0, 0) |
| 153–155 | body 2415 mawfish / 2416 piranha / 2417 blowfish | 0x131 | body 1.2 | box (2.4, 2.4, 6); bodyOff (0, 0, 0) |

## Appendix C. Pose cases

Notation as §7.7–§7.8: Δ*L*, Δ*R* hand position offsets; ρ*L*, ρ*R* hand rotation offsets;
*T* torso angles; *B* body tilt; *S* torso shift; *K* head shift; *H* head angles; *Q* tail
angles; *U* shoulder roll; *E* lean (*E*.z in blocks); *h* pivot height; *L*, *R* hand base
targets; *F* foot extra angle; ℓ body lean; *r* smoothing rate (0.01 unless written). Vectors
are (x, y, z); a single component written alone leaves the other components as they were.
"Actions" are the action ids mapped to the pose by 0x4b4ef0 (§7.2). Addresses are case
entries from the jump table.

| Pose (entry) | Actions | Case |
|---|---|---|
| 1 (0x4174a0) | 47, 54 | **t < 500** (fixed 500 ms), *q* = min(*t*/500, 1): two-handed main → ρ*R*.x := 120*q*, Δ*R* := (−*R*.x, 0, 14), Δ*L* := (2, 0, 12); otherwise Δ*R* := (1, 0, 14), Δ*L* := (−1, 0, 14), ρ*L*.x := −100, ρ*R*.x := −100. Then *T*.x := −40*q*; *B*.x := −40*q*; *F* := −50*q*; if the record action id is 48, *B*.x −= min(360·*t*/500, 360); the object's walk amplitude (obj +0x1188) := 0. **500 ≤ t < D**, *q*′ = min(2·(*t* − 500)/500, 1): *B*.x := −60·(1 − *q*′); *T*.x := −20; ρ*R*.x := −70; two-handed → Δ*R* := (−*R*.x, 4, 0), Δ*L* := (2, 4, 0); otherwise Δ*R* := (1, 3, 8), Δ*L* := (−1, 3, 8), ρ*L*.x := −150, ρ*R*.x := −150; *r* := 0.1. **E**: two-handed → *R* += (−8, 4, 0), G((−2, 0, −1)) with arm lift, ρ*R*.x := −60, ρ*R*.z := 20; otherwise Δ*R* := (1, 3, 2), Δ*L* := (−1, 3, 2), ρ*L*.x := −250, ρ*R*.x := −250. **Every phase**, afterwards: off-hand slot item type 3 subtype 13 (shield) → ρ*L* := (−250, −90, 0). (The Δ*L*.x values written as 2 are computed as 2 minus the value present, which is 0 on every path into this case.) |
| 2 (0x41644b) | 57, 60 | **W**, *q* = min(1.5·*t*/P1, 1): *B*.x := 5 + 10*q*; *T* := (10, 10, −60); ρ*R* := (lerp(90, 60, *q*), 0, lerp(90, 60, *q*)); Δ*R* := (0, 0, 14*q*); Δ*L* := Δ*R*; G((2, 0, 0)) with arm lift; ρ*L* := ρ*R*; *r* := 0.01. **P1 ≤ t < D**, *u* = min((*t* − P1)/P2, 1): *B*.x := −10; *T*.y := 10, *T*.z := 60; ρ*R* := (−70, 0, −60); Δ*R* := (0, 2·sin(*u*·π/2), 0); Δ*L* := Δ*R*; G((2, −1, 0)) without arm lift; ρ*L* := ρ*R*; *r* := 0.04. **E**: pose id := 0; *R* += (0, 2, 3); G((−3, 0, −3)) with arm lift; ρ*R*.x := 0, ρ*R*.y := 20; ρ*L* := ρ*R*. Arm-frame head-pitch tilt omitted (§7.8). |
| 3 (0x4170ee) | 58 | **W**: *B*.z := 0; ρ*R* := (−90, 0, 0); Δ*R* := (−5, 8, 0); *r* := 0.005. **P1 ≤ t < P1 + P2/2**: *B* := (10, 0, −10); *T* := (30, 0, −30); ρ*R* := (0, 0, 90); Δ*R* := (−8, −4, 20); *r* := 0.005. **P1 + P2/2 ≤ t < D**: *B*.z := −30; *T* := (0, 20, −60); ρ*R* := (70, 0, 90); Δ*R* := (5, −2, 0); *r* := 0.02. In these three branches then Δ*L* := Δ*R*, G((2, 0, 0)) with arm lift, ρ*L* := ρ*R*. **E**: pose id := 0; G((−2, 0, −1)) with arm lift; ρ*R*.y := 90; *T*.z := −80; ρ*L* := ρ*R*. Arm-frame head-pitch tilt omitted. |
| 4 (0x416052) | 59 | **t < P1/2**, *q* = min(*t*/(P1/2), 1): *B*.x := 5*q*; *T*.y := 10*q*; *T*.z := −20*q*; ρ*R* := (60*q*, lerp(120, 60, *q*), 60*q*); G((2, 0, 0)) with arm lift; ρ*L* := ρ*R*; *r* := 0.005. **t ≥ P1/2** (no upper limit): if record bytes 0x130 and 0x131 (class, specialization) are both 1 → Δ*R* := (2, 7, 4), ρ*R* := (0, −60, 0), *r* := 0.01; otherwise *q*₂ = min(1.5·(*t* − P1/2)/(P1/2), 1), *B*.x := 0, *T* := (20, 0, 0), ρ*R* := (lerp(90, 30, *q*₂), 0, lerp(90, 0, *q*₂)), *R*.z += 0.5·cos(0.5·*t*), Δ*R* := (−6, 0, 16*q*₂), *r* := 0.005. Then Δ*L* := Δ*R*; G((2, 0, 0)) with arm lift; ρ*L* := ρ*R*. |
| 5 (0x419a6f) | 64 | All times: ρ*R* := (−90, 0, −40); ρ*L* := (−90, 0, 40); wobble (z); Δ*R* := (1, −7, 0); Δ*L* := (−1, −7, 0); *T*.z := −40; *r* := 0.005. |
| 6 (0x419bee) | 63 | Record byte 0x131 (specialization) = 1: ρ*R* := (−50, −60, 40); ρ*L* := (−90, 90, −50); wobble (z); Δ*R* := (−3, 7, 3); Δ*L* := (−2, 2, 0); *T*.z := −20; *r* := 0.005. Otherwise: ρ*R* := (−90, 0, −40); ρ*L* := (−90, 90, −40); wobble (z); Δ*R* := (1, −7, 0); Δ*L* := (−1, 0, 1); *T*.z := −40; *r* := 0.005. |
| 7 (0x417ab1) | none in §7.2 | **t < D/2**: *U* := the 3-vector at record 0x144 (meaning *unresolved*); *r* := 0.005. **D/2 ≤ t < D**: *c* = min(15·((*t* − D/2)/(D/2))³, 1); *U* := *c*·(record 0x138 vector) + (1 − *c*)·(record 0x144 vector); *r* := 0.03. **E**: *U* := record 0x138 vector. Every phase: ρ*R* := (0, 90, 0); if two-handed, G((2, 0, 0)) with arm lift. |
| 8 (0x419ebd) | 98 | **Off-hand model pointer set**: *t* < 100: *T*.z := 80, ρ*L* := (0, 0, 90), ρ*R* := (−45, 45, 0); 100 ≤ *t* < 500: *T*.z := −90, ρ*L* := (0, 60, 90), Δ*L*.z += 5, ρ*R* := (−90, 0, 45); *t* ≥ 500: *T*.z := −30, ρ*L* := (0, 100, 60), ρ*R* := (−45, 45, 0). **No off-hand model**: *t* < 100: *T*.z := −80, ρ*R* := (0, 0, 90); 100 ≤ *t* < 500: *T*.z := 90, ρ*R* := (0, 60, −85), Δ*L* += (2, 3, 3), Δ*R* += (2, 3, 3); *t* ≥ 500: *R* += (0, 2, 3), G((−3, 0, −3)) with arm lift, ρ*R*.x := 0, ρ*R*.y := 20. In all three no-off-hand branches, then G((−3, 0, −3)) without arm lift and ρ*L* := ρ*R*. |
| 9 (0x41b54e) | 8; 48 with an off-hand shield | **W**: *T*.z := 80; ρ*L* := (0, 0, 90); ρ*R* := (−45, 45, 0); if the record action id is 48, *B*.z := −360·*t*/300. **t ≥ P1**: *T*.z := −90; ρ*L* := (0, 0, 90); ρ*R* := (−90, 0, 45). |
| 10 (0x41b850) | 10 | **W**: as pose 9 W (including the action-48 term). **P1 ≤ t < D**: *T*.z := −90; ρ*L* := (0, 0, 90); ρ*R* := (−90, 0, 45); *L*.x += sin(0.1·(*t* − 600)); *r* := 0.03. **E**: STANCE (shield form). |
| 11 (0x41b006) | 28 | Unless appearance flag 0x0004 is set: *T*.z := 60; ρ*R* := (0, −45, −60); ρ*L* := (−45, −45, 0). |
| 15 (0x41db88) | 2 | First: if *c* = 2 → *B*.z := −360·clamp((*t* − P1)/P2), *r* := 0.03; else *B*.z := 0. *U* := record 0x138 vector. **W**: *T*.z := 40; ρ*L* := (0, −90, 30); Δ*L*.y −= 4, Δ*L*.z += 4; ρ*R* := (0, −90, −120); Δ*R*.y −= 6. **P1 ≤ t < D**: *T*.z := −90; *E*.x := −0.1; ρ*L* := (−90, −90, −30); ρ*R* := (0, −90, −120); Δ*R*.y −= 6. **E**: STANCE. Every phase, last: off-hand subtype byte 13 → ρ*L* := (0, 0, 90). |
| 16 (0x41e029) | 1 | First: if *c* = 1 → *B*.z := −360·clamp((*t* − P1)/P2), *r* := 0.03; else *B*.z := 0. *U* := record 0x138 vector. **W**: *T*.z := 90; ρ*R* := (0, −90, 0); ρ*L* := (−90, 0, 90); Δ*R*.y += 6, Δ*R*.z += 4. **P1 ≤ t < D**: *T*.z := 20; ρ*L* := (−90, −60, 90); Δ*L*.y −= 4, Δ*L*.z += 4; ρ*R* := (0, −90, −220); Δ*R*.y −= 6; *E*.x := 0.1; left foot base y −= 4; right foot base y += 4; turn. **E**: STANCE. Every phase, last: off-hand subtype byte 13 → ρ*L* := (0, 0, 90). |
| 17 (0x41d3ee) | 4, 18 | **W**: *T*.z := 40; ρ*R* := (−90, 90, 30); ρ*L* := (−90, 0, −20); Δ*L*.y −= 6. **P1 ≤ t < D**: *T*.z := −90; ρ*R* := (−40, 0, 30); Δ*R* := (3, 0, 4); ρ*L* := (−90, 90, 90); Δ*L* := (−3, −1, 0); turn; *r* := 0.05. **E**: STANCE. |
| 18 (0x41d123) | 3, 9, 19, 62 | **W**: *T*.z := −40; ρ*L* := (−90, −90, −30); ρ*R* := (−90, 0, 20); Δ*R*.y −= 6. **P1 ≤ t < D**: *T*.z := 90; ρ*L* := (0, 0, 40) if the off-hand subtype byte is 13, else (−40, 0, −30); Δ*L* := (−3, 0, 4); ρ*R* := (−90, −90, −90); Δ*R* := (3, −1, 0); turn; *r* := 0.05. **E**: STANCE (shield form). |
| 19 (0x41be27) | 7 | **P1 < t < D**: *T*.z := −20; *B*.z := −20; ρ*L* := (0, 90, 40); Δ*L* := (0, 8, 4); ρ*R* := (0, 90, 30); Δ*R* := (1, −4, −1); *H*.x := −10; *S*.y := 3; *r* := 0.03. **Otherwise**: guard idle (*k* = 0.005). |
| 20 (0x41badc) | 6 | **P1 < t < D**: *T*.z := 45; *B*.z := 45; ρ*R* := (0, −90, −90); Δ*R* := (8, 0, 4); ρ*L* := (0, −90, −30); Δ*L* := (−1, −4, −1); *H*.x := −10; *S*.y := 3; *r* := 0.03. **Otherwise**: guard idle (*k* = 0.01). |
| 21 (0x418591) | 22, 23 | **Main-hand subtype byte (slot 7) not 6 or 7**: if the off-hand subtype is 7, ρ*L*.y := −90. *t* < 2·D: Δ*L*.y += 5 and Δ*L*.x −= 7 when the off-hand subtype is 6, else Δ*L*.y += 2 and Δ*L*.x −= 3; Δ*L*.z += 4; Δ*R* += (−10, 3, 3); *T*.z −= 70; ρ*L*.z := 70; ρ*R*.z := 70; and while *t* < D/2 also Δ*L*.z −= 2, ρ*L*.x := −5, Δ*R* := (−5, 1, 0), main-hand model removed, *T*.z := −50, ρ*L*.z := 70. *t* ≥ 2·D: Δ*L*.x −= 1; ρ*L*.x := −30; ρ*R*.x := −30; ρ*R*.y := 45; off-hand subtype 7 → ρ*L*.z := 50. From *t* ≥ D/2 on, if the off-hand slot holds item type 3, the main hand draws model 840 (arrow). **Main-hand subtype 6 or 7**: subtype 7 → ρ*R*.y := −90. *t* < 2·D: Δ*R*.y += 5 and Δ*R*.x += 7 for subtype 6, else Δ*R*.y += 2 and Δ*R*.x += 3; Δ*R*.z += 4; Δ*L* += (10, 3, 3); *T*.z := 70; ρ*R*.z := −70; ρ*L*.z := −70; while *t* < D/2 also Δ*R*.z −= 2, ρ*R*.x := −5, Δ*L* := (5, 1, 0), off-hand model removed, *T*.z := 50, ρ*R*.z := −70. *t* ≥ 2·D: Δ*R*.x += 1; ρ*R*.x := −30; ρ*L*.x := −30; ρ*L*.y := −45. From *t* ≥ D/2 on, if the main-hand slot holds item type 3, the off hand draws model 840. |
| 22 (0x417c84) | 24, 25, 50, 55; 48 with a main-hand bow or crossbow | **Main-hand subtype not 6 or 7**: Δ*L* += (−4, 3, 2); Δ*R* += (−10, 2, 1); *T*.z := −60; ρ*L*.z := 60; ρ*L*.y := −90; ρ*R*.z := 60; if the off-hand slot holds item type 3, the main hand draws model 840; while *t* < 300: Δ*L*.z −= 1, ρ*L*.x += 20, ρ*L*.y −= 20. **Main-hand subtype 6 or 7**: Δ*R* += (4, 3, 2); Δ*L* += (10, 2, 1); *T*.z := 60; ρ*R*.z := −60; ρ*R*.y := −90; ρ*L*.z := −60; if the main-hand slot holds item type 3, the off hand draws model 840; while *t* < 300: Δ*R*.z −= 1, ρ*R*.x += 20, ρ*R*.y += 20. |
| 23 (0x41a5c1) | 37, 43 | **W**: *T*.z := −30; ρ*R* := (0, −45, −60); ρ*L* := (−45, −45, 0); Δ*R*.y += 4, Δ*L*.y += 4; wobble (Δ). **t ≥ P1**: *T*.z := 60; ρ*R* := (0, −45, −60); ρ*L* := (−45, −45, 0). |
| 24 (0x41ad90) | 39, 41 | **W**: Δ*R*.y += 4, Δ*L*.y += 4; wobble (Δ); *T*.z := 20. **P1 ≤ t < D**: *E*.x := 0.5; *T*.z := −60; *B*.z := −30; ρ*L* := (0, 45, 60); ρ*R* := (−45, 45, 0); Δ*L* := (−2, 0, 0); turn. |
| 25 (0x41ab1a) | 40, 42 | **W**: as pose 24 W with *T*.z := −20. **P1 ≤ t < D**: *E*.x := −0.5; *T*.z := 60; *B*.z := 30; ρ*R* := (0, −45, −60); ρ*L* := (−45, −45, 0); Δ*R* := (2, 0, 0); turn. |
| 26 (0x41a80b) | 38, 44, 45, 46, 94, 95 | **W**: as pose 23 W. **P1 ≤ t < D**: *T*.z := 90; ρ*R* := (−90, 0, −90); ρ*L* := (−45, −45, 0); Δ*R* := (3, 0, 1); *r* := 0.03. **E**: ρ*R* := (−60, 0, 0); Δ*R* := (3, 0, 1). |
| 27 (0x41a3e6) | 36 | All times: as pose 23 W. |
| 28 (0x418ac6) | 11 | **W**: *T*.z := −80; *B*.z := 0; ρ*R* := (−90, 90, −90); ρ*L* := (20, 90, 90). **A**: *T*.z := −90; *B*.z := 360·(*t* − P1)/P2; ρ*L* := (0, 100, 90); ρ*R* := (0, 90, 0); *r* := 0.2. **t ≥ P1 + P2**: *T*.z := 80; *B*.z := 0; ρ*L* := (90, 90, 270); ρ*R* := (0, 90, 90). |
| 29 (0x419347) | 12 | **W**: ρ*R* := (−80, −90, 60); ρ*L* := (−80, 90, −60); Δ*R*.y += 3, Δ*R*.x −= 5; Δ*L*.x += 5. **P1 ≤ t < D**: *T* := (0, 0, −30); Δ*R*.y −= 5; Δ*L*.y −= 5; ρ*L* := (−90, 90, 110); ρ*R* := (−90, −90, −110); *E*.y := −0.5; *r* := 0.04. **E**: STANCE (no creature-type exception). |
| 30 (0x41c115) | 13 | **W**, *q* = *t*/P1: ρ*R* := (−80, 90, −90); Δ*R* := (−10, 8, 8); *T*.z := −45; *B*.z := −20*q*; *E*.x := 0; if *c* = 2 and *t* < P1 + P2/2, *B*.z += 360*q*; *r* := 0.04. **P1 ≤ t < P1 + P2 + P3**, *q*₂ = min(1.2·(*t* − P1)/P2, 1): *E*.x := −0.8*q*₂; ρ*R* := lerp((−90, 90, 48), (−110, 60, 0), *q*₂); Δ*R* := lerp((−10, 8, 8), (−12, 0, −1), *q*₂); *H*.x := −10; *T*.z := 90*q*₂ − 45·(1 − *q*₂); *B*.z := 30*q*₂; ℓ := 10*q*₂; *r* := 0.05. **Later**: Δ*R* := (2, 0, 0); ρ*R* := (0, 0, 0); *T*.z := −30; *r* := 0.03. **Every phase**, then: Δ*L* := Δ*R*; ρ*L* := ρ*R*; *L* := *R* + (−0.2, −0.2, −1). |
| 31 (0x41c67c) | 14 | **W**, *q* = *t*/P1: ρ*R* := (0, −90, 0); Δ*R* := (−8, 6, 6); *H*.x := −10, *H*.z := 30; *T*.z := 45; *E*.x := 0; *B*.z := 20; if *c* = 2 and *t* < P1 + P2/2, *B*.z −= 360*q*; *r* := 0.05. **P1 ≤ t < D**, *q*₂ = min(1.2·(*t* − P1)/P2, 1): *E*.x := 0.8*q*₂; *H*.z := −30; ρ*R* := lerp((0, −90, 20), (0, −90, −130), *q*₂); Δ*R* := lerp((−8, 6, 6), (0, 2, 0), *q*₂); *T*.z := 45·(1 − *q*₂) − 45*q*₂; *B*.z := −30*q*₂; *B*.x := −20*q*₂; ℓ := 10*q*₂; *r* := 0.05. **E**: Δ*R* := (2, 0, 0); ρ*R* := (0, 0, 0); *T*.z := −30; *r* := 0.03. **Every phase**, then as pose 30. |
| 32 (0x41cbda) | 15 | **W**, *q* = *t*/P1: Δ*R* := (2, 2, 14); ρ*R* := (50, 0, 0); *T*.z := −45; *B*.z := −20*q*; *E*.x := 0.5*q*; *r* := 0.04. **P1 ≤ t < P1 + P2 + P3**, *q*₂ = min(2·(*t* − P1)/P2, 1): *B*.x := 20·(1 − *q*₂) − 10*q*₂; ρ*R* := lerp((90, 0, 45), (−90, 0, −45), *q*₂); Δ*R* := lerp((2, 2, 14), (0, 9, 0), *q*₂); *T*.z := 45*q*₂ − 45·(1 − *q*₂); *r* := 0.05. **Later**: Δ*R* := (2, 0, 0); ρ*R* := (0, 0, 0); *T*.z := −30; *r* := 0.03. **Every phase**, then as pose 30. |
| 33 (0x41cf8b) | 5 | **W**: Δ*R* := (4, 2, 8); ρ*R* := (−90, −180, 90); *T*.z := −90; *E*.y := −1; *r* := 0.02. **A**: Δ*R* := (6, 0, 0); *E*.y := 0.5; *T*.z := 90; ρ*R* := (−90, 0, −90); turn; *r* := 0.05. **t ≥ P1 + P2**: Δ*R* := (2, 0, 0); ρ*R* := (0, 0, 0); *T*.z := −30; *r* := 0.03. **Every phase**, then as pose 30. |
| 34 (0x419a23) | 48 (no bow, crossbow or off-hand weapon) | ρ*R* := (−90, −90, 80); *T*.z := −80. |
| 35 (0x419748) | 16, 17; 48 with an off-hand weapon; 93 with an off-hand item | **W**: ρ*R* := (−90, 0, 0); ρ*L* := (−90, 0, 40); Δ*R* := (1, −7, 0); Δ*L* := (−1, −7, 0); *T*.z := −40; *E*.y := −0.5; *r* := 0.005; turn. **P1 ≤ t < D**: ρ*R* := (−90, −60, −30); ρ*L* := (−90, 60, −50); Δ*R* := (3, 5, 2); Δ*L* := (6, 8, 2); *T*.z := 40; *r* := 0.05; turn. **E**: STANCE. |
| 37 (0x415dee) | 67 | **W**: ρ*R* := (−90, −90, 80); *T*.z := −80. **P1 ≤ t < D**: ρ*R* := (−90, 0, −90); *T*.z := 90; *R*.x += 4; left foot base y −= 4; right foot base y += 4; turn; *r* := 0.05. In both: G((2, 0, 0)) with arm lift; ρ*L* := ρ*R*. **E**: pose id := 0; *R* += (−8, 4, 0); ρ*R*.x := −60, ρ*R*.z := 20; *T*.z := −20; then *L* := *R* + (2, 0, 0) with the left arm base lifted twice (y += 20, z −= 8 in total); ρ*L* := ρ*R*. Arm-frame head-pitch tilt omitted except in E. |
| 38 (0x4144fc) | 68 | **t < P1 + P2/2**: *H* := (−30, 10, 90); *T* := (0, 0, 60); *K* := (−3, −2, 0). **P1 + P2/2 ≤ t < D**: *H* := (30, −10, −30); *T* := (10, 0, −20); *K* := (3, 0, 2). |
| 39 (0x41466c) | 69 | **t < P1 + P2/2**: *H* := (−70, 0, 0); *T* := (−10, 0, 0); *K* := (0, −3, −1); ρ*L*.x := 20; ρ*R*.x := 20. **P1 + P2/2 ≤ t < D**: *H* := (50, 0, 0); *T* := (20, 0, 0); *S* := (0, −3, 1); *K* := (0, 0, 2); ρ*R*.x := −20; ρ*L*.x := −20; *Q* := (−10, 0, 0). |
| 40 (0x415624) | 70 | **W**: *B*.z := 90; *H* := (0, 10, −60); *Q* := (0, 0, −70); *r* := 0.002. **P1 ≤ t < D**: *B*.z := 200; *H* := (0, −20, 60); *K* := (−3, 0, −2); *Q* := (0, 0, 70). |
| 41 (0x415771) | 71, 72 | **W**, *f* = *t*/P1: each hand base, each foot base and the head base move toward the appearance body offset (the head toward body offset + (0, 0, −2)) by the fraction *f*, i.e. *x* := *x* + (target − *x*)·*f*; the tail base moves toward body offset + (0, 4, 4) the same way. **A**: the head, hand, foot and tail model pointers are cleared (not drawn); *S* := (0, 0, −3). The pose id is not changed. **R**, *g* = 1 − (*t* − P1 − P2)/P3: the same moves with fraction *g*. |
| 42 (0x415113) | 73 | **t < P1 + P2/2**: *H* := (−10, 10, 30); *T* := (−10, 0, 0); *K* := (−3, 0, −1); ρ*L*.x := 20; ρ*R*.x := 20; *r* := 0.005. **P1 + P2/2 ≤ t < D**: *H* := (50, 30, 0), then *H*.y := −30 while *t* < P1 + P2; *T* := (30, 0, 0); *S* := (0, −1, 2); *K* := (0, 0, 2); ρ*R*.x := −60; ρ*L*.x := −20; *Q* := (−20, 0, 0); *r* := 0.005. |
| 43 (0x414878) | 74 | **t < P1 + P2/2**: *H* := (−20, 0, 0); *T* := (50, 0, 0); *S* := (0, −2, 2); *K* := (0, −6, −2); ρ*L*.x := 20; ρ*R*.x := 30; *Q* := (−50, 0, 0). **P1 + P2/2 ≤ t < D**: *H* := (−10, 0, 0); *T* := (0, 0, 0); *K* := (0, 0, 1); ρ*L*.x := 0; ρ*R*.x := 0; *r* := 0.02. |
| 44, 73 (0x414a6e) | 75, 110 | Five steps, first matching: (1) *t* < P1: *H* := (10, 0, 0); *T* := (−20, 0, 0); *S* := (0, −1, 0); *K* := (0, 1, 1); ρ*L*.x := 20; ρ*R*.x := 20; Δ*L*.z := 2; Δ*R*.z := 2; *Q* := (30, 0, 10). (2) *t* < P1 + P2/3: *H* := (−30, 0, 0); *T* := (50, 0, 0); *S* := (0, 0, 1); *K* := (0, −4, 2); ρ*L*.x := 60; ρ*R*.x := 70; *Q* := (−30, 0, 0). (3) *t* < P1 + (2·P2)/3: *H* := (−30, 0, 0); *T* := (40, 0, 0); *S* := (0, 1, 1); *K* := (0, −2, 2); ρ*L*.x := 70; ρ*R*.x := −30; Δ*R* := (0, 3, −2); *Q* := (−30, 0, −10). (4) *t* < P1 + P2: as (3) but *T* := (10, 0, 0), ρ*L*.x := −30, ρ*R*.x := −40, and Δ*L* := (0, 3, −2) instead of the Δ*R* write. (5) *t* < D: *H* := (10, 0, −10); *T* := (−20, 0, 0); *S* := (0, 1, 0); *K* := (0, 1, 1); ρ*L*.x := 20; ρ*R*.x := 20; Δ*L*.z := 2; Δ*R*.z := 2; *Q* := (30, 0, 10). |
| 45 (0x41550e) | 76 | **W**: *T*.x := −10; *H*.x := 10; *Q*.x := −60. **A**: ρ*L*.x := 80; ρ*R*.x := 80; *H*.x := −60; *B*.x := −360·(*t* − P1)/P2; *K* := (0, −3, 0). **R**: *T*.x := 20; *H*.x := −10; *K* := (0, −2, 0). |
| 46 (0x415420) | 77 | **W**: *T*.z := −10; *T*.y := 30; ρ*L*.y := −30; ρ*R*.y := −30; *H*.z := 60. **A**: ρ*L*.y := 80; ρ*R*.y := 80; *B*.y := −360·(*t* − P1)/P2. **R**: *T*.y := −30; ρ*L*.y := 30; ρ*R*.y := 30. |
| 47 (0x415332) | 78 | **W**: *T*.z := 10; *T*.y := −30; ρ*L*.y := 30; ρ*R*.y := 30; *H*.z := −60. **A**: ρ*L*.y := −80; ρ*R*.y := −80; *B*.y := 360·(*t* − P1)/P2. **R**: *T*.y := 30; ρ*L*.y := −30; ρ*R*.y := −30. |
| 48 (0x4168ac) | 65 | *U* := record 0x138 vector. **t < P1/2**, *q* = min(*t*/(P1/2), 1): *T*.y := 10*q*; *T*.z := −80*q*; ρ*R* := (−90, 90*q*, −30*q*); G((2, 0, 0)) with arm lift; *r* := 0.01. **P1/2 ≤ t < P1**: *T* := (10, 0, −90); ρ*R* := (0, 110, −20); G((2, 0, 0)) with arm lift; *r* := 0.01. **P1 ≤ t < D**: *T* := (0, 0, 90); ρ*R* := (0, 90, 90); G((−2, 0, 0)) with arm lift; *r* := 0.01. **E**: pose id := 0; G((−2, 0, −1)) with arm lift; ρ*R*.y := 90; *T*.z := −80. **Every phase**, then: *B*.z := 360·clamp((*t* − P1)/P2); ρ*L* := ρ*R*. (The third branch also writes *B*.z := 20 before this final assignment replaces it.) |
| 49 (0x416d25) | 61, 66; 93 with an empty off-hand slot | *U* := record 0x138 vector. **W**: *B*.z := 30; *T* := (0, 0, 90); ρ*R* := (−90, −90, 90); G((2, 0, 0)) with arm lift; ρ*L* := ρ*R*; *r* := 0.005. **P1 ≤ t < D**: *T* := (0, 0, 0); ρ*R* := (−90, −90, 0); *E*.x := 0.25 while *t* < P1 + P2; G((−2, 0, 0)) with arm lift; *B*.z := −360·min((*t* − P1)/P2, 1); ρ*L* := ρ*R*; *r* := 0.03. **E**: pose id := 0; G((−2, 0, −1)) with arm lift; ρ*R* := (−90, 90, 0); *T*.z := 0; ρ*L* := ρ*R*. |
| 50 (0x41d62d) | 20, 21 | First: ρ*R* := (60, −20, 20); Δ*R* := (0, 2, 4); ρ*L* := (60, 20, −20); Δ*L* := (0, 2, 4). **W**: *T*.z := 50; ρ*L*.x := −90; ρ*L*.z := 90; ρ*R*.x := −90; Δ*L* := (−4, −4, −2); Δ*R* := (0, 4, 1). **A**, *q* = clamp(1.5·(*t* − P1)/P2): *B*.z := −270*q*; *B*.y := −40*q*; *S*.z += 4; ℓ := 40*q*; ρ*L*.x := −90; ρ*L*.z := 90; ρ*R*.x := −90; Δ*L* := (−4, −4, −2); Δ*R* := (0, 4, 1); *H*.x := −30; *H*.z := −60; *r* := 0.03. **t ≥ P1 + P2**: guard idle (*k* = 0.005, *r* unchanged). |
| 51 (0x41414f) | 80 | While the record action time is below the held item's use limit (record 0x1d8 item; §6.5): *H* := (45 + 5·cos(0.02·*t*), 0, 0); *T* := (10, 0, 0); *B* := (10, 0, 0); Δ*L* := (1, 7, 8); Δ*R* := (−1, 7, 8); ρ*L* := (100, 0, 0); ρ*R* := (100, 0, 0). |
| 52 (0x41409a) | 81 | The writes of pose 57, then the case of pose 51. |
| 53 (0x4142fc) | 82 | *T* := (−30, 0, 0); Δ*L* := (4, 6, 6); Δ*R* := (−4, 6, 6); *S*.y := 3. |
| 54 (0x4143ab) | 79 | Only while the record's movement intent (record 0x30, 3-D) has a squared length > 0: *B*.x := 20; *T*.x := −10; *H*.x := −20; ρ*L*.x := −20; ρ*R*.x := −20; Δ*L*.y += 4, Δ*R*.y += 4; *F* := −40; both foot bases y += 4 and z += 3. |
| 55 (0x413f91) | 84 | *B* := (−90, 0, 0); *H*.x := 20; *E*.z := (box.y − box.z)·0.5 − 0.15; *K* := (0, −1, 0); Δ*L* := (0, 0, 3); Δ*R* := (0, 0, 3). |
| 56 (0x41e576) | any, while record 0x120 > 0 | *T*.x += 30; Δ*L* := (−2, 0, 0); Δ*R* := (2, 0, 0); ρ*L* := (−10, −10, 90); ρ*R* := (−10, 10, −90). |
| 57 (0x413afd) | 83 | *E*.z := −0.14; *T*.x := 10; Δ*L* := (0, 4, 0); Δ*R* := (0, 4, 0); *K* := (0, 2, −2). |
| 58 (0x413c3d) | 106 | If a mount creature is passed to 0x4128f0 (last argument): *E*.y −= 0.25; *E*.z += 0.75·(mount box.z) + 0.3·(4·*a*·*s₀*), with *a* and *s₀* the drawn creature's own walk amplitude and swing base (§7.4 step 1); then *E* += a seat offset by mount creature type (0x446950): types 26 and 30 → (0, 0, −0.9), 33 → (0, 0, −0.2), 74 → (0, 0, −0.3), 75 → (0, 0, 0.1), 151 → (0, 0, −0.35), any other → 0; mount types 22 and 23 → *E*.z −= 0.75; *h* := −*E*.z. Always: *T*.x := 20; Δ*L* := (4, 6, −3); Δ*R* := (−4, 6, −3); *K* := (0, 2, −2); ρ*L* := (60, 90, 60); ρ*R* := (60, −90, −60). |
| 59 (0x413e2b) | 107 | *T*.x := 20; *E*.z += 1.25; *B*.z := −50; Δ*L* := (4, 6, −3); Δ*R* := (−4, 6, −3); *K* := (0, 1, 0); *H* := (0, 0, 30); ρ*L* := (60, 90, 60); ρ*R* := (60, −90, −60). |
| 60 (0x418cdf) | 86 | *t* < 600: as pose 28 W. 600 ≤ *t* < 5000: *T*.z := 0; *B*.z := 360·(*t* − 600)/300; ρ*L* := (0, −90, 0); ρ*R* := (0, 90, 0); *r* := 0.2. *t* ≥ 5000: as pose 28's last branch. |
| 61 (0x41b08b) | 30–34, 49, 87–90, 92, 105 | **W**: *T*.x := 20; *H*.x := 20; Δ*L* := (3·cos(0.005·*t*) − 1, 5, 5·sin(0.005·*t*) + 5); Δ*R* := (3·cos(0.005·*t* + π) + 1, 5, 5·sin(0.005·*t* + π) + 5). **A**: *T*.x := −20; *T*.z := 10; Δ*L* := (2, 4, 0); Δ*R* := (−2, 6, 0); ρ*L* := (0, 45, 0); ρ*R* := (0, −45, 0). **R**: Δ*L* := (−3, −6, 0); Δ*R* := (3, −6, 0); ρ*L* := (−90, 0, 130); ρ*R* := (−90, 0, −130); *r* := 0.03. |
| 62 (0x41b3f0) | 35 | All times: as pose 61 W. |
| 63 (0x418e84) | 91 | Nothing while *t* ≥ P1 + P2 + 400. Otherwise, with *k* = *t* − P1 + 100: if *t* ≤ P1, or ⌊*k*/500⌋ is odd → "both": Δ*L* := (2, −2, 12), Δ*R* := (−2, −2, 12), *T*.x := 20, *B*.x := 10. Else if ⌊*k*/1000⌋ is even → "right": Δ*L* := (0, 0, −5), Δ*R* := (−2, −2, 12), *T* := (−10, 0, −20), *B* := (−5, 0, −10), *r* := 0.03. Else → "left": Δ*L* := (2, −2, 12), Δ*R* := (0, 0, −5), *T* := (−10, 0, 20), *B* := (−5, 0, 10), *r* := 0.03. (The sequence after P1 is right, both, left, both, right, … in 500 ms steps offset by 100 ms.) |
| 65 (0x41a2fd) | 96 | While the record action time is < D: *T* := (0, 0, 90·sin(0.03·*t*)); Δ*R* := (0, 0, 0); Δ*L* := (0, 0, 0); *r* := 0.04. |
| 66 (0x417f9e) | 26 | **If obj +0x13b4 > 0** (meaning *unresolved*): W: *T*.z := −90; Δ*R* := (2, −2, 1); ρ*R* := (0, 10, 0); the off hand draws the main-hand weapon model. P1 ≤ *t* < P1 + 300: *T*.z := 10; Δ*L* := (−2, −2, 1); ρ*L* := (0, −10, 0); the off hand draws the main-hand model and the main-hand model is removed. P1 + 300 ≤ *t* < D: both weapon models removed; *T*.z := 90. In every branch then *r* := 0.03 and, while P1 < *t* < D, *B*.z := min(360·(*t* − P1)/200, 700). **Otherwise**: W: *T*.z := −90; Δ*R* := (2, −2, 1); ρ*R* := (0, 10, 0). P1 ≤ *t* < D: *T*.z := 90; main-hand model removed. |
| 67 (0x41825e) | 27 | *T*.z := −90·(record 0x134); Δ*R* := (2, −2, 1); ρ*R* := (0, 10, 0); wobble (z); if record 0x134 > 0, the off hand draws the main-hand weapon model. |
| 68 (0x41b689) | 101, 103 | **W**: *T*.x := −20; ρ*L* := (−90, 0, 0); ρ*R* := (−90, 0, 0); Δ*R* := (1, −2, 0); Δ*L* := (−1, −2, 0). **t ≥ P1**: *T*.x := 0; ρ*L* := (0, 45, 0); ρ*R* := (0, −45, 0); Δ*R* := (−2, 5, 0); Δ*L* := (2, 6, 0). |
| 69 (0x41916a) | 104 | **t < P1 + P2**: *r* := 0.03; *B*.z := −360·*t*/(P1 + P2); ρ*R* := (−90, −90, −90); Δ*R* := (4, 3, 0); *T*.z := −60·*t*/(P1 + P2); ρ*L* := (0, 0, 90). **Otherwise**: ρ*L* := (0, 0, 90); ρ*R* := (−60, −60, −60). |
| 70 (0x4144af) | 51 | **W**: *B*.x := −30. **A**: *B*.x := 120. |
| 71 (0x4183ac) | 108 | **W**: Δ*R* := (−3, −2, 10); Δ*L* := (3, −2, 10); the off hand draws model 2101 (`iron-deposit`); the frame's weapon scale value is multiplied by 1.5; *T*.x := 40; *B*.x := 10. **A**: *r* := 0.01; Δ*R* := (0, 5, 0); Δ*L* := (0, 5, 0); *T*.x := −20; *B*.x := −40. **t ≥ P1 + P2**: Δ*R* := (2, −1, 0); Δ*L* := (−2, −1, 0). |
| 72 (0x413bb1) | 109 | While *t* < 5000: *T*.x := 20; *H*.x := 30; Δ*L* := (0, 0, 16); Δ*R* := (0, 0, 16). |

---

## Re-certification (2026-11-15)

Every section was re-checked against `Cube.exe` (SHA-256 `84a7a132…30bf`) only: the
loader 0x4ee230 (all 2557 load calls), the constructor 0x428750, the generator 0x43f7c0
(all 156 jump-table cases at 0x443ca0, read instruction by instruction), 0x42c080,
0x44be40, 0x444680, the key-colour and mesher routines, the item selectors, 0x4b4ef0,
0x4128f0 with its pose cases and draw tail, and the walk-cycle and display steps of
0x60c510. Reconstruction leads (Infinite-Cubes) were **not** consulted. Implementer
advice, verification procedures, "resolves X-n" history notes and in-text "correction to"
notes were removed; the text was rewritten as independent prose.

### Corrections (old → new; evidence)

Marked **[REVERSAL]** where a claim was inverted and **[DRAW ORDER]** where the order of
draws (random or render) changed.

1. §1: "74 pose cases" → the jump table has 73 entries (poses 1–73; bound check at
   0x413aed); poses 12, 13, 14, 36, 64 have no case and 44/73 share one.
2. §3.2: "head pitch 0" → body pitch (offset 0x48) 0 (0x428750).
3. §3.3 **[REVERSAL]**: 0x444680 was cited as the quadruped test and as excluding
   0x0800/0x1000 "together with 0x0200"; it tests no single flag bit — it returns true only
   when flags & 0x1a00 = 0 and the type is in a fixed list (0x444680).
4. §3.3: "seen on" lists rebuilt from the per-type writes of 0x43f7c0. Slimes (0x29) and the
   snout beetle (0x29) do **not** carry 0x0010; 0x0001 is also set on slimes, the
   frightener, beetles and the dragon; 0x0020 is on every flagged type except 86 and 97;
   bit 0x0004 (type 118, 0x44069a) and run-time bit 0x2000 (0x60c9c6) were added.
5. §5.1: "flag 1 for 2068 models" → 2068 load calls with flag 1 and 489 with flag 0
   (2557 calls); 2550 slots are filled and 2061 of them end with flag 1 (0x4ee230).
6. §5.1 rule 3 re-stated from the calls: the meshed voxel is tested with the model's key
   flag (e.g. 0x4e7938..0x4e7946), neighbours in strict mode (e.g. 0x4ea1fe..0x4ea203).
7. §4.2 / Appendix B header: the hair-colour list "types 0–5, 7–14 and 148–149 copy" (which
   contradicted "13–14 set white") → 0–5, 7–12, 148–149 copy; 13–14 white; 15–16 copy via
   0x428920; 6 and 46 draw.
8. Appendix B type 119 **[DRAW ORDER]**: old order head, hand, foot, body, tail with
   separate hand and foot draws → head (draw 1), body (draw 2), hand **and** foot from one
   shared draw 3, tail (draw 4), then draws 5–8 as before (0x443ad9..0x443c48).
9. Appendix B: boxes of types 4, 5, 9, 10, 48, 49, 81, 87–93 are computed from the base box
   (÷1.2, ×1.5, ×1.1, ×0.7, ×0.9 in single precision; 0x43fc40, 0x440aaa, 0x4412bc,
   0x44144d, 0x441516); type 81's z is 3.2400002, not exactly 3.24.
10. Appendix B type 116: the box draw is made before the model writes (0x4439a6).
11. Header: server generator "byte-identical in size and logic" → same size only; logic
    identity and the server new-player defaults moved to §8 (U-4).
12. §4.1: the creation screen also derives the type itself, writes the class byte (record
    0x130) and uses 6/15 ranges for types without their own range (0x42c080).
13. §5.2: head tinting (was open) → the head draw sets no material and is not tinted
    (0x420433..0x4206c7).
14. §5.2 / §6.5 shimmer: boots were missing; both feet set shimmer from slot 3 (0x423ae4,
    0x423d03); the right hand sets none; the lantern clears it (0x423352).
15. §6.1: item types 14, 16, 17, 18 were "unknown" → named from their models; level is
    16-bit read signed, default 1 dropped (0x4c70b0).
16. §6.3: chest material 6 with subtype > 6 was unstated → falls into the shoulder default
    1149 + 5v + r (0x4ec5eb..0x4ec5f4, 0x4ec94d).
17. §6.4 held item: "at the right hand" → at the midpoint of the two hands, during actions
    80/81 below the use limit (0x420de8..0x420ec7, 0x4c6b80).
18. §6.4 slot 12: "in the hand when a pet is held (inferred)" → at the hands' midpoint
    during action 82 (0x420f5a).
19. §6.4 slot 11: glider (mode 0x10) and boat (action 107) are independent tests (0x41f726,
    0x41f8c8).
20. §6.4 additions: head, hair, tail, arm and wing are appearance-only; an absent hand model
    also suppresses the lantern (0x422ff4); slot offsets 0x2f0 + 0x118·k.
21. §7.5 **[DRAW ORDER]**: gliders/boats/special items were drawn last (#15) → glider
    (0x41f72d), boat (0x41f8c6) and mana cube (0x41fa97) are drawn first, before the chest.
22. §7.5 **[DRAW ORDER]**: the two bow back-quivers (0x421092, 0x421269) come before the
    main weapon; the slot-6 quiver (0x42243d) after the off-hand weapon; the lantern between
    the left and right hands (0x423219); the left hand before the right; the −x wing before
    the +x wing; left foot before right foot.
23. §6.5 **[REVERSAL]** sheathed main weapon: "Ry(−45), −135 for a staff" → −45 for a staff
    (subtype 10), −135 for every other subtype (0x4215b7..0x4215e2).
24. §6.5 sheathed main weapon: "same grip translation as in the hand" → always centred
    (0x421634 → 0x42198a).
25. §6.5 grip tables: the tests are made in a fixed order (arrow, staff, bracelet, bow,
    crossbow, boomerang, fist, other for the main hand).
26. §6.5 glow: "each weapon" → main weapon in hand or sheathed (0x421467); off-hand weapon
    only in the hand (sheathed path skips 0x42227b).
27. §7.7 step 9 **[REVERSAL]** knocked down: upright *E*.z = (box.y − box.z)/2, quadruped
    (box.x − box.z)/2 — the old text had box.x and box.y swapped (0x41e711, 0x41e787).
28. §7.4 step 3 quadruped hand targets: y is *h*.y + 2.1 − 5*a* (was *h*.y) (0x413567 →
    0x41369e).
29. §7.4 step 7 base translation: "creature position − camera position" → displayed x/y
    plus two caller-supplied 64-bit offsets; z = displayed z − ⌊bob·65536⌋ + ⌊E.z·65536⌋
    with no offset (0x41f19d..0x41f220).
30. §7.4 step 1: swing constant 2.19911485751 → stored double 2.19911482006215 (0.7π rounded
    to single); phase + π/2 is rounded to single before the sine (0x412df0..0x412e26).
31. §7.4 step 1: the "unknown limit" of poses 51/52 → the held item's use limit (0x4c6b80).
32. §7.7 step 13: the action-28 vector follows normalised record field 26, not the
    velocity, with its own rate-0.01 weight (0x41ef87..0x41f02c).
33. §7.4 step 2: the riding leg pose also requires record byte 0x50 ≠ 5 (0x413479).
34. §7.4 step 7 dodge: "field 15 > 0" → field 15 ≠ 0 (0x41f22b); the hand raise applies to
    the composed hands even when the roll is skipped, and twice when also gliding.
35. §7.1: mount id row "record 0x1b8 (obj +0x11c8)" → client-only obj +0x11c0 (u64); inputs
    actually read were added.
36. §7.6: walking hand targets (5.43, 7.80, 1.9) / (−5.43, −13.60, 1.9) → (5.448, 7.792, 1.9) /
    (−5.448, −13.592, 1.9).
37. §7.4 step 6: shortest-arc step restated from 0x423f70 (asin of clamped sin·cos − cos·sin).
38. §7.2 **[REVERSAL]** action 93 with an empty off hand: pose 2 → pose 49 (0x4b52dc →
    0x4b4fbd).
39. §7.2 action 48: "any off-hand weapon → 35" → off-hand item type 3 and subtype ≠ 13 → 35;
    off-hand type other than 3 → 34 (0x4b51f0..0x4b5225).
40. §7.3 **[REVERSAL]** missing mount: "a mount that no longer exists or has health ≤ 0
    clears the id" → only an existing mount with health ≤ 0 clears it; an unresolved id is
    kept (0x61e9c5 → 0x61e9e6).
41. §7.3 riding: "the rest of the step is skipped" → only the phase/amplitude increments;
    the flag clears, clamp and three relaxations still run (jump to 0x61f000).
42. §7.3: the size value *S* (was open) → collision box x (0x619137..0x619151).
43. §7.3 footsteps: the sound is placed at the creature position, the ground probe at
    position.z − 0.5·box.z − 0.5 block, block type = type byte AND 0x1F; footsteps only in the
    on-ground non-climbing branches; the gate is a non-null local-player reference
    (0x61ed8f..0x61ef06, 0x434c10).
44. §7.3 additions: gliding bit 0x10 cleared unless slot 11 holds a glider (type 23 subtype 0);
    lantern bit 0x200 cleared unless slot 10 holds type 24 (0x61f008..0x61f03b).
45. §7.3: relaxation is *dt* successive single-precision steps (0x411d40); the "power"
    equivalence advice was removed.
46. §7.10: two-stage fixed-point position smoothing (all axes by trunc(Δ·wf/65536), then z
    again by trunc(float Δz·w)) (0x42c580, 0x42c900, 0x42c980, 0x61fc09..0x61fc4f).
47. §7.10 attach rule (condition was "not traced") → record byte 0x50 = 5, the creature id at
    record 0x188 resolves, and that creature's action is 106 (0x61f968..0x61f994);
    "carrying" label dropped.
48. §7.8 STANCE: "unless the action is 87 or 96" → unless the creature **type** is 87 or 96
    (0x4199a7 and the other stubs); pose 29 has no such test.
49. §7.8 STANCE shield rule: "(−80, 40, 60) skipped with a shield" → poses 10 and 18 write
    (0, 0, 90) with a shield; other entries always write (−80, 40, 60) (0x41baa4, 0x41d3b6).
50. §7.8 Grip G: the arm lift is per case, not part of G (absent in pose 2 P1..D, pose 8
    below 500 ms, poses 30–33).
51. Appendix C pose cases (0x413afd..0x41e660): every former "n.t." cell resolved and the
    following values changed — pose 1 500..D *B*.x curve and both ρ.x = −150 (0x4178ba);
    pose 2 phase split P1..D / E (0x416625, 0x4167b8); pose 3 middle phase ends at P1 + P2/2
    with Δ*R* (−8, −4, 20), W Δ*R* (−5, 8, 0) (0x417169..0x417247); pose 4 W is *t* < P1/2
    (0x416052, 0x416398); pose 6 non-specialised Δ*L* (−1, 0, 1) (0x419ea1); pose 7 *U* blend
    of record 0x144/0x138 (0x417ab1..0x417bcf); pose 8 Δ*L*.z is a channel offset
    (0x419f88); poses 9/10 action-48 term negative, −360·*t*/300 (0x41b5e9, 0x41b8eb); pose
    10 *L*.x += sin(0.1·(*t* − 600)) (0x41b9b1); pose 15 spin divisor P2 (0x41dbc3) and shield
    rule in every phase (0x41dfe8); pose 17 W Δ*L*.y −= 6, not Δ*R* (0x41d47a); pose 18 ρ*L*
    and Δ*R* resolved; poses 19/20 extra Δ writes and *S*.z sine; pose 21 limit 2·D, second
    limit D/2, hand fix-ups were swapped (0x4185d1); pose 22 extra *t* < 300 writes
    (0x417dc5, 0x417f46); pose 24 "(also *L*.z)" removed; pose 30 spin window P1 + P2/2, W
    rate 0.04, blends resolved; pose 31 spin subtracted; pose 32 W rate 0.04, blends
    resolved; pose 33 W rate 0.02; pose 37 E net effect; poses 38, 39, 42, 43 boundary
    P1 + P2/2 and *Q* values (39: (−10, 0, 0); 42: (−20, 0, 0); 43: (−50, 0, 0)); pose 41
    also removes the head and does not reset the pose id (0x415a78..0x415a96); pose 44/73
    thresholds and step-5 *S* (0, 1, 0) (0x415063); pose 45 R *K* (0, −2, 0); pose 48 phase
    split and *B*.z override (0x416c9f); pose 49 adds ρ*L* := ρ*R*; pose 50 keeps the default
    rate after its active phase; pose 54 foot z += 3 (0x41391b); pose 56 ρ*R* (−10, 10, −90);
    pose 58 seat-offset table of 0x446950; pose 60 *B*.z uses *t* − 600 (0x418d93); pose 63
    active until P1 + P2 + 400 with a 500/1000 ms alternation (0x418e84..0x419151); poses
    65, 66 (middle phase ends at P1 + 300), 67, 69, 71 resolved.

### Confirmations

* Appendix A: all 2569 slots match the load calls of 0x4ee230, including the empty and
  doubly loaded slots; the key-flag 0 set of §5.1 matches exactly.
* §3.1/§3.2: layout size 172 and every default value (0x428750).
* §4.1: race/sex table, the five no-record draws and their colour mapping (B, G, R), the 20-
  byte record read by 0x44be40.
* §4.3 example and Appendix B: every model index, modulus, flag value, scalar and vector of
  all 156 types (0x43f7c0), apart from the items listed above.
* §5.1: key colours, list offsets, scan order, the hair-model voxel counts (data1
  `human-hair-m01`), and the red voxels of `troll-head`, `santa-head`, `santa-body`.
* §6.1–§6.3.1 and §7.11 item 9: every item model rule (0x51be60, 0x4ec370, 0x4ec400).
* §7.2: every action→pose cell except action 93; field 17 → pose 56 (0x4b532f).
* §7.3–§7.7, §7.9, §7.10: constants, rates, channel offsets, smoothing order, rotation
  orders and frames, except the items listed above; worked examples 1–8 recomputed.

### Unresolved

U-1 … U-14 of §8.
