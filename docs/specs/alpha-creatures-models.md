# Alpha creatures: models, appearance, equipment and poses

Behavioural specification of how the Cube World **Alpha** client (`Cube.exe`, SHA-256
`84a7a132…30bf`) builds and draws a creature: the model table, the 172-byte appearance
record, the creature-type table that fills it, colours, equipment models, and the
procedural pose system. Written from the Alpha decompile and checked against the
executable's assembly and constants; the server (`Server.exe`) contains the same
appearance generator (server 0x40a840 = client 0x43f7c0, byte-identical size and logic).

Related: `alpha-net-packets.md` (§1.2 appearance, §2.1 creature record — this document
resolves open question **P-2** and part of **P-11**), `alpha-rendering.md` §11 (creature
lighting and material), `alpha-data-stores.md` (`.cub` models), `alpha-saves.md` (§3.2
field 13), `alpha-player-control.md`, `alpha-creatures-behaviour.md`.

---

## 0. Conventions

* Addresses are `Cube.exe` virtual addresses (base 0x400000) unless marked **server**.
* "Record offset" = offset in the 4456-byte network creature record. The client's creature
  object holds that record at object offset +0x10, so object offset = record offset + 0x10.
  Client-only state beyond the record is given as object offsets ("obj +0x1188").
* "Appearance offset" = offset inside the 172-byte appearance record (record offset 0x68,
  object offset +0x78).
* Lengths are in **model units** unless stated: one voxel of an unscaled model. How model
  units become blocks is §7.4. Angles are in **degrees** unless stated.
* "draw" = one call of the C runtime `rand()` (15-bit, 0..32767) of the thread doing the
  work; the generator's draws use the caller's random state (world generation re-seeds
  per zone, see `alpha-worldgen-random.md`). "draw *k*" = the *k*-th draw made by the
  routine being described.
* Float maths is single precision unless a step is marked (double).

---

## 1. Where things are

| Routine | What it does |
|---|---|
| 0x4ee230 | loads every voxel model from `data1.db` into the **model table** (§2) |
| 0x428750 | default appearance record (§3.2) |
| 0x43f7c0 (**server** 0x40a840) | **appearance generator**: creature type + optional character-creation choices → appearance (§4) |
| 0x44be40 | character loader; regenerates type and appearance from saved field 13 (§4.1) |
| 0x42c080 | character-creation screen; wraps the head/hair choices (§4.1) |
| 0x4ec400 | item → model (§6) |
| 0x51be60 | weapon (item type 3) → model index (§6.2) |
| 0x4ec370 | consumable (item type 1) → model index (§6.3) |
| 0x4b4ef0..0x4b5342 | per frame: creature action id → **pose id** (§7.2) |
| 0x4128f0 | pose and draw of one creature (§7) |
| 0x60c510 (≈0x61e9e6..0x61f0f1) | per simulation step: walk-cycle phase/amplitude and other animation decays (§7.3) |
| 0x413ae9..0x41e660 | the 74 pose cases of 0x4128f0 (jump table at 0x423d4c, index = pose − 1; §7.8, Appendix C) |
| 0x41e660..0x41f147 | pose-state smoothing and the hand/arm composition (§7.7) |
| 0x41f147..0x423d33 | matrices and draws of every part, equipment included (§6.5, §7.9) |
| 0x61fb7a..0x61fd54 | displayed position and body rotation (obj +0x1350, +0x1374) (§7.10) |

---

## 2. The model table

At start-up 0x4ee230 creates 2569 empty model slots and then loads named `.cub` assets from
`data1.db` (`alpha-data-stores.md`) into fixed slots. Every number in this document that
names a model ("model 430") is a slot of this table. Slots never loaded stay empty
(1559–1563, 1589–1593, 2060, 2061, 2071, 2353–2356, 2419, 2420). A few slots are loaded
twice; the later load wins (slot 1300 ends as `orc-head.cub`; 1224–1228 and 1230 load the
same key twice). The complete table is Appendix A.

An index is valid if 0 ≤ index < 2569; the renderer treats any other value (65535 in
particular, i.e. −1 as a signed 16-bit value) as "no model".

---

## 3. The appearance record (resolves P-2)

### 3.1 Layout

| Off | Size | Type | Meaning |
|---|---|---|---|
| 0x00 | 2 | 2 × u8 | *unknown*; 0 by default and never written by the generator |
| 0x02 | 3 | 3 × u8 | **hair colour** R, G, B (§5) |
| 0x05 | 1 | — | padding |
| 0x06 | 2 | u16 | **appearance flags** (§3.3) |
| 0x08 | 12 | 3 × f32 | **collision box** size x, y, z in blocks (used by physics, `alpha-player-control.md`) and the model scale (§7.4) |
| 0x14 | 2 | u16 | model: **head** |
| 0x16 | 2 | u16 | model: **hair** (second head layer; eyes for skeletons, imps, trolls, lantern fish) |
| 0x18 | 2 | u16 | model: **hand** (drawn twice, mirrored; front legs of quadrupeds) |
| 0x1a | 2 | u16 | model: **foot** (drawn twice, mirrored; hind legs of quadrupeds) |
| 0x1c | 2 | u16 | model: **body** |
| 0x1e | 2 | u16 | model: **tail** |
| 0x20 | 2 | u16 | model: **arm** (upper arm of large humanoids: trolls, golems, minotaur, lich) |
| 0x22 | 2 | u16 | model: **wing** (drawn twice, mirrored) |
| 0x24 | 4 | f32 | head scale (head and hair) |
| 0x28 | 4 | f32 | body scale (also multiplies tail, shoulder pads and body-worn items) |
| 0x2c | 4 | f32 | hand scale (hands and anything held in the hand) |
| 0x30 | 4 | f32 | foot scale |
| 0x34 | 4 | f32 | arm scale |
| 0x38 | 4 | f32 | weapon scale (both weapon slots) |
| 0x3c | 4 | f32 | tail scale |
| 0x40 | 4 | f32 | shoulder-armour scale |
| 0x44 | 4 | f32 | wing scale |
| 0x48 | 4 | f32 | body pitch (degrees): added to the body's x rotation and subtracted from the head's, so the head stays level |
| 0x4c | 12 | 3 × f32 | hand rotation at rest (degrees; right hand (x, y, z), left hand (x, −y, −z)) |
| 0x58 | 4 | f32 | foot angle (degrees, added to the leg swing rotation) |
| 0x5c | 4 | f32 | wing angle (degrees, x rotation of the wings) |
| 0x60 | 4 | f32 | tail angle (degrees, x rotation of the tail) |
| 0x64 | 12 | 3 × f32 | body offset |
| 0x70 | 12 | 3 × f32 | head offset |
| 0x7c | 12 | 3 × f32 | hand offset (right hand; the left hand mirrors x) |
| 0x88 | 12 | 3 × f32 | foot offset (right foot; the left foot mirrors x) |
| 0x94 | 12 | 3 × f32 | tail offset |
| 0xa0 | 12 | 3 × f32 | wing offset |

Offsets are in model units relative to the creature's pivot (§7.4); x is to the
creature's right, y forward, z up. The scalar names come from which drawn part each one
multiplies in 0x4128f0 (§7.5); 0x4c–0x60 are named from their single use each.

### 3.2 Defaults (0x428750)

A fresh appearance holds: bytes 0x00–0x01 = 0; hair colour (255, 255, 255); flags 0;
collision box (1, 1, 1); all eight model indices **65535 (none)** (0x4287bb sets them
from −1); scales head 1.01, body 1, hand 1,
foot 0.98, arm 1, weapon 0.95, tail 0.8, shoulder 1, wing 1; head pitch 0; hand rotation
(0, 0, 0); foot/wing/tail angles 0; body offset (0, 0, −5); head offset (0, 0.5, 5); hand
offset (6, 0, 0); foot offset (3, 1, −10.5); tail offset (0, −8, 2); wing offset (0, 0, 0).

An appearance is constructed with these defaults (0x428750 is the appearance constructor;
the spawn-object constructor 0x5adb20 calls it, *assumed* likewise for every creature)
before the generator runs, so a part
that a type does not set stays **none** and is not drawn: a human has no arm, tail or wing
model, a dog no hair, arm or wing. Model index 0 (`body2.cub`) is only drawn where a type
sets it explicitly (body of types 1, 4–6, 11, 12, 15, 16, 18, 44).

### 3.3 Appearance flags (0x06)

Set by the generator (bitwise OR into the existing value). Observed effects:

| Bit | Seen on | Effect found |
|---|---|---|
| 0x0001 | four-legged animals, fish | **quadruped**: hand parts are front legs; no arm swing, legs swing ×0.75 (§7.3); 0x444680 test |
| 0x0002 | birds, insects, bats, vampire, frightener, sand horror | **flyer**: when not on ground (contact bit 0 clear) the leg swing and bob are zero |
| 0x0008 | skeleton, slimes, runners, plant creatures, monkey, moles, crabs… | *unknown* |
| 0x0010 | fish, beetles, onionlings, bunnies, insects, birds, squid, slimes | **natural weapon**: no weapon combo; primary attack id 75 (`alpha-creatures-behaviour.md` §3.1), combo threshold 20 |
| 0x0020 | humanoids, most monsters | *unknown* (likely "upright") |
| 0x0100 | fish, lantern fish | **swimmer** (*inferred*: set on every water creature) |
| 0x0200 | — (set at run time) | highlight (`alpha-rendering.md` §11.2 step 5) |
| 0x0400 | skeleton, imp, frightener, dark troll, vampire, lantern fish | **glowing eyes**: the hair layer is the eyes (special handling in the hair draw) |
| 0x0800, 0x1000 | — (run time) | excluded together with 0x0200 by 0x444680; meaning *unknown* |

---

## 4. Creature types and the appearance generator

### 4.1 Inputs

The generator takes the creature type (record field 8, offset 0x54), the appearance to
fill, and optionally a 20-byte **character choice** record:

| Off | Size | Meaning |
|---|---|---|
| 0x00 | i32 | race 0..7 |
| 0x04 | u8 | sex: bit 0 = 1 female, 0 male |
| 0x08 | i32 | head choice |
| 0x0c | i32 | hair choice |
| 0x10 | 3 × u8 | hair colour R, G, B |
| 0x13 | 1 | padding |

This is exactly saved character field 13 (`alpha-saves.md` §3.2, "extra data" bytes
0–19), which resolves **V-2**: the creature type and appearance are not saved; on load
(0x44be40) the client calls the generator with the creature's type and this record, which
first recomputes the type from race and sex.

**Race and sex → type** (only when a choice record is given):

| Race | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|---|
| Base type | 2 (human) | 0 (elf) | 9 (dwarf) | 11 (orc) | 4 (goblin) | 7 (lizard) | 15 (undead) | 13 (frogman) |

type = base + (sex bit). A race outside 0..7 leaves the type unchanged.

**No choice record** (NPCs): the generator makes its own: draw 1 → blue, draw 2 → green,
draw 3 → red (each the low byte of the draw), draw 4 mod 100 → head choice, draw 5 mod
100 → hair choice, race and sex 0 (the type is **not** changed). Five draws are always
consumed in this case, before any type-specific draws.

The character-creation screen (0x42c080) keeps the head and hair choices within the
type's ranges (wrap to 0 above the last, to the last below 0), and sets class = menu
index + 1.

### 4.2 Order of work

1. Choice record handling as above.
2. Collision box := (0.96, 0.96, 2.16) (always, before the per-type step).
3. Per-type step (Appendix B): writes some of hair colour, flags (OR), box, part models,
   scales, angles and offsets. Anything not written keeps its previous value.
4. Types ≥ 156 write nothing more.

Choices are used as `choice mod n` with C semantics (sign of the dividend; negative
choices give negative remainders and therefore other model indices — the creation screen
never produces them). Hair colour is copied from the choice record by the player races
0–5, 7–12 and 148–149; types 13 and 14 (frogman) force (255, 255, 255); types 15 and 16
(undead) copy it; types 6 (bull terrier) and 46 (ogre) take draws 1–3 as R = draw 3,
G = draw 2, B = draw 1 (low bytes) *in addition* to the five no-record draws when no
record was given.

### 4.3 Worked example: a male human, choices (head 2, hair 7, colour 120/80/40)

Type = 2 + 0 = 2. Box (0.96, 0.96, 2.16). Head = 1246 + (2 mod 6) = 1248
`human-head-m03`; hair = 1252 + (7 mod 15) = 1259 `human-hair-m08`; hand = 430
`hand2` (head 1248 is not > 1248); foot 432 `foot`; body 1 `body4`; hair colour
(120, 80, 40). Everything else stays at the defaults of §3.2 (head scale 1.01, head offset
(0, 0.5, 5), hand offset (6, 0, 0), foot offset (3, 1, −10.5), body offset (0, 0, −5)).
With head choice 3 the head is 1249 and the hand becomes 431 `brown-hand` (darker skin
heads 04–06 get darker hands).

Server: for a new player the Alpha server sets the box to (1.2, 1.2, 2.7) and type 44
(`alpha-net-packets.md` §1.2); the client overwrites its own player's appearance from its
save, and it is the client's record that other players see.

---

## 5. Colouring

### 5.1 Key colours in models (0x4e7290, 0x4e7650, 0x4e71d0, mesher 0x4e7870)

Each model is loaded with a **key flag** (model object byte +0x55; the third argument of
the loader 0x4e7290). The model table loader 0x4ee230 sets it per model:

* **Key colours active** (flag 0) for 489 models: every hair layer (`*-hair-*`), the eye
  layers (`skeleton-eyes`, `imp-eyes`, `dark-troll-eyes`, `vampire-eyes`,
  `frightener-eyes`), all wood/obsidian staffs, wood wands, gold/silver bracelets, and
  the "random" variants of iron maces, wood bows, crossbows and boomerangs.
* **Key colours inactive** (flag 1) for the other 2068 models (heads, bodies, hands,
  feet, armour, items, buildings…).

With key colours active, three exact colours are **keys**:

| Key | R, G, B | Collected into (model object) |
|---|---|---|
| red | 255, 0, 0 | point list at +0x04 |
| green | 0, 255, 0 | point list at +0x10 |
| blue | 0, 0, 255 | point list at +0x1c |

Rules (only when the flag is 0):

1. At load (0x4e7650) the voxel coordinates (x, y, z) of every key voxel are appended to
   that key's list, scanning x outermost, then y, then z.
2. The mesher's "is this voxel empty" test (0x4e71d0) treats a key voxel as **empty**:
   it emits no faces for it. The colour must match exactly; (0, 255, 80) or (254, 0, 0)
   are ordinary colours.
3. **Neighbour tests do not use the key rule**: a face of an ordinary voxel is emitted
   only toward a neighbour whose colour is exactly (0, 0, 0) (or outside the model), and
   the occlusion sampler (0x4eb8d0, `alpha-rendering.md` §11.1) counts only (0, 0, 0)
   as empty. So key voxels still hide the faces of ordinary voxels that touch them and
   still darken them by occlusion — the hair's inner surface against the stand-in head is
   neither drawn nor lit as open.
4. Key voxels are **not** replaced by any colour; no skin, hair or eye colour is
   substituted. The lists are kept but the creature draw does not read them (M-7).

So in `human-hair-m01.cub` (18 × 16 × 16) the red stand-in head (298 voxels), the blue
(10) and green (4) eye markers vanish, and what is drawn is the white hair (463 voxels),
its light grey shading voxels and the skin-coloured (255, 211, 182) parts (286 voxels:
ears and face edges that belong to the hair model).

With the flag 1, pure red/green/blue are ordinary colours and are drawn: red eyes in
`troll-head`, `minotaur-head`, `undead-head-m03`; Santa's suit (`santa-head` 396, `santa-body`
358 red voxels); green eyes in `elf-head-m03`; red, green and blue trim on
`obsidian-chest`.

**Skin tone**: body, hand, head and foot models carry their skin as ordinary colours
(for example `hand2` (255, 182, 182) and (255, 211, 182)); there are no skin key
colours. Skin tone differences come from choosing other models (§4, hand 430 vs 431).

### 5.2 Tinting

* Body-part models are drawn with their voxel colours (`alpha-rendering.md` §11.1).
* The **hair layer** (appearance 0x16 model) is drawn with material
  (R/255, G/255, B/255, 1) of appearance 0x02–0x04 multiplied componentwise by the frame
  material (0x4128f0 near the hair draw). Because white hair voxels are (255, 255, 255)
  they take the hair colour exactly; the skin-coloured voxels of a hair model are tinted
  too. If appearance flag 0x0400 (glowing eyes) is set the hair layer uses the hair
  colour **without** the frame material, i.e. it is not darkened by night or shade.
  Whether the head is tinted as well is open question M-2 (no tint was found in the head
  draw).
* Skin tone is not a colour: it is the choice of hand model (human types 2 and 3,
  desert nomads 83/84) or a different head model.
* Equipment colour comes from its model; materials 1, 11, 12 and 22 shimmer
  (`alpha-rendering.md` §11.2).

---

## 6. Equipment and items

### 6.1 Item fields used for models (adds to `alpha-net-packets.md` §1.1, P-1)

| Off | Meaning (from 0x4ec400, 0x51be60, 0x444db0) |
|---|---|
| 0x00 | item type |
| 0x01 | subtype |
| 0x04 | **variant seed** (u32): picks one of several models |
| 0x0c | **rarity** 0..4 (values > 3 are treated as 4) |
| 0x0d | **material** |
| 0x10 | level (u16; 1 by default) |

Item types: 1 consumable, 2 recipe ("formula"), 3 weapon, 4 chest armour, 5 gloves,
6 boots, 7 shoulder armour, 8 amulet, 9 ring, 10 block, 11 resource/ingredient, 12 coin,
13 ice crystal, 14 *unknown* (models 2318–2321), 15 parrot beak, 16 *unknown*, 17
*unknown*, 18 *unknown*, 19 pet box / egg, 20 pet food, 21 special (keys, potions,
plaster…), 23 vehicle (glider, boat), 24 lantern, 25 mana cube.

Materials (from model names): 1 iron, 2 wood, 5 obsidian, 6 unique/NPC clothing, 7 bone,
11 gold, 12 silver, 18 saurian, 19 parrot, 20 mammoth, 22 ice crystal, 23 light,
25 silk, 26 linen, 27 wool.

Below, "v" = seed mod *n*, "r" = min(rarity, 4).

### 6.2 Weapons (type 3), 0x51be60

| Sub | Weapon | Model index |
|---|---|---|
| 0 | sword | material 5 → 2059, 7 → 2072, else 899 + 5·(seed mod 11) + r |
| 1 | axe | 5 → 2060 (empty slot), 7 → 2074, else 5·(seed mod 11 + 354) + r |
| 2 | mace | 2 → 664 + r, 7 → 2073, else 609 + 5·(seed mod 11) + r |
| 3 | dagger | 954 + 5·(seed mod 11) + r |
| 4 | fist | 1009 + 5·(seed mod 11) + r |
| 5 | longsword | 5·(seed mod 11 + 376) + r |
| 6 | bow | 674 + 5·(seed mod 11) + r |
| 7 | crossbow | 729 + 5·(seed mod 11) + r |
| 8 | boomerang | 784 + 5·(seed mod 11) + r |
| 9 | arrow | 840 |
| 10 | staff | 5 → 604 + r, else 434 + 5·(seed mod 11) + r |
| 11 | wand | 489 + 5·(seed mod 11) + r |
| 12 | bracelet | 11 → 544 + 5·(seed mod 6) + r, else 574 + 5·(seed mod 6) + r |
| 13 | shield | 2 → 1119 + r, else 1064 + 5·(seed mod 11) + r |
| 14 | quiver | 839 |
| 15 | greatsword | 5 → 1936, 7 → 1935, else 5·(seed mod 11 + 365) + r |
| 16 | greataxe | 5 → 1993, 7 → 1992, 18 → 1994, else 1937 + 5·(seed mod 11) + r |
| 17 | greatmace | 2 → 1995, 5 → 2053, 7 → 2052, else 1996 + 5·(seed mod 11) + r |
| 18 | pitchfork | 2331 |
| 19 | pickaxe | 2051 |
| 20 | torch | 842 |
| other | — | 899 |

`seed mod 11` uses the seed as a signed 32-bit value (C `%`); each family has 55 models
(5 plain + 10 × 5 "random" variants), so 11 variants × 5 rarities fill it exactly. Material
5 of an axe points at slot 2060, which is empty: such an axe is drawn without a model.

### 6.3 Armour, jewellery and other items, 0x4ec400

* **Chest (4)**: material 5 → 2054, 6 → by subtype 0..6 → 1229..1235 (innkeeper chest…),
  7 → 2067, 11 → 2062, 18 → 1224, 19 → 2369, 20 → 2375, 22 → 2360, 23 → 670,
  25 → 1669 + 5v + r, 26 → 1359 + 5v + r, 27 → 1459 + 5v + r, otherwise 1124 + 5v + r
  (v = seed mod 5, unsigned).
* **Gloves (5)**: 1 → 1174 + 5v + r, 5 → 2057, 7 → 2070, 11 → 2065, 18 → 1226, 19 → 2368,
  20 → 2374, 22 → 2362, 23 → 672, 25 → 1744 + 5v + r, 26 → 1434 + 5v + r,
  27 → 1534 + 5v + r, otherwise model 7 (`glove`).
* **Boots (6)**: 1 → 1199 + 5v + r, 5 → 2056, 7 → 2069, 11 → 2064, 18 → 1227, 19 → 2370,
  20 → 2376, 22 → 2361, 23 → 673, 25 → 1719 + 5v + r, 26 → 1409 + 5v + r,
  27 → 1509 + 5v + r, otherwise 433 (`boot`).
* **Shoulders (7)**: 5 → 2055, 7 → 2068, 11 → 2063, 18 → 1225, 19 → 2372, 20 → 2378,
  22 → 2359, 23 → 671, 25 → 1694 + 5v + r, 26 → 1384 + 5v + r, 27 → 1484 + 5v + r,
  otherwise 1149 + 5v + r.
* **Amulet (8)**: material 12 → 1594 + 5v + r, otherwise 1564 + 5v + r.
* **Ring (9)**: 12 → 1644 + 5v + r, otherwise 1619 + 5v + r.
* **Consumable (1)**: subtype 0 → 852, 1 → 849, 2 → 850, 3 → 851, 4 → 2395, 5 → 2092,
  6 → 2393, 7 → 2546, 8 → 2335, 9 → 2333, other → 843.
* Types 2 → 2421 (`formula`), 10 → 2075 (`cube`), 12 → 2084/2085/2086 (copper / silver
  (12) / gold (11)), 13 → 2357, 15 → 2367, 19 → 2080 (subtype 24 → 2164, 25 → 2161),
  24 → lantern 2433, 25 → manacube 2566.
* **Vehicle (23)** (resolves M-3): subtype 1 → 2548 `boat2`, any other subtype → 2547
  `glider`.
* Types 11, 14, 16–18, 20, 21 (resolves F-4) are below (§6.3.1). They are inventory,
  world-item and hand-held models. Any combination not listed returns **no model** (0).

#### 6.3.1 Per-subtype tables (0x4ec400)

**Resource (11)**, by subtype; "m" = material:

| Sub | Model | Sub | Model |
|---|---|---|---|
| 0 | by m: 1 → 2337 `iron-nugget`, 2 → 2365 `wood-log`, 11 → 2339 `gold-nugget`, 12 → 2338 `silver-nugget`, 13 → 2340 `emerald-nugget`, 14 → 2341 `sapphire-nugget`, 15 → 2342 `ruby-nugget`, 16 → 2343 `diamond-nugget`, 17 → 2364 `sandstone`, other → none | 14 | by m: 129 → 2350 `unholy-cube`, 130 → 2349 `ice-cube`, 131 → 2351 `wind-cube`, other → 2348 `fire-cube` |
| 1 | 2365 `wood-log` | 15 | 843 `mushroom` |
| 2 | 2366 `parrot-feather` | 16 | 2332 `pumpkin` |
| 5 | 2394 `plant-fiber` | 17 | 2334 `pineapple` |
| 6 | 2352 `cobweb` | 18 | 2391 `radish-slice` |
| 7 | 2373 `mammoth-hair` | 19 | 844 `shimmer-mushroom` |
| 8 | 2357 `ice-crystal` | 20 | 2097 `ginseng-root` |
| 9 | 2363 `ice-coated-yarn` | 21 | 2392 `onion-slice` |
| 10 | by m: 1 → 2344 `iron-cube`, 2 → 2347 `wood-cube`, 11 → 2346 `gold-cube`, 12 → 2345 `silver-cube`, other → none | 22 | 845 `heartflower` |
| 11 | 2160 `cotton` | 23 | 847 `prickly-pear` |
| 12 | 2389 `flask` | 24 | 846 `heartflower-frozen` |
| 13 | 2336 `spiribit` | 25 | 848 `soulflower` |
| 3, 4, ≥ 28 | none | 26 / 27 | 2390 `water-flask` / 2091 `snow-berry` |

**Type 14** ("digested leftovers"): by rarity *k* = rarity − 1: *k* > 3 → 2321; *k* < 0 →
2318; otherwise 2318 + *k* (`digested-leftovers01..04`).
**Type 16**: 2313 + (seed AND 1) (`painting01`/`painting02`).
**Type 17**: 2113 + (seed AND 3) (`vase` … `vase4`).
**Type 18**: subtype 1 → 2120 + (seed mod 3) (`undead-candle1..3`); otherwise 2117 +
(seed mod 3) (`candle01..03`). Here the seed is used **unsigned**.

**Pet food (20)**, by subtype (default → 2334 `pineapple`):

| Sub → model |
|---|
| 19 → 875 `bubble-gum`; 22 → 868 `vanilla-cupcake`; 23 → 869 `chocolate-cupcake`; 25 → 877 `cinnamon-role`; 26 → 879 `waffle`; 27 → 871 `croissant`; 30 → 859 `candy`; 33 → 858 `pumpkin-mash`; 34 → 863 `cotton-candy`; 35 → 857 `carrot`; 36 → 889 `blackberry-marmelade`; 37 → 853 `jelly-green`; 38 → 854 `jelly-pink`; 39 → 856 `jelly-yellow`; 40 → 855 `jelly-blue` |
| 50 → 870 `banana-split`; 53 → 864 `popcorn`; 55 → 876 `licorice-candy`; 56 → 865 `cereal-bar`; 57 → 890 `salted-caramel`; 58 → 891 `ginger-tartlet`; 59 → 892 `mango-juice`; 60 → 893 `fruit-basket`; 61 → 894 `melon-icecream`; 62 → 895 `bloodorange-juice`; 63 → 884 `milk-chocolate-bar`; 64 → 886 `mint-chocolate-bar`; 65 → 887 `white-chocolate-bar`; 66 → 885 `caramel-chocolate-bar`; 67 → 874 `chocolate-cookie` |
| 74 → 888 `sugar-candy`; 75 → 878 `apple-ring`; 86 → 880 `water-ice`; 87 → 862 `donut-chocolate`; 88 → 896 `pancakes`; 90 → 866 `strawberry-cake`; 91 → 867 `chocolate-cake`; 92 → 860 `lollipop`; 93 → 861 `softice`; 98 → 882 `candied-apple`; 99 → 881 `date-cookie`; 102 → 2408 `bread`; 103 → 897 `curry`; 104 → 872 `lolly`; 105 → 873 `lemon-tart`; 106 → 883 `strawberry-cocktail`; 151 → 898 `biscuit-role` |

**Special (21)**, by subtype (byte): 3 → 2422 `key1`; 4 and 5 → 851 `mana-potion`;
6 → 2550 `plaster`; 7 → 609 `iron-mace1`; 8 → 2363 `ice-coated-yarn`; 9 → 2395
`soup-ginseng`; any other → 2081 + (subtype AND 3) (`quest-item-amulet01`,
`quest-item-amulet02`, `quest-item-jewel-case`, `copper-coin`).

Pet-food subtypes are creature types (the pet the food tames); the subtype numbers
above are the keys of the type-20 switch in 0x4ec400; *inferred*: each matches the
creature type of Appendix B that eats it.

### 6.4 Equipment slots (record field 44, 13 items)

| Slot | Holds (item type) | Drawn as |
|---|---|---|
| 0 | *unknown* | not drawn |
| 1 | amulet (8) | drawn on the body (body scale), swinging with the walk |
| 2 | chest (4) | **replaces the body model** |
| 3 | boots (6) | **replace the foot model** |
| 4 | gloves (5) | **replace the hand model** |
| 5 | shoulders (7) | extra part on the body, shoulder scale × (body scale + 0.1) |
| 6 | weapon (3), off hand | in the left hand, weapon scale |
| 7 | weapon (3), main hand | in the right hand, weapon scale |
| 8, 9 | rings (9) | not drawn |
| 10 | lantern (24) | drawn below the **left** hand while mode flag 0x200 is set (§6.5) |
| 11 | vehicle (23) | glider drawn above the body while mode flag 0x10 is set; boat during action 107 (§6.5) |
| 12 | pet item | drawn in the hand when a pet is held (*inferred*) |

Record field 43 (offset 0x1d8, outside the equipment) is the **item held in the hand**
(drawn at the right hand with the hand scale).

If an equipment slot holds no item (type 0) or its model is missing, the appearance part
is used. A quiver (type 3, subtype 14) in the off-hand slot 6 is drawn on the back (weapon
scale, §6.5) instead of in the hand. Body parts are hidden by draw flags passed by the
caller (first-person and preview cases): bit 0 head, 1 hair, 3 body, 2 shoulders, 6 both
weapons, 4 left hand (and the lantern), 5 right hand, 7 left foot, 8 right foot.

"Left" and "right" in this document follow §3.1: the **right** hand is the +x hand. It
holds the main weapon (slot 7) and is the hand drawn mirrored (§7.9); the left (−x) hand
holds the off-hand weapon (slot 6) and the lantern.

### 6.5 How equipment is drawn (0x41f147..0x423d33)

Every equipment model is drawn with the part-matrix rule of §7.4 step 8: a parent frame,
then the listed translations and rotations, then the scale, then a **centring** translation
by fractions of the model's voxel size (*sx*, *sy*, *sz*). "Centred" means
(−*sx*/2, −*sy*/2, −*sz*/2). The parent frames are defined in §7.9: **body frame** (the
body matrix of §7.4 step 8), **arm frame** (§7.9.2) and **base frame** (§7.4 step 7).
R*a*(θ) is a rotation about axis *a* by θ degrees (right-handed, applied in the order
written). *s* is the walk swing of §7.4 step 1 **after** all its factors, *s′* the swing
before the longsword, pose and glide factors (the value kept after the quadruped factor).
*w* = appearance weapon scale (0x38), *b* = body scale (0x28), *n* = hand scale (0x2c).

**Shimmer.** Before each draw of an equipment model the shader's shimmer switch is set to 1
when the item's type is non-zero and its material is 1, 11, 12 or 22 (0x4c7be0), and back to 0
after the draw (chest, amulet, shoulders, gloves, both weapons).

**Chest (slot 2 or appearance body).** Body frame · T(body offset) ·
R*z*(0.5·*Y* + *s*·90/π) · S(*b*) · centred. *Y* is the aim yaw (§7.7).
If record field 20 (offset 0x12c, *unknown* f32) is > 0 the **plaster** model 2550 is
drawn over the chest: body frame · T(body offset) · R*z*(0.5·*Y* + *s*·90/π) · S(*b*) ·
T(0, −½·(chest model *sy*), 0) · S(0.5) · R*y*(45) · R*z*(180) · T(−*sx*/2, 0, −*sz*/2)
(*inferred*: a bandage after damage).

**Amulet (slot 1).** Body frame · T(body offset + (0, 3, 6.05)) · R*z*(*s*·90/π + 0.5·*Y*) ·
R*x*(0.7·*s*²·180/π) · S(*b*) · T(−*sx*/2, −1.5, −*sz*): it hangs from its top edge and swings
forward with the square of the swing.

**Shoulders (slot 5).** Body frame · T(0, 0, 4 + 2·*a*) · T(body offset) · R*z*(*s*·90/π) ·
S(shoulder scale) · S(*b* + 0.1) · centred (*a* = walk amplitude).

**Gloves (slot 4)** and **boots (slot 3)** replace the hand and foot models and are drawn
exactly as hands (§7.9.4) and feet (§7.9.5).

**Main weapon (slot 7), in the hand.** Arm frame · T(right hand) · R*z*(ρ.z) · R*y*(ρ.y) ·
R*x*(ρ.x) · R*z*(*W*.z) · R*y*(*W*.y) · R*x*(*W*.x + 0.5·*p*) · S(*w*) · grip translation, where
ρ is the right-hand rotation (§7.7), *W* the main-hand grip rotation (zero unless a pose
writes it, §7.7) and *p* the head pitch (record field 5). Grip translation by main-weapon
subtype (it sets which voxel of the model sits in the hand):

| Main subtype (or model) | Grip translation |
|---|---|
| model 840 (`arrow`) | (−*sx*/2 − 1, 0.5, 1) |
| 4 fist | centred |
| 6 bow | (−*sx*/2, 1.5 − *sy*, −*sz*/2) |
| 7 crossbow | (−*sx*/2, 0, −*sz*/2) |
| 8 boomerang | (−*sx*/2, −2, −4) |
| 10 staff | (−*sx*/2, −*sy*/2, −10) |
| 12 bracelet | first S(0.6), then (−*sx*/2, 1 − *sy*, −*sz*/2) |
| any other (swords, axes, maces, daggers, longswords, great weapons, pitchfork, pickaxe, torch…) | (−*sx*/2, −*sy*/2, −4) |

So a sword's handle voxel 4 above the bottom of the model is held in the hand and the blade
points along the hand's +z.

**Main weapon, on the back (sheathed).** Used when the action is 0, 21, 79–85, 91, 96,
105, 106, 107 or 109, or the creature is gliding (mode 0x10), unless the main weapon is a
fist (4) or bracelet (12): body frame · R*z*(*s*·45/π) · T(0, −7.1, 0.1) · R*y*(−45) (−135
for a staff) · R*z*(90) · S(*w*) · the same grip translation as in the hand.

**Off-hand weapon (slot 6), in the hand.** Arm frame · T(left hand) (pose 71 instead:
T(midpoint of the two hands + (0, 0, 8))) · R*z*(λ.z) · R*y*(λ.y) · R*x*(λ.x) ·
R*z*(*W*′.z) · R*y*(*W*′.y) · R*x*(*W*′.x + 0.5·*p*) · S(*w*, ×1.5 in pose 71) · grip:

| Off-hand subtype (or case) | Grip translation |
|---|---|
| model 840 (`arrow`) | (1 − *sx*/2, 0.5, 1) |
| pose 71 | centred |
| 6 bow | (−*sx*/2, 1.5 − *sy*, −*sz*/2) |
| 7 crossbow | (−*sx*/2, 0, −*sz*/2) |
| 12 bracelet | first S(0.6), then (−*sx*/2, 1 − *sy*, −*sz*/2) (*as read*) |
| 13 shield | (−*sx*/2, 3, −*sz*/2) |
| main weapon is a boomerang (8) | (−*sx*/2, −2, −4) |
| 4 fist | first R*y*(180), then centred |
| any other | (−*sx*/2, −*sy*/2, −4) |

λ is the left-hand rotation, *W*′ the off-hand grip rotation.

**Off-hand weapon, on the back.** Same action list as the main weapon (gliding does
**not** sheathe it), unless the off-hand subtype is 20 (torch), 4 or 12: body frame ·
T(0.5, −0.65, 0.5) · R*z*(*s*·45/π) · then by subtype — shield (13): T(0, −8.2, 0) ·
R*z*(180); bow/crossbow: T(0, −6.2, 0) · R*y*(−45) · R*z*(90); other: T(0, −7.2, 0) ·
R*y*(−225) · R*z*(90) — then S(*w*) · centred.

**Quivers.** (a) If slot 6 holds a quiver (type 3, subtype 14) its model (839) replaces the
off-hand weapon, which is then not drawn: body frame · T(0.65, −0.5, 0.3) · R*z*(*s*·45/π) ·
T(0, −6.2, 0) · R*y*(−45) · R*z*(90) · S(*w*) · centred. (b) If slot 6 is a bow or crossbow,
model 839 is drawn on the back as: body frame · R*z*(*s*·45/π) · T(0, −7.1, 0.1) · R*y*(45) ·
R*z*(90) · S(*w* + 0.01) · centred; (c) the same for a main-hand bow or crossbow with
R*y*(−45). (b) and (c) are drawn whatever the action.

**Glow while charging.** While record field 23 (offset 0x134) is > 0, each weapon is drawn
with the frame material multiplied by 1 + 0.25·(1 + cos(0.01·*t*)), *t* = action time in
ms (*inferred*: the charge glow).

**Weapon trails.** After each in-hand or sheathed weapon draw, two points of the weapon's
matrix are stored, (*sx*/2, *sy*/2, 0) and (*sx*/2, *sy*/2, *sz*) (bottom and top of the
blade axis): main hand in pose state +0x62c / +0x56c, off hand +0x7ac / +0x6ec. A 16-step
trail history follows them while P1 < action time < duration for poses 1–3, 8, 10, 12, 14,
28–32, 35, 48, 49 (both hands), 15, 17, 19 (off hand), 16, 18, 20, 50 (main hand), and a
16-step circle in pose 69 (0x4225fd..0x422fe5; the key is the pose id, *inferred*). Trail
rendering is follow-up F-5.

**Lantern (slot 10).** Drawn while mode flag 0x200 is set and the left hand is drawn:
arm frame · T(left hand + (0, 2, −2)) · S(0.8) · T(−*sx*/2, 1 − *sy*/2, 2 − *sz*), with the
material set to (1, 1, 1, 1) and the shader's light switch at 0 (it is not darkened).

**Held item (record field 43).** During actions 80 and 81 while the action time is below the
item's use limit (consumables (type 1): 10000 ms, 3000 ms for subtype 1; others 0): arm frame
· T(midpoint of the two hands) · R*x*(λ.x) · S(*n*) · centred. **Pet item (slot 12)** during
action 82: the same matrix, but centred only in x and y (T(−*sx*/2, −*sy*/2, 0)).

**Mana cube** (model 2566) during pose 72 while the action time is < 5000: arm frame ·
T(midpoint of the hands + (0, 0, 8)) · R*z*(0.02·*t*) · R*y*(45) · R*x*(45) · centred, scale 1.

**Glider (slot 11, mode 0x10).** Body frame · T(body offset + (0, 0, 8·box.z/box.x)) ·
R*x*(−*B*.x) · S(2*b*) · centred (*B* = smoothed body tilt, §7.7: the glider stays level).
**Boat (slot 11, action 107).** Body frame · T(body offset + (0, 0, −9·box.z/box.x)) ·
R*x*(−*B*.x) · R*z*(−*B*.z) · S(2*b*) · T(1, 2, 0) · T(−*sx*/2, −*sy*/2, 0).

---

## 7. Poses and animation

The creature has no skeleton file. Every frame 0x4128f0 computes the pose procedurally
from the creature's record (action id, action time, velocity, flags), client-only
animation state, and smoothed pose vectors kept per creature (obj +0x1498, "pose state").

### 7.1 Inputs per creature

| Source | Meaning |
|---|---|
| record 0x58 (field 9) | **action id** (u8) |
| record 0x5c (field 10) | **action time** in ms since the action began |
| record 0x24 (field 2) | velocity (blocks/s) |
| record 0x30 (field 3) | movement intent (`alpha-player-control.md` §3) |
| record 0x48 (field 5) | head pitch (degrees) |
| record 0x4c (field 6) | contact flags: bit 0 on ground, bit 1 in water, bit 2 touching a wall |
| record 0x114 (field 14) | mode flags: 0x01 climbing allowed, 0x04 aiming, 0x10 gliding, 0x40 running, 0x200 lantern lit, 0x400 sitting/riding pose (*names inferred from use*) |
| record 0x118 (field 15) | dodge-roll time left (ms, 600 at start) |
| record 0x120 (field 17) | > 0 forces pose 56 (stun, *inferred*) |
| record 0x1b8 (obj +0x11c8)… | mount id (obj +0x11c0, u64): non-zero → riding pose |
| obj +0x1188 | **walk amplitude** 0..1 (§7.3) |
| obj +0x118c | **walk phase** (radians, grows without bound) |
| obj +0x1180 | landing bob (decays toward 0) |
| obj +0x1184 | hit flash (`alpha-rendering.md` §11.2) |
| obj +0x1374 | displayed body rotation *D* (x, y, z), smoothed toward record field 1 (§7.10) |
| obj +0x1350 | displayed position (3 × i64), smoothed toward record field 0 (§7.10) |

### 7.2 Action id → pose id (0x4b4ef0)

Each frame the client copies the action time into the pose state and maps the action id
to a **pose id** (pose state +8). Action 0 → pose 0. Then, if record field 17 > 0, the pose
is forced to 56.

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
| 110→73 | ≥ 111 → 0 | | |

Action 48: main weapon (slot 7) a bow or crossbow (type 3, subtype 6/7) → 22; else
off-hand (slot 6) a shield (subtype 13) → 9; else any off-hand weapon → 35; else 34.
Action 93: off-hand slot empty → 2 (as action 57); else 35.

What each action means is in `alpha-creatures-behaviour.md` §3.

### 7.3 Walk cycle state (simulation step, 0x61e9e6..0x61f0f1)

Per creature and per step of dt milliseconds (dt_s = dt/1000):

* Let *speed* = |velocity| (3-D) and *speed_xy* = its horizontal part,
  *k* = sqrt(0.8 / *S*) where *S* is a per-creature size value (open question M-4;
  most likely the collision box x).
* If speed² > 0.5, the action is not 107 (boat), and the creature is on ground or in
  water or (climbing allowed and touching a wall) or a flyer:
  * climbing (mode 0x01 and wall contact): phase += *k*·speed·dt_s·6 and
    amplitude += 0.2·dt_s (nothing else in this list);
  * on ground, during an attack's wind-up — action 48 always, action 54 before its end,
    actions 6, 7, 10, 17, 18, 19, 20 while the action time is below their first phase
    (`alpha-creatures-behaviour.md` §3.2): phase += *k*·dt·0.0001·speed_xy·1.5;
  * on ground otherwise: phase += *k*·speed_xy·dt_s·1.5 (×2 for action 79);
  * not on ground (water, air): phase += *k*·dt·0.002·speed_xy·1.5;
  * in the last three cases amplitude += dt_s.
* Otherwise, if not on ground, not in water and not climbing: phase := 0 and
  amplitude += *k*·dt_s.
* Then amplitude := min(amplitude, 1) and the amplitude relaxes toward 0 by the
  per-millisecond rule *a* ← *a* + (0 − *a*)·0.005 applied dt times.
* Landing bob (obj +0x1180) relaxes toward 0 at 0.01 per ms if negative, 0.02 if
  positive; hit flash (obj +0x1184) toward 0 at 0.0075 per ms.
* **Riding** (checked before all of the above; mount id obj +0x11c0 non-zero, the mount
  exists and its health > 0, and the rider's controller kind ≠ 5): position := mount
  position + (0, 0, 1.5 − mount landing bob) blocks; velocity := 0; orientation := mount
  orientation + (20, 0, 0); intent := 0; amplitude := mount amplitude × 0.5; phase :=
  mount phase; the rest of the walk-cycle step is skipped. A mount that no longer exists
  or has health ≤ 0 clears the mount id.

"Relax toward *t* at rate *r* per ms" always means: repeat dt times *x* ← *x* + (*t* − *x*)·*r*
(0x411d40), i.e. *x* ← *t* + (*x* − *t*)·(1 − *r*)^dt. Implement it as the loop or the
power; results differ in the last float bits only.

**Footsteps** (only creatures in the local world, not dodging, not in water, speed_xy > 5):
whenever ⌊phase/π⌋ increases, a positional sound event is emitted at the feet
(position − box.z/2): if the ground under the feet is block type 3 (*inferred*: water
edge / snow; follow-up), sound 33 + (draw mod 3) at pitch 1; otherwise sound 32 at pitch
1 + 0.2·draw/32767. (These are world-update section 4 records, see
`alpha-creatures-behaviour.md` §6.)

### 7.4 From pose to matrices

All of this is per frame in 0x4128f0. Notation: R*x*(*a*), R*y*(*a*), R*z*(*a*) rotations
about the local axes by *a* degrees (applied to the current matrix in the order written,
each post-multiplied), T(*v*) a translation, S(*s*) a uniform scale.

1. **Swing.** *s₀* = sin(phase + π/2) computed in double then rounded to float;
   *swing* = amplitude·0.7π·*s₀* (radians; 0.7π = 2.19911485751, double). Then:
   ×0.5 for action 79; ×0.1 for actions 106/107; ×1.25 and "running" if (mode 0x40 set,
   not climbing, |intent|² > 0.1 and |velocity_xy|² > 0.1) or action 48; 0 if flyer and
   not on ground; ×0.75 if quadruped; ×0.25 if main weapon is a longsword and action ≠ 0;
   ×0.1 for poses 51/52 while the action time is below the value of 0x4c6b80
   (*unknown* limit), and for pose 53;
   ×0.5 for actions 22, 36, 63, 8, 28 or poses 2, 3, 48, 49, 37; 0 if gliding.
2. **Leg angles.** *L* = swing·180/π (double divide); left leg = 1.5·*L*, right leg =
   −1.5·*L* (degrees). Overrides: riding → legs 20/20 and body lean 10; poses 57, 52, 58,
   59 → legs 40/40, lean 10; pose 55 → legs −10/−10. Body lean otherwise 0.
3. **Hand targets** (not quadruped), with *h* = hand offset, *a* = amplitude,
   *r* = *h*.x·(1 + *a*):
   right = (cos(swing)·*r*, *h*.y + sin(swing)·*r* + 2.1 − 5*a*, *h*.z − 4.1 + 6*a*),
   left = (−cos(swing)·*r*, *h*.y − sin(swing)·*r* + 2.1 − 5*a*, *h*.z − 4.1 + 6*a*).
   Quadrupeds: right = (*h*.x, *h*.y, *h*.z), left = (−*h*.x, *h*.y, *h*.z). Riding: y += 2,
   x ×0.5. Arm-part targets (for the arm model) are the midpoints:
   ((±hand.x + *h*.x·∓0.3)·0.5, (hand.y − 8)·0.25, (hand.z + *h*.z + 12)·0.5).
4. **Foot targets**: right = foot offset, left = (−x, y, z). Tail target = tail offset.
5. **Pose overrides**: the pose id selects a case that replaces some of these targets and
   sets extra rotations (attacks, bow aiming, sitting, swimming, gliding…). Each case is a
   short list of constant targets and of time curves of the action time (§7.8,
   Appendix C); pose 0 adds the appearance hand rotation (§3.1, 0x4c).
6. **Smoothing.** With *w* = 1 − 0.99^dt (0x411d40 with rate 0.01; dt in ms, the
   caller's frame time), every smoothed vector *p* of the pose state moves toward its
   target: *p* ← *p* + (target − *p*)·*w*; the body-lean scalar likewise; rotation angles
   use the shortest-arc rule *a* ← *a* + asin(sin(target − *a*))·*w* (degrees in, degrees
   out; sin/asin in double).
7. **Base matrix**, in this order (verified at 0x41f225..0x41f640):
   T(creature position − camera position, in blocks: the 64-bit difference × 1/65536;
   the position is the **displayed** position obj +0x1350 (§7.10), and its z first gets
   + lean.z − landing bob (both in blocks, each converted to fixed point by ×65536 and
   truncation; lean is the smoothed vector *E* of §7.7));
   if dodging (field 15 > 0, and not a rogue-class special case: class 4 spec 1 during
   actions 17, 5 or 20 while the action time is below its duration): a roll about the
   axis velocity × (0, 0, 1) = (v.y, −v.x, 0) (normalised; type 101 uses the velocity
   itself; skipped when that axis is shorter than 0.01) by angle (1 − field15/600)·(−360)°,
   and **both hand targets** are raised in z by 5·box.z/box.x model units (correction:
   not the pivot height); the same hand raise applies while gliding (mode 0x10);
   R*z*(displayed rotation z); T(0, 0, *h*); R*y*(displayed rotation y);
   R*x*(displayed rotation x); T(lean.x, lean.y, 0); R*z*, R*y*, R*x* of the smoothed body
   angles (the body tilt *B* of §7.7); T(0, 0, −*h*); **S(box.x / 11.2)**. From here on lengths are model units: a
   box 0.96 wide makes one model unit 0.0857 blocks. *h* is a smoothed pivot height
   (pose state) whose target is box.z/2 while **gliding** (mode 0x10, falling, not on
   ground, field 16 < 1 and not dodging — 0x444650) and 0 otherwise.
8. **Body matrix** = base · T(0, 0, 4·|*s′*|) (×4 again for action 79; *s′* = the swing
   kept after the quadruped factor, before the longsword, pose and glide factors) ·
   T(torso shift *S*) · R*z*(*T*.z) · R*y*(*T*.y) · R*x*(appearance body pitch + *T*.x), with
   *S* and *T* the smoothed torso shift and torso angles of §7.7. Each part is
   then drawn as: part matrix = body matrix · T(part offset) · rotations · S(part scale) ·
   T(−model size / 2) (the model is centred: −0.5·width, −0.5·depth, −0.5·height in
   voxels).

The order and constants of steps 7–8 are verified at 0x41f225..0x41f640; the per-part
rotation lists are summarised in §7.5 and given in full in §7.9 (equipment in §6.5).

### 7.5 Parts in drawing order

| # | Part (model source) | Offset | Rotation | Scale |
|---|---|---|---|---|
| 1 | body (slot 2 or appearance body) | body offset | R*z*(0.5·aim yaw + swing·90/π) | body |
| 2 | slot 1 item | body offset | R*z*(swing·0.5·180/π), R*x*(0.7·swing²·180/π) | body |
| 3 | shoulders (slot 5) | (0, 0, 2*a* + 4) + body offset | R*z*(swing·0.5·180/π) | shoulder × (body + 0.1) |
| 4 | tail | body + tail offset | R*x*(tail angle), R*z*(−swing·180/π), smoothed tail angles | tail × body |
| 5 | head | head offset, then (0, 0, −3) | R*x*(−body angle − body pitch), R*y*, R*z*, R*y*, then R*x*(field 5 head pitch …), then (0, 0, 3) | head |
| 6 | hair | as head | as head | head |
| 7 | arms ×2 (appearance arm) | arm targets | smoothed | arm |
| 8 | held item (field 43), pet item | midpoint of the hands | R*x*(left-hand rotation x) | hand |
| 9 | main weapon (slot 7) | right hand; on the back: (0, −7.1, 0.1) from the body frame | hand rotation, grip rotation, grip translation per subtype (§6.5) | weapon (×0.6 more for bracelets) |
| 10 | off-hand weapon (slot 6) | left hand; on the back (0.5, −0.65, 0.5) then per subtype; quivers on the back | per subtype (§6.5) | weapon (×1.5 in pose 71) |
| 11 | hands ×2 (slot 4 or appearance hand) | hand targets | hand rotation, quadruped variant (§7.9.4) | hand (the **right** hand mirrored in x) |
| 12 | lantern (slot 10, mode 0x200) | **left** hand + (0, 2, −2) | — | 0.8 |
| 13 | wings ×2 | ±wing offset (arm frame) | R*x*(wing angle), R*y*(±flap), R*z*(−90), R*x*(90) | wing |
| 14 | feet ×2 (slot 3 or appearance foot) | foot targets (base frame) | §7.9.5 | foot |
| 15 | gliders, boats, special items | §6.5 | §6.5 | body × 2 |

### 7.6 Worked example: standing and walking human

Standing (amplitude 0, phase 0): *s₀* = 1, swing = 0, legs 0/0. Right hand target
(6, 0 + 2.1, 0 − 4.1) = (6, 2.1, −4.1), left (−6, 2.1, −4.1). Feet (3, 1, −10.5) and
(−3, 1, −10.5). Body at (0, 0, −5), head at (0, 0.5, 5), head scale 1.01. Model scale
0.96/11.2 = 0.085714 blocks per unit, so the head pivot is 0.43 blocks above and the
feet pivots 0.90 blocks below the creature position.

Walking at amplitude 1, phase π/3 (60°): *s₀* = sin(π/3 + π/2) = 0.5; swing = 0.7π·0.5 =
1.0996 rad; legs ±1.5·63.0° = ±94.5°; *r* = 6·2 = 12; right hand =
(12·cos 1.0996, 0 + 12·sin 1.0996 + 2.1 − 5, −4.1 + 6) = (5.43, 7.80, 1.9); left hand =
(−5.43, −13.60, 1.9). These are targets; the drawn pose approaches them at
*w* = 1 − 0.99^dt per frame.

### 7.7 Pose state, channels and the per-frame order (resolves F-2, part 1)

The pose function keeps, per creature, a **pose state** (obj +0x1498; offsets below are
pose-state offsets) holding the copied action time (+0x04), the pose id (+0x08), smoothed
**channels**, the model pointers of the parts (+0xbc..+0xe8) and the part matrices. Pose
cases never move a part directly: they set channel **targets**; the channels follow their
targets with the smoothing of §7.4 step 6 and are then added to the base targets.

| Channel | Symbol | Pose state | Kind | Used by (§7.9) |
|---|---|---|---|---|
| left-hand position offset | Δ*L* | +0x0c | vector | added to the left hand; 0.4·Δ*L* to the left arm |
| right-hand position offset | Δ*R* | +0x18 | vector | added to the right hand; 0.4·Δ*R* to the right arm |
| left-hand rotation offset | ρ*L* | +0x24 | vector | added to the left-hand rotation λ |
| right-hand rotation offset | ρ*R* | +0x30 | vector | added to the right-hand rotation ρ |
| torso angles | *T* | +0x54 | vector | body frame, arm frame, head counter-rotation |
| body tilt | *B* | +0x60 | angles | base frame (rotation about the pivot height) |
| torso shift | *S* | +0x6c | vector | body frame and arm frame translation |
| head shift | *K* | +0x78 | vector | added to the head offset |
| head angles | *H* | +0x84 | angles | head and hair |
| tail angles | *Q* | +0x90 | angles | tail |
| shoulder roll | *U* | +0x9c | vector (only *U*.y used) | arm frame |
| aim yaw | *Y* | +0xa8 | scalar | chest, amulet R*z*(*Y*/2); feet R*z*(*Y*) |
| lean | *E* | +0xac | vector | base frame T(*E*.x, *E*.y, 0); *E*.z is added to the world z in **blocks** |
| pivot height | *h* | +0xb8 | scalar | base frame |

"vector" channels move by *p* ← *p* + (target − *p*)·*w*; "angles" channels by the
shortest-arc rule of §7.4 step 6. The weight is *w* = 1 − (1 − *r*)^dt (0x411d40) with the
**smoothing rate** *r* = 0.01 unless the pose case sets another (Appendix C; values seen
0.002, 0.005, 0.01, 0.02, 0.03, 0.04, 0.05, 0.1, 0.2). One weight is used for all channels.

Besides the channels, a frame uses these **unsmoothed** per-frame values: hand base
targets *L*, *R* (§7.4 step 3), arm base targets, foot base targets, tail target (= tail
offset), hand rotations λ, ρ, leg angles leg*L*, leg*R*, a foot extra angle *F* (0), the
body lean ℓ (0; riding 10 …, §7.4 step 2), a **foot-turn flag** (off), grip rotations *W*,
*W*′ (0, §6.5) and *r*.

**Order of work** (0x4128f0):

1. Part models (§7.5) and swing *s* (§7.4 step 1). The leg angles use *s′* (the swing kept
   after the quadruped factor, before the longsword, pose and glide factors): leg*L* =
   1.5·*s′*·180/π, leg*R* = −leg*L* (correction to §7.4 step 2, which named it "swing").
2. All channel targets := 0, except *h* := box.z/2 while gliding (0x444650). Aim yaw
   target: if mode 0x04 (aiming), |intent_xy|² > 5 and |*a*_xy|² > 0 where *a* = record
   field 26 (offset 0x150, *inferred*: aim direction): with *u* = intent_xy and *v* = *a*_xy
   normalised, *Y* = asin(clamp(*v*.x·*u*.y − *v*.y·*u*.x, −1, 1))·180/π, negated if
   *u*·*v* < −0.2. Otherwise *Y* = 0.
3. Base targets as §7.4 steps 2–4. Hand rotations: pose 0 → ρ = appearance hand rotation
   (x, y, z), λ = (x, −y, −z); every other pose → both 0.
4. A slot-6 quiver is taken out of the off hand (§6.5).
5. **Sitting** (mode 0x400): *B* := (−70, 0, 0); *H*.x := 70; Δ*L* := (0, 3, 1); Δ*R* := (0, 3, 1)
   (Δ*R*.y := 11 if the main weapon subtype is 7, crossbow); *E*.z := (box.y − box.z)/2;
   *K* := (0, −1, 0); leg*L* = leg*R* := −20. If an action is running (id ≠ 0, time <
   duration) Δ*L* and Δ*R* each get + (0, −5, 6).
6. The **pose case** for the pose id (§7.8, Appendix C). It may overwrite any of the above.
7. If the pose id is 0, the copied action time := 0.
8. Arm targets: left arm := ½(left arm + *L*), right arm := ½(right arm + *R*).
9. **Knocked down** (record field 16 > 0): not quadruped → *B* := (90, 0, 0),
   *E*.z := (box.x − box.z)/2; quadruped → *B* := (0, 90, 0), *E*.z := (box.y − box.z)/2.
10. **Climbing** (mode 0x01 and wall contact): Δ*L*.z += 7, Δ*R*.z += 7, ρ*L*.x += 90,
    ρ*R*.x += 90 (arms up).
11. **Torch in the off hand** (slot 6 subtype 20): ρ*L*.x := 0, Δ*L*.y += 4, Δ*L*.z += 4.
12. **Running lean** (not quadruped, action not 106/107): if the running test of §7.4 step 1
    passed: *B*.x −= 25, *H*.x += 10; else if |velocity_xy|² > 0.1 and intent·velocity > 0:
    *B*.x −= 10, *H*.x += 5.
13. Smooth all channels (order in the code: ρ*R*, ρ*L*, Δ*R*, Δ*L*, *T*, *E*, *h*, *B*, *H*, *Q*,
    *Y*, *U*, *S*, *K*). During action 28 a further vector (pose state +0x86c) follows the
    normalised velocity at rate 0.01, else it is set to 0 (no reader found in the draw).
14. Composition: ρ += ρ*R*, λ += ρ*L*, *R* += Δ*R*, *L* += Δ*L*, right arm += 0.4·Δ*R*,
    left arm += 0.4·Δ*L*.
15. Draw flags (§6.4) remove part models; matrices and draws (§7.4 steps 7–8, §7.9, §6.5).

### 7.8 Action poses: how a pose case works (resolves F-2, part 2)

Every pose case reads only the copied action time *t* (ms), the action's phase lengths
(P1, P2, P3 and duration D = P1 + P2 + P3 of `alpha-creatures-behaviour.md` §3.3, with the
attack-speed factors included), the equipment, a few record fields and its own constants.
Phases:

| Phase | Time | Meaning |
|---|---|---|
| W | *t* < P1 | wind-up |
| A | P1 ≤ *t* < P1 + P2 | active (the hit window) |
| R | P1 + P2 ≤ *t* < D | recovery |
| E | *t* ≥ D | after the end (one frame or so before the action id is cleared) |

A case with fewer branches merges phases (written "A..D" for P1 ≤ *t* < D). Progress
values are *q* = *t*/P1 in W and *u* = (*t* − P1)/P2 in A, clamped to [0, 1] where Appendix C
writes "clamp"; a few cases use other divisors, noted there. Lerps are written
lerp(*x*, *y*, *q*) = (1 − *q*)·*x* + *q*·*y*.

A case writes some of: channel targets (":=" replaces, "+=" adds to the default of §7.7),
base targets (*L*, *R*, arm, foot, head and tail bases), *r*, *F*, ℓ, the foot-turn flag,
the pose id (":= 0" only resets the time copy, step 7), and the part models (a hand may be
given the arrow model 840 or the iron-deposit model 2101, or lose its weapon model).
Two shared blocks recur:

* **Grip G(*g*)** (two-handed hold): *L* := *R* + *g*; left arm base y += 10, z −= 4; usually
  followed by ρ*L* := ρ*R*. Default *g* = (2, 0, 0).
* **STANCE** (0x4195c3, end of poses 10 and 15–18, 29, 35): pose id := 0; *U* := 0; unless
  the action is 87 or 96: ρ*L*.x := −90 (−60 for pose 35), *T*.z := 20, ρ*R* := (−120, −90, 30);
  then (skipped when the off hand holds a shield) ρ*L* := (−80, 40, 60); Δ*L*.y −= 4,
  Δ*L*.z += 2; and if a main-hand weapon is drawn and no off-hand weapon: *R* += (0, 2, 3),
  G((−3, 0, −3)), ρ*R*.x := 0, ρ*R*.y := 20, ρ*L* := ρ*R*.

Poses 0, 12, 13, 14, 36 and 64 have no case (default: nothing overridden). Poses 44 and
73 share one case. Appendix C lists every case.

### 7.9 Part transforms in full

Frames (all in model units after the base scale S(box.x/11.2)):

* **Base frame** = §7.4 step 7 (displayed position, dodge roll, displayed rotation, *h*,
  *E*, *B*).
* **Body frame** = §7.4 step 8: base · T(0, 0, bob) · T(*S*) · R*z*(*T*.z) · R*y*(*T*.y) ·
  R*x*(body pitch + *T*.x), with bob = 4·|*s′*| (×4 again for action 79).

#### 7.9.1 Body-attached parts

Chest, plaster, amulet, shoulders: §6.5. **Tail**: body frame · T(body offset) · T(tail
target) · R*x*(tail angle) · R*z*(−*s*·180/π) · R*z*(*Q*.z) · R*y*(*Q*.y) · R*x*(*Q*.x) ·
S(tail scale) · S(*b*) · T(−*sx*/2, −*sy*, −*sz*/2) (the tail's root is its +y face).

#### 7.9.2 Arm frame

Arm frame = base · T(0, 0, bob) · T(*S*) · [R*x*(2·*p*) unless (mode 0x01 and wall contact)
or the pose is 2, 3 or 37] · [R*x*(60) while sitting] · T(0, 0, −4) · R*y*(*U*.y) · T(0, 0, 4) ·
R*z*(*T*.z) · R*y*(*T*.y) · R*x*(*T*.x). *p* = record field 5 (head pitch). The body pitch is
**not** applied. Arms, hands, held items, both weapons, the lantern and the wings hang
from this frame.

#### 7.9.3 Head and hair

Body frame · T(head offset + *K* + (0, 0, −bob/2)) · T(0, 0, −3) · R*x*(−*T*.x − body pitch) ·
R*y*(−*T*.y) · R*z*(*H*.z − *T*.z) · R*y*(*H*.y) · R*x*(*p* − *D*.x + *H*.x) · T(0, 0, 3) ·
S(head scale) · centred (creature type 101: y centring −0.2·*sy*). *D* = displayed body
rotation (§7.10). The head therefore cancels the torso's x and y turn and half of its
bob, and pitches with the look pitch less the body's displayed pitch. The hair uses the
same matrix and the tint of §5.2.

#### 7.9.4 Arms and hands

**Arms** (appearance arm model, both sides the same, not mirrored): arm frame · T(arm
target) · S(arm scale) · centred.
**Left hand** (−x): arm frame · [quadruped: R*x*(leg*R*)] · T(*L*) · R*z*(λ.z) · R*y*(λ.y) ·
[quadruped: R*x*(leg*R*)] · R*x*(λ.x) · S(*n*) · centred.
**Right hand** (+x): the same with *R*, ρ and leg*L*, and S(−*n*, *n*, *n*): mirrored in x; the
renderer swaps the triangle culling order for this one draw (render state 22 set to 3,
then back to 2). Glove models (slot 4) are drawn this way too.

#### 7.9.5 Feet

From the **base** frame (no body bob, no torso): base · R*z*(*Y*) · R*x*(*a*₁) · R*y*(±ℓ) ·
T(foot target + (0, 0, 3·|*s′*| + lift)) · [action 79: R*x*(−1.5·leg)] · S(foot scale) ·
[left foot with the foot-turn flag: R*z*(60); otherwise R*x*(*a*₂)] · centred.

| | *a*₁ | *a*₂ | R*y* | lift | leg |
|---|---|---|---|---|---|
| left foot (−x) | foot angle + *F* + leg*L* | *F* + leg*L* | +ℓ | 4·*a*·*s₀* | leg*L* |
| right foot (+x) | *F* + leg*R* | foot angle + *F* + leg*R* | −ℓ | 4·*a*·(1 − *s₀*) | leg*R* |

*s₀* = sin(phase + π/2) and *a* the walk amplitude (§7.4 step 1); both lifts are 0 for a
flyer off the ground. (Correction to §7.5 row 14.) Boots (slot 3) replace the model only.
Quadrupeds use the same rule for their hind legs; their front legs are the hands.

#### 7.9.6 Wings

Arm frame · T(∓wing offset.x, wing offset.y, wing offset.z) · R*x*(wing angle) · R*y*(±*f*) ·
R*z*(−90) · R*x*(90) · S(wing scale) · T(−*sx*/2, 0, −*sz*/2), with
*f* = cos(phase/2)·*a*·90 − 45; the −x wing is drawn first with +*f*, then the +x wing with −*f*.

### 7.10 Displayed position and body rotation (resolves F-3)

The world step (0x60c510 at 0x61fb7a..0x61fd54) keeps a **displayed position** (obj +0x1350,
3 × i64) and **displayed body rotation** *D* (obj +0x1374, 3 × f32 degrees) apart from the
record's position (obj +0x10, record field 0) and orientation (obj +0x28, record field 1):

* If the creature's client counter obj +0x1398 is > 0 (*unknown* meaning; zeroed by
  0x43b690, 0x446330, 0x447110 — *inferred*: set for creatures updated from the network):
  *w*ᴰ = 1 − (1 − 0.015)^dt (0x411d40 with rate 0.015); the displayed position moves
  toward the record position by the fraction *w*ᴰ (fixed-point arithmetic); each component
  of *D* moves by the shortest-arc rule *D* ← *D* + asin(sin(target − *D*))·*w*ᴰ toward the
  record orientation; the landing bob is set to 0.
* Otherwise: displayed position := record position and *D* := record orientation.

A second rule (0x61f9c0..0x61fb75, condition not traced) attaches a creature to another:
its displayed and record positions become the other's plus (0, 0, ½·*z*₁ − ½·*z*₂ + 0.01)
(two box heights), *D* and the orientation are copied, landing bob, walk amplitude and
phase are copied, and its velocity and intent are zeroed (*inferred*: carrying).

*D* is what the base frame rotates by (§7.4 step 7) and what the head pitch subtracts
(§7.9.3).

### 7.11 Worked examples (test vectors)

1. **Smoothing weight.** dt = 16 ms: *r* = 0.01 → *w* = 1 − 0.99¹⁶ = 0.148542;
   *r* = 0.03 → 0.385746; *r* = 0.005 → 0.077069; *r* = 0.05 → 0.559873.
2. **Greatsword wind-up (pose 2, action 57)**, player with *S* = 1 and attack-speed
   multiplier 1: P1 = 800 ms. At *t* = 200: *q* = min(1.5·200/800, 1) = 0.375. Targets:
   *B*.x = 5 + 10·0.375 = 8.75; *T* = (10, 10, −60); ρ*R* = (0.625·90 + 0.375·60, 0, same) =
   (78.75, 0, 78.75); Δ*R* = Δ*L* = (0, 0, 14·0.375) = (0, 0, 5.25); *r* = 0.01. For a standing
   human (§7.6, *R* = (6, 2.1, −4.1)): the left hand base becomes *R* + (2, 0, 0) =
   (8, 2.1, −4.1). From rest (all channels 0) one 16 ms frame moves ρ*R*.x to
   78.75·0.148542 = 11.6977 and Δ*R*.z to 0.779847; the drawn right hand target is then
   (6, 2.1, −4.1 + 0.779847) = (6, 2.1, −3.320153). Fully settled it would be (6, 2.1, 1.15)
   and the left hand (8, 2.1, 1.15).
3. **Sword in the hand.** A hypothetical sword model 5 × 5 × 18 voxels, weapon scale 0.95:
   grip translation (−2.5, −2.5, −4), so the model voxel (2.5, 2.5, 4) sits exactly at the
   right hand's origin and the model extends 14·0.95 = 13.3 model units along the hand's +z.
4. **Feet while walking** (§7.6 case: amplitude 1, phase π/3, human): *s′* = 1.0995574 rad,
   leg*L* = 94.5°, leg*R* = −94.5°, *s₀* = 0.5. Left foot: *a*₁ = *a*₂ = 94.5°, z target =
   −10.5 + 3·1.0995574 + 4·0.5 = −5.2013277; right foot: *a*₁ = *a*₂ = −94.5°, same z.
5. **Dodge roll.** Field 15 = 450 ms left: angle (1 − 450/600)·(−360) = −90° about
   (v.y, −v.x, 0)/|…|; velocity (3, 4, 0) → axis (0.8, −0.6, 0).
6. **Displayed rotation.** *D*.z = 350°, record orientation z = 10°, dt = 16:
   *w*ᴰ = 1 − 0.985¹⁶ = 0.214801; *D*.z ← 350 + 20·0.214801 = 354.29602°.
7. **Wing flap.** Amplitude 1, phase π: *f* = cos(π/2)·90 − 45 = −45°: the −x wing turns
   R*y*(−45), the +x wing R*y*(45).
8. **Lantern.** Idle human (left hand (−6, 2.1, −4.1)): the lantern's frame origin is the
   arm-frame point (−6, 4.1, −6.1), then S(0.8) and T(−*sx*/2, 1 − *sy*/2, 2 − *sz*).
9. **Item models.** Type 11 subtype 0 material 11 → 2339 `gold-nugget`; type 17 seed 6 →
   2113 + 2 = 2115 `vase3`; type 18 subtype 1 seed 7 → 2120 + 1 = 2121; type 14 rarity 0
   → 2318, rarity 3 → 2320, rarity 4 → 2321; type 21 subtype 10 → 2081 + 2 = 2083
   `quest-item-jewel-case`; type 23 subtype 1 → 2548 `boat2`, subtype 0 → 2547 `glider`.

---

## 8. Open questions (and how to settle them)

The user's 32-bit Alpha mod DLLs can read creature objects from the running client.

* **M-1** *Resolved*: unset parts are 65535 (§3.2).
* **M-7** Who reads the key-colour point lists of §5.1 (none found in the creature draw;
  candidates are the particle/glow code of staffs, wands and bracelets). Find readers of
  model object +0x04/+0x10/+0x1c.
* **M-2** Whether the head as well as the hair is tinted by the hair colour. Change the
  hair colour of a character and screenshot.
* **M-3** *Resolved* for the model (§6.3: subtype 1 = boat); the boat is drawn during
  action 107, the glider while mode 0x10 is set.
* **M-8** Record field 20 (0x12c) > 0 draws the plaster on the chest (§6.5); what sets it.
* **M-9** Meaning of obj +0x1398 (smoothing on/off for the displayed position, §7.10) and
  of obj +0x1314 (its (n/2) mod 3 picks the spin variants of poses 15, 16, 30).
* **M-10** Record field 24 (0x138, 3 × f32) is copied into the shoulder roll *U* by poses 15,
  16, 48, 49; only its y is used. What it holds.
* **M-11** obj +0x13b4 > 0 selects the second branch of pose 66 (boomerang in flight,
  *inferred*).
* **M-4** The size value *S* of the walk-cycle rate. Read [creature list node] at
  0x60eba4 in a debugger, or compare walking speeds of a small and a big creature.
* **M-5** Appearance flags 0x0008, 0x0020, 0x0100 meanings (toggle and observe).
* **M-6** Meaning of appearance bytes 0x00–0x01 (never written).

**Follow-ups**

* **F-1** Climbing-cycle term and the exact branch conditions of §7.3 (from 0x61ea44
  onwards).
* **F-2** *Resolved* in structure (§7.7, §7.8, §7.9, Appendix C). Remaining: the cells of
  Appendix C marked "n.t." (not transcribed: values produced through registers or shared
  tails that the transcription did not follow) and the intermediate thresholds of pose
  44/73. Read those branches at the RVAs given in Appendix C.
* **F-3** *Resolved* (§7.10), except the condition of the attach rule.
* **F-4** *Resolved* (§6.3.1).
* **F-5** Weapon-trail rendering from the stored blade points (0x4225fd..0x422fe5 and its
  reader), and the 16-step circle of pose 69.
* **F-6** Verify Appendix C against recordings: with the mod DLLs log pose state +0x08
  (pose id), +0x0c..+0xb8 (channels) and the action time for one attack of each weapon;
  the targets and *w* of §7.7 predict every frame.

---

## Appendix A. Model table (slot: key without `.cub`, ten per row)

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
2560: mission, city, skull, airship, sphinx01, obelisk, manacube, maptile, cloud02```

## Appendix B. Creature types (per-type step of the generator)

Columns: models set; flags OR-ed into appearance 0x06; scalars set (names of §3.1; "bodyPitch" is 0x48); vectors set as (x, y, z), "·" = component not written; "angles" = (foot 0x58, wing 0x5c, tail 0x60). Types with no box entry keep (0.96, 0.96, 2.16); parts not listed stay 65535 (none). "head choice"/"hair choice" are fields 0x08/0x0c of the choice record (§4.1); "draw *k*" counts the routine's own draws, after the five no-record draws when those happen. Type 58: draw 1 odd → the red parrot models listed, even → parrot-blue (270–273). Types 6 and 46 also set the hair colour from draws 1–3 (R = draw 3, G = draw 2, B = draw 1) and take further draws after them (the head of type 6 is draw 4). Types 0–5, 7–14 and 148–149 copy the hair colour from the choice record; 13–14 set it to (255, 255, 255); 15–16 copy it. Type 116 and 145–146 box values use draw 1. Type 44 is the Alpha server's default new player. Keys loaded into more than one slot (body2 0/1232, body4 1/1234, body3 2/1233, foot 228/432, torch 842/2434, mermaid-hand 1350/1358, door 2076/2482, window 2077/2484, goddess2 2078/2481, egg1 2164/2165/2166) are written with the slot the generator uses.

| Type | Models | Flags | Scalars | Vectors |
|---|---|---|---|---|
| 0 | head head choice mod 4 → 1236.. (elf-head-m01 … elf-head-m04); hair hair choice mod 10 → 1280.. (elf-hair-m01 … elf-hair-m10); hand hand2; foot foot (432); body body4 (1) |  |  |  |
| 1 | head head choice mod 6 → 1240.. (elf-head-f01 … elf-head-f06); hair hair choice mod 10 → 1290.. (elf-hair-f01 … elf-hair-f10); hand hand2; foot foot (432); body body2 (0) |  |  |  |
| 2 | head head choice mod 6 → 1246.. (human-head-m01 … human-head-m06); hair hair choice mod 15 → 1252.. (human-hair-m01 … human-hair-m15); hand brown-hand (431) if head index > 1248, else hand2 (430); foot foot (432); body body4 (1) |  |  |  |
| 3 | head head choice mod 6 → 1267.. (human-head-f01 … human-head-f06); hair hair choice mod 7 → 1273.. (human-hair-f01 … human-hair-f07); hand brown-hand (431) if head index > 1269, else hand2 (430); foot foot (432); body body4 (1) |  |  |  |
| 4 | head head choice mod 5 → 75.. (goblin-head-m01 … goblin-head-m05); hair hair choice mod 6 → 80.. (goblin-hair-m01 … goblin-hair-m06); hand goblin-hand; foot foot (432); body body2 (0) |  | weapon 1.2 | box (0.8, 0.8, 1.8) |
| 5 | head head choice mod 5 → 86.. (goblin-head-f01 … goblin-head-f05); hair hair choice mod 6 → 91.. (goblin-hair-f01 … goblin-hair-f06); hand goblin-hand; foot foot (432); body body2 (0) |  | weapon 1.2 | box (0.8, 0.8, 1.8) |
| 6 | head draw mod 3 → 2379.. (bullterrier-head1 … bullterrier-head3); hand brown-hand; foot foot (432); body body2 (0) |  | head 0.9; weapon 1.2 | headOff (0, 3, 5) |
| 7 | head head choice mod 2 → 98.. (lizard-head-m01 … lizard-head-m02); hair hair choice mod 6 → 100.. (lizard-hair-m01 … lizard-hair-m06); hand lizard-hand; foot lizard-foot; body lizard-body |  |  |  |
| 8 | head head choice mod 5 → 106.. (lizard-head-f01 … lizard-head-f05); hair hair choice mod 6 → 100.. (lizard-hair-m01 … lizard-hair-m06); hand lizard-hand; foot lizard-foot; body lizard-body |  |  |  |
| 9 | head head choice mod 5 → 282.. (dwarf-head-m01 … dwarf-head-m05); hair hair choice mod 3 → 287.. (dwarf-hair-m01 … dwarf-hair-m03); hand hand2; foot foot (432); body dwarf-body |  | head 0.9; weapon 1.2 | box (0.8, 0.8, 1.8) |
| 10 | head head choice mod 5 → 290.. (dwarf-head-f01 … dwarf-head-f05); hair hair choice mod 5 → 295.. (dwarf-hair-f01 … dwarf-hair-f05); hand hand2; foot foot (432); body dwarf-body-female |  | head 0.9; weapon 1.2 | box (0.8, 0.8, 1.8) |
| 11 | head head choice mod 5 → 1300.. (orc-head … orc-head-m05); hair hair choice mod 10 → 1310.. (orc-hair-m01 … orc-hair-m10); hand orc-hand; foot foot (432); body body2 (0) |  | head 0.9; shoulder 1.2 | box (1.04, 1.04, 2.34) |
| 12 | head head choice mod 5 → 1305.. (orc-head-f01 … orc-head-f05); hair hair choice mod 4 → 1320.. (orc-hair-f01 … orc-hair-f04); hand orc-hand; foot foot (432); body body2 (0) |  | head 0.8; body 0.95; shoulder 1.1 | box (1.04, 1.04, 2.34); headOff (0, 1.5, 4) |
| 13 | head head choice mod 5 → 1324.. (frogman-head-m01 … frogman-head-m05); hair hair choice mod 5 → 1329.. (frogman-hair-m01 … frogman-hair-m05); hand frogman-hand; foot foot (432); body body4 (1) |  |  |  |
| 14 | head head choice mod 4 → 1334.. (frogman-head-f01 … frogman-head-f04); hair hair choice mod 4 → 1338.. (frogman-hair-f01 … frogman-hair-f04); hand frogman-hand; foot foot (432); body body4 (1) |  |  |  |
| 15 | head head choice mod 6 → 303.. (undead-head-m01 … undead-head-m06); hair hair choice mod 6 → 309.. (undead-hair-m01 … undead-hair-m06); hand undead-hand; foot foot (432); body body2 (0) |  | head 0.9 |  |
| 16 | head head choice mod 6 → 315.. (undead-head-f01 … undead-head-f06); hair hair choice mod 6 → 321.. (undead-hair-f01 … undead-hair-f06); hand undead-hand; foot foot (432); body body2 (0) |  | head 0.9 |  |
| 17 | head skeleton-head; hair skeleton-eyes; hand skeleton-hand; foot skeleton-foot; body skeleton-body | 0x428 | head 0.8 | handRot (0, -90, 0); bodyOff (0, 0, -2); headOff (0, 4, 7); handOff (5, 3, 0) |
| 18 | head old-head; hand hand2; foot foot (432); body body2 (0) |  |  |  |
| 19 | head collie-head; hand collie-hand; foot collie-foot; body collie-body; tail collie-tail | 0x31 | head 0.9; hand 1; foot 1; tail 1 | box (0.8, 0.8, 1); angles (·, ·, -30); bodyOff (0, 0, 2); headOff (0, 9, 6); handOff (3, 4, -2.5); footOff (3, -4, -1); tailOff (0, -8, -1) |
| 20 | head shepherd-dog-head; hand shepherd-dog-hand; foot shepherd-dog-foot; body shepherd-dog-body; tail shepherd-dog-tail | 0x31 | head 0.9; hand 1; foot 1; tail 1 | box (0.8, 0.8, 1); angles (·, ·, -30); bodyOff (0, 0, 2); headOff (0, 9, 6); handOff (3, 4, -2.5); footOff (3, -4, -1); tailOff (0, -8, -1) |
| 21 | head skull-bull-head; hand skull-bull-hand; foot skull-bull-foot; body skull-bull-body; tail skull-bull-tail | 0x31 | head 0.7; body 1.2; hand 0.8; foot 0.8; tail 1 | box (2.8, 2.8, 4); angles (·, ·, -30); bodyOff (0, 0, 1); headOff (0, 12.5, 3); handOff (3, 2.5, -4.5); footOff (3, -4, -4); tailOff (0, -8, -2.5) |
| 22 | head alpaca-head; hand alpaca-hand; foot alpaca-foot; body alpaca-body | 0x31 | head 0.8; hand 1; foot 1 | bodyOff (0, 0, -4); headOff (0, 9, 5); handOff (3, 4, -8.5); footOff (3, -4, -9) |
| 23 | head alpaca-brown-head; hand alpaca-brown-hand; foot alpaca-brown-foot; body alpaca-brown-body | 0x31 | head 0.8; hand 1; foot 1 | bodyOff (0, 0, -4); headOff (0, 9, 5); handOff (3, 4, -8.5); footOff (3, -4, -9) |
| 24 | body egg1 (2164) | 0x20 |  | box (0.8, 0.8, 1.2); bodyOff (0, 0, -1.5) |
| 25 | head turtle-head; hand turtle-foot; foot turtle-foot; body turtle-body | 0x31 | head 0.65; hand 0.8; foot 0.8; tail 1 | box (0.8, 0.8, 1); angles (·, ·, -30); bodyOff (0, 0, 3.5); headOff (0, 13, 0.5); handOff (3, 4, -3.5); footOff (3, -4, -3.5); tailOff (0, -8, 4.5) |
| 26 | head dog-head2; hand dog-hand2; foot dog-foot2; body dog-body2; tail dog-tail2 | 0x31 | head 0.9; body 1.05; hand 1; foot 1; tail 0.8 | box (1.04, 1.04, 2.34); angles (·, ·, -120); bodyOff (0, 0, -5.5); headOff (0, 9, -1); handOff (3, 4, -8.5); footOff (3, -4.1, -8.5); tailOff (0, -10, 3) |
| 27 | head scottish-terrier-head; hand scottish-terrier-hand; foot scottish-terrier-foot; body scottish-terrier-body; tail scottish-terrier-tail | 0x31 | head 0.9; body 1.05; hand 1; foot 1; tail 0.8 | box (0.8, 0.8, 0.7); angles (·, ·, -120); bodyOff (0, 0, 0.5); headOff (0, 9, 5); handOff (3, 4, -2.5); footOff (3, -4.1, -2.5); tailOff (0, -7, 2) |
| 28 | head wolf-head; hand wolf-hand; foot wolf-foot; body wolf-body; tail wolf-tail | 0x31 | head 0.8; body 1.2; hand 0.9; foot 0.9 | box (1.52, 1.52, 3.23); bodyOff (0, 0, -3.5); headOff (0, 11, -1); handOff (3, 3, -7.5); footOff (3, -4.1, -7.5); tailOff (0, -5.5, -1) |
| 29 | head panther-head; hand panther-hand; foot panther-foot; body panther-body; tail panther-tail | 0x31 | head 0.6; hand 1.1; foot 1; tail 1 | angles (·, ·, -30); bodyOff (0, 0, 0); headOff (0, 14, -1); handOff (3, 5, -8.5); footOff (3, -6, -9); tailOff (0, -11, -4) |
| 30 | head cat-head; hand cat-hand; foot cat-foot; body cat-body; tail cat-tail | 0x31 | head 0.9; hand 1; foot 1; tail 1.1 | angles (·, ·, -30); bodyOff (0, 0, -4); headOff (0, 9, -1); handOff (3, 4, -8.5); footOff (3, -4.1, -8.5); tailOff (0, -8, -1) |
| 31 | head cat-head2; hand cat-hand2; foot cat-foot2; body cat-body2; tail cat-tail2 | 0x31 | head 0.9; hand 1; foot 1; tail 1.1 | angles (·, ·, -30); bodyOff (0, 0, -4); headOff (0, 9, -1); handOff (3, 4, -8.5); footOff (3, -4.1, -8.5); tailOff (0, -8, -1) |
| 32 | head cat-head3; hand cat-hand3; foot cat-foot3; body cat-body3; tail cat-tail3 | 0x31 | head 0.9; hand 1; foot 1; tail 1.1 | angles (·, ·, -30); bodyOff (0, 0, -4); headOff (0, 9, -1); handOff (3, 4, -8.5); footOff (3, -4.1, -8.5); tailOff (0, -8, -1) |
| 33 | head pig-head; hand pig-foot; foot pig-foot; body pig-body | 0x31 | head 0.85; body 1.2; hand 1; foot 1; tail 1 | box (0.8, 0.8, 0.8); angles (·, ·, -30); bodyOff (0, 0, 1); headOff (0, 9, 1.9); handOff (3, 4, -4.2); footOff (3, -4, -4.2); tailOff (0, -8, -1) |
| 34 | head sheep-head; hand sheep-foot; foot sheep-foot; body sheep-body | 0x31 | head 0.85; body 1.2; hand 1; foot 1; tail 1 | box (1.2, 1.2, 1.2); angles (·, ·, -30); bodyOff (0, 0, 1); headOff (0, 4, 1.9); handOff (3, 4, -4.2); footOff (3, -4, -4.2); tailOff (0, -8, -1) |
| 35 | foot bunny-foot; body bunny-body | 0x30 | hand 0.9 | box (1, 1, 1.1); bodyOff (0, 0, 2); headOff (0, 1, 8); handOff (9, 0, 8); footOff (5, 1, -2.5) |
| 36 | foot porcupine-foot; body porcupine-body | 0x30 | hand 0.9 | box (1, 1, 1.1); bodyOff (0, 0, 2); headOff (0, 1, 8); handOff (9, 0, 8); footOff (5, 1, -2.5) |
| 37 | body slime-green | 0x29 | body 1.5 | box (0.8, 0.8, 0.8); bodyOff (0, 0, 0) |
| 38 | body slime-pink | 0x29 | body 1.5 | box (0.8, 0.8, 0.8); bodyOff (0, 0, 0) |
| 39 | body slime-yellow | 0x29 | body 1.5 | box (0.8, 0.8, 0.8); bodyOff (0, 0, 0) |
| 40 | body slime-blue | 0x29 | body 1.5 | box (0.8, 0.8, 0.8); bodyOff (0, 0, 0) |
| 41 | head frightener-head; hair frightener-eyes | 0x42b | head 0.8 | box (1.4, 1.4, 2.5); headOff (0, 0, 0) |
| 42 | head sand-horror-head; hand sand-horror-hand | 0x2a | head 0.8; hand 0.5 | box (1.4, 1.4, 2.5); headOff (0, 0, 0); handOff (6, 0, 2) |
| 43 | head wizard-head; hand hand2; foot foot (432); body wizard-body | 0x20 |  |  |
| 44 | head elf-head-female; hair none (65535); hand hand2; foot foot (432); body body2 (0) |  |  |  |
| 45 | head witch-head; hand hand2; foot foot (432); body witch-body | 0x20 | head 0.8 | headOff (0, 3, 4) |
| 46 | head ogre-head; hand ogre-hand; foot ogre-foot; body ogre-body | 0x20 | head 0.61 | box (2.4, 2.4, 5.4); bodyOff (0, 0, -2); headOff (0, 4, 6); handOff (7.5, 0, 3) |
| 47 | head rockling-head; hand rockling-hand; foot rockling-foot | 0x20 | head 0.8; hand 0.85; foot 0.85; weapon 0.65 | box (2.4, 2.4, 3.6); bodyOff (0, 0, -1); headOff (0, 0, 2); handOff (7.5, 0, 5); footOff (3, 1, -6.5) |
| 48 | head gnoll-head; hand gnoll-hand; foot gnoll-foot; body gnoll-body | 0x20 |  | box (0.8, 0.8, 1.8) |
| 49 | head polar-gnoll-head; hand polar-gnoll-hand; foot polar-gnoll-foot; body polar-gnoll-body | 0x20 | head 1.2 | box (0.8, 0.8, 1.8); headOff (0, 1.5, 4) |
| 50 | head monkey-head; hand monkey-hand; foot monkey-foot; body monkey-body | 0x28 | head 1.05; body 0.8 | box (0.64, 0.64, 1.36); bodyOff (0, 0, -4); headOff (0, 3.5, 2); footOff (3.5, 1, -8.5) |
| 51 | head gnobold-head; hand gnobold-hand; foot gnobold-foot; body zombie-body | 0x20 |  |  |
| 52 | head insectoid-head; hand insectoid-hand; foot insectoid-foot; body insectoid-body | 0x20 |  | bodyOff (0, 0, -4); handOff (6, 0, 1) |
| 53 | head hornet-head; foot hornet-foot; body hornet-body; wing hornet-hand | 0x32 | bodyPitch -45 | angles (30, 45, ·); footOff (3, -1, -7); wingOff (1, -6, 1) |
| 54 | head insect-guard-head; hand insect-guard-hand; foot insect-guard-foot; body insect-guard-body; wing insect-guard-wing | 0x32 | bodyPitch -45 | angles (30, 45, ·); footOff (3, -1, -7); wingOff (1, -6, 1) |
| 55 | head crow-head; foot crow-foot; body crow-body; wing crow-hand | 0x32 | bodyPitch -45 | angles (·, 45, ·); headOff (0, 3, 6); footOff (3, -4, -10); wingOff (3, -5, 0) |
| 56 | head chicken-head; foot chicken-foot; body chicken-body; wing chicken-hand | 0x32 | body 1.3; bodyPitch 0 | box (0.8, 0.8, 1.65); angles (·, 45, ·); headOff (0, 3, 2); footOff (3, 0, -10); wingOff (3, -2, -5) |
| 57 | head seagull-head; foot seagull-foot; body seagull-body; wing seagull-hand | 0x32 | bodyPitch -45 | angles (·, 45, ·); headOff (0, 5, 8); footOff (3, -4, -10); wingOff (3, -5, 0) |
| 58 | head parrot-head; foot parrot-foot; body parrot-body; wing parrot-hand | 0x32 | bodyPitch -45 | angles (·, 45, ·); headOff (0, 3, 6); footOff (3, -4, -10); wingOff (3, -5, 0) |
| 59 | head bat-head; foot bat-foot; body bat-body; wing bat-hand | 0x32 | bodyPitch -45 | angles (·, 45, ·); footOff (3, -4, -10); wingOff (3, -5, 0) |
| 60 | head fly-head; foot fly-foot; body fly-body; wing fly-hand | 0x32 | bodyPitch -45 | angles (30, 45, ·); headOff (0, 3, 4); footOff (3, -1, -7); wingOff (1, -6, 1) |
| 61 | head midge-head; foot midge-foot; body midge-body; wing midge-hand | 0x32 | bodyPitch -45 | angles (30, 45, ·); headOff (0, 3, 4); footOff (3, -1, -5); wingOff (1, -5, -1) |
| 62 | head mosquito-head; foot mosquito-foot; body mosquito-body; wing mosquito-hand | 0x32 | bodyPitch -45 | angles (30, 45, ·); headOff (0, 3, 4); footOff (3, -1, -7); wingOff (1, -6, 1) |
| 63 | hand none (65535); foot plainrunner-foot; body plainrunner-body | 0x28 | foot 1.5 | box (1.2, 1.2, 2.7); bodyOff (0, 3, 6); footOff (4, 0, -1.5) |
| 64 | hand none (65535); foot leafrunner-foot; body leafrunner-body | 0x28 | foot 1.5 | box (1.2, 1.2, 2.7); bodyOff (0, 3, 6); footOff (4, 0, -1.5) |
| 65 | hand none (65535); foot snowrunner-foot; body snowrunner-body | 0x28 | foot 1.5 | box (1.2, 1.2, 2.7); bodyOff (0, 3, 6); footOff (4, 0, -1.5) |
| 66 | hand none (65535); foot desertrunner-foot; body desertrunner-body | 0x28 | foot 1.5 | box (1.2, 1.2, 2.7); bodyOff (0, 3, 6); footOff (4, 0, -1.5) |
| 67 | head peacock-head; foot peacock-foot; body peacock-body | 0x28 | head 0.5; body 0.7; foot 1.5 | box (1.2, 1.2, 3.75); bodyOff (0, 0, 4); headOff (0, 4, 8); footOff (4, 0, -6) |
| 68 | head frog-head; hand frog-hand; foot frog-foot; body frog-body | 0x20 | hand 1.2 | bodyOff (0, 0, -6); headOff (0, 1, 2); handOff (8, 0, 2); footOff (5, 1, -8.5) |
| 69 | head plant-creature-head; hand plant-creature-hand; foot plant-creature-foot; body plant-creature-body | 0x28 | hand 0.8; foot 0.7 | box (1.2, 1.2, 2.7); handRot (-45, 45, -45); bodyOff (0, 0, -4); headOff (0, 1, 7); handOff (6, 3, 0); footOff (3, 2, -10) |
| 70 | hand radish-creature-hand; foot radish-creature-foot; body radish-creature-head | 0x28 |  | box (1.2, 1.2, 2.7); handRot (-45, 45, -45); bodyOff (0, 0, 5); headOff (0, 1, 7); handOff (6, 3, 0); footOff (3, 2, -8) |
| 71 | foot onionling-foot; body onionling-head | 0x30 |  | box (1.2, 1.2, 2.7); bodyOff (0, 0, 1); headOff (0, 0, 0); footOff (3, 0, -12) |
| 72 | foot desert-onionling-foot; body desert-onionling | 0x30 |  | box (1.2, 1.2, 2.7); bodyOff (0, 0, 1); headOff (0, 0, 0); footOff (3, 0, -12) |
| 73 | foot devourer-foot; body devourer-head | 0x30 |  | box (1.2, 1.2, 2.7); headOff (0, 0, -12); footOff (3, 0, -11) |
| 74 | head duckbill-head; hand duckbill-foot; foot duckbill-foot; body duckbill-body; tail duckbill-tail | 0x31 | head 0.73; body 0.9; hand 1; foot 1; tail 0.8 | box (1, 1, 1); angles (·, ·, -80); bodyOff (0, 0, 0.6); headOff (0, 8.5, 1.5); handOff (4, 1, -3.7); footOff (4, -3, -3.7); tailOff (0, -8.5, -2.9) |
| 75 | head crocodile-head; hand crocodile-foot; foot crocodile-foot; body crocodile-body | 0x31 | head 1.1; body 2; hand 1.6; foot 1.6 | box (1, 1, 1); bodyOff (0, -9, 2.6); headOff (0, 17.5, 2.5); handOff (6, 9, -1.7); footOff (6, -9, -1.7) |
| 76 | head spike-head; hand spike-hand; foot spike-foot; body spike-body | 0x20 | body 0.9 | handOff (6, 0, -2) |
| 77 | head anubis-head; hand anubis-hand; foot anubis-foot; body anubis-body | 0x20 | body 0.9 | handOff (6, 0, -2) |
| 78 | head horus-head; hand horus-hand; foot horus-foot; body horus-body | 0x20 | body 0.9 | handOff (6, 0, -2) |
| 79 | head jester-head; hand jester-hand; foot jester-foot; body jester-body | 0x20 | body 0.9 | handOff (6, 0, -2) |
| 80 | head spectrino-head; hand spectrino-hand; foot spectrino-foot; body spectrino-body | 0x20 |  |  |
| 81 | head djinn-head; hand djinn-hand; foot djinn-foot; body djinn-body | 0x20 | head 0.8; foot 0.8 | box (1.44, 1.44, 3.24); headOff (0, 1.8, 6); footOff (3.5, 1, -10.8) |
| 82 | head minotaur-head; hand minotaur-hand; foot minotaur-foot; body minotaur-body; arm minotaur-arm | 0x20 | head 0.5; body 0.9; hand 0.7; foot 0.9; arm 0.65; weapon 0.9 | box (3.2, 3.2, 5.8); bodyOff (0, 0, -2); headOff (0, 3, 4.5); handOff (7, 0, 1.5); footOff (3, 1, -7.5) |
| 83 | head draw mod 3 → 180.. (desert-nomad-head1 … desert-nomad-head3); hand desert-nomad-hand2 (189) if head index ≠ 180, else desert-nomad-hand (188); foot desert-nomad-foot; body desert-nomad-body | 0x20 | head 0.9 | headOff (0, 0.8, 6) |
| 84 | head draw mod 3 → 183.. (desert-nomad-female-head1 … desert-nomad-female-head3); hand desert-nomad-hand2 (189) if head index ≠ 183, else desert-nomad-hand (188); foot desert-nomad-foot; body desert-nomad-body | 0x20 | head 0.9 | headOff (0, 0.8, 6) |
| 85 | head imp-head; hair imp-eyes; hand imp-hand; foot imp-foot; body imp-body | 0x428 | body 0.9 | handRot (-90, 30, ·); handOff (8, 5, 0) |
| 86 | foot squid-foot; body squid-body | 0x10 |  | box (0.8, 0.8, 1); bodyOff (0, 0, 2); footOff (5, 1, -5.5) |
| 87 | head mole-head; hand mole-hand; foot mole-foot; body mole-body | 0x28 | hand 0.9 | box (0.8, 0.8, 1.8); handRot (·, -30, ·); bodyOff (0, 0, -6); headOff (0, 1, 2); handOff (9, 0, 2); footOff (5, 1, -8.5) |
| 88 | head biter-head; hand biter-hand; foot biter-foot; body biter-body | 0x28 | hand 0.9 | box (1.056, 1.056, 2.376); handRot (·, -30, ·); bodyOff (0, 0, -5); headOff (0, 1, 3); handOff (9, 0, 2); footOff (5, 1, -8.5) |
| 89 | head koala-head; hand koala-hand; foot koala-foot; body koala-body | 0x28 | head 1.2; hand 0.9 | box (1.056, 1.056, 2.376); handRot (·, -30, ·); bodyOff (0, 0, -5); headOff (0, 1, 3); handOff (9, 0, 2); footOff (5, 1, -8.5) |
| 90 | head squirrel-head; hand squirrel-hand; foot squirrel-foot; body squirrel-body | 0x28 | head 1.2; hand 0.75; foot 0.8 | box (0.672, 0.672, 1.512); handRot (·, -30, ·); bodyOff (0, -5, -5); headOff (0, 1, 4); handOff (7, 0, 0); footOff (5, 1, -8.5) |
| 91 | head raccoon-head; hand raccoon-hand; foot raccoon-foot; body raccoon-body | 0x28 | head 1.2; hand 0.75; foot 0.8 | box (0.864, 0.864, 1.944); handRot (·, -30, ·); bodyOff (0, -5, -5); headOff (0, 1, 4); handOff (7, 0, 0); footOff (5, 1, -8.5) |
| 92 | head owl-head; foot owl-foot; wing owl-hand | 0x2a | hand 0.9; bodyPitch -10 | box (0.8, 0.8, 1.8); angles (·, 80, ·); headOff (0, 0, -2); handOff (9, 0, 2); footOff (5, 0, -11.5); wingOff (3, -5, 0) |
| 93 | head penguin-head; hand penguin-hand; foot penguin-foot; body penguin-body | 0x28 | hand 0.9 | box (0.8, 0.8, 1.8); handRot (-60, -45, 0); bodyOff (0, 0, -6); headOff (0, 1, 2); handOff (8, 0, 2); footOff (5, 1, -8.5) |
| 94 | head werewolf-head; hand werewolf-hand; foot werewolf-foot; body werewolf-body | 0x28 | head 0.8; hand 0.6; shoulder 1.2 | box (2.4, 2.4, 5.4); handRot (0, -45, 0); headOff (0, 3, 3); handOff (5, 4, 0) |
| 95 | head santa-head; hand hand2; foot foot (432); body santa-body | 0x20 | head 0.8 |  |
| 96 | head zombie-head; hair none (65535); hand zombie-hand; foot foot (228); body zombie-body | 0x28 |  | handRot (0, -90, 0); handOff (6, 3, -2) |
| 97 | head vampire-head; hair vampire-eyes; hand vampire-hand; foot foot (432); body wool-chest5; wing bat-hand | 0x402 | wing 2 | angles (·, 60, ·) |
| 98 | head pony-head; hand pony-foot; foot pony-foot; body pony-body; tail pony-tail | 0x31 | head 0.8; body 0.9; hand 0.8; foot 0.8; tail 1 | box (1.44, 1.44, 1.6); bodyOff (0, 0, 3); headOff (0, 8, 6); handOff (3, 3, -3.25); footOff (3, -5, -3.25); tailOff (0, -7, -4) |
| 99 | head camel-head; hand camel-foot; foot camel-foot; body camel-body | 0x31 | head 0.65; body 0.95; hand 0.8; foot 0.75 | box (1.8, 1.8, 3); bodyOff (0, 0, 1); headOff (0, 12, 4); handOff (3, 3, -5.75); footOff (3, -4, -5.75) |
| 100 | head cow-head; hand cow-foot; foot cow-foot; body cow-body | 0x31 | head 0.65; body 1.2; hand 0.8; foot 0.75 | box (1.8, 1.8, 3); bodyOff (0, 0, -2); headOff (0, 10, 1); handOff (3, 3, -6.75); footOff (3, -4, -6.75) |
| 101 | head dragon-head; hand dragon-foot; foot dragon-foot; body dragon-body; tail dragon-tail | 0x33 | head 0.51; hand 1.5; foot 1 | bodyOff (0, 0, -2); headOff (0, 12.5, 0.5); handOff (3, 4, 1); footOff (3, -4, 1); tailOff (0, -16, 4) |
| 102 | head barkbeetle-head; foot barkbeetle-foot | 0x31 | head 0.75 | box (1.6, 1.6, 1.6); headOff (0, 1, 1); footOff (3.5, 1, -2.5) |
| 103 | head firebeetle-head; foot firebeetle-foot | 0x31 | head 0.9 | box (1.6, 1.6, 1.6); headOff (0, 1, 1); footOff (3.5, 1, -2.5) |
| 104 | head snoutbeetle-head; foot snoutbeetle-foot | 0x29 | head 0.9 | box (1.6, 1.6, 1.6); headOff (0, 1, 1); footOff (3.5, 1, -2.5) |
| 105 | head lemonbeetle-head; foot lemonbeetle-foot | 0x31 | head 0.9 | box (1.6, 1.6, 1.6); headOff (0, 1, 1); footOff (3.5, 1, -2.5) |
| 106 | head crab-head; hand crab-hand; foot crab-foot; body crab-body | 0x28 | head 0.75; hand 1.2 | box (0.8, 0.8, 1.2); handRot (90, 30, 0); bodyOff (0, 0, 1); headOff (0, 1, 5); handOff (10, 0, 8); footOff (6, 1, -5.5) |
| 107 | head crab-head-blue; hand crab-hand-blue; foot crab-foot-blue; body crab-body-blue | 0x28 | head 0.75; hand 1.2 | box (4.8, 4.8, 7.2); handRot (90, 30, 0); bodyOff (0, 0, 1); headOff (0, 1, 5); handOff (10, 0, 8); footOff (6, 1, -5.5) |
| 108 | head troll-head; hand troll-hand; foot troll-foot; body troll-body; arm troll-arm | 0x20 | head 0.6; foot 0.75; arm 0.9; shoulder 1.2 | box (5.6, 5.6, 12.6); headOff (0, 3, 3); handOff (8, 3, 0) |
| 109 | head dark-troll-head; hair dark-troll-eyes; hand dark-troll-hand; foot dark-troll-foot; body dark-troll-body; arm dark-troll-arm | 0x420 | head 0.6; foot 0.75; arm 0.9; shoulder 1.2 | box (5.6, 5.6, 12.6); headOff (0, 3, 3); handOff (8, 3, 0) |
| 110 | head hell-demon-head; hand hell-demon-hand; foot hell-demon-foot; body hell-demon-body; arm hell-demon-arm | 0x20 | head 0.6; foot 0.75; arm 0.85; shoulder 1.1 | box (3.2, 3.2, 7.2); headOff (0, 3, 1); handOff (8, 3, 0) |
| 111 | head golem-head; hand golem-hand; foot golem-foot; body golem-body; arm golem-arm | 0x20 | head 0.6; foot 0.755; arm 0.9; shoulder 1.2 | box (5.6, 5.6, 12.6); headOff (0, 4, 3); handOff (8, 3, 0) |
| 112 | head golem-ember-head; hand golem-ember-hand; foot golem-ember-foot; body golem-ember-body; arm golem-ember-arm | 0x20 | head 0.61; foot 0.755; arm 0.9; shoulder 1.2 | box (5.6, 5.6, 12.6); headOff (0, 4, 3); handOff (8, 3, 0) |
| 113 | head golem-snow-head; hand golem-snow-hand; foot golem-snow-foot; body golem-snow-body; arm golem-snow-arm | 0x20 | head 0.61; foot 0.755; arm 0.9; shoulder 1.2 | box (5.6, 5.6, 12.6); headOff (0, 4, 3); handOff (8, 3, 0) |
| 114 | head yeti-head; hand yeti-hand; foot yeti-foot; body yeti-body; arm yeti-arm | 0x20 | head 0.6; foot 0.75; arm 0.9; shoulder 1.2 | box (5.6, 5.6, 12.6); headOff (0, 3, 3); handOff (8, 3, 0) |
| 115 | head cyclops-head; hand cyclops-hand; foot cyclops-foot; body cyclops-body; arm cyclops-arm | 0x20 | head 0.6; foot 0.75; arm 0.9; shoulder 1.2 | box (4.8, 4.8, 10.8); headOff (0, 3, 3); handOff (8, 3, 0) |
| 116 | head mammoth-head; hand mammoth-foot; foot mammoth-foot; body mammoth-body | 0x31 | head 0.4; body 0.7; hand 0.5; foot 0.5 | box (s, s, 1.12·s) with s = 3·draw1/32767 + 7; bodyOff (0, 0, 1); headOff (0, 8, 2); handOff (3, 3, -4.1); footOff (2.9, -3, -4.1); tailOff (0, -4, -3) |
| 117 | head lich-head; hand lich-hand; foot none (65535); body lich-body; arm lich-arm | 0x20 | head 0.4; hand 0.75 | box (6.4, 6.4, 14.4); handRot (0, -90, 0); bodyOff (0, 0, 3); headOff (0, 1, 11); handOff (6, 5, 6) |
| 118 | head rune-giant-head-laser; hand rune-giant-hand01; foot rune-giant-foot01; body draw mod 2 → 2155.. (rune-giant-body01 … rune-giant-body02) | 0x2c | head 0.7; hand 1.5; foot 0.8; arm 0.9; shoulder 1.2 | box (4, 4, 10); bodyOff (0, 0, 2); headOff (0, 4, 8); handOff (8, 0, 5); footOff (3, 1, -9.5) |
| 119 | head draw mod 5 → 2145.. (monster-head-reptile-lizard … monster-head-reptile-horn3); hand draw mod 3 → 2142.. (monster-foot-reptile-normal … monster-foot-reptile-claw2); foot draw mod 3 → 2142.. (monster-foot-reptile-normal … monster-foot-reptile-claw2); body draw mod 9 → 2133.. (monster-body-reptile-shell … monster-body-reptile-spiked2); tail draw mod 2 → 2150.. (monster-tail-reptile-normal … monster-tail-reptile-spike1) | 0x31 | head 0.1·draw5/32767 + 0.4; body 0.7; hand 0.5; foot 0.5; tail 0.7 | box (9, 9, 10.08); bodyOff (0, 0, 1); headOff (0, 8, 0.5·draw6/32767 − 1); handOff (draw7/32767 + 3, 3, -4.1); footOff (draw8/32767 + 3, -3, -4.1); tailOff (0, -4, -3) |
| 120 | body bush |  |  | box (2, 2, 2); bodyOff (0, 0, 0) |
| 121 | body snow-bush |  |  | box (2, 2, 2); bodyOff (0, 0, 0) |
| 122 | body berry-bush |  |  | box (2, 2, 2); bodyOff (0, 0, 0) |
| 123 | body cotton-plant |  |  | box (0.5, 0.5, 1.5); bodyOff (0, 0, 0) |
| 124 | body scrub (2093) if draw 1 is odd, else scrub-green (2094) |  |  | box (2, 2, 2); bodyOff (0, 0, 2) |
| 125 | body cobwebscrub |  | body 1.5 | box (0.5, 0.5, 1.7); bodyOff (0, 0, 0) |
| 126 | body fire-scrub |  |  | box (2, 2, 2); bodyOff (0, 0, 2) |
| 127 | body ginseng |  |  | box (1, 1, 1); bodyOff (0, 0, 0) |
| 128 | body cactus1 |  |  | box (1.5, 1.5, 4); bodyOff (0, 0, 0) |
| 129 | body fir-tree |  |  | bodyOff (0, 0, -4) |
| 130 | body thorn-tree |  |  | box (3, 3, 12); bodyOff (0, 0, -4) |
| 131 | body gold-deposit |  |  | box (2, 2, 2); bodyOff (0, 0, -4) |
| 132 | body iron-deposit |  |  | box (2, 2, 2); bodyOff (0, 0, -4) |
| 133 | body silver-deposit |  |  | box (2, 2, 2); bodyOff (0, 0, -4) |
| 134 | body sandstone-deposit |  |  | box (2, 2, 2); bodyOff (0, 0, -4) |
| 135 | body emerald-deposit |  |  | box (2, 2, 2); bodyOff (0, 0, -4) |
| 136 | body sapphire-deposit |  |  | box (2, 2, 2); bodyOff (0, 0, -4) |
| 137 | body ruby-deposit |  |  | box (2, 2, 2); bodyOff (0, 0, -4) |
| 138 | body diamond-deposit |  |  | box (2, 2, 2); bodyOff (0, 0, -4) |
| 139 | body ice-crystal-deposit |  |  | box (2, 2, 2); bodyOff (0, 0, -4) |
| 140 | body scarecrow | 0x20 | body 2 | box (0.8, 0.8, 4); bodyOff (0, 0, 0) |
| 141 | body aim | 0x20 |  | box (1.5, 1.5, 2); bodyOff (0, 0, 0) |
| 142 | body dummy | 0x20 |  | box (1.5, 1.5, 2.5); bodyOff (0, 0, 0) |
| 143 | body draw mod 4 → 2113.. (vase … vase4) |  |  | box (1.5, 1.5, 1.25); bodyOff (0, 0, 0) |
| 144 | body bomb1 |  | body 0.8 | box (0.8, 0.8, 0.8); bodyOff (0, 0, 1) |
| 145 | body fish1 | 0x131 | body 0.9 | box (s, s, s) with s = (0.5·draw1/32767 + 1.2)·0.8; bodyOff (0, 0, 0) |
| 146 | body fish2 | 0x131 | body 0.9 | box (s, s, s) with s = (0.5·draw1/32767 + 1.2)·0.8; bodyOff (0, 0, 0) |
| 147 | body seahorse | 0x131 |  | box (0.8, 0.8, 2); bodyOff (0, 0, 0) |
| 148 | head head choice mod 3 → 1343.. (mermaid-head-f01 … mermaid-head-f03); hair hair choice mod 3 → 1346.. (mermaid-hair-f01 … mermaid-hair-f03); hand mermaid-hand (1350); body mermaid-body |  |  |  |
| 149 | head head choice mod 3 → 1351.. (merman-head-m01 … merman-head-m03); hair hair choice mod 3 → 1354.. (merman-hair-m01 … merman-hair-m03); hand mermaid-hand (1350); body merman-body |  |  |  |
| 150 | body shark | 0x131 | body 1.2 | box (2.4, 2.4, 6); bodyOff (0, 0, 0) |
| 151 | head bumblebee; foot bumblebee-foot; wing bumblebee-hand | 0x32 | head 1.2; foot 1; wing 0.6; bodyPitch -45 | box (1, 1, 1.875); angles (·, 45, ·); headOff (0, 3, 0); footOff (3, 4, -8); wingOff (6.5, 3, -1) |
| 152 | head lantern-fish; hair lantern-fish-eyes | 0x531 | head 1.2 | box (2.4, 2.4, 6); bodyOff (0, 0, 0); headOff (0, 0, 0) |
| 153 | body mawfish | 0x131 | body 1.2 | box (2.4, 2.4, 6); bodyOff (0, 0, 0) |
| 154 | body piranha | 0x131 | body 1.2 | box (2.4, 2.4, 6); bodyOff (0, 0, 0) |
| 155 | body blowfish | 0x131 | body 1.2 | box (2.4, 2.4, 6); bodyOff (0, 0, 0) |

## Appendix C. Pose cases (0x4128f0, jump table 0x423d4c)

Notation of §7.7–§7.8: channels Δ*L*, Δ*R* (hand position offsets), ρ*L*, ρ*R* (hand rotation
offsets), *T* (torso angles), *B* (body tilt), *S* (torso shift), *K* (head shift), *H* (head
angles), *Q* (tail angles), *U* (shoulder roll), *E* (lean; *E*.z in blocks), *h*; base targets
*L*, *R* (hands); *F* foot extra angle; ℓ body lean; *r* smoothing rate (0.01 unless given);
"turn" = foot-turn flag on. Unlisted channels keep the defaults of §7.7 (0). Phases W, A,
R, E and "A..D" as §7.8; *q* = *t*/P1, *u* = progress through A; "clamp" = clamped to
[0, 1]. "n.t." = not transcribed (follow-up F-2). Vectors are (x, y, z); a component
written alone (e.g. "*T*.z := 40") leaves the others at their defaults. "Actions" lists the
action ids that map to the pose (§7.2). Leg and lean overrides of §7.4 step 2 (poses 52,
55, 57–59) apply in addition.

| Pose (entry) | Actions | Case |
|---|---|---|
| 1 (0x4174a0) | 47, 54 | **t < 500** (fixed): *q* = clamp(*t*/500). Two-handed/ranged main (0x444230): ρ*R*.x := 120*q*; Δ*R* := (−*R*.x, 0, 14); Δ*L* := (2 − Δ*L*.x, 0, 12). Otherwise Δ*R* := (1, 0, 14), Δ*L* := (−1, 0, 14), ρ*L*.x = ρ*R*.x := −100. Both: *T*.x := −40*q*; *B*.x := −40*q*; *F* := −50*q*; during action 48 *B*.x −= min(360·*t*/500, 360); the creature's walk amplitude := 0. **500 ≤ t < D**: *B*.x from a lerp of −60 (n.t.); *T*.x := −20; ρ*R*.x := −70; two-handed: Δ*R* := (−*R*.x, 4, 0), Δ*L* := (2 − Δ*L*.x, 4, 0); otherwise Δ*R* := (1, 3, 8), Δ*L* := (−1, 3, 8), ρ*L*.x := −150; *r* = 0.1. **E**: two-handed: *R* += (−8, 4, 0), G((−2, 0, −1)), ρ*R*.x := −60, ρ*R*.z := 20; otherwise Δ*R* := (1, 3, 2), Δ*L* := (−1, 3, 2), ρ*L*.x = ρ*R*.x := −250. Always: off-hand shield → ρ*L* := (−250, −90, 0). |
| 2 (0x41644b) | 57, 60, 93 (no off-hand) | **W**: *q* = clamp(1.5·*t*/P1); *B*.x := 5 + 10*q*; *T* := (10, 10, −60); ρ*R* := (lerp(90, 60, *q*), 0, lerp(90, 60, *q*)); Δ*R* = Δ*L* := (0, 0, 14*q*); G((2, 0, 0)); ρ*L* := ρ*R*; *r* = 0.01. **A**: *B*.x := −10; *T*.y := 10, *T*.z := 60; ρ*R* := (−70, 0, −60); Δ*R* = Δ*L* := (0, 2·sin(π*u*/2), 0) (*u* clamp); G((2, −1, 0)); ρ*L* := ρ*R*; *r* = 0.04. **R..E**: pose id := 0; *R* += (0, 2, 3); G((−3, 0, −3)); ρ*R*.x := 0, ρ*R*.y := 20; ρ*L* := ρ*R*. Arm frame: no head-pitch tilt. |
| 3 (0x4170ee) | 58 | **W**: *B*.z := 0; ρ*R* := (−90, 0, 0); Δ*R* = Δ*L* := n.t.; G((2, 0, 0)); ρ*L* := ρ*R*; *r* = 0.005. **A**: *B* := (10, 0, −10); *T* := (30, 0, −30); ρ*R* := (0, 0, 90); rest as W. **R**: *B*.z := −30; *T* := (0, 20, −60); ρ*R* := (70, 0, 90); Δ*R* = Δ*L* := (5, −2, 0); G((2, 0, 0)); ρ*L* := (70, 0, 90); *r* = 0.02. **E**: pose id := 0; G((−2, 0, −1)); ρ*R*.y := 90; *T*.z := −80; ρ*L* := ρ*R*. No head-pitch tilt. |
| 4 (0x416052) | 59 | **W**: *q* = clamp(*t*/P1); *B*.x := 5*q*; *T*.y := 10*q*; *T*.z := −20*q*; ρ*R* := (60*q*, lerp(120, 60, *q*), 60*q*); G((2, 0, 0)); ρ*L* := ρ*R*; *r* = 0.005. **A..D**: class 1 spec 1: Δ*R* := (2, 7, 4), ρ*R* := (0, −60, 0); otherwise *q*₂ = clamp(1.5·…) (n.t.): *B*.x := 0, *T* := (20, 0, 0), ρ*R* := (lerp(90, 30, *q*₂), 0, lerp(90, 0, *q*₂)), *R*.z += 0.5·cos(0.5·*t*), Δ*R* := (−6, 0, 16·*q*₂) (n.t. factor). Then Δ*L* := Δ*R*; G((2, 0, 0)); ρ*L* := ρ*R*. |
| 5 (0x419a6f) | 64 | All times: ρ*R* := (−90, 0, −40); ρ*L* := (−90, 0, 40); *R*.z += 0.5·cos(0.5·*t*); *L*.z += 0.5·cos(0.5·*t*); Δ*R* := (1, −7, 0); Δ*L* := (−1, −7, 0); *T*.z := −40; *r* = 0.005. |
| 6 (0x419bee) | 63 | Specialization 1: ρ*R* := (−50, −60, 40); ρ*L* := (−90, 90, −50); hand z wobble as pose 5; Δ*R* := (−3, 7, 3); Δ*L* := (−2, 2, 0); *T*.z := −20; *r* = 0.005. Otherwise: ρ*R* := (−90, 0, −40); ρ*L* := (−90, 90, −40); wobble; Δ*R* := (1, −7, 0); Δ*L* := (−1, −7, 0); *T*.z := −40; *r* = 0.005. |
| 7 (0x417ab1) | — | *U* := a vector eased by (*t*/D)³·15 (n.t.); ρ*R* := (0, 90, 0); two-handed main: G((2, 0, 0)); *r* = 0.005 / 0.03. |
| 8 (0x419ebd) | 98 | **Off-hand model present**: *t* < 100: *T*.z := 80, ρ*L* := (0, 0, 90), ρ*R* := (−45, 45, 0); 100 ≤ *t* < 500: *T*.z := −90, ρ*L* := (0, 60, 90), *L*.z += 5, ρ*R* := (−90, 0, 45); *t* ≥ 500: *T*.z := −30, ρ*L* := (0, 100, 60), ρ*R* := (−45, 45, 0). **No off-hand model**: *t* < 100: *T*.z := −80, ρ*R* := (0, 0, 90); 100 ≤ *t* < 500: *T*.z := 90, ρ*R* := (0, 60, −85), Δ*L* += (2, 3, 3), Δ*R* += (2, 3, 3); *t* ≥ 500: *R* += (0, 2, 3), ρ*R*.x := 0, ρ*R*.y := 20; then (all three) G((−3, 0, −3)) and ρ*L* := ρ*R*. |
| 9 (0x41b54e) | 8, 48 (shield) | **W**: *T*.z := 80; ρ*L* := (0, 0, 90); ρ*R* := (−45, 45, 0); action 48: *B*.z := 360·*t*/300. **after W**: *T*.z := −90; ρ*L* := (0, 0, 90); ρ*R* := (−90, 0, 45). |
| 10 (0x41b850) | 10 | **W**: as pose 9 W. **A..D**: *T*.z := −90; ρ*L* := (0, 0, 90); ρ*R* := (−90, 0, 45); plus a sine term of (*t* − 600) (n.t.). **E**: STANCE (shield entry with a shield). |
| 11 (0x41b006) | 28 | Unless appearance flag 0x0004: *T*.z := 60; ρ*R* := (0, −45, −60); ρ*L* := (−45, −45, 0). |
| 15 (0x41db88) | 2 | Spin variant when (obj +0x1314 / 2) mod 3 = 2: *B*.z := −360·clamp((*t* − P1)/(P1 + P2)), *r* = 0.03; else *B*.z := 0. *U* := record field 24. **W**: *T*.z := 40; ρ*L* := (0, −90, 30); Δ*L*.y −= 4, Δ*L*.z += 4; ρ*R* := (0, −90, −120); Δ*R*.y −= 6. **A..D**: *T*.z := −90; *E*.x := −0.1; ρ*L* := (−90, −90, −30); ρ*R* := (0, −90, −120); Δ*R*.y −= 6. **E**: STANCE. |
| 16 (0x41e029) | 1 | Spin variant when (obj +0x1314 / 2) mod 3 = 1, as pose 15. *U* := record field 24. **W**: *T*.z := 90; ρ*R* := (0, −90, 0); ρ*L* := (−90, 0, 90); Δ*R*.y += 6, Δ*R*.z += 4. **A..D**: *T*.z := 20; ρ*L* := (−90, −60, 90); Δ*L*.y −= 4, Δ*L*.z += 4; ρ*R* := (0, −90, −220); Δ*R*.y −= 6; *E*.x := 0.1; left foot base y −= 4, right foot base y += 4; turn. **E**: STANCE. Always: off-hand shield → ρ*L* := (0, 0, 90). |
| 17 (0x41d3ee) | 4, 18 | **W**: *T*.z := 40; ρ*R* := (−90, 90, 30); ρ*L* := (−90, 0, −20); Δ*R*.y −= 6. **A..D**: *T*.z := −90; ρ*R* := (−40, 0, 30); Δ*R* := (3, 0, 4); ρ*L* := (−90, 90, 90); turn; *r* = 0.05. **E**: STANCE. |
| 18 (0x41d123) | 3, 9, 19, 62 | **W**: *T*.z := −40; ρ*L* := (−90, −90, −30); ρ*R* := (−90, 0, 20); Δ*R*.y −= 6. **A..D**: *T*.z := 90; ρ*L* := n.t. (shield-dependent); Δ*L* := (−3, 0, 4); ρ*R* := (−90, −90, −90); Δ*R* := n.t.; turn; *r* = 0.05. **E**: STANCE. |
| 19 (0x41be27) | 7 | **P1 < t < D**: *T*.z := −20; *B*.z := −20; ρ*L* := (0, 90, 40); Δ*L* := (0, 8, 4); ρ*R* := (0, 90, 30); then *H*.x := −10, *S*.y := 3, *r* = 0.03. **Otherwise** (guard idle): *T*.z := 0; *B*.z := 0; ρ*R* := (60, −20, 20); Δ*R* := (0, 2 + sin(0.01*t*), 4 + cos(0.01*t*)); ρ*L* := (60, 20, −20); Δ*L* := (0, 2 + sin(0.01*t* + π/2), 4 + cos(0.01*t* + π/2)); a sine is added to one *S* component (n.t.); *r* = 0.03. |
| 20 (0x41badc) | 6 | **P1 < t < D**: *T*.z := 45; *B*.z := 45; ρ*R* := (0, −90, −90); Δ*R* := (8, 0, 4); ρ*L* := (0, −90, −30); then as pose 19 (*H*.x := −10, *S*.y := 3, *r* = 0.03). **Otherwise**: guard idle as pose 19. |
| 21 (0x418591) | 22, 23 | Bow/crossbow **in the off hand** (main subtype not 6/7): off-hand crossbow → ρ*L*.y := −90. *t* < D: Δ*L*.y += 5 (bow) or 2 (otherwise, *as read*); Δ*L*.z += 4; Δ*R* += (−10, 3, 3); *T*.z −= 70; ρ*L*.z := 70; ρ*R*.z := 70; while *t* < (a second limit, n.t.): ρ*L*.x := −5, Δ*R* := (−5, 1, 0), main-hand model removed. *t* ≥ D: Δ*R*.x −= 1; ρ*L*.x = ρ*R*.x := −30; ρ*R*.y := 45 (off-hand crossbow: ρ*L*.z := 50). If the off hand is a weapon (type 3) the main hand shows the arrow (840). **Bow/crossbow in the main hand**: the mirror image — crossbow ρ*R*.y := −90; Δ*R*.y += 5 / 2; Δ*R*.z += 4; Δ*L* += (10, 3, 3); *T*.z := 70; ρ*R*.z = ρ*L*.z := −70; then ρ*R*.x := −5, Δ*L* := (5, 1, 0), off-hand model removed; after D: Δ*L*.x += 1, ρ*L*.x = ρ*R*.x := −30, ρ*L*.y := −45; the off hand shows the arrow if the main is type 3. |
| 22 (0x417c84) | 24, 25, 50, 55, 48 (bow) | **Aiming.** Main not a bow/crossbow: Δ*L* += (−4, 3, 2); Δ*R* += (−10, 2, 1); *T*.z := −60; ρ*L*.z := 60; ρ*L*.y := −90; ρ*R*.z := 60; if slot 6 is a weapon the main hand shows the arrow (840); for *t* < 300 ρ*L*.x += 20 (*as read*). Main bow/crossbow: Δ*R* += (4, 3, 2); Δ*L* += (10, 2, 1); *T*.z := 60; ρ*R*.z := −60; ρ*R*.y := −90; ρ*L*.z := −60; if slot 7 is a weapon the off hand shows the arrow; *t* < 300: ρ*R*.x += 20. |
| 23 (0x41a5c1) | 37, 43 | **W**: *T*.z := −30; ρ*R* := (0, −45, −60); ρ*L* := (−45, −45, 0); Δ*R*.y += 4, Δ*L*.y += 4; Δ*R*.x, Δ*R*.z, Δ*L*.x, Δ*L*.z each += 0.5·sin or cos of a time term (n.t.). **after W**: *T*.z := 60; ρ*R*, ρ*L* as W. |
| 24 (0x41ad90) | 39, 41 | **W**: Δ*R*.y += 4, Δ*L*.y += 4, wobble as 23 (also *L*.z); *T*.z := 20. **A..D**: *E*.x := 0.5; *T*.z := −60; *B*.z := −30; ρ*L* := (0, 45, 60); ρ*R* := (−45, 45, 0); Δ*L* := (−2, 0, 0); turn. |
| 25 (0x41ab1a) | 40, 42 | **W**: as 24 with *T*.z := −20. **A..D**: *E*.x := −0.5; *T*.z := 60; *B*.z := 30; ρ*R* := (0, −45, −60); ρ*L* := (−45, −45, 0); Δ*R* := (2, 0, 0); turn. |
| 26 (0x41a80b) | 38, 44–46, 94, 95 | **W**: as 23 W. **A..D**: *T*.z := 90; ρ*R* := (−90, 0, −90); ρ*L* := (−45, −45, 0); Δ*R* := (3, 0, 1); *r* = 0.03. **E**: ρ*R* := (−60, 0, 0); Δ*R* := (3, 0, 1). |
| 27 (0x41a3e6) | 36 | As 23 W at all times. |
| 28 (0x418ac6) | 11 | **W**: *T*.z := −80; *B*.z := 0; ρ*R* := (−90, 90, −90); ρ*L* := (20, 90, 90). **A**: *T*.z := −90; *B*.z := 360·*u*; ρ*L* := (0, 100, 90); ρ*R* := (0, 90, 0); *r* = 0.2. **R..**: *T*.z := 80; *B*.z := 0; ρ*L* := (90, 90, 270); ρ*R* := (0, 90, 90). |
| 29 (0x419347) | 12 | **W**: ρ*R* := (−80, −90, 60); ρ*L* := (−80, 90, −60); Δ*R*.y += 3, Δ*R*.x −= 5; Δ*L*.x += 5. **A..D**: *T* := (0, 0, −30); Δ*R*.y −= 5; Δ*L*.y −= 5; ρ*L* := (−90, 90, 110); ρ*R* := (−90, −90, −110); *E*.y := −0.5; *r* = 0.04. **E**: STANCE. |
| 30 (0x41c115) | 13 | **W**: *q* = *t*/P1; ρ*R* := (−80, 90, −90); Δ*R* := (−10, 8, 8); *T*.z := −45; *B*.z := −20*q* (spin variant when the combo test gives 2 and *t* < P1 + P2: + 360*q*, *inferred* same test as pose 15). **A**: *q*₂ = clamp(1.2·(*t* − P1)/P2): *E*.x := −0.8*q*₂; ρ*R*, Δ*R* := blends (n.t.); *H*.x := −10; *T*.z := 90*q*₂ − 45(1 − *q*₂); *B*.z := 30*q*₂; ℓ := 10*q*₂; *r* = 0.05. **R..**: Δ*R* := (2, 0, 0); ρ*R* := 0; *T*.z := −30. **All phases**: Δ*L* := Δ*R*; ρ*L* := ρ*R*; *L* := *R* + (−0.2, −0.2, −1). |
| 31 (0x41c67c) | 14 | **W**: ρ*R* := (0, −90, 0); Δ*R* := (−8, 6, 6); *H*.x := −10, *H*.z := 30; *T*.z := 45; *B*.z := 20; *r* = 0.05 (spin variant as 30). **A**: *q*₂ = clamp(1.2·(*t* − P1)/P2): *E*.x := 0.8*q*₂; *H*.z := −30; ρ*R*, Δ*R* blends (n.t.); *T*.z := 45(1 − *q*₂) − 45*q*₂; *B*.z := −30*q*₂; *B*.x := −20*q*₂; ℓ := 10*q*₂; *r* = 0.05. **R..**: as 30. Tail as 30. |
| 32 (0x41cbda) | 15 | **W**: *q* = *t*/P1; Δ*R* := (2, 2, 14); ρ*R* := (50, 0, 0); *T*.z := −45; *B*.z := −20*q*; *E*.x := 0.5*q*. **A**: *q*₂ = clamp(2·(*t* − P1)/P2): *B*.x := 20(1 − *q*₂) − 10*q*₂; ρ*R*, Δ*R* blends (n.t.); *T*.z := 45*q*₂ − 45(1 − *q*₂). **R..**: as 30. Tail as 30. |
| 33 (0x41cf8b) | 5 | **W**: Δ*R* := (4, 2, 8); ρ*R* := (−90, −180, 90); *T*.z := −90; *E*.y := −1. **A**: Δ*R* := (6, 0, 0); *E*.y := 0.5; *T*.z := 90; ρ*R* := (−90, 0, −90); turn. **R..**: Δ*R*.x := 2; ρ*R* := 0; *T*.z := −30. Tail as 30. |
| 34 (0x419a23) | 48 (else) | ρ*R* := (−90, −90, 80); *T*.z := −80. |
| 35 (0x419748) | 16, 17, 48/93 (off-hand) | **W**: ρ*R* := (−90, 0, 0); ρ*L* := (−90, 0, 40); Δ*R* := (1, −7, 0); Δ*L* := (−1, −7, 0); *T*.z := −40; *E*.y := −0.5; *r* = 0.005; turn. **A..D**: ρ*R* := (−90, −60, −30); ρ*L* := (−90, 60, −50); Δ*R* := (3, 5, 2); Δ*L* := (6, 8, 2); *T*.z := 40; *r* = 0.05; turn. **E**: STANCE (ρ*L*.x := −60). |
| 37 (0x415dee) | 67 | **W**: ρ*R* := (−90, −90, 80); *T*.z := −80. **A..D**: ρ*R* := (−90, 0, −90); *T*.z := 90; *R*.x += 4; left foot base y −= 4, right foot base y += 4; turn; *r* = 0.05. **W and A..D**: then G((2, 0, 0)) with the left arm base taken from the unmodified arm (n.t.), ρ*L* := ρ*R*. **E**: pose id := 0; *R* += (−8, 4, 0); G((−2, 0, −1)); ρ*R*.x := −60, ρ*R*.z := 20; *T*.z := −20. No head-pitch tilt. |
| 38 (0x4144fc) | 68 | **W**: *H* := (−30, 10, 90); *T* := (0, 0, 60); *K* := (n.t., −2, 0). **after W**: *H* := (30, −10, −30); *T* := (10, 0, −20); *K* := (3, 0, 2). |
| 39 (0x41466c) | 69 | **W**: *H* := (−70, 0, 0); *T* := (−10, 0, 0); *K* := (0, −3, −1); ρ*L*.x = ρ*R*.x := 20. **after W**: *H* := (50, 0, 0); *T* := (20, 0, 0); *S* := (0, −3, 1); *K* := (0, 0, 2); ρ*R*.x = ρ*L*.x := −20; *Q* := (n.t., 0, 0). |
| 40 (0x415624) | 70 | **W**: *B*.z := 90; *H* := (0, 10, −60); *Q* := (0, 0, −70); *r* = 0.002. **after W**: *B*.z := 200; *H* := (0, −20, 60); *K* := (−3, 0, −2); *Q* := (0, 0, 70). |
| 41 (0x415771) | 71, 72 | **W** (*f* = *t*/P1): hand, foot and head base targets move toward the body offset point by the fraction *f* (head toward body offset + (0, 0, −2), tail toward body offset + (0, 4, 4)): the creature curls up. **A**: pose id := 0; hand, foot and tail models removed (not drawn); *S* := (0, 0, −3). **R** (*g* = 1 − (*t* − P1 − P2)/P3): the same lerps with *g*: it uncurls. |
| 42 (0x415113) | 73 | **W**: *H* := (−10, 10, 30); *T* := (−10, 0, 0); *K* := (−3, 0, −1); ρ*L*.x = ρ*R*.x := 20; *r* = 0.005. **after W**: *H* := (50, 30, 0) (*H*.y := −30 while *t* < P1 + P2); *T* := (30, 0, 0); *S* := (0, −1, 2); *K* := (0, 0, 2); ρ*R*.x := −60; ρ*L*.x := −20; *Q* as 39; *r* = 0.005. |
| 43 (0x414878) | 74 | **W**: *H* := (−20, 0, 0); *T* := (50, 0, 0); *S* := (0, −2, 2); *K* := (0, −6, −2); ρ*L*.x := 20; ρ*R*.x := 30; *Q* as 39. **after W**: *H* := (−10, 0, 0); *T* := 0; *K* := (0, 0, 1); ρ*L*.x = ρ*R*.x := 0; *r* = 0.02. |
| 44, 73 (0x414a6e) | 75, 110 | Five steps (thresholds after the first n.t.): (1) *t* < P1: *H* := (10, 0, 0); *T* := (−20, 0, 0); *S* := (0, −1, 0); *K* := (0, 1, 1); ρ*L*.x = ρ*R*.x := 20; Δ*L*.z = Δ*R*.z := 2; *Q* := (30, 0, 10). (2) *H* := (−30, 0, 0); *T* := (50, 0, 0); *S* := (0, 0, 1); *K* := (0, −4, 2); ρ*L*.x := 60; ρ*R*.x := 70; *Q* as 39. (3) *H* := (−30, 0, 0); *T* := (40, 0, 0); *S* := (0, 1, 1); *K* := (0, −2, 2); ρ*L*.x := 70; ρ*R*.x := −30; Δ*R* := (0, 3, −2); *Q* := (−30, 0, −10). (4) as (3) with *T* := (10, 0, 0), ρ*L*.x := −30, ρ*R*.x := −40, Δ*L* := (0, 3, −2) instead of Δ*R*. (5) *t* < D: *H* := (10, 0, −10); then as (1). |
| 45 (0x41550e) | 76 | **W**: *T*.x := −10; *H*.x := 10; *Q*.x := −60. **A**: ρ*L*.x = ρ*R*.x := 80; *H*.x := −60; *B*.x := −360·*u*; *K* := (0, −3, 0). **R**: *T*.x := 20; *H*.x := −10; *K* := (n.t., −2, 0). |
| 46 (0x415420) | 77 | **W**: *T*.z := −10; *T*.y := 30; ρ*L*.y = ρ*R*.y := −30; *H*.z := 60. **A**: ρ*L*.y = ρ*R*.y := 80; *B*.y := −360·*u*. **R**: *T*.y := −30; ρ*L*.y = ρ*R*.y := 30. |
| 47 (0x415332) | 78 | Mirror of 46: **W**: *T*.z := 10; *T*.y := −30; ρ*L*.y = ρ*R*.y := 30; *H*.z := −60. **A**: ρ.y := −80; *B*.y := 360·*u*. **R**: *T*.y := 30; ρ.y := −30. |
| 48 (0x4168ac) | 65 | *U* := record field 24. **W**: *q* = clamp(*t*/P1); *T*.y := 10*q*; *T*.z := −80*q*; ρ*R* := (−90, 90*q*, −30*q*); G((2, 0, 0)); *r* = 0.01. **A**: *T* := (10, 0, −90); ρ*R* := (0, 110, −20); *L* := *R*; *r* = 0.01. **R**: *B*.z := 20; *T* := (0, 0, 90); ρ*R* := (0, 90, 90); *L* := *R*. **E**: pose id := 0; G((−2, 0, −1)); ρ*R*.y := 90; *T*.z := −80; ρ*L* := ρ*R*. **A..E**: left arm base y += 10, z −= 4; *B*.z := 360·clamp(progress) (*as read*); ρ*L* := ρ*R*. |
| 49 (0x416d25) | 61, 66 | *U* := record field 24. **W**: *B*.z := 30; *T* := (0, 0, 90); ρ*R* := (−90, −90, 90); G((2, 0, 0)); ρ*L* := ρ*R*; *r* = 0.005. **A..D**: *T* := 0; ρ*R* := (−90, −90, 0); *E*.x := 0.25 while *t* < P1 + P2; G((−2, 0, 0)); *B*.z := −360·clamp(progress); *r* = 0.03. **E**: pose id := 0; G((−2, 0, −1)); ρ*R* := (−90, 90, 0); *T*.z := 0. |
| 50 (0x41d62d) | 20, 21 | Base: ρ*R* := (60, −20, 20); Δ*R* := (0, 2, 4); ρ*L* := (60, 20, −20); Δ*L* := (0, 2, 4). **W**: *T*.z := 50; ρ*L*.x := −90; ρ*L*.z := 90; ρ*R*.x := −90; Δ*L* := (−4, −4, −2); Δ*R* := (0, 4, 1). **A**: *q* = clamp(1.5·*u*): *B*.z := −270·*q* (*as read*); *B*.y := −40*q*; *S*.z += 4; ℓ := 40*q*; hands as W; *H*.x := −30; *H*.z := −60; *r* = 0.03. **R..**: guard idle as pose 19. |
| 51 (0x41414f) | 80 | While *t* < the held item's use limit (§6.5): *H* := (45 + 5·cos(0.02*t*), 0, 0); *T* := (10, 0, 0); *B* := (10, 0, 0); Δ*L* := (1, 7, 8); Δ*R* := (−1, 7, 8); ρ*L* := (100, 0, 0); ρ*R* := (100, 0, 0) (eating, drinking). |
| 52 (0x41409a) | 81 | Pose 57's settings, then pose 51's (sitting and eating). |
| 53 (0x4142fc) | 82 | *T* := (−30, 0, 0); Δ*L* := (4, 6, 6); Δ*R* := (−4, 6, 6); *S*.y := 3 (holding the pet item). |
| 54 (0x4143ab) | 79 | Only while moving (|intent|² > 0): *B*.x := 20; *T*.x := −10; *H*.x := −20; ρ*L*.x = ρ*R*.x := −20; Δ*L*.y += 4, Δ*R*.y += 4; *F* := −40; both foot bases y += 4 and z += n.t. |
| 55 (0x413f91) | 84 | *B* := (−90, 0, 0); *H*.x := 20; *E*.z := (box.y − box.z)/2 − 0.15; *K* := (0, −1, 0); Δ*L* = Δ*R* := (0, 0, 3) (lying down). |
| 56 (0x41e576) | field 17 > 0 | *T*.x += 30; Δ*L* := (−2, 0, 0); Δ*R* := (2, 0, 0); ρ*L* := (−10, −10, 90); ρ*R* := n.t. (stunned). |
| 57 (0x413afd) | 83 | *E*.z := −0.14; *T*.x := 10; Δ*L* = Δ*R* := (0, 4, 0); *K* := (0, 2, −2) (sitting on a seat, *inferred*). |
| 58 (0x413c3d) | 106 | **Riding** (with the creature passed as the last argument of 0x4128f0): *E*.y −= 0.25; *E* += a seat offset by that creature's type (0x446950); *E*.z += its box.z·0.75 + 0.3·4·*a*·*s₀*; types 22 and 23: *E*.z −= 0.75; *h* := −*E*.z. Then *T*.x := 20; Δ*L* := (4, 6, −3); Δ*R* := (−4, 6, −3); *K* := (0, 2, −2); ρ*L* := (60, 90, 60); ρ*R* := (60, −90, −60) (reins). |
| 59 (0x413e2b) | 107 | **Boat**: *T*.x := 20; *E*.z += 1.25; *B*.z := −50; Δ*L* := (4, 6, −3); Δ*R* := (−4, 6, −3); *K* := (0, 1, 0); *H* := (0, 0, 30); ρ*L* := (60, 90, 60); ρ*R* := (60, −90, −60). |
| 60 (0x418cdf) | 86 | *t* < 600: as pose 28 W. 600 ≤ *t* < 5000: *T*.z := 0; *B*.z := 360·*t*/300; ρ*L* := (0, −90, 0); ρ*R* := (0, 90, 0); *r* = 0.2. *t* ≥ 5000: as pose 28 R. |
| 61 (0x41b08b) | 30–34, 49, 87–90, 92, 105 | **W** (casting): *T*.x := 20; *H*.x := 20; Δ*L* := (3·cos(0.005*t*) − 1, 5, 5·sin(0.005*t*) + 5); Δ*R* := (3·cos(0.005*t* + π) + 1, 5, 5·sin(0.005*t* + π) + 5) (hands circle in opposition). **A**: *T*.x := −20; *T*.z := 10; Δ*L* := (2, 4, 0); Δ*R* := (−2, 6, 0); ρ*L* := (0, 45, 0); ρ*R* := (0, −45, 0). **R**: Δ*L* := (−3, −6, 0); Δ*R* := (3, −6, 0); ρ*L* := (−90, 0, 130); ρ*R* := (−90, 0, −130); *r* = 0.03. |
| 62 (0x41b3f0) | 35 | Pose 61 W at all times. |
| 63 (0x418e84) | 91 | Only for *t* < P1 + P2. **W**: one of two mirrored sets by a sign/parity test (n.t.): Δ*L* := (2, −2, 12), Δ*R* := (0, 0, −5), *T* := (−10, 0, 20), *B* := (−5, 0, 10); or Δ*L* := (0, 0, −5), Δ*R* := (−2, −2, 12), *T* := (−10, 0, −20), *B* := (−5, 0, −10); *r* = 0.03. **A**: Δ*L* := (2, −2, 12); Δ*R* := (−2, −2, 12); *T*.x := 20; *B*.x := 10. |
| 65 (0x41a2fd) | 96 | *t* < D: *T*.z := 90·sin(n.t.); Δ*L* = Δ*R* := 0; *r* = 0.04. |
| 66 (0x417f9e) | 26 | If obj +0x13b4 > 0 (M-11): **W**: *T*.z := −90; Δ*R* := (2, −2, 1); ρ*R* := (0, 10, 0); off-hand model replaced (n.t.). **A**: *T*.z := 10; Δ*L* := (−2, −2, 1); ρ*L* := (0, −10, 0); main-hand model removed. **R**: both weapon models removed; *T*.z := 90. **A..D**: *B*.z := min(360·(*t* − P1)/200, 700). Otherwise: **W**: *T*.z := −90; Δ*R* := (2, −2, 1); ρ*R* := (0, 10, 0); **after W**: *T*.z := 90; main-hand model removed (the boomerang is in flight). |
| 67 (0x41825e) | 27 | *T*.z := −90·(record field 23); Δ*R* := (2, −2, 1); ρ*R* := (0, 10, 0); *R*.z and *L*.z += 0.5·cos(n.t.); if field 23 > 0 the off-hand model is replaced (n.t.). |
| 68 (0x41b689) | 101, 103 | **W**: *T*.x := −20; ρ*L* := (−90, 0, 0); ρ*R* := (−90, 0, 0); Δ*R* := (1, −2, 0); Δ*L* := (−1, −2, 0). **after W**: *T*.x := 0; ρ*L* := (0, 45, 0); ρ*R* := (0, −45, 0); Δ*R* := (−2, 5, 0); Δ*L* := (2, 6, 0). |
| 69 (0x41916a) | 104 | *t* < P1 + P2: *B*.z := −360·(fraction, n.t.); ρ*R* := (−90, −90, −90); Δ*R* := (4, 3, 0); *T*.z := −60·*t*/(P1 + P2) (*as read*); ρ*L* := (0, 0, 90); *r* = 0.03. Otherwise: ρ*L* := (0, 0, 90); ρ*R* := (−60, −60, −60). |
| 70 (0x4144af) | 51 | **W**: *B*.x := −30. **A**: *B*.x := 120. |
| 71 (0x4183ac) | 108 | **W**: Δ*R* := (−3, −2, 10); Δ*L* := (3, −2, 10); the off hand draws model 2101 `iron-deposit` at the hands' midpoint + (0, 0, 8) (§6.5) with weapon scale ×1.5; *T*.x := 40; *B*.x := 10. **A**: Δ*R* = Δ*L* := (0, 5, 0); *T*.x := −20; *B*.x := −40. **R..**: Δ*R* := (2, −1, n.t.); Δ*L* := (−2, −1, 0) (lifting and throwing, *inferred*). |
| 72 (0x413bb1) | 109 | *t* < 5000: *T*.x := 20; *H*.x := 30; Δ*L* = Δ*R* := (0, 0, 16) (the mana cube is held overhead, §6.5). |
