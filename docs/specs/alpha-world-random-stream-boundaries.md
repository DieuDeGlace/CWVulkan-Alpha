# Alpha world random-call and seed boundaries

Status: FINAL / STABLE / HELD. Re-certified on 2026-11-17 against the original client image. This contract covers random and seed import boundaries in client world update 0x20C510, generic hit producer 0x196D30, world hit applier 0x195A60 and loot routine 0x19B180. It establishes the descriptor seed input and its order relative to actor creation and later updates.

## Supported image and evidence

The evidence root is identified relatively as `disassembled alpha cube world`. All evidence paths below are relative to that root. The supported original image is `binary/Cube.exe`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred image base 0x00400000. All routine and call-site addresses in this document are RVAs in that image.

The import directory and original native instructions were inspected directly. Individually identified original decompilation files supply additional context. The original image and `decompiled/client/assembly.txt` provide the world findings. The original C export for `0x20C510` timed out, and the routine-only assembly export omits action-dispatch destinations. The original image's dispatch tables and the complete instruction listing supply those destinations.

## Imported identity and numeric boundary

| Original import | Importing DLL name in this image | IAT RVA | Established call boundary |
| --- | --- | --- | --- |
| `rand` | `MSVCR110.dll` | 0x2FC468 | No explicit argument supplied at the scoped calls; result used from the low 32-bit integer return |
| `srand` | `MSVCR110.dll` | 0x2FC4B4 | One four-byte seed argument at the world call; caller removes its four argument bytes afterward |

These are named imports from the same declared DLL. The scoped random draws do not name separate DLLs, separate import slots or per-world/per-creature generator objects. Several sites cache the pointer loaded from the `rand` import slot and call that pointer; those are the same imported-call identity as the direct IAT calls.

This establishes identity at the executable's import boundary. It does not establish the exact runtime DLL binary selected by the loader, its internal recurrence, initial state, thread locality, state size, default seed, or behavior under import replacement. The import names alone do not prove that distinct threads or processes share a state, or that two calls with equal seed arguments must have equal surrounding draw histories. No runtime DLL implementation is specified here.

Native callers interpret results as 32-bit integers before their particular arithmetic. Some use signed division/remainder, some convert that signed integer to binary32, and some discard it. The binary32 constant at image RVA 0x2FD338 has bits 0x46FFFE00, equal to 32767.0, and is used as a denominator by several scoped presentation/probability calculations. That observed divisor does not by itself prove a range or distribution for every possible imported return value.

Evidence: the PE import directory of `binary/Cube.exe`; direct and cached calls below; original constant bytes at RVA 0x2FD338. The DLL spelling and symbol names above are import identities, not inferred descriptions of a reconstructed generator.

## World descriptor seed admission and input

The world update has one direct call through its `srand` IAT slot: RVA 0x20FD38. It is inside descriptor activation, not an unconditional entry seed and not a seed performed once for every actor. The containing descriptor pass is bypassed when world byte +0xB4 is nonzero. It also requires the four-byte member at world+0xA4 to be nonzero: the test is helper 0x077220 applied to world+0x94, whose result is true exactly when that member is zero. Temporary source-collection admission precedes the descriptor visits; this audit does not reconstruct the entire collection-gathering process.

For the seed input, distinguish three objects. Q is the current source object whose descriptor-pointer vector begins at Q+0x18. D is the current descriptor obtained from that vector. M is a world spatial record resolved for Q. Before descriptor traversal, the native path reads Q's signed four-byte coordinate components at Q+0x60 and Q+0x64, divides each by 8 with truncation toward zero, and passes those coordinates to world helper 0x087DA0. Its returned pointer is M. The helper requires each divided coordinate to be within 0 through 8,191 inclusive and returns null if a coordinate is outside that interval or the underlying world lookup fails. The complete upstream origins and lifetime of Q, D and M remain outside this contract.

Before the distance test and the seed-specific predicates, D must pass these established boundaries:

- If the signed four-byte members D+0x40 and D+0x44 are unequal, the signed world value returned by helper 0x071910 must lie inclusively between them. That helper reads the four-byte world+0x80015C member. Equality with either endpoint is admitted. Equal descriptor endpoints bypass these two bound checks.
- World creature lookup with the complete eight-byte identifier at D+0x48 must return null. A nonnull existing creature rejects this descriptor visit before the seed.
- A negative signed four-byte D+0x38 is replaced by zero. A nonzero remaining D+0x38 rejects the visit when four-byte D+0x3C equals world+0x800160; zero does not take that equality rejection.
- The temporary proximity-candidate collection must supply a candidate whose tested distance quantity is strictly smaller than the square of binary32 D+8. Equality does not admit this branch. The descriptor radius is loaded as binary32 and squared at binary32 precision; the distance-helper result is stored to binary32 before the comparison. Full construction of the candidate collection and the preceding position-conversion helpers is not closed here.

At that admitted proximity branch, seeding additionally requires nonnull M, four-byte M+0x34 equal to 5, and bit 0x1000 present in the two-byte field D+0x7A. Without this conjunction, this particular seed and its immediately following helper call are bypassed. Descriptor activation has other routes after this branch; absence of this seed is not equivalent to absence of all later descriptor work.

When four-byte D+0x28 is unequal to 6, the path first copies four bytes from M+0x3C into D+0x34 and one byte from M+0x40 into D+0x58. Equality to 6 skips those copies but still permits the seed. The seed argument is the low 32 bits of the sum of the raw four-byte M+0x30 value and the low four bytes of the identifier at D+0x48. No high identifier word, carry into a wider seed, floating-point conversion, saturation or additional salt is supplied at this call. Arithmetic wraps at 32 bits; the resulting four-byte bit pattern is passed directly.

Evidence: world native 0x20F942–0x210499, especially 0x20FB48–0x20FB87, 0x20FBC1–0x20FC29 and 0x20FC61–0x20FD38; `decompiled/client/functions/00477220.c`, `00471910.c`, `00487da0.c`, `00424860.c`. The comparison and conversion statements are bounded to these native sites; they do not specify all spatial arithmetic or general descriptor eligibility.

## Local order after the seed and before actor processing

The seed call at 0x20FD38 precedes the synchronous helper call at 0x20FD4A to RVA 0x1C7730. That call receives D and a four-byte value 1, with the current world as its receiver. The world caller makes no direct `rand` call between its seed and entry to this helper. This does not make the helper a single draw: its original body has many conditional draws through the same `rand` import, including a cached import pointer. Its descriptor-bit and kind branches can also bypass substantial work. Nested routines invoked there are not fully audited for their own draws, seeds or callbacks.

After the helper returns, the world path performs descriptor-flag and equipment-related work. The later nested item traversal at 0x20FE32–0x20FFAE can make two direct cached-pointer draws per qualifying item, at 0x20FF14 and 0x20FF25. The first return is tested by signed remainder with divisor 20; a zero remainder increments that item's byte +0x10. The second return is tested with divisor 100 and can increment the same byte again. Both calls occur for a qualifying item even when the first remainder is nonzero. The final byte is limited to 4 by the subsequent unsigned comparison and byte assignment.

These two draws require the item's four-byte leading member to be nonzero, helper 0x034870 applied to item+4 to return nonzero, and the item's byte +4 to be one of 3, 4, 5, 6, 7, 8 or 9. Once the leading-member and helper gates pass, two bytes from M+0x3C are copied into item+0x14 before the item category is tested. For the listed categories, the value of the byte at M+0x40 plus 1, reduced to eight bits, is stored at item+0x10 before the first draw. The later increments are also byte-width operations with wrap and precede the final unsigned limit. The item-validity helper's full policy and item-generation behavior remain open.

The proximity branch leads to actor creation whether it takes or bypasses the seed branch. At 0x20FFBA–0x21000C the ordinary successful-allocation path constructs an actor with D's complete eight-byte identifier at D+0x48, then records the actor pointer under the same identifier in the world creature map beginning at world+0x04. The constructor at 0x03B690 copies the identifier's low and high words into actor+0x08 and actor+0x0C. The earlier existence lookup at 0x20FBFB and the creation map access at 0x210000 compare signed high words and, when equal, unsigned low words. This is signed 64-bit key order. Keys with the high sign bit set use the same lookup and construction boundaries. The evidence includes the lookup helper 0x02F000, its key comparison at 0x077DB0–0x077DE0, the creation map comparison at 0x068AF3–0x068B24, and the identifier copy at 0x03B6BA–0x03B6CE. All upstream descriptor construction and the full actor initialization remain open.

Descriptor visits continue in their existing container traversal after this work. An additional eligible visit can therefore execute the same seed site again with its own argument before actor processing begins. The pass finishes before the world creature-map iteration starting at 0x210499. That iteration processes actor-associated queued work before the living-creature general-update boundary around 0x211AE1. No second direct `srand` import call in this world body restores an earlier seed at either boundary.

There is no established save-and-restore operation for a previous random state around the direct seed and descriptor helper. In particular, the caller does not receive a prior-state token from the seed call or pass one to a direct restoration call. This is an absence of proven restoration at the inspected boundary, not a proof that every nested callee is incapable of changing or restoring runtime state.

Evidence: world native 0x20FD31–0x20FFAE, 0x210433–0x210522 and 0x211A86–0x211AE1; original descriptor helper `decompiled/client/functions/005c7730.c`, with its native import-pointer load at 0x1C77BB and direct import call at 0x1C81FE. Full helper/item-generation consumption and the meanings of unaudited predicates remain unresolved.

## Direct world draw-site inventory

The inventory covers 47 native sites in the world routine. Thirty-five call the `rand` IAT slot directly and twelve call through a pointer loaded from the same slot. The per-action sites are reachable through the original action-dispatch table at 0x211E14–0x211E1B. Each site's execution count and relative order depend on the branch taken, loop iterations and intervening calls.

| World-update phase | `rand` calls through the IAT | `rand` calls through a cached pointer |
| --- | --- | --- |
| Earlier world work before the descriptor-seed pass | 0x20D24B, 0x20EF9B, 0x20F38D | — |
| Descriptor attachment/item traversal after its conditional seed and helper | — | 0x20FF14, 0x20FF25 |
| Creature action-dispatch branches before the common buff pass | 0x212AFD, 0x212F7C, 0x212FEF, 0x21327A, 0x213EA9, 0x2145C7, 0x21463B, 0x214949, 0x2149B4, 0x214A75, 0x214C9F, 0x214E4D, 0x21596D, 0x216B76, 0x216C19, 0x216CDA, 0x217812, 0x2179AB, 0x217AB2, 0x217E5A, 0x217F8B, 0x218028, 0x2180C5 | 0x212442, 0x212502, 0x212738, 0x212809, 0x216460, 0x21652A, 0x2174D9, 0x21756C, 0x217597, 0x2175C4 |
| Later work in the world creature-map iteration | 0x21C1A0, 0x21E7F2, 0x21EE7E, 0x21EEAB | — |
| Later world work after that creature-map iteration | 0x221A71, 0x221AEE, 0x221F81, 0x222406, 0x222483 | — |

The earlier sites can already consume imported results before descriptor activation is reached. The two attachment sites follow any consumption inside helper 0x1C7730. The later actor and projectile-related sites remain in the same imported-call identity; the world does not pass them a private generator object or select a separate hit/loot import. Their full branch predicates and all intervening generators are outside this inventory.

World calls to the scoped hit routines occur on both sides of descriptor activation. Direct hit application at 0x20CD9B and generic hit production at 0x20DD18 and 0x20F45D precede the descriptor pass. During action dispatch, production calls are at 0x21476B and 0x2166C8, and application calls are at 0x214BAC and 0x214F72. The buff pass calls production at 0x2183DA. Later world work calls application at 0x221E14 and production at 0x221E8C and 0x222AB8. These calls are synchronous. Their nested draws belong to the corresponding point in a same-thread sequence, in addition to the sites listed in the table.

The evidence is `decompiled/client/assembly.txt` and `binary/Cube.exe`. The descriptor sites use the pointer loaded at 0x20FF09. Within action dispatch, the import pointer is loaded at 0x21243C, 0x2124FC, 0x21254C, 0x2126E7, 0x212803, 0x212853, 0x212965, 0x21645A and 0x2174B3. The inventory covers calls issued by the world routine itself. Transitive helpers and virtual-call consumption remain open.

## Shared boundary in generic hit production and application

Generic hit producer 0x196D30 has eight direct calls through the same `rand` IAT slot. Their bounded roles are distinguished below so that a discarded draw or an early-return branch is not silently treated as absent. The table is not a single unconditional execution sequence.

| Direct call RVA | Established local role |
| --- | --- |
| 0x196E20 | Optional early presentation when source byte +0x68 equals 28 |
| 0x196EF9 | Presentation in the early category-3 production route |
| 0x1971C0 | Presentation in the early category-1 production route |
| 0x197478 | Presentation after producing the category-5 portion |
| 0x197D1B | Later ordinary-hit presentation calculation |
| 0x197E3D | Source-null route: result discarded, followed by target float +0x1184 assigned 0.5 |
| 0x198258 | Critical-related presentation calculation |
| 0x198545 | Later status/probability comparison |

The source-null draw is reached only after the routine's initial target admission. It is not an entry draw that also occurs on every nonnull-source route. Conversely, its discarded result still constitutes an imported call. The late probability branch makes its draw after preparing its threshold; the directly inspected path does not omit the call merely because a resident-record contribution has reduced that threshold to zero. Complete eligibility, amount/stat arithmetic and all presentation emission details are outside this contract.

The producer appends records before its identified local-application calls. With world byte +0xB4 zero, those calls enter 0x195A60 at producer call sites 0x19700F, 0x197229, 0x1974DD and 0x1986D0. The category-3 and category-1 routes leave after their local-application opportunity. The category-5 route can continue with a remaining ordinary amount. Consequently nested application is a real ordering boundary: its draws and any fatal/loot work occur before the producer resumes after that call, rather than being postponed to the end of all producer draws. This statement does not expand the native record schema or all application predicates.

Hit applier 0x195A60 has one direct `rand` import call of its own at 0x196A1E on a later conditional item-generation route. A signed remainder of zero with divisor 2 selects generator 0x1F51E0. A nonzero remainder selects 0x1F8AD0. The selected generator can consume randomness, and its draw count remains open. The applier's fatal path can call loot routine 0x19B180 at 0x19622E earlier in the applier body. That call can consume additional draws before the later item-generation route.

Neither scoped hit body contains a direct `srand` import call. This does not establish seed absence in every callee. The eight producer sites, the applier's one site, and the directly identified loot calls all reach IAT RVA 0x2FC468. Their common identity is proven by native targets, independently of labels such as combat, presentation or loot.

Evidence: `decompiled/client/functions/00596d30.c`, `00595a60.c` and original instructions at the cited RVAs. The source-null branch is at 0x197E3D–0x197E58; the applier's item-choice draw and generator selection are at 0x196A1E–0x196A62. The full admission and mutation contracts of these routines remain separate research dependencies.

## Loot entry and nested-generator limits

Loot routine 0x19B180 loads its random-call pointer from IAT RVA 0x2FC468 at 0x19B1E8. Before its first draw, it rejects creature byte +0x60 equal to 0 or 3, and rejects bit 0x0800 in the two-byte field at creature+0x7E. Other ordinary values passing those checks reach the call at 0x19B1EE. That first result is discarded. It precedes the later equipment/item collection and the branch-specific drop decisions.

The body uses that same imported target for subsequent direct calls, reloading it at 0x19BAB5, 0x19BC0D and 0x19C2D9 after other uses of its temporary storage. These reloads do not establish new seeds or separate streams. The later branches contain chance tests, selection draws, per-drop direction components and other values; nested item generators and helpers lie between some of those direct draws. In particular, the first optional drop branch can call a nested item generator before its later direction and scalar draws. Therefore a list of visible direct `rand` sites cannot be converted into a fixed whole-loot draw count.

There is no direct `srand` IAT call in this loot body and no direct caller-side save/restoration boundary around its `rand` calls. Full drop selection, the reachability of every rare or malformed-state branch, generator internals, and calls made by generated-item helpers are not closed here. The original decompiler marks some body fragments unreachable; this contract does not count those fragments as guaranteed executed draws.

Evidence: `decompiled/client/functions/0059b180.c`; native 0x19B1C1–0x19B1EE and the stated import-pointer reloads in `binary/Cube.exe` / `decompiled/client/assembly.txt`. The fatal caller is the original hit applier's site at 0x19622E.

## Scope of the stream conclusion

For an ordinary execution on one thread with unchanged import bindings, the scoped routines make synchronous calls to the same imported random function, and their direct seed calls use the identified seed import. The descriptor seed occurs before its activation helper and before the later actor pass. This is the established ordering and identity relation. The evidence does not support independent combat and presentation streams at these direct sites, nor does it establish a separate loot stream merely because loot is a different routine.

A complete resulting random sequence remains unresolved. Necessary missing dependencies include the runtime DLL's actual state model and binary identity; thread assignment, concurrency and import initialization; all earlier seeds and draws; the full conditional traversal history; nested generators, callbacks and virtual calls; and exceptional or reentrant paths. No full random algorithm, global seed lifetime, state restoration, deterministic replay guarantee, equal-seed equality guarantee or cross-thread/process state sharing is inferred from the API names. No original runtime was executed for this contract.

## 2026-11-17 re-certification

The original image bytes and action table at 0x222C84/0x222CCC confirm 23 IAT call sites and ten cached call sites on action-dispatch branches that the routine-only export omitted. The earlier 14-site inventory is replaced by the 47-site inventory above. Four calls to the scoped hit routines on those branches were also added. The original PE import directory, the one direct world seed at 0x20FD38 and the cached calls confirm the import identity and ordering. The descriptor item-copy position, actor-key comparison and creation boundary were rechecked at 0x20FBF1–0x21000C and in the key helpers. Runtime DLL state, transitive draw histories, and native runtime acceptance remain open.
