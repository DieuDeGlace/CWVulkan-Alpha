# Native light-byte writers and passive observation points in the Alpha client

Author status **FINAL / STABLE / HELD**.

This contract lists every original route found that can change the first three bytes of a stored cell in a resident terrain column, and the places where a passive in-process observer can record an ordered event for each route. It states which thread runs each point, which world critical sections are held there, where the needed values live, and the length and position dependence of the instructions at each point. It is a description of the original client only. It does not describe how an observer is built.

## Supported image

The evidence is the original 32-bit Alpha client `Cube.exe`: 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, PE machine 0x014C, timestamp field 0x51EA955E, preferred image base 0x00400000, image size 0x003BC000. All addresses are RVAs. The image is relocatable, so a running copy places each RVA at its actual module base plus the RVA. Instruction lengths and branch targets were taken from the image bytes. The evidence call index omits part of the world-step routine, so callers were also found by scanning the code section for direct relative calls and for absolute references to each routine.

The world is the instance embedded at client +0x2E4. C0 is the critical section at world +0x8000C0 (enter 0x201CB0, leave 0x201E90). D8 is the section at world +0x8000D8 (enter 0x19C480, leave 0x201EA0). Both are ordinary recursive Windows critical sections. The client pointer is held at RVA 0x36B1C8 once construction finishes (see alpha-extractor.md, section B1.1).

## Cell bytes and addressing

A stored cell is four bytes. Byte +3 holds the material tag in its low five bits and flags in its top three bits. For stored tags 0 and 2 (air and water), byte +0 is committed light, byte +1 is the next-round working value and byte +2 is the current-round working value. For other tags, bytes +0 to +2 are color and the light routine leaves them alone. Section B2.4 of alpha-extractor.md says the light storage was undetermined. The commit at 0x19A7CA–0x19A7CD, the seed write at 0x19A189–0x19A18C and the transfer at 0x19A72B–0x19A72E settle that for tags 0 and 2. Its addressing rules are unchanged.

To find the bytes for whole-cell world coordinates X, Y (each 0 through 16,777,215) and signed height Z:

1. Zone indices are X divided by 256 and Y divided by 256. Region indices are the zone indices divided by 64. The region pointer is the 32-bit value stored at world +0xBC + 4 × (1,024 × region X + region Y). A null region means no zone.
2. The zone pointer is the 32-bit value stored at region +0x10018 + 4 × (64 × (zone X mod 64) + (zone Y mod 64)). A null entry means the zone is not resident.
3. With local x = X mod 256 and local y = Y mod 256, the column record is at the zone's column-array pointer (zone +0xA8) plus 0x20 × (x + 256 × y). X varies fastest in this array, unlike the region and zone tables.
4. The column holds a signed 32-bit base height at +0x10, a 32-bit cell-array pointer at +0x18 and a signed 32-bit stored count at +0x1C. Z is stored when Z minus base lies in 0 through count minus 1. The cell is then at the cell-array pointer plus 4 × (Z − base). Bytes +0, +1, +2 and +3 follow in memory order.

Heights outside the stored span have no storage. Reads there return shared preset records (0x19AAF0, 0x02F730), which are not resident data. The native routines do these lookups without taking a lock (0x0347A0, 0x034D10, 0x034A90). An observer's table read is valid only while zone departure is held off (see Safety limits). A cell address is valid only until the next column resize, described under edit writers.

## Inventory of writers

The image has five direct calls to the rectangle light routine at 0x19A0E0. There are no absolute references to it. Edit writers change all four bytes of individual cells.

| Route | Caller site | Return address | Thread | C0 | D8 | Destination |
| --- | --- | --- | --- | --- | --- | --- |
| Mesh-builder relight | 0x09DB84 | 0x09DB89 | mesh worker | no | yes | resident columns |
| New-zone lighting | 0x1EDE84 | 0x1EDE89 | streaming | no | no | explicit unpublished zone |
| Bomb destruction relight | 0x20F071 | 0x20F076 | main | yes | no | resident columns |
| Building-placement relight | 0x2118DC | 0x2118E1 | main | yes | yes | resident columns |
| Variable-radius destruction relight | 0x213FC0 | 0x213FC5 | main | yes | no | resident columns |
| Queued client cell clear | 0x08F320 → 0x071920 | — | main | yes | yes | resident column |
| Queued client cell write | 0x08F34A → 0x04E7A0 | — | main | yes | yes | resident column |
| Bomb terrain clear | 0x20EECD → 0x071920 | — | main | yes | no | resident column |
| Variable-radius terrain clear | 0x213DD0 → 0x071920 | — | main | yes | no | resident column |
| Building-placement cell writes | 0x211682, 0x211725 → 0x04E7A0 | — | main | yes | yes | resident columns |

Notes on the table:

- The mesh worker is the thread whose body is at 0x0690A0. The streaming thread's body is at 0x06A8A0 and calls the generator at 0x06ACD1. The main thread runs the controller update at 0x088EE0, which holds C0 from 0x08902F to 0x09D2EA. The world step at 0x20C510 is called from that update at 0x08CF2E and 0x08CF49.
- The relight at 0x2118DC was absent from the evidence call index and from the earlier held lighting contracts. It belongs to a world-step branch that starts at 0x2112F7. The branch skips when the byte at world +0xB4 is nonzero. It fetches a voxel model from the map at world +0x800154, keyed by the signed actor byte at +0x18D (0x211319, routine 0x198840). It enters D8 at 0x211451, writes model cells through 0x04E7A0 and appends each change to the step result's list at +0x18 (0x2117A6). If world +0xB8 is null it relights at 0x2118DC with margin 8 and no explicit zone. It cleans decorations at 0x211929 and leaves D8 at 0x2119C2. A reconstruction lead identifies this as building placement. The original evidence confirms the behavior above but not the name.
- The three world-step relights run only when the 32-bit pointer at world +0xB8 is null (tests at 0x20EFDE–0x20EFEB, 0x2117FD–0x21180A and 0x213EF5–0x213F02). The Bomb and variable-radius relights also need at least one qualifying deletion in the same branch (byte at frame offset −0x2B29, tested at 0x20EF82 and 0x213E7B). alpha-extractor.md records that start-up and server join set +0xB8. The traced local routes therefore skip these calls while it is set.
- Queued edits run inside the D8 interval 0x08F25F–0x08F3D9 and produce no immediate relight. Their light effect comes from the four-byte write itself, followed by a later mesh-builder relight through the dirty byte.
- Generation and saved-edit replay during zone formation write only the zone under construction. That zone is published later at 0x1EE05A. The call index traces every other cell-writing caller to the generator at 0x1E4850. Whether the generator ever writes into another zone that is already published remains unresolved (alpha-extractor.md, X-7).
- The world-map thread enters D8 on the world (for example 0x0697E3–0x069804) but has no cell-write calls.

## Mesh-builder light request

### Admission

The worker holds the cache lock at client +0x800600 throughout candidate processing (entered by import call at 0x0693F4, left at 0x069487). It released C0 at 0x0693DD. When a candidate needs a build and the pass still has its build opportunity (byte at worker frame offset −0x39 equal to 1), control reaches 0x069560 and the admission routine is called at 0x069569.

| Point | Meaning | Values |
| --- | --- | --- |
| 0x069560 | Admission is requested for this candidate. | ESI points to the 16-byte candidate record: +0x00 slot pointer, +0x08 chunk X (signed 32-bit), +0x0C chunk Y (signed 32-bit). The dword at [EBX] is the client pointer. Slot +0x74 is the dirty byte (8-bit), and slot +0x18 and +0x1C hold the published chunk indices. |
| 0x06F490 (entry) | Admission evaluation begins. | ECX is the client. [ESP+4] is chunk X and [ESP+8] chunk Y, both signed 32-bit. |
| 0x06F5FC | Admitted. Earliest point where the decision is final. | [EBP+8] chunk X, [EBP+0xC] chunk Y, [EBP−8] client. Neither argument slot is rewritten. |
| 0x06F607 | Denied. Earliest point where the decision is final. | Same frame values. |
| 0x06956E | Back in the worker. AL is 1 if admitted and 0 if denied. | ESI and EBX as at 0x069560. |
| 0x069576 | Admitted, immediately before the builder call at 0x069580. | Same as 0x069560. EDI holds chunk Y. |

The admission routine looks up zones only. It checks the containing zone ((32 × chunk X) and (32 × chunk Y), each divided by 256 with signed truncation). When the within-zone cell origin on an axis is 0 the step on that axis is −1, when it is 224 the step is +1, and otherwise it is 0. If any step is nonzero it looks up the chunk offset by both steps. When both steps are nonzero it then looks up the X-only and Y-only offsets. Each neighbor index is the offset chunk coordinate times 32, divided by 256 with truncation. The first null pointer denies (0x06F4AA–0x06F5FA). The routine retains no per-neighbor result. At the deny exit the registers and locals do not reliably identify which lookup failed, because the early range and containing-zone failures leave the step locals unset. An observer that needs per-neighbor presence must repeat the same table reads at entry or at an exit. It holds no world lock there, so its reads can differ from the native reads (see Safety limits).

### Relight decision, propagation and commit

The builder at 0x09D910 is called only from 0x069580, as a call on the client with chunk X and chunk Y as stack arguments. It enters D8 at 0x09DB0B. D8 stays held through the relight until 0x0A08E8. The worker's cache lock remains held, and C0 is not held. A coordinate failure before that point exits at 0x0A1499 without a light request.

| Point | Meaning |
| --- | --- |
| 0x09DB10 | First instruction after D8 is entered. The request byte has not been read yet. |
| 0x09DB55 | A relight has been chosen. DL holds the dirty byte exactly as the native code read it at 0x09DB17. A nonzero DL selects four cells of padding and zero selects sixteen. Control reaches this point only when a relight follows. |
| 0x09DB89 | The light routine has returned with its commit finished. The skip branch from 0x09DB53 also arrives here, so a visit without a preceding 0x09DB55 in the same build means the build skipped relighting (a clean interior chunk). |

At all three points the stack offsets are relative to ESP at the instruction. The light routine removes its own six arguments, so the offsets match at 0x09DB89.

| Value | Location | Width |
| --- | --- | --- |
| Client | EBX, also [ESP+0x14] | 32-bit pointer |
| World | ESI, also [ESP+0x1C] | 32-bit pointer |
| Chunk X, chunk Y | [EBP+8] and [EBP+0xC]; also [ESP+0xFC] and [ESP+0x134] | signed 32-bit |
| Chunk origin X in whole cells | [ESP+0x78] | signed 32-bit |
| Chunk origin Y in whole cells | EDI, also [ESP+0x80] | signed 32-bit |
| Cache slot | [ESP+0xB0] | 32-bit pointer |
| Dirty byte | DL at 0x09DB55; slot +0x74 | 8-bit |

EBX, ESI and EDI survive the light call. EAX, ECX and EDX are undefined at 0x09DB89. The requested rectangle runs from the origin minus the padding (inclusive) to the origin plus 32 plus the padding (exclusive) on each axis, with working margin zero and no explicit zone. The same values appear as the light routine's arguments.

## The light routine as a common point

All five relights pass through 0x19A0E0, so a single entry and exit pair records every light request in order. The routine takes no lock and makes no calls except to the column lookup 0x0347A0, the sample 0x02F730 and the cell accessor 0x19AAF0. It has one exit sequence.

At the entry (0x19A0E0), ECX is the world. [ESP] is the return address, which identifies the caller (module base plus one of 0x09DB89, 0x1EDE89, 0x20F076, 0x2118E1 or 0x213FC5). The arguments are signed 32-bit values: [ESP+4] low X, [ESP+8] low Y, [ESP+0xC] high X, [ESP+0x10] high Y. Low bounds are inclusive and high bounds are exclusive whole cells. [ESP+0x14] is the working margin, and [ESP+0x18] is the explicit zone pointer, or null for resident lookup.

At the exit sequence (0x19A7EE), the frame is intact. [EBP+4] holds the return address, [EBP+0xC] to [EBP+0x1C] hold low Y, high X, high Y, margin and explicit zone, and [EBP−0xC] holds the world. The low-X slot [EBP+8] is no longer the argument. The commit loop rewrites it at 0x19A7E7 and leaves high X there when low X was below high X. An observer must capture low X at entry. This point follows the final commit store, and no light byte changes before the return at 0x19A7F4.

Three optional points mark internal phases.

- At 0x19A1C2 the seed sweep has finished. The empty-rectangle branch from 0x19A10A also arrives here.
- At 0x19A759 the transfer for one round has finished, and [EBP−0x18] holds the zero-based round index as a 32-bit value.
- At 0x19A772 all sixteen rounds have finished and the destination commit is about to begin.

The routine does not retain destination column pointers. The observer resolves them by the addressing rule above, over the destination rectangle (commit) and the working rectangle (work bytes). Face reads reach one further cell horizontally, so the zones that can influence the result are those intersecting the working rectangle widened by one cell. With an explicit zone, only that zone's own 256-by-256 square can resolve.

The mode that selected the work, by caller:

| Caller | Selection |
| --- | --- |
| Mesh builder | Dirty byte (nonzero gives padding 4, zero at a zone seam gives padding 16). Margin 0. No explicit zone. |
| New zone | Whole zone, margin 0, explicit zone non-null. |
| Bomb | Fixed eight-cell destination around the bomb position, margin 8, after at least one deletion, with world +0xB8 null. |
| Building placement | Model-size destination, margin 8, with world +0xB8 null. In binary32, low X is base X minus half of size at model +0x44, low Y is base Y minus half of size at +0x48, high X is base X plus half of size at +0x48, and high Y is base Y plus half of size at +0x4C. Each is truncated toward zero, with the half from the constant 0.5 at 0x345DA0 (0x211810–0x2118DB). |
| Variable-radius destruction | Radius-sized destination, margin 8, after at least one deletion, with world +0xB8 null. |

The Bomb and variable-radius bounds follow alpha-native-mesh-cache-light-padding.md and alpha-light-propagation-lifecycle.md. The building bounds use the model's sizes in that mixed order exactly as the image does. The base here is the whole-cell placement vector the branch computes before its loops.

## World-step decision points

These points record a world-step request that the +0xB8 test or the deletion flag suppresses, which never reaches 0x19A0E0.

| Point | Meaning | Values |
| --- | --- | --- |
| 0x20EF82 | Bomb deletion flag. A zero byte skips both the relight and the D8 cleanup. | DL, 8-bit |
| 0x20EFDE | Bomb relight test of world +0xB8 | [EBP−0x2B18] world pointer, 32-bit |
| 0x20F076 | Bomb relight returned. Reached only when it ran. | same frame |
| 0x213E7B | Variable-radius deletion flag | byte at [EBP−0x2B29] |
| 0x213EF5 | Variable-radius relight test of +0xB8 | [EBP−0x2B18] world |
| 0x213FC5 | Join after the variable-radius relight or its skip | same frame |
| 0x2117FD | Building relight test of +0xB8 | [EBP−0x2B18] world; [EBP−0x2B50] model pointer |
| 0x2118E1 | Join after the building relight or its skip | same frame |

These frame offsets are EBP-relative in the world step. All of these points run on the main thread under C0. Only the building points also hold D8.

## Edit writers

The common four-byte store is at 0x04E700. It is a call on a column pointer in ECX, with a stack index relative to the column base ([ESP+4], signed 32-bit) and a pointer to the new four-byte record ([ESP+8]). A negative index or one at or beyond the count first resizes through 0x050C50 (calls at 0x04E719 and 0x04E74F). A resize can change the base, count and cell-array pointer of a resident column. When the existing cell has flag 0x80 set and the new tag is 0, nothing is written. Otherwise the four bytes are copied, and with 0x80 previously set that flag is set again. The exits are 0x04E73E (plain copy and no-write cases) and 0x04E789 (flag-preserving copy). At 0x04E789 the four bytes are final, and the flag store follows inside that window.

The clear at 0x071920 is a call on the world with whole-cell X, Y and Z ([ESP+4], [ESP+8], [ESP+0xC], signed 32-bit). It uses resident lookup only and acts only when Z lies inside the stored span, so it never resizes. It writes the record at 0x36B058 (255, 255, 255, tag 0, set by 0x2F9B10) above height zero and the record at 0x36B054 (255, 255, 255, 0x82, set by 0x2F9B70) at or below zero. The exits are 0x0719D0 after the store and 0x0719E5 for the common path, including no-op cases.

The cell write at 0x04E7A0 is a call on the world with X, Y, Z, a record pointer and an optional explicit zone ([ESP+4] through [ESP+0x14]). It returns at 0x04E887 with five arguments removed. A record whose byte +3 is entirely zero, at or above the stored top, writes nothing. A record whose tag and low flag bits (mask 0x5F) are zero is replaced by the record at 0x36B010 at or below height zero. If the write lands below the previous base, the cells between it and the old base are filled with tag 6 records. The exit sequence begins at 0x04E877 and is reached after every outcome.

## Instruction properties at each point

No instruction in the listed windows is a relative branch or call unless the table says so. "Branch target" means direct branches land on the first byte. None of the windows contains a direct branch target after its first byte, unless the table says so.

| Point | Whole-instruction lengths in bytes | Properties |
| --- | --- | --- |
| 0x069560 | 3, 2, 1 | No direct branch target. Memory operands through ESI and EBX. |
| 0x06956E | 2, 6 | The second instruction is a conditional relative near branch. Unsuitable without relocating that branch. |
| 0x069576 | 2, 1, 3 | No direct branch target. |
| 0x06F490 | 1, 2, 3 | Routine entry. |
| 0x06F5FC | 1, 1, 2, 1 | Branch target of 0x06F563, 0x06F5B5 and 0x06F5B9. The result byte is set inside this window. Followed by 2, 1, 3 to the return. |
| 0x06F607 | 1, 1, 2, 1 | Branch target of eight sites. The result byte is cleared inside the window. |
| 0x09DB10 | 7 | One ESP-relative load. No branch target. |
| 0x09DB55 | 2, 4 | Branch target of 0x09DB1C, 0x09DB30, 0x09DB44 and 0x09DB4B. The first instruction sets the flags that a conditional move at 0x09DB67 consumes, and the second is ESP-relative. |
| 0x09DB84 | 5 | Relative call. Position-dependent. |
| 0x09DB89 | 4, 2 | Branch target of 0x09DB53. The first instruction is ESP-relative. |
| 0x19A0E0 | 1, 2, 3 | Routine entry. |
| 0x19A1C2 | 2, 3 | Branch target of 0x19A10A. |
| 0x19A759 | 3, 3 | No direct branch target. |
| 0x19A772 | 3, 3 | No direct branch target. |
| 0x19A7EE | 1, 1, 1, 2 | Branch target of 0x19A778. Followed by a 1-byte restore and a 3-byte return that removes 0x18 bytes. |
| 0x20EFDE | 6 | No direct branch target. |
| 0x20F076 | 6 | No direct branch target. |
| 0x213EF5 | 6 | No direct branch target. |
| 0x213FC5 | 2, 6 | Branch target of 0x213F02. |
| 0x2117FD | 6 | Branch target of 0x211467. |
| 0x2118E1 | 2, 3 | Branch target of 0x21180A. |
| 0x04E700 | 1, 2, 1, 3 | Routine entry. Also called from 0x04E815 and 0x04E86D. |
| 0x04E73E | 1, 1, 1, 3 | Branch target of 0x04E76D. The last instruction is the return that removes 8 bytes. |
| 0x04E789 | 3, 1, 5 | No direct branch target. The third instruction is the flag store. |
| 0x04E7A0 | 1, 2, 3 | Routine entry. The next instruction carries an absolute address covered by a base relocation. |
| 0x04E877 | 3, 1, 1 | Branch target of 0x04E7DB, 0x04E7F3 and 0x04E81D. The bytes after it include a relative call and must not be extended into. |
| 0x071920 | 1, 2, 1, 3 | Routine entry. |
| 0x0719D0 | 1, 1, 1, 3 | No direct branch target. The last instruction is the return that removes 0xC bytes. |
| 0x0719E5 | 1, 1, 1, 3 | Branch target, and its second byte 0x0719E6 is a separate target of 0x071929. A five-byte overwrite here would split a target and is unsafe. |

The scan covered direct relative branches within each routine. Indirect jump-table targets in the world step were not enumerated, and a window there cannot be fully excluded as a target on this evidence. At every point the native code relies on all general registers and, at 0x09DB55, on flags the relocated instructions recompute. An observer must therefore return with every register and flag as the native code expects.

## Safety limits

- **Concurrent relights on the same columns.** The mesh worker relights under D8 alone. The Bomb and variable-radius relights hold C0 alone. Nothing excludes these from each other, so two threads can be inside 0x19A0E0 at once and write the same columns' work bytes. Their clears can also store into cells the mesh relight is propagating, but clears never resize. The seed reads byte +0 and writes it back with the adjacent byte as one two-byte store (0x19A166–0x19A189), so a concurrent store to +0 in that gap is lost. An ordered trace across threads therefore needs the observer's own global sequence. A byte snapshot at one thread's exit can contain the other thread's partial phase.
- **No observer locking.** The order of the native locks differs between threads. Zone removal enters D8 and then C0, region removal and the main thread enter C0 and then D8, and the mesh worker holds D8 together with the cache lock. An observer that blocks on C0 or D8 at any of these points can deadlock or alter scheduling. The observer must not block, wait or call back into game routines at these points.
- **Zone lifetime.** Departure of a zone or region needs both C0 and D8. While the observed thread holds either section, resident zones present at that point stay present, but newly generated zones can still appear. Publication at 0x1EE05A takes no world lock, so presence recorded at a request can become stale during the propagation that follows. Admission runs with no world lock, so presence observed there can change before the builder enters D8.
- **Allocation lifetime.** Cell-array addresses change only on resize, through the main-thread queued writes and building writes under D8. During a mesh relight they are stable. At main-thread points without D8, the main thread itself is the only resizer.
- **Briefly valid values.** The cache slot pointer is meaningful while the worker holds the cache lock. Mesh marking sets the dirty byte under the slot lock, which the builder does not hold, so only DL at 0x09DB55 is the value the native code used. Argument low X is lost before 0x19A7EE. The ESP offsets apply only at the exact listed instructions. Registers other than those named are scratch.
- **Unpublished zones.** At the new-zone call, the explicit zone pointer is known only to the streaming thread until publication. No world lock covers it, and its contents are not yet visible through the resident tables.

## Unresolved

- Whether world +0xB8 is ever null in ordinary local or online play, and therefore how often the three world-step relights run.
- The meaning of world byte +0xB4 and the full identity of the building-placement branch. The name is a reconstruction lead only.
- Whether any generator path writes into an already published neighbouring zone (alpha-extractor.md, X-7).
- The complete set of producers that fill the client cell queue at client +0x800648.
- Indirect-branch targets inside the world step near the listed world-step points.
- Measured frequencies and actual interleavings of the concurrent writers. Nothing here was observed on a running client.
