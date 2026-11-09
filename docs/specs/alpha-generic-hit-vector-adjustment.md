# Alpha generic-hit vector adjustment

This original-only, code-free contract covers the common vector block at client RVAs `0x197e5e–0x197fa8` within the generic hit producer at RVA `0x196d30`, including its null-source join and the subsequent initial main-record vector copy. The supported image is `binary/Cube.exe`, SHA256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, image base `0x00400000`. All addresses below are RVAs; evidence paths are relative to the original Alpha evidence root. Earlier held artifacts remained unopened and unchanged. No replacement code, history, tests, probes, captures, SDK, reconstruction or other contract supplied facts.

The consequential result is that stable ordinary finite inputs bypass the vector-normalization/write branch. The block first selects the larger of target velocity's squared length and the supplied vector's squared length, then requires a freshly recomputed target-velocity squared length to be strictly greater than that selected value. The comparison directions are confirmed in the original image; no intended clamp or alternate comparison is substituted for them.

Inputs, join and distinct destinations

The routine receives its world implicitly and its target as explicit stack argument 1, at the entry-frame position `+8`. Explicit stack argument 2, at `+0x0c`, is the source pointer. Explicit stack argument 8, at `+0x24`, points to the supplied three-component binary32 vector. These argument numbers exclude the implicit world. The target pointer remains the base for the creature-field accesses described here.

The null-source path joins the common block at `0x197e5e` through `0x197e58`; another continuation can join through `0x197e5a`. The vector block itself does not dereference the source pointer or add a source-present gate. Earlier target/world/action admission and the null-source path's preceding scalar/random work are outside this vector contract.

| Storage | Role in this bounded block |
| --- | --- |
| Target `+0x34/+0x38/+0x3c` | Velocity used for the squared-length comparisons and conditional normalization; the only creature vector conditionally written by this block |
| Target `+0x4c/+0x50/+0x54` | Stored response vector; no direct access through these target-relative offsets in this block |
| Three binary32 values addressed by explicit argument 8 | Supplied vector; read for its squared length, then freshly read again for the initial main-record copy |
| Stack squared-length and component temporaries | Intermediate selected squared length, normalized components and scaled component copies; separate from the main hit record |

There is no direct store through the supplied-vector pointer in this block. A caller-created alias between that pointer and target velocity would share storage with the explicit velocity destination. If argument 8 points to stored response, the supplied-vector reads can read that response indirectly; this does not introduce a response write. Independent pointee ownership or alias restrictions are not proved here.

Original evidence: `decompiled/client/assembly.txt`, argument bases at `0x196d6f–0x196d95`, source-null selection at `0x196db4–0x196db6`, join at `0x197e3b–0x197e5e`, supplied-vector pointer load at `0x197e68`, and creature destinations at `0x197f7e–0x197fa3`; `decompiled/client/functions/00596d30.c`, RVA `0x196d30`, as supporting original decompilation. Register/argument and arithmetic conclusions use the original instructions rather than the decompiler's inferred signature.

Length selection and actual branch gate

First the block loads target velocity's Y, X and Z components and forms each component square with binary32 multiplication. It adds the X square and Y square in binary32, then adds the Z square in binary32. This completed binary32 velocity squared length is retained and copied into a stack temporary.

The supplied vector's X, Y and Z components are independently loaded and squared in binary32. Its X and Y squares are added first, followed by its Z square, again with binary32 results. If that supplied squared length is strictly greater than the initially computed velocity squared length, the supplied result replaces the retained/stack squared-length value. Otherwise the initial velocity result remains selected; equality does not select the supplied result.

Next the block reloads target velocity's Y, X and Z components and recomputes its squared length with binary32 component squares, X-square-plus-Y-square first, then Z square. Only when this recomputed squared length is strictly greater than the selected squared length does execution continue into normalization. Less-than and equality skip directly to main-record construction at `0x197fa8`.

For unchanged finite velocity components and finite squared-length intermediates, the initial and recomputed velocity squared lengths are identical operations on identical operands. The selected value is at least that value. The strict-greater gate consequently fails whether the supplied vector is smaller, equal or larger. This conclusion also covers an unchanged zero velocity and zero supplied vector. No square-root, reciprocal or velocity write is executed on that ordinary stable path.

There is no separate test for a nonzero vector, a positive length, an epsilon, the hit amount's sign, or a positive status timer in this scoped block. A zero vector does not cause an unconditional normalization. The only admission into the arithmetic tail is the strict squared-length comparison just described. Changing reads, exceptional arithmetic or other conditions outside the stable finite bounds are not silently treated as ordinary successful inputs.

Original evidence: `decompiled/client/assembly.txt` and original `binary/Cube.exe` bytes, initial velocity squared length at `0x197e5e–0x197e91`, supplied squared length and retained-value selection at `0x197e70–0x197eb4`, repeated velocity squared length and branch at `0x197eba–0x197ee0`. The first comparison skips replacement at `0x197eaf`; the second comparison skips normalization at `0x197ee0`. Both original branches were independently checked against the image bytes.

Conditional arithmetic tail, if its gate is reached

The following describes the actual tail's operations without asserting that stable finite inputs reach it.

1. Target velocity is loaded yet again. Its Y and X component squares are formed in binary32 and added in Y-square-plus-X-square order; the binary32 Z square is then added. This completed binary32 squared length is promoted to binary64 for the imported precise square-root call. The returned binary64 square root is narrowed to binary32.
2. Binary32 exactly 1 is divided by that narrowed square root, producing a binary32 reciprocal. The reciprocal is multiplied separately by freshly loaded target velocity X, Y and Z, each multiplication yielding binary32. These three products are held as normalized-component temporaries. There is no binary64 component division and no direct mutation of the supplied vector.
3. The previously selected binary32 squared length is loaded from its stack temporary, promoted to binary64, and passed to a second precise square-root call. This second returned binary64 result is narrowed to binary32. It is multiplied separately by each normalized-component temporary, producing three binary32 scaled components. These products are all prepared before the following creature writes.
4. The scaled X component is stored to target velocity `+0x34`. Copies of scaled X, Y and Z are then stored to a separate three-component local scratch area. Target velocity Y at `+0x38` is written next, followed by target velocity Z at `+0x3c`. Thus the creature writes occur in X/Y/Z order, with the scratch copies between X and Y. These are replacements, not additions to the old velocity.

The selected squared-length temporary is at the native stack offset `+0x14` during this block. The normalized-component temporaries are at stack offsets `+0x10/+0x24/+0x28`; the final three-component scratch copy is at `+0x1c8/+0x1cc/+0x1d0`. These offsets identify original temporary storage at the cited instructions; they do not name replacement structures. The final scratch copy is not the main hit record's vector and is not copied back through argument 8 by this block.

The exactly-one constant is original image RVA `0x345dc0`, binary32 bit pattern `0x3f800000`. Both square-root calls use the original imported stub at RVA `0x28d9c8`. This contract establishes the binary32 sums, binary64 call boundaries, binary32 narrowing, reciprocal and multiplication order. It does not re-specify the imported library's internal square-root approximation or an untraced global floating-point mode.

Original evidence: `decompiled/client/assembly.txt` and `binary/Cube.exe`, third velocity load/squared-length preparation and first root at `0x197ee6–0x197f15`; root narrowing and reciprocal at `0x197f1a–0x197f26`; normalized products and temporary stores at `0x197f2a–0x197f54`; selected-length root at `0x197f4b–0x197f62`; scaled products and ordered target/scratch writes at `0x197f66–0x197fa3`; import identity in `decompiled/client/functions/0068d9c8.c` and original import stub RVA `0x28d9c8`.

Initial main-record vector copy

After the comparison skip or the conditional tail, main-record construction begins at `0x197fa8`. At `0x198075–0x1980af`, the producer reloads the supplied-vector pointer from explicit argument 8, reads its current X, Y and Z values, and stores them into main hit-record relative fields `+0x38/+0x3c/+0x40`, respectively. Those stores are binary32 value copies; the normalization tail's local scratch is not their source.

At this construction point the native main-record base is stack `+0xc4`, and its vector destinations are stack `+0xfc/+0x100/+0x104`. The later list append explicitly passes that record base. The record-vector source is argument 8's pointed storage, including any caller-created alias; construction does not separately select the target's stored response or the conditional velocity result. The pointer is re-read here, so this is not a promise that its pointed values equal an earlier snapshot under concurrent mutation.

This is an initial-construction statement, not an assertion that every later producer branch leaves the record unchanged. Subsequent producer modifications, record transport and application by the separate hit applier are outside the present boundary. No response mutation in that separate application is attributed to this producer block.

Original evidence: `decompiled/client/assembly.txt` and original `binary/Cube.exe`, supplied-vector reload and three record stores at `0x198075–0x1980af`; main-record append base at `0x198679–0x198687`. The surrounding original decompilation is `decompiled/client/functions/00596d30.c`, RVA `0x196d30`.

Limits and held result

The stable finite bypass is a consequence of the original repeated calculation and comparison directions, not an inferred design purpose. The conditional tail is retained as a bounded native path; no concurrent-mutator schedule or exceptional-input trigger for reaching it is invented. Invalid pointers, corrupt objects, nonfinite values or intermediates, SIMD exception masks/rounding/denormal controls, asynchronous state changes and imported math internals are not assigned recovery behavior. This artifact does not claim that velocity or response is unchanged elsewhere in the complete producer or later applier.

Status: FINAL / STABLE / HELD after full prose readback. Earlier held artifacts remain unopened and unchanged.
