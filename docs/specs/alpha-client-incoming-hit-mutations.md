# Alpha incoming controlled-creature hit mutations

Status: FINAL / STABLE / HELD. This independently worded source-side contract establishes the bounded incoming local mutation phase for record categories 0, 1, 3 and 5. It does not certify a replacement implementation, a complete hit resolver, all category meanings or runtime acceptance.

The evidence root is `disassembled alpha cube world`. The supported original image is `binary/Cube.exe`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred image base `0x00400000`. Addresses below are RVAs except explicitly identified object/record offsets. The principal evidence is the incoming hit walk inside client frame `0x088EE0`, especially `0x08FF10–0x090304`, and individually identified original helpers. The large frame's function export times out, so its native instructions establish the frame operations. No other contract, reconstruction, replacement, handoff, SDK, exporter, tests, probes, captures, logs or implementation history supplies facts. No server consumer behavior is claimed.

## Target, source and field admission

The frame walks the native incoming hit list at client `+0x800630` in list order. For each record, it resolves the 64-bit target identifier beginning at record `+0x08` through the world embedded at client `+0x2E4`, using helper `0x02F000`. Only a resolved pointer equal to client controlled-creature pointer `+0x8006D0` enters the local mutation block. Other targets skip these local mutations and continue the incoming walk. A valid nonnull controlled creature is a dependency; the comparison is not an added independent null-controlled-creature guard.

The local block does not resolve the record's source identifier or require it to be foreign, nonzero, or different from the target. There is no source-equals-target exclusion here, and no additional socket, world-mode `+0xB4`, or distinguished-pointer `+0xB8` test in this block. Full earlier frame reachability and the lookup helper's complete identity-container behavior remain outside scope.

The record category is one byte at `+0x45`. Amount is binary32 at `+0x10`; its sign and ordered comparisons are used directly. The critical byte `+0x14` is not read by these local mutation operations, so it does not double the amount here. Source ID, record position, and bytes `+0x44/+0x46` do not control these mutations. Subsequent forwarding or presentation phases can have different field uses and are not substituted for this local boundary.

Record `+0x18` is read as a signed 32-bit timer only in the admitted category-0/category-3 health/status path. The second four-byte record timer/status field at `+0x1C` is not read anywhere in this scoped mutation block. Neither field is counted down or converted using frame elapsed time here.

Sources: original `binary/Cube.exe`, incoming-list start `0x08FECB–0x08FF05`, target lookup/equality `0x08FF10–0x08FF37`, category dispatch `0x08FF3D–0x08FF7F`, `0x090081–0x090089`, timer use `0x090277–0x090295`; `decompiled/client/functions/0042f000.c`; corresponding instructions in `decompiled/client/assembly.txt`.

## Category 1: vector assignment, response and resource fields

Category 1 does not require controlled health to be positive. Its amount is also not required to be positive for the main resource operations. It does not invoke the category-0/category-3 health or timer path afterward.

When its amount is ordered strictly greater than zero, the block first copies the three received vector components at record `+0x38/+0x3C/+0x40` into controlled creature `+0x34/+0x38/+0x3C`, then writes controlled binary32 response field `+0x1184` to exact 1. It subsequently performs the same vector copy again as part of the unconditional category-1 operations. When amount is zero, negative or unordered, it skips that first copy and response write but still performs the unconditional vector copy once.

These operations are assignments. Original helper `0x011E30` copies three consecutive four-byte words without vector addition, scaling or normalization. Two calls with the same received vector therefore do not add it twice to existing velocity. The conditional response write and the unconditional velocity assignment are distinct effects. A skipped response write leaves the prior `+0x1184` value unchanged in this block.

After the unconditional vector assignment, the block captures amount and calls controlled-creature helper `0x03E190` exactly once. It stores the helper's floating return as binary32 and uses it as the denominator of a binary32 amount division. The full resource/stat formula of that helper and its item/stat dependencies are not reconstructed here. There is no caller-side zero-denominator or finite-value guard.

The established resource mutations occur in this order:

1. Add the old binary32 `+0x174` to binary32 `+0x170`, and store the sum at `+0x170`. The amount/denominator quotient is calculated before this first resource store and retained for the later subtraction.
2. Add the still-current old `+0x174` to binary32 `+0x144`, and store that sum at `+0x144`.
3. Replace `+0x170` with exact 1 only when it is ordered greater than 1. Equality and unordered comparison retain the stored value.
4. Replace `+0x144` with the updated `+0x170` only when `+0x144` is ordered greater. Equality and unordered comparison retain it.
5. Subtract the retained amount/denominator quotient from `+0x174`, store the result, and replace it with exact -1 only when the result is ordered less than -1. Equality and unordered comparison retain the result.

There is no lower cap on `+0x170/+0x144` in this region and no upper cap on `+0x174`. The old `+0x174` need not be positive to enter this incoming branch. A negative amount can change the direction of its quotient-based subtraction; no sign toggle is inserted by this consumer.

Category 1 does not read either received timer for a local timer write, does not write local health `+0x16C`, and does not call the health-limit helper. It does not directly rewrite the action byte `+0x68`, action clock `+0x6C`, or lock/status fields `+0x128/+0x130` in this block.

Sources: `decompiled/client/functions/00411e30.c`, `0043e190.c`; original vector-copy helper `0x011E30–0x011E49`; conditional vector/response `0x08FF3D–0x08FF77`, unconditional vector/helper call `0x08FF7A–0x08FFAD`, resource addition/division/stores `0x08FFB1–0x08FFF1`, upper caps `0x08FFF9–0x09003D`, final resource subtraction/lower cap `0x09003D–0x09007C`. Binary32 1 and -1 are at image RVAs `0x345DC0/0x345E8C`.

## Category 5: every resident type-6 magnitude and one update per match

Category 5 also has no controlled-health positivity admission and no amount-positivity admission. It does not use the category-1 vector/resource operations or the category-0/category-3 health/status operations.

The block visits the controlled creature's resident list at `+0x1178` in native list order. For every resident entry whose first byte is type 6, it subtracts the incoming binary32 amount from that entry's binary32 magnitude at resident-record `+0x04` and stores the result. It uses the full incoming amount again for every matching entry. It does not distribute a total amount across entries, stop when one entry is exhausted, or require a remaining-duration/positive-magnitude admission before processing an entry.

If the stored remainder is ordered at most zero, the block overwrites its magnitude with positive zero and overwrites the resident entry's four-byte remaining-time field at `+0x08` with zero. Equality to either signed zero takes this clearing case. An unordered remainder does not take the clearing case: the stored unordered magnitude and prior time survive that particular comparison. A positive remainder retains the existing remaining time. No lower/upper magnitude rule beyond that observed comparison is supplied.

After each matching entry's mutation and any zero/time clearing, the block builds one 40-byte update event. Both of its leading 64-bit identifiers are filled from the controlled creature's identifier at creature `+0x08/+0x0C`. The event's final 24 bytes are copied directly from that resident record after the mutation. This preserves all 24 resident bytes, including the bytes not otherwise interpreted by this contract; the incoming hit's source identifier is not substituted into them. The native event append helper copies five eight-byte pieces, so the emitted 40-byte payload preserves that complete temporary event.

The update appends to the shared frame output aggregate's list at aggregate `+0x58`. There is one update for every visited type-6 entry, even if the input amount is zero, the magnitude did not change numerically, or the entry is cleared to zero time. The append for one matching entry precedes advancing to the next entry. The resident node is not removed by this block; setting its time to zero and later expiration/removal are separate phases. A non-type-6 entry is passed without these writes or update emission.

Category 5 does not write controlled health, either received timer into an actor timer, velocity, response `+0x1184`, or the three category-1 resource fields. There is no direct action or action-clock rewrite. Event allocation failure and any broader rollback behavior remain outside the normal successful-append ordering described here.

Sources: original `binary/Cube.exe`, category admission/list walk `0x090081–0x0900DF`, magnitude/time mutation `0x0900E5–0x090103`, event construction/identifiers/24-byte copy/append `0x09010A–0x090182`, iteration `0x090187–0x0901CB`; `decompiled/client/functions/0043b660.c`, `004460a0.c`, `00451c70.c`. Update append is `0x0460A0`, with the complete 40-byte copy in `0x051C70`.

## Categories 0 and 3: positive-health admission, response meter and timer

For the exact categories 0 and 3, the observed dispatch reaches the same local health/status region. Category 3's earlier explicit comparison does not suppress this region. Neither of these categories performs the category-1 received-vector assignment or its response-field `+0x1184` write. Their incoming vector is not added or substituted into local velocity in this scoped block.

The region requires current controlled binary32 health `+0x16C` to be ordered strictly greater than zero. Zero, negative and unordered health skip all of the following category-0/category-3 meter, timer, health and health-limit operations for that record. It does not use a source-health condition and cannot be treated as an unconditional resurrection operation from zero health.

Only when amount is ordered strictly positive does it first reduce controlled binary32 field `+0x1190`. The caller multiplies amount by exact binary32 0.25. Separately, it decrements controlled four-byte field `+0x190` by one with native 32-bit arithmetic, interprets that result as signed for conversion to binary32, and multiplies the converted result by exact binary32 0.25. It passes exact binary32 base 2 and that binary32 exponent to helper `0x011D10`. The helper promotes both inputs to binary64 for the original power routine, rounds the returned power to binary32, and returns that binary32 value. The caller stores it as binary32, divides the quarter-amount by it, and subtracts that binary32 quotient from `+0x1190`.

After storing this `+0x1190` result, it replaces it with positive zero only when ordered negative. Zero equality and unordered comparison retain the result. There is no upper cap here. The complete gameplay meaning of `+0x1190`, valid range of `+0x190`, and exceptional power/division behavior are not invented. Negative, zero and unordered amounts skip this meter-reduction block entirely.

Next, independently of amount sign, a strictly positive signed 32-bit received timer at record `+0x18` causes controlled signed 32-bit timer `+0x12C` to be set to the signed maximum of its current value and the received timer. Equality leaves the same value, although the positive-timer path still stores it. Zero or negative received values do not write this actor timer. This is a maximum/extension operation, with no addition, elapsed scaling or countdown in this phase. Record `+0x1C` is ignored.

The timer merge precedes the health subtraction, including when that subtraction will reduce health to zero. The block does not directly interrupt by writing action byte `+0x68` or resetting action clock `+0x6C`, and does not directly change `+0x128/+0x130`. Any later actor/update response to `+0x12C` is outside this consumer boundary.

Sources: original `binary/Cube.exe`, category-3 initial dispatch `0x08FF3D–0x08FF7F`, health/positive-amount admission `0x0901D0–0x0901EF`, quarter-amount/exponent/power/division/meter writes `0x0901F5–0x09026D`, signed timer merge `0x090277–0x09028F`; `decompiled/client/functions/00411d10.c`, supported by original helper instructions `0x011D10–0x011D35`, which establish both arguments beyond the incomplete recovered signature. Exact binary32 0.25 is at image RVA `0x345D98`; exact base 2 is immediate bits `0x40000000`.

## Categories 0 and 3: health clamps and exact helper-call order

After the optional meter reduction and optional timer merge, the block reads current controlled health, subtracts the stored record amount once in binary32 and immediately stores that result. No further critical multiplication or armor/resource reduction is inserted by this local consumer. For ordinary finite values, positive amount reduces health and negative amount increases it.

It then replaces resulting health with binary32 positive zero only when ordered strictly negative. Equality to zero does not take that replacement, and an unordered result does not take it. This lower check precedes the upper-limit calculation.

For every record that passed this region's initial positive-health gate, it reads the post-lower-check health value into a binary32 temporary and calls controlled-creature health-limit helper `0x044DB0` once. The helper's return is stored as binary32. The saved health value is compared to that first limit. Only an ordered strictly greater health value causes a **second** call to the same helper, whose returned value is stored as controlled health. Equality retains health and uses only one helper call. Unordered comparison likewise does not take that second-call/store path.

The second result is freshly requested; the first limit is not simply reused. A zero or negative incoming amount still reaches this upper check when the initial health gate passed. A record that fails the initial health gate calls the health-limit helper zero times in this region. The full health-limit/stat formula is a named dependency, not reconstructed from assumed gameplay values.

Records are processed in order against the actor state left by earlier records. In particular, if one category-0/category-3 record leaves health zero, a later such record encounters that zero in its initial health gate and skips this region; category-1 and category-5 admission does not gain a health requirement from that earlier record. Amounts must not be aggregated into one subtraction or combined with one shared limit call when reproducing the observed operations.

Sources: original `binary/Cube.exe`, direct health subtraction/store `0x090295–0x0902A8`, lower clamp `0x0902B0–0x0902BF`, saved health/first limit call `0x0902C9–0x0902E2`, strict comparison/second call/store `0x0902E6–0x090304`; `decompiled/client/functions/00444db0.c`. The helper is identified by original RVA and caller order; its item/stat dependencies and original power-library details remain unexpanded.

## Direct draws, presentation and frame ordering limits

There is no random-function call, audio-play call or presentation-list append in the scoped local mutation region. Its category-1 vector copier and category-5 update constructor/copy introduce no chance draw. The 40-byte category-5 state update is a state event, not an inferred sound or visual effect. The inspected stat helpers contain arithmetic and item/stat calls rather than a new caller-side hit-critical or pitch decision; a complete recursive audit of every stat/math dependency is not claimed.

When the scoped world-call region and this incoming mutation region both run in a frame, all world pieces at `0x08CF2E/0x08CF49` have finished before these incoming actor mutations. The incoming hit walk proceeds in native list order and advances after each record. A later pass separately forwards selected incoming records to the shared frame output list, and the incoming queue is cleared at `0x0905C4`. Those later routing decisions are not an additional admission test for mutations already performed.

The specified controlled primary-start call at `0x09BF82` and outgoing frame queue-copy region beginning at `0x09C02F` are later than this incoming mutation phase. This establishes caller ordering; it does not establish the primary helper's own readiness result, every other input route, a later actor stun-interruption policy or the behavior of the next world update.

This contract supports the exact numeric categories 0, 1, 3 and 5 described above. It does not assign unspecified category values the meaning of category 0, or reconstruct their complete consumers merely from a shared residual native branch. Later hit displays, audio/presentation consumers, forwarding, network echo policy, complete frame reachability, concurrency/lifetime changes and authoritative snapshot correction remain separate dependencies.

All stated arithmetic stages retain the observed binary32 stores/operations, with the identified binary64 power helper boundary. There are no newly supplied finite-value filters or denominator substitutes. Exceptional floating-point values, invalid integer ranges, machine exception/rounding state and allocation failures are bounded by the described native operations and comparisons, rather than by an invented global error policy.

Sources: original `binary/Cube.exe`, mutation block `0x08FF10–0x090304`, iteration `0x09030A–0x090344`, later forwarding/clear anchors `0x090409–0x0905C4`, world-call sites `0x08CF2E/0x08CF49`, later primary/outgoing sites `0x09BF82/0x09C02F`; original helper exports named above and corresponding `decompiled/client/assembly.txt` ranges.
