# Alpha skills: the skill window and skill points

Behavioural specification of the **skill window** of Cube World **Alpha** (`Cube.exe`) and of
**skill points**: where the window sits, the 11-skill tree and its buttons, the two
specialization buttons, how points are counted and spent, what re-skilling and changing
specialization cost, the texts and tooltips, and what is networked and saved. This is
follow-up **F-6** of `alpha-ui-screens.md` §12.

Written from the Alpha decompile (`disassembled alpha cube world/decompiled/client`), fresh
disassembly of `binary/Cube.exe` where the export lost return values or control flow (the
mouse handler 0x47b600 and the per-frame routine 0x488ee0), the `gui.plx` templates and the
`data4.db` dictionary `dict_en.xml` (keys only; its texts are game content and are read from
the player's install, never copied). Nothing here is copied code; routines are cited by
address in `Cube.exe` as references only. *Believed* marks readings that are not fully
proven.

Already specified elsewhere and only referenced here: `alpha-ui-screens.md` §2 (layout
model, pivot rule), §3.4 (text origin and alignment flags), §5.5 (ability bar, ability icon
table, skill icon table, specialization icon table), §7 (menu-button row, `skills.png`
button), §8 (window table row), §8.1.2 (`blackwidget` panels), §8.1.7 (the `preview` tooltip
box), §8.1.9 (mana cubes, "You gain one skill point."), §10 (interface sounds);
`alpha-creatures-behaviour.md` §10.2 (level factor *F*), §11 (skill factor *k*), §16 (which
ability uses which skill, ability value *v*, cooldown lengths, mana cost);
`alpha-talk-vendors.md` §2 (dictionary markup grammar, page rendering and style colours);
`alpha-saves.md` §3.2 (save fields 8, 16, 22, 23); `alpha-net-packets.md` §2.1 (creature
record fields 22, 46, 47).

**Status.** Window host, size, placement, opening and closing; the tree (order, templates,
positions, links, icons, locks); specialization choice; point budget; click rules; Learn /
Cancel; costs, the class-trainer test and the money check; all texts, sizes and colours; the
tooltip text and its variables; network and saves are specified. Open points are in §9.

---

## 0. Conventions

* *G* is the game object; *P* the local player's creature object (G+0x8006d0). Creature
  offsets are **object** offsets (record offset + 0x10) as in `alpha-ui-screens.md` §8.1.1:
  +0x140 class (1 warrior, 2 ranger, 3 mage, 4 rogue), +0x141 specialization (0 or 1),
  +0x190 level *L*, **+0x1138 + 4·*i*** the level of skill *i* (*i* = 0 … 10, i32),
  **+0x1164 the mana-cube counter *c*** (i32), +0x1304 money in copper.
* *W*, *H*: client size in pixels. All interface positions are window pixels at 1:1.
* **The window widget** is the SkillWidget object (constructor 0x4dd750, kept at
  G+0x80090c). Its fields:

| Offset | Content |
|---|---|
| +0x160 + 4·*i* (*i* = 0 … 10) | the **draft** level of skill *i* (what the window shows and edits) |
| +0x18c | the **draft** specialization (i32, 0 or 1) |
| +0x190 | back pointer to *G* |

  The draft is a working copy. Nothing changes on the player until **Learn** (§4.4).
* *F*(*L*) = *F*(*L*, 0) of `alpha-creatures-behaviour.md` §10.2 (0x445f60 with (*L*, 0)):
  2^(3·(1 − 1/((*L* − 1)·0.05 + 1))). Values: *F*(1) = 1, *F*(6) = 1.5157166,
  *F*(10) = 1.9066427.
* Text drawn in code uses `resource1.dat` in two passes (0x639b30): an outline pass (white
  fill, black outline of the stated stroke) then a fill pass in the stated colour with no
  outline. "*s* px, stroke *k*" gives the size and the outline width. Alignment flags as in
  `alpha-ui-screens.md` §3.4 (0 left, 1 centred, 2 right-aligned); the origin is the first
  baseline.
* Sounds go through 0x484350 at the listener (G+0x140), volume 1, pitch 1: **0x55**
  `menu-select.wav`, **0x58** `menu-drop-item.wav`, **0x53** `menu-open2.wav`, **0x54**
  `menu-close2.wav`.

---

## 1. Window host, size, placement, opening and closing

### 1.1 Host and size

Built once at start-up (0x459c40, 0x464dde … 0x464ee2): a clone of the `blackwidget`
template (look as `alpha-ui-screens.md` §3.3: 5-px notched corners, black fill alpha 0.67,
3-px black outer stroke, 2-px white inner outline) made under the in-game root (G+0x800884),
kept at **G+0x800908**, resized with the 9-slice rule to **300 × 460**. An empty container is
made inside it and the SkillWidget is built on that container and bound to the panel ("fit",
0x631460), so the widget rectangle is (0, 0) … (300, 460) in panel space (*w* = 300,
*h* = 460). The SkillWidget's draw routine is **0x4dd810** (its only own virtual method; the
others are the library defaults).

The skill and specialization **buttons are not children of the panel**: they are separate
clones made under the in-game root and moved every frame (§2.3).

### 1.2 Placement

Layout routine 0x482a40 (0x483a9e … 0x483b56), redone on every resize: the panel's top-left
corner is at

* **x = W/2 − *w*ᵢ/2 − 300 − 10 = W/2 − 510** (left of the centred inventory, *w*ᵢ = 400, with
  a 10-px gap),
* **y = H/2 − *h*ᵢ = H/2 − 285** (top aligned with the inventory, *h*ᵢ = 285; the integer
  halves are truncated).

This is the same spot as the crafting panel (G+0x8008c0, 350 wide), which is why the two
windows exclude each other (§1.3). At 1280 × 720 the panel spans **(130, 75) … (430, 535)**.

### 1.3 Opening and closing

The window is open while the flag **G+0x8008f1** is set. The panel's visibility is set equal
to the flag every frame (0x48d9ff).

| Event | Effect |
|---|---|
| **X** key, or the `skills.png` menu button (`alpha-ui-screens.md` §7) → 0x488c70 | flag := not flag. If it is now set: copy the player's 11 skill levels (+0x1138 …) into the draft (+0x160 …) and the player's specialization (+0x141) into the draft specialization; hide the crafting panel (G+0x8008c0) and the blueprint preview (G+0x800ad4). |
| the crafting panel is visible at the start of a frame (0x48b80c) | flag := 0 |
| the start-screen root is visible (0x48d9b9) | flag := 0 (and the inventory, G+0x800900 and G+0x8008dc are hidden) |
| a right-button press over no interface widget (0x47b719) | flag := 0, together with G+0x8008f0 and the Esc-row flag G+0x8008f2 |
| the player's movement intent becomes non-zero (input routine 0x4a6b50, 0x4a8deb … 0x4a8ebd; the vector at obj +0x40, *believed* the intent of `alpha-player-control.md` §3) | every window is closed: flag := 0 and the other window flags/panels cleared |

Closing never commits the draft; a pending change is simply lost and the next opening copies
the player's values again.

**Sounds.** The toggle itself plays nothing. Each frame (0x48b2cb … 0x48b346) the game asks
whether any interface window holds the input (0x47f1d0, which includes G+0x8008f1); when that
answer changes from no to yes it plays **0x53**, from yes to no **0x54**. So opening the
skill window with nothing else open plays `menu-open2.wav` and closing it `menu-close2.wav`.
While the window is open the interface holds the input, so the ability bar does not fire
(`alpha-creatures-behaviour.md` §16.3).

---

## 2. The tree

### 2.1 The 11 skills

Skill *i* is the player's level array entry *i*. The order and meaning are fixed for every
class; only skills 6, 7 and 8 depend on class and specialization (the same rule as the
ability bar, `alpha-creatures-behaviour.md` §16.2). Name keys are the dictionary key
suffixes registered at start-up (0x58eb00): skill index → G+0x800114, ability id →
G+0x80011c.

| *i* | Name key | Icon (`data3.db`) | Button | Parent | Row, column |
|---:|---|---|---|---|---|
| 6 | ability of key 1 (table below) | the ability's icon | `skillbutton` | — | row 1, col 0 |
| 7 | ability of key 2 | the ability's icon | `childskillbutton` | 6 | row 1, col 1 |
| 8 | ability of key 3 (specialization dependent) | the ability's icon | `childskillbutton` | 7 | row 1, col 2 |
| 9 | `SkillAbility4` | none | `childskillbutton` | 8 | row 1, col 3 |
| 0 | `SkillPetTaming` | `pet-taming.png` | `skillbutton` | — | row 2, col 0 |
| 1 | `SkillPetRiding` | `pet-riding.png` | `childskillbutton` | 0 | row 2, col 1 |
| 2 | `SkillClimbing` | `climbing.png` | `skillbutton` | — | row 3, col 0 |
| 3 | `SkillHangGliding` | `hang-gliding.png` | `childskillbutton` | 2 | row 3, col 1 |
| 4 | `SkillSwimming` | `swimming.png` | `skillbutton` | — | row 4, col 0 |
| 5 | `SkillBoatDriving` | `ship-driving.png` | `childskillbutton` | 4 | row 4, col 1 |
| 10 | `SkillAbility5` | none | `childskillbutton` | — | **never shown** |

**Skills 6–8 by class** (the specialization used is the **draft** one, so the window
previews the other specialization's ability; ids and icon names as
`alpha-ui-screens.md` §5.5):

| Class | Skill 6 | Skill 7 | Skill 8 |
|---|---|---|---|
| 1 warrior | 54 smash (`AbilitySmash`) | 86 cyclone (`AbilityCyclone`) | spec 0: 102 warfrenzy (`AbilityWarFrenzy`); spec 1: 101 bulwark (`AbilityBulwark`) |
| 2 ranger | 21 ranger-kick (`AbilityRangerKick`) | 50 retreat2 (`AbilityRetreat`) | spec 0: 99 aim (`AbilityAim`); spec 1: 100 swiftness (`AbilitySwiftness`) |
| 3 mage | spec 0: 88 fire-explosion (`AbilityFireExplosion`); spec 1: 34 healing-stream (`AbilityHealingStream`) | 103 mana-shield (`AbilityManaShield`) | 49 teleport (`AbilityTeleport`) |
| 4 rogue | 48 intercept (`AbilityIntercept`) | 79 sneak (`AbilitySneak`) | spec 0: 97 camouflage (`AbilityCamouflage`); spec 1: 96 shuriken (`AbilityShuriken`) |
| other | none | none | none |

Skill 9 is a real, spendable slot with no icon and no dictionary text (the Alpha ability bar
never uses it); skill 10 exists in the array, is saved and networked, but its button is
hidden every frame.

### 2.2 Buttons and templates (`gui.plx`)

Built at start-up (0x46556b … 0x465623) as clones under the in-game root:

* two clones of `specializationbutton` → G+0x800850 (spec 0) and G+0x800854 (spec 1);
* eleven clones into the list **G+0x800844** (index = skill *i*): `skillbutton` for
  *i* = 0, 2, 4, 6 and `childskillbutton` for every other *i*.

The three templates are then removed from the template tree. Shapes, relative to the button's
top-left corner (*x*₀, *y*₀) (each template's pivot is its body's top-left corner):

| Template | Body | `frame` | Other children |
|---|---|---|---|
| `skillbutton` | 42 × 42, 3-px notched corners, white fill (carries the icon texture, shape offset 0x7ac), black 4-px stroke | the same outline inset 1 px, 2-px white stroke (inner), no fill | `info` text pair (below) |
| `childskillbutton` | as `skillbutton` | as `skillbutton` | `info` as `skillbutton`; plus **`node`**: the link glyph (below) |
| `specializationbutton` | **60 × 60**, 3-px notched corners, white fill (icon texture), black 4-px stroke | inset 1 px, 2-px white stroke | none (no level text) |

* **`info`** (the level number): origin (*x*₀ + 43.8, *y*₀ + 43.7), 10 px, right-aligned
  (flags 2); an outline layer filled (0, 0.763, 0.814) with a black 2.5-px outline, under a
  white fill layer with no outline. The number therefore sits on the button's bottom-right
  corner, its right edge 1.8 px outside the body.
* **`node`** (child buttons only): a small 16-point polygon, white fill, black 2-px stroke,
  spanning *x*₀ − 15 … *x*₀ + 3, *y*₀ + 12 … *y*₀ + 31. It bridges the 13-px gap between a
  parent and its child to the left of the child (the parent's right edge is at *x*₀ − 13),
  vertically about centred. It is the only "line" of the tree: there are no other connector
  lines. (It carries a `Button` widget named `node`, 19 × 21, that the code never queries.)
* Hit areas: each button's widget. The skill buttons' widget is 42 × 42 (the body). The
  `specializationbutton` template's widget is also bound 42 × 42 although its body is 60 × 60
  (*believed* the hit area is the body's top-left 42 × 42; §9 S-2).

### 2.3 Positions (every frame while open, 0x48da47 … 0x48e1fa)

Let (*X*, *Y*) be the panel's top-left corner (the host node's position, 0x62f630 / 0x62f660,
*believed* the top-left) and *x*ₐ = trunc(*X* + 50), *y*ₐ = trunc(*Y* + 70). Each button's
pivot (its body's top-left) is put on:

| Button | Top-left |
|---|---|
| specialization 0 | (*x*ₐ, *y*ₐ) |
| specialization 1 | (*x*ₐ + 70, *y*ₐ) |
| skill 6, 7, 8, 9 | (*x*ₐ + 0 / 55 / 110 / 165, *y*ₐ + 80) |
| skill 0, 1 | (*x*ₐ + 0 / 55, *y*ₐ + 140) |
| skill 2, 3 | (*x*ₐ + 0 / 55, *y*ₐ + 200) |
| skill 4, 5 | (*x*ₐ + 0 / 55, *y*ₐ + 260) |
| skill 10 | not placed (keeps the template position) and hidden |

Panel-relative: specialization buttons at (50, 70) and (120, 70), 60 px square with a 10-px
gap; skill rows at y = 150, 210, 270, 330 (60-px pitch, 42-px buttons), columns at
x = 50, 105, 160, 215 (55-px pitch).

### 2.4 Per-frame state of the buttons (0x48b820 …, 0x48e200 … 0x48eac8)

1. **Start of every frame:** all 11 skill buttons are hidden (0x48b820 … 0x48b8a9).
2. **Window closed:** both specialization buttons are hidden (0x48ead4); nothing else.
3. **Window open**, after placing (§2.3):
   * **Specialization button *s*** (0, 1): made visible. Its icon is looked up in the
     specialization icon table G+0x800830 with the key (player class +0x140, *s*):
     (1, 0) `warrior-berserker.png`, (1, 1) `warrior-guardian.png`, (2, 0)
     `ranger-sniper.png`, (2, 1) `ranger-scout.png`, (3, 0) `mage-fire.png`, (3, 1)
     `mage-water.png`, (4, 0) `rogue-assassin.png`, (4, 1) `rogue-ninja.png` (table built
     in 0x459c40).
     Found: body texture := icon, `textureSaturation` (shape offset 0x544) := **1 if *s* is
     the draft specialization, else 0** (grey). Not found (no class): texture := none (−1).
   * **Skill button *i***: visible exactly when *i* ≠ 10. Its `info` text := the **draft
     level** as a plain decimal number ("0", "7", …). Icon: for *i* = 6, 7, 8 the ability id
     of §2.1 looked up in the ability icon table (G+0x800820); for every other *i* the skill
     icon table (G+0x800828) by *i*; not found → texture none (−1) and the saturation is left
     as it was. Found → texture := icon and `textureSaturation` := **0 when the button is
     locked, else 1**.
   * **Locked** (draft values): 1 when skill 0 < 5; 3 when skill 2 < 5; 5 when skill 4 < 5;
     7 when skill 6 < 5; 8 when skill 7 < 5; 9 when skill 8 < 5. Roots (0, 2, 4, 6) and 10
     are never locked.
   * **Hover** (the button's widget contains the mouse, 0x6294c0): its `frame` child's fill
     := **(0, 1, 1, 1)** (cyan), otherwise (1, 1, 1, 1). A hovered button also feeds the
     tooltip (§5.3): the preview widget (G+0x8008e8) gets item := none (+0x160 = 0) and
     either +0x170 := *s* (specialization) or +0x168 := *i* and +0x16c := draft level of *i*;
     the preview box (G+0x8008e4) is made visible and the "found" flag of the tooltip
     placement is set. (+0x168 and +0x170 are reset to −1 at the start of the block.)

---

## 3. Skill points

### 3.1 The budget

There is no stored "unspent points" value. The budget is derived on every click from the
level and the mana cubes (0x47b8d1 … 0x47b938):

**Points available *A* = 2·*L* − 2 + ⌊*c* / 4⌋**

(*L* = level +0x190; *c* = mana-cube counter +0x1164; C-style division, *c* ≥ 0.) So a
character earns **2 points per level above 1** and **1 point per four mana cubes** (the
fourth, eighth, … cube prints "You gain one skill point.", `alpha-ui-screens.md` §8.1.9).

**Points used *U*** = Σ of the 11 **draft** levels (skills 0 … 10).

### 3.2 The "Points" line shows a different total

The window line is "Points: " + *U* + "/" + (2·*L* − 2) (0x4ddbb1 … 0x4ddc0c): the right-hand
number **omits the mana-cube points**. A character who has spent cube points therefore sees
the left number exceed the right (for example "Points: 19/18"). This is the original
behaviour, kept for parity.

### 3.3 Limits

* **No maximum level** per skill exists in the window: a skill can be raised as long as
  points remain.
* **Prerequisite:** a child can be raised only when its parent's draft level is **≥ 5**
  (1 ← 0, 3 ← 2, 5 ← 4, 7 ← 6, 8 ← 7, 9 ← 8).
* **Floor:** levels never go below 0 in the draft.
* A new character starts with all skills 0 and *c* = 0 (*believed*: zeroed with the creature;
  matches the sample save).

---

## 4. Spending: mouse rules

All clicks are handled by the mouse-press routine **0x47b600** only while G+0x8008f1 is set,
in this order: specialization buttons (left button only), skill buttons (both buttons), then,
for the left button only, Learn and Cancel. Every button whose widget contains the mouse is
handled (they do not overlap).

### 4.1 Left click on specialization button *s* (0x47b85a … 0x47b8a5)

Draft specialization := *s*; sound **0x55**. Always allowed (any level, any points, even if it
already was *s*; the sound plays anyway). It only changes the draft; see §4.4 for the cost.

### 4.2 Left click on skill button *i* — raise (0x47b8d1 … 0x47b9dd)

1. If *U* ≥ *A* → nothing (no sound).
2. If *i* is a child (1, 3, 5, 7, 8, 9) and its parent's draft level is < 5 → nothing.
3. Otherwise draft level *i* += 1; sound **0x55**.

### 4.3 Right click on skill button *i* — lower (0x47b9e2 … 0x47ba9e)

Any non-left button lands here (*believed* only the right button reaches this code).

1. Let *n* = draft level *i*. If *n* ≤ 5 and *i* is a parent (6 → child 7, 7 → 8, 8 → 9,
   0 → 1, 2 → 3, 4 → 5) whose child's draft level is > 0 → nothing (no sound). (Lowering a
   parent from 6 or more is always allowed, since it stays ≥ 5.)
2. Otherwise draft level *i* := *n* − 1; sound **0x58**; if the result is < 0 it is set to 0
   (so a right click on a 0 still plays the sound).

Lowering below the player's **committed** level is allowed; it is what costs money (§4.4).

### 4.4 Cost of a change (0x4df9c0)

With the committed values (player) and the draft:

**cost = trunc( Σ over *i* with draft*ᵢ* < player*ᵢ* of *F*(*L*) · (player*ᵢ* − draft*ᵢ*) · 10
  + [draft spec ≠ player spec and *L* > 1] · *F*(*L*) · 100 )** copper,

accumulated in single precision in skill order 0 … 10, then the specialization term, then
truncated toward zero. **Raising** skills is free. **Lowering** committed levels (a re-skill)
costs 10·*F*(*L*) per point. **Changing specialization** costs 100·*F*(*L*), and is **free at
level 1**. Specialization is reversible at the same price as choosing it.

### 4.5 The class-trainer test (0x47f030)

Needed only when cost > 0. True when some creature in the game's creature map (G+0x2e8) has
**appearance flag 0x40** (byte +0x7e, bit 6), the **same class** (+0x140) as the player and a
squared distance to the player, in blocks (each i64 axis difference × 1/65536), **below 16**
(i.e. within 4 blocks). The flag is *believed* to mark class trainers (§9 S-3).

### 4.6 Learn and Cancel (left button, 0x47c806 … 0x47c8e4)

The two buttons exist only while the draft differs from the player (any of the 11 levels or
the specialization).

* **Learn** is **allowed** when: something differs, cost ≤ money (+0x1304), and (cost = 0 or
  the trainer test passes). Hit area (widget-local): 0 < *x* < *w*/2 and *h* − 30 < *y* < *h*
  (0x4df880). A left press inside it while allowed: money −= cost; the player's 11 skill
  levels := the draft; the player's specialization := the draft specialization. **No sound.**
* **Cancel** hit area: *w*/2 < *x* < *w*, *h* − 30 < *y* < *h* (0x4df760). A left press inside
  it (while something differs): draft := the player's levels and specialization. No sound.

After Learn or Cancel nothing differs, so both buttons disappear on the next draw.

### 4.7 Effect on the game

The ability bar and every skill effect read the **player's** levels (+0x1138 …), not the
draft: an ability slot appears on the bar when its skill becomes > 0 after Learn
(`alpha-creatures-behaviour.md` §16.2). The level of skill 8 is shared by both
specializations' abilities (a ranger's aim and swiftness use the same skill 8 level).

---

## 5. Texts

### 5.1 Window texts (SkillWidget draw 0x4dd810; widget-local, origin = panel top-left)

| Text | When | Origin | Size, stroke | Align | Fill |
|---|---|---|---|---|---|
| "Skills" | always | (15, 25) | 12 px, 3 | left | white |
| "Points: " + *U* + "/" + (2·*L* − 2) | always | (15, 45) | 10 px, 2 | left | white |
| "Not enough money." | cost > 0 and cost > money | (15, trunc(*h* − 65)) = (15, 395) | 10 px, 2 | left | (1, 0.25, 0.25, 1) |
| "Requires class trainer." | cost > 0, cost ≤ money, trainer test fails | (15, 395) | 10 px, 2 | left | (1, 0.25, 0.25, 1) |
| "COST:" | cost > 0 | (15, 415) | 10 px, 2 | left | white |
| ⌊cost/10000⌋ + " G " | cost > 0 | (140, 415) | 10 px, 2 | right | (1, 0.9, 0, 1) |
| (⌊cost/100⌋ mod 100) + " S " | cost > 0 | (180, 415) | 10 px, 2 | right | (0.7, 0.7, 0.7, 1) |
| (cost mod 100) + " C " | cost > 0 | (220, 415) | 10 px, 2 | right | (0.8, 0.5, 0, 1) |
| "Learn" | draft ≠ player | (*w*/3, *h* − 20) = (100, 440) | 12 px, 3 | centred | grey (0.5, 0.5, 0.5, 1) when not allowed; else **(0, 1, 1, 1)** while hovered, white otherwise |
| "Cancel" | draft ≠ player | (2*w*/3, *h* − 20) = (200, 440) | 12 px, 3 | centred | (0, 1, 1, 1) while hovered, else white |

The three coin parts are all drawn (zeros included) whenever cost > 0. Money and coins as
`alpha-ui-screens.md` §8.1.1 (1 silver = 100 copper, 1 gold = 10 000).

There is **no "x/y" level text**: each skill button shows only its draft level (§2.4), and
there are no skill-name labels in the window; names appear only in the tooltip.

### 5.2 Tooltip box and placement

The ordinary item tooltip (`alpha-ui-screens.md` §8.1.7, `preview` 300 × 300, PreviewWidget
draw 0x4d50a0) with no item: all four rarity stars hidden, no model. The hover sets the
tooltip's "found" flag, so it is placed as a window-source tooltip: **top-left = mouse +
(20, 20)** (*believed*, same flag as the inventory hover). The text block starts at (*X* + 14,
*Y* + 25) of the box with a **wrap width of 275**.

### 5.3 Tooltip text

* **Specialization button** (0x4a62c0): the dictionary entry
  `specialization:<class>:<spec>` with <class> = `warrior`, `ranger`, `mage`, `rogue` (player
  class 1–4) and <spec> = class 1: 0 `berserker`, 1 `guardian`; class 2: 0 `sniper`, 1
  `scout`; class 3: 0 `fire`, 1 `water`; class 4: 0 `assassin`, 1 `ninja`. No variables.
* **Skill button *i*** at draft level *n* (0x4a5710), with *Name* = the name key of §2.1:
  1. variables := V(*i*, *n*) (§5.4); render `description:`*Name*;
  2. if *n* > 0: render `skill:level`, then `details:`*Name* (both with V(*i*, *n*));
  3. variables := V(*i*, *n* + 1); render `skill:nextlevel`, then `details:`*Name*.

  Keys missing from the dictionary render nothing (skills 9 and 10 show only the
  `skill:nextlevel` heading). In the shipped dictionary `skill:level` and `skill:nextlevel`
  use `%` page breaks to leave blank pages around a short heading (`@level` in the first).

Rendering is the speech renderer of `alpha-talk-vendors.md` §2.2 (pages, `$style` colours:
white default, `$stress` violet (0.6, 0.3, 1), `$number` green (0.3, 1, 0.3), …, `@variable`
substitution). Each **page** becomes one paragraph laid out by this word layout
(0x4a5710 / 0x4a62c0, identical):

1. Size *s* = **12 px for the first page**, **10 px for every later page**. Pen *x* = left
   edge *x*ₗ, a "no space" flag set.
2. For each word (segment) with measured width *d*:
   * it is **punctuation-like** when it equals ".", "-", ",", ";", "!" or "?", or contains
     ":" or "/" or ")" (*believed* "contains" for the last three tests, 0x439190);
   * unless the flag is set, advance *x* by **3** before a punctuation-like word, else by
     *s* (the space);
   * the flag := (the word is "-");
   * if the word is not punctuation-like and *x* + *d* > *x*ₗ + 275: new line, *y* += *s* + 4,
     *x* := *x*ₗ;
   * draw it at (*x*, *y*): outline pass stroke 3 (white fill, black outline), then the fill
     pass in the segment colour;
   * *x* += *d*; if the word is "/" or "(" then *x* −= 9.
3. After the page: *y* += **18**. (Empty pages, from `% %`, still advance 18 each.)

### 5.4 Tooltip variables V(*i*, *n*) (0x478800)

Every call sets all of these (the dictionary picks the ones it uses). R(*x*) =
trunc(*x*·100 + 0.5)·0.01, mirrored for negatives (0x4874a0); numbers are written with the
default stream format (6 significant digits, no trailing zeros: 17.67, 5, 0.91). *v*(*n*) =
1 − 1/(0.1·*n* + 1), 0 for *n* < 1 (0x43c980); *k*(*n*) = 1 − 1/(0.1·(*n* + 1) + 1)
(0x43e660); *v*(id, *n*) and length(id, *n*) are `alpha-creatures-behaviour.md` §16.1 /
§16.5 evaluated at level *n* (0x43ed60, 0x43e6a0 with an explicit level). "%" means the text
gets a "%" appended.

| Variable | Value |
|---|---|
| `@level` | *n* |
| `@health` | R(50·*v*(*n*)) % |
| `@ridingspeed` | R(100·(1 + *v*(*n*))) % |
| `@climbingpower` | R(100·*k*(*n*)) % |
| `@flyingspeed` | R(100·(3·*v*(*n*) + 1.5)) %, 0 % for *n* < 1 |
| `@swimmingspeed` | R(100·(0.5·*v*(*n*) + 0.5)) %, 50 % for *n* < 1 |
| `@boatspeed` | R(100·(3·*v*(*n*) + 1)) %, 0 % for *n* < 1 |
| `@smashcooldown`, `@cyclonecooldown`, `@bulwarkcooldown`, `@warfrenzycooldown`, `@rangerkickcooldown`, `@retreatcooldown`, `@aimcooldown`, `@swiftnesscooldown`, `@fireexplosioncooldown`, `@manashieldcooldown`, `@teleportcooldown`, `@interceptcooldown`, `@shurikencooldown`, `@camouflagecooldown`, `@sneakcooldown` | R(length(id, *n*) / 1000) seconds, for ids 54, 86, 101, 102, 21, 50, 99, 100, 88, 103, 49, 48, 96, 97, 79 |
| `@cycloneduration` | R(5000 / 1000) = 5 (the P2 length of action 86, 0x447310) |
| `@bulwarkamount` | R(100·(0.3·*v*(101, *n*) + 0.25)) % |
| `@bulwarkduration`, `@warfrenzyduration`, `@swiftnessduration` | the fixed text "10" |
| `@manashieldduration` | the fixed text "30" |
| `@warfrenzyamount` | R(100·(9·*v*(102, *n*) + 1)) % (*as read*) |
| `@rangerkickknockback`, `@retreatdistance`, `@aimstealth`, `@swiftnessamount`, `@fireexplosionknockback` | R(100·*v*(*n*)) % |
| `@manashieldpower` | R(100·*v*(103, *n*)) % |
| `@healingstreamcost` | R(800·cost(34, *n*)) (= R(100·(1 − 0.75·*v*))), no % |
| `@camouflageduration` | R((12000·*v*(97) + 8000) / 1000) with *v*(97) at the player's **committed** level (called with level −1), not *n* — so the "next level" page shows the current duration |
| `@sneakspeed` | R(100·0.4·*v*(79, *n*)) % |
| `@sneakstealth` | R(100·*v*(79, *n*)) % |

---

## 6. Network and saves

* **Nothing is sent by the window.** Learn only writes the local player's creature. The
  changed record fields go out in the player's next creature update (packet 0, delta encoded)
  because the client owns its player: **field 46** (record 0x1128, 11 × i32, the skill
  levels) and **field 22** (record 0x131, the specialization). Money (+0x1304) is outside the
  record and is never sent (as for vendors, `alpha-talk-vendors.md` §3.5). The draft is
  local.
* **Field 47** (record 0x1154 = obj +0x1164, i32) is the **mana-cube counter** *c*. This
  settles "unknown" for `alpha-net-packets.md` field 47 and `alpha-saves.md` field 22.
* **Saves** (`alpha-saves.md` §3.2): field 8 specialization (+0x141), field 16 money,
  **field 22 mana-cube counter**, field 23 the 11 skill levels (+0x1138; read only when the
  count is 11). The save writer copies +0x1138 (44 bytes) and +0x1164 (0x44d790); the reader
  restores them (0x44be40). The draft is not saved.
* The server was not seen to validate or clamp skill levels (*believed*; §9 S-5).

---

## 7. Sounds

| Id | When |
|---|---|
| 0x55 `menu-select.wav` | a specialization button is clicked; a skill is raised |
| 0x58 `menu-drop-item.wav` | a right click on a skill passes the parent test (also at 0) |
| 0x53 / 0x54 | the window opens / closes and no other window holds the input (§1.3) |
| — | Learn, Cancel, refused clicks: silent |

---

## 8. Worked examples

All at 1280 × 720: panel (130, 75) … (430, 535); *x*ₐ = 180, *y*ₐ = 145.
Specialization buttons (180, 145) and (250, 145), 60 × 60. Skill buttons: 6 (180, 225),
7 (235, 225), 8 (290, 225), 9 (345, 225); 0 (180, 285), 1 (235, 285); 2 (180, 345),
3 (235, 345); 4 (180, 405), 5 (235, 405), each 42 × 42. Title baseline (145, 100); Points
(145, 120); warning (145, 470); cost row y = 490 (gold right edge 270, silver 310, copper
350); Learn centred on (230, 515), Cancel on (330, 515); Learn hit band (130 … 280) ×
(505 … 535), Cancel (280 … 430) × (505 … 535).

### 8.1 A level-10 warrior spending points

Warrior (class 1), specialization 0 (Berserker), *L* = 10, 5 mana cubes, all skills 0,
money 500 copper. *A* = 2·10 − 2 + ⌊5/4⌋ = **19**.

1. **X**: draft := all 0, spec 0; sound 0x53. "Points: 0/18". Skill 6 shows the smash icon,
   7 cyclone (grey: locked), 8 warfrenzy (grey), 9 blank, 0–5 their skill icons (1, 3, 5
   grey). Spec button 0 (`warrior-berserker.png`) in colour, 1 (`warrior-guardian.png`) grey.
2. Left click on cyclone (235, 225): locked (skill 6 = 0 < 5) → nothing, silent.
3. Left click on smash five times: draft 6 = 1 … 5, five 0x55 sounds. Cyclone turns colour.
4. Cyclone ×5 → 5 (warfrenzy unlocks); warfrenzy ×5 → 5 (skill 9 unlocks); climbing (180,
   345) ×4 → 4. *U* = 19 = *A*.
5. Left click on pet taming: *U* ≥ *A* → refused, silent. The line reads **"Points: 19/18"**.
6. Learn and Cancel are shown; cost = 0 (only raises), so Learn is white (cyan when hovered)
   and no COST row or warning is drawn. Left click at (200, 520) → Learn: the player's skills
   := draft, money unchanged, silent. The ability bar now shows smash, cyclone, warfrenzy on
   keys 1–3.
7. Right click on smash (draft 5): 5 ≤ 5 and cyclone's draft is 5 > 0 → refused, silent.
8. Right click on warfrenzy (5): skill 9 is 0 → draft 8 := 4, sound 0x58. Draft 8 (4) <
   committed (5): cost = trunc(*F*(10)·1·10) = trunc(19.066427) = **19**. Drawn: "COST:" at
   (145, 490), "0 G " right edge 270, "0 S " right edge 310, "19 C " right edge 350.
   * Far from any warrior trainer: "Requires class trainer." in red at (145, 470) and Learn
     grey; clicking it does nothing.
   * Within 4 blocks of one: no warning, Learn white; Learn → money 500 → 481, warfrenzy 4.
9. Also clicking specialization 1 (Guardian) before Learn: cost = trunc(19.066427 +
   190.66427) = **209** → "0 G ", "2 S ", "9 C "; skill 8's icon and tooltip switch to
   bulwark. Cancel instead restores the committed values and spec 0.

Tooltip check, hovering smash at draft level 2: `description:AbilitySmash`, then (n = 2 > 0)
`skill:level` with `@level` = 2 and `details:AbilitySmash` with `@smashcooldown` = R(17666 /
1000) = **17.67**; then `skill:nextlevel` and the details again with `@level` = 3 and
`@smashcooldown` = R(16769 / 1000) = **16.77** (length(54, 3) = trunc(20000 − 14000 ·
0.2307692) = 16769).

### 8.2 A ranger choosing Sniper

Ranger (class 2), currently specialization 1 (Scout), *L* = 6, committed skills 6 → 3,
7 → 2, 8 → 1, money 1000 copper. *A* = 10; "Points: 6/10".

1. **X**: spec button 0 (180, 145) shows `ranger-sniper.png` grey; button 1 (250, 145)
   `ranger-scout.png` in colour. Skill 8 shows swiftness.
2. Hovering spec button 0: cyan frame; tooltip `specialization:ranger:sniper`, first page
   12 px, later pages 10 px.
3. Left click on it: draft spec := 0, sound 0x55. Button 0 in colour, 1 grey; skill 8 now
   shows **aim** (tooltip `AbilityAim`, `@aimcooldown` etc. at level 1).
4. Draft differs → Learn and Cancel. Cost = trunc(100·*F*(6)) = trunc(151.57166) = **151**:
   "0 G ", "1 S ", "51 C ". It needs a ranger trainer within 4 blocks; with one, Learn → money
   849, player spec := 0. The ability bar's key 3 becomes aim (skill 8 = 1 carries over).
5. Switching back to Scout later costs the same 151 at level 6. At **level 1** the change is
   free (*L* > 1 fails), needs no trainer, and Learn is always allowed.

---

## 9. Open points

* **S-1** Skill 10's clone is hidden but stays at its template position (top-left
  (570, 1058) in window pixels). Whether the hover/click test (0x6294c0) ignores hidden nodes
  was not checked; if it does not, a click there at 1920 × 1080 would raise skill 10
  invisibly. Confirm by capture before copying the behaviour.
* **S-2** The specialization button's hit area (42 × 42 widget bind on a 60 × 60 body).
* **S-3** Appearance flag 0x40 as "class trainer": which spawns set it (town NPC generator,
  `alpha-worldgen-structures.md`) was not traced. Classes 131/137 are the interaction windows
  of `alpha-talk-vendors.md` §4, not this test.
* **S-4** The tooltip-placement flag for skill hovers is taken as the window source
  (mouse + (20, 20)); confirm by capture.
* **S-5** Whether the server checks skill levels or the point budget (none found in
  `Server.exe`; seven references to the skill array are record copies).
* **S-6** `@warfrenzyamount` = R(100·(9·*v* + 1)) % reads oddly large (550 % at *v* = 0.5);
  the constant 9.0 was read twice from 0x702a90. Keep as read.
* **S-7** The tooltip word-layout tests 0x439190 (":" "/" ")") are taken as "contains".
