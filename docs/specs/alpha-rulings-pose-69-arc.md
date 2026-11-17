Status: FINAL / STABLE / HELD

# Alpha rulings: the pose-69 trail arc

Scope: one conflict between held contracts about what the client pose refresh RVA 0x0128F0
(VA 0x4128F0) builds into the main-hand weapon-trail histories while the pose is 69 (action
104, the shield charge release). The arc block is VA 0x4226F6 … 0x4229EE inside the history
ageing range VA 0x4225FD … 0x422FE5.

Contracts in conflict:

* **Models**: alpha-creatures-models.md §6.5, "Weapon trail points": "a 16-step circle is
  built for pose 69 (0x4225fd..0x422fe5)".
* **Trails**: alpha-trails-poses-schedules.md §1.2 rule 1 and §1.2.1 (and its
  "Re-certification (2026-11-16)" item 21 and conflict note): sixteen points over a half
  circle, angles *i*·π/16 − π/2, spanning 168.75°.

Image: client Cube.exe (SHA-256 84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF,
verified for this ruling), 32-bit, image base 0x400000. Addresses below are VAs. "Image X" is a
constant read from the file bytes at VA X (binary32 unless stated). Static reading only.
Reconstruction sources were not consulted.

## Terms

* **Pose state**: the structure the refresh ages (EDI in the block). Pose id at +0x08, pose
  clock *t* (32-bit integer) at +0x04, creature pointer at +0x00.
* **Main tip history**: 16 points of 3 × f32 at pose state +0x56C … +0x62B (point *i* at
  +0x56C + 12*i*). **Main butt history**: 16 points at +0x62C … +0x6EB (point *i* at +0x62C +
  12*i*). The off-hand histories at +0x6EC and +0x7AC are not touched by the arc.
* Tip / butt identity (verified for this ruling): after the main-hand weapon draw, +0x62C
  receives the weapon matrix applied to the model point with z = 0 (VA 0x421A33 … 0x421AA0)
  and +0x56C the one with z = the model's z extent (VA 0x421AA5 … 0x421B1E). So +0x56C is the
  tip ring, +0x62C the butt ring, as Trails states.
* **P1**, **P2**: the timing helpers VA 0x43CAA0 and VA 0x447310, each called with −1 (the
  same P1 / P2 as Trails §1.2.1).
* **Shift**: the triplet built at VA 0x4225FD … 0x42268A (x, y from the displayed position
  and the pose state's +0x880 / +0x888 values; z = 0), the same shift Trails §1.1 applies to
  point 0. **M**: the matrix built at VA 0x422731 … 0x422815 (identity, then a translation,
  then a rotation fed by the creature's +0x1374 component 2); no scale step is applied.

---

## Ruling. What pose 69 builds

**Winner: Trails §1.2.1.** Models §6.5's "16-step circle" is wrong as a shape description:
the count of 16 is right (16 points, one per loop iteration), but the shape is an open arc of
**168.75°**, not a circle. The range cited by Models is the whole history-ageing range, not
the arc alone. Trails §1.2 rule 1 and §1.2.1 are confirmed as written, including the
precision statements of its re-certification item 21.

### Behaviour

**Gate** (VA 0x4226F6 … 0x42272B): pose id = 69 (0x45), and P1 < *t* (strict), and *t* <
P1 + P2 (strict). Outside the window the arc block is skipped and the next rules of Trails
§1.2 are tried (with pose 69 not in rules 2–4, it falls to collapse).

**Per-call scalars** (single precision, computed once before the loop):

1. *f* = (float(*t* − P1) × 20.0) ÷ float(P1 + P2). The integer difference is converted to
   binary32, multiplied by image 0x6FCD74 = 20.0 (bits 0x41A00000), then divided by the
   binary32 conversion of the integer sum P1 + P2 (VA 0x42281E … 0x422866). Multiply before
   divide; the divisor is P1 + P2, not P2.
2. *h* = *f* × 0.5 (image 0x745DA0, bits 0x3F000000) (VA 0x422879 … 0x422881).

**Loop**: *i* = 0, 1, …, 15 in increasing order (counter compared with 16, VA 0x4229E5;
exactly 16 iterations). For each *i*:

1. Angle *a*ᵢ = (double(*i*) × π × 0.0625) − π/2, evaluated in binary64 with the constants
   image 0x6FCE08 = π (bits 0x400921FB54442D18), image 0x6FCDD8 = 0.0625 (bits
   0x3FB0000000000000) and image 0x6FCDF0 = π/2 (bits 0x3FF921FB54442D18), in that order
   (multiply, multiply, subtract), then rounded to binary32 (VA 0x422887 … 0x4228A8).
2. *c* = cos(*a*ᵢ) and *s* = sin(*a*ᵢ): each helper widens the binary32 angle to binary64,
   calls the runtime's precise SSE2 cosine (helper VA 0x40E420 → MSVCR110
   libm_sse2_cos_precise) or sine (helper VA 0x424B50 → libm_sse2_sin_precise), and rounds
   the result to binary32. Cosine is evaluated first, then sine.
3. Direction *u*ᵢ = (*s*, *c*, 0): x = sine, y = cosine, z = 0.
4. **Butt point *i*** (+0x62C + 12*i*) := M · (*h* · *u*ᵢ) + shift (component-wise binary32
   scale, VA 0x451510; matrix apply VA 0x4248A0; add VA 0x412280; copy VA 0x411E30 into
   ESI + 0xC0).
5. **Tip point *i*** (+0x56C + 12*i*) := M · (*f* · *u*ᵢ) + shift (same helpers, copy into ESI).

Butt is written before tip within each iteration. After the loop the block jumps to the end
of the ageing range (VA 0x4229EE → 0x422FE5), so no other ageing rule runs in that call.
Point 0 is overwritten like the others; the weapon points recorded for point 0 earlier in the
refresh have no effect on the two main-hand histories while the arc is active. Nothing in the
block depends on dt.

### Geometry that results

* Step: π/16 = **11.25°** (the step of a 32-point full circle).
* Start angle (*i* = 0): −90°, direction ≈ (−1, 0, 0), the creature's left.
* *i* = 8: exactly 0° (the binary64 expression is exact), direction (0, 1, 0), straight ahead.
* End angle (*i* = 15): 7π/16 = **+78.75°**, direction ≈ (0.98079, 0.19509, 0), 11.25° short
  of the creature's right.
* Total sweep: 15 intervals × 11.25° = **168.75°**. The arc is open; it does not close and does
  not reach +90°.
* Centre: the origin of M (the creature's displayed position with the base height of Trails
  §1.2.1), in the creature's horizontal plane (z = 0 in M's frame). Radii in blocks: outer
  (tip) ring *f*, inner (butt) ring *f*/2. *f* grows linearly from 0 just after P1 toward
  20·P2/(P1 + P2) at the window's end (20 blocks for action 104, where P1 = 0).

Binary32 values of the angle and its sine/cosine (computed from the stated constants; see
uncertainty 2):

| *i* | *a*ᵢ (bits) | degrees | *s* = x | *c* = y |
|---|---|---|---|---|
| 0 | 0xBFC90FDB | −90.00 | −1 | −4.371139E−08 |
| 1 | 0xBFAFEDDF | −78.75 | −0.980785251 | 0.195090353 |
| 2 | 0xBF96CBE4 | −67.50 | −0.9238795 | 0.382683426 |
| 3 | 0xBF7B53D1 | −56.25 | −0.8314696 | 0.555570245 |
| 4 | 0xBF490FDB | −45.00 | −0.707106769 | 0.707106769 |
| 5 | 0xBF16CBE4 | −33.75 | −0.555570245 | 0.8314696 |
| 6 | 0xBEC90FDB | −22.50 | −0.382683456 | 0.9238795 |
| 7 | 0xBE490FDB | −11.25 | −0.195090324 | 0.980785251 |
| 8 | 0x00000000 | 0 | 0 | 1 |
| 9 | 0x3E490FDB | 11.25 | 0.195090324 | 0.980785251 |
| 10 | 0x3EC90FDB | 22.50 | 0.382683456 | 0.9238795 |
| 11 | 0x3F16CBE4 | 33.75 | 0.555570245 | 0.8314696 |
| 12 | 0x3F490FDB | 45.00 | 0.707106769 | 0.707106769 |
| 13 | 0x3F7B53D1 | 56.25 | 0.8314696 | 0.555570245 |
| 14 | 0x3F96CBE4 | 67.50 | 0.9238795 | 0.382683426 |
| 15 | 0x3FAFEDDF | 78.75 | 0.980785251 | 0.195090353 |

Note for point 0: because the binary32 angle is slightly beyond −π/2, its cosine is a tiny
negative value, not zero; the y of point 0 is not exactly 0 before M is applied.

### Evidence

| VA | Fact |
|---|---|
| 0x4226F6 / 0x4226FA | pose id (+0x08) compared with 0x45; not equal skips to 0x4229F9 |
| 0x422704 … 0x42270C | *t* ≤ P1 skips the arc |
| 0x422716 … 0x42272B | *t* ≥ P1 + P2 skips to 0x4229F3 (then the next rules) |
| 0x422731 … 0x422815 | M built in a local matrix: identity, translation, rotation; no scale |
| 0x42281E … 0x42283B | float(*t* − P1) × image 0x6FCD74 (20.0) |
| 0x422841 … 0x422866 | ÷ float(P1 + P2) → *f* |
| 0x422870 | loop pointer starts at pose state +0x56C (tip point 0) |
| 0x422879 / 0x422881 | *h* = *f* × image 0x745DA0 (0.5) |
| 0x422887 … 0x4228A8 | binary64 angle from the loop counter, constants 0x6FCE08, 0x6FCDD8, 0x6FCDF0; rounded to binary32 |
| 0x4228B7 / 0x4228CB | cosine helper (0x40E420) then sine helper (0x424B50) on the binary32 angle |
| 0x4228ED … 0x422910 | vector (sine, cosine, 0) constructed (x = sine result, y = cosine result, z = 0) |
| 0x422915 … 0x422955 | scaled by *h*, M applied, shift added, stored at pointer + 0xC0 (butt point *i*) |
| 0x42295A … 0x4229D4 | the same with *f*, stored at the pointer (tip point *i*) |
| 0x4229D9 … 0x4229E8 | counter + 1, pointer + 12, loop while counter < 16 |
| 0x4229EE | jump to 0x422FE5 (end of the ageing range) |
| 0x421A33 … 0x421B1E | main-hand weapon draw: +0x62C := matrix · (½x, ½y, 0); +0x56C := matrix · (½x, ½y, z extent) |
| 0x40E420 / 0x424B50 | widen to binary64, call libm_sse2_cos_precise / libm_sse2_sin_precise, round to binary32 (function table names) |

Stack slots in the loop were assigned by counting pending pushes: the cosine result and the
sine result are read back into the y and x components respectively, *h* is read for the first
(butt) store and *f* for the second (tip) store.

### Residual uncertainty

1. Static reading only; not observed at runtime.
2. The sine/cosine table was evaluated with a host double-precision library, not the
   MSVCR110 precise routines. A last-place binary64 difference could change a binary32 result
   only at a rounding boundary; none of the tabulated values is believed to be near one, but
   this is not proved.
3. The internal composition of M (which position, lean and bob terms form its translation, and
   the rotation axis of helper 0x424610) and the content of the shift were not re-derived;
   they are taken from Trails §1.1 and §1.2.1. This ruling confirms only that M has no scale
   step and that the shift is added after M.
4. What P1 and P2 equal for action 104 (Trails: P1 = 0, P2 = 300 / (*S*·*a*)) was not
   re-derived here.
5. The draw-side use of the two rings (strip order, alphas) is outside this ruling; Trails §1.3
   is unchanged by it.

---

## Summary of corrections

| Contract | Statement | Correct reading |
|---|---|---|
| Models §6.5 | "a 16-step circle is built for pose 69 (0x4225fd..0x422fe5)" | 16 points on an open arc from −90° to +78.75° in 11.25° steps (168.75° sweep); built at 0x4226F6 … 0x4229EE inside that range |
| Trails §1.2 rule 1 | window P1 < *t* < P1 + P2, all 16 points of both main-hand histories overwritten | confirmed (off-hand histories untouched; no other rule runs that call) |
| Trails §1.2.1 | *f* = 20(*t* − P1)/(P1 + P2); angles *i*·π·0.0625 − π/2 in binary64 rounded to binary32; *u* = (sin, cos, 0); tip := M·(*f u*) + shift, butt := M·(0.5 *f u*) + shift | confirmed; additionally: multiply before divide, cosine evaluated before sine, butt point written before tip point per *i* |
| Trails re-certification conflict note | half circle spanning 168.75° | confirmed |
