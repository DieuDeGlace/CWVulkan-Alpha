# Alpha interface: screens, HUD, windows and controls

Behavioural specification of what the Cube World **Alpha** client (`Cube.exe`, SHA-256
`84a7a132…30bf`) shows and how the player drives it: the start screen and its menus, character
and world screens, the multiplayer connect dialog, options, the system menu, the in-game HUD,
the windows, and the keyboard and mouse controls. Written from the Alpha decompile; the file
format and drawing of the `.plx` interface files are in `alpha-ui-plasma.md`, which this page
assumes.

**Coverage.** This pass fully specifies the layout model, the start-screen menus, the options and
system menus, the anchoring of every HUD element the game positions, the controls, the UI
sounds and all interface text. The second pass adds the pivot rule (§2.1), the per-frame bar
rules with worked test vectors (§5.3–5.4), the ability bar (§5.5), floating combat text
(§5.6), the Tab quick-item wheel (§5.7), the prompt and death behaviour (§5.8), the minimap
(§5.1), the Esc button row (§7) and the chat box (§9.1). The contents and behaviour inside the item windows (inventory,
equipment, crafting, skills, character creation) are listed with their entry points and
templates but not traced in depth; they are the follow-ups of §12.

Routines are cited by address in `Cube.exe`. "G" is the game object; "W" and "H" are the window
client width and height in pixels (G+0x11c, G+0x120). Game logic behind the controls (what
jumping or dodging does) belongs to the player-control specification, not here.

---

## 0. Text in the interface

* **All interface text is English and hard-coded** in the executable (UTF-16 literals). The
  dictionaries in `data4.db` (`dict_en.xml`, `dict_de.xml`) hold only creature names
  (`<name>`), landscape name patterns (`<landscape>`) and long texts (`<speech>`: skill and
  specialization descriptions, quest objectives and dialogue, with the markup
  `[text $style]`, `%` and `@variable`). The `language` option selects the dictionary; it does
  not translate menus.
* Text drawn by code (not from a `.plx`) uses `resource1.dat`.
* Quoted strings below are exact, including spaces.

---

## 1. Interface roots and files

Created at start-up (0x459c40):

| Root | Holds |
|---|---|
| start-screen root (G+0x800874) | `start.plx`; the start-menu, character, world and connect screens (four screen containers G+0x800888 … G+0x800894, hidden until used) |
| in-game root (G+0x800884) | `gui.plx` and `quest-tag.plx` templates and every HUD clone |
| help container (G+0x80089c) | `help.plx` (hidden; F1) |

Templates looked up by name at start-up and kept for cloning (0x459c40): `questtag`,
`smallquesttag` (quest-tag.plx); `cubeworld`, `picroma` (start.plx); `star1`–`star4`,
`landname`, `blackwidget`, `itembox`, `wideitembox`, `itemselector`, `combopoints`,
`leftbutton`, `rightbutton`, `upbutton`, `downbutton`, `scrollbutton`, `edit`, `button`,
`button2`, `smallbutton`, `preview`, `equippedpreview`, `crosshair`, `zoomcrosshair`,
`lockedenemy`, `lockedfriend`, `abilitybutton`, `skillbutton`, `specializationbutton`,
`childskillbutton`, `quickitembutton`, `menubutton`, `equipmentboxleft`, `equipmentboxright`,
`experiencebar`, `ridingbar`, `manacubebar`, `questbar`, `lifebar`, `mpbar`, `chargebar`,
`enemylifebar`, `friendlifebar`, `staticlifebar`, `neutrallifebar` and their `:small`
variants, `hpbar`, `castbar`, `staminabar`, `selector`, `speech`, `craftbutton`, `craftbar`,
`itemshadow`, `itemframe`, `rightarrow`. Inside clones the game looks up the child names
`text`, `info`, `name`, `count`, `class`, `bar`, `bar2`, `background`, `frame`, `cooldown`,
`rarename`, `objective`, `detail`, `show`, `shine`, `manacubes`.

---

## 2. Layout model

* The interface is laid out in **window pixels at 1:1**. There is no interface scale: a
  1920 × 1080 window shows the same-size elements as 1280 × 720, with more space between them.
* Each placed node is positioned by writing its translation so that its **pivot** lands on an
  anchor point computed from W and H. This is redone whenever the window size changes
  (GameController layout routine 0x482a40).
* Windows (inventory, equipment, …) are centred, or placed to the left/right of the centred
  window with a 10-pixel gap (below).

Anchors (0x482a40; slots are G offsets; "text label" = a text node made in code, font
`resource1.dat` 12 px, outline 3):

| Element | Anchor (x, y) |
|---|---|
| title logo `cubeworld` (start screen) | (W/2, H/2) |
| `picroma` logo (start screen) | (W − 20, H − 20) |
| quest banner `questtag` (G+0x800868) | (W/2, H − 300) |
| small quest tag `smallquesttag` (G+0x80086c) | (W − 200, 400) |
| combo counter `combopoints` (G+0x800ad0) | (W/2, H − 190) |
| land-name banner `landname` (G+0x800758) | (W/2, 230) |
| speech/dialogue widget (G+0x80093c, 400 × 400) | x = W/2 − width/2, y = 300 |
| objectives widget (G+0x800944) | (20, 170) |
| statistics widget (G+0x80094c) | (20, 470) |
| help picture (G+0x80089c) | (W/2 − 430, 50) |
| "Please wait…"-type label (G+0x800898) | (W/2, H/2) |
| label G+0x8009f4 | (W/2, H − 200) |
| red error label `connectionError` (G+0x8009f8, colour (1, 0.25, 0.25)) | (W/2, H/2 + 100) |
| character-name edit (G+0x800974) | (W/2, H − 50) |
| world-name edit (G+0x8009bc) | (W/2, H − 50) |
| world-seed edit (G+0x8009b8) | (W/2, H − 100) |
| server-address edit (G+0x8009ac) | (W/2, H/2 − 100) |
| button row A = the **ability bar** (list at G+0x8007fc, *n* = 7 buttons, §5.5) | button *i*: x = W/2 + (2*i* − *n*)·24 + 6 = W/2 − 162 + 48*i*, y = H − 50 (48 px pitch) |
| button row B = the six **menu buttons** (list at G+0x800838, *n* = 6, §7) | button *i*: x = W/2 + (2*i* − *n* + 1)·30 = W/2 − 150 + 60*i*, y = H − 140 (60 px pitch) |
| chat box (ChatWidget G+0x800a14, 400 × 200) | top-left at (20, H − 220) (§9) |
| crosshair `crosshair` / `zoomcrosshair` (G+0x8008ac / G+0x8008b0) | pivot at (W/2, H/2), every frame (0x4927da, 0x4928ae) |
| quick-item wheel label `selector` (G+0x8008e0) | pivot at (W/2, H/2 − 200), every frame (0x492727) |
| start menu (G+0x800918, 200 × 100) | top-left at x = (W − 200)/2, y = H/2 + 50 (§3.3) |
| system panel (G+0x800910, 150 × 120) | top-left at x = (W − 150)/2, y = (H − 120)/2 (§3.3) |
| two-column list (G+0x800a34) | entry *i*: x = W/2 − 200 + (*i* mod 2)·400, y = H/2 + ⌊*i*/2⌋·56 + 10 |
| centred menu buttons (list ending at G+0x80097c) | x = W/2 − width/2 |
| inventory window (G+0x8008bc) | x = W/2 − width/2, y = H/2 − height |
| windows G+0x8008c0, G+0x800908, G+0x8008dc | left of centre: x = W/2 − w₀/2 − width − 10, y = H/2 − height (w₀ = the centred window's width) |
| window G+0x800ad4 | as above, y lowered by its height + 10 |
| windows G+0x8008c4, G+0x8008f8, G+0x800900 | right of centre: x = W/2 + w₀/2 + 10, y = H/2 − height |
| window G+0x8008dc (when the 3D preview is active) | (screen W/2 − 340, H/2 − 250) |

Row B is the menu-button row (it is the list the Esc menu iterates, §7). Row A is the ability
bar (settled, §5.5): six `abilitybutton` clones followed by the `quickitembutton` template
itself (0x460232 … 0x46050b).

### 2.1 The pivot rule (how "anchor" is meant, and the node transform)

Every anchor in this file is the screen point where the node's **pivot** lands. The game does
this by writing translation = anchor − pivot (0x482a40, and per frame in 0x488ee0 with the
same subtraction, 0x468df0). This only works because the library composes a node's matrix as
(0x678600):

**x′ = t + p + R · D · (x − p)**

(translate by translation *t*, translate by pivot *p*, rotate about x, y then z by the
`rotation` angles, apply the `deformation` matrix *D*, translate by −*p*). So with
t = A − p the pivot lands on A, and a shape point *x* lands at A + R·D·(x − p). For identity
R and D a shape point simply moves by *t* (x′ = x + t) — which is why `lifebar`'s mesh plus its
file translation reproduces its widget rectangle exactly. **This corrects
`alpha-ui-plasma.md` §4.1**, which writes x′ = t + R·D·(x − p) (the + p is missing) and leaves
P-7 open; the order is translation, pivot, Rx, Ry, Rz, deformation, minus pivot.

Consequences used below:

* **Bars fill by scaling the deformation about the pivot**: the game resets the `bar` node's
  deformation to identity and multiplies its x row by the fill fraction *f* (0x423e70 then
  0x424730 with (f, 1, 1)). A bar point *x* is drawn at pivot + f·(x − p) horizontally. Every
  bar's pivot is on (or within 5 px of) its left edge, so the bar shrinks towards the left.
* Children are placed in their parent's space by the same rule; for a child with identity
  R and D its shape points are at shape + child *t* in the parent's space, and a text child's
  origin (first baseline, §3.4) is at the child's *t*.

Worked screen rectangles in §5 are computed with this rule from the `gui.plx` values; they are
test vectors for the renderer.

---

## 3. Start screen

### 3.1 Logos

`start.plx`'s `cubeworld` (the title, a vector logo with a masked shine group) is centred on the
screen and `picroma` sits in the bottom-right corner (§2). Their timing and the background
are in §3.3.

### 3.2 Start menu (StartMenuWidget, drawn in code, 0x583320)

Three text items, centred horizontally in the widget (x = widget width / 2), font
`resource1.dat` 18 px:

| Item | Baseline y | Hover band (mouse y) | Action |
|---|---:|---|---|
| "Start Game" | 30 | [10, 40) | to the character screen |
| "Options" | 80 | [60, 90) | open the options window (§6) |
| "Exit" | 130 | [110, 140) | quit |

Each item is drawn twice: first with a 4-pixel outline in black, then the fill. The fill is
white (1, 1, 1, 1), or **(0.2, 1, 1, 1)** (cyan) while the mouse is inside the item's band and
within the widget's width. A click on the hovered item triggers it (index kept at widget
+0x160). Menu clicks play sound 0x55 (`menu-select.wav`, §10).

### 3.3 Placement, backgrounds and timing of the start menu and system panel

**Start menu.** Its host node (G+0x800918) is an empty container made at start-up under the
start-screen root and sized **200 × 100** (0x459c40, right after the StartMenuWidget
constructor 0x583270). The layout routine 0x482a40 places its top-left corner at
**x = (W − 200)/2, y = H/2 + 50**, so the menu sits below the centred title logo. The item
baselines of §3.2 (30, 80, 130) and hover bands are measured from that corner. It has **no
background panel**: it is only the outlined text.

**System panel.** The panel toggled by **O** and by the "system" menu button (G+0x800910) is a
clone of the `blackwidget` template, resized with the 9-slice rule to **150 × 120**. The
SystemWidget (0x587660) is built on a container inside it. The layout routine centres it:
**x = (W − 150)/2, y = (H − 120)/2**. Its look comes from `blackwidget`:

* a 12-corner rectangle with 5-pixel notched corners;
* filled with black at alpha 0.67;
* a 3-pixel black stroke with alignment 1;
* over it a second outline of the same shape: no fill, 2-pixel white stroke with
  alignment −1. Its last three vertex colours are (0.51, 0, 1, 0.67), but that shape has no
  fill, so they do not show.

The three items of §7 are drawn on this panel.

**What is behind the start screen (S-1).** There is no separate start-screen render path.
Every frame begins by clearing colour and depth to **(0, 0.44, 1)** (D3D colour 0xFF0070FF;
white instead if G+0x8006e6 is set), at 0x4ac345. Then the normal world frame is drawn
(`alpha-rendering.md` §1.3): sky gradient, sun, terrain of whatever world is loaded, from the
game camera. The interface is drawn on top. With no world loaded, this leaves the sky
gradient over the clear colour (to be confirmed by capture, S-1). The one exception is the
loading state: while the "Please wait…" label (G+0x800898) is visible, the frame hides every
interface root except that label, draws it, presents, and skips the world entirely. The
loading screen is therefore the plain (0, 0.44, 1) clear colour with the centred label
(0x4ac260, the block that tests G+0x800898).

**Logo animation and timing.**

* `start.plx` contains **no sequences**, and no code other than layout and visibility touches
  the logo nodes. The logos are static; the masked `Group` (clipping, display flags 4) only
  cuts the lettering.
* Each update (0x488ee0, at its start), while the start-screen root is visible:
  * the options window's node (G+0x800a00) is moved under the start-screen root;
  * the `cubeworld` logo and the start menu are both made visible exactly when the options
    window is hidden, and both hidden while it is open.
* While the start-screen root is hidden, the options node is moved under the in-game root.
* The logos and the start menu therefore appear together, on the same frame, with **no delay
  and no fade**. `picroma` simply follows the start-screen root's visibility.

### 3.4 Where a text's origin sits (all code-drawn text and TextShapes)

Both code-drawn text (start and system menus, labels) and TextShapes go through the same
font layout (draw 0x65bc70, layout 0x65c040, measuring 0x65d530):

* **The origin is the first line's baseline.** The measuring routine starts every line's box
  at y from −size to 0 relative to the pen, and x from 0. Size is the font's line size, i.e.
  the pixel size. The text therefore rises above the origin, and the origin is not the top of
  the text box. The start-menu baselines of §3.2 (30, 80, 130 below the menu's top-left
  corner) are baselines in this sense.
* **Further lines** start lower by size + `lineSpacing` at each line break (character 10).
  With flag bit 4 (16) the string is first word-wrapped to the wrap width (0x660d50).
* **Horizontal alignment** is applied per line:
  * bit 0 (1): the line is shifted left by half its measured width (centred on the origin);
  * bit 1 (2): shifted left by its full width (right-aligned);
  * neither bit: left-aligned at the origin.
* **Vertical alignment:** the start point is moved by the difference between the block's box
  measured with the shape's flags and measured with no flags. This is how bit 2 (4) moves the
  block. Bit 2 is believed to centre the block vertically on the origin; this is not proven
  (`alpha-ui-plasma.md` P-9).
* **Pixel snapping:** for unscaled text the pen position is snapped to whole pixels
  (rounded half up).

### 3.5 Character and world screens

Built at start-up (0x459c40, lines after `start.plx`). Buttons are clones of `button` resized
with the 9-slice rule (`alpha-ui-plasma.md` §5.5); captions are the button's `text` node.

| Button caption | Size | On click |
|---|---|---|
| "Create Character" | 300 × 40 | 0x4821a0: create the character from the name edit; "There is already a character with that name." if taken |
| "Create World" | 300 × 40 | 0x482530: create from world name + seed; "There is already a world with that name." if taken |
| "Back" | 200 × 40 | 0x4814f0: previous screen |
| "Multiplayer Worlds..." | 300 × 40 (row element 40 × 40) | 0x484230: switch the world list to multiplayer |
| "Connect to server" | 300 × 40 | 0x481fe0: open the connect dialog |
| "Connect" | 200 × 40 | 0x4815e0: connect to the address in the edit |

Edits (clones of `edit`, each with a 12-px label above it, white with a black 3-px outline):

| Label | Menu key (`alpha-net-transport.md` §1.2) |
|---|---|
| "Name" | `charactername` |
| "Name" | `worldName` |
| "Seed (must be a number)" | `worldSeed` |
| "Server address" | `serverName` |

Other strings of these screens: "New character", "Select", "Delete Character",
"Delete World", "Create", "Singleplayer worlds...", "Multiplayer worlds...", "Please wait...",
"Error", and the connection messages "Connected.", "Connection error.", "Connection error",
"Error: could not connect to server.", "Error: Server has different version (" *x* ") than
client (" *y* ")", "Error: Server is full.", "Disconnected.". Character entries show
"LVL " + level and the class.

### 3.6 Character creation (CharacterStyleWidget 0x427ce0, CharacterPreviewWidget)

Rows of `leftbutton` / `rightbutton` arrows around the turning 3D preview (lit with front
(1, 1, 1), back (0.2, 0.3, 0.4), ambient (0.4, 0.4, 0.4), `alpha-rendering.md` §11.5), with
the labels and choices:

* "Race": "Human", "Elf", "Dwarf", "Orc", "Goblin", "Lizard", "Undead", "Frogman"
* "Gender"
* "Class": "Warrior", "Ranger", "Mage", "Rogue"
* "Face " *n* / "Face", "Haircut " *n* / "Haircut", "Hair color"

Exact row positions and the gender captions: F-1.

---

## 4. Multiplayer connect dialog

The "Connect to server" button shows the server-address edit (anchor §2, label "Server
address", key `serverName`) and the "Connect" button. Connecting prints the status strings of
§3.5 into the red error label / info label. The chat command `/connect <address>` does the same
in game (§9). Network behaviour: `alpha-net-transport.md`.

---

## 5. In-game HUD

The player's own bars are the `gui.plx` template nodes themselves (looked up, not cloned:
`lifebar` G+0x800768, `experiencebar` G+0x8007c4, `mpbar` G+0x8007b8, `hpbar` G+0x8007e4,
`castbar` G+0x8007ec, `staminabar` G+0x8007f4, `ridingbar` G+0x8007d4, `manacubebar`
G+0x8007d8; 0x460e87 … 0x461497). Clones are made only for the pet and party bars and the
creature bars (§5.3, §5.4). Bars fill by **scaling** the `bar` child about its pivot (§2.1,
§5.3); `bar2` is the empty remainder on creature bars. Their numbers go into every `text` node
as "*current*/*max*" (the template's placeholder is `20000/2000o`).

| Template | What it shows | Text written | Notes |
|---|---|---|---|
| `lifebar` (widget) | player health | `text` = HP numbers; `info` = "HP"; `class` = "LVL " + level + class name | red gradient `bar` (1, 0.21, 0)→(0.87, 0, 0.32) |
| `mpbar` | mana | `info` "MP", `text` | `chargebar` child = charge |
| `hpbar` | health (alternate) | `info` "HP", `text` | |
| `experiencebar` (widget) | XP | `info` "XP", `text` | |
| `staminabar` | stamina (climbing/swimming) | `name` = "Stamina" | |
| `castbar` | channel/cast progress | `name` = ability name | |
| `ridingbar` | pet riding | five `icon` children | |
| `manacubebar` | mana cubes | four `icon` children | |
| `questbar` | quest progress | | inside `smallquesttag` |
| `enemylifebar`, `friendlifebar`, `staticlifebar`, `neutrallifebar` and `:small` forms | target health by attitude | `info` = name ("ENEMY"), `rarename` = rarity title, `text` | `star1`–`star4` children on `_enemylifebar` show rarity |
| `crosshair` / `zoomcrosshair` | aim marks + **interaction prompt** | `crosshair`'s `name` child = "[R] Talk", "[R] Revive", … (§5.8) | four 7 × 7 white squares at ±9 px from the centre; zoom form has longer 20 × 4 bars and a fifth mark at +13 |
| `lockedenemy` / `lockedfriend` | lock-on marker | — | |
| `selector` | label of the **quick-item wheel** (Tab, §5.7) | `name` = item name (+ " +" level), `count` = stack size | not the interaction prompt |
| `abilitybutton` | an ability slot | `info` = key label "M1", "M2", "1"–"4"; `cooldown` overlay | §5.5 |
| `quickitembutton` | quick item slot (7th button of the ability bar) | `info` = "Q", `count` | §5.5 |
| `equipmentboxleft` / `equipmentboxright` | the two weapons | `info` = "WEAPON 1", `count` | |
| `menubutton` | buttons that open windows | `count` = key label ("F1", "x", "C", "B", "M", "O") | icon is the button's own texture; §7 |
| `combopoints` | hit combo | `Text` = *n* + " HIT"/" HITS" | plays `hit`, `button:enter` sequences |
| `questtag` | "QUEST COMPLETE!" banner | `objective` | sequence `shine`; sound 0x1e |
| `smallquesttag` | current quest | `objective`, `questbar` | sequences `show`, `show2`, `shine` |
| `landname` | entering a region | `name` = land name, `detail` | font `resource2.dat` 69 px / 40 px |
| `speech` | NPC speech bubble | | SpeechWidget |

Messages written to the chat/info area: "You receive " *item*, *name* + " receives ",
"You receive a mana cube.", "You gain one skill point.", "LEVEL UP!" (sound 0x1d),
"You gain " *n* " XP.", "You can't carry more of these items.", "Pet name too long",
"There is nothing special.". Interaction prompts: "[R] Revive", "[R] Talk", "[R] Ride",
"[R] Open", "[R] Close", "[R] Sit Down", "[R] Sleep", "[R] Teleport", "[R] Use Furnace",
"[R] Use Anvil", "[R] Use Spinning Wheel", "[R] Use Loom", "[R] Use Saw", "[R] Use Workbench",
"[R] Use Customization Bench", "[R] Examine".

### 5.1 Minimap (the "corner map", drawn by the frame, 0x4baa84 … 0x4babaf → 0x5fc1b0)

0x5fc1b0 is the **map renderer**: the same routine draws the world map (§5.2, called at
0x4ae004 with a centred projection, radius 16 and its map-only extras switched on) and the
minimap. This section describes the minimap call. The routine's object is G+0x800d44 (called
"the map object" below).

**When.** Every in-game frame, after the world and before the interface. The caller first
binds the world pixel shader, sets white = 0, alpha = 1 and material (1, 1, 1, 1)
(0x4ba9fc … 0x4baa62) and **clears the depth buffer to 1.0** (device Clear, flags = depth only,
colour and stencil untouched; 0x4baa72 … 0x4baa93). It then draws the minimap only if the
in-game root (G+0x800884) is visible and the HUD node returned by 0x487490 is visible
(0x4baa95 … 0x4baabc). The minimap is not a separate render target and has no background,
frame or mask: it is a small 3D model drawn straight into the frame.

**Arguments of the minimap call** (0x4baac2 … 0x4babaf): pivot = the local player's position
([G+0x8006d0] + 0x10, 64-bit fixed point); projection centre x₀ = W − 240, y₀ = H − 170;
window W, H; rotation (−120°, 0, −ψ) where ψ is the camera yaw G+0x1ac (element 2 of the
pitch/roll/yaw triple at G+0x1a4, `alpha-physics-camera.md`); the view scale *s* and radius
*r* of the zoom table below; the frame time step G+0x8006e8 (ms, *inferred*); map extras off.

*Correction (0x4bab62 … 0x4bab85, 0x428970):* the yaw is G+0x1ac, not G+0x1a4 (that is the
camera pitch). *Correction (0x4baac2):* the zoom input G+0x1c4 is the **map zoom** (the
current value of the zoom the world map uses, initial 1), not a camera distance.

#### 5.1.1 Units

*Correction (0x5fcfd3 … 0x5fd4d3, 0x5fe2f5 … 0x5fe98a; tile format `alpha-saves.md` §6.4):*
the miniature is not built in blocks. Its model space is **map units of 8 blocks**, centred on
the player's exact position:

* **Zone tiles.** Each known zone has a small voxel model (its `tile`, 32 × 32 × 8–12 voxels,
  one voxel = 8 × 8 × 8 blocks), already meshed into the normal 8-byte world vertex format, so
  tile vertex positions are 0..32 map units across and their z counts 8-block steps above the
  tile's base height. A zone is 32 map units wide.
* **Land pictures.** At the farthest zoom the routine draws the player's **land** (the named
  region the player stands in) as its map picture: a flat voxel picture of about 95 × 98 × 1
  voxels with **one voxel per zone** (`land` record, `alpha-saves.md` §6.2). Its world
  matrix scales it by 32, so it lands in the same 8-block map units.
* The view then multiplies everything by the view scale *s*. A 3 × 3-zone minimap is
  96 map units × 0.8 = 77 view units wide, which at the view distance of 300 and a 45° lens is
  roughly 220–310 px wide in a 1280 × 720 window (worked example below), not 614 units.

#### 5.1.2 Zoom and content

The zoom table (0x4baac2 … 0x4bab5b):

| Map zoom *d* (G+0x1c4) | View scale *s* | Radius *r* | Terrain drawn |
|---|---|---|---|
| *d* < 0.35 | 0.025 | 0 | the player's land picture (one slab) |
| 0.35 ≤ *d* < 2 | 0.8 | 1 | the zone tiles of the 3 × 3 zones around the player's zone |
| *d* ≥ 2 | 1.6 | 0 | 2 × 2 zone tiles: the player's zone plus the neighbour towards the nearer edge in x and in y (a neighbour on the −side when the player is in the first 128 blocks of the zone, else on the +side) |

The routine picks the land path when *s* < 0.35 and the zone path otherwise (0x5fce30).

**Land path** (*s* < 0.35; 0x5fce30 … 0x5fd663). The land cell is the region cell
(`alpha-worldgen-regions.md`) whose noise-displaced point is nearest the player (0x5cade0,
falling back to block ÷ 16384). For each land cell (lx, ly) within ±*r* (only the player's own
at *r* = 0) whose map record (map object +0x4000b0, 1024 × 1024 pointers, index lx·1024 + ly)
exists, has a mesh (+0x2c) and is ready (byte +0x28):

* World matrix (row-vector form, *v*·*M*): translate by (dx, dy, −1) in picture voxels, then
  scale by 32, where (dx, dy) = (land origin − player) in zones (land origin = record +0x20,
  +0x24 in zones; quantised to 1/65536 zone). If the record's int at +0x1c is negative the
  slab is lowered by a further half voxel (16 map units); otherwise its height is multiplied by
  (1 + value/256) (the top moves, the bottom stays at −32). The land is therefore a slab from
  −32 to 0 map units around the pivot's level; the player's height is **not** used.
* Material (the "tint"): (0.2, 0.4, 1, 1) × *k* per colour channel, each clamped to 1, alpha 1,
  with *k* = 0.8 + 0.1·((lx + 2·ly) mod 4) (so 0.8, 0.9, 1.0 or 1.1; neighbouring lands get
  different shades of blue). It **multiplies the vertex colours** in the vertex shader like any
  material colour (`alpha-rendering.md` §5 step 9). Reset to (1, 1, 1, 1) after the draw.

**Zone path** (*s* ≥ 0.35; 0x5fd663 … 0x5fee50). The player's zone is (x ÷ 256, y ÷ 256) in
blocks. For each zone offset (i, j) in the range above (x outer, y inner), skipping zones
outside 0..65535 or whose map region record (map object +0xb0, index by zone ÷ 64) is missing,
the zone's map record is region record + ((zx mod 64)·64 + (zy mod 64))·0x34. If it has a
tile mesh (+8):

* **Rise-in.** h = base height (+4, map units) − 0.1 × the record's timer (+0x2c); then the
  timer −= the frame time step, floored at 0 (0x5fe2f5 … 0x5fe35a). A newly loaded tile thus
  rises into place at 0.1 map unit per millisecond (the timer's start value was not traced).
* **World matrix**: identity with translation
  t = ((player-zone origin − player)ₓᵧ ÷ 8 + 32·(i, j),  h − player z ÷ 8) map units
  (0x5fe362 … 0x5fe8a1). No rotation, no scale (a ×4 scale branch at 0x5fe8aa is unreachable:
  it requires a missing mesh).
* Material (1, 1, 1, 1) (0x5fe328); reset to (1, 1, 1, 1) after the draw. The 0.6 dimming at
  0x5fe9f8 applies only with map extras on (world map).
* Then, whether or not the zone had a tile, if the map object has its **marker tile** (+0x8000b0: a 4 × 4 × 1 voxel model, white
  2 × 2 centre with a (10, 10, 10) border, built by 0x5fae40) and the zone record's list at
  +0x20 is non-empty, each list entry is drawn as that marker: world matrix = scale 5 then
  translate by (entry block position +8/+0xc/+0x10 − player block position); material
  (1, 1, 1, 1); view = the stored block view (map object +0x44, see "State left behind"),
  projection = the biased marker projection (§5.1.3). What the list entries are was not
  traced.

Zones without a tile (never seen by the player) draw nothing. Nothing else of the world is
drawn: **no water pass** (water and plants exist only as colours inside the tile and land
pictures), no statics, no clouds, no sky.

**Creature markers** (both paths, after the terrain; 0x5ffeea … 0x601c78). The creatures of
the world list ([map object +0xac] + 4) are collected when

* controller kind (obj +0x60) is 0 (players), or
* controller kind is 1 and appearance flag 0x2000 (obj +0x7e; the mission-target flag) is set,
  or
* *s* ≥ 1.6 and the class (obj +0x140) is 0x80–0x87 (the non-combat town NPCs),

and both |Δx| and |Δy| to the player are ≤ max(*r*, 1)·256 blocks. They are sorted by their
projected depth (0x5fa9e0) and drawn each in two passes, placed at the creature's displayed
position (obj +0x1350) relative to the player, in blocks:

| Creature | Model(s) (`alpha-creatures-models.md` Appendix A) | Turned about z by |
|---|---|---|
| kind 1 | 2562 `skull` | the creature's body yaw (obj +0x1374, element 2) |
| kind 3, class 0x80 / 81 / 82 / 83 / 84 / 85 / 86 / 87 | 2452 `shop-sign` / 2453 `weapon-shop-sign` / 2454 `armor-shop-sign` / 2455 `identifier-sign` / 2451 `inn-sign` / 2456 `smithy-sign` / 2457 `carpentersshop-sign` / 2458 `weavingmill-sign`, at half size | +ψ (so they always face the viewer) |
| any other (players) | the creature's head model then hair model (appearance +0x14, +0x16) | the creature's body yaw |

Each model is centred on its middle. **Pass 1** (outline): depth writes off (render state 14
= 0), material (0, 0, 0, 1), uniform scale 5/*s*. **Pass 2**: depth writes on (state 14 = 1),
uniform scale 4/*s*, material (1, 1, 1, 1), except the hair model, whose material is the hair
colour ÷ 255 with alpha 1. Dividing by *s* keeps the markers the same size on screen at every
zoom (5/8 and 1/2 view unit per voxel). View = the block view (map object +0x44),
projection = the biased marker projection.

#### 5.1.3 Matrix chain (0x5fc1b0 start, 0x5fc1f8 … 0x5fcc77)

Row vectors throughout (clip = *v*·World·View·Proj, as Direct3D 9; `alpha-rendering.md` §5 uses
the same matrices).

**Projection P** (0x5fc217 … 0x5fc6c0). With *y*ₛ = 1 / tan 22.5° (0x6fd098 = π/8, a double)
= 2.4142136, *x*ₛ = −*y*ₛ / (W/H) (**negated**: the same x mirror as the world camera,
0x4304a7, so the image is not mirrored), *o*ₓ = 2·(x₀ − W/2)/W, *o*ᵧ = 2·(y₀ − H/2)/H:

| | col 0 | col 1 | col 2 | col 3 |
|---|---|---|---|---|
| row 0 | *x*ₛ | 0 | 0 | 0 |
| row 1 | 0 | *y*ₛ | 0 | 0 |
| row 2 | *o*ₓ | *o*ᵧ | 0.9001 | 1 |
| row 3 | 0 | 0 | −0.10001 | 0 |

This is a left-handed perspective (near 0.1, far 1000.1: 1.0001 and −0.10001) followed by a
clip-space shift of (*o*ₓ, *o*ᵧ, −0.1): the centre of projection moves to (*o*ₓ, *o*ᵧ) in NDC
and every depth is pulled 0.1 nearer (so NDC z = 0.9001 − 0.10001/z: nothing is clipped by the
far plane).

*Correction (0x5fc26f … 0x5fc2e2):* NDC y points up, so *o*ᵧ = 1 − 340/H > 0 puts the centre
**170 px below the top edge**: the miniature is centred at pixel **(W − 240, 170)** in
top-left window coordinates, i.e. near the **top-right** corner. In 1280 × 720 the centre is
(1040, 170), not (1040, 550).

**View V** (0x5fc6fc … 0x5fcc77), built by left-multiplying onto the identity in this order:
translate (0, 0, 300) → uniform scale *s* on rows 0–2 (skipped when *s* = 1) → turn about z by
0 (a literal zero; the rotation's y element is never used) → turn about x by rot.x (−120°) →
turn about z by rot.z (−ψ). So a point *p* in map units goes through, in this order:

1. about z by θ = −ψ: x′ = x cos θ − y sin θ, y′ = x sin θ + y cos θ;
2. about x by φ = −120°: y″ = y′ cos φ − z sin φ, z″ = y′ sin φ + z cos φ;
3. × *s*;
4. z += 300.

*Correction (0x5fc865 … 0x5fc93e):* *s* is a **view** scale applied after the turns, not a
model scale. The camera forward (sin ψ, −cos ψ, 0) maps to straight up on screen and the
camera right to screen right; world up tilts to (0, 0.866, −0.5) in view space, so the
miniature is seen from 30° above its ground plane. 300 is the view depth along +z (the lens
looks down +z, w = z).

*Correction (0x5fc6cb … 0x5fc6f3, used at 0x5fedde, 0x601264 … 0x601c1e):* the constant 80
is **not** a vertical view offset. The routine copies the previous call's projection (map
object +4) and subtracts 80 from its row 3, column 2; this **marker projection** is used only
for the zone markers and creature markers, and pulls them 80 clip units nearer (NDC z −80/w,
about −0.27 at w = 300), so they draw over the terrain. (Being the previous call's matrix it
lags one call; an implementation should use the current P with the same −80.)

**World matrices**: as given per path in §5.1.2 (land: translate then ×32; zone tile:
translation t only; markers: block positions with the block view).

#### 5.1.4 Draw state

| State | Value | Where |
|---|---|---|
| Depth buffer | cleared to 1.0 just before | caller 0x4baa93 |
| Depth test | on (set by the routine), function less-or-equal (from the frame) | 0x5fcd43; `alpha-rendering.md` §1.3 |
| Depth write | terrain: on (as the world pass leaves it); creature markers: off in pass 1, on in pass 2; left on | 0x60098c, 0x6015a3 |
| Culling | clockwise, as the world pass leaves it | `alpha-rendering.md` §1.3 step 13 |
| Blending | source-alpha / inverse-source-alpha, as left; the shader writes alpha = the alpha constant (1), so the result is opaque | 0x4baa28 |
| Shaders | world vertex shader + **world pixel shader** (0x447d10) for everything; the water shader is never bound | 0x5fcd4f, 0x5ffed2 |
| Fog | effectively off: fogScale = 1/10⁶ for the terrain, 1/10⁹ for the creature markers | 0x5fcd62, 0x5ffee5 |
| Point lights | all ten cleared (radius 0) | 0x5fce1f |
| Terrain lights | **not set**: the world's terrain lights of this frame stay in force (front P4, back P5, ambient P6, sun direction; `alpha-rendering.md` §4), so the minimap darkens at night; material alpha is 1 (not S), and model-style tile vertices carry alpha 1 | 0x4b1a9c (last light upload before the call) |
| Creature-marker lights | front (1, 1, 1), back (0.2, 0.3, 0.4), ambient (0.4, 0.4, 0.4), direction normalize(0, 0.5, 1) | 0x600502 … 0x600633 |
| Darkness, white, shininess | (0, 0, 0, 1), 0, 0, as left by the frame and the caller | |
| Scissor | see below | |

*Correction (0x5fcd30, 0x5ffde5; closes M-1):* the routine calls SetScissorRect with
{left W − 320, top 320, right W − 20, bottom 20}, but nothing in `Cube.exe` ever turns the
scissor test on (Direct3D's default is off); the only scissor-test render state call (174) is
the routine's own **disable** at 0x5ffde5 after the terrain. The rectangle has no effect and
the miniature is not clipped. (Read as top 20, bottom 320 it would be a 300 × 300 box around
y = 170, but the miniature does not have to fit it: in the worked example it spans x ≈ 880 …
1190.)

**State left behind** (0x5ffdd5 … 0x5ffecc, 0x600318 … 0x6004fe): map object +4 = P;
+0x44 = V with rows 0–2 multiplied by 1/8 (a view that takes **block** offsets from the
player; used by the markers and by the map overlay 0x4c9680); +0x84, +0x88, +0x8c = a point
300 blocks from the player along the horizontal direction from the world centre (2²³, 2²³)
to the local player, relative to the pivot (for the overlay); +0x8000b8 += the frame time
step. Scissor test off, depth write on, fogScale 1/10⁹, the marker lights above.

*Note:* the lighting front (1, 1, 1), back (0.2, 0.3, 0.4), ambient (0.4, 0.4, 0.4), direction
(0.5, 0.4, −0.6) set **after** the minimap call (0x4babb4 … 0x4bac7d) lights the item models of
the quick-item wheel (§5.7), not the map.

**Per frame:** clear depth → pick *s*, *r* from the map zoom → build P and V → bind world
shaders, fog off, point lights off, depth test on → draw the land slab (blue-tinted) or the
zone tiles (white material) with their zone markers → disable scissor test → store P and V/8
→ set marker lights → draw the collected creature markers (black outline pass, then the
coloured pass).

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
renderer, and stored in the map object (G+0x800d44, "M" below). Integer division written
÷₀ truncates toward zero (the compiler's `(v + (v >> 31 & 7)) >> 3` idiom), floor_div floors.

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
  | +0x20, +0x24 | list head and count of the **zone markers** (12-byte entries) |
  | +0x28 | byte **dirty**: the zone was (re)generated since the tile was built |
  | +0x2c | int **rise-in timer**, ms |
  | +0x30 | byte flags: bit 0 = **discovered**, bit 1 = **a tile exists** (in memory or in `map_<name>.db`) |

* *Land record* (M+0x4000b0, 1024 × 1024 pointers, the same index as the world's region
  cells W+0x4000bc; 0x34 bytes): +0x04 … +0x1f = a copy of the region cell's 28 bytes (point
  x, point y, fire byte, moisture, warmth, cell id, **elevation at +0x1c**; regions spec §2.1),
  +0x20, +0x24 = picture origin (zone x, zone y), +0x28 = byte **visited**, +0x2c = the
  **land picture** voxel model, +0x30 = byte **rebuild requested** (not saved).

#### The map worker (0x469590)

A loop that sleeps 10 ms per pass and runs only while the map belongs to the open world
(map world name and id equal the world's, 0x46962f … 0x4696a0). Each pass:

1. **Centre zone** (c_x, c_y) = trunc(cur + pan ÷ 256) per axis, where cur is the client's
   current zone (two ints at G+0x2bc; *inferred*) and pan the world-map pan offset in blocks
   (floats G+0x1000e4c, G+0x1000e50, used by the overlay 0x4c9680) (0x4695dc … 0x46962c). Its region is
   (c ÷ 64) (floor).
2. **Region records** for the 7 × 7 regions within ±3 of the centre region (x outer):
   0x603230 creates any that is missing (below).
3. **Land pictures** for the same 7 × 7 indices: 0x6024d0 (below).
4. **Tiles**, up to 10 per pass: scan zones x from c_x − 10 to c_x + 9 (outer), y from
   c_y − 10 to c_y + 9 (inner); a zone **needs work** when its zone map record is missing, or
   when it has no tile or is dirty **and** either flag bit 1 is set (a saved tile can be
   loaded) or the zone is loaded in the world (W+0xbc region → +0x10018 zone pointer)
   (0x469a0c … 0x469ac7). Take the needy zone with the smallest (zx − c_x)² + (zy − c_y)²
   (strictly smaller wins, so the first in scan order on ties) and call 0x603a00 for it.
5. Every time more than 1000 ms have passed: **unload** (0x5fbed0): free tiles of zones more
   than 10 zones from the centre in x or y (0x6022d0; the tile is already in the database);
   save (`reg` record and the `discovered` counter, 0x602160) and free region records more
   than 8 regions away; save (0x6050b0) and free land records more than 8 away.

#### Region record creation (0x603230)

Allocate the region record; if `reg<rx>_<ry>` exists in `map_<name>.db`, read its 64 × 64
cells into each zone record (u8 → +0x30, u8 × 4 → +0x10 … +0x13, u32 → +0x14, u32 → +0x18,
u8 → +0x1c) and its 8 × 8 location copies (`alpha-saves.md` §6.3). Then refresh +0x10 … +0x1f
of every zone from the world region's per-zone table and the location copies from the world
(when that world region is present), and write the `reg` record back at once (0x605420).
Tile pointers start null: **tiles are not read here**; a zone whose bit 1 is set is picked up
by step 4 and loaded from its `tile` record on demand.

#### Discovery and dirtiness

* **Discovered** (bit 0 of +0x30; 0x5fc160). Every in-game frame (0x48cf65 … 0x48d033) the
  client marks the zones under the four points (player x ± 64, player y ± 64) blocks; i.e.
  every zone touched by a 128 × 128-block square centred on the player. Setting the bit
  for the first time adds 1 to the **discovered counter** M+0x8000bc (the `discovered` key).
  Other callers (0x4887aa; 0x48d204, for points of a list of map locations) mark further
  zones; not traced.
* **Dirty** (+0x28 = 1) is set by the zone loader right after a zone is generated or loaded
  into the world (0x46acf8, after 0x5e4850).
* **Consequence: tiles are not limited to discovered zones.** A tile is built for every zone
  that the world generates or loads while it lies in the worker's 20 × 20-zone window,
  discovered or not, and stays in `map_<name>.db` for good. The discovered bit only
  shades land pictures (below) and the world map's extras colouring (0x5fd949 …, §5.2). The
  minimap's zone path tests only the tile pointer (+0x08).

#### Building or loading one tile (0x603a00, zone (zx, zy))

1. Stop unless the world is ready (W+0xa4 ≠ 0), the zone is in 0 … 65535 and its map region
   record exists. Refresh +0x10 … +0x1f (0x4a6ad0) and the location copy for map cell
   (zx ÷₀ 8, zy ÷₀ 8) (0x487da0 → 0x5fb9f0).
2. Let Z = the world's zone object if loaded (0x434d10), else none. **Stop if a tile is in
   memory and (not dirty or Z is none).**
3. **Load** — only when no tile is in memory and `tile<zx>_<zy>` exists: read version (ignored),
   B → +0x04, X, Y, Z sizes; if all ≥ 1, read u32 *c* and, if *c* ≠ 0, *c* bytes of zlib, make a
   voxel model of X × Y × Z with those RGB bytes and mesh it (0x4e6a20, 0x4e75c0, 0x4e7870);
   set flag bit 1, +0x08 = the model, **timer +0x2c = 250**. Then read u32 *n* (always
   present), clear the marker list and append *n* entries of 3 × i32. If the zone is not dirty,
   done; otherwise continue with the build (needs Z).
4. **Build** (when Z is present; 0x603f32 … 0x604b91), from Z's 256 × 256 columns (terrain
   spec §1.2; per column: bottom b = +0x10, count n = +0x1c, **floor level F = +0x14**):
   * lo = max(0, min over all columns of (F − 1)); hi = max(0, max(F of column (0, 0),
     max over all columns of (b + n))). **B = lo ÷ 8**, tile size **32 × 32 × ((hi − lo) ÷ 8 + 1)**
     (0x603f6a … 0x603fe7). The Z size is the zone's relief in 8-block layers: zones of
     56 … 88 blocks between their lowest floor and highest stored block give 8 … 12.
   * Clear dirty (+0x28 = 0), clear the marker list, allocate the model (all voxels (0, 0, 0)
     = empty).
   * For each voxel column (vx, vy), vx outer 0 … 31, vy inner 0 … 31, covering block columns
     x = 256·zx + 8·vx + sx, y = 256·zy + 8·vy + sy (sx, sy = 0 … 7):

     **(a) Surface colour and height** (0x6040e0 … 0x6042f5). For the 64 columns (sx outer,
     sy inner), with f32 sums starting at 0:
     * if b + n ≤ 0 (nothing stored above sea level): add the **water colour (80, 100, 255)**,
       height 0;
     * else scan the stored blocks from the top (z = b + n − 1) down: the first block of
       non-zero type gives its colour — **(80, 100, 255) for types 2 and 3** (water, lake
       bed), else its own RGB bytes (type 1 included) — and height z; if instead an empty
       stored block is met at z = F − 1, the colour is Rk(x, y, F) (terrain spec §6.6) and
       height F − 1; if the scan runs out, colour Rk(x, y, F) and height b − 1;
     * also track m = min F over the 64 columns.

     Surface colour S = sum × (1/64) per channel; mean height h̄ = height sum × 1/64 (f32);
     **top layer T = (trunc(h̄) ÷₀ 8) − B** (written as −B − trunc(−h̄)÷₀8 at 0x6042fb …
     0x604335; same value). If 0 ≤ T < Z size, voxel (vx, vy, T) = trunc(S) per byte.
     (Four 16 × 16 quadrant sums of S are also accumulated at 0x604650 and never read.)

     **(b) Body below the top** (0x6046a3 … 0x604b5f). For each layer k from
     max(0, (m ÷₀ 8) − B − 1) up to T − 1: average the 8 × 8 × 8 blocks at
     z = 8·(B + k) + sz (sx outer, sy, sz inner), read through the world's zone grid like the
     mesher reads blocks (terrain spec §1.4: no column or z < b → **type 1 grey**; stored
     block; empty stored block at z ≤ 0 without bit 0x40 → water; above the stored range →
     water if z ≤ 0, else air). Skip type 0. Count the rest (*c*) and add: type 1 →
     **Rk(x, y, z)**; types 2, 3 → (80, 100, 255); others → their RGB. If *c* > 0, voxel
     (vx, vy, k) = trunc(sum × (1/*c*)) per byte, else it stays empty. **There is no fill
     threshold**: one solid block makes a full voxel. Layers above T are never filled, so
     the tile is a height field (overhangs, arches and anything above the mean surface are
     lost); trees and other statics are not blocks and do not appear; water shows only as
     its flat blue colour.

     **(c) Zone markers** (0x604371 … 0x60449f), only when vx mod 4 = 0 and vy mod 4 = 0 (an
     8 × 8 grid, 32 blocks apart): with x₀ = 256·zx + 8·vx, y₀ = 256·zy + 8·vy and
     owner = the world's region-cell owner (0x477e10, regions spec §3.4), if
     owner(x₀, y₀) ≠ owner(x₀ + 32, y₀) or owner(x₀, y₀) ≠ owner(x₀, y₀ + 32), append the
     entry (x₀, y₀, max(F of column (8vx, 8vy), 0)), absolute block coordinates. **The markers
     trace land borders**: the minimap draws each as the white 4 × 4 marker tile scaled by 5
     (§5.1.2), i.e. a dotted line every 32 blocks along the border between two lands. A zone
     inside one land has none (all 105 sampled `tile` records end in 0).
   * Mesh the model (0x4e7870), then write `tile<zx>_<zy>`: u32 version (M+0x8000f8), i32 B,
     3 × i32 size, u32 *c* + zlib bytes (when all sizes > 0), **u32 marker count + 12 bytes
     per marker**. Under the lock: if there was no tile in memory, **timer +0x2c = 250**;
     otherwise delete the old model (the timer is left alone, so a rebuild does not rise
     again); set flag bit 1; +0x08 = the new model. Finally write the `discovered` counter.

*Correction to `alpha-saves.md` §6.4:* the second field is the base height B (map units); the
last u32 is the marker count, followed by that many (x, y, z) i32 triples.

**Rise-in.** With the minimap's h = B − 0.1 × timer and the timer counting down by the frame
time (§5.1.2), a newly loaded or newly built tile starts **25 map units (200 blocks) below**
its place and rises into it in 250 ms.

**Meshing and light.** The tile is an ordinary voxel model (constructor 0x4e6a20: shade colour
(0, 0, 0); key-colour flag byte +0x55 = 1 from the word 0x100 at +0x54, so pure red, green and
blue voxels draw normally) meshed by 0x4e7870 exactly as `.cub` models (`alpha-rendering.md`
§11.1): faces only toward empty (0, 0, 0) neighbours, each vertex darkened toward black by the
occlusion factor, vertex alpha 1. **No light is baked**; the frame's terrain lights shade it
when drawn (§5.1.4). Positions are in voxel units 0 … 32 across, z from 0 at layer B.

#### Land pictures (0x6024d0, land cell (lx, ly))

1. Stop unless the world is ready and 0 ≤ lx, ly < 1024. If no land record is in memory and
   `land<lx>_<ly>` exists, create the record from it (version, origin +0x20/+0x24, visited
   +0x28, picture, then the 28 cell bytes into +0x04 … +0x1f) and continue to step 2. If a
   record is in memory and its **rebuild flag +0x30 is 0, stop**.
2. Build only when the world has region cell (lx, ly) **and all 8 neighbours** (else stop).
   First refresh the zone records' +0x10 … +0x1f and location copies of map region (lx, ly)
   if it is loaded, create the land record if missing, and copy the world cell's 28 bytes
   to +0x04 … +0x1f (so **+0x1c = the cell's elevation**).
3. **Ownership of a zone** (zx, zy) (0x602b35 … 0x602c93):
   * **Point.** x = 256·zx, y = 256·zy (the zone's corner block, not its centre). With
     D = D(x, y) from 0x5eefa0 (regions spec §3.1; D.x is driven by y, D.y by x), the point is
     p = (trunc(f32((float)x + D.x)), trunc(f32((float)y + D.y))), whole blocks, exactly the
     displaced point of regions spec §3.3. It is passed on as 64-bit p·65536 (no fraction).
   * **Distance** to a cell c (0x5eeee0, arguments: cell pointer, p.x·65536, p.y·65536):
     the regions spec §3.3 value before truncation, i.e. ex = f32((float)(c.x·65536 −
     p.x·65536) · 2⁻¹⁶), ey likewise, d² = f32(f32(ey²) + f32(ex²)), kept as a **float**.
     c.x, c.y are the cell record's +0x00/+0x04 (whole blocks).
   * **d₀** = d² to the land's own cell (the world cell at (lx, ly), the one copied in
     step 2), then **truncated to an integer** (and back to float for the comparison).
   * **Neighbours.** All 8 cells around (lx, ly) compete, each read straight from the
     world's region-cell grid (W+0x4000bc, index (lx + i)·1024 + (ly + j)): outer loop
     i = −1, 0, 1, inner loop j = −1, 0, 1, skipping only (0, 0). For each one,
     d²(neighbour) is computed as above (float, not truncated); if
     **d²(neighbour) < d₀** (strict; 0x602c77) the zone is **not owned** and the loops stop.
     If all 8 are passed the zone is owned. Equal values belong to both lands.
     The 4 side and 4 diagonal cells are treated identically; no cell is compared through
     anything but this index, and there is no early accept.
   * **Missing neighbour in pass 1:** an index out of 0 … 1023 or a null grid entry makes
     the zone **not owned** (it jumps to the next zone). Step 2 has already required all 8
     cells, so in practice this never happens.

   Pass 1 (0x602ad0 …) scans zx from 64·lx − 64 to 64·lx + 128 **inclusive** (outer loop)
   and, for each, zy from 64·ly − 64 to 64·ly + 128 inclusive (inner loop), and takes the
   bounding box (min/max zone x and y) of the owned zones; +0x30 = 0. If none is owned, stop.
4. Picture = voxel model of (max x − min x + 1) × (max y − min y + 1) × 1 (0x4e75c0; voxel
   (vx, vy) at byte 3·(vx + vy·width)); origin **+0x20 = min zone x, +0x24 = min zone y**.
   Pass 2 (0x602e03 …) scans zx from 64·lx − 64 to 64·lx + 128 **exclusive** (outer), zy
   likewise (inner), and repeats the step 3 test with the same point, distances, neighbour
   order and strict comparison (0x602f71). The one difference: a **missing or out-of-range
   neighbour is skipped** (the test goes on with the next neighbour) instead of rejecting the
   zone. For each owned zone it writes voxel (zx − min x, zy − min y, 0) (a write outside the
   model goes to a scratch global and is lost):

   | Zone map record | Voxel |
   |---|---|
   | missing (region not loaded), or flags +0x30 = 0 | (200, 200, 200) |
   | flags ≠ 0 but not discovered (only "tile exists") | (220, 220, 220) |
   | discovered (bit 0) | (255, 255, 255) |

   Unowned zones stay empty. (A zone owned on the inclusive edge 64·lx + 128 widens the box
   but is never painted.)

   *Checking note.* 0x6024d0 is the only builder of land pictures (sole caller 0x469959), and
   the disassembly of both passes shows the full 8-neighbour test above. Saved pictures that
   are larger than this rule predicts by zones where a single diagonal cell is nearer
   therefore point at the inputs (that diagonal cell's point, D, or the zone-to-voxel
   mapping), not at a different neighbour rule. The saved `land` files carry each land's
   own cell bytes (+0x04 … +0x1f, so its point at +0x04/+0x08) and can be used to check the
   generated points.
5. Mesh it like a tile (black shade, occlusion, alpha 1), swap it into +0x2c (deleting the old
   one) and write `land<lx>_<ly>` at once (0x6050b0): u32 version, i32 +0x20, i32 +0x24,
   u8 +0x28, 3 × i32 size, u32 *c* + zlib RGB, then the 28 bytes +0x04 … +0x1f.

On the minimap the slab is tinted blue (§5.1.2), so discovered zones show as the lightest
blue, zones with only a tile slightly darker, and the rest darker still; the occlusion darkens
the slab's rim.

**Visited and rebuild requests** (0x48d51c … 0x48d5e1, every in-game frame): with C = the
world's owner cell of the player's block column and L = the map's land record found for the
player (0x601cc0: the nearest land record, by the same displaced squared distance over the
3 × 3 cells around the player, that has a picture; none if any of the nine is missing or
pictureless), if both exist and L's copied point (+0x04, +0x08) equals C's point:
L+0x28 = 1 (**visited**; the minimap's land path draws only visited lands), and if more than
2000 ms have passed since the last request (one global time, 0x76b0fc), L+0x30 = 1. So the
picture of the land the player is in is rebuilt and resaved at most every 2 s, picking up
newly discovered zones.

*Correction to `alpha-saves.md` §6.2:* the two i32 are the picture origin in zones (hence
32,600–32,800), the u8 is the visited flag, and the 28 trailing bytes are the region cell
(point x, point y, fire byte + 3 padding, moisture, warmth, id, elevation).

#### Test vectors

* **Tile size.** Lowest floor F = 40 (so lo = 39), highest b + n = 120: B = 39 ÷ 8 = 4,
  Z size = (120 − 39) ÷ 8 + 1 = 11. Lowest floor 49, highest 120: lo = 48, B = 6, Z size = 72 ÷ 8 + 1 = 10.
* **Surface voxel.** 48 columns whose top block is grass (60, 160, 40) at z = 70 and 16 water
  columns (b + n ≤ 0): S = ((48·60 + 16·80)/64, (48·160 + 16·100)/64, (48·40 + 16·255)/64)
  = (65, 145, 93.75) → bytes (65, 145, 93); h̄ = 48·70/64 = 52.5, T = 52 ÷ 8 − B = 6 − B
  (layer 2 with B = 4).
* **Body voxel.** 512 blocks of which 200 are rock (120, 120, 130) and 312 air → (120, 120,
  130). 256 grey stored blocks (100, 100, 100) and 256 water (z ≤ 0) → (90, 100, 177.5) →
  (90, 100, 177). All 512 air → stays empty.
* **Body range.** m = 40, B = 4: layers from max(0, 5 − 4 − 1) = 0 up to T − 1.
* **Rise-in.** Timer 250 → h = B − 25; after 100 ms of frames, 150 → B − 15; after 250 ms, B.
* **Markers.** A zone wholly inside one land: 0 entries. A zone crossed by one straight
  north–south border: at most 8 entries, all with the same vx (the sample column just west of
  the border, within 32 blocks).
* **Land voxel.** Owned zone with flags 0x03 → (255, 255, 255); 0x02 → (220, 220, 220);
  0x00 or record missing → (200, 200, 200). Drawn with tint (0.2, 0.4, 1)·k (§5.1.2):
  discovered at k = 1 → (51, 102, 255) before lighting and occlusion.

### 5.2 World map (resolves F-10)

The world map is not a picture: it is the minimap's 3D miniature (§5.1, map renderer 0x5fc1b0)
drawn full-window, centred on the player plus a pan offset, at the zoom the player picks, with
some extra content switched on, plus a code-drawn text overlay. While it is open the frame draws
**no world at all** (no terrain, water, clouds, sun, stars or creatures), only the sky gradient
behind the map. Routines: key handler 0x47e1b0; update 0x488ee0 (pan 0x48b62b … 0x48b7fc,
open/close bookkeeping 0x497333 … 0x497432); mouse-wheel 0x47ef40; mouse-look 0x47ea00;
mouse release 0x47ddd0; frame 0x4ac260 (map branch 0x4adede … 0x4ae279); map renderer
0x5fc1b0; overlay MapOverlayWidget (constructor 0x4c95a0, draw 0x4c9680).

State used below (game object G):

| Field | Meaning | Initial |
|---|---|---|
| G+0x8006e4 | byte **map mode** | 0 |
| G+0x800de4 | byte **teleport mode** (the map was opened from a portal) | 0 |
| G+0x800dd4, +0x800dd8 | int pair **hovered portal zone** (written by the overlay every frame) | (−1, −1) |
| G+0x800ddc, +0x800de0 | int pair **selected portal zone** | (−1, −1) |
| G+0x1c4, G+0x1c8 | current and target **map zoom** *s* (`alpha-physics-camera.md` §7.1) | 1, 1 |
| G+0x1000e4c, +e50, +e54 | float triple **pan**, in blocks (z stays 0) | 0, 0, 0 |
| G+0x8008a0 | the **map root** container (made at start-up, 0x45c090) | hidden |
| G+0x8008a4 | the overlay's container inside the map root (0x45c0d7) | — |
| G+0x8008a8 | the **cursor label** node from `cursor.plx` (0x461e1b) | — |

#### 5.2.1 Opening and closing

* **M** (0x47e7db): map mode := not map mode. No other condition beyond those of §11.2.
* **Esc** (0x47e770): map mode := 0, and the menu-button row G+0x8008f2 is toggled (§7).
* **R at a portal** (0x497403 … 0x497432): when R is pressed (edge-triggered, not while the
  quick-item wheel is open, player alive) and the interaction target G+0x800a84 resolves
  (0x5a0910) to an object whose type is **45** (0x2d; believed to be the portal, whose prompt is
  "[R] Teleport", §5), the update sets map mode := 1 **and** teleport mode := 1.
* **A teleport** (§5.2.6) closes the map.
* **Bookkeeping while closed.** Every update in which map mode is 0: teleport mode := 0 and the
  selected portal zone := (−1, −1) (0x497333 … 0x49735a), and the pan := (0, 0, 0)
  (0x48b7b8 … 0x48b7f6). The map therefore always reopens centred on the player. The zoom is
  **not** reset: it keeps its last value across closing and reopening.
* While the map is open the game is in **cursor mode** (0x47f1d0 returns true when map mode is
  set): the cursor is shown and not re-centred (§11.2), and the camera's focus point is not
  updated (`alpha-physics-camera.md` §7.3 item 5).

#### 5.2.2 Controls: zoom, pan, turn and tilt

**Zoom (mouse wheel only; 0x47ef40).** Each wheel message gives n = ±1 (+1 = wheel up). With
the map open:

* n < 0 (wheel down): target zoom G+0x1c8 ×= 0.9 (zoom **out**);
* n > 0 (wheel up): target zoom ÷= 0.9 (zoom **in**);
* then clamp to **[0.01, 10]** (0.01 = 0x6fcda0, 10 = 0x745e74).

The current zoom G+0x1c4 eases towards the target every update: Blend(G+0x1c4 → G+0x1c8,
0.004), i.e. w = 1 − 0.996^dt (dt in ms; 0x496611 … 0x496627, `alpha-physics-camera.md` §7.3
item 2). No key changes the zoom. (With the map closed the wheel changes the camera distance
instead.) The current zoom *s* is passed straight to the map renderer as its **view scale**
(there is no zoom table as for the minimap), and the content switches at these values:

| *s* | Effect |
|---|---|
| *s* < 0.35 | **land pictures** instead of zone tiles (renderer's land path, §5.1.2; 0x6fd8d8 = 0.35) |
| *s* ≥ 1.6 | the town NPC signs join the creature markers (§5.1.2; 0x71e068) |
| *s* > 2 | the overlay draws **zone labels** (district and portal names, §5.2.5 pass A; 0x745e10) |
| 8/*s* | size of the mission and town markers, clamped to [2, 10] (§5.2.4) |

From the start value 1: 10 notches down give 0.9¹⁰ = 0.349 (land pictures); 5 notches up give
1.694 (signs); 7 notches up give 2.091 (zone labels); the limits are 22 notches up (10.15 →
clamped to 10) and 44 notches down (0.0097 → clamped to 0.01).

**Pan (middle mouse button held; 0x48b6ca … 0x48b7b5).** Every update while the map is open and
the middle button flag (G+0xa, DirectInput button 2, 0x4c8ae0) is set, under the lock
G+0x8005d0:

1. (Δx, Δy) = cursor position − cursor position of the previous frame, in window pixels (the
   interface object G+0x800710 keeps current +0xd4/+0xd8 and previous +0xdc/+0xe0).
2. v = (Δx, −Δy, 0) × (4 / *s*) blocks.
3. v is turned about z by the camera yaw ψ (rotation matrix 0x424610 of element 2 of the angle
   triple G+0x1a4, applied with 0x488e50), so the drag is aligned with the screen.
4. pan += v.

One pixel of drag moves the pan by 4/*s* blocks = 0.5/*s* map units; at the window centre a
map unit is about 2.9·*s* px tall in a 720-px window, so the map moves about 1.45 × as far as
the cursor (worked example §5.2.7). The content follows the cursor's direction. No key pans
the map.

**Turn and tilt (left button drag).** Cursor mode's mouse look (0x47ea00, `alpha-physics-camera.md`
§7.2) also runs with the map open: while the left button is held and the cursor is over no
widget (0x650ae0 = 0), the cursor movement changes the camera's pitch target θ* (clamped to
[0, 180]) and yaw target ψ*, with the usual smoothing. The map uses the current θ and ψ
(§5.2.3), so a left drag **turns** the map about the vertical (ψ) and **tilts** it (θ).
*Note:* `alpha-physics-camera.md` §7.2 says the drag "moves the map instead" with the map open;
in the code the left drag still turns the camera and the **middle** drag pans.

#### 5.2.3 The map frame (0x4adede … 0x4ae279)

The frame routine runs as usual up to the sky (`alpha-rendering.md` §1.3 steps 1–6): clear
colour (0, 0.44, 1) and depth, palette, the frame constants (including the set-up lights at
0x4ad957: front P4, back P5, ambient P5 and the sun direction, `alpha-rendering.md` §4 row
"Frame set-up and sky"), all ten point lights cleared, then the sky-gradient quad. The only
change there: fogGradientTranslation is computed from the pitch **90** instead of θ
(0x4ada17 … 0x4ada3d), so the sky gradient is the one a horizontal camera would see,
(90 − 80)/(−30) = −1/3. Then, if map mode is set (0x4adede):

1. **Rotation.** rot = (min(−θ, −110°), 0, −ψ) (θ = G+0x1a4, ψ = G+0x1ac; −110 = 0x702b84;
   0x4adeeb … 0x4adf0d, 0x4adf5d … 0x4adf82).
2. **Pivot.** Pan → fixed point: each component trunc(pan × 65536) as 64-bit (0x4adf13 …
   0x4adf51); pivot = local player position (G+0x8006d0 +0x10) + that (0x42c800).
3. **Map renderer call** (0x4adf6c … 0x4ae004), with the minimap's argument order: pivot;
   projection centre x₀ = trunc(W·0.5), y₀ = trunc(H·0.5); window W = G+0x11c, H = G+0x120;
   rotation rot; view scale *s* = G+0x1c4; frame time step G+0x8006e8; **radius 16**; **map
   extras on**. What it draws is §5.2.4.
4. Material (1, 1, 1, 1) (0x4ae009 … 0x4ae043).
5. **Map-mode lights** (0x4ae048 … 0x4ae15e): front (1, 1, 1), back (0.2, 0.3, 0.4), direction
   normalize(0, 0.5, −1) = (0, 0.4472, −0.8944), ambient (0.4, 0.4, 0.4). They are set **after**
   the map renderer has returned, so they do **not** light the map; they light only the compass
   of step 7. (The next frame's constants at 0x4ad957 replace them before the next map draw.)
6. **Clear depth** to 1.0 (device Clear, depth only; colour 0xFF6464FF is ignored; 0x4ae163 …
   0x4ae187).
7. **Compass.** Model slot 2559 `compass2` (29 × 29 × 15 voxels; taken from the model list
   G+0x300 when it holds more than 2559 entries, else nothing is drawn) is drawn by the
   interface model routine 0x476660 (the one the quick-item wheel uses, F-13) at window pixel
   **(W − 60, 40)**, rotation (−120°, 0, −ψ), scale 0.003, centred on its middle
   (0x4ae189 … 0x4ae21b). It turns with the camera yaw like the map, so it shows north.
8. **Interface.** The in-game root G+0x800884 is hidden (so the HUD, bars, chat and every other
   node under it disappear) and the map root G+0x8008a0 is shown (0x4ae220 … 0x4ae24b); the
   interface is drawn (0x650980), which draws the MapOverlayWidget (§5.2.5) and the cursor
   label.
9. EndScene, release the world lock (0x601e90 on G+0x2e4), and jump to the end of the frame
   (present). Nothing else of the world frame runs.

#### 5.2.4 What the map renderer draws with map extras on

Everything of §5.1 applies (matrix chain, units, draw state, creature markers), with these
differences.

**Projection and view.** x₀ = W/2, y₀ = H/2, so *o*ₓ = *o*ᵧ = 0 for even window sizes: the pivot
is at the **window centre**. The view turns about x by min(−θ, −110°) instead of a fixed −120°:
world up maps to (0, −sin φ, cos φ) with φ = that angle, so the map is seen from
(−φ − 90)° above its ground plane: **30°** at the default θ = 120, **90° (straight down)** at
θ = 180, and never flatter than **20°** (θ ≤ 110). The scale step uses *s* = the map zoom.

**Terrain.** With *s* < 0.35 the **land path** (§5.1.2) with radius 16: the land pictures of
the 33 × 33 land cells around the pivot's land cell (only visited lands with a picture), tinted
blue as on the minimap. With *s* ≥ 0.35 the **zone path** with radius 16: the 33 × 33 zones
(zx, zy) = pivot zone + (i, j), i, j = −16 … 16 (x outer, y inner). For each zone, in this order:

1. **Placeholder** (extras only; 0x5fd89d …): when the zone has no map record, or no tile, or a tile that is still rising (timer
   +0x2c ≠ 0), draw model slot 2567 `maptile` (8 × 8 × 8 voxels, all solid, greys 212–255)
   scaled **×4** (so one zone wide and 32 map units tall), placed at the zone's square like a
   tile (the translation t of §5.1.2), with its base at height
   z = (E − 100)·0.125 + rise − pivot z ÷ 8 map units, where E is the elevation (+0x1c) of the
   land record found for the zone's corner block (0x601cc0; 0 when none) and
   rise = (timer/250 − 1)·20 when a rising tile exists, else 0 (so the placeholder sinks 20
   map units while the real tile rises in). Material (the tint), by the zone record's flags
   byte +0x30:

   | Flags | Material |
   |---|---|
   | no record, or flags = 0 | (0.2, 0.3, 1, 1) |
   | flags ≠ 0, not discovered | (0.2, 0.35, 1, 1) |
   | discovered (bit 0) | (0.2, 0.4, 1, 1) |

   Material reset to (1, 1, 1, 1) afterwards. So unknown parts of the map show as flat blue
   blocks following the land's elevation, a little lighter where the player has been.
2. **Tile** (as on the minimap, rise-in included) with material (1, 1, 1, 1), **dimmed** to
   (0.6, 0.6, 0.6, 1) (0x5fe9f8 … 0x5fea4f; 0.6 = 0x6fd8dc) when the zone is **not discovered**
   (flags bit 0 clear), **except** the zone that is the target of an active mission: the
   world's location record of the zone's map cell (0x487da0 for (zx ÷ 8, zy ÷ 8)) has a
   non-zero int at +0x34, byte +0x41 = 1, and its int pair at +0x4c, +0x50 equals (zx, zy).
   (These are fields of the record's mission area, `alpha-worldgen-locations.md` §1.2 +0x2c …;
   their names are inferred.) Material reset afterwards.
3. **Zone markers** (land borders, as §5.1.2).

**Mission and town markers** (extras only; 0x5fee56 … 0x5ffdd0). After the terrain, for each of
the 64 location records (8 × 8, x outer) of the world region that holds the pivot (0x5cb0d0),
with the block view (map object +0x44) and the plain projection, material (1, 1, 1, 1), the
terrain's lights, scale k = clamp(8/*s*, 2, 10) (left unscaled when exactly 1, which cannot
happen inside the clamp):

* **Mission marker**, when the int at +0x34 is non-zero and byte +0x41 ≠ 2 (2 believed to mean
  finished): model slot 2560 `mission`, at (P − pivot) blocks, where P = the centre of zone
  (+0x4c, +0x50) (block 256·z + 128 per axis) when +0x34 = 1, else the record's centre
  (+0x00, +0x08), and P.z = the record's ground height (+0x14) **+ 100 blocks**. World matrix:
  translate by −(size x/2, size y/2, 0) (centred in x and y, its bottom at P), scale k, turn
  about z by T·0.02° (T = the map object's millisecond counter +0x8000b8), then translate to
  P. It spins at **20° per second** (one turn in 18 s).
* **Town marker**, for every record of type 1 (a town), when the **local player's** horizontal
  distance to the town centre is more than 512 blocks (distance² > 262,144, 0x703a1c): model
  slot 2561 `city`, at the record's centre, ground height + 100 blocks, centred in x and y,
  scale k, not turned.

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

**State left behind** as §5.1.4: map object +4 = P, +0x44 = V/8 (the overlay's matrices).
*Note:* the mission, town and zone markers are drawn with the +0x44 view stored by the
**previous** call (it is rewritten later in the same call), so they lag one frame behind a
change of pan, zoom or angle, and on the first map frame after play they use the minimap's
view; an implementation should use the current view.

#### 5.2.5 The overlay (MapOverlayWidget, 0x4c95a0, draw 0x4c9680)

**Construction** (0x45c0f1 … 0x45c11b): a 0x164-byte widget built on the interface G+0x800710
with parent container G+0x8008a4 (inside the map root); the game object G is kept at
widget+0x160. Its draw runs whenever the interface is drawn with the map root visible, i.e.
in map frames only, after the map renderer has stored its matrices.

**Screen position of a point.** For a point Q (blocks, fixed point) the overlay forms
d = (Q − player position − pan) in blocks (pan's x and y; the z offset is Q.z − player z),
then, with the map object's matrices (G+0x800d44 + 0x44 = V/8, + 4 = P), row vectors:

1. e = (dₓ, dᵧ, d_z, 1)·V/8; if e.z ≤ 0 the point is skipped;
2. c = (e.x, e.y, e.z, 1)·P; n = (c.x/c.w, c.y/c.w); skipped unless −1 ≤ nₓ, nᵧ ≤ 1;
3. pixel x = W/2 + nₓ·W/2, pixel y = H/2 − nᵧ·H/2 (W, H = the interface size, G+0x800710
   +0x10c, +0x110).

The pixel is the text's origin (first baseline, §3.4).

**Text.** Every label is drawn twice with font `resource1.dat`, centred on the origin
(layout flag 1, §3.4): first fill white (1, 1, 1, 1) with a **black (0, 0, 0, 1) outline of
3 px**, then the label's fill colour with no outline over it. The visible result is the fill
colour with a black 3-px rim.

At the start of every draw the hovered portal zone G+0x800dd4 := (−1, −1). Then two passes.

**Pass A: zone labels** (only when *s* = G+0x1c4 **> 2**; 0x4c9787 … 0x4ca3ef). Centre zone
(c_x, c_y) = floor((player + pan)/256) per axis. For zx = c_x − 32 … c_x + 31 (outer),
zy = c_y − 32 … c_y + 31: take the zone map record (0x602440, §5.1.6); skip unless its
per-zone copy has kind (+0x10) ≠ 0 **and** it is discovered (+0x30 bit 0). Point Q = zone
centre (256·zx + 128, 256·zy + 128) at z = the generator's base height there (0x5c5e20). Text
= the zone **detail** (0x4e5a20, §5.9.1 "The detail") of that entry: "Trade District",
"Crafting District", "Adventurer District", "Pet District" or "Portal of *N*" (dungeon
entrances, kind 3, have no key, so their text is empty and nothing shows). **10 px.** Fill:

| Zone kind | Condition | Fill |
|---|---|---|
| 1 (town quarter) | always | white (1, 1, 1) |
| 3 (dungeon) | entry level (+0x18) > player level (+0x190) + 2 | red (1, 0.2, 0.2) |
| 3 | player level − 2 ≤ entry level ≤ player level + 2 | green (0.2, 1, 0.2) |
| 3 | entry level < player level − 2 | white |
| 4 (portal) | not in teleport mode | white |
| 4 | teleport mode | purple (0.6, 0.25, 1) |
| 4 | teleport mode and this zone is the **selected** zone | pink (1, 0.25, 1) |
| 4 | teleport mode and the cursor is in the label's **hover box** | cyan (0.2, 1, 1), and hovered zone := (zx, zy) |

The hover box is cursor x ∈ [x − 100, x + 100), cursor y ∈ [y − 20, y + 10) around the label
origin (100 = 0x6fd0b4, 20 = 0x6fcd74, 10 = 0x745e74). Hover wins over selected. Portal labels,
and with them teleporting, therefore need *s* > 2.

**Pass B: location names** (every zoom; 0x4ca4ee …). Same 64 × 64-zone scan. For each zone
whose record is discovered, take the location copy of its 8 × 8-zone map cell (0x6023b0, map
region record +0x34000, §5.1.6). Draw it when it exists, its type (+0x18) is neither 0 nor 10,
it has not been drawn yet in this draw (a set of record addresses), and the zone's centre is
inside it: bump(f(L, zone centre)) > 0 (0x5fa4c0, §5.9.1). So a place's name appears as soon as
the player has discovered any zone whose centre lies within the place. Point Q = the record's
centre (+0x00, +0x08) at the generator's base height there (0x5c5e20). Text = the **location
name** (0x4e5590, §5.9.1: "*N* City", "*N* Lake", "Ruins of *N*", "*N* Castle", …). **12 px.**
Fill, with p = the level power 0x43ca60 (§5.4.1 item 4) of the record's level (+0x24) and p₀
that of the player's level:

| Location | Condition | Fill |
|---|---|---|
| type 1 (town) | always | white |
| other | p > p₀ + 0.1 | red (1, 0.2, 0.2) |
| other | p₀ − 0.1 < p ≤ p₀ + 0.1 | cyan (0, 1, 1) |
| other | p ≤ p₀ − 0.1 | white |

Region names ("Lands of *N*", "*N* Ocean") are **not** drawn on the map; nor is a player arrow,
a frame, a legend or a grid. Everything else on the map comes from the renderer (§5.2.4) and
the compass (§5.2.3 step 7).

#### 5.2.6 Cursor, selecting a portal and teleporting

* **Cursor label** (0x48b3a5 …, 0x48b638 … 0x48b6c1): every update the cursor label
  G+0x8008a8 is first set to ""; then, with the map open, teleport mode on, a selected zone
  ≠ (−1, −1) and selected = hovered, it is set to **"Teleport"** (string 0x701830).
* **Left button release** (0x47ddd0, button 0, map open, 0x47df01 …):
  * if teleport mode is on, the selected zone ≠ (−1, −1) and it equals the hovered zone:
    **teleport**: under the world's lock (0x601cb0 / 0x601e90), the local player's position :=
    ((256·zx + 128)·65536, (256·zy + 128)·65536, **0**) in fixed point, i.e. the centre of the
    portal's zone at height 0 (the physics then resolves the player out of the ground, not
    traced here); hovered and selected := (−1, −1); map mode := 0;
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
  zone not discovered but its region known (flags 0x02): base z = (140 − 100)·0.125 − 12.5 =
  −7.5 map units, top at 24.5; tint (0.2, 0.35, 1) on maptile grey 233 → (46.6, 81.6, 233)
  before lighting and occlusion.
* **Markers.** At *s* = 2.5 the mission and town markers are scaled by 8/2.5 = 3.2; at
  *s* = 0.5 by 10 (clamped); at *s* = 6 by 2 (clamped). A mission marker turns 3.2° in a
  160-ms span of frames.
* **Location colour.** Player level 10 (p₀ = 0.31034), a castle of level 14
  (p = 1 − 1/1.65 = 0.39394): p ≤ p₀ + 0.1 = 0.41034 and > p₀ − 0.1 → cyan. Level 20
  (p = 0.48718 > 0.41034) → red. Level 5 (p = 0.16667 ≤ 0.21034) → white.

#### 5.2.8 Open points

* What the location record's mission fields (+0x34, +0x41, +0x4c, +0x50) mean exactly, and
  what object type 45 is, belong to the mission and object specifications.
* Whether the map containers can ever be hit by the widget test 0x650ae0 (they are created with
  zero size at 0x45c090 / 0x45c0d7, so a left drag over the map is believed always to turn the
  camera).
* The compass routine 0x476660's exact matrix (placement and lens) is part of F-13.
* The point stored at map object +0x84 … +0x8c (§5.1.4) has no reader in the overlay.

### 5.3 Player bars: placement, fill and text (F-8)

All of this is redone **every frame** in the game update (0x488ee0), after the layout routine,
so these per-frame anchors win over anything 0x482a40 or the start-up code wrote. "A" is where
the template's pivot lands (§2.1); "O" = A − pivot is the offset added to every child's
shape-plus-translation to get window pixels. Fill fraction *f* is applied to the `bar` child as
an x-scale about its pivot (§2.1).

**Anchors, values and text** (0x490a93 … 0x492f1d):

| Bar (node) | Anchor A (pivot) | Fill *f* | Text written | Shown |
|---|---|---|---|---|
| `experiencebar` | (100, 75) | XP ÷ XP-for-next-level, clamped to ≤ 1 | `text` = XP + "/" + XP needed (both integers) | always |
| `lifebar` | (100, 30) | HP ÷ max HP (not clamped) | `text` = ⌊HP⌋ + "/" + ⌊max HP⌋; `info` = player name; `class` = "LVL " + level + " Warrior" / " Ranger" / " Mage" / " Rogue" (class 1–4) | always |
| `manacubebar` | (100, 130) | — | — | always; the first *k* children visible, *k* = mana cubes mod 4 (4 while a sequence plays in it; the file has none, so effectively mod 4) |
| pet `lifebar` clone (G+0x800770) | (100, 180) | pet HP ÷ pet max HP | as `lifebar`, `info` = pet name, `class` = "LVL " + level + " " + species name | only while the player has a pet (G+0x2e4 lookup of the player's pet id) |
| pet `experiencebar` clone (G+0x8007cc) | (100, 225) | pet XP ÷ needed, ≤ 1 | as `experiencebar` | with the pet |
| `ridingbar` | (100, 290) | — | — | with the pet; children visible: *k* = min(5, trunc(riding value·5 + 0.9999)) |
| party `lifebar` clones (list G+0x800778, 3 of them) | (310 + 220·*j*, 30), *j* = 0, 1, 2 | that player's HP ÷ max HP | as `lifebar` | one per other **player** creature in the world list (type 0, not the local player), in list order, at most 3; all hidden first each frame |
| `mpbar` | (W/2 + 8, H − 90) | `bar`: mana (0…1); `chargebar`: charge (0…1) | `text` = trunc(mana·100 + 0.5) + "/100" | always |
| `hpbar` | (W/2 − 163, H − 90) | HP ÷ max HP | `text` = ⌊HP⌋ + "/" + ⌊max HP⌋ | always |
| `staminabar` | (W/2, H − 115) | stamina (0…1) | `name` stays "Stamina" | only while stamina < 1 |
| `castbar` | (W/2, H − 150) | see below | `name` = caption below | see below |

Integer conversions are truncations toward zero (0x68d910 and the truncating float-to-integer
conversions in 0x490e54 …).

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
the notched outline drawn as a black stroke (8 px wide on life/XP/MP/HP, 4 px on stamina/cast;
flags 6 = stroke only). Pet and party bars are the same shapes moved with their anchor.

**Labels** (window pixels, origin = first baseline, §3.4; every label is an outlined node with a
plain node over it, §4.4 of the Plasma spec):

| Bar | Label | Origin | Size, alignment, colour |
|---|---|---|---|
| `lifebar` | `info` (name) | (100, 17) | 10 px, left, white, black outline 3 |
| | `class` | (100, 31) | 10 px, left, cyan (0, 0.979, 1) |
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

* Shown when any of: block power (player value at +0x174) < 1; the player is in one of the
  listed casting animations below and the animation time ≤ its duration; the stealth meter
  (+0x1190) > 0 *(believed: stealth)*; the "blocking" test (0x444590) is true and block power ≠ 0.
  Otherwise hidden.
* Content, in this priority: stealth meter > 0 and not in the state tested by 0x444610 →
  *f* = stealth meter, caption "Stealth"; else blocking or block power < 1 → *f* = max(0, block
  power), caption "Block Power"; else *f* = animation time ÷ animation duration, caption from the
  animation id: 0x1e "Fire Swirl", 0x1f "Fire Vortex", 0x20 "Water Swirl", 0x21 "Water Vortex",
  0x22 "Healing Stream", 0x25 "Fireball", 0x26–0x28 "Firebolt", 0x29/0x2a/0x2c "Water drop",
  0x2b "Water splash", 0x2d "Water Salvo", 0x2e "Fire Salvo", 0x31 "Teleport", 0x57 "Explosion",
  0x58 "Fire Explosion", 0x59 "Lava", 0x5c "Summon", 0x5e "Firebeam", 0x5f "Fireray",
  0x69 "Teleport to City"; any other id → empty caption (table 0x49d3e0/0x49d430).

**Worked examples (test vectors).**

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

### 5.4 Creature life bars over heads (F-8)

Rebuilt every frame (0x499e9b … 0x49b14a): the container G+0x8007b0 is emptied, then for each
candidate creature in a list prepared earlier in the update (0x4a8f20 iteration) one bar is
**cloned** under that container. Only the `:small` templates are used in Alpha
(`enemylifebar:small` G+0x8007a0, `friendlifebar:small` +0x8007a4, `staticlifebar:small`
+0x8007a8, `neutrallifebar:small` +0x8007ac); the large forms (G+0x800790 … +0x80079c) are
looked up at start-up but never used.

* **Which creatures.** Never the local player, nor a creature the player is riding. A list
  entry carries a "focus" flag, set only for this frame while the creature is under the
  crosshair (§5.4.1 item 6). Unfocused creatures are shown only if the **V** toggle (G+0x8007b4)
  is on or the creature is a player (controller kind 0); with V off, unfocused non-player
  creatures get no bar. The creature must be within a visibility range
  of 200 units, reduced in darkness: range = 200·(light·0.9/0.3) + 20 when the light value at the
  creature (0x4718b0 ÷ 255 × a factor) is below 0.3 *(inferred)*; 0x59ee90 tests the line of
  sight.
* **Template by controller kind** (creature byte +0x60, `alpha-creatures-behaviour.md` §1;
  not the creature type at +0x64): 1 → `neutrallifebar:small` if the creature is a peaceful
  animal (0x444680, §5.4.1 item 5), else `enemylifebar:small`; 6 → `staticlifebar:small`;
  anything else (players, NPCs, pets) → `friendlifebar:small`.
* **Position.** The anchor point is the creature's position raised by half its height minus
  the value at +0x1180 (+2 more for animation states 0x6a/0x6b), projected with the game camera
  to window pixels; behind the camera → no bar; unfocused bars outside the screen (normalised
  x or y outside −1 … 1) → no bar; the window y is clamped to ≥ 50. The bar's pivot is put on
  that point.
* **Shapes** (relative to the anchor point P): `bar` x P.x − 40.06 … P.x − 40.06 + 80·*f*,
  y P.y − 11 … P.y − 5 (80 × 6); `bar2` (black, alpha 0.392) is scaled by 1 − *f* about its pivot
  on the right edge, so it covers x P.x + 39.94 − 80·(1 − *f*) … P.x + 39.94; `frame` = 4-px
  black stroke around the 80 × 6 bar. Bar colours: enemy (0.931, 0, 0) → (1, 0.228, 0.228);
  friend (0, 0.395, 1) → (0.216, 0.525, 1); static (0.497 grey); neutral (0.179, 1, 0); all at
  alpha 0.738. *f* = HP ÷ max HP.
* **Texts.** `info` (origin P + (−0.08, −21), 12 px, centred) = the species name from the
  dictionary ("singular" form; key from 0x59aa60) + " +" + the creature's power tier when
  non-zero (+0x1a8) (§5.4.1 items 1–2). `rarename` (P + (−0.08, −35), 12 px, centred; fill
  purple (0.502, 0, 1) on enemies, (0, 0.731, 1) on the others) = the creature's own name when
  it has one; otherwise a generated title for bosses (appearance flag 0x200 at +0x7e) and NPCs
  (controller kind 3) (0x5a0ed0, §5.4.1 item 3); hidden when empty. The small templates have
  no `text` node, so the "HP/max" string written to `text` has no effect.
* **Name colour** (applied to every `info` text's fill): players (controller kind 0)
  (0, 0.8, 1); controller kind 6 (0.8, 0.8, 0.8); others by comparing a level-power value
  p = 0x43ca60(level) (§5.4.1 item 4) with the local player's p₀: p > p₀ + 0.3 → red
  (1, 0.2, 0.2); p > p₀ + 0.1 → orange (1, 0.6, 0.2); p > p₀ − 0.1 → (0, 1, 1); otherwise white.

**Worked example.** A goblin with 45 of 60 HP, projected to (700, 300): *f* = 0.75; `bar` x
659.94 … 719.94, y 289 … 295; `bar2` x 719.94 … 739.94; species label baseline (699.92, 279).

### 5.4.1 Names, titles and colours of creature bars

Creature fields used here (object offsets; the network record offset is 0x10 less):
+0x08 the 64-bit **creature id**; +0x60 the **controller kind** (0 player, 1 hostile NPC,
3 NPC, 6 inert object; `alpha-creatures-behaviour.md` §1); +0x64 the **creature type**
(0 … 155, a 32-bit integer); +0x7e the appearance flags (0x200 boss, 0x800, 0x1000);
+0x190 the level; +0x1a8 the **power tier** (u8, record field 37); +0x1168 the creature's
own name (the 16-byte network name field, 8-bit characters).

**1. Species name (`info`).** Two lookups (0x49adcf … 0x49ae3e):

1. *Type → key* (0x59aa60). The world object (G+0x2e4) holds a table from creature type
   (+0x64, compared as a signed 32-bit integer) to a **dictionary key**, filled once in the
   world constructor (0x58ee37 … 0x5901b0). A type with no row gives the empty key.
2. *Key → text* (0x480e00 on the world's dictionary, G+0x314). The dictionary keeps the
   `<name>` elements of `dict_en.xml` (data4.db) by key — only elements named `name`, not
   `item`, `landscape` or `speech` — and for each its child elements by element name. The bar
   asks for the child **`singular`**. If the key is not there, or the entry has no
   `singular`, the result is the empty string. Keys and element names compare exactly
   (case-sensitive).

The label is then that text, followed — only when the power tier is non-zero — by " +", the
tier as a decimal number, and a line break (the text is built in a wide string stream ended
with `endl`, 0x49ae76 … 0x49ae9f). With tier 0 there is no line break. A missing name with a
tier gives just " +N".

The key table (every row of the constructor; "en" is the English `singular` it yields today;
types 36 and 62 are written twice with the same key):

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
no `<name>` entry in either dictionary, so they also show an empty label. "Insektoid" (52) is
the dictionary's own spelling.

**2. The " +N" number (+0x1a8).** It is the creature's **power tier**, the same value that
raises maximum health by 2^(0.25·tier) (`alpha-creatures-behaviour.md` §4, 0x444db0). It is
copied unchanged from the spawn object's byte +0x58 when the world step creates the creature
(0x61004c; `alpha-creatures-behaviour.md` §9.1–9.2). The world generator fills that byte from
the zone's or location's **sub-level** (`alpha-worldgen-locations.md` §6.1 step 9: 0 below
level 5, otherwise 0 … 4 from draws; e.g. 0x5ba4ff, 0x5be675, 0x5d3bb3, 0x5eae58); mission
creatures take the map cell's tier instead (§9.2 step 6 there). Players have tier 0 (record
initialiser). Formatting: unsigned decimal, no padding, no sign other than the literal "+",
preceded by one space: "Goblin +2".

**3. Generated title (`rarename`, 0x5a0ed0).** Chosen in this order (0x49a881 … 0x49aa48):

1. The own name (+0x1168) is not empty → that name (converted to wide text).
2. Otherwise, appearance flag 0x200 set → generated title.
3. Otherwise, controller kind 3 → generated title.
4. Otherwise → empty (the node is hidden).

The generated title uses **no random draws and no dictionary**. With *s* = the low 32 bits of
the creature id (+0x08) as an unsigned number and *t* = the creature type:

* *a* = (⌊*s*/7⌋ + 13·*s*) mod 2³², index A = *a* mod *n*;
* *b* = (7·*s* + ⌊*s*/10⌋) mod 2³², index B = *b* mod *n*;
* title = first-part[A] followed directly by second-part[B] (no space).

All arithmetic is unsigned 32-bit with wrap-around; *n* = 10 for the listed types and 20 for
the fallback. The parts (built once, on first use):

| Types | *n* | First part [0 … n−1] | Second part [0 … n−1] |
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
| every other type (6, 17, 19 … 42, 44, 46 … 82, 85 and up) | 20 | Kro, Aru, As, Ge, Kur, Lugo, Iko, Liku, Tero, Var, An, The, Ga, Lan, Dura, Dama, Se, Thal, Nar, San | no, mi, la, sel, ron, ion, kor, lon, rok, tar, den, gar, dar, dara, lan, ka, gor, rior, ria, mor |

(The groups follow the species keys of item 1: 0/1 elves; 2/3 humans, with 18 OldMan,
43 Wizard, 83 NomadMale and 45 Witch, 84 NomadFemale; 4/5 goblins; 7/8 lizardmen; 9/10
dwarves; 11/12 orcs; 13/14 frogmen; 15/16 undead.)

Because the title depends only on the id and the type, a creature keeps the same title every
frame and on every client that knows the same id.

**4. Level power (0x43ca60).** For a level *L* (the integer at +0x190 converted to a float):

  p(*L*) = 1 − 1 / ((*L* − 1) · 0.05 + 1),

evaluated in single precision (subtract 1, multiply by 0.05, add 1, divide 1 by it, subtract
from 1). p(1) = 0, p(10) ≈ 0.31034, p(20) ≈ 0.48718; it tends to 1. The same curve is the
*g*/3 of the health formula (`alpha-creatures-behaviour.md` §4).

**5. Neutral bar test (0x444680).** True ("peaceful animal") exactly when the appearance flags
(+0x7e) have none of 0x0200, 0x0800, 0x1000 set **and** the creature type is one of these 32:
19 Collie, 20 ShepherdDog, 22 Alpaca, 23 BrownAlpaca, 25 Turtle, 26 Terrier,
27 ScottishTerrier, 30 Cat, 31 BrownCat, 32 WhiteCat, 33 Pig, 34 Sheep, 35 Bunny, 53 Hornet,
55 Crow, 56 Chicken, 57 Seagull, 58 Parrot, 67 Peacock, 68 Frog, 74 Duckbill, 87 Mole,
89 Koala, 92 Owl, 93 Penguin, 98 Horse, 100 Cow, 106 Crab, 107 SeaCrab, 145 SapphireFish,
146 LemonFish, 147 Seahorse. Only creatures of controller kind 1 reach this test (§5.4); a
boss Crow therefore gets the red enemy bar.

**6. The focus flag.** The candidate list is rebuilt from scratch every frame
(0x4908cf … 0x4909f4): it is emptied, then every creature of the world whose health (+0x16c)
is above 0 and whose squared distance from the local player is at most **3600** (60 blocks;
positions in 1/65536-block units, the creature's taken from +0x1350, the player's from
+0x10) is added as {creature, squared distance, focus = 0}; the list is then sorted
(0x458ba0). A second pass (0x498690 … 0x498d03) sets focus = 1 on an entry when all of these
hold:

1. it is not the local player (and, while the local player's action +0x68 is 34 and its
   action time is below the value of 0x43d1a0, it is not of controller kind 1 or 6);
2. the line of sight from the camera to it is clear (0x59ee90) within range 256 — in darkness
   (light ÷ 255 × factor < 0.3, as in §5.4) the range is light·0.9/0.3·256 + 25.6 instead;
3. its box covers the screen centre: the eight corners of a box the creature's size + 1 in
   each axis, centred on its position (lowered by +0x1180), are projected with the game
   camera (G+0x26c); among the corners in front of the camera there is at least one with
   normalised x < 0, one with x > 0, one with y < 0 and one with y > 0.

So "focus" means **under the crosshair this frame**; it lasts exactly one frame and is
re-evaluated on the next. (The same pass also stores the id of the nearest focused creature as
the current target, G+0x800a70, and finds the nearest creature within 16 blocks.)

**Worked examples.**

* Goblin, type 4, tier 2: key `GoblinMale` → "Goblin"; `info` = "Goblin +2" + line break.
  Tier 0: "Goblin".
* Boss goblin (flag 0x200), type 4, id low bits *s* = 1000, no own name: A = (142 + 13000)
  mod 10 = 2 → "Ur"; B = (7000 + 100) mod 10 = 0 → "zic"; title "Urzic".
* NPC (controller kind 3) of type 44 (Bandit), *s* = 123456: A = (17636 + 1604928) mod 20 = 4
  → "Kur"; B = (864192 + 12345) mod 20 = 17 → "rior"; title "Kurrior"; `info` is empty
  (no dictionary entry).
* Colours, local player level 10 (p₀ ≈ 0.31034): a level-20 creature (p ≈ 0.48718) is above
  p₀ + 0.1 but not p₀ + 0.3 → orange; level 14 (p ≈ 0.39394) → (0, 1, 1); level 5
  (p ≈ 0.16667) → white.

### 5.5 Ability bar (F-3)

Built once at start-up (0x460232 … 0x46050b) and updated every frame (0x48b8ab … 0x48c100,
0x491f16 … 0x492727).

**Buttons.** Row A (G+0x8007fc) holds 7 nodes: six clones of `abilitybutton` (index 0–5) and then
the `quickitembutton` template itself (index 6). Each button's pivot (the top-left corner of its
42 × 42 body) is placed at x = W/2 − 162 + 48·*i*, y = H − 50 (§2).

| Index | Key label (`info`) | Highlighted while held | Content |
|---:|---|---|---|
| 0 | "M1" | left mouse button | ability in slot 0 |
| 1 | "M2" | right mouse button | slot 1 |
| 2 | "1" | key 1 | slot 2 |
| 3 | "2" | key 2 | slot 3 |
| 4 | "3" | key 3 | slot 4 |
| 5 | "4" | — (no highlight is applied to this button) | slot 5 |
| 6 | "Q" (from the template) | Q | quick item |

(Labels: "M1", "M2" literal; the others are the index − 1 written as a number, 0x460409.)

**Shapes, relative to the button's top-left corner (x₀, y₀)** (from `gui.plx`):

* Body: a 42 × 42 square with 3-px notched corners, white fill, black 4-px stroke; it carries
  the **icon** as its texture (`SmoothMeshShape.texture`, shape offset 0x7ac).
* `frame`: the same outline inset by 1 px (x₀ + 1 … x₀ + 41), 2-px white stroke, no fill. Its
  display fill colour is set every frame: **(1, 0, 0.2, 1)** while the button's control is held
  (and no window/menu has input), else (1, 1, 1, 1) (0x4922bb … 0x492713).
* `cooldown`: a black rectangle, alpha 0.552, x₀ + 1 … x₀ + 41, with its pivot on its bottom
  edge (y₀ + 41). It is scaled vertically by *c* (0x434ad0 with (1, *c*)) so it covers
  y₀ + 41 − 40·*c* … y₀ + 41.
* `info` (key label): origin (x₀, y₀ + 4), 10 px, left-aligned; white with a black 2.5-px
  outline (the outline node's own fill is (0, 0.763, 0.814), hidden under the white).
* Quick item only: `count` origin (x₀ + 36, y₀ + 37), 10 px, right-aligned; a 36 × 36
  `background` box (x₀ + 3 … x₀ + 39, y₀ + 3 … y₀ + 39) in which the item model is drawn by the
  model widget G+0x800a18 *(inferred)*; the body fill is black, alpha 0.469.

**Per-frame rules.**

1. *Slot list.* The six ability ids are rebuilt every frame from the player's class,
   specialization and weapon (0x48b8ab … 0x48bd3e; the rules belong to the player-control
   specification). Slots 2–5 are emptied (id 0) when the matching skill level (player values
   from +0x1150 in steps of 4) is ≤ 0.
2. *Icon.* Look the id up in the icon table (G+0x800820, built at start-up from `data3.db`);
   found → that texture; not found → `none.png` (id 0). Set the body's
   `textureSaturation` (shape offset 0x544) to **1 when the ability can be used now**
   (0x43e5a0: mana, cooldown and state checks) and **0 otherwise** (grey icon) (0x48bfcd …
   0x48c071).
3. *Cooldown.* If the player's cooldown table (+0x139c) has an entry for the id and the
   ability's cooldown length (0x43e6a0) is non-zero: show `cooldown`, reset its deformation and
   scale y by *c* = max(0, remaining ÷ length); otherwise hide it (0x48bdea … 0x48bfa8). At
   start-up the overlay is pre-flattened (y scale 0).
4. *Quick item.* `count` = the stack size of the selected quick item (§5.7; " " when there is
   none) (0x491f16 … 0x49226f).
5. *Highlight* as in the table above.

**Ability icon table** (id → file in `data3.db`, 0x45ffxx): 0 none, 1 attack-right, 2
attack-left, 3 stab-right, 4 stab-left, 5 perforate, 6 punch-right, 7 punch-left, 8 block,
9 shield-attack, 0xa shield-slam, 0xb swirl, 0xc slash, 0xd slash-left, 0xe slash-right,
0x10 mutilate, 0x11 ambush, 0x12 pierce-left, 0x13 pierce-right, 0x14 kick, 0x15 ranger-kick,
0x16 shoot, 0x17 shoot-fast, 0x18 charge-shoot, 0x19 salvo, 0x1a boomerang, 0x1b
charge-boomerang, 0x1c beam, 0x1e fire-shock, 0x1f fire-burst, 0x20 water-shock, 0x21
water-burst, 0x22 healing-stream, 0x24 charge-bullet, 0x25 fireball, 0x26–0x28 firebolt,
0x29/0x2a/0x2c waterbolt, 0x2b waterball, 0x2d water-salvo, 0x2e fire-salvo, 0x30 intercept,
0x31 teleport, 0x32 retreat2, 0x36 smash, 0x37 salvo, 0x39 slam, 0x3a slam-back, 0x3b
charge-slam, 0x3f swirl, 0x40 charge-mutilate, 0x41 slam-left, 0x42 slam-right, 0x43 jab,
0x4f sneak, 0x56 cyclone, 0x58 fire-explosion, 0x5f beam, 0x60 shuriken, 0x61 camouflage,
0x63 aim, 0x64 swiftness, 0x65 bulwark, 0x66 warfrenzy, 0x67 mana-shield (all `.png`). Two
further tables: skills 0 pet-taming, 1 pet-riding, 2 climbing, 3 hang-gliding, 4 swimming,
5 ship-driving (G+0x800828); specializations mage-fire/mage-water, warrior-berserker/
warrior-guardian, ranger-sniper/ranger-scout, rogue-ninja/rogue-assassin (G+0x800830).

**Worked example (1280 × 720).** Button corners at x = 478, 526, 574, 622, 670, 718, 766,
y = 670; each spans 42 px. Label "M1" baseline at (478, 674). An ability with 1500 ms of a
4000 ms cooldown left: *c* = 0.375, overlay x 479 … 519, y 696 … 711. Holding key 2 tints the
frame of the button at x = 622.

### 5.6 Floating combat text (F-8)

A list of floating texts (G+0x800738) is drawn every frame (0x49584d … 0x495e7a): the
container G+0x800750 is emptied, and for each entry two text nodes are made and placed; the
entry's age grows by the frame time and the entry is removed once age > lifetime.

**Entry fields:** age (ms, starts 0), rise (px, default −100), world anchor, screen offset
(px), colour, text, lifetime (ms, default 1000), size (px), alignment flags (§3.4).

**Drawing**, with *t* = age ÷ lifetime:

* Project the world anchor with the game camera; behind the camera → skip this frame.
* Origin = projected point + screen offset + (0, rise·*t*).
* Outline node: fill transparent, outline colour = colour × (0.1, 0.1, 0.1, 1 − *t*), outline
  radius = size × 0.125. Fill node on top: colour × (1, 1, 1, 1 − *t*), no outline. Font
  `resource1.dat`, the entry's size and flags.

**Producers:**

| Event | Text | Colour | Size | Rise | Offset | Life | Flags |
|---|---|---|---|---:|---|---:|---:|
| hit where the local player is attacker or target (types 4 and 6 skipped; also skipped when \|damage\| < 0.1 and the type is 0) (0x494e0e …) | "BLOCK" (type 1), "MISS" (3), "ABSORB" (5); else \|damage\|: below 1 with one decimal (trunc(x·10)·0.1), else truncated integer | heal (damage < 0): (0.4, 1, 0.5); damage to the local player: (1, 0.3, 0.3); hit flag at +0x44 set: yellow (1, 1, 0.25); otherwise white | 20; 60 when the hit flag at +0x14 is set *(believed: critical)* | +100 (sinks) when the local player is the target; else −100; −200 with the +0x14 flag | (0, 0) | 1000 | 1 (centred) |
| level up (0x493d84) | "LEVEL UP!" | (0.8, 0.6, 1) | 50 | −100 | (0, −250) | 3000 | 2 |
| XP gained (0x493ed5) | "XP +" + gain | (0.8, 0.6, 1) | 30 | −100 | (0, −200) | 3000 | 2 |

The world anchor for hits is the hit position raised by about 2 world units *(inferred; the
exact vertical term involving the target's size, 0x494ec7 … 0x494f53, is open, M-3)*; for
level and XP it is the player's position. Flags 2 means right-aligned per §3.4 *(this looks
odd for a centred banner; settle by capture, M-4)*.

**Worked example.** A white hit of 23.6 on a creature: text "23", size 20, outline radius 2.5.
At age 250 ms: *t* = 0.25, alpha 0.75, the origin is 25 px above the projected point; at
1000 ms it reaches 100 px above with alpha 0 and is removed on the next frame. A 0.46 hit shows
"0.4".

### 5.7 Quick-item wheel (Tab) (resolves S-2)

**Tab** toggles G+0x800a40 (0x47e7ae). While it is open:

* Candidate items = the usable items from the inventory (0x47ae10); the selected index
  G+0x800a44 is taken modulo the count (negative values wrap).
* **A** or a leftward mouse movement steps the index −1, **D** or rightward +1, each step with
  sound 0x55 and a 200 ms repeat delay; **W** or **S** close the wheel (0x497952 … 0x497b06).
* **R** or **E** uses the selected item if its level requirement ≤ the player's level and closes
  the wheel; otherwise plays sound 0x32 (0x497db6 … 0x497e7b).
* The frame draws each candidate's 3D item model at scale 0.09, lit as in the §5.1.4 note,
  on a circle round the window centre (0x4bac91 … 0x4bb031): radius *R* (G+0x800a48) eases
  towards 200 px and the turn *a* (G+0x800a4c, degrees) eases towards −360·index ÷ count, both
  with factor 1 − 0.99^ms per frame (rate 0.01 per millisecond, 0x4ac150). Item *i* of *n* is at
  angle θ = 2π·*i*/*n* + *a*·π/180 − π/2 and position (W/2 + *R*·cos θ, H/2 + *R*·sin θ).
  Closed → *R* is reset to 0.
* The `selector` node (pivot at (W/2, H/2 − 200)) is visible exactly while the wheel is open;
  its `name` (origin (W/2, H/2 − 120), 16 px, centred, white, outline 4) = item name + " +" +
  the item's level value, its `count` (origin (W/2 + 57.5, H/2 − 170), 22 px, right-aligned) =
  stack size (0x497b36 … 0x497d91).

**Worked example (1280 × 720).** 4 items, index 1, settled: *a* = −90°; item 0 at
θ = −180° → (440, 360); item 1 at θ = −90° → (640, 160), i.e. the selected item sits at the top.

### 5.8 Crosshair, interaction prompt and death (resolves F-11)

* `crosshair` (pivot at (W/2, H/2)) is visible when no window/menu has input, the player is not
  aiming (player flag 0x400 clear) and Left Shift is not held; `zoomcrosshair` is visible while
  flag 0x400 is set (0x4927da … 0x492975).
* The **interaction prompt** is the crosshair's `name` text: origin (W/2, H/2 + 33.25), 12 px,
  centred, white, black outline 2.5. Every frame it is first cleared, then set (0x496890 …):
  "[R] Revive" while the player's HP ≤ 0; otherwise the prompt for the creature or object under
  the interaction target ("[R] Talk", "[R] Ride", "[R] Open", … §5 list).
* **Death.** There is no death screen, overlay, fade or message: the HUD stays as it is, the
  bars show 0, and the prompt reads "[R] Revive". While HP ≤ 0 the interaction target is cleared
  (0x499545). Pressing **R** (edge-triggered, and not while the quick-item wheel is open) with
  HP ≤ 0 (0x49736c … 0x4973df): resets the player's transient state (0x447110: stamina to 1,
  stealth, cooldowns and timers to 0), moves the player to a respawn point found by searching the
  3 × 3 zones around the current zone (0x5a03d0) *(believed: the nearest spawn point)*, and sets
  HP to max HP.

**Worked example.** 1280 × 720, HP 0: "[R] Revive" centred on (640, 393.25).

### 5.9 Land-name banner timing

When the land name or detail text changes (0x49434d …), `landname`'s `name` and `detail` are
set and a timer *T* = 8000 ms starts; each frame *T* decreases by the frame time (not below
0). While *T* > 0 the banner is visible with display fill alpha 1 − (2·(*T*/8000 − 0.5))²
(0 at the start and end, 1 at the middle, 4 s in); at *T* = 0 it is hidden. Example:
*T* = 6000 → alpha 0.75. The code-made labels `landscape` / `landscapedetail` (G+0x800858,
20 px, white, black outline 3, at (20, 45)) and the "TIME"/"TEMP" readout (G+0x800860, placed at
(W − 15, 20)) are also written every frame; whether they are visible in normal play is open
(M-5).

### 5.9.1 Land names

Where the banner's two texts come from, how the names in them are made, and what the
code-made labels show. Routines: banner update 0x494225 … 0x494464 inside the frame routine
0x488ee0; land name 0x4e5320; location name 0x4e5590; detail 0x4e5c10 / 0x4e5a20; name maker
0x5a0ed0; the two key tables are filled by the world constructor 0x58eb00 (0x592a20 …
0x593206). Worldgen terms (region cell, owner, location record, per-zone table entry,
f(L, X, Y), bump) are those of [alpha-worldgen-regions.md](alpha-worldgen-regions.md) and
[alpha-worldgen-locations.md](alpha-worldgen-locations.md).

**Is this world generation?** Only partly. The name maker uses **no `srand` and no `rand()`
at all**: a name is a fixed function of one 32-bit number (below). The seed enters only
through the numbers world generation stores: a region cell's id (= 1024·ry + rx + A, regions
spec §2.1, so it depends on the seed via A), and a location record's serial or id (drawn
during region preparation, locations spec §6). Everything else here is interface code that
reads the finished world each frame.

#### Inputs each frame

* The player is the local creature at G+0x8006d0; its position is the 64-bit fixed-point
  pair at +0x10 (x) and +0x18 (y), 1/65536 block. The block column is
  (x, y) = (fx_x / 65536, fx_y / 65536), 64-bit division rounding **towards zero**, low 32
  bits kept.
* The world is W = G+0x2e4. The dictionary object is G+0x314 (loaded from `data4.db`
  `dict_en.xml`, or `dict_de.xml` for German): its map at +0x0c holds, for every
  `<landscape key="K">`, the text of its `<normal>` child; its map at +0x04 holds, for every
  `<name key="K">`, its children by element name (here only `<singular>` is read). A missing
  key or child reads as the empty string.
* **name** = the land name of (x, y) (0x4e5320, below); **detail** = the zone detail of
  (x, y) (0x4e5c10, below). Both are recomputed every frame.

#### When it counts as "changed" (0x49434d … 0x494464)

Two remembered strings, *shownName* and *shownDetail*, start empty (function statics,
0x76b11c and 0x76b104). Each frame:

1. If name ≠ shownName **or** detail ≠ shownDetail (exact UTF-16 comparison, 0x4514e0):
   * if *T* = 0: shownName ← name, shownDetail ← detail, `landname`'s `name` ← name and
     `detail` ← detail, *T* ← 8000 (the fade of §5.9 starts this frame at alpha 0);
   * if *T* > 0: nothing is copied; the running banner just continues (§5.9).
2. Otherwise: if *T* > 0 the banner continues; if *T* = 0 the banner is hidden.

So a change while a banner is showing is not lost: the remembered strings still differ when
*T* reaches 0, and the next frame starts a new banner with the texts current at that moment.
On the first frame of play the banner appears if either text is non-empty. An empty name or
detail is shown as an empty line; it is not skipped.

#### The land name (0x4e5320)

1. **Location test.** Look up the location record of sub-cell (lx, ly) =
   (x / 2048, y / 2048) (rounded towards zero; regions spec §6.3; needs 0 ≤ lx, ly < 8192 and
   the region object present). If the record exists, its type (+0x18) is neither 0 nor 10,
   and w = bump(f(L, x << 16, y << 16)) (= max(1 − f, 0)², 0x5fa4c0; the position is the
   block's corner in fx, 64-bit) satisfies **w ≥ 0.2** (float constant 0x6fcdbc =
   0.20000000298023224; the code skips when 0.2 > w): name = the **location name** of L
   (below). With f = d²/r² this is "within about 0.744·r of the (wobbled) centre".
   Type-10 sites never give a location name.
2. **Region name** otherwise. C = owner(x, y) (0x477e10, regions spec §3.4: the displaced
   nearest region cell). If there is none: name = "".
   * pattern = landscape["Lands of"]; if C's elevation (+0x18) < 0, pattern =
     landscape["Ocean"] instead. With `dict_en.xml` these are "Lands of @" and "@ Ocean".
   * If pattern is empty: name = "". Find the first "@" (0x4d9950); if there is none:
     name = "" (unlike the two cases below).
   * name = pattern with that one "@" replaced by **N(C.id)** (cell +0x14; N below).

   So land regions read "Lands of *N*" and sea regions "*N* Ocean", one name per region cell.

#### Location names (0x4e5590)

1. key = W's **location-key table** at W+0x800124, looked up by the exact pair
   (type, variant) = (L+0x18, L+0x1c) (0x5a5240, a sorted map; no match → "").
2. pattern = name[key]["singular"]; if empty, name = "".
3. If the pattern has an "@", the first one is replaced by **N(L+0x20)** (the record's
   serial, or its id for a town); without "@" the pattern is used as it is.

Location-key table (0x592a20 … 0x593064; pattern = `dict_en.xml` `<name>` `<singular>`):

| (type, variant) | key | English pattern |
|---|---|---|
| (1, 0) | Village | "@ City" |
| (2, 0) | Mountain | "@ Mountains" |
| (3, 0) | Forest | "@ Forest" |
| (4, 0) | Lake | "@ Lake" |
| (5, 0), (5, 1), (5, 2), (5, 3) | Ruins | "Ruins of @" |
| (5, 4) | Gravesite | "Gravesite of @" |
| (6, 0) | Canyon | "@ Canyon" |
| (7, 0) | Valley | "@ Valley" |
| (8, 0) | Crater | "@ Crater" |
| (9, 0) | Cave | "@ Cave" (the file also has an earlier `<name key="Cave">` with singular "Cave"; which one the loader keeps is not traced — type 9 is never created, so it does not matter) |
| (10, 0) | Portal | "Portal of @" (never reached: type 10 is excluded in step 1 above) |
| (11, 0) | Rock | "Rock of @" |
| (12, 0) | Tree | "@ Tree" |
| (13, 0) | Peak | "@ Peak" |
| (14, 0) | Castle | "@ Castle" |
| (14, 1) | Ruins | "Ruins of @" |
| (14, 2) | Catacombs | "Catacombs of @" |
| (14, 3) | Palace | "@ Palace" |
| (14, 4) | Temple | "Temple of @" |
| (14, 5) | Pyramid | "@ Pyramid" |
| (15, 0) | Island | "@ Island" |

Consequences of the exact (type, variant) match, as the code is written:

* A **town** (type 1) has variant = the region's style 0..5 (locations spec §1.2, §4.1); only
  style 0 has an entry, so only style-0 towns are named "*N* City" (N of the town's id = its
  region cell's id, i.e. the same *N* as "Lands of *N*"). Inside a town of style 1..5 the
  name is **""**. An **oasis** (type 3) is "*N* Forest" only for variant 0; variants 1 and 2
  give "". (Worth confirming in a recording; nothing in the code fills the gap.)
* Type-5 variants are 0, 2 or 3 in fresh worlds ("Ruins of *N*"); (5, 4) "Gravesite" is not
  produced by region preparation. Dungeon entrances (type 14) use E.variant 0..5.
* Types 8, 9 and 13 are never created by region preparation, so "Crater", "Cave" and "Peak"
  do not appear in fresh worlds.

#### The detail (0x4e5c10, 0x4e5a20)

1. Zone (zx, zy) = (x / 256, y / 256) (towards zero); needs 0 ≤ zx, zy < 65536 and the
   region object (zx / 64, zy / 64) present (0x4a6ad0, 0x434a90). E = that region's per-zone
   table entry for (zx mod 64, zy mod 64) (locations spec §1.1). If there is none: detail = "".
2. key = W's **zone-key table** at W+0x80012c, exact pair (E.kind, E.variant) (bytes +0, +1;
   0x59fe70 → 0x5a64b0). Entries (0x5930ab … 0x5931ff):

   | (kind, variant) | key | English pattern (`<name>` `<singular>`) |
   |---|---|---|
   | (1, 1) | Trade Quarter | "Trade District" |
   | (1, 2) | Crafting Quarter | "Crafting District" |
   | (1, 3) | Class Quarter | "Adventurer District" |
   | (1, 4) | Pet Quarter | "Pet District" |
   | (4, 0) | Portal | "Portal of @" |
   | (5, 0) | Palace | "@ Palace" |

   No match → detail = "". (Dungeon entrances, kind 3, have no entry, so their zones show no
   detail; the record's own location name covers them.)
3. pattern = name[key]["singular"]; empty → "". No "@" → detail = pattern.
4. With an "@": if E.id (+4) ≠ 0 and E.kind ≠ 0, the "@" becomes **N(E.id)**; otherwise it
   becomes the **grid name** of (x, y) (0x5a6550, below).

In fresh worlds the detail is therefore "Trade District", "Crafting District", "Adventurer
District" or "Pet District" in the four town-quarter zones, "Portal of *N*(site id)" in the
zone of a type-10 site (its zone entry has kind 4 and the site's id, locations spec §7), and
"" everywhere else. Kind 5 is never written, so the grid name is not reached.

#### The name maker N(n) (0x5a0ed0 with kind −1)

0x5a0ed0 takes (n, kind). Every land-name caller passes kind = −1, which selects the
default pair of 20-entry tables (0x76b808 and 0x76b9f0); other kinds select other table pairs
(used for creature titles, §5 enemy labels) and are not part of this section. With n an
unsigned 32-bit number and all arithmetic modulo 2³² (products and sums wrap; "/" is
unsigned division rounding down):

* i = (n / 7 + 13·n) mod 2³² mod 20;
* j = (7·n + n / 10) mod 2³² mod 20;
* **N(n) = First[i] followed by Second[j]**, no separator, no change of letter case (First
  entries are capitalised, Second entries are lower case). Length is simply the sum of the
  two syllable lengths (3 to 8 characters).

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

The tables are built in the executable (lazy statics), not read from the dictionary.

**Test vectors of N** (n → i, j → name):

| n | i | j | N(n) | where it comes from (seed 123, A = 440) |
|---:|---:|---:|---|---|
| 525240 | 14 | 4 | Duraron | home cell (512, 512) id |
| 525241 | 7 | 11 | Likugar | cell (513, 512) id |
| 526264 | 12 | 14 | Galan | cell (512, 513) id |
| 524215 | 2 | 6 | Askor | cell (511, 511) id |
| 274 | 1 | 5 | Aruion | home region record 28, type 5 serial |
| 692 | 14 | 13 | Duradara | home region record 17, type 14 serial |
| 25782 | 9 | 12 | Vardar | home region record 12, type-10 id |
| 0 | 0 | 0 | Krono | — |
| 4294967295 | 3 | 2 | Gela | wrap check: 13·n and 7·n wrap modulo 2³² |

Worked example, n = 525240: n / 7 = 75034, 13·n = 6828120, sum 6903154 → i = 14 "Dura";
7·n = 3676680, n / 10 = 52524, sum 3729204 → j = 4 "ron"; N = "Duraron".

**Full-text examples** (seed 123, named world, English; cells and records from the regions
spec §2.3 and the locations spec §12.1):

* Column (8396928, 8396928) (the home point): sub-cell (4100, 4100) holds record 36, the town
  (type 1), centre 8397824 blocks, r = 516: f > 1 there, so w = 0 and the region name is used.
  The owner is the home cell (e = 74 > 0): **name = "Lands of Duraron"**. Its zone
  (32800, 32800) is local (32, 32), not one of the town quarters: **detail = ""**.
* Inside cell (513, 512) (e = −100): **"Likugar Ocean"**.
* Near the centre of home-region record 28 (type 5, variant 0, serial 274): **"Ruins of
  Aruion"**; record 17 (type 14, variant 0, serial 692): **"Duradara Castle"**; record 5
  (type 12, serial 764): "Aruron Tree"; record 6 (type 4, serial 901): "Arurior Lake";
  record 27 (type 2, serial 641): "Kurgar Mountains".
* In the home town (style 3, so variant 3): name **""**; in its quarter zones detail "Trade
  District" etc. The zone of record 12 (type-10 id 25782): detail **"Portal of Vardar"**.

#### The grid name (0x5a6550; not reached in fresh worlds)

For completeness: for 0 ≤ x, y < 2²⁴ it takes the grid coordinates G(x, y) of the regions
spec §5.3 (truncated to integers gx, gy, 0x58d6c0) and returns First2[(3·gy + gx + W[0x80028c])
mod 20] + Second2[(3·gx + gy + W[0x800290]) mod 20] (signed remainder), with
First2 (0x76d9d8) = Hy, Ty, Tri, Ge, Go, Lu, Der, Ter, Sel, Ko, Am, Ka, Kel, Ko, Sa, Mo, Ru,
Al, Ta, Har and Second2 (0x76dbc0) = ra, mala, ura, el, al, mer, kun, la, mur, gu, la, go, sel,
rion, remo, antis, uro, lic, reon, tuma. W+0x80028c / +0x800290 are set by 0x5a52e0 (not
traced). Outside the range it returns "".

#### The code-made labels (0x494161 … 0x494885)

* **`landscape` / `landscapedetail`** (G+0x800858): every frame the node is placed at
  (W − 260, 90) and its child `landscape` is given the **name** and `landscapedetail` the
  **detail**, the same two strings as the banner (no extra formatting).
* **"TIME"/"TEMP" readout** (G+0x800860, placed at (W − 15, 20)): its child `info` gets, every
  frame, the text
  "TIME " + *h* + ":" + *mm* + "  TEMP " + *t* + " °C  HUM " + *u* + "%"
  (two spaces before "TEMP" and "HUM"), where, with *ms* = the world's time of day in
  milliseconds (W+0x80015c) and the player's block column (x, y):
  * *h* = *ms* / 3600000 (integer, no padding); *mm* = (*ms* / 60000) mod 60, written with
    two digits, padded with "0";
  * *t* = −20 − trunc(f32(warmth(x, y) · −60)) = trunc(60·warmth) − 20 (warmth blend 0x5c4dd0,
    regions spec §4), so −20 … +40 °C;
  * *u* = trunc(f32(moisture(x, y) · 100)) (moisture blend 0x5c4800), 0 … 100.

  Example: *ms* = 49 500 000 (13:45), warmth 0.5601367, moisture 0.4915098 →
  "TIME 13:45  TEMP 13 °C  HUM 49%".
* Whether either node is visible in normal play is still open (M-5): both are children of the
  common HUD parent G+0x800884 and the frame code writes them unconditionally; the texts
  above are what they would show.

---

## 6. Options window (OptionsWidget, 0x4cf3c0, draw 0x4d0230)

A fixed 450 × 440 panel, centred in the window, with eleven rows of "label — ‹ value ›" and
three buttons along the bottom. Every value is changed only by clicking the small left/right
arrows of its row; there are no sliders to drag, no mouse-wheel handling and no keys of its own.
The window edits a private copy of the settings; **Apply** and **OK** hand that copy to the game,
which takes every value into use at once (nothing needs a restart) and rewrites `options.cfg`.

All coordinates in §6.2–§6.5 are **window-local pixels**: x to the right and y downwards from
the panel's top-left corner, as for the start menu (§3.2–§3.3). Text positions are text origins
in the sense of §3.4 (first baseline; centred text is centred on its origin).

### 6.1 Building, opening and closing

**Construction** (start-up, 0x459c40 at 0x4648aa … 0x4649fb):

1. The panel is a clone of the `blackwidget` template (G+0x8008c8), made under the in-game
   root (G+0x800884) and stored at G+0x800a00. Its widget is resized to **450 × 440** with the
   widget-stretching rule (`alpha-ui-plasma.md` §5.5), exactly as the system panel is resized to
   150 × 120 (§3.3); so it has the same look: 5-pixel notched corners, black fill at alpha 0.67,
   a 3-pixel black outer stroke and a 2-pixel white inner outline. The clone is then hidden.
2. An OptionsWidget (0x4cf3c0, G+0x8009fc) is made with an empty node of its own and bound to the
   clone's root (0x631460 with "fit" set), so the widget covers the panel exactly: its rectangle
   is (0, 0) … (450, 440) in panel space and its draw routine writes the text over the panel.
3. The constructor clones, as children of the widget's node, one `leftbutton` and one
   `rightbutton` per row (22 arrows, +0x170 … +0x1c4, row by row, left first) and three `button2`
   clones (+0x1c8 Apply, +0x1cc OK, +0x1d0 Cancel). Each `button2` clone's widget is resized to
   **110 × 15** and its `text` nodes (both the outlined and the plain copy) are set to "Apply",
   "OK" and "Cancel". Callbacks are attached to the "press" event (§6.4).
4. The settings copy (12 integers at +0x1d4, same order as the game's block, §6.6) is filled from
   the settings in force at start-up, and the resolution list (+0x160, §6.5) is filled from the
   display modes (0x4d4de0, the only time this happens).

**Placement.** The layout routine (0x482a40, run whenever the window size changes) puts the
panel's top-left at **x = (W − 450) / 2, y = (H − 440) / 2** (floating point, not rounded; the
text and the panel's edges are then subject to the normal pixel snapping of §3.4).

**Parent.** On the start screen the panel node is moved under the start-screen root each update;
in the world it is moved under the in-game root (§3.3). On the start screen the `cubeworld` logo
and the start menu are hidden for as long as the panel is visible (§3.3).

**Opening** (sets the panel's `visibility` to 1; no sound of its own):

* start menu: a left-button **press** on "Options" (StartMenuWidget hovered index 1, 0x47b6db,
  in the press handler 0x47b600); the start menu's click sound is §3.2's;
* system panel: a left-button **release** on "Options" (SystemWidget index 0, 0x47e16f, in the
  release handler 0x47ddd0); the system panel is hidden and the Esc row flag G+0x8008f2 cleared
  first (§7).

Opening does **not** reload the widget's copy from the game: whatever the copy held when the
window was last closed is shown again (see Cancel, §6.4).

**Closing:** only by **OK** or **Cancel** (both set the visibility of the panel node — the
nearest ancestor of the widget that carries a widget, 0x62b400 — to 0). No key closes it: the
game's key handler (§11.2) does not look at the options window, so Esc, B, O … keep their normal
meaning while it is open, and right-clicking empty space (§7) does not close it either.

**While open in the world** the game is in cursor mode (the "window open" test 0x47f1d0 includes
G+0x800a00, 0x47f38d), so the cursor is shown and the camera does not follow the mouse.

### 6.2 Rows: labels and values

Drawn every frame by the draw routine 0x4d0230 (vtable slot 1). Each row *k* = 0 … 10 has its
text baseline at **y = 27 + 30·k**. Every label and every value is drawn twice at the same pen
position with `resource1.dat`, **12 px**:

1. outline pass: outline radius **3**, outline colour black (0, 0, 0, 1), glyph colour white;
2. fill pass: no outline, glyph colour white (1, 1, 1, 1).

So all text is white with a 3-pixel black outline; nothing changes colour on hover.

* **Label:** origin x = **15**, left-aligned (flags 0).
* **Value:** origin x = (450 − 250) × 0.5 + 240 = **340**, centred (flags 1). (The routine
  computes it from the widget's width, (width − 250)/2 + 240; the width is always 450.)

| *k* | Baseline y | Label | Value text | `options.cfg` key |
|---:|---:|---|---|---|
| 0 | 27 | "Mode" | "Windowed" if `fullscreen` = 0, else "Fullscreen" | `fullscreen` |
| 1 | 57 | "Resolution" | *w* " x " *h* of the selected list entry (§6.5), e.g. "1280 x 720"; empty if the list is empty | `resolutionX`, `resolutionY` |
| 2 | 87 | "Anti-aliasing" | "Disabled" if `antiAliasing` < 1, else 2·`antiAliasing` followed by "x" ("2x", "4x") | `antiAliasing` |
| 3 | 117 | "Render Distance" | *n* "%" (e.g. "50%") | `renderDistance` |
| 4 | 147 | "Sound FX Volume" | *n* "%" | `soundVolume` |
| 5 | 177 | "Music Volume" | *n* "%" | `musicVolume` |
| 6 | 207 | "Camera Speed" | *n* "%" | `cameraSpeed` |
| 7 | 237 | "Camera Smoothness" | *n* "%" | `cameraSmoothness` |
| 8 | 267 | "Invert Y Axis" | "Yes" if `invertY` ≠ 0, else "No" | `invertY` |
| 9 | 297 | "FPS Limit" | "none" if `minTimeStep` < 1, else ⌊1000 / `minTimeStep`⌋ followed by " fps" (16 → "62 fps", 9 → "111 fps", 50 → "20 fps") | `minTimeStep` |
| 10 | 327 | "Language" | the number itself (e.g. "0", or "-1") | `language` |

Numbers are written as plain decimal integers (wide-string stream, no padding, "-" for
negatives).

### 6.3 Arrows

**Template.** `leftbutton` / `rightbutton` (`gui.plx`), used at their file size. Each is a
**19 × 21** `Button` widget. In box-local pixels (origin at the widget rectangle's top-left,
= shape coordinates − (24.18, −95.82)):

* `leftbutton`: a white arrow pointing left, filled, no stroke — three quads
  (11, 1)–(18, 0)–(18, 21)–(11, 20), (6, 4)–(11, 1)–(11, 20)–(6, 17) and
  (0, 7)–(6, 4)–(6, 17)–(0, 14) (vertices rounded to 0.1 px here; exact values in the file) —
  under its child `border`, a 12-corner outline (0, 6), (5, 6), (5, 3), (10, 3), (10, 0),
  (19, 0), (19, 21), (10, 21), (10, 18), (5, 18), (5, 15), (0, 15) drawn as a 2-pixel black
  stroke without fill;
* `rightbutton`: the mirror image (tip at x = 19, outline (0, 0), (9, 0), (9, 3), (14, 3),
  (14, 6), (19, 6), (19, 15), (14, 15), (14, 18), (9, 18), (9, 21), (0, 21)).

**Placement** (layout routine 0x4d46d0, vtable slot 10, run when the widget is sized): for row
*k* the arrow rectangles' top-left corners are

* left arrow: **(240, 12 + 30·k)**;
* right arrow: **(width − 19 − 15, 12 + 30·k) = (416, 12 + 30·k)**.

So each arrow spans 21 pixels vertically, from 15 pixels above the row's baseline to 6 below
it, and the value text sits centred between the arrows (left arrow 240 … 259, right arrow
416 … 435, value centre 340).

**Hover.** Each frame, for all 22 arrows, the arrow's `Display.fillColor` is set to
**(0, 1, 1, 1)** while the cursor is over it (0x629300) and to (1, 1, 1, 1) otherwise. Since the
fill colour multiplies the subtree's colours, a hovered arrow turns **cyan** (the black border
stays black). There is no pressed look (the templates have no `button:*` sequences, S-4).

### 6.4 Mouse and keyboard

* **Click = left-button press.** The interface fires a button's "press" event (event 2,
  0x6527f0) when the left button goes **down** over it; every arrow and button acts at that
  moment, once. Release does nothing. Holding the button does not repeat: the button class's
  auto-repeat event (0x665b60: first repeat after 650 ms, then every 140, 130, … down to 50 ms)
  is not connected for the options window.
* No sound is played by any arrow or by Apply/OK/Cancel.
* No dragging, no wheel, no keyboard input, no keyboard focus.
* The Mode row's left arrow also writes the word "click " to the program's standard output
  (debug leftover, invisible to the player; the right arrow does not).

**What each arrow does** to the window's copy (nothing reaches the game until Apply/OK). Each
arrow clamps only at its own end of the range:

| Row | Left arrow | Right arrow | Range reachable with the arrows |
|---|---|---|---|
| Mode | `fullscreen` = (`fullscreen` + 1) mod 2 | same (toggle) | 0, 1 |
| Resolution | index − 1, not below 0 | index + 1, not above count − 1 | the list (§6.5); then `resolutionX`, `resolutionY` = entry (index mod count) |
| Anti-aliasing | − 1, not below 0 | + 1, not above 2 | 0, 1, 2 ("Disabled", "2x", "4x") |
| Render Distance | − 5, not below 25 | + 5, not above 100 | 25 … 100 in 5s |
| Sound FX Volume | − 10, not below 0 | + 10, not above 100 | 0 … 100 in 10s |
| Music Volume | − 10, not below 0 | + 10, not above 100 | 0 … 100 in 10s |
| Camera Speed | − 5, not below 10 | + 5, not above 100 | 10 … 100 in 5s |
| Camera Smoothness | − 5, not below 0 | + 5, not above 100 | 0 … 100 in 5s |
| Invert Y Axis | `invertY` = 1 if it was 0, else 0 | same (toggle) | 0, 1 |
| FPS Limit | `minTimeStep` **+ 1**, not above 50 (lower limit) | `minTimeStep` **− 1**, not below 0 (higher limit) | 0 ("none") … 50 ("20 fps") |
| Language | − 1 (no limit) | + 1 (no limit) | any integer |

Values loaded from `options.cfg` are not range-checked; a value outside the range stays as it
is until an arrow moves it (for example `renderDistance 7` shows "7%", and one right click gives
12).

**Buttons** (`button2` clones, 110 × 15, §6.5):

* **Apply** (0x4d4470): hands the copy to the game (0x46f390, below). The window stays open.
* **OK** (0x4d4650): the same, then closes the window.
* **Cancel** (0x4d4510): closes the window only. It does **not** undo the edits in the window's
  copy: they are shown again the next time the window opens and are applied by the next Apply
  or OK. (Faithful behaviour; an engine may choose to reset the copy on open, but Alpha does
  not.)

**Applying** (0x46f390, on the game object G, under the game's lock at G+0x2e4):

1. the 12 values are copied into the game's block at G+0x170;
2. `options.cfg` is rewritten from that block (0x4cef80, §6.6);
3. the music volume is set at once: music player (G+0x800714) volume = `musicVolume` × 0.01.

The other values take effect from the game's block without any further step:

| Value | Taken into use |
|---|---|
| `fullscreen`, `resolutionX/Y`, `antiAliasing` | next frame: 0x4c85f0 sees that one of these four differs from the start-up copy (0x76b1d8), copies the whole block there and resets the Direct3D device (0x4c8940). Fullscreen: back buffer = `resolutionX` × `resolutionY`. Windowed: back buffer = the window's client size (the resolution is ignored and the window is not resized). Multisampling = 2·`antiAliasing` samples (0 = off), quality 0. The same reset also runs on every WM_SIZE. |
| `renderDistance` | read live by the world worker (0x4690a0): a radius of `renderDistance` × 0.01 × 0x1800000 world units (its use is `alpha-rendering.md` question 12) |
| `soundVolume` | read each time a sound is played (0x484350: volume × `soundVolume` × 0.01) |
| `cameraSpeed`, `cameraSmoothness`, `invertY` | read live by the mouse look (`alpha-physics-camera.md` §7.2) |
| `minTimeStep` | read live by the main loop: after each frame it sleeps until the frame has lasted `minTimeStep` ms (0x4c8ae0, timer resolution 1 ms); 0 = no limit |
| `language` | stored and saved only: no reader was found, and the dictionary is always `dict_en.xml` (the only dictionary name in the executable, pushed at 0x4e1b3e). §0's remark that `language` selects the dictionary is therefore not borne out. |

So **nothing needs a restart**.

### 6.5 Buttons and the resolution list

**Button look** (`button2` resized to 110 × 15; every vertex of its rectangles lies on the
edges of its 100 × 100 bind rectangle, so the stretch simply gives a 110 × 15 rectangle):

* body: a rectangle with transparent fill (all vertex colours alpha 0) and a **3-pixel black
  stroke**, alignment 1 (outside);
* `Shape`: the same rectangle, no fill, **2-pixel white stroke**, alignment −1 (inside);
* `text`: "Apply" / "OK" / "Cancel", `resource1.dat` **14 px**, white, drawn as an outlined copy
  (3-pixel black outline) and a plain copy, text flags 5 (centred horizontally, block centred
  vertically, §3.4). In the template its origin is 0.17 px left of and 3.5 px above the button's
  centre; its widget has horizontal and vertical alignment 2, which is read as "keep centred",
  so the origin stays at the button centre + (−0.17, −3.5) after resizing (inferred; check by
  capture);
* `clip` / `Shape`: a white (alpha 0.32) 160 × 30 highlight with a cyan stroke, entirely outside
  its own mask in the template, so it never shows.

There is no hover or press feedback on these buttons (no sequences in the template, S-4).

**Placement** (0x4d46d0; *w* = 110 and *h* = 15 the button size, 450 × 440 the panel):

| Button | Top-left x | Top-left y | Rectangle |
|---|---|---|---|
| Apply | 20 | 440 − 15 − 20 = 405 | (20, 405) … (130, 420) |
| OK | (450 − 40 − 110) / 2 + 20 = 170 | 405 | (170, 405) … (280, 420) |
| Cancel | 450 − 20 − 110 = 320 | 405 | (320, 405) … (430, 420) |

**Resolution list** (built once at start-up, 0x4c8ae0): every display mode Direct3D 9 reports
for adapter 0 in format X8R8G8B8 (GetAdapterModeCount / EnumAdapterModes), in the order
reported, keeping only the first mode of each distinct width × height (refresh rates are
ignored). The list is not refreshed later. When the window's copy is filled (0x4d4de0) the index
is set to the entry equal to the current `resolutionX` × `resolutionY`, or to **0** if there is
none — and the copy's resolution is then replaced by that entry, so an unlisted resolution
(for example the desktop size in windowed mode) becomes the list's first entry the next time
Apply is pressed.

### 6.6 `options.cfg` and the defaults

**Settings block** (12 × i32; start-up copy 0x76b1d8, game copy G+0x170, window copy
OptionsWidget+0x1d4), in this order, with the **defaults** used when `options.cfg` is missing
or lacks a key (0x4ce660 constructor; resolution from GetSystemMetrics at 0x4c8d9e):

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

(The observed file in `alpha-saves.md` §7 holds the user's own values, e.g. smoothness 100.)

**Reading** (0x4ce6e0, once at start-up before the game object exists): the file
`options.cfg` in the working directory is read as whitespace-separated tokens; each token is
compared with the twelve key names (case-sensitive) and, on a match, the next token is read as a
decimal integer into that field. Unknown tokens are skipped one at a time. Reading stops at the
end of the file or at the first token that fails to parse. A missing file leaves the defaults.

**Writing** (0x4cef80, only from Apply/OK): `options.cfg` in the working directory is opened
for writing (truncated) as a text stream and gets twelve lines `key value`, one space between,
each ended by a line break (CR LF on Windows, text mode), in this order — note `language`
before `invertY`, unlike the block:

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
| 10 Language | (430, 467) | (755, 467) | (655, 452) … (674, 473) | (831, 452) … (850, 473) |

Buttons: Apply (435, 545) … (545, 560), OK (585, 545) … (695, 560), Cancel (735, 545) …
(845, 560); the "OK" label is centred on (639.83, 549).

Test vectors, starting from the defaults (windowed, 1280 × 720 in the list at index *i*):

* A left press at (660, 160) hits row 0's left arrow: the value becomes "Fullscreen" (copy
  only); the arrow is drawn cyan while the cursor stays on it.
* A press at (840, 160) toggles it back; a press at (700, 160) hits nothing (the gap between
  the arrows).
* Three presses at (660, 310) (FPS Limit, left): `minTimeStep` 9 → 12, shown "83 fps"; nine
  presses at (840, 310) bring it to 3 ("333 fps"); three more to 0 ("none"); further presses do
  nothing.
* A press at (600, 550) (OK): the game's block takes the copy, `options.cfg` is rewritten,
  the window closes; on the next frame the device is reset fullscreen if Mode was left on
  "Fullscreen".
* A press at (780, 550) (Cancel) with "Fullscreen" showing closes the window without applying;
  reopening it still shows "Fullscreen".

At 1920 × 1080 the panel's top-left is (735, 320) and every position above shifts by
(+320, +180).

---

## 7. Esc menu row and system panel

**Toggling.** **Esc** (0x47e770) clears map mode (G+0x8006e4 = 0) and flips the row flag
G+0x8008f2 (initially off, 0x45a302). The row closes when:

* a right click hits no interface widget (0x47b719): clears G+0x8008f2 and the two neighbouring
  flags G+0x8008f0/G+0x8008f1 (the latter is the skill-window flag), and hides the inventory
  (G+0x8008bc), the crafting window (G+0x8008c0) and the preview window (G+0x8008dc);
* an item of the system panel is chosen (mouse release, 0x47e053): the panel is hidden and
  G+0x8008f2 cleared.

Clicking a menu button does **not** close the row.

**Buttons.** Six `menubutton` clones (row B, G+0x800838), made at start-up (0x4605f0 …
0x46098e). Each clone's body texture (shape offset 0x7ac) is its icon from `data3.db`, and its
`count` text is the key label:

| # | Icon | Label | Click action (left button, 0x47b785 → table 0x47ddac) |
|---:|---|---|---|
| 0 | `help.png` | "F1" | toggle the help picture's visibility (as F1) |
| 1 | `skills.png` | "x" (lower case in the executable) | toggle the skill window (0x488c70, as X) |
| 2 | `crafting.png` | "C" | toggle the crafting window (0x488bd0, as C) |
| 3 | `inventory.png` | "B" | toggle the inventory (0x488c00, as B / I) |
| 4 | `worldmap.png` | "M" | enter map mode (G+0x8006e4 = 1; does not toggle) |
| 5 | `system.png` | "O" | toggle the system panel (0x488d00, as O) |

A left click is tested against each button's widget rectangle in order; every button hit
fires (they do not overlap). The click handler plays no sound of its own; the window toggles
play 0x53/0x54.

**Geometry** (row B anchor, §2; `menubutton` pivot (0, 0)): button *i* has its pivot at
(W/2 − 150 + 60·*i*, H − 140). Body: a 48 × 48 square from pivot + (−25, −25) to pivot +
(23, 23), white fill with the icon texture, no stroke (mesh flags 0); its widget hit rectangle
is the same 48 × 48 square (`menubutton` widget bind 48 × 48). Label: `count` origin at
pivot + (−1.5, 26), 10 px, centred, white over a cyan-filled (0, 0.763, 0.814) black-outlined
(2.5 px) layer. No background panel.

**Per frame** (0x48eb40 … 0x48ec8d): every menu button's visibility = G+0x8008f2; while shown,
the button whose widget is under the mouse gets **`textureBrightness` 1.5** (shape offset
0x494), all others 1.0 — the hovered icon is brightened, not enlarged (this corrects the
earlier "1.5× scale").

**Worked example (1280 × 720).** Pivots at x = 490, 550, 610, 670, 730, 790, y = 580; the
`system.png` button covers (765, 555) … (813, 603) and its "O" label is centred on (788.5, 606).
A left click at (770, 560) toggles the system panel; at (520, 560) it hits nothing (the gap
between button 0, x 465 … 513, and button 1, x 525 … 573).

**System panel** (SystemWidget 0x587660, drawn by 0x587710): opened by **O** or the `system.png`
button, on the `blackwidget` panel of §3.3. Three text items, centred, `resource1.dat`
14 px with a 3-px outline, white, cyan (0.2, 1, 1, 1) when hovered:

| Item | Baseline y | Action |
|---|---:|---|
| "Options" | 30 | options window |
| "Start Menu" | 60 | leave the world, back to the start screen |
| "Exit Game" | 90 | quit |

Hover bands are 30 pixels high ending 10 below each baseline (as §3.2).

---

## 8. Windows

| Window (class, constructor) | Opened by | Template / size | Notes |
|---|---|---|---|
| Inventory (InventoryWidget type 0, 0x4c1bb0) | **B** or **I** (0x488c00) | grid of `itembox` | tabs "Equipment", "Items", "Ingredients" with icons `inventory-equipment.png`, `inventory-items.png`, `inventory-ingredients.png`, `inventory-consumables.png`, `inventory-pets.png`; opening it closes the window toggled by **O**. InventoryWidget also exists as types 2 and 3; the panels created next to them are 350 × 285 and 350 × 568 |
| Crafting window (G+0x8008c0; the menu row's `crafting.png` button does the same) | **C** (0x488bd0) | panel 350 × 285 (InventoryWidget type 2 is built next to it) | see the crafting row below for its contents |
| Equipment / character (CharacterWidget 0x434d90) | from the inventory (F-2) | `equippedpreview`; slots "Left", "Right", "Left Ring", "Right Ring", "Neck", "Shoulder", "Chest", "Hands", "Feet", "Light", "Special" | stats "Power", "HP", "ARMOR", "RESI", "CRIT", "TEMPO", "REG", "Weapon Rating", "Armor Rating" |
| Skills (SkillWidget 0x4dd750) | **X** (0x488c70: copies the 11 skill levels into the widget, closes C) | `skillbutton`, `childskillbutton`, `specializationbutton` | specialization texts from the dictionary keys `specialization:<class>:<spec>`, skill texts `description:…`, `details:…`, `skill:level`, `skill:nextlevel`; icons e.g. `warrior-berserker.png` |
| Crafting (an InventoryWidget + BlueprintPreviewWidget 0x42f190) | crafting stations ("[R] Use Anvil" …) / menu button | `craftbutton`, `craftbar`, `itemshadow`, `wideitembox` | categories "Weapons", "Armor", "Amulets", "Cooking", "Alchemy", "Formulas" with `craft-*.png`; "Requires Anvil" etc.; sounds 0x59/0x5a |
| Vendor | talking to a vendor | | "Vendor", `buy.png`, `buy-back.png` |
| Enchanting (EnchantWidget 0x44e910), Adaption (AdaptionWidget 0x40ecd0) | customization bench | `itemframe`, `rightarrow` | " (adapted)", "Adaption" |
| System panel (SystemWidget, G+0x800910) | **O** (0x488d00) or the `system.png` menu button | `blackwidget`, 150 × 120, centred | mutually exclusive with the inventory; §3.3, §7 |
| Help | **F1** | `help.plx` | keyboard picture |
| World map | **M** | — | §5.2 |
| Menu-button row | **Esc** | six `menubutton`s | §7 |

Item tooltips: rarity stars `star1`–`star4`, stats as above, "Requires …", "Ridable",
"Already known", "Warrior "/"Ranger "/"Mage "/"Rogue " class requirement, "Unknown".

The inventory, equipment panel and tooltip are specified in §8.1.

## 8.1 Inventory, equipment and items in hand

Resolves F-2, F-5 and F-13. Every routine was read in `Cube.exe`; *believed* marks a reading
that is not fully proven. Coordinates are window pixels with (0, 0) at the top-left of the
client area; W, H = client width and height (G+0x11c, G+0x120). "Outlined text" means the
code-drawn pair of §3.4: first an outline pass (the given fill colour, black stroke of the
given radius), then a fill pass (stroke 0) in the stated colour. All code-drawn text uses
`resource1.dat`. Sizes are pixel sizes. Item fields are those of `alpha-creatures-behaviour.md`
§10.1; *F*(*v*, *R*) and *G* are its §10.2 level factors; g(*L*) = 1 − 1/((*L* − 1)·0.05 + 1).

### 8.1.1 Data: where the items live (creature record and saves)

The local player's creature (G+0x8006d0) holds:

| Offset | Content |
|---|---|
| +0x140, +0x141, +0x190 | class (1 warrior, 2 ranger, 3 mage, 4 rogue), specialization, level *L* |
| +0x300 + 0x118·*k* | equipment slot *k* (0–12), one 280-byte item each |
| +0x11dc | **bags**: a growable list of *T* bags (begin/end/capacity at +0x11dc/+0x11e0/+0x11e4); each bag is a growable list of **slots** |
| +0x11e8, +0x11ec | the **item in hand** (the stack on the mouse cursor): i32 count + 280-byte item, one slot |
| +0x1304 | money in copper (1 silver = 100, 1 gold = 10 000) |
| +0x1308 | platinum coins |
| +0x130c | outgoing item-record queue (300-byte records, sent as packet 6, §8.1.9) |

A **slot** is 284 (0x11c) bytes: i32 count, then the item. A slot is empty when its count is
0; whenever a count drops below 1 the code sets it to 0 and clears the item's type and
subtype bytes. A slot whose count is negative is drawn black (used by the crafting list,
never by the inventory).

**Bag index ("tab") of an item** (0x47fa30): types 3–9, 23, 24 → 0 (Equipment); type 11 → 2
(Ingredients); types 19, 20 → 3 (Pets); every other type → 1 (Items).

**A new character** (0x4772b0) gets 4 bags; each is grown to 50 empty slots. Bags are never
shrunk; a bag grows by one slot when an item must be added and it has no empty slot (below).

**Saves** (`alpha-saves.md` §3.2): fields 14/15 are the bags exactly as above (u32 *T*, then per
bag u32 *n* and *n* slots of 284 bytes); field 16 (+0x1304) is the **money** and field 17
(+0x1308) the **platinum coins** (this settles both "unknown" fields). The item in hand is
**not saved**.

**Adding an item to the inventory** (0x46ebe0, bag "auto" = the tab rule above):

1. Empty item (type 0 byte) → nothing.
2. Quantity *q*: for the **quantity types** — 12, 13, 21, 0, 25, 20, 24, 23, and 11 unless its
   subtype is 14 — *q* = the item's i16 at +0x10 and the stored copy gets +0x10 := 1; for all
   other items *q* = 1.
3. Type 12 (coins): subtype 10 → money += *q*; 11 → money += 10 000·*q*; 12 → money +=
   100·*q*; nothing is stored. Type 13 → platinum += *q*; nothing stored.
4. Make sure the bag exists (grow the bag list). Walk the bag's slots in order, remembering the
   first empty one; if a slot holds a **stackable** item (types 1, 10, 11, 12, 13, 21; 0x47f9f0)
   equal to the new one (all compared fields of `alpha-net-packets.md` §1.1, 0x42f4a0), add *q*
   to its count and stop.
5. Else put the item with count *q* into the first empty slot; if there is none, append a new
   slot to the bag.

**"Can carry" test** (0x43e4a0), used before picking up and buying: only for types 1, 11 and 20.
*N* = the sum of the counts of all slots (and the item in hand) holding an item equal to the
new one. Refused when the type is 20 and *N* > 0 (one stack of pet food at most), or when
*N* > 49. Everything else is always accepted (bags grow).

**"Can use" test** (0x43e420), for equipping and for the wheel's/Q's level gate in §8.1.7:
item power *p*(item) = trunc(g(item level)·100 + 1) (0x4c76a0); player power
*p*(player) = trunc(g(*L*)·100 + 1) (0x445f10). Usable when *p*(player) ≥ *p*(item) **and**
the class test passes (0x4c6f20):

* type 2 (formula): the test of the item type it teaches (byte +0x08), unless that is 2;
* type 3 by subtype: 0, 1, 2, 13, 15, 16, 17 → warrior; 3, 4, 5 → rogue; 6, 7, 8, 14 → ranger;
  10, 11, 12 → mage; 9 and ≥ 18 → anyone;
* types 4–7 by material: 1, 5, 18, 22 → warrior; 19, 26 → ranger; 23, 25 → mage; 27 → rogue;
  any other material → anyone;
* every other type → anyone.

Test vectors: *p* = 1 at *L* = 1, 17 at *L* = 5, 32 at *L* = 10.

**Two-handed test** (0x444820): type 3 with subtype 5, 6, 7, 8, 10, 11, 15, 16, 17 or 18 (the W2
set of §10.2).

**Price** (0x4c76e0) in copper: *k* = 2 for types 3, 4; 1 for 5, 6, 9; 1.5 for 7, 8; 100 for 23;
0.2 otherwise; ×2 again for a two-handed weapon. Price = max(1, trunc(*F*(*L*, *R*)·10·*k*))
with *L*, *R* the item's level and rarity. Example: iron sword, level 5, rarity 0:
*F* = 1.4142135 → **28** (shown "28 C ").

### 8.1.2 Windows: what exists and where it sits

All windows below are built once at start-up (0x459c40 at 0x463fd0 … 0x465560) as clones of
the `blackwidget` template (G+0x8008c8; look as in §3.3: 5-px notched corners, black fill
alpha 0.67, 3-px black outer stroke, 2-px white inner outline), resized with the 9-slice
rule, made under the in-game root (G+0x800884), each with its widget bound to the clone
(0x631460, "fit") so the widget rectangle is (0, 0) … (w, h) in panel space.

| Host node | Size | Widget | Placement (layout routine 0x482a40) |
|---|---|---|---|
| G+0x8008bc | **400 × 285** | InventoryWidget **type 0** (G+0x800954), the player's bags | top-left **(W/2 − 200, H/2 − 285)**: centred, bottom edge on the screen's centre line. (An earlier line of the same routine puts it at (W − 20 − 400, H − 285 − 220); the later placement overrides it.) |
| (parent of G+0x800960) | **260 × 200** | CharacterWidget (G+0x800960, 0x434d90; draw 0x434e30), §8.1.4 | fixed top-left **(20, 340)** |
| 12 nodes, list G+0x800a34 | 50 × 50 each | SpriteWidgets (list G+0x800a28), §8.1.5 | below the inventory, §8.1.5 |
| G+0x8008c4 | 350 × 568 | InventoryWidget **type 3** (G+0x80095c), the vendor; bags G+0x800c0c | (W/2 + 200 + 10, H/2 − 285): right of the inventory, tops aligned |
| G+0x8008c0 | 350 × 285 | InventoryWidget **type 2** (G+0x800958), crafting; bags G+0x800adc | (W/2 − 200 − 350 − 10, H/2 − 285): left of the inventory |
| G+0x800ad4 | 350 × 330 | BlueprintPreviewWidget (0x42f190) | left of the inventory, top at H/2 − 285 + 10 + 285 |
| G+0x8008f8 / G+0x800900 | 350 × 285 / 350 × 400 | Enchant / Adaption | right of the inventory, tops aligned |

The general rule for the side windows: x = W/2 − 200 − *w* − 10 (left side) or W/2 + 200 + 10
(right side), y = H/2 − 285 (the inventory's top), all as floats.

**Opening.** B or I (0x488c00) toggles the inventory host's visibility, recomputes its scroll
range (0x4c6350, 0x4c64c0) and, if it is now open, hides the system panel (G+0x800910). The
character panel and the 12 equipment boxes have **no key of their own: every frame their
visibility is set equal to the inventory's** (0x48c6c3 … 0x48c7fc; the flag is kept at
G+0x8008f0). So "the equipment/character window" is simply the part of the inventory screen
below and left of the bag grid (this resolves F-2). **Closing:** B/I again, or a right-button
press over no widget (0x47b70f: hides the inventory, the crafting host G+0x8008c0 and
G+0x8008dc, and clears the Esc-row flag). No sound is played by the toggle itself.

**Worked example (1280 × 720).** Inventory panel (440, 75) … (840, 360); character panel
(20, 340) … (280, 540); equipment boxes in the band y = 370 … 700 (§8.1.5).

### 8.1.3 The inventory window (InventoryWidget, constructor 0x4c1bb0, draw 0x4c2050)

**Fields.** Type (+0x194: 0 inventory, 2 crafting, 3 vendor); the bags it shows (+0x160);
current tab *t* (+0x1b4); hovered tab (+0x1b0); per-tab scroll row list (+0x198); scroll
positions *n* (+0x1a4); slot size *s*w × *s*h (+0x1a8, +0x1ac: the `itembox` template's
widget size, 40 × 40 after truncation; 40 × 40 is also the default); hovered slot (tab +0x184,
index +0x188; reset to −1 every frame); selected slot (+0x18c, +0x190).

**Grid geometry** (layout 0x4c5d50, run on resize; draw 0x4c2050 uses the same numbers), for a
panel *w* × *h*:

* columns *C* = trunc((*w* − 10) / (*s*w + 5)), rows *V* = trunc((*h* − 40) / (*s*h + 5));
  400 × 285 → **8 columns × 5 rows**, pitch **45 px**, gap 5 px;
* slot (*c*, *r*) has its top-left at (**10 + 45*c*, 40 + 45*r***) in panel space and is
  40 × 40;
* one clone of `itembox` per visible cell is made as a child of the widget (old clones of the
  same shape are deleted first). `itembox` is a 9-slice rectangle filled **white at alpha
  0.048** (template node 116, box −25 … 15 around its origin, widget 40 × 40);
* the selector (`itemselector`, crafting only) is moved to the selected slot.

**Scrolling.** Bag index *i* of the visible cell (*c*, *r*) is *i* = (row₀ + *r*)·*C* + *c*,
row₀ = the tab's scroll row. Scroll positions (0x4c6350): for a non-empty bag of *m* slots,
*n* = max(1, trunc((*m* − 1)/*C*) − *V* + 2) (unsigned division); the scroll row is clamped to
*n* − 1; an empty bag sets *n* = 0 and row 0. 50 slots → *n* = 3 (rows 0 … 2; the last view
shows slots 16 … 49).

* `upbutton` clone at (*w* − 30, 10), `downbutton` at (*w* − 30, *h* − 30); pressing them
  scrolls −1 / +1 within 0 … *n* − 1 (0x4c60f0, 0x4c5a00).
* `scrollbutton` clone (the thumb, 16 px wide): track from y = 35 to *h* − 35 (length
  *h* − 70); thumb height *t*h = trunc((*h* − 70)/*n*); thumb at x = *w* − 28,
  y = 35 + trunc(((*h* − 70 − *t*h)·row)/(*n* − 1)) when *n* > 1; with *n* = 1 the thumb fills
  the whole track (y = 35, height *h* − 70) (0x4c64c0). Dragging it with the left button sets
  row = trunc(((*y*thumb − 35)·(*n* − 1))/(*h* − 70 − *t*h)) with the thumb top clamped to
  35 … 35 + (*h* − 70 − *t*h) (0x4c5bb0). 400 × 285, *n* = 3: thumb 16 × 71 at
  (372, 35 + 72·row).
* The mouse wheel does **not** scroll the inventory (0x47ef40 only zooms the camera/map).
* All three buttons are tinted (0, 1, 1, 1) while hovered, else white.

**Tabs.** Type 0 gets **four** clones of the `tab` button (`tab` template: 38 × 38 widget, a
32 × 32 textured body, 38 × 38 `border`, 36 × 36 `frame`), whose body texture is set to, in
order, `inventory-equipment.png`, `inventory-items.png`, `inventory-ingredients.png`,
`inventory-pets.png` (tab 0 … 3 = bag 0 … 3). Placement (0x4c6140): tab *k* at
x = 4 + 40·*k* (each next x = trunc(previous + tab width + 2)), y = −(tab height) − 3 =
−41.07, i.e. **a row above the panel's top edge**, 3 px clear of it. Each frame (end of
0x4c2050): the tab's colour is (1, 1, 1, 1) when it is the current tab or hovered, else
(0.8, 0.8, 0.8, 1); its `frame` is **(0, 1, 1, 1) while hovered, (0, 1, 0, 1) on the
current tab, else white**. Pressing a hovered tab other than the current one (0x4c5a60):
current tab := it, scroll range recomputed, the selection moves to the first visible slot,
sound **0x56** (`menu-tab.wav`). The vendor (type 3) uses the same mechanism with its own
tabs (`buy.png`, `buy-back.png`).

**Title.** Outlined text at (**15, 25**), 12 px, white, stroke 3, left-aligned: type 0 —
tab 0 "Equipment", 1 "Items", 2 "Ingredients", tab 3 no title; type 2 — "Weapons", "Armor",
"Amulets", "Cooking", "Alchemy", else "Formulas"; type 3 — "Vendor".

**Money row** (type 0 only), 10 px, stroke 2, baseline y = *h* − 10, right-aligned, always
drawn even when 0, with money *M*:

| Text | x (right edge) | Fill |
|---|---|---|
| (*M* mod 100) + " C " | *w* − 30 | (0.8, 0.5, 0, 1) copper |
| (⌊*M*/100⌋ mod 100) + " S " | *w* − 70 | (0.7, 0.7, 0.7, 1) |
| ⌊*M*/10 000⌋ + " G " | *w* − 110 | (1, 0.9, 0, 1) |
| platinum + "  Platinum Coins " (left-aligned at x = 15) | 15 | (0.5, 0.2, 1, 1) |

**Items in slots** (for each visible bag index that exists; type 0 draws only slots with
count ≠ 0, types 2 and 3 draw all):

* world shader, 10 point lights all cleared, depth test on (render state 7 = 1) for the grid
  and off after it; material colour **(1, 1, 1, 1)**, or (0, 0, 0, 1) for a negative count;
* model drawn by the item-model routine of §8.1.6 at the slot's centre
  (slot x + trunc(*s*h/2), slot y + trunc(*s*h/2)), i.e. **+20, +20**, with the shared turn
  vector G+0x800a1c, depth 0 and scale **0.03**, or **0.045** while the mouse is in the slot
  (*x* ∈ [slot x, slot x + 40), *y* ∈ [slot y, slot y + 40)); rings (type 9) × 0.75;
* the hovered slot is recorded in +0x184/+0x188.

**Stack counts.** For stackable types (1, 10, 11, 12, 13, 21) with count > 0 (count 1 included):
outlined count, 10 px, stroke 2, white, **right-aligned at (slot x + 36, slot y + 37)** (the
same offset as the quick-item button's `count`, §5.5).

**Wide rows (types 2 and 3 only).** For non-empty slots the item name (§8.1.6 name rule) +
" +" + power (not for types 12, 13, 21, 11 ≠ subtype 14, 0, 25, 20, 24, 23) is drawn
word-wrapped (flag 16, wrap width *s*w − *s*h − 20) at (slot x + *s*h + 10, slot y + 10),
10 px, stroke 2, in the rarity colour. Vendor rows add the price at the slot's right edge in
coins (C/S/G, colours as the money row, each 40 px further left, zero parts skipped).

**Item in hand.** Every frame, if the hand count ≠ 0, its model is drawn at the mouse position
with scale **0.05** (depth 0, same turn vector), and the cursor label of `cursor.plx`
(G+0x8008a8, §5.2.6) shows the hand count (width 3) when the item is stackable, else "".

**Worked example (1280 × 720, type 0).** Panel at (440, 75). Tabs at x = 444, 484, 524, 564,
y = 33.93 … 72. Title baseline (455, 100). Slot (*c*, *r*) top-left (450 + 45*c*,
115 + 45*r*): slot (7, 4) = (765, 295) … (805, 335), item centre (785, 315), count right edge
(801, 332). Money baseline y = 350: copper right edge 810, silver 770, gold 730; platinum
from x = 455. Up/down buttons at (810, 85) / (810, 330); thumb x = 812, y = 110 + 72·row.

### 8.1.4 The character panel (CharacterWidget, draw 0x434e30)

Drawn on its 260 × 200 `blackwidget` panel at (20, 340). Positions are panel-relative. Labels
at x = 10, values at **x = 160**; label/value text 10 px, stroke 3 (outline pass in white),
labels filled white, values filled **(0.3, 0.6, 1, 1)** unless stated.

| Baseline y | Label | Value (all rounded to 0.1 by 0x439110 unless stated) |
|---:|---|---|
| 20 | player name (12 px, white, wrapped at 180, flag 16) | — |
| 36 | "LVL " + *L* + " Warrior"/" Ranger"/" Mage"/" Rogue" + spec " \| Berserker"/" \| Guardian", " \| Sniper"/" \| Scout", " \| Fire"/" \| Water", " \| Assassin"/" \| Ninja" (spec 0/1), 12 px, fill **(1, 0.5, 0, 1)** | — |
| 56 | "Power" | *p*(player) = trunc(g(*L*)·100 + 1), integer |
| 76 | "HP" | maximum health (`alpha-creatures-behaviour.md` §4.1, 0x444db0) |
| 92 | "ARMOR" | armour (§4.2, 0x43cff0) |
| 108 | "RESI" | resistance (§4.2, 0x4467a0) |
| 124 | "CRIT" | 100 × critical chance (0x43e9e0, §4.5) + "%" |
| 140 | "TEMPO" | 100 × attack-speed factor *S* (0x447700, §4.3) + "%" |
| 156 | "REG" | round₀.₁(10·*r* / 2^((*L* − 1)·0.25) + 100) + "%", then at x = 210 "(" + round₀.₁(*r*) + ")"; *r* = regeneration sum (below) |
| 172 | "Weapon Rating" | rating *W* (below) as trunc(100·*W* + 0.5) + "%", coloured by band |
| 188 | "Armor Rating" | rating *A* as trunc(100·*A* + 0.5) + "%", coloured by band |

round₀.₁(*x*) = trunc(10*x* + 0.5)/10 for *x* ≥ 0 (negative values are mirrored).

* **Regeneration of an item** (0x4c78c0), types 3–7 only: *F*(*L* + 0.1·*n*, *R*)·*k*·*q* with
  *k* = 0.2 for chest (4), else 0.1, *q* = *u*₁/20 (+0.5 for material 26, +1 for material 27),
  *u*₁ as §10.2. *r* (0x446150) = the sum over both hands (type 3), chest, boots, gloves,
  shoulders and the amulet slot as far as their types match.
* **Weapon Rating** *W*: sum over the hand slots 6 and 7 holding a weapon of
  *F*(item level, item rarity) / *F*(*L*, 0) *(believed: the player's rarity argument is 0)*;
  halved unless the main weapon is two-handed (0x444230).
* **Armor Rating** *A*: (*f*shoulders + 2·*f*chest + *f*gloves + *f*boots)/5 with the same ratio
  *f* for each slot holding its proper type (absent → 0).
* **Bands** (both ratings): < 0.8 → (0.7, 0.7, 0.7); 0.8 … < 1.1 → white; 1.1 … < 1.2 →
  (0, 1, 0); 1.2 … < 1.5 → (0.25, 0.25, 1); 1.5 … < 1.8 → (0.5, 0, 1); ≥ 1.8 → (1, 1, 0)
  (the rarity colours of §8.1.6).

Worked example: level 1 warrior, *g* = 0 → Power 1; two level-1 rarity-0 one-handers →
*W* = (1 + 1)·0.5 = 1.0 → "100%" in white.

### 8.1.5 Equipment boxes (12 SpriteWidgets)

Built at 0x4609b8 … 0x460bf0: for box *i* = 0 … 11 a clone of **`equipmentboxright` when *i* is
even, `equipmentboxleft` when *i* is odd** (the templates are then deleted), each with a
SpriteWidget (0x51c310) bound ("fit") to its `background` child; the `info` label is set to:

| *i* | Label | Slot (creature offset) | Accepts type | | *i* | Label | Slot | Accepts |
|---:|---|---|---|---|---:|---|---|---|
| 0 | "Left\nWeapon" | 6 (+0x990) | 3 | | 1 | "Right\nWeapon" | 7 (+0xaa8) | 3 |
| 2 | "Left Ring" | 8 (+0xbc0) | 9 | | 3 | "Right Ring" | 9 (+0xcd8) | 9 |
| 4 | "Neck" | 1 (+0x418) | 8 | | 5 | "Shoulder" | 5 (+0x878) | 7 |
| 6 | "Chest" | 2 (+0x530) | 4 | | 7 | "Hands" | 4 (+0x760) | 5 |
| 8 | "Feet" | 3 (+0x648) | 6 | | 9 | "Light" | 10 (+0xdf0) | 24 |
| 10 | "Special" | 11 (+0xf08) | 23 | | 11 | "Pet" | 12 (+0x1020) | 19, 20 |

Slot 0 (+0x300) has no box.

**Placement** (0x483740): box *i* is placed so that its template pivot lands at
(**W/2 − 200 + 400·(*i* mod 2), H/2 + 10 + 56·⌊*i*/2⌋**). The right template's pivot is its
box's **top-left** corner and the left template's is its **top-right** corner, so:

* even *i*: box x ∈ [W/2 − 200, W/2 − 150] (flush with the inventory's left edge);
* odd *i*: box x ∈ [W/2 + 150, W/2 + 200] (flush with its right edge);
* y ∈ [Y, Y + 50], Y = H/2 + 10 + 56·⌊*i*/2⌋: six rows under the inventory, 6 px apart.

**Box contents** (relative to the box's top-left (*x*₀, *y*₀), both templates): body 50 × 50,
black alpha 0.469; `frame` (*x*₀ + 1 … *x*₀ + 49) white 2-px outline; `background` widget
44 × 44 at (*x*₀ + 3, *y*₀ + 3) (invisible, alpha 0) carrying the model; `count` text
right-aligned at (*x*₀ + 44, *y*₀ + 45), 10 px, set to "" every frame. `info` label, 10 px,
white with a black 2.5 outline: even boxes at (*x*₀ + 60, *y*₀ + 24) left-aligned (flags 4),
odd boxes at (box right edge − 60, *y*₀ + 24) right-aligned (flags 6) — the labels sit inside
the inventory's width, beside their boxes.

**Every frame** (0x48c7fc …) widget *i*'s item pointer := the slot of the table and its model
:= the item's model (0x4ec400); an empty slot draws nothing.

Worked example (1280 × 720): "Left Weapon" box (440, 370) … (490, 420), label at (500, 394);
"Right Weapon" box (790, 370) … (840, 420), label right edge at 780; "Pet" box (790, 650) …
(840, 700).

### 8.1.6 Drawing an item model in the interface (resolves F-13)

**SpriteWidget** (constructor 0x51c310, draw 0x51c3d0; fields: raw model +0x160, item pointer
+0x164, game +0x168). Used by the 12 equipment boxes and the quick-item button (G+0x800a18,
bound to `quickitembutton`'s 36 × 36 `background`). Draw: if an item pointer is set and its
type byte ≠ 0, call the item-model routine with the **widget rectangle's centre on screen**,
the turn vector G+0x800a1c, **scale = widget width × 0.0006** and depth 0 (equipment box
0.0264, quick item 0.0216); with only a raw model: scale = width × 0.0006 / (largest model
dimension). Before drawing: world shader, the 10 point lights cleared, alpha 1, shininess 0,
material (1, 1, 1, 1), depth test on; off after.

**Item-model routine** (0x4758c0; inputs: screen point (*x*, *y*), turn angles (*a*, *b*, *c*)
in degrees, scale σ, item, depth δ):

1. Model = the item's model (`alpha-creatures-models.md` §6.3, 0x4ec400); an empty type-0 item
   uses model 2396 if the model table has it. No model → nothing.
2. Depth test on, depth function LESS (render state 0x17 = 2).
3. Model matrix (row vectors, *v*′ = *v*·*M*): start from identity with translation
   (0, 0, 1). Rotate by *a* about X (row₁′ = row₁ cos *a* + row₂ sin *a*, row₂′ = row₂ cos *a* −
   row₁ sin *a*), then *b* about Y (row₂′ = row₂ cos *b* + row₀ sin *b*, row₀′ = row₀ cos *b* −
   row₂ sin *b*), then *c* about Z (row₀′ = row₀ cos *c* + row₁ sin *c*, row₁′ = row₁ cos *c* −
   row₀ sin *c*), each applied to the rows produced by the previous step. **Rings (type 9)**
   get two more: +90° about X, then −90° about Z (same row formulas). Multiply rows 0–2 by σ,
   then by 1/max(*s*x, *s*y, *s*z) (the model's voxel size, model +0x44/+0x48/+0x4c). Finally
   translation += (−*s*x/2)·row₀ + (−*s*y/2)·row₁ + (−*s*z/2)·row₂ (the model is centred).
   So the largest dimension becomes σ world units, centred 1 unit in front of the camera.
4. Projection: perspective, vertical field of view π/4, aspect **−W/H** (negative: the image
   is mirrored left–right), near 0.1, far 1000.
5. View: identity, then a **screen-space shift** so that the model's centre lands on (*x*, *y*):
   NDC offset (2(*x* − W/2)/W, −2(*y* − H/2)/H) and depth δ, applied after projection
   *(believed from the matrix order; the result is that the centre sits exactly on the
   point)*.
6. Shader constants: world/view/projection, alpha 1, shininess 1 for materials 1, 11, 12, 22
   else 0 (0x4c7be0), the state reset 0x4e6df0; draw with colour (1, 1, 1, 1).

**On-screen size.** One world unit at distance 1 is 1.2071·H pixels (H/2 ÷ tan(π/8)), so the
model's largest dimension spans about **1.2071·H·σ/(1 + δ)** pixels — it grows with the
window height. At H = 720: slot 26.1 px (hovered 39.1, ring 19.6), hand 43.5, equipment box
22.9, quick item 18.8, wheel (σ 0.09) 78.2, tooltip (σ 0.1, δ −0.05) 91.5 px.

**Turn vector G+0x800a1c**, set every frame just before the interface is drawn (0x4bbabe):
*a* = 225°, *b* = 0°, *c* += frame ms × 0.03 (30° per second, unbounded). Every interface item
model therefore shows the same slow spin about Z under a 225° tilt.

**Lighting.** The directional lights in force are those set after the minimap pass (§5.1.4
note: front (1, 1, 1), back (0.2, 0.3, 0.4), ambient (0.4, 0.4, 0.4), direction
(0.5, 0.4, −0.6)), *believed* still current when the interface draws; point lights are off.

**Item name colour** (0x4c7d20): type 21 → (0.5, 0.5, 0.5); type 13 → (0.5, 0.1, 1); type 25
→ (0.2, 0.5, 1); otherwise by rarity: 0 white, 1 (0, 1, 0), 2 (0.25, 0.25, 1), 3 (0.5, 0, 1),
4 (1, 1, 0), any other (1, 0, 0). The name text itself comes from the item-name routine
0x598a50 (items-generator spec).

### 8.1.7 Tooltip (PreviewWidget, constructor 0x4d4f10, draw 0x4d50a0; text 0x4a28c0)

**Nodes.** The `preview` template node (G+0x8008e4; a **300 × 300** box, black alpha 0.621 with
a 0.669 rim, pivot = its top-left; children `star1` … `star4`, 50 × 50 / 52 × 50 at
(239, 239) from the top-left) carries the PreviewWidget (G+0x8008e8); `equippedpreview`
(G+0x8008ec, 250 × 300, pivot = top-left) is the comparison panel.

**Which item is shown** (0x48d76d … 0x48d99e), first match wins:

1. the quick-item wheel is open (§5.7): the selected wheel item;
2. the hovered inventory slot (needs count ≠ 0) — *window source*;
3. the hovered non-empty equipment box — window source;
4. the crafting, vendor and two further window hovers (0x47b340, 0x47b550, 0x47b3e0,
   0x47b010) — window source;
5. otherwise the world item under the interaction target (G+0x800a78, §8.1.8) — *world source*.

The `preview` node is visible exactly when an item was found.

**Placement** (0x48ec93 …): window source — preview top-left = **mouse + (20, 20)**,
equippedpreview top-left = mouse + (−235, 20) (a 5-px gap to the left of the tooltip); world
source — preview top-left = **(W − 320, H − 320)**, equippedpreview (W − 320, H − 575).

**Compare list** (0x48ecxx, before drawing): every equipped item other than the shown one
whose type equals the shown item's type, checked in this order: right weapon, left weapon,
pet, shoulders, chest, gloves, boots, amulet, right ring, left ring.

**Draw** (all relative to the preview's top-left (*X*, *Y*)):

1. Stars: rarity 1 → `star1` visible, 2 → `star2`, 3 → `star3`, 4 → `star4`; 0 → none.
2. Model: item-model routine at (*X* + 150, *Y* + 150), scale **0.1**, depth −0.05.
3. Text block at (*X* + 14, *Y* + 25), scale *s* = 1, wrap 280, name on, class line on.
4. Price (only while the vendor window is open, 0x47f940): coins of the price (§8.1.1),
   right-aligned from (*X* + 290, *Y* + 280), 10 px stroke 2, colours of the money row, each
   present part 40 px further left (copper, silver, gold).
5. If the compare list is non-empty: "Currently equipped" at (*X* − 190, *Y* + 20), 8 px,
   stroke 3, white, left-aligned; then each compared item's text block (wrap 230) at
   (*X* − 240, *Y* + 40 + 130·*k*).

**Text block** (0x4a28c0; origin (*x*, *y*) = baseline of the first line; *s* = scale; every
line is outlined text with the given stroke; lines 1–3 wrap at the wrap width (flag 16)):

| Step | Shown when | Text | Size, stroke | Fill |
|---|---|---|---|---|
| name | name flag | "Pet food: " (type 20 only) + name + (" +" + *p*(item), except for the quantity types and type 19) | 10*s*, 3 | name colour |
| | | then *y* += 30*s* | | |
| line 2 | always | quantity types: *q* + " X" when *q* > 1, else empty; type 19: "LVL " + level; others: "Power " + *p*(item); + " (adapted)" if item byte +0x0e bit 0 | 9*s*, 3 | white; **(1, 0, 0)** when *p*(player) < *p*(item) |
| class | class flag and the player's class fails 0x4c6f20 | *y* += 14*s*; the classes that pass, in order "Warrior ", "Ranger ", "Mage ", "Rogue " | 9*s*, 3 | (1, 0, 0) |
| known | class flag, type 2, recipe already known (0x444a90) | *y* += 27*s*; "Already known" | 9*s*, 3 | (1, 0, 0) |
| | | *y* += 16*s* | | |
| pet | type 19 | "X" + seed (u32 +0x04) + "/" + trunc(g(level)·1000 + 50); *y* += 14*s*; then "Ridable" if the pet subtype is one of 19, 20, 22, 23, 25–40, 63–67, 74, 75, 98–100, 102–105, 151 (0x444760); *y* += 14*s* | 10*s*, 2 | (0.5, 1, 1) / (0.75, 0.5, 1) |
| DMG | type 3 | "DMG " + round₀.₁(damage §10.3); *y* += 14*s* | 10*s*, 2 | (0.75, 0.5, 1) |
| HP | item health ≥ 0.1 | "HP " + round₀.₁(health §10.3); *y* += 14*s* | 10*s*, 2 | (0.75, 0.5, 1) |
| ARMOR | ≥ 0.1 | "ARMOR " + round₀.₁(armour); *y* += 16*s* | 10*s*, 2 | (0.75, 0.5, 1) |
| RESI | ≥ 0.1 | "RESI " + round₀.₁(resistance); *y* += 16*s* | 10*s*, 2 | (0.75, 0.5, 1) |
| TEMPO | speed ≥ 0.001 | "TEMPO " + round₀.₁(100·speed); *y* += 16*s* | 10*s*, 2 | (0, 1, 1) |
| CRIT | crit ≥ 0.001 | "CRIT " + round₀.₁(100·crit); *y* += 16*s* | 10*s*, 2 | (0, 1, 1) |
| REG | item regeneration ≥ 0.1 | "REG " + round₀.₁(regeneration, §8.1.4); *y* += 16*s* | 10*s*, 2 | (0, 1, 1) |
| heal | type 1, heal ≥ 0.1 | "HP +" + trunc(heal); *y* += 14*s* | 10*s*, 2 | (0.25, 1, 0.25) |

Heal (0x4c6e10), type 1 only: subtypes 1, 2, 4, 5, 6 → 200·*F*(*L*, *R*); 8, 9 → 100·*F*(*L*, *R*);
others 0. Stat formulas are `alpha-creatures-behaviour.md` §10.3. The line-2 *y* step uses
9*s* text but the next step is the fixed 16*s*.

Worked example: an iron sword, level 5, rarity 2, seed 10, shown from a slot with the mouse at
(600, 200): tooltip box (620, 220) … (920, 520), `star2` visible, model centred at (770, 370);
name (rarity 2 colour) at (634, 245) "… +17"; "Power 17" at (634, 275); "DMG 8.0" at
(634, 291); "HP …" etc. follow.

### 8.1.8 Mouse actions

Buttons arrive as press events 0 (left), 1 (right), 2 (middle) (0x47b600); releases
(0x47ddd0) do **not** move items. There is **no drag**: an item is picked up onto the cursor
with one press and put down with another.

**Left press — equipment boxes** (only while the inventory is open, 0x47c4a5 …). Let *E* = the
hovered box's slot, *H* = the hand. If the hand is non-empty and fails "can use" (§8.1.1),
play sound **0x32** and stop. Otherwise, if the hand is empty or holds a type the box accepts,
**swap** *E* and the hand's item, with these extras:

* Right Weapon: if the hand holds a two-handed weapon and Left Weapon is occupied, Left
  Weapon is first moved into the inventory and cleared.
* Left Weapon: a two-handed weapon other than subtypes 6 and 7 (bow, crossbow) is refused
  (sound 0x32). If the hand holds a two-hander, or Right Weapon holds one, Right Weapon is
  moved into the inventory and cleared first.
* Pet: a hand stack of pet food (type 20) with count ≥ 2 does not swap: if the box does not
  already hold an equal item, its old item goes to the inventory, the box gets one, and the
  hand count drops by 1.

After a swap the hand count becomes 1 if the hand now holds an item, else 0. Sound **0x57**
(`menu-grab-item.wav`) if the hand is now non-empty, **0x58** (`menu-drop-item.wav`) if empty.

**Left press — bag slot** (inventory open, a slot hovered): if the hand is non-empty and the
hand item's tab (0x47fa30) ≠ the current tab → sound 0x32, nothing else. If the hand holds a
stackable type and the slot holds an equal item → the slot's count += the hand count and the
hand is emptied (no sound). Otherwise **swap** slot and hand (count and item, 0x44a690), sound
0x57 / 0x58 as above. (The swap is skipped while controller flag +0x29 is set; its meaning is
not identified, *believed* never set in normal play.)

**Left press over no widget with an item in hand** (0x49662f, edge-triggered, inventory open,
0x650ae0 = 0): **drops one item into the world**: a 300-byte item record holding the hand's
item is queued at creature +0x130c (packet 6, §8.1.9) and the hand count drops by one
(0x42f140). Each click drops one more.

**Right (1) or middle (2) press — bag slot**, by open window:

* Adaption window open → it takes the slot as its source; Enchant window open → likewise.
* **Vendor open → sell one**: count −1, money += price, sound 0x3a (`drop-coin.wav`, third
  argument 0.75) at the player, a copy goes to the buy-back list (at most 10 entries, oldest
  dropped), vendor lists rebuilt (0x4a2300).
* Window G+0x8008dc open → a weapon is handed to it (type 3), else sound 0x31.
* **No window — use or equip** (needs "can use", else sound 0x32; nothing for count 0):

| Type | Action (one item is taken from the stack) |
|---|---|
| 3, subtype 13 or 14 (shield …) | if Right Weapon is a two-hander it goes to the inventory and is cleared; old Left Weapon to the inventory; Left Weapon := item |
| 3, right button, subtype 6 or 7 | old Left Weapon to inventory, Left := item; old Right to inventory, Right cleared |
| 3, right button, other | old Right to inventory, Right := item; if the item is two-handed, Left to inventory and cleared |
| 3, middle button | one-hander → old Left to inventory, Left := item; two-hander → as the right-button "other" row |
| 19, or 20 when Pet does not hold an equal item | old Pet item to inventory, Pet := item |
| 4 / 7 / 8 / 5 / 6 / 24 | Chest / Shoulder / Neck / Hands / Feet / Light, old item to inventory |
| 9 | right button → Right Ring, middle → Left Ring |
| 23 (glider, boat) | Special; the player's action byte (+0x68) := 0 |
| 11 with count > 1 | splits: one item, with its +0x10 field incremented, is re-added through §8.1.1 *(as read; purpose not clear)* |
| 1 | **use** (§8.1.10) |
| 2 (formula) | if the taught recipe is already known → sound 0x31; else it is learnt (added to the known-recipe list, save field 18), one item removed, sound 0x2f (`upgrade2.wav`) |

  Each equip or unequip refreshes the grid and plays 0x58.

**Right press — equipment box** (non-empty): vendor open → sell it (money += price, sound 0x3a,
buy-back list, box cleared); Adaption open → it becomes the Adaption source; window
G+0x8008dc open → weapons are handed to it; otherwise **unequip**: the item goes into the
inventory (§8.1.1, never refused) and the box is cleared, sound 0x58.

**Right press — vendor item**: buy-back tab → buy back; otherwise if price ≤ money and "can
carry": add it, money −= price, sound 0x39 (`drop.wav`) at the player; if it cannot be carried:
chat "You can't carry more of these items." in (1, 0.2, 0.2, 1) and sound 0x31.

**Right press over no widget**: closes the inventory (§8.1.2).

**Sorting**: there is none in Alpha.

### 8.1.9 Picking items up from the world

* **Target.** The interaction target G+0x800a78 holds (zone *x*, zone *y*, index) of a world
  item in that zone's item list (zone +0x30, 328-byte records, `alpha-net-packets.md` §3.7),
  index −1 = none. *How the target is chosen (range, aim) belongs to the interaction-target
  rule of §5.8 and was not re-traced here.* There is no pick-up prompt text; while aiming at
  an item its tooltip is shown at the bottom right (§8.1.7, world source).
* **Trigger: E**, edge-triggered (controller flag +0x11 down, previous state up, 0x4975c1).
  Nothing is automatic: walking over items does not collect them.
* If the item exists: **"can carry"** (§8.1.1) → a 300-byte item record (zeroed by 0x459530)
  with the target triple copied to record +0x11c … +0x127 is queued at creature +0x130c and
  sent to the server as **packet 6** (one per tick). This settles most of `alpha-net-packets.md`
  P-8: packet 6 is an item request — **pick up** (target set, item empty), **drop** (item set,
  §8.1.8) and **use** (consumable subtype 7, byte +0x128 = 1, §8.1.10).
* If it cannot be carried: chat line **"You can't carry more of these items."** in
  (1, 0.2, 0.2, 1) and sound **0x31** (`human-voice01.wav`) at the player.
* **Receiving.** The server answers in world-update section 9 (288 bytes: u64 player id +
  item; client 0x48f8a0): type 13 → the player's experience += 10 × the item's +0x10 value
  (level-ups as §4.6); type 25 → mana-cube counter (+0x1164) += 1, action 109, chat
  "You receive a mana cube." and, every fourth one, "You gain one skill point." with sound
  0x1e; every other item → the **chat announcement** (0x480fb0): "You receive " (local player)
  or "<name> receives " in white, then for coins "<*q*> <coin name>" in white, otherwise
  (quantity types: "<*q*> X ") + name + (" +" + *p*(item) for non-quantity items) in the
  name colour, then ".\n" in white.
* **Into the bags.** Items reach the inventory through the incoming-item queue G+0x1000e68
  (pushed by the receive path *(believed)* and by disenchanting): every frame a timer
  G+0x1000e70 += frame ms; when the queue is non-empty and the timer exceeds **100 ms**, the
  timer is reset, the front item is added (§8.1.1, auto tab), sound **0x2d** (`pickup.wav`) is
  played at the player and the grid refreshed. So several items arrive one per 100 ms. No
  carry test is made at this point.

### 8.1.10 Using items: quick-item wheel, Q and right-click

* **Candidates** (0x47ae10): every slot of every bag, in bag then slot order, with count ≠ 0,
  type 1 and item level (+0x10) ≤ the player's level *L*. The **selected quick item** is
  candidate (G+0x800a44 mod count) (negative indices wrap). With no candidates the wheel is
  closed and the quick-item button is empty (§5.5, §5.7).
* **Q** (0x4920ae, edge-triggered, only while no window has input and candidates exist): if the
  item's level > *L* → sound 0x32 at the listener; else use it. **R/E** in the wheel: §5.7.
* **Use** (0x4a2780), type 1 only and only while the player's hit-stun counter
  (creature +0x12c, record field 16) is < 1:
  * subtype 7: a packet-6 record with byte +0x128 = 1 and the item is queued (§8.1.9);
  * other subtypes: the item is copied to the held-item field (record field 43, creature
    +0x1e8), the action becomes **80** (drink) for subtypes 1 and 2, **81** (eat) for the
    others, action time 0, creature +0x128 := 0. The model is shown in the hand
    (`alpha-creatures-models.md` §6.5). Eating (81) ends as action 83 once the action time
    exceeds the item's use limit. The health effect of the action (the tooltip's "HP +"
    value) is applied by the action step and is not traced here (I-1).
  * the stack count drops by 1 (cleared at 0).

### 8.1.11 Sounds used here

0x2d `pickup.wav` (item arrives), 0x2e `disenchant2.wav`, 0x2f `upgrade2.wav` (recipe learnt),
0x31 `human-voice01.wav` (refused: can't carry, not allowed here), 0x32 `human-voice02.wav`
(refused: level/class/tab), 0x39 `drop.wav` (buy), 0x3a `drop-coin.wav` (sell), 0x56
`menu-tab.wav`, 0x57 `menu-grab-item.wav`, 0x58 `menu-drop-item.wav`. Sounds aimed "at the
player" use the player position (creature +0x10); the others use the listener (G+0x140).

### 8.1.12 Open points

* **I-1** The consumables' effect (healing amount and timing) inside actions 80/81, world step
  0x60c510.
* **I-2** How the interaction target G+0x800a78 picks a world item (range and aim).
* **I-3** The visibility of the `equippedpreview` node: no code toggles it; check by capture
  whether it is hidden in `gui.plx` or always shown with the tooltip.
* **I-4** Controller flag +0x29 (blocks the bag-slot swap).
* **I-5** Confirm by capture: item-model mirroring and size (§8.1.6) and the label offsets of
  the equipment boxes.


---

## 9. Chat

* **Enter** (no window open) opens the chat input (chat widget G+0x800a14, flag +0x180); typed
  characters arrive through WM_CHAR. **Enter** again submits (0x47e1b0).
* Commands: `/name <name>`, `/connect <address>`, `/disconnect`, `/namepet <name>` (pet names
  longer than 15 characters are refused with "Pet name too long" in (1, 0.2, 0.2, 1)).
  Anything else is sent as a chat message (offline: added locally).
* While the chat input is open, game keys are not processed.

### 9.1 Chat box layout and drawing (ChatWidget, constructor 0x4393b0, draw 0x439730)

* **Box.** A code-drawn widget of 400 × 200 under the in-game root, top-left at (20, H − 220)
  (size set at 0x465a05, position at 0x483606). No background or frame. Always drawn in game;
  only the input caret depends on the input flag (+0x180).
* **Font.** `resource1.dat`, 10 px, outline radius 2, for every piece of text in the box.
  Each piece is drawn twice at the same pen position: first the outline pass (black outline,
  alpha 1, no fill), then the fill pass (the piece's colour, alpha 1, no outline).
* **History.** At most **10 lines**; when an 11th exists the oldest is dropped (checked at the
  start of every draw). Line *k* (0 = oldest) has its baseline at box y 19 + 18·*k*. A line is a
  list of words, each with its own RGB colour; drawing starts at box x 3; a space word advances
  5 px, any other word is drawn and advances by its measured width.
* **Adding text** (0x43a500; colour given as three bytes): the string is split into words at
  spaces, line feeds and carriage returns; words are appended to the **last** line; when the
  line's width would reach the box width − 20 (= 380 px) a new line is started (word wrap); a
  line feed or carriage return ends the line. Message strings therefore end with a line feed
  ("You receive a mana cube.\n"). A variant (0x43ab30) takes a float colour.
* **Input line.** The typed text (+0x168) is drawn at box (3, 200 − 12 = 188), white. While the
  input is open the caret "|" blinks — visible when ⌊time ÷ 500 ms⌋ is odd — at x = (pen x of the
  cursor position) + 3, same baseline, white over black.
* **Colours used by the game** (0x48ccc0, 0x48cd44, 0x48f959, 0x48fa34, 0x4940d9, 0x49766e):

| Message | Colour (RGB 0…255) |
|---|---|
| chat: sender name + ": " | (0, 255, 255) cyan |
| chat: the message text | (255, 255, 255) |
| "You receive a mana cube." | (100, 150, 255) |
| "You gain one skill point." | (150, 128, 255) |
| "You gain " *n* " XP." | (150, 128, 255) |
| "You can't carry more of these items." | (255, 51, 51) (float (1, 0.2, 0.2)) |
| "Pet name too long" | (255, 51, 51) |

**Worked example (H = 720).** Box (20, 500) … (420, 700). With 3 lines in the history their
baselines are at window y 519, 537, 555, starting at x = 23; the input line's baseline is at
y = 688. A chat line "Bob: hello there" draws "Bob:" in cyan at x 23, then (after the space,
+5 px) "hello" white, etc.

---

## 10. Interface sounds

Sounds are played by id through 0x484350 (key = the `data2.db` file name, volume falls off with
distance for world sounds). Interface ids:

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
| 0x2d | `pickup.wav` | item picked up in the world |
| 0x2e | `disenchant2.wav` | disenchant |
| 0x2f | `upgrade2.wav` | upgrade |
| 0x39 / 0x3a / 0x3b | `drop.wav` / `drop-coin.wav` / `drop-item.wav` | dropping items/coins |

The full id table (0–100, combat and creature sounds included) is in 0x484350.

---

## 11. Controls

### 11.1 Held keys and buttons (DirectInput, polled every frame; 0x4c8720 main loop)

Keyboard state is read with DirectInput 8 (scan codes) only while the game window has focus;
mouse buttons and movement likewise:

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

The bindings are fixed (no key configuration). The meaning of each flag (movement, jump,
dodge, abilities, interaction with R, items with Q) is for the player-control specification.

### 11.2 Key presses (window messages, 0x4c83f0 → GameController 0x47e1b0)

Processed only when no start-screen container is visible and no modal widget has focus:

| Virtual key | Action |
|---|---|
| Enter | open chat (§9) |
| Tab | toggle the quick-item wheel (G+0x800a40, §5.7) |
| Esc | leave map mode and toggle the menu-button row (§7) |
| B, I | toggle the inventory |
| C | toggle the crafting window |
| F | toggle player flag 0x200 (S-3) |
| G | hang-glider toggle (requires the glider state; sets the player's state to 107) |
| M | toggle the world map |
| O | toggle the system panel (§3.3, §7) |
| V | toggle G+0x8007b4: show life bars over all nearby creatures, not only players and focused ones (§5.4) |
| X | toggle the skill window |
| F1 | toggle the help picture |

Mouse buttons and the wheel also arrive as messages: buttons 0 left, 1 right, 2 middle (press
0x47b600, release 0x47ddd0); the wheel sends ±1 per notch (0x47ef40); mouse movement 0x47ea00.
The cursor is hidden and re-centred while the camera is controlled (0x4c8720: SetCursorPos to the
window centre). `cameraSpeed`, `cameraSmoothness` and `invertY` scale and smooth the mouse look.

---

## 12. Follow-ups (not specified in this pass)

* **F-1** Character select/creation layout: positions of the character list, arrow rows and
  captions (CharacterStyleWidget 0x427ce0, start-screen code around 0x459c40 lines 1840–2300,
  button callbacks 0x4821a0, 0x482530, 0x4814f0, 0x484230, 0x481fe0, 0x4815e0).
* **F-2** *Resolved* (§8.1.2, §8.1.4, §8.1.5): the window hosts, sizes and placement; the
  character panel and the 12 equipment boxes have no key and are shown with the inventory.
* **F-3** *Resolved* (§5.5): row A is the ability bar, 6 ability buttons + the quick-item
  button; labels, highlight, icon, grey-out and cooldown overlay specified. Remaining: the
  slot-to-ability rules (player-control spec, 0x48b8ab … 0x48bd3e).
* **F-4** *Resolved* (§6): options window layout, arrows and ranges, resolution list, Apply/OK/Cancel
  and the writing of `options.cfg`. Remaining: confirm by capture the button-label centring
  (§6.5) and that `language` has no reader.
* **F-5** *Resolved* (§8.1): grid, tabs, scrolling, slot drawing, counts, tooltip, mouse
  actions (click to pick up / put down, no drag), use/equip, selling, world pickup and drop.
  Remaining: I-1 … I-5 (§8.1.12).
* **F-6** Skill window layout and skill-point spending (SkillWidget 0x4dd750).
* **F-7** Crafting window and the blueprint preview (0x42f190).
* **F-8** *Resolved* for the player, pet, party and creature bars (§5.3, §5.4), floating combat
  text (§5.6) and the land-name banner timing (§5.9). Remaining: the `textFX` / `minimap`
  string users and the visibility of the `landscape` and TIME/TEMP labels (M-5).
* **F-9** Speech bubbles, objective list and statistics widgets (0x4e5c90, 0x4ce180, 0x583b40),
  the dictionary markup (`[text $style]`, `%`, `@name`) rendering.
* **F-10** *Resolved* (§5.2): opening/closing, wheel zoom and its limits, middle-drag pan,
  left-drag turn and tilt, the map frame and its call of 0x5fc1b0, the map extras
  (placeholders, 0.6 dimming, mission and town markers), the map-mode lights (compass only),
  the overlay's zone and location labels, the cursor label and portal teleporting. Remaining:
  the mission fields and object type 45 (§5.2.8); the compass routine belongs to F-13.
  `alpha-physics-camera.md` §7.2/§7.3 should be corrected to match §5.2.2 (wheel up zooms the
  map **in**; the left drag turns the map, the middle drag pans).
* **F-11** *Resolved* (§5.8): there is no death screen; the crosshair prompt reads
  "[R] Revive" while HP ≤ 0 and R respawns with full HP. Remaining: the respawn point rule
  (0x5a03d0) belongs to the player-control / world specification.
* **F-12** *Resolved* (§5.1): minimap content, tint, lights, markers and draw state of
  0x5fc1b0. Remaining: what the zone records' marker lists (+0x20) hold, and the start value
  of the tiles' rise-in timer.
* **F-13** *Resolved* (§8.1.6): SpriteWidget 0x51c310 / 0x51c3d0 and the item-model routine
  0x4758c0 (matrices, turn vector, scale rules, lighting state).

## 13. Open questions

* **S-1** Partly settled (§3.3): there is no start-screen-specific background; the clear colour
  is (0, 0.44, 1) and the normal world frame is drawn under the interface. Still to confirm
  by capture: what the world pass shows before any world is loaded.
* **S-2** *Settled* (§5.7): Tab opens the quick-item wheel.
* **S-3** Partly settled: V (G+0x8007b4) shows life bars over all nearby creatures (§5.4).
  F (player flag 0x200) is still open.
* **M-1** *Settled* (§5.1.4): the scissor test is never enabled in `Cube.exe` (the only
  state-174 call is the minimap's own disable at 0x5ffde5), so the rectangle has no effect.
* **M-2** *Settled* (§5.1.2–5.1.4): the (0.2, 0.4, 1, 1) tint is the material colour of the
  land picture at the farthest zoom (×0.8–1.1 per land); zone tiles use (1, 1, 1, 1); the
  terrain keeps the world's lights; players, mission targets and (zoomed in) town NPCs appear
  as markers with their own lights. A capture at the three zoom steps (top-right corner) is
  still the check.
* **M-3** The exact world anchor height of hit numbers (0x494ec7 … 0x494f53 adds 2 units and a
  term from the target's size; and moves the anchor when the local player is the target).
* **M-4** "LEVEL UP!" and "XP +n" use text flags 2, which §3.4 reads as right-aligned; verify by
  capture (it would put the banner left of the player's head).
* **M-5** Whether the code-made `landscape`/`landscapedetail` labels (G+0x800858, at (20, 45))
  and the TIME/TEMP readout (G+0x800860, at (W − 15, 20)) are visible in normal play or are a
  debug readout.
* **M-6** The meaning of the hit flags +0x14 (60-px text, rise −200; believed critical) and
  +0x44 (yellow text), and of the list "focus" flag that selects which creatures get bars.
* **M-7** Which ability-bar flag the "4" button should use: the update tints buttons 0–4 from
  the M1, M2, 1, 2, 3 controls and the quick-item button from Q, but applies nothing to button 5
  ("4"). Faithful behaviour is no highlight on "4"; a capture holding 4 would confirm.
* **S-4** Hover/click feedback of `.plx` buttons (the `button:*` sequences exist in the
  library; the shipped `button` templates contain no such sequences) — capture a button hover.
