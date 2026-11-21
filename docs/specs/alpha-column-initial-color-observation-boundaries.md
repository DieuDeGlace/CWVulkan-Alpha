# Alpha initial column-color observation boundaries

**FINAL / STABLE / HELD**

This source-only contract covers the first two stored vertical records constructed for a selected column in root generation C `0x1E4850` and S `0x118630`. C and S denote client and dedicated RVAs. Each observation point describes the state immediately before its instruction executes. Post-call points below are the first instructions after the named native call returns. Lower and upper mean initial stored indices zero and one. The assigned held `alpha-worldgen-terrain.md` supplied leads. Original bytes establish the bounded observations below.

| Original image | Reconfirmed SHA-256 |
| --- | --- |
| `binary/Cube.exe` | `84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF` |
| `binary/Server.exe` | `1FFBCBCE251E1506A63C9CE18AB0009D5553FCBB9E6202BE2BC3A7D870D34FB1` |

Both images are PE32 x86 with preferred base `0x00400000`. Runtime addresses use the actual loaded image base plus the stated RVA. Relative evidence paths refer to the original disassembled Alpha evidence root. The selected root windows and four-byte color-producer boundaries supplied 423 checked instruction encodings in each image, all matching original bytes. Evidence `binary/Cube.exe`, `binary/Server.exe`, `decompiled/client/assembly.txt` and `decompiled/server/assembly.txt`.

At the root-level points in this contract, the following locations identify the active world column. Every EBP in this table belongs to the root generator frame. Each pointer occupies four bytes. Coordinates and the stored base Z are signed 32-bit integers.

| Root-frame location | Native value |
| --- | --- |
| EBP `-0x12E0` | World/environment address |
| EBP `-0x1308` | Zone descriptor address |
| EBP `-0x1318` | World-column X |
| EBP `-0x12C8` | World-column Y |
| EBP `-0x12D4` | Initial integer base Z used for color sampling |
| EBP `-0x12CC` | Address of the active 32-byte column record |
| EBP `-0x131C` | Binary32 height whose truncation supplied that base Z |

The initial lower record occupies base Z and the upper record occupies base Z plus one. The four-byte producer and both root rock-color calls described below receive the same base-Z coordinate. The upper stored record's color is sampled using that base Z. Root X and Y are full world-column coordinates. Their assignment and column selection are visible at C `0x1E4CC8` through `0x1E4E3A`, S `0x118AA8` through `0x118C1A`. Height conversion and saved Z are visible at C `0x1E5308` through `0x1E532E`, S `0x1190E8` through `0x11910E`.

The first root mix receives a four-byte producer result from C `0x1C56E0` or S `0x0F9450`. At its root post-call point, the three RGB bytes are already quantized. A fourth byte carries the native material/flag value. The root widens those three unsigned bytes exactly to binary32 and mixes them with a returned principal rock-color vector from C `0x1F9620` or S `0x12D030`. The first mix weight is binary32 at root EBP `-0x1344`. The surface-byte contribution uses its rounded binary32 complement, and the rock contribution uses the weight itself. Root channel products and their additions are binary32. The resulting mixed RGB vector is quantized before the initial stored pair is written. Evidence C `0x1E532E` through `0x1E54D8`, S `0x11910E` through `0x1192B8`.

| Root observation | Client RVA | Dedicated RVA | Scalar or byte values available |
| --- | --- | --- | --- |
| Four-byte producer completed | `0x1E5333` | `0x119113` | Four bytes at EBP `-0x134C`; first mix weight at EBP `-0x1344` |
| Principal rock vector for first mix completed | `0x1E540E` | `0x1191EE` | Three binary32 channels at EAX; producer bytes and first mix weight still available |
| First mixed vector immediately before byte conversion | `0x1E54C4` | `0x1192A4` | Three binary32 channels beginning at EBP `-0xFC`; ECX is channel index zero through two |
| First mixed vector has been quantized | `0x1E54DA` | `0x1192BA` | Resulting RGB bytes at EBP `-0x134C` through `-0x134A` |

At the first mixed-vector conversion point, the selected scalar is the four-byte value at EBP `-0xFC` plus four times ECX. The native conversion truncates toward zero into a signed 32-bit integer and retains the low byte. The three binary32 values remain available together before conversion. The RGB bytes produced there become the upper record's initial candidate color. Evidence C `0x1E54C4` through `0x1E54D8` and S `0x1192A4` through `0x1192B8`.

The lower record has two ordinary color paths. At C `0x1E5556` or S `0x119336`, the one-byte value at root EBP `-0x1309` is still the path selector. A nonzero value selects the same quantized RGB used for the upper record. Zero selects an additional lower-color mix. That selector storage is subsequently reused for a material byte, so this point has the original selection value. The lower mix weight is binary32 at root EBP `-0x12F4` and has reached its final root-level value at this point. Evidence C `0x1E551A` through `0x1E55F7`, S `0x1192FA` through `0x1193D7`.

The additional lower mix receives a three-binary32 vector from producer C `0x1C7140` or S `0x0FAE90`, plus a fresh principal rock-color vector. It also consumes the already quantized upper candidate RGB, widened from bytes to binary32. The additional producer's vector is weighted by the rounded complement of the lower mix weight. The other contribution combines the widened upper candidate and fresh rock vector, then uses the lower mix weight multiplied by binary32 one-half. The final channel additions and products use binary32 precision. These are separately observable native inputs to the final lower candidate. Evidence C `0x1E55DB` through `0x1E57D6`, S `0x1193BB` through `0x1195B6`.

| Root lower-mix observation | Client RVA | Dedicated RVA | Scalar values available |
| --- | --- | --- | --- |
| Additional producer completed | `0x1E55F7` | `0x1193D7` | Three binary32 channels at EAX; lower mix weight at root EBP `-0x12F4` |
| Fresh principal rock vector completed | `0x1E56A3` | `0x119483` | Three binary32 channels at EAX; quantized upper RGB bytes at root EBP `-0x134C` |
| Lower mixed vector immediately before byte conversion | `0x1E57C2` | `0x1195A2` | Three binary32 channels beginning at EBP `-0x120`; ECX is channel index zero through two |

The selected lower scalar at that conversion point is the four-byte value at EBP `-0x120` plus four times ECX. It is converted toward zero and its low byte is retained. On the direct-copy lower path, the first mixed-vector conversion above supplies the color bytes for both records. This makes the two observed scalar-vector boundaries sufficient to distinguish the root's ordinary initial color paths. Evidence C `0x1E5556` through `0x1E5886` and S `0x119336` through `0x119666`.

C `0x1E5900` or S `0x1196E0` is the common point after both initial stored-record writes and before further column work. The active column address is still at root EBP `-0x12CC`. Its `+0x10` field contains signed 32-bit base Z, `+0x18` contains the current 32-bit block-storage address, and `+0x1C` contains the signed 32-bit count. At ordinary successful construction, the first four bytes of that storage hold the lower record and the next four hold the upper record. RGB occupies record offsets zero, one and two. Reading this point captures the actual stored bytes, including the native write-admission effects. Evidence C `0x1E54FE`, `0x1E5565` through `0x1E5900`; S `0x1192DE`, `0x119345` through `0x1196E0`.

The four-byte producer has an earlier quantization boundary of its own at C `0x1C5DD5` or S `0x0F9B45`. Here EBP belongs to that producer, and three binary32 channels begin at its EBP `-0x10`. EDX selects channel zero through two. ESI addresses its four-byte output. Its own frame carries signed 32-bit X, Y and Z at EBP `+0x0C`, `+0x10` and `+0x14`. Its world address is saved at EBP `-0x38`. This boundary exposes scalar precision that has already been discarded when the root first-mix input is observed as bytes. Evidence C `0x1C56E0` through `0x1C5715`, `0x1C5DD3` through `0x1C5DFD`; S `0x0F9450` through `0x0F9485`, `0x0F9B43` through `0x0F9B6D`.

A finite channel whose truncated value fits signed 32 bits follows the conversion rule stated above, including low-byte wrap outside zero through 255. Masked invalid conversion of NaN, infinity or signed-32-bit overflow yields integer bits `0x80000000` and low byte zero. Unmasked exceptions can interrupt completion. Actual scalar values depend on live producer results, environment fields and floating-point controls. The complete x87 control word and MXCSR can be associated with the root and nested-producer observations on the same native thread. This contract establishes the inspected widths and boundaries, while live controls and loaded mathematical-runtime identity remain unresolved.

The final stored-pair point describes initial construction of the selected column. Persistence through later generation work, complete formulas inside the additional producers, exceptional allocation paths and full client/dedicated numerical equivalence remain outside this bounded observation contract. No running-native values are claimed. The earlier held artifacts retain their existing contents.
