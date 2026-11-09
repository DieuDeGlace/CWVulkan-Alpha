# Alpha block-light propagation lifecycle

Status: **FINAL / STABLE / HELD**. Source-author freeze: 2026-11-07.

This is a bounded, code-free contract for the original Alpha client's stored terrain lighting. It covers the complete rectangular propagation operation, its column and zone lookup rules, the first generation pass, and the traced terrain-rebuild admission path. It does not establish visual parity, a replacement engine's authorship, or the complete set of native invalidation events.

## Evidence and supported image

The source-author policy was read before reference inspection. Reference filenames were inventoried before content searches. No replacement engine, exporter, SDK, tests, captures, logs, existing specifications, or implementation history were inspected. Reconstruction-owned terrain files under `Infinite-Cubes-main/Infinite-Cubes-main/alpha` supplied search leads; the native claims below were independently checked against original Alpha decompiled routines and original assembly. No reconstruction-only claim is required by this contract.

Evidence paths are relative to the `disassembled alpha cube world` evidence root. The supported native image is `binary/Cube.exe`:

| Identity property | Value |
| --- | --- |
| File length | 3,885,568 bytes |
| SHA-256 | `84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF` |
| PE machine | `0x014C`, 32-bit x86 |
| Preferred image base | `0x00400000` |
| PE timestamp field | `0x51EA955E` |
| Image size | `0x003BC000` |
| Entry-point RVA | `0x0028E1E0` |

All addresses below are RVAs in that image. Original evidence resides under `decompiled/client`: individually identified numeric `.c` files in `functions`, and address-bearing instructions in `assembly.txt`. The generation routine's decompilation timed out, so its lifecycle claims come from original assembly. No claim is made for the server image or another Alpha build.

## Stored state and initial allocation

Each stored terrain cell occupies four bytes. Byte `+3` contains a material identifier in its low five bits and flags in the high three bits. For cells whose masked material is 0 or 2, bytes `+0`, `+1`, and `+2` respectively serve as the committed light result, the next-round working value, and the current-round working value. Those same byte positions retain color data for other materials; they must not generally be interpreted as light.

A native column record has a 32-byte stride. Its signed base height is at `+0x10`, its cell pointer at `+0x18`, and its cell count at `+0x1C`; these are 32-bit fields in this image. The zone's column-array pointer is at `+0xA8`, and its two 32-bit zone coordinates are at `+0x60` and `+0x64`. A zone contains 256 by 256 columns. Local X varies fastest in storage.

Zone construction at RVA `0x2240F0` constructs 65,536 column records. The original column-construction instructions at RVA `0x050BF0` initialize base height, cell pointer and count to zero. An empty new column therefore has no stored light cells. The resize operation at RVA `0x050C50` clears newly allocated four-byte cells to zero before copying retained cells. Retained cells preserve all four bytes, with their position adjusted when storage grows downward. Generation and subsequent terrain writes can replace these zero values; this is not evidence that every cell entering the lighting operation has zero committed light.

## Rectangular operation and reset

The propagation operation starts at RVA `0x19A0E0`. Its inputs are a horizontal rectangle with inclusive low bounds and exclusive high bounds, a horizontal margin, and an optional restriction to a particular zone. The working rectangle extends each side of the requested rectangle by that margin. Missing columns are skipped. The operation does not allocate or generate missing columns, extend their stored vertical ranges, or add implicit cells to storage.

Initialization visits X in increasing order, then Y in increasing order, then each stored column from its highest cell downward. The scan begins with an unobstructed-sky condition. Masked materials 0 and 2 permit that condition to continue. Their two working bytes both become 255 while the condition holds, and both become zero after an obstructing material has been encountered. Every other masked material permanently closes the condition for the remainder of that column. Its cell bytes are left untouched by the lighting operation.

This initialization preserves byte `+0` until the final commit. It overwrites both working bytes of every eligible stored cell in the working rectangle, regardless of their previous values. Consequently, a repeated invocation restarts lighting within its working rectangle; it does not continue earlier rounds or preserve an earlier interior light field as its starting state. Material 2 receives full direct-sky light with no extra vertical attenuation in this operation. Flag bits do not change the initialization eligibility test.

The requested rectangle and the working rectangle have different effects when the margin is nonzero. Both working bytes are reset and propagated throughout the expanded area, but byte `+0` is committed only within the requested rectangle. Working bytes outside the commit rectangle can therefore change while their committed results remain unchanged.

## Propagation rounds, order and material gates

The operation performs exactly sixteen rounds. It has no convergence test, changed-cell frontier, lighting queue, recursive flood, or persistent round cursor. Each round first calculates next-round values throughout the working rectangle, then performs a separate transfer throughout that rectangle from byte `+1` to byte `+2`. This separation prevents a newly calculated value from advancing through multiple cells during one calculation sweep.

The calculation sweep visits X increasingly, then Y increasingly, then stored cells from the column's lowest index upward. A target is calculated only when its masked material is 0 or 2 and its current-round byte is not 255. Cells already at 255 retain their initialized working values. Solid targets are neither recalculated nor transferred.

For a calculated target, the six axis-adjacent neighbors are considered in this order: negative X, positive X, negative Y, positive Y, negative Z, positive Z. The largest available brightness is retained. As soon as that maximum reaches 255, remaining neighbors are not read. Ties do not create an ownership or scheduling distinction. There are no diagonal propagation neighbors.

A resolved neighbor with masked material 13 supplies brightness 255. A neighbor with masked material 0 or 2 supplies its byte `+2`, raised to a minimum of 5. Every other masked material supplies zero. Material 13 is therefore a full-brightness source when sampled, while still obstructing the initialization scan and being ineligible as a propagation target. The semantic identity and ordinary occurrence of material 13 are not established here.

The next-round byte is 85 percent of the maximum neighbor brightness, truncated downward to an integer. A maximum of 255 therefore yields 216. This replaces the next-round value; it is not compared against the target's previous value. The minimum of 5 applies when reading an eligible neighbor, not to every target or final result. For example, six solid neighbors provide zero, while an eligible neighbor whose stored working value is zero provides 5 and can produce a target value of 4.

After the sixteenth transfer, a final increasing-X, increasing-Y, increasing-height sweep copies byte `+2` into byte `+0` for eligible stored cells in the requested rectangle. Material and flags remain unchanged. The fixed round limit is part of the observed behavior; an unrestricted flood to equilibrium is not the established native operation.

These facts are supported by the complete routine at RVA `0x19A0E0`, including initialization around `0x19A130`, attenuation around `0x19A67E`, transfer around `0x19A714`, and the sixteen-round bound at `0x19A769`. The separate brightness query at RVA `0x0718B0` independently confirms the material-13 exception, material-0/material-2 eligibility, and minimum of 5.

## Neighbor resolution and boundaries

Column lookup at RVA `0x0347A0` accepts horizontal block coordinates from zero through `0x00FFFFFF`, inclusive. Without a zone restriction, it looks up the currently admitted zone containing that column; it does not create a zone. Zone lookup at RVA `0x034D10` and region lookup at RVA `0x034A90` only return existing entries. A missing entry fails the column lookup.

With a zone restriction, a horizontal coordinate must fall inside that zone's own 256-by-256 extent. The lookup does not fall back to the world lookup for an adjacent zone, even when such a zone is already admitted. The restriction is applied to neighbor samples as well as target-column visits.

The propagation routine resolves neighbor cells using the following native boundary behavior:

| Neighbor location | Resolved material and lighting consequence |
| --- | --- |
| Missing column, failed zone lookup, outside the allowed horizontal range, or below a column's stored base | Material 1; contributes zero |
| At or above the stored column end, with global height greater than zero | Material 0 with all three value bytes 255; contributes 255 |
| At or above the stored column end, with global height zero or below | Material 2 with all three value bytes 255 and flag `0x80`; contributes 255 |
| Stored material 0 at global height zero or below, with flag `0x40` clear | The same full-brightness material-2 fallback replaces the stored cell for this read |
| Other stored cell within the column | Its stored bytes are read with the material gates above |

The distinction between global height and column-relative index matters. The below-base fallback takes precedence over the implicit above-column air/water choice. Initialization and target selection operate on stored records directly; the below-sea substitution belongs to neighbor resolution.

The fallback records have original initializers at RVAs `0x2FAAD0`, `0x2FAB00`, and `0x2FAB30`. Their corresponding data RVAs are `0x36B7D0` for white material 0, `0x36B7D4` for material 1 with value bytes 200, and `0x36B7CC` for white material 2 with flag `0x80`. Solid fallback color does not contribute brightness.

Neighbors can lie outside the working rectangle. Such samples read the existing byte `+2` of an admitted stored cell, subject to the same gates. That exterior state is not reset or transferred by the current call. It must not be assumed to equal byte `+0`, especially following an earlier call with a nonzero margin. Cross-zone light can therefore enter a world-wide rectangular invocation from existing adjacent-zone state, but absent zones do not provide a deferred frontier.

## Generation, admission and later rebuilds

The generation routine at RVA `0x1E4850` returns an already admitted zone without rebuilding it. For a newly constructed zone, the call at RVA `0x1EDE84` requests its entire 256-by-256 horizontal extent, with margin zero and the new zone supplied as the restriction. This excludes all adjacent zones from that first lighting operation. The generated zone is installed into its region's zone table later, at RVA `0x1EE05A`. The first pass therefore occurs before that publication and is sufficient to operate on the new zone without world-table membership.

The traced terrain-rebuild routine at RVA `0x09D910` addresses 32-by-32 block tiles. It invokes lighting when the tile record's byte `+0x74` is nonzero, or when the tile touches a zone edge. A nonzero byte selects an extension of four columns on every side; an edge tile with that byte zero selects sixteen. This extension is incorporated into the requested rectangle itself; the lighting call's separate margin is zero. Both working and committed light are consequently updated throughout that extended rectangle. The call at RVA `0x09DB84` supplies no zone restriction and can cross admitted zone boundaries. A tile with that byte zero that is strictly inside a zone skips this lighting call.

In the observed caller at RVA `0x0690A0`, rebuild admission is tested by RVA `0x06F490`. The containing zone must exist. A tile strictly inside its zone needs no neighboring zone for this test. An edge tile requires the zone immediately across the touched side. A corner tile checks the diagonally adjacent zone first, then the X-adjacent zone, then the Y-adjacent zone; all three must exist. Failure postpones this caller's rebuild. The predicate performs presence checks rather than generating those neighbors. This is rebuild eligibility, distinct from the six-neighbor order inside a lighting round.

Every lighting call that proceeds performs the reset and sixteen rounds described above. Publication of a zone does not resume a stored light queue. The generation and admission instructions inspected here do not directly invoke a second cross-zone lighting pass merely by installing the zone; later eligible tile rebuilds supply a confirmed path for recalculation across that boundary.

The lighting operation and its column lookup helpers acquire no lock themselves. The traced tile-rebuild routine enters the world critical section at offset `+0x8000D8` before its lighting call, through RVA `0x19C480`; the corresponding unlock helper is RVA `0x201EA0`. This does not establish the complete locking discipline of generation or of all other callers.

## Explicit limits

Two additional calls to the same lighting operation were located at RVAs `0x20F071` and `0x213FC0` in original update assembly. Their complete triggering events and rectangle policies were not traced here. The full set of writes to tile byte `+0x74`, all rebuild scheduling priorities, generation-to-render timing, and all save/network invalidation paths remain unresolved. The presence predicate above must not be generalized into a universal rule governing every native lighting call.

Fresh allocation, retained-cell copying, working-state reset, and commit are established separately. This contract does not assign a single uniform pre-lighting value to all generated or loaded cells, and does not claim that admitting a neighbor immediately updates every affected committed value. No runtime observation or native acceptance result was used to close those gaps.

This file is held after final readback. Its bounded facts are stable for the identified image; the unresolved lifecycle coverage remains open.
