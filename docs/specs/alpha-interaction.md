# Alpha interaction: targets, "[R]" prompts, static objects and portals

Behavioural specification of how the Cube World **Alpha** client (`Cube.exe`, SHA-256
`84a7a132…30bf`) decides what the player is pointing at, which "[R] …" prompt it shows,
what pressing **R** (and **E** for items) does for each kind of target, and the static
objects these targets come from: their record, kinds, models, state, animation, saving and
networking. It resolves the open point of `alpha-ui-screens.md` §5.2.8 about "object type
45": **static kind 45 is the runestone** that worldgen puts in every type-10 site (the
"Portal of *N*" zones), and pressing R at it opens the world map in teleport mode.

Written from the Alpha decompile (`disassembled alpha cube world/decompiled/client`), the
reduced-analysis retry of the frame update 0x488ee0 (its default decompile timed out), the
per-function assembly, and a fresh disassembly of the jump-table cases the export did not
reach. Nothing here is copied code. Routines are cited by address in `Cube.exe` as
references only.

Related: `alpha-ui-screens.md` (§5.2 world map and teleporting, §5.4.1 creature focus,
§5.8 crosshair and prompt, §8 windows, §11 keys), `alpha-worldgen-structures.md` (§2.2 static
record, §4 type-10 site and runestone, town/dungeon statics),
`alpha-worldgen-locations.md` (§7 type-10 sites, kind-4 zones), `alpha-server-simulation.md`
(§8 packet 6 kinds), `alpha-net-packets.md` (§3.6 section 6, §3.7 section 7, §4.1 packet 6),
`alpha-saves.md` (§5 zone save), `alpha-rendering.md` (§10.1 static lights, §11.3).

**Status.** Target selection (all three passes, constants, tie rules, fall-backs, clearing),
every prompt, every R/E action, the static record layout, the kind → model table, the
state byte and its animation, the use handler of packet-6 kind 3, and the runestone/portal
chain are specified. Partly traced: the trap simulation (§6.4), what the static cell index
is built from (§4.3), and the dialogue contents of "Talk" (quest specification).

---

## 0. Conventions

* "G" is the game object, "W" the world object (G+0x2e4), "P" the local player creature
  (G+0x8006d0). Creature fields are those of `alpha-ui-screens.md` §5.4.1 (+0x08 64-bit id,
  +0x10 position, +0x60 controller kind: 0 player, 1 hostile NPC, 3 NPC, 5 pet, 6 inert;
  +0x68 action byte, +0x16c health, +0x1350 drawn position, +0x1180 drawn-height offset,
  +0x80 size vector).
* Positions are i64 in 1/65536 block. A **zone** is 256 × 256 blocks; a **cell** here is
  8 × 8 blocks (32 × 32 cells per zone). "Distance" is Euclidean in blocks; "squared
  distance" is said explicitly.
* A **static** (static object, static entity) is a 0x188-byte (392-byte) record in a
  zone's static vector (zone + 0x0c). It is addressed everywhere by the triple
  **(zone x, zone y, index)** — three i32. (−1, −1, 0) or any triple with a negative zone
  x means "none".
* **Edge-triggered key**: the key flag is set this frame and was clear last frame. R is
  G+0x0f (previous frame 0x76b138); E is G+0x11 (previous 0x76b139); see
  `alpha-ui-screens.md` §11.1.
* "Look mode" = no window, map or menu holds the cursor (G's method 0x47f1d0 returns
  false).

---

## 1. The three targets (game object state)

The interaction target is not one value but three, all rebuilt every frame by the update
0x488ee0 (block 0x4984fb … 0x499db3) and all reset at start-up (0x45a502 … 0x45a54e):

| Field | Type | Meaning | Reset value |
|---|---|---|---|
| G+0x800a70 | u64 | **creature target**: id of the creature under the crosshair | 0 |
| G+0x800a78 | 3 × i32 | **item target**: (zone x, zone y, index) of a world item lying on the ground (zone + 0x30 list, 0x148-byte records; resolved by 0x59fb90) | (−1, −1, 0) |
| G+0x800a84 | 3 × i32 | **object target**: (zone x, zone y, index) of a static (zone + 0x0c vector; resolved by **0x5a0910**) | (−1, −1, 0) |

**0x5a0910** (object lookup): find the zone (zx, zy) with 0x434d10 (W's region table:
region (zx >> 6, zy >> 6), zone pointer at region + 0x10018 + 4·(64·(zx & 63) + (zy & 63)),
zones outside 0 … 65535 → none); if it exists and 0 ≤ index < (number of statics in the
zone), return the address of static *index*; otherwise none. 0x59fb90 does the same for
the zone's world-item list (stride 0x148).

At most one of the creature and object targets is meaningful at a time (selecting either
clears the other, §2.5); the item target is independent and is used only by **E**.

---

## 2. Choosing the targets each frame (0x4984fb … 0x499db3)

### 2.1 Order of work

1. Clear all three targets (creature := 0, item := (−1, −1, 0), object := (−1, −1, 0)) and
   the frame's working values: **best camera distance** *B* := −1, **best player
   distance** *Fd* := −1 (fall-back for creatures and statics), *Fi* := −1 (fall-back for
   items), fall-back creature id := 0, fall-back object := none, fall-back item := none.
2. Creature pass (§2.2).
3. If a speech/dialogue bubble is open (G+0x800940 ≠ 0): creature target := the bubble's
   creature (bubble + 0x188 → its id). Talking keeps the NPC targeted.
4. (Aim-point and lock-on bookkeeping for combat, not part of interaction.)
5. Static pass (§2.3).
6. If P's health ≤ 0: creature target := 0 and object target := (−1, −1, 0) (0x499545).
   (The item target is not cleared here; the static pass has already run.)
7. Item pass (§2.4).
8. Fall-backs (§2.5).

"Under the crosshair" in every pass is the **box test** of §2.6.

### 2.2 Creature pass (0x498690 … 0x498d03)

Candidates are the creature list rebuilt earlier in the frame (`alpha-ui-screens.md`
§5.4.1 item 6): every creature with health > 0 within **60 blocks** of P (squared
distance ≤ 3600). For each candidate *c*, in that list's order:

1. Skip P itself. While P's action is 34 and its action time is below the action's
   length, skip creatures of controller kind 1 or 6.
2. **Line of sight** from the camera position (G+0x140) to *c* (drawn position
   +0x1350, lowered by +0x1180): clear within 256 blocks, or in darkness
   (light ÷ 255 × the day factor < 0.3) within light·0.9/0.3·256 + 25.6 (0x59ee90,
   blocks only). Not visible → skip.
3. Box test (§2.6) on a box of size (*c*.size + 1) in each axis, centred on the drawn
   position lowered by +0x1180.
4. *dp* = distance from P (+0x10) to *c* (+0x1350). If *dp* **< 16** and (*Fd* < 0 or
   *dp* < *Fd*): *Fd* := *dp*, fall-back creature := *c*, fall-back object := none.
5. If the box test passed: *c* is **focused** this frame (byte *c*-entry + 8 := 1, which
   shows its life bar, `alpha-ui-screens.md` §5.4). *dc* = distance from the camera
   (G+0x140) to *c* (+0x1350). If *B* < 0 or *dc* < *B*: *B* := *dc*, creature target :=
   *c*.id.

There is **no distance limit** on the creature target other than the 60-block list and the
line of sight (as read; see open point I-1).

### 2.3 Static pass (0x498ed1 … 0x499530)

Let (*cx*, *cy*) = P's block position divided by 8 (towards zero): P's cell. For cells
*X* = *cx* − 1 … *cx* + 1 (outer), *Y* = *cy* − 1 … *cy* + 1 (inner), take the cell's list
from the zone's **cell index** (0x42f640: zone of cell = (X >> 5, Y >> 5); list at
zone + 0xac + 8·(32·(Y & 31) + (X & 31))); each list node holds a pointer to a static and
that static's (zx, zy, index) triple (§4.3). For each node, in list order:

1. *dp* = distance from P (+0x10) to the static's position (+0x08). If *dp* **> 16**: skip.
   (Every kind is a candidate: tables, vases, fences, runestones alike.)
2. *dc* = distance from the camera (G+0x140) to the static's **centre** = position +
   (0, 0, scale.z / 2).
3. Fall-back: if *Fd* < 0 or *dp* < *Fd*: *Fd* := *dp*, fall-back object := this triple,
   fall-back creature := 0.
4. Only if *B* < 0 or *dc* < *B*:
   * line of sight from the camera to the centre within **200** blocks (0x59ee90, blocks
     only); not visible → skip;
   * box: size = the static's scale (+0x24, +0x28, +0x2c), with x and y **swapped when the
     rotation index (+0x20) is odd**; corners at position + (i·sx − sx/2, j·sy − sy/2,
     k·sz) for i, j, k ∈ {0, 1} (x and y centred, z from the base up);
   * if the box test passes: *B* := *dc*, object target := this triple, creature target
     := 0.

Creatures and statics therefore compete on one value, the camera distance *B*: whichever
box under the crosshair is nearest the camera wins; on equal distance the earlier one
(creatures first) stays.

### 2.4 Item pass (0x499594 … 0x499d16)

For the 3 × 3 zones around P's current zone (G+0x2bc, G+0x2c0; zx outer, zy inner) and
each world item *k* (index 0, 1, …) of the zone's list (zone + 0x30):

1. Skip unless the item may be picked up by P (0x43e550: an item of category 25 must have
   its word +0x04 equal to *v* = P's +0x1164, where *v* > 100 is first replaced by
   100 + (*v* mod 100); every other category passes).
2. *q* = **squared** distance from P to the item (+0x118). Skip if *q* > 16 (more than
   4 blocks). (A second test, *q* > 100, can never fail after the first.)
3. *qc* = squared distance from the camera to the item.
4. Can P carry it (0x43e4a0: for categories 1, 11 and 20, count the matching items P
   already carries; refuse if that count is > 49, or if the category is 20 and the count
   is > 0)? If not, add a **penalty of 16** to both *q* and *qc*.
5. Fall-back: if *Fi* < 0 or *q* < *Fi*: *Fi* := *q*, fall-back item := (zx, zy, *k*),
   fall-back object := none, fall-back creature := 0.
6. Only if *B* < 0 or *qc* < *B*: line of sight from the camera within 200 blocks; the item
   must have a model (0x4ec400); box = the model's bounds placed with the item's position,
   rotation and scale (0x4122c0 … 0x47b5f0); if the box test passes: *B* := *qc*, item
   target := (zx, zy, *k*), object target := (−1, −1, 0), creature target := 0.

Note that this pass compares **squared** camera distances with *B*, which may hold a
**linear** distance from the earlier passes (open point I-2).

### 2.5 Fall-backs (0x499d1c … 0x499dae)

* If the creature target is 0 **and** the object target is none: creature target := the
  fall-back creature (if any), and if the fall-back object exists, object target := it.
* Independently: if the item target is none and the fall-back item exists, item target :=
  it.

So standing within 16 blocks of a creature or static targets the nearest one even when
the crosshair points elsewhere, and any pick-up-able item within 4 blocks becomes the E
target without aiming. An item fall-back cancels the creature/static fall-back of the same
frame (step 5 of §2.4).

### 2.6 The box test

Each of the 8 corners is moved into view space with the camera (G+0x26c; corners are first
offset by the render origin G+0x1d8/G+0x1e0, as everywhere in the frame). Corners with view
z ≤ 0 are ignored; the others are projected with G+0x800a90 (projection × view) to
normalised screen coordinates (x, y). The test passes when the projected corners include
one with x < 0, one with x > 0, one with y < 0 and one with y > 0 — that is, the projected
corner set straddles the **screen centre** (the crosshair) in both axes. No ray or
triangle test is made; the test uses the box only.

### 2.7 When the targets are empty

* Every frame before the passes (§2.1 step 1); nothing persists between frames.
* While P's health ≤ 0 the creature and object targets are emptied (§2.1 step 6), so the
  only prompt is "[R] Revive".
* A target whose zone is unloaded resolves to "none" in 0x5a0910 / 0x59fb90 and is
  ignored.

---

## 3. The prompt (0x4968d0 … 0x496f37)

The prompt is the `name` text of the `crosshair` clone G+0x8008ac (placement and font in
`alpha-ui-screens.md` §5.8). Every frame it is first set to "", then, in this order (a
later assignment replaces an earlier one):

1. **Dead**: if P's health ≤ 0 → "[R] Revive".
2. **Creature target** (looked up in W's creature map, 0x42f000):
   * controller kind **3** (NPC) and look mode → **"[R] Talk"**;
   * controller kind **5** (pet), its type (+0x64) is rideable (0x444760: types 19, 20,
     22, 23, 25–40, 63–67, 74, 75, 98–100, 102–105, 151), P has a riding item equipped
     (P+0x113c ≠ 0), the pet's owner id (+0x198) is P's id, and P's action is not 106
     (riding) → **"[R] Ride"**;
   * otherwise no text. (When a creature is found the object branch is not tried.)
3. **Otherwise the object target** (0x5a0910), by static kind:

| Static kind | Prompt | Suppressed when |
|---|---|---|
| 1 door, 2 big-door, 3 window, 10 chest | **"[R] Open"** if the state byte (+0x30) ≠ 0, **"[R] Close"** if it is 0 | — |
| 16 stool, 18 bench | "[R] Sit Down" | the static's occupant (+0x40, u64) is P |
| 19 bed, 68 beach-towel, 69 wood-mat | "[R] Sleep" | occupant is P |
| 45 runestone | **"[R] Teleport"** | occupant is P (never in practice) |
| 71 furnace | "[R] Use Furnace" | occupant is P |
| 72 anvil | "[R] Use Anvil" | occupant is P |
| 73 spinning wheel | "[R] Use Spinning Wheel" | occupant is P |
| 74 loom | "[R] Use Loom" | occupant is P |
| 75 sawbench | "[R] Use Saw" | occupant is P |
| 76 workbench | "[R] Use Workbench" | occupant is P |
| 77 customization bench | "[R] Use Customization Bench" | occupant is P |
| every other kind (0, 4–9, 11–15, 17, 20–44, 46–67, 70) | "[R] Examine" | — |

(Jump tables 0x49d4ac / 0x49d47c; strings 0x701bd0 … 0x701d80.) There is **no prompt for
items**; the item target is shown only by the highlight of §5.4.

---

## 4. Static objects

### 4.1 Record (0x188 bytes; constructor 0x58eaa0, copy 0x58ea10)

| Offset | Type | Meaning | Default |
|---|---|---|---|
| +0x00 | i32 | **kind** (0 … 77, §4.2) | — |
| +0x04 | — | (unused here) | — |
| +0x08, +0x10, +0x18 | 3 × i64 | position (fx); x, y at the footprint centre, z at the base | — |
| +0x20 | i32 | rotation index (quarter turns; the town builder stores −r for some stations) | — |
| +0x24, +0x28, +0x2c | 3 × f32 | scale = box size in blocks (x, y, z) | — |
| +0x30 | u8 | **state**: 1 closed (default), 0 open, 2 closed chest with contents | 1 |
| +0x34 | i32 | **state timer**, ms since the last change (§6.3); spike traps get a random start phase d % 4000 | 0 |
| +0x38 | i32 | *unknown* | 0 |
| +0x40 | u64 | **occupant**: id of the creature sitting/sleeping on it | 0 |
| +0x48 | 12 bytes | **contents**: a list of item lists (chest loot) | empty |
| +0x54 | i32 | *unknown* | — |
| +0x58 | 280 bytes | an item record (0x43c020) | empty item |
| +0x170, +0x174 | 2 × i32 | *unknown* | — |
| +0x178 | i32 | **link index**: index of a partner static in the same zone (levers, runestones; §6.2) | −1 |
| +0x17c, +0x180 | 2 × i32 | *unknown* | −1, −1 |
| +0x184 | i32 | *unknown* | 0 |

Statics are created only by world generation (the structure and vegetation passes insert
them with 0x5a0e00 into zone + 0x0c), so every client and the server generate the same
list from the seed; only the state byte (and in section 6 a few more words) is saved or
sent.

### 4.2 Kinds and models (table G+0x800724, filled at start-up 0x462c02 …)

The drawer takes the model of kind *k* from a 78-entry table G+0x800724, filled from the
model registry (slot numbers of `alpha-worldgen-structures.md` §0):

| Kind | Model (slot) | Kind | Model (slot) | Kind | Model (slot) |
|---|---|---|---|---|---|
| 0 | goddess2 (2481) | 26 | open-crate (2507) | 52–55 | fence01–04 (2531–2534) |
| 1 | door (2482) | 27 | sack (2508) | 56–64 | flower-vase1–9 (2322–2330) |
| 2 | big-door (2483) | 28 | shelter (2509) | 65 | campfire (2554) |
| 3 | window (2484) | 29 | cupboard (2501) | 66 | tent (2555) |
| 4 | castle-window (2485) | 30 | desktop (2510) | 67 | beach-umbrella (2556) |
| 5 | gate (2486) | 31 | counter (2511) | 68 | beach-towel (2557) |
| 6 | — (no model) | 32–34 | shelf1–3 (2512–2514) | 69 | wood-mat (2558) |
| 7 | spike-trap (2487) | 35–37 | castle-shelf1–3 (2515–2517) | 70 | — (no model) |
| 8 | stomp-trap (2488) | 38–40 | stone-shelf1–3 (2518–2520) | 71 | furnace (2535) |
| 9 | lever (2489) | 41–43 | sandstone-shelf1–3 (2521–2523) | 72 | anvil (2536) |
| 10 | chest-base02 (2490) | 44 | corpse (2524) | 73 | spinningwheel (2540) |
| 11 | chest-top02 (2491) — the chest lid | 45 | **runestone (2525)** | 74 | loom (2541) |
| 12 | table (2492) | 46 | artifact (2079) | 75 | sawbench (2537) |
| 13 | stone-table (2493) | 47–49 | flower-box01–03 (2526–2528) | 76 | workbench (2538) |
| 14 | — (no model) | 50 | street-light02 (2529) | 77 | customization-bench (2539) |
| 15 | stone-stool (2495) | 51 | street-light01 (2530) | | |
| 16 | stool (2497) | 17 | sandstone-stool (2496) | 18 | bench (2498) |
| 19 | bed (2499) | 20 | bedtable (2500) | 21–23 | market-stand1–3 (2502–2504) |
| 24 | barrel (2505) | 25 | crate (2506) | | |

A kind with no model (6, 14, 70) is not drawn but still exists, can be targeted (its box
is its scale) and is simulated (kind 6 is a trap, §6.4).

### 4.3 The cell index

Besides the vector, each zone keeps a 32 × 32 grid of lists (zone + 0xac, 8-byte heads),
one per 8 × 8-block cell; a node holds {static pointer, zone x, zone y, index}. The target
pass (§2.3), the "nearest static" helper 0x5a5f60 (§6.2) and the draw use it. *Believed*:
every static is listed in the cell that contains its position, which may belong to a
neighbouring zone's grid (nodes carry their own triple); the builder was not traced (open
point I-4). An implementation may use any spatial index that returns the statics of the
3 × 3 cells around a point.

### 4.4 Drawing (0x4b3097 … 0x4b3290 → 0x4be760)

Each static is drawn with its kind's model, translated to its position, rotated about z
by rotation index · 90°, scaled so that the model's size matches the record's scale, lit
by the light value of the block at its position (block light byte ÷ 255, at least 5/255 in
air or water, 1 in kind-13 blocks; street lights 50 and 51 always at full light) and faded
out with distance. Moving parts by kind, with *p* the open fraction of §6.3:

| Kind | Moving part |
|---|---|
| 1, 2, 3 | the whole model turns about z by *p*·90° about a hinge at one edge (half the model width) |
| 5 gate | raised by 0.8·scale.z·*p* |
| 7 spike trap | moved by −(scale.z·*p*) − 1 (spikes come up as *p* falls) |
| 8 stomp trap | moved by scale.z·*p* |
| 9 lever | tilted by (60·*p* − 30)° |
| 10 chest | the base, then the lid (kind 11's model) opened by *p*·80° about its back edge |

Kinds 50/51 (street lights) and 51/65 (torch light, campfire) also add point lights
(`alpha-rendering.md` §10.1); kind 65 also emits fire particles at position + (0, 0,
scale.z/2) (0x4bbd80).

**Highlight.** The static that is the object target is drawn with an extra material
colour of (0.1, 0.1, 0.1, 0.1); the world item that is the item target with
(0.25, 0.25, 0.25, 0.25) (0x4b30ab, 0x4b2b7c). A focused creature shows its life bar
instead.

### 4.5 Saving and networking

* **Save**: the zone save stores one byte per static, the state byte +0x30, in vector order
  (`alpha-saves.md` §5, item 5). Occupants, timers and contents are not saved; a chest's
  contents are regenerated with the zone (so a chest saved as state 2 refills on load —
  *inferred*).
* **Network**: every state change made by the simulation is put in the event set as a
  section-6 record (0x4dc8b0 → event list + 0x30): zone x, zone y, index and the static's
  words, including the state byte (`alpha-net-packets.md` §3.6). The server also sends the
  states of the zones a client requests. A client applies them to its own generated
  statics. Online clients never change a static themselves (§6).

---

## 5. Pressing R (0x496994 … 0x497653) and E

All R actions require: R edge-triggered, the quick-item wheel closed (G+0x800a40 = 0).

### 5.1 Dead: revive

If P's health ≤ 0: reset P's transient state, move P to a respawn point and restore full
health (`alpha-ui-screens.md` §5.8). Nothing else happens on that press.

### 5.2 Creature target

Two blocks run on the same press, in this order.

**a) From the prompt block** (only when its prompt was shown):

* **Talk** (NPC, look mode): if a speech bubble is open and has finished typing, the press
  closes it (bubble's creature := none, hidden, G+0x800940 := 0). Otherwise:
  1. queue a packet-6 record of **kind 2** whose target is the NPC's **home triple**
     (creature +0x1b0: home zone x, zone y, spawn index) — the request of
     `alpha-server-simulation.md` §8 kind 2 (inn rest at night);
  2. open or advance the NPC's speech bubble (0x4882e0): an innkeeper (class 132) uses the
     dictionary text `innkeeper`; other NPCs show their quest dialogue (quest
     specification). A second press while the text is still typing completes it; a press
     after that shows the next page.
* **Ride** (own rideable pet): P's action := **106** (riding), action time := 0. No packet.

**b) Always** (0x4889e0): G+0x8008d8 := the creature target (the "talk partner"), then by
the NPC's class byte (+0x140):

| Class | Effect |
|---|---|
| 128, 129, 130 (vendors) | open the vendor pair: windows G+0x8008c4 (vendor stock) and G+0x8008bc (inventory) shown; the vendor's inventory (+0x11dc), its gold (+0x11e8), its item (+0x11ec) and two words (+0x1304, +0x1308) are copied into the game's vendor state (G+0x800c18, G+0x800d34/38). Contents: inventory/vendor specification |
| 131 | window G+0x8008f8 shown; G+0x8008fc reset (selection (−1, −1), flag 1) — *believed* the trainer/skill-reset window |
| 137 | window G+0x800900 shown — *believed* the pet/stable window |

If there is **no** creature target: the nearest **pet** (controller kind 5) within
5 blocks (squared distance < 25) whose owner id is all ones (no owner) is linked to P
(P+0x198 := its id) — *believed*: claiming a free pet.

### 5.3 Object target (0x4973eb … 0x4975b4)

If the object target resolves (0x5a0910), by kind:

| Kind | Effect | Packet 6 |
|---|---|---|
| **45 runestone** | map mode := 1 and **teleport mode** := 1 (`alpha-ui-screens.md` §5.2.1); the portal is then chosen and entered on the map (§5.2.6 there: two clicks on a "Portal of *N*" label move P to that zone's centre at height 0) | none |
| 77 customization bench | prepare the enchant/adaption widget (G+0x8008f4, 0x58ce20); toggle the preview window G+0x8008dc; the inventory G+0x8008bc follows its visibility | none |
| 65 campfire, 71 furnace, 72 anvil, 73 spinning wheel, 74 loom, 75 sawbench, 76 workbench | toggle the crafting window G+0x8008c0; campfire also selects crafting tab **3** ("Cooking"); furnace, spinning wheel and sawbench select tab **5**; then the recipe list is refreshed (0x4a14c0). Which station is "near" for "Requires …" is the crafting window's rule | none |
| every other kind | if a speech bubble is open it is closed first (if finished). Queue a packet-6 record of **kind 3** with target = the object triple (§6). Then, unless the kind is 1, 2, 3, 10, 16, 18, 19 or 69, P's speech bubble says **"There is nothing special."** (0x488030; a second press completes/advances it like Talk) | kind 3 |

So "[R] Examine" on a table, vase or lever sends kind 3 **and** shows the bubble;
"[R] Open"/"[R] Sit Down"/"[R] Sleep" send kind 3 silently (a beach towel, kind 68, sends
kind 3 and also shows the bubble).

### 5.4 E: pick up (0x4975c1 … 0x4976e2)

If E is edge-triggered and the item target resolves (0x59fb90):

* if P can carry it (0x43e4a0): queue a packet-6 record of **kind 5** with target = the
  item triple (the server or the offline step then checks the 4-block range and moves the
  item, `alpha-server-simulation.md` §8 kind 5);
* otherwise: chat message "You can't carry more of these items." in (1, 0.2, 0.2, 1) and
  sound 49 at P's position (volume 1, pitch 1).

### 5.5 The packet-6 record (0x459530 + caller)

The client builds a 300-byte record: item (280 bytes, zeroed, count 1), target triple at
0x118 (default (−1, −1, −1)), a word at 0x124 = 0, **kind byte at 0x128**, a u16 at 0x12a
= 1; then appends it to P's queue (P+0x130c, 0x486100). Online the queue is sent as packet
6 (`alpha-net-packets.md` §4.1); offline the client's own step handles it (§6). Kinds used
here: 2 talk, 3 use static, 5 pick up.

---

## 6. Using a static: packet-6 kind 3 (client step 0x60c510, case 0x61094a; server 0x5489e0 case 3)

Run by whoever owns the simulation (the server, or an offline client) for creature *C*
whose queue held the record. An online client skips every part marked *(owner)*: it only
waits for the section-6 answer.

### 6.1 Steps

1. If *C*'s action is 79 (sneaking): action := 0 and translucency (+0x1190) := 0. Using
   anything ends sneaking.
2. Resolve the target triple; none → done.
3. **Kind 45** (runestone), only when *C* is the local player of that step: §6.2.
4. **Seats and beds** — kinds 16, 18 (seat) and 19, 68, 69 (bed): if the occupant is 0
   and *C*'s action is 0, 83 or 84: *(owner)* occupant := *C*.id; *C*'s position := static
   position + (0, 0, *C*.size.z/2 + scale.z); *C*'s motion vectors (+0x34, +0x40) := 0;
   *C*'s yaw (+0x28 z) := (rotation + 2)·90 for a seat or rotation·90 for a bed; action :=
   **83 (sit)** or **84 (sleep)**; a section-6 record of the static is emitted. In all
   these cases *C*'s +0x40 vector is then zeroed. Done.
5. Otherwise *(owner)*:
   * **Chest with contents** (kind 10, state 2): every item of every list in +0x48 is
     dropped as a world item at the chest's position + (2 − 4*r*₁, 2 − 4*r*₂, 0) blocks
     with angle 360·*r*₃ (three `rand()` draws per item, *r* = draw / 32767, order
     x, y, angle), then the lists are emptied (0x59aeb0).
   * **Kinds 1, 2, 3, 9, 10**: set the state to (state = 0 ? 1 : 0) — closed ↔ open; a
     chest in state 2 becomes 0 (open) — with the state setter (§6.3), and emit a
     section-6 record.
   * **Kind 9 (lever)**: if its link index +0x178 is a valid index in the same zone, set
     that static's state to (lever state ≠ 0 ? 1 : 0) and emit its section-6 record.
   * Every other kind: nothing.

Non-player creatures also use statics: town NPCs pick a random free entry (occupant 0) of
one of their house lists — one list at night (world time before 6:00 or after 22:59:59),
the other by day (*believed*: beds at night, seats by day) — walk to it and, within
3 blocks (squared distance < 9), queue kind 3 on it themselves (0x4db130), which seats or
beds them by step 4.

### 6.2 The runestone link (kind 45)

The step finds the static nearest to *C* (0x5a5f60: 3 × 3 cells around *C*, any kind,
squared distance ≤ 16, nearest wins, first on ties), takes **that** static's zone and the
used runestone's link index +0x178, and if that index is valid: plays sound 47 (volume 1)
at *C*'s position and moves *C* to that static's position + (0, 0, 2) blocks.

The client never sends kind 3 for a runestone (R opens the map instead, §5.3), and no
world-generation routine traced so far sets +0x178, which stays −1; so this path is inert
in Alpha (open point I-3). **Portal travel in Alpha is the map teleport only.**

### 6.3 The state setter (0x5a58e0) and the timer

set(static, new):
* if the state already equals *new*: nothing;
* otherwise: timer := max(1000 − timer, 0), state := *new*, and a positional sound event
  at the static's position (volume 1, pitch 1): kind 5 (gate) → sound **51**; kind 6 →
  **53** (only when *new* ≠ 0); kind 7 → **52** (only when *new* ≠ 0); any other kind →
  **54**.

The owner's step adds the frame time to every active static's timer (+0x34 += dt, 0x60d3c3).
The draw (§4.4) turns timer and state into the open fraction *p*:

* state 0 (open): *p* = min(timer·0.001, 1);
* state ≠ 0 (closed): *p* = max(1 − timer·0.001, 0) — for kinds 7 and 8 closing uses
  0.01 per ms (ten times faster).

So a change always animates over 1 s from wherever the previous animation was (the
1000 − timer rule mirrors an unfinished movement). On clients that do not simulate
(online), the timer is part of the section-6 words (*inferred*).

### 6.4 Occupants and traps (owner step, 0x60d3b0 …; outline)

* **Occupant check** each step, for a static with an occupant: the seat is freed
  (occupant := 0, section-6 record) when the occupant no longer exists, or its +0x12c is
  positive, or it is more than 4 blocks away (squared > 16), or it moves (+0x40 non-zero),
  or its action is neither 83 nor 84.
* **Traps** (kinds 6, 7, 8): after 2 s in a state (timer > 2000), an extended trap (state
  ≠ 0) retracts (state 0); a retracted one extends again when a player creature
  (controller 0) comes within range (squared distance test against 900). Extended traps
  of kinds 6/7 hurt creatures other than hostile NPCs within 5 blocks (squared 25) every
  100 ms of world time; kind 8 when its timer is 0. Damage values and exact geometry are
  not traced (open point I-5).

---

## 7. Portals end to end

1. **Worldgen.** Every zone whose per-zone entry has kind 4 (a type-10 site,
   `alpha-worldgen-locations.md` §7) gets a ring of 6–8 rock pillars at radius 25 around
   the zone centre and **one runestone** (`alpha-worldgen-structures.md` §4 step 5): kind
   **45**, at (cx + 3.5, cy + 3.5) blocks with (cx, cy) = (256·zx + 128, 256·zy + 128),
   settled onto the ground, rotation d & 3, scale **(4, 4, 5)**. The map labels such a zone
   "Portal of *N*" (`alpha-ui-screens.md` §5.2.5).
2. **Targeting.** Within 16 blocks of the stone, aiming at its 4 × 4 × 5 box (or being
   nearest to it with nothing aimed at) makes it the object target; the prompt reads
   **"[R] Teleport"** and the stone is drawn highlighted.
3. **R** opens the world map with teleport mode on. Portal labels turn purple; at map
   zoom > 2 they can be hovered (cyan), clicked once to select (pink), and clicked again
   to teleport: P's position := ((256·zx + 128)·65536, (256·zy + 128)·65536, 0), the map
   closes. Any discovered portal zone can be chosen, including the one P stands in.
4. Nothing is sent to the server: the teleport is a local position change, which reaches
   other players through P's normal creature updates.

---

## 8. Worked examples

1. **Runestone of zone (32800, 32801).** cx = 256·32800 + 128 = 8,396,928; cy =
   256·32801 + 128 = 8,397,184. The stone stands at x = 8,396,928·65536 + 229,376 →
   **8,396,931.5** blocks, y → **8,397,187.5**, z = the settled ground height there. Its
   target box spans x 8,396,929.5 … 8,396,933.5, y 8,397,185.5 … 8,397,189.5, z from its
   base to base + 5 (rotation does not matter: the footprint is square). Teleporting *to*
   this portal puts P at (8,396,928, 8,397,184, 0), √(3.5² + 3.5²) = **4.95** blocks
   south-west of the stone, at height 0, from where physics pushes P up out of the ground.
2. **Which target wins.** P stands 10 blocks from a door (kind 1, state 1) and 12 blocks
   from an NPC behind it; the crosshair covers both boxes; camera distances: door 13.2,
   NPC 15.0. The creature pass sets *B* = 15.0 and creature target = NPC; the static pass
   finds 13.2 < 15.0, so object target := door, creature target := 0. Prompt "[R] Open".
   Pressing R queues kind 3 for the door; the owner sets state 0 (timer 25,000 → 0), plays
   sound 54; the door swings to 90° over 1 s; the prompt reads "[R] Close".
3. **Out of reach.** A table 17 blocks from P is never an object target (*dp* > 16), even
   under the crosshair; nothing is shown.
4. **Nothing aimed at.** Crosshair at the sky; a stool 3 blocks away and an NPC 7 blocks
   away: fall-back by player distance → object target = stool ("[R] Sit Down"), creature
   target = 0. If a pick-up-able item also lies 1.5 blocks away (squared 2.25 ≤ 16), the
   item fall-back clears the stool and NPC fall-backs: no prompt; E picks the item up.
5. **Sitting.** Stool (kind 16) at z = 100 blocks, scale z 0.5, rotation 1; P's height
   1.8. R → kind 3 → P at z = 100 + 0.9 + 0.5 = **101.4**, yaw (1 + 2)·90 = **270**, action
   83. The prompt over the stool disappears (occupant = P). Walking away (velocity ≠ 0)
   frees the stool on the next step.
6. **Chest.** A dungeon chest (state 2, 3 items): R → the 3 items drop within ±2 blocks
   (x, y) of the chest, state 0, sound 54, lid opens to 80° in 1 s; prompt "[R] Close". R
   again closes it (state 1, empty).

---

## 9. Implementation notes for AlphaVulkan

* Keep the three targets separate (creature id, item triple, static triple) and rebuild
  them every frame in the order of §2.1; the prompt, R and E read them after the passes.
* Address statics only by (zone x, zone y, index) — this is what the network, the saves
  and NPC furniture lists use; the index order is world-generation order and must match
  the original exactly.
* The box test needs only the 8 projected corners; do not substitute a ray cast, or the
  "nearest box under the crosshair" behaviour (large stones winning over small objects in
  front of them) changes.
* Static state changes go through one setter (§6.3) that emits the section-6 record and the
  sound; that setter is the natural hook point for mods (new usable kinds).

---

## 10. Open points

* **I-1** The creature target has no 16-block limit when aimed (only the 60-block list and
  line of sight), so "[R] Talk" may appear for distant NPCs. Verify in the game.
* **I-2** The item pass compares squared camera distances with the linear best distance of
  the earlier passes; as read, items lose to statics and creatures more often than their
  real distance suggests.
* **I-3** Whether any generator writes the link index +0x178 (levers, runestones). None of
  the traced structure routines do; if none does, kind-3 runestone travel and lever links
  are dead code in Alpha.
* **I-4** The builder of the 8-block cell index (zone + 0xac) and whether a cell may list
  statics of a neighbouring zone.
* **I-5** Trap ranges (the 900 test), damage, and the per-kind occupant table
  (0x622bdc) of §6.4.
* **I-6** Classes 131 and 137 window identities (trainer and pet stable are guesses), and
  crafting tab 5's name.
* **I-7** The meaning of P+0x12c (frees seats) and of record words +0x38, +0x54,
  +0x170 … +0x184.
