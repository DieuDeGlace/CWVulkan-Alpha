# Alpha zone-generator observation boundaries

**FINAL / STABLE / HELD**

This bounded source-only contract identifies original ABI facts for selecting a native zone invocation during observation. C and S denote client and dedicated RVAs. Runtime addresses use the corresponding loaded image base plus the RVA. Evidence paths are relative to the original disassembled Alpha evidence root. The assigned held `alpha-generation-worker-floating-point-controls.md` supplied leads, and the boundaries below were checked against original bytes.

| Original image | Reconfirmed SHA-256 |
| --- | --- |
| `binary/Cube.exe` | `84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF` |
| `binary/Server.exe` | `1FFBCBCE251E1506A63C9CE18AB0009D5553FCBB9E6202BE2BC3A7D870D34FB1` |

Both are PE32 x86 images with preferred base `0x00400000`. At root entry C `0x1E4850` or S `0x118630`, ECX is the 32-bit world/environment address. Entry ESP addresses the caller's return address, ESP `+4` contains signed 32-bit zone X, and ESP `+8` contains signed 32-bit zone Y. After frame establishment, X and Y are at EBP `+8` and EBP `+0x0C`. The accepted bounds are zero through 65,535 inclusive on each axis. A zone index corresponds to a horizontal origin at 256 times that index in terrain-column units. Evidence `decompiled/client/assembly.txt`, C `0x1E4850` through `0x1E49F4`, and `decompiled/server/assembly.txt`, S `0x118630` through `0x1187D4`.

The mapped worker calls this entry at C `0x06ACD1` or S `0x1497D4`. Immediately before either call, ECX already holds the world address, and the two four-byte stack arguments begin at the current ESP with X followed by Y. The client worker derives that world address from its client owner plus `0x2E4`. Post-call points C `0x06ACD6` and S `0x1497D9` expose the returned EAX value. Evidence C `0x06ACC1` through `0x06ACDE` and S `0x1497CC` through `0x1497DE`.

| Native boundary | Client RVA | Dedicated RVA | Available identity |
| --- | --- | --- | --- |
| Root entry | `0x1E4850` | `0x118630` | World in ECX and two signed zone-index arguments |
| Newly allocated zone after coordinate assignment | `0x1E49D5` | `0x1187B5` | Zone descriptor in EDI, world in ESI, descriptor coordinate fields established |
| Common ordinary-return epilogue | `0x1EE09E` | `0x121E7E` | Final returned zone address or zero in EAX |
| Ordinary return instruction | `0x1EE0B8` | `0x121E98` | Same EAX result, with eight bytes of arguments consumed by return |

A zone descriptor has signed 32-bit zone X at `+0x60` and signed 32-bit zone Y at `+0x64`. The new-zone boundary above occurs immediately after those fields receive the incoming indices. A resident-zone lookup can instead return the existing descriptor through the common epilogue. An out-of-bounds coordinate selects a zero result. Thus a root entry for the desired coordinate can represent either reuse or fresh generation. The new-zone boundary distinguishes the ordinary allocation path. Allocation and exceptional failures remain outside the established ordinary-return claims. Evidence C `0x1E494C` through `0x1E49D5`, `0x1EE098` through `0x1EE0BA`; S `0x11872C` through `0x1187B5`, `0x121E78` through `0x121E9A`.

The following four-byte fields are relative to the world/environment address received in ECX. They permit observation of a logical seed and selected derived noise state for that same invocation.

| World offset | Native meaning established here |
| --- | --- |
| `+0x800164` | Stored 32-bit world-seed bits supplied to the environment reset |
| `+0x800188` | Derived 32-bit base used to seed zone-local native random draws |
| `+0x80022C` | Signed 32-bit X noise offset consumed by the rock-color path |
| `+0x800230` | Signed 32-bit Y noise offset consumed by the rock-color path |

The reset stores the seed at C `0x1A5324` or S `0x0D83E4` before passing it to imported `MSVCR110.dll!srand`. Subsequent imported `rand` results supply the derived fields, with a native signed remainder by 100,000. The base at `+0x800188` is assigned at C `0x1A5349` or S `0x0D8409`. The two rock-noise offsets belong to the six-word bank beginning at `+0x80021C`, initialized at C `0x1A5434` through `0x1A5453` or S `0x0D84F4` through `0x0D8513`. Their signed-coordinate use is visible at C `0x1F98B3` through `0x1F9907` and S `0x12D2C3` through `0x12D317`. Evidence the cited original ranges and both executable PE import directories.

On the new-zone path, the native random seed combines that world base with X and with 65,536 times Y using 32-bit integer arithmetic, including its wraparound. The call using that seed is C `0x1E49B5` or S `0x118795`. These fields expose selected initialized parameters. Equality of them establishes only that selected state, while complete deterministic equivalence additionally depends on the remaining environment, native runtime, resident data and floating-point controls. Evidence C `0x1E4996` through `0x1E49BB` and S `0x118776` through `0x11879B`.

This contract supplies static observation locations and widths. A live invocation's world address, thread, loaded-image identity, ambient x87 control word and MXCSR remain time-specific values. Descriptor lifetime across world changes, concurrent mutation, complete random-state ownership and the full transitive generation state remain unresolved. Selected entry, return, descriptor, seed and noise-field ranges were checked with 521 original instruction encodings and zero mismatches. The earlier held artifacts retain their existing contents.
