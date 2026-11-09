# Alpha client common-action processing state and resource amounts

Status: FINAL / STABLE / HELD. This is an independently worded, bounded source-side behavioral contract. It does not certify a replacement implementation or runtime acceptance.

The evidence root is `disassembled alpha cube world`. The supported original image is `binary/Cube.exe`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, with preferred image base `0x00400000`. Addresses below are RVAs; creature and world offsets are explicitly marked as offsets. Evidence consists of individually identified original client routines, original assembly, and the supported image. No other contract, reconstruction, replacement implementation, runtime capture, test, or implementation history supplies facts here. No server counterpart is established.

The byte at creature `+0x13B8` is distinct from both the resource amount at `+0x13B4` and the byte at `+0x13C0`. In the audited common-action processing, `+0x13B8` is cleared at or before anticipation, can also be cleared by a positive pending-resource amount, and is set to 1 before a later target-processing call. The zero-clock initializer does not itself clear this byte unconditionally. Its only identified direct reader governs a counter/clock adjustment before resource initialization; it does not establish that a hit succeeded or directly reject later target candidates.

## Field and timing boundaries

| Creature offset | Native width and role in this contract |
| --- | --- |
| `+0x60` | One-byte control discriminator used by the common body's ownership gate. |
| `+0x68` | One-byte action identifier; action numbers below are decimal. |
| `+0x6C` | Signed 32-bit current action clock. |
| `+0x70` | Signed 32-bit action-related counter; one audited reader path writes zero to it. |
| `+0x128` | 32-bit lock value, tested for exact zero on that counter-adjustment path. |
| `+0x140`, `+0x141` | One-byte class and specialization discriminators. |
| `+0x144` | Binary32 pending-resource amount. The shared finalization phase always writes positive zero after inspecting it. |
| `+0x170` | Binary32 mana/resource quantity. |
| `+0x17C` | Binary32 timing multiplier used by anticipation and other timing helpers. |
| `+0x1190` | Binary32 quantity with one later, bounded read/update described below. Its complete meaning is not established here. |
| `+0x13B4` | Binary32 recorded amount used by common-action processing. It is not a reliable total of all mana debits. |
| `+0x13B8` | One-byte processing state. Direct writes found here store 0 or 1; its reader distinguishes zero from every nonzero byte. |
| `+0x13C0` | A separate one-byte common-action latch. Its zero-clock reset is described only to distinguish it from `+0x13B8`. |

The outer update routine at `0x20C510` receives a 32-bit integer update amount. It copies that argument into the value used by the common body's clock-plus-update comparisons; these inspected comparisons perform no float-to-time conversion. Its upstream time source, unit conversion, valid range, and scheduling are outside this contract. Clock additions use native 32-bit arithmetic and signed comparisons, without a local saturation or overflow guard.

“Anticipation” below is the signed integer result of the current-action helper at `0x03CAA0`, called with the native negative selector that requests the current action. The middle contribution is the corresponding integer result at `0x047310`; full duration is the result at `0x03D1A0`. Their complete action timing tables and stat-factor construction are dependencies outside this document. For actions 31 and 33, anticipation specifically uses binary32 numerator 1600 divided by the binary32 product of the timing-factor result at `0x047700` and creature `+0x17C`, then truncates toward zero to signed 32-bit. The timing-factor return is stored as binary32 before this arithmetic. Their middle contribution is fixed integer 1200. No constant anticipation of 1600 is implied when the denominator differs from 1.

Sources: `decompiled/client/assembly.txt`, original instructions `0x20C549–0x20C54F`; `decompiled/client/functions/0043caa0.c`, `00447310.c`, `0043d1a0.c`, `00447700.c`; `binary/Cube.exe`, the corresponding routine entries and common-body instruction ranges cited below.

## Common-action membership and ownership

The native outer dispatch maps exactly these action bytes to the common body at `0x212357`: **1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 30, 31, 32, 33, 51, 54, 57, 58, 60, 61, 62, 65, 66, 67, 68, 69, 70, 72, 74, 75, 76, 77, 78, 86, 87, 88, 91, 93, 104**. The initializer and direct state-byte reader described here are in this common region. This list does not establish identical per-target behavior for all its members. Other action bodies are outside this contract, including physical-projectile actions 22/23/26 and the separate magic-projectile body.

The common body admits a creature when world byte `+0xB4` is nonzero. With that byte zero, a nonzero creature control byte `+0x60` also admits it. When both bytes are zero, the creature pointer must equal the world's native 32-bit pointer at world `+0xB8`; otherwise this common region is skipped. These exact tests bound the writes and reads below. They are not a complete model of the original client's earlier living-creature gates, remote updates, or a server executable.

At entry, class 4, specialization 1, action 5, 17, or 20, and clock exactly zero write integer 600 to creature `+0x128`. This entry write can prevent the later counter/clock adjustment that requires a zero lock. It does not prevent the resource initializer described below from running.

Sources: `binary/Cube.exe`, dispatch instructions `0x211E04–0x211E1B`, selector bytes at `0x222CCC`, target table at `0x222C84`, ownership and entry lock instructions `0x212357–0x2123A9`; matching original ranges in `decompiled/client/assembly.txt`.

## Direct lifetime of the byte at `+0x13B8`

The original creature-construction region writes zero to `+0x13B8` at `0x03B91F`, after writing zero to `+0x13B4`. The bounded executable-text inventory identified these five direct creature-field accesses to the literal displacement `+0x13B8`:

| Native site | Established access |
| --- | --- |
| `0x03B91F` | Construction write of byte 0. |
| `0x2131F9` | Common-body write of byte 0 after the at-or-before-anticipation clearing work. |
| `0x213448` | Common-body comparison of the byte with 0. |
| `0x2135CF` | Common resource-finalization write of byte 0 after consuming a positive pending amount. |
| `0x2146E9` | Common target-processing write of byte 1 before the hit-resolution call. |

There are five sites in the table, including construction; four are in the common body. Other literal byte-pattern hits in the image were immediate integer values or stack-relative addresses, rather than direct accesses to this creature field. This inventory does not prove the absence of a bulk copy, aliased access through a differently based pointer, a computed displacement, or an indirect mutation by an unaudited dependency.

Before resource initialization, the common body compares the current signed action clock with anticipation. A clock strictly greater than anticipation bypasses the clearing phase. A clock less than or equal to anticipation reaches that phase, which clears native collections associated with creature offsets `+0x11AC` and `+0x11B4` and then writes byte zero at `+0x13B8`. Equality therefore clears the byte. The write is not restricted to clock zero, and negative clocks are not given a separate exemption. Full meanings of those collections and their associated events are not specified here.

The direct reader follows this phase. Any nonzero `+0x13B8` bypasses the following counter/clock adjustment. A zero byte permits consideration of that adjustment, but is not sufficient by itself. The clock plus the integer update amount must be at least anticipation plus the middle contribution; the current clock must still be strictly below full duration; and `+0x128` must be exactly zero. Equality at anticipation plus middle qualifies for this check, while equality at full duration prevents the adjustment. A nonzero lock prevents the adjustment without preventing subsequent resource finalization.

When all those conditions hold, the native timing factor is computed and stored as binary32. The counter at `+0x70` is then set to zero, and the timing factor is computed again. The current clock is converted to binary32 and multiplied by the binary32 ratio of the pre-reset factor to the post-reset factor; the result is truncated toward zero to signed 32-bit and stored back to `+0x6C`. This rescaling uses the current clock, without adding the update amount to that stored value. The direct reader and rescaling do not write `+0x13B8`. Resource initialization sees the clock after any such adjustment.

The later write of 1 occurs within target processing, after a native bookkeeping call on creature `+0x11AC` at `0x2146DE` and before the hit-resolution helper at `0x196D30` is called from `0x21476B`. The helper's success/failure result is tested afterward. Therefore reaching this write does not require that the later call return success. The byte is not cleared by that test. The original target filters, per-action geometry, target collection policies, and the full conditions for reaching this site are bounded dependencies; this contract does not assert that every common action sets the byte on every update or whenever any target is nearby.

The reader at `0x213448` is the sole identified direct reader of this creature byte. It appears before initialization and target processing. It supplies no direct per-candidate rejection in the later loop. The state byte must not be conflated with the separate temporary resource indicator produced by the initializer, or with `+0x13C0`.

Sources: `binary/Cube.exe`, construction `0x03B915–0x03B926`, anticipation comparison `0x213052–0x213077`, clearing tail `0x2131DB–0x213200`, reader and rescaling `0x213448–0x2134EE`, pending reset `0x2135A2–0x2135D6`, target-processing order `0x2146CA–0x214772`; matching original ranges in `decompiled/client/assembly.txt`. The executable-text literal-displacement inventory was checked against the supported image, with native stack and immediate-value matches excluded from the creature-field list.

## Ordinary zero-clock initialization

Actions 31 and 33 take their separate charge path before the ordinary zero-clock test. For every other action entering this common initializer, a clock of exactly zero resets byte `+0x13C0` to 0, then writes binary32 positive zero to `+0x13B4`. This pair of writes does not include a reset of `+0x13B8`. A nonzero clock, including a negative clock, bypasses the pair and preserves the previous recorded amount until another operation below overwrites it. Both paths still reach shared pending-resource finalization.

At ordinary clock zero, the nominal cost helper at `0x044AE0` is called with the current action and the native negative auxiliary selector. Its result is stored as binary32 and compared with zero. Only an ordered, strictly positive result selects direct cost spending. In that case the helper is called again, and that second binary32 result is written to `+0x13B4` and subtracted from mana at `+0x170`. There is no affordability test, no cap to available mana, and no direct reset of `+0x13B8` on this positive-cost operation. Mana clamping occurs later in shared finalization, after any pending amount is handled.

A zero or negative nominal result follows the nonpositive branch and is not subtracted as a cost; negative cost is not a direct mana refund here. An unordered comparison also selects that branch under the inspected comparison instructions. Within the supported common-action membership, however, the cost helper supplies fixed binary32 0.1 for actions 3 and 4, type-9-dependent zero or binary32 0.3 for charge actions 31 and 33, and zero for every other member. A negative nominal result for a listed common action is not established by the original cost lookup. Other action values handled by that helper do not establish reachability through this common dispatch.

The nonpositive ordinary branch has the following action-specific effects before pending-resource finalization:

| Current action | Recorded amount and mana effect at ordinary clock zero |
| --- | --- |
| 5, 17, 20 | Copy the current four-byte mana representation into `+0x13B4`, then set mana to positive zero. There is no requirement for a positive copied amount. These actions then have resident type-11-specific processing before shared finalization. |
| 21, 54, 87, 88 | Set `+0x13B4` to binary32 1.0 without a corresponding mana debit at this operation. |
| Every other common member except 31/33 | Retain the positive-zero amount written at the beginning of ordinary initialization; no nominal-cost debit occurs. |

The type-11-specific operations for actions 5/17/20 include a presence lookup, resident/event helper calls when present, and a later write of 1 to `+0x13C0`. They do not supply a direct write to `+0x13B8`. Detailed buff payload, source-identifier and replication behavior is outside this contract. This ordering places the mana copy/reset and any such buff processing before the shared pending amount can overwrite the recorded amount.

Sources: `binary/Cube.exe`, ordinary route selection `0x2134F7–0x213514`, resets and cost handling `0x21351A–0x213593`, nonpositive action cases `0x21364B–0x21369D`, buff-specific continuation `0x21369D–0x21374F`; `decompiled/client/functions/00444ae0.c`; matching original instructions in `decompiled/client/assembly.txt`.

## Charge actions 31 and 33

These two action bytes bypass ordinary zero-clock initialization regardless of the clock value. They therefore do not receive its unconditional `+0x13B4` or `+0x13C0` resets. Their earlier at-or-before-anticipation phase can still clear `+0x13B8`, and shared pending-resource finalization still applies.

At the charge-specific phase, a current clock strictly greater than anticipation performs no charge-cost operation. A current clock less than or equal to anticipation is checked against the projected clock after the integer update. Only a projected clock strictly greater than anticipation performs the charge-cost operation. Equality of the projected clock with anticipation does not charge. A current clock exactly equal to anticipation can charge when the projected clock is greater. Thus this phase requires crossing out of the inclusive anticipation interval; it does not require a current clock of zero.

When that crossing is detected, `+0x13B4` is first assigned fixed binary32 0.1, bit pattern `0x3DCCCCCD` (approximately 0.10000000149011612). The nominal cost helper is called once and its binary32 result is subtracted from mana without a cost-positivity test. An ordered negative resulting mana value is then clamped to positive zero before pending-resource processing. This recorded 0.1 is independent of whether the nominal debit is binary32 0.3 or zero, and independent of how much mana was available.

For these actions, the nominal helper returns zero when the resident list at `+0x1178` contains type 9 and binary32 0.3 otherwise. That presence check does not require positive remaining duration or magnitude. The charge-cost operation itself does not remove or overwrite the type-9 record. Any later buff consumption is outside this initializer contract.

When no crossing occurs, the charge phase leaves the prior `+0x13B4` representation intact until shared finalization. In particular, clock zero for action 31/33 does not by itself erase an amount from the prior state. No direct `+0x13B8` or `+0x13C0` write occurs inside this charge-cost phase. Any positive pending amount subsequently has the overwrite and reset effects in the next section.

Sources: `binary/Cube.exe`, charge selection `0x2134F7–0x213504`, charge comparison and debit `0x213754–0x2137D4`, no-crossing continuation `0x2137E1–0x2137E4`; cost helper at `0x044AE0`; `decompiled/client/functions/00444ae0.c`, `0043caa0.c`; matching original ranges in `decompiled/client/assembly.txt`.

## Shared pending-resource finalization

This phase is shared by ordinary zero-clock initialization, ordinary nonzero-clock processing, and the charge paths both with and without a crossing. It runs before the later target-window test. It is not restricted to a new action or to an update that ultimately processes a target.

An ordered, strictly positive binary32 pending amount at `+0x144` first replaces the recorded amount at `+0x13B4`. It is an overwrite, not an addition to the prior nominal, full-mana, fixed-one, or charge amount. The full pending amount is then subtracted from the current mana and stored. Byte `+0x13B8` is reset to 0 after that subtraction. For current action 5 or 11, the recorded amount is subsequently multiplied by binary32 0.5; the mana debit remains the full pending amount. Other common actions retain the full pending amount as their new recorded amount.

Zero and negative pending values do not debit mana, overwrite `+0x13B4`, or clear `+0x13B8` through this operation. An unordered pending comparison also skips those positive-only effects. Every route nevertheless clears `+0x144` to binary32 positive zero. The field is not retained as a negative credit or retry request.

After clearing the pending field, the shared phase clamps mana to positive zero only when zero is ordered greater than the current mana. Equal zero, including negative zero, is not changed by that clamp; an unordered mana comparison skips it. There is no upper clamp to 1 in this initializer. Pending subtraction and the other resource arithmetic use binary32 operations in the stated order.

The phase then produces a temporary one-byte resource indicator: an ordered, strictly positive recorded amount makes it true; otherwise action 11 alone also makes it true; otherwise it is false. The comparison threshold is exact binary32 zero at image RVA `0x345D84`, not a positive tolerance. This temporary indicator is not stored into `+0x13B8` or `+0x13C0`.

The resulting recorded amount need not equal the mana decrease. With both a positive nominal cost and positive pending amount, mana has both debits but the recorded amount contains the pending amount alone. For actions 5/11 it can contain half the pending debit. A fixed-one action can record 1 without nominal spending, and a charge action can record 0.1 while nominal spending is zero or 0.3. Resource shortage can also make actual available-mana reduction smaller than the recorded amount because clamping follows subtraction. These distinctions are native behavior of the inspected phase.

Sources: `binary/Cube.exe`, common finalization `0x213593–0x213646`, nonzero-clock continuation `0x2137D9–0x2137DC`, charge continuations `0x213766–0x2137E4`, temporary-indicator storage `0x2137E9–0x2137F1`; binary32 constants at RVAs `0x345D84` (0), `0x345DA0` (0.5), `0x345DC0` (1), `0x2FCB7C` (0.1), and `0x2FCDC0` (0.3); matching original ranges in `decompiled/client/assembly.txt`.

## Bounded downstream uses and remaining dependencies

The later common target-processing call at `0x21476B` receives both the current recorded amount read from `+0x13B4` and the temporary resource indicator described above. The write of 1 to `+0x13B8` precedes this call. Successful return is tested separately. The full behavior of the hit-resolution helper at `0x196D30`, including damage, hit acceptance, random draws and event production, remains outside this contract.

After a successful return, a true temporary resource indicator together with class 4 and specialization 0 allows the current recorded amount to be added to creature binary32 `+0x1190`. An ordered result greater than 1 is then clamped to binary32 1. This is a later use of the recorded amount; it does not clear that amount or `+0x13B8`. No complete meaning or lifetime for `+0x1190` is inferred from this one use.

Outside the common initializer, direct zero writes to `+0x13B4` also occur in the construction region and at native sites `0x04649D` and `0x0471C0`. There is no corresponding direct `+0x13B8` access at those latter two sites in the literal-displacement inventory. Their complete lifecycle behavior, bulk operations, and all other reads of the recorded amount are not audited here. Likewise, the producers of pending `+0x144`, input-start behavior, hardware input mapping, general creature serialization, and ownership changes remain unclosed.

The direct state-byte inventory and initializer boundaries do not authorize treating `+0x13B8` as a guaranteed successful-hit marker, resetting it on every action assignment, equating `+0x13B4` with total mana spent, or moving pending-resource consumption inside the ordinary zero-clock gate. Those broader relations are not supported by the inspected original behavior.

Sources: `binary/Cube.exe`, later argument formation and result test `0x21473A–0x214772`, recorded-amount addition `0x2148B9–0x214906`, additional recorded-amount resets `0x03B915`, `0x04649D`, and `0x0471C0`; matching original instructions in `decompiled/client/assembly.txt`. Target membership, detailed per-action windows, all collection-helper effects, complete stat/timing formulas, non-finite timing behavior and server behavior remain explicit dependencies.
