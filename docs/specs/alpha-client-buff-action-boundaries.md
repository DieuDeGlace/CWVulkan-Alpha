# Alpha client buff and action boundaries

This source-only contract covers the client action-selection boundary at RVA `0x0456A0`, its type-9 anticipation shortcut, and the common-action type-11 consume and byte-latch behavior. It separates the selected-action gate from the actions the selector can actually return. It does not specify every action body, every caller's input eligibility, or the complete combat result.

All evidence paths are relative to `disassembled alpha cube world`. No held contract, replacement implementation, SDK, extractor, test, probe, capture, log, or implementation history was read for this assignment.

## Supported originals and fields

| Original image | Size | SHA-256 |
| --- | ---: | --- |
| `binary/Cube.exe` | 3,885,568 bytes | `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf` |
| `binary/Server.exe` | 1,718,784 bytes | `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1` |

Both are the original x86 images with preferred base `0x00400000`. Routine identifiers below are RVAs unless explicitly called virtual addresses. Original executable bytes were also disassembled in bounded address ranges where the existing text disassembly omitted reachable blocks.

The creature fields used here are evidence identifiers, not proposed replacement structures:

| Creature offset | Width and interpretation in this contract |
| --- | --- |
| `+0x08` | Opaque 64-bit creature identifier |
| `+0x60` | Unsigned 8-bit control discriminator |
| `+0x64` | 32-bit creature-kind value; compared to specific integer constants |
| `+0x68` | Unsigned 8-bit action value |
| `+0x6C` | Signed 32-bit action clock, milliseconds |
| `+0x70` | Signed 32-bit counter used by selection and readiness |
| `+0x128` | Signed 32-bit action-lock field; selection tests zero versus nonzero |
| `+0x140`, `+0x141` | Two unsigned 8-bit selection discriminators |
| `+0x144` | Binary32 pending resource amount in the common initializer |
| `+0x170` | Binary32 current mana/resource value |
| `+0x17C` | Binary32 timing multiplier |
| `+0xAA8`, `+0xAA9` | Two unsigned 8-bit item-selection discriminators |
| `+0x1178`, `+0x117C` | Native 32-bit list-head pointer and 32-bit resident buff count |
| `+0x13B4` | Binary32 amount retained by common-action resource processing |
| `+0x13B8` | 8-bit common-action processing state |
| `+0x13C0` | 8-bit latch, written as zero or one and read as zero versus nonzero |

The resident buff payload is 24 bytes: type at `+0x00` is an unsigned byte, magnitude at `+0x04` is binary32, remaining milliseconds at `+0x08` is signed 32-bit, and embedded source identifier at `+0x10` is opaque 64-bit. The other seven bytes are alignment storage whose universal initialization is not established. Identifiers are copied bit-for-bit; arithmetic signedness is not assigned to them here.

## Complete selection results at the audited boundary

Client selector RVA `0x0452A0` returns one of the twelve values `3`, `4`, `5`, `23`, `31`, `33`, `37`, `43`, `45`, `46`, `65`, and `66`. The following priority facts fully determine that set.

The combination `+0xAA8 = 3` and `+0xAA9 = 6` selects action `23` ahead of the other class/kind selection conditions.

Apart from that early choice, the ranged selection family is used when `+0x140 = 3`, when the 32-bit value at `+0x64` is `117` or `86`, or when `+0xAA8 = 3` and `+0xAA9` is `10`, `11`, or `12`. Within that family the subtype and the equality test on `+0x141` determine the result:

| `+0xAA9` in this family | `+0x141 = 1` | `+0x141` differs from `1` |
| --- | ---: | ---: |
| `10` | `33` | `31` |
| `11` | `45` | `46` |
| Any other value | `43` | `37` |

Outside that family, `+0xAA8 = 3` with subtype `5` selects action `5`. Remaining admitted subtypes `7`, `8`, `15`, `16`, `17`, and `18` select `66` for an even `+0x70` value and `65` for an odd value. Earlier subtype decisions take priority over repeated comparisons appearing later in the native routine. All other combinations select `3` for even `+0x70` and `4` for odd `+0x70`. Negative odd counters count as odd: this is a signed remainder/nonzero distinction, not an assumption that the counter is nonnegative.

These are selector facts, not a claim that every combination is normally reachable through player equipment or input. No resident buff is read or changed by this selector.

Anchors: `decompiled/client/functions/004452a0.c`, with original assembly at virtual addresses `0x004452A0..0x004453AB`. Server selector `decompiled/server/functions/00410290.c`, RVA `0x010290`, has the same tested selection relations.

## Admission before the new action is stored

Client helper RVA `0x0456A0` first obtains the current action's total duration from RVA `0x03D1A0`. If `+0x70` is zero, that total is the readiness threshold. If `+0x70` is nonzero, the helper subtracts the current action's recovery duration from that total to obtain the threshold. The subtraction concerns the old action, before selection is committed. The signed action clock must be at least that threshold; equality passes.

For this readiness subtraction, the recovery durations have the following complete grouping. A timing-scaled entry means its listed numerator is divided by the binary32 product of the tempo value returned by RVA `0x047700` and the creature's binary32 `+0x17C`, then truncated toward zero to a signed 32-bit millisecond value.

| Current action values | Recovery used by admission |
| --- | --- |
| `0`, `50`, `96` | Fixed `100` ms |
| `10` | Fixed `600` ms |
| `48` | Fixed `0` ms |
| `54` | Fixed `400` ms |
| `71`, `72` | Fixed `200` ms |
| `3`, `4`, `5`, `57`, `58`, `62` | Timing-scaled numerator `300` |
| `7`, `14`, `18`, `65`, `66` | Timing-scaled numerator `200` |
| `11`, `22`, `26`, `30`, `31`, `32`, `33`, `34`, `37`, `38`, `39`, `40`, `41`, `42`, `43`, `44`, `45`, `46`, `60`, `61`, `67`, `94`, `95`, `104` | Timing-scaled numerator `100` |
| `15` | Timing-scaled numerator `400` |
| `23` | Timing-scaled numerator `10` |
| `68`, `69`, `93` | Timing-scaled numerator `800` |
| Every other action value | Timing-scaled numerator `500` |

The total-duration helper adds its separately obtained integer phase durations. Its full table for arbitrary old actions is a dependency beyond this contract; a single floating-point total rounded once is not established as equivalent to its phase-wise integer results.

The helper then selects a candidate and applies the action-lock and mana admission tests. A nonzero `+0x128`, including a negative nonzero value, rejects the candidate except when `+0x140 = 4`, `+0x141 = 1`, and candidate action is `5`, `17`, or `20`. The current selector can return `5` from this exception set; it does not return `17` or `20`. The helper itself does not add a health, stun, world-side, local-creature, or control-discriminator test. Caller eligibility is a separate boundary.

The mana requirement helper is called for the candidate with its auxiliary argument equal to signed `-1`. For the twelve selectable results above, actions `3` and `4` require binary32 `0.1`; actions `31`, `33`, `37`, `43`, `45`, and `46` require binary32 `0.3` unless a resident type `9` exists, in which case their requirement is zero. Actions `5`, `23`, `65`, and `66` require zero through this helper. Equality between current mana and requirement passes. An ordered comparison that finds requirement greater than current mana rejects; the original comparison does not reject an unordered floating-point comparison by that test alone.

A resident type `9` is recognized by list membership and type only. Neither this cost lookup nor the anticipation lookup requires positive remaining time, positive magnitude, or a particular embedded source. The lookup walks the resident list in forward order. It does not draw randomness.

After admission, the selector is called again. The resulting action byte is stored at `+0x68` and the new action clock is first set to zero. The helper clears `+0x128` on normal accepted selection, while preserving it for the class/specification/action exception above. The Boolean result is returned in the low byte: zero on rejection, one on acceptance; the upper return bits are not specified as a Boolean value here.

Anchors: `decompiled/client/functions/004456a0.c`, RVAs `0x0456A0..0x045D6B`; total duration `0043d1a0.c`, RVA `0x03D1A0`; cost `00444ae0.c`, RVA `0x044AE0`; tempo `00447700.c`, RVA `0x047700`. Admission comparisons and the two selector calls are checked at virtual addresses `0x00445830..0x004458A3`; accepted lock handling and low-byte results at `0x00445D4C..0x00445D6B`.

## Type-9 anticipation shortcut and its limits

The exact new-action gate for the anticipation shortcut is `31`, `33`, `37`, `46`, and `95`. It is not limited to the two projectile cases `37` and `46`, and it does not include all actions receiving the type-9 zero-cost benefit. In particular, selectable actions `43` and `45` can receive the zero mana requirement without receiving this clock shortcut.

When the gate matches and a resident type `9` is found, the just-cleared clock is replaced with that new action's anticipation duration:

| New action in the native gate | Anticipation numerator before timing division |
| --- | --- |
| `31`, `33` | `1600` |
| `37` with `+0xAA9 = 12` | `600` |
| `37` with any other `+0xAA9` | `1200` |
| `46` | `1200` |
| `95` | `1000` |

Each uses the tempo and `+0x17C` timing product and signed truncation described above. The tempo is obtained after the new action byte has been stored. The shortcut does not reset `+0x70`. The boundary is set exactly to anticipation, without adding one millisecond or the current update duration.

Although `95` is present in the native shortcut gate, the complete selector audited here has no return of `95`. Its gate entry and anticipation value are established; a normal route from this selector to that entry is not. For this selector's actual results, the reachable shortcut set is therefore `31`, `33`, `37`, and `46`.

The action-selection helper does not debit mana, remove or replace type `9`, reduce its remaining duration, change its embedded source, construct a buff event, or write `+0x13C0`. Its accepted state changes are the action, action clock, and conditional lock clearing described above. Later action processing must not be folded into this helper merely because it follows in another update phase.

Timing arithmetic is binary32 multiplication and division followed by native signed conversion toward zero for ordinary finite, representable values. Zero timing denominators, nonfinite values, conversion overflow, and integer overflow are not assigned a replacement policy by this contract. The full tempo formula depends on other creature/item statistics and resident type `12`; it is not a constant attack speed supplied here.

Anchors: explicit five-value gate and type lookup at virtual addresses `0x004458AA..0x004458EB`; selected anticipation cases and final clock store in `004456a0.c` and original assembly through `0x00445D49`. Original binary32 numerators are at virtual addresses `0x006FD8FC` (`1600`), `0x006FCE74` (`600`), `0x006FD8F8` (`1200`), and `0x006FD8F4` (`1000`).

## Type-11 consumption in common-action initialization

This is a later boundary in the general world update, not an additional side effect of RVA `0x0456A0`. The common-action branch is reached through the original action dispatch. Its ownership gate allows the branch when the world byte at `+0xB4` is nonzero, when the creature control byte at `+0x60` is nonzero, or when the creature is the world pointer at `+0xB8`. Consequently, in a zero-byte world, a control-zero creature that differs from that local pointer is excluded. This gate is not generalized to unrelated ranged action branches.

At the common initializer's exact-zero action-clock boundary, the byte at `+0x13C0` is set to zero and the amount at `+0x13B4` is cleared. Actions `31` and `33` take a separate charging path before this zero-clock initializer; they do not receive its `+0x13C0` reset simply by starting at clock zero. A nonzero clock also bypasses the reset. The clock being tested is the value observed after the preceding common-action counter/timing adjustments, not necessarily an untouched value from input selection.

Within the zero-clock initializer, the nonpositive-cost path for action `5`, `17`, or `20` copies current mana from `+0x170` to `+0x13B4`, then sets current mana to zero. It looks for a resident type `11` afterward. The consumption therefore does not depend on a nonzero amount of mana: the lookup still occurs when the copied mana value is zero. The ordinary cost helper used here returns zero for these three action values.

If no type `11` is found, no type-11 replacement event or latch-one write occurs through this path. When type `11` exists, a fresh 24-byte payload is constructed with type `11`, magnitude zero, remaining duration zero, and embedded source identifier zero. The resident setter replaces the first existing type-11 payload in place. Its previous magnitude, lifetime, and embedded source do not survive that replacement. This is not immediate node deletion, and later duplicate type-11 nodes, if any, are not removed by this setter.

A 40-byte event is then appended to the outgoing update's buff-event list at `+0x58`, with count at `+0x5C`. Its outer source and outer target are both the acting creature's identifier. Its embedded 24-byte payload is the newly constructed zeroed-meaningful-fields type-11 payload. In particular, its embedded source is zero even though both outer identifiers are the creature's nonzero identifier in the ordinary case. The constructor does not guarantee zero values for every alignment byte.

After that event append completes, the creature's byte at `+0x13C0` is set to one. The event does not contain a separate copy of that byte. The native sequence subsequently rejoins common resource handling, including the separately tested pending amount at `+0x144`; it does not wait for the resident node's later expiry before setting the latch. Allocation failure and abnormal termination are outside the normal-completion ordering stated here.

The common branch also has a class/specification exception at its entry: with `+0x140 = 4`, `+0x141 = 1`, action `5`, `17`, or `20`, and exact-zero action clock, it sets `+0x128` to `600`. This common-update lock write is separate from the selection helper's exception that preserves a lock value.

Anchors: original client executable dispatch at virtual addresses `0x00611E04..0x00611E1B` and its tables at `0x00622C84/0x00622CCC`; common ownership and special lock at `0x00612357..0x006123A9`; common zero-clock reset at `0x006134F1..0x00613531`; resource transfer, type-11 replacement, event, and latch at `0x0061364B..0x0061374F`, within world update RVA `0x20C510`. Original resident lookup is `decompiled/client/functions/00443f10.c`, RVA `0x043F10`; payload construction `0042c520.c`, RVA `0x02C520`; first-type replacement `00446af0.c`, RVA `0x046AF0`; event append `004460a0.c`, RVA `0x0460A0`.

## What the byte latch demonstrably changes

The observed direct reader of creature `+0x13C0` is in common-action hit processing at client virtual address `0x006144B4`. A nonzero byte makes the local special-hit Boolean true without calling the normal chance helper. With a zero byte, that Boolean comes from the chance helper at RVA `0x0444A0`.

That local Boolean has two confirmed downstream uses in the audited common-hit sequence. A true value multiplies the computed binary32 hit amount by exactly `2` before the native combat helper is called. The same Boolean is passed to that combat helper as a flag. The reader does not clear or decrement `+0x13C0`, and the byte is not looked up through the buff's embedded source identifier. This supports a latched common-hit effect, not a general claim that every projectile or every damage route consumes or reads the byte.

The random-draw distinction is exact. With the byte nonzero, the reader skips the normal chance-helper invocation and its random draw. Other draws in the common hit calculation still occur. With the byte zero, the chance helper searches resident type `11` itself, without a positive-duration or magnitude test. A found type `11` sets its probability threshold to one but still performs one random draw. It compares binary32 random value divided by `32767` strictly below that threshold. At threshold one, the maximum draw `32767` therefore fails the strict test; the byte-forced path does not have that endpoint failure. Without resident type `11`, the helper uses its separate base-stat result plus binary32 `0.15` times creature `+0x1190`, then performs the same draw and strict comparison. The full base-stat formula is outside this contract.

The byte's lifetime is not the same as the resident record's lifetime. The consume path writes a zero-duration record and sets the byte; this reader uses the byte independently of whether that record still exists. No automatic byte clearing on buff-record expiry is established. Starting an action that bypasses the common zero-clock reset does not, through that initializer, clear it either.

Anchors: chance helper `decompiled/client/functions/004444a0.c`, RVA `0x0444A0`, especially virtual addresses `0x004444A7..0x0044451E`; common byte read and Boolean construction `0x006144A8..0x006144D6`; doubling `0x0061467B..0x00614694`; flag passed to combat call at `0x00614757..0x0061476B`. This bounds the byte's local effect without importing the complete semantics of combat helper RVA `0x196D30`.

## Direct resets, client/server comparison, and explicit gaps

The original client has direct byte-zero writes at creature construction, a broad creature reset, a further lifecycle reset, and the common zero-clock initializer. Their virtual addresses are `0x0043B930`, `0x004464A7`, `0x004471CA`, and `0x00613520`. The consume path's direct byte-one write is at `0x00613748`; the direct field reader above is at `0x006144B4`.

A check of original executable displacement bytes agrees with those direct field references. A superficially matching reference at virtual address `0x00498419` is a stack address, not a reader or writer of creature `+0x13C0`. These are direct literal-offset findings; they do not prove that no indirect alias, bulk copy, or external routine can ever touch the storage. The lifecycle reset's callers and all lifecycle transitions have not been exhaustively audited here.

Anchors for the client reset routines: `decompiled/client/functions/0043b690.c`, `00446330.c`, and `00447110.c`, with matching original assembly and image bytes. Only the byte-write behavior of these broader routines is included here.

The server image contains corresponding mechanisms; these are not client-exclusive inventions. Its selected-action helper at RVA `0x010690` and selector at `0x010290` have the same five-value type-9 gate, the same reachable selection set, the same shortcut numerators, and the same admission relationships described above. Its common branch has the matching ownership and special-lock boundary at virtual address `0x00538117`; type-11 resource transfer and replacement/event/latch sequence at `0x0053943D..0x0053950F`; direct byte reader at `0x0053A274`; and doubled-amount step at `0x0053A43B..0x0053A454`.

Server direct reset evidence includes construction at virtual address `0x004066A0`, broad reset at `0x00411247`, and common initialization at `0x005392E0`. The server chance helper called when its byte is zero is RVA `0x00F520`. A complete equivalence claim for all client/server callers, networking, random streams, lifecycle resets, or action bodies is not made. The matching source boundaries above establish the shared mechanisms while retaining separate image identities and addresses.

Server anchors: `decompiled/server/functions/00410690.c`, `00410290.c`, and the stated spans of `decompiled/server/assembly.txt` and `binary/Server.exe`. Its anticipation constants at virtual addresses `0x00558788`, `0x005588FC`, `0x00558900`, and `0x005588F8` are respectively `600`, `1200`, `1600`, and `1000`.

Unclosed dependencies include all caller-specific input/AI gates, a normal selectable route to action `95` through any other boundary, every old-action total-duration case, the complete tempo and base-probability formulas, exceptional arithmetic inputs, the exact hit-admission intervals and victim filtering of every common action, and the remainder of downstream combat and event transport. The type-9 helper's absence of consumption/event/source mutation and the type-11 path's replacement, self/self event, zero embedded source, byte write, direct read effect, and explicit reset boundaries are the held result.
