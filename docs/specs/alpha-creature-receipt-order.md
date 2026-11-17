# Alpha creature receipt order

Status **FINAL / STABLE / HELD**

Evidence is the original x86 Cube.exe with SHA-256
84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf.
Routine addresses are RVAs with image base 0x400000. Creature offsets
begin at the whole object. Its transmitted record begins at creature
+0x10. The ID stored in the creature begins at +0x08. The world's
ordered creature map is at world +0x04, and the same map is at game
+0x2e8. A map key and the ID stored in its creature are separate inputs.

The receive dispatch selects packet 0 at 0x06baad, packet 1 at 0x06c296
and packet 2 at 0x06be25. The world lock is taken and released through
0x201cb0 and 0x201e90. (Dispatch at 0x06b8b7, tables at 0x06d180 and
0x06d1a4.)

## 1. Packet 1 temporary records and repeated IDs

After its count read, packet 1 makes a temporary ordered map of creature
records. Under the world lock it walks existing world-map entries, skips
null creature pointers and copies each non-null creature's record into
the temporary map under the creature's own ID from +0x08. It then
releases the world lock. This seed walk uses the object's ID as the
temporary key. The later world-map membership pass uses the world's
map key. (0x06c2e0–0x06c359, whole-record assignment at 0x04b040.)

For each wire occurrence, the receiver looks up that occurrence's
eight-byte ID in the temporary record map and passes the resulting
record to the stream field reader. The map accessor returns the
existing value for an equal key. If a key is absent, the accessor
constructs a default record and inserts it, then returns that new
value. Its equality path returns the existing record without a
default construction or a record copy. On successful field receipt,
the receiver inserts the ID into a separate per-message ordered set.
(0x06c3a0–0x06c463, 0x068b70–0x068c58, initializer 0x03c100,
stream reader 0x0cd3e0, set insertion 0x053de0.)

The 64-bit mask applies to the current occurrence. Fields read from
the socket overwrite the corresponding fields of the temporary
record. An unset bit leaves that field in the existing temporary
record. Thus the first occurrence starts from the pre-receive
snapshot when one was seeded, or from the record default for a new
key. Every later occurrence of that ID sees the fields left by the
earlier occurrences in wire order. When more than one occurrence
supplies a field, the last one to supply it determines its value.
A field omitted throughout the batch retains its seeded or default
value. (0x0cd3e0–0x0cd4bb, 0x0cc4c0–0x0cc524,
0x0cc650–0x0cc6b4, 0x0cd97c–0x0cd99d.)

The temporary map and per-message set order IDs by the high 32-bit
word as signed, then by the low word as unsigned when the high words
match. This is ascending signed 64-bit order. Two keys are equal
when both words match. Set insertion of an equal key retains one
entry. The apply pass begins at the leftmost set node and walks its
successors in ascending key order. This order can differ from the
wire occurrence order.
(0x068ba2–0x068bd8, 0x053e62–0x053e83, 0x053ed6–0x053f05,
0x06c60d–0x06c620, 0x06c977–0x06c9bd.)

## 2. Packet 1 live removal and application

After the receive loop, the receiver inserts the local creature's
stored ID into the per-message set. It takes the world lock and
collects non-null world-map entries whose keys are absent from that
set. It destroys and erases the collected entries before applying
any merged record. A normally registered local creature is preserved
by the local ID insertion. (0x06c469–0x06c4be, 0x06c4c3–0x06c541,
0x06c5d6–0x06c60d, membership lookup 0x077d90.)

Under the same lock, each ID in the set is looked up in the live
world map in order. An absent entry or a null creature pointer
causes a live creature to be constructed and stored at that key.
The local creature pointer is skipped at the application check.
For every other distinct set key, after any creation, the receiver
chooses snap or smooth once and assigns the whole final temporary
record once. Repeated wire occurrences contribute to that single
record assignment. There is no intervening live snap or assignment
between two occurrences of the same ID during the receive loop.
If the field reader returns its error sentinel −1, the batch takes
its failure exit before live pruning or application.
(0x06c41c–0x06c427, 0x06c56c–0x06c59d, 0x06c620–0x06c709,
0x06c95c–0x06c9d1.)

## 3. Packet 0 and snap timing

Packet 0 looks up the ID under the world lock. A missing or null
result creates and installs the live creature before field decoding.
It copies the creature's record as the temporary baseline, releases
the lock, then decodes the buffered fields into that copy. The
decoder's −1 exit precedes live record application. A creature
already installed by the first phase stays installed on this exit.
On success the receiver inserts the ID in the persistent seen set
and reacquires the lock. It then skips application if its creature
pointer is the local one. (0x06bbdc–0x06bc8d, 0x06bc93–0x06bccb,
buffered field reader 0x0ccfa0.)

Both receipt cases decide snap before the whole-record assignment.
The cases snap for a newly created creature, a difference in any
of the six 32-bit words of the 24-byte snap field, or ordered live
health at most zero. They compare the temporary snap field at record
+0x1b0 against the current live value at creature +0x1c0 and read
live health at creature +0x16c. Thus packet 1 compares the final
merged field and the live pre-assignment health, after the batch's
pruning and creation decisions. (Packet 0 at 0x06bcd1–0x06bcfe and
0x02c680. Packet 1 at 0x06c69a–0x06c703.)

The smooth arm sets creature +0x1398 to 60, saves displayed position
from +0x1350 into +0x1338, displayed body rotation from +0x1374 into
+0x1368, and displayed aim from +0x138c into +0x1380. Snap sets
+0x1398 to zero, clears the container at +0x11dc, sets displayed
position and body rotation from the temporary record, and saves
that displayed position at +0x1338. The snap arms differ in the
three binary32 values written at +0x1368, +0x136c and +0x1370.

| Receive case | Values saved in the three slots on snap |
|---|---|
| Packet 0 | Newly assigned displayed rotation components at +0x1374, +0x1378 and +0x137c |
| Packet 1 | Newly assigned displayed X, Y and Z, each loaded as signed fixed i64, rounded to binary32 and multiplied in binary32 by 1/65,536 |

These saves precede the whole-record assignment in each case.
(0x06bd00–0x06bdff, 0x06c709–0x06c96c, packet-1 scale 0x2fcd94.)

## 4. Packet 2 and the persistent seen set

The receive thread initializes the persistent seen set at its
entry. Packet 0 inserts into it after field decode and before its
local-creature skip. Packet 1 inserts its wire IDs and the local ID
into the separate per-message set. It destroys that per-message set
after use, without merging it into or clearing the persistent set.
(0x06b7c8–0x06b7e1, 0x06bc93–0x06bcb1, 0x06c35e–0x06c37d,
0x06c445–0x06c48c, 0x06c9c9–0x06ca19, 0x06c582–0x06c598.)

Packet 2 considers each non-null world-map entry other than the
local creature pointer. It destroys and erases entries whose keys
are absent from the persistent seen set, then clears that set.
Packet 1 alone supplies no survival mark for packet 2. A key
marked by a successful packet 0 since the previous clear can
still be kept after packet 1 has applied that key.
(0x06be25–0x06bf7f, 0x077d90.)

This contract covers valid full receipt and the stated reader-error
exits. Full socket failure and allocation behavior, concurrent
replacement or aliasing of live entries, and consumers of the saved
pose fields remain open.
