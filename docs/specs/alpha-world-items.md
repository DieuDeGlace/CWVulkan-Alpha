Status: FINAL / STABLE / HELD

# Alpha: items lying in the world

Behavioural specification of world items in Cube World Alpha. Evidence: the original
`Cube.exe` (client, image base 0x400000) and `Server.exe`; regions missing from the
exported listing (parts of the frame routine and of the client world step) were read
from the image bytes directly. All addresses are RVAs; unless marked "Server.exe" they
are RVAs in `Cube.exe`. *Believed* marks readings that were not fully confirmed; open
points are collected in §7.

Related specs (pointers only, not repeated here):

* record layout and network section 7: `alpha-net-packets.md` §3.7;
* zone save layout: `alpha-saves.md` §5.2;
* server step, kill drops and packet 6: `alpha-server-simulation.md` §3 step 5, §6.4, §8;
* choosing the item target (E) and the highlight: `alpha-interaction.md` §2.4, §4.4;
* models per item: `alpha-creatures-models.md` §6.3;
* generated items: `alpha-worldgen-vegetation.md` §4, `alpha-worldgen-structures.md`;
* item lights and sunlight strength: `alpha-rendering.md` §2.2, §10.1;
* the bags, the pick-up key and the chat line: `alpha-ui-screens.md` §8.1.1, §8.1.9.

Notation: *T* is a world item's landing timer (+0x13c, milliseconds); *s* its scale
(+0x134); *a* its angle (+0x130, degrees); (*sx*, *sy*, *sz*) the three 32-bit size
fields of the item's model at model +0x44, +0x48, +0x4c (*believed* to be the model's
x, y, z voxel extents); *r* = one `rand()` result divided by 32767; G is the client
game object, W = G+0x2e4 the world; "the clock" is the world's millisecond counter
W+0x8000bc (read by RVA 0x88b80); "the day" is the world's day counter W+0x800160.
Positions are signed 64-bit fixed point, 65536 units per block.

---

## 1. The record (zone +0x30, 0x148 = 328 bytes each)

The zone holds its world items as a contiguous array of 328-byte records at zone +0x30
(the save writer divides the array's byte length by 328).

| Off | Type | Meaning | Default (RVA 0x4a970) |
|---|---|---|---|
| 0x000 | item (280 bytes) | the item | empty item: type/subtype word 0, count word +0x10 = 1, attached list (+0x14, 256 bytes) zeroed, attached count +0x114 = 0 |
| 0x118 | 3 × i64 | position. x, y = the item's centre on the ground plane, z = its base | not written |
| 0x130 | f32 | angle *a*, degrees, about the vertical axis | not written |
| 0x134 | f32 | scale *s*: **world blocks per model voxel** | **1/14 = 0.071428575** (bit pattern 0x3d924925) |
| 0x138 | u8 | origin tag: 0 dropped, 1 structure/cave loot, 2 vegetation item | 0 |
| 0x13c | i32 | landing timer *T*, ms | 0 |
| 0x140 | i32 | hide timer, ms (*believed*; see §3.4) | 0 |
| 0x144 | i32 | day the item was dropped; −1 for generated items | −1 |

**+0x138.** The tag is copied (RVA 0x4a8c0, 0x4b3b0), compared (RVA 0x49ce0), written
by the zone save writer (RVA 0x4e340) and set by the generators. It has one behavioural
reader: the settling pass of the zone generator (§4.3), which lowers every generated
record whose tag has bit 0 clear. Nothing reads it to choose a drawing pose or offset.

**What the scale means.** +0x134 is the absolute size of one model voxel in blocks. The
drop routine (RVA 0x19aeb0) does not take an absolute scale: it multiplies the default
1/14 by a factor argument. The "scale 1.0 / 0.8 / 3.0 / 0.75" of drops are **factors of
1/14**:

| Source | Factor | +0x134 |
|---|---|---|
| player drop (packet 6 kind 6); equipped item or generated item from a kill (RVA 0x19b180) | 1.0 | 0.0714286 |
| creature-type material drop (RVA 0x19b180, switch on creature type +0x64), creature types 0x3a, 0x46, 0x47, 0x78–0x80, 0x86, 0x8b | 0.8 | 0.0571429 |
| the same drop for creature types 0x83–0x85 and 0x87–0x8a (all type 11 subtype 0 items) | 3.0 | 0.2142857 |
| recipe (type 2 item), coins, the type-14 item of §3.2, items from the dying creature's inventory (RVA 0x19b180) | 0.75 | 0.0535714 |
| vegetation mushroom (type 11 subtype 15) | default left | 0.0714286 |
| vegetation heartflower / frozen heartflower | written directly | 0.1 |
| structure / cave loot | written directly | 0.06 or 0.1 (structures spec; cave 0.06, §4.2) |

The 3.0 factor belongs to those creature types, not to the item kind: creature type 0x86
also drops a type 11 subtype 0 item (material 17) but at factor 0.8.

A generated 0.1 item is therefore 1.4 times the size of a factor-1 dropped item; a 0.06
item is 0.84 times.

---

## 2. Drawing (frame routine RVA 0xac260, item loop RVA 0xb2270 … 0xb2c87)

For every loaded zone in the frame's draw range, every record of the zone's array in
order, with *k* = the record's index in the array (the counter advances for every
record, hidden ones included):

1. **Hidden**: if +0x140 ≠ 0, skip (RVA 0xb2282). The light pass skips these items too
   (`alpha-rendering.md` §10.1).
2. **Model** = the item's model (RVA 0xec400, `alpha-creatures-models.md` §6.3). The draw
   loop has no "no model" test of its own. (The item-target pass of §2.6 does skip a
   record whose model lookup returns none.)
3. **Cull sphere**: radius *R* = *sx* · 0.7 · *s* blocks (RVA 0xb22a1 … 0xb22cb).
4. **Hop**: the drawn position *P* = the record position; if *T* > 0 (RVA 0xb22d6):
   *P*.z += 5 · (1 − ((*T* − 250) / 250)²) blocks (*T* − 250 is formed as an integer
   before conversion). See §2.4.
5. **Mana-cube filter** (RVA 0x3e550, with the creature at G+0x8006d0): an item of type
   25 passes only when its 32-bit field +0x04 equals the creature's mana-cube counter *c*
   (+0x1164), where a counter above 100 is first replaced by 100 + (*c* mod 100). Every
   other type passes. Each player sees only the mana cube meant for them.
6. **View test** (RVA 0x7f760): a sphere centred at *P* + (0, 0, *sz* · *s* / 2) blocks
   with radius *R*, together with a distance value taken from a frame local (*believed*
   the six view planes and the fog-based limit of `alpha-rendering.md` §10.1; W-1).
   Failing items are skipped.
7. **Model matrix** (§2.1), **material colour** (§2.2), **highlight** (§2.3), then the
   model is drawn (RVA 0x482a0, 0x48280, 0xe6df0), then the item's attached sub-cubes
   (§2.5).

### 2.1 Model matrix

Model space: voxel coordinates *v* with the model spanning [0, *sx*] × [0, *sy*] ×
[0, *sz*] (*believed*, W-6); centre *c* = (*sx*/2, *sy*/2, *sz*/2). The matrix helpers
post-multiply (each new operation applies to model space before the earlier ones) and
take degrees (factor 0.01745329 at RVA 0x2fcda4). Rotations are right-handed about the
model axes: Rz(θ) turns +x towards +y (RVA 0x24610); Ry(θ) turns +x towards −z (RVA
0x244f0); Rx(θ) turns +y towards +z (RVA 0x243d0).

The frame builds: identity, translate to *P* (camera-relative like all frame draws,
render origin G+0x1d8 / G+0x1e0), uniform scale *s* (RVA 0x24730), then a pose chosen
by the item type (index table RVA 0xbbbb4, jump table RVA 0xbbba4, types 3–25; types
outside 3–25 use the lying pose), then the tumble of §2.4. As a mapping from model space
to the world:

world(*v*) = *P* + *s* · Pose(Tumble(*v*))

| Types | Pose(*u*) | Look |
|---|---|---|
| 3 weapon, 4 chest armour (RVA 0xb28b6) | Rz(*a*) · ((0, 0, *sx*/2) + Ry(90°) · (*u* − *c*)) | **standing**: the model's x axis is vertical (model +x points down), its centre *sx*/2 voxels above the ground, so it stands on its end |
| 12 coin, 13 (RVA 0xb2943) | as types 3, 4 but with *a* replaced by *a* + 0.2 · clock | **standing on edge and spinning** at 0.2° per ms (200° per second) |
| 25 mana cube (RVA 0xb280b) | Rz(0.02 · clock) · ((0, 0, *sz*) + Rx(45°) · Ry(45°) · (*u* − *c*)) | **floating on a corner**: centre one model height above the position, tilted 45° and 45°, turning 20° per second; +0x130 is ignored |
| all others: 5–11, 14–24 and types outside 3–25 (RVA 0xb29df) | Rz(*a*) · (*u* − (*sx*/2, *sy*/2, 0)) | **lying**: centred on the position in x and y, base on the ground |

So the position is the centre of the item's footprint at ground level for every pose
except the mana cube, which hovers.

### 2.2 Material colour (RVA 0xb2516 … 0xb2589, 0xb2943 … 0xb29b3)

1. *l* = light value of the block that holds the **record** position (not the hopped
   one), block coordinates = position / 65536 truncated toward zero (RVA 0x2f570). The
   light query (RVA 0x718b0) returns 255 in a type-13 block, 0 in any block whose type is
   neither 0 (air) nor 2 (water), otherwise the block's first byte, raised to at least 5.
2. Colour = (1, 1, 1, (*l* / 255) · S), S = a frame-wide daylight factor computed from the
   world's time of day (G+0x800440; `alpha-rendering.md` §2.2). The fourth
   channel scales the sun/sky term in the shader.
3. **Glowing items** — type 11 subtype 19 (shimmer mushroom) and every type 18 (candles):
   the fourth channel is set to 1 (fully lit at night).
4. **Coins and type 13 sparkle** (inside the type 12/13 pose case): all four channels are
   multiplied by (0.5 · cos(0.4 · clock degrees) + 0.5)² + 1, a pulse between 1 and 2
   with a period of 900 ms (the clock is doubled as an integer, then scaled by 0.2).
5. The tumble is applied, the colour is copied, and the highlight (§2.3) is added.

### 2.3 Highlight (RVA 0xb2b7c)

If (zone x, zone y, *k*) equals the item target G+0x800a78 (three 32-bit fields compared
by RVA 0x68870), (0.25, 0.25, 0.25, 0.25) is added to the colour.

### 2.4 Landing hop and tumble

* **Hop** (only while *T* > 0; §2 step 4): height above the record position
  h(*T*) = 5 · (1 − ((*T* − 250)/250)²) blocks: 0 at *T* = 500, 5 at *T* = 250, 0 at
  *T* = 0. Dropped items start with *T* = 500 … 799 (§3.2), so for *T* > 500 the formula
  is **negative**: the item is drawn under the ground (down to −19.1 blocks at
  *T* = 799), rises through the surface at *T* = 500, peaks 5 blocks up and lands when
  *T* reaches 0. In practice the item appears to pop out of the ground.
* **Tumble** (only while *T* ≠ 0; RVA 0xb2a49 … 0xb2b5f), innermost in the matrix:
  Tumble(*v*) = *c* + Rx(*T*°) · Ry(0.7 · *T*°) · (*v* − *c*); the timer value in ms is
  used directly as degrees. It applies to every pose.

At *T* = 0 neither applies; the item rests in its pose. There is no idle bobbing; the
only idle motion is the spin of coins/type 13 and the turning of mana cubes.

### 2.5 Attached sub-cubes (RVA 0x71b60)

Skipped entirely when the item's type byte is 0. Otherwise a shine value is set for the
pass: 1 when the item's material byte (+0x0d) is 1, 11, 12 or 22, else 0 (RVA 0xc7be0);
it is reset to 0 afterwards. Then each of the item's attached entries (+0x14, 8 bytes
each, count +0x114) is drawn with the item's final matrix moved by the entry's three
signed bytes (+0, +1, +2: x, y, z in model voxels), with a colour derived from the
entry's material byte (+3) and the item's current colour (light, glow, sparkle and
highlight included) (RVA 0xc7250). The model drawn for every entry is the one held at
G+0x800730 (*believed* a single-cube model; W-5).

### 2.6 Box for the item-target test (RVA 0x99538 … 0x99c3b)

`alpha-interaction.md` §2.4 places the item model's box with the same construction as
the drawing (identity, camera-relative translate, scale *s*), with these differences
(index table RVA 0x9d520, jump table RVA 0x9d514, types 3–13):

* the record position (no hop, no tumble);
* types 3 and 4: the standing pose of §2.1;
* types 12 and 13: the standing pose **without** the Rz rotation;
* every other type, **including the mana cube**: the lying pose (Rz(*a*), centred in x,
  y, base on the ground).

The box is the eight model corners (0 or *sx*, 0 or *sy*, 0 or *sz*) through that matrix.

---

## 3. The world step (client, single player)

### 3.1 Per step: every item of every active zone (RVA 0x20cf40 … 0x20d2cf)

Active zones as in `alpha-server-simulation.md` §3. For each record, in array order,
with *dt* = the step's milliseconds:

1. **Type 16 (paintings) snap to a wall** (RVA 0x20cf53 … 0x20d1b9). Four tests in this
   order, each reading the position as left by the previous one; a "solid" block is any
   type other than 0 (air) and 2 (water) (RVA 0x3b480); ⌊ ⌋ is position / 65536
   truncated toward zero (RVA 0x2f570):
   * block at (x, y − 1, z) solid → y := ⌊y⌋ + 0.2 blocks, *a* := 180;
   * block at (x, y + 1, z) solid → y := ⌊y⌋ + 1 − 0.2 blocks, *a* := 0;
   * block at (x − 1, y, z) solid → x := ⌊x⌋ + 0.2 blocks, *a* := 90;
   * block at (x + 1, y, z) solid → x := ⌊x⌋ + 1 − 0.2 blocks, *a* := 270.

   A later test wins. This runs online too.
2. **Hide timer**: if +0x140 > 0: +0x140 −= *dt*, floor 0.
3. **Landing timer**: if *T* > 0: *T* −= *dt*; when it becomes ≤ 0: *T* := 0 and, **only
   when the world is offline** (W+0xb4 = 0), one `rand()` is drawn and a sound record is
   queued (step output +0x8, section 4; RVA 0x864c0) at the item position: sound 58
   (`drop-coin.wav`) for type 12, sound 59 (`drop-item.wav`) otherwise; field +0x10 =
   (draw · 0.1) / 32767 + 1 (*believed* pitch), +0x14 = 1. An online client still counts
   *T* down, so its hop and tumble play from the received value.

Sound numbers map directly to the sound loader's switch (RVA 0x84350, table RVA 0x85e90):
45 `pickup.wav`, 57 `drop.wav`, 58 `drop-coin.wav`, 59 `drop-item.wav`.

Nothing else happens to items per step. **Items never despawn while their zone is
loaded.**

### 3.2 Dropping into the world (RVA 0x19aeb0; Server.exe twin RVA 0xd2810)

Arguments: item, position *Q*, angle, scale factor *f*, changed-zone set. Callers in the
client: packet-6 kind 6 (§3.3), a further drop in packet-6 kind 3 (RVA 0x210f50; factor
1.0, with its own random x/y offset), and the kill drops (RVA 0x19b180, nine calls). The
Server.exe twin is called by the server's kill drops (Server.exe RVA 0xd2ae0).

1. A fresh record (§1 defaults); position := *Q*; angle := the argument; +0x134 :=
   (1/14) · *f*.
2. `rand()` → *T* := 500 + (draw mod 300).
3. +0x144 := the day.
4. Zone := the zone holding *Q* (block x, y = *Q* / 65536, zone = block / 256, both
   divisions truncate toward zero). If that zone is not loaded, **the item is lost**
   (nothing else happens; only the first draw has been consumed).
5. Copy the item into the record (RVA 0x2c5e0).
6. `rand()` → angle := (draw · 360) / 32767. **The angle argument is always overwritten**,
   so the "angle 0" of a player drop becomes random.
7. **Fall**: start at block z₀ = *Q*.z / 65536 truncated toward zero when *Q*.z ≥ 0, and
   that truncated quotient minus one when *Q*.z < 0 (so an exact negative block multiple
   starts one block below its own block). Walk down one block at a time while the block
   type (low 5 bits of block byte +3) is 0 or 2. Block reads use these implicit rules:
   block x or y outside 0 … 2²⁴ − 1, a missing zone, or z below the column's stored
   range → solid (type 1); z above the stored range → air when z > 0, water when z ≤ 0;
   inside the range, an empty block at z ≤ 0 whose flag bit 0x40 is clear reads as
   water. The item therefore passes through air and water and sinks to the sea floor.
   Position z := (found z + 1) · 65536 exactly. x and y are not changed (not centred on
   the block). If the start block is already solid the item is put on top of it.
8. Append the record to the zone's array (RVA 0x4d590) and add the zone to the
   changed-zone set (RVA 0x18e7d0).

At the end of the step every zone in the set gets its "items changed" flag (zone +0x75)
set and its whole array is queued for section 7 (RVA 0x22118d).

**Kill-drop draw order.** Immediately before each of its drop calls, RVA 0x19b180 (after
any draws that build the item) draws two values for an x/y offset of (1 − 2*r*₁, 1 − 2*r*₂, 0) blocks from the creature position, then one
more for an angle argument ((draw · 360) / 32767), before the two draws of the drop
routine; that angle draw is consumed and then overwritten by step 6. Coins (§5) and the
other 0.75-factor drops follow this pattern. When the creature's word +0x7e has a bit of
0x18 set, the coin drop is replaced by a 1-in-20 type-14 item (tier byte +0x0c, level
from creature +0x190), factor 0.75.

### 3.3 Packet-6 records in the creature's queue (RVA 0x2104ec … 0x211a91)

The client step reads each queued record of a creature *C* (queue at *C* +0x130c) and
switches on byte +0x128 (kinds 1–8, table RVA 0x222c20). Kinds 5 and 6 are handled only
offline (W+0xb4 = 0); an online client skips them. Offline the queue is emptied after the
loop (RVA 0x6f870). The client-side checks before queuing (E pressed, "can carry", chat
"You can't carry more of these items.") are in `alpha-ui-screens.md` §8.1.9.

**Kind 5, pick up (RVA 0x210746):**

1. Zone := the zone (record +0x118 = zone x, +0x11c = zone y, 32-bit); none → stop.
   Index *k* := record +0x120; stop unless 0 ≤ *k* < array size.
2. Mana-cube filter (RVA 0x3e550, as §2 step 5, with *C*); fail → stop.
3. Squared distance in blocks from *C* (+0x10) to the item position ≤ **16** (4 blocks;
   squared length RVA 0x24860, constant at RVA 0x345e78); else stop.
4. The zone joins the changed-zone set (RVA 0x19f070).
5. A **gift record** (*C*'s id from +0x08/+0x0c, then a copy of the item) is queued in
   the step output at +0x40 (RVA 0x86380; section 9 of `alpha-net-packets.md`).
6. If *C* is the local player (W+0xb8) and the item is **not** type 25: the item goes
   **straight into the bags** (RVA 0x6ebe0 on *C* +0x11dc with tab −1 = choose
   automatically, RVA 0x7fa30; §5). There is no carry test at this point.
7. One `rand()` is drawn and sound 45 (`pickup.wav`, field +0x10 = (draw · 0.1) / 32767
   + 1, +0x14 = 1) is queued at *C*'s position (step output +0x8, section 4).
8. The record is erased from the array (RVA 0x19c800); later items move down one index.

The gift record is then processed like a received section 9 (*believed*, W-4): type 13
→ experience += 10 × its +0x10; type 25 → mana-cube counter + 1, action 109, chat
"You receive a mana cube." and the skill-point message every fourth one; other items →
the chat announcement "You receive …" (`alpha-ui-screens.md` §8.1.9).

**Kind 6, drop from the hand (RVA 0x2119cc):** the drop routine (§3.2) with the record's
item, position = *C*'s position (+0x10), angle 0 (replaced by a random angle), factor
1.0, the step's changed-zone set; then sound 57 (`drop.wav`, default fields, no draw) at
*C*'s position. There is no random x/y offset for a player drop. Taking the item out of
the hand is done by the inventory screen before queuing (`alpha-ui-screens.md` §8.1.8).

### 3.4 The hide timer +0x140

The draw and the light pass skip items whose +0x140 is not 0, and the step counts it
down. The drop routine, the default record and the generators traced here leave it 0;
the save reader copies it. No writer of a non-zero value was identified (W-2).

### 3.5 Day-based cleanup when a zone loads (RVA 0x4cdb0)

There is no despawn during play. When a zone is generated, its saved state (if a save
exists) is applied by RVA 0x4cdb0, called from the zone generator (RVA 0x1e4850) at
RVA 0x1ede46:

* each saved item is kept only if its day (+0x144) ≥ **current day − 3**, or its day is
  negative (generated items, −1);
* the same rule filters the saved **block edits** by their day field;
* if the kept list differs from the freshly generated array (RVA 0x49ce0), the zone's
  array is replaced by the kept list and the zone's "items changed" flag (+0x75) is set.

So a dropped item survives as long as its zone stays loaded, and after an unload it
comes back only if the zone is regenerated within 3 game days of the drop (the counter
W+0x800160).

---

## 4. Generated items and the zone save

### 4.1 Vegetation items (RVA 0x1ecb6f … 0x1ed176, 0x1ed19e … 0x1ed2bc)

The decision rules are in `alpha-worldgen-vegetation.md` §4. The record built, with its
draws in this order (after the decision draws): angle draw, then subtype draw:

| Field | Grass (here = 4) | Snow (here = 10) |
|---|---|---|
| item type / subtype | 11 / 15 (mushroom) when the subtype draw is even, 22 (heartflower) when odd | 11 / 24 (frozen heartflower; the subtype draw always lands here) |
| item count +0x10 | 1 | 1 |
| position | (x + 0.5, y + 0.5, zt) blocks: the column's centre on top of the surface block (RVA 0x68fc0 subtracts 0.5 · (−65536)) | same |
| angle +0x130 | (draw / 32767) · 360 (division first) | same |
| scale +0x134 | 1/14 for subtype 15, **0.1** for subtype 22 | 0.1 |
| +0x138 | 2 | 2 |
| +0x13c, +0x140, +0x144 | 0, 0, −1 | 0, 0, −1 |

### 4.2 Structure and cave loot

Built the same way with the values in `alpha-worldgen-structures.md` (scale 0.06 with
+0x138 = 1; scale 0.1 with +0x138 = 2) and the cave builder (RVA 0x1d3298: scale 0.06,
+0x138 = 1; coins there are type 12, material 10, amount at +0x10: RVA 0x1d33f7,
0x1d33fe, 0x1d3405; see §5). Timers 0, day −1.

### 4.3 Regeneration, settling and saves

* Generated items are **re-created every time the zone is generated** from the seed
  (they are part of the generator output, appended to zone +0x30 in generator order:
  structures, then the vegetation pass with x outer, y inner).
* **Settling pass** (RVA 0x1ed8b4 … 0x1eda48, in the generator before the save is
  applied at RVA 0x1ede46): for every record of the array, a per-(type, subtype,
  material) counter is incremented (use not traced, W-7); then, if bit 0 of +0x138 is
  clear, the record is lowered by whole blocks while the block containing
  (x, y, z − 0.5 blocks) is not solid. Structure/cave loot (tag 1) is not moved.
* Then the zone save, if one exists, is applied as in §3.5 and replaces the array if it
  differs. So once a zone's items have changed (pick-up or drop sets +0x75), the save is
  the truth for that zone: picked-up generated items stay gone, dropped items come back
  (within the 3-day rule).
* The zone is saved (RVA 0x1a5050) if +0x75 is set or it has block edits. The writer
  (RVA 0x4e340) stores the item array only when +0x75 is set; otherwise it writes a count
  of 0.
* **Consequence as read** (W-3): a zone saved only for its block edits (+0x75 clear)
  stores a count of 0; on the next load the empty saved list differs from the generated
  array, so the generated items of that zone would disappear and +0x75 become set.

---

## 5. Coin kind

**Coins are type 12, subtype 0; the coin kind is the material byte +0x0d; the amount is
the 16-bit +0x10.** The subtype is not used. (This is the field `alpha-ui-screens.md`
§8.1.1 calls "subtype".)

* **Created** by the kill loot (client RVA 0x19b180; Server.exe twin RVA 0xd2ae0): the
  coin value *V* (truncated to an integer) is split as copper = *V* mod 100, silver =
  (*V* / 100) mod 100, gold = *V* / 10000; each non-zero part becomes one item, dropped
  in the order copper, silver, gold, each at factor 0.75: word +0x00 := 0x000C (type 12,
  subtype 0), byte **+0x0d** := **10 copper** (RVA 0x19bd64), **12 silver** (RVA
  0x19be5b), **11 gold** (RVA 0x19bf3e), word +0x10 := the part (RVA 0x19bd6b, 0x19be62,
  0x19bf45). Cave coins: §4.2.
* **Added to the bags** by RVA 0x6ebe0: an empty item (type 0) is ignored. If byte +0x00
  = 12 (RVA 0x6eca5) it reads byte **+0x0d** (RVA 0x6ecaa): 10 → money += amount; 12 →
  money += amount × 100 (RVA 0x6ecb9); 11 → money += amount × 10000 (RVA 0x6ecd5); money
  is the inventory's +0x128 (RVA 0x6ecbc, 0x6ecdb). A type-12 item with any other
  material is stored in the bags as an ordinary item. Type 13 adds its amount to +0x12c
  (RVA 0x6ecf9). The amount is the signed 16-bit +0x10 for types 12, 13, 20, 21, 23, 24,
  25 and type 11 except subtype 14 (and nominally type 0); it is 1 for everything else.
* The coin models follow the same byte: material 12 → silver model, 11 → gold, otherwise
  copper (`alpha-creatures-models.md` §6.3).

---

## 6. Worked examples

Model voxel sizes used below are illustrative values for the named models; they come from
the owner's content files, not from the executable (W-8).

### 6.1 A generated mushroom

Grass column x = 8,396,900, y = 8,397,000, surface top zt = 40; the item draw passes,
the angle draw is 16,384 and the subtype draw is even (subtype 15).

* Position: x = 8,396,900 · 65,536 + 32,768 = 550,299,271,168; y = 8,397,000 · 65,536 +
  32,768 = 550,305,824,768; z = 40 · 65,536 = 2,621,440.
* Angle = (16,384 / 32,767) · 360 = 180.0055°. Scale 1/14. +0x138 = 2, timers 0, day −1,
  count 1. The settling pass leaves it in place (the block below its base is solid).
* With a 6 × 6 × 7 model → 0.43 × 0.43 × 0.5 blocks, lying pose: footprint centred on
  (8,396,900.5, 8,397,000.5), base at z = 40, turned 180° about the vertical.
* Cull sphere radius 6 · 0.7 / 14 = 0.30 blocks, centre 0.25 blocks above the base. No
  hop.

A heartflower (subtype 22, scale 0.1) with a 13 × 13 × 8 model at the same place is
1.3 × 1.3 × 0.8 blocks.

### 6.2 The player drops a shield

The local player stands at (8,396,950.25, 8,397,010.75, 41.0) blocks on a solid block
at z = 40 and drops an iron shield (type 3, subtype 13, model 14 × 2 × 12).

* Drop routine: factor 1 → *s* = 0.0714286; draw 1 = 12,345 → *T* = 500 + 12,345 mod
  300 = 545; day = today; zone (32800, 32800) (8,396,950 / 256 = 32,800.6); draw 2 =
  8,192 → angle (8,192 · 360) / 32,767 = 90.0027°.
* Fall: z₀ = 41 is air, z = 40 is solid → position z = 41 · 65,536. x, y unchanged.
* Sound 57 at the player at once; when *T* reaches 0 (545 ms later) a third draw for the
  pitch and sound 59.
* Drawing at *T* = 545: hop = 5 · (1 − (295/250)²) = −1.96 blocks (below ground),
  tumble 545° about x and 381.5° about y. At *T* = 250: 5 blocks up. At rest: standing
  pose, the shield's 14-voxel x axis vertical: 1.0 block tall, centre 0.5 blocks above
  z = 41, turned 90° about the vertical.

### 6.3 A goblin's loot

The goblin of `alpha-server-simulation.md` §6.4 drops its shield and 28 copper.

* Shield (equipped item, factor 1.0): *s* = 0.0714286, as §6.2 but at goblin position +
  (1 − 2*r*₁, 1 − 2*r*₂, 0) blocks, then the fall; an angle draw precedes the drop
  routine's own two draws.
* Coins: one item, type 12, subtype 0, material 10, count 28, factor 0.75 → *s* =
  0.0535714. With an 8 × 8 × 2 `copper-coin` model: standing on its rim, 0.43 blocks
  across, centre 0.21 blocks above the ground, spinning 200° per second, brightness pulse
  ×1 … ×2 every 900 ms. Landing sound 58 when *T* reaches 0.
* Picking the coins up (within 4 blocks, E): money += 28 (offline straight into the
  bags), sound 45, the record is erased, and the chat announcement "You receive 28 …"
  with the copper coin name (`alpha-ui-screens.md` §8.1.9, names in
  `alpha-item-names.md`).

---

## 7. Open points

* **W-1** The distance argument and plane test of the item view test (RVA 0x7f760; frame
  local read at RVA 0xb2362); *believed* the fog-based limit of `alpha-rendering.md`
  §10.1.
* **W-2** +0x140: no writer of a non-zero value identified in either image; meaning of
  the timer unknown.
* **W-3** The "empty saved list wipes generated items" consequence of §4.3 follows from
  the save writer and the load comparison as read; the load path for a zero saved count
  was not traced end to end.
* **W-4** Whether the offline frame processes the step's section-9 and section-4 records
  through the same handlers as a world update (*believed* RVA 0x8f8a0 and the sound
  player); the chat line, experience/mana-cube effects and the sounds offline depend on
  it. The bag-refresh routine after an offline pick-up is not identified.
* **W-5** The model at G+0x800730 used for attached sub-cubes, and the meaning of the
  shine value.
* **W-6** Model size fields +0x44/+0x48/+0x4c taken as the model's x/y/z voxel extents
  (consistent with the poses: coins and shields stand on their long x axis).
* **W-7** The use of the per-(type, subtype, material) counter in the settling pass.
* **W-8** Model voxel sizes in §6 are content values, not confirmed from the executable.
* **W-9** Whether the packet-6 kind-3 drop (RVA 0x210f50) is the chest-opening path of
  `alpha-interaction.md` §6.1; the queued record's total size is given in
  `alpha-net-packets.md`, not confirmed here.
* **W-10** Whether the client's incoming-item queue of `alpha-ui-screens.md` §8.1.9 has
  any feeder besides the one named there; offline pick-ups bypass it (§3.3 step 6).

---

## Re-certification (2026-11-14)

Re-checked against `Cube.exe` and `Server.exe` (image bytes and listings). All addresses
restated as RVAs (image base 0x400000 subtracted).

**Confirmed:** record size 328 and default values of +0x134 … +0x144; item size 280;
scale factors 1.0 / 0.8 / 3.0 / 0.75 and the creature types 0x83–0x85, 0x87–0x8a for
3.0; drawing order (hidden, model, cull radius 0.7, hop, mana filter, view sphere,
matrix, light, glow, pose, tumble, highlight, draw, sub-cubes); rotation senses and
degree input; pose jump table contents for types 3–25; hop constants 250 and 5; tumble
angles *T* and 0.7·*T*; mana-cube counter reduction; light query rules (255, 0, minimum
5); glow types; coin pulse formula and 900 ms period; coin spin 0.2°/ms and mana-cube
turn 0.02°/ms; highlight key and +0.25; target-box pose table; painting snap order,
offsets and angles; hide and landing timer countdown; offline-only landing sound, ids
58/59 and pitch formula; sound id to file mapping 45/57/58/59; drop routine draw order
(*T* then angle), *T* = 500 + draw mod 300, day stamp, zone lookup and loss, fall walk
and landing height; kind 5 checks (offline, index range, mana filter, 16-block² limit),
gift record, direct bag insert for the local player except type 25, pickup sound,
erase; kind 6 drop and sound 57; changed-zone flag +0x75; day − 3 filter on items and
block edits; save condition and item list written only with +0x75; vegetation fields,
subtype and scale rules and the +0.5 centring; cave loot scale/tag and coin fields; coin
split, materials 10/12/11 and bag money arithmetic; amount rule; worked-example
arithmetic; Server.exe drop twin and its kill-drop caller.

**Corrected (old → new):**
1. **Reversal:** +0x138 "has no behaviour" → it controls the generator settling pass
   (records with bit 0 clear are lowered onto solid ground); §1, §4.3.
2. 3.0 factor tied to "type 11 subtype 0" → tied to creature types 0x83–0x85, 0x87–0x8a;
   creature type 0x86 drops type 11 subtype 0 at 0.8.
3. Kind-5 record fields: zone +0x11c/+0x120, index +0x124 → zone +0x118/+0x11c, index
   +0x120.
4. Tumble condition "*T* > 0" → *T* ≠ 0 (the hop keeps *T* > 0).
5. Mana-cube field "word +0x04" → 32-bit +0x04.
6. Default record position "0" → not written by the default constructor.
7. Vegetation angle "360 · draw / 32767" → (draw / 32767) · 360, division first (the drop
   routine multiplies first); numeric precision only, draw order unchanged.
8. Fall start for negative z made exact (exact negative multiples start one block lower)
   and the implicit block rules stated from the code (out-of-range/missing → solid type
   1; above column air/water by sign of z; in-column empty block at z ≤ 0 without flag
   0x40 → water), replacing a pointer-only statement.
9. **Random-draw order:** added that each kill-drop call consumes an x/y offset pair and
   an angle-argument draw before the drop routine's two draws, the angle being
   overwritten; added that a lost drop consumes only the first draw, and that pickup and
   offline landing each consume one draw for the sound field.
10. Sub-cube pass: "model not traced, coloured by material" → skipped for empty items,
    colour from entry material combined with the item's current colour, model at
    G+0x800730 (identity still believed).
11. "Every drop goes through it (… chests …)" → actual client callers listed (kinds 6 and
    3, kill drops); chest attribution moved to W-9.
12. Coin handling: added that a type-12 item with a material other than 10/11/12 is
    stored as an ordinary item and that empty items are ignored.
13. 0.75-factor sources: "digested leftovers" label → the type-14 item that replaces
    coins for creatures with +0x7e bits 0x18; "inventory of a dying hostile" → items in
    the dying creature's inventory.
14. Daylight factor S described from the code as a time-of-day-derived frame value.

**Removed:** advice addressed to an implementer ("an engine must skip items without one",
"an engine wanting the look of the original keeps it", "an engine must still honour it");
verification procedures in the open points ("confirm by capture", "verify in a game
run"); the editorial instruction to change another spec; the decompiler-export
provenance wording; the 300-byte queue record size (pointer to `alpha-net-packets.md`
kept, W-9); the unconfirmed claim that the incoming-item queue is fed only by
disenchanting (W-10); the bag-refresh routine attribution (W-4).

**Unresolved:** W-1 … W-10 above.
