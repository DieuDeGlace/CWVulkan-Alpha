# Alpha client frame and world-call ordering

Status: FINAL / STABLE / HELD. This is an independently worded, bounded original-source contract. It establishes the specified client frame's world-call boundary and a later controlled-action caller. It does not certify a replacement implementation, a complete input/movement reconstruction or runtime acceptance.

The evidence root is `disassembled alpha cube world`. The supported image is `binary/Cube.exe`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred image base `0x00400000`. Addresses below are RVAs except explicitly identified object offsets. The principal routine is client frame `0x088EE0`, with aggregate construction at `0x08CE2E–0x08CE35`, world-call splitting at `0x08CEF4–0x08CF49`, and controlled-action caller `0x09BF70–0x09BF87`. Its large original function export times out; the instruction-level original evidence supplies the frame facts here. No other contract, reconstruction, replacement engine, tests, captures or implementation history supplies facts. No server frame counterpart is claimed.

## Elapsed input and the earlier time-scaling flag

The frame copies its first explicit four-byte argument into a local elapsed slot at `0x088F31–0x088F34`. Its split comparison later treats that value as a signed 32-bit integer. The world object used by this frame is the object beginning at client `+0x2E4`; its address is retained before the scoped calls.

An earlier one-byte client flag at `+0x8006E5` can change the elapsed value. When nonzero, the frame divides the signed input by 4, truncating toward zero. It stores that quotient back into the elapsed slot. If the quotient is less than 1, it replaces the stored value with 1 and invokes the native imported wait with argument 5. A quotient equal to 1 is retained without that particular wait. When the flag is zero, this time-scaling region leaves elapsed unchanged. This is a time-scaling/minimum rule; it does not suppress stepping or turn elapsed into zero.

One identified external driver at `0x0C85F0` requires two global object pointers to be nonnull, obtains the original `timeGetTime` result, subtracts a retained four-byte previous tick, and supplies that four-byte difference to virtual slot `+0x28`. If its retained previous tick is zero, it substitutes the current tick before calculating the difference, producing zero for that invocation. The client virtual table has frame `0x088EE0` in slot `+0x28`. The native tick subtraction is four-byte arithmetic; this contract does not reinterpret a difference with its top bit set as a positive signed frame duration. Earlier dispatch into that driver and all external timing conditions remain outside scope.

Sources: `binary/Cube.exe`, frame elapsed capture `0x088F24–0x088F34`, world address retention `0x089020–0x08902F`, scaling/minimum `0x08C100–0x08C134`, driver `0x0C85F0–0x0C8633`; original client virtual-table entry at image RVA `0x2FFDF0` and client constructor table assignment at `0x059CC3`. Original driver prose evidence is in `decompiled/client/functions/004c85f0.c`; instruction ranges are in `decompiled/client/assembly.txt`. The full outer driver/object lifetime is not reconstructed.

## One aggregate, optional connection-associated preparation, then world admission

The frame constructs one output aggregate at `0x08CE35`, using a local object whose base is supplied to native constructor `0x066880`. That constructor initializes thirteen empty native lists. The aggregate is constructed once for this frame boundary, not separately for each world piece. The frame also writes its current four-byte elapsed value to client `+0x8006E8` before the optional preparation and before the world calls.

When the native four-byte pointer at client `+0x8006C8` is nonnull, the frame invokes three helpers on that object, in order: `0x0DD6A0` with the aggregate base, `0x0DD5E0` with aggregate `+0x58`, and `0x0DD640` with aggregate `+0x20`. The first helper transfers queued list content under its own native lock and empties its source list. The other two helpers and the complete meaning/lifetime of that connection-associated object are not reconstructed here. When the pointer is null, the frame skips these three calls and still reaches the following world checks. A nonnull connection-associated pointer is therefore not a requirement for this scoped world-call boundary.

The first world check compares client four-byte field `+0x800A50` with the world value returned by helper `0x0787F0`, which reads world `+0x800164`. The values must be equal. This contract retains them as numeric identity evidence rather than assigning an unproved meaning to that discriminator.

The second check requires a true low-byte result from comparison helper `0x051440` between the string object at client `+0x800A54` and the world string object returned earlier by `0x080DF0`, beginning at world `+0x94`. That comparison uses the native strings' stored lengths and contents; a mismatch skips the world-call region. The complete string comparator's character behavior is an unexpanded dependency. Both checks occur after aggregate construction and any connection-associated preparation.

Failure of either check continues at `0x08D5F0`; it does not return from the entire frame. This contract therefore does not treat skipped world calls as proof that all later input processing is skipped. Full initialization and transition rules for the expected identity and string are missing dependencies.

Immediately before the split, a nonnull controlled-creature pointer at client `+0x8006D0` causes the frame to place controlled-creature `+0x11DC` into field `+0x160` of the separate object pointed to by client `+0x800954`. A null controlled pointer skips only that particular assignment in this block and does not itself bypass the world-call split. The purpose of that recipient field and safety of every subsequent null-controlled-creature route remain unresolved.

Sources: `decompiled/client/functions/00466880.c`, `004dd6a0.c`, `004787f0.c`, `00480df0.c`, `00451440.c`, `0040c520.c`; original `binary/Cube.exe`, aggregate/current elapsed `0x08CE2E–0x08CE5A`, optional helpers `0x08CE5C–0x08CE99`, identity/string checks `0x08CE9D–0x08CEC9`, controlled-pointer-dependent assignment `0x08CECF–0x08CEEE`. The comparator's unexpanded byte-comparison helper is original entry `0x00C590`.

## Positive elapsed: piece count, exact multiples and shared arguments

For a current signed elapsed value from 1 through 20 inclusive, the frame makes one world call with that value. At exactly 20 it does not enter the repeated-call region.

For a current signed value greater than 20, the frame makes consecutive 20-unit world calls and then one final world call. The number of preceding 20-unit calls is the integer part of elapsed minus 1 divided by 20. The final value is the original current elapsed minus 20 times that preceding-call count, and is always from 1 through 20 inclusive. The total number of calls is elapsed divided by 20 rounded upward. This is mathematical description of the observed native arithmetic, not an independent timestep policy.

| Current elapsed | World-call values, in order |
| --- | --- |
| 1 | 1 |
| 20 | 20 |
| 21 | 20, 1 |
| 39 | 20, 19 |
| 40 | 20, 20 |
| 41 | 20, 20, 1 |
| 60 | 20, 20, 20 |

Thus an exact positive multiple ends with a final 20-unit call; it does not add a final zero-unit call. Every repeated call at `0x08CF2E` returns before the next begins, and every repeated call finishes before the final call at `0x08CF49`.

All calls use the same world object, entry `0x20C510`, and the same aggregate base constructed for this frame. Each supplies three explicit arguments: the elapsed piece, that aggregate pointer, and a null auxiliary pointer. The auxiliary argument is null for both repeated and final calls. There is no aggregate reconstruction or controlled primary-start call between these pieces in the audited caller. World-internal actor phases, callbacks, list mutation and reentrancy remain callee dependencies.

Sources: original `binary/Cube.exe`, signed threshold `0x08CEF4–0x08CEF9`, positive piece-count/remainder arithmetic `0x08CEFB–0x08CF17`, repeated calls and termination `0x08CF1B–0x08CF34`, final call `0x08CF36–0x08CF49`. World entry captures its elapsed and aggregate arguments at `0x20C549–0x20C564`. Original corresponding instructions are in `decompiled/client/assembly.txt`.

## Zero, negative and pause/connection limits

There is no positive-elapsed admission test at this split. If the current stored elapsed value is zero, the signed comparison takes the single final-call path and supplies zero to the world routine. A negative signed value likewise takes that path and is supplied unchanged. These statements apply after the earlier scaling/minimum rule: when that flag is nonzero, zero or negative input is changed to 1 before the split. They also assume that execution reaches and passes the identity/string checks.

A zero call is still a native function invocation with the same aggregate and null auxiliary pointer. This contract does not assert that world processing is a no-op for zero, or that negative elapsed is a supported gameplay state. Full arithmetic and side effects inside the world routine for those inputs are unexpanded dependencies.

At frame entry, nonzero client byte `+0x800585` together with zero client four-byte field `+0x8006CC` invokes helper `0x0719F0` before the scoped work. Its cleanup consequences are not reconstructed here. The identified external driver and the aggregate-to-split region contain no additional direct pause Boolean admission test. The optional pointer and numeric/string checks above are the confirmed tests at this boundary. No claim that every paused, connecting, disconnected or modal-UI state reaches this boundary is supplied: earlier dispatch, helper side effects, expected-world transitions and the meanings of unnamed client flags remain missing dependencies.

Sources: original `binary/Cube.exe`, entry cleanup condition `0x088F2A–0x088F4A`, scaling `0x08C100–0x08C134`, aggregate admission and signed split `0x08CE2E–0x08CF49`; `decompiled/client/functions/004c85f0.c` and the matching original driver instructions. This contract does not rename `+0x8006E5` as a pause flag.

## Controlled-action input follows the scoped world calls

The requested ordinary primary-start helper call is at `0x09BF82`, later in the same frame than both world-call sites. Its object is the controlled creature reloaded from client `+0x8006D0`; it receives no elapsed argument. All world pieces in this caller have already finished before this site can run. The caller does not invoke that helper once per world piece.

The immediate caller requires the controlled creature's action byte `+0x68` to differ from decimal 48 and client input byte `+0x04` to be nonzero. The path completing the preceding action-slot iteration also requires its local one-byte handled/suppression value to be zero at `0x09BF69`. A short-collection path can arrive directly at `0x09BF70`, after that local was reset to zero earlier. The action-slot iteration can instead set that local to 1, suppressing the primary-start call.

There are broader caller gates before that iteration: a nonzero local inhibition byte at `0x09B58E`, or nonzero client byte `+0x800704`, skips the whole region to `0x09C02F`. The earlier local inhibition value includes a client virtual query result stored at `0x091F54`; the complete evolution of that local across intervening UI/input regions is not reconstructed here. Those bytes must not be treated as universal pause or connection flags from this bounded evidence alone.

The helper's own roll/action/readiness/mana admission and selected-action behavior are separate actor dependencies; entering this caller does not guarantee that an action starts. Its original body is identified as `0x045410`, but this contract does not reconstruct that body or import facts from another artifact.

The frame also contains later control-dependent vector operations, for example the client-byte `+0x0B/+0x0C` tests and controlled-creature `+0x160` vector updates at `0x09B1C9–0x09B2C8`. Their position after the world calls is established, but their full movement-versus-aim/camera meaning is not established here. Ordinary controlled movement input, device polling, every alternate action route, earlier per-frame actor writes, and world-internal movement/physics have not been comprehensively traced. The proven ordering is therefore the specified later primary-start site and those identified later control operations after all scoped world pieces; a claim about every movement/action input route remains unresolved.

Sources: original `binary/Cube.exe`, broader input gates `0x09B58E–0x09B5B8`, local reset and short-collection route `0x09B5BE–0x09B617`, handled/suppression write `0x09B635–0x09B646`, final iteration and immediate primary gate/call `0x09BF4D–0x09BF87`; earlier virtual result `0x091F44–0x091F54`; later control/vector operations `0x09B1C9–0x09B2C8`; `decompiled/client/assembly.txt`. Full actor/input and pause-state closure, rather than guessed ordering beyond these anchors, remains the explicit limit.
