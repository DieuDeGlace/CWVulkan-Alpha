Status: FINAL / STABLE / HELD

# Alpha world generation: random numbers, seeds and the noise kernel

Behavioural contract for the randomness used by the Cube World **Alpha** world generator:
the C runtime random-number generator, the points at which the generator re-seeds it, the
per-world seed offsets, and the lattice-noise kernel on which the terrain, climate, colour,
vegetation and cloud formulas are built. Evidence: the Alpha client `Cube.exe` (SHA-256
`84a7a132…30bf`), the Alpha server `Server.exe` (SHA-256 `1ffbcbce…4fb1`) and the runtime
library `msvcr110.dll` shipped with the game. Companion documents (pointers only):
[overview](alpha-worldgen-overview.md), [regions](alpha-worldgen-regions.md),
[terrain](alpha-worldgen-terrain.md), [vegetation](alpha-worldgen-vegetation.md),
[clouds](alpha-worldgen-clouds.md).

Routines are cited by virtual address in `Cube.exe` (image base 0x400000) unless marked
**server** (`Server.exe`, also based at 0x400000; the two address spaces are unrelated).

---

## 0. Conventions

* **f32(x)** is x rounded to IEEE single precision. "Float" arithmetic rounds every
  intermediate to single precision (scalar single SSE operations); "double" means IEEE
  double (scalar double SSE operations). Where a formula mixes the two, the text names the
  point of rounding.
* **trunc(x)** converts to a 32-bit signed integer by truncation toward zero (the SSE
  truncating conversions). An out-of-range or NaN operand yields 0x80000000.
* Integer arithmetic is 32-bit two's complement and wraps, unless stated otherwise.
* **x mod n** with a possibly negative x is the C remainder (its sign follows x).
  **floor_div** divides rounding toward −∞; for example floor_div(−1, 16384) = −1.
* smooth(t) denotes 3t² − 2t³; sat(t) clamps t to [0, 1].
* The noise kernel takes two double arguments and returns a float.

---

## 1. The C runtime generator

### 1.1 Algorithm

Every draw goes through `rand` and `srand` imported from **MSVCR110.DLL** (import slots
0x6fc468 for `rand`, 0x6fc4b4 for `srand`). The shipped `msvcr110.dll` implements the
linear congruential generator:

* state ← state · 214013 + 2531011, modulo 2³²;
* result = (state >> 16) & 0x7FFF, an integer from 0 to 32767;
* `srand(s)` sets the state to s, taken as an unsigned 32-bit value.

The state lives in the runtime's per-thread data block, so each thread has its own
sequence. Every generator routine of §2.3 calls `srand` before its own first draw, and no
routine reachable through direct calls from the zone generator before its `srand` draws
(§2.3). Whether draws made by other systems can interleave with generator draws depends
on which thread runs zone generation (open question 1).

### 1.2 Draw forms

"Draw" always means one `rand` call. The generator uses these forms:

| Written here | Meaning |
|---|---|
| draw % n | one draw, remainder by n (draws are non-negative) |
| draw & 1, draw & 3, … | one draw, its low bits (any signed-remainder correction emitted for these is never taken, since draws are non-negative) |
| u(a) = draw · a / 32767 | one draw scaled; precision is stated at each use |

### 1.3 Test vector

After `srand(123)` the first three draws are 440, 19053 and 23075 (the first state is
123 · 214013 + 2531011 = 28854610, and 28854610 >> 16 = 440). A newly constructed world
object is initialised with seed 123: the world constructor 0x58eb00 pushes 0x7b at 0x58ee17
and calls the initialiser at 0x58ee1d.

---

## 2. Seeds and re-seeding points

### 2.1 The world seed

The world seed S is a 32-bit integer at W+0x800164, where the world object W is located at
G+0x2e4 inside the game object G.

* **Client.** During start-up 0x459c40 stores the result of `timeGetTime` (call at
  0x45a72d) in G+0x800a50 and initialises the world with it (0x45a741). The handler
  0x46f620 stores a seed received from the server in G+0x800a50 and copies the
  accompanying world name to G+0x800a54. The streaming routine 0x46a8a0 compares
  G+0x800a50 with the current world seed and the pending name with the current world name
  (W+0x94); if either differs it later re-initialises the world with the pending seed and
  name (call at 0x46afab). In multiplayer the client therefore generates from the server's
  seed.
* **Server.** The server initialiser **server** 0x4d83a0 performs the same seed storage,
  the same 77 draws into the same offsets in the same order, and the same home-position
  writes as the client initialiser. Its constructor **server** 0x4c8570 calls it with 123
  (call at **server** 0x4c888d); it is also called from **server** 0x549550 (at 0x549a9e)
  and **server** 0x549c50 (at 0x549e67) with a seed obtained elsewhere (open question 2).

### 2.2 World initialisation: the seed offsets (0x5a52e0)

The initialiser takes a seed and a world name. It stores S at W+0x800164, calls `srand(S)`
and then makes exactly **77** draws. Each draw is reduced modulo 100000, which leaves it
unchanged because draws are below 32768, and stored as a 32-bit integer, in this order:

| Order | Count | Offsets in W | Direct readers in `Cube.exe` | Name used in companion specs |
|---|---:|---|---|---|
| 1 | 1 | +0x800188 | 0x5d7a70, 0x5da280, 0x5e4850 (seeds, §2.3) | **A**, the base offset |
| 2 | 4 | +0x800168 … +0x800174 | 0x5f9340 | O168 … O174 |
| 3 | 20 | +0x80018c … +0x8001d8 | 0x5c5e20 | O18c … O1d8 |
| 4 | 2 | +0x8001dc, +0x8001e0 | 0x5effa0 | O1dc, O1e0 |
| 5 | 2 | +0x8001e4, +0x8001e8 | none found | — |
| 6 | 2 | +0x8001ec, +0x8001f0 | none found | — |
| 7 | 2 | +0x8001f4, +0x8001f8 | 0x5a5bc0 | O1f4, O1f8 |
| 8 | 8 | +0x8001fc … +0x800218 | 0x59fc90, 0x5eea60 | O1fc … O218 |
| 9 | 6 | +0x80021c … +0x800230 | 0x5f9620 (only +0x80022c, +0x800230) | O22c, O230 |
| 10 | 6 | +0x800234 … +0x800248 | 0x5fa170 | O234 … O248 |
| 11 | 10 | +0x80024c … +0x800270 | 0x5c7140 | O24c … O270 |
| 12 | 4 | +0x800274 … +0x800280 | 0x5c4520, 0x5f9bc0 | O274 … O280 |
| 13 | 2 | +0x800284, +0x800288 | none found | — |
| 14 | 4 | +0x800178 … +0x800184 | 0x5e4850 (only +0x800178, +0x80017c) | O178, O17c |
| 15 | 2 | +0x80028c, +0x800290 | 0x5a6550 | O28c, O290 |
| 16 | 2 | +0x800294, +0x800298 | 0x5e4850 | O294, O298 |

Group 14 lies lower in memory than groups 2–13 but is drawn after group 13; the draw
order is the table order. Groups without a reader are still drawn and still advance the
sequence. "Direct readers" lists routines whose instructions address the offset through a
register base; the meaning each companion spec gives a group was not re-checked here.

After the draws the initialiser releases and clears the world's two 1024 × 1024 pointer
grids (at W+0xbc and W+0x4000bc), sets the home position W+0x8000f0 = W+0x8000f4 =
8396928.0 (float bits 0x4b002080) and W+0x8000f8 = 0, and stores the world name at W+0x94.
When the byte at W+0xb4 is zero and the name is non-empty, it then opens the world save
`Save/world_<name>.db`, reads the entry keyed `time`, and stores two 32-bit values from it
at W+0x800160 and W+0x80015c (0x5a5624..0x5a57e1).

**Test vector (S = 123):** A = 440; O168 … O174 = 19053, 23075, 13104, 32363;
O18c … O1d8 = 3265, 30749, 32678, 9760, 28064, 20429, 750, 4480, 3863, 30429, 18586, 30758,
31595, 31567, 24630, 16059, 2734, 703, 3966, 12383; O1dc, O1e0 = 18065, 28392;
O1e4, O1e8 = 29902, 26443; O1ec, O1f0 = 16505, 11114; O1f4, O1f8 = 11737, 10421;
O1fc … O218 = 3541, 2251, 22174, 14324, 24238, 18619, 29848, 25829;
O21c … O230 = 25215, 16284, 8757, 22925, 28488, 12097;
O234 … O248 = 3722, 27392, 7902, 16118, 9541, 29057;
O24c … O270 = 22149, 13505, 29411, 27626, 27840, 21480, 6435, 29263, 24936, 1203;
O274 … O280 = 26554, 15326, 23548, 10611; O284, O288 = 27976, 15369;
O178 … O184 = 25656, 17603, 5259, 6888; O28c, O290 = 2281, 21774; O294, O298 = 8658, 3288.

### 2.3 Re-seeding points inside generation

| Routine | Arguments | Seed | When |
|---|---|---|---|
| region cell, 0x5d7a70 | (rx, ry), each 0..1023 | rx + 0x108a + 1024·ry + 3A | when the cell grid entry (W+0x4000bc, index 1024·rx + ry) is still empty |
| region, 0x5da280 | (rx, ry), each 0..1023 | A + 1024·ry + rx | when the region grid entry (W+0xbc, same index form) is empty; after it has requested the 5 × 5 region cells (rx + i, ry + j), i outer and j inner, each from −2 to 2 |
| zone, 0x5e4850 | (zx, zy), each 0..65535 | A + 65536·zy + zx | when the zone does not yet exist; after it has requested the 3 × 3 regions around floor_div(zx, 64), floor_div(zy, 64), first offset outer, each from −1 to 1, and after allocating the zone object |

Arguments outside the stated ranges return without generating. All sums are 32-bit and
wrap. The names rx, ry, zx, zy follow the argument order (first, second); the axis meaning
is defined in the overview spec.

Call structure: the zone generator is called only from the streaming routine 0x46a8a0
(at 0x46acd1); the region routine only from the zone generator (at 0x5e4901); the region
cell routine only from the region routine (at 0x5da33f). `Cube.exe` contains six direct
`srand` calls: 0x5a52e0, 0x5d7a70, 0x5da280, 0x5e4850, 0x4e4bd0 and 0x60c510. Of these,
only 0x5d7a70 and 0x5da280 are reachable through direct calls from the zone generator, and
neither 0x434d10 (the zone-exists test) nor the zone-object constructor 0x6240f0, both
called before the zone's `srand`, reaches a `rand` call. Indirect (virtual or pointer)
calls were not followed.

---

## 3. The noise kernel N(x, y) (0x4c0ef0)

A single routine: cosine-interpolated value noise on the integer lattice. The companion
specs write their formulas in terms of **N(x, y)**. The server copy **server** 0x4d5d30
uses the same constants. Calling convention: cdecl, x then y as doubles on the stack,
float result returned on the x87 stack.

### 3.1 Lattice value

For a 32-bit integer n:

1. n ← n XOR (n << 13), 32-bit;
2. h = ((n·n·60493 + 19990303)·n − 0x2DF722F3) AND 0x7FFFFFFF, with 32-bit wrap
   (60493 = 0xEC4D, 19990303 = 0x131071F; subtracting 0x2DF722F3 equals adding
   0xD208DD0D modulo 2³²);
3. value(n) = 1 − h · 2⁻³⁰ in double (the constant 9.313225746154785e-10 at 0x702b20).
   The value lies in (−1, 1].

### 3.2 Interpolation

1. ix = trunc(x), iy = trunc(y): truncation toward zero, not floor.
2. Corner indices, 32-bit: a = ix + 57·iy, b = (ix + 1) + 57·iy, c = ix + 57·(iy + 1),
   d = (ix + 1) + 57·(iy + 1). The "+ 1" is formed in double and truncated again.
3. fx = (x − ix)·p and fy = (y − iy)·p in double, where p is the double nearest the decimal
   3.1415927 (0x702b48, bits 0x400921FB5A7ED197), not the double nearest π.
4. wx = (1 − cos fx)·0.5 and wy = (1 − cos fy)·0.5, cosine in double through the
   runtime's `cos` (0x68d7a4, a thunk to the MSVCR110 SSE2 precise cosine); the x cosine is
   evaluated before the y cosine.
5. N = [value(a)·(1 − wx) + value(b)·wx]·(1 − wy) + [value(c)·(1 − wx) + value(d)·wx]·wy,
   evaluated in double in that grouping, then rounded once to float.

Consequences: the double result lies in (−1, 1], and after rounding the float result lies
in [−1, 1]; the kernel is not symmetric about 0; at integer arguments N is the corner
value rounded to float. For a negative non-integer argument the fraction is negative,
the cosine is even, and the corners are the same as for the positive mirror, so on each
axis the interval (−1, 0) mirrors (0, 1).

### 3.3 Test vectors

Computed from §3.1–3.2; float results shown widened to double.

| x | y | N(x, y) |
|---|---|---|
| 0 | 0 | −0.28179097175598145 (corner value of 0: h = 0x5208DD0D) |
| 0.5 | 0 | −0.4591194987297058 |
| 0 | 0.5 | −0.43308860063552856 |
| 1.25 | 2.75 | 0.14833128452301025 |
| −0.3 | 0 | −0.3548884093761444 (equal to N(0.3, 0)) |
| 100.5 | 200.25 | 0.39360496401786804 |
| 4198.464 | 3423 | 0.5244963765144348 |

In the last row x equals 0.0005 times the home coordinate 8396928.

---

## 4. Shared notation

Names used by the companion specs:

* **unit(n)** = (n + 1)·0.5 in float, mapping a noise value into [0, 1].
* **|v|**: the generator forms an absolute value by clearing the sign bit of a double.
  When v is a float it is widened first and narrowed afterwards, which is exact, so the
  result equals the float absolute value. Each use states whether v is float or double.
* **bump(f)** = max(1 − f, 0)² for a normalised squared distance f (defined in the
  regions spec).
* **ease(t)** = 1 − (1 − t²)², with variants written out in full at each use.

---

## 5. Observable state

* After world initialisation the seed is at W+0x800164 and the offsets of §2.2 are at the
  listed positions of W (W = G+0x2e4).
* `rand` state is per thread, so the re-seeding sequence of §2.3 can be observed per
  thread at the MSVCR110 `srand` import.

---

## 6. Open questions

1. **Generation thread.** The zone generator is called from 0x46a8a0. A two-instruction
   adapter 0x46dba0 adjusts its object pointer and jumps to 0x46a8a0, and 0x450e70 starts
   a thread through `_beginthread` with entry 0x450e00, which invokes a stored function
   object. That this thread runs 0x46a8a0 was not established; if it does, generation has
   its own `rand` state.
2. **Server seed source.** Where the seed passed by **server** 0x549550 and 0x549c50 comes
   from (configuration, save file or time) was not traced.
3. **Indirect calls.** Whether any routine reached from the zone generator only through an
   indirect call invokes `srand` or `rand` before the zone's own `srand`, or `srand`
   afterwards, was not checked.
4. The meaning of the groups that have no direct reader (5, 6, 13) and of the unread
   entries of groups 9 and 14 is unknown.
5. The use of the last test-vector row by the regions spec is a pointer that was not
   re-checked here.

---

## Re-certification (2026-11-14)

Checked against `Cube.exe` (SHA-256 84a7a132…30bf), `Server.exe` (1ffbcbce…4fb1) and the
shipped `msvcr110.dll`, using the original assembly listing and image bytes.

**Confirmed.** Import slots of `rand`/`srand`; LCG constants and output form (found in
`msvcr110.dll`, state in per-thread data); test draws for seed 123; constructor seed 123
at 0x58ee17; client seed sources (0x45a72d, 0x45a741, 0x46f620, 0x46afab); server
initialiser structure and its three call sites; the 77-draw order, the modulo 100000, all
offsets and counts; all §2.2 test values (recomputed); home-position bits 0x4b002080 and
W+0x8000f8 = 0; the `Save/world_` file prefix; the three re-seeding formulas and their
argument order; the 5 × 5 and 3 × 3 prerequisite loops; the noise kernel's hash constants,
57 stride, truncation, constant p at 0x702b48, cosine blend, double evaluation and final
float rounding; server noise constants; all seven noise test vectors (recomputed).

**Corrected.**
* Float range of N: old "(−1, 1]"; new: the double is in (−1, 1], the float in [−1, 1].
* Table 2.2: old "not used by the routines specified here" for groups 7, 10, 15 and 16;
  new: each has direct readers (0x5a5bc0; 0x5fa170; 0x5a6550; 0x5e4850), listed with every
  group's readers. Group-meaning labels taken from companion specs were replaced by reader
  addresses, since their meaning was not re-checked.
* Re-initialisation condition: old "if it differs" (seed only); new: the seed or the world
  name differs.
* Save read: old "single player, when a save name is set"; new: byte W+0xb4 zero and name
  non-empty, file `Save/world_<name>.db`, key `time`, two values to W+0x800160/W+0x80015c.
* Server call sites: the containing routines of the server calls are named (0x4c8570,
  0x549550, 0x549c50).
* §4 absolute value: old wording tied the sign-bit idiom to float "ridge" sums only; new:
  the idiom is also used on double operands, and each use states its precision.
* Open question 3 (nested re-seeding): old "not checked"; new: the direct call graph from
  the zone generator reaches no `srand` other than 0x5d7a70 and 0x5da280, and nothing called
  before the zone's `srand` draws; only indirect calls remain open. This bears on
  random-draw order but reverses no earlier claim.
* Added range checks (0..1023, 0..65535), the grid-entry "already exists" tests, the loop
  order of the prerequisite calls and the full test vector for every group.

**Removed.** Advice addressed to an implementer: the bit-exactness instruction in §0, the
"private state" permission in §1.1, the "must keep truncation semantics" sentence in §3.2
and the "checking against the running game" procedure (§5, reduced to observable state and
the kernel's calling convention). The unsupported phrase "the world only uses positive
block coordinates" was dropped. The trailing see-also note pointing to the caves spec was
replaced by the direct call-graph result above.

**Unresolved.** Open questions 1–5 of §6.
