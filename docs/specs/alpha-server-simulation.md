# Alpha multiplayer: the dedicated server's world simulation

Behavioural specification of what Cube World **Alpha**'s `Server.exe` (SHA-256
`1ffbcbce…34fb1`) simulates each tick so that it can host NPCs for Alpha clients: the main
loop, which zones it generates, keeps and unloads, how NPCs are spawned, run, despawned,
killed and respawned, what the server does with the hits and requests clients send, and
what it sends back. It closes open question S-3 of `alpha-net-server.md`.

Written from the Alpha decompile (`disassembled alpha cube world/decompiled/server` and
`…/client`), the per-function assembly where the C output is missing, and a fresh
disassembly of the jump-table cases that the export did not reach. Nothing here is copied
code. Routines are cited by address in `Server.exe` ("server 0x…") or `Cube.exe` ("client
0x…"); offsets into in-memory objects are given only for cross-checking.

**Status.** The main loop, the tick order, the zone generation and unload rules, the active
zone set, spawn activation, despawn, death, respawn, kill experience, mission rewards, NPC
loot, pickup and drop, the interaction requests of packet 6, the creature id rules, and
the meaning of world-update sections 3, 8, 9 and 10 are specified. NPC behaviour itself
(steering, combat AI, actions, hit resolution, physics) is the shared code already described
in `alpha-creatures-behaviour.md` and `alpha-physics-camera.md`, and is only referenced.
Projectile flight, static-entity kinds (traps, doors, chests) and the dynamic objects of
packet 3 are only outlined (§13).

Related: `alpha-net-server.md` (configuration, joins, interest rule, trust), `alpha-net-packets.md`
(record layouts), `alpha-creatures-behaviour.md` (§9 spawn objects, §12 kills, §13 AI),
`alpha-worldgen-overview.md` (§1 where generation runs), `alpha-saves.md` (§5 world save).

---

## 0. Conventions

As in `alpha-net-server.md`: "player" is a creature whose controller byte (record 0x50) is
0, "NPC" any other creature. Positions are i64 in 1/65536 block; a **zone** is 256 × 256
blocks, zone (zx, zy) = (⌊x / 65536 / 256⌋, ⌊y / 65536 / 256⌋) rounded toward zero; a
**region** is 64 × 64 zones; a **map cell** is 8 × 8 zones. Distances are in blocks unless
a zone distance is stated. "draw" is one C runtime `rand()` (0..32767), *r* = draw / 32767.
"dt" is the step's elapsed time in milliseconds.

The 13 world-update sections are numbered as in `alpha-net-packets.md` §3.4. Inside both
programs they live in one 13-list "event set"; §11 gives the mapping.

---

## 1. One simulation, two programs

The world simulation step is the same code in both programs. Server 0x5322d0 and client
0x60c510 were compared instruction by instruction after normalising addresses: they differ
only by one stack adjustment near the start (`alpha-physics-camera.md` notes the same), and
every routine either one calls has a counterpart at the same position in the other (the
call sequences pair up one to one, 2,109 calls each).

What makes the server behave as a server is only the data it runs on:

* Its world object is created with the **online flag** (world +0xb4) **cleared** and with
  **no local player** (world +0xb8 = none). The client sets the online flag when it joins a
  server (client 0x58eb00) and has a local player.
* It passes a non-empty **input event set** to the step (the hits and "event 40" records
  received from clients). The client passes none.
* It has many players in its creature table instead of one.

So the server simulates exactly what an offline client simulates (NPC AI, spawning,
deaths, loot, missions), for every player at once, and everything that an offline client
would apply to "the local player" is instead **sent to the player concerned** (§6). An
AlphaVulkan single-player host can run the same rules with one player.

**The owner rule.** Throughout the step, a creature's action effects (a melee hit landing at
the end of the wind-up, an ability's buff, a dodge) are computed only by its owner: "online
flag clear and the creature is an NPC, or the creature is the local player"
(for example server 0x53aaab, 0x53ce3a). On the server this means NPCs only. Players'
attacks therefore reach NPCs only through packet 7; the server never re-simulates them.

---

## 2. The main loop (server 0x549c50)

One iteration, repeated until `Q` is typed (adds detail to `alpha-net-server.md` §2):

1. *t* := `timeGetTime()`; dt := *t* − *t*previous. dt is **not** split into 20 ms pieces as
   on the client; with the 20 ms sleep below it is normally 20–22 ms.
2. Briefly holding the world lock, take (and empty) the queue of received **hits** (packet 7) into
   section 2 of a fresh input set, and the queue of received **"event 40"** records
   (packet 8) into its section 12.
3. **Lock the world** (world critical section +0x8000c0).
4. Make an empty output set and run the **step** (§3) with dt, the output set and the
   input set.
5. Append the received **shoots** (packet 9) to output section 5, unchanged.
6. Distribute the output set to every connection (server 0x4272d0; per-player filters of
   `alpha-net-server.md` §4.2) and answer pending zone and map requests.
7. Rebuild the generator's **player-zone list**: one entry (zx, zy) per player creature.
8. **Unlock the world**, `Sleep(20)`.

The connection threads read creatures under the same lock (`alpha-net-server.md` §4.1).

### 2.1 The generation thread (server 0x549550)

A second thread runs continuously (it does not sleep) while the server runs:

* **Generate.** Among all zones (zx, zy) with (zx − px)² + (zy − py)² < 9 around any
  player zone (px, py) — the 5 × 5 block of zones centred on the player's zone — find the
  one **not yet generated** that is closest to a player zone, and generate it with the
  normal zone generator (server 0x518630, the counterpart of client 0x5e4850: terrain,
  structures, static entities, world items and **spawn objects**). One zone per pass.
* **Unload**, at most once per 1000 ms: every loaded zone with no player zone within
  squared zone distance < 9 is **saved** (server 0x4d81b0, `alpha-saves.md` §5.2) and
  removed (server 0x4d79f0). A region with no player zone within 3 regions on both axes
  has its map-cell data saved (`mission`/`monster` keys, server 0x4d7c50) and dropped
  (server 0x4d7960); a region with no player within 5 regions is freed (server 0x4d78e0).
* If the server object's seed or world name stop matching the world's, every zone is saved
  and the world is re-initialised (server 0x4d83a0). This does not happen in normal play.

With no players connected nothing is generated and, within a second, everything is
unloaded and saved. The clients generate the same zones themselves from the seed
(`alpha-worldgen-overview.md` §1); the server's copy is what NPCs, items and requests use.

---

## 3. The step: order of work (server 0x5322d0)

Positions in the step are given for the server; add 0xDA240 for the client.

1. **Clock** (0x532330–0x5325fd). If any player is resting (action 84) with health > 0, the
   time of day advances **100 × dt**, otherwise **10 × dt** (one game day = 8,640 s ≈
   2 h 24 min of real time; the local player of a client also regenerates 5 % of maximum
   health per second while resting). Past 86,400,000 ms: day + 1, time − 86,400,000, and
   every loaded region's "missions generated" flag (region +0x15a18) is cleared
   (0x524500), so missions are rebuilt on the next visit (step 2). A running total of
   simulated ms is kept; whenever it crosses a multiple of **10,000 ms** the key `time`
   (day, time) is written to the world save. (A world with an empty name keeps 9:00 and
   never saves; the server's world is always named.)
2. **Creature pre-pass** (0x5326a1–0x532ac5), for every creature:
   * controller 3 (traders are likely): if its inventory is empty it is refilled from its
     spawn object's inventory;
   * a creature with appearance flag 0x2000 (mission target), an empty threat map and a
     map cell whose mission is complete (cell +0x41 = 2) is killed silently (health 0);
   * every **player** is added to the step's player list, and the 3 × 3 zones around its
     zone that are loaded are added to the **active zone set**; then, on the server or
     offline: if the player's region has not had its missions generated this day, they are
     generated (0x50d8d0: map-cell missions, mission spawn objects, section-13 records),
     and if the player's map cell has a mission not yet discovered (cell +0x41 = 0) and the
     player is inside its area (0x52dee0 > 0), the cell is marked discovered (+0x41 = 1)
     and its 56-byte state is put into **section 13**.
3. **Incoming hits** (0x532adf–0x532b92): every input hit is applied with the hit routine
   (server 0x4cea80, §6.1).
4. **Incoming "event 40"** (0x532b92–0x532c6a): every input record is a u64 creature id and
   a 32-byte buff; if that creature exists the buff is added to (or replaces the same-kind
   entry of) its buff list (0x411740). The record is **not** copied to the output set.
5. **Active zones: world items** (0x532c6a–0x533097): every item lying in an active zone
   counts down its two timers (item record +0x13c, +0x140, −dt, floor 0); when +0x13c
   reaches 0 a **landing sound** goes to section 4 (sound 58 `drop-coin.wav` for coins,
   item type 12; 59 `drop-item.wav` otherwise; pitch 1 + 0.1*r*). Items of type 16 turn to
   face a neighbouring wall.
6. **Active zones: static entities** (0x533097–0x533bab): traps fire on creatures that
   step on them (hit resolution with fixed damage), doors, gates and similar objects
   change state; every change is put into **section 6** (§13).
7. **Spawn timers** (0x533c7b–0x533dc3): every spawn object of every active zone follows its
   time-of-day schedule and counts its respawn countdown down by dt.
8. **Despawn** (0x533dc3–0x5341d9, §5.4).
9. **Player pets** (0x5341d9–0x5347c7, §5.6).
10. **Dead creatures** (0x5347c7–0x535702, §6.3).
11. **Active zones: dynamic objects and spawn activation** (0x535702–0x536259, §5.2).
12. **Per creature** (0x536259–0x545c21), for every creature, players included:
    the creature's queued **interaction requests** (packet 6, §8); health clamped to the
    maximum; nothing more if health ≤ 0; for NPCs the **AI** (every ≥ 32 ms,
    `alpha-creatures-behaviour.md` §13); steering, action clock, action effects (owner
    rule, §1), stamina, mana, stealth, buffs, physics and collision
    (`alpha-physics-camera.md`). Hits produced here go through the hit routine (§6.1).
13. **Dynamic objects and projectiles** (0x545c21–0x546c40; §13).
14. **Wrap-up** (0x546efc–0x547222): for every zone whose item list changed this step, the
    zone is flagged for saving and its **whole item list** goes into **section 7** keyed by
    (zx, zy); every zone named by a section-6 record is flagged for saving; every zone whose
    16-byte object list changed (§8 kind 7) sends that whole list in **section 8**; block
    edits made this step are appended to their zones' stored edit lists (they were already
    put into section 1 when made).

---

## 4. Zones on the server

| Set | Extent around each player's zone | Used for |
|---|---|---|
| generated / loaded | 5 × 5 zones (squared zone distance < 9) | everything; answers packet 11 |
| **active** | 3 × 3 zones | spawn activation and timers, world-item and static-entity simulation |
| saved and unloaded | outside every player's 5 × 5 | checked once a second (§2.1) |

A packet-11 request for a zone outside the loaded set gets no answer (`alpha-net-packets.md`
§4.4); a client asks for the zones it needs every second, and those are normally inside the
5 × 5 block.

**Example.** A player at block (8,398,402, 8,397,472) is in zone (32806, 32802). Loaded:
zones 32804–32808 × 32800–32804 (25 zones, 1,280 × 1,280 blocks). Active: 32805–32807 ×
32801–32803. A second player 2,000 blocks east adds its own 25 and 9 zones.

---

## 5. NPCs on the server

### 5.1 Where NPCs come from

All NPCs come from **spawn objects** (`alpha-creatures-behaviour.md` §2, §9.1) created by
the zone generator (towns, dungeons, wildlife, ravine deposits) or by the daily mission
generator (§3 step 2). There is no other spawning: no random wandering monsters, no
waves. The spawn object stays in its zone; the creature it makes is the NPC.

### 5.2 Activation (0x535908–0x5361f3, = client §9.2)

The rule is the client's (`alpha-creatures-behaviour.md` §9.2), applied to the spawn objects
of **active** zones and with "a player" meaning **any** player of the step's player list:

1. skip if the spawn has a time-of-day window and the time is outside it;
2. skip if a creature with the spawn's id already exists;
3. clamp the countdown at 0; skip if it is not 0 **and** the death day is today;
4. skip unless some player is within the spawn's radius (spawn +0x08, default 200 blocks;
   squared distance in blocks < radius²);
5. mission adjustments, then create the creature with the spawn's fields; additionally the
   creature remembers its **home zone and spawn index** (record field 39: zx, zy, index)
   and its **despawn radius** = the spawn radius (object +0x1d2c), and gets health =
   maximum and its behaviour (§13.1 of the creature spec).

Before activation, in the same per-zone pass, each entry of the zone's dynamic-object list
is turned into a world dynamic object once a player is within 512 blocks (§13).

### 5.3 Creature ids

| Creatures | Id |
|---|---|
| players | 1–10 (`alpha-net-server.md` §3.1) |
| NPCs from spawn objects | **(zy · 65,536 + zx) · 256 + i**, as a u64, where (zx, zy) is the spawn's zone and *i* its index in the zone's spawn list (server 0x4f3850, set by the zone generator 0x518630 and the mission generator 0x509e40) |
| pets and placed bombs | **one less than the smallest id in use**, starting below 0: −1, −2, … (as i64) |

Spawn ids are deterministic, so the same NPC has the same id on the server and in an
offline client. They are unique as long as a zone has fewer than 256 spawns; a zone with
zx < 1 and zy = 0 could collide with player ids, which does not happen in the playable
world (zones near 32768).

**Example.** Spawn index 3 of zone (zx 32806, zy 32800): (32800 · 65536 + 32806) · 256 + 3 =
550,301,083,139 = 0x80_2080_2603; on the wire `03 26 80 20 80 00 00 00`.

### 5.4 Despawn (0x533e10–0x5341d9)

For every living NPC that is not a pet:

* **Kept** if some player is in its threat map with threat > 0 and the NPC is within
  **512 blocks** of its home (squared distance < 262,144).
* Otherwise let *m* = the smallest squared distance from the NPC to any player (−1 with no
  players). If *m* > (despawn radius + 20)², by default 220² = 48,400, the NPC is **removed
  from the world** (deleted and taken out of every threat map).

Despawning does not start the respawn countdown: the spawn simply activates again when a
player comes back within its radius. With no players connected nothing despawns (but the
zones unload within a second, §2.1).

### 5.5 What clients see

Unchanged from `alpha-net-server.md` §4.1: every NPC within 200 blocks of a player is listed
in that player's creature batch (packet 0, then packet 2). Because activation happens at
≤ 200 blocks and despawn at > 220, an NPC normally appears just inside a player's view and
disappears from it (packet 2 removal) a little before it is deleted on the server. The
packet-0 deltas are the server's own copy of each NPC record, compared field by field with
what that connection was sent last (`alpha-net-packets.md` §2.2); the step itself produces
no creature messages.

### 5.6 Player pets (0x5341d9–0x5347c7)

For every player whose equipment slot 12 holds a pet item (item type 19): if the player has
no pet creature, the server creates one: id per §5.3, controller 5, owner id in record
field 35, creature type = the item's subtype, level = item +0x10, experience = item +0x04,
name = the item's sub-block bytes (the `/namepet` name), at the owner's position, health =
maximum. If the item changes, the pet's type, level and name are refreshed. If slot 12 no
longer holds a pet item the pet is killed (unless it carries a buff of kind 7). Pets then
behave as NPCs (they follow `alpha-creatures-behaviour.md` §13.2 pet rules) and are listed
to clients like any NPC.

---

## 6. Hits, deaths, loot and rewards

### 6.1 The hit routine (server 0x4cea80 = client 0x595a60)

Every hit, whether received in packet 7 or produced by the step (NPC attacks, traps,
projectiles, spirit life steal), goes through one routine:

1. Ignore it if the target does not exist or its health ≤ 0.
2. **Target is a player and there is no local player (the server): append the hit record
   unchanged to section 2 and stop.** The target's own client applies it
   (`alpha-net-packets.md` §5.1). This is how NPC damage, PvP hits from packet 7, heals and
   block/absorb results reach players.
3. Otherwise (the target is an NPC, or offline the local player): 
   * a hit with positive amount reduces the target's stealth and wakes a resting target
     (action 84 → 0, hit-stun 3000 ms);
   * if the attacker exists and the amount ≥ 0: both remember each other as last
     attacker/target, the attacker's per-target hit counter (object +0x13ac) grows by 1,
     and the **attacker's threat** in the target's threat map grows by 0.01 + amount × *m*
     (*m* = 1 when 0x40f5a0 holds for the attacker, 10 for attacker type 25, 20 for a
     shield user of specialization 1, 2 otherwise); then 0x4d5f40 runs (pets joining in
     is likely);
   * knock-back (hit +0x38) replaces the target's velocity if it is longer (× 0.1 while the
     target is already stunned and the hit carries no stun); hit +0x18 raises record field
     16 (hit-stun) and hit +0x1c raises field 17, as on the client;
   * kind 1 raises the block gauge, kind 4 applies a special effect (0x408230), kind 5
     shrinks absorbing buffs, any other kind subtracts the amount from health (a negative
     amount heals, clamped to maximum, and adds threat for the healer on every creature
     hostile to the healed one);
   * if health is now ≤ 0, the death of §6.2 follows.

So hits on NPCs are applied on the server and **not relayed**; other clients see the result
through the NPC's health in packet 0.

### 6.2 Death of an NPC (same routine, continued)

1. Health := 0, death timer (record 0x5c) := 0; **sound 27** (`destroy2.wav`, pitch 1,
   volume 1) at the victim goes to section 4.
2. If the victim's home zone has a static entity of kind 5 linked to the victim's spawn
   index and still armed, it is triggered (0x4d8c90) and its new state goes to **section
   6** (a gate or chest unlocked by a guardian's death is the likely use).
3. **Loot** (§6.4).
4. Every creature that targeted the victim (or the victim's pet) forgets it; the victim's
   pet dies too.
5. **Kill experience** *e* = max(1, 20 · *c*(level + tier)), × 20 for the big types,
   × 10 with the boss flag, 0 with appearance flag 0x800 (`alpha-creatures-behaviour.md`
   §12.3).
6. **Missions** (server and offline only): as in the creature spec §12.3; on progress or
   completion the map cell's 56-byte state goes to **section 13**; on completion the
   mission's extra spawn objects are removed from the 64 zones of the cell.
7. For every **player** in the victim's threat map, when the victim is a hostile NPC
   (controller 1): a **kill record** goes to **section 10**: u64 player id, u64 victim id,
   i32 victim type, i32 ⌊*e*⌋ (24 bytes). The player's client adds *e* to its experience;
   the server adds it to the player's pet, if alive, and to the pet item it tracks. If the
   mission was completed, each such player also gets two **item gifts** in **section 9**
   (u64 player id + 280-byte item): an experience item (type 13, level ⌊50 · *c*(mission
   level + tier) + 1⌋) and a reward from the armour or the weapon generator (even/odd
   draw; mission level and rarity, the player's class).
8. If the victim came from a spawn object: countdown := **1,200,000 ms**, death day :=
   today (`alpha-creatures-behaviour.md` §9.4).

Dead NPCs are removed from the world **1000 ms** after death (§3 step 10; the record's
death timer counts up by dt). Dead pets instead come back after **20,000 ms** at their
owner's position with full health. A dead bomb (type 144) explodes on its first dead
step: particles (section 3), every block within 8 blocks removed (section 1) and damage to
creatures around it.

### 6.3 Respawn timeline (example)

A wolf spawn in an active zone, radius 200. A player walks to 150 blocks: the wolf is
created on the next step (id from §5.3) and appears in that player's batch. The player
kills it at 10:00 game time on day 7: kill record to the player, sound 27, loot (§6.4);
1000 ms later the creature is deleted and disappears from every batch. The spawn now has
countdown 1,200,000 and death day 7. It comes back when either 20 real minutes have passed
(countdown 0, but only while its zone is active) or the day counter becomes 8 — at the
clock's ×10 rate that is 14 h of game time = 84 real minutes after 10:00, so the 20-minute
countdown wins unless someone rests. If the player walks away (> 220 blocks) before
killing it, the wolf is simply removed and recreated, unharmed, on return.

### 6.4 NPC loot (server 0x4d2ae0)

Nothing drops for players, controller 3 (merchants) or appearance flag 0x800. Otherwise,
in this order (one draw is spent first):

1. **Equipment**: among the victim's non-empty slots 6, 7, 2, 5, 4, 3, 1, one at random
   drops when a draw mod 4 = 0, always for bosses (flag 0x200). Scale 1.0.
2. Hostile NPCs (controller 1) and type 143, one draw in 10: one **random item** from the
   armour or the weapon generator (even/odd draw; `alpha-items-generator.md` §2) of level
   max(1, level − 1 + draw parity), rarity from the rarity roll with the tier (server
   0x52bf40), for any class, level re-rolled with spread 0.05; then one draw in 5 also a **pet-food**
   item (type 20, subtype a random tameable creature type, re-rolled the same way).
3. **Type drops**: 1 + draw mod 3 items of type 11 by creature type — 58 → subtype 2
   material 19; 70 → 18; 71 → 21; 120, 121, 124, 126 → subtype 5 material 21 or subtype 1
   material 2 (draw parity); 122 → 27; 123 → 11 material 27; 125 → 6; 127 → 20; 128 → 23;
   139 → 8 material 22; deposits: 131 gold → subtype 0 material 11, 132 iron → material 1,
   133 silver → 12, 134 → 17, 135 emerald → material 13 rarity 1, 136 sapphire → 14 rarity
   2, 137 ruby → 15 rarity 3, 138 diamond → 16 rarity 4. Scale 0.8 (3.0 for 131–133 and
   135–138); other types drop nothing here.
4. Hostile NPCs and type 143: one draw in 50: a **recipe** (server 0x52a760, the recipe
   generator of `alpha-items-generator.md` §3) of the victim's level, rarity from tier + 1.
   Then, for creatures without appearance flags 0x18 (not animals): **coins** worth
   *V* · 10 · (1 + 2*r*), × 10 for bosses, where *V* = 2^(3·*c*(*L*)) · 2^(*t*/4) (server
   0x410f00, *c* the level curve, *t* the tier), split into copper (value mod 100),
   silver (⌊value/100⌋ mod 100) and gold (⌊value/10000⌋) — item type 12, material 10, 12
   and 11, amount in item +0x10, one item per non-zero kind. Animals instead have one
   chance in 20 of an item of type 14 whose rarity is 0 (nothing) below 700 on draw mod
   1000 + 20 × tier, 1 below 950, 2, or 3 above 998; bosses + 1, at most 3. The item gets a seed draw and
   the victim's level.
5. Hostile NPCs: every inventory stack except healing potions (type 1 subtype 1) drops one
   item per unit.

Every dropped item (server 0x4d2810) becomes a **world item** in the zone under it: position
= victim position + (1 − 2*r*, 1 − 2*r*′, 0) blocks, then moved down to the first solid block
and placed one block above it; angle 360·*r* degrees; landing-sound delay 500 + draw mod
300 ms (+0x13c); the current day at +0x144 (−1 marks generated items). The zone joins the
step's changed-zone set, so its whole item list is sent in section 7 at the end of the step
(§3 step 14) and saved with the zone.

**Example.** A level-5 tier-0 goblin warrior (controller 1) with sword, shield and chest
dies. Draw mod 4 = 0 → its shield drops. Draw mod 10 ≠ 0 → no random item. Type has no type
drop. Draw mod 50 ≠ 0 → no recipe. Coins: *c*(5) = 1 − 1/1.2 = 0.1667, *V* = 2^0.5 = 1.414,
with *r* = 0.5 the value is ⌊1.414 · 10 · 2⌋ = 28 → one item of 28 copper. The player gets
a kill record of 3 experience; the next world update to every client carries section 7 for
that zone with the two new items.

---

## 7. Packet 7 and packet 8 from clients

* **Packet 7 (hit)**: queued, then applied by the hit routine in step 3 of §3 — on an NPC it
  changes the NPC on the server; on a player it is relayed in section 2 to everybody (only
  the target applies it). The sender's own hits on NPCs are never echoed back.
* **Packet 8 ("event 40")**: queued, then added to the named creature's buff list on the
  server (§3 step 4). It is **not** relayed. Section 12 carries only the buff records the
  server's own step makes (NPC abilities).
* **Packet 9 (shoot)**: not given to the step; appended unchanged to section 5 for every
  client (the main loop, §2 step 5). The server does not simulate player projectiles.

---

## 8. Packet 6: interaction requests

Packet 6 is an **interaction request** made by the sender's player. The server stores it in
that player's creature queue (object +0x130c); the step handles the queue at the start of
the creature's turn (§3 step 12) and empties it. Offline, the client puts the same records
in the local player's queue and its own step handles them; online the client sends them
instead (`alpha-net-packets.md` §4.1). Layout:

| Off | Size | Meaning |
|---|---|---|
| 0x000 | 280 | an item (what is used or dropped) |
| 0x118 | 12 | a target: 3 × i32 (zone x, zone y, index) for kinds 2, 3 and 5 |
| 0x124 | 4 | *unknown* |
| 0x128 | 1 | **kind**, 1–8 (jump table server 0x5489e0) |
| 0x12a | 2 | *unknown* |

| Kind | Meaning on the server |
|---|---|
| 1 | **use an item**: item type 1 subtype 7 places a **bomb** — a creature of type 144, controller 6, level = item level, at the player, id per §5.3; it loses 0.025·dt health per step and explodes when dead (§6.2). Any other item heals the player by its healing value (0x413be0): sound 44 (`drink2.wav`) in section 4 and a hit record with amount −heal on the player in section 2, which is what heals the player's client |
| 2 | **talk to an NPC** (target = the NPC's home zone and spawn index): only one server effect, the **inn** — an NPC of class 132 (0x84) at night (time ≥ 18:00 or ≤ 6:00): day + 1, time := 7:00, sound 29 (`levelup2.wav`, volume 0.75), every spawned NPC removed with its countdown reset (0x4d7ae0) |
| 3 | **use a static entity** (target = zone, entity index): doors, chests and other usable objects (the handler switches on the entity kind: 16, 18, 19, 68, 69, 10, 1–3, 9); a chest (kind 10) that is opened drops its contents as world items; the new state goes to section 6 |
| 4 | nothing |
| 5 | **pick up a world item** (target = zone, item index): if the item exists, the player may take it (0x409660) and the item is within **4 blocks** (squared distance ≤ 16), the item is removed from the zone, a **section-9 gift** (player id + item) is sent, sound 45 (`pickup.wav`, pitch 1 + 0.1*r*) goes to section 4, and the zone's new item list goes out in section 7 |
| 6 | **drop an item**: the item becomes a world item at the player's position (scale 1.0, angle 0), sound 57 (`drop.wav`) in section 4; the zone's list goes out in section 7 |
| 7 | **place a voxel object**: object number = record byte 0x17d of the player (field 32), placed in front of the player along its look vector (field 26); its blocks go to section 1 and are stored with the zone; the zone's 16-byte object list is updated and sent in section 8 (*partly traced*) |
| 8 | **reset the pet**: the player's pet forgets its threats, comes back to the player if more than 200 blocks away, and the player's attacker list and target are cleared |

This answers `alpha-net-packets.md` P-8: dropping is kind 6, and picking up, using, talking
and building go through the same packet.

---

## 9. Section 9 and 10 records seen by the client

* **Section 9** (288 bytes): u64 player id, then a 280-byte item. "Give this item to this
  player": pickups, mission experience items and mission rewards. Only the named player's
  client is expected to act on it, adding the item to its inventory (*inferred*: offline
  the step adds the same item straight to the local player's inventory, 0x427000).
* **Section 10** (24 bytes): the kill record of §6.2 step 7.
* **Section 11** (24 bytes): no writer was found in the server; it is always empty (open
  question SS-4).

---

## 10. The client online and offline

| | Offline client (and the server) | Online client |
|---|---|---|
| spawn activation, despawn, respawn timers, pets | runs (§3 steps 7–11) | skipped; NPCs exist only as creatures received in packet 0 and are deleted by packet 2 |
| AI, action effects | NPCs and the local player | the local player only (owner rule) |
| hit routine on NPCs | applies damage, threat, death, loot, rewards | threat bookkeeping only; damage, knock-back and death come from the server |
| hit routine on the local player | applies | applies records received in section 2; hit resolution against itself is skipped (server 0x4cfd50 start) |
| missions, map-cell discovery, item landing sounds, kill rewards | runs | from sections 13, 4, 10 and 9 |
| packet-6 queue | handled by the step | sent to the server |
| movement, physics, animation of NPCs | simulated | still simulated between updates; positions from packet 0 become interpolation targets (`alpha-net-packets.md` §2.3) |

Online, the client also keeps its pets' owner links from record field 35 (§3 step 8 when
online, 0x533bbf). So an Alpha client online never decides anything about an NPC; it
draws what the server sends and predicts motion in between.

---

## 11. The event set

Both programs keep the 13 sections in one 13-list structure in a different order from the
wire. The server serialises it in wire order (server 0x423dd0 → 0x421b10, 0x421b80, …).

| Offset in the set | Section | Records |
|---|---|---|
| 0x00 | 2 | hits on players (§6.1) |
| 0x08 | 4 | sounds |
| 0x10 | 3 | **particle bursts** (below) |
| 0x18 | 1 | block edits |
| 0x20 | 5 | shoots |
| 0x28 | 7 | zone item lists |
| 0x30 | 6 | static-entity states |
| 0x38 | 8 | zone 16-byte object lists |
| 0x40 | 9 | item gifts |
| 0x48 | 10 | kill records |
| 0x50 | 11 | (unused) |
| 0x58 | 12 | "event 40" buffs |
| 0x60 | 13 | map-cell states |

**Section 3 = particle bursts** (resolves `alpha-net-packets.md` P-5). Written by the spirit
bursts (server 0x4d2190, `alpha-creatures-behaviour.md` §12.1), the hit resolution and the
bomb explosion. The record starts with the effect position as 3 × i64, which is why the
distributor filters it by distance like a position. Layout as built by 0x4d2190: 0x00
position (3 × i64), 0x18 velocity (3 × f32), 0x24 colour (4 × f32), 0x34 f32 (0.1, size
likely), 0x38 i32 count, 0x3c i32 (2, a style likely), 0x40 f32 (3.0), 0x44 *unknown*.

---

## 12. Worked example: one server tick near a fight

Two players (ids 1 and 2) stand 30 blocks apart in zone (32806, 32802); a level-5 goblin
(id 550,301,083,139) attacks player 1, who hits back. In one tick:

1. The receive threads have queued from client 1 one packet 7 (attacker 1 → target goblin,
   amount 12.0, kind 0).
2. Step: clock + 10·dt. Pre-pass: both players listed; active zones are the 3 × 3 around
   32806, 32802. The queued hit is applied: goblin health 30 → 18, player 1's threat on it
   0.01 + 12 × 2 = 24.01 (with *m* = 2). No relay.
3. Per creature: the goblin's AI (if 32 ms have accumulated) keeps player 1 as target; its
   swing reaches the end of its wind-up this step: hit resolution against player 1 makes a
   kind-0 hit (goblin → 1, amount 7.5) and an impact sound; the hit routine sees a player
   target on a server and appends it to section 2.
4. Output: section 2 = 1 hit, section 4 = 1 sound (within 200 blocks of both players, so
   both get it). Section 2 goes to both clients; only client 1 applies it.
5. Creature batches: client 1 gets player 2 and the goblin (health 18 changed → bit 27);
   client 2 gets player 1 and the goblin. Then packet 2, packet 5, packet 4.

When the goblin's health reaches 0 the same tick also carries: sound 27, a kill record
(1, goblin id, type, 3) in section 10 to everybody (only client 1 acts), and section 7 for
the zone if anything dropped; 1000 ms later the goblin stops being listed.

---

## 13. Outlined only

* **Static entities** (zone list of 392-byte entities): traps (kinds 6, 7, 8, re-armed
  after 2000 ms) hit creatures standing in them through hit resolution, with damage
  derived from the zone's level; doors and similar objects react to packet 6 kind 3 and to
  nearby players; states go to section 6. The per-kind rules are not specified (SS-2).
* **Dynamic objects** (world +0xc, created from each active zone's list when a player is
  within 512 blocks; 160-byte objects with a position, a kind and a 3-state motion
  routine): the objects of the unsent packet 3 (`alpha-net-packets.md` §3.3). Boats or
  platforms are likely. Their state is saved per zone (`alpha-saves.md` §5.2 item 4). Not
  needed for Alpha clients, which never receive packet 3 (SS-3).
* **Projectiles** (world +0x14, kinds 1–3, lifetime 5000 ms): NPC arrows and spells, with
  hits through the hit routine. Open question F-3b of the creature spec.

---

## 14. Corrections to other specs

* `alpha-net-server.md` §2 step 4 and §4.2: section 12 is **not** a relay of packet 8; the
  server applies packet-8 buffs to its own creature copies and sends only buffs its NPCs
  make (§7). Section 2 carries hits whose **target is a player** (NPC attacks, PvP from
  packet 7, heals), not every player hit (§6.1). Section 3 carries particles (§11).
* `alpha-net-server.md` §3.3 / S-5: ids are now specified (§5.3).
* `alpha-net-packets.md` §3.4: sections 3, 8, 9, 10 are now known (§11, §8, §9); P-5 and
  P-8 are answered.
* `alpha-creatures-behaviour.md` §12.3 ("no per-monster loot drop was found") and N-1: NPCs
  drop loot through 0x4d2ae0 (client counterpart called from the same place, client
  0x595a60), §6.4 here; kill records travel in section 10 and item rewards in section 9.

---

## 15. Open questions (and how to settle them)

Captures with the user's 32-bit mod DLLs hooking `send`/`recv` of a scratch `Server.exe`
(`alpha-net-packets.md` §7) settle most of these.

* **SS-1** Packet 6 kinds 1, 2, 3 and 7 in full: which items heal how much (0x413be0), the
  other NPC classes that react to kind 2 (appearance flag 0x80 path), and the object list
  of kind 7. Capture each action online.
* **SS-2** Static-entity kinds and their section-6 transitions (traps, doors, chests,
  teleports): open each in a capture and diff the 88-byte records.
* **SS-3** Dynamic objects (world +0xc) and their 160-byte record; only matters for
  saving and for packet 3.
* **SS-4** Section 11: confirm it is always empty from `Server.exe` (count field in every
  captured world update).
* **SS-5** World-item timer +0x140 and whether dropped items ever expire (the dropped day
  +0x144 suggests a day-based cleanup that was not found in the step or the zone loader).
* **SS-6** Whether the world is saved at shutdown (`alpha-net-server.md` S-6); the unload
  path of §2.1 saves zones during play.
* **SS-7** The 72-byte particle record of §11 beyond the fields built by 0x4d2190.
