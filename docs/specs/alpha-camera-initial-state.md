Status: FINAL / STABLE / HELD after complete saved-document readback. Bounded to the constructed values, option sources, located non-frame writers, first-update anchor relation and program-driven front-end targets of the listed client camera members; open items are listed at the end.

# Alpha client camera: constructed state, option defaults, and non-frame writers

This is a bounded, code-free behavioral contract. It establishes what the client's camera members hold when client construction finishes, where the three camera options come from, which routines other than the already contracted per-frame routes store into those members, what the first per-frame update finds in the follow anchor, and in which front-end states the program rather than the mouse drives the angle and distance targets. It does not restate the per-frame smoothing, follow, collision or matrix arithmetic described by the two earlier camera contracts; those were used only to locate members and routines, and every fact below was confirmed against the original listing and image bytes.

## Image, evidence and notation

The supported original is `binary/Cube.exe` under the evidence root `disassembled alpha cube world`: 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf` (recomputed before use), preferred image base 0x00400000. Addresses are RVAs unless marked VA. Evidence files are `decompiled/client/assembly.txt` (called A), `decompiled/client/assembly-functions/00459c40.asm` (called K), `decompiled/client/assembly-functions/00488ee0.asm` (called U), and the image bytes themselves. Immediate constants, the instruction bytes of every store listed in the tables below, the client virtual table, the import names and the read-only constants were read from the image. No decompiler output, runtime observation, reconstruction or implementation material supplied a fact.

`C` is the client object. `W` is the embedded world at C+0x2E4. `A0` is the creature pointer stored at C+0x8006D0. `N` is the signed 32-bit argument of the per-frame update at 0x088EE0. A "widget property" is the tested zero/nonzero 32-bit value reached from a widget pointer through its pointed object at +0x3C, the index at +0x68 and the four-byte array at +0x94; routine 0x07FA10 returns its nonzero test. Widget pointers are named only by their client offsets.

Fixed coordinates are signed 64-bit values with 65,536 raw units per coordinate unit. Angles are binary32 degrees. Bit patterns of binary32 values are written as eight hexadecimal digits.

## Storage is not pre-cleared

The startup routine 0x0C8AE0 obtains the client's 0x1001018 bytes from the imported `MSVCR110.dll` allocation operator (thunk 0x28D668, import slot VA 0x006FC3EC) at 0x0C8FB4–0x0C8FB9 and passes that storage straight to the constructor at 0x0C8FF9. Nothing clears the block in between. A member that construction does not store therefore has no proven value when construction returns. Evidence: A at 0x0C8FA9–0x0C9002 and 0x28D668; image import directory.

## Constructed values

The constructor is 0x059C40. Its receiver is kept in one register from 0x059C72 until 0x05B331; every store below lies in that span and uses that receiver. The base-part initialiser it calls first (0x03B4C0) stores only into C+0x00–C+0x18, C+0x19–C+0x11B and C+0x124–C+0x134 and reaches none of the members below. Evidence: K at 0x059C40–0x059EA6 and 0x05AACA–0x05AB0D; A at 0x03B4C0–0x03B536.

Construction stores the camera members in two groups. The first group runs at 0x059D35–0x059EA6. The second group runs much later, at 0x05AAD3–0x05AB0D, after the front-end screen widgets have been created, and replaces some first-group values. The second group's order is: current distance, target distance, current angle component 0, current angle component 2, then three copies that load each current angle component and store it to the matching target component, in ascending component order.

| Member | Width and type | First group | Second group | Value when construction returns |
| --- | --- | --- | --- | --- |
| +0x140 eye x | signed 64 fixed | 0x0000000027100000 | not stored | 0x0000000027100000, which is 10,000 units |
| +0x148 eye y | signed 64 fixed | 0x0000000027100000 | not stored | 0x0000000027100000, which is 10,000 units |
| +0x150 eye z | signed 64 fixed | 0x00000000003C0000 | not stored | 0x00000000003C0000, which is 60 units |
| +0x158, +0x160, +0x168 follow anchor | three signed 64 fixed | all zero | not stored | all zero |
| +0x1A4 current angle 0 | binary32 | 00000000 | 42F00000 | 42F00000, which is 120 degrees |
| +0x1A8 current angle 1 | binary32 | 00000000 | not stored | 00000000 |
| +0x1AC current angle 2 | binary32 | 00000000 | 00000000 | 00000000 |
| +0x1B0 target angle 0 | binary32 | 00000000 | copy of +0x1A4 | 42F00000 |
| +0x1B4 target angle 1 | binary32 | 00000000 | copy of +0x1A8 | 00000000 |
| +0x1B8 target angle 2 | binary32 | 00000000 | copy of +0x1AC | 00000000 |
| +0x1BC current distance | binary32 | 41800000 (16) | 00000000 | 00000000 |
| +0x1C0 target distance | binary32 | 41800000 (16) | 00000000 | 00000000 |
| +0x1C4 current scale | binary32 | 3F800000 (1) | not stored | 3F800000 |
| +0x1C8 target scale | binary32 | 3F800000 (1) | not stored | 3F800000 |
| +0x1CC distance copy | four bytes | not stored | not stored | not established; see below |
| +0x1D8, +0x1E0 rebasing members | two signed 64 fixed | not stored | not stored | not established; see below |
| +0x1E8 shake amplitude | binary32 | 00000000 | not stored | 00000000 (positive zero) |
| +0x18C, +0x190, +0x194 options | three signed 32 | copied from the option block | not stored | the option block's values; see the option section |

The value 16 placed in both distance members by the first group does not survive construction. The eye value written by construction is not derived from any creature or anchor; it is three immediate constants.

The first group also stores zero to the byte at C+0x1A0 and positive zero to the binary32 at C+0x1D4; the second group is followed at 0x05AB13–0x05AB36 by a store to the binary32 at C+0x1D0. Those three neighbours are not camera members of this contract and are mentioned only so that they are not mistaken for the unwritten members beside them. The mode byte at C+0x8006E4 that selects the wheel route is stored as zero at 0x059FF7.

### Members construction never stores

No instruction in the constructor that uses the client receiver stores to C+0x1CC, C+0x1D8, C+0x1DC, C+0x1E0 or C+0x1E4, and the complete-image search described later found no other routine that stores to them on the client before the per-frame update does. Their first proven stores are the per-frame ones already contracted: C+0x1CC at 0x09CA7A, and C+0x1D8 then C+0x1E0 at 0x09CCF7–0x09CD2C. Because the storage is not pre-cleared, their content before those stores is unresolved.

This matters for one proven read. Inside the per-frame update, at 0x08C434–0x08C486, both pieces of C+0x1D8 and both pieces of C+0x1E0 are read to build a fixed vector that is combined with A0's position. That site executes earlier in the update than the stores at 0x09CCF7–0x09CD2C. On the first update that reaches it, it therefore reads the two rebasing members before any proven store to them; on later updates it reads the values published by the preceding update. The conditions under which the update reaches 0x08C434 were not audited. Evidence: U at 0x08C434–0x08C4AE.

## The three camera options

### Where the client copies come from

The constructor's fifth stack argument is a pointer to a 48-byte block of twelve 32-bit values. At 0x059DD8–0x059E27 the constructor copies all 48 bytes, as six 8-byte moves in ascending order, to C+0x170–C+0x19F. The members of this contract are therefore block offsets +0x1C (C+0x18C), +0x20 (C+0x190) and +0x24 (C+0x194). The startup routine passes the address of a process-wide block at VA 0x0076B1D8 as that argument (0x0C8FD9), so the three source values are the 32-bit values at VA 0x0076B1F4, VA 0x0076B1F8 and VA 0x0076B1FC. Evidence: K at 0x059C7A–0x059E27; A at 0x0C8FC5–0x0C8FF9.

### Built-in defaults

The process-wide block lies beyond the initialised part of the image's data section, so it is zero when the image is loaded. A small stub at 0x2F9F10 passes the block's address to the default routine 0x0CE660; the stub's address appears once in the image, in a read-only pointer table at VA 0x006FC744. That the runtime walks this table before the startup routine runs is the ordinary start-up initialiser mechanism and was not separately traced. Routine 0x0CE660 stores twelve immediates; the three of interest are:

| Option | Block offset | Client member | Default |
| --- | --- | --- | --- |
| Rotation sensitivity | +0x1C | +0x18C | 50 (0x00000032) |
| Angle smoothing | +0x20 | +0x190 | 80 (0x00000050) |
| Vertical direction | +0x24 | +0x194 | 0 |

Evidence: A at 0x0CE660–0x0CE6B5 and 0x2F9F10–0x2F9F15; image scan for the stub address.

### Overwrite by configuration loading

Before constructing the client, the startup routine calls loader 0x0CE6E0 on the process-wide block (0x0C8DAF). Just before that call it stores two results of an imported call into block offsets +0x04 and +0x08; those are not camera options.

The loader opens the text file named `options.cfg` for reading. If the stream is not usable immediately after opening, the loader changes nothing and the built-in defaults remain. Otherwise it repeats the following until the stream stops being usable: it obtains one text token through helper 0x0CE2A0; if the stream is then unusable it stops; otherwise it compares the token against twelve fixed key texts, one after another and without skipping the remaining comparisons after a match. A comparison succeeds only when the token has exactly the key's length and the same bytes. For each successful comparison it extracts one signed 32-bit decimal integer from the stream, using the imported `MSVCP110.dll` narrow input-stream extraction operator for a signed integer (import slot VA 0x006FC1C4), directly into the matching block member. The keys relevant here are:

| Key text in the image | Key length | Destination block offset | Client member |
| --- | --- | --- | --- |
| `cameraSpeed` (VA 0x007032B4) | 11 | +0x1C | +0x18C |
| `cameraSmoothness` (VA 0x007032C0) | 16 | +0x20 | +0x190 |
| `invertY` (VA 0x007032E0) | 7 | +0x24 | +0x194 |

The loader applies no range check, clamp or normalisation to an extracted value, and neither the startup routine nor the constructor applies one before the copy into the client. A file can therefore place any value the extraction operator produces, including negative values and values outside the menu's bounds below, into the three client members. The token helper's delimiter rules, the extraction operator's behaviour on malformed text, and whether a failed extraction leaves or alters the destination are runtime-library dependencies not closed here. Evidence: A at 0x0CE6E0–0x0CEF76 and 0x0C8D92–0x0C8DAF; image strings and import directory.

### Later replacement of the client's option block

Client routine 0x06F390 replaces the entire 48-byte block at C+0x170–C+0x19F from a block supplied by its caller, again as six 8-byte moves in ascending order, between a world lock call (0x201CB0) and its matching release (0x201E90). It then calls 0x0CEF80 on the client's block and forwards block offset +0x18, converted to binary32 and multiplied by binary32 0.01 (bits 3C23D70A), to an audio object. It performs no camera arithmetic and stores no other camera member. Its two located callers are 0x0D4470 and 0x0D4650; each passes the address of a 48-byte block held inside an options-screen object at that object's +0x1D4 and takes the client pointer from that object's +0x16C. Evidence: A at 0x06F390–0x06F425, 0x0D4470–0x0D4482, 0x0D4650–0x0D4686.

Separately, the outer frame routine 0x0C85F0 compares four values of the client block (offsets +0x00, +0x04, +0x08, +0x0C) with the process-wide block after each update, and on any difference copies all 48 bytes from the client block back to the process-wide block. That direction never changes a client member. Evidence: A at 0x0C8636–0x0C86D6.

### Stored range enforced by the options screen

The options-screen object adjusts its own block through small routines whose addresses appear as immediates inside the options-screen construction code. Each operates only on that object's block; nothing reaches the client until 0x06F390 is called.

| Option | Routine | Adjustment | Bound applied after the adjustment |
| --- | --- | --- | --- |
| Rotation sensitivity | 0x0D44D0 | subtract 5 | a signed result below 10 becomes 10 |
| Rotation sensitivity | 0x0D44F0 | add 5 | a signed result above 100 becomes 100 |
| Angle smoothing | 0x0D4490 | subtract 5 | a negative result becomes 0 |
| Angle smoothing | 0x0D44B0 | add 5 | a signed result above 100 becomes 100 |
| Vertical direction | 0x0D45D0 | toggle | becomes 1 when it was 0, otherwise becomes 0 |

Through this screen the sensitivity is thus kept in 10 to 100 in steps of 5 and the smoothing in 0 to 100 in steps of 5, relative to whatever value the block held when the screen obtained it; a value that was off the 5-step grid or outside the bounds stays so until an adjustment moves it. How the options-screen block is seeded, which user action invokes each routine, and what triggers the two callers of 0x06F390 were not audited. Evidence: A at 0x0D4490–0x0D450D, 0x0D45D0–0x0D45E1; image scan locating each routine address inside 0x0CF3C0's code.

### Units

These statements restate, from the original instructions, only how each stored integer is consumed; the surrounding arithmetic belongs to the earlier contracts.

The sensitivity is a signed 32-bit integer. The mouse callback converts it to binary32 and multiplies it by binary32 0.005 (bits 3BA3D70A, VA 0x006FCD9C); that product, multiplied by a mouse displacement, is an angle change in degrees. With the default 50 the factor is 0.25 degree per unit of displacement before rounding. Evidence: A at 0x07EE84–0x07EF24.

The smoothing is a signed 32-bit integer. The per-frame update converts it to binary32 and multiplies it by binary32 0.01 (bits 3C23D70A, VA 0x006FCDA0); the product is used as the exponent applied to the base 0.005 (bits 3BA3D70A) to obtain the per-count angle interpolation coefficient. It is dimensionless: 0 gives exponent 0 and 100 gives exponent 1. Evidence: U at 0x0965A2–0x0965C5.

The vertical-direction value is tested only for zero versus nonzero. Zero adds the vertical mouse product to target angle 0; any nonzero value subtracts it. Evidence: A at 0x07EE84–0x07EEB5.

## Writers outside the contracted per-frame routes

The already contracted routes are the mouse callback 0x07EA00, the wheel callback 0x07EF40, the smoothing calls at 0x0965A2–0x09662C, the late follow, collision and matrix phase at 0x09C2E6–0x09CEDB and the amplitude decay that ends it. Every other located store to the listed members is described here.

### Three front-end button handlers

Three client routines are passed as immediates, together with the client pointer and the code 3, when buttons are attached during construction: 0x083E70 for a button stored at C+0x800990 whose parent is the widget at C+0x800888 (K at 0x0656A6), 0x084170 for a button stored at C+0x8009E8 whose parent is the widget at C+0x80088C (K at 0x065842), and 0x082530 for a button stored at C+0x8009B4 whose parent is the widget at C+0x800890 (K at 0x05CC81). No direct call to any of the three exists in the listing. The widget library's rule for when such a registered routine is invoked was not audited; the natural reading is activation of the button.

Each routine performs the same eight stores in the same order, differing only in one constant:

1. target distance (+0x1C0) becomes 5 (bits 40A00000);
2. current distance (+0x1BC) becomes 5 (bits 40A00000);
3. target angle 0 (+0x1B0) becomes 90 (bits 42B40000);
4. target angle 1 (+0x1B4) becomes positive zero;
5. target angle 2 (+0x1B8) becomes the routine's constant;
6. current angle 0 (+0x1A4) becomes a copy of target angle 0;
7. current angle 1 (+0x1A8) becomes a copy of target angle 1;
8. current angle 2 (+0x1AC) becomes a copy of target angle 2.

Both distances and all three current angles therefore jump to the new values with no interpolation. The scale members, the eye, the anchor, the distance copy, the rebasing members and the shake amplitude are not stored.

| Routine | Stores at | Target angle 2 constant | Condition for the eight stores | Other effects in the same routine |
| --- | --- | --- | --- | --- |
| 0x083E70 | 0x083EFE–0x083F4E | positive zero | The property of C+0x800888 is cleared first, unconditionally. The stores happen only when the signed index at C+0x800A0C is negative, or is not below the element count of the four-byte-element sequence at C+0x800984–C+0x800988. | In the storing case the property of C+0x800880 is then set to 1 and many fields of A0 are reset. In the other case no camera member is stored; routine 0x0806C0 is called with the index and A0 and the property of C+0x80088C is set to 1. |
| 0x084170 | 0x08418C–0x0841DC | 180 (bits 43340000) | Unconditional. The property of C+0x80088C is cleared first. | Afterwards, when the signed index at C+0x800A10 is below the element count of the sequence at C+0x8009DC–C+0x8009E0, routine 0x06F620 is called with two values taken from the indexed element; otherwise the property of C+0x800890 is set to 1. |
| 0x082530 | 0x082580–0x0825D0 | 180 (bits 43340000) | Unconditional. The property of C+0x800890 is cleared first. | Afterwards a new element is appended to the sequence at C+0x8009DC, C+0x800A10 is set to its index, and 0x06F620 is called with two values taken from it. |

Routine 0x06F620 itself stores no camera member and does not store A0's position. Evidence: A at 0x083E70–0x084169, 0x084170–0x08422A, 0x082530–0x0829BD, 0x06F620 onward.

### Menu selection in the mouse-button callback

Routine 0x07DDD0 is the client virtual-table entry at byte slot +0x20 (table at VA 0x006FFDC8) and receives one 32-bit argument. After unrelated earlier work, it reaches a selection block only when all three of these hold: the property of C+0x800910 is nonzero, the argument is zero, and the signed 32-bit value at +0x160 of the object pointed to by C+0x800914 is not negative. It then clears that property, clears the byte at C+0x8008F2 and acts on that signed value:

- value 0: sets the property of C+0x800A00 to 1 and stores no camera member;
- value 2: sets the byte at C+0x1A0 to 1 and stores no camera member;
- value 1: calls 0x087520, 0x0878A0 and 0x0719F0 on the client, calls 0x0806C0 once per element of the sequence at C+0x800984, sets the property of C+0x800874 to 1, calls 0x06F620 with the current result of the imported `WINMM.dll` time query and an empty text, and then performs four stores in this order: current distance (+0x1BC) becomes positive zero; target distance (+0x1C0) becomes positive zero; target angle 0 (+0x1B0) becomes 120 (bits 42F00000); target angle 2 (+0x1B8) becomes positive zero;
- any other value: no further action.

On the value-1 route the current angles, target angle 1, the scale members, the eye and the anchor are not stored. The four stored values equal the constructed values of the same four members; the current angles keep whatever they held and are left to the contracted smoothing of later updates. Evidence: A at 0x07DDD0–0x07E1A3, stores at 0x07E145–0x07E163; image virtual table.

### Shake amplitude producer inside the per-frame update

At 0x095642 the per-frame update stores 1 (bits 3F800000) to the shake amplitude C+0x1E8. This happens inside a loop over records taken from a client container near C+0x800638; for each record the update calls 0x084350 with the record's 32-bit value at +0x0C, a position converted from the record, and two binary32 record fields, and then performs the store when that 32-bit value equals 0x51 or 0x52. The store is an assignment, not an addition; several qualifying records in one update leave the amplitude at 1. This site executes before the amplitude is read and decayed by the already contracted final phase of the same update. What places such records in the container was not audited. Evidence: U at 0x0955B1–0x09564C.

### Routines that only read

The following located uses take the address of a listed member but were confirmed as reads: 0x084350 copies the six 32-bit pieces of a position it is handed, and the client eye address is handed to it from 0x0709C0, 0x07B600, 0x084320 and several sites of the update; the update reads the anchor at 0x09843A and 0x099DDC as an operand of the fixed addition helper 0x02C800 and the fixed subtraction helper 0x02C7A0, both of which write only to a separate result; the update reads current angle 2 at 0x08B6E6 and 0x09B1A8; the update reads the eye at 0x089177 as the subtracted operand of 0x02C7A0. One use was not closed: 0x0A6B50 passes the eye address as an argument to world routine 0x19EE90 at 0x0A86DD and 0x0A8BFB, and that routine was not audited for stores through the argument. The update sites that read the anchor or eye earlier than the late phase observe the values left by the preceding update, or the constructed values on a first update. Renderer 0x0AC260 contains no direct store to any listed member.

## Program-driven angle and distance targets in front-end states

Three blocks early in the per-frame update store camera targets directly. They lie at 0x089CD3–0x089D4B, 0x089D4F–0x089DBC and 0x08A818–0x08A881, which is before the smoothing calls at 0x0965A2 of the same update, so an update that reaches both applies the smoothing to targets already modified by these blocks. Admission of the update's earlier code up to 0x089CD3 was not audited beyond observing that the path through 0x089982 loads `N` into the register the blocks use.

Each block is admitted by widget properties, queried through 0x07FA10:

| Block | Admission |
| --- | --- |
| First (0x089CD3) | the property of C+0x800874 is nonzero, or else the property of C+0x800894 is nonzero; and in either case the property of C+0x800898 is zero |
| Second (0x089D4F) | the property of C+0x800888 is nonzero and the property of C+0x800898 is zero |
| Third (0x08A818) | the property of C+0x80088C is nonzero and the property of C+0x800898 is zero |

An admitted block performs three operations in this order:

1. target distance (+0x1C0) is assigned 10 (bits 41200000);
2. target angle 0 (+0x1B0) is assigned 100 (bits 42C80000);
3. target angle 2 (+0x1B8) is increased: `N` is converted from signed 32-bit to binary32, multiplied in binary32 by 0.005 (bits 3BA3D70A, VA 0x006FCD9C), the old target angle 2 is added in binary32, and the sum is stored.

`N` is the same update argument used by the smoothing; the outer frame routine supplies it as the 32-bit difference of two results of the imported millisecond time query. The advance is therefore 0.005 degree per unit of `N`, about 5 degrees per second of elapsed time, always in the increasing direction, and with no wrap, clamp or reduction of the accumulated angle. A nonpositive `N` is not excluded by the block: zero adds zero and a negative `N` decreases the angle.

The three admissions are tested independently and in sequence. If more than one is satisfied in the same update, each satisfied block repeats all three operations, so target angle 2 advances once per satisfied block. The blocks do not store current angles, current distance, target angle 1 or the scale members, so the current values approach these targets only through the contracted smoothing. The second and third blocks' admissions also gate other screen layout work that follows them; that work stores no camera member.

Nothing in these blocks disables the mouse or wheel callbacks. Whether those callbacks can also change the same targets while one of these screens is shown depends on the admission rules of the earlier input contract and was not re-derived here. Evidence: U at 0x089982–0x089DBC and 0x08A7D8–0x08A881; A at 0x07FA10 and 0x028970 (the component-address helper used for target angles 0 and 2).

Taken with the earlier sections, the proven program-driven values are: constructed and "menu selection value 1" state of distance 0 with angle 0 of 120; front-end rotation state driving target distance 10, target angle 0 of 100 and a steadily increasing target angle 2; and the three button handlers snapping distance to 5, angle 0 to 90, angle 1 to 0 and angle 2 to 0 or 180.

## The follow anchor before and at the first update

The only located stores to the anchor (C+0x158, C+0x160, C+0x168) are the constructor's zeros and the late per-frame phase already contracted. No routine tied to connection, world selection, character selection or creation, the menu selection above, or the option routines stores the anchor, the eye, or A0's position into either of them. In particular the connection routine 0x06FC50, routine 0x06F620 and the three button handlers contain no store to C+0x140–C+0x16F.

Construction does give A0 a position. After creating A0 at 0x061FDE–0x062022, the constructor obtains the address W+0x8000F0 through 0x087FE0 and converts that three-component binary32 vector to fixed coordinates directly into A0+0x10 (0x062048–0x062059). Earlier in the same constructor, at 0x05A741, world routine 0x1A52E0 has been called, and that routine unconditionally stores binary32 bits 4B002080 (8,396,928) to W+0x8000F0 and W+0x8000F4 and zero to W+0x8000F8 (0x1A55F8–0x1A560C). If nothing between those two points changes that vector, which was not exhaustively audited, A0's constructed position is raw 0x0000008020800000 in x and in y and zero in z, that is 8,396,928 units in each horizontal coordinate. Evidence: K at 0x05A729–0x05A741 and 0x061FDE–0x062059; A at 0x1A52E0–0x1A5616.

Consequently the anchor is not initialised to the creature's position before the first update. When the late phase first runs, the anchor still holds its constructed zeros unless an earlier run of that same phase changed it, and the creature position it is compared with is far from zero under the values above. The first movement of the anchor is then made by the late phase itself, by whichever of its contracted mechanisms applies: the strict squared-distance test against 2,500 that copies the creature position into the anchor, and, when one of the four front-end widget properties tested by that phase is nonzero, the alternative route that replaces the anchor from W+0x8000F0 and the column heights. With the constructed values the squared-distance test is satisfied by a very large margin. No separate initialisation step exists to reproduce.

The same reasoning applies after the button handlers and the menu selection: they leave the anchor wherever the previous update left it, and only the late phase's snap or alternative route moves it to a newly entered world position.

Which front-end widget properties are nonzero at the first update is not established. The constructor explicitly clears the properties of C+0x800880, C+0x800890, C+0x800888, C+0x800894 and C+0x80088C immediately after creating each widget (0x05A83D, 0x05A8C6, 0x05A94F, 0x05A9D8, 0x05AA61) and does not clear the property of C+0x800874 after creating it at 0x05AA95. That pattern suggests, but does not prove, that a newly created widget's property is nonzero and that the first block of the previous section is admitted from the first update; the widget-creation default was not audited.

## Search method and its limits

Stores were located in three ways.

First, the complete listing A was scanned for every instruction whose destination operand is memory addressed as a general register other than the stack or frame register plus one of the displacements 0x140 through 0x16C in steps of 4, 0x18C, 0x190, 0x194, 0x1A4 through 0x1CC in steps of 4, 0x1D8, 0x1DC, 0x1E0, 0x1E4 or 0x1E8, with a mnemonic that can modify memory (moves of every width, floating and integer stores, arithmetic and logic with a memory destination, increments, decrements, shifts, exchanges and conditional byte sets). This is independent of which register holds the client, so it also returns many stores into unrelated objects that happen to use the same displacements. The 962 results were grouped by owning routine. Every result inside a client routine (the constructor, the virtual-table routines and the other routines between 0x059C40 and 0x0BFFFF) was examined in context to decide whether its base register holds the client. Every result outside that range with a displacement in the 0x1A4–0x1E8 band was examined at the level of its own instruction and its routine's set of stored displacements, widths and immediates. Results outside that range with displacements 0x140–0x16C, 0x18C, 0x190 or 0x194 were reviewed only as a per-routine summary of stored displacements, not instruction by instruction. Results whose base was a creature, a widget, an options-screen object, a copied creature-data record or a library object were discarded; the ones retained are the ones described above.

Second, address formation was scanned: every load-effective-address instruction and every register add or subtract of one of those displacements. Each such site inside the constructor, the per-frame update, the button handlers and the other client routines in the range 0x059C40–0x0BFFFF was followed far enough to classify the use. Uses that pass a member address to a helper were classified from the helper's own instructions for 0x084350, 0x028970 and the fixed-vector helpers; the option block address was followed to 0x06F390.

Third, because the listing omits or leaves unattributed some code, the code section of the image was scanned byte-wise for store-shaped encodings with a register-plus-32-bit-displacement operand, no index byte, and one of the same displacements. Of 1,010 such encodings, 48 had no matching listed store. They were inspected individually: they are immediate loads into a register, 64-bit loads misread as stores by the byte pattern, or stores inside creature, text-tokenizer and widget-library code. None is a store to a client camera member.

Limits. A store through a pointer that was computed in one routine and used in another is caught only where the pointer formation was found and followed as described; a pointer formed by arithmetic not matching the scanned shapes, a store using an index register, a block copy or block fill spanning the members, a store through the stack or frame register to an object embedded in a stack frame, and any store performed by an imported library on an address it was given are not excluded. Address formation outside the client routine range was examined only where the owning routine was otherwise shown to hold the client. One routine, 0x06EB80, stores its argument at +0x1B4 of its receiver; no call to it and no occurrence of its address were found in the listing or the image, so its receiver type is unknown and it is not attributed to the client. The classification of a base register as "not the client" rests on reading each routine's surrounding instructions, not on type information.

## Unresolved

- The content of C+0x1CC, C+0x1D8 and C+0x1E0 between construction and their first per-frame stores, and the conditions under which the update's early read of the two rebasing members is reached on a first update.
- The default property of a newly created widget, and therefore which front-end state and which program-driven block applies on the very first update.
- Whether the late follow phase is reached on every update while a front-end screen is shown.
- Whether anything between the world routine's stores and the creature's construction changes W+0x8000F0, every later producer of that vector, and every change to A0's position between construction and the first update.
- The widget library's rule for invoking the three registered button handlers and the options-screen routines, how the options-screen block is seeded, and what triggers the two callers of 0x06F390.
- The token helper's parsing rules, the runtime library's integer extraction behaviour on malformed or out-of-range text, and whether the pointer table containing the default stub is processed before startup in every launch path.
- The producer of the records that set the shake amplitude to 1.
- Whether the mouse and wheel callbacks are admitted concurrently with the three program-driven blocks.
- Any store reaching the listed members by a route outside the stated search limits.
