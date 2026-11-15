Status: FINAL / STABLE / HELD

# Alpha tick-period rulings: zone-object hazards and the beam pulse

Scope: settles two disagreements between held contracts. It does not edit them.
Images: Cube.exe (client) and Server.exe (server), both with image base 0x400000.
All addresses are RVAs. The server twin of every client site below lies at the
client RVA minus 0xDA240; each twin cited was read separately from Server.exe.
Evidence was read from the original image bytes with dumpbin and a direct read
of the dispatch tables. The Infinite-Cubes reconstruction was not consulted.

Division convention used below: "q(x)" is the signed 32-bit quotient of x by
200, truncated toward zero. In both images it is a reciprocal multiply by
0x51EB851F, an arithmetic shift of the high half by 6 and a sign-bit correction.
That sequence equals truncating division by exactly 200 for every signed 32-bit
input. A shift by 5 would mean 100; no such shift is present at either site.

## 1. Hazard firing gate for zone-object types 6 and 7

### 1.1 Rule

Let Δ be the world update's first stack argument (signed 32-bit milliseconds).
The update copies it to a frame local at entry (client 0x20C549–0x20C54F) and
adds that local to world +0x8000BC early in the routine (client 0x20C678,
server 0x132438). Let C be world +0x8000BC as it stands after that addition.

In the object loop, for an object of type 6 or 7 whose active byte +0x30 is
nonzero after the toggle step, the hazard pass runs in this step exactly when
q(C + Δ) differs from q(C). C + Δ is a wrapping 32-bit sum. C is read fresh from
world +0x8000BC at the gate (client 0x20DADD); Δ is the same local that was
added to the clock.

* Divisor: **200** for both operands.
* Operands: **C** (the clock after this step's advance) and **C + Δ**. It is not
  the pre-advance clock compared with the advanced clock.
* Types 6 and 7 share one code path with no type-dependent difference in the
  gate. Type 8 uses its own gate (timer +0x34 equal to 0) immediately after.

In words: a pass happens in a step when a multiple of 200 lies in the interval
from C (exclusive) to C + Δ (inclusive), in the truncation-toward-zero sense. At
ordinary positive frame times that is about five passes per second, at most one
per step.

### 1.2 Evidence

* Clock advance: client 0x20C678 adds the Δ local to world +0x8000BC; server
  0x132438 likewise. The local is written once in the routine, at 0x20C54F,
  from the first stack argument.
* Gate: client 0x20DADD–0x20DB10, server 0x13389D–0x1338D0. It loads world
  +0x8000BC (the advanced value), forms its wrapping sum with the Δ local, takes
  q of the sum and q of the loaded value, and enters the hazard pass when they
  differ. Types 6 and 7 both branch to the first instruction of this gate
  (client 0x20DAD3–0x20DADB).
* No instruction of the update routine between the advance and the gate writes
  world +0x8000BC or the Δ local. The other direct writers of +0x8000BC in the
  client image (0x18EC1C, 0x1FAED6, 0x1FBE70, 0x1FC193) are in routines starting
  at 0x18EB00, 0x1FAE40, 0x1FBC90 and 0x1FC160, none of which is a direct call
  target between the advance and the gate. Indirect callees were not audited.

### 1.3 Which held statement is right

* `alpha-hazards-blast-ticks.md` §3.3 step 7 ("the truncating quotient of
  (C + Δ) by 200 differs from that of C by 200", with C the advanced clock): **right**.
* `alpha-open-points-1.md` §1.3 ("⌊(T + dt) / 100⌋ ≠ ⌊T / 100⌋", T after the
  advance): the operands and the post-advance clock are **right**; the divisor
  100 is **wrong** (it is 200). Its consequence "about ten hit passes per second"
  is also wrong (about five).
* The attack-routine summary "two clock values divided by 200 differ (world
  +0x8000BC is one operand)": divisor and operand are **right**. The other value
  is that operand plus the frame time Δ, not a second stored clock.

## 2. Beam pulse for actions 28, 94, 95 (and action 104)

### 2.1 Dispatch

The action dispatch (client 0x211E04, server 0x137BC4) subtracts 1 from the
action byte +0x68, bounds it by 0x6D and indexes a byte table (client 0x222CCC,
server 0x148A8C) and a target table (client 0x222C84, server 0x148A44). Read
from image bytes: actions **28, 94 and 95** go to the beam case (client
0x215C68, server 0x13BA28). Action **104** goes to a different case (client
0x212357, server 0x138117), shared with many other actions. No direct branch in
the client image targets the beam case other than through this table. The
branches inside the beam case that test for action 104 are therefore not reached
through the dispatch in either image.

### 2.2 Rule

Let t be the action time +0x6C (signed 32-bit), Δ the frame-time local, and D
the value the duration routine (client 0x3CAA0, server 0x7DB0) returns with
argument −1. Let E be D plus the value of client routine 0x47310 with argument −1 (its
server twin was not identified). After
the per-action mana handling (not restated here), the beam case:

1. ends without a pulse when t + Δ < D;
2. ends without a pulse when t ≥ E;
3. **first pulse**: when t ≤ D and t + Δ > D, a pulse happens. No quotient test
   applies to this pulse. It applies to actions 28, 94 and 95 alike;
4. otherwise (t > D, or t + Δ = D exactly), only actions **95 and 28** can pulse:
   a pulse happens when q(t + Δ − D) differs from q(t − D) and t + Δ > D. For
   action 94 the case ends with no pulse.

So for actions 28 and 95 the pulse rule is: one pulse in the step where t passes
D (t ≤ D < t + Δ), then one pulse in each later step in which t − D passes a
multiple of 200 (200, 400, …). The repeat period is **200 ms** of action time,
phased from D. The wind-up crossing **does** pulse. Action 94 pulses only once,
at the crossing. When t + Δ equals D exactly there is no pulse in that step; the
crossing pulse then comes in the next step with positive Δ.

### 2.3 Evidence

* Common gates: client 0x215CDD–0x215D22 (t + Δ against D, t against E).
* Crossing pulse: client 0x215D28–0x215D56. When t ≤ D and t + Δ > D it branches
  directly to the pulse body at 0x215F5C. When t > D it branches to 0x215ECB.
* Repeat path: client 0x215ECB–0x215F56, server 0x13BC8B–0x13BD16. It ends
  unless the action byte is 0x5F or 0x1C. It then takes q of (t + Δ − D) and of
  (t − D) with the 0x51EB851F / shift 6 sequence, ends when they are equal, ends
  when t + Δ ≤ D, and otherwise enters the pulse body at 0x215F5C.

### 2.4 Which held statement is right

* `alpha-combat-aim-triplet.md` §3.1 ("A first pulse happens when clock ≤ B <
  clock + step"; afterwards only 0x5F and 0x1C repeat on a change of the
  quotient by 200 of (clock + step − B) against (clock − B)): **right**. Its
  statements that action 0x68 shares this case are not supported by the dispatch
  table (2.1).
* `alpha-open-points-1.md` §3.1 ("⌊(t + dt − D)/100⌋ ≠ ⌊(t − D)/100⌋ … the first
  tick comes when t − D passes 100, not 0"): **wrong** in the divisor (200) and
  in the absence of a crossing pulse (the crossing pulses).
* `alpha-open-points-1.md` §3.1 on action 104 ("enters the same body by a
  separate entry (Cube 0x215d3a–0x215d56)"): **wrong**. That range is the common
  crossing test reached by actions 28, 94 and 95. Action 104 is dispatched to
  client 0x212357 / server 0x138117.

## 3. Unresolved

* Whether action 104 reaches any ray or beam pulse through the case at client
  0x212357 / server 0x138117, and on what timing. This was not traced here.
* The durations D for actions 28 and 95 and the value added to form E were not
  re-verified here. The values in the held contracts (0 for 28, a speed-scaled
  1000 for 95) are restated by those contracts only.
* Indirect callees between the clock advance and the hazard gate were not
  audited for writes to world +0x8000BC.
* Behaviour for zero or negative Δ follows the rules above as read but was not
  checked separately.
