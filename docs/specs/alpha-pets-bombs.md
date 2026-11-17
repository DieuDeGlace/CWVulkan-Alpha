# Alpha bombs and pets

Status: FINAL / STABLE / HELD

Behavioural specification of two things Cube World **Alpha** builds out of ordinary
creatures: the **bomb** a player places by using a Bomb item (a short-lived creature of type
144 that burns down and explodes), and the **pet** (a creature tamed with pet food, carried
as a pet item in the Pet slot, re-created next to its owner, following and fighting for it).
It covers using a bomb, the bomb creature, its fuse, the explosion's damage to creatures,
the healing branch of the "use an item" request, taming, the pet item, the per-step pet
pass, pet behaviour, death and return, pet experience, the T key ("pet reset"), how pets
look on the network and the `/namepet` command.

Written from the Alpha decompile (`disassembled alpha cube world/decompiled/server` and
`…/client`), the per-instruction listings, and fresh disassembly of the two binaries where
the listing has gaps (the step routine server 0x5322d0 / client 0x60c510 has no C output).
No code was copied. Routines are cited as "server 0x…" (`Server.exe`) or "client 0x…"
(`Cube.exe`); offsets are given for cross-checking only. Readings marked *believed* rest on
partial evidence; *as read* marks behaviour that looks unintended but is what the code does.

Bomb placement, fuse, explosion damage, the kind-1 healing branch, taming, the pet item,
the pet pass, the two pet behaviours, death and return, pet experience, packet 6 kind 8 and
`/namepet` are specified. Re-certified against the Alpha binaries on 2026-11-16 (corrections
listed in the final section). Open points are in §19.

This document **adds to** and does not repeat:

* `alpha-server-simulation.md` — the step and its numbered stages (§3), creature ids
  (§5.3), player pets (§5.6, refined in §9 here), the hit routine (§6.1), NPC death (§6.2),
  packet 6 (§8), online versus offline (§10), the event set (§11);
* `alpha-block-edits.md` §8 — the bomb's particle burst, the natural-block removal, the
  explosion sound and light flood (steps 1–3); §4 here is its step 4;
* `alpha-creatures-behaviour.md` — derived stats (§4), hit resolution (§5), kill experience
  (§8), the behaviour layer (§11; corrected in §18 here), sound ids (§13);
* `alpha-travel.md` §5 — the Pet slot, rideable types, mounting, riding energy and the pet
  under its rider;
* `alpha-net-packets.md` — the creature record (§2.1), the hit record (§5.1), packet 6
  (§4.1);
* `alpha-loot-rolls.md` §4 (the 45 tameable types), `alpha-item-names.md` §5 (pet and pet
  food names), `alpha-talk-vendors.md` (pet-food talk; the vendor that sells Bombs),
  `alpha-skills.md` §2.1, §5.4 (pet taming and pet riding skills), `alpha-ui-screens.md`
  §5.3 (pet bars), §8 (Pet slot rules, pet tooltip), §9 (chat commands), §11 (keys),
  `alpha-creatures-models.md` §4 (appearance generator; type 144 = `bomb1`);
* `alpha-attack-routine.md` — the full decision and draw order of hit resolution (the
  bomb is its call site 2).

---

## 0. Conventions

* As in `alpha-travel.md` §0: "obj +x" is an object offset (record offset + 0x10); record
  fields are numbered as in `alpha-net-packets.md` §2.1. dt is the step's elapsed time in
  integer milliseconds. "draw" = one C `rand()` (0..32767).
* Inside the step, client address = server address + 0xDA240
  (`alpha-server-simulation.md` §1); only the server address is given there. Stage numbers
  ("stage 9") are those of `alpha-server-simulation.md` §3.
* "Offline" = a client world without a server; "server"; "online client" — as in
  `alpha-block-edits.md` §0. Rules said to run "on the server or offline" are skipped on an
  online client.
* Skill factor v(*n*) = 1 − 1/(0.1·*n* + 1) for *n* ≥ 1, 0 otherwise (`alpha-travel.md` §0).
* Level curve: *g*(*L*) = 3·(1 − 1/((*L* − 1)·0.05 + 1)), *P*(*L*) = 2^*g*(*L*)
  (`alpha-creatures-behaviour.md` §4). Test values: *P*(1) = 1, *P*(3) = 1.20808,
  *P*(5) = 1.41421, *P*(10) = 1.90664.

### 0.1 Fields used here

| Where | Meaning |
|---|---|
| record 0x50 (field 7) | controller kind: 0 player, 1 hostile, 3 NPC, **5 pet**, **6 inert (bombs)** |
| record 0x54 (field 8) | creature type (144 = bomb) |
| record 0x58, 0x5c (fields 9, 10) | action and action time; for a **dead** creature the action time is the **death timer** |
| record 0x60 (field 11) | combo counter; also the taming bite counter (§7.3) |
| record 0x114 (field 14) | mode flags; 0x10 gliding, 0x40 running |
| record 0x118 (field 15) | dodge time (a dodging creature is not hit by a bomb, §4.2) |
| record 0x15c (field 27) | health |
| record 0x168 (field 30) | stat multipliers: health, attack speed, damage, armour, resistance |
| record 0x180, 0x184 (fields 33, 34) | level, experience |
| record 0x188 (field 35) | **owner id** of a pet (0 = none) |
| record 0x1d8 (field 43) | the item in hand (drunk or eaten during actions 80, 81) |
| record 0x1010 (field 44, slot 12) | the **Pet slot**: pet item (type 19) or pet food (type 20) |
| record 0x1128 + 4·*i* (field 46) | skill levels; skill 0 = **pet taming**, skill 1 = pet riding |
| record 0x1158 (field 45) | name (16 bytes) |
| obj +0x1178 | buff list (kind 7 = taming progress, kind 8 = pacified, §7) |
| obj +0x11ac | a bomb's "already hit" set of creature ids (§4.2) |
| obj +0x11c0 | the creature's mount id (u64; `alpha-creatures-models.md`); always 0 for a bomb (§4.2) |
| obj +0x11c8 | the creature's pet id (u64, not in the record) |
| obj +0x11d0 | the creature's current target id |
| obj +0x13a4 | threat map |
| obj +0x13ac | per-target hit counters: how often this creature hit each target (`alpha-server-simulation.md` §6.1) |
| obj +0x13e4 | AI behaviour object |
| obj +0x1d44, +0x1d48 | a player's copies of its taming-skill level and of its Pet slot item, used to notice changes (§9.2) |

---

# Part A — Bombs

## 1. The Bomb item

A Bomb is the consumable item **(type 1, subtype 7)** (`alpha-item-names.md` §1, "Bomb").
Alpha's item vendor (class 128) offers one with a 50 % chance (`alpha-talk-vendors.md`).
Its level is the item level (item +0x10, i16).

## 2. Using a bomb (client 0x4a2780)

Using a consumable (the quick item Q, or the
inventory) runs one routine. It acts only on items of **type 1** and only while the
player's hit-stun counter (field 16) is below 1. For a **Bomb**:

1. a packet-6 record of **kind 1** is built, holding a copy of the item, and put in the
   local player's request queue (obj +0x130c; `alpha-server-simulation.md` §8);
2. the stack count drops by one; an emptied stack is cleared.

Nothing else happens on the client: no action is started, no sound is played, the player
is not slowed. Online the queue goes out as packet 6; offline the client's own step handles
it (§3). (Other type-1 items start a drinking or eating action instead, §6.)

## 3. The bomb creature (packet 6 kind 1, server 0x536e64)

The request is handled at the start of the requesting player's turn in stage 12, on the
server and offline only (an online client never reaches it, its queue is sent instead). If
the item is type 1 subtype 7:

| Property | Value |
|---|---|
| id | **(the smallest id in use, or 0 if all ids are positive) − 1**, as a signed 64-bit number: the first bomb or pet of a world is −1, the next −2 (`alpha-server-simulation.md` §5.3) |
| position | the player's position, copied exactly (field 0) |
| controller | **6** |
| type | **144** |
| level | the item level |
| owner (field 35) | **none** — the placer is not recorded anywhere |
| appearance | the appearance generator for type 144 with no choice record (server 0x40a840; `alpha-creatures-models.md` §4: body `bomb1`, scale 0.8, box (0.8, 0.8, 0.8), body offset (0, 0, 1)) |
| everything else | the new-creature defaults of the creature constructor (server 0x406400, record defaults server 0x407020): health **500**, multipliers 100, 1, 1, 1, 1, tier 0, no class, no equipment, no behaviour |

Order of operations: the id is chosen (one scan of the creature table), the creature is made
with that id and inserted in the table, its position is copied, then level, type and
controller are written, then the appearance generator runs. **Draws**: the generator,
called without a choice record, makes its five choice-record draws
(`alpha-creatures-models.md` §4.1); the type-144 case makes no further draw. Placing a bomb
therefore consumes exactly five draws. The request is not checked against the player's
inventory.

The bomb has **no behaviour object** (no AI), its action clock does not run (controller 6
is skipped by the action-time advance, server 0x53e2a8), and it never despawns while a
player is within 220 blocks (`alpha-server-simulation.md` §5.4; its despawn radius is the
default 200).

### 3.1 The fuse

On each of the bomb's turns in stage 12 (server 0x537856..0x537987; the type test is
server 0x53793b = client 0x611b7b):

1. health is clamped to the maximum health (this clamp applies to every creature; a
   creature whose health is then ≤ 0 does nothing more in its turn) — for the bomb
   **maxHealth = 100 · *P*(*L*)** (NPC formula with tier 0 and no class or items,
   `alpha-creatures-behaviour.md` §4.1), so the initial 500 drops to 100 · *P*(*L*) on its
   first turn;
2. health −= **0.025 · dt**, floored at 0 (server and offline only; an online client takes
   the bomb's health from packet 0).

The burn is single-precision: dt is converted to a float and multiplied by 0.025, and the
product is subtracted from the float health.

The fuse therefore burns for about **4000 · *P*(*L*) ms**: 4.0 s at level 1, 5.66 s at
level 5, 7.63 s at level 10 *as read* (higher-level bombs burn longer). When its health
reaches 0 nothing happens in that turn; the explosion comes in **stage 10 of the next
step** (§4), because stage 10 runs before stage 12.

**First fuse turn.** The creature table is ordered by the **signed** 64-bit id, ascending
(the table's lookup compares the high 32 bits signed, server 0x530600;
`alpha-server-simulation.md` §0). A new bomb takes the smallest id in the table, so it is
inserted **before** every existing creature. Stage 12 walks the table in order, and the
requesting player's turn is already in progress. The new bomb is therefore **not** visited
in the step that placed it: its first fuse turn, and its 500 → maximum clamp, happen in
stage 12 of the **next** step. In later steps bombs and pets (negative ids) are visited
before every player in each per-creature pass.

### 3.2 Movement

The bomb is a creature with a box, so the ordinary creature physics of stage 12
(`alpha-physics-camera.md`) moves it: it starts with zero velocity, falls under gravity if
the player was in the air, and lies where it lands. It has no intent, so it never walks.
A hit's knock-back can push it (§4.2) (*believed*: no exclusion for controller 6 was seen
in the physics stage).

### 3.3 What the client shows

* The bomb is drawn as its model (`bomb1`). While a creature of type 144 is drawn (client
  0x4b409d, in the creature draw routine 0x4ac260) the client adds a **temporary point
  light** to its short-lived light list (G+0x800748): at the bomb's position, colour
  **(1.0, 0.8, 0.5)**, radius **16**, lifetime **800 ms**. The list ages every entry by the
  frame time and drops entries older than their lifetime (client 0x4963cb). The light is
  added again on every frame, so a bomb glows steadily, with a trail while it falls.
* There is no fuse sound and no fuse particle.

## 4. The explosion

The dead-creature pass (stage 10, server 0x534998..0x535266) visits, in table order
(ascending signed id), every creature that is not a player (controller ≠ 0) and has health
≤ 0. A creature whose death timer is **still 0** (its first dead step) and whose type is
**144** explodes; then, like every dead NPC, its timer grows by dt, and once the timer
reaches 1000 ms it is erased from every creature's threat map and per-target hit counters
and removed from the world (`alpha-server-simulation.md` §6.3).
Since the bomb's action clock never runs, its timer is still 0 on its first dead step,
whether it died by the fuse or by a hit (a hit also sets the timer to 0).

Stage 10 is skipped on an online client (§14.1), so only the server (or the offline client)
explodes bombs.

### 4.1 Blocks, particles, sound

Steps 1–3 of `alpha-block-edits.md` §8 (the burst, the natural blocks inside the radius-8
sphere, the sound only if blocks were removed, the light flood on the server).

### 4.2 Damage to creatures (step 4)

With **B** the bomb and **P** its position, for every creature X in the world, in table
order (ascending signed id):

1. **Who can be hit**, tested in this order: X exists and is not B; the hostility test of
   the bomb (server 0x4d18c0) passes — it always does because B has controller 6 — so
   **players, hostile and friendly NPCs, merchants, pets, other live bombs and the player
   who placed it** can all be hit; X's id differs from B's mount id (obj +0x11c0, which is
   never set for a bomb, so in effect only a creature with id 0 is excluded); X has health
   > 0; X's dodge time (field 15) is **0** (a creature in the middle of a dodge is not
   hit); X is not yet in B's "already hit" set.
2. **Range** (horizontal, then vertical; both required): with Δ = X's position − **P**,
   * horizontal: Δx² + Δy² < (**8 + 0.7 · bₓ**)², bₓ = X's box width (appearance box x).
     The test is made in fixed point: each of Δx², Δy² (Δ in 1/65,536 block) is divided
     by 65,536 with signed 64-bit truncating division, the two are summed, and the sum
     must be strictly below the single-precision value (8 + 0.7 · bₓ)² × 65,536 truncated
     to an integer;
   * vertical: |Δz| < **8 + 0.8 + 0.5 · b_z** in blocks (single precision), i.e. B's box
     height (0.8) + half X's box height + 8.

   So the reach is a cylinder slightly wider than the block-removal sphere.
3. **Knock-back direction**: the vector from **P** to X in blocks (single precision) with
   its z set to 0; when its squared length exceeds 0.01 it is normalised. No draw.
4. **Critical**: one draw; critical when draw mod 4 = 0 (one chance in four). This draw is
   made only for a creature that passed steps 1 and 2.
5. **Knock-back**: the direction of step 3 with z := 0.25, then all × 10:
   **k** = (10·ûₓ, 10·û_y, 2.5). A creature standing (almost) on the bomb gets
   k ≈ (0, 0, 2.5).
6. **Damage**: *x* = **25 · d(B)**, × 2 when critical, where d(B) is the bomb's damage stat
   (server 0x408f70 = `alpha-creatures-behaviour.md` §4.2 with no weapons): d(B) = damage
   multiplier (1) · *P*(*L*) · 2^(0.25 · tier) = *P*(*L*). Level 1: 25 (50); level 10: 47.67
   (95.33).
7. X's id is added to B's "already hit" set (B is processed only once anyway).
8. The hit goes through **hit resolution** (server 0x4cfd50 = client 0x596d30; call site 2
   of `alpha-attack-routine.md` §6) with target X, attacker B, amount *x*, flag A = the
   critical flag, flag B (record byte 0x44) **set**, stun weight W = **0**, knock-back
   vector **k**, mitigation selector **set** (resistance), kind K = 0 and the blockable
   flag **set**. Consequences of those inputs (draws as in `alpha-attack-routine.md` §3–4):
   * a controller-1 target with mode flag 0x80 or contact flag 0x20 is immune: a kind-3
     record with the full knock-back and sound 24 (one draw), and nothing more; otherwise a
     blocking target blocks it: a kind-1 record with half knock-back and sound 24 (one
     draw), and nothing more; otherwise kind-6 shield buffs absorb up to their sum (the
     factor *c* = 1 − min(1, field 11 / 50) = 1 because B's field 11 (obj +0x70) is 0): a
     kind-5 record with half knock-back and
     sound 91 (one draw), and only the remainder, if positive, continues;
   * the target's **resistance** (not armour) is subtracted:
     *x* := max(0, *x* − *c* · resistance(X)), *c* = 1;
   * no per-weapon or item-extension effects (B's fraction field 11 / 50 is 0 and it has
     no weapons);
   * impact sound **39** (`fire-hit.wav`) at pitch **1.5** at the target (the resistance-path
     sound of an attacker of specialization 0); no draw;
   * a critical hit on a target whose controller is not 6 also makes **one draw** (pitch)
     and plays the target's hurt voice for its type (the draw is spent even when the type
     has no voice);
   * **stun**: when X's hit-stun counter (field 16) is below −3000 (the usual state of a
     creature not hit recently, since it falls by dt every step) and X's controller is not
     6, the stun roll makes **one draw**. With W = 0 the chance is 0 — **except** that a
     target whose own action is **84** sets W to 1, so such a target can be stunned by a
     bomb (chance and effects as in `alpha-attack-routine.md` M10);
   * the final record is a **kind-0 hit**: attacker = B's id, target = X's id, amount *x*
     (multiplied by the value of each kind-1 buff of X), critical flag at byte 0x14,
     position = X's position, knock-back **k** (z = 10 when stunned), byte 0x44 = 1
     (`alpha-net-packets.md` §5.1). The routine does not change X's velocity itself.

   **Draws per hit creature**, in order: the critical draw (step 4); then either one
   immune/block draw, or [one absorb draw if shields absorb, then, if damage remains]
   the hurt-voice draw (critical only) and the stun draw (when gated); then any draws of
   the hit routine (death of an NPC: loot and the like, `alpha-server-simulation.md` §6.2).
9. **Where the hit goes.** Hit resolution appends the record to the step's hit list (the
   list that is world-update **section 2**) and, on the server or offline, applies it with
   the hit routine (`alpha-server-simulation.md` §6.1): an NPC target loses health on the
   spot (and may die, §6.2 there: sound 27, loot, kill records); a player target on the
   server is relayed in section 2 and its own client applies it; offline the local player
   is damaged directly. *As read*, for a player target on the server the hit routine
   appends the same record to section 2 a second time (this affects every hit the step
   makes on a player, not only bombs; open point **PB-4**).

### 4.3 Threat and credit

The hit routine treats B as the attacker: B gains threat in X's threat map (0.01 + amount ×
2, `alpha-server-simulation.md` §6.1) and the two remember each other as last
attacker/target. B is removed 1000 ms later and its id is then erased from every threat
map and every per-target hit-counter map. Consequences, *as read*:

* the player who placed the bomb gets **no credit**: kill experience goes only to players
  already in the victim's threat map (`alpha-creatures-behaviour.md` §8); a monster
  killed outright by a bomb nobody else touched gives no experience and no kill record;
* a hit NPC that survives may choose the (dead) bomb as its highest-threat target until it
  is removed.

### 4.4 Chains

A live bomb inside another bomb's reach is hit like any creature. Its health drops by the
damage (after its own resistance); if it reaches 0 it "dies" through the hit routine
(sound 27, no loot, no experience — it is not hostile) and its death timer is 0. When it
explodes depends on table order, because the explosion happens inside the stage-10 walk.
A bomb with a **larger** id than the exploding one (an older bomb, since each new bomb
takes a smaller id) has not yet been visited by that walk, so it explodes **later in the
same pass**. A bomb with a smaller id (a newer bomb) has already been passed, so it
explodes in the next step's stage 10. A bomb that survives the hit simply has a shorter
fuse. Every exploding bomb has its own "already hit" set, so a creature in range of two
bombs is hit by both.

## 5. Worked examples (bombs, dt = 16 ms)

**A. A level-1 bomb.** Player 1 (level 5, box (1.2, 1.2, 2.7)) uses a Bomb at
(8,398,402.5, 8,397,472.5, 65.0) blocks on day 5. The world holds players 1 and 2 and the
pet −1, so the bomb's id is **−2** (`fe ff ff ff ff ff ff ff` on the wire).

* Placing it spends five draws (the appearance generator). Its first turn, in the step
  after placement, clamps health 500 → 100 and burns 0.4 → 99.6; after about 250 turns
  (≈ 4.0 s; single-precision rounding of the repeated 0.4 subtraction may add one turn)
  health is 0; the next step's stage 10 explodes it.
* Player 1 has walked 5 blocks east (Δ = (5, 0, 0)): 25 < (8 + 0.84)² = 78.1 and
  0 < 8 + 0.8 + 1.35 = 10.15 → hit. Draw mod 4 = 2 → not critical: *x* = 25. (Player 1's
  hit-stun counter is far below −3000, so the stun roll spends a further draw, with chance
  0.)
  Resistance of a level-5 player with no armour: *P*(5) · 2 · 1 = 2.83 → **22.17** damage.
  k = (10, 0, 2.5). Sound 39 at pitch 1.5 at the player. The record goes to section 2 and
  player 1's client subtracts 22.17 health and adds the knock-back.
* A goblin (box 0.96 wide) 8.6 blocks north: limit 8 + 0.672 = 8.672 > 8.6 → hit; at 8.7
  blocks it would be missed. The goblin is damaged on the server; nobody gets experience
  if it dies, unless a player had already hit it.
* A wolf that was dodging at that moment (dodge time 400) is not hit at all.
* The crater, particles and explosion sound: `alpha-block-edits.md` §13.1.

**B. A level-10 bomb.** maxHealth = 190.66 → fuse **7626.6 ms** (477 turns). Damage 47.67,
critical 95.33.

**C. Two bombs.** Bomb −3 (level 1, 60 health left) lies 3 blocks from bomb −2 when −2
explodes. −3 takes 25 (critical 50) less its own resistance (*P*(1) · 1 = 1): 24 → 36
health left, its fuse shortened from 2.4 s to 1.44 s. With a critical (49) it would be at
11; with two such blasts it would explode at once.

## 6. Packet 6 kind 1 for other items

The request handler's non-bomb branch (server 0x536fd2) heals the requester by the item's
healing value (server 0x413be0): health += heal on the server's copy (no clamp here; the
stage-12 clamp follows), sound **44** (`drink2.wav`, pitch 1, no draw) at the player in
section 4, and a kind-0 hit (attacker id 0, target = the
player, amount −heal, position = the player) in section 2, which heals the player's client
(`alpha-server-simulation.md` §8, confirmed).

**Refinement:** the Alpha client never sends kind 1 for anything but a Bomb (client
0x4a2780 is the only place that builds a kind-1 record, and only for subtype 7), so this
branch is **dormant** in Alpha. Potions and food work differently, on the client that owns
the player:

* using any other type-1 item copies it into the item in hand (field 43) and starts action
  **80** (subtypes 1 Life Potion and 2 Cactus Potion) or **81** (all others), action time
  0, dodge time 0;
* during that action (server 0x53e6de, client 0x61891e; for the local player, and for NPCs
  on the server or offline) every time the action time crosses a multiple of 100 ms before
  the item's use duration *D* (server 0x413aa0) the drinker heals heal(item) · 200/*D*
  (half for NPCs), clamped to the maximum, and a kind-0 hit with amount −that (attacker id
  all ones) goes to section 2; every 200 ms sound 44 plays; a mana-like gain may also play
  sound 47. Over the whole action a player gains 2 × heal(item), an NPC 1 × heal(item)
  *as read*. Action 81 turns into 83 when it outlasts *D*. (The step runs this for the
  local player, and on the server or offline for every creature other than a player.
  "Crosses" means: with *t* the action time after this step's advance, *t* − dt < *D*
  and ⌊(*t* − dt)/100⌋ ≠ ⌊*t*/100⌋.)

The kind-1 heal branch nevertheless exists and acts on any kind-1 request whose item is
not a Bomb. The consumable rules above are only outlined here.

---

# Part B — Pets

## 7. Taming

### 7.1 Holding out the food (action 82, client 0x49c272)

Pet food is item **type 20**, its subtype the creature type it tames
(`alpha-item-names.md` §5; the 45 types that drop as loot: `alpha-loot-rolls.md` §4). The
player puts it in the **Pet slot** (slot 12; `alpha-ui-screens.md` §8 accepts types 19 and
20 there, one stack of food at most).

Each frame, for the local player: if the current action is not one of 8, 28, 29, 35, 36,
79, 82, 83, 84, 106, 107 and the right mouse button is not held (controller +0x05), then

* if the Pet slot holds pet food and the current action ended more than **500 ms** ago
  (action time > its duration + 500), the action becomes **82** (holding the food out;
  pose 53 of `alpha-creatures-models.md`, the food drawn in the hand) — the action time is
  **not** reset;
* otherwise an action that has run longer than 10,000 ms is reset to 0.

So a player who stands still with food in the Pet slot holds it out about half a second
after the last action, and keeps holding it until another action replaces 82.

Starting an ability (client 0x49b686, once the ability is usable) moves pet food
from the Pet slot back into the bags.

### 7.2 Being noticed (CombatBehavior, server 0x402f40 = client 0x42cb20)

In the acquire step of `alpha-creatures-behaviour.md` §11.2 (step 7), a candidate that is in
action 82 with pet food of **this creature's type** in its Pet slot counts as a valid
target **even when this creature is a harmless animal** (server 0x40f710), which otherwise
acquires nobody — that is how a wild sheep or collie walks up to a player holding its food.
The food does **not** replace the hostility test (server 0x4cfcc0): the candidate must
still be hostile to this creature (for a controller-1 creature, any player is), except for
a pet with no owner (controller 5, owner 0), for which the food condition alone suffices
(a controller-5 creature is never hostile to a player). Candidates of controller 5 are
never considered. The usual 20-block radius, stealth and 200-block sight rules apply. A
pacified creature (buff of kind 8) acquires nothing. A non-hostile creature (for example
controller 3) is not drawn to the food; it can still be fed if it already targets the
player through its threat map (§7.3).

### 7.3 Eating (the in-range branch)

When the creature is in range of its target and has sight of it (§11.2 step 11 there; the
chase time, path and intent have just been cleared) and **all** of these hold: it is not
pacified; the target is in action 82; the target's Pet slot holds type 20 with subtype =
this creature's type; the creature is not a pet that already has an owner; the squared
distance is ≤ the squared range — then
the creature remembers the target as its target (obj +0x11d0) and, once its current action
has finished:

* if its action is not already 110 the bite counter (field 11) := 0;
* while the bite counter is below 5: action := **110** (eating), time 0.

Action 110 has the default phases (P1 400, P2 300, P3 500, divided by the attack-speed
factor; `alpha-creatures-behaviour.md` §3.3). Its effect (stage 12 action switch, server
0x53cba1 = client 0x616de1; server or offline only) fires when the action time crosses P1:

"Crosses" means: action time < P1 and action time + dt ≥ P1 (the time before this step's
advance). Otherwise, and when the conditions below fail, nothing happens (no draw).

1. If the target (obj +0x11d0) still exists and still holds the right food in its Pet slot
   (type 20, subtype = the creature's type), in this order:
   * sound **44** (`drink2.wav`) at the creature's position, pitch 0.9 + 0.2*r* with *r* =
     draw / 32767 (**one draw**, the only draw of the bite), in section 4;
   * a **kind-6 hit** (attacker = the creature, target = the player, amount 0, byte 0x14 =
     0, position = the **creature's** position, no knock-back) is put straight into
     section 2 — it is a signal, not damage; it does not go through the hit routine;
   * progress *v* := (the value of the creature's kind-7 buff — the last kind-7 entry in its
     buff list, 0 if none) + **0.2**, single precision; bite counter + 1; field 12 := 0;
   * if *v* ≥ 1: the creature **becomes the player's pet** — controller := 5 and the pet
     behaviour of §9.4 is built with the player as owner (server 0x522580), which also
     sets owner (field 35), caps the level and sets the player's pet id; *v* := 1;
   * a **kind-7 buff** (value *v*, **5000 ms**, carrying the creature's owner id — 0 until the
     last bite) replaces any previous one, applied on the server and announced in
     **section 12** (server 0x4ce9f0; kind 7 is one of the kinds that replace an existing
     entry of the same kind, server 0x411740).

So **five bites** in a row tame the creature: the single-precision sums of 0.2 are 0.2,
0.4, 0.6, 0.8 and exactly **1.0** after the fifth bite, which passes the ≥ 1 test. A bite
counts only when the food check passes. The buff keeps partial progress for 5 s if the
feeding is interrupted. Progress belongs to the creature, not to the player: whoever is the
target at the fifth bite becomes the owner.

**Failure.** If the bite counter has reached 5, the action is still 110 and the creature is
still not a pet (its kind-7 buff expired between bites), the next in-range decision after
the action ends, with the same feeding conditions holding: if the creature has a kind-7
buff, that buff is re-applied with value 0 and time 0 (replacing it); then a **kind-8
buff** (value 0, **20,000 ms**, carrying the target player's id) is applied — kind 8 does
not replace an earlier kind-8 entry, it is added. The creature is **pacified** while any
kind-8 buff lasts (it acquires nobody, `alpha-creatures-behaviour.md` §11.2 step 6) and
cannot be fed.

### 7.4 The food becomes a pet item (client 0x494a97, in the frame routine 0x488ee0)

Every frame the client walks the frame's hit records (those its own step produced and,
online, those received in section 2 — *believed*). For a record of **kind 6** whose target
is the local player, while the Pet slot still holds **pet food**: the attacker creature is
looked up; if it carries a **kind-7 buff of value ≥ 1 whose id is the local player**:

1. the Pet slot is replaced by a fresh empty item;
2. the player's action byte := 0 (the food is no longer held out; the action time is not
   changed);
3. the player's pet id (obj +0x11c8) := the creature's id;
4. Pet slot type := **19**, subtype := the creature's type, level (item +0x10) := the
   creature's level, item +0x04 := the creature's experience — the food is used up.

The buff is found by walking the creature's buff list; the first kind-7 entry meeting both
conditions converts the slot.

No packet is sent for this; online, the changed slot reaches the server in the player's
next creature update (packet 0, field 44).

The 5-second kind-7 buff is what makes this work online: the server has already made the
creature a pet, but until packet 0 brings the new pet item the server still sees food in
the Pet slot; the pet pass (§9.5) does not remove a pet that carries a kind-7 buff.

## 8. The pet item (type 19)

| Field | Meaning |
|---|---|
| +0x00 | type 19 |
| +0x01 | subtype = creature type |
| +0x04 | the pet's **experience** (u32); the tooltip shows "X" + this + "/" + the cost of the next level (`alpha-ui-screens.md` §8) |
| +0x10 | the pet's **level** (i16) |
| +0x14 … +0x113 | sub-blocks: the pet's **name**, one character per entry in byte 3 (§15) |
| +0x114 | number of name characters (0–15) |

Material, rarity and the rest are 0 for a tamed pet. Its name is "Pet: " + the creature key
(`alpha-item-names.md` §5). "Ridable" is shown for the rideable types of `alpha-travel.md`
§5.1. The item lives in the Pet slot or the bags (type 19 sorts into the Pets tab,
`alpha-ui-screens.md` §8) and is saved with the character like any equipment
(`alpha-saves.md`, character item 11).

## 9. The pet pass (stage 9, server 0x5341d9..0x5347c7, client 0x60e419..0x60ea07)

Runs on the server and offline (an online client skips stages 7–10, §14.1), and only when
the world has a name (the pass is skipped when the world-name string at world +0x94 is
empty; `alpha-server-simulation.md` §3 stage 9), for every player of the step's player
list, in list order; §9.1–§9.3 apply when the Pet slot holds a pet item, §9.5 otherwise.

### 9.1 Finding the pet

If the Pet slot holds a pet item (type 19):

* if the player's pet id is set and names an existing creature, that is the pet;
* otherwise the pet id is cleared and the creature table is searched, in table order, for
  a creature of **controller 5 whose owner (field 35) is this player**; the first found
  becomes the pet (this re-attaches a pet after a reconnect, or one whose owner link was
  lost).

### 9.2 Rebuilding a changed pet

If a pet exists and **either** the player's taming skill (skill 0) differs from its copy
(obj +0x1d44) **or** the Pet slot item differs from its copy (obj +0x1d48): the pet is
rebuilt in place, keeping its id, position and health. The item comparison (server
0x4078f0) is field by field, not a byte compare: type, subtype, +0x04, +0x0e, +0x08,
+0x0c, level (+0x10, i16), +0x0d, the sub-block count (+0x114), and for each of the first
*count* sub-blocks bytes 3, 0, 1 and 2 (bytes 4–7 of a sub-block and the unused sub-blocks
are not compared). Rebuild:

1. its transient state is reset (server 0x4110d0), which **first resets its record**
   (server 0x411360): level 1, experience 0, velocity and the two motion vectors 0,
   multipliers to 100, 1, 1 (armour and resistance kept), mode flags, action, action time
   and combo 0, hit-stun −3000, **equipment cleared**, class 0, tier 0, **owner 0**, home
   and spawn triples to "none", default appearance, name and skills zeroed (health is not
   touched); then behaviour deleted, stealth 0, stamina 1, mount id, target and the
   creature's own pet id 0, request queue emptied, threat, hit-counter and one further
   per-creature map cleared, path state cleared, and the **buff list emptied** (so a
   kind-7 taming buff is lost here);
2. type := item subtype; level := item level; experience := item +0x04;
3. name := the item's name characters (only when the count is below 16), NUL-terminated;
4. owner (field 35) := the player;
5. appearance := the default appearance again, then the appearance generator for the type
   with no choice record (server 0x40a840) — its five choice-record draws plus any draws
   of the type's own case (`alpha-creatures-models.md` §4);
6. the pet behaviour of §9.4.

Then, whether or not a pet exists, the copies are refreshed: obj +0x1d44 := skill 0,
obj +0x1d48 := the Pet slot item. This refresh happens **only while the slot holds a pet
item**; when it holds food or nothing (§9.5) the copies keep their old values.

*As read*: a tamed creature therefore loses its original look and equipment on the first
pass after taming (the slot changed from food to pet item), and changing the pet item to
one of another type **morphs the existing pet** into the new type rather than replacing it.

### 9.3 Creating a pet

If the pass did not rebuild, the player's pet id is 0 (after §9.1), and the player is **not
gliding** (mode flag 0x10): a new creature is made, in this order:

1. id := the smallest id in use (or 0) − 1, as for bombs (§3); the player's pet id := the
   new id;
2. the creature is made with that id, inserted in the table (at its start, so stage 10 and
   stage 12 of the same step visit it) and reset as in §9.2 step 1;
3. owner := the player, controller := 5, type, level and experience from the item;
4. position := the owner's position, exactly;
5. health := the maximum health computed now (level from the item, multipliers 100, 1, 1
   from the reset) — that is, **before** the behaviour builder raises the health
   multiplier and caps the level. A fresh pet therefore starts a little below full when the
   owner has taming skill (it regenerates, §10.2), at a third of full for a Turtle, and
   above the maximum when the item level exceeds the owner's level (the stage-12 clamp then
   lowers it);
6. name from the item, as in §9.2 step 3;
7. the appearance generator with no choice record (draws as in §9.2 step 5);
8. the behaviour of §9.4 (equipment and class none, except §9.4).

### 9.4 The pet behaviour builder (server 0x522580, client 0x5ee7a0)

Given the owner id and the creature:

1. the AI becomes a **sequence of Combat (radius 20), then Companion** (owner id)
   (`alpha-creatures-behaviour.md` §11.1 sequence; §10 here);
2. stat multipliers (field 30) := health **100**, attack speed 1, damage 1, armour 1,
   resistance 1; then by type:

   | Type | Change |
   |---|---|
   | 25 Turtle | health **300**, damage **0.1**, armour **5**, resistance **5** |
   | 86 Spitter | class 3, specialization 1 |
   | 104 Snout Beetle | class 2, specialization 1 |

3. if the owner exists: the pet's **level is capped at the owner's level**; owner (field
   35) := the owner; health multiplier × **(1 + 0.5 · v(*n*))**, *n* = the owner's pet-taming
   skill (the `@health` text of `alpha-skills.md` §5.4); the owner's pet id := the pet.

### 9.5 Removing the pet

If the Pet slot does **not** hold a pet item: if the pet id names a creature, and that
creature has **no kind-7 buff**, the pet id is cleared and the creature's health := 0; if it
has a kind-7 buff nothing happens this step (the taming grace period of §7.4). A pet id
that names nothing is just cleared. The change copies of §9.2 are not refreshed. The killed
pet is then removed by the stale-pet rule of §11.2 in the same step.

## 10. Pet behaviour

The pet runs NPC AI (on the server or offline) every ≥ 32 ms like any NPC; the sequence
stops at the first behaviour that reports "handled".

### 10.1 Combat for a pet

As `alpha-creatures-behaviour.md` §11.2 with these differences:

* **Target**: Combat first picks the pet's own highest-threat target as for any creature;
  then, for controller 5, it looks up the owner (field 35). If the owner **exists**: when
  the owner's action is **106** (riding) Combat stops and reports "not handled"; otherwise
  the own-threat choice is discarded and the target is the creature, still in the world,
  with the **largest value in the owner's per-target hit counters** (obj +0x13ac; walked
  in key order, an entry replaces the best only when strictly greater, starting from 0, so
  ties keep the earlier entry) — the pet fights what its owner fights. If the owner exists
  but has hit nothing that still exists, there is no target. If the owner does **not**
  exist (owner 0, or an owner who is gone), the pet keeps its **own threat-map target**,
  like any NPC.
* A pet whose owner id is non-zero never acquires targets on its own (step 7 there), even
  when that owner is gone.
* With no target, after giving up a chase (20 s), or while the owner rides, Combat reports
  **"not handled"** and Companion runs; otherwise it reports "handled" and Companion does
  not run that tick (server 0x4036c7, 0x404fb5 return 0; 0x4052de returns 1).
* Hostility is not tested on this path *believed*: a pet attacks whatever its owner hit,
  players included (open point **PB-6**).

### 10.2 Companion (server 0x405780, client 0x43ace0)

If the owner (the id given to the builder) does not exist, Companion does nothing (the
sequence then reports nothing handled and the intent is zeroed,
`alpha-creatures-behaviour.md` §11.1). Otherwise, with **d** = owner position − pet
position:

1. Mode flag **0x40 (running)** := |**d**| > **8** blocks, using the full 3-D distance in
   fixed point (the sum of each squared component divided by 65,536 must exceed
   64 · 65,536).
2. **Regeneration** (controller 5 only): health += maxHealth · (accumulated ms) · 0.001 ·
   0.1, capped at the maximum — **10 % of maximum health per second** while Companion runs
   (that is, out of combat).
3. intent := 0. If the 3-D squared distance in blocks |**d**|² < **16** (4 blocks): stay
   (nothing more).
4. If the straight route to the owner is clear (server 0x52ef00 with the two box widths
   summed): any path is dropped and intent := **5 · (dₓ, d_y, 0)**, normalised and scaled
   to length **30** when its squared length exceeds 900 (so the pet slows down as it
   arrives: full speed beyond 6 blocks horizontally).
5. Otherwise it **path-finds** *believed*: from the owner's cell it scans down up to
   21 cells (the owner's own cell first) for the first block whose type (block byte 3 AND
   0x1f) is neither air (0) nor water (2), then upward from that block up to 21 cells for
   the first air or water cell; if the pet's position lowered by half its box height is
   within 4 blocks (squared distance < 16) of that cell it stops (path cleared, intent 0); if no path is running and the cell's zone is loaded, a path
   search to it starts (goal radius 2) and the pet follows the path (up to 10 path steps
   per tick, as in the chase of §11.2 step 10 there).

There is **no automatic teleport**: a pet that cannot keep up stays behind until the
player presses **T** (§13) or the pet dies and returns (§11.1).

### 10.3 While the owner rides, dies or is gone

* **Riding**: Combat does nothing; Companion runs but the pet-under-rider rule
  (`alpha-travel.md` §5.4) overrides its position every step.
* **Owner dies**: offline, the owner's death goes through the hit routine's death branch,
  which also kills the pet (`alpha-server-simulation.md` §6.2 step 4); in addition, when
  a kill record names the local player as victim, the kill-record handler (server
  0x4d61c0) sets the local player's pet's health to 0, clears that pet's threat map and
  clears the local player's target. The pet returns 20 s
  later at the owner's position even if the owner is still dead (§11.1, health 0 counts
  as "not negative") *as read*. On the server the owner's death happens on the owner's
  client and the server's pet is not killed *believed*; it keeps following the body.
* **Owner gone** (disconnected): the pet stays in the world with its owner id; Combat finds
  no owner and falls back to the pet's own threat map (so it fights back whoever hits it,
  but acquires nobody); Companion finds no owner, so otherwise it stands still (intent 0);
  it is never despawned (pets are
  excluded from despawn) and, if dead, never returns or disappears *as read*. When a player
  with that id joins again, §9.1 re-attaches it.

## 11. Death and return

### 11.1 Return after 20 seconds (stage 10, server 0x53527b)

A dead pet (health ≤ 0, controller 5) is not removed after 1000 ms like other NPCs. Each
step its death timer grows by dt; when its owner exists, the timer is **≥ 20,000 ms** and
the owner's health is not negative:

* health := maximum health;
* position := the owner's position;
* target cleared, threat map cleared, and the **owner's** per-target hit counters cleared
  (so the pet comes back calm).

The timer is not reset; since it is also the action time, the AI's 10-second rule
(`alpha-creatures-behaviour.md` §11.2 step 1) puts the returned pet back to action 0.
While dead, the pet keeps its id and slot link, so no new pet is created.

### 11.2 Stale pets (stage 10 start, server 0x534871)

In one walk of the creature table (in table order; the step of §11.3 runs first for each
creature in the same walk), for every creature of controller 5 whose owner exists but whose
owner's pet id is **not** this creature: its transient state and record are reset as in
§9.2 step 1 (level 1, buffs emptied, …), then health := 0, **controller := 3**, death
timer := 5000, owner := 0. A controller-5 creature whose owner does not exist is left
alone. The dead-creature pass of the same step then removes it (timer
≥ 1000). This is how a pet removed by §9.5, or a second pet of the same owner, disappears.

### 11.3 The local player's pet item tracks the pet (stage 10 start, server 0x534818)

On a client (the step's local player only; offline in practice, §14.1), every step: if the
local player's pet exists and the Pet slot holds a pet item, item level := pet level and
item +0x04 := pet experience.

## 12. Experience and levels

* A pet gets no kill records of its own (kill records go to players,
  `alpha-creatures-behaviour.md` §8).
* **Shared kill experience** (server 0x4d61c0, on the server or offline): when a player
  receives a kill record of *e* experience and that player's pet is alive (health > 0),
  the pet's experience += *e*, the level-up loop runs on the pet (cost and rule of
  `alpha-creatures-behaviour.md` §4.6: experience −= cost(*L*), *L* + 1, health := maximum),
  and the owner's Pet slot item gets the new experience and level, and its change copy
  (obj +0x1d48) is refreshed so the pass of §9.2 does not rebuild the pet. The handler
  itself does not test the victim's controller or the Pet slot's item type (the caller
  sends kill records only for hostile victims, `alpha-server-simulation.md` §6.2 step 7);
  the slot's +0x04 and +0x10 fields are written whatever it holds.
* Level-ups are not capped; the owner-level cap of §9.4 applies only when the pet is built
  or rebuilt (next taming, `/namepet`, item swap, reconnect) *as read*.
* Mission experience items (`alpha-server-simulation.md` §6.2 step 7) give the pet nothing.
* **Online**, *as read*: the server updates its pet and its own copy of the owner's item,
  but the item that is saved is the client's, and the client neither runs §11.3 nor the
  pet part of 0x4d61c0 while online. The client's next equipment change sends its stale pet
  item; the server then sees an item different from its copy and rebuilds the pet from it,
  losing the experience earned online (open point **PB-7**).

## 13. Packet 6 kind 8: call the pet back (T key)

Despite the name "reset the pet" used elsewhere, this is the **T** key. When the T key flag
(controller +0x10, `alpha-ui-screens.md` §11.1) goes down (edge, client 0x4976e7), the
client queues a packet-6 record of kind 8 with an empty item. On the server or offline
(server 0x536459):

1. if the player's pet exists: its **threat map is cleared**, and if it is more than
   **200 blocks** from the player (3-D squared distance in blocks, single precision,
   > 40,000) it is **moved to the player's position**;
2. the player's per-target hit counters (obj +0x13ac) are cleared — the pet's Combat then
   has no target (§10.1) and goes back to following;
3. the player's own target (obj +0x11d0) is cleared.

So T makes the pet stop fighting and, when it is far away, brings it back. It works on a
dead pet too (it is moved, still dead).

## 14. Pets and bombs on the network

### 14.1 What an online client runs

On an online client the step jumps from the end of stage 6 straight to stage 11 (server
0x533bab..0x533c76): stages 7–10 — spawn timers, despawn, the pet pass, the stale-pet rule,
the pet-item sync, dead creatures, **the bomb explosion** and pet return — never run there.
Instead, every step, for every creature of controller 5 whose owner exists, the owner's pet
id := that creature (server 0x533bbf). This gives every client the pet links of every
player (used for riding, the pet bars and the pet's posing).

### 14.2 What travels

* A **pet** is an NPC in packet 0 (`alpha-server-simulation.md` §5.5): controller 5,
  field 35 = the owner's id, type, level, experience, health, name (field 45), appearance.
  It is listed to every player within 200 blocks, like any NPC. No other record names it.
* Only the **owner's** client shows the pet bars (`alpha-ui-screens.md` §5.3: life bar,
  experience bar, riding bar, found through the player's pet id), offers "[R] Ride"
  (`alpha-travel.md` §5.2) and converts food into the pet item (§7.4).
* Taming shows up on the wire as section-4 sounds (44), section-2 kind-6 hits and section-12
  buff records (kinds 7 and 8) for the creature.
* A **bomb** is an NPC of controller 6 and type 144 in packet 0; its fuse is visible only as
  falling health; its explosion as sections 3 (particles), 1 (blocks), 4 (sounds), 2 (hits)
  and 7 (loot of anything it kills); it disappears by packet 2 about 1 s later.
* **Trust**: the server places a bomb for any kind-1 request with a Bomb item, without
  checking the player's inventory; pet stats come from the client's Pet slot item (level,
  experience, type), capped only by the owner level (`alpha-net-server.md` trust rule).

## 15. `/namepet` (client 0x47e1b0 chat handler)

`/namepet <name>`: if the name has **16 or more** UTF-16 units the chat shows
"Pet name too long" (`alpha-ui-screens.md` §9) and nothing else happens. Otherwise, if the
**Pet slot holds a pet item** (type 19), under the world lock: the item's sub-block count
:= the name length, and for each character *k* entry *k* gets byte 3 := the **low byte** of
the character (bytes 0–2 := 0; bytes 4–7 untouched). With no pet item in the slot nothing
happens and no message is shown. An empty name clears the name.

Nothing is sent: the changed item reaches the pet pass (offline the next step; online the
server via packet 0), which sees an item different from its copy and **rebuilds** the pet
(§9.2) with the new name — which also re-applies the level cap and resets its transient
state. Non-Latin-1 characters lose their high byte *as read*.

## 16. Worked examples (pets)

**A. Taming a collie.** A level-3 player with pet-taming skill 2 holds Collie food
(20, 19) in the Pet slot and stands still: 500 ms after her last action she is in action
82. A level-4 wild collie (type 19, harmless; assumed here to be a controller-1 creature,
so that it counts her as hostile, §7.2) within 20 blocks acquires her, walks up and, in
range, starts action 110. Its attack-speed factor *S* = 0.75 + 0.1 · *P*(4)/8 = 0.7664, so
P1 = 521 ms and the action lasts 521 + 391 + 652 = 1564 ms. Bites at about 0.52, 2.09,
3.65, 5.21 and 6.78 s: progress 0.2, 0.4, 0.6, 0.8, 1.0. At the fifth bite the server (or
offline step) makes it controller 5, owner = the player, level min(4, 3) = **3**, health
multiplier 100 · (1 + 0.5 · v(2)) = 100 · 1.08333 = 108.33; each bite sends sound 44, a
kind-6 hit and a kind-7 buff. The client sees the fifth kind-6 hit, finds the kind-7 buff
(1.0, her id) and turns the Pet slot into (19, 19), level 3, experience 0. Next pass: item ≠
copy → rebuild (record reset, buffs emptied, generator draws spent): the collie gets a
freshly generated collie look, level 3, maxHealth = 108.33 ·
*P*(3) = 108.33 · 1.20808 = **130.87**, and regenerates 13.09 health per second while
following.

**B. Following.** The owner runs 20 blocks east: |d| = 20 > 8 → running; intent = 5 · (20, 0)
= (100, 0) → shortened to (30, 0, 0). At 5 blocks: intent (25, 0, 0), not running. At 3.9
blocks: intent 0.

**C. Experience.** With the collie alive, the owner kills a level-5 goblin (kill record
*e* = 3): offline the pet's experience 0 → 3, the item's +0x04 → 3; cost(3) = 140, so 47
such kills raise it to level 4 — past the owner's level 3; the cap comes back only at the
next rebuild.

**D. Death and T.** The pet dies at 12:00:00.000; it lies dead (death timer counting) and
returns at the first step with timer ≥ 20,000 ms, at the owner's position with 130.87
health. Later the pet is stuck 250 blocks behind: T clears its threats, the owner's hit
counters and target, and moves it to the owner (250² = 62,500 > 40,000). At 150 blocks T
only calms it.

**E. Ids.** Players 1 and 2; player 1's pet is −1. Player 2 equips a pet item: its pet is
**−2**. A bomb placed next is **−3**. If pet −1 is then removed, the next new creature is
still −4 (−3 is the smallest id in use).

## 17. (Withdrawn)

This section held non-contract guidance and was removed at re-certification. Where each
rule runs (server, offline, online client) is stated in the sections above and in §14.1.

## 18. Corrections to other specs

* `alpha-creatures-behaviour.md` §11.2, Taming: client 0x42c520 only initialises a buff
  record. Taming completes in the **action-110 effect** (server 0x53cba1 = client
  0x616de1, §7.3): five bites of +0.2 progress (kind-7 buff, 5 s); the creature becomes a
  pet at progress 1. The **kind-8 buff for 20,000 ms is the failure case** (pacified), not
  given to a new pet. Pet targeting: "the creature its owner has hit most" (owner's
  obj +0x13ac), not "the attacker of their owner".
* `alpha-creatures-behaviour.md` **A-1** (partly): Combat reports "not handled" when it has
  no target, gives up a chase or its owner rides, "handled" otherwise (§10.1).
  **A-2** (partly): Companion (client 0x43ace0) is the pet's follow behaviour (§10.2).
* `alpha-travel.md` §5.1 step 2: obj +0x1138 is the owner's **pet-taming skill**, not the
  pet's level; the pet is rebuilt when that skill or the Pet slot item changes. Step 3:
  the id is (smallest id, or 0) − 1.
* `alpha-server-simulation.md` §5.6: the rebuild also happens when the taming skill
  changes; a pet's level is capped at its owner's; the health multiplier includes
  1 + 0.5 · v(taming); the kind-7 buff that keeps a pet alive with no pet item is the
  taming-progress buff, i.e. a 5-second grace period right after taming (§7.4). §6.2: a pet returns only while its owner
  exists and the owner's health is not negative. §8 kind 1: the non-bomb branch is dormant
  in Alpha (§6). §8 kind 8: it is the T key; "attacker list" = the player's per-target hit
  counters. §10: online, stages 7–10 are skipped entirely (§14.1).
  **SS-4** *believed* answered: section 11 (event set +0x50) has a writer — the threat
  announcement made after every threat increase (server 0x4d5f40, called from the hit
  routine and from Combat), 24 bytes: target id, attacker id, threat.
* `alpha-block-edits.md` §8 last paragraph and **E-2**: an online client does **not** run
  the explosion (stages 7–10 are skipped online), so section-1 records are not duplicated
  locally. Step 4 is §4 here.
* `alpha-interaction.md` §5.2: the 5-block "claim a free pet" rule writes the player's own
  field 35; with §9.1 searching by owner it has no visible effect *believed*.

## 19. Open points

* **PB-1** Confirm the fuse in a game run (offline, a copy of the save): place a level-1 and
  a higher-level Bomb and time the explosions (expected 4.0 s and 4000 · *P*(*L*) ms);
  log the bomb's health each frame.
* **PB-2** Confirm the reach and damage: stand a test character at 8.5 and 9.5 blocks
  (horizontal) and 9 and 11 blocks (vertical) from a bomb; log health before and after
  and the knock-back; one blast in four should do double damage.
* **PB-3** Whether a dodging character is really spared (dodge through the blast).
* **PB-4** Whether a step-made hit on a player appears twice in section 2 on an Alpha
  server (capture one NPC hit or bomb hit on a client; count the records).
* **PB-5** Whether the dead bomb is drawn during its last 1000 ms, and whether its light
  stays (the draw routine adds the light for every drawn type-144 creature).
* **PB-6** Whether a pet attacks another player its owner hit (PvP), and whether the hit
  lands (hostility of pet against player).
* **PB-7** Online pet experience: on a scratch server, kill monsters with a pet out, then
  change equipment and relog; check whether the pet item's level and experience kept the
  gains (expected: lost, §12).
* **PB-8** The order in which an online client applies section 12 (buffs) and section 2
  (hits) of one world update: if hits come first, the fifth kind-6 hit would find no
  kind-7 buff of value 1 and the food would never turn into a pet item online. Tame a
  creature on a scratch server and watch the Pet slot.
* **PB-9** The exact path-following of Companion when the straight route is blocked (§10.2
  step 5): record a pet following its owner around a wall.
* **PB-10** *Answered* (2026-11-16): the table is ordered by signed id, so a new bomb is
  inserted before the walking position and its first fuse turn is in the next step (§3.1).
* **PB-11** Whether any creature can carry id 0 (it would be spared by every bomb through
  the mount-id comparison of §4.2 step 1).
* **PB-12** Which controller the wild tameable animals are spawned with; only hostile ones
  (or ownerless pets) are drawn to held-out food (§7.2).
* **PB-13** What action 84 is (a target in it can be stunned by a bomb, §4.2 step 8).

## Re-certification (2026-11-16)

Re-checked against the original `Server.exe` and `Cube.exe` (fresh disassembly of the step
routine server 0x5322d0..0x548956 and client 0x60c510.., plus the decompiled routines
cited). Every rule in §1–§16 was re-read at its cited address. Non-contract guidance (old
§17 and the closing advice of §6) was removed. Status set to FINAL / STABLE / HELD.

### Corrections

1. **Table order and first fuse turn (reversal).** The creature table is ordered by the
   *signed* id (the lookup compares the high 32 bits signed, server 0x530600). The old
   §3.1 said *unsigned* and that a new bomb runs its first fuse turn in the step that
   placed it. In fact a new bomb is inserted before every creature, behind stage 12's
   walking position (server 0x536290..0x545c1b). It is first visited in the next step.
   PB-10 is closed. [§3.1, §4, §4.2]
2. **Draws at creation (omission, random-draw order).** A bomb placement spends five
   draws: the appearance generator runs without a choice record (server 0x536fc5 →
   0x40a840, five draws at entry, none in the type-144 case). Pet creation and pet
   rebuild also run the generator without a choice record: five draws plus the type's
   own draws (server 0x53445c, 0x5346a3). [§3, §9.2, §9.3, §5 A]
3. **Hit-resolution draws for a bomb hit (omission, random-draw order).** Besides the
   critical draw, these draws occur. The immune and block paths each make one sound-24
   draw. The absorb path makes one sound-91 draw. A critical hit on a target whose
   controller is not 6 makes one hurt-voice draw (server 0x4d1278), spent even for a
   type with no voice. The stun roll makes one draw (server 0x4d1565) whenever the
   target's field 16 is below −3000 and its controller is not 6. [§4.2 step 8]
4. **"No stun" was wrong for one case (reversal).** W = 0 is overridden to 1 when the
   target's action is 84 (server 0x4d14b8), so such a target can be stunned by a bomb.
   [§4.2 step 8]
5. **Step order of the explosion damage.** The knock-back direction is formed before the
   critical draw. The damage stat is read after the draw. Neither makes a draw. The
   filter also compares X's id with B's mount id (obj +0x11c0, server 0x534f71); the old
   text omitted this test. The horizontal test is a fixed-point integer comparison
   (server 0x41ce90, 0x4dade0). [§0.1, §4.2 steps 1–6]
6. **Hit-resolution inputs restated.** The bomb is call site 2 of
   `alpha-attack-routine.md`: flag B set, W 0, mitigation selector set, K 0, blockable
   set (server 0x5351d5..0x53521d). The immune path also plays sound 24 with full
   knock-back. The absorb factor is 1 − min(1, field 11 / 50) = 1 (server 0x40fcf0). The
   record amount is multiplied by the target's kind-1 buffs. [§4.2 step 8]
7. **Removal bookkeeping.** A removed dead creature's id is also erased from every
   per-target hit-counter map, not only from the threat maps (server 0x5353a5, 0x5353b8).
   The section reference is now server-sim §6.3. [§4, §4.3]
8. **Kind-1 heal sound.** The 1.0 is the sound's pitch, not its volume (record +0x10, as
   for the bite sound, server 0x53702d). No clamp is applied at that point. [§6]
9. **Drinking-action gate and crossing rule stated** (server 0x53e6c3..0x53e73b). [§6]
10. **Food attraction (reversal).** Held-out food does **not** bypass the hostility test
    (server 0x402f40, acquire branch; hostility 0x4cfcc0). It only lifts the
    harmless-animal restriction (0x40f710). Only an ownerless pet may take the food as a
    substitute for hostility. Controller-5 candidates are never considered.
    `alpha-creatures-behaviour.md` §11.2 step 7 already states this correctly. [§7.2,
    §16 A, PB-12]
11. **Bite details.** The single draw is the sound pitch. The kind-6 record carries the
    *creature's* position (server 0x53ccd1) and does not go through the hit routine. The
    progress read is the last kind-7 entry. A bite counts only when the food check
    passes. [§7.3]
12. **Five bites sum to exactly 1.0**, not "1.0000001". [§7.3]
13. **Failure buffs.** The zeroed kind-7 buff is issued only if a kind-7 buff exists.
    Kind 8 is *added*, not replacing: server 0x411740 replaces only kinds 1, 2, 3, 6, 7,
    9, 10 and 11. Failure requires the action still to be 110. [§7.3]
14. **Pet-item conversion order:** slot cleared, action byte 0 (time kept), pet id, then
    type, subtype, level and experience (client 0x494b7a..0x494bfa). [§7.4]
15. **Pet pass needs a named world.** The pass is skipped when the world-name string
    (world +0x94, test 0x419f80) is empty (server 0x5341df). [§9]
16. **Item comparison is field-wise** (server 0x4078f0, 0x4079c0), not a byte compare.
    Bytes 4–7 of a sub-block and the unused sub-blocks are ignored. [§9.2]
17. **The reset includes the record reset and empties the buff list** (server 0x4110d0
    calls 0x411360 first). A rebuild or stale-pet reset therefore drops the kind-7 buff
    and also clears the mount id and the third map. [§9.2 step 1, §11.2]
18. **Change copies are refreshed only while the slot holds a pet item** (server 0x53447d
    lies inside the type-19 branch). The old text said "whether or not a pet exists",
    which held only within that branch. [§9.2, §9.5]
19. **Pet creation order and health.** The new pet is reset before its fields are written
    (server 0x5345f9). Its health is computed with the item level before the cap, so it
    can start above the maximum when the item level exceeds the owner's. A Turtle starts
    at one third. [§9.3]
20. **Pet target without an owner (reversal).** When the owner does not exist, Combat
    keeps the pet's own threat-map target (server 0x402f40, the controller-5 branch jumps
    back with the threat choice kept). The old text said there was no target. Ties in the
    owner's hit map keep the earlier entry (strict comparison). [§10.1, §10.3]
21. **Companion details.** With no owner, Companion does nothing at all. The running test
    and the 4-block stay test use 3-D distance. The intent is normalised to 30 when its
    squared length exceeds 900. Regeneration is controller-5 only. The path-cell scan
    details are given (server 0x405780). [§10.2]
22. **Owner death on the local client.** The kill-record handler, with the local player
    as victim, also kills that player's pet and clears its threat map (server 0x4d6251,
    0x4d6279). [§10.3]
23. **Kill-share checks.** The pet's share tests neither the victim's controller nor the
    Pet slot's item type (server 0x4d62de..0x4d6338). [§12]
24. **T-key distance** is the 3-D single-precision squared distance (server 0x5364b1).
    [§13]
25. **Stale-pet walk** shares the table walk with §11.3, and pets whose owner is missing
    are left alone (server 0x534800..0x5348cb). [§11.2]
26. **Cross-references updated** to the current numbering of
    `alpha-creatures-behaviour.md` (behaviour layer §11, kills §8, sound ids §13) and of
    `alpha-server-simulation.md` (table order §0, after-death §6.3). Citations to
    non-existent subsections (§8.4, §16.3, §16.8) were replaced by the routines
    themselves (server 0x406400/0x407020; client 0x49b686). [throughout]
27. **Client address of the fuse.** Client 0x611b7b is the type-144 test (server
    0x53793b), not the start of the fuse range. [§3.1]
28. **Chain timing (reversal).** The old §4.4 said a bomb killed by another bomb always
    explodes in the next step. The dead-creature walk (server 0x534950..0x535429) visits
    the table in signed-id order and explodes inside that walk. An older bomb (larger id)
    killed by the blast therefore explodes later in the **same** pass. Only a newer bomb
    (smaller id, already passed) waits for the next step. [§4.4]

### Confirmed unchanged (sample)

Bomb record fields and constants (type 0x90, controller 6, level from item +0x10, health
500, multipliers 100/1/1/1/1). Fuse 0.025 · dt with floor 0. Explosion gate: death timer 0
and type 144. Reach 8 + 0.7 bₓ and 8 + 0.8 + 0.5 b_z. Damage 25 · d(B), × 2 on draw mod 4 =
0. Knock-back (10ûₓ, 10û_y, 2.5) with the 0.01 normalise threshold. Sound 39 at pitch 1.5.
Bomb light (1, 0.8, 0.5), radius 16, 800 ms. Action-82 exclusion list and the 500 ms and
10 000 ms rules. Bite pitch 0.9 + 0.2r and kind-7 buff of 5000 ms. Builder multipliers and
the type table (25, 86, 104). Taming factor 1 + 0.5 v(n). Pet return at ≥ 20 000 ms with
owner health ≥ 0. Stale-pet reset to controller 3 with timer 5000. Level cost
⌊1000 (1 − 1/((L − 1) 0.05 + 1)) + 50⌋. T key > 40 000. `/namepet` limit 16 with byte-3
writes. Online skip of stages 7–10 and the pet-link copy.

### Remaining uncertainties

* PB-1…PB-9 and PB-11…PB-13 stand. The path-finding branch of Companion (§10.2 step 5) and
  the pet's lack of a hostility test (§10.1, PB-6) remain *believed*.
* §3.2 (knock-back moving a bomb) remains *believed*. `alpha-attack-routine.md` M6 states
  that hit resolution itself never changes the target's velocity. Whether the hit routine
  or the client applies the knock-back to a bomb was not traced.
* The despawn statement in §3 relies on `alpha-server-simulation.md` §5.4 and was not
  re-traced.
* §6's consumable outline: the 200 ms sound-44 cadence and the sound-47 gain were not
  re-traced beyond the heal formula.

### Cross-spec notes (other specs not edited)

* `alpha-creatures-behaviour.md` §11.2 step 11 still says the creature at combo ≥ 5
  "becomes the target's pet (0x42C520)". This spec: the combo ≥ 5 branch is the
  **failure** branch, which issues a zeroed kind-7 buff and an added kind-8 buff. Taming
  completes in the action-110 effect (server 0x53cba1..0x53cdcd). Evidence: server
  0x402f40 (in-range branch, `combo < 5` test), 0x53cdb4..0x53cdcd. This was already
  listed in §18.
* `alpha-creatures-models.md` (correction 35) calls obj +0x11c0 a **client-only** mount
  id. This spec: the server's creature object carries the field too. It is zeroed by the
  server constructor (0x40651a) and by the reset (0x411113), and compared by the server
  step (0x534f71, 0x53c02e, 0x547402, 0x548380). Only the meaning "mount id" is taken from
  that spec.
* `alpha-server-simulation.md` §0 (signed table order, negative ids first) **agrees**
  with correction 1. The error was internal to this spec.
* `alpha-attack-routine.md` (site 2 arguments, M5, M9, M10) **agrees** with corrections
  3, 4 and 6.
