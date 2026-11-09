# Alpha hit-response vector consumers and creature-record mapping

This original-only, code-free contract follows the three binary32 creature fields at `+0x4c/+0x50/+0x54` from hit application into the client world's movement work and native creature-state record codec. The supported image is `binary/Cube.exe`, SHA256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, image base `0x00400000`. All routine addresses below are RVAs. Evidence paths are relative to the original Alpha evidence root. Earlier held artifacts remained unopened and unchanged; no replacement code, tests, probes, captures, logs, implementation history, SDK or reconstruction supplied facts.

“Response” is a descriptive name for the hit-written vector, not an asserted original source identifier. The proven distinction is operational: it contributes additively alongside velocity when an initial displacement is assembled, retains its own storage, has separate relaxation and clearing operations, and occupies a separate field in the native creature-state record. The evidence does not support collapsing it into velocity or into the movement-input vector.

Storage and independent roles

The creature's state subobject begins at creature `+0x10`. This adjustment is explicit in the constructor and in the outgoing local-creature snapshot. Consequently state-relative offsets and creature-relative offsets differ by sixteen bytes. In particular state `+0x3c` is creature `+0x4c`; a state codec reference to `+0x4c` describes a different field.

| Quantity | Creature-relative storage | State-relative storage | Proven role in this trace |
| --- | --- | --- | --- |
| Position | Three signed 64-bit components at `+0x10/+0x18/+0x20` | `+0/+8/+0x10` | Position used by the movement/collision pass |
| Rotation vector | Three binary32 components at `+0x28/+0x2c/+0x30` | `+0x18/+0x1c/+0x20` | Separate rotation storage and record field |
| Velocity | Three binary32 components at `+0x34/+0x38/+0x3c` | `+0x24/+0x28/+0x2c` | Independently updated velocity, later added to response |
| Movement input/acceleration vector | Three binary32 components at `+0x40/+0x44/+0x48` | `+0x30/+0x34/+0x38` | Independently supplied movement vector; time-scaled in earlier movement preparation and tested when selecting response relaxation |
| Hit response | Three binary32 components at `+0x4c/+0x50/+0x54` | `+0x3c/+0x40/+0x44` | Separately retained additive contribution to initial displacement |

The movement input's complete action-dependent conversion into velocity is not specified here. Its distinct storage, its multiplication by the elapsed-time scale in movement preparation, and its later influence on the response relaxation coefficient are established. This is enough to distinguish it from the response without assigning every input branch a gameplay name.

Original evidence: `decompiled/client/functions/0043b690.c`, RVA `0x03b690`, with explicit state-subobject adjustment in `decompiled/client/assembly.txt` at `0x03b6cb–0x03b6d1`; state initialization in `decompiled/client/functions/0043c100.c`, RVA `0x03c100`; local snapshot adjustment at `0x069ef5–0x069f19`; movement-input preparation at `0x218f44–0x218f9b`; response/velocity addition at `0x21aea0–0x21aedb`; record field sequence in `decompiled/client/functions/004cdaa0.c`, RVA `0x0cdaa0`.

Hit-written starting state

The hit applier at RVA `0x195a60` reads the incoming response candidate from hit `+0x38/+0x3c/+0x40`. In its admitted non-category-3, nonnegative-amount vector branch, a positive creature timer `+0x12c` combined with nonpositive signed hit `+0x18` attenuates each candidate component by binary32 `0x3dcccccd`, approximately 0.1. Positive-hit cancellation of action `0x54` can already have assigned timer 3000 before this test.

The candidate replaces the three response components only when the creature role byte `+0x60` is not 6, the vector ownership condition succeeds, and the candidate's squared length is at least the old response's squared length. That ownership condition is satisfied by a mode-zero world and a nonzero-role creature, or by the creature pointer equaling world-local pointer `+0xb8`. Squared lengths use binary32 component squares, first combine the first two squares, then add the third. Equality admits replacement. The write replaces the vector; it neither adds the candidate to the old response nor adds it directly to velocity.

The following positive-hit-`+0x18` operation clears velocity `+0x34/+0x38/+0x3c`, independently of whether the response replacement succeeded. This establishes an immediate observable distinction: the same hit application can zero velocity while retaining or replacing a nonzero response. Full target/source/protection admission and the other category mutations are outside this consumer contract; the conditions just described apply after those earlier gates admit the vector branch.

Original evidence: `decompiled/client/functions/00595a60.c`, RVA `0x195a60`; `decompiled/client/assembly.txt`, attenuation at `0x195d7a–0x195dbd`, response ownership/comparison/writes at `0x195dbd–0x195e36`, and separate velocity reset at `0x195e36–0x195e51`. The native attenuation constant is `binary/Cube.exe` RVA `0x2fcb7c`.

Update interval and scope of movement admission

The world update routine at RVA `0x20c510` receives a signed 32-bit elapsed argument. It retains that integer for timer subtraction and the repetition count used by the relaxation helper. Separately it converts the integer to binary32 and multiplies by binary32 `0x3a83126f`, approximately 0.001, producing the movement time scale. This multiplication is binary32; it is not an exact real-number division by 1000.

The traced client caller in RVA `0x088ee0` splits a positive elapsed argument exceeding 20 into repeated calls with integer 20 and a final call with the remaining interval, which is in 1 through 20. An interval at most 20 takes the final call directly. This describes the admitted call site; it does not establish all outer frame-clock, pause, connection, or world-selection conditions. That caller has preceding world/state checks whose complete meaning is outside this contract. An arbitrary direct call with zero or negative elapsed is not assigned normal forward-motion behavior here.

Within the update's creature traversal, null mapped creatures are skipped. The routine caps health against its maximum and skips the remaining per-creature body when finite health is nonpositive at the ensuing health gate. The response consumers below are in the later movement body, after action and resident-effect work. This contract does not re-specify every queued-operation, action-dispatch or resident-effect branch before that body. It is a bounded consumer contract for execution reaching that body, not a proof that every living creature reaches it on every outer frame.

At the beginning of the identified movement body, creature signed timer `+0x12c` loses the integer elapsed argument. A still-positive timer sets action time `+0x6c` and action byte `+0x68` to zero. This timer update does not clear the response. The ensuing response addition itself has no additional role-zero, role-6, mode, or local-pointer test. Role/mode/local conditions surrounding other movement operations must not be substituted for an absent check at that addition.

Original evidence: `decompiled/client/assembly.txt`, argument retention at `0x20c549–0x20c54f`, binary32 interval conversion at `0x20c83d–0x20c862`, map traversal and null skip at `0x210493–0x2104e6`, health cap/admission at `0x211a96–0x211adb`, movement-body timer/action work at `0x2184c2–0x2184e8`, and caller interval split at `0x08cef4–0x08cf49`. The time-scale constant is `binary/Cube.exe` RVA `0x2fd870`.

Response contribution before collision and positional consumption

At RVA `0x21aea0`, the movement body reads current velocity and current response, adds corresponding components in binary32, then multiplies each sum by the already calculated movement time scale in binary32. It copies those three products into the initial displacement vector. Thus the order is component addition first, elapsed scaling second. The response is not multiplied into velocity storage, and the addition does not clear response storage.

This occurs after earlier input/action/gravity preparation of velocity, so the velocity used here is its current value at this point. Later relaxation is not retroactively used for this initial displacement. Creature-interaction work can alter the temporary displacement after it is assembled. In particular the ensuing candidate-creature interaction traversal is skipped for role 6 and actions `0x53` or `0x54`; those skips are after the response has already contributed to the temporary displacement. The internals of that interaction traversal are outside this contract.

The displacement then enters native subdivision and axis movement. The traced subdivision computes a binary32 squared length with the first two squares added before the third, promotes it to binary64 for the square-root helper, and converts the result to binary32. It adds binary32 1, truncates that sum toward zero to signed 32-bit, converts the resulting count back to binary32, takes binary32 1 divided by that count, and multiplies every displacement component by that reciprocal. For finite nonnegative lengths in representable range the count is the integer part of length plus one. This count depends on the displacement after the preceding interaction work, not solely on the raw hit vector.

For each admitted subdivision, components are processed in index order 0, 1, 2. The candidate position component is advanced by the corresponding step component converted to the native fixed-position scale. The scalar conversion first multiplies the binary32 step by exactly 65536 in binary32 and then converts the finite in-range result to a signed 64-bit integer with truncation toward zero. Position addition is a 64-bit component addition. Collision branches can subsequently reverse or adjust that candidate movement. The unblocked axis path does not merge response into velocity; it simply retains the positional advance.

The subdivision count is spatial, while the response relaxation count described below is the original integer elapsed argument. These are separate loops. A single aggregate update cannot generally be replaced by adding the full unrounded displacement once: each axis step has its own fixed-position conversion, and the intervening collision work can change the outcome.

The routine calls an additional motion/environment helper at RVA `0x20ab30` before axis subdivision only when world mode is zero or this is the world-local creature, and the creature role is not 6. This is an ownership gate for that helper, not for the earlier response-plus-velocity calculation or a general proof that all subsequent motion is restricted to the local pointer. The helper's full environment/collision behavior is unresolved here.

Original evidence: `decompiled/client/assembly.txt` and `binary/Cube.exe`, addition/time-scale/copy at `0x21aea0–0x21aedb`; vector addition helper `decompiled/client/functions/00412280.c`, RVA `0x012280`; component scaling helper `decompiled/client/functions/00451510.c`, RVA `0x051510`; copy helper `decompiled/client/functions/00411e30.c`, RVA `0x011e30`; role/action interaction delimiter at `0x21aee0–0x21aef7`; subdivision and environment-helper gate at `0x21b3f5–0x21b494`; axis candidate addition at `0x21b4c0–0x21b51b`; subdivision/axis advance at `0x21cc78–0x21cca1`. Supporting arithmetic helpers are `decompiled/client/functions/00423f20.c`, RVA `0x023f20`; `00412780.c`, RVA `0x012780`; `0042c580.c`, RVA `0x02c580`; `0042c9b0.c`, RVA `0x02c9b0`; and `0068d946.c`, RVA `0x28d946`, with conversion instructions confirmed at `0x02c583–0x02c59b` and `0x28d946–0x28d9ba`. Fixed-position scale 65536 is image RVA `0x2fce7c`.

Vertical contact clears one response component

In the traced axis collision path, a detected collision during component index 2 reaches RVA `0x21c41d`. After the optional falling/contact side-effect branches, the routine reverses that attempted position-component advance, writes velocity component 2 to zero, and separately writes response component 2 to zero. The response address used for this second write is the response pointer retained when the initial displacement was assembled. It is therefore creature `+0x54`, not a temporary copy or velocity alias.

The side-effect tests include world-local conditions for an optional health/contact effect, but their failure still joins the position reversal and both zero writes. Consequently those optional local-only effects are not prerequisites for clearing response Z once this collision path has been reached. The writes do not clear response X or Y. This contract does not generalize the Z clear into an all-axis clear or claim that every possible terrain, water, step, or creature-contact branch reaches it.

Original evidence: `decompiled/client/assembly.txt`, axis/collision selection at `0x21c13e–0x21c161`, optional effects and joins at `0x21c161–0x21c41d`, position reversal and distinct velocity/response writes at `0x21c41d–0x21c47a`; original `binary/Cube.exe` bytes at `0x21c459–0x21c47a`; component-address helper `decompiled/client/functions/00428970.c`, RVA `0x028970`; position subtraction helper `0042ca00.c`, RVA `0x02ca00`.

Response relaxation after movement

Later in the same movement pass, after subdivision/collision and further movement-state rebuilding, the routine chooses a relaxation coefficient. The default is binary32 `0x3ba3d70a`, approximately 0.005. If the current movement input/acceleration vector at `+0x40/+0x44/+0x48` has a binary32 squared length strictly greater than zero, it chooses binary32 `0x3b23d70a`, approximately 0.0025 instead. The squared-length test uses no positive epsilon. The coefficient is chosen once at this point, not independently for each response component.

The response relaxation call is made if either of the two low bits represented by mask `0x03` in creature byte `+0x5c` is set, or bit `0x10` in creature byte `+0x124` is set, or the container at creature `+0x1460` is nonempty. If both flag tests are clear and that container is empty, this call is skipped. Emptiness here is the container's 32-bit count at `+4` equal to zero, hence creature count `+0x1464`. These tests use the state then present, after the intervening movement/collision work; the contract does not replace them with named ground/air states.

When admitted, RVA `0x205950` relaxes the response toward a three-component zero target. For each positive unit of the original signed integer elapsed argument, it subtracts the current component from zero, multiplies that binary32 difference by the coefficient, and adds the binary32 product to the old component. All three differences are prepared from the iteration's current response before the stored component updates. The native writes then store X, Y and Z in that order. The next repetition uses the newly stored vector. A nonpositive repetition count performs no iterations.

There is no response-length threshold, explicit clamp to zero, normalization or single power operation in this helper. In particular “multiply by one minus the coefficient” is an algebraic description that omits the actual subtraction/multiplication/addition rounding order. For ordinary finite fields, positive coefficients below one reduce magnitudes, but the contract does not promise exact zero after a particular elapsed duration or treat underflow handling as a separately established global floating-point setting.

The response relaxation gate differs from the immediately preceding velocity relaxation gates. The same helper is also used on velocity in other branches, including a preceding flag/action-dependent call near RVA `0x21d73d`; that shared helper does not make the two stored vectors interchangeable. This response call occurs after its contribution to initial displacement and after any traced Z clear.

Original evidence: `decompiled/client/assembly.txt` and `binary/Cube.exe`, coefficient selection at `0x21d19e–0x21d1df`, response gate and call at `0x21d74d–0x21d7ac`; `decompiled/client/functions/00605950.c`, RVA `0x205950`, with instruction-level operation/write order at `0x205980–0x205a07`; `decompiled/client/functions/00424860.c`, RVA `0x024860`, squared length; `decompiled/client/functions/00477230.c`, RVA `0x077230`, count-based emptiness. Coefficient constants are original image RVAs `0x2fcd9c` and `0x31dce8`.

Other traced all-component clears

The constructor initializes the state-relative response triplet to positive zero. Both the state-reset helper at RVA `0x0465c0` and the narrower state-reset helper at RVA `0x047270` also zero state `+0x3c/+0x40/+0x44`. Their creature-facing wrappers pass creature `+0x10`, so these are all-component response clears at creature `+0x4c/+0x50/+0x54`, alongside separate velocity and movement-input clears. Full lifecycle scheduling of these reset wrappers is outside this contract.

The steering routine at RVA `0x1a7eb0` has another explicit all-component response clear at its successful tail. That tail follows its velocity and movement-input changes, clears response X, Y and Z, and returns low byte 1. Early exits earlier in the steering routine can bypass the tail, so merely calling the routine is not proof that response is cleared.

One traced caller in the world update admits that steering call in mode zero when the creature's `+0x1460` container is nonempty and timer `+0x12c` is nonpositive. Its integer elapsed argument is passed through. This call precedes the later movement-body response consumption, so a successful steering tail can remove the stored response before that same update assembles displacement. The routine also has a separate caller inside client routine RVA `0x088ee0`; this contract does not close that caller's full UI/navigation admission. Neither caller is silently generalized into an unconditional per-tick response clear.

Original evidence: `decompiled/client/functions/0043c100.c`, RVA `0x03c100`; `004465c0.c`, RVA `0x0465c0`; `00447270.c`, RVA `0x047270`; creature wrapper adjustments in `decompiled/client/assembly.txt` at `0x046338–0x04633d` and `0x047113–0x047134`; steering body `decompiled/client/functions/005a7eb0.c`, RVA `0x1a7eb0`, and original tail bytes at `0x1a9c5f–0x1a9c74`; world steering admission/call at `0x211bc7–0x211c37`; second call site RVA `0x09816f`.

Native creature-state record mapping

The native record codec processes a 64-bit presence mask followed by enabled fields in a fixed logical field order. The first five fields establish the relevant mapping:

| Zero-based presence-bit index | State-relative field | Creature-relative equivalent | Enabled payload |
| --- | --- | --- | --- |
| 0 | `+0` | Position `+0x10` | Three 64-bit position components, 24 bytes |
| 1 | `+0x18` | Rotation `+0x28` | Three binary32 components, 12 bytes |
| 2 | `+0x24` | Velocity `+0x34` | Three binary32 components, 12 bytes |
| 3 | `+0x30` | Movement input/acceleration `+0x40` | Three binary32 components, 12 bytes |
| 4 | `+0x3c` | Response `+0x4c` | Three binary32 components, 12 bytes |

Response is therefore presence mask bit 4, numeric mask `0x10` in the low 32 bits. This is a record field index, not a fixed wire byte offset: preceding optional fields change the response payload's location. Its payload is the three stored 32-bit component representations in X/Y/Z order, copied as twelve bytes. The codec performs no response quantization, normalization, combination with velocity, source-ID insertion or sign change.

The writer at RVA `0x0cdaa0` supplies old and current state to the three-float field helper at RVA `0x0cbbc0`. Without the force flag, it omits the field only when every old/current component compares equal as a floating-point value. Any unequal component enables bit 4 and includes all twelve current bytes. There is no epsilon or per-component presence mask. For ordinary finite values this is exact numerical equality; equality of positive and negative zero can omit a changed zero bit pattern. With the force flag, this comparison is bypassed and the response field is included. The bit index advances even when a field is omitted.

The writer finally copies the 64-bit mask, then the enabled payload spans in field order. The local-creature sender at RVA `0x069c10` snapshots creature `+0x10`, prefixes the creature's own 64-bit resident ID, and invokes this codec with previous/current state. Its first traced force flag is set and is cleared after the successful send path, which then copies the current snapshot into the previous-state storage. Thus response has a normal outgoing creature-record path; it is not merely an internal movement temporary.

After assembling that record, this sender passes the aggregate buffer through RVA `0x049420`, which invokes RVA `0x1fc0d0` and an imported ordinal before the buffer is sent with a length. This contract's raw field mapping concerns the assembled creature record before that aggregate transformation. The imported transformation, full transport framing, send-failure/retry behavior, server authorization, and remote-creature admission are not re-specified here.

Original evidence: `decompiled/client/functions/004cdaa0.c`, RVA `0x0cdaa0`; field helper `004cbbc0.c`, RVA `0x0cbbc0`; span creation `004cb5a0.c`, RVA `0x0cb5a0`; state snapshot/copy `00466050.c`, RVA `0x066050`, and `0044b040.c`, RVA `0x04b040`; local sender `00469c10.c`, RVA `0x069c10`, with explicit original instruction arguments at `0x069ef5–0x069f81`; aggregate transform `00449420.c`, RVA `0x049420`, and `005fc0d0.c`, RVA `0x1fc0d0`. The codec's offset sequence also appears independently in its matching readers below.

Matching record readers and bounded application claim

The memory-buffer reader at RVA `0x0ccfa0` reads the 64-bit mask and walks the same field sequence. Its response call is the fifth logical field, passing state `+0x3c` to the twelve-byte field reader at RVA `0x0cc540`. With bit 4 absent, that reader leaves the existing response unchanged and advances the logical index. With bit 4 present and twelve bytes available, it directly replaces those twelve state bytes and advances the buffer cursor by twelve. It applies no hit ownership check, vector-length comparison, attenuation or relaxation at this codec level.

The socket-state reader at RVA `0x0cd3e0` has the same state-relative response destination and field index. Its enabled three-float reader at RVA `0x0cc4c0` repeatedly receives until the twelve-byte field is filled or the socket read terminates. These routines establish the native response field mapping in both reading interfaces. They do not by themselves establish which received creature records are accepted by a particular client/server handler or which object ultimately receives a decoded temporary. That handler-level admission/application chain remains outside the closed trace.

Malformed/truncated handling is not a successful-record contract. In the buffer helper, insufficient enabled-field bytes moves the cursor to the end without copying that whole field, while still advancing the field index and returning the enabled field's nominal width. The socket helper can already have written a partial field before a later read terminates. Those facts must not be converted into an invented atomic failure/rollback guarantee. Complete malformed-mask, decompression, allocation and connection recovery are unresolved here.

Original evidence: `decompiled/client/functions/004ccfa0.c`, RVA `0x0ccfa0`; `004cc540.c`, RVA `0x0cc540`; `004cd3e0.c`, RVA `0x0cd3e0`; `004cc4c0.c`, RVA `0x0cc4c0`. State-subobject correspondence is separately proved by the constructor and local sender cited above.

Limits and held result

The closed result is the distinct response storage, hit replacement origin, response-plus-velocity initial displacement order, spatial subdivision/conversion stages, one explicit Z-clear path, gated per-integer-unit response relaxation, identified all-component resets/steering tail, and presence-bit-4 native record mapping. Original image bytes confirm the cited addition, coefficient selection, relaxation gate and clear instructions; no reconstruction fills missing control-flow exports.

This contract is not a complete character-physics model. Action-specific acceleration/speed rules, the candidate-creature interaction formulas between initial displacement and subdivision, full environment helper RVA `0x20ab30`, every collision/step/water response branch, remote interpolation, complete outer update scheduling, imported aggregate transformation, receiver-handler ownership and fatal behavior remain outside its bounds. It does not assert that the listed writes are the only writes anywhere in the image. Numeric descriptions assume well-formed objects, ordinary finite fields, positive ordinary elapsed intervals and representable integer conversions; exceptional arithmetic, concurrent mutation and malformed structures have no added recovery behavior.

Status: FINAL / STABLE / HELD after full prose readback. Earlier held artifacts remain unopened and unchanged.
