# Alpha world actor-creation boundary

Status: FINAL / STABLE / HELD. Full author readback completed.

This contract covers the original client’s actor-creation branch at RVA `0x2157A8`, through the pointer publication at `0x215BA6`. It identifies the actual action selector, the local admission and live scans, all assignments made directly by this branch, the relevant constructor and copy defaults, attached setup records, and the closed random-call ordering. It leaves the complete appearance output for arbitrary selected kinds and future virtual-method behavior explicitly bounded.

The evidence root is `disassembled alpha cube world`. The supported image is `binary/Cube.exe`, a 32-bit image with preferred base `0x00400000`, length 3,885,568 bytes and SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`. All addresses below are RVAs, including data and virtual-table identifiers. Evidence is this image, its original instruction listing `decompiled/client/assembly.txt`, and individually selected original function files under `decompiled/client/functions/`. The world routine’s decompiled file `0060c510.c` reports a decompilation timeout; its unavailable body is not used to fill gaps. No other project contract, reconstruction or runtime material supplies facts here.

“Current actor” means the actor being processed by the world call. “New actor” means the separately allocated object produced by this branch. These terms do not assign a gameplay action name or a universal ownership relation.

## Actual caller phase and selector

World update `0x20C510` takes an integer first argument that is saved without conversion as a 32-bit value at `0x20C549–0x20C54F`. The comparisons and additions described here use that integer step. This contract does not derive a physical duration unit from its value or reconstruct all world callers.

The actor pass begins at `0x210493` over the current world map at world `+0x04`. It obtains each mapped actor pointer and skips null pointers. This is a live map traversal, not an actor-array snapshot. Before the action dispatch, the routine processes an earlier actor list at `+0x130C`; that list’s full contents and callbacks are outside this contract. It then conditionally performs its mode-zero cleanup, queries helper `0x044DB0` for the current actor, and clamps health at `+0x16C` to the queried value only when health is greater. That clamp makes a second query on its taken route. With ordinary finite values, health at or below zero then skips the later living-actor path.

Further work before dispatch can alter the current actor. In the mode-zero, nonzero-discriminator route, the accumulator at actor `+0x13C4` receives the integer step. If the signed value reaches at least `32` and actor `+0x12C` is nonpositive, a nonnull actor `+0x13E4` leads to that object’s first virtual entry with the actor, world, accumulated value and shared output. At this reached boundary the accumulator is then cleared, including when its callback pointer is null. A positive `+0x12C` bypasses this callback/clear boundary. Other mode-zero queued-state helpers can also run before dispatch. Their complete decisions and transitive effects are not reconstructed here.

Immediately before the selector read, the routine subtracts the integer step from signed 32-bit actor `+0x130`. A still-positive result leads to helper `0x03D1A0` and an action-clock assignment of that helper’s result plus one; a negative result is subsequently clamped to zero. The timers at `+0x134` and `+0x138` are reduced and lower-clamped, and a separate negative scalar at `+0x13C` can be advanced toward zero. Thus the creation branch uses the actor state after those phases, including any changes made by earlier callbacks. It does not use an action value saved at world-call entry.

The selector is exactly the unsigned byte at actor `+0x68`, read at `0x211E04`. The range and two-table dispatch at `0x211E07–0x211E1B` reaches `0x2157A8` for exactly selector `92` decimal, or `0x5C`. The byte table at `0x222CCC` maps its entry `91` to table index `12`; entry `12` of the pointer table at `0x222C84` identifies this branch. No other selector in the admitted `1..110` range shares that branch target. The neighboring creation-looking branches are not aliases of this selector. No action name is inferred.

Evidence: image/listing `0x20C541–0x20C564`, `0x210493–0x21051C`, `0x211A86–0x211E1B`, and original table bytes at `0x222C84` and `0x222CCC`.

## Admission and threshold calls

At the branch boundary, execution is admitted when either world byte `+0xB4` is zero and current actor byte `+0x60` is nonzero, or the current actor pointer equals the four-byte designated pointer at world `+0xB8`. This gate does not itself exclude discriminator `6`, compare actor identifiers, test a class/specialization or inspect a source flag word. Its pointer comparison is exact native pointer identity.

After that gate, helper `0x03CAA0` is queried with the current actor and signed argument `-1`. It resolves that negative selector argument by reading the current action byte. The current signed 32-bit action clock at `+0x6C` must be no greater than this first result. Failure ends this branch after one query. Passing causes a second separate query with the same actor and `-1`; the 32-bit sum of the then-read clock and integer step must be strictly greater than this second result. Equality at this second boundary rejects creation. The native addition wraps at 32 bits before the signed comparison. There is no additional positive-step or zero-step guard within these branch tests.

For action `0x5C`, the original threshold helper uses its general scalar route. It obtains helper `0x047700`’s scalar, rounds the returned value to binary32, multiplies it by actor binary32 `+0x17C`, divides binary32 `400.0` by that binary32 product, and truncates the binary32 quotient toward zero to a signed 32-bit result. The constant is stored at image RVA `0x2FD8EC` as `0x43C80000`. The ordinary numeric relation assumes a finite quotient representable by that integer conversion. Invalid conversions, zero denominators and floating-point traps are outside the closed ordinary relation.

The two queries remain two actual calls. Helper `0x047700` depends on current actor scalar/type, class/specialization, equipment and resident-record state through its scoped stat helpers; this contract does not replace that dependency with a fixed duration or constant speed. The inspected query/stat call graph has no native `rand` or `srand` call. Imported numeric-library internals and asynchronous changes are not an ambient-state guarantee.

Evidence: image/listing `0x2157A8–0x2157F7`, `0x03CAA0–0x03CE1F`, particularly `0x03CDF8–0x03CE1A`; original files `0043caa0.c`, `00447700.c`, `004478b0.c`, `00444cb0.c`, `004c7c00.c`, `00445ff0.c`.

## Live count and identifier selection

Once both threshold tests pass, the branch starts its own traversal of the current world `+0x04` map. Its iterator starts at the live first node, advances by the original ordered-map successor helper, and compares against the current end sentinel. It does not use the earlier actor pass’s cursor or a list of actors captured at the beginning of the world call.

Every nonnull mapped actor is examined. The scan counts those whose full 64-bit link at actor `+0x198` equals the current actor’s full 64-bit identifier at `+0x08`. It does not additionally require positive health, a particular discriminator, a specific action, spatial proximity or a different actor pointer. Consequently the current actor can itself count if its link equals its identifier, and zero identifiers/zero links are ordinary equality inputs. Three matching nonnull actors reject creation; exactly two still admit a new actor. A dead but nonnull actor can count. The ordinary relation assumes a valid container and a count that does not overflow its signed 32-bit counter.

The same scan chooses an identifier baseline independently of the link count. It starts at signed 64-bit zero and retains the lowest signed 64-bit map key among nonnull mapped actors and that initial zero. Keys with null mapped pointers do not change the baseline. The new identifier is the baseline decreased by one with native 64-bit wrap. It comes from the map keys, not from an actor’s stored identifier, a random draw or a separate allocation service. Thus ordinary nonnegative keys produce identifier `-1`; an existing nonnull key `-1` produces `-2`. The two stored halves of the chosen identifier become the new actor’s `+0x08` and `+0x0C` through its constructor.

There is no separate collision check between this calculation and publication. For example, a null-valued entry already at the chosen key is ignored by the minimum scan and can later have its mapped pointer replaced. Signed-minimum underflow wraps to the largest signed identifier and lies outside the ordinary predecessor-order conclusion below. This contract does not invent an allocation rejection for that case.

Evidence: image/listing `0x2157FD–0x215938`; original iterator helpers `0046f440.c`, `0060a9f0.c`, `004688d0.c`, `00468c90.c`, `00468d70.c`, `005c3ea0.c`.

## New actor state and source identities

The ordinary successful route allocates `0x1E60` bytes and invokes creature constructor `0x03B690` with the chosen identifier. It then performs the following complete set of branch-level actor assignments and copy/generation operations before publishing the actor pointer. The source identities in this table are deliberately separate.

| New actor destination | Width and resulting source/value |
| --- | --- |
| `+0x08`, `+0x0C` | Two 32-bit halves of the scan-selected 64-bit identifier, installed by construction. |
| `+0x60` | One byte: `3` when current actor `+0x60` is zero, otherwise `1`. |
| `+0x64` | Full 32-bit kind from the choice described below. |
| Appearance at `+0x78` | Either the selected actor’s semantic appearance fields, or the kind-dependent output of `0x03F7C0` applied to the new actor’s default appearance. |
| Thirteen item records at `+0x300` | Copied from the selected actor on the two copy alternatives; otherwise their constructor defaults remain. |
| Flags word `+0x7E` | After appearance work, bit `0x0200` is cleared and `0x0800` set in the full 16-bit word. All other bits survive this operation. |
| `+0x190` | Full 32-bit value copied from the current actor. |
| `+0x16C` | One query of `0x044DB0` on the current actor, rounded into this binary32 destination. It is not a copy of current health and is not a query on the selected appearance/equipment actor or the new actor. |
| Position storage `+0x10..+0x27` | Six raw 32-bit copies, preserving all 24 bytes from the current actor through `0x02C5B0`; no offset or coordinate arithmetic is applied here. |
| `+0x13E4` | Four-byte pointer to the separately built setup object described below. |
| `+0x198`, `+0x19C` | Two 32-bit halves of the current actor’s full identifier at `+0x08`, installed after setup-object construction and before map publication. |

The branch does not copy the current actor’s full data object. Its own initial action byte/clock, class/specialization bytes, experience, other stat scalars, contact identifiers, resident list and maps retain their constructor values except where explicitly listed above. In particular, current actor `+0x11D0` is a selection input on one alternative; it is not copied into new actor `+0x11D0`. The new `+0x198` link is installed from the current actor even if another actor provided appearance/equipment.

There is no post-assignment new-actor health query or clamp within this branch. The source of the health scalar can therefore have different class, rarity or equipment from the new actor’s retained default fields or copied equipment. A separate later maximum-health query is not silently substituted for this original call.

Evidence: image/listing `0x2158FD–0x215A6B`, `0x215B70–0x215BA6`; original `0043b690.c`, `0042c5b0.c`, `00444db0.c`. The inspected maximum-query item/stat helpers `004c70b0.c` and `00445f60.c` contain numeric calculations and imported powers, without native random/seed calls.

### Candidate selection precedes the copy/generation choice

The candidate collection belongs to the current actor at `+0x13F8`. Its entries are four bytes each. An empty collection supplies full identifier `0x25` without a candidate draw. A nonempty collection causes one imported random draw; its unsigned 32-bit result is reduced by unsigned remainder over the collection count and selects a full 32-bit entry. There is no eligibility filter on the selected kind in this branch.

Only afterward does the original read and compare the current actor’s full 32-bit field `+0x13F4`. The exact values `1` and `2` select copies. Value `1` uses the current actor. Value `2` resolves the full 64-bit key at current actor `+0x11D0` through world lookup `0x02F000`; a missing entry or null mapped pointer falls back to the current actor. Lookup has no special zero-key or same-actor rejection. On either copy route, kind is copied first, then thirteen item records, then appearance.

Every other 32-bit `+0x13F4` value selects generation. It stores the already chosen candidate/default kind in new actor `+0x64` and calls `0x03F7C0` with that field’s address, the new appearance address and a null third argument. With that null third argument, this helper does not replace the chosen kind through its nonnull-descriptor remapping route. It operates on appearance; it is not an equipment-generation call.

The candidate selection draw has already occurred when the collection is nonempty, even on either copy alternative where the selected value is discarded. A copy alternative with an empty collection still copies its selected actor’s kind; `0x25` is not substituted for that copied kind.

Evidence: image/listing `0x21593E–0x215A29`; original files `00477240.c`, `00487f50.c`, `00468c60.c`, `0042f000.c`, `0044af00.c`, `0043f7c0.c`.

### Copy extents and constructor-backed defaults

Appearance assignment `0x059800` copies bytes at appearance-relative offsets `0..4`, the 16-bit word at `+0x06`, the three 32-bit fields at `+0x08/+0x0C/+0x10`, eight 16-bit fields at `+0x14..+0x22`, and all four-byte fields from `+0x24` through `+0xA8` in increasing four-byte steps. Appearance byte `+0x05` is not part of this semantic copy. No numeric conversion or random generation is performed by that copy.

Equipment assignment `0x04AF00` processes thirteen `0x118`-byte item records in increasing record order. For each record, `0x02C5E0` copies bytes `+0/+1`, four-byte fields `+0x04/+0x08`, bytes `+0x0C/+0x0D/+0x0E`, the 16-bit field at `+0x10`, 32 successive eight-byte records from `+0x14` through `+0x113`, and the four-byte field at `+0x114`. Each eight-byte record preserves four individual leading bytes followed by a four-byte field. Header gaps `+0x02/+0x03/+0x0F/+0x12/+0x13` are outside these copies. The branch does not copy an item inventory beyond these thirteen records.

On generation alternatives, the item constructor `0x03BC00` leaves every copied semantic item field zero except the 16-bit field at item `+0x10`, which is `1`. The 32 eight-byte records are all zero, as is item `+0x114`. It initializes all thirteen items in that way. Neither item construction nor these field-copy helpers invokes a random/seed API.

Relevant retained actor defaults are established by `0x03C100` on actor `+0x10` and by `0x03B690`, rather than by assuming the allocation is zero-filled:

| Actor-relative field/group | Constructor value retained unless overridden above |
| --- | --- |
| `+0x28..+0x57` | Twelve four-byte zero stores. |
| `+0x58/+0x5C` | Four-byte zero each. |
| `+0x68` and `+0x6C/+0x70/+0x74` | Action byte zero; each clock/counter a 32-bit zero. |
| `+0x124` | Complete 16-bit flags word zero. |
| `+0x128` | A 32-bit zero store. |
| `+0x12C` | Signed 32-bit `-3000`. |
| `+0x130/+0x134/+0x138/+0x13C` | Four-byte zero each. |
| `+0x140/+0x141` | Two bytes cleared together by a 16-bit zero store. |
| `+0x144..+0x168` | Four-byte zero stores at increasing four-byte offsets. This includes the triplet at `+0x160`. |
| `+0x170/+0x174` | Binary32 zero each. |
| `+0x178` | Binary32 `100.0`. |
| `+0x17C/+0x180/+0x184/+0x188` | Binary32 `1.0` each. |
| `+0x18C`, `+0x194`, `+0x1A0/+0x1A4`, `+0x1A8` | A 16-bit zero, then the listed 32-bit zeros, and byte zero at `+0x1A8`. |
| `+0x1AC/+0x1B0/+0x1B4` and `+0x1B8` | Each of the first three is 32-bit all-ones; `+0x1B8` is 32-bit zero. |
| Resident list `+0x1178` | Constructed empty with count zero. |
| `+0x1180/+0x1184/+0x1188/+0x118C/+0x1190` | Four-byte zero each. |
| `+0x1194/+0x1198` and `+0x119C` | Binary32 `1.0` at each of the first two; four-byte zero at the last. |
| `+0x11C0/+0x11C8/+0x11D0` | Each complete 64-bit identifier zero. |
| Maps `+0x139C/+0x13A4/+0x13AC` | Constructed empty; zero counts. |
| `+0x13B4`, bytes `+0x13B8/+0x13C0`, `+0x13C4` | Four-byte zero, the two byte zeros, and a 32-bit zero accumulator. |
| `+0x13F4` and collection `+0x13F8` | Full 32-bit `1`; empty candidate collection. These are new actor defaults, not inherited configuration. |

The new actor is not a byte-for-byte clone. This table does not declare every private byte or padding byte initialized. Other private constructor fields and their later consumers are outside the semantic closure of this branch. The overridden initial health default is binary32 `500.0`; the overridden initial level field `+0x190` is 32-bit `1`; the setup pointer `+0x13E4` and link `+0x198` start at zero before this branch fills them.

Evidence: image/listing `0x03B690–0x03BBF6`, `0x03C100–0x03C379`, `0x03BC00–0x03C01D`, `0x04AF00–0x04AFF0`, `0x02C5E0–0x02C675`, `0x059800–0x059995`; corresponding original constructor/copy files. The inspected container constructors perform allocation and empty-header setup without gameplay random/seed calls; exceptional allocation handling is not an ordinary-success guarantee.

## Appearance generation and random boundaries

At the null-descriptor call used by this branch, appearance producer `0x03F7C0` always consumes five imported random values before its kind-specific appearance work. The first three provide temporary byte material and the next two temporary selectors reduced by signed remainder over `100`. These are actual draws even for kinds that do not use those temporary values. The helper then writes appearance binary32 fields `+0x08/+0x0C/+0x10` to the exact bit patterns `0x3F75C290`, `0x3F75C290` and `0x400A3D71`, before kind-specific overrides.

The complete direct random-call inventory of this helper, with the third argument null, has the following extra draws after those first five. Kinds are full unsigned 32-bit identifiers. The order of sites within a row is the call order.

| Kind(s), hexadecimal | Extra draws | Original call RVAs |
| --- | --- | --- |
| `06` | 4 | `0x03FCBD`, `0x03FCD6`, `0x03FCEA`, `0x03FD0E` |
| `2E` | 3 | `0x041741`, `0x041754`, `0x04176C` |
| `3A` | 1 | `0x043389` |
| `53` | 1 | `0x0409C8` |
| `54` | 1 | `0x040A27` |
| `74` | 1 | `0x0439A6` |
| `76` | 1 | `0x0405B1` |
| `77` | 8 | `0x043AD9`, `0x043AED`, `0x043B01`, `0x043B19`, `0x043B3E`, `0x043B72`, `0x043BF7`, `0x043C2F` |
| `7C` | 1 | `0x04375C` |
| `8F` | 1 | `0x04382C` |
| `91`, `92` | 1 | `0x041B86` |
| All other identifiers | 0 | No additional direct random call on their kind route. |

The five common call sites are `0x03F80B`, `0x03F824`, `0x03F838`, `0x03F85C` and `0x03F869`. The helper’s remaining ordinary calls are small three-byte/three-component copies and the security-cookie boundary; the copies make no random calls. No seed call occurs in this appearance helper. The count and ordering inventory does not itself provide every kind’s full model, color and attachment-field outputs. Those complete outputs remain open except for the bounded fallback below.

For the empty-candidate generation fallback kind `0x25`, all five common draws still occur and there are no extra draws. Its complete kind-specific changes are: appearance 16-bit field `+0x1C` becomes `0x095F`; the three binary32 fields at `+0x64/+0x68/+0x6C` become zero; binary32 `+0x28` becomes `1.5`; fields `+0x08/+0x0C/+0x10` become the exact binary32 `0.8` pattern `0x3F4CCCCD`; and flags word `+0x06` gains `0x0029`. The later caller’s clear-`0x0200`/set-`0x0800` operation makes the new actor’s flags word `0x0829`, starting from its default appearance. The temporary random material does not change that fallback output.

For this fallback, all appearance fields not just listed keep the constructor values. Bytes `+0/+1` are zero and bytes `+0x02/+0x03/+0x04` are `0xFF`; all eight 16-bit fields at `+0x14..+0x22` start `0xFFFF` except the subsequently replaced `+0x1C`. The remaining four-byte appearance defaults are captured below as exact raw patterns, avoiding decimal-rounding ambiguity. Every listed offset is relative to the appearance object, which begins at actor `+0x78`.

| Appearance offsets | Initial four-byte pattern |
| --- | --- |
| `24` | `3F8147AE` |
| `28, 2C, 34, 40, 44, 8C` | `3F800000` |
| `30` | `3F7AE148` |
| `38` | `3F733333` |
| `3C` | `3F4CCCCD` |
| `48, 4C, 50, 54, 58, 5C, 60, 64, 68, 70, 80, 84, 94, A0, A4, A8` | `00000000` |
| `6C` | `C0A00000` |
| `74, 78, 7C, 88` | Respectively `3F000000`, `40A00000`, `40C00000`, `40400000` |
| `90, 98, 9C` | Respectively `C1280000`, `C1000000`, `40000000` |

Appearance byte `+0x05` has no established constructor store. The kind-independent three binary32 stores are subsequently overwritten by this fallback’s `0.8` fields. For unsigned kinds greater than `0x9B`, the producer’s range default returns after those kind-independent stores without extra kind-specific writes; the caller still applies its flags change. Other in-range kind outputs require the unclosed appearance relations, even though their direct random counts above are established.

Within this creation branch, successful copy alternatives consume exactly the optional candidate draw and no appearance-generation draws. Successful generation alternatives consume that same optional draw, then the five common draws, then the selected kind’s extra draws above. Constructor, semantic copies, the inspected threshold and maximum-query native stat helpers, setup-object construction and list linking add no native gameplay random/seed call. No seed call occurs between `0x2157A8` and `0x215BA6`. Earlier and later world phases, imported-library internals, allocation-failure handlers and unrelated threads are outside this local count; this is not a whole-world random-stream guarantee.

These direct calls use `rand` from `MSVCR110.dll`, at IAT RVA `0x2FC468`, confirmed from the supported image’s PE imports and operands. The image’s seed import is `srand` from that DLL at IAT RVA `0x2FC4B4`; its identity does not imply a seed operation in this branch. No random algorithm or seed lifetime is inferred from the API names.

Evidence: original image/listing `0x03F7C0–0x043C7F`, kind pointer table `0x043CA0`, `0x028750–0x0288AA`, `0x0419EC–0x041A53`, `0x215958–0x2159BC`; original files `0043f7c0.c`, `00428750.c`, `00428920.c`, `00411e30.c`.

## Attached setup objects and publication

After level assignment, current-actor maximum query and position copy, the branch builds a separate `0x0C`-byte root object through `0x0DC2D0`. Its virtual-table identifier is RVA `0x303A00`. Root `+0x04` is an initially empty pointer-list header and root `+0x08` its zero count. The root constructor creates an empty sentinel; it does not invoke any child virtual behavior.

Three separately allocated child pointers are appended to that list in this exact order:

| Child | Constructor and direct initialized fields |
| --- | --- |
| First, `0x14` bytes | Constructor `0x02C550`, virtual-table identifier `0x2FD310`; signed 32-bit `+0x04` is `20000`, binary32 `+0x08` is explicit `20.0` (`0x41A00000`), and four-byte fields `+0x0C/+0x10` are zero. No duration meaning is assigned to these numbers here. |
| Second, `0x10` bytes | Constructor `0x03AB80`, virtual-table identifier `0x2FD868`; constructor clears both halves at `+0x08/+0x0C`, after which the branch writes the current actor’s full 64-bit identifier there. No store to this child’s `+0x04` is established by that constructor or the branch. |
| Third, `0x08` bytes | Constructor `0x0DBF10`, virtual-table identifier `0x3039E8`; four-byte field `+0x04` is zero. |

Each append through `0x0860B0` creates a `0x0C`-byte list node whose data at node `+0x08` is the four-byte child pointer. It appends at the tail and updates the count. Normal completion gives three entries in the order above. The insertion helper copies the pointer; it does not execute the pointed object’s virtual update. Thus child order is established, while future child execution, removal and destruction semantics remain dependencies.

Only after the third append does the branch assign the root pointer to new actor `+0x13E4`. It then writes the current actor identifier to new actor `+0x198`, obtains the world map slot for the scan-selected key through `0x068AD0`, and stores the new actor pointer in that slot at `0x215BA6`. If the key exists, the slot helper returns its existing mapped-value storage; otherwise it inserts a key with an initially null mapped value before the branch stores the pointer. The branch does not change world `+0xB8` to the new actor.

The two explicit identifier relations are consequently aligned: second child `+0x08` and new actor `+0x198` both hold the current actor identifier, including zero. Neither store adds a nonzero test or an inequality test against the new identifier. Neither uses the selected appearance provider’s identifier unless that provider is also the current actor. No reciprocal link is written to the current actor, and this route does not publish a hit or buff record into the shared output. Earlier callback effects remain independent of that statement.

For an ordinary valid signed-key map without minimum-identifier underflow, the inserted key is strictly lower than every existing nonnull key, including the current actor’s key. The outer actor pass continues by successor from its current node and therefore does not process this newly inserted predecessor in that same pass. This conclusion uses actual ordering and successor traversal, not a frozen map snapshot. The branch’s count scan finishes before insertion, so it also does not count its own new actor; the link limit can become three after an admitted count of two.

After publication the branch rejoins the current actor’s common processing. The `+0x74` accumulation, adjacent map work and resident-record countdown/removal follow this dispatch; the later action-lock reduction and common action-clock advancement are at `0x2184C2` onward. This branch therefore creates before that later common clock advance. It contains no direct reset of the current actor’s action byte or action clock. Subsequent common processing and future input may change them. No same-call automatic control transfer to the new actor is established.

Evidence: image/listing `0x215A6B–0x215BAE`, `0x211F41–0x212163`, `0x21843C–0x218597`, `0x21FE21–0x21FE5B`; original constructors `004dc2d0.c`, `0042c550.c`, `0043ab80.c`, `004dbf10.c`, list helpers `004860b0.c`, `00583cb0.c`, `0046d550.c`, map helpers `00468ad0.c`, `004522a0.c`, `00453420.c`, and successor `005c3ea0.c`.

## Explicit limits

The closed result is the action-`92` creation boundary, its direct new-actor fields, copied/default item data, copy extents, complete direct appearance draw counts, the full empty-candidate generation fallback, setup-record construction and map publication. The complete kind-specific appearance fields for other candidates remain open; the raw kind list and `+0x13F4` producers are not reconstructed. The complete scalar value of the threshold and maximum-stat dependencies remains a separate numeric dependency even where their native random-call absence is checked.

The earlier actor-list processing, callback side effects, queued-state helpers, later common-action body, movement/physics, all world callers, future virtual setup behavior, removal and actor lifetime are not supplied by this contract. In particular the first child’s `20000` and `20.0` values do not establish an expiry duration without its consumer. Null allocations are not described as a graceful creation rejection: some explicit null-result alternatives are followed by unconditional uses. Allocation failures, exceptions, malformed containers, stale pointers, concurrent changes, floating-point exceptional states and map-key wrap collisions retain those unresolved native-runtime boundaries. No separate server behavior or general ownership/faction meaning is inferred. Existing held artifacts were neither opened nor changed.
