# Generated world-item origins and scales in Alpha zone population

FINAL / STABLE / HELD

The original 32-bit Windows Alpha client, Cube.exe, is 3,885,568 bytes with SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`. All addresses below are RVAs from image base `0x00400000`. Seven direct append sites in ordinary zone generation were checked against the executable, including the early terrain branch omitted from the exported call list.

## Fields at creation

The constructor at `0x04A970` sets the binary32 scale at resident `+0x134` to `0x3D924925`, the nearest binary32 value to 1/14. It sets the origin byte at `+0x138` to 0, both following countdowns to 0 and the signed day at `+0x144` to −1. The embedded item has separate category and subtype bytes at `+0x00` and `+0x01`. All seven generated paths retain day −1. Each appends to the zone's item vector at `+0x30` through `0x04D590`. The copy at `0x04A8C0` preserves scale, origin and day as separate fields.

## Production sites

| Generation branch | Embedded item | Origin | Initial scale | Original evidence |
| --- | --- | ---: | --- | --- |
| Early terrain/cave decoration | Category 11, subtype 19, embedded byte `+0x0D` set to 9. This is case 0 of the four-way branch following a terrain write. | 0 | 0.1, bits `0x3DCCCCCD` | Constructor `0x1E649E`, item and scale `0x1E64E0–0x1E658E`, append `0x1E65D2`. No origin assignment follows construction. |
| Structure loot positioned relative to a generated object | The eligible object types at this branch are 12, 13, 35, 36 and 37. A candidate passes when its draw has remainder zero modulo 10. The contents vary, including category 11/subtype 26, category 11/subtype 9, category 1 subtypes 1, 4, 5 and 7 and items supplied by two other item selectors. | 1 | 0.06, bits `0x3D75C28F` | Object and candidate tests `0x1D319B–0x1D328C`, origin and scale `0x1D3292–0x1D32A4`, contents `0x1D32AE–0x1D344B`, append `0x1D3560`. The selected contents do not change the enclosing origin or scale. |
| Category-11 decoration in the feature generator at `0x1AF170` | Subtype 16 or 17. Of the six selector cases at `0x1B1960`, cases 0 and 5 supply those respective subtypes. The item branch requires a nonzero selected subtype after successive zero-remainder draws modulo 10 and modulo 4. | 2 | 0.1, bits `0x3DCCCCCD` | Selection `0x1B18EA–0x1B1A2B`, gates `0x1B1D72–0x1B1DA7`, scale and origin `0x1B1E94–0x1B1EA6`, append `0x1B1EAD`. |
| Two other feature branches in `0x1AF170` | Feature entry codes 17 and 19 each produce category 25, subtype 0. The embedded item factory assigns `+0x04` from a draw modulo 200. | 0 | Constructor value, `0x3D924925` | Code 17 is tested at `0x1BAFFE` and appends at `0x1BB34D`. Code 19 is tested at `0x1BB35E` and appends at `0x1BB6BD`. Both call `0x1F77B0` and copy only the embedded item through `0x02C5E0`. |
| Late surface vegetation, terrain material 4 | Category 11. After the candidate passes the modulo-150 gate, the producer draws its angle, then uses an even subtype draw for 15 or an odd draw for 22. | 2 for both | Subtype 15 retains `0x3D924925`. Subtype 22 receives 0.1, `0x3DCCCCCD`. | Material and gate `0x1ECB4B–0x1ECB69`, subtype and scale `0x1ECC1C–0x1ECC7B` and `0x1ED152`, common origin and append `0x1ED16F–0x1ED176`. |
| Late surface vegetation, terrain material 10 | Category 11, subtype 24. After the modulo-300 gate and the angle draw, the producer consumes a parity draw. Both ordinary parity results enter the same subtype and scale assignment. | 2 | 0.1, bits `0x3DCCCCCD` | Material and gate `0x1ED180–0x1ED198`, subtype and scale `0x1ED24B–0x1ED295`, common origin and append `0x1ED2B5–0x1ED2BC`. |

The two late material codes come from the low five bits of the terrain byte at offset 3. The outer routine initializes the candidate divisors to 150 and 300 at `0x1EBC40–0x1EBC4A` and obtains the material at `0x1EBCEE–0x1EBCF9` through `0x034C10`. Other terrain selection gates precede these draws. The divisors do not give unconditional per-zone spawn probabilities.

## Order and settling

The early terrain append is in the outer routine at `0x1E4850`. Conditional calls at `0x1E84C3` and `0x1E84F7` enter the generator containing the origin-1 object loot. The call at `0x1E9672` enters the feature generator containing the origin-2 subtype 16/17 branch and the two origin-0 category-25 branches. The material-4 and material-10 vegetation branches belong to the later surface pass. All these records already carry their producer's origin and scale before the final traversal of the zone's item vector.

The final pass at `0x1ED87E–0x1EDA48` visits items in vector order and increments the corresponding item-count entry before it tests bit 0 of the origin byte. A set bit skips the ground check. A clear bit samples half a block below the current item position. If the low-five-bit terrain material there is 0 or 2, the pass lowers the item by one full block, then repeats the half-block-below check until it finds a different material. An initially supported item stays at its current height. This pass leaves scale and origin intact. The original branches give three examples. Origin 1 at scale 0.06 skips settling, origin 2 settles at either 0.1 or the constructor scale, and origin 0 also settles at either scale. Evidence includes `0x1ED915–0x1EDA15`, `0x012220`, `0x02CA00` and `0x03B480`.

The optional ordinary saved-zone overlay follows this pass at `0x1EDD69–0x1EDE46`. A saved record can carry its own position, origin, scale and day. Runtime actor drops and object-released loot that enter through `0x19AEB0` have a separate placement and day-stamping path.

Coverage is bounded to these direct native creation sites and the traced ordinary caller. The room or biome names of every numeric selector, all upstream placement conditions, and any indirect or bulk producer outside these paths remain unresolved.
