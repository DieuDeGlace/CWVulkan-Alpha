Status: FINAL / STABLE / HELD. Full author readback completed.

# Alpha creature aim triplet: writers and consumers

## Scope and evidence

This contract lists who writes and who reads the creature aim triplet, the three binary32 values at actor `+0x160`, `+0x164` and `+0x168`. The held contract `alpha-created-actor-combat.md` describes one writer, the created-actor combat continuation. That contract was read in full and is used here only for its terms and its writer. Every other fact below was checked against the original images.

Supported images, both 32-bit with preferred base `0x00400000`. Hashes were verified before use:

- Client `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
- Server `binary/Server.exe`, SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`.

All addresses are RVAs. Evidence comes from the original instruction listings `decompiled/client/assembly.txt` and `decompiled/server/assembly.txt` and from image bytes: jump tables, constants, and call targets in code the listing does not decode. The isolated Infinite-Cubes reconstruction was used only to find candidate sites. No statement relies on it unless it is marked as a lead.

Terms follow the held combat contract: subject S, target T, position (three signed 64-bit fixed-point values at `+0x10/+0x18/+0x20`, with 65536 units per world unit), action `+0x68` (byte), clock `+0x6C`, flags word `+0x124`, movement triplet `+0x40/+0x44/+0x48`, velocity triplet `+0x34/+0x38/+0x3C`, and step (the signed 32-bit milliseconds of this update). These names are added here:

- **Aim** A: the triplet at `+0x160/+0x164/+0x168`.
- **Yaw**: binary32 `+0x30`, the third value of the triplet at `+0x28`, in degrees.
- **Head pitch**: binary32 `+0x58`.
- **Drawn aim**: a second binary32 triplet at `+0x138C/+0x1390/+0x1394`.
- **Size**: binary32 `+0x80/+0x84/+0x88`.
- **B**: the result of subject query `0x03CAA0` with argument −1.
- **C**: the result of subject query `0x047310` with argument −1.
- **R0**: the result of subject query `0x03D1A0`.
- **Face-aim bit**: bit `0x0004` of the flags word.
- **Gate**: a test at the start of an action case. On the client (world byte `+0xB4` nonzero) the case runs only for the local player (world `+0xB8`). On the server it runs for every subject whose byte `+0x60` is nonzero, and for any other subject only if it is the world's local player.
- **Fixed conversion**: each binary32 component times `65536.0` (`0x47800000`), then truncated to signed 64-bit (helper `0x02C460`).
- **Float conversion**: each signed 64-bit component converted to binary32 with one rounding, then multiplied by `1/65536` (helper `0x02C4A0`).
- **Precise normalization**: each component times (`1.0` divided by the binary32 rounding of the binary64 square root of the binary32 squared length). The squared length is formed as (x² + y²) + z².
- **Blend toward a target over the step**: repeated once per millisecond of the step, each time value = (target − value) × rate + value, rounded in binary32. Nothing happens when the step is not positive (helpers `0x011D40`; vector form `0x205950`).

All binary32 arithmetic rounds after each operation. Behaviour with NaN or infinite values, with a zero vector passed to an unconditional normalization, and with overflow is not supplied.

## 1. Storage and network field

The aim is stored as three raw binary32 values. It has no unit vector or angle form: most writers store an un-normalized separation vector, and consumers normalize it as needed.

**Network serialization.** In the entity-state codec, the aim is field index **26** of the 64-bit field mask. Bit 0 is the position. Bit 26 is set when the field is present. The bytes follow as 12 raw bytes in the order `+0x160`, `+0x164`, `+0x168`, with no conversion.

- The client field-by-field reader (`0x0CCFA0`, entity offset `+0x150`, which is the actor's `+0x160`) and the stream receiver (`0x0CD3E0`) read the field only when bit 26 is set. Otherwise the destination keeps its prior value.
- The client writer (`0x0CDAA0`, called from `0x069F81`) includes the field when a full update is requested, or when any of the three components fails an ordered equality test against the previous copy. NaN therefore always counts as changed, and +0 and −0 count as equal.
- The server's reader `0x015DD0` and writer `0x016210` have the same field order around the aim. The server's handler `0x026020` reads a client packet into a copy of that connection's player entity and assigns the copy back (`0x026237`). The server therefore takes a player's aim from that player's client whenever bit 26 is present.
- The client calls its reader and receiver at `0x06BC82` and `0x06C41C`. These call sites were found by scanning image bytes because the listing does not decode them. How the client then applies the result (for example, whether the local player is excluded) is not resolved; see Section 9.

## 2. Writers

### 2.1 Combat continuation (NPC, server-side update)

The held combat contract gives the computation. The common store at `0x02EAEF–0x02EB17` writes T − S after the separation conversion. Some routes replace it (the action-`0x5F` source point, the `+0x1320` replacement) or offset it: for action `0x1C` with d² > 25.0, the first two values receive 1.5·cos and 1.5·sin of clock × 0.005. The same routine also writes the aim directly at `0x02DEF6`, `0x02E0F9` and `0x02E14F` (the (f)/(g) sub-routes), but the common store overwrites those values. The give-up reset, the far route and the special-state branch leave the aim unchanged. The near route sets the face-aim bit. The server's copy is `0x002F40`.

### 2.2 Look-at behaviour update (`0x0C8140`; server `0x014730`)

This is a behaviour update entry with four arguments that returns a byte. Client and server agree instruction for instruction (188 listed instructions). It scans the world's creature map. A candidate must be non-null, have byte `+0x60` equal to 0, and have binary32 `+0x16C` not less than 0.0. For each candidate, the 64-bit difference from S is converted with one rounding per component and × `1/65536`, and its squared length (y² + x²) + z² is formed. The nearest candidate whose squared length is strictly below `64.0` (`0x42800000`) wins. Ties keep the earlier one.

The face-aim bit is then cleared. If a winner exists and the action is neither `0x53` nor `0x54`:

1. The face-aim bit is set.
2. The aim is set to winner − S, converted the same way.
3. The movement triplet is zeroed.
4. Path-clearing helper `0x02EF10` runs.
5. The entry returns 1.

Otherwise it returns 0.

### 2.3 Per-tick path following (world update; client `0x211BC7–0x211D07`, server `0x137987–0x137AC7`)

This runs in the world creature update, on the server only (world `+0xB4` zero), after the behaviour call in Section 4. It runs every world update, not every 32 ms. The steps are:

1. Bit `0x0080` of the flags word is cleared.
2. If the path list headed at `+0x1460` is empty, the block ends, and the aim and the face-aim bit are unchanged.
3. If signed `+0x12C` is positive, the block also ends.
4. Otherwise the face-aim bit is cleared, and world routine `0x1A7EB0` (path following) runs with the subject and the step.
5. If that routine returns nonzero, `0x1A7C90` runs. The aim and the face-aim bit are not written.
6. If it returns zero and the path list is still non-empty, the face-aim bit is set. The aim becomes the float conversion of (C + H − S position). C is the last list element's three 32-bit cell coordinates, each shifted to fixed point (× 65536, sign-extended). H is the half-cell offset of 32768 fixed units in each axis, formed from binary64 `0.5` × `65536.0` and truncated.
7. Bit `0x0080` is then set.

The combat near route clears the path through `0x02EF10`, so after a near-route invocation this block does nothing. After a far-route invocation that left a path, this block replaces the aim on every following update with a vector toward the centre of the path's last cell.

### 2.4 Local player (client controller update `0x088EE0`)

These writes happen in the client controller, not in the world update. Their order relative to the world update within a client frame is not resolved.

- **Path following** (`0x098078–0x098254`): this runs when global byte `0x36B040` is nonzero. Whenever the world clock crosses a multiple of 32 ms, `0x1A7C90` runs. Then up to ten rounds of `0x1AAAB0` followed by `0x1A7C90` run, stopping early when the path length exceeds 50 or the path's last cell equals the goal. The face-aim bit is then cleared and `0x1A7EB0` runs. If that returns zero and the path is non-empty, the face-aim bit is set and the aim is written exactly as in 2.3 step 6.
- **Free control** (`0x09B4C6–0x09B589`): this runs when that byte is zero and build mode is off.
  - If the controller input byte at `+0x14` is set, the aim becomes the unit forward vector of the player's yaw: (−sin y, cos y, 0). Here y is yaw × `0.017453292` (binary32), the cosine and sine are the precise binary64 imports rounded to binary32, and the matrix rotation `0x024610` and direction transform `0x088E50` apply them to (0, 1, 0).
  - Otherwise the aim becomes the float conversion of (an aim point − player position). The aim point is a fixed-point triple held by the controller. Its derivation, believed to be the cursor or camera pick, is not resolved.
- **Build mode** (`0x09B18D–0x09B4C1`): this runs when the controller byte `+0x800704` is set. For each of six held inputs, the aim is increased by a camera-yaw-rotated unit axis × (step as binary32 × `0.005`). The inputs are bytes `+0x0B`, `+0x0C`, `+0x0D`, `+0x0E` and the two that follow. In this mode the aim is a build-cursor offset, not a combat direction.

The reconstruction suggests that the player controller also sets the face-aim bit during attacks. That was not confirmed here (Section 9).

### 2.5 Initialization and copies

Three routines store zeros to all three components: `0x0CE180`, `0x0CF3C0` and `0x0E5C90`. Their roles (construction or reset) were not closed. Several routines copy the whole triplet as part of larger record copies: `0x027CE0`, `0x059C40` (at `0x059DAE`), `0x25C040` and `0x27E660`. Their meaning was not closed.

## 3. Consumers in the world creature update

The world update is client `0x20C510`. The server's copy is `0x1322D0`, at a constant offset of −`0xDA240`. Over the whole body (19,204 listed instructions), client and server agree instruction for instruction once absolute addresses are set aside. Every site below therefore exists in both images at that offset. Where an action case has a gate, the case runs on the server for NPCs and on each client only for that client's own player.

The action dispatch is at `0x211E04`. The action minus 1 is bounded by `0x6D`, then indexed through the byte table `0x222CCC` and the target table `0x222C84`. These tables were read from image bytes.

### 3.1 Rays and beams: actions `0x1C`, `0x5E`, `0x5F` (and `0x68`), case `0x215C68`

These actions share the case. Action `0x68` also uses the ray loop, but with a fixed length. Before the ray, the case handles mana: for `0x68` at clock 0, a charge transfer; for `0x1C`, binary32 `+0x170` is reduced by a per-update binary32 elapsed value × `0.0002` (`0x3951B717`); for `0x5E` and `0x5F`, a cost is deducted at the first crossing. If `+0x170` would be negative, it is set to 0 and nothing else happens this update.

**Timing.**

- The case requires clock + step ≥ B and clock < B + C.
- A **first pulse** happens when clock ≤ B < clock + step.
- After that, only actions `0x5F` and `0x1C` pulse again: once each time the signed truncating quotient by 200 of (clock + step − B) differs from that of (clock − B), and only while clock + step > B.

**Direction and origin (per pulse, `0x215F5C–0x215FDD`).**

- A local direction D is a copy of the aim. It is normalized precisely only if its squared length is strictly greater than 0.0. Otherwise it stays as it is.
- The origin O is the sight point from subject routine `0x046BB0`, which works in 64-bit fixed point:
  - If bit `0x04` of byte `+0x7E` is clear, O is the position.
  - Otherwise, let a = yaw × `0.017453292` and form (−sin a × (size `+0x84` × 0.5), cos a × (size `+0x84` × 0.5), size `+0x88` × `0.35`). Cosine and sine are the precise imports rounded to binary32. The vector passes through an identity-based rotation and a homogeneous divide by exactly 1.0. Each component is then given the fixed conversion and added to the position.
- The length L is the binary32 result of world ray routine `0x1A35D0`. It is called with O, a separately and unconditionally normalized copy of the aim, maximum `200.0` (`0x43480000`), and flags 0 and 1. For action `0x68`, L is replaced by `10.0`.

**The creature's beam or shot direction is the aim, normalized.** No yaw, target id or drawn aim is involved in the gameplay ray.

**Sampling.** Two per-subject sets (`+0x11AC`, `+0x11B4`) are cleared. If L > 0.0, a sample point P starts at O, and a counter i starts at 0 (its binary32 value is "travelled"). The loop repeats while L > binary32(i), with i increasing after each pass. After each pass, P is advanced by the fixed conversion of D. A zero aim therefore never advances P.

In each pass, every other creature in the world map is tested. A creature is skipped when any of these holds:

- it is null or is the subject;
- its 64-bit id equals the subject's `+0x11C0`;
- its `+0x16C` is not strictly greater than 0.0;
- it is already in set `+0x11AC`;
- its `+0x128` is nonzero.

A creature is hit when both of these hold:

- **Horizontal test.** Let r = (its size `+0x80` × 0.5) + 1.0, plus "travelled" for action `0x68`. In 64-bit fixed point, the first two components of P − creature position are each squared with wrapping multiply and signed-divided by 65536, and the two are summed. The sum must be strictly less than the truncated value of r² × 65536.
- **Vertical test.** The absolute value (binary64 and-mask) of the binary32 conversion of the third component difference must be strictly less than (its size `+0x88` × 0.5) + 1.0.

A creature that passes both tests is inserted into set `+0x11AC`. One direct `rand` is made for it (`0x216460`), plus one more for action `0x68` (`0x21652A`). Only then are admission routine `0x1988A0` and hit routine `0x196D30` applied. For action `0x68` only, the hit vector passed on is the unconditionally normalized aim × `10.0`, with its third component then set to `5.0`. Other actions pass a zero vector. If any admitted hit occurs, `+0x70` is incremented and `+0x74` is zeroed after the loop.

**After the loop.** If set `+0x11AC` is still empty (no creature passed the geometric tests) and the action is not `0x68`, an effect record is emitted (`0x2169EC–0x216B1E`). Its point is S position + fixed conversion of (unconditionally normalized aim × L), and it carries D. Its consumers were not closed. In the same branch `+0x70` is zeroed, and the clock is rescaled: it becomes the truncation of (q₁ / q₂) × clock, where q₁ and q₂ are the results of subject query `0x047700` before and after that zeroing. The water-cloud projectile (`+0x140` = 3 and `+0x141` = 1, first pulse only) is placed at O + fixed conversion of (normalized aim × L).

### 3.2 Teleport-like action `0x31`, case `0x21548F` (gate as above)

On the crossing clock ≤ B < clock + step, a centre point Q = position + fixed conversion of the aim. The search runs over x from −4 to 4 (outer), y from −4 to 4, and z from −2 to 6 (inner), in whole cells added to Q. Each offset that is not blocked (`0x19F0A0` with size `+0x80`) is a candidate. The candidate whose binary32-converted squared distance to Q is strictly smallest, starting from `400.0`, becomes the destination. If the best is below 400.0, the position moves there and then descends cell by cell, stopping at a water block. Binary32 `+0x13BC` receives the new height. The aim is used here as an un-normalized offset, so its length matters.

### 3.3 Projectile spawns: direction from the aim; origin is the plain position

In every case below, the projectile record takes its position from S position (`+0x10`), not from the sight point. The owner id is S `+0x08`, and a 12-byte value is copied from `+0x1B0`. The record is pushed to world list `+0x14` and to the output list `+0x20`.

- **Actions `0x16`, `0x17`, `0x1A`** (case `0x21707A`). A shot is spawned on the crossing clock ≤ B < clock + step. For action `0x1A`, a second shot is spawned on the crossing of B + 300 when binary32 `+0x13B4` > 0. Velocity = unconditionally normalized aim × `100.0` (`0x42C80000`), at `0x2171ED`. There is one direct `rand` per shot (`0x2174D9`).
- **Actions `0x32`, `0x37`, `0x60`** (case `0x217634`). The period P = C signed-divided by 4 with truncation. A shot is spawned when clock ≥ B, clock < B + C, the signed truncating quotients of clock + step and clock by P differ, and the aim's squared length is strictly greater than 0.0. Velocity = normalized aim × `150.0` (`0x43160000`). There are two direct `rand` calls per shot: `0x217812` (sound) and `0x2179AB`.
- **Actions `0x25`–`0x2E` and `0x6C`** (case `0x217A21`). A shot is spawned when clock < B + C and clock ≤ E < clock + step. The release time E is B for most of these actions. For `0x2D` and `0x2E` it is derived from C ÷ 3 (not closed). The aim's squared length must be strictly greater than 0.0. Velocity = normalized aim × `100.0` when byte `+0xAA9` is `0x0C`, and × `50.0` (`0x42480000`) otherwise. For action `0x6C`, it is normalized aim × `100.0`. For race `+0x64` = `0x65`, the origin gets an offset from size. One direct `rand` is used for strength.

A creature's shot direction is therefore the normalized aim at the moment of spawning. The projectile is not re-aimed later, except for the boomerang in 3.6.

### 3.4 Close spells, actions `0x1E`–`0x21`

- In case `0x212357`, at `0x212495`, a sound is emitted every 200 ms while clock < B (one `rand` for pitch). On the crossing clock < B ≤ clock + step, a particle burst is placed at position + fixed conversion of the aim (`0x212599`).
- In the same case's strike geometry (`0x2137E9` onward), the strike centre is first set to position + the yaw-rotated forward offset. For these four actions it is then replaced by position + fixed conversion of the aim (`0x213A42`).

The aim is an un-normalized offset here too. The combat continuation's `+0x1320` replacement (combat contract 6.4 (e)) is what places the spell at an earlier target point.

### 3.5 Melee strikes: direction from yaw, not from the aim

For every other strike in case `0x212357`, the hit-volume centre is position + fixed conversion of (a matrix rotation by yaw applied to (0, width × share, 0)) (`0x213806–0x213A29`). Width and share depend on the action. The aim is not read. A melee attack therefore follows the aim only indirectly, through the yaw turn in 3.7, which is smoothed and reads the drawn aim.

### 3.6 Boomerang steering (projectile update after the creature loop, `0x220AC0` onward)

This applies to a projectile record of type 2 (record `+0x60`). The owner is looked up by id. The record's age at `+0x68` is in milliseconds.

- If the owner is missing, or the age is above 1200, the record is spent.
- Above 1100, the record is pulled back to the owner.
- From above 800 up to 1100, the velocity becomes (owner position − projectile position) after float conversion, × `10.0`.
- At age 800 or below, the velocity at record `+0x3C` is blended toward ((owner position + fixed conversion of the owner's aim) − projectile position) after float conversion, × `5.0`. The blend runs over the step at rate `0.05` (`0x220BF6`). The aim is used as an un-normalized offset that defines a target point.

### 3.7 Facing: yaw and head pitch, from the drawn aim (`0x21F29A–0x21F62B`)

This runs when the face-aim bit is set and signed `+0x12C` ≤ 0. It runs after the action dispatch and movement of the same update, and before the drawn-aim store in 3.8. It therefore reads the drawn aim left by the previous update.

1. **Yaw.** Let (x, y) be the first two drawn-aim components. If x² + y² is strictly greater than 0.0, and a per-update "driven" flag is clear, the following happens:
   1. (x, y) is normalized precisely in two dimensions.
   2. y is clamped to [−1, 1].
   3. A heading h is formed. When x > 0, h = (−acos y) / π × 180. Otherwise h = (acos y) / π × 180. The arccosine is the precise import rounded to binary32. The negation is in binary32, and the division by π and multiplication by 180 are in binary64, rounded to binary32.
   4. Yaw is blended over the step at rate `0.1` (`0x3DCCCCCD`) toward the angle-interpolation result of (yaw, h, 1.0).

   Angle interpolation (`0x023F70`) converts both angles with ÷180 then ×π, and forms s = sin b · cos a − cos b · sin a in binary64 from binary32-rounded trigonometry. It clamps s to [−1, 1] and returns a + asin(s) × (1.0 × 180/π). The result therefore moves at most a quarter turn per evaluation and folds larger differences.

   The "driven" flag is cleared at `0x21A269`. Certain action-driven movements set it (for example at `0x21A381`). Their full list is not closed.
2. **Head pitch.** If the drawn aim's squared length (three components) is strictly greater than 0.0, the drawn aim is normalized precisely. A tilt t = asin(z) / π × 180 is formed. Head pitch `+0x58` is blended over the step at rate `0.005` toward the angle-interpolation result of (pitch, t, 1.0).

When the face-aim bit is clear (or `+0x12C` is positive), the yaw comes from movement instead. If `+0x128` is 0, the horizontal velocity squared length is greater than `0.02`, the three-component dot product of velocity and movement triplet is greater than 0.0, and the "driven" flag is clear, yaw is set directly (without blending) to the heading of the normalized horizontal velocity, using the same formula. No draws occur in this block.

### 3.8 Drawn aim (`0x21FD54–0x21FDFC`)

At the end of each creature's update:

1. A share s starts at 0.0 and is blended toward 1.0 over the step at rate `0.025` (`0x3CCCCCCD`).
2. The drawn aim is increased by (aim − drawn aim) × s, component-wise.
3. If the action is 0, or the clock is 0, or clock > R0, the drawn aim is then set equal to the aim.

The yaw and pitch of 3.7 and the client's beam drawing (Section 5) read the drawn aim. The gameplay ray and the projectiles do not.

### 3.9 Movement speed limit (`0x21A952–0x21AA3F`)

This runs when the face-aim bit is set and `+0x128` is 0, except during action `0x32` or `0x60` with clock ≤ 1000. Let a be the aim's first two components and v the velocity's first two components. If the two-dimensional dot product (v.y × a.y) + (v.x × a.x) is strictly less than 0.0, both are normalized precisely in two dimensions. The current speed limit is then multiplied by (their dot product × 0.5 + 1.0). Moving away from the aim is thus at most half as fast.

## 4. Order within one world update (server, per creature)

The relevant order inside the creature loop is:

1. **Behaviour.** For a subject whose byte `+0x60` is nonzero, the step is added to `+0x13C4`. If signed `+0x12C` ≤ 0 and that accumulator is at least 32, the behaviour object at `+0x13E4` (if non-null) is called through its table entry 0 with subject, world, accumulated milliseconds and output, and the accumulator is reset to 0. While `+0x12C` is positive, bit `0x0080` is cleared instead (`0x211AE1–0x211B75`). This is where the combat continuation (2.1) and the look-at behaviour (2.2) write the aim. They run at most once per 32 ms of accumulated time.
2. **Path following** (2.3), every update.
3. Timers.
4. **Action dispatch** (3.1–3.5), which reads the aim as it stands after steps 1 and 2.
5. **Movement**, including the speed limit (3.9).
6. **Facing** from the drawn aim (3.7).
7. **Drawn-aim update** (3.8).

After the creature loop, projectiles are advanced, including the boomerang (3.6).

Consequences: a beam or shot fired in an update uses the aim the behaviour wrote in that update or in an earlier one, possibly up to the behaviour cadence earlier. If a path is pending, it uses the path-follow aim instead. For the action-`0x1C` beam, the sweep offset written by the combat continuation therefore moves the gameplay ray directly, at the behaviour cadence. The visible turn and the beam drawing lag behind through the drawn-aim smoothing.

The world update itself makes no direct `rand` call when it writes or reads the aim. The draws named in 3.1–3.4 belong to sound, damage and strength and to hit admission, and occur only when shots, pulses or hits occur.

## 5. Client-only consumers (pose and drawing)

These consumers affect no gameplay state that was observed:

- **Pose body turn** (animation routine `0x0128F0`, at `0x0131B1–0x013349`). When the face-aim bit is set, the movement triplet's horizontal squared length is above `5.0`, and the aim's horizontal squared length is above 0.0, both horizontal vectors are normalized precisely. Let p be the normalized movement and f the normalized aim. A sine s = (f.x × p.y) − (f.y × p.x) is clamped to [−1, 1]. A turn angle is formed as binary32(asin(s) × `57.29577951308232`). The angle is negated when (f.y × p.y) + (f.x × p.x) < −0.2. This reads the **raw aim**, not the drawn aim.
- **Beam pose** (`0x01EF7B–0x01F065`). While the action is `0x1C`, a pose vector is blended toward the aim (normalized when its squared length is above 0). It uses a share obtained by blending 0 toward 1 at rate `0.01` over the frame's milliseconds. For any other action it is zero.
- **Beam drawing** (client draw routine `0x0AC260`, at `0x0B4706`). The drawn beam uses the unconditionally normalized **drawn aim**, origin `0x046BB0`, and ray `0x1A35D0` with maximum `100.0`. The gameplay ray uses the aim with maximum 200.0. Other drawing reads, not closed further, are at `0x0B4157` (drawn aim, close spells), `0x0B33A3` (the player's build preview at position + aim), and `0x0B68D3`, `0x0B6D5E`, `0x0B6E54` (impact effects along the normalized aim).

## 6. Answer: is a creature's shot or beam direction derived from the aim?

Yes, for ranged attacks:

- The gameplay beam or ray direction is the aim normalized, from the sight point.
- Projectile velocity is the normalized aim × 100, 150 or 50, from the plain position.
- Teleport and close-spell placement use position + aim, un-normalized.
- The outbound boomerang homes on the owner's position + aim.

Melee strike volumes do not read the aim. They use the yaw, which the face-aim facing turns toward the smoothed drawn aim.

## 7. Random draws

No writer or consumer of the aim draws a random number to compute or apply it. The draws named in 3.1, 3.3 and 3.4 are made in the same cases, for other quantities. Draws inside the hit admission and hit routines (`0x1988A0`, `0x196D30`), the ray routine `0x1A35D0` and the path routines were not audited.

## 8. Server agreement

- The world update (Sections 2.3, 3 and 4) is identical in both images at offset −`0xDA240`. This was established by comparing the whole body.
- The combat continuation agrees (held contract).
- The look-at behaviour agrees (2.2).
- The entity codec's field order around the aim agrees, with bit 26.

The server has no counterpart of the controller (2.4), the pose (5) or the drawing (5) routines.

## 9. Unresolved

- How the client applies a received entity update (call sites `0x06BC82`, `0x06C41C`): whether the local player's aim is excluded or overwritten, and when.
- The order of the controller's aim writes (2.4) relative to the client world update in a frame. The origin of the controller's aim point. The global byte `0x36B040`. Whether the controller sets the face-aim bit during attacks (a reconstruction lead only).
- The roles of the zeroing routines `0x0CE180`, `0x0CF3C0`, `0x0E5C90` and of the whole-record copies.
- The full list of actions that set the "driven" flag in 3.7. Whether every creature reaches the facing and drawn-aim blocks on the client.
- The consumers and full fields of the effect record emitted after an empty ray loop, and the meaning of query `0x047700`. The meaning of fields `+0x11C0`, `+0x12C`, `+0x128`, `+0x13B4`, `+0x13BC` and `+0x1B0` beyond their use here.
- The internals of the ray routine `0x1A35D0`, path routines `0x1A7EB0` and `0x1A7C90`, hit routines `0x1988A0` and `0x196D30`, blocking test `0x19F0A0`, and duration queries B, C and R0.
- Behaviour with a zero aim in the unconditionally normalized consumers (ray length, action-`0x68` hit vector, the `0x16`/`0x17`/`0x1A` and `0x6C` shots, beam drawing).
- The gameplay meaning of the action codes.
