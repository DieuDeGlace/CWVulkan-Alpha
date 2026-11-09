# Alpha imported power-helper call boundary

Status: FINAL / STABLE / HELD after complete prose readback by the author.

This is a bounded original-caller and PE-import contract. It establishes the scalar register boundary used by the supported Alpha client and the subsequent result conversions in the examined health/stat paths. It does not recover the runtime DLL's implementation or certify its numerical behavior.

## Supported original evidence

The supported image is `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred image base `0x00400000`. Its PE machine value is `0x014C` and optional-header magic is `0x010B`: this contract describes the original 32-bit x86 client boundary. All source paths below are relative to the original `disassembled alpha cube world` evidence root. Addresses are RVAs unless explicitly labeled VAs.

The evidence is `binary/Cube.exe`, `decompiled/client/assembly.txt`, `decompiled/client/calls.csv` and `decompiled/client/external_functions.csv`. The reviewed direct callers are the scalar wrapper at RVA `0x011D10`, maximum health at `0x044DB0`, continuous stat power at `0x045F60`, and the related normalized stat-power helper at `0x045FF0`. Fixed item health at `0x0C70B0` and the item-stat helper at `0x0C6A90` establish the outer game-helper boundary. No other specification or implementation material was read for this clarification; the held health contract was left unopened and unchanged.

## Import identity and thunk

The client imports the exact ASCII name `_libm_sse2_pow_precise` from the descriptor named `MSVCR110.dll`. The import is by name, not by ordinal. Direct PE evidence is:

| Import property | Original image value |
| --- | --- |
| Import directory RVA | `0x36074C` |
| MSVCR110 original lookup-table RVA | `0x360BF8` |
| MSVCR110 first-thunk / IAT base RVA | `0x2FC380` |
| Entry index within that descriptor, zero-based | 53 |
| Lookup slot RVA | `0x360CCC` |
| Lookup word | `0x00363508`, with the ordinal flag clear |
| Hint/name record RVA | `0x363508` |
| Hint | `0x0371` |
| Name following the two-byte hint | `_libm_sse2_pow_precise` |
| Import-address slot RVA | `0x2FC454` |

The hint is a name-lookup hint; it is not an ordinal import. The on-disk IAT word also contains `0x00363508` before loader resolution. The game thunk at RVA `0x28D8D4`, VA `0x0068D8D4`, is a direct indirect jump through the resolved slot at VA `0x006FC454`. The thunk adds no argument preparation, conversion, stack frame or extra return address. [Evidence: the PE import structures in `binary/Cube.exe`; `decompiled/client/assembly.txt`, VA `0x0068D8D4`; `decompiled/client/external_functions.csv`, entry identifying `_libm_sse2_pow_precise`.]

## Scalar register boundary established by the callers

Immediately before each reviewed import call, the first binary64 input occupies bits 63:0 of XMM0 and the second binary64 input occupies bits 63:0 of XMM1. In the health/stat-power callers, XMM0 contains binary64 two as the base and XMM1 contains the prepared exponent. In the scalar wrapper, the two binary32 wrapper arguments are separately widened to binary64 in XMM0 and XMM1. The return is read from bits 63:0 of XMM0 as a binary64 value. The caller narrows that returned value with a scalar binary64-to-binary32 SSE conversion. It does not obtain the imported result from the x87 return stack, an integer register, memory, or XMM1.

The upper binary64 halves of both XMM0 and XMM1 are positive zero on entry to every reviewed import call. A binary64 memory load supplies a scalar input and clears the destination's upper half. Where the exponent is widened from binary32, the source's second binary32 lane is zero, and the packed widening operation therefore produces a positive-zero upper binary64 lane. The scalar wrapper similarly loads each binary32 argument with its other lanes cleared before widening. These are established caller-supplied upper-lane values. Caller evidence alone does not prove that the runtime DLL ignores them. [Evidence: `decompiled/client/assembly.txt`, VAs `0x00411D13–0x00411D23`, `0x00444DB6–0x00444E5F`, `0x00445F63–0x00445FCA` and `0x00445FF3–0x00446081`.]

No additional application-supplied scalar argument is established by these callers in another XMM lane, a general-purpose register or the x87 stack. In particular, the scalar wrapper prepares only XMM0 and XMM1; it does not initialize XMM2 or XMM3 for the import. Health/stat callers can leave intermediate values in other registers, but those values are not a separately marshaled third argument in the observed boundary. This finding describes caller preparation, rather than proving every possible internal dependency or scratch-register rule of the unseen callee.

The reviewed direct callers do not push the two binary64 inputs onto the stack, create a stack argument area for this import, or perform a caller-local stack alignment adjustment around it. An ordinary x86 call places its return address on the existing stack, and the thunk's jump passes that stack unchanged to the DLL. The callers expect their pre-call stack depth to be restored on return and perform no import-argument cleanup. The original instructions establish no further alignment modulus or caller-reserved scratch-space requirement. Whether the DLL has stronger internal alignment assumptions is unresolved by this evidence.

The stack words supplied by the fixed item helpers to RVA `0x045F60` are a different boundary: they are one binary32 level and one 32-bit rank for that game-owned helper. The helper reads them and prepares the binary64 register inputs for its imported calls. Those two words are not two stack-passed binary64 arguments to the import. [Evidence: `decompiled/client/assembly.txt`, complete bodies at VAs `0x00411D10–0x00411D35` and `0x00445F60–0x00445FE5`, and VAs `0x004C6B07–0x004C6B46`, `0x004C7159–0x004C719B`.]

## Exact narrowing in the fixed health/stat paths

Each conversion below consumes the imported binary64 result from XMM0. Other-register preparation, integer operations or a comparison can occur between the call and that conversion; none of these callers performs arithmetic on the returned power value in binary64 before narrowing it.

| Game-owned caller and import call RVA | Native result-consumption sequence |
| --- | --- |
| Maximum health `0x044DB0`, level call `0x044DFA` | The caller starts preparing the rank in XMM1. At `0x044E0D`, it narrows XMM0's low binary64 value into XMM0's low binary32 lane. At `0x044E19`, it stores that binary32 level factor to its local slot. |
| Maximum health `0x044DB0`, rank call `0x044E29` | A role-byte comparison occurs first. At `0x044E32`, the caller narrows XMM0 in place to binary32. The multiplications beginning at `0x044E36` consume that binary32 value. |
| Maximum health `0x044DB0`, fixed-exponent call `0x044E5F` | The next instruction, at `0x044E64`, narrows XMM0 in place to binary32. The following multiplications consume the narrowed value. |
| Continuous stat power `0x045F60`, level call `0x045F9E` | At `0x045FA3`, XMM1 is cleared. At `0x045FA6`, XMM0's low binary64 result is narrowed into XMM1's low binary32 lane. XMM0 is then reloaded with the next call's binary64 base. At `0x045FB2`, the narrowed level factor is stored as binary32 in the helper's incoming level slot. |
| Continuous stat power `0x045F60`, rank call `0x045FCA` | At `0x045FCF`, the saved binary32 level factor is loaded into XMM1. At `0x045FD4`, XMM0's binary64 result is narrowed in place. The binary32 product is formed at `0x045FD8`, stored as binary32 at `0x045FDC`, and loaded from that binary32 slot onto the x87 return stack at `0x045FE1`. |

These sequences are established by `decompiled/client/assembly.txt`, VAs `0x00444DFA–0x00444E7A` and `0x00445F9E–0x00445FE5`. The narrowing operations are scalar SSE conversions subject to the ambient SSE control state; they are not integer truncations or x87 conversions of the imported binary64 result.

The related helper at RVA `0x045FF0` follows the same register boundary for all three of its imported calls. Its first call at `0x04602E` is narrowed in XMM0 at `0x04603B` and saved as binary32 at `0x046047`. Its second call at `0x046057` is followed by clearing XMM1 and narrowing the result into XMM1 at `0x04605F`; the following multiplication is binary32. Its third call at `0x046081` is narrowed in XMM0 at `0x04608B`, before the binary32 division at `0x04608F`. The helper stores the final binary32 value and loads that stored value for its x87 return. [Evidence: `decompiled/client/assembly.txt`, VAs `0x0044602E–0x0044609C`.]

The outer item helpers consume the game-owned helper's x87 result after this internal SSE narrowing and binary32 product. Fixed item health calls RVA `0x045F60` at `0x0C7186`, immediately stores/pops its x87 result as binary32 at `0x0C718B`, and reloads it into XMM0 at `0x0C718E` before item scaling. The item-stat helper does the corresponding call at `0x0C6B34`, binary32 store/pop at `0x0C6B39`, and XMM0 reload at `0x0C6B3C`. These x87 stores belong to the game-helper return boundary and do not change where the imported DLL result was returned. [Evidence: `decompiled/client/assembly.txt`, VAs `0x004C7186–0x004C7193` and `0x004C6B34–0x004C6B41`.]

For corroboration, the independent scalar wrapper at RVA `0x011D10` narrows the imported result in XMM0 at `0x011D28`, stores it as binary32 at `0x011D2C`, and loads that stored binary32 value for its x87 return at `0x011D31`. [Evidence: `decompiled/client/assembly.txt`, VAs `0x00411D10–0x00411D35`.]

## Unresolved callee behavior

This contract establishes the two low-lane binary64 inputs, caller-supplied positive-zero upper halves, low-XMM0 binary64 result consumption, lack of import-specific stack argument preparation, by-name import identity, and exact subsequent narrowing in the stated callers. The decompiler's external-function annotation is not evidence for an ordinary language-level function signature; the original register preparation and result consumption establish this boundary.

No runtime DLL implementation or runtime execution was inspected. The callee's exponentiation algorithm, accuracy, last-bit results, special-value handling, NaN payload behavior, exception flags, traps, denormal policy, errno or other CRT state, complete register-preservation rules, and any stronger stack/control-state requirements remain unresolved. The original caller evidence also does not establish the ambient MXCSR or x87 control state at every invocation. It therefore supports this bounded native call boundary without claiming numerical equivalence to another power routine or a complete portable ABI specification for the imported symbol.
