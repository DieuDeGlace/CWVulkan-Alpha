# Alpha terrain candidate sort callable boundary

Status: FINAL / STABLE / HELD. This is a code-free ABI and execution-dependency contract for original client RVA `0x0ABAE0`. It describes ordinary valid calls and separately identifies security-failure dependencies. It does not certify a particular isolated execution environment or supply a substitute sort.

The relative evidence root is `disassembled alpha cube world`. The supported image is `binary/Cube.exe`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, a PE32 x86 image (machine type `0x14C`) with preferred base `0x400000` and entry RVA `0x28E1E0`. All routine and global addresses below are RVAs. This audit used original executable bytes and individually identified original function/assembly exports only. No other contract, replacement implementation, runtime execution or probe supplied evidence.

## Public call boundary within this image

The wrapper uses the 32-bit x86 caller-cleanup stack convention (cdecl). It takes three four-byte stack arguments. It reads no incoming register value: ECX and EDX are only used as scratch after being loaded from the stack, and no instance pointer is expected in ECX.

| Argument in source order | Offset from stack pointer on wrapper entry, including the return address | Width and role |
| --- | --- | --- |
| First | `+0x04` | 32-bit pointer to the first candidate record |
| Second | `+0x08` | 32-bit pointer immediately after the last candidate record |
| Third | `+0x0C` | Opaque 32-bit context value, forwarded through internal helpers |

The only call to the wrapper in this image is at `0x0B0E84`, inside the terrain routine that begins at `0x0AC260`. That caller pushes the three words and removes 12 argument bytes after the call. The wrapper returns with a plain near return and does not remove its caller's arguments. It passes four words to its main internal helper (the two range pointers, the record count it computed and the context value) and removes those 16 bytes itself.

The wrapper has no defined result or status value. It does not assign a success indicator or output pointer before return, and the original caller does not read EAX after the call. EAX's remaining contents depend on internal work and must not be treated as a return contract. The observable result is the in-place record order.

The ordinary call graph preserves EBX, ESI, EDI and EBP and restores its stack frames; every helper that uses EBX, ESI or EDI saves and restores it. EAX, ECX, EDX, condition flags and XMM temporaries are volatile. The routine is a 32-bit native boundary; no 64-bit calling convention follows from it.

Sources: `binary/Cube.exe`, wrapper RVAs `0x0ABAE0`–`0x0ABB0D`, caller `0x0B0E42`–`0x0B0E8D`; `decompiled/client/functions/004abae0.c`; main helper `004aa580.c` (`0x0AA580`–`0x0AA6B9`). All shortened function filenames below are relative to `decompiled/client/functions/`.

## Range and record memory

The wrapper derives a signed native count by dividing the byte span between its two pointers by the 48-byte record stride, with the division truncating toward zero. Ordinary valid input has an ordered range, a whole-record extent and a nonnegative span representable in the native signed arithmetic. There is no explicit validation of pointer accessibility, record boundaries, reversed ranges, arithmetic wrap or allocation ownership.

Records are caller-owned contiguous storage. The sort writes that storage in place; it does not require a vector header, capacity field, allocator object or container constructor before the first record. The wrapper's two pointers delimit the records themselves.

| Record bytes | Treatment by this sort |
| --- | --- |
| `+0x00`–`+0x17` | 24 payload bytes, moved without interpreting their contents |
| `+0x18`–`+0x27` | 16 further payload bytes, likewise moved without a comparison role |
| `+0x28`–`+0x2B` | Binary32 sorting key, compared numerically and moved with the payload |
| `+0x2C`–`+0x2F` | Four trailing bytes, not part of any identified record transfer or comparison |

The first 44 bytes of each record move together as one unit. The physical extent remains 48 bytes per record, but a permutation of all 48 bytes is not promised: the exchange, shift, insertion and heap transfers end at offset `+0x2B`, and the trailing four bytes are not carried with their record. The sort does not follow pointers contained in the payload. Ordinary finite comparable key values are the relevant numeric precondition.

## Ordering direction and size-dependent routes

Every comparison in the graph is a scalar single-precision ordered comparison of two keys at record offset `+0x28`, and each decision asks only whether one key is strictly less than another. The completed order is ascending: after a successful call, each record's key is not greater than the key of the record that follows it. Equal keys are not separated by any secondary criterion, and the relative order of records with equal keys is not promised.

A range of zero or one record takes the main helper's immediate return, without accessing record memory or reaching any protected helper's cookie load. A range of two to 32 records goes directly to the insertion route. A range of more than 32 records is partitioned repeatedly under a depth budget that starts at the record count and is reduced, at each partition step, to its integer half plus the integer half of that half. Pieces of 32 or fewer records are left for a final insertion pass on that piece; if the budget falls below one while a piece still has more than 32 records, that piece is ordered by the heap helpers instead. These routes affect only performance and the arrangement of equal keys; the ascending result is the same.

For ranges of two or more records, all meaningful record bytes used for movement must be readable and writable. Invalid addresses are not converted into a defined error return.

The inspected memory operations impose no 16-byte record-alignment check and contain no aligned 128-bit memory load or store. They use ordinary 32-bit scalar moves, scalar single-precision XMM moves and 64-bit XMM moves, none of which requires alignment. This is not a certification of arbitrary page-crossing or invalid memory layouts. A normal valid x86 call stack with sufficient writable space remains necessary.

Sources: `004abae0.c`; `004aa580.c`; record exchange `004abb10.c`; insertion and shift helpers `004a9520.c` and `004aa0d0.c`; partition `004ab190.c`; sample ordering `004a9f00.c`; heap helpers `004a9a00.c`, `004aa840.c`, `004a9200.c` and `004aa160.c`. Original assembly for these routines shows the exchange, shift, insertion and heap record transfers ending with the key word at offset `+0x28`, and every key comparison addressing offset `+0x28`.

## Complete ordinary helper dependency boundary

For ordinary valid inputs, the wrapper's closed transitive graph consists of the main helper, partition and sample ordering, record exchange, insertion ordering, heap helpers and the image's security-cookie checker. The identified routine RVAs are:

| Role | Native RVAs |
| --- | --- |
| Wrapper and main helper | `0x0ABAE0`, `0x0AA580` |
| Partition and sample ordering | `0x0AB190`, `0x0A9F00` |
| Exchange and insertion movement | `0x0ABB10`, `0x0A9520`, `0x0AA0D0` |
| Heap construction/extraction and adjustment | `0x0A9A00`, `0x0AA840`, `0x0A9200`, `0x0AA160` |
| Matching-cookie check | `0x28D788` |

Every call instruction in these eleven sort routines targets another routine in this table; there is no indirect call. The graph contains no allocation or deallocation, imported call, callback, lock acquisition, file or network access, timer sampling, random draw, device access or application/world/window object access. It contains no grow or reallocate path and does not free the supplied range. Temporary record copies reside in the helpers' stack frames. The main helper calls itself on one side of each partition and loops on the other, so stack capacity is a caller/environment prerequisite rather than a heap dependency.

The third wrapper argument is copied and passed down through the main helper, partition, insertion and heap helpers, but no routine in the graph dereferences it, compares it, calls through it or uses it as a secondary ordering criterion. No live object behind that value is a demonstrated prerequisite in this ordinary graph.

The sort uses SSE scalar comparisons and SSE2 64-bit XMM transfers, including in the insertion route. A compatible x86/SSE2 execution environment and native floating-point comparison state are required. The inspected graph does not initialize floating-point control state, install a comparison callback or save and restore XMM temporaries. Exceptional or unmasked floating-point behavior, subnormal handling under altered control modes and invalid-key behavior are not certified here.

Sources: the eleven sort routines listed above in `decompiled/client/assembly.txt` and their individually named function exports; `0068d788.c`. Native insertion transfers at `0x0A955A`–`0x0A959A` demonstrate SSE2 use. The ordinary graph's only global data reference is the image security cookie, read by the protected helpers and by the matching checker described next.

## Global state on success and security-failure paths

Four helpers are protected: partition `0x0AB190`, insertion `0x0A9520`, and heap helpers `0x0A9A00` and `0x0AA840` load the four-byte image security cookie at RVA `0x36AA78` on entry, combine it with their frame pointer, and pass the recombined value to RVA `0x28D788` before returning. The checker compares that value against the same global and returns directly on equality. That equality path does not use an IAT entry. The wrapper, main helper, sample ordering, exchange, shift and heap-adjustment helpers do not read the cookie.

No writable global is required by the ordinary sorting work itself. The cookie must be readable and remain unchanged while any protected helper is active; changing it during an active call can produce a mismatch. The image's initial stored cookie is `0xBB40E64E`; its adjacent complement at `0x36AA7C` initially contains `0x44BF19B1`. Ordinary successful checks do not read that complement.

The process entry at `0x28E1E0` normally initializes the cookie through RVA `0x28E7A1`, which draws on operating-system time, process/thread identity and counter sources and writes the cookie and complement. That initializer is not called from this sort graph. The sort's matching requirement does not independently require that the application entry or cookie initializer has run: a consistent readable cookie is sufficient for these comparisons. This is a dependency finding, not an assurance of the security properties or complete behavior of an environment that omits normal runtime startup.

Cookie mismatch transfers to RVA `0x28DBAB`. That failure route queries processor feature 23 and, when it is present, raises native fast-fail interrupt `0x29` with reason 2. Its fallback records exception and context information in image globals, including the exception fields beginning at `0x36DF60` and context fields from `0x36DFB0` through the writes at `0x36E078`. It also reads the cookie and complement and reaches the failure raiser at `0x28DB6E`, which writes the debugger-state global at `0x36E27C`.

Thus writable diagnostic globals and runtime/operating-system services are dependencies of failure reporting, not of ordinary successful sorting. The failure route is intended to terminate the process, not to return a sortable error value to the wrapper's caller.

Sources: `0068d788.c`, `0068e7a1.c`, `0068dbab.c` and `0068db6e.c`; `binary/Cube.exe`, checker RVAs `0x28D788`–`0x28D796`, cookie data `0x36AA78`–`0x36AA7F`, failure reporting `0x28DBAB`–`0x28DCA3` and raising `0x28DB6E`–`0x28DBAA`.

## Imported services and exception limits

The successful ordinary graph has no imported-routine/IAT call dependency once executable code and required data are correctly present at their native addresses. The mismatch-reporting graph does have these imports, each confirmed against the image's import descriptors and name tables:

| Failure-route IAT RVA | Original DLL and imported routine |
| --- | --- |
| `0x2FC0C8` | `KERNEL32.dll`, `IsProcessorFeaturePresent` |
| `0x2FC068` | `KERNEL32.dll`, `IsDebuggerPresent` |
| `0x2FC558` | `MSVCR110.dll`, `_crt_debugger_hook` |
| `0x2FC380` | `MSVCR110.dll`, `__crtUnhandledException` |
| `0x2FC384` | `MSVCR110.dll`, `__crtTerminateProcess` |

The last service receives status `0xC0000409` from the failure raiser. Imported service implementations, debugger/operating-system policy and fast-fail behavior are external dependencies; their complete execution is not reconstructed.

The ordinary sort graph contains no C++ throw call, exception-allocation routine, exception-handler registration or catch/recovery boundary. The wrapper supplies no rollback guarantee if memory access, stack exhaustion, a floating-point exception or another abnormal execution condition interrupts its in-place movements. Security-failure reporting is distinct from a recoverable C++ sorting exception.

The executable's normal loader still resolves its PE imports when loading the complete original image. Absence of an ordinary sort IAT call does not imply that the whole executable can be normally loaded or launched without its imported DLLs. Correct image mapping/relocation, executable code, readable cookie storage, writable record/stack memory and a suitable native thread environment remain prerequisites. This document gives no binary extraction, loader or invocation procedure.

Sources: `binary/Cube.exe`, original import descriptors/name tables and IAT; jump thunks at `0x2CF8C2` (to `0x2FC0C8`), `0x28E5F2` (to `0x2FC558`), `0x28E5F8` (to `0x2FC380`) and `0x28E5FE` (to `0x2FC384`); the raiser calls `0x2FC068` directly; `006cf8c2.c`; failure raiser `0068db6e.c`; all ordinary helper bodies listed above.

## Initialization conclusion and explicit limits

No prior game-client construction, world initialization, graphics device, terrain cache, registered window class, window handle, audio/input initialization or game asset load is an observed dependency of ordinary valid finite-record sorting through this wrapper. Its context word is not a route to those objects in the inspected graph. The required mutable work is in the caller's record storage and stack; the successful runtime dependency is a stable readable image cookie when protected helpers are reached.

This is a static original-evidence conclusion. It does not certify an isolated host, absent loader/runtime setup, a particular floating-point environment, concurrent cookie mutation, overlapping calls on the same records, malformed ranges or abnormal termination. Earlier artifacts remain unopened and unchanged.

## Unresolved

- What the third argument's value is in the original caller. The caller loads it through a separate accessor call before the sort; this audit did not identify that value, and nothing here depends on it. An earlier description of it as a client pointer is withdrawn as unconfirmed.
- The meaning of the 40 payload bytes and of the trailing four bytes of each record (for example, any position, size or color interpretation). The sort treats them as opaque; their meaning is outside this evidence.
- The exact final arrangement of records with equal keys, and of any keys that compare unordered, for ranges that reach the partition or heap routes. Only the ascending key order of finite keys is specified.
- Whether the partition helper's inline transfers ever touch the trailing four bytes of a record. The other movement helpers do not; the final content of those bytes at each position is therefore not specified.
- Behavior with non-finite keys, altered floating-point control state or unmasked floating-point exceptions.

## Re-certification (2026-11-22)

A fresh source-only author re-checked this document against the original `binary/Cube.exe` image (hash, header, cookie bytes, thunks and import tables) and against the original function and assembly exports for every routine named here. The RVAs, image identity, calling convention, argument offsets, stack cleanup, 48-byte stride, 44-byte moved extent, key offset and width, routine list and failure-route imports were confirmed. Corrections: the wrapper's sole call site is now identified; recursion is attributed to the main helper rather than to partitioning; the protected helpers are named and the checker range end is corrected to `0x28D796`; the previously unstated ascending, strict-less ordering and the 32-record route thresholds were added from the evidence. The description of the third argument as a client pointer, the record field semantics and the final content of the trailing record bytes were moved to the unresolved section. Open items remain the third argument's identity, record field meaning, the trailing bytes, tie and unordered-key arrangement, and non-finite or altered floating-point behavior.
