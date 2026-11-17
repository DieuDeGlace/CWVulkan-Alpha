Status: FINAL / STABLE / HELD

# Alpha ruling: SpawnLocation new-entry rounds and ruin creature routes

Scope: (1) how many search rounds SpawnLocation runs in the update in which its current
schedule entry changes; (2) the WalkPath route of the ruin (location type 5) mission
creatures built by the town builder; (3a) whether ring creatures store their own route point
exactly; (3b) whether open-plot ruin creatures get behaviours.
Images: client Cube.exe (SHA-256 84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF)
and Server.exe (SHA-256 1FFBCBCE251E1506A63C9CE18AB0009D5553FCBB9E6202BE2BC3A7D870D34FB1),
both 32-bit, image base 0x400000. Addresses below are virtual addresses ("client 0x…",
"server 0x…"); RVA = VA − 0x400000. "Draw" = one call of the C-runtime rand() through the
client import slot 0x6fc468. fx = 64-bit fixed point, 65536 per block.

## 1. SpawnLocation, new entry (server 0x428940, client 0x4dfad0)

### Ruling

**alpha-trails-poses-schedules.md §3.3 step 3.4 is right; alpha-path-search.md §4.6 is wrong
for SpawnLocation.**

In the update in which the current index changes and ground is found, SpawnLocation runs, in
this order: search start (server 0x4dd2e0 / client 0x5a9f90), arrival radius := 4.0
(0x40800000), **exactly one** search round (server 0x4dde90 / client 0x5aaab0), **one**
path rebuild (server 0x4dafe0 / client 0x5a7c90), intent (obj +0x40, +0x44, +0x48) := 0,
and returns **handled** (the result byte is set to 1: server 0x428bda, client 0x4dfd6a).
It returns directly from there (server 0x428bd8 … 0x428bea); the 10-round loop of the
"on the way" branch (server 0x428d4b … 0x428dda) is **not** entered in that update. The
search start makes no round of its own (it calls neither round routine: server 0x4dd2e0,
client 0x5a9f90 call lists). So **at most one cell is expanded** in that update, not 11.

From the next update (same entry, not yet arrived) the normal loop of up to 10 rounds runs,
each preceded by the stop tests. Over the first two updates the search therefore expands at
most 1 + 10 cells.

Ground fit before the start (unchanged from trails §3.3 step 3.3, re-read): the raise loop
tests the block at the goal at most 11 times (counter 0 … 10, server 0x428b1e … 0x428b22),
raising the goal by one block after each solid test; 11 solid tests → result 0 (not handled,
server 0x428b24 … 0x428b36). The lower loop then tests the block one below the goal at most
11 times (server 0x428b99 … 0x428b9f), lowering by one block after each non-solid test; no
solid block in 11 tests → result 0. "Solid" = the low five bits of the block's byte +3 are
neither 0 nor 2. On either failure no search is started and no intent is written; the index
(obj +0x1488) was already updated (server 0x4289d5) and the search state already dropped
(server 0x4289c8), so the next update sees an empty path and records arrival.

**Draws: none.** SpawnLocation calls no random routine on any path (server call list: 0x405330,
0x54a946 (float-to-integer), 0x406050, 0x4dd2e0, 0x4dde90, 0x4dafe0, 0x405450 and the stack
check only). Client and server bodies are parallel at these points (client 0x4dfcaf /
0x4dfd2a counters, 0x4dfd34 start, 0x4dfd3c radius, 0x4dfd46 round, 0x4dfd4e rebuild,
0x4dfd6a result 1).

### Which contract statement is right

* **Right:** trails §3.3 step 3.4 ("run **one** round and rebuild the path once, intent := 0;
  **handled**") and the §3.4 row "new entry, ground found, search started | handled".
* **Wrong:** path-search §4.6 second paragraph as it applies to SpawnLocation ("run one
  additional round … before this loop, so up to 11 cells are expanded in that update"). For
  SpawnLocation it must read: one round and one rebuild after the start, then return handled;
  the loop is not run in that update. This ruling does not re-examine the Companion and
  WalkPath callers named in the same sentence.

## 2. Ruin group creatures: the WalkPath route (client 0x5bef3b … 0x5bf2cc, in 0x5af170)

These are the third mission-creature site of trails §3.6 (sequence constructor call
0x5bf109). They are the **group** creatures of alpha-worldgen-structures.md §5.5.9 "Ruins
(type 5)" step 3, second bullet.

### Ruling

**alpha-worldgen-structures.md §5.5.9 is right; alpha-trails-poses-schedules.md §3.6 is wrong
for this site.** The route holds the creature's own position first and, conditionally, a
second point taken from a randomly chosen house's spawn list.

Per house (with a non-empty spawn list; P = a copy of that house's spawn list, house +0x24,
made at 0x5beae3), after the ring creatures:

1. **Draw** (0x5bef41): m = d & 1 groups (sign-corrected; equal to d & 1 for rand values).
2. For each of the m iterations, in order; the iteration ends the loop if P is empty
   (0x5bef6a):
   1. **Draw** (0x5bef81): index = d mod |P| (unsigned); that block point is copied
      (converted to fx by × 65536 per axis, 0x5bef99) and removed from P (0x5befb0 … 0x5befc8).
   2. **Draw** (0x5befda): group = GL[d mod |GL|] (unsigned). If the group's list a is empty
      (0x5beff6) the iteration ends here: no creature and no further draw in it.
   3. New spawn; position := (x, y, z + 1 block) of the point, all whole blocks (0x5bf021 …
      0x5bf07c; the + 1 block at 0x5bf03f).
   4. **Draw** (0x5bf081): facing +0x54 = f32(f32(d) × 360.0) ÷ 32767.0 (floats at 0x6fce6c,
      0x6fd338). +0x34 := location +0x24; +0x28 := 1; +0x7a |= 0x1000.
   5. **Draw** (0x5bf0c8): kind +0x2c = a[d mod |a|] (unsigned). +0x58 := low byte of
      location +0x28.
   6. Behaviour: Sequential (0x5bf109); child **Combat** (20.0, 0x5bf13f) appended; a
      **WalkPath** constructed with **2.0** (0x40000000 at 0x5bf183).
   7. **Route point 1** (0x5bf1ac): an exact 24-byte copy of the spawn's own position
      +0x10 … +0x27 (append routine 0x5bf4f0 copies the three 64-bit values unchanged).
   8. If the zone's house list (zone +0x88, the same list the per-house loop walks) is not
      empty (0x5bf1b7; always true here, since the current house is in it):
      * **Draw** (0x5bf1d1): house = list[d mod |list|] (unsigned). Any house may be chosen,
        including the current one.
      * If that house's spawn list (house +0x24, the original list, not P) is **empty**
        (0x5bf1f4): nothing is added. The house draw was consumed and its selection is not
        stored anywhere (this is the draw whose value is not kept).
      * Otherwise **draw** (0x5bf206): index = d mod |list| (unsigned); **route point 2**
        := that block point converted to fx by × 65536 per axis (0x5bf223), appended
        (0x5bf235). It is **not** raised by one block (unlike the creature's own position)
        and not fitted to the ground. It may be a point already removed from P, or the very
        point the creature stands on (then the two route points differ by one block in z).
   9. The WalkPath child is appended after Combat (0x5bf256); +0x109c := the Sequential
      (0x5bf25b).
   10. **Draw** (0x5bf261): e = d & 1 (sign-corrected); an item (category 1, subtype 1, level
       = the spawn's +0x34 as 16 bits) is added e times to +0xf6c.
   11. Append the spawn to zone +0x18 (0x5bf2c7).

So the route is **[own position]** or **[own position, one spawn point of a random house]**,
in that order; **radius 2.0**. The sequence is Combat 20, WalkPath 2 only (no LookAtPlayer,
SpawnLocation, RandomInteraction or RandomWalk). Draws per iteration: 2 when list a is empty;
otherwise 2 + 2 + (1 or 2) + 1, the "1" case being an empty spawn list at the chosen house.

### The other two mission-creature sites (re-read; trails §3.6 is right for them)

* **Ruin kind-20 plot creature** (sequence 0x5be6a9): Combat 20 (0x5be6df), WalkPath 2.0
  (0x5be72e), route = **one point**, an exact copy of its own position (0x5be752). No draw
  after the kind draw. Also +0x7a |= 0x1000 | 0x200 and +0x10e8 := 1 (0x5be678 … 0x5be68d).
* **Ruin ring creature** (sequence 0x5bedc2): Combat 20 (0x5bedf8), WalkPath 2.0
  (0x5bee44), route = **one point**, an exact copy of its own position (0x5bee61); then the
  item draw e = d & 1 (0x5bee8a).

### Which contract statement is right

* **Right:** structures §5.5.9 Ruins step 3 second bullet ("if the house list is non-empty: a
  draw selects a house, and if its spawn list is non-empty a draw adds one of its points to
  the creature's route").
* **Wrong:** trails §3.6 table row "Mission creatures … WalkPath (r = 2, one point: its own
  spawn point)" as a statement about all three sites. It holds for 0x5be6a9 and 0x5bedc2;
  for 0x5bf109 the route is own position plus the conditional second point above. The "plots
  they serve" are the ruins (location type 5) branch of the town builder: kind-20 plots
  (0x5be6a9), house rings (0x5bedc2) and house groups (0x5bf109).

## 3. Further determinations

### 3(a) Ring creatures' own route point: exact, not truncated to whole blocks

Both kinds of ring creature store their own position **exactly** as route point (a 24-byte
copy of +0x10 … +0x27; no conversion, no rounding):

* **Ruin house rings** (town builder): position = centre (block point × 65536) + (X, Y, one
  block), where Y = trunc64(f32(f32(f32(sin a) × 2.0) × 65536.0)) is added to y and X likewise
  with cos to x (helpers 0x424b50 (sine, evaluated first), 0x40e420 (cosine), 0x4122e0
  (× 65536.0 float at 0x6fce7c, float-to-64-bit truncation 0x68d946, add); 2.0 is the float at
  0x745e10). The position has sub-block x and y; the route copy (0x5bee56 … 0x5bee61) keeps
  them.
* **Location rings** (alpha-location-creatures.md §4.3.2, routine 0x5dc6e0): the WalkPath
  (2.0, 0x5deb89) receives a copy of +0x10 (0x5deb9b … 0x5deba6, the same append routine
  0x5bf4f0). Exact, with the radius-3 offsets.

The only truncation is the 1/65536-unit truncation used to form the position itself.

### 3(b) Open-plot ruin creatures do get behaviours, at activation

The town builder writes **no** behaviour for them (client 0x5be7d5 … 0x5bea05: +0x109c is
not written, so it keeps the constructor's 0; structures "no AI objects" is right about the
builder). At activation (client 0x60c510, test at 0x610208; server 0x5322d0, test at
0x535fc8), a spawn whose +0x109c is null and whose controller kind +0x28 is not 6 receives a
**default sequence** built on the spot, stored in the creature's behaviour field
(obj +0x13e4, 0x610370):

1. **Combat** constructed with 20.0 (0x61027d);
2. **LookAtPlayer** (constructor 0x4c8130, 0x6102c6);
3. **RandomWalk** (constructor 0x4dbf10, 0x610342), added only if the creature's controller
   byte obj +0x60 ≠ 6, its class byte obj +0x140 is none of 0x80 … 0x87, and bit 0x40 of
   obj +0x7e is clear (0x6102ee … 0x610326). obj +0x60 and obj +0x140 are copied from spawn
   +0x28 and +0x30 (0x61002b, 0x610037); obj +0x7e lies in the appearance block copied from
   spawn +0x74 (0x610052 … 0x610059).

For open-plot ruin creatures (+0x28 = 1, class +0x30 = 0 from the constructor) the sequence
is therefore Combat 20, LookAtPlayer and, unless that appearance bit is set, RandomWalk. No
WalkPath (so no home route), no SpawnLocation, no RandomInteraction. The default build makes
no draw. A spawn that does carry a behaviour (+0x109c non-null) gets a clone of it through the
behaviour's second virtual method (0x610211 … 0x61021c). The server activation has the same
tests and children (0x535fe3, Combat 20.0 at 0x53603d, the class tests 0x5360ae … 0x5360da).

## Evidence

Static instruction listings of both original images, read instruction by instruction:
server 0x428940 … 0x428eb1 and client 0x4dfad0 … 0x4dfd7a (SpawnLocation); the call lists of
server 0x4dd2e0 and client 0x5a9f90 (search start); client 0x5be484 … 0x5bf2cc (ruin branch
of the town builder) and the helpers 0x5bf4f0, 0x5a6a70, 0x43abf0, 0x4120b0, 0x4122e0,
0x424b50, 0x40e420, 0x42c5b0, 0x43b610, 0x4db110; client 0x5deb00 … 0x5dec29 (location
ring); client 0x610000 … 0x610370 and server 0x535fc8 … 0x5360da (activation). Constants read
from the Cube.exe image bytes (2.0 at 0x745e10, 65536.0 at 0x6fce7c, 360.0 at 0x6fce6c,
32767.0 at 0x6fd338). Reconstruction sources were not consulted.

## Unresolved

* Not verified at runtime; static reading only.
* The ruin branch of the town builder was read in the client only; a server counterpart of
  the three ruin sites was not located.
* The value of bit 0x40 of obj +0x7e for open-plot ruin creatures is not determined here
  (their appearance is not generated in the builder site; any later finishing pass that
  writes spawn +0x74 was not examined), so whether they get RandomWalk is conditional.
* How WalkPath walks a two-point route whose second point is one block below a standable
  spawn point (the follower and route rules of alpha-materials-wildlife-routes.md §4.6) is
  not re-derived here.
* Path-search §4.6's statement for Companion and WalkPath (an extra round at search start)
  was not re-examined.
