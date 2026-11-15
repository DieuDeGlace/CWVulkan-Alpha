Status: FINAL / STABLE / HELD

# Alpha path search: the clear-route test, the grid search and path following

Behavioural specification of how Cube World **Alpha** creatures find their way: the
**clear-route test** that decides whether a creature may simply walk at its target, the
**line-of-sight** test the behaviours use, the block-grid **path search** that runs when the
straight route is blocked, and the **path follower** that turns the found path into intent
and velocity every step.

Written from the original Alpha images (`Server.exe`, `Cube.exe`; both have image base
0x400000), their decompile and instruction listings, and the constants read from the
images. All routine addresses below are **RVAs** with the image named ("Server RVA 0x…",
"Cube RVA 0x…"). Field offsets are creature-object or static-record offsets. Readings
marked *as read* look unintended but are what the original does. Items that could not be
confirmed are collected in **Unresolved** at the end and are not used as facts in the body.

This document adds to, and does not repeat, these companion specifications (pointers only):
`alpha-creatures-behaviour.md` (behaviour layer and Combat), `alpha-pets-bombs.md` §10.2
(Companion for pets), `alpha-materials-wildlife-routes.md` §4.6–§4.7 (WalkPath and
wandering groups), `alpha-physics-camera.md` (box cells, contact flags, the path-timer
increment, climbing and step-up), `alpha-interaction.md` §4 (static objects).

---

## 0. Conventions

* Positions are 3 × signed 64-bit fixed point, 65536 units per block. A creature's position
  is the **centre** of its collision box (w, d, h) at obj +0x80/+0x84/+0x88, so its feet are
  at z − h/2. The cell of a position is its floor division by 65536 per axis.
* **Solid** = block type (low 5 bits of the block's type byte) other than 0 and 2;
  **water** = type 2.
* **Default world cells** (Server initialiser routines at RVAs 0x157110–0x157560 fill the
  default cells each routine uses): a cell outside the world range, in a zone that is not
  loaded, or below the bottom of its stored column counts as **solid** (type 1); a cell above
  the stored column counts as **water** (type 2 with flag 0x80) at z ≤ 0 and as air above;
  an empty stored cell at z ≤ 0 without its 0x40 flag counts as water.
* **Static records** (`alpha-interaction.md` §4): kind at +0x00, position at +0x08
  (3 × i64), rotation index at +0x20, scale at +0x24/+0x28/+0x2c (floats), state byte at
  +0x30, state timer at +0x34. A static's box is x, y centred on its position and z from its
  position upward by scale z; its horizontal sizes are scale x and scale y, **swapped when the
  rotation index is odd**. Every static test in this document applies that swap.
* **Static lookup.** Statics are reached through each zone's index of 8 × 8-block cells. The
  routines differ in which index cells they visit: the clear-route test and the search start
  visit the 3 × 3 index cells around the point tested; the search round visits the index
  cells that cover the node's grown cell range (§4.3); the door opener visits the index cells
  that cover the creature's box grown by one block horizontally (§6.5). The line-of-sight
  object test instead walks the static lists of every 256 × 256-block zone between the ray's
  start and end cells (§1).
* "**Passable static**" (used by §2 and §4.1): kinds 6, 7 and 9 never count; kinds 1, 2, 3,
  5 and 8 count only while their state byte is non-zero; every other kind counts. (The search
  round uses different static rules, §4.3–§4.4.)
* dt = step length in ms. **Relax(x → t, r)** = repeat dt times: x ← x + (t − x)·r.
* Routines (Server RVA; Cube RVA of the counterpart of the same body size): clear-route
  0x12ef00 / 0x209480; box test 0xd4f90 / 0x19f0a0; line of sight 0xd4d80 / 0x19ee90; ray
  cast 0xd6730 / 0x1a35d0; search start 0xdd2e0 / 0x1a9f90; search round 0xdde90 /
  0x1aaab0. The path rebuild (Server 0xdafe0, 454 bytes) and the follower (Server 0xdb200,
  7644 bytes) have Cube counterparts of different size (Cube 0x1a7c90, 445 bytes; Cube
  0x1a7eb0, 7409 bytes). Other Server routines: drop search state 0x5330; ground probe
  0xd5740; node lookup 0xdd0f0; node write 0xdd1a0; open-set membership 0xdcff0; heuristic
  0xdd090.
* No routine covered here draws a random number.

### 0.1 Fields used here (creature object)

| Offset | Meaning |
|---|---|
| +0x10 | position (centre) |
| +0x34 | velocity; +0x40 intent; +0x4c push vector (3 floats each) |
| +0x5c | contact flags byte: 0x02 in water, 0x04 wall, 0x20 stuck, 0x40 an object blocked; 0x07 any of ground, water, wall (`alpha-physics-camera.md`) |
| +0x7e | appearance flags (16 bit); **0x0100 swimmer** |
| +0x80, +0x84, +0x88 | collision box w, d, h |
| +0x124 | mode flags (16 bit): **0x01 climbing allowed**, 0x04 facing a look vector, 0x40 running, 0x80 see §6.1 |
| +0x12c | hit-stun (follower skipped while > 0) |
| +0x160 | look vector (3 floats, blocks; §6.1) |
| +0x1408 | **best node** (reference into the node map; equals the map's end marker when there is none) |
| +0x140c, +0x1410 | **node map** (ordered map cell → record) and its size |
| +0x1414, +0x1418 | **open set** (ordered set of cells) and its size |
| +0x141c | start cell (3 × i32) |
| +0x1428 | start position; +0x1440 goal position |
| +0x1458 | **arrival radius** r (float, blocks) |
| +0x145c | **path timer** (ms; its increment is described in `alpha-physics-camera.md`) |
| +0x1460, +0x1464 | **path list** (cells, walking order) and its length |
| +0x1468 | **ignored statics**: set of static records touching the start or goal box (§4.1) |
| +0x1478, +0x147c, +0x1480 | **seat reference**: three words identifying one static (set by RandomInteraction to the seat it heads for, −1 in the first two words when none); compared with the three-word reference held in each static-index entry (§4.4) |

Cells are ordered lexicographically by signed x, then y, then z; both the node map and the
open set iterate in that order. A node **record** holds g (cost so far), h (heuristic), f and
the parent cell, in that order (6 words).

---

## 1. Line of sight (Server RVA 0xd4d80, Cube RVA 0x19ee90)

Arguments: from-position A, to-position B, an objects flag, a range L. Every behaviour call
found passes L = 200 and objects flag 1: Combat (two calls, Server RVA 0x2f40, with the two
creature centres) and RandomInteraction (Server RVA 0x1bc20, from the creature to a seat
static's position).

1. d = |B − A| in blocks. If d² < 0.0001: **visible**. If d² > L²: **not visible**.
2. Otherwise cast a ray (Server RVA 0xd6730) from A along (B − A)/d with maximum length d.
   It starts in A's cell and visits cells one by one, each time advancing to the nearest
   cell face along the ray (axes whose direction component squared is below 0.000001 are
   ignored). Before each advance it classifies the current cell and stops when the cell is
   **solid** (water and air pass) or when more than 500 advances have been made; it also stops
   when the travelled length reaches d. The length t is capped at d.
3. With the objects flag, the statics of kinds **3, 4, 5 and 8 whose state byte is non-zero**
   also shorten the ray: for each such static, the ray's crossings of the static box's two
   x-faces and two y-faces are computed; a crossing at a length between 0 and the current t
   whose point lies inside the box's other horizontal extent and its height range becomes the
   new t. Z-faces are not tested. Doors (kinds 1, 2) and all other kinds do not shorten it.
4. **Visible** iff t ≥ d − 0.01.

So a target whose line passes through any solid cell is out of sight, and a creature
standing inside a solid cell sees nothing.

Example: A = (0.5, 0.5, 10.5), B = (10.5, 0.5, 10.5), a solid block at cell x = 5. The ray
enters cell 5 at t = 4.5 and stops: 4.5 < 9.99, not visible. Without the block t = 10,
visible.

---

## 2. Clear-route test (Server RVA 0x12ef00, Cube RVA 0x209480)

Arguments: the creature C (its box and its ignored-statics set), start position P, target
position T, radius R. Result: **clear** or **not clear**. A copy Q of the position is walked
toward T in whole-block steps.

**Reached(Q)**: horizontal distance from Q to T ≤ R (strictly < R for the first test, before
any step), and |Q.z − T.z| < h + R (h = C's own box height).

1. If Reached(P): **clear**.
2. Horizontal offset D = (T − P)ₓᵧ; if |D|² > 2500 it is rescaled to length **50**. Number of
   steps n = truncation of (√(|D|² + 0.5625) + 1).
3. Q := P. For k = 0 … n − 1:
   1. If Reached(Q): **clear**.
   2. u = unit horizontal vector from Q toward T (recomputed every step; left zero when Q is
      straight above or below T). The step is (uₓ, u_y, **−0.71**) blocks.
   3. For each axis in the order **x, y, z**: move Q along that axis by the step's component,
      then test:
      * **Blocks** (box test, Server RVA 0xd4f90, without statics): any solid cell among the
        cells on the surface of C's box cell range at Q (for boxes up to two cells wide in a
        dimension that is every cell). The range is the integer division of the box's minimum
        and maximum corners by 65536.
      * **Statics**, tested only when the blocks were free or the axis is z: a static in the
        3 × 3 index cells around Q blocks when it is passable-filtered in (§0), is not in C's
        ignored set, the signed offset of Q from the static's position along the current axis
        multiplied by the step's component on that axis is negative (Q is moving toward the
        static's position on that axis), and its box overlaps C's box at Q (on each
        horizontal axis: static minimum ≤ box maximum and box minimum < static maximum; on z:
        static base ≤ box top and box bottom < static top).
      * If the **blocks** blocked on **x or y** and C is not a swimmer (appearance 0x0100),
        try a **step-up**: C's box raised by 1.01 blocks must contain no solid cell (whole
        cell range, statics not tested). If free, the horizontal move is kept and the box
        bottom is set to floor(bottom) + **2.02** (*as read*: 1.01 is added twice; the
        physics step-up adds it once). Otherwise the move on that axis is undone.
      * Anything else that blocked (a static, the z axis, or a swimmer) undoes the move on
        that axis.
4. After the last step: **clear** iff Q is more than **25 blocks** (3-D) from P, otherwise
   **not clear**.

Consequences: the walker sinks 0.71 per step and lands on the first floor, so it walks down
any drop; it climbs ledges of at most one block; it never climbs higher walls; water does not
stop it and it does not swim up; closed doors next to it block unless they are in its ignored
set, which is filled when a path search starts (§4.1) and emptied whenever the search state
is dropped. For a target more than 50 blocks away the answer is "clear" when the walker gets
more than 25 blocks from P.

### 2.1 Worked examples

Default box (0.96, 0.96, 2.16), on flat ground with floor blocks at z = 9 (bottom 10.00,
centre z = 11.08).

* **Open ground**, T 10 blocks east at the same height, R = 2.88. n = trunc(√100.5625 + 1) =
  11. The z part of every step hits the floor and is undone. Before step k the walker is
  10 − k blocks away; at k = 8 the distance is 2 ≤ 2.88: **clear**.
* **A one-block ledge**: P = (100.5, 100.5, 11.08), T = (106.5, 100.5, 12.08) standing on a
  ledge whose blocks occupy z = 10 for x ≥ 103. n = trunc(√36.5625 + 1) = 7. Steps 0 and 1
  move to x = 101.5 and 102.5. Step 2 moves to x = 103.5: the cell (103, z = 10) is solid;
  the raised box (z cells 11…13) is free, so the bottom becomes 10 + 2.02 = 12.02; the z part
  then lowers it to 11.31, still free. Step 3 moves to x = 104.5; the z part would put the
  bottom at 10.60, inside the ledge, and is undone. Before step 4 the walker is 2.0 blocks
  from T horizontally and its centre is 12.39 against T's 12.08: |Δz| = 0.31 < 2.16 + 2.88:
  **clear**.
* **A wall two blocks high** in the same place: the raised box at step 2 contains the upper
  wall block; every later step pushes into the wall and is undone; after 7 steps the walker
  is 2 blocks from P: **not clear**.

---

## 3. When a search starts and with what goal

A search is started with the search start (§4.1) after the caller has set the start position
(+0x1428) and goal position (+0x1440); the arrival radius r (+0x1458) is set before or right
after the call. Each caller then runs search rounds (§4.6) and the follower (§6) moves the
creature every step.

| Caller (Server RVA) | Starts a search when | Start | Goal | r | Rounds stop when |
|---|---|---|---|---|---|
| **Combat chase** (0x2f40) | the clear-route test is not run or not clear, or the stuck flag 0x20 is set; and the path list is empty; and the target's contact flags & 0x07 ≠ 0. The clear-route test runs only when the line-of-sight result is true, with radius = the attack range *R* of `alpha-creatures-behaviour.md` | own feet + 0.1 | target's feet + 0.1 | own w + target w | path longer than 50 cells, or path's last cell within **own w** of the goal cell (squared cell distance ≤ w²) |
| **Companion** (0x5780) | the path list is empty under the conditions of the companion specs, and the goal's zone is loaded | own feet + 0.1 | ground point found for the leader | **2** | path longer than 50, or last cell within r of the goal cell (squared < r²) |
| **WalkPath** (0xc5e00) | the path list is empty under the conditions of the route spec, and the goal's zone is loaded | own feet + 0.1 | the route point | read from the behaviour's record (+0x18) | as Companion |
| **SpawnLocation** (0x28940) | as in the town-schedule spec | own feet + 0.5 | the chosen ground position | **4** | as Companion |
| **RandomInteraction** (0x1bc20) | it picks a seat static (kind 16, 18 or 68) in line of sight (range 200, objects flag 1) | own feet + 0.5 | the seat's position | **2** | as Companion |
| **Follower re-plan** (§6.2) | the path timer exceeds 3000 ms | own feet + 0.5 | unchanged | unchanged | — |

When the clear-route test is clear (and, for Combat, the stuck flag is clear), Combat and
Companion **drop** the search state (Server RVA 0x5330: node map, open set and path list
emptied, best node reset, ignored set emptied).

---

## 4. The search

A weighted best-first search over **block cells**. A node is the cell that holds the
creature's **feet**: the creature "standing in" cell (x, y, z) has its centre at
(x + 0.5, y + 0.5, z + h/2). The search state lives in the creature (§0.1) and persists
between updates.

### 4.1 Start (Server RVA 0xdd2e0, Cube RVA 0x1a9f90)

1. Drop the whole search state (as Server RVA 0x5330). Start cell S = cell of the start
   position (stored at +0x141c); path timer := 0.
2. Insert S with g = **1** (*as read*; every later g is offset by one), h = H(S), f = h, and
   put it in the open set. It becomes the best node.
3. **Ignored statics**: C's own box (w, d, h) is placed with its centre at the start cell's
   standing centre (x + 0.5, y + 0.5, z + h/2), and again at the goal cell's standing centre.
   Every passable-filtered-in static (§0) in the 3 × 3 index cells around each placement whose
   box overlaps it (horizontal: static minimum ≤ box maximum, box minimum < static maximum; z:
   static base ≤ box top, box bottom < static top) is added to +0x1468. The clear-route test
   (§2) and the search round (§4.3–§4.4) skip these statics.

**Heuristic** H(c) (Server RVA 0xdd090 and inline copies), with a, b = the absolute x and y
cell differences to the goal cell and e = the absolute z difference:

H = 2 · (10 · max(a, b) + 4 · min(a, b) + 10 · e).

It is twice the flat-ground move cost of §4.4, so the search is greedy (weight 2) and does
not guarantee the shortest path.

### 4.2 One round (Server RVA 0xdde90, Cube RVA 0x1aaab0)

1. If the node map is empty: nothing. If the open set has **501 or more** cells or the node
   map **20 001 or more**: **abandon** — drop the whole search state including the path (as
   Server RVA 0x5330) and stop.
2. If the open set is empty: nothing.
3. Take the open cell whose record has the smallest **f**; on equal f the first in cell
   order. Remove it from the open set (closed cells are never reopened).
4. If it is the goal cell: stop (it is not expanded).
5. Otherwise classify its surroundings (§4.3) and expand it (§4.4).

At most one cell is expanded per round.

### 4.3 The cells around a node

The node box is C's box with its centre at (x + 0.5, y + 0.5, z + h/2) raised by 0.01. Its
cell range x₀…x₁, y₀…y₁, z₀…z₁ runs from the cell of its minimum corner to the cell of its
maximum corner; for the default box (0.96, 0.96, 2.16) that is the single column x, y with
z₀ = z, z₁ = z + 2. The solid cells (§0 rule and default cells) of the range grown by one in
every direction set these flags:

| Name | Solid cells |
|---|---|
| **floor** | x₀…x₁ × y₀…y₁ at z₀ − 1 |
| **ceiling** | x₀…x₁ × y₀…y₁ at z₁ + 1 |
| **wall −x / +x** | column x₀ − 1 / x₁ + 1, y₀…y₁, heights z₀…z₁ |
| **wall −y / +y** | row y₀ − 1 / y₁ + 1, x₀…x₁, heights z₀…z₁ |
| **ledge −x / +x / −y / +y** | the same four faces at height z₀ − 1 |
| **head −x / +x / −y** | the faces x₀ − 1, x₁ + 1, y₀ − 1 at height z₁ + 1 |
| **head +y** | *as read*: the face y₁ + 1 at height **z₀ − 1** (the same cells as ledge +y) |
| **post** (4 corners) | the corner columns (x₀ − 1 or x₁ + 1, y₀ − 1 or y₁ + 1) at heights z₀…z₁ |
| **corner floor** (4) | the same corner columns at z₀ − 1 |
| **water** | the 0x02 bit of the block-type byte of the cell holding (x + 0.5, y + 0.5, z + h/2 + 65/65536) is set (true for water, type 2) |

Then every static in the index cells covering the grown range that is **not in the ignored
set** (+0x1468) is examined, with C's box (w, d, h) centred at (x + 0.5, y + 0.5, z + h/2)
(without the 0.01 raise):

* **on static**: the static's box overlaps C's box **lowered by one block** (horizontal:
  static minimum ≤ box maximum, box minimum < static maximum; z: static base ≤ box top − 1,
  box bottom − 1 < static top). This applies to every static kind in any state.
* **static −x / +x / −y / +y**: examined only when the static is at least **3** blocks tall
  (scale z ≥ 3) or one of ceiling, head −x, head +x, head −y or head +y is set; and only for
  kinds other than 1, 2, 6, 7 and 8 (no state-byte test); and only when the static-index
  entry's three-word reference differs from the creature's seat reference (+0x1478). The flag
  is set when the static's box overlaps C's box shifted by one block toward −x, +x, −y or +y
  (same overlap rules as above, z unshifted).

"**Supported**" below means: floor, or on static, or water.

### 4.4 Expansion: neighbours and costs

The neighbours are tried in this order; each is **allowed** only under its condition. For an
allowed neighbour: if it is not in the node map, insert it with g = parent g + cost, h = H,
f = g + h, parent = this node, and add it to the open set; if it is in the map **and still in
the open set** and the new g is smaller, overwrite its record (new g, its old h, new g + old
h, new parent). A move is "**added**" when it inserted or improved a record in this
expansion.

| # | Move | Allowed when | Cost (this node on floor / not) |
|---|---|---|---|
| 1 | x − 1 | no wall −x, no static −x; and floor, ledge −x, wall −y, wall +y, one of the two −x posts, or water | 10 / 20 |
| 2 | x + 1 | no wall +x, no static +x; and supported, ledge +x, wall −y, wall +y, or one of the two +x posts | 10 / 20 |
| 3 | y − 1 | no wall −y, no static −y; and supported, ledge −y, wall −x, wall +x, or one of the two −y posts | 10 / 20 (a **new** cell costs 30 when not on floor, *as read*) |
| 4 | y + 1 | no wall +y, no static +y; and supported, ledge +y, wall −x, wall +x, or one of the two +y posts | 10 / 20 |
| 5 | (x − 1, y − 1) | no post there; moves 1 and 3 both added; supported or that corner floor | 14 / 24 |
| 6 | (x + 1, y − 1) | no post there; moves 2 and 3 both added; supported or that corner floor | 14 / 24 |
| 7 | (x − 1, y + 1) | no post there; moves 1 and 4 both added; supported or that corner floor | 14 / 24 |
| 8 | (x + 1, y + 1) | no post there; moves 2 and 4 both added; supported or that corner floor | 14 / 24 |
| 9 | z − 1 (**drop**) | no floor and not on static | 0 |
| 10 | z + 1 (**climb**) | no ceiling; and any wall (−x, +x, −y, +y) or water | 0 on floor / 40 not |

Move 1's support list does not include "on static" (moves 2–4 do), *as read*.

**Best node** (node write, Server RVA 0xdd1a0): on every insertion or improvement the written
node becomes best when there is no best node or its h is strictly smaller than the best's h
(ties keep the older one).

What this means for movement:

* **Walking**: on flat ground straight steps cost 10 and diagonals 14; a diagonal is tried
  only when both of its straight neighbours were added in the same expansion.
* **Stepping off a ledge**: a sideways move needs support only at the node it starts from, so
  the search walks off edges; from an unsupported node the only moves are the free drop,
  climbing next to a wall, sideways moves along a wall and moves onto ground beside it (ledge
  flags). There is no fall limit.
* **Steps and walls**: next to a wall the search climbs for free from a floor node and at 40
  per cell after that, then steps onto the top (sideways, supported by the ledge flag,
  cost 20). A one-block step therefore costs 0 + 20 = 20. There is no climbing height limit.
* **Water**: inside water every move is supported; climbing costs 40 per cell (0 from a floor
  node); sinking is free.
* **Statics**: any static under the lowered box supports the node (traps, levers and open
  doors included); doors, stomp traps, spike traps and unseen traps never block sideways;
  windows, gates, levers and other kinds block sideways, whatever their state, when tall
  enough or when the head is blocked; ignored statics and the creature's seat are skipped.
* The search never checks the creature's own body at the node itself: a cell reached
  sideways has a free body (the parent's wall flags), a cell reached upward has a free head,
  a cell reached by dropping had no floor.

### 4.5 The path list (Server RVA 0xdafe0)

Run after every round. The path list is emptied and rebuilt: starting from the **best node**
(or, if the best-node reference is the map's end marker, from the goal cell alone), parent
cells are followed until the cell of the start position is reached, a cell's parent is
itself, a cell has no record, or the number of steps exceeds the node-map size. The cells are
stored in walking order: **start cell first, best node last**.

So the creature always has a path toward the most promising node, even while the search is
still running; when the search reaches the goal the path ends at the goal cell.

After an **abandoned** search (§4.2 step 1) the rebuild that follows in the same round finds
no best node and the path becomes the single goal cell.

### 4.6 Rounds per update

Every caller in §3 runs, once per behaviour update, a loop of **at most 10 rounds**, each
followed by a rebuild. Before the loop it requires the node map to be non-empty (tested once;
Combat also skips the loop when the start and goal positions are identical). Each iteration
first stops the loop when the path has more than 50 cells or when the path is non-empty and
its last cell is within the caller's stop distance of the goal (§3 table).

Companion, WalkPath and SpawnLocation, in the update in which they start a search, run one
additional round and rebuild immediately after the start, before this loop, so up to 11
cells are expanded in that update. Combat and RandomInteraction do not.

---

## 5. Worked example: a search on flat ground

Default box, floor everywhere at z = 9, nothing else around. Start cell S = (0, 0, 10), goal
(3, 2, 10).

* Start: g = 1, h = 2·(10·3 + 4·2) = 76, f = 76.
* **Round 1** expands S (floor: costs 10 and 14; no wall and no water, so no climb; floor, so
  no drop):

  | Cell | g | h | f |
  |---|---|---|---|
  | (−1, 0) | 11 | 2·(40 + 8) = 96 | 107 |
  | (1, 0) | 11 | 2·(20 + 8) = 56 | 67 |
  | (0, −1) | 11 | 2·(30 + 12) = 84 | 95 |
  | (0, 1) | 11 | 2·(30 + 4) = 68 | 79 |
  | (−1, −1) | 15 | 2·(40 + 12) = 104 | 119 |
  | (1, −1) | 15 | 2·(30 + 8) = 76 | 91 |
  | (−1, 1) | 15 | 2·(40 + 4) = 88 | 103 |
  | (1, 1) | 15 | 2·(20 + 4) = 48 | 63 |

  ((0, −1) is new but S stands on floor, so it costs 10.) Best node (1, 1); path [S, (1, 1)].
* **Round 2** expands (1, 1) (f 63): (2, 1) gets g 25, h 28; (1, 2) gets g 25, h 40; (2, 2)
  gets g = 29, h = 20, f = 49 and becomes best; path [S, (1, 1), (2, 2)].
* **Round 3** expands (2, 2): (3, 2) gets g = 39, h = 0. Path [S, (1, 1), (2, 2), (3, 2)].
  A Companion caller (r = 2) does not run round 3 in the same update: after round 2 the
  path's last cell (2, 2) is at squared distance 1 < 4 from the goal, which ends its loop.

---

## 6. Following the path (Server RVA 0xdb200; Cube counterpart RVA 0x1a7eb0)

### 6.1 When it runs

In the Server world step (Server RVA 0x137987–0x137bbc), for each creature processed there,
when the world byte +0xb4 is zero: mode flag 0x80 is cleared; if the **path list is empty**
or **hit-stun** (+0x12c) is positive, the follower does not run and nothing else here
happens. Otherwise mode 0x04 is cleared and the follower runs with the step's dt. It returns
a result:

* **true** when it reached the end of steering (§6.4 steps 13 or 15) or stopped at step 12
  (target too close): the path list is then rebuilt once more (§4.5), and modes 0x04 and 0x80
  are **not** set this step;
* **false** on arrival, empty path, re-plan or the partial-path stop: then, if the path is
  non-empty, mode 0x04 is set and the **look vector** (+0x160) := (minimum corner of the
  path's **last** cell + 0.5 on all three axes) − position, in blocks; in either case mode
  0x80 is set.

The follower overwrites the intent the behaviour set this step: intent := 0 first.

### 6.2 Arrival, empty path, re-plan

1. intent := 0.
2. **Arrived**: if the cell of (feet + 0.5) is within r of the goal cell (squared cell
   distance ≤ r²): drop the search state (as Server RVA 0x5330) and stop.
3. Path empty: stop.
4. **Re-plan**: if the path timer > **3000 ms**: start a new search (§4.1) from feet + 0.5
   with the same goal. This empties the path, so the follower then stops at the next check;
   the caller's rounds rebuild the path on later updates.

### 6.3 Where the creature is on the path

5. **Nearest cell**: the path cell with the smallest squared distance to the creature's
   **feet cell** (cell of position − (0, 0, h/2)); on ties the later cell. Call that distance
   q.
6. If q = 0: path timer := 0.
7. **Partial path**: contact flag 0x40 is clear, the path has fewer than **30** cells, and its
   last cell is farther than r from the goal cell (squared distance > r²).
8. **Re-rooting** (only when not partial, q = 0 and the nearest cell N differs from the stored
   start cell):
   * start cell := N; start position := N's minimum corner (N × 65536);
   * the open set is cut down to the **70** cells with the smallest h (repeatedly taking the
     smallest, ties: first in cell order), or all of them if fewer;
   * for every remaining open cell the parent chain is followed: if it reaches N, the open
     cell and every chain cell before N are kept with their records; if it ends elsewhere
     (missing record or a self-parented cell), the open cell is removed from the open set;
     all other records are forgotten;
   * N is kept with its record and itself as parent; the node map is replaced by the kept
     records; the best node is recomputed as the record with the smallest h (ties: first in
     cell order).

### 6.4 Steering

9. **Ground under a cell** (Server RVA 0xd5740): a cell "has ground" when, with C's box
   placed standing in it, some cell of the box's columns at height floor(feet − 0.2) is solid.
10. **Target cell**: g₀ = ground under N.
    * If the path is not partial and N is not the last cell, the target is the **next** cell
      N₁; g₁ = ground under N₁.
    * If, further, (g₀ or g₁), N₁ is at a different height than N, and N₁ is not the last
      cell, the target is the cell after it, N₂; g₂ = ground under N₂.
    * Otherwise (partial path, or N is the last cell) the target is N.
11. **Climb flag**: mode 0x01 := (q < 4, wall contact 0x04, and not g₀); cleared otherwise.
12. **Direction**: **u** = standing centre of the target cell (x + 0.5, y + 0.5, z + h/2) −
    position, in blocks. If |**u**|² ≤ 0.001: stop (intent stays 0). If |**u**|² > 1:
    normalise it. Then: u_z > 0 → u_z := **1**; otherwise u_z < 0 and in water (contact 0x02)
    → u_z := **−2**; then, if contact 0x40 (an object blocked) → u_z := **3** and doors are
    opened (§6.5).
13. **Partial path and not climbing** (partial and mode 0x01 clear): stop (intent 0, push
    vector unchanged). This is checked before step 14, so it applies even when off the path.
14. **Off the path** (q ≥ 4): intent := **40·u**; no velocity change.
15. **On the path** (q < 4). Two conditions:
    * **Lift** = the target centre is more than 1 block **above** the creature's centre
      (u_z before step 12's changes > 1), or contact 0x40, or not g₀, or the target is N₂ and
      N₂ has no ground.
    * **Loose** = the target is N (partial path or N last), or N₁ has no ground and the
      target is not an N₂ with ground.

    | Case | Velocity change |
    |---|---|
    | partial (so climbing, step 13), u_z ≥ 0 | v := **8·u** (all three axes) |
    | otherwise, vertical | if not partial, u_z ≥ 0 and Lift: Relax(v.z → 4·u_z, 0.05) |
    | otherwise, horizontal, not Loose | Relax(v.xy → 6·u.xy, 0.001); 12·u.xy when running (mode 0x40) |
    | otherwise, horizontal, Loose | Relax(v.xy → 8·u.xy, 0.01) |

    (A partial path is always Loose, so a partial climbing path with u_z < 0 takes the last
    row.) Then intent := **20·u**.
16. Push vector := 0 (whenever steps 14 or 15 ran).

### 6.5 Opening doors

When contact flag 0x40 is set, the follower visits the index cells covering the creature's box
grown by one block on each horizontal side. Every **door (kind 1) or big door (kind 2)** there
whose state byte is non-zero and whose box overlaps the creature's box grown by one block on
each horizontal side (z not grown) is **opened**: state byte := 0, state timer (+0x34) := 0.
Windows and gates are not opened.

### 6.6 Worked example of following

The path of §5, [S, (1, 1), (2, 2), (3, 2)], creature standing in S with floor everywhere,
dt = 16, not running:

* q = 0 → timer := 0. Not partial (the last cell is the goal). N = S is the stored start: no
  re-root. N₁ = (1, 1) has ground and the same height → target N₁; g₀ = g₁ = true.
* u = (1.5 − 0.5, 1.5 − 0.5, 11.08 − 11.08) = (1, 1, 0), normalised (0.707, 0.707, 0). Climb
  flag cleared. q < 4; not Lift → no vertical change; not Loose → v.xy relaxes toward
  (4.24, 4.24) at 0.001: from rest, v.xy = 4.24·(1 − 0.999¹⁶) = 0.068 per axis after one
  step. Intent := (14.1, 14.1, 0). The follower returns true, so the path is rebuilt.
* As soon as the feet cell is (1, 1) the timer resets and the target moves to (2, 2).
* If the creature were pushed two cells off the path (q = 4), it would get intent 40·u toward
  the next cell and no velocity change; if it then never stood exactly in a path cell for
  3 s, the follower would re-plan from its feet.

---

## 7. Server and client

* `Cube.exe` contains counterparts of the routines (§0 list).
* In `Cube.exe`, the frame routine at Cube RVA 0x88ee0 contains, at Cube RVA
  0x98078–0x98409, code that calls the rebuild, the search round and the follower for a
  creature referenced from the frame state (the round is called while that creature's path has
  at most 50 cells). It is gated by the byte at Cube
  RVA 0x36b040, which lies in zero-initialised data and which no instruction addresses
  directly except two reads.
* The line of sight is also used by other systems (see the interaction, item and projectile
  specs).

---

## 8. Further consequences

* Closed doors block the clear-route test (§2) unless ignored, so a chase through a closed
  door uses the search; the search passes doors (they never block sideways) and the follower
  opens them on contact.
* When the search abandons itself (501 open cells), the rebuild leaves the single goal cell
  as the path, so a caller that starts searches only on an empty path does not restart at
  once; the follower heads straight for the goal (off-path or Loose steering) and re-plans
  when the path timer exceeds 3000 ms.
* The heuristic counts height at 20 per block while climbing costs 40 per cell (0 for the
  first cell from a floor node) and dropping 0: the search prefers going down and around to
  climbing.
* Swimmers (appearance 0x0100) are treated like walkers by the search; only the clear-route
  step-up distinguishes them.

---

## Unresolved

* **Client counterparts**: only body sizes were compared. The search start, round, clear-route,
  box test, line of sight and ray cast match in size; the rebuild (Cube RVA 0x1a7c90) and the
  follower (Cube RVA 0x1a7eb0) are smaller than the Server bodies and their logic was not
  compared. The Cube caller addresses (Combat, Companion, WalkPath, SpawnLocation,
  RandomInteraction) were not re-checked. Whether the client default cells match was not
  re-checked.
* **Online and offline split**: which images run the behaviour and the follower in single
  player and online, and the meaning of world byte +0xb4, were not re-traced here.
* **World-step selection**: which creatures the Server world step processes (for example only
  living NPCs) was not re-traced.
* **WalkPath radius**: the value stored at the WalkPath record's +0x18 (formerly given as 10).
* **Caller trigger conditions** for Companion, WalkPath and SpawnLocation (distances, ground
  search) belong to their own specs and were not re-traced; Combat's attack range R and any
  chase time limit were not re-traced.
* **Gated path code** in `Cube.exe` (§7): which creature it serves (formerly "the local
  player"), its restart and round-count details, and whether the gate byte is written through
  an indirect address were not checked.
* **Water flag** in §4.3: which block types other than 2 carry the 0x02 type bit.
* **Static kind 4** (counted by the line-of-sight object test): its meaning.
* **Seat reference words** (+0x1478…+0x1480) and the matching three words in each
  static-index entry: their exact meaning (believed to be zone coordinates and an index in the
  zone's static list).
* **Door state propagation**: how an opened door reaches online clients.
* **Path-timer increment**: taken from `alpha-physics-camera.md`, not re-checked here.
* **Negative coordinates**: the box test divides corners with truncation toward zero, other
  routines with floor; behaviour at negative block coordinates was not examined.

---

## Re-certification (2026-11-15)

Checked against the original Server.exe and Cube.exe images (image base 0x400000), their
decompile and instruction listings, and constants read from the image bytes.

**Confirmed**: LOS thresholds (0.0001, range², t ≥ d − 0.01, 500-advance limit, solid rule,
range 200 with objects flag 1 at the Combat and RandomInteraction calls); clear-route reached
test, 50-block cap via 2500, 0.5625 step-count term, −0.71 step, x/y/z axis order, swimmer
bit 0x0100, 1.01 raise and the +2.02 bottom (*as read*), 625 (25²) final test, passable
statics and ignored set; box-test surface cells; default cells; heuristic and its weight 2;
g = 1 start; abandon limits 501 / 20 001; f selection and cell ordering; all ten moves, their
support lists and costs 10/20/30/14/24/0/0/40; the misplaced +y head flag; best-node rule
(strict, now unqualified); rebuild order; Combat/Companion/SpawnLocation/RandomInteraction
radii and start offsets; follower arrival, 3000 ms re-plan, nearest-cell rule, partial
(30 cells), 70-cell re-root, ground probe (0.2), climb flag, constants 1, −2, 3, 0.001, 4,
0.05, 6, 12, 8, 0.01, 20, 40; door opening of kinds 1 and 2; §2, §5 and §6.6 examples.

**Corrected** (old → new):
1. Addresses given as virtual addresses → RVAs with the image named.
2. Client counterparts "same size and logic" → rebuild and follower differ in size; logic not compared.
3. Static lookup "always 3 × 3 index cells" → per-routine (grown range in the round, grown box for doors, zone lists for LOS).
4. LOS objects "all statics, believed" → only kinds 3, 4, 5, 8 with non-zero state, x/y faces only (PS-6 answered).
5. Clear-route statics → added the condition that Q moves toward the static's position on the tested axis.
6. Search-start probe box (1, 1, h) → the creature's own box (w, d, h); the rotation swap applies there too.
7. **Reversal**: "the search itself does not consult the ignored set" → the round skips ignored statics.
8. "On static" from non-passable statics only, believed → any static not ignored, any kind or state (PS-8 answered).
9. Static side-blocking "believed, not decoded" → fully specified: kinds 1, 2, 6, 7, 8 excluded, no state test, ≥ 3 tall or head flags, seat reference excluded, one-block shifted overlap (PS-2 answered).
10. Water flag → the 0x02 type bit of the cell at centre + 65/65536.
11. "At most 10 rounds" → plus one immediate round when Companion, WalkPath or SpawnLocation start a search; map-empty test made once before the loop.
12. WalkPath r = 10 → read from the behaviour record (moved to Unresolved); RandomInteraction start "not traced" → feet + 0.5.
13. **Reversal**: follower result "not set deliberately; 0x80 set in either case" → explicit result; on true the path is rebuilt and modes 0x04/0x80 are not set; empty path or hit-stun leaves 0x80 cleared (PS-5 answered).
14. Steering order: off-path before partial stop → partial-not-climbing stop is checked first.
15. Lift "vertical difference more than 1 block" → target more than 1 block above (signed).
16. Re-root start position → the cell's minimum corner.
17. Door index cells "3 × 3" → cells covering the grown box; overlap no longer "believed".
18. Abandon consequence "path empty, chase restarts at once" → path becomes the goal cell; restart via the 3000 ms re-plan.
19. §5 Companion stop wording → it stops before round 3.
20. +0x1478 "unknown" → seat reference (three words); look vector offset +0x160 added.

Flagged: items 5–9, 11, 13–15 and 18 affect NPC pathing; none affects random-draw order (no
random draws occur in these routines).

**Removed**: the replacement-engine advice after §7; §9 (corrections to other specs, which
were not opened); the game-check procedures of §10 (the open points are answered above or
listed in Unresolved); the 32 ms cadence and "about 300 cells per second" figure (taken from
another spec, not re-checked); the 20 000 ms chase limit; the per-caller trigger details not
re-traced (now in Unresolved); the "health > 0 NPC" and online-client statements of §6.1/§7.

**Unresolved**: see the Unresolved section above.
