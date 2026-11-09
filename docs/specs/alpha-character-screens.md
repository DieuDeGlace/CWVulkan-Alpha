# Alpha interface: character select, character creation and world select

Behavioural specification of the screens between the start menu's "Start Game" and entering a
world in the Cube World **Alpha** client (`Cube.exe`): the character list, the character
creation screen, the world list, the world creation screen and the connect screen, how they
are laid out, how they react to the mouse, what they write to the saves and how the player
moves between them. This resolves follow-up **F-1** of `alpha-ui-screens.md` §12.

It assumes, and does not repeat:

* `alpha-ui-screens.md` §2 (layout model, pivot rule), §3.2–§3.4 (start menu, text origin
  rules), §3.5–§3.6 (string list, first sketch of these screens), §6.3 (arrow templates);
* `alpha-ui-plasma.md` (the `.plx` format, the 9-slice resize, cloning);
* `alpha-creatures-models.md` §4 (character choice record → creature type and appearance);
* `alpha-saves.md` §3–§4 (`characters.db`, `worlds.db` records);
* `alpha-crafting.md` §2.2 (recipe grants at creation);
* `alpha-physics-camera.md` (camera fields G+0x1a4 … G+0x1c0 and their easing).

Conventions as in `alpha-ui-screens.md`: "G" is the game object, W × H the window client size
(G+0x11c, G+0x120). "P" is the local player creature (G+0x8006d0). Positions are window pixels,
x right, y down. A "widget position" is the top-left corner of a widget's rectangle in its
parent's space (set by the library routine 0x62a650; read back with 0x62f630 = x,
0x62f660 = y, 0x62f600 = width, 0x6291d0 = height). Text origins are first baselines (§3.4 of
the screens spec). dt is the frame time in milliseconds (the update routine's argument).
Routines are cited by address only. *Believed* marks a reading that is likely but not proven.

---

## 1. The screens and how they are switched

### 1.1 Containers

All five screens are empty container nodes made at start-up (0x459c40, 0x45a7e8 … 0x45aa0c),
each a direct child of the window's root node (the interface manager G+0x800710, its node
+0xb4) and each **hidden** at creation. They are siblings of the start-screen root (G+0x800874,
which holds `start.plx` and the start menu) and of the in-game root (G+0x800884).

| Container | Screen | Holds |
|---|---|---|
| G+0x800888 | **character select** | one 600 × 220 panel per saved character plus a "New character" panel; Select, small "x" and Delete Character buttons |
| G+0x800880 | **character creation** | the style panel (300 × 330), the name label and edit, the Create Character button |
| G+0x80088c | **world select** | one 600 × 220 panel per listed world plus a "New world" panel; Select, small and Delete World buttons |
| G+0x800890 | **world creation** | the name and seed labels and edits, the Create World button |
| G+0x800894 | **connect** | the server-address label and edit |

Four buttons are not inside any screen container: they are children of the window root and
their visibility is recomputed every frame (§1.3): **Back** (G+0x80099c),
**Multiplayer Worlds...** (G+0x8009a0), **Connect to server** (G+0x8009a4) and **Connect**
(G+0x8009a8).

### 1.2 Transitions

Exactly one of the start-screen root and the five containers is visible at a time while the
player is outside a world. The only code that changes their visibility is listed here.

| From | Action | Effect | Routine |
|---|---|---|---|
| start screen | "Start Game" (start menu item 0, mouse press) | hide start-screen root, show **character select** | 0x47b6e3 in 0x47b600 |
| character select | **Select** on a saved character | hide character select; load that character into P (0x4806c0); show **world select** | 0x483e70 |
| character select | **Create** (Select on the "New character" panel) | hide character select; reset P and the creation widget (§3.2); set the camera (§7); show **character creation** | 0x483e70 |
| character creation | **Create Character** | hide creation; save the new character (§3.8); show **world select** | 0x4821a0 |
| world select | **Select** on a world | hide world select; set the camera (§7); start that world (0x46f620 with its seed and name) | 0x484170 |
| world select | **Create** (Select on the "New world" panel) | hide world select; set the camera; show **world creation** | 0x484170 |
| world creation | **Create World** | hide world creation; set the camera; append and save the world; start it (§5.4) | 0x482530 |
| world select | **Connect to server** | show **connect**, hide world select, clear the address field (*believed*) | 0x481fe0 |
| connect | **Connect** | if the address text is not empty: hide connect and connect to it (`alpha-net-transport.md`); empty: nothing | 0x4815e0 |
| any | **Back** | see the Back table below | 0x4814f0 |

**Back** (0x4814f0) tests the containers in this order and acts on the first visible one:

| Visible screen | Back hides it and shows |
|---|---|
| world creation | world select |
| world select | character select |
| character creation | character select |
| character select | the start screen (start-screen root) |
| connect | world select |

Back saves nothing and undoes nothing. Leaving character creation with Back keeps the
changes made to P; they are overwritten the next time a character is selected or created.

**Keyboard.** While any of the five containers is visible the game's key handler (0x47e1b0)
ignores every key (its first test, 0x47e20d), so **Esc does not go back**, and Enter does not
press a button (*believed*: no other key path was found). Typed characters still reach the
interface and go to the focused edit (`alpha-ui-plasma.md` §6.6). The "window open" test
0x47f1d0 includes all five containers and the start-screen root, so the cursor is free.

### 1.3 Per-frame visibility (update routine 0x488ee0, at 0x489529 … 0x4896a4)

Every frame, before anything else in this spec is updated:

* **Back** is visible ⇔ any of character select, character creation, world select, world
  creation or connect is visible.
* **Multiplayer Worlds...** and **Connect to server** are visible ⇔ world select is visible.
* **Connect** is visible ⇔ connect is visible.
* The in-game root (G+0x800884) is visible only when the start-screen root and all five
  containers (and the "Please wait..." label) are hidden (0x4ae2f5 … 0x4ae356).
* The message label G+0x8009f4 ("There is already ..." messages, §3.7, §5.3) is hidden; the
  creation screens may show it again later in the same frame.

---

## 2. Shared pieces

### 2.1 Big buttons (`button` clones)

Every big button is a clone of the `gui.plx` template `button` (looked up at 0x45c8f0, kept in
a local), resized with the widget-stretching rule (`alpha-ui-plasma.md` §5.5), its caption set
on its `text` nodes (0x636ad0), and a callback attached to its click event (0x4576f0, event
number 3; *believed* "released over the button", compare the press event 2 used by the arrows).

Template facts (from `gui.plx`): a 100 × 100 bind rectangle; a 12-corner notched body filled
black at alpha 0.67 with a stroke, an inner outline `Shape`, a `text` widget holding the
caption text shape, and a `clip`/`Shape` 160 × 30 highlight that lies outside its own mask.
No `button:*` sequences are in the template, so there is no hover or press look (S-4). No
sound is played by the code of any of these buttons.

| Button | Template | Size | Parent | Caption(s) |
|---|---|---|---|---|
| Create Character | `button` | 300 × 40 | character creation | "Create Character" |
| Create World | `button` | 300 × 40 | world creation | "Create World" |
| Back | `button` | 200 × 40 | window root | "Back" |
| Multiplayer Worlds... | `button` | 300 × 40 | window root | "Multiplayer Worlds..." / "Singleplayer worlds..." / "Multiplayer worlds..." (§4.4) |
| Connect to server | `button` | 300 × 40 | window root | "Connect to server" |
| Connect | `button` | 200 × 40 | window root | "Connect" |
| character Select | `button2` | 200 × 30 | character select | "Select" or "Create" (§3.5) |
| character small button | `smallbutton` | 20 × 20 | character select | template text (not set by code; open point C-3) |
| Delete Character | `button` | 300 × 30 | character select | "Delete Character" |
| world Select | `button2` | 200 × 30 | world select | "Select" or "Create" |
| world small button | `smallbutton` | 20 × 20 | world select | template text |
| Delete World | `button` | 300 × 30 | world select | "Delete World" |

(Template choice from the locals loaded before each clone call: `button` 0x45c8fd, `button2`
0x45c9bd, `smallbutton` 0x45ca7d. Construction 0x45cb85 … 0x45cf30 and 0x465628 … 0x465a00.)

*Correction to `alpha-ui-screens.md` §3.5:* "Multiplayer Worlds..." and "Connect to server"
are 300 × 40; the second call with (40, 40) at construction is an initial **position**
(0x62a650), not a size, and is overwritten by the layout routine.

### 2.2 Placement of the fixed buttons (layout routine 0x482a40, 0x482e71 … 0x4832a6)

Run at start-up and on every window resize. Top-left corners:

| Button | Top-left |
|---|---|
| Back | (20, 20) |
| Multiplayer Worlds... | (W − 20 − 300, 20) |
| Connect to server | (W − 20 − 300, H − 20 − 40) |
| Create Character | (W − 20 − 300, H − 20 − 40) |
| Create World | (W − 20 − 300, H − 20 − 40) |
| Connect | ((W − 200)/2, (H − 40)/2) |

### 2.3 Labelled edits

Each text field is built the same way (0x45cf35 … 0x45d2ce for the name; the world name, seed
and server address repeat it):

1. A **label node** is made in the screen container and named after its menu key
   (`charactername`, `worldName`, `worldSeed`, `serverName`). Its text shape: `resource1.dat`,
   **12 px**, white (1, 1, 1, 1), black outline (0, 0, 0, 1) of **3 px**, **centred** on the
   origin (flags 1). A second, plain copy of the shape (same size, no outline) is a child of
   the label node, so the label is drawn outlined then filled (as all code text).
2. A clone of the `edit` template is made as a child of the label node, its text cleared, and
   its widget placed at **(−w/2, +10)** relative to the label origin, where w = 125 is the edit
   template's width (`gui.plx`: `edit` widget 125 × 20). So the box is centred under the
   label, its top 10 px below the label's baseline.
3. The layout routine puts the label node's origin (its pivot is (0, 0)) on its anchor:

| Field | Label text | Label origin | Edit box (top-left … bottom-right) |
|---|---|---|---|
| character name | "Name" | (W/2, H − 50) | (W/2 − 62.5, H − 40) … (W/2 + 62.5, H − 20) |
| world name | "Name" | (W/2, H − 50) | same as above (other screen) |
| world seed | "Seed (must be a number)" | (W/2, H − 100) | (W/2 − 62.5, H − 90) … (W/2 + 62.5, H − 70) |
| server address | "Server address" | (W/2, H/2 − 100) | (W/2 − 62.5, H/2 − 90) … (W/2 + 62.5, H/2 − 70) |

W/2 and H/2 are integer halves (C division) in the layout routine. The edit has no
character filter and no length limit set by the game code (*believed*; the library's `Edit`
widget was not traced, open point C-1); the screens validate the text instead (§3.7, §5.3).

### 2.4 The message labels

* **G+0x8009f4** ("info" label, made at 0x45b84b): `resource1.dat` 12 px, white with a 3-px
  black outline, centred, origin (W/2, H − 200). Used for "There is already a character with
  that name." and "There is already a world with that name.".
* **G+0x8009f8** (red error label, `alpha-ui-screens.md` §2): connection messages only.

### 2.5 The list panels (both lists)

A list entry is a clone of the `blackwidget` template (G+0x8008c8; look in
`alpha-ui-screens.md` §3.3: 5-px notched corners, black fill alpha 0.67, 3-px black stroke,
2-px white inner outline) made inside the list's container, resized to **600 × 220**, with an
entry widget bound to it so the widget covers the panel (0x631460, "fit"):
CharacterPreviewWidget (0x424e80, draw 0x425450) for characters, WorldPreviewWidget
(0x605a20, draw 0x605ae0) for worlds. The widget's record pointer (+0x160) is the character
or world entry, or **null for the trailing "New ..." panel**.

---

## 3. Character select and character creation

### 3.1 The character list (builder 0x49d650)

Built at start-up (after `characters.db` is read; 0x4657bf) and rebuilt after every create or
delete. Steps:

1. Detach the Select, small and Delete buttons; delete every existing panel of the container
   (0x632870); empty the panel list G+0x800978.
2. For *i* = 0 … *N* (*N* = number of characters, list G+0x800984; so *N* + 1 panels):
   make a panel (§2.5) whose record is character *i*, or null when *i* = *N*; set its widget
   position to **(300, 50 + 230·i)** (the frame recentres x at once, §3.3); append it.
3. Re-attach the three buttons to the container (they draw on top of the panels).

The list is in `characters.db` key order 0 … *N*−1, the "New character" panel always last.

### 3.2 What a panel shows (CharacterPreviewWidget draw 0x425450)

All text: `resource1.dat`, white with a 3-px black outline (outline pass, then fill pass).
Panel-local coordinates.

**"New character" panel** (record null): the text "New character", **14 px**, origin
(300, 30) (= panel width / 2, truncated), flags 5 (centred horizontally; block centred
vertically on the origin, *believed*), wrap width 180. Nothing else.

**Character panel:**

| Line | Origin | Size | Flags | Text |
|---|---|---|---|---|
| name | (200, 30) | 14 | 0x10 (word-wrap at 180) | the character's name (record +0x1168) |
| level line | (200, 55) | 12 | 0 (left) | "LVL " *level* " " *race* " " *class* |
| specialization | (200, 75) | 10 | 0 | "Specialization: " *name* |

* *race* from the creature type (record +0x64): 0, 1 "Elf"; 2, 3 "Human"; 4, 5 "Goblin";
  7, 8 "Lizardman"; 9, 10 "Dwarf"; 11, 12 "Orc"; 15, 16 "Undead"; **any other type (frogmen
  13, 14) adds nothing**, leaving two spaces ("LVL 1  Warrior").
* *class* (record +0x140): 1 "Warrior", 2 "Ranger", 3 "Mage", 4 "Rogue"; other values add
  nothing.
* *specialization* (record +0x141, 0 or 1) by class: Warrior "Berserker" / "Guardian",
  Ranger "Sniper" / "Scout", Mage "Fire Mage" / "Water Mage", Rogue "Assassin" / "Ninja".
  Other values add nothing after "Specialization: ".
* The level is printed as a plain decimal integer (record +0x190).

**The model.** The character is drawn in the panel with its own projection (0x425450 …
0x4128f0 call): vertical field of view 45° (tan 22.5°), near 0.1, far 1000 (projection
terms 1.0001 and −0.10001), aspect W/H; the projection is shifted so that the model's origin
lands on the screen point **(panel x + 100, panel y + 110)**. The view looks at the model from
17 blocks, turned by the fixed angles −1.658 rad and 0.785 rad (*believed* pitch ≈ −95° and
yaw 45°). Each frame the model's body yaw (record +0x137c) decreases by **0.05 · dt** (so the
figure turns continuously, *believed* 50°/s) and record +0x1188 is set to 0.2. Lighting for
this draw: front (1, 1, 1, 1), back (0.2, 0.3, 0.4, 1), ambient (0.4, 0.4, 0.4, 1), light
direction (0.5, 1, 1.5)/√3.5 = (0.267, 0.535, 0.802), fog distance 10⁹ (none). The model wears
its equipment (equipment block record +0x300 is passed). *(The direction differs from the one
in `alpha-rendering.md` §11.5, which may belong to another preview; open point C-5.)*

### 3.3 Scrolling and placement of the panels (0x489d4f … 0x48a3a9, every frame)

Only while character select is visible and the "Please wait..." label is hidden; otherwise
the scroll speed G+0x800a04 is set to 0 (0x48a80e).

Let the panels be p₀ … p_*N* (top to bottom), each 600 × 220, and v = G+0x800a04 (pixels per
second).

1. **Bounds.** T = 10, B = H − 10 − h_last (h_last = 220). Let the span
   S = trunc(y_last + h_last − y₀). If S < H − 20 the column is shorter than the window and is
   centred instead: T = ⌊H/2⌋ − trunc(S/2), B = (⌊H/2⌋ + trunc(S/2)) − h_last (C integer
   halves).
2. **Pull down to the top bound.** If y₀ > T: move y₀ towards T with the easing
   y₀′ = (1 − f)·y₀ + f·T, f = 1 − 0.99^dt (0x4aba20 with rate 0.01); every panel is moved by
   y₀′ − y₀, and x := W/2 − 300 (W/2 integer).
3. **Pull up to the bottom bound.** Then (whether or not step 2 ran), if y₀ < T **and** y_last < B: move
   y_last towards B with the same easing and move every panel by the same amount, x as above.
4. **Coast and pick.** For every panel *i* in order: x := W/2 − 300; y := y + v · 0.001 · dt.
   If the mouse is over panel *i* (0x6294c0 → 0x629300): if *i* differs from the selected index
   G+0x800a0c, hide Delete Character; selected index := *i*.
5. **Friction.** v := (1 − f)·v with f = 1 − 0.995^dt (0x4aba20, target 0, rate 0.005).

**Dragging** (mouse-move handler 0x47ea00, 0x47eaa7 … 0x47ebad): the handler keeps a smoothed
mouse y, m := (1 − g)·m + g·y_mouse with g = 1 − 0.99^Δt (Δt = ms since the previous mouse
message, timeGetTime). While character select is visible **and** the left button is held,
every panel moves by the change of m (and is recentred in x), and
v += (y_mouse − y_previous) · Δt · 2 (the two mouse values are the interface's current and
previous cursor y, interface +0xd8 and +0xe0, *believed*). Releasing the button lets the list
coast with v. **Any mouse-button press** sets v = 0 (0x47b70f) — but only for this list (§4.3).
There is no mouse-wheel scrolling (*believed*: the wheel handler was not seen to touch these
values).

**Selection follows the mouse.** There is no click-to-select: the selected entry is the
last panel the mouse passed over, and it stays selected when the mouse leaves. Nothing
highlights it except that the buttons of §3.4 sit on it. The initial selected index is 0
(0x45a438), so with no characters the "New character" panel is selected at once.

### 3.4 The buttons on the selected panel (0x48a3af … 0x48a7c8, every frame)

With the selected panel at (x, y) and size 600 × 220, if 0 ≤ selected < number of panels:

| Button | Top-left | Visibility set here |
|---|---|---|
| Select / Create (200 × 30) | (x + 600 − 200 − 20, y + 220 − 30 − 20) = (x + 380, y + 170) | shown |
| small button (20 × 20) | (x + 600 − 20 − 20, y + 20) = (x + 560, y + 20) | shown |
| Delete Character (300 × 30) | (x + 600 + 20, y) = (x + 620, y) — outside the panel, to its right | unchanged |

Otherwise the three buttons are hidden. Then, if selected ≥ *N* (the "New character" panel):
the small button and Delete Character are hidden and the Select caption is set to
**"Create"**; else the caption is **"Select"**. (The caption is rewritten every frame.)

### 3.5 Deleting a character (two steps, no dialog)

1. Clicking the small 20 × 20 button (0x4829c0) **shows** Delete Character; it does nothing
   else. This is the only confirmation.
2. Delete Character stays visible until the selection moves to another panel (§3.3 step 4)
   or the "New character" panel is selected.
3. Clicking **Delete Character** (0x4816f0) deletes the selected character *k*:
   * destroys its in-memory record and removes it from the list;
   * writes `num` = *N* − 1, then renames keys *k*+1 … *N*−1 down by one
     (`alpha-saves.md` §3.1); no world or map file is touched;
   * rebuilds the list (§3.1). The selected index is **not** changed and Delete Character is
     **not** hidden (*believed*: the rebuild only re-parents it), so after the rebuild the button sits beside the next character (now at
     index *k*), and a second click deletes that one too without another confirmation, unless
     the mouse has passed over another panel in between (faithful quirk; an engine may hide the
     button after deleting). If *k* was the last character the "New character" panel becomes
     selected and the buttons hide.

### 3.6 Select

Select on a character (0x483e70, selected < *N*): hide character select, load the record into
P (0x4806c0: also copies the position to the interpolation state and resets the controller
byte, `alpha-saves.md` §3.2), show world select. The camera is not touched.

Create on the "New character" panel (selected ≥ *N*):

1. camera targets and current values := pitch 90, roll 0, yaw 0, distance 5 (§7);
2. show character creation; reset P (0x446330) and then: name := "" (P+0x1168), P+0x60 := 0,
   P+0x28 … P+0x3c := 0, P+0x5c := 0, P+0x180 := 1.0, **level P+0x190 := 1**, health P+0x16c
   := maximum health (0x444db0), main-hand and off-hand slots preset to an iron sword
   (overwritten by the kit, §3.9), P+0x140 := 1, a fresh 64-byte extra-data object
   (P+0x1d28: empty recipe list, no world entries);
3. set the name edit's text to "";
4. reset the creation widget (0x42bd90): race 0, class 0, gender 0, face 0, haircut 0, hair
   colour (255, 200, 50), hover colour (255, 255, 255);
5. apply the choices (0x42c080, §3.9).

### 3.7 The character creation screen

**Layout** (all at their anchors every resize; 1280 × 720 values in §8):

| Element | Placement |
|---|---|
| Back | (20, 20), 200 × 40 |
| style panel (`blackwidget` clone G+0x80096c, 300 × 330, CharacterStyleWidget G+0x800968 bound to it) | top-left (20, H − 20 − 330) (0x482ed5) |
| "Name" label and edit | §2.3 |
| Create Character | (W − 320, H − 60), 300 × 40 |
| info label G+0x8009f4 | (W/2, H − 200) when shown |
| the character itself | drawn by the world frame behind the interface, seen by the game camera (§7; *believed*) |

**Validation** (every frame while creation is visible, 0x4896ab … 0x489966). Let *t* be the
name edit's text (UTF-16):

1. If *t*, converted to the narrow string (0x6089c0), is equal (exact, case-sensitive,
   0x428db0) to the name of any saved character: show G+0x8009f4 with
   "There is already a character with that name." and **hide** Create Character.
2. Otherwise Create Character is visible ⇔ **2 ≤ length(t) ≤ 15** (length in UTF-16 units;
   0x4898a7: length − 2 ≤ 13 unsigned).

No character is rejected; spaces and punctuation are allowed, and a name of spaces only is
accepted. The name is stored narrow (0x659f50) in P+0x1168 (16-byte field), so 15 UTF-16 units
fit when they are single-byte characters (*believed*; non-ASCII names, open point C-2).

### 3.8 Create Character (0x4821a0)

1. Hide character creation, show **world select**.
2. Copy the edit text (narrow) into P+0x1168; P+0x190 (level) := 1.
3. `characters.db`: write `num` = *N* + 1, then write P as record key *N* (0x487520 →
   0x44d790, format `alpha-saves.md` §3.2). Selected index := *N*.
4. Append a copy of P to the in-memory character list (new 0x1e60-byte creature: network
   record, inventory, P+0x11e8, P+0x11ec, P+0x1304, P+0x1308, the extra data and its recipe
   list) and rebuild the character list (§3.1).

The new character is **not** asked for a world yet: the next screen is world select with P as
the active character.

### 3.9 The style panel (CharacterStyleWidget: constructor 0x427ce0, draw 0x428e40, layout 0x42bb00)

**Rows.** Five rows of `leftbutton` / `rightbutton` (19 × 21 each, `alpha-ui-screens.md`
§6.3) plus a hair-colour palette. Panel-local coordinates; the panel is 300 × 330.

| Row *k* | Label (x = 15, baseline 27 + 30·k) | Value (centred at x = (300 − 110)/2 + 100 = **195**, same baseline) | Left arrow top-left | Right arrow top-left |
|---:|---|---|---|---|
| 0 | "Race" | "Human", "Elf", "Dwarf", "Orc", "Goblin", "Lizard", "Undead", "Frogman" | (100, 12) | (300 − 19 − 15, 12) = (266, 12) |
| 1 | "Gender" | "Male" (0) / "Female" (1) | (100, 42) | (266, 42) |
| 2 | "Class" | "Warrior", "Ranger", "Mage", "Rogue" | (100, 72) | (266, 72) |
| 3 | "Face" | "Face " *n* with *n* = face index + 1 | (100, 102) | (266, 102) |
| 4 | "Haircut" | "Haircut " *n* with *n* = haircut index + 1 | (100, 132) | (266, 132) |

Then the label **"Hair color"** centred at (150, 182) (no value), and the palette below it.

All this text is `resource1.dat` **12 px**, drawn twice (3-px black outline pass, then white
fill), as in the options window. Labels are left-aligned at their origin, values centred. An
arrow under the mouse is drawn with fill colour **(0, 1, 1, 1)** (cyan), otherwise
(1, 1, 1, 1) (0x428e40 ≈ lines 960–1110). The value x uses the widget width, always 300.

**Arrow actions** (connected to the press event 2 of each arrow, 0x427b40). Every arrow first
plays sound **0x55** (`menu-select.wav`) at volume 1, pitch 1 (0x484350), then changes its
value and re-applies everything (0x42c080):

| Row | Left arrow | Right arrow | Range |
|---|---|---|---|
| Race (+0x18c) | − 1; below 0 → 7 | + 1; above 7 → 0 | 0 … 7, wraps |
| Gender (+0x194) | toggle (0 ↔ 1) | toggle (same routine 0x42b990) | 0, 1 |
| Class (+0x190) | − 1; below 0 → 3 | + 1; above 3 → 0 | 0 … 3, wraps |
| Face (+0x198) | − 1 | + 1 | wrapped in 0x42c080 by the type's face count |
| Haircut (+0x19c) | − 1 | + 1 | wrapped by the type's haircut count |

**Apply (0x42c080)**, run after every arrow, after a palette click, and whenever the hovered
palette colour changes:

1. **Creature type** P+0x64 := base(race) + gender with base = 2, 0, 9, 11, 4, 7, 15, 13 for
   race 0 … 7 (same table as `alpha-creatures-models.md` §4.1).
2. **Class** P+0x140 := class index + 1 (1 warrior, 2 ranger, 3 mage, 4 rogue). There is no
   specialization row; the specialization byte P+0x141 is left as the reset left it
   (*believed* 0).
3. **Wrap face and haircut** by the counts of the new type (table below): a value below 0
   becomes count − 1, a value ≥ count becomes **0**. (So changing race can reset a high face
   index to 0 rather than clamping it.)
4. **Choice record** (extra data, `alpha-creatures-models.md` §4.1): race := race, sex :=
   gender, head choice := face, hair choice := haircut, hair colour := the **hover colour**
   (+0x1a3 … +0x1a5; the code's test of that pointer is always true) — which equals the
   committed colour (+0x1a0 … +0x1a2) except while the mouse is over a palette swatch.
5. Rebuild the appearance: defaults (0x428750) and the appearance generator (0x43f7c0) with
   the type and the choice record.
6. **Starting kit** (0x4772b0, §3.10).

Face and haircut counts used for wrapping (the model-id ranges first … last of the switch in
0x42c080; count = last − first + 1):

| Type | Race, gender | Faces | Haircuts |
|---:|---|---:|---:|
| 0 | Elf, male | 4 | 10 |
| 1 | Elf, female | 6 | 10 |
| 2 | Human, male (also the default for any other type) | 6 | 15 |
| 3 | Human, female | 6 | 7 |
| 4 | Goblin, male | 5 | 6 |
| 5 | Goblin, female | 5 | 6 |
| 7 | Lizard, male | 2 | 6 |
| 8 | Lizard, female | 5 | 6 |
| 9 | Dwarf, male | 5 | 3 |
| 10 | Dwarf, female | 5 | 5 |
| 11 | Orc, male | 5 | 10 |
| 12 | Orc, female | 5 | 4 |
| 13 | Frogman, male | 5 | 5 |
| 14 | Frogman, female | 4 | 4 |
| 15 | Undead, male | 6 | 6 |
| 16 | Undead, female | 6 | 6 |

(Model-id ranges, for cross-checking `alpha-creatures-models.md` Appendix B: faces / hair —
0: 1236–1239 / 1280–1289; 1: 1240–1245 / 1290–1299; 2: 1246–1251 / 1252–1266;
3: 1267–1272 / 1273–1279; 4: 75–79 / 80–85; 5: 86–90 / 91–96; 7: 98–99 / 100–105;
8: 106–110 / 100–105; 9: 282–286 / 287–289; 10: 290–294 / 295–299; 11: 1300–1304 /
1310–1319; 12: 1305–1309 / 1320–1323; 13: 1324–1328 / 1329–1333; 14: 1334–1337 / 1338–1341;
15: 303–308 / 309–314; 16: 315–320 / 321–326.)

**Hair-colour palette** (drawn every frame by 0x428e40):

* 16 base colours, column *c* = 0 … 15: (1, 0, 0), (1, 0.25, 0), (1, 0.5, 0), (1, 1, 0),
  (0.5, 1, 0), (0, 1, 0), (0, 1, 0.5), (0, 1, 1), (0, 0.5, 1), (0, 0.25, 1), (0, 0, 1),
  (0.25, 0, 1), (0.5, 0, 1), (0.75, 0, 1), (1, 0, 1), (0.5, 0.5, 0.5).
* 7 rows, *r* = 0 … 6, row top y_r = 197 + 17·r (197, 214, 231, 248, 265, 282, 299). Row
  colour: *r* < 3 → base × (0.2 + 0.25·r) (× 0.2, 0.45, 0.7); *r* = 3 → base; *r* > 3 →
  base + (1 − base)·0.25·(r − 3) (25, 50, 75 % towards white; *believed*, from the helper
  pair 0x428990 / 0x4289e0).
* Each swatch: a black 17 × 17 square at (13 + 17·c, y_r), then the colour as a 15 × 15
  square at (14 + 17·c, y_r + 1).
* **Hover.** At the start of the palette pass the hover colour is reset to the committed
  colour. If the mouse (panel-local, 0x6294d0) is inside [13 + 17c, 30 + 17c) ×
  [y_r, y_r + 17), the hover colour := (trunc(R·255), trunc(G·255), trunc(B·255)) of that
  swatch. If the hover colour changed this frame, Apply runs (so the figure shows the colour
  under the mouse and returns to the committed one when the mouse leaves).
* **Click.** An invisible 280 × 150 click area (vertex colours all zero) positioned at
  (10, 200) in the panel (0x427bc0 with callback 0x42b8b0, press event) plays sound 0x55,
  copies the hover colour into the committed colour and runs Apply. Clicking the area between
  swatches therefore commits the unchanged colour.

The default colour (255, 200, 50) is not one of the swatches.

### 3.10 Starting state of a new character (0x4772b0, re-run by every Apply)

Each run starts from a blank creature's inventory and money fields (0x43bc00 / 0x44af00 /
0x43c020: inventory P+0x11dc, P+0x11e8, P+0x11ec, P+0x1304, P+0x1308 copied from it),
makes sure the inventory has **4 tabs of 50 slots**, then:

**Equipment** (`alpha-creatures-models.md` §6.4 slots; item fields per
`alpha-net-packets.md` §1.1; rarity 0):

| Slot | Warrior | Ranger | Mage | Rogue |
|---|---|---|---|---|
| 7 main hand (type 3) | subtype 17 **Greatmace**, material 2 (Wood), level 1 | subtype 6 **Bow**, Wood, level 1 | subtype 10 **Staff**, Wood, level 1 | subtype 3 **Dagger**, material 1 (Iron), level 1 |
| 6 off hand | — | — | — | **Dagger**, Iron, level 1 |
| 8 ring | type 9, material 12 (Silver), level 0 | same | same | same |
| 9 ring | type 9, material 11 (Gold), level 0 | same | same | same |
| 10 lantern | type 24, subtype 0, material 1, level 1 | same | same | same |

**Inventory** (added in this order with the add-to-inventory routine 0x46ebe0; each added
item is level 1, rarity 0):

| Class | Items |
|---|---|
| Warrior | Iron Sword (3, 0, Iron) × 2, Iron Shield (3, 13, Iron) × 1 |
| Ranger | Wood Crossbow (3, 7, Wood) × 1, Wood Wand (3, 11, Wood) × 1 |
| Mage | Gold Bracelet (3, 12, Gold) × 2, Wood Wand (3, 11, Wood) × 1 |
| Rogue | Iron Fist (3, 4, Iron) × 2, Iron Longsword (3, 5, Iron) × 1 |
| all classes, then | Life Potion (1, 1), level 1, × 5 |

(The ranger's wand comes from code shared with the mage's last item — faithful, probably
unintended. The jump table at 0x4777d8 orders the cases warrior, ranger, mage, rogue.)

**Then:** health := maximum health (0x444db0, depends on class and level); the recipe list
is emptied and the level-1 grants are made (`alpha-crafting.md` §2.2). Because Apply runs on
every arrow click and on every palette-hover change, the grant's `rand()` draws (armour and
weapon key seeds) are consumed again each time; only the last run's keys are kept.

**Summary of the record written by Create Character** (`alpha-saves.md` §3.2 fields): level
1, class 1–4, specialization unchanged (*believed* 0), health = maximum, name as typed,
equipment and inventory as above, money/unknowns from the blank creature, recipe list = the
level-1 grants, choice record (race, sex, face, haircut, committed-or-hovered colour), no
world entries, no current world. Position, orientation and experience are not set by these
routines (*believed* zero after the reset; open point C-4).

---

## 4. World select

### 4.1 The world list (builder 0x4a23d0)

Built at start-up, after "Create World", after "Delete World" and whenever the multiplayer
toggle is pressed. Steps as §3.1, with the panel list G+0x8009c0, and:

* The world entries (list G+0x8009dc, read from `worlds.db`) are filtered by the mode flag
  G+0x8009b0 (0 = singleplayer at start-up): an entry is an **online world** when its name is
  longer than 7 characters and starts with `online_`. Singleplayer mode lists the other
  entries, multiplayer mode lists only online worlds.
* After the listed worlds, the **"New world"** panel (record null) is always added, in both
  modes.
* The *j*-th panel made (counting listed worlds and the New world panel) is placed at
  **(300, 50 + 230·j)**. Two maps keep panel → world index (G+0x8009cc) and world index →
  panel (G+0x8009d4); the New world panel stands for index *M* = number of world entries.

### 4.2 What a world panel shows (WorldPreviewWidget draw 0x605ae0)

* **"New world"** panel: "New world", 14 px, origin (300, 30), flags 5, as the "New
  character" panel.
* **World panel:** the world's preview model (the `worlds.db` preview voxels, drawn in the
  left part of the panel with its own projection, slowly turning; *believed*, not traced in
  detail) and:

| Line | Origin | Size | Flags | Text |
|---|---|---|---|---|
| name | (200, 30) | 14 | 0x10, wrap 180 | the world name |
| explored | (200, 55) | 10 | 0 | "Explored: " *d* " km²" — *d* = the entry's discovered counter, printed as a float by the default stream format (so "38", "1.23457e+06" past six digits) |
| seed | (200, 80) | 10 | 0 | "Seed: " *seed* (decimal) |

### 4.3 Scrolling, selection and buttons

Identical to §3.3 and §3.4 with: container G+0x80088c, panel list G+0x8009c0, scroll speed
G+0x800a08, selected **world index** G+0x800a10 (the hovered panel's world index via
G+0x8009cc), and the buttons Select (G+0x8009e8), small (G+0x8009ec) and Delete World
(G+0x8009f0). If the selected world index has no panel in the current mode (G+0x8009d4 gives
none) all three buttons are hidden. Selected ≥ *M* (New world) → caption "Create", small and
Delete hidden; else "Select". Differences:

* the speed G+0x800a08 is **not** zeroed by a mouse press (only G+0x800a04 is, 0x47b70f), so a
  flung world list keeps coasting under a click (faithful quirk);
* hidden world select sets G+0x800a08 to 0 (0x48b2c1).

**Multiplayer Worlds...** (0x484230): toggles G+0x8009b0, sets the caption to
"Singleplayer worlds..." when the flag becomes 1 and to "Multiplayer worlds..." (lower-case
w) when it becomes 0, and rebuilds the list. The start-up caption is
"Multiplayer Worlds..." (capital W).

### 4.4 Select / Create (0x484170)

Hide world select; camera targets and current values := pitch 90, roll 0, **yaw 180**,
distance 5. If the selected world index < *M*: start that world (0x46f620 with its seed and
name: stores them as the character's current world and loads or generates it; see the world
specs). Otherwise show **world creation**. Selecting an online world in multiplayer mode
starts it the same way, as a local world (*believed*).

### 4.5 Deleting a world (0x4829e0, then 0x481d30)

The small button shows Delete World; Delete World on world *k* (0 ≤ *k* < *M*):

1. deletes the files `Save\world_<name>.db` and `Save\map_<name>.db` (DeleteFileA);
2. removes the entry from the list; if the selected index is now ≥ the new count, it becomes
   count − 1;
3. writes `num` = *M* − 1 to `worlds.db`, then writes every remaining entry with 0x4878a0 —
   **all to the key of the (adjusted) selected index** (the loop passes the selected index,
   not the loop index). Net effect on disk: key *k* ends up holding the last entry, keys
   *k*+1 … *M*−2 keep their old contents and key *M*−1 is beyond `num`; so after a restart the
   list is the old list with the last world moved into the deleted world's place. (Faithful
   quirk; in memory the order is the shifted one until the next start.)
4. rebuilds the list. As with characters, Delete World is not hidden afterwards.

---

## 5. World creation

### 5.1 Layout

Back (20, 20); "Name" label (W/2, H − 50) with its edit; "Seed (must be a number)" label
(W/2, H − 100) with its edit (§2.3); Create World (W − 320, H − 60), 300 × 40; info label
(W/2, H − 200) when shown.

### 5.2 Initial text

The edits keep whatever they held last time (they are cleared only at construction;
*believed*). There is no random-seed button and no default seed: the player must type one.

### 5.3 Validation (every frame while world creation is visible, 0x4899a6 … 0x489cd3)

Let *n* be the name text and *s* the seed text.

1. If *n* (narrow) equals the name of any world entry (0x427a00): show the info label with
   "There is already a world with that name." and hide Create World.
2. Otherwise Create World is visible ⇔ **length(n) ≥ 2** and **length(s) ≥ 1** and every
   character of *s* is '0' … '9'. There is no upper limit on the name length, and the seed
   may not have a sign.

### 5.4 Create World (0x482530)

1. Hide world creation; camera := pitch 90, roll 0, yaw 180, distance 5 (targets and current).
2. `worlds.db`: write `num` = *M* + 1.
3. Make a world entry: name := *n* (narrow), seed := *s* parsed by a wide input stream as an
   unsigned 32-bit number, discovered := 0, no preview. A seed with more digits than fit makes
   the stream fail (value not set; open point C-6).
4. Append it, selected world index := *M*, write it as key *M* (0x4878a0).
5. Start the world (0x46f620 with seed and name), then rebuild the world list.

Note that a name starting with `online_` typed here creates a world that only the multiplayer
list shows.

---

## 6. Connect screen

Server-address label and edit at (W/2, H/2 − 100) (§2.3), Connect button centred, Back
(20, 20). Connect sends the narrow address text to the connect routine (0x46fc50);
messages appear in the red label G+0x8009f8 (`alpha-ui-screens.md` §4). Back returns to world
select.

---

## 7. The camera behind these screens

The world frame always runs behind the interface (`alpha-ui-screens.md` §3.3), so the screens
show the scene from the game camera:

* While the **start screen** or **connect** is visible (0x489cd3 … 0x489d4b), while
  **character select** is visible (0x489d7d …) and while **world select** is visible
  (0x48a818 …), and the "Please wait..." label is hidden: camera target distance G+0x1c0 := 10,
  target pitch G+0x1b0 := 100, target yaw G+0x1b8 += 0.005 · dt (a slow turn,
  *believed* 5°/s). The camera eases towards these as described in `alpha-physics-camera.md`.
* **Character creation** and **world creation** do not touch the camera: it keeps what the
  entering routine set and snapped (current := target): character creation pitch 90,
  roll 0, yaw 0, distance 5 (target and current, 0x483e70); world creation pitch 90, yaw 180,
  distance 5 (0x484170). So the creation screen shows the player character P from 5 blocks,
  level with it, unturning (*believed*: the camera focus is P, and P is drawn by the world
  frame). This is the "3-D preview" of character creation; it is lit by the world's normal
  lights, not by the preview lights of §3.2.

---

## 8. Worked examples (1280 × 720)

**8.1 Fixed elements.** Back (20, 20)–(220, 60). Multiplayer Worlds... (960, 20)–(1260, 60).
Connect to server, Create Character, Create World (960, 660)–(1260, 700). Connect
(540, 340)–(740, 380). Name label origin (640, 670), edit (577.5, 680)–(702.5, 700). Seed label
(640, 620), edit (577.5, 630)–(702.5, 650). Server label (640, 260), edit (577.5, 270)–(702.5,
290). Info label (640, 520).

**8.2 Character list with one character.** Panels p₀ (the character) and p₁ (New character)
are made at y 50 and 280; x becomes 640 − 300 = 340. S = 280 + 220 − 50 = 450 < 700, so the
column is centred: T = 360 − 225 = 135, B = 360 + 225 − 220 = 365. On the first frame
(dt = 16) y₀ = 50 < T and y₁ = 280 < B, so step 3 eases p₁: f = 1 − 0.99¹⁶ = 0.1486,
y₁′ = 280 + 0.1486·(365 − 280) = 292.6, both panels move by 12.6 (y₀ = 62.6). After about a
second the panels rest at y 135 and 365. With p₀ selected (mouse over it): Select
(720, 305)–(920, 335), small button (900, 155)–(920, 175), Delete Character (once shown)
(960, 135)–(1260, 165). The model's origin is drawn at (440, 245); the texts at (540, 165)
name, (540, 190) "LVL 1 Human Warrior", (540, 210) "Specialization: Berserker".

**8.3 Four characters.** Five panels at 50, 280, 510, 740, 970: S = 1140 ≥ 700, so T = 10,
B = 720 − 10 − 220 = 490. y₀ = 50 > 10: step 2 pulls the column up until y₀ = 10 (panels at
10, 240, 470, 700, 930). Dragging up by 600 px moves the last panel to 330 < 490 while
y₀ = −590 < 10, so on release step 3 pulls it back down to 490.

**8.4 Creation panel.** Panel (20, 370)–(320, 700). Screen positions: "Race" label origin
(35, 397), value "Human" centred on (215, 397), arrows (120, 382) and (286, 382); "Gender"
row baseline 427; "Class" 457; "Face" 487; "Haircut" 517; "Hair color" centred on (170, 552);
palette swatch (c, r) black square at (33 + 17c, 567 + 17r), e.g. the grey column's brightest
row (15, 6) at (288, 669)–(305, 686). Hovering swatch (1, 3) (orange) gives hover colour
(255, 63, 0); hovering (0, 0) gives (51, 0, 0); (0, 4) gives (255, 63, 63).

**8.5 Arrows.** Start: Human, Male, Warrior, Face 1, Haircut 1. Race left: race 0 − 1 → 7
"Frogman", type 13, faces 5, haircuts 5. Face right ×6 from Face 1: indices 1, 2, 3, 4, 0, 1 →
shows "Face 2". With Human Male Face 6 (index 5), race right → Elf (type 0, 4 faces): 5 ≥ 4 →
index 0, shows "Face 1". Gender on Lizard Male Face 2 (index 1) → Lizard Female (5 faces):
stays index 1.

**8.6 Name check.** "A" → button hidden. "Al" → shown. "Abcdefghijklmno" (15) → shown;
16 characters → hidden. An existing character "test" and *t* = "test" → hidden and the info
label shows the message; "Test" → shown (case-sensitive).

**8.7 World check.** Name "ab", seed "123" → shown. Seed "" → hidden. Seed "12a" or "-5" →
hidden. Name "x" → hidden.

---

## 9. Open points

* **C-1** The library `Edit` widget (`edit` template, WM_CHAR via 0x652710): maximum
  length, accepted characters, caret, focus and whether Enter does anything. Trace the edit
  class or capture typing 30 characters.
* **C-2** Narrow conversion 0x659f50 (code page or UTF-8) and what happens to names whose
  narrow form exceeds 15 bytes.
* **C-3** The small button's caption (the `smallbutton` template's text shape, not set by
  code) and the `button` caption's font size (*believed* 14 px like `button2`). Read the text
  shapes of `gui.plx`.
* **C-4** Experience, position and orientation of a new character: confirm that 0x446330
  zeroes P+0x194 and the position, or capture a fresh `characters.db` record.
* **C-5** The character-panel model view: exact meaning of the angles −1.658 rad and
  0.785 rad, the direction of the light (0.267, 0.535, 0.802) against
  `alpha-rendering.md` §11.5, and the turning rate unit of record +0x137c.
* **C-6** Seed overflow (more than 10 digits or > 4294967295) and the uninitialised seed field
  of the new entry; and whether the multiplayer list's online worlds can really be started
  offline.
* **C-7** The click event number 3 used by the big buttons (press vs release), and the camera
  focus during character creation (P drawn at its position by the world frame?) — capture.
* **C-8** WorldPreviewWidget's model placement and turning (0x605ae0 lines 420–1000 not
  traced).
