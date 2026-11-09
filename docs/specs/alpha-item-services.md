# Alpha item services: Identification, Adaption and the Weapon Customization bench

Behavioural specification of the two NPC item-service windows of Cube World **Alpha**
`Cube.exe` (the identifier's **Identification** window and the **Adaption** window) and of
the **customization bench** (static kind 77), whose window is titled **"Weapon
Customization"** and edits the cubes ("spirits") attached to a weapon. Written from the
user's Ghidra decompile of the client and from the assembly and data of `Cube.exe`
(constants and strings were read from the binary). No code was copied; addresses are
citations only.

Read first (not repeated here): `alpha-crafting.md` §6 (outline of both NPC windows),
`alpha-talk-vendors.md` §3.6 and §4 (classes 131 and 137, the forced-inventory rule),
`alpha-interaction.md` §5.3 (static 77), `alpha-ui-screens.md` §8.1 (bags, the add rule
0x46ebe0, money and platinum fields, the item-model routine 0x4758c0, the tooltip text block
0x4a28c0, mouse actions §8.1.8), `alpha-creatures-behaviour.md` §10 and §12 (stats and spirit
effects), `alpha-world-items.md` §2.5 (attached sub-cubes), `alpha-items-generator.md` §2
(weapon and armour generators), `alpha-loot-rolls.md` §3.5–§3.6 (Leftovers, NPC inventory
drops).

**Status.** Every handler named in the task was read to the instruction: both draw routines,
both cost routines, the Identify/Adapt/Goodbye handlers, the source-assignment rules, the
VoxelWidget (bench) constructor, draw, palette builder, ray pick, click, place, remove and
rotate rules. *Believed* marks readings that are not fully proven. Section 7 lists
corrections to other specs that this reading forces.

---

## 0. Conventions

* "G" is the game object, "P" the local player's creature (G+0x8006d0).
* An **item** is the 280-byte record: type +0x00, subtype +0x01, seed +0x04, product +0x08,
  rarity +0x0c, material +0x0d, flags +0x0e (bit 0 = "adapted"), level/quantity i16 +0x10,
  32 spirit entries of 8 bytes from +0x14, spirit count i32 +0x114. A **bag slot** is 284
  bytes: u32 count, then the item. P's bags: list at +0x11dc … +0x11e0 (12 bytes per bag);
  P's money (copper) +0x1304, **platinum** +0x1308 (`alpha-ui-screens.md` §8.1.1).
* **Spirit entry** (8 bytes): x, y, z (three signed bytes, voxel cell in the weapon model's
  grid), material (u8), level (i32).
* g(*L*) = 1 − 1/((*L* − 1)·0.05 + 1) (0x43ca60); *F*(*v*, *R*) = 2^(3·g(*v*))·2^(0.25·*R*)
  (0x445f60). **Power** p(item) = trunc(g(level)·100 + 1) (0x4c76a0, single precision).
  Its inverse, used by the bench, is lvl(*p*) = trunc((1/(1 − 0.01·*p*) − 1)·20 + 1)
  (0x4462f0).
* **Price** of an item (0x4c76e0): max(1, trunc(*F*(level, rarity)·10·*k*)), *k* = 2 for
  types 3 and 4, 1 for 5, 6, 9, 1.5 for 7, 8, 100 for 23, 0.2 for every other type
  (Leftovers included); a weapon of subtype 5–8, 10, 11, 15–18 doubles *k*.
* "Outlined text" = an outline pass (black stroke of the stated radius) followed by a fill
  pass in the stated colour, `resource1.dat`, sizes in pixels (`alpha-ui-screens.md` §8.1).
  Positions below are **window pixels relative to the window's top-left**, x right, y down;
  for text, (x, y) is the baseline origin; alignment 0 = left, 1 = centred, 2 = right.
* *w*, *h* = the window's width and height; W, H = the client area.
* A **draw** is one MSVC `rand()` of the shared CRT stream.
* Sounds: 0x2e (`disenchant2.wav`), 0x31 (refusal voice), 0x55, 0x58 (`menu-drop-item.wav`).

---

## 1. Identification (class 131, the identifier)

### 1.1 Objects and placement

* Host node G+0x8008f8, **350 × 285**, widget **EnchantWidget** (constructor 0x44e910, draw
  0x44ea30, layout method 0x450b90) at G+0x8008fc. The widget clones the `itemframe`
  template as a child (widget +0x170).
* Widget fields: **+0x160 bag (tab) index**, **+0x164 slot index**, **+0x168 mode**
  (constructor: −1, −1, mode 1).
* Placement (layout routine 0x482a40, every resize): top-left = (W/2 + *wᵢ*/2 + 10,
  H/2 − *hᵢ*) with *wᵢ*, *hᵢ* = the inventory window's size (400 × 285), i.e.
  **(W/2 + 210, H/2 − 285)**: right of the inventory, tops aligned.
* Layout method: the `itemframe` clone is put at ((*w* − frame width)/2, **50**), i.e.
  centred horizontally, 50 px below the window top (pivot = the frame's top-left,
  *believed*; the frame's own size comes from the `gui.plx` template, open point S-1).

### 1.2 Opening, closing

* **Open**: R on a class-131 NPC (0x4889e0): host visible; source := (−1, −1); mode := 1.
  Every frame while it is visible the inventory is forced visible
  (`alpha-talk-vendors.md` §3.6); closing the inventory hides it.
* **Goodbye**: on **any** mouse-button release (0x47ddd0) while the host is visible and the
  mouse is in the Goodbye zone (§1.4): host hidden, source := (−1, −1) (mode kept).
* Any non-zero movement intent closes it with the other inventory-type windows (0x4a6b50,
  `alpha-player-control.md` §3).

### 1.3 Choosing the source

* Right (1) or middle (2) press on a **bag slot** while the Adaption host is **not** visible
  and the Identification host **is** visible (0x47b600, right/middle-press branch): source := (current
  tab, slot). No type test here; nothing is taken from the slot; no sound. (If both windows
  were open, Adaption wins.)
* Equipment boxes never feed this window (their right press goes to the vendor, Adaption,
  bench or unequip rules of `alpha-ui-screens.md` §8.1.8).
* **Valid source** (0x450960): the tab and slot indices exist, the slot count ≠ 0 and the
  item's **type is 14** ("Leftovers", `alpha-loot-rolls.md` §3.5). Anything else is treated
  as "no source": nothing is drawn for it and the Identify label is grey.
* **Source tooltip**: while the window is visible and the source slot exists with count ≠ 0
  (any type), hovering the window's **top-left 120 × 120 px** shows the standard tooltip of
  that item (0x47b010; window source of `alpha-ui-screens.md` §8.1.7).

### 1.4 Drawing (0x44ea30), in order

Let *v* = the valid source item (or none).

1. If *v* and it has a model: the item model (0x4758c0, with the turn vector G+0x800a1c,
   scale **0.06**, depth 0) centred on the `itemframe` clone's centre, i.e. at
   ((*w* − *f*w)/2 + *f*w/2, 50 + *f*h/2) = (**175**, 50 + *f*h/2) for frame size
   (*f*w, *f*h). World shader, point lights cleared, material (1, 1, 1, 1).
2. If *v*: its **text block** (0x4a28c0) at (**15, 180**), scale 1, wrap 300, name line on,
   class line off — name, "Power …", stats, as `alpha-ui-screens.md` §8.1.7.
3. Title **"Identification"** at (**15, 25**), **12 px**, outline 3, white, left-aligned.
4. **"Identify"** at (*w*/3, *h* − 20) = (**116.7, 265**), 12 px, outline 3, centred. Fill:
   (0.7, 0.7, 0.7, 1) when there is no valid source; else **(0, 1, 1, 1)** when the mouse is
   in the Identify zone, white otherwise.
5. **"Goodbye!"** at (2*w*/3, *h* − 20) = (**233.3, 265**), 12 px, outline 3, centred;
   (0, 1, 1, 1) when the mouse is in the Goodbye zone, white otherwise.
6. Only if *v*: the **fee** row at baseline *y* = trunc(*h* − 50) = **235**, every part drawn
   even when 0, 10 px, outline 2 (outline pass white), right-aligned:

   | Text | Right edge x | Fill |
   |---|---|---|
   | (fee mod 100) + " C " | **220** | (0.8, 0.5, 0, 1) |
   | (⌊fee/100⌋ mod 100) + " S " | **180** | (0.7, 0.7, 0.7, 1) |
   | ⌊fee/10 000⌋ + " G " | **140** | (1, 0.9, 0, 1) |
   | "COST:" (left-aligned) | x = **15** | (1, 1, 1, 1) |

**Zones** (mouse in window coordinates, open intervals, 0x450ab0 / 0x450a00):
Identify zone = 0 < x < *w*/2 and *h* − 30 < y < *h* (only counted with a valid source);
Goodbye zone = *w*/2 < x < *w* and *h* − 30 < y < *h*. For 350 × 285: Identify (0, 255) …
(175, 285); Goodbye (175, 255) … (350, 285).

### 1.5 The fee (0x450920)

fee = 0 with no valid source; otherwise **max(1, ⌊price(*v*)/2⌋)** copper, with the price of
§0 (*k* = 0.2 for type 14): fee = max(1, ⌊max(1, trunc(2·*F*(*L*, *R*)))/2⌋) for the
Leftovers' level *L* and rarity *R*.

### 1.6 Identify (left press, 0x47b600 at 0x47c8ea … 0x47cb06)

Runs on a **left press** (button 0) only, and only when all hold, tested in this order:
host visible; mouse in the Identify zone with a valid source; money ≥ fee; **mode = 1**;
source indices exist and slot count ≠ 0; source type = 14 (0x450b70). Then:

1. money (P+0x1304) −= fee.
2. One item is taken from the slot (0x42f140: count − 1; at 0 the count becomes 0 and the
   type and subtype bytes become 0 — level and rarity bytes stay readable).
3. **One draw** *d*. Level *L* := the slot item's i16 +0x10 and rarity *R* := its byte
   +0x0c (read after step 2, unchanged by it).
4. *d* even → **weapon generator** 0x5f8ad0(*L*, *R*, class −1): 16 draws, 11 candidates.
   *d* odd → **armour generator** 0x5f51e0(*L*, *R*, class −1): 24 draws, 21 candidates.
   (Generator object = the world object G+0x2e4; `alpha-items-generator.md` §2.) So a
   whole Identify spends **17 or 25 draws**. The result keeps *L* and *R* unchanged (no
   rarity roll, no level re-roll), flags 0, no spirits.
5. The result is copied and **added to P's bags** with the add rule (0x46ebe0, bag chosen by
   the item, never refused), the inventory grid's scroll range is refreshed (0x4c6350,
   0x4c64c0).
6. Sound **0x2e** at P's position, volume 1, pitch 1.
7. Chat announcement (0x480fb0): "You receive " + name + " +" + p(result) + "." in the name
   colour (`alpha-ui-screens.md` §8.1.9).

The source stays selected; as Leftovers do not stack (type 14 is not a stackable type) the
slot is normally empty after one Identify and the window shows nothing until a new source
is chosen.

### 1.7 Mode 0 (*disenchant*, unreachable)

The same handler has a mode-0 branch (0x47cb06 …): with the same tests (including type 14),
if the item's material ≠ 0 it takes one item, clears the source when the slot is still
non-empty, makes 5 + (draw mod 6) records of type 10 with the source's material and pushes
each to the incoming-item queue (0x44d460), clears G+0x1000e70 and plays 0x2e; then, in
every case, it takes **one more** item. The fee is checked but never charged. Nothing in the
client sets mode 0 (the constructor and the opener both write 1), and Leftovers have
material 0, so this branch never runs. Not to be implemented.

---

## 2. Adaption (class 137)

### 2.1 Objects and placement

* Host G+0x800900, **350 × 400**, widget **AdaptionWidget** (constructor 0x40ecd0, draw
  0x40f8f0, layout 0x411410) at G+0x800904. Children: an `itemframe` clone (+0x170) and a
  `rightarrow` clone (+0x174).
* Fields: **+0x160 equipment-item pointer**, **+0x164 tab**, **+0x168 slot** (constructor:
  null, −1, −1).
* Placement: like Identification, **(W/2 + 210, H/2 − 285)** (0x482a40); it is 115 px
  taller, so it reaches H/2 + 115.
* Layout: `itemframe` at ((*w* − *f*w)/2, 50); `rightarrow` at (*w*/2 − 10, 230) =
  **(165, 230)** (pivot top-left, *believed*).

### 2.2 Opening, closing, source

* **Open**: R on a class-137 NPC: host visible. The source is **not** reset on opening.
* **Goodbye** (any button release in the Goodbye zone): host hidden; pointer := null, tab
  and slot := −1. Closing through the inventory or movement does **not** clear the source
  (a stale pointer is kept, see N-3).
* **Bag slot** right/middle press while the host is visible: tab, slot := (current tab,
  slot); pointer := null. Takes priority over Identification, vendor and bench.
* **Equipment box** right press (any non-empty box) while visible: tab, slot := −1;
  **pointer := that equipped item** (edited in place).
* **Valid source** (0x40f570): the pointer if set, else the bag slot (indices exist, count
  ≠ 0); then the item must be of **type 3–9** and have **flags bit 0 clear** (not adapted).
  Otherwise "no source".

### 2.3 Drawing (0x40f8f0), in order

Let *s* = the valid source, *L*ₚ = P's level (P+0x190).

1. If *s* is none: `rightarrow` hidden. Otherwise:
   * model of *s* at the `itemframe` centre, scale 0.06, depth 0 (as §1.4 step 1);
   * text block of *s* at (**15, 180**), scale 1, wrap 300, name on, class off;
   * a **preview copy** of *s* (whole record, spirits included, 0x40ee70) with level :=
     *L*ₚ (as i16) and nothing else changed (flags bit 0 stays clear, so no "(adapted)"):
     its text block at (trunc(*w*/2 + 50), 180) = (**225, 180**), scale 1, **wrap 300**,
     name on, class off (the block runs past the window's right edge for long lines, as
     read);
   * `rightarrow` visible.
2. Title **"Adaption"** at (15, 25), 12 px, outline 3, white.
3. **"Adapt"** at (*w*/3, *h* − 20) = (**116.7, 380**), 12 px, outline 3, centred; grey
   (0.7, 0.7, 0.7) without a valid source, (0, 1, 1) when hovered (zone of §1.4 with the
   Adaption window's size; 0x411340), else white.
4. **"Goodbye!"** at (2*w*/3, *h* − 20) = (**233.3, 380**), as in §1.4.
5. **Always** (also with no source): baseline *y* = trunc(*h* − 50) = **350**:
   * cost + **" Platinum Coins "**, right edge x = **220**, 10 px, outline **2.5** (outline
     pass white), fill **(0.5, 0.2, 1, 1)** — "0 Platinum Coins " when there is no source
     or nothing to pay;
   * "COST:" at x = 15, 10 px, outline 2, white, left-aligned.

### 2.4 The cost (0x40f4f0; the draw repeats the same loop)

With *l* = the source's level, *r* = its rarity and *n* = *L*ₚ − *l*:

* *n* ≤ 0 → cost **0**.
* Otherwise *q* = 2^((*l* + *r* − 1)·0.25) (exponent computed in single precision, the power
  in double, rounded to single), *t* = 2·*q* in single precision, and the cost starts at 0
  and *n* times becomes trunc(cost + *t*). Because the running cost is a whole number,
  this equals **cost = *n*·⌊2·2^((*l* + *r* − 1)/4)⌋** platinum coins (exact for any cost
  below 2²⁴).

This confirms the formula of `alpha-crafting.md` §6.2 but **corrects its example**: level 5,
rarity 1, player level 8 gives *t* = 4.7568 → 4, 8, **12** platinum (not 13).

| *l* + *r* | ⌊2*q*⌋ per level | *l* + *r* | ⌊2*q*⌋ per level |
|---:|---:|---:|---:|
| 1 | 2 | 9 | 7 |
| 2 | 2 | 10 | 9 |
| 3 | 2 | 11 | 11 |
| 4 | 3 | 12 | 13 |
| 5 | 4 | 13 | 16 |
| 6 | 4 | 14 | 19 |
| 7 | 5 | 17 | 32 |
| 8 | 6 | 21 | 64 |

### 2.5 Adapt (left press, 0x47b600, the block just before the Identify block at 0x47c8ea)

On a **left press** when: host visible; mouse in the Adapt zone with a valid source (0x411340);
a valid source exists; **platinum (P+0x1308) ≥ cost**:

1. platinum −= cost;
2. the source's level := *L*ₚ (i16), **in place** (bag slot or equipped item);
3. source flags |= 1 (adapted; tooltip line 2 gains " (adapted)");
4. sound **0x2e** at P, volume 1, pitch 1.

No chat line, no draw, no inventory refresh. The source is now invalid (adapted), so the
window empties and shows "0 Platinum Coins ". Consequences, as read:

* An item **above** P's level (*n* < 0) is adapted for **0** platinum and its level is
  **lowered** to *L*ₚ.
* An item at P's level is "adapted" for 0 and only gains the flag.
* Crafted gear already has bit 0 (`alpha-crafting.md` §5) and can never be adapted; an
  adapted item can never be adapted again.
* The stats follow the new level at once (`alpha-creatures-behaviour.md` §10); spirits are
  kept.

---

## 3. Weapon Customization (static 77, the customization bench)

### 3.1 Objects and placement

* Host **G+0x8008dc**, **400 × 625** (made in 0x459c40), widget **VoxelWidget**
  (constructor 0x587f70, draw 0x588500) at **G+0x8008f4**.
* Placement (0x482a40): left of the inventory, tops aligned: (W/2 − 200 − 400 − 10,
  H/2 − 285) = **(W/2 − 610, H/2 − 285)**; at 1280 × 720: (30, 75) … (430, 700).
* VoxelWidget fields:

| Off | Meaning | Initial |
|---|---|---|
| +0x160 | **edited item**: pointer into a bag slot or an equipment slot | null |
| +0x164, +0x168, +0x16c | view angles *a*, *b*, *c* in degrees | **−90, 0, 90** |
| +0x170 … +0x178 | **palette**: list of 284-byte slots (count + item), rebuilt every frame | empty |
| +0x17c | **selected palette entry** | −1 |
| +0x180 | **cursor item** (280 bytes; shown by the cursor label) | empty, level 1 |
| +0x29c, +0x2a0, +0x2a4 | **target cell** (x, y, z) | — |
| +0x2a8 | target valid (the ray hit something) | 0 |
| +0x2ac | **hovered palette entry** | −1 |
| +0x2b0 | copy of the hovered palette item (tooltip source) | empty |
| +0x3c8 | **holding** an attached cube | 0 |
| +0x3cc … +0x3d4 | held cube's original cell | — |
| +0x3d8, +0x3dc | held cube's material, level | — |

  The constructor also builds a 1 × 1 × 1 white voxel model (believed the cube mesh used for
  attached cubes).
* Reset (0x58ce20): selected entry := −1, holding := 0.

### 3.2 Opening, closing, choosing the weapon

* **R on a static of kind 77** (0x4973eb …): reset; toggle the host's visibility; the
  inventory host's visibility := the bench host's. So the bench opens and closes the
  inventory with it.
* Closing: R on the bench again; a right press over no widget (closes inventory, crafting
  and this host, `alpha-ui-screens.md` §8.1.2); any movement intent (0x4a6b50).
* **Choosing the weapon** (only while the host is visible, and no vendor/Adaption/
  Identification window has priority): right or middle press on a **bag slot** whose item
  is **type 3** → edited item := pointer to that slot's item, then reset; right press on a
  **non-empty equipment box** holding a type-3 item → the same with the equipped weapon.
  Any other type → sound **0x31** and nothing else. The item is edited **in place**; no copy
  is made.
* A **right press** (button 1) anywhere while the host is visible also resets (drops the
  hold without effect, clears the palette selection), unless the flag G+0x800704 is set
  (then P+0x18c is incremented instead; *believed* a debug switch, never set).
* **Rotating**: while the **middle button is held** (controller flag +0x0a) and the host is
  visible, every frame *c* += 0.5·Δx and *a* −= 0.5·Δy, Δ = mouse movement in pixels since
  the last frame (0x47ea00). There are no limits; *b* never changes.

### 3.3 Which items accept cubes

Only **weapons (type 3)** can be chosen. Capacity (0x4c7660): **32** for subtypes 5, 6, 7,
8, 10, 11, 15, 16, 17, 18 (bow, crossbow, boomerang, staff, wand, greatweapons, pitchfork,
the longsword 5 included), **16** for every other weapon (swords, axes, maces, daggers,
fists, shields 13, quivers 14 …). The store holds 32 entries for every item.

### 3.4 The palette (rebuilt every frame by the draw, 0x588500 …)

With an edited weapon *e* (material *m*ₑ, power *p*ₑ = p(*e*)), the palette is cleared and
filled in this order:

1. **Own-material cubes.** The total count *N* of bag stacks **exactly equal** (0x42f4a0:
   every field, spirits included) to the item (type 11, subtype 10 "Cube", material *m*ₑ,
   level 1, rarity 0, seed 0, product 0, flags 0, no spirits). If *N* > 0: entry (count
   *N*, that item).
2. **Spirits.** For *p* from max(1, *p*ₑ − 10) up to *p*ₑ, and inside each *p* for material
   128, 129, 130, 131: the total count *N* of bag stacks with type 11, subtype **14**
   ("Spirit"), that material and **p(item) = *p*** (rarity, seed and other fields ignored).
   If *N* > 0: entry (count *N*, item type 11, subtype 14, material, level **lvl(*p*)**,
   everything else 0).

So only spirits whose power is **at most** the weapon's power and **at least** 10 below it
are offered. If the selection index ≥ the entry count it is clamped to count − 1.

Spirit items: (11, 14, material 128 Fire / 129 Unholy / 130 Ice / 131 Wind), named
"Fire Spirit" etc. (`alpha-item-names.md`). Type 11 is stackable but subtype 14 is not a
quantity type, so +0x10 is the spirit's level. **Source in Alpha**: the zone generator gives
every **boss** spawn object (appearance flag 0x200) one spirit — type 11, subtype 14,
material 128 + (draw mod 4), **rarity 2**, level := the spawn's level (+0x34) — in its
inventory (client 0x5ed35c … 0x5ed3d0, server 0x52115b); a dying hostile drops its whole
inventory (`alpha-loot-rolls.md` §3.6), so **bosses drop one spirit each**.

### 3.5 Picking the target cell (each frame, 0x588500)

The weapon model (the item's model, 0x4ec400) is drawn in a 3D view (§3.7). From the mouse
position a ray is cast through that view into the model's voxel grid and walked **cell by
cell** (3D DDA) up to 1000 cells:

* cells outside the model's size count as empty;
* the walk stops at the first **solid model voxel** or the first **attached cube** of the
  weapon (the held cube, if any, is ignored); that cell is the **hit cell**, "target valid"
  := 1; the axis and direction of the last step are remembered (the hit face);
* if the hit is an attached cube and nothing is held, the **cursor item** becomes a Spirit
  (11, 14) with that cube's material and level and rarity 2 — its name and " +" + power are
  shown by the cursor label G+0x8008a8 (0x48b516 …);
* if nothing is hit, "target valid" := 0.

**Target cell**: the hit cell itself while nothing is held or selected (so a click there picks
up a cube); while **holding** a cube or with a palette entry **selected** (and no palette
entry hovered), the target is the **empty cell in front of the hit face** (hit cell minus
the last step). Without a hit, the ghost (below) floats 100 units along the ray and a
click does nothing.

### 3.6 Clicking (left press, 0x47b600 … 0x47bcee → 0x588250 / 0x58ce40)

On every left press while the host is visible and a weapon is set:

* **A palette entry is hovered** → toggle the selection: selected := hovered, or −1 if it
  was already selected (0x58ce40). No sound.
* **Otherwise** (0x588250), by state:

| State | Effect |
|---|---|
| target not valid (missed the weapon) | if holding and the mouse is over **no widget**, the held cube is **removed** from the weapon and lost (not returned to the bags); in all cases the hold and the selection are reset |
| holding a cube, target valid | **move**: remove the cube at its original cell (0x4c79b0), then place a cube with the held material and level at the target cell (0x4c69b0); hold := 0 |
| a palette entry selected, target valid, cube count < 32 | **place**: put (target cell, entry material, entry level) on the weapon (0x4c69b0); then remove **one** item from the **first** bag stack (bag order, slot order) whose type, subtype and material equal the entry's and whose power equals the entry's power (0x42f140) |
| nothing held or selected, target valid | if an attached cube sits at the target cell: **pick it up** (holding := 1, remember its cell, material and level); the cube stays on the weapon until it is dropped somewhere |
| a palette entry selected, cube count ≥ 32 | nothing |

**Place rule** (0x4c69b0): if a cube already exists at that cell its material and level are
**replaced**; otherwise, if count < capacity (§3.3), the entry is appended and the count
rises by 1; else nothing happens. The palette item is consumed in every case of the "place"
row — including when a 16-capacity weapon is full and nothing is added (a loss, as read).
**Remove rule** (0x4c79b0): the first entry with that cell is deleted and the later entries
move down one place; the count falls by 1.

There are **no costs** other than the consumed cube item, no level or class requirement
and no sound for place, move, pick up or drop. Palette selection plays nothing either.

### 3.7 Drawing (0x588500), in order

1. Palette and cursor item (§3.4, §3.5).
2. If a weapon is set and it has a model, the **3D view**: world shader, lights cleared,
   material (1, 1, 1, 1), then:
   * model matrix: rotate by *a* about X, *b* about Y, *c* about Z (degrees, 0.01745329 per
     degree, rows as the item-model routine of `alpha-ui-screens.md` §8.1.6), scale
     **0.25 / max(sx, sy, sz)**, centred on the model's middle (−½ size), placed **1 unit**
     in front of the camera;
   * perspective with vertical field of view **2·0.3927 rad (45°)**, aspect −W/H, near 0.1,
     far 1000, shifted so that the model centre lands on the window point (**w/2, 0.6·h**)
     = **(200, 375)**;
   * so the weapon's longest side spans about 1.2071·H·0.25 ≈ **0.30·H** pixels (217 px at
     H = 720).
3. **Ghost cube** (only when no palette entry is hovered and something is held or selected):
   the cube model at the target cell (or 100 units along the ray without a hit), coloured
   by the held material or the selected entry's material (0x4c7250, strength 0).
4. **Attached cubes**: each entry as one unit cube at (x, y, z) in model cells, coloured by
   its material (§3.9). The held cube is not drawn at its original cell. The cube at the
   hovered cell (target valid, no hold/selection) is drawn **1.1 ×** larger about its centre
   and **pulsing**: colour × (1 + 0.2·cos(0.01·*t*)), *t* = the game's millisecond clock.
5. **Palette grid**: entry *i* in column *c* = *i* mod 8, row *r* = ⌊*i*/8⌋: hit box
   (20 + 45*c*, 40 + 45*r*) … +40 × 40; the entry under the mouse becomes "hovered" (+0x2ac,
   its item copied to +0x2b0 → the standard tooltip, `alpha-ui-screens.md` §8.1.7 item 4,
   0x47b3e0). Model at the box centre (40 + 45*c*, 60 + 45*r*), turn vector G+0x800a1c,
   scale **0.02**, **0.03** when hovered or selected.
6. "Upgrades " + count + "/" + capacity at (**20**, *h* − 20 = **605**), 10 px, outline 2,
   white, left-aligned.
7. Title **"Weapon Customization"** at (15, 25), 12 px, outline 3, white — drawn even with no
   weapon (then steps 2–6 are skipped).
8. Palette counts: entry *i*'s count right-aligned at (60 + 45*c*, 80 + 45*r*), 10 px,
   outline 2 (outline pass white), fill white, **(0, 1, 1, 1)** for the selected entry.

### 3.8 How cubes change the weapon

* **Stats**: every attached cube, of **any** material (own-material cubes too), adds 0.1 to
  the level used for health, damage, armour and resistance: *v* = *L* + 0.1·*n*; speed and
  crit use *L* (`alpha-creatures-behaviour.md` §10.2). 32 cubes = +3.2 levels.
* **Combat effects**: only cubes of material **≥ 128** (spirits) — bursts and the
  fire/unholy/ice/wind effects with *c* = g(cube level) (`alpha-creatures-behaviour.md`
  §12). Own-material cubes have no effect beyond *v*.
* **Name**: unchanged (`alpha-item-names.md` §2 note 1). **Power** p(item) and price ignore
  cubes. **Equality** (stacking, recipe matching) compares cubes, so a customized weapon
  never stacks or matches a plain one.

### 3.9 How attached cubes are drawn elsewhere

Everywhere an item model is drawn (world items, held/equipped weapons through 0x4ac260, and
interface icons through 0x4758c0), the attached-cube routine 0x471b60 draws each entry with
the model's matrix moved by (x, y, z) model cells, colour from 0x4c7250, shine 1 for item
materials 1, 11, 12, 22 (`alpha-world-items.md` §2.5). Spirit colours (0x4c7250, strength *s*,
*f* = 1 + 0.5·*s*; *s* = 0 for ordinary drawing):

| Material | RGB | At *s* = 0 |
|---|---|---|
| 128 Fire | (*f*, 0.5*f*, 0.1*f*) | (1, 0.5, 0.1) orange |
| 129 Unholy | (0.3*f*, *f*, 0.5*f*) | (0.3, 1, 0.5) green |
| 130 Ice | (0.3*f*, 0.5*f*, *f*) | (0.3, 0.5, 1) blue |
| 131 Wind | (0.8*f*, 0.8*f*, *f*) | (0.8, 0.8, 1) pale |

Alpha for spirits = (input alpha + *s*)·*f*; the input colour is ignored. Own-material cubes
use that material's ordinary colour from the same routine (the input colour times the
material's factors), as the weapon's own voxels would.

---

## 4. Network and saves

* **Nothing is sent** by any of the three windows. Identify, Adapt and every bench edit
  change P's own bags, equipment, money or platinum on the client, exactly as crafting and
  vendors do (`alpha-talk-vendors.md` §3.5).
* An edited or adapted **equipped** item reaches other players through P's own creature
  update (equipment, field 44 of `alpha-net-packets.md` §1.2, 13 × 280 bytes, spirits
  included); the server does not validate it. Bag contents are not sent.
* **Saves**: the character save stores equipment (field 11) and bags with whole item
  records (spirits, count, flags), money (+0x1304, field 16) and platinum (+0x1308,
  field 17) (`alpha-saves.md`; the money/platinum meaning is settled by
  `alpha-ui-screens.md` §8.1.1). Window state (sources, selection, angles) is not saved.
* The draws of Identify (17 or 25) come from the shared CRT stream; in single-player they
  shift the world's later draws, as any other client-side `rand()` use.

---

## 5. Worked examples

### 5.1 Identifying Leftovers

A boar killed at tier 0 dropped Leftovers: type 14, level 12, rarity 2. P (money 150
copper) opens the identifier (class 131) and right-clicks the Leftovers in bag 0, slot 4.

1. Source := (0, 4); valid (type 14, count 1).
2. Price: *F*(12, 2) = 2^(3·0.354839)·2^0.5 = 2.09146·1.41421 = 2.95778; price =
   trunc(2.95778·10·0.2) = trunc(5.9156) = 5; **fee = max(1, 2) = 2** copper.
3. Drawn (1280 × 720, window at (850, 75)): model at window (175, 50 + *f*h/2); the
   Leftovers' text block at (15, 180) → screen (865, 255); "0 G " right edge 140,
   "0 S " 180, "2 C " 220 on baseline 235 → screen y 310; "COST:" at (15, 235).
4. Mouse at window (100, 270): inside the Identify zone (0 < 100 < 175, 255 < 270 < 285) →
   "Identify" cyan. Left press: money 150 → **148**; slot count 1 → 0 (type cleared); one
   draw — suppose it is even → weapon generator (12, 2, −1), 16 more draws; the weapon (rarity 2,
   level 12) is added to bag 0 by the add rule; sound 0x2e; chat "You receive <name> +36."
   (p = trunc(g(12)·100 + 1) = trunc(35.48 + 1) = 36). The window is now empty.

Other fees: (level 5, rarity 1) → price 3, fee **1**; (20, 3) → price 9, fee **4**; (30, 4) →
price 13, fee **6**.

### 5.2 Adapting a sword

P is level 8 with 20 platinum. A sword of level 5, rarity 1, flags 0 is right-clicked in
the bags while the Adaption window is open.

1. Valid (type 3, not adapted). *n* = 3, *q* = 2^1.25 = 2.37841, *t* = 4.75683 → cost 4, 8,
   **12**. Drawn: "12 Platinum Coins " right edge 220, baseline 350; source block at
   (15, 180) "… Power 17"; preview block at (225, 180) "… Power 26" (p(8) = trunc(25.93 + 1));
   arrow visible.
2. Left press on "Adapt" (window (100, 390)): platinum 20 → **8**; level 5 → **8**; flags
   |= 1; sound 0x2e. Its tooltip now reads "Power 26 (adapted)". The window shows
   "0 Platinum Coins ".

Further costs: (*l* 3, *r* 4, *L*ₚ 20): ⌊2·2^1.5⌋ = 5 per level, 17 levels → **85**;
(1, 0, 10): 2 per level → **18**; (15, 0, *L*ₚ 10): **0**, and the item drops to level 10.

### 5.3 Customizing a longsword

P owns an iron longsword (3, 5, material 1) of level 10, rarity 0, no cubes: *p*ₑ =
trunc(g(10)·100 + 1) = 32, capacity 32 (subtype 5). Bags: 5 Iron Cubes (11, 10, 1, level 1);
3 Fire Spirits level 12 (p 36); 2 Ice Spirits level 8 (p 26).

1. R on the bench: the window and the inventory open. Right-click the longsword in the bags:
   edited item := that slot, reset.
2. Palette: entry 0 = Iron Cube, count 5 (own material). Spirits for *p* = 22 … 32: Ice at
   *p* = 26 → entry 1 = Ice Spirit, level lvl(26) = trunc((1/0.74 − 1)·20 + 1) = 8, count 2.
   The Fire Spirits (p 36 > 32) are **not offered**. Grid: entry 0 box (20, 40), entry 1 box
   (65, 40); "Upgrades 0/32" at (20, 605).
3. Left-click entry 1 → selected (its count "2" turns cyan). Moving the mouse over the blade:
   the ray hits blade cell (3, 0, 14) through its +x face → target (4, 0, 14); a blue ghost
   cube is drawn there.
4. Left-click: entry (4, 0, 14, material 130, level 8) appended, count 1; one Ice Spirit is
   removed from the first matching stack (2 → 1). "Upgrades 1/32".
5. Effect: the longsword uses damage factor 4 (`alpha-creatures-behaviour.md` §10.3):
   4·*F*(10, 0) = **7.6266** → 4·*F*(10.1, 0) = **7.6642**. In combat (area flag,
   ρ = 1, one weapon) the ice cube adds 0.25·*c*·0.1 = 0.0065 damage with *c* = g(8) = 0.2593
   and slows the target 500 ms.
6. Clicking the new cube with nothing selected (right-click first to deselect) picks it up;
   clicking another blade face moves it; clicking outside every window while holding it
   destroys it (count 0; the spirit item is not returned).

---

## 6. Notes for AlphaVulkan

* **N-1** Keep all three windows client-side and packet-free for parity; a server-checked
  mode (anti-cheat for platinum/money and cube edits) would be a new feature, behind a
  switch.
* **N-2** Faithful quirks to reproduce by default, each behind a "strict vanilla" switch:
  Adaption lowers over-level items for free; a full 16-capacity weapon still consumes the
  placed cube item; dropping a held cube outside every widget destroys it; Goodbye reacts
  to any button's release; the Adaption preview block uses wrap 300 at x = 225.
* **N-3** The Adaption source pointer (equipment item) and the bench's edited-item pointer
  are raw pointers that outlive their slot (closing by inventory/movement keeps them). Store
  them as (container, index) handles and re-validate every frame; this changes nothing
  visible in normal play.
* **N-4** Put the tunables (fee divisor 2, cost base 2·2^((l+r−1)/4), capacity 16/32, power
  window 10, +0.1 level per cube, palette 8 × 45 px) in a data table for mods (modding-
  friendly rule). Spirit materials 128–131 should be an extensible list.
* **N-5** Mode 0 of the Identification widget (§1.7) is dead code; do not implement.

---

## 7. Corrections to other specs

* `alpha-crafting.md` §6.1: the title "Identification" is **12 px** (x = 15, y = 25), not
  15 px. §6.2: the worked example costs **12** platinum (4, 8, 12), and the cost is
  simply *n*·⌊2*q*⌋. The Adapt handler also lowers over-level items (§2.5 here).
* `alpha-talk-vendors.md` §4 / B-4: resolved by §1 and §2 here (fee, Identify roll with 17/25
  draws, Adaption rules); class 137 opening does not reset the source.
* `alpha-interaction.md` §5.3, static 77: the widget G+0x8008f4 is the **VoxelWidget
  "Weapon Customization"** (not an enchant/adaption widget); G+0x8008dc is its 400 × 625
  host, not a "preview window".
* `alpha-ui-screens.md` §2 anchors row "window G+0x8008dc (when the 3D preview is active)
  (W/2 − 340, H/2 − 250)": the layout routine places G+0x8008dc at (W/2 − 610, H/2 − 285)
  (0x483d1d …); the other placement could not be found (open point S-4). §8.1.2: the
  Enchant/Adaption hosts are indeed tops-aligned with the inventory (y = H/2 − 285).
* `alpha-world-items.md` W-5 (partly): spirit colours are §3.9 here; the cube model is
  *believed* the unit cube.
* `alpha-net-packets.md` P-1: the sub-block list is the attached-cube list of §0 here.

---

## 8. Open points

* **S-1** The `itemframe` and `rightarrow` template sizes and pivots (`gui.plx`) were not
  decoded, so the model centre's y (50 + frame height/2) and the arrow's exact extent are
  open. Check with a screenshot or the plasma decoder (`alpha-ui-plasma.md`).
* **S-2** The mesh used for attached cubes and the ghost cube (0x4e6df0's bound model) is
  *believed* the unit cube built by the VoxelWidget constructor; confirm in a recording.
* **S-3** The meaning of flag G+0x800704 (right press increments P+0x18c instead of
  resetting the bench) — *believed* a debug switch.
* **S-4** The second placement of G+0x8008dc quoted by `alpha-ui-screens.md` §2
  ((W/2 − 340, H/2 − 250)) was not found in 0x482a40.
* **S-5** The 3D view's exact matrix order (post-projection shift vs. view translation) is
  read only in outline; implement as "model centre lands on (w/2, 0.6·h), longest side
  0.25 units at distance 1" and verify against a screenshot of the bench.
* **S-6** Whether the hovered-attached-cube cursor label (§3.5) also shows while a palette
  entry is selected was not traced through 0x488ee0's full condition.
