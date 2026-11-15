Status: FINAL / STABLE / HELD

# Alpha skills: the skill window and skill points

Behavioural specification of the **skill window** of Cube World **Alpha** (`Cube.exe`) and of
**skill points**: where the window sits, the 11-skill tree and its buttons, the two
specialization buttons, how points are counted and spent, what re-skilling and changing
specialization cost, the texts and tooltips, and what is saved and carried in the creature
record. This is follow-up **F-6** of `alpha-ui-screens.md` §12.

Written from the original Alpha client image `binary/Cube.exe` (32-bit, image base
0x400000), its decompile export and fresh disassembly of the image bytes, plus the dictionary
`dict_en.xml` (keys and page structure only; its texts are game content and are not
reproduced here). Every routine is cited as an **RVA in `Cube.exe`**. Nothing here is copied
code. Readings that could not be fully proven are not stated as facts; they are listed in §9.

Referenced elsewhere and only pointed to here: `alpha-ui-screens.md` §2 (layout model), §3.3
(`blackwidget` look), §3.4 (text origin and alignment flags), §5.5 (ability bar and icon
tables), §7 (menu-button row), §8.1.1 (money and coins), §8.1.7 (the `preview` tooltip box),
§8.1.9 (mana cubes), §10 (interface sounds); `alpha-creatures-behaviour.md` §10.2 (level
factor), §16 (abilities, ability values, cooldown lengths); `alpha-talk-vendors.md` §2
(dictionary markup and page rendering); `alpha-saves.md` §3.2; `alpha-net-packets.md` §2.1.

---

## 0. Conventions

* *G* is the game object; *P* the local player's creature object (G+0x8006d0). Creature
  offsets are **object** offsets (record offset + 0x10): +0x140 class (1 warrior, 2 ranger,
  3 mage, 4 rogue), +0x141 specialization (0 or 1, one byte), +0x190 level *L*,
  **+0x1138 + 4·*i*** the level of skill *i* (*i* = 0 … 10, i32), **+0x1164 the mana-cube
  counter *c*** (i32), +0x1304 money in copper.
* *W*, *H*: client size in pixels (G+0x11c, G+0x120). Interface positions are window pixels.
* **The window widget** is the SkillWidget object (0x194 bytes, constructor RVA 0xdd750,
  kept at G+0x80090c). Its fields:

| Offset | Content |
|---|---|
| +0x160 + 4·*i* (*i* = 0 … 10) | the **draft** level of skill *i* (zeroed by the constructor) |
| +0x18c | the **draft** specialization (i32, 0 or 1) |
| +0x190 | back pointer to *G* |

  The draft is a working copy. Nothing changes on the player until **Learn** (§4.6).
* *F*(*L*) is the level factor of RVA 0x45f60 called with (*L*, 0): in single precision,
  *t* = 3·(1 − 1/((*L* − 1)·0.05 + 1)), *F* = 2^*t* (power evaluated in double, rounded to
  single) multiplied by 2^(0.25·0) = 1. Values: *F*(1) = 1, *F*(6) = 1.5157166,
  *F*(10) = 1.9066427.
* *v*(*n*) = 1 − 1/(0.1·*n* + 1) for *n* ≥ 1 and 0 for *n* < 1 (RVA 0x3c980).
  *k*(*n*) = 1 − 1/(0.1·(*n* + 1) + 1) with no guard (RVA 0x3e660).
  *v*(id, *n*) (RVA 0x3ed60, evaluated on *P*): the same curve 1 − 1/(0.1·*n* + 1) at level
  *n* of the ability's skill; a negative *n* means the player's **committed** level of that
  skill; an ability without a skill gives 1 (see also §9 U-9).
* Ability → skill slot (RVA 0x3c9c0): ids 21, 34, 48, 54, 88 → skill 6; 50, 79, 86, 103 →
  skill 7; 49, 96, 97, 99, 100, 101, 102 → skill 8; every other id → none.
* Window texts are drawn with `resource1.dat` in two passes of RVA 0x239b30: an outline pass
  (white fill, black outline of the stated stroke) then a fill pass in the stated colour with
  stroke 0. "*s* px, stroke *k*" gives the size and outline width. Alignment flag 0 left,
  1 centred, 2 right-aligned; the origin is the first baseline.
* Sounds are played by RVA 0x84350 at the listener position (G+0x140), volume 1, pitch 1.
  Ids used here: **0x53** `menu-open2.wav`, **0x54** `menu-close2.wav`, **0x55**
  `menu-select.wav`, **0x58** `menu-drop-item.wav` (sound switch at RVA 0x84604).

---

## 1. Window host, size, placement, opening and closing

### 1.1 Host and size

Built once at start-up (RVA 0x64ddc … 0x64ee2, inside the start-up routine RVA 0x59c40): a
clone of the `blackwidget` template (template node kept at G+0x8008c8; look as
`alpha-ui-screens.md` §3.3) made under the in-game root (G+0x800884), kept at
**G+0x800908**, and resized to **300 × 460**. An empty container node is created, the
SkillWidget is constructed on it and stored at G+0x80090c, and it is bound to the panel's
node of the template's name with resizing enabled (RVA 0x231460). The SkillWidget's
virtual-table slot 1 is its draw routine **RVA 0xdd810**; the other slots are shared
widget-library routines.

The skill and specialization **buttons are not children of the panel**: they are separate
clones made under the in-game root and moved every frame (§2.3).

### 1.2 Placement

Layout routine RVA 0x82a40 (RVA 0x83a9e … 0x83b56), redone on every layout pass. With the
inventory panel G+0x8008bc (built 400 × 285) and the live node sizes, the panel position is

* **x = trunc(W/2) − 0.5·400 − 300 − 10 = W/2 − 510**,
* **y = trunc(H/2) − 285**

(W/2 and H/2 are integer halves truncated toward zero). This is the same spot as the
crafting panel (G+0x8008c0, built 350 × 285), which is why the two windows exclude each
other (§1.3). At 1280 × 720 the panel spans **(130, 75) … (430, 535)**.

### 1.3 Opening and closing

The window is open while the byte flag **G+0x8008f1** is set. Every frame the panel's
visibility is set equal to the flag (RVA 0x8d9f8 … 0x8da06).

| Event | Effect |
|---|---|
| the **X** key (character code 88 in the key switch at RVA 0x7e756, case at RVA 0x7e7cf), or menu-button index 1 of the mouse handler (RVA 0x7b805) → toggle RVA 0x88c70 | flag := not flag. If it is now set: copy the player's 11 skill levels (+0x1138 …) into the draft (+0x160 …) and the player's specialization (+0x141) into the draft specialization; hide the crafting panel (G+0x8008c0) and the node G+0x800ad4. |
| the crafting panel is visible at the start of a frame (RVA 0x8b806 … 0x8b819) | flag := 0 |
| the node G+0x800874 is visible (RVA 0x8d9b3 … 0x8d9f3) | flag := 0, and the nodes G+0x8008bc (inventory), G+0x800900 and G+0x8008dc are hidden |
| a right-button press (mouse-handler argument 1) while the interface query RVA 0x250ae0 returns none (RVA 0x7b70f …) | the two bytes G+0x8008f0 and G+0x8008f1 := 0, G+0x8008f2 := 0, and the nodes G+0x8008bc, G+0x8008c0 and G+0x8008dc are hidden |
| on an input pass where the player object's vector +0x40 … +0x48 has a squared length > 0 (RVA 0xa8deb … 0xa8ebd, inside the input routine RVA 0xa6b50) | the nodes G+0x8008bc, G+0x8008c0, G+0x8008c4, G+0x8008dc, G+0x8008f8, G+0x800900 and G+0x800910 are hidden and the two bytes G+0x8008f1 and G+0x8008f2 := 0 |

Closing never commits the draft; a pending change is lost and the next opening copies the
player's values again.

**Sounds.** The toggle itself plays nothing. Each frame (RVA 0x8b2cb … 0x8b346) the game
evaluates "an interface window holds the input" (a virtual call on *G*, or the byte
G+0x800a40 set) and compares it with the previous frame's answer (a static byte): when it
changes from no to yes it plays **0x53**, from yes to no **0x54**. The routine RVA 0x7f1d0
answers yes when, among others, G+0x8008f0, G+0x8008f1 or G+0x8008f2 is set. So opening the
skill window with nothing else open plays `menu-open2.wav` and closing it `menu-close2.wav`.

---

## 2. The tree

### 2.1 The 11 skills

Skill *i* is the player's level array entry *i*. Only the abilities shown for skills 6, 7
and 8 depend on class and specialization. Start-up (RVA 0x18eb00) registers two name maps:
skill index → key (G+0x800114) and ability id → key (G+0x80011c).

| *i* | Skill-map key | Icon (skill icon table) | Template | Parent | Row, column |
|---:|---|---|---|---|---|
| 6 | `SkillAbility1` (the window uses the ability key, below) | the ability's icon | `skillbutton` | — | row 1, col 0 |
| 7 | `SkillAbility2` (ability key used) | the ability's icon | `childskillbutton` | 6 | row 1, col 1 |
| 8 | `SkillAbility3` (ability key used) | the ability's icon | `childskillbutton` | 7 | row 1, col 2 |
| 9 | `SkillAbility4` | none | `childskillbutton` | 8 | row 1, col 3 (placed, never drawn) |
| 0 | `SkillPetTaming` | `pet-taming.png` | `skillbutton` | — | row 2, col 0 |
| 1 | `SkillPetRiding` | `pet-riding.png` | `childskillbutton` | 0 | row 2, col 1 |
| 2 | `SkillClimbing` | `climbing.png` | `skillbutton` | — | row 3, col 0 |
| 3 | `SkillHangGliding` | `hang-gliding.png` | `childskillbutton` | 2 | row 3, col 1 |
| 4 | `SkillSwimming` | `swimming.png` | `skillbutton` | — | row 4, col 0 |
| 5 | `SkillBoatDriving` | `ship-driving.png` | `childskillbutton` | 4 | row 4, col 1 |
| 10 | `SkillAbility5` | none | `childskillbutton` | — | not placed, never drawn |

The skill icon table (G+0x800828, keys 0 … 5 as above) and the specialization icon table
(G+0x800830) are filled in the start-up routine (RVA 0x5f97d … 0x6001c).

**Skills 6–8 by class** (the specialization used is the **draft** one, so the window previews
the other specialization's ability; RVA 0x8e751 … 0x8e9ac for icons, RVA 0xa5710 for
tooltip names, ability-map keys from RVA 0x18eb00):

| Class | Skill 6 | Skill 7 | Skill 8 |
|---|---|---|---|
| 1 warrior | 54 (`AbilitySmash`) | 86 (`AbilityCyclone`) | spec 0: 102 (`AbilityWarFrenzy`); otherwise 101 (`AbilityBulwark`) |
| 2 ranger | 21 (`AbilityRangerKick`) | 50 (`AbilityRetreat`) | spec 1: 100 (`AbilitySwiftness`); otherwise 99 (`AbilityAim`) |
| 3 mage | spec 1: 34 (`AbilityHealingStream`); otherwise 88 (`AbilityFireExplosion`) | 103 (`AbilityManaShield`) | 49 (`AbilityTeleport`) |
| 4 rogue | 48 (`AbilityIntercept`) | 79 (`AbilitySneak`) | spec 1: 96 (`AbilityShuriken`); otherwise 97 (`AbilityCamouflage`) |
| other | none | none | none |

Skill 9 is a real slot of the array: it counts toward the points used, can be raised by the
mouse handler when its hit test succeeds (§4.2, §9 U-1), is compared by Learn/Cancel, saved
and carried in the record, but its button is never drawn (§2.4). Skill 10 likewise exists in
the array, is saved and carried, and its button is never drawn or placed.

### 2.2 Buttons and templates

Built at start-up (RVA 0x6556b … 0x65623) as clones under the in-game root:

* two clones of `specializationbutton` → G+0x800850 (spec 0) and G+0x800854 (spec 1);
* eleven clones appended to the list **G+0x800844** (index = skill *i*): `skillbutton` for
  *i* = 0, 2, 4, 6 and `childskillbutton` for every other *i*.

The three templates are then removed from the template tree. The template shapes come from
the game's `gui.plx` content; the per-frame code uses a child node named `frame` (hover
colour), a shape texture field at +0x7ac and a shape field at +0x544 (set to 0 or 1, below),
and writes the level number as the button's text. The inherited geometry readings for these
templates could not be re-confirmed (§9 U-2).

### 2.3 Positions (every frame while open, RVA 0x8da47 … 0x8e1fa)

Let (*X*, *Y*) be the panel host node's position (RVA 0x22f630 / 0x22f660) and
*x*ₐ = trunc(*X* + 50), *y*ₐ = trunc(*Y* + 70). Each button is positioned at:

| Button | Position |
|---|---|
| specialization 0 | (*x*ₐ, *y*ₐ) |
| specialization 1 | (*x*ₐ + 70, *y*ₐ) |
| skill 6, 7, 8, 9 | (*x*ₐ + 0 / 55 / 110 / 165, *y*ₐ + 80) |
| skill 0, 1 | (*x*ₐ + 0 / 55, *y*ₐ + 140) |
| skill 2, 3 | (*x*ₐ + 0 / 55, *y*ₐ + 200) |
| skill 4, 5 | (*x*ₐ + 0 / 55, *y*ₐ + 260) |
| skill 10 | not positioned (keeps its template position) |

Panel-relative: specialization buttons at (50, 70) and (120, 70); skill rows at y = 150, 210,
270, 330 (60-px pitch); columns at x = 50, 105, 160, 215 (55-px pitch). That the position is
the button body's top-left corner is an inherited reading (§9 U-3).

### 2.4 Per-frame state of the buttons

1. **Start of every frame:** all 11 skill buttons are hidden (RVA 0x8b820 … 0x8b8a9).
2. Every frame, before the open test, the preview widget (G+0x8008e8) fields +0x168 and
   +0x170 are set to −1 (RVA 0x8da0b … 0x8da2c).
3. **Window closed:** both specialization buttons are hidden (RVA 0x8ead4 … 0x8eaed).
4. **Window open**, after positioning (§2.3), for *s* = 0, 1 then *i* = 0 … 10 (RVA 0x8e200 …
   0x8eacc):
   * **Specialization button *s***: made visible. Its icon is looked up in the specialization
     icon table G+0x800830 with the key (player class +0x140, *s*): (1, 0)
     `warrior-berserker.png`, (1, 1) `warrior-guardian.png`, (2, 0) `ranger-sniper.png`,
     (2, 1) `ranger-scout.png`, (3, 0) `mage-fire.png`, (3, 1) `mage-water.png`, (4, 0)
     `rogue-assassin.png`, (4, 1) `rogue-ninja.png`. Found: texture (+0x7ac) := icon and
     field +0x544 := **1 if *s* is the draft specialization, else 0**. Not found: texture :=
     −1 (none).
   * **Skill button *i***: visibility := (*i* ≠ 10). Its text := the **draft level** as a
     decimal integer. Icon: for *i* = 6, 7, 8 the ability id of §2.1 looked up in the
     ability icon table (G+0x800820); for every other *i* the skill icon table (G+0x800828)
     by *i*. Not found (including a class outside 1 … 4 for *i* = 6 … 8) → texture := −1 and
     field +0x544 is left unchanged. Found → texture := icon and field +0x544 := **0 when the
     button is locked, else 1**.
   * **Locked** (draft values): 1 when skill 0 < 5; 3 when skill 2 < 5; 5 when skill 4 < 5;
     7 when skill 6 < 5; 8 when skill 7 < 5; 9 when skill 8 < 5. Others are never locked.
   * **Hover** (the hit test RVA 0x2294c0 on the button's widget succeeds): the `frame`
     child's colour := **(0, 1, 1, 1)**, otherwise (1, 1, 1, 1). A hovered button also feeds
     the tooltip (§5): preview +0x160 := 0, and either +0x170 := *s* (specialization) or
     +0x168 := *i* and +0x16c := draft level of *i*; the preview box (G+0x8008e4) is made
     visible and the frame's "a hover was found" byte is set.
5. **After that loop, in both the open and the closed case**, skill button **9** is hidden
   again (RVA 0x8eaef … 0x8eafe). Skill 9 is therefore never drawn, although it is positioned
   and its icon/text are updated.

---

## 3. Skill points

### 3.1 The budget

There is no stored "unspent points" value. The budget is derived on every raise click from
the level and the mana-cube counter (RVA 0x7b8d1 … 0x7b938):

**Points available *A* = 2·*L* − 2 + (*c* / 4)**

(*L* = +0x190; *c* = +0x1164; signed integer division truncating toward zero.) So a character
earns **2 points per level above 1** and **1 point per four mana cubes**; the frame routine
prints "You gain one skill point." when *c* is a multiple of 4 (RVA 0x8f9c3 … 0x8fa23).

**Points used *U*** = Σ of the 11 **draft** levels (skills 0 … 10).

### 3.2 The "Points" line shows a different total

The window line is "Points: " + *U* + "/" + (2·*L* − 2) (in RVA 0xdd810): the right-hand
number **omits the mana-cube points**. A character who has spent cube points therefore sees
the left number exceed the right (for example "Points: 19/18").

### 3.3 Limits

* **No maximum level** per skill exists in the window: a skill can be raised as long as
  *U* < *A*.
* **Prerequisite:** a child can be raised only when its parent's draft level is **≥ 5**
  (1 ← 0, 3 ← 2, 5 ← 4, 7 ← 6, 8 ← 7, 9 ← 8).
* **Floor:** a lowered draft level below 0 is set back to 0.

---

## 4. Spending: mouse rules

All presses go through the mouse-press routine **RVA 0x7b600** (argument 0 = left button,
1 = right button). While G+0x8008f1 is set it handles, in this order: the specialization
buttons (left only), the skill buttons (any button; argument 0 raises, any non-zero argument
lowers), and later, on the left-button path only (the branch at RVA 0x7baab), Learn then
Cancel. Every button whose hit test succeeds is handled.

### 4.1 Left click on specialization button *s* (RVA 0x7b85a … 0x7b8a5)

Draft specialization := *s*; sound **0x55**. Always allowed (any level, any points, even if it
already was *s*; the sound plays anyway). It only changes the draft; §4.4 gives the cost.

### 4.2 Left click on skill button *i* — raise (RVA 0x7b8b0 … 0x7b9dd)

1. If *U* ≥ *A* → nothing (no sound).
2. If *i* is a child (1, 3, 5, 7, 8, 9) and its parent's draft level is < 5 → nothing.
3. Otherwise draft level *i* += 1; sound **0x55**.

### 4.3 Other button on skill button *i* — lower (RVA 0x7b9e2 … 0x7ba9e)

1. Let *n* = draft level *i*. If *n* ≤ 5 and *i* is a parent (6 → child 7, 7 → 8, 8 → 9,
   0 → 1, 2 → 3, 4 → 5) whose child's draft level is > 0 → nothing (no sound). (Lowering a
   parent from 6 or more is always allowed, since it stays ≥ 5.)
2. Otherwise draft level *i* := *n* − 1; sound **0x58**; if the result is < 0 it is set to 0
   (so lowering a 0 still plays the sound).

Lowering below the player's **committed** level is allowed; it is what costs money (§4.4).

### 4.4 Cost of a change (RVA 0xdf9c0)

With the committed values (player) and the draft:

**cost = trunc( Σ over *i* with draft*ᵢ* < player*ᵢ* of *F*(*L*) · (player*ᵢ* − draft*ᵢ*) · 10
  + [draft spec ≠ player spec and *L* > 1] · *F*(*L*) · 100 )** copper,

accumulated in single precision in skill order 0 … 10 (each term computed as *F* times the
difference, times 10, then added to the running sum), then the specialization term, then
truncated toward zero. **Raising** skills is free. **Lowering** committed levels (a re-skill)
costs 10·*F*(*L*) per point. **Changing specialization** costs 100·*F*(*L*), and is **free at
level 1**. Changing back costs the same.

### 4.5 The class-trainer test (RVA 0x7f030)

Needed only when cost > 0. True when some creature in the game's creature map (G+0x2e8) has
**byte +0x7e bit 0x40** set, the **same class** (+0x140) as the player, and a squared
distance to the player below **16**, where each axis difference of the i64 positions
(+0x10, +0x18, +0x20) is converted to float and multiplied by 1/65536 (i.e. within 4 blocks).
What that bit marks is not established (§9 U-4).

### 4.6 Learn and Cancel (left button, RVA 0x7c806 … 0x7c8e4)

The two buttons exist only while the draft differs from the player (any of the 11 levels or
the specialization).

* **Learn** is **allowed** when: something differs, cost ≤ money (+0x1304), and (cost ≤ 0 or
  the trainer test passes) (RVA 0xdf880). Hit area (widget-local): 0 < *x* < *w*/2 and
  *h* − 30 < *y* < *h*. A left press inside it while allowed: cost is computed again, money
  −= cost; the player's 11 skill levels := the draft; the player's specialization := the
  draft specialization. **No sound.**
* **Cancel** (RVA 0xdf760, tested after Learn): hit area *w*/2 < *x* < *w*,
  *h* − 30 < *y* < *h*. A left press inside it while something differs: draft := the
  player's levels and specialization. No sound.

After Learn or Cancel nothing differs, so both buttons disappear on the next draw.

### 4.7 Effect on the game

Skill effects and tooltips evaluated with a negative level read the **player's** committed
levels (+0x1138 …), not the draft. One skill level serves every ability mapped to that slot
(§0): for example a ranger's aim and swiftness both use skill 8, and a mage's fire explosion
and healing stream both use skill 6. Ability-bar behaviour: `alpha-creatures-behaviour.md`
§16.2.

---

## 5. Texts

### 5.1 Window texts (SkillWidget draw RVA 0xdd810; widget-local)

*w* and *h* are the widget's width and height (300 × 460 when sized to the panel, §9 U-5).

| Text | When | Origin | Size, stroke | Align | Fill |
|---|---|---|---|---|---|
| "Skills" | always | (15, 25) | 12 px, 3 | left | white |
| "Points: " + *U* + "/" + (2·*L* − 2) | always | (15, 45) | 10 px, 2 | left | white |
| "Not enough money." | cost > 0 and cost > money | (15, trunc(*h* − 65)) = (15, 395) | 10 px, 2 | left | (1, 0.25, 0.25, 1) |
| "Requires class trainer." | cost > 0, cost ≤ money, trainer test fails | (15, 395) | 10 px, 2 | left | (1, 0.25, 0.25, 1) |
| (cost mod 100) + " C " | cost > 0 | (220, 415) | 10 px, 2 | right | (0.8, 0.5, 0, 1) |
| ((cost / 100) mod 100) + " S " | cost > 0 | (180, 415) | 10 px, 2 | right | (0.7, 0.7, 0.7, 1) |
| ((cost / 100) / 100) + " G " | cost > 0 | (140, 415) | 10 px, 2 | right | (1, 0.9, 0, 1) |
| "COST:" | cost > 0 | (15, 415) | 10 px, 2 | left | white |
| "Learn" | draft ≠ player | (*w*/3, *h* − 20) = (100, 440) | 12 px, 3 | centred | (0.5, 0.5, 0.5, 1) when not allowed; else **(0, 1, 1, 1)** while the Learn press test succeeds, white otherwise |
| "Cancel" | draft ≠ player | (2*w*/3, *h* − 20) = (200, 440) | 12 px, 3 | centred | (0, 1, 1, 1) while the Cancel press test succeeds, else white |

The cost-row y is the warning y + 20. Rows are drawn in the order listed. The three coin
parts are all drawn (zeros included) whenever cost > 0 (1 silver = 100 copper, 1 gold =
10 000 copper).

There is **no "x/y" level text**: each skill button shows only its draft level (§2.4), and
there are no skill-name labels in the window; names appear only in the tooltip.

### 5.2 Tooltip box

The ordinary preview tooltip (`alpha-ui-screens.md` §8.1.7; preview draw RVA 0xd50a0) with
preview +0x160 = 0. When +0x170 ≥ 0 it renders the specialization text (RVA 0xa62c0); when
+0x168 ≥ 0 the skill text (RVA 0xa5710). Both start at (trunc(*X*₀ + 14), trunc(*Y*₀ + 25))
from the box's anchor point and use a **wrap width of 275**. Box placement: §9 U-6.

### 5.3 Tooltip text

* **Specialization button** (RVA 0xa62c0): the dictionary entry
  `specialization:<class>:<spec>` with <class> = `warrior`, `ranger`, `mage`, `rogue` (player
  class 1–4) and <spec> = class 1: 0 `berserker`, 1 `guardian`; class 2: 0 `sniper`, 1
  `scout`; class 3: 0 `fire`, 1 `water`; class 4: 0 `assassin`, 1 `ninja`. The variable map
  is empty. Any other class or spec renders nothing.
* **Skill button *i*** at draft level *n* (RVA 0xa5710), with *Name* = the ability key
  (*i* = 6, 7, 8; empty for a class outside 1 … 4) or the skill-map key (other *i*):
  1. variables := V(*i*, *n*) (§5.4); render `description:`*Name*;
  2. if *n* > 0: render `skill:level`, then `details:`*Name*;
  3. variables := V(*i*, *n* + 1); render `skill:nextlevel`, then `details:`*Name*.

  Keys missing from the dictionary render nothing (the shipped dictionary has no entries for
  `SkillAbility4` or `SkillAbility5`). In the shipped dictionary `skill:level` and
  `skill:nextlevel` use `%` page breaks around a short heading (`@level` in the first).

Rendering uses the dictionary page renderer of `alpha-talk-vendors.md` §2. Each **page**
becomes one paragraph laid out by this word layout (identical in RVA 0xa5710 and 0xa62c0):

1. Size *s* = **12 px for the first page**, **10 px for every later page**. Pen *x* = left
   edge *x*ₗ; a "no space" flag is set.
2. For each word (segment), with measured width *d* (truncated to an integer):
   * it is **punctuation-like** when it is exactly ".", "-", ",", ";", "!", "?", ":", "/"
     or ")";
   * unless the flag is set, advance *x* by **3** before a punctuation-like word, else by
     *s*;
   * the flag := (the word is exactly "-");
   * if the word is not punctuation-like and *x* + *d* > *x*ₗ + 275: new line, *y* += *s* + 4,
     *x* := *x*ₗ;
   * draw it at (*x*, *y*): outline pass stroke 3, then the fill pass in the segment colour;
   * *x* += *d*; if the word is exactly "/" or "(" then *x* −= 9.
3. After the page: *y* += **18**. (Empty pages still advance 18 each.)

### 5.4 Tooltip variables V(*i*, *n*) (RVA 0x78800)

Every call sets all of these, in the order listed (the dictionary picks the ones it uses);
*i* does not affect any value. R(*x*) = trunc(*x*·100 + 0.5)·0.01 for *x* ≥ 0; for *x* < 0 it
is −R(−*x*), with −0 shown as 0 (RVA 0x874a0). Numbers are written with the default
wide-stream float format (6 significant digits, no trailing zeros: 17.67, 5, 0.91). length(id,
*n*) is the ability length routine RVA 0x3e6a0 at level *n* (ids without an entry give 0).
"%" means the text gets a "%" appended.

| Variable | Value |
|---|---|
| `@level` | *n* |
| `@health` | R(50·*v*(*n*)) % |
| `@ridingspeed` | R(100·(1 + *v*(*n*))) % for *n* ≥ 1, **0 % for *n* < 1** (RVA 0x46aa0) |
| `@climbingpower` | R(100·*k*(*n*)) % |
| `@flyingspeed` | R(100·(3·*v*(*n*) + 1.5)) %, 0 % for *n* < 1 (RVA 0x3f770) |
| `@swimmingspeed` | R(100·(0.5·*v*(*n*) + 0.5)) %, 50 % for *n* < 1 (RVA 0x476a0) |
| `@boatspeed` | R(100·(3·*v*(*n*) + 1)) %, 0 % for *n* < 1 (RVA 0x3e2c0) |
| `@smashcooldown` (54), `@cyclonecooldown` (86) | R(length(id, *n*) / 1000) |
| `@cycloneduration` | R(5000 / 1000) = 5 (RVA 0x47310 gives 5000 for id 86) |
| `@bulwarkcooldown` (101) | R(length / 1000) |
| `@bulwarkamount` | R(100·(0.3·*v*(101, *n*) + 0.25)) % |
| `@bulwarkduration` | the fixed text "10" |
| `@warfrenzycooldown` (102) | R(length / 1000) |
| `@warfrenzyduration` | the fixed text "10" |
| `@warfrenzyamount` | R(100·(9·*v*(102, *n*) + 1)) % (constant 9.0 at RVA 0x302a90) |
| `@rangerkickcooldown` (21) | R(length / 1000) |
| `@rangerkickknockback` | R(100·*v*(*n*)) % |
| `@retreatcooldown` (50) | R(length / 1000) |
| `@retreatdistance` | R(100·*v*(*n*)) % |
| `@aimcooldown` (99) | R(length / 1000) |
| `@aimstealth` | R(100·*v*(*n*)) % |
| `@swiftnesscooldown` (100) | R(length / 1000) |
| `@swiftnessamount` | R(100·*v*(*n*)) % |
| `@swiftnessduration` | the fixed text "10" |
| `@fireexplosioncooldown` (88) | R(length / 1000) |
| `@fireexplosionknockback` | R(100·*v*(*n*)) % |
| `@manashieldduration` | the fixed text "30" |
| `@manashieldcooldown` (103) | R(length / 1000) |
| `@manashieldpower` | R(100·*v*(103, *n*)) % |
| `@teleportcooldown` (49) | R(length / 1000) |
| `@healingstreamcost` | R(100·8·cost(34, *n*)), no %; cost(34, *n*) = (1 − 0.75·*v*(34, *n*))·0.125 (RVA 0x44ae0), so the value is R(100·(1 − 0.75·*v*)) |
| `@interceptcooldown` (48), `@shurikencooldown` (96), `@camouflagecooldown` (97) | R(length / 1000) |
| `@camouflageduration` | R((12000·*v*(97) + 8000) / 1000) with *v*(97) at the player's **committed** skill-8 level (level −1), not *n* — so the "next level" page shows the current duration |
| `@sneakcooldown` (79) | R(length(79, *n*) / 1000); the length routine has no entry for 79, so the text is "0" |
| `@sneakspeed` | R(100·0.4·*v*(79, *n*)) % |
| `@sneakstealth` | R(100·*v*(79, *n*)) % |

Cooldown lengths used by the tooltip (RVA 0x3e6a0, *v* = *v*(id, *n*), result truncated to an
integer): 21 and 88: 8000 − 8000·*v*; 48: 20000 − 12000·*v*; 49 and 50: 16000 −
10000·*v*; 54 and 96: 20000 − 14000·*v*; 86: 60000 − 40000·*v*; 97, 100, 101 and 102:
60000 − 30000·*v*; 99: 12000 − 10000·*v*; 103: 20000 − 10000·*v*; 72: the constant 15000;
every other id: 0 (switch at RVA 0x3e6a6; see also `alpha-creatures-behaviour.md` §16.5).

---

## 6. Record and saves

* **Nothing is sent by the window.** Learn writes only the local player's creature. The skill
  array (record offset 0x1128, 11 × i32) and the mana-cube counter (record offset 0x1154,
  i32) are the last two fields of the creature-record serializer (RVA 0xccfa0) and its reader
  (RVA 0xcd3e0); the specialization byte is record offset 0x131. Field numbering and update
  transport: `alpha-net-packets.md` §2.1. Money (+0x1304) is outside the record. The draft is
  local.
* **Saves** (`alpha-saves.md` §3.2): the save writer (RVA 0x4d790) writes the specialization
  byte (+0x141) after the class byte, money (+0x1304) later, then the mana-cube counter
  (+0x1164), the count 11 and the 44 bytes of +0x1138; the reader (RVA 0x4be40) restores the
  skill array only when the stored count is 11. The draft is not saved.

---

## 7. Sounds

| Id | When |
|---|---|
| 0x55 `menu-select.wav` | a specialization button is clicked; a skill is raised |
| 0x58 `menu-drop-item.wav` | a lower click on a skill passes the parent test (also at 0) |
| 0x53 / 0x54 | "a window holds the input" changes to yes / to no (§1.3) |
| — | Learn, Cancel, refused clicks: silent |

---

## 8. Worked examples

All at 1280 × 720 with the widget sized 300 × 460: panel (130, 75) … (430, 535);
*x*ₐ = 180, *y*ₐ = 145. Specialization buttons at (180, 145) and (250, 145). Skill buttons:
6 (180, 225), 7 (235, 225), 8 (290, 225), 9 (345, 225, never drawn); 0 (180, 285),
1 (235, 285); 2 (180, 345), 3 (235, 345); 4 (180, 405), 5 (235, 405). Title baseline
(145, 100); Points (145, 120); warning (145, 470); cost row y = 490 (gold right edge 270,
silver 310, copper 350); Learn centred on (230, 515), Cancel on (330, 515); Learn hit band
x 130 … 280, y 505 … 535; Cancel x 280 … 430, y 505 … 535.

### 8.1 A level-10 warrior spending points

Warrior (class 1), specialization 0, *L* = 10, *c* = 5, all skills 0, money 500 copper.
*A* = 2·10 − 2 + 5/4 = **19**.

1. **X**: draft := all 0, spec 0; sound 0x53 (nothing else open). "Points: 0/18". Skill 6
   shows the smash icon, 7 cyclone (field +0x544 = 0: locked), 8 warfrenzy (locked), 9 not
   drawn, 0–5 their skill icons (1, 3, 5 locked). Spec button 0 (`warrior-berserker.png`) with
   +0x544 = 1, button 1 (`warrior-guardian.png`) with 0.
2. Left click on cyclone: skill 6 = 0 < 5 → nothing, silent.
3. Left click on smash five times: draft 6 = 1 … 5, five 0x55 sounds. Cyclone unlocks.
4. Cyclone ×5 → 5 (warfrenzy unlocks); warfrenzy ×5 → 5 (skill 9 unlocks, still undrawn);
   climbing ×4 → 4. *U* = 19 = *A*.
5. Left click on pet taming: *U* ≥ *A* → refused, silent. The line reads **"Points: 19/18"**.
6. Learn and Cancel are drawn; cost = 0 (only raises), so Learn is allowed and no COST row or
   warning is drawn. Left click at (200, 520) → Learn: the player's skills := draft, money
   unchanged, silent.
7. Right click on smash (draft 5): 5 ≤ 5 and cyclone's draft is 5 > 0 → refused, silent.
8. Right click on warfrenzy (5): skill 9 is 0 → draft 8 := 4, sound 0x58. Draft 8 (4) <
   committed (5): cost = trunc(*F*(10)·1·10) = trunc(19.066427) = **19**. Drawn: "COST:" at
   (145, 490), "0 G " right edge 270, "0 S " right edge 310, "19 C " right edge 350.
   * No same-class creature with bit 0x40 within 4 blocks: "Requires class trainer." at
     (145, 470) and Learn grey; clicking it does nothing.
   * With one: no warning; Learn → money 500 → 481, skill 8 = 4.
9. Clicking specialization 1 before Learn instead: cost = trunc(19.066427 + 190.66427) =
   **209** → "0 G ", "2 S ", "9 C "; skill 8's icon and tooltip switch to bulwark. Cancel
   restores the committed values and spec 0.

Tooltip check, hovering smash at draft level 2: `description:AbilitySmash`, then (*n* = 2 > 0)
`skill:level` with `@level` = 2 and `details:AbilitySmash` with `@smashcooldown` =
R(17666 / 1000) = **17.67** (length(54, 2) = trunc(20000 − 14000·0.1666667) = 17666); then
`skill:nextlevel` and the details again with `@level` = 3 and `@smashcooldown` =
R(16769 / 1000) = **16.77** (length(54, 3) = trunc(20000 − 14000·0.2307692) = 16769).

### 8.2 A ranger choosing Sniper

Ranger (class 2), committed specialization 1, *L* = 6, *c* = 0, committed skills 6 → 3,
7 → 2, 8 → 1, money 1000 copper. *A* = 10; "Points: 6/10".

1. **X**: spec button 0 shows `ranger-sniper.png` with +0x544 = 0; button 1
   `ranger-scout.png` with 1. Skill 8 shows swiftness (100).
2. Hovering spec button 0: `frame` colour (0, 1, 1, 1); tooltip
   `specialization:ranger:sniper`, first page 12 px, later pages 10 px.
3. Left click on it: draft spec := 0, sound 0x55. Button 0 gets +0x544 = 1, button 1 gets 0;
   skill 8 now shows aim (99) (tooltip `AbilityAim`, variables at level 1).
4. Draft differs → Learn and Cancel. Cost = trunc(100·*F*(6)) = trunc(151.57166) = **151**:
   "0 G ", "1 S ", "51 C ". It needs the trainer test; with it, Learn → money 849, player
   spec := 0. Skill 8 = 1 carries over to aim.
5. Switching back later costs the same 151 at level 6. At **level 1** the change is free
   (*L* > 1 fails), needs no trainer test, and Learn is allowed whenever something differs.

---

## 9. Unresolved

* **U-1** The hit test RVA 0x2294c0 was not shown to ignore hidden nodes. If it does not,
  the never-drawn skill-9 button at (*x*ₐ + 165, *y*ₐ + 80) and the skill-10 button at its
  template position can still be raised and lowered by clicks.
* **U-2** Template geometry from `gui.plx` (inherited readings, not re-confirmed): skill
  buttons 42 × 42 with 3-px notched corners, 4-px black stroke and an inset 2-px white
  `frame`; specialization button body 60 × 60 with a 42 × 42 widget; level text child `info`
  at (43.8, 43.7), 10 px, right-aligned, outline colour (0, 0.763, 0.814) with a 2.5-px black
  outline; child-button link polygon `node` spanning −15 … +3 × +12 … +31; the hit area of
  each button.
* **U-3** Whether a button's position (§2.3) is its body's top-left corner, and whether the
  panel host position is its top-left corner.
* **U-4** What creature byte +0x7e bit 0x40 marks (inherited reading: class trainers) and
  which spawns set it.
* **U-5** The widget is bound to the panel node with resizing (RVA 0x231460); that its
  rectangle is exactly (0, 0) … (300, 460) was not re-proven. All widget-local text and hit
  positions scale with it.
* **U-6** Tooltip box placement for skill hovers (inherited reading: mouse + (20, 20), the
  window-source rule) and whether rarity stars or a model are drawn with preview +0x160 = 0.
* **U-7** The identities of the nodes G+0x800874 (inherited reading: start-screen root) and
  G+0x800ad4 (inherited reading: blueprint preview), and that the per-frame "holds the input"
  virtual call on *G* is RVA 0x7f1d0.
* **U-9** RVA 0x3ed60 substitutes a level when the requested level is 0 and creature byte
  +0x60 is set; whether that byte can be set on the local player (which would change tooltip
  values at level 0) was not established.
* **U-10** Whether a new character starts with all skills 0 and *c* = 0.
* **U-11** Whether the server checks skill levels or the point budget (no check was found in
  `Server.exe` in the inherited reading; not re-examined).

---

## Re-certification (2026-11-15)

**Confirmed against `Cube.exe`:** budget formula and order of the raise tests; prerequisite
pairs and the parent-lowering rule; floor at 0 and sound on lowering a 0; specialization click
always allowed with sound 0x55; cost formula, its single-precision accumulation order and
truncation; *F* formula and values; trainer test (bit 0x40 of +0x7e, same class, squared
distance < 16 with 1/65536 scaling); Learn/Cancel conditions, hit bands, silent commit,
left-button-only path; all window texts, origins, sizes, strokes, alignments and colours;
coin split; panel size 300 × 460, placement formula, inventory 400 × 285 and crafting width
350; open-time draft copy; per-frame visibility; button positions and offsets; icon tables
and keys; ability choice per class and draft specialization; lock rules; hover colours and
preview fields; tooltip key sequence, page sizes 12/10, spacing 3 or *s*, line step *s* + 4,
paragraph step 18, wrap 275, the −9 rule; every tooltip variable formula, constant, suffix
and fixed text; R rounding; worked-example numbers; save order and the count-11 check; record
offsets 0x1128, 0x1154, 0x131; sound ids and file names.

**Corrected (old → new):**
1. All addresses were image virtual addresses → now RVAs in `Cube.exe` (image base 0x400000).
2. Skill 9 was "shown with no icon" → its button is hidden again after the per-frame loop and
   is never drawn (RVA 0x8eaef); §2.1, §2.3, §2.4, §8.1 and U-1 updated.
3. `@ridingspeed` was R(100·(1 + *v*)) at every level → 0 % for *n* < 1 (tooltip number).
4. Word-layout tests for ":", "/", ")" were "contains" (believed) → exact equality; the old
   open point is closed (reversal of a believed reading).
5. `@sneakcooldown`: added that the length routine has no entry for id 79, so it shows 0.
6. Start-screen close: the hidden nodes are G+0x8008bc, G+0x800900 and G+0x8008dc (text was
   ambiguous).
7. Right-button close: added the hidden nodes G+0x8008bc, G+0x8008c0, G+0x8008dc and the
   interface-query condition (RVA 0x250ae0).
8. Movement close: "when the intent becomes non-zero" → on every input pass while the vector
   is non-zero; the listed nodes and both bytes G+0x8008f1/G+0x8008f2 are cleared.
9. Open/close sound: added the G+0x800a40 alternative to the "holds the input" answer.
10. The skill tooltip name for skills 9 and 10 comes from the skill-name map; added that
    skills 6–8 also have skill-map keys (`SkillAbility1`–`3`) which the window does not use.
11. Line-wrap compares the integer-truncated word width.
12. Added the per-id cooldown length constants used by the tooltip (RVA 0x3e6a0), including
    60000 − 30000·*v* for ids 97, 100, 101 and 102.

**Removed:** the parity remark on the Points line; capture-confirmation instructions in the
open points; the advice on how dictionary texts are obtained; the unre-verified statements
that the ability bar does not fire while the window is open and that a bar slot appears
after Learn (companion pointer kept instead).

**Moved to unresolved:** template geometry and hit areas (U-2), pivot assumptions (U-3),
trainer meaning (U-4), widget size binding (U-5), tooltip placement and star/model hiding
(U-6), node identities (U-7), new-character zero values (U-10), server validation (U-11);
added U-9. (U-8 is unused.)
