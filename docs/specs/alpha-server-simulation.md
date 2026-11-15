Status: FINAL / STABLE / HELD

# Alpha multiplayer: the dedicated server's world simulation

Behavioural specification of what Cube World **Alpha**'s dedicated server simulates each
iteration: the main loop, which zones it generates, keeps and unloads, how NPCs are spawned,
run, despawned, killed and respawned, what the server does with the hits and requests clients
send, and what it puts into the outgoing world update.

Images: `Server.exe` (SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`)
and `Cube.exe` (SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`).
Both have image base 0x400000. Every address in this document is a **relative virtual
address**: "S 0x…" is an RVA in `Server.exe`, "C 0x…" an RVA in `Cube.exe`. Offsets into
in-memory objects are given for cross-checking only.

NPC behaviour itself (steering, combat AI, action effects, hit resolution, physics) is shared
code described in `alpha-creatures-behaviour.md` and `alpha-physics-camera.md` and is only
referenced here. Projectile flight, static-entity kinds and dynamic objects are only outlined
(§13).

Related: `alpha-net-server.md`, `alpha-net-packets.md`, `alpha-creatures-behaviour.md`,
`alpha-items-generator.md`, `alpha-worldgen-overview.md`, `alpha-saves.md`.

---

## 0. Conventions

* A creature's **controller byte** is at creature object +0x60. "Player" is a creature with
  controller 0, "NPC" any other creature. Controller 1 is used for hostile NPCs, 3 for
  merchants/traders (likely), 5 for pets, 6 for placed bombs and similar objects.
* Positions are signed 64-bit integers in 1/65,536 block (creature position at object
  +0x10). A **zone** is 256 × 256 blocks; its coordinates are the block coordinates divided by
  256 with rounding toward zero. A **region** is 64 × 64 zones (region = zone divided by 64,
  toward zero). A **map cell** is 8 × 8 zones (cell = zone divided by 8, toward zero).
  Distances are in blocks unless a zone distance is stated; "squared distance" is computed in
  single-precision floats from the 64-bit difference scaled by 1/65,536.
* "Draw" is one call of the C runtime `rand()` (import slot S 0x1582f4), giving 0..32,767;
  *r* = draw / 32,767. "draw mod *n*" is the C remainder of one draw. `srand` is import slot
  S 0x15832c. "dt" is the iteration's elapsed time in milliseconds.
* The **creature table** is ordered by the signed 64-bit creature id, ascending; every
  "for every creature" loop below runs in that order (negative pet/bomb ids first, then
  players 1–10, then NPCs).
* The world-update output of one iteration is a 13-list **event set** (§11).

---

## 1. One simulation, two programs

The world step is S 0x1322d0 in `Server.exe` and C 0x20c510 in `Cube.exe`. After normalising
addresses the two instruction streams are identical except for one extra stack-adjustment
instruction in the server (S 0x1325cd, after a string release in the clock section); both
contain exactly 2,109 calls. Positions inside the step correspond by adding 0xDA240 (checked,
for example, at the packet-6 dispatch S 0x136306 / C 0x210546).

What makes the server act as a server is the data the step runs on:

* Its world object (constructed by S 0xc8570) has the **online flag** (world +0xb4) clear and
  **no local player** (world +0xb8 = none).
* It passes a non-empty **input event set** to the step (received hits and "event 40"
  records). It has many players in its creature table.

Everything the step would apply to "the local player" is therefore either sent to the player
concerned (§6.1) or, where it is guarded by "is the local player", does not happen on the
server.

**The owner rule.** Action effects (a melee hit at the end of a wind-up, an ability's buff, a
dodge) are computed for a creature only when "the online flag is clear and the creature is an
NPC, or the creature is the local player" (for example S 0x13aaab, S 0x13ce3a). On the server
this means NPCs only; players' attacks reach NPCs only through received hits.

---

## 2. The main loop (S 0x149c50)

One iteration, repeated until the console stop flag is cleared (the server prints "Server
started. Enter Q to quit."):

1. *t* := `timeGetTime()`; dt := *t* − *t* of the previous iteration (the first previous value
   is taken just before the loop). dt is passed to the step whole.
2. Make an empty **input set**. Under the world lock (critical section at world +0x8000c0),
   move the queue of received **hits** into input section offset 0x00; then, under the lock
   again, move the queue of received **"event 40"** records into input offset 0x58.
3. Take the world lock and keep it for steps 4–7.
4. Make an empty **output set** and run the **step** (§3) with dt, the output set and the
   input set.
5. Move the queue of received **shoots** into output offset 0x20 (section 5), unchanged.
6. Hand the output set to the connection side (S 0x272d0; see `alpha-net-server.md` for the
   per-connection handling).
7. Under the separate player-zone-list lock, rebuild the generator's **player-zone list**: one
   entry (zx, zy) per creature whose controller byte is 0, in creature-table order.
8. Release the world lock, `Sleep(20)`, discard both sets.

### 2.1 The generation thread (S 0x149550)

A second thread loops without sleeping while the server runs. Each pass copies the
player-zone list (under its lock), then:

* **World change.** If the server object's seed or world name no longer match the world's
  (seed at world +0x800164, name at world +0x94), then under both locks every loaded zone is
  saved and removed, every region is dropped (as below), and the world is re-initialised
  (S 0xd83a0). This does not happen in normal play.
* **Generate.** The best squared zone distance starts at 9. For each player zone in list
  order, every zone (zx, zy) in the window of ± the current best around it (clamped to
  0..65,535) is examined, x outer and y inner, ascending; a zone whose squared zone distance
  is below the current best and which is **not loaded** becomes the candidate and its distance
  the new best. The result is the closest unloaded zone within squared zone distance < 9
  (the 5 × 5 block around a player zone), the first found winning ties. That one zone is
  generated with the zone generator S 0x118630 (arguments zx, zy). One zone per pass.
* **Unload**, when more than 1,000 ms have passed since the last unload pass. For every
  region object (world grid at +0xbc, 1,024 × 1,024): every loaded zone with no player zone
  at squared zone distance < 9 is removed by S 0xd79f0, which detaches it under the locks,
  **saves** it (S 0xd81b0, `zone` key; only while the online flag is clear) and deletes it.
  Then, if there are no players, or no player zone lies in a region differing by fewer than
  3 on both axes, the region is dropped by S 0xd7960: its map-cell data is saved
  (`mission`/`monster` keys, S 0xd7c50) and the region object (zone table, map cells, mission
  flag) is deleted. Finally, if no player zone lies in a region differing by fewer than 5 on
  both axes, a second per-region block (world grid at +0x4000bc) is freed by S 0xd78e0.

With no players connected nothing is generated and, within about a second, every zone and
region is saved and unloaded.

---

## 3. The step: order of work (S 0x1322d0)

Ranges are Server.exe RVAs; add 0xDA240 for Cube.exe.

1. **Clock** (S 0x132330–0x1325fd). If any player has action 84 (resting) and health > 0, the
   time of day (world +0x80015c) advances **100 × dt**, otherwise **10 × dt** (one game day =
   86,400,000 game ms = 8,640 s of real time at ×10). If that player is the local player it
   also regenerates 0.05 × maximum health per second (never on the server). A world whose
   name is empty has its time forced to 9:00 (32,400,000). While the time exceeds 86,400,000:
   day (world +0x800160) + 1, time − 86,400,000, and (online flag clear) every loaded region's
   "missions generated" flag (region +0x15a18) is cleared (S 0x124500). A running total of
   simulated ms (world +0x8000bc) is increased by dt; when the new total and the total plus dt
   fall in different 10,000-ms intervals, the world store handle (world +0xac) is present and
   the world is named, the key `time` (day, time) is written.
2. **Creature pre-pass** (S 0x1326a1–0x132ac5), for every creature:
   * controller 3: if its inventory (creature +0x11dc) is empty and its home spawn exists, the
     inventory is replaced by the spawn object's inventory (spawn +0xf6c);
   * a creature with appearance flag 0x2000 (creature +0x7e), an empty threat map (+0x13a4),
     and whose **home zone's** map cell has a mission (cell +0x34 ≠ 0) that is complete
     (cell +0x41 = 2) gets health 0 and death timer 0 (silently killed);
   * every player is appended to the step's **player list**, and each loaded zone of the 3 × 3
     around its current zone is inserted into the **active zone set**. Then, online flag
     clear: if the player's region has its "missions generated" flag clear, the region's
     missions are generated (S 0x10d8d0, given the output set); and if the map cell of the
     player's current zone has a mission (cell +0x34 ≠ 0) not yet discovered (cell +0x41 = 0)
     and the player is inside its area (S 0x12dee0 returns > 0), the cell becomes discovered
     (+0x41 = 1) and a **section-13 record** for it is output (§6.2 step 7 gives the layout).
   The active zone set is ordered by the zones' in-memory addresses; every "for every active
   zone" loop below follows that order.
3. **Incoming hits** (S 0x132adf–0x132b92): every input hit is applied with the hit routine
   (S 0xcea80, §6.1).
4. **Incoming "event 40"** (S 0x132b92–0x132c6a): every input record is 40 bytes — u64 source
   id, u64 **target** creature id, 24-byte buff. If the target exists the buff is added to its
   buff list (S 0x11740): for buff kinds 1, 2, 3, 6, 7, 9, 10 and 11 an existing entry of the
   same kind is overwritten, otherwise the buff is appended. Nothing is copied to the output.
5. **Active zones: world items** (S 0x132c6a–0x133097): every item lying in an active zone
   counts down its timers at item +0x140 and +0x13c by dt (each only while positive, floor 0).
   When +0x13c reaches 0 and the online flag is clear, a **landing sound** goes to section 4 at
   the item position: sound 58 for coins (item type 12), 59 otherwise, pitch 1 + 0.1*r*
   (one draw), volume 1. Items of type 16 set their angle (+0x130) to 180 or 0 according to
   solid blocks beside them.
6. **Active zones: static entities** (S 0x133097–0x133bab): traps, doors and similar objects
   (§13); state changes go to section 6.
7. **Spawn timers** (S 0x133c7b–0x133dc3; skipped when the online flag is set): for every
   spawn object of every active zone, each entry of its schedule list (spawn +0x10a0,
   32-byte entries) whose time threshold (entry +0x18) is ≤ the time of day overwrites the
   spawn's 24-byte active block (spawn +0x10), in list order; then the respawn countdown
   (spawn +0x38) is decreased by dt (no clamp here).
8. **Despawn** (S 0x133dc3–0x1341d9; skipped online; §5.4).
9. **Player pets** (S 0x1341d9–0x1347c7; skipped online or when the world name is empty;
   §5.6).
10. **Dead creatures** (S 0x1347c7–0x135702; skipped online; §6.3).
11. **Active zones: dynamic objects and spawn activation** (S 0x135702–0x136259; skipped
    online or when the world name is empty; §5.2).
12. **Per creature** (S 0x136259–0x145c21), for every creature, players included: the queued
    **interaction requests** (packet 6, §8) are handled and (online flag clear) the queue is
    emptied; health is clamped to the maximum; nothing more if health ≤ 0. For NPCs with the
    online flag clear, an AI accumulator (+0x13c4) grows by dt; when the creature is not
    hit-stunned (+0x12c ≤ 0) and the accumulator is ≥ 32 ms, the creature's behaviour object
    (+0x13e4) runs with the accumulated time and the accumulator resets
    (`alpha-creatures-behaviour.md` §13). A bomb (type 144) with the online flag clear loses
    0.025 × dt health (floor 0). Steering, action clock, action effects (owner rule), stamina,
    mana, stealth, buffs, physics and collision follow (`alpha-physics-camera.md`). Hits
    produced here go through the hit routine.
13. **Dynamic objects and projectiles** (S 0x145c21–0x146c40; §13).
14. **Target clean-up and mounted pets** (S 0x146c40–0x146efc): every creature whose target
    (+0x11d0) names a creature with health ≤ 0 forgets the target; a pet whose owner has
    action 106 is re-placed relative to the owner (not further traced).
15. **Wrap-up** (S 0x146efc–0x147222): every zone in the step's changed-item-zone set gets
    its item-save flag (zone +0x75) set and its whole item list goes into **section 7**; every
    zone named by a section-6 record gets its flag at zone +0x76 set; every zone whose
    16-byte object list changed sends that whole list in **section 8**; every section-1 block
    edit of the step is appended to its zone's stored edit list (zone +0x68).

---

## 4. Zones on the server

| Set | Extent around each player's zone | Used for |
|---|---|---|
| generated / loaded | 5 × 5 zones (squared zone distance < 9) | everything |
| **active** | 3 × 3 zones (loaded ones only) | spawn timers and activation, world items, static entities, dynamic-object creation |
| saved and unloaded | outside every player's 5 × 5 | checked about once a second (§2.1) |

**Example.** A player at block (8,398,402, 8,397,472) is in zone (32806, 32802). Loaded:
zones 32804–32808 × 32800–32804 (25 zones, 1,280 × 1,280 blocks). Active: 32805–32807 ×
32801–32803.

---

## 5. NPCs on the server

### 5.1 Where NPCs come from

NPCs come from **spawn objects** in each zone's spawn list (zone +0x18) created by the zone
generator and by mission generation. Other server-created creatures are player pets (§5.6)
and placed bombs (§8 kind 1). No other spawning path was found in the step.

### 5.2 Activation (S 0x135908–0x1361f3)

For every active zone (in active-set order), first each entry of the zone's dynamic-object
list (zone +0x24) that is not yet in the world's dynamic-object table (world +0xc) is turned
into a 160-byte world dynamic object when some player is at squared distance < 262,144
(512 blocks). Then, for each spawn object of the zone in list order:

1. if its time window is non-empty (spawn +0x40 ≠ +0x44), skip unless start ≤ time of day ≤
   end;
2. skip if a creature with the spawn's id (spawn +0x48) exists;
3. if the countdown (+0x38) is negative set it to 0; skip if it is not 0 **and** the death
   day (+0x3c) equals today;
4. skip unless some player (player-list order; first match) is at squared distance below
   radius² (radius at spawn +0x08);
5. **mission adjustment**: if the zone's map cell has mission kind 5 (cell +0x34) and the
   spawn has flag 0x1000 (spawn +0x7a): unless the spawn's controller (+0x28) is 6, the spawn
   level (+0x34) becomes the cell's level (cell +0x3c) and spawn byte +0x58 the cell's byte
   +0x40. Then **`srand` is called** with cell +0x30 plus the low 32 bits of the spawn id,
   and the spawn's equipment is regenerated (S 0xfb480). If the spawn has the boss flag
   0x200: flag 0x2000 is added when spawn byte +0x10e8 is non-zero. Otherwise the levels of
   nine equipment items (spawn +0x8c8, +0x7b0, +0x698, +0x350, +0x580, +0x468, +0x238, +0x9e0,
   +0xaf8, in that order) are re-rolled downward with spread 0.05 (S 0x14470; one draw per
   item whose level is above 1 and stays above the floor). Finally every inventory item of the
   spawn of an eligible type gets the cell's level, and for item types 3–9 rarity = cell byte
   +0x40 + 1, + 1 on draw mod 20 = 0, + 1 on draw mod 100 = 0, at most 4. These changes stay
   in the spawn object;
6. **create** the creature (object size 0x1e60) with the spawn's id and copy from the spawn:
   position, the orientation value at creature +0x30 (from spawn +0x54), level, controller, type, class (+0x140), specialization (+0x141), tier
   byte (+0x1a8), appearance, equipment, stat multipliers (creature +0x178..+0x18b), name,
   schedule list; its **home position** (+0x1c0) = the spawn position; its **home zone and
   spawn index** (creature +0x1b0: zx, zy, index); its **despawn radius** (+0x1d2c) = the
   spawn radius; its current schedule index (+0x1484, +0x1488) = the last schedule entry whose
   threshold ≤ the time of day; its behaviour object (cloned from the spawn's if present,
   otherwise a default set unless controller 6; `alpha-creatures-behaviour.md` §13.1); its
   inventory from the spawn; health = maximum. A boss doubles its appearance scale; the
   creature's height is then raised by 0.5 × its appearance size (z) + 0.1 blocks (conversion
   not fully traced).

### 5.3 Creature ids

| Creatures | Id |
|---|---|
| players | 1–10 (`alpha-net-server.md`) |
| NPCs from spawn objects | **(zy · 65,536 + zx) · 256 + i** as a signed 64-bit value, where (zx, zy) is the spawn's zone and *i* its index (S 0xf3850; called from S 0x118630, S 0xe28e0 and S 0x100300) |
| pets and placed bombs | **one less than the smallest id in use, where the scan starts from 0**: −1, −2, … |

Ids are unique while a zone has fewer than 256 spawns; zone x = 0 with zone y = 0 would
overlap player ids.

**Example.** Spawn index 3 of zone (zx 32806, zy 32800): (32800 · 65536 + 32806) · 256 + 3 =
550,301,083,139 = 0x80_2080_2603; little-endian `03 26 80 20 80 00 00 00`.

### 5.4 Despawn (S 0x133e10–0x1341d9)

For every NPC with health > 0 that is not a pet (controller ≠ 0, ≠ 5):

* **Kept** if its threat map holds an existing player with threat > 0 and the NPC's squared
  distance from its home position is < 262,144 (512 blocks).
* Otherwise let *m* = the smallest squared distance to any player (−1 with no players). If
  *m* > (despawn radius + 20)² — 220² = 48,400 for radius 200 — the NPC is listed for removal.

Listed NPCs are taken out of every creature's threat map and deleted. Despawning does not
start the respawn countdown.

### 5.5 What clients see

The step produces no creature messages; clients learn about NPCs through the connection
side's creature batches (`alpha-net-server.md`). Activation requires squared distance below
radius², despawn needs more than (radius + 20)².

### 5.6 Player pets (S 0x1341d9–0x1347c7)

Only when the world is named. For every player in the player list:

* If the item at creature +0x1020 (equipment slot 12 by the 0x118-byte slot stride from
  +0x300) is a pet item (type 19): if the stored pet id (+0x11c8) is zero or names no
  creature, it is cleared and replaced by the id of any controller-5 creature whose owner
  (+0x198) is this player. If the pet exists and the item changed (a change counter at
  +0x1138 differs from the copy at +0x1d44, or the item differs from the copy at +0x1d48),
  the pet's type, level, experience, name, owner and appearance are refreshed from the item
  (type = item subtype, level = item +0x10, experience = item +0x04, name = item sub-block
  bytes). The copies are then updated. If nothing was refreshed, no pet id is stored and the
  player's byte at +0x124 lacks bit 0x10, a pet is **created**: id per §5.3, owner = the
  player, controller 5, type/level/experience/name from the item, position = the player's,
  health = maximum.
* Otherwise, if the stored pet exists and has a buff of kind 7, nothing happens; else the
  stored id is cleared and the pet (if it exists) gets health 0.

---

## 6. Hits, deaths, loot and rewards

### 6.1 The hit routine (S 0xcea80)

Hit record fields used: +0x00 attacker id, +0x08 target id, +0x10 amount (f32), +0x18
stun (i32), +0x1c second timer (i32), +0x38 knock-back (3 × f32), +0x45 kind. The third
argument is the step's changed-item-zone set.

1. Ignore the hit if the target does not exist or its health ≤ 0.
2. If there is no local player (the server) and the target is a player: append the hit
   unchanged to **section 2** and stop.
3. Otherwise (the attacker may be missing):
   * amount > 0: the target's stealth (+0x1190) is reduced (floor 0) and a resting target
     (action 84) gets action 0 and stun (+0x12c) 3,000 ms;
   * amount ≥ 0 and the attacker exists: attacker and target store each other as target
     (+0x11d0); the attacker's per-target hit counter map (+0x13ac) entry for the target
     grows by 1; the attacker's entry in the target's threat map grows by amount × *m* + 0.01,
     *m* = 1 when the attacker test S 0xf5a0 holds, else 10 for attacker type 25, else 20 for
     a slot-7 shield user (S 0xf8b0) of specialization 1, else 2; then S 0xd5f40 runs with
     attacker, target and output;
   * a target with appearance flag 0x2000 continues only if its threat map holds an existing
     player with threat > 0; otherwise the hit ends here;
   * kind 3: nothing further;
   * amount ≥ 0: knock-back is scaled by 0.1 when the target is stunned and the hit's stun
     ≤ 0; it replaces the target velocity (+0x4c) when the target's controller is not 6, the
     target is an NPC (online flag clear) or the local player, and the current speed² ≤ the
     knock-back length²; a hit with stun > 0 also zeroes the vector at target +0x34;
   * the hit's +0x1c raises target +0x130 to at least that value;
   * kind 1: block gauge (+0x170) + 0.25, at most 1.0. Kind 4: special effect S 0x8230.
     Any other kind: the hit's stun raises +0x12c to at least that value, then (online flag
     clear or target is the local player):
     - amount < 0 (any of these kinds): heal by −amount, clamped to maximum; with the online
       flag clear, every creature *c* for which S 0xcfcc0(healer, *c*) holds and whose
       threat map already lists the healed creature gets the healer's threat increased by
       0.1 × (−amount), followed by S 0xd5f40;
     - amount > 0, kind 5: every buff of kind 6 on the target is reduced by the amount (both
       value words zeroed at ≤ 0) and a 40-byte record (attacker id, target id, buff) goes to
       **section 12**;
     - amount > 0, other kinds: health − amount, only if the damage test S 0x9620 holds for
       the target and for the attacker when present (controllers 0, 1, 3, 5, or 6 except
       types 140–142);
     - if health is now ≤ 0, the death of §6.2 follows.

Hits on NPCs are applied on the server and **not relayed**.

### 6.2 Death (same routine, continued)

1. Health := 0; **sound 27** (pitch 1, volume 1) at the victim to section 4; death timer
   (+0x6c) := 0.
2. In the victim's home zone, the first static entity (392-byte entries, zone +0x0c) of kind 5
   linked to the victim's spawn index (entity +0x178) and armed (entity +0x30 ≠ 0) is
   triggered (S 0xd8c90) and its record goes to **section 6**.
3. **Loot** (S 0xd2ae0, §6.4).
4. The victim's pet, if it exists, gets health 0.
5. **Kill experience** *e* = max(1, 20 · *c*(tier + level)), then × 20 for the types of
   S 0xf650 (107–109, 114–119, 101), × 10 with the boss flag 0x200, 0 with flag 0x800;
   *c*(*x*) = 1 − 1/((*x* − 1) · 0.05 + 1) (S 0x7d60).
6. **Missions** (online flag clear): if the map cell of the victim's home zone exists, its
   mission is not complete (+0x41 ≠ 2), and the victim has flag 0x2000 or (mission kind 5 and
   victim byte +0x1d38 ≠ 0): the first threat-map entry with threat > 0 that is an existing
   player decides: mission kinds 1–5 and 13 complete (+0x41 = 2); kinds 7–12 increase the
   count (+0x44) and set +0x41 = 1, completing when the count reaches the target (+0x48);
   other kinds change nothing. On progress or completion a **section-13 record** goes out:
   56 bytes = cell x, cell y (i32 each), 8 further bytes, then a copy of the cell's 40-byte
   state (cell +0x2c). On completion the extra spawn objects (those from each zone's base
   count at zone +0xa0 onward) are deleted from the 64 zones of the cell.
7. For **every creature** in table order: if its target is the victim or the victim's pet it
   is cleared; the victim's and the pet's ids are removed from its threat map. If the victim
   is a hostile NPC (controller 1) and this creature is a player:
   * if the victim's threat map contains this player: a **kill record** goes to
     **section 10** (u64 player id, u64 victim id, i32 victim type, i32 ⌊*e*⌋; 24 bytes) and
     S 0xd61c0 adds *e* to that player's living pet and to the pet item it tracks;
   * if this kill completed the mission: **two item gifts** go to **section 9**
     (u64 player id + 280-byte item) — whether or not the player is in the threat map: an
     experience item (type 13, level ⌊50 · *c*(mission level + mission rarity) + 1⌋), then
     a reward from S 0x128bf0 on an even draw or S 0x12c4e0 on an odd draw (mission level,
     mission rarity, the player's class).
8. If the victim's home zone and spawn index name a spawn: countdown := **1,200,000 ms**,
   death day := today.

### 6.3 After death (step 10, S 0x1347c7–0x135702)

* A pet whose owner exists but no longer stores it as its pet is reset: health 0,
  controller 3, death timer 5,000, owner cleared.
* For every non-player with health ≤ 0: a bomb (type 144) whose death timer is still 0
  explodes first: a particle record to section 3; every block within squared distance < 64
  (8 blocks) is removed (section 1); if any block was removed, sound 1 with pitch
  0.5 + 0.4*r* (one draw) goes to section 4; creatures around it are hit through hit
  resolution. Then the death timer grows by dt.
* A dead pet whose owner exists with health ≥ 0 comes back once the timer reaches 20,000 ms:
  full health, owner's position, target cleared, its threat map and the owner's hit-counter
  map cleared.
* Any other dead non-player with timer ≥ 1,000 ms is removed from every threat map and
  hit-counter map and deleted.
* Every world dynamic object with no player at squared distance < 262,144 is destroyed and
  removed from the table.

**Respawn timeline (example).** A wolf spawn in an active zone, radius 200. A player walks to
150 blocks: the wolf is created on the next step. The player kills it at 10:00 game time on
day 7; 1,000 ms later the creature is deleted. The spawn has countdown 1,200,000 and death
day 7. It can come back when its countdown reaches 0 (counted only while its zone is
active; 20 real minutes) or when the day counter becomes 8 (14 h of game time = 84 real
minutes at ×10). If the player walks away beyond 220 blocks without holding positive threat
on it, the wolf is removed and recreated unharmed on return.

### 6.4 NPC loot (S 0xd2ae0)

Nothing drops for players, controller 3 or appearance flag 0x800. Otherwise, in this order
(one draw is spent first). Every drop below is preceded by two offset draws (x = 1 − 2*r*,
y = 1 − 2*r*′, z = 0 blocks, added to the victim position) and one angle-argument draw, in
that order; the angle argument is ignored by the drop routine.

1. **Equipment**: the non-empty items at creature +0x990, +0xaa8, +0x530, +0x878, +0x760,
   +0x648, +0x418 (slots 6, 7, 2, 5, 4, 3, 1) form a list. If it is not empty and draw mod 4
   = 0 or the boss flag is set: offsets, then an index draw (mod list length), then the
   angle draw; scale 1.0.
2. Hostile NPCs (controller 1) and type 143, on draw mod 10 = 0: level = max(1, level − 1 +
   parity draw); then a parity draw picks S 0x128bf0 (even) or S 0x12c4e0 (odd), each with the
   rarity roll S 0x12bf40(tier) and no class; level re-rolled upward with spread 0.05; scale
   1.0. Then on draw mod 5 = 0 a **pet-food** item: a rarity roll whose result is discarded
   (its draws are still spent), S 0x12b3f0 (type 20, subtype one draw from the world's list
   at world +0x88, level 1), re-roll (no draw at level 1), scale 1.0.
3. **Type drops**: a count draw (1 + draw mod 3) is always spent; for types without a drop
   the section ends. Item type 11 (level 1) by creature type — 58 → subtype 2 material 19;
   70 → subtype 18; 71 → 21; 120, 121, 124, 126 → one parity draw per item: subtype 5 material
   21 (even) or subtype 1 material 2 (odd); 122 → subtype 27; 123 → 11 material 27;
   125 → 6; 127 → 20; 128 → 23; 139 → 8 material 22; 131 → subtype 0 material 11; 132 →
   material 1; 133 → 12; 134 → 17; 135 → material 13 rarity 1; 136 → 14 rarity 2; 137 → 15
   rarity 3; 138 → 16 rarity 4. Scale 3.0 for 131–133 and 135–138, 0.8 for the others.
4. Hostile NPCs and type 143 only:
   * draw mod 50 = 0: offsets and angle draws, then the rarity roll with tier + 1, then a
     **recipe** of the victim's level (S 0x12a760); scale 0.75;
   * without appearance flags 0x18: **coins** worth ⌊(1 + 2*r*) · 10 · *V*⌋ (× 10 for bosses,
     applied before truncation), *V* = 2^(3·*c*(level)) · 2^(tier/4) (S 0x10f00); copper =
     value mod 100, silver = ⌊value/100⌋ mod 100, gold = ⌊value/10,000⌋; one item each when
     non-zero, in the order copper, silver, gold (type 12, materials 10, 12, 11; amount in
     item +0x10); scale 0.75;
   * with flags 0x18 (animals): rarity from draw mod 1,000 + 20 × tier: 0 below 700, 1 below
     950, 2 up to 998, 3 above; + 1 for bosses, at most 3. Then draw mod 20; when it is 0 and
     the rarity is not 0, an item of type 14 gets a seed draw, the victim's level and that
     rarity; scale 0.75.
5. Hostile NPCs: every inventory stack except healing potions (type 1 subtype 1) drops one
   item per unit, scale 0.75.

The rarity roll S 0x12bf40(*t*) is: draw mod (*t* + 1), + 1 on draw mod 100 = 0, + 1 on draw
mod 1,000 = 0, + 1 on draw mod 10,000 = 0, at most 4 (four draws).

**The drop routine (S 0xd2810)** takes an item, a position, an ignored angle argument, a scale
factor and the changed-item-zone set. It draws the landing delay (500 + draw mod 300 ms, item
+0x13c) and records today (item +0x144). If the zone under the position is not loaded,
nothing more happens. Otherwise it copies the item, draws the angle (360*r* degrees, item
+0x130), multiplies the default scale (item +0x134) by the factor, lowers the position to the
first block below whose type is neither 0 nor 2 and places the item one block above it,
appends it to the zone's item list and adds the zone to the changed set (sent in section 7 in
the wrap-up).

**Example.** A level-5 tier-0 hostile goblin with an equipped shield dies. First draw spent.
Draw mod 4 = 0 → offsets, index and angle draws; the shield drops. Draw mod 10 ≠ 0. The count
draw is spent; the type has no drop. Draw mod 50 ≠ 0. Coins: *c*(5) = 1 − 1/1.2 = 0.1667,
*V* = 2^0.5 = 1.414; with *r* = 0.5 the value is ⌊2 · 10 · 1.414⌋ = 28 → one item of 28 copper.
A player in its threat map gets a kill record of ⌊20 · 0.1667⌋ = 3.

---

## 7. Received hits, "event 40" records and shoots

* **Hits** are queued and applied by the hit routine in step 3 — on an NPC they change the
  NPC on the server; on a player they are appended to section 2. They are not echoed
  otherwise.
* **"Event 40" records** are queued and applied to the target creature's buff list in step 4.
  They are not relayed. Section 12 carries only records made by the server's own step (for
  example the kind-5 absorb updates of §6.1).
* **Shoots** are not given to the step; the main loop appends them unchanged to section 5.

---

## 8. Packet 6: interaction requests

Each received interaction request is stored in the sender's creature queue (+0x130c) and is
handled at the start of that creature's turn in step 12, dispatched on the kind byte through
the table at S 0x1489e0. Record fields used:

| Off | Size | Meaning |
|---|---|---|
| 0x000 | 280 | an item |
| 0x118 | 12 | target: 3 × i32 (zone x, zone y, index) for kinds 2, 3 and 5 |
| 0x124 | 4 | *unknown* |
| 0x128 | 1 | **kind**, 1–8 |

| Kind | Handler | Meaning on the server (all except kind 3's first action require the online flag clear) |
|---|---|---|
| 1 | S 0x136e64 | **use an item**: item type 1 subtype 7 places a **bomb** (id per §5.3, type 144, controller 6, level = item level, at the player's position). Any other item: the server's copy of the player's health rises by the healing value (S 0x13be0); sound 44 (pitch 1) at the player to section 4; a hit record (attacker id 0, target = the player, amount = −heal, kind 0, position = the player) to section 2 |
| 2 | S 0x13630d | **talk**: the creatures are searched in table order for one whose home (zone x, zone y, index) equals the target. If it has class 132 and the time is ≥ 18:00 or ≤ 6:00 (**inn**): day + 1, time := 7:00, sound 29 (volume 0.75) at the player to section 4, every NPC that is not a pet and whose home spawn is loaded is deleted with its spawn countdown set to 0 (S 0xd7ae0), and the "missions generated" flags are cleared (S 0x124500). If a matching creature has appearance flag 0x80 the search ends with no effect; otherwise it continues |
| 3 | S 0x13670a | **use a static entity** (target = zone, entity index): a player with action 79 first gets action 0. Entity kind 45 acts only for a local player. Kinds 16, 18, 19, 68 and 69 (seats and similar) bind a free entity to the player when the player's action is 0, 83 or 84 and move the player onto it; a chest (kind 10) in state 2 drops its contents as world items; kinds 1, 2, 3, 9 and 10 in state 0 are triggered (S 0xd8c90); the new states go to section 6 (*outlined*) |
| 4 | — | nothing |
| 5 | S 0x136506 | **pick up** (target = zone, item index): if the item exists, the player may take it (S 0x9660) and the squared distance is ≤ 16, the zone is added to the changed set, a **section-9 gift** (player id + item) is output, sound 45 with pitch 1 + 0.1*r* (one draw) goes to section 4, and the item is removed from the zone |
| 6 | S 0x13778c | **drop**: the request's item goes through the drop routine at the player's position with scale factor 1.0 (the angle is drawn as usual); sound 57 at the player to section 4 |
| 7 | S 0x1370b7 | **place a voxel object**: object number = the player's byte at creature +0x18d (S 0xd1860); placed in front of the player; its blocks go to section 1 and the zone's object list is updated (*partly traced*) |
| 8 | S 0x136459 | **reset the pet**: the player's pet forgets its threats and, if farther than 200 blocks (squared > 40,000), is moved to the player; the player's hit-counter map (+0x13ac) and target (+0x11d0) are cleared |

---

## 9. Section 9, 10 and 11 records

* **Section 9** (288 bytes): u64 player id, 280-byte item — "give this item to this player":
  pickups and mission rewards. The step adds such items directly to the inventory only for a
  local player (S 0x27000); on the server they are only output.
* **Section 10** (24 bytes): the kill record of §6.2 step 7.
* **Section 11**: serialised with the same 24-byte writer as section 10; no producer was found
  in the step.

---

## 10. Online and offline behaviour of the shared step

| | Online flag clear (server, offline client) | Online flag set |
|---|---|---|
| spawn timers, despawn, pets, dead creatures, spawn activation (§3 steps 7–11) | run (step 9 and 11 also need a named world) | skipped; only the pets' owner links are copied back to the owners (+0x11c8, S 0x133bbf) |
| NPC AI | runs for NPCs | skipped |
| action effects | owner rule: NPCs and the local player | owner rule: the local player |
| hit routine | threat, damage, heal, knock-back, death, loot, rewards | threat bookkeeping for NPC targets; damage, knock-back and death only for the local player |
| missions, map-cell discovery, landing sounds | run | skipped |
| packet-6 requests | handled, queue emptied | kinds other than 3's first action skipped; queue kept |

Hit resolution (S 0xcfd50) is skipped against a target with health ≤ 0 and, with the online
flag set, against the local player.

---

## 11. The event set

| Offset in the set | Section | Records |
|---|---|---|
| 0x00 | 2 | hits on players (§6.1) |
| 0x08 | 4 | sounds (24 bytes: position, id, pitch, volume) |
| 0x10 | 3 | **particle bursts** (72 bytes) |
| 0x18 | 1 | block edits |
| 0x20 | 5 | shoots |
| 0x28 | 7 | zone item lists |
| 0x30 | 6 | static-entity states |
| 0x38 | 8 | zone 16-byte object lists |
| 0x40 | 9 | item gifts |
| 0x48 | 10 | kill records |
| 0x50 | 11 | (no producer found) |
| 0x58 | 12 | "event 40" buffs (40 bytes) |
| 0x60 | 13 | map-cell states (56 bytes) |

The serialiser S 0x23dd0 writes its (per-connection) copy of the set in an order that, read
against this layout, is offsets 0x18, 0x00, 0x10, 0x08, 0x20, 0x30, 0x28, 0x38, 0x40, 0x48,
0x50, 0x58, 0x60 — sections 1–13; sections 4, 10 and 11 share one 24-byte writer.

**Particle bursts.** Written by the spirit bursts (S 0xd2190, called from hit resolution
S 0xcfd50) and by the bomb explosion. Layout built by S 0xd2190: 0x00 position (3 × i64),
0x18 velocity (3 × f32), 0x24 colour (4 × f32), 0x34 f32 0.1, 0x38 i32 count (⌊3·*s* + 1⌋,
+ 10 in the doubled variant), 0x3c i32 2, 0x40 f32 3.0, 0x44 *unknown*.

---

## 12. Worked example: one server iteration near a fight

Two players (ids 1 and 2) stand 30 blocks apart in zone (32806, 32802); a level-5 goblin (id
550,301,083,139) attacks player 1, who hits back.

1. The receive side has queued from client 1 one hit (attacker 1 → target goblin, amount
   12.0, kind 0).
2. Step: clock + 10·dt. Pre-pass: both players listed; active zones are the 3 × 3 around
   (32806, 32802). The queued hit is applied: goblin health 30 → 18, player 1's threat on it
   grows by 12 × 2 + 0.01 = 24.01 (with *m* = 2). Nothing is relayed.
3. Per creature: the goblin's AI keeps player 1 as target; its swing completes this step and
   hit resolution makes a kind-0 hit (goblin → 1) and an impact sound; the hit routine sees a
   player target with no local player and appends the hit to section 2.
4. Output: section 2 = 1 hit, section 4 = 1 sound; the connection side forwards them and the
   updated goblin record.

When the goblin's health reaches 0 the same iteration also carries sound 27, any loot drops
(section 7 for the zone in the wrap-up) and the kill record (1, goblin id, type, 3) in
section 10; 1,000 ms later the goblin is deleted.

---

## 13. Outlined only

* **Static entities** (zone list of 392-byte entities, step 6): traps of kinds 6, 7 and 8
  are re-armed after 2,000 ms and hit creatures through hit resolution; doors and similar
  objects react to packet-6 kind 3 and to players; states go to section 6. Per-kind rules are
  not specified.
* **Dynamic objects** (world +0xc, 160-byte objects created from active zones' lists within
  512 blocks of a player and destroyed when no player is within 512 blocks): a three-state
  motion routine runs on each in step 13.
* **Projectiles** (world +0x14; kind at +0x60, age at +0x68 counting up by dt): kinds 1 and 3
  end once their age exceeds 5,000 ms; kind 2 is handled against its owning creature with a
  1,200 ms window (not traced). Hits go through the hit machinery.

---

## 14. Open questions

* **SS-1** Packet-6 kinds 1, 3 and 7 in full: the healing values of S 0x13be0, the per-kind
  static-entity transitions and the voxel object placement of kind 7.
* **SS-2** Static-entity kinds and their section-6 transitions.
* **SS-3** Dynamic objects and their 160-byte record.
* **SS-4** Section 11: no producer found.
* **SS-5** World-item timer +0x140 and whether dropped items ever expire (the day at +0x144
  suggests a cleanup that was not found).
* **SS-6** Whether the world is saved at shutdown; the unload path of §2.1 saves zones during
  play.
* **SS-7** Byte 0x44–0x47 of the particle record.

---

## Re-certification (2026-11-15)

Checked against `Server.exe` and `Cube.exe` (hashes above), the decompiled server and client
exports, the per-function assembly and fresh disassembly of the original images with the MSVC
disassembler (packet-6 cases, drop and loot routines, coin value, owner-rule sites), and
constant reads from the images' `.rdata`. The Infinite-Cubes reconstruction was not consulted.

**Confirmed.** Server/client step identity (2,109 calls, single stack-adjustment difference,
0xDA240 position offset); world created with online flag clear and no local player; owner rule
sites; main-loop order, 20 ms sleep, separate queue moves, shoots appended after the step,
player-zone list rebuild; generation radius (squared zone distance < 9), unload interval,
region distances; clock rates (×100 resting, ×10), 5 %/s local rest regeneration, day
rollover, 9:00 for unnamed worlds, `time` key and 10,000 ms save interval; pre-pass rules
(merchant refill, mission-target kill, active 3 × 3, mission generation, discovery); item
timers and landing sounds 58/59; activation rules 1–4 and the creature fields copied;
id formula, example and little-endian bytes; pet/bomb id rule; despawn rule (512-block
home test, (radius + 20)², −1 with no players); pet creation/refresh/kill fields; the hit
routine's relay of player-target hits, threat formula and multipliers, stealth/rest wake,
knock-back rule, kinds 1/4/5; death order items 1–3, 5 and 8, sound 27, 1,200,000 ms
countdown, experience formula; dead removal at 1,000 ms, pet revival at 20,000 ms, bomb
explosion radius and 0.025·dt bomb decay; loot slot order, chances, levels, type-drop table,
scales 0.8/3.0, coin formula and split, animal rarity thresholds, inventory rule; drop delay,
day and angle; packet-6 layout and kinds 1, 2 (inn times, 7:00, sound 29 at volume 0.75), 4,
5 (16, sound 45), 6 (sound 57), 7 (creature byte +0x18d), 8; section 9/10 sizes; event-set
offsets and wire order; particle layout; worked-example arithmetic; zone example.

**Corrected** (old → new):
1. Addresses given as virtual addresses → RVAs with the image named.
2. Main loop step 6 "answers pending zone and map requests" → only the hand-off to S 0x272d0
   is confirmed; the rest moved to unresolved.
3. Unload: save and removal are both done by S 0xd79f0 (save only with the online flag
   clear); S 0xd7960 saves mission/monster data **and deletes the region object**, S 0xd78e0
   frees a second per-region block (old text had these roles swapped); "within 3/5 regions"
   → "differing by fewer than 3/5 on both axes".
4. Mission-target silent kill uses the cell of the creature's **home zone**, needs a mission
   present, and also zeroes the death timer.
5. "Event 40" record: "u64 id + 32-byte buff" → 40 bytes (source id, target id, 24-byte
   buff), applied to the target; same-kind replacement only for kinds 1, 2, 3, 6, 7, 9, 10,
   11.
6. Spawn timers: "follows its time-of-day schedule" → schedule entries overwrite the spawn's
   active block; countdown decremented without clamping.
7. Online/unnamed-world gating added: steps 7–11 skipped online, steps 9 and 11 also skipped
   for an unnamed world. §10 table: "steps 7–11 skipped" kept; "AI for NPCs and the local
   player" → AI runs only for NPCs with the online flag clear (reversal of the table's AI row).
8. Step 10 additions: orphaned-pet reset (controller 3, timer 5,000); revival conditions and
   clean-up; dynamic objects destroyed beyond 512 blocks.
9. New step 14 (target clean-up, pets of owners with action 106); wrap-up now numbered 15;
   section-6 zones get flag +0x76 (not the item-save flag).
10. Active zone set iterates in zone-address order (affects cross-zone draw order).
11. Activation: "mission adjustments" → specified, including the **`srand` reseed** with the
    cell seed plus the spawn id, equipment regeneration, re-roll and inventory level/rarity
    draws (affects random-draw order).
12. Id callers: "zone generator 0x518630 and mission generator 0x509e40" → S 0x118630,
    S 0xe28e0, S 0x100300 (the 0x509e40 claim was not confirmed).
13. Pets: need a named world; creation needs bit 0x10 clear in the player's byte +0x124;
    slot 12 stated with its offset.
14. Hit routine: added the flag-0x2000 gate, kind 3 no-op, the damage test S 0x9620,
    stun-raise only for kinds other than 1 and 4, the +0x34 zeroing, block-gauge numbers and
    the kind-5 section-12 record; heal threat → only creatures satisfying S 0xcfcc0(healer, *c*)
    that already list the healed creature, + 0.1 × heal.
15. Death order: forgetting targets moved from before the experience step to the final
    per-creature loop, which also strips the victim's and pet's ids from all threat maps.
16. **Mission reward gifts go to every player creature when a hostile victim completes the
    mission, not only to players in its threat map** (multiplayer-affecting).
17. Section-13 "56-byte state" → 56-byte record (cell x, cell y, 8 bytes, 40-byte state); mission
    kinds 1–5/13 complete, 7–12 count, others unchanged; only the first qualifying player.
18. **Coins and animal items only for hostile NPCs and type 143** (old: any creature without
    flags 0x18) (affects drops and draws).
19. Loot draw order specified (offsets, index, ignored angle argument, then delay and angle in
    the drop routine); type-drop count draw always spent; pet-food rarity draws spent and
    level 1; animal rarity draw precedes the 1-in-20 draw; scale 0.75 for recipes, coins,
    animal items and inventory (random-draw order).
20. Drop routine: position offset belongs to the loot routine; the angle is always drawn
    (packet-6 kind 6 "angle 0" was wrong); nothing is placed when the zone is unloaded;
    lowering passes block types 0 and 2.
21. Inn also clears the "missions generated" flags; the sound is at the requesting player;
    flag-0x80 NPCs end the search; the reset skips pets and NPCs with unloaded home zones.
22. Pickup order: changed set, gift, sound draw, removal.
23. Pet reset clears the player's hit-counter map and target (old "attacker list").
24. Static-entity kind 45 is local-player-only; seat kinds and trigger kinds listed.
25. Heal item also raises the server's copy of the player's health; the hit record's attacker
    id is 0.
26. Section 12 also carries kind-5 absorb records (old: "only NPC abilities").
27. Bomb explosion: squared radius < 64; sound 1 with one pitch draw when blocks were removed.
28. Respawn example: a player holding positive threat keeps the NPC from despawning.
29. Projectiles: kinds 1 and 3 expire after 5,000 ms; kind 2 differs.

**Removed.** The sentence addressed to a replacement host; the statement that the document
closes another specification's question; §14 "Corrections to other specs" (statements about
other documents); capture and recording procedures in the open questions; sound file names;
packet-0 field numbers and bit numbers (replaced by object offsets); connection-side ordering
of packets in the worked example; the claim that NPC ids match "in an offline client".

**Unresolved.**
* Whether the connection side (S 0x272d0 and the connection threads) answers zone and map
  requests and how it filters per player.
* Client-side claims not re-compared: the client setting the online flag on joining, and
  client counterparts of the hit routine and zone generator.
* Mapping of sound ids (1, 27, 29, 44, 45, 57, 58, 59) to sound files.
* Packet-0 field numbering of the creature offsets used here.
* Draw sequence of the spawn equipment regeneration S 0xfb480, of the region mission
  generator S 0x10d8d0, of chest drops and of S 0xd8c90; which mission spawns receive ids
  through S 0xe28e0 and S 0x100300.
* Exact cross-zone draw order, which follows zone object addresses.
* Meaning of S 0xd5f40, of the attacker test S 0xf5a0, of the relation S 0xcfcc0, of the
  player byte +0x124 bit 0x10 and of the stealth reduction exponent.
* Equipment slot 12 for the pet item is inferred from the 0x118-byte slot stride.
* Section 11 producer; particle bytes 0x44–0x47; world-item timer +0x140; whether generated
  items carry day −1; shutdown save.
* Whether the zone generator takes the world lock when called from the generation thread.
