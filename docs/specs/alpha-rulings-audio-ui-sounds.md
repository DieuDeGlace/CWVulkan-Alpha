Status: FINAL / STABLE / HELD

# Alpha ruling: interface sound panning, sound 0x58 at 0x47ba7e, sounds 0x39 / 0x3a / 0x3b

Scope: three conflicts between `alpha-ui-screens.md` §10 (interface sounds) and the
re-certified `alpha-audio.md` (§1.1, §1.2, §4, "Re-certification (2026-11-16)").
Images: client Cube.exe (SHA-256 84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF)
and Server.exe (SHA-256 1FFBCBCE251E1506A63C9CE18AB0009D5553FCBB9E6202BE2BC3A7D870D34FB1),
both 32-bit, image base 0x400000. Addresses below are virtual addresses in those images
(RVA = address − 0x400000). Ids are sound ids of the play routine 0x484350 (0x39 = 57,
0x3a = 58, 0x3b = 59, 0x58 = 88).

## Ruling 1: panning — `alpha-audio.md` wins

No voice ever receives a pan, an output matrix or per-channel volumes. Every sound plays
through XAudio2's default channel mapping to the single mastering voice.

* The play routine 0x484350 computes a pan value (constant −0.1 at 0x702ac8, view-space x
  divided by w, clamped to [−1, 1] with −1 at 0x745e8c; 0x485dd6 … 0x485e15) and passes it
  as the third argument of the engine's virtual slot +0x18 (call 0x485e53). The engine
  vtable at 0x71e23c holds 0x623610 in that slot.
* 0x623610 reads its handle (+0x8), volume (+0xc) and pitch (+0x14) arguments; the pan
  argument (+0x10) is never read. Its only voice calls, in order: create source voice
  (IXAudio2 slot +0x20, maximum frequency ratio 2.0, 0x62365f), set volume (+0x30,
  0x623675), submit buffer (+0x54, 0x623684), start (+0x4c, 0x623691), set volume again
  (+0x30, 0x6236a7), set frequency ratio = pitch (+0x68, 0x6236bd).
* No virtual call through voice slot +0x38 (channel volumes) or +0x40 (output matrix)
  exists anywhere in the audio code 0x622da0 … 0x6240b0; the complete list of voice
  method calls there is create, volume, submit, start, stop, state query, destroy and
  frequency ratio. The image imports no X3DAudio and contains no such library name.
* The listener has no orientation (`alpha-audio.md` §2), so there is no other stereo
  placement path.

Correct statement: sounds farther than the cut-off are not played; nearer ones are
attenuated linearly with distance; **none is panned**. `alpha-ui-screens.md` §10's word
"panned" is wrong; `alpha-audio.md` §1.1 step 5 / §1.2 step 5 are right.

## Ruling 2: sound 0x58 at 0x47ba7e — `alpha-audio.md` wins (skill lowered)

0x47ba7e lies in the mouse-press handler 0x47b600 (slot +0x18 of the vtable whose entry is
at 0x6ffde4), inside the skill-window block guarded by the byte G+0x8008f1 ≠ 0 (0x47b849).
The block loops over the 11 skill buttons (array G+0x800844, index 0 … 10, 0x47b8b0 …
0x47baa5) and acts on the hovered one (hit test 0x6294c0):

* Press argument 0 (the left-press path): the skill is **raised** (+1 at 0x47b9b4) and 0x55
  (menu-select) plays at the listener (0x47b9d8).
* Press argument non-zero (the right-press path, the same branch that holds the vendor
  sell of Ruling 3): the skill counter is at (skill object G+0x80090c) +0x160 + 4·index.
  If its value is greater than 5 it is lowered at once; otherwise it is lowered only when
  the dependent skill is 0 (index 6 needs +0x17c = 0, 7 needs +0x180, 8 needs +0x184,
  0 needs +0x164, 2 needs +0x16c, 4 needs +0x174; indices 1, 3, 5, 9, 10 have no
  dependent test) (0x47b9e2 … 0x47ba57). Lowering: value − 1 is stored (0x47ba5d), then
  **0x58 plays at the listener G+0x140, volume 1, pitch 1** (0x47ba7e), then a negative
  value is reset to 0 (0x47ba89 … 0x47ba93).

Consequence: lowering a skill already at 0 (with its dependent at 0) still plays 0x58 and
leaves the value at 0.

Correct statement: 0x58 at 0x47ba7e is the skill-lowered sound. It is not an item being put
down. `alpha-ui-screens.md` §10's row "0x58 … an item is put down (0x47ba7e, 0x47d2aa,
0x47da26)" must drop 0x47ba7e from the put-down list. 0x47da26 was re-read and is an
equipment box emptied back into the bag (item added to the bag, box cleared, 0x58 at the
listener). 0x47d2aa was not re-read for this ruling.

## Ruling 3: sounds 0x39 / 0x3a / 0x3b — `alpha-audio.md` wins on the direct sites, with one correction

Every use of these ids as a sound in either image:

| Id | Site | Kind | Trigger | Position | Volume | Pitch |
|---|---|---|---|---|---:|---|
| 0x39 `drop.wav` | 0x47dc2c (client) | direct call | **buy back** (vendor buy-back tab, vendor widget +0x1b4 = 1) | player (+0x10 of G+0x8006d0) | 1 | 1 |
| 0x39 `drop.wav` | 0x47dd6b (client) | direct call | **buy** (other tabs): price ≤ money and "can carry"; item added, money (+0x1304) −= price (0x47dd45) | player | 1 | 1 |
| 0x3a `drop-coin.wav` | 0x47ce74 (client) | direct call | **sell from a bag slot** (money += price, 0x47ce4e) | player | **0.75** | 1 |
| 0x3a `drop-coin.wav` | 0x47d89a (client) | direct call | **sell from an equipment box** (money += price, 0x47d874) | player | **0.75** | 1 |
| 0x3a / 0x3b | client 0x60d289, server 0x533049 (shared world step) | sound record | a world item's countdown at entry +0x13c goes from positive to ≤ 0 this step (then it is set to 0), and the byte +0xb4 of the step's context object is 0 | the item's position (entry +0x118, converted to float blocks) | 1 | 1 + 0.1·*r*/32767 (*r* one CRT `rand()`) |

Record id rule: **0x3a if the item's type byte (entry +0) is 12 (coin), else 0x3b**
(client 0x60d268 … 0x60d28c; server 0x533028 … 0x53304c). The pitch is formed in single
precision as float(*r*) × 0.1, divided by 32767, plus 1 (client constants 0x6fcb7c,
0x6fd338, 0x745dc0; server 0x5586bc, 0x558834, 0x5586dc); its `rand()` is the only draw.

Buy-back detail (0x47da47 … 0x47dc38): the buy-back entry is the one at the hovered row in
the list G+0x800d3c. For each unit of its count: if the unit cannot be carried, the chat
line "You can't carry more of these items." and sound 0x31 at the player follow and the
loop stops; a unit whose price exceeds money is skipped silently; otherwise it is added
and paid for. The bought units are taken from the entry (an emptied entry is unlinked),
then **0x39 plays unconditionally** — also after a refusal and when nothing was bought —
and the vendor lists are rebuilt (0x4a2300). On the plain buy path, price > money plays
nothing and "cannot carry" plays only 0x31 (0x47dd1a); 0x39 plays only on success.

Not used: no direct call or helper call (0x484320) passes 0x3b; no sound record carries
0x39 in either image (all record producers in the client world step 0x60c510 and every
immediate 0x39 / 0x3a / 0x3b in both images were checked; the remaining occurrences are
pose, table or list values, not sound ids). There is no "dropping" call site: the player's
act of dropping an item plays none of these ids directly.

Which statements are right:

* `alpha-audio.md` §4 rows 57 and 58 (buy / buy back; sell at 0.75) — **right**, but the
  0x47dc2c row should note that buy back plays 0x39 even when nothing is bought.
* `alpha-audio.md` §4 note "59 is only the world-item landing sound" — **right as to the
  call site** (0x3b exists only as the world-step record). Its row "57, 58, 59 … records"
  is **wrong for 57**: no record carries 0x39. 58 is a record id too, for coin items, from
  the same moment as 59.
* `alpha-ui-screens.md` §10 row "0x39 / 0x3a / 0x3b … buying, selling, dropping" — buying
  and selling are right; "dropping" is replaced by the world-item record rule above.
  (Its own §8.1 entries — 0x39 buy, 0x3a sell at 0.75 — already agree.)

## Evidence

Static reading of the original listings for both images (instruction by instruction for
0x623610, the tail of 0x484350, the skill block of 0x47b600, the buy / buy-back / sell
blocks, and the world-item countdown in client 0x60c510 and server 0x5322d0); constants,
the engine vtable 0x71e23c and the `rand` import slot 0x6fc468 read from the image bytes.
Reconstruction sources were not consulted.

## Residual uncertainty

* Not verified at runtime.
* What arms the world-item countdown at entry +0x13c (and so whether its expiry is the
  moment an item lands, as `alpha-audio.md` calls it, or another delay after a drop) was
  not traced here; no immediate non-zero store to it was found, so it arrives by copy.
  The meaning of the gating byte +0xb4 of the step's context object is likewise open.
* Which physical mouse button produces the non-zero press argument of 0x47b600 is taken
  from `alpha-ui-screens.md` §8.1's right-press naming of the same branch, not re-derived.
* The absence of panning rests on the absence of output-matrix / channel-volume calls in
  the audio code region and of any 3D-audio import; voice pointers were not traced through
  every other routine of the image.
* The coin type value 12 is the type byte compared at 0x60d268; its naming as "coin" relies
  on the item-type tables of the other contracts.
