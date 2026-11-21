# Alpha surface-cover observation boundaries

**FINAL / STABLE / HELD**

This source-only contract identifies where the scalars of the per-column surface pass can be read for one selected column in root generation C `0x1E4850` and S `0x118630`. It covers the mountain factor P, the four ridge noises and the ridge pattern T, the adjusted factor P′, the product G, its saturated scale and the rock cover. C and S denote client and dedicated RVAs. Each observation point describes the state immediately before its instruction executes. Post-call points are the first instructions after the named native call returns. The assigned held `alpha-worldgen-terrain.md` sections 1, 3.4 and 5.4 supplied leads. The held column initial-color and zone-generator observation contracts supplied format and frame leads. Original bytes establish the bounded observations below.

| Original image | Reconfirmed SHA-256 |
| --- | --- |
| `binary/Cube.exe` | `84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF` |
| `binary/Server.exe` | `1FFBCBCE251E1506A63C9CE18AB0009D5553FCBB9E6202BE2BC3A7D870D34FB1` |

Both images are PE32 x86 with preferred base `0x00400000`. Runtime addresses use the actual loaded image base plus the stated RVA. Relative evidence paths refer to the original disassembled Alpha evidence root. Inside the root and mountain-factor windows below, every dedicated RVA equals the client RVA minus `0xCC220`. The separately named noise and location-distance routines lie outside those windows and have their own RVAs. The paired windows have identical instruction counts, identical operations and operands apart from absolute addresses, and identical bytes at every referenced constant. Evidence `binary/Cube.exe`, `binary/Server.exe`, `decompiled/client/assembly.txt` and `decompiled/server/assembly.txt`.

## Column identity and frame

The root-frame column locations of the held initial-color contract apply unchanged. World-column X is the signed 32-bit value at root EBP `-0x1318`, and world-column Y is at root EBP `-0x12C8`. The world/environment address is at root EBP `-0x12E0`, and the zone descriptor address is at root EBP `-0x1308`. X is the outer loop and Y the inner loop. At the inner-loop head C `0x1E4DE0` or S `0x118BC0`, EDX holds X and ECX holds Y, and neither is modified before the mountain-factor call. The zone location record address is at root EBP `-0x1300`, and its signed 32-bit type field is at record `+0x18`. Evidence C `0x1E4CC8` through `0x1E4E95`, `0x1E6B08` through `0x1E6B57`; S `0x118AA8` through `0x118C75`, `0x11A8E8` through `0x11A937`.

All surface-pass arithmetic in the root window uses SSE. Binary32 operations use scalar single-precision instructions, and binary64 operations use scalar double-precision instructions. In the root, the x87 unit is used only to carry routine results in ST0 and to store them. Every result of the noise, mountain-factor and location-distance routines is returned in ST0 and immediately stored to a four-byte slot of the calling frame. That store rounds the x87 value to binary32 under the live x87 rounding control. The caller thereafter reloads the binary32 slot. Inside the mountain-factor routine the same pattern applies, and its type-10 location branch additionally converts two signed 64-bit differences through x87 binary32 stores. These memory round trips fix where binary32 rounding occurs.

Several root slots are reused temporaries. EBP `-0x12DC` successively holds N2, N3, N4, a location distance and a further noise. EBP `-0x12F0` successively holds N1, the first ridge term, an adjustment weight and G. EBP `-0x12F4` holds P′ until C `0x1E553E` or S `0x11931E`, where the lower mix weight of the held initial-color contract replaces it. An observation must therefore read each slot inside the stated interval.

## Mountain factor P

The mountain-factor routine is C `0x1EFFA0` or S `0x123D80`. The root calls it at C `0x1E4E95` or S `0x118C75`. Immediately before that call, ECX holds the world address loaded from root EBP `-0x12E0`. The current ESP addresses signed 32-bit X, ESP `+4` holds signed 32-bit Y and ESP `+8` holds a third four-byte argument. On the path through C `0x1E4E6B` or S `0x118C4B`, that third value is loaded from root EBP `-0x1308`. On the other path it is the register value already used as the zone address for column selection. The routine consumes twelve argument bytes on return. Evidence C `0x1E4E59` through `0x1E4EB7`, `0x1F071D`; S `0x118C39` through `0x118C97`, `0x1244FD`.

Both return paths of the routine first form a binary32 result, store it to memory and load it into ST0. The returned ST0 value is therefore exactly representable in binary32, so the root's binary32 store of P is exact.

| Root observation | Client RVA | Dedicated RVA | Value and location |
| --- | --- | --- | --- |
| Mountain factor returned | `0x1E4E9A` | `0x118C7A` | P in x87 ST0, an exactly binary32 value |
| P stored | `0x1E4EA0` | `0x118C80` | P binary32 at root EBP `-0x12D8` |

P remains at root EBP `-0x12D8` through the surface-height product at C `0x1E52DD` or S `0x1190BD`. The root compares it against binary32 0.25 for the zone counter at C `0x1E4EA8` or S `0x118C88`.

Inside the routine, the frame is ESP-based. Let E be the value of ESP at C `0x1EFFC2` or S `0x123DA2`, after the routine has saved two registers. Between C `0x1EFFE8` and `0x1F01B5` (S `0x123DC8` through `0x123F95`), ESP equals E minus `0x10`. From C `0x1F01B9` (S `0x123F99`) onward, ESP equals E at every listed point that names an ESP offset, because the remaining direct callees each consume their own stack arguments. The ordinary-path return points after C `0x1F0704` (S `0x1244E4`) follow the restoration of two saved registers and name registers only. The offsets below are relative to ESP at the stated instruction. Each noise call receives two binary64 coordinates, the first at ESP and the second at ESP `+8`. The first coordinate depends on X and the second on Y. Evidence C `0x1EFFA0` through `0x1F071F`; S `0x123D80` through `0x1244FF`.

| Routine observation | Client RVA | Dedicated RVA | Value and location |
| --- | --- | --- | --- |
| Entry | `0x1EFFA0` | `0x123D80` | ECX world; ESP `+4` X, `+8` Y, `+0x0C` third argument, all four bytes |
| Noise a returned | `0x1F0014` | `0x123DF4` | a in x87 ST0 |
| a stored | `0x1F0024` | `0x123E04` | a binary32 at ESP `+0x28` |
| Complement 1 − a² stored | `0x1F0044` | `0x123E24` | Binary32 at ESP `+0x1C` |
| Noise for b returned | `0x1F007F` | `0x123E5F` | Unscaled noise in x87 ST0 |
| Unscaled noise stored | `0x1F0083` | `0x123E63` | Binary32 at ESP `+0x28` |
| b formed | `0x1F00A3` | `0x123E83` | b, the binary32 product with binary32 0.1, at ESP `+0x28` |
| Noise c returned | `0x1F00EB` | `0x123ECB` | c in x87 ST0; b still at ESP `+0x28` |
| c stored | `0x1F00EF` | `0x123ECF` | c binary32 at ESP `+0x20` |
| Rounded magnitude of b + c | `0x1F011C` | `0x123EFC` | Binary32 in XMM1 low lane |
| Factor after the ridge-shape step | `0x1F014C` | `0x123F2C` | Binary32 at ESP `+0x20`, also XMM0 low lane |
| Fourth noise returned | `0x1F0184` | `0x123F64` | x87 ST0; binary32 copy at ESP `+0x1C` from C `0x1F0188` or S `0x123F68` |
| Factor after the fourth-noise scale | `0x1F01BD` | `0x123F9D` | Binary32 in XMM1 low lane; at ESP `+0x10` from C `0x1F01D8` or S `0x123FB8` |
| Factor after the valley-location step | `0x1F031A` | `0x1240FA` | Binary32 in XMM3 low lane, before the negative test |
| Factor entering the location adjustments | `0x1F0465` | `0x124245` | Binary32 at ESP `+0x10`, including the zero result of the negative test |
| Final pre-smooth factor, ordinary paths | `0x1F06DE` | `0x1244BE` | Binary32 in XMM2 low lane |
| Final pre-smooth factor, type-2 path | `0x1F04DB` | `0x1242BB` | Binary32 in XMM2 low lane |
| Returned value formed, ordinary paths | `0x1F070B` | `0x1244EB` | Binary32 in XMM1 low lane |
| Returned value loaded, ordinary paths | `0x1F0715` | `0x1244F5` | Same value in x87 ST0 until the return at C `0x1F071D` or S `0x1244FD` |
| Returned value formed and loaded, type-2 path | `0x1F050B` | `0x1242EB` | Binary32 at ESP `+0x0C` and in XMM1; ST0 from C `0x1F050F` or S `0x1242EF` until the return at C `0x1F0522` or S `0x124302` |

The b + c sum is a binary32 addition. Its magnitude is taken in binary64 and narrowed to binary32. The ordinary final pre-smooth point is the common join of the absent-location path, location types other than 4 and 5, both type 4 and 5 interior branches, and the type-10 adjustment. On the type-2 path, the routine returns the smoothstep of the factor plus a binary32 location term and bypasses the ordinary join. Evidence C `0x1F00EF` through `0x1F014C`, `0x1F0465` through `0x1F0522`, `0x1F0616` through `0x1F071D`; S `0x123ECF` through `0x123F2C`, `0x124245` through `0x124302`, `0x1243F6` through `0x1244FD`.

## Ridge noises and T

The root's four ridge noise calls use the noise routine C `0x0C0EF0` or S `0x0D5D30`. The caller removes its sixteen argument bytes. Immediately before each call, the first binary64 coordinate is at ESP and the second is at ESP `+8`. The X-dependent coordinates are precomputed per X in the outer loop and stored at root binary64 slots. The Y-dependent coordinates are formed per column. The four calls occur in the following order. Evidence C `0x1E4D3E` through `0x1E4DD8`, `0x1E4EB7` through `0x1E5086`; S `0x118B1E` through `0x118BB8`, `0x118C97` through `0x118E66`.

| Root observation | Client RVA | Dedicated RVA | Value and location |
| --- | --- | --- | --- |
| N1 call | `0x1E4F01` | `0x118CE1` | First coordinate copied from root EBP `-0x147C` |
| N1 returned | `0x1E4F06` | `0x118CE6` | N1 in x87 ST0 |
| N1 stored | `0x1E4F0C` | `0x118CEC` | N1 binary32 at root EBP `-0x12F0` |
| N2 returned | `0x1E4F31` | `0x118D11` | N2 in x87 ST0 |
| N2 stored | `0x1E4F37` | `0x118D17` | N2 binary32 at root EBP `-0x12DC`; N1 still at `-0x12F0` |
| v1 before narrowing | `0x1E4F6D` | `0x118D4D` | Binary64 in XMM0 low lane |
| v1 after narrowing | `0x1E4F71` | `0x118D51` | v1 binary32 in XMM0 low lane |
| First ridge term stored | `0x1E4F9C` | `0x118D7C` | (1 − v1³)² + 0 binary32 at root EBP `-0x12F0`, replacing N1 |
| N3 returned | `0x1E4FC1` | `0x118DA1` | N3 in x87 ST0 |
| N3 stored | `0x1E4FC7` | `0x118DA7` | N3 binary32 at root EBP `-0x12DC` |
| Scaled N3 stored | `0x1E4FE2` | `0x118DC2` | Binary64 product of widened N3 and binary64 0.05 at root EBP `-0x1474` |
| N4 returned | `0x1E500F` | `0x118DEF` | N4 in x87 ST0 |
| N4 stored | `0x1E5015` | `0x118DF5` | N4 binary32 at root EBP `-0x12DC` |
| v2 before narrowing | `0x1E5054` | `0x118E34` | Binary64 in XMM0 low lane |
| v2 after narrowing | `0x1E5058` | `0x118E38` | v2 binary32 in XMM0 low lane |
| T formed | `0x1E5086` | `0x118E66` | T binary32 in XMM0 low lane |
| T stored | `0x1E508E` | `0x118E6E` | T binary32 at root EBP `-0x1310` |

N1 through N4 are kept as binary32. For v1, the root widens N1 and N2 exactly to binary64 and multiplies widened N2 by binary64 0.05. It adds that product to widened N1, then takes the magnitude and subtracts it from binary64 one. For v2, it adds widened N4 to the stored binary64 scaled N3 and applies the same magnitude and subtraction. The scaled N2 is not stored. Each v is narrowed once, and the cube, complement, square and final sums are binary32. T is the second ridge term plus the stored first ridge term. T remains at root EBP `-0x1310` through its use in G. Evidence C `0x1E4F37` through `0x1E5086`; S `0x118D17` through `0x118E66`.

## P′, G, saturation and cover

At C `0x1E5072` or S `0x118E52`, the root copies P from EBP `-0x12D8` to EBP `-0x12F4` as the default P′. XMM1 keeps the same P. At C `0x1E508E` or S `0x118E6E`, EAX holds the zone location type. Types 6 and 13 enter the adjustment branch, and all other types branch to C `0x1E51FD` or S `0x118FDD` with P′ unchanged. The root reads that location record address without a null test inside this window. Evidence C `0x1E503C` through `0x1E5210`; S `0x118E1C` through `0x118FF0`.

The adjustment branch calls the location-distance routine C `0x1F8E10` or S `0x12C820`. Immediately before that call, ECX is the location record address. ESP addresses the address of the X point, and ESP `+4` addresses the address of the Y point. Each point is a signed 64-bit value equal to the column coordinate shifted left by sixteen. The root's 64-bit X point begins at EBP `-0x1508` and its Y point begins at EBP `-0x14C8`. The branch then calls the noise routine once with root-formed binary64 coordinates. Evidence C `0x1E509C` through `0x1E51F3`; S `0x118E7C` through `0x118FD3`.

| Root observation | Client RVA | Dedicated RVA | Value and location |
| --- | --- | --- | --- |
| Location distance returned | `0x1E50E3` | `0x118EC3` | Distance in x87 ST0; binary32 copy at root EBP `-0x12DC` from C `0x1E50F4` or S `0x118ED4` |
| Location weight stored | `0x1E514B` | `0x118F2B` | Binary32 at root EBP `-0x12F0` |
| Adjustment noise returned | `0x1E5173` | `0x118F53` | x87 ST0; binary32 copy at root EBP `-0x12DC` from C `0x1E5179` or S `0x118F59` |
| Saturated adjustment noise | `0x1E51AD` | `0x118F8D` | Binary32 in XMM2 low lane |
| Adjusted P′ formed | `0x1E51F3` | `0x118FD3` | Binary32 in XMM1 low lane |
| P′ common join | `0x1E5210` | `0x118FF0` | P′ binary32 in XMM1 low lane and at root EBP `-0x12F4` on both paths |
| G formed | `0x1E521B` | `0x118FFB` | G = P′·T binary32 in XMM0 low lane |
| G stored | `0x1E522E` | `0x11900E` | G binary32 at root EBP `-0x12F0`; XMM1 low lane holds unclamped binary32 1.25·G |
| Saturated scale | `0x1E5240` | `0x119020` | sat(1.25·G) binary32 in XMM1 low lane, common to all clamp paths |
| Cover formed | `0x1E526E` | `0x11904E` | Cover binary32 in XMM2 low lane |
| Cover stored | `0x1E5276` | `0x119056` | Cover binary32 at root EBP `-0x1344` |

G remains at EBP `-0x12F0` through its surface-height use at C `0x1E52CD` or S `0x1190AD`. The cover is the binary32 smoothstep of the saturated scale, formed in the product order stated by the terrain specification. The upper clamp uses binary32 one and the lower clamp uses binary32 zero. Evidence C `0x1E5210` through `0x1E5276`; S `0x118FF0` through `0x119056`.

The cover at EBP `-0x1344` is the first mix weight of the held initial-color contract. No instruction writes that slot between its store and the four-byte producer's return at C `0x1E5333` or S `0x119113`, and the producer's output occupies EBP `-0x134C` through `-0x1349`. At that post-call point the root reloads the cover and compares it with binary32 one-half to select the rock type. It then forms the binary32 complement from binary32 one minus this cover, which weights the surface bytes. After the rock-color call returns at C `0x1E540E` or S `0x1191EE`, the root reloads the same slot as the rock weight. Evidence C `0x1E5276` through `0x1E5412`; S `0x119056` through `0x1191F2`.

## Limits

The selected root window, mountain-factor routine, loop back-edge and zone-descriptor assignment supplied 896 checked instruction encodings in each image, with zero mismatches against original bytes. Evidence windows C `0x1E4CC8` through `0x1E555F`, `0x1EFFA0` through `0x1F071F`, `0x1E6B08` through `0x1E6B57`, `0x1E4980` through `0x1E4997`. The corresponding dedicated windows are S `0x118AA8` through `0x11933F`, `0x123D80` through `0x1244FF`, `0x11A8E8` through `0x11A937`, `0x118760` through `0x118777`. Return forms of the noise, location-lookup, location-distance, fixed-point difference and shaping-field callees were checked in both images.

The binary32 value of each x87 result depends on the live x87 rounding control. Its pre-store value depends on the noise routine and the precision control. Every SSE result depends on the live MXCSR rounding, flush and denormal controls. The held zone-generator contract leaves those controls time-specific. The identity of the third mountain-factor argument is established only as the root's zone address, not as a semantic input of the factor. The non-null status of the zone location record in the adjustment test is unresolved. The internal formulas of the shaping-field, distance and noise routines lie outside this contract. No running-native values are claimed. The earlier held artifacts retain their existing contents.
