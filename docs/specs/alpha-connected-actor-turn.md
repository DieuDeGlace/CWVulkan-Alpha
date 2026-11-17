# Alpha connected-client creature turn

Status **FINAL / STABLE / HELD**

## Scope

The source is the original 32-bit Alpha client `binary/Cube.exe` with image base
`0x400000` and SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
All code addresses are RVAs in that image. The world step is `0x20C510`.
The original listing `decompiled/client/assembly-functions/0060c510.asm` supplies the
main turn and original `binary/Cube.exe` supplies the switch tables and the
instructions in omitted spans. The client mode and local actor are world byte
`+0xB4` and pointer `+0xB8`. Here a connected nonlocal actor is a creature on the
nonzero-mode path whose pointer differs from `+0xB8`. “Living” is the path taken
after maximum-health clamping when binary32 health at actor `+0x16C` is strictly
positive. Evidence includes `0x211A96–0x211AE1`.

Arithmetic below assumes finite valid state, ordinary nonnegative elapsed time,
and in-range signed arithmetic and conversions. The world step retains a signed
integer millisecond count for clocks and a binary32 conversion for float rates.
The seconds value is that conversion multiplied in binary32 by `0x3A83126F`.
Each stated binary32 stage rounds independently. Numeric action and sound
identifiers in the prose are decimal. Evidence is `0x20C549–0x20C54F` and
`0x20C83D–0x20C862`.

## 1. Admission, existing clocks and action preparation

The connected nonlocal path skips the controller behavior update, species-144
health drain and the world-byte-zero path-following span. It enters the signed
action lock at `0x211D0D`. Subtract elapsed milliseconds from actor `+0x130`.
A strictly positive remainder overwrites the action clock `+0x6C` with the
current action duration from `0x03D1A0` plus one. A negative remainder is
stored as zero. Exactly zero preserves the existing clock. The next signed
timers `+0x134` and `+0x138` subtract elapsed milliseconds and floor at zero.
A negative binary32 penalty `+0x13C` increases by seconds divided by 180 and is
capped at zero. This precedes dispatch. Evidence is `0x211AE1–0x211E04`.

The action number `+0x68` is an unsigned byte. The action clock `+0x6C`, stun
`+0x12C` and roll, lock and cooldown durations are signed 32-bit millisecond
values. The first effects dispatch reads the live action before cooldown decay,
buff expiration or common stun subtraction. The dispatch itself makes no
actor-equality check. The body at `0x212357` admits the nonzero world mode
directly. On the zero-mode path it admits either nonzero control byte
`+0x60` or the local pointer. The dispatch table identifies the following
members of that body.

| Body RVA | Action numbers |
| --- | --- |
| `0x212357` | 1–7, 9–21, 30–33, 51, 54, 57, 58, 60–62, 65–70, 72, 74–78, 86–88, 91, 93, 104 |

Connected nonlocal members can run ordinary clock-zero resource preparation
and shared pending-resource finalization. For example, at ordinary clock zero
actions 3 and 4 clear byte `+0x13C0` and recorded binary32 amount `+0x13B4`,
then record and subtract binary32 0.1 from mana. Actions 5, 17 and 20 can
record all current mana and write positive zero
to mana. When resident type 11 is present at that latter operation, a
zero-duration type-11 replacement and a self-addressed buff record precede the
write of one to `+0x13C0`. Actions 21, 54, 87 and 88 can record binary32 one
without a corresponding ordinary debit. Class 4, specialization 1, action 5,
17 or 20 at clock zero first writes a signed roll duration of 600 at `+0x128`,
before those ordinary resource operations. Evidence is the dispatch at
`0x211E04–0x211E1B`, table RVAs `0x222C84` and `0x222CCC`, and body spans
`0x212357–0x2123A9` and `0x2134F7–0x21374F`.

For common actions 31 and 33, the ordinary resets are bypassed. The
charge-cost branch first tests the pre-increment clock at or below current
anticipation from `0x03CAA0`, then clock plus the full elapsed integer strictly
above anticipation. On that crossing it records binary32 0.1, subtracts
nominal cost and floors negative mana at zero. Nominal cost is binary32 0.3,
or zero if resident type 9 is present. Presence is measured before the later
buff expiration pass. Other eligible clocks preserve the recorded amount at
this stage. At shared finalization for either preparation route, strictly
positive pending amount `+0x144` overwrites the recorded amount, subtracts the
full pending amount from mana and clears byte `+0x13B8`. Actions 5 and 11 then
halve the recorded amount without halving the debit. Shared finalization
clears pending amount to positive zero on every path and floors negative mana
at zero. This is the pre-clock action-effect phase. Evidence is
`0x213593–0x213646`, `0x213754–0x2137F1` and `0x044AE0`.

Some early sound and draw sites run for connected nonlocal actors. A member
taking the ordinary start-sound route at clock exactly zero can emit sound 17
before resource preparation. This route excludes actions 30–33, 87, 88 and
91. It requires held equipment category `+0xAA8` equal to 3 and subtype
`+0xAA9` in 5, 6, 7, 8, 10, 11, 15, 16, 17 or 18. It takes one
`MSVCR110.rand` draw at `0x212442` and appends at the actor's current position,
with volume 1 and pitch formed by round-to-binary32 conversion of the integer
draw, multiplication by binary32 0.1, division by binary32 32767, and addition
of binary32 1. Actions 47 and 48 enter a different early body. Clock at or
below zero with clock plus elapsed strictly positive emits sound 14 at the
current actor position, with one draw and the same pitch stages using 0.25
in place of 0.1. Volume is 1. The latter branch has no side or local-pointer
predicate. Evidence is `0x212168–0x2121F4`, `0x2123A9–0x212495`,
`0x044230–0x04426C`, sound defaults at `0x065FB0`, and the imported
`rand` cell at `0x2FC468`.

The physical-projectile bodies of actions 22, 23 and 26 and the
magic-projectile body of actions 37–46 and 108 require local actor equality
on the connected path. The early action-8 effects and the early grouped
charge-sound effects of actions 24, 25, 27, 36, 59, 63 and 64 have the
same rejection. These limits are at the named effect entries. The later
common clock and resource phases remain eligible for those actions. Evidence
is the dispatch tables and `0x2121F4–0x212215`, `0x215BB3–0x215BD4`,
`0x21707A–0x21709B` and `0x217A21–0x217A42`.

### Bounded downstream common effects

The common target path has additional predicates after preparation. A
connected mode check at `0x213B1E` skips a world-byte-zero span and rejoins
at `0x214084`, before candidate iteration. For action 31 on that continuation,
clock at least anticipation and resident type 9 cause the resident record to
be replaced with duration zero and a self-addressed type-9 buff record to
be appended before candidate iteration. Consequently the earlier debit
decision can see type 9 where later buff processing sees a zero-duration
record. This route still requires entry into the common target window at
`0x2137F7–0x213808`. Evidence is `0x2140AE–0x214178` and the resident
overwrite helper `0x046AF0`.

An eligible common candidate can reach the hit routine at `0x196D30` on a
connected client. The call site records the target in the struck set and
writes byte `+0x13B8` to one before the call. The hit routine's opening
connected-mode comparison rejects a target equal to the world's local
pointer. A living nonlocal target can pass it. For example, when a common
attacker using action 1 reaches this call on a living nonlocal control-kind-1
target with either actor toggle `0x0080` or contact flag `0x20`, the helper
appends sound 24 at the target position, using one draw with pitch
`1 + (binary32 draw × binary32 0.3) / 32767` in that order and volume 1.
It then appends a type-3 hit record using the caller's amount, and returns
false on the connected path. The call to the authority-side hit application
in this branch occurs only with world byte zero. Evidence is
`0x2146CA–0x214772` and `0x196D7F–0x197016`.

The hit routine's success tail at `0x1986D5` can return true in connected
mode while skipping the call to authority-side hit application. At the
common call site, success can increment the attacker's combo count `+0x70`
and zero its combo timer `+0x74`. This requires that the struck set was empty
and the attacker's stun was at most zero before the candidate was inserted.
The timeout skip below applies separately. Success also admits a change
to the attacker's meter `+0x1190` when class is 4, specialization is 0, and
the earlier temporary resource indicator is true. That indicator is true
for strictly positive recorded `+0x13B4` or current action 11. The mutation
adds the recorded amount in binary32 and caps the resulting meter at one.
It precedes the lens phase. Evidence is `0x213625–0x213646`,
`0x21469C–0x214772`, `0x214899–0x214906`, `0x077230` and
`0x1986B4–0x1986D7`. The complete selection and hit-result branches, further
mutations and transitive draws remain open.

## 2. Cooldown, buff and signed clock boundary

The connected nonlocal actor skips the `+0x74` combo timer and timeout
rescaling at `0x211F41–0x212047`. The existing cooldown pass still visits
every present signed-key, signed-duration entry at actor `+0x139C`.
It subtracts integer milliseconds, floors at zero and leaves the entry
present. This includes any existing entry for the current action. The
conditional cooldown-policy predicate `0x287D10` returns false in this
image. Evidence is `0x212048–0x2120E8`.

The buff pass subtracts signed milliseconds from resident remaining duration
before testing it. An expired duration at or below zero is queued for removal
and skips continuing effect. A surviving type 1 clears positive stun. The
continuing type-4 damage boundary is restricted to zero world mode.
The removal pass finishes before common stun subtraction. Evidence is
`0x212130–0x212163` and `0x218310–0x2184C2`.

Subtract integer milliseconds once from `+0x12C` without a zero floor.
Only a strictly positive result writes both current action and clock to zero.
An already-admitted action effect may therefore run before being cleared.
Control kind 6 bypasses the ordinary clock branch and its action-54 velocity
change, but reaches the action-zero check. For other kinds, a negative clock
with a nonzero action jumps straight to the mana phase. It skips the ordinary
clock branch, action-54 adjustment and action-zero bookkeeping region.

At an eligible clock exactly zero, the action byte widened to an integer is
looked up or inserted in the cooldown map. Its native current-state cooldown
from `0x03E6A0` replaces the value. This assigns the full result after the
earlier cooldown decay, including for action zero after stun cleared it.
The helper and its skill lookup do not draw random numbers. The clock then
adds the full elapsed integer, even if the result exceeds duration. Action
zero with a negative clock also advances, with no cooldown assignment until
a later eligible zero clock. Action 54 uses the clock **after** the increment.
It writes binary32 −60 to vertical live velocity when current anticipation
is strictly below that clock, the clock is strictly below total duration,
and it is strictly below anticipation plus middle contribution. Evidence is
`0x2184C8–0x218597`, `0x03E6A0–0x03E93A` and `0x03ED60–0x03EDDB`.

With the live action zero, the common bookkeeping checks recorded dodge
candidates against the struck set and current actor lookup. For each present
target absent from the struck set, it calls the dodge-benefit operation
and then appends a type-4 zero-amount hit record using this actor as source,
the target as recipient, and the target's position. Both sets are then cleared.
No side predicate intervenes. The benefit helper itself requires a nonzero
target roll timer and class 4, specialization 1. It adds binary32 0.25 to
the target's mana, caps at 1, installs type 11 for 30,000 ms and appends
a self-addressed buff record for that target before the type-4 hit. The
hit append is still reached when the benefit predicate fails. Evidence is
`0x218597–0x218713`, `0x03CF20–0x03CFE1` and `0x046AF0`.

## 3. Mana, blocking and the lens meter

The common mana drift uses the **live** action and clock at
`0x218713–0x2187E0`. Class 3, species 117 or 86, or held equipment category
3 and subtype 10, 11 or 12 selects the caster branch. It adds milliseconds
times binary32 0.0001 except for action 28 and, while clock is strictly
below duration, actions 95, 37, 46, 45, 31, 33, 43 and 34. Other actors
subtract milliseconds times binary32 0.00005 except with actions 24, 25, 27,
59, 63, 64 or 8. The final lower and upper bounds are 0 and 1 on all routes
through this phase. Evidence includes predicate `0x044610`.

Blocking at the next stage requires signed lock `+0x130` at most zero and
action 71, 72, 8 or 98, or action 59 or 63 together with class 1 and
specialization 1. While blocking and resource `+0x174` is positive, subtract
binary32 milliseconds divided by 1200 when equipment bytes `+0x990` and
`+0x991` are 3 and 13 respectively, otherwise divide by 600. Floor that
subtraction at zero. If blocking with a nonpositive resource, skip this
subtraction. If not blocking, add milliseconds divided by 2000. The
final upper bound is one. The nonblocking branch has no lower floor.
This phase reaches connected nonlocal actors. Evidence is `0x044590` and
`0x2187E0–0x21888F`.

A nonzero live action clears toggle `0x0010` in the 16-bit mask at `+0x124`.
A strictly positive squared norm of the live three-component intent at
`+0x40` clears `0x0400` in that mask. The same positive-intent predicate
clears action 83 or 84 without clearing its clock. These operations precede
the meter. They apply to connected nonlocal actors independently of the
player-owned producer of `0x0400`. Evidence is `0x21888F–0x2188FD` and
norm helper `0x024860`.

On actions other than 79, the lens meter reads the then-current `0x0400`
bit. With it set, obtain current-state skill contribution for action 99
at `0x03ED60`, multiply by binary32 0.0005, add binary32 0.00001, multiply
by binary32 milliseconds, add the meter at `+0x1190` and cap at one. With
the bit clear, subtract milliseconds multiplied by binary32 0.0005 and
floor at zero. On both routes, a resident type-3 buff then forces the
meter to one. Shared final mana adds the product of milliseconds, binary32
0.00025, and the resulting meter in that order, then caps mana at one.
The same final checks follow action 79. There is no connected-mode or
local-pointer test spanning these meter and final-mana operations. They
read the fields after the earlier common mana phase and buff expiration.
Evidence is `0x2195BA–0x2195C4`, `0x219C12–0x219CF9` and `0x043F10`.
The binary32 rates have bits `0x3A03126F`, `0x3727C5AC` and `0x3983126F`.

The current-state skill helper selects signed entry 8 at `+0x1158` for
action 99 and entry 7 at `+0x1154` for action 79. An exactly zero entry on
a nonzero control kind uses signed level `+0x190` divided by two with
truncation toward zero, then adds unsigned byte `+0x1A8` and one.
For the resulting integer the helper converts to binary32, multiplies by
binary32 0.1, adds 1, divides 1 by that result, then subtracts it from 1.
Evidence is `0x03C9C0–0x03CA08`, its selector table and `0x03ED60–0x03EDDB`.

### Action 79 route

Action 79 enters before the meter's `0x0400` test. It uses the current-state
skill contribution for action 79 and scales the then-current speed limit by
`0.5 + 0.4 × skill`. Its meter adjustment then consults present lighting
inputs and live velocity. It retains this eligibility for connected
nonlocal actors. The route's measured inputs and order are as follows.

1. Take the signed world day clock from `+0x80015C`. Form binary32
   `2 × clock / 86400000 − 1`, then square using the power helper
   `0x011D10` with exponent 2. Subtract that result from binary32 1
   and square with the same helper. Each helper promotes its binary32
   arguments to binary64 and rounds its result back to binary32.
2. Multiply the second square by the light sample from `0x19FEA0` at the
   actor's current position. The query truncates each signed fixed coordinate
   by 65536 toward zero. For block type 13 the sample numerator is 255.
   For type 0 or 2 it is the larger of 5 and the block's first unsigned
   byte. For other types it is zero. Divide the converted numerator by
   binary32 255.
3. In each visited region, the traversal checks three collections in order.
   At `+0x04` it accepts records with mask `0x01` at record `+0x38`.
   At `+0x0C` it accepts type 50 and at `+0x30` it accepts type 18.
   For a squared distance strictly below 256,
   add one half of the square of `1 − distance² × 0.00390625`. The first
   two groups use the position at record `+0x08` and the third at `+0x118`.
   Each distance uses the three-component fixed position difference
   converted to binary32 blocks and the binary32 squared norm. Cap the
   running total at one after the region traversal.
4. Visit nonnull creature-map entries carrying `0x0200` at actor `+0x124`.
   At squared distance strictly below 100, add the square of
   `1 − distance² / 100`. This stage does not exclude the actor itself,
   and has no upper cap on the total before its use below.
5. Query the action-79 skill again and form `0.9 − 0.5 × skill`, multiply
   by the light total, and subtract the product from 1. Query the skill
   once more and form `0.5 − 0.5 × skill`, multiply by the full length of
   current live velocity at `+0x34`, and subtract that
   product from the first result. Multiply the resulting factor by
   `milliseconds × 0.0005` and add the old meter. Floor the updated
   meter at zero, then use the shared upper cap and the type-3 and final
   mana operations above.

The route is `0x2195C4–0x219C12`, including the speed assignment
at `0x219609`, regional checks at `0x2196D3–0x2199FB` and creature-light
checks at `0x219A0D–0x219B1C`. Supporting native helpers include `0x071910`,
`0x19FEA0`, `0x024860`, `0x023F20` and `0x011D10`. The region list's
construction and the lighting records' network lifetimes are outside this
audit. There is no direct draw from the random import in this route.

## 4. Post-clock movement, roll and charge

Timed consumption effects for 80/81 are skipped by the ownership check at
`0x218903–0x218918`. The post-clock action-81 completion gate still runs
at `0x21A1EA–0x21A211`. Clock strictly above the item duration from
`0x0C6B80` sets action 83 without resetting that clock. At or below duration,
this branch instead sets the temporary speed limit to zero.

Movement uses live intent `+0x40` for acceleration and live velocity
`+0x34` for displacement. Positive current stun zeroes the intent-derived
acceleration contribution. Connected nonlocal melee lunge is excluded by
`0x219EE5–0x219EF5`. Evidence for intent and velocity is
`0x218F44–0x218FA0` and `0x21AEA0–0x21AEE0`.

The old roll field `+0x128` is tested for **nonzero** at the gate before those
movement branches. Actions 50 and 96 keep their action while clock is at most
total duration. Class 4, specialization 1 preserves actions 5, 20 and 17
as well. In that class and specialization other actions are cleared with
charge retained. Elsewhere cancellation clears both action and charge
`+0x144`. The clock is unchanged at this site. This is before the regular
roll decrement near the tail of the turn. Evidence is `0x21A211–0x21A266`.

Shared movement tests that follow read the surviving action and current
signed clock. Define anticipation as the current result from `0x03CAA0`
and middle as the current result from `0x047310`. Each limit in the
table is the speed limit set at that site. Direction is relative to
the actor's current yaw.

| Action | Current clock condition | Effect |
| --- | --- | --- |
| 68, 69, 93 | At or below anticipation | Speed limit zero. |
| 68, 69, 93 | Strictly above anticipation and below anticipation plus three quarters of middle, truncated toward zero | Replace acceleration contribution by a forward vector of magnitude 20. Speed limit 50. |
| 76, 77, 78 | Strictly above anticipation and below anticipation plus middle | Replace acceleration contribution by a vector of magnitude 20, forward for 76 and yaw-rotated horizontal vectors (−1, 0) for 77 and (1, 0) for 78. Speed limit 60. |
| 71 | Strictly below duration | Speed limit zero. |
| 72 | Strictly below duration | Add binary32 `720 × seconds` to live yaw. Speed limit 12. |
| 57, 74 | At least anticipation and strictly below duration | Speed limit binary32 0.1. |

Control kind 4 has a separate speed-limit overwrite to 30 after the
71/72 blocks and before the 57/74 blocks. These branches run for the
connected nonlocal actor when their predicates hold. Evidence is
`0x21A266–0x21A74D`.

Later charge uses the action surviving that gate, with no connected-mode
or local-pointer rejection. Action 36 adds binary32 milliseconds times
0.0005, multiplied by the binary32 timing-speed return from `0x047700`.
Actions 24, 25, 27, 8, 59, 63 and 64 instead require live clock at least
anticipation, and form the corresponding gain with binary32 0.00075.
In that eligible grouped branch, class 2/specialization 0 multiplies the
gain by one plus the smaller of four and the binary32 quotient of combo
count and current combo limit. The quotient branch at or below four
repeats the combo count and limit read. A surviving type-2 buff then
multiplies the gain by one plus nine times the current-state skill
contribution for action 102. The gain is added before the type-10 lookup.
A surviving type 10 in this same eligible grouped branch assigns charge
equal to current mana. Every route reaches the ceiling against current mana,
lowering charge only when charge exceeds mana. The shared final mana from
the lens stage is already available at that ceiling. Evidence is
`0x21A74D–0x21A918`, `0x044CB0` and `0x043F10`.

Glider crash stun assignment at `0x21ACDC` and `0x21ADE5` requires local
pointer equality. The following **late stun** check is shared. A positive
live stun clears toggles `0x0010` and `0x0001` and writes binary32 positive
zero to charge. This site preserves action, clock and stun duration.
Next, a nonzero action with clock strictly below its duration independently
clears the same two toggles. Connected nonlocal actors then enter displacement,
custom contact and collision processing. The separate unstuck call is
skipped by the connected nonlocal gate. Evidence is `0x21AC47–0x21AEA0`
and `0x21B456–0x21B483`.

## 5. Collision and late feedback

The connected nonlocal path can emit the following events in this
relative order within a creature turn. The sound helper appends via `0x0864C0`.
It receives sound records with the defaults at `0x065FB0` and the position
conversion at `0x02C4A0`. These are event construction and append sites.
Playback, network transmission and the later consumer's own random draws
are outside this scope.

1. **Blocked z move.** The z-axis collision response can emit sound 32 if
   the world has a nonnull local creature and live vertical velocity is
   strictly below −2. This does not require that the moving actor be the
   local creature. The record uses the current moved position before
   undoing the blocked z displacement, volume 0.5, and one random draw
   with pitch `1 + (binary32 draw × binary32 0.2) / 32767`. The draw is at
   `0x21C1A0`. The separate fall-damage/stun/sound-23 branch requires local
   actor equality and is skipped by the connected nonlocal actor. The
   general z response still undoes the z move and zeros live and push
   vertical velocity. Evidence is `0x21C14C–0x21C1F9`,
   `0x21C20C–0x21C27F` and `0x21C41D–0x21C48B`.
2. **Water entry.** In the post-move water branch, the updated flags must
   have water bit `0x02` and action must differ from 107. For each elapsed
   millisecond, velocity takes a binary32 approach step toward zero at
   binary32 rate 0.005. After that relaxation,
   the splash requires the saved pre-rebuild water bit clear and the
   **then-current** vertical velocity strictly below −3. It first appends a
   type-15 visual event at the actor position with vector (0, 0, 10),
   color (0.2, 0.7, 1, 1) and binary32 0.4 at record `+0x34`. It then takes
   one draw at `0x21E7F2` for sound 31, pitch binary32 `0.9 + draw / 32767`
   and volume 1. Both use this creature's current position. There is no
   local-pointer presence or identity predicate on this splash span.
   Evidence is the saved bit at `0x21CCA7–0x21CCEB`, the water drag and
   splash at `0x21E684–0x21E836`, the approach helper `0x205950` and
   effect append `0x086470`.
3. **Ground walk phase.** A ground walk path reaching `0x21EC08` can append
   one footfall after advancing binary32 cycle `+0x118C`. It needs a
   nonnull world local pointer, zero old roll field, no current water bit,
   horizontal live speed strictly above 5, and a strictly greater
   signed truncation of the new cycle divided by binary64 π than of the
   prior cycle divided by binary64 π. Both cycles are converted from
   binary32 before the divisions. The query at `0x21EE38–0x21EE79`
   samples the support block. Type 3 takes one random draw at `0x21EE7E`,
   selects sound `33 + (draw mod 3)` and uses pitch 1. Every other type
   takes one draw at `0x21EEAB` and uses sound 32 with the same pitch
   stages as the blocked-z sound. Both use the current actor position
   and volume 1. The presence check does not compare the actor to
   the local pointer. The alternate attachment, airborne and climbing
   animation paths need not reach this footfall gate. Evidence is
   `0x21E9B5–0x21EC08`, `0x21EC08–0x21ED8F` and `0x21ED8F–0x21EF06`.
4. **Dodge threshold.** With a nonnull world local pointer, signed old roll
   time strictly above 500 and old time minus elapsed at most 500,
   append sound 26 at the actor position, with unit pitch and volume
   and no random draw. The actor identity is unrestricted there.
   After the sound check, store the old signed roll in `+0x1318`,
   subtract elapsed from the current roll if nonzero and floor it at
   zero. The snapshot and timer update also run if the sound is skipped.
   Evidence is `0x21F63D–0x21F6DA`.

The late equipment tests can independently clear `0x0010` when equipment
bytes `+0xF08` and `+0xF09` fail the required pair 23 and 0, and clear
`0x0200` when `+0xDF0` differs from 24. These tests follow the walk-cycle
and footfall region and precede the dodge threshold. The connected mode
jump that bypasses the optional world-byte-zero static pose adjustment
is after the dodge snapshot and timer update. Evidence is
`0x21F008–0x21F03B` and `0x21F6A9–0x21F6E7`.

## 6. Drawn aim reads the late live clock

For the living paths reaching `0x21FD54`, drawn aim `+0x138C` relaxes
toward live aim `+0x160`. The step weight starts at binary32 zero, and
`0x011D40` repeats the binary32 approach toward one at rate `0x3CCCCCCD`
for the integer milliseconds. Each vector component takes a binary32
live-minus-drawn subtraction, multiplies by that weight and adds the
old drawn component.

Only after the approach does this site test the live action byte and
**live signed actor clock**. Action zero or clock exactly zero copies
live aim over drawn aim. Otherwise it queries current total duration
and then reads the actor clock again at `0x21FDEF`. A clock strictly
greater than that signed duration also copies live aim. Equality to
positive duration remains on the relaxed path. A negative clock reaches
the signed duration comparison when the action is nonzero. This point
reads the post-effects actor fields, including any clock overwrite,
increment or action assignment that preceded it. Evidence is
`0x21FD54–0x21FDFC` and `0x011D40`, `0x0121C0`, `0x0252F0` and
`0x012850`. The live aim and the interpolation of position and rotation
are separate record fields.

## Limits

The complete action dispatch, target selection, remaining hit-resolution
branches, attachment variants, packet application and downstream event
consumption remain open. The random calls cited here are per actor and
per world step. The total draw count across actors, conditional action
bodies and individual steps remains open. Evidence for this contract is
static and limited to the original client image.
