# Alpha path search: the clear-route test, the grid search and path following

Behavioural specification of how Cube World **Alpha** creatures find their way: the
**clear-route test** that decides whether a creature may simply walk at its target, the
**line-of-sight** test the behaviours use, the block-grid **path search** that runs when the
straight route is blocked, and the **path follower** that turns the found path into intent
and velocity every step.

Written from the Alpha decompile (`disassembled alpha cube world/decompiled/server` and
`…/client`), the per-instruction listings, and fresh disassembly of `Server.exe` and
`Cube.exe` where constants or return values were not visible in the C output. No code was
copied. Routines are cited as "server 0x…" or "client 0x…"; offsets are given for
cross-checking only. Readings marked *believed* rest on partial evidence; *as read* marks
behaviour that looks unintended but is what the code does.

**Status.** The clear-route test, the line-of-sight test, the search (grid, neighbours,
costs, heuristic, limits, rounds), the path list, the follower (re-planning, re-rooting,
waypoint choice, intent and velocity, door opening) and the callers' parameters are
specified. The search's static-object side blocking is only partly decoded (§4.4). Open
points are in §10.

This document **adds to** and does not repeat:

* `alpha-creatures-behaviour.md` — the behaviour layer (§13.1 cadence, §13.2 Combat; the
  chase of step 10 starts the search described here), the steering summary of §7 (replaced
  by §6 here, see §9);
* `alpha-pets-bombs.md` §10.2 — Companion for pets (uses §2 and §4–§6 here);
* `alpha-materials-wildlife-routes.md` §4.6–§4.7 — WalkPath and Companion for wandering
  groups (route walking stays there; it calls the search described here);
* `alpha-physics-camera.md` — box cells, "solid", water, contact flags (§0, §4, §5), the
  path timer obj +0x145c (§5.1), climbing (§3.5) and step-up (§4.3);
* `alpha-interaction.md` §4 — static objects (record, kinds, state byte, cell index).

---

## 0. Conventions

* As in `alpha-physics-camera.md` §0: positions are 3 × i64 in 1/65536 block, the
  creature position is the **centre** of its collision box **s** = (w, d, h) (obj +0x80),
  so the feet are at z − h/2; block (cell) of a position = floor(position / 65536);
  "solid" = block type (low 5 bits) other than 0 and 2; "water" = type 2. "obj +x" is a
  creature-object offset.
* **Default world cells** used by every test here (server constants set at start-up,
  0x557110…0x557560; the same in the client): a cell in a zone that is not loaded, or
  below the bottom of its stored column, counts as **solid**; an empty cell at height
  z ≤ 0 counts as **water** (sea) unless its 0x40 flag is set; an empty cell above the
  stored column counts as air (as water at z ≤ 0).
* **Static objects** (`alpha-interaction.md` §4.1): box size = scale (+0x24, +0x28,
  +0x2c) with x and y swapped when the rotation index is odd; x, y centred on the
  position, z from the base up. Statics are found through the zone's 8 × 8-block cell
  index; every test here looks at the 3 × 3 index cells around the point tested.
* "**Passable static**" (same rule in §2, §4.1 and the box test): kinds 6 (unseen trap),
  7 (spike trap) and 9 (lever) never block; kinds 1 (door), 2 (big door), 3 (window),
  5 (gate) and 8 (stomp trap) block only while their state byte is non-zero (closed);
  every other kind blocks.
* dt = step length in ms. **Relax(x → t, r)** = repeat dt times x ← x + (t − x)·r
  (`alpha-physics-camera.md` §0).
* Client address of the shared routines (server → client): clear-route 0x52ef00 →
  0x609480; box test 0x4d4f90 → 0x59f0a0; line of sight 0x4d4d80 → 0x59ee90; search start
  0x4dd2e0 → 0x5a9f90; search round 0x4dde90 → 0x5aaab0; path rebuild 0x4dafe0 →
  0x5a7c90; follower 0x4db200 → 0x5a7eb0. The pairs have the same size and logic.

### 0.1 Fields used here (creature object)

| Offset | Meaning |
|---|---|
| +0x10 | position (centre) |
| +0x40 | intent; +0x34 velocity; +0x4c push vector |
| +0x5c | contact flags: 0x01 ground, 0x02 in water, 0x04 wall, 0x20 stuck, 0x40 an object blocked (`alpha-physics-camera.md` §5.1) |
| +0x7e | appearance flags; **0x0100 swimmer** |
| +0x80, +0x84, +0x88 | collision box w, d, h |
| +0x124 | mode flags: **0x01 climbing allowed**, 0x04 facing a look vector, 0x40 running, 0x80 see §6.6 |
| +0x12c | hit-stun |
| +0x1408 | **best node** (the node nearest the goal by heuristic) |
| +0x140c, +0x1410 | **node map** (cell → record) and its size |
| +0x1414, +0x1418 | **open set** (cells) and its size |
| +0x141c | start cell |
| +0x1428 | start position; +0x1440 goal position |
| +0x1458 | **arrival radius** r (blocks) |
| +0x145c | **path timer** (ms; += dt every physics step, `alpha-physics-camera.md` §5.1) |
| +0x1460, +0x1464 | **path list** (cells) and its length |
| +0x1468 | **ignored statics**: the statics touching the start or goal box (§4.1) |
| +0x1478 | *unknown* (three words compared with a static's index reference in §4.4) |

A node **record** holds g (cost so far), h (heuristic), f = g + h and the parent cell.

---

## 1. Line of sight (server 0x4d4d80, client 0x59ee90)

Arguments: from-position A, to-position B, an objects flag, a range L (200 in every
behaviour use; Combat passes the two creature centres and objects flag 1).

1. d = |B − A| in blocks. If d² < 0.0001: **visible**. If d² > L²: **not visible**.
2. Otherwise cast a ray from A along (B − A)/d with maximum length d (ray cast client
   0x5a35d0): it visits cells one by one, each time advancing to the nearest cell face
   along the ray, starting with A's own cell, and stops at the first **solid** cell (water
   and air pass), after more than 500 cells, or when the length reaches d. It returns the
   length t travelled (at most d). With the objects flag the static boxes along the ray
   also stop it *believed* (the static part of 0x5a35d0 was not decoded; the camera's use
   in `alpha-physics-camera.md` §7.3 step 8 calls the same option "objects included").
3. **Visible** iff t ≥ d − 0.01.

So a target whose centre is inside a solid cell, or whose line passes through any solid
cell, is out of sight; a creature standing inside a solid cell sees nothing.

Example: A = (0.5, 0.5, 10.5), B = (10.5, 0.5, 10.5), a solid block at cell x = 5. The ray
enters cell 5 at t = 4.5 and stops: 4.5 < 9.99, not visible. Without the block t = 10,
visible.

---

## 2. Clear-route test (server 0x52ef00, client 0x609480)

Arguments: the creature C (its box and its ignored-statics set +0x1468), start position P
(C's centre), target position T (centre), radius R. Result: **clear** or **not clear**.
It walks a copy of C's box toward T in whole-block steps and asks whether it gets there.

**Reached(Q)**: horizontal distance from Q to T ≤ R (strictly < R for the first test
before any step), and |Q.z − T.z| < h + R (h = C's own box height).

1. If Reached(P): **clear**.
2. Horizontal distance D = |T − P|ₓᵧ, capped at **50**. Number of steps
   n = trunc(√(D² + 0.5625) + 1).
3. Q := P. For k = 0 … n − 1:
   1. If Reached(Q): **clear**.
   2. u = unit horizontal vector from Q toward T (recomputed every step; zero if Q is
      straight above or below T). The step is (uₓ, u_y, **−0.71**) blocks: one block
      sideways and 0.71 block down.
   3. For each axis in the order **x, y, z**: move Q along that axis by the step's
      component, then test the box at Q:
      * **Blocks**: any solid cell of the box (box cells as in `alpha-physics-camera.md`
        §0; this box test, server 0x4d4f90, checks only the cells on the surface of the
        box's cell range, which for boxes up to two cells wide is every cell).
      * **Statics** (only when the blocks were free, or on the z axis): any static whose
        box overlaps Q's box, except passable statics (§0) and those in C's ignored set.
      * If something blocked on **x or y**, it was **blocks** (not a static) and C is not
        a swimmer (appearance 0x0100), try a **step-up**: the box raised by 1.01 blocks
        must contain no solid cell (statics are not tested here). If it is free, keep the
        horizontal move and set the bottom to floor(bottom) + **2.02** (*as read*: the
        routine adds 1.01 twice; the physics step-up adds it once,
        `alpha-physics-camera.md` §4.3). Otherwise undo the move on that axis.
      * Anything else that blocked (z axis, a static, a swimmer): undo the move on that
        axis.
4. After the last step: **clear** iff Q is more than **25 blocks** (3-D) from P,
   otherwise **not clear**.

Consequences: the walker hugs the ground (it sinks 0.71 per step and lands on the first
floor, so it walks down any drop, however deep); it climbs ledges of at most one block (the
step-up test is made one block up); it never climbs higher walls; water does not stop it
and it does not swim up; the ignored set is filled when a path search starts (§4.1) and
emptied whenever the search state is dropped, so it is usually empty when the test runs
and closed doors next to the creature block it. For a target more than 50 blocks
away the answer is "clear" when the walker gets at least 25 blocks along.

### 2.1 Worked examples

Default box (0.96, 0.96, 2.16), on flat ground with floor blocks at z = 9 (bottom 10.00,
centre z = 11.08).

* **Open ground**, T 10 blocks east at the same height, R = 2.88 (melee range,
  `alpha-creatures-behaviour.md` §13.2 step 9). n = trunc(√100.5625 + 1) = 11. The z part
  of every step hits the floor and is undone. Before step k the walker is 10 − k blocks
  away; at k = 8 the distance is 2 ≤ 2.88: **clear**.
* **A one-block ledge**: P = (100.5, 100.5, 11.08), T = (106.5, 100.5, 12.08) standing on
  a ledge whose blocks occupy z = 10 for x ≥ 103. n = trunc(√36.5625 + 1) = 7. Steps 0 and 1
  move to x = 101.5 and 102.5. Step 2 moves to x = 103.5: the cell (103, z = 10) is solid;
  the raised box (z cells 11…13) is free, so the bottom becomes 10 + 2.02 = 12.02; the z
  part then lowers it to 11.31, still free. Step 3 moves to x = 104.5; the z part would put
  the bottom at 10.60, inside the ledge, and is undone. Before step 4 the walker is 2.0
  blocks from T horizontally and its centre is 12.39 against T's 12.08: |Δz| = 0.31 <
  2.16 + 2.88: **clear**.
* **A wall two blocks high** in the same place: the raised box at step 2 contains the
  upper wall block; every later step pushes into the wall and is undone; after 7 steps the
  walker is 2 blocks from P: **not clear**, so Combat starts a path search.

---

## 3. When a search starts and with what goal

A search is started with server 0x4dd2e0 after the caller has set the start position
(+0x1428), the goal position (+0x1440) and the arrival radius r (+0x1458). Each caller
then runs up to **10 search rounds** per behaviour update (§4.6) and the follower (§6)
moves the creature every step.

| Caller | Starts a search when | Start | Goal | r | Rounds stop early when |
|---|---|---|---|---|---|
| **Combat chase** (server 0x402f40, client 0x42cb20; `alpha-creatures-behaviour.md` §13.2 step 10) | chasing, and not (target in sight, route clear by §2 with radius = attack range *R*, and no stuck flag 0x20); the path list is empty; and the target touches ground, water or a wall (its contact flags & 7 ≠ 0) | own feet + 0.1 (position − (0, 0, h/2 − 0.1)) | target's feet + 0.1 | own w + target w | the path has more than 50 cells, or the path's last cell is within **own w** of the goal cell (squared cell distance ≤ w²) |
| **Companion** (server 0x405780, client 0x43ace0; pets §10.2, wandering groups) | owner/leader ≥ 4 blocks away, §2 not clear with radius = own w + leader w, ground cell G found under the leader, the creature more than 4 blocks from G, path list empty, G's zone loaded | own feet + 0.1 | G | **2** | more than 50 cells, or last cell within r of the goal (squared < r²) |
| **WalkPath** (server 0x4c5e00, client 0x58d050; `alpha-materials-wildlife-routes.md` §4.6) | as there | own feet + 0.1 | the fitted route point | **10** | as Companion |
| **SpawnLocation** (server 0x428940, client 0x4dfad0; town schedules) | a schedule entry is due and standable ground is found within 11 blocks up or down of its position | own feet + 0.5 | that ground cell | **4** | as Companion |
| **RandomInteraction** (server 0x41bc20, client 0x4db130) | it picks a seat (stool 16, bench 18, beach towel 68) in sight within 200 blocks *believed* | not traced | the seat | **2** | as Companion |
| **Follower re-plan** (§6.2) | the path timer exceeds 3000 ms | own feet + 0.5 | unchanged | unchanged | — |

When §2 is clear, Combat and Companion **drop** the path and search state (server
0x405330: node map, open set, path list, best node and ignored set are emptied) and steer
straight (`alpha-creatures-behaviour.md` §13.2 step 10; pets §10.2 step 4).

In the chase table row, "attack range" is the *R* of `alpha-creatures-behaviour.md` §13.2
step 9; with no sight the test is skipped and the search starts directly.

---

## 4. The search

A weighted best-first (A*-like) search over **block cells**. A node is the cell that holds
the creature's **feet**: the creature "standing in" cell (x, y, z) has its centre at
(x + 0.5, y + 0.5, z + h/2). The search state lives in the creature (§0.1) and persists
between updates; nothing runs on another thread.

### 4.1 Start (server 0x4dd2e0)

1. Empty the whole search state (as server 0x405330), then: start cell S = cell of the
   start position (stored at +0x141c); path timer +0x145c := 0.
2. Insert S with g = **1** (*as read*; every later g is offset by one), h = H(S), f = h,
   and put it in the open set. It becomes the best node.
3. **Ignored statics**: for the box (1, 1, h) placed with its centre at the start cell
   centre (x + 0.5, y + 0.5, z + h/2), and again at the goal cell, every non-passable
   static (§0) whose box overlaps it is added to +0x1468. Those statics are then ignored by
   the clear-route test (§2). (The search itself does not consult the set.)

**Heuristic** H(c), with a, b = the absolute x and y cell differences to the goal cell and
e = the absolute z difference:

H = 2 · (10 · max(a, b) + 4 · min(a, b) + 10 · e).

It is twice the move cost of §4.3 on flat ground, so the search is **greedy** (weight 2)
and does not guarantee the shortest path.

### 4.2 One round (server 0x4dde90)

1. If the node map is empty: nothing. If the open set has **501 or more** cells or the
   node map **20 001 or more**: **abandon** — empty the whole search state and the path
   (as 0x405330) and stop.
2. If the open set is empty: nothing (the search is exhausted; the path stays as built).
3. Take the open cell with the smallest **f**; on equal f the first in the open set's
   order (cells ordered by x, then y, then z). Remove it from the open set (it is now
   closed; closed cells are never reopened).
4. If it is the goal cell: stop (it is not expanded).
5. Otherwise **classify its surroundings** (§4.3) and **expand** it (§4.4).

Exactly one cell is expanded per round.

### 4.3 The cells around a node

Place the box at the node: centre (x + 0.5, y + 0.5, z + h/2) raised by 0.01. Its cell
range is x₀…x₁, y₀…y₁, z₀…z₁ (`alpha-physics-camera.md` §0 box cells). For the default
box (0.96, 0.96, 2.16) that is the single column x₀ = x₁ = x, y₀ = y₁ = y, z₀ = z,
z₁ = z + 2. Look at the solid cells (§0 rule) of the range grown by one in every
direction:

| Name | Cells (solid) |
|---|---|
| **floor** | under the body: x₀…x₁ × y₀…y₁ at z₀ − 1 |
| **ceiling** | over the body: x₀…x₁ × y₀…y₁ at z₁ + 1 |
| **wall −x / +x** | the column faces x₀ − 1 / x₁ + 1, y₀…y₁, body heights z₀…z₁ |
| **wall −y / +y** | the faces y₀ − 1 / y₁ + 1, x₀…x₁, body heights |
| **ledge −x / +x / −y** | the same faces at the floor height z₀ − 1 (ground beside the floor) |
| **ledge +y** | the face y₁ + 1 at z₀ − 1 (*as read* a second flag that should be the ceiling over +y is also computed at the floor height) |
| **post** (4 corners) | the corner columns (x₀ − 1 or x₁ + 1, y₀ − 1 or y₁ + 1) at body heights |
| **corner floor** (4) | the same corner columns at z₀ − 1 |
| **water** | the cell holding the box centre (x + 0.5, y + 0.5, z + h/2 + 0.001) is water |
| **on static** | a non-passable static overlaps the node's box lowered by one block (something to stand on or in) *believed* |
| **static −x / +x / −y / +y** | a static blocks that horizontal move (§4.4, partly decoded) |

"**Supported**" below means: floor, or on static, or water.

### 4.4 Expansion: neighbours and costs

The neighbours are tried in this order; each is **allowed** only under its condition. For
an allowed neighbour: if it is not in the node map, insert it with g = parent g + cost,
h = H, f = g + h, parent = this node, and open it; if it is in the map **and still open**
and the new g is smaller, overwrite its record (new g, its old h, new f, new parent). A
move "**added**" below means inserted or improved in this expansion.

| # | Move | Allowed when | Cost (on floor / not) |
|---|---|---|---|
| 1 | x − 1 | no wall −x, no static −x; and floor, ledge −x, wall −y, wall +y, the two −x posts, or water | 10 / 20 |
| 2 | x + 1 | no wall +x, no static +x; and supported, ledge +x, wall −y, wall +y, the two +x posts | 10 / 20 |
| 3 | y − 1 | no wall −y, no static −y; and supported, ledge −y, wall −x, wall +x, the two −y posts | 10 / 20 (a **new** cell costs 30 when not on floor, *as read*) |
| 4 | y + 1 | no wall +y, no static +y; and supported, ledge +y, wall −x, wall +x, the two +y posts | 10 / 20 |
| 5 | (x − 1, y − 1) | no post there; moves 1 and 3 both added; supported or that corner floor | 14 / 24 |
| 6 | (x + 1, y − 1) | no post there; moves 2 and 3 both added; supported or that corner floor | 14 / 24 |
| 7 | (x − 1, y + 1) | no post there; moves 1 and 4 both added; supported or that corner floor | 14 / 24 |
| 8 | (x + 1, y + 1) | no post there; moves 2 and 4 both added; supported or that corner floor | 14 / 24 |
| 9 | z − 1 (**drop**) | no floor and not on static | 0 |
| 10 | z + 1 (**climb**) | no ceiling; and any wall (−x, +x, −y, +y) or water | 0 on floor / 40 not |

"On floor" in the cost column is the **floor** flag of the node being expanded. Move 1's
support list does not include "on static" (moves 2–4 do), *as read*.

What this means for movement:

* **Walking**: on flat ground every straight step costs 10 and every diagonal 14; a
  diagonal is tried only when both of its straight neighbours were inserted or improved
  in the same expansion (so a diagonal next to an already-closed cell is skipped).
* **Stepping off a ledge**: a sideways move needs support only at the node it starts
  from, so the search walks off edges; from an unsupported node the only moves are the
  free drop (9), climbing next to a wall (10), sideways moves along a wall and moves onto
  ground beside it (ledge flags). A drop of any height is free: there is no fall limit.
* **Steps and walls**: next to a wall the search may climb for free from a floor node and
  at 40 per cell after that, then step onto the top (sideways, supported by the ledge
  flag, cost 20). A one-block step therefore costs 0 + 20 = 20. There is no height limit
  on climbing; the follower makes the creature climb (§6.4).
* **Water**: inside water every move is supported; climbing up costs 40 per cell (0 from
  a floor node); sinking is free.
* **Statics** *believed*: a static under or in the node supports it; a static next to the
  node blocks the matching sideways move when it is at least **3** blocks tall or when the
  node's head is blocked (one of the ceiling flags), but doors (1, 2), stomp traps (8),
  spike traps (7) and unseen traps (6) never block sideways in the search (closed doors
  are opened by the follower, §6.5). The exact overlap test for this part (server
  0x4de9b0…0x4df77b, using +0x1478 and server 0x4ce350) was not decoded.
* The search never checks the creature's own body at the node itself: a cell reached by
  a sideways move has a free body (the wall flags of the parent), a cell reached upward
  has a free head (no ceiling), a cell reached by dropping had no floor.

The **best node** (+0x1408) is updated on every insertion or improvement: the new node
becomes best when its h is strictly smaller than the best's *believed* (ties keep the
older one).

### 4.5 The path list (server 0x4dafe0)

Called after every round. The path is rebuilt from scratch: starting from the **best
node** (or, if the node map is empty, from the goal cell alone), follow the parent cells
back until the start cell (or a cell whose parent is itself, or more steps than there are
nodes), and store the cells in walking order: **start cell first, best node last**.

So the creature always has a path toward the most promising node, even while the search is
still running or after it failed; when the search reaches the goal the path ends at the
goal cell.

After an **abandoned** search (§4.2 step 1) the next rebuild finds an empty node map and
the path becomes the single goal cell: the follower then heads straight for the goal
(§6.4, off-path case) until the path timer forces a new search after 3000 ms.

### 4.6 Rounds per update

Every caller runs, once per behaviour update (at least 32 ms apart,
`alpha-creatures-behaviour.md` §13.1), **at most 10 rounds**, each followed by a rebuild,
while: the node map is not empty, the path has at most 50 cells, and the path's last cell
is not within the caller's stop distance of the goal (§3 table). Combat also skips the
rounds when the start and goal positions are identical. At most ten cells are expanded per
update, roughly 300 per second.

---

## 5. Worked example: a search on flat ground

Default box, floor everywhere at z = 9, nothing else around. Start cell S = (0, 0, 10),
goal (3, 2, 10).

* Start: g = 1, h = 2·(10·3 + 4·2) = 76, f = 76.
* **Round 1** expands S (floor: costs 10 and 14; no wall, so no climb; floor, so no drop):

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

  ((0, −1) is new but S stands on floor, so it costs 10; the 30 of §4.4 applies only
  without floor.) Best node (1, 1); path [S, (1, 1)].
* **Round 2** expands (1, 1) (f 63): among others (2, 2) gets g = 29, h = 2·10 = 20,
  f = 49, and becomes best; path [S, (1, 1), (2, 2)].
* **Round 3** expands (2, 2): (3, 2) gets g = 39, h = 0. Path
  [S, (1, 1), (2, 2), (3, 2)]. In the same update a Companion caller stops here (last cell
  within 2 of the goal already after round 2: squared distance 1 < 4).

---

## 6. Following the path (server 0x4db200, client 0x5a7eb0)

### 6.1 When it runs

In the world step, for every NPC with health > 0 on the server (offline: in the client),
every step, after the behaviour update (server 0x537994…0x537ab3): mode flag 0x80 is
cleared; if the **path list is empty** or the creature is **hit-stunned** the follower does
not run. Otherwise mode 0x04 is cleared and the follower runs with the step's dt. After it,
if the path is still non-empty: mode 0x04 is set and the **look vector** := centre of the
path's **last** cell (cell + 0.5 on all three axes) − position, in blocks; and in either
case mode 0x80 is set. (If the follower reports success the path is rebuilt once more,
§4.5; its result register is not set deliberately, *as read*.)

The follower overwrites the intent the behaviour set this step: intent := 0 first.

### 6.2 Arrival, empty path, re-plan

1. intent := 0.
2. **Arrived**: if the cell of (feet + 0.5) is within r of the goal cell (squared cell
   distance ≤ r²): drop the path and search state (as 0x405330) and stop.
3. Path empty: stop.
4. **Re-plan**: if the path timer > **3000 ms**: start a new search (§4.1) from feet + 0.5
   with the same goal (this empties the path and resets the timer) and stop; the caller's
   rounds rebuild the path on the next updates.

### 6.3 Where the creature is on the path

5. **Nearest cell**: the path cell with the smallest squared distance to the creature's
   **feet cell** (cell of position − (0, 0, h/2)); on ties the later cell. Call that
   distance q.
6. If q = 0 (standing exactly in a path cell): path timer := 0.
7. **Partial path**: when no object blocked the creature this step (contact 0x40 clear),
   the path has fewer than **30** cells and its last cell is farther than r from the goal,
   the path is "partial".
8. **Re-rooting** (only when not partial, q = 0 and the nearest cell is not the stored
   start cell): the search is re-rooted at the nearest cell N, so it keeps running from
   where the creature now is:
   * start cell and start position := N;
   * the open set is cut down to the **70** cells with the smallest h (ties: first in
     order);
   * every remaining open cell whose parent chain leads back to N is kept together with
     the cells of that chain; open cells whose chain does not reach N are dropped from the
     open set; all other cells are forgotten (and may be searched again);
   * N is kept with itself as parent; the best node is recomputed as the smallest h of the
     kept cells (ties: first in order).

### 6.4 Steering

9. **Ground under a cell** (server 0x4d5740): a cell "has ground" when, with the box
   placed standing in it, some cell of the box's columns one layer below the feet
   (floor(feet − 0.2)) is solid — the physics ground probe
   (`alpha-physics-camera.md` §5.4).
10. **Target cell**: g₀ = ground under the nearest cell N.
    * If the path is not partial and N is not the last cell, the target is the **next**
      cell N₁; g₁ = ground under N₁.
    * If, further, N₁ is at a different height than N, (g₀ or g₁), and N₁ is not the last
      cell, the target is the cell **after** that, N₂ (two ahead), with g₂ = ground under
      it.
    * Otherwise (partial path, or N is the last cell) the target is N.
11. **Climb flag**: mode 0x01 (climbing allowed) := q < 4, wall contact (0x04) and not g₀.
    It is cleared otherwise. (So NPCs climb walls only while following a path and only
    where the path cell has no ground under it; the physics of climbing is
    `alpha-physics-camera.md` §3.5.)
12. **Direction**: **u** = centre of the target cell standing (x + 0.5, y + 0.5, z + h/2)
    − position, in blocks. If |**u**|² ≤ 0.001: stop (intent stays 0). If |**u**| > 1:
    normalise it. Then: u_z > 0 → u_z := **1**; u_z < 0 and in water (contact 0x02) →
    u_z := **−2**; contact 0x40 (an object blocked) → u_z := **3** and open doors (§6.5).
13. **Off the path** (q ≥ 4, at least two cells from the nearest path cell): intent :=
    **40·u**; no velocity change. Done.
14. **Partial path and not climbing** (partial and mode 0x01 clear): stop (intent 0) — the
    creature waits for the search to reach farther.
15. **On the path** (q < 4). Two conditions:
    * **Lift** = the vertical difference to the target centre (step 12, before
      normalising) is more than 1 block, or contact 0x40, or not g₀, or the target is N₂
      and N₂ has no ground.
    * **Loose** = the target is N (partial path or N last), or N₁ has no ground and the
      target is not an N₂ with ground.

    | Case | Velocity change |
    |---|---|
    | partial (so climbing, step 14), u_z ≥ 0 | v := **8·u** (all three axes) |
    | otherwise, vertical | if not partial, u_z ≥ 0 and Lift: Relax(v.z → 4·u_z, 0.05) |
    | otherwise, horizontal, not Loose | Relax(v.xy → 6·u.xy, 0.001); **12·u.xy** when running (mode 0x40) |
    | otherwise, horizontal, Loose | Relax(v.xy → 8·u.xy, 0.01) |

    Then intent := **20·u**.
16. Push vector := 0 (whenever steps 13 or 15 ran).

The intent (20 or 40 times a vector whose horizontal part is at most 1 and whose vertical
part is 1, −2, 3 or the normalised value) is then turned into acceleration by the physics
(`alpha-physics-camera.md` §3.1).

### 6.5 Opening doors

When the creature's contact flags say an object blocked it (0x40), the follower looks at
the statics in the 3 × 3 index cells around it; every **door (kind 1) or big door
(kind 2)** that is closed (state ≠ 0) and whose box overlaps the creature's box grown by
one block on each horizontal side *believed* is **opened**: state := 0, state timer
(+0x34) := 0. Windows and gates are not opened. (How the opened door reaches online
clients is the usual static-state path, `alpha-interaction.md` §6 — not checked here.)

### 6.6 Worked example of following

The path of §5, [S, (1, 1), (2, 2), (3, 2)], creature standing in S with floor everywhere,
dt = 16, not running:

* q = 0 → timer := 0. Not partial (the last cell is the goal). N = S is the stored start:
  no re-root. N₁ = (1, 1) has ground and the same height → target N₁; g₀ = g₁ = true.
* u = (1.5 − 0.5, 1.5 − 0.5, 11.08 − 11.08) = (1, 1, 0), normalised (0.707, 0.707, 0).
  Climb flag cleared. q < 4; Lift false (Δz = 0, ground) → no vertical change; not
  Loose → v.xy relaxes
  toward (4.24, 4.24) at 0.001: from rest, v.xy = 4.24·(1 − 0.999¹⁶) = 0.068 per axis
  after one step. Intent := (14.1, 14.1, 0).
* As soon as the feet cell is (1, 1) the timer resets and the target moves to (2, 2).
* If the creature were pushed two cells off the path (q = 4), it would get intent
  40·u toward the next cell and no velocity relaxation; if it then never stood exactly in
  a path cell for 3 s, the follower would re-plan from its feet.

---

## 7. Server and client

* The server and the client contain the same routines (§0 pairs); offline the client
  runs all of them for every NPC. Online the client runs **no NPC behaviour and no
  follower** (`alpha-creatures-behaviour.md` §13.1: world +0xb4); the server does, and the
  clients only see the resulting creature records.
* The client frame routine (client 0x498078…0x498409) can also run the search and the
  follower for the **local player** (start cell changed → restart; up to 10 rounds per
  frame while the path has at most 50 cells; then the follower). It is gated by a global
  byte (client 0x76b040) that nothing writes and that starts at 0: **dead code** in the
  shipped game.
* The line-of-sight test is also used by the client for interaction, item highlighting and
  projectiles (cited in those specs); that use is local and runs online too.

AlphaVulkan: run §2–§6 on the server (or the host in single player) only; give the
search a per-creature state that survives between updates; keep the 10-round budget, the
501 / 20 001 abandon limits and the 3000 ms re-plan so that NPCs behave (and stall) as in
Alpha.

---

## 8. Further consequences

* Creatures cannot see their way through closed doors in §2 (closed doors block), so a
  chase through a closed door always uses the search; the search walks through the door
  and the follower opens it on contact.
* Because the search abandons itself at 501 open cells and the chase restarts it at once
  (the path is empty), a target that cannot be reached in open terrain makes the chaser
  alternate between short searches and straight walking toward the goal (§4.5), until the
  20 000 ms chase limit of Combat ends it.
* The heuristic counts height at 20 per block while climbing costs 40 and dropping 0: the
  search prefers going down and around to climbing.
* Swimmers (appearance 0x0100) are treated like walkers by the search; the physics keeps
  them in water (`alpha-physics-camera.md` §4.1), so paths that leave the water stall.

---

## 9. Corrections to other specs

* `alpha-creatures-behaviour.md` §7: the steering is the follower of §6 here. "Path still
  long (≥ 4 cells)" is "the creature is at least two cells (squared distance ≥ 4) from the
  nearest path cell"; the "flying/swimming variant" (8·u at 0.01) is the "Loose" case of
  §6.4 step 15 (target cell without ground, or the last cell); "if the creature can fly …
  velocity set to 8·u" is the case "partial path while climbing"; the climb toward 4·u_z
  happens only in the "Lift" case.
* `alpha-creatures-behaviour.md` §13.1: the follower runs only when the creature has a
  path and is not stunned; mode 0x80 is set (and 0x04 with the look vector toward the
  path's last cell) after it runs, not "when the creature has no path".
* `alpha-pets-bombs.md` §10.2 step 5: "up to 10 path steps per tick" means up to 10
  **search rounds** (cell expansions) per behaviour update; the pet moves every step by
  §6. PB-9 is answered by §3–§6 here.
* `alpha-materials-wildlife-routes.md` MW-8: the routines it names are specified here.

---

## 10. Open points

* **PS-1** Step-up height in the clear-route test (§2 step 3, +2.02 *as read*). *Game
  check*: offline, put a level-1 hostile behind a one-block step and a two-block step from
  the TEST character and log whether it chases straight (no path; mod DLL creature log:
  path length obj +0x1464 each frame) or starts a search.
* **PS-2** The static side-blocking of §4.4 (≥ 3 blocks tall, doors never) and the
  +0x1478 comparison. *Game check*: chase the TEST character around a market stand and a
  shelf and log the path cells.
* **PS-3** The 30-cost of a new y − 1 cell without floor and the missing "on static" in
  move 1: visible only in rare climbs; log a path up a wall facing −y versus +y.
* **PS-4** Door opening (§6.5): stand behind a closed door in a town building and let an
  NPC chase you; check the door's state byte on the server and on a second client.
* **PS-5** The follower's result register (§6.1): whether the extra rebuild ever happens
  in practice; harmless either way.
* **PS-6** Line of sight with objects (§1 step 2): whether a closed door or a crate hides
  the TEST character from a monster (acquire at 15 blocks behind a door versus behind an
  open doorway).
* **PS-7** Abandon behaviour (§8): a monster on an island chasing the player across deep
  water — expected to alternate searches and straight walks, then give up after 20 s.
* **PS-8** The "on static" overlap of §4.3 (box lowered by one block): whether a creature
  pathing across tables and beds uses them as floor.
