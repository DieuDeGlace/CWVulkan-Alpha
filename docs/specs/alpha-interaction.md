Status: FINAL / STABLE / HELD

# Alpha interaction: targets, "[R]" prompts, static objects and portals

Behavioural specification of how the Cube World **Alpha** client decides what the player is
pointing at, which "[R] …" prompt it shows, what pressing **R** (and **E** for items) does
for each kind of target, and the static objects these targets come from: their record,
kinds, models, state, animation and use. Static kind 45 is the runestone that world
generation places in every type-10 site ("Portal of *N*" zones); pressing R at it opens the
world map in teleport mode.

**Images.** All addresses are RVAs. "Cube.exe" is the Alpha client, SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, image base 0x400000.
Server.exe (SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`) is
named only in §10. Every RVA below is in Cube.exe unless stated otherwise. Routines are
cited as references only.

Related (pointers only): `alpha-ui-screens.md` (§5.2 world map and teleporting, §5.4.1
creature fields and focus, §5.8 crosshair and prompt, §11 keys),
`alpha-worldgen-structures.md` (§0 model slots, §2.2 static record, §4 type-10 site),
`alpha-worldgen-locations.md` (§7 type-10 sites), `alpha-server-simulation.md` (§8 packet-6
kinds), `alpha-net-packets.md` (§3.6 section 6, §4.1 packet 6), `alpha-saves.md` (§5 zone
save), `alpha-rendering.md` (§10.1 static lights).

---

## 0. Conventions

* "G" is the game object, "W" the world object (G+0x2e4), "P" the local player creature
  (G+0x8006d0). Creature fields used here: +0x08 64-bit id, +0x10 position, +0x28 rotation
  vector (z component = yaw), +0x34 and +0x40 motion vectors, +0x60 controller-kind byte,
  +0x64 type, +0x68 action byte, +0x6c action time, +0x80 size vector, +0x140 class byte,
  +0x16c health, +0x198 owner/partner id (64-bit), +0x1180 drawn-height offset, +0x1190
  translucency, +0x1350 drawn position. Controller-kind values that occur here: 0, 1, 3,
  5, 6 (labels "player", "hostile NPC", "NPC", "pet", "inert" per `alpha-ui-screens.md`
  §5.4.1).
* Positions are 64-bit integers in 1/65536 block. A **zone** is 256 × 256 blocks; a
  **cell** is 8 × 8 blocks (32 × 32 cells per zone). Unless "linear" is said, every
  distance compared in this document is a **squared** distance in blocks²: the client's
  distance helper at RVA 0x3b4a0 returns the dot product of a fixed-point vector with
  itself.
* A **static** (static object) is a 0x188-byte (392-byte) record in a zone's static vector
  (zone + 0x0c). It is addressed by the triple **(zone x, zone y, index)**, three 32-bit
  integers. A triple whose zone x is negative means "none" (the cleared value is
  (−1, −1, 0)).
* **Edge-triggered key**: the key byte is set this frame and its previous-frame copy is
  clear. R is G+0x0f (previous copy at RVA 0x36b138); E is G+0x11 (previous copy at RVA
  0x36b139). Both copies are refreshed at RVA 0x9b593 … 0x9b59e.
* **Look mode**: G's virtual method in slot 1 (RVA 0x7f1d0; vtable at RVA 0x2ffdc8) returns
  false, meaning no window, map or menu holds the cursor. The frame update evaluates it
  once at RVA 0x91f4b and the Talk prompt reads that result.

---

## 1. The three targets

The interaction target is three separate values, all rebuilt every frame by the frame
update (RVA 0x88ee0, block 0x984fb … 0x99db3) and all set at game-object construction
(RVA 0x5a502 … 0x5a54e, inside the constructor at RVA 0x59c40):

| Field | Type | Meaning | Reset value |
|---|---|---|---|
| G+0x800a70 | u64 | **creature target**: id of a creature | 0 |
| G+0x800a78 | 3 × i32 | **item target**: triple of a world item on the ground (zone + 0x30 vector, 0x148-byte records; resolved by RVA 0x19fb90) | (−1, −1, 0) |
| G+0x800a84 | 3 × i32 | **object target**: triple of a static (zone + 0x0c vector; resolved by RVA 0x1a0910) | (−1, −1, 0) |

**Object lookup (RVA 0x1a0910).** Find zone (zx, zy) with RVA 0x34d10: zones with either
coordinate outside 0 … 65535 are none; otherwise the region (zx ÷ 64, zy ÷ 64) is found
(RVA 0x34a90) and the zone pointer read at region + 0x10018 + 4·(64·(zx mod 64) +
(zy mod 64)). If the zone exists and 0 ≤ index < number of statics, the result is static
*index*; otherwise none. RVA 0x19fb90 does the same over the zone's world-item vector
(zone + 0x30, stride 0x148).

Selecting a creature or a static clears the other (§2); the item target is independent
and is used only by **E**.

---

## 2. Choosing the targets each frame (RVA 0x984fb … 0x99db3)

### 2.1 Order of work

1. Clear the three targets (creature := 0, item := (−1, −1, 0), object := (−1, −1, 0)) and
   the frame's working values: best camera distance *B* := −1; best player distance
   *Fd* := −1 (fall-back for creatures and statics); *Fi* := −1 (fall-back for items);
   fall-back creature id := 0; fall-back object := (−1, −1, 0); fall-back item :=
   (−1, −1, 0).
2. Creature pass (§2.2).
3. If a dialogue bubble is open (G+0x800940 ≠ 0): creature target := the id of the
   bubble's creature (bubble + 0x188), RVA 0x98d21. The later passes can still replace it.
4. Combat aim and lock-on bookkeeping (not part of interaction).
5. Static pass (§2.3).
6. If P's health ≤ 0: creature target := 0 and object target := (−1, −1, 0) (RVA 0x99545).
7. Item pass (§2.4).
8. Fall-backs (§2.5). They run whatever P's health is.

"Under the crosshair" in every pass means the **box test** of §2.6.

### 2.2 Creature pass (RVA 0x98690 … 0x98d03)

**Candidates.** Earlier in the same frame (RVA 0x908cf … 0x909f4) the client walks W's
creature map and lists every creature with health > 0 whose drawn position (+0x1350) is
within squared distance 3600 (60 blocks) of P's position (+0x10). Each entry holds the
creature pointer, that squared distance and a focus byte (0). The list is then sorted on
the stored squared distance with a non-stable sort (RVA 0x58ba0, called at RVA 0x90a31).
The pass visits the entries in sorted order. For each candidate *c*:

1. Skip P itself. While P's action is 34 and its action time is below the value from
   RVA 0x3d1a0, skip creatures of controller kind 1 or 6.
2. **Line of sight** from the camera position (G+0x140) to the point *c*.+0x1350 lowered
   by *c*.+0x1180 (RVA 0x19ee90). The linear range is 256 blocks. Let *v* = (light of
   *c*'s block, RVA 0x718b0) ÷ 255 × *f*, with day factor *f* = (1 − (2*t* ÷ 86,400,000 −
   1)⁴)⁵ and *t* the world time (W+0x80015c, read by RVA 0x71910). If *v* < 0.3, the range
   becomes *v* × 0.9 ÷ 0.3 × 256 + 25.6. Not visible → skip.
3. Box test (§2.6) on a box of size (*c*.size + 1) on each axis, centred on all three axes
   on the same lowered point.
4. *dp* = squared distance from P (+0x10) to *c* (+0x1350). If *dp* < 16 (4 blocks) and
   (*Fd* < 0 or *dp* < *Fd*): *Fd* := *dp* and fall-back creature := *c*'s id.
5. If the box test passed: the entry's focus byte := 1 (the life-bar focus of
   `alpha-ui-screens.md` §5.4). *dc* = squared distance from the camera (G+0x140) to *c*
   (+0x1350). If *B* < 0 or *dc* < *B*: *B* := *dc* and creature target := *c*'s id.

No distance limit applies to the aimed creature target apart from the 60-block list and
the line of sight.

### 2.3 Static pass (RVA 0x98ed1 … 0x99538)

Let (*cx*, *cy*) be P's block x and y divided by 8, rounding towards zero. For cells
*X* = *cx* − 1 … *cx* + 1 (outer loop) and *Y* = *cy* − 1 … *cy* + 1 (inner loop), take
that cell's list from the zone cell index (RVA 0x2f640, §4.3). Each list node carries a
static pointer and that static's triple. For each node, in list order:

1. *dp* = squared distance from P (+0x10) to the static's position (+0x08). If *dp* > 16
   (beyond 4 blocks): skip. There is no kind filter.
2. *dc* = squared distance from the camera (G+0x140) to the static's **centre**, which is
   its position + (0, 0, scale.z ÷ 2).
3. Fall-back: if *Fd* < 0 or *dp* < *Fd*: *Fd* := *dp*, fall-back object := this triple and
   fall-back creature := 0.
4. Only if *B* < 0 or *dc* < *B*:
   * line of sight from the camera to the centre, linear range **200** blocks
     (RVA 0x19ee90); not visible → skip;
   * the box size is the static's scale (+0x24, +0x28, +0x2c), with x and y **swapped
     when the rotation index (+0x20) is odd**. Its corners are position +
     (*i*·sx − sx/2, *j*·sy − sy/2, *k*·sz) for *i*, *j*, *k* ∈ {0, 1}: centred in x and
     y, rising from the base in z;
   * if the box test passes: *B* := *dc*, object target := this triple and creature
     target := 0.

Creatures and statics compete on one value, the squared camera distance *B*. Whichever
box under the crosshair is nearest the camera wins. On equal distance the earlier one
(creatures first) stays.

### 2.4 Item pass (RVA 0x99594 … 0x99d16)

For the 3 × 3 zones around P's current zone (G+0x2bc, G+0x2c0; zone x outer, zone y
inner) and each world item *k* (index 0, 1, …) of the zone's vector (zone + 0x30):

1. Skip unless P may pick the item up (RVA 0x3e550). An item whose category byte (+0x00)
   is 25 passes only if its word +0x04 equals *v*. Here *v* is P's +0x1164, first replaced
   by 100 + (*v* mod 100) when *v* > 100. Every other category passes.
2. *q* = squared distance from P to the item position (+0x118). Skip if *q* > 16. A second
   test, *q* > 100, can never fail after the first.
3. *qc* = squared distance from the camera to the item position.
4. Carry check (RVA 0x3e4a0). Only categories 1, 11 and 20 are limited. The check sums the
   counts of matching items P holds: the item at P+0x11ec, weighted by P+0x11e8, plus
   every matching stack in P's inventory (P+0x11dc). It refuses when the category is 20
   and the sum is ≥ 1, or when the sum is ≥ 50. If refused, **16** is added to both *q*
   and *qc*.
5. Fall-back: if *Fi* < 0 or *q* < *Fi*: *Fi* := *q*, fall-back item := (zx, zy, *k*),
   fall-back object := (−1, −1, 0) and fall-back creature := 0.
6. Only if *B* < 0 or *qc* < *B*: line of sight from the camera to the item, linear range
   200 blocks; the item must have a model (RVA 0xec400). The box comes from the model's
   bounds, placed at the item's position and transformed by the item record (+0x130,
   +0x134), with category-specific cases for categories 3 … 13 (jump table RVA
   0x9d520 / 0x9d514; not restated here). If the box test passes: *B* := *qc*, item target
   := (zx, zy, *k*), object target := (−1, −1, 0) and creature target := 0.

All three passes compare squared distances against the same *B*.

### 2.5 Fall-backs (RVA 0x99d1c … 0x99dae)

* If the creature target is 0 **and** the object target's zone x is negative: if the
  fall-back creature id is non-zero, creature target := it; if the fall-back object's
  zone x is ≥ 0, object target := it.
* Independently: if the item target's zone x is negative and the fall-back item's zone x is
  ≥ 0, item target := the fall-back item.

Standing within 4 blocks (squared distance < 16 for creatures, ≤ 16 for statics) of a
creature or static therefore targets the nearest such object even when the crosshair
points elsewhere. Any item P may pick up within 4 blocks becomes the E target without
aiming. An item fall-back clears the creature and static fall-backs found earlier in the
same frame (step 5 of §2.4).

### 2.6 The box test

Each of the 8 corners is offset by the render origin (G+0x1d8 x, G+0x1e0 y) and moved
into view space with the camera transform G+0x26c (RVA 0x248a0). Corners whose view-space
z is ≤ 0 are ignored. The others are projected with G+0x800a90 (RVA 0x88d60) to normalised
screen coordinates (x, y). The test passes when the projected corners include one with
x < 0, one with x > 0, one with y < 0 and one with y > 0: the corner set straddles the
screen centre (the crosshair) on both axes. No ray or triangle test is made.

### 2.7 When the targets are empty

* All targets are cleared every frame before the passes (§2.1 step 1); nothing persists
  between frames.
* While P's health ≤ 0, step 6 empties the creature and object targets after the static
  pass. The item pass and the fall-backs still run afterwards and can set them again.
* A target whose zone is not loaded resolves to none in RVA 0x1a0910 / 0x19fb90 and is
  ignored by the prompt and the R/E actions.

---

## 3. The prompt (RVA 0x96890 … 0x96f37)

The prompt is the text of the crosshair label G+0x8008ac (placement and font in
`alpha-ui-screens.md` §5.8). It is first set to the empty string (RVA 0x96890). Then, in
this order, a later assignment replaces an earlier one:

1. **Dead**: if P's health ≤ 0 → "[R] Revive".
2. **Creature target** (looked up in W's creature map, RVA 0x2f000), if it exists:
   * controller kind **3** and look mode → **"[R] Talk"**;
   * otherwise controller kind **5**, with all of the following: its type (+0x64) is
     rideable (RVA 0x44760; types 19, 20, 22, 23, 25–40, 63–67, 74, 75, 98–100, 102–105,
     151); P+0x113c ≠ 0; the creature's +0x198 equals P's id; P's action is not 106 →
     **"[R] Ride"**;
   * when neither applies, the object branch below is evaluated.
   When Talk or Ride is shown, the object branch is skipped this frame.
3. **Object target** (RVA 0x1a0910), by static kind (byte table RVA 0x9d4ac indexed by
   kind − 1, jump table RVA 0x9d47c):

| Static kind | Prompt | Suppressed when |
|---|---|---|
| 1, 2, 3, 10 | **"[R] Open"** if the state byte (+0x30) ≠ 0, **"[R] Close"** if it is 0 | — |
| 16, 18 | "[R] Sit Down" | the static's occupant (+0x40, u64) equals P's id |
| 19, 68, 69 | "[R] Sleep" | occupant is P |
| 45 | **"[R] Teleport"** | occupant is P |
| 71 | "[R] Use Furnace" | occupant is P |
| 72 | "[R] Use Anvil" | occupant is P |
| 73 | "[R] Use Spinning Wheel" | occupant is P |
| 74 | "[R] Use Loom" | occupant is P |
| 75 | "[R] Use Saw" | occupant is P |
| 76 | "[R] Use Workbench" | occupant is P |
| 77 | "[R] Use Customization Bench" | occupant is P |
| every other kind (0, 4–9, 11–15, 17, 20–44, 46–67, 70, and any kind > 77) | "[R] Examine" | — |

The strings are UTF-16 at RVA 0x301bd0 … 0x301d80. There is **no prompt for items**; the
item target is shown only by its highlight (§4.4).

---

## 4. Static objects

### 4.1 Record (0x188 bytes; constructor RVA 0x18eaa0, copy RVA 0x18ea10)

| Offset | Type | Meaning | Constructor value |
|---|---|---|---|
| +0x00 | i32 | **kind** (0 … 77, §4.2) | not set |
| +0x08, +0x10, +0x18 | 3 × i64 | position (fixed point): x, y at the footprint centre, z at the base | not set |
| +0x20 | i32 | rotation index (quarter turns) | not set |
| +0x24, +0x28, +0x2c | 3 × f32 | scale = box size in blocks (x, y, z) | not set |
| +0x30 | u8 | **state**: 1 closed (constructor value), 0 open, 2 closed chest with contents | 1 |
| +0x34 | i32 | **state timer** in ms (§6.3) | 0 |
| +0x38 | i32 | *unknown* | 0 |
| +0x40 | u64 | **occupant**: id of the creature using it as a seat or bed | 0 |
| +0x48 | 0x130 bytes | compound initialised by RVA 0x3c020: four zero words at +0x48 … +0x57, the first 12 bytes being the contents vector used for chests (§6.1); an item record of 280 bytes at +0x58; two zero words at +0x170, +0x174 | empty |
| +0x178 | i32 | **link index**: index of a partner static in a zone (levers, runestones; §6.1, §6.2) | −1 |
| +0x17c, +0x180 | 2 × i32 | *unknown* | −1, −1 |
| +0x184 | i32 | *unknown* | 0 |

World generation inserts statics into zone + 0x0c with RVA 0x1a0e00 (85 call sites in
Cube.exe). Spike traps (kind 7) get a timer start value of one `rand()` draw mod 4000
(RVA 0x1d1bdd … 0x1d1c01).

### 4.2 Kinds and models (table G+0x800724)

At start-up (from RVA 0x62c02) G+0x800724 is sized to 78 null entries and filled from the
model registry. The slot numbers are those of `alpha-worldgen-structures.md` §0, and the
file names are those the registry loads into each slot:

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
| 11 | chest-top02 (2491), the chest lid | 45 | **runestone (2525)** | 74 | loom (2541) |
| 12 | table (2492) | 46 | artifact (2079) | 75 | sawbench (2537) |
| 13 | stone-table (2493) | 47–49 | flower-box01–03 (2526–2528) | 76 | workbench (2538) |
| 14 | — (no model) | 50 | street-light02 (2529) | 77 | customization-bench (2539) |
| 15 | stone-stool (2495) | 51 | street-light01 (2530) | | |
| 16 | stool (2497) | 17 | sandstone-stool (2496) | 18 | bench (2498) |
| 19 | bed (2499) | 20 | bedtable (2500) | 21–23 | market-stand1–3 (2502–2504) |
| 24 | barrel (2505) | 25 | crate (2506) | | |

Kinds 6, 14 and 70 have no model entry. Slot 2494 (sandstone-table) is loaded but not
assigned to any kind. A kind without a model is not drawn but still exists, can be targeted
(its box is its scale) and is stepped (kind 6 is a trap, §6.4).

### 4.3 The cell index

Each zone holds at zone + 0xac a pointer to an array of 32 × 32 list heads, 8 bytes each,
one per 8 × 8-block cell. RVA 0x2f640 takes a global cell (*X*, *Y*), rejects coordinates
outside 0 … 0x1fffff, finds zone (*X* ÷ 32, *Y* ÷ 32) and returns the head at index
32·(*Y* mod 32) + (*X* mod 32). Each list is circular. A node holds a static pointer and
that static's (zone x, zone y, index) triple. The target pass (§2.3), the nearest-static
routine (RVA 0x1a5f60, §6.2) and the static draw use it. Which statics are listed in which
cell is unresolved (§10).

### 4.4 Drawing (RVA 0xb3097 … 0xb3290 → per-static RVA 0xbe760)

Each static is drawn with its kind's model, translated to its position (relative to the
render origin) and rotated about z by rotation index × 90°. The scale factor is uniform:
scale.x (+0x24) ÷ the model's x size.

**Lighting.** The light value comes from the block at the static's position:

* block type (low 5 bits of the block's fourth byte) 13 → 255;
* types 0 and 2 → the block's light byte, raised to at least 5;
* any other type → 0.

That value ÷ 255 multiplies the alpha of the supplied colour. Kinds 50 and 51 always use
1. The draw also fades with distance: the fade factor is 1 within the near radius,
otherwise 1 − (distance − near) ÷ (far − near).

**Open fraction.** With *p* the open fraction of §6.3, the moving parts are:

| Kind | Moving part |
|---|---|
| 1, 2, 3 | the whole model turns about z by *p*·90°, about a hinge offset by half of the model's second size field (model +0x48) |
| 5 gate | raised by 0.8·scale.z·*p* |
| 7 spike trap | moved along z by −(scale.z·*p*) − 1 |
| 8 stomp trap | moved along z by scale.z·*p* |
| 9 lever | tilted by (60·*p* − 30)° |
| 10 chest | the base is drawn, then the lid (kind 11's model) turned by *p*·80° about a displaced horizontal axis |

**Highlight.** The static that is the object target is drawn with an added colour of
(0.1, 0.1, 0.1, 0.1) (RVA 0xb30ab … 0xb3112). The world item that is the item target gets
(0.25, 0.25, 0.25, 0.25) (RVA 0xb2b7c … 0xb2bdc). A focused creature shows its life bar
instead.

### 4.5 State records

Every state change made by §6 is appended to the step's event list (+0x30) as a
**section-6 record** built by RVA 0xdc8b0: the triple followed by a copy of the whole
static record. How records are serialised and sent, and how a receiving client applies
them, is specified in `alpha-net-packets.md` §3.6. What the zone save stores is specified
in `alpha-saves.md` §5.

---

## 5. Pressing R and E

### 5.1 Talk and Ride responses (RVA 0x96994 … 0x96b19)

These run inside the prompt code, only on a frame whose prompt is Talk or Ride, and only
when R is edge-triggered. The quick-item wheel is **not** tested here.

* **Talk** (controller kind 3, look mode):
  1. if a dialogue bubble is open (G+0x800940), its finished-test RVA 0xe6530 is called
     and the result is discarded;
  2. a packet-6 record of **kind 2** is queued. Its target is the NPC's **home triple**
     (creature +0x1b0, 12 bytes). This happens on every such press (see
     `alpha-server-simulation.md` §8 kind 2);
  3. if a bubble is open and RVA 0xe6530 reports it finished, the press closes it (bubble
     +0x188 := 0, bubble hidden, G+0x800940 := 0);
  4. otherwise RVA 0x882e0 runs with the NPC. If a bubble from the pool G+0x80092c is
     already attached to that NPC: when finished it advances to the next page (+0x170
     += 1, +0x16c := 0, +0x174 := 0); otherwise it completes the text (+0x16c := full
     length). With no attached bubble, the last free bubble of the pool is taken (none
     free → nothing). An NPC of class **132** shows the dictionary text `innkeeper`
     (UTF-16 at RVA 0x301dec). Other NPCs get dialogue from RVA 0xe4bd0 (quest
     specification). An NPC with bit 0x80 of byte +0x7e set takes a separate path
     (§10).
* **Ride**: P's action := **106**, action time := 0. No packet.

### 5.2 The R block (RVA 0x9735f …)

This block runs when the quick-item wheel is closed (G+0x800a40 = 0) and R is
edge-triggered.

**Dead.** If P's health ≤ 0 (RVA 0x97383 … 0x973df): P's transient state is reset
(RVA 0x47110), P's position := the point returned by W's routine RVA 0x1a03d0 for P's
current position, and P's health := RVA 0x44db0's result. The rest of the R block is
skipped; the E handling (§5.4) still runs.

**Otherwise, a) creature effects (RVA 0x889e0)**, run on every such press:

* G+0x8008d8 := pointer to the creature-target creature (from W's creature map), or null.
* If there is **no** creature target: RVA 0x6fac0 finds the creature of controller kind 5
  nearest to P, with squared distance < 25 (strictly nearer replaces, so the first wins on
  ties, in map order). If that one creature's +0x198 is all ones, P's +0x198 := its id.
  No packet is sent.
* Otherwise, by the creature's class byte (+0x140):

| Class | Effect |
|---|---|
| 128, 129, 130 | RVA 0xa2300 runs. Windows G+0x8008c4 and G+0x8008bc are shown. G+0x80095c +0x1b4 := 0. The creature's inventory (+0x11dc) is copied into G+0x800c0c (RVA 0x4ad30). Its +0x11e8 is copied to G+0x800c18, its +0x11ec onward to G+0x800c1c, and +0x1304 / +0x1308 to G+0x800d34 / G+0x800d38 |
| 131 | window G+0x8008f8 shown; G+0x8008fc fields +0x160, +0x164 := −1 and +0x168 := 1 |
| 137 | window G+0x800900 shown |

**b) Object effects (RVA 0x973eb … 0x975bb).** These run on the same press after a),
whenever the object target's zone x is ≥ 0 and it resolves, by kind:

| Kind | Effect | Packet 6 |
|---|---|---|
| **45** | G+0x8006e4 := 1 (map open) and G+0x800de4 := 1 (teleport mode); see `alpha-ui-screens.md` §5.2 | none |
| 77 | RVA 0x18ce20 runs on G+0x8008f4; the visibility of G+0x8008dc is toggled; G+0x8008bc is set to G+0x8008dc's new visibility | none |
| 65, 71, 72, 73, 74, 75, 76 | the visibility of G+0x8008c0 is toggled. Kind 65 then selects tab **3**, and kinds 71, 73 and 75 select tab **5** (RVA 0x6eb80 on G+0x800958; table RVA 0x9d508 / 0x9d4fc). Then the recipe list is refreshed (RVA 0xa14c0) | none |
| every other kind | see below | kind 3 |

For every other kind:
1. if a bubble is open, RVA 0xe6530 is called and its result discarded;
2. a packet-6 record of **kind 3** is queued, its target the object triple;
3. if a bubble is open and finished, the press closes it, as for Talk;
4. otherwise RVA 0x88030 runs. If P already has a bubble from the pool, the press advances
   or completes it, whatever the kind. Otherwise, unless the kind is 1, 2, 3, 10, 16, 18,
   19 or 69, a free bubble is attached to P with the text **"There is nothing special."**
   (UTF-16 at RVA 0x301db8; colour (1, 1, 1, 1)).

So "[R] Examine" on a table, vase or lever sends kind 3 **and** shows the bubble.
"[R] Open", "[R] Sit Down" and "[R] Sleep" send kind 3 without it. A beach towel (kind 68)
sends kind 3 and also shows the bubble.

### 5.3 The packet-6 record (RVA 0x59530 and callers)

The client builds a 300-byte record:

* an item (280 bytes, zeroed, with its 16-bit field +0x10 set to 1);
* the target triple at +0x118 (default (−1, −1, −1));
* a word at +0x124 = 0;
* the kind byte at +0x128 (set by the caller);
* a 16-bit field at +0x12a = 1.

The record is appended to P's queue (P+0x130c, RVA 0x86100). Kinds used here: 2 (talk),
3 (use static), 5 (pick up). Transport is specified in `alpha-net-packets.md` §4.1.

### 5.4 E: pick up (RVA 0x975c1 … 0x976e2)

This handling is independent of the quick-item wheel. If E is edge-triggered, the item
target's zone x is ≥ 0 and it resolves (RVA 0x19fb90):

* if P may carry it (RVA 0x3e4a0): a packet-6 record of **kind 5** is queued, its target
  the item triple (`alpha-server-simulation.md` §8 kind 5);
* otherwise: the chat (G+0x800a14, RVA 0x3ab30) gets "You can't carry more of these
  items." (UTF-16 at RVA 0x3010b0) in colour (1, 0.2, 0.2, 1), and sound 49 plays at P's
  position with two parameters both 1 (RVA 0x84350).

---

## 6. Using a static: packet-6 kind 3 (Cube.exe step RVA 0x20c510, case at RVA 0x21094a)

This case handles a kind-3 record for creature *C* (the creature whose queue held it).
Several parts test a world flag at W+0xb4 and are skipped when it is non-zero. These are
marked *(flag clear)*. That the flag means "this client does not own the simulation" is
unresolved (§10).

### 6.1 Steps

1. If *C*'s action is 79: action := 0 and translucency (+0x1190) := 0.
2. Resolve the target triple (zone via RVA 0x34d10, 0 ≤ index < count). None → done.
3. **Kind 45**: only when *C* is the creature at W+0xb8; otherwise done. See §6.2.
4. **Seats and beds**, kinds 16, 18, 19, 68 and 69, when the occupant is 0 and *C*'s action
   is 0, 83 or 84:
   * *(flag clear)*: occupant := *C*'s id. Then, by the table at RVA 0x222c4c / 0x222c40:
     * **seats (16, 18)**: *C*'s position := static position + (0, 0, *C*.size.z ÷ 2 +
       scale.z); *C*'s +0x34 := 0; yaw := (rotation + 2) × 90; action := **83**;
     * **beds (19, 68, 69)**: the same position, +0x34 := 0, yaw := rotation × 90, action
       := **84**.
     A section-6 record of the static is then emitted.
   * In every case of this step, flag clear or not, *C*'s +0x40 := 0. Done.
   * When the occupant is not 0 or *C*'s action does not match, processing continues with
     step 5, where these kinds have no effect.
5. Otherwise, only *(flag clear)*:
   * **Chest with contents** (kind 10, state 2). For each entry of each list in the
     contents vector (+0x48), as long as the entry's count is > 0, one world item is
     dropped (RVA 0x19aeb0) and the count decremented (RVA 0x2f140). Each drop makes
     exactly **three `rand()` draws** in this order (*r* = draw ÷ 32767):
     1. first draw → **y** offset 2 − 4*r*;
     2. second draw → **x** offset 2 − 4*r*;
     3. third draw → angle 360·*r*.
     The item is placed at the chest position + (x offset, y offset, 0) blocks with that
     angle.
   * **Kinds 1, 2, 3, 9, 10**: the state setter (§6.3) is called with new state 1 if the
     state is 0, else 0 (a chest in state 2 becomes 0). A section-6 record of the static
     is emitted.
   * **Kind 9 (lever)**, after the toggle: if its link index (+0x178) is ≥ 0 and below the
     static count of the **same** zone, the setter is applied to that partner with new
     state 1 if the lever's state is now ≠ 0, else 0. A second section-6 record is
     emitted. It is built from the **lever's** triple and static again, not the
     partner's.
   * Every other kind: nothing.

**Town NPCs** (RVA 0xdb130). When the NPC's countdown is 0, the house list is chosen by
world time (W+0x80015c): before 21,600,000 ms (6:00) or after 82,799,999 ms, the list at
house + 0x54; otherwise the list at house + 0x48. One `rand()` draw mod the list length
picks one entry. Only if that static is unoccupied does the NPC adopt it as its goal; the
countdown is then set to 20,000. Once within squared distance < 9 of the goal static, the
NPC queues a kind-3 record on it itself, which step 4 handles. That the two lists are beds
(night) and seats (day) is a belief only.

### 6.2 The runestone link (kind 45)

The nearest-static routine (RVA 0x1a5f60) searches the 3 × 3 cells around *C*'s position,
across all kinds. The squared distance is to each static's position, and only a strictly
nearer static replaces the current best, so the first wins on ties. The result is accepted
when its squared distance is ≤ 16; otherwise the routine returns (0, 0, −1). The step
takes the zone of that result triple and the **used** runestone's link index (+0x178). If
that index is ≥ 0 and below that zone's static count:

* sound 47 (parameter 1.0) plays at *C*'s position;
* *C*'s position := the indexed static's position + (0, 0, 2) blocks.

The client never queues kind 3 for a runestone (R opens the map instead, §5.2). Whether
any generator sets +0x178 is unresolved (§10).

### 6.3 The state setter (RVA 0x1a58e0) and the timer

When the setter is called with a new state:

* if the state already equals the new value, nothing happens;
* otherwise the timer becomes max(1000 − timer, 0), the state becomes the new value, and
  a positional sound event is added at the static's position (two parameters 1, 1):
  * kind 5 → sound **51**;
  * kind 6 → **53**, only when the new state ≠ 0;
  * kind 7 → **52**, only when the new state ≠ 0;
  * any other kind → **54**.

The setter does not itself emit a section-6 record; its callers do.

The static step (loop from RVA 0x20d369) adds the frame time to the timer (+0x34) of every
static in each zone it processes (RVA 0x20d3c3). The draw (§4.4) derives the open
fraction *p*:

* state 0 (open): *p* = min(timer × 0.001, 1);
* state ≠ 0 (closed): *p* = max(1 − timer × 0.001, 0). For kinds 7 and 8 the closed case
  uses 0.01 instead of 0.001.

A change therefore animates over 1 s (0.1 s for closing traps) from wherever the previous
animation stood.

### 6.4 Occupants and traps (static step, RVA 0x20d3b0 …)

**Occupants.** For a static with a non-zero occupant, in each step:

* if the occupant no longer exists in W's creature map, or its +0x12c > 0: *(flag clear)*
  occupant := 0 and a section-6 record is emitted;
* otherwise, for kinds 16 and 18 (table RVA 0x222ba4 / 0x222b98) the occupant is placed
  back on the seat: position as in §6.1, +0x34 := 0, yaw := (rotation + 2) × 90, action
  := 83. For kinds 19, 68 and 69 it is placed on the bed the same way with yaw := rotation
  × 90 and action := 84. This placement is not conditioned on the flag;
* after that placement, the seat is freed *(flag clear)* (occupant := 0, section-6
  record) when any of these holds:
  * the squared distance from the occupant to the static's position is > 16;
  * the occupant's +0x40 vector is non-zero;
  * its action is neither 83 nor 84;
* if the seat is not freed, the occupant is placed again by the table at RVA
  0x222be8 / 0x222bdc, without the action change.

**Traps** (kinds 6, 7, 8), only *(flag clear)* (RVA 0x20d902 …):

* when the timer > 2000 and the state ≠ 0: setter to 0, plus a section-6 record;
* when the timer > 2000 and the state is 0: if any creature of controller kind 0 is within
  squared distance < 900 (30 blocks) of the trap position, setter to 1, plus a section-6
  record;
* while the state ≠ 0, a damage pass runs:
  * kinds 6 and 7: when ⌊(world time + frame time) ÷ 100⌋ differs from ⌊world time ÷
    100⌋ (world time W+0x8000bc);
  * kind 8: when its timer is 0.
  The damage pass affects creatures within squared distance < 25 of the trap whose
  controller kind is not 1. The damage itself is unresolved (§10).

---

## 7. Portals end to end

1. **Worldgen** (RVA 0x1e9bc9 … 0x1e9e15). In a zone whose site entry has kind 4
   (`alpha-worldgen-locations.md` §7; the site's other structures are in
   `alpha-worldgen-structures.md` §4), one static is placed:
   * kind **45**;
   * x, y = (256·zx + 128, 256·zy + 128) blocks + 3.5 blocks each (fixed-point offset
     229,376; centre computed at RVA 0x1e49d5 … 0x1e4a01);
   * z settled by stepping one block at a time: down while the block there has type 0 or
     2, then up while it does not;
   * rotation index = one `rand()` draw mod 4, made after settling;
   * scale **(4, 4, 5)**.
2. **Targeting**. With P within squared distance 16 of the stone's base position, either
   aiming at its 4 × 4 × 5 box or being nearest with nothing aimed at makes it the object
   target. The prompt reads **"[R] Teleport"** and the stone is drawn highlighted.
3. **R** sets the map-open and teleport-mode flags (§5.2). Choosing and entering a portal
   on the map is specified in `alpha-ui-screens.md` §5.2. No packet-6 record is queued for
   the press.

---

## 8. Worked examples

1. **Runestone of zone (32800, 32801).** cx = 256·32800 + 128 = 8,396,928; cy =
   256·32801 + 128 = 8,397,184. The stone stands at x = **8,396,931.5** and y =
   **8,397,187.5** blocks, with z the settled height. Its target box spans x
   8,396,929.5 … 8,396,933.5 and y 8,397,185.5 … 8,397,189.5. Its z range runs from the
   base to base + 5; rotation does not matter because the footprint is square. The zone
   centre lies √(3.5² + 3.5²) ≈ **4.95** blocks from the stone, so standing at the centre
   is outside the 4-block fall-back radius.
2. **Which target wins.** P stands 3 blocks from a door (kind 1, state 1) and 3.5 blocks
   from an NPC behind it. The crosshair covers both boxes. Squared camera distances: door
   30.25, NPC 36. The creature pass sets *B* = 36 and creature target = NPC. The static
   pass finds 30.25 < 36, so object target := door and creature target := 0. Prompt:
   "[R] Open". Pressing R queues kind 3 for the door. The step sets state 0 (a timer of
   25,000 becomes 0) and adds sound 54. The door turns to 90° over 1 s, and the prompt
   then reads "[R] Close".
3. **Out of reach.** A table 4.5 blocks from P (squared 20.25 > 16) is never an object
   target, even under the crosshair. No prompt is shown.
4. **Nothing aimed at.** The crosshair is on the sky. A stool is 3 blocks away (squared 9)
   and an NPC 3.5 blocks away (squared 12.25). The creature pass sets the NPC as
   fall-back; the static pass finds 9 < 12.25 and replaces it with the stool. Result:
   object target = stool ("[R] Sit Down"), creature target = 0. If an item P may pick up
   also lies 1.5 blocks away (squared 2.25 ≤ 16), its fall-back clears the stool and NPC
   fall-backs. No prompt is shown, and E picks the item up.
5. **Sitting.** A stool (kind 16) is at z = 100 blocks, with scale z 0.5 and rotation 1.
   P's size z is 1.8. R → kind 3 → P at z = 100 + 0.9 + 0.5 = **101.4**, yaw (1 + 2)·90 =
   **270**, action 83. The prompt over the stool disappears (occupant = P). The step frees
   the stool once P's +0x40 vector becomes non-zero.
6. **Chest.** A chest in state 2 holds entries totalling 3 units. R drops 3 world items,
   making 9 `rand()` draws in the order (y, x, angle) × 3, each within ±2 blocks in x and
   y of the chest. The state becomes 0, sound 54 plays, the lid opens to 80° in 1 s, and
   the prompt reads "[R] Close". A second R closes it (state 1); every entry's count is
   then 0.

---

## 9. (Removed)

The former implementation-notes section was removed in the re-certification below.

---

## 10. Unresolved

* **U-1** Which statics each cell list (zone + 0xac) holds, and the builder of that index.
  Not traced.
* **U-2** Whether any generator writes the link index +0x178 (levers, runestones). Not
  traced. If none does, kind-3 runestone travel and lever links are inert.
* **U-3** The meaning of the world flag W+0xb4 (believed: "not the simulation owner") and
  of the creature at W+0xb8 (believed: local player). Also whether the static step
  (RVA 0x20d369 …) runs on every client.
* **U-4** The server's kind-3 handler. The previously cited Server.exe address (VA
  0x5489e0, RVA 0x1489e0) does not decode as code in Server.exe. The server path was not
  located or compared.
* **U-5** Trap damage amount and kind; the meaning of creature +0x12c (frees seats).
* **U-6** The identities of the windows opened for classes 131 and 137, the name of
  crafting tab 5, and the meaning of the vendor fields copied for classes 128–130.
* **U-7** The separate dialogue path for NPCs with bit 0x80 of byte +0x7e set (it adds map
  markers through G+0x800d44); the contents of quest dialogue (RVA 0xe4bd0).
* **U-8** The meaning of record words +0x38 and +0x17c … +0x184, and whether statics are
  created by anything other than world generation.
* **U-9** Point lights and fire particles of street lights and campfires, the exact chest-
  lid pivot, and the item-box cases for categories 3 … 13. Not re-verified here.
* **U-10** What the zone save stores for statics, and the wire content of section-6
  records (companion specs; not re-verified here).
* **U-11** The direction of the candidate sort in §2.2 (believed ascending) and its order
  for equal keys.

---

## Re-certification (2026-11-15)

The source author checked this document against Cube.exe (image bytes via dumpbin, the
Ghidra listing and the lightc retry of the frame update). Server.exe was checked only for
U-4. The Infinite-Cubes reconstruction was not consulted. All addresses were converted
from VAs to RVAs (image base 0x400000).

**Confirmed.**
* Field offsets, reset values and lookup routines of the three targets.
* Creature-pass skip rule, LOS ranges and darkness formula, box sizes and corner layout,
  focus byte; static-pass cell loop, centre, x/y swap and LOS 200; item-pass pick-up rule
  (category 25), carry limits (50; category 20 limited to 1), penalty 16, squared item
  distances; fall-back logic; box-test projection rule.
* Every prompt string, its kind table and the occupant suppression.
* Ride conditions and action 106; class handling 131/137; the crafting-window kinds and
  tabs 3 and 5; the kind-77 window toggling; the "nothing special" exclusion list
  (1, 2, 3, 10, 16, 18, 19, 69).
* The E path, chat colour and sound 49; packet-6 record layout and kinds 2, 3, 5.
* Static constructor values; the kind → model table (all slots and file names).
* Open-fraction formulas and per-kind moving parts (gate 0.8, spike, stomp, lever 60/30,
  chest lid 80°, hinge door 90°); highlight colours; street-light full light.
* Setter timer rule and sounds 51/52/53/54; seat/bed placement, yaw and actions 83/84;
  chest state 2 → 0; runestone link sound 47 and +2 offset; nearest-static ≤ 16 rule.
* Runestone kind, 3.5 offset, scale (4, 4, 5) and zone centre; spike-trap timer phase
  rand mod 4000; NPC furniture time limits and squared-distance 9.

**Corrected** (old → new):
1. **Reversal; affects gameplay.** Player and camera distances in the creature and static
   passes: linear, 16-block fall-back/reach → squared distances (RVA 0x3b4a0), so the
   reach and fall-back radius is 4 blocks. The examples in §8 and the text of §7 were
   adjusted to match.
2. **Reversal.** Open point I-2 ("items compare squared against linear *B*") → all passes
   compare squared distances; withdrawn.
3. **Random-draw order.** Chest drop draws: per item x, y, angle → per **unit** (each count
   of each entry), first y, then x, then angle.
4. **Random-draw order; affects gameplay.** NPC furniture: a random free entry → one draw
   picks an entry, used only if unoccupied; lists +0x48 by day and +0x54 by night.
5. **Affects gameplay.** The lever's second section-6 record: the partner's record → the
   lever's own triple and static again.
6. **Affects gameplay.** Talk press: the kind-2 record was queued only when no bubble was
   closed → queued on every Talk press, before the close-or-advance check.
7. **Affects gameplay.** Talk and Ride responses: required the quick-item wheel closed →
   they do not test the wheel.
8. Examine path order: close the bubble, then queue kind 3 → queue kind 3, then close the
   bubble if finished, otherwise show or advance the text. With a bubble already on P, the
   kind exclusion does not apply.
9. Prompt: "when a creature is found, the object branch is not tried" → the object branch
   is skipped only when Talk or Ride is shown.
10. **Affects gameplay.** Dead state: "the only prompt is Revive" → the item pass and the
    fall-backs can restore targets after the dead clearing, so a later prompt can replace
    "[R] Revive".
11. The bubble creature "keeps the NPC targeted" → the later passes can replace it.
12. The creature candidate list was in creature-list order → it is sorted on squared
    distance (non-stable).
13. The creature-pass fall-back "also clears the fall-back object" → removed (the
    creature fall-back sets only the creature id). The creature fall-back is evaluated
    after LOS and before the box result.
14. Free-pet claim: the nearest unowned pet → the nearest pet within squared distance
    < 25, claimed only if its owner field is all ones.
15. The vendor copy: "gold" and "item" labels and the targets → the actual source and
    destination offsets, labels removed.
16. Cell index: lists at zone + 0xac + 8·i → zone + 0xac holds a pointer to the head
    array.
17. Record layout: contents at +0x48 and the item record built separately → a single
    0x130-byte compound from RVA 0x3c020 at +0x48 (item at +0x58, zeroed words at
    +0x170 and +0x174).
18. **Affects gameplay.** Occupant step: occupants are freed by distance, movement or
    action → each step first places the occupant back and sets action 83/84, so the
    action test cannot fail for seat and bed kinds. The +0x40 test is the effective
    release.
19. Seat effects: only the occupant write was marked owner-only → all placement effects
    are skipped when W+0xb4 is set; only +0x40 := 0 always happens.
20. Trap trigger range: unknown "900 test" → squared distance < 900 (30 blocks). The
    damage range test is squared < 25 for creatures whose controller kind is not 1.
21. Timer: "added to every active static by the owner" → added to every static of each
    processed zone, with no flag test.
22. Draw scale: matched to the record scale per axis → one uniform factor, scale.x ÷ the
    model's x size. Lighting: added the 0 case for other block types; "air/water" → block
    types 0 and 2.
23. Darkness factor: unnamed "day factor" → (1 − (2*t* ÷ 86,400,000 − 1)⁴)⁵.
24. Runestone rotation: "d & 3" → one rand() draw mod 4, made after the settling loop;
    settling rule stated.
25. Section-6 record contents: "zone x, y, index and the static's words" → the triple
    followed by a full copy of the static record (RVA 0xdc8b0). Worked example 1 no
    longer claims a compass direction or a physics effect.

**Removed.**
* §9 "Implementation notes for AlphaVulkan" (all four bullets).
* §4.3 advice on substitute spatial indexes; the "Status" paragraph.
* Unverified claims:
  * the save byte layout and inferred refill;
  * section-6 networking behaviour and "online clients never change statics";
  * "statics are created only by world generation";
  * point-light and particle kinds;
  * the map label and click details and the teleport destination;
  * the rock-pillar ring;
  * "physics pushes P up";
  * the Server.exe case citation (now U-4).

**Unresolved.** See §10, U-1 … U-11.
