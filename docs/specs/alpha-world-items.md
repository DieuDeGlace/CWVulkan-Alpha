# Alpha: items lying in the world

Clean-room specification of world items in Cube World Alpha (`Cube.exe`, `Server.exe`),
written from the Ghidra export and checked in the assembly where the export lost the code
(the frame routine 0x4ac260 and the world step 0x60c510 only exist as assembly; their
switch cases were disassembled from the binary). RVAs are citations only. *Believed* marks
readings that were not fully confirmed.

Related specs, not repeated here:

* record layout and network section 7: `alpha-net-packets.md` §3.7;
* zone save layout: `alpha-saves.md` §5.2;
* server step, kill drops and packet 6: `alpha-server-simulation.md` §3 step 5, §6.4, §8;
* choosing the item target (E) and the highlight: `alpha-interaction.md` §2.4, §4.4;
* models per item: `alpha-creatures-models.md` §6.3;
* generated items: `alpha-worldgen-vegetation.md` §4, `alpha-worldgen-structures.md`;
* item lights: `alpha-rendering.md` §10.1;
* the bags, the pick-up key and the chat line: `alpha-ui-screens.md` §8.1.1, §8.1.9.

Notation: *T* is a world item's landing timer (+0x13c, milliseconds); *s* its scale
(+0x134); *a* its angle (+0x130, degrees); (*sx*, *sy*, *sz*) the item model's size in
voxels (model +0x44, +0x48, +0x4c; *believed* to be the .cub x, y, z sizes); *r* = one
`rand()` draw / 32767; "the clock" is the world's millisecond counter W+0x8000bc
(`vegetation-wind.md`); "the day" is the world's day counter W+0x800160.

---

## 1. The record (zone +0x30, 0x148 = 328 bytes each)

| Off | Type | Meaning | Default (0x44a970) |
|---|---|---|---|
| 0x000 | item (280) | the item | empty item, level/count +0x10 = 1 |
| 0x118 | 3 × i64 | position, 1/65536 block. x, y = the item's centre on the ground plane, z = its base | 0 |
| 0x130 | f32 | angle *a*, degrees, about the vertical axis | not set |
| 0x134 | f32 | scale *s*: **world blocks per model voxel** | **1/14 = 0.071428575** (0x3d924925) |
| 0x138 | u8 | origin tag: 0 dropped, 1 structure/cave loot, 2 vegetation item | 0 |
| 0x13c | i32 | landing timer *T*, ms | 0 |
| 0x140 | i32 | hide timer, ms (*believed*; see §3.4) | 0 |
| 0x144 | i32 | day the item was dropped; −1 for generated items | −1 |

**+0x138 has no behaviour.** It is only copied (0x44a8c0, 0x44b3b0), compared (0x449ce0),
saved (0x44e340) and set by the generators. Nothing reads it to choose a pose or an offset.

**What the scale means.** +0x134 is the absolute size of one model voxel in blocks. Code that
drops items (0x59aeb0) does not write it directly: it multiplies the default 1/14 by a
factor. So the "scale 1.0 / 0.8 / 3.0 / 0.75" of loot drops are **factors of 1/14**:

| Source | Factor | +0x134 |
|---|---|---|
| player drop (packet 6 kind 6), equipped item or random item from a kill | 1.0 | 0.0714286 |
| creature-type drop, most materials (0x59b180) | 0.8 | 0.0571429 |
| creature-type drop of type 11 subtype 0 (ore nuggets; creature types 0x83–0x85, 0x87–0x8a) | 3.0 | 0.2142857 |
| recipe, coins, digested leftovers, inventory of a dying hostile | 0.75 | 0.0535714 |
| vegetation mushroom (grass, subtype 15) | written directly | 0.0714286 (default left) |
| vegetation heartflower / frozen heartflower | written directly | 0.1 |
| structure / cave loot | written directly | 0.06 or 0.1 (structures spec) |

A generated 0.1 item is therefore 1.4 times the size of a dropped item; a 0.06 item is
0.84 times.

---

## 2. Drawing (0x4ac260, item loop 0x4b2270 … 0x4b2c87)

For every loaded zone in the frame's draw range, every record *k* of the zone's list, in
order:

1. **Hidden**: if +0x140 ≠ 0, skip (0x4b2282). The light pass skips these items too
   (`alpha-rendering.md` §10.1).
2. **Model** = item → model (0x4ec400, `alpha-creatures-models.md` §6.3). The draw itself
   does not test for "no model"; an engine must skip items without one.
3. **Cull sphere**: radius *R* = *sx* · 0.7 · *s* blocks (0x4b22a1 … 0x4b22cb).
4. **Hop**: the drawn position *P* = the record position; if *T* > 0 (0x4b22d6):
   *P*.z += 5 · (1 − ((*T* − 250) / 250)²) blocks. See §2.4.
5. **Mana-cube filter** (0x43e550 with the local player): an item of type 25 is drawn only
   when its word +0x04 equals the player's mana-cube counter *c* (+0x1164; *c* > 100 is first
   replaced by 100 + (*c* mod 100)). Every other type passes. Each player sees only the mana
   cube meant for them.
6. **View test** (0x47f760): sphere centred at *P* + (0, 0, *sz* · *s* / 2) with radius *R*
   must be inside the six view planes and within the view distance passed by the frame
   (*believed* the same fog-based limit as the lights, `alpha-rendering.md` §10.1).
7. **Model matrix** (§2.1), **material colour** (§2.2), **highlight** (§2.3), then the model
   is drawn (0x4482a0, 0x448280, 0x4e6df0), then the item's attached sub-cubes (§2.5).

### 2.1 Model matrix

Model space: voxel coordinates *v* with the model spanning [0, *sx*] × [0, *sy*] × [0, *sz*]
(*believed*, as for the other voxel models); centre *c* = (*sx*/2, *sy*/2, *sz*/2). Rotations
are right-handed about the world axes: Rz(θ) turns +x towards +y; Ry(θ) turns +x towards
−z; Rx(θ) turns +y towards +z (0x424610, 0x4244f0, 0x4243d0).

The frame builds the matrix as identity, translate to *P* (camera-relative like all frame
draws, render origin G+0x1d8 / G+0x1e0), uniform scale *s* (0x424730), then a pose chosen by
the item type (jump table 0x4bbba4 / 0x4bbbb4, types 3–25; types outside that range and
those listed as "lying" use the lying pose), then the tumble of §2.4. Written as a function
from model space to the world:

world(*v*) = *P* + *s* · Pose(Tumble(*v*))

| Types | Pose(*u*) | Look |
|---|---|---|
| 3 weapon, 4 chest armour (0x4b28b6) | Rz(*a*) · ((0, 0, *sx*/2) + Ry(90°) · (*u* − *c*)) | **standing**: the model's x axis is vertical (model +x points down), its centre *sx*/2 voxels above the ground, so it stands on its end |
| 12 coin, 13 (0x4b2943) | as types 3, 4 but with *a* replaced by *a* + 0.2 · clock | **standing on edge and spinning** at 0.2° per ms (200° per second) |
| 25 mana cube (0x4b280b) | Rz(0.02 · clock) · ((0, 0, *sz*) + Rx(45°) · Ry(45°) · (*u* − *c*)) | **floating on a corner**: centre one model height above the position, tilted 45° and 45°, turning 20° per second; +0x130 is ignored |
| all others: 5–11, 14–24 (0x4b29df) | Rz(*a*) · (*u* − (*sx*/2, *sy*/2, 0)) | **lying**: centred on the position in x and y, base on the ground |

So the position is the centre of the item's footprint at ground level for every pose except
the mana cube, which hovers.

### 2.2 Material colour (0x4b2516 … 0x4b2589, 0x4b2943 …)

1. *l* = light value of the block that holds the **record** position (not the hopped one):
   0x4718b0 returns 255 in a type-13 block, 0 inside a solid block, otherwise the block's
   light byte, at least 5.
2. Colour = (1, 1, 1, (*l* / 255) · S), S = sunlight strength (`alpha-rendering.md` §2.2).
   The fourth channel scales the sun/sky term in the shader.
3. **Glowing items** — type 11 subtype 19 (shimmer mushroom) and every type 18 (candles):
   the fourth channel is set to 1 (fully lit at night).
4. **Coins and type 13 sparkle**: all four channels × (0.5 · cos(0.4 · clock °) + 0.5)² + 1,
   a pulse between 1 and 2 with a period of 900 ms.
5. Highlight (§2.3) is added after this.

### 2.3 Highlight (0x4b2b7c)

If (zone x, zone y, *k*) equals the item target G+0x800a78 (0x468870), (0.25, 0.25, 0.25,
0.25) is added to the colour.

### 2.4 Landing hop and tumble

While *T* > 0:

* **Hop** (§2 step 4): height above the record position h(*T*) = 5 · (1 − ((*T* − 250)/250)²)
  blocks: 0 at *T* = 500, 5 at *T* = 250, 0 at *T* = 0. Dropped items start with *T* = 500 …
  799 (§3.2), so for *T* > 500 the formula is **negative**: the item is drawn under the
  ground (down to −19.1 blocks at *T* = 799), rises through the surface at *T* = 500, peaks
  5 blocks up and lands when *T* reaches 0. This is the game's arithmetic; in practice the
  item seems to pop out of the ground. (An engine wanting the look of the original keeps it.)
* **Tumble** (0x4b2a49 … 0x4b2b5f), innermost in the matrix: Tumble(*v*) = *c* + Rx(*T*°) ·
  Ry(0.7 · *T*°) · (*v* − *c*); the timer value in ms is used directly as degrees. It applies
  to every pose.

At *T* = 0 neither applies; the item rests in its pose. There is no idle bobbing; the only
idle motion is the spin of coins/type 13 and the turning of mana cubes.

### 2.5 Attached sub-cubes (0x471b60)

After the model, each used entry of the item's attached list (+0x14, 8 bytes each, count
+0x114) is drawn with the same matrix moved by the entry's three signed bytes (x, y, z, in
voxels), coloured by the entry's material byte (0x4c7250), with a shine value of 1 when the
item's material is 1, 11, 12 or 22 and 0 otherwise (0x4c7be0). *Which model the entries use
is not traced (believed the single-cube model).*

### 2.6 Box for the item-target test (0x499538 … 0x499c3b)

`alpha-interaction.md` §2.4 step 6 places the item model's box with the same matrix as the
drawing, with these differences (switch 0x49d514 / 0x49d520, types 3–13):

* the record position (no hop, no tumble);
* types 3 and 4: the standing pose of §2.1;
* types 12 and 13: the standing pose **without** the Rz rotation;
* every other type, **including the mana cube**: the lying pose (Rz(*a*), centred in x, y,
  base on the ground).

The box is the eight model corners (0 or *sx*, 0 or *sy*, 0 or *sz*) through that matrix.

---

## 3. The world step (single player)

### 3.1 Per step: every item of every active zone (0x60cf40 … 0x60d2cf)

Active zones as in `alpha-server-simulation.md` §3. For each record, in list order, with
*dt* = the step's milliseconds:

1. **Type 16 (paintings) snap to a wall** (0x60cf53 … 0x60d1b9). With block coordinates read
   from the current position, four tests in this order, each using the position as left by
   the previous one; a "solid" block is any type other than 0 (air) and 2 (water) (0x43b480):
   * block at y − 1 solid → y := ⌊y⌋ + 0.2, *a* := 180;
   * block at y + 1 solid → y := ⌊y⌋ + 0.8, *a* := 0;
   * block at x − 1 solid → x := ⌊x⌋ + 0.2, *a* := 90;
   * block at x + 1 solid → x := ⌊x⌋ + 0.8, *a* := 270.

   (⌊ ⌋ is the integer block part, truncated toward zero, 0x42f570.) A later test wins. Runs
   online too.
2. **Hide timer**: if +0x140 > 0: +0x140 −= *dt*, floor 0.
3. **Landing timer**: if *T* > 0: *T* −= *dt*; when it reaches ≤ 0: *T* := 0 and, **only when
   the world is offline** (W+0xb4 = 0), a sound is queued (section 4) at the item position:
   sound 58 `drop-coin.wav` for type 12, sound 59 `drop-item.wav` otherwise, field +0x10 =
   1 + 0.1*r* (*believed* pitch), +0x14 = 1. An online client still counts *T* down, so its
   hop and tumble play from the received value; the server sends the sound.

Nothing else happens to items per step. **Items never despawn while their zone is loaded.**

### 3.2 Dropping into the world (0x59aeb0)

Arguments: item, position *Q*, angle, scale factor *f*, changed-zone set. Every drop in the
game goes through it (packet 6 kind 6, kills 0x59b180, chests `alpha-interaction.md` §6.1,
and the server's twin 0x4d2810).

1. A fresh record (§1 defaults); position := *Q*; angle := the argument; +0x134 := (1/14) ·
   *f*.
2. `rand()` → *T* := 500 + (draw mod 300).
3. +0x144 := the day.
4. Zone := the zone holding *Q* (block x, y = *Q* / 65536, zone = block / 256, both divisions
   truncate). If that zone is not loaded, **the item is lost** (nothing else happens).
5. Copy the item into the record (0x42c5e0).
6. `rand()` → angle := 360 · *r* (DIVSS order: (draw · 360) / 32767). **The angle argument is
   always overwritten**, so the "angle 0" of a player drop becomes random.
7. **Fall**: start at block z₀ = ⌊*Q*.z⌋ (for negative z the code takes the truncated
   quotient minus one) and walk down one block at a time until a solid block is found
   (type not 0 and not 2). Reading blocks follows the implicit rules of
   `alpha-worldgen-terrain.md` §1.4: below a column and in missing zones the block is solid;
   above the stored column it is air (z > 0) or water (z ≤ 0). The item passes through air
   and water, so it sinks to the sea floor. Position z := (solid z + 1) · 65536 exactly. x
   and y are not changed (not centred on the block). If the start block is already solid the
   item is put on top of it.
8. Append the record to the zone's list (0x44d590) and add the zone to the changed-zone set
   (0x58e7d0).

At the end of the step every zone in the set gets its "items changed" flag (zone +0x75) set
and its whole list is queued for section 7 (0x62118d).

### 3.3 Packet-6 records in the creature's queue (0x6104ec … 0x611a91)

The client step reads each queued 300-byte record of a creature *C* (creature +0x130c) and
switches on byte +0x128 (table 0x622c20). Kinds 5 and 6 are handled only offline
(W+0xb4 = 0); an online client skips them and waits for the server. Offline the queue is
emptied after the loop (0x46f870). The client-side checks before queuing (E pressed, "can
carry", chat "You can't carry more of these items.") are in `alpha-ui-screens.md` §8.1.9.

**Kind 5, pick up (0x610746):**

1. Zone := the zone (rec +0x11c, +0x120); none → stop. Index *k* := rec +0x124; stop unless
   0 ≤ *k* < list size.
2. Mana-cube filter (0x43e550, as §2 step 5, with *C*); fail → stop.
3. Squared distance from *C* (+0x10) to the item ≤ **16** (4 blocks; 0x424860 is the squared
   length, constant 0x745e78 = 16); else stop.
4. The zone joins the changed-zone set (0x59f070).
5. A **gift record** (*C*'s id from +0x08/+0x0c, then a copy of the item) is queued for
   section 9 (0x486380).
6. If *C* is the local player (W+0xb8) and the item is **not** type 25: the item goes
   **straight into the bags** (0x46ebe0 with tab −1 = automatic, §5). There is no carry test
   at this point and the 100 ms incoming-item queue of `alpha-ui-screens.md` §8.1.9 is **not**
   used offline (in the code traced that queue is only fed by disenchanting, 0x47cc77).
7. Sound 45 (`pickup.wav`, field +0x10 = 1 + 0.1*r*) at *C*'s position, section 4.
8. The record is erased from the list (0x59c800); later items move down one index.

The gift record is then processed like a received section 9 (*believed*: the frame routine
handles the step's event set offline as it handles a world update online, 0x48f8a0): type 13
→ experience += 10 × its +0x10; type 25 → mana-cube counter + 1, action 109, chat
"You receive a mana cube." and the skill-point message every fourth one; other items → the
chat announcement "You receive …" (`alpha-ui-screens.md` §8.1.9). The bag refresh for the
local player is 0x4a14c0 (*believed*).

**Kind 6, drop from the hand (0x6119cc):** 0x59aeb0 (§3.2) with the record's item,
position = *C*'s position (+0x10), angle 0 (replaced by a random angle), factor 1.0, the
step's changed-zone set; then sound 57 (`drop.wav`, default fields) at *C*'s position,
section 4. There is no random x/y spread for a player drop. Taking the item out of the hand
is done by the inventory screen before queuing (`alpha-ui-screens.md` §8.1.8).

### 3.4 The hide timer +0x140

The draw and the light pass skip items whose +0x140 is not 0, and the step counts it down.
No routine traced here writes a non-zero value (all drop paths and generators leave 0, the
loaders copy it). *Believed* a "hidden while … " timer kept for a feature that Alpha never
uses; an engine must still honour it for records received from a server or a save.

### 3.5 Day-based cleanup when a zone loads (0x44cdb0)

There is no despawn during play. When a zone is generated, its saved state (if a save
exists) is applied at the end of the generator (call at 0x5ede46) by 0x44cdb0:

* each saved item is kept only if its day (+0x144) ≥ **current day − 3**, or its day is
  negative (generated items, −1);
* the same rule filters the saved **block edits** by their day field;
* if the kept list differs from the freshly generated list (size or any field, 0x449ce0),
  the zone's list is replaced by the kept list and the zone's "items changed" flag (+0x75) is
  set.

So a dropped item survives as long as its zone stays loaded, and after an unload it comes
back only if the zone is reloaded within 3 days of the drop (game days, the counter W+0x800160).

---

## 4. Generated items and the zone save

### 4.1 Vegetation items (0x5ecb6f … 0x5ed176, 0x5ed19e … 0x5ed2bc)

The decision rules are in `alpha-worldgen-vegetation.md` §4. The record built:

| Field | Grass (here = 4) | Snow (here = 10) |
|---|---|---|
| item type / subtype | 11 / 15 (mushroom) or 22 (heartflower) | 11 / 24 (frozen heartflower; the subtype draw always lands here) |
| item count +0x10 | 1 | 1 |
| position | (x + 0.5, y + 0.5, zt) blocks: the column's centre on top of the surface block (0x468fc0 subtracts −0.5 · 65536) | same |
| angle +0x130 | 360 · draw / 32767 | same |
| scale +0x134 | 1/14 for subtype 15, **0.1** for subtype 22 | 0.1 |
| +0x138 | 2 | 2 |
| +0x13c, +0x140, +0x144 | 0, 0, −1 | 0, 0, −1 |

### 4.2 Structure and cave loot

Built the same way with the values in `alpha-worldgen-structures.md` (scale 0.06 with
+0x138 = 1; scale 0.1 with +0x138 = 2) and the cave builder (0x5d3298: scale 0.06, +0x138 = 1;
coins there are type 12, material 10, see §5). Timers 0, day −1.

### 4.3 Regeneration and saves

* Generated items are **re-created every time the zone is generated** from the seed (they
  are part of the generator output, appended to zone +0x30 in generator order: structures,
  then the vegetation pass with x outer, y inner).
* Then the zone save, if one exists, is applied as in §3.5 and replaces the list if it
  differs. So once a zone's items have changed (pick-up or drop sets +0x75), the save is the
  truth for that zone: picked-up generated items stay gone, dropped items come back (within
  the 3-day rule).
* The zone is saved (0x5a5050) if +0x75 is set or it has block edits. The writer stores the
  item list only when +0x75 is set, otherwise *n* = 0.
* **Quirk as read**: a zone saved only for its block edits (+0x75 clear) stores *n* = 0; on
  the next load the empty saved list differs from the generated list, so the generated
  items of that zone **disappear** and +0x75 becomes set. *Not confirmed in a game run (open
  point W-3).*

---

## 5. Coin kind (resolves the conflict between `alpha-ui-screens.md` §8.1.1 and §6.4 / item names §7)

**Coins are type 12, subtype 0; the coin kind is the material byte +0x0d; the amount is the
16-bit +0x10.** The subtype is not used.

* **Created** by the kill loot (client 0x59b180; server twin in 0x4d2ae0): the coin value
  *V* is split as copper = *V* mod 100, silver = (*V* / 100) mod 100, gold = *V* / 10000; each
  non-zero part becomes one item: word +0x00 := 0x000C (type 12, subtype 0), byte **+0x0d** :=
  **10 copper** (0x59bd64), **12 silver** (0x59be5b), **11 gold** (0x59bf3e), word +0x10 := the
  part (0x59bd6b, 0x59be62, 0x59bf45). Cave coins: type 12, +0x0d = 10 (0x5d33f7, 0x5d33fe),
  +0x10 = the amount (0x5d3405).
* **Added to the bags** by 0x46ebe0: if byte +0x00 = 12 (0x46eca5) it reads byte **+0x0d**
  (0x46ecaa): 10 → money += amount; 12 → money += amount × 100 (0x46ecb9); 11 → money +=
  amount × 10000 (0x46ecd5); money is the inventory's +0x128 (0x46ecbc, 0x46ecdb). Type 13
  adds its amount to +0x12c (0x46ecf9). The amount is +0x10 for types 0, 12, 13, 20, 21, 23,
  24, 25 and type 11 except subtype 14; it is 1 for everything else.
* The coin models follow the same byte: material 12 → silver model, 11 → gold, otherwise
  copper (`alpha-creatures-models.md` §6.3).

`alpha-ui-screens.md` §8.1.1 should read "material (+0x0d)" where it says "subtype".

---

## 6. Worked examples

### 6.1 A generated mushroom

Grass column x = 8,396,900, y = 8,397,000, surface top zt = 40; the item draw passes, the
angle draw is 16,384 and the subtype draw is even (subtype 15).

* Position: x = 8,396,900 · 65,536 + 32,768 = 550,299,271,168; y = 8,397,000 · 65,536 +
  32,768 = 550,305,824,768; z = 40 · 65,536 = 2,621,440.
* Angle = 16,384 / 32,767 · 360 = 180.0055°. Scale 1/14. +0x138 = 2, timers 0, day −1,
  count 1.
* Model `mushroom` 6 × 6 × 7 voxels → 0.43 × 0.43 × 0.5 blocks, lying pose: footprint centred
  on (8,396,900.5, 8,397,000.5), base at z = 40, turned 180° about the vertical.
* Cull sphere radius 6 · 0.7 / 14 = 0.30 blocks, centre 0.25 blocks above the base. No hop.

A heartflower (subtype 22, scale 0.1, model 13 × 13 × 8) at the same place is 1.3 × 1.3 ×
0.8 blocks.

### 6.2 The player drops a shield

The local player stands at (8,396,950.25, 8,397,010.75, 41.0) blocks on a solid block at
z = 40 and drops an iron shield (type 3, subtype 13, model 14 × 2 × 12).

* 0x59aeb0: factor 1 → *s* = 0.0714286; draw 1 = 12,345 → *T* = 500 + 12,345 mod 300 = 545;
  day = today; zone (32800, 32800) (8,396,950 / 256 = 32,800.6); draw 2 = 8,192 → angle
  90.0027°.
* Fall: z₀ = 41 is air, z = 40 is solid → position z = 41 · 65,536. x, y unchanged.
* Sound 57 at the player at once; when *T* reaches 0 (545 ms later) sound 59.
* Drawing at *T* = 545: hop = 5 · (1 − (295/250)²) = −1.96 blocks (below ground), tumble
  545° about x and 381.5° about y. At *T* = 250: 5 blocks up. At rest: standing pose, the
  shield's 14-voxel x axis vertical: 1.0 block tall, centre 0.5 blocks above z = 41,
  turned 90° about the vertical.

### 6.3 A goblin's loot

The goblin of `alpha-server-simulation.md` §6.4 drops its shield and 28 copper.

* Shield (equipped item, factor 1.0): *s* = 0.0714286, as §6.2 but at goblin position +
  (1 − 2*r*₁, 1 − 2*r*₂, 0) blocks, then the fall.
* Coins: one item, type 12, subtype 0, material 10, count 28, factor 0.75 → *s* = 0.0535714.
  Model `copper-coin` 8 × 8 × 2: standing on its rim, 0.43 blocks across, centre 0.21 blocks
  above the ground, spinning 200° per second, brightness pulse ×1 … ×2 every 900 ms. Landing
  sound 58 when *T* reaches 0.
* Picking the coins up (within 4 blocks, E): money += 28 (offline straight into the bags),
  sound 45, the record is erased, and the chat announcement "You receive 28 …" with the
  copper coin name (`alpha-ui-screens.md` §8.1.9, names in `alpha-item-names.md`).

---

## 7. Open points

* **W-1** View distance argument of the item view test (frame local at 0x4b2362; *believed*
  the fog-based limit of `alpha-rendering.md` §10.1).
* **W-2** +0x140: no writer found; confirm by capture that it is always 0 from `Server.exe`.
* **W-3** The "empty saved list wipes generated items" quirk of §4.3: verify in a game run
  (edit a block in a zone with ground items, leave and come back).
* **W-4** Whether the offline frame processes the step's section-9 and section-4 records
  through the same handlers as a world update (0x48f8a0 and the sound player); the chat line
  and the pick-up sound offline depend on it.
* **W-5** The model used for attached sub-cubes (§2.5) and the meaning of the shine value.
* **W-6** Model size fields +0x44/+0x48/+0x4c taken as the .cub x/y/z sizes (fits the poses:
  coins and shields stand on their long x axis).
