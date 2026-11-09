# Alpha ranged action scheduling

Status: FINAL / STABLE / HELD. This is a bounded source-side behavioral contract, independently worded from original Alpha evidence. It does not establish runtime acceptance, complete combat parity, or independent provenance of any existing implementation.

## Supported image and evidence

The supported image is the original `binary/Cube.exe` in the assigned Alpha evidence directory: 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, PE machine `0x014c`, timestamp `0x51ea955e`, image base `0x00400000`, image size `0x003bc000`. Addresses below are RVAs unless explicitly introduced as VAs. The original program is 32-bit; the field offsets describe that image, not a required layout for another engine.

Primary evidence is the original creature simulation routine at RVA `0x20c510`, its individually exported assembly, the unowned instruction ranges of that same routine in the original `assembly.txt`, the original image's dispatch tables, and original routines at RVAs `0x03caa0`, `0x047310`, `0x03d1a0`, `0x047700`, `0x044cb0`, `0x03e350`, `0x03e5a0`, and `0x02cb20`. Input transitions are confirmed against original binary ranges within RVA `0x088ee0`. The main action dispatch uses the byte table at RVA `0x222ccc` and destination table at RVA `0x222c84`.

Infinite-Cubes Alpha files concerning creature state, combat decisions, input, and world simulation supplied search leads only. Their explanatory comments and claimed shot counts were not accepted as native evidence. No replacement engine, exporter, SDK, tests, captures, logs, existing specifications, semantic audits, or implementation history were read. No reference files were modified.

## State and time vocabulary

The following offsets are relative to the creature object seen by the native routines.

| Offset | Width | Meaning established for this contract |
| --- | --- | --- |
| `0x60` | 8 bits | Creature control/hostility discriminator used in emission ownership and behavior gates |
| `0x68` | 8 bits | Current action number |
| `0x6c` | signed 32 bits | Current action's elapsed milliseconds |
| `0x70` | signed 32 bits | Combo counter used by charging |
| `0x12c` | signed 32 bits | Remaining stun milliseconds; negative values are retained |
| `0x128` | signed 32 bits | Roll timer used as a zero/nonzero action gate |
| `0x130` | signed 32 bits | Action-lock timer |
| `0x140`, `0x141` | 8 bits each | Class and specialization discriminators |
| `0x144` | binary32 | Accumulated charge |
| `0x160` | three binary32 values | Attack-direction vector |
| `0x170` | binary32 | Mana-like resource, ordinarily clamped to the interval zero through one |
| `0x17c` | binary32 | Additional action-time scaling factor |
| `0x13b4` | binary32 | Charge retained for an already released physical shot/volley |
| `0x13c4` | signed 32 bits | Accumulated behavior-update milliseconds |

In the prose below, the old time is the action clock before this update's general clock increment; the new time is the old time plus the update's integer millisecond argument. The simulation also converts that same argument to binary32 for continuous resource/charge changes. This conversion is visible at RVA `0x20c83d` through `0x20c862`.

The anticipation duration is the integer returned by RVA `0x03caa0`; the strike duration is returned by RVA `0x047310`; the total action duration is returned by RVA `0x03d1a0`. A negative action argument to the first two selects the creature's current action. The timing-speed value is returned by RVA `0x047700`. For the scaled entries below, divide each listed base duration separately by the product of that speed and the binary32 field at `0x17c`, then truncate toward zero. The total duration adds these separately truncated anticipation, strike, and recovery components. It is not one truncation of a combined floating-point sum.

| Action number | Descriptive use | Anticipation base | Strike base | Recovery base |
| --- | --- | --- | --- | --- |
| 22 | Ordinary arrow emission | 400 ms | 50 ms | 100 ms |
| 23 | Released charged arrow | 50 ms | 50 ms | 10 ms |
| 24, 25, 27, 36 | Held ranged charging forms | 50 ms | 300 ms | 500 ms |
| 26 | Thrown projectile | 300 ms | 1,200 ms | 100 ms |
| 55 | Released arrow volley | zero | 500 ms | 500 ms |
| 45, 46 | Repeated magic projectiles | 1,200 ms | 500 ms | 100 ms |
| 37, 43 | Large magic projectile forms | 600 ms when weapon byte `0xaa9` is 12; otherwise 1,200 ms | 100 ms | 100 ms |
| 38, 44 | Magic projectile forms | 500 ms | 300 ms | 100 ms |
| 39, 40, 41, 42 | Magic projectile forms | 200 ms | 300 ms | 100 ms |

Action 50 and action 96 share the arrow-volley scheduling branch: anticipation is zero, strike is a fixed 500 ms, and recovery is a fixed 100 ms. Unlike action 55, those two components are not scaled. Action 108 shares the magic-projectile scheduling branch; its timing is the default scaled anticipation/strike/recovery of 400/300/500 ms. These action labels describe the observed branches and are not an assertion about every localized UI name.

The complete speed calculation is outside this contract. It is state dependent, including combo, equipment-related contribution, and an active buff numbered 12. Substituting a constant speed is not established native behavior. The routines above are the evidence anchors for later independent expansion.

## Ownership and the simulation phase order

The physical-shot, arrow-volley, held-charge-sound, and magic-projectile branches share this exact ownership restriction: the creature is allowed when it equals the pointer at world offset `0xb8`; a different creature is allowed only when world byte `0xb4` is zero and creature byte `0x60` is nonzero. A disallowed creature performs none of that branch's resource spending, projectile creation, branch sounds, or branch buff removal. This is confirmed at RVAs `0x21707a`, `0x217634`, `0x217a21` and original binary range `0x215bb3`.

For a living creature reaching this phase, the relevant native order is:

1. When world byte `0xb4` is zero, behavior processing for a creature with nonzero `0x60` accumulates the update duration. It invokes the behavior only when the existing stun timer is at most zero and the accumulator has reached at least 32 ms. Invocation receives the entire accumulator, then the accumulator is reset to zero. A positive stun suppresses the invocation without discarding the accumulated time. Evidence: `0x211ae1` through `0x211b6f`.
2. The action-lock timer is reduced by the update duration. If it remains positive, the action clock is set to one millisecond beyond the current total action duration. Negative lock time is clamped to zero. This precedes action dispatch. Evidence: `0x211d0d` through `0x211d4e`.
3. Current-action effects, including the ranged emissions below, run against the old action clock and the prospective new time.
4. Combo/cooldown work follows. Buff lifetimes are reduced; expired buffs are excluded from their continuing effects. A surviving buff numbered 1 clears a positive stun to zero. Evidence: `0x212130` through `0x21843c`, particularly `0x2183e1`.
5. The stun timer is reduced by the update duration. Only a still-positive result resets both the action number and action clock to zero. A result of exactly zero does not interrupt the action. There is no lower clamp here. Evidence: `0x2184c8` through `0x2184e8`.
6. Except for control discriminator 6, the general clock advances when its old value is nonnegative or the action number is zero. A negative clock with a nonzero action does not advance through this path. When the clock is exactly zero before this increment, the current action's cooldown entry is initialized first. Evidence: `0x2184e8` through `0x218541`.
7. Later resource work clamps mana to zero through one. Charging then occurs in the movement phase, using the already incremented action clock. A later positive-stun gate clears charge. Evidence: `0x218713` through `0x2187e0`, `0x21a74d` through `0x21a918`, and `0x21ae3f` through `0x21ae70`.

Consequently, a stun equal to this update's duration blocks an early behavior invocation but expires before the interruption test; that test then preserves the action. A still-positive stun clears the action, after which the ordinary clock increment can leave action zero with a clock equal to this update's duration. A pending ranged action and a newly applied stun can coexist at entry: there is no general stun rejection inserted before the ranged switch in this phase, so an eligible emission can precede the later interruption. These are consequences of the observed order, not a claim that every stun application reaches that state.

## Held charging and input release

The later charging phase treats actions 24, 25, 27, 8, 59, 63, and 64 together. They gain charge only once their already incremented clock is at least their anticipation duration. The ordinary gain is the timing-speed value multiplied by the update's binary32 milliseconds and `0.00075`. For class discriminator 2, specialization 0, that gain is multiplied by one plus the smaller of four and the ratio of the combo counter to the native combo-limit result at RVA `0x044cb0`. When buff 2 is present, the gain is additionally multiplied by one plus nine times the skill-contribution result for action 102. After adding the gain, buff 10 sets charge directly to the current mana. The final charge ceiling is the current mana. Evidence: `0x21a78c` through `0x21a918`; constants are in the supported image at RVAs `0x31e1e0`, `0x345e60`, and `0x302a90`.

Action 36 instead adds timing speed multiplied by milliseconds and `0.0005`, without an anticipation test in this charging branch; it then shares the final mana ceiling. It does not pass through the grouped buff-10 fill described above. Evidence: `0x21a74d` through `0x21a78c` and `0x21a8fb`.

For the ownership-allowed charging sound branch, a change in the signed integer quotient of action time divided by 200 triggers one sound numbered 55. The old and new quotients are compared; there is no loop for all skipped 200 ms intervals. Its pitch is one plus half the charge value present during action dispatch, before this update's later charge gain. No random draw is used for this sound. The original action dispatch maps 24, 25, 27, 36, 59, 63, and 64 to this branch at RVA `0x215bb3`. The original binary range through `0x215c68` confirms the quotient comparison and pitch calculation.

The original input-release dispatch maps a released held action 24 to action 23; 25 to 55; 27 to 26; and 36 to 37. Each transition sets the action clock to zero. These assignments are confirmed from original image dispatch tables at VAs `0x0049d5e4` and `0x0049d610`, and the destination ranges at RVAs `0x09be73`, `0x09be8f`, `0x09beab`, and `0x09bec7`. The release branch is entered only when the released bar action matches the creature's current action. It does not itself clear accumulated charge or the roll timer.

Held actions 24, 25, and 27 share a native start branch at RVA `0x09b989`: if the requested action already matches, it leaves the clock alone; otherwise it selects that action, zeroes the clock, and clears the roll timer through the common start path at `0x09bd83` through `0x09bdb5`. This is a local transition contract, not a complete keyboard binding or skill-availability contract. The availability routine at RVA `0x03e5a0` rejects positive stun except for requested action 101 and also checks resource and cooldown state.

The original behavior routine at RVA `0x02cb20`, in its ranged-selection path, waits for roll time to be zero and the action clock to be strictly greater than total action duration before its charge/release decisions. For equipped nonempty weapon ID 7, a current action 24 whose charge has reached at least mana releases as action 22, with clock zero; otherwise it selects/continues action 24, preserving the clock if already charging. Weapon ID 8 similarly uses 27 then 26. Weapon ID 6, and the fallback special race number 104, use 25 then 55. The first two weapon bytes are at `0xaa8` and `0xaa9`; the weapon ID is the latter. This deliberately records the native AI's action 22 release, which differs from the input-release action 23. Evidence: the original routine's native labels within RVAs `0x02e8bf` through `0x02eaf8`. Entry into this selected branch still depends on target/range and competing decisions; those are outside this bounded contract.

## Physical shot scheduling: actions 22, 23, and 26

Evidence: RVAs `0x21707a` through `0x217634`.

The anticipation crossing includes an old clock equal to anticipation, but requires the new time to be strictly greater than anticipation. Landing exactly on anticipation does not cross. At this crossing, resource cost is resolved before projectile construction. A positive native action cost is subtracted; otherwise accumulated charge is subtracted. Mana is floored at zero. Charge is copied into the retained-release field at `0x13b4`, and accumulated charge is cleared.

That crossing permits one projectile. If it did not occur, action 26 alone may emit one additional projectile when retained release charge is positive and this update crosses anticipation plus 300 ms, with the same inclusive-old, strict-new boundary convention. The extra 300 ms is a fixed offset after anticipation, not a separately tempo-scaled duration. The additional emission does not repeat the initial charge transfer or spending.

The ordinary anticipation crossing takes precedence over the extra crossing. One update spanning both thresholds emits one projectile, not two. There is no loop over missed thresholds. Once the ordinary crossing has been passed, actions 22 and 23 do not emit again from this branch. The branch contains no additional strike-end or total-duration rejection for an otherwise qualifying crossing.

The physical-shot branch normalizes the attack direction and uses speed 100. Unlike the volley and magic branches, it has no explicit positive-direction-length rejection before normalization. Zero-direction behavior of the called normalization routine is unresolved here.

Retained positive charge marks the projectile powered; action 23 is powered even at zero retained charge. An emitted powered shot removes buff 10 when present, after projectile attributes have begun construction and before the shot is queued. Projectile creation is followed by a damage random draw, insertion into the world projectile collection, insertion into the outgoing projectile collection, then a sound-pitch random draw and outgoing sound insertion. No damage random draw or shot sound occurs when neither threshold qualifies. This describes scheduling order only; damage, collision, and sound-content parity remain separate contracts.

## Arrow-volley scheduling: actions 50, 55, and 96

Evidence: RVAs `0x217634` through `0x217a21`.

At the same inclusive-old, strict-new anticipation crossing, this branch copies charge to retained release charge, subtracts the charge from mana, clears accumulated charge, and removes buff 10 if present. This branch does not floor mana immediately after that subtraction; the later general resource phase supplies the floor.

The pulse period is the strike duration divided by four using signed integer division. An update emits only if the old action clock is at least anticipation, the old clock is strictly less than anticipation plus strike duration, and the integer quotients of the new and old action clocks divided by that period differ. Those quotients use the full action clock, not time since anticipation. The attack direction must also have positive squared length.

The anticipation-crossing resource transfer does not itself force a projectile. A tick starting before anticipation can spend charge yet fail the old-clock-at-least-anticipation condition. At anticipation zero and a 500 ms strike, a tick from zero to 124 ms produces no arrow, while a tick from zero to 125 ms qualifies. This differs from the strict-new single-shot boundary test.

Each qualifying update creates exactly one arrow, irrespective of how many pulse boundaries it crosses. There is no fixed four-shot counter, no catch-up loop, and no rejection based solely on the new time having passed the strike end. The integer period and the old-time strike-window test determine the opportunities. A runtime should not infer a universal exactly-four-emissions guarantee from the division by four.

For a qualifying positive direction, the sound-pitch random draw and outgoing sound insertion occur first; projectile construction uses normalized direction at speed 150; the damage random draw follows; then the projectile is inserted into the world and outgoing collections. Rejected window/direction checks perform none of these two random draws.

## Magic projectile scheduling: actions 37 through 46 and 108

Evidence: RVAs `0x217a21` through `0x218305`.

Before the emission test, an old clock at most anticipation minus 200 ms can produce one channel sound when its 200 ms integer quotient changes across the update. This sound has a pitch random draw. It is possible for that channel sound and a projectile to occur during the same large update.

For actions other than 45 and 46, the candidate emission time is anticipation. For 45 and 46, divide the integer strike duration by three, select the last whole-period boundary at or before the new time relative to anticipation, then clamp that candidate upward to anticipation. Signed integer division truncates toward zero.

An emission requires the old clock to be strictly less than anticipation plus strike duration, at most the candidate time, and the new time strictly greater than the candidate time. The attack direction must have positive squared length. The branch creates at most one projectile per update. It does not iterate earlier crossed boundaries.

For the repeated magic actions, landing exactly on the selected candidate rejects emission even if an earlier candidate was crossed in the same update. Since the candidate is selected using the new time, the next update can emit at that boundary if its old clock equals it and its new time is greater. Integer thirds and the strike window must be preserved; there is no proved hard count of three projectiles. For example, a 500 ms strike supplies a 166 ms period and candidate boundaries at offsets 0, 166, 332, and 498 ms inside the strike window. Which opportunities result in emissions depends on the update intervals.

The action-cost subtraction is distinct from the later emission test: it occurs only at the anticipation crossing and only with positive direction length; mana is floored at zero. Consequently, an update can spend that cost without emitting when the chosen repeated-magic candidate fails its strict-new test. Conversely, a later repeated emission does not repeat the anticipation cost.

The emitted projectile reads the current accumulated charge. The world insertion occurs before spending positive charge; positive charge is then subtracted from mana, mana is floored at zero, and charge is cleared. Powered projectiles remove buff 9 if present, then the outgoing projectile insertion and outgoing firing sound follow. Thus subsequent pulses normally see charge already consumed by the earlier emission. This does not establish all projectile attribute or buff semantics.

## Limits and unresolved cases

This contract establishes positive-duration scheduling for ordinary finite state. Native divisions by strike-quarter or strike-third have no local zero-period guard in the reviewed ranges. Behavior with extreme speed/scaling values that make those periods zero, integer overflow, NaNs, or invalid pointers is not specified as a supported gameplay case.

The full speed formula, combo-limit formula, skill contribution formula, action availability rules, target/range competition, stun application durations, stun immunity/reapplication rules, collision/hit timing, rendering, network delivery order outside the local collection insertions, complete input processing order, and every possible action cancellation are outside this document. The examined scheduling branches do not alone prove a universal automatic return to action zero when total duration is reached. No original native runtime session was performed by this author.

Source status is held after complete readback. Any future enlargement should be a separately bounded source-side contract and must retain the distinction between original Alpha confirmation and reconstruction leads.
