# Alpha client hit-event transmission and bounded local replay

Status: FINAL / STABLE / HELD. This independently worded source-side contract establishes the original client routing, copied fields, queue consumption and the local incoming boundary needed to assess source/self records. It does not certify a replacement implementation, network acceptance or server behavior.

The evidence root is `disassembled alpha cube world`. The supported image is `binary/Cube.exe`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred image base `0x00400000`. Addresses below are RVAs except named object, list or record offsets. Evidence is restricted to individually identified original client functions, the original client assembly and executable bytes. No other contract, reconstruction, replacement engine, SDK, tests, probes, captures, logs or implementation history supplies facts. No original server counterpart or server echo policy is inferred.

## Shared frame output and the outgoing source-ID filter

Client frame `0x088EE0` constructs one output aggregate at `0x08CE35`. Its hit-record list occupies the aggregate's base, with a four-byte native list-head pointer and four-byte count. The frame passes that same aggregate to its world calls. Generic-hit producer `0x196D30` appends its 72-byte records to that base list; it does not supply a separate list for source/self records.

Later in the frame, client four-byte socket field `+0x8006CC` must be nonzero to enter the direct outgoing-queue copy region. The frame walks the shared hit list from beginning to end. For each record, it compares record source identifier `+0x00/+0x04` with controlled creature identifier `+0x08/+0x0C`, using equality of both 32-bit words. Only a full 64-bit source-ID match appends a semantic copy to client hit queue `+0x8006EC`, whose count is at client `+0x8006F0`.

This filter does not read the record's target ID, amount, critical byte, category or final flags for admission. It does not compare source ID with target ID. It also has no additional world-mode-byte `+0xB4` or world distinguished-pointer `+0xB8` test in this region. The controlled creature comes from client `+0x8006D0`; this path directly dereferences it, so a nonnull valid controlled creature is a dependency, not an added null check in the filter.

Iteration advances through the original list without erasing its elements. Matching records are appended at the outgoing queue tail in their original relative order; nonmatching records remain in the shared aggregate. The native iterator operation `0x068D30` copies/advances an iterator and does not remove a list node.

Generic hit's source/self auxiliary producer fills both identifiers from the source creature and stores its negative auxiliary amount with category byte zero. Its target records use their actual target identifier. The outgoing filter makes no distinction between those producer cases. A source/self record whose source ID equals the controlled ID passes the same test as a target record with that source ID; source-equals-target is not excluded. This is an identity-routing rule, not a claim that every such record is successfully delivered.

Sources: original `binary/Cube.exe`, aggregate/world-call anchors `0x08CE2E–0x08CF49`, socket admission `0x09C02F–0x09C036`, shared-list iteration/source comparison/copy `0x09C03C–0x09C0F2`; `decompiled/client/functions/00468d30.c`, `00486290.c`, `00451ee0.c`, `004599a0.c`. Generic source/self assignment and direct health ordering are identified at `0x197996–0x197A71`; the network distinction above is established by the frame's actual comparison, without reading a prior contract. Corresponding instruction ranges are in `decompiled/client/assembly.txt`.

## What copies preserve, and what the raw send includes

Frame queue append `0x086290` and sender list-copy constructor `0x058FD0` ultimately use record copy helper `0x0599A0`. The list-copy constructor walks the source list in order through range-copy helper `0x0540C0`. It allocates independent native nodes rather than transferring the original shared or queued nodes.

| Hit-record offset | Width | Semantic copy behavior |
| --- | --- | --- |
| `+0x00` | 64-bit source identifier | Both 32-bit words copied unchanged. |
| `+0x08` | 64-bit target identifier | Both words copied unchanged. |
| `+0x10` | Binary32 amount | Its four bytes copied unchanged; no sign, critical or magnitude adjustment. |
| `+0x14` | One-byte critical field | Copied unchanged. |
| `+0x18`, `+0x1C` | Two four-byte status fields | Copied unchanged; this routing contract does not assign new signedness/meaning to every possible value. |
| `+0x20–+0x37` | 24 position bytes | Six four-byte pieces copied unchanged. |
| `+0x38`, `+0x3C`, `+0x40` | Three binary32 vector fields | Each copied unchanged. |
| `+0x44`, `+0x45`, `+0x46` | Three one-byte fields, including category at `+0x45` | Each copied unchanged. |
| `+0x15–+0x17`, `+0x47` | Four padding bytes | Not copied by the semantic copy helper. No defined value or preservation is established for them. |

Native node allocation is 80 bytes: two four-byte list links followed by this 72-byte payload. Neither list link is sent as part of the hit payload.

The sender does not serialize these semantic fields one by one. For each queued hit snapshot it requests one four-byte send containing numeric message kind 7, followed by one send of **72 contiguous bytes** beginning at node payload `+0x08`. Consequently the raw outgoing request includes the payload's padding bytes, even though earlier semantic copies do not preserve or establish those bytes. Padding is not a defined gameplay field and must not be described as guaranteed zero or as an exact copy of the producer's original stack padding.

There is no target/source identity recheck, source-equals-target exclusion or category distinction in this sender loop. Its low-level send calls test return against -1; this loop does not retry a positive short write until 72 bytes have been completed. These facts describe requested bytes and original caller checks, not a delivery guarantee or an inferred TCP message boundary.

Sources: `decompiled/client/functions/00486290.c`, `00458fd0.c`, `004540c0.c`, `00451ee0.c`, `004599a0.c`, `0046d810.c`, `00469c10.c`; original list-copy constructor `0x058FD0–0x05904C`, send loop `0x06A0ED–0x06A151`, send thunk `0x22723A`. Payload pointer and requested size are explicit at `0x06A12B–0x06A13B`.

## Sender queue consumption and state-message order

Original sender worker `0x069C10` uses client active byte `+0x800585` and socket field `+0x8006CC`. Its normal batch begins by requesting a four-byte numeric kind-0 message header. It then acquires the world lock on the world embedded at client `+0x2E4`.

Under that lock, it first makes a semantic list copy of client hit queue `+0x8006EC` into a local hit list. It also copies adjacent queues of other record kinds; those record formats are outside this contract. After additional native queue work, it resets the client hit-list head links, sets count `+0x8006F0` to zero, and deletes the former queued nodes. That consumption happens before the hit records are transmitted and does not wait for per-hit acknowledgment. The local copied list remains the source of the later kind-7 sends.

Still under the same lock, and after clearing those outgoing queues, the worker reads the current controlled creature identifier and copies current controlled state beginning at creature `+0x10` through helper `0x066050`. That state copy includes creature binary32 health `+0x16C`, represented at snapshot-relative `+0x15C`. It then releases the world lock. The copied state is current at that later capture; it is not a per-event pre-effect snapshot.

The worker constructs a state message using the captured identifier and native state encoder `0x0CDAA0`, with prior/current snapshot data and its native first-message flag. The encoder includes the snapshot-health field among its considered fields. Full delta-field selection and encoding are not reconstructed here, so this does not assert that health is explicitly present in every state payload.

After the already-requested kind-0 header, the worker requests a four-byte encoded-state length and then that state payload. On the normal successful path, it updates its previous-state copy after those state sends. Optional text and action messages can follow before the hit list. Only then does it traverse the local hit list in order, sending kind 7 plus each raw 72-byte payload. Therefore the visible state-send requests precede this batch's hit-send requests; optional intervening messages do not reverse that order.

For a locally produced auxiliary self record, the producer's direct source-health addition precedes the later frame queue-copy site. The frame's world-lock acquisition at `0x08902F` and later release at `0x09D2EA` bracket this normal frame path; the sender's copy/clear/state capture uses that same embedded world lock. A sender batch containing that queued record therefore captures current controlled state after the frame's direct effect and queue insertion have occurred. Later native changes before capture can affect that state; the contract does not equate captured health with one isolated addition.

The shared frame aggregate survives the direct source-ID filter. It is destroyed later by the frame at `0x09D365`; native aggregate cleanup eventually empties its base hit list, deletes those remaining nodes and deletes its sentinel. The outgoing queue and sender snapshot have their own copies, so that aggregate cleanup does not revoke their already-copied records. No retry/requeue guarantee is established for a later send failure after the client queue was cleared.

Sources: `decompiled/client/functions/00469c10.c`, `00458fd0.c`, `00466050.c`, `004cdaa0.c`; original worker lock/copy `0x069CDB–0x069D2B`, client hit-queue consumption `0x069E3B–0x069E77`, controlled ID/state capture and unlock `0x069EF5–0x069F26`, state encoding/length/content requests `0x069F46–0x069FED`, previous-state update `0x069FF6–0x06A00A`, later hit sends `0x06A0ED–0x06A151`. Frame lock/release/cleanup anchors are `0x08902F`, `0x09D2EA`, `0x09D35E–0x09D365`; aggregate base-list cleanup is native `0x067ED0–0x067F0D`, reached from cleanup entry `0x067C70`. Original cleanup continuation is used because the short function export alone does not show all member cleanup.

## Separate optional recipient distribution in the client image

After the direct outgoing-queue copy region, the frame separately tests client pointer `+0x8006C8`. When nonnull, it calls `0x0DC8E0` with the same shared aggregate. This is an additional route; it is not the source-ID comparison above and is not proof of behavior in `Server.exe`.

The original client-image routine locks the world pointer held at object `+0x04` and walks its recipient-pointer array bounded by object `+0x28/+0x2C`. For each nonnull recipient pointer it copies the **entire** shared hit list into that recipient's native hit queue at recipient `+0x18`, using the same ordered semantic range-copy helper. That hit copy precedes the routine's later recipient-creature lookup and distance tests for other lists. No source-ID, target-ID, source-equals-target, category, amount, or distance test filters this hit-list copy.

Thus shared records rejected by the direct client-source filter can still enter this optional recipient-copy route if that object and recipient are present. The shared list is copied rather than consumed by this operation. The subsequent transport worker for these recipient queues, exact recipient lifecycle and any later filtering are not closed here. No conclusion about server broadcast, exclusion of an originating peer or echo delivery is supplied from this partial client-image route.

Sources: `decompiled/client/functions/004dc8e0.c`, `004540c0.c`; original frame pointer gate/call `0x09C256–0x09C26D`, object lock and recipient admission `0x0DC916–0x0DC955`, complete hit-range copy `0x0DC97A–0x0DC9A2`, subsequent recipient lookup `0x0DC9A7–0x0DC9DE`. This document distinguishes that native client-image routine from an uninspected server counterpart.

## Incoming client records and conditional source/self replay

The matching incoming client boundary uses a different framing path from outgoing single-record kind 7. Receive worker `0x06B740` dispatches incoming kind 4 to the original region beginning at `0x06CC0E`. After obtaining its sized byte buffer, it constructs an aggregate and invokes decoder `0x0583F0` for the aggregate's hit component. That decoder reads a four-byte signed count, and for a positive count reads each raw 72-byte record in order and appends a semantic copy. There is no source/target identity filter or self-ID exclusion in that decoded hit-list construction. Malformed counts, truncated buffers and all decoder failure cases are not certified here.

Under the embedded world lock, the receiver appends semantic copies of those decoded hits to client incoming hit list `+0x800630`, preserving their order. The receiver does not directly perform the local health operation in this block. The temporary decoded aggregate is later destroyed independently of that client-list copy.

Later frame processing walks `+0x800630` and resolves each record's target identifier at record `+0x08` through the world. Its local actor-effect branch requires that resolved pointer equal client controlled-creature pointer `+0x8006D0`. It does **not** require a foreign source, compare source and target IDs, or distinguish a generic auxiliary self producer. Other categories have separate local operations which are not reconstructed here.

For an incoming category-0 record with a finite negative amount, the relevant local branch requires controlled binary32 health to be ordered strictly positive. It subtracts the stored amount directly from that health, which increases health for the negative amount. It replaces an ordered negative resulting health with positive zero, then compares against the native health-limit helper `0x044DB0`; a result strictly above the limit is replaced by a second call's returned limit, while equality is retained. The complete limit formula is outside scope. A zero duration at record `+0x18` does not set a positive stun duration. This category-0 health operation does not multiply the amount again from the record's critical byte.

A distinct subsequent pass walks the same incoming hit list and copies into the shared frame output list only records whose full source ID **differs** from the controlled creature's ID. Local-source records are excluded from that forwarding pass even if their target was the controlled creature and the local actor-effect branch already ran. The frame then clears the incoming list through `0x06F870` at `0x0905C4`. Application and forwarding are separate decisions, followed by consumption of the incoming queue.

Consequently, **if** an emitted negative category-0 self record is actually received through this incoming path with both IDs still naming the controlled creature, and controlled health is ordered positive, the audited local branch can apply its negative amount as a further health increase with the stated cap. Its local source ID prevents forwarding by the subsequent source-ID pass; it does not suppress the earlier local health operation. The client evidence alone does not establish that the original server echoes this record, which recipients receive it, or how server state interacts with the earlier outgoing state snapshot. An unconditional double-application or no-replay claim is therefore not established.

Sources: original receive dispatch `0x06B8A7–0x06B8B7`, table at image RVAs `0x06D180/0x06D1A4`, kind-4 buffer/aggregate/hit decode `0x06CC0E–0x06CD02`, locked incoming-hit copy `0x06CEE2–0x06CF26`, receiver unlock/temporary cleanup `0x06D080–0x06D093`; `decompiled/client/functions/004583f0.c`, `00457b80.c`, `004540c0.c`, `0046f870.c`. Frame target-resolution gate is `0x08FF10–0x08FF37`; category dispatch and ordinary local-health admission are `0x08FF3D–0x08FF7F`, `0x090081–0x090089`, `0x0901D0–0x0901EF`; zero/nonpositive duration bypass and direct health/limit are `0x090277–0x090304`; separate source-ID forwarding is `0x090409–0x0904AE`; incoming queue clear is `0x0905BD–0x0905C4`. No broad generic resolver reconstruction supplies additional claims.

The verified copies, routing comparisons, requested wire bytes and local replay conditions above do not close network delivery/retry, every packet component, complete snapshot encoding, all category consumers, recipient-worker transport, concurrent identity changes or server-side validation/echo. Those remain explicit dependencies rather than inferred parity behavior.
