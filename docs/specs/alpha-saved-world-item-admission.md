# Saved world-item admission in a single-player Alpha zone

FINAL / STABLE / HELD

The evidence is the original 32-bit Windows Alpha client, Cube.exe, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`. All addresses below are RVAs from image base `0x00400000`. The findings apply to complete, ordinarily produced zone payloads. Source listings and the relevant branches in the executable were checked directly.

## When the saved row is read

The client populates the zone before looking up its saved row. In the outer zone routine at `0x1E4850`, the lookup at `0x1EDD69–0x1EDE46` requires world mode byte `+0xB4` to be zero and the world save-name length at `+0xA4` to be nonzero. The row key is `zone`, the first decimal zone coordinate, an underscore, and the second decimal zone coordinate. Only a successful lookup calls the zone decoder. A missing row leaves the items already in the zone in place. An existing row with zero saved items takes the comparison path below and can clear them. Evidence includes `0x077220`, `0x0498D0` and `0x04CDB0`.

## Item section and origin

The ordinary writer emits a four-byte version of 1, a four-byte item count, then the records in resident vector order. Offsets below are from the first byte of each record. The archive is little-endian. Each item takes 325 bytes (`0x145`) on disk and 328 bytes (`0x148`) in the vector at zone `+0x30`. Evidence includes the archive initializer at `0x04A8A0`, decoder at `0x04CDB0` and writer at `0x04E340`.

| Disk offset | Size | Contents |
| --- | ---: | --- |
| `0x000` | 280 | Embedded item data. The first two bytes identify category and subtype. The writer copies this entire span from the resident item. |
| `0x118`, `0x120`, `0x128` | 8 each | Signed 64-bit world X, Y and Z positions, in order. |
| `0x130`, `0x134` | 4 each | Binary32 angle and scale fields. The placement routine generates the angle on a 360-degree scale and multiplies the initial scale by its scale input. |
| `0x138` | 1 | World-item origin/type byte, separate from the embedded category and subtype. |
| `0x139`, `0x13D` | 4 each | Two 32-bit fields from resident `+0x13C` and `+0x140`. The constructor zeroes both. The ordinary placement routine assigns the first a random integer from 500 through 799. Their complete gameplay meanings are unresolved here. |
| `0x141` | 4 | Signed saved day from resident `+0x144`. The three alignment bytes after the resident origin byte occupy no space in the archive. |

The constructor at `0x04A970` initializes origin to 0 and day to −1. The world placement routine at `0x19AEB0` keeps origin 0, stamps the current world day from `+0x800160`, and appends to the resident zone when zone and placement selection succeed. Its append site is `0x19B140`. A stamp at placement does not by itself establish that a row write takes place.

Zone population also appends records with the constructor day of −1. Two branches at `0x1ECB6F–0x1ECC7B`, `0x1ED152–0x1ED176` and `0x1ED19E–0x1ED2BC` produce category 11, origin 2 items, including subtypes 15, 22 and 24. Another producer selects origin 1 before appending at `0x1D3292–0x1D3560`. Further population paths at `0x1BB273–0x1BB34D` and `0x1BB5E3–0x1BB6BD` set the position and embedded item while retaining origin 0 and day −1. Consequently origin 0 alone cannot distinguish a dated runtime placement from these generated items. The saved byte is retained as data. The reader imposes no origin, category or subtype admission rule.

## The signed day test

The decoder reads the final four bytes of each saved item and the current world day. It subtracts three from the world day with 32-bit arithmetic and compares the results as signed integers. It admits a saved day at or above that threshold. It also admits every negative saved day, including values other than the constructor sentinel −1. It drops a nonnegative saved day below the threshold. The decoder appends each survivor in input order. There is no upper limit on an admitted future day and no restamping, sorting or deduplication at this step. The original branch is at `0x04D028–0x04D051`.

| World day | Saved day or ordered input days | Result on read |
| ---: | --- | --- |
| 20 | 16, 17, 20, 21, −7, −1, 17 | 17, 20, 21, −7, −1, 17 survive in that order. |
| 21 | 17 | Excluded. The same stamp was still eligible on day 20. |
| 2 | 0 | Admitted. The threshold is −1. |
| −2,147,483,648 | 0, 2,147,483,644, 2,147,483,645, −2,147,483,648 | The last two survive. The native subtraction wraps to the signed threshold 2,147,483,645. Reachability of this world day through ordinary play is unestablished. |

## Comparison with resident items

The decoder compares the survivor count with the resident count, then compares corresponding records in order when the counts agree. The item comparison checks the embedded category and subtype, four-byte fields at `+0x04` and `+0x08`, bytes at `+0x0C` through `+0x0E`, the two-byte field at `+0x10`, and the count at `+0x114`. For each active eight-byte entry starting at `+0x14`, it compares its first four bytes. It also compares the three positions, both binary32 values numerically, the origin byte and the next two 32-bit fields. The saved day does not participate. Evidence includes `0x028940`, `0x02F4A0` and `0x049CE0`.

A count or value difference sets zone byte `+0x75` to 1 and replaces the entire resident item vector with the survivors, including their days. When the lists compare equal, the resident records, their days and the prior value of the flag stand. At world day 20, a resident item stamped 19 and an otherwise equal saved item stamped 17 therefore leave the resident stamp at 19. Changing the saved origin alone makes that comparison differ and installs the saved item with day 17. An empty survivor list clears a nonempty resident list. Evidence includes `0x04D05C–0x04D0C8`, `0x04ABB0`, `0x04A8C0` and `0x04B3B0`. The native copy helpers operate on embedded item fields, so this contract does not promise that unused item padding round-trips.

## Writeback and subsequent record counts

Unload at `0x1A4890` detaches the resident zone and, in mode zero, passes it to the writer before disposal. World destruction and the setup routine's resident-zone teardown also visit zone slots in mode zero, with calls at `0x193F55` and `0x1A5585`. The wrapper at `0x1A5050` needs a nonempty save name, a nonnull zone and either a nonzero `+0x75` flag or nonempty terrain modifications at `+0x68`. It invokes the serializer at `0x1A51B8` and the keyed database write at `0x1A51DB`. Skipping this wrapper's write leaves an existing row in place.

| Zone flag and modifications | Ordinary result, assuming the other write gates pass |
| --- | --- |
| Flag clear, no modifications | Skip the row write, even with resident items. |
| Flag clear, modifications present | Write the row with item count zero. |
| Flag set, with or without modifications | Write the current resident item count and every record in order, including an empty list. |

The writer applies no day, origin or item-category filter and preserves the resident day. An item stamped 17 still resident when saved on day 21 can be written with stamp 17 and excluded by a later load on day 21. Aging alone does not trigger this writer or assert the flag. A separate client update loop sets the flag for each zone in a collected set at `0x221180–0x2211A6`. Membership for every possible gameplay action and the timing of any independent live-item removal remain outside this record.

Three more counts follow the items. First come 20-byte terrain modification records. Their own final signed day receives the same threshold and negative-day exception on read, followed by terrain application and retention of accepted modifications. A positive source count also triggers a support check on eligible existing scenery. Next the writer records two four-byte fields per existing pointer entry in zone collection `+0x18`, taken from object offsets `+0x38` and `+0x3C`. Finally it records one byte at `+0x30` per existing 392-byte object in zone collection `+0x0C`. Each of those last two readers overlays by index only when its count equals the corresponding resident count. The ordinary serializer emits no full dynamic-object identity, transform or object record for either overlay. These later counts never gate or alter the preceding item admission or replacement. Evidence includes `0x04D157–0x04D2CB`, `0x04D305–0x04D42B`, `0x04E340` and `0x04E7A0`.

This record establishes static client behavior for the ordinary zone row. The complete origin enumeration, gameplay identities of the two overlay collections, all causes of zone writes, and runtime observations of save or reload remain open.
