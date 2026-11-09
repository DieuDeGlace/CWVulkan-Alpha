# Alpha terrain candidate sort callable boundary

Status: FINAL / STABLE / HELD. This is a code-free ABI and execution-dependency contract for original client RVA `0x0ABAE0`. It describes ordinary valid calls and separately identifies security-failure dependencies. It does not certify a particular isolated execution environment or supply a substitute sort.

The relative evidence root is `disassembled alpha cube world`. The supported image is `binary/Cube.exe`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, a PE32 x86 image with preferred base `0x400000`. All routine and global addresses below are RVAs. This audit used original executable bytes and individually identified original function/assembly exports only. No other contract, replacement implementation, runtime execution or probe supplied evidence.

## Public call boundary within this image

The wrapper uses the 32-bit x86 caller-cleanup convention conventionally called cdecl. It takes three four-byte stack arguments. There is no instance pointer supplied through ECX and no register argument required by this wrapper.

| Argument in source order | Offset from stack pointer on wrapper entry, including the return address | Width and role |
| --- | --- | --- |
| First | `+0x04` | 32-bit pointer to the first candidate record |
| Second | `+0x08` | 32-bit pointer immediately after the last candidate record |
| Third | `+0x0C` | Opaque 32-bit context value, forwarded through internal helpers |

The original terrain caller supplies these three words and removes 12 argument bytes after the call. The wrapper returns without removing its caller's arguments. It separately removes the 16 bytes of arguments it supplied to its own internal sort helper.

The wrapper has no defined result or status value. It does not assign a success indicator or output pointer before return, and the original terrain caller does not consume a result. EAX's remaining contents depend on internal work and must not be treated as a return contract. The observable result is the in-place record order.

The ordinary call graph preserves EBX, ESI, EDI and EBP and restores its stack frames. EAX, ECX, EDX, condition flags and XMM temporaries are volatile. The routine is a 32-bit native boundary; no 64-bit calling convention follows from it.

Sources: `binary/Cube.exe`, wrapper RVAs `0x0ABAE0`–`0x0ABB0D`, caller `0x0B0E42`–`0x0B0E8D`; `decompiled/client/functions/004abae0.c`; main sort helper `004aa580.c`. All shortened function filenames below are relative to `decompiled/client/functions/`.

## Range and record memory

The wrapper derives a signed native count from the byte span between its two pointers using 48-byte record stride. Ordinary valid input has an ordered range, a whole-record extent and a nonnegative span representable in the native signed arithmetic. There is no explicit validation of pointer accessibility, record boundaries, reversed ranges, arithmetic wrap or allocation ownership.

Records are caller-owned contiguous storage. The sort writes that storage in place; it does not require a vector header, capacity field, allocator object or container constructor before the first record. The wrapper's two pointers delimit the records themselves.

| Record bytes | Treatment by this sort |
| --- | --- |
| `+0x00`–`+0x17` | 24 payload bytes, carried without interpreting their numeric contents |
| `+0x18`–`+0x27` | 16 further payload bytes, likewise carried without a comparison criterion |
| `+0x28`–`+0x2B` | Binary32 sorting key, compared numerically and carried with the payload |
| `+0x2C`–`+0x2F` | Four padding bytes outside the inspected meaningful copies/swaps |

The first 44 bytes of each record move together. The physical extent remains 48 bytes per record, but a permutation of all 48 bytes is not promised: the movement helpers do not carry the final padding word as part of the meaningful record. The wrapper does not inspect the payload's original position/radius/color semantics or follow pointers contained in it. Ordinary finite comparable key values are the relevant numeric precondition.

An empty range or a one-record range takes the main helper's no-reordering return, without accessing candidate payloads or reaching the protected sorting helpers' cookie loads. For larger valid ranges, all meaningful record bytes used for movement must be readable and writable. Invalid addresses are not converted into a defined error return.

The inspected memory operations impose no explicit 16-byte record-alignment check and contain no aligned 128-bit memory load/store requirement. They use ordinary native scalar and unaligned-capable XMM transfers. This is not a certification of arbitrary page-crossing or invalid memory layouts. A normal valid x86 call stack with sufficient writable space remains necessary.

Sources: `004abae0.c`; `004aa580.c`; record swap `004abb10.c`; insertion and shift helpers `004a9520.c` and `004aa0d0.c`; partition `004ab190.c`; heap helpers `004a9a00.c`, `004aa840.c`, `004a9200.c` and `004aa160.c`. Original assembly corroborates semantic transfers through record `+0x28`, excluding the final padding word.

## Complete ordinary helper dependency boundary

For ordinary valid inputs, the wrapper's closed transitive graph consists of the internal main sorter, partition and sample selection, record movement, insertion ordering, heap helpers and the image's security-cookie checker. The identified routine RVAs are:

| Role | Native RVAs |
| --- | --- |
| Wrapper and main helper | `0x0ABAE0`, `0x0AA580` |
| Partition and sample selection | `0x0AB190`, `0x0A9F00` |
| Exchange and insertion movement | `0x0ABB10`, `0x0A9520`, `0x0AA0D0` |
| Heap construction/extraction and adjustment | `0x0A9A00`, `0x0AA840`, `0x0A9200`, `0x0AA160` |
| Matching-cookie check | `0x28D788` |

This ordinary graph contains no allocation/deallocation, imported call, indirect callback, lock acquisition, file/network access, timer sampling, random draw, device access or application/world/window object access. It contains no grow/reallocate path and does not free the supplied range. Temporary meaningful records reside on the call stack; partitioning also uses recursive calls, so stack capacity is a caller/environment prerequisite rather than a heap dependency.

The third wrapper argument is copied and forwarded, but no inspected helper validates or dereferences it, invokes it as a callback or uses it as a secondary comparison criterion. The original caller's client pointer is therefore not evidence that a constructed client, world or window object is required by the sort. No live object behind that value is a demonstrated prerequisite in this ordinary graph.

The sort uses SSE comparisons and SSE2 XMM transfers, including in the insertion route. A compatible x86/SSE2 execution environment and native floating-point comparison state are required. The inspected graph does not initialize floating-point control state, install a comparison callback or save/restore XMM temporaries. Exceptional/unmasked floating-point behavior, subnormal handling under altered control modes and invalid-key behavior are not certified here.

Sources: the eleven sort routines listed above in `decompiled/client/assembly.txt` and their individually named function exports; `0068d788.c`. Native insertion transfers at `0x0A955A`–`0x0A959A` demonstrate SSE2 use. The ordinary graph's only global data references are the protected helpers' security-cookie loads and the matching checker described next.

## Global state on success and security-failure paths

The ordinary protected helpers read the four-byte image security cookie at RVA `0x36AA78`, combine it with their stack frame value and compare it through RVA `0x28D788` before returning. The checker compares against the same global and returns directly on equality. That equality path does not use an IAT entry.

No writable global is required by the ordinary sorting work itself. The cookie must be readable and remain consistent while any protected helper is active; changing it during an active call can produce a mismatch. The image's initial stored cookie is `0xBB40E64E`; its adjacent complement at `0x36AA7C` initially contains `0x44BF19B1`. Ordinary successful checks do not read that complement.

The process entry normally initializes the cookie through RVA `0x28E7A1`, which uses operating-system entropy sources and writes the cookie/complement. That initializer is not called from this sort graph. The sort's static matching requirement does not independently require that the application entry or cookie initializer has run: a consistent readable cookie is sufficient for these inspected comparisons. This is a dependency finding, not an assurance of the security properties or complete behavior of an environment that omits normal runtime startup.

Cookie mismatch transfers to RVA `0x28DBAB`. That failure route queries a processor feature and can invoke native fast-fail interrupt `0x29` with reason 2. Its fallback records exception/context information in image globals, including the exception fields beginning at `0x36DF60` and context fields at `0x36DFB0` through the inspected writes at `0x36E078`. It also reads the cookie/complement and reaches the failure raiser at `0x28DB6E`, which writes the debugger-state global at `0x36E27C`.

Thus writable diagnostic globals and runtime/operating-system services are dependencies of failure reporting, not ordinary successful sorting. The inspected failure service is intended to terminate/fail the process, not return a sortable error value to the wrapper's caller.

Sources: `0068d788.c`, `0068e7a1.c`, `0068dbab.c` and `0068db6e.c`; `binary/Cube.exe`, checker RVAs `0x28D788`–`0x28D792`, cookie data `0x36AA78`–`0x36AA7F`, failure reporting `0x28DBAB`–`0x28DCA3` and raising `0x28DB6E`–`0x28DBAA`.

## Imported services and exception limits

The successful ordinary graph has no imported-routine/IAT call dependency once executable code and required data are correctly present at their native addresses. The mismatch-reporting graph does have these independently identified imports:

| Failure-route IAT RVA | Original DLL and imported routine |
| --- | --- |
| `0x2FC0C8` | `KERNEL32.dll`, `IsProcessorFeaturePresent` |
| `0x2FC068` | `KERNEL32.dll`, `IsDebuggerPresent` |
| `0x2FC558` | `MSVCR110.dll`, `_crt_debugger_hook` |
| `0x2FC380` | `MSVCR110.dll`, `__crtUnhandledException` |
| `0x2FC384` | `MSVCR110.dll`, `__crtTerminateProcess` |

The last service receives status `0xC0000409` from the inspected failure raiser. Imported service implementations, debugger/operating-system policy and fast-fail behavior are external dependencies; their complete execution is not reconstructed.

The ordinary sort graph contains no C++ throw call, exception-allocation routine, exception-handler registration or catch/recovery boundary. The wrapper supplies no rollback guarantee if memory access, stack exhaustion, a floating-point exception or another abnormal execution condition interrupts its in-place movements. Security-failure reporting is distinct from a recoverable C++ sorting exception.

The executable's normal loader still resolves its PE imports when loading the complete original image. Absence of an ordinary sort IAT call does not imply that the whole executable can be normally loaded or launched without its imported DLLs. Correct image mapping/relocation, executable code, readable cookie storage, writable record/stack memory and a suitable native thread environment remain prerequisites. This document gives no binary extraction, loader or invocation procedure.

Sources: `binary/Cube.exe`, original import descriptors/name tables and IAT; thunk RVAs `0x2CF8C2`, `0x28E5F2`, `0x28E5F8`, `0x28E5FE`; `006cf8c2.c`; failure raiser `0068db6e.c`; all ordinary helper bodies listed above.

## Initialization conclusion and explicit limits

No prior game-client construction, world initialization, graphics device, terrain cache, registered window class, window handle, audio/input initialization or game asset load is an observed dependency of ordinary valid finite-record sorting through this wrapper. Its context word is not a route to those objects in the inspected graph. The required mutable work is in the caller's record storage and stack; the successful runtime dependency is a stable readable image cookie when protected helpers are reached.

This is a static original-evidence conclusion. It does not certify an isolated host, absent loader/runtime setup, a particular floating-point environment, concurrent cookie mutation, overlapping calls on the same records, malformed ranges, abnormal termination, or all alternate callers. The complete arbitrary larger mixed-tie output permutation is also outside this ABI contract. Earlier artifacts remain unopened and unchanged.
