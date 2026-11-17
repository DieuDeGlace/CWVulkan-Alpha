Status: FINAL / STABLE / HELD

# Alpha ruling: `manacubebar` icon count and the `manacubes` sequence

Scope: the conflict between `alpha-ui-screens.md` §5.3 (`manacubebar` row, "the file has no
sequence there, so effectively mod 4") and `alpha-ui-plasma.md` (re-certified 2026-11-16:
`gui.plx` has a `manacubes` sequence on `manacubebar`'s own shape, played at 0x48f9e1).
Image: client Cube.exe (SHA-256 84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF),
32-bit, image base 0x400000. Addresses below are virtual addresses in that image (RVA =
address − 0x400000). G = the game object (`this` of the frame routine 0x488ee0); P = the
local player creature, G+0x8006d0. Mana-cube count = P+0x1164 (signed 32-bit).

## Ruling

The icon count is **count mod 4 (C-style signed remainder), except 4 while the `manacubes`
sequence is running on `manacubebar`**. The game starts that sequence once, with **period 0
(play once, no loop)**, exactly when a mana-cube pickup raises the count to a multiple of 4.
So on the 4th, 8th, 12th … cube the bar keeps showing **all four icons for about 1.3 s**
(while the vertex-colour animation plays). After that it drops to count mod 4 (normally 0).
At all other times the rule is the plain mod-4 rule.

**`alpha-ui-plasma.md` wins.** In `alpha-ui-screens.md` §5.3, the clause "the file has no
sequence there, so effectively mod 4" is wrong. The rest of that row is right: the first *k*
children visible, *k* = count mod 4, or 4 while a sequence plays in the node (0x6364f0).

## 1. When the game plays `manacubes` (0x48f8a0 … 0x48fa7f)

The frame routine walks a list of pickup records (loop head 0x48f8a0, back-edge 0x48fac6).
Each record holds a creature id followed by an item record. The item type byte sits at
record +8. For each record, in this order:

1. The creature id is looked up in the world creature table (G+0x2e4, 0x42f000). If there
   is no creature with that id, the record is skipped (0x48f8c0). If the creature is P,
   0x4a14c0 is called (0x48f8c6 … 0x48f8d0).
2. If the type byte is 0x0D and the record's id equals P's id, P+0x194 gains 10 × the
   record's signed 16-bit field +0x18 (0x48f8d5 … 0x48f913; not part of this ruling).
3. If the type byte is not 0x19 (25), the generic pickup handler 0x480fb0 runs and the
   cube path below is skipped (0x48f91c / 0x48f91f → 0x48fa81).
4. **Type 0x19 (mana cube)**, in order:
   1. P+0x1164 += 1 (0x48f930). This is the local player's count. In this branch the
      record's creature is **not** compared with P. The only requirement is that the id
      resolves (step 1).
   2. P+0x68 = 0x6D, P+0x6C = 0 (0x48f943, 0x48f94d).
   3. Chat line "You receive a mana cube." (string 0x701878) in colour (100, 150, 255)
      (0x48f954 … 0x48f988).
   4. 0x484350 is called with sound id 0x1E, the position G+0x140 and two 1.0 factors
      (0x48f9a1 … 0x48f9be).
   5. The new count is read. If its signed remainder mod 4 is non-zero, the record ends
      here (0x48f9c3 … 0x48f9db).
   6. Otherwise: `play(G+0x8007d8 = manacubebar, "manacubes", period 0)`. The string is at
      0x7018ac, and the call 0x636810 is set up at 0x48f9e1 … 0x48fa0a, with 0 pushed as the
      period at 0x48f9f8. Then the chat line "You gain one skill point." (0x7018c0) is added in
      colour (150, 128, 255) (0x48fa23 … 0x48fa66).

This is the only call that plays a sequence on `manacubebar`. The other play-call sites are
`smallquesttag`, `questtag`, `combopoints`, the widget `widget:enable`/`widget:disable` calls
on another object's node (0x628f40, 0x62a8b0), and the Button state routines (0x6653a0 …
0x665b30). The other write to P+0x1164 is a bulk copy at 0x44cab8 (0x44be40). It does not
play anything. So a count that becomes a multiple of 4 through that copy shows plain
mod 4, with no four-icon window.

## 2. What "a sequence plays in it" returns over time

**Test (0x6364f0).** The test is true if any of these holds:

* the playing byte +0x2c is set on the node's shape (+0x34), transformation (+0x38) or
  display (+0x3c);
* the test is recursively true for any child whose node word +0xC8 has bit 2 clear.

**Setting the byte.** At start (0x664c10), the playing byte of an object is set only if at
least one of its attributes has a sequence with that name (0x661d90 → 0x661df0, byte set at
0x664c4e). Here that object is the shape of `manacubebar` (vertex colours). The test is
true from the play call onwards, in the same frame.

**Advance (once per interface update, 0x664cf0).** Each object is guarded by the per-update
byte +0x2e, which is cleared at the start of the walk (0x636be0) and set by 0x664cf0, so an
object advances at most once per update. If its playing byte is set, each attribute is
advanced (0x6626d0):

* An attribute with an empty working key list reports "not running".
* Otherwise the attribute is **evaluated at its elapsed time before this step's increment**.
  Its elapsed time then increases by the step (0x662701 … 0x66270e). If the attribute
  reports "not running", its working key list is cleared (0x66271b).

If no attribute reported "running", the object's playing byte is cleared (0x664d45).

**Evaluation result (0x662300), for this sequence.** The sequence has three keys at 0, 600 and
1300 ms (`alpha-ui-plasma.md` §3.8), so no implicit start key is inserted, and the period is
0. For an evaluation time *t*:

* 0 ≤ *t* < 1300: segment interpolation. The value is written, the applied-frame cache
  (attribute +0x3c) is reset to −1, and the result is "running" (0x662425 … 0x66267e).
* *t* ≥ 1300 the first time: the cache differs from the last key's frame, so the last
  frame is applied and cached. Result: "running" (0x6623b2 … 0x66241c).
* *t* ≥ 1300 the second time: the cache equals the last key's frame. Result: "not running"
  (0x6623dc → 0x662529). The key list is cleared and the playing byte drops.

A key that refers to a missing frame also yields "not running" (0x6624be … 0x6624dc). That
would end the window early. The shipped file is not expected to hit this (see Unresolved).

**Timeline.** Frame 0 is the pickup frame. Let *d*ᵢ be the interface step passed in frame *i*,
and let Eᵢ = *d*₀ + … + *d*ᵢ₋₁ (so E₀ = 0) be the time evaluated by frame *i*'s advance.

* Let *m* be the first frame with Eₘ ≥ 1300. The playing byte is cleared during the advance
  of frame *m* + 1.
* Within the frame routine, the pickup loop (0x48f8a0) and the HUD block (0x490d70) both
  run before the interface advance (0x49d2f9 → 0x659fb0 → root walk 0x6372a0).
* So the HUD reads "playing = true" in frames 0 … *m* + 1. It reads false from frame *m* + 2
  on, unless the sequence is restarted.

With a constant step *d* ms, four icons are shown for ⌈1300 / *d*⌉ + 2 frames. Example:
*d* = 16 gives 84 frames, and the drop is first visible in the frame that starts 1344 ms
after the pickup frame. So the window lasts 1300 ms plus up to about two frames.

**Step value.** The step is the frame routine's first argument (stored at 0x488f34, passed
at 0x49d2ef). 0x659fb0 adds it to the document clock +0xe8 and stores it in +0xe4 unchanged.

## 3. The resulting icon rule (0x490d70 … 0x490e37, every frame, if G+0x8007d8 ≠ 0)

1. *k* = P+0x1164 signed remainder 4 (0x490d76 … 0x490d88).
2. If the test of §2 is true for `manacubebar`, *k* = 4 (0x490d8f … 0x490d9b).
3. Its children are visited in file order (four `icon1` groups). Each child is made visible
   if *k* > 0, otherwise hidden, and *k* decreases by 1 after each child (0x490de0 …
   0x490e0d).

Consequences:

* Normal play: icons = count mod 4 (0 to 3).
* On reaching a multiple of 4 by pickup: four icons, and `manacubes` animates the bar
  shape's vertex colours to its 1300 ms key, which stays as the held value. Then 0 icons.
* Pickups during the window (count mod 4 becoming 1, 2 or 3) do not change the display
  until the window ends; then it shows count mod 4. Four further pickups would restart the
  sequence at elapsed 0.
* The window lengthens if the bar's shape is not advanced in some frames:
  * The interface advance is skipped on frames that leave the routine through 0x49d322
    (the coordinate range tests at 0x495746 … 0x49576e).
  * The update walk does not enter a node with node-word bit 2 set (0x6372cd, 0x6374f2).
  * The walk does not enter a node whose display visibility value is 0, unless node flag
    0x2000 is set on it or passed down from an ancestor (0x6372e9, 0x63751a … 0x637538).
  * The HUD block passes 1 to `manacubebar`'s display each frame (0x490d5a … 0x490d6e), so
    only an invisible ancestor or a skipped walk can freeze it.

## Which contract statement is right

* **Right:** `alpha-ui-plasma.md`, Re-certification (2026-11-16), "Cross-specification
  disagreements", first bullet, and §6.5 "Calls by the game" (`manacubes` on G+0x8007d8,
  period 0). Its open remark "what triggers that path was not traced" is now answered
  (§1: mana-cube pickup record, type 0x19).
* **Right:** `alpha-ui-screens.md` §5.3 `manacubebar` row, the rule itself: first *k*
  children, *k* = count mod 4, or 4 while a sequence plays in it.
* **Wrong:** the same row's clause "the file has no sequence there, so effectively mod 4".
  It should read: "`manacubes` plays once (period 0) when a pickup makes the count a multiple
  of 4; for its duration (last key 1300 ms, cleared on the second update evaluated at
  ≥ 1300 ms) four icons show, otherwise count mod 4."

## Evidence

* Static reading of the original client image's assembly listing, instruction by
  instruction, over the ranges cited above: 0x48f8a0 … 0x48fac6, 0x490cf5 … 0x490e0d,
  0x6364f0 … 0x63655b, 0x664c10 … 0x664d4e, 0x6626d0 … 0x662725, 0x662300 … 0x66267e,
  0x6372a0 … 0x6374c4, the start of 0x6374d0, and 0x488ee0's prologue and its call to
  0x659fb0.
* The string addresses and texts (0x701878, 0x7018ac, 0x7018c0, 0x71e7dc, 0x71e7f8) come
  from the image's string table.
* The key times, the target attribute and the node of `manacubes` come from the re-certified
  byte-level `.plx` parse in `alpha-ui-plasma.md` §3.8. They were not re-parsed for this
  ruling.
* Reconstruction sources were not consulted.

## Unresolved

* Not verified at runtime.
* The unit and producer of the frame step (the first argument of 0x488ee0, reached through
  an indirect call) were not traced. The timeline assumes milliseconds, the unit of the key
  times. The frame counts in §2 hold for any step sequence once expressed in that unit.
* The network origin of the pickup record list iterated at 0x48f8a0, and why the type-0x19
  branch credits P without comparing the record's creature with P, were not traced. The
  server may send only the local player's pickups.
* The `gui.plx` key frames were not re-parsed here. A missing frame index in a key would end
  the window on the first update (§2). The plasma parse reports valid keys.
* It was not traced whether the HUD block 0x490cf5 … can itself be skipped while the HUD is
  hidden (menus, death). A skipped HUD block changes only when the count is shown, not the
  rule.
* The meaning of P+0x68 = 0x6D / P+0x6C = 0, the sound id 0x1E and the skill-point
  accounting are outside this ruling.
