# Alpha connected receipt supplement

**FINAL / STABLE / HELD**

Independent source ruling, 2026-11-17. This supplements the
[connected client receipt order](alpha-rulings-connected-client-receipt-order.md).

| Reference | Supported original image | SHA-256 |
|---|---|---|
| C | `binary/Cube.exe` | `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf` |
| S | `binary/Server.exe` | `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1` |

Both preferred image bases are `0x00400000`. Code and data references
below are RVAs. C and S identify the image per site. Field offsets are
relative to the named object or record. The evidence is original
instructions checked against each image, native dispatch tables, and
direct original decoding where the instruction export has gaps.
Packet sections below are counted from one within the decompressed list
group.

## Objective updates at controller +0x800690

### Wire record and staging

The intervening list is the received objective-update list. Packet 4
section 13 contains a four-byte count followed by that many 56-byte
records in list order. Each record begins with two signed 32-bit
objective-cell coordinates at `+0x00` and `+0x04`, then eight identity
bytes at `+0x08`, then the 40-byte objective descriptor at `+0x10`.
The descriptor's kind dword is record `+0x18` and the state byte is
record `+0x25`.

The dedicated sender snapshots the peer list at `+0x78` as group
`+0x60` and serializes section 13 immediately after the 40-byte buff
section. The client reads section 13 into its decoded group and appends
copies to controller `+0x800690` under the world critical section. The
controller initializes this list through the thirteen-list constructor
at controller `+0x800630`. The three explicit C text references to
controller `+0x800690` are the receive copy, the frame reader, and the
frame clear. Local world producers below append to the separate frame
aggregate's `+0x60` list.

Evidence at S `0x024907–0x024911`, `0x0234E6–0x0234F1`,
`0x024B93–0x024BBD`, `0x024D05`, `0x024F7D–0x024F9E`, and
`0x021CE0–0x021D44`. C evidence is `0x059FB6–0x059FC0`,
`0x058650–0x058753`, `0x06CED8`, `0x06D06C–0x06D088`,
`0x065C00–0x065C63`, `0x08FDC3`, and `0x090646`.

### Immediate mutations and later effects

The frame visits the list at C `0x08FDC3–0x08FEC1`, after resident
buff application and before the first hit traversal. For each record,
the world resolves the objective cell by its exact signed coordinate
pair. That lookup admits coordinates from 0 through 8,191 inclusive.
It finds the enclosing 8-by-8 region by the integer quotients by eight.
For a present region, the cell index is eight times the remainder of
the first coordinate plus the remainder of the second coordinate.
The cells start at region `+0x14018` with a 104-byte stride. The
region itself comes from the world's 1,024-by-1,024 pointer table at
world `+0xBC`, indexed first by the first coordinate.

If the cell resolves, its live descriptor at `+0x2C` receives the
record descriptor. The native assignment copies nine dwords at relative
`+0x00`, `+0x04`, `+0x08`, `+0x0C`, `+0x10`, `+0x18`,
`+0x1C`, `+0x20`, and `+0x24`, and the separate bytes at
`+0x14` and `+0x15`. This assignment preserves the two destination
padding bytes at `+0x16` and `+0x17`.

The frame then separately divides both signed record coordinates by
eight with truncation toward zero and looks up that region. On success
it sets region byte `+0x08` to zero. The branch reaches the region
lookup even when the direct cell lookup failed. For example, an axis
of minus one fails the cell bounds and gives zero for that axis of the
region lookup. The region lookup requires both resulting axes in 0
through 1,023. Neither lookup allocates missing cells or regions.

Evidence at C `0x08FE11–0x08FE89`, cell lookup
`0x087DA0–0x087E22`, region lookup `0x034A90–0x034AC3`, and
descriptor assignment `0x068620–0x06866B`.

The region byte is also read in the frame's later region-request work.
A present region with a nonzero byte can cause a coordinate pair to
be appended to controller `+0x2D4`. The connected sender snapshots
that queue and uses its pairs for packet 12. That send phase requires
a signed elapsed time greater than 1,000 milliseconds since its stored
sample. Thus the receipt write can suppress that later request for
the same region while the byte remains zero. Evidence at C
`0x09D121–0x09D130`, `0x09D1C6–0x09D215`,
`0x06A234–0x06A26F`, and `0x06A2E4–0x06A327`.

After the two hit traversals the frame appends copies of the whole
received objective list to aggregate `+0x60` in order and then clears
its controller queue. The copy includes records whose early cell or
region lookup failed. The later aggregate pass, beginning at
`0x090651`, uses the copied records alongside locally generated ones.

- A zero kind dword at record `+0x18` skips that later effect work.
  With nonzero kind and state one, the identity at record `+0x08`
  is looked up as an actor. A present actor is passed to
  `0x0882E0` with argument one.
- With nonzero kind and state two, the frame again resolves the
  objective cell. A present cell reaches interface calls using the
  native strings `shine` and `objective`. The latter is supplied
  with a value formatted from the resolved cell. It then invokes
  sound helper `0x084350` with selector 30 and two binary32 values
  of 1.0.

Evidence at C `0x09056A–0x09064C`, `0x057C70`, `0x054810`,
`0x090651–0x090815`, `0x077FA0`, and strings at
`0x3018F8` and `0x301864`.

### Local and per-peer producers

- In the C and S world routines' mode-zero preparatory actor path,
  a present cell with a nonzero kind at cell `+0x34` and zero state
  at `+0x41` can produce an objective event. The separate spatial
  helper's result must be ordered greater than zero. The path writes
  state one, packages the coordinate pair and current descriptor,
  and appends the new event to result `+0x60`. The identity field
  in this event begins at zero. C evidence is `0x20CB1B–0x20CB22`,
  `0x20CC3D–0x20CCE0`, and constructor `0x065C70`. S evidence is
  `0x1328DB–0x1328E2`, `0x1329FD–0x132AA0`, and constructor
  `0x022D00`.

- The admitted objective-progress branch of the native hit receiver is
  another producer. At the descriptor-kind router, kinds 7 through 12
  increment the dword at cell `+0x44` and write state one. If the
  resulting signed count meets or exceeds cell `+0x48`, they write
  state two. Kinds 1 through 5 and 13 go directly to the state-two
  path. Kind 6 leaves this branch. The changed descriptor is packaged
  with the cell coordinates and appended to result `+0x60`.
  These are results of the admitted progress branch. Earlier native
  gates include cell presence, current state other than two and
  selection of a qualifying actor from the hit's surrounding state.
  Evidence at C `0x196404–0x196579`, tables
  `0x196C78` and `0x196C84`, and S `0x0CF424–0x0CF599`, tables
  `0x0CFC98` and `0x0CFCA4`.

- During S result handoff, result `+0x60` is copied to each visited
  peer's `+0x78`. S also produces objective snapshots directly for a
  requesting peer. Packet 12 supplies an eight-byte signed region pair
  to that peer's list at `+0x88` under its `+0x98` critical section.
  The handoff copies and clears the request list and visits the region
  pairs. For each pair it traverses the 8-by-8 objective cells with the
  second axis advancing inside the first. Each base coordinate comes
  from shifting its request word left six bits, retaining the low 32
  bits, then signed division by eight truncated toward zero. The inner
  offset of zero through seven is added before the cell bounds and
  presence checks. For each resolved cell, the handoff appends the
  coordinate pair, zero identity, and the stored descriptor directly to
  the requesting peer's `+0x78`. The snapshot covers resolved cells
  with zero kind as well.
  Evidence at S dispatch table `0x02671C`, `0x026659–0x0266D7`,
  `0x02780A–0x02781E`, `0x027823–0x02790D`, and
  `0x027CAE–0x027ED1`.

Other S world helpers have confirmed result `+0x60` append sites at
`0x10D1A0` and `0x10DC01`. The latter follows clearing the cell's kind
and progress dwords on its admitted path. The wider generation rules
feeding those helpers, the full semantics of each descriptor field,
and the effects inside the state-one actor helper remain open.

## Dedicated packet 8 and packet 4 section 12

### Received records

The native connected client emits packet 8 as the four-byte type value
followed by a 40-byte buff event. Its frame queues outgoing buff events
whose outer source matches the local actor. The dedicated decoder's
packet-type table selects the 40-byte intake at S `0x0264E0` for type
8. On a completed read the worker appends a copy of all 40 bytes to
backend `+0x3C` under the world hold. This includes the outer source
and target identities in the first 16 bytes.

The dedicated wrapper drains this backend list into a fresh optional
input aggregate `+0x58` before the world call, and constructs a separate
fresh result aggregate. In the world, the optional buff pass uses the
outer target at record `+0x08` for lookup. For a resolved target it
supplies only the 24-byte resident payload at record `+0x10` to the
actor's resident applicator. The applicator replaces or appends resident
state at actor `+0x1178`. Its stored bytes include the identity inside
the resident payload, which is distinct from the event's outer source.

On this intake path the outer source stays in the input event. The
world's input traversal has no copy from that event to the result
list. The mode-zero dedicated path retains the current result buffs,
and the other mode's nonempty-input tail clears result `+0x58`.
The caller destroys the optional input after the iteration.

Evidence at C `0x09C131–0x09C165`, `0x069D02–0x069D14`,
and `0x06A153–0x06A1B1`. S evidence is `0x0260C9–0x0260D8`,
table `0x02671C`, capture sites `0x025DB7–0x025E88`,
`0x0264E0–0x026549`, append and copy `0x011040` and `0x020080`,
drain `0x028170–0x0281CD`, wrapper `0x14A050–0x14A0AE` and
`0x14A257–0x14A26C`, optional processing
`0x132B92–0x132C6A`, and applicator `0x011740–0x0117F7`.

### Generated records and source identity

The dedicated fanout copies result `+0x58` to peer `+0x70`. The
sender snapshots that list, clears the peer nodes, and serializes its
copy as packet 4 section 12. The section has a four-byte count and
40 bytes per record. It emits the stored outer source bytes in each
record directly. This chain carries entries from the result aggregate.
The packet 8 input list has no direct relay into this chain.

An independently generated effect can carry the same outer source
value. For example, the dedicated hit receiver's admitted positive
type-five path visits the target's current type-six residents, reduces
their binary32 amounts by the hit amount, and clears amount and time
when the reduced amount is ordered at most zero. It makes a new
buff result using the triggering hit's source and target for the
two outer identities and the updated resident as its payload.
A previously received resident can thus contribute to a later
result whose outer source equals the old packet 8 outer source
when the triggering hit independently supplies that same identity.
The earlier packet 8 source bytes do not participate in that copy.

The dedicated optional hit pass precedes the optional buff pass.
That ordering puts an input resident already installed in a prior
iteration within reach of an admitted later received hit. The
hit receiver's dedicated player-target route can instead append
the hit and return early when world `+0xB8` is null and target byte
`+0x60` is zero. The example of a generated resident update requires
a target and hit that reach the type-five body.

Evidence at S `0x0277DC–0x0277F0`, `0x024907–0x024911`,
`0x0234D6–0x0234E1`, `0x024B5D–0x024B83`, `0x024F7D–0x024F8B`,
and serializer `0x0219C0–0x021A24`. The generated example is at
S `0x132AEE–0x132B92`, `0x0CEAB6–0x0CEB38`,
`0x0CEEF2–0x0CEF1F`, and `0x0CEF25–0x0CEFD1`.

## Late client type-six hit transition

In the first received-hit traversal, a type-six record whose target
resolves to the local actor takes the ordinary local health-admission
path. It bypasses the type-one and type-five branches and reaches the
health test, and then the applicable stun and health changes. The
later received-hit copy checks the source identity. A nonlocal-source
type-six hit can reach the retained aggregate this way. Evidence at C
`0x08FF26–0x08FF7F`, `0x090081–0x090083`,
`0x0901D0–0x09030A`, and `0x090403–0x0904AE`.

The later aggregate-hit pass at C `0x094971` resolves the target
before reaching a separate special branch. An unresolved target
advances the hit iterator. For a continuing record, the branch
checks the target identity against the local actor, the hit selector
for six, and the local actor's byte `+0x1020` for `0x14`. It then
looks up the hit source and scans that actor's live resident list at
`+0x1178`. The first qualifying resident has type seven, ordered
binary32 amount at least 1.0, and the eight-byte identity at resident
`+0x10` matching the local actor. The choice belongs to this late
pass, after received buff applications. It is reached independently
of the positive-amount feedback check earlier in the late traversal.

When those gates succeed, the following local actor writes occur
before the ordinary late continuation.

1. Apply a default item record at actor `+0x1020`, then clear
   action byte `+0x68`.
2. Copy the source actor's eight-byte identity at `+0x08` to the
   local actor's `+0x11C8`.
3. Write `0x13` at local `+0x1020` and the source byte `+0x64` at
   local `+0x1021`.
4. Copy the source word at `+0x190` to local `+0x1030`, and the
   source dword at `+0x194` to local `+0x1024`.

This is a gated actor and item mutation in the late hit pass. The
predicate is evaluated for each retained hit whose target lookup
and intervening common work complete. A type-six record reaches
the mutation only with the specified target, item and source-resident
conditions. The source lookup at the special site has no explicit
null branch before the resident access. The state available in
unresolved-source cases and any exceptional outcome there remain open.

Evidence at C `0x094971–0x094A31`, `0x094A79–0x094B75`,
`0x094B7A–0x094C00`, and default item helper `0x02F3E0–0x02F429`.

## Remaining coverage limits

The contracts close the described copies, mutations, ownership and
ordering for the supported paths. Full objective generation rules,
later interface rendering, complete hit effects, exceptional network
outcomes and the scheduling of delivery remain outside this evidence.
