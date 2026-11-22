# Original Alpha client projectile contacts and hit arguments

**FINAL / STABLE / HELD**

This contract covers the direct projectile contact at RVA `0x221E8C` and the later area contact at `0x222AB8`. Both call the world's hit routine at `0x196D30`. **Both pass full 32-bit zero in stack slot 12. Direct critical chance belongs to the later source lookup. Area critical chance belongs to the earlier saved lookup. Both hits name the later lookup as their source.**

The reference is the original 32-bit x86 client `binary/Cube.exe` under `disassembled alpha cube world`. The file is 3,885,568 bytes with SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred image base `0x00400000` and PE timestamp field `0x51EA955E`. Code and constant addresses in this document are relative to that base. Field offsets are relative to the named object. Evidence comprises the image, the corresponding ranges of `decompiled/client/assembly.txt` and individually identified original client function listings under `decompiled/client/functions`. This is a static contract for this image. Native runtime acceptance and server equivalence are open.

## The projectile, its actors and its clock

The projectile collection is at world `+0x14`. World `+0xB4` is a mode byte and `+0xB8` is a 32-bit distinguished-actor pointer. Each projectile starts with its eight-byte source ID. Actor ID is eight bytes at actor `+0x08`. The world lookup at `0x02F000` searches on both halves of the ID at `0x077D90`. A missing entry returns null. A match returns the actor pointer stored in the entry.

At `0x221465–0x22148B` the earlier saved pointer starts null. A lookup at `0x221476` tests presence. Only a nonnull result causes a second lookup at `0x221486`, whose result becomes the earlier saved pointer. At `0x22156C` another lookup runs regardless of the presence test. Its result becomes the later pointer at `0x221578–0x22157A` and is restored on the area route at `0x2222DF`. All three lookups use the same world and projectile ID. With a stable live mapping the two saved pointers coincide. With a consistently missing entry both are null. The observed uses below do not require an assumption about changes to the mapping or the lifetime of a returned pointer.

| Use | Direct scan | Area scan |
| --- | --- | --- |
| Ordinary source/self/associated-ID checks and lock bookkeeping | Later pointer | Later pointer |
| Source resource increment | Later pointer | No such increment in this scan |
| Critical check | Later pointer at `0x221B58–0x221B63` | Earlier pointer at `0x2228AD–0x2228E5` |
| Buff chance, resident buff, outer event IDs and cue origin | Earlier pointer | Earlier pointer |
| Hit helper's source argument | Later pointer at `0x221E85` | Later pointer at `0x222AB0` |
| Subtype-2 direct record | Earlier pointer gates the record. Its source ID comes from the projectile itself. | Subtype 2 can reach the ordinary area call. |

Projectile `+0x18` is a position of three signed 64-bit fixed-point components and `+0x3C` holds three binary32 velocity components. The binary32 contact amount is at `+0x48`, the direct radius at `+0x4C` and the separate hit quantity at `+0x54`. The flag at `+0x5C` and element/subtype at `+0x64` are independent bytes. The kind at `+0x60`, clock at `+0x68` and value at `+0x6C` occupy four bytes. Clock and `+0x6C` comparisons here are signed. Projectile selectors and actor fields have separate meanings even where their offsets coincide.

Kind 2 needs a present source on its earlier lookup at `0x220AE9` and a pre-increment clock no greater than 1,200. At `0x220CBB–0x220CDA` the update increments the clock using 32-bit arithmetic. Kind 2 bypasses the subsequent 5,000-unit limit. Other kinds reach this contact-processing region through this gate only when the updated signed clock is at most 5,000. Movement and other expiry decisions occur outside these gates.

## Candidate admission

The direct scan obtains the current candidate at `0x2215E0`. It skips a null pointer and rejects an ordered nonpositive binary32 health at candidate `+0x16C`. An unordered health comparison bypasses this particular rejection. With a later source it then excludes the source itself, requires the actor-pair predicate at `0x1988A0` and excludes an ID equal in both halves to the ID at later source `+0x11C0/+0x11C4`. A missing later source bypasses those three checks. Kind 3 with subtype 2 also bypasses them, after the health test.

The pair predicate rejects identical pointers first. Among distinct actors, either control byte at actor `+0x60` equal to 6 admits the pair, as does exactly one equal to 1. Two actors with control 1 can qualify through different low-byte classifications from `0x044680`. A set bit `0x20` at either actor's `+0x124` also admits an otherwise unmatched pair. Without an admitting condition the result is false.

Both scans compare three axes at candidate position `+0x10` with the binary32 dimensions at candidate `+0x80`. Each candidate extent is half the corresponding dimension. The projectile uses its `+0x4C` radius on all axes in the direct scan and exactly 5 on all axes in the area scan. Native offset helpers `0x012220` and `0x0122E0` scale the binary32 extent by 65,536, convert it to an integer and subtract or add it to the fixed-point component. On each axis the projectile upper bound may equal the candidate lower bound. The projectile lower bound must be strictly below the candidate upper bound. The comparisons at `0x194690` and `0x194660` distinguish these faces. After overlap, a nonzero result is required from the world segment test `0x19EE90`, directed from projectile position to candidate position with selector 1 and binary32 limit 200. The direct tests occupy `0x221659–0x221939` and the area tests `0x2225E7–0x222879`.

Candidate `+0x128` is a 32-bit field. A nonzero value diverts direct contact at `0x221945` before timing, draws and effects. If the later source exists the diversion also performs candidate-ID bookkeeping at its `+0x11B4`. In the area scan at `0x222885` both a nonzero field and a present later source are required for the bookkeeping and diversion. A candidate with a nonzero field can continue in the area scan if that source is null.

## Direct timing, chance order and subtype outcome

At `0x221971–0x221A1F`, flag-zero kinds 0, 1 and 4 qualify immediately. Kind 3 qualifies when current and prior clock have different signed quotients on division by 1,000. Kind 2 requires a current clock strictly greater than 300 and different quotients on division by 100. The prior clock is the current clock minus this update increment in 32-bit arithmetic. Both divisions truncate toward zero. The kind-2 and kind-3 timing tests impose no flag-zero condition. Other kind/flag combinations cannot make a direct attempt through these tests.

1. **Resource.** At `0x221A3E–0x221B46` a zero projectile flag, a present later source and a kind other than 1, 3 or 4 allow two draws. The admitted kinds for this operation are 0 and 2. Convert the first random integer to binary32, multiply by 2, divide by 32,767, subtract the result from 1, multiply by 0.05 and add 0.1. For kind 2 then multiply by 0.2. Convert the signed integer current-action duration from `0x03D1A0` on the later source to binary32, divide by 300 and multiply by the prepared magnitude. The second draw doubles that increment if its signed remainder on division by eight is zero. The individual floating operations use binary32 precision in this order. Both draws precede the write-permission test. Mode zero allows the write. In nonzero mode the later source must equal the distinguished actor. When permitted, add the increment to binary32 source `+0x170`, store it, and replace an ordered sum greater than 1 with exactly 1. There is no lower cap here.
2. **Critical.** The later source supplies one critical roll at `0x221B5E` if present. Its absence leaves the critical byte false and consumes no critical draw. Load the binary32 projectile amount at `+0x48` and multiply by 2 in binary32 on a true result.
3. **Source buff.** At `0x221B8A–0x221CDF` the earlier source is eligible if present and either distinguished or in mode zero with a nonzero control byte. An eligible source consumes one probability draw with base 0.15 before the producer checks the projectile flag and the source's class and specialization bytes. After a successful roll it requires flag zero, class 2 at earlier source `+0x140` and specialization 1 at `+0x141`. It then applies type 10, appends the buff event and appends cue 47 in the order specified below. A later selector failure does not undo the draw. This producer has no element restriction.
4. **Amount and outcome.** At `0x221CF3–0x221D07` an ordered negative amount becomes positive zero. Otherwise the prepared amount is retained, including negative zero at this comparison. Any subtype other than 2 takes the ordinary direct hit path at `0x221E8C`. The hit routine can still reject it after all preceding caller effects.

Subtype 2 takes the separate branch at `0x221D0D–0x221E22`. To create a record it requires the earlier source, a false result from `0x196CA0` for that source and candidate, and candidate control other than 6. The record gets the candidate's fixed-point position, the sign-negated prepared amount, and the direct critical byte. Earlier-source control 1 additionally multiplies that negated amount by 0.5. The source ID is copied from the projectile, and the other ID from the candidate. After the append at `0x221DCA`, mode zero allows local application at `0x195A60`. In nonzero mode local application requires the earlier source and the candidate both to equal the distinguished actor. The earlier resource, critical and buff decisions stand whether or not this branch makes a record. This route makes no direct generic hit call.

After either attempted direct outcome, `0x221EB0–0x221EBB` replaces signed candidate `+0x134` with signed projectile `+0x6C` only when the latter is greater. This update also follows a false ordinary-hit return and a subtype-2 attempt that made no record. A direct contact flag and an accumulated hit-success flag are separate. For example, the path at `0x221EC1–0x221F1E` can set the contact flag for kinds 0, 1 and 4 without a successful hit.

## Area entry and per-candidate order

Kind 3 skips this impact route at `0x221F24`. Kind 2 takes its separate substep outcome at `0x222345`. For other kinds, entry at `0x222457` requires either the direct-contact flag or the later collision flag. It further requires kind 1 or a nonzero projectile flag. A clear direct-contact flag means the later collision supplied entry. In that case `0x22246F–0x2224DD` appends a cue at the projectile position before the candidate scan. Its first trailing scalar is the random integer converted to binary32, multiplied by 0.4, divided by 32,767, then increased by 1, with binary32 arithmetic at each step. Its other initialized scalar remains 1. The cue is 42 for kind 1 with subtype 2, 39 for other kind-1 subtypes and 20 for other admitted kinds. A set direct-contact flag skips both this draw and the cue. The area caller then snapshots the projectile flag byte at `0x2224E2–0x2224EB`.

At `0x222553–0x222594` null candidates advance the iteration. For a nonnull candidate, a present earlier source with control zero which differs from the distinguished actor terminates the entire area scan in mode zero. A missing earlier source cannot trigger this cutoff. With a later source, the scan rejects the source itself, a false pair result, the matching associated ID, or ordered nonpositive candidate health, in that order. With a null later source all four checks are bypassed, including this caller health test. Geometry, segment and lock handling still apply. The hit routine has its own health check.

1. At `0x2228AD–0x222904` the **earlier** pointer supplies one critical roll if present. A null earlier pointer leaves a false critical byte and consumes no critical draw, regardless of the later pointer. A true result doubles the binary32 projectile amount. This scan has no direct-style resource pair or caller-side lower clamp on the amount.
2. At `0x22290C–0x22294A` the same earlier-source ownership rule as the direct producer permits one probability draw with base 0.25. After successful chance, the producer requires the snapshotted flag to equal zero, kind to equal 1 and subtype to equal 1, in that order. Class and specialization are immaterial to this producer. On success it applies type 9, appends its buff event and then cue 47. Other admitted combinations can still consume the probability draw.
3. The area call at `0x222AB8` receives the current candidate and the **later** pointer as source, including when it is null. It accepts the prepared signed binary32 amount. Producer failure or a subtype other than 1 does not prevent this call through the producer gates. Subtype 2 can reach this call.

## Chance arithmetic and the resident records

The critical helper `0x0444A0` searches the actor's resident list through the 32-bit pointer at `+0x1178`. A resident type 11 sets the threshold to binary32 1. This presence check tests neither duration, magnitude nor embedded source ID. It neither consumes the record nor skips the random draw. Otherwise the base critical statistic from `0x03EA40` is first stored as binary32. Multiply binary32 actor `+0x1190` by 0.15, then add that binary32 product to the stored base, in binary32. There is no threshold clamp.

The base statistic visits the following actor item offsets in order. On a matching first category byte it adds the contribution from `0x0C6BA0`. These stat and item calculations do not perform a chance draw. Their full numeric formulas are outside this contract.

| Actor item offset | `+0x990` | `+0xAA8` | `+0x530` | `+0x648` | `+0x760` | `+0x878` | `+0x418` | `+0xBC0` | `+0xCD8` |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Matching category | 3 | 3 | 4 | 6 | 5 | 7 | 8 | 9 | 9 |

The separate buff probability helper at `0x0441A0` scales the phase's supplied binary32 value. With equipment category 3 at source `+0xAA8`, subtype 0, 1, 2 or 12 at `+0xAA9` scales it by 0.5. Subtype 8 or 10 under category 3 scales it by 0.3. All other pairs retain the supplied value. Any scale is one binary32 multiplication stored before the draw. The category pair is independent of class and specialization. The base values written at `0x221BC0` and `0x22293C` have bit patterns `0x3E19999A` and `0x3E800000` respectively.

Each invocation of either chance helper consumes exactly one imported random draw, including for zero, negative or out-of-range thresholds. It converts the integer draw to binary32 and divides in binary32 by exactly 32,767. Success requires the normalized draw to be strictly below the threshold. Equality and unordered comparisons fail. A draw of 32,767 therefore fails even with resident type 11. The buff helper also leaves the threshold unclamped. The direct or area producer's subsequent comparisons cannot prevent a draw already taken.

| Binary32 value | Image constant RVA |
| --- | --- |
| 0.05, 0.1, 0.15, 0.2, 0.3, 0.4 | `0x2FCDB4`, `0x2FCB7C`, `0x2FCDB8`, `0x2FCDBC`, `0x2FCDC0`, `0x2FD318` respectively |
| 0.5, 1, 2, 300, 32,767 | `0x345DA0`, `0x345DC0`, `0x345E10`, `0x2FCE68`, `0x2FD338` respectively |

A successful source buff has three ordered effects before the ordinary hit or direct subtype decision.

1. **Resident application.** Initialize a 24-byte record with `0x02C520`, set its type byte to 10 for direct contact or 9 for area contact, and set its signed 32-bit duration at `+0x08` to 30,000. Binary32 magnitude at `+0x04` and the eight-byte embedded source ID at `+0x10` are zero. At `0x046AF0` the earlier source receives the record. For both these types the first existing matching type is overwritten in place with all 24 bytes, replacing any previous embedded ID with zero and leaving subsequent duplicates alone. Without a match the new record is appended at the list tail. The record copy is visible at `0x051D00`. Padding has no established semantic value.
2. **Buff event.** Append a 40-byte event to the tail of the shared event collection at accumulator `+0x58` via `0x0460A0`, with the copy at `0x051C70`. Both outer eight-byte actor IDs come from the earlier source, followed by the resident record. The contacted candidate supplies neither buff recipient nor outer ID.
3. **Positional cue.** Append kind 47 at the earlier source's position to the collection at accumulator `+0x08` via `0x0864C0`. The two trailing binary32 scalars initialized by `0x065FB0` are both 1. The positional conversion at `0x02C4A0` converts each signed 64-bit component to binary32 before multiplying by 1/65,536. No separate random draw appears in these three producer effects. Later rejection by the hit routine leaves them in place.

## The fourteen hit arguments

The world is the native receiver. Four-byte stack slots below start at 1 and exclude the receiver and return address. The immediate call preparations are `0x221E28–0x221E8C` and `0x222A52–0x222AB8`. The return at `0x1986F9` removes 56 bytes.

| Stack slot | Direct and area arguments |
| --- | --- |
| 1 | Current candidate pointer. |
| 2 | Later lookup's actor pointer, possibly null. |
| 3 | Prepared binary32 contact amount. The direct caller applies its ordered-negative clamp. |
| 4 | Critical result in the low byte. Both callers push a four-byte local after writing its low byte. The scoped writes do not establish the upper three bytes. |
| 5 | Projectile flag from `+0x5C`. The direct caller zero-extends the byte. The area caller pushes a four-byte local with that byte snapshotted before iteration, without establishing its upper bytes here. The flag byte can have values beyond 0 and 1. The resolver forwards the slot at `0x197661` and takes the low byte for its hit record. |
| 6 | Binary32 projectile `+0x54`. |
| 7 | Address of projectile fixed-point XYZ at `+0x18`. |
| 8 | Address of the three binary32 impulse components prepared before either scan. The caller copies projectile velocity `+0x3C`, normalizes it when its binary32 squared length compares greater than zero, then scales by five times `+0x54`. Finally it overwrites Z with three times `+0x54`. Evidence is at `0x220CA5` and `0x2214E0–0x221568`. |
| 9 | Pointer to the update's shared game-event accumulator. |
| 10 | Pointer to the caller's auxiliary collection also passed for local hit application. |
| 11 | Full 32-bit 1 when projectile kind is 1, otherwise zero. The hit helper tests the low byte. |
| 12 | Full 32-bit integer zero, placed at `0x221E3F` or `0x222A69`. This holds for every admitted combination of kind, flag, element, action and critical result. |
| 13 | Pointer to the current projectile record. The iterator accessor `0x02C6C0` returns the entry address plus eight. |
| 14 | Full 32-bit integer 1, placed before the iterator accessor, which consumes no stack argument. The hit helper tests the low byte. |

Slot 12 is at entry-stack offset `+0x30` and frame offset `+0x34`. The resolver compares the entire 32-bit slot with 28 at `0x197C5A` and 17 at `0x19804D`. Neither comparison matches for these two calls. The kind-1 Boolean in slot 11, projectile flag in slot 5 and critical byte in slot 4 retain their independent roles.

## Early returns, success and limits

At `0x196D73–0x196DCB` the hit routine returns a false low byte for ordered nonpositive candidate health, and for the distinguished candidate in nonzero mode. If the later source exists and its current action byte at `+0x68` is 91 or 74, candidate flag byte `+0x5C` must have bit `0x01` set to avoid the next false return. A null source takes its own path after the health and mode checks. On the ordinary record path at `0x197FA8–0x1980B8` the record takes the candidate position and ID, and either the later source's ID or eight zero bytes if it is missing. These observations cover only the indicated callee branches.

The low-byte returns at `0x221E91–0x221EA4` and `0x222ABD–0x222AD0` accumulate success. A true result sets it and a false result preserves a previous success. At `0x222B10–0x222B2E` a set success with a present later source and kind other than 3 increments the source's four-byte `+0x70` once, calls `0x0453B0`, and clears its four-byte `+0x74`. Kind 2 has the corresponding per-substep action at `0x22234F–0x222367`. Admission, contact, cues, buff application and consumed draws all precede or occur independently of the ordinary return. None certifies a successful ordinary hit.

Draw order above is per eligible attempt or area entry. The traversal visits candidates in native collection order and can repeat across movement subdivisions. It can process direct contacts before an area scan. No global per-projectile draw count, sort order, deduplication rule or guarantee of one visit per actor follows. Open dependencies include complete projectile creation, movement and expiry, full terrain/visibility and collision-state production, actor classifications and durations, item and base-stat formulas, resident expiry, event serialization and replication, and other calls, admissions, mitigation, protection, damage or randomness inside the common hit resolver. Weapon, element, buff and cue display names, exceptional non-finite outcomes beyond the stated comparisons, runtime lookup lifetime and native runtime results remain unestablished.

The historical attribution of both critical decisions to the later lookup is withdrawn. The direct call's receiver is loaded from the later result at `0x221A42`, while the area call at `0x2228C7` uses the earlier saved result loaded at `0x2228AD`. The later pointer survives as the area hit source. The original calls also establish the position layout and the asymmetric overlap bounds described above, narrowing the older uncertainty about those particular geometry facts.
