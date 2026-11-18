# Alpha dedicated projectile residents and relay

Status: FINAL / STABLE / HELD

## Scope and evidence

Server addresses are RVAs in the original 32-bit Server.exe,
1,718,784 bytes, SHA-256
`1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`.
Comparisons identify the original 32-bit Cube.exe, 3,885,568 bytes,
SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
The ordinary dedicated turn and projectile types 0 through 4 are
covered. Projectile launch stat formulas and the complete shared
hit-application branches are outside this ruling.

## 1. Dedicated world and two projectile lifetimes

Dedicated startup passes zero for the world mode to `0x0C8570`.
The constructor stores mode byte zero at world `+0xB4` and a
null local-creature pointer at `+0xB8`. Those are the dedicated
world values used below. Connection setup sets the joining
creature's controller byte `+0x60` to zero
(`0x149CB7–0x149CC4`, `0x0C8679–0x0C8689`, `0x025A01–0x025A23`).

The scheduler holds the world lock across the turn. It
initializes an output collector and calls world `0x1322D0` with
the current signed tick difference, the collector and its other
turn input. The world runs actor work, including the launch
sites below, then its final actor walk at `0x145CC2`. The
resident projectile walk begins at `0x146830`, after that
actor walk ends. Newly launched residents are already at the
tail of world `+0x14` when it begins. Each launch also takes a
separate output copy in collector `+0x20`. The first live
step can therefore change or remove a resident while the
collector still contains its launch record
(`0x14A07F–0x14A0AE`, `0x146809–0x14688D`).

Packet 9 ingress follows a separate lifetime. The native
selector at `0x0260CF–0x0260D8` selects the packet-9 reader
at `0x026560`. One received 112-byte record is appended under
the world lock through receive-context `+0x20` to host
`+0x44`. That pointer comes from the thread context setup
at `0x025DBA–0x025E0E` and `0x026B70–0x026B73`. This append
has no resident-list destination.

After the world turn returns, `0x14A0C2` invokes `0x0281D0`
with collector `+0x20` as destination. It appends the entire
pending host range at the tail in ingress list order and clears
host `+0x44`. Thus packet 9 records follow server-produced
launch records already in the collector. They leave this path
as relays, without a server flight or collision visit.
`0x14A0D0` then calls `0x0272D0` for fanout. The projectile
range reaching a peer is copied in order to peer `+0x38`.
The peer sender snapshots and clears that range before sending
packet 4 section index 4, with a 32-bit count and 112 bytes per
entry. Earlier peer entries retain their order when new
entries append
(`0x0265AC–0x0265C4`, `0x0281D0–0x02822D`,
`0x02768E–0x0276AE`, `0x024907–0x024A40`,
`0x024D05–0x024DB6`, `0x021AA0–0x021B04`).

## 2. Native launch copies

The six paired sites below append to world `+0x14` first and
collector `+0x20` second through `0x028360`. They copy the
same temporary record into distinct list nodes. Initial age
`+0x68` is zero and the source key at record `+0x00` comes
from the producing actor's key at `+0x08`. The copy retains
the signed fixed position at `+0x18`, binary32 velocity at
`+0x3C` and typed collision fields at `+0x48–0x6C`. The
resident list visits these entries in insertion order
(`0x022890`, `0x028360–0x0283A0`, `0x14683B–0x14688D`).

| Resident append | Output append | Verified record selection |
| --- | --- | --- |
| `0x13B230` | `0x13B245` | Type 3. Variant 2 for action 90, otherwise variant 1 in this branch. The entry is the action-89/90 branch. |
| `0x13BF84` | `0x13BF99` | Type 3, variant 2, zero charge. The preceding branch tests actor class `+0x140` equal to three and specialization `+0x141` equal to one. |
| `0x13C6BE` | `0x13C6D3` | A second type-3, variant-2, zero-charge site, also gated by class three and specialization one. |
| `0x13D2E6` | `0x13D2FB` | The action-22/23/26 branch makes type 2 for action 26 and type 0 for the others. |
| `0x13D7BC` | `0x13D7D1` | Type 0 with charge one. Entry is the action-50/55/96 branch. |
| `0x13DF19` | `0x13E021` | Type 4 for action 108, otherwise type 1 in the branch for actions 37–46 and 108. |

The selection above is from the action byte at actor `+0x68`.
The selector subtracts one, checks the result against unsigned
`0x6D`, reads the byte table at `0x148A8C` and uses the
pointer table at `0x148A44`. Its slot 2 enters `0x13CE3A`,
slot 6 enters `0x13D7E1`, slot 9 enters `0x13D3F4`, and
slot 11 enters `0x13AFB9`. In these four branches, the
projectile-producing stage rejects a kind-zero source for the
dedicated world with null local pointer. Slot 11 has some
earlier sound work before that gate. This confirms the launch
role at the traced sites even with a network player's live
action byte in the actor map. Admission timing, source stats
and the full entry conditions for the two other type-3 sites
remain partial
(`0x137BC1–0x137BDB`, `0x13B072–0x13B0A2`,
`0x13BDFA–0x13BE50`, `0x13C58D–0x13C5E1`,
`0x13CE3A–0x13CE81`, `0x13D1A8–0x13D1D5`,
`0x13D3F4–0x13D415`, `0x13D7E1–0x13D802`,
`0x13DBCD–0x13DCD2`).

## 3. Resident flight and motion

Convert signed world-step milliseconds to binary32, then
multiply by binary32 0.001. Type 1 enters displacement without
a gravity change. Types 0, 3 and 4 subtract from Z velocity
the product of seconds times 30, then times 0.25, with the two
binary32 multiplications kept in that order
(`0x132605–0x132622`, `0x14688D–0x14689C`,
`0x146A2F–0x146A59`).

Type 2 first looks up its full source key at `0x005420`.
A missing source or signed old age strictly above 1,200
queues removal before displacement. Up through age 800,
convert source aim `+0x160` to fixed units by binary32
multiply by 65,536 and truncation, add it to live source
position, and subtract live projectile position in fixed
units. Convert that signed fixed delta component to
binary32 before multiplying by 1/65,536 and then by five.
`0x0079F0` is scalar multiplication on each component.
Use the full desired velocity for the per-millisecond
binary32 blend at `0x12E7C0`. On each iteration, subtract
current from desired, multiply the difference by 0.05, then
add it to current. A nonpositive signed step does not iterate.
Above 800 through 1,100, replace velocity with the converted
source-minus-projectile delta times ten, with no aim addend.
Neither desired velocity construction normalizes its delta.

Above 1,100 through 1,200, take the signed difference of old
age and 1,100, convert to binary32 and divide by 100. Convert
that factor to fixed units twice. Scale the fixed source-minus-
projectile delta by the first factor, truncate the signed
product quotient by 65,536, and repeat with the second factor.
Add the resulting fixed delta to the projectile position.
The existing velocity then supplies displacement. These
three steering routes match the original client arithmetic,
including the pre-increment age selection
(`0x1468A2–0x146A2D`, `0x002510`, `0x002550`,
`0x002A10`, `0x002BD0`, `0x0079F0`, `0x12E7C0`).

Form the binary32 full displacement from velocity times
seconds, then add signed step milliseconds to age. Type 2
bypasses the common expiry. For the other types, new age
strictly above 5,000 queues removal. For a projectile other
than type 3 with a resolved source, clear source combo `+0x70`
and rescale its clock `+0x6C` by the ratio of action speed
before the clear to speed after it. The ratio and product use
binary32 stages, followed by a signed truncated clock result
(`0x146A5D–0x146B35`).

An unexpired resident computes the signed substep count by
truncating binary32 two times full displacement length plus
one. The length uses binary32 squared components and sum,
binary64 square root and binary32 result. Divide the full
displacement by the binary32 count for the repeated substep
vector. Before the candidate loop, copy velocity into the
impulse, normalize the copy when its squared length is ordered
positive, scale by binary32 five times record `+0x54` and
overwrite Z with binary32 three times `+0x54`. This prepared
impulse is used in both direct and impact calls. A nonpositive
substep count bypasses the substep walk
(`0x147225–0x147350`).

## 4. Direct candidates and server authority

### 4.1 Admission, roll and timing

A substep resets the creature-stop and success indicators and
starts the world creature map at its first key. The order is
signed 64-bit ascending, with signed high-word then unsigned
low-word comparison. It skips a null object or ordered health
at most zero. For a resolved source it excludes the same
pointer, requires relation `0x0D18C0` and rejects the target
object's full ID matching source `+0x11C0/+0x11C4`. A missing
source or type 3 variant 2 bypasses the source tests.
Relation `0x0D18C0` returns true for controller kind six on
either side, exactly one controller kind one, a different
`0x00F710` class for a pair of kind-one actors, or status mask
`0x20` at `+0x124` on either actor after those kind tests.
It returns false for identical pointers.

Use the resident position and target's live `+0x10` position.
Actor half-sizes derive from the three binary32 `+0x80`
components times 0.5. The projectile half-width is field
`+0x4C` on each axis. Scale the offsets by 65,536 and truncate
to signed fixed units. Test projectile upper bound at least
actor lower bound on X, Y, Z, then projectile lower bound
strictly below actor upper on X, Y, Z. Line of sight uses
`0x0D4D80` with limit 200 and object flag one.

After line of sight, a nonzero target roll dword at `+0x128`
skips this contact. For a resolved source it inserts target
ID into source `+0x11B4` first. A missing source still skips a
rolling target, without insertion. The source and geometry
tests and the roll branch precede all these per-contact draws
(`0x130600–0x130673`, `0x147350–0x147731`,
`0x0CE350–0x0CE3AD`, `0x0D18C0–0x0D194C`).

An admitted contact on type 0, 1 or 4 sets creature stop.
Those types run direct hit evaluation only at charge zero.
Type 3 evaluates on an old-to-new signed age quotient change
by 1,000. Type 2 requires new age strictly above 300 and a
signed quotient change by 100. Both divisions truncate toward
zero and recover old age by subtracting step milliseconds.
Types 2 and 3 continue the candidate walk after the contact.
Types 0, 1 and 4 end it even on a false shared hit result
(`0x147731–0x1477FE`, `0x147C81–0x147CDE`).

### 4.2 Direct draws and result

For an uncharged type 0 or 2 with a resolved source and a
cadence-admitted contact, draw at `0x147831`. In binary32
compute one minus two times draw divided by 32,767, multiply
by 0.05 and add 0.1. Apply factor 0.2 for type 2, then multiply
by the source duration from `0x0084B0` converted to binary32
and divided by 300. Draw at `0x1478AE` and double the result
if its remainder by eight is zero. In the dedicated world
the mode-zero branch credits source mana `+0x170` and caps
the result at one before critical evaluation. The client with
a connected nonlocal source reaches the same draws but skips
that credit.

Next a resolved source runs critical helper `0x00F520`.
The draw at `0x00F565` occurs even with a resident type-11
buff, which chooses probability one. Otherwise probability is
the result of `0x009AC0` plus binary32 0.15 times source
`+0x1190`. Compare probability strictly greater than the
binary32 draw divided by 32,767. A missing source gives false
without this draw. The hit amount is binary32 `+0x48`, doubled
only for critical. After this, the dedicated world permits the
weapon proc call with input 0.15 for a resolved source of
nonzero controller kind. Its draw is `0x00F26E`. The helper
scales the input by 0.5 for category-3 weapon subtypes
0, 1, 2 and 12, by 0.3 for subtypes 8 and 10, and by one for
the other entries. The result is strict probability greater
than draw divided by 32,767. A successful result with zero
charge and source class 2 specialization 1 runs the type-10
buff and sound-47 branch. The connected nonlocal-client
ownership gate skips this proc. Direct amount floors ordered
negative values at zero before selecting the hit route
(`0x1477FE–0x147AC3`, with amount clamp at
`0x147AB3–0x147AC3`, `0x00F220–0x00F2AC`,
`0x00F520–0x00F59E`).

For variant 2 the separate direct route requires a resolved
source, false relation `0x0CFCC0` and target kind other than
six. This relation has the same kind-one class and status
tests as the earlier relation. It returns false for either
kind six and for source kind five with target kind zero,
and does not test pointer equality. Append the sign-negated
type-0 amount, halved for source kind one, with the projectile
source key, target live key and position, and critical flag.
Dedicated mode zero calls the record-apply routine on that
output. Other variants call ordinary hit helper `0x0CFD50`
with the same projectile inputs as in the impact call below.
When the direct output route reaches the follow-up, raise
signed target `+0x134` to the projectile's `+0x6C` if larger.
A false helper result or a skipped variant-2 append can still
reach that clock write
(`0x0CFCC0–0x0CFD4A`, `0x147AC3–0x147C81`).

### 4.3 State ownership after a hit record

The ordinary hit helper initially checks target health again.
Its connected local-target rejection is inactive in the
dedicated mode-zero world. When its normal type-0 path
produces a hit, it appends the record to the collector's hit
list and calls `0x0CEA80` with that same record and collector.
The call performs a fresh target-key lookup and rejects a
missing object or ordered nonpositive health. If world
`+0xB8` is null and target controller kind is zero, it
appends the same record again to the hit list and returns
before the ordinary target application. A normal type-0
hit from this branch therefore contributes two consecutive
copies for a still-living kind-zero target. Direct variant 2
uses its own first append then the same mode-zero apply call,
with the same second-copy result for a qualifying target.

A living target of nonzero controller kind continues into
server-side application. The positive ordinary health write
requires `0x009620` on the target and on a resolved source.
That check admits controller kinds 0, 1, 3 and 5, plus kind 6
except species 140, 141 and 142. In the ordinary health path,
subtract the signed amount from binary32 health `+0x16C`.
The negative-amount route caps the result at target maximum.
The ordinary death branch floors nonpositive health at zero.
The shared routine has additional status,
impulse, shield and other hit-type gates. Their full results
are outside this ruling. The kind-zero early return concerns
this record application. It leaves the caller's later
`+0x134` comparison and earlier hit-helper work separate.

The collector-to-peer hit copy takes the range head to tail
without deduplicating these records. This is independent of
the packet 9 projectile range, and the result survives peer
output collection as separate records
(`0x0CFD9F–0x0CFDD4`, `0x0D1697–0x0D16F5`,
`0x0CEACB–0x0CEB3D`, `0x0CEFEC–0x0CF078`,
`0x009620–0x009654`, `0x0CF153–0x0CF170`, `0x02736A–0x027392`,
`0x020EF0–0x020F77`).

## 5. Terrain, impact and removal

After the direct walk, type 3 advances to the next substep
without a position advance or terrain probe. Type 2 first
tests the signed age quotient change by 200 and on a change
draws `0x147D41` for current-position sound 15, pitch
binary32 one plus draw times 0.25 divided by 32,767 and
volume one. The bucket is evaluated per substep.

The other covered types and type 2 after that sound advance
by the fixed substep displacement. The sampled cell passes
when its low-five-bit type is zero or two. Other types cause
rollback, and type 0 also appends the deterministic cell-colored
effect. A passable cell calls the server ray
`0x0D6730` with advanced position, negated normalized current
velocity, maximum equal to velocity length times full step
seconds, and flags zero and one. Only a result strictly less
than the limit rolls back and sets terrain contact.

The server ray body `0x0D6730–0x0D786A` has the same relative
instruction and branch layout as client `0x1A35D0–0x1A470A`.
The native float constants match in both images. The server
fallback cells initialized at `0x157110`, `0x157140` and
`0x157170` have the same type bytes 0, 1 and `0x82`. The
server follows the fixed cell-boundary march, X/Y/Z tie
order, zero and unordered-limit gates and placed-object pass
described in `alpha-rulings-projectile-ray-and-variant.md`.
The ray does not spend a draw. The calls and rollback at
`0x148001–0x14808E` establish the same post-advance ray
boundary in the dedicated world.

Terrain contact without a creature stop can clear the
resolved source's combo and retime it with the same before/
after speed ratio as expiry. Type 2 increments combo, calls
`0x0103A0` and clears source `+0x74` on a true substep result,
then zeros velocity if terrain contact was set. It continues
its substeps. The nearby sound-19 draw at `0x1481C6` has the
same inconsistent entrance as the client copy. Its only
incoming test would require the previously admitted type 2
to change before the repeated type read. The intervening
actor combo work and tree traversal make no type write
(`0x148099–0x148212`, `0x0103A0`).

For types 0, 1 and 4, either stop flag admits retirement. Type
1 or charge nonzero admits a second candidate scan. On
terrain contact without creature stop the scan first draws
at `0x148243` and appends sound 42 for type 1 variant 2,
39 for other type 1, or 20 for other charged records. Pitch
is binary32 one plus draw times 0.4 divided by 32,767,
volume one, at the current projectile position.

This impact scan has an earlier authority gate than the
direct walk. After the first nonnull candidate, a resolved
kind-zero source in the dedicated world with null local
pointer aborts the scan before geometry or per-target dice.
With a resolved eligible source, the walk checks same
pointer, relationship, linked ID and health before overlap
and line of sight. Use fixed extent five on each projectile
axis, then the same asymmetric actor-bound and sight
predicates. After sight, roll with a resolved source defers
and skips. A missing source bypasses these source-dependent
gates, including this roll skip. The shared hit helper will
still retest target health. The preceding terrain sound can
already exist when the scan is aborted
(`0x148217–0x1483A7`, `0x1483A7–0x14866D`).

For each admitted candidate, the resolved source critical
call `0x148687` precedes the weapon proc at `0x148703`.
The proc takes 0.25 and is eligible for the dedicated
nonzero-kind source. A passing result on zero charge, type
1 variant 1 gives the type-9 buff and sound-47 route. Impact
amount is raw `+0x48` doubled on critical, without the
direct's negative floor. The ordinary call at `0x148878`
uses live target and source, amount, critical result, cached
raw charge, raw `+0x54` strength, position and prepared
impulse addresses, collector and scratch, type-1 flag,
zero action argument, resident-record address and a final
enabled flag of one. It also handles variant 2. A true result
sets substep success
(`0x14866D–0x148890`).

On retirement, a true result and resolved source increment
combo, call `0x0103A0` and clear `+0x74` for a non-type-3
resident. Type 1 variant 2 converts in place to type 3. Age
becomes 3,000 for the old zero charge or zero for nonzero
charge. Base amount is multiplied by 0.05, extent becomes
binary32 five times old strength plus five, and strength and
charge become zero. The same node survives with its variant,
position and velocity. There is no second projectile output
append for this conversion. Other retiring residents enter
the deferred erase walk after resident iteration
(`0x1488D0–0x148952`, `0x146B28–0x146C02`).

## Evidence limits

The server-to-client separation is established for these
original dedicated lists and ordinary well-formed messages.
The whole producer timing and stat formulas, all alternate
shared hit results, application death side effects, and the
separate incoming hit-message path remain open. The relay
ruling concerns packet 9 and the resulting packet 4
projectile section.
