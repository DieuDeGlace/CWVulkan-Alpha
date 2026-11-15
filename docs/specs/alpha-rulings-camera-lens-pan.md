Status: FINAL / STABLE / HELD

# Alpha rulings: camera corner offsets, mode bit 0x400, world-map pan

This document settles three apparent conflicts between held contracts. It does not edit or
replace them. Where a held contract statement is wrong, the correct reading is given here.

## Image and evidence

* Client: `binary/Cube.exe`, 3,885,568 bytes, SHA-256
  `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, image base 0x00400000.
* Server: `binary/Server.exe`, SHA-256
  `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`, image base 0x00400000.
  It is cited only where it is named.
* Addresses are client RVAs unless marked VA or Server. Constants were read from the image
  bytes. Instruction evidence comes from the original listing `decompiled/client/assembly.txt`,
  plus `decompiled/server/assembly.txt` for the Server rows. No reconstruction, runtime
  observation, implementation or capture was consulted.
* "Fixed" means a signed 64-bit value in units of 1/65536 block.

---

## 1. Camera collision corner offsets

**Ruling.** The eight sample offsets are **±0.25 block per axis**, which is raw fixed
**±16,384 (±0x4000)**. They are the corners of a cube with an edge of 0.5 block. They are
offsets from the first desired eye point E. E is the focus minus the fixed conversion of the
view direction times the current distance, so it is the point P of `alpha-physics-camera.md`
§7.3 item 7. `alpha-camera-follow-collision.md` is right. `alpha-physics-camera.md` is wrong
where §7.3 item 8 says "a unit cube centred on P (offsets ±0.5 per axis)".

**Construction** (late follow routine, RVA 0x9c823–0x9c932, loops closed at 0x9c9e0–0x9ca10):

1. Three nested counters each run over 0 and 1. The outer counter gives x, the middle one
   gives y and the inner one gives z, so the visitation order is the one listed in
   `alpha-camera-follow-collision.md`.
2. Each component is the counter converted to binary64, minus binary64 0.5
   (bit pattern 0x3FE0000000000000 at VA 0x745de8). This gives −0.5 or +0.5.
3. The helper at RVA 0x59c00 multiplies each component by binary64 65,536
   (0x40F0000000000000 at VA 0x6fd588) and converts it to a 64-bit integer through the
   library conversion. This gives raw ±32,768.
4. The same 0.5 constant, converted the same way, gives a fixed scalar of 32,768. The helpers
   at RVA 0x2c6e0 and RVA 0x2c900 multiply each component by this scalar: a 64-bit product,
   then signed 64-bit division by 0x10000. This gives raw ±16,384 exactly, which is 0.25 block.
5. The helper at RVA 0x2c800 adds that vector to E to form the sample point. Every sample uses
   the same E. The voxel test and ray call that follow are as in
   `alpha-camera-follow-collision.md`. The ray's maximum length is the unreduced current
   distance.

The ±0.5 figure in `alpha-physics-camera.md` leaves out step 4's second factor of 0.5.

---

## 2. Creature mode bit 0x400

The mode word is the u16 at creature object +0x124, which is creature record +0x114. The
shared set/clear helper is RVA 0x2f160 (Server: its VA 0x405570). It takes a flag and a
set/clear byte and operates on the record.

### 2.1 Readers (client)

| RVA | Routine | Effect while the bit is set |
|---|---|---|
| 0xae4d2 (consumed at 0xae5c1–0xae5cc) | frame render (RVA 0xac260) | the local player's (C+0x8006d0) vertical field of view is binary32 0x3F060A92 = 0.5235988 rad (30°); otherwise 0x3F5F66F4 = 0.8726648 rad (50°). Passed to the projection builder at 0xae5d3. |
| 0x927ff | update (RVA 0x88ee0) | the `crosshair` node (G+0x8008ac) is hidden. It is shown only when not in cursor mode, the local player's bit is clear and game byte +0x14 is 0. |
| 0x928ce | update | the `zoomcrosshair` node (G+0x8008b0) is shown. It is shown only when not in cursor mode and the local player's bit is set. |
| 0x1394c (block 0x13959–0x13ae5) | creature pose routine (RVA 0x128f0) | a pose override. The body pitch slot is binary32 −70 (0xC28C0000). A further slot is +70, and both hand offsets are (0, 3, 1). This is the block that `alpha-creatures-models.md` labels "Sitting (mode 0x400)". |
| 0x20b0a | creature pose routine | an extra X rotation of binary32 60 (0x42700000) through the rotation helper RVA 0x243d0. This is the "Rx(60) while sitting" factor of `alpha-creatures-models.md`. |
| 0x219c17 | creature step (RVA 0x20c510) | when the action is not 79: meter obj +0x1190 grows each step by the step's time value × (k × 0.0005 + 0.00001), capped at 1. Here k is the skill value that RVA 0x3ed60 returns for action 99 (0x63). With the bit clear, the meter falls by 0.0005 × the step's time value, with a lower bound (read as 0). Action 79 has its own earlier branch. On every branch, the meter is then forced to 1 if RVA 0x43f10 with argument 3 returns non-zero. The meter feeds obj +0x170 (RVA 0x219cb7–0x219cef). |

The bit therefore controls **both** the camera lens (the 30° field of view plus the zoom
crosshair) **and** a body pose override. It also drives a skill-scaled meter. One bit does
all of this; there is no separate lens bit.

### 2.2 Writers

* **Clear on intent (client RVA 0x2188a1–0x2188ca, creature step).** The squared length of
  intent (obj +0x40, three binary32, through RVA 0x24860) is compared with binary32 0.0
  (VA 0x745d84). If it is strictly greater, the helper clears 0x400. This runs for each
  creature that the step processes, not only for the local player. The action 83/84 reset on
  the same condition follows at RVA 0x2188cf–0x2188f9. That reset is a separate state: the
  action byte at obj +0x68, not this bit. The Server image mirrors the clear at Server VA
  0x53e661–0x53e68a.
* **No setter found.** No helper call with flag 0x400 and the set byte equal to 1 exists in
  either image. No OR, XOR or bit-set of 0x400 into +0x124/+0x114 exists either. The only other
  writers are whole-field copies and initialisers:
  * dword copies of +0x124 at RVAs 0x4a8fa, 0x4b12b, 0x4b3ea, 0x594fa and 0x6613a;
  * word copies of record +0x114 at RVAs 0x4b0fa and 0x66109;
  * zeroing in constructors and resets, for example RVAs 0x2f3c4, 0x3bc6f, 0x3c1cf, 0x46674
    and 0x207090.

  The bit can reach a creature only through such a copy (for example from received or loaded
  creature data) or through code not covered by these searches.

### 2.3 Rulings on the contract statements

* `alpha-physics-camera.md` §2 item 4, "If |intent|² > 0, mode 0x400 is cleared": **right**.
* `alpha-physics-camera.md` §7.4, "30° … while the local player has mode flag 0x400":
  **right**, but incomplete. The same bit also swaps the crosshairs and overrides the pose.
* `alpha-creatures-models.md`, "0x400 sitting/riding pose (*names inferred from use*)", and
  "Sitting (mode 0x400)": the RVAs and pose values agree with the image. The name **"sitting"
  is not supported** by original evidence. Native sitting is action 83, which "[R] Sit Down"
  and the action 83/84 handling use, and no sit or ride path sets 0x400. A neutral name such
  as "zoom/lens mode" fits the 30° lens and the `zoomcrosshair`. Even that name is an
  inference.
* `alpha-trails-poses-schedules.md`, "intent ends sitting": **right** for action 83/84, and
  separately true for bit 0x400. The two are different fields.

---

## 3. Middle-button drag: world-map pan versus customization preview

Both paths test game byte **G+0xa**. The DirectInput poll (RVA 0xc8ae0) writes it at RVA
0xc9383 from mouse-state button index 2, which is conventionally the middle button. Index 0
goes to +0x4 and index 1 to +0x5. Both paths use the cursor movement held by the interface
object G+0x800710, in window pixels: Δx = +0xd4 − +0xdc and Δy = +0xd8 − +0xe0.

### 3.1 World-map pan (update RVA 0x88ee0, block 0x8b62b–0x8b7fc)

This path applies only while map mode is set (game byte +0x8006e4 ≠ 0) and G+0xa ≠ 0. It runs
once per update, under the lock at G+0x8005d0:

1. v = (Δx, −Δy, 0) × (4 / s). The 4 is binary32 4.0 at VA 0x745e60, and s is the current map
   zoom G+0x1c4. The division is single precision.
2. v is rotated about z by the camera yaw (element 2 of G+0x1a4; helpers RVA 0x24610 and
   0x88e50).
3. The float triple pan at G+0x1000e4c receives v as an addition.

While map mode is 0, the pan is set to (0, 0, 0) (RVA 0x8b7b8–0x8b7f6). The
`alpha-ui-screens.md` §5.2.2 statement "One pixel of drag moves the pan by 4/*s* blocks" is
**right**.

### 3.2 Customization-bench preview drag (mouse callback RVA 0x7ea00, block 0x7ede4–0x7ee7c)

This path is reached only in cursor mode, after the camera-drag test and whether or not the
cursor is over a widget. It applies while G+0xa ≠ 0 and the window **G+0x8008dc** is visible.
It changes the **VoxelWidget** at **G+0x8008f4**, a child of that window:

* VoxelWidget +0x16c += 0.5 × Δx;
* VoxelWidget +0x164 −= 0.5 × Δy.

The 0.5 is binary32 0.5 at VA 0x745da0. There is no zoom factor, sensitivity, yaw or clamp.
The callback then returns. The constructor (RVA 0x187f70) sets +0x164 to −90 and +0x16c to
+90. The widget routine at RVA 0x188500 multiplies both by 0.01745329 (VA 0x6fcda4) and takes
library sine and cosine. These are angles in degrees, so the drag **rotates the 3D voxel
preview**; it does not pan anything.

**Identity of G+0x8008dc.** It is a clone of the `blackwidget` template (G+0x8008c8), sized
400 × 625 and initially hidden (RVA 0x6512a–0x65198). Its child G+0x8008f4 is built by the
VoxelWidget constructor; the RTTI name is `.?AVVoxelWidget@cube@@` (RVA 0x65206–0x65217). In
the update, R at an interaction object of type **77 (0x4d)** toggles this window (RVA
0x9743c–0x9747e). The same code first refreshes the VoxelWidget (RVA 0x18ce20) and then gives
G+0x8008bc the same visibility. Type 77's interaction prompt is **"[R] Use Customization
Bench"** (UTF-16 string at VA 0x701d48). The prompt is selected through the jump table at RVA
0x96b3d: index 76 → case 10 → RVA 0x96eac, whose path pushes it at RVA 0x96ec2. G+0x8008dc is
therefore the **customization-bench window**, with its rotatable voxel preview.

### 3.3 Rulings on the contract statements

* `alpha-physics-camera.md` §7.2, "A separate drag path pans the view state of the screen at
  game +0x8008dc by 0.5 × the cursor movement while byte +0xa is set and that screen is open":
  the gate and the 0.5 rate are **right**. The rest is **wrong**. The target is the child
  VoxelWidget's two angle fields at G+0x8008f4. The y term is subtracted. The effect is a
  preview rotation, and it has nothing to do with the map.
* `alpha-physics-camera.md` reversal 21, "a separate byte +0xa drag pans the screen at
  +0x8008dc at half rate": **wrong** as a map statement. The half-rate path belongs to the
  customization-bench preview. The map pan is the 4/s path of §3.1, which runs in the update,
  not in the mouse callback.
* `alpha-ui-screens.md` §5.2.2, middle-drag pan at 4/*s*, plus its note that the left drag
  turns and tilts: **right**.

---

## Unresolved

* Which code or data path sets mode bit 0x400 is not identified. It may be received or loaded
  creature data copied as a whole field. What gameplay state it represents (scoped aiming or
  similar) is not established, and neither is the meaning of the action-99 skill and the meter
  at obj +0x1190.
* Bulk copies done through generic memory-copy routines, and the network/save deserialisers,
  were not audited for the mode word.
* Which bone slots the 0x400 pose block's −70/+70 values and the 60° rotation address is
  taken from `alpha-creatures-models.md`. It was not re-derived here.
* Whether the map and the customization-bench window can be open at once is not established.
  If they can, one middle drag would both pan the map and rotate the preview. Why G+0x8008bc
  follows the window's visibility is also unknown.
* How the VoxelWidget's two angles map onto its view or model axes (beyond degrees converted
  through sine and cosine) is not established. The VoxelWidget's flag +0x180, which the update
  tests at RVA 0x8b520, is also unknown.
* Calling DirectInput button index 2 "middle" follows the usual device convention. It was not
  confirmed on hardware.
