# Alpha talking: speech bubbles, what NPCs say, vendors

Behavioural specification of what happens after "[R] Talk" in Cube World **Alpha**
(`Cube.exe`, SHA-256 `84a7a132…30bf`): the speech bubbles (pool, template, placement over
the head, typing, paging, closing, sounds), how an NPC's words are chosen from
`dict_en.xml` and how the dictionary markup is parsed and rendered, the inn, the vendors
(stock, where it lives, buy / sell / buy-back, what is and is not networked), and the two
other town windows (classes 131 and 137).

Written from the Alpha decompile (`disassembled alpha cube world/decompiled/client`), fresh
disassembly of `binary/Cube.exe` where the export lost control flow, the `gui.plx`
template and `data4.db` `dict_en.xml`. Nothing here is copied code; routines are cited by
address in `Cube.exe` as references only. *Believed* marks readings that are not fully
proven.

Already specified elsewhere and only referenced here: `alpha-interaction.md` §5.2 (the R
press, the packet-6 kind-2 record, classes 128–130 opening the vendor pair),
`alpha-ui-screens.md` §8.1 (inventory widget, vendor window geometry 350 × 568, prices
§8.1.1, mouse actions §8.1.8, tooltip §8.1.7), `alpha-item-names.md` (item words, location
names), `alpha-worldgen-structures.md` §5.5.9 (town NPC spawns and the vendor stock
generators V1/V2/V3, to the draw), `alpha-server-simulation.md` §8 (packet-6 kind 2, the
inn), `alpha-loot-rolls.md` §4 (the 45-entry tameable list), `alpha-worldgen-random.md`
§1 (the C `rand()` generator).

**Status.** Bubbles, typing, paging, placement, sounds, the markup grammar and its
renderer, the NPC text choice (all four cases), the inn, vendor stock origin, the vendor
window tabs, buy / sell / buy-back and the absence of any vendor network traffic are
specified. Corrections to other specs are listed in §8.

---

## 0. Conventions

* G = game object, W = world object (G+0x2e4), P = the local player creature
  (G+0x8006d0). Creature object offsets as in `alpha-interaction.md` §0; the network record
  offset is the object offset − 0x10 (`alpha-net-packets.md` §2).
* NPC creature fields used here: +0x08 64-bit id (its low 32 bits are the **talk seed**);
  +0x60 controller kind (3 = NPC); +0x64 creature type; +0x7e appearance flags; +0x80 size
  vector (blocks); +0x140 **class** byte; +0x1b0 home triple (zone x, zone y, spawn index);
  +0x1d8 **talk kind** byte (network field 42); +0x1dc, +0x1e0 **talk target** zone x, zone y
  (network field 41, 3 × i32); +0x1180 drawn-height offset; +0x11dc bags; +0x1304 money;
  +0x1350 drawn position (3 × i64 fx).
* Positions are i64 in 1/65536 block ("fx"); 1 block = 65536 fx.
* Times are milliseconds of game frame time (the per-frame delta the frame update adds).
* "Outlined text" = the two-pass text of `alpha-ui-screens.md` §3.4: first the given fill
  colour with a black stroke of the given radius, then the fill colour with stroke 0. All
  bubble text is `resource1.dat`.
* `rand()` is the MSVC generator of `alpha-worldgen-random.md` §1.1 (state·214013 +
  2531011, result (state >> 16) & 0x7FFF; `srand(s)` sets the state).

Town NPC classes (byte +0x140; from `alpha-worldgen-structures.md` §5.5.9 and §6.2):

| Class | Who | Window on R (0x4889e0) | Stock |
|---:|---|---|---|
| 128 (0x80) | general shop ("Item Shop", `shop-sign`) | vendor | V1 |
| 129 (0x81) | weapon shop (`weapon-shop-sign`) | vendor | V3 |
| 130 (0x82) | armour shop (`armor-shop-sign`) | vendor | V2 |
| 131 (0x83) | identifier (`identifier-sign`) | **Identification** (§4) | V3 (never shown) |
| 132 (0x84) | innkeeper (`inn-sign`) | none; text `innkeeper` (§2.4) | none |
| 133–135 (0x85–0x87) | smithy, carpenter, weaver (craft houses) | none | none |
| 136 (0x88) | town resident | none | none |
| 137 (0x89) | marker NPC of mode 7 (tower, landing place) | **Adaption** (§4) | none |
| 1–4 | class-house NPCs (warrior … mage, modes 9–12) | none | none |

---

## 1. Speech bubbles

### 1.1 Objects

Built once at start-up (0x459c40, 0x461a68 … 0x461cc5):

* The `speech` template is looked up by name under the in-game root (G+0x800884). The
  node list G+0x800920 is sized to **4**; each entry is a clone of the template made a child
  of the in-game root, and each gets its own **SpeechWidget** (0x4e5c90, 0x1e0 bytes)
  stored in the parallel list G+0x80092c. All four start hidden. So **at most four bubbles
  exist at once**, shared by NPCs and the player.
* A fifth SpeechWidget (G+0x800938) on an empty node G+0x80093c, resized to 400 × 400 and
  laid out at (W/2 − 200, 300) (`alpha-ui-screens.md` §2). It is the "dialogue" widget. It is
  advanced every frame (§1.4) but **nothing ever fills it**, and the "open dialogue" pointer
  G+0x800940 is only ever set to 0 (reset 0x45a345; cleared at 0x496a3b and 0x497552).
  Every branch that tests G+0x800940 ≠ 0 is therefore dead in Alpha (*believed*: a
  leftover of a planned quest dialogue).

**Template geometry** (`gui.plx`, node `speech` with two SmoothMeshShapes named `speech`,
both the same 15-vertex outline). Coordinates are node-local pixels, y down:

* a box from x = 49.175 to 299.175 (**250 px wide**) and y = −115.824 to 34.176
  (**150 px high**) with 5-px notched corners (the same corner style as `blackwidget`);
* a tail on the bottom edge: (169.175, 34.176) → (174.175, **54.176**) → (179.175, 34.176),
  10 px wide, 20 px long, its tip under the box's horizontal centre (x = 174.175);
* shape 1: fill black at alpha **0.503**, stroke black **8 px**; shape 2: no fill (alpha
  0.503 copy), stroke **white 2 px**. So: a translucent black box with a thick black rim and
  a thin white inner outline, pointing down.

**SpeechWidget fields** (constructor 0x4e5c90):

| Offset | Meaning | Initial |
|---|---|---|
| +0x148 | the node it draws into | |
| +0x160 … +0x168 | **pages**: a vector of pages; each page is a list of **segments** (wide string + RGBA colour) | empty |
| +0x16c | **elapsed** time on the current page (ms) | 0 |
| +0x170 | current **page index** | 0 |
| +0x174 | the typed-character count of the previous frame (for the sound) | 0 |
| +0x178 … +0x180 | answer strings (24-byte wide strings) — always empty in Alpha (§1.8) | empty |
| +0x184 | selected answer | 0 |
| +0x188 | the **creature** the bubble belongs to (pointer; 0 = free) | 0 |
| +0x190 … +0x1a4 | the bubble's world **anchor** (3 × i64 fx) | |
| +0x1a8 | font `resource1.dat` | |
| +0x1c0, +0x1c4 | text size **14**, outline stroke **3** | |
| +0x1c8, +0x1cc, +0x1d0 | layout cursor x, y, characters consumed (rebuilt every draw) | |
| +0x1d4 | **40** — milliseconds per character | |
| +0x1d8 | back pointer to G | |

The widget rectangle is 250 × 200 with a 10-px inset (0x627c00 called with (0, 0),
(250, 200), (10, 10), (230, 180)); its width query (0x627d50) returns **250**.

### 1.2 Length of a page

The **length** of a page (0x4e6550) is 40 × Σ (characters of the segment + 1) over its
segments, i.e. every segment counts one extra character for the space that follows it.
A page index outside the pages gives length 0. A page is **fully typed** when elapsed ≥
length (0x4e6530).

### 1.3 Opening a bubble

**For an NPC** (0x4882e0, called by the Talk press of `alpha-interaction.md` §5.2 with
"restart" = false — the only caller):

1. If one of the four bubbles already belongs to this NPC (bubble+0x188 = the NPC): if its
   page is fully typed → page index + 1, elapsed := 0, +0x174 := 0 ("next page"); otherwise
   elapsed := the page length ("finish typing"). Stop.
2. Otherwise pick a free bubble (+0x188 = 0): the pool is scanned in order and the **last**
   free one wins. None free → nothing is shown (the kind-2 packet was still queued).
3. Build the text (§2.4) into a fresh page vector. If it is empty or its first page has no
   segment, stop (no bubble).
4. Show the bubble's node, bubble+0x188 := the NPC, anchor := the NPC's drawn position
   (+0x1350), pages := the new pages, elapsed := 0, +0x174 := 0, page := 0, answers cleared,
   selected answer := 0.

The NPC branch with appearance flag 0x80 (+0x7e & 0x80) is described in §1.8.

**For the player** (0x488030, "There is nothing special.", called from the object R press
of `alpha-interaction.md` §5.3 unless the static kind is 1, 2, 3, 10, 16, 18, 19 or 69):
the same pattern with creature := P. If P already owns a bubble it is advanced or finished
exactly as in step 1. Otherwise the last free bubble is taken; its anchor := P's position
(+0x10, not the drawn position); its pages are cleared and **one page** is made from the
string "There is nothing special." split at whitespace into white (1, 1, 1, 1) segments:
"There", "is", "nothing", "special." (punctuation is *not* split off here, unlike §2.1).

### 1.4 Every frame (frame update 0x488ee0)

In this order:

1. **Time** (0x490860 … 0x4908c9): every bubble's elapsed += dt, and the dialogue widget's
   too.
2. **Auto-advance and close** (0x4966e2 … 0x49688b), for each of the four bubbles and then
   the dialogue widget: if it has no answers and elapsed > length + **3000** (strictly) →
   page index + 1, elapsed := 0, +0x174 := 0. Then, if the page index ≥ the number of pages
   → hide its node and bubble+0x188 := 0 (free). A page therefore stays **3 s after it has
   finished typing**; the bubble closes 3 s after the last page finished.
3. The R / E presses (`alpha-interaction.md` §5).
4. **Placement** (0x496f37 … 0x497329), §1.5, for every bubble that has a creature.

There is **no distance limit** and no other timeout: a bubble follows its creature
anywhere until its pages run out. (The bubble keeps a raw pointer to the creature; nothing
frees the bubble when the creature is removed — §7 B-1.)

### 1.5 Placement over the head

For a bubble with creature *C*:

1. **Follow**: s = 1 − 0.98^dt (0x4ac150(dt, 0.02), double, rounded to float). Target =
   C's drawn position + (0, 0, −C.+0x1180) (the height offset is in blocks, converted ×65536
   to fx). anchor += (target − anchor) × s (fx, per component). The bubble glides after
   the creature with a time constant of about 50 ms.
2. **Head point**: H = anchor + (0, 0, ½·C.size.z + 1) blocks (C.+0x80 element 2;
   *believed*: the two horizontal parts of that offset are 0).
3. **Project** H (relative to the camera) with the frame's view-projection; if the
   projected depth ≤ 0 (behind the camera) the node is hidden for this frame, else shown.
4. **Screen point** (with W, H = renderer width and height, renderer +0x10c / +0x110):
   Px = ndc.x·W/2 + W/2, Py = −ndc.y·H/2 + H/2.
5. The node is translated by (Px, Py) − (125, 170) − the node's pivot (0x497268 …
   0x4972db). *Believed* (the pivot is the box's top-left corner (49.175, −115.824)), this
   puts the box at **x = Px − 125 … Px + 125, y = Py − 170 … Py − 20**, and the tail tip
   exactly on (Px, Py): the bubble is centred over the head point and points at it.

Worked example (1280 × 720): an NPC whose head point projects to the screen centre
(640, 360): box (515, 190) … (765, 340), tail tip (640, 360), text origin (§1.6) (525, 215).

### 1.6 Drawing the text (draw 0x4e5f90, per segment 0x4e65a0)

Only the current page is drawn. Per draw the cursor (x, y) := (0, 0) and consumed := 0;
"glue" := true (no gap before the first segment). Let V = ⌊elapsed / 40⌋, the number of
characters typed so far. For each segment *S* of the page, in order:

1. If V < consumed: stop drawing this page (the rest is not yet typed).
2. w = the advance width of the **whole** word at size 14.
3. punct = *S* is exactly one of `.` `-` `,` `;` `!` `?` `:`.
4. If not glue: x += **3** for punct, else **14** (the word gap).
5. glue := (*S* = "-") — the word after a hyphen is glued to it.
6. **Wrap** (not for punct): if x + w > 250 − 30 = **220** → x := 0, y += **18**.
7. Visible part: the whole word, or only its first V − consumed characters if
   V < consumed + |*S*| (the typing effect; the layout already uses the full width, so
   words never jump when they finish typing).
8. Draw it as outlined text, size 14, stroke 3, in the segment's colour, with its baseline
   at (box left + **10** + x, box top + **25** + y) (*believed* box-relative as in §1.5;
   the code adds the node's position).
9. x += w; consumed += (visible characters) + 1.

So: 14-px text, 18-px lines, a 10-px left margin, at most 220 px of text per line and room
for 7 lines in the 150-px box. **Paging is only by `%`** (§2.2): a page that does not fit
simply overflows the box. Punctuation never wraps.

**Typing sound**: after the segments, if ⌊+0x174 / 3⌋ ≠ ⌊consumed / 3⌋ the sound **0x32**
(`human-voice02.wav`) is played at **P's position** (not the speaker's), volume 1, pitch
1 + 0.5·r/32767 with r = `rand()`; then +0x174 := consumed. So one voice blip per three
typed characters, and none once the page is complete.

### 1.7 Pressing R again (summary)

| State of the NPC's bubble | R on the NPC |
|---|---|
| none | open a new bubble (§1.3), if one of the four is free |
| typing | finish the page at once |
| page typed, more pages | next page (typing restarts) |
| last page typed | page index runs past the end → closed on the next frame |

Every press also queues the packet-6 kind-2 record (`alpha-interaction.md` §5.2), whatever
the bubble does. The player's own bubble is advanced the same way by pressing R on a
static that produces it again.

### 1.8 Unused paths (keep for parity, no visible effect)

* **Answers** (+0x178): when a page is fully typed and answers exist, they are drawn on
  one row at y = widget height − 75 (= 125), starting at x = 15, each outlined 14 px, the
  selected one (+0x184) cyan (0, 1, 1, 1), the others white, each next one 20 px + its
  width further right; while answers exist the 3-s auto-advance is off. Nothing in Alpha
  ever adds an answer.
* **Dialogue widget** G+0x800938: never filled (§1.1).
* **Appearance flag 0x80 NPCs** (0x4882e0 second branch): no text; instead every location
  record of the NPC's home region (home zone >> 6; 8 × 8 records of 0x68 bytes at region
  +0x14018) whose type (+0x18) ≠ 0 and whose level (+0x24) ≤ P's level + 2 is marked
  discovered on the world map (0x5fc160 at the record's zone, x = record x fx / 65536 / 256).
  No generator in either program sets flag 0x80 on an NPC (*believed* dead).

---

## 2. What NPCs say

### 2.1 Speech entries and their parse tree

`dict_en.xml` `<speech key="K">` elements are read by the dictionary loader (0x4e1970,
0x4e25f2 …) and their text is parsed once (0x4da850) into a tree of **text nodes**
(`QuestTextNode`, 0x44 bytes, 0x4da3b0). A node has: a **condition set** (tags starting
with `#`), a **style set** (words starting with `$`), its own **text**, a parent and an
ordered list of children. The root is an empty node.

**Tokenizer.** The text is scanned character by character. A token ends at a **delimiter**:
space, tab, line feed, NUL, `{`, `}`, `[`, `]`, `|`, or one of the punctuation characters
`.` `:` `-` `,` `;` `!` `?` `/` `(` `)`. At each delimiter, in this order:

1. `{` → a **group** node is added as a child of the current node and entered; then a first
   **alternative** node is added to the group and entered (two levels).
2. `[` → a **span** node is added as a child of the current node and entered.
3. The token since the previous delimiter (if any characters, or if the delimiter is
   punctuation):
   * starts with `#` → added to the current node's condition set;
   * starts with `$` → added to the current node's style set;
   * otherwise → a **word** node with that text is added as a child of the current node;
     if the delimiter is punctuation, a second word node holding just that character is
     added after it. (An empty token before punctuation makes an empty word node, which
     renders as nothing.)
4. `|` → leave the current alternative and add/enter a new alternative of the same group.
5. `}` → leave two levels (the alternative and the group).
6. `]` → leave one level (the span).

A `}` with fewer than 3 levels open, or `]`/`|` with fewer than 2, abandons parsing (the
tree built so far is kept). `%` and `'` are ordinary characters, so `%` on its own is a
word ("25%" is a word too) and "I've" or "'o" are single words. Because `{` and `[` are
handled before the pending word, markup must be preceded by a delimiter (the dictionary
always has a space).

Example: `In [Cube World $stress]!` → root: word "In", span{style $stress: word "Cube",
word "World"}, word "" , word "!".

### 2.2 Rendering a tree into pages (0x4e4350)

Input: the tree, a **tag set** and a **variable map** (§2.3), and the page vector (if it is
empty, one empty page is created first, 0x4e4a20). Node *N* is rendered as follows; the
result is "continue" or "stop":

1. If *N* has conditions and **none** of them is in the tag set → skip *N*, result
   "continue". If *N* has conditions and at least one is present → *N* is "matched".
2. If *N* has text:
   * text = `%` → start a new empty page;
   * colour := white (1, 1, 1, 1), then from the **parent's** style set, testing in this
     order (a later match overrides): `$creature` (1, 0.3, 0.3, 1) red · `$name`
     (0.6, 0.3, 1, 1) violet · `$item` (0.3, 0.3, 1, 1) blue · `$object` (1, 0.5, 0.3, 1)
     orange · `$zone` (0.3, 1, 0.3, 1) green · `$stress` (0.6, 0.3, 1, 1) violet ·
     `$number` (0.3, 1, 0.3, 1) green;
   * text not starting with `@` → its whitespace-separated words are appended to the
     current (last) page as segments of that colour (0x4e5740);
   * text starting with `@` → the whole token (with the `@`) is looked up in the variable
     map; if found, the value's words are appended in that colour; if not, nothing.
3. Render the children in order; stop at the first child whose result is "stop".
4. Result: "stop" if *N* was matched, else "continue".

So in `{ #a #b X | #c Y }` the first alternative with **any** of its tags present is
rendered and ends the group; an alternative without tags is always rendered and does not
end the group. Styles colour only the direct children of the span that carries them.

### 2.3 Variables and tags

Three setters fill the tag set and variable map from dictionary `<name>` entries; *p* is a
prefix chosen by the caller:

* **creature** (0x594c80, creature type *t*): entry = `name[key(t)]` (the type → key table of
  `alpha-ui-screens.md` §5.4.1); for each word *g* of its `tags` attribute the tag
  `#p_g`; for each child element *c* the variable `@p_c` = its text (so `@p_singular`,
  `@p_plural`).
* **item** (0x595010, item): the same with entry `name[K(type, subtype)]`
  (`alpha-item-names.md` §1.1); material, level and seed are ignored.
* **location** (0x5953a0, location record *L*): entry = `name[location key(L.type,
  L.variant)]` (`alpha-ui-screens.md` §5.9.1); tags `#p_g`; each child text with its first
  `@` replaced by the location's generated name N(L+0x20) → `@p_c`.

Example: a "Ruins" location (tags `p in the`) gives tags `#scenery_p`, `#scenery_in`,
`#scenery_the` and `@scenery_singular` = "Ruins of *N*".

### 2.4 Choosing the text (0x4882e0 → 0x4e4bd0)

**Innkeeper** (class 132): always the entry `innkeeper` with no variables — day or night:
"You can rest in our inn between [six 'o clock in the evening] and [six 'o clock in the
morning]." (bracketed parts violet). Nothing else is chosen; no `rand()` is used.

**Every other NPC** (talk kind *k* = +0x1d8, talk target (zx, zy) = +0x1dc, +0x1e0, both
from network fields 42 and 41, i.e. from the NPC's spawn):

0. *Target check* (in 0x4882e0): with cx = trunc(zx/8), cy = trunc(zy/8) in 0 … 8191 and the
   region (cx/8, cy/8) loaded, the location record (cx mod 8, cy mod 8) of that region is
   read; if its level (+0x24) is not within P's level ± 2, (zx, zy) := (−1, −1).
1. `srand`(the NPC id's low 32 bits). The same NPC therefore always says the same thing
   (until its id changes, e.g. after respawning).
2. Location lookup (0x487da0) at (trunc(zx/8), trunc(zy/8)) with C division and the +7
   fix-up, so −1 maps to cell 0.
3. **k = 4** and that location record exists with type ≠ 0 → variables from the location
   (prefix `scenery`), entry **`explored`**: "Have you already explored {the} [*place*]?"
   % "Look, I've marked the location on your [map]!" — two pages. No `rand()` draw. Done.
4. **k = 3** → pet = tameable[`rand()` mod 45] (`alpha-loot-rolls.md` §4); variables
   `creature` from the pet, `food` from the item (20, pet) (its pet food); entry
   **`petfood`**: "Do you know what [@creature_plural] like to eat? [@food_plural], of
   course!" Done.
5. Otherwise (k = 0, or k = 4 without a valid location) — **random talk**, four draws in
   this order:
   1. item = (11, 12) Flask, (24, 0) Lamp, (23, 0) Hang Glider, (23, 1) Boat, (1, 7) Bomb for
      `rand()` mod 5 = 0 … 4 → variables `itemvendoritem`;
   2. pet = tameable[`rand()` mod 45] → variables `pet`;
   3. monster type = (108 Troll, 109 Dark Troll, 114 Yeti, 115 Cyclops, 119 Saurian,
      111 Golem, 113 Snow Golem, 112 Ember Golem, 116 Mammoth, 110 Hell Demon, 117 Lich)
      [`rand()` mod 11] → variables `monster`;
   4. entry **`random:`** + (`rand()` mod 30 + 1).

   All four draws are made whatever entry is chosen. The 30 entries use `@itemvendoritem_plural`
   (5), `@pet_singular` (11, 12, 19) and `@monster_plural` (21); the others are plain text.

Which kind an NPC has comes from world generation (`alpha-worldgen-structures.md` §5.5.9):
residents get k = 4 with the target = a point of interest when a draw is even and the POI
list is not empty, then k = 3 when d % 10 = 0 (overriding 4); shop, inn and marker NPCs keep
0. (*Believed*: the POI entries are zone coordinates; the client reads them as zones.)

### 2.5 The map mark

After choosing the text (not for the innkeeper), 0x4882e0 marks the zone (zx, zy) on the
world map (G+0x800d44): the map cell gets its "discovered" bit (cell +0x30 bit 0, with the
map's discovered counter + 1, 0x5fc160) and a copy of the 16-byte zone summary of W's
region table (region + 0x18 + 16·((zx mod 64)·64 + (zy mod 64))) at cell +0x10. With
(zx, zy) = (−1, −1) nothing happens. As read this runs for every non-innkeeper NPC with its
stored target, so the "I've marked the location on your map" promise is literal.

### 2.6 Quests

Alpha NPCs give **no quest dialogue**. No code references the keys `title:…`,
`speech:<class>:…` or `objective:<class>:…` of `dict_en.xml`; the `objective:<mission>` and
`description:` keys are used by the mission tags and skill descriptions, not by Talk. The
quest-like class chain in the dictionary is unused data.

### 2.7 The inn

Talking to an innkeeper always shows the `innkeeper` bubble (§2.4) **and** queues the
packet-6 kind-2 record (target = the NPC's home triple). Whoever owns the simulation (the
server, or the client offline) then applies the inn rule of `alpha-server-simulation.md` §8:
class 132 and time ≥ 18:00 or ≤ 6:00 → day + 1, time := 7:00, sound 29 (`levelup2.wav`,
volume 0.75), every spawned NPC removed and its respawn countdown reset. There is **no
rest message**: the only feedback is the unchanged innkeeper text, the level-up sound and
the jump to morning. In the daytime the press does nothing beyond the bubble.

---

## 3. Vendors

### 3.1 Stock: who generates it

The stock is made by **world generation**, not by the vendor or the server, when the town
zone is generated (`alpha-worldgen-structures.md` §5.5.9, to the draw):

| Class | Generator | Content |
|---|---|---|
| 128 | **V1** (0x5c9b30) | round 0 extras: Flask (11, 12, glass), Lamp (24, 0, iron) and up to four rarer Lamps, Hang Glider and Boat (23, 0/1, wood), one pet food (20, *X*), *X* ∈ {34, 35, 19, 26, 30, 87}; then per round 0–2 formulas (type 2, teaching an armour or weapon piece of the round's level) and a 50 % chance of a Bomb (1, 7) |
| 129, 131 | **V3** (0x5ca0a0) | weapons: per round 1–3 picks from 11 candidates (two class weapons, the two-hander set, shield, dagger, longsword, fist, bow, boomerang, staff, wand, bracelet) |
| 130 | **V2** (0x5c8390) | armour: per round 1–3 picks from 21 candidates (iron/silk/linen/cotton chest, gloves, boots, shoulders; amulet; ring) |

Common rules:

* **30 rounds**, round *k* with level L from the band (L0, L1) of §5.5.9 (round 0: level 1;
  rounds reach level 580 and beyond). The town's level band is passed in but **ignored**,
  so every vendor sells items of all levels from 1 up.
* Rarity 0–3 from three draws (1 in 100, then +1 for 1 in 1000, +1 for 1 in 10 000).
* Every weapon and armour piece has byte +0x0e = 1, which the tooltip shows as
  **" (adapted)"** after the power (`alpha-ui-screens.md` §8.1.7).
* Everything is added to **bag 0** of the spawn (0x46ebe0 with bag 0, not the automatic
  tab rule), so the whole stock is one list. Non-stackable items take one slot each.
* About 30–90 items for V2/V3; V1 has up to ~60 formulas, ~15 bombs and the extras.

### 3.2 Where the stock lives

* The spawn record keeps the bags at +0xf6c. When the spawn becomes a creature
  (0x6103a6) its bags are copied to the creature's bags (+0x11dc).
* The bags are **not** part of the network creature record (the record ends at object
  offset 0x1178). An online client receives NPCs from the server without stock, so every
  step (0x60c8ff … 0x60c9a6) each controller-kind-3 creature with **empty bags** is looked
  up by its home triple in the client's own generated zone and the spawn's bags are copied
  in. The stock is therefore generated independently, and identically, on every client and
  the server from the world seed.
* It never changes: buying does not remove anything (§3.4), selling does not add to it,
  and a respawn (e.g. after resting at the inn) copies the same spawn stock again.
  "Regeneration" never happens.

### 3.3 Opening the vendor window (0x4889e0, 0x4a2300)

On every R on a class 128–130 NPC: the talk partner G+0x8008d8 := the NPC; the vendor
lists are rebuilt for the current tab (§3.5); the vendor host G+0x8008c4 and the inventory
G+0x8008bc are shown; the vendor widget's tab := 0 (Buy); then the NPC's bags, item in hand,
money (+0x1304) and platinum (+0x1308) are copied into a **vendor shadow creature** inside G
(bags at G+0x800c0c, hand G+0x800c18/0x800c1c, money G+0x800d34, platinum G+0x800d38). The
window shows the shadow, never the NPC itself. The bubble (§2.4) is shown at the same
time; a vendor speaks random talk.

**Tabs** (`alpha-ui-screens.md` §8.1.3 mechanism; title "Vendor" on both):

| Tab | Icon | Shows |
|---:|---|---|
| 0 | `buy.png` | shadow bag 0 = the vendor's whole stock |
| 1 | `buy-back.png` | shadow bag 1 = the buy-back list, newest first |

Rows are the wide rows of §8.1.3 with the price at the right. There is no money row in the
vendor window; the player's money shows in the inventory window opened beside it.

**Rebuild** (0x4a2300): tab 1 → shadow bag 1 := the buy-back entries from the newest to the
oldest; tab 0 → recopy the talk partner's bags etc. (only if a partner is set). Then the
scroll range is recomputed.

### 3.4 Buy, sell, buy back (0x47b600)

Prices are the single price of `alpha-ui-screens.md` §8.1.1 for buying, selling and buying
back alike: **there is no vendor margin**.

* **Buy** (tab 0, right press on a stock row): if price > P's money → nothing (no sound).
  Else if P cannot carry it (0x43e4a0) → chat "You can't carry more of these items." in
  (1, 0.2, 0.2, 1) and sound 0x31 at P. Else one item is added to P's inventory (automatic
  bag), money −= price, sound **0x39** (`drop.wav`, volume 1, pitch 1) at P. The stock slot
  is **unchanged** — vendors have unlimited stock.
* **Sell** (right or middle press on an inventory slot or an equipment box while the vendor
  window is visible): one unit is taken (slot count − 1, cleared at 0), money += price,
  sound **0x3a** (`drop-coin.wav`, volume 0.75, pitch 1) at P; a new buy-back entry (count 1,
  a copy of the item) is appended; if the list then has more than **10** entries the
  oldest is dropped; the lists are rebuilt. Entries are never merged.
* **Buy back** (tab 1, right press on row *i*): the entry is the (count − 1 − *i*)-th from the
  oldest (row 0 = newest). For each unit of its count: if P cannot carry it → the chat
  message and sound 0x31, and stop; else if price ≤ money → add it, money −= price, else skip
  the unit. The entry's count −= units bought (removed at 0), then sound 0x39 (even if none
  was bought) and rebuild. A whole stack is bought back with one press, as far as money
  allows.
* The buy-back list (G+0x800d3c, count G+0x800d40) is **global**: it is shared by all
  vendors, survives closing the window, and is not saved.
* The vendor's money (+0x1304) is copied into the shadow but never read: vendors have
  unlimited money.

### 3.5 Network and saves

Buying, selling and buying back send **nothing**. The player's money and platinum are
creature fields outside the network record (object +0x1304, +0x1308), changed only on the
client and kept in the character save (`alpha-saves.md` fields 16, 17). The server never
sees a trade; the only packet caused by talking to a vendor is the kind-2 record, which the
server ignores for non-innkeepers. (For AlphaVulkan's server-authoritative mode this is a
deliberate parity gap; see §6.)

### 3.6 Closing

Every frame (0x4964e7 …): if the Identification or Adaption window is visible the inventory
is forced visible; if the inventory is **not** visible, the vendor, Identification and
Adaption windows are hidden. So closing the inventory (B/I, or a right press over no widget)
closes the vendor. If a talk partner is set and P is **≥ 25 blocks** from it, the partner is
cleared — the window itself stays open, and buying from the shadow copy still works (as
read).

---

## 4. Classes 131 and 137

* **131, the identifier**: R shows the host G+0x8008f8 (350 × 285, right of the inventory)
  with the **EnchantWidget** (0x44e910, G+0x8008fc), whose title is **"Identification"**;
  its selection is reset to (−1, −1) and +0x168 := 1. An item is put in by right/middle
  pressing it in the inventory. The fee is max(1, ⌊price/2⌋) (0x450920) of the placed item and
  must be ≤ P's money; for **Leftovers** (type 14) a new item is generated with `rand()`
  (0x47ca47 … 0x47cab8) and added to the inventory, sound 0x2e (`disenchant2.wav`)
  (*believed*; details not traced). The identifier's V3 weapon stock is never shown.
* **137**: R shows the host G+0x800900 (350 × 400) with the **AdaptionWidget** (0x40ecd0,
  G+0x800904), title **"Adaption"** (`itemframe`, `rightarrow` templates); its result items
  carry the " (adapted)" flag. Class 137 NPCs are the mode-7 marker NPCs of towers and
  landing places (`alpha-worldgen-structures.md` §6.2). Details not traced.

The customization bench (static kind 77, `alpha-interaction.md` §5.3) uses different
widgets (G+0x8008f4, G+0x8008dc).

---

## 5. Worked examples

### 5.1 An innkeeper at night

22:00, P presses R at an innkeeper; at least one bubble is free.

1. Packet-6 kind 2 with the innkeeper's home triple is queued. The owner of the simulation
   sees class 132 and 22:00 ≥ 18:00: day + 1, time 7:00, `levelup2.wav` at volume 0.75, all
   spawned NPCs removed (and later respawned from their spawns).
2. The client opens a bubble with one page of 21 segments: "You" "can" "rest" "in" "our"
   "inn" "between" (white), "six" "'o" "clock" "in" "the" "evening" (violet), "and"
   (white), "six" "'o" "clock" "in" "the" "morning" (violet), "." (white).
   Length = 40 × (73 characters + 21) = **3760 ms**; 31 voice blips (one per three of the 94
   units). Lines break where the measured widths pass 220 px (§1.6); expect about three
   lines (an estimate — exact breaks depend on the `resource1.dat` advances).
3. Without another press the bubble closes after elapsed > 6760 ms on its page.
4. At 14:00 the same press shows the same bubble and the simulation does nothing.

### 5.2 A resident with random talk

Talk kind 0, id low word **1** (a test value). `srand(1)` → 41, 18467, 6334, 26500:
item 41 mod 5 = 1 → Lamp (`@itemvendoritem_plural` = "Lamps"); pet 18467 mod 45 = 17 →
type 40 BlueSlime (`@pet_singular` = "Slime"); monster 6334 mod 11 = 9 → 110 Hell Demon;
entry `random:` + (26500 mod 30 + 1) = **`random:11`**: "My [@pet_singular $creature] is
adorable!" → segments "My" (white), "Slime" (red), "is", "adorable", "!" (white).
Length 40 × (18 + 5) = 920 ms; closes 3 s later.

### 5.3 A resident with pet-food talk

Talk kind 3, id low word 1000. `srand(1000)`, first draw 3304; 3304 mod 45 = 19 → type 38
PinkSlime ("Slime" / "Slimes"), food (20, 38) = "Pink Jelly" / "Pink Jellies": "Do you know
what [Slimes] like to eat? [Pink Jellies], of course!" ("Slimes" red, "Pink" "Jellies"
blue, "," and "?" and "!" separate segments with a 3-px gap).

### 5.4 A resident with "explored"

Talk kind 4, target = a level-appropriate Ruins location named *N*: tags include
`#scenery_the`, so page 1 = "Have" "you" "already" "explored" "the" "Ruins" "of" "*N*" "?"
("Ruins of *N*" green); page 2 = "Look" "," "I've" "marked" "the" "location" "on" "your"
"map" (violet) "!". Page 1 length 40 × (42 + |*N*|) ms (1880 ms for a 5-letter name), page 2
40 × 46 = 1840 ms. The location's zone is marked discovered on the world map. If P's level
is more than 2 away from the location's level the target is dropped and the resident uses
random talk instead (with its own seed, as in 5.2).

### 5.5 "There is nothing special."

R on a table: segments "There" "is" "nothing" "special." (white), length 40 × (22 + 4) =
**1040 ms**, eight blips, closes after elapsed > 4040 ms; the bubble sits over P's head.

### 5.6 A weapon vendor's first rounds

Assume the draws at the start of V3 are the `srand(123)` stream (440, 19053, 23075, …; the
real stream is whatever the zone's generation has reached, §5.5.9). Following §5.5.9 V3
exactly:

| Round | L | Picked (subtype, material, rarity, seed) | Price (copper) |
|---:|---:|---|---:|
| 0 | 1 | Staff (3, 10), wood, 0, 29057 | 40 |
| 0 | 1 | Greatmace (3, 17), iron, 0, 4480 | 40 |
| 0 | 1 | Fist (3, 4), iron, 0, 10421 | 20 |
| 1 | 2 | Sword (3, 0), iron, 0, 21774 | 22 |
| 2 | 2 | Boomerang (3, 8), wood, 0, 11861 | 44 |
| 2 | 2 | Dagger (3, 3), iron, 0, 16067 | 22 |
| 2 | 2 | Longsword (3, 5), iron, 0, 6527 | 44 |

(The second candidate's subtype is d % 3 + 15: Greatsword 15, Greataxe 16, Greatmace 17.)
Prices: F(1, 0) = 1 → one-hander 2 × 10 = 20, two-hander 40;
F(2, 0) = 2^(3·(1 − 1/1.05)) = 1.1040895 → 22 and 44. Each row is shown with its generated
name (`alpha-item-names.md` §3, adjective from the seed), "Power …" and " (adapted)".

---

## 6. Implementation notes for AlphaVulkan

* Keep the bubble pool at four and choose the last free one; keep the dialogue widget out
  (it is never shown). Hold the creature by **id**, not pointer, and free the bubble when the
  creature disappears (fixes B-1 without visible change in normal play).
* Pre-parse speech entries into the node tree once at load and render per call; the
  grammar of §2.1 is small and the `$`-colours are fixed. Keep the `rand()` order of §2.4
  exactly, seeded by the NPC id, so the same NPC says the same thing as in the original.
* Generate vendor stock with the zone (worldgen spec) and copy it into NPCs on both host
  and clients; never send it. Buying/selling stays client-side for wire compatibility with
  original Alpha peers; a mod or a server-authoritative option can hook the three
  operations of §3.4 (one function each) without touching the window code.
* Mod hooks worth exposing: the text chooser (§2.4), the stock generator per class (§3.1)
  and the class → window table (0x4889e0).

---

## 7. Open points

* **B-1** A bubble keeps a raw creature pointer; when the NPC is removed (e.g. the inn
  rest removes all NPCs while the innkeeper's bubble is open) the original reads freed
  memory until the pages run out. Behaviour in the game is unverified.
* **B-2** The exact node-pivot arithmetic of §1.5 step 5 (box centred, tail on the head
  point) is derived from the template numbers; confirm with a screenshot.
* **B-3** Whether the dictionary XML reader turns CR LF into LF (the file has CR LF line
  ends; CR is not a tokenizer delimiter). AlphaVulkan should treat CR as whitespace.
* **B-4** The Identification fee, the Leftovers roll and the Adaption rules (§4).
* **B-5** Whether the POI entries written to resident spawns (+0x60) are zone coordinates
  as the client assumes (worldgen spec marks their contents *inferred*); if they were fx,
  "explored" would never appear.
* **B-6** dt units: the frame delta added to elapsed is taken to be milliseconds (40 ms per
  character, 3 s pages); a recording would confirm the typing speed (25 characters/s).

---

## 8. Corrections to other specs

* `alpha-interaction.md` §5.2: G+0x800940 is never non-zero, so the "close the finished
  bubble" branch of Talk is dead — every Talk press goes to 0x4882e0 (§1.3 here). Class 131
  is the **identifier** and opens the **Identification** window (EnchantWidget), not a
  trainer; class 137 opens the **Adaption** window, not a pet stable. This resolves its
  open point I-6.
* `alpha-ui-screens.md` §8.1.2 table: G+0x8008f8 / G+0x800900 are the Identification
  (EnchantWidget, title "Identification") and Adaption hosts, opened by classes 131 / 137;
  §8.1.8 "Right press — vendor item": the vendor's stock is not decremented and buy-back
  buys the whole stack (§3.4 here).
