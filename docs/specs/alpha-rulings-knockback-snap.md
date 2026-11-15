Status: FINAL / STABLE / HELD

# Alpha rulings: kind-1 hit vector and creature-receive snap health test

Scope: two disagreements between held contracts, settled from the original client
image only. Supported image: `binary/Cube.exe`, 3,885,568 bytes, SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, image base
`0x00400000`. Addresses are RVAs unless stated as object or record offsets. Evidence was
read with `dumpbin /disasm` on the original image bytes; the reconstruction's
`decompiled/client/assembly.txt` was consulted as a lead and agrees. Existing contracts
are not edited by this ruling. Creature-object offsets equal record offsets plus `0x10`
(the creature record begins at object `+0x10`).

## Ruling 1: kind-1 hit vector is assigned, not added

For an incoming hit whose target resolves to the controlled creature and whose kind byte
(hit `+0x45`) is exactly 1:

1. If the hit amount (binary32, hit `+0x10`) is ordered strictly greater than +0.0, the
   three 32-bit words at hit `+0x38/+0x3C/+0x40` are copied into controlled-creature
   object `+0x34/+0x38/+0x3C` (record `+0x24`, the velocity), and then object
   `+0x1184` is set to binary32 1.0. Zero, negative and NaN amounts skip both effects
   (the comparison is an ordered compare whose below-or-equal/unordered outcome skips).
2. Unconditionally for kind 1, the same three words are copied into the same three
   destination words again, before the resource operations of the held hit-mutations
   contract.

The copy routine moves three 32-bit words verbatim: no addition, scaling, conversion or
rounding, so the destination equals the received bit patterns. The two copies are
idempotent; the final velocity is the received vector regardless of the amount. Nothing
in the incoming-hit walk writes object `+0x4C` or reads the hit vector anywhere else
(only these two copy sites reference hit `+0x38` in `0x08FECB-0x090350`). Kind 3 jumps
past the conditional copy and kind values other than 1 skip the unconditional copy.

Evidence: kind tests and conditional copy `0x08FF3D-0x08FF6D` (destination formed as
controlled pointer plus `0x34`, call at `0x08FF62`, `+0x1184` store at `0x08FF6D`);
unconditional copy `0x08FF7A-0x08FF92` (call at `0x08FF92`); word-copy routine
`0x011E30-0x011E49`.

Verdict: `alpha-client-incoming-hit-mutations.md` is right ("These operations are
assignments."). `alpha-net-packets.md` §5.1 is wrong where it says "knock-back vector,
added to the target's velocity" and "the knock-back is added once more"; it should read
that the vector is stored into the velocity (twice, idempotently, the first time only
for a positive amount).

## Ruling 2: snap tests the creature's old health

In both creature-receive paths, the health test reads object `+0x16C` (record `+0x15C`)
of the live creature while the received data is still held only in a separate record
(message type 0: the temporary record on the stack; message type 1: the local map
record). The received record is copied into the creature only afterward. The tested
value is therefore the health before the record is applied.

The test is: snap when binary32 +0.0 is greater than or equal to the old health, using
an ordered compare whose "above-or-equal" outcome selects the snap. Zero, negative
zero and negative health snap; positive health does not; a NaN old health does not
snap (unordered falls through to smoothing). The test is reached only after the
"new creature" and snap-field (object `+0x1C0`, record `+0x1B0`) tests have not already
selected a snap.

Evidence: message type 0, new flag `0x06BCD1`, snap-field helper `0x06BCE5-0x06BCF2`,
health compare `0x06BCF4-0x06BCFE`, record copy into the creature at `0x06BDFF` (after
both branches). Message type 1, new flag `0x06C6A6`, inline snap-field word compare
`0x06C6C8-0x06C6F7`, health compare `0x06C6F9-0x06C703`, record copy into the creature
at `0x06C95C-0x06C96C` (after both branches).

Verdict: `alpha-remote-creature-interpolation.md` is right ("0.0 ≥ the creature's
current health `+0x16C` (the old value; a NaN health does not snap)"). `alpha-net-packets.md`
§2.3 ("when the new health is ≤ 0") and its correction-list item 4 ("health ≤ 0",
where meant as the received value) are wrong about which value is tested; the received
health is not consulted by this decision.

## Unresolved

- The meaning of object `+0x1184` and of the snap field `+0x1C0` is not established here.
- Whether any later frame stage adds to or otherwise transforms the velocity after the
  kind-1 store (outside the incoming-hit walk) is not examined.
- The initial health value of a newly constructed creature is not examined; it is not
  observable in this decision because a new creature always snaps first.
