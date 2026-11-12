Status: FINAL / STABLE / HELD. Full author readback completed.

# Alpha combat cast-point producers

## Scope and evidence

This contract closes three items that the held contract `alpha-created-actor-combat.md` leaves open in its §6.4(e) step 3, §6.4(b) step 2 and §11:

- every writer of the subject's 24-byte field at `+0x1320`, which that contract's dispatch class (e) copies into and later reads as a cast point;
- every writer of the 24-byte field at `+0x1350`, which class (e) copies from the target;
- the full computation of point routine `0x046BB0`, which class (b) uses when bit `0x04` of subject byte `+0x7E` is set.

It also states when each writer runs relative to the combat continuation. The held contract was read whole and is treated as the consumer description. Every fact below was checked in the original images.

Supported images, both 32-bit with preferred base `0x00400000`. Hashes were verified before use:

- Client `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
- Server `binary/Server.exe`, SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`.

All addresses are RVAs. Evidence came from the original instruction listings `decompiled/client/assembly.txt` and `decompiled/server/assembly.txt`, the call tables `calls.csv` and symbol tables `symbols.csv` beside them, and image bytes for every constant. Decompiler output was used only as a lead. One reconstruction source was consulted for leads only, and its field descriptions were not taken as fact. Where it suggested writers that the original images lack, this contract follows the images (see §7).

Terms used here:

- **Actor**: an object of the class the combat continuation updates. Its position is three signed 64-bit fixed-point coordinates at `+0x10`, `+0x18` and `+0x20`, with 65536 fixed-point units per world unit.
- **Cast point**: the actor's 24-byte field `+0x1320..+0x1337`.
- **Follow point**: the actor's 24-byte field `+0x1350..+0x1367`.
- Both fields hold three signed 64-bit fixed-point coordinates, in the same absolute world space and units as the position, stored in x, y, z order.
- **Subject** (S) and **target** (T) are used as in the held contract.
- **Fixed conversion of a binary32 triple**: each component is multiplied in binary32 by `65536.0` and then truncated toward zero to signed 64-bit. The truncation uses the runtime float-to-integer helper (client `0x28D946`, server `0x14A946`).
- **Float conversion of a fixed triple**: each signed 64-bit component is converted to binary32 and multiplied by `1/65536`. This is the held contract's separation conversion.

These names describe identities and storage only. They do not assert gameplay meaning.

All binary32 arithmetic rounds after each operation. Behaviour with NaN or infinite operands, with 64-bit overflow, and with invalid pointers is not supplied.

## 1. Writers of the cast point `+0x1320`

The search covered every listed instruction in both images whose operand addresses `+0x1320` through `+0x1367` from a general register, including address formations (`LEA`, `ADD reg, 0x1320`/`0x1350`) that hand the field to a helper. The actor constructor (client `0x03B690`, server `0x006400`) writes neither the cast point nor anything between `+0x1320` and `+0x134F`.

### 1.1 Combat continuation, class (e) (both images)

Client `0x02DDC3–0x02DE2A` in routine `0x02CB20`, and server `0x0041E3–0x00424A` in routine `0x002F40`, run this sequence. The sequence matches the held contract's §6.4(e) step 3:

1. It runs only when the subject's action byte `+0x68` is `0x1E`, `0x1F`, `0x20` or `0x21`. The action is read after routine `0x0456A0` or `0x045410` (server `0x010690` / `0x010400`) has run in step 2 of that class.
2. B is queried: client `0x03CAA0`, server `0x007DB0`, each with argument −1.
3. The value (subject `+0x6C` − step) is formed in signed 32-bit arithmetic. The step is the integer step argument of the update entry.
4. When the value is at most B, the 24 bytes of T's follow point are copied unchanged into S's cast point (client copy helper `0x02C5B0`, server `0x002A40`). No conversion or offset is applied.
5. When the value is greater than B, the cast point is only read, as the held contract describes.

This is the only server writer of the cast point.

### 1.2 Client local-actor controller (client only)

Client routine `0x088EE0` is the per-frame controller update. Its first argument is a signed 32-bit frame step, held in a stack slot. The routine maintains a frame-local 24-byte fixed-point point, called **L** here. L is built in this order:

1. At `0x098409–0x098456`, L is set to the controller's fixed point at controller `+0x158` plus the fixed conversion of a local binary32 direction scaled by `100.0` (immediate `0x42C80000`).
2. World ray routine `0x1A35D0` is called with that direction, the point at controller `+0x158`, a maximum of `60.0` (immediate `0x42700000`) and two further integer arguments, 0 and 1. If its binary32 result d is at least `0.0` (`0x345D84`), L is replaced at `0x0984AC–0x0984F6` by the point at `+0x158` plus the fixed conversion of (d × direction).
3. A scan of nearby actors (`0x098690–0x098D09`) reads each candidate's follow point and keeps a nearest value h. If h is at least `0.0` and the local actor's action is not `0x31`, a candidate point Q is formed at `0x098D6E–0x098E1D`. Q is the controller's point at `+0x140` plus the fixed conversion of (binary32 rounding of the precise binary64 square root of h) × direction. Q replaces L only when the binary32 dot product of the direction with the float conversion of (Q − local actor position) is strictly greater than `0.0`.

Then, at `0x098E7C–0x098ECC`, the routine applies the same kind of test as §1.1 to the local actor, which is the actor pointer at controller `+0x8006D0`:

- The test runs only when that actor's action is `0x1E`, `0x1F`, `0x20` or `0x21`.
- B is queried (`0x03CAA0`, argument −1).
- If (actor `+0x6C` − frame step) is at most B, L is copied into the actor's cast point.
- Otherwise, the actor's cast point is copied back into L.

During the first part of such an action, the local actor's cast point therefore follows L each frame. After that, it holds the last value written, and the routine's later uses of L see the held value. This writer has no server counterpart.

The direction vector and the controller points at `+0x140` and `+0x158` are not traced here. Neither are the internals of the ray routine and of the actor scan (see §8).

### 1.3 Readers that are not writers

- **World update (both images).** Client `0x2183B9` in `0x20C510`, and server `0x13E179` in `0x1322D0`, pass the cast point's address to an actor routine (client `0x196D30`, server `0x0CFD50`) as its seventh argument. Neither routine reads its seventh argument: there is no access to `[EBP+0x20]` in either body. This is not a consumer in effect.
- **Container routine `0x0EC400` (client).** It compares integer offsets such as `0x1320` against the length of a four-byte element container. It does not address an actor.

### 1.4 Initial value

No writer runs at construction. A cast point that is read through the latched branch of §1.1 before §1.1 has ever copied into it holds whatever the allocation contained. Whether actor storage is zero-filled on allocation was not established.

## 2. Writers of the follow point `+0x1350`

### 2.1 Construction (both images)

The actor constructor stores zero in all six dwords `+0x1350..+0x1364`: client `0x03B836–0x03B868`, server `0x0065A6–0x0065D8`.

### 2.2 World-update per-actor step (both images)

The world update is client `0x20C510` and server `0x1322D0`. Addresses in §2.2 and §2.3 are server RVAs. The client counterpart of each is the server RVA + `0xDA240` (§6). Its per-actor loop walks the world's ordered actor map at world `+0x04`, in ascending signed 64-bit identifier order. The ordering was confirmed in the map-find comparator, server `0x130600`, which compares the high dwords signed and the low dwords unsigned.

The loop body runs from server `0x136290` to `0x145BE1`; the back edge is at `0x145C1B`. The body is skipped for a null entry (`0x1362A6`) and for an actor whose binary32 `+0x16C` is not strictly positive (`0x13789B`). For each remaining actor, the body does the following, in this order:

1. **Behaviour invocation**, at server `0x1378A1–0x137917`. It runs when all of the following hold:
   - actor byte `+0x60` is nonzero;
   - world byte `+0xB4` is zero;
   - the actor's signed 32-bit `+0x12C` is not positive;
   - after the update step has been added to it, the actor's `+0x13C4` is at least 32.

   The actor's behaviour root is then called through its virtual entry with the actor, the world, the value of `+0x13C4` and the output. Afterwards `+0x13C4` is reset to zero. The combat continuation, and with it the cast-point copy of §1.1, runs inside this call.
2. The actor's movement and the rest of its step follow, ending with an optional airship-crew placement that may overwrite the position (`0x14549A–0x145722`).
3. **Follow-point update**, at server `0x145728–0x145B14`. Exactly one of three branches runs:
   - **(a) Carried pet.** This branch runs when actor byte `+0x60` is `5`, the actor whose identifier is held in the 64-bit `+0x198` exists (lookup `0x005420`), and that owner's action byte `+0x68` is `0x6A`. A height z is formed in binary32 as ((actor `+0x88` × `0.5`) − (owner `+0x88` × `0.5`)) + `0.01`. The owner lookup is the world's identifier lookup on its map at world `+0x04`. The constants are `0x3F000000` at server `0x1586D0` and `0x3C23D70A` at server `0x1586A8`. The follow point becomes the owner's follow point plus the fixed conversion of (0, 0, z), added with 64-bit wrapping. The same branch also:
     - copies the owner's binary32 triple `+0x1374`;
     - sets the actor's position to the owner's position plus the same offset;
     - copies the owner's `+0x28` triple and the owner's `+0x1180`, `+0x1188` and `+0x118C`;
     - zeroes the actor's `+0x34` and `+0x40` triples.
   - **(b) Smoothing.** This branch runs otherwise, when the signed 32-bit `+0x1398` is strictly positive. A factor f starts at `0.0` and is moved toward `1.0` at rate `0.015` (immediate `0x3C75C28F`) by helper `0x12E710`. That helper repeats f ← ((1.0 − f) × rate) + f in binary32, once per millisecond of the update step; a step that is not positive makes no iteration. The follow point then receives (position − follow point) × F component-wise, where F = truncation of (f × 65536.0) to signed 64-bit. Each fixed product is a wrapping 64-bit multiplication followed by signed division by 65536 that truncates toward zero. After that, the third coordinate alone additionally receives the truncation to signed 64-bit of (binary32 rounding of (position z − the updated follow z)) × f. The branch also interpolates the binary32 triple `+0x1374` and clears `+0x1180`. Neither image contains a store of a nonzero value to `+0x1398`: the only stores are zeros at client `0x03B890`, `0x04646B`, `0x047198` and server `0x006600`, `0x01120B`. On the evidence searched, this branch is unreachable.
   - **(c) Default.** The follow point becomes a 24-byte copy of the actor's position at `+0x10`, and `+0x1374` becomes a copy of `+0x28` (`0x145AF6–0x145B0E`). No conversion is applied.

### 2.3 After the per-actor loop (both images)

These two passes run after every actor's step in the same world update, and therefore after every behaviour invocation of that update.

1. **Airship riders**, at server `0x1464A2–0x14670B`. Each airship's smoothed position at airship `+0x80` and its smoothed heading at airship `+0x98` are advanced first. Then, for each rider identifier in the airship's set at `+0x78`, if the actor exists and the binary32 squared length of the float conversion of (rider position − airship `+0x10`) is not above `4096.0` (server `0x173CC0`), the following happen:
   - A rotation about the vertical axis is built (`0x001CD0`, `0x002030`). Its angle is the change of the airship's smoothed heading during this update, in degrees, converted by × `0.0174532924` (`0x3C8EFA35`). Its cosine and sine use the precise binary64 imports, each rounded to binary32.
   - The rider's follow point becomes the airship's new smoothed position plus the fixed conversion of the rotation applied (`0x0021F0`) to the float conversion of (rider follow point − the airship's smoothed position before this update).
   - The rider's position is transformed the same way.
   - The heading change is added in binary32 to the third components of the rider's `+0x1374` and `+0x28`.

   Riders further away are skipped and removed from the set.
2. **Carried pets, second pass**, at server `0x146C50–0x146EC2`. The pass walks the actor map again, in the same order, and runs no positive-`+0x16C` test.
   - First, for any actor whose referenced actor at 64-bit `+0x11D0` exists with `+0x16C` not strictly positive, that reference is zeroed.
   - Then branch (a) of §2.2 is repeated with its full effect, using the same constants and order, whenever its conditions hold.

### 2.4 Client-only writers

These run only in client controller or interface code. Each stores a plain 24-byte copy, with no conversion:

- `0x06B63C–0x06B669` (in `0x06A8A0`): the local actor's follow point becomes its own position.
- `0x08088F–0x0808BC` (in `0x0806C0`): after this routine places an actor at a stored position, that actor's follow point becomes its new position.
- `0x082407–0x082434` (in `0x0821A0`): the follow point of an actor freshly constructed from the local actor's state becomes the new actor's position.
- `0x207107–0x207134` (in `0x205AE0`): an actor's follow point becomes its own position, and `+0x137C` is cleared.

Every other reference to `+0x1350` in the client is a read. These include the controller's actor scan in `0x088EE0`, the drawing routines `0x0128F0`, `0x0AC260` and `0x1FC1B0`, and visibility helper `0x07F760`. Their called helpers write only to separate outputs. This was checked for `0x0120B0`, `0x012220`, `0x012260`, `0x0122C0`, `0x0122E0`, `0x02C7A0` and `0x02C800`.

## 3. Timing relative to the combat continuation

On the server, the combat continuation of subject S runs inside S's behaviour invocation (§2.2 step 1). This is before S's own movement and follow-point update in the same world update, and only on updates where S's behaviour gate opens.

When class (e) copies T's follow point, the value it obtains depends on where T falls in the update order:

- **T has a lower identifier than S** and was processed earlier in the same update: T's position at the end of T's step in this update, or the carried-pet value of branch (a).
- **T has a higher identifier**: the value left by the previous world update. That is T's end-of-step value from that update, as modified afterwards by the airship-rider and carried-pet passes of §2.3.
- **T was skipped by the positive-`+0x16C` test**: the value from the last update in which T was processed.
- **T has not completed a per-actor step since construction**, and no pass of §2.3 has written it: zero in all three coordinates, from §2.1.

The follow point can therefore differ from T's current position. This happens whenever T's position was changed after T's last follow-point update, for example by incoming state, by the passes of §2.3, or by T's own step later in the same update.

The cast point is a latch. During the part of an action in {`0x1E`..`0x21`} for which (clock − step) ≤ B, each behaviour invocation that reaches class (e) refreshes it from T's follow point. Afterwards it keeps the last value copied, which may come from an earlier action or an earlier target.

On the client, the same world-update structure exists (§6). Its behaviour gate also requires world byte `+0xB4` to be zero. The controller writer of §1.2 runs in the controller update. Its order relative to the client world update is not established here.

## 4. Point routine `0x046BB0`

Client `0x046BB0` and server `0x011800` are a subject routine. They take one argument: the address of a 24-byte output. They return that address. The output is computed as follows.

1. **Flag clear.** If bit `0x04` of subject byte `+0x7E` is clear, the output is a copy of the subject's position. No other work is done.
2. **Flag set.** The following steps run:
   1. An angle a is formed in binary32 as subject `+0x30` × `0.0174532924`. The constant is `0x3C8EFA35`, at client `0x2FCDA4` and server `0x1586AC`. Subject `+0x30` is the third component of the binary32 triple at `+0x28`; the constant converts degrees to radians.
   2. c is the binary32 rounding of the precise binary64 cosine of a: client import `0x28D7A4` / `0x2FC444`, server `0x14A9C2`. s is computed the same way with the precise sine: client `0x28D9C2` / `0x2FC45C`, server `0x14A9C8`. The cosine is evaluated first.
   3. h is subject `+0x84` × `0.5` (`0x3F000000`, client `0x345DA0`, server `0x1586D0`). v is subject `+0x88` × `0.35` (`0x3EB33333`, client `0x2FD8D8`, server `0x1588B8`). Both are binary32 products; `+0x84` and `+0x88` are the second and third components of the binary32 triple at `+0x80`.
   4. The offset is the binary32 triple (−(h × s), h × c, v), in x, y, z order.
3. **How the offset is computed.** The routine builds a 4×4 identity matrix (`0x023E70`, server `0x001CD0`). It applies a rotation about the vertical axis with c and s, transforms the homogeneous vector (0, h, v, 1), and divides by the resulting w component through the reciprocal 1.0 ÷ w (`0x3F800000`). Every term that this adds beyond the three listed products is a product with an exact zero, a sum with an exact zero, or a multiplication by exactly 1.0, because w evaluates to exactly 1.0. Each component therefore equals its single rounded binary32 product, with the first component negated exactly. Signed zeros may differ in the intermediate terms, but this cannot affect the result after the conversion in step 4.
4. **Conversion and output.** Each offset component is converted with the fixed conversion: × `65536.0` (client `0x2FCE7C`, server `0x15878C`), then truncation to signed 64-bit. The output is the subject's position plus these three values, with 64-bit wrapping addition per coordinate.

The routine reads no other state. It makes no draws, and no draw is statically reachable from it in either image.

Consumers of this routine are outside this contract except for class (b) of the combat continuation (client call `0x02DA82`, server `0x003EA2`). They are a world-update use (client `0x215FAB`, server `0x13BD6B`) and two client drawing uses (`0x0B471B`, `0x0B64CA`).

## 5. Random draws

No producer described here makes a direct `rand` or `srand` call. A static walk of the direct-call graph found none reachable, and found no indirect call, from the following:

- server `0x011800`;
- the server follow-point blocks of §2.2 step 3 and §2.3, with their direct helpers;
- client `0x046BB0`;
- the client counterparts of those blocks;
- the 24-byte copy helpers.

The behaviour invocation in §2.2 step 1 is an indirect call; its draws belong to the behaviour contracts. The client controller path of §1.2 calls the world ray routine `0x1A35D0` and scan helpers whose transitive draws were not audited. These producers therefore do not change the combat continuation's draw order.

## 6. Client and server agreement

- **Point routine.** Client `0x046BB0` and server `0x011800` have 189 listed instructions each. They are identical once absolute addresses are set aside, and every constant they use has the same bit pattern in both images.
- **Combat copy.** The sequence of §1.1 is identical at the held contract's constant offset (server = client − `0x29BE0`).
- **World update.** The follow-point blocks are identical at server = client − `0xDA240`: 451 instructions for server `0x145480–0x145C21` against client `0x21F6C0–0x21FE61`, and 566 instructions for server `0x146540–0x146E60` against client `0x220780–0x2210A0`. So is the behaviour gate (server `0x1378A1`, client `0x211AE1`). Every constant used has the same value in both images: `0.5`, `0.01`, `0.25`, `1.0`, `30.0`, `−0.5`, `0.001`, `100.0` and `4096.0`.
- **Client only.** The controller cast-point writer of §1.2 and the follow-point writers of §2.4 have no server counterpart.

## 7. Divergence from the reconstruction lead

A reconstruction lead described further client follow-point writers that run when actor state arrives from the network. It also described a positive smoothing time that would enable branch (b). Neither exists in the client image: there is no store to `+0x1338..+0x134F`, and no nonzero store to `+0x1398`. The lead also presented the world-update use of the cast point as an input to the hit routine. In both images, that argument is never read (§1.3).

## 8. Unresolved

- Whether actor storage is zero-filled at allocation, and so the value of a cast point that is read before its first copy.
- The order of the client controller update (`0x088EE0`) relative to the client world update, and the condition under which client world byte `+0xB4` is zero, so that client behaviours run.
- The origin of the controller's direction vector and of its points at `+0x140` and `+0x158`; the internals of ray routine `0x1A35D0` and of the controller's actor scan (`0x098690–0x098D09`); and how L is used after §1.2.
- The internals of the matrix-vector helper `0x0021F0` used for airship riders, beyond the rotation and the w divide named in §2.3. Also the airship's own smoothing and the capture of its pre-update position and heading.
- Writers that do not address the fields through a register plus a displacement, such as bulk copies of whole actor objects. The listing search could not see these.
- The gameplay meaning of the fields, action codes and flags named here.
- Behaviour with exceptional arithmetic, overflow and invalid pointers.
