# Alpha interface: screens, HUD, windows and controls

Status: FINAL / STABLE / HELD (re-certified 2026-11-15; see "Re-certification" at the end)

Behavioural specification of what the Cube World **Alpha** client (`Cube.exe`, SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, image base 0x00400000)
shows and how the player drives it: the start screen and its menus, the character and world
screens, the multiplayer connect screen, options, the system menu, the in-game HUD, the windows,
and the keyboard and mouse controls. The file format and drawing of the `.plx` interface files
are described in `alpha-ui-plasma.md`, which this page assumes.

**Scope.** The layout model, the start-screen menus, the options and system menus, the
anchoring of every HUD element the game positions, the per-frame bar rules (§5.3–5.4), the
ability bar (§5.5), floating combat text (§5.6), the Tab quick-item wheel (§5.7), the prompt
and death behaviour (§5.8), the minimap (§5.1), the Esc button row (§7), the chat box (§9.1),
the controls, the interface sounds and the interface text. The contents of the item windows
(inventory, equipment, crafting, skills, character creation) are covered to the depth given in
§8; open items are collected in §12 (Unresolved), and the certification record is the
final section.

Routines are cited by virtual address in `Cube.exe` (RVA = VA − 0x400000). "G" is the game
object (the GameController); "W" and "H" are the window client width and height in pixels
(G+0x11c, G+0x120). What the controlled actions do in the game (jumping, dodging and so on)
belongs to the player-control specification, not here.

---

## 0. Text in the interface

* **All interface text is English.** Menu, label, prompt and message text is either written
  into the executable as UTF-16 literals or stored as template text inside the `.plx` files
  (for example "QUEST COMPLETE!" in `quest-tag.plx`; "WEAPON 1", "ENEMY", "Stamina" and the
  placeholder "20000/2000o" in `gui.plx`).
* The client reads one dictionary, **`dict_en.xml`** from `data4.db`; the file name is a fixed
  literal (0x4e1b3e, in 0x4e1970). `dict_de.xml` is present in `data4.db` but no reference to
  it exists in `Cube.exe`. The dictionary holds `<name>` entries (with `<singular>` and
  `<plural>` children; creature and object names), `<item>` entries, `<landscape>` name
  patterns and `<speech>` long texts (skill and specialization descriptions, quest objectives
  and dialogue, using the markup `[text $style]`, `%` and `@variable`). It does not translate
  menus. What the `language` option changes is not established here (unresolved: U-F1-1).
* Text drawn by code (not from a `.plx`) uses the font `resource1.dat`.
* Quoted strings below are exact, including spaces.

---

## 1. Interface roots and files

The start-up routine 0x459c40 creates seven top-level containers directly under the interface
root, in this order (0x45a778 … 0x45aa95):

| Container | Holds |
|---|---|
| in-game root (G+0x800884) | `gui.plx` and `quest-tag.plx` templates and every HUD clone; also the options window, the system panel and the other in-game windows |
| character-creation screen (G+0x800880) | the character style panel, the "Create Character" button and the name edit |
| world-creation screen (G+0x800890) | the "Create World" button, the world-name and seed edits |
| character-list screen (G+0x800888) | shown by "Start Game" (§3.2) |
| connect screen (G+0x800894) | the server-address edit and the "Connect" button (§4) |
| world-list screen (G+0x80088c) | shown after a character is created; holds the "Connect to server" button |
| start-screen root (G+0x800874) | `start.plx` (the `cubeworld` and `picroma` logos) and the start menu |

The help container (G+0x80089c, 0x45c1e8) holds `help.plx`; **F1** toggles its visibility
(key handler 0x47e1b0, key code 0x70), and so does the first menu button (§7).

The roles of G+0x800888, +0x80088c, +0x890 and +0x894 above are read from the screen
transitions in §3.2, §3.5 and §4 (0x47b600, 0x4814f0, 0x4821a0, 0x481fe0, 0x4815e0).

Templates looked up by name at start-up and kept for cloning (0x459c40): `questtag`,
`smallquesttag` (`quest-tag.plx`); `cubeworld`, `picroma` (`start.plx`); `star1`–`star4`,
`landname`, `blackwidget`, `itembox`, `wideitembox`, `itemselector`, `combopoints`,
`leftbutton`, `rightbutton`, `upbutton`, `downbutton`, `scrollbutton`, `edit`, `button`,
`button2`, `smallbutton`, `preview`, `equippedpreview`, `crosshair`, `zoomcrosshair`,
`lockedenemy`, `lockedfriend`, `abilitybutton`, `skillbutton`, `specializationbutton`,
`childskillbutton`, `quickitembutton`, `menubutton`, `equipmentboxleft`, `equipmentboxright`,
`experiencebar`, `ridingbar`, `manacubebar`, `questbar`, `lifebar`, `mpbar`, `chargebar`,
`enemylifebar`, `friendlifebar`, `staticlifebar`, `neutrallifebar` and their `:small`
variants, `hpbar`, `castbar`, `staminabar`, `selector`, `speech`, `craftbutton`, `craftbar`,
`itemshadow`, `itemframe`, `rightarrow`. Inside nodes the game looks up the child names
`text`, `info`, `name`, `count`, `class`, `bar`, `bar2`, `background`, `frame`, `cooldown`,
`rarename`, `objective`, `detail`, `show`, `shine`, `manacubes`, `singular`.

---

## 2. Layout model

* The interface is laid out in **window pixels at 1:1**. The layout uses W and H directly with
  no scale factor, so a 1920 × 1080 window shows the same-size elements as 1280 × 720, with
  more space between them.
* A placed node is positioned either by writing its translation so that its **pivot** lands on
  an anchor point (translation = anchor − pivot), or, for widget-backed elements, by setting
  the widget's top-left position. Both are computed from W and H and redone whenever the
  window size changes (GameController virtual slot 4, routine 0x482a40). The routine ends by
  setting the interface cursor position to the centre of the new window size (0x483e52).
* Windows (inventory, equipment, …) are centred, or placed to the left or right of the centred
  inventory window with a 10-pixel gap (below).

Anchors written by 0x482a40 ("pivot" = the node's pivot lands on the point; "direct" = the
point is written as the translation itself; "top-left" = the widget's top-left corner is put
there; w and h are the element's own width and height):

| Element | Point (x, y) | Kind |
|---|---|---|
| title logo `cubeworld` (G+0x800878) | (W/2, H/2) | pivot |
| `picroma` logo (G+0x80087c) | (W − 20, H − 20) | pivot |
| quest banner `questtag` (G+0x800868) | (W/2, H − 300) | pivot |
| small quest tag `smallquesttag` (G+0x80086c) | (W − 200, 400) | pivot |
| combo counter `combopoints` (G+0x800ad0) | (W/2, H − 190) | pivot |
| land-name banner `landname` (G+0x800758) | (W/2, 230) | pivot |
| speech container (G+0x80093c), holding a 400 × 400 widget (G+0x800938, built by 0x4e5c90) | x = W/2 − (widget width)/2, y = 300 | direct |
| node G+0x800944 (named "objectives" here; identity inferred) | (20, 170) | direct |
| node G+0x80094c (named "statistics" here; identity inferred) | (20, 470) | direct |
| help container (G+0x80089c) | (W/2 − 430, 50) | direct |
| "Please wait..." label (G+0x800898) | (W/2, H/2) | direct |
| red name-error label (G+0x8009f4, text "Error") | (W/2, H − 200) | direct |
| red connection-error label (G+0x8009f8) | (W/2, H/2 + 100) | direct |
| character-name edit group (G+0x800974) | (W/2, H − 50) | direct |
| world-name edit group (G+0x8009bc) | (W/2, H − 50) | direct |
| world-seed edit group (G+0x8009b8) | (W/2, H − 100) | direct |
| server-address edit group (G+0x8009ac) | (W/2, H/2 − 100) | direct |
| start menu (G+0x800918) | x = (W − w)/2, y = H/2 + 50 | top-left |
| options window (G+0x800a00) | ((W − w)/2, (H − h)/2) | top-left |
| system panel (G+0x800910) | ((W − w)/2, (H − h)/2) | top-left |
| character style panel (G+0x80096c) | (20, H − 20 − h) | top-left |
| "Create Character" button (G+0x800970) | (W − 20 − w, H − 20 − h) | top-left |
| "Create World" button (G+0x8009b4) | (W − 20 − w, H − 20 − h) | top-left |
| "Back" button (G+0x80099c) | (20, 20) | top-left |
| "Multiplayer Worlds..." button (G+0x8009a0) | (W − 20 − w, 20) | top-left |
| "Connect to server" button (G+0x8009a4) | (W − 20 − w, H − 20 − h) | top-left |
| "Connect" button (G+0x8009a8) | ((W − w)/2, (H − h)/2) | top-left |
| button list G+0x800978 … G+0x80097c | x = W/2 − w/2, y unchanged | top-left |
| button row A = the **ability bar** (list G+0x8007fc, *n* = 7, §5.5) | button *i*: x = W/2 + (2*i* − *n*)·24 + 6 = W/2 − 162 + 48*i*, y = H − 50 | pivot |
| button row B = the six **menu buttons** (list G+0x800838, *n* = 6, §7) | button *i*: x = W/2 + (2*i* − *n* + 1)·30 = W/2 − 150 + 60*i*, y = H − 140 | pivot |
| chat box (ChatWidget G+0x800a14, 400 × 200) | (20, H − 220) | top-left |
| host of the character panel (the widget of the nearest ancestor of G+0x800960, a CharacterWidget built by 0x434d90 inside a 260 × 200 `blackwidget` clone) | (20, 340) | top-left |
| two-column list (G+0x800a34) | entry *i*: x = W/2 − 200 + (*i* mod 2)·400, y = H/2 + ⌊*i*/2⌋·56 + 10 | pivot |
| inventory window (G+0x8008bc) | x = W/2 − w₀/2, y = H/2 − h₀ (w₀, h₀ = its own size) | top-left |
| windows G+0x8008c0, G+0x800908, G+0x8008dc | x = W/2 − w₀/2 − w − 10, y = H/2 − h₀ | top-left |
| window G+0x800ad4 | x = W/2 − w₀/2 − w − 10, y = H/2 − h₀ + 10 + (height of window G+0x8008c0) | top-left |
| windows G+0x8008c4, G+0x8008f8, G+0x800900 | x = W/2 + w₀/2 + 10, y = H/2 − h₀ | top-left |
| node of window G+0x8008dc | (interface width/2 − 340, interface height/2 − 250) | pivot |

Here w₀ and h₀ are always the inventory window's width and height, so every side window's top
edge is level with the inventory's top edge, except G+0x800ad4, which sits 10 pixels below
window G+0x8008c0. The inventory is first given a bottom-right position
(W − 20 − w₀, H − 220 − h₀) in the same pass (0x483599) and then moved to the centred position
above (0x4838e4), so only the centred position takes effect. Window G+0x8008dc receives both a
pivot placement of its node (0x48362f … 0x4836b2) and the left-of-centre placement of its
widget (0x483d1d … 0x483dd5); how the two combine on screen is unresolved (U-F1-2).

Placed every frame by the update routine 0x488ee0 rather than by 0x482a40: the crosshair
`crosshair` / `zoomcrosshair` (G+0x8008ac / G+0x8008b0), pivot at (W/2, H/2) (0x4927da,
0x4928ae); the quick-item wheel label `selector` (G+0x8008e0), pivot at (W/2, H/2 − 200)
(0x492727 … 0x492798).

Row B is the menu-button row: it is the list the Esc menu iterates (§7). Row A is the ability
bar: six `abilitybutton` clones followed by the `quickitembutton` template node itself
(0x460234 … 0x46050b).

### 2.1 The pivot rule and the node transform

Every "pivot" anchor in this file is the screen point where the node's **pivot** lands. The
game writes translation = anchor − pivot (0x482a40, and per frame in 0x488ee0 through
0x468df0). This works because the library builds a node's matrix (0x678600) as

**x′ = t + p + R · D · (x − p)**

that is, translate by the translation *t*, translate by the pivot *p*, rotate about x, then y,
then z by the `rotation` angles (degrees), apply the `deformation` matrix *D*, and translate by
−*p*. With t = A − p the pivot lands on A, and a shape point *x* lands at A + R·D·(x − p). For
identity R and D a shape point simply moves by *t* (x′ = x + t).

Consequences used below:

* **Bars fill by scaling the deformation about the pivot**: the game resets the `bar` node's
  deformation to identity and scales its x row by the fill fraction *f* (0x423e70, then
  0x424730 with (f, 1, 1)). A bar point *x* is drawn at pivot + f·(x − p) horizontally, so each
  bar shrinks towards its pivot; the bar pivots of §5.3 are on or within 5 px of the left edge.
* Children are placed in their parent's space by the same rule. For a child with identity R
  and D its shape points are at shape + child *t* in the parent's space, and a text child's
  origin (first baseline, §3.4) is at the child's *t*.

The screen rectangles in §5 follow from this rule and the `gui.plx` values.

---

## 3. Start screen

### 3.1 Logos

`start.plx`'s `cubeworld` (the title, a vector logo with a masked shine group) is centred on the
screen and `picroma` sits at the bottom-right corner (§2). Their timing and the background are
in §3.3.

### 3.2 Start menu (StartMenuWidget, draw 0x583320)

The start menu is a StartMenuWidget (constructor 0x583270, size 200 × 100, 0x463fab … 0x463fd7)
drawn entirely in code. Three text items are centred horizontally in the widget
(x = widget width / 2), font `resource1.dat` at 18 px:

| Item | Baseline y | Hover band (mouse y) | Action on left-button press |
|---|---:|---|---|
| "Start Game" | 30 | [10, 40) | hide the start-screen root, show the character-list screen G+0x800888 |
| "Options" | 80 | [60, 90) | show the options window G+0x800a00 (§6) |
| "Exit" | 130 | [110, 140) | set the quit request byte G+0x1a0 |

Each draw first resets the hovered index (widget +0x160) to −1. An item is hovered when the
mouse, in the widget's coordinates, has 0 ≤ x < widget width and y inside the item's band; the
hovered index becomes 0, 1 or 2.

Each item is drawn twice: first in white (1, 1, 1, 1) with a 4-pixel black (0, 0, 0, 1)
outline, then without outline in the item colour over it. The item colour is white, or
**(0.2, 1, 1, 1)** (cyan) for the hovered item. The visible result is the item colour with a
black rim.

The press is handled by the mouse-button routine 0x47b600: on the left button, while the
start-screen root and the start menu are visible and the hovered index is ≥ 0, it performs the
action in the table. No sound is played by this path.

### 3.3 Placement, backgrounds and timing of the start menu and system panel

**Start menu.** Its host (G+0x800918) is an empty node made at start-up under the start-screen
root (0x463f61). The layout routine places the menu's top-left corner at
**x = (W − 200)/2, y = H/2 + 50**, below the centred title logo. The baselines of §3.2
(30, 80, 130) and the hover bands are measured from that corner. It has **no background
panel**: it is only the outlined text.

**System panel.** The panel toggled by **O** and by the "system" menu button (G+0x800910) is a
clone of the `blackwidget` template under the in-game root, resized to **150 × 120**
(0x464a00 … 0x464a74). The SystemWidget (constructor 0x587660, stored at G+0x800914) is built
on a node inside it. The layout routine centres it: **x = (W − 150)/2, y = (H − 120)/2**. Its
look is the `blackwidget` template's (the shape data are summarised in U-F1-3). The three items
of §7 are drawn on this panel.

**Options window.** Also a `blackwidget` clone under the in-game root, sized **450 × 440**
(0x4648bd … 0x46491e), hosting the OptionsWidget (constructor 0x4cf3c0, stored at G+0x8009fc),
centred by the layout routine.

**What is behind the start screen.** There is no separate start-screen render path. Every frame
(0x4ac260) begins with a device clear of colour and depth (flags 3, depth 1, stencil 0,
0x4ac343) to the colour **0xFF0070FF = (0, 0.439, 1)**, or to white (0xFFFFFFFF) when the byte
G+0x8006e6 is set. The normal world frame follows (`alpha-rendering.md` §1.3) and the interface
is drawn on top. What appears when no world is loaded is unresolved (U-F1-4).

**Loading state.** When the "Please wait..." label (G+0x800898) is visible (test at 0x4ad47e),
the frame records the visibility of the start-screen root and of the containers G+0x800880,
+0x800884, +0x800888, +0x80088c and +0x800890, hides them, draws the interface (0x650980),
restores their visibility, ends the scene, releases the world lock and leaves the frame routine
(0x4ad5a1 … 0x4ad67f). The world passes that follow in the frame routine are skipped, so the
loading screen is the clear colour with the centred label plus whatever interface nodes remain
visible. (The block between the clear and this test, 0x4ac394 … 0x4ad472, still runs; its
effect on the picture is unresolved: U-F1-5.)

**Logo animation and timing.**

* No code other than layout and visibility touches the logo nodes. The masked `Group`
  (clipping) only cuts the lettering (unresolved: U-F1-6, whether `start.plx` holds any
  sequences).
* Early in each update (0x488f4e … 0x48901b), while the start-screen root is visible:
  * the options window node (G+0x800a00) is moved under the start-screen root if it is not
    already there;
  * the `cubeworld` logo and the start menu are both made visible exactly when the options
    window is hidden, and both hidden while it is open.
* While the start-screen root is hidden, the options node is moved under the in-game root.
* The logo and the start menu therefore appear together on the same frame, with **no delay
  and no fade**. `picroma` simply follows the start-screen root's visibility.

### 3.4 Where a text's origin sits (all code-drawn text and TextShapes)

Code-drawn text (start and system menus, labels) and TextShapes go through the same font
routines (draw 0x65bc70, layout 0x65c040, measuring 0x65d530):

* **The origin is the first line's baseline.** Each measured line box starts at
  y = −size (the font's line size, i.e. its pixel size) and ends at 0 relative to the pen,
  with x from 0. The text rises above the origin; the origin is not the top of the text box.
  The start-menu baselines of §3.2 are baselines in this sense.
* **Further lines** start lower by size + `lineSpacing` at each line-feed character (10).
  With flag bit 4 (16) the string is first word-wrapped to the wrap width (0x660d50).
* **Horizontal alignment**, applied to each line:
  * bit 0 (1): the line is shifted left by half its measured width (centred on the origin);
  * bit 1 (2): shifted left by its full width (right-aligned);
  * neither bit: left-aligned at the origin.
* **Vertical alignment:** the start point is moved by the difference between the block's box
  measured with the shape's flags and measured with no flags. This is how bit 2 (4) moves the
  block; that bit 2 centres the block vertically on the origin is believed but not proven
  (U-F1-7).
* **Pixel snapping:** the drawing position is snapped to whole pixels by adding 0.5 and
  truncating (0x65bc70).

### 3.5 Character and world screens

Built at start-up (0x459c40, after `start.plx`). Buttons are clones of `button` resized with the
9-slice rule (`alpha-ui-plasma.md` §5.5); captions go into the button's `text` node. Each button
is connected to its handler at start-up (0x45cbf9 … 0x45cf21).

| Button caption | Screen | Size | On click |
|---|---|---|---|
| "Create Character" | character creation (G+0x800880) | 300 × 40 | 0x4821a0: hide the character-creation screen, show the world-list screen (G+0x80088c), copy the name edit's text into the player's name |
| "Create World" | world creation (G+0x800890) | 300 × 40 | 0x482530: hide the world-creation screen, reset the camera angle triple G+0x1a4 and its target G+0x1b0 to (90, 0, 180) and the two floats G+0x1bc, G+0x1c0 to 5 and start creating the world (how it uses the name and seed edits: U-F1-11) |
| "Back" | — | 200 × 40 | 0x4814f0: see below |
| "Multiplayer Worlds..." | — | 300 × 40 | 0x484230: switch the world list between "Singleplayer worlds..." and "Multiplayer worlds..." |
| "Connect to server" | — | 300 × 40 | 0x481fe0: show the connect screen (G+0x800894), hide the world-list screen, clear the address edit |
| "Connect" | connect (G+0x800894) | 200 × 40 | 0x4815e0: if the address edit is not empty, hide the connect screen and connect to that address (0x46fc50) |

The last three buttons are given a provisional position of (40, 40) at start-up
(0x45cd7d and the matching calls); the layout routine (§2) replaces it.

**"Back"** (0x4814f0) checks the screens in this order and acts on the first visible one:
world creation → world list; world list → character list; character creation → character
list; character list → start screen; connect → world list.

The duplicate-name messages "There is already a character with that name." and "There is
already a world with that name." are produced by the update routine (0x4898c0, 0x489c62),
not by the click handlers.

Edit groups (each a node holding a 12-px label, white with a 3-px black outline, and a clone of
`edit`):

| Label | Group node name | Screen |
|---|---|---|
| "Name" | `charactername` (G+0x800974) | character creation |
| "Name" | `worldName` (G+0x8009bc) | world creation |
| "Seed (must be a number)" | `worldSeed` (G+0x8009b8) | world creation |
| "Server address" | `serverName` (G+0x8009ac) | connect |

Other strings of these screens: "Select", "Delete Character", "Delete World", "Create",
"Singleplayer worlds...", "Multiplayer worlds...", "Please wait...", "Error". Character list
entries are drawn by the CharacterPreviewWidget (draw 0x425450), which uses "New character" and
"LVL " + level.

### 3.6 Character creation (CharacterStyleWidget, constructor 0x427ce0, draw 0x428e40)

The character style panel is a `blackwidget` clone of **300 × 330** under the
character-creation screen (G+0x80096c, 0x464795 … 0x464803), placed at the bottom-left (§2);
the CharacterStyleWidget (stored at G+0x800968) is built inside it. Rows of `leftbutton` /
`rightbutton` arrows sit beside the rows. The draw routine writes, in `resource1.dat` at
12 px, each label first with a 3-px outline and then plain:

| Row label | Label baseline (x, y) | Value text |
|---|---|---|
| "Race" | (15, 27) | "Human", "Elf", "Dwarf", "Orc", "Goblin", "Lizard", "Undead", "Frogman" |
| "Gender" | (15, 57) | "Male" when the gender value (widget +0x194) is 0, otherwise "Female" |
| "Class" | (15, 87) | "Warrior", "Ranger", "Mage", "Rogue" |
| "Face" | (15, 117) | "Face " + number |
| "Haircut" | (15, 147) | "Haircut " + number |

Each value text is centred at x = 100 + (w − 110)/2 on its row, where w is the widget width.
"Hair color" is a title centred at (w/2, 182). The character preview beside it is a
CharacterPreviewWidget (draw 0x425450) lit with back (0.2, 0.3, 0.4) and ambient
(0.4, 0.4, 0.4) (`alpha-rendering.md` §11.5). The arrow placement and the hair-colour control
are unresolved (U-F1-8).

---

## 4. Multiplayer connect screen

"Connect to server" shows the connect screen with the server-address edit (anchor §2, label
"Server address", group `serverName`) and the "Connect" button. The connect routine 0x46fc50
writes its failure messages into the red connection-error label G+0x8009f8: "Connection
error.", "Connection error", "Error: could not connect to server.", "Error: Server has
different version (" *x* ") than client (" *y* ")", "Error: Server is full.". On success it
posts "Connected." as a message coloured (1, 0.5, 0.2, 1) (0x47009d; that this is the chat
area is inferred). "Disconnected." comes from 0x4719f0. The chat command `/connect <address>`
is handled by the key routine 0x47e1b0 (§9). Network behaviour: `alpha-net-transport.md`.

---

## 5. In-game HUD

The player's own bars are the `gui.plx` template nodes themselves (looked up, not cloned:
`lifebar` G+0x800768, `experiencebar` G+0x8007c4, `mpbar` G+0x8007b8, `hpbar` G+0x8007e4,
`castbar` G+0x8007ec, `staminabar` G+0x8007f4, `ridingbar` G+0x8007d4, `manacubebar`
G+0x8007d8; 0x460e87 … 0x46195a). Clones are made only for the pet bars (G+0x800770,
G+0x8007cc), the party bars and the creature bars (§5.3, §5.4). Bars fill by **scaling** the
`bar` child about its pivot (§2.1, §5.3); `bar2` is the empty remainder on creature bars.
Numbers are written as "*current*/*max*" into `text` nodes (the template text is the
placeholder `20000/2000o`).

| Template | What it shows | Text written by code | Notes |
|---|---|---|---|
| `lifebar` (widget) | player health | `text` = HP numbers; `info` = player name; `class` = "LVL " + level + class name (§5.3) | "HP" is static template text |
| `mpbar` | mana | `text` = mana + "/100" | `chargebar` child = charge; "MP" is static |
| `hpbar` | health (bottom bar) | `text` | "HP" is static |
| `experiencebar` (widget) | XP | `text` | "XP" is static |
| `staminabar` | stamina (climbing/swimming) | none; `name` keeps the template text "Stamina" | |
| `castbar` | channel/cast progress | `name` = caption (§5.3) | |
| `ridingbar` | pet riding | — | five icon children |
| `manacubebar` | mana cubes | — | four icon children |
| `questbar` | quest progress | | inside `smallquesttag` |
| `enemylifebar`, `friendlifebar`, `staticlifebar`, `neutrallifebar` and `:small` forms | creature health by attitude | `info` = species name, `rarename` = name or title, `text` (§5.4) | |
| `crosshair` / `zoomcrosshair` | aim marks + **interaction prompt** | `crosshair`'s `name` child = "[R] Talk", "[R] Revive", … (§5.8) | shape data: U-F1-9 |
| `lockedenemy` / `lockedfriend` | lock-on marker | — | |
| `selector` | label of the **quick-item wheel** (Tab, §5.7) | `name` = item name (+ " +" level), `count` = stack size | not the interaction prompt |
| `abilitybutton` | an ability slot | `info` = key label; `cooldown` overlay | §5.5 |
| `quickitembutton` | quick item slot (7th button of the ability bar) | `info`, `count` | §5.5 |
| `equipmentboxleft` / `equipmentboxright` | the two weapons | `count` | template text "WEAPON 1" |
| `menubutton` | buttons that open windows | `count` = key label | icon is the button's own texture; §7 |
| `combopoints` | hit combo | *n* + " HIT" / " HITS" | template also holds `hit` and `button:enter` names (U-F1-10) |
| `questtag` | "QUEST COMPLETE!" banner | `objective` | sound 0x1e (0x4907d7) |
| `smallquesttag` | current quest | `objective`, `questbar` | `show`, `show2`, `shine` names in `quest-tag.plx` |
| `landname` | entering a region | `name` = land name, `detail` | §5.9 |
| `speech` | NPC speech bubble | | |

The `star1`–`star4` templates are cloned at start-up under the code-made "landscape" label
(G+0x800858, a 20-px `resource1.dat` text node in the in-game root, 0x45af8b), and are looked up
and shown or hidden there by the land-name code of the update (0x494722 … 0x494826). No code
reads star nodes on the creature bars.

Messages written to the chat/info area: "You receive " *item*, *name* + " receives ",
"You receive a mana cube." (sound 0x1e, 0x48f9be), "You gain one skill point.", "LEVEL UP!"
(sound 0x1d, 0x493e8f), "You gain " *n* " XP.", "You can't carry more of these items.",
"Pet name too long", "There is nothing special.". Interaction prompts: "[R] Revive",
"[R] Talk", "[R] Ride", "[R] Open", "[R] Close", "[R] Sit Down", "[R] Sleep",
"[R] Teleport", "[R] Use Furnace", "[R] Use Anvil", "[R] Use Spinning Wheel",
"[R] Use Loom", "[R] Use Saw", "[R] Use Workbench", "[R] Use Customization Bench",
"[R] Examine".

### 5.1 Minimap (the "corner map", drawn by the frame, 0x4baa84 … 0x4babaf → 0x5fc1b0)

0x5fc1b0 is the **map renderer**: the same routine draws the world map (§5.2, called at
0x4ae004 with a centred projection, radius 16 and its map-only extras switched on) and the
minimap. This section describes the minimap call. The routine's object is G+0x800d44 (called
"the map object" below).

**When.** Every in-game frame, after the world and before the interface. The caller first
binds the world vertex and pixel shaders, sets the "white" constant to 0, the pixel alpha
constant to 1 and the material to (1, 1, 1, 1) (0x4ba9fc … 0x4baa62), makes one further call on
G+0x800754 (0x632870, not traced, unresolved: U-F2-1), and **clears the depth buffer to 1.0**
(device Clear with the depth flag only; colour and stencil untouched; 0x4baa72 … 0x4baa93). It
then draws the minimap only if the in-game root (G+0x800884) is visible and the node held by
the interface object at +0xb4 (read through 0x487490) is visible (0x4baa95 … 0x4baabc). The
minimap is not a separate render target and has no background, frame or mask: it is a small 3D
model drawn straight into the frame.

**Arguments of the minimap call** (0x4baac2 … 0x4babaf), in order: pivot = the local
player's position ([G+0x8006d0] + 0x10, 64-bit fixed point, 65536 per block); projection
centre x₀ = W − 240, y₀ = H − 170; window W, H; rotation (−120°, 0, −ψ), where ψ is the camera
yaw G+0x1ac (element 2 of the angle triple at G+0x1a4, `alpha-physics-camera.md`; element 0,
G+0x1a4, is the pitch); the view scale *s*; the frame time step G+0x8006e8 (milliseconds,
inferred from its use as a millisecond counter); the radius *r*; map extras off. The zoom input
G+0x1c4 is the **map zoom** (the current value of the zoom the world map uses, §5.2.2), not a
camera distance.

#### 5.1.1 Units

The miniature's model space is **map units of 8 blocks**, centred on the player's exact
position:

* **Zone tiles.** Each known zone has a small voxel model (its `tile`, 32 × 32 voxels across,
  one voxel = 8 × 8 × 8 blocks; its height in voxels depends on the zone's relief, §5.1.6),
  meshed into the normal world vertex format, so tile vertex positions are 0 … 32 map units
  across and their z counts 8-block layers above the tile's base height. A zone is 32 map
  units wide.
* **Land pictures.** At the farthest zoom the routine draws the player's **land** (the named
  region the player stands in) as its map picture: a flat voxel picture one voxel thick with
  **one voxel per zone** (`land` record, `alpha-saves.md` §6.2). Its world matrix scales it by
  32, so it lands in the same 8-block map units.
* The view then multiplies everything by the view scale *s*. A 3 × 3-zone minimap is
  96 map units × 0.8 ≈ 77 view units wide; at the view distance of 300 and a 45° lens this is
  roughly 220–310 px across in a 1280 × 720 window (worked example §5.1.5).

#### 5.1.2 Zoom and content

The zoom table (0x4baac2 … 0x4bab5b; 0.35 = 0x6fd8d8, 2 = 0x745e10):

| Map zoom *d* (G+0x1c4) | View scale *s* | Radius *r* | Terrain drawn |
|---|---|---|---|
| *d* < 0.35 | 0.025 | 0 | the player's land picture (one slab) |
| 0.35 ≤ *d* < 2 | 0.8 | 1 | the zone tiles of the 3 × 3 zones around the player's zone |
| *d* ≥ 2 | 1.6 | 0 | 2 × 2 zone tiles: the player's zone plus the neighbour towards the nearer edge in x and in y (a neighbour on the − side when the player is in the first 128 blocks of the zone along that axis, else on the + side; 0x5fd726 … 0x5fd779) |

The routine takes the land path when *s* < 0.35 and the zone path otherwise (0x5fce30).

**Land path** (*s* < 0.35; 0x5fce30 … 0x5fd68f). The centre land cell is the region cell
(`alpha-worldgen-regions.md`) whose noise-displaced point is nearest the pivot (0x5cade0); if
that lookup gives no cell, the pivot's block coordinates divided by 16384 (rounded down) are
used. For each land cell (lx, ly) within ±*r* of it (x outer, y inner; only the centre cell at
*r* = 0) whose land record (map object +0x4000b0, 1024 × 1024 pointers, index lx·1024 + ly)
exists, has a picture mesh (+0x2c) and has its **visited** byte (+0x28) set, under the map
object's lock at +0x8000d8:

* World matrix (row vectors, *v*·*M*): a point of the picture is first translated by
  (dx, dy, −1) picture voxels and then scaled by 32, where (dx, dy) = (land picture origin −
  pivot) in zones (picture origin = record +0x20, +0x24, in zones; the difference is taken in
  whole 1/256-block steps, i.e. quantised to 1/65536 zone). If the record's integer at +0x1c
  (the copied cell elevation) is negative, the slab is first lowered by a further half voxel
  (16 map units); otherwise its height is first multiplied by (1 + value/256) when that factor
  is not exactly 1. The slab therefore spans z = −32 up to 32·value/256 map units around the
  pivot's level for a non-negative value (−48 … −16 for a negative value); the player's
  height is **not** used.
* Material (the "tint"): (0.2, 0.4, 1, 1) × *k* on the three colour channels, each clamped to 1,
  alpha 1, with *k* = 0.8 + 0.1·((lx + 2·ly) mod 4) (so 0.8, 0.9, 1.0 or 1.1; neighbouring lands
  get different shades of blue). The material colour multiplies the vertex colours as any
  material colour does (`alpha-rendering.md` §5). Reset to (1, 1, 1, 1) after the draw.

**Zone path** (*s* ≥ 0.35; 0x5fd694 … 0x5fee50). The player's zone is the pivot's block
coordinates divided by 256 (rounded down). For each zone offset (i, j) in the range above
(x outer, y inner), skipping zones outside 0 … 65535, under the map object's lock at +0x8000d8:
the zone's map region record is map object +0xb0 indexed by (zone ÷ 64); if it is present, the
zone's map record is that region record + ((zx mod 64)·64 + (zy mod 64))·0x34. Without map
extras, a zone that has no record or whose record has no tile model (+8) draws no terrain.
Otherwise:

* **Rise-in.** h = base height (+4, map units) − 0.1 × the record's timer (+0x2c, value before
  this frame's update); then the timer is reduced by the frame time step and floored at 0
  (0x5fe2f5 … 0x5fe35a). A newly loaded or built tile starts with the timer at 250 (§5.1.6),
  so it rises into place at 0.1 map unit per millisecond over 250 ms.
* **World matrix**: identity with translation
  t = ((player-zone origin − pivot) in blocks ÷ 8 + 32·(i, j),  h − pivot z ÷ 8) map units
  (0x5fe362 … 0x5fe8a1). No rotation, no scale. (A ×4 scale branch at 0x5fe8aa is taken only
  when the record has no tile model, which cannot happen inside this branch.)
* Material (1, 1, 1, 1) (0x5fe328); reset to (1, 1, 1, 1) after the draw. The 0.6 dimming at
  0x5fe9f8 applies only with map extras on (world map, §5.2.4).
* Then, whether or not the zone had a tile, if the map object has its **marker tile**
  (+0x8000b0, built by 0x5fae40: a voxel model allocated 4 × 4 × 4 whose bottom layer holds a
  white (255, 255, 255) 2 × 2 centre inside a (10, 10, 10) border, the other layers empty) and
  the zone record's marker list (head at +0x20) is non-empty, each list entry is drawn as that
  marker: world matrix = uniform scale 5, then translation by (entry block position (entry
  ints at +8, +0xc, +0x10) − pivot block position); material (1, 1, 1, 1); view = the block view
  stored at map object +0x44 by the **previous** call (it is rewritten later in this call;
  "State left behind"); projection = the marker projection (§5.1.3). The entries are the
  land-border markers built with the tile (§5.1.6 step 4(c)).

Zones without a tile (never generated or loaded in range of the map worker, §5.1.6) draw
nothing on the minimap. Nothing else of the world is drawn: **no water pass** (water and plants
exist only as colours inside the tile and land pictures), no statics, no clouds, no sky.

**Creature markers** (both paths, after the terrain; 0x5ffeea … 0x601c78). The creatures of
the world list ([map object +0xac] + 4) are collected when

* the controller kind (creature +0x60) is 0 (players), or
* the controller kind is 1 and appearance flag 0x2000 (creature +0x7e; believed to be the
  mission-target flag) is set, or
* *s* ≥ 1.6 (0x71e068) and the creature's class (+0x140) is 0x80 … 0x87 (the non-combat town
  NPCs), whatever its controller kind,

and both |Δx| and |Δy| between the pivot and the creature's position (+0x10) are at most
max(*r*, 1)·256 blocks. Each collected creature gets a depth key computed with the block view
just stored at map object +0x44 in this call; the list is then sorted so that the **farthest
marker is drawn first** (ascending by the negated view depth; 0x5fa9e0, 0x60030a). Each
creature is then drawn in two passes, placed at its displayed position (+0x1350) relative to
the pivot, in blocks:

| Creature | Model(s) (`alpha-creatures-models.md` Appendix A) | Turned about z by |
|---|---|---|
| controller kind 1 | 2562 `skull` | the creature's body yaw (+0x1374, element 2) |
| controller kind 3, class 0x80 / 81 / 82 / 83 / 84 / 85 / 86 / 87 | 2452 `shop-sign` / 2453 `weapon-shop-sign` / 2454 `armor-shop-sign` / 2455 `identifier-sign` / 2451 `inn-sign` / 2456 `smithy-sign` / 2457 `carpentersshop-sign` / 2458 `weavingmill-sign`, at half size | +ψ (the negated rotation z, so they always face the viewer) |
| any other controller kind (players) | the creature's head model, then its hair model (appearance +0x14, +0x16) | the creature's body yaw |

Each model is centred on its middle. **Pass 1** (outline): depth writes off (render state 14
= 0, 0x60098c), material (0, 0, 0, 1), uniform scale 5/*s*. **Pass 2**: depth writes on
(render state 14 = 1, 0x6015a3), uniform scale 4/*s*, material (1, 1, 1, 1), except the hair
model, whose material is the hair colour ÷ 255 with alpha 1. Dividing by *s* keeps the markers
the same size on screen at every zoom (5/8 and 1/2 view unit per voxel). Both passes of one
creature are drawn before the next creature. View = the block view stored in this call (map
object +0x44), projection = the marker projection.

#### 5.1.3 Matrix chain (0x5fc1b0 start, 0x5fc1f8 … 0x5fcc77)

Row vectors throughout (clip = *v*·World·View·Proj, as Direct3D 9; `alpha-rendering.md` §5
uses the same matrices).

**Projection P** (0x5fc217 … 0x5fc6c0). With *y*ₛ = 1 / tan 22.5° (the double π/8 at 0x6fd098)
= 2.4142136, *x*ₛ = −*y*ₛ / (W/H) (**negated**, the same x mirror as the world camera, 0x4304a7,
so the image is not mirrored), *o*ₓ = 2·(x₀ − W/2)/W and *o*ᵧ = 2·(y₀ − H/2)/H:

| | col 0 | col 1 | col 2 | col 3 |
|---|---|---|---|---|
| row 0 | *x*ₛ | 0 | 0 | 0 |
| row 1 | 0 | *y*ₛ | 0 | 0 |
| row 2 | *o*ₓ | *o*ᵧ | 0.9001 | 1 |
| row 3 | 0 | 0 | −0.10001 | 0 |

This is a left-handed perspective with near 0.1 and far 1000.1 (constants 1.0001 at 0x6fd094
and 0.10001 at 0x6fd090) followed by a clip-space shift of (*o*ₓ, *o*ᵧ, −0.1) (0.1 at 0x6fcb7c):
the centre of projection moves to (*o*ₓ, *o*ᵧ) in NDC and every depth is pulled 0.1 nearer
(NDC z = 0.9001 − 0.10001/z, so nothing is clipped by the far plane).

NDC y points up, so for the minimap *o*ᵧ = 1 − 340/H > 0 puts the centre **170 px below the top
edge**: the miniature is centred at pixel **(W − 240, 170)** in top-left window coordinates,
near the **top-right** corner; in 1280 × 720 the centre is (1040, 170).

**View V** (0x5fc6fc … 0x5fcc77), built by left-multiplying onto the identity in this order:
translate (0, 0, 300) (300 at 0x6fce68) → uniform scale *s* on rows 0–2 (skipped when *s* is
exactly 1) → turn about z by 0 (a constant zero angle; the rotation's y element is never used)
→ turn about x by rot.x → turn about z by rot.z (degrees converted with 0x6fcda4 = π/180). So a
point *p* in map units goes through, in this order:

1. about z by θ = rot.z (−ψ): x′ = x cos θ − y sin θ, y′ = x sin θ + y cos θ;
2. about x by φ = rot.x (−120° on the minimap): y″ = y′ cos φ − z sin φ, z″ = y′ sin φ + z cos φ;
3. × *s*;
4. z += 300.

*s* is therefore a **view** scale applied after the turns, not a model scale. The camera forward
(sin ψ, −cos ψ, 0) maps to straight up on screen and the camera right to screen right; world up
tilts to (0, 0.866, −0.5) in view space, so the miniature is seen from 30° above its ground
plane. 300 is the view depth along +z (the lens looks down +z, w = z).

**Marker projection** (0x5fc6cb … 0x5fc6f3; used at 0x5fedde and 0x601264 … 0x601c1e). At its
start the routine copies the projection stored at map object +4 by the **previous** call and
subtracts 80 (0x6fce50) from its row 3, column 2. This projection is used only for the zone
markers and creature markers; it pulls them 80 clip units nearer (NDC z − 80/w, about −0.27 at
w = 300), so they draw over the terrain. Because it is the previous call's matrix it lags one
call behind the current projection (and on the first minimap frame after a world-map frame it
is the world map's projection).

**World matrices**: as given per path in §5.1.2 (land: translate then ×32; zone tile:
translation t only; markers: block positions with the block view).

#### 5.1.4 Draw state

| State | Value | Where |
|---|---|---|
| Depth buffer | cleared to 1.0 just before | caller 0x4baa93 |
| Depth test | on (render state 7 = 1, set by the routine); comparison function as left by the frame (less-or-equal, `alpha-rendering.md` §1.3) | 0x5fcd43 |
| Depth write | terrain: as the world pass leaves it (on); creature markers: off in pass 1, on in pass 2; left on | 0x60098c, 0x6015a3 |
| Culling | as the world pass leaves it (`alpha-rendering.md` §1.3) | — |
| Blending | as left by the frame; the caller sets the pixel alpha constant to 1 (0x4baa28), so the result is opaque (`alpha-rendering.md` §5) | 0x4baa28 |
| Shaders | world vertex shader + world pixel shader for everything (bound by 0x447d10); the water shader is never bound | 0x5fcd4f, 0x5ffed2 |
| Fog | effectively off: the fog-scale constant is set from 10⁶ for the terrain and from 10⁹ for the creature markers (the constant is 1 divided by the argument) | 0x5fcd62, 0x5ffee5 |
| Point lights | all ten cleared (zero entries uploaded) | 0x5fce1f |
| Terrain lights | **not set by the routine**: the frame's terrain lights stay in force (last uploaded at 0x4b1a9c: front P4, back P5, ambient P6, sun direction; `alpha-rendering.md` §4), so the minimap darkens at night; material alpha is 1, and tile vertices carry alpha 1 | 0x4b1a9c |
| Creature-marker lights | front (1, 1, 1), back (0.2, 0.3, 0.4), ambient (0.4, 0.4, 0.4), direction (0, 0.5, 1) normalised | 0x600502 … 0x600633 |
| Darkness, white, shininess | not set by the routine (white = 0 from the caller; the others as left by the frame, unresolved: U-F2-2) | — |
| Scissor | see below | |

The routine calls SetScissorRect with {left W − 320, top 320, right W − 20, bottom 20}
(0x5fcd30), but no code in `Cube.exe` enables the scissor test (Direct3D's default is off): the
only use of render state 174 is the routine's own **disable** at 0x5ffde5 after the terrain. The
rectangle has no effect and the miniature is not clipped; in the worked example it spans
x ≈ 880 … 1190.

**State left behind** (0x5ffdd5 … 0x5ffecc, 0x600318 … 0x6004fe): map object +4 = P; +0x44 = V
with rows 0–2 multiplied by 1/8 (a view that takes **block** offsets from the pivot; used by the
markers and by the map overlay 0x4c9680); +0x84, +0x88 = the horizontal offset, in blocks
relative to the pivot, of a point 300 blocks from the local player along the horizontal
direction from the world centre (2²³, 2²³) to the local player, and +0x8c = 0 (its reader is
unresolved: U-F2-3). Scissor test off, depth write on, fog-scale constant from 10⁹, the marker
lights above. The map object's millisecond counter +0x8000b8 is advanced by the frame time step
at the **start** of every call (0x5fc211).

The lighting front (1, 1, 1), back (0.2, 0.3, 0.4), ambient (0.4, 0.4, 0.4), direction
normalise(0.5, 0.4, −0.6) set **after** the minimap call (0x4babb4 … 0x4bac7d) does not light
the map; it precedes the quick-item wheel's drawing (§5.7).

**Per frame:** clear depth → pick *s*, *r* from the map zoom → advance the map counter → build
P and V → scissor rectangle, depth test on, world shaders, fog off, point lights off → draw the
land slab (blue-tinted) or the zone tiles (white material) with their zone markers → disable
the scissor test → store P and V/8 → set the marker lights → draw the collected creature
markers, farthest first (black outline pass, then the coloured pass, per creature).

#### 5.1.5 Worked example

1280 × 720, *d* = 1 (so *s* = 0.8, *r* = 1: 3 × 3 zones), ψ = 30° (rotation (−120°, 0, −30°)),
the player standing at the centre of its zone at height 100 blocks. Take the point at the
zone's origin corner at ground height 100 (100 blocks = 12.5 map units; tile vertices lie on
8-block steps, this point only checks the arithmetic). In map units relative to the player:
*p* = (−16, −16, 0).

* *x*ₛ = −1.3579951, *y*ₛ = 2.4142136, *o*ₓ = 0.625, *o*ᵧ = 0.5277778.
* View matrix V (rows): (0.69282, 0.2, 0.34641, 0), (0.4, −0.34641, −0.6, 0),
  (0, 0.69282, −0.4, 0), (0, 0, 300, 1).
* After step 1: (−21.8564, −5.8564, 0); step 2: (−21.8564, 2.9282, 5.0718); step 3:
  (−17.4851, 2.3426, 4.0574); step 4: view (−17.4851, 2.3426, 304.0574).
* Clip = (213.7806, 166.1302, 273.5821, 304.0574); NDC (0.70309, 0.54638, 0.89977);
  pixel (1089.98, 163.30).
* Checks: the pivot (0, 0, 0) → clip (187.5, 158.3333, 269.9300, 300), pixel (1040, 170).
  The far corners of the 3 × 3 block: (−48, −48, 0) → (1186.04, 150.43); (48, 48, 0) →
  (881.61, 191.22); (48, −48, 0) → (1004.64, 104.01); (−48, 48, 0) → (1087.99, 259.54).
  A point 10 units along the camera forward, (5, −8.6603, 0) → (1040, 158.67): straight up
  from the centre.

### 5.1.6 Map tiles and land pictures

Everything here is built by the client on its **map worker thread** (0x469590), never by the
renderer, and stored in the map object (G+0x800d44, "M" below). "÷₀" below is integer division
truncating toward zero; "floor" rounds down.

**Records.**

* *Map region record* (M+0xb0, 1024 × 1024 pointers, index rx·1024 + ry, one per 64 × 64
  zones; 0x35a00 bytes): 4096 **zone map records** of 0x34 bytes at
  ((zx mod 64)·64 + (zy mod 64))·0x34, then 64 copies of the world's 0x68-byte location
  records (one per 8 × 8-zone map cell) at +0x34000.
* *Zone map record* (0x34 bytes):

  | Off | Meaning |
  |---|---|
  | +0x04 | int **base height** B of the tile, in map units (8 blocks) |
  | +0x08 | pointer to the **tile** voxel model (null = none in memory) |
  | +0x10 … +0x1f | 16 bytes copied from the world region's per-zone table (0x4a6ad0; regions spec §6.1) |
  | +0x20, +0x24 | list head and count of the **zone markers** (entries of three ints) |
  | +0x28 | byte **dirty**: the zone was (re)generated since the tile was built |
  | +0x2c | int **rise-in timer**, ms |
  | +0x30 | byte flags: bit 0 = **discovered**, bit 1 = **a tile exists** (in memory or in `map_<name>.db`) |

* *Land record* (M+0x4000b0, 1024 × 1024 pointers, the same index as the world's region
  cells W+0x4000bc; 0x34 bytes): +0x04 … +0x1f = a copy of the region cell's 28 bytes (point
  x, point y, fire byte, moisture, warmth, cell id, **elevation at +0x1c**; regions spec §2.1),
  +0x20, +0x24 = picture origin (zone x, zone y), +0x28 = byte **visited**, +0x2c = the
  **land picture** voxel model, +0x30 = byte **rebuild requested** (not saved).

#### The map worker (0x469590)

A loop that runs while the byte G+0x800584 is non-zero, holds the lock G+0x8005e8 for each pass
and sleeps 10 ms between passes. Each pass:

1. **Centre zone** (c_x, c_y) = the client's current zone (two ints at G+0x2bc; meaning
   inferred) plus the world-map pan (floats G+0x1000e4c, G+0x1000e50, in blocks) ÷ 256,
   truncated toward zero per axis (0x4695dc … 0x46962c). Its region is c ÷₀ 64.
2. **Ownership check.** The work of steps 3–5 is done only while the map belongs to the open
   world: the map's world id and name (G+0x800a50, G+0x800a54) must equal the world's
   (G+0x800448, G+0x378). The check is made under the lock G+0x8005d0 at the start of the pass
   and again before each region and land step; a mismatch skips the rest of steps 3–5 for this
   pass (0x46962f … 0x46969a).
3. **Region records** for the 7 × 7 regions within ±3 of the centre region (x outer, y inner),
   each under the world lock: 0x603230 creates any that is missing (below).
4. **Land pictures** for the same 7 × 7 indices, each under the world lock: 0x6024d0 (below).
5. **Tiles**, 10 attempts per pass: scan zones x from c_x − 10 to c_x + 9 (outer), y from
   c_y − 10 to c_y + 9 (inner), under the world lock and the map lock; a zone **needs work** when
   its zone map record is missing, or when it has no tile or is dirty **and** either flag bit 1
   is set (a saved tile can be loaded) or the zone is loaded in the world (W+0xbc region →
   +0x10018 zone pointer) (0x4699e0 … 0x469ac7). Take the needy zone with the smallest
   (zx − c_x)² + (zy − c_y)² (strictly smaller wins, so the first in scan order on ties) and
   call 0x603a00 for it under the world lock.
6. Whenever more than 1000 ms have passed since the last unload (also when step 2 failed):
   **unload** (0x5fbed0): free tiles of zones more than 10 zones from the centre in x or y
   (0x6022d0; the tile is already in the database); save (`reg` record and the `discovered`
   counter, 0x602160) and free region records more than 8 regions away; save (0x6050b0) and
   free land records more than 8 away.

#### Region record creation (0x603230)

Allocate the region record; if `reg<rx>_<ry>` exists in `map_<name>.db`, read its header and its
64 × 64 cells into each zone record (u8 → +0x30, four u8 → +0x10 … +0x13, u32 → +0x14,
u32 → +0x18, u8 → +0x1c) and its 8 × 8 location copies (`alpha-saves.md` §6.3). Then refresh
+0x10 … +0x1f of every zone from the world region's per-zone table and the location copies from
the world (when that world region is present), and write the `reg` record back at once
(0x605420). Tile pointers start null: **tiles are not read here**; a zone whose bit 1 is set is
picked up by worker step 5 and loaded from its `tile` record on demand.

#### Discovery and dirtiness

* **Discovered** (bit 0 of +0x30; 0x5fc160, under the lock M+0x8000c0). Every in-game frame
  (0x48cf65 … 0x48d033) the client marks the zones under the four points (player x ± 64,
  player y ± 64) blocks, i.e. every zone touched by a 128 × 128-block square centred on the
  player. Setting the bit for the first time adds 1 to the **discovered counter** M+0x8000bc
  (the `discovered` key). Only zones whose map record exists are marked. Other callers
  (0x4887aa; 0x48d204, for points of a list of map locations) mark further zones (not traced,
  unresolved: U-F2-4).
* **Dirty** (+0x28 = 1) is set by the zone loader, under the map lock, right after a zone is
  generated or loaded into the world (0x46acf8, after 0x5e4850).
* **Tiles are not limited to discovered zones.** A tile is built for every zone that the world
  generates or loads while it lies in the worker's 20 × 20-zone window, discovered or not, and
  stays in `map_<name>.db`. The discovered bit only shades land pictures (below) and the world
  map's extras colouring (0x5fd949 …, §5.2). The minimap's zone path tests only the tile
  pointer (+0x08).

#### Building or loading one tile (0x603a00, zone (zx, zy))

1. Stop unless the world is ready (W+0xa4 ≠ 0), the zone is in 0 … 65535 and its map region
   record exists. Refresh +0x10 … +0x1f (0x4a6ad0, under the map lock) and the location copy
   for map cell (zx ÷₀ 8, zy ÷₀ 8) (0x487da0 → 0x5fb9f0).
2. Let Z = the world's zone object if loaded (0x434d10), else none. **Stop if a tile is in
   memory and (it is not dirty or Z is none).**
3. **Load**, only when no tile is in memory and `tile<zx>_<zy>` exists: read the version
   (ignored), B → +0x04, the X, Y, Z sizes; if all are ≥ 1, read a u32 *c* and, if *c* ≠ 0,
   *c* bytes of zlib data, make a voxel model of X × Y × Z with those RGB bytes and mesh it
   (0x4e6a20, 0x4e75c0, 0x4e7870); under the map lock set flag bit 1, +0x08 = the model and
   **timer +0x2c = 250**. Then read a u32 *n* (always present), clear the marker list and
   append *n* entries of three i32. If the zone is not dirty, or Z is none, stop; otherwise
   continue with the build.
4. **Build** (Z present; 0x603f51 … 0x604b91), from Z's 256 × 256 columns (terrain spec §1.2;
   per column: bottom b = +0x10, count n = +0x1c, **floor level F = +0x14**):
   * lo = max(0, min over all columns of (F − 1)); hi = max(0, max(F of column (0, 0),
     max over all columns of (b + n))). **B = lo ÷₀ 8**, tile size
     **32 × 32 × ((hi − lo) ÷₀ 8 + 1)** (0x603f51 … 0x604001). The Z size is the zone's
     relief in 8-block layers.
   * Clear dirty (+0x28 = 0), clear the marker list, allocate the model (all voxels (0, 0, 0)
     = empty).
   * For each voxel column (vx, vy), vx outer 0 … 31, vy inner 0 … 31, covering block columns
     x = 256·zx + 8·vx + sx, y = 256·zy + 8·vy + sy (sx, sy = 0 … 7):

     **(a) Surface colour and height** (0x6040e0 … 0x604335). For the 64 columns (sx outer,
     sy inner), with f32 sums starting at 0:
     * if b + n ≤ 0 (nothing stored above sea level): add the colour of the game's static
       default block, whose type is water (0x76de0c, set at 0x6faf20), i.e. the **water colour
       (80, 100, 255)**; height 0;
     * else scan the stored blocks from the top (z = b + n − 1) down: the first block of
       non-zero type gives its colour — **(80, 100, 255) for types 2 and 3** (water, lake bed),
       else its own RGB bytes (type 1 included) — and height z; if instead an empty stored
       block is met at z = F − 1, the colour is Rk(x, y, F) (terrain spec §6.6) and the height
       F − 1; if the scan runs out, colour Rk(x, y, F) and height b − 1;
     * also track m = min F over the 64 columns.

     Surface colour S = sum × (1/64) per channel; mean height h̄ = height sum × 1/64 (f32);
     **top layer T = (trunc(h̄) ÷₀ 8) − B** (0x6042fb … 0x604335). If 0 ≤ T < Z size, voxel
     (vx, vy, T) = trunc(S) per byte. (Four 16 × 16 quadrant sums of S are also accumulated
     at 0x604650; no use of them was found.)

     **(b) Body below the top** (0x6046a3 … 0x604b5f). For each layer k from
     max(0, (m ÷₀ 8) − B − 1) up to T − 1: average the 8 × 8 × 8 blocks at
     z = 8·(B + k) + sz (sx outer, sy, sz inner), read through the world's zone grid like the
     mesher reads blocks (terrain spec §1.4): no loaded column, or z below the column's
     bottom → the static **type 1 block**; a stored block, except that an empty stored block
     at z ≤ 0 without bit 0x40 reads as water; above the stored range → water if z ≤ 0, else
     air. Skip type 0. Count the rest (*c*) and add: type 1 → **Rk(x, y, z)**; types 2, 3 →
     (80, 100, 255); others → their RGB. If *c* > 0, voxel (vx, vy, k) = trunc(sum × (1/*c*))
     per byte, else it stays empty. **There is no fill threshold**: one solid block makes a
     full voxel. Layers above T are never filled, so the tile is a height field (overhangs,
     arches and anything above the mean surface are lost); trees and other statics are not
     blocks and do not appear; water shows only as its flat blue colour.

     **(c) Zone markers** (0x604371 … 0x60449f), only when vx mod 4 = 0 and vy mod 4 = 0 (an
     8 × 8 grid, 32 blocks apart): with x₀ = 256·zx + 8·vx, y₀ = 256·zy + 8·vy and
     owner = the world's region-cell owner (0x477e10, regions spec §3.4), if
     owner(x₀, y₀) ≠ owner(x₀ + 32, y₀) or owner(x₀, y₀) ≠ owner(x₀, y₀ + 32), append (under
     the map lock) the entry (x₀, y₀, max(F of column (8vx, 8vy), 0)), absolute block
     coordinates. **The markers trace land borders**: the minimap draws each as the marker
     tile scaled by 5 (§5.1.2), giving a dotted line every 32 blocks along the border between
     two lands. A zone inside one land has none.
   * Mesh the model (0x4e7870), then write `tile<zx>_<zy>`: u32 version (M+0x8000f8, set to 1
     by the map object's constructor 0x5fae40), i32 B, 3 × i32 size, u32 *c* + zlib bytes (when
     all sizes > 0), **u32 marker count + 12 bytes per marker**. Under the map lock: if there
     was no tile in memory, **timer +0x2c = 250**; otherwise delete the old model (the timer is
     left alone, so a rebuild does not rise again); set flag bit 1; +0x08 = the new model.
     Finally write the `discovered` counter.

The `tile` record's second field is the base height B (map units); its last u32 is the marker
count, followed by that many (x, y, z) i32 triples.

**Rise-in.** With the minimap's h = B − 0.1 × timer and the timer counting down by the frame
time (§5.1.2), a newly loaded or newly built tile starts **25 map units (200 blocks) below**
its place and rises into it in 250 ms.

**Meshing and light.** The tile is an ordinary voxel model of the game's `cube::Sprite` class
(constructor 0x4e6a20: shade colour (0, 0, 0); the word 0x100 at +0x54, i.e. byte +0x55 = 1)
meshed by 0x4e7870 as `.cub` models are (`alpha-rendering.md` §11.1). **No light is baked**;
the frame's terrain lights shade it when drawn (§5.1.4). Positions are in voxel units 0 … 32
across, z from 0 at layer B.

#### Land pictures (0x6024d0, land cell (lx, ly))

1. Stop unless the world is ready and 0 ≤ lx, ly < 1024. If no land record is in memory and
   `land<lx>_<ly>` exists, create the record from it (version, origin +0x20/+0x24, visited
   +0x28, picture, then the 28 cell bytes into +0x04 … +0x1f; rebuild flag 0) and continue to
   step 2. If a record is in memory and its **rebuild flag +0x30 is 0, stop**.
2. Build only when the world has region cell (lx, ly) **and all 8 neighbours** (all within
   0 … 1023 and present; else stop). First, under the map lock, refresh the zone records'
   +0x10 … +0x1f and location copies of map region (lx, ly) if it is loaded; create the land
   record if missing, and copy the world cell's 28 bytes to +0x04 … +0x1f (so
   **+0x1c = the cell's elevation**).
3. **Ownership of a zone** (zx, zy) (0x602b35 … 0x602c93):
   * **Point.** x = 256·zx, y = 256·zy (the zone's corner block, not its centre). With
     D = D(x, y) from 0x5eefa0 (regions spec §3.1), the point is
     p = (trunc(f32(x + D's first component)), trunc(f32(y + D's second component))), whole
     blocks, the displaced point of regions spec §3.3. It is passed on as 64-bit p·65536 (no
     fraction).
   * **Distance** to a cell c (0x5eeee0; arguments: the cell, p.x·65536, p.y·65536): the
     regions spec §3.3 squared distance before truncation, kept as a **float**. c.x, c.y are
     the cell record's +0x00/+0x04 (whole blocks).
   * **d₀** = the squared distance to the land's own cell (the world cell (lx, ly), copied in
     step 2), **truncated to an integer** (and converted back to float for the comparison).
   * **Neighbours.** All 8 cells around (lx, ly) compete, each read from the world's
     region-cell grid (W+0x4000bc, index (lx + i)·1024 + (ly + j)): outer loop i = −1, 0, 1,
     inner loop j = −1, 0, 1, skipping only (0, 0). For each one the squared distance is
     computed as above (float, not truncated); if **it is < d₀** (strict; 0x602c77) the zone
     is **not owned** and the test stops. If all 8 are passed the zone is owned; equal values
     belong to both lands. Side and diagonal cells are treated identically, and there is no
     early accept.
   * **Missing neighbour in pass 1:** an index outside 0 … 1023 or a null grid entry makes
     the zone **not owned**. Step 2 has already required all 8 cells, so in practice this does
     not happen.

   Pass 1 (0x602ab7 …) scans zx from 64·lx − 64 to 64·lx + 128 **inclusive** (outer loop)
   and, for each, zy from 64·ly − 64 to 64·ly + 128 inclusive (inner loop), and takes the
   bounding box (min/max zone x and y) of the owned zones. If none is owned, stop. Then
   +0x30 = 0.
4. Picture = voxel model of (max x − min x + 1) × (max y − min y + 1) × 1 (0x4e75c0; voxel
   (vx, vy) at byte 3·(vx + vy·width)); origin **+0x20 = min zone x, +0x24 = min zone y**.
   Pass 2 (0x602e03 …) scans zx from 64·lx − 64 to 64·lx + 128 **exclusive** (outer), zy
   likewise (inner), and repeats the step 3 test with the same point, distances, neighbour
   order and strict comparison (0x602f75). The one difference: a **missing or out-of-range
   neighbour is skipped** (the test goes on with the next neighbour) instead of rejecting the
   zone. For each owned zone it writes voxel (zx − min x, zy − min y, 0) (a write outside the
   model goes to a scratch location and is lost):

   | Zone map record | Voxel |
   |---|---|
   | missing (region not loaded), or flags +0x30 = 0 | (200, 200, 200) |
   | flags ≠ 0 but not discovered (only "tile exists") | (220, 220, 220) |
   | discovered (bit 0) | (255, 255, 255) |

   Unowned zones stay empty. (A zone owned only on the inclusive edge 64·lx + 128 widens the
   box but is never painted.)
5. Mesh it like a tile, swap it into +0x2c under the map lock (deleting the old one) and write
   `land<lx>_<ly>` at once (0x6050b0): u32 version, i32 +0x20, i32 +0x24, u8 +0x28,
   3 × i32 size, u32 *c* + zlib RGB, then the 28 bytes +0x04 … +0x1f.

On the minimap the slab is tinted blue (§5.1.2), so discovered zones show as the lightest
blue, zones with only a tile slightly darker, and the rest darker still; the mesher's occlusion
darkens the slab's rim.

**Visited and rebuild requests** (0x48d51c … 0x48d5e1, every in-game frame, under the map
lock): with C = the world's owner cell of the player's block column (0x477e10) and L = the
map's land record found for the player (0x601cc0: among the land records of the 3 × 3 cells
around the player, the one with the smallest truncated squared displaced distance, first found
on ties; none if any of the nine is missing or has no picture), if both exist and L's copied
point (+0x04, +0x08) equals C's point: L+0x28 = 1 (**visited**; the minimap's land path draws
only visited lands), and if more than 2000 ms have passed since the last request (one global
time, 0x76b0fc), L+0x30 = 1 and the time is updated. So the picture of the land the player is
in is rebuilt and resaved at most every 2 s, picking up newly discovered zones.

The `land` record's two i32 are the picture origin in zones, the u8 is the visited flag, and
the 28 trailing bytes are the region cell (point x, point y, fire byte + 3 padding, moisture,
warmth, id, elevation).

#### Worked examples

* **Tile size.** Lowest floor F = 40 (so lo = 39), highest b + n = 120: B = 39 ÷₀ 8 = 4,
  Z size = (120 − 39) ÷₀ 8 + 1 = 11. Lowest floor 49, highest 120: lo = 48, B = 6,
  Z size = 72 ÷₀ 8 + 1 = 10.
* **Surface voxel.** 48 columns whose top block is grass of type 4 or above with colour
  (60, 160, 40) at z = 70 and 16 water columns (b + n ≤ 0): S = ((48·60 + 16·80)/64,
  (48·160 + 16·100)/64, (48·40 + 16·255)/64) = (65, 145, 93.75) → bytes (65, 145, 93);
  h̄ = 48·70/64 = 52.5, T = 52 ÷₀ 8 − B = 6 − B (layer 2 with B = 4).
* **Body voxel.** 512 blocks of which 200 are of a coloured type (not 0–3) with colour
  (120, 120, 130) and 312 air → (120, 120, 130). 256 such blocks of colour (100, 100, 100) and
  256 water (z ≤ 0) → (90, 100, 177.5) → (90, 100, 177). All 512 air → stays empty.
* **Body range.** m = 40, B = 4: layers from max(0, 5 − 4 − 1) = 0 up to T − 1.
* **Rise-in.** Timer 250 → h = B − 25; after 100 ms of frames, 150 → B − 15; after 250 ms, B.
* **Markers.** A zone wholly inside one land: 0 entries. A zone crossed by one idealised
  straight north–south border: at most 8 entries, all with the same vx (the sample column
  within 32 blocks west of the border).
* **Land voxel.** Owned zone with flags 0x03 → (255, 255, 255); 0x02 → (220, 220, 220);
  0x00 or record missing → (200, 200, 200). Drawn with tint (0.2, 0.4, 1)·*k* (§5.1.2):
  discovered at *k* = 1 → (51, 102, 255) before lighting and occlusion.

### 5.2 World map

The world map is not a picture: it is the minimap's 3D miniature (§5.1, map renderer 0x5fc1b0)
drawn full-window, centred on the player plus a pan offset, at the zoom the player picks, with
some extra content switched on, plus a code-drawn text overlay. While it is open the frame draws
**no world at all** (no terrain, water, clouds, sun, stars or creatures), only the sky gradient
behind the map. Routines: key handler 0x47e1b0; update 0x488ee0 (pan 0x48b62b … 0x48b7fc,
open/close bookkeeping 0x497333 … 0x497432); mouse wheel 0x47ef40; mouse movement 0x47ea00;
mouse release 0x47ddd0; frame 0x4ac260 (map branch 0x4adede … 0x4ae279); map renderer
0x5fc1b0; overlay MapOverlayWidget (constructor 0x4c95a0, draw 0x4c9680).

State used below (game object G):

| Field | Meaning | Initial |
|---|---|---|
| G+0x8006e4 | byte **map mode** | 0 |
| G+0x800de4 | byte **teleport mode** (the map was opened from a portal) | 0 |
| G+0x800dd4, +0x800dd8 | int pair **hovered portal zone** (reset and rewritten by the overlay every map frame) | (−1, −1) |
| G+0x800ddc, +0x800de0 | int pair **selected portal zone** | (−1, −1) |
| G+0x1c4, G+0x1c8 | current and target **map zoom** *s* (`alpha-physics-camera.md` §7.1) | 1, 1 |
| G+0x1000e4c, +e50, +e54 | float triple **pan**, in blocks (z stays 0) | 0, 0, 0 |
| G+0x8008a0 | the **map root** container, a zero-size top-level container of the interface made at start-up (0x45c090); every non-map frame hides it (0x4ae357 … 0x4ae369) | — |
| G+0x8008a4 | the overlay's zero-size container inside the map root (0x45c0d7) | — |
| G+0x8008a8 | the **cursor label** container (made at 0x461e29; `cursor.plx` is loaded into it at 0x461e7b) | — |

#### 5.2.1 Opening and closing

* **M** (key-press case 0x47e7db): map mode := not map mode, subject only to the common
  key-press conditions of §11.2.
* **Esc** (case 0x47e770): map mode := 0, and the menu-button row flag G+0x8008f2 is toggled
  (§7).
* **R at a portal** (0x49735f … 0x497432): in the update, when the R flag (G+0xf) is set and was
  clear on the previous update (the previous value is kept at 0x76b138, stored at 0x49b596),
  the quick-item wheel (G+0x800a40) is closed, the local player's health (+0x16c) is above 0,
  the interaction target index G+0x800a84 is not negative, and that target resolves (0x5a0910)
  to an object whose type is **45** (0x2d), the update sets map mode := 1 **and** teleport
  mode := 1. Type 45 is believed to be the portal, whose prompt is "[R] Teleport" (§5)
  (unresolved: U-F3-3).
* **A teleport** (§5.2.6) closes the map.
* **Bookkeeping while closed.** Every update in which map mode is 0: teleport mode := 0 and the
  selected portal zone := (−1, −1) (0x497333 … 0x49735a), and the pan := (0, 0, 0) under the
  lock G+0x8005d0 (0x48b7b8 … 0x48b7fc). The map therefore always reopens centred on the
  player. The zoom is **not** reset: it keeps its last value across closing and reopening.
* While the map is open the game is in **cursor mode** (0x47f1d0 returns true when map mode is
  set, 0x47f1e9): the cursor is shown and not re-centred (§11.2), and the camera's focus point
  is not updated (`alpha-physics-camera.md` §7.3 item 5). The cursor-label container
  G+0x8008a8 is shown exactly while cursor mode holds and is moved to the cursor every update
  (0x48cbc3 … 0x48cc0d).

#### 5.2.2 Controls: zoom, pan, turn and tilt

**Zoom (mouse wheel only; 0x47ef40).** The window procedure turns each wheel message into
n = the sign of the wheel delta: +1 for a positive delta (wheel rotated away from the user),
−1 for a negative one (0x4c856c … 0x4c858b). With the map open:

* n < 0: target zoom G+0x1c8 ×= 0.9 (0x6fd31c; zoom **out**);
* otherwise: target zoom ÷= 0.9 (zoom **in**);
* then clamp: above 10 (0x745e74) becomes 10, below 0.01 (0x6fcda0) becomes 0.01.

The current zoom G+0x1c4 eases towards the target every update (0x496611 … 0x496627) with the
same easing as the camera values (`alpha-physics-camera.md` §7.3 item 2): with the update's
whole-millisecond step dt, weight w = 1 − 0.996^dt (rate 0.004, built by repeated steps in
double precision, 0x4ac150), current := current·(1 − w) + target·w. No key changes the zoom.
With the map closed the same wheel message changes the camera distance G+0x1c0 instead (by
−2n, clamped to [0, 14]; 0x47efa6 … 0x47eff3). The current zoom *s* is passed straight to the
map renderer as its **view scale** (there is no zoom table as for the minimap), and the
content switches at these values:

| *s* | Effect |
|---|---|
| *s* < 0.35 | **land pictures** instead of zone tiles (renderer's land path, §5.1.2; 0x6fd8d8 = 0.35) |
| *s* ≥ 1.6 | the town NPC signs join the creature markers (§5.1.2; 0x71e068 = 1.6) |
| *s* > 2 | the overlay draws **zone labels** (district and portal names, §5.2.5 pass A; 0x745e10 = 2) |
| 8/*s* | size of the mission and town markers, clamped to [2, 10] (§5.2.4) |

From the start value 1: 10 notches down give 0.9¹⁰ = 0.349 (land pictures); 5 notches up give
1.694 (signs); 7 notches up give 2.091 (zone labels); the limits are reached after 22 notches up
(10.15, clamped to 10) and 44 notches down (0.0097, clamped to 0.01).

**Pan (middle mouse button held; 0x48b6ca … 0x48b7b5).** Every update while the map is open and
the middle-button flag G+0xa is set (§11.1), under the lock G+0x8005d0:

1. (Δx, Δy) = cursor position − cursor position of the previous frame, in window pixels (the
   interface object G+0x800710 keeps current +0xd4/+0xd8 and previous +0xdc/+0xe0;
   0x480d90, 0x480db0).
2. v = (Δx, −Δy, 0) × (4 / *s*) blocks (4 = 0x745e60).
3. v is turned about z by +ψ, the camera yaw (element 2 of the angle triple G+0x1a4; rotation
   built at 0x424610, applied as a row vector at 0x488e50), so the drag is aligned with the
   screen.
4. pan += v.

One pixel of drag moves the pan by 4/*s* blocks = 0.5/*s* map units; at the window centre a
map unit is about 2.9·*s* px tall in a 720-px window, so the map moves about 1.45 × as far as
the cursor (§5.2.7). The content follows the cursor's direction. No key pans the map.

**Turn and tilt (left button drag).** The mouse-movement routine 0x47ea00 runs its cursor-mode
branch with the map open: while the left-button flag (G+0x4) is set and the cursor is over no
widget (0x650ae0 returns 0), the cursor movement changes the camera's pitch target θ*
(G+0x1b0, by Δy × k × 0.005, where k is the integer G+0x18c, believed to be the mouse
sensitivity option; the sign is reversed when G+0x194, believed to be the invert option, is
set; clamped to [0, 180]) and yaw target ψ* (G+0x1b8, by −Δx × k × 0.005)
(0x47ed00 … 0x47eddc). The map uses
the current θ and ψ (§5.2.3), so a left drag **turns** the map about the vertical (ψ) and
**tilts** it (θ); the **middle** drag pans.

#### 5.2.3 The map frame (0x4adede … 0x4ae279)

The frame routine runs as usual up to the sky (`alpha-rendering.md` §1.3 steps 1–6): clear
colour (0, 0.44, 1) and depth, palette, the frame constants (including the set-up lights at
0x4ad957: front P4, back P5, ambient P5 and the sun direction, `alpha-rendering.md` §4 row
"Frame set-up and sky"), all ten point lights cleared, then the sky-gradient quad. The only
change there: the sky-gradient translation is computed from the pitch **90** instead of θ
(0x4ada17 … 0x4ada3d; the translation is (pitch − 80)/(−30), 0x448090), so the sky gradient is
the one a horizontal camera would see, (90 − 80)/(−30) = −1/3. Then, if map mode is set
(0x4adede):

1. **Rotation.** rot = (min(−θ, −110°), 0, −ψ) (θ = G+0x1a4, ψ = G+0x1ac; −110 = 0x702b84;
   0x4adeeb … 0x4adf0d, 0x4adf5d … 0x4adf82).
2. **Pivot.** Pan → fixed point: each component truncated after × 65536, as 64-bit
   (0x4adf13 … 0x4adf51); pivot = local player position (G+0x8006d0 +0x10) + that (0x42c800).
3. **Map renderer call** (0x4adf6c … 0x4ae004), with the minimap's argument order: pivot;
   projection centre x₀ = trunc(W·0.5), y₀ = trunc(H·0.5); window W = G+0x11c, H = G+0x120;
   rotation rot; view scale *s* = G+0x1c4; frame time step G+0x8006e8; **radius 16**; **map
   extras on**. What it draws is §5.2.4.
4. Material (1, 1, 1, 1) (0x4ae009 … 0x4ae043).
5. **Map-mode lights** (0x4ae048 … 0x4ae15e): front (1, 1, 1), back (0.2, 0.3, 0.4), direction
   normalize(0, 0.5, −1) = (0, 0.4472, −0.8944), ambient (0.4, 0.4, 0.4). They are set **after**
   the map renderer has returned, so they do **not** light the map; they light only the compass
   of step 7. The next frame's constants at 0x4ad957 replace them before the next map draw.
6. **Clear depth** to 1.0 (device Clear, depth only; the colour argument 0xFF6464FF is unused;
   0x4ae163 … 0x4ae187).
7. **Compass.** Model slot 2559 `compass2` (taken from the model list G+0x300 when it holds more
   than 2559 entries, else nothing is drawn) is drawn by the interface model routine 0x476660
   (the routine of §8.1.6) at window pixel **(W − 60, 40)**, rotation (−120°, 0, −ψ), scale 0.003
   (0x4ae189 … 0x4ae21b). It turns with the camera yaw like the map, so it shows north. Its
   placement inside 0x476660 and the model's size are unresolved (U-F3-1).
8. **Interface.** The in-game root G+0x800884 is hidden (so the HUD, bars, chat and every other
   node under it disappear) and the map root G+0x8008a0 is shown (0x4ae220 … 0x4ae24b); the
   interface is drawn (0x650980), which draws the MapOverlayWidget (§5.2.5) and the cursor
   label.
9. EndScene, release the world lock (0x601e90 on G+0x2e4), and jump to the end of the frame
   (present, 0x4bbb5a). Nothing else of the world frame runs.

#### 5.2.4 What the map renderer draws with map extras on

Everything of §5.1 applies (matrix chain, units, draw state, creature markers), with these
differences.

**Projection and view.** x₀ = trunc(W/2), y₀ = trunc(H/2), so *o*ₓ = *o*ᵧ = 0 for even window
sizes: the pivot is at the **window centre**. The view turns about x by φ = min(−θ, −110°)
instead of a fixed −120°: world up maps to (0, −sin φ, cos φ), so the map is seen from
(−φ − 90)° above its ground plane: **30°** at θ = 120, **90° (straight down)** at θ = 180, and
never flatter than **20°** (θ ≤ 110). The scale step uses *s* = the map zoom.

**Terrain.** With *s* < 0.35 the **land path** (§5.1.2) with radius 16: the land pictures of the
33 × 33 land cells around the pivot's land cell (only visited lands with a picture), tinted blue
as on the minimap. With *s* ≥ 0.35 the **zone path** with radius 16: the 33 × 33 zones
(zx, zy) = pivot zone + (i, j), i, j = −16 … 16 (x outer, y inner). For each zone, in this order:

1. **Placeholder** (extras only; 0x5fd89d … 0x5fe2e3): when the zone has no map record, or no
   tile, or a tile that is still rising (timer +0x2c ≠ 0), model slot 2567 `maptile` is drawn
   scaled **×4** (one zone wide), placed at the zone's square like a tile (the translation t of
   §5.1.2), with its base at height z = (E − 100)·0.125 + rise − pivot z ÷ 8 map units, where E
   is the elevation (+0x1c) of the land record found for the zone's corner block (0x601cc0;
   E term 0 when none) and rise = (timer/250 − 1)·20 when a rising tile exists, else 0
   (0x5fd8ea … 0x5fd943; so the placeholder sinks 20 map units while the real tile rises in).
   Material (the tint), by the zone record's flags byte +0x30 (0x5fd949 … 0x5fda15):

   | Flags | Material |
   |---|---|
   | no record, or flags = 0 | (0.2, 0.3, 1, 1) |
   | flags ≠ 0, not discovered | (0.2, 0.35, 1, 1) |
   | discovered (bit 0) | (0.2, 0.4, 1, 1) |

   Material reset to (1, 1, 1, 1) afterwards. So unknown parts of the map show as flat blue
   blocks following the land's elevation, a little lighter where the player has been. The
   voxel content of `maptile` is unresolved (U-F3-1).
2. **Tile** (as on the minimap, rise-in included) with material (1, 1, 1, 1), **dimmed** to
   (0.6, 0.6, 0.6, 1) (0x5fe9f8 … 0x5fea4f; 0.6 = 0x6fd8dc) when the zone is **not discovered**
   (flags bit 0 clear), **except** the zone that is the target of an active mission: the
   world's location record of the zone's map cell (0x487da0 for (zx ÷ 8, zy ÷ 8)) has a
   non-zero int at +0x34, byte +0x41 = 1, and its int pair at +0x4c, +0x50 equals (zx, zy)
   (0x5fe9ba … 0x5fe9f6). These are fields of the record's mission area
   (`alpha-worldgen-locations.md` §1.2); their exact meaning is not settled here
   (unresolved: U-F3-3). Material reset afterwards.
3. **Zone markers** (land borders, as §5.1.2).

**Mission and town markers** (extras only; 0x5fee56 … 0x5ffdd0). After the terrain, for each of
the 64 location records (8 × 8, x outer) of the world region that holds the pivot (0x5cb0d0),
with the block view (map object +0x44) and the plain projection, material (1, 1, 1, 1), the
terrain's lights, scale k = clamp(8/*s*, 2, 10) (8 = 0x745e70; left unscaled when exactly 1,
which cannot happen inside the clamp; 0x5ff503 … 0x5ff54e):

* **Mission marker**, when the int at +0x34 is non-zero and byte +0x41 ≠ 2 (2 believed to mean
  finished, unresolved: U-F3-3): model slot 2560 `mission`, at (P − pivot) blocks, where P = the
  centre of zone (+0x4c, +0x50) (block 256·z + 128 per axis) when +0x34 = 1, else the record's
  centre (+0x00, +0x08), and P.z = the record's ground height (float +0x14) **+ 100 blocks**
  (0x5fef09 … 0x5fefcc). World matrix: translate by −(size x/2, size y/2, 0) (centred in x and
  y, its bottom at P), scale k, turn about z by T·0.02° (0.02 = 0x6fcda8; T = the map object's
  millisecond counter +0x8000b8; 0x5ff390 … 0x5ff3e7), then translate to P. It spins at
  **20° per second** (one turn in 18 s).
* **Town marker**, for every record of type 1 (int +0x18, a town), when the horizontal distance
  from the world's player entry (world object +0xb8, believed to be the local player;
  unresolved: U-F3-5) to the record's centre is more than 512 blocks (distance² > 262,144,
  0x703a1c; 0x5ff7b3 … 0x5ff8bf): model slot 2561 `city`, at the record's centre, ground height
  + 100 blocks, centred in x and y, scale k, not turned.

**Creature markers** as §5.1.2, with r = 16: creatures within 16·256 = 4096 blocks of the pivot
in x and y; the town NPC signs only when *s* ≥ 1.6. The player's own head marks where the
player is (there is no separate arrow): head and hair models turned by the body yaw, black
outline pass at 5/*s*, coloured pass at 4/*s*. Other players appear the same way; mission
targets as skulls.

**Lights and fog.** Placeholders, tiles, land pictures, zone markers and mission/town markers
are drawn under the lights in force when the renderer is called, i.e. the frame's set-up lights
(step 5 of `alpha-rendering.md` §1.3: front P4, back P5, ambient P5, sun direction), so the map
follows the time of day and is dark at night like the minimap; the creature markers use the
renderer's own marker lights (§5.1.4). Fog is effectively off (§5.1.4). No water pass, no
statics.

**State left behind** as §5.1.4: map object +4 = P, +0x44 = V/8 (the overlay's matrices). The
mission, town and zone markers read the +0x44 view before this call rewrites it (the store
follows them, 0x5ffdd5 …), so they use the view of the previous renderer call: they lag one
frame behind a change of pan, zoom or angle, and on the first map frame after play they use
the minimap's last view.

#### 5.2.5 The overlay (MapOverlayWidget, 0x4c95a0, draw 0x4c9680)

**Construction** (0x45c0f1 … 0x45c11b): a 0x164-byte widget built on the interface G+0x800710
with parent container G+0x8008a4 (inside the map root); the game object G is kept at
widget+0x160. Its draw runs whenever the interface is drawn with the map root visible, i.e.
in map frames only, after the map renderer has stored its matrices.

**Screen position of a point.** For a point Q (blocks, fixed point) the overlay forms
d = (Q − player position − pan) in blocks (pan's x and y; the z offset is Q.z − player z),
then, with the map object's matrices (G+0x800d44 + 0x44 = V/8, + 4 = P), row vectors:

1. e = (dₓ, dᵧ, d_z, 1)·V/8; the point is skipped unless the depth term is positive
   (0x4c9baf … 0x4c9bb5);
2. c = (e.x, e.y, e.z, 1)·P; n = (c.x/c.w, c.y/c.w); skipped unless −1 ≤ nₓ, nᵧ ≤ 1
   (0x4c9d23 … 0x4c9d49);
3. pixel x = W/2 + nₓ·W/2, pixel y = H/2 − nᵧ·H/2 (W, H = the interface size, G+0x800710
   +0x10c, +0x110).

The pixel is the text's origin (first baseline, §3.4).

**Text.** Every label is drawn twice with font `resource1.dat`, centred on the origin
(layout flag 1, §3.4): first fill white (1, 1, 1, 1) with a **black (0, 0, 0, 1) outline of
3 px**, then the label's fill colour with no outline over it. The visible result is the fill
colour with a black 3-px rim.

At the start of every draw the hovered portal zone G+0x800dd4 := (−1, −1) (0x4c96d7). Then two
passes, both over the same 64 × 64-zone window around the **centre zone** (c_x, c_y): each
axis of player position + pan (pan converted to fixed point by truncation after × 65536) is
divided by 65536 and then by 256, both divisions truncating toward zero (0x4c96f1 … 0x4c9799;
world coordinates are positive, so this equals the floor).

**Pass A: zone labels** (only when *s* = G+0x1c4 **> 2**; 0x4c9787 … 0x4ca3ef). For
zx = c_x − 32 … c_x + 31 (outer), zy = c_y − 32 … c_y + 31: take the zone map record (0x602440,
§5.1.6); skip unless its per-zone copy has kind (+0x10) ≠ 0 **and** it is discovered (+0x30
bit 0) (0x4c9829 … 0x4c9848). Point Q = zone centre (256·zx + 128, 256·zy + 128) at z = the
generator's base height there (0x5c5e20). Text = the zone **detail** (0x4e5a20, §5.9.1 "The
detail") of that entry: "Trade District", "Crafting District", "Adventurer District", "Pet
District" or "Portal of *N*" (dungeon entrances, kind 3, have no key, so their text is empty
and nothing shows). **10 px.** Fill (0x4c9f4e … 0x4ca1ce):

| Zone kind | Condition | Fill |
|---|---|---|
| 1 (town quarter) | always | white (1, 1, 1) |
| other than 1 and 4 (dungeon, 3) | entry level (+0x18) > player level (+0x190) + 2 | red (1, 0.2, 0.2) |
| other than 1 and 4 | player level − 2 ≤ entry level ≤ player level + 2 | green (0.2, 1, 0.2) |
| other than 1 and 4 | entry level < player level − 2 | white |
| 4 (portal) | not in teleport mode | white |
| 4 | teleport mode | purple (0.6, 0.25, 1) |
| 4 | teleport mode and this zone is the **selected** zone | pink (1, 0.25, 1) |
| 4 | teleport mode and the cursor is in the label's **hover box** | cyan (0.2, 1, 1), and hovered zone := (zx, zy) |

The hover box is cursor x ∈ [x − 100, x + 100), cursor y ∈ [y − 20, y + 10) around the label
origin (100 = 0x6fd0b4, 20 = 0x6fcd74, 10 = 0x745e74; 0x4ca104 … 0x4ca16d). Hover wins over
selected. Portal labels, and with them teleporting, therefore need *s* > 2.

**Pass B: location names** (every zoom; 0x4ca44d …). Same 64 × 64-zone scan. For each zone
whose record is discovered, take the location copy of its 8 × 8-zone map cell (0x6023b0, map
region record +0x34000, §5.1.6; cell = zone ÷ 8 truncated toward zero). Draw it when it exists,
its type (+0x18) is neither 0 nor 10, it has not been drawn yet in this draw (a set of record
addresses), and the zone's centre is inside it: bump(f(L, zone centre)) > 0 (0x5fa4c0,
§5.9.1) (0x4ca4ee … 0x4ca5ef). So a place's name appears as soon as the player has discovered
any zone whose centre lies within the place. Point Q = the record's centre (+0x00, +0x08) at
the generator's base height there (0x5c5e20). Text = the **location name** (0x4e5590, §5.9.1:
"*N* City", "*N* Lake", "Ruins of *N*", "*N* Castle", …). **12 px.** Fill (0x4cac22 … 0x4cadab),
with p = the level power 0x43ca60 of the record's level (int +0x24) and p₀ that of the player's
level, where the level power of level L is 1 − 1/(1 + 0.05·(L − 1)) (0.05 = 0x6fcdb4):

| Location | Condition | Fill |
|---|---|---|
| type 1 (town) | always | white |
| other | p > p₀ + 0.1 | red (1, 0.2, 0.2) |
| other | p₀ − 0.1 < p ≤ p₀ + 0.1 | cyan (0, 1, 1) |
| other | p ≤ p₀ − 0.1 | white |

(0.1 = 0x6fcb7c.) Region names ("Lands of *N*", "*N* Ocean") are **not** drawn on the map; nor
is a player arrow, a frame, a legend or a grid. Everything else on the map comes from the
renderer (§5.2.4) and the compass (§5.2.3 step 7).

#### 5.2.6 Cursor, selecting a portal and teleporting

* **Cursor label** (0x48b3a5 …, 0x48b638 … 0x48b6c1): every update the cursor label
  G+0x8008a8 is first set to the empty text; then, with the map open, teleport mode on, a
  selected zone ≠ (−1, −1) and selected = hovered, it is set to **"Teleport"** (string
  0x701830).
* **Left button release** (0x47ddd0, button 0, map open, 0x47df01 … 0x47e008):
  * if teleport mode is on, the selected zone ≠ (−1, −1) and it equals the hovered zone:
    **teleport**: under the world's lock (0x601cb0 / 0x601e90), the local player's position :=
    ((256·zx + 128)·65536, (256·zy + 128)·65536, **0**) in fixed point, i.e. the centre of the
    portal's zone at height 0 (how the player is then lifted out of the ground is not covered
    here; unresolved: U-F3-6); selected and hovered := (−1, −1); map mode := 0;
  * otherwise: selected := hovered (a click on a portal label selects it; a click elsewhere
    deselects, since hovered is then (−1, −1)).
* So a teleport is **two clicks** on the same portal label: the first selects it (pink), the
  second, while hovering it (cyan, cursor label "Teleport"), teleports. Without teleport mode
  (map opened with M) clicks only select and nothing happens.
* There is no other click action on the map: no waypoint, no marker placement, no
  right-click action.

#### 5.2.7 Worked examples

1280 × 720, θ = 120, ψ = 0, pan 0, player standing on flat ground at the pivot's height
(points below have z = 0 relative to the pivot). *x*ₛ = −1.3579951, *y*ₛ = 2.4142136,
*o*ₓ = *o*ᵧ = 0; rot = (−120°, 0, 0).

* **Zoom 1.** Pivot → (640, 360). One zone (32 map units) along +x: view (32, 0, 300), clip
  x = −43.456 → pixel **(547.29, 360)** (at ψ = 0, +x is screen left; camera right is −x).
  One zone forward (0, −32, 0): after the x-turn (0, 16, 27.713), view (0, 16, 327.713) →
  pixel **(640, 317.57)**. So a zone is about 93 px wide across and 42 px deep at the centre.
* **Zoom 2.5** (labels on): the same forward zone → view (0, 40, 369.282) → pixel
  (640, 265.86). A portal zone two zones ahead, label point (0, −512, 0) blocks →
  V/8 gives the map-unit point (0, −64, 0) → pixel **(640, 201.46)**; its hover box is
  x ∈ [540, 740), y ∈ [181.46, 211.46).
* **Tilt.** θ = 180 (rot.x = −180°, top-down): the forward zone → (640, 267.29). θ = 100 is
  clamped to rot.x = −110°: the forward zone → (640, 331.18).
* **Turn.** ψ = 90° (camera forward = +x): the zone along +x, (32, 0, 0), goes to
  (640, 317.57), straight up from the centre, and the zone at (0, −32, 0) goes to
  (732.71, 360), to the right. The camera's forward is always up on the map.
* **Pan.** Zoom 1, middle-drag 10 px to the right, ψ = 0: v = (10, 0, 0) × 4 = (40, 0, 0)
  blocks; the pivot moves 5 map units along +x; a ground point at the old pivot is now at
  (−5, 0, 0) relative → pixel (654.49, 360): the map moved 14.5 px right for 10 px of drag.
* **Zoom easing.** *s* = 1, three notches up: target 1/0.9³ = 1.37174; after one 16-ms update
  w = 1 − 0.996¹⁶ = 0.06212, *s* = 1 + 0.37174 × 0.06212 = 1.02309.
* **Placeholder.** A zone without a tile in a land of elevation 140, player at z = 100 blocks,
  zone not discovered but its record present with flags 0x02: base z = (140 − 100)·0.125 − 12.5
  = −7.5 map units, top at 24.5 (one ×4-scaled 8-voxel model); tint (0.2, 0.35, 1) on a grey
  voxel of 233 → (46.6, 81.55, 233) before lighting and occlusion.
* **Markers.** At *s* = 2.5 the mission and town markers are scaled by 8/2.5 = 3.2; at
  *s* = 0.5 by 10 (clamped); at *s* = 6 by 2 (clamped). A mission marker turns 3.2° in a
  160-ms span of frames.
* **Location colour.** Player level 10 (p₀ = 1 − 1/1.45 = 0.31034), a castle of level 14
  (p = 1 − 1/1.65 = 0.39394): p ≤ p₀ + 0.1 = 0.41034 and > p₀ − 0.1 → cyan. Level 20
  (p = 0.48718 > 0.41034) → red. Level 5 (p = 0.16667 ≤ 0.21034) → white.

#### 5.2.8 Open points

* U-F3-1: the content of models 2559 `compass2` and 2567 `maptile`, and the compass's exact
  placement and lens inside 0x476660 (§8.1.6).
* U-F3-3: the meaning of the location record's mission fields (+0x34, +0x41, +0x4c, +0x50) and
  of object type 45.
* U-F3-4: whether the zero-size map containers (0x45c090, 0x45c0d7) can ever be hit by the
  widget test 0x650ae0; a left drag over the map is believed always to turn the camera.
* U-F3-5: the identity of the world's entry at +0xb8 used for the town-marker distance.
* U-F3-6: how the physics resolves a player teleported to height 0.
* U-F2-3: the point stored at map object +0x84 … +0x8c (§5.1.4) has no reader in the overlay;
  its reader is unknown.

### 5.3 Player bars: placement, fill and text

All of this is redone **every frame** in the game update (0x488ee0), after the layout routine,
so these per-frame anchors win over anything 0x482a40 or the start-up code wrote. "A" is where
the template's pivot lands (§2.1); "O" = A − pivot is the offset added to every child's
shape-plus-translation to get window pixels. Fill fraction *f* is applied to the `bar` child as
an x-scale about its pivot (§2.1). W/2 is the integer half of the window width (truncated).

**Anchors, values and text** (0x490a93 … 0x493b0f):

| Bar (node) | Anchor A (pivot) | Fill *f* | Text written | Shown |
|---|---|---|---|---|
| `experiencebar` | (100, 75) | XP ÷ XP-for-next-level, clamped to ≤ 1 | `text` = XP + "/" + XP needed (both integers) | always |
| `lifebar` | (100, 30) | HP ÷ max HP (not clamped) | `text` = trunc(HP) + "/" + trunc(max HP); `info` = player name; `class` = "LVL " + level + " Warrior" / " Ranger" / " Mage" / " Rogue" (class 1–4) | always |
| `manacubebar` | (100, 130) | — | — | always; the first *k* children visible, *k* = mana cubes (+0x1164) mod 4, or 4 while a sequence plays in it (0x6364f0); the file has no sequence there, so effectively mod 4 (0x490d70 … 0x490d9b) |
| pet `lifebar` clone (G+0x800770) | (100, 180) | pet HP ÷ pet max HP | as `lifebar`, `info` = pet name, `class` = "LVL " + level + " " + species name | only while the player has a pet (G+0x2e4 lookup of the player's pet id) |
| pet `experiencebar` clone (G+0x8007cc) | (100, 225) | pet XP ÷ needed, ≤ 1 | as `experiencebar` | with the pet |
| `ridingbar` | (100, 290) | — | — | with the pet; children visible: *k* = min(5, trunc(riding value (+0x1198)·5 + 0.9999)) (0x491811 … 0x49183d) |
| party `lifebar` clones (list G+0x800778, 3 of them) | (310 + 220·*j*, 30), *j* = 0, 1, 2 | that player's HP ÷ max HP | as `lifebar` | one per other creature of controller kind 0 (+0x60) in the world list, not the local player, in list order, at most the 3 clones; all hidden first each frame (0x491902 … 0x491df3) |
| `mpbar` | (W/2 + 8, H − 90) | `bar`: mana (0…1); `chargebar`: charge (0…1) | `text` = trunc(mana·100 + 0.5) + "/100" | always |
| `hpbar` | (W/2 − 163, H − 90) | HP ÷ max HP | `text` = trunc(HP) + "/" + trunc(max HP) | always |
| `staminabar` | (W/2, H − 115) | stamina (+0x1194, 0…1) | `name` stays "Stamina" | only while stamina < 1 |
| `castbar` | (W/2, H − 150) | see below | `name` = caption below | see below |

Integer conversions are truncations toward zero (0x68d910 and the truncating float-to-integer
conversions, e.g. 0x491a64, 0x4929c6).

**Geometry of each bar** (from `gui.plx`; window pixels; *f* = fill):

| Bar | Filled rectangle (x range × y range) | Full size | Gradient (bottom → top vertex colours) |
|---|---|---|---|
| `lifebar` (A = (100, 30)) | x 104 … 104 + 112·*f*, y 45 … 68 | 112 × 23 | (1, 0.207, 0) → (0.869, 0, 0.324) |
| `experiencebar` (A = (100, 75)) | x 104.18 … 104.18 + 112·*f*, y 84.45 … 108.45 | 112 × 24 | (0.634, 0, 1) → (0.221, 0, 1) |
| `mpbar` (A = (W/2 + 8, H − 90)) | x (W/2 + 7) + 5·*f* … (W/2 + 7) + 155·*f*, y H − 90 … H − 63 | 150 × 27 | mana (0, 0.524, 1) → (0.089, 0, 0.648); `chargebar` same rectangle rule, (1, 0, 0.869) → (0.676, 0, 1), drawn over the mana fill |
| `hpbar` (A = (W/2 − 163, H − 90)) | x (W/2 − 164) + 5·*f* … (W/2 − 164) + 155·*f*, y H − 90 … H − 63 | 150 × 27 | (1, 0.207, 0) → (0.869, 0, 0.324) |
| `staminabar` (A = (W/2, H − 115)) | x W/2 − 80 … W/2 − 80 + 160·*f*, y H − 115 … H − 107 | 160 × 8 | (0, 0.979, 1) → (0, 0.648, 1) |
| `castbar` (A = (W/2, H − 150)) | x W/2 − 80 … W/2 − 80 + 160·*f*, y H − 150 … H − 142 | 160 × 8 | (0.683, 0.614, 1) → (0.519, 0.414, 1) |

(The `mpbar`/`hpbar` bar pivot is 5 px left of the bar's left edge, which is why their left
edge moves by 5·*f*.) Each bar also has, unscaled: a `background` of the same full rectangle
(black, alpha 0.2; `mpbar`/`hpbar` 0.2 at the bottom to 0.483 at the top; stamina/cast 0.641),
a white gloss `Shape` over the upper half (alpha 0.124; life, XP, MP, HP bars), and a `frame`:
the notched outline drawn as a stroke only (flags 6; stroke width 8 px on life/XP/MP/HP, 4 px on
stamina/cast; stroke colour unresolved: U-F3-9). The `staminabar` frame also holds three
thin cyan (0, 0.979, 1) tick lines at x = W/2 − 40, W/2 and W/2 + 40, spanning y H − 115 … about H − 107.3. Pet and party bars are
the same shapes moved with their anchor.

**Labels** (window pixels, origin = first baseline, §3.4; every label is an outlined text node
with a plain one at the same place over it, `alpha-ui-plasma.md` §3.3):

| Bar | Label | Origin | Size, alignment, colour |
|---|---|---|---|
| `lifebar` | `info` (name) | (100, 17) | 10 px, left, white, black outline 3 |
| | `class` | (100, 31) | 10 px, left, cyan (0, 0.979, 1), black outline 3 |
| | "HP" (static) | (99, 46) | 9 px, left, white, outline 2.5 |
| | `text` (numbers) | (163, 59.97) | 9 px, centred, white, black outline 2.5 |
| `experiencebar` | "XP" | (99.19, 84.45) | 9 px, left, white, outline 2.5 |
| | `text` | (161.87, 99.89) | 9 px, centred |
| `mpbar` | "MP" | (W/2 + 7, H − 89) | 10 px, left, white, outline 3 |
| | `text` | (W/2 + 88.97, H − 73.17) | 9 px, centred |
| `hpbar` | "HP" | (W/2 − 164, H − 89) | 10 px, left |
| | `text` | (W/2 − 82, H − 73.17) | 9 px, centred |
| `staminabar` | `name` "Stamina" | (W/2 − 0.34, H − 122) | 10 px, centred, cyan (0, 0.979, 1), outline 2.5 |
| `castbar` | `name` | (W/2 − 0.49, H − 157) | 10 px, centred, (0.683, 0.614, 1), outline 2.5 |

**Mana cubes and riding icons.** `manacubebar` (A = (100, 130)) has four `icon1` groups centred
at x = 115, 147, 179, 211, y = 122 (each a 24 × 26 cube icon, blues (0.441, 0.757, 1) /
(0.228, 0.664, 1) / (0.683, 0.862, 1)). `ridingbar` (A = (100, 290)) has five icons centred at
x = 115, 139, 163, 187, 211, y = 282. Children are shown in file order: the first *k* visible,
the rest hidden (0x490d70, 0x49180b).

**Cast bar** (0x492f1d … 0x493b0f), decided every frame:

* Shown when any of: block power (player float +0x174) < 1; the player's animation (byte +0x68)
  is one of 0x1e–0x22, 0x25–0x2e, 0x31, 0x57–0x59, 0x5c, 0x5e, 0x5f, 0x69 and the animation
  time (int +0x6c) ≤ its duration (0x43caa0); the stealth meter (+0x1190) > 0 (believed:
  stealth); the "blocking" test (0x444590) is true and block power ≠ 0. Otherwise hidden
  (0x492f2a … 0x492ffc).
* Content, in this priority: stealth meter > 0 and the test 0x444610 false → *f* = stealth
  meter, caption "Stealth"; else blocking or block power < 1 → *f* = max(0, block power),
  caption "Block Power" (0x493a3c …); else *f* = animation time ÷ animation duration, caption
  from the animation id: 0x1e "Fire Swirl", 0x1f "Fire Vortex", 0x20 "Water Swirl", 0x21
  "Water Vortex", 0x22 "Healing Stream", 0x25 "Fireball", 0x26–0x28 "Firebolt",
  0x29/0x2a/0x2c "Water drop", 0x2b "Water splash", 0x2d "Water Salvo", 0x2e "Fire Salvo",
  0x31 "Teleport", 0x57 "Explosion", 0x58 "Fire Explosion", 0x59 "Lava", 0x5c "Summon",
  0x5e "Firebeam", 0x5f "Fireray", 0x69 "Teleport to City"; any other id → empty caption
  (index table 0x49d430, case table 0x49d3e0).

**Worked examples.**

1. Health 37 of 130, any window size: *f* = 37/130 = 0.284615. `lifebar` fill spans x 104.000 …
   135.877 (width 31.877), y 45 … 68; `text` = "37/130". The `background` still spans
   104 … 216.
2. Same health, 1280 × 720, bottom `hpbar`: fill x 477.423 … 520.115, y 630 … 657; text "37/130"
   centred at (558, 646.83).
3. Mana 0.37, charge 0, 1280 × 720: text "37/100"; mana fill x 648.85 … 704.35, y 630 … 657;
   `chargebar` has width 0 (both edges at 647).
4. XP 50 of 120: *f* = 0.416667, fill x 104.18 … 150.85; text "50/120". XP 130 of 120: *f* = 1,
   text "130/120".
5. Health 37.9 of 130.6: text "37/130" (both truncated).
6. Stamina 0.5 at 1280 × 720: bar visible, fill x 560 … 640, y 605 … 613. Stamina 1.0: hidden.
7. Second party member (*j* = 1): `lifebar` clone anchored at (530, 30); its fill starts at
   x 534.
8. Riding value 0.41: *k* = trunc(2.05 + 0.9999) = 3 icons. Mana cubes 6: *k* = 2.

### 5.4 Creature life bars over heads

Rebuilt every frame (0x499e9b … 0x49b14a): the container G+0x8007b0 is emptied, then for each
candidate creature in a list prepared earlier in the update (0x4a8f20 iteration) one bar is
**cloned** under that container. Only the `:small` templates are used in Alpha
(`enemylifebar:small` G+0x8007a0, `friendlifebar:small` +0x8007a4, `staticlifebar:small`
+0x8007a8, `neutrallifebar:small` +0x8007ac); the large forms (G+0x800790 … +0x80079c) are
looked up at start-up (0x4614d0 … 0x4615cc) and never read afterwards.

* **Which creatures.** Never the local player. No bar either for a creature whose linked
  creature id (+0x198) names a creature currently in animation state 0x6a, nor, when the local
  player is in state 0x6a, for the creature whose +0x198 is the local player's id
  (0x499f02 … 0x499f2e, 0x49a174 … 0x49a197); +0x198 is believed to be the rider/mount link and
  0x6a the riding state, so mounts being ridden get no bar (unresolved: U-F3-8). A list entry
  carries a "focus" flag (entry byte +8), set only for this frame while the creature is under
  the crosshair (§5.4.1 item 6). Unfocused creatures are shown only if the **V** toggle
  (G+0x8007b4, key case 0x47e7f0) is on or the creature is a player (controller kind 0); with V
  off, unfocused non-player creatures get no bar (0x49a3a0 … 0x49a3af).
* **Visibility range.** The raised point (below) must pass the visibility test 0x59ee90 (from
  G+0x140) with a range of 200 blocks, reduced in darkness: with L = the light value at the
  creature (0x4718b0) ÷ 255 × a factor from the update's state (unresolved: U-F3-7), when
  L < 0.3 the range is 200·(0.9·L/0.3) + 20 (0x499f74 … 0x49a05f).
* **Template by controller kind** (creature byte +0x60, `alpha-creatures-behaviour.md` §1;
  not the creature type at +0x64): 1 → `neutrallifebar:small` if the creature is a peaceful
  animal (0x444680, §5.4.1 item 5), else `enemylifebar:small`; 6 → `staticlifebar:small`;
  anything else (players, NPCs, pets) → `friendlifebar:small` (0x49a338 … 0x49a401).
* **Position.** The anchor point is the creature's displayed position (+0x1350) raised by half
  its height (element 2 of +0x80) minus the value at +0x1180, transformed into camera space
  (matrix G+0x26c). For creatures in animation state 0x6a or 0x6b, 2 is then added to the
  camera-space depth component (0x49a149 … 0x49a170; the point appears slightly farther from
  the camera, not higher). A depth ≤ 0 (behind the camera) → no bar; the point is then
  projected to window pixels; unfocused bars outside the screen (normalised x or y outside
  −1 … 1) → no bar; the window y is raised to at least 50 (0x49a315 … 0x49a326). The bar's
  pivot is put on that point.
* **Shapes** (relative to the anchor point P, from `gui.plx`): `bar` x P.x − 40.06 …
  P.x − 40.06 + 80·*f*, y P.y − 11 … P.y − 5 (80 × 6); `bar2` (black, alpha 0.392) is scaled by
  1 − *f* about its pivot on the right edge (0x49b0c2 … 0x49b102), so it covers
  x P.x + 39.94 − 80·(1 − *f*) … P.x + 39.94; `frame` = a 4-px stroke-only outline around the
  80 × 6 bar (stroke colour unresolved: U-F3-9). Bar colours (bottom → top): enemy
  (0.931, 0, 0) → (1, 0.228, 0.228); friend (0, 0.395, 1) → (0.216, 0.525, 1); static (0.497 grey); neutral (0.179, 1, 0); all at alpha
  0.738. *f* = HP ÷ max HP.
* **Texts.** `info` (origin P + (−0.08, −21), 12 px, centred) = the species name from the
  dictionary ("singular" form; key from 0x59aa60) + " +" + the creature's power tier when
  non-zero (+0x1a8) (§5.4.1 items 1–2). `rarename` (P + (−0.08, −35), 12 px, centred; fill
  purple (0.502, 0, 1) on enemies, (0, 0.731, 1) on the others) = the creature's own name when
  it has one; otherwise a generated title for bosses (appearance flag 0x200 at +0x7e) and NPCs
  (controller kind 3) (0x5a0ed0, §5.4.1 item 3); hidden when empty. The small templates have
  no `text` node, so the "HP/max" string written to `text` has no effect.
* **Name colour** (applied to every `info` text's fill; 0x49aae7 … 0x49ad0e): players
  (controller kind 0) (0, 0.8, 1); controller kind 6 (0.8, 0.8, 0.8); others by comparing the
  level power p = 0x43ca60(level) (§5.4.1 item 4) with the local player's p₀: p > p₀ + 0.3 →
  red (1, 0.2, 0.2); p > p₀ + 0.1 → orange (1, 0.6, 0.2); p > p₀ − 0.1 → (0, 1, 1); otherwise
  white.

**Worked example.** A goblin with 45 of 60 HP, projected to (700, 300): *f* = 0.75; `bar` x
659.94 … 719.94, y 289 … 295; `bar2` x 719.94 … 739.94; species label baseline (699.92, 279).

### 5.4.1 Names, titles and colours of creature bars

Creature fields used here (object offsets; the network record offset is 0x10 less):
+0x08 the 64-bit **creature id**; +0x60 the **controller kind** (0 player, 1 hostile NPC,
3 NPC, 6 inert object; `alpha-creatures-behaviour.md` §1); +0x64 the **creature type**
(0 … 155, a 32-bit integer); +0x7e the appearance flags (0x200 boss, 0x800, 0x1000);
+0x190 the level; +0x1a8 the **power tier** (one byte); +0x1168 the creature's own name
(a zero-terminated string of 8-bit characters).

**1. Species name (`info`).** Two lookups (0x49adcf … 0x49ae3e):

1. *Type → key* (0x59aa60). The world object (G+0x2e4) holds a table from creature type
   (+0x64) to a **dictionary key**, filled once by the world constructor 0x58eb00. A type with
   no row gives the empty key.
2. *Key → text* (0x480e00 on the world's dictionary, G+0x314). The dictionary keeps the
   `<name>` elements of `dict_en.xml` (`data4.db`) by key, and for each its child elements by
   element name. The bar asks for the child **`singular`** (string literal at 0x701740). If
   the key is not there, or the entry has no `singular`, the result is the empty string.

The label is that text, followed, only when the power tier is non-zero, by " +", the tier as
an unsigned decimal number without padding, and a line break (0x49ae66 … 0x49ae9f). With tier
0 there is neither suffix nor line break. A missing name with a non-zero tier gives just
" +N" and the line break. Example: "Goblin +2".

The key table (every row the constructor writes; "en" is the English `singular` found in
`dict_en.xml`; types 36 and 62 are written twice with the same key):

| # | Key | en | # | Key | en | # | Key | en |
|---:|---|---|---:|---|---|---:|---|---|
| 0 | `ElfMale` | Elf | 52 | `Insectoid` | Insektoid | 104 | `SnoutBeetle` | Snout Beetle |
| 1 | `ElfFemale` | Elf | 53 | `Hornet` | Hornet | 105 | `LemonBeetle` | Lemon Beetle |
| 2 | `HumanMale` | Human | 54 | `InsectGuard` | Insect Guard | 106 | `Crab` | Crab |
| 3 | `HumanFemale` | Human | 55 | `Crow` | Crow | 107 | `SeaCrab` | Sea Crab |
| 4 | `GoblinMale` | Goblin | 56 | `Chicken` | Chicken | 108 | `Troll` | Troll |
| 5 | `GoblinFemale` | Goblin | 57 | `Seagull` | Seagull | 109 | `DarkTroll` | Dark Troll |
| 6 | `Bullterrier` | — *(no entry)* | 58 | `Parrot` | Parrot | 110 | `HellDemon` | Hell Demon |
| 7 | `LizardmanMale` | Lizardman | 59 | `Bat` | Bat | 111 | `Golem` | Golem |
| 8 | `LizardmanFemale` | Lizardman | 60 | `Fly` | Fly | 112 | `EmberGolem` | Ember Golem |
| 9 | `DwarfMale` | Dwarf | 61 | `Midge` | Midge | 113 | `SnowGolem` | Snow Golem |
| 10 | `DwarfFemale` | Dwarf | 62 | `Mosquito` | Mosquito | 114 | `Yeti` | Yeti |
| 11 | `OrcMale` | Orc | 63 | `PlainRunner` | Plain Runner | 115 | `Cyclops` | Cyclops |
| 12 | `OrcFemale` | Orc | 64 | `LeafRunner` | Leaf Runner | 116 | `Mammoth` | Mammoth |
| 13 | `FrogmanMale` | Frogman | 65 | `SnowRunner` | Snow Runner | 117 | `Lich` | Lich |
| 14 | `FrogmanFemale` | Frogman | 66 | `DesertRunner` | Desert Runner | 118 | `RuneGiant` | Rune Giant |
| 15 | `UndeadMale` | Undead | 67 | `Peacock` | Peacock | 119 | `Saurian` | Saurian |
| 16 | `UndeadFemale` | Undead | 68 | `Frog` | Frog | 120 | `Bush` | Bush |
| 17 | `Skeleton` | Skeleton | 69 | `PlantCreature` | Cormling | 121 | `SnowBush` | Bush |
| 18 | `OldMan` | old Man | 70 | `RadishCreature` | Radishling | 122 | `SnowBerryBush` | Snowberry Bush |
| 19 | `Collie` | Collie | 71 | `Onionling` | Onionling | 123 | `CottonPlant` | Cotton Plant |
| 20 | `ShepherdDog` | Shepherd Dog | 72 | `DesertOnionling` | Desert Onionling | 124 | `Scrub` | Scrub |
| 21 | `SkullBull` | Skull Bull | 73 | `Devourer` | Devourer | 125 | `CobwebScrub` | Scrub |
| 22 | `Alpaca` | Alpaca | 74 | `Duckbill` | Duckbill | 126 | `FireScrub` | Fire Scrub |
| 23 | `BrownAlpaca` | Alpaca | 75 | `Crocodile` | Crocodile | 127 | `Ginseng` | Ginseng |
| 24 | `Egg` | Egg | 76 | `SpikeCreature` | Spike Creature | 128 | `Cactus` | Cactus |
| 25 | `Turtle` | Turtle | 77 | `Anubis` | Ancient Guardian | 129 | *(none)* | — |
| 26 | `Terrier` | Terrier | 78 | `Horus` | Ancient Guardian | 130 | `ThornTree` | Thorn Tree |
| 27 | `ScottishTerrier` | Scottish Terrier | 79 | `Jester` | Jester | 131 | `GoldDeposit` | Gold Deposit |
| 28 | `Wolf` | Wolf | 80 | `Spectrino` | Spectrino | 132 | `IronDeposit` | Iron Deposit |
| 29 | `Panther` | Panther | 81 | `Djinn` | Djinn | 133 | `SilverDeposit` | Silver Deposit |
| 30 | `Cat` | Cat | 82 | `Minotaur` | Minotaur | 134 | `SandstoneDeposit` | Sandstone Deposit |
| 31 | `BrownCat` | Cat | 83 | `NomadMale` | Nomad | 135 | `EmeraldDeposit` | Emerald Deposit |
| 32 | `WhiteCat` | Cat | 84 | `NomadFemale` | Nomad | 136 | `SapphireDeposit` | Sapphire Deposit |
| 33 | `Pig` | Pig | 85 | `Imp` | Imp | 137 | `RubyDeposit` | Ruby Deposit |
| 34 | `Sheep` | Sheep | 86 | `Spitter` | Spitter | 138 | `DiamondDeposit` | Diamond Deposit |
| 35 | `Bunny` | Bunny | 87 | `Mole` | Mole | 139 | `IceCrystalDeposit` | Ice Crystal Deposit |
| 36 | `Porcupine` | Porcupine | 88 | `Biter` | Biter | 140 | `Scarecrow` | Scarecrow |
| 37 | `GreenSlime` | Slime | 89 | `Koala` | Koala | 141 | `Aim` | Aim |
| 38 | `PinkSlime` | Slime | 90 | `Squirrel` | Squirrel | 142 | `Dummy` | Dummy |
| 39 | `YellowSlime` | Slime | 91 | `Raccoon` | Raccoon | 143 | `Vase` | Vase |
| 40 | `BlueSlime` | Slime | 92 | `Owl` | Owl | 144 | `Bomb` | Bomb |
| 41 | `Frightener` | Frightener | 93 | `Penguin` | Penguin | 145 | `SapphireFish` | Sapphire Fish |
| 42 | `SandHorror` | Sand Horror | 94 | `Werewolf` | Werewolf | 146 | `LemonFish` | Lemon Fish |
| 43 | `Wizard` | Wizard | 95 | *(none)* | — | 147 | `Seahorse` | Seahorse |
| 44 | `Bandit` | — *(no entry)* | 96 | `Zombie` | Zombie | 148 | `Mermaid` | Mermaid |
| 45 | `Witch` | Witch | 97 | `Vampire` | Vampire | 149 | `Merman` | Merman |
| 46 | `Ogre` | Ogre | 98 | `Horse` | Horse | 150 | `Shark` | Shark |
| 47 | `Rockling` | Rockling | 99 | `Camel` | Camel | 151 | `Bumblebee` | Bumblebee |
| 48 | `Gnoll` | Gnoll | 100 | `Cow` | Cow | 152 | `LanternFish` | Lantern Fish |
| 49 | `PolarGnoll` | Polar Gnoll | 101 | `Dragon` | Dragon | 153 | `MawFish` | Maw Fish |
| 50 | `Monkey` | Monkey | 102 | `BarkBeetle` | Bark Beetle | 154 | `Piranha` | Piranha |
| 51 | `Gnobold` | Gnobold | 103 | `FireBeetle` | Fire Beetle | 155 | `Blowfish` | Blowfish |

Types 95 and 129 have no row (empty label). `Bullterrier` (6) and `Bandit` (44) have rows but
no `<name>` entry in `dict_en.xml`, so they also show an empty label. "Insektoid" (52) is the
dictionary's own spelling.

**2. The " +N" number (+0x1a8).** It is the creature's **power tier**. When the world step
creates a creature from a spawn object it copies the spawn object's byte +0x58 into +0x1a8
unchanged (0x610049 … 0x61004c). How the generator fills that byte belongs to
`alpha-worldgen-locations.md` and `alpha-creatures-behaviour.md`. The tier of player
creatures is 0 (unresolved: U-F4-1).

**3. Generated title (`rarename`).** Chosen in this order (0x49a881 … 0x49aa48):

1. The own name (+0x1168) is not empty → that name, widened to UTF-16.
2. Otherwise, appearance flag 0x200 set → the generated title.
3. Otherwise, controller kind 3 → the generated title.
4. Otherwise → the empty string.

The generated title is produced by the name maker 0x5a0ed0, called with the low 32 bits of the
creature id (+0x08) as the number *s* and the creature type (+0x64) as the selector *t*. It
uses **no random draws and no dictionary**. All arithmetic is unsigned 32-bit with
wrap-around and divisions round down:

* the first index is (⌊*s*/7⌋ + 13·*s*) reduced modulo 2³², then modulo *n*;
* the second index is (7·*s* + ⌊*s*/10⌋) reduced modulo 2³², then modulo *n*;
* the title is first-part[first index] followed directly by second-part[second index], with
  no separator and no change of letter case.

*n* is 10 for the listed selectors and 20 for the fallback. The syllable lists are built once,
on first use, as executable string literals (0x5a0efb … 0x5a34ff; tables 0x76b808 …
0x76d8e8):

| Selector *t* | *n* | First part [0 … n−1] | Second part [0 … n−1] |
|---|---:|---|---|
| 0 | 10 | El, Li, Dri, Elan, Le, Ly, A, Tho, Da, Les | ric, kun, zic, dor, las, sander, reon, reas, lundra, andor |
| 1 | 10 | Ar, A, Si, Mer, Lu, Ari, Su, Sira, Hia, Li | weya, luna, laya, leya, ma, lia, matra, vy, zyna, ly |
| 2, 18, 43, 83 | 10 | De, Ro, As, Ge, An, Pat, Wolf, Ale, Mi, Ben | rek, man, gram, rald, rick, ram, sander, kal, ton, ny |
| 3, 45, 84 | 10 | Sa, Jen, Kla, Mi, La, Auri, Mela, Alu, Ve, Est | rah, ny, ria, elle, na, ga, ma, riana, rona, una |
| 4 | 10 | Gra, Xe, Ur, Bel, Chu, Ki, Go, Zim, Kubo, Raz | zic, nax, tik, tuk, ruk, bo, rix, mi, tok, nor |
| 5 | 10 | Kur, Xe, Ki, Am, Zifa, Zu, Ma, Chi, Zi, Mia | ra, lia, bi, ba, ly, ki, bara, mi, zy, xa |
| 7 | 10 | Cho, Zer, Kaz, Kraz, Zel, Drak, Lo, Raz, Spi, Zen | rux, rek, zic, tac, mec, kor, rax, zor, ro, go |
| 8 | 10 | Zi, Zer, Az, Ki, Iza, Drak, Li, May, Spi, Zan | rya, ri, zia, ki, mah, ira, maya, ana, ra, ya |
| 9 | 10 | Tor, Gem, Bar, Me, As, Tem, Arak, Ur, Grim, Xor | lox, bart, kor, thos, kun, bur, thor, lok, li, bor |
| 10 | 10 | Mum, Grun, Brun, Hei, Mo, Hil, Ur, Lum, Grim, Xor | pie, hild, di, muna, ki, trud, sa, axa, ira, ika |
| 11 | 10 | Uz, Ur, Chu, Ku, Mor, Ura, Ak, Ur, Or, Gor | ku, ruk, muk, thak, dok, tor, rorok, chak, kaz, rack |
| 12 | 10 | Uz, Ur, Chu, Ku, Mor, Ura, Ak, Ur, Or, Gor | ka, rua, mua, thara, daka, tah, rorah, chaka, kaya, rana |
| 13 | 10 | Qua, Rib, Quib, Zib, Il, Wok, Wib, Moko, Sli, Bul | rik, bit, ble, bik, wak, wok, wib, luk, mey, bak |
| 14 | 10 | Qua, Rib, Quib, Zib, Il, Wok, Wib, Moko, Sli, Bul | ria, bia, bla, bia, waka, woka, wibba, lua, maya, ba |
| 15 | 10 | Chu, Dro, Al, Dem, Mor, Mur, Cro, Zul, Dra, The | lu, kor, card, morius, enius, tus, demar, lus, ruul, zad |
| 16 | 10 | Xu, Dra, Al, Dem, Mor, Myr, Cra, Zul, Dri, Thu | la, kira, cara, moria, ena, tana, diria, laza, rah, zazah |
| any other value (6, 17, 19 … 42, 44, 46 … 82, 85 and up, and −1) | 20 | Kro, Aru, As, Ge, Kur, Lugo, Iko, Liku, Tero, Var, An, The, Ga, Lan, Dura, Dama, Se, Thal, Nar, San | no, mi, la, sel, ron, ion, kor, lon, rok, tar, den, gar, dar, dara, lan, ka, gor, rior, ria, mor |

(The groups follow the species keys of item 1: 0/1 elves; 2/3 humans, with 18 OldMan,
43 Wizard, 83 NomadMale and 45 Witch, 84 NomadFemale; 4/5 goblins; 7/8 lizardmen; 9/10
dwarves; 11/12 orcs; 13/14 frogmen; 15/16 undead.)

Because the title depends only on the id and the type, a creature keeps the same title every
frame and on every client that knows the same id.

**4. Level power (0x43ca60).** For a level *L* (the integer at +0x190 converted to a float):

  p(*L*) = 1 − 1 / ((*L* − 1) · 0.05 + 1),

evaluated in single precision in that order: subtract 1, multiply by 0.05 (constant
0x6fcdb4), add 1, divide 1 by the result, subtract that from 1. p(1) = 0, p(10) ≈ 0.31034,
p(14) ≈ 0.39394, p(20) ≈ 0.48718; it tends to 1.

**5. Neutral bar test (0x444680).** True ("peaceful animal") exactly when the appearance flags
(+0x7e) have none of 0x0200, 0x0800, 0x1000 set **and** the creature type is one of these 32:
19 Collie, 20 ShepherdDog, 22 Alpaca, 23 BrownAlpaca, 25 Turtle, 26 Terrier,
27 ScottishTerrier, 30 Cat, 31 BrownCat, 32 WhiteCat, 33 Pig, 34 Sheep, 35 Bunny, 53 Hornet,
55 Crow, 56 Chicken, 57 Seagull, 58 Parrot, 67 Peacock, 68 Frog, 74 Duckbill, 87 Mole,
89 Koala, 92 Owl, 93 Penguin, 98 Horse, 100 Cow, 106 Crab, 107 SeaCrab, 145 SapphireFish,
146 LemonFish, 147 Seahorse. Only creatures of controller kind 1 reach this test (§5.4); a
boss Crow therefore gets the enemy bar.

**6. The focus flag.** The candidate list is rebuilt from scratch every frame
(0x4908cf … 0x4909f4): it is emptied, then every creature of the world list whose health
(+0x16c) is above 0 and whose squared distance from the local player, in blocks, is at most
**3600** (60 blocks; constant 0x6fd334; the creature's position from +0x1350, the player's
from +0x10) is added with its squared distance and focus = 0; the list is then sorted
(0x458ba0). A second pass (0x498690 … 0x498d03) sets focus = 1 on an entry when all of these
hold:

1. it is not the local player; and, while the local player's action (+0x68) is 34 and its
   action time (+0x6c) is below the value returned by 0x43d1a0, it is not of controller
   kind 1 or 6;
2. the line-of-sight test 0x59ee90 succeeds within range **256** (0x702ab8); when the light
   value at the creature (0x4718b0, divided by 255 and scaled) is below **0.3**, the range is
   instead light · 0.9 / 0.3 · 256 + **25.6** (constants 0x6fcdc0, 0x6fd31c, 0x702a98);
3. its box covers the screen centre: the eight corners of a box of the creature's size + 1 in
   each axis, centred on its position lowered by the value at +0x1180, are projected with the
   game camera (G+0x26c); among the corners in front of the camera there is at least one with
   normalised x < 0, one with x > 0, one with y < 0 and one with y > 0.

So "focus" means **under the crosshair this frame**; it lasts one frame and is re-evaluated on
the next. The same pass also stores the id of the nearest focused creature as the current
target (G+0x800a70) and looks for the nearest creature within 16 blocks (0x745e78).

**Worked examples.**

* Goblin, type 4, tier 2: key `GoblinMale` → "Goblin"; `info` = "Goblin +2" followed by a line
  break. Tier 0: "Goblin".
* Boss goblin (flag 0x200), type 4, id low bits *s* = 1000, no own name: first index
  (142 + 13000) mod 10 = 2 → "Ur"; second index (7000 + 100) mod 10 = 0 → "zic"; title
  "Urzic".
* NPC (controller kind 3) of type 44 (Bandit), *s* = 123456: first index
  (17636 + 1604928) mod 20 = 4 → "Kur"; second index (864192 + 12345) mod 20 = 17 → "rior";
  title "Kurrior"; `info` is empty (no dictionary entry).
* Colours, local player level 10 (p₀ ≈ 0.31034), with the thresholds of §5.4: a level-20
  creature (p ≈ 0.48718) is above p₀ + 0.1 but not above p₀ + 0.3 → orange; level 14
  (p ≈ 0.39394) → (0, 1, 1); level 5 (p ≈ 0.16667) → white.

### 5.5 Ability bar

Built once at start-up (0x460232 … 0x46050b) and updated every frame (0x48b8ab … 0x48c100,
0x491f16 … 0x492727).

**Buttons.** Row A (G+0x8007fc) holds 7 nodes: six clones of `abilitybutton` (index 0–5),
made in a loop, and then the `quickitembutton` template node itself appended as index 6
(0x460502 … 0x46050b). Each button's pivot is placed at x = W/2 − 162 + 48·*i*, y = H − 50
(§2).

| Index | Key label (`info`) | Highlighted while held | Content |
|---:|---|---|---|
| 0 | "M1" | left mouse button (input flag G+0x4) | ability slot 0 |
| 1 | "M2" | right mouse button (G+0x5) | slot 1 |
| 2 | "1" | key 1 (G+0x6) | slot 2 |
| 3 | "2" | key 2 (G+0x7) | slot 3 |
| 4 | "3" | key 3 (G+0x8) | slot 4 |
| 5 | "4" | — (no highlight is applied to this button) | slot 5 |
| 6 | "Q" (the template's own text) | Q (G+0x17) | quick item |

Labels: index 0 and 1 get the literals "M1" and "M2"; indices 2–5 get the number *i* − 1
written in decimal (0x460409 … 0x460472). Each clone's `cooldown` child is flattened at
start-up (vertical scale 0, 0x4604bd … 0x4604d6).

**Shapes, relative to the button's pivot (x₀, y₀)** (from `gui.plx`; unresolved: U-F4-2):

* Body: a 42 × 42 square with 3-px notched corners, white fill, black 4-px stroke; it carries
  the **icon** as its texture (shape offset 0x7ac).
* `frame`: the same outline inset by 1 px (x₀ + 1 … x₀ + 41), 2-px white stroke, no fill. Its
  display fill colour is set every frame (0x4922bb … 0x492713): **(1, 0, 0.2, 1)** while the
  button's control is held and no window or menu has input, otherwise (1, 1, 1, 1).
* `cooldown`: a black rectangle, alpha 0.552, x₀ + 1 … x₀ + 41, with its pivot on its bottom
  edge (y₀ + 41). It is scaled vertically by *c* (0x434ad0 with (1, *c*)), so it covers
  y₀ + 41 − 40·*c* … y₀ + 41.
* `info` (key label): origin (x₀, y₀ + 4), 10 px, left-aligned; white with a black 2.5-px
  outline.
* Quick item only: `count` origin (x₀ + 36, y₀ + 37), 10 px, right-aligned; a 36 × 36
  `background` box (x₀ + 3 … x₀ + 39, y₀ + 3 … y₀ + 39) in which the item model is drawn; the
  body fill is black, alpha 0.469.

**Per-frame rules.**

1. *Slot list.* The six ability ids are rebuilt every frame from the player's class,
   specialization and weapon (0x48b8ab … 0x48bd3e; the rules belong to the player-control
   specification). Slots 2–5 are emptied (id 0) when the matching skill level (player values
   from +0x1150 in steps of 4) is ≤ 0.
2. *Icon.* The id is looked up in the icon table G+0x800820; found → that texture; not found
   → the id-0 texture `none.png`. The body's texture saturation (shape offset 0x544) is set to
   **1 when the ability can be used now** (0x43e5a0: mana, cooldown and state checks) and
   **0 otherwise** (grey icon) (0x48bfcd … 0x48c071).
3. *Cooldown.* If the player's cooldown table (+0x139c) has an entry for the id and the
   ability's cooldown length (0x43e6a0) is non-zero, `cooldown` is shown, its deformation
   reset and its vertical scale set to *c* = max(0, remaining ÷ length); otherwise it is
   hidden (0x48bdea … 0x48bfa8).
4. *Quick item.* `count` = the stack size of the selected quick item (§5.7; " " when there is
   none) (0x491f16 … 0x49226f).
5. *Highlight* as in the table above.

**Ability icon table** (id → file, registered into G+0x800820 by the start-up routine
0x459c40, e.g. 0x45e1e3 … 0x45f317; the content store the files are read from is
unresolved: U-F4-3): 0 none, 1 attack-right, 2 attack-left, 3 stab-right,
4 stab-left, 5 perforate, 6 punch-right, 7 punch-left, 8 block, 9 shield-attack,
0xa shield-slam, 0xb swirl, 0xc slash, 0xd slash-left, 0xe slash-right, 0x10 mutilate,
0x11 ambush, 0x12 pierce-left, 0x13 pierce-right, 0x14 kick, 0x15 ranger-kick, 0x16 shoot,
0x17 shoot-fast, 0x18 charge-shoot, 0x19 salvo, 0x1a boomerang, 0x1b charge-boomerang,
0x1c beam, 0x1e fire-shock, 0x1f fire-burst, 0x20 water-shock, 0x21 water-burst,
0x22 healing-stream, 0x24 charge-bullet, 0x25 fireball, 0x26–0x28 firebolt,
0x29/0x2a/0x2c waterbolt, 0x2b waterball, 0x2d water-salvo, 0x2e fire-salvo,
0x30 intercept, 0x31 teleport, 0x32 retreat2, 0x36 smash, 0x37 salvo, 0x39 slam,
0x3a slam-back, 0x3b charge-slam, 0x3f swirl, 0x40 charge-mutilate, 0x41 slam-left,
0x42 slam-right, 0x43 jab, 0x4f sneak, 0x56 cyclone, 0x58 fire-explosion, 0x5f beam,
0x60 shuriken, 0x61 camouflage, 0x63 aim, 0x64 swiftness, 0x65 bulwark, 0x66 warfrenzy,
0x67 mana-shield (all `.png`). Two further tables: skills (G+0x800828) 0 pet-taming,
1 pet-riding, 2 climbing, 3 hang-gliding, 4 swimming, 5 ship-driving; specializations
(G+0x800830) mage-fire, mage-water, warrior-berserker, warrior-guardian, ranger-sniper,
ranger-scout, rogue-ninja, rogue-assassin (the ids used for these eight keys are not
recorded here, unresolved: U-F4-4).

**Worked example (1280 × 720).** Button pivots at x = 478, 526, 574, 622, 670, 718, 766,
y = 670; each spans 42 px. Label "M1" baseline at (478, 674). An ability with 1500 ms of a
4000 ms cooldown left: *c* = 0.375, overlay x 479 … 519, y 696 … 711. Holding key 2 tints the
frame of the button at x = 622.

### 5.6 Floating combat text

A list of floating texts (G+0x800738) is drawn every frame (0x49584d … 0x495e7a): the
container G+0x800750 is emptied, and for each entry two text nodes are made and placed; the
entry's age grows by the frame time and the entry is removed once its age exceeds its
lifetime.

**Entry fields** (byte offsets in the entry): +0x00 age (ms, set to 0 by every producer);
+0x04 rise (px, default −100); a world anchor; a 2-D screen offset (px); +0x2c colour; the
text; +0x50 lifetime (integer ms, default 1000); +0x54 size (px); +0x58 alignment flags
(§3.4). The defaults are written by the entry initialiser 0x4664b0.

**Drawing**, with *t* = age ÷ lifetime:

* The world anchor is projected with the game camera; behind the camera → nothing is drawn
  for this entry this frame.
* Origin = projected point + screen offset + (0, rise·*t*).
* Outline node: transparent fill, outline colour = colour × (0.1, 0.1, 0.1, 1 − *t*), outline
  radius = size × 0.125 (0x6fd8d4). Fill node on top: colour × (1, 1, 1, 1 − *t*), no outline.
  Font `resource1.dat`, the entry's size and flags.

**Producers:**

| Event | Text | Colour | Size | Rise | Offset | Life | Flags |
|---|---|---|---|---:|---|---:|---:|
| hit where the local player is attacker or target (0x494e0e …; hit types 4 and 6 skipped; also skipped when \|damage\| < 0.1 and the type is 0) | "BLOCK" (type 1), "MISS" (3), "ABSORB" (5); otherwise \|damage\|: below 1 as trunc(\|damage\|·10)·0.1 printed as a float, otherwise the truncated integer | heal (damage < 0): (0.4, 1, 0.5); damage taken by the local player: (1, 0.3, 0.3); otherwise yellow (1, 1, 0.25) when the hit flag at +0x44 is set, else white | 20; 60 when the hit flag at +0x14 is set | +100 (sinks) when the local player is the target, both for heals and for damage; otherwise −100, or −200 when the +0x14 flag is set | (0, 0) | 1000 | 1 (centred) |
| level up (0x493d84 …) | "LEVEL UP!" | (0.8, 0.6, 1) | 50 | −100 | (0, −250) | 3000 | 2 |
| XP gained (0x493ed5 …) | "XP +" + gain | (0.8, 0.6, 1) | 30 | −100 | (0, −200) | 3000 | 2 |

The world anchor for hits is the hit position raised by 2 world units plus a further term
that involves the target's size (0x494e9b … 0x494f53; unresolved: U-F4-5); for level and XP
it is the player's position. Flags 2 is the right-alignment bit of §3.4.

**Worked example.** A white hit of 23.6 on a creature: text "23", size 20, outline radius 2.5.
At age 250 ms: *t* = 0.25, alpha 0.75, the origin is 25 px above the projected point; at
1000 ms it is 100 px above with alpha 0, and it is removed once its age exceeds 1000 ms. A
0.46 hit shows "0.4".

### 5.7 Quick-item wheel (Tab)

**Tab** toggles G+0x800a40 (0x47e7ae). While it is open:

* Candidate items = the usable items from the inventory (0x47ae10); the selected index
  G+0x800a44 is taken modulo the count (negative values wrap).
* **A** or a leftward mouse movement steps the index −1, **D** or a rightward movement +1;
  each step plays sound 0x55 and arms a **200 ms** repeat delay for that direction (statics
  0x76b144, 0x76b148); **W** or **S** close the wheel (0x497952 … 0x497b06).
* **R** or **E** (edge-triggered): if the selected item's level requirement (signed 16-bit
  value at item +0x14) is greater than the player's level (+0x190), sound 0x32 plays and the
  wheel stays open; otherwise the item is used (0x4a2780) and the wheel closes
  (0x497db6 … 0x497e7b).
* The frame draws each candidate's 3D item model at scale **0.09** on a circle round the
  window centre (0x4bac91 … 0x4bb031), lit as described in the §5.1.4 note: the radius *R*
  (G+0x800a48) eases towards **200** px and the turn *a* (G+0x800a4c, degrees) eases towards
  **−360**·index ÷ count, both with rate **0.01** per millisecond (0x4aba20 with weight from
  0x4ac150: starting from 0, the weight w is replaced by w + (1 − w)·0.01 once per whole
  millisecond of the frame time, in double precision, and the result is rounded to single
  precision; mathematically 1 − 0.99^ms). Item *i* of *n* is at angle
  θ = 2π·*i*/*n* + *a*·π/180 − π/2 and position (W/2 + *R*·cos θ, H/2 + *R*·sin θ). While the
  wheel is closed *R* is reset to 0.
* The `selector` node (pivot at (W/2, H/2 − 200)) is visible exactly while the wheel is open.
  Its `name` text = item name + " +" + the item's level value, and its `count` text = stack
  size (0x497b36 … 0x497d91). From `gui.plx` (unresolved: U-F4-2): `name` origin
  (W/2, H/2 − 120), 16 px, centred, white, outline 4; `count` origin (W/2 + 57.5, H/2 − 170),
  22 px, right-aligned.

**Worked example (1280 × 720).** 4 items, index 1, settled: *a* = −90°; item 0 at
θ = −180° → (440, 360); item 1 at θ = −90° → (640, 160), i.e. the selected item sits at the top.

### 5.8 Crosshair, interaction prompt and death

* `crosshair` (pivot at (W/2, H/2)) is visible when no window or menu has input, the player is
  not aiming (player flag 0x400 at +0x124 clear) and Left Shift (input flag G+0x14) is not
  held; `zoomcrosshair` is visible while flag 0x400 is set (0x4927da … 0x492975).
* The **interaction prompt** is the crosshair's `name` text (from `gui.plx`, unresolved:
  U-F4-2: origin (W/2, H/2 + 33.25), 12 px, centred, white, black outline 2.5). Every frame it
  is first set to "" and then (0x496890 …) to "[R] Revive" while the player's HP ≤ 0, or else
  to the prompt for the creature or object under the interaction target ("[R] Talk",
  "[R] Ride", "[R] Open", … §5 list).
* **Death.** There is no death screen, overlay, fade or message: the HUD stays as it is, the
  bars show 0, and the prompt reads "[R] Revive". While HP ≤ 0 the interaction target is
  cleared (0x499545). Pressing **R** (edge-triggered; skipped while the quick-item wheel is
  open, 0x49735f) with HP ≤ 0 (0x49736c … 0x4973df): the player's transient state is reset
  (0x447110: stamina to 1, stealth, cooldowns and timers to 0), the player is moved to the
  point returned by the respawn search 0x5a03d0 (given the world and the player's position;
  its rule is unresolved: U-F4-6), and HP is set to the maximum HP (0x444db0).

**Worked example.** 1280 × 720, HP 0: "[R] Revive" centred on (640, 393.25).

### 5.9 Land-name banner timing

The banner timer *T* is a static integer in milliseconds (0x76b134), initially 0. When the land
name or detail text changes while *T* = 0 (§5.9.1), `landname`'s `name` and `detail` texts are
set and *T* := 8000. On every frame in which *T* > 0, the banner (G+0x800758) is made visible
with display fill colour (1, 1, 1, α), α = 1 − (2·(*T*/8000 − 0.5))² (constant 0x6fd904 =
8000), computed from *T* before this frame's decrease; then *T* decreases by the frame time and
is clamped to 0 (0x494464 … 0x4944f4). On a frame with *T* = 0 and no change, the banner is
hidden. So α is 0 at the start and the end and 1 at the middle, 4 s in. Example: *T* = 6000 →
α = 0.75.

The code-made `landscape`/`landscapedetail` node (G+0x800858) and the "TIME"/"TEMP" readout
(G+0x800860) are also written every frame (§5.9.1, "The code-made labels").

### 5.9.1 Land names

Where the banner's two texts come from, how the names in them are made, and what the
code-made labels show. Routines: banner update 0x4941b9 … 0x4944f4 inside the frame routine
0x488ee0; land name 0x4e5320; location name 0x4e5590; detail 0x4e5c10 / 0x4e5a20; name maker
0x5a0ed0; the two key tables are filled by the world constructor 0x58eb00 (0x592a20 …
0x593206). Worldgen terms (region cell, owner, location record, per-zone table entry,
f(L, X, Y), bump) are those of [alpha-worldgen-regions.md](alpha-worldgen-regions.md) and
[alpha-worldgen-locations.md](alpha-worldgen-locations.md).

**Random draws.** None. The name maker calls no random-number routine: a name is a fixed
function of one 32-bit number. The world seed enters only through the numbers world
generation stores: a region cell's id, and a location record's serial or id (see the worldgen
specifications). Everything else here reads the finished world each frame.

#### Inputs each frame

* The player is the local creature at G+0x8006d0; its position is the 64-bit fixed-point
  pair at +0x10 (x) and +0x18 (y), 1/65536 block. The block column (x, y) is the position
  divided by 65536, rounding towards zero, low 32 bits kept (0x4120b0, 0x42f570).
* The world is G+0x2e4; the dictionary object is G+0x314 (loaded from `dict_en.xml` in
  `data4.db`, §0). For every `<landscape key="K">` it keeps the
  text of its `<normal>` child; for every `<name key="K">` it keeps the children by element
  name (here only `<singular>` is read). A missing key or child reads as the empty string.
* **name** = the land name of (x, y) (0x4e5320, below); **detail** = the zone detail of
  (x, y) (0x4e5c10, below). Both are recomputed every frame.

#### When it counts as "changed" (0x49434d … 0x494464)

Two remembered strings, *shownName* (0x76b11c) and *shownDetail* (0x76b104), are statics that
start empty. Each frame:

1. If name ≠ shownName **or** detail ≠ shownDetail (exact UTF-16 comparison, 0x4514e0):
   * if *T* = 0: shownName ← name, shownDetail ← detail, `landname`'s `name` ← name and
     `detail` ← detail, *T* ← 8000 (the fade of §5.9 starts this frame at α = 0);
   * if *T* > 0: nothing is copied; the running banner continues.
2. Otherwise: if *T* > 0 the banner continues; if *T* = 0 the banner is hidden.

So a change while a banner is showing is not lost: the remembered strings still differ when
*T* reaches 0, and the next frame starts a new banner with the texts current at that moment.
On the first frame of play the banner appears if either text is non-empty. An empty name or
detail is shown as an empty line; it is not skipped.

#### The land name (0x4e5320)

1. **Location test.** Look up the location record of sub-cell (x / 2048, y / 2048), each
   rounded towards zero (0x487da0; regions spec §6.3). If the record exists, its type (+0x18)
   is neither 0 nor 10, and w = bump(f(L, x·65536, y·65536)) (0x5fa4c0; the block's corner
   in fixed point) satisfies **w ≥ 0.2** (single-precision constant 0x6fcdbc), the name is the
   **location name** of L (below). With bump = max(1 − f, 0)² and f = d²/r² this is "within
   about 0.744·r of the displaced centre". Type-10 sites never give a location name.
2. **Region name** otherwise. C = owner(x, y) (0x477e10, regions spec §3.4). If there is none,
   the name is "".
   * The pattern is landscape["Lands of"]; if C's elevation (+0x18) is negative, it is
     landscape["Ocean"] instead. With `dict_en.xml` these are "Lands of @" and "@ Ocean".
   * If the pattern is empty, the name is "". The first "@" is searched (0x4d9950); if there is
     none, the name is "".
   * Otherwise the name is the pattern with that one "@" replaced by **N(C.id)** (cell +0x14;
     N below).

   So land regions read "Lands of *N*" and sea regions "*N* Ocean", one name per region cell.

#### Location names (0x4e5590)

1. key = the world's **location-key table** at W+0x800124, looked up by the exact pair
   (type, variant) = (L+0x18, L+0x1c) (0x5a5240, an ordered map; no match → "").
2. pattern = name[key]["singular"]; if it is empty, the name is "".
3. If the pattern has an "@", the first one is replaced by **N(L+0x20)** (the record's serial,
   or its id for a town); without "@" the pattern is used as it is.

Location-key table (0x592a20 … 0x593064; pattern = `dict_en.xml` `<name>` `<singular>`). The
first eleven rows are written for **every variant 0 … 9** by a loop (0x592a34 … 0x592cf6);
the rest are single entries:

| (type, variant) | key | English pattern |
|---|---|---|
| (1, 0 … 9) | Village | "@ City" |
| (2, 0 … 9) | Mountain | "@ Mountains" |
| (3, 0 … 9) | Forest | "@ Forest" |
| (4, 0 … 9) | Lake | "@ Lake" |
| (6, 0 … 9) | Canyon | "@ Canyon" |
| (11, 0 … 9) | Rock | "Rock of @" |
| (12, 0 … 9) | Tree | "@ Tree" |
| (7, 0 … 9) | Valley | "@ Valley" |
| (8, 0 … 9) | Crater | "@ Crater" |
| (13, 0 … 9) | Peak | "@ Peak" |
| (15, 0 … 9) | Island | "@ Island" |
| (5, 0), (5, 1), (5, 2), (5, 3) | Ruins | "Ruins of @" |
| (5, 4) | Gravesite | "Gravesite of @" |
| (14, 0) | Castle | "@ Castle" |
| (14, 1) | Ruins | "Ruins of @" |
| (14, 2) | Catacombs | "Catacombs of @" |
| (14, 3) | Palace | "@ Palace" |
| (14, 4) | Temple | "Temple of @" |
| (14, 5) | Pyramid | "@ Pyramid" |
| (9, 0) | Cave | "@ Cave" or "Cave" (the dictionary has two `<name key="Cave">` entries with these singulars; which one is kept is unresolved: U-F4-7) |
| (10, 0) | Portal | "Portal of @" (never reached: type 10 is excluded by the location test) |

Consequences of the exact (type, variant) match:

* A **town** (type 1) is named "*N* City" for every variant 0 … 9, so towns of every style
  are named; *N* is that of the town's id (per the locations spec, its region cell's id, the
  same *N* as "Lands of *N*").
* Types 2, 3, 4, 6, 7, 8, 11, 12, 13 and 15 are named for variants 0 … 9; a variant of 10 or
  more would give "". Type 5 is named only for variants 0 … 4 and type 14 only for 0 … 5.
* Which types and variants region preparation actually produces belongs to the locations
  specification.

#### The detail (0x4e5c10, 0x4e5a20)

1. Zone (zx, zy) = (x / 256, y / 256), rounded towards zero. E = the per-zone table entry of
   that zone (0x4a6ad0; locations spec §1.1). If there is none, the detail is "".
2. key = the world's **zone-key table** at W+0x80012c, exact pair (E.kind, E.variant) (bytes
   +0, +1; 0x59fe70). Entries (0x593070 … 0x593206):

   | (kind, variant) | key | English pattern (`<name>` `<singular>`) |
   |---|---|---|
   | (1, 1) | Trade Quarter | "Trade District" |
   | (1, 2) | Crafting Quarter | "Crafting District" |
   | (1, 3) | Class Quarter | "Adventurer District" |
   | (1, 4) | Pet Quarter | "Pet District" |
   | (4, 0) | Portal | "Portal of @" |
   | (5, 0) | Palace | "@ Palace" |

   No match → the detail is "". (Kind 3, dungeon entrances, has no entry, so those zones show
   no detail.)
3. pattern = name[key]["singular"]; empty → "". Without "@" the detail is the pattern.
4. With an "@": if E.id (+4) ≠ 0 and E.kind ≠ 0, the "@" becomes **N(E.id)**; otherwise it
   becomes the **grid name** of (x, y) (0x5a6550, below).

So the detail is "Trade District", "Crafting District", "Adventurer District" or "Pet
District" in town-quarter zones, "Portal of *N*" (of the entry's id) in a kind-4 zone, and ""
where no entry matches. Which kinds region preparation writes belongs to the locations
specification.

#### The name maker N(n) (0x5a0ed0 with selector −1)

0x5a0ed0 takes (n, selector). Every land-name caller passes selector −1 (0x4e5527, 0x4e56cf,
0x4e5b54), which selects the default pair of 20-entry tables (0x76b808 and 0x76b9f0); the
other selectors are the creature-type tables of §5.4.1 item 3. With n an unsigned 32-bit
number and all arithmetic modulo 2³² (divisions round down):

* i = ((n / 7 + 13·n) mod 2³²) mod 20;
* j = ((7·n + n / 10) mod 2³²) mod 20;
* **N(n) = First[i] followed by Second[j]**, no separator, no change of letter case (First
  entries are capitalised, Second entries are lower case). The length is the sum of the two
  syllable lengths (3 to 8 characters).

| index | First (0x76b808) | Second (0x76b9f0) |
|---:|---|---|
| 0 | Kro | no |
| 1 | Aru | mi |
| 2 | As | la |
| 3 | Ge | sel |
| 4 | Kur | ron |
| 5 | Lugo | ion |
| 6 | Iko | kor |
| 7 | Liku | lon |
| 8 | Tero | rok |
| 9 | Var | tar |
| 10 | An | den |
| 11 | The | gar |
| 12 | Ga | dar |
| 13 | Lan | dara |
| 14 | Dura | lan |
| 15 | Dama | ka |
| 16 | Se | gor |
| 17 | Thal | rior |
| 18 | Nar | ria |
| 19 | San | mor |

The tables are built in the executable on first use, not read from the dictionary.

**Worked values of N** (n → i, j → name):

| n | i | j | N(n) |
|---:|---:|---:|---|
| 525240 | 14 | 4 | Duraron |
| 525241 | 7 | 11 | Likugar |
| 526264 | 12 | 14 | Galan |
| 524215 | 2 | 6 | Askor |
| 274 | 1 | 5 | Aruion |
| 692 | 14 | 13 | Duradara |
| 764 | 1 | 4 | Aruron |
| 25782 | 9 | 12 | Vardar |
| 0 | 0 | 0 | Krono |
| 4294967295 | 3 | 2 | Gela (13·n and 7·n wrap modulo 2³²) |

Worked example, n = 525240: n / 7 = 75034, 13·n = 6828120, sum 6903154 → i = 14 "Dura";
7·n = 3676680, n / 10 = 52524, sum 3729204 → j = 4 "ron"; N = "Duraron". Full texts follow
by substitution: a land cell with id 525240 reads "Lands of Duraron" (or "Duraron Ocean" at
negative elevation); a type-5 variant-0 record with serial 274 reads "Ruins of Aruion"; a
type-14 variant-0 record with serial 692 reads "Duradara Castle"; a type-12 record with serial
764 reads "Aruron Tree"; a kind-4 zone entry with id 25782 gives the detail "Portal of
Vardar".

#### The grid name (0x5a6550)

Used only by step 4 of the detail. It requires x < 2²⁴ (compared unsigned) and y < 2²⁴
(compared signed); otherwise it returns "". It takes the grid coordinates (gx, gy) of (x, y)
(0x59fc90 then 0x58d6c0; regions spec §5.3) and returns First2[(3·gy + gx + W[0x80028c]) mod
20] followed by Second2[(3·gx + gy + W[0x800290]) mod 20] (signed remainder), with First2
(0x76d9d8) = Hy, Ty, Tri, Ge, Go, Lu, Der, Ter, Sel, Ko, Am, Ka, Kel, Ko, Sa, Mo, Ru, Al, Ta,
Har and Second2 (0x76dbc0) = ra, mala, ura, el, al, mer, kun, la, mur, gu, la, go, sel, rion,
remo, antis, uro, lic, reon, tuma. W+0x80028c / +0x800290 are set by 0x5a52e0 (unresolved:
U-F4-8).

#### The code-made labels (0x494161 … 0x494885)

* **`landscape` / `landscapedetail`** (G+0x800858; a node made at start-up under the in-game
  root, text 20 px with outline 3, 0x45aead … 0x45af90): every frame its translation is set to
  (W − 260, 90) (0x494161 … 0x49419e), and its children `landscape` and `landscapedetail` are
  given the land name and the detail texts (no extra formatting). The same frame code also
  looks up children `star1` … `star4` of this node and applies a call with argument 0 to each
  (0x494722 … 0x49480d; believed to hide them, unresolved: U-F4-9).
* **"TIME"/"TEMP" readout** (G+0x800860): every frame its translation is set to (W − 15, 20)
  (0x4944fe … 0x494539), and its child `info` gets the text
  "TIME " + *h* + ":" + *mm* + "  TEMP " + *t* + " °C  HUM " + *u* + "%"
  (two spaces before "TEMP" and before "HUM"), where, with *ms* = the world's time of day in
  milliseconds (W+0x80015c) and the player's block column (x, y):
  * *h* = *ms* / 3600000 (integer, no padding); *mm* = (*ms* / 60000) mod 60, written with
    two digits padded with "0";
  * *t* = −20 − trunc(warmth(x, y) · −60) in single precision (warmth blend 0x5c4dd0, regions
    spec §4), so −20 … +40 °C;
  * *u* = trunc(moisture(x, y) · 100) in single precision (moisture blend 0x5c4800),
    0 … 100.

  Example: *ms* = 49 500 000 (13:45), warmth 0.5601367, moisture 0.4915098 →
  "TIME 13:45  TEMP 13 °C  HUM 49%".
* Whether either node is visible in normal play is unresolved (U-F4-10): both are children of
  the in-game root G+0x800884 and the frame code writes them unconditionally; the texts above
  are what they would show.

---

## 6. Options window (OptionsWidget, 0x4cf3c0, draw 0x4d0230)

A fixed 450 × 440 panel, centred in the window, with eleven rows of "label — ‹ value ›" and
three buttons along the bottom. Every value is changed only by pressing the small left/right
arrows of its row; there are no sliders, no mouse-wheel handling and no keys of its own. The
window edits a private copy of the settings; **Apply** and **OK** hand that copy to the game,
which takes every value into use without a restart and rewrites `options.cfg`.

All coordinates in §6.2–§6.5 are **window-local pixels**: x to the right and y downwards from
the panel's top-left corner, as for the start menu (§3.2–§3.3). Text positions are text origins
in the sense of §3.4 (first baseline; centred text is centred on its origin).

### 6.1 Building, opening and closing

**Construction** (start-up routine 0x459c40, at 0x4648aa … 0x4649fb):

1. The panel is a clone of the `blackwidget` template (G+0x8008c8), made under the in-game
   root (G+0x800884) and stored at G+0x800a00. It is resized to **450 × 440** with the
   widget-stretching rule (`alpha-ui-plasma.md` §5.5), as the system panel is resized to
   150 × 120 (§3.3), so it has the same `blackwidget` look. The clone is then hidden.
2. An OptionsWidget (0x4cf3c0, kept at G+0x8009fc) is made on a new empty node and attached to
   the panel clone (0x42b800, 0x631460), so that it covers the panel: its rectangle is
   (0, 0) … (450, 440) in panel space and its draw routine writes the text over the panel.
3. The constructor clones, as children of the widget's node, one `leftbutton` and one
   `rightbutton` per row (22 arrows, kept at widget +0x170 … +0x1c4, row by row, left arrow
   first) and three `button2` clones (+0x1c8 Apply, +0x1cc OK, +0x1d0 Cancel). Each `button2`
   clone is resized to **110 × 15** and its caption is set to "Apply", "OK" and "Cancel". Each
   arrow and button has one handler, attached to the interface's **press** event (event 2).
4. The widget's settings copy (12 integers at widget +0x1d4, in the order of §6.6) first takes
   the defaults (0x4ce660). At start-up the resolution list is copied into the widget
   (+0x160 … +0x168, 0x468310; the list itself is built by the main routine, §6.5), and then
   0x4d4de0 — called only this once — copies the settings in force into the widget's copy and
   picks the list index (§6.5).

**Placement.** The layout routine (0x482a40, run whenever the window size changes) puts the
panel's top-left at **x = (W − 450) × 0.5, y = (H − 440) × 0.5**, computed in floating point
from W and H converted to floats, not rounded (0x482f14 … 0x482f85).

**Parent.** On the start screen the panel node is moved under the start-screen root each update;
in the world it is under the in-game root (§3.3). On the start screen the `cubeworld` logo and
the start menu are hidden for as long as the panel is visible (§3.3).

**Opening** (sets the panel's visibility to 1; no sound of its own):

* start menu: a left-button **press** on "Options" (StartMenuWidget hovered index 1, 0x47b6db,
  in the press handler 0x47b600);
* system panel: a left-button **release** on "Options" (SystemWidget index 0, 0x47e16f, in the
  release handler 0x47ddd0); the system panel is hidden and the Esc-row flag G+0x8008f2
  cleared first (0x47e050 … 0x47e059, §7).

Opening does **not** refresh the widget's copy from the game: whatever the copy held when the
window was last closed is shown again (see Cancel, §6.4).

**Closing:** only by **OK** or **Cancel**, which set the visibility of the panel node that
carries the widget (found by 0x62b400) to 0. No key closes it: the key handler (§11.2) does not
look at the options window, so Esc, B, O … keep their normal meaning while it is open, and a
right-button press over empty space (§7) does not close it either (0x47b719 … 0x47b77e hides
only the inventory, crafting and preview windows).

**While open** the game is in cursor mode: the window-open test 0x47f1d0 (GameController
vtable slot 1) includes G+0x800a00 (0x47f38d), so the cursor is shown and the camera does not
follow the mouse.

### 6.2 Rows: labels and values

Drawn every frame by the draw routine 0x4d0230 (OptionsWidget vtable slot 1). Row *k* = 0 … 10
has its text baseline at **y = 27 + 30·k**. Every label and every value is drawn twice at the
same pen position with `resource1.dat`, **12 px**:

1. outline pass: outline radius **3**, outline colour black (0, 0, 0, 1), glyph colour white
   (1, 1, 1, 1);
2. fill pass: no outline, glyph colour white (1, 1, 1, 1).

So all text is white with a 3-pixel black outline; no text changes colour on hover.

* **Label:** origin x = **15**, left-aligned (flags 0).
* **Value:** origin x = (widget width − 250) × 0.5 + 240, which is **340** for the 450-pixel
  panel; centred (flags 1).

| *k* | Baseline y | Label | Value text | `options.cfg` key |
|---:|---:|---|---|---|
| 0 | 27 | "Mode" | "Windowed" if `fullscreen` = 0, else "Fullscreen" | `fullscreen` |
| 1 | 57 | "Resolution" | *w* " x " *h* of the list entry at the widget's index (§6.5), e.g. "1280 x 720"; nothing if the list is empty | `resolutionX`, `resolutionY` |
| 2 | 87 | "Anti-aliasing" | "Disabled" if `antiAliasing` < 1, else 2·`antiAliasing` followed by "x" ("2x", "4x") | `antiAliasing` |
| 3 | 117 | "Render Distance" | *n* "%" (e.g. "50%") | `renderDistance` |
| 4 | 147 | "Sound FX Volume" | *n* "%" | `soundVolume` |
| 5 | 177 | "Music Volume" | *n* "%" | `musicVolume` |
| 6 | 207 | "Camera Speed" | *n* "%" | `cameraSpeed` |
| 7 | 237 | "Camera Smoothness" | *n* "%" | `cameraSmoothness` |
| 8 | 267 | "Invert Y Axis" | "Yes" if `invertY` ≠ 0, else "No" | `invertY` |
| 9 | 297 | "FPS Limit" | "none" if `minTimeStep` < 1, else 1000 ÷ `minTimeStep` (integer quotient) followed by " fps" (16 → "62 fps", 9 → "111 fps", 50 → "20 fps") | `minTimeStep` |
| 10 | 327 | "Language" | the number itself (e.g. "0", or "-1") | `language` |

Numbers are written as plain decimal integers (no padding, "-" for negatives).

### 6.3 Arrows

**Template.** `leftbutton` / `rightbutton` (`gui.plx`), used at their file size, **19 × 21**
(unresolved: U-F5-1 for the exact widget size). In box-local pixels (origin at the widget
rectangle's top-left, = shape coordinates − (24.18, −95.82)):

* `leftbutton`: a white arrow pointing left, filled — three quads
  (11, 1)–(18, 0)–(18, 21)–(11, 20), (6, 4)–(11, 1)–(11, 20)–(6, 17) and
  (0, 7)–(6, 4)–(6, 17)–(0, 14) (vertices rounded to 0.1 px here; exact values in the file),
  all vertex colours white. Its child `border` is a 12-corner outline (0, 6), (5, 6), (5, 3),
  (10, 3), (10, 0), (19, 0), (19, 21), (10, 21), (10, 18), (5, 18), (5, 15), (0, 15) drawn as
  a 2-pixel black stroke without fill (unresolved: U-F5-1);
* `rightbutton`: the mirror image (tip at x = 19, outline (0, 0), (9, 0), (9, 3), (14, 3),
  (14, 6), (19, 6), (19, 15), (14, 15), (14, 18), (9, 18), (9, 21), (0, 21)) (unresolved:
  U-F5-1).

**Placement** (layout routine 0x4d46d0, OptionsWidget vtable slot 10): for row *k* the arrow
rectangles' top-left corners are

* left arrow: **(240, 12 + 30·k)**;
* right arrow: **(widget width − arrow width − 15, 12 + 30·k) = (416, 12 + 30·k)**.

So each arrow spans 21 pixels vertically, from 15 pixels above the row's baseline to 6 below
it, and the value text sits centred between the arrows (left arrow 240 … 259, right arrow
416 … 435, value centre 340).

**Hover.** Every frame, for each of the 22 arrows, the draw routine sets the arrow's display
fill colour to **(0, 1, 1, 1)** while the cursor is over it (hit test 0x6294c0, 0x4d3b8a …) and
to (1, 1, 1, 1) otherwise. Because the fill colour multiplies the colours of the arrow's
subtree, a hovered arrow turns **cyan** while its black border stays black. No pressed look is
known (unresolved: U-F5-3).

### 6.4 Mouse and keyboard

* **Activation = left-button press.** While the game is in cursor mode, the press handler
  (0x47b600 at 0x47b67f) passes a left-button press to the interface (0x6527f0), which fires
  the press event (event 2) of the widget under the cursor; every arrow and button acts at
  that moment, once. Release (0x652940, event 3) does nothing here. Holding the button does
  not repeat: the button class's repeat event (event 21, 0x665b60; first repeat 650 ms after
  the press, then after 140, 130, … ms, the interval shrinking by 10 per repeat down to 50 ms;
  start values set at 0x6659f7 … 0x665a01) has no handler in the options window.
* No arrow and no button plays a sound. (In the world, closing the window with OK or Cancel can
  end cursor mode, which plays sound 0x54 at the next update, §7.)
* No dragging, no wheel, no keyboard input, no keyboard focus.
* The Mode row's left arrow also writes "click " and a line break to the program's standard
  output (0x4d4571 … 0x4d458e; invisible to the player; the right arrow does not).

**What each arrow does** to the window's copy (nothing reaches the game until Apply/OK). Each
arrow clamps only at its own end of the range (0x4d4430 … 0x4d4dc0):

| Row | Left arrow | Right arrow | Range reachable with the arrows |
|---|---|---|---|
| Mode | `fullscreen` := remainder of (`fullscreen` + 1) ÷ 2 | same (toggle) | 0, 1 |
| Resolution | index − 1, not below 0 | index + 1, not above count − 1 | the list (§6.5); then `resolutionX`, `resolutionY` := the entry at (index mod count), when the list is not empty |
| Anti-aliasing | − 1, not below 0 | + 1, not above 2 | 0, 1, 2 ("Disabled", "2x", "4x") |
| Render Distance | − 5, not below 25 | + 5, not above 100 | 25 … 100 in 5s |
| Sound FX Volume | − 10, not below 0 | + 10, not above 100 | 0 … 100 in 10s |
| Music Volume | − 10, not below 0 | + 10, not above 100 | 0 … 100 in 10s |
| Camera Speed | − 5, not below 10 | + 5, not above 100 | 10 … 100 in 5s |
| Camera Smoothness | − 5, not below 0 | + 5, not above 100 | 0 … 100 in 5s |
| Invert Y Axis | `invertY` := 1 if it was 0, else 0 | same (toggle) | 0, 1 |
| FPS Limit | `minTimeStep` **+ 1**, not above 50 (lower limit) | `minTimeStep` **− 1**, not below 0 (higher limit) | 0 ("none") … 50 ("20 fps") |
| Language | − 1 (no limit) | + 1 (no limit) | any integer |

With an empty resolution list the right arrow leaves the index at −1 and changes nothing else.
Values loaded from `options.cfg` are not range-checked; a value outside the range stays as it
is until an arrow moves it (for example `renderDistance 7` shows "7%", and one right press
gives 12).

**Buttons** (`button2` clones, 110 × 15, §6.5):

* **Apply** (0x4d4470): hands the copy to the game (0x46f390, below). The window stays open.
* **OK** (0x4d4650): the same, then closes the window.
* **Cancel** (0x4d4510): closes the window only. It does **not** undo the edits in the window's
  copy: they are shown again the next time the window opens and are applied by the next Apply
  or OK.

**Applying** (0x46f390, on the game object G, under the lock at G+0x2e4):

1. the 12 values are copied into the game's block at G+0x170;
2. `options.cfg` is rewritten from that block (0x4cef80, §6.6);
3. the music volume is set at once: music player (G+0x800714) volume = `musicVolume` × 0.01.

The other values take effect from the game's block without any further step:

| Value | Taken into use |
|---|---|
| `fullscreen`, `resolutionX/Y`, `antiAliasing` | 0x4c85f0, run every main-loop pass after the game update and before drawing, compares these four with the start-up copy (0x76b1d8); if any differs it copies the whole block there and resets the Direct3D device (0x4c8940). Fullscreen: back buffer = `resolutionX` × `resolutionY`. Windowed: back buffer = the window's client size (the resolution is ignored and the window is not resized). Multisampling type = 2·`antiAliasing` (0 = off), quality 0. A failed reset is retried every 100 ms until it succeeds. The same reset also runs on the window messages WM_SIZE and WM_SETFOCUS (0x4c8428, message table 0x4c85a4). |
| `renderDistance` | read live by the world worker (0x469396): a radius of `renderDistance` × 0.01 × 0x1800000 world units (its use is `alpha-rendering.md` question 12) |
| `soundVolume` | read each time a sound is played (0x484350, read at 0x485e30) |
| `cameraSpeed`, `invertY` | read live by the mouse look 0x47ea00 (0x47ed16 …; `alpha-physics-camera.md` §7.2) |
| `cameraSmoothness` | read live by the camera (`alpha-physics-camera.md` §7.2; unresolved: U-F5-5) |
| `minTimeStep` | read live by the main loop 0x4c8ae0: after each pass it sleeps until the pass has lasted `minTimeStep` ms (timer resolution set to 1 ms); 0 = no limit |
| `language` | stored and saved only: no reader of it was found, and the only dictionary file named in the executable is `dict_en.xml` (pushed at 0x4e1b3e) (unresolved: U-F5-11) |

So **nothing needs a restart**.

### 6.5 Buttons and the resolution list

**Button look** (`button2` resized to 110 × 15; unresolved: U-F5-2 for every item of this
list):

* body: a rectangle with transparent fill (all vertex colours alpha 0) and a **3-pixel black
  stroke**, alignment 1 (outside);
* `Shape`: the same rectangle, no fill, **2-pixel white stroke**, alignment −1 (inside);
* `text`: "Apply" / "OK" / "Cancel", `resource1.dat` **14 px**, white, drawn as an outlined copy
  (3-pixel black outline) and a plain copy, text flags 5 (centred horizontally and moved
  vertically as §3.4 describes). In the template its origin is 0.17 px left of and 3.5 px
  above the button's centre, and it is believed to stay at the button centre + (−0.17, −3.5)
  after resizing;
* `clip` / `Shape`: a white (alpha 0.32) 160 × 30 highlight with a cyan stroke, entirely outside
  its own mask in the template, so it never shows.

No hover or press feedback on these buttons is known (unresolved: U-F5-3).

**Placement** (0x4d46d0; *w* = 110 and *h* = 15 the button size, 450 × 440 the panel):

| Button | Top-left x | Top-left y | Rectangle |
|---|---|---|---|
| Apply | 20 | 440 − 15 − 20 = 405 | (20, 405) … (130, 420) |
| OK | (450 − 40 − 110) × 0.5 + 20 = 170 | 405 | (170, 405) … (280, 420) |
| Cancel | 450 − 20 − 110 = 320 | 405 | (320, 405) … (430, 420) |

**Resolution list** (built once at start-up by the main routine 0x4c8ae0, before
`options.cfg` is read): every display mode Direct3D 9 reports for adapter 0 in format
X8R8G8B8 (mode count, then each mode by index), in the order reported, keeping only the first
mode of each distinct width × height (refresh rates are ignored). The list is never refreshed.
When 0x4d4de0 fills the window's copy, the index is set to the entry equal to the current
`resolutionX` × `resolutionY`, or to **0** if there is none, and the copy's resolution is then
replaced by that entry (when the list is not empty). So an unlisted resolution (for example the
desktop size in windowed mode) becomes the list's first entry the next time Apply is pressed.

### 6.6 `options.cfg` and the defaults

**Settings block** (12 × i32; start-up copy 0x76b1d8, game copy G+0x170, window copy
OptionsWidget +0x1d4), in this order, with the **defaults** (0x4ce660) used when `options.cfg`
is missing or lacks a key. The start-up copy's resolution is set to the desktop size
(GetSystemMetrics, 0x4c8d9e) just before the file is read:

| # | Key | Default | Window shows |
|---:|---|---|---|
| 0 | `fullscreen` | 0 | "Windowed" |
| 1 | `resolutionX` | desktop width (SM_CXSCREEN) | |
| 2 | `resolutionY` | desktop height (SM_CYSCREEN) | |
| 3 | `antiAliasing` | 0 | "Disabled" |
| 4 | `renderDistance` | 50 | "50%" |
| 5 | `soundVolume` | 100 | "100%" |
| 6 | `musicVolume` | 100 | "100%" |
| 7 | `cameraSpeed` | 50 | "50%" |
| 8 | `cameraSmoothness` | 80 | "80%" |
| 9 | `invertY` | 0 | "No" |
| 10 | `language` | 0 | "0" |
| 11 | `minTimeStep` | 9 | "111 fps" |

**Reading** (0x4ce6e0, called once from the main routine at 0x4c8daf, before the game object
exists): the file `options.cfg` in the working directory is read as whitespace-separated
tokens. Each token is compared, case-sensitively, with the twelve key names in the order
`fullscreen`, `resolutionX`, `resolutionY`, `antiAliasing`, `renderDistance`, `soundVolume`,
`musicVolume`, `cameraSpeed`, `cameraSmoothness`, `language`, `invertY`, `minTimeStep`; on a
match the next token is read as a decimal integer into that field. Unknown tokens are skipped
one at a time. Reading stops at the end of the file or at the first token that fails to parse.
A missing file leaves the defaults.

**Writing** (0x4cef80, only from Apply/OK): `options.cfg` in the working directory is opened
for writing (truncated) as a text stream and gets twelve lines, each the key name, one space,
the value, and a line break (CR LF, text mode), in this order — `language` before `invertY`,
unlike the block. For example, with a 1920 × 1080 resolution and otherwise default values:

```
fullscreen 0
resolutionX 1920
resolutionY 1080
antiAliasing 0
renderDistance 50
soundVolume 100
musicVolume 100
cameraSpeed 50
cameraSmoothness 80
language 0
invertY 0
minTimeStep 9
```

### 6.7 Worked example (1280 × 720)

Panel top-left = ((1280 − 450)/2, (720 − 440)/2) = **(415, 140)**; it covers (415, 140) …
(865, 580). In screen pixels:

| Row | Label origin | Value origin (centred) | Left arrow | Right arrow |
|---|---|---|---|---|
| 0 Mode | (430, 167) | (755, 167) | (655, 152) … (674, 173) | (831, 152) … (850, 173) |
| 1 Resolution | (430, 197) | (755, 197) | (655, 182) … (674, 203) | (831, 182) … (850, 203) |
| 2 Anti-aliasing | (430, 227) | (755, 227) | (655, 212) … (674, 233) | (831, 212) … (850, 233) |
| 3 Render Distance | (430, 257) | (755, 257) | (655, 242) … (674, 263) | (831, 242) … (850, 263) |
| … | +30 per row | | | |
| 9 FPS Limit | (430, 437) | (755, 437) | (655, 422) … (674, 443) | (831, 422) … (850, 443) |
| 10 Language | (430, 467) | (755, 467) | (655, 452) … (674, 473) | (831, 452) … (850, 473) |

Buttons: Apply (435, 545) … (545, 560), OK (585, 545) … (695, 560), Cancel (735, 545) …
(845, 560); with the template offset of §6.5 the "OK" caption is centred on (639.83, 549)
(unresolved: U-F5-2).

Starting from the defaults (windowed, 1280 × 720 in the list):

* A left press at (660, 160) hits row 0's left arrow: the value becomes "Fullscreen" (copy
  only); the arrow is drawn cyan while the cursor stays on it.
* A press at (840, 160) toggles it back; a press at (700, 160) hits nothing (the gap between
  the arrows).
* Three presses at (660, 430) (FPS Limit, left): `minTimeStep` 9 → 12, shown "83 fps"; nine
  presses at (840, 430) bring it to 3 ("333 fps"); three more to 0 ("none"); further presses
  change nothing.
* A press at (600, 550) (OK): the game's block takes the copy, `options.cfg` is rewritten,
  the window closes; in the same main-loop pass, after the update, the device is reset to
  fullscreen if Mode was left on "Fullscreen".
* A press at (780, 550) (Cancel) with "Fullscreen" showing closes the window without applying;
  reopening it still shows "Fullscreen".

At 1920 × 1080 the panel's top-left is (735, 320) and every position above shifts by
(+320, +180).

---

## 7. Esc menu row and system panel

**Toggling.** **Esc** (0x47e770) clears map mode (G+0x8006e4 := 0) and flips the row flag
G+0x8008f2 (initially off, 0x45a302). The row closes when:

* a right-button press hits no interface widget (0x47b719 … 0x47b77e): it clears G+0x8008f2
  and the two neighbouring flags G+0x8008f0 and G+0x8008f1 (the latter is the skill-window
  flag), and hides the inventory (G+0x8008bc), the crafting window (G+0x8008c0) and the window
  G+0x8008dc;
* an item of the system panel is chosen (mouse release, 0x47e053 … 0x47e059): the panel is
  hidden and G+0x8008f2 cleared.

Activating a menu button does **not** close the row.

**Buttons.** Six `menubutton` clones (row B, list G+0x800838), made at start-up under the
in-game root (0x4605f0 … 0x46098e, in this order). Each clone's body texture (shape offset
0x7ac) is its icon from `data3.db`, and its `count` text is the key label:

| # | Icon | Label | Action (left-button press, 0x47b785 … 0x47b840 → table 0x47ddac) |
|---:|---|---|---|
| 0 | `help.png` | "F1" | toggle the help picture's visibility (G+0x80089c, as F1) |
| 1 | `skills.png` | "x" (lower case in the executable) | toggle the skill window (0x488c70, as X) |
| 2 | `crafting.png` | "C" | toggle the crafting window (0x488bd0, as C) |
| 3 | `inventory.png` | "B" | toggle the inventory (0x488c00, as B / I) |
| 4 | `worldmap.png` | "M" | enter map mode (G+0x8006e4 := 1; does not toggle) |
| 5 | `system.png` | "O" | toggle the system panel (0x488d00, as O) |

While the row flag is set, a left-button press is tested against each button in list order with
the under-cursor test 0x6294c0; every button hit acts (they do not overlap).

**Sounds.** Neither the button handler nor the window toggles (0x488bd0, 0x488c00, 0x488c70,
0x488d00) play a sound. Sounds 0x53 and 0x54 come from a check made every update
(0x48b2cb … 0x48b346): when the window-open test 0x47f1d0 (or the state byte G+0x800a40,
unresolved: U-F5-6) becomes true, sound 0x53 is played once; when it becomes false again,
sound 0x54 is played once. A toggle therefore sounds only when it changes whether any window
is open.

**Geometry** (row B anchor, §2; `menubutton` pivot (0, 0)): button *i* has its pivot at
(W/2 − 150 + 60·*i*, H − 140). Body: a 48 × 48 square from pivot + (−25, −25) to pivot +
(23, 23), filled white with the icon texture. Its hit rectangle is believed to be the same
48 × 48 square, and the label is believed to be the `count` text with origin pivot +
(−1.5, 26), 10 px, centred, white over a cyan-filled (0, 0.763, 0.814), black-outlined (2.5 px)
copy (unresolved: U-F5-4). No background panel.

**Per frame** (0x48eb40 … 0x48ec8d): every menu button's visibility := G+0x8008f2; while shown,
the button under the mouse (0x6294c0) gets the value **1.5** in its body shape's
texture-brightness field (shape offset 0x494; `alpha-ui-plasma.md`), all others 1.0. The
hovered icon is brightened, not enlarged.

**Worked example (1280 × 720).** Pivots at x = 490, 550, 610, 670, 730, 790, y = 580; the
`system.png` button covers (765, 555) … (813, 603) (with the label offset of U-F5-4 its "O" is
centred on (788.5, 606)). A left press at (770, 560) toggles the system panel; at (520, 560) it
hits nothing (the gap between button 0, x 465 … 513, and button 1, x 525 … 573).

**System panel** (SystemWidget 0x587660, drawn by 0x587710): opened by **O** or the `system.png`
button, on the `blackwidget` panel of §3.3. Three text items, centred on x = widget width / 2,
`resource1.dat` 14 px, each drawn as an outline pass (white glyphs, black 3-pixel outline) and
a fill pass whose colour is white, or cyan (0.2, 1, 1, 1) while the item is hovered:

| Item | Baseline y | Hover band (mouse y) | Action (release handler 0x47ddd0, 0x47e060 …) |
|---|---:|---|---|
| "Options" | 30 | [10, 40) | options window (§6.1) |
| "Start Menu" | 60 | [40, 70) | leave the world, back to the start screen |
| "Exit Game" | 90 | [70, 100) | quit (G+0x1a0 := 1) |

An item is hovered only while the mouse x is within [0, widget width); the hovered index
(widget +0x160) is reset to −1 at the start of every draw.

---

## 8. Windows

| Window (class, constructor) | Opened by | Template / size | Notes |
|---|---|---|---|
| Inventory (InventoryWidget type 0, 0x4c1bb0) | **B** or **I** (0x488c00) | grid of `itembox` | tab icons `inventory-equipment.png`, `inventory-items.png`, `inventory-ingredients.png`, `inventory-pets.png` (the executable holds no other `inventory-*.png` name); opening it closes the system panel. InventoryWidget also exists as types 2 and 3; the panels created for them are 350 × 285 and 350 × 568 |
| Crafting window (G+0x8008c0; the menu row's `crafting.png` button does the same) | **C** (0x488bd0) | panel 350 × 285 (InventoryWidget type 2 is built on it) | see the crafting row below for its contents |
| Equipment / character (CharacterWidget 0x434d90) | shown together with the inventory (§8.1.2) | `equippedpreview`; slots "Left", "Right", "Left Ring", "Right Ring", "Neck", "Shoulder", "Chest", "Hands", "Feet", "Light", "Special" | stats "Power", "HP", "ARMOR", "RESI", "CRIT", "TEMPO", "REG", "Weapon Rating", "Armor Rating" |
| Skills (SkillWidget 0x4dd750) | **X** (0x488c70: flips the skill flag G+0x8008f1; when opening, copies the player's 11 skill values (creature +0x1138 …) and specialization (+0x141) into the widget and hides the crafting window and the blueprint preview) | `skillbutton`, `childskillbutton`, `specializationbutton` | specialization texts from the dictionary keys `specialization:<class>:<spec>`, skill texts `description:…`, `details:…`, `skill:level`, `skill:nextlevel`; icons e.g. `warrior-berserker.png` |
| Crafting (an InventoryWidget + BlueprintPreviewWidget 0x42f190) | crafting stations ("[R] Use Anvil" …) / menu button | `craftbutton`, `craftbar`, `itemshadow`, `wideitembox` | categories "Weapons", "Armor", "Amulets", "Cooking", "Alchemy", "Formulas" with `craft-*.png` icons; "Requires Anvil" etc.; sounds 0x59/0x5a (unresolved: U-F5-10) |
| Vendor | talking to a vendor (unresolved: U-F5-10) | | "Vendor", `buy.png`, `buy-back.png` |
| Enchanting (EnchantWidget 0x44e910), Adaption (AdaptionWidget 0x40ecd0) | customization bench (unresolved: U-F5-10) | `itemframe`, `rightarrow` | " (adapted)", "Adaption" |
| System panel (SystemWidget, G+0x800910) | **O** (0x488d00) or the `system.png` menu button | `blackwidget`, 150 × 120, centred | opening it hides the inventory and opening the inventory hides it; §3.3, §7 |
| Help | **F1** | `help.plx` | keyboard picture |
| World map | **M** | — | §5.2 |
| Menu-button row | **Esc** | six `menubutton`s | §7 |

Item tooltips: rarity stars `star1`–`star4`, stats as above, "Requires …", "Ridable",
"Already known", "Warrior "/"Ranger "/"Mage "/"Rogue " class requirement, "Unknown".

The inventory, equipment panel and tooltip are specified in §8.1.

## 8.1 Inventory, equipment and items in hand

*Believed* marks a reading that is not fully proven. Coordinates are window pixels with (0, 0)
at the top-left of the client area; W, H = client width and height (G+0x11c, G+0x120).
"Outlined text" means the code-drawn pair of §3.4: first an outline pass (the given fill
colour, black stroke of the given radius), then a fill pass (stroke 0) in the stated colour.
All code-drawn text uses `resource1.dat`. Sizes are pixel sizes. Item fields are those of
`alpha-creatures-behaviour.md` §10.1; *F*(*v*, *R*) and *G* are its §10.2 level factors;
g(*L*) = 1 − 1/((*L* − 1)·0.05 + 1).

### 8.1.1 Data: where the items live (creature record and saves)

The local player's creature (G+0x8006d0) holds:

| Offset | Content |
|---|---|
| +0x140, +0x141, +0x190 | class (1 warrior, 2 ranger, 3 mage, 4 rogue), specialization, level *L* |
| +0x300 + 0x118·*k* | equipment slot *k* (0–12), one 280-byte item each (unresolved: U-F5-7) |
| +0x11dc | **bags**: a growable list of *T* bags (begin/end/capacity at +0x11dc/+0x11e0/+0x11e4); each bag is a growable list of **slots** |
| +0x11e8, +0x11ec | the **item in hand** (the stack on the mouse cursor): i32 count + 280-byte item, one slot |
| +0x1304 | money in copper (1 silver = 100, 1 gold = 10 000) |
| +0x1308 | platinum coins |
| +0x130c | outgoing item-record queue (300-byte records, sent as packet 6, §8.1.9) (unresolved: U-F5-7) |

A **slot** is 284 (0x11c) bytes: i32 count, then the item. A slot is empty when its count is
0. Whenever a count drops below 1 the code is believed to set it to 0 and clear the item's type
and subtype bytes, and a slot whose count is negative is believed to be drawn black (used by
the crafting list, never by the inventory) (unresolved: U-F5-7).

**Bag index ("tab") of an item** (0x47fa30): types 3–9, 23, 24 → 0 (Equipment); type 11 → 2
(Ingredients); types 19, 20 → 3 (Pets); every other type → 1 (Items).

**A new character** (0x4772b0) gets 4 bags; each is grown to 50 empty slots (0x477391 …
0x4773f7). Bags are never shrunk; a bag grows by one slot when an item must be added and it
has no empty slot (below).

**Saves** (`alpha-saves.md` §3.2): fields 14/15 are the bags exactly as above (u32 *T*, then per
bag u32 *n* and *n* slots of 284 bytes); field 16 (+0x1304) is the money and field 17
(+0x1308) the platinum coins. The item in hand is not saved. (Unresolved: U-F5-7.)

**Adding an item to the inventory** (0x46ebe0; bag "auto" = the tab rule above):

1. Empty item (type byte 0) → nothing.
2. If the bag index is at or beyond the number of bags, the bag list is grown to include it.
3. Quantity *q*: for the **quantity types** — 12, 13, 21, 0, 25, 20, 24, 23, and 11 unless its
   subtype (+0x01) is 14 — *q* = the item's i16 at +0x10 and the stored copy gets +0x10 := 1;
   for all other items *q* = 1.
4. Type 12 (coins), by the item's **material byte (+0x0d)**: 10 → money += *q*; 11 → money +=
   10 000·*q*; 12 → money += 100·*q*; nothing is stored. (A type-12 item with any other
   material is stored like an ordinary item.) Type 13 → platinum += *q*; nothing stored.
5. Walk the bag's slots in order, remembering the first empty one; if the new item is
   **stackable** (types 1, 10, 11, 12, 13, 21; 0x47f9f0) and a slot holds an item equal to it
   (all compared fields of `alpha-net-packets.md` §1.1, 0x42f4a0), add *q* to that slot's
   count and stop.
6. Else put the item with count *q* into the first empty slot; if there is none, append a new
   slot to the bag.

**"Can carry" test** (0x43e4a0), used before picking up and buying: only for types 1, 11 and 20.
*N* = the sum of the counts of all slots (and of the item in hand) holding an item equal to the
new one. Refused when the type is 20 and *N* > 0 (one stack of pet food at most), or when
*N* > 49. Everything else is always accepted (bags grow).

**"Can use" test** (0x43e420; called from the mouse-press handler at 0x47bd6b and 0x47cfd6,
other users unresolved: U-F5-8): item power *p*(item) = trunc(g(item level)·100 + 1) (0x4c76a0,
via 0x43ca60, U-F5-9); player power *p*(player) = trunc(g(*L*)·100 + 1) (0x445f10). Usable when
*p*(player) ≥ *p*(item) **and** the class test passes (0x4c6f20):

* type 2 (formula): the test of the item type it teaches (byte +0x08), unless that is 2;
* type 3 by subtype: 0, 1, 2, 13, 15, 16, 17 → warrior; 3, 4, 5 → rogue; 6, 7, 8, 14 → ranger;
  10, 11, 12 → mage; 9 and ≥ 18 → anyone;
* types 4–7 by material: 1, 5, 18, 22 → warrior; 19, 26 → ranger; 23, 25 → mage; 27 → rogue;
  any other material → anyone;
* every other type → anyone.

Worked values: *p* = 1 at *L* = 1, 17 at *L* = 5, 32 at *L* = 10.

**Two-handed test** (0x444820): type 3 with subtype 5, 6, 7, 8, 10, 11, 15, 16, 17 or 18 (the W2
set of `alpha-creatures-behaviour.md` §10.2).

**Price** (0x4c76e0) in copper: *k* = 2 for types 3, 4; 1 for 5, 6, 9; 1.5 for 7, 8; 100 for 23;
0.2 otherwise; ×2 again for a two-handed weapon. Price = max(1, trunc(*F*(*L*, *R*)·10·*k*))
with *L*, *R* the item's level (+0x10) and rarity (+0x0c). Example: a one-handed sword, level 5,
rarity 0: with *F* = 1.4142135 the price is **28** (U-F5-9).

### 8.1.2 Windows: what exists and where it sits

All windows below are built once at start-up (0x459c40 at 0x463fd0 … 0x465560) as clones of
the `blackwidget` template (G+0x8008c8; look as in §3.3), resized with the 9-slice rule and
made under the in-game root (G+0x800884), each with its widget attached to the clone so that
the widget rectangle is (0, 0) … (w, h) in panel space.

| Host node | Size | Widget | Placement (layout routine 0x482a40) |
|---|---|---|---|
| G+0x8008bc | **400 × 285** | InventoryWidget **type 0** (G+0x800954), the player's bags | top-left **(⌊W/2⌋ − 200, ⌊H/2⌋ − 285)**: centred, bottom edge on the screen's centre line (0x483856 … 0x4838e4). An earlier line of the same routine puts it at (W − 20 − 400, H − 285 − 220) (0x48353b … 0x483599); the later placement overrides it. |
| (parent of G+0x800960) | **260 × 200** | CharacterWidget (G+0x800960, 0x434d90; draw 0x434e30), §8.1.4 | fixed top-left **(20, 340)** (0x483830 … 0x483851) |
| 12 nodes, list G+0x800a34 | 50 × 50 each | SpriteWidgets (list G+0x800a28), §8.1.5 | below the inventory, §8.1.5 |
| G+0x8008c4 | 350 × 568 | InventoryWidget **type 3** (G+0x80095c), the vendor; bags G+0x800c0c | (⌊W/2⌋ + 200 + 10, ⌊H/2⌋ − 285): right of the inventory, tops aligned |
| G+0x8008c0 | 350 × 285 | InventoryWidget **type 2** (G+0x800958), crafting; bags G+0x800adc | (⌊W/2⌋ − 200 − 350 − 10, ⌊H/2⌋ − 285): left of the inventory |
| G+0x800ad4 | 350 × 330 | BlueprintPreviewWidget (0x42f190) | left of the inventory, top at ⌊H/2⌋ − 285 + 10 + 285 (below the crafting window) |
| G+0x8008f8 / G+0x800900 | 350 × 285 / 350 × 400 | Enchant / Adaption | right of the inventory, tops aligned |

The rule for the side windows (0x4838e9 … 0x483dda): x = ⌊W/2⌋ − 200 − *w* − 10 (left side) or
⌊W/2⌋ + 200 + 10 (right side), y = ⌊H/2⌋ − 285 (the inventory's top; the inventory's height is
used, not the window's own). Here 200 and 285 are half the inventory's width and its height,
read from the inventory panel; ⌊W/2⌋ and ⌊H/2⌋ are integer halves (truncated) converted to
float; the rest is computed in floating point.

**Opening.** B or I (0x488c00) toggles the inventory host's visibility, recomputes its scroll
range (0x4c6350, 0x4c64c0) and, if it is now open, hides the system panel (G+0x800910). The
character panel and the 12 equipment boxes have **no key of their own: every frame their
visibility is set equal to the inventory's** (0x48c6c3 … 0x48c7fc; the flag is kept at
G+0x8008f0). So "the equipment/character window" is simply the part of the inventory screen
below and left of the bag grid. **Closing:** B/I again, or a right-button press over no widget
(0x47b719 … 0x47b77e: hides the inventory, the crafting host G+0x8008c0 and G+0x8008dc, and
clears the Esc-row flag). The toggle itself plays no sound (sounds 0x53/0x54 follow the
window-open state, §7).

**Worked example (1280 × 720).** Inventory panel (440, 75) … (840, 360); character panel
(20, 340) … (280, 540); equipment boxes in the band y = 370 … 700 (§8.1.5).

### 8.1.3 The inventory window (InventoryWidget, constructor 0x4c1bb0, draw 0x4c2050)

**Fields.** Type (+0x194: 0 inventory, 2 crafting, 3 vendor); the bag list it shows (+0x160);
current tab *t* (+0x1b4); hovered tab (+0x1b0); per-tab scroll rows (+0x198); number of scroll
positions *n* (+0x1a4); slot size *s*w × *s*h (+0x1a8, +0x1ac: the `itembox` template's widget
size, 40 × 40 after truncation; 40 × 40 is also the default); hovered slot (tab +0x184, index
+0x188; reset to −1 at the start of every draw); selected slot (+0x18c, +0x190). Bag slots are
284-byte records (0x11c).

**Grid geometry** (layout 0x4c5d50, run on resize; the draw 0x4c2050 uses the same numbers), for
a panel *w* × *h*:

* columns *C* = trunc((*w* − 10) / (*s*w + 5)), rows *V* = trunc((*h* − 40) / (*s*h + 5));
  400 × 285 gives **8 columns × 5 rows**, pitch **45 px**, gap 5 px;
* slot (*c*, *r*) has its top-left at (**10 + 45*c*, 40 + 45*r***) in panel space and is
  40 × 40;
* the layout deletes the widget's previous slot clones and makes one clone of `itembox` per
  visible cell as a child of the widget. `itembox` is a 9-slice rectangle filled white at
  alpha 0.048 (box −25 … 15 around its origin, widget 40 × 40) (unresolved: U-F6-1);
* the selector (`itemselector`, present on the crafting window) is shown and moved to the
  selected slot (+0x190, relative to the current tab's scroll row) while the current tab has a
  scroll entry, else hidden.

**Scrolling.** The bag index of visible cell (*c*, *r*) is (row₀ + *r*)·*C* + *c*, where row₀ is
the current tab's scroll row. Scroll positions (0x4c6350): when the current tab has no bag, *n* = 1;
for an empty bag *n* = 0 and the scroll row is set to 0; for a bag of *m* > 0 slots
*n* = max(1, ⌊(*m* − 1)/*C*⌋ − *V* + 2) (the division is unsigned), and the scroll row is
clamped to *n* − 1. Example: 50 slots, 8 × 5 → *n* = 3 (rows 0 … 2; the last view shows slots
16 … 49).

* `upbutton` clone at (*w* − 30, 10), `downbutton` at (*w* − 30, *h* − 30); pressing them
  scrolls −1 / +1 within 0 … *n* − 1 (0x4c60f0, 0x4c5a00).
* `scrollbutton` clone (the thumb, 16 px wide): track from y = 35 to *h* − 35 (length
  *h* − 70); thumb height *t*h = trunc((*h* − 70)/*n*); thumb at x = *w* − 28,
  y = 35 + trunc(((*h* − 70 − *t*h)·row)/(*n* − 1)) when *n* > 1; with *n* = 1 the thumb is at
  y = 35 with the full track height (0x4c64c0). Dragging it with the left button sets
  row = trunc(((*y*thumb − 35)·(*n* − 1))/(*h* − 70 − *t*h)) with the thumb top clamped to
  35 … 35 + (*h* − 70 − *t*h) (0x4c5bb0). 400 × 285, *n* = 3: thumb 16 × 71 at
  (372, 35 + 72·row).
* The mouse wheel does not scroll the inventory (the wheel handler 0x47ef40 only zooms the
  camera or the map).
* The up, down and thumb buttons are tinted (0, 1, 1, 1) while hovered, else white (end of
  0x4c2050).

**Tabs.** Type 0 gets four clones of the `tab` button (`tab` template: 38 × 38 widget, a
32 × 32 textured body, 38 × 38 `border`, 36 × 36 `frame`), whose body textures are, in order,
`inventory-equipment.png`, `inventory-items.png`, `inventory-ingredients.png`,
`inventory-pets.png` (tab 0 … 3 = bag 0 … 3). Placement (0x4c6140): the first tab at x = 4, each
next x = trunc(previous x + tab width + 2) (so 4 + 40·*k*), y = −(tab height) − 3, about −41, i.e.
a row above the panel's top edge, 3 px clear of it. Every draw (end of 0x4c2050): a tab is
coloured (1, 1, 1, 1) when it is the current tab or hovered, else (0.8, 0.8, 0.8, 1); its
`frame` is (0, 1, 1, 1) while hovered, (0, 1, 0, 1) on the current tab, else white. Pressing a
hovered tab other than the current one (0x4c5a60): current tab := it, scroll range recomputed,
the selection moves to the first visible slot, sound 0x56 (`menu-tab.wav`). The vendor (type 3)
uses the same mechanism with its own tabs (`buy.png`, `buy-back.png`).

**Title.** Outlined text at (**15, 25**), 12 px, white, stroke 3, left-aligned (0x4c2050): type 0 —
tab 0 "Equipment", tab 1 "Items", tab 2 "Ingredients", tab 3 no title; type 1 — "Equipment";
type 2 — "Weapons", "Armor", "Amulets", "Cooking", "Alchemy" for tabs 0–4, any other tab
"Formulas"; type 3 — "Vendor".

**Money row** (type 0 only), 10 px, stroke 2, baseline y = *h* − 10, right-aligned, always drawn
even when 0, with money *M* (bag-list record +0x128) and platinum (+0x12c):

| Text | x (right edge) | Fill |
|---|---|---|
| (*M* mod 100) + " C " | *w* − 30 | (0.8, 0.5, 0, 1) copper |
| (⌊*M*/100⌋ mod 100) + " S " | *w* − 70 | (0.7, 0.7, 0.7, 1) |
| ⌊*M*/10 000⌋ + " G " | *w* − 110 | (1, 0.9, 0, 1) |
| platinum + "  Platinum Coins " (left-aligned) | starts at x = 15 | (0.5, 0.2, 1, 1) |

**Items in slots** (for each visible bag index that exists; a slot is drawn when its count is
non-zero, and on types 2 and 3 always):

* world shader, the 10 point-light registers uploaded as zero, depth test on (render state 7 = 1)
  for the grid and off after it; material colour (1, 1, 1, 1), or (0, 0, 0, 1) for a negative
  count;
* the model is drawn by the item-model routine of §8.1.6 at (slot x + ⌊*s*h/2⌋, slot y +
  ⌊*s*h/2⌋), i.e. **+20, +20**, with the shared turn vector G+0x800a1c, depth 0 and scale
  **0.03**, or **0.045** while the mouse is in the slot (*x* ∈ [slot x, slot x + *s*w),
  *y* ∈ [slot y, slot y + *s*h)); rings (type 9) use 0.75 × that scale;
* the hovered slot is recorded in +0x184/+0x188 whether or not the slot holds an item.

**Stack counts.** For the stackable types (1, 10, 11, 12, 13, 21) with count > 0 (count 1
included): outlined count, 10 px, stroke 2, white, **right-aligned at (slot x + 36, slot y +
37)**.

**Wide rows (types 2 and 3 only).** For non-empty slots the item name (§8.1.6 name rule) +
" +" + power (not for types 12, 13, 21, 11 other than subtype 14, 0, 25, 20, 24, 23) is drawn
word-wrapped (flag 16, wrap width *s*w − *s*h − 20) at (slot x + *s*h + 10, slot y + 10),
10 px, stroke 2, in the rarity colour. Vendor rows add the price at the slot's right edge in
coins (C/S/G, colours as the money row, each part 40 px further left, zero parts skipped)
(unresolved: U-F6-2).

**Item in hand.** At the end of every draw of an inventory-type window, if the hand count
(creature +0x11e8) is non-zero, the hand item (+0x11ec) is drawn at the mouse position with scale
**0.05** (depth 0, same turn vector) (0x4c595a). Separately, every game update
(0x48b3a5 … 0x48b627) first sets the cursor label of `cursor.plx` (G+0x8008a8, §5.2.6) to "";
then, when the hand count is non-zero, the label shows the hand count (field width 3) for a
stackable item and "" otherwise; and when window G+0x8008dc is open and the item held by widget
G+0x8008f4 (+0x180) exists, the label shows that item's name, followed by " +" and its power for
items that show a power (unresolved: U-F6-10).

**Worked example (1280 × 720, type 0).** Panel at (440, 75). Tabs at x = 444, 484, 524, 564,
y ≈ 34 … 72. Title baseline (455, 100). Slot (*c*, *r*) top-left (450 + 45*c*, 115 + 45*r*):
slot (7, 4) = (765, 295) … (805, 335), item centre (785, 315), count right edge (801, 332).
Money baseline y = 350: copper right edge 810, silver 770, gold 730; platinum from x = 455.
Up/down buttons at (810, 85) / (810, 330); thumb x = 812, y = 110 + 72·row.

### 8.1.4 The character panel (CharacterWidget, draw 0x434e30)

Drawn on its 260 × 200 `blackwidget` panel placed at (20, 340) (layout 0x482a40). Positions are
panel-relative. Labels at x = 10, values at **x = 160** (label x + 150); label and value text
10 px with a stroke-3 outline pass, labels filled white, values filled **(0.3, 0.6, 1, 1)**
unless stated.

| Baseline y | Label | Value |
|---:|---|---|
| 20 | player name (12 px, white, wrapped at 180, flag 16) | — |
| 36 | "LVL " + *L* + " Warrior"/" Ranger"/" Mage"/" Rogue" + specialization " \| Berserker"/" \| Guardian", " \| Sniper"/" \| Scout", " \| Fire"/" \| Water", " \| Assassin"/" \| Ninja" (spec 0/1), 12 px, fill (1, 0.5, 0, 1) | — |
| 56 | "Power" | *p*(player) = trunc(g(*L*)·100 + 1), integer |
| 76 | "HP" | maximum health (`alpha-creatures-behaviour.md` §4.1, 0x444db0) |
| 92 | "ARMOR" | armour (§4.2, 0x43cff0) |
| 108 | "RESI" | resistance (§4.2, 0x4467a0) |
| 124 | "CRIT" | 100 × critical chance (0x43e9e0, §4.5) + "%" |
| 140 | "TEMPO" | 100 × attack-speed factor *S* (0x447700, §4.3) + "%" |
| 156 | "REG" | round₀.₁(10·*r* / 2^((*L* − 1)·0.25) + 100) + "%", then at x = 210 "(" + round₀.₁(*r*) + ")"; *r* = regeneration sum (below) |
| 172 | "Weapon Rating" | rating *W* (below) as trunc(100·*W* + 0.5) + "%", coloured by band |
| 188 | "Armor Rating" | rating *A* as trunc(100·*A* + 0.5) + "%", coloured by band |

Values other than Power and the two ratings are rounded to 0.1 by 0x439110:
round₀.₁(*x*) = trunc(10*x* + 0.5)/10 for *x* ≥ 0 (negative values are mirrored)
(unresolved: U-F6-2).

* **Regeneration of an item** (0x4c78c0), types 3–7 only: *F*(*L* + 0.1·*n*, *R*)·*k*·*q* with
  *k* = 0.2 for chest (4), else 0.1, *q* = *u*₁/20 (+0.5 for material 26, +1 for material 27),
  *u*₁ as `alpha-creatures-behaviour.md` §10.2. *r* (0x446150) = the sum over both hands
  (type 3), chest, boots, gloves, shoulders and the amulet slot as far as their types match
  (unresolved: U-F6-2).
* **Weapon Rating** *W*: the sum, over the hand slots 6 (+0x990) and 7 (+0xaa8) that hold a
  weapon (type 3), of *F*(item level, item rarity) / *F*(*L*, 0) — the player's rarity argument
  is 0 (0x4382d0 … 0x438359); the sum is halved unless the main weapon is two-handed (0x444230).
* **Armor Rating** *A*: (*f*shoulders + 2·*f*chest + *f*gloves + *f*boots)/5 with the same ratio
  *f* for each slot holding its proper type (absent → 0).
* **Bands** (both ratings; 0x434e30): < 0.8 → (0.7, 0.7, 0.7); 0.8 … < 1.1 → white;
  1.1 … < 1.2 → (0, 1, 0); 1.2 … < 1.5 → (0.25, 0.25, 1); 1.5 … < 1.8 → (0.5, 0, 1);
  ≥ 1.8 → (1, 1, 0) (the rarity colours of §8.1.6).

Worked example: a level 1 warrior, *g* = 0 → Power 1; two level-1 rarity-0 one-handers →
*W* = (1 + 1)·0.5 = 1.0 → "100%" in white.

### 8.1.5 Equipment boxes (12 SpriteWidgets)

Built at start-up (0x460994 … 0x460bf0): for box *i* = 0 … 11 a clone of **`equipmentboxright`
when *i* is even, `equipmentboxleft` when *i* is odd** (the two templates are then removed),
each with a SpriteWidget (0x51c310) bound to its `background` child; the label is set to:

| *i* | Label | Slot (creature offset) | Accepts type | | *i* | Label | Slot | Accepts |
|---:|---|---|---|---|---:|---|---|---|
| 0 | "Left\nWeapon" | 6 (+0x990) | 3 | | 1 | "Right\nWeapon" | 7 (+0xaa8) | 3 |
| 2 | "Left Ring" | 8 (+0xbc0) | 9 | | 3 | "Right Ring" | 9 (+0xcd8) | 9 |
| 4 | "Neck" | 1 (+0x418) | 8 | | 5 | "Shoulder" | 5 (+0x878) | 7 |
| 6 | "Chest" | 2 (+0x530) | 4 | | 7 | "Hands" | 4 (+0x760) | 5 |
| 8 | "Feet" | 3 (+0x648) | 6 | | 9 | "Light" | 10 (+0xdf0) | 24 |
| 10 | "Special" | 11 (+0xf08) | 23 | | 11 | "Pet" | 12 (+0x1020) | 19, 20 |

Slot 0 (+0x300) has no box.

**Placement** (layout 0x482a40, at 0x483740 …): box *i* is placed so that its template pivot lands
at (**W/2 − 200 + 400·(*i* mod 2), H/2 + 10 + 56·⌊*i*/2⌋**). The right template's pivot is its
box's top-left corner and the left template's is its top-right corner, so:

* even *i*: box x ∈ [W/2 − 200, W/2 − 150] (flush with the inventory's left edge);
* odd *i*: box x ∈ [W/2 + 150, W/2 + 200] (flush with its right edge);
* y ∈ [Y, Y + 50], Y = H/2 + 10 + 56·⌊*i*/2⌋: six rows under the inventory, 6 px apart.

**Box contents** (relative to the box's top-left (*x*₀, *y*₀), both templates; unresolved:
U-F6-1): body 50 × 50, black alpha 0.469; `frame` (*x*₀ + 1 … *x*₀ + 49) white 2-px outline;
`background` widget 44 × 44 at (*x*₀ + 3, *y*₀ + 3) (alpha 0) carrying the model; `count` text
right-aligned at (*x*₀ + 44, *y*₀ + 45), 10 px, set to "" every frame. `info` label, 10 px, white
with a black 2.5 outline: even boxes at (*x*₀ + 60, *y*₀ + 24) left-aligned (flags 4), odd boxes
at (box right edge − 60, *y*₀ + 24) right-aligned (flags 6), so the labels sit inside the
inventory's width beside their boxes.

**Every frame** (0x48c7fc …) widget *i*'s item pointer := the slot of the table, and its model :=
the item's model (0x4ec400); an empty slot draws nothing.

Worked example (1280 × 720): "Left Weapon" box (440, 370) … (490, 420), label at (500, 394);
"Right Weapon" box (790, 370) … (840, 420), label right edge at 780; "Pet" box (790, 650) …
(840, 700).

### 8.1.6 Drawing an item model in the interface

**SpriteWidget** (constructor 0x51c310, draw 0x51c3d0; fields: raw model +0x160, item pointer
+0x164, game +0x168). Used by the 12 equipment boxes and the quick-item button (G+0x800a18, bound
to `quickitembutton`'s 36 × 36 `background`). The draw does nothing unless an item pointer or a
raw model is set. Before drawing: depth test on, the 10 point-light registers uploaded as zero,
alpha 1, shininess 0, material (1, 1, 1, 1); depth test off after. The point is the **centre of
the widget rectangle on screen**.

* With an item pointer whose type byte is non-zero: the item-model routine with the turn vector
  G+0x800a1c, **scale = widget width × 0.0006** and depth 0 (equipment box 0.0264, quick item
  0.0216). A type-0 item draws nothing here.
* With no item pointer but a raw model: the raw-model routine 0x476660 (the one the compass and
  quick-item wheel use) with scale = widget width × 0.0006 / (largest model dimension).

**Item-model routine** (0x4758c0; inputs: screen point (*x*, *y*), turn angles (*a*, *b*, *c*) in
degrees, scale σ, item, depth δ):

1. Model = the item's model (`alpha-creatures-models.md` §6.3, 0x4ec400). An item of type 0 uses
   model 2396 instead, provided the model table holds at least 2397 entries; otherwise nothing is
   drawn. No model → nothing.
2. Depth test on, depth function LESS (render state 0x17 = 2).
3. Model matrix (row vectors, *v*′ = *v*·*M*), starting from the identity with translation
   (0, 0, 1). It is turned by *a* about X, then by *b* about Y, then by *c* about Z, each turn
   acting on the axis rows produced so far; for a turn by θ about X the Y and Z rows become
   Y cos θ + Z sin θ and Z cos θ − Y sin θ, and likewise for Y (Z, X rows) and Z (X, Y rows).
   **Rings (type 9)** get two more turns: +90° about X, then −90° about Z. The three axis rows
   are then multiplied by σ (skipped when σ = 1) and by 1/max(*s*x, *s*y, *s*z) (the model's
   voxel dimensions, model +0x44/+0x48/+0x4c). Finally the translation is moved by −½ of each
   dimension along the corresponding axis row, so the model is centred. The largest dimension
   thus becomes σ units, centred 1 unit in front of the eye.
4. Projection: perspective with vertical field of view π/4, aspect argument **−W/H** (the
   horizontal scale is negated, the same sign convention as the world camera's projection),
   near 0.1, far 1000.
5. Screen placement (0x476455 … 0x476591): a shift matrix (identity with translation
   (2(*x* − W/2)/W, −2(*y* − H/2)/H, δ)) is applied **after** the projection (the combined
   matrix is projection followed by shift, 0x424f30). In normalised device coordinates the model's
   centre therefore lands exactly on (*x*, *y*), and δ is a pure depth offset.
6. Shader constants: model, combined view-projection, alpha 1, shininess from 0x4c7be0
   (1 for materials 1, 11, 12, 22, else 0), the state reset 0x4e6df0; drawn with colour
   (1, 1, 1, 1).

**On-screen size.** A unit at distance 1 spans H/2 ÷ tan(π/8) = 1.2071·H pixels, so the model's
largest dimension spans about **1.2071·H·σ** pixels, independent of δ; it grows with the window
height. At H = 720: slot 26.1 px (hovered 39.1, ring 19.6), hand 43.5, equipment box 22.9,
quick item 18.8, wheel (σ 0.09) 78.2, tooltip (σ 0.1) 86.9 px.

**Turn vector G+0x800a1c**, set every frame just before the interface is drawn (0x4bbabc …
0x4bbb16): *a* = 225°, *b* = 0°, *c* += frame milliseconds × 0.03 (30° per second, unbounded).
Every interface item model therefore shows the same slow spin about Z under a 225° tilt.

**Lighting.** The directional lights in force are believed to be those set after the minimap
pass (§5.1.4: front (1, 1, 1), back (0.2, 0.3, 0.4), ambient (0.4, 0.4, 0.4), direction
(0.5, 0.4, −0.6)); point lights are off (unresolved: U-F6-12).

**Item name colour** (0x4c7d20): type 21 → (0.5, 0.5, 0.5); type 13 → (0.5, 0.1, 1); type 25 →
(0.2, 0.5, 1); otherwise by rarity (item +0x0c): 0 white, 1 (0, 1, 0), 2 (0.25, 0.25, 1),
3 (0.5, 0, 1), 4 (1, 1, 0), any other (1, 0, 0); alpha 1. The name text itself comes from the
item-name routine 0x598a50 (items-generator specification).

### 8.1.7 Tooltip (PreviewWidget, constructor 0x4d4f10, draw 0x4d50a0; text 0x4a28c0)

**Nodes.** The `preview` template node (G+0x8008e4; a 300 × 300 box, black alpha 0.621 with a
0.669 rim, pivot = its top-left; children `star1` … `star4`, 50 × 50 / 52 × 50 at (239, 239)
from the top-left) carries the PreviewWidget (G+0x8008e8); `equippedpreview` (G+0x8008ec,
250 × 300, pivot = top-left) is the comparison panel (unresolved: U-F6-1).

**Which item is shown** (0x48d76d … 0x48d99e), first match wins:

1. the quick-item wheel is open (§5.7): the selected wheel item;
2. the hovered inventory slot (needs count ≠ 0) — *window source*;
3. the hovered non-empty equipment box — window source;
4. the crafting, vendor and two further window hovers (0x47b340, 0x47b550, 0x47b3e0, 0x47b010) —
   window source;
5. otherwise the world item under the interaction target (G+0x800a78, §8.1.9) — *world source*.

The `preview` node is visible exactly when an item was found.

**Placement** (0x48ec93 … 0x48eeb5): window source — preview top-left = **mouse + (20, 20)**,
equippedpreview top-left = mouse + (−235, 20) (a 5-px gap to the left of the tooltip); world
source — preview top-left = **(W − 320, H − 320)**, equippedpreview (W − 320, H − 575).

**Compare list** (before drawing): every equipped item other than the shown one whose type equals
the shown item's type, checked in this order: right weapon, left weapon, pet, shoulders, chest,
gloves, boots, amulet, right ring, left ring (unresolved: U-F6-3).

**Draw** (all relative to the preview's top-left (*X*, *Y*); 0x4d50a0):

1. Stars: rarity 1 → `star1` visible, 2 → `star2`, 3 → `star3`, 4 → `star4`; 0 → none.
2. Model: item-model routine at (*X* + 150, *Y* + 150), scale **0.1**, depth −0.05.
3. Text block at (*X* + 14, *Y* + 25), scale 1, wrap 280, name on, class line on.
4. Price (only while the vendor window is open, 0x47f940): coins of the price (§8.1.1),
   right-aligned from (*X* + 290, *Y* + 280), 10 px stroke 2, colours of the money row, each
   present part 40 px further left (copper, silver, gold).
5. If the compare list is non-empty: "Currently equipped" at (*X* − 190, *Y* + 20), 8 px,
   stroke 3, white, left-aligned; then each compared item's text block (wrap 230) at
   (*X* − 240, *Y* + 40 + 130·*k*).

**Text block** (0x4a28c0; origin (*x*, *y*) = baseline of the first line; *s* = scale; every line
is outlined text with the given stroke; lines 1–3 wrap at the wrap width (flag 16))
(unresolved: U-F6-3):

| Step | Shown when | Text | Size, stroke | Fill |
|---|---|---|---|---|
| name | name flag | "Pet food: " (type 20 only) + name + (" +" + *p*(item), except for the quantity types and type 19) | 10*s*, 3 | name colour |
| | | then *y* += 30*s* | | |
| line 2 | always | quantity types: *q* + " X" when *q* > 1, else empty; type 19: "LVL " + level; others: "Power " + *p*(item); + " (adapted)" if item byte +0x0e bit 0 | 9*s*, 3 | white; (1, 0, 0) when *p*(player) < *p*(item) |
| class | class flag and the player's class fails 0x4c6f20 | *y* += 14*s*; the classes that pass, in order "Warrior ", "Ranger ", "Mage ", "Rogue " | 9*s*, 3 | (1, 0, 0) |
| known | class flag, type 2, recipe already known (0x444a90) | *y* += 27*s*; "Already known" | 9*s*, 3 | (1, 0, 0) |
| | | *y* += 16*s* | | |
| pet | type 19 | "X" + seed (u32 +0x04) + "/" + trunc(g(level)·1000 + 50); *y* += 14*s*; then "Ridable" if the pet subtype is one of 19, 20, 22, 23, 25–40, 63–67, 74, 75, 98–100, 102–105, 151 (0x444760); *y* += 14*s* | 10*s*, 2 | (0.5, 1, 1) / (0.75, 0.5, 1) |
| DMG | type 3 | "DMG " + round₀.₁(damage, `alpha-creatures-behaviour.md` §10.3); *y* += 14*s* | 10*s*, 2 | (0.75, 0.5, 1) |
| HP | item health ≥ 0.1 | "HP " + round₀.₁(health); *y* += 14*s* | 10*s*, 2 | (0.75, 0.5, 1) |
| ARMOR | ≥ 0.1 | "ARMOR " + round₀.₁(armour); *y* += 16*s* | 10*s*, 2 | (0.75, 0.5, 1) |
| RESI | ≥ 0.1 | "RESI " + round₀.₁(resistance); *y* += 16*s* | 10*s*, 2 | (0.75, 0.5, 1) |
| TEMPO | speed ≥ 0.001 | "TEMPO " + round₀.₁(100·speed); *y* += 16*s* | 10*s*, 2 | (0, 1, 1) |
| CRIT | crit ≥ 0.001 | "CRIT " + round₀.₁(100·crit); *y* += 16*s* | 10*s*, 2 | (0, 1, 1) |
| REG | item regeneration ≥ 0.1 | "REG " + round₀.₁(regeneration, §8.1.4); *y* += 16*s* | 10*s*, 2 | (0, 1, 1) |
| heal | type 1, heal ≥ 0.1 | "HP +" + trunc(heal); *y* += 14*s* | 10*s*, 2 | (0.25, 1, 0.25) |

Heal (0x4c6e10), type 1 only: subtypes 1, 2, 4, 5, 6 → 200·*F*(*L*, *R*); 8, 9 → 100·*F*(*L*, *R*);
others 0. Stat formulas are in `alpha-creatures-behaviour.md` §10.3.

Worked example: an item shown from a slot with the mouse at (600, 200): tooltip box (620, 220) …
(920, 520), model centred at (770, 370); name baseline (634, 245); line 2 baseline (634, 275).

### 8.1.8 Mouse actions

Buttons arrive as press events 0 (left), 1 (right), 2 (middle) (0x47b600) and release events
(0x47ddd0); releases do not move items. There is no drag: an item is picked up onto the cursor
with one press and put down with another.

**Left press — equipment boxes** (only while the inventory is open, 0x47c4a5 …) (unresolved:
U-F6-4). Let *E* = the hovered box's slot, *H* = the hand. If the hand is non-empty and fails
"can use" (§8.1.1), play sound 0x32 and stop. Otherwise, if the hand is empty or holds a type the
box accepts, **swap** *E* and the hand's item, with these extras:

* Right Weapon: if the hand holds a two-handed weapon and Left Weapon is occupied, Left Weapon is
  first moved into the inventory and cleared.
* Left Weapon: a two-handed weapon other than subtypes 6 and 7 (bow, crossbow) is refused
  (sound 0x32). If the hand holds a two-hander, or Right Weapon holds one, Right Weapon is moved
  into the inventory and cleared first.
* Pet: a hand stack of pet food (type 20) with count ≥ 2 does not swap: if the box does not
  already hold an equal item, its old item goes to the inventory, the box gets one, and the hand
  count drops by 1.

After a swap the hand count becomes 1 if the hand now holds an item, else 0. Sound 0x57
(`menu-grab-item.wav`) if the hand is now non-empty, 0x58 (`menu-drop-item.wav`) if empty.

**Left press — bag slot** (inventory open, a slot hovered) (unresolved: U-F6-4): if the hand is
non-empty and the hand item's tab (0x47fa30) differs from the current tab → sound 0x32, nothing
else. If the hand holds a stackable type and the slot holds an equal item → the slot's count +=
the hand count and the hand is emptied (no sound). Otherwise **swap** slot and hand (count and
item, 0x44a690), sound 0x57 / 0x58 as above. The swap is skipped while controller flag +0x29 is
set (unresolved: I-4).

**Left press over no widget with an item in hand** (0x49662f …: left-button flag G+0x4 newly
down, hand count ≠ 0, inventory G+0x8008bc open, cursor over no widget, 0x650ae0 = 0): **drops one
item into the world**: a 300-byte item record holding the hand's item is queued at creature
+0x130c (packet 6, §8.1.9) and the hand count drops by one (0x42f140). Each click drops one more.

**Right (1) or middle (2) press — bag slot**, by open window, in this priority:

* Adaption window (G+0x800900) open → it takes the slot as its source (its +0x164/+0x168 reset
  to −1);
* Enchant window (G+0x8008f8) open → likewise;
* **vendor window (G+0x8008c4) open → sell one**: count −1, money (+0x1304) += price, sound 0x3a
  (`drop-coin.wav`, volume 0.75) at the player, a copy goes to the buy-back list (at most 10
  entries, the oldest dropped), vendor lists rebuilt (0x4a2300);
* window G+0x8008dc open → a weapon (type 3) is handed to it, else sound 0x31;
* **no window — use or equip** (needs "can use", 0x43e420, else sound 0x32; nothing for count 0)
  (unresolved: U-F6-4 for rows other than the first three):

| Type | Action (one item is taken from the stack) |
|---|---|
| 3, subtype 13 or 14 (shields) | if Right Weapon is a two-hander it goes to the inventory and is cleared; old Left Weapon to the inventory; Left Weapon := item |
| 3, right button, subtype 6 or 7 | old Left Weapon to inventory, Left := item; old Right to inventory, Right cleared |
| 3, right button, other | old Right to inventory, Right := item; if the item is two-handed, Left to inventory and cleared |
| 3, middle button | one-hander → old Left to inventory, Left := item; two-hander → as the right-button "other" row |
| 19, or 20 when Pet does not hold an equal item | old Pet item to inventory, Pet := item |
| 4 / 7 / 8 / 5 / 6 / 24 | Chest / Shoulder / Neck / Hands / Feet / Light, old item to inventory |
| 9 | right button → Right Ring, middle → Left Ring |
| 23 (glider, boat) | Special; the player's action byte (+0x68) := 0 |
| 11 with count > 1 | splits: one item, with its +0x10 field incremented, is re-added through §8.1.1 (unresolved: U-F6-4) |
| 1 | **use** (§8.1.10) |
| 2 (formula) | if the taught recipe is already known → sound 0x31; else it is learnt (added to the known-recipe list, save field 18), one item removed, sound 0x2f (`upgrade2.wav`) |

  Each equip or unequip refreshes the grid and plays 0x58.

**Right press — equipment box** (non-empty), in this priority: Adaption window (G+0x800900) open
→ the item becomes its source; vendor window (G+0x8008c4) open → sell it (money += price, sound
0x3a at volume 0.75, buy-back list, box cleared); window G+0x8008dc open → a weapon (type 3) is
handed to it (other types: unresolved, U-F6-4); otherwise **unequip**: the item goes into the
inventory (§8.1.1, never refused), the box is cleared, the grid refreshed, sound 0x58.

**Right press — vendor item**: on the buy-back tab → buy back; otherwise if price ≤ money and
"can carry" (0x43e4a0): add it, money −= price, sound 0x39 (`drop.wav`) at the player; if it cannot
be carried: chat "You can't carry more of these items." in (1, 0.2, 0.2, 1) and sound 0x31 at the
player.

**Right press over no widget** (0x47b70f …): hides the inventory (G+0x8008bc) and the windows
G+0x8008c0 and G+0x8008dc, clears the two bytes G+0x8008f0 / G+0x8008f1, and clears the
menu-button-row flag G+0x8008f2 (§7).

**Sorting**: there is none in Alpha.

### 8.1.9 Picking items up from the world

* **Target.** The interaction target G+0x800a78 holds (zone *x*, zone *y*, index) of a world item
  in that zone's item list (zone +0x30, 328-byte records, `alpha-net-packets.md` §3.7); index −1
  = none. How the target is chosen (range, aim) belongs to the interaction-target rule of §5.8
  (unresolved: I-2). There is no pick-up prompt text; while aiming at an item its tooltip is
  shown at the bottom right (§8.1.7, world source).
* **Trigger: E**, edge-triggered (controller flag +0x11 down, previous state up, 0x4975c1).
  Nothing is automatic: walking over items does not collect them.
* If the item exists (0x59fb90) and passes **"can carry"** (§8.1.1, 0x43e4a0): a 300-byte item
  record (zeroed by 0x459530) with the target triple copied to record +0x11c … +0x127 is queued
  at creature +0x130c and sent to the server as **packet 6** (one per tick). Packet 6 is an item
  request — **pick up** (target set, item empty), **drop** (item set, §8.1.8) and **use**
  (consumable subtype 7, byte +0x128 = 1, §8.1.10).
* If it cannot be carried: chat line "You can't carry more of these items." in (1, 0.2, 0.2, 1)
  and sound 0x31 (`human-voice01.wav`) at the player.
* **Receiving** (unresolved: U-F6-5). The server answers in world-update section 9 (288 bytes:
  u64 player id + item; client 0x48f8a0): type 13 → the player's experience += 10 × the item's
  +0x10 value (level-ups as `alpha-creatures-behaviour.md` §4.6); type 25 → mana-cube counter
  (+0x1164) += 1, action 109 with action time 0, chat "You receive a mana cube." and, every
  fourth one, "You gain one skill point." with sound 0x1e; every other item → the chat
  announcement (0x480fb0): "You receive " (local player) or "<name> receives " in white, then for
  coins "<*q*> <coin name>" in white, otherwise (quantity types: "<*q*> X ") + name + (" +" +
  *p*(item) for non-quantity items) in the name colour, then ".\n" in white.
* **Into the bags.** Items reach the inventory through the incoming-item queue G+0x1000e68
  (pushed by the receive path and by disenchanting): every frame a timer G+0x1000e70 += frame ms,
  whether or not the queue holds anything; when the queue is non-empty and the timer exceeds
  **100 ms**, the timer is reset to 0, the front item is added (§8.1.1, automatic tab), sound
  0x2d (`pickup.wav`) is played at the player and the grid refreshed (0x48cda1 … 0x48ce23). So
  queued items arrive one per 100 ms, and the first one after a quiet period arrives at once.
  No carry test is made at this point.

### 8.1.10 Using items: quick-item wheel, Q and right-click

* **Candidates** (0x47ae10): every slot of every bag, in bag then slot order, with count ≠ 0,
  type 1 and item level (+0x10) ≤ the player's level *L*. The selected quick item is candidate
  (G+0x800a44 mod count) (negative indices wrap). With no candidates the wheel is closed and the
  quick-item button is empty (§5.5, §5.7) (unresolved: U-F6-6).
* **Q** (0x4920ae, edge-triggered: controller flag +0x17 down, previous state up): if the
  selected item's level > *L* → sound 0x32 at the listener; else it is used. Further gating
  (window state, candidates present) is unresolved (U-F6-6). **R/E** in the wheel: §5.7.
* **Use** (0x4a2780), type 1 only and only while the player's hit-stun counter (creature +0x12c,
  record field 16) is < 1 (unresolved: U-F6-6):
  * subtype 7: a packet-6 record with byte +0x128 = 1 and the item is queued (§8.1.9);
  * other subtypes: the item is copied to the held-item field (record field 43, creature
    +0x1e8), the action becomes **80** (drink) for subtypes 1 and 2, **81** (eat) for the
    others, action time 0, creature +0x128 := 0. The model is shown in the hand
    (`alpha-creatures-models.md` §6.5). Eating (81) ends as action 83 once the action time
    exceeds the item's use limit. The health effect of the action (the tooltip's "HP +" value)
    is applied by the action step (unresolved: I-1).
  * the stack count drops by 1 (cleared at 0).

### 8.1.11 Sounds used here

0x2d `pickup.wav` (item arrives), 0x2e `disenchant2.wav`, 0x2f `upgrade2.wav` (recipe learnt),
0x31 `human-voice01.wav` (refused: cannot carry, not allowed here), 0x32 `human-voice02.wav`
(refused: level, class or tab), 0x39 `drop.wav` (buy), 0x3a `drop-coin.wav` (sell, volume 0.75),
0x56 `menu-tab.wav`, 0x57 `menu-grab-item.wav`, 0x58 `menu-drop-item.wav` (sound table
0x484350). Sounds played "at the player" use the player position (creature +0x10);
the others use the listener position (G+0x140).

### 8.1.12 Open points

* **I-1** The consumables' effect (healing amount and timing) inside actions 80/81, world step
  0x60c510.
* **I-2** How the interaction target G+0x800a78 picks a world item (range and aim).
* **I-3** Whether the `equippedpreview` node is ever hidden: no code toggling it was found.
* **I-4** The meaning of controller flag +0x29 (blocks the bag-slot swap).

---

## 9. Chat

* **Enter** opens the chat input (chat widget G+0x800a14, flag +0x180) when the key gate of §11.2
  passes and the input is closed. While the input is open every key-down goes to the input
  (0x43a0d0) and typed characters arrive through WM_CHAR. **Enter** again submits: if the text
  is non-empty it is interpreted, then the input closes (0x47e1b0).
* Commands (0x47e1b0, compared with 0x439190):
  * `/name <name>`: a non-empty argument is copied as the player's name (creature +0x1168);
  * `/connect <address>`: connects to the address (`alpha-net-transport.md`);
  * `/disconnect`;
  * `/namepet <name>`: a name of 16 or more characters is refused with "Pet name too long" in
    (1, 0.2, 0.2, 1); a shorter name is written to the pet item in the Pet slot, only when that
    slot holds a pet (type 19).
  * Anything else is sent as a chat message; the game uses one of two paths depending on
    G+0x8006cc (unresolved: U-F6-9).
* While the chat input is open, game keys are not processed.

### 9.1 Chat box layout and drawing (ChatWidget, constructor 0x4393b0, draw 0x439730)

* **Box.** A code-drawn widget of 400 × 200 under the in-game root, top-left at (20, H − 220)
  (size set at 0x465a05, position at 0x482a40). No background or frame. Always drawn in game;
  only the input caret depends on the input flag (+0x180).
* **Font.** The widget's font (`resource1.dat`), 10 px, outline radius 2, for every piece of text
  in the box. Each piece is drawn twice at the same pen position: first the outline pass (black
  outline at alpha 1, transparent fill), then the fill pass (the piece's colour at alpha 1, no
  outline).
* **History.** At most **10 lines**: at the start of every draw the oldest lines are dropped while
  more than 10 exist. Line *k* (0 = oldest) has its baseline at box y 19 + 18·*k*. A line is a
  list of words, each with its own RGB colour (bytes ÷ 255); drawing starts at box x 3; a space
  word advances 5 px, any other word is drawn and advances by its measured width.
* **Adding text** (0x43a500; colour given as three bytes): the string is split into words at
  spaces, line feeds and carriage returns; words are appended to the **last** line; when the
  line's width would reach the box width − 20 (= 380 px) a new line is started (word wrap); a line
  feed or carriage return ends the line. Message strings therefore end with a line feed
  ("You receive a mana cube.\n"). A variant (0x43ab30) takes a float colour.
* **Input line.** The typed text (+0x168) is drawn at box (3, 200 − 12 = 188), white with the
  black outline pass. While the input is open the caret "|" blinks — visible when
  ⌊time ÷ 500 ms⌋ is odd — at x = (pen x of the cursor position) + 3, same baseline, white over a
  black outline.
* **Colours used by the game** (0x48ccc0, 0x48cd44, 0x48f959, 0x48fa34, 0x4940d9, 0x49766e):

| Message | Colour (RGB 0…255) |
|---|---|
| chat: sender name + ": " | (0, 255, 255) cyan |
| chat: the message text | (255, 255, 255) |
| "You receive a mana cube." | (100, 150, 255) |
| "You gain one skill point." | (150, 128, 255) |
| "You gain " *n* " XP." | (150, 128, 255) |
| "You can't carry more of these items." | float (1, 0.2, 0.2) |
| "Pet name too long" | float (1, 0.2, 0.2) |

**Worked example (H = 720).** Box (20, 500) … (420, 700). With 3 lines in the history their
baselines are at window y 519, 537, 555, starting at x = 23; the input line's baseline is at
y = 688. A chat line "Bob: hello there" draws "Bob:" in cyan at x 23, then (after the space,
+5 px) "hello" in white, and so on.

---

## 10. Interface sounds

Sounds are played by id through 0x484350 (the id selects a `data2.db` file name; the call takes a
position and a volume factor; a sound farther than a cut-off from the listener G+0x140 is not
played, nearer ones are attenuated with distance and panned). Interface ids:

| Id | Sound | Played when (call sites) |
|---:|---|---|
| 0x53 | `menu-open2.wav` | a window opens |
| 0x54 | `menu-close2.wav` | a window closes (0x48b33d) |
| 0x55 | `menu-select.wav` | a menu item or button is clicked (0x42b810 … 0x42bab0, 0x47b899, 0x497a97) |
| 0x56 | `menu-tab.wav` | a tab is chosen |
| 0x57 | `menu-grab-item.wav` | an item is picked up with the mouse (0x47c548) |
| 0x58 | `menu-drop-item.wav` | an item is put down (0x47ba7e, 0x47d2aa, 0x47da26) |
| 0x59 / 0x5a | `craft.wav` / `craft-proc.wav` | crafting (0x470c9a, 0x470d11) |
| 0x1d | `levelup2.wav` | level up (0x493e8f) |
| 0x1e | `missioncomplete.wav` | quest complete (0x48f9be, 0x4907d7) |
| 0x2d | `pickup.wav` | an item arrives in the bags |
| 0x2e | `disenchant2.wav` | disenchant |
| 0x2f | `upgrade2.wav` | upgrade, recipe learnt |
| 0x39 / 0x3a / 0x3b | `drop.wav` / `drop-coin.wav` / `drop-item.wav` | buying, selling, dropping |

The full id table (0–100, combat, ambient and creature sounds included) is in 0x484350; it selects
files by id only, without random variation.

---

## 11. Controls

### 11.1 Held keys and buttons (DirectInput, polled every frame; main loop 0x4c8ae0)

The main loop (0x4c8ae0, after the Direct3D set-up 0x4c8720) reads the keyboard (DirectInput 8,
256 scan-code bytes) and the mouse (DirectInput mouse state) only while the window is active and
has focus:

| Scan code (key) | Controller flag | | Mouse | Controller flag |
|---|---|---|---|---|
| 0x11 (W) | +0x0b | | left button | +0x04 |
| 0x1F (S) | +0x0c | | right button | +0x05 |
| 0x1E (A) | +0x0d | | middle button | +0x0a |
| 0x20 (D) | +0x0e | | movement / cursor | method +0x18 |
| 0x12 (E) | +0x11 | | | |
| 0x13 (R) | +0x0f | | | |
| 0x14 (T) | +0x10 | | | |
| 0x39 (Space) | +0x12 | | | |
| 0x2A (Left Shift) | +0x14 and +0x15 | | | |
| 0x1D (Left Ctrl) | +0x13 | | | |
| 0x10 (Q) | +0x17 | | | |
| 0x02–0x05 (1–4) | +0x06 … +0x09 | | | |

* **Movement.** In cursor mode the method +0x18 receives the cursor position in client pixels;
  otherwise it receives the DirectInput relative motion of the frame.
* **Cursor.** The system cursor is hidden for the whole session (ShowCursor at start-up). Outside
  cursor mode, while the window has focus, the cursor is put back to the window centre every
  frame; on entering cursor mode it is placed once at the interface's stored cursor position.
* The loop sleeps out the rest of the frame period held by the game object (frame limiter).

The bindings are fixed (no key configuration). The meaning of each flag (movement, jump, dodge,
abilities, interaction with R, items with Q) belongs to the player-control specification.

### 11.2 Key presses (window messages, 0x4c83f0 → GameController 0x47e1b0)

Key-down messages (WM_KEYDOWN, virtual keys) reach 0x47e1b0. They are processed only when none of
five screen containers is visible (character creation G+0x800880, character list G+0x800888,
world list G+0x80088c, world creation G+0x800890, connect G+0x800894; §1) and no widget holds
the input focus (0x6531e0). While the chat
input is open, only Enter is interpreted (§9). Otherwise:

| Virtual key | Action |
|---|---|
| Enter | open chat (§9) |
| Tab | toggle the quick-item wheel (G+0x800a40, §5.7) |
| Esc | leave map mode (G+0x8006e4 := 0) and toggle the menu-button row (G+0x8008f2, §7) |
| B, I | toggle the inventory |
| C | toggle the crafting window |
| F | toggle player flag 0x200 (creature +0x124) |
| G | the Special slot (+0xf08): see below |
| M | toggle the world map (map mode) |
| O | toggle the system panel (§3.3, §7) |
| V | toggle G+0x8007b4: show life bars over all nearby creatures, not only players and focused ones (§5.4) |
| X | toggle the skill window |
| F1 | toggle the help picture (G+0x80089c) |

**G** (0x47e1b0): with the Special slot empty, player flag 0x10 is cleared and an action 107 is
ended (action := 0). With a Special item of subtype 0, player flag 0x10 is set if it was clear and
the test 0x42c860(2) holds, else cleared, and the action is reset to 0. With subtype 1, action 107
is ended if active, else entered (action 107, action time 0) only when bit 1 of creature +0x5c is
set. Which subtype is the glider and which the boat is unresolved (U-F6-8).

Mouse buttons and the wheel also arrive as messages: buttons 0 left, 1 right, 2 middle (press
0x47b600, release 0x47ddd0); the wheel sends the sign of the wheel delta, ±1 per message
(0x47ef40); mouse movement goes to 0x47ea00. The options `cameraSpeed` and `invertY` (§6) are read by
that mouse look (§6.4); the reader of `cameraSmoothness` is unresolved (U-F5-5).

---

## 12. Unresolved

Every item below is open. Statements in the body that depend on them carry the same tag. Items
marked *believed* or *inferred* in the body are also unproven, even where they have no tag.

**Text, screens and layout**

* U-F1-1 What the `language` option changes. No German dictionary load exists (§0, §6.4).
  U-F5-11 records the matching negative search of the settings block for a reader.
* U-F1-2 How the node placement and the widget placement of window G+0x8008dc combine (§2).
* U-F1-3 The `blackwidget` template's shape data. Inherited and not proven: a 12-corner
  rectangle with 5-px notched corners, black fill at alpha 0.67, a 3-px black stroke with
  alignment 1, and a second outline with no fill and a 2-px white stroke with alignment −1.
* U-F1-4 What is drawn behind the start screen when no world is loaded (§3.3).
* U-F1-5 How the frame code between the clear and the loading test (0x4ac394 … 0x4ad472)
  affects the loading picture.
* U-F1-6 Whether `start.plx` holds any sequences.
* U-F1-7 That text flag bit 2 centres a block vertically (§3.4).
* U-F1-8 Where the character style panel's arrows are placed, how the hair colour is chosen,
  and whether the preview turns (§3.6). The positions of the character-list entries are also
  open.
* U-F1-9 The crosshair geometry. Inherited and not proven: four 7 × 7 white squares at ±9 px;
  the zoom form uses 20 × 4 bars and adds a fifth mark at +13.
* U-F1-10 Whether code plays the `hit` / `button:enter` sequences of `combopoints` and the
  `show` / `show2` / `shine` sequences of the quest tags.
* U-F1-11 How "Create World" (0x482530) uses the name and seed edits.
* The `landname` banner fonts (inherited, not proven: `resource2.dat` at 69 px for `name` and
  40 px for `detail`) and the identities of nodes G+0x800944 and G+0x80094c (§2).

**Minimap and world map**

* U-F2-1 What the call 0x632870 on G+0x800754, made before the minimap's depth clear, does.
* U-F2-2 The darkness and shininess constants in force while the minimap draws.
* U-F2-3 The meaning and reader of the point stored at map object +0x84 … +0x8c.
* U-F2-4 What the other discovery callers (0x4887aa; 0x48d204) mark.
* U-F3-1 The content of models 2559 `compass2` and 2567 `maptile`, and the compass's placement
  and lens inside 0x476660.
* U-F3-3 The meaning of the location record's mission fields (+0x34, +0x41, +0x4c, +0x50) and of
  object type 45 (believed to be the portal).
* U-F3-4 Whether the zero-size map containers can ever be hit by the widget test 0x650ae0.
* U-F3-5 The identity of world object +0xb8 in the town-marker distance (believed to be the local
  player).
* U-F3-6 How the player is lifted out of the ground after a teleport to height 0.
* Inherited inferences used in §5.1–§5.2: G+0x8006e8 is in milliseconds; G+0x2bc is the
  client's current zone; appearance flag 0x2000 marks a mission target; G+0x18c is the mouse
  sensitivity and G+0x194 the invert flag.

**HUD**

* U-F3-7 The light factor in the creature-bar range, and the meaning of origin G+0x140 passed to
  the visibility test 0x59ee90.
* U-F3-8 The meaning of creature +0x198 (believed: rider/mount link) and of animation states
  0x6a / 0x6b (0x6a believed: riding).
* U-F3-9 The stroke colours of the bar frames.
* U-F4-1 That player creatures have power tier 0.
* U-F4-2 The `gui.plx` geometry quoted in §5.5, §5.7 and §5.8 (ability-button shapes, label
  origins and sizes, selector and prompt text origins).
* U-F4-3 Which content store the ability icon files are read from.
* U-F4-4 The ids registered for the eight specialization icons.
* U-F4-5 The vertical term of the hit-text anchor that involves the target's size
  (0x494e9b … 0x494f53).
* U-F4-6 The rule of the respawn search 0x5a03d0 (inherited, not proven: the nearest spawn
  point in the 3 × 3 zones).
* U-F4-7 Which of the two `Cave` dictionary entries the loader keeps.
* U-F4-8 The setter 0x5a52e0 of the grid-name offsets W+0x80028c / +0x800290.
* U-F4-9 What the per-frame call with argument 0 on `star1` … `star4` under G+0x800858 does.
* U-F4-10 Whether the code-made `landscape` / `landscapedetail` label (G+0x800858, at
  (W − 260, 90)) and the TIME/TEMP readout (G+0x800860, at (W − 15, 20)) are visible in normal
  play. Which code uses the `textFX` and `minimap` strings is also open.
* The meaning of the hit flags +0x14 (60-px text with rise −200; believed: critical) and +0x44
  (yellow text).
* The meaning of player flag 0x200 at creature +0x124, toggled by F.

**Options, menus and windows**

* U-F5-1 The `leftbutton` / `rightbutton` widget size, `border` outline and `rightbutton`
  geometry. Only the `leftbutton` fill was checked.
* U-F5-2 The `button2` look after resizing, and with it the caption point (639.83, 549) of §6.7.
* U-F5-3 Hover and press feedback of the arrow and button templates. The library has `button:*`
  sequences; the shipped templates are believed to contain none.
* U-F5-4 The `menubutton` hit rectangle and label geometry and colours.
* U-F5-5 The reader of `cameraSmoothness` (G+0x190).
* U-F5-6 The meaning of the state byte G+0x800a40 in the window-sound condition of §7.
* U-F5-7 The cross-specification item and save facts of §8.1.1: the equipment slot layout, the
  outgoing queue and packet 6, the rule for counts below 1, negative counts drawn black, the save
  field numbers, and that the item in hand is not saved.
* U-F5-8 Whether the quick-item wheel and Q use the "can use" test 0x43e420.
* U-F5-9 That item power via 0x43ca60 equals g(item level); the value *F*(5, 0) = 1.4142135; the
  format of the displayed price text.
* U-F5-10 The §8 table rows not traced: tab captions, crafting categories and sounds 0x59 / 0x5a,
  and how the vendor, enchanting and adaption windows are opened.
* U-F6-1 The `gui.plx` template geometry and colours quoted in §8.1.3, §8.1.5 and §8.1.7: the
  itembox, the tab sizes, the preview panel and its stars, and the equipment-box parts.
* U-F6-2 The character-panel value formulas (regeneration, REG percentage, 0.1 rounding, armour
  rating weights) and the vendor wide-row price layout.
* U-F6-3 The tooltip text-block rows (0x4a28c0), heal 0x4c6e10, the ridable list 0x444760 and the
  order of the compare list.
* U-F6-4 The left-press extras on equipment boxes, the pet-food rule, bag-slot merge and swap, the
  use/equip table rows other than shields and right-button bows and others, the type-11 split,
  and non-weapons when window G+0x8008dc is open.
* U-F6-5 The receive path of world-update section 9 (0x48f8a0) beyond the mana-cube block.
* U-F6-6 The quick-item candidates (0x47ae10), the use routine (0x4a2780) and any further gating
  of Q.
* U-F6-8 Which Special-slot subtype (0 or 1) is the glider and which the boat; the meaning of
  player flag 0x10, bit 1 of creature +0x5c and the test 0x42c860(2).
* U-F6-9 Which chat-send path (G+0x8006cc zero or non-zero) is the offline one.
* U-F6-10 The condition of the first test in the cursor-label update (0x48b3ef), and what window
  G+0x8008dc and widget G+0x8008f4 are.
* U-F6-12 The directional lights in force when the interface draws item models.
* U-F6-13 Whether the negated horizontal projection scale mirrors interface item models relative
  to their appearance in the world.
* I-1 … I-4 of §8.1.12.

**Not specified in this document.** These belong elsewhere or are still to be written: the
ability-slot assignment rules (0x48b8ab … 0x48bd3e; player-control specification); the skill
window layout and skill-point spending (SkillWidget 0x4dd750); the crafting window and the
blueprint preview (0x42f190); the speech bubble, objective list and statistics widgets
(0x4e5c90, 0x4ce180, 0x583b40) and how they render the dictionary markup (`[text $style]`, `%`,
`@name`).

---

## Re-certification (2026-11-15)

**Image.** `Cube.exe`, SHA-256 84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf,
image base 0x00400000. Addresses are VAs, so RVA = VA − 0x400000. **Evidence:** the instruction
listing and per-function decompile of that image, its defined strings and string references,
the RTTI type names, raw `.rdata` / `.data` constants read from the file, and the shipped
`gui.plx`, `quest-tag.plx`, `start.plx`, `cursor.plx`, `options.cfg` and `data4.db` dictionaries,
used as content evidence only. Infinite-Cubes/alpha reconstruction leads were **not consulted**.
No random draw occurs anywhere in this document's scope: the name makers, land names, minimap,
map, bars and menus call no random routine. **No random-draw order was changed.** Two corrections
change a deterministic order instead: the creature-marker drawing order (correction 37) and
the right-press handling priority (correction 110).

Tags: **[R]** marks a reversal, where the old claim was inverted or wrongly attributed.
**[DO]** marks a change in order.

### Corrections

*§0–§5 introduction*

1. [R] §0 language. Old: "the `language` option selects the dictionary". New: `dict_en.xml` is
   a fixed literal (0x4e1b3e in 0x4e1970), and nothing in the image refers to `dict_de.xml`. The
   effect of the option is U-F1-1.
2. §0 dictionary contents. Old: names, landscapes and speech only. New: `<item>` entries also
   exist.
3. §0 hard-coded text. Old: all text is in the executable. New: it is in the executable or in
   `.plx` template text ("QUEST COMPLETE!", "WEAPON 1", "ENEMY", "Stamina", "20000/2000o").
4. §1 roots. Old: the start-screen root holds four screen containers G+0x800888 … +0x894. New:
   seven sibling top-level containers with identified roles (0x45a778 … 0x45aa95; transitions
   0x47b600, 0x4814f0, 0x4821a0, 0x481fe0, 0x4815e0).
5. §2 speech row. Old: G+0x80093c is a 400 × 400 widget. New: G+0x80093c is the container; the
   widget is G+0x800938 (0x4e5c90, 0x461ccc), and the placement is a direct translation
   (0x482a67, 0x482aa2).
6. §2 label G+0x8009f4. Old: unnamed. New: the red name-error label "Error" (0x45b8b8 …
   0x45b9f5).
7. §2 side windows. Old: y = H/2 − height (the window's own). New: y = H/2 − h₀, using the
   inventory's height (0x483966, 0x483bba, 0x483c50, 0x483ce6, 0x483da3).
8. §2 G+0x800ad4. Old: "lowered by its height + 10". New: H/2 − h₀ + 10 + the height of window
   G+0x8008c0 (0x4839c8, 0x483a45 … 0x483a73).
9. §2 G+0x8008dc. Old: (W/2 − 340, H/2 − 250) "when the 3D preview is active". New: an
   unconditional node placement plus the left-of-centre widget placement; there is no preview
   condition (0x48362f … 0x4836b2, 0x483d1d … 0x483dd5).
10. §2 missing anchors added: options window, character style panel, the screen buttons, Back,
    the character panel host, the overridden first inventory placement, and the final cursor
    centring (0x482e71 … 0x483e52). Each row now states its placement kind (pivot, direct or
    top-left).
11. §2 crosshair and selector rows. Moved from the layout routine to the per-frame update
    (0x4927da, 0x4928ae, 0x49275e).
12. [R] §3.2 start-menu sound. Old: clicks play 0x55. New: no sound on this path. The 0x55 calls
    in 0x47b600 belong to the character-style arrows.
13. §3.2 actions. Old: "to the character screen"; "quit". New: the exact transitions (hide the
    start root, show G+0x800888; show G+0x800a00; set G+0x1a0). The hovered index resets to −1
    on every draw, and hover requires mouse x ≥ 0 (0x47b600, 0x583320).
14. §3.2 draw passes. Old: a black outline first, then the fill. New: first a white fill with a
    4-px black outline, then the item colour without outline (0x583320).
15. §3.3 start-menu size. Old: the host is sized 200 × 100. New: the StartMenuWidget is sized
    (0x463fd7); the host is an empty node (0x463f61).
16. §3.3 clear. Old: 0x4ac345. New: Clear at 0x4ac343, flags 3, depth 1, stencil 0.
17. §3.3 loading state. Old: hides every root except the label, presents, and skips the world.
    New: it saves, hides and restores exactly six containers (not G+0x800894 or help), draws the
    interface, ends the scene, releases the world lock and leaves the routine (0x4ad47e …
    0x4ad67f). The earlier block still runs (U-F1-5).
18. §3.3 system and options panels. The construction details are added: system 150 × 120 at
    0x464a74, widget G+0x800914; options 450 × 440 at 0x46491e, widget G+0x8009fc.
19. §3.4 pixel snapping. Old: "rounded half up, unscaled text". New: add 0.5 and truncate on
    both branches (0x65bc70).
20. [R] §3.5 duplicate-name messages. Old: emitted by the click handlers. New: emitted by the
    update routine (0x4898c0, 0x489c62). The "Create Character" handler hides G+0x800880,
    shows G+0x80088c and copies the name to +0x1168 (0x4821a0).
21. §3.5 "row element 40 × 40". New: a provisional position (40, 40) given to three buttons
    and replaced by the layout (0x45cd5c … 0x45cd7d).
22. §3.5 "Back", "Connect to server", "Connect". The vague targets are replaced by the exact
    order of checks and transitions (0x4814f0, 0x481fe0, 0x4815e0).
23. §3.5 edit table. Old column: "menu key". New: "group node name" (0x45d06b … 0x45d09d).
24. §3.5 strings. "New character" and "LVL " belong to the CharacterPreviewWidget draw
    0x425450. The connection messages move to §4.
25. §3.6 character creation. Old: "row positions and gender captions open". New: label
    baselines (15, 27 + 30k), value centre 100 + (w − 110)/2, "Hair color" at (w/2, 182),
    "Male" / "Female" by widget +0x194, panel 300 × 330 (0x428e40, 0x464803; constants
    0x6fd0b4, 0x6fd0b8). "Turning preview" is removed (U-F1-8).
26. §4. Old: status into "the red error label / info label". New: failures go to G+0x8009f8
    (0x46fe10 … 0x4709a1); "Connected." is posted in (1, 0.5, 0.2, 1) (0x47009d).
27. [R] §5 `lifebar` `info`. Old: "HP". New: the player's name (0x490f78 … 0x490fc1). "HP",
    "MP" and "XP" are static template text that code never writes.
28. [R] §5 stars. Old: `star1`–`star4` on the enemy life bar show rarity. New: they are cloned
    under the land-name label G+0x800858 (0x45af8b, 0x45c399) and used by the land-name code
    (0x494722 … 0x494826).
29. §5 sounds. Sound 0x1e is added for "You receive a mana cube." (0x48f9be).

*§5.1 minimap and tiles*

30. [R] Land path record test. Old: byte +0x28 = "ready". New: the **visited** flag
    (0x5fcfa8 … 0x5fcfb6; writer 0x48d5bc).
31. [R] Land slab extent. Old: from −32 to 0 map units. New: from −32 up to 32·value/256 for a
    non-negative elevation value; from −48 to −16 when the value is negative (0x5fd3b0 …
    0x5fd4cf; 1/256 at 0x702a2c).
32. Zone path. Old: "skip zones whose region record is missing". New: without extras, a
    missing record or tile draws no terrain (0x5fd809 … 0x5fd8b7).
33. Rise-in timer start. Old: "not traced". New: 250, set on load and on the first build
    (0x603e7d, 0x604e7d).
34. Zone markers. Old: the entries were "not traced" and used the stored view. New: they are
    the land-border markers and use the view stored by the **previous** call (0x5fede6 versus
    the store at 0x5ffe0c).
35. [R] Marker tile. Old: 4 × 4 × 1. New: allocated 4 × 4 × 4 with only the bottom layer filled
    (0x5faf3d … 0x5fb009).
36. Creature-marker range. Old: measured to the player. New: measured between the pivot and
    creature +0x10 (0x5fff91 … 0x60006b).
37. [DO] Creature markers. Old: "sorted by their projected depth". New: sorted ascending by the
    negated view depth, so the farthest is drawn first; both passes of one creature are drawn
    before the next creature (0x600157 … 0x6002b5, 0x60028d, 0x5fa9e0). This is a
    deterministic sort, not a random draw.
38. Map counter +0x8000b8. Old: advanced as state left behind. New: advanced at the start of
    each call (0x5fc211).
39. Point +0x84. Old: "for the overlay". New: its reader is unknown (U-F2-3) (0x600318 …
    0x60053e).
40. Worker centre region. Old: floor. New: truncation toward zero (0x4696b4 … 0x4696c0).
41. Worker ownership check. It is repeated before each region and land step, under the lock
    G+0x8005e8, and the 1000-ms unload still runs on a mismatch (0x46962f … 0x469bc7).
42. Empty-column colour. The source is now stated: the static default block, initialised as
    water (0x604164 … 0x6041bd, 0x6faf20).
43. Body read. "Type 1 grey" becomes the static type-1 block, whose colour is irrelevant
    because Rk is used (0x6048b9 … 0x60490a).
44. Worked body voxel. The examples now use explicitly coloured block types, not 0–3, because
    type 1 takes Rk.
45. Visited-land lookup (0x601cc0). Old: the nearest displaced distance. New: the smallest
    **truncated** squared distance; the first found wins on ties.
46. Quadrant sums. Old: "never read". New: "no use found" (0x604650).
47. Minimap caller. The untraced call 0x632870 is added (U-F2-1), and the visibility node is
    identified as interface +0xb4 (0x4baa67 … 0x4baa6d, 0x487490).

*§5.2 world map; §5.3–§5.4 bars*

48. Overlay centre zone. Old: floor((player + pan)/256). New: truncating divisions by 65536
    and then by 256 (0x4c96f1 … 0x4c9799).
49. Party bars. Old: "type 0". New: controller kind +0x60 = 0 (0x491a1b … 0x491a2b).
50. [R] Creature-bar position. Old: "+2 raises the point for states 0x6a / 0x6b". New: +2 is
    added to the camera-space depth after the transform and does not raise the point
    (0x49a13e … 0x49a170).
51. Creatures without a bar. Old: "a creature the player is riding". New: any creature whose
    +0x198 link names a creature in state 0x6a, and the creature linked to the local player
    when the local player is in state 0x6a (0x499f0e … 0x499f2e, 0x49a174 … 0x49a197).
52. Cursor label. Old: "node from cursor.plx (0x461e1b)". New: a container made at 0x461e29,
    loaded at 0x461e7b, shown exactly in cursor mode and moved to the cursor every update
    (0x48cbc3 … 0x48cc0d).
53. Map root. "Hidden at start-up" becomes: hidden by every non-map frame (0x4ae357 …
    0x4ae369).
54. Wheel. Old: n = ±1, ÷0.9 for n > 0. New: n = the sign of the delta; ×0.9 for n < 0,
    otherwise ÷0.9 (0x4c856c … 0x4c858b, 0x47ef4c … 0x47ef64). The closed-map case is added:
    camera distance −= 2n, clamped to [0, 14] (0x47efa6 … 0x47eff3).
55. Placeholder worked example. Old: 81.6. New: 81.55.
56. Bar frames. Old: "black stroke". New: stroke only (flags 6); the colour is U-F3-9.
57. Added: three cyan tick lines on the `staminabar` frame (`gui.plx`).
58. R at a portal. "Edge-triggered, wheel closed, alive" is replaced by the actual tests:
    G+0xf against 0x76b138, G+0x800a40, +0x16c > 0, G+0x800a84 ≥ 0 and type 45 (0x49735f …
    0x497432).
59. Left drag. Added: Δy·k·0.005 to the pitch target and −Δx·k·0.005 to the yaw target, the
    sign set by G+0x194, clamped to [0, 180] (0x47ed00 … 0x47eddc).
60. Pass A colours. Old: the level rule applies to kind 3. New: it applies to every kind
    except 1 and 4 (0x4c9f70 … 0x4c9f7c).
61. Pass B. Added: the level-power formula, and that the map cell is zone ÷ 8 truncated
    (0x4ca505 … 0x4ca52c).

*§5.4.1–§5.9.1*

62. [R] Location-key table. Old: one entry per (type, 0); towns of style 1–5 and oasis
    variants 1–2 have no name; the home-town example gave "". New: the eleven types are
    registered for every variant 0 … 9 by a loop (0x592a34 … 0x592cf6), so every town style is
    named "*N* City".
63. Code-made label position. Old: (20, 45). New: (W − 260, 90), set every frame (0x494161 …
    0x49419e).
64. Banner timer. Alpha is computed from *T* before the frame's decrease; *T* is the static
    0x76b134 (0x494464 … 0x4944f4).
65. Floating-text entries. Offsets and defaults are added from the initialiser 0x4664b0
    (rise −100, lifetime 1000).
66. Hit rise. Old: −200 whenever the +0x14 flag is set. New: −200 only when the local player
    is not the target and the hit is damage; +100 for heals and for damage taken by the local
    player (0x494f58 … 0x4950d7).
67. Removal. Old: "removed on the next frame at 1000 ms". New: removed once the age exceeds
    the lifetime (strict).
68. Flags 2. "Looks odd; settle by capture" becomes a plain statement: flags 2 is the
    right-alignment bit (0x493dd3, 0x493f36).
69. Wheel use. Added: the requirement is the signed 16-bit value at item +0x14; on failure the
    wheel stays open (0x497e06 … 0x497e7b).
70. Wheel easing. Old: a closed formula. New: an iterated weight per whole millisecond in
    double precision, rounded to single (0x4ac150, 0x4aba20).
71. Icon table location. Old: "0x45ffxx". New: 0x45e1e3 … 0x45f317 into G+0x800820. The
    content store is U-F4-3.
72. Quick-item button. Appended after the loop (0x460502 … 0x46050b). "Q" is the template's
    own text; the loop's "Q" branch is unreachable.
73. Empty title. Old: "the node is hidden". New: the empty string (0x49aa02).
74. Species label. Stream-implementation wording is removed; the suffix and line break occur
    only for a non-zero tier (0x49ae66 … 0x49ae9f).
75. Power tier. Unverified generator VAs and the player-tier claim are removed; only the copy
    0x610049 … 0x61004c is stated, and the player tier is U-F4-1.
76. Level power. The cross-spec "g/3 of the health formula" remark is removed.
77. Grid name. Old: 0 ≤ x, y < 2²⁴. New: x is compared unsigned and y signed (0x5a6550).
78. Name-maker selector. Old: "(n, kind)". New: a selector; the land-name callers pass −1
    (0x4e5527, 0x4e56cf, 0x4e5b54).
79. Detail step 1. The bounds detail, which was not re-traced, is removed: E = the per-zone
    entry (0x4a6ad0), and none gives "".
80. Cave. The worldgen claim "type 9 is never created" is removed; which entry is kept is
    U-F4-7.
81. Worked examples. The seed-123 examples derived from other specifications are removed and
    replaced by examples computed from N (for example N(764) = "Aruron").
82. Code-made labels. Added: the per-frame call on `star1` … `star4` (0x494722 … 0x49480d;
    U-F4-9).

*§6–§8.1.2*

83. Resolution list. Old: filled by 0x4d4de0 in the options constructor. New: built by the
    main routine 0x4c8ae0 and copied by 0x468310 (0x4649ca); 0x4d4de0 (0x4649dc) copies the
    settings and picks the index.
84. Arrow hover hit test. Old: 0x629300. New: 0x6294c0 (0x4d3b8a …).
85. Device reset. Old: also on WM_SIZE. New: on WM_SIZE and WM_SETFOCUS (table 0x4c85a4); a
    failed reset is retried every 100 ms; the check runs after the update and before drawing
    (0x4c8633 … 0x4c86ec, 0x4c8a58 … 0x4c8a75).
86. `cameraSmoothness`. Old: read by the mouse look. New: no reader was found in 0x47ea00,
    where `cameraSpeed` and `invertY` are read (0x47ed16 …); see U-F5-5.
87. Options worked example. Old: FPS-Limit presses at y = 310, which hit Music Volume. New:
    y = 430; a row-9 line is added (0x4d46d0 arrow y values).
88. [R] Window sounds. Old: the window toggles play 0x53 / 0x54. New: the toggles play
    nothing; 0x53 and 0x54 follow changes of the window-open state, checked every update
    (0x48b2cb … 0x48b346, 0x76b0c0).
89. Inventory icons. Old: they include `inventory-consumables.png`. New: that name does not
    exist; the four names are listed (0x700d80 … 0x700dc8).
90. Coins. Old: the denomination is chosen by subtype. New: by the material byte +0x0d; other
    materials are stored like ordinary items (0x46ebe0).
91. "Can use" test. Old: used by the wheel and Q. New: its only callers are in the
    mouse-press handler (0x47bd6b, 0x47cfd6); see U-F5-8.
92. Side-window arithmetic. Old: "all floats". New: integer-halved W and H, and the
    inventory's height (0x483864 … 0x4838e4, 0x48396f).
93. Mode arrow. The debug output is "click " followed by a line break (0x4d4571 … 0x4d458e).
94. Resolution arrows. Added: with an empty list the right arrow leaves the index at −1
    (0x4d4d24 … 0x4d4d42).
95. The resolution list is built before `options.cfg` is read (0x4c8d9e, 0x4c8daf).
96. `options.cfg` reader. The key comparison order is added: `language` before `invertY`
    (0x4ce6e0).
97. Skills toggle. Added: it flips G+0x8008f1, copies the 11 skill values and the
    specialization, and hides the crafting window and blueprint preview (0x488c70 …
    0x488cfb).
98. System panel. Added: hover bands [10, 40) / [40, 70) / [70, 100), the x condition, the
    index reset and the outline-pass colour (0x587710).

*§8.1.3–§11*

99. [R] Item-model on-screen size. Old: 1.2071·H·σ/(1 + δ), so the tooltip is 91.5 px. New:
    1.2071·H·σ, independent of δ, so the tooltip is 86.9 px. δ is a depth offset applied after
    projection (0x476455 … 0x476591, 0x424f30).
100. Screen shift. Old: "believed after projection". New: confirmed as projection followed by
     shift (0x424f30).
101. Aspect sign. The claim that "the image is mirrored" is removed; the body states the
     world-camera sign convention, and mirroring is U-F6-13 (0x476422 … 0x476450).
102. Model 2396. Used only when the table holds at least 2397 entries (0x4758c0).
103. SpriteWidget raw model. Drawn through 0x476660, not 0x4758c0; a type-0 item draws
     nothing (0x51c3d0).
104. Weapon Rating. The player's rarity argument 0 is confirmed (0x4382d0 … 0x438359).
105. Inventory titles. Added: type 1 "Equipment"; type-2 tabs of 5 or more "Formulas"
     (0x4c2050).
106. Item in hand. Old: drawn every frame. New: drawn at the end of each inventory-type
     window draw; the cursor label also shows the item of widget G+0x8008f4 while window
     G+0x8008dc is open (0x4c595a, 0x48b3a5 … 0x48b627).
107. Scrolling. Added: *n* = 1 when the tab has no bag (0x4c6350).
108. Selector. Old: moved to the selected slot. New: shown and moved only while the tab has a
     scroll entry, otherwise hidden (0x4c5d50).
109. Right press over no widget. Old: "closes the inventory". New: hides G+0x8008bc,
     G+0x8008c0 and G+0x8008dc and clears G+0x8008f0 / f1 / f2 (0x47b713 … 0x47b740).
110. [DO] Right-press priority on boxes and slots. Old: vendor first. New: Adaption, then
     vendor, then G+0x8008dc, then unequip / use (0x47b600). This is the order in which input
     is handled, not a random draw.
111. Incoming-item timer. It runs even while the queue is empty (0x48cda1 … 0x48cdcb).
112. Enter. Old: "no window open". New: the §11.2 key gate applies, and there is no window
     condition (0x47e1b0).
113. Chat commands. Added: empty `/name` is ignored; `/namepet` writes only to a type-19 pet;
     a name of 16 or more characters is refused. "Offline: added locally" becomes two paths by
     G+0x8006cc (U-F6-9).
114. Main loop. Old: 0x4c8720. New: 0x4c8720 is the Direct3D set-up and the loop is 0x4c8ae0.
     Cursor and movement handling are added.
115. Key gate. Old: "no start-screen container and no modal focus". New: five named screen
     containers and the focus test 0x6531e0 (0x47e1b0, 0x4c83f0).
116. G key. Old: "hang-glider toggle". New: behaviour by Special-slot subtype; the
     glider/boat mapping is U-F6-8 (0x47e1b0).
117. Sound attenuation. The wording is made generic: a cut-off, attenuation and pan, with no
     random variant selection (0x484350).

*Merge-level corrections*

118. §5.9.1 inputs. Old: "`dict_en.xml`, or `dict_de.xml` for German". New: `dict_en.xml` only,
     consistent with correction 1.
119. §11.2. The identity of G+0x800880 in the key gate is resolved through §1: it is the
     character-creation screen.
120. Former open items that the text now settles were closed and removed: the zone-marker
     meaning (land borders), the rise-in start value (250), the button-5 highlight (none),
     the flags-2 placement (right-aligned), and the character-creation row positions and
     captions.
121. Editorial, with no change of fact, across the whole file:
     * removed history wording: "This pass / second pass", "Correction (…)", "This corrects
       …", "resolves / closes / settles" and the F-, S-, M- and P- follow-up tags;
     * removed implementer advice ("an implementation should use the current …" and a remark
       about optionally resetting the options copy on open) and verification procedures
       ("to be confirmed by capture", "check by capture", "settle by capture", "Worth confirming in a recording", the "Checking note",
       and the "test vectors" headings, now "Worked examples");
     * removed owner-data references: "all 105 sampled tile records", the sample-derived
       sizes (95 × 98, 8–12 voxels, 32,600–32,800) and the user's own `options.cfg`;
     * removed the compiler-idiom wording for truncating division;
     * removed `gui.plx` details that were not re-derived (moved to U-items);
     * replaced the former §12 and §13 with §12 Unresolved.

### Confirmed

* §1–§4: the anchors and constants of every remaining row of the §2 table. These include
  the quest tags (400 = 0x6fd8ec), the land-name banner (230 = 0x702ab4), the help picture,
  the error labels (colour (1, 0.25, 0.25), 0x45bc84) and the chat box (400 × 200, 0x465a05).
  Also confirmed: the ability-row and menu-row formulas (0x460234 … 0x46050b); the start-menu
  font size, baselines, hover bands and cyan (0x583320); the clear colour 0xFF0070FF; the
  options reparenting and logo visibility (0x488f4e … 0x48901b); the text-layout rules
  (0x65bc70, 0x65c040, 0x65d530, 0x660d50); the node matrix order (0x678600); the button sizes
  and handlers; and the RTTI class names used in this document.
* §5.1: the zoom table; the projection, view and marker matrices; the scissor rectangle, which
  is never enabled anywhere in the image; the depth, fog, shader and light states; the model
  ids and class mapping; and every printed digit of the §5.1.5 worked example. In §5.1.6, the
  map worker loops, unload distances, region record layout, discovery, tile build, land-picture
  passes and the visited/rebuild rule, with all worked arithmetic.
* §5.2–§5.4: the key cases M, Esc, V and Tab; the zoom factor, clamps, easing and thresholds;
  the pan; the whole map-frame sequence; the placeholder, dimming and mission/town markers; both
  overlay passes, their colours and the hover box; teleporting; every bar anchor and text
  formula; the cast-bar caption table (decoded from raw bytes); the `gui.plx` geometry of all
  player and `:small` creature bars; the creature-bar range, template choice and name colours;
  and all worked examples except correction 55.
* §5.4.1–§5.9.1: all 154 species-key rows; all 17 syllable-table pairs and the name-maker
  formula; the level-power constants; the 32 neutral-animal types; the focus-test constants;
  the ability labels and highlight colour; the floating-text constants; the wheel constants;
  the revive sequence; the 8000-ms banner and its alpha; the land-name, location-name and
  detail logic; the zone keys; the grid-name tables; and the TIME/TEMP readout.
* §6–§8.1.2: the options panel construction, all eleven arrow handlers and ranges, the draw
  positions and value strings, the layout, the press, release and repeat events, Apply / OK /
  Cancel, the `options.cfg` keys, defaults, reading and writing, the display-mode enumeration,
  the menu-button table and brightness, the system panel, the window sizes and placements, and
  the item-storage rules.
* §8.1.3–§11: the inventory grid, scroll, tab, title, money and slot-drawing constants; the
  character-panel positions and rating bands; the equipment-box mapping and placement; the
  item-model matrix and projection; the turn vector; the name colours; the tooltip offsets; the
  mouse-action sound ids and volumes; the drop, pick-up and Q triggers; the 100-ms incoming
  queue; the chat layout, colours and commands; the sound-table entries; the DirectInput
  mappings; and the key cases.

### Cross-specification disagreements (facts stated in the body; the other specifications are not edited here)

* `alpha-ui-plasma.md` §4.1: its node matrix omits the "+ pivot" term (0x678600).
* `alpha-net-transport.md` §1.2: it calls `serverName` a menu key; it is the name of the
  edit-group node (0x459c40).
* `alpha-physics-camera.md` §7.2: it says a left drag moves the map; the left drag steers the
  camera targets, and the middle button pans (0x47ea00, 0x48b6ca). The same section also
  attributes a `cameraSmoothness` reader to the mouse look, which was not found there.
* `alpha-saves.md` §6.2 and §6.4: the `land` and `tile` record fields are as stated in §5.1.6.
  This was not rechecked against that specification's current wording.

### Unresolved

See §12. The ids are U-F1-1 … U-F1-11, U-F2-1 … U-F2-4, U-F3-1 and U-F3-3 … U-F3-9,
U-F4-1 … U-F4-10, U-F5-1 … U-F5-11, U-F6-1 … U-F6-6, U-F6-8 … U-F6-10, U-F6-12, U-F6-13 and
I-1 … I-4. Two ids were retired: U-F6-7 was resolved by §1, and U-F6-11 was merged into U-F5-5.
