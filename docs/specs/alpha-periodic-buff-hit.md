# Alpha type-4 periodic buff: native hit boundary

Status: FINAL / STABLE / HELD. Original-only source contract, 2026-11-08. This is an independently worded, bounded audit of the surviving type-4 periodic-buff call at RVAs `0x218310`–`0x2183DF`, its hit callee at RVA `0x196D30`, and the relevant local event-application paths. Numeric field and category identifiers below are evidence identifiers, not recovered native textual names.

## Image and source boundary

The supported original client is `binary/Cube.exe` under the `disassembled alpha cube world` evidence root: 3,885,568 bytes; SHA-256 `84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF`; PE32 machine `0x014C`; preferred base `0x00400000`. All source paths below are relative to that evidence root, and all addresses described as RVAs exclude that base. Server-image equivalence and native runtime acceptance are not established.

The assigned source-author policy was read before this scope. Only original function, assembly and binary evidence was consulted for this audit. No reconstruction, replacement implementation, SDK, extractor, tests, captures, probes, logs, implementation history or existing behavioral specification was read for this scope. Previously held artifacts were not changed.

## The precise call supplied by the periodic site

The enclosing update subtracts elapsed milliseconds from each resident buff's signed 32-bit remaining time at record offset `+8`. A nonpositive result is queued for later expiration erasure and does not enter this periodic call. For a surviving record whose type byte at `+0` equals 4, the world byte at `+0xB4` must be zero. The old and new remaining times, each divided by 400 with truncation toward zero, must differ. There is one hit call on that crossing, including when an update crosses more than one such interval; this site does not replay a call for every crossed interval. [P1]

The record's eight-byte source identifier at `+0x10` is looked up in the world's creature map. The lookup performs exact identifier comparison and returns the stored creature pointer, or null for no entry. It does not test source health, class, source/target equality or buff ownership validity. No special rule for identifier zero is present in this lookup. [P1, P2]

The hit call has the world as its receiver and the following supplied values. Stack offsets are relative to the hit routine's established frame, making the original evidence independently locatable without relying on a decompiler's parameter types. [P1, P3]

| Hit-frame offset | Value supplied by this site | Verified role or limit |
| --- | --- | --- |
| `+0x08` | Current creature pointer | Target |
| `+0x0C` | Result of the buff-source lookup | Source, possibly null or the target itself |
| `+0x10` | Four-byte float from buff `+4` | Raw amount; this site does not clamp or validate it |
| `+0x14` | Zero | Byte copied into hit record `+0x14`; also disables the target reaction-sound branch |
| `+0x18` | Zero | Byte copied into hit record `+0x44`; disables the equipment-affix damage/resource-effect branch |
| `+0x1C` | Floating zero | Initial stun-probability input; later native special cases can replace it |
| `+0x20` | Address of target `+0x1320` | Supplied address; no read of this argument occurs in the audited hit body |
| `+0x24` | Address of three floating zeros | Initial impulse vector |
| `+0x28` | Caller-owned outgoing collection | Receives hit, sound, particle and related records in the paths described below |
| `+0x2C` | Address of a second caller-owned context | Passed onward to local event application; its complete contents are outside this contract |
| `+0x30` | One | Selects target stat routine RVA `0x0467A0`, rather than RVA `0x03CFF0` |
| `+0x34` | Zero | The hit routine's selector-17 insertion of a new type-4 buff is not taken |
| `+0x38` | Zero | No use of this argument was identified in the audited hit body |
| `+0x3C` | Zero | Disables the resource-blocking and ordinary attack-presentation branches gated by this byte |

The initial zero impulse is not a promise that the final hit record has no impulse: the native successful-stun path changes its vertical component to `10.0`. [P3]

## Null source, self source and other source

The hit routine first rejects a target whose current-health float at `+0x16C` is nonpositive. Its general remote/local-target rejection also exists, but the periodic caller has already required world `+0xB4` to be zero. [P3]

A null source consumes one `rand` draw whose result is discarded, writes `0.5` to target float `+0x1184`, and carries the raw amount into the later target-buff multiplier stage. It bypasses the found-source mitigation and target-stat subtraction described below. This path does not simply subtract the raw amount from health. [P3]

A found source is used as found, even if dead. There is no self-source branch: a self lookup uses the target object for all source reads as well as target reads. With another source, those same reads use the other object. In particular, the integer at source `+0x70` and the maximum returned for that source by RVA `0x044CB0` are source values. Substituting the target's resource values for them would change this call when source and target differ. [P3, P4]

For a found source whose ability byte at `+0x68` equals 91 or 74, target ground flag `0x01` at `+0x5C` must be set or the hit returns before producing the later hit. Source ability 28 instead produces an early particle record, consuming one random draw for its lifetime. [P3]

For a target with class byte `+0x60` equal to 1, either target flag `0x80` at `+0x124` or flag `0x20` at `+0x5C` selects an early response: one random sound draw and sound identifier 24 precede a hit record whose category byte at `+0x45` is 3. That record contains the original amount and the supplied impulse, is queued and locally applied, and the hit routine returns. Category 3 is excluded from the later local health/stun application path, although its earlier resource/contact bookkeeping can still run. [P3, P5]

## Found-source amount, resource and stat use

The target's resident buff list is at `+0x1178`. The hit routine sums the floating strengths of every resident type-6 buff in list order, without checking their remaining times. It obtains the source ratio from source signed integer `+0x70` divided by the integer maximum returned by RVA `0x044CB0`, with the denominator converted to float. Only an upper cap of 1 is applied for the mitigation factor. The amount of type-6 mitigation is the strength sum multiplied by one minus that capped ratio. No lower ratio clamp or zero-denominator guard is present in this audited calculation. [P3, P4]

A positive mitigation amount causes a separate category-5 hit record to be queued first. Its event amount is the original input amount, not the computed mitigation amount. A random sound draw then produces sound identifier 91, and that category-5 record is locally applied. Only afterward does the hit routine subtract the computed mitigation from its working amount. A nonpositive remainder ends this invocation. The category-5 buff effects below therefore occur before that early return. [P3, P5]

If calculation continues, the same one-minus-capped-source-ratio factor multiplies the return from target stat routine RVA `0x0467A0`; this product is subtracted from the working amount, which is then clamped below at zero. The periodic call's selector is explicitly 1, so this is the target's `0x0467A0` stat path. That routine uses target float `+0x188`, target level integer `+0x190`, byte `+0x1A8`, class `+0x60`, flag `0x20` at `+0x7E`, and eligible equipped records at `+0x530`, `+0x648`, `+0x760`, `+0x878`. Its equipment contributions call RVA `0x0C7AF0`. This contract establishes the selected object and routine; it does not replace the full equipment-stat curve with a constant or infer a stat name. [P3, P6]

The source maximum at RVA `0x044CB0` truncates source float `+0x17C` times a selected coefficient to an integer. The default coefficient is 50; source flag `0x10` at `+0x7E` selects 20 outside the item-class-3 case. For an item-class-3 record at source `+0xAA8`, its subtype at `+0xAA9` selects 60 for subtype 6, 80 for subtype 8, and 20 for subtypes 11, 12, 15, 16 or 17; other such subtypes use 50. [P4]

The source ratio is obtained again without the upper cap. Above `0.25`, the native routine visits its selected source equipment and can queue element particles through RVA `0x19A800`. This remains reachable with the periodic site's zero at hit-frame `+0x18`. That zero does exclude the later affix-derived extra damage, source healing and timer effects. Source ability 28 also causes a lower-bound update of target `+0x1184` to `0.3` when the found-source path reaches that point. [P3, P7]

Before final event emission, every resident target type-1 buff multiplies the working amount by its strength, in resident list order. There is no remaining-time test in that scan, and no clamp after those multiplications. Thus a queued-for-expiration type-1 node can still affect this hit while it remains linked. The site does not itself establish that the resulting amount is positive or finite. [P3]

## Native stun test despite a zero supplied base

With a found source, the later stun test is entered only when target signed timer `+0x12C` is strictly below `-3000` and target class is not 6. Source ability 58, 12, 68, 93 or 69 replaces the zero base with 1. A target species predicate can scale the current base by `0.15`; a nonmatching source with nonzero class scales it by `0.1`. Source ability 91 or 74 subsequently replaces the base with 1, as does target ability 84. The species predicate's true set is the numeric target/source `+0x64` identifiers 101, 107, 108, 109, 114, 115, 116, 117, 118 and 119. [P3, P8]

The result is multiplied by a power of 2 using ten times the difference between the source and target level curves at RVA `0x03CA60`, capped above at 1, and scaled by `0.9`. The presence of any resident target type-1 buff makes the final threshold zero. For the level curve, the denominator is one plus `0.05` times the quantity “level minus one”; its reciprocal is subtracted from one. The native sequence retains its floating conversions and power operation; no probability simplification is asserted here. [P3, P8]

Once this test is entered it consumes a `rand` draw even when the resulting threshold is zero. The comparison is strict: the draw divided by `32767.0` must be below the threshold. Success sets hit-record stun duration `+0x18` from target routine RVA `0x044FF0`, changes its impulse's vertical component to `10.0`, and queues sound identifier 23 before the final hit record. That target duration is 1500 for class 0. Otherwise the target byte at `+0x1A8` is multiplied by 1500, that product is shifted right by two, and the shifted value is subtracted from 3000. [P3, P8]

## Final event and bounded local health application

The ordinary final hit record carries the source object's identifier, or zero when the original source pointer was null; the target identifier; the working amount; target position; the supplied zero flags; and category byte `+0x45` equal to 0. Source ability 28 sets the record byte `+0x46` to 1 without changing that category. The final record is appended before calling the local event applier at RVA `0x195A60`. The periodic site's world-mode gate makes this local application reachable, rather than leaving the hit as a queued record only. [P3, P5]

The applier looks up the target and source identifiers again. It does not inherit an assumption that a zero source identifier cannot resolve. The target must exist and still have positive health. If world pointer `+0xB8` is null and target class is 0, it appends/forwards the record and returns without the later health path. Otherwise positive event amounts first reduce target float `+0x1190`, clamped at zero, by one quarter of the amount divided by a power of 2. That power's exponent is `0.25` times the sum of “target level minus one” and target byte `+0x1A8`. A positive amount also cancels target ability 84 and sets target `+0x12C` to 3000. These changes precede category-dependent health application. [P5]

For nonnegative amounts and a resolved source, the applier updates source/target contact identifiers, target/source threat records and their outgoing records through RVA `0x1A0970`. This can occur even for category 3 or 5. Complete threat propagation is outside this contract. Target flag `0x2000` at `+0x7E` additionally requires a positive threat entry associated with a found class-0 creature before the later application path is admitted. [P5]

For category 0, a positive event amount is subtracted from target health only if target predicate RVA `0x03E310` is true and, when the event source resolves, that source predicate is also true. The predicate accepts classes 0, 1, 3 or 5, and class 6 except numeric species 140, 141 and 142. It rejects the other classes. A self-source receives the same predicate checks on the same object; it has no exemption. This is a conditional health effect, not an unconditional periodic subtraction. [P5, P9]

The applier uses a positive record stun duration to raise target `+0x12C` to the larger of its existing value and the record value. For a nonnegative event amount, a positive stun duration also clears target velocity floats `+0x34`, `+0x38`, `+0x3C`. These writes occur before the positive-health predicate. The record's additional timer at `+0x1C` is zero for the audited generated records. The initial zero vector cannot produce ordinary horizontal knockback here; successful stun is the explicitly verified vertical-vector exception. [P5]

A negative ordinary event amount takes a healing path, capped by target maximum-health routine RVA `0x044DB0`; zero does not subtract health. No positive-strength precondition was added by the periodic site. This contract does not certify all negative, nonfinite or zero-capacity combinations through the earlier mitigation branches. [P1, P3, P5]

If application leaves target health nonpositive, the applier clamps it to zero, clears target `+0x6C`, queues sound identifier 27 and enters native death, loot, relationship and world-record paths. Their complete side effects and RNG consumption are outside this contract. In particular, this document does not certify fatal-hit downstream behavior as equivalent to setting health to zero. [P5, P10]

## Buff mutation before expiration cleanup

For a positive category-5 event, the applier visits every resident target type-6 buff, subtracts the event amount from that buff's float strength, and writes both strength and remaining time to zero when the resulting strength is nonpositive. It queues a buff update for each matching node. The traversal has no remaining-time filter. It does not distribute a single budget among those buffs: each matching buff receives the same event-amount subtraction. [P5]

The audited category-5 loop changes buff payloads but does not unlink those nodes. Neither the audited hit-body scans nor this category-5 application loop performs a synchronous erase from target `+0x1178`. The periodic site's selector zero also excludes the generic hit routine's selector-17 insertion of a new type-4 buff. These are bounded findings for the traced paths; unaudited fatal downstream calls are not covered by a global “no buff removal” claim. [P3, P5]

The caller later erases the iterators collected by its expiration scan, after periodic processing. This ordering matters. A type-6 node depleted before the scan reaches it can be queued and erased in that same update. A node already visited as surviving, then depleted by a later periodic hit, is not retroactively added to that collected set by the category-5 loop. An expired node already collected remains resident for the hit routine's type-1/type-6 scans until cleanup. These consequences follow from the native resident-list scans, payload-only depletion and deferred expiration erasure; no container redesign is specified. [P1, P3, P5]

## RNG and event-order limits

Before any unaudited fatal downstream work, the explicitly verified direct draws are: the null-source discarded draw; the found-source ability-28 particle draw; the class-1 early-response sound draw; the positive type-6 mitigation sound draw; and the conditional stun-test draw. Their order follows the prose above. With hit-frame `+0x3C` and `+0x14` zero, the native ordinary attack-presentation and target reaction-sound draws gated by those inputs are excluded. The equipment particle helper and its inspected color helper do not draw random values. [P3, P7]

Mitigation-category events are queued before their sound and local application. The final event is queued after any successful-stun sound and before its local application. A source-ability-28 particle can precede either early response. Health, threat and buff changes caused by a mitigation event can therefore already be visible when the routine computes or applies the final event. This is not a single generic hit category with a fixed RNG count. Downstream death/loot draws and complete threat/world effects require separate native contracts. [P3, P5, P10]

## Original evidence references

- **P1:** `decompiled/client/assembly-functions/0060c510.asm`, routine RVA `0x20C510`: remaining-time decrement/expiration collection at `0x212130`–`0x212163`; surviving periodic call at `0x218310`–`0x2183DF`; iterator advance at `0x2183FA`–`0x218436`; deferred expiration erasure at `0x21843C`–`0x2184C0`. Division multiplier and shifts at `0x218336`–`0x21835C` establish the 400-ms bucket test.
- **P2:** `decompiled/client/functions/0042f000.c`, RVA `0x02F000`, creature lookup; `decompiled/client/functions/00477d90.c`, RVA `0x077D90`, exact eight-byte key comparison and no-entry result.
- **P3:** `decompiled/client/functions/00596d30.c`, RVA `0x196D30`; original instructions in `decompiled/client/assembly.txt`, restricted to that function. Entry and gates at `0x196D73`–`0x197235`; type-6 mitigation and selected target stat at `0x197235`–`0x19754D`; equipment particle path at `0x19754D`–`0x1976D8`; source-null path at `0x197E3D`; resident type-1 multiplier at `0x198023`–`0x19804D`; periodic-disabled selector-17 branch at `0x19804D`–`0x1981CB`; stun calculation at `0x1983EC`–`0x198658`; final append/application at `0x198658`–`0x1986D0`. No instruction in this body reads hit-frame `+0x20` or `+0x38`.
- **P4:** `decompiled/client/functions/00444cb0.c`, RVA `0x044CB0`; source object selection confirmed by original hit instructions at `0x197267`–`0x1972D5` and `0x19754D`–`0x197573`.
- **P5:** `decompiled/client/functions/00595a60.c`, RVA `0x195A60`; original instructions in `decompiled/client/assembly.txt`, restricted to that function. Target/source lookup and forwarding gate at `0x195AAB`–`0x195B3E`; target resource and contact bookkeeping at `0x195B3E`–`0x195CE4`; threat/class-0 admission at `0x195CE7`–`0x195D70`; category/impulse/timer behavior at `0x195D70`–`0x195EED`; positive category-5 buff depletion at `0x195EFB`–`0x195FC7`; class-gated positive health subtraction at `0x195FCC`–`0x196003`; negative-amount healing at `0x196008`–`0x196058`; fatal continuation from `0x196133`. Event initialization: `decompiled/client/functions/00459a30.c`, RVA `0x059A30`.
- **P6:** `decompiled/client/functions/004467a0.c`, RVA `0x0467A0`, and its original instructions in `decompiled/client/assembly.txt`; item stat helper `decompiled/client/functions/004c7af0.c`, RVA `0x0C7AF0`. Complete item scaling is an explicit boundary rather than inferred replacement arithmetic.
- **P7:** `decompiled/client/functions/0043df60.c`, RVA `0x03DF60`, selected equipment list; `decompiled/client/functions/0059a800.c`, RVA `0x19A800`, equipment particle append; `decompiled/client/functions/004c7250.c`, RVA `0x0C7250`, inspected color calculation.
- **P8:** `decompiled/client/functions/004445d0.c`, RVA `0x0445D0`, numeric species set; `decompiled/client/functions/0043ca60.c`, RVA `0x03CA60`, level curve; `decompiled/client/functions/00444ff0.c`, RVA `0x044FF0`, target stun duration. Receiver selection and order confirmed in original hit instructions at `0x19842E`–`0x19856B`.
- **P9:** `decompiled/client/functions/0043e310.c`, RVA `0x03E310`, health eligibility predicate. Its target-then-source invocation order is at `0x195FCC`–`0x195FEE`.
- **P10:** `decompiled/client/functions/005a0970.c`, RVA `0x1A0970`, threat/contact propagation inspected to bound its role; `decompiled/client/functions/0059b180.c`, RVA `0x19B180`, death/loot continuation inspected but not completely specified. Additional fatal world/relationship callees remain unaudited.
- **P11:** Constants read directly from `binary/Cube.exe`: RVAs `0x2FCB7C` (`0.1`), `0x2FCDC0` (`0.3`), `0x2FCDB8` (`0.15`), `0x2FD31C` (`0.9`), `0x2FCD74` (`20`), `0x2FCD7C` (`50`), `0x2FCE48` (`60`), `0x2FCE50` (`80`), `0x2FD338` (`32767`), `0x345E74` (`10`), and double at `0x345E18` (`2`). Other fixed 0/1, 0.25 and 0.5 values are present in the cited original instructions and image.

The author read the complete final prose back after writing it. FINAL / STABLE / HELD: author writes are stopped.
