# Alpha client later projectile hit-call boundary

Status: FINAL / STABLE / HELD. This is an independently worded, bounded source-side contract for original client call boundaries. It does not certify a replacement implementation or runtime acceptance.

The evidence root is `disassembled alpha cube world`. The supported original image is `binary/Cube.exe`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, with preferred image base `0x00400000`. Addresses below are RVAs unless explicitly identified as field offsets. The audited sites are the later projectile-phase calls at `0x221E8C` and `0x222AB8` to the generic hit routine at `0x196D30`, with their immediate argument formation, visible draws, buff producers, and ownership/discriminator gates. Other calls to the same hit routine, full projectile physics, and the hit routine's complete behavior are outside this contract. No reconstruction, other contract, implementation, runtime capture, test, or implementation history supplies facts here. No server equivalence is claimed.

Both audited calls supply **full 32-bit selector value `0x00000000` in stack argument 12**. The separate kind-1 Boolean is stack argument 11. It must not be substituted for the selector.

## Stack numbering and selector evidence

Stack arguments here are numbered from 1, excluding the world pointer passed separately in the native object register and excluding the return address. Thus stack argument 12 occupies entry-stack offset `+0x30`, or frame offset `+0x34` after the hit routine establishes its frame. The callee removes 56 argument bytes on return: fourteen four-byte stack arguments. It directly compares the complete 32-bit value of argument 12 with 28 at `0x197C5A` and with 17 at `0x19804D`. Those are full-width comparisons, not byte comparisons.

| Caller site | Full-width selector at stack argument 12 | Distinct stack argument 11 |
| --- | --- | --- |
| `0x221E8C` | Literal positive integer zero, placed at `0x221E3F` | Zero-extended 32-bit 1 when projectile kind `+0x60` equals 1; zero otherwise. |
| `0x222AB8` | Literal positive integer zero, placed at `0x222A69` | Zero-extended 32-bit 1 when projectile kind `+0x60` equals 1; zero otherwise. |

Each caller also supplies full integer 1 as stack argument 14. It is placed before a pointer-returning helper at `0x02C6C0`. That helper uses its object pointer, returns the pointed-to native address plus 8, and does not consume a stack argument; the preceding integer 1 remains an argument of the later hit call. This detail is necessary to preserve the argument numbering.

The selector remains zero for every kind, element, flag, source action, and critical result that reaches either call. Consequently, it does not match the callee's selector-17 or selector-28 cases. This statement does not close any of the callee's other cases or effects.

The nearby critical argument is a separate stack slot: the callers establish its low byte and push a four-byte local value. The scoped byte writes do not establish normalized values for the other three bytes of that local. The second caller likewise passes a four-byte local whose low byte was copied from projectile flag `+0x5C`; the first caller explicitly zero-extends that projectile byte. These facts do not weaken the fully established 32-bit zero selector.

Sources: `binary/Cube.exe`, argument formation `0x221E28–0x221E8C` and `0x222A52–0x222AB8`, callee entry `0x196D30–0x196D7C`, full-width selector reads `0x197C5A` and `0x19804D`, return cleanup `0x1986F6–0x1986F9`; `decompiled/client/functions/0042c6c0.c`, `00596d30.c`; corresponding original ranges in `decompiled/client/assembly.txt`.

## Values and source identities at this boundary

Projectile kind at `+0x60` is a 32-bit discriminator. Element at `+0x64` and flag at `+0x5C` are one-byte discriminators. This contract uses their numeric values without assigning weapon, spell, or element names. Projectile `+0x48` is a binary32 input amount; `+0x54` is another binary32 quantity passed separately to the hit routine; `+0x68` is the signed 32-bit projectile clock. The position-like object beginning at projectile `+0x18` is passed to both hit calls, but its complete numeric representation and collision use are not reconstructed here.

The projectile's first eight bytes identify its source for native world lookup `0x02F000`. The surrounding code performs separate lookups: a cached-source result obtained in `0x221465–0x22148B` is used by the buff-producing and some ownership tests; a later source result obtained at `0x22156C` is reused as the hit call's source argument and the critical-chance source. These lookups address the same projectile identifier, but the contract preserves their separate observed uses instead of assuming an unproved lifetime or aliasing guarantee. “Cached source” and “hit source” below distinguish those results.

Both call paths pass their current candidate creature as stack argument 1 and the hit-source pointer, which may be null, as stack argument 2. Their input amount comes from projectile `+0x48`, doubled by binary32 multiplication when the caller's critical Boolean is true. The first path clamps an ordered negative doubled/base amount to positive zero before selecting its element-specific outcome. The second path has no corresponding caller clamp between its amount load and the generic hit call. The callee can perform additional checks and arithmetic.

World `+0xB4` is a one-byte mode discriminator and world `+0xB8` is a native 32-bit distinguished-creature pointer. Creature `+0x60` is a one-byte control discriminator. Creature health-like amount `+0x16C`, mana/resource `+0x170`, and critical-related amount `+0x1190` are binary32. Creature `+0x128` is a 32-bit lock value; creature class/specialization at `+0x140`/`+0x141` are one byte each. Resident buff-list pointer `+0x1178` is 32-bit.

The health comparisons described below reject ordered nonpositive health. An unordered comparison does not take that particular rejection branch; this observation does not establish any later non-finite-health behavior.

Sources: `decompiled/client/functions/0042f000.c`; `binary/Cube.exe`, source lookups `0x221465–0x22148B` and `0x221561–0x22157A`, amount formation `0x221B50–0x221B82`, first-path clamp `0x221CE5–0x221CFB`, second amount formation `0x2228AD–0x22290C`, and argument ranges above; matching original assembly.

## Chance helpers and probability scaling

The critical helper at `0x0444A0` performs one native random draw whenever called. A resident type-11 record makes its comparison threshold exactly binary32 1, but does not skip the draw. That presence test does not require positive remaining duration, magnitude, or a particular source identifier. Without type 11, the threshold is the binary32 result of the base critical-stat helper at `0x03EA40`, plus binary32 creature `+0x1190` multiplied by binary32 0.15. The base return is stored as binary32, and multiplication and addition use binary32 arithmetic. The helper does not clamp the threshold to the unit interval.

The draw is converted to binary32 and divided by exact binary32 32767. Success requires that normalized result be strictly less than the threshold. Equality fails. In particular, a draw of 32767 fails a threshold of exactly 1. An unordered comparison fails after consuming the draw. The critical helper does not consume the resident type-11 record. The complete base-stat and item formulas are left as dependencies; the inspected stat/item helpers use mathematical/stat calculations rather than a native chance draw.

The separate probability helper at `0x0441A0` also performs exactly one native random draw whenever called, even if its eventual probability is zero, negative, or fails a later caller condition. It first scales the caller-supplied binary32 probability according to the source's byte pair at `+0xAA8`/`+0xAA9`:

| Source byte pair | Probability multiplier before the draw |
| --- | --- |
| First byte 3 and second byte 0, 1, 2, or 12 | Binary32 0.5 |
| First byte 3 and second byte 8 or 10 | Binary32 0.3 |
| Every other pair | No multiplier; retain the caller probability. |

Scaling is one binary32 multiplication, stored before the draw. The normalized draw uses the same binary32 division by 32767 and strict comparison as above. There is no probability clamp, and equality or an unordered comparison fails. For the first projectile path the passed probability is binary32 0.15, bit pattern `0x3E19999A`; for the later impact path it is exactly binary32 0.25, bit pattern `0x3E800000`. The multiplier is equipment-pair dependent, not a class/specialization test. The caller's later flag/kind/element/class tests do not prevent an already eligible call to this probability helper.

Sources: `decompiled/client/functions/004444a0.c`, `004441a0.c`, `0043ea40.c`, `004c6ba0.c`; `binary/Cube.exe`, critical helper `0x0444A0–0x04451E`, probability helper `0x0441A0–0x044211`, call-site probability constants `0x221BC0` and `0x22293C`. Relevant image binary32 constants: RVA `0x2FCDB8` (0.15), `0x2FCDC0` (0.3), `0x345DA0` (0.5), `0x345DC0` (1), and `0x2FD338` (32767).

## First candidate path and call at `0x221E8C`

This path is reached after earlier projectile movement and clock processing. The nearest audited lifetime filter admits kind 2 to this region without the general post-increment 5000-clock limit; other kinds require the updated signed projectile clock to be at most 5000. Kind 2 also has an earlier source-exists and pre-increment clock-at-most-1200 filter. These do not establish all movement or lifetime rules.

For an otherwise admitted candidate, the immediate attempt timing admits flag-zero kinds 0, 1, and 4 directly. Kind 3 requires a change between the signed clock divided by 1000 and the previous signed clock, obtained by subtracting this update amount, divided by 1000. Kind 2 requires current clock strictly greater than 300 and a change between the analogous quotients divided by 100. These divisions truncate toward zero, and clock/subtraction arithmetic is native 32-bit. Other kind/flag combinations do not reach this first attempt through the audited timing tests. No conclusion about their later impact path follows from that exclusion.

The candidate must be nonnull and pass the immediate positive-health comparison and native geometric/visibility predicates. With a nonnull hit source, the normal candidate route also excludes that source itself, requires a true relationship-predicate result at `0x1988A0`, and excludes a candidate whose full 64-bit identifier equals the identifier stored at hit-source `+0x11C0`/`+0x11C4`. The kind-3, element-2 route bypasses these source/self/associated-identifier tests. Their gameplay meanings and the geometric helpers' complete behavior remain unresolved. A nonzero candidate `+0x128` prevents the first attempt before the random/probability sequence; with a hit source it also invokes native bookkeeping on that source's `+0x11B4`.

For an attempt that passes those tests, the visible draw/effect order is:

1. A resource-gain calculation is eligible only when projectile flag `+0x5C` is zero, kind is neither 1, 3, nor 4, and the hit source exists. Under the admitted kind tests this includes kind 0 and kind 2. It consumes two draws before critical chance: a magnitude draw, then a signed-remainder-by-8 draw that doubles the gain when the remainder is zero. The ownership test that may suppress the mana write occurs after both draws.
2. If the hit source exists, call the critical helper once and use its Boolean to double the projectile input amount. With no hit source the critical Boolean is false and this critical draw does not occur.
3. If the cached source passes the buff-producer ownership test below, call the probability helper once with binary32 0.15. Only after its successful result are projectile flag and source class/specialization checked. A successful producer applies its resident record and appends its buff event, then appends its separate kind-47 positional entry.
4. Clamp an ordered negative input amount to positive zero, then select the element-specific outcome. Only an element byte other than 2 reaches generic hit `0x221E8C`. The generic hit can still reject the attempt after these earlier draws and producer effects.

The resource magnitude calculation uses binary32 operations in this order: convert the first random integer, multiply it by 2, divide by 32767, subtract that result from 1, multiply by binary32 0.05, and add binary32 0.1. Kind 2 then multiplies by binary32 0.2. The current source-action full duration is converted to binary32, divided by 300, and multiplied by that magnitude. The second draw doubles the resulting gain on remainder zero. With world mode byte zero, the gain is added to hit-source mana. With a nonzero mode byte, that write requires hit source equal to world `+0xB8`; otherwise both draws have still been consumed. The sum is stored and only an ordered value greater than 1 is capped to binary32 1. Full source-action duration and resource semantics remain dependencies.

The first buff producer requires a nonnull cached source and either equality with world `+0xB8`, or both world mode byte zero and cached-source control byte nonzero. This ownership test precedes the 0.15 draw. Following a successful draw, it requires projectile flag `+0x5C` equal to zero, cached-source class byte 2, and specialization byte 1. Therefore a role-eligible source with a different class, specialization, or flag still consumes that probability draw. No element test precedes this producer; the later element-2 outcome does not undo it.

The produced resident record has type byte 10 and signed 32-bit remaining amount 30000. Its binary32 magnitude and embedded 64-bit source identifier are zero from native initialization. It is applied to the cached source before the event is appended. The buff event's two outer 64-bit identifiers are both copied from that cached source's creature identifier. Consequently, it is not an event assigning this buff to the collision candidate. The subsequent positional entry has numeric kind 47, position copied from the cached source, and the constructor's binary32 values 1 for its two trailing initialized quantities. No additional chance draw appears in this visible producer sequence.

Element 2 bypasses this generic hit call. Its separate route additionally requires a cached source, a false result from relationship helper `0x196CA0`, and candidate control byte other than 6 before constructing its own event. Its detailed application behavior is outside the hit-call contract. The earlier critical/probability draws and possible type-10 producer have already occurred by this element decision.

Sources: `binary/Cube.exe`, earlier kind-2 filters `0x220AE2–0x220B11`, post-increment lifetime gate `0x220CBB–0x220CE3`, candidate/source gates `0x2215E0–0x221659`, geometric/lock tail `0x2218C1–0x22196C`, attempt timing `0x221971–0x221A1F`, resource draws and ownership `0x221A3E–0x221B50`, critical `0x221B50–0x221B82`, producer gate and mutation `0x221B8A–0x221CDF`, clamp/element split `0x221CE5–0x221E28`, and call `0x221E8C`; `decompiled/client/functions/0042c520.c`, `00446af0.c`, `0043b660.c`, `004460a0.c`, `00465fb0.c`, `00596ca0.c`; matching original assembly. Resource constants are binary32 at image RVAs `0x2FCDB4` (0.05), `0x2FCB7C` (0.1), `0x2FCDBC` (0.2), `0x345E10` (2), and `0x2FCE68` (300).

## Later impact path and call at `0x222AB8`

This is a later candidate scan after the first path and additional collision work. Kind 3 skips this impact route earlier, and kind 2 takes a separate outcome before its entry. For the remaining kinds, entry requires a nonzero earlier candidate-contact state or a nonzero later collision state. The complete geometric/terrain production of those states is not reconstructed here. The kind/flag test at this impact entry admits kind 1 regardless of flag; every other kind reaching it requires projectile flag `+0x5C` nonzero.

If the earlier candidate-contact state is zero, entry was supplied by the other collision state and the caller consumes a random draw for a positional entry before scanning candidates. Its binary32 trailing value is the random integer converted to binary32, multiplied by binary32 0.4, divided by 32767, then added to 1. Numeric kind is 39 for projectile kind 1 with element other than 2, 42 for kind 1 with element 2, and 20 otherwise. If the earlier contact state is nonzero, this draw and positional entry are skipped. This earlier contact state is not proof that the generic hit returned success.

A candidate must be nonnull. If the cached source exists, world mode is zero, its control byte is zero, and it differs from world `+0xB8`, this route leaves the candidate scan before critical or producer draws. A null cached source does not trigger that ownership exclusion. With a nonnull hit source, the route also rejects the source itself, requires a true `0x1988A0` relationship result, excludes the 64-bit identifier stored at hit-source `+0x11C0`/`+0x11C4`, and performs the positive-health comparison before geometry. With a null hit source those particular source-dependent tests, including that caller health test, are skipped; the generic hit routine still has its own health gate. Geometric and visibility helpers must succeed in either case.

A nonzero candidate lock at `+0x128` combined with a nonnull hit source invokes the source's native `+0x11B4` bookkeeping and skips this candidate before the chance sequence. A null hit source does not cause that lock check to reject the later impact candidate. This differs from the first candidate path's immediate lock exclusion.

For a later-impact candidate that reaches chance processing, the visible order is:

1. If hit source exists, call the critical helper once. Otherwise establish a false critical byte without that draw. Double the projectile input amount when the Boolean is true.
2. If cached source passes the same buff-producer ownership rule described for the first path, call the probability helper with exact binary32 0.25. The ownership check is before the draw.
3. Only after a successful probability result require the copied projectile flag byte to be zero, kind to equal 1, and element byte to equal 1. If these hold, apply resident type 9 to the cached source, append its buff event, then append the kind-47 positional entry.
4. Call generic hit `0x222AB8`, with full-width selector zero. Neither producer failure nor an element other than 1 prevents this generic call through these immediate tests. In particular, element 2 is not excluded here as it is at the first generic hit site.

The kind/flag restrictions imply that this impact producer can succeed only for kind 1 with flag zero and element 1. Nevertheless, other admitted kind/flag/element combinations can consume the 0.25 draw before being rejected by its later producer-only tests. This path has no first-path resource-gain draw pair between its candidate gates and critical chance.

The type-9 record has the same initialized numeric payload shape as the first producer: type 9, signed 32-bit remaining amount 30000, zero binary32 magnitude, and zero embedded 64-bit source identifier. It is applied to cached source, followed by an event with both outer 64-bit identifiers equal to that source, followed by the same kind-47 positional entry. Its production occurs before the generic hit call and does not require the call to succeed.

Sources: `binary/Cube.exe`, kind-3 skip `0x221F1E–0x221F24`, kind-2 separate outcome and impact entry `0x222345–0x222467`, positional-entry draw `0x22246F–0x2224DD`, copied flag `0x2224E2–0x2224EB`, candidate ownership/source tests `0x222553–0x2225E7`, geometric/lock tail `0x222804–0x2228A8`, critical and producer `0x2228AD–0x222A4D`, call formation `0x222A52–0x222AB8`; original `decompiled/client/assembly.txt` and the producer/helper files cited above. The binary32 pitch-related multiplier 0.4 is at image RVA `0x2FD318`.

## Resident application, immediate callee gates, and limits

The resident setter at `0x046AF0` treats types 9 and 10 as replaceable by type: it overwrites the first matching resident record's full 24-byte representation in place, or appends a new record at the list tail when no match exists. The producers therefore replace an earlier record's embedded source identifier with the fresh record's zero identifier rather than preserving it. Other duplicates are not removed by this setter. The event append at `0x0460A0` places the corresponding 40-byte event at its list tail. Record padding not initialized by `0x02C520` is not assigned semantic values here. General expiry, packet serialization, source lookup lifetime, and replication ownership are outside this document.

At generic hit entry, an ordered health value at candidate `+0x16C` less than or equal to zero returns a false low result byte. A nonzero world mode byte together with candidate equal to world `+0xB8` also returns false before the later hit processing. With a nonnull hit source whose current action byte is 91 or 74, candidate flag byte `+0x5C` must have bit `0x01` set to avoid another immediate false return. These callee checks do not roll back caller draws or buff events. The source-null path and all further callee admissions, mitigation, protection, damage, randomness and event effects remain unclosed.

Both callers combine a nonzero returned low byte into their surrounding success state after the call. The selector is zero before this result is available. A producer buff or positional entry, a consumed draw, an admitted candidate, or the earlier contact state is not itself evidence of a successful generic hit.

Draw order in this contract is per admitted candidate and per impact-entry operation. The outer code can scan multiple candidates and movement subdivisions, and can reach the first scan before a later impact scan. It must not be assumed that the listed draws occur only once per projectile, or that a later rejection refunds or skips prior draws. Exact collection iteration order, deduplication, complete collision predicates, broader projectile construction/lifetime, complete item/base-critical statistics, and full generic-hit behavior are explicit unresolved dependencies.

Sources: `decompiled/client/functions/0042c520.c`, `00446af0.c`, `004460a0.c`, `00451c70.c`, `00451d00.c`, `00596d30.c`; `binary/Cube.exe`, resident initialization `0x02C520`, application `0x046AF0`, event append `0x0460A0`, 40-byte event copy `0x051C70`, 24-byte resident copy `0x051D00`, generic hit entry `0x196D73–0x196DD1` and false return `0x197014–0x197016`, caller result accumulation `0x221E91–0x221EA4` and `0x222ABD–0x222AD0`; matching original assembly.
