# Alpha generic-hit record production

Status: FINAL / STABLE / HELD. This independently worded source-side contract establishes a bounded record-production boundary. It does not certify a replacement implementation, runtime acceptance, a complete damage formula, or the behavior of the later record consumer.

The evidence root is `disassembled alpha cube world`. The supported original image is `binary/Cube.exe`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred image base `0x00400000`. Routine and instruction addresses below are RVAs; creature and record offsets are identified separately. This audit covers the 72-byte records produced inside generic hit `0x196D30`, including categories 0, 1, 3 and 5. It uses individually identified original function exports and original executable instructions/constants. No other contract, reconstruction, replacement implementation, tests, captures or implementation history supplies facts here. No server counterpart is claimed.

## Inputs and entry limits

The native object is the world. Explicit stack arguments are numbered from one, excluding that object. The relevant arguments are target pointer 1, source pointer 2, binary32 input amount 3, critical byte in argument 4, a separately used low byte in argument 5, binary32 status-chance input 6, a pointer to three binary32 vector components in argument 8, event-output object 9, auxiliary object 10, the low-byte selector in argument 11, a full 32-bit selector in argument 12, and a low-byte permission/presentation input in argument 14. The routine removes 56 bytes of explicit arguments on return. These argument descriptions do not assign unproved gameplay names to the unused or partially audited inputs.

The target must exist for the routine's direct accesses. Ordered nonpositive target binary32 health at creature `+0x16C` rejects the call before these records are constructed. An unordered health comparison does not take that particular rejection. A nonzero world mode byte `+0xB4` also rejects the call when the target pointer equals world `+0xB8`. With a nonnull source, source action byte `+0x68` equal to 91 or 74 requires target byte `+0x5C` bit `0x01`; otherwise the routine returns before record production. The action values are decimal.

A null source skips the source-dependent early category branches, consumes one random draw whose result is discarded, writes target binary32 `+0x1184` to 0.5, and supplies the original input amount to the later common-record region. The common record's source identifier is then zero. This is not a guarantee that every other argument combination is valid with a null source: selector 17's separate additional work directly accesses the source. Full caller admission and all such combinations remain outside scope.

Sources: `decompiled/client/functions/00596d30.c`; `binary/Cube.exe`, inputs and entry gates `0x196D6F–0x196DD1`, null-source continuation `0x197E3D–0x197E58`, common identifier assignment `0x197FFD–0x198023`, selector-17 source access `0x1980BF–0x1980D7`, return `0x1986F9`. Corresponding original instructions are also in `decompiled/client/assembly.txt`.

## Shared layout and initialization guarantees

The payload spans record `+0x00` through `+0x47`. Its native list node occupies 80 bytes: two four-byte links followed by the 72-byte payload. Appending uses allocation/copy helper `0x051EE0` and semantic copy helper `0x0599A0`, either through append helper `0x086290` or through the same operations expanded inside generic hit. Both forms append at the existing list tail.

Constructor `0x059A30`, used for the early records and the source/self record, sets the critical byte, the two 32-bit status fields, all 24 position bytes, all three vector components, and the three final bytes to zero. It does **not** initialize the first two identifiers or the binary32 amount. Those fields are assigned by each producer. The main category-0 record does not call that constructor; its observed stores nevertheless initialize every semantic field described below.

| Record offset | Width and interpretation established here | Shared production meaning |
| --- | --- | --- |
| `+0x00` | 64-bit identifier; copied as two 32-bit words | Source identifier, except that the source/self record uses the source identifier in both identifier fields. The main record uses zero when its source is null. No signed numerical ordering of identifiers is established. |
| `+0x08` | 64-bit identifier; copied as two 32-bit words | Target identifier, or source identifier again for the source/self record. |
| `+0x10` | Binary32 | Amount, with category-dependent origin described below. |
| `+0x14` | One byte | Incoming critical byte for the main record; zero for the other records audited here. The main assignment preserves the supplied byte rather than normalizing it to 0 or 1. |
| `+0x15–+0x17` | Three padding bytes | No semantic value established. The constructor does not initialize them and the semantic copy helper does not copy them. |
| `+0x18` | Signed 32-bit duration result when populated | Initially zero; only the main record's successful status-chance path replaces it. |
| `+0x1C` | 32-bit status word | Zero on every producer audited here. These zero stores do not establish a signed versus unsigned nonzero interpretation or its full consumer meaning. |
| `+0x20–+0x37` | Three eight-byte position components | Raw 24-byte creature-position copy. Nearby native presentation conversion treats the creature components as signed 64-bit integers, but the record producer preserves their bits without conversion. World units are not reconstructed here. |
| `+0x38`, `+0x3C`, `+0x40` | Three binary32 vector components | Full incoming vector, half incoming vector, or positive zero, depending on the producer. A main-record status success replaces its last component with 10. |
| `+0x44` | One byte | Argument 5's low byte for the main record; zero for early and source/self records. It is distinct from the critical byte and category. |
| `+0x45` | One-byte category | 0, 1, 3 or 5 according to the producer below. |
| `+0x46` | One byte | Initially zero. The main producer sets it to 1 for nonnull source action 28; the early and source/self records retain zero. |
| `+0x47` | One padding byte | No semantic value established; neither initialized by the constructor nor copied by the semantic copy helper. |

The semantic copier copies both identifiers, amount, critical byte, both status words, position, vector, and each of the three final bytes individually. This layout is a native resident/output-list layout; it does not by itself establish a wire serialization format or authorize copying opaque padding.

Sources: `decompiled/client/functions/00459a30.c`, `004599a0.c`, `00451ee0.c`, `0046d810.c`, `00486290.c`; corresponding original routines `0x059A30`, `0x0599A0`, `0x051EE0`, `0x06D810`, `0x086290`. Main initialization and field assignment: `binary/Cube.exe` `0x197FA8–0x1980BF`; later duration/vector and final-byte changes `0x198569–0x198598`, `0x198658–0x198677`.

## Category 3: an early terminal record

This branch requires a nonnull source and target control byte `+0x60` equal to 1, together with target byte `+0x124` bit `0x80` or target byte `+0x5C` bit `0x20`. It takes precedence over the category-1 predicate and resident-type-6 processing. Source action 28 can already have produced a separate presentation entry before this test; that entry is not a 72-byte record.

The branch first emits a separate presentation entry involving the target position and a random draw, then constructs its category-3 record. Record source identifier comes from source creature `+0x08/+0x0C`; target identifier comes from target `+0x08/+0x0C`; record position is the target's raw position beginning at `+0x10`. Amount is the unmodified input amount. The vector is the three unscaled input vector components. Critical byte and both status words are zero. Final bytes `+0x44/+0x45/+0x46` are respectively 0, 3, 0.

The record is appended to explicit argument 9's output list before the world-mode test. When world mode `+0xB4` is zero, generic hit then calls separate application routine `0x195A60`, passing this newly constructed record, the same output object, and auxiliary argument 10. Nonzero mode suppresses this application call without undoing the append. The generic-hit low-byte return is false in either case, including after append/application. No main category-0 record is produced by this terminal branch.

Sources: `decompiled/client/functions/00596d30.c`; original `binary/Cube.exe` branch admission `0x196EC1–0x196EDE`, presentation before construction `0x196EDE–0x196F41`, record construction `0x196F46–0x196FEE`, application gate and false return `0x196FF3–0x197016`.

## Category 1: the later early terminal record

After the category-3 branch has been bypassed, this branch requires nonzero argument 14's low byte, a true target predicate from `0x044590`, and target binary32 `+0x174` strictly greater than zero. Equality to zero and unordered comparison do not admit it. The predicate requires signed 32-bit target `+0x130` at most zero and target action byte in decimal set 71, 72, 8, 98; actions 59 and 63 also qualify when both target class byte `+0x140` and specialization byte `+0x141` equal 1. No other action passes that predicate.

Before constructing the record, the branch adds the old target `+0x174` to target binary32 `+0x170` and `+0x144`. It caps the new `+0x170` above at 1 and then caps the new `+0x144` above at the updated `+0x170`; equality retains the computed value. Its later reduction of target `+0x174` uses the original input amount divided by the binary32 result of target helper `0x03E190`. A residual strictly below -1 is replaced by -1; equality is retained. That helper's stat formula and zero/non-finite denominator cases are dependencies not resolved here.

The record uses source and target creature identifiers, target raw position, and the original input amount. It does not store the amount divided by that stat helper or the amount of a target-resource change. Its critical byte and both status words are zero. Each vector component is multiplied once by exact binary32 0.5. Final bytes are 0, 1, 0.

The target `+0x174` reduction occurs before appending the record. After append, the producer emits a separate target-position presentation entry, including its random draw, and only then checks mode for application. Application through `0x195A60` occurs only when world `+0xB4` is zero, with the record and same output/auxiliary objects. The routine then returns false in its low byte; no main category-0 record follows.

Sources: `decompiled/client/functions/00596d30.c`, `00444590.c`; original predicate `0x044590–0x0445CB`; generic branch admission `0x19701B–0x197043`, resource operations `0x197049–0x1970B8`, construction and resource residual `0x1970B8–0x197196`, append `0x197196–0x1971A5`, intervening presentation and draw `0x1971A5–0x197204`, application/return `0x197209–0x197230`. Binary32 0.5 is at image RVA `0x345DA0`, 1 at `0x345DC0`, and -1 at `0x345E8C`.

## Category 5: an early record that can be followed by the main record

After both terminal branches have been bypassed, the producer walks the target's resident list at creature `+0x1178`. It adds the binary32 magnitude of every resident type-6 entry in native list order. It does not filter those entries by remaining time in this walk. The sum is multiplied by a source-dependent factor: 1 minus the ratio of source signed 32-bit `+0x70` to helper `0x044CB0`'s signed 32-bit result, after conversion to binary32, with the ratio capped above at 1. There is no lower cap in this region. When the first ratio is not ordered-greater than 1, the native routine calls the helper again and recalculates the ratio rather than reusing that first result. The stat helper and permissible counter/denominator ranges remain dependencies.

A category-5 record is produced only when this binary32 product is strictly positive. Zero, negative and unordered products bypass its construction. The record nevertheless stores the **original input amount**, not the positive product, not a minimum of the two, and not the later remainder. Its identifiers come from the source and target creatures and its position from the target. Critical and both status words are zero. Each vector component is the input component multiplied once by binary32 0.5. Final bytes are 0, 5, 0.

The record is appended first. A separate target-position presentation entry and random draw follow. World mode zero then permits application through `0x195A60` with the same output and auxiliary objects; nonzero mode skips application. Only afterward does generic hit subtract the positive product from the original input amount. An ordered nonpositive remainder ends the routine with low-byte true and no main category-0 record. A positive remainder continues; an unordered remainder does not take that ordered-nonpositive exit. Consequently append does not imply that a category-0 record will exist, and category-5 amount is not rewritten to reflect the eventual continuation decision.

Sources: `decompiled/client/functions/00596d30.c`; original list sum/factor/admission `0x197235–0x1972F2`, constructor/identifiers/position `0x1972F8–0x197379`, amount/vector/final bytes and append `0x197379–0x1973FA`, intervening presentation `0x1973FF–0x1974BC`, application gate `0x1974C1–0x1974DD`, remainder and continuation `0x1974E2–0x19750B`, true return `0x1986D5`.

## Category 0: source/self record before the main target record

A separate source-dependent equipment region can accumulate an auxiliary binary32 magnitude. Its admission includes argument 5's low byte being nonzero, a strictly positive source-counter/stat ratio, and a nonempty source-derived collection. The collection helper `0x03DF60`, item contribution/stat helper `0x03CA60`, all item interpretations, and a complete numerical derivation of this auxiliary magnitude are not closed by this bounded contract. The confirmed production boundary is a strict-positive check on that accumulated magnitude; zero, negative and unordered values do not construct this record.

The producer calls the constructor, fills **both** record identifiers from source creature `+0x08/+0x0C`, copies the source's raw position from `+0x10`, and sign-toggles the positive auxiliary magnitude into the amount field using the native `0x80000000` sign mask. Critical byte, both status words, all three vector components and all three final bytes remain constructor zero. It is therefore a category-0 record with a negative amount and source position; it is not the main target record and does not use the incoming vector or incoming critical byte.

It appends that record to the same output-list tail. Immediately after successful append, the routine directly increases source binary32 health `+0x16C` by the positive auxiliary magnitude. There is no world-mode test, health upper cap, or call to `0x195A60` for this particular source/self record in this producer region. This direct health change precedes later main-record construction and application. It does not establish the behavior of remote consumers of the appended record.

Sources: `decompiled/client/functions/00596d30.c`; original auxiliary admission/accumulation region `0x1976D8–0x19795D`, strict-positive test `0x19798D–0x197990`, constructor/position/identifier/sign assignment `0x197996–0x197A21`, tail append `0x197A2E–0x197A5D`, direct source health change `0x197A5F–0x197A79`. Sign mask at image RVA `0x345F70` has `0x80000000` in each lane. The original item/stat helpers are explicitly missing dependencies rather than substituted formulas.

## Category 0: main target record

The main record uses the supplied source creature's identifier, or all-zero source identifier for a null source; it uses the target creature's identifier and raw target position. The source identifier and position are captured before the final target resident-type-1 multiplication walk. The target identifier and amount are filled afterward. Source/target identifiers are not read from earlier event records or resident-buff source fields.

Its binary32 amount is the working amount that survives the preceding generic-hit processing, multiplied once for every target resident type-1 magnitude in native list order. That walk does not test remaining duration. The source-dependent upstream working amount can include the category-5 subtraction, the selected target stat reduction, and equipment contributions. The reduction chooses helper `0x03CFF0` when argument 11's low byte is zero and `0x0467A0` otherwise, subtracts the source factor times that result, and replaces an ordered negative result with positive zero. The full stat/equipment formulas and additional helper effects are not reconstructed here. For the null-source continuation the working amount begins as the original input amount instead. There is no final nonnegative clamp after the resident-type-1 multiplication walk in this record-production region.

Record critical byte `+0x14` copies argument 4's low byte, independently of that amount assignment. No additional critical multiplication occurs at this assignment. The three vector fields copy the incoming components without scaling or normalization. Record `+0x18` and `+0x1C` begin at zero; final bytes begin as argument 5's low byte, 0, 0.

For nonnull source, target signed 32-bit `+0x12C` strictly below -3000, and target control byte different from 6, an additional status-probability region can replace `+0x18`. Equality to -3000 does not admit it. A successful strict comparison of the native random fraction against its computed threshold writes the result of target helper `0x044FF0` into `+0x18` and overwrites only vector component `+0x40` with exact binary32 10. The first two vector components remain the copied input values. The same success also emits a separate presentation entry before main-record append. The producer does not require the returned duration to be positive before these stores.

Helper `0x044FF0` returns signed 32-bit 1500 for target control zero. Otherwise it returns 3000 minus the integer quotient of the zero-extended target byte `+0x1A8` multiplied by 1500 and divided by 4. The product is nonnegative for all possible byte values, so this division truncates toward zero. The helper does not clamp its returned duration. No assumed narrower gameplay range for that byte is supplied here.

The probability calculation includes source/target action and classification helpers and stat transforms that are not fully reconstructed by this record contract. Its confirmed type-1 suppression still matters to draw order: any target resident type-1 entry replaces that threshold with zero, but the eligible region still consumes its subsequent random draw before failing a strict comparison against zero. The draw is converted to binary32 and divided by exact binary32 32767. No random draw is introduced by the record constructor or semantic copy operation itself.

For any nonnull source reaching the end of this region, source action byte 28 sets record byte `+0x46` to 1. Other actions retain zero. This final-byte decision does not depend on status success. The record category at `+0x45` stays zero and `+0x1C` stays zero.

Other side operations can precede this final append. In particular, full-width selector 17 performs additional resident-record and different-output-list mutations after the main record's amount/initial fields have been assigned. Those auxiliary record layouts and their application consequences are outside this 72-byte contract. Presentation operations can also consume draws before the status draw. This contract does not provide a complete whole-routine random-draw count.

Sources: `decompiled/client/functions/00596d30.c`, `00444ff0.c`; original selected reduction/clamp `0x19750B–0x19754D`; main position/default fields/source identifier `0x197FA8–0x198023`; resident-type-1 multiplication and target identifier/amount/critical/vector/final-byte assignments `0x198023–0x1980BF`; selector-17 side-operation boundary `0x1980BF–0x1981CB`; status outer gates `0x1983EC–0x198412`, probability calculation/type-1 suppression/draw `0x198412–0x198563`, successful duration/vector stores `0x198569–0x198598`, final source-action byte `0x198658–0x198677`; duration helper `0x044FF0–0x04501A`. Binary32 10 is immediate bits `0x41200000`; divisor 32767 is at image RVA `0x2FD338`.

## Output and application ordering limits

All audited 72-byte records append to the list at the base of explicit argument 9's event-output object; no extra member offset is added for these records. Normal append performs allocation/semantic copy, checks native list capacity, increases the list count and links the new tail. Native allocation/length failures are not normal successful emissions.

The main record appends only after its fields, optional status/presentation work and final source-action byte have been settled. Only after append does world mode zero permit the call to `0x195A60`, with the world as its native object and explicit arguments consisting of this record, the same event-output object, and auxiliary argument 10. Nonzero mode skips the call, even though the record has already been appended. This producer does not add a separate distinguished-source equality exception to that application gate. The initial world/target rejection remains an earlier admission rule.

The main path returns true in its low byte after append, including when application was skipped by mode. Category 5 can also return true after only its early record; categories 1 and 3 return false after their record. These return values therefore do not encode a uniform rule that one record was appended, local health changed, or the resolver accepted a hit.

The separate application routine may have further lookups, rejection, queuing, health/status calculations, effects and random draws. None is recovered afresh in this contract. Append-before-call ordering establishes that the output record already exists before any such consumer behavior. Full damage/status formulas, all caller validity rules, auxiliary lists, numeric behavior of unexpanded helpers, source/target lifetime changes, packet serialization and server production differences remain explicit missing dependencies.

Sources: `decompiled/client/functions/00486290.c`, `00451ee0.c`, `0046d810.c`, `004599a0.c`, `00596d30.c`; original main append `0x198677–0x1986B2`, mode/application arguments `0x1986B4–0x1986D0`, true return `0x1986D5`; early append/application/return anchors cited in the category sections above. The consumer is identified only by original entry RVA `0x195A60`; its body is not a source for additional resolver claims in this document.
