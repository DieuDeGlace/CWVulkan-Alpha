Status: FINAL / STABLE / HELD

# Alpha data stores: how Cube World Alpha keeps its content

Behavioural specification of the four content stores used by the Cube World **Alpha**
client image `Cube.exe` (SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`,
image base 0x400000; addresses below are virtual addresses in that image). Store facts were
checked read-only against the installation's `data1.db` … `data4.db` files whose SHA-256
values are listed in section 6.

## 1. Files

There are four SQLite 3 databases. Each contains exactly one table, `blobs`, declared as
`blobs(key TEXT PRIMARY KEY, value BLOB)`, plus SQLite's automatic primary-key index and
nothing else (no views, no triggers). In every row the key is stored as text and is the
asset's file name; the value is stored as a blob holding the asset's bytes in scrambled
form (section 2).

| Store | Rows | Holds | Opened by the routine at |
|---|---:|---|---|
| `data1.db` | 2,857 | voxel models, every key ending `.cub` | 0x4ee230 |
| `data2.db` | 93 | sounds and music, every key ending `.wav`, unscrambled content is RIFF/WAVE | 0x622da0 |
| `data3.db` | 104 | interface pictures, every key ending `.png` | 0x459c40 |
| `data4.db` | 2 | the dictionaries `dict_en.xml` and `dict_de.xml` | 0x4e1970 |

Keys are unique within each store. No value is empty.

### 1.1 How the client uses a store

* Opening (routine 0x4497b0): any previously open handle on the same store object is closed
  first; the file is then opened in SQLite read-write mode with create-if-missing; on
  success the client issues `CREATE TABLE blobs(key TEXT PRIMARY KEY, value BLOB);` and
  ignores its result (on an existing store the statement simply fails). The store file
  names are relative names. For `data1.db` the name is appended to a directory string held
  by the caller, which is empty unless set; where that string comes from is unresolved.
* Reading (routine 0x449810): the client prepares
  `SELECT value FROM blobs WHERE key = ?`, binds the key as text, and steps once. A row
  yields a copy of the whole value; no row means "not found". If the statement ends with
  SQLite's schema-changed result code (17) the read is retried; any other failure is
  reported as "not found".
* The same store module also contains insert-or-update by key (0x4499f0, with the console
  message `DATABASE WRITE ERROR` on an insert failure), delete by key (0x449720) and rename
  key (0x449920). Whether any of these run during normal play is unresolved.
* Three loaders also have a path that reads a loose file from disk instead of a store
  value: the sound loader (0x623a60) when its sound store handle is absent, the picture
  loader (0x486a20) when called without the store option or when the key is not found, and
  the model loader (0x4e7290) when its caller selects the file path. The dictionary loader's
  behaviour for a missing key is unresolved.

## 2. Unscrambling a value

Every consumer of a store value (the routines at 0x486a20, 0x4e1970, 0x4e7290 and 0x623a60)
first unscrambles the whole value in place with the routine at 0x4496a0. That routine uses a
fixed table *T* of 44 unsigned 32-bit little-endian numbers held in `.rdata` at
VA 0x6ffa68 (file offset 0x2fea68, 176 bytes):

    4242, 9551, 840, 84800, 9242, 9846, 127, 9, 9483, 394, 123,
    4834, 32444, 24355, 2433, 17, 34234, 42342, 4243, 14, 184934, 1987,
    3094, 1901, 89409, 4813, 37, 143, 3490, 19483, 1343, 432, 84732,
    9184, 9612, 1233, 3434, 1839, 2984, 1993, 2984, 4895, 816583, 13

For a value of *n* bytes:

1. Walk the position *i* downward from *n* − 1 to 0. At each step add *i* to the table entry
   at position *i* mod 44, keep the sum as an unsigned 32-bit number, reduce it modulo *n*
   to obtain the partner position *j*, and exchange the bytes at *i* and *j*.
2. After the walk, replace every byte *b* with 255 − *b*.

A value of length zero is left untouched. The inverse (scrambling) complements every byte
first and then performs the same exchanges with *i* rising from 0 to *n* − 1. All 3,056
stored values reproduce their stored bytes exactly when unscrambled and then scrambled
again.

The unscrambled bytes are the plain asset file. No value, once unscrambled, is a zlib
stream, and the loaders do not inflate store values.

## 3. Voxel models (`.cub`)

The model loader (0x4e7290, when reading from a store) unscrambles the value, reads three
little-endian signed 32-bit sizes *x*, *y*, *z* from offsets 0, 4 and 8, and then copies the
following bytes as red-green-blue byte triples (a whole number of triples, 3·*x*·*y*·*z*
bytes). Behaviour for a value too short to hold its header or its triples is unresolved.
The voxel at (*px*, *py*, *pz*) is the triple at index
(*pz*·*y* + *py*)·*x* + *px*, so *x* varies fastest, then *y*, then *z* (model setup routine
0x4e7650). Voxel data is retained only when all three sizes are positive.

Every value in `data1.db` is exactly 12 + 3·*x*·*y*·*z* bytes long with all three sizes
positive.

The colour (0, 0, 0) means "no voxel": the client compares voxels against a three-byte
constant at VA 0x76b340, which lies in the zero-filled tail of `.data` and has no direct
writer in the image, and it also returns that constant for any position outside the model.

When a model is set up with its marker flag clear, the setup routine records the positions
of voxels whose colour is exactly pure red (255, 0, 0), pure green (0, 255, 0) or pure blue
(0, 0, 255) in three separate lists, and the per-voxel test at 0x4e71d0 treats those three
colours, like (0, 0, 0), as not drawn. With the flag set, only (0, 0, 0) is treated as not
drawn. How the marker lists are consumed, and how drawn voxels are shaded, is outside this
contract.

## 4. Other loaders

* `data3.db` pictures are handed on by the picture loader (0x486a20), after unscrambling,
  as an in-memory buffer with its length. Every unscrambled value begins with the PNG
  signature.
* `data4.db` dictionaries begin with a UTF-8 byte-order mark followed by `<`. The image
  stores the literal key prefix `dict_en`; how the German dictionary key is chosen is
  unresolved.
* `data2.db` values begin with `RIFF` and carry the form type `WAVE`.

## 5. Open questions

* Which code paths request which keys (sprite tables, sound ids) is not specified here.
* `db.dat`, `resource1.dat`, `resource2.dat` and the `.plx` interface files are separate
  formats, not covered here.
* The source of the directory string prefixed to `data1.db`, and whether the four store
  names always resolve to the installation folder.
* Whether the write, delete and rename operations of the store module are ever used.
* The dictionary loader's behaviour for a missing key; model-loader behaviour for a
  truncated value.

## 6. Evidence identities

* `data1.db` SHA-256 `0e482f6d628414db017d983f62843e5dbdc52c5b54fd478f6f0e06c786639b9d`
* `data2.db` SHA-256 `e2231a7478d882d6382a4146ee70f1dae67662b58c8a9188fbcc294061b550fb`
* `data3.db` SHA-256 `e41dbde87bd58142d73bc22e0d3e9fd61fdb340e80f1e1e84b674edc63430438`
* `data4.db` SHA-256 `d859dd14eb855aa8b08dc5fae2062d7b722060b9204682ac5cabbc4af971599e`

## Re-certification (2026-11-14)

Confirmed against the original image and store files:
* Executable identity; table address, file offset, length and all 44 values; the downward
  exchange walk with the unsigned 32-bit sum modulo *n* followed by complementing (routine
  0x4496a0); the inverse order.
* One `blobs(key TEXT PRIMARY KEY, value BLOB)` table per store; row counts 2,857 / 93 /
  104 / 2; unique keys; extensions per store; 3,056 exact round trips; no zlib streams;
  every model length equal to 12 + 3·*x*·*y*·*z*.

Corrected:
* Old: the content stores "live in the install folder". New: opened by relative names,
  `data1.db` with a caller-held directory prefix; resolution unresolved.
* Old: "a loader opens the files read-only and only selects". New: the native client opens
  read-write with create-if-missing, issues a `CREATE TABLE`, reads with a keyed select with
  a schema-change retry, and also carries write/delete/rename operations.
* Old: voxel order stated by analogy with the Steam release. New: order established from the
  native index computation at 0x4e7650; the Steam comparison is dropped.
* Old: "(0, 0, 0) is an empty voxel" without evidence. New: tied to the zero constant at
  0x76b340 and the out-of-range rule; marker-colour behaviour added.
* Removed the meta sentence about who reads the page and the unverified cross-reference to
  another contract's section.
* Added the four opening routines, the four consumers of the unscrambler, and the store
  identities.

Unresolved: listed in section 5.
