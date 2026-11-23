# Alpha element-2 projectile contact event and later kind-3 conversion

Status: FINAL / STABLE / HELD. This is an independently worded, bounded source-side behavioral contract, re-certified on 2026-11-22. It does not certify a replacement implementation or runtime acceptance.

The evidence root is `disassembled alpha cube world`. The supported original image is `binary/Cube.exe`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, with preferred image base `0x00400000`. Addresses below are RVAs unless explicitly identified as object/record offsets. The scope is the first projectile element-2 event route at `0x221D03–0x221E28`, its relationship predicate, event construction and append/application boundary, and the later kind-1/element-2 conversion at `0x222B35–0x222B92`. Numeric kind and element identifiers are retained without assigning unproved gameplay names. No other contract, reconstruction, implementation, runtime capture, test, or implementation history supplies facts here. No server counterpart or complete projectile reconstruction is claimed.

## Object identities and admission

The projectile has a 64-bit source identifier in its first eight bytes. The surrounding original code resolves that identifier in the world's creature map more than once, each time through the same lookup routine at `0x02F000` with the projectile's first eight bytes as the key; that routine yields null when the key is absent. The earlier result, obtained at `0x221476`/`0x221486` (an existence test followed by a repeated lookup whose result is kept), is called the cached source here and is the one used by this event route. A later result at `0x22156C` is called the hit source here; it supplies the earlier critical calculation and some prior candidate checks. Although both use the same key, they are taken at different times, so this contract preserves the actual pointer used at each operation rather than treating them as interchangeable.

The candidate is the creature selected by the surrounding native scan. Its position object begins at creature `+0x10`, its identifier occupies creature `+0x08` through `+0x0F`, and its control discriminator is one byte at `+0x60`. The projectile element discriminator is one byte at projectile `+0x64`; its kind is a 32-bit discriminator at `+0x60`. World mode `+0xB4` is one byte, and world `+0xB8` is a native 32-bit distinguished-creature pointer.

After the earlier amount calculation and clamp, element byte 2 selects this separate event route. It requires a nonnull cached source. The relationship helper at `0x196CA0` receives **the cached source as its first explicit stack argument and the candidate as its second**; the helper removes both arguments itself on return. The world object and the hit-source pointer are not either of those relationship arguments, and the helper uses no implicit world pointer.

Only a false low-byte relationship result permits event construction. The candidate control byte must then be different from 6. This is an additional caller test: the relationship helper itself returns false for a candidate with control 6, but that false result does not admit the event because of this subsequent exclusion.

No new pointer-self exclusion or cached-source class/specialization test occurs inside this route. Earlier candidate health, geometry, lock, timing, and source-dependent admission remain separate prerequisites. A null hit source does not itself reject this route when the cached source exists; a null cached source does. No event amount minimum is tested here.

Every element-2 outcome, including failed cached-source, relationship or control admission, bypasses the first generic hit call at `0x221E8C`. Failure here is not a fallback to that generic hit routine.

Sources: `binary/Cube.exe`, lookup routine `0x02F000–0x02F027`, cached and hit lookups `0x221465–0x22148B` and `0x221561–0x22157A`, candidate-position identity `0x221674–0x221681`, element/source/relationship/control admission `0x221D03–0x221D33`, element-2 continuation `0x221E19–0x221E28`; corresponding ranges in `decompiled/client/assembly.txt`.

## Relationship predicate, with argument order preserved

The following rules describe `0x196CA0` for its actual first and second pointers. In this call they are the cached source and the candidate, respectively. The returned contract is the low byte of the result register; other bits are not a Boolean result guarantee.

The predicate first returns false when the first creature's control byte is 5 and the second's is 0, when the first control byte is 6, or when the second control byte is 6. These early false cases take precedence over its later classification and flag checks.

Outside those exceptions, having control byte 1 on exactly one of the two creatures produces true. When both controls are 1, the helper compares their one-byte classifications from `0x044680`; unequal classifications produce true. It computes the first creature's classification before the second's. When that comparison is equal, the native routine repeats the same first-then-second classification pair through its second control-1 test before reaching its final flag test. The classification helper has no chance draw or mutation.

Classification byte 1 requires both of these conditions: the creature's 16-bit flags beginning at `+0x7E` have none of mask `0x1A00` set, and its 32-bit discriminator at `+0x64` is one of **19, 20, 22, 23, 25, 26, 27, 30, 31, 32, 33, 34, 35, 53, 55, 56, 57, 58, 67, 68, 74, 87, 89, 92, 93, 98, 100, 106, 107, 145, 146, 147** (32 values). Otherwise the classification's low byte is 0. Gameplay meanings for that classification and discriminator set are not inferred here.

After the applicable control and classification checks, the relationship helper returns true if bit `0x20` is set in the one-byte flags at creature `+0x124` on either argument (first argument tested first); otherwise it returns false. Swapping the two creature arguments is not authorized by this contract, because the early control-5/control-0 case is ordered. Neither helper checks resident buffs, remaining buff time, source identifiers, mana, class at `+0x140`, or specialization at `+0x141` for this relationship result.

Sources: `binary/Cube.exe`, relationship body `0x196CA0–0x196D2A`, including the two stack-argument reads `0x196CA4–0x196CA8`, ordered early case `0x196CAB–0x196CCA`, classification call order `0x196CCD–0x196D07`, and final flag test `0x196D09–0x196D19`; classification routine `0x044680–0x044759`; matching `decompiled/client/assembly.txt`.

## Signed amount and event fields

The input to this route is the earlier binary32 candidate amount. That amount originated at projectile `+0x48` and was doubled by binary32 multiplication with the image constant 2.0 when the earlier critical Boolean, calculated from the hit source, was true; with a null hit source the critical Boolean is zero. The first-path amount clamp replaces an ordered negative amount with binary32 positive zero. It does not clamp a positive value downward, does not replace negative zero (which compares equal to zero), and does not supply a substitute for an unordered comparison.

The event producer toggles the amount's sign bit using the native sign mask. It then multiplies the signed result by exact binary32 0.5 **only when the cached source's control byte at `+0x60` equals 1**. The tested creature is the cached source, not the candidate or the hit source. Every other cached-source control value that reaches this point retains the full sign-toggled amount.

For an ordinary finite positive input, the event therefore contains a negative amount, with half magnitude for cached-source control 1 and full magnitude otherwise. A clamped or positive-zero input becomes negative zero, and a negative-zero input becomes positive zero; zero is not filtered out before append. The operation is a sign-bit toggle rather than an absolute-value operation. Non-finite input interpretation beyond the observed operations remains unresolved.

The native event constructor at `0x059A30` initializes the fields listed as zero below; the caller then fills the identifiers, amount, critical byte, and candidate position. The payload occupies 72 bytes inside a native 80-byte list node whose first eight bytes are two four-byte links. The node's copy helper copies the semantic fields individually rather than copying record padding as a 72-byte block.

| Event-relative offset | Width | Value produced on this route |
| --- | --- | --- |
| `+0x00` | 64-bit identifier | Copied from projectile `+0x00`/`+0x04`. This is the projectile's stored source identifier, not a fresh copy from either source creature pointer. |
| `+0x08` | 64-bit identifier | Candidate creature identifier copied from candidate `+0x08`/`+0x0C`. |
| `+0x10` | Binary32 | Sign-toggled candidate amount, subsequently multiplied by 0.5 only for cached-source control 1. |
| `+0x14` | One byte | Earlier critical Boolean's low byte. This route copies it and does not evaluate critical chance again. |
| `+0x15–+0x17` | Padding | No semantic value established; not copied by the semantic event copy helper. |
| `+0x18` | 32-bit, compared as signed in the application routine | Zero. |
| `+0x1C` | 32-bit, compared as signed in the application routine | Zero. |
| `+0x20–+0x37` | 24 bytes | Raw copy, as six 32-bit words, of the 24 bytes beginning at candidate `+0x10`. This producer does not reinterpret their component layout, signedness or units. |
| `+0x38`, `+0x3C`, `+0x40` | Three binary32 fields | Positive zero. |
| `+0x44`, `+0x45`, `+0x46` | Three separate one-byte flags/discriminators | Zero. The constructor clears the first two together with a 16-bit store and the third with a byte store. Their full gameplay meanings are not specified here. |
| `+0x47` | Padding | No semantic value established; not copied by the semantic event copy helper. |

The critical byte and signed amount have distinct roles. The earlier critical choice can already have doubled the amount; this producer does not perform another critical multiplication when copying the flag. The constructor's zero fields do not encode a resident-buff event, and no buff record is created by this contact-event construction.

Sources: `binary/Cube.exe`, earlier critical amount `0x221B50–0x221B82` (multiplier constant 2.0 at image RVA `0x345E10`), clamp `0x221CE5–0x221CFB`, constructor `0x059A30–0x059A8D`, candidate-position copy `0x221D44–0x221D50` with 24-byte copy routine `0x02C5B0`, sign and coefficient `0x221D55–0x221D80`, flag and identifiers `0x221D88–0x221DBD`, semantic copy `0x0599A0–0x059A23`, node allocation of 80 bytes `0x06D810–0x06D843`, and node creation/copy setup `0x051EE0–0x051F04`. The sign mask at image RVA `0x345F70` contains sign bit `0x80000000` per lane; binary32 0.5 is at `0x345DA0`.

## Append first, then conditional application

After constructing the event, the caller appends it at the native list tail through `0x086290`. The receiver is the outer routine's second explicit stack argument (an event-output list object) itself, with no additional member offset added at this call. The append is not guarded by world mode or equality with world `+0xB8`. Those checks occur afterward.

The caller then attempts local application through the separate routine at `0x195A60` when world mode byte `+0xB4` is zero. With a nonzero mode byte, it attempts application only when **both cached source and candidate pointers equal world `+0xB8`**. Equality of only one is insufficient. The hit-source pointer is not used by this application gate.

Application receives the world as its native object, followed by three explicit arguments in this order: the freshly constructed event, the same event-output object used for append, and the address of an auxiliary object in the outer routine's frame (the same auxiliary object that the generic hit call receives); its full purpose is outside this contract. The event already exists in the output list when this call occurs. Failing the pointer/mode gate suppresses the application call without undoing the append.

The element-2 event path does not call generic hit `0x196D30` and does not update the generic-hit success accumulator that follows that generic call. It is not a generic hit with a negative selector or a substituted critical argument.

On reaching the shared first-candidate continuation, native candidate 32-bit field `+0x134` is raised to projectile 32-bit `+0x6C` only when the latter is signed-greater than the current field. This shared write can occur after an emitted event, an application call, a skipped local application, or an element-2 admission failure. Its complete meaning is not established here and it is not an event success marker.

Sources: `binary/Cube.exe`, outer argument capture `0x20C549–0x20C564`, append `0x221DB7–0x221DCA`, tail-append routine `0x086290–0x0862D0`, post-append mode/equality gate `0x221DCF–0x221DF8`, application arguments and call `0x221DFE–0x221E19`, generic-call bypass `0x221E1E–0x221E28`, shared field update `0x221EAA–0x221EBB`; matching `decompiled/client/assembly.txt`.

## Bounded application-routine behavior

The application routine resolves the target through the event's second identifier at event `+0x08`, not by accepting the candidate pointer directly. It first requires that the identifier resolve to a nonnull creature and rejects ordered nonpositive target binary32 health at creature `+0x16C`. An unordered health comparison does not take that particular rejection branch. These lookups can introduce a later lifetime dependency after the producer's pointer checks.

There is an additional early application case: if world `+0xB8` is null and the resolved target's control byte is zero, the application routine appends the event through `0x086290` to its supplied event-output object and returns before source resolution or local health processing. The producer has already appended to that same object. Under these conditions and normal successful allocation, the visible sequence therefore contains two append operations rather than an immediate health change from this application call. From this producer the case is reachable only with world mode zero, because a nonzero mode requires the nonnull cached source to equal world `+0xB8`. This is the observed original ordering, not a general replication policy for every event consumer.

Outside that early case, the routine separately resolves the source from event `+0x00`, the projectile's stored identifier, and tolerates a null result. It does not receive the producer's cached-source or hit-source pointers as its source object. This contract does not equate the new lookup result with either earlier pointer under every lifetime change.

For a strictly negative event amount, the routine's positive-amount and non-negative-amount blocks are skipped. For the resolved-target case with target flag mask `0x2000` clear in its 16-bit flags beginning at `+0x7E`, the event's zero byte at `+0x45` passes the checked exclusion of value 3. The relevant health-write region requires world mode zero or resolved target equal to world `+0xB8`. A strictly negative event amount is subtracted from target binary32 health, increasing it, and a result greater than the binary32 limit returned by native helper `0x044DB0` is replaced by that helper's result. Equality with the limit is retained. The helper is called again when the upper replacement is needed. The full health-limit/stat formula is outside this contract.

That confirmed health operation uses the stored signed amount directly. It does not double it again because the event's critical byte is true. Negative zero does not satisfy the strict-negative health-increase test. The broader consequences of a zero event, target flag `0x2000` cases, threat/relationship bookkeeping, protection, presentation, random draws and secondary events inside the application routine, and everything after the upper-limit replacement, are not fully audited here. Appending the event or invoking this routine therefore does not by itself prove a health increase.

Sources: `binary/Cube.exe`, target lookup and entry rejection `0x195AAB–0x195AF7`, early forwarding/append case `0x195AFD–0x195B18`, source lookup `0x195B1D–0x195B3E`, amount-sign branches `0x195B49–0x195B55` and `0x195BD5–0x195BDD`, target flag test `0x195CE7–0x195CF0`, checked event discriminator `0x195D70–0x195D74`, mode/target gate `0x195ED2–0x195EE7`, sign split `0x195EED–0x195EF5`, strict-negative health operation and upper limit `0x196008–0x196052`; corresponding `decompiled/client/assembly.txt`.

## Later kind-1/element-2 conversion after the impact phase

At the post-impact continuation, the conversion requires current projectile kind exactly 1 and element byte exactly 2. It does not itself test hit success, candidate count, source-pointer existence, relationship result, world mode, or whether the earlier element-2 event was emitted or applied. Earlier conditions needed to reach this post-impact continuation, including full collision-state construction, remain outside this contract.

Immediately before this conversion, a separate success-dependent owner-counter operation runs when the accumulated generic-hit success byte is nonzero, projectile kind is not 3, and the hit source exists. That operation increments 32-bit hit-source `+0x70`, invokes native helper `0x0453B0` on the hit source, and then clears 32-bit hit-source `+0x74`. Its helper behavior is not reconstructed here. The conversion test follows regardless of whether that optional operation ran.

The conversion performs these established projectile mutations in order:

| Projectile field | Width | New value and dependency |
| --- | --- | --- |
| `+0x60` | 32-bit kind | 3. |
| `+0x68` | Signed 32-bit clock | 3000 when the preexisting flag byte at `+0x5C` is zero; 0 for every nonzero preexisting flag value. This decision precedes clearing the flag. |
| `+0x48` | Binary32 amount | Its current stored value multiplied once by native binary32 0.05, bit pattern `0x3D4CCCCD`. This uses the projectile's stored amount, not a candidate-local critical-adjusted event amount. |
| `+0x4C` | Binary32 quantity | The preexisting binary32 value at `+0x54` multiplied by exact binary32 5, then exact binary32 5 added, with the multiplication and addition separate. The previous `+0x4C` value is not an input. |
| `+0x54` | Binary32 quantity | Positive zero (all-zero bit pattern) after it has supplied the preceding calculation. |
| `+0x5C` | One-byte flag | Zero after its old value has selected the new clock. |

The element byte at `+0x64`, identifiers and position are not assigned by this conversion block. There is no probability draw or event construction in the block. After the stores it jumps directly to the shared projectile-iteration continuation. The non-converting exit from the same test instead first performs a separate append, through `0x0860B0`, of the current projectile iteration position into a list local to the outer routine, and only then reaches that continuation; the converted projectile skips that append. The purpose of that local list is not established here. The conversion does not immediately run another contact operation for the newly written kind. The complete lifetime and subsequent kind-3 behavior are not inferred from this one transition.

Sources: `binary/Cube.exe`, optional preceding owner-counter update `0x222B10–0x222B2E`, conversion conditions and stores `0x222B35–0x222B92`, non-converting exit and local-list append `0x220D68–0x220D75`, shared iteration continuation `0x220D7A`, local-list append routine `0x0860B0–0x0860F0`; matching `decompiled/client/assembly.txt`. Binary32 0.05 is at image RVA `0x2FCDB4`; exact binary32 5 is at `0x2FCE24`.

## Unresolved

- Internal behavior of the critical routine at `0x0444A0`, including whether and when it draws a random value, is not audited here; this contract only fixes that its result is evaluated once, from the hit source, before this route.
- An earlier cached-source-dependent block at `0x221B8A–0x221CE5` (including a call at `0x221BC7` that receives binary32 0.15, and class `+0x140`/specialization `+0x141` tests) runs before this route; its behavior and any random draw it makes are outside this contract.
- The component layout, signedness and units of the 24 bytes copied from candidate `+0x10` into event `+0x20` are not established by this route.
- Non-finite amount handling beyond the observed comparisons and bit operations.
- Everything in the application routine after the health upper-limit replacement, the zero-amount and positive-amount branches, target flag `0x2000` handling, and the helper at `0x044DB0`.
- The meaning of candidate `+0x134`, projectile `+0x6C`, hit-source `+0x70`/`+0x74` and helper `0x0453B0`.
- The purpose of the outer routine's local list appended through `0x0860B0`, and therefore the full consequence of the converted projectile skipping that append.
- Clock units, collision geometry, all later kind-3 scheduling, full source/candidate lifetime, and server behavior.

## Re-certification (2026-11-22)

A fresh source-only author re-checked every statement in this document against the original `binary/Cube.exe` image (hash and size confirmed) and its instruction listing. The relationship argument order, control and classification rules, the 32-value discriminator set, event offsets and widths, the sign toggle and 0.5 coefficient, the append-then-gate order, the application-routine boundary and the conversion stores and constants were confirmed. Corrections: the event position field is now described as a raw 24-byte copy without asserting three eight-byte components; the conversion exit is now described as skipping a local-list append that the non-converting exit performs; the cached and hit sources are identified as two lookups of the same projectile key at different times; negative-zero clamp behavior and the mode-zero reachability of the early double-append case were made explicit; and source citations were limited to the image and its instruction listing. The open items are listed in the Unresolved section above.
