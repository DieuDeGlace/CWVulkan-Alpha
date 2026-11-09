# Alpha yaw trigonometric scalar ABI

This addendum establishes the scalar call boundary used by original client rotation helper RVA `0x024610`. Evidence is limited to the original files below, relative to `disassembled alpha cube world`. All prior held artifacts remained unopened and unchanged. No reconstruction, project implementation, runtime execution or other contract was used.

| Original file | Size | SHA-256 |
|---|---:|---|
| `binary/Cube.exe` | 3,885,568 bytes | `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf` |
| `game-files/msvcr110.dll` | 875,472 bytes | `b30160e759115e24425b9bcdf606ef6ebce4657487525ede7f1ac40b90ff7e49` |

Both are x86 PE32 images. The client's preferred base is `0x00400000`; the bundled runtime's is `0x10000000`. RVAs below are qualified by image where needed. The contract identifies the bundled runtime file; it does not establish which runtime file a particular Windows loader invocation actually chose. A call through the client thunk requires its import-address slot to have been resolved to the corresponding runtime entry.

## Identities and scalar lanes

The original client imports both functions by name from `MSVCR110.dll`. Its local thunks transfer control through the import-address slots without changing the argument representation.

| Operation | Client thunk RVA | Client import-address slot RVA | Imported name | Matching bundled-runtime export |
|---|---|---|---|---|
| Cosine | `0x28D7A4` | `0x2FC444` | `_libm_sse2_cos_precise` | Runtime RVA `0x036F80`, ordinal 878 |
| Sine | `0x28D9C2` | `0x2FC45C` | `_libm_sse2_sin_precise` | Runtime RVA `0x03DCF0`, ordinal 883 |

Each call receives one binary64 scalar in XMM0 bits 63 through 0 and returns its binary64 scalar result in those same bits. The upper 64 bits are not a second scalar argument or a second result. The client places no eight-byte scalar argument on the stack for these calls and does not obtain their return from the x87 floating stack. These facts describe the identified 32-bit register interface; they do not assign it a portable C/C++ prototype or a Microsoft x64 calling convention.

Evidence: client `binary/Cube.exe` import descriptors/name records, thunks `0x28D7A4` and `0x28D9C2`, and caller `0x024614–0x02465C`; runtime `game-files/msvcr110.dll` export table and entry bodies `0x036F80–0x037137`, `0x03DCF0–0x03DEB4`. Original client text is also present in `decompiled/client/assembly.txt` and `decompiled/client/functions/00424610.c`.

## What the original rotation caller supplies and consumes

The caller first loads a binary32 angle from memory into XMM0's low 32 bits. That memory load clears the rest of the register. It multiplies the low scalar by binary32 literal `0.01745329238474369` (`0x3C8EFA35`) and stores the rounded binary32 product. The packed promotion then produces the argument binary64 in XMM0's low 64 bits and positive zero in its upper 64 bits, because its second binary32 source component is zero.

Cosine is called first. The returned low binary64 is rounded to binary32 and saved to memory before sine is called. Sine receives a fresh memory load of the same saved binary32 angle product, followed by the same promotion; it also receives upper 64 bits equal to positive zero. The caller consumes only the returned low binary64 and rounds it to binary32 for the matrix calculation. No upper-lane value from either result participates in that calculation. The caller reloads its required matrix values and saved cosine after the sine call rather than expecting the math call to retain their XMM registers. Evidence: client `0x024614–0x02465C`.

For the bounded direct runtime paths below, incoming upper XMM0 bits cannot influence the returned low scalar. The other incoming XMM register contents are not additional numeric arguments. Some internal paired operations use both lanes after the runtime has supplied their data; that does not make this a packed two-input interface. Returned upper bits have no scalar meaning and are not uniformly zero or preserved across paths. The original caller's zero upper input is established separately from this lack of scalar dependence.

## Stated controls and bounded register guarantees

The strongest preservation and clobber findings in this addendum cover finite binary64 inputs with absolute value strictly below 90,112, including either signed zero and smaller finite values. They assume valid x86 execution with SSE2 available, a correctly mapped/relocated copy of the identified runtime and its constant tables, a valid return address, writable ordinary stack storage, and these controls:

- SSE rounding is round to nearest, and all SSE floating exceptions are masked. The runtime directly checks that MXCSR bits selected by `0x7F80` equal `0x1F80`.
- Denormals-are-zero and flush-to-zero are clear for this stated contract. The entry's preceding mask does not test those two controls, so its admission alone does not establish their values.
- The x87 control word's low seven bits equal `0x7F`, as tested by the runtime. The inspected direct bodies do not use x87 arithmetic, so their scalar computation adds no dependence on the untested x87 precision/rounding fields.

MXCSR exception-status bits need not remain clear between the two calls. They are outside the entry's control mask and may be changed by floating arithmetic. This is a statement of sufficient call conditions, not a claim that application startup has established them.

The runtime checks those controls before classifying the scalar magnitude. Under the stated controls, magnitudes below 90,112 stay within the inspected direct code, including its small-value handling. Its principal arithmetic body admits magnitudes from exactly `2^-252` through values strictly below 90,112; smaller magnitudes use a separate direct portion. These ranges follow the original tests of the sign-cleared high 16 bits of the low binary64, with lower boundary word `0x3030` and first excluded upper word `0x40F6`. This addendum does not reproduce the arithmetic approximation or its tables.

For this bounded call domain:

| State | Established constraint |
|---|---|
| XMM0 | Low 64 bits contain the scalar result. Remaining bits have no promised result meaning. |
| XMM1 through XMM7 | Scratch registers. Both principal arithmetic bodies overwrite all seven; no caller value in them may be relied on after the call. |
| EAX, EDX | Scratch registers; the principal bodies use both. No integer return value is established. |
| ECX, EBX, ESI, EDI, EBP | Unchanged by the inspected direct bodies. This is bounded body evidence, not a broader promise about every runtime fallback. |
| ESP and return | The entry temporarily uses eight bytes of stack storage, restores its stack position and returns to the ordinary four-byte return address. There is no scalar stack-argument cleanup and no caller-provided x64 shadow area. |
| Flags | Ordinary integer condition flags are not preserved. |
| x87 stack and controls | No floating result is pushed there; stack depth and control word are unchanged by these direct bodies. |
| MXCSR | Control bits are not rewritten by these bodies. Exception-status flags may change. |

In particular, XMM6 and XMM7 are scratch here. A preservation rule from a different architecture or calling convention is not evidence for these x86 exported entries. Small-value portions need not overwrite every listed scratch register on every call; the table gives the allowance required over the complete bounded domain.

The direct bodies read their own mapped constant tables and use temporary stack storage. No app/world/window object, heap allocation, thread-local runtime lookup, random generator or further function call is accessed on these bounded paths. This does not audit Windows module loading, DLL initialization or the client's import binding. Evidence: runtime entry/control checks `0x036F80–0x036FC8`, direct return portions through `0x03711E`; sine `0x03DCF0–0x03DD38`, direct return portions through `0x03DE9B`.

## Fallback and result limits

A nonmatching checked control state or a sufficiently large input can reach the runtime fallback wrappers at `0x03711F` for cosine and `0x03DE9C` for sine. Their external register-facing scalar boundary remains visible: they pass only the low binary64 to an internal eight-byte stack-scalar boundary, capture an internal x87 return as binary64, and reload that result into XMM0 with upper 64 bits cleared. The internal targets are runtime RVAs `0x032775` and `0x032EAE`. This audit stops at that bridge and does not establish all transitive clobbers, floating-control restoration, error handlers, global/TLS dependencies or exceptional returns of those targets. The direct-path preservation table therefore is not extended to these fallback routes.

The imported identities establish which operations the original caller requested. The file hashes, lane widths, conversion boundaries and inspected register effects do not establish correctly rounded sine/cosine for every input, an error bound, identical results from another runtime build, or a guarantee under changed floating controls. Numerical approximation, range-reduction internals, NaN/infinity behavior, signaling inputs, exception delivery and general fallback execution remain outside this small ABI contract. No original algorithm, coefficient table or implementation instruction sequence is reproduced.

Status: FINAL / STABLE / HELD after complete author readback. All prior held artifacts remain unopened and unchanged.
