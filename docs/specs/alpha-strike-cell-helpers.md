Status: FINAL / STABLE / HELD. Full author readback completed.

# Strike block breaking and blast block removal: cell lookup, cell test, cell removal and change records

This independently authored, code-free contract closes the helpers of the melee strike's block-breaking step (step K3 of the held contract `alpha-strike-call-sites.md`) and compares that step with the block removal of the kind-144 blast (section 4.2 of the held contract `alpha-hazards-blast-ticks.md`). Those two contracts were used only to locate the two steps and for terms. Every statement below was re-established from the original images. The Infinite-Cubes alpha reconstruction was consulted for leads only (where the change list is consumed). Every lead stated here as fact was confirmed in the originals; the one that could not be confirmed is marked as a reconstruction observation (section 6.2) and listed as unresolved.

Supported originals: `binary/Cube.exe` (client), SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, and `binary/Server.exe` (server), SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`. Both hashes were recomputed before use. Both images have preferred base `0x00400000`. Every address below is an RVA, written client / server where both are given; where one is given it is the server RVA. Inside the world update routine the client lies `0xDA240` above the server. Evidence paths are relative to `Documents/disassembled alpha cube world`.

## 1. Evidence method and terms

The routines were read in `decompiled/server/assembly.txt` and `decompiled/client/assembly.txt`. Static data values and the three instruction windows that the client listing omits were read from the image bytes. Callers of the cell test, the cell removal, the change-list append and the follow-up routines were taken from `decompiled/<image>/calls.csv` and confirmed by scanning each image for every relative call and every absolute 32-bit reference to those addresses. Draw statements rest on a transitive walk of `decompiled/server/calls.csv`. The decompiler output of the light and plant routines (section 7) was read only as a lead; the statements made about them were checked against the instruction listing.

Terms:

- **Raw coordinate**: a signed 64-bit fixed-point value, 65536 units per block. **Presentation units**: a raw value converted to binary32 and scaled by 2^-16, as in the held contracts.
- **Block index**: a signed 32-bit integer block coordinate.
- **Cell**: a 4-byte record. Bytes 0, 1 and 2 are called C0, C1 and C2. Byte 3 is the **cell flags byte**: its low five bits (mask `0x1F`) are the **cell type**; bit `0x20` is called **flag A**, bit `0x40` **flag B**, bit `0x80` **flag C**. These are labels only; their game meaning is not established.
- **Column**: a 32-byte record. Signed 32-bit `+0x10` is the **base height** (block index of the first stored cell), `+0x18` points to the cell array, signed 32-bit `+0x1C` is the **cell count**. The stored heights are base height to base height + count − 1.
- **Zone**: a record holding 256 × 256 columns. Its pointer `+0xA8` is the start of the column array; the column for block (x, y) is at index (y mod 256) × 256 + (x mod 256). The zone's own grid coordinates are signed 32-bit `+0x60` and `+0x64`.
- **Truncating division** divides toward zero. **Floor-adjusted division** by 65536 is defined in section 2.1.
- **Draw**, **scaled unit draw with factor k**: as in the held contracts (one call through the runtime random import, client slot `0x2FC468` / server slot `0x1582F4`).
- **Aggregate**: the world update's output aggregate, as in the held contracts. Its list `+0x10` receives effect records, `+0x08` presentation records and `+0x18` change records.

## 2. Cell lookup (`0x02F860` / `0x006050`)

Receiver: the world. Arguments, in order: raw x, raw y, raw z (64-bit each) and a 32-bit **zone hint**. Both the strike and the blast pass a zone hint of 0. Result: a pointer to a 4-byte cell. The routine never returns null.

### 2.1 Block indices

Each raw coordinate v is converted independently:

1. When v is negative (high dword negative), 65536 is subtracted from v first (64-bit). When v is zero or positive nothing is subtracted.
2. The result is divided by 65536 with the runtime's signed 64-bit truncating division.
3. The low 32 bits are kept as the block index.

This is **floor-adjusted division**. For v ≥ 0 it equals the floor of v / 65536. For negative v that is not a multiple of 65536 it also equals the floor. For a negative exact multiple, v = −k × 65536 with k ≥ 1, it gives −k − 1, one below the floor. Examples: raw −1 gives −1; raw −32768 gives −1; raw −65536 gives −2; raw −65537 gives −2; raw 65535 gives 0; raw 65536 gives 1.

### 2.2 Column (`0x0347A0` / `0x006100`)

Inputs: block x, block y and the zone hint.

1. When x < 0, y < 0, x ≥ 2^24 or y ≥ 2^24, there is no column.
2. With a zone hint of 0, the zone is looked up by grid (x / 256, y / 256) (section 2.4). A nonzero hint is used as the zone pointer instead, and there is no column unless zone `+0x60` × 256 ≤ x < that + 256 and zone `+0x64` × 256 ≤ y < that + 256.
3. When the zone is null, there is no column. Otherwise the column is the one at index (y mod 256) × 256 + (x mod 256) of the zone's array. The array pointer itself is not tested.

### 2.3 Cell selection (`0x02F7E0` / `0x005FD0`)

Let z be the block z index and the three **fallback cells** be the static 4-byte values listed in section 2.5.

1. No column: the **grey fallback**.
2. z < base height: the grey fallback.
3. z ≥ base height + cell count: the **fluid fallback** when z ≤ 0, the **air fallback** when z > 0.
4. Otherwise the stored cell at index z − base height (through `0x02F730` / `0x005F20`, whose out-of-range branches are not reached here). Then, when that cell's type is 0, z ≤ 0 and flag B is clear, the fluid fallback is returned instead of the stored cell.

The result points into the column's storage, or to a static fallback. The lookup does not write anything.

### 2.4 Zone lookup (`0x034D10` / `0x006290`, sector `0x034A90` / `0x006210`)

Inputs: zone grid coordinates (zx, zy).

1. When zx or zy is negative or ≥ 65536, the result is null.
2. The sector is the pointer at world `+0xBC` + 4 × ((zx / 64) × 1024 + zy / 64). A null sector gives null.
3. The zone is the pointer at sector `+0x10018` + 4 × ((zx mod 64) × 64 + (zy mod 64)), which may itself be null.

### 2.5 Fallback cells

Each fallback is a static 4-byte value written by a small initialiser routine whose address is listed in a table of routine pointers in read-only data (presumably the runtime's static-initialisation table; the table's use was not traced). The data file holds zeros there; the values below are the initialised ones.

| Fallback | C0, C1, C2 | Flags byte | Type | Flags set | Client / server address | Initialiser client / server |
| --- | --- | --- | --- | --- | --- | --- |
| grey | 200, 200, 200 | `0x01` | 1 | none | `0x36AFC4` / `0x183D14` | `0x2F9750` / `0x1569E0` |
| fluid | 255, 255, 255 | `0x82` | 2 | C | `0x36AFBC` / `0x183D0C` | `0x2F9780` / `0x156A10` |
| air | 255, 255, 255 | `0x00` | 0 | none | `0x36AFC0` / `0x183D10` | `0x2F9720` / `0x1569B0` |

The initialiser pointers are at server `0x158440`–`0x158448` and client `0x2FC68C`–`0x2FC694`. Apart from these initialisers, no instruction in either image writes these addresses directly; only the selection routine refers to them. Because the lookup returns a writable pointer, a caller could in principle write through it into a fallback; whether any caller does was not audited.

## 3. Cell test (`0x20AA60` / `0x1306C0`)

The receiver is the cell pointer returned by the lookup. The result is flag A (bit `0x20` of the flags byte) as 0 or 1. The routine reads only that byte; it reads no type bits, colour, zone, column, height or world state, and makes no draw.

None of the three fallbacks has flag A set. So a missing zone, a missing column, a height outside the stored range, or the substituted fluid fallback always fails the test. Only a stored cell with flag A set passes, whatever its type and other flags.

Callers (confirmed by byte scan of both images): server `0x134B58` (blast) and `0x139A54` (strike); client `0x20ED98` and `0x213C94`, their counterparts. There are no others.

## 4. Cell removal (`0x071920` / `0x130470`) and column write (`0x04E700` / `0x01FE60`)

### 4.1 Removal

Receiver: the world. Arguments: block x, block y and block z (signed 32-bit each). No result.

1. When x < 0, y < 0, x ≥ 2^24 or y ≥ 2^24, nothing happens.
2. The zone is looked up directly by grid (x / 256, y / 256) (section 2.4). When it is null, nothing happens.
3. The column is taken as in section 2.2 step 3. When the computed column address is zero, nothing happens.
4. When z < base height or z ≥ base height + cell count, nothing happens. The removal never extends a column.
5. The **removal value** is chosen: the **fluid removal value** when z ≤ 0, the **air removal value** when z > 0. The column write (4.2) is applied with index z − base height and that value.

| Removal value | C0, C1, C2 | Flags byte | Client / server address | Initialiser client / server |
| --- | --- | --- | --- | --- |
| fluid | 255, 255, 255 | `0x82` | `0x36B054` / `0x1842D4` | `0x2F9B70` / `0x157560` |
| air | 255, 255, 255 | `0x00` | `0x36B058` / `0x1842D8` | `0x2F9B10` / `0x157500` |

These are separate static copies with the same values as the fluid and air fallbacks; their initialiser pointers are at server `0x158530`–`0x158538` and client `0x2FC6E0`–`0x2FC6E4`. Unlike the fallbacks, these two values are also read by other routines (server `0x12EF00` and `0x1308B0`, not audited). A third copy holding 200, 200, 200 and `0x01` sits at server `0x1842DC` and is not used by the removal.

### 4.2 Column write

Arguments: an index into the column and a pointer to a 4-byte value. With the removal the index is always inside the stored range, so only the in-range rule applies:

1. When the stored cell's flag C is clear, all four bytes are replaced by the value.
2. When flag C is set and the value's type is 0, nothing is written.
3. When flag C is set and the value's type is nonzero, the four bytes are replaced and flag C is then set.

For indices below or above the stored range, the write would first resize the column (`0x050C50` / `0x013420`); that path is not reached from the removal and was not compared between the images.

### 4.3 Effect of one removal

- At z > 0: a stored cell without flag C becomes C0 = C1 = C2 = 255 with flags byte 0 (type 0, no flags). A stored cell with flag C is left unchanged.
- At z ≤ 0: the stored cell becomes C0 = C1 = C2 = 255 with flags byte `0x82` (type 2, flag C), whether or not it had flag C.

Nothing else is written. The removal changes only the one cell: no neighbouring cell, no column bounds, no zone list, no plant or sprite entry, no light value and no record. It makes no draw. Callers (byte scan): server `0x134C8D` (blast) and `0x139B90` (strike); client `0x20EECD` and `0x213DD0`, their counterparts, plus the client-only received-change consumer at `0x08F320` (section 6.3).

## 5. Change records

### 5.1 Layout (20 bytes)

| Offset | Width | Value written by the strike and the blast |
| --- | --- | --- |
| `+0x00` | 32 signed | block x |
| `+0x04` | 32 signed | block y |
| `+0x08` | 32 signed | block z |
| `+0x0C` | 4 bytes | a copy of the air removal value: 255, 255, 255, then flags byte 0 (client copy at `0x36DE30`, initialiser `0x2FAFF0`; server uses `0x1842D8`) |
| `+0x10` | 32 signed | the world's **day counter**, world `+0x800160`, read just after the removal |

The record is initialised with `+0x0C` and `+0x10` zero (`0x01D7E0` in the server) and then filled. It is appended to aggregate list `+0x18` (node append `0x128400`, which copies all 20 bytes). The cell bytes are always the air value, even when the removal wrote the fluid value (z ≤ 0) or wrote nothing (flag C set, or the removal's height was out of range).

### 5.2 Day counter

World `+0x800160` is a signed 32-bit counter. The world initialiser sets it to 0 (`0x0C885F`). Early in the server world update (`0x13244C`–`0x1324A2`) the time-of-day value world `+0x80015C` is increased by the frame time times 10, or times 100 when a flag not traced here is set; it is pinned to 32 400 000 when world `+0xA4` is zero. While the time of day exceeds 86 400 000, the day counter is increased by 1, 86 400 000 is subtracted, and a routine (`0x124500`, not audited) is called. Another routine (`0x0D83A0`) also stores into the counter; it was not audited. The client's counterparts of these writes were not compared.

### 5.3 Block coordinates of the record and the removal

The strike and the blast convert the same raw cell position in different ways:

| Step | Strike (K3) | Blast |
| --- | --- | --- |
| lookup and test | floor-adjusted on x, y and z | floor-adjusted on x, y and z |
| removal x and y | truncating division by 65536 (`0x02F570` / `0x005640`) | truncating division by 65536 |
| removal z | truncating division by 65536 | floor-adjusted (`0x005510`) |
| record x, y, z | truncating division by 65536 on each (`0x0C64F0` in the server) | floor-adjusted on each (`0x005450` in the server) |

For non-negative coordinates every method gives the same block. For a negative x or y the lookup yields no column, so the test fails and neither removal nor record is produced; the x and y difference therefore has no effect. For a negative z the strike's removal and record use a different height from the tested one: one block higher for a non-multiple of 65536 (for example raw z −32768 is tested at −1 but removed and recorded at 0), and one block higher for an exact negative multiple (raw −65536 is tested at −2 but removed and recorded at −1). The blast's removal and record match its tested height.

## 6. Consumption of the change records

### 6.1 Server world update

Late in the world update (`0x14710B`–`0x1471DC`), aggregate list `+0x18` is walked in list order. For each record the zone is looked up by grid (record x / 256, record y / 256), with truncating division, through the zone lookup of section 2.4. When the zone exists, the 20-byte record is appended to the zone's change vector at zone `+0x68` (vector append `0x01F4D0`). When it does not, the record is not stored in any zone. The list itself is not altered. The gating of this step within the world update was not audited.

### 6.2 Further server use

How the server transmits aggregate list `+0x18` and the zone change vectors to clients, and whether it saves them, was not confirmed in the original. The reconstruction suggests the list is copied to every connection, and that a zone's stored changes are sent when a client requests the zone. This is a reconstruction observation only.

### 6.3 Client consumer (client only, `0x08F242`–`0x08F431` in client routine `0x088EE0`)

A received list of 20-byte change records at `+0x800648` of that routine's receiver is processed when it is not empty. The records are walked in list order:

1. When the record's flags byte (`+0x0F`) is 0, the cell removal (section 4) is applied at the record's block x, y and z. Every record made by the strike or the blast takes this path.
2. Otherwise a cell-set routine (`0x04E7A0`, not audited) is applied with the record's coordinates and its 4-byte value.
3. The minimum and maximum block x and y over all records are tracked, starting from the first record.

After the walk the plant pass (section 7.2, client `0x1A5CB0`) runs once over (minimum x, minimum y, maximum x, maximum y). The routines called just before and after the walk (`0x19C480`, `0x201EA0`) and the later append of the records to another list (`0x057B30`) were not audited. The day field `+0x10` is not read here. The light pass is not run by this consumer. How records reach the list at `+0x800648` was not traced.

The client's world update also contains the zone-vector step of 6.1 (client `0x22134B`, instruction-identical).

## 7. Follow-up work after the cell loop, and the null world `+0xB8` branch

### 7.1 Order in the strike (after K3's cell loop)

1. When at least one cell passed the test, a presentation record with identifier 2 is appended to aggregate list `+0x08` (`0x139C49`–`0x139CB0`), at the striker's raw position `+0x10` converted to presentation units, T2 = 1, and T1 = scaled unit draw with factor 0.4, plus 0.5. This is **one draw**. When no cell passed, steps 2 and 3 are skipped as well.
2. **Null world `+0xB8` branch** (`0x139CC8`–`0x139D80`): only when world `+0xB8` is null, the light pass (7.3) runs on the rectangle described below with margin 8 and zone hint 0.
3. The plant pass (7.2) runs on the same rectangle, whatever world `+0xB8` is.

The **strike rectangle**: let cx and cy be the strike centre's raw x and y and F the raw offset obtained from R × 65536 computed in binary32 and converted to 64 bits by the runtime helper `0x14A946` (round, then corrected toward zero, so truncation). Its corners, in block indices by truncating division by 65536, are (cx − F, cy − F) and (cx + F, cy + F). R here is the strike reach after all K2 adjustments and is not capped at 8.

The null branch makes no draw. It only rewrites light bytes of type-0 and type-2 cells (7.3); it appends no record.

### 7.2 Plant pass (`0x1A5CB0` / `0x0D9160`)

Arguments: block x low, block y low, block x high, block y high (inclusive).

1. Negative arguments are raised to 0. Each bound is converted to a zone grid coordinate by division by 256.
2. For each zone grid x from low to high, and for each zone grid y from low to high (inner), the zone is looked up as in section 2.4. Missing zones are skipped.
3. The zone's list at zone `+0x04` (head node pointer; count at zone `+0x08`) is walked in list order. An entry is marked for removal when its byte at node `+0x40` has bit `0x02` set and the cell returned by the lookup (section 2, zone hint 0) at the entry's raw position (node `+0x10`, `+0x18`, `+0x20`) with z lowered by 65536 has type 0.
4. Every marked entry is then unlinked, freed and the count decreased, in marking order.

Because the lookup substitutes the fluid fallback (type 2) for empty cells at z ≤ 0 without flag B, an entry whose lower neighbour is such a cell is not removed. The pass makes no draw. Client and server are instruction-identical; three instruction runs missing from the client listing after the free calls (`0x1A5ED1`, `0x1A5EFE`, `0x1A5F0D`) were read from the image bytes and match the server bytes.

### 7.3 Light pass (`0x19A0E0` / `0x0D1A70`)

Arguments: block x low, block y low, block x high, block y high, a margin m and a zone hint (here 8 and 0). Only cells of type 0 or type 2 (**light-carrying cells**) are changed. The outer area is x from (x low − m) to (x high + m) and y from (y low − m) to (y high + m), both exclusive of their upper end; columns are found as in section 2.2 with the given hint.

1. **Sky pass.** In each column of the outer area, cells are scanned from the highest stored index down. Until the first cell that is not light-carrying is met, each light-carrying cell gets C1 = C2 = 255; after it, each light-carrying cell gets C1 = C2 = 0. C0 is kept.
2. **Spreading, 16 rounds.** In each round, every light-carrying cell of the outer area whose C2 is not 255 takes the largest of the values of its neighbours at x − 1, x + 1, y − 1, y + 1, z − 1 and z + 1, examined in that order and stopping early once 255 is reached. A neighbour of type 13 counts 255; a light-carrying neighbour counts its C2, raised to 5 when below 5; any other neighbour counts 0. Neighbour cells are found with the fallback rules of section 2.3, using this routine's own copies of the three fallback values (server `0x184240`, `0x184244`, `0x184248`, the same values; client `0x36B7CC`, `0x36B7D0`, `0x36B7D4`). C1 becomes (that largest value × 85) ÷ 100, as unsigned integers truncated. After every cell of the round, each light-carrying cell's C2 is set to its C1.
3. **Commit.** For x from x low to x high and y from y low to y high, both exclusive of the upper end, each light-carrying cell's C0 becomes its C2.

The pass makes no draw and appends nothing. Client and server are instruction-identical apart from the addresses of the fallback copies.

### 7.4 Blast follow-up (dead-creature loop, `0x134D42`–`0x134ED0`)

1. When at least one cell passed the test, a presentation record with identifier 1 is appended to aggregate list `+0x08` at the blast creature's raw position in presentation units, T2 = 1, and T1 = scaled unit draw with factor 0.4, plus 0.5 (**one draw**). Otherwise steps 2 to 4 are skipped.
2. Only when world `+0xB8` is null, the light pass runs with margin 8 and zone hint 0 on the **blast rectangle**, whose corners are the blast creature's raw x and y minus and plus exactly 8 blocks (8 × 65536), converted by truncating division.
3. The world's critical section at world `+0x8000D8` is entered (`0x0D3E00`).
4. The plant pass runs on the blast rectangle, and the critical section is left (`0x0D5C70`).

The strike does not enter or leave that critical section around its plant pass.

### 7.5 Other callers of the follow-up routines and other change-record producers

Light pass: server `0x121C64`, `0x13769C`; client `0x09DB84`, `0x1EDE84`, `0x2118DC`, plus the two blast and strike sites. Plant pass: server `0x1376E9`; client `0x08F3D0`, `0x211929`, plus the blast and strike sites. Other appends of 20-byte change records: server routine `0x124540` (18 sites) and world update site `0x137566`. None of these was audited, and none calls the cell test or the cell removal.

## 8. The two block-removal steps compared

### 8.1 Strike (K3, `0x1398D2`–`0x139E44`)

1. Gate (located by the held contract; the comparisons were re-read): world byte `+0xB4` zero, the striker size x value greater than 4 (ordered), action not `0x57`, `0x5B` or `0x4A`. The cell flag local is cleared.
2. n is R converted to a 32-bit integer by truncation, capped at 8.
3. Integer offsets (i, j, k) each run from −n to n inclusive, i outermost (x), then j (y), then k innermost (z).
4. The cell position is the strike centre plus (i, j, k) × 65536 (64-bit). The squared length of (cell position − centre) in presentation units, which equals i² + j² + k² exactly, must be less than n × n (a 32-bit product converted to binary32; ordered comparison). Otherwise the offset is skipped.
5. Lookup (section 2, zone hint 0) and test (section 3). A failing cell is skipped.
6. **Effect record** to aggregate list `+0x10` (initialiser `0x0C8510`): raw position = the unsnapped cell position (`+0x00`, 3 × 64); vector (0, 0, 10) (`+0x18`, 3 × binary32); colour (C0 ÷ 255, C1 ÷ 255, C2 ÷ 255, 1) (`+0x24`, 4 × binary32), read from the cell before removal; binary32 0.5 (`+0x34`); integer 3 (`+0x38`); and from the initialiser integer 0 (`+0x3C`) and binary32 3 (`+0x40`). Each list element occupies 72 bytes; the bytes after `+0x43` were not traced.
7. Cell removal (section 4) with the strike's block indices (5.3).
8. Change record (section 5) with the strike's block indices.
9. The cell flag local is set.
10. After the loop: section 7.1.

### 8.2 Blast (`0x134A5A`–`0x134D3C` and 7.4)

1. Offsets (i, j, k) each run from −8 to 8 inclusive, i outermost (x), then j (y), then k innermost (z).
2. The cell position is the blast creature's raw position plus the offset × 65536. The squared length of the offset in presentation units must be less than 64 (ordered).
3. Steps 5 to 9 of 8.1 follow with the same lookup, the same test, the same effect record values, the same removal routine and the same change-record layout, except that the removal's z and the record's coordinates use the blast's conversions (5.3).
4. After the loop: section 7.4.

### 8.3 Differences

| Point | Strike | Blast |
| --- | --- | --- |
| centre | strike centre (K2) | blast creature's raw position |
| radius | n = truncated R capped at 8; i² + j² + k² < n² | 8; i² + j² + k² < 64 |
| removal z and record coordinates | truncating | floor-adjusted |
| presentation identifier | 2, at the striker | 1, at the blast creature |
| follow-up rectangle | centre ± R (binary32, truncated to raw) | position ± 8 blocks |
| critical section around the plant pass | no | yes |

Everything else in the cell step is shared: the same lookup, test, removal and column write, and the same change-record layout and values.

## 9. Random draws

- Cell lookup, cell test, removal, column write, change record, effect record, the zone-vector step, the light pass and the plant pass: none. The transitive walk of the server call list from each of these routines reaches no random import; the only imports reached are the container error routines and, for the lock helpers, the critical-section imports.
- Strike K3: exactly one draw per frame, the identifier-2 record's T1, only when at least one cell passed the test.
- Blast: exactly one draw per blast, the identifier-1 record's T1, only when at least one cell passed the test (the same draw as the held blast contract's removal-sound draw).
- The null world `+0xB8` branch: none.
- The client consumer of section 6.3: its removal and plant pass make none; the routines it calls that were not audited were not walked.

## 10. Client and server

The following were compared instruction by instruction after masking absolute addresses, and agree: the lookup and its column, cell-selection, zone and sector helpers; the cell test; the removal; the column write; the strike block step (client `0x213B12`–`0x214084` against server `0x1398D2`–`0x139E44`, 330 instructions each); the dead-creature loop containing the blast (client `0x20EB63`–`0x20F66F` against server `0x134923`–`0x13542F`, 670 instructions each); the light pass; the plant pass (after the byte check of 7.2); and the zone-vector step. The only differing operands are absolute addresses: callee, branch, runtime-routine and exception-handler addresses, which pair consistently, and static data addresses (the fallbacks, the removal values, the light pass's fallback copies and the change-record value). Every pair was read from its initialiser in both images and holds the same bytes. The client's change record uses its own air copy (`0x36DE30`) rather than the removal's copy, with identical content.

The only behavioural difference found is the client-only consumer of section 6.3, which has no server counterpart. Which program actually runs the strike and blast steps depends on runtime values (world byte `+0xB4`, world `+0xB8`) that are not established here.

## 11. Unresolved

- Game meaning of the cell type values, of flags A, B and C, of the light bytes, of the zone list at `+0x04` and its bit `0x02`, and of the day counter. "Fluid", "air", "grey", "light", "plant" and "day" are descriptive labels only.
- Network transmission of aggregate list `+0x18`, the server's use of the zone change vector `+0x68` (sending, saving) and how the client's received list is filled (reconstruction lead only, section 6.2).
- The client cell-set routine `0x04E7A0`, the routines bracketing the client consumer, the later list append there, and the client counterparts of the day-counter writes.
- Whether any caller writes through a pointer to a fallback cell.
- The column-resize routine and its client counterpart, not reached from the removal.
- The other producers of change records and the other callers of the light and plant passes (section 7.5).
- The gating of the zone-vector step within the world update, and the runtime values of world byte `+0xB4` and pointer `+0xB8` on each program.
- Behaviour of the light pass when its rectangle is very large (the strike rectangle grows with R), and of every routine at coordinates near the 64-bit or 32-bit wrap.
- The 72-byte effect-record node beyond `+0x43`, and the rendering of effect and presentation records.
