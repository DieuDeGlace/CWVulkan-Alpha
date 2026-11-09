# Alpha open points, batch 1: traps, the quest banner, the beam end

Clean-room behavioural specification. It closes three open points of earlier specs:

| Point | Spec it belongs to | Section here |
|---|---|---|
| **I-5** trap damage, range, timing, occupant table | `alpha-interaction.md` §6.4, §10 | §1 |
| **M-1** what shows the "QUEST COMPLETE!" banner | `alpha-missions.md` §8.4, §12 | §2 |
| **AP-3** trigger of the beam-end burst | `alpha-particles.md` §5.6, §12 | §3 |

It also corrects `alpha-audio.md` §6.4 (ids 52 and 53 are played, §1.5 here) and the list
named in `alpha-missions.md` §8.4 (§2.1 here). Addresses are citations into the original
client (`Cube.exe`) and server (`Server.exe`); both programs make the same decisions where
both are cited. Conventions are those of the cited specs: positions are 3 × i64 in 1/65536
block, a creature's position is the centre of its collision box
(`alpha-physics-camera.md` §0), "draw" is one C `rand()` call, *believed* marks a reading
that is not fully certain, *as read* marks behaviour that looks unintended but is what the
code does.

Sources read: server world step 0x5322d0 (static-entity part 0x533097–0x533bab, including
the jump-table bodies 0x5331da–0x5335e2 that the bulk listing omits), server hit resolution
0x4cfd50, server state setter 0x4d8c90, server level lookup 0x4d2340; client twins
0x60c510 (0x60d3b0–0x60dd57), 0x596d30, 0x5a58e0, 0x59a9b0; client frame update 0x488ee0
(0x48cf20–0x48cf49, 0x48fdc3–0x48fec1, 0x4904b0–0x49081b); client mission death branch
0x595a60 (0x596579); client beam block 0x615ecb–0x616b1e and its server twin around
0x53c799. Constants were read from the binaries' data sections.

---

## 1. Traps: static kinds 6, 7 and 8 (resolves I-5)

### 1.1 Who runs it

The trap rules are part of the static-entity pass of the world step (server 0x533097 …,
client 0x60d3b0 …; `alpha-server-simulation.md` §3 step 6). For every static of every
active zone, in vector order:

1. **Always**, on every world including an online client: timer (+0x34) += dt. (This
   settles the *inferred* note of `alpha-interaction.md` §6.3: online clients advance the
   timers themselves, so their door and trap animations run locally; only the state byte
   comes from the server.)
2. The occupant re-pin of §1.6 (also on every world).
3. **Only when the world is not remote** (world +0xb4 = 0: the server, or an offline
   client; gates at server 0x5336b5, client 0x60d8f5): the trap rules below.

An online client never changes a trap's state and never produces a trap hit; it sees the
server's section-6 records (state), section-4 records (sound) and section-2 records (hits
on itself).

### 1.2 Arming cycle (kinds 6, 7, 8 alike)

State 0 = retracted (spikes down, stomper raised), state ≠ 0 = extended (spikes up,
stomper down). The drawing rules of `alpha-interaction.md` §4.4 match this.

Each step, after the timer was advanced:

* If timer > **2000** (strictly) and state ≠ 0: set state 0 (the setter, §1.5). A
  section-6 record of the static is emitted.
* If timer > 2000 and state = 0: walk the world's creature list (world +0x4) in order; at
  the **first** creature with controller kind **0** (a player) whose squared distance to
  the trap is **< 900** (the fixed-point test of §1.4), set state 1 and emit a section-6
  record. No player in range: nothing; the trap stays retracted (its timer keeps growing).
* Otherwise (timer ≤ 2000): nothing.

Because the setter makes timer := max(1000 − timer, 0), and the timer is > 2000 when it
fires, every change resets the timer to **0**. So with a player within 30 blocks a trap
alternates **2 s extended, 2 s retracted** (plus up to one step each), for as long as the
zone is active. The retract does not depend on any player. The 30-block test is a **sphere**
(z included) around the trap's position (x, y = footprint centre, z = base).

Start phase: statics are created in state 1 (extended). World generation gives spike traps
(the only trap kind any traced generator places: `alpha-worldgen-structures.md` §1.10.6
step 4) a timer of d % 4000, so on the first active step about half of them retract at once
and the rest stay up until their timer passes 2000; this de-synchronises neighbouring
traps. Kinds 6 and 8 start with timer 0 (*believed*: no traced generator places them).

### 1.3 When an extended trap hurts

Right after §1.2, if state ≠ 0:

* **Kinds 6 and 7**: when ⌊(T + dt) / 100⌋ ≠ ⌊T / 100⌋, where T is the world's millisecond
  clock (W+0x8000bc, `vegetation-wind.md` §2) **after** this step's advance (both programs
  add dt to it at the start of the step: server 0x532438, client 0x60c678) and ÷ truncates.
  In words: once per step in which the clock passes a multiple of 100 ms — about **ten hit
  passes per second** at the normal 10–20 ms steps; never more than one per step.
* **Kind 8**: when timer = 0. The timer was advanced earlier in the same step, so it is 0
  only in the step in which the setter just extended the trap: **one hit pass per slam**.

The extend of §1.2 and a hit pass may happen in the same step. A trap that retracted this
step does not hurt (its state is already 0).

A hit pass walks the world's creature list (world +0x4) and hurts every creature that

* exists (list slot not empty),
* has squared distance to the trap **< 25** (the test of §1.4; a 5-block sphere around the
  trap's position), and
* has controller kind **≠ 1** — so players (0), town NPCs (3), pets (5) and inert objects
  such as bombs (6) are hurt; hostile monsters (1) are not.

Health ≤ 0 is filtered by the hit routine (step 1 of `alpha-creatures-behaviour.md` §5).

### 1.4 The distance test

Both tests (900 and 25) use the same fixed-point rule (server 0x402c50, 0x4056c0, 0x402d10;
client 0x42c7a0, 0x43b4a0, 0x42c860): with Δ = trap position − creature position in
1/65536 block, each of Δx², Δy², Δz² is computed in 64 bits and divided by 65536 with
truncation, the three are summed, and the creature is in range when the sum is **strictly
less** than *n*·65536 (*n* = 900 or 25). Equivalent to "squared distance in blocks < *n*"
up to the last 1/65536.

### 1.5 The hit and the sounds

For each creature *C* of a hit pass, in list order:

1. **Knock-back k**: v = position(*C*) − position(trap) in blocks (float), v.z := 0; if
   |v|² > 0, v := v/|v|; k := 20·v with k.z := **5**; for kind 8, k := 2·k. So
   **k = (20·ûx, 20·ûy, 5)** for kinds 6 and 7 and **(40·ûx, 40·ûy, 10)** for a stomp
   trap; a creature exactly above the centre gets (0, 0, 5) or (0, 0, 10).
2. **Level** *L* (server 0x4d2340, client 0x59a9b0): the map cell of the trap's (x, y)
   (block ÷ 2048, truncated) gives a location record (`alpha-missions.md` §0 "Location
   record of a cell"). If the record exists, its type (+0x18) is not 0 and the normalised
   location distance f(record, trap) ≤ 1 (`alpha-worldgen-regions.md` §7), *L* = the
   record's level (+0x24); otherwise *L* = 1. So a trap inside a dungeon or other typed
   location hurts at that location's level; anywhere else at level 1.
3. **Damage** *x* = **10 · 2^((L − 1)/4)** for kinds 6 and 7, **50 · 2^((L − 1)/4)** for
   kind 8 (constants 0.25, 10 and 5 at server 0x5586c8, 0x55873c, 0x55872c; client
   0x745d98, 0x745e74, 0x6fce24). It does **not** depend on the victim's level or
   armour, and is not random.
4. **Hit resolution** (server 0x4cfd50, client 0x596d30) with: target *C*; **no
   attacker**; amount *x*; critical 0; area flag 0, area value 0; hit position = the
   trap; knock-back k; magic flag = (kind = 6); attacker action 0; no projectile; "can be
   blocked" 1. With no attacker the routine takes its short path
   (`alpha-creatures-behaviour.md` §5 step 2): **one draw is consumed**, the target's hit
   flash (+0x1184) := 0.5, and blocking, shield absorb, immunity, armour/resistance, spirit
   effects, the impact sound, the hurt voice (critical is 0) and stun are all skipped. So
   the magic flag of kind 6 has no effect *as read*. Then the knock-back rule (step 11
   there: *C*'s velocity keeps its direction and takes length max(|velocity|, |k|) — the
   direction is *C*'s own, not k's, *as read*), the kind-1 buff multipliers (step 12), and
   a final hit record of **kind 0** (attacker id 0, target *C*, amount, critical 0,
   position = *C*'s position, knock-back k, area byte 0) appended to the step's hit list
   (section 2).
5. **Application** (server 0x4cea80 / client 0x595a60, `alpha-server-simulation.md` §6.1):
   an NPC or pet loses the amount at once (no threat is added: there is no attacker; a
   death by trap credits no mission and gives no kill reward, `alpha-missions.md` §5.1); a
   player on the server is relayed in section 2 and its own client applies the record
   (`alpha-net-packets.md` §5.1); offline the local player is damaged directly.

**Sounds** (the state setter, server 0x4d8c90, client 0x5a58e0, appends a 24-byte sound
record to the step's sound list, section 4, at the static's position, volume 1, pitch 1,
only when the state actually changes):

| Kind | Extending (state → 1) | Retracting (state → 0) |
|---|---|---|
| 6 unseen trap | **53** `fire-trap.wav` | none |
| 7 spike trap | **52** `spike-trap.wav` | none |
| 8 stomp trap | 54 | 54 |

This **corrects `alpha-audio.md` §6.4**: ids 52 and 53 are not unplayed; the setter selects
them by kind (the switch on the static's kind, not a literal at a call site, is why the
earlier search missed them). Id 52 is heard every time a spike trap shoots up. Id 53 is
reachable only if some static of kind 6 exists (§1.2). A trap hit itself makes **no**
sound.

### 1.6 Occupant table (the per-kind jump table of I-5)

The step's occupant check (`alpha-interaction.md` §6.4 first bullet) dispatches on the
static's kind through two 54-entry byte tables (kinds 16 … 69; server 0x548964 and
0x5489a8, client 0x622bdc and its partner). Only five kinds have their own case:

| Kinds | Case |
|---|---|
| 16 stone stool, 18 bench | **seat** |
| 19 bed, 68 beach towel, 69 wood mat | **bed** |
| every other kind | none |

For an occupied static whose occupant exists and has hit-stun (+0x12c) ≤ 0:

1. **Re-pin** (first table, every world): occupant position := static position +
   (0, 0, occupant height/2 + scale.z); occupant velocity (+0x34) := 0; yaw :=
   (rotation + 2)·90 for a seat, rotation·90 for a bed; action := **83** (seat) or **84**
   (bed).
2. Free test (not remote only, §1.1): free the static (occupant := 0, section-6 record)
   when the occupant is more than 4 blocks away (squared > 16), or its +0x40 vector is
   non-zero, or its action is neither 83 nor 84. After step 1 only the +0x40 test can
   fire for kinds 16, 18, 19, 68, 69.
3. Otherwise **re-pin again** (second table) without touching the action.

An occupant that no longer exists or has hit-stun > 0 frees the static at once (not
remote only). So a sitting creature that is hit is thrown off its seat.

### 1.7 Worked example

A spike trap (kind 7, scale 10 × 10 × 10) stands in a level-5 dungeon at (X + 5, Y + 5,
Z + 1), Z being the floor's top block, inside that dungeon's location (f ≤ 1, *L* = 5).
Its timer starts at 3127 (d % 4000), state 1.

1. First active step: timer 3127 + 16 > 2000, state 1 → retract: state 0, timer
   max(1000 − 3143, 0) = 0, no sound, section-6 record. Clients animate the spikes down
   over 1 s.
2. 2001+ ms later a player (height 1.8) stands on the floor 3 blocks east of the trap
   centre: centre z = Z + 1 + 0.01 + 0.9. Δ = (3, 0, 0.91), squared distance 9.83 < 900 →
   extend: state 1, timer 0, sound 52 at the trap, section-6 record; the spikes rise in
   0.1 s (closing runs ten times faster for kinds 7 and 8, `alpha-interaction.md` §6.3).
3. Hits: 9.83 < 25, controller 0 → hurt. *x* = 10 · 2^(4/4) = **20** per pass, k =
   (20, 0, 5). With 16 ms steps the clock passes a multiple of 100 ms in about every sixth
   step, so a player who stays for the whole 2 s takes about **20 hits = 400 damage**
   (less if the knock-back carries it out of the 5-block sphere, which at |k| ≈ 20.6
   blocks/s it usually does after the first hit).
4. At timer > 2000: retract (silent); 2 s later it extends again if a player is still
   within 30 blocks.

A **stomp trap** in the same place would deal one hit of 50 · 2 = **100** with k =
(40, 0, 10) in the step it slams (sound 54), and sound 54 again when it rises.

A trap outside every typed location (*L* = 1) deals 10 (spike) or 50 (stomp) per hit.

---

## 2. The "QUEST COMPLETE!" banner (resolves M-1)

### 2.1 Which list the frame update walks

`alpha-missions.md` §8.4 names the list G+0x800638. That is a misreading of the frame
update 0x488ee0; the lists are:

* The game keeps one **received world update** at G+0x800630, laid out like the step's
  output set (section lists at +0x00 hits, +0x08 sounds, +0x10 particles, … +0x30 statics,
  … **+0x60 section 13**). The network code fills it; the section-13 reader of
  `alpha-missions.md` §6.3 (0x48fe11) walks **G+0x800690** = G+0x800630 + 0x60.
* The frame update runs the local world step (0x48cf2e / 0x48cf49) with a frame-local
  output set (on the stack, call it *O*). It then **splices** each received section into
  *O*: G+0x800638 (received sounds) into *O*'s sound list, G+0x800640 (particles) into
  *O*+0x10, …, **G+0x800690 (received section 13) into *O*+0x60** (0x490580–0x4905b8), and
  empties the received lists (0x4905c9 …).
* The banner loop (0x490690–0x4907dc) walks ***O*+0x60**: the frame's **section-13
  records**, local ones first, then received ones.

So the "event records" of §8.4 are section-13 mission records (layout
`alpha-missions.md` §6.1). Their "kind byte +0x25" is the record's **state** (byte 37), and
the "+0x18 ≠ 0" test is the record's **mission kind** (bytes 24–27).

### 2.2 The rule

For each section-13 record *R* of the frame, in list order, with *R*.kind ≠ 0:

* *R*.state = 1: the creature whose id is in bytes 8–15 is looked up and, if it exists, its
  speech bubble is opened (0x4882e0). Every builder writes 0 there
  (`alpha-missions.md` §6.1), so *believed* nothing happens (open point Q-2).
* ***R*.state = 2**: L = the client's location record of cell (*R*.cx, *R*.cy) (0x487da0;
  none when that region is not loaded). If L exists:
  1. the banner `questtag` (G+0x800868, at (W/2, H − 300), `alpha-ui-screens.md`) plays its
     plasma sequence **`shine`** from the start (0x636810; `alpha-ui-plasma.md` §6.5);
  2. its child **`objective`** gets L's objective text (0x477fa0, `alpha-missions.md`
     §8.3: the dictionary entry `objective:<key>` with that mission's name, creature and
     place). The words "QUEST COMPLETE!" are part of the banner's graphics in
     `quest-tag.plx`, not set by code;
  3. interface sound **30** (`missioncomplete.wav`) at the listener position (G+0x140),
     volume 1, pitch 1 (0x484350).

Records with kind 0 (a daily reset, `alpha-missions.md` §2.3 step 2) and state 0 (a new
mission) show nothing. There is no de-duplication: several state-2 records in one frame
restart the sequence each time (the last record's text stays) and play sound 30 once per
record.

**Duration**: the banner has no timer in code; what is seen and for how long is the
`shine` sequence's keyframes in `quest-tag.plx` (open point Q-1).

### 2.3 Where state-2 records come from

| World | Source | Who sees the banner |
|---|---|---|
| Offline | the mission death branch of the client's hit routine (0x595a60, append at 0x596579 to *O*+0x60) when a qualifying kill completes a mission (`alpha-missions.md` §5.2: kinds 1–5, 13 on the kill; counting kinds on the last kill) | the local player, in the frame of the kill |
| Server | server 0x4cea80 at the same point; section 13 is broadcast to **every** client, unfiltered (`alpha-missions.md` §6.2) | **every connected player whose client has that region loaded**, including players who took no part (rewards still go only to the threat-map players) |
| Server, packet 12 | the answer to a map-region request (server 0x4272d0) sends all 64 cells of the region, each with its current kind and state | the requesting player: *believed* one banner and one sound 30 per cell whose mission is complete and not yet cleared by the daily reset, each time the client asks for that region (Q-3) |

So the banner fires on **mission completion by kill**, not on the reward: the reward items
(section 9) travel separately and show nothing of their own here. A remote client never
builds section-13 records itself (`alpha-missions.md` §9).

### 2.4 Example

Offline, the player kills the troll of a kind-1 hunt in cell (4097, 4100)
(`alpha-missions.md` §10.6). The death branch sets state 2 and appends the record to the
step's section 13. In the same frame the banner loop finds kind 1, state 2, cell
(4097, 4100) with its region loaded: `shine` restarts, `objective` shows that hunt's
objective text, sound 30 plays. Online, the server does the same and broadcasts the
record; each client with region (512, 512) loaded shows the banner when the update
arrives.

---

## 3. The beam end (resolves AP-3)

### 3.1 The beam tick (client 0x615ecb …, server twin)

For a creature *A* whose action is **28** or **95** (the block also serves action 104,
whose differences are noted), let *t* = action time (+0x6c), dt the step, *D* = the
action's duration (0x43caa0 with −1: **0** for action 28; 1000 / (*A*'s attack-speed
factor × +0x17c) for action 95, `alpha-creatures-behaviour.md` §3). A **tick** happens in
a step when

* ⌊(*t* + dt − *D*)/100⌋ ≠ ⌊(*t* − *D*)/100⌋ (÷ truncating toward zero), and
* *t* + dt > *D*.

That is, once every 100 ms of action time after the wind-up. Because truncation maps
−99 … 99 to 0, the first tick comes when *t* − *D* passes **100**, not 0 (*as read*).

A tick:

1. **Direction** û = *A*'s aim (record field 26, object +0x160) normalised (left as is
   when its length is 0).
2. **Origin** S = *A*'s position, or, for a creature with appearance flag 4 (+0x7e bit 2),
   that position plus a model-transform offset (0x446bb0; *believed* the head or hand
   point).
3. **Length** *L* = the distance from S along û to the first solid block, at most 200
   blocks (0x5a35d0 with 200); action 104 uses *L* = 10.
4. **Clear** *A*'s per-tick hit set (+0x11ac) and a second set (+0x11b4).
5. (A water mage, class 3 specialization 1, also queues a beam visual from S to S + *L*·û;
   not part of this point.)
6. **March**: for *i* = 0, 1, 2, … while *i* < *L*: the point P = S + *i*·û (P starts at
   S and is advanced by û, converted to fixed point, after each *i*). At each P every
   creature *C* of the world list is tested: *C* exists, *C* ≠ *A*, *C*.id ≠ *A*+0x11c0,
   *C*.health > 0, *C* not yet in the hit set, *C*.dodge (+0x128) = 0, horizontal squared
   distance P↔*C* < (0.5·*C*.sx + 1)² (+ *i* added to the radius for action 104), and
   |P.z − *C*.z| < 0.5·*C*.sz + 1. A *C* that passes is **added to the hit set first**;
   then, only if *A* and *C* are hostile to each other (0x5988a0), the hit routine runs and
   the tick's "hit" flag is set.
7. If the hit flag is set: *A*'s combo (+0x70) += 1 and +0x74 := 0.

### 3.2 The burst

After the march (client 0x6169d9, server 0x53c799): **if the hit set is empty and the
action is not 104**, write the burst of `alpha-particles.md` §5.6:

* position = *A*'s **position** (+0x10, not S) + *L*·û — the ray's end shifted by
  (position − S) for creatures with appearance flag 4 (*as read*);
* v₀ = −û (1 block/s back toward *A*), spread 3;
* colour (1, 0.2, 0.5) as recorded (style 1, so seen in fire colours), size 0.1, count 5,
  style 1;
* appended to the step's particle list (section 3).

Then *A*'s combo := 0 and its action time is rescaled: *t* := trunc(*t* · s_old / s_new),
where s is the attack-speed routine (0x447700) evaluated before and after the combo reset
(*believed*: s depends on the combo; if it does not, *t* is unchanged).

So:

* **The list examined** is *A*'s per-tick hit set (+0x11ac). It is **empty** when no
  creature at all — friend or foe — entered the beam's cylinder during this tick. A
  friendly creature standing in the beam is added to the set (step 6) without being hurt
  and **suppresses** the burst (*as read*).
* **Per tick**, not per block step and not once per action: up to ten bursts per second
  while the beam hits nothing, at the wall (or 200 blocks out) where the ray stopped. A
  zero-length ray (*A* against a wall) puts the burst at *A*'s position.
* A miss also **resets the combo**; a tick that touched a creature raises it by one.

Who runs it: the owner rule of `alpha-server-simulation.md` §1 (the client for its own
player; the server for NPCs), as for every section-3 producer of `alpha-particles.md` §5.

### 3.3 Example

A monster in action 95 (*D* = 1000 ms at speed 1) beams at a wall 12.4 blocks away; no
creature is near the line. Ticks come at *t* ≈ 1100, 1200, 1300 … ms. Each tick marches
P through *i* = 0 … 12 (13 points), finds nothing, and writes five fire cubes at
position + 12.4·û drifting back at 1 block/s ± 3, then sets the combo to 0. When the
player steps into the beam 6 blocks out, the tick at *i* = 6 adds the player to the set,
the hit routine runs, the combo rises, and that tick writes no burst.

---

## 4. Remaining open points

| Id | Question | Suggested game check |
|---|---|---|
| **Q-1** | Length and look of the banner's `shine` sequence (keyframes of `questtag` in `quest-tag.plx`). | Parse the sequence keys of `quest-tag.plx` (`alpha-ui-plasma.md` §6.5), or record the screen while completing a hunt offline and time the banner. |
| **Q-2** | Whether a state-1 record can ever open a speech bubble (bytes 8–15 are 0; is there a creature with id 0?). | Discover a mission offline and watch for a bubble over any creature. |
| **Q-3** | Whether opening the world map online re-shows the banner for every completed, not-yet-reset mission of the region (packet-12 answer). | On a server, complete a hunt, reconnect, open the map in that region; watch for the banner and count sound 30. |
| **Q-4** | Whether any generator places static kinds 6 (unseen trap) or 8 (stomp trap); no traced structure routine does. | Search the remaining structure builders for inserts of kinds 6 and 8, or dump a dungeon zone's static vector (`alpha-worldgen-structures.md` worked dumps). |
| **Q-5** | The attack-speed routine 0x447700 and whether the beam-miss rescale of action time (§3.2) changes anything. | Read 0x447700; in game, beam at a wall with a high combo and see whether the beam's timing jumps. |
| **Q-6** | The model offset of the beam origin for appearance flag 4 (0x446bb0). | Read 0x446bb0 with the model specification; in game compare beam start and burst point on such a creature. |
| **Q-7** | Trap numbers in play: confirm 20 damage per spike hit at dungeon level 5 and about ten hits per second. | Offline, stand on a spike trap in a dungeon of known level with a test character and log health per frame. |

`alpha-interaction.md` I-5, `alpha-missions.md` M-1 and `alpha-particles.md` AP-3 can be
marked answered by §1, §2 and §3 here; `alpha-server-simulation.md` SS-2 is answered for
the trap and seat kinds.
