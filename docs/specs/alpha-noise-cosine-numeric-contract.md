# Alpha bounded cosine numeric contract

Status: **FINAL / STABLE / HELD**. Source-author freeze: 2026-11-08.

This contract determines the scalar binary64 output of the identified original Alpha cosine import for every finite binary64 input whose magnitude does not exceed the client constant with bits `0x400921FB5A7ED197`, under the floating-point conditions below. It gives complete phase values and rounded mathematical relations. It does not establish correct rounding to ideal mathematical cosine, another library's equivalence, running-native observations, or end-to-end noise parity.

## Evidence and scope

The isolated source-author policy was applied. Only original binary evidence was used for this assignment. Evidence paths are relative to the `disassembled alpha cube world` root.

| Property | Client | Bundled runtime |
| --- | --- | --- |
| Path | `binary/Cube.exe` | `game-files/msvcr110.dll` |
| Length | 3,885,568 bytes | 875,472 bytes |
| SHA-256 | `84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF` | `B30160E759115E24425B9BCDF606EF6EBCE4657487525EDE7F1AC40B90FF7E49` |
| PE machine | `0x014C`, x86 | `0x014C`, x86 |
| Preferred image base | `0x00400000` | `0x10000000` |
| Runtime version | — | `11.00.51106.1`; file-version suffix `built by: Q11REL` |

The client noise routine starts at client RVA `0x0C0EF0`. Its calls at client RVAs `0x0C0F80` and `0x0C0FBC` reach the thunk at `0x28D7A4`, through IAT RVA `0x2FC444`. The original PE import-by-name entry identifies `MSVCR110.dll!_libm_sse2_cos_precise`, whose export in this DLL starts at DLL RVA `0x36F80`. The import's hint is 877, its ordinal-import flag is clear, and the DLL's export ordinal is 878.

The inspected numeric body is at DLL RVAs `0x36FB1` through `0x3711E`; its phase constants begin at `0x37140` and its reduction and correction constants occupy `0x37940` through `0x379B0`. PE data, disassembled native instructions and constant bytes support the facts below. No replacement code, SDK, exporter, tests, probes, captures, logs or implementation history were inspected. The original game and DLL were not executed. Earlier held contracts were not edited.

## Domain and rounding meaning

Let the input be the single binary64 value \(x\). The domain is finite \(x\) with \(|x|\leq C\), where \(C\) has bits `0x400921FB5A7ED197`, approximately 3.1415926999999999. This is the client's stored angle multiplier, rather than nearest-binary64 mathematical π. The scalar argument and result occupy the low half of XMM0 at the inspected boundary. A second independent input is not obtained from its upper half.

The environment required by this contract is:

- MXCSR masked with `0x7F80` equals `0x1F80`.
- The x87 control word masked with `0x007F` equals `0x007F`.
- Denormals-are-zero and flush-to-zero are disabled.

The first two conditions are the native entry's acceptance checks. The last condition is an explicit limit of this contract; the native check does not inspect those two modes. Existing exception-status flags are not part of its acceptance check. The x87 precision and rounding fields are not tested and do not supply arithmetic precision or rounding for this accepted SSE2 calculation. MXCSR `0x1F80` with x87 control word `0x037F` satisfies the stated conditions.

Throughout this document, \(R\) means nearest, ties-to-even rounding to binary64 with gradual underflow. Every elementary addition, subtraction or multiplication enclosed by \(R\) is independently rounded. Nested occurrences preserve distinct rounding points; they do not permit fused operations or retention of extra precision across those points. Signed zero follows the IEEE binary-operation rules, including positive zero for an exact cancellation of opposite nonzero terms under nearest rounding, and the usual sign of a zero product. Integer labels used as arithmetic operands are represented exactly in binary64.

The relations below describe numeric dependencies. They assign no native registers, storage layout, evaluation loop or source-language implementation. No FMA or x87 floating-point arithmetic occurs in the accepted numeric body.

For \(|x|<2^{-252}\), the result is exactly positive one, bits `0x3FF0000000000000`. This includes either signed zero and all binary64 subnormal inputs. At \(|x|=2^{-252}\), the regular relations below apply. Exception-status flags are outside this numeric-output contract.

## Reduction and phase

All hexadecimal encodings below denote full binary64 values, not bytes in file order.

| Symbol | Meaning | Bits | DLL RVA |
| --- | --- | --- | --- |
| \(K\) | Scale near 32/π | `40245F306DC9C883` | `0x37980` |
| \(H\) | Leading step near π/32 | `3FB921FB54400000` | `0x379A0` |
| \(L\) | Secondary step | `3D90B4611A600000` | `0x37990` |
| \(T\) | Further step correction | `3B63198A2E037073` | `0x379A8` |

Let \(n\) be the nearest integer, ties to even, to \(R(Kx)\). This defines the input partitions after the rounded multiplication, rather than by comparison with exact mathematical multiples of π. In the stated domain, \(-32\leq n\leq32\). The native integral-rounding offset has bits `4338000000000000` at DLL RVA `0x37988`, equal to \(3\cdot2^{51}\). Within this bounded range, its rounded addition and subtraction produce exactly the same binary64 integral value \(n\) used in these relations.

The reduced quantities satisfy:

\[
u=R\bigl(x-R(nH)\bigr),\qquad v=R(nL),\qquad r=R(u-v),
\]

\[
\delta=R\bigl(R(nT)-R(R(u-r)-v)\bigr).
\]

Thus \(u\) retains the leading-step displacement, \(r\) retains the secondary reduction, and \(\delta\) carries the remaining reduction correction. Both displacements have magnitude below 0.0491 in this domain. The distinction between \(u\) and \(r\) remains relevant in the rounded correction relations.

The phase is the unique integer \(j\) in 0 through 63 congruent to \(n+16\) modulo 64. Its four numeric values are denoted \(a_j,b_j,e_j,d_j\). The main phase value is \(b_j\), with small correction \(e_j\); the corresponding phase slope is divided between coarse value \(d_j\) and residual \(a_j\). The sixteen-position offset expresses the quarter-cycle shift used for cosine.

## Complete phase values

The following table gives the exact values for phases 0 through 16. The sign identities and axis values following it determine every remaining phase; all of those identities were checked against the original 64-phase constant bytes.

| \(j\) | \(a_j\) bits | \(b_j\) bits | \(e_j\) bits | \(d_j\) bits |
| --- | --- | --- | --- | --- |
| 0 | `0000000000000000` | `0000000000000000` | `0000000000000000` | `3FF0000000000000` |
| 1 | `BF73B92E176D6D31` | `3FB917A6BC29B42C` | `BC3E2718E0000000` | `3FF0000000000000` |
| 2 | `BF93AD06011469FB` | `3FC8F8B83C69A60B` | `BC626D19C0000000` | `3FF0000000000000` |
| 3 | `BFA60BEA939D225A` | `3FD294062ED59F06` | `BC75D28DA0000000` | `3FF0000000000000` |
| 4 | `BFB37CA1866B95CF` | `3FD87DE2A6AEA963` | `BC672CEDE0000000` | `3FF0000000000000` |
| 5 | `BFBE3A6873FA1279` | `3FDE2B5D3806F63B` | `3C5E0D8920000000` | `3FF0000000000000` |
| 6 | `BFC592675BC57974` | `3FE1C73B39AE68C8` | `3C8B25DD20000000` | `3FF0000000000000` |
| 7 | `BFCD0DFE53ABA2FD` | `3FE44CF325091DD6` | `3C68076A20000000` | `3FF0000000000000` |
| 8 | `3FCA827999FCEF32` | `3FE6A09E667F3BCD` | `BC8BDD3420000000` | `3FE0000000000000` |
| 9 | `3FC133CC94247758` | `3FE8BC806B151741` | `BC82C5E120000000` | `3FE0000000000000` |
| 10 | `3FAC73B39AE68C87` | `3FEA9B66290EA1A3` | `3C39F630E0000000` | `3FE0000000000000` |
| 11 | `BF9D4A2C7F909C4E` | `3FEC38B2F180BDB1` | `BC76E0B180000000` | `3FE0000000000000` |
| 12 | `BFBE087565455A75` | `3FED906BCF328D46` | `3C7457E620000000` | `3FE0000000000000` |
| 13 | `3FA4A03176ACF82D` | `3FEE9F4156C62DDA` | `3C8760B1E0000000` | `3FD0000000000000` |
| 14 | `BFAC1D1F0E5967D5` | `3FEF6297CFF75CB0` | `3C75621720000000` | `3FD0000000000000` |
| 15 | `BF9BA1650F592F50` | `3FEFD88DA3D12526` | `BC887DF640000000` | `3FC0000000000000` |
| 16 | `0000000000000000` | `3FF0000000000000` | `0000000000000000` | `0000000000000000` |

For each integer \(k\) from 1 through 15, the remaining non-axis values have these numeric identities:

| Phase | Source phase in the table | Components with reversed sign | Components with the same sign |
| --- | --- | --- | --- |
| \(16+k\) | \(16-k\) | \(a,d\) | \(b,e\) |
| \(32+k\) | \(k\) | \(a,b,e,d\) | None |
| \(48+k\) | \(16-k\) | \(b,e\) | \(a,d\) |

Sign reversal means toggling the binary64 sign bit while retaining every other bit. The source values in these identities are nonzero. The two remaining axes are specified explicitly to retain their positive-zero encodings:

| Phase | \(a_j\) bits | \(b_j\) bits | \(e_j\) bits | \(d_j\) bits |
| --- | --- | --- | --- | --- |
| 32 | `0000000000000000` | `0000000000000000` | `0000000000000000` | `BFF0000000000000` |
| 48 | `0000000000000000` | `BFF0000000000000` | `0000000000000000` | `0000000000000000` |

These values are native numeric facts. Substituting freshly evaluated ideal sine or cosine values for them is not an identity of this contract.

## Rounded correction relations

The signed correction constants are:

| Symbol | Approximate mathematical coefficient | Exact bits | DLL RVA |
| --- | --- | --- | --- |
| \(s_3\) | -1/6 | `BFC5555555555555` | `0x37940` |
| \(s_5\) | 1/120 | `3F81111111111111` | `0x37950` |
| \(s_7\) | -1/5040 | `BF2A01A01A01A01A` | `0x37960` |
| \(s_9\) | 1/362880 | `3EC71DE3A556C734` | `0x37970` |
| \(c_2\) | -1/2 | `BFE0000000000000` | `0x37948` |
| \(c_4\) | 1/24 | `3FA5555555555555` | `0x37958` |
| \(c_6\) | -1/720 | `BF56C16C16C16C17` | `0x37968` |
| \(c_8\) | 1/40320 | `3EFA01A01A01A01A` | `0x37978` |

For the selected phase, omit the subscript \(j\) from \(a,b,e,d\). The effective phase slope and reduced powers satisfy:

\[
\kappa=R(a+d),\qquad z=R(r\,r),\qquad w=R(z\,z).
\]

The odd and even correction families are fixed by the following rounded relations:

\[
S_0=R\bigl(R(s_5z)+s_3\bigr),\qquad
S_1=R\bigl(R(R(s_9u)r)+s_7\bigr),\qquad
P=R\bigl(S_0+R(S_1w)\bigr),
\]

\[
Q_0=R\bigl(R(c_4z)+c_2\bigr),\qquad
Q_1=R\bigl(R(R(c_8u)r)+c_6\bigr),\qquad
Q=R\bigl(Q_0+R(Q_1w)\bigr).
\]

Their phase-weighted contributions are:

\[
p=R\bigl(P\,R(R(\kappa r)z)\bigr),\qquad
q=R\bigl(Q\,R(bz)\bigr).
\]

In particular, the highest-degree factors contain both \(u\) and \(r\), as written. Replacing \(u\) with \(r\), or combining successive products into one final rounding, does not preserve the specified rounded relations.

The phase's linear contribution and its two summation corrections satisfy:

\[
g=R(dr),\qquad h=R(ar),\qquad \ell=R(g+b),\qquad m=R(h+\ell),
\]

\[
\epsilon_1=R\bigl(R(b-\ell)+g\bigr),\qquad
\epsilon_2=R\bigl(R(\ell-m)+h\bigr).
\]

The reduction correction contributes:

\[
J=R\bigl(\delta\,R(R(br)-\kappa)\bigr).
\]

The regular-domain result \(y\) is determined by this final rounded relation:

\[
y=R\left(m+R\left(R\left(R\left(R\left(R(J+e)+\epsilon_1\right)+\epsilon_2\right)+p\right)+q\right)\right).
\]

The nesting retains the small phase tail, the two linear-sum corrections, and the two local correction families at their distinct binary64 rounding positions. Together with the exact constants and tiny-input result, these relations determine the numeric output throughout the stated domain. They do not depend on a native register schedule or an unspecified system cosine.

## Static arithmetic anchors

The following anchors were derived from the original constants and arithmetic, with no execution of the original DLL or game. A separate static evaluation of the inspected instructions and the mathematical relations above agreed on 2,705 selected domain values, including adjacent values near reduction boundaries, tiny-input limits and cancellation cases. That correspondence check is finite; the complete relation is grounded in the inspected arithmetic, rather than an assertion that those examples exhaust the input domain.

Every hexadecimal value in this section is a binary64 encoding. Neighboring encodings in each positive-input group are immediately adjacent representable inputs.

| Case | Input bits | Reduction label \(n\) | Result bits |
| --- | --- | --- | --- |
| Below the tiny/regular boundary | `302FFFFFFFFFFFFF` | Tiny class | `3FF0000000000000` |
| At the tiny/regular boundary | `3030000000000000` | 0 | `3FF0000000000000` |
| Above the tiny/regular boundary | `3030000000000001` | 0 | `3FF0000000000000` |
| Near phase-label boundary 0/1 | `3FA921FB54442D17` | 0 | `3FEFF621E3796D7E` |
| Near phase-label boundary 0/1 | `3FA921FB54442D18` | 0 | `3FEFF621E3796D7E` |
| Near phase-label boundary 0/1 | `3FA921FB54442D19` | 1 | `3FEFF621E3796D7E` |
| Near phase-label boundary 16/17 | `3FF9EB0B2EE64E80` | 16 | `BFA91F65F10DD7F3` |
| Near phase-label boundary 16/17 | `3FF9EB0B2EE64E81` | 16 | `BFA91F65F10DD813` |
| Near phase-label boundary 16/17 | `3FF9EB0B2EE64E82` | 17 | `BFA91F65F10DD833` |
| Near phase-label boundary 31/32 | `4008BD7366F31C62` | 31 | `BFEFF621E3796D7E` |
| Near phase-label boundary 31/32 | `4008BD7366F31C63` | 31 | `BFEFF621E3796D7E` |
| Near phase-label boundary 31/32 | `4008BD7366F31C64` | 32 | `BFEFF621E3796D7E` |
| Cancellation near mathematical π/2 | `3FF921FB54442D17` | 16 | `3CB469898CC51702` |
| Nearest-binary64 mathematical π/2 | `3FF921FB54442D18` | 16 | `3C91A62633145C07` |
| Cancellation near mathematical π/2 | `3FF921FB54442D19` | 16 | `BCA72CECE675D1FD` |
| Near half of the client constant | `3FF921FB5A7ED196` | 16 | `BE58EA91F6E59D9C` |
| Half of the client constant | `3FF921FB5A7ED197` | 16 | `BE58EA91FAE59D9C` |
| Near half of the client constant | `3FF921FB5A7ED198` | 16 | `BE58EA91FEE59D9C` |
| Positive domain endpoint \(C\) | `400921FB5A7ED197` | 32 | `BFEFFFFFFFFFFFF6` |
| Negative domain endpoint \(-C\) | `C00921FB5A7ED197` | -32 | `BFEFFFFFFFFFFFF6` |

The three cancellation results near mathematical π/2 are approximately 2.8327694488239898e-16, 6.123233995736766e-17, and -1.6081226496766366e-16. The half-client-constant result is approximately -2.3205103333142417e-08. The endpoint result is approximately -0.99999999999999889. These distinctions remain meaningful before the surrounding noise calculation's subsequent blending and rounding.

## Limits retained

The accepted calculation is scalar binary64 arithmetic using SSE2, including internal packed operations. It contains no FMA. No x87 extended-precision arithmetic or hardware `FCOS` participates in this contracted path. Reassociation, contraction, a different set of phase constants, or an approximation based solely on real trigonometric identities is not established to preserve these results.

When the native entry's control checks fail, its wrapper reaches the x87 fallback at DLL RVA `0x3711F`, which calls DLL RVA `0x32775`. That path, nondefault denormal modes, exception-status effects, handler behavior, processor-dependent fallback accuracy, infinities, NaNs and magnitudes above \(C\) are outside this contract. The complete control state at each running-game call, the loader's actual selected DLL on another installation, and whole-noise behavior for invalid coordinate conversions were not established here.

No uniform maximum ULP error or proof of correct rounding against ideal mathematical cosine has been established. The bounded numeric relation is complete for the identified binary evidence and stated environment; those accuracy and runtime questions remain open. This artifact is held after complete final readback.
