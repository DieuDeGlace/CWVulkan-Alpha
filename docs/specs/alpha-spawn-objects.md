# Alpha spawn objects: the constructor, the finishing pass and the zone stream

Behavioural specification of the **spawn object** (`cube::Spawn`, 0x10f0 bytes) of the Cube
World **Alpha** client (`Cube.exe`, SHA-256 `84a7a132…30bf`): what its constructor writes,
**when the appearance generator and the NPC equipment generator run on it during zone
generation**, how many draws of the zone's random stream that costs per spawn and creature
type, and which other passes between the voxel-tree grid and the wildlife pass consume the
stream. Written from the Alpha decompile (`disassembled alpha cube world/decompiled/client`,
the retry decompile of the zone generator 0x5e4850 and the per-instruction listing), a raw
scan of `Cube.exe` and `Server.exe` for direct calls, and constants read from the binary. No
code was copied.

It answers a measurement: spawn objects read from the running game right after generation
already carry their type's appearance flags (appearance +0x06 at spawn +0x7a: type 69 → 0x28,
53 → 0x32, 30 and 100 → 0x31, a boss of type 67 → 0x228), and a reimplementation that never
generates appearances during world generation drifts in the zone's stream. Short answer:

* The constructor 0x5adb20 makes **no draws** and does **not** generate an appearance.
* Neither the wildlife pass nor any type setter nor the append to zone + 0x18 does it either.
* After **every** other pass of the zone generator (after the plant-sprite pass), a
  **finishing pass** (0x5ed2c6 … 0x5ed865) walks zone + 0x18 in order and, per spawn, runs the
  appearance generator with the spawn's **final** type, the equipment generator in mode 0, a
  boss spirit item, seven rarity rolls, up to nine level re-rolls and one discarded draw.
  Plants, deposits, companions, town people, dungeon creatures: all of them, once.
* The one exception, which **does** move draws before the wildlife pass: the location-creature
  routine 0x5dc6e0 (called at 0x5eace7, between the tree grid and the wildlife pass) runs the
  generator and the equipment generator **inline** on its ring creatures, which then get the
  finishing pass a second time.
* Two further passes between the tree grid and the wildlife pass are in no spec yet: a statue
  with 2–3 draws and a 39-draw candidate grid (§6). A reimplementation that lacks them drifts
  before the wildlife pass in every zone where they run.

Related: `alpha-creatures-models.md` §3–§4 and Appendix B (appearance record, defaults
0x428750, generator 0x43f7c0), `alpha-creatures-behaviour.md` §2.1, §9 (spawn fields,
activation, equipment generator 0x5c7730), `alpha-materials-wildlife-routes.md` §3 (wildlife
pass), `alpha-loot-rolls.md` §1–§2 (rarity roll 0x5f8530, level re-roll 0x4c7e80),
`alpha-missions.md` §3 (zone pass 0x5d9460), `alpha-worldgen-structures.md` (creators),
`alpha-worldgen-caves.md` §7 (ravine spawns), `alpha-worldgen-vegetation.md` §5 (tree grid),
`alpha-extractor.md` §2.6 (dumping zone + 0x18).

---

## 0. Conventions

* Addresses are `Cube.exe` virtual addresses (base 0x400000) unless marked **server**.
* "Spawn +0x…" is an offset in the 0x10f0-byte spawn object. "Appearance +0x…" is an offset in
  the 172-byte appearance record, which sits at spawn +0x74 (so appearance +0x06, the flags, is
  spawn +0x7a). "Slot *n*" is equipment slot *n* (`alpha-creatures-models.md` §6.4); its
  280-byte item record starts at spawn +0x120 + 0x118·*n*. Item fields as in
  `alpha-loot-rolls.md` §0: type +0x00, subtype +0x01, seed +0x04, rarity +0x0c, material
  +0x0d, level +0x10 (i16).
* **draw** = one call of the C runtime `rand()` (import slot 0x6fc468) on the generating
  thread; the zone generator seeds it once per zone (`alpha-worldgen-random.md`; the `srand`
  of 0x5e4850). Nothing in this document re-seeds. "d mod n" with a fresh draw is the plain
  remainder (draws are never negative).
* *As read* marks behaviour that looks unintended but is what the code does; *believed* marks
  readings resting on partial evidence.

---

## 1. Where things are

| Routine / range | What it does |
|---|---|
| 0x5adb20 | spawn constructor (§2); destructor 0x5adca0 |
| 0x428750 | appearance defaults (`alpha-creatures-models.md` §3.2) |
| 0x43bc00 | 13 empty equipment items |
| 0x43c020 | the inventory block at spawn +0xf6c |
| 0x43f7c0 | appearance generator (server 0x40a840) |
| 0x5c7730 | NPC equipment generator (`alpha-creatures-behaviour.md` §9.3) |
| 0x5f8530 | rarity roll, always 4 draws (`alpha-loot-rolls.md` §1) |
| 0x4c7e80 | level re-roll (`alpha-loot-rolls.md` §2) |
| 0x5c0120 | spawn id from zone and index (no draws) |
| 0x46ebe0 | add an item to an inventory (no draws) |
| 0x5e4850 | zone generator; its spawn creators and passes are listed in §7 |
| 0x5ea7e7 … 0x5eaa24 | the statue pass (§6.1) |
| 0x5eaa24 … 0x5eace7 | the location candidate grid and the call of 0x5dc6e0 (§6.2, §6.3) |
| 0x5dc6e0 | location creatures (camp props and rings of creatures; §6.3) |
| 0x5ed2c6 … 0x5ed865 | **finishing pass** over zone + 0x18 (§3) |
| 0x5edd64 | zone pass 0x5d9460 (`alpha-missions.md` §3), after the finishing pass |

Every direct call of the generator in `Cube.exe` (raw scan of `E8` calls): 0x42c3c2
(creation screen), 0x44c549 (character loader), 0x4621ea (start-up), 0x5d79b4 (wandering
group), 0x5d99ec (zone pass), 0x5deced (0x5dc6e0), **0x5ed32f (finishing pass)**, 0x60e69c,
0x60e8e3, 0x611205, 0x6159b4 (world step). Of these only 0x5deced and 0x5ed32f run inside
zone generation before the zone pass. The equipment generator is called at 0x5d79c9,
0x5d99fd, 0x5decfe, **0x5ed34f** and 0x60fd4a. The server has the same structure: its zone
generator calls the generator at server 0x52110f (client 0x5ed32f shifted by the same 0xcc220
as the whole routine).

---

## 2. The constructor (0x5adb20)

Called with the object in ECX on memory from `operator new` (0x68d668), which is **not**
zeroed. Field by field, in the order written:

| Spawn offset | Written | Meaning |
|---|---|---|
| +0x00 | 0x71dbec | vtable of `cube::Spawn` |
| +0x04 | — | **not written** (stale heap bytes) |
| +0x08 | 200.0 | activation radius, blocks |
| +0x0c | — | not written |
| +0x10 … +0x27 | — | **position (3 × i64) not written**; every creator writes it |
| +0x28 | 1 | controller kind |
| +0x2c | 4 | creature type (a placeholder; every creator overwrites it) |
| +0x30, +0x31 | 0, 0 (one u16 write) | class, specialization |
| +0x32, +0x33 | — | not written |
| +0x34 | 1 | level |
| +0x38, +0x3c | 0, 0 | respawn countdown, death day |
| +0x40, +0x44 | 0, 0 | active window (equal: always active) |
| +0x48, +0x4c | 0, 0 | creature id (low, high) |
| +0x50 | 0 (byte) | mode-flag byte |
| +0x54 | 0.0 | yaw |
| +0x58 | 0 (byte) | power tier |
| +0x5c | 0 | (to record field 42) |
| +0x60, +0x64 | −1, −1 | (+0x60 to record field 41; +0x64 *unknown*) |
| +0x68 | 0 | *unknown* |
| +0x6c | −1 | *unknown* |
| +0x70 | 0 | *unknown* |
| +0x74 … +0x11f | 0x428750 | appearance with the defaults of `alpha-creatures-models.md` §3.2: bytes 0–1 = 0, hair colour (255, 255, 255), byte 5 **not written**, flags 0, box (1, 1, 1), eight model indices 65535, the default scales, angles and offsets |
| +0x120 … +0xf57 | 0x43bc00 | 13 items, each: type and subtype 0, seed 0, +0x08 = 0, rarity and material 0, byte +0x0e = 0, **level 1**, the 256-byte spirit area zeroed, spirit count 0. Bytes +0x02–0x03, +0x0f, +0x12–0x13 of each item are not written |
| +0xf58 … +0xf68 | 100, 1, 1, 1, 1 | stat multipliers (health, attack speed, damage, armour, resistance) |
| +0xf6c … +0x109b | 0x43c020 | inventory block: a list header (+0xf6c, +0xf70, +0xf74 = 0, freed by the destructor), +0xf78 = 0, then an empty item at +0xf7c (level 1) whose role is *unknown* |
| +0x109c | 0 | behaviour object pointer |
| +0x10a0 … +0x10a8 | 0 | schedule list |
| +0x10ac … +0x10b4 | 0 | special-action list |
| +0x10b8 | 0 | a counter (the boss rule of §4.2 writes d mod 3 here) |
| +0x10bc … +0x10c4 | 0 | a second id list (the boss rule appends 17 and 96) |
| +0x10c8 | −1 | *unknown* |
| +0x10cc | 0 (byte) | name: first byte only; the rest of the buffer up to +0x10df is not written |
| +0x10e0, +0x10e4 | 0, 0 | *unknown* |
| +0x10e8 | 0 (byte) | "location boss" byte (`alpha-creatures-behaviour.md` §9.2 step 6) |
| +0x10e9 … +0x10ef | — | not written |

**No draws**: 0x5adb20, 0x428750, 0x43bc00 and 0x43c020 call nothing that draws. The
constructor does **not** generate an appearance, gives **no** default equipment (the earlier
wording "default equipment" in `alpha-creatures-behaviour.md` §2.1 means 13 empty items of
level 1) and sets no name. A comparison must skip the unwritten bytes above unless a later
writer fills them (§5).

---

## 3. The finishing pass (0x5ed2c6 … 0x5ed865)

### 3.1 When it runs

Inside 0x5e4850, after the surface pass (ravine spawns), the structure passes (dungeon, home
building, camps, town), the tree grid, the passes of §6, the wildlife pass and the plant-sprite
pass (0x5ebc60 … 0x5ec5e8), and after a loop that gives ids to the elements of another
per-zone list at zone + 0x24 (0x5ec62c; no draws; what that list holds is *unknown*). It is the
**last consumer of the zone's stream inside 0x5e4850 itself**; after it come object-list
bookkeeping loops (no draws found), zone + 0xa0 := the spawn count (the "base count" of
`alpha-missions.md` §3) and the zone pass 0x5d9460 (0x5edd64), which draws and makes its own
spawns.

### 3.2 Steps, per spawn

For index *i* = 0, 1, … over zone + 0x18 **in list order** (the order the creators appended,
§7), re-reading the count each time:

1. **Id**: if the spawn's id (+0x48, +0x4c) is 0, id := 0x5c0120(zone, *i*) =
   (zy·65536 + zx)·256 + *i* as a 64-bit value (`alpha-missions.md` §3). No draws. Spawns
   from the town and dungeon builders already have ids and keep them.
2. **Appearance**: the generator 0x43f7c0 with the type at +0x2c, the appearance at +0x74 and
   **no** choice record (§4.1).
3. **Equipment**: the equipment generator 0x5c7730 on the spawn, **mode 0** (§4.2).
4. **Boss spirit**: if the appearance has the boss flag 0x200: an item record (0x42f3e0)
   with type 11, subtype 14 (a **spirit**), level := spawn +0x34 (16 bits), rarity byte := 2,
   **draw**: material := 128 + d mod 4 (the spirit materials 128–131); it is added to the
   spawn's inventory (+0xf6c) by 0x46ebe0 with bag −1 (automatic). No draw without the flag.
5. **Rarity of seven slots**: for slots **7, 6, 5, 2, 4, 3, 1** in that order: rarity
   (+0x0c) := ρ(tier, boss) with tier = spawn +0x58 and boss = the 0x200 flag
   (0x5f8530: **4 draws each**, 28 in all; with the boss flag the result is min(tier + 1, 4),
   the draws are still spent). Empty slots get a rarity too.
6. **Level re-roll** (only without the boss flag): for slots **7, 6, 5, 2, 4, 3, 1, 8, 9** in
   that order, 0x4c7e80 with spread 0.05 and the widening flag **off**. With N = the slot
   item's level (+0x10), whatever the item type (empty slots included): N < 2 → nothing,
   no draw; N = 2 → level 1, no draw; N ≥ 3 → **one draw**, level := lo + d mod (N − lo + 1)
   with lo of `alpha-loot-rolls.md` §2 (the flag off makes hi = N, so the level only goes
   down or stays).
7. **Discarded draw**: if the controller kind (+0x28) is 1 **and** the boss flag is clear:
   **one draw**, result unused (*as read*).

So which type does the generator see? The **final** type: whatever the creator last wrote to
+0x2c. The constructor's placeholder 4 is never generated; the wildlife picker's type is the
one generated for animals, the plant type (120–130) for plants, the variant for companions
(`alpha-materials-wildlife-routes.md` §3), the deposit type for ravine deposits. Generation
happens **once** per spawn, except for 0x5dc6e0's ring creatures (§3.3).

### 3.3 Spawns generated twice

0x5dc6e0 (§6.3) runs the generator and the equipment generator on each ring creature right
after creating it (0x5deced, 0x5decfe), then appends it to zone + 0x18. The finishing pass
runs both again. Effects, *as read*:

* the appearance keeps the flags (OR) and the per-type models; the five no-record draws and
  any type draws are spent twice and the second run's choices win;
* the equipment generator clears the weapon and the armour slots 1–5 again but **not** their
  levels (and never touches slots 8 and 9), so armour levels left by the first run make extra
  re-roll draws in step 6;
* multipliers the generator **multiplies** are applied twice where the per-type step does
  not first re-assign them: types 47 and 88 get attack speed ×0.25; a ring boss (0x200,
  +0x10e8 = 1) of a type without per-type values gets health 100 × 15 × 15, capped at
  10 000, damage ×9, attack speed ×0.5625, armour and resistance +4 (for types whose
  per-type step assigns a value, that value is reset before the second boss step); its boss
  special and the +0x10bc entries are appended twice.

The zone pass (0x5d9460, after the finishing pass) generates the appearance and equipment
of the spawns it makes itself (`alpha-missions.md` §3.1, §3.2 step 7); they are not in the
finishing pass.

---

## 4. Draw counts

### 4.1 The appearance generator without a choice record (0x43f7c0)

Always **5 draws** first (blue, green, red, head choice, hair choice;
`alpha-creatures-models.md` §4.1). Then the per-type step adds:

| Type | Extra draws | Use |
|---|---|---|
| 6 bull terrier | 4 | hair colour (3), head |
| 46 ogre | 3 | hair colour (passed to 0x428920) |
| 58 parrot | 1 | parity: model set |
| 83, 84 desert nomads | 1 | head |
| 116 mammoth | 1 | box size |
| 118 rune giant | 1 | body |
| 119 | 8 | head, body, hand+foot, tail, head scale, a vector, two hand offsets |
| 124 scrub | 1 | body |
| 143 vase | 1 | body |
| 145, 146 fish | 1 | box size |
| every other type | 0 | |

These are all 28 `rand` call sites of the routine (5 + 4 + 3 + 1 + 1 + 1 + 1 + 1 + 8 + 1 + 1
+ 1). The generator only ORs flags into appearance +0x06 and overwrites what the type sets; it
does not reset the record to defaults.

### 4.2 The equipment generator in mode 0 (0x5c7730)

In this order (offsets and loadouts in `alpha-creatures-behaviour.md` §9.3):

1. Appearance flag 0x40: return, **0 draws** (no seeds either).
2. Class byte +0x30 = 0x88: **1 draw** (chest subtype), multipliers, return (no seeds).
   Class 0x80–0x87 or 0x89: 0 draws, multipliers, return.
3. Otherwise clear the weapon slots and armour slots 1–5 (types only). Then the first that
   applies:
   * flags & 0x18 ≠ 0, controller kind 6, or type 118: no weapons, **0 draws**;
   * types 117, 108, 114, 46, 109, 81, 82, 115, 47, 111, 112, 113: fixed loadout, **0 draws**;
   * types 43, 45: **1 draw** (staff or wand);
   * everything else: class draw, specialization draw (type 97: class forced to 4 after its
     draw), then the class's weapon draws — warrior 1 + (1 for a great weapon, 2 for two
     one-handers, 1 with a shield), ranger 1, mage 1, rogue 1 — then, unless flag 0x20, the
     armour draws: 3 (shoulders, gloves, boots) and for amulet and two rings 1 each, + 1
     more each time d mod 10 = 0. So 3–5 weapon-part draws (class and specialization
     included) and 0 or 6–9 armour draws.
4. Slots 7 and 6 **always** get level := spawn level here, weapon or not.
5. Per-type multipliers (no draws).
6. Boss flag 0x200: **2 draws** (one of five specials → +0x10ac; +0x10b8 := d mod 3).
7. **6 draws**: item seeds of slots 6, 7, 5, 2, 4, 3.

So: 0 (flag 0x40 or classes 0x80–0x89), 1 (class 0x88), else 6 + weapon part + armour part
+ 2 if boss.

### 4.3 One spawn in the finishing pass

Total = generator (5 + type extra) + equipment (§4.2) + boss spirit (1 if boss) + 28 +
re-roll (one per slot of 7, 6, 5, 2, 4, 3, 1, 8, 9 with level ≥ 3, non-boss only) + 1 if
controller 1 and not boss.

For an ordinary spawn the re-roll touches only slots 7 and 6 (level = spawn level by §4.2
step 4; the other slots stay at 1). Examples:

| Spawn | Flags after | Generator | Equipment | Spirit | Rarity | Re-roll | Discard | Total |
|---|---|---|---|---|---|---|---|---|
| wildlife 69, level 3–6 | 0x28 | 5 | 6 | 0 | 28 | 2 | 1 | **42** |
| wildlife 53, level 9–14 | 0x32 | 5 | 6 | 0 | 28 | 2 | 1 | **42** |
| wildlife 30 (3–6) or 100 (6–9) | 0x31 | 5 | 6 | 0 | 28 | 2 | 1 | **42** |
| wildlife boss 67 | 0x228 | 5 | 6 + 2 | 1 | 28 | 0 | 0 | **42** |
| animal of level 2 (range (2, 4)) | | 5 | 6 | 0 | 28 | 0 | 1 | 40 |
| plant 120–123, 125–130 (kind 6, level 1) | 0 | 5 | 6 | 0 | 28 | 0 | 0 | **39** |
| plant 124 scrub | 0 | 6 | 6 | 0 | 28 | 0 | 0 | 40 |
| fish 145/146, level 3–4 | 0x131 | 6 | 6 | 0 | 28 | 2 | 1 | 43 |
| ogre 46, level 31–47 | 0x20 | 8 | 6 | 0 | 28 | 2 | 1 | 45 |
| ravine deposit 131–138 (kind 6, level 1) | | 5 | 6 | 0 | 28 | 0 | 0 | 39 |
| type-9 guardian 107 (level ≥ 3) | 0x28 | 5 | 6 | 0 | 28 | 2 | 1 | 42 |
| town shopkeeper (class 0x80–0x84) | | 5 + type extra | 0 | 0 | 28 | one per slot whose level is already ≥ 3 | 1 only if controller 1 | |

(Levels from `alpha-materials-wildlife-routes.md` §3.4; flags from
`alpha-creatures-models.md` Appendix B. 69, 53, 30, 100, 67 and 107 have 0x08 or 0x10 and so
take no weapons.)

Worked example: a level-4 plant-creature (type 69) leader. Draws 1–5: generator (no type
draws). Equipment: flags 0x28 → no weapons, slots 7 and 6 get level 4, no multipliers, draws
6–11 the six seeds. No boss. Draws 12–39: rarities of slots 7, 6, 5, 2, 4, 3, 1 (tier 0: base
d mod 1 = 0, so each is 0 unless a bonus fires). Draw 40: slot 7 re-roll, lo = 2: level
2 + d mod 3; draw 41: slot 6 likewise. Draw 42: discarded (controller 1). Its companion, the
next spawn in the list, then takes its own 42.

---

## 5. What a comparison should expect right after generation

Everything below is written by the finishing pass, so a dump taken after 0x5e4850 returns
(`alpha-extractor.md` §2.6) contains it; a reimplementation that skips the pass differs in
all of these fields, and in every draw after it (the zone pass):

* +0x48 id; +0x74 … +0x11f appearance (flags OR-ed into +0x7a, models, scales, box, offsets
  of the type);
* +0x30, +0x31 class and specialization (general branch of §4.2, or type 86 → 3 and 1);
* the 13 items: weapon and armour types, subtypes, materials, levels, **rarity bytes of
  slots 7, 6, 5, 2, 4, 3, 1 (empty slots included)**, seeds of slots 6, 7, 5, 2, 4, 3;
  slot 6 and 7 levels = spawn level, then (non-boss) re-rolled down;
* +0xf58 … +0xf68 per type and boss (`alpha-creatures-behaviour.md` §9.3; mode 0);
* +0x10ac specials, +0x10b8 and +0x10bc (boss);
* the inventory at +0xf6c (boss spirit; heap pointers — compare contents, not addresses);
* untouched by every writer, so never comparable: +0x04, +0x0c, +0x32–0x33, appearance
  byte 5, the unwritten item bytes, +0x10cd … +0x10df, +0x10e9 … +0x10ef.

---

## 6. Between the tree grid and the wildlife pass

In code order after the tree grid loop ends (0x5ea7e1):

### 6.1 Statue (0x5ea7e7 … 0x5eaa24), when (zx + zy) mod 2 = 0

1. **draw**: x = 256·zx + 16 + d mod 224; **draw**: y = 256·zy + 16 + d mod 224 (*believed*
   x first: the first value goes with the outer column base).
2. If the column (x, y) does not exist (0x4347a0), stop (2 draws).
3. z: start at the column's top (+0x10 plus the stored count +0x1c) and go down while the
   block (low five bits) is 0 or 2; the statue stands on that block, at z + 1.
4. A static object (0x58eaa0) of **kind 0** (`goddess2`, `alpha-interaction.md` §4.2), at
   (x·65536 + 32768, y·65536 + 32768, (z + 1)·65536), scale (2, 2, 8); **draw**: rotation
   index = d mod 4; inserted into the statics at zone + 0x0c (0x5a0e00).

Draws: 0 when (zx + zy) is odd, else 2 or 3. (The parity is the same value the camp attempts
test, `alpha-worldgen-caves.md` §8 step 2, kept from there.)

### 6.2 Location candidate grid (0x5eaa24 … 0x5eacd5)

Skipped (no draws) when Lz's type (+0x18) is 0, 1, 5, 10 or 14. Otherwise, for i = 0 … 13
(outer) and j = 0 … 13 (inner), with c = i + 3j:

1. If c mod 5 ≠ 0, next cell (no draw).
2. x = 256·zx + 18i + 4, y = 256·zy + 18j + 4; v = 1 − f(Lz, (x·65536, y·65536)) (0x5f8e10);
   w = v² if v > 0, else 0.
3. **draw**; if d/32767 > 0.75·w (float), next cell.
4. If the column does not exist, next cell. z = the first height at or above the column's
   +0x14 whose block is 0 or 2.
5. Append (x·65536 + 32768, y·65536 + 32768, z·65536) to a local candidate list (24-byte
   entries, 0x5bf4f0).

Exactly **39 draws** whenever the grid runs (the 39 cells with c mod 5 = 0 each draw once).

### 6.3 Location creatures (0x5dc6e0, called at 0x5eace7)

Called right after the grid (even with an empty list) with the zone, Lz, the candidate list
and 5. Returns at once when Lz's type is 0. Outline (draw counts not yet specified; follow-up):

* **Set-up**: a creature "faction" is chosen from a type-dependent list by Lz +0x20 mod its
  size (no draw); some factions draw 1–2 times while filling their type lists (factions 1 and
  2 draw twice, faction 10 and the default faction not at all; the other factions were not
  read).
* The candidate deepest inside Lz (largest (1 − f)²) is marked, but only when Lz's centre is
  in this zone.
* **Per candidate**, in list order: neighbouring candidates are collected; depending on the
  lists either a single creature (0x5dedc4: draws for the list, yaw d mod 360 and type),
  a camp-like group (campfire kind 65 and a 2 × 2 ring of tent / stool / table / mat props
  with two draws each, as in `alpha-worldgen-structures.md` §2.2), then a **ring**:
  **draw** start angle φ = d·2π/32767 (double); **draw** k = d mod 3 + 1 creatures; creature
  c at θ = f32(2cπ/k + φ), 3 blocks from the candidate (x by cos θ, y by sin θ), yaw
  θ·180/π + 90, controller 1, flag 0x1000, **draw** type from the faction list, level and
  tier from Lz (+0x24, +0x28); on the marked candidate the first ring creature gets the boss
  flag 0x200 and +0x10e8 = 1; behaviours (Combat 20, WalkPath 2, RandomInteraction,
  RandomWalk; no draws); then **the appearance generator and the equipment generator
  (mode 0) inline** — §4.1 and §4.2 draws — and the append to zone + 0x18. A third creator
  (0x5df1d4, **draw** count d mod 3 + 1) appends without inline generation.

All of 0x5dc6e0's spawns go to zone + 0x18 and get the finishing pass.

### 6.4 The rest before the wildlife pass

* **Type-9 guardian** (0x5ead31 … 0x5eae65): when Lz has type 9 and its centre (+0x00,
  +0x08 as fx) lies in this zone: a spawn of type **107**, controller 1, at Lz's centre with
  z = the column's +0x10, level Lz +0x24, tier Lz +0x28, appended. **No draws.**
* The 64 × 64 sorted scan (0x5eae6a … 0x5eaf72): no draws (`alpha-materials-wildlife-routes.md`
  §3).

So between the tree grid and the first wildlife draw the stream advances by: statue 0/2/3,
then (location types other than 0, 1, 5, 10, 14) 39 + 0x5dc6e0's set-up, prop and ring draws
including **5 + type extra + equipment draws per ring creature**.

---

## 7. Spawn creators in zone-generation order

All append to zone + 0x18; none of them generates appearances except 0x5dc6e0's rings.

| Order | Creator | Spawn calls | Draws of its own |
|---|---|---|---|
| 1 | ravine spawns in the surface pass | 0x5e660a (deposits), 0x5e67c5 (bats) | `alpha-worldgen-caves.md` §7 |
| 2 | dungeon builder 0x5cc510 (0x5e84c3, 0x5e84f7) | 0x5d2591 … 0x5d5bd3 | structures §1.10.9 |
| 3 | home building / prefab placer 0x5f0ce0 (0x5e8d7b) | 0x5f3837, 0x5f41a2 | structures §6 |
| 4 | camps 0x5ad320 (0x5e8f3e) | none (props only) | structures §2 |
| 5 | town builder 0x5af170 (0x5e9672; also calls 0x5f0ce0) | 0x5b1c2e … 0x5bf016 | structures §5.5.9, §5.5.10 |
| 6 | tree grid | none | vegetation §5 |
| 7 | statue, candidate grid | none | §6.1, §6.2 |
| 8 | 0x5dc6e0 (0x5eace7) | 0x5de9a4 (rings, **generated inline**), 0x5dedc4, 0x5df1d4 | §6.3 |
| 9 | type-9 guardian | 0x5eadcc | 0 |
| 10 | wildlife pass | 0x5eb34d (leaders and plants), 0x5eb8d9 (companions) | wildlife §3 |
| 11 | plant sprites, street lights, ground items | none | vegetation §2–§4 |
| 12 | **finishing pass** | — | §3, §4 |
| 13 | zone pass 0x5d9460 (0x5edd64) | 0x5d9707, 0x5d703f, 0x5d7583 (generated inline) | missions §3 |

---

## 8. Notes for AlphaVulkan

* Keep the constructor's defaults exactly (§2) and generate nothing at construction; add a
  **finishing pass** after the plant-sprite pass that walks the zone's spawn list in creation
  order and performs §3.2 steps 1–7. Model it as one function per spawn so a mod can hook
  "spawn finished" (modding note: a mod changing a spawn's type must do so before this pass).
* 0x5dc6e0's inline generation (and its double application) must be kept for stream parity;
  keep the doubled boss multipliers behind a parity flag if a fixed mode is wanted later.
* Implement the statue and the 39-draw grid (§6.1, §6.2) before the wildlife pass; without
  them every zone with an even zx + zy, and every location zone, drifts there.
* The appearance generator's per-type draws are data (§4.1): keep them in the type table so
  added creature types declare their draw count.

---

## 9. Open questions

* **SO-1** 0x5dc6e0 is only outlined (§6.3): its faction lists, the neighbour test, the prop
  ring and the single-creature branches need a full draw-exact spec before location zones
  (Lz types 2–4, 6–9, 11–13) match in the wildlife pass. *Game check*: hook `rand` on the
  generating thread for one such zone and log return addresses (0x5dc6e0's sites, 0x43f7c0,
  0x5c7730) between 0x5eace7 and 0x5eaf75.
* **SO-2** The statue's axis order (§6.1, *believed* x first) and its kind 0 model: *check* a
  zone with even zx + zy in the extractor's statics dump (zone + 0x0c, kind 0, scale 2, 2, 8).
* **SO-3** The discarded draw of §3.2 step 7 is read from the assembly at 0x5ed845 (call, result
  unused); confirm with the `rand` hook that it fires once per non-boss controller-1 spawn.
* **SO-4** What zone + 0x24 holds (ids given at 0x5ec62c), and spawn fields +0x64, +0x68,
  +0x6c, +0x70, +0x10c8, +0x10e0, +0x10e4 and the item at +0xf7c.
* **SO-5** Test vector: for a non-location zone with odd zx + zy, the number of draws from the
  end of the plant-sprite pass to the zone pass should equal the sum of §4.3 over the zone's
  spawn list; the measured flags (69 → 0x28, 53 → 0x32, 30/100 → 0x31, boss 67 → 0x228) are
  consistent with §3.2 step 2 and Appendix B.
