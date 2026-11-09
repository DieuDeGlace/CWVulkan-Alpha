# Alpha crafting: recipes, stations, the crafting window and the NPC item windows

Behavioural specification of crafting in Cube World **Alpha** `Cube.exe`: which recipes a
character knows, what each recipe needs, which station it needs and how "near" is decided,
the crafting window and its blueprint preview, what pressing the craft button does, and
(briefly) the two item-service windows built next to it (`EnchantWidget`, `AdaptionWidget`).
Written from the user's Ghidra decompile of the client and from the assembly (several
switch tables were decoded from the binary, because the decompiler lost their case bodies).
No code was copied; addresses are citations only.

Read first (not repeated here): `alpha-ui-screens.md` §8 (window table, crafting host
G+0x8008c0 350 × 285, InventoryWidget type 2, BlueprintPreviewWidget 350 × 330, tab titles,
sounds 0x59/0x5a) and §8.1 (slots, bags, the add rule 0x46ebe0, the "can carry" test, wide
rows, mouse rules, learning a formula); `alpha-interaction.md` §4.2 (static kinds) and §5.3
(stations 65, 71–77 toggle the crafting window and pick tabs 3 / 5);
`alpha-items-generator.md` (the recipe/formula generator, the weapon and armour
generators); `alpha-item-names.md` (names of every item below).

**Status.** The recipe table, the level grants, the station rule, the list building and
ordering, the preview layout, the craft timer, ingredient removal and the product are read
to the instruction. *Believed* marks readings that are not fully proven.

---

## 0. Conventions

* "G" is the game object, "P" the local player's creature (G+0x8006d0). Positions are i64
  in 1/65536 block (`alpha-interaction.md` §0).
* An **item** is the 280-byte record (type +0x00, subtype +0x01, seed +0x04, product +0x08,
  rarity +0x0c, material +0x0d, flags +0x0e, level/quantity i16 +0x10, spirits, spirit
  count +0x114). Written here as **(type, subtype, material)**, with rarity 0, seed 0,
  product 0, flags 0, level field 1 and no spirits unless stated.
* **Equal items**: the full comparison 0x42f4a0 (type, subtype, seed, flags, product,
  rarity, level field, material, spirit count and spirits). Ingredient matching uses it, so
  an ingredient in the bags must match **exactly**, including rarity and the level field.
  (Ingredients are type 11, a quantity type, so the add rule always stores them with level
  field 1, which is what the recipes ask for.)
* *L* = P's level (+0x190). Classes 1 warrior, 2 ranger, 3 mage, 4 rogue (P+0x140).
* Ingredient names used below (from `alpha-item-names.md`): (11, 0, m) "*m* Nugget",
  (11, 1, 2) Wood Log, (11, 5, 21) Plant Fiber, (11, 6, 0) Cobweb, (11, 9, m) "*m* Yarn"
  (25 Silk, 26 Linen, 27 Cotton), (11, 10, m) "*m* Cube" (1 Iron, 2 Wood, 11 Gold,
  12 Silver), (11, 11, 27) Cotton Capsule, (11, 12, 24) Glass Flask, (11, 15) Mushroom,
  (11, 16) Pumpkin, (11, 17) Pineapple, (11, 20) Ginseng Root, (11, 21) Onion Slice,
  (11, 22) Heartflower, (11, 23) Prickly Pear, (11, 24) Iceflower, (11, 26) Water Flask,
  (11, 27) Snowberry. The game's own word for the "Cube" of a metal or wood is just "Cube";
  "bar"/"plank" are not used.

---

## 1. Data

### 1.1 Known recipes (save field 18)

P+0x1d28 points to a small object whose +0x14 is a doubly linked **list of items**, the
**recipe keys**; +0x18 is its length. A key is simply the item the recipe makes. A key is
"known" (0x444a90) when an equal item (0x42f4a0) is in the list. The list is saved as field
18 (`alpha-saves.md`). Keys are added only by:

* the **level grants** (§2.2) — at character creation (0x4772b0 empties the list, then
  grants level 1) and on every level change (§2.2);
* **learning a formula** (right-click a type-2 item, `alpha-ui-screens.md` §8.1.8): the key
  is a copy of the formula with **type := its product byte (+0x08)** and **product := 0**;
  subtype, seed, rarity, material, level and spirits are kept (the same transform as the
  formula's name, `alpha-item-names.md` §4). Already known → sound 0x31, nothing else.
  Otherwise the key is appended, one formula is taken from the stack, sound 0x2f at the
  listener.

Nothing ever removes a key except character creation.

### 1.2 Recipe record (0x128 bytes)

Built from a key by the **recipe builder** 0x59cff0 (it never fails):

| Off | Size | Content |
|---|---|---|
| +0x000 | 0x118 | **product** = a copy of the key |
| +0x118 | i32 | **station** id (§3; 0 = none) |
| +0x11c | 12 | **ingredients**: a vector of 0x11c-byte entries, each an item (0x118) followed by an i32 **count needed** |

The same record lives in three places: the per-tab recipe lists (built by 0x4a14c0), the
preview widget (+0x16c … +0x293) and the "recipe being crafted" G+0x1000e7c … (§5.4).

---

## 2. The recipe list

### 2.1 Ingredients and station of every key (0x59cff0)

The builder looks at the key's type and subtype (and material). Then, **for every key**, a
**rarity surcharge** is appended last: key rarity *r* = 1, 2, 3 or 4 adds **2 × (11, 0,
material 12 + *r*) with rarity *r*** — 2 Emerald (r 1), Sapphire (2), Ruby (3) or Diamond
(4) Nuggets whose own rarity byte is *r*. Rarity 0 adds nothing.

**Consumables (type 1).** Category: subtypes 1 and 2 → **Alchemy** (tab 4), every other
subtype → **Cooking** (tab 3).

| Key | Product | Ingredients | Station |
|---|---|---|---|
| (1, 1) | Life Potion | 1 Heartflower (11, 22, 0) + 1 Water Flask (11, 26, 0) | none |
| (1, 2) | Cactus Potion | 1 Prickly Pear (11, 23, 0) + 1 Glass Flask (11, 12, 24) | none |
| (1, 4) | Ginseng Soup | 1 Ginseng Root (11, 20, 0) | campfire |
| (1, 5) | Snowberry Mash | 1 Snowberry (11, 27, 0) | none |
| (1, 6) | Mushroom Spit | 1 Mushroom (11, 15, 0) + 1 Onion Slice (11, 21, 0) | campfire |
| (1, 8) | Pineapple Slice | 1 Pineapple (11, 17, 0) | none |
| (1, 9) | Pumpkin Muffin | 1 Pumpkin (11, 16, 0) | campfire |
| (1, 0), (1, 3), (1, 7) | Cookie, Mana Potion, Bomb | none | none |

**Ingredients made from ingredients (type 11).** Category: **tab 5** (the tab titled
"Formulas", icon `inventory-ingredients.png`, §4.1).

| Key | Product | Ingredients | Station |
|---|---|---|---|
| (11, 9, 25) | Silk Yarn | 1 Cobweb (11, 6, 0) | spinning wheel |
| (11, 9, 26) | Linen Yarn | 1 Plant Fiber (11, 5, 21) | spinning wheel |
| (11, 9, 27) | Cotton Yarn | 1 Cotton Capsule (11, 11, 27) | spinning wheel |
| (11, 9, other material) | — | none | none |
| (11, 10, 2) | Wood Cube | 1 Wood Log (11, 1, 2) | sawbench ("Saw") |
| (11, 10, *m* ≠ 2) | *m* Cube | 1 *m* Nugget (11, 0, *m*) | furnace |
| (11, 22) | Heartflower | 1 Iceflower (11, 24, 0) | campfire |
| (11, 26) | Water Flask | 1 Glass Flask (11, 12, 24) | water (§3.2) |
| any other (11, *s*) | — | none | none |

**Weapons (type 3)**, category **Weapons** (tab 0). "Iron" = Iron Cube (11, 10, 1), "Wood" =
Wood Cube (11, 10, 2).

| Subtypes | Ingredients | Station |
|---|---|---|
| 0, 1, 2 (sword, axe, mace), 3 (dagger), 13 (shield) | 8 Iron | anvil |
| 4 (fist) | 6 Iron + 2 Cotton Yarn (11, 9, 27) | anvil |
| 5 (longsword), 15, 16, 17 (great weapons) | 16 Iron | anvil |
| 6 (bow) | 15 Wood + 1 Linen Yarn (11, 9, 26) | workbench |
| 7 (crossbow) | 6 Iron + 9 Wood + 1 Linen Yarn | workbench |
| 8 (boomerang), 10 (staff), 11 (wand) | 16 Wood | workbench |
| 12 (bracelet) | 8 Cubes of the bracelet's own material (11, 10, key material) | anvil |
| 9, 14 and ≥ 18 | none | none |

**Armour (types 4–7)**, category **Armor** (tab 1). Count by type: chest (4) **20**, gloves
(5) **6**, boots (6) **8**, shoulders (7) **8**.

* key material 1 (iron) → that many Iron Cubes, **anvil**;
* any other material *m* → that many **Yarn of the same material** (11, 9, *m*) — Silk
  (25), Linen (26), Cotton (27) — **loom**.

**Amulets and rings**, category **Amulets** (tab 2): amulet (8) → 6 Cubes of its material
(11, 10, *m*); ring (9) → 3 Cubes of its material; **no station**.

Types 2, 10 and every other type have no recipe content and are **not listed** in any tab
(0x4a14c0 skips them).

*(Quirk, harmless.)* The builder's working ingredient keeps its material between pushes,
which is why every row above states its material explicitly; the only places where it
matters are already resolved in the tables.

### 2.2 Level grants (0x47fae0, called with the new level)

Called with *L* = 1 at character creation (0x4777b6) and from the frame update whenever
P's level differs from the last level seen (P+0x1d3c; 0x493d78 … 0x493ec4: the level-up
text and sound 0x1d are shown, the last level := *L*, then the grant). Only the **new**
level is granted; a jump of several levels skips the levels in between. Each grant is
appended only if not already known. The key's level field is *L* unless stated; rarity 0.

| Condition | Keys granted |
|---|---|
| *L* mod 4 = 1 | (1, 1) Life Potion, level *L* |
| *L* mod 4 = 3 | (1, 2) Cactus Potion, level *L* |
| *L* mod 5 = 1 | (1, 4) Ginseng Soup, then (1, 8) Pineapple Slice, level *L* |
| *L* mod 5 = 2 | (1, 9) Pumpkin Muffin, level *L* |
| *L* mod 5 = 3 | (1, 6) Mushroom Spit, level *L* |
| *L* mod 5 = 4 | (1, 5) Snowberry Mash, level *L* |
| *L* = 1 | (11, 26) Water Flask; (11, 22) Heartflower; rogue: (11, 9, 27) Cotton Yarn; ranger: (11, 9, 26) Linen Yarn; mage: (11, 9, 25) Silk Yarn (attempted twice, the second is a no-op); then (11, 10, 1) Iron, (11, 10, 2) Wood, (11, 10, 11) Gold, (11, 10, 12) Silver Cube — all level 1. Warriors get no yarn recipe. |
| **every** *L* | one **armour** key and one **weapon** key (below) |

**Armour key**: type by *L* mod 4: 0 → shoulders (7), 1 → chest (4), 2 → boots (6),
3 → gloves (5); subtype 0; **seed = rand() mod 11** (first draw); material by class:
warrior 1, ranger 26, mage 25, rogue 27 (any other class leaves the material at 12 from the
level-1 block, or 0 — *believed* unreachable); level *L*.

**Weapon key**: type 3, **seed = rand() mod 11** (next draw), level *L*, by class and
*L* mod 3:

| Class | *L* mod 3 = 0 | = 1 | = 2 |
|---|---|---|---|
| warrior (material 1) | great weapon: subtype 15 + rand() mod 3 | sword/axe/mace: subtype rand() mod 3 | shield (13) |
| ranger (material 2) | bow (6) | crossbow (7) | boomerang (8) |
| mage | staff (10), material 2 | wand (11), material 2 | bracelet (12), material 12 if rand() is odd, 11 if even |
| rogue (material 1) | dagger (3) | longsword (5) | fist (4) |

Draws per grant: 2, plus 1 for a warrior with *L* mod 3 ∈ {0, 1} or a mage with *L* mod 3 =
2. They come from the client's C runtime stream (no seed of their own), so two players of
the same class get different seeds. Because the key includes the seed, a new pair of
equipment recipes is (almost always) added at every level.

### 2.3 Formula-taught equipment

A formula comes from the recipe generator (`alpha-items-generator.md` §3): product type
3–9, subtype/material/seed from the wrapped weapon or armour, level *L*g and rarity *R* of
the generator call. Learning it (§1.1) gives the key (product type, subtype, seed 0–99,
material, rarity *R*, level *L*g). Its recipe is §2.1 for that type/subtype/material plus
the rarity surcharge for *R*. The product made is exactly the key: **its level is the
formula's level, not the player's**, and its rarity is the formula's.

### 2.4 Building the six tab lists (0x4a14c0, 0x4a19d0)

Run when the crafting window is toggled (C, 0x488bd0, or a station, 0x4975b6), after a
craft completes, on some window presses (0x47dd88), and when a world-update section-9 item
arrives for P (0x48f8d0).

1. For every key in the known list (list order), build its recipe (§2.1) and put it in the
   category of its type: 3 → 0 Weapons; 4–7 → 1 Armor; 8, 9 → 2 Amulets; 1 with subtype 1
   or 2 → 4 Alchemy; other 1 → 3 Cooking; 11 → 5; anything else → dropped.
2. **Sort** each category in **descending** order of **6 · level + rarity** (std::sort with
   that key; for ≤ 32 recipes this is an insertion sort, which keeps list order among
   equal keys; larger lists may reorder ties).
3. Crafting bag *t* (G+0x800adc, one bag per tab) is cleared and refilled, one slot per
   recipe: **count** = the number of times it can be made = min over its ingredients of
   ⌊have / need⌋, where *have* = the sum of the counts of all bag slots (all bags, not the
   item in hand) holding an equal item; a recipe without ingredients gets count **0**; the
   slot's item = the product.
4. If this tab and row are the crafting widget's selected slot (+0x18c, +0x190), the
   preview's recipe := this recipe (product, station, ingredients).

Counts are therefore never negative: the "negative count drawn black" feature of the
InventoryWidget (`alpha-ui-screens.md` §8.1.1) is never used by crafting, and the hover
test 0x47b340 (tooltip source) would skip such rows anyway.

---

## 3. Stations

### 3.1 Station ids

| Id | Needs | Static kind | Text (when missing) |
|---|---|---|---|
| 0, 2 | nothing | — | — |
| 1 | water | — | "Requires water" |
| 3 | furnace | 71 | "Requires Furnace" |
| 4 | anvil | 72 | "Requires Anvil" |
| 5 | spinning wheel | 73 | "Requires Spinning Wheel" |
| 6 | loom | 74 | "Requires Loom" |
| 7 | sawbench | 75 | "Requires Saw" |
| 8 | workbench | 76 | "Requires Workbench" |
| 9 | campfire | 65 | "Requires Campfire" |

The customization bench (77) is not a crafting station. Id 2 is never produced.

### 3.2 "Near" (decided every frame by the preview draw, 0x431afe … 0x43460c)

* **Static stations (3–9).** P's block x and y (position ÷ 65536, truncated toward zero),
  then the 8-block **cell** (÷ 8, truncated toward zero). Every static listed in the 3 × 3
  cells (cell x − 1 … + 1, cell y − 1 … + 1) of the zone cell index
  (`alpha-interaction.md` §4.3, lookup 0x42f640) is tested: if its kind is the station's
  kind and the **3-D squared distance** in blocks between its position (record +0x08,
  +0x10, +0x18) and P's position is **< 16**, the station is present (search stops). So:
  **within 4 blocks**, any direction, of the static's anchor point. The static's state
  (open/closed) is not checked.
* **Water (1).** Present when P's contact flags (+0x5c) have bit value 2 set (*believed*
  "in water"), or the block at P's position lowered by half P's height (z − size.z/2,
  P+0x88) has block type 3 (water; low 5 bits of the block's 4th byte).
* **Station 0/2**: always present. So amulets, rings, potions, Snowberry Mash and
  Pineapple Slice can be made anywhere, with the window opened by **C**.

When missing, the text is drawn (§4.3) and the craft button is disabled (§5.1).

---

## 4. The crafting window

### 4.1 The list window (InventoryWidget type 2, host G+0x8008c0, 350 × 285)

* **Opening**: C or a crafting station toggles the host (and refreshes, §2.4); stations
  65 → tab 3, 71/73/75 → tab 5 (`alpha-interaction.md` §5.3). X (skills) closes it; a
  right press over no widget closes it (`alpha-ui-screens.md` §8.1.2). The preview host
  (G+0x800ad4) is made visible exactly when the crafting host is, every frame (frame
  update 0x488ee0, which also refreshes it while shown).
* **Tabs** (built once at start-up, 0x4a1e50): six clones of `tab`, in order, with these
  icons:

  | Tab | Title | Icon (by P's class at start-up) |
  |---|---|---|
  | 0 | "Weapons" | warrior `craft-melee-weapons.png`, ranger `craft-ranged-weapons.png`, mage `craft-magic-weapons.png`, rogue `craft-rogue-weapons.png` |
  | 1 | "Armor" | warrior `craft-heavy-armor.png`, ranger `craft-medium-armor.png`, mage `craft-light-armor.png`, rogue `craft-rogue-armor.png` |
  | 2 | "Amulets" | `craft-amulets.png` |
  | 3 | "Cooking" | `craft-cooking.png` |
  | 4 | "Alchemy" | `craft-alchemy.png` |
  | 5 | "Formulas" | `inventory-ingredients.png` |

  For a class other than 1–4 the first two tabs are not made (open point C-1). Tab
  placement, colours and sound 0x56 are the InventoryWidget rule (§8.1.3 there).
* **Rows**: wide rows (`wideitembox`), one per recipe in sorted order; name + " +" + power
  in the rarity colour (§8.1.3 there). For type 2 only, a row whose count > 0 also shows the
  count: outlined text, 10 px, stroke 2, white, **right-aligned at (slot x + *s*w − 4,
  slot y + 37)** (widget space). Count 0 rows are drawn normally with no number.
* **Selection**: any mouse press while the crafting host is visible (0x47b600, 0x4c6610):
  if a row is hovered it becomes the selection (tab, index), sound **0x55**, and the
  `itemselector` moves to it. Then the preview recipe is rebuilt from the selected row's
  product with the builder (§2.1); with no valid selection the preview gets an empty
  recipe.
* Hovering a row shows the product's tooltip (`alpha-ui-screens.md` §8.1.7).

### 4.2 The preview window (BlueprintPreviewWidget, constructor 0x42f190, draw 0x42f910)

Panel 350 × 330 (`blackwidget` clone), left of the inventory under the crafting window
(top at H/2 + 10). Fields: game +0x294, `itemshadow` +0x298 (not used by the draw,
*believed*), `craftbutton` +0x29c and `craftbar` +0x2a0 (both made children of the panel;
the bar's child `bar` at +0x2a4), recipe +0x16c, model turn angles +0x160/+0x164/+0x168
(start −120°, 0°, 0°), hovered ingredient +0x2a8, hovered item +0x2ac, "button ready and
hovered" +0x3c4. Positions of the button and bar come from their `gui.plx` templates.

Coordinates below are **panel-local** (w = 350, h = 330) for text and screen-space for
models (panel top-left + local).

**Every frame:**

1. **Model.** Recipe known → the product's model; else (empty selection) model slot 2396
   and the word "Unknown". No model → nothing at all is drawn this frame.
2. The model turns: +0x168 += frame ms × 0.02 (20°/s about z). Scale = 0.06 / (largest
   model dimension), × **1.5** while the mouse is in the box x ∈ (w/2 − 60, w/2 + 60),
   y ∈ (50, 170) — the product then becomes the hovered item (tooltip). Centre on screen:
   (left + w/2, top + 110). Projection, view shift and centring as the item-model routine
   (`alpha-ui-screens.md` §8.1.6: π/4, aspect −W/H, near 0.1, far 1000, 1 unit ahead);
   lights: front (1, 1, 1), back (0.2, 0.2, 0.4), ambient (0.4, 0.4, 0.4), direction
   (0.7, 0.5, −1)/√2; material colour (1, 1, 1, 0.9).
3. **Name** (known): item name (+ " +" + power for non-quantity types, the §8.1.3 rule),
   outlined, 12 px, stroke 3, **centred at (w/2, 30)**, wrapped to the widget width, in
   the name colour *(believed: the rarity colour of §8.1.6 there)*. Unknown: "Unknown",
   15 px, stroke 3, white, at (15, 65).
4. **Ingredients** (known only), entry *k* of the list, *C* = trunc((w − 10)/45) columns
   (**7** at w = 350):
   * box: top-left (10 + 45·(*k* mod *C*), h − 120 + 45·⌊*k*/*C*⌋), 40 × 40; model at
     its centre (+20, +20), scale **0.025**, **0.0375** while the mouse is in the box (that
     ingredient becomes the hovered item, index in +0x2a8);
   * "have/need": *have* = the §2.4 sum for that ingredient, *need* = its count; text
     *have* + "/" + *need*, 10 px, stroke 2, **right-aligned at (50 + 45·col,
     h − 75 + 45·row)** (the box's right edge, 5 px below it); fill **(0, 1, 0)** when
     *have* ≥ *need*, **white** when short. Any short ingredient marks the recipe
     "missing".
5. **Station** (§3): if missing, the "Requires …" text: outlined, 9 px, stroke 3, fill
   **(1, 0.25, 0.25, 1)**, left-aligned at **(15, h − 40)**, wrapped to w − 50.
6. **Craft button frame** (`craftbutton`'s `frame`): white (disabled) when a station or an
   ingredient is missing or the recipe is unknown; otherwise **(0, 1, 1)** when the mouse is
   over the button (+0x3c4 := 1), else **(0, 1, 0)** (+0x3c4 := 0).
7. **Craft bar**: visible exactly while a craft is running (timer ≥ 0, §5); its `bar`
   child is scaled so its width is (bar width − 8) · timer (0x434c20).

### 4.3 Worked layout (1280 × 720)

Crafting window (80, 75) … (430, 360); preview panel (80, 370) … (430, 700). Product
model centre (255, 480), hover box x 195 … 315, y 420 … 540. Name baseline centred at
(255, 400). Ingredient 0 box (90, 580) … (130, 620), ingredient 1 (135, 580); 8th
ingredient (row 1) at (90, 625). Count of ingredient 0 right edge 130, baseline 625.
"Requires Anvil" baseline (95, 660).

---

## 5. Crafting (frame update 0x48f183 … 0x48f23d; completion 0x4709c0)

### 5.1 Start

G+0x1000e78 is the **craft timer** (−1 = idle). Only while the preview is visible: if
+0x3c4 is set (ready and hovered last frame), the **left mouse button is held** (G+0x04)
and the timer is < 0 → timer := 0 and the preview's recipe is **copied** to
G+0x1000e7c (product, station, ingredients). Nothing is sent, no player action or
animation is started, no sound plays at the start.

### 5.2 Progress

While the preview is visible and timer ≥ 0: timer += frame ms × 0.001. When it reaches
**1** (one second) the timer becomes −1 and the craft completes (§5.3). The bar shows the
timer (§4.2 step 7). If the window is closed the timer **freezes** and resumes when it is
reopened. Holding the button keeps crafting: the next craft starts on the first frame after
completion if the recipe is still ready.

### 5.3 Completion (0x4709c0)

Works on the copied recipe; it does **not** re-check the station or the ingredients.

1. Copy the product. If P **cannot carry** it (`alpha-ui-screens.md` §8.1.1, types 1, 11,
   20: 50 or more equal items already) → chat "You can't carry more of these items." in
   (1, 0.2, 0.2, 1), sound 0x31 at P; nothing is consumed; stop.
2. **Remove ingredients.** For each ingredient (count *n*), walk all bags in order and
   their slots in order; at each slot holding an equal item:
   * if *n* < slot count: slot count −= *n*, and go on with the **next bag** (the
     remaining need is not cleared);
   * else: the slot is emptied (count 0, item cleared) and *n* is reduced by the slot's
     count **after** it was set to 0 — i.e. not at all — and the walk continues.

   With the usual single stack per ingredient this removes exactly *n* (or empties a stack
   of exactly *n*). With several stacks the original over-removes (every stack ≤ *n* is
   emptied, the first larger one loses *n*); see note N-1.
3. If the product type is 3–9 (equipment): flags +0x0e **|= 1** — crafted equipment shows
   " (adapted)" and cannot be adapted (§6.2).
4. Sound **0x59** (`craft.wav`) at the listener.
5. If the product is type 11 and its subtype is not 26 (Water Flask): with chance 1/5 a
   **bonus**: quantity (+0x10) += 1, or += 2 if a second rand() mod 5 = 0, or += 3 if a
   third is also 0 (P = 4/25, 4/125, 1/125 for +1/+2/+3); then sound **0x5a**
   (`craft-proc.wav`). Draws: 1, 2 or 3 rand() calls only for these products.
6. Add the product with the inventory add rule (0x46ebe0, auto bag; type 11 adds
   quantity +0x10 and stores level 1), refresh the inventory scroll range, chat "You
   receive …" (0x480fb0, `alpha-ui-screens.md` §8.1.9), rebuild the recipe lists (§2.4).

**The product's level** is the key's level (§2.2: the level at which the grant happened,
or the formula's level); the player's current level plays no part.

### 5.4 Network

Crafting, learning formulas, level grants, identification and adaption are entirely
**client-side**: they change only P's bags, money, platinum and known list, which are saved
locally (`alpha-saves.md`). No packet is sent for them (no packet-6 record is queued; the
only network-related call is the refresh on an incoming section-9 item). *Believed*: the
server never learns crafted items unless they are dropped (packet 6) or equipped (creature
update).

---

## 6. The item-service windows (briefly)

Both are built at start-up next to the inventory (right side, `alpha-ui-screens.md` §8.1.2)
and fed by right/middle-pressing a bag slot or an equipment box while open (§8.1.8 there).
Correction to `alpha-interaction.md` §5.2 b: NPC class **131** opens the **Identification**
window (G+0x8008f8, mode := 1, source := none) and class **137** opens the **Adaption**
window (G+0x800900); neither is a trainer or pet window.

### 6.1 EnchantWidget (0x44e910; draw 0x44ea30) — "Identification"

* Title "Identification" (15 px at (15, 25)); buttons "Identify" and "Goodbye!" (12 px,
  bottom row); the source item's tooltip block (0x4a28c0) and model.
* Source: a bag slot (+0x160, +0x164) whose item is **type 14** (shown "Leftovers" by the
  name table; here it is the unidentified item). Other types are ignored.
* Cost (0x450920): max(1, ⌊price(item)/2⌋) in copper (price §8.1.1 there), shown in
  G/S/C right-aligned; greyed (0.7, 0.7, 0.7) when no valid source.
* **Identify** (mode 1, money ≥ cost, button hovered; handler in 0x47b600): money −= cost, one item
  taken from the stack, then rand(): even → weapon generator, odd → armour generator
  (`alpha-items-generator.md` §2) with (level, rarity) = the source's level and rarity and
  class −1; the result is added (add rule), sound 0x2e at P, chat "You receive …".
* Mode 0 (*disenchant*: one item removed, 5 + rand() mod 6 items of type 10 with the
  source's material pushed to the incoming-item queue, sound 0x2e) exists in the handler,
  but nothing ever sets mode 0 — *believed* unreachable in Alpha.
* "Goodbye!" closes the window.

### 6.2 AdaptionWidget (0x40ecd0; draw 0x40f8f0) — "Adaption"

* Title "Adaption", button "Adapt". Source: a bag slot (+0x164, +0x168) or an equipment
  slot pointer (+0x160); accepted only for types 3–9 **not already adapted** (flags bit 0
  clear, 0x40f570).
* **Cost** (0x40f4f0) in **platinum coins**: with *l* = item level, *r* = rarity,
  *n* = *L* − *l*: if *n* ≤ 0 the cost is 0; else *q* = 2^((*l* + *r* − 1)·0.25) and the
  cost starts at 0 and, *n* times, becomes trunc(cost + 2*q*).
* **Adapt** (window open, button pressed, valid source, platinum ≥ cost): platinum −=
  cost; item level := *L*; flags |= 1; sound 0x2e at P. The stats then follow the new level
  (`alpha-creatures-behaviour.md` §10). Example: item level 5, rarity 1, player level 8:
  *q* = 2^1.25 = 2.3784, 2*q* = 4.7568 → 4, 8, 13 → **13 platinum**.

(The customization bench, static 77, opens a different window — G+0x8008dc with the spirit
editor G+0x8008f4 — which edits an item's spirit cubes; it is not part of crafting.)

---

## 7. Worked examples

### 7.1 Cooking a Pumpkin Muffin at a campfire

A ranger reached level 2: the grant (*L* mod 5 = 2) added key (1, 9) level 2; the same grant
added boots (2 mod 4 = 2 → type 6) material 26 and a boomerang (2 mod 3 = 2).

1. Bags: tab 2 holds 3 Pumpkins (11, 16, 0), count 3. The player stands at block
   (1000.5, 2000.5, 30.0); a campfire static (kind 65) sits at (1002.0, 2001.0, 30.0).
2. R on the campfire toggles the crafting window, selects tab 3 "Cooking", rebuilds the
   lists. The Cooking list holds (1, 9) level 2 → key 6·2 + 0 = 12 (it is the only entry).
   Its row count = ⌊3/1⌋ = 3, drawn "3" at the row's right.
3. Clicking the row selects it (sound 0x55); the preview shows the muffin model, the name
   "Pumpkin Muffin" (type 1 is not a quantity type, so " +5": *p* = trunc(g(2)·100 + 1) =
   trunc(4.76 + 1) = 5), one ingredient box with "3/1" in green.
4. Station 9: P's cell (125, 250); the campfire is in cell (125, 250); squared distance
   1.5² + 0.5² + 0 = 2.5 < 16 → present, no "Requires Campfire"; the button frame is green,
   cyan under the mouse.
5. Holding the left button over it: timer 0 → 1 in 1 s, bar fills. Completion: can carry
   (no muffins yet); the Pumpkin stack 3 > 1 → 2; type 1 → no adapted flag, no bonus; sound
   0x59; the muffin (1, 9, level 2) goes to bag 1 ("Items"); chat "You receive Pumpkin
   Muffin +5."; lists rebuilt → row count 2. Still holding → a second muffin a second later.
   Its heal is 100·*F*(2, 0) (`alpha-ui-screens.md` §8.1.7).

### 7.2 Forging an iron sword from a formula at an anvil

1. A warrior right-clicks "Formula: … Iron Sword": type 2, product 3, subtype 0, material
   1, seed 37, rarity 2, level 6. Not known → key (3, 0, seed 37, material 1, rarity 2,
   level 6, product 0) appended, one formula removed, sound 0x2f.
2. Recipe (§2.1): **8 Iron Cubes** (11, 10, 1) + rarity surcharge **2 Sapphire Nuggets**
   (11, 0, material 14, **rarity 2**); station 4 (anvil). Category Weapons; sort key
   6·6 + 2 = 38 (above the level-1 to level-6 grant weapons with keys 6 … 36).
3. Iron Cubes come from the furnace recipe (11, 10, 1): 1 Iron Nugget each, 1 s each, with
   the 1/5 bonus (e.g. 10 nuggets typically give ~12 cubes).
4. Bags: 12 Iron Cubes, 1 Sapphire Nugget of rarity 2. Preview: "12/8" green, "1/2" white →
   missing; button white. Row count = min(⌊12/8⌋, ⌊1/2⌋) = 0 (no number). The player is
   3 blocks from an anvil: no "Requires Anvil", but the button stays disabled.
5. With a second matching Sapphire Nugget (merged into the same stack, 2): "2/2" green,
   row count 1, button green. Craft: Iron 12 → 4; Sapphires 2 = 2 → stack emptied; the
   sword (3, 0, seed 37, material 1, rarity 2, level 6) gets flags bit 0; sound 0x59; added
   to bag 0; chat "You receive <name> +21." (*p* = trunc(g(6)·100 + 1) = trunc(20 + 1) =
   21; the name with its rarity-2/seed-37 adjective is `alpha-item-names.md` §3). The
   tooltip line 2 reads "Power 21 (adapted)".

### 7.3 Grants of a new level-1 mage

*L* = 1: Life Potion (1, 1); Ginseng Soup (1, 4); Pineapple Slice (1, 8); Water Flask;
Heartflower; Silk Yarn; Iron, Wood, Gold and Silver Cubes; chest (4, 0, material 25, seed
rand() mod 11, level 1); wand (3, 11, material 2, seed rand() mod 11, level 1). Two draws.

---

## 8. Notes for AlphaVulkan

* **N-1** The ingredient-removal quirk (§5.3 step 2) and the missing re-check at completion
  are faithful readings; an implementation that wants "vanilla" results must keep them only
  where they matter (they matter only with several stacks of one ingredient, which the add
  rule normally prevents, or when ingredients are moved away during the 1-second timer).
  Recommend: reproduce by default, behind a "strict vanilla" switch.
* **N-2** Keep crafting client-side as Alpha does; a server-authoritative mode would be a
  new feature (multiplayer anti-cheat), not parity.
* **N-3** All tunables (counts, stations, 4-block range, 1-s time, bonus odds) are plain
  data; put them in a recipe table so mods can add recipes (modding-friendly rule).

## 9. Open points

* **C-1** Tab icons are chosen once at start-up from P's class (0x4a1e50, only caller
  0x4644f7). If P has no class then (before a character is loaded), only four tabs are
  built and the icons shift; check in the running game whether the crafting tabs show the
  class icons and six tabs.
* **C-2** The rarity-surcharge gems (11, 0, material 13–16) must carry **rarity 1–4** to
  match; confirm that such gems drop in Alpha (otherwise rarity ≥ 1 formulas are
  uncraftable).
* **C-3** The name colour in the preview (step 3) and the exact geometry of `craftbutton` /
  `craftbar` (from `gui.plx`).
* **C-4** Type 14 as "unidentified item" (Identification window) versus the name table's
  "Leftovers"; and model slot 2396 (the preview's "Unknown" model).
* **C-5** The P+0x5c bit (value 2) used by "Requires water".
