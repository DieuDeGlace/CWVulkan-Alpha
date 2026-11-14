Status: FINAL / STABLE / HELD. Full author readback completed.

# Alpha world-object list (world +0x0C): producers, removal, the voxel collision tests in the creature movement sub-steps, per-frame update and presentation

## Scope and evidence

This contract covers the world's object list at world `+0x0C`, whose members creatures collide with voxel by voxel during the movement sub-steps of the world creature update. The held contract `alpha-beam-height-leftovers.md` was read only to locate the sub-step loop and for its terms; every statement below was checked against the original images. Where this contract and that one disagree, this one was re-derived from the instructions (see 3.2, the distance filter).

Supported images, both 32-bit with preferred base `0x00400000`. Hashes were recomputed before use:

- Client `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
- Server `binary/Server.exe`, SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`.

All addresses are RVAs (virtual address minus `0x00400000`). Evidence: the instruction listings `decompiled/client/assembly.txt` and `decompiled/server/assembly.txt`, the call lists `decompiled/<image>/calls.csv`, the function range, symbol and string tables beside them, and the image bytes (constants, section layout, jump tables, and one client region that the listing leaves undecoded, the network receive handler at `0x06BFB0`–`0x06C296`, which was decoded by hand from the image). Unless stated otherwise, client and server code agree; Section 9 lists the comparisons. The isolated Infinite-Cubes reconstruction was consulted for leads only. A statement that rests on it alone is marked "(reconstruction lead)".

### Terms

The held contract's terms are used: subject S (the creature being moved), position `+0x10` (three signed 64-bit values, 65536 units per world unit), size triplet `+0x80` (three binary32), step (the world update's first argument, signed 32-bit milliseconds, stored at `0x20C54F`), **T(x)** (binary32 x times 65536.0 in binary32, then converted to signed 64-bit by the runtime conversion `0x28D946`, truncating toward zero), **F(v)** (signed 64-bit v loaded to the x87 unit, stored once to binary32 under the ambient rounding control, then multiplied in binary32 by 2⁻¹⁶; helper `0x02C4A0` per component), and **block index** (helper `0x02F040` per component: the signed 64-bit quotient of v by 65536, truncated toward zero, minus one when v is negative). These are added:

- **L**: the container at world `+0x0C`.
- **O**: one object held by L.
- **Q(v)**: the binary32 squared length of a binary32 triplet v, formed as ((v.x × v.x) + (v.y × v.y)) + (v.z × v.z) with every product and sum rounded to binary32 (helper `0x024860`; the result passes through an x87 load and store that does not change it). It is a squared length; no square root is taken.
- **Model**: entry `0xA03` of the world's model table at world `+0x1C` (fetched by `0x0120C0`, which returns null for a negative index or an index not below the table's element count). The model is a voxel grid with signed 32-bit sizes at model `+0x44` (x), `+0x48` (y) and `+0x4C` (z) and a voxel array pointer at `+0x30`.
- **hx, hy, hz**: the model sizes each converted to binary32 (round to nearest) and multiplied in binary32 by 0.5.
- **Player**: a creature whose byte `+0x60` is 0 (Section 2.3).
- **Unconnected world**: world byte `+0xB4` is 0 (Section 2.6).

## 1. The container and the object record

### 1.1 The container

L is an ordered map from a signed 64-bit key to a pointer to an object. Iteration is in ascending key order (successor routine `0x1C3EA0`); keys compare as signed 64-bit values (high halves signed, low halves unsigned). The routines that touch L in both images are the shared map routines also used for the world creature map at world `+0x04`: first element `0x06F440`, end `0x20A9F0`, iterator inequality `0x0688D0`, element access `0x068C90` (the key is the first 8 bytes of the element, the object pointer the next 4), find `0x077D90`, insert-or-find `0x068AD0` (inserts a null pointer for an absent key and returns its slot), erase `0x03EDE0`.

### 1.2 The object record

An object is 160 (`0xA0`) bytes. The first 120 (`0x78`) bytes form a **state record**, the unit that is copied by the state copy routine `0x068790` (server `0x023350`) and that the client receives from the network (2.5). Fields, as written and read by the routines of this contract:

| Offset | Width | Content |
|---|---|---|
| `+0x00` | signed 64-bit | Identifier; it is the key of O in L. |
| `+0x08` | 8-bit | Copied with the state; no reader was found in the routines of this contract. |
| `+0x10` | 3 × signed 64-bit | Target position. |
| `+0x28` | 3 × binary32 | Velocity, world units per second (6.2). |
| `+0x34` | binary32 | Target yaw, degrees. |
| `+0x38` | 3 × signed 64-bit | Home position. |
| `+0x50` | binary32 | Heading, degrees. |
| `+0x58` | 3 × signed 64-bit | Destination. |
| `+0x70` | 8-bit | Course state, 0 to 3. |
| `+0x78` | 8 bytes | Ordered set of signed 64-bit keys (head pointer, count): creature identifiers inserted by the collision tests (3.2, 4) and pruned by 6.3. |
| `+0x80` | 3 × signed 64-bit | Current position: the pose used for collision and drawing. |
| `+0x98` | binary32 | Current yaw, degrees: the pose used for collision and drawing. |
| `+0x9C` | 32-bit | Zeroed and copied; no other access found. |

The state copy `0x068790` copies `+0x00`–`+0x07`, the byte `+0x08`, `+0x10`–`+0x53`, `+0x58`–`+0x6F` and the byte `+0x70`. It does not copy `+0x09`–`+0x0F`, `+0x54`–`+0x57` or `+0x71`–`+0x77`.

**Construction** (`0x066510`, which first runs `0x066590`): every field of `+0x00`–`+0x6F` is 0 (the bytes it writes: `+0x00`–`+0x08`, `+0x10`–`+0x53`, `+0x58`–`+0x6F`), the state byte `+0x70` is **1**, the set `+0x78` is empty, and `+0x80`–`+0x9F` are 0. A constructed object therefore has current position (0, 0, 0) and current yaw +0.0 until a producer writes them.

**Copy construction** (`0x208FF0`, server `0x12E9E0`): state copy of the source, a copy of the source's set `+0x78`, and a raw copy of `+0x80`–`+0x9F`.

## 2. Producers and removal

There are three producers. Templates are built during zone generation and kept in the zone (2.1, 2.2); L receives copies of templates in an unconnected world (2.4) or, on the client, objects built from received records (2.5). Removal is described in 2.4 and 2.5.

### 2.1 Template generation (zone generation)

**Reach.** The settlement layout routine `0x1AF170` (server: the same code at −`0xCC890`) is called by the zone generator `0x1E4850` at `0x1E9672`. The generator is called by `0x06A8A0` at `0x06ACD1`. For each layout plot whose kind (plot entry `+0x0C`) is 4 (`0x1BA636`), the routine places model `0x887` (the image loads `landingplace.cub` for it) through `0x1F0CE0`, which makes random draws of its own (Section 8). Then it examines the eight region records around the zone (`0x1BA705`–`0x1BA781`): for i and j each from −1 to 1, i outer, excluding (0, 0), the record at region coordinates (zone x ÷ 64 + i, zone y ÷ 64 + j) is fetched by `0x07FAA0` (the divisions truncate toward zero), and it counts when it exists and its signed 32-bit `+0x18` is positive. When none counts, no template is made (`0x1BA786`–`0x1BA793`). The counted records are not used further.

**Fields.** Otherwise a 160-byte object is allocated and constructed (`0x1BA799`–`0x1BA7C3`), then, with r the plot's signed 32-bit rotation (plot entry `+0x14`), H the plot's top height (plot entry `+0x04`), A half the plot size (computed at `0x1B02F4`–`0x1B02FF`, signed halving), and Px, Py the plot's column offsets within the zone (the values the routine adds for the plot's x and y columns; their derivation in the layout is outside this contract):

1. Heading `+0x50` = binary32 of the 32-bit integer r × 90 (`0x1BA7DD`–`0x1BA7F1`).
2. Angle a = binary32 of (binary64 (r + 2) × π) × 0.5, with π the binary64 `0x400921FB54442D18` (`0x1BA7F6`–`0x1BA82B`).
3. Home `+0x38` (`0x1BA838`–`0x1BA8DE`). The integer column Y = truncation toward zero of the binary32 sum (binary32 sin a × 19.0) + binary32 (zone y × 256 + A + Py); X = the same with cos a and (zone x × 256 + A + Px); Z = H + 66. sin and cos are the runtime's precise binary64 routines applied to the widened binary32 angle, each result narrowed to binary32; sin is evaluated before cos. The integers are 32-bit, and each becomes fixed point by a shift left of 16 (`0x1A6A70`).
4. h = binary32 of ((binary64 of the negated heading) × π) ÷ 180.0 (binary64 180.0); then the offset triplet (binary32 sin h × −100.0, binary32 cos h × −100.0, 100.0), with the cosine evaluated before the sine and −100.0 the binary32 at image `0x31DCA8`, is converted by T per component and added (64-bit) to home; the sum is the target position `+0x10` (`0x1BA8E3`–`0x1BA9B5`).
5. Target yaw `+0x34` = heading `+0x50` (raw copy, `0x1BA9CE`).
6. Destination `+0x58` = home + (T(50.0), T(50.0), T(20.0)) (`0x1BA9C0`–`0x1BAA16`).
7. The identifier `+0x00` stays 0, the state byte stays 1, and current position and yaw stay 0 (construction).

**Companion spawn record** (`0x1BAA1B`–`0x1BAB4A`, bounded). A 4336-byte (`0x10F0`) spawn record is built (constructor `0x1ADB20`) with `+0x28` = 3, byte `+0x30` = `0x88`, `+0x10` = the template's target position, `+0x2C` = 4, `+0x34` = the 32-bit value at `+0x24` of a record the layout routine holds (not traced), binary32 `+0x08` = 512.0, binary32 `+0x54` = the template's target yaw, a pointer at `+0x109C` to a new 12-byte behaviour object whose list holds one new element, and a 64-bit key at `+0x10E0` equal to K(zone x, zone y, n) where n is the zone's template count before this template is added (K is defined below). The spawn record is appended to the zone's list at zone `+0x18`, then the template pointer to the zone's template list at zone `+0x24` (`0x1BAB47`, `0x1BAB56`). The meaning of these spawn fields was not traced.

**Identifier assignment** (`0x1EC5EE`–`0x1EC64B`, in the zone generator after the layout; server at −`0xCC220`). For each index i of the zone's template list in order, a template whose identifier is 0 receives K(first generator argument, second generator argument, i). K(a, b, c) (`0x1C0120`) sign-extends its three 32-bit arguments to 64 bits and forms ((b shifted left by 16) + a) shifted left by 8, plus c, all wrapping in 64 bits. With the generator's arguments being the zone coordinates in the order used for the region search above, the template's identifier equals its companion spawn record's key.

**Zone coordinates.** "Zone x" and "zone y" are the zone's two signed 32-bit coordinates at zone `+0x60` and `+0x64` (fetched by index 0 and 1 at `0x1B02D5`–`0x1B02EE`).

### 2.2 Template persistence

Templates live in the zone's list `+0x24` and are not modified by any routine of this contract; L receives copies. No save or load form of the template list was found (the search was not exhaustive; Section 10). The reconstruction likewise shows no save form (reconstruction lead).

### 2.3 Players and player zones (inputs of 2.4)

At the top of the world creature update (`0x20CA3D`–`0x20CB15`), each creature reaching `0x20CA3D` whose byte `+0x60` is 0 is appended to a local **player list**, in creature-map order. For each such creature, with bx and by its position x and y each divided by 65536 as signed 64-bit values (truncating toward zero, helper `0x02F570`, low 32 bits kept) and then by 256 (truncating toward zero), every loaded zone (zone lookup `0x034D10`) at zone coordinates (bx + i, by + j), for i and j from −1 to 1, is inserted into a local **player-zone set** (an ordered set of zone pointers). Because that set is ordered by pointer value, the zone order in 2.4 depends on memory addresses. It cannot change the resulting map, because identifiers from different zones differ.

### 2.4 Copies into L, and removal, in an unconnected world

**Removal** (`0x20F724`–`0x20F928`). It lies on the branch taken at `0x20DDEB` when world byte `+0xB4` is 0; the other branch resumes at `0x20F942`. It runs whether or not the world is named. For each entry of L in key order whose pointer is not null: if no player p satisfies 262144.0 > Q(F(O `+0x10` − p `+0x10`)) (binary32 `0x48800000` at image `0x303A1C`; an ordered strict comparison, the subtraction 64-bit per component), the entry is scheduled. Entries with a null pointer are kept. Then, in scheduling order, each scheduled object is destroyed (deleting destructor `0x06D370`, which releases the set `+0x78` and frees the object) and its entry erased (`0x03EDE0`). With no players at all, every non-null entry is removed.

**Copy producer** (`0x20F942`–`0x20FB48`). It runs only when world byte `+0xB4` is 0 (`0x20F948`) and the world name string at world `+0x94` is not empty (`0x077220`, which tests the string's length word at world `+0xA4`). For each zone of the player-zone set, and for each index of that zone's template list in order, with X the template:

1. If L already holds the key X `+0x00` (find `0x077D90`), X is skipped (`0x20F9FE`–`0x20FA0C`).
2. Otherwise the player list is scanned in order. At the first player p with 262144.0 > Q(F(X `+0x10` − p `+0x10`)) (`0x20FA89`–`0x20FA98`), a 160-byte object is allocated (`0x20FAD2`), copy-constructed from X (`0x20FAF5`), and stored by insert-or-find under key X `+0x00` (`0x20FB0E`–`0x20FB13`). Then **current position `+0x80` = target position `+0x10`** (`0x20FB1F`) and **current yaw `+0x98` = target yaw `+0x34`** (`0x20FB24`–`0x20FB27`), and the player scan stops.

Removal precedes the copy producer in the same update. An object that drifts beyond 512 world units of every player is therefore removed and, when its zone is still a player zone and a player is within 512 units of the template's original target position, re-created from the template in the same update: its state, set and pose are reset to the template's.

### 2.5 Client network receive (connected client)

**Dispatch.** The client network thread `0x06B740` reads a 32-bit message identifier. Identifiers 0 to 15 are mapped by the byte table at image `0x06D1A4` to an index into the jump table at `0x06D180` (`0x06B8A7`–`0x06B8B7`). **Identifier 3** selects the handler at `0x06BFB0`.

**Wire form.** One signed 32-bit count, then (only when the count is positive) count × 120-byte state records, read by repeated socket reads until complete (`0x06BFB0`–`0x06C03B`; the records are read with the receive-all helper `0x086B60`). A read failure marks the connection lost. When the count is 0 or less, nothing more is read and **L is not changed**.

**Application** (`0x06C06B`–`0x06C22B`), between calls of the lock pair `0x201CB0` and `0x201E90` on the controller's world at controller `+0x2E4` (a region lock, reconstruction lead):

1. The identifier `+0x00` of every object in L is set to 0, in key order (no null test).
2. For each received record in order: the slot for its identifier is obtained by insert-or-find. When the slot is null, a constructed object is allocated and stored there (a second insert-or-find of the same key). The record's state fields (the state copy of 1.2) are copied into the object. **The set `+0x78`, the current position `+0x80` and the current yaw `+0x98` are not written**, so a newly received object starts at current position (0, 0, 0) and yaw +0.0 and only converges toward its target through the per-frame update (6.2). A later record with the same identifier overwrites an earlier one.
3. Every entry whose object identifier is now 0 (both halves; no null test) is collected in key order, i.e. every object absent from this message, plus any object received with identifier 0. For each collected entry whose pointer is not null, its set is released, the object freed, and the entry erased (`0x03EDE0`).

### 2.6 Which play produces objects

- **World byte `+0xB4`** is written by the world constructor `0x18EB00` from its argument (`0x18EC0C`). On the client it is set to 1 by the connect routine `0x06FC50` before it opens its socket (`0x06FCA7`, controller `+0x398` = world `+0xB4`). It is set back to 0 at `0x06FE47`, `0x06FEE9`, `0x07018C`, `0x070745` and `0x070931` within that routine, and at `0x06A869`, `0x071A5B` and `0x071AAD`; the conditions of these stores were not traced.
- **Unconnected client (single-player).** The client's own world update performs 2.4. Objects exist when the world is named and a player is within 512 world units of a template of a loaded zone near a player. That the single-player world is named is a reconstruction lead, not traced here.
- **Server.** The server world update performs 2.4 identically. The server's world flag and name were not traced (Section 10).
- **Connected client.** The client world update skips 2.4. L changes only through 2.5. The original server's send thread `0x023DD0` collects state records of objects into a local list (state copy call at `0x0243A9`, elements of 120 bytes) but **never sends it**: the list is only freed (`0x025151`, `0x0251B1`, `0x025275`, `0x0253B2`–`0x0253C4`). No immediate value 3 is stored as a send header in that thread (headers taken from registers there were not traced). The only immediate send of the value 3 in the server is a handshake refusal in `0x0254A0` (`0x02576F`–`0x025779`), sent on a connection that is then closed and read by the client's connect routine, not by the receive thread. A client connected to the original server therefore receives no object records: identifier-3 messages arise only from peer data.
- Whether L is cleared when a client connects, disconnects or changes world was not found (Section 10). Objects created while unconnected may therefore persist into a connected session; this was not traced.

## 3. The per-axis collision test (client `0x21BCA3`–`0x21C15B`)

### 3.1 Placement in the sub-step

Per the held contract (section 1.2), each sub-step runs the three axes x, y, z in order and, per axis, the terrain test, then the static-entity test, then this test. Confirmed here:

- This test runs only when no earlier test of the same axis has recorded a collision. The per-axis collision flag is cleared at `0x21B52D`, and at `0x21BCA3` a set flag skips this test.
- A local **base height** b (signed 64-bit) is set to 0 at the start of every sub-step (`0x21B4C2`), not per axis.
- A local **object-landing flag** is cleared once per creature update, before the sub-steps (`0x21B464`).

### 3.2 Per object, in key order (`0x21BCB0`–`0x21C138`)

For each entry of L:

1. **Distance filter.** d = F(S position `+0x10` − O `+0x80`) per component (64-bit subtraction, wrapping). O is skipped when Q(d) > 4096.0 (binary32 `0x45800000` at image `0x302B80`; the comparison is ordered, so a NaN would not skip, but Q of F values cannot be NaN). Because Q is a squared length, **the filter radius is 64 world units**, not 4096. The held beam contract's wording ("within 4096.0 world units … the binary32 length") is corrected by this section.
2. **Null tests.** The object pointer is tested for null **after** the filter has already read O `+0x80`; a null pointer would fault before the test. The model is then fetched (index `0xA03`); when it is null, O is skipped.
3. **Model frame.** With θ = binary32 of (the sign-flipped current yaw −O `+0x98`) × 0.017453292 (binary32 `0x3C8EFA35`, image `0x2FCDA4`), c = binary32 of the runtime's precise cosine of θ widened to binary64, and s likewise with sine (cosine evaluated first; rotation helper `0x024610`), the subject centre in the model frame is P:
   - P.x = ((c × d.x) + (−s × d.y)) + hx;
   - P.y = ((s × d.x) + (c × d.y)) + hy;
   - P.z = d.z + hz;

   with every product and sum rounded to binary32. (The transform is built as a 4 × 4 binary32 matrix: identity, then the half-size translation, then the rotation by θ, then the translation by d, helpers `0x023E70`, `0x024A60`, `0x024610`, `0x024990`, applied to the origin by `0x0248A0`. The additional terms it adds are exact zeros and its final division is by exactly 1.0, so they change at most the sign of a zero result.) There is no scale: one voxel is one world unit.
4. **Box.** e = 0.5 × S size `+0x80` per component (binary32). lo = block index of T(P − e) and hi = block index of T(P + e), per component, the subtraction and addition in binary32.
5. **Voxel scan.** For x from lo.x to hi.x, then y from lo.y to hi.y, then z from lo.z to hi.z (inclusive, x outermost), the voxel (x, y, z) is fetched by `0x0A2870` and is **solid** when any of its three bytes is nonzero (comparison `0x028940` against (0, 0, 0)). For coordinates inside the grid (each ≥ 0 and below the matching size) the voxel is the 3-byte entry at offset 3 × (((z × size y) + y) × size x + x) of the voxel array. For any coordinate outside the grid, the accessor returns the image's **shared out-of-grid cell** (Section 5).
6. **On every solid voxel**, with no early exit from the scan or from the object loop:
   - the axis collision flag is set;
   - on the z axis, the object-landing flag is set (`0x21C024`–`0x21C049`);
   - b = O `+0x80` z − T(hz) (`0x21C04F`–`0x21C090`): the 64-bit current z minus T of the model's binary32 half height;
   - the subject's identifier, the signed 64-bit value at creature `+0x08`, is inserted into O's set `+0x78` (unique insertion `0x07F000` / `0x053DE0`; a key already present is left unchanged).

   Consequently b ends as the base of the **last object in key order with any solid voxel** in the box, and repeated hits only repeat the same assignment and insertion.

### 3.3 After the object loop

When the flag is still clear (`0x21C13E`–`0x21C146`), the axis move is kept and the next axis follows. Otherwise the collision handling of the held contract runs (`0x21C14C`–`0x21C15B`): on the z axis the move is undone and the vertical state reset; on a horizontal axis the step-up attempt of Section 4 runs when the step-up permission is still set and bit `0x0100` of creature word `+0x7E` is clear, else the move is undone. This test never clears the step-up permission (only the static-entity test does).

### 3.4 The object-landing flag

After the sub-step loop and the step-down snap, bit `0x01` of creature `+0x5C` is set when either the static-entity landing flag or the object-landing flag is set (`0x21D5B0`–`0x21D5C8`). The object-landing flag is set by a z-axis object hit regardless of the vertical velocity's sign; unlike the static-entity flag, it has no velocity condition.

## 4. The step-up attempt's object test and the base height (client `0x21C49E`–`0x21CC5E`)

1. The attempt clears the collision flag (`0x21C4C3`), then tests the terrain in the raised box as described in the held contract (`0x21C4AD`–`0x21C6E1`). A terrain hit fails the attempt at once (`0x21CC25`).
2. **Objects** (`0x21C6E7`–`0x21CAFD`), in key order, while the flag is clear (checked before each object, `0x21C720`):
   - The distance filter, null tests and model frame are exactly those of 3.2 steps 1–3, with S's position after the axis move.
   - The raised centre P′ is P with z replaced by binary32 (1.01 + P.z), where 1.01 is binary32 `0x3F8147AE`, added in the model frame (the transform applied to (0, 0, 1.01); x and y are unchanged). This is not the same rounding as the terrain test's fixed-point raise by T(1.01).
   - The box is lo = block index of T(P′ − e), hi = block index of T(P′ + e), and the voxel scan order and solidity are those of 3.2 step 5.
   - **At the first solid voxel**, the subject's identifier is inserted into O's set `+0x78`, the flag is set, and both the voxel scan and the object loop end (`0x21CA8A`–`0x21CAB8`).
   - This test does **not** change b or the object-landing flag.
3. A set flag undoes the axis move (`0x21CC31`–`0x21CC5E`). Otherwise the step-up snap of the held contract runs (`0x21CB09`–`0x21CC23`) with the base height b of 3.2 step 6: b is 0 unless an object hit occurred on an earlier or the current axis **of the same sub-step**, in which case it is the current z of the last hit object minus T(hz).

**Random draws:** none in Sections 3 and 4. The only draw in the sub-step span `0x21B4C0`–`0x21CCA7` is at `0x21C1A0`, in the z-axis collision handling (landing sound, outside this contract).

## 5. The model and the shared out-of-grid cell

- **Model source.** The model loader `0x0EE230` loads the file name at image `0x311EBC` (`airship.cub`) into model index `0xA03` (`0x11BAA7`–`0x11BAEC`). It then visits every voxel, x outermost, then y, then z, and replaces each voxel equal to (255, 0, 0) by (0, 0, 0), appending its integer coordinates to the model's position list 0 at model `+0x28` (`0x11BAF1`–`0x11BC40`). Pure-red voxels therefore never collide. The server loads the same file name (server image `0x1694DC`, used at `0x05EC77`) into index `0xA03` (`0x05EC91`).
- **Shared out-of-grid cell.** For any coordinate outside the grid, the voxel accessor returns the address of one 3-byte cell that all models share: client image `0x36B340` (beyond the raw data of `.data`, hence zero at start), server image `0x183DFC` (zero in the image). The client writes this cell directly in several places, whenever those routines store a voxel at an out-of-grid coordinate: `0x1FAE40` (store at `0x1FAFF2`), `0x2024D0` (`0x203146`–`0x203158`, `0x2031AD`–`0x2031BF`, `0x203211`–`0x203220`, values (255, 255, 255), (220, 220, 220) and (200, 200, 200)), `0x203A00` (`0x204633`–`0x20464E`, `0x204B28`–`0x204B3E`), and `0x187F70` (`0x188182`–`0x18818E`, reachable only when a model size is 0 or less). **After any such store of a nonzero value, every out-of-grid coordinate tests solid** in Sections 3 and 4. Within 64 world units of an object, a subject whose box extends beyond the model grid would then collide. Among the server's immediate references to its cell, none is a store. Writes through the pointer that the accessor returns, and whether the client routines above perform out-of-grid stores in practice, were not traced (Section 10).

## 6. Per-frame update of L (client `0x21FE90`–`0x220A70`, bounded)

After the creature loop of the world update, L is iterated twice in key order. The first pass (`0x21FEA0`–`0x21FED0`) does nothing per entry. The second pass (`0x21FED2`–`0x220A70`) skips null pointers (`0x21FF12`). For each other object it saves the current position and yaw as P₀ and Y₀ (`0x21FF1A`–`0x21FF40`), then runs the following.

### 6.1 Course steering (unconnected world only, bounded)

When world byte `+0xB4` is 0 (`0x21FF39`), the block `0x21FF4E`–`0x22063A` steers by the state byte `+0x70`: the first test is state 1 (`0x21FF51`). It writes velocity `+0x28`, target yaw `+0x34` and the state: 2 at `0x21FFCA`, 1 at `0x220164`, 3 at `0x2202C0`, 0 at `0x220381`. The binary32 constants it uses are 0.001, 0.005, 1.0, −1.0, 10.0, −10.0, 20.0, 30.0, 50.0, −50.0, 100.0 and 900.0, together with binary64 π and 180.0. The reconstruction describes the four courses as circling behind home, returning home, circling ahead of home and flying to the destination at a held height above the ground (reconstruction lead). The per-branch arithmetic was not audited.

### 6.2 Integration and pose follow (every world)

1. **Target position.** secs = binary32 of the step, times 0.001 (binary32 `0x3A83126F`, image `0x2FD870`), computed once per update (`0x20C83D`–`0x20C862`). Then `+0x10` += T(secs × velocity) per component, the product binary32 and the addition 64-bit (`0x22063F`–`0x22067A`).
2. **Follow factor.** f starts at 0.0 and approaches 1.0 through the repeated-approach helper `0x011D40` with rate 0.01 (binary32 `0x3C23D70A`) and repeat count = step. Each repetition computes f = ((1.0 − f) × 0.01) + f, rounded to binary32 at each operation; a step of 0 or less leaves f = 0.0.
3. **Current position.** `+0x80` += ((`+0x10` − `+0x80`) × T(f)) ÷ 65536 per component (`0x2206BC`–`0x2206F8`). The difference is 64-bit; the product is the low 64 bits of a 64 × 64-bit multiplication; the division is signed and truncates toward zero.
4. **Current yaw.** An angle-interpolation helper `0x023F70` (uses cosine, sine and arcsine; not audited) is evaluated with (current yaw, target yaw `+0x34`, 1.0), and `+0x98` approaches its result through `0x011D40` with rate 0.01 and repeat count = step (`0x2206FD`–`0x220747`).

### 6.3 Members of the set +0x78 (every world)

For each key k of O's set in ascending order (`0x220762`–`0x220998`), C is the world creature with identifier k (lookup `0x02F000` in the creature map at world `+0x04`):

- When C is absent, or Q(F(C `+0x10` − O `+0x10`)) > 4096.0, k is scheduled for removal. Note that this uses the object's **target** position.
- Otherwise, with Δ = binary32 (new `+0x98` − Y₀) and R the rotation helper applied to an identity matrix with angle Δ (same conversion as 3.2 step 3, with Δ in place of −yaw):
  - the 3 × signed 64-bit triplet at C `+0x1350` becomes new `+0x80` + T(R applied to F(C `+0x1350` − P₀));
  - the binary32 at C `+0x1374` + 8 becomes (binary32 (new `+0x98` − Y₀)) + itself;
  - C position `+0x10` becomes new `+0x80` + T(R applied to F(C `+0x10` − P₀));
  - the binary32 at C `+0x28` + 8 is increased in the same way.

  R applied to (x, y, z) gives ((c × x) + (−s × y), (s × x) + (c × y), z), with c, s the binary32 cosine and sine of the converted Δ and binary32 rounding of each operation (the remaining matrix terms are exact zeros, as in 3.2).
- After the pass, the scheduled keys are erased from the set (`0x2209D0`–`0x220A23`).

The set is therefore the list of creatures carried with the object. A creature enters it only through the collision tests of Sections 3 and 4, and leaves it when it is more than 64 world units from the object's target position or no longer exists.

### 6.4 Other consumers (bounded, not audited)

- **Creature placement** (`0x21F6DA`–`0x21F962`, unconnected world only): a creature whose signed 64-bit `+0x1D30` is nonzero and is a key of L is positioned relative to that object using model `0xA03`, its position list and the object's current pose (reconstruction lead: the companion spawn of 2.1 is kept on its object).
- **Unstuck routine** `0x20AB30` (server `0x1308B0`), part from `0x20B03F`: reads L, the model and each object's current pose to move a creature out of overlapped model voxels (reconstruction lead for the details).

**Random draws:** none in Section 6. No draw instruction lies in `0x21FE90`–`0x220A80`, and no recorded call path from the helpers used there reaches `rand` (call graph searched to depth 10; calls through virtual tables are not recorded and were not followed).

## 7. Drawing (client only, bounded)

The client renderer `0x0AC260` iterates L in key order at `0x0B35D4`–`0x0B3801`. It skips null pointers. It skips an object when the visibility helper `0x07F760` rejects the object's **target** position `+0x10` with radius 64.0. It fetches model `0xA03` from the controller's world model table (controller `+0x300`) and skips the object when the model is null. The model is placed by an identity matrix translated by (F(`+0x80` x + controller 64-bit `+0x1D8`), F(`+0x80` y + controller 64-bit `+0x1E0`), F(`+0x80` z)), rotated by **+**`+0x98` (rotation helper `0x024610`), and translated by (binary32 size × −0.5) per axis (binary32 `0xBF000000`, image `0x2FCE8C`). It is drawn by `0x0E6DF0`. The controller values `+0x1D8` and `+0x1E0` (presumably the render origin) were not traced. Drawing therefore uses the current pose with the inverse of the collision rotation, and both centre the model grid on the current position. The server has no drawing.

## 8. Random draws

- **Sections 2.3–2.6, 3, 4, 6 and 7:** no draws (direct instructions searched; nested helpers checked through the recorded call graph, Section 6).
- **Section 2.1:** the template and its companion spawn record make no draws (no draw in `0x1BA636`–`0x1BAB64`, and none reachable from its non-placement helpers). The landing-place placement `0x1F0CE0`, called just before the region search, does draw directly. Its draws precede the template, and their number was not counted.
- **Client receive (2.5):** none.

## 9. Client/server agreement

Verified after masking absolute addresses, instruction for instruction:

- World update sites at −`0xDA240`: `0x21B4C0`–`0x21CCA7` (sub-step loop, both tests), `0x20F942`–`0x20FB48` (copy producer), `0x20F724`–`0x20F942` (removal), `0x20C9C0`–`0x20CB30` (player list and zones), `0x21FE90`–`0x220A80` (per-frame update).
- Zone generation: `0x1BA636`–`0x1BAB64` at −`0xCC890`; the identifier loop at −`0xCC220` (server `0x1203E4`, located by byte pattern).
- Helpers of Sections 3–4, compared byte for byte with call offsets masked: `0x024810`, `0x024820`, `0x07B5F0`, `0x023E70`, `0x012280`, `0x02C7A0`, `0x024860`, `0x0248A0`, `0x02C4A0`, `0x0121C0`, `0x0120C0`, `0x012220`, `0x02F040`, `0x051510`, `0x024990`, `0x02C460`, `0x024610`, `0x024A60`, `0x0A2870`, `0x028940` (server `0x002150`, `0x002160`, `0x002170`, `0x001CD0`, `0x0014F0`, `0x002C50`, `0x0021B0`, `0x0021F0`, `0x002550`, `0x001450`, `0x0013F0`, `0x0014B0`, `0x005450`, `0x0E1520`, `0x0022E0`, `0x002510`, `0x002030`, `0x0023B0`, `0x030730`, `0x0079C0`). Six of them (`0x0248A0`, `0x02C4A0`, `0x012220`, `0x02C460`, `0x024610`, `0x0A2870`) differ in one absolute operand each and the others are identical. The server constants are equal: 4096.0 (`0x173CC0`), 0.5 (`0x1586D0`), 2⁻¹⁶ (`0x15869C`), 1.0 (`0x1586DC`), 65536.0 (`0x15878C`), 0.017453292 (`0x1586AC`). The voxel accessor differs only in its out-of-grid cell address.
- Client only: the network receive (2.5), the connect routine's world flag, drawing (7), and the direct stores to the shared out-of-grid cell (5).
- Server only: the send thread's unsent collection (2.6).

## 10. Unresolved

- Whether any routine other than those of Section 2 inserts into or erases from L, including the world destructor and any clearing on world setup, connect or disconnect. The searches covered calls of the shared map routines with a recognisable world operand, plus the hand-decoded receive handler. Inline or otherwise undecoded accesses were not excluded.
- Save or load forms of the zone template list (2.2).
- Whether the single-player world and the server world are named, and the server's world flag, which together decide whether 2.4 runs.
- Derivation of the plot inputs r, H, A, Px and Py of 2.1, and the meaning of the companion spawn fields.
- Whether the client routines of Section 5 perform out-of-grid stores in practice, and writes through the voxel accessor's returned pointer in either image.
- The course-steering arithmetic of 6.1, the angle-interpolation helper `0x023F70`, the creature placement of 6.4, the unstuck routine's object part, and the meanings of creature `+0x1350`, `+0x1374`, `+0x1D30` and object byte `+0x08`.
- The visibility helper `0x07F760` and the render-origin values of Section 7.
- Calls through virtual tables reached from the routines above, which the call graph does not record, were not followed for random draws.
