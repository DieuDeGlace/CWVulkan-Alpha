# Alpha bag and crafting item-list scroll controls

Source-author hold of 2026-11-21. FINAL / STABLE / HELD.

## Original evidence and ownership

The owner's original 32-bit Alpha client, `binary/Cube.exe`, has SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
All addresses are RVAs from preferred image base 0x00400000. Evidence
includes the original client instruction listing, matching executable bytes,
floating-point constants, the widget's virtual table and its original
routine exports. Measurements below use native interface coordinates.

The client constructs both controls at 0x0C1BB0. The bag has constructor
mode 0 and is stored at client +0x800954 (0x065315–0x065346).
Crafting has mode 2 at +0x800958 (0x06448E–0x0644CC). The same two
routines calculate the scroll range and apply the thumb rectangle.
Each operates on the receiving widget's own measured dimensions, child,
active tab and saved row offsets. The two controls can therefore have
different rectangles or row counts for the same number of entries.

The data binding at widget +0x160 points to the collection of tabs.
Bag binding to the local creature's +0x11DC is visible at
0x08CECF–0x08CEEE. Crafting binds to client +0x800ADC at 0x0644C6.
The active tab at widget +0x1B4 initially is zero. The signed 32-bit
row offsets are held per tab in the vector at +0x198. The signed count of
available scroll positions at +0x1A4 initially is one. The native child
at +0x178 supplies its layout object at child +0x40. When provided, the
constructor binds its named `scrollbutton` movement event 12 to 0x0C5BB0
with the owning list widget as receiver (0x0C1EB0–0x0C1F10).

## Range calculation

The range routine at 0x0C6350 returns immediately for a null data binding.
With a binding, it measures the parent width and height. The column count is
the integer part of (width minus 10) divided by (cell width plus 5).
Visible rows are the integer part of (height minus 40) divided by
(cell height plus 5). Subtraction and division are binary32, with conversion
to signed integer toward zero. The cell dimensions at +0x1A8 and +0x1AC
default to 40 each. A supplied cell template at +0x168 supplies its measured
integer width and height during construction (0x0C1F2A–0x0C1F60).

The widget resizes its offset vector to the number of outer tabs, preserves
the retained offsets and fills new offsets with zero (0x0871E0).
An empty outer collection, or an active tab at or above its size, sets
the range to one. A valid active tab containing zero entries instead sets
the range to zero and resets just that tab's offset to zero.

For a populated tab, the native measures the number of 0x11C-byte entries.
Subtract one from the entry count and take the integer quotient by the
number of columns. From that quotient subtract the visible row count, add
two, and take at least one. This gives the number of scroll positions.
For positive grid dimensions, one entry or any populated grid fitting
in the visible rows gives one position. Every further occupied row adds
one position. The active offset is reduced to the last available position
if it exceeds it. This range routine has no lower clamp for an existing
offset. The original division of entry count minus one by columns is
unsigned. A nonempty tab requires usable, nonzero column geometry. The
path also assumes a nonnegative active tab. See 0x0C63D5–0x0C64B5.

## Thumb rectangle

At 0x0C64C0–0x0C660C a rectangle is applied only with a data binding,
a thumb child, a nonzero range and a nonempty offset vector. If any of
these gates fails, this routine issues no child rectangle or visibility
write. A valid empty tab therefore leaves previously assigned geometry
untouched by this call. For an empty outer collection the offset vector
itself supplies the gate. With a nonempty outer collection and the
out-of-range-tab fallback of one, the one-position branch can place
the thumb without indexing the invalid tab.

The track starts at Y 35. Its integer length is parent height minus 70,
rounded toward zero after the binary32 subtraction. The thumb height
is track length divided by the range, using signed integer division
toward zero. Its X is parent width minus 28, in binary32, and its width
is its own currently measured width. The rectangle setter receives X, Y,
width, height and update flag one. It sets the position and then the size
(0x22BB20). The parent and child dimensions come from 0x22F600 and 0x2291D0.

With one position the thumb fills the track, starting at Y 35.
With two or more positions its travel is track length minus thumb height.
Multiply this integer travel by the active row offset, divide by range
minus one with truncation toward zero, then add 35 for its Y.
A last-row offset reaches the full travel. No minimum thumb height is
applied here. The up and down child positions are also established by
parent layout, at X equal to parent width minus 30. The up child starts
at Y 10, the down child at parent height minus 30 (0x0C604D–0x0C60B4).

## Movement and recalculation

The movement dispatcher at 0x252C10 saves the previous pointer position,
updates the current position, and sends event 12 to the eligible target.
For the thumb, 0x0C5BB0 requires a bound list, a positive range and bit
zero set in input state +0xF4. Button-zero press and release set and clear
that bit at 0x2527FE and 0x25294E. The registered callback presumes its
child and tab offset exist. It does not recalculate the range on entry.

The callback subtracts previous pointer Y (+0xE0) from current pointer Y
(+0xD8), adds the child's current Y, then truncates to integer. Both float
operations are binary32. It first limits Y to at least 35, then to at most
35 plus travel. For positive travel, multiply (Y minus 35) by (range
minus one), divide by travel toward zero, and store the resulting active
row offset. With zero travel this division is skipped. In both cases the
offset is then limited to zero through range minus one. The callback
refreshes the selection marker without making a new selection (0x0C6610).
It reads the child's X again and writes that X and the bounded drag Y with
update flag one, leaving its size in place (0x0C5CEC–0x0C5D2B).

The drag Y can lie between the positions produced by the rectangle routine.
For example, with parent height 171 and three scroll positions, the track
has length 101, the thumb height is 33 and travel is 68. Dragging to Y 87
selects offset one and places the thumb at 87. A subsequent rectangle
refresh for that offset places it at 69. There is no row-position snap at
the end of the movement callback.

The up and down callbacks (0x0C60F0 and 0x0C5A00) recalculate the range.
A permitted one-row move changes the offset, refreshes the marker, then
reapplies the rectangle. A blocked move skips those latter actions.
Selection of a different valid hovered tab at 0x0C5A60 adopts that tab,
recalculates its range, refreshes selection and reapplies the rectangle.
The normal list draw invokes the range calculation at 0x0C209D. That call
alone does not reposition the thumb. Widget size/layout dispatch uses
virtual slot +0x28 (0x22C33F and table RVA 0x302BB4) and enters 0x0C5D50.
With a cell template it recalculates the range and applies the thumb
rectangle after arranging the other children at 0x0C60B4.

## Item-result timing and limits

At 0x08F719–0x08F801 the first traversal resolves the result's recipient.
For a resolved local creature and item kind other than 25, it calls the bag
item insertion routine and immediately recalculates the bag range and
rectangle in that order. There is no intervening success-value check.
Other recipients, unresolved recipients and kind 25 skip this bag pair.

In the subsequent result traversal, each resolved local recipient invokes
the crafting rebuild at 0x08F8C6–0x08F8D5 before the following item-kind
test. The rebuild finishes its six category updates in order, zero through
five, via 0x0A19D0, then recalculates the crafting range and rectangle,
once for that rebuild, at 0x0A186A–0x0A1885. Multiple local results can
therefore cause repeated refreshes. The two result paths do not test
panel visibility before these calls. Each helper's own gates still apply.

These are the range, rectangle and input effects at the widget boundary.
The checks assume well-formed tab storage and representable ordinary
interface geometry. Skin-specific initial dimensions, inherited transform
effects on final pixels, rendering of stale geometry, and a complete
catalog of unrelated client events that can request layout remain open.
