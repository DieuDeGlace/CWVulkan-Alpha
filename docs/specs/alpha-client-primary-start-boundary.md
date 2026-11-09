# Alpha client ordinary-primary selection and start boundary

Status: FINAL / STABLE / HELD. This is an independently worded source-side contract for a bounded client input-helper boundary. It does not certify a replacement implementation or native runtime acceptance.

The evidence root is `disassembled alpha cube world`. The supported image is `binary/Cube.exe`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, with preferred image base `0x00400000`. All routine and instruction addresses below are RVAs unless explicitly described as field offsets or table locations. The scope is the ordinary-primary selector at `0x045020`, its start helper at `0x045410`, their native timing dependencies, and limited surrounding caller and common-action boundaries. No server equivalence is claimed. No reconstruction, other contract, runtime capture, replacement implementation, or implementation verification material supplies evidence here.

The central distinction is that the start helper's true return byte does not prove an action started. A nonzero lock or old action 48 produces false. After those two exclusions, the helper returns true both when it replaces the action and when the old action has not reached the replacement threshold. Only the replacement case invokes the selector and writes the action and clock. Sources: `decompiled/client/functions/00445410.c`; `binary/Cube.exe`, `0x045417–0x04542E` and `0x0455CA–0x0455EE`.

## Fields and numeric conventions

All creature offsets are relative to the creature passed to the helper. Equality tests on byte fields use the stored eight bits. Decimal action values are used throughout.

| Offset | Width and use at this boundary |
| --- | --- |
| `+0x60` | One-byte control discriminator. It can affect the timing factor; neither helper uses it as a direct ownership or admission gate. |
| `+0x64` | 32-bit creature discriminator, compared for equality with 117, 86, and 104 by the selector. |
| `+0x68` | One-byte old/current action; replaced only after readiness succeeds. |
| `+0x6C` | Signed 32-bit action clock, compared with signed integer timing thresholds. |
| `+0x70` | 32-bit action-related counter. The start helper distinguishes zero from every nonzero value. It is not reset here. |
| `+0x7E` | One-byte flag access in the selector; bit `0x10` participates late in selection. A surrounding caller also reads a word beginning at this offset. |
| `+0x128` | 32-bit lock value. Any nonzero bit pattern prevents the attempt; negative values have no exception. |
| `+0x140`, `+0x141` | One-byte class and specialization discriminators. The numeric tests below do not require gameplay names. |
| `+0x170` | Binary32 mana/resource quantity; neither input helper reads or writes it. |
| `+0x17C` | Binary32 multiplier used in the timing denominator. |
| `+0x990`, `+0x991` | One-byte pair used by lower-precedence selection tests. |
| `+0xAA8`, `+0xAA9` | One-byte pair used by several selection tests, including the highest-precedence family. |
| `+0x1178` | Native 32-bit pointer to the resident-buff list sentinel. The timing dependency can inspect type 12; neither input helper performs a type-9 admission or consumption operation. |
| `+0x13C0` | One-byte common-action latch. Neither input helper accesses it. |

No frame duration is supplied to these helpers. They compare the already stored action clock with native integer timing quantities. The tables below preserve their native units; conversion from real elapsed time into the clock belongs to the outer update and is outside this contract.

## Complete selector precedence and results

The selector at `0x045020` only returns an action value. It does not install the value or reset the clock. The first applicable family in the following table wins. Later rows do not override an earlier match. Within rows that alternate actions, the old action and old clock remain available throughout selection; the caller writes the new state afterward.

“Within the combo window” means the signed old clock is strictly less than the old action's full duration plus 200. Equality is outside that window. The comparison uses the full duration at `0x03D1A0`, without subtracting recovery, and does not directly test the counter at `+0x70`. The timing factor used by that duration can nevertheless depend on the counter.

| Precedence | Selection condition | Returned action |
| --- | --- | --- |
| 1 | Class byte `+0x140` is 3, or creature discriminator `+0x64` is 117 or 86, or `+0xAA8` is 3 with `+0xAA9` equal to 10, 11, or 12 | Use the family-specific results immediately below this table. |
| 2 | Discriminator `+0x64` is 104, or `+0xAA8` is 3 with `+0xAA9` equal to 6 or 7 | 22 |
| 3 | `+0x990` is 3 and `+0x991` is 6 or 7 | 22 |
| 4 | `+0xAA9` is 3 | 18 when the old action is 19 and within its combo window; otherwise 19. The test does not also require `+0xAA8` to be 3. |
| 5 | `+0xAA9` is 4, or `+0xAA8` is zero | 7 when the old action is 6 and within its combo window; otherwise 6. Thus a zero `+0xAA8` takes precedence over every following subtype test. |
| 6 | `+0xAA9` is 5 | 13 when the old action is 14 and within its combo window; otherwise 14. |
| 7 | `+0xAA8` is 3 and `+0xAA9` is 8 | 26 |
| 8 | The predicate at `0x044230` is true: `+0xAA8` is 3 and `+0xAA9` is one of 5, 6, 7, 8, 10, 11, 15, 16, 17, or 18 | 67 when the old action is 57 and within its combo window; 58 when the old action is 67 and within its combo window; otherwise 57. Because earlier rows have already handled the other qualifying subtypes, this row is reached for subtypes 15, 16, 17, or 18. |
| 9 | `+0x990` is 3 and `+0x991` is 13 | 9 when the old action is 10 and within its combo window; otherwise 10. |
| 10 | Bit `0x10` of `+0x7E` is set | 75 |
| 11 | No earlier condition applies | 2 when the old action is 1 and within its combo window; otherwise 1. |

The highest-precedence family has its own complete internal priority. A subtype byte `+0xAA9` of 11 returns 44 for specialization byte `+0x141` equal to 1 and 38 for every other specialization value. A subtype of 10 returns 32 for specialization 1 and 30 otherwise. For every remaining subtype, specialization 1 returns 41 when the old action is 42 and within its combo window, and 42 otherwise. Every other specialization returns 39 when the old action is 40 and within its combo window, and 40 otherwise. The subtype 10 and 11 results require no old-action or clock test once this family has matched.

This priority matters when class or creature discriminator alone admits the first family. For example, class 3 with the pair `+0xAA8`/`+0xAA9` equal to 3/6 still uses that first family; it does not reach the later action-22 row. Conversely, neither a valid family match nor a flag grants admission past the start helper's earlier lock and action-48 exclusions.

The complete returned-action set is **1, 2, 6, 7, 9, 10, 13, 14, 18, 19, 22, 26, 30, 32, 38, 39, 40, 41, 42, 44, 57, 58, 67, 75**. The selector has no zero result and no failure result for unmatched equipment/discriminator combinations. It does not return 5, 17, 20, 31, 33, 37, 43, 45, or 46.

Sources: `decompiled/client/functions/00445020.c`, `00444230.c`; `binary/Cube.exe`, selector body `0x045020–0x04529C`, especially initial family precedence `0x045023–0x04508E`, remaining precedence `0x045090–0x045210`, highest-family results `0x045211–0x04529C`, and predicate `0x044230`.

## Start admission, readiness, and accepted writes

The start helper first rejects a nonzero 32-bit value at `+0x128`. It next rejects old action 48. These are unconditional within this helper: no class, specialization, equipment pair, mana amount, buff, counter, action clock, or ownership value supplies an exception. The lock field is neither decremented nor cleared. A rejected attempt returns zero in the low result byte and never invokes the duration helper or selector.

After these exclusions, the helper obtains the old action's full signed 32-bit duration. If the counter at `+0x70` is zero, readiness requires the old signed action clock to be at least that full duration. Old actions 30 and 32 also require the full duration even when the counter is nonzero. For every other old action with a nonzero counter, readiness requires the old clock to be at least the full duration minus the old action's recovery quantity. Equality is ready in both cases.

Recovery subtraction is a property of the old action, not the proposed new action. The helper does not select a proposed action before deciding readiness. An old action 30 or 32 cannot use the recovery subtraction merely because the selector would choose a different family. A negative counter is still nonzero for this gate, although it can independently affect timing-factor calculations.

When readiness fails, the old action and clock are left in place and the selector is not called. The low result byte is nevertheless 1. When readiness succeeds, the helper calls the selector once, stores its low byte to `+0x68`, then stores signed 32-bit zero to `+0x6C`. It returns low byte 1 after those writes. A same-action result still performs both writes, so it restarts that action at clock zero. Neither path establishes meaningful values in the other result-register bits.

The accepted writes are limited to that one-byte action followed by that four-byte clock. In particular, acceptance does not clear `+0x128`, reset `+0x70`, change the creature/source identifier, alter resident buff records, debit mana, set a pending-resource amount, or write `+0x13C0`. A resident type 9 does not move this new clock to an anticipation boundary: the stored clock is exactly zero.

Sources: `decompiled/client/functions/00445410.c`; `binary/Cube.exe`, admission `0x045417–0x04542E`, full-duration and counter/action exceptions `0x04542F–0x045455`, recovery subtraction `0x0455B8–0x0455C8`, signed readiness and writes `0x0455CA–0x0455DA`, returns `0x0455E1–0x0455EE`.

## Exact timing quantities used by the gate

The full-duration helper at `0x03D1A0` adds three signed 32-bit integer contributions: an opening contribution, a middle contribution, and recovery. These labels identify timing components for this contract; they do not establish that every action body treats them as animation, attack, or release phases. The following tables close the duration lookup even for an old action outside the selector's 24-value result set.

In these tables, **fixed** means the listed integer is used directly. **Scaled** means the listed binary32 numerator is divided by the binary32 product of the native timing-factor result at `0x047700` and creature binary32 `+0x17C`. The timing-factor result is first stored as binary32; multiplication and division use binary32 arithmetic. Each component's quotient is separately converted to signed 32-bit by truncation toward zero. Numerators must not be added together before that division and conversion. A scaled recovery in the start helper recomputes the timing factor after the full-duration helper; it does not reuse an intermediate quotient from that helper. A combo-window check in the selector can then call the full-duration helper again, still using the old state.

The native operations are 32-bit additions and subtraction, with signed comparisons and no saturating arithmetic or nonnegative threshold clamp. Ordinary finite, representable timing values are covered by the tables. This contract supplies no substitute behavior for zero/negative timing denominators, non-finite values, out-of-range float-to-integer results, or altered floating-point exception settings.

| Old action values | Opening contribution |
| --- | --- |
| 0, 8, 11, 28, 50, 55, 60, 61, 62, 96, 98, 104 | Fixed 0 |
| 48, 101 | Fixed 100 |
| 54 | Fixed 400 |
| 71, 72 | Fixed 200 |
| 105 | Fixed 5000 |
| 6, 7, 23, 24, 25, 27, 36, 59, 63, 64 | Scaled 50 |
| 2, 3, 4, 5, 10, 14, 17, 18, 19, 20, 21 | Scaled 100 |
| 12, 13, 15, 16, 39, 40, 41, 42, 67 | Scaled 200 |
| 1, 9, 26, 65, 66, 68, 69, 70, 73, 74, 75, 76, 77, 78 | Scaled 300 |
| 38, 44, 94 | Scaled 500 |
| 30, 32, 57, 58, 93 | Scaled 800 |
| 91, 95 | Scaled 1000 |
| 45, 46 | Scaled 1200 |
| 31, 33, 34 | Scaled 1600 |
| 87 | Scaled 5000 |
| 37, 43, 89 | Scaled 600 when byte `+0xAA9` is 12; scaled 1200 otherwise. There is no additional `+0xAA8` test here. |
| Every other one-byte action value, including 22 | Scaled 400 |

| Old action values | Middle contribution |
| --- | --- |
| 0, 49 | Fixed 0 |
| 54 | Fixed 100 |
| 10 | Fixed 200 |
| 50, 76, 77, 78, 96 | Fixed 500 |
| 30, 32, 73, 93 | Fixed 600 |
| 68, 69 | Fixed 1000 |
| 31, 33 | Fixed 1200 |
| 95 | Fixed 2000 |
| 71 | Fixed 3000 |
| 72, 86 | Fixed 5000 |
| 91 | Fixed 6000 |
| 22, 23 | Scaled 50 |
| 3, 4, 37, 43, 62 | Scaled 100 |
| 6, 7, 18, 19 | Scaled 150 |
| 1, 2, 9, 13, 14, 15, 67 | Scaled 200 |
| 5, 12, 16, 17, 20, 21, 65, 66 | Scaled 400 |
| 45, 46, 55, 94 | Scaled 500 |
| 26, 34 | Scaled 1200 |
| Every other one-byte action value | Scaled 300 |

| Old action values | Recovery contribution and the start helper's subtraction quantity |
| --- | --- |
| 0, 50, 96 | Fixed 100 |
| 10 | Fixed 600 |
| 48 | Fixed 0; the primary start helper rejects this old action before timing evaluation. |
| 54 | Fixed 400 |
| 71, 72 | Fixed 200 |
| 23 | Scaled 10 |
| 11, 22, 26, 30, 31, 32, 33, 34, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 60, 61, 67, 94, 95, 104 | Scaled 100 |
| 7, 14, 18, 65, 66 | Scaled 200 |
| 3, 4, 5, 57, 58, 62 | Scaled 300 |
| 15 | Scaled 400 |
| 68, 69, 93 | Scaled 800 |
| Every other one-byte action value | Scaled 500 |

For an unscaled illustration, old action 0 has full duration 100: counter zero requires clock at least 100, while any nonzero counter permits readiness at clock 0. For an old action 30 with a timing denominator of exactly 1, the full duration is 800 + 600 + 100 = 1500, and both zero and nonzero counters require clock at least 1500. A selector combo check against that old action's full duration, when applicable to its own action family, would use a strict upper boundary 200 later; the start helper's readiness comparison remains inclusive.

The timing factor itself is a separate stat dependency. The inspected native routine reads the control discriminator, class/specialization, counter and its cap, item-derived quantities, and resident type 12. Its resident-list test does not require positive remaining time or magnitude before applying its type-12-related timing effect. It does not test resident type 9. The item/stat and cap formulas are not fully specified by this contract; a consumer must not replace the native timing factor with a universal value of 1 or infer a universal duration from the numerator tables. These dependencies do not grant an exception to either initial admission exclusion.

Sources: `decompiled/client/functions/0043d1a0.c`, `00445410.c`, `00447700.c`, `00444cb0.c`, `004478b0.c`, `004c7c00.c`, `00445ff0.c`; `binary/Cube.exe`, duration `0x03D1A0–0x03D9C5`, opening selector bytes at `0x03DA54` and target table at `0x03D9C8`, middle selector bytes at `0x03DB40` and target table at `0x03DAC0`, recovery selector bytes at `0x03DBEC` and target table at `0x03DBAC`, start-helper recovery selector bytes at `0x045630` and target table at `0x0455F0`. Binary32 constants are in the same image at RVAs `0x2FCD7C` (50), `0x2FCE60` (200), `0x2FCE68` (300), `0x2FCE70` (500), `0x2FCE74` (600), `0x2FD0B4` (100), `0x2FD8E8` (150), `0x2FD8EC` (400), `0x2FD8F0` (800), `0x2FD8F4` (1000), `0x2FD8F8` (1200), `0x2FD8FC` (1600), `0x2FD900` (5000), and `0x345E74` (10).

## Mana, buffs, source identifiers, events, and random draws

There is no mana minimum for either the primary selector or primary start helper. The start helper does not call the action-cost helper at `0x044AE0`, does not inspect mana at `+0x170`, and does not reserve or spend it. Every action in the selector's complete result set also takes the zero-cost default in that cost helper. This is a statement about that cost lookup and this input boundary, not a guarantee that all later action execution is resource-free.

Resident type 9 is absent from the input admission and mutation rules. Neither helper searches for it, overwrites it, appends a record, removes a record, changes an embedded source identifier, or constructs a buff event. There is no consume-on-primary-start action here. Other resident records likewise remain untouched. The timing dependency's type-12 presence test only changes computed timing.

Neither helper nor the inspected selection/timing/stat dependencies draws from the native random generator or appends a gameplay event. The selector predicate `0x044230` is a read-only byte-pair test. Timing uses stat calculations and mathematical library calls; it supplies no chance draw. A caller may have performed draws before invoking the helper, and later action execution may draw or generate events. Those effects must not be attributed to primary-helper acceptance.

Sources: `decompiled/client/functions/00445020.c`, `00445410.c`, `00444230.c`, `00444ae0.c`, `0043d1a0.c`, `00447700.c`, `00444cb0.c`, `004478b0.c`, `004c7c00.c`, `00445ff0.c`; corresponding image bodies at RVAs `0x045020`, `0x045410`, `0x044230`, `0x044AE0`, `0x03D1A0`, `0x047700`, `0x044CB0`, `0x0478B0`, `0x0C7C00`, and `0x045FF0`.

## Boundary with later common-action processing

The ordinary-primary helper does not itself run a per-action body. The inspected native outer-update dispatch sends primary results 1, 2, 6, 7, 9, 10, 13, 14, 18, 19, 30, 32, 57, 58, 67, and 75 to the common-action body at `0x212357`. Results 22 and 26 go to `0x21707A`; results 38, 39, 40, 41, 42, and 44 go to `0x217A21`. This contract does not extend the common-action initializer's effects to those two other bodies.

The common-action body has an ownership gate that is absent from the input helpers. A nonzero world byte `+0xB4` admits the creature. With that world byte zero, a nonzero creature control byte `+0x60` also admits it; a zero control byte instead requires the creature pointer to equal the world's native 32-bit pointer at `+0xB8`. This identifies the native conditions and does not infer a general client/server or local/remote guarantee for every caller.

At common-action entry, class 4 with specialization 1, action 5, 17, or 20, and clock exactly zero writes 600 to `+0x128`. Those three actions are absent from the ordinary-primary selector's result set. This later write is not a class-specific permission to bypass a preexisting nonzero lock in `0x045410`.

The later common initializer excludes actions 31 and 33 from its ordinary zero-clock initialization route. For the primary results that enter the common body, clock exactly zero allows that route to reset byte `+0x13C0` to 0 and binary32 `+0x13B4` to zero before its cost/resource processing. A nonzero clock skips that initialization. This is a subsequent world-update effect, not an additional accepted write of the input helper.

The initializer's type-11 consumption case is restricted to actions 5, 17, and 20 after the cost decision. It can replace a resident type-11 record, append a buff event, and then set `+0x13C0` to 1. None of these three actions is an ordinary-primary selector result, so this consumption case is not reached merely by accepting an ordinary primary through the two helpers covered here. An already existing latch is also not read by those helpers to admit or choose an action.

Even for an ordinary-primary result whose nominal cost lookup is zero, the later common initializer can process a positive binary32 pending amount at `+0x144`, debit mana at `+0x170`, record the amount at `+0x13B4`, and clear the pending field. The input helper performs no corresponding affordability check. Full pending-resource provenance, later hit behavior, physical/magic action bodies, and the end-to-end lifetime of `+0x13C0` are not established here.

Sources: `decompiled/client/assembly.txt`, original instructions at RVAs `0x211E04–0x211E1B`, `0x212357–0x2123A9`, and `0x2134F1–0x213754`; `binary/Cube.exe`, corresponding instruction ranges, outer dispatch selector bytes at `0x222CCC` and target table at `0x222C84`; `decompiled/client/functions/00444ae0.c`.

## Caller and ownership limits

Three direct native call sites to `0x045410` were identified in the supported image's executable text: `0x02DDBE`, `0x02E2D5`, and `0x09BF82`. All three continue without testing its returned low byte. Thus their following work cannot be interpreted as proof that this helper replaced the action. This inventory does not exclude an indirect call or a dynamically computed entry.

At the inspected local-control call site `0x09BF82`, the immediate conditions are a zero temporary byte in the caller's stack state, a controlled-creature old action other than 48, and a nonzero byte at caller-object `+0x04`. The creature pointer is read from caller-object `+0x8006D0`. Only then is the start helper called. These immediate tests do not prove what every earlier input/UI gate or the temporary byte means. Immediately afterward the caller may process an item whose byte at creature `+0x1020` is 20, independently of the helper's return byte; this surrounding item operation is not a mutation made by primary acceptance.

At `0x02DDBE`, a surrounding creature-update route first requires a true predicate result from `0x044610` and a caller-local binary32 quantity strictly below the image constant 3600. Mana at `+0x170` greater than binary32 0.5 causes a random draw whose odd parity chooses the secondary start helper instead. Mana at or below 0.5, or an unordered mana comparison, takes the primary route without this draw; an even parity draw also reaches the primary helper. The random draw precedes either helper's own admission checks. It may therefore occur even when the later primary attempt is rejected by its lock or action-48 test. This is a caller choice using mana, not a mana requirement imposed by the primary helper. The predicate's full meaning, the caller-local quantity's provenance, and all earlier entry conditions remain outside the audited boundary.

At `0x02E2D5`, the immediately preceding flag route uses bit `0x08` from the word beginning at creature `+0x7E`, after a separate bit-`0x10` route has taken precedence. Discriminator 104 and subtype values 6, 7, and 8 have separate earlier routes at this caller and do not arrive through this immediate branch. No helper return value is tested. The full creature-update decision tree and the meaning of these flag bits are not reconstructed here.

The selector also has a separate observed direct caller at `0x08BA17`, besides the start helper's call at `0x0455D2`. The surrounding `0x08BA17` caller stores its result into a temporary value; it is not evidence that a selector call itself starts an action. Its full UI purpose and all alternate action-start routes are left unresolved.

Sources: `decompiled/client/assembly.txt`, original routine region rooted at `0x02CB20` and caller instructions `0x02DD6C–0x02DDC3`, `0x02E109–0x02E117`, `0x02E25E–0x02E2DF`, `0x08BA17–0x08BA32`, and `0x09BF69–0x09BFC8`; `binary/Cube.exe`, same instructions and executable-text direct-call displacements. The caller constants are binary32 values at image RVAs `0x2FD334` (3600) and `0x345DA0` (0.5). Numeric native parity here describes the original draw selection only; no distribution claim beyond the native generator's output is required.

This contract closes the selector, the primary start helper's admission/result/write boundary, its old-action timing lookups, and the identified immediate phase distinctions. Hardware input mapping, the complete outer caller gates, indirect entries, timing-stat formulas, server behavior, general buff replication, later action-body randomness, and physical or magic execution remain explicit dependencies rather than inferred behavior.
