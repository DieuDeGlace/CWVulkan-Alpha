# Alpha terrain-box range, zone traversal and indexed-model fade

Source-author hold of 2026-11-21. FINAL / STABLE / HELD.

## Original evidence

The evidence is the owner's original 32-bit Alpha client, `binary/Cube.exe`,
SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
Addresses below are RVAs relative to its preferred image base of 0x00400000.
The original client instruction listing and the corresponding arithmetic,
zone-lookup and indexed-model routine exports were checked against this image.
This record covers the terrain-box call and the zone-local indexed-model
collection in the main scene routine at 0x0AC260. It describes finite inputs
within the ranges of the original integer conversions and fixed-point products.

## Terrain-box horizontal range

At 0x0AC4E0–0x0AC515 the scene reads the binary32 client value at +0x1D4 and
adds the binary32 constant 22.4 (RVA 0x302A94). Call the rounded sum D.
A resident slot with matching logical coordinate tags reaches the box test at
0x0AC665–0x0AC6D0. Its caller adds 32.0 to D in a second binary32 operation.
That rounded result is the radius. The two corner arguments begin at slot
+0x20 and +0x38. The slot's separate extent and reference fields supply
neither the radius nor the midpoint operands at this call.

The box helper at 0x07F3C0 works in raw signed 64-bit coordinates, with
65,536 raw units per world unit. It adds the two corners component by
component. Each sum is multiplied by 32,768 and divided by 65,536 using
signed integer arithmetic with truncation toward zero (0x02C900).
It subtracts the X and Y midpoints separately from the client reference
components at +0x140 and +0x148. Each difference is squared as a 64-bit
product, then divided by 65,536. The two integer quotients are added.
The vertical component contributes nothing to this range test.

The helper squares the supplied radius in binary32 and multiplies the
rounded square by binary32 65,536. It converts that result to a signed
64-bit integer with truncation toward zero. The box proceeds when the sum
of the two fixed-point contributions is less than or equal to that threshold.
A larger sum rejects immediately. See 0x07F3D3–0x07F536 and conversion
0x28D946. The subsequent six-plane test uses the translated horizontal
corners and untranslated vertical limits, at 0x07F53C–0x07F73E.
The later slot distance key at 0x0AC6E3–0x0AC737 uses the reference at
slot +0x50. It is a separate three-dimensional calculation.

## Indexed-model zone traversal

Bounds come from components zero and one of the position at +0x10 on the
creature referenced by client +0x8006D0, at 0x0B0708–0x0B082E.
For each endpoint the component is read as signed fixed point, divided by
65,536 toward zero, and the low signed 32-bit result is converted to binary32
(0x0120B0 and 0x02F570). The minimum subtracts D. The maximum adds D.
Each rounded result is multiplied by binary32 1/256 (RVA 0x302A2C) and
converted to a signed 32-bit zone coordinate by truncation toward zero.
For example, a scaled minimum strictly between minus one and zero becomes
zero. Both endpoints use the full D. Each endpoint reads the component
again, in minimum-X, maximum-X, minimum-Y, maximum-Y order.

At 0x0B21C7–0x0B2265 and 0x0B329A–0x0B32D4, X runs from its minimum to
its maximum inclusive. For every X, Y restarts at its minimum and runs to
its maximum inclusive. Both increase by one and an inverted range is empty.
There is no endpoint clamp in that computation. The separate lookup at
0x034D10 admits each coordinate only in 0 through 65,535 inclusive. It
looks up the containing 64-by-64 region, then its zone pointer. A missing
region, empty pointer or out-of-range coordinate yields no zone to visit.

For each present zone the scene visits the collection at zone +0x30 first,
then walks the indexed records at zone +0x0C in stored forward order.
The indexed ordinal starts at zero in each zone. It advances for every
record, including rejected indices. See 0x0B2225–0x0B225F,
0x0B2C82–0x0B2D17 and 0x0B3261–0x0B3294.

Client byte +0x8006E6 is initialized to zero at 0x05A000. A nonzero byte
at 0x0B21C7 skips both zone collections for this traversal. Control rejoins
at 0x0B32DA, sets the scalar render parameter to one and reaches the shared
deferred sort at 0x0B332C. The same byte is tested at six other scene sites
(0x0AC312, 0x0ADEB6, 0x0B1527, 0x0B1AF4, 0x0B4E5B and 0x0B93D5).
A search of the original executable code for the full field displacement
found these seven reads and the zero initialization. Its native name, any
indirect writer and the circumstances that might set it nonzero remain open.

## Indexed-model distance fade

At 0x0B06F3–0x0B0766 the far distance is D multiplied by binary32 0.3
(RVA 0x2FCDC0). The near distance is that rounded far distance multiplied
by binary32 0.9 (RVA 0x2FD31C). The model's raised center is used earlier
for admission. The common draw consumes the separate squared reference
distance to the unraised origin, computed with three binary32 components
at 0x0B300E–0x0B3097. Immediate and deferred calls both supply this
quantity to 0x0BE760, at 0x0B3164 and 0x0BA503–0x0BA54A respectively.

At 0x0C0403–0x0C045D, squared distance less than or equal to the binary32
square of the near distance gives a fade value of exactly one. Otherwise
the routine widens the squared distance to binary64, takes its square root
and rounds it to binary32. It subtracts near from this distance and divides
by far minus near, then subtracts that quotient from one. Each subtraction
and the division round to binary32. A rounded distance equal to far gives
zero for a nonzero interval. There is no clamp here. A distance beyond far
can give a negative scalar when the preceding admission has permitted it.

The renderer receives the scalar in the first component of a four-float
parameter. The other three components are zero (0x047FB0). The common draw
then sets its transform and separate color and submits the model, at
0x0C0462–0x0C049A. This establishes the value at the renderer boundary.
Shader output alpha, blending, final pixel coverage, special preliminary
draws and effects belonging to other object families are outside this record.
