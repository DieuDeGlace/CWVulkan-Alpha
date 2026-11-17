Status: FINAL / STABLE / HELD

# Alpha ruling: channel2 / Explosion window for actions 30–33, 87, 88, and the action-104 beam sounds

Scope: two disagreements between held contracts about world-step action sounds.

1. The window that bounds the 200-ms channel2 (sound 36) ticks of actions 30–33, 87, 88 and
   the Explosion (sound 81) of actions 87, 88: `alpha-audio.md` §6.2 states it with P1,
   `alpha-particles.md` §5.3–5.4 with "the action's duration D".
2. Sounds 5 / 16 for action 104 (0x68): `alpha-audio.md` §6.2 lists them (and an exception in
   the 37 / 36 row for "the step on which action 104 plays its at-P1 sound");
   `alpha-open-points-3.md` §2.2 says the action-104 branch of the beam block never runs.

Images: client Cube.exe (SHA-256 84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF)
and Server.exe (SHA-256 1FFBCBCE251E1506A63C9CE18AB0009D5553FCBB9E6202BE2BC3A7D870D34FB1),
both 32-bit, preferred base 0x400000; hashes recomputed before use. Addresses below are
image-relative RVAs written client / server (add 0x400000 for the VAs `alpha-audio.md` uses).
Inside the world-step routine (client 0x20C510 / server 0x1322D0) the server RVA is the client
RVA minus 0xDA240.

Notation: *τ* = the creature's action time (+0x6c) as read by the step, before this step's
advance; dt = the step length (the same frame slot added to *τ* everywhere below); P1 = the
value of the phase routine client 0x03CAA0 / server 0x007DB0 called with argument −1; P2 = the
second-phase routine client 0x047310 called with −1. *u* = float(`rand()`) × *k* ÷ 32767.0 in
binary32, then + *a*.

## 1. Channel2 ticks and Explosion window (actions 30–33, 87, 88)

### Ruling

**No behavioural conflict: both contracts name the same quantity, and `alpha-audio.md`'s
wording (P1) wins.** The "duration D" of `alpha-particles.md` §5.3–5.4 is the value of the
same routine with the same argument (client 0x03CAA0, −1) that `alpha-audio.md` calls P1 — the
first phase only, not the whole action (P1 + P2). Every window below is bounded by P1;
`alpha-particles.md` §5.3–5.4 must be read with D := P1.

Exact behaviour, per creature passing the step's usual filter, in the default action handler
(client 0x212357 / server 0x138117), which serves 30–33, 87 and 88:

1. **No swing-slow.** Actions 30–33, 87, 88 branch away before the *τ* = 0 two-hander test, so
   sound 17 never plays for them and its draw is not made.
2. **Channel2 tick (sound 36)** — the same test for 30–33 and for 87, 88:
   * window: **strictly *τ* < P1** (skipped when *τ* ≥ P1);
   * tick: trunc((*τ* + dt)/200) ≠ trunc(*τ*/200), signed integer division truncating toward
     zero (multiply by 0x51EB851F, shift 6, add sign bit). For *τ* ≥ 0 this is "a multiple of
     200 lies in (*τ*, *τ* + dt]": a boundary equal to *τ* + dt counts, one equal to *τ* does not,
     and 0 never ticks. At most one sound per step, whatever dt is;
   * the grid is absolute action time (not offset by P1). Because only the step's start is
     tested, the last tick may fall on a boundary in [P1, *τ* + dt] of the step that crosses P1
     (e.g. P1 = 800: ticks at the steps crossing 200, 400, 600 and 800);
   * on a tick: one `rand()` drawn, record id 36 at the creature's position (+0x10), volume
     default 1, pitch 1 + 0.25*u* (*k* = 0.25 at client 0x345D98, *a* = 1.0 at 0x345DC0,
     divisor 32767.0 at 0x2FD338). No draw when there is no tick.
3. **Release / Explosion**, immediately after the tick test:
   * **30–33 (staff release):** on the step with **P1 > *τ*** and **P1 ≤ *τ* + dt** (late form,
     *τ* < P1 ≤ *τ* + dt): the burst of `alpha-particles.md` §5.3 is built (20 particles, size
     0.25, v₀ (0, 0, 2), spread 8, style 2 with colour (0, 0.2, 1, 1) when byte +0x141 = 1, else
     style 1), then a sound record 41 (+0x141 = 1) or 38 (otherwise) with pitch field 1.0 is
     written, then the burst is appended to section 3. **No draw.**
   * **87, 88 (fire explosion):** on the step with ***τ* − dt < P1** and **P1 ≤ *τ*** (the first
     step whose start has reached P1): the burst of `alpha-particles.md` §5.4 is built (80
     particles at +0x10, size 0.25, v₀ (0, 0, 2), spread 20, style 1), then record id **81**
     with pitch field 1.0 and default volume, then the burst is appended. **No draw.** The
     client, on playing a record of id 81 (or 82), sets the camera-shake value G + 0x1E8 to
     1.0 right after the play call (client 0x095630 … 0x095642; `alpha-audio.md` §1.3).
   * Because the tick needs *τ* < P1 and the Explosion needs *τ* ≥ P1, the Explosion step never
     carries a channel2 tick. The staff-release step can carry one (the tick record precedes the
     release sound).
4. **At-P1 swing (sound 15)** follows, for these actions as for any other except 91: on the step
   with ***τ* ≤ P1 < *τ* + dt**, one `rand()` is drawn (pitch (0.9 + 0.2*u*)·1.5, then ×0.5 if a
   two-hander or action 68, 69, 93; the draw precedes the action-10/11/54 id choice, none of
   which applies here).
   * 30–33: in the release step when P1 < *τ* + dt (release then swing, same step); when
     P1 = *τ* + dt the release comes one step earlier than the swing.
   * 87, 88: in the same step as the Explosion only when *τ* = P1 exactly (Explosion then
     swing); otherwise the swing comes in the step before the Explosion.
5. Later rows of `alpha-audio.md` §6.2 follow in their listed order (for 30–33 the multi-hit
   swings of `alpha-open-points-3.md` §2.1; 87, 88 are not multi-hit actions).

Per-step random-draw order for one such creature is therefore: [channel2 tick draw, if a tick]
→ [release or Explosion: none] → [at-P1 swing draw, if at P1] → later rows. The burst records
carry no draws; their particle draws (6 or 7 per particle, `alpha-particles.md` §2.1) are made
later by the client's frame update when it expands section 3, not inside the step.

* **Right:** `alpha-audio.md` §6.2 rows 36 and 81 (including the strict *τ* < P1 tick window
  and the *τ* − dt < P1 ≤ *τ* Explosion test) and its re-certification item 8;
  `alpha-particles.md` §5.3–5.4 edges (*T* < *D* ≤ *T* + dt, *T* − dt < *D* ≤ *T*), counts,
  spreads and sounds.
* **Wrong (wording only):** `alpha-particles.md` §5.3 "the action's duration *D*"; it is the
  first phase P1, not the action's total length.

### Evidence

* Phase routine identity: the default handler's at-P1 swing compares against client 0x03CAA0(−1)
  (client 0x2123A9 … 0x2123B2, used at 0x2126F3 … 0x212708); the multi-hit and beam windows
  add client 0x047310(−1) to the same value to bound the second phase (client 0x212B43 …
  0x212BD8; 0x215CFF … 0x215D22). Hence 0x03CAA0(−1) is P1, and it is the routine both
  contracts cite.
* Branching: client 0x2123BA … 0x2123ED (87, 88 → 0x21279C; 30–33 → 0x212495), bypassing the
  *τ* = 0 / two-hander swing-slow at 0x2123F3 … 0x212485.
* 30–33 tick: client 0x212495 … 0x212545 (*τ* ≥ P1 skips at 0x2124A3; the two divisions at
  0x2124AF … 0x2124D2; id 0x24 written at 0x212533) / server 0x138255 … 0x138305.
* 30–33 release: client 0x212552 … 0x2126DC (*τ* ≥ P1 exits at 0x212568; *τ* + dt < P1 exits at
  0x212588; spread 8.0 at 0x212650; count 0x14 at 0x212678; id 0x29 / 0x26 chosen at
  0x21269F … 0x2126B0; pitch 1.0 at 0x2126C3; append at 0x2126DC).
* 87, 88 tick: client 0x21279C … 0x21284C / server 0x13855C … 0x13860C.
* 87, 88 Explosion: client 0x212859 … 0x21294F (*τ* − dt ≥ P1 exits at 0x212879; *τ* < P1 exits
  at 0x212895; size 0.25, style 1, spread 20.0 at 0x2128D4 … 0x2128E8; count 0x50 at 0x212910;
  pitch 1.0 and id 0x51 at 0x21293B … 0x212945; record write 0x21294F; burst append via
  0x2126D8 … 0x2126DC) / server 0x138619 … 0x138655 and onward, parallel.
* At-P1 swing after both paths: client 0x2126ED … 0x212A45 (draw at 0x212738).
* Constants read from the image bytes: 0x345D98 = 0.25, 0x345DC0 = 1.0, 0x2FD338 = 32767.0,
  0x2FCDBC = 0.2, 0x2FD31C = 0.9, 0x2FCDE0 = 1.5, 0x345DA0 = 0.5.

## 2. Sounds 5 / 16 for action 104 (0x68)

### Ruling

**`alpha-open-points-3.md` §2.2 wins.** The beam-block branch that plays sound 5 (`hit1`) or 16
(`shield-swing`) at P1 for action 104 (client 0x216BC7 … 0x216C6A / server 0x13C987 …
0x13CA2A) is **never executed**, in either image. So:

* Sounds 5 / 16 **never** play for action 104 from the world step, and that branch plays them
  for **no** action (its only entry condition is action 104).
* The `alpha-audio.md` §6.2 row "5 / 16 … at P1, action 104" is dead and must be dropped; the
  37 / 36 row's exception "except the step on which action 104 plays its at-P1 sound" is
  vacuous. The beam-block channel sound (37 if the beam struck something this step, else 36,
  pitch 1 + 0.25*u*, absolute 200-ms grid) plays only for actions **28 and 95**; 94 skips it.
  Its beam-struck flag (frame byte at −0x2B29) only ever selects between 37 and 36.
* Action 104 is served by the default handler, where it is treated like any generic action:
  swing-slow 17 at *τ* = 0 when holding a two-hander (pitch 1 + 0.1*u*), and the generic at-P1
  swing 15 (pitch (0.9 + 0.2*u*)·1.5, ×0.5 when a two-hander). The default handler contains no
  action-104 sound test (its only 104 comparison, client 0x213916, scales a melee value, no
  sound). The rest of the beam block's 104 code (charge-to-mana at *τ* = 0, the single
  10-long march, client 0x215C89 … 0x215CDB and the 104 tests at 0x215FE9, 0x2162F6, 0x2164B0,
  0x2165EE, 0x21666A, 0x2169EC) is equally unreachable.

Within the world step, sound 16 plays only for action 10 at P1 (client 0x212782); sound 5 is
played elsewhere (weapon-impact sounds of `alpha-creatures-behaviour.md` §5), not by any
world-step action row.

### Evidence

* Dispatch, client 0x211E04 … 0x211E1B: action − 1 (> 109 → loop continuation 0x215EC0) indexes
  the byte map at client 0x222CCC, which selects an entry of the table at 0x222C84. Decoded from
  the image bytes: entry 4 (client 0x215C68, the beam block) is selected only for **28, 94,
  95**; **104** selects entry 0 (client 0x212357, default handler) together with 1–7, 9–21,
  30–33, 51, 54, 57, 58, 60–62, 65–70, 72, 74–78, 86–88, 91, 93. Server map 0x148A8C / table
  0x148A44 decode identically (beam 0x13BA28 for 28, 94, 95; 104 on default 0x138117).
* No other entry: a scan of every direct jump and call in the client listing whose target lies
  in client 0x215C68 … 0x216D34 and whose source lies outside finds only four, all to 0x215EC0
  (from 0x211E0E, 0x212372, 0x2154AA, 0x2157C3); 0x215EC0 reloads the creature and jumps to the
  per-creature loop head (0x211F41), i.e. it is the "skip" exit, not beam code. The handler
  located just before the block (client 0x215BB3) ends with an unconditional backward jump
  immediately before 0x215C68, so there is no fall-through.
* The beam block writes no byte to any +0x68 field, so an actor entering as 28, 94 or 95 cannot
  become 104 inside it. The only immediate store of 104 into an action byte in the client is in
  the creature combat routine (client 0x02E66F, inside 0x02CB20), which is not among the beam
  block's callees.
* Channel-sound exit for 94: client 0x216C80 … 0x216C84 jumps past the 37 / 36 record when the
  action is 0x5E.

## Evidence method

Static disassembly of both original images over the cited ranges; jump-table bytes and float
constants read from the image bytes through the section tables. Reconstruction sources
(Infinite-Cubes/alpha) were not consulted for this ruling.

## Unresolved

* Not verified at runtime; both rulings are static readings.
* The step's *τ* is taken, as in `alpha-audio.md` §6.2, to be the pre-advance action time; the
  advance itself was not re-traced here. Steps are assumed to advance *τ* by dt.
* The relative order of the step's sound draws and the client frame update's particle-expansion
  draws on the shared C-runtime generator depends on thread arrangement (`alpha-audio.md`
  AU-7); not settled here.
* The beam block's callees were not exhaustively audited for indirect writes to the actor's
  action byte; the only direct "set action 104" site found is outside them (low residual risk
  for ruling 2).
* The camera-shake value's later consumption is as described in `alpha-audio.md` §1.3 /
  `alpha-physics-camera.md`; not re-derived.

## Re-certification (2026-11-16)

Authored and read back in full on 2026-11-16 from the two original images identified above
(hashes recomputed that day). Conflict 1: no behavioural conflict; P1 (client 0x03CAA0 with −1)
bounds both the channel2 ticks (strict *τ* < P1) and the 87/88 Explosion (*τ* − dt < P1 ≤ *τ*);
`alpha-particles.md` §5.3's "duration D" is P1 by name only. Conflict 2: in favour of
`alpha-open-points-3.md` §2.2; `alpha-audio.md` §6.2's 5 / 16 row for action 104 and the 104
exception of the 37 / 36 row are dead. No existing spec was edited. No reconstruction leads,
implementation material, tests, captures or logs were used.
