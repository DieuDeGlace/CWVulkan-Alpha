# Alpha connected combo helper and action-zero deferred cleanup

**FINAL / STABLE / HELD**

Evidence is the original 32-bit Alpha `Cube.exe`, 3,885,568 bytes, SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
Addresses are RVAs for image base `0x00400000`. The parent scope is the
living connected nonlocal creature turn at `0x20C510`. The per-candidate
call and its eligibility for the combo reward are in
[alpha-connected-melee-candidates.md](alpha-connected-melee-candidates.md).

## 1. The call between the combo writes

On a successful common hit with the saved first-struck and nonpositive-stun
condition, the caller increments the signed 32-bit combo count `+0x70`,
calls `0x0453B0` on the attacker, and writes zero to the signed 32-bit
combo timer `+0x74`. The caller reloads the attacker pointer after the call
and uses no result from it (`0x214899–0x2148B9`).

The helper reads the attacker's cooldown map at `+0x139C`. In valid native
state it visits the tree from the leftmost node through the in-order
successors until the sentinel, returning immediately for an empty tree.
It reads only the sentinel and the node links and sentinel flags. The
walk writes no fields, invokes no other routine, and takes no random draw
(`0x0453B0–0x04540B`). It visits populated maps with the same lack of
output as an empty map.

The map uses signed 32-bit action keys and signed 32-bit millisecond
values. Action availability checks present values for zero, the earlier
cooldown progression subtracts elapsed time from eligible entries and
floors at zero, and the later action-clock start writes that action's
cooldown. Those operations have their own call sites. Evidence is
`0x03B89A–0x03B8BD`, `0x03E5E5–0x03E600`, `0x068930–0x068988`,
`0x212048–0x2120E8` and `0x218507–0x218535`.

## 2. Later action-zero gate

After the buff walk and expired-entry removal, the turn subtracts
elapsed milliseconds from signed stun `+0x12C`. Only a still-positive
result clears both the current action byte `+0x68` and the signed
32-bit action clock `+0x6C` at that site. Next, controller kind 6
goes directly to the action-zero gate. For other kinds, a negative
clock with a nonzero action skips to the continuation past cleanup.
Otherwise the clock phase can proceed. Exactly zero pre-increment
clock inserts or finds the current action's cooldown key in
`+0x139C` and writes the result from `0x03E6A0` for that action and
override minus one. This includes action key zero. Then add the
full elapsed milliseconds to the live clock (`0x2184C2–0x218544`).

The cleanup gate at `0x218597` checks only whether the live action
byte is zero. A previously idle actor and an actor whose action
was just cleared by stun can both enter. The clock value and world
mode do not add predicates at this gate. Kind 6 reaches the same
gate after skipping the ordinary cooldown write and clock increment.
A nonzero action goes directly to the next common stage at `0x218713`.

## 3. Deferred walk, effects and clear order

Start at the first ID in the acting creature's deferred set
`+0x11B4` and take successors in signed 64-bit order. For each ID,
look it up in that creature's struck set `+0x11AC`. Only a missing
struck key proceeds to the live world creature-map lookup by the
same 64-bit ID. A nonnull target reaches feedback. This walk adds no
health, hostility, geometry or local-pointer condition. The tree
lookup compares signed high 32 bits and, for equal highs, unsigned
low 32 bits (`0x2185A1–0x218632`, `0x077DB0–0x077DC5`, `0x02F000`).

First call the target's dodge benefit at `0x03CF20`. Nonzero target
roll at `+0x128` and target class 4, specialization 1 admit it.
It adds binary32 0.25 to mana `+0x170`, capped at one, installs
type-11 strength zero and duration 30,000 ms with resident source
ID zero, and appends a self-addressed type-11 buff event. Then append
the type-4 hit regardless of whether the benefit passed. The hit
has zero amount and critical byte, the acting and resolved creature
IDs from their resident `+0x08` fields, and the resolved creature's
live fixed position from `+0x10`. The default stun and impulse are
zero. Each qualifying ID finishes those appends before the next ID.
The path takes no random draw (`0x218638–0x2186EF`, `0x03CF20–0x03CFE1`,
`0x059A30`, `0x086290`).

After the walk, clear struck first and deferred second, including
when deferred was empty. This cleanup writes neither the hit latch
`+0x13B8` nor the saved combo count `+0x1314`. It returns to the
common stage at `0x218713` (`0x2186F5–0x218715`, `0x27E480`).
The earlier anticipation and upper-bound feedback passes have
their own predicates and clears. Hit record presentation and
transport are outside this supplement.
