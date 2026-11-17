# Alpha blueprint placement and runestone travel

Status **FINAL / STABLE / HELD**

This contract covers client inputs and state for packet 6 kind 7 and the
runestone map interaction. Evidence comes from the original x86 `Cube.exe`
with SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
Addresses are RVAs. Game offsets are relative to the game controller.
Creature-object offsets include its 0x10-byte prefix. Creature-record offsets
start after the prefix. Positions use 65,536 fixed units per block.

## Blueprint placement

Game construction clears the placement-mode byte at game +0x800704. The
client image contains six instances of that exact displacement in its text
section. They resolve to the constructor's zero store at 0x05a09b and five
tests at 0x07ccff, 0x09797d, 0x09b18d, 0x09b5b1 and 0x0b3338. The tests
gate the mouse rotation, R request, aim movement, slot input and in-world
preview paths, respectively. No stock input or menu path enabling the byte
was established. Direct displacement evidence leaves aliased and computed
writes outside that conclusion.

The rotation and selected blueprint are consecutive signed bytes at creature
object +0x18c and +0x18d, or record +0x17c and +0x17d. Creature construction
and the traced creature reset both clear the pair with a word store.
Member-wise record copies transfer both bytes. The creature record codecs
also transfer each as a separate byte under its field mask. These routes can
carry an existing value. A stock placement selection control remains
unverified. (0x03b6cb–0x03b6d5, 0x03c1ba–0x03c276,
0x04633a–0x04633d, 0x046697–0x0466a0, 0x04b22b–0x04b23f,
0x06623a–0x06624e, 0x0cd253–0x0cd27b, 0x0cd847–0x0cd879,
0x0cdeaf–0x0cdedc, 0x0cb5d0, 0x0cbde0, 0x0cbe50.)

The stock game also constructs an item blueprint preview widget at game
+0x800964. Its draw branch reads an item record at widget +0x16c, including
when the recipe selection updates the displayed item there. This widget's
traced item writes address the widget record. The in-world placement preview
reads the creature bytes and tests the placement-mode flag.
(0x0646f2–0x064771, 0x02f20d–0x02f23e, 0x02f95d–0x02f98e,
0x07bbc7–0x07bbf0, 0x0a1db8–0x0a1e05, 0x0b3338–0x0b3364,
0x0b34b0–0x0b34c4.)

The Windows right-button-down message 0x204 dispatches button index 1 to
the game's mouse-down handler. When that event reaches the placement arm
with the mode byte set, it increments the local creature's rotation byte
once, wrapping at eight bits. Both the in-world preview and request
consumer sign-extend the byte as a count of quarter-turns.
(0x0c85c0, 0x0c8518–0x0c8522, 0x2ffde4, 0x07b600,
0x07ccfa–0x07cd14, 0x0b34b6, 0x2112f7–0x21146b.)

In the frame's placement arm, an R edge with the mode byte set appends
kind 7 to the local creature's queue. The record uses the default item and
target. The placement consumer takes the selected blueprint, rotation, aim
and position from the live creature record when it runs.
(0x09797d–0x0979c7, 0x059530, 0x086100, 0x2112f7–0x21146b.)

## Runestone destination selection

An R edge with the quick-item wheel closed and the local creature alive
runs the static interaction check after the creature interaction UI. The
selected static must resolve and its zone X must be nonnegative. Static
kind 45 sets map-open byte +0x8006e4 and teleport-mode byte +0x800de4 to 1.
This arm enqueues zero interaction records. When the frame reaches
0x097333 with the map closed, it clears teleport mode and sets the selected
destination pair to (−1, −1) before handling the R edge.
(0x097333–0x097393, 0x0973e4–0x097437, 0x1a0910.)

The map overlay refreshes a hovered destination pair at game +0x800dd4
and +0x800dd8. It starts each overlay pass at (−1, −1). A separate pair at
+0x800ddc and +0x800de0 holds the selected destination. Both pairs are
signed 32-bit zone coordinates. With the map open, a positive wheel step
divides the binary32 zoom target at +0x1c8 by the binary32 value 0.9 and a
negative step multiplies by it. The target is clamped to [0.01, 10]. The
frame moves effective zoom at +0x1c4 toward the target. The site-marker pass
requires effective zoom strictly greater than 2.0.
(0x05c10c–0x05c11b, 0x0c9613–0x0c96e1, 0x0c856c–0x0c858b,
0x07ef43–0x07efa2, 0x096611–0x09662c, 0x0aba20, 0x0c9787–0x0c97a9.)

The site-marker pass visits a 64-by-64 zone square centered on the player
position with the map pan applied, from 32 below through 31 above the
center on each axis. It visits ascending X with ascending Y inside it.
It looks up the map-world record for each zone at game +0x800d44.
The record must exist, have a nonzero site kind at +0x10 and have bit 0
set in the byte at +0x30. The tile lookup admits zone coordinates 0
through 65,535 on each axis. The projected marker must have positive
depth in the first projection and normalized X and Y both within [−1, 1]
after the second projection. (0x0c96eb–0x0c9848, 0x202440–0x2024b8,
0x0c9856–0x0c9d49, 0x0ca3ef–0x0ca447.)

Site kind 4 can supply a hovered destination while teleport mode is set.
In the overlay's cursor coordinate units the hit area extends from the
projected X minus 100 up to X plus 100, and from Y minus 20 up to Y plus 10.
Each lower edge is inclusive and each upper edge is exclusive. A hit
stores the site's zone pair. If areas overlap, the last hit in the same
pass supplies the pair. The frame adds the “Teleport” hint when the map
and teleport mode are both open and the selected pair is valid and equals
the hovered pair. (0x0ca036–0x0ca1d3, constants at 0x2fd0b4, 0x2fcd74
and 0x345e74, 0x08b62b–0x08b6c6, text at 0x301830.)

## Confirming runestone travel

Windows left-button release 0x202 dispatches button index 0 to the game
mouse-up path. With the map open, a release commits travel when teleport
mode is set, the selected pair differs from (−1, −1) and both selected
components equal the currently stored hover components. Otherwise the
release copies the hovered pair into the selected pair, including the
invalid sentinel when no eligible marker was hovered. Thus on a stable
marker the first release selects and a further release commits. The
confirmation is a coordinate comparison on release with no click timer.
(0x0c85c0, 0x0c8503–0x0c850d, 0x2ffde8, 0x07def9–0x07df54,
0x07dff6–0x07e008, 0x068840, 0x0688a0.)

The successful release acquires the live world's critical section at
world +0x8000c0. It writes the local creature's three signed 64-bit
position components beginning at creature object +0x10. For each horizontal
axis compute 256 times the selected zone coordinate plus 128 in signed
32-bit block arithmetic, sign-extend, and multiply by 65,536. Write zero
for Z. Release the lock, invalidate both destination pairs, then close the
map. The direct move emits zero packet-6 requests and consumes zero random
draws. (0x07df5a–0x07dfed, 0x02c5b0, 0x201cb0, 0x201e90.)

The original kind-3 consumer's static-45 branch requires the local actor
and uses a static link (0x2109e3, 0x1a5f60). The full discovery and
persistence rules for map visibility, later terrain adjustment, remote
acceptance of an online position change and any other source of a kind-3
record targeting a runestone remain outside this contract.
