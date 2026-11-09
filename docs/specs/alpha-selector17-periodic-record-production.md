# Alpha selector-17 periodic-record production

Status: FINAL / STABLE / HELD after complete prose readback by the author.

## Supported image and bounded scope

The supported original image is `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred image base `0x00400000`. All evidence paths below are relative to the original `disassembled alpha cube world` root. Addresses are RVAs unless explicitly labeled VAs.

This recovery covers the ordinary path in generic hit resolution at RVA `0x196D30` that produces a resident type-4 record when its selector argument is 17. It includes the immediately preceding target multipliers, the corresponding 40-byte event, local insertion/application boundaries, and the relevant world ownership/retention gates. It does not recover the earlier hit-damage formulas, the later periodic tick scheduler, packet transport, or all other hit-resolution branches. Original evidence only was used; prior held artifacts and other specifications remained unopened and unchanged.

Primary evidence is `decompiled/client/assembly.txt`, the original routine files `decompiled/client/functions/00596d30.c`, `00451d00.c`, `00451c70.c`, `00446af0.c`, and the original receiving/application paths identified below. Original image bytes establish the coefficient bits.

## Entry and selector roles

The generic-hit routine receives the world object in ECX. Its first stack argument is the target creature pointer, its second is the attacker creature pointer, its ninth is the event-bundle pointer, and its twelfth is the 32-bit selector. The selector is read as a full 32-bit value at the original frame offset `+0x34`; the relevant comparison is equality with integer 17. This is distinct from testing the attacker's current action byte, and from the resident type byte, which is 4.

The selector branch is reached only after the ordinary resolver path's preceding gates and processing. At entry, the routine rejects an ordered target health value at `+0x16C` that is at or below zero. It also rejects the target when world byte `+0xB4` is nonzero and the target pointer equals the creature pointer at world `+0xB8`. Other earlier redirects/rejections, including the special current-action-28 path and the block-related path, remain upstream of the scoped production point. This document does not replace those gates with selector equality alone.

At the reached selector-17 branch there is no additional test of critical flag, magic flag, current target health, target role, current attacker action, positive damage, world mode, or owner pointer before producing the record. The branch dereferences the attacker pointer to copy its identifier. A valid nonnull attacker and event bundle are therefore required by this production path; it supplies no zero-source fallback for a null attacker. [Evidence: `decompiled/client/assembly.txt`, VAs `0x00596D6F–0x00596DD3` and `0x0059804D–0x005981C9`.]

## Amount and ordering with existing target records

Immediately before selector production, the resolver walks the target's resident list in linked-list order. The sentinel pointer is stored at target `+0x1178`. Every resident whose type byte at node `+0x08` equals 1 multiplies the running hit damage by that resident's binary32 amount at node `+0x0C`. The resident amount is the first scalar multiplication operand and the previous damage is the second. Each result is stored as binary32 before the next resident is examined. The loop checks the type byte; it adds no separate timer-validity or amount-range check.

The resulting binary32 damage is then copied into the ordinary direct-hit record's damage field. Only after that snapshot does selector 17 multiply the same damage by the binary32 coefficient 0.1 for the new periodic amount. The stored coefficient at RVA `0x2FCB7C` has bits `0x3DCCCCCD`. The multiplication is a scalar binary32 operation and its result is stored as binary32. There is no binary64 intermediate, integer conversion, rounding to a damage integer, division by a tick count, absolute value, positive minimum, maximum-health scale, or cap to the target's remaining health in this production calculation.

Thus the new amount is based on the post-type-1-multiplier damage, rather than on an earlier caller damage value or the eventual amount of HP actually removed. Negative, zero, nonfinite and unusual multiplier results receive no new sanitization in this selector branch. Earlier ordinary resolver behavior and ambient floating-point exception handling remain separate limits.

The new type-4 resident is inserted after the type-1 traversal, so that insertion is not revisited by the same multiplier loop. Its amount multiplication also leaves the already constructed ordinary direct-hit record's damage snapshot unchanged. [Evidence: `decompiled/client/assembly.txt`, VAs `0x00598023–0x005980C8`, especially the binary32 damage stores at VAs `0x00598041` and `0x00598078`; `binary/Cube.exe`, constant RVA `0x2FCB7C`.]

## Exact resident payload and local insertion

The resident payload is 24 bytes. Its semantic assignments in this producer are:

| Resident-relative offset | Width | Value established by this producer |
| --- | --- | --- |
| `+0x00` | One byte | Type 4 |
| `+0x01–+0x03` | Three bytes | Not initialized on this production path |
| `+0x04` | Binary32 | The post-target-multiplier damage multiplied by stored binary32 0.1 |
| `+0x08` | 32-bit timer word | Literal 3000, bits `0x00000BB8` |
| `+0x0C` | Four-byte word | Not initialized on this production path |
| `+0x10` | Eight-byte identifier | Attacker identifier copied from creature `+0x08` and `+0x0C` |

The timer is a literal native timer value, not derived from charge, damage, selector count, elapsed time or attacker level. This bounded producer recovery does not establish the timer's later countdown/tick interpretation or assign a phase value to `+0x0C`.

The uninitialized bytes are a material native limit. This producer writes the type as one byte, writes the amount and timer, and copies the source identifier; it does not clear the 24-byte temporary first or assign the intervening word at `+0x0C`. The record-copy helpers copy the entire payload, including those bytes. This contract consequently assigns neither zero nor another deterministic value to them. The semantic assignments above are stable; unspecified stack contents are not reconstructed.

The producer allocates/copies a new resident through RVA `0x051D00` and appends it at the target list's tail. That helper copies exactly three eight-byte chunks into the new node's payload at node `+0x08`. The target's stored resident count at `+0x117C` is incremented and the new node is linked into the list. There is no scan to merge, refresh, replace or suppress an existing type-4 record, including one from the same attacker. Successful repeated invocations append repeated residents. The native resident-count capacity is guarded at `0x07FFFFFE` under the list's normal count invariants. [Evidence: `decompiled/client/assembly.txt`, VAs `0x005980C5–0x0059815D` and `0x00451D00–0x00451D35`; `decompiled/client/functions/00596d30.c`, the declarations and assignments of the 24-byte temporary around its selector-17 branch.]

This local insertion is the resident application performed by this branch. It does not itself perform the resident's future periodic HP operation, reduce the current direct-hit damage by the periodic amount, or require a successful later direct-hit HP subtraction.

## Emitted 40-byte event

After the local resident has been appended and linked, the routine queues a 40-byte event in the supplied event bundle's list at `+0x58`, with its count at `+0x5C`. Its payload is:

| Event-relative offset | Width | Value |
| --- | --- | --- |
| `+0x00` | Eight bytes | Source header identifier: attacker creature identifier |
| `+0x08` | Eight bytes | Target header identifier: target creature identifier |
| `+0x10` | 24 bytes | Copy of the produced resident payload |

Consequently the embedded resident source at event `+0x20` repeats the attacker identifier. The event's source header, target header and embedded source are separate fields; the producer does not exchange source and target or substitute a world-owner identifier. Each identifier is copied as its two native 32-bit halves without numeric conversion.

The 24 payload bytes are copied from the same temporary used for the local resident, preserving the amount bits, timer, source and unspecified bytes. Event constructor RVA `0x051C70` copies exactly five eight-byte chunks into the list node's payload. The event count is incremented and the node is linked at the tail; the normal event-count capacity guard is `0x05555554`. This event append is not conditioned on world `+0xB4` after the selector branch has been reached. [Evidence: `decompiled/client/assembly.txt`, VAs `0x00598127–0x005981C9` and `0x00451C70–0x00451CB9`; `decompiled/client/functions/00451c70.c`.]

The local resident insertion occurs first, the 40-byte event append second. Subsequent ordinary resolver effects occur afterward. The ordinary direct-hit record is eventually appended to the event bundle's separate base list, after the periodic-record event has already been queued. These are separate lists; this construction order does not establish a global ordering for all later transport or receiving phases. Allocation/capacity failures are ordinary native exception boundaries rather than a proved atomic transaction across the two appends.

No random draw, seed change or probabilistic proc test is added by the selector-17 production sequence itself. Earlier/later generic resolver draws remain outside this narrow statement. [Evidence: `decompiled/client/assembly.txt`, VAs `0x005980C5–0x005981C9`, followed by the ordinary hit-record append at VAs `0x00598677–0x005986B2`.]

## Receiving the event and applying the resident

The world-update receiver walks the incoming bundle's `+0x58` list. For each event it looks up the creature identified by event `+0x08`, the target header. If that target lookup yields a nonnull creature pointer, it passes the 24 bytes at event `+0x10` to resident application at RVA `0x046AF0`. The event's source header at `+0x00` is not looked up or used as an ownership gate by this receiving loop. This loop does not recalculate the amount, timer or source and does not re-evaluate selector 17, a critical decision or a current action. It makes no target-health check before that resident-application call.

The client-side incoming-event application visible in the routine at RVA `0x088EE0` uses the same target-header lookup and embedded-resident application pattern. In both reviewed receiving loops, absence of the target prevents local resident application for that event. This does not prove a general network-delivery or retry rule.

RVA `0x046AF0` can replace matching residents for types 1, 2, 3, 6, 7, 9, 10 and 11. Type 4 is outside that replacement set. A received type-4 payload therefore appends a new resident, copies all 24 bytes, and increments the resident count; it does not refresh an existing type-4 entry. There is no source-based deduplication in this helper. [Evidence: `decompiled/client/assembly.txt`, VAs `0x0060CDEA–0x0060CE8B` with the receiving call at VA `0x0060CE52`; VAs `0x0048FD00–0x0048FD2F`; and VAs `0x00446AF0–0x00446BA7`; `decompiled/client/functions/00446af0.c`.]

## World ownership and retention gates

The original world fields are identified numerically here rather than assigning an unproved client/server name to each mode:

| World field | Width | Relevant role |
| --- | --- | --- |
| `+0xB4` | One byte | Mode gate used for producer entry, immediate direct-hit application and received-event-list retention |
| `+0xB8` | Creature pointer | World-associated/owned creature identity used by those hit ownership checks |

The ordinary producer rejects a hit against the world-associated creature when `+0xB4` is nonzero, as described above. For a hit that passes the earlier resolver stages and reaches selector 17, the new local resident and 40-byte event are produced without an additional ownership test.

After the ordinary direct-hit record has been appended, generic hit resolution immediately invokes direct-hit application at RVA `0x195A60` only when world `+0xB4` is zero. A nonzero mode skips that immediate application call. This later gate does not undo or defer the already completed local type-4 insertion/event append. [Evidence: `decompiled/client/assembly.txt`, VAs `0x005986B4–0x005986D0`.]

The reviewed direct-hit application routine first resolves its target and rejects an absent/null target or an ordered nonpositive health value. If the world's associated creature pointer at `+0xB8` is null and the resolved target's numeric role byte is zero, it appends the ordinary direct-hit record to the supplied event bundle's base hit list and returns along that relay path instead of entering its normal local application body. Within the normal body, the HP-application ownership gate permits progression when world `+0xB4` is zero or the resolved target pointer equals world `+0xB8`. These direct-hit gates govern ordinary hit application; neither is a second predicate for the earlier selector-produced resident. This document does not expand the other HP/status rules in that routine. [Evidence: `decompiled/client/assembly.txt`, VAs `0x00595AC7–0x00595B18` and `0x00595ED2–0x00595EED`; `decompiled/client/functions/00486290.c` confirms the base-list hit-record append.]

After processing incoming 40-byte events in the world-update receiver, a nonzero world `+0xB4` clears the incoming bundle's `+0x58` collection. A zero mode leaves that collection intact at this phase, preserving it for subsequent handling/relay. Resident application in the preceding per-event loop is not conditional on this mode. This is the established retention/clear gate; the full network relay, serialization, recipient selection and eventual queue lifetime are not recovered here. [Evidence: `decompiled/client/assembly.txt`, VAs `0x0060CE93–0x0060CEA5`; `decompiled/client/functions/0046f870.c` establishes the collection reset to an empty sentinel/count.]

## Limits

The closed production facts are the full-width selector test, post-target-multiplier binary32 amount, literal timer, attacker source, append-only local type-4 application, exact 40-byte header/payload construction, ordering relative to the ordinary hit record, and the stated ownership/retention gates. Resident padding and the word at `+0x0C` remain unspecified because this producer does not initialize them. No deterministic elapsed counter or tick phase is assigned by this contract.

The periodic consumer's countdown, tick cadence, attribution during later damage, expiry/erasure, broader resolver eligibility, complete direct-hit application, transport behavior and delivery deduplication remain outside scope. The game-owned production uses scalar binary32 multiplication with the original coefficient; exact exceptional-value behavior remains subject to native ambient floating-point controls and allocation/runtime exceptions. No runtime captures, tests, probes, SDKs, reconstruction or replacement-engine material were used.
