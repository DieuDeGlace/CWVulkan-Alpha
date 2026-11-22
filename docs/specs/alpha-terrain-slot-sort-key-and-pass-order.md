# Alpha terrain-slot sort key and cross-slot visit order

Source-author hold of 2026-11-21. FINAL / STABLE / HELD.

## Original evidence

This record covers the owner's original 32-bit Alpha client, `binary/Cube.exe`,
SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
Addresses are RVAs from its preferred image base of 0x00400000. The original
client instruction listing, corresponding routine exports, executable bytes
and constants establish the control flow and comparisons below. The numeric
examples use finite keys and representable fixed-point coordinate products.

## How a slot enters the list

The client initializes the grid side length at +0x2DC to 40 (0x059F39). It
allocates the square of that side length in 0x268-byte slots and stores the
array address at +0x2E0 (0x061EEB–0x061F54). The slot constructor at
0x0596E0 initializes its two logical coordinate tags to minus one. It has
no explicit write to the four bytes at +0x6C.

The scene starts with an empty vector of slot pointers at 0x0AC394.
The logical scan begins at client +0x2AC for X and +0x2B0 for Y. X is
outermost and increases by one. For every X, Y restarts at its origin
and increases by one. Both stop before their origin plus the current side
length. Only coordinates in 0 through 524,287 inclusive on both axes
reach the slot lookup. The physical slot index is the remainder of Y
divided by the side length, times the side length, plus the remainder of
X divided by the side length. Hence the admission order follows the
logical coordinates even across physical array wrap. See 0x0AC4E0–0x0AC586
and 0x0AD442–0x0AD472. A 40-by-40 window can admit at most 1,600 pointers.

At 0x0AC658–0x0AC6DE the scene holds the slot's critical section at +0x250.
It requires a nonzero resident count at +0x0C, exact X and Y tag matches
at +0x18/+0x1C and acceptance by the box test at 0x07F3C0. Success appends
the slot pointer at the end of the vector, with growth as needed
(0x26ADD0). The scene then writes the key on that physical slot. Failure
skips both the append and the key write. It does not replace the key with
a sentinel. The slot lock is released later at 0x0AD42D–0x0AD434.

## Key calculation and another reader

At 0x0AC6E3–0x0AC737 the scene subtracts the client reference at +0x140,
+0x148 and +0x150 from the slot reference at +0x50, +0x58 and +0x60.
Each component is a signed 64-bit world coordinate with 65,536 raw units
per world unit. The differences are formed in raw integer coordinates
(0x02C7A0). Each component is squared and divided by 65,536 with signed
64-bit arithmetic, truncating each quotient toward zero. The three integer
contributions are added in X, Y, Z order (0x03AC20). The sum is converted
to binary32, then multiplied by binary32 1/65,536 (constant RVA 0x2FCD94).
That final rounded world-distance squared is stored at slot +0x6C. There
is no square root. Separate quantization and rounding can produce equal
keys for different slot references.

During this same grid scan the scene also walks the slot's collection at
+0x240. At 0x0ACAB3–0x0ACAC5 it reads the current slot key and permits the
following record-specific time/type work only for a key strictly less
than 4,096 (constant RVA 0x302B80). Equality skips that work. The walk is
reachable from the admission-failure branches via 0x0AC73C, so it can
read a value retained from an earlier visit when a record is present.
The initial value of that field before its first proved write is unresolved.
This test occurs before slot sorting and is independent of the list
membership decision. It does not impose a 64-world-unit admission radius
on the terrain passes.

## Sort, ties and the two terrain passes

Before sorting, a nonzero client byte at +0x8006E6 clears the pointer
vector by setting its end to its beginning (0x0B1AF4–0x0B1B09 and
0x04BE20). This leaves the previously written slot keys in place.
The sort at 0x0B1B48 takes the whole remaining vector. Its entry at
0x0ABA70 measures the count in four-byte pointers and invokes 0x0AA260.
Comparisons use the binary32 +0x6C values in ascending order. Logical
coordinates, physical slot indices, pointer values and arrival ordinals
supply no secondary comparison.

For zero or one entry there is nothing to reorder. For an entire input
of two through 32 entries the native insertion pass (0x0A9320) moves
preceding slots only when their keys are strictly greater. Equal keys
therefore retain the logical X-then-Y admission order on this path.

For larger inputs the native partitions the pointer range, including
exchanges between nonadjacent positions (0x0AA980 and 0x0A9BF0).
It uses insertion sorting on short subranges and has a heap fallback
(0x0A96B0 and 0x0AA6C0). Equal-key arrival order is no longer guaranteed.
For a concrete ordering example, take 33 pointers, the first 32 with key
two and the last with key one. The three-sample step at 0x0A9D66–0x0A9DA5
moves the last pointer to the front and the original first pointer to
position 16, counting from zero. The original second pointer stays at
position one. The equal-key group then spans positions one through 32.
These two formerly adjacent equal-key pointers have reversed relative
order. This is an instruction-derived example, independent of map layout.
The cutoff of 32 selects a sorting method and never caps the draw count.

The first terrain pass walks from the beginning to the end of this sorted
vector (0x0B1B75–0x0B1BD7 and 0x0B2169–0x0B21A6). Distinct keys are visited
nearest first. The scene locks each slot again. An empty resident collection
at slot +0x08 skips its draw work. Otherwise that collection is walked in
its own forward order. The terrain submission requires its entry's +0x08
resource and uses the entry's +0x14 count (0x0B1C23–0x0B1DD8).
After this, the same slot's +0x240 collection is considered under a separate
horizontal range test. It recomputes the squared X/Y distance in binary32
from the slot/client references, permits equality, and compares against the
square of 32 plus the rounded product of client +0x1D0 and binary32 0.2
(0x0AD9D1–0x0AD9FC and 0x0B1DDE–0x0B1E71). These records are visited in
forward list order before the next slot. Their own eligibility still applies.

The later terrain pass reuses the same sorted pointer vector, starting
at its end and stepping back one slot at a time (0x0BA1AE–0x0BA213 and
0x0BA3E5–0x0BA432). The last pointer from the first pass is the first here,
including within any tie. Slot visitation is farthest first for distinct
keys. Each slot is locked separately. Its +0x08 collection is still walked
forward. This pass instead requires the entry's +0x0C resource and uses
its +0x18 count (0x0BA21B–0x0BA3D8). Thus reversing the outer traversal
does not reverse the resident entry order. Both passes can skip individual
submissions, so actual draw calls need not be mirror images.

## Which model records inherit this order

For ordinary eligible models from a slot's +0x240 collection, the first
pass tests the model table at client +0x800718. At 0x0B25AF–0x0B25F2 the
ordinary branch checks the signed index, skips index 18 and requires a
non-null model pointer. Index 60 branches separately at 0x0B1EB4.
Further tests can reject, draw immediately at 0x0B2801, or append a
16-byte deferred record with its own per-record squared distance at
0x0B276C–0x0B27B1. The enclosing slot order governs the visitation and
the insertion sequence of these records. Deferred consumption has its
own ordering.

The separate zone collection at zone +0x0C resolves model indices against
client +0x800724. Its coordinate traversal begins after the forward slot
pass at 0x0B21C7 and enters the indexed collection at 0x0B2C95. Immediate
submissions follow zone/record visitation. It can also append to the shared
deferred collection (0x0B308D). That collection is sorted at 0x0B332C by
its own binary32 field at record +0x08, larger values first, and consumed
forward after the reverse terrain pass at 0x0BA467–0x0BA588. Its 16-byte
records and sort entry at 0x0ABAC0 differ from the terrain pointer vector.
The slot +0x6C key supplies no cross-zone ordering or per-model fade.

This establishes ordering in the specified scene passes, subject to their
admission and resource gates. It does not establish a universal tie rule
for other containers, a default for uninitialized slot storage, every
possible writer through an indirect alias, or final framebuffer visibility.
