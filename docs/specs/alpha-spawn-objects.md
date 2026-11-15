Status: FINAL / STABLE / HELD

# Alpha spawn objects: the constructor, the finishing pass and the zone stream

Behavioural specification of the **spawn object** (RTTI name `.?AUSpawn@cube@@`, 0x10f0 bytes)
of the Cube World **Alpha** client: what its constructor writes, when the appearance generator
and the NPC equipment generator run on it during zone generation, how many draws of the zone's
random stream that costs per spawn, and which other passes between the tree grid and the
wildlife pass consume the stream.

Evidence: `Cube.exe` (SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`)
and `Server.exe` (SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`),
read through the Alpha decompile and instruction listing, a raw scan of both images for
direct `E8` calls, a disassembly of an unlisted range of the zone generator, and constants
read from the image bytes. No code was copied.

Summary:

* The constructor (Cube.exe RVA 0x1adb20) makes **no draws** and does **not** generate an
  appearance.
* No spawn creator of the zone generator runs the appearance generator itself, except the
  location-creature routine (RVA 0x1dc6e0, §6.3), which runs the appearance generator and the
  equipment generator inline on its ring creatures.
* After every other pass of the zone generator, a **finishing pass** (RVA 0x1ed2c6 … 0x1ed865)
  walks the spawn list at zone + 0x18 in order and, per spawn, assigns a missing id, runs the
  appearance generator with the spawn's current type, the equipment generator in mode 0, a
  boss spirit item, seven rarity rolls, up to nine level re-rolls and one discarded draw.
  Ring creatures of RVA 0x1dc6e0 therefore receive both generators twice.
* Between the tree grid and the wildlife pass, a statue pass (0, 2 or 3 draws) and a location
  candidate grid (0 or exactly 39 draws) run, followed by RVA 0x1dc6e0 (§6).

Related (pointers only): `alpha-creatures-models.md` §3–§4 and Appendix B (appearance record,
generator), `alpha-creatures-behaviour.md` §2.1, §9 (spawn fields, equipment generator),
`alpha-materials-wildlife-routes.md` §3 (wildlife pass), `alpha-loot-rolls.md` §0–§2 (item
record, rarity roll, level re-roll), `alpha-missions.md` §3 (zone pass),
`alpha-worldgen-structures.md` (creators), `alpha-worldgen-caves.md` §7 (ravine spawns),
`alpha-worldgen-vegetation.md` §5 (tree grid), `alpha-interaction.md` §4.2 (static kinds),
`alpha-worldgen-random.md` (zone seed).

---

## 0. Conventions

* Addresses are **RVAs in Cube.exe** (image base 0x400000) unless marked **Server.exe**
  (image base 0x400000 as well).
* "Spawn +0x…" is an offset in the 0x10f0-byte spawn object. "Appearance +0x…" is an offset in
  the 172-byte (0xac) appearance record at spawn +0x74, so appearance +0x06 (the 16-bit flags)
  is spawn +0x7a. "Slot *n*" is equipment slot *n* (0 … 12); its 280-byte (0x118) item record
  starts at spawn +0x120 + 0x118·*n*. Item fields: type +0x00 (byte), subtype +0x01 (byte),
  seed +0x04 (32-bit), rarity +0x0c (byte), material +0x0d (byte), level +0x10 (signed 16-bit).
* **draw** = one call of the C runtime `rand()` through the import slot at RVA 0x2fc468 on the
  generating thread. The zone generator (RVA 0x1e4850) seeds it once per zone through the
  `srand` import slot (RVA 0x2fc4b4) at RVA 0x1e49b5, with zy·65536 + zx plus a 32-bit field at
  world-object + 0x800188; nothing in this document re-seeds. Draws are never negative, so
  "d mod n" with a fresh draw is the plain remainder.
* zx, zy are the zone generator's first and second arguments (each must lie in 0 … 65535,
  otherwise the generator does nothing); they are also stored at zone + 0x60 and + 0x64.
* "Lz" is the 0x68-byte location record the zone generator selects for the zone (address
  computed at RVA 0x1e4aa5 … 0x1e4ac0; the selection rule is outside this document). Fields
  used here: +0x00 and +0x08 the centre (64-bit, 65536 units per block), +0x18 the location
  type, +0x20 a selector, +0x24 a level (32-bit), +0x28 a tier (byte).
* Field names given for spawn offsets follow the companion specifications; this document
  certifies the written values and the code paths, not the names.
* *As read* marks behaviour that looks unintended but is what the code does.

---

## 1. Where things are

| RVA (Cube.exe) | What it does |
|---|---|
| 0x1adb20 | spawn constructor (§2); destructor 0x1adca0 |
| 0x28750 | appearance defaults (§2) |
| 0x3bc00 | 13 empty equipment items (§2) |
| 0x3c020 | the inventory block at spawn +0xf6c (§2) |
| 0x3f7c0 | appearance generator (§4.1); Server.exe RVA 0xa840 has the same prologue apart from absolute addresses |
| 0x1c7730 | NPC equipment generator (§4.2) |
| 0x1f8530 | rarity roll, always 4 draws (§3.2 step 5) |
| 0xc7e80 | level re-roll (§3.2 step 6) |
| 0x1c0120 | spawn id from zone and index (no draws) |
| 0x6ebe0 | add an item to an inventory (no draws) |
| 0x1e4850 | zone generator; its spawn creators and passes are listed in §7 |
| 0x1ea7e7 … 0x1eaa24 | statue pass (§6.1) |
| 0x1eaa24 … 0x1eacd5 | location candidate grid (§6.2) |
| 0x1eace7 | call of the location-creature routine 0x1dc6e0 (§6.3) |
| 0x1ed2c6 … 0x1ed865 | **finishing pass** over zone + 0x18 (§3) |
| 0x1edd64 | call of the zone pass 0x1d9460, after the finishing pass |

Every direct call of the appearance generator in Cube.exe (raw scan of `E8` calls): 0x2c3c2,
0x4c549, 0x621ea, 0x1d79b4, 0x1d99ec, 0x1deced (inside 0x1dc6e0), **0x1ed32f (finishing
pass)**, 0x20e69c, 0x20e8e3, 0x211205, 0x2159b4. The equipment generator is called directly at
0x1d79c9, 0x1d99fd, 0x1decfe (inside 0x1dc6e0), **0x1ed34f** and 0x20fd4a. Among the direct
callees of the zone generator other than the zone pass, only 0x1dc6e0 reaches either generator
through direct calls; 0x1d79b4/0x1d79c9 lie in RVA 0x1d6040, which the zone pass calls (at
0x1d9aa7), and 0x1d99ec/0x1d99fd lie in the zone pass itself. (Calls through function pointers
were not traced.)

Server.exe has the same structure: its zone generator starts at Server.exe RVA 0x118630 (client
RVA 0x1e4850 − 0xcc220) and calls its appearance generator (Server.exe RVA 0xa840) at Server.exe
RVA 0x12110f, i.e. client RVA 0x1ed32f − 0xcc220; the bytes around both call sites match apart
from the call displacements.

---

## 2. The constructor (RVA 0x1adb20)

Called with the object in ECX on memory from the runtime `operator new` (direct call target
RVA 0x28d668), which does **not** zero it; the creators examined here (zone generator and
RVA 0x1dc6e0) allocate 0x10f0 bytes.
Fields, in the order written (the name byte +0x10cc is written last, after +0x10e8):

| Spawn offset | Written | Meaning (companion naming) |
|---|---|---|
| +0x00 | 0x0071dbec (VA; RVA 0x31dbec) | vtable |
| +0x04 | — | **not written** |
| +0x08 | 200.0 (f32) | activation radius, blocks |
| +0x0c | — | not written |
| +0x10 … +0x27 | — | position (3 × i64) **not written** |
| +0x28 | 1 | controller kind |
| +0x2c | 4 | creature type (placeholder) |
| +0x30, +0x31 | 0, 0 (one 16-bit write) | class, specialization |
| +0x32, +0x33 | — | not written |
| +0x34 | 1 | level |
| +0x38, +0x3c | 0, 0 | respawn countdown, death day |
| +0x40, +0x44 | 0, 0 | active window |
| +0x48, +0x4c | 0, 0 | id (low, high) |
| +0x50 | 0 (byte) | mode-flag byte |
| +0x54 | 0 | yaw |
| +0x58 | 0 (byte) | power tier |
| +0x5c | 0 | *unknown* |
| +0x60, +0x64 | −1, −1 | *unknown* |
| +0x68 | 0 | *unknown* |
| +0x6c | −1 | *unknown* |
| +0x70 | 0 | *unknown* |
| +0x74 … +0x11f | RVA 0x28750 | appearance defaults: bytes 0–1 = 0, bytes 2–4 = 255, byte 5 **not written**, flags (+0x06) = 0, +0x08/+0x0c/+0x10 = 1.0, eight 16-bit model indices at +0x14 … +0x22 = 0xffff, then fixed scale/angle/offset floats from +0x24 to +0xa8 (values as listed in `alpha-creatures-models.md` §3.2) |
| +0x120 … +0xf57 | RVA 0x3bc00 | 13 items, each: type and subtype 0 (one 16-bit write), seed 0, +0x08 = 0, rarity and material 0 (one 16-bit write), byte +0x0e = 0, **level 1**, the 256 bytes at +0x14 zeroed, the 32-bit count at +0x114 = 0. Bytes +0x02–0x03, +0x0f and +0x12–0x13 of each item are not written |
| +0xf58 … +0xf68 | 100.0, 1.0, 1.0, 1.0, 1.0 | multipliers (health, attack speed, damage, armour, resistance) |
| +0xf6c … +0x109b | RVA 0x3c020 | inventory block: a list header (+0xf6c, +0xf70, +0xf74 = 0; the destructor frees this list), +0xf78 = 0, an empty item at +0xf7c written like the 13 items above (level 1; role *unknown*), then +0x1094 = 0 and +0x1098 = 0 |
| +0x109c | 0 | behaviour object pointer (freed by the destructor) |
| +0x10a0 … +0x10a8 | 0 | a list (freed by the destructor) |
| +0x10ac … +0x10b4 | 0 | special-action list (freed by the destructor) |
| +0x10b8 | 0 | a counter (§4.2 boss step writes d mod 3 here) |
| +0x10bc … +0x10c4 | 0 | a second id list (freed by the destructor; §4.2 boss step appends 17 and 96) |
| +0x10c8 | −1 | *unknown* |
| +0x10cc | 0 (byte) | name: first byte only; +0x10cd … +0x10df not written |
| +0x10e0, +0x10e4 | 0, 0 | *unknown* |
| +0x10e8 | 0 (byte) | "location boss" byte |
| +0x10e9 … +0x10ef | — | not written |

**No draws**: RVA 0x1adb20, 0x28750, 0x3bc00 and 0x3c020 reach no draw through direct calls.
The constructor does not generate an appearance, gives no equipment beyond the 13 empty
level-1 items, and sets no name.

---

## 3. The finishing pass (RVA 0x1ed2c6 … 0x1ed865)

### 3.1 When it runs

Inside the zone generator, in code order after: the surface pass containing the ravine spawn
creators, the structure passes (dungeon, prefab placer, camps, town), the tree grid, the passes
of §6, the wildlife pass, the pass over RVA 0x1ebc60 … 0x1ec5e8 (plant sprites in the
vegetation companion), and a loop over the per-zone pointer list at zone + 0x24 that gives each
element whose 64-bit field at element +0x00 is 0 the id RVA 0x1c0120(zx, zy, index) (call at
RVA 0x1ec62c; no draws; what that list holds is *unknown*).

The draw at RVA 0x1ed845 is the last direct `rand` call site of the zone generator. After the
pass come list bookkeeping loops whose direct callees reach no draw, then zone + 0xa0 := the
current count of zone + 0x18 (RVA 0x1edd5e), then the zone pass RVA 0x1d9460 (call at RVA
0x1edd64), which draws and makes its own spawns.

### 3.2 Steps, per spawn

The loop starts at index *i* = 0 when the list is non-empty, and continues while
*i* < the list's count, which is re-read after every spawn. Per spawn, in list order:

1. **Id**: if the 64-bit id (+0x48, +0x4c) is 0, id := (zy·65536 + zx)·256 + *i* as a 64-bit
   value (RVA 0x1c0120; no draws). A non-zero id is kept.
2. **Appearance**: the appearance generator (RVA 0x3f7c0) with the type at +0x2c, the
   appearance at +0x74 and **no** choice record (§4.1).
3. **Equipment**: the equipment generator (RVA 0x1c7730) on the spawn, **mode 0** (§4.2).
4. **Boss spirit**: only if appearance flag 0x200 is set: a fresh item record (constructed by
   RVA 0x2f3e0) gets type 11 and subtype 14 (one 16-bit write 0x0e0b), level := the low 16 bits
   of spawn +0x34, then **one draw**: material := 128 + d mod 4, rarity := 2; it is added to the
   inventory at +0xf6c by RVA 0x6ebe0 with bag argument −1. No draw without the flag.
5. **Rarity of seven slots**: for slots **7, 6, 5, 2, 4, 3, 1** in that order, rarity
   (+0x0c) := the rarity roll (RVA 0x1f8530) with tier = byte +0x58 and boss = flag 0x200.
   The roll always makes **4 draws** (28 in all): base := d₁ mod (tier + 1); +1 if
   d₂ mod 100 = 0; +1 if d₃ mod 1000 = 0; +1 if d₄ mod 10000 = 0; with the boss flag the result
   is tier + 1 instead (the draws are still spent); the result is capped at 4. Empty slots get
   a rarity too.
6. **Level re-roll**, only without the boss flag: for slots **7, 6, 5, 2, 4, 3, 1, 8, 9** in
   that order, the re-roll RVA 0xc7e80 with spread 0.05 and the widening flag **off**,
   regardless of item type (empty slots included). With N = the item's level (+0x10):
   * N < 2: unchanged, no draw;
   * otherwise g := 1 − 1 / ((N − 1)·0.05 + 1) and x := g − spread (single precision);
     if x ≤ 0 the level becomes 1 with no draw (this is the case for N = 2);
   * otherwise (every N ≥ 3) lo := max(1, truncate((1 / (1 − x) − 1)·20 + 1)), hi := N
     (flag off), **one draw**, level := lo + d mod (hi − lo + 1). The level never rises.
   (With the flag on, hi would instead come from the same inverse applied to
   min(g + spread, 0.9999); not used here.) Constants: 1.0 at RVA 0x345dc0, 0.05 at 0x2fcdb4,
   20 at 0x2fcd74, 0.9999 at 0x302a3c.
7. **Discarded draw**: if the controller kind (+0x28) is 1 **and** the boss flag is clear:
   **one draw** (call at RVA 0x1ed845), result unused (*as read*).

The type seen by the appearance generator is whatever +0x2c holds when the pass reaches the
spawn. Each spawn in the list is generated once by this pass; ring creatures of RVA 0x1dc6e0
were already generated once at creation (§3.3).

Fields written by the pass (directly or through the two generators): +0x48/+0x4c (if 0); the
appearance at +0x74 (flags OR-ed into +0x7a, box, models, scales and offsets of the type);
+0x30/+0x31 in the general branch of §4.2, +0x31 for types 43 and 45, and both (3 and 1) for
type 86 in the per-type step; the 13 items' types, subtypes,
materials and levels as §4.2 sets them, the rarity bytes of slots 7, 6, 5, 2, 4, 3, 1 and the
seeds of slots 6, 7, 5, 2, 4, 3; the re-rolled levels; +0xf58 … +0xf68; +0x10ac, +0x10b8,
+0x10bc (boss); the inventory at +0xf6c (boss spirit). The constructor-unwritten bytes of §2
(+0x04, +0x0c, +0x32–0x33, appearance byte 5, the unwritten item bytes, +0x10cd … +0x10df,
+0x10e9 … +0x10ef) are not written by the finishing pass.

### 3.3 Spawns generated twice

RVA 0x1dc6e0 runs the appearance generator and the equipment generator (mode 0) on each ring
creature right after creating it (calls at RVA 0x1deced and 0x1decfe), then appends it to
zone + 0x18; the finishing pass runs both again. Effects, *as read*:

* the appearance generator re-sets the box at appearance +0x08/+0x0c/+0x10, ORs its flags again
  and overwrites what the type sets; its 5 no-record draws and any per-type draws are spent
  twice, and the second run's results stand;
* the equipment generator clears the types and subtypes of slots 1–7 again but never resets
  levels and never clears slots 8 and 9, so levels left by the first run (and jewellery placed
  in slots 1, 8, 9 by either run) can add re-roll draws in step 6;
* multipliers that the generator multiplies or adds to are compounded where its per-type step
  does not first assign them: types 47 and 88 get attack speed × 0.5 twice (× 0.25); a ring
  boss (flag 0x200, +0x10e8 = 1) of a type without per-type values gets health 100 × 15 × 15
  capped at 10 000, damage × 9, attack speed × 0.5625, armour and resistance + 4; a field that
  the per-type step assigns is re-assigned before the second boss step; the boss's chosen
  special (+0x10ac) and the two +0x10bc entries are appended twice, and +0x10b8 is overwritten.

The zone pass runs after the finishing pass and itself contains (directly and through RVA
0x1d6040) calls of both generators for the spawns it makes; those spawns are not processed by
the finishing pass (`alpha-missions.md` §3).

---

## 4. Draw counts

### 4.1 The appearance generator without a choice record (RVA 0x3f7c0)

On entry it sets appearance +0x08, +0x0c, +0x10 to 0.96, 0.96, 2.16. Without a choice record
it always makes **5 draws** first: three colour components (the first draw supplies the third
byte of the local colour triple, the third draw the first byte), then two values each reduced
mod 100. Then the per-type step adds:

| Type | Extra draws | Use (appearance offsets) |
|---|---|---|
| 6 | 4 | three colour bytes at +0x02 … +0x04, then model +0x14 := 0x94b + d mod 3; this type returns without the common flag step |
| 46 | 3 | a colour triple passed to RVA 0x28920 |
| 58 | 1 | parity selects one of two model sets |
| 83 | 1 | model +0x14 := 0xb4 + d mod 3 |
| 84 | 1 | model +0x14 := 0xb7 + d mod 3 |
| 116 | 1 | a vector (s, s, 1.12·s) with s = 3·d/32767 + 7 |
| 118 | 1 | model +0x1c := 0x86b + parity |
| 119 | 8 | model +0x14 (d mod 5), model +0x1c (d mod 9), models +0x18/+0x1a (d mod 3), model +0x1e (parity), scale +0x24, one vector component, two offset components |
| 124 | 1 | model +0x1c := 0x82e or 0x82d by parity |
| 143 | 1 | model +0x1c := 0x841 + d mod 4 |
| 145, 146 | 1 | a uniform vector (s, s, s) with s = (0.5·d/32767 + 1.2)·0.8 |
| every other type | 0 | |

These are all 28 `rand` call sites of the routine (5 + 4 + 3 + 1 + 1 + 1 + 1 + 1 + 8 + 1 + 1 +
1), all unconditional within their branches. The routine only ORs values into the flags at
appearance +0x06 (it never assigns them); it does not reset the record to defaults.

Flag contributions confirmed for the types used in §4.3: 30, 100, 119 → 0x31; 53 → 0x32; 67,
69, 107 → 0x28; 46 → 0x20; 145, 146 → 0x131; 120–138 → none. (A flag such as 0x200 or 0x1000
set by a creator before generation is kept.)

### 4.2 The equipment generator in mode 0 (RVA 0x1c7730)

Slot item offsets: slot 1 +0x238, 2 +0x350, 3 +0x468, 4 +0x580, 5 +0x698, 6 +0x7b0,
7 +0x8c8, 8 +0x9e0, 9 +0xaf8. In this order:

1. Appearance flag 0x40: return at once, **0 draws**, nothing written.
2. Class byte +0x30:
   * 0x88: slot 2 := type 4, subtype 0, material 6; **1 draw**: slot 2 subtype := 2 + d mod 5;
     multipliers damage := 0.1, attack speed := 0.3, armour := 10, resistance := 10; return
     (no level writes, no seeds).
   * 0x80 … 0x87 or 0x89: the same without the draw (subtype 0); **0 draws**; return.
3. Otherwise clear type and subtype of slots 7, 6, 5, 2, 3, 4, 1 (levels untouched). Then the
   first rule that applies:
   * flags & 0x18 ≠ 0, controller kind (+0x28) = 6, or type 118: no weapons, **0 draws**;
   * types 117, 108, 114, 46, 115, 109, 81, 82, 47, 111, 112, 113: a fixed result
     (weapons and/or special-list entries, or nothing), **0 draws**;
   * types 43, 45: specialization := 2, slot 7 := type 3, **1 draw**: subtype 10 + parity,
     material 2; slot 6 cleared;
   * every other case (the general branch): **draw**: class +0x30 := 1 + d mod 4 (type 97:
     forced to 4 after this draw); then, per class, **draw**: specialization +0x31 := parity,
     then the weapon draws:
     * class 1: **draw** k := d mod 3; k = 0: one **draw** (slot 7 subtype 15 + d mod 3, slot 6
       cleared); k = 1: two **draws** (slot 7 subtype d mod 3, slot 6 type 3 with subtype
       d mod 3); k = 2: one **draw** (slot 7 subtype d mod 3, slot 6 := type 3 subtype 13);
     * class 2: one **draw** (slot 7 subtype 6 + d mod 3);
     * class 3: one **draw** k := d mod 3 (slot 7 subtype 10, 11, or for k = 2 subtype 12 with
       slot 6 also type 3 subtype 12);
     * class 4: one **draw** (slot 7 subtype 3 + d mod 3; slot 6 receives the same subtype
       unless it is 5).
     So 3–5 draws including class and specialization.
     Then, unless appearance flag 0x20 is set, the armour part: slot 2 := type 4 with level :=
     spawn level and material m (m = 25 if the slot 7 subtype is 10–12, 26 if 6–8, 27 if 3–5,
     else 1), no draw; three **draws**, each giving its slot (in order slot 5 type 7, slot 4
     type 5, slot 3 type 6) with level := spawn level and material m when the draw is odd;
     then three **draws** for slot 1 (type 8), slot 9 (type 9) and slot 8 (type 9) in that
     order, each placed with level := spawn level when d mod 10 = 0, followed in that case by
     one more **draw** choosing material 11 or 12. So 0 or 6–9 armour draws.
4. Slots 7 and 6 **always** get level := spawn level (+0x34) here, weapon or not.
5. Per-type multipliers (no draws); types 47 and 88 multiply attack speed by 0.5 (RVA
   0x345da0), every other per-type entry assigns; type 86 also sets class 3 and
   specialization 1 here.
6. Boss flag 0x200: health × 15, capped at 10 000; damage × 3; attack speed × 0.75; armour and
   resistance + 2; then **2 draws**: one of the five specials 0x5c, 0x5d, 0x56, 0x59, 0x57
   (d mod 5) is appended to +0x10ac, and +0x10b8 := d mod 3; 17 and 96 are appended to
   +0x10bc.
7. **6 draws**: item seeds of slots 6, 7, 5, 2, 4, 3, in that order.

Total: 0 (flag 0x40 or classes 0x80 … 0x87, 0x89), 1 (class 0x88), otherwise 6 + weapon
part + armour part + 2 if boss. (Mode ≠ 0 skips steps 5 and 6.)

### 4.3 One spawn in the finishing pass

Total = appearance generator (5 + type extra) + equipment (§4.2) + boss spirit (1 if boss) +
28 + re-roll (one per slot of 7, 6, 5, 2, 4, 3, 1, 8, 9 whose level is ≥ 3, non-boss only) +
1 if controller kind 1 and not boss.

For a spawn that takes the flag-0x18, controller-6 or fixed rules of §4.2 and whose other slots
still hold the constructor's level 1, the re-roll touches only slots 7 and 6 (both set to the
spawn level by §4.2 step 4). In the general branch, slot 2 and any armour or jewellery slot
given an item also carry the spawn level and are re-rolled.

Examples (each row states its assumptions):

| Spawn | Flags after | Generator | Equipment | Spirit | Rarity | Re-roll | Discard | Total |
|---|---|---|---|---|---|---|---|---|
| type 69, controller 1, level ≥ 3, not boss | 0x28 | 5 | 6 | 0 | 28 | 2 | 1 | **42** |
| type 53, same assumptions | 0x32 | 5 | 6 | 0 | 28 | 2 | 1 | **42** |
| type 30 or 100, same assumptions | 0x31 | 5 | 6 | 0 | 28 | 2 | 1 | **42** |
| type 67 with boss flag set by its creator | 0x228 | 5 | 6 + 2 | 1 | 28 | 0 | 0 | **42** |
| type 69, controller 1, level 2 | 0x28 | 5 | 6 | 0 | 28 | 0 | 1 | 40 |
| type 120–123 or 125–130, controller 6, level 1 | 0 | 5 | 6 | 0 | 28 | 0 | 0 | **39** |
| type 124, controller 6, level 1 | 0 | 6 | 6 | 0 | 28 | 0 | 0 | 40 |
| type 145/146, controller 1, level ≥ 3 | 0x131 | 6 | 6 | 0 | 28 | 2 | 1 | 43 |
| type 46, controller 1, level ≥ 3 | 0x20 | 8 | 6 | 0 | 28 | 2 | 1 | 45 |
| type 131–138, controller 6, level 1 | 0 | 5 | 6 | 0 | 28 | 0 | 0 | 39 |
| type 107, controller 1, level ≥ 3 | 0x28 | 5 | 6 | 0 | 28 | 2 | 1 | 42 |
| class byte 0x80 … 0x87 or 0x89 | | 5 + type extra | 0 | 0 | 28 | one per slot whose level is already ≥ 3 | 1 if controller 1 | |

(Types with flag 0x08 or 0x10 take no weapons. Levels, controller kinds and boss flags of
actual creators are in the companion specifications.)

Worked example: a level-4, controller-1, tier-0 spawn of type 69. Draws 1–5: appearance
generator (no type draws). Equipment: flags 0x28 → no weapons; slots 7 and 6 get level 4; no
per-type multipliers; draws 6–11 the six seeds. No boss. Draws 12–39: rarities of slots 7, 6,
5, 2, 4, 3, 1 (tier 0: base d mod 1 = 0, so each is 0 unless a bonus fires). Draw 40: slot 7
re-roll with lo = 2 (x ≈ 0.0804): level := 2 + d mod 3; draw 41: slot 6 likewise. Draw 42:
discarded. The next spawn in the list then takes its own draws.

---

## 5. (Withdrawn)

The former comparison section was removed in the re-certification; the fields written by the
finishing pass are listed in §3.2.

---

## 6. Between the tree grid and the wildlife pass

In code order after the tree-grid loop ends (its last back-branch is at RVA 0x1ea7e1):

### 6.1 Statue (RVA 0x1ea7e7 … 0x1eaa24), when (zx + zy) mod 2 = 0

The parity is a stored value computed at RVA 0x1e8d80 … 0x1e8d9a (after the prefab-placer
call); when it is 0 the zone generator also skips the code range RVA 0x1e8da6 … 0x1e8f8d, which
contains the camp-builder call. It is not rewritten before the statue test.

1. **draw**: x := 256·zx + 16 + d mod 224; **draw**: y := 256·zy + 16 + d mod 224 (x first).
2. If the column (x, y) does not exist (lookup RVA 0x347a0), stop (2 draws).
3. z := the sum of the column's +0x10 and +0x1c fields; while the block at (x, y, z) (lookup
   RVA 0x2f7e0; low five bits of its byte +3) is 0 or 2, z := z − 1. The statue stands on
   that block, at z + 1.
4. A static object (constructed by RVA 0x18eaa0) of **kind 0** (`alpha-interaction.md` §4.2) at
   (x·65536 + 32768, y·65536 + 32768, (z + 1)·65536), scale (2, 2, 8); **draw**: rotation index
   := d mod 4; inserted into the statics at zone + 0x0c (RVA 0x1a0e00).

Draws: 0 when zx + zy is odd, else 2 or 3.

### 6.2 Location candidate grid (RVA 0x1eaa24 … 0x1eacd5)

Skipped entirely (no draws, and RVA 0x1dc6e0 is not called) when Lz's type (+0x18) is 0, 10,
14, 1 or 5. Otherwise, for i = 0 … 13 (outer) and j = 0 … 13 (inner), with c = i + 3j:

1. If c mod 5 ≠ 0, next cell (no draw). (The divisor 5 is a stored value written at RVA
   0x1e9ea2.)
2. x := 256·zx + 18i + 4, y := 256·zy + 18j + 4; v := 1 − f(Lz, x·65536, y·65536) (RVA
   0x1f8e10); w := v² if v > 0, else 0.
3. **draw**; if d / 32767 > 0.75·w (single precision), next cell.
4. If the column (x, y) does not exist, next cell. z := the column's +0x14 field, raised by one
   while the block (low five bits of byte +3) is neither 0 nor 2.
5. Append (x·65536 + 32768, y·65536 + 32768, z·65536) to a local list of 24-byte entries
   (RVA 0x1bf4f0).

Exactly **39 draws** whenever the grid runs (the 39 cells with c mod 5 = 0 each draw once).

### 6.3 Location creatures (RVA 0x1dc6e0, called at RVA 0x1eace7)

Called right after the grid, even with an empty list, with the zone, Lz, the candidate list and
the constant 5 (world object as receiver). Confirmed parts (the routine is only partly
specified here):

* **Return** at once when Lz's type is 0.
* **Faction**: a list is built (type 4: [9]; otherwise [0, 1, 2, 3, 4, 5, 1, 7, 8], then 6
  unless the type is 13 or 2, then 10 if the type is 3); the faction is the entry at
  (Lz +0x20) mod list length (no draw). While filling its type lists, faction 1 draws twice,
  factions 2 and 3 twice, faction 4 once, factions 0 and 5–10 and the default not at all.
* **Mark**: only when Lz's centre (+0x00, +0x08 divided by 65536 and then by 256, rounding
  toward zero) equals (zx, zy): the candidate with the largest (1 − f)² (strictly greater than
  0 and than every earlier candidate) is marked; otherwise none is.
* **Per candidate**, in list order: the other candidates whose horizontal squared distance in
  blocks lies strictly between 25 and 16384 are collected; then one of several branches runs
  (the selection rules are unresolved, §8). Confirmed branch contents:
  * single creature (constructor at RVA 0x1dedc4): **draw** an entry of a group list filled
    during set-up (d mod its length; if the entry's type list is empty nothing is created for
    this candidate); controller 1, flag 0x1000, **draw** yaw := d mod 360, **draw** type from
    the entry's type list (d mod length); level and tier from Lz +0x24 and +0x28; when neighbours were collected,
    three **draws**, each picking a neighbour (d mod count); appended to zone + 0x18; then, if
    the entry's second list is non-empty, **draw** n := d mod 3 + 1 followers (constructor at
    RVA 0x1df1d4), each at the creature's position, controller 1, flag 0x1000, **draw** type,
    level and tier from Lz, appended without inline generation;
  * camp-like group: a campfire of static kind 65 with one **draw** (d mod 4), then four prop
    attempts on a 2 × 2 layout, each with two **draws** (a kind among 16, 12, 69 and 66 chosen
    by d mod 4, then d mod 4); a prop is inserted only when a placement check (RVA 0x1f4da0,
    no draws) accepts it, but its draws are spent either way;
  * **ring** (constructor at RVA 0x1de9a4): **draw** φ := f32(d·2π / 32767) (double
    arithmetic); **draw** k := d mod 3 + 1 creatures; creature c (0 … k − 1) at
    θ := f32(2cπ / k + φ), placed at the candidate plus truncate(f32(cos θ)·3·65536) in x and
    truncate(f32(sin θ)·3·65536) in y (products in single precision), same z;
    yaw := f32(f32(θ·180) / π + 90); controller 1, flag 0x1000; **draw** type from a type
    list filled during set-up (d mod length); level and tier from Lz +0x24
    and +0x28; on the marked candidate the first ring creature (c = 0) also gets flag 0x200
    and +0x10e8 = 1; four behaviour entries are attached through the object at +0x109c (no
    draws); then **the appearance generator and the equipment generator (mode 0) inline**
    (§4.1, §4.2 draws), and the append to zone + 0x18.

All of RVA 0x1dc6e0's spawns go to zone + 0x18 and get the finishing pass.

### 6.4 The rest before the wildlife pass

* **Type-9 guardian** (RVA 0x1ead31 … 0x1eae65): when Lz's type is 9 and its centre (+0x00,
  +0x08 divided by 65536 and by 256, rounding toward zero) equals (zx, zy): a spawn
  (constructor at RVA 0x1eadcc) at Lz's centre (x, y copied as 64-bit values) with
  z := the column's +0x10 field × 65536, controller 1, type **107**, level := Lz +0x24, tier :=
  Lz +0x28, appended to zone + 0x18. **No draws.**
* A 64 × 64 loop (RVA 0x1eae6a … 0x1eaf72) collecting entries with a squared distance and
  sorting them: no draws. The first wildlife draw follows at RVA 0x1eaf9c.

So between the end of the tree grid and the first wildlife draw the stream advances by the
statue's 0/2/3, then (location types other than 0, 1, 5, 10, 14) 39 plus RVA 0x1dc6e0's
draws, which include the full §4.1 and §4.2 draws of every ring creature.

---

## 7. Spawn creators in zone-generation order

All append to zone + 0x18 (confirmed at the append following each constructor site for most
sites; see §8); none generates appearances except RVA 0x1dc6e0's rings.

| Order | Creator | Constructor call sites (RVA) | Draws of its own |
|---|---|---|---|
| 1 | ravine spawns in the surface pass | 0x1e660a, 0x1e67c5 | `alpha-worldgen-caves.md` §7 |
| 2 | dungeon builder 0x1cc510 (called at 0x1e84c3, 0x1e84f7) | 0x1d2591, 0x1d28f6, 0x1d2c5b, 0x1d2fc7, 0x1d3ae0, 0x1d4d2a, 0x1d5443, 0x1d5778, 0x1d5bd3 | structures companion |
| 3 | prefab placer 0x1f0ce0 (called at 0x1e8d7b) | 0x1f3837, 0x1f41a2 | structures companion |
| 4 | camp builder 0x1ad320 (called at 0x1e8f3e) | none | structures companion |
| 5 | town builder 0x1af170 (called at 0x1e9672; it also calls 0x1f0ce0) | 13 sites from 0x1b1c2e to 0x1bf016 | structures companion |
| 6 | tree grid | none | vegetation companion §5 |
| 7 | statue, candidate grid | none | §6.1, §6.2 |
| 8 | 0x1dc6e0 (called at 0x1eace7) | 0x1de9a4 (rings, **generated inline**), 0x1dedc4, 0x1df1d4 | §6.3 |
| 9 | type-9 guardian | 0x1eadcc | 0 |
| 10 | wildlife pass | 0x1eb34d, 0x1eb8d9 | wildlife companion §3 |
| 11 | pass over 0x1ebc60 … 0x1ec5e8 | none | vegetation companion |
| 12 | **finishing pass** | — | §3, §4 |
| 13 | zone pass 0x1d9460 (called at 0x1edd64) | 0x1d9707; through 0x1d6040: 0x1d703f, 0x1d7583 | missions companion §3 |

---

## 8. Unresolved

* **SO-1** RVA 0x1dc6e0 is only partly specified (§6.3): the rule choosing between the
  single-creature, camp-like and ring branches per candidate (including two parity draws seen
  in one alternative), the contents of the faction type lists, the meaning of the constant
  argument 5, and the exact prop placement are not established, so its total draw count per
  zone is not given.
* **SO-2** What the per-zone list at zone + 0x24 holds; meanings of spawn fields +0x5c, +0x60,
  +0x64, +0x68, +0x6c, +0x70, +0x10c8, +0x10e0, +0x10e4 and of the item at +0xf7c.
* **SO-3** Whether every creator overwrites the constructor's placeholder type 4 before the
  finishing pass, and which creators pre-assign ids (+0x48/+0x4c) so that step 1 keeps them
  (a write to +0x4c was seen near one dungeon constructor site; not traced).
* **SO-4** A few constructor sites (town 0x1bcbf6, 0x1bd5c6, 0x1bec57; dungeon 0x1d5443,
  0x1d5778, 0x1d5bd3) were not individually traced to an append to zone + 0x18; the
  surface-pass sites lie in a range missing from the instruction listing and were only seen in
  a raw disassembly.
* **SO-5** Draw-freedom statements rest on direct calls; calls through function pointers or
  virtual tables in the routines named here were not traced.
* **SO-6** Static kinds 16, 12, 69, 66 and 65 in §6.3 and kind 0 in §6.1 are given by number
  only; their models are in `alpha-interaction.md`, which was not re-checked here.

---

## Re-certification (2026-11-15)

Checked against Cube.exe and Server.exe (hashes above), the client decompile and instruction
listing, raw `E8` scans of both images, a raw disassembly of RVA 0x1e6460 … 0x1e69a0, and
constants read from the image bytes. All addresses were converted from virtual addresses to
RVAs (VA − 0x400000) with the image named.

**Confirmed**: spawn size 0x10f0 and allocation by the runtime `operator new`; every constructor
write and value, the default-appearance, item and inventory initializers and the 172-byte
appearance layout; the generator and equipment-generator call-site lists; the Server.exe
offset 0xcc220 and the server call site; the finishing-pass range, list order, re-read count,
id formula, boss spirit (type 11, subtype 14, material 128 + d mod 4, rarity 2, bag −1), the
rarity slot order 7, 6, 5, 2, 4, 3, 1 with 4 draws each, the re-roll slot order 7, 6, 5, 2, 4,
3, 1, 8, 9 with spread 0.05 and flag off, the N = 2 / N ≥ 3 behaviour, and the discarded draw
at RVA 0x1ed845; zone + 0xa0 := count before the zone pass; all 28 draw sites of the
appearance generator and their per-type counts; the equipment generator's early returns,
fixed-type list, general-branch draw structure, boss rule (×15 capped 10 000, ×3, ×0.75, +2,
specials d mod 5, +0x10b8 := d mod 3, 17 and 96) and the seed order 6, 7, 5, 2, 4, 3; every
total and the worked example of §4.3 under the stated assumptions; the statue's condition,
draws, scale, kind and insertion; the candidate grid's skip types, cell rule, divisor 5,
threshold 0.75·w and the count of 39; the call of RVA 0x1dc6e0 with the constant 5; the ring's
angle, count, placement, yaw, boss marking and inline generation; the third creator's
d mod 3 + 1 count; the guardian spawn; the creator table's call sites.

**Corrected** (old → new):

1. All addresses were virtual addresses → stated as image-named RVAs.
2. Constructor write order: "+0x10cc before +0x10e0" → +0x10cc is written last, after +0x10e8.
3. Inventory block: added the zero writes at +0x1094 and +0x1098.
4. Appearance generator: "does not reset the record" → it first sets the box (+0x08, +0x0c,
   +0x10) to 0.96, 0.96, 2.16 on every call; per-type "use" entries restated as appearance
   offsets.
5. Equipment generator, class 0x88/0x80–0x89 branches: added the slot 2 writes and the exact
   multiplier values.
6. Equipment generator, general branch: the armour part also always gives slot 2 (chest) the
   spawn level; slots 5, 4, 3 are given on odd draws; jewellery slots 1, 9, 8 (in that draw
   order) also receive the spawn level.
7. §4.3: "for an ordinary spawn the re-roll touches only slots 7 and 6" → true only for
   no-weapon/fixed-rule spawns; general-branch spawns also re-roll slot 2 and any armour or
   jewellery slot they received. Affects the draw count of every general-branch spawn
   (world generation / random-draw order).
8. §3.3: "never touches slots 8 and 9" → never clears them or resets levels, but the general
   branch can place items there; affects re-roll draw counts of doubly generated ring creatures
   (random-draw order).
9. Statue axis order: *believed* x first → confirmed x first (first draw pairs with
   256·zx and is the column lookup's first coordinate). **Reversal of an open question**, no
   change of value.
10. Location-creature set-up: "factions other than 1, 2, 10 and default not read" → faction 3
    draws twice, faction 4 once, factions 0 and 5–9 not at all.
11. Single-creature branch: added the three neighbour-pick draws and the optional follower
    group of RVA 0x1df1d4; camp-like group: added the campfire's own draw (one draw before the
    props). Affects the documented draw order inside location zones (random-draw order).
12. Ring placement: added single-precision rounding of φ, θ and the sine/cosine and truncation
    of the offsets.
13. Guardian: centre test stated precisely (division by 65536 then by 256, toward zero); x, y
    are copied from Lz as 64-bit values.
14. §1: "only 0x5deced and 0x5ed32f run inside zone generation before the zone pass" → stated
    in terms of direct callees (only RVA 0x1dc6e0 reaches either generator); RVA 0x1d6040 is
    reached through the zone pass.
15. §3.1: "last consumer of the zone's stream inside 0x5e4850 itself" → the last direct `rand`
    call site of the zone generator.
16. Rarity roll and level re-roll: rules stated in full from the original instead of pointers.

**Removed**: the opening paragraph about a measurement and a reimplementation drifting; the
sentence that a comparison must skip unwritten bytes; the former §5 ("What a comparison should
expect", reworded as the list of fields written by the finishing pass in §3.2); the sentence in
§6 about a reimplementation lacking the passes; the former §8 "Notes for AlphaVulkan"; the
game-check, hook, extractor and test-vector procedures of the open questions; caller role names
for the generator call sites (creation screen, character loader, start-up, wandering group,
world step); creature names in the generator table; the record-field mappings of +0x5c and
+0x60; the claims that the constructor's type 4 is never generated and that town and dungeon
spawns already carry ids (moved to unresolved); the prop names tent / stool / table / mat.

**Unresolved**: SO-1 … SO-6 above.
