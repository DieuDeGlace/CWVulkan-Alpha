# Alpha created-actor setup behavior

Status: FINAL / STABLE / HELD. Full author readback completed.

This is a bounded original-client contract for the future update entries of the setup root and three child objects installed by the actor-creation branch ending at RVA `0x215BA6`. It closes root iteration, the third child’s ordinary update, selected identifier/vector relations of the second child, and a bounded caller/state relation for the much broader first child. It also distinguishes update results, cloning and cleanup. A stored counter is not treated as actor lifetime without its consumer.

The original evidence root is `disassembled alpha cube world`. The supported image is `binary/Cube.exe`, a 32-bit image based at `0x00400000`, length 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`. All addresses below are RVAs. Evidence is the image, its original instruction listing `decompiled/client/assembly.txt`, and individually identified original function files under `decompiled/client/functions/`. No other project contract, reconstruction, implementation or runtime observation is evidence here.

The actor passed into an update is called the subject. The identifier stored by the second child is a lookup key; the actor obtained from that lookup is called the reference actor. These terms establish object identities without assigning allegiance or ownership labels.

## Entry identities and invocation

The supported image’s pointer tables establish these entries:

| Object identifier | Update entry at table `+0` | Entry at table `+4` |
| --- | --- | --- |
| Root, table `0x303A00` | `0x0DC390` | `0x0DC400`, which constructs a clone |
| First child, table `0x2FD310` | `0x02CB20` | `0x02EFC0`, which constructs a clone |
| Second child, table `0x2FD868` | `0x03ACE0` | `0x03B450`, which constructs a clone |
| Third child, table `0x3039E8` | `0x0DBF20` | `0x0DC1E0`, which constructs a fresh object |

Table `+4` is not a destruction entry for these objects. The root additionally has a cleanup wrapper at table `+8`, RVA `0x0DC360`, whose concrete operation is described below. No additional child-table slot is inferred merely from adjacent data.

Updates receive their object pointer as the native receiver and four four-byte arguments: subject actor pointer, world pointer, signed 32-bit step, and shared output pointer. Their consumed result is the low result byte. Decompiled `void` signatures do not negate the explicit native low-byte result stores and the root’s low-byte tests. Upper result-register bits are not a Boolean extension contract.

One verified world caller is at `0x211AE1–0x211B75`. In its mode-zero, nonzero-subject-discriminator route, it adds the world step to subject `+0x13C4`. Once that signed 32-bit accumulated value is at least `32`, and subject `+0x12C` is nonpositive, a nonnull subject `+0x13E4` is invoked with subject, world, that accumulated value and shared output. The accumulator is then cleared at this reached boundary, including when the pointer was null. A positive `+0x12C` bypasses the callback/clear boundary. This caller does not test the update result to remove the object or actor. The complete world admission and all other callers remain outside this contract.

The creation branch independently establishes the child list order: first the object with table `0x2FD310`, then `0x2FD868`, then `0x3039E8`. It initially supplies first-child fields `+0x04 = 20000`, binary32 `+0x08 = 20.0`, `+0x0C = 0`, `+0x10 = 0`; second-child full 64-bit `+0x08` is the creating actor’s identifier; third-child signed 32-bit `+0x04` starts zero. These are construction facts, not inferred durations. Evidence: original table bytes; image/listing `0x215A6B–0x215B82`; original constructors `004dc2d0.c`, `0042c550.c`, `0043ab80.c`, `004dbf10.c` and the cited world caller.

## Root: repeated ordered admission, with no child removal

At `0x0DC390`, each invocation starts at the first node of the live child list whose header pointer is root `+0x04`. It calls each child’s table-`+0` entry in list order with the same four argument values. A nonzero low result byte immediately ends root processing with low-byte result `1`. A zero result advances to the next list node. There is no persistent current-child index and no rule that the next invocation resumes after the child that previously returned nonzero. The first child is reconsidered on every invocation.

If the list is empty or all reached children return zero, the root writes three four-byte zeros to subject `+0x40`, `+0x44`, `+0x48`, then returns low-byte zero. It does not undo any other side effects of children that returned zero. Those children can already have changed flags, health, counters, action or path state before a later child is tried.

The update does not erase a list node, release a child, decrement the root’s count or remove the subject on a zero result. Root `+0x08`, the list count, is not an iteration limit in this routine; traversal ends at the header sentinel. Null child pointers are not skipped internally. Valid objects and a valid list are caller obligations.

For the constructed three-child list, reaching the third child normally yields a nonzero result, so the root’s all-zero fallback is not reached merely because both earlier children return zero. The third child may leave an existing direction unchanged and still return nonzero. A child that is not reached does not have its counters reduced by this invocation. Root itself has no random/seed call or direct world mutation. Evidence: image/listing `0x0DC390–0x0DC3FA`; original `004dc390.c`.

## First child: bounded state relation for the broad update

The first child’s entry `0x02CB20` contains extensive target selection, item use, action choice and movement/path work. Its complete action and navigation relation is not closed here. The following entry gates, local-state operations and selected exits are original-evidenced; unclosed later branches must not be replaced with a single timer rule.

Before its early admission exits, it clears bit `0x0004` in the complete 16-bit word at subject `+0x124`. It checks the subject’s action byte at `+0x68`. If that byte is none of `0x53`, `0x52`, `0x54`, `0x4F` and signed 32-bit action clock `+0x6C` is strictly greater than `10000`, it writes action byte zero. Equality at `10000` does not take this action-clear route. It does not clear the clock at that store.

Next, a nonzero full 32-bit subject `+0x128` causes an immediate low-byte-`1` return. An action byte `0x50` with signed clock strictly below `3000` also returns `1`. Equality at `3000` continues. These early exits preserve the child’s counters and make no direct random draw. They nevertheless retain the earlier subject flag/action changes. They prevent the root from trying either following child.

Later target-selection work uses world `+0x04`, subject state and helper `0x044BF0`, with an additional subject-link route for discriminator `5`. That complete selection and relationship policy remains a dependency. It is not equated with the identifier stored by the second child.

| First-child field | Established update relation |
| --- | --- |
| Signed 32-bit `+0x04` | On the preliminary nonnull-target route at `0x02CE36–0x02CE4F`, the integer step is subtracted and a negative result is stored as zero. Earlier exits and preliminary no-target routes can bypass this reduction. A later accepted nonzero action choice resets this field to `20000` at `0x02D869`, writes that choice’s low byte to subject `+0x68` and zeroes subject `+0x6C`. The complete admission of that action-choice route is unclosed. |
| Binary32 `+0x08` | Read and squared during a later no-target candidate scan. It is not reduced by the step. Its initial `20.0` is consequently not an established countdown or lifetime. Full candidate admission is unclosed. |
| Signed 32-bit `+0x0C` | Cleared on the established no-final-target exit. A later selected-target/path-failure route adds the integer step. Values through `20000` remain on that route; a result strictly greater than `20000` reaches the reset described below. Other reached processing can also reset this field to zero. |
| Signed 32-bit `+0x10` | Reduced by the integer step and negative-clamped to zero at `0x02D288–0x02D29D`. Its positive state also participates in a no-target scan gate. The later over-`20000` route sets it to `3000`. |

These native additions/subtractions operate at 32 bits before signed tests. The contract does not invent positive-step admission at those stores. It also does not imply that each field is updated on every root invocation.

The established no-final-target exit at `0x02D2A0–0x02D2A9` clears child `+0x0C` and returns low-byte zero after the `+0x10` reduction. Earlier target/flag effects survive that return. Before this exit, a reached selected-target publication route writes binary32 `1.0` into subject map `+0x13A4` under the selected actor’s full identifier, then calls world helper `0x1A0970` with selected actor first, subject second and shared output third. The helper’s full record-production body is outside this scope.

The route at `0x02EB24` adds the step to child `+0x0C`. When its signed result exceeds `20000`, the original clears the subject’s map at `+0x13A4`, invokes helper `0x02EF10`, sets child `+0x10` to `3000`, resets child `+0x0C` to zero, and returns low-byte zero. Equality at `20000` follows the other route. This is a local state/reset relation, not deletion of the child, root or subject. The earlier selected-target/path conditions that admit this route are not fully specified here.

Other reached portions can write subject direction at `+0x40..+0x48`, response at `+0x34..+0x3C`, action byte/clock, the triplet at `+0x160`, item/inventory state, maps, and shared output through additional helpers. Those complete mutations and action membership remain unclosed. The common successful exit explicitly returns low-byte `1`; the two zero exits above explicitly return zero. A root implementation cannot treat the unresolved middle as side-effect-free or infer its result merely from child `+0x04` reaching zero.

### First-child random boundaries

The audited prefix has a qualifying item-use route that consumes two direct `rand` calls, at `0x02CCC1` then `0x02CCE8`, after selecting/decrementing a qualifying inventory record and starting action `0x50`. Another established direct draw at `0x02D4AD` occurs when the reached child `+0x04` is zero and the subject’s four-byte candidate range at `+0x13E8..+0x13EC` is nonempty; unsigned remainder over that count selects a full 32-bit value. That draw occurs before the subsequent selected-action admission is fully resolved.

The complete direct random-site inventory also includes `0x02D928`, `0x02DA43`, `0x02DCB6`, `0x02DCFB`, `0x02DD47`, `0x02DDA3`, `0x02DF1C`, `0x02E175`, `0x02E338`, `0x02E42F`, `0x02E770`, `0x02E867`, `0x02EA7C`. These belong to different action/type-dependent routes. Their per-route admission, result use and joined ordering are not all closed by this contract; the address inventory is not a promise that calls occur in numerical-address order or on every update. No fixed draw count is supplied for this broad child. No direct seed call was identified in the update; transitive draws or seed calls in its unclosed helpers remain unresolved.

Evidence: original `0042cb20.c`; image/listing `0x02CB71–0x02CBC6`, `0x02CE36–0x02CE4F`, `0x02D20F–0x02D2A9`, `0x02D4AD`, `0x02D865–0x02D87A`, `0x02EB24–0x02EBA5`, `0x02EEBE–0x02EEE3`, and identified direct call sites. This bounded relation is intentionally not a complete target/action resolver.

## Second child: stored identifier and reached vector routes

The second child’s update `0x03ACE0` reads its full 64-bit field at `+0x08` and looks up that exact key in world `+0x04`. It does not reject zero or a key that resolves to the subject itself, and does not derive the key from subject `+0x198` during this update. A missing map entry or null mapped actor returns low-byte zero before the later subject mutations. The routine makes no update store to its own identifier or its unused `+0x04`; there is no child countdown in this route. No positive-health test on the reference actor is added by the lookup admission.

With a nonnull reference actor, it subtracts subject position from reference position as three signed 64-bit differences from actor offsets `+0x10`, `+0x18`, `+0x20`. Helper `0x03AC20` forms a fixed-point three-component dot product: each component product retains native 64-bit multiplication width, is signed-divided by `65536` with truncation toward zero, and the three results are added at 64 bits. This call supplies that difference vector as both dot-product operands, producing a squared-distance-like quantity. In ordinary nonoverflowing inputs, the subject’s word `+0x124` gains bit `0x0040` exactly when that signed quantity is strictly greater than `0x400000`; otherwise that bit is cleared. Equality clears it. Other bits survive. The full overflow/exceptional arithmetic domain is not supplied.

If subject discriminator byte `+0x60` is `5`, the routine also updates subject health before the following direction decision. It queries `0x044DB0`, rounds the returned scalar to binary32, converts the signed step to binary32, multiplies that step first by binary32 `0.001` (`0x3A83126F`) then `0.1` (`0x3DCCCCCD`), multiplies the first queried scalar by that product, and adds existing health in binary32. This result is stored at subject `+0x16C`. It queries the maximum helper again for the comparison; only if the new health is strictly greater does it make a third query and store that result. Thus the route makes two queries, or three on the upper-clamp route. No lower clamp occurs at these stores. The ordinary created discriminator values `1` and `3` do not enter this conditional unless later state changes to `5`.

The routine then independently converts the same signed 64-bit positional differences to binary32 and multiplies each by binary32 `1/65536` (`0x37800000`). It zeroes the subject triplet `+0x40/+0x44/+0x48`. When the resulting three-component squared distance is below binary32 `16.0`, this route returns low-byte zero with that zero triplet; equality proceeds to the next test. This subject-vector result can therefore differ from the earlier integer distance gate used for flag `0x0040`.

At the reached distance boundary it calls world helper `0x209480` with subject, subject-position address, reference-position address, and the binary32 sum of subject `+0x80` followed by reference `+0x80`. This contract uses the helper’s low-byte result without naming an unproved spatial predicate. A nonzero result leads to helper `0x02EF10` on the subject, then a direction based on the horizontal scaled differences multiplied by binary32 `5.0`; the third component is zero. If the squared length of that triplet is strictly greater than binary32 `900.0`, it normalizes using the original double square-root call, rounds the result to binary32, forms a binary32 reciprocal, and multiplies each direction component first by the reciprocal and then by binary32 `30.0`. At or below `900.0`, it retains the triplet multiplied by `5.0`. It stores `+0x40`, `+0x44`, `+0x48` in that order and returns low-byte `1`.

When `0x209480` returns zero, the remaining branch searches terrain cells vertically through helper `0x02F860`, then can build or consume subject state around `+0x1408..+0x1468`, call `0x1A9F90`, `0x1AAAB0` and `0x1A7C90`, and return either zero or one through its native exits. The full terrain admission, path construction, coordinate transformations and those nested effects remain open here. They must not be replaced by the nonzero-helper direction relation. The update has no direct `rand` or `srand` call, but that does not prove absence of transitive draws in these unresolved spatial helpers.

The reached clearing helper `0x02EF10` is a meaningful side-effect boundary: its direct stores restore empty header links and zero counts at subject `+0x140C/+0x1410`, `+0x1414/+0x1418`, and `+0x1460/+0x1464`; it releases their reached nodes, repoints subject `+0x1408` to the first header and reaches another container-clear helper. The full contents/lifetimes of these structures remain unclosed. Returning zero from the second child does not undo its flag, health, vector or clearing effects.

Evidence: original `0043ace0.c`, `0043ac20.c`, `0042ef10.c`; image/listing `0x03ACE0–0x03AE63`, `0x03AF6B–0x03B05A`, `0x03B073–0x03B447`, `0x02EF10–0x02EFA7`. The threshold constants are original image data at RVAs `0x345E78`, `0x2FD874`, `0x2FCE24`, `0x2FCD78`.

## Third child: complete ordinary counter and direction update

At entry `0x0DBF20`, the third child always clears bit `0x0040` in the subject’s full 16-bit word at `+0x124`. It then subtracts the signed 32-bit step from its own signed 32-bit field `+0x04`, storing zero when the wrapped subtraction result is negative. If the stored result is nonzero, it returns low-byte `1` without changing subject direction or action. It does not yield zero merely while waiting.

A stored result of zero, including exact equality after subtraction, refreshes direction. The route compares the subject’s horizontal position at `+0x10/+0x18` with the two signed 64-bit reference coordinates at subject `+0x1C0/+0x1C8`. Each horizontal difference is squared at native 64-bit width and signed-divided by `65536`; the two terms are added at 64 bits. The random-direction route is taken when that signed sum is at most `0x640000`. Equality takes the random route. The following ordinary relations exclude overflowing coordinate products and invalid floating-point operations.

On that random route, the first direct random draw determines the horizontal component stored at subject `+0x40`, and the second determines `+0x44`. Each result is converted from signed 32-bit integer to binary32, multiplied by `2.0`, divided by `32767.0`, reduced by `1.0`, then multiplied by `10.0`, rounding at each binary32 operation. The third component at `+0x48` is zero multiplied by `10.0`. The actual direction stores occur in `+0x40`, `+0x48`, `+0x44` order after both draws. This route does not normalize the two random components.

When the fixed-point sum is greater than `0x640000`, direction is based on reference coordinates minus subject position instead. Each horizontal signed 64-bit difference is converted to binary32 and multiplied by `1/65536`; the third direction component is set to zero. The routine also converts the third positional difference from reference `+0x1D0`, but it does not use it as the stored vertical direction. It sums squared direction components in binary32, promotes that sum to binary64 for the original square-root call, rounds the square root to binary32, forms a binary32 reciprocal, and multiplies each component by that reciprocal and by `10.0`. This route consumes no direction draws. The origin and future producers of subject `+0x1C0..+0x1D7` are not established here; they are not assumed to equal the subject’s current or creation position.

After either direction-refresh route, one further random draw is reduced by signed remainder over `5000`, increased by `3000`, and stored as the child’s new 32-bit `+0x04`. With the original imported generator’s ordinary nonnegative results this gives `3000..7999`. Only after that store does the routine set subject action byte `+0x68` to zero. It does not also clear subject action clock `+0x6C`. It then returns low-byte `1`.

The three direct call sites are `0x0DC137`, `0x0DC15E`, `0x0DC1AF`: near/random refresh uses all three in that order; far/reference refresh uses only the last; a nonzero remaining counter uses none. There is no direct seed call, world-map lookup, hit/buff publication or identifier mutation in this child. Its world and shared-output arguments are not read by this routine. The root can therefore stop on this child while preserving a direction from an earlier invocation when its counter remains nonzero.

Evidence: original `004dbf20.c`; image/listing `0x0DBF20–0x0DC1DD`, particularly the counter gate `0x0DBF3C–0x0DBF64`, integer-distance split `0x0DBFB8–0x0DC004`, far direction `0x0DC00A–0x0DC135`, and draws/final stores `0x0DC137–0x0DC1D3`. Numeric constants are original image data, not inferred timing units.

## Cloning, cleanup and remaining lifetime limits

The root’s table-`+4` entry allocates another `0x0C`-byte root and invokes copy construction `0x0DC200`. It walks the source child list from first to last, calls each child’s table-`+4` entry and appends each returned pointer to a newly built destination list. Normal successful cloning preserves child order. It does not run child updates.

First-child cloning at `0x02EFC0` copies its 32-bit `+0x04` and raw four-byte `+0x08`, while resetting destination `+0x0C/+0x10` to zero. Second-child cloning at `0x03B450` preserves the full identifier at `+0x08` and does not initialize or read its `+0x04`. Third-child entry `0x0DC1E0` constructs a new object with counter `+0x04` zero, rather than copying the source counter. These clones and the inspected list constructors make no direct random/seed calls. Null allocations and failed child cloning are not made safe by the root update’s iteration; valid nonnull child objects remain required.

Concrete root cleanup at `0x0DC300` first walks the child list and passes each child pointer directly to deallocation helper `0x28D66E` in list order. It does not invoke a child virtual update or clone entry at that point. It then restores the root list’s empty header links, zeroes its count, frees the old nodes in forward order and frees the sentinel. Wrapper `0x0DC360` calls that cleanup, then additionally frees the root object when its supplied low-byte flag has mask `0x01` set. These cleanup operations have no established actor/world mutation or random/seed draw.

This cleanup is distinct from all actor-pointer release sites. For example, original actor teardown at `0x03C41A–0x03C42B` and reset at `0x046342–0x04634D` load a nonnull actor `+0x13E4` and pass it directly to the same deallocation helper; they do not call the root cleanup wrapper through table `+8` at that observed boundary. The scoped replacement site `0x1EE8E0–0x1EE8EB` likewise directly releases the previous pointer before later storing another pointer at `0x1EE93C`. This contract therefore does not claim that every actor release route necessarily traverses and frees the root’s child list. The full deallocation/runtime behavior and all callers of the root-specific cleanup remain explicit lifetime dependencies.

Direct gameplay random calls identified here use image IAT RVA `0x2FC468`, the `rand` import from `MSVCR110.dll`; image IAT `0x2FC4B4` identifies `srand`. The numeric square-root thunk `0x28D9C8` reaches the original `_libm_sse2_sqrt_precise` import. The contract does not infer an algorithm, seed lifetime or complete shared stream from these names. Imported library internals, allocation hooks and the unresolved nested action/spatial helpers remain outside the closed draw relations.

Evidence: original `004dc200.c`, `004dc400.c`, `0042efc0.c`, `0043b450.c`, `004dc1e0.c`, `004dc300.c`, `004dc360.c`, `0068d9c8.c`; image/listing at their entries and the cited actor-pointer release boundaries, plus the supported image’s import data.

The result is intentionally bounded. The first child’s full target/action/movement resolver, the second child’s terrain/path continuation, subject reference-coordinate producers, all later world/physics effects, exceptional arithmetic, invalid pointers/containers, allocation failures and concurrent mutation are unresolved where stated. No child result is treated as automatic deletion, no actor expiry is derived from `20000`, and no separate server behavior or ownership label is supplied. Earlier held artifacts remained unopened and unchanged.
