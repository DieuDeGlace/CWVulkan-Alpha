# Alpha living-creature update phases

Status: FINAL / STABLE / HELD. This is an independently worded, original-evidence-only contract for selected common creature-update phases. It is not a complete action catalog, physics specification, native runtime acceptance result, or provenance certification for another implementation. Earlier held contracts are unchanged.

## Image, sources, and scope

The supported original image is `binary/Cube.exe`, relative to the assigned `disassembled alpha cube world` evidence directory. It contains 3,885,568 bytes and has SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, PE machine `0x014c`, timestamp `0x51ea955e`, preferred base `0x00400000`, and image size `0x003bc000`. Every address below is an RVA. Field offsets describe the original 32-bit image and do not prescribe another program's memory layout.

The principal source is `decompiled/client/assembly-functions/0060c510.asm`, covering the original update routine at `0x20c510`. Its relevant common phases run from `0x211ae1` through `0x21ae70`. Individually selected original ranges in `decompiled/client/assembly.txt` supply dispatch destinations that the routine-only export omits. The dispatch tables at `0x222ccc` and `0x222c84`, image constants, and the original individual function files named below provide supporting evidence. No replacement engine, SDK, exporter, extractor, tests, probes, captures, logs, implementation history, existing specification, or semantic audit was inspected for this assignment. Reconstruction material was not used for this expansion.

The contract begins with a creature that reaches the living-creature boundary. Immediately before it, health at offset `0x16c` is capped to the result of `0x044db0`; health at most zero leaves this path. Earlier interactions, replication, spawning, and death processing are excluded. The species-specific health reduction mentioned below can subsequently reach zero without another health rejection before the common action phase.

Action effects and movement can change state between the documented common phases. References to an action, timer, buff, or resource mean its value when that phase is reached, after any preceding changes. They do not imply a snapshot taken at the routine's entry.

## Fields and arithmetic domain

| Creature offset | Width and representation | Use in this contract |
| --- | --- | --- |
| `0x60` | 8 bits | Control/hostility discriminator |
| `0x64` | 32 bits | Species discriminator |
| `0x68` | 8 bits | Current action |
| `0x6c` | signed 32 bits | Action clock, milliseconds |
| `0x70` | signed 32 bits | Combo count |
| `0x74` | signed 32 bits | Time since the last combo-producing event |
| `0x124` | 16-bit mask | Behavioral and movement toggles |
| `0x128` | signed 32 bits | Roll-related timer, read by later cancellation gates |
| `0x12c` | signed 32 bits | Remaining stun time |
| `0x130` | signed 32 bits | Action-lock time |
| `0x134`, `0x138` | signed 32 bits each | Two temporary movement-modifier timers |
| `0x13c` | binary32 | Negative recovery-penalty value |
| `0x140`, `0x141` | 8 bits each | Class and specialization discriminators |
| `0x144` | binary32 | Stored charge |
| `0x16c` | binary32 | Health |
| `0x170` | binary32 | Mana-like resource |
| `0x174` | binary32 | Blocking resource |
| `0x17c` | binary32 | Action-time and combo-limit scale |
| `0x190` | signed 32 bits | Level-like input used for non-player skill fallback |
| `0x1a8` | unsigned 8 bits | Additional skill-fallback input |
| `0x1150`, `0x1154`, `0x1158` | signed 32 bits each | Skill-point entries 6, 7, and 8 |
| `0x1178` | 32-bit list linkage | Active-buff collection |
| `0x139c` | 32-bit collection linkage | Action-keyed cooldown entries |
| `0x13c4` | signed 32 bits | Behavior-update accumulator |
| `0x13e4` | 32-bit pointer | Optional behavior object |

The world byte at `0xb4` is the side/replica gate used by this routine. The world pointer at `0xb8` identifies the locally owned creature for the comparisons below. “Local” means pointer equality with that field; “other” means inequality. This records exact native gates without assigning undocumented meanings to every nonzero control discriminator.

The routine receives a signed 32-bit millisecond argument. It retains the integer for timer arithmetic, also converts it to binary32 milliseconds, then multiplies that binary32 value by the binary32 constant with bits `0x3a83126f` to obtain seconds. The constant is approximately `0.001`, not an exact decimal rational. Evidence: `0x20c548` through `0x20c862`.

All continuous arithmetic specified below uses the native binary32 operations and intermediate stores. Integer conversions from binary32 truncate toward zero. This contract covers ordinary nonnegative update durations, finite values, valid state and collections, and arithmetic without integer overflow. Negative updates, out-of-range conversions, NaNs, infinities, concurrent mutation, and invalid state are not supported cases established here.

## Behavior, preliminary timers, and action effects

Behavior time accumulates only when control discriminator `0x60` is nonzero and world byte `0xb4` is zero. The incoming integer milliseconds are added before checking stun. Behavior is eligible when the existing stun timer is at most zero and the accumulator is at least 32 ms. If the behavior pointer exists, its virtual update receives the creature, world, whole accumulator, and outgoing-update object. Upon reaching this eligible accumulator boundary, the accumulator is reset to zero even if the behavior pointer is null. A positive stun prevents that reset, allowing elapsed behavior time to accumulate. Positive stun on this path also clears toggle mask `0x0080`. Evidence: `0x211ae1` through `0x211b75`.

There is a separate species-144 health reduction when world byte `0xb4` is zero: binary32 milliseconds are multiplied by the binary32 constant `0x3ccccccd` (approximately `0.025`) and subtracted from health, with a zero floor. Path-following work then runs only when world byte `0xb4` is zero; it clears toggle `0x0080` and uses a nonempty path only when stun is at most zero. Its steering, search completion, and aiming are outside this contract. Evidence: `0x211b7b` through `0x211d07`.

The action-lock timer at `0x130` is reduced by the full integer update duration. Only a strictly positive remainder replaces the current action clock with the current total action duration plus one millisecond. A negative remainder is clamped to zero. A remainder of exactly zero neither changes the action clock nor needs a clamp. This operation does not itself clear the action or spend mana. It precedes the action dispatcher, so action-specific effects see the overridden clock when the lock survives. The native total-duration calculation is at `0x03d1a0`. Evidence: `0x211d0d` through `0x211d4e`.

The timers at `0x134` and `0x138` are then independently reduced by integer milliseconds and floored at zero. The negative value at `0x13c`, if below zero, increases by binary32 seconds divided by `180.0`, and any positive result is capped to zero. A value initially at least zero is untouched by this recovery operation. Evidence: `0x211d4e` through `0x211e04`.

The current action's effects run next, before combo timeout, cooldown decay, buff expiration, the common stun countdown, and the general clock increment. Dispatch is indexed by the 8-bit current action. Values 1 through 110 enter the native action table; zero and values above 110 reach its default destination. The dispatch entry imposes no common local-ownership or stun rejection. Individual actions supply their own restrictions and can change state, consume resources, create events, initialize cooldowns, or cancel actions. Evidence: `0x211e04` through `0x211e1b`, with default destination `0x215ec0`.

Ownership is therefore not one universal rule for all action effects. For example, the physical-projectile branch at `0x21707a` permits the local creature, or an other creature only when world byte `0xb4` is zero and its control discriminator is nonzero. The branch entered by action 1 at `0x212357` instead rejects only an other creature with control discriminator zero on world-byte-zero; its world-byte-nonzero path is not rejected by that gate. The action-1 dispatch destination was checked against the original image table. These are examples of the distinction, not a complete audit of the action bodies.

There is no generic action-cost subtraction in the later zero-clock initialization step. Cost ownership and timing belong to the particular action effect. A projectile action may spend during anticipation crossing before its cooldown is initialized later in this same update. This contract does not replace the separate bounded action-effect contracts.

## Combo timeout and cooldown decay

The post-dispatch combo timer advances when world byte `0xb4` is zero or the creature is local. Other creatures on a world-byte-nonzero copy skip this combo-timeout block. The integer duration is added to `0x74`, and timeout handling requires the result to be strictly greater than 4,000 ms; equality does not trigger it. Evidence: `0x211f41` through `0x211f66`.

At timeout, the native timing-speed result at `0x047700` is obtained and stored as binary32. The combo count is cleared. Timing speed is obtained again and stored as binary32. The action clock is rescaled using the binary32 quotient of the pre-clear speed divided by the post-clear speed, multiplied by the action clock converted to binary32, and converted back to signed integer by truncation. The division precedes the multiplication. The timer at `0x74` is not reset here; while it remains above 4,000, this handling can be revisited on subsequent updates, including when the combo count is already zero. Evidence: `0x211f66` through `0x211faf`.

The surrounding machine code contains a conditional cooldown-refresh pass after that rescaling. Its policy callee at `0x287d10` always returns a zero result in its low byte in this supported image; its complete native body was confirmed from the original binary and `decompiled/client/functions/00687d10.c`. Therefore the conditional refresh assignment is not taken through that callee. A description that treats some cooldowns as held based merely on the surrounding conditional would not describe this image.

A following pass visits every cooldown entry then present, without the combo block's ownership restriction. Each entry has a signed 32-bit action key and signed 32-bit remaining milliseconds. With the supported constant-false policy, its remaining time is reduced by the update duration and floored at zero. This pass does not erase zero-valued entries. Entries created or modified by an earlier action body are already present for this decay. Evidence: `0x212048` through `0x2120e8`; map value access is additionally confirmed by `decompiled/client/functions/00468910.c`.

Cooldown initialization at the general clock boundary happens later than this decay. Consequently, an entry initialized there receives its full newly calculated value rather than that value minus the same update duration. The distinction remains even if the key already existed and was decremented earlier.

## Buff expiration and surviving effects

The buff collection is processed after cooldown decay. Within the native buff payload, the type is an 8-bit field at offset zero, strength is binary32 at offset four, remaining milliseconds are signed 32 bits at offset eight, and the source identifier occupies the 64-bit field beginning at offset `0x10`. These are payload offsets, not offsets from the creature or list node. Evidence: `0x212130` through `0x212163` and `0x218310` through `0x2183fa`.

Each buff's remaining time is reduced first. A result at most zero is queued for removal and skips its continuing effect. Expired nodes are removed in a separate cleanup pass after this traversal, before the stun countdown. They are not erased immediately when first marked; this contract does not assume that a nested call's independent list lookup cannot see a node awaiting removal. Evidence: `0x212143` through `0x212163` and `0x21843c` through `0x2184c2`.

A surviving type-1 buff sets a positive stun timer to zero. A zero or negative stun value is left alone by this effect. This occurs before the common subtraction from stun. A type-1 buff whose lifetime becomes exactly zero this update does not provide that clearing effect.

A surviving type-4 buff has a periodic effect only when world byte `0xb4` is zero. Its post-reduction remaining time and its prior remaining time are separately divided by 400 using signed integer division; differing quotients cause one call into the native hit/damage routine at `0x196d30`. The source identifier is looked up, and the lookup result and buff strength are passed along. There is no loop for every skipped interval and no local source-existence rejection between lookup and this call. A buff that expires during the reduction produces no final periodic call, even if one or more 400 ms boundaries were passed. A reduction from 400 to 399 ms crosses a quotient boundary; a reduction from 401 to 400 does not. The complete damage call and its downstream side effects are outside this phase contract. Evidence: `0x218310` through `0x2183df`.

Other surviving type numbers have no direct continuing-effect branch in this particular expiration pass. This does not establish that those buffs have no effects elsewhere: later predicates and timing-speed calculations can consult the surviving collection.

## Stun, clock initialization, and action-zero cleanup

After expired-buff cleanup, integer milliseconds are subtracted from stun at `0x12c`. The common interruption occurs only if the resulting stun time is strictly positive; it sets action clock and action number to zero. Exactly zero preserves both through this interruption test. Negative stun time remains negative and continues to be reduced on later updates; this site has no negative floor. Evidence: `0x2184c8` through `0x2184e8`.

The early behavior gate and this later interruption observe different times. For example, a 10 ms stun entering a 10 ms update prevents early behavior but reaches zero before interruption, so this common test does not clear the action. A surviving type-1 buff can clear stun even earlier, after behavior and action effects have already had their opportunities. An action effect that is otherwise eligible can occur before the later common stun interruption; moving that interruption to the start of the update is not established native ordering.

Control discriminator 6 skips the general clock-initialization and increment block. Every other discriminator enters it when the action clock is nonnegative, or when the current action is zero. A negative clock with a nonzero action skips this block. These tests use the clock after action effects, combo rescaling, and stun interruption, rather than the original entry clock. Evidence: `0x2184e8` through `0x218505`.

If that eligible clock is exactly zero, an entry for the current action number is created or found in the cooldown collection and assigned the result of `0x03e6a0` for that action with the current-state skill selection. The byte action is widened to the 32-bit map key. No mana cost is subtracted here. The full integer update duration is then added to the action clock, without a clamp to action duration. A negative clock with action zero can advance without initializing a cooldown, because initialization still requires equality to zero. Evidence: `0x218507` through `0x218541` and `decompiled/client/functions/00468910.c`.

Thus a still-positive stun can clear action and clock, after which the ordinary clock block initializes the cooldown for action zero and leaves its clock equal to the update duration. For control discriminator 6, the same stun interruption leaves action and clock zero because that discriminator skips the following clock block. No general automatic action-zero assignment occurs merely because the clock has reached total duration in this common block.

An action-specific velocity adjustment for action 54 follows the increment; its physics details are excluded. If the action is zero, the native routine then handles recorded dodge candidates: candidates already in the struck set are excluded, missing target lookups are excluded, and remaining targets receive a dodge-benefit call followed by a zero-damage outgoing hit notification with type 4. The two recorded sets are then cleared. This is after clock progression and before general resource drift. The benefit callee `0x03cf20` can itself alter a target's mana and buffs, so this cleanup is not presumed resource-neutral. Full hit packet semantics are excluded. Evidence: `0x218597` through `0x218713`, supported by the individual original files `0043cf20.c` and `0043c820.c`.

## Mana and blocking-resource phases

The mana phase has no local/other or world-byte restriction of its own. Its native caster predicate, `0x044610`, is true for class discriminator 3, species 117 or 86, or equipment category byte `0xaa8` equal to 3 with item byte `0xaa9` equal to 10, 11, or 12. Other state follows the non-caster branch. Evidence: `decompiled/client/functions/00444610.c` and `0x218713` through `0x2187e0`.

A caster gains binary32 milliseconds multiplied by the binary32 constant `0x38d1b717` (approximately `0.0001`), except that action 28 suppresses the gain unconditionally. Actions 95, 37, 46, 45, 31, 33, 43, and 34 suppress gain only while the already incremented current clock is strictly below current total action duration. Equality to total duration permits gain. The multiplication is performed before addition to mana.

A non-caster loses binary32 milliseconds multiplied by `0x3851b717` (approximately `0.00005`), except during actions 24, 25, 27, 59, 63, 64, and 8. The multiplication precedes subtraction. Both branches then floor mana at zero and cap it at one, including when no drift applied. These changes follow any earlier action spending or dodge benefit.

The blocking resource at `0x174` is updated next, again without an ownership restriction. The native blocking predicate `0x044590` requires action-lock time below one and either action 71, 72, 8, or 98, or action 59/63 together with class 1 and specialization 1. When blocking and the resource is positive, it loses binary32 milliseconds divided by 1,200 if equipment bytes `0x990` and `0x991` are respectively 3 and 13; otherwise it loses milliseconds divided by 600. This subtraction is floored at zero. When blocking with resource already at most zero, this subtraction is skipped.

When the blocking predicate is false, the resource instead gains binary32 milliseconds divided by 2,000. A final upper cap of one applies in either case. There is no universal lower clamp on the non-blocking recovery path. Evidence: `decompiled/client/functions/00444590.c` and `0x2187e0` through `0x21888f`.

## Later movement boundary and charging

Between the preceding resource phase and charging, the routine performs toggle changes, action-specific consumption effects, relationship-weight bookkeeping, healing, and movement setup. Their full behavior is not covered. Two relevant boundaries are established: on world-byte-zero, control discriminator 1 with positive health and an empty collection at `0x13a4` is refilled to the native maximum-health result; the subsequent acceleration-based movement contribution is replaced by a zero vector when current stun is positive. Evidence: `0x218f08` through `0x218fa0`. Terrain collision, movement integration, and relationship-weight formulas require separate contracts.

Before charging, a nonzero roll-related timer can cancel the current action. Actions 50 and 96 are exempt while their clock is at most total action duration. For class 4 and specialization 1, actions 5, 20, and 17 are also exempt; other actions are cleared to zero, but charge is retained by that class/specialization branch. Outside class 4/specialization 1, cancellation clears both action and stored charge. This gate does not reset the action clock. Evidence: `0x21a211` through `0x21a266`. Other nearby action-specific movement changes are excluded.

Charge growth occurs later, after the action clock increment and mana/blocking-resource work, and uses whatever action survives preceding changes. It has no local/other, world-byte, or control-discriminator rejection of its own. For action 36, binary32 milliseconds are first multiplied by `0x3a03126f` (approximately `0.0005`); that product is multiplied by the binary32 timing-speed result, then added to charge. This branch has no anticipation test. Evidence: `0x21a74d` through `0x21a78c`.

Actions 24, 25, 27, 8, 59, 63, and 64 gain charge only when their current clock is at least the current anticipation result from `0x03caa0`. The ordinary gain first multiplies binary32 milliseconds by `0x3a449ba6` (approximately `0.00075`), then by the binary32 timing-speed result. Class 2/specialization 0 multiplies that gain by one plus the smaller of four and combo count divided by the native combo limit; both integer inputs to the ratio are converted to binary32. The combo limit is returned by `0x044cb0` and can depend on field `0x17c`, equipment bytes `0xaa8`/`0xaa9`, and the mask at `0x7e`. Its full interpretation is a separate stat contract.

If surviving buff 2 exists, the grouped gain is additionally multiplied by one plus nine times the skill contribution for action 102. Charge is then increased by that gain. Surviving buff 10 subsequently replaces charge with current mana, overriding the just-added result. Finally, on all paths including actions that gained nothing, charge greater than current mana is lowered to mana; this site supplies no independent zero floor. Buff 2 and 10 lookups occur after the earlier expiration cleanup, so a buff expired during this update does not supply these later effects. Evidence: `0x21a78c` through `0x21a918`, `decompiled/client/functions/00443f10.c`, and `00444cb0.c`.

Physics after charging can assign a new stun duration at `0x21acdc` or `0x21ade5`; both sites lie behind a local-creature gate at `0x21ac47`. Their detailed geometric eligibility and the duration helper are excluded. At `0x21ae3f`, a positive current stun clears toggle masks `0x0010` and `0x0001`, then clears charge. That late gate does not subtract elapsed milliseconds again and does not itself clear the action or action clock. A stun assigned after the earlier countdown can therefore erase this update's charge without passing through that countdown again. Mask width is confirmed by the original toggle helper at `0x02f160`. The selected contract boundary ends at `0x21ae70`; later physics may have further effects.

## Current-state cooldown values and skill contribution

The zero-clock assignment calls the native cooldown routine with current-state skill selection. The following table records that routine's complete nonzero catalog in the supported image. “Reduction factor” is the skill contribution described below. Multiply that contribution by the listed reduction amount in binary32, subtract from the listed base in binary32, then truncate toward zero to obtain integer milliseconds. Action 72 is a fixed integer result. All unlisted actions return zero. Evidence: `decompiled/client/functions/0043e6a0.c` and original assembly at `0x03e6a0` through `0x03e93a`.

| Actions | Base milliseconds | Reduction amount |
| --- | --- | --- |
| 21, 88 | 8,000 | 8,000 |
| 48 | 20,000 | 12,000 |
| 49, 50 | 16,000 | 10,000 |
| 54, 96 | 20,000 | 14,000 |
| 72 | 15,000 fixed | none |
| 86 | 60,000 | 40,000 |
| 97, 100, 101, 102 | 60,000 | 30,000 |
| 99 | 12,000 | 10,000 |
| 103 | 20,000 | 10,000 |

For current-state contribution, actions 21, 34, 48, 54, and 88 select skill entry 6; actions 50, 79, 86, and 103 select entry 7; actions 49, 96, 97, 99, 100, 101, and 102 select entry 8. Other action numbers return contribution one through this helper. The selected signed 32-bit entry is used unless it is exactly zero and control discriminator `0x60` is nonzero. In that case it is replaced by signed level field `0x190` divided by two with truncation toward zero, plus unsigned byte `0x1a8`, plus one.

To form the contribution, the effective integer is converted to binary32, multiplied by the binary32 constant `0x3dcccccd` (approximately `0.1`), and increased by one. One divided by that result is then subtracted from one, with each operation in binary32. No generic clamp is added by these helpers. Action 102 uses this same entry-8 current-state contribution for the charging multiplier above. Evidence: `decompiled/client/functions/0043c9c0.c`, `0043ed60.c`, and original assembly `0x03ed60` through `0x03eddb`.

## Ownership summary and acceptance limits

| Common work | Gate established here |
| --- | --- |
| Behavior accumulator/update | World byte zero; control discriminator nonzero; existing stun at most zero for invocation |
| Action-lock and preliminary timer reductions | No local/other gate at these sites |
| Action-specific effects | Each body has its own gate; no universal ownership rule established |
| Combo elapsed-time/timeout handling | World byte zero or local creature |
| Existing cooldown decay | All creatures reaching this common pass |
| Buff lifetime reduction and type-1 continuing effect | All creatures reaching the pass |
| Type-4 periodic effect | World byte zero; buff survives reduction |
| Stun countdown/interruption | All creatures reaching the pass |
| Zero-clock cooldown initialization and clock increment | Control discriminator other than 6, plus the action/clock guard described above |
| Mana drift, blocking-resource update, charge growth/ceiling | No separate ownership gate at these sites |
| Two late physics stun-assignment sites | Local creature; remaining physics eligibility excluded |

This contract does not audit all per-action bodies, complete hit/dodge behavior, maximum-health calculation, complete timing-speed/stat formulas, path following, consumption, relationship management, geometry, collision, replication policy, or later update phases. It records native instruction evidence and derived equality/order consequences; no native runtime session or replacement-engine verification was performed. In particular, a valid native action body cannot be replaced by assuming that cooldown initialization also performs its resource spending, and this selected phase boundary does not prove that actions universally return to zero at total duration.

The entire final document was read back before the source-side handoff. FINAL / STABLE / HELD; no further edits are part of this assignment.
