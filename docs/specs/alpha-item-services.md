Status: FINAL / STABLE / HELD

# Alpha item services: Identification, Adaption and the Weapon Customization bench

Behavioural specification of the two NPC item-service windows of Cube World **Alpha**
(the identifier's **Identification** window and the **Adaption** window) and of the
**customization bench** (static kind 77), whose window is titled **"Weapon
Customization"** and edits the cubes attached to a weapon. Every statement below was
re-read against the original `Cube.exe` (and, where marked, `Server.exe`) image bytes and
their decompile. **All addresses are RVAs in `Cube.exe` (image base 0x400000) unless the
image is named as `Server.exe` (also base 0x400000).** *Believed* marks readings whose
meaning (not their mechanics) is not fully proven.

Companion pointers (not repeated, not re-verified here): `alpha-crafting.md` §5–§6,
`alpha-talk-vendors.md` §3.6 and §4, `alpha-interaction.md` §5.3, `alpha-ui-screens.md`
§8.1, `alpha-creatures-behaviour.md` §10 and §12, `alpha-world-items.md` §2.5,
`alpha-items-generator.md` §2, `alpha-loot-rolls.md` §3.5–§3.6, `alpha-item-names.md`,
`alpha-net-packets.md`, `alpha-saves.md`.

---

## 0. Conventions

* "G" is the game object, "P" the local player's creature (pointer at G+0x8006d0).
  W, H = the client width and height (G+0x11c, G+0x120).
* An **item** is the 280-byte record: type u8 +0x00, subtype u8 +0x01, u32 +0x04
  ("seed"), u32 +0x08 ("product"), rarity u8 +0x0c, material u8 +0x0d, flags u8 +0x0e
  (bit 0 = "adapted"), level/quantity i16 +0x10, 32 attached-cube entries of 8 bytes from
  +0x14, attached-cube count i32 +0x114 (layout as copied by RVA 0xee70). A **bag slot** is
  284 bytes: i32 count, then the item. P's bags: a list of 12-byte bag records between
  P+0x11dc and P+0x11e0, each holding a slot array. P's money (copper) i32 P+0x1304,
  platinum i32 P+0x1308, level i32 P+0x190.
* **Attached-cube entry** (8 bytes): x, y, z (three signed bytes, a voxel cell of the
  weapon model's grid), material (u8), level (i32).
* g(*L*) = 1 − 1/((*L* − 1)·0.05 + 1), single precision (0x3ca60).
  *F*(*v*, *R*) = 2^(3·g(*v*)) · 2^(0.25·*R*): each power is evaluated in double from a
  single-precision exponent, rounded to single, and the product is single (0x45f60).
  **Power** p(item) = trunc(g(level)·100 + 1), single precision (0xc76a0; level only).
  The bench's inverse is lvl(*p*) = trunc((1/(1 − 0.01·*p*) − 1)·20 + 1) (0x462f0 applied to
  0.01·*p*, result stored as i16).
* **Price** of an item (0xc76e0): *k* = 2 for types 3 and 4; 1 for 5, 6, 9; 1.5 for 7, 8;
  100 for 23; 0.2 for every other type. A type-3 item of subtype 5, 6, 7, 8, 10, 11, 15,
  16, 17 or 18 doubles *k*. Price = trunc(*F*(level, rarity)·10·*k*) in single precision,
  raised to 1 when that is below 1.
* "Outlined text" = an outline pass (radius as stated; first colour opaque white, second
  colour opaque black — identical for every label in these windows) followed by a fill
  pass (radius 0) in the stated colour; font `resource1.dat`; sizes in pixels. Positions
  are **window pixels relative to the window's top-left**, x right, y down; alignment
  0 = left, 1 = centred, 2 = right. *w*, *h* = the window's own width and height.
* A **draw** is one call of the MSVC CRT `rand()` (the shared stream).
* Sound identifiers are the numbers passed to the client sound routine (0x84350): 0x2e
  and 0x31 are used below.

---

## 1. Identification (NPC class 131)

### 1.1 Objects and placement

* Host node G+0x8008f8, **350 × 285** (created in 0x59c40, size set at 0x64b7a); widget
  (class "EnchantWidget"; constructor 0x4e910, draw 0x4ea30, layout method 0x50b90) at
  G+0x8008fc. The widget clones the `itemframe` template as a child (widget +0x170).
* Widget fields: **+0x160 bag index**, **+0x164 slot index**, **+0x168 mode**, +0x16c G.
  Constructor: −1, −1, mode 1.
* Placement (layout routine 0x82a40, block at 0x83bf1): top-left = (W/2 + *wᵢ*/2 + 10,
  H/2 − *hᵢ*), with *wᵢ*, *hᵢ* = the size of host G+0x8008bc (the inventory host, created
  400 × 285 at 0x652a4), i.e. **(W/2 + 210, H/2 − 285)**: right of the inventory, tops
  aligned.
* Layout method: the `itemframe` clone is placed at ((*w* − frame width)/2, **50**), i.e.
  centred horizontally, 50 px below the window top (pivot = the frame's top-left,
  *believed*; the frame's size comes from the `gui.plx` template, see unresolved).

### 1.2 Opening, closing

* **Open** (0x889e0; class byte at creature +0x140 = 0x83, branch at 0x88a61): host
  visible; source := (−1, −1); mode := 1.
* While the Identification **or** Adaption host is visible, every frame the inventory host
  is made visible; if the inventory host is then not visible, the vendor host G+0x8008c4,
  the Identification host and the Adaption host are hidden (0x964e7 in 0x88ee0).
* **Goodbye**: on **any** mouse-button release (0x7ddd0, block 0x7de35, no button test)
  while the host is visible and the mouse is in the Goodbye zone (§1.4): host hidden,
  source := (−1, −1); mode unchanged.
* When P's movement vector (P+0x40, +0x44, +0x48) has a non-zero squared length, the hosts
  G+0x8008bc, G+0x8008c0, G+0x8008c4, G+0x8008dc, G+0x8008f8, G+0x800900 and G+0x800910 are
  hidden (0xa8e14 in 0xa6b50).

### 1.3 Choosing the source

* A right or middle press (buttons 1 or 2) on a **bag slot**, when the Adaption host is
  **not** visible and the Identification host **is** visible (press handler 0x7b600,
  right/middle branch): source := (bag index, slot index) under the mouse. No type test,
  nothing is taken from the slot, no sound. If both hosts are visible the Adaption window
  takes the press (§2.2).
* Equipment boxes never feed this window: their right/middle press goes, in priority order,
  to Adaption, the vendor host G+0x8008c4, the bench, else unequip.
* **Valid source** (0x50960): the bag list is non-empty, the bag and slot indices are in
  range, the slot count ≠ 0 and the item's **type is 14**. Anything else is "no source":
  nothing is drawn for it and the Identify label is grey.
* **Source tooltip** (0x7b010): while the host is visible and the source indices are in
  range with slot count ≠ 0 (any type), a mouse inside the window's **top-left 120 × 120 px**
  (left/top inclusive, right/bottom exclusive) makes that item the tooltip source.

### 1.4 Drawing (0x4ea30), in order

Let *v* = the valid source item (or none).

1. If *v* exists and has a model (model lookup 0xec400): the item model (0x758c0) with the
   turn vector at G+0x800a1c, scale **0.06**, last argument 0, centred on the `itemframe`
   clone's centre — window x = *w*/2 = **175**, y = 50 + frame height/2. Around this draw a
   render state (index 7) is enabled and then disabled, the light arrays are zeroed and the
   colour is (1, 1, 1, 1) (*believed*: world shader with point lights cleared).
2. If *v* exists: its **text block** (0xa28c0) at (**15, 180**), scale 1, wrap 300, name-line
   flag 1, the two further flags 0.
3. Title **"Identification"** at (**15, 25**), **12 px**, outline 3, white, left-aligned.
4. **"Identify"** at (*w*/3, *h* − 20) = (**116.7, 265**), 12 px, outline 3, centred. Fill:
   (0.7, 0.7, 0.7, 1) without a valid source; else **(0, 1, 1, 1)** when the mouse is in
   the Identify zone, white otherwise.
5. **"Goodbye!"** at (2*w*/3, *h* − 20) = (**233.3, 265**), 12 px, outline 3, centred;
   (0, 1, 1, 1) when the mouse is in the Goodbye zone, white otherwise.
6. Only if *v* exists: the **fee** row at *y* = trunc(*h* − 50) = **235**, every part drawn
   even when 0, 10 px, outline 2, right-aligned except "COST:":

   | Text | x | Fill |
   |---|---|---|
   | (fee mod 100) + " C " | right edge **220** | (0.8, 0.5, 0, 1) |
   | (⌊fee/100⌋ mod 100) + " S " | right edge **180** | (0.7, 0.7, 0.7, 1) |
   | ⌊fee/10 000⌋ + " G " | right edge **140** | (1, 0.9, 0, 1) |
   | "COST:" (left-aligned) | **15** | (1, 1, 1, 1) |

**Zones** (mouse in window coordinates, strict inequalities; 0x50ab0 / 0x50a00):
Identify zone = 0 < x < *w*/2 and *h* − 30 < y < *h*, and only with a valid source;
Goodbye zone = *w*/2 < x < *w* and *h* − 30 < y < *h*. For 350 × 285: Identify (0, 255) …
(175, 285); Goodbye (175, 255) … (350, 285).

### 1.5 The fee (0x50920)

fee = 0 with no valid source; otherwise **max(1, ⌊price(*v*)/2⌋)** copper, with the price
of §0 (*k* = 0.2 for type 14): fee = max(1, ⌊max(1, trunc(2·*F*(*L*, *R*)))/2⌋) for the
Leftovers' level *L* and rarity *R*.

### 1.6 Identify (left press, 0x7b600, block 0x7c8ea … 0x7cb06)

Runs on a **left press** (button 0) only, when all hold, tested in this order: host visible;
mouse in the Identify zone (which requires a valid source); fee ≤ money; **mode = 1**;
source indices in range and slot count ≠ 0; source type = 14 (0x50b70). Then:

1. money (P+0x1304) −= fee.
2. One item is taken from the slot (0x2f140: count − 1; when the count falls below 1 it
   becomes 0 and the type and subtype bytes become 0; the other bytes stay).
3. **One draw** *d*. Level *L* := the slot item's i16 +0x10 and rarity *R* := its byte
   +0x0c, read after step 2.
4. *d* even → **weapon generator** (0x1f8ad0) with (*L*, *R*, class −1): 16 draws, 11
   candidates. *d* odd → **armour generator** (0x1f51e0) with (*L*, *R*, class −1): 24
   draws, 21 candidates. Both are called on the object at G+0x2e4. A whole Identify
   therefore spends **17 or 25 draws**. Every candidate is built with level *L*, rarity *R*,
   flags 0 and no attached cubes; the last draw of the generator picks the result.
5. The result is copied and added to P's bags by the bag-add routine (0x6ebe0; its result is
   not tested here); the inventory grid's scroll state is refreshed (0xc6350, 0xc64c0).
6. Sound **0x2e** at P's position (P+0x10), volume 1, pitch 1.
7. Chat announcement (0x80fb0) for the local player: "You receive " in white, then the
   item name + " +" + p(result) in the item's name colour, then "." in white.

The source stays selected. A slot emptied by step 2 is no longer a valid source, so the
window shows nothing until a new source is chosen.

### 1.7 Mode 0 (unreachable branch)

The same handler has a mode-0 branch (from 0x7cb06): with the same fee and index tests and
type 14, if the source item's material ≠ 0 it takes one item, sets the source to (−1, −1)
when the slot count is still non-zero, makes 5 + (one draw mod 6) records of type 10 with
the source's material and pushes each to the incoming-item queue (0x4d460), sets
G+0x1000e70 := 0 and plays sound 0x2e; then, in every case, the slot count is decreased by
one more (same emptying rule). The fee is tested but never charged. No code path writes 0
to the mode field (the constructor and the opener write 1; no other write to it was found),
so the branch never runs.

---

## 2. Adaption (NPC class 137)

### 2.1 Objects and placement

* Host G+0x800900, **350 × 400** (size set at 0x64cd7); widget (class "AdaptionWidget";
  constructor 0xecd0, draw 0xf8f0, layout 0x11410) at G+0x800904. Children: an `itemframe`
  clone (+0x170) and a `rightarrow` clone (+0x174).
* Fields: **+0x160 item pointer**, **+0x164 bag index**, **+0x168 slot index**, +0x16c G.
  Constructor: null, −1, −1.
* Placement (0x82a40, block 0x83c87): like Identification, **(W/2 + 210, H/2 − 285)**; being
  115 px taller it reaches H/2 + 115.
* Layout: `itemframe` at ((*w* − frame width)/2, 50); `rightarrow` at (*w*/2 − 10, 230) =
  **(165, 230)** (pivot top-left, *believed*).

### 2.2 Opening, closing, source

* **Open** (0x889e0, class byte 0x89, block 0x88b54): host visible. The source is **not**
  reset on opening.
* **Goodbye** (any button release in the Goodbye zone; 0x7ddd0 block 0x7de8f, zone test
  0x50a00 on this widget, so with 350 × 400): host hidden; pointer := null, bag and slot
  := −1. Closing by the inventory rule or by movement does **not** clear the source.
* **Bag slot** right or middle press while the host is visible: bag, slot := those under the
  mouse; pointer := null. Takes priority over Identification, vendor and bench.
* **Equipment box** right or middle press on a non-empty box while the host is visible: bag,
  slot := −1; **pointer := that equipped item** (edited in place). Takes priority over the
  vendor, the bench and unequip.
* **Valid source** (0xf570): the pointer if set, else the bag slot (list non-empty, indices in
  range, count ≠ 0); the item must be of **type 3–9** and have **flags bit 0 clear**.
  Otherwise "no source".

### 2.3 Drawing (0xf8f0), in order

Let *s* = the valid source, *L*ₚ = P's level (P+0x190).

1. If *s* is none: `rightarrow` hidden. Otherwise:
   * model of *s* at the `itemframe` centre, scale 0.06 (as §1.4 step 1);
   * text block of *s* at (**15, 180**), scale 1, wrap 300, name-line flag 1, further flags 0;
   * a **preview copy** of *s* (whole record, attached cubes included, 0xee70) with level :=
     the low 16 bits of *L*ₚ and nothing else changed (flags bit 0 stays clear, so no
     " (adapted)"): its text block at (trunc(*w*/2 + 50), 180) = (**225, 180**), scale 1,
     wrap 300, **name-line flag 0** and further flags 0 — the name line is not drawn but its
     line height is still skipped, so the block starts with the "Power …" line one line
     lower;
   * `rightarrow` visible.
2. Title **"Adaption"** at (15, 25), 12 px, outline 3, white.
3. **"Adapt"** at (*w*/3, *h* − 20) = (**116.7, 380**), 12 px, outline 3, centred; fill
   (0.7, 0.7, 0.7, 1) without a valid source, (0, 1, 1, 1) when the mouse is in the Adapt
   zone (0x11340: the Identify-zone rule with this window's size), else white.
4. **"Goodbye!"** at (2*w*/3, *h* − 20) = (**233.3, 380**), colours as in §1.4.
5. **Always** (also with no source), *y* = trunc(*h* − 50) = **350**:
   * cost + **" Platinum Coins "**, right edge x = **220**, 10 px, outline **2.5**, fill
     **(0.5, 0.2, 1, 1)** — "0 Platinum Coins " when there is no source or nothing to pay;
   * "COST:" at x = 15, 10 px, outline 2, white, left-aligned.

### 2.4 The cost (0xf4f0; the draw repeats the same loop)

With *l* = the source's level (i16), *r* = its rarity (unsigned byte) and *n* = *L*ₚ − *l*:

* *n* ≤ 0 → cost **0**.
* Otherwise the integer *l* + *r* − 1 is converted to single and multiplied by 0.25,
  *q* = 2^that (double power, rounded to single), *t* = 2·*q* in single precision; the cost
  starts at 0 and *n* times becomes trunc(cost + *t*) (single-precision sum). This equals
  **cost = *n*·⌊*t*⌋ platinum coins** except when rounding the single-precision sum carries
  it to the next integer (only possible for very large running costs); the iterated rule is
  authoritative.

Worked: level 5, rarity 1, player level 8 gives *t* = 4.7568 → 4, 8, **12** platinum.

| *l* + *r* | ⌊*t*⌋ per level | *l* + *r* | ⌊*t*⌋ per level |
|---:|---:|---:|---:|
| 1 | 2 | 9 | 8 |
| 2 | 2 | 10 | 9 |
| 3 | 2 | 11 | 11 |
| 4 | 3 | 12 | 13 |
| 5 | 4 | 13 | 16 |
| 6 | 4 | 14 | 19 |
| 7 | 5 | 17 | 32 |
| 8 | 6 | 21 | 64 |

### 2.5 Adapt (left press, 0x7b600, block 0x7c748 … 0x7c806)

On a **left press** when, in this order: host visible; mouse in the Adapt zone (0x11340,
which requires a valid source); a valid source exists; **cost ≤ platinum (P+0x1308)**:

1. platinum −= cost;
2. the source's level := the low 16 bits of *L*ₚ, **in place** (bag slot or equipped item);
3. source flags |= 1 (the tooltip's power line gains " (adapted)", 0xa28c0);
4. sound **0x2e** at P, volume 1, pitch 1.

No chat line, no draw, no inventory refresh. The source is now invalid (adapted), so the
window empties and shows "0 Platinum Coins ". Consequences, as read:

* An item **above** P's level (*n* < 0) is adapted for **0** platinum and its level is
  **lowered** to *L*ₚ.
* An item at P's level is adapted for 0 and only gains the flag.
* An item that already has flags bit 0 can never be adapted (again).
* Attached cubes are kept; stats follow the new level (see §3.8 for the level-driven stat
  routines).

---

## 3. Weapon Customization (static 77, the customization bench)

### 3.1 Objects and placement

* Host **G+0x8008dc**, **400 × 625** (size set at 0x65182); widget (class "VoxelWidget";
  constructor 0x187f70, draw 0x188500) at **G+0x8008f4**.
* Placement (0x82a40, block 0x83d1d): left of the inventory host, tops aligned:
  (W/2 − 200 − 400 − 10, H/2 − 285) = **(W/2 − 610, H/2 − 285)**; at 1280 × 720:
  (30, 75) … (430, 700). Earlier in the same routine (0x8362f) the host's position is first
  written as (W′/2 − 340, H′/2 − 250) minus its pivot, with W′, H′ read from G+0x800710
  (+0x10c, +0x110); the later block overwrites it in the same call (*believed*: both set the
  same position).
* VoxelWidget fields:

| Off | Meaning | Initial |
|---|---|---|
| +0x160 | **edited item**: pointer into a bag slot or an equipment slot | null |
| +0x164, +0x168, +0x16c | view angles *a*, *b*, *c* in degrees | **−90, 0, 90** |
| +0x170 … +0x178 | **palette**: list of 284-byte slots (count + item), rebuilt every draw | empty |
| +0x17c | **selected palette entry** | −1 |
| +0x180 | **cursor item** (280 bytes; shown by the cursor label) | empty, level 1 |
| +0x298 | G | — |
| +0x29c, +0x2a0, +0x2a4 | **target cell** (x, y, z), i32 each | 0 |
| +0x2a8 | target valid (the ray hit something) | 0 |
| +0x2ac | **hovered palette entry** | −1 |
| +0x2b0 | copy of the hovered palette item (tooltip source) | empty, level 1 |
| +0x3c8 | **holding** an attached cube | 0 |
| +0x3cc … +0x3d4 | held cube's original cell | — |
| +0x3d8, +0x3dc | held cube's material, level | —, 0 |

  The constructor also builds a 1 × 1 × 1 voxel model whose single voxel is white
  (0xff, 0xff, 0xff).
* Reset (0x18ce20): selected entry := −1, holding := 0.

### 3.2 Opening, closing, choosing the weapon

* Interacting with a **static of kind 77** (0x97437 in 0x88ee0): reset; toggle the bench host's
  visibility; the inventory host's visibility := the bench host's.
* Closing: the same interaction again; a right press over no interface widget (hides hosts
  G+0x8008bc, G+0x8008c0 and the bench host; start of 0x7b600); the movement rule of §1.2.
* **Choosing the weapon** (while the bench host is visible): a right or middle press on a
  **bag slot** whose item is **type 3**, when neither Adaption, Identification nor the vendor
  host G+0x8008c4 is visible → edited item := pointer to that slot's item, then reset; a
  right or middle press on a **non-empty equipment box** holding a type-3 item, when neither
  Adaption nor the vendor host is visible → the same with the equipped weapon. Any other type → sound **0x31** and nothing else. The
  item is edited **in place**; no copy is made.
* A **right press** (button 1): when the byte flag G+0x800704 is 0, and the bench host is
  visible, the bench is reset (drops the hold, clears the selection). When the flag is
  non-zero, the byte P+0x18c is incremented instead, whether or not the bench is visible
  (*believed* a debug switch; no write enabling it was traced).
* **Rotating** (0x7edee in 0x7ea00): while the byte flag G+0x0a is set (*believed* "middle
  button held") and the bench host is visible, each frame *c* += 0.5·Δx and *a* −= 0.5·Δy,
  Δ = mouse position minus the previous position (G+0x800710 +0xd4/+0xd8 against
  +0xdc/+0xe0). There are no limits; *b* never changes.

### 3.3 Which items accept cubes

Only **weapons (type 3)** can be chosen. Capacity (0xc7660): **32** for subtypes 5, 6, 7, 8,
10, 11, 15, 16, 17, 18; **16** for every other subtype. The record holds 32 entries for every
item.

### 3.4 The palette (rebuilt at the start of every draw, 0x188500)

At the start of every draw the cursor item is reset to empty (level 1) and the palette list is
emptied. With an edited weapon *e* (material *m*ₑ, power *p*ₑ = p(*e*)) it is then filled in
this order:

1. **Own-material cubes.** The total count *N* of bag stacks equal (0x2f4a0, §3.8) to the item
   (type 11, subtype 10, material *m*ₑ, level 1, everything else 0, no attached cubes). If
   *N* ≠ 0: entry (count *N*, that item).
2. **Spirits.** For *p* from max(1, *p*ₑ − 10) up to *p*ₑ, and inside each *p* for material
   128, 129, 130, 131: the total count *N* of bag stacks with type 11, subtype **14**, that
   material and **p(item) = *p*** (other fields ignored). If *N* ≠ 0: entry (count *N*, item type 11, subtype 14, that material, level
   **lvl(*p*)**, everything else 0).

So only spirits whose power is **at most** the weapon's power and **at least** 10 below it are
offered. After the cursor step below, a selection index ≥ the entry count is clamped to
count − 1 (−1 when the palette is empty).

Then, if the hovered entry (as left by the previous draw's grid pass) is −1: when holding, the
cursor item becomes type 11, subtype 14 if the held material is ≥ 128 (else subtype 10), with
the held material and level; else, with a valid selection, the cursor item becomes a copy of
the selected palette item.

**Spirit source**: the zone generator gives every spawn record whose word at +0x7a has bit
0x200 set (the boss flag, per the creature specs) one item — type 11, subtype 14, material
128 + (one draw mod 4), **rarity 2**, level := the record's i16 +0x34 — added to the spawned
creature's inventory at +0xf6c (`Cube.exe` 0x1ed35c … 0x1ed3d5 in 0x1e4850; `Server.exe`
0x12113e … 0x1211b5 in 0x118630). Tooltips show such an item's "Power" (not a quantity),
since subtype 14 is excluded from the quantity rule of the text block.

### 3.5 Picking the target cell (each draw, 0x188500)

The weapon model (0xec400) is drawn in a 3D view (§3.7). From the mouse position a ray is cast
through that view into the model's voxel grid and walked **cell by cell** (3D DDA) while the
travelled ray parameter is below **1000** (grid units):

* cells outside the model's size count as empty;
* in each cell: a **solid model voxel** sets "target valid"; independently, each attached cube
  of the weapon whose cell equals the current cell — skipping the held cube's original cell
  while holding — sets "target valid" and makes the **cursor item** type 11, subtype 14, with
  that cube's material and level and rarity 2 (whatever the cube's material, and also while
  holding or with a selection);
* the walk stops in the first cell that set "target valid"; that cell is the **hit cell**,
  stored as the target; the axis and direction of the last step are remembered as a
  one-cell back-step;
* if nothing is hit, "target valid" := 0.

**Target cell**: the hit cell itself, except when the hovered palette entry (previous grid
pass) is −1 **and** the bench is holding or has a valid selection: then, with a hit, the target
becomes the cell in front of the hit face (hit cell plus the back-step). Without a hit, the
ghost (below) floats 100 units along the ray and the target is not valid, so a left press
takes the "target not valid" row of §3.6.

**Cursor label** (0x8b503 in 0x88ee0): while the bench host is visible and the cursor item's
type ≠ 0, the label G+0x8008a8 shows the cursor item's name, followed by " +" + its power for
types that show a power (spirits do; own-material cubes do not).

### 3.6 Clicking (left press, 0x7b600 at 0x7bcee → 0x188250 / 0x18ce40)

On every left press while the bench host is visible and a weapon is set (wherever the mouse
is):

* **A palette entry is hovered** → toggle the selection: selected := hovered, or −1 if it was
  already selected (0x18ce40). No sound.
* **Otherwise** (0x188250), by state, first matching row wins:

| State | Effect |
|---|---|
| target not valid | if holding **and** the mouse is over no interface widget, the held cube is **removed** from the weapon (0xc79b0) and lost; in all cases the hold and the selection are reset |
| target valid, selection valid (0 ≤ index < palette count), attached count < **32** | **place**: put (target cell, entry material, entry level) on the weapon (0xc69b0); then remove **one** item (0x2f140) from the **first** bag stack (bag order, slot order) with count ≠ 0 whose type, subtype and material equal the entry's and whose power equals the entry's power. A hold, if any, is left unchanged |
| target valid, selection valid, attached count ≥ 32 | nothing |
| target valid, no valid selection, holding | **move**: remove the cube at its original cell (0xc79b0), then place a cube with the held material and level at the target cell (0xc69b0); hold := 0 |
| target valid, no valid selection, not holding | if an attached cube sits at the target cell (first match): **pick it up** (holding := 1, remember its cell, material and level); the cube stays on the weapon until moved or destroyed |

**Place rule** (0xc69b0): if an entry already exists at that cell (x, y, z equal) its material
and level are **replaced**; otherwise, if the count is below the capacity (§3.3), the entry is
appended and the count rises by 1; else nothing is added. The "place" row's palette-item
consumption happens in every case — including when a 16-capacity weapon is full and nothing is
added.
**Remove rule** (0xc79b0): the first entry with that cell is deleted, the later entries move
down one place and the count falls by 1.

There are **no costs** other than the consumed cube item, no level or class requirement and no
sound for place, move, pick up, drop or palette selection.

### 3.7 Drawing (0x188500), in order

1. Cursor reset, palette and cursor item (§3.4), selection clamp.
2. If a weapon is set and it has a model, the **3D view**: render state 7 enabled, light arrays
   zeroed, colour (1, 1, 1, 1), then:
   * model matrix: three successive axis rotations by *a*, *b*, *c* (degrees × 0.01745329;
     *believed* about X, Y, Z respectively), uniform scale **0.25 / max(sx, sy, sz)** (the
     model's integer sizes), the model offset by −½ of its size so that its middle is the
     origin, translated **1 unit** along the view axis;
   * perspective: vertical field of view 2 × 0.392699 rad (**45°**), x scale negated
     (aspect −W/H), near 0.1, far 1000 (depth terms 1.0001 and −0.10001), followed by a
     screen shift that puts the model origin at the window point (**w/2, 0.6·h**) =
     **(200, 375)**;
   * the ray of §3.5 is cast through this view.
3. **Ghost cube** (only when the hovered entry is −1 and something is held or selected): the
   cube model at the target cell (or 100 units along the ray without a hit), coloured by the
   material colour routine (0xc7250, input (1, 1, 1, 1), strength 0) of the held material
   when holding, else of the selected entry's material.
4. **Attached cubes**: each entry as one unit cube at (x, y, z) in model cells, coloured by its
   material (0xc7250, input (1, 1, 1, 1), strength 0). The held cube is not drawn at its
   original cell. An entry whose cell equals the target cell while the target is valid is
   drawn **1.1 ×** larger about its centre with input colour (*f*, *f*, *f*, *f*),
   *f* = 1 + 0.2·cos(0.01·*t*), *t* = the integer at +0xe8 of the interface object at widget
   +0x04 (*believed* a millisecond clock). For spirit materials the input RGB is ignored
   (§3.9), so only their alpha pulses.
5. **Palette grid** (if a weapon is set): the hovered entry is first reset to −1; entry *i* has
   column *c* = *i* mod 8 and row *r* = ⌊*i*/8⌋, box origin (20 + 45*c*, 40 + 45*r*) (window
   coordinates truncated to screen pixels), hit box 40 × 40 (left/top inclusive, right/bottom
   exclusive). The entry under the mouse becomes "hovered" (+0x2ac) and its item is copied to
   +0x2b0 (tooltip source, 0x7b3e0). Model at (40 + 45*c*, 60 + 45*r*), turn vector
   G+0x800a1c, scale **0.02**, **0.03** when hovered or selected, colour (1, 1, 1, 1).
6. "Upgrades " + attached count + "/" + capacity at (**20**, *h* − 20 = **605**), 10 px,
   outline 2, white, left-aligned (if a weapon is set).
7. Title **"Weapon Customization"** at (15, 25), 12 px, outline 3, white — drawn even with no
   weapon (then steps 2–6 are skipped).
8. Palette counts: entry *i*'s count right-aligned at (60 + 45*c*, 80 + 45*r*), 10 px,
   outline 2, fill white, **(0, 1, 1, 1)** for the selected entry.

### 3.8 How cubes change the weapon

* **Stat level**: five item-stat routines (0xc6a90, 0xc70b0, 0xc78c0, 0xc7af0, 0xc7f60) use
  level + **0.1 × attached count** — every attached cube, of any material, counts. 32 cubes =
  +3.2 levels in those routines. Which stats these are is in `alpha-creatures-behaviour.md`
  §10 (not re-traced here).
* **Combat effects** of spirit-material cubes: `alpha-creatures-behaviour.md` §12 (not
  re-traced here).
* **Power** p(item) reads only the level; the **price** reads only type, subtype, level and
  rarity — both ignore attached cubes.
* **Equality** (0x2f4a0) compares type, subtype, the u32 at +0x04, flags, the u32 at +0x08,
  rarity, level, material and the attached count, then, for each of the first *count* entries,
  the material and the three cell bytes — **not** the entry level. A customized weapon is
  therefore never equal to a plain one.

### 3.9 How attached cubes are drawn elsewhere

The attached-cube routine (0x71b60), called from the item-model routine 0x758c0 and from
0x4ac260 (six call sites), draws each entry with the model's matrix translated by (x, y, z)
model cells, coloured by 0xc7250; it sets shine 1 for the whole pass when the item's material
is 1, 11, 12 or 22 (0xc7be0), else 0. Spirit colours (0xc7250, strength *s*,
*f* = 1 + 0.5·*s*):

| Material | RGB | At *s* = 0 |
|---|---|---|
| 128 | (*f*, 0.5*f*, 0.1*f*) | (1, 0.5, 0.1) |
| 129 | (0.3*f*, *f*, 0.5*f*) | (0.3, 1, 0.5) |
| 130 | (0.3*f*, 0.5*f*, *f*) | (0.3, 0.5, 1) |
| 131 | (0.8*f*, 0.8*f*, *f*) | (0.8, 0.8, 1) |

Alpha for spirits = (input alpha + *s*)·*f*; the input RGB is ignored. Other materials use the
input colour times that material's factors (e.g. material 1: × 0.7 grey), alpha = input alpha.

---

## 4. State, network and draws

* Identify, Adapt and every bench edit change P's own bags, equipment, money or platinum
  directly in the client; no network message is built in these handlers (*believed*: the
  called routines were not exhaustively traced for sends).
* Window state (sources, selection, angles, hold) lives only in the widgets.
* The draws of Identify (17 or 25) and of mode 0 come from the shared CRT `rand()` stream, as
  does the boss-spirit material draw in the zone generator.

---

## 5. Worked examples

### 5.1 Identifying Leftovers

Leftovers: type 14, level 12, rarity 2, in bag 0, slot 4. P has 150 copper and opens the
identifier (class 131), then right-clicks the Leftovers.

1. Source := (0, 4); valid (type 14, count 1).
2. Price: *F*(12, 2) = 2^(3·0.354839)·2^0.5 = 2.09146·1.41421 = 2.95778; price =
   trunc(2.95778·10·0.2) = trunc(5.9156) = 5; **fee = max(1, 2) = 2** copper.
3. Drawn (1280 × 720, window at (850, 75)): model at window (175, 50 + frame height/2); the
   text block at (15, 180) → screen (865, 255); "0 G " right edge 140, "0 S " 180, "2 C " 220
   at y 235 → screen y 310; "COST:" at (15, 235).
4. Mouse at window (100, 270): inside the Identify zone (0 < 100 < 175, 255 < 270 < 285) →
   "Identify" cyan. Left press: money 150 → **148**; slot count 1 → 0 (type cleared); one
   draw — if even, weapon generator (12, 2, −1) with 16 more draws; the weapon (rarity 2,
   level 12) is added to the bags; sound 0x2e; chat "You receive " + name + " +36" + "."
   (p = trunc(g(12)·100 + 1) = trunc(36.48) = 36). The window is now empty.

Other fees: (level 5, rarity 1) → price 3, fee **1**; (20, 3) → price 9, fee **4**; (30, 4) →
price 13, fee **6**.

### 5.2 Adapting a sword

P is level 8 with 20 platinum. A sword (type 3) of level 5, rarity 1, flags 0 is right-clicked
in the bags while the Adaption window is open.

1. Valid (type 3, not adapted). *n* = 3, *q* = 2^1.25 = 2.37841, *t* = 4.75683 → cost 4, 8,
   **12**. Drawn: "12 Platinum Coins " right edge 220 at y 350; source block at (15, 180) with
   its name line and "Power 17"; preview block at (225, 180) without a name line, starting
   with "Power 26" (p(8) = trunc(25.93 + 1)) one line lower; arrow visible.
2. Left press on "Adapt" (window (100, 390)): platinum 20 → **8**; level 5 → **8**; flags
   |= 1; sound 0x2e. Its tooltip now reads "Power 26 (adapted)". The window shows
   "0 Platinum Coins ".

Further costs: (*l* 3, *r* 4, *L*ₚ 20): ⌊2·2^1.5⌋ = 5 per level, 17 levels → **85**;
(1, 0, 10): 2 per level, 9 levels → **18**; (15, 0, *L*ₚ 10): **0**, and the item drops to
level 10.

### 5.3 Customizing a weapon of subtype 5

P owns a type-3, subtype-5 weapon of material 1, level 10, rarity 0, no cubes: *p*ₑ =
trunc(g(10)·100 + 1) = 32, capacity 32. Bags: 5 cubes (11, 10, material 1, level 1, all
other fields 0); 3 spirits (11, 14, material 128) of level 12 (p 36); 2 spirits (11, 14,
material 130) of level 8 (p 26).

1. Interact with the bench: the bench and the inventory open. Right-click the weapon in the
   bags: edited item := that slot, reset.
2. Palette: entry 0 = the material-1 cube, count 5. Spirits for *p* = 22 … 32: material 130 at
   *p* = 26 → entry 1 = (11, 14, 130), level lvl(26) = trunc((1/0.74 − 1)·20 + 1) = 8, count
   2. The material-128 spirits (p 36 > 32) are **not offered**. Grid: entry 0 box (20, 40),
   entry 1 box (65, 40); "Upgrades 0/32" at (20, 605).
3. Left-click entry 1 → selected (its count "2" turns cyan). With the mouse over the blade,
   suppose the ray enters solid cell (3, 0, 14) through its +x face (last step from x = 4 to
   x = 3) → target = the cell the ray came from, **(4, 0, 14)**; a ghost cube coloured
   (0.3, 0.5, 1) is drawn there.
4. Left-click: entry (4, 0, 14, material 130, level 8) appended, count 1; one item is removed
   from the first matching spirit stack (2 → 1). "Upgrades 1/32".
5. Stat level in the routines of §3.8: 10 → 10.1 (e.g. *F*(10, 0) = 1.90665 → *F*(10.1, 0) =
   1.91604).
6. Right-click (clears the selection), then left-click the new cube: picked up (hold). A
   left-click on another face of the blade moves it; a left-click where the ray misses the
   weapon while the mouse is over no interface widget destroys it (count 0; nothing returns
   to the bags).

---

## 6. Unresolved

* **U-1** The `itemframe` and `rightarrow` template sizes and pivots (`gui.plx`) were not
  decoded, so the model centre's y (50 + frame height/2) and the arrow's exact extent are
  open.
* **U-2** The mesh bound for attached cubes and the ghost cube (drawn through 0xe6df0) is
  *believed* to be the 1 × 1 × 1 white voxel model built by the VoxelWidget constructor; the
  binding was not traced.
* **U-3** The meaning of the byte flag G+0x800704 (right press increments P+0x18c instead of
  resetting the bench) — *believed* a debug switch; no enabling write was traced.
* **U-4** The 3D view's exact composition order (rotation axes, post-projection shift vs. view
  translation) is read only in outline; the projected size of the weapon depends on it.
* **U-5** Meaning of the byte flag G+0x0a used for rotation (*believed* middle button held),
  and of the time value at +0xe8 of the interface object (*believed* milliseconds).
* **U-6** Whether the earlier placement write of G+0x8008dc (0x8362f) and the later one
  (0x83d1d) set the same position property (*believed* yes, the later one wins).
* **U-7** Which stats the five level + 0.1·count routines produce, whether speed and crit
  exclude cubes, and the spirit combat effects — not re-traced (companion
  `alpha-creatures-behaviour.md`).
* **U-8** That a dying creature drops its whole inventory (hence one spirit per boss), that
  crafted gear carries flags bit 0, item naming ("Fire/Unholy/Ice/Wind Spirit", "Cube"), the
  effect of the bag-add routine when bags are full, and the save/network carriage of
  equipment, money and platinum — companion claims not re-verified here.
* **U-9** Absence of any network send inside every routine called by these handlers was not
  exhaustively traced.

---

## Re-certification (2026-11-15)

**Confirmed against `Cube.exe`/`Server.exe` bytes and decompile:** item and slot layout, bag
list, money/platinum/level offsets; g, *F*, power, lvl and price formulas and constants
(0.05, 3, 0.25, 100, 20, 0.01, 10, *k* table and subtype list); host sizes 350 × 285,
350 × 400, 400 × 625 and inventory 400 × 285; placements of all three hosts; widget fields and
initial values; opening by class 131/137 and static 77; Goodbye on any release; forced
inventory; movement close; Identification source rules, valid-source test, 120 × 120 tooltip
area; every draw position, size, outline and colour of both NPC windows; zones; fee; Identify
order, one parity draw then generator (16/11 weapon, 24/21 armour; totals 17/25); sound 0x2e;
mode-0 branch and its unreachability; Adaption source rules, cost loop and Adapt effects
(including free lowering of over-level items); palette construction and window of 10 power;
capacity 16/32; DDA cap 1000; target/back-step rule; place/move/pick-up/destroy effects;
place and remove rules; view constants (scale 0.25, distance 1, 45°, 0.1/1000, (w/2, 0.6·h));
grid geometry, scales 0.02/0.03; pulse 1 + 0.2·cos(0.01·t), 1.1 ×; Upgrades text; spirit
colours and alpha rule; shine materials; boss spirit (client and server); worked-example
arithmetic.

**Corrected (old → new):**
1. Cost table *l* + *r* = 9: 7 → **8** per level (2·2^2 = 8). Affects gameplay numbers.
2. Adaption preview text block: "name on" → **name-line flag 0** (no name line; "Power" line
   one line lower). Visual.
3. Item equality: "every field, spirits included" → attached entries compared by material and
   cell only, **not level**, over the first *count* entries.
4. Bench click priority: "holding → move" listed before placing → a **valid selection takes
   precedence over a hold** (the hold survives a placement). Gameplay behaviour.
5. Bench place limit: row condition clarified as the hard **32** test, separate from the
   capacity test of the place rule (unchanged result, now stated precisely).
6. Cursor item on hitting an attached cube: "only if nothing is held" → **whenever** the ray
   hits a non-held attached cube, always subtype 14 and rarity 2; cursor also set from the
   held cube or the selected entry; cursor reset every draw. (Former open point S-6 resolved:
   the label shows whenever the cursor item is non-empty.)
7. DDA limit: "up to 1000 cells" → **ray parameter below 1000**.
8. "Without a hit a click does nothing" → a press takes the **target-not-valid** row (resets
   hold and selection; destroys a held cube over no widget).
9. Highlight pulse: spirit cubes pulse **only in alpha** (input RGB ignored).
10. Equipment-box selection for Adaption and the bench: right press → **right or middle**
    press.
11. Right press with G+0x800704 set: increments P+0x18c **regardless of bench visibility**
    (and as a byte).
12. Former open point S-4 reversed: the second placement (W′/2 − 340, H′/2 − 250) **is in
    0x82a40** (0x8362f), overwritten later in the same call. **Reversal.**
13. Adapt block location: "just before the Identify block" → 0x7c748 … 0x7c806 (another
    window's block lies between it and 0x7c8ea).
14. Chat announcement: whole line in name colour → "You receive " and "." in white, name and
    power in the name colour.
15. Exactness note of the cost formula: "exact below 2²⁴" → exact unless single-precision
    rounding of the running sum carries to the next integer.
16. Addresses restated as image RVAs (previously virtual addresses); bench-open citation
    0x4973eb → RVA 0x97437.

No random-draw order was changed.

**Removed:** "Status." paragraph and authoring-history wording; the "Not to be implemented"
remark; the whole "Notes for AlphaVulkan" section (N-1 … N-5: parity switches, quirk
toggles, handle-storage advice, tunable tables, extensibility); the "Corrections to other
specs" section; recording/screenshot verification suggestions in open points; sound file-name
mapping and sound 0x55; weapon-subtype names; narrative "boar at tier 0"; combat-effect figures
of example 5.3 (damage factor 4, 0.0065 damage, 500 ms slow); network/save field numbers.
Native facts from removed text were kept as facts (mode-0 unreachability, raw-pointer source
persistence as "closing does not clear the source").

**Unresolved:** U-1 … U-9 above.
