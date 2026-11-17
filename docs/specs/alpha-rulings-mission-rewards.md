Status: FINAL / STABLE / HELD

# Alpha ruling: mission progress gating, reward recipients, daily-pass region

Scope: three conflicts between held contracts about missions in the creature death routine
and in the step's creature pre-pass:

* `alpha-creatures-behaviour.md` §8 ("Mission progress", "Mission rewards");
* `alpha-missions.md` §2.1, §5.1, §5.3 and its re-certification of 2026-11-16
  (corrections 1, 10 and 11);
* `alpha-server-simulation.md` §3 step 2 and §6.2 steps 3–7.

Images: client Cube.exe (SHA-256 84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF)
and Server.exe (SHA-256 1FFBCBCE251E1506A63C9CE18AB0009D5553FCBB9E6202BE2BC3A7D870D34FB1),
both 32-bit, image base 0x400000. Addresses below are virtual addresses ("S" = Server.exe,
"C" = Cube.exe). `alpha-server-simulation.md` writes server addresses image-relative
(its "S 0xcea80" is S 0x4cea80 here).

Terms: **remote flag** = world byte +0xb4 (the flag `alpha-server-simulation.md` calls the
"online flag"); **local player** = world +0xb8; **controller** = creature byte +0x60
(0 player, 1 hostile NPC); **creature table** = the world's creature map, keyed by the
64-bit creature id.

---

## Ruling 1 — who receives the mission completion rewards

**Winner:** `alpha-missions.md` §5.3 (correction 11) and `alpha-server-simulation.md` §6.2
step 7. **Loser:** `alpha-creatures-behaviour.md` §8 "Mission rewards: each such player".

### Behaviour

In the per-creature loop at the end of the death routine (S 0x4cf6a5–0x4cfb79,
C 0x596685 onwards), every entry of the creature table is visited in the table's in-order
sequence, which is **ascending creature id** compared as a signed 64-bit value (the table's
find routine S 0x530600 compares the high words signed, the low words unsigned). For each
visited creature, after the target clearing and threat-map stripping of
`alpha-server-simulation.md` §6.2 step 7:

1. If the victim's controller is not 1, nothing further for this creature
   (S 0x4cf84a, C 0x59682a).
2. If this creature's controller is not 0, nothing further (S 0x4cf857, C 0x596837).
3. The victim's threat map is searched for this creature's id (S 0x4cf875, C 0x596855).
   Found: the 24-byte kill record is built and applied (S 0x4cf882–0x4cf908, ending in
   S 0x4d61c0 / C 0x5a0bf0). Not found: control jumps **directly to step 4**
   (S 0x4cf87c → 0x4cf90d, C 0x59685c → 0x5968ed); no kill record.
4. If this death completed the mission (the completion byte, cleared at S 0x4cf3b3 before
   the progress code and set only when the progress code completes the cell), the two
   reward items are made for this creature: the experience item (type 13, level
   truncate(50 × c(mission level + mission tier) + 1), constants 50.0 at S 0x558828 and 1.0
   at S 0x5586dc), queued as a section-9 gift and, if this creature is the local player,
   also added to its inventory (S 0x4cfa22, 0x427000); then **one** `rand` draw
   (S 0x4cfa3e, import 0x5582f4): even → S 0x528bf0 (C 0x5f51e0), odd → S 0x52c4e0
   (C 0x5f8ad0), called with mission level, mission tier and this creature's class
   (creature +0x140); that item is queued the same way and given to a local player the same
   way (S 0x4cfadc).

So **every player creature in the world** is rewarded when a hostile victim completes the
mission, whether or not it appears in the victim's threat map. On a server this is every
connected player regardless of distance; offline it is the local player. Players are served
in ascending id order, and each player's work (kill record if any, experience item, the one
parity draw, the generator's own draws, gift queueing) finishes before the next player is
visited.

The "such player" reading of `alpha-creatures-behaviour.md` §8 must be replaced by the above.
Its kill-record bullet is unaffected: kill records do require the player to be present in
the victim's threat map (no threat > 0 test).

### Residual uncertainty

* The draws made inside the two item generators and inside the kill-record application
  (S 0x4d61c0 pet experience / level-up) were not re-traced here.
* Whether the creature map visited by this loop uses exactly the same comparator instance
  as S 0x530600 was taken from the shared find calls on that map (S 0x4cf4bf, 0x4cf2c1);
  the iteration itself is a plain in-order successor walk.

---

## Ruling 2 — when mission progress runs, and its position in the death routine

**Winner:** `alpha-missions.md` §5.1 (correction 10) and its cross-spec note:
the progress code is gated **only by the remote flag**. `alpha-server-simulation.md` §6.2
step 6 ("online flag clear") agrees. `alpha-creatures-behaviour.md` §8 "(offline only)" is
**incomplete**, correct only in the sense "never on a client connected to a server".

### Gating

The single gate is the remote-flag test at S 0x4cf3ba / C 0x59639a; set → skip to the
per-creature loop (S 0x4cf424 → 0x4cf6a5, C 0x596404 → 0x596685). No controller, server or
local-player test precedes it on the death path. Server.exe constructs its world with the
flag clear (S 0x549cb7 pushes 0 for the argument stored to world +0xb4 at S 0x4c867c), so
the progress code runs **on the server**, and on an offline client. On a client connected
to a server it does not run, and since the completion byte then stays 0, that client also
makes no reward items (Ruling 1 step 4) — it receives them as section-9 gifts instead.

The progress test itself (S 0x4cf42a–0x4cf6a5): home-zone cell (+0x1b0, +0x1b4, each ÷ 8
toward zero) exists and its state +0x41 ≠ 2; victim flag 0x2000, or cell kind +0x34 = 5 with
victim byte +0x1d38 ≠ 0; then the threat map is walked in order and the first entry with
threat > 0.0 (constant S 0x558698) whose id resolves in the creature table to a creature
with controller 0 decides the update; entries failing either test are skipped. The update
is dispatched through the kind table (S 0x4cf513 / 0x4cf51a). It makes **no random draw**.

### Order within the death routine

Confirmed for both images:

1. health := 0, sound 27, death timer := 0 (S 0x4cf169–0x4cf1ac);
2. linked static-entity trigger (S 0x4cf1b3–0x4cf239);
3. **loot**: S 0x4d2ae0 called at **S 0x4cf24e** (C 0x59b180 at C 0x59622e);
4. pet health := 0 (S 0x4cf299–0x4cf2fc);
5. kill experience computation (S 0x4cf306–0x4cf3a5);
6. **mission progress** (gate S 0x4cf3ba);
7. per-creature loop with kill records and rewards (S 0x4cf6a5 onwards).

Therefore any draws made by the loot routine precede the reward draws of Ruling 1, and the
progress code contributes none in between.

### Residual uncertainty

* The kind-table targets for kinds 1–5 and 13 (state 2, completion byte set) were taken
  from the held contract's re-certification; only the kinds 7–12 path (S 0x4cf521–0x4cf537)
  was re-read here.
* The loot routine's own draw count is out of scope.

---

## Ruling 3 — "the player's region" for the daily mission pass

**Winner:** `alpha-missions.md` §2.1 (correction 1). This is a **clarification** of
`alpha-server-simulation.md` §3 step 2, not a contradiction: "the player's region" there
must be read as the region found by the ownership lookup below, not the region containing
the block position (block ÷ 16384).

### Behaviour

Per player in the pre-pass, only when the remote flag is clear (S 0x5328db, C 0x60cb1b):

1. The player's position (+0x10, two signed 64-bit components in 1/65,536 block) is reduced
   to block coordinates by signed 64-bit division by 65,536 (S 0x405640, toward zero),
   x and y separately.
2. The region-ownership lookup owner(x, y) of `alpha-worldgen-regions.md` §3.4 is called
   with those block coordinates (S 0x4febd0 called at S 0x532921; C 0x5cade0 at
   C 0x60cb61). It searches the region cells in the window around the position (bounds
   from (block ± 16384) ÷ 16384), compares against a noise-displaced position, keeps the
   nearest present cell, and yields its indices (rx, ry), or (−1, −1) when none is found.
3. The region object at (rx, ry) is fetched (S 0x406210 at S 0x532952; C 0x434a90 at
   C 0x60cb92); indices outside 0 … 1023 yield none.
4. If the region exists and its "missions generated" byte +0x15a18 is 0, the mission driver
   runs for (rx, ry) (S 0x50d8d0 at S 0x532977; C 0x5d9ad0 at C 0x60cbb7).

Near a region border the region tested is therefore the one owning the player's position,
which can differ from block ÷ 16384. The later map-cell discovery test of the same pre-pass
(S 0x53297c onwards) uses the cell of the player's zone (block ÷ 256 ÷ 8), not the owner
indices.

### Residual uncertainty

* The noise displacement and distance details inside S 0x4febd0 are deferred to
  `alpha-worldgen-regions.md` §3.4; only the call, the window shape, the (−1, −1) default
  and the use of its two outputs were re-read here.

---

## Evidence

Static instruction listings of both original images (the death routine S 0x4cea80 /
C 0x595a60, the step S 0x5322d0 / C 0x60c510, the helpers S 0x405640, S 0x406210,
S 0x4013d0, S 0x4e2320, S 0x4febd0, S 0x530600 and the world constructor S 0x4c8570 with
its caller S 0x549c50); float constants read from the Server.exe image bytes. Client and
server bodies are parallel in the regions cited (same branch displacements in the death
routine). Reconstruction sources were not consulted. Not verified at runtime.
