Status: FINAL / STABLE / HELD. Full author readback completed.

# Alpha client controller: local-player aim writers, inputs and frame order

## Scope and evidence

This contract closes, as far as the original image supports, the items that the held aim contract (`alpha-combat-aim-triplet.md`, sections 2.4 and 9) left open about the local player's aim in the client controller update at RVA `0x088EE0`. It uses that contract and the two held camera contracts (`alpha-camera-initial-state.md`, `alpha-camera-follow-collision.md`) only to locate routines and for their terms. Every statement below was checked against the original image.

Supported image: client `binary/Cube.exe`, 32-bit, preferred base `0x00400000`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf` (recomputed before use). The server image `binary/Server.exe` (SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`, recomputed) has no counterpart of these routines and was not otherwise used. All addresses are RVAs unless marked VA. Evidence comes from the original instruction listings `decompiled/client/assembly.txt` and `decompiled/client/assembly-functions/00488ee0.asm`, from image bytes (constants, jump tables, a virtual table, the import list), and, for the receive routine that the listing does not decode, from a plain disassembly of the image bytes at `0x06B8B0–0x06D130`. No reconstruction source, runtime observation or implementation material supplied a fact. The reconstruction lead about attack facing (held aim contract, end of 2.4) was tested against the original instructions only.

Terms follow the held contracts. C is the client object (receiver of `0x088EE0`); W is its world at C+`0x2E4`; P0 is the local player creature at C+`0x8006D0`. Aim, yaw, head pitch, drawn aim, flags word (`+0x124`), face-aim bit (bit `0x0004` of the flags word), action (`+0x68`), clock (`+0x6C`), fixed conversion, float conversion and precise normalization are as defined in the held aim contract. F0 (follow anchor, C+`0x158`), E (camera eye, C+`0x140`) and the current camera angles (C+`0x1A4`) are as in the camera contracts. N is the update argument (the frame's elapsed milliseconds). Fixed values are signed 64-bit with 65,536 units per world unit. All binary32 arithmetic rounds after each operation.

Stack-frame locations named below (for example "the aim-point slot") are the update's own locals; they are identified by their offset from the update's steady stack pointer, with each listed instruction's pending pushes accounted for.

## 1. Frame order (item 5)

### 1.1 Outer frame

The outer routine `0x0C8AE0` runs one iteration per frame:

1. Window messages are processed (camera contract).
2. **Input sampling** (`0x0C91BF–0x0C942A`), admitted only when the byte at VA `0x00766218` is nonzero and an imported call's result equals the window handle at VA `0x0076B1C0` (read as "the game window has focus"; the import identity behind that call was not separately confirmed). It reads a 256-byte keyboard state and a 16-byte mouse state through device-object methods (table slot `+0x24`; the image imports `DINPUT8.dll!DirectInput8Create`), writes the controller input bytes of section 2, and calls the client's mouse callback (table slot `+0x18`, `0x07EA00`) with the mouse displacement. When not admitted, the input bytes keep their previous values.
3. The frame routine `0x0C85F0` is called (`0x0C9432`), which runs the update `0x088EE0` and then the renderer `0x0AC260` (camera contract).

### 1.2 Inside the update `0x088EE0`

Within one update the relevant work runs in this order. No backward jump in the listing returns from a later step to an earlier one, and no forward jump bypasses the step that produces the aim point (checked over the whole update).

1. The world lock is acquired (`0x08902F`, routine `0x201CB0` on W); it is released only near the end (`0x09D2EA`, `0x201E90`). Another acquire/release pair lies inside this span at `0x08D55B`/`0x08D5EB`; the lock's re-entrancy was not audited.
2. **World update**: `0x20C510` is called at `0x08CF2E` (in a loop of 20 ms slices while more than 20 ms remain) and once more at `0x08CF49` with the remainder. This world update reads P0's aim and face-aim bit as they were left by the **previous** update's controller writes, and advances P0's drawn aim from that aim.
3. **Camera forward vector V** (`0x090A36–0x090A74`): V is the direction transform (`0x088E50`, no translation) of (0, 0, 1) by the client matrix at C+`0x1EC`, then precisely normalized (`0x0240F0`). The matrix at C+`0x1EC` is rebuilt only at `0x09CBE3–0x09CC54` (identity, a translation, then rotations by current angle components 2, 1 and 0, in the same order as the late follow route's temporary matrix; no shake is applied to it; the shaken view matrix is C+`0x26C`). V is therefore the unshaken camera forward direction of the previous update's late camera phase.
4. Camera smoothing at `0x0965A2` (camera contract); it changes the current angles but not the matrix at C+`0x1EC`, F0 or E.
5. **Controller flags block** (`0x098259–0x098404`), section 4.
6. **Aim point** (`0x098409–0x0984F6`), section 3.1, and the **creature pick** (`0x09858F–0x098E1D`), section 3.2, then the close-spell freeze (`0x098E7C–0x098ECC`).
7. **Aim write** (free control `0x09B4C6–0x09B589`, or build mode `0x09B18D–0x09B4C1`), section 3.4.
8. Action and attack input handling (`0x09B58E–0x09C02F`).
9. Late camera follow/collision (`0x09C2E6` onward) and the rebuild of C+`0x1EC`, C+`0x22C` and C+`0x26C` (`0x09CBE3` onward).
10. Lock release (`0x09D2EA`); the frame routine then renders.

Consequences:

- The aim written in update k is first used by the world update of update k+1 (beams, shots, path, facing and drawn-aim smoothing of the held aim contract). The client's own beam drawing in the renderer of update k reads the drawn aim, which was last advanced by the world update of update k.
- The aim ray of update k uses F0, E and V as left by the late camera phase of update k−1, not the angles smoothed earlier in update k.
- Network threads (section 6) take the same world lock, so a received update is never applied in the middle of steps 2–9.

## 2. Controller input bytes (item 2)

The input sampler writes each byte as 1 when the source byte is nonzero, otherwise 0. Keyboard sources are indices into the 256-byte keyboard state; the key names in parentheses are the conventional names of those indices in the DirectInput scan-code numbering, given as an interpretation. Indices are fixed in the image; no binding table is consulted at this site.

| Client byte | Source | Conventional key |
| --- | --- | --- |
| +0x04 | mouse button 0 | left button |
| +0x05 | mouse button 1 | right button |
| +0x0A | mouse button 2 | middle button |
| +0x06, +0x07, +0x08, +0x09 | keyboard 0x02, 0x03, 0x04, 0x05 | 1, 2, 3, 4 |
| +0x0B | keyboard 0x11 | W |
| +0x0C | keyboard 0x1F | S |
| +0x0D | keyboard 0x1E | A |
| +0x0E | keyboard 0x20 | D |
| +0x0F | keyboard 0x13 | R |
| +0x10 | keyboard 0x14 | T |
| +0x11 | keyboard 0x12 | E |
| +0x12 | keyboard 0x39 | Space |
| +0x13 | keyboard 0x1D | Left Ctrl |
| +0x14 | keyboard 0x2A | Left Shift |
| +0x15 | keyboard 0x2A | Left Shift (same source as +0x14) |
| +0x17 | keyboard 0x10 | Q |

Bytes +0x16 and +0x18 are not written by this sampler; their producers were not traced. All of +0x04 to +0x18 are zeroed by the base-part initializer (`0x03B4C0`, stores at `0x03B522–0x03B52F`). A separate per-key array at C+`0x19` (256 bytes) and per-button array at C+`0x119` (3 bytes) are set and cleared by small base routines (`0x03B580`, `0x03B5A0`, `0x03B5C0`, `0x03B5D0`); they are distinct from the bytes above.

Uses relevant to this contract:

- **+0x14 (Left Shift)** selects the yaw-forward aim in free control (3.4), suppresses the face-aim set and the bit-`0x40` rule of section 4, and makes the controller store P0's own id into P0's target-id field (`+0x1A0/+0x1A4`, at `0x098E5E–0x098E76`).
- **+0x13 (Left Ctrl)** is copied into flags bit `0x0001` every time the flags block runs (`0x0983F4–0x098404`). Bit `0x0001` is cleared at `0x09C01D` when a local of the action-input region is set (not audited). In the pose routine, bit `0x0001` participates in the head-pitch condition (section 7).
- **+0x12 (Space)** is the value of flags bit `0x0040` in one route of section 4.
- **+0x0B, +0x0C, +0x0D, +0x0E and the two following inputs** drive the build-mode offsets (held aim contract 2.4). Copies of +0x0F, +0x10 and +0x11 are kept at VA `0x0076B138–0x0076B13A` (`0x09B593–0x09B5A6`) for edge detection. The remaining inputs' gameplay roles were not traced.

## 3. The local player's aim (item 1)

### 3.1 Aim point from the camera ray

The aim point A* is a fixed-point triple in a local of the update (the aim-point slot). It is produced every update on every path that reaches the aim write (`0x098409–0x0984F6`):

1. D100 = V × `100.0` (binary32, component-wise; `0x42C80000`), then fixed conversion.
2. A* = F0 + D100 (64-bit adds).
3. A world ray (`0x1A35D0` on W) is cast from **F0** in direction V with maximum length `60.0` (`0x42700000`) and the two option arguments 0 and then 1, in the same argument order as the held aim contract's ray calls. The returned value is narrowed to binary32 and kept as L (it is also used in 3.3).
4. If L is not less than `0.0` (the comparison is against binary32 zero at VA `0x00745D84`; an unordered result counts as "less"), A* is replaced by F0 + fixed conversion of (V × L).

So the aim point is where the camera's centre line, started at the follow anchor rather than at the eye, meets terrain within 60 units, or the point 100 units along that line when the ray reports a negative result. What the ray treats as a hit, and what it returns when nothing is hit, belongs to `0x1A35D0` and was not audited; this contract preserves only the caller's comparison. The mouse cursor position is not an input: the line is fixed to the screen centre through V.

Immediately after this, the update clears the 32-bit pointers C+`0x8006D8` and C+`0x8006D4` and the 64-bit id at C+`0x800A70`, and resets two cell selections (C+`0x800A84`, C+`0x800A78`) to (−1, −1, 0) (`0x0984FB–0x09855A`).

### 3.2 Creature pick on the screen centre

The update then scans the world's creature map (`0x098640–0x098D03`). Several locals start at binary32 −1.0 (`0x00745E8C`), among them the best admitted squared distance Dc and the best hovered squared distance; two 64-bit ids start at zero (`0x09858F–0x0985D5`). For each creature K other than P0:

1. While P0's action is `0x22` and P0's clock is below R0, creatures whose byte `+0x60` is 6 or 1 are skipped.
2. A world visibility routine (`0x19EE90` on W) is called with E, K's point (K `+0x1350` plus the fixed conversion of (0, 0, −K`+0x1180`)), 0, and a binary32 value derived from a world column query; K is skipped if it returns zero. (The receive path stores a creature's received position at `+0x1350`; see section 6.) The routine and its scale value were not audited.
3. Eight corners of a box around K's point are visited (three nested two-way counters). Along each axis the box spans K's size component (`+0x80`, `+0x84`, `+0x88`) plus `1.0`, centred on the point; the corner construction also takes the camera rebasing members (C+`0x1D8`, C+`0x1E0`) through `0x03B610`, whose exact role was not closed. Each corner is transformed by the shaken view matrix C+`0x26C` (`0x0248A0`). Only corners whose transformed component 2 is strictly greater than 0.0 are then transformed by the matrix at C+`0x800A90` (`0x088D60`; the renderer stores its projection matrix there, `0x0AED96`). Four flags record whether any such corner had transformed component 0 below 0, component 1 below 0, component 0 above 0, and component 1 above 0.
4. Independently of step 3, if the squared distance from P0's position to K's point is below `16.0` and below the best recorded so far (or none is recorded), K's id is kept as the nearest close creature (`0x098B37–0x098BB0`).
5. If all four flags of step 3 are set (the box straddles the screen centre on both axes), K is marked (record byte `+0x08` = 1) and its candidate value d² is the squared distance from E to K's point (fixed self dot product `0x03B4A0`, each square divided by 65,536, then float conversion `0x0120F0`). If d² is smaller than the best hovered value (or none), it becomes the best and C+`0x800A70` receives K's id. If additionally the admission routine `0x1988A0` (W, P0, K) returns nonzero, and d² is smaller than Dc (or Dc is still negative), Dc = d². If the player-distance value of step 4 is smaller than a separate best (or none), K's id is also kept in a third pair of locals.

After the scan, if C+`0x800940` is non-null, C+`0x800A70` is replaced by the id of the creature that object references (`0x098D21–0x098D42`).

**Override** (`0x098D48–0x098E1D`). If Dc is not negative and P0's action is not `0x31`, a point Q = E + fixed conversion of (V × r) is formed, where r is the binary32 square root of Dc (precise import, widened and narrowed). If the three-component dot product of V with the float conversion of (Q − P0 position) is strictly greater than 0.0, A* = Q. The aim then points at the depth of the nearest admitted creature under the screen centre, measured from the eye, provided that point lies in front of the player along V. This override does not compare Dc with the terrain length L.

**Target id** (`0x098E22–0x098E76`). While P0's action is `0x22` with clock below B, and the third id pair is nonzero, that id is stored at P0 `+0x1A0/+0x1A4`. If input +0x14 is set, P0's own id is stored there instead (written after, so it wins).

**Close-spell freeze** (`0x098E7C–0x098ECC`). For actions `0x1E`–`0x21`: if (clock − N) is not greater than B, the current A* is stored at P0 `+0x1320`; otherwise A* is replaced by P0 `+0x1320`. The spell's aim point is therefore held from the update in which clock − N first exceeds B. (B is subject query `0x03CAA0` with argument −1.)

### 3.3 A second override that is not reached in ordinary flow

At `0x099DB3–0x099E96`, if L is not negative and the pointer C+`0x8006D8` is non-null, and L² is strictly greater than the squared length of the float conversion of (that creature's position `+0x10` − F0), A* is replaced by F0 + fixed conversion of (V × the binary32 length of that difference). The only located stores to C+`0x8006D8` are the constructor's zero, a store while processing hit records (`0x094A73`, earlier in the update), the zero at `0x0984FD` and stores in routine `0x0A6B50` (called at `0x0982B9`, before the zero). No store lies between the zero at `0x0984FD` and the test at `0x099DC9`, and no backward jump re-enters that span. The test therefore sees a null pointer on every path found, and this override is dormant unless an indirect store not covered by this search sets the pointer.

### 3.4 The aim write

**Free control** (`0x09B4C6–0x09B589`) runs when the build-mode byte C+`0x800704` is zero and global byte VA `0x0076B040` is zero:

- If input +0x14 is set, the aim becomes the unit forward vector of P0's yaw, as stated in the held aim contract (2.4).
- Otherwise the aim becomes the float conversion of (A* − P0 position): a 64-bit subtraction per component (`0x02C7A0`), then each component rounded to binary32 and multiplied by 1/65536 (`0x02C4A0`). The aim is therefore an un-normalized offset whose length equals the distance from the player to the aim point, up to rounding.

**Build mode** (`0x09B18D–0x09B4C1`) is as in the held aim contract (2.4); A* is not used there.

When VA `0x0076B040` is nonzero, free control does not write the aim, and the aim is written only by the path-following branch (section 5).

### 3.5 Precision and draws

All conversions are those of the held contracts. The sequence uses V (binary32, normalized), the binary32 products with 100.0, L or r, and their fixed conversions, added to the 64-bit F0 or E. No direct random-number call occurs anywhere in `0x090A36–0x09B589` (the update's only listed `rand` call sites are at `0x09534E–0x0954FB`). Draws inside the nested routines `0x1A35D0`, `0x19EE90`, `0x1988A0` and the column query were not audited.

## 4. Face-aim bit and neighbouring flags (item 4)

The controller writes P0's flags word through the bit setter `0x02F160` (on P0 `+0x10`; the setter ORs or clears its mask in the word at `+0x124`). The block `0x098259–0x098404` runs on the same path as free control (section 5 lists its gates). In order:

1. The face-aim bit is **cleared** (`0x0982BE`).
2. Bit `0x0040` is set or cleared (`0x0982D0–0x098313`). If flags bit `0x0010` is set and bit 0 of P0's byte `+0x5C` is clear, the value is input +0x12 when P0's binary32 `+0x1194` is strictly greater than 0.0, and 0 otherwise. In every other case the value is 1 when input +0x14 is clear and 0 when it is set.
3. If input +0x14 is clear, and the action is none of `0x6D`, `0x53`, `0x6A`, `0x6B`, `0x69`, `0x52`, `0x22`, `0x54`, `0x50`, `0x51`, `0x4F`, `0x0B`, and either the clock is below R0 + 200 (R0 is subject query `0x03D1A0`) or the action is one of `0x18`, `0x19`, `0x1B`, `0x08`, `0x24`, `0x40`, `0x3B`, `0x3F`, `0x1C`, `0x23`, then:
   - the face-aim bit is **set**, unless the action is `0x58`, `0x57`, `0x59` or `0x5C`;
   - bit `0x0040` is cleared (also for those four actions).
4. Bit `0x0001` is set to input +0x13 (`0x0983F4–0x098404`).

For action 0, R0 evaluates to 100 (both duration tables give 0 for action 0 and a final term adds 100; `0x03D1A0`, tables at VA `0x0043DA54`, `0x0043DB40`, `0x0043DBEC`). The face-aim bit is therefore set while idle only during the first 300 ms of the idle clock. During any non-excluded action it is set until 200 ms past R0, and throughout for the listed sustained actions. Holding Left Shift keeps it clear.

Verdict on the reconstruction lead: the controller does set the face-aim bit around attacks, but not in its attack-input handling. It is a per-update recomputation from P0's current action and clock, made **before** the action-input region (`0x09B58E` onward) of the same update. The update in which an attack starts therefore leaves the bit as computed for the previous action, and the next update's world step runs with that value. No other store to the face-aim bit exists in `0x088EE0` (the other setter calls are the path branch's clear and set, the bit-`0x40` and bit-`0x0001` writes, and `0x09C01D`, which clears bit `0x0001`). No direct store to P0's yaw (`+0x30`) or head pitch (`+0x58`) exists in the update; stores through component-address helpers were not excluded.

Byte `+0x7E` bit `0x04`, which the sight-point routine `0x046BB0` tests on the creature passed to it, is a different field from the flags word's bit `0x0004`. The update contains no store to `+0x7E` (its only references test it, at `0x08C2C7` and `0x09A925`). Its producer is not established here.

## 5. Global byte VA `0x0076B040` and the path branch (item 3)

The byte lies at RVA `0x36B040`, beyond the raw (file-backed) part of the `.data` section, whose raw data ends at RVA `0x36B000`. It is therefore zero when the image is loaded. A byte scan of the whole image finds exactly two encodings of its address, both reads: `0x098078` (selects the path branch) and `0x09B4C6` (selects free control). The neighbouring static initializers at `0x2F9A80–0x2F9AD0` store only up to VA `0x0076B03B`. No direct writer exists; an indirect store through a computed pointer was not excluded. In ordinary flow the byte stays zero, so free control and the flags block always run, and the path branch is dormant. Its meaning (for example click-to-move) cannot be proven from its uses.

When the branch would run (`0x098078–0x098254`), its behaviour is as stated in the held aim contract (2.4). The goal it compares against is P0's three-component cell value at `+0x1440`, converted by `0x050F60` and compared with the path list's last element (`0x098106–0x098120`). The branch clears and may set the face-aim bit itself, and then jumps past the flags block of section 4, but not past the aim point of section 3.1. Client writers of `+0x1440` exist at `0x02ECE5` and `0x0DFC25`; they were not traced.

Gates on the path to sections 4 and 5 (in order). If C+`0x800704` (build mode) is set, the update jumps to the aim point directly or after a separate routine (`0x097983–0x0979C7`). If byte C+`0x800A40` is set, a separate block runs and jumps to the aim point. If the pointer C+`0x800940` differs from a register value at `0x097F27` (whose value there was not traced; believed null), the referenced widget property is set, and the test `0x077240` on its `+0x178` fails, an interaction block runs and jumps to the aim point. If C+`0x800A14` is non-null with byte `+0x180` set, or P0's signed `+0x12C` is positive, the update jumps to the aim point. Only otherwise are section 4 (byte zero) or section 5 (byte nonzero) reached. In every gated case the face-aim bit, bit `0x0040` and bit `0x0001` keep their previous values. Before section 4, routine `0x0A6B50` runs only when none of six front-end widget properties (C+`0x800874`, `+0x800880`, `+0x800890`, `+0x800888`, `+0x800894`, `+0x80088C`) is set (`0x098259–0x0982B9`).

## 6. Received entity updates for the local player (item 6)

Both receive sites lie in code that the listing does not decode (`0x06B8BD–0x06D0DD`). They run in a routine that loops over blocking socket reads, and take the world lock around every access to the world. How this routine is started (presumably as a separate thread) was not traced. The send routine `0x069C10`, which contains the field writer call at `0x069F81`, likewise takes the world lock; its schedule was not audited.

**Single-entity message (`0x06BC82`).** After reading a 64-bit id, the routine locks the world and looks up or creates the creature. It copies the creature's current entity record (from `+0x10`) into a local record (`0x066050`) and unlocks. The field reader `0x0CCFA0` then fills only the fields whose mask bits are present into that local copy. The routine locks again. If the creature is P0 (`0x06BCC5`), it skips straight to the unlock (`0x06BE04`): the local copy is discarded and **nothing** of the received record is applied to the local player, neither position, yaw (`+0x28` triplet), head pitch, aim nor any other field. For any other creature it updates the interpolation members (`+0x1338`, `+0x1350`, `+0x1368–+0x1388`, `+0x1398`) and assigns the whole local record back to the creature (`0x044B040` at `0x06BDFF`).

**Multi-entity message (`0x06C41C`).** Under the world lock, the routine first builds a map keyed by id holding a copy of every existing creature's record (`0x06C2F8–0x06C34F`). It reads a count, then for each entry an 8-byte id and the stream receiver `0x0CD3E0` into that id's map record, collecting the ids into a set. P0's id is then added to the set (`0x06C469–0x06C48C`), and creatures absent from the set are deleted, so P0 is never deleted by this message. For each id in the set, the creature is looked up or created. If it is P0 (`0x06C69A`), the routine skips to the next entry (`0x06C977`), past the record assignment at `0x06C96C`. The local player's record is therefore never overwritten here either.

Hence the local player's aim, yaw and head pitch on its own client are written only by the local controller and the local world update (and the build-mode and path branches), never by received entity updates on these two paths. Other message kinds that may touch individual P0 fields (hits, health and similar) were not examined.

## 7. Head pitch and drawn aim in the pose routine (item 7)

Within the animation routine `0x0128F0` (client only), head pitch `+0x58` is read at these sites. The part identities and the full matrix chain were not closed.

- `0x020AC6–0x020B1D`. If flags bit `0x0001` is clear, or bit `0x04` of byte `+0x5C` is clear, and a part code (the pose context's `+0x08`) is none of 2, 3 and `0x25`, the part matrix is rotated by head pitch × `2.0` (`0x00745E10`) through the angle-0 rotation helper `0x0243D0`. When flags bit `0x0400` is set, a further rotation by `60.0` (`0x42700000`) is applied with the same helper. The doubling is applied to the stored pitch, so a stored pitch of t degrees draws as 2t on that part.
- `0x02059D` and `0x020831`: (head pitch − the binary32 at `+0x1374`) + a pose value is applied with `0x0243D0` to two matrices of the pose context (`+0x1AC`, `+0x1EC`). Each is followed by a call of matrix helper `0x024A60` with (0, 0, 3.0).
- `0x0217AA` and `0x021FE3`: head pitch × `0.5` (`0x00745DA0`) + a pose value is applied with a different rotation helper (`0x0241B0`), followed by a call of matrix helper `0x024730` with three equal values.

The drawn aim (`+0x138C`) is read by the renderer at `0x0B4157`, `0x0B4336`, `0x0B4444` and `0x0B4706` (the beam use is closed in the held aim contract). The world update reads it at `0x21F2B4` and `0x21F410` (the facing block of the held aim contract). The pose routine's body turn reads the raw aim, as stated in the held aim contract (section 5).

## 8. Unresolved

- The internals of the ray routine `0x1A35D0` (what counts as a hit, its no-hit value, the meaning of its option arguments), of the visibility routine `0x19EE90` and its scale argument, of the admission routine `0x1988A0`, and of the column query feeding the scale. Draws inside them were not audited.
- The axis-to-counter mapping of the eight corners in 3.2, the homogeneous handling in `0x088D60`, and the exact producer and content of the matrix at C+`0x800A90` beyond its copy from the renderer's local at `0x0AED96`.
- The meaning of K `+0x1350` beyond being the receive path's position slot, of `+0x1180`, `+0x1194`, `+0x5C`, `+0x12C`, `+0x1A0`, `+0x1320` and of flags bits `0x0001`, `0x0010`, `0x0040` and `0x0400`.
- Any indirect writer of VA `0x0076B040` or of C+`0x8006D8` inside 3.3's span. The producers of P0 `+0x1440` and of input bytes +0x16 and +0x18. The producer of creature byte `+0x7E` bit `0x04`.
- The import identity behind the focus check of the input sampler, and the confirmation that the 256-byte and 16-byte buffers are the DirectInput keyboard and mouse states (the key names in section 2 are an interpretation of standard scan-code numbering).
- The admission of `0x090A36` (V) and of the controller region beyond the gates listed. The behaviour of the first update after construction, when the matrix at C+`0x1EC` has not yet been built by the late phase.
- How the network send and receive routines are started and scheduled, the lock's re-entrancy, and other message kinds that write individual fields of P0.
- The part identities and matrix order in the pose routine for the head-pitch sites, and whether every creature's facing block runs on the client.
- Stores to P0's yaw or head pitch through component-address helpers inside `0x088EE0`.
