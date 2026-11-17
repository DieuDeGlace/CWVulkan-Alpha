# Alpha missions and map cells

Status: FINAL / STABLE / HELD (re-certified 2026-11-16 against the original Alpha evidence;
see the re-certification section at the end)

Behavioural specification of Cube World **Alpha**'s missions ("quests"): the daily
"missions generated" pass over a region, the mission and monster areas of the 104-byte
location record of each **map cell**, the extra spawn objects the pass adds to zones, how a
mission is discovered, counted and completed, its rewards, the network record that carries
it (world-update **section 13**), its save keys, and how the client shows it (world-map
markers, the small quest tag and progress bar, the "QUEST COMPLETE!" banner, the objective
text).

Written from the Alpha decompile (`disassembled alpha cube world/decompiled/server` and
`…/client`), the per-instruction listings, and a disassembly of the two binaries where the
listing has gaps (the step routine server 0x5322d0 / client 0x60c510 failed to decompile).
No code was copied. Routines are cited as "server 0x…" (`Server.exe`) or "client 0x…"
(`Cube.exe`); offsets are given for cross-checking only. Readings marked *believed* rest on
partial evidence; *as read* marks behaviour that looks unintended but is what the code does.

This document **adds to** and does not repeat:

* `alpha-worldgen-locations.md` — the region object, the 64 location records (§1.2), how
  types, levels and sub-levels are chosen, phase H (§9: loading the stored areas);
* `alpha-worldgen-regions.md` — region cells C, owner(x, y) (§3.4), the normalised location
  distance f(L, x, y) (§7);
* `alpha-server-simulation.md` — the step (§3), spawn activation and the mission adjustment
  of kind-5 creatures (§5.2 step 5), NPC death (§6.2), item gifts and kill records (§6.2
  step 7), the inn (§8 kind 2);
* `alpha-creatures-behaviour.md` — spawn objects (§2), the level curve (§4.6), kill
  experience and the mission switch (§8), the NPC equipment generator (§9), behaviours
  (§11);
* `alpha-net-packets.md` §3.9 (the section-13 record; §6 here gives the same layout with
  field meanings);
* `alpha-saves.md` §5.3, §5.4 (the `mission` and `monster` keys; **V-5** answered in §7);
* `alpha-ui-screens.md` §5.2.4 (map markers), §5.2.8 (open points answered in §8.1), and
  its objective widgets (partly answered in §8.4);
* `alpha-talk-vendors.md` §2.3 (dictionary variables), §2.6 (no quest dialogue);
* `alpha-items-generator.md` and `alpha-loot-rolls.md` (armour and weapon generators).

---

## 0. Conventions

* **Zone** = 256 × 256 blocks; **map cell** = 8 × 8 zones (2048 blocks, the "slot" of the
  locations spec); **region** = 8 × 8 map cells = 64 × 64 zones. Cell (cx, cy) belongs to
  region (cx ÷ 8, cy ÷ 8) and uses location record 8·(cx mod 8) + (cy mod 8) of that region
  (`alpha-worldgen-locations.md` §0). Below, **L** is the location record of the cell being
  discussed, and "the cell's zones" are the 64 zones (8·cx … 8·cx + 7, 8·cy … 8·cy + 7).
* **C** is the region's own region cell, the cell (rx, ry) of the region-cell table (the
  cell whose climate the locations spec §3 uses): warmth (+0x10), moisture (+0x0c), fire
  flag (+0x08).
* **Players** means every creature of the world with controller kind 0: on the server, all
  connected players; offline, the local player. **P_max** is the highest player level among
  them.
* **draw** means one call of the C library `rand()` (MSVC: 15-bit result 0 … 32767). The
  mission routines **never seed** the generator: they continue whatever stream the step is
  in. Missions are therefore not a function of the world seed; two servers with the same
  seed get different missions. Only the draw order matters for parity with a recording.
  (The stream is, however, re-seeded by other work of the step: the activation of a kind-5
  cell's creatures calls srand(seed + spawn id), §3.3, so every later draw of the step,
  mission draws included, follows from that seed.)
* **c(x)** = 1 − 1/((x − 1)·0.05 + 1), the level curve (server 0x407d60, client 0x43ca60;
  `alpha-creatures-behaviour.md` §4.6, §8). **f(L, p)** is the normalised location
  distance and **bump(L, p)** = max(1 − f(L, p), 0)² (server 0x52c820 / 0x52dee0, client
  0x5f8e10 / 0x5fa4c0; `alpha-worldgen-regions.md` §7, `alpha-ui-screens.md` §5.9.1). A point
  is **inside** L when bump > 0, i.e. f < 1.
* The world flag **remote** (world + 0xb4) is set on a client connected to a server. Every
  routine here does nothing on a remote world; it runs on the server and on an offline
  client (§9).

Routines (the two programs make the same decisions in the same order):

| Routine | Server | Client twin |
|---|---|---|
| Daily pass for one region (the "driver") | 0x50d8d0 | 0x5d9ad0 |
| Build the mission of one cell | 0x50c550 | 0x5d8750 |
| Zone pass: rebuild a zone's extra spawns | 0x50d260 | 0x5d9460 |
| Wandering group for one zone | 0x509e40 | 0x5d6040 |
| Clear "missions generated" in every loaded region | 0x524500 | 0x5f0c80 |
| Monster type for a level | 0x52b230 | 0x5f7820 |
| Mission creature type (nine-entry list) | 0x52ae10 | 0x5f7400 |
| Wildlife species picker (also used by the zone generator) | 0x5290d0 | 0x5f56c0 |
| Location record of a cell | 0x4286f0 | 0x487da0 |
| Mission progress on an NPC death | inside 0x4cea80 | inside 0x595a60 |
| Save one region's areas | 0x4d7c50 | 0x5a4af0 (writers 0x44e120, 0x44e280) |

---

## 1. The mission and monster areas of a location record

The location record (0x68 bytes, `alpha-worldgen-locations.md` §1.2) holds two areas that
the world generator leaves at zero and only this system fills.

### 1.1 Mission area (+0x2c … +0x53, 40 bytes)

| Off | Type | Name | Meaning |
|---|---|---|---|
| +0x2c | u32 | — | never written by any mission routine; copied along in section 13; not saved. Always 0 in practice (*believed* unused). |
| +0x30 | u32 | **seed** | one draw (0 … 32767) taken when the mission is made. Re-seeds the equipment of kind-5 creatures (§3.3; `alpha-server-simulation.md` §5.2 step 5) and names the target in the objective text (§8.3). |
| +0x34 | i32 | **kind** | 0 = no mission; otherwise the kind of §1.3. |
| +0x38 | i32 | **target type** | a creature type: the type to defeat (kinds 1, 3, 4) or the creature named in the text (kinds 5, 9). |
| +0x3c | i32 | **mission level** | P_max when the mission was made. Sets the level of kind-5 creatures and of the rewards. |
| +0x40 | u8 | **tier** | the location's sub-level (record +0x28) when made; 0 for type-0 cells and for dungeon entrances (whose sub-level sits in the zone table, not the record). |
| +0x41 | u8 | **state** | 0 = not yet discovered, 1 = discovered (active), 2 = complete. |
| +0x42 | 2 bytes | — | padding. |
| +0x44 | i32 | **count** | kills counted so far (counting kinds). |
| +0x48 | i32 | **required** | kills needed (counting kinds); also set, but unused, for kind 5. |
| +0x4c | i32 | **target zone x** | the zone where the target is (−1 when none). |
| +0x50 | i32 | **target zone y** | |

### 1.2 Monster area (+0x54 … +0x67, 20 bytes)

Every **empty** cell (location type 0, §2.3) gets a roaming big monster each day, whether or
not a mission uses it.

| Off | Type | Meaning |
|---|---|---|
| +0x54 | i32 | **monster type** (0 = no monster) |
| +0x58 | i32 | monster level |
| +0x5c | u8 | monster tier (always 0 as made) |
| +0x5d | 3 bytes | padding |
| +0x60 | i32 | zone x of the monster's spawn |
| +0x64 | i32 | zone y |

### 1.3 Mission kinds

The kind decides the objective text key (§8.3), how a kill counts (§5) and which target is
chosen (§2.5). Alpha's generator makes only kinds **1, 3, 4, 5 and 9**; the code of both
programs also handles 2, 7, 8, 10, 11, 12 and 13, which nothing creates (*dead*).

| Kind | Text key (`objective:…`) | Made by Alpha for | Target type | Required | Completes |
|---|---|---|---|---|---|
| 1 | `monster` | empty cells (type 0) | the cell's monster (+0x54) | — | on the first qualifying kill |
| 2 | `rareboss` | *dead* | (wildlife picker, mode 0) | — | first kill |
| 3 | `villagemonster` | towns (type 1) | monster for a level (§2.5) | — | first kill |
| 4 | `villagerareboss` | towns | wildlife picker, mode 0 | — | first kill |
| 5 | `dungeon` | every other location type except 10 | nine-entry list | 10 … 19 (unused) | first kill |
| 7 | `invasion` | *dead* | nine-entry list | 25 … 32 | count |
| 8 | `sceneryinvasion` | *dead*, no explicit branch (would take the default branch of §2.5 step 5) | nine-entry list | 10 … 19 | count |
| 9 | `villageinvasion` | towns | nine-entry list | 10 … 19 | count |
| 10 | `caveinvasion` | *dead* | nine-entry list | 10 … 14 | count |
| 11 | `riverinvasion` | *dead* | nine-entry list | 15 … 19 | count |
| 12 | `gang` | *dead* | wildlife picker, mode 1 | 6 … 10 | count |
| 13 | `gangboss` | *dead* | wildlife picker, mode 1 | — | first kill |

Which victims qualify is the important limit (§5.1): only creatures carrying the mission-target
flag (appearance 0x2000), or bosses of a kind-5 cell. In Alpha the flag is set in exactly two
places: the cell monster of a kind-1 mission (§3.1) and a boss activated in a kind-5 cell
(§3.3; server 0x535b27). **Kinds 3, 4 and 9 therefore can never make
progress or complete** (*as read*; a scan of both binaries for writes of 0x2000 to the
appearance flags found no third place). A reimplementation that wants parity keeps this;
one that wants playable town missions must flag their targets (a design decision outside
parity).

---

## 2. The daily pass ("missions generated")

### 2.1 When it runs

Region + 0x15a18 is the **missions generated** flag (`alpha-worldgen-locations.md` §1 calls
it "stored mission data loaded"; it means both).

* **Set** by the driver at the end of its pass (§2.6), and by phase H when a stored `mission`
  record of the region was read (`alpha-worldgen-locations.md` §9). A region reloaded from the
  save in the same game day therefore keeps its saved missions and is not regenerated.
* **Cleared** for every loaded region (0x524500) when (a) the clock passes midnight
  (`alpha-server-simulation.md` §3 step 1) and (b) a player rests at an inn (server 0x53644d,
  client 0x61068d, right after the inn removes every spawned NPC; `alpha-server-simulation.md`
  §8 kind 2). Both do nothing on a remote world.
* **Tested** in the step's creature pre-pass (`alpha-server-simulation.md` §3 step 2), only
  when the world is not remote: for every player, the **owner** of the player's position is
  found (owner(x, y) of `alpha-worldgen-regions.md` §3.4, in its index form: server 0x4febd0
  called at 0x532921, client 0x5cade0 at 0x60cb61; it yields the indices (rx, ry) of the
  region cell nearest to the displaced position within the search window, or (−1, −1) when
  there is none). The region object at those indices is looked up (server 0x406210 at 0x532952,
  client 0x434a90 at 0x60cb92); if it exists and its flag is 0, the driver runs for it
  (server 0x532977, client 0x60cbb7). It is **not** the region containing the player's
  block position (block ÷ 16384): near a region border the region whose land the player
  stands in (by ownership) is the one tested. So the pass runs at most once per region per
  day (or per inn rest), when the region owns some player's position, and never for other
  regions.

The driver does nothing when the world is remote, when the world has no name, or when the
region is not loaded.

### 2.2 Players

The driver lists the players (every creature of the world's creature table with controller
kind 0, creature + 0x60, in table order) and takes **P_max**, their highest level (creature
+ 0x190). It also computes the lowest level but never uses it; the wandering groups (§3.2)
compute their own P_min and P_max. No draws.

### 2.3 Cell scan

For each of the region's 64 cells, x outer (i = 0 … 7, cx = 8·rx + i), y inner (j = 0 … 7,
cy = 8·ry + j), with L its record:

1. **Ownership test.** Take L's centre (+0x00, +0x08) in blocks and find owner(centre)
   (`alpha-worldgen-regions.md` §3.4; server 0x4feec0 returns the region object at the
   owner cell's index). If the owner is not this region's cell C, skip the cell entirely (no
   draws). This skips the slots that the locations spec §4.2 step 1 skipped (centre (0, 0))
   and those whose centre lies in another cell's land (§4.2 step 5); typed records and
   unassigned candidates pass (*believed* always, since their centres were tested the same
   way when placed; a dungeon entrance's snapped centre is not re-tested there).
2. **Reset the old mission.** If kind ≠ 0: kind := 0, count := 0, and a **section-13 record**
   of the cell (§6) is emitted with the cleared area. The state byte, target and the rest are
   left as they were.
3. **Reset the old monster.** If monster type ≠ 0: monster type, monster level and monster
   tier := 0.
4. **New monster, empty cells only.** If L's type is 0:
   * level range (lo, hi) from C exactly as `alpha-worldgen-locations.md` §3 step 6 (here the
     maximum is used): start (1, 10); warmth < 0.2 → (10, 20); moisture < 0.2 and warmth >
     0.8 → (15, 25); moisture > 0.8 and warmth > 0.8 → (10, 20); fire flag = 1 → (20, 30);
   * **draw**: monster level = lo + draw mod (hi − lo + 1); monster tier := 0;
   * monster type := the **monster for a level** (server 0x52b230) with C and that level:
     build the list
     1. troll (108), or yeti (114) when C's warmth ≤ 0.2;
     2. reptile monster (119), or mammoth (116) when warmth ≤ 0.2;
     3. if level > 10: cyclops (115);
     4. if level > 20: lich (117);
     5. if level > 50: snow golem (113) when warmth < 0.2, else ember golem (112) when the
        fire flag is 1, else golem (111); then dark troll (109);
     6. if level > 70: hell demon (110);

     **draw**: type = list[draw mod length];
   * **draw**: zone y = 8·cy + 2 + draw mod 4; **draw**: zone x = 8·cx + 2 + draw mod 4 (y is
     drawn first; the spawn is in the inner 4 × 4 zones of the cell);
   * append (cx, cy) to the **monster list M**.
5. **Location candidates.** If L's type is neither 0 nor 10, L's level (+0x24) ≤ P_max + 2,
   and (type ≠ 1, or **draw** mod 50 = 0 for a town — the draw is made only for towns, and
   only after the type and level tests have passed):
   append (cx, cy) to the **matching list A** if L's level ≥ P_max − 2, else to the **low
   list B**.

### 2.4 Choosing the missions

Two rounds for locations, then up to two monster missions. "Take a random entry" means
**draw**: index = draw mod length; the entry is removed (the order of the rest is kept).

* Location rounds (twice): **draw** r. If r mod 6 = 0 and B is not empty: take a random
  entry of B. Otherwise, if A is not empty take a random entry of A, else if B is not empty
  take a random entry of B, else nothing. A taken cell gets its mission built (§2.5).
* Monster rounds (twice, stopping early when M is empty): take a random entry of M and build
  its mission.

So a region gets at most **four missions a day**: up to two at places near the strongest
player's level (one in six rounds prefers an easier place), and up to two monster hunts.
Towns enter the lists only one time in fifty.

### 2.5 Building the mission of one cell (server 0x50c550)

Input: the cell (cx, cy). Nothing happens if the world has no name.

1. **Remove the old targets.** Every creature with the mission-target flag (appearance
   0x2000) whose home zone (creature + 0x1b0, + 0x1b4) lies in this cell is destroyed and
   removed from the world. In each loaded zone of the cell, every spawn object past the
   zone's **base count** (zone + 0xa0, the number of spawn objects the zone generator made)
   is deleted and the list is cut back to the base count. No draws.
2. Look up L. If there is none, stop. Clear kind, state, count and required (:= 0). If L's
   type is 10 (a type-10 site), stop: the cell gets no mission.
3. **Kind list** by L's type: type 1 (town) → (9, 3, 4); type 0 → (1); any other type
   (including 14, the dungeon entrance) → (5). **draw**: kind = list[draw mod length] (a
   one-entry list still costs a draw).
4. Players: P_max again (the same value as §2.2).
5. **Target**, by kind:
   * **1**: target type := monster type; target zone := the monster's zone. No draws.
   * (7 *dead*: nine-entry list draw, required = 25 + draw mod 8, target zone (−1, −1).)
   * (10, 11 *dead*: nine-entry list draw, required 10 + draw mod 5 resp. 15 + draw mod 5,
     target zone (−1, −1).)
   * **all others** (3, 4, 5, 9 in Alpha): choose the **target zone**. For each of the cell's
     64 zones (x outer, y inner) compute w = bump(L, zone centre) (centre = 256·z + 128
     blocks per axis); for the dead kinds 12 and 13, w is doubled when zx + zy is odd. Keep
     the zones with w > 0, in scan order. If none is kept, **the cell gets no mission** (kind
     stays 0, no record). Otherwise sort by w, **largest first** (the C++ library sort,
     server 0x4f6080: with at most 32 kept zones it is a straight insertion sort,
     0x4f4ab0, which moves an entry forward only past strictly smaller weights, so equal
     weights keep scan order; with more than 32 the library's introsort partitioning runs
     first and the relative order of equal weights is whatever that algorithm produces —
     *unresolved* for exact tie parity; equal weights need points symmetric about the
     location centre), and take the first zone: target zone := that zone. Then:
     * **3**: target type := monster for a level (§2.3 step 4 list) called with (cx, cy) in
       place of the region coordinates and P_max as the level. The routine accepts only
       indices 0 … 1023; cell coordinates are 4096-scale numbers in generated worlds (any
       region index ≥ 128), so it returns its fallback, **troll (108)**, without building the
       list and **without a draw** (*as read*; the call was evidently meant to get (rx, ry)).
     * **4**: target type := wildlife species picker (server 0x5290d0, mode 0) at the target
       zone's centre, with L's ground height (+0x14, truncated) as the height argument. That
       picker is the zone generator's own (not yet specified; open point M-2). Its draws
       count here.
     * (12, 13 *dead*: picker in mode 1; kind 12 also required = 6 + draw mod 5.)
     * **5, 9** (and any other kind reaching this branch, e.g. the dead kind 8): target type
       := the **nine-entry list** (server 0x52ae10): **draw**, type =
       (gnoll 48, gnobold 51, zombie 96, spectrino 80, spike 76, slime-blue 40, wizard 43,
       witch 45, insectoid 52)[draw mod 9]; then **draw**: required = 10 + draw mod 10.
6. **Finish** (every kind that got here): **draw**: seed := draw; tier := L's sub-level
   (+0x28, low byte); mission level := P_max; kind := the chosen kind. State stays 0 and
   count stays 0 (cleared in step 2). Emit a section-13 record of the cell (§6).

### 2.6 Zone pass and end of the driver

After the four rounds, for every **loaded** zone of the region (64 × 64, x outer), run the
**zone pass** (§3). Then set the region's missions-generated flag.

The zone pass is also run by the zone generator for every newly generated zone (server
0x518630 at its end, client 0x5e4850), so a zone that loads later (or again, after being
unloaded) gets its monster or group from the stored monster area.

### 2.7 Draw order of one driver call

1. Cell scan, cells in x-outer order: per empty owned cell, 4 draws (level, type, zone y,
   zone x); per town that is not too strong, 1 draw (the 1-in-50).
2. Location rounds: per round 1 draw (r), plus 1 for the index when a list is used, plus the
   cell's build (§2.5).
3. Monster rounds: per round 1 index draw plus the build (2 draws: kind, seed).
4. Zone pass for every loaded zone of the region, zones in x-outer order, each zone's draws
   complete before the next zone: for the zone holding a cell monster, the draws of §3.1 in
   the order listed there (weapon rarity, weapon generator, armour rarity, armour generator,
   appearance generator, equipment generator in mode 0); for any other zone that passes the
   tests of §3.2, the group draws of §3.2 steps 1 … 6, then the appearance generator and the
   equipment generator (mode 0) of each new spawn object in list order (leader first).

Build draws: kind 1 → 2 (kind, seed); kind 5 or 9 → 4 (kind, type, required, seed);
kind 3 → 2; kind 4 → 2 plus the picker's draws.

---

## 3. Extra spawn objects

Spawn objects past a zone's base count are "extra": made by the zone pass, removed and
remade by the next pass, never saved (the zone save stores none of them,
`alpha-saves.md` §5.2). Their ids follow the usual rule (zy·65536 + zx)·256 + index, index =
position in the zone's spawn list (`alpha-server-simulation.md` §5.3).

### 3.1 The zone pass (server 0x50d260)

For one zone (zx, zy):

1. For every extra spawn object: if a creature with its id exists, destroy it and remove it
   from the world; delete the spawn object. Cut the list back to the base count.
2. Look up the record L of the zone's cell. If L's monster type ≠ 0 and the monster's zone
   is this zone, make the **cell monster** (below) and stop. Otherwise make a **wandering
   group** (§3.2).

**Cell monster.** A spawn object with: controller kind 1; position = the zone centre
(256·z + 128 blocks per axis) at the zone's **ground height** (here and in §3.2: the sum of
the fields +0x10 and +0x1c of the zone's 32-byte column record at that block, the record
returned by server 0x406100; *believed* the column's base height plus its stored block
count, i.e. the top of the column); creature type = monster type; level = monster level;
tier = monster tier; appearance flag **0x2000 when L's mission kind is 1** (only then is it
a mission target); the default activation radius (200 blocks, set by the spawn constructor
0x4e0f40). Its inventory gets two items, in this order:

* a **weapon** (server 0x52c4e0, `alpha-items-generator.md` §2.2) of the monster's level,
  rarity r = tier + 1, + 1 when **draw** mod 20 = 0, + 1 when **draw** mod 100 = 0, at most
  4, class −1 (both rarity draws are taken before the weapon generator's own draws);
* then an **armour** piece (server 0x528bf0, §2.1 there) of the same level, rarity tier + 1,
  + 1 when **draw** mod 100 = 0, + 1 when **draw** mod 1000 = 0, at most 4, class −1 (these
  two draws follow the weapon generator's draws and precede the armour generator's).

Behaviour: a list of two behaviour objects (server constructors 0x4029e0 with 20.0, and
0x41cb90; *believed* Combat with radius 20 and a random walk, `alpha-creatures-behaviour.md`
§11). Then its appearance is generated from the type (server 0x40a840, which draws) and its
equipment generator runs in mode 0 (server 0x4fb480, as the zone generator's spawns); only
then is its id assigned and the spawn appended.

The inventory is what the creature drops (`alpha-loot-rolls.md` §3.6). A cell monster
exists every day in every owned empty cell, mission or not; only a kind-1 mission flags it.

### 3.2 Wandering groups (server 0x509e40)

Run for every zone that does not hold its cell's monster. Nothing is made unless all of:

* the zone's per-zone table kind is 0 (no town quarter, dungeon entrance or type-10 site;
  `alpha-worldgen-locations.md` §1.1);
* the zone is not the **home zone** (the zone of the world's home point, world + 0x8000f0;
  zone (32800, 32800) in every world, `alpha-worldgen-random.md`);
* no extra spawn object of the zone has a creature, i.e. no creature with the spawn's id
  exists in the world's creature table (always true right after §3.1 step 1);
* no player is within **100 blocks** (3-D distance, squared < 10 000) of the zone centre at
  the zone's ground height;
* a region cell exists near the zone centre (server 0x42e090; true in generated land).

Then:

1. P_min and P_max are taken over the players (no draw). The extra spawn objects of the zone
   are deleted again (none has a creature). **draw**: level = P_min + draw mod (P_max −
   P_min + 1).
2. **draw**: species = (elf 0, dwarf 9, orc 11, goblin 4, lizardman 7, undead 15, human 2,
   frogman 13, gnobold 51, gnoll 48, spike 76, witch 45, wizard 43)[draw mod 13].
3. The **leader**: **draw**: controller kind = 1 (hostile) if the draw is odd, else 3
   (*believed* not hostile; such creatures give no kill record, `alpha-server-simulation.md`
   §6.2 step 7); position = zone centre at ground height; type = the species, with the
   variant draw for paired species (0/1, 2/3, 4/5, 7/8, 9/10, 11/12, 13/14, 15/16: **draw**,
   type = first + draw mod 2; the five single species take no draw); level as drawn; tier 0;
   behaviours: four objects (0x4029e0 with 20.0, 0x414720, 0x4c5d50 with 10.0, 0x41cb90;
   *believed* combat radius 20, look-at-player, route walk, random walk). The route gets
   **20 points**; for each, **y is drawn first**: **draw** y = 256·zy + 16 + draw mod 224,
   then **draw** x = 256·zx + 16 + draw mod 224 (blocks); z starts at the field +0x14 of the
   column record at the **zone centre** (server 0x406100 called with the centre, not with the
   point; *believed* the column's lowest stored height) and the block at (x, y, z) is tested
   (server 0x406050), rising one block at a time, at most 100 tests (z = start … start + 99);
   z is the first block whose type (low 5 bits of the block's fourth byte) is 0 or 2
   (*believed* air or water), and start + 100 when none of the 100 qualifies. The point
   (x, y, z) is appended to the route.
4. **draw**: the leader carries draw mod 2 potions (item category 1, subtype 1, level = the
   group level).
   The leader's id is assigned just before this draw and the leader is appended to the
   zone's spawn list just after the potions are added.
5. **draw** u: group size n = trunc(4 · (u / 32767)²) (u / 32767 in single precision, the
   square through the double-precision power function with exponent 2.0, converted back to
   single precision, times 4.0, truncated): 0 for u ≤ 16383, 1 for 16384 … 23169, 2 for
   23170 … 28377, 3 for 28378 … 32766, 4 only for 32767.
6. For each member k = 1 … n: a copy of the leader's spawn (controller, position, level,
   tier) placed, then the **leader's** position is moved by (k mod 2, k ÷ 2) blocks (so the
   followers end up at offsets 0, (1, 0), (1, 1), (2, 2) from the start and the leader
   itself at (2, 4) after four; *as read*); type = the species with its own variant draw;
   behaviours (0x4029e0 with 20.0, 0x414720, 0x4055f0 with the leader's id, 0x4c5d10 sharing
   the leader's route, 0x41cb90; *believed* combat, look-at-player, follow the leader, the
   same route, random walk); **draw**: draw mod 2 potions.
7. Then, for every new spawn object in list order (leader first, then members k = 1 … n):
   its id is (re)assigned, its appearance is generated (server 0x40a840) and its equipment
   generator runs in mode 0 (server 0x4fb480) — appearance and equipment draws interleave
   per spawn.

Wandering groups are not mission targets (no 0x2000 flag), though they share species with
the nine-entry list. In practice nearly every wilderness zone of a region a player has
visited today carries a group of one to five.

### 3.3 Kind-5 creatures

A kind-5 mission adds no spawn. Its targets are the location's own creatures, made by the
dungeon and town builders with appearance flag 0x1000, and the boss among them (flags 0x1000
and 0x200, spawn + 0x10e8 = 1; `alpha-worldgen-structures.md` §1.10.6 step 9 and §5.5.9
plot kind 20). On activation (server 0x535abe–0x535b30; `alpha-server-simulation.md` §5.2
step 5), when the spawn's zone lies in a cell whose mission kind is 5 and the spawn has flag
0x1000: unless its controller kind is 6, its level := mission level and its tier := mission
tier; then **srand(seed + low 32 bits of the spawn id)** re-seeds the shared stream and the
spawn's equipment generator runs (server 0x4fb480, mode 1); a spawn with the boss flag 0x200
and spawn + 0x10e8 ≠ 0 gets flag 0x2000 (the other non-boss adjustments are in that
section). A location type whose builder places no boss
gives a kind-5 mission that cannot complete (*believed*; which location types lack a boss
is not traced here).

### 3.4 Removal

Extra spawn objects disappear (a) at the next pass for the region (§2.5 step 1 for chosen
cells, where only flagged creatures of the cell are destroyed; §3.1 step 1 for every loaded
zone, where the creature of each extra spawn is destroyed with it) and (b) when a mission
completes: every extra spawn object of the cell's 64 zones is deleted (§5.3) and their
creatures stay until they despawn or die, except leftover mission targets (§5.4).

---

## 4. Discovery

In the step's creature pre-pass (`alpha-server-simulation.md` §3 step 2; server
0x5329fd–0x532aa5, client 0x60cc3d), right after the driver test and like it only on a world
that is not remote, for every player: L = the record of the player's own cell (block ÷ 256
÷ 8 per axis, truncating; not the owner region of §2.1). If L exists, its kind ≠ 0, its
state is 0, and bump(L, player x, y) > 0 (server 0x52dee0; the player is inside the
location's area, not the target zone), then state := 1 and a section-13 record of the cell
is emitted.

For an empty cell (kind 1) the area is the unused candidate disc of the locations spec §4.2
(radius 512 … 767 blocks); for a dungeon it is the 150-block entrance circle; for a town the
town's own disc. A record whose radius is below 0.001 would count every point as inside
(f = 0); no generated record is that small.

---

## 5. Progress and completion

### 5.1 Which deaths count

On the death of a creature (server 0x4cea80 at 0x4cf44a, client 0x595a60 at 0x59642a),
**after** the static-entity trigger, the loot (server call at 0x4cf24e; its draws come
first) and the kill-experience computation, and **before** the per-creature loop that makes
the kill records and rewards (`alpha-server-simulation.md` §6.2 steps 2–7), with V the
victim (any controller kind):

* the world is not remote;
* L = the record of V's **home zone**'s cell (creature + 0x1b0, + 0x1b4, ÷ 8) exists and its
  state ≠ 2;
* V has appearance flag 0x2000, **or** L's kind is 5 and V came from a boss spawn (creature
  + 0x1d38, copied from spawn + 0x10e8 at activation).

Then V's threat map is walked in order; the **first** entry with threat > 0 whose creature
exists and is a player triggers the update below, once (other players add nothing). With no
player in the threat map nothing happens.

### 5.2 The update

| Kind | Effect |
|---|---|
| 1, 2, 3, 4, 5, 13 | state := 2 (**complete**) |
| 7, 8, 9, 10, 11, 12 | count += 1, state := 1; if count ≥ required, state := 2 (**complete**) |
| any other (0, 6, 14 …) | nothing, no record |

A section-13 record of the cell is emitted with the new state (§6), progress or completion.

### 5.3 Completion

When the update completed the mission:

* every extra spawn object of the cell's 64 loaded zones is deleted (§3.4); their creatures
  are not touched;
* in the per-creature loop of the death routine (`alpha-server-simulation.md` §6.2 step 7;
  server 0x4cf6a5–0x4cfaf8, client twin from 0x596685), which walks the world's whole
  creature table in table order, **every player creature of the world** (controller kind 0)
  receives the rewards when V is hostile (controller 1) — **whether or not the player is in
  V's threat map**: the threat-map lookup (server 0x4cf875, client 0x596855) only decides the
  kill record, and a miss jumps straight to the reward test (server 0x4cf87c → 0x4cf90d,
  client 0x59685c → 0x5968ed). On a server this is every connected player, wherever they
  are; offline it is the local player. For each such player, after its kill record (if
  any), two **item gifts** go to section 9, and when the player is the world's local player
  (world + 0xb8) each item is also put straight into its inventory:
  1. an **experience item**: type 13, level ⌊50·c(mission level + tier) + 1⌋ (truncated);
     receiving it adds 10 × its level to the player's experience (*believed*; not
     re-traced here, see `alpha-creatures-behaviour.md` U-5);
  2. **draw**: if even, an **armour** piece, if odd a **weapon** (server 0x528bf0 /
     0x52c4e0, client 0x5f51e0 / 0x5f8ad0) of the **mission level**, rarity = **tier**
     (not tier + 1), made for the receiving player's class (creature + 0x140). One draw per
     player, then the generator's own draws; players are served in creature-table order,
     each player's draws complete before the next player's.

  Note the condition order: the item rewards need V to be hostile, the completion does not.
  A completed mission whose target was not hostile (not possible with Alpha's generators)
  would complete without rewards.

### 5.4 Leftover targets

A creature with flag 0x2000, an empty threat map, whose home cell's record exists with a
mission (kind ≠ 0) that is complete (state 2), is killed silently in the next step's
pre-pass (health := 0 and creature + 0x6c, the death timer, := 0; server 0x532786–0x5327f6,
client from 0x60c9c6; `alpha-server-simulation.md` §3 step 2). This removes a kind-5 boss of
a dungeon whose mission was completed by killing a different boss, and similar leftovers.
Because the daily pass resets a completed cell's kind to 0 but keeps its state byte (§2.3
step 2), leftovers of a mission that has been reset are no longer removed by this rule. The
test is made for every creature of any controller kind and, *as read*, before the step's
remote test (server 0x5328db, client 0x60cb1b), so it is not gated by the remote flag; its
effect on a client of a server (which holds section-13 copies of the records) is not traced.

---

## 6. Network: section 13

### 6.1 Record layout (agrees with `alpha-net-packets.md` §3.9)

Read from the three server builders (0x50c550 finish, 0x5329fd discovery, 0x4cea80 kill;
packet-12 answer 0x4272d0) and the client builder (0x595a60) and reader (frame update,
client 0x48fe11–0x48fec1). 56 bytes, little-endian:

| Off | Size | Type | Field |
|---|---|---|---|
| 0 | 4 | i32 | cell x (cx) |
| 4 | 4 | i32 | cell y (cy) |
| 8 | 8 | — | zero (written as 0 by every builder; ignored by the §6.3 reader; looked up as a creature id by the banner walk for state-1 records, §8.4) |
| 16 | 4 | u32 | area +0x2c (always 0) |
| 20 | 4 | u32 | seed |
| 24 | 4 | i32 | kind |
| 28 | 4 | i32 | target type |
| 32 | 4 | i32 | mission level |
| 36 | 1 | u8 | tier |
| 37 | 1 | u8 | state |
| 38 | 2 | — | padding (left over memory from the server; zeroed by the client builder; ignored) |
| 40 | 4 | i32 | count |
| 44 | 4 | i32 | required |
| 48 | 4 | i32 | target zone x |
| 52 | 4 | i32 | target zone y |

So bytes 16 … 55 are the mission area +0x2c … +0x53 laid out exactly as in memory.

### 6.2 When records are sent

| Event | Where | Who receives it |
|---|---|---|
| a cell's old mission cleared by the daily pass | §2.3 step 2 | every client (section 13 has no filter, `alpha-net-packets.md` §3) |
| a mission made | §2.5 step 6 | every client |
| a mission discovered | §4 | every client |
| progress or completion | §5.2 | every client |
| packet 12 for a region | server 0x4272d0 | the requesting client only: all 64 cells of the region, missions or not |

### 6.3 What the client does with a record

For each record of the received section-13 list (G + 0x800690; frame update, client
0x48fe11–0x48fec1): look up the client's own record of cell (cx, cy) (client 0x487da0); if
its region is loaded, copy bytes 16 … 55 into the mission area (member by member, padding
untouched). Then, whether or not the record was found, mark the world-map region
(cx ÷ 8, cy ÷ 8) for redrawing (the world-map object's 1024 × 1024 region table, looked up
with the generic index helper 0x434a90; byte + 8 := 0). Records for unloaded regions are
dropped; the client asks again with packet 12 when it opens the map
(`alpha-net-packets.md` §4.4). The same list is copied later in the frame and walked again
for the banner (§8.4).

The monster area is **not** sent: a remote client does not know where cell monsters are and
learns about them only as creatures in packet 0.

---

## 7. Saves (answers `alpha-saves.md` V-5)

Both areas are written for all 64 cells of a region when the region is saved (client
0x5a4800 → 0x5a4af0, server 0x4d7c50), **only if** the region's missions-generated flag is set
and the world is local and named; they are read back by phase H. Values are raw little-endian.

`mission<cx>_<cy>` (38 bytes, client 0x44e120):

| Off | Type | Field |
|---|---|---|
| 0 | u32 | format version 1 |
| 4 | u32 | seed |
| 8 | i32 | kind |
| 12 | i32 | target type |
| 16 | i32 | mission level |
| 20 | u8 | tier |
| 21 | u8 | state |
| 22 | i32 | count |
| 26 | i32 | required |
| 30 | 2 × i32 | target zone x, y |

The area word +0x2c is not saved. The sample in `alpha-saves.md` §5.3 fits: seed 17 607,
target type = the cell's monster type (a kind-1 hunt), mission level 1 (a level-1 player).

`monster<cx>_<cy>` (21 bytes, client 0x44e280):

| Off | Type | Field |
|---|---|---|
| 0 | u32 | format version 1 |
| 4 | i32 | monster type (0 = none) |
| 8 | i32 | monster level (the sample's 4) |
| 12 | u8 | monster tier |
| 13 | 2 × i32 | monster zone x, y (inner 4 × 4 zones of the cell: the sample's 32 773 = 8·4096 + 5) |

Spawn objects are not saved. After a reload the zone generator's zone pass (§2.6) rebuilds
the cell monster from the monster record, flagged if the reloaded mission kind is 1.

---

## 8. Display

### 8.1 World map (answers `alpha-ui-screens.md` §5.2.8)

The map code reads exactly the fields of §1.1: **+0x34 is the kind**, **+0x41 the state**
(2 = complete, as believed there), **+0x4c, +0x50 the target zone**. Hence:

* the **mission marker** (model `mission`, slot 2560) appears for every record of the map's
  region with a mission that is not complete — undiscovered missions included; it stands
  over the monster's zone for a kind-1 hunt and over the location's centre otherwise
  (`alpha-ui-screens.md` §5.2.4);
* the **target zone's tile** is drawn undimmed while the mission is discovered (state 1)
  (§5.2.4 item 2 there);
* creatures carrying 0x2000 appear as skulls among the map's creature markers.

Object type 45 (the portal, `alpha-ui-screens.md` §5.2) is not part of the mission system;
no mission routine reads or writes static entities.

The small quest tag (§8.2) also marks the **target zone** as discovered on the world map
(0x5fc160, which bumps the map's discovered counter only the first time) every frame while
it shows a discovered mission.

### 8.2 Small quest tag and progress bar (client frame update 0x48d054–0x48d51c)

The HUD keeps one remembered mission area (a global copy, client 0x76b0d0). Every frame:

1. L = the record of the **local player's** cell. If there is none: hide the tag
   (`smallquesttag`, G+0x80086c) and forget the remembered mission. Stop.
2. v = bump(L, player x, y) (client 0x5fa4c0). For **kind 1**, v is replaced by
   (1 − min(d²/16 900, 1))², d = the horizontal distance in blocks from the player to the
   centre of the zone stored in the **monster area** (+0x60, +0x64; client 0x48d0fd,
   0x48d13c) — not the target zone (+0x4c, +0x50), although the two are equal when a kind-1
   mission is made (130-block reach). On a client of a server the monster area is never
   received (§6.3) and stays as the client's own world generator left it (zero, *believed*:
   nothing else writes it on a remote world), so d is the distance to the centre of zone
   (0, 0) and the tag would never show for a kind-1 hunt there (*believed*; to be checked
   with a capture on a server).
3. If v ≤ 0: hide the tag (only the tag; the bar is not touched here) and forget the
   remembered mission (it is reset to a freshly constructed, empty area). Stop.
4. If kind ≠ 0, state ≠ 2 and v > 0.1:
   * if state = 1, mark the target zone on the world map (§8.1);
   * if the remembered mission differs from L's in kind, target zone, seed or state:
     remember L's; and if state = 1: play the tag's `show` sequence, set its `objective`
     child to the objective text (§8.3) and play interface sound 0x2e (volume 1, pitch 1)
     at the position held at G + 0x140 (*believed* the listener / local player position);
   * show the tag;
   * if required ≠ 0 and count ≠ 0: show the bar (`questbar`, G+0x8007dc) and scale its fill
     (G+0x8007e0) to (count / required, 1, 1); otherwise hide the bar.
5. Otherwise (kind = 0, or state = 2, or v ≤ 0.1): if state = 2 or v < 0.05, hide the tag
   and the bar; else leave both as they are — for a mission with state ≠ 2 this is the
   hysteresis band 0.05 ≤ v ≤ 0.1, and for a record with kind 0 (state ≠ 2) it is every
   v ≥ 0.05 (client 0x48d3d0–0x48d3fd).

The tag appears with its text only once the mission is discovered (state 1); in state 0 it
may show with the previous text for the frames between entering the area and the discovery
record (offline these coincide).

### 8.3 Objective text (client 0x477fa0)

The text is the dictionary entry `objective:<key>` of `dict_en.xml` with the key of §1.3
(kinds 6 and ≥ 14 give no text), rendered with the markup and variables of
`alpha-talk-vendors.md` §2.2–§2.3. Three variables are set:

* the **name**: the creature-title maker 0x5a0ed0 with n = seed and kind = target type
  (`alpha-ui-screens.md` §5.4.1 item 3, `alpha-item-names.md`);
* the **creature**: the target type (0x594c80; singular, plural and case forms by the type's
  key);
* the **place**: the location L (0x5953a0, the `scenery` forms).

The `description:<key>` entries of the dictionary are never looked up for missions (the
client only builds `description:` for skills); `mission:giveup` and the class quest chains
(`objective:mage:…`, `objective:warrior:…`, `objective:killsome`, …) are unused data.

### 8.4 "QUEST COMPLETE!" banner and the objectives widget

* The banner (`questtag`, G+0x800868; client 0x490690–0x4907dc). The list walked there is a
  copy, made later in the same frame (client 0x4905b8, before the received lists are cleared
  at 0x4905c4–0x49064c), of the **received section-13 list** G + 0x800690 that §6.3 applies;
  its elements are the 56-byte records of §6.1. For each record whose **kind** (offset 24)
  is non-zero:
  * **state** (offset 37) = 1: the u64 at bytes 8 … 15 is looked up as a creature id
    (client 0x42f000) and, if such a creature exists, a speech bubble is opened for it
    (0x4882e0); mission records always carry 0 there, so in practice nothing happens;
  * state = 2: the record's first two words give a cell; if the client's location record
    of that cell exists (its region is loaded) the banner plays its `shine` sequence, its
    `objective` child is set to that cell's objective text (§8.3), and interface sound 0x1e
    (`missioncomplete.wav`) plays at G + 0x140.

  So the banner fires for **every completion record applied** — on a server's clients for
  any mission completed anywhere in a region the client has loaded (section 13 is
  broadcast), not only for the client's own kill; offline, *believed*, for the local
  completion (assuming the offline step's records reach the same list, §9). A
  packet-12 answer also carries state-2 records (all 64 cells of the region, §6.2), so,
  *believed*, opening the map after a completion the same day replays the banner (not
  checked with a capture). This answers the former open point M-1. The other use of sound
  0x1e (0x48f9be) is the mana-cube pickup, not missions.
* The **objectives widget** (`ObjectiveWidget`, G+0x800944, constructor 0x4ce180) is shown
  when a list it watches is not empty (0x49845a); no mission routine fills it, and it plays
  no part in the mission display traced here (what it shows stays open, M-6).

---

## 9. Server and client

| Part | Offline client | Server | Client of a server |
|---|---|---|---|
| Daily pass, zone pass, wandering groups | runs (client twins) | runs | never |
| Missions-generated flag reset (midnight, inn) | runs | runs | never |
| Discovery (§4) | runs | runs | never; learns it from section 13 |
| Kill progress, completion, spawn removal | runs | runs | never |
| Rewards | straight into the inventory (and a section-9 gift) | section 9 gifts to every player in the world | applies section 9 (`alpha-server-simulation.md` §6.2) |
| Section-13 records | built and applied locally | built and broadcast | applied (§6.3) |
| `mission` / `monster` saves | `world_<name>.db` | `world_server_<seed>.db` | none |
| Map markers, quest tag, banner | from its own records | — | from the copied records |
| Leftover-target silent kill (§5.4) | runs | runs | *as read* not gated by the remote flag; effect not traced |

On a remote client every mission area comes from the server, and the monster area stays
empty (§6.3). Only the server's copy makes spawns, so this does not affect spawning, but the
kind-1 quest tag measures its distance from the monster area's zone (§8.2 step 2) and,
*believed*, never shows on a remote client.

---

## 10. Worked examples

The draw values below are illustrations: they are the first values the MSVC generator gives
after srand(1) (41, 18467, 6334, 26500, 19169, 15724, 11478, 29358, 26962, 24464, 5705, …).
In the game the stream position depends on everything drawn before (§0).

### 10.1 An empty cell gets its monster

Region (512, 512), C with warmth 0.5, moisture 0.5, no fire → level range (1, 10). Cell
(4097, 4100) has a type-0 record whose centre is owned by C. One player of level 5
(P_max = 5). The scan of this cell draws:

| Draw | Use | Result |
|---|---|---|
| 41 | level = 1 + 41 mod 10 | 2 |
| 18467 | list (troll 108, reptile monster 119) since warmth > 0.2 and level ≤ 10; 18467 mod 2 = 1 | type 119 |
| 6334 | zone y = 8·4100 + 2 + 6334 mod 4 | 32 804 |
| 26500 | zone x = 8·4097 + 2 + 26500 mod 4 | 32 778 |

Monster area: type 119, level 2, tier 0, zone (32 778, 32 804); the cell joins M.

### 10.2 Choosing and building missions

Say after the scan A = [(4099, 4102)] (a dungeon entrance of level 5), B = [(4100, 4096)]
(a type-3 place of level 2), and M holds (4097, 4100) among others.

* Round 1: r = 19169, 19169 mod 6 = 5 → A: index 15724 mod 1 = 0 → cell (4099, 4102). Build:
  kind list (5), draw 11478 → kind 5; zones of the cell with bump > 0 sorted, first = the
  entrance zone (its centre is the circle's centre, w = 1); nine-entry list 29358 mod 9 = 0 →
  gnoll (48); required 10 + 26962 mod 10 = 12 (unused); seed 24464; tier 0; level 5.
  Section-13 record sent.
* Round 2: r = 5705, 5705 mod 6 = 5 → A is empty → B: index draw → cell (4100, 4096), built
  the same way with the next draws.
* Monster round 1: index draw picks (4097, 4100). Build: kind list (1), one draw → kind 1;
  target type 119, target zone (32 778, 32 804); seed = the next draw, say 11 478; level 5;
  tier 0.

### 10.3 The section-13 record of that hunt

| Bytes | Value |
|---|---|
| 0–3, 4–7 | 01 10 00 00, 04 10 00 00 (cell 4097, 4100) |
| 8–15 | 00 × 8 |
| 16–19 | 00 00 00 00 |
| 20–23 | D6 2C 00 00 (seed 11 478) |
| 24–27 | 01 00 00 00 (kind 1) |
| 28–31 | 77 00 00 00 (type 119) |
| 32–35 | 05 00 00 00 (level 5) |
| 36, 37 | 00 (tier), 00 (state: undiscovered) |
| 38–39 | padding |
| 40–47 | 00 × 8 (count 0, required 0) |
| 48–55 | 0A 80 00 00, 24 80 00 00 (zone 32 778, 32 804) |

Saved the same day: `mission4097_4100` = 01 00 00 00 · D6 2C 00 00 · 01 00 00 00 · 77 00 00 00 ·
05 00 00 00 · 00 · 00 · 00 00 00 00 · 00 00 00 00 · 0A 80 00 00 · 24 80 00 00 (38 bytes);
`monster4097_4100` = 01 00 00 00 · 77 00 00 00 · 02 00 00 00 · 00 · 0A 80 00 00 · 24 80 00 00
(21 bytes).

### 10.4 The zone pass makes the monster

Zone (32 778, 32 804) is loaded. Its extra spawns are cleared; its cell's monster lives
here, so the cell monster is made: type 119, level 2, tier 0, flag 0x2000 (kind 1), at block
(32 778·256 + 128, 32 804·256 + 128) = (8 391 296, 8 397 952). Say the next two draws are
29358 (mod 20 = 18) and 26962 (mod 100 = 62): the weapon has rarity 1 and the weapon
generator then takes its own draws; the two draws after those (say mod 100 = 64 and
mod 1000 = 705) give the armour rarity 1, then the armour generator, the appearance
generator and the equipment generator draw. If the zone already had 3 spawn objects from
the generator, the monster's id is (32 804·65 536 + 32 778)·256 + 3.

### 10.5 Discovery and the quest tag

The player walks into the type-0 record's disc: bump > 0, state 0 → 1, record sent. Offline
(or on the server's own records), the tag for kind 1 depends only on the distance d to the
centre of the monster area's zone, here the same zone (32 778, 32 804) as the target zone:

| d (blocks) | d²/16 900 | v | Tag |
|---|---|---|---|
| 100 | 0.5917 | 0.1667 | shown (v > 0.1): `show`, objective text, sound 0x2e |
| 107.5 | 0.6838 | 0.1000 | limit for showing |
| 110 | 0.7160 | 0.0807 | unchanged (hysteresis band) |
| 114.55 | 0.7764 | 0.0500 | limit for hiding |
| 120 | 0.8521 | 0.0219 | hidden |
| ≥ 130 | 1 | 0 | hidden and forgotten |

No bar: required is 0.

### 10.6 The kill

The player kills the monster (it has the player in its threat map with threat > 0). Its home
zone (32 778, 32 804) ÷ 8 = cell (4097, 4100); state 1 ≠ 2; flag 0x2000 → qualifies. Kind 1 →
state 2; record sent (state byte 2); every extra spawn object of the cell's loaded zones is
deleted.

* The loot draws of the victim come first (§5.1).
* Kill experience (`alpha-creatures-behaviour.md` §8): c(2 + 0) = 1 − 1/1.05 = 0.047619;
  20·c = 0.952 → raised to 1; type 119 is a big type → × 20 = **20**.
* Every player of the world (here the one player; on a server also every other connected
  player, wherever they are) gets the rewards; the killer also gets its kill record first.
* Experience item: c(5 + 0) = 1 − 1/1.2 = 0.166667; ⌊50·0.166667 + 1⌋ = ⌊9.33⌋ = **level 9**
  → +90 experience (*believed*, §5.3).
* One draw: even → armour, odd → weapon, level 5, rarity 0, the player's class.
* The client applying the state-2 record plays the "QUEST COMPLETE!" banner (§8.4).
* The tag: state 2 → hidden next frame.

### 10.7 The counting rule (as coded; no Alpha mission can reach it)

A kind-9 town mission with required = 10 + 26962 mod 10 = 12. If qualifying kills existed,
the first would set count 1 and state 1 and show the bar at 1/12 = 0.083 of its width; the
fourth 4/12 = 0.333; the twelfth would complete it. In Alpha no creature qualifies for kind 9
(§1.3), so the bar never appears.

---

## 11. Notes for a reimplementation

* Keep the draw order of §2.7 if missions must match a recording made on the same stream; the
  stream is shared with everything else the step draws, so in practice only a full step
  reimplementation reproduces it.
* Parity behaviours that look unintended (*as read*): kind 3 always targets trolls (§2.5 step
  5); kinds 3, 4 and 9 cannot be completed (§1.3); a kind-5 mission at a location without a
  boss cannot be completed (§3.3); the group leader drifts by up to (2, 4) blocks (§3.2 step 6);
  the reward rarity is the tier, not tier + 1 (§5.3); every player of the world is rewarded
  for a completion, not only the participants (§5.3); the daily reset of a cell keeps its old
  state byte (§2.3 step 2); the kind-1 quest tag measures from the monster area, which a
  remote client never receives (§8.2); every client shows the banner for every completion it
  applies (§8.4).
* Hooks for mods (`modding-friendly`): the kind lists of §2.5 step 3, the target lists, the
  required-count ranges, the qualifying rule of §5.1 and the reward rule of §5.3 are natural
  data-driven points.

---

## 12. Open points

* **M-1** *Answered* (§8.4): the banner list is a copy of the received section-13 list and
  the banner fires for records with kind ≠ 0 and state 2. Still to confirm with captures:
  the offline path into that list, and the replay on a packet-12 answer.
* **M-2** The wildlife species picker (server 0x5290d0, client 0x5f56c0; used by kind 4 and by
  the zone generator's wildlife): its climate and height tables and its draws. Belongs to a
  wildlife specification.
* **M-3** Which location types have a boss (spawn + 0x10e8) and therefore a completable kind-5
  mission; the setters are server 0x4e28e0, 0x500300 and 0x5104e0.
* **M-4** Controller kind 3 of half the wandering groups (§3.2): confirm it is the non-hostile
  wildlife controller.
* **M-5** The behaviour objects of §3.1 and §3.2 by name (server constructors 0x4029e0,
  0x414720, 0x4c5d50, 0x4c5d10, 0x4055f0, 0x41cb90) against `alpha-creatures-behaviour.md`
  §11.
* **M-6** The `ObjectiveWidget` (§8.4): what list drives its visibility and what it shows.
* **M-7** *Answered*: the packet-12 builder zeroes all eight bytes 8 … 15 (one 8-byte store
  of zero, server 0x427e47); the daily-pass, build, discovery and kill builders zero them as
  well. They matter only to the banner walk's state-1 branch (§8.4).
* **M-8** The meanings of the column-record fields +0x10, +0x14 and +0x1c used for the spawn
  heights of §3.1 and §3.2 (only their use is established here).
* **M-9** The order of equal weights in the target-zone sort when more than 32 zones are kept
  (§2.5 step 5).
* **M-10** The effect on a client of a server of the leftover-target rule (§5.4), which is
  not gated by the remote flag.

---

## Re-certification (2026-11-16)

Every claim of this contract was re-read against the original Alpha evidence only: the
`Server.exe` and `Cube.exe` decompile and instruction listings of the supported image, the
separate listings of the server step 0x5322d0 and client frame update 0x488ee0, and raw
bytes of the two binaries where a listing has a gap (constants and the inn call site). No
reconstruction finding was needed. Addresses are virtual addresses of the supported
images ("S" = `Server.exe`, "C" = `Cube.exe`). Claims not listed below were confirmed
unchanged, including: the location-record areas and offsets, the driver's cell order,
level ranges, monster-for-level list, the y-before-x monster-zone draws, the 1-in-50 town
draw, the location and monster rounds, the kind lists, the nine-entry and thirteen-entry
species lists, the variant draws (S 0x52bfa0), the zone-scan weights and kinds 12/13
doubling, the build's finish and its record, the zone-pass monster test, the rarity draws,
the group leader drift, the kill update table (S jump table 0x4cfc98/0x4cfca4), the
section-13 layout, the save layouts (C 0x44e120, 0x44e280; S 0x4d7c50 key order
`mission<cx>_<cy>`), the inn and midnight resets (S 0x532493; S 0x53644d → 0x524500 read
from the raw bytes, a listing gap), the quest-tag constants 16 900, 0.1 and 0.05
(C 0x702ac0, 0x6fcb7c, 0x6fcdb4), and the radius floor 0.001 of the location distance
(S 0x558858).

### Corrections

1. **§2.1 — region tested for the daily pass (reversal).** It is the region at the
   *owner* indices of the player's position (S 0x4febd0 called at 0x532921, then
   S 0x406210 at 0x532952; C 0x5cade0 at 0x60cb61, C 0x434a90 at 0x60cb92), not the region
   containing the block position (block ÷ 16384).
2. **§2.2 — P_min.** The driver computes the lowest player level but never uses it
   (S 0x50d8d0 player loop); only P_max matters there.
3. **§3.2 step 3 — route-point draw order (random-draw order change).** Each of the 20
   route points draws **y first, then x** (S 0x50b06a uses zone + 0x64, S 0x50b09c zone
   + 0x60; client twin C 0x5d6040 identical). The contract previously said x first.
4. **§3.2 step 3 — route-point height.** The upward search starts at field +0x14 of the
   column record at the **zone centre** (S 0x50b0f0 → 0x406100), tests the block at the
   route point (S 0x406050) at most 100 times, and ends at start + 100 if nothing
   qualifies.
5. **§3.1, §3.2 — ground height.** Defined as the sum of fields +0x10 and +0x1c of the
   zone-centre column record (S 0x406100; used at S 0x50a02f and in the zone pass); the
   former reading "zone object's base height fields" is replaced (field meanings M-8).
6. **§3.2 step 5 — group-size boundaries.** n = 1 for u = 16384 … 23169 and n = 2 for
   23170 … 28377 (u/32767 single precision, power with exponent 2.0 at S 0x5588d8, × 4.0 at
   S 0x558728; S 0x50b319–0x50b34a). The former "1 up to 23170" put 23170 in the wrong band.
7. **§3.2 preconditions — extra-spawn test.** It is the existence of a creature with the
   spawn's id in the creature table (S 0x509f44), not a "living" creature.
8. **§2.7 step 4, §3.1, §3.2 step 7 — draw order inside the zone pass (random-draw order
   clarification).** Cell monster: weapon rarity (2), weapon generator, armour rarity (2),
   armour generator, appearance generator S 0x40a840, equipment generator S 0x4fb480 mode
   0, then the id (draws at S 0x50d5e5, 0x50d5f8; weapon S 0x50d62a; draws 0x50d640,
   0x50d653; armour 0x50d686; appearance 0x50d7ec; equipment 0x50d7fd). Wandering group: the
   appearance and equipment
   draws run per spawn, interleaved, after all spawns are made (S 0x50b7b4, 0x50b7c9). The
   former text put "the equipment generator of each new spawn" after the whole pass.
9. **§10.4 — worked example.** The armour rarity draws follow the weapon generator's own
   draws; the former example used the two draws directly after the weapon rarity draws.
10. **§5.1 — position in the death routine.** Mission progress runs **after** the loot
    (S 0x4cf24e) and the kill-experience computation, and before the per-creature loop
    (S 0x4cf44a onwards), not "before the kill records and loot". Loot draws therefore
    precede the reward draws.
11. **§5.3 — who is rewarded (reversal).** Every player creature of the world receives the
    completion rewards when the victim is hostile, whether or not it is in the victim's
    threat map: a failed threat-map lookup jumps straight to the reward test (S 0x4cf87c →
    0x4cf90d; C 0x59685c → 0x5968ed). Players are served in creature-table order, each
    after its own kill record. The contract previously limited rewards to threat-map
    players.
12. **§5.4 — leftover-target rule.** It also requires the home cell's kind ≠ 0 (S 0x5327dc),
    sets the death timer (creature + 0x6c) to 0 as well as health, applies to any
    controller kind, and is tested before the step's remote test (S 0x532786 vs 0x5328db;
    C 0x60c9c6 vs 0x60cb1b). Added the consequence of the daily kind reset.
13. **§8.2 step 2 — kind-1 tag distance.** Measured to the centre of the **monster area's**
    zone (+0x60, +0x64; C 0x48d0fd, 0x48d13c), not the target zone; consequence for
    remote clients added (*believed*).
14. **§8.2 steps 3 and 5 — hide rules.** With v ≤ 0 only the tag is hidden (C 0x48d4f8);
    for a record of kind 0 with state ≠ 2 every v ≥ 0.05 leaves the tag unchanged
    (C 0x48d3d0–0x48d3fd), not only the 0.05 … 0.1 band.
15. **§8.2 step 4, §8.4 — sound position.** Sounds 0x2e and 0x1e are played at the
    position G + 0x140 (C 0x48d305, 0x4907bd), *believed* the listener; "at the player" was
    unproven.
16. **§8.4 — the banner (former M-1 answered).** The walked list is a copy (C 0x4905b8) of
    the received section-13 list G + 0x800690; a record with kind (offset 24) ≠ 0 and state
    (offset 37) = 2 shows the banner, state 1 tries a creature lookup with bytes 8 … 15
    (C 0x49069e–0x4906de). The former "kind 2 record on event list G + 0x800638" is
    replaced.
17. **§6.3 — map refresh.** The world-map region is marked for redrawing whether or not the
    cell's record was found (C 0x48fe5c–0x48fe89); 0x434a90 is the generic table helper
    applied to the world-map object.
18. **§1.3 — kind 8.** It has no explicit branch but would take the default branch of the
    build (nine-entry list, required 10 … 19; S 0x50c550); "no generator code at all" was
    wrong (dead either way).
19. **§2.5 step 5 — target-zone sort.** Largest first by the library sort S 0x4f6080: an
    insertion sort (S 0x4f4ab0) that keeps scan order on ties for ≤ 32 kept zones;
    introsort first for more (tie order M-9).
20. **§2.5 step 5 — kind 3.** Removed the contradictory "(one draw)"; the call makes no
    draw (S 0x52b230 returns the fallback before its list for indices > 1023).
21. **§3.3, §0 — activation of kind-5 creatures.** Stated from the evidence (S 0x535abe–
    0x535b30): controller kind 6 keeps its level and tier; srand(seed + low 32 bits of the
    spawn id) re-seeds the shared stream before the equipment generator (mode 1); the boss
    flag 0x2000 needs flag 0x200 and spawn + 0x10e8 ≠ 0. §0 now notes that this re-seed
    affects all later draws of the step.
22. **§12 M-7 answered.** The packet-12 builder zeroes all eight bytes 8 … 15 with one
    8-byte store (S 0x427e47); the §6.1 row now notes the banner walk's use of these bytes.
23. **§5.3 — experience from the reward item.** "Adds 10 × its level" is marked *believed*
    (not in the evidence traced; see the cross-spec notes).
24. **Cross-references.** `alpha-net-packets.md` §3.9 (not §3.8; its former P-7 is
    resolved there); `alpha-creatures-behaviour.md` §2, §4.6, §8, §9, §11 (the former
    §9.1, §9.2 step 6, §9.3, §12.3, §13 no longer exist or now hold other topics);
    activation now cited from `alpha-server-simulation.md` §5.2 step 5; the
    `alpha-ui-screens.md` F-9 point no longer exists there.

### Cross-spec conflicts (other specs not edited)

* **`alpha-creatures-behaviour.md` §8, "Mission rewards"**: "on completion, each such
  player" (the players present in the hostile victim's threat map) gets the rewards. This
  contract (§5.3) and `alpha-server-simulation.md` §6.2 step 7 ("whether or not the player
  is in the threat map") say every player creature of the world. Evidence: S 0x4cf84a–
  0x4cf87c (lookup miss jumps to the reward test at 0x4cf90d) and C 0x59682a–0x59685c
  (jump to 0x5968ed).
* **`alpha-creatures-behaviour.md` §8, "Mission progress (offline only)"**: the progress
  code is gated by the remote flag only (S 0x4cf3ba, C twin), so it also runs on a
  server; "offline only" is correct only in the sense "never on a client of a server".
* **`alpha-server-simulation.md` §3 step 2** says "the player's region" for the daily-pass
  test without naming the owner lookup; this is a clarification, not a contradiction: the
  region is found by owner(x, y) (correction 1).
* **`alpha-creatures-behaviour.md` U-2** lists the mission-creature adjustment at
  activation as unconfirmed; §3.3 here states it from S 0x535abe–0x535b30, in agreement
  with `alpha-server-simulation.md` §5.2 step 5.

### Remaining uncertainties

* M-2 (wildlife picker tables and draws), M-3 (which locations have a boss), M-4
  (controller kind 3 of groups), M-5 (behaviour names), M-6 (objectives widget), M-8
  (column-record field meanings), M-9 (tie order above 32 kept zones), M-10 (leftover rule
  on a client of a server).
* *Believed*, not checked by capture: the remote-client kind-1 tag never showing (§8.2),
  the offline path of completion records into the banner list and the banner replay on a
  packet-12 answer (§8.4), the sound position G + 0x140, and the reward item's experience
  amount (§5.3).
* The client twins were spot-checked (step, death routine, wandering group, tag, reader,
  banner) rather than re-traced instruction by instruction; the draw orders are taken
  from the server routines.
