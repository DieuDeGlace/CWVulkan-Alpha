# Alpha taming progress, threshold and capture

**FINAL / STABLE / HELD**  
2026-11-17

## Reference and scope

C is the original x86 `Cube.exe`, SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
S is the original x86 `Server.exe`, SHA-256
`1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`.
All addresses below are image-relative. The action dispatch entries and the
feeding branches were checked directly in both images. Instruction exports
were checked against the image bytes where those exports cover a site.

The creature is the actor being fed. The offerer is the actor whose held
item is being offered. The feeding action runs in the world step. The late
capture then equips the result on the local client when its separate gates
pass.

## Offering and arming

The local client writes action 82 to actor byte `+0x68` in the held-item
branch at C `0x09C2B5–0x09C2D7`. This branch requires held item kind
`0x14` at actor `+0x1020` and an existing signed action age `+0x6C`
strictly greater than the current action's finish bound. The action write
leaves that age as it was. This is the writer within the frame's gated
control path.

The creature AI at C `0x02CB20` and S `0x002F40` reaches a feeding
branch after counterpart selection and contact checks. World scheduling of
this AI is through the creature's behavior object `+0x13E4`, on the
nonzero creature-kind, mode-zero path with living health, no positive stun,
and a signed accumulated AI clock `+0x13C4` of at least 32. The AI checks
for any resident of type eight and bypasses the feeding branch when one is
present. At the feeding branch the selected counterpart must have action 82
and item kind `0x14`. The zero-extended item subtype at offerer `+0x1021`
must equal the creature's full species dword `+0x64`. Creature kind five
is eligible here when its 64-bit owner field `+0x198` is zero. The final
feeding-range comparison accepts ordered squared separation at most the
binary32 sum of twice the creature field `+0x80` and the counterpart
field `+0x80`, including equality.

The creature copies the selected counterpart's 64-bit identity from actor
`+0x08` to `+0x11D0`. To arm a cycle, its signed action age `+0x6C`
must exceed its current action's finish bound. If the current action
differs from 110, the creature resets its cycle count `+0x70` to zero.
A signed count below five sets action 110 and resets action age to zero.
After a cycle, a repeated feeding contact can rearm action 110 once its
finish bound has been exceeded. For action 110 that bound is the sum of
three separately truncated signed integers. Their binary32 numerators are
400, 300 and 500. Each is divided by the binary32 product of the native
haste read and actor field `+0x17C`.

The AI has a separate continuation if the same feeding checks admit it and
the count is at least five. It scans the creature's resident list and, if a
type-seven record is present, publishes a copy of the last such record
with zero strength and zero remaining duration while preserving its
embedded identity. It then applies and publishes type eight on the
creature with zero strength, duration 20,000 milliseconds, and the
offerer's identity inside the resident. Both outer identities on both
publications are the creature's. After a threshold transition that sets
a nonzero owner, the earlier eligibility check rejects that now-kind-five
creature on later contact.

Evidence for the AI call is C `0x211AD1–0x211B75` and S
`0x137891–0x137935`. Evidence for the type-eight gate, final contact,
and post-count path is C `0x02CE52–0x02CEF1`, `0x02D3B0–0x02D3CE`,
`0x02D533–0x02D734`, and S `0x003272–0x003311`,
`0x0037D0–0x0037EE`, `0x003953–0x003B54`. The finish-bound
paths are C `0x03D4EB–0x03D510`, `0x03D7DD–0x03D804`,
`0x03D994–0x03D9C5` and S `0x0087FB–0x008820`,
`0x008AED–0x008B14`, `0x008CA4–0x008CD5`. The haste reads
are C `0x047700` and S `0x012150`.

## World producer and the type-six hit

The world action table sends action 110 to C `0x216DE1` and S
`0x13CBA1`. World byte `+0xB4` must be zero for this producer.
The stand-alone client uses this path in its mode-zero world. The original
dedicated world is initialized to zero, while the connected client sets
the byte to one and bypasses this producer. The client and dedicated
action-table jumps are C `0x211E04–0x211E1B` and S
`0x137BC4–0x137BDB`. The mode stores are C `0x06FCA7` and
`0x06A869`, and S `0x149CB7–0x149CBD` and
`0x0C8679–0x0C867C`.

The existing signed age must be strictly below the action period. Its
signed sum with the current elapsed piece must be at least that period.
The crossing is inclusive at the end of the piece. The action times and
elapsed pieces use milliseconds. The action-110
period is 400 divided by the binary32 product of the haste read and actor
field `+0x17C`, with the division rounded to binary32 and the result
truncated to a signed 32-bit integer. The branch resolves the stored
offerer identity from the creature `+0x11D0` and again checks for item
kind `0x14` and an exact species/subtype match. These are the firing
checks after the clock crossing. Missing offerer or a failed item check
skips the publications in this branch.

Each admitted firing appends a sound event at the creature position with
selector 44, then appends one type-six hit. Pitch uses one CRT random draw.
The draw is converted to binary32, multiplied by the binary32 value of 0.2,
divided by 32,767, and increased by the binary32 value of 0.9. Each
arithmetic result is binary32. The hit is appended before the resident
strength calculation and occurs on progress cycles as well as on the
threshold cycle.

| Hit offset | Stored value |
| --- | --- |
| `+0x00` | 64-bit creature identity |
| `+0x08` | 64-bit offerer identity |
| `+0x10` | Positive binary32 zero for amount |
| `+0x14` | Zero critical byte |
| `+0x18`, `+0x1C` | Zero dwords, including stun at `+0x18` |
| `+0x20`, `+0x28`, `+0x30` | Creature position copied from actor `+0x10` as three signed 64-bit fixed-point coordinates |
| `+0x38`, `+0x3C`, `+0x40` | Three positive binary32 zeros for impulse |
| `+0x44`, `+0x45`, `+0x46` | Bytes zero, six, zero |

The hit occupies 72 bytes in the result `+0x00` list. The table gives
the stored fields. Padding values are outside the contract.

Evidence for the clock and item checks is C `0x216DE1–0x216E51` and S
`0x13CBA1–0x13CC11`. Period reads are C `0x03CAA0–0x03CAC1`,
`0x03CDF8–0x03CE1F` and S `0x007DB0–0x007DD1`,
`0x008108–0x00812F`. The sound and hit publications are C
`0x216E57–0x216F23` and S `0x13CC17–0x13CCE3`. Hit defaults
are C `0x059A30–0x059A8D` and S `0x022A90–0x022AED`.
The three feeding sound and progress constants are at C `0x2FCDBC`,
`0x2FD338`, `0x2FD31C` and S `0x1586C4`, `0x158834`,
`0x158814`.

## Progress, threshold and publication

After appending the hit, the world reads the creature's live resident list
at actor `+0x1178`. The starting strength is positive binary32 zero when
no type seven is present. With matches, it uses the last type-seven
strength in list order, selected by type alone. It adds binary32
`0x3E4CCCCD` in one binary32 addition, increments the signed cycle count
`+0x70`, and zeroes the inactivity clock `+0x74`. The ordinary world
tail can advance that clock again and resets the cycle count when the
clock exceeds 4,000 milliseconds. Resident expiry and the cycle count
have separate lifetimes.

The exact result words for five uninterrupted updates from zero are
`0x3E4CCCCD`, `0x3ECCCCCD`, `0x3F19999A`, `0x3F4CCCCD` and
`0x3F800000`. The owner transition requires the newly added strength
to be ordered and at least 1.0. On this path the world caps the strength
to exactly 1.0, sets creature kind `+0x60` to five, and calls the native
ownership routine with the creature and the resolved offerer's identity.
The helper replaces the creature's behavior state, including the normal
companion defaults and species-specific adjustments. When the owner is in
the world map, it limits the creature level at `+0x190` to the owner's
level, writes the owner ID at creature `+0x198`, and writes the creature
ID at owner `+0x11C8`.

After that conditional transition, the same firing writes and applies a
24-byte resident to the creature. The packet-facing event is a separate
40-byte copy in result list `+0x58`.

| Resident offset | Stored value |
| --- | --- |
| `+0x00` | Type byte seven |
| `+0x04` | Updated binary32 strength, capped to 1.0 on the threshold path |
| `+0x08` | Signed duration of 5,000 milliseconds |
| `+0x10` | Creature's current 64-bit owner from actor `+0x198` |

Both outer event IDs are the creature's, at event offsets `+0x00` and
`+0x08`. The resident begins at event `+0x10`. Its embedded identity
is zero on progress cycles for an unowned creature. On a successful
threshold transition it is the offerer's ID, read back from the creature
after ownership changes. Applying type seven replaces the first
same-type resident in place or appends a resident if there is no match.

The world then reaches its ordinary resident aging for the creature in the
same elapsed piece. It subtracts the signed elapsed piece from the live
remaining duration and removes nonpositive entries during cleanup.
The separately appended event still carries the producer's full 5,000.
Any later world piece can age the live resident again. In a stand-alone
client frame the produced hit already belongs to the aggregate later
visited by the capture pass.

Evidence for progress, threshold and the record producer is C
`0x216F28–0x217075` and S `0x13CCE8–0x13CE35`. The threshold constant
is C `0x345DC0` and S `0x1586DC`. The ownership writes are in C
`0x1EE7A0–0x1EEA30`, specifically `0x1EE98F–0x1EEA19`, and S
`0x122580–0x122810`, specifically `0x12276F–0x1227F9`. The common
publishers are C `0x1957C0–0x195848` and S `0x0CE9F0–0x0CEA78`.
The resident applicators are C `0x046AF0–0x046BA7` and S
`0x011740–0x0117F7`. The inactivity and resident clocks are C
`0x211F3B–0x211F75`, `0x2120E8–0x212163`, `0x2183FA–0x21843C`
and S `0x137CFB–0x137D35`, `0x137EA8–0x137F23`,
`0x13E1BA–0x13E1FC`.

## Connected delivery

- **Offerer state.** The connected client's packet zero carries the local
  actor state delta. The action and signed age are covered at C
  `0x0CDBEA–0x0CDC19`, and the item block starts at snapshot offset
  `+0x2F0` and spans `0xE38` bytes, which covers the held-item kind and
  subtype. The snapshot begins at actor `+0x10`. The dedicated receiver
  resolves the world actor using the connection's assigned identity,
  reads the packet against its actor state, and copies the decoded
  result into that actor under the world hold. Action 82 and the offering
  item can therefore be live when the dedicated creature AI runs.
  C evidence is `0x069C85–0x069CA5`, `0x069EF5–0x069FF0`,
  `0x066364–0x066371`, and `0x0CE04E–0x0CE068`. S evidence is
  switch table `0x02671C`, handler `0x0260DF–0x026271`, action
  reads `0x015EB5–0x015ED4`, and item block reads
  `0x0161A0–0x0161B0` and `0x015450–0x0154CF`.
- **Dedicated result.** The world appends the hit and buff to its fresh
  result. After the world call, the dedicated wrapper publishes that
  result to peer staging under the outer world hold. Packet four encodes
  the hit list in section 1 and the buff list in section 12. Entries keep
  the outer IDs and the resident keeps its embedded owner. The stock pair
  is world output from the feeding action. The optional packet-seven and
  packet-eight inputs have their separate receipt paths. S evidence is
  `0x14A081–0x14A0D0`, `0x02736A–0x027392`, `0x0277DC–0x0277F0`,
  `0x024D58–0x024D6A`, and `0x024F7D–0x024F8B`.
- **Connected receipt.** Packet four stages hits at controller
  `+0x800630` and buffs at `+0x800688`. After the client world calls,
  the frame applies the staged buffs for resolved targets before the
  first hit pass. The following aggregate append retains hits whose source
  identity differs from the local actor's. Thus a creature-to-local hit
  can enter the late pass with the matching type-seven resident already
  live. C evidence is `0x06CF17–0x06CF26`, `0x06D04D–0x06D05C`,
  `0x08FCB8–0x08FD6A`, `0x08FECB–0x090344`, and
  `0x090403–0x0904AE`.

## Late capture

The late aggregate pass resolves the hit target. It reaches capture for
a target whose identity equals the local actor, selector six at hit
`+0x45`, and local held item kind `0x14`. It resolves the hit source
and searches that creature's current residents. The first matching
resident must have type seven, ordered binary32 strength at least 1.0,
and embedded identity equal to the local actor. The comparison uses live
resident state at the time the hit is visited. A hit from an earlier
cycle in the same aggregate can therefore qualify when the matching
threshold resident has already taken effect.

On success the client writes a default item to the local held slot,
clears local action `+0x68`, links local `+0x11C8` to the source
creature ID, and sets held kind `0x13`. It writes the low species byte
from source `+0x64` to local `+0x1021`, the low level word from source
`+0x190` to local `+0x1030`, and the source dword `+0x194` to local
`+0x1024`. Later hits encounter the resulting held item state.

C evidence is the late traversal at `0x094971–0x0949CF`, capture
gate and source resident search at `0x094A79–0x094B75`, and item and
actor writes at `0x094B7A–0x094BFA`.

## Coverage limits

The earlier AI counterpart selection, all creature-specific behavior
classes, and all user controls that can reach the local action writer
remain outside this supplement. The late source lookup has no explicit
null guard at its resident-list access. That path's runtime result when a
source is unavailable remains unresolved. The receipt order is for the
staged group the frame reaches. Publication and transport timing across
different packet-four groups remain dependent on the frame and peer
scheduling.
