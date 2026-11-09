# Alpha bombs and pets

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

**Status.** Bomb placement, fuse, explosion damage, the kind-1 healing branch, taming, the
pet item, the pet pass, the two pet behaviours, death and return, pet experience, packet 6
kind 8 and `/namepet` are specified. Open points are in §19.

This document **adds to** and does not repeat:

* `alpha-server-simulation.md` — the step and its numbered stages (§3), creature ids
  (§5.3), player pets (§5.6, refined in §9 here), the hit routine (§6.1), NPC death (§6.2),
  packet 6 (§8), online versus offline (§10), the event set (§11);
* `alpha-block-edits.md` §8 — the bomb's particle burst, the natural-block removal, the
  explosion sound and light flood (steps 1–3); §4 here is its step 4;
* `alpha-creatures-behaviour.md` — derived stats (§4), hit resolution (§5), kill experience
  (§12.3), the behaviour layer (§13; corrected in §18 here), sound ids (§14);
* `alpha-travel.md` §5 — the Pet slot, rideable types, mounting, riding energy and the pet
  under its rider;
* `alpha-net-packets.md` — the creature record (§2.1), the hit record (§5.1), packet 6
  (§4.1);
* `alpha-loot-rolls.md` §4 (the 45 tameable types), `alpha-item-names.md` §5 (pet and pet
  food names), `alpha-talk-vendors.md` (pet-food talk; the vendor that sells Bombs),
  `alpha-skills.md` §2.1, §5.4 (pet taming and pet riding skills), `alpha-ui-screens.md`
  §5.3 (pet bars), §8 (Pet slot rules, pet tooltip), §9 (chat commands), §11 (keys),
  `alpha-creatures-models.md` §4 (appearance generator; type 144 = `bomb1`).

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

Using a consumable (the quick item Q of `alpha-creatures-behaviour.md` §16.8, or the
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
| appearance | the appearance generator for type 144 (server 0x40a840; `alpha-creatures-models.md` §4: body `bomb1`, scale 0.8, box (0.8, 0.8, 0.8), body offset (0, 0, 1)) |
| everything else | the new-creature defaults (`alpha-creatures-behaviour.md` §8.4): health **500**, multipliers 100, 1, 1, 1, 1, tier 0, no class, no equipment, no behaviour |

The bomb has **no behaviour object** (no AI), its action clock does not run (controller 6
is skipped by the action-time advance, server 0x53e2a8), and it never despawns while a
player is within 220 blocks (`alpha-server-simulation.md` §5.4; its despawn radius is the
default 200).

### 3.1 The fuse

On each of the bomb's turns in stage 12 (server 0x537856..0x537987, client 0x611b7b):

1. health is clamped to the maximum health — for the bomb
   **maxHealth = 100 · *P*(*L*)** (NPC formula with tier 0 and no class or items,
   `alpha-creatures-behaviour.md` §4.1), so the initial 500 drops to 100 · *P*(*L*) on its
   first turn;
2. health −= **0.025 · dt**, floored at 0 (server and offline only; an online client takes
   the bomb's health from packet 0).

The fuse therefore lasts **4000 · *P*(*L*) ms** from placement: 4.0 s at level 1, 5.66 s at
level 5, 7.63 s at level 10 *as read* (higher-level bombs burn longer). When its health
reaches 0 nothing happens in that turn; the explosion comes in **stage 10 of the next
step** (§4), because stage 10 runs before stage 12.

The bomb is created in the same turn and inserted in the creature map. *Believed*: the map
is ordered by the id as an unsigned number, so the new bomb (id 2^64 − *k*) is visited
after every player and runs its first fuse turn in the same step.

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

The dead-creature pass (stage 10, server 0x534998..0x535266) visits every creature that is
not a player and has health ≤ 0. A creature of type **144** whose death timer is **still 0**
(its first dead step) explodes; then, like every dead NPC, its timer grows by dt, and it is
removed from the world once the timer reaches 1000 ms (`alpha-server-simulation.md` §6.2).
Since the bomb's action clock never runs, its timer is still 0 on its first dead step,
whether it died by the fuse or by a hit (a hit also sets the timer to 0).

Stage 10 is skipped on an online client (§14.1), so only the server (or the offline client)
explodes bombs.

### 4.1 Blocks, particles, sound

Steps 1–3 of `alpha-block-edits.md` §8 (the burst, the natural blocks inside the radius-8
sphere, the sound only if blocks were removed, the light flood on the server).

### 4.2 Damage to creatures (step 4)

With **B** the bomb and **P** its position, for every creature X in the world, in creature
map order:

1. **Who can be hit.** X is not B; the hostility test of the bomb (server 0x4d18c0) always
   passes because B has controller 6 — so **players, hostile and friendly NPCs, merchants,
   pets, other live bombs and the player who placed it** can all be hit; X has health
   > 0; X's dodge time (field 15) is **0** (a creature in the middle of a dodge is not
   hit); X is not yet in B's "already hit" set.
2. **Range** (horizontal and vertical, both required): with Δ = X's position − **P** in
   blocks,
   * horizontal: Δx² + Δy² < (**8 + 0.7 · bₓ**)², bₓ = X's box width (appearance box x);
   * vertical: |Δz| < **8 + 0.8 + 0.5 · b_z**, i.e. B's box height (0.8) + half X's box
     height + 8.

   So the reach is a cylinder slightly wider than the block-removal sphere.
3. **Critical**: one draw; critical when draw mod 4 = 0 (one chance in four).
4. **Damage**: *x* = **25 · d(B)**, × 2 when critical, where d(B) is the bomb's damage stat
   (server 0x408f70 = `alpha-creatures-behaviour.md` §4.2 with no weapons): d(B) = damage
   multiplier (1) · *P*(*L*) · 2^(0.25 · tier) = *P*(*L*). Level 1: 25 (50); level 10: 47.67
   (95.33).
5. **Knock-back**: the horizontal direction from **P** to X, normalised when its squared
   length exceeds 0.01, then its z set to 0.25, all × 10: **k** = (10·ûₓ, 10·û_y, 2.5). A
   creature standing (almost) on the bomb gets k ≈ (0, 0, 2.5).
6. X's id is added to B's "already hit" set (B is processed only once anyway).
7. The hit goes through **hit resolution** (server 0x4cfd50 = client 0x596d30,
   `alpha-creatures-behaviour.md` §5) with attacker B, target X, amount *x*, the critical
   flag, the **area flag set**, **knock-back k**, the **magic flag set**, attacker action 0
   and "can be blocked" set. Consequences of those inputs:
   * a blocking target blocks it (a kind-1 record with half knock-back, sound 24);
     absorbing shields absorb it (kind 5); a controller-1 target with mode flag 0x80 or
     contact flag 0x20 is immune (kind 3);
   * the target's **resistance** (not armour) is subtracted:
     *x* := max(0, *x* − resistance(X)) (the combo factor *c* is 1 because B's combo is 0);
   * no spirit effects (B's combo is 0) and **no stun** (the stun chance of the server
     version is scaled by an input the bomb passes as 0, *as read*);
   * impact sound **39** (`fire-hit.wav`) at pitch **1.5** at the target (the magic sound of
     an attacker of specialization 0); a critical hit also plays the target's hurt voice
     (unless X has controller 6);
   * the final record is a **kind-0 hit**: attacker = B's id, target = X's id, amount *x*,
     critical flag at byte 0x14, position = X's position, knock-back **k**, byte 0x44 = 1
     (the area flag) (`alpha-net-packets.md` §5.1).
8. **Where the hit goes.** Hit resolution appends the record to the step's hit list (the
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
map. Consequences, *as read*:

* the player who placed the bomb gets **no credit**: kill experience goes only to players
  already in the victim's threat map (`alpha-creatures-behaviour.md` §12.3); a monster
  killed outright by a bomb nobody else touched gives no experience and no kill record;
* a hit NPC that survives may choose the (dead) bomb as its highest-threat target until it
  is removed.

### 4.4 Chains

A live bomb inside another bomb's reach is hit like any creature. Its health drops by the
damage (after its own resistance); if it reaches 0 it "dies" through the hit routine
(sound 27, no loot, no experience — it is not hostile), its death timer is 0, and it
explodes on the next step's stage 10. A bomb that survives the hit simply has a shorter
fuse. Every exploding bomb has its own "already hit" set, so a creature in range of two
bombs is hit by both.

## 5. Worked examples (bombs, dt = 16 ms)

**A. A level-1 bomb.** Player 1 (level 5, box (1.2, 1.2, 2.7)) uses a Bomb at
(8,398,402.5, 8,397,472.5, 65.0) blocks on day 5. The world holds players 1 and 2 and the
pet −1, so the bomb's id is **−2** (`fe ff ff ff ff ff ff ff` on the wire).

* Its first turn clamps health 500 → 100 and burns 0.4 → 99.6; after 250 turns (≈ 4.0 s)
  health is 0; the next step's stage 10 explodes it.
* Player 1 has walked 5 blocks east (Δ = (5, 0, 0)): 25 < (8 + 0.84)² = 78.1 and
  0 < 8 + 0.8 + 1.35 = 10.15 → hit. Draw mod 4 = 2 → not critical: *x* = 25.
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
healing value (server 0x413be0): health += heal on the server's copy, sound **44**
(`drink2.wav`, volume 1) in section 4, and a kind-0 hit (attacker id 0, target = the
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
  *as read*. Action 81 turns into 83 when it outlasts *D*.

An implementation should keep the kind-1 heal branch for compatibility (a modified client
could send it) but must not rely on it. The consumable rules above belong to a future
consumables specification and are only outlined here.

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

Starting an ability (`alpha-creatures-behaviour.md` §16.3, client 0x49b686) moves pet food
from the Pet slot back into the bags.

### 7.2 Being noticed (CombatBehavior, server 0x402f40 = client 0x42cb20)

In the acquire step of `alpha-creatures-behaviour.md` §13.2 (step 7), a candidate that is in
action 82 with pet food of **this creature's type** in its Pet slot counts as a valid
target even when it would not be hostile, and even when this creature is a harmless animal
— that is how a sheep or a collie walks up to a player holding its food. (A pet with no
owner, controller 5 and owner 0, may also pick such a player.) The usual 20-block radius,
stealth and 200-block sight rules apply. A pacified creature (buff of kind 8) acquires
nothing.

### 7.3 Eating (the in-range branch)

When the creature is in range of its target (§13.2 step 11 there) and **all** of these hold:
it is not pacified; the target is in action 82; the target's Pet slot holds type 20 with
subtype = this creature's type; the creature is not a pet that already has an owner — then
the creature remembers the target as its target (obj +0x11d0) and, once its current action
has finished:

* if its action is not already 110 the bite counter (field 11) := 0;
* while the bite counter is below 5: action := **110** (eating), time 0.

Action 110 has the default phases (P1 400, P2 300, P3 500, divided by the attack-speed
factor; `alpha-creatures-behaviour.md` §3.3). Its effect (stage 12 action switch, server
0x53cba1 = client 0x616de1; server or offline only) fires when the action time crosses P1:

1. If the target (obj +0x11d0) still exists and still holds the right food in its Pet slot:
   * sound **44** (`drink2.wav`) at the creature, pitch 0.9 + 0.2*r*, in section 4;
   * a **kind-6 hit** (attacker = the creature, target = the player, amount 0, no
     knock-back) is put straight into section 2 — it is a signal, not damage;
   * progress *v* := (the value of the creature's kind-7 buff, 0 if none) + **0.2**; bite
     counter + 1; field 12 := 0;
   * if *v* ≥ 1: the creature **becomes the player's pet** — controller := 5 and the pet
     behaviour of §9.4 is built with the player as owner (server 0x522580), which also
     sets owner (field 35), caps the level and sets the player's pet id; *v* := 1;
   * a **kind-7 buff** (value *v*, **5000 ms**, carrying the creature's owner id — 0 until the
     last bite) replaces any previous one, applied on the server and announced in
     **section 12** (server 0x4ce9f0).

So **five bites** in a row tame the creature (0.2 × 5 = 1.0000001 in single precision);
the buff keeps partial progress for 5 s if the feeding is interrupted.

**Failure.** If the bite counter has reached 5 and the creature is still not a pet (its
kind-7 buff expired between bites, or the target changed), the next decision after the
action ends replaces the kind-7 buff by a zeroed one (value 0, time 0) and adds a **kind-8
buff** for **20,000 ms** carrying the player's id: the creature is **pacified** for 20 s
(it acquires nobody, `alpha-creatures-behaviour.md` §13.2 step 6) and cannot be fed.

### 7.4 The food becomes a pet item (client 0x494a97, in the frame routine 0x488ee0)

Every frame the client walks the frame's hit records (those its own step produced and,
online, those received in section 2 — *believed*). For a record of **kind 6** whose target
is the local player, while the Pet slot still holds **pet food**: the attacker creature is
looked up; if it carries a **kind-7 buff of value ≥ 1 whose id is the local player**:

1. the Pet slot is replaced by a fresh empty item, then: type := **19**, subtype := the
   creature's type, level (item +0x10) := the creature's level, item +0x04 := the
   creature's experience — the food is used up;
2. the player's action := 0 (the food is no longer held out);
3. the player's pet id (obj +0x11c8) := the creature's id.

No packet is sent for this; online, the changed slot reaches the server in the player's
next creature update (packet 0, field 44).

The 5-second kind-7 buff is what makes this work online: the server has already made the
creature a pet, but until packet 0 brings the new pet item the server still sees food in
the Pet slot; the pet pass (§9.3) does not remove a pet that carries a kind-7 buff.

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

Runs on the server and offline (an online client skips stages 7–10, §14.1), for every
player of the step's player list, in this order.

### 9.1 Finding the pet

If the Pet slot holds a pet item (type 19):

* if the player's pet id is set and names an existing creature, that is the pet;
* otherwise the pet id is cleared and the creature map is searched for a creature of
  **controller 5 whose owner (field 35) is this player**; the first found becomes the pet
  (this re-attaches a pet after a reconnect, or one whose owner link was lost).

### 9.2 Rebuilding a changed pet

If a pet exists and **either** the player's taming skill (skill 0) differs from its copy
(obj +0x1d44) **or** the Pet slot item differs from its copy (obj +0x1d48, a byte compare
of the item): the pet is rebuilt in place, keeping its id, position and health:

1. its transient state is reset (server 0x4110d0): behaviour, path, request queue, threat
   and hit-counter maps, target, stealth 0, stamina 1; and its record (server 0x411360):
   level 1, experience 0, velocity and the two motion vectors 0, multipliers to 100, 1, 1
   (armour and resistance kept), mode flags, action and combo 0, hit-stun −3000,
   **equipment cleared**, class 0, tier 0, **owner 0**, home and spawn triples to "none",
   default appearance, name and skills zeroed;
2. type := item subtype; level := item level; experience := item +0x04;
3. name := the item's name characters (only when the count is below 16), NUL-terminated;
4. owner (field 35) := the player;
5. appearance := the appearance generator for the type with no choices (server 0x40a840);
6. the pet behaviour of §9.4.

Then, whether or not a pet exists, the copies are refreshed: obj +0x1d44 := skill 0,
obj +0x1d48 := the Pet slot item.

*As read*: a tamed creature therefore loses its original look and equipment on the first
pass after taming (the slot changed from food to pet item), and changing the pet item to
one of another type **morphs the existing pet** into the new type rather than replacing it.

### 9.3 Creating a pet

If the pass did not rebuild, the player still has no pet, and the player is **not gliding**
(mode flag 0x10): a new creature is made:

| Property | Value |
|---|---|
| id | the smallest id in use (or 0) − 1, as for bombs (§3) |
| controller, owner | 5, the player |
| type, level, experience, name | from the item, as in §9.2 |
| position | the owner's position, exactly |
| health | the maximum health **before** the behaviour builder raises the health multiplier and caps the level (so a fresh pet starts a little below full when the owner has taming skill; it regenerates, §10.2) |
| appearance | as in §9.2 |
| equipment, class | none (except §9.4) |

The player's pet id := the new id. Then the behaviour of §9.4.

### 9.4 The pet behaviour builder (server 0x522580, client 0x5ee7a0)

Given the owner id and the creature:

1. the AI becomes a **sequence of Combat (radius 20), then Companion** (owner id)
   (`alpha-creatures-behaviour.md` §13.1 sequence; §10 here);
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
that names nothing is just cleared. The killed pet is then removed by the stale-pet rule of
§11.2 in the same step.

## 10. Pet behaviour

The pet runs NPC AI (on the server or offline) every ≥ 32 ms like any NPC; the sequence
stops at the first behaviour that reports "handled".

### 10.1 Combat for a pet

As `alpha-creatures-behaviour.md` §13.2 with these differences:

* **Target**: not its own threat map. The pet looks up its owner; if the owner's action is
  **106** (riding) Combat stops and reports "not handled". Otherwise the target is the
  creature, still in the world, that the **owner has hit most often** (largest entry of
  the owner's per-target hit counters, obj +0x13ac) — the pet fights what its owner fights.
  If the owner has hit nothing (or the owner is missing) there is no target.
* A pet with an owner never acquires targets on its own (step 7 there).
* With no target, after giving up a chase (20 s), or while the owner rides, Combat reports
  **"not handled"** and Companion runs; otherwise it reports "handled" and Companion does
  not run that tick (server 0x4036c7, 0x404fb5 return 0; 0x4052de returns 1).
* Hostility is not tested on this path *believed*: a pet attacks whatever its owner hit,
  players included (open point **PB-6**).

### 10.2 Companion (server 0x405780, client 0x43ace0)

With **d** = owner position − pet position in blocks:

1. Mode flag **0x40 (running)** := |**d**| > **8** blocks.
2. **Regeneration**: health += maxHealth · 0.1 · (accumulated ms)/1000, capped at the
   maximum — **10 % of maximum health per second** while Companion runs (that is, out of
   combat).
3. intent := 0. If |**d**| < **4** blocks: stay (nothing more).
4. If the straight route to the owner is clear (server 0x52ef00 with the two box widths
   summed): any path is dropped and intent := **5 · (dₓ, d_y, 0)**, shortened to length
   **30** when longer (so the pet slows down as it arrives: full speed beyond 6 blocks).
5. Otherwise it **path-finds** *believed*: from the owner's position it scans down up to
   21 blocks for the first block that is neither air (type 0) nor water (type 2), then up
   up to 21 blocks for the first air or water cell above it; if the pet is within 4 blocks
   of that cell it stops; if no path is running and the cell's zone is loaded, a path
   search to it starts (goal radius 2) and the pet follows the path (up to 10 path steps
   per tick, as in the chase of §13.2 step 10 there).

There is **no automatic teleport**: a pet that cannot keep up stays behind until the
player presses **T** (§13) or the pet dies and returns (§11.1).

### 10.3 While the owner rides, dies or is gone

* **Riding**: Combat does nothing; Companion runs but the pet-under-rider rule
  (`alpha-travel.md` §5.4) overrides its position every step.
* **Owner dies**: offline, the owner's death goes through the hit routine's death branch,
  which also kills the pet (`alpha-server-simulation.md` §6.2 step 4); the pet returns 20 s
  later at the owner's position even if the owner is still dead (§11.1, health 0 counts
  as "not negative") *as read*. On the server the owner's death happens on the owner's
  client and the server's pet is not killed *believed*; it keeps following the body.
* **Owner gone** (disconnected): the pet stays in the world with its owner id; Combat finds
  no owner, Companion finds no owner, so it stands still; it is never despawned (pets are
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
(`alpha-creatures-behaviour.md` §13.2 step 1) puts the returned pet back to action 0.
While dead, the pet keeps its id and slot link, so no new pet is created.

### 11.2 Stale pets (stage 10 start, server 0x534871)

For every creature of controller 5 whose owner exists but whose owner's pet id is **not**
this creature: its transient state is reset, health := 0, **controller := 3**, death
timer := 5000, owner := 0. The dead-creature pass of the same step then removes it (timer
≥ 1000). This is how a pet removed by §9.5, or a second pet of the same owner, disappears.

### 11.3 The local player's pet item tracks the pet (stage 10 start, server 0x534818)

On a client (the step's local player only; offline in practice, §14.1), every step: if the
local player's pet exists and the Pet slot holds a pet item, item level := pet level and
item +0x04 := pet experience.

## 12. Experience and levels

* A pet gets no kill records of its own (kill records go to players,
  `alpha-creatures-behaviour.md` §12.3).
* **Shared kill experience** (server 0x4d61c0, on the server or offline): when a player
  receives a kill record of *e* experience and that player's pet is alive (health > 0),
  the pet's experience += *e*, the level-up loop runs on the pet (cost and rule of
  `alpha-creatures-behaviour.md` §4.6: experience −= cost(*L*), *L* + 1, health := maximum),
  and the owner's Pet slot item gets the new experience and level, and its change copy
  (obj +0x1d48) is refreshed so the pass of §9.2 does not rebuild the pet.
* Level-ups are not capped; the owner-level cap of §9.4 applies only when the pet is built
  or rebuilt (next taming, `/namepet`, item swap, reconnect) *as read*.
* Mission experience items (`alpha-server-simulation.md` §6.2 step 7) give the pet nothing.
* **Online**, *as read*: the server updates its pet and its own copy of the owner's item,
  but the item that is saved is the client's, and the client neither runs §11.3 nor the
  pet part of 0x4d61c0 while online. The client's next equipment change sends its stale pet
  item; the server then sees an item different from its copy and rebuilds the pet from it,
  losing the experience earned online (open point **PB-7**).

## 13. Packet 6 kind 8: call the pet back (T key)

Not a "reset" in the sense of the roadmap: it is the **T** key. When the T key flag
(controller +0x10, `alpha-ui-screens.md` §11.1) goes down (edge, client 0x4976e7), the
client queues a packet-6 record of kind 8 with an empty item. On the server or offline
(server 0x536459):

1. if the player's pet exists: its **threat map is cleared**, and if it is more than
   **200 blocks** from the player (squared distance > 40,000) it is **moved to the
   player's position**;
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
82. A level-4 collie (type 19, harmless) within 20 blocks acquires her, walks up and, in
range, starts action 110. Its attack-speed factor *S* = 0.75 + 0.1 · *P*(4)/8 = 0.7664, so
P1 = 521 ms and the action lasts 521 + 391 + 652 = 1564 ms. Bites at about 0.52, 2.09,
3.65, 5.21 and 6.78 s: progress 0.2, 0.4, 0.6, 0.8, 1.0. At the fifth bite the server (or
offline step) makes it controller 5, owner = the player, level min(4, 3) = **3**, health
multiplier 100 · (1 + 0.5 · v(2)) = 100 · 1.08333 = 108.33; each bite sends sound 44, a
kind-6 hit and a kind-7 buff. The client sees the fifth kind-6 hit, finds the kind-7 buff
(1.0, her id) and turns the Pet slot into (19, 19), level 3, experience 0. Next pass: item ≠
copy → rebuild: the collie gets the default collie look, level 3, maxHealth = 108.33 ·
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

## 17. Implementing it in AlphaVulkan

* Keep pets and bombs ordinary creatures with controller 5 and 6, owner in field 35, and
  the pet id outside the record, so that Alpha clients and the server read them unchanged.
* Run the pet pass, stale-pet rule, dead pass (bomb explosion, pet return) and the kind-1,
  kind-8 and action-110 effects only where the world is authoritative (server, or the host
  in single player); on a client keep only §14.1, §7.1, §7.4 and the drawing of §3.3.
* Section 2 should carry each player hit once (PB-4); if wire compatibility with Alpha
  clients is wanted, test whether they tolerate the duplicate before changing it.
* Hook points for mods: the bomb damage formula and reach (§4.2), the fuse rate (§3.1), the
  taming step size 0.2 and bite count (§7.3), the type table of §9.4, the follow distances
  of §10.2 and the 20-s return (§11.1).

## 18. Corrections to other specs

* `alpha-creatures-behaviour.md` §13.2, Taming: client 0x42c520 only initialises a buff
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
* **PB-10** Whether the map order puts a new bomb's first fuse turn in the same step
  (§3.1, a one-step difference in the fuse).
