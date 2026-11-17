# Alpha quick-item wheel and Q use

Status **FINAL / STABLE / HELD**

This contract covers the original client quick-item wheel and its two paths
to item use. Evidence comes from the x86 `Cube.exe` with SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
All addresses are RVAs. Game offsets are relative to the game controller.
Creature offsets are relative to the whole creature object. An inventory
stack begins with a signed 32-bit count followed by the item at stack +4.

## Opening and retaining selection

Construction sets the wheel byte at game +0x800a40 and the signed 32-bit
selection index at +0x800a44 to zero. An admitted Windows key-down message
for virtual key 9 (Tab) toggles the wheel byte between zero and one. The
message handler forwards the key value, and the Tab arm itself has zero
previous-key tests. Each admitted repeated key-down message can toggle it.
The window loop dispatches a pending message before its input poll and
gameplay update. The raw index is retained across closing and reopening.
(0x05a4b0–0x05a4cb, 0x0c845b–0x0c84cd, 0x2ffdd0, 0x07e753–0x07e7be,
tables at 0x07e92c and 0x07e95c, 0x0c9057–0x0c90a8.)

Before the keyboard cases, the handler exits when any of the window handles
at game +0x800880, +0x800890, +0x800888, +0x800894 or +0x80088c is visible
or when the GUI's key receiver at game +0x800710 dereferenced then +0xcc
is present. Active chat input at the object referenced by game +0x800a14,
byte +0x180, routes the press to chat ahead of the Tab case.
(0x07e1ea–0x07e2a0, 0x2531e0, 0x07e6fa–0x07e71f.)

Early in the gameplay update the client compares the combined state
of the wheel being open or its usual modal-interface predicate being true
with the preceding sample at 0x36b0c0. A rise requests sound 0x53
(`menu-open2.wav`) and a fall requests 0x54 (`menu-close2.wav`). This
sample is taken before the later wheel-closing checks. A visible interface
can keep the combined state true across a wheel toggle.
(0x08b2cb–0x08b346, 0x085990, 0x0859ca.)

## Candidates and Q before the wheel update

Each candidate scan visits the local creature's inventory at +0x11dc in
ascending bag index and then ascending slot index. Include the slot when
its signed count is nonzero, item type byte is 1, and the signed 16-bit
requirement at item +0x10 (stack +0x14) is at most the signed 32-bit
creature level at +0x190. Store the bag and slot indices. Each eligible
stack contributes one candidate. All type-1 subtypes pass the same filter.
(0x07ae10–0x07afea, 0x0db110, 0x00ef50.)

The early gameplay scan at 0x091f16 takes place after the frame's world
pieces and before the Q use site. An empty scan clears the open byte.
Q and the quick-item HUD selection use this scan only when it contains
candidates and the modal-interface predicate at 0x07f1d0 is false. The
predicate is true for active chat, open map, any of the game flags
+0x8008f0, +0x8008f1 or +0x8008f2, or visibility of a window referenced by
game +0x800910, +0x8008bc, +0x8008c4, +0x8008f8, +0x800900, +0x8008dc,
+0x8008c0, +0x800880, +0x800888, +0x800874, +0x80088c, +0x800894,
+0x800890 or +0x800a00. The wheel-open byte is independent of this
predicate. (0x08cef4–0x08cf49, 0x091f16–0x091f69, 0x07f1d0–0x07f3aa.)

Q uses the selection index retained when that site is reached, wrapped
into the early candidate count. For a negative index take the nonnegative
remainder in that count. The index field itself holds the raw value.
The held Q input is game +0x17, sampled from DirectInput scan 0x10.
The use check requires Q down and the saved byte at 0x36b100 clear.
An eligible pass stores current Q into that saved byte even when it had no
rising edge. The empty-list and modal branches bypass the saved-byte
write. Q can use the retained selection with the wheel closed or open.
A Q use leaves wheel closure to the ordinary close checks.
(0x0c92a6–0x0c92b5, 0x091f58–0x091fc4, 0x0920ae–0x092145,
0x092217–0x0922b9.)

A Q edge resolves the chosen bag and slot against the live inventory and
rechecks the signed requirement. A failure requests sound 0x32
(`human-voice02.wav`). An acceptable level calls the shared use routine at
0x0a2780. The Q count label is read before this use call. The model and item
pointers for the quick-item HUD are refreshed from the same slot after use.
(0x091ff2–0x092036, 0x0920ae–0x0921f7, 0x08528a.)

## Late wheel input and confirmation

The frame reaches the wheel after the left-click drop, R creature talk,
R static interaction, E pickup and T pet-return sites. The static R site
requires the wheel closed at that earlier point. The R talk and E pickup
sites have their own gates and share the same saved R and E bytes as the
later wheel. The frame writes the R, E and T histories at 0x36b138,
0x36b139 and 0x36b13a together at 0x09b593–0x09b5a6.
(0x09662f–0x0966c9, 0x096922–0x0969fa, 0x09735f–0x097511,
0x0975bb–0x097649, 0x0976e7–0x09771e.)

At entry to the late section the client samples the steering-ready byte at
0x36b140 if the wheel is open and clears it if the wheel is closed. Held
W or S clears the open byte next, before the choice between blueprint
placement, the open wheel and ordinary attack input. Placement mode
uses its own R arm. With placement mode clear and the wheel still open,
the wheel arm handles navigation and confirmation. This arm exits past
the ordinary attack branch even when its confirmation closes the wheel.
(0x097952–0x0979d2, 0x097e34–0x097f27, 0x098409.)

The held movement bytes are W +0x0b, S +0x0c, A +0x0d and D +0x0e on the
game controller (DirectInput scans 0x11, 0x1f, 0x1e and 0x20). The open
wheel treats held A or signed horizontal input at game +0x124 below zero
as left, and held D or that input above zero as right. The sign tests
have a zero threshold. On the traced focused window polling route the
client feeds the mouse sample to the UI and camera callback and then zeros
the movement pair beginning at +0x124 before dispatching the gameplay
frame. A stock mouse-only route that supplies a nonzero value to this
wheel test remains unverified. (0x0c91f2–0x0c9237, 0x0c92b8–0x0c934d,
0x0c93e0–0x0c9432, 0x0c8633, 0x07ea00, 0x0979d8–0x097a17.)

The signed 32-bit left and right repeat times are at 0x36b144 and 0x36b148.
Each direction that is inactive clears its timer first. A frame with
neither direction active sets the steering-ready byte. With a direction
active, a step requires that the byte already be set at entry to the late
section and the respective timer be at most zero. Test left before right.
Left decrements the raw index once and right increments it once. Both
can step in one frame. Each step requests sound 0x55 (`menu-select.wav`)
and sets its timer to 200 ms. On a neutral frame or an active frame with
steering ready, subtract the current integer frame milliseconds from
both timers after testing and saturate at zero. An active frame that is
waiting for neutral skips this subtraction. (0x097a17–0x097b0b, 0x085a04.)

After steering, the open-wheel arm rebuilds the candidate list from the
current inventory. In an empty late list it clears the selector's count
text and exits that arm. The earlier empty-list check owns the automatic
close. With candidates available, reduce a nonnegative raw index modulo
the late count. For a negative raw index first take its nonnegative
remainder in the **early** count from 0x091f16 and then reduce that result
modulo the late count. For example, with three early candidates and raw
index −1, Q can consume the final unit of the last candidate's stack.
The late arm then selects index 0 in the two remaining candidates, from
an early remainder of 2. (0x097b0b–0x097b8b, 0x097ea7–0x097f22,
0x09d346–0x09d34d.)

The wheel refreshes the selected stack's labels and tests for an R rising
edge first. It checks E if R has no rising edge. A single frame's
confirmation reaches the selected stack at most once. It resolves that
stack and rechecks the same signed item requirement against live creature
level. A failure requests sound 0x32 and leaves the wheel open. A passing
requirement calls the shared use routine and then closes the wheel, even
if the shared routine declines the use. (0x097db6–0x097e82.)

## Shared use and packet order

The use routine accepts a live type-1 stack when the creature's signed
32-bit hit-stun counter at +0x12c is at most zero. Subtype 7 appends a
kind-1 request with a copy of the item to the local creature's queue at
+0x130c. The stack count is decremented after enqueue. Accepted other
subtypes copy the item into the active consumable slot and set the
timed drink or eat action, then decrement the same stack. An exhausted
stack gets count, type and subtype zeroed. (0x0a2780–0x0a2854, 0x059530,
0x02c5e0, 0x086100.)

The two keyboard use sites append in Q-then-wheel order when both admit
a bomb, with any independently admitted earlier producers between them.
Both sites follow the frame's world pieces. Their appends are first
eligible in a later frame's world piece. Candidate selection, direction
steps, the level gate and the shared use path have zero game random draws.
The navigation and level sounds are immediate client requests at the
listener position at game +0x140, with pitch and volume inputs both 1.
The menu open and close sounds use the same position and inputs. The
traced sound dispatcher selects a fixed filename for each sound ID.
Any later timed action or bomb simulation belongs to the world step.
(0x0920f1–0x092136, 0x097a70–0x097ae5, 0x097e12–0x097e2f,
0x084350, switch data at 0x085e90.)

## Wheel drawing and labels

The render pass scans candidates independently. While the wheel is open
it eases the radius at game +0x800a48 toward 200 and the angle in degrees
at +0x800a4c toward minus 360 times the raw index, divided by the render
candidate count. The angle target uses binary32 arithmetic. Each
interpolation forms the weight from zero, repeatedly adding binary32 0.01
times one minus the current weight in binary64, once per whole elapsed
millisecond. Round the resulting weight to binary32 before combining the
old value and target. The radius is reset to zero in the closed draw
branch and the angle is retained. (0x0bac82–0x0bad2a, 0x0aba20,
0x0ac150–0x0ac246, 0x0bb039.)

For a nonempty render list, visit its candidates in order. Form the
radian angle in binary64 by adding the element's fraction of a full turn
to the interpolated angle converted with π divided by 180, then subtract
π divided by two. Round the angle to binary32. The item center is the
viewport's half-width plus radius times cosine and half-height plus
radius times sine. Each trig helper rounds its result to binary32 before
the radius product. The item model call uses scale 0.09. An empty list
skips the item loop. The angle target arithmetic precedes that count
check. (0x0bae83–0x0bb031, 0x024b50, 0x00e420, 0x0758c0.)

The named selector node takes its visibility from the wheel byte at
0x0927c7, before the late wheel arm. The late wheel arm writes the selected
stack's count as decimal to its `count` child. Its `name` child receives
the item name, the literal space and plus sign, then an integer item-level
display value. From the signed item requirement, subtract one, multiply
by binary32 0.05 and add one. Take one minus the reciprocal of that result,
multiply by 100, add one, then truncate toward zero. These display
calculations use binary32. The compiled label path establishes the text
and sampling order. The original asset supplies the node layout.
(0x0619c2–0x0619f1, 0x0927c7–0x0927d5, 0x097baf–0x097db1,
0x0c76a0–0x0c76dc, 0x03ca60–0x03ca94.)
