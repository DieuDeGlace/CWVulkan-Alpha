# Alpha data stores: how Cube World Alpha keeps its content

Behavioural specification of the four content stores of the Cube World **Alpha** client
(`Cube.exe`, SHA-256 `84a7a132…30bf`). Facts come from the Alpha resource survey in
`disassembled alpha cube world/resources` (`decoder-provenance.json`, `asset-summary.json`,
`asset-manifest.csv`) and the client's decoder routine at address 0x4496a0. Nothing here is
code; AlphaVulkan's loader is written from this page only.

## 1. Files

The install folder (the one holding `Cube.exe`) has four SQLite 3 databases. Each has one
ordinary table, `blobs(key TEXT PRIMARY KEY, value BLOB)`. The key is the asset's file name;
the value is the asset's bytes, scrambled (section 2).

| Store | Rows | Holds |
|---|---:|---|
| `data1.db` | 2,857 | voxel models (`*.cub`) |
| `data2.db` | 93 | sounds and music (`*.wav`, RIFF/WAVE) |
| `data3.db` | 104 | interface pictures (`*.png`) |
| `data4.db` | 2 | the dictionaries `dict_en.xml`, `dict_de.xml` |

Keys are unique within a store. A loader opens the files read-only and only selects from
`blobs`; nothing else in the database is run.

## 2. Unscrambling a value

The client unscrambles a value of *n* bytes in place with a fixed table *T* of 44 unsigned
32-bit numbers (stored in the client at VA 0x6ffa68):

    4242, 9551, 840, 84800, 9242, 9846, 127, 9, 9483, 394, 123,
    4834, 32444, 24355, 2433, 17, 34234, 42342, 4243, 14, 184934, 1987,
    3094, 1901, 89409, 4813, 37, 143, 3490, 19483, 1343, 432, 84732,
    9184, 9612, 1233, 3434, 1839, 2984, 1993, 2984, 4895, 816583, 13

1. For *i* from *n* − 1 down to 0: swap byte *i* with byte *j* = (T[*i* mod 44] + *i*) mod *n*,
   the sum taken as an unsigned 32-bit number.
2. Then replace every byte *b* by its complement, 255 − *b*.

Scrambling is the exact reverse: complement every byte, then do the swaps for *i* from 0 up
to *n* − 1. Every one of the 3,056 stored values round-trips through this pair byte for byte.
The unscrambled bytes are the plain file: no further compression (none of the values is a
complete zlib stream).

## 3. Voxel models (`.cub`)

Same layout as the Steam release: three little-endian int32 sizes *x*, *y*, *z*, then
*x*·*y*·*z* RGB byte triples, *x* varying fastest, then *y*, then *z*. (0, 0, 0) is an empty
voxel. Every model in `data1.db` has exactly 12 + 3·*x*·*y*·*z* bytes. How models are shaded is
in `alpha-rendering.md` §11.1.

## 4. Open questions

* Which code paths ask for which keys (sprite tables, sound ids) is not yet specified.
* `db.dat`, `resource1.dat`, `resource2.dat` and the `.plx` interface files are separate
  formats, not covered here.
