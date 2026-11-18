# Alpha ruling on connected client receipt order

**FINAL / STABLE / HELD**

## Ruling, 2026-11-17

The received buff queue is visited at `0x08FCB8–0x08FD6A`, with resident
application at `0x08FD2F`. The range `0x09034A–0x090401` visits the
received projectile queue and inserts the qualifying projectiles at
`0x0903C4`. This corrects the address and phase attribution for received
buff application in the hit-consumption contract, section 5. The original
agrees with the projectile-receipt contract on these sites.
[Hit consumption, section 5](alpha-connected-hit-consumption.md) and
[projectile receipt](alpha-connected-projectile-receipt-step.md).

The resident buff pass completes before both traversals of the received hit
list. The first hit traversal handles local-target effects. Projectile
insertion follows it. The second hit traversal selects nonlocal-source
records for the frame aggregate. A later traversal of that aggregate can
read resident buff state again.

## Original evidence and objects

The supported original image is `binary/Cube.exe`, SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`,
with preferred image base `0x00400000`. All image addresses below are RVAs.
The frame routine is `0x088EE0`. The world is embedded at controller
`+0x2E4`, and controller `+0x8006D0` supplies the local actor pointer.
The local actor's identity occupies eight bytes at actor `+0x08`.
The caller's frame aggregate is the mutable object made at `0x08CE35`.

The receipt sites, branch targets, helpers and field accesses were checked
against original image bytes. The client instruction export supplied the
frame and helper evidence. Direct decoding of the original image supplied
the socket worker's staging sites where the export has gaps.

## Receipt staging and frame order

The socket receive worker appends decoded records to controller-owned
queues. In the scoped staging group it appends the hits at controller
`+0x800630`, the projectiles at `+0x800650`, and the buffs at
`+0x800688`, in that staging order amongst the other lists. The worker
takes the world critical section for the group. The frame also takes that
section before the world calls and holds it through the scoped receipt
work. Its normal release is later at `0x09D2EA`. The shared critical
section is at world `+0x8000C0`.

Evidence at `0x06CEE2–0x06CEE8`, `0x06CF17–0x06CF26`,
`0x06CF74–0x06CF83`, `0x06D04D–0x06D05C`, `0x06D080–0x06D088`,
`0x089020–0x08902F`, `0x09D2E4–0x09D2EA`, and the lock helpers
`0x201CB0` and `0x201E90`.

On normal flow through this part of the frame, the relevant order is as
follows. Traversals use each list's forward order.

| Phase | Source | Observable destination or effect | Frame evidence |
|---|---|---|---|
| Received buffs | Controller `+0x800688` | Apply to resolved target residents, then copy the entire received list into aggregate `+0x58` | `0x08FCB8–0x08FDBA` |
| Intervening receipt | Controller `+0x800690` | Visit its separate coordinate and object records | `0x08FDC3–0x08FEC1` |
| First received-hit pass | Controller `+0x800630` | Mutate the local actor on the local-target path | `0x08FECB–0x090344` |
| Received projectiles | Controller `+0x800650` | Append nonlocal-source copies to world `+0x14` | `0x09034A–0x090401` |
| Second received-hit pass | Controller `+0x800630` again | Append nonlocal-source hits to aggregate `+0x00` | `0x090403–0x0904AE` |
| Shared receipt cleanup | Controller lists | Reset received lists and release their nodes | `0x0905BD–0x09064C` |
| Later aggregate-hit pass | Frame aggregate `+0x00` | Visit retained hit records for later frame effects and selected actor state work | `0x094971–0x09529F` |

An empty buff queue goes directly to the intervening receipt at
`0x08FDC3`. An empty hit queue in the first pass goes to the projectile
entry at `0x09034A`. The populated hit path rejoins there at traversal end.
An empty projectile queue branches to `0x090409`, where the second hit
pass still has the hit-list address. A populated projectile queue exhausts
its traversal and reloads the hit-list address at `0x090403`. The second
hit pass therefore runs after the projectile stage in both cases. Its
empty exit and populated completion both continue at `0x0904B0`.

Evidence for these joins at `0x08FCCB`, `0x08FF05`, `0x090344`,
`0x09037F`, `0x090401–0x090409`, and `0x090436–0x0904B0`.

## Buff application and the two received-hit passes

Each received buff event contains an eight-byte source identity at
`+0x00`, an eight-byte target identity at `+0x08`, and a 24-byte
resident record at `+0x10`. The receipt loop resolves the target in the
world and calls the resident applicator only for a present target. The
outer admission uses target presence alone. The source identity, health,
elapsed value and duration are outside that condition.

The applicator uses actor list `+0x1178`. For resident types 1, 2, 3,
6, 7, 9, 10 and 11 it replaces the first matching type in place if present.
Replacement copies all 24 resident bytes. With no existing match, or
for another type, it appends a resident record. After all the target
lookups and applicator calls, the frame appends copies of all the
received events to aggregate `+0x58` in their original order, including
events whose target lookup failed. This copy leaves the controller
queue available for the later shared clear.

Evidence at `0x08FCB8–0x08FDBA`, lookup `0x02F000`, resident applicator
`0x046AF0–0x046BA7`, aggregate copy helpers `0x057A10`, `0x0544D0`,
and the 40-byte event copy at `0x051C70–0x051CB9`.

The first hit pass resolves the target identity at hit record `+0x08`.
The local-effect branch requires the resulting actor pointer to equal
controller `+0x8006D0`. Source identity is independent of that branch.
On the type-five path, selected by hit byte `+0x45`, it visits the
local actor's current resident list `+0x1178`. For each resident type six
it subtracts hit amount `+0x10` from resident binary32 amount `+0x04`.
An ordered result at most zero clears the resident amount and remaining
time. It also appends a buff event for that resident state into aggregate
`+0x58` with the local actor's identity in both identity fields. This
happens after the bulk copy of received buff events. The pass advances
over the hit nodes and keeps the list for the second traversal.

Evidence at `0x08FECB–0x08FF37`, `0x090081–0x0901CB`, the buff-event
append at `0x090182`, and traversal advance at `0x09030A–0x090344`.

The projectile pass compares its record's leading eight-byte source
identity with the local actor's. A different identity appends a copy
to controller `+0x2F8`, which is world collection `+0x14`. An
identical identity skips insertion. This path uses the identity comparison
alone for the ownership decision. The copy preserves the received signed
age at projectile `+0x68`. Since the frame's world pieces all precede
receipt insertion, the next opportunity for the world routine's projectile
motion and collision pass is a later admitted world invocation.

Evidence at `0x09034A–0x090401`, append helper `0x0861A0`, projectile
copy `0x051DA0`, `0x0595B0–0x05965B`, earlier world calls
`0x08CF2E` and `0x08CF49`, and the world projectile pass at
`0x220A70–0x220AB2`, `0x220C9D–0x220CDA`.

The second hit pass compares each hit's source at record `+0x00` with
the local actor's eight-byte identity. A different source appends a copy
to aggregate `+0x00`. An identical source skips that append. The source
comparison is the sole record selection in this traversal. A hit sourced elsewhere and targeting the local actor can
therefore qualify for both operations. A hit sourced locally can still
take its earlier local-target branch if the target lookup identifies the
local actor. Evidence at `0x090403–0x0904AE`, append helper
`0x086290`, and record copy at `0x051EE0–0x051F04`, `0x0599A0`.

The shared cleanup resets the received hit, projectile, and buff lists
at `0x0905C4`, `0x0905F4`, and `0x090641`. The reset helper restores
both sentinel links, stores count zero and releases the old nodes.
The inserted projectile and the copies in the aggregate have independent
lifetimes. Evidence at `0x08FEDB`, `0x0905BD–0x09064C` and
`0x06F870–0x06F8A3`.

## Buff visibility and later consumers

By the first received-hit pass, every entry then in the staged buff list
has had its target lookup, and every successful application has taken
effect. The type-five branch's resident type-six read is a direct
consumer of that state. The second received-hit pass follows those
applications and any resident changes already made by the first hit
pass. Its append decision uses source identity alone. Thus multiple
received buffs can replace an earlier resident record before either hit
traversal, and multiple local-target hits can see preceding hit updates.

The late aggregate-hit pass begins at `0x094971`. On one special
path it resolves the target and the hit source, requires the target's
identity to be local, requires hit type six and local actor byte
`+0x1020` equal to `0x14`, then searches the source actor's current
resident list. The match requires resident type seven, ordered binary32
amount at least 1.0, and the identity at resident `+0x10` matching the
local actor. The check uses live resident state at that late site. Earlier
world invocations in the same frame have already run when the received
buff queue starts applying.

The later outbound pass, when controller `+0x8006CC` is nonzero,
selects aggregate hits with a matching local source for controller
`+0x8006EC`, followed by the analogous buff selection for
`+0x8006F4`. These outgoing lists are distinct from the three
controller receipt queues. Evidence at `0x094971–0x094BF4`,
`0x09C02F–0x09C1A3`, and earlier world calls `0x08CF2E`,
`0x08CF49`.

## World-step admission and elapsed control

The frame's seed and name checks are upstream of the world calls. Either
failed comparison skips the world-call sequence and joins later frame
work at `0x08D5F0`, before the scoped receipt passes. The normal
post-world path also reaches that continuation. Once reached, the
buff, hit and projectile phases use the list and ownership conditions
above. Those list and ownership conditions govern the normal receipt
block for both world-call paths and for all admitted elapsed values. Evidence at `0x08CE9D–0x08CEC9`,
`0x08CF2E–0x08CF49`, `0x08D5E5–0x08D603`, and
`0x08FCB8–0x09064C`.

The frame reads the elapsed argument into a local field on entry. A
separate upstream branch tests controller byte `+0x8006E5`. When it
is nonzero, the frame divides the signed elapsed value by four with
truncation toward zero. If the quotient is below one, it stores one
and sleeps for five milliseconds. With the byte zero, the branch
bypasses the adjustment. The later world-call admission and subdivision
read this local value. This narrows the distinction between the display
driver's elapsed sample and the value the frame actually submits.
The constructor clears the flag along with the preceding byte.
Evidence at `0x088F31–0x088F34`, `0x059FF7`,
`0x08C104–0x08C134`, `0x08CE3E–0x08CE42`, and
`0x08CEF4–0x08CF49`.
[World-call phase boundary](alpha-world-call-phase-boundary.md).

The source evidence here establishes the world-step skip, the guarded
elapsed adjustment, and the receipt behavior for a frame that reaches
this block. The activation route and user-facing meaning of
controller byte `+0x8006E5`, the receipt schedule while frame updates
are suspended, socket delivery timing, exceptional exits, and complete
hit-effect bodies remain unresolved.
