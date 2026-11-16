Status: FINAL / STABLE / HELD

# Alpha rulings: hit knock-back rescale, taming success, pending-charge finalisation

Scope: three disagreements between held contracts, settled from the original images
only. Supported images: client `binary/Cube.exe`, 3,885,568 bytes, SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, image base
`0x00400000`; server `binary/Server.exe`, 1,718,784 bytes, SHA-256
`1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`, image base
`0x00400000`. Evidence was read with `dumpbin /disasm` on the original image bytes and
by reading binary32 constants directly from the client image. Addresses are RVAs unless
stated as object or record offsets; some held contracts cite the same places as virtual
addresses (RVA + `0x400000`), for example hit resolution `0x596D30` = RVA `0x196D30`.
Creature-object offsets equal record offsets plus `0x10`. No reconstruction source,
reconstruction listing or decompiler output was consulted. Existing contracts are not
edited by this ruling.

## Ruling A: hit resolution never changes the target velocity at step 12

Context: the hit-resolution routine (client RVA `0x196D30`), the step after the flash /
impact-sound stage and before the main hit record is formed. Every path that reaches it
(including the attackerless path) runs the same block.

Native behaviour, in order:

1. The target velocity (object `+0x34/+0x38/+0x3C`, binary32 x, y, z) gives a squared
   length computed as (x·x + y·y) + z·z in binary32.
2. The knock-back vector (the pointer argument, three binary32) gives a squared length
   computed the same way, (kx·kx + ky·ky) + kz·kz.
3. A working maximum starts as the velocity value. It is replaced by the knock-back value
   only when the knock-back value is ordered strictly greater; an equal or unordered
   comparison keeps the velocity value.
4. The velocity squared length is computed a second time from the same three fields with
   the same multiplications and additions in the same order. Nothing writes the velocity
   between the two computations, so the second result is bit-identical to the first.
5. The velocity would be rescaled (normalised, then multiplied by the square root of the
   working maximum, and written back to `+0x34/+0x38/+0x3C`) only when the second result
   is ordered strictly greater than the working maximum.

The working maximum is either the velocity value itself or a value strictly greater than
it, and an unordered velocity value fails the ordered test. The condition in step 5
therefore never holds for any input, including zero, infinite and NaN components. The
target velocity is never changed by this step, no square root is taken, and no
non-number can be produced here. The rescale body is unreachable.

Evidence (client): first squared lengths and maximum `0x197E5E-0x197EB4`; ordered
compare `0x197EAC` with below-or-equal branch `0x197EAF`; recomputation
`0x197EBA-0x197ED9`; compare `0x197EDD`, below-or-equal/unordered branch `0x197EE0` to
`0x197FA8`; unreachable rescale body `0x197EE6-0x197FA3` (square-root call `0x197F15`,
stores `0x197F7E-0x197FA3`). Server: identical sequence `0x0D0E7E-0x0D0F00`, branch to
`0x0D0FC8`.

Verdict: `alpha-attack-routine.md` M6 is right. `alpha-creatures-behaviour.md` §5 step 12
is wrong: its condition is reversed ("|v|² < m²") and the rescale it describes, together
with the "zero v produces a non-number" remark, does not occur. Step 12 has no observable
effect.

## Ruling B: taming succeeds in the action-110 effect; the combo-5 branch pacifies

Two places are involved.

### B.1 The in-range branch of the creature AI (client RVA `0x02D5BE-0x02D734`)

Taken when: the kind-8-present indicator is false; the target's action is 82 (`0x52`);
the target's Pet-slot item type (target object `+0x1020`) is 20 and its subtype (target
object `+0x1021`) equals this creature's type (object `+0x64`); this creature is not a
controller-5 creature with a nonzero owner identifier (object `+0x198/+0x19C`); and the
squared separation is not greater than the squared reach (an ordered "below" outcome
skips the branch).

Then, in order: the target identifier is stored at object `+0x11D0/+0x11D4`. If the
action time (object `+0x6C`) is not strictly greater than the value of duration helper
RVA `0x03D1A0` (described by the held contracts as the current action's total
duration; not re-derived here), the branch ends. If the action (object `+0x68`) is not
110 (`0x6E`), the counter at object `+0x70` (record `+0x60`, the combo counter) is
cleared. If that counter is less than 5, the action becomes 110 with time 0 and the branch ends.

Otherwise (counter at least 5), no pet is created. Helper RVA `0x02C520` only clears a
24-byte buffer's byte `+0`, dwords `+4`, `+8`, `+0x10`, `+0x14`; it writes nothing else
and has no other effect. The creature's buff list (object `+0x1178`) is walked in order
and each kind-7 entry's 24 bytes are copied over the buffer (the last one wins). If one
was found, the buffer's value (`+4`) and duration (`+8`) are zeroed and it is applied
through the buff routine (client RVA `0x1957C0`). The buffer is then rewritten as kind 8,
value 0, duration 20,000 ms (`0x4E20`), source = the target's identifier at
`+0x10/+0x14` (other bytes keep the earlier buffer contents) and applied again. The
creature is thereby pacified for 20 s.

### B.2 The action-110 effect (client RVA `0x216DE1-0x217075`)

Runs only when the world is offline/authoritative (world byte `+0xB4` zero) and only on
the step at which the action time crosses P1 (time below P1 and time plus step at least
P1). The creature remembered at object `+0x11D0` must exist and still hold type-20 food
whose subtype is this creature's type; otherwise nothing below happens. Then, in order:
one draw for the pitch of sound 44; a hit record of kind 6 (record `+0x45` = 6) naming this
creature and the target is emitted; the progress is the value of the last kind-7 buff met
in the list walk (0 if none) plus 0.2 (`0x3E4CCCCD`); the counter at object `+0x70` is
incremented and object `+0x74` cleared. If the progress is ordered greater than or equal
to 1.0, the creature's controller byte (object `+0x60`) becomes 5, the progress is set to
exactly 1.0 and the owner-binding routine (client RVA `0x1EE7A0`) is called with the
target's identifier and the creature. That routine installs a new behaviour object at
object `+0x13E4` and, when the owner is found, stores the owner's identifier at object
`+0x198/+0x19C` and the creature's identifier as the owner's pet id (owner object
`+0x11C8/+0x11CC`), among further stat adjustments not restated here. Finally a kind-7
buff is applied with value = progress, duration 5,000 ms (`0x1388`), source = the
creature's owner field (object `+0x198/+0x19C`).

So five consecutive bites within the 5-s buff window tame the creature at the fifth bite
(0.2 accumulated five times reaches at least 1.0 in binary32). After success the creature
is an owned controller-5 creature and fails the B.1 gate, so the combo-5 outcome of B.1 is
reached only when five bites have been counted without the progress reaching 1.0 (for
example when the kind-7 buff expired between bites); that outcome is the pacify/failure
path.

Evidence (client): B.1 gate `0x02D5BE-0x02D60E`, target store `0x02D614-0x02D622`, finish
test `0x02D628-0x02D630`, counter reset and action 110 `0x02D636-0x02D654`, counter-5
outcome `0x02D659-0x02D734`, helper `0x02C520-0x02C541`; B.2 gate and crossing
`0x216DE1-0x216E1C`, food test `0x216E22-0x216E51`, sound `0x216E71-0x216EB8`, kind-6 hit
`0x216EBD-0x216F23`, kind-7 walk `0x216F2E-0x216FBD`, increment and threshold
`0x216FBF-0x216FF2`, controller 5 `0x216FFB`, binding call `0x21700D`, kind-7 buff
`0x217012-0x21706A`; owner binding `0x1EE93C` (behaviour), `0x1EE9C5-0x1EE9D1` (owner
id), `0x1EEA10-0x1EEA19` (owner's pet id). Constants: `0x2FCDBC` = 0.2, `0x345DC0` = 1.0.
Server: counter-5 outcome `0x003A79-0x003B54` (helper `0x0029B0`, buff routine
`0x0CE9F0`), action-110 effect from `0x13CBA1` (controller 5 at `0x13CDBB`, 5,000 ms at
`0x13CDF8`).

Verdict: `alpha-created-actor-combat.md` §6.2 and `alpha-pets-bombs.md` §7.3 (with its
§18 correction) are right. `alpha-creatures-behaviour.md` §11.2 step 11, Taming, is wrong
where it says the creature becomes the target's pet at combo ≥ 5 via `0x42C520`; the
re-issue of the kind-7 buff at value and duration 0 and the 20,000-ms kind-8 buff it
lists are correct but constitute pacification, and only the last kind-7 entry is
re-issued, once.

## Ruling C: pending-charge finalisation runs on every route

The per-creature action processing in the world step (client RVA `0x2134F7` onward)
chooses one of five routes, and every one of them continues into the same finalisation
block:

* action 31 or 33 with time greater than P1: straight to the finalisation;
* action 31 or 33 with time at most P1 and no crossing (time plus step not greater than
  P1): to the finalisation;
* action 31 or 33 at the crossing: action power (object `+0x13B4`) := 0.1, mana (object
  `+0x170`) −= cost, floored at 0, then to the finalisation;
* any other action with time 0: the zero-clock start costs (object `+0x13C0` := 0,
  action power := 0, cost cases), then to the finalisation;
* any other action with nonzero time: straight to the finalisation.

The finalisation, in order: if the pending field (object `+0x144`, record `+0x134`,
"charge") is ordered strictly greater than +0.0, action power := pending, mana −=
pending, object `+0x13B8` := 0, and for action 11 or 5 action power := 0.5 × action power
(`0x345DA0` = 0.5). Unconditionally the pending field := +0.0. Then mana := +0.0 only when
+0.0 is ordered greater than mana. Then a temporary indicator (true for an ordered
positive action power, else true for action 11, else false) is formed.

Consequently a positive pending value is consumed on the next pass whatever the action
and clock value, including during actions 31 and 33 and mid-action.

Evidence (client): route selection `0x2134F7-0x213514`; zero-clock start
`0x21351A-0x213593` with nonpositive cases `0x21364B-0x21374F` returning to
`0x213593/0x213596`; nonzero clock `0x2137D9-0x2137DC` to `0x21359C`; charge route
`0x213754-0x2137D4` (time above P1 `0x213766` to `0x213593`; no crossing `0x213784` via
`0x2137E1-0x2137E4` to `0x2135A2`; crossing to `0x213596`); finalisation
`0x213593-0x213646`, indicator store `0x2137E9-0x2137F1`. Server: the same structure at
client RVA − `0xDA240` (route test `0x1392BA`, finalisation `0x139353-0x139406`, jumps
into it from `0x13950F`, `0x139526`, `0x139594`, `0x13959C`, `0x1395A4`).

Verdict: `alpha-common-action-processing-state.md` is right. `alpha-creatures-behaviour.md`
§3.5 step 1 is wrong in scope: its "Then, if charge > 0 … Charge := 0; mana is floored at
0" sentence states the correct effects but must be read as a separate step that runs after
whichever of step 1 (zero-clock start) or step 2 (deferred cost of 31 and 33) applies, and
also when neither applies.

## Unresolved

- The meaning and consumers of action power (object `+0x13B4`), of object `+0x13B8` and
  of object `+0x74` are not established here.
- Whether every creature reaches the action-processing block on every world step (earlier
  exits) is not examined.
- If the owner-binding routine does not find the owner, the creature becomes controller 5
  with no owner identifier; whether it can then re-enter the B.1 branch, and what follows,
  is not traced.
- The kind-7 list walk order in B.1 and B.2 follows the list's own ordering; which entry
  is "last" when several kind-7 buffs coexist is not established.
- `alpha-pets-bombs.md` cites the AI combat section as §13.2; in
  `alpha-creatures-behaviour.md` it is §11.2 (combat, RVA `0x02CB20`). Reference only.
- The online client ordering of the kind-6 hit and the buff records of B.2 is not examined.
- Full effects of the buff routine (RVA `0x1957C0`) and of the owner-binding routine
  beyond the fields named are not restated.

## Re-certification (2026-11-15)

Authored and read back in full from the two original images identified above (hashes
recomputed during this work). Conflict A settled in favour of alpha-attack-routine.md M6;
conflict B in favour of alpha-created-actor-combat.md §6.2 and alpha-pets-bombs.md §7.3;
conflict C in favour of alpha-common-action-processing-state.md. alpha-creatures-behaviour.md
§5 step 12, §11.2 step 11 (Taming) and §3.5 step 1 are wrong on the points stated and were
not edited. No reconstruction leads, implementation material, tests, captures or logs
were used.
