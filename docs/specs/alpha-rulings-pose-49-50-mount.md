Status: FINAL / STABLE / HELD

# Alpha rulings: pose 49 rest copy, pose 50 body lean, mount argument readers

Scope: three conflicts between held contracts about the client pose refresh RVA 0x0128F0
(VA 0x4128F0) and its per-selector case bodies (RVAs 0x013AFD … 0x01E660). Pose id and
selector are the same number: pose 49 = selector 0x31, pose 50 = selector 0x32, pose 58 =
selector 0x3A, pose 21 = selector 0x15.

Contracts in conflict:

* **Models**: alpha-creatures-models.md, Appendix C (pose cases) and §7.9.5 (feet).
* **Selectors**: alpha-pose-selectors-31-49.md, §6 (`0x31`, `0x32`, `0x3A`) and §12.
* **Poses**: alpha-weapon-attachment-poses.md, §4.

Image: client Cube.exe (SHA-256 84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF,
verified for this ruling), 32-bit, image base 0x400000. Addresses below are RVAs unless written
as VA. "Image X" is a binary32 constant read from the file bytes at RVA X. Server.exe has no
counterpart of this evaluator (Selectors §13); that was not re-checked here.

## Terms

* **D**: the stack depth of the refresh body (after its two register saves). The case bodies
  and the post-evaluator code run at D. "Local +x" is the stack slot at D + x. A stack-relative
  operand inside the original is converted to a local by subtracting 4 for each argument
  push still pending at that instruction.
* **Main rest** = local +0x17C (main-hand rest position triplet); **off rest** = local +0x158
  (off-hand rest position triplet). In the Models notation these are *R* and *L*.
* **Triplet copy** = helper RVA 0x011E30. Verified for this ruling: it copies the three
  32-bit words *from* the triplet whose address is its single stack argument *into* the triplet
  addressed by ECX, and pops its argument. **Element helper** = RVA 0x028970 (returns the
  address of component k of the triplet in ECX, pops its argument).
* T1, T2, R0: the timing helpers RVAs 0x03CAA0, 0x047310, 0x03D1A0 (Selectors §2).

---

## Ruling 1. Pose 49 (selector 0x31), first phase: which rest is copied

**Winner: Selectors §6 `0x31` step 2.** Models Appendix C row 49, phase **W**, "G((2, 0, 0))
with arm lift" is wrong for this phase only.

### Behaviour (pose clock < T1)

After the creature-angle Z, body-angle and main-hand angle writes of that phase, in this order:

1. The **main rest** becomes a copy of the **off rest** (the off rest as it stands at that
   point, i.e. its pre-evaluator value; nothing earlier in this case writes it).
2. Off rest X += 2.0 (image 0x345E10, bits 0x40000000). Off rest Y and Z are unchanged.
3. Auxiliary off-side point (local +0x188) Y += 10 (image 0x345E74) and Z −= 4 (image
   0x345E60). This is the Models "arm lift".
4. Blend rate 0.005 (image 0x2FCD9C); off-hand angle target := main-hand angle target; exit.

So with L0 the off rest on entry: *R* := L0 and *L* := L0 + (2, 0, 0). The Models formula
*L* := *R* + (2, 0, 0) (main rest kept, off rest derived from it) does not occur in this phase.

The other two phases of the case do copy in the Models direction and are confirmed as written
in both contracts:

* **T1 ≤ clock < R0**: off rest := main rest; off rest X −= 2; arm lift (copy at
  0x016F15 … 0x016F24; X at 0x016F3B). Models "G((−2, 0, 0)) with arm lift" is right.
* **clock ≥ R0**: selector := 0; off rest := main rest; off rest X −= 2 and Z −= 1.0 (image
  0x345DC0); arm lift (0x01700F … 0x017096). Models "G((−2, 0, −1)) with arm lift" is right.

### Evidence

| RVA | Fact |
|---|---|
| 0x016D3E / 0x016D43 / 0x016D46 | T1 evaluated; pose clock (C +0x4) ≥ T1 branches to 0x016E5E |
| 0x016DCF | address of local +0x158 (off rest) formed as the copy *source* (pushed at 0x016DDD) |
| 0x016DD6 | ECX = local +0x17C (main rest), the copy *destination*; formed before the push, so no offset correction |
| 0x016DDE | triplet copy call: main rest := off rest |
| 0x016DE3 … 0x016E06 | element 0 of local +0x158 (one push pending at the LEA) += image 0x345E10 = 2.0 |
| 0x016DFD … 0x016E40 | local +0x188 Y += 10, Z −= 4 |
| 0x016E44 … 0x016E59 | rate 0.005, then the tail copy at 0x01E65A (off-hand angle target := main-hand angle target) |

No jump in the listing targets 0x016DCF or 0x016DE3, so the sequence is entered only from the
first-phase path. A scan of the case-body range for adjacent address formations of locals
+0x158 and +0x17C finds this site (0x016DCF … 0x016DDD) as the only one whose destination is the
main rest; the other G-type sites in the range (for example 0x015F29, 0x0169B0, 0x016F15,
0x01700F, 0x017351) all copy main rest into off rest.

### Residual uncertainty

* Static reading only; not observed at runtime. Whether the reversed direction was intended by
  the original authors is irrelevant to parity and not claimed.
* The pre-evaluator values of the two rest positions (and hence the visible offset between
  the hands in this phase) are taken from the held contracts and not re-derived here.
* Only pose 49's G terms were re-read. The other Models rows that use G((2, 0, 0)) (poses 2,
  3, 4, 7, 37, 48) were not individually audited for this ruling, beyond the scan result above.

---

## Ruling 2. Pose 50 (selector 0x32), active phase: does p × 40 reach an output

**Winner: Models.** Appendix C row 50, **A**, "ℓ := 40*q*" is right. Selectors §6 `0x32`
step 3, "p × 40 is formed and stored to the scratch slot but never read", is wrong: the store
is to the body-lean local, not to the scratch slot, and it is read by the foot draws of the same
refresh.

### Behaviour (T1 ≤ pose clock < T1 + T2)

* Local **+0x30** is the body lean ℓ of Models §7.4 step 2 and §7.9.5. Before the evaluator it is
  set to ((s × 0) × 80) ÷ π, which is a signed zero for finite s (store at 0x01339C; image
  0x345D84 = 0, image 0x2FCE50 = 80). It is then set to 10 (image 0x345E74) when riding
  (0x013463 … 0x013487) and for selectors 0x39, 0x34, 0x3A, 0x3B (0x013499 … 0x0134B8).
* In this phase of selector 0x32, local +0x30 receives in turn I(clock − T1) (0x01D7F8), the
  clamped value p (0x01D827, 0x01D832 or 0x01D845; clamp exactly as Selectors §6 `0x32`
  step 3 states), and finally **p × 40** (image 0x2FCE40, bits 0x42200000; product at
  0x01D8CD, store at 0x01D8D5). The final value of ℓ for the refresh is p × 40; it replaces any
  pre-evaluator value (including the riding value 10).
* ℓ is not a blend target: it is used directly in the same refresh, unsmoothed. The left foot
  passes +ℓ and the right foot passes −ℓ (exact sign toggle) to the foot rotation step that
  Models §7.9.5 writes R*y*(±ℓ).
* The first phase and the after-phase of selector 0x32 do not write local +0x30, so there ℓ
  keeps its pre-evaluator value. The after-phase's values a and b go to local +0x2C (the
  scratch slot of Selectors §5), not to +0x30 (stores at 0x01DA1B with one push pending).

Every other Selectors §6 `0x32` step-3 fact (creature-angle Z = p × −270, Y = p × −40,
body-offset Z += 4, hand targets, head target, rate 0.03) is unchanged by this ruling.

### Evidence

| RVA | Fact |
|---|---|
| 0x01D7F8 | I(clock − T1) stored at stack +0x34 with one push pending: local +0x30 |
| 0x01D803 … 0x01D845 | divide by I(T2), × 1.5 (image 0x2FCDE0), clamp; p stored to local +0x30 |
| 0x01D8C7 … 0x01D8D5 | local +0x30 (stack +0x34, one push pending) × 40 → local +0x30 |
| 0x01D9AA | jump to the post-evaluator code 0x01E660 |
| 0x02395C | left-foot draw: local +0x30 read (depth D after the 16-byte argument block of 0x0241B0, which returns with RET 0x10) and passed to 0x0244F0 |
| 0x023B88 / 0x023B8E | right-foot draw: local +0x30 read, sign toggled with the mask at image 0x345F70 (0x80000000), passed to 0x0244F0 |

Between 0x01E660 and 0x023B88 no instruction stores to local +0x30: every store with a
stack-relative displacement of 0x30 … 0x5F in that range was checked against its pending
pushes (for example 0x01EBC0 is local +0x28, 0x01EF4D is +0x2C, 0x020A79 is +0x28, 0x01F35A and
0x01F3E4 are +0x38), and no address of local +0x30 is formed there. The selector-0x1E case
(pose 30) uses the same slot for its "ℓ := 10*q*₂" (0x01C512 … 0x01C520), consistent with Models.

### Residual uncertainty

* Static reading only. The internal action of 0x0244F0 (taken from Models §7.9.5 as the R*y*
  step) was not re-derived here; only that it receives ±ℓ was read.
* The store-alias check after the evaluator was done by displacement and pending-push counting,
  not by full stack simulation of every post-evaluator path.
* The Selectors §5 note that the scratch slot +0x2C has no traced later read is untouched by
  this ruling; only the claim about the 40 × p store is corrected.

---

## Ruling 3. Which selectors read the mount argument

**Winner: neither contract fully.** The mount argument is read by **selector 0x3A only**.

* Models row 58 (mount terms in selector 0x3A) is right.
* Poses §4, "The mount argument is read only inside the selector `0x15` case", is wrong: the
  selector-0x15 case does not read the mount at all.
* Selectors §12, "Selector `0x3A` also reads it", is right about 0x3A but wrong in implying that
  0x15 reads it.

### Behaviour

* The mount pointer is stack argument 10 (EBP +0x2C). It is read from the argument area once, on
  entry (0x012932), and stored to local **+0x80** (0x012942). No other instruction of the refresh
  reads EBP +0x2C.
* Selector 0x3A's case starts by loading local +0x80 (0x013C3D), tests it for null (0x013C44,
  null branches to 0x013D1F), and on nonnull reads mount +0x88 (0x013C4C) and the mount's 32-bit
  code at +0x64, exactly as Selectors §6 `0x3A` step 1 states.
* Selector 0x15 (pose 21, case 0x018591) **reuses local +0x80 as a temporary**: on each of its
  two half-phase paths it first stores into local +0x80 the address of a component of a target
  triplet (0x018709 and 0x01899F, each at stack +0x84 with one push pending) and only later loads
  local +0x80 back (0x0187A5 and 0x018A3B) to write through that address: 70 into the off-hand
  angle target Z on the first path, −70 into the main-hand angle target Z on the second, matching
  the repeated "ρ*L*.z := 70" and "ρ*R*.z := −70" writes of Models row 21 while *t* < D/2. No jump in the listing targets an address between a
  store and its reload (the nearby branches 0x0185DD → 0x0187B7 and 0x01887D → 0x018A4D land after
  the reloads), so every path to 0x0187A5 passes 0x018709, and every path to 0x018A3B passes
  0x01899F. The mount value is overwritten there, never read.
* No other case body, and nothing before the evaluator, reads local +0x80 or its pending-push
  equivalents (scan of 0x013A00 … 0x01E660 and of the pre-evaluator listing). That the
  compiler reuses the slot inside selector 0x15 also shows the mount value is dead after the
  evaluator.

So: with a nonnull mount and selector 0x3A, the mount terms of Selectors §6 `0x3A` / Models row 58
apply; for every other selector, including 0x15, the mount argument has no effect on the refresh.

### Evidence

| RVA | Fact |
|---|---|
| 0x012932 / 0x012942 | EBP +0x2C (argument 10) loaded and stored to local +0x80 |
| 0x013C3D … 0x013C4C | selector 0x3A: local +0x80 loaded, null test, mount +0x88 read |
| 0x013C54 | V read from local +0x90 (Selectors §6 `0x3A`), not the mount |
| 0x018709 / 0x0187A5 | selector 0x15, first branch: store then reload of local +0x80 (case's own pointer) |
| 0x01899F / 0x018A3B | selector 0x15, subtype-6/7 branch: same pattern |

### Residual uncertainty

* Post-evaluator reads at stack displacement +0x80 (for example 0x01ECC0, 0x01F50E, 0x01F9A9)
  were judged by pending pushes and the slot-reuse argument above, not by full stack
  simulation of every path; 0x01ECC0 was confirmed to be local +0x7C.
* What the mount's +0x64 code and +0x88 field mean is outside this ruling (Selectors §14).

---

## Summary of corrections

| Contract | Statement | Correct reading |
|---|---|---|
| Models App. C row 49, **W** | G((2, 0, 0)) with arm lift | *R* := *L*; *L*.x += 2; arm lift |
| Selectors §6 `0x31` step 2 | main rest := off rest, then off rest X += 2 | confirmed |
| Models App. C row 50, **A** | ℓ := 40*q* | confirmed (local +0x30, read by both foot draws as ±ℓ) |
| Selectors §6 `0x32` step 3 | p × 40 stored to the scratch slot, never read | stored to the body-lean local +0x30 and read by the foot draws |
| Poses §4 | mount read only in selector 0x15 | mount read only in selector 0x3A; 0x15 reuses the slot |
| Selectors §12 | 0x3A "also" reads the mount | 0x3A is the only reader |
| Models App. C row 58 | mount terms in selector 0x3A | confirmed |

Reconstruction sources were not consulted for these rulings.
