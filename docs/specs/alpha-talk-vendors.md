Status: FINAL / STABLE / HELD

# Alpha talking: speech bubbles, what NPCs say, vendors

Behavioural contract for what happens after the talk press in Cube World **Alpha**:
the speech-bubble pool, typing, paging, placement and sounds; how an NPC's words are
chosen from `dict_en.xml` and how the dictionary markup is parsed and rendered; the
innkeeper; vendor stock origin, the vendor window, buy / sell / buy-back; and the two other
town windows (classes 131 and 137).

Evidence: `Cube.exe` (32-bit, image base 0x400000, SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`), with
`Server.exe` (image base 0x400000, SHA-256
`1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`) where named, and the
extracted `dict_en.xml` of `data4.db`. Every routine is cited as an **RVA in Cube.exe**
unless another image is named. No code is reproduced and no game text is quoted;
dictionary entries are named by key only. *Believed* marks readings that are not fully
proven; open items are collected in §6.

Companion contracts referred to by name only (not re-read for this contract):
`alpha-interaction.md`, `alpha-ui-screens.md`, `alpha-item-names.md`,
`alpha-worldgen-structures.md`, `alpha-server-simulation.md`, `alpha-loot-rolls.md`,
`alpha-worldgen-random.md`, `alpha-net-packets.md`, `alpha-saves.md`.

---

## 0. Conventions

* G = game object; W = world object at G+0x2e4; P = the local player creature, pointer
  at G+0x8006d0. P's level is the 32-bit field P+0x190.
* NPC creature fields used here: +0x08 64-bit id (its low 32 bits are the **talk seed**);
  +0x10 position (3 × i64); +0x60 controller kind (3 = NPC); +0x7e appearance flags;
  +0x80 size vector (3 × f32, blocks); +0x140 **class** byte; +0x1b0 home triple (zone x,
  zone y, spawn index, 3 × i32); +0x1d8 **talk kind** byte; +0x1dc, +0x1e0 **talk target**
  zone x, zone y (i32); +0x1180 drawn-height offset (f32, blocks); +0x11dc bags;
  +0x11e8 / +0x11ec item-in-hand fields; +0x1304 money (i32); +0x1308 platinum;
  +0x1350 drawn position (3 × i64).
* Positions are i64 in 1/65536 block ("fx").
* Times are the integer per-frame delta that the frame update adds to timers; read as
  milliseconds (§6 B-6).
* "Outlined text": each string is drawn twice at the same place, first in its fill colour
  with a black stroke of the given radius, then in its fill colour with stroke 0. All
  bubble text uses font `resource1.dat`.
* `rand()` / `srand()` are the C runtime generator (state · 214013 + 2531011, result
  (state >> 16) & 0x7FFF). All draws named here come from that single shared stream.
* Sound ids are those of the sound dispatcher at RVA 0x84350: 0x1d `levelup2.wav`,
  0x1e `missioncomplete.wav`, 0x2e `disenchant2.wav`, 0x31 `human-voice01.wav`,
  0x32 `human-voice02.wav`, 0x39 `drop.wav`, 0x3a `drop-coin.wav`.

Town NPC classes (byte +0x140). The window column is from RVA 0x889e0; the stock column
from the zone generator (RVA 0x1af170), which sets the spawn class and calls the stock
generator for that class:

| Class | Window on talk | Stock generator |
|---:|---|---|
| 128 (0x80) | vendor | V1 (RVA 0x1c9b30) |
| 129 (0x81) | vendor | V3 (RVA 0x1ca0a0) |
| 130 (0x82) | vendor | V2 (RVA 0x1c8390) |
| 131 (0x83) | identification (§4) | V3 (never shown) |
| 132 (0x84) | none; innkeeper text (§2.4) | none |
| 137 (0x89) | adaptation (§4) | none |
| any other | none | none |

The roles of classes 132–137 (innkeeper, craft-house NPCs, residents, marker NPCs) are as
described by `alpha-worldgen-structures.md` and are not re-derived here.

---

## 1. Speech bubbles

### 1.1 Objects

Built once at start-up inside RVA 0x59c40 (bubble construction RVA 0x61a68 … 0x61cc5):

* The template named `speech` is looked up under the in-game root (G+0x800884). If it
  exists, the node list G+0x800920 is sized to **4** and the widget list G+0x80092c to the
  same count; each node is a clone of the template attached to the in-game root, and each
  gets its own **SpeechWidget** (constructor RVA 0xe5c90, allocation 0x1e0 bytes). So **at
  most four bubbles exist at once**, shared by NPCs and the player.
* A fifth SpeechWidget (G+0x800938) is built on an empty node G+0x80093c resized to
  400 × 400: the "dialogue" widget. It is advanced every frame (§1.4) but nothing is ever
  placed in it (*believed*), and the "open dialogue" pointer G+0x800940 is only ever
  written with 0 (RVA 0x5a345, 0x96a3b, 0x97552). Every branch that tests G+0x800940 ≠ 0
  is therefore dead.

**SpeechWidget fields** (constructor RVA 0xe5c90):

| Offset | Meaning | Constructor value |
|---|---|---|
| +0x148 | the node it draws into | argument |
| +0x160 … +0x168 | **pages**: a vector of 8-byte page records; each page is a list of **segments** (wide string + RGBA colour) | empty |
| +0x16c | **elapsed** time on the current page | 0 |
| +0x170 | current **page index** | 0 |
| +0x174 | typed-character count seen by the previous draw (for the voice blip) | not set; zeroed whenever a bubble is opened or advanced |
| +0x178 … +0x180 | answer strings (24-byte wide strings) | empty |
| +0x184 | selected answer | 0 |
| +0x188 | the **creature** the bubble belongs to (raw pointer; 0 = free) | 0 |
| +0x190 … +0x1a4 | the bubble's world **anchor** (3 × i64 fx) | |
| +0x1a8 | font name `resource1.dat` | |
| +0x1c0, +0x1c4 | text size **14**, outline stroke **3** | |
| +0x1c8, +0x1cc, +0x1d0 | layout cursor x, y, characters consumed (reset every draw) | |
| +0x1d4 | **40** — time units per character | |
| +0x1d8 | back pointer to G | argument |

The widget rectangle is set with corner (0, 0), size (250, 200), inner origin (10, 10)
and inner extent (230, 180) (RVA 0x227c00); the width query used for wrapping (RVA
0x227d50) evaluates to **250** for these values.

### 1.2 Length of a page

The **length** of the current page (RVA 0xe6550) is 40 × the sum over its segments of
(segment character count + 1). A page index below 0 or at/after the page count gives
length 0. A page is **fully typed** when elapsed ≥ length (RVA 0xe6530).

### 1.3 Opening a bubble for an NPC (RVA 0x882e0)

Inputs: the NPC and a **restart** flag. Two call sites exist: the talk press (call at
RVA 0x96a4f, restart = false) and the per-frame event-record pass (call at RVA 0x906d5,
restart = true; §1.4 step 1).

For an NPC **without** appearance flag 0x80:

1. The four bubbles are scanned for one owned by this NPC (+0x188 = the NPC). If one is
   found and restart is false: if its page is fully typed → page index + 1, elapsed := 0,
   +0x174 := 0 ("next page"); otherwise elapsed := the page length ("finish typing").
   Stop. If one is found and restart is true, that bubble is rebuilt from step 3.
2. Otherwise a free bubble (+0x188 = 0) is chosen: the pool is scanned in order and the
   **last** free one wins. None free → stop (nothing is shown).
3. Build the text (§2.4) into a fresh page vector, including the map mark of §2.5.
4. If the page vector is empty or its first page has no segment → stop (no bubble).
5. Show the bubble's node; +0x188 := the NPC; anchor := the NPC's drawn position
   (+0x1350); pages := the new pages; elapsed := 0; +0x174 := 0; page index := 0; answers
   cleared; selected answer := 0.

For an NPC **with** appearance flag 0x80 (+0x7e & 0x80): step 1 is the same (an owned
bubble is advanced or finished). Otherwise, if a free bubble exists (or restart is true
with an owned one), no text is built and no bubble is shown; instead every location
record of the NPC's home region (region of (home zone x ÷ 64, home zone y ÷ 64), 8 × 8
records of 0x68 bytes from region +0x14018) whose type (+0x18) ≠ 0 and whose level
(+0x24) ≤ P's level + 2 is marked discovered on the world map (RVA 0x1fc160) at map
coordinates (record x ÷ 65536 ÷ 256, record y ÷ 65536 ÷ 256), using the record's i64
fields +0x00 and +0x08. No generator observed sets flag 0x80 on an NPC (*believed* dead).

### 1.4 Opening the player's own bubble (RVA 0x88030)

Called from the object talk press (call at RVA 0x97563, restart = false) with a static
object's kind:

1. If P already owns a bubble, it is advanced or finished exactly as in §1.3 step 1,
   **whatever the static's kind**.
2. Otherwise the last free bubble is taken; none free → nothing. If the static kind is
   1, 2, 3, 10, 16, 18, 19 or 69 → nothing.
3. Otherwise: show the node; +0x188 := P; anchor := P's position (+0x10, not the drawn
   position); pages := **one page** made from a fixed four-word English sentence built
   into Cube.exe (wide string at RVA 0x301db8), split at whitespace into white
   (1, 1, 1, 1) segments (its final full stop stays attached to the last word; there is
   no punctuation splitting here, unlike §2.1); elapsed, +0x174 and page index := 0;
   answers cleared; selected answer := 0.

### 1.5 Every frame (frame update RVA 0x88ee0)

In this order:

1. **Event-record pass** (RVA 0x90651 … 0x9081b): a temporary list is assembled from
   game lists including G+0x800638 and G+0x800640 and walked. A record with a non-zero
   field +0x18 and kind byte +0x25 = 1 names a creature by the 64-bit id at +0x08; if
   that creature exists in W, §1.3 runs for it with restart = true. Kind 2 records refer
   to a location and produce a message keyed `objective` and sound 0x1e. The origin of
   these records is not established here (§6 B-7).
2. **Time** (RVA 0x9081b … 0x908c9): every bubble's elapsed += dt, then the dialogue
   widget's.
3. **Window and partner check** (RVA 0x964e7 …; §3.6).
4. **Auto-advance and close** (RVA 0x966e2 … 0x9688b), for each of the four bubbles and
   then the dialogue widget: if it has no answers and elapsed > length + **3000**
   (strictly) → page index + 1, elapsed := 0, +0x174 := 0. Then, if the page index ≥ the
   page count → hide its node and +0x188 := 0 (free). A page therefore stays **3000 units
   after it has finished typing**, and the bubble closes 3000 units after its last page
   finished. (The dialogue widget, having no pages, is hidden and freed every frame.)
5. **Talk press on an NPC** (RVA 0x96922 … 0x96a54), on the talk-key edge when the
   current target is a creature of controller kind 3 (further gating conditions of
   `alpha-interaction.md` apply): the packet-6 kind-2 record carrying
   the NPC's home triple (+0x1b0) is queued on P (P+0x130c), then §1.3 runs with
   restart = false. The record is queued whatever the bubble does.
6. **Placement** (RVA 0x96f37 … 0x97329), §1.6, for every bubble that has a creature.
7. On the same talk-key edge, after placement: the talk-partner window logic (call at
   RVA 0x973e6 to RVA 0x889e0, §3.3) runs while P's field +0x16c is positive (*believed*:
   P alive), and the object talk press (§1.4) follows later in the frame.

There is **no distance limit** and no other timeout for a bubble: it follows its creature
until its pages run out. The bubble keeps a raw creature pointer and nothing frees the
bubble when the creature is removed (§6 B-1).

### 1.6 Placement over the head

For a bubble with creature *C*:

1. **Follow**: a factor s is computed (RVA 0xac150) by starting from 0 in double precision
   and applying s := s + (1 − s) × 0.02 exactly dt times (dt as an integer), then rounding
   to f32; this equals 1 − 0.98^dt up to that rounding. Target = C's drawn position +
   (0, 0, −C.+0x1180) converted to fx. The anchor moves the fraction s of the way from its
   current value to the target (fx arithmetic; exact fixed-point rounding is §6 B-2).
2. **Head point**: H = anchor + an upward offset of (C.size.z × 0.5 + 1) blocks
   (C.+0x80 element 2).
3. **Project** H relative to the camera with the frame's view-projection; if the projected
   depth is ≤ 0 the node is hidden for this frame, otherwise shown.
4. **Screen point** (*believed* exact form): Px = ndc.x · W/2 + W/2,
   Py = −ndc.y · H/2 + H/2, with W, H the renderer's width and height.
5. The node is translated by (Px, Py) − (**125**, **170**) − the node's pivot
   (RVA 0x97268 … 0x972db). With the template's box 250 px wide and 150 px high and a
   20-px tail under its centre (*believed* from the `gui.plx` template, §6 B-2), this
   places the box at x = Px − 125 … Px + 125, y = Py − 170 … Py − 20 with the tail tip on
   (Px, Py).

### 1.7 Drawing the text (draw RVA 0xe5f90, per segment RVA 0xe65a0)

Nothing is drawn when the page index is outside the pages. Otherwise only the current
page is drawn. Per draw the cursor (x, y) := (0, 0) and consumed := 0; "glue" := true.
Let V = ⌊elapsed / 40⌋. For each segment *S* of the page, in order:

1. An empty segment is skipped (drawing continues).
2. If V < consumed: stop drawing this page. Otherwise, if the font is unavailable, the
   segment is skipped (drawing continues).
3. w = the advance width of the **whole** segment at size 14.
4. punct = *S* is exactly one of `.` `-` `,` `;` `!` `?` `:`.
5. If not glue: x += **3** if punct, else **14**.
6. glue := (*S* is exactly `-`).
7. **Wrap** (only if not punct): if x + w > 250 − **30** = 220 → x := 0, y += **18**.
8. Visible part: the whole segment, or only its first V − consumed characters if
   V < consumed + |*S*|. The layout always uses the full width.
9. Draw it as outlined text, size 14, stroke 3, in the segment's colour, at
   (node origin x + x + **10**, node origin y + y + **25**).
10. x += w; consumed += (visible characters) + 1.

So: 14-px text, 18-px lines, at most 220 px per line; punctuation never wraps. Paging
happens only at `%` (§2.2); text that does not fit simply overflows the box.

**Voice blip**: after the segments, if ⌊+0x174 / 3⌋ ≠ ⌊consumed / 3⌋, sound 0x32 is played
at **P's position** (not the speaker's), volume 1, pitch 1 + 0.5 · r / 32767 with
r = one `rand()` draw (from the shared stream); then +0x174 := consumed. At most one blip
per draw; none once the page is complete; a page of N units (Σ(characters + 1)) can give
at most ⌊N / 3⌋ blips.

**Answers** (+0x178, never filled in Alpha; *believed*): when the page is fully typed and
answers exist they are drawn on one row at y = widget height − **75**, starting at x = 15,
outlined size 14, the selected one (+0x184) cyan (0, 1, 1, 1) and the others white, each
next one 20 px plus the previous width further right. While answers exist the auto-advance
of §1.5 step 4 is off.

### 1.8 Pressing talk again (summary)

| State of the NPC's bubble | Talk on the NPC |
|---|---|
| none | open a new bubble (§1.3), if one of the four is free |
| typing | finish the page at once |
| page typed, more pages | next page (typing restarts) |
| last page typed | page index runs past the end → closed by the next auto-advance check |

The player's own bubble is advanced the same way by a talk press on any static object
(§1.4 step 1), including the excluded kinds.

---

## 2. What NPCs say

### 2.1 Speech entries and their parse tree

`dict_en.xml` `speech` elements (keyed by attribute `key`) are read by the dictionary
loader (RVA 0xe1970) and each text is parsed once (RVA 0xda850) into a tree of text nodes
(0x44 bytes each, constructor RVA 0xda3b0). A node has: a **condition set** (+0x04; tokens
starting with `#`), a **style set** (+0x14; tokens starting with `$`), its own **text**
(+0x1c), a parent (+0x34) and an ordered child list (+0x38). The root is an empty node.

**Tokenizer.** The text is scanned character by character, including its terminating
NUL. A token ends at a **delimiter**: space, tab, line feed, NUL, `{`, `}`, `[`, `]`,
`|`, or one of the punctuation characters `.` `:` `-` `,` `;` `!` `?` `/` `(` `)`.
Carriage return is **not** a delimiter. At each delimiter, in this order:

1. `{` → a **group** node is added as a child of the current node and entered; then a
   first **alternative** node is added to the group and entered (two levels).
2. `[` → a **span** node is added as a child of the current node and entered.
3. The pending token (the characters since the previous delimiter), handled if it is
   non-empty, or if the delimiter is punctuation (then possibly empty):
   * starts with `#` → added to the current node's condition set;
   * starts with `$` → added to the current node's style set;
   * otherwise → a **word** node with that text is added as a child of the current node;
     if the delimiter is punctuation, a second word node holding just that character is
     added after it. An empty token before punctuation gives an empty word node, which
     renders nothing.
4. `|` → with fewer than 2 levels open, parsing is abandoned; otherwise leave one level
   and add/enter a new alternative under the node now current.
5. `}` → with fewer than 3 levels open, parsing is abandoned; otherwise leave two levels.
6. `]` → with fewer than 2 levels open, parsing is abandoned; otherwise leave one level.

Abandoning keeps the tree built so far (only a final bookkeeping pass, RVA 0xda770, is
skipped; *believed* to have no effect on rendering). `%` and `'` are ordinary characters. Because
`{` and `[` are handled before the pending token, markup is only well formed after a
delimiter.

### 2.2 Rendering a tree into pages (RVA 0xe4350; driver RVA 0xe4a20)

Inputs: the tree, a **tag set** and a **variable map** (§2.3), and the page vector (if it
is empty, one empty page is created first, RVA 0xe4a20). A node renders with result
"continue" or "stop":

1. A null node → "continue". If the node's condition set is non-empty and **none** of its
   entries is in the tag set → skip the node, "continue". If at least one is present the
   node is "matched".
2. If the node's text is non-empty:
   * text exactly `%` → start a new empty page; nothing else is appended for this node;
   * otherwise the colour starts as white (1, 1, 1, 1); then each entry of the **parent's**
     style set is compared, in set order, with the seven names below, and a match sets the
     colour (a later match overrides): `$creature` (1, 0.3, 0.3, 1) · `$name`
     (0.6, 0.3, 1, 1) · `$item` (0.3, 0.3, 1, 1) · `$object` (1, 0.5, 0.3, 1) · `$zone`
     (0.3, 1, 0.3, 1) · `$stress` (0.6, 0.3, 1, 1) · `$number` (0.3, 1, 0.3, 1);
   * text not starting with `@` → its whitespace-separated words are appended to the last
     page as segments of that colour (RVA 0xe5740; whitespace includes carriage return);
   * text starting with `@` → the whole token is looked up in the variable map; if found,
     the value's words are appended in that colour; otherwise nothing.
3. The children are rendered in order; the loop stops at the first child whose result is
   "stop".
4. Result: "stop" if the node was matched, otherwise "continue".

Consequences: in a group, the first alternative that carries a present tag is rendered
and ends the group; an alternative without tags is always rendered and does not end the
group. Styles colour only the direct children of the node that carries them.

### 2.3 Variables and tags

Three setters fill the tag set and variable map from dictionary `name` entries; *p* is a
prefix passed by the caller:

* **creature** (RVA 0x194c80, creature type *t*): the name entry for *t*; for each word *g*
  of its `tags` attribute the tag `#` *p* `_` *g*; for each child element *c* the variable
  `@` *p* `_` *c* = that child's text.
* **item** (RVA 0x195010): the same, with the name entry chosen by the item's type and
  subtype bytes only (material, level and seed are not used).
* **location** (RVA 0x1953a0, location record *L*): the name entry chosen by
  (L+0x18, L+0x1c); tags as above; each child text has its first `@` replaced by the
  location's generated name (from L+0x20) before being stored as `@` *p* `_` *c*.

The key tables that map type numbers to name entries belong to `alpha-ui-screens.md` and
`alpha-item-names.md`.

### 2.4 Choosing the text (RVA 0x882e0 → RVA 0xe4bd0)

**Innkeeper** (class 132): the entry with key `innkeeper` (key string RVA 0x301dec) is
rendered with no variables and no tags, day or night. No `rand()` draw is made, the text
chooser RVA 0xe4bd0 is not called and the map mark of §2.5 is skipped. In the shipped
dictionary this entry yields one page of 21 segments and 73 characters (two bracketed
parts styled `$stress`).

**Every other NPC** (talk kind *k* = +0x1d8; target (zx, zy) = +0x1dc, +0x1e0):

0. *Target check* (in RVA 0x882e0, before any draw): cx = zx ÷ 8 and cy = zy ÷ 8
   (C division, truncating toward zero); if both are in 0 … 8191 and region
   (⌊8cx / 64⌋, ⌊8cy / 64⌋) of W's region table is loaded, the location record
   (cx mod 8, cy mod 8) of that region is read; if its level (+0x24) is above P's level
   + 2 or below P's level − 2, (zx, zy) := (−1, −1).
1. `srand`(the NPC id's low 32 bits).
2. Location lookup (RVA 0x87da0) at (zx ÷ 8, zy ÷ 8) with truncating division, so −1
   maps to cell 0; the lookup returns a record only if that cell is in range and its
   region is loaded.
3. **k = 4** and the record exists with type (+0x18) ≠ 0 → location variables with prefix
   `scenery`, entry **`explored`** (two pages in the shipped dictionary, separated by `%`;
   §6 B-3). **No** `rand()` draw. Done.
4. **k = 3** → pet = entry [`rand()` mod n] of the world's tameable-type list (W+0x88 …
   W+0x8c, n = its length; `alpha-loot-rolls.md` §4 gives its contents); creature
   variables with prefix `creature` from the pet; item variables with prefix `food` from
   the item (type 20, subtype = pet type); entry **`petfood`**. Exactly one draw. Done.
5. Otherwise (k not 3 or 4, or k = 4 without a usable record) — **random talk**, four draws
   in this order:
   1. `rand()` mod 5 selects the item (type, subtype) = (11, 12), (24, 0), (23, 0),
      (23, 1), (1, 7) for 0 … 4 → item variables with prefix `itemvendoritem`;
   2. pet = tameable entry [`rand()` mod n] → creature variables with prefix `pet`;
   3. monster creature type = 108, 109, 114, 115, 119, 111, 113, 112, 116, 110, 117 for
      `rand()` mod 11 = 0 … 10 → creature variables with prefix `monster`;
   4. entry key **`random:`** followed by the decimal number (`rand()` mod 30 + 1).

   All four draws are made whichever entry is chosen. In the shipped dictionary the
   thirty `random:` entries use `@itemvendoritem_plural` (entry 5), `@pet_singular`
   (11, 12, 19) and `@monster_plural` (21); the rest use no variables.

The same NPC therefore says the same thing on every press as long as its id, its target's
validity for P's level and the tameable list are unchanged.

Which talk kind and target an NPC has comes from world generation
(`alpha-worldgen-structures.md`; not re-derived here, §6 B-5).

### 2.5 The map mark

For every non-innkeeper NPC, after choosing the text, RVA 0x882e0 calls the world-map
mark (RVA 0x1fc160 on the map at G+0x800d44) with (zx, zy): if map coordinates are within
0 … 65535 and the map cell exists (RVA 0x202440; cells of 0x34 bytes), its bit 0 at
cell +0x30 is set and, only if it was clear, the map's discovered counter (+0x8000bc) is
incremented. Then, if the cell exists and W's region for (zx ÷ 64, zy ÷ 64) is loaded, the
16-byte zone summary at region + 0x18 + 16 · ((zx mod 64) · 64 + (zy mod 64)) is copied
to cell +0x10. With (zx, zy) = (−1, −1) nothing happens. The mark is made for every
non-innkeeper NPC with a usable stored target, whatever entry was chosen.

### 2.6 Quests

Talk produces no quest dialogue. Cube.exe contains no strings for the `title:…` or
`speech:<class>:…` dictionary keys; its `objective:` strings are mission-type keys
(monster, invasion, boss, dungeon, gang variants) and `description:` is a separate key
prefix. The class-chain speech entries of the dictionary are unused by Talk.

### 2.7 The innkeeper and the inn

Talking to an innkeeper shows the `innkeeper` bubble (§2.4) **and** queues the packet-6
kind-2 record with the NPC's home triple (§1.5 step 5). The rest rule applied by whoever
owns the simulation is specified in `alpha-server-simulation.md` and is not re-verified
here (§6 B-8). The client shows no rest message of its own: the bubble text is the same
day or night.

---

## 3. Vendors

### 3.1 Stock: who generates it

The stock is made by **world generation** when the town zone is generated (zone generator
RVA 0x1af170, which sets the spawn class at spawn +0x30 and then calls the generator):

| Class | Generator | Content (summary) |
|---|---|---|
| 128 | **V1** (RVA 0x1c9b30) | (beyond the bag target and the extras' items, §6 B-12) first-round extras (a flask (11, 12), a lamp (24, 0), up to four extra lamps from four 1-in-10 … 1-in-40 checks, a hang glider (23, 0) and a boat (23, 1), one pet food (type 20) chosen by `rand()` mod 6), then per round formulas (type 2) and a 50 % bomb (1, 7) |
| 129, 131 | **V3** (RVA 0x1ca0a0) | weapons: 30 rounds; per round 11 candidates, then 1–3 picks without replacement |
| 130 | **V2** (RVA 0x1c8390) | armour and accessories: 30 rounds; per round 21 candidates, then 1–3 picks without replacement |

Facts confirmed for V2 / V3 (exact candidate construction and draw order are the subject
of `alpha-worldgen-structures.md`):

* **30 rounds** (k = 0 … 29). Each round's level is the first draw of the round: with
  f(u) = (1 / (1 − u) − 1) · 20 + 1, level = ⌊f(k/30)⌋ + `rand()` mod (⌊f((k + 0.99999)/30)⌋
  − ⌊f(k/30)⌋ + 1), at least 1, and forced to 1 in round 0 (the draw is still made). The
  level is stored in a 16-bit field. Bands therefore grow from 1 (round 0) to several
  hundred and more in the last rounds; no town level is used.
* V3's 11 candidates, in order: a one-handed weapon of subtype `rand()` mod 3, a
  two-handed weapon of subtype `rand()` mod 3 + 15, then subtypes 13, 3, 5, 4, 6, 8, 10,
  11 and 12 (five extra discarded draws precede subtype 8; subtype 12's material depends
  on one draw). Each candidate makes one discarded draw, three rarity draws
  (1 in 100, then +1 for 1 in 1000, +1 for 1 in 10 000, capped at 4) and a seed draw.
* Pick count = `rand()` mod 3 + 1; each pick draws `rand()` mod (remaining candidates).
* Weapons and armour carry byte +0x0e = 1 (the tooltip's adapted marker,
  `alpha-ui-screens.md`).
* Items are added to the spawn's bags (spawn +0xf6c) through RVA 0x6ebe0. V1 passes bag 0
  explicitly. V2 and V3 pass the automatic choice (RVA 0x7fa30), which maps item types
  3 … 9, 23 and 24 to bag 0, type 11 to bag 2, types 19 and 20 to bag 3 and all other
  types to bag 1; V3 items are type 3 and V2 items are armour / accessory types within
  3 … 9 (*believed* for V2), so they also land in bag 0. The whole stock is therefore
  bag 0.
* Count: 30–90 items for V2 / V3; V1 up to 60 formulas, about 15 bombs on average, plus
  the extras.

### 3.2 Where the stock lives

* When a spawn becomes a creature its bags (spawn +0xf6c) are copied to the creature's
  bags (+0x11dc): Cube.exe RVA 0x2103a6; Server.exe RVA 0x136166 does the same.
* During the world update (RVA 0x20c8ff … 0x20c9a6) each controller-kind-3 creature with
  **empty bags** is looked up by its home triple in the locally generated zone, and if
  that spawn index exists the spawn's bags are copied in. NPC stock therefore never needs
  to travel over the network; that the creature network record excludes the bags is the
  subject of `alpha-net-packets.md` (§6 B-9).
* Trading never changes the NPC's bags (§3.4), and a respawn copies the same spawn stock
  again.

### 3.3 Opening the vendor window (RVA 0x889e0; rebuild RVA 0xa2300)

RVA 0x889e0 looks up the current target id (G+0x800a70) among W's creatures and stores
the result as the talk partner G+0x8008d8 (0 if not a creature; the non-creature branch
belongs to `alpha-interaction.md`). Then, by the partner's class:

* **128–130**: in this order — the vendor lists are rebuilt for the vendor widget's
  current tab; the vendor host G+0x8008c4 and the inventory G+0x8008bc are shown; the
  vendor widget's (G+0x80095c) tab +0x1b4 := 0; the NPC's bags, item-in-hand fields
  (+0x11e8, +0x11ec), money (+0x1304) and platinum (+0x1308) are copied into a **vendor
  shadow** inside G (bags at G+0x800c0c, hand G+0x800c18 / G+0x800c1c, money G+0x800d34,
  platinum G+0x800d38). The window shows the shadow, never the NPC.
* **131**: §4. **137**: §4.

Because the talk press (§1.5 step 5) runs in the same frame, a vendor also speaks random
talk (or its stored kind).

**Tabs** (icons `buy.png`, `buy-back.png`):

| Tab | Shows |
|---:|---|
| 0 | shadow bag 0 = the vendor's whole stock |
| 1 | shadow bag 1 = the buy-back list, newest first |

**Rebuild** (RVA 0xa2300): tab 1 → shadow bag 1 is emptied and refilled with the buy-back
entries walked from the newest to the oldest; tab 0 → if a partner is set, the partner's
bags, hand, money and platinum are copied into the shadow again. Then the two list
refreshers run (RVA 0xc6350, 0xc64c0).

### 3.4 Buy, sell, buy back (RVA 0x7b600)

One price function (RVA 0xc76e0, specified as the item price in `alpha-ui-screens.md`) is
used for buying, selling and buying back: **there is no vendor margin**. The routine
handles two press codes (1 and 2) through the same trade code; their mouse-button mapping
is `alpha-ui-screens.md`'s (§6 B-10). Only P's money is ever read or changed.

* **Buy** (tab 0, press on a stock row): if price > P's money → nothing (no sound).
  Else if P cannot carry it (RVA 0x3e4a0) → a red chat line (1, 0.2, 0.2, 1) and sound
  0x31 at P. Else one copy is added to P's inventory with the automatic bag choice, money
  −= price, sound **0x39** (volume 1, pitch 1) at P's position. The stock row is
  **unchanged**: stock is unlimited.
* **Sell** (press on an inventory slot while the vendor host is visible): the slot's count
  −1 (slot cleared below 1); money += the price of one item; sound **0x3a** (volume 0.75,
  pitch 1) at P; a buy-back entry (count 1, a copy of the item) is appended; if the list
  then holds more than **10** entries the oldest is removed; rebuild. A second sell path
  (*believed*: an equipment box) gives the same money, sound and buy-back entry but
  empties the item entirely. Entries are never merged.
* **Buy back** (tab 1, press on row *i*): the entry is the (count − 1 − *i*)-th from the
  oldest (row 0 = newest). For each unit of its count: if P cannot carry it → the red chat
  line and sound 0x31, and stop; else if price ≤ money → add it, money −= price; else skip
  the unit. The entry's count −= units bought (entry removed at 0); then sound 0x39 (even
  if nothing was bought) and rebuild.
* The buy-back list (G+0x800d3c, count G+0x800d40) is **global**: shared by all vendors,
  kept when the window closes.
* The vendor's money copied into the shadow is never read: vendors have unlimited money.

### 3.5 Network and saves

The three trade operations change only client-side fields: P's money (+0x1304), P's
bags, the shadow and the global buy-back list (*believed*: no send is reached from RVA
0x7b600). The only packet caused by talking to a vendor is the kind-2 record of §1.5
step 5. Persistence of money and platinum is `alpha-saves.md`'s subject.

### 3.6 Closing (RVA 0x964e7 …)

Every frame: if the identification host (G+0x8008f8) or the adaptation host (G+0x800900)
is visible, the inventory host is made visible. If the inventory host is **not** visible,
the vendor host, the identification host and the adaptation host are hidden. So closing
the inventory closes the vendor.

If a talk partner is set and the squared distance between the partner's position (+0x10)
and P's position, converted to blocks, is **≥ 25** (that is, the distance is ≥ **5
blocks**), the partner is cleared. The windows stay open; buying uses the shadow and does
not consult the partner (*believed*).

---

## 4. Classes 131 and 137

* **131**: RVA 0x889e0 shows the identification host G+0x8008f8; the widget at G+0x8008fc
  gets its selection (+0x160, +0x164) := (−1, −1) and +0x168 := 1. The fee query
  (RVA 0x50920) returns 0 unless the selected inventory item exists and has item type 14;
  for such an item the fee is max(1, ⌊price / 2⌋). The rest of this window (placement of
  the item, the generated result, sound 0x2e) is §6 B-4. The identifier's V3 stock is
  never shown.
* **137**: RVA 0x889e0 shows the adaptation host G+0x800900 (widget at G+0x800904).
  Details are §6 B-4.

---

## 5. Worked examples

### 5.1 An innkeeper

Talk press at an innkeeper with at least one bubble free: the kind-2 record with the
innkeeper's home triple is queued; the bubble opens with one page of 21 segments and
73 characters, so length = 40 × (73 + 21) = **3760**; at most 31 voice blips. Without
another press the page advances (and the bubble closes) on the first frame with
elapsed > 6760.

### 5.2 A resident with random talk

Talk kind 0, id low word **1** (a test value). `srand(1)` gives 41, 18467, 6334, 26500:
41 mod 5 = 1 → item (24, 0); 18467 mod 45 = 17 → tameable entry 17 (if the list has 45
entries); 6334 mod 11 = 9 → creature type 110; 26500 mod 30 + 1 = **11** → entry
`random:11`, which in the shipped dictionary is a short sentence of four fixed segments
(2 + 2 + 8 characters plus a separate `!`) around a `$creature`-styled `@pet_singular`.
For a one-word pet name of p characters its length is 40 × (13 + p + 5).

### 5.3 A resident with pet-food talk

Talk kind 3, id low word 1000. `srand(1000)`, first draw 3304; 3304 mod 45 = 19 →
tameable entry 19 (for a 45-entry list); `food` comes from item (20, that type). No
further draws.

### 5.4 A resident with "explored"

Talk kind 4 with a target whose location record exists, has type ≠ 0 and a level within
P's level ± 2: no draw; the `explored` entry renders the location's singular name in
`$zone` green, preceded by an optional article word that appears only when the location
name entry carries the tag `the` (tag `#scenery_the`); the target zone is marked on the
world map. If the level is more than 2 away, the target becomes (−1, −1): the lookup then
uses cell (0, 0), and unless that cell yields a record with type ≠ 0 the resident uses
random talk with its own seed.

### 5.5 The player's own bubble

Talk on a table: one page of 4 white segments and 22 characters, length 40 × 26 =
**1040**, at most 8 blips, closed on the first frame with elapsed > 4040; anchored over P.

---

## 6. Open points

* **B-1** A bubble keeps a raw creature pointer; nothing frees the bubble when that
  creature is removed. What the original does if the creature is freed while its bubble
  is open is not established.
* **B-2** The `gui.plx` `speech` template geometry (box, tail, fills, outline widths) and
  the exact pivot arithmetic of §1.6 step 5 were not re-read in this pass; nor the
  fixed-point rounding of the anchor follow, nor the exact projection-to-pixel formula.
* **B-3** Whether the dictionary reader keeps carriage returns. CR is not a tokenizer
  delimiter but is whitespace for the word splitter, so CR never becomes visible inside a
  word; however a `%` immediately followed by CR would form the token `%`+CR, which is not
  a page break and would render a visible `%` word. The `explored` entry's `%` stands
  alone on a CR LF-terminated line, so its page break depends on this.
* **B-4** The identification window beyond the fee query (item placement, the generated
  result and its draws) and the adaptation window are not traced.
* **B-5** How world generation assigns talk kinds and targets, and whether the targets are
  zone coordinates.
* **B-6** The unit of the frame delta (taken as milliseconds: 40 per character, 3000 per
  page hold).
* **B-7** The origin and meaning of the event records processed in §1.5 step 1 (kind 1
  rebuilds an NPC's bubble with restart = true; kind 2 shows an `objective` message with
  sound 0x1e).
* **B-8** The inn rest rule (time window, day/time change, sound, NPC removal) was not
  re-verified against Cube.exe or Server.exe in this pass.
* **B-9** That the creature network record ends before the bag field, and the save fields
  holding money and platinum.
* **B-10** The mapping of RVA 0x7b600's press codes 1 and 2 to mouse buttons, and which
  inventory/equipment targets reach the sell path.
* **B-11** The contents and length of the tameable list at W+0x88 (taken from
  `alpha-loot-rolls.md` as 45 entries) were not re-verified; the worked examples' entry
  indices assume 45.
* **B-12** V1's exact per-round content and draw order, and V2's candidate list, were only
  checked for their counts and bag targets here.

---

## Re-certification (2026-11-15)

Checked against Cube.exe (SHA-256 above; decompile listing and fresh dumpbin
disassembly of the image bytes), Server.exe for one routine, and the extracted
`dict_en.xml`. No reconstruction source was consulted.

**Confirmed**: bubble pool of 4 built from the `speech` template, 0x1e0-byte widgets,
fifth dialogue widget (400 × 400) and the dead G+0x800940 pointer (three zero writes
only); widget field offsets and constants 14 / 3 / 40, rectangle (0,0)/(250,200)/(10,10)/
(230,180) and width 250; page length rule; last-free-bubble choice; advance / finish rule;
3000 strict hold and close; placement constants 0.02, size.z × 0.5 + 1, depth ≤ 0 hide,
(125, 170) offset; text layout gaps 3 / 14, glue on `-`, wrap at 220, line step 18, text
offset (10, 25), typing by ⌊elapsed / 40⌋; voice blip sound 0x32 at P, pitch
1 + 0.5 · r / 32767; answer row constants; tokenizer delimiters, order of handling and
abandon thresholds; `%` page break; the seven style colours and parent-style rule;
`@` lookup; tag/variable name formats `#p_g` and `@p_c`; location `@` substitution;
innkeeper path (no draw, no chooser, no map mark); target level window ± 2; seeding by the
id's low 32 bits; draw counts and order for kinds 4, 3 and random talk; item, monster and
entry tables; dictionary has 30 `random:` entries with variables in 5, 11, 12, 19, 21;
map-mark coordinates and zone-summary copy; class → window and class → generator maps;
30 rounds, 11 / 21 candidates, 1–3 picks, rarity draws, adapted byte; spawn → creature
bag copy (both images) and client refill of empty NPC bags; shadow copy fields;
buy / sell / buy-back rules, sounds 0x31 / 0x39 / 0x3a with volumes, 10-entry global
buy-back list, row-to-entry mapping; no vendor margin; sound-id names.

**Corrected** (old → new):

1. Second caller of RVA 0x882e0 → was "the only caller, restart = false"; now the
   event-record pass at RVA 0x906d5 also calls it with restart = true (rebuilds an owned
   bubble). *Reversal.*
2. Talk-partner range → was "≥ 25 blocks"; now squared distance ≥ 25, i.e. **≥ 5 blocks**.
   *Gameplay number.*
3. Frame order → was time, auto-advance, presses, placement; now event pass, time,
   window/partner check, auto-advance, NPC talk press, placement, then window opening and
   object talk press.
4. Player bubble → was "called unless the static kind is excluded"; now an existing player
   bubble is advanced or finished for any static, and only a new bubble is suppressed for
   the excluded kinds.
5. Stock bag → was "bag 0, not the automatic rule"; now V1 passes bag 0, V2 / V3 pass the
   automatic rule, which yields bag 0 for their item types.
6. Voice blips → was "one per three characters" (31 for the innkeeper); now at most one
   per draw, at most ⌊N / 3⌋ per page, and each blip consumes one shared `rand()` draw.
   *Random-draw order.*
7. Map mark → counter increment only when the bit was newly set and the map cell exists.
8. +0x174 → was "initial 0"; the constructor does not set it (it is zeroed on open/advance).
9. Follow factor → was a closed form; now the iterative double-precision computation.
10. Identification fee → was max(1, ⌊price / 2⌋) of any placed item; now 0 unless the
    selected item has type 14.
11. Second sell path (*believed* equipment box) empties the item instead of decrementing a count.
12. Rendering of an empty segment skips it without ending the page; failed font load
    likewise.
13. B-3 reframed: CR is harmless for words; it matters only for `%` page breaks.

**Removed**: the implementation-advice section addressed to the replacement engine and its
mod-hook suggestions; advice on CR handling and on the replacement's server mode;
recording / screenshot verification suggestions; the "corrections to other specs" section
(its native facts are kept in §1.1, §1.3, §4); all quoted game text (dictionary sentences,
the built-in sentence, the chat line, window titles, item and creature names); the vendor
worked example (prices and generated rows) and the price-function numbers, which depend
on companion contracts not re-verified here; sign names and window sizes not re-read;
network field numbers and save field numbers.

**Unresolved**: B-1 … B-12 in §6.
