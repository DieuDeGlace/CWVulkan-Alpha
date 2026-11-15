Status: FINAL / STABLE / HELD

# Alpha crafting: recipes, stations, the crafting window and the NPC item windows

Behavioural specification of crafting in Cube World **Alpha** `Cube.exe` (32-bit PE, image
base 0x400000): which recipes a character knows, what each recipe needs, which station it
needs and how "near" is decided, the crafting window and its blueprint preview, what
pressing the craft button does, and (briefly) the two item-service windows built next to it
(the image's `EnchantWidget` and `AdaptionWidget` classes). Every address is an **RVA in
Cube.exe**. Prose only; no code was copied.

Companion contracts named as pointers (not repeated here, not re-checked by this document):
`alpha-ui-screens.md` §8 (window table, InventoryWidget, slot/bag rules, the add rule, the
"can carry" wording, tooltips), `alpha-interaction.md` (static kinds, interaction),
`alpha-items-generator.md` (formula, weapon and armour generators), `alpha-item-names.md`
(item names), `alpha-saves.md` (save fields).

*Believed* marks readings that are not fully proven. Items that could not be confirmed are
listed under **Unresolved** (§8).

---

## 0. Conventions

* "G" is the game object, "P" the local player's creature (pointer at G+0x8006d0).
  Positions are signed 64-bit values in 1/65536 block (P+0x10, +0x18, +0x20 for x, y, z).
* An **item** is the 280-byte (0x118) record: type +0x00 (byte), subtype +0x01 (byte),
  seed +0x04 (i32), product +0x08 (i32), rarity +0x0c (byte), material +0x0d (byte), flags
  +0x0e (byte), level/quantity +0x10 (i16), 32 spirit entries of 8 bytes at +0x14, spirit
  count +0x114 (i32). Written here as **(type, subtype, material)**, with rarity 0, seed 0,
  product 0, flags 0, level field 1 and no spirits unless stated.
* **Equal items** (RVA 0x2f4a0): equal when type, subtype, seed, flags, product, rarity,
  level field, material and spirit count all match and every used spirit entry matches.
  Ingredient matching uses it, so an ingredient in the bags must match **exactly**,
  including rarity and the level field.
* A bag slot is 0x11c bytes: an i32 count followed by an item. P's bags are a vector of
  per-bag slot vectors at P+0x11dc/+0x11e0 (12 bytes per bag).
* *L* = P's level (P+0x190, i32). Class byte P+0x140: 1 warrior, 2 ranger, 3 mage, 4 rogue.
* Item names below follow `alpha-item-names.md` for readability; the (type, subtype,
  material) identifiers are the normative content. Ingredients: (11, 0, m) "*m* Nugget",
  (11, 1, 2) Wood Log, (11, 5, 21) Plant Fiber, (11, 6, 0) Cobweb, (11, 9, m) "*m* Yarn"
  (25 Silk, 26 Linen, 27 Cotton), (11, 10, m) "*m* Cube" (1 Iron, 2 Wood, 11 Gold,
  12 Silver), (11, 11, 27) Cotton Capsule, (11, 12, 24) Glass Flask, (11, 15) Mushroom,
  (11, 16) Pumpkin, (11, 17) Pineapple, (11, 20) Ginseng Root, (11, 21) Onion Slice,
  (11, 22) Heartflower, (11, 23) Prickly Pear, (11, 24) Iceflower, (11, 26) Water Flask,
  (11, 27) Snowberry.

---

## 1. Data

### 1.1 Known recipes (save field 18)

P+0x1d28 points to a small object whose +0x14 is a doubly linked **list of items** (the
**recipe keys**) and +0x18 its length. A key is simply the item the recipe makes. A key is
"known" (RVA 0x44a90) when an equal item is in the list. The list is saved as field 18
(`alpha-saves.md`). Keys are added by:

* the **level grants** (§2.2): at character creation (RVA 0x772b0 empties the list, then
  calls the grant with level 1 at RVA 0x777b6) and on every level change;
* **learning a formula** (a press on a type-2 bag item in the item-press handler RVA
  0x7b600): the key is a copy of the formula with **type := the formula's product field
  (+0x08, low byte)** and **product := 0**; subtype, seed, rarity, material, level and
  spirits are kept. Already known → sound 0x31 at the listener (G+0x140), nothing else.
  Otherwise the key is appended to the end of the list, one formula is taken from the stack,
  then sound 0x2f at the listener.

Appends always go to the end of the list; a key is appended only when it is not already
known.

### 1.2 Recipe record (0x128 bytes)

| Off | Size | Content |
|---|---|---|
| +0x000 | 0x118 | **product** = a copy of the key |
| +0x118 | i32 | **station** id (§3; 0 = none) |
| +0x11c | 12 | **ingredients**: a vector of 0x11c-byte entries, each an item (0x118) followed by an i32 **count needed** |

The **recipe builder** (RVA 0x19cff0) copies the key into the product, appends ingredients,
may write the station, and always reports success. It does not clear the station or the
ingredient vector itself; every caller passes a freshly initialised record (station 0, no
ingredients), so a recipe whose rule sets no station has station 0.

The record lives in three places: the per-tab recipe lists built by RVA 0xa14c0, the
preview widget (+0x16c … +0x293: product +0x16c, station +0x284, ingredients +0x288) and
the "recipe being crafted" at G+0x1000e7c (ingredients at G+0x1000f98) (§5).

---

## 2. The recipe list

### 2.1 Ingredients and station of every key (builder RVA 0x19cff0)

The builder looks at the key's type and subtype (and material). Each ingredient is a type-11
item with rarity 0, seed 0, product 0, flags 0, level field 1 and no spirits unless stated.
Then, **for every key** of any type, a **rarity surcharge** is appended last: key rarity
*r* = 1, 2, 3 or 4 adds **2 × (11, 0, material 12 + *r*) with rarity byte *r*** (2 Emerald,
Sapphire, Ruby or Diamond Nuggets for *r* = 1 … 4 respectively). Any other rarity adds
nothing.

**Consumables (type 1).** Category: subtypes 1 and 2 → **Alchemy** (tab 4), every other
subtype → **Cooking** (tab 3).

| Key | Product | Ingredients | Station |
|---|---|---|---|
| (1, 1) | Life Potion | 1 Heartflower (11, 22, 0) + 1 Water Flask (11, 26, 0) | none |
| (1, 2) | Cactus Potion | 1 Prickly Pear (11, 23, 0) + 1 Glass Flask (11, 12, 24) | none |
| (1, 4) | Ginseng Soup | 1 Ginseng Root (11, 20, 0) | campfire (9) |
| (1, 5) | Snowberry Mash | 1 Snowberry (11, 27, 0) | none |
| (1, 6) | Mushroom Spit | 1 Mushroom (11, 15, 0) + 1 Onion Slice (11, 21, 0) | campfire (9) |
| (1, 8) | Pineapple Slice | 1 Pineapple (11, 17, 0) | none |
| (1, 9) | Pumpkin Muffin | 1 Pumpkin (11, 16, 0) | campfire (9) |
| (1, any other subtype) | — | none | none |

**Ingredients made from ingredients (type 11).** Category: **tab 5** (header "Formulas",
icon `inventory-ingredients.png`, §4.1).

| Key | Product | Ingredients | Station |
|---|---|---|---|
| (11, 9, 25) | Silk Yarn | 1 Cobweb (11, 6, 0) | spinning wheel (5) |
| (11, 9, 26) | Linen Yarn | 1 Plant Fiber (11, 5, 21) | spinning wheel (5) |
| (11, 9, 27) | Cotton Yarn | 1 Cotton Capsule (11, 11, 27) | spinning wheel (5) |
| (11, 9, other material) | — | none | none |
| (11, 10, 2) | Wood Cube | 1 Wood Log (11, 1, 2) | sawbench (7) |
| (11, 10, *m* ≠ 2) | *m* Cube | 1 *m* Nugget (11, 0, *m*) | furnace (3) |
| (11, 22, any) | Heartflower | 1 Iceflower (11, 24, 0) | campfire (9) |
| (11, 26, any) | Water Flask | 1 Glass Flask (11, 12, 24) | water (1) |
| (11, any other subtype) | — | none | none |

**Weapons (type 3)**, category **Weapons** (tab 0). "Iron" = Iron Cube (11, 10, 1), "Wood" =
Wood Cube (11, 10, 2).

| Subtypes | Ingredients | Station |
|---|---|---|
| 0, 1, 2, 3, 13 | 8 Iron | anvil (4) |
| 4 | 6 Iron + 2 Cotton Yarn (11, 9, 27) | anvil (4) |
| 5, 15, 16, 17 | 16 Iron | anvil (4) |
| 6 | 15 Wood + 1 Linen Yarn (11, 9, 26) | workbench (8) |
| 7 | 6 Iron + 9 Wood + 1 Linen Yarn | workbench (8) |
| 8, 10, 11 | 16 Wood | workbench (8) |
| 12 | 8 Cubes of the key's own material (11, 10, key material) | anvil (4) |
| 9, 14 and ≥ 18 | none | none |

(Subtype readings per `alpha-item-names.md`: 0–2 sword/axe/mace, 3 dagger, 4 fist,
5 longsword, 6 bow, 7 crossbow, 8 boomerang, 10 staff, 11 wand, 12 bracelet, 13 shield,
15–17 great weapons.)

**Armour (types 4–7)**, category **Armor** (tab 1). Count by type: 4 (chest) **20**,
5 (gloves) **6**, 6 (boots) **8**, 7 (shoulders) **8**.

* key material 1 → that many Iron Cubes (11, 10, 1), **anvil (4)**;
* any other material *m* → that many (11, 9, *m*) Yarn of the same material, **loom (6)**.

**Amulets and rings**, category **Amulets** (tab 2): type 8 → 6 Cubes of the key's material
(11, 10, *m*); type 9 → 3 Cubes of the key's material; **no station**.

Types 2, 10, 0 and every type ≥ 12 are **not listed** in any tab (the list builder skips
them), although the builder still appends a rarity surcharge for them.

The builder's working ingredient keeps its material from one append to the next unless it is
set again; every row above states the resulting material.

A listed key with no ingredients and no station (for example (1, 0), (1, 3), (1, 7)) would
show count 0 but would pass every craft-button check (§4.2 step 6); none of these keys is
produced by the native grants (§2.2).

### 2.2 Level grants (RVA 0x7fae0, called with the new level)

Called with level 1 at character creation (RVA 0x777b6) and from the frame update (RVA
0x88ee0) whenever P's level (P+0x190) differs from the last level seen (P+0x1d3c): the check
is at RVA 0x93d78; on a difference the frame update shows a level-up notice, plays sound
0x1d at P's position, sets the last level seen := *L*, sets P+0x1d40 := 0, then calls the
grant with *L* (RVA 0x93ec4). Only the **new** level is granted; a jump of several levels
skips the levels in between. Each key is appended only if not already known. Keys have
rarity 0, product 0, flags 0, no spirits; the level field is *L* unless stated.

Grant order:

| Order | Condition | Keys granted |
|---|---|---|
| 1 | *L* mod 4 = 1 | (1, 1) Life Potion |
| 2 | *L* mod 4 = 3 | (1, 2) Cactus Potion |
| 3 | *L* mod 5 = 1 | (1, 4) Ginseng Soup, then (1, 8) Pineapple Slice |
| 4 | *L* mod 5 = 2 | (1, 9) Pumpkin Muffin |
| 5 | *L* mod 5 = 3 | (1, 6) Mushroom Spit |
| 6 | *L* mod 5 = 4 | (1, 5) Snowberry Mash |
| 7 | *L* = 1 | (11, 26, 0) Water Flask; (11, 22, 0) Heartflower; rogue: (11, 9, 27); ranger: (11, 9, 26); mage: (11, 9, 25), attempted twice (the second is a no-op); then (11, 10, 1), (11, 10, 2), (11, 10, 11), (11, 10, 12); all level 1. Warriors get no yarn key. |
| 8 | every *L* | one **armour** key, then one **weapon** key (below) |

**Armour key**: type by *L* mod 4: 0 → 7, 1 → 4, 2 → 6, 3 → 5; subtype 0; **seed =
rand() mod 11** (first draw of the grant); material by class: warrior 1, ranger 26,
mage 25, rogue 27 (any other class leaves the material at 12 after the level-1 block, else
0; *believed* unreachable); level *L*.

**Weapon key**: type 3, **seed = rand() mod 11** (second draw), level *L*; subtype and
material by class and *L* mod 3:

| Class | *L* mod 3 = 0 | = 1 | = 2 |
|---|---|---|---|
| warrior (material 1) | subtype 15 + (rand() mod 3) | subtype rand() mod 3 | 13 |
| ranger (material 2) | 6 | 7 | 8 |
| mage | 10, material 2 | 11, material 2 | 12; material 12 if the next rand() is odd, 11 if even |
| rogue (material 1) | 3 | 5 | 4 |

(For another class the weapon key keeps subtype 0 and the armour key's material.)

Draws per grant, in order: armour seed, weapon seed, then one more for a warrior with
*L* mod 3 ∈ {0, 1} or a mage with *L* mod 3 = 2. They come from the C runtime `rand`
(MSVCR110) with no re-seeding here, so the seeds depend on all earlier draws in the
process. Because the key includes the seed, a new equipment key pair is usually added at
every level.

### 2.3 Formula-taught equipment

A formula comes from the formula generator (`alpha-items-generator.md`). Learning it
(§1.1) gives a key with the formula's product as type and the formula's subtype, seed,
material, rarity *R* and level. Its recipe is §2.1 for that type/subtype/material plus the
rarity surcharge for *R*. The product made is exactly the key: **its level is the formula's
level, not the player's**, and its rarity is the formula's.

### 2.4 Building the six tab lists (RVA 0xa14c0, per tab RVA 0xa19d0)

Run when the crafting window is toggled by the C key (RVA 0x88bd0 toggles the crafting host
G+0x8008c0 and then rebuilds; key dispatch at RVA 0x7e769, key code 0x43) or by a station
(RVA 0x975b6), after a craft completes (RVA 0x70d5c), at the end of several bag/equipment
press paths of the item-press handler (RVA 0x7dd88), and when an incoming item-list entry
addressed to P is processed in the frame update (RVA 0x8f8d0; *believed*: the world-update
item section).

1. For every key in the known list (list order), build its recipe (§2.1) and put it in the
   category of its type: 3 → tab 0; 4–7 → tab 1; 8, 9 → tab 2; 1 with subtype 1 or 2 →
   tab 4; other 1 → tab 3; 11 → tab 5; anything else → dropped.
2. **Sort** each category in **descending** order of **6 · level + rarity** (level the i16
   field, rarity the unsigned byte), with the MSVC standard sort: for 32 or fewer recipes an
   insertion sort with a strict comparison, which keeps list order among equal keys; larger
   lists use the introsort path, which may reorder ties.
3. Crafting bag *t* (G+0x800adc, a vector of one slot vector per tab, grown if needed) is
   cleared and refilled, one slot per recipe in sorted order: **count** = min over the
   recipe's ingredients of ⌊have / need⌋, where *have* = the sum of the counts of all of P's
   bag slots (all bags) holding an equal item; a recipe without ingredients gets count
   **0**; the slot's item = the product.
4. If this tab and row equal the crafting widget's (G+0x800958) selected tab +0x18c and row
   +0x190, the preview's (G+0x800964) product, station and ingredients := this recipe's.

Counts are therefore never negative.

---

## 3. Stations

### 3.1 Station ids

| Id | Needs | Static kind | Text (when missing) |
|---|---|---|---|
| 0, 2 and any unlisted id | nothing | — | — |
| 1 | water | — | "Requires water" |
| 3 | furnace | 71 | "Requires Furnace" |
| 4 | anvil | 72 | "Requires Anvil" |
| 5 | spinning wheel | 73 | "Requires Spinning Wheel" |
| 6 | loom | 74 | "Requires Loom" |
| 7 | sawbench | 75 | "Requires Saw" |
| 8 | workbench | 76 | "Requires Workbench" |
| 9 | campfire | 65 | "Requires Campfire" |

Id 2 is never produced by the builder. Static kind 77 (customization bench) is not a
crafting station: interacting with it opens the spirit editor (G+0x8008f4) and toggles the
window G+0x8008dc, with the inventory made visible exactly when that window is (RVA
0x97437 … 0x9747e).

### 3.2 "Near" (decided every frame by the preview draw, RVA 0x2f910)

* **Static stations (3–9).** P's block x and y (position ÷ 65536, truncated toward zero),
  then the 8-block **cell** (÷ 8, truncated toward zero). Every static in the 3 × 3 cells
  (cell x − 1 … + 1, cell y − 1 … + 1, x outer loop) of the zone cell index (lookup RVA
  0x2f640) is tested: if its kind is the station's kind and the **3-D squared distance** in
  blocks between its position (record +0x08, +0x10, +0x18, i64 fixed point) and P's
  position is **< 16**, the station is present and the search stops. So: **within 4
  blocks**, in any direction, of the static's anchor point. No other static state is
  checked.
* **Water (1).** Present when P+0x5c has bit value 2 set (*believed* "in water"), or else
  when the block at P's x, y and z lowered by half of P+0x88 (*believed* P's height, in
  blocks, scaled by 65536) has block type 3 (water; low 5 bits of the block's byte 3).
* **Every other id**: always present. So amulets, rings, Life and Cactus Potions,
  Snowberry Mash and Pineapple Slice can be made anywhere, with the window opened by **C**.

When missing, the text is drawn (§4.2 step 5) and the craft button is disabled (step 6).

---

## 4. The crafting window

### 4.1 The list window (crafting widget G+0x800958, host G+0x8008c0)

* **Opening**: the C key toggles the host and rebuilds (§2.4). Interacting with a static of
  kind 65 or 71–76 toggles the host (RVA 0x9756a), then selects tab 3 for kind 65 and tab 5
  for kinds 71, 73 and 75 (kinds 72, 74, 76 keep the current tab), then rebuilds. Opening
  the skills window (X key, RVA 0x88c70) hides the crafting host and the preview host.
  Every frame (RVA 0x8c677 in the frame update) the preview host (G+0x800ad4) is made
  visible exactly when the crafting host is, and while it is visible a widget routine of
  the crafting widget is also run (RVA 0x87e60).
* **Tabs** (built once at start-up, RVA 0xa1e50, called only from the start-up routine at
  RVA 0x644f7): with P's class at that moment 1–4, six tab icons in order:

  | Tab | Header text when selected | Icon |
  |---|---|---|
  | 0 | "Weapons" | warrior `craft-melee-weapons.png`, ranger `craft-ranged-weapons.png`, mage `craft-magic-weapons.png`, rogue `craft-rogue-weapons.png` |
  | 1 | "Armor" | warrior `craft-heavy-armor.png`, ranger `craft-medium-armor.png`, mage `craft-light-armor.png`, rogue `craft-rogue-armor.png` |
  | 2 | "Amulets" | `craft-amulets.png` |
  | 3 | "Cooking" | `craft-cooking.png` |
  | 4 | "Alchemy" | `craft-alchemy.png` |
  | 5 | "Formulas" | `inventory-ingredients.png` |

  For any other class only the last four icons are built. The header text is chosen from
  the widget's current tab (+0x1b4; 0–4 as above, anything else "Formulas") by the
  InventoryWidget draw (RVA 0xc2050).
* **Rows**: one slot per recipe in sorted order, laid out by the InventoryWidget rules
  (`alpha-ui-screens.md` §8.1). For a widget of type 2 (field +0x194), a row whose item
  type is non-zero and whose count is > 0 also shows the count: outlined text, 10 px, an
  outline pass with stroke 2 then a fill pass, white with black outline, alignment code 2
  (*believed* right-aligned), at widget-space x = 6 + *sw* + col·(*sw* + 5) and
  y = 37 + *sh* + row·(*sh* + 5), where *sw*/*sh* are the widget's cell width and height
  (+0x1a8, +0x1ac) and col/row the slot's position after scrolling (i.e. slot x + *sw* − 4,
  slot y + *sh* − 3 relative to the selector origin below). Count-0 rows show no number.
* **Selection**: in the item-press handler (RVA 0x7b600), while the crafting host is
  visible, the crafting widget's press routine (RVA 0xc6610) runs: if a row is hovered
  (hover tab +0x184, row +0x188 ≥ 0) it becomes the selection (+0x18c, +0x190) and sound
  **0x55** plays (non-positional); the selector child is shown at widget-space
  (10 + col·(*sw* + 5), 40 + row·(*sh* + 5)) when the selected row is on screen, else
  hidden. Then the preview recipe is rebuilt from the selected row's slot item with the
  builder (§2.1); if the selection is out of range, the preview gets an empty recipe
  (empty product, station 0, no ingredients).

### 4.2 The preview window (BlueprintPreviewWidget, constructor RVA 0x2f190, draw RVA 0x2f910)

Fields: game +0x294, a second widget pointer +0x298 (*believed* the item shadow; not used
by the draw), craft button +0x29c and craft bar +0x2a0 (both made children of the panel),
the bar's child named `bar` at +0x2a4, recipe +0x16c, model angles in degrees
+0x160/+0x164/+0x168 (constructed −120, 0, 0), hovered ingredient index +0x2a8, hovered item
+0x2ac, "button ready and hovered" byte +0x3c4.

Coordinates below are **panel-local** (*w*, *h* = the panel's width and height) unless
stated; mouse tests use the mouse position relative to the panel.

**Every frame:**

1. **Model.** If the preview's product is a **known key** (§1.1): the product's model.
   Otherwise: model slot 2396 of the game's model table (nothing is drawn this frame if the
   table is shorter). If the chosen model is absent, nothing at all is drawn this frame.
2. The model turns: +0x168 += (frame time in ms, *believed*) × 0.02 (degrees). Scale =
   0.06 / (largest model dimension), × **1.5** while the mouse is strictly inside x ∈
   (*w*/2 − 60, *w*/2 + 60), y ∈ (50, 170); the product then becomes the hovered item.
   The hovered index is reset to −1 and the hovered item cleared before this test, every
   frame. Centre: panel origin + (*w*/2, 110). Projection: vertical field of view π/4,
   near 0.1, far 1000 (depth terms 1.0001 and −0.10001), the model placed 1 unit ahead and
   shifted so that its centre lands on that screen point. Lights: front (1, 1, 1, 1), back
   (0.2, 0.2, 0.4, 1), ambient (0.4, 0.4, 0.4, 1), direction (0.7, 0.5, −1) normalised
   (divided by √1.74); material colour (1, 1, 1, 0.9).
3. **Name.** Known: item name, followed by " +" and the item's power unless the type is 0,
   12, 13, 20, 21, 23, 24, 25, or 11 with a subtype other than 14; 12 px, an outline pass
   with stroke 3 then a fill pass, alignment code 0x11 (*believed* centred with wrapping),
   at (half the panel's horizontal extent, 30). Unknown: "Unknown", 15 px, stroke 3, white
   with black outline, at (15, 65).
4. **Ingredients** (known only), entry *k* of the list, *C* = trunc((*w* − 10)/45)
   columns (**7** at *w* = 350):
   * box: top-left (10 + 45·(*k* mod *C*), *h* − 120 + 45·⌊*k*/*C*⌋), 40 × 40; model at
     its centre (+20, +20), scale **0.025**, **0.0375** while the mouse (screen position)
     is in the box (left/top edges inclusive, right/bottom exclusive); that ingredient then
     becomes the hovered item and its index goes to +0x2a8;
   * "have/need": *have* = the sum of the counts of P's non-empty bag slots holding an
     equal item, *need* = its count; text *have* + "/" + *need*, 10 px, outline pass stroke
     2 then a fill pass, alignment code 2 (*believed* right-aligned), at (50 + 45·col,
     *h* − 75 + 45·row); fill **(0, 1, 0, 1)** when *have* ≥ *need*, **white** when short.
     Any short ingredient marks the recipe "missing".
5. **Station** (§3, for every recipe including unknown ones): if missing, the
   "Requires …" text: 9 px, outline pass stroke 3 then fill pass, fill
   **(1, 0.25, 0.25, 1)**, black outline, alignment code 0x10 (*believed* left with
   wrapping), at **(15, *h* − 40)**.
6. **Craft button frame** (the button's child `frame`), only if the button exists: white
   (1, 1, 1, 1) and +0x3c4 := 0 when a station or an ingredient is missing or the product is
   not a known key; otherwise **(0, 1, 1, 1)** with +0x3c4 := 1 when the mouse is over the
   button, else **(0, 1, 0, 1)** with +0x3c4 := 0.
7. **Craft bar**: if it exists, it is visible exactly while the craft timer (§5) is ≥ 0.
   Its `bar` child is scaled each frame by the frame update (RVA 0x34c20) by the factor
   (bar width − 8) · max(timer, 0) / (the child's horizontal extent), i.e. its width
   becomes (bar width − 8) · timer (*believed*).

### 4.3 Worked layout (panel-local offsets)

With *w* = 350, *h* = 330 and the panel's top-left at (*X*, *Y*): product model centre
(*X* + 175, *Y* + 110); product hover box x *X* + 115 … *X* + 235, y *Y* + 50 … *Y* + 170;
name at (*X* + 175, *Y* + 30); ingredient 0 box (*X* + 10, *Y* + 210) … (*X* + 50,
*Y* + 250), ingredient 1 at *X* + 55; the 8th ingredient (row 1) at (*X* + 10, *Y* + 255);
count text of ingredient 0 at (*X* + 50, *Y* + 255); "Requires …" at (*X* + 15,
*Y* + 290).

---

## 5. Crafting (frame update RVA 0x8f183 … 0x8f23d; completion RVA 0x709c0)

### 5.1 Start

G+0x1000e78 is the **craft timer** (float; −1 = idle). Only while the preview host is
visible: if the preview's +0x3c4 byte is set (as last written by the preview draw), the
input byte G+0x04 is non-zero (*believed* left mouse button held) and the timer is < 0 →
timer := 0 and the preview's recipe is **copied** to G+0x1000e7c (product, station,
ingredients). Nothing is sent, no player action or animation is started, no sound plays at
the start.

### 5.2 Progress

Still only while the preview host is visible, in the same frame and after the start test:
if timer ≥ 0, timer += (frame time in ms) × 0.001; if the result is ≥ **1** the timer
becomes −1 and the craft completes (§5.3). The bar is then scaled from the timer (§4.2
step 7). If the window is closed the timer **freezes** and resumes when it is reopened.
Holding the button keeps crafting: the next craft starts on a later frame whenever the
start conditions hold again.

### 5.3 Completion (RVA 0x709c0)

Works on the copied recipe; it does **not** re-check the station or the ingredients.

1. Copy the product. If P **cannot carry** it (RVA 0x3e4a0: for product types 1, 11 and 20
   the equal items in all bags, plus a matching item held on the cursor (P+0x11e8 count;
   *believed* the cursor item), are counted; type 20 with any count > 0, or any of these
   types with 50 or more, cannot be carried) → chat "You can't carry more of these items."
   in (1, 0.2, 0.2, 1), sound 0x31 at P; nothing is consumed; stop.
2. **Remove ingredients.** For each ingredient in order (count needed *n*), walk P's bags
   in order and each bag's slots in order; at each slot holding an equal item:
   * if *n* < slot count: slot count −= *n*, and the walk continues with the **next bag**
     (the same *n* is still applied there);
   * else: the slot is emptied (count 0, item replaced by an empty item) and *n* is reduced
     by the slot's count **after** it was set to 0, i.e. not at all, and the walk continues
     with the next slot.

   With a single stack per ingredient this removes exactly *n* (or empties a stack of
   exactly *n*). With several equal stacks, in **each bag** every equal stack ≤ *n* before
   the first larger one is emptied, that first larger one loses *n*, and later equal stacks
   in the same bag are untouched.
3. If the product type is 3–9: flags +0x0e |= 1 (crafted equipment counts as adapted and is
   refused by the Adaption window, §6.2).
4. Sound **0x59** at the listener (G+0x140).
5. If the product type is 11 and its subtype is not 26: draw rand() mod 5; if 0, a
   **bonus** of 1, then draw again: if 0 the bonus becomes 2 and a third draw: if 0 it
   becomes 3. The bonus is added to the quantity field (+0x10) and sound **0x5a** plays at
   the listener. Probabilities 4/25 (+1), 4/125 (+2), 1/125 (+3). Draws: 1, 2 or 3 calls,
   only for these products.
6. Add the product with the inventory add rule (RVA 0x6ebe0, automatic bag), run two
   inventory-widget refresh routines (RVA 0xc6350, 0xc64c0), chat "You receive …" (RVA
   0x80fb0), rebuild the recipe lists (§2.4).

**The product's level** is the key's level (the level at which the grant happened, or the
formula's level); the player's current level plays no part.

### 5.4 Network

The routines for crafting completion, formula learning, level grants, identification and
adaption change only P's bags, money (P+0x1304), platinum (P+0x1308) and known list; none of
them queues an outgoing network record (*believed*, from the routines read for this
document).

---

## 6. The item-service windows (briefly)

Both are built at start-up next to the inventory. NPC interaction (RVA 0x889e0): an NPC of
class **131** shows the **Identification** host (G+0x8008f8) and sets its widget
(G+0x8008fc) source := none (−1, −1) and mode (+0x168) := 1; an NPC of class **137** shows
the **Adaption** host (G+0x800900). Bag-slot presses in the item-press handler set the
Identification widget's source to (bag +0x160, slot +0x164) and the Adaption widget's
(G+0x800904) source to (bag +0x164, slot +0x168) with its item pointer +0x160 := none
(*believed*: while the respective window is open; the press conditions were not
re-checked).

### 6.1 EnchantWidget (constructor RVA 0x4e910, draw RVA 0x4ea30): "Identification"

* Title "Identification", 12 px, stroke 3, at (15, 25); buttons "Identify" and "Goodbye!"
  (12 px). The constructor sets mode := 1.
* Source: a valid bag slot with non-zero count whose item is **type 14** (RVA 0x50b70).
* Cost (RVA 0x50920): 0 without a source, else max(1, ⌊price/2⌋) with the item price of
  RVA 0xc76e0 (in copper). The draw uses a grey (0.7, 0.7, 0.7) colour for the cost
  (*believed* when it cannot be paid or there is no valid source).
* **Identify** (in the item-press handler: host visible, the button test RVA 0x50ab0
  passes, money ≥ cost, mode 1, valid type-14 source): money −= cost; one item is taken
  from the stack (when the stack empties, its count and type/subtype bytes become 0 but the
  other fields stay); then one rand(): even → weapon generator (RVA 0x1f8ad0), odd →
  armour generator (RVA 0x1f51e0), each called with the source slot's level and rarity
  (read after the take) and class −1 (`alpha-items-generator.md`); the result is added
  (add rule), the inventory is refreshed, sound 0x2e at P, chat "You receive …".
* A mode-0 path (*disenchant*) also exists: with a valid type-14 source whose material is
  non-zero, it takes one item, clears the source if the stack is still non-empty, draws
  5 + (rand() mod 6) and appends that many type-10 items of the source's material to a
  list (*believed* an incoming-item queue), clears G+0x100e70 and plays 0x2e at P; then it
  takes one more item regardless of material. No money is deducted. Nothing found sets
  mode 0 (*believed* unreachable in Alpha).

### 6.2 AdaptionWidget (constructor RVA 0xecd0, draw RVA 0xf8f0): "Adaption"

* Title "Adaption" (12 px, stroke 3, at (15, 25)); buttons "Adapt" and "Goodbye!".
* Source (RVA 0xf570): an item pointer (+0x160) if set, else a valid bag slot (+0x164,
  +0x168) with non-zero count; accepted only for types 3–9 with flags bit 0 clear.
* **Cost** (RVA 0xf4f0) in **platinum coins**: with *l* = item level (i16), *r* = rarity
  byte: if *l* ≥ *L* the cost is 0; else *q* = 2^((*l* + *r* − 1) · 0.25) (computed in
  double, then stored as single precision), and the cost starts at 0 and, *L* − *l* times,
  becomes trunc(cost + 2*q*) (single-precision addition).
* **Adapt** (host visible, the button pressed, valid source, platinum ≥ cost): platinum −=
  cost; item level := *L*; flags |= 1; sound 0x2e at P. Example: item level 5, rarity 1,
  player level 8: *q* = 2^1.25 ≈ 2.3784, 2*q* ≈ 4.7568; cost 4, 8, 12 → **12 platinum**.

---

## 7. Worked examples

### 7.1 Cooking a Pumpkin Muffin at a campfire

A ranger reached level 2: the grant (*L* mod 5 = 2) added key (1, 9) level 2; the same grant
added boots (2 mod 4 = 2 → type 6) material 26 and a weapon of subtype 8 (2 mod 3 = 2)
material 2.

1. The bags hold one stack of 3 Pumpkins (11, 16, 0). The player stands at
   (1000.5, 2000.5, 30.0); a campfire static (kind 65) sits at (1002.0, 2001.0, 30.0).
2. Interacting with the campfire toggles the crafting window, selects tab 3 "Cooking" and
   rebuilds the lists. The Cooking list holds (1, 9) level 2 → sort key 6·2 + 0 = 12 (the
   only entry). Its row count = ⌊3/1⌋ = 3, drawn as "3".
3. Clicking the row selects it (sound 0x55); the preview shows the muffin model, the name
   with " +" and its power (type 1 is not excluded), one ingredient box with "3/1" in green.
4. Station 9: P's cell (125, 250); the campfire is in cell (125, 250); squared distance
   1.5² + 0.5² + 0 = 2.5 < 16 → present, no "Requires Campfire"; the button frame is green,
   cyan under the mouse.
5. Holding the button: the timer runs 0 → 1 in about 1 s and the bar fills. Completion: can
   carry (no muffins yet); the Pumpkin stack 3 > 1 → 2; type 1 → no adapted flag, no bonus,
   no extra draw; sound 0x59; the muffin (1, 9, level 2) is added by the add rule; chat
   "You receive …"; lists rebuilt → row count 2. Still holding → another muffin about a
   second later.

### 7.2 Forging a sword from a formula at an anvil

1. A warrior learns a formula: type 2, product 3, subtype 0, material 1, seed 37, rarity 2,
   level 6. Not known → key (3, 0, seed 37, material 1, rarity 2, level 6, product 0)
   appended, one formula removed, sound 0x2f.
2. Recipe (§2.1): **8 Iron Cubes** (11, 10, 1) + rarity surcharge **2 × (11, 0, 14) with
   rarity 2**; station 4 (anvil). Category tab 0; sort key 6·6 + 2 = 38 (above the grant
   weapons of levels 1–6, keys 6 … 36).
3. Iron Cubes come from the furnace recipe (11, 10, 1): 1 Iron Nugget each, about 1 s
   each; the bonus adds 0.248 cubes per craft on average (10 nuggets give about 12.5 cubes
   on average).
4. Bags: 12 Iron Cubes, 1 matching Sapphire Nugget. Preview: "12/8" green, "1/2" white →
   missing; button white. Row count = min(⌊12/8⌋, ⌊1/2⌋) = 0 (no number). The player is
   3 blocks from an anvil: no "Requires Anvil", but the button stays disabled.
5. With a second matching nugget in the same stack (2): "2/2" green, row count 1, button
   green. Craft: Iron 12 → 4; nuggets 2 ≤ 2 → stack emptied; the sword (3, 0, seed 37,
   material 1, rarity 2, level 6) gets flags bit 0; sound 0x59; added by the add rule; chat
   "You receive …".

### 7.3 Grants of a new level-1 mage

*L* = 1, in order: (1, 1); (1, 4); (1, 8); (11, 26, 0); (11, 22, 0); (11, 9, 25) (second
attempt a no-op); (11, 10, 1), (11, 10, 2), (11, 10, 11), (11, 10, 12); armour (4, 0,
material 25, seed = first rand() mod 11, level 1); weapon (3, 11, material 2, seed = second
rand() mod 11, level 1). Two draws.

---

## 8. Unresolved

* **U-1** Tab icons are chosen once at start-up from P's class (RVA 0xa1e50). If P has no
  class 1–4 at that moment, only four icons are built; whether the running game reaches this
  state is not established.
* **U-2** The rarity-surcharge nuggets (11, 0, material 13–16) must carry rarity 1–4 to
  match; whether such items are obtainable in Alpha is not established (otherwise
  rarity ≥ 1 formulas cannot be crafted).
* **U-3** The preview name's fill/outline colours (the draw passes white for the visible
  colour arguments; any rarity colouring is not established), the semantics of the text
  alignment codes 2, 0x10 and 0x11, the wrap widths, and the craft button/bar geometry
  (from `gui.plx`).
* **U-4** Type 14 as "unidentified item" versus the name table's wording; what model slot
  2396 shows.
* **U-5** The meaning of P+0x5c bit value 2 and of P+0x88 used by "Requires water".
* **U-6** Whether loading a save (field 18) or any path other than character creation
  removes keys from the known list was not re-checked.
* **U-7** Not re-checked here (companion facts): the item names and sound file names; the
  power value and its formula; which bag the add rule chooses; the absolute placement and
  size of the crafting host and preview panel (companion: 350 × 330 panel); the identity
  of the frame-time source and of input byte G+0x04; the mouse buttons that feed the
  item-service windows; right-press closing behaviour of the crafting window.
* **U-8** The cost-display format of the Identification window and the exact condition for
  its grey colour.

---

## Re-certification (2026-11-15)

Checked against Cube.exe (SHA-256 84a7a132…30bf) decompile and assembly, with switch
tables, constants and strings read from the original image bytes (dumpbin and direct reads).

**Confirmed.** Item layout and the equality order; known-list location and the learning
transform with sounds 0x31/0x2f; all recipe rows, counts, materials and stations of the
builder, including the rarity surcharge and the material carry-over; category routing
(switch table decoded) and the 6·level + rarity descending sort with the ≤ 32 insertion
path; tab-bag counts (min of ⌊have/need⌋, 0 without ingredients) and the selected-row
preview update; the level-grant order, conditions, materials, subtypes and the exact
random-draw order (armour seed, weapon seed, optional third draw); character-creation
clearing and grant(1); the level-change trigger and sound 0x1d; station ids, kinds and texts;
the 3 × 3-cell, squared-distance < 16 test for all seven static stations; the water test;
C-key and station toggles and tabs 3/5; the X key hiding the crafting windows; tab icon
files and header texts; preview constants (angles −120/0/0, 0.02°/ms, 0.06 and × 1.5 scale,
hover box, centre +110, 0.025/0.0375 ingredient scale, 7 columns, 45-px grid, h − 120 boxes,
(50, h − 75) counts, green/white, "Requires" at (15, h − 40) in (1, 0.25, 0.25, 1), button
colours and +0x3c4); timer 0.001 × ms to 1.0; completion order, can't-carry message and
colour, adapted flag for types 3–9, sound 0x59, bonus draws and odds with sound 0x5a,
rebuild; NPC classes 131/137; identification weapon/armour split by rand() parity; adaption
acceptance and cost formula.

**Corrected** (old → new):
1. Light direction: "(0.7, 0.5, −1)/√2" → normalised, divided by √1.74.
2. Adaption worked example: "4, 8, 13 → 13 platinum" → 4, 8, 12 → **12 platinum** (the
   formula itself was right; gameplay number).
3. Ingredient over-removal summary: "every stack ≤ *n* emptied, the first larger one loses
   *n*" → applies **per bag**; later equal stacks in a bag are untouched and the same *n*
   continues into every later bag (gameplay behaviour with several stacks).
4. Can-carry: "types 1, 11, 20: 50 or more" → also type 20 with any count > 0, and a
   matching cursor item is counted (*believed* cursor).
5. Preview name power suffix: "non-quantity types" → the explicit excluded type list.
6. Identification title: 15 px → 12 px; the mode/source fields belong to the widget
   G+0x8008fc, not the host G+0x8008f8.
7. Disenchant (mode 0) path: "one item removed" → one item, plus a second take; requires a
   non-zero material; no money deducted (still *believed* unreachable).
8. Crafting row count position: "slot y + 37" → y = 37 + *sh* + row·(*sh* + 5) (equal to
   slot y + *sh* − 3); visibility also requires a non-zero item type.
9. Station default: the builder does not initialise the station; callers pass station 0.
10. Tab "titles" → header text chosen from the current tab; per-class icons via start-up
    texture slots.
11. Start and progress happen in the same frame (the timer advances on the start frame).
12. The level-change step also clears P+0x1d40; sound 0x1d is played at P's position.
13. "Two players of the same class get different seeds" → seeds depend on the shared CRT
    `rand` state; no re-seeding here.
14. All addresses converted from virtual addresses to RVAs in Cube.exe.

No random-draw order was changed.

**Removed.** The "Notes for AlphaVulkan" section (implementation advice and switches) and
the open point asking for in-game checks; the claim about the hover test skipping negative
rows and the InventoryWidget negative-count remark; power values (+5, +21), tooltip text,
bag numbers and the 1280 × 720 absolute layout in the examples (not re-derived; the layout
is restated as panel-local offsets); the claim that nothing but character creation removes
keys (moved to U-6); the sound file names and server-knowledge statements (moved to U-7 or
softened to the routines read).

**Unresolved.** U-1 … U-8 above. The Infinite-Cubes reconstruction was not consulted.
