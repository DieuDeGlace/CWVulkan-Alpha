# Alpha element-2 projectile contact event and later kind-3 conversion

Status: FINAL / STABLE / HELD. This is an independently worded, bounded source-side behavioral contract. It does not certify a replacement implementation or runtime acceptance.

The evidence root is `disassembled alpha cube world`. The supported original image is `binary/Cube.exe`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, with preferred image base `0x00400000`. Addresses below are RVAs unless explicitly identified as object/record offsets. The scope is the first projectile element-2 event route at `0x221D03–0x221E28`, its relationship predicate, event construction and append/application boundary, and the later kind-1/element-2 conversion at `0x222B35–0x222B92`. Numeric kind and element identifiers are retained without assigning unproved gameplay names. No other contract, reconstruction, implementation, runtime capture, test, or implementation history supplies facts here. No server counterpart or complete projectile reconstruction is claimed.

## Object identities and admission

The projectile has a 64-bit source identifier in its first eight bytes. The surrounding original code performs separate world lookups using that identifier. The earlier cached-source result, obtained through `0x221476`/`0x221486`, is used by this event route. A later hit-source result at `0x22156C` supplies the earlier critical calculation and some prior candidate checks. These results are not interchangeable by assumption: this contract preserves the actual pointer and identifier used at each operation.

The candidate is the creature selected by the surrounding native scan. Its position object begins at creature `+0x10`, its identifier occupies creature `+0x08` through `+0x0F`, and its control discriminator is one byte at `+0x60`. The projectile element discriminator is one byte at projectile `+0x64`; its kind is a 32-bit discriminator at `+0x60`. World mode `+0xB4` is one byte, and world `+0xB8` is a native 32-bit distinguished-creature pointer.

After the earlier amount calculation/clamp, element byte 2 selects this separate event route. It requires a nonnull cached source. The relationship helper at `0x196CA0` receives **cached source as its first explicit stack argument and candidate as its second**. The caller places the candidate first in the descending stack, then the cached source; the callee reads the cached source at frame `+0x08` and candidate at `+0x0C`. The world object and the later hit-source pointer are not either of those relationship arguments. The helper reads its two explicit pointers, not an implicit world pointer.

Only a false low-byte relationship result permits event construction. The candidate control byte must then be different from 6. This is an additional caller test: the relationship helper itself returns false for a candidate with control 6, but that false result does not admit the event because of this subsequent exclusion.

No new pointer-self exclusion or cached-source class/specialization test occurs inside this route. Earlier candidate health, geometry, lock, timing, and source-dependent admission remain separate prerequisites. A null later hit source does not itself reject this route when the cached source exists; a null cached source does. No event amount minimum is tested here.

Every element-2 outcome, including failed cached-source/relationship/control admission, bypasses the first generic hit call at `0x221E8C`. Failure here is not a fallback to that generic hit routine.

Sources: `binary/Cube.exe`, cached/hit lookups `0x221465–0x22148B` and `0x221561–0x22157A`, candidate-position identity `0x221674–0x221681`, element/source/relationship/control admission `0x221D03–0x221D33`, element-2 continuation `0x221E19–0x221E28`; `decompiled/client/functions/00596ca0.c`; original corresponding ranges in `decompiled/client/assembly.txt`.

## Relationship predicate, with argument order preserved

The following rules describe `0x196CA0` for its actual first and second pointers. In this call they are cached source and candidate, respectively. The returned contract is the low byte; other result-register bits are not a Boolean result guarantee.

The predicate first returns false when the first creature's control byte is 5 and the second's is 0, when the first control byte is 6, or when the second control byte is 6. These early false cases take precedence over its later classification and flag checks.

Outside those exceptions, having control byte 1 on exactly one of the two creatures produces true. When both controls are 1, the helper compares their one-byte classifications from `0x044680`; unequal classifications produce true. It computes the first creature's classification before the second's. When that first comparison is equal, the native routine can repeat the same first-then-second classification pair through its second control-1 test before reaching its final flag test. The classification helper has no chance draw or mutation.

Classification byte 1 requires both of these conditions: the creature's 16-bit flags beginning at `+0x7E` have none of mask `0x1A00` set, and its 32-bit discriminator at `+0x64` is one of **19, 20, 22, 23, 25, 26, 27, 30, 31, 32, 33, 34, 35, 53, 55, 56, 57, 58, 67, 68, 74, 87, 89, 92, 93, 98, 100, 106, 107, 145, 146, 147**. Otherwise the classification's low byte is 0. Gameplay meanings for that classification and discriminator set are not inferred here.

After the applicable control/classification checks, the relationship helper returns true if bit `0x20` is set in the one-byte flags at creature `+0x124` on either argument; otherwise it returns false. Thus swapping the two creature arguments is not authorized by the contract: the early control-5/control-0 case is ordered. Neither helper checks resident buffs, remaining buff time, source identifiers, mana, class at `+0x140`, or specialization at `+0x141` for this relationship result.

Sources: `decompiled/client/functions/00596ca0.c`, `00444680.c`; `binary/Cube.exe`, relationship body `0x196CA0–0x196D2A`, including explicit stack reads `0x196CA4–0x196CA8`, ordered early case `0x196CAB–0x196CCA`, classification call order `0x196CCD–0x196D07`, and final flag test `0x196D09–0x196D19`; classification routine at `0x044680`; matching original assembly.

## Signed amount and event fields

The input to this route is the earlier binary32 candidate amount. That amount originated at projectile `+0x48` and was doubled by binary32 multiplication when the earlier critical Boolean, calculated from the later hit source, was true. The first-path amount clamp replaces an ordered negative amount with binary32 positive zero. It does not clamp a positive value downward or supply a finite-value substitute for an unordered comparison.

The event producer toggles the amount's sign bit using the native sign mask. It then multiplies the signed result by exact binary32 0.5 **only when the cached source's control byte at `+0x60` equals 1**. The tested creature is the cached source, not the candidate or the later hit source. Every other cached-source control value that reaches this point retains the full sign-toggled amount.

For an ordinary finite positive input, the event therefore contains a negative amount, with half magnitude for cached-source control 1 and full magnitude otherwise. A clamped or positive-zero input becomes negative zero, and zero is not filtered out before append. The operation is a sign-bit toggle rather than an absolute-value operation. Non-finite input interpretation beyond the observed operations remains unresolved.

The native event constructor at `0x059A30` initializes the fields below; the caller then fills the identifiers, amount, critical byte, and candidate position. The payload occupies 72 bytes inside a native 80-byte list node with two four-byte links. Its copy helper copies semantic fields individually rather than copying record padding as a 72-byte block.

| Event-relative offset | Width | Value produced on this route |
| --- | --- | --- |
| `+0x00` | 64-bit identifier | Copied from projectile `+0x00`/`+0x04`. This is the projectile's stored source identifier, not a fresh copy from either source creature pointer. |
| `+0x08` | 64-bit identifier | Candidate creature identifier copied from candidate `+0x08`/`+0x0C`. |
| `+0x10` | Binary32 | Sign-toggled candidate amount, subsequently multiplied by 0.5 only for cached-source control 1. |
| `+0x14` | One byte | Earlier critical Boolean's low byte. This route copies it and does not draw for critical chance again. |
| `+0x15–+0x17` | Padding | No semantic value established; not copied by the semantic event copy helper. |
| `+0x18` | Signed 32-bit integer interpretation in the application routine | Zero. |
| `+0x1C` | Signed 32-bit integer interpretation in the application routine | Zero. |
| `+0x20–+0x37` | Three eight-byte position components | Raw 24-byte copy of the candidate position beginning at candidate `+0x10`. This producer does not reinterpret their signedness or units. |
| `+0x38`, `+0x3C`, `+0x40` | Three binary32 fields | Positive zero. |
| `+0x44`, `+0x45`, `+0x46` | Three separate one-byte flags/discriminators | Zero. The constructor clears the first two together with a 16-bit store and the third with a byte store. Their full gameplay meanings are not specified here. |
| `+0x47` | Padding | No semantic value established; not copied by the semantic event copy helper. |

The critical byte and signed amount have distinct roles. The earlier critical choice can already have doubled the amount; this producer does not perform another critical multiplication when copying the flag. The constructor's zero fields do not encode a resident-buff event, and no buff record is created by this contact-event construction.

Sources: `decompiled/client/functions/00459a30.c`, `004599a0.c`, `0042c5b0.c`, `00451ee0.c`, `0046d810.c`; `binary/Cube.exe`, earlier critical amount `0x221B50–0x221B82`, clamp `0x221CE5–0x221CFB`, constructor/candidate-position copy `0x221D39–0x221D50`, sign and coefficient `0x221D55–0x221D80`, flag/identifiers `0x221D88–0x221DBD`, semantic copy `0x0599A0`, and node allocation/copy setup `0x06D810`, `0x051EE0–0x051F04`. The sign mask at image RVA `0x345F70` contains sign bit `0x80000000` per lane; binary32 0.5 is at `0x345DA0`.

## Append first, then conditional application

After constructing the event, the caller appends it at the native list tail through `0x086290`. The receiver is the outer world-update routine's supplied event-output object itself, with no additional member offset added at this call. The append is not guarded by world mode or equality with world `+0xB8`. Those checks occur afterward.

The caller then attempts local application through the separate routine at `0x195A60` when world mode byte `+0xB4` is zero. With a nonzero mode byte, it attempts application only when **both cached source and candidate pointers equal world `+0xB8`**. Equality of only one is insufficient. The later hit-source pointer is not used by this application gate.

Application receives the world as its native object, followed by the freshly constructed event, the same outer event-output object used for append, and an auxiliary native object whose full purpose is outside this contract. The event already exists in the output list when this call occurs. Failing the pointer/mode gate suppresses the application call without undoing the append.

The element-2 event path does not call generic hit `0x196D30` and does not update the generic-hit success accumulator through the code following that generic call. It is not a generic hit with a negative selector or a substituted critical argument.

On reaching the shared first-candidate continuation, native candidate field `+0x134` is raised to projectile 32-bit `+0x6C` only when the latter is signed-greater than the current field. This shared write can occur after an emitted event, an application call, a skipped local application, or an element-2 admission failure. Its complete meaning is not established here and it is not an event success marker.

Sources: `decompiled/client/functions/00486290.c`, `00451ee0.c`, `004599a0.c`; `binary/Cube.exe`, event-output argument capture `0x20C549–0x20C564`, append `0x221DB7–0x221DCA`, post-append mode/equality gate `0x221DCF–0x221DF8`, application arguments/call `0x221DFE–0x221E19`, generic-call bypass `0x221E1E–0x221E28`, shared field update `0x221EAA–0x221EBB`; matching original assembly.

## Bounded application-routine behavior

The application routine resolves the target through the event's second identifier at event `+0x08`, not by accepting the candidate pointer directly. It first requires that the identifier resolve to a nonnull creature and rejects ordered nonpositive target health at creature `+0x16C`. An unordered health comparison does not take that particular rejection branch. These lookups can introduce a later lifetime dependency after the producer's pointer checks.

There is an additional early application case: if world `+0xB8` is null and the resolved target's control byte is zero, the application routine appends the event through `0x086290` to its supplied event-output object and returns before local health processing. The producer has already appended to that same object. Under these conditions and normal successful allocation, the visible sequence therefore contains two append operations, rather than an immediate health change from this application call. This is the observed original ordering, not a general replication policy for every event consumer.

Outside that early case, the routine separately resolves the source from event `+0x00`, the projectile's stored identifier. It does not receive the producer's cached-source or hit-source pointers as its source object. This contract does not equate the new lookup result with either earlier pointer under every lifetime change.

For the ordinary resolved-target case with target flag mask `0x2000` clear in its 16-bit flags beginning at `+0x7E`, the event's zero byte at `+0x45` passes the checked event-kind exclusion. The relevant health-write region requires world mode zero or resolved target equal to world `+0xB8`. A strictly negative event amount is subtracted from target binary32 health, increasing it, and a result greater than the binary32 limit returned by native helper `0x044DB0` is replaced by that helper's result. Equality with the limit is retained. The helper is called again when the upper replacement is needed. The full health-limit/stat formula is outside this contract.

That confirmed health operation uses the stored signed amount directly. It does not double it again because the event's critical byte is true. Negative zero does not satisfy the strict-negative health-increase test. The broader consequences of a zero event, source re-resolution failure, target flag `0x2000` cases, threat/relationship bookkeeping, protection, presentation, random draws and secondary events inside the application routine are not fully audited here. Appending the event or invoking this routine therefore does not by itself prove a health increase.

Sources: `decompiled/client/functions/00595a60.c`; `binary/Cube.exe`, target lookup and entry rejection `0x195AAB–0x195AF7`, early forwarding/append case `0x195AFD–0x195B18`, source lookup `0x195B1D–0x195B3E`, checked event discriminator `0x195D70–0x195D74`, mode/target gate `0x195ED2–0x195EE7`, strict-negative health operation and upper limit `0x196008–0x196052`; corresponding original assembly. Special target-flag admissions and the unexpanded helper/dependency behavior remain explicit limits.

## Later kind-1/element-2 conversion after the impact phase

At the post-impact continuation, the conversion requires current projectile kind exactly 1 and element byte exactly 2. It does not itself test hit success, candidate count, source-pointer existence, relationship result, world mode, or whether the earlier element-2 event was emitted or applied. Earlier conditions needed to reach this post-impact continuation, including full collision-state construction, remain outside this contract.

Immediately before this conversion, a separate success-dependent owner-counter operation can run when the accumulated generic-hit success byte is nonzero, projectile kind is not 3, and hit source exists. That operation increments source `+0x70`, invokes native helper `0x0453B0`, and clears source `+0x74`. Its helper behavior is not reconstructed here. The conversion follows regardless of whether that optional operation ran.

The conversion performs these established projectile mutations in order:

| Projectile field | Width | New value and dependency |
| --- | --- | --- |
| `+0x60` | 32-bit kind | 3. |
| `+0x68` | Signed 32-bit clock | 3000 when the preexisting flag byte at `+0x5C` is zero; 0 for every nonzero preexisting flag value. This decision precedes clearing the flag. |
| `+0x48` | Binary32 amount | Its current stored value multiplied once by native binary32 0.05, bit pattern `0x3D4CCCCD`. This uses the projectile's stored amount, not a candidate-local critical-adjusted event amount. |
| `+0x4C` | Binary32 quantity | The preexisting binary32 value at `+0x54` multiplied by exact binary32 5, then exact binary32 5 added, with the multiplication and addition separate. The previous `+0x4C` value is not an input. |
| `+0x54` | Binary32 quantity | Positive zero after it has supplied the preceding calculation. |
| `+0x5C` | One-byte flag | Zero after its old value has selected the new clock. |

The element byte at `+0x64`, identifiers and position are not assigned by this conversion block. There is no probability draw or event construction in the block. It continues to the outer projectile iteration rather than immediately running another contact operation for the newly written kind. The complete lifetime and subsequent kind-3 behavior are not inferred from this one transition.

Sources: `binary/Cube.exe`, optional preceding owner-counter update `0x222B10–0x222B2E`, conversion conditions and stores `0x222B35–0x222B92`; matching original `decompiled/client/assembly.txt`. Binary32 0.05 is at image RVA `0x2FCDB4`; exact binary32 5 is at `0x2FCE24`. Clock units, collision geometry, all later kind-3 scheduling, full source/candidate lifetime and server behavior remain unresolved dependencies.
