Status: FINAL / STABLE / HELD

# Alpha open points, batch 1: traps, the quest banner, the beam end

Clean-room behavioural specification. It closes three open points of earlier specs:

| Point | Spec it belongs to | Section here |
|---|---|---|
| **I-5** trap damage, range, timing, occupant table | `alpha-interaction.md` §6.4, §10 | §1 |
| **M-1** what shows the "QUEST COMPLETE!" banner | `alpha-missions.md` §8.4, §12 | §2 |
| **AP-3** trigger of the beam-end burst | `alpha-particles.md` §5.6, §12 | §3 |

It also corrects `alpha-audio.md` §6.4 (ids 52 and 53 are played, §1.5 here) and the list
named in `alpha-missions.md` §8.4 (§2.1 here). Companion spec names are pointers only;
statements attributed to them were not re-verified in this document unless stated.

**Addresses.** Every code or data address is an RVA (image base 0x400000 in both images)
and names its image: **Server** = `Server.exe`, **Cube** = `Cube.exe`. Both programs make
the same decisions where both are cited. Offsets written "+0x…" are field offsets inside an
object, not addresses. "G" is the client game object, "W" a world object, "O" a step's
output set.

**Conventions** (those of the cited specs): positions are 3 × signed 64-bit integers in
1/65536 block; "draw" is one C runtime `rand()` call (import slot Server RVA 0x1582f4,
Cube RVA 0x2fc468); *believed* marks a reading that is not fully certain; *as read* marks
behaviour that looks unintended but is what the code does.

Output-set lists used here (byte offsets inside O): +0x00 hit records, +0x08 sound
records, +0x10 particle records, +0x30 static-state records, +0x60 mission records. The
companion specs call these sections 2, 4, 3, 6 and 13.

Routines read: Server world step RVA 0x1322d0 (static-entity part 0x133097–0x133bab,
including the occupant case bodies 0x1331da–0x1335e2), Server hit routine 0xcfd50, Server
hit application 0xcea80, Server state setter 0xd8c90, Server level lookup 0xd2340, Server
static constructor 0xc84b0, Server spike-trap placement 0x1058ed–0x1059f1; Cube twins
0x20c510 (static part 0x20d3b0–0x20dd57), 0x196d30, 0x195a60, 0x1a58e0, 0x19a9b0; Cube
frame update 0x88ee0 (0x8cf20–0x8cf49, 0x8fdc3–0x8fec1, 0x904b0–0x9081b); Cube beam block
0x215c68–0x216b26 and its Server twin 0x13b800–0x13c8b0. Constants were read from
the images' data sections.

---

## 1. Traps: static kinds 6, 7 and 8 (resolves I-5)

### 1.1 Who runs it

The trap rules are part of the static-entity pass of the world step (Server RVA 0x133097 …,
Cube RVA 0x20d3b0 …; `alpha-server-simulation.md` §3 step 6). For every static of every
active zone, in vector order:

1. **Always**, on every world including an online client: timer (+0x34) += dt (Server
   0x133183, Cube 0x20d3c3). Online clients therefore advance the timers themselves; only
   the state byte (+0x30) comes from the server.
2. The occupant handling of §1.6 (its re-pin runs on every world; its release does not).
3. **Only when the world is not remote** (world +0xb4 = 0: the server, or an offline
   client; gates Server 0x1336b5, Cube 0x20d8f5): the trap rules below.

An online client never changes a trap's state and never produces a trap hit itself.

### 1.2 Arming cycle (kinds 6, 7, 8 alike)

State 0 and state ≠ 0 are the two positions of the trap (*believed*: 0 = retracted, ≠ 0 =
extended, per `alpha-interaction.md` §4.4).

Each step, after the timer was advanced:

* If timer > **2000** (strictly) and state ≠ 0: the setter (§1.5) sets state 0, and a
  static-state record of the static is appended to O+0x30.
* If timer > 2000 and state = 0: walk the world's creature list (world +0x4) in order; at
  the **first** creature that exists and whose controller kind (+0x60) is **0** (a player)
  and whose squared distance to the trap is **< 900** (the test of §1.4), the setter sets
  state 1 and a static-state record is appended; the walk stops there. No player in range:
  nothing; the state stays 0 and the timer keeps growing.
* Otherwise (timer ≤ 2000): nothing.

The setter makes timer := 1000 − timer, replaced by 0 when negative, and only when the
state actually changes. Because the timer is > 2000 whenever the cycle calls it, every
change resets the timer to **0**. So with a player within 30 blocks a trap alternates
about **2 s in state 1, 2 s in state 0** (each phase ends in the first step whose timer
exceeds 2000), for as long as its zone is processed. The return to state 0 does not depend
on any player. The 30-block test is a **sphere** (z included) around the static's position
(+0x08; *believed* x, y = footprint centre and z = base).

Start phase: the static constructor (Server RVA 0xc84b0) creates every static in state 1
with timer 0. The one traced trap generator (Server RVA 0x1058ed–0x1059f1; *believed* the
builder of `alpha-worldgen-structures.md` §1.10.6 step 4) places a spike trap (kind 7) at a
candidate point where one draw d₀ gives d₀ % 40 = 0; its position is (+5, +5, +1) blocks
from a 10-block grid point and its scale 10 × 10 × 10; then one draw d₁ gives rotation
(+0x20) = d₁ % 4 (sign-preserving remainder), then one draw d₂ gives timer = d₂ % 4000. On the first active step about half
of such traps therefore go to state 0 at once and the rest stay in state 1 until their
timer passes 2000. Whether any generator places kinds 6 or 8 is unresolved (Q-4); if one
does not set the timer, such a trap starts with timer 0.

### 1.3 When a trap in state ≠ 0 hurts

Right after §1.2, if state ≠ 0:

* **Kinds 6 and 7**: when ⌊(T + dt) / 100⌋ ≠ ⌊T / 100⌋, where T is the world's millisecond
  clock (W+0x8000bc, `vegetation-wind.md` §2) **after** this step's advance (both programs
  add dt to it before the static pass: Server 0x132438, Cube 0x20c678) and ÷ truncates
  toward zero (signed). In words: once per step in which the clock would pass a multiple
  of 100 ms — about **ten hit passes per second** at ordinary step lengths; never more than
  one per step.
* **Kind 8**: when timer = 0. The timer was advanced earlier in the same step, so (for
  dt > 0) it is 0 only in the step in which the setter just changed the state to 1:
  **one hit pass per slam**.

The change to state 1 of §1.2 and a hit pass may happen in the same step. A trap that went
to state 0 in this step does not hurt.

A hit pass walks the world's creature list (world +0x4) and hurts every creature that

* exists (list slot not empty),
* has squared distance to the trap **< 25** (the test of §1.4; a 5-block sphere around the
  static's position), and
* has controller kind (+0x60) **≠ 1** (kind 1 is the hostile-monster kind of the creature
  specifications; every other controller kind, players included, is hurt).

A target with health (+0x16c) ≤ 0 is ignored by the hit routine (its first test, Server
0xcfd9f, Cube 0x196d7f).

### 1.4 The distance test

Both tests (900 and 25) use the same fixed-point rule (Server RVAs 0x2c50, 0x6260 → 0x56c0,
0x2d10; Cube RVAs 0x2c7a0, 0x3b4a0, 0x2c860): with Δ = static position − creature position
(+0x10) in 1/65536 block, each of Δx², Δy², Δz² is computed in 64 bits and divided by
65536 with truncation toward zero, the three are summed, and the creature is in range when
the sum is **strictly less** than *n*·65536 (*n* = 900 or 25). Equivalent to "squared
distance in blocks < *n*" up to the last 1/65536.

### 1.5 The hit and the sounds

For each creature *C* of a hit pass, in list order:

1. **Knock-back k**: v = position(*C*) − position(static), converted to float blocks
   (× 1/65536), v.z := 0; if |v|² > 0, v := v/|v|; k := 20·v, then k.z := **5**; for kind
   8, k := 2·k. So **k = (20·ûx, 20·ûy, 5)** for kinds 6 and 7 and **(40·ûx, 40·ûy, 10)**
   for kind 8; a creature exactly above the centre gets (0, 0, 5) or (0, 0, 10).
2. **Level** *L* (Server RVA 0xd2340, Cube RVA 0x19a9b0) from the static's (x, y): cell x =
   (x ÷ 65536) ÷ 2048, both truncating toward zero; cell y = trunc(y × 1/2048 in double
   precision) ÷ 65536, truncating. A cell outside 0 … 8191 on either axis has no record.
   The cell's location record (`alpha-missions.md` §0) is used when it exists, its type
   (+0x18) is not 0 and the normalised location distance f(record, position) ≤ **1.0**
   (`alpha-worldgen-regions.md` §7): then *L* = the record's level (+0x24); otherwise
   *L* = 1. So a trap inside a typed location hurts at that location's level; anywhere else
   at level 1.
3. **Damage** *x* = **10 · 2^((L − 1) · 0.25)** for kinds 6 and 7, **50 · 2^((L − 1) ·
   0.25)** for kind 8 (the power is taken with base 2.0; constants 0.25, 10 and 5 at Server
   RVAs 0x1586c8, 0x15873c, 0x15872c and Cube RVAs 0x345d98, 0x345e74, 0x2fce24). It does
   **not** depend on the victim and is not random.
4. **Hit routine** (Server RVA 0xcfd50, Cube RVA 0x196d30), called with target *C*, **no
   attacker**, amount *x*, critical flag 0, record area byte 0, a zero float, hit position
   = the static's position, knock-back k, the step's output set, magic flag = (kind = 6),
   and the remaining arguments 0 except the last, which is 1. With no attacker the routine
   takes a short path (Server 0xd0e5d, Cube 0x197e3d):
   * **one draw is consumed** and its value is not used;
   * the target's hit flash (+0x1184) := 0.5;
   * a velocity step (Server 0xd0e7e–0xd0fc8, Cube 0x197e5e–0x197fa8) compares |velocity|²
     (+0x34) with max(|velocity|², |k|²) and rescales the velocity only when the first is
     strictly greater, which cannot happen (outside NaN values): **the velocity is left
     unchanged** *as read*;
   * the amount is multiplied by the value of every buff of kind 1 on the target (buff
     list +0x1178);
   * a hit record is built: attacker id 0, target id (*C* +0x08), amount, critical 0,
     stun 0, position = *C*'s position (+0x10), knock-back k, area byte 0, record kind
     byte 0; it is appended to O+0x00;
   * the attacker-dependent parts (blocking, absorb, immunity, armour and resistance,
     the critical hurt voice, stun) are not reached. So the magic flag of kind 6 has no
     effect *as read*;
   * finally, only when the world is not remote, the application routine runs.
5. **Application** (Server RVA 0xcea80, Cube RVA 0x195a60), as far as read:
   * a target with health ≤ 0 is ignored;
   * on a world without a local player (the server), a player target's record is only
     forwarded (its own client applies it, `alpha-net-packets.md` §5.1) and nothing else
     happens there;
   * otherwise k is the knock-back offered to the target (scaled by 0.1 when the target's
     hit-stun (+0x12c) is > 0, the record's own stun being 0): for a target whose
     controller kind is not 6 and that is either a non-player on a non-remote world or the
     world's local player, the target's push vector (+0x4c) := k when |k|² ≥ |push|²
     (Server 0xced9a–0xcee51, Cube 0x195d9a–0x195e27);
   * the amount is subtracted from health (+0x16c) subject to the routine's eligibility
     test of the target (Server 0xcefee; *believed* an "alive/damageable" check);
   * if health then is ≤ 0, the death branch runs; it sets health 0 and appends sound
     **27** at the victim's position (volume 1, pitch 1; Server 0xcf186–0xcf1a1) among
     other effects. What a trap death credits is unresolved here (§4).

**Sounds** (the state setter, Server RVA 0xd8c90, Cube RVA 0x1a58e0, appends a 24-byte
sound record to O+0x08 at the static's position, id, volume 1, pitch 1, only when the state
actually changes; file names *believed*, per `alpha-audio.md`):

| Kind | To state ≠ 0 | To state 0 |
|---|---|---|
| 6 | **53** `fire-trap.wav` | none |
| 7 | **52** `spike-trap.wav` | none |
| 8 | 54 | 54 |

The same setter selects **51** for kind 5 and **54** for every kind other than 5, 6 and 7,
on any change. This **corrects `alpha-audio.md` §6.4**: ids 52 and 53 are played; the
setter selects them by the static's kind rather than by a literal at a call site. Id 52 is
heard every time a spike trap goes to state 1. Id 53 is reachable only if some static of
kind 6 exists (Q-4). A trap hit makes no sound of its own; only a kill adds the death sound
27 of the application.

### 1.6 Occupant table (the per-kind jump table of I-5)

When a static's occupant id (+0x40, 64 bits) is not 0, the step looks the occupant up. An
occupant that no longer exists, or that has hit-stun (+0x12c) > 0, releases the static at
once (occupant := 0, static-state record appended), not remote only.

Otherwise the step dispatches on the static's kind through two byte tables of 54 entries
covering kinds 16 … 69 (Server byte tables RVAs 0x148964 and 0x1489a8 with address tables
0x148958 and 0x14899c; Cube byte tables 0x222ba4 and 0x222be8 with address tables 0x222b98
and 0x222bdc). Both tables have the same contents; only five kinds have their own case:

| Kinds | Case |
|---|---|
| 16, 18 (*believed* stone stool, bench) | **seat** |
| 19, 68, 69 (*believed* bed, beach towel, wood mat) | **bed** |
| every other kind | none |

1. **Re-pin** (first table, every world; seat and bed cases only): occupant position
   (+0x10) := static position + (0, 0, 0.5·occupant size.z (+0x80, third float) + static
   scale.z (+0x24, third float)); occupant velocity (+0x34) := 0; the third float of the
   occupant's orientation vector (+0x28) := (rotation + 2)·90 for a seat, rotation·90 for
   a bed, with rotation the static's +0x20 integer; occupant action (+0x68) := **83**
   (seat) or **84** (bed).
2. **Release test** (every occupied static whose occupant exists with hit-stun ≤ 0,
   whatever its kind; the release itself not remote only): release when the squared
   distance occupant − static in float blocks is > **16**, or the occupant's +0x40 vector
   has squared length > 0, or its action is neither 83 nor 84. After step 1 the action
   test cannot fire for kinds 16, 18, 19, 68, 69, and the distance test fires only when
   half the occupant's size.z plus the static's scale.z exceeds 4 blocks.
3. Otherwise **re-pin again** (second table, seat and bed cases only) without touching the
   action.

So a seated creature that is hit (hit-stun > 0) releases its seat; this rule does not
change the creature's action itself.

### 1.7 Worked example

A spike trap (kind 7, scale 10 × 10 × 10) stands inside a location of type ≠ 0 and level 5
with f ≤ 1, so *L* = 5. Its timer starts at 3127 (d₂ % 4000), state 1.

1. First active step (dt = 16): timer 3127 + 16 > 2000, state 1 → state 0, timer
   1000 − 3143 < 0 → 0, no sound (kind 7 to state 0), static-state record.
2. More than 2000 ms later a player stands 3 blocks east of the static's position at about
   the same height: squared distance ≈ 9 to 10 < 900 → state 1, timer 0, sound 52 at the
   static's position, static-state record.
3. Hits: squared distance < 25, controller 0 → hurt. *x* = 10 · 2^(4 · 0.25) = **20** per
   pass, k = (20, 0, 5). With 16 ms steps the clock passes a multiple of 100 ms in about
   every sixth step, so a player that stays inside the 5-block sphere for the whole 2 s
   takes about **20 passes = 400 damage**. Whether k moves the player out of the sphere is
   decided by the push-vector and movement rules (`alpha-physics-camera.md`), not here.
4. At timer > 2000: state 0 (silent); about 2 s later state 1 again if a player is still
   within 30 blocks.

A **stomp trap** (kind 8) in the same place would deal one hit of 50 · 2 = **100** with k =
(40, 0, 10) in the step it changes to state 1 (sound 54), and sound 54 again when it
returns to state 0.

A trap outside every typed location (*L* = 1) deals 10 (kinds 6, 7) or 50 (kind 8) per hit.

---

## 2. The "QUEST COMPLETE!" banner (resolves M-1)

### 2.1 Which list the frame update walks

`alpha-missions.md` §8.4 names the list G+0x800638. That is a misreading of the frame
update (Cube RVA 0x88ee0); the lists are:

* The game keeps one **received world update** at G+0x800630, thirteen lists at an 8-byte
  stride laid out like the step's output set (+0x00 hits, +0x08 sounds, +0x10 particles,
  … +0x30 static states, … **+0x60 mission records**). *Believed*: the network code fills
  it. The mission-record reader of `alpha-missions.md` §6.3 (Cube 0x8fdc3–0x8fec1) walks
  **G+0x800690** = G+0x800630 + 0x60.
* The frame update runs the local world step (Cube 0x8cf2e in a loop of 20 ms sub-steps,
  then 0x8cf49 for the remainder) with a frame-local output set O on the stack. It then
  **splices** received lists to the end of O's lists: G+0x800640 (particles) into O+0x10
  (Cube 0x904ce–0x90506), G+0x800638 (sounds) into O+0x08 (0x9052d–0x90565), and
  **G+0x800690 (mission records) into O+0x60** (0x90580–0x905b8). It then empties all
  thirteen received lists G+0x800630 … G+0x800690 (0x905bd–0x9064c).
* The banner loop (Cube 0x90690–0x907dc) walks ***O*+0x60**: the frame's **mission
  records**, local ones first, then received ones.

So the "event records" of §8.4 are mission records (layout `alpha-missions.md` §6.1).
Their "kind byte +0x25" is the record's **state** byte (byte 37), and the "+0x18 ≠ 0" test
is the record's **mission kind** (bytes 24–27).

### 2.2 The rule

For each mission record *R* of the frame, in list order, with *R*.kind ≠ 0:

* *R*.state = 1: the creature whose id is in bytes 8–15 is looked up in the client world
  (G+0x2e4) and, if it exists, Cube 0x882e0 is called for it (*believed*: opens its speech
  bubble; whether any record carries a non-zero id there is open point Q-2).
* ***R*.state = 2**: L = the client's location record of cell (*R* bytes 0–3, *R* bytes
  4–7) (Cube 0x87da0; none when a cell coordinate is outside 0 … 8191 or its region is not
  loaded). If L exists:
  1. the banner `questtag` (G+0x800868) plays its plasma sequence **`shine`** (string at
     Cube 0x3018f8) through Cube 0x236810 with argument 0 (*believed*: from the start;
     `alpha-ui-plasma.md` §6.5);
  2. its child **`objective`** (string at Cube 0x301864) gets L's objective text (Cube
     0x77fa0, `alpha-missions.md` §8.3) through Cube 0x236a00;
  3. interface sound **30** (*believed* `missioncomplete.wav`) at the listener position
     (G+0x140), volume 1, pitch 1 (Cube 0x84350).

Records with kind 0 (`alpha-missions.md` §2.3 step 2) and records with state 0 show
nothing. There is no de-duplication: several state-2 records in one frame restart the
sequence each time (the last record's text stays) and play sound 30 once per record.

**Duration**: the banner loop sets no timer; what is seen and for how long depends on the
`shine` sequence (open point Q-1).

### 2.3 Where state-2 records come from

| World | Source | Who sees the banner |
|---|---|---|
| Offline | the death branch of the client's hit application (Cube RVA 0x195a60): when it sets a mission's state to 2 it appends a mission record with that cell to O+0x60 (Cube 0x196579); the qualifying conditions are those of `alpha-missions.md` §5.2 (not re-verified here) | the local player, in the frame of the kill |
| Server | the same point of the Server application (Server RVA 0xcea80, append at 0xcf599) | per `alpha-missions.md` §6.2 the mission records are sent to every client (not re-verified here); each client then shows the banner when that cell's region is loaded |

So the banner follows a **mission record of state 2**, not any reward record. Whether
other paths (the map-region answer, a remote client's handling of received hits) produce
state-2 records is unresolved (Q-3, §4).

### 2.4 Example

Offline, the player kills the target of a kind-1 mission of cell (4097, 4100)
(`alpha-missions.md` §10.6). The death branch sets state 2 and appends the record to the
step's O+0x60. In the same frame the banner loop finds kind 1, state 2, cell (4097, 4100),
whose region (512, 512) is loaded: `shine` is played, `objective` shows that mission's
objective text, sound 30 plays.

---

## 3. The beam end (resolves AP-3)

### 3.1 The beam tick (Cube RVA 0x215ecb …, Server twin)

For a creature *A* whose action (+0x68) is **28** or **95**, let *t* = action time (+0x6c),
dt the step, *D* = the action's duration (Cube RVA 0x3caa0 with argument −1: **0** for
action 28; trunc(1000 / (*s* × *A*+0x17c)) for action 95, with *s* the attack-speed routine
of §3.2 and 1000 at Cube RVA 0x2fd8f4). A **tick** happens in a step when

* ⌊(*t* + dt − *D*)/100⌋ ≠ ⌊(*t* − *D*)/100⌋ (÷ truncating toward zero), and
* *t* + dt > *D*.

That is, once every 100 ms of action time after the wind-up. Because truncation maps
−99 … 99 to 0, the first tick comes when *t* − *D* passes **100**, not 0 (*as read*).

Action **104** (duration 0 in the same table) enters the same body by a separate entry
(Cube 0x215d3a–0x215d56): only in the step where *t* ≤ *D* < *t* + dt, without the 100 ms
test (further gates before that entry are not traced here). Its differences are noted
below.

A tick:

1. **Direction** û = *A*'s aim vector (+0x160), normalised when its squared length is > 0
   (left as is otherwise).
2. **Origin** S from Cube RVA 0x46bb0: *A*'s position, or, for a creature with
   appearance flag 4 (+0x7e bit 2), a position derived from *A*'s model transform
   (*believed* a head or hand point; Q-6).
3. **Length** *L* = the distance returned by the ray routine Cube RVA 0x1a35d0 from S
   along û, limited to **200** blocks: it stops at the first block whose type is neither 0
   nor 2 (*believed*: air and water). Action 104 uses *L* = **10** instead.
4. **Clear** *A*'s per-tick hit set (+0x11ac) and a second set (+0x11b4).
5. (For class (+0x140) 3 with specialization (+0x141) 1, a beam visual record is queued
   when *t* ≤ *D* < *t* + dt; with ordinary steps shorter than 100 ms this coincides with
   no tick. Not part of this point.)
6. **March** (Cube 0x2161e6–0x2169ab): if *L* > 0, for *i* = 0, 1, 2, … while *i* < *L*:
   the point P starts at S and is advanced by û (converted to fixed point) after each
   *i*. At each P every creature *C* of the world list (world +0x4) is tested, in list
   order: *C* exists, *C* ≠ *A*, *C*'s id (+0x08) ≠ *A*+0x11c0, *C*.health > 0, *C* not
   yet in the hit set, *C*+0x128 = 0, horizontal squared distance P↔*C* (x and y only,
   the fixed-point rule of §1.4) < r² with r = 0.5·*C*.size.x + 1 (+ *i* for action 104),
   and |P.z − *C*.z| < 0.5·*C*.size.z + 1. A *C* that passes:
   * is **added to the hit set first** (Cube 0x2163d7);
   * then, **before the hostility test**, the critical roll (Cube RVA 0x444a0, called at
     0x2163e2) consumes **one draw**, one further **draw** scales the damage (0x216460),
     and for action 104 one more **draw** follows (0x21652a); Server twin: 0xf520 at
     0x13c1a2, draws at 0x13c220 and 0x13c2ea;
   * then, only if *A* and *C* are hostile to each other (Cube RVA 0x1988a0 at 0x21658e;
     *believed* Server twin 0xd18c0 at 0x13c34e), the hit routine runs and the tick's "hit" flag is set
     (whatever the routine returns). A friendly creature therefore consumes the same
     draws without being hurt.
7. If the hit flag is set: *A*'s combo (+0x70) += 1, Cube RVA 0x453b0 is called for *A*
   (not traced), and +0x74 := 0.

### 3.2 The burst

After the march (Cube 0x2169d9, Server 0x13c799): **if the hit set is empty and the
action is not 104**, write the burst of `alpha-particles.md` §5.6 (Cube 0x2169f6–0x216ad5):

* position = *A*'s **position** (+0x10, not S) + *L*·û — the ray's end shifted by
  (position − S) for creatures with appearance flag 4 (*as read*);
* velocity = −û (1 block/s back toward *A* when û is normalised), spread 3 (the particle
  record's default, Cube RVA 0x65ed0);
* colour (1, 0.2, 0.5), size 0.1, count 5, style 1;
* appended to O+0x10.

Then *A*'s combo := 0 and its action time is rescaled: *t* := trunc(*t* · s_old / s_new)
(truncation toward zero), where s is the attack-speed routine (Cube RVA 0x47700) evaluated
before and after the combo reset. That routine reads the combo only for class 1
specialization 0 (adds 0.5 · min(combo / n, 1)) and class 3 specialization 1 (adds
min(combo / n, 1)), n being a per-creature count (Cube RVA 0x44cb0); for those creatures a
miss after a combo makes s_new < s_old and so increases *t*. For other creatures the combo terms
are absent; whether the rest of the routine (Cube RVA 0x478b0 and a kind-12 buff term)
depends on the combo is not traced (Q-5).

So:

* **The list examined** is *A*'s per-tick hit set (+0x11ac). It is **empty** when no
  creature at all — friend or foe — passed the tests of step 6 during this tick. A
  friendly creature standing in the beam is added to the set without being hurt and
  **suppresses** the burst (*as read*).
* **Per tick**, not per block step and not once per action: up to ten bursts per second
  while the beam touches no creature, at the ray's end (wall, or 200 blocks out). A ray of
  length ≤ 0 (*A* against a wall) skips the march and puts the burst at *A*'s position.
* A miss also **resets the combo**; a tick with a hostile creature raises it by one.

Who runs it: the owner rule of `alpha-server-simulation.md` §1, as for every particle
producer of `alpha-particles.md` §5 (pointer; not re-verified here).

### 3.3 Example

A monster in action 95 with *s* × +0x17c = 1 (*D* = 1000 ms) beams at a wall 12.4 blocks
away; no creature is near the line. Ticks come when *t* passes about 1100, 1200, 1300 …
ms. Each tick marches P through *i* = 0 … 12 (13 points), finds nothing, writes five
particles at position + 12.4·û moving back at 1 block/s with spread 3, then sets the combo
to 0 (no draw is consumed by the beam in such a tick). When a hostile player steps into the
beam 6 blocks out, the tick at *i* = 6 adds the player to the set, consumes two draws, runs
the hit routine, raises the combo, and writes no burst.

---

## 4. Remaining open points

| Id | Question |
|---|---|
| **Q-1** | Length and look of the banner's `shine` sequence, and whether the words "QUEST COMPLETE!" are part of its graphics rather than set by code (no code in the banner loop sets them). |
| **Q-2** | What Cube RVA 0x882e0 does for a state-1 record, and whether any record carries a non-zero creature id in bytes 8–15. |
| **Q-3** | Whether the Server's map-region answer (packet 12; earlier cited as Server RVA 0x272d0, not re-verified) sends cells whose records reach O+0x60 with state 2, re-showing the banner. |
| **Q-4** | Whether any generator places static kinds 6 or 8, and whether the Cube image has its own twin of the spike-trap placement of §1.2. |
| **Q-5** | Whether Cube RVA 0x478b0 or the kind-12 buff term of the attack-speed routine depends on the combo. |
| **Q-6** | The model offset of the beam origin for appearance flag 4 (Cube RVA 0x46bb0). |
| **Q-8** | What a trap kill credits (missions, kill rewards, threat): the death branch of the application was not re-read for the no-attacker case. |
| **Q-9** | The effect of Cube RVA 0x453b0 called on a beam hit. |

---

## Re-certification (2026-11-14)

Source-side re-check of every factual statement against `Server.exe` and `Cube.exe`
(original images and their disassembly; image base 0x400000 in both).

**Confirmed** (restated as RVAs): static-pass order (timer, occupant, trap rules); the
remote gates; 2000 ms threshold; first-player search with 900 and controller kind 0;
setter timer rule 1000 − timer floored at 0 and state-change-only behaviour; kind 6/7
clock test after the clock advance and kind 8 timer-0 test; hit-pass filters (exists, < 25,
controller ≠ 1); the fixed-point distance rule (strictly less than *n*·65536); knock-back
k values; level lookup (type ≠ 0, f ≤ 1.0, level +0x24, else 1); damage formula and
constants 0.25, 10, 5 in both images; no-attacker short path with one discarded draw and
hit flash 0.5; hit-record kind 0; sounds 52, 53, 54 by kind; constructor state 1 and spike
timer d % 4000; occupant seat/bed cases, re-pin values, actions 83/84, release thresholds
16 and 0; banner list chain G+0x800630 / +0x690 → O+0x60, kind and state tests, `shine`,
`objective`, sound 30 at G+0x140; mission-record appends in both images; beam tick test,
durations 0 (action 28) and 1000/(s·+0x17c) (action 95), length cap 200 and 10 for action
104, hit-set clearing, march bounds and creature tests, burst values (count 5, size 0.1,
style 1, colour (1, 0.2, 0.5), velocity −û, spread 3), combo reset and truncating
rescale.

**Corrected** (old → new):

1. Address form: all citations were image VAs → now RVAs with the image named.
2. **Reversal.** Hit-routine knock-back: "velocity keeps its direction and takes length
   max(|velocity|, |k|)" → the velocity step can never fire; velocity is unchanged *as
   read*; k reaches the target through the application's push vector (+0x4c), scaled by
   0.1 when the target is stunned.
3. Server distance helper cited as 0x4056c0 → the call is to RVA 0x6260, which calls
   0x56c0.
4. Occupant tables: "client 0x622bdc and its partner" (an address table) → Cube byte
   tables 0x222ba4 / 0x222be8 with address tables 0x222b98 / 0x222bdc; Server address
   tables added.
5. Occupant release: "after step 1 only the +0x40 test can fire" → the distance test can
   also fire (half size.z + scale.z > 4); the release test applies to occupied statics of
   every kind.
6. "A sitting creature that is hit is thrown off its seat" → the seat is released; the
   creature's action is not changed by this rule.
7. "A trap hit itself makes no sound" → true except that a kill adds death sound 27.
8. Setter sounds: added that kind 5 selects 51 and all other kinds 54 on any change.
9. **Random-draw order.** Beam march: added the critical-roll draw and the damage draw
   (plus one for action 104) consumed for every creature that passes the beam tests,
   before the hostility test, friendly creatures included.
10. Action 104: "the block also serves action 104" (implied per tick) → it runs the body
    once, at the step where *t* ≤ *D* < *t* + dt, without the 100 ms test.
11. Water-mage beam visual: "per tick" → only when *t* ≤ *D* < *t* + dt.
12. Beam hit: added the call to Cube RVA 0x453b0 alongside combo +1.
13. Ray stop: "first solid block" → first block whose type is neither 0 nor 2.
14. Attack-speed rescale (Q-5): "*believed* s depends on the combo" → it does for class 1
    spec 0 and class 3 spec 1; open only for the rest of the routine.
15. **World generation.** Spike-trap placement: added the 1-in-40 placement draw before,
    and the rotation draw (d₁ % 4) between it and the timer draw (d₂ % 4000), plus
    position (+5, +5, +1) and scale 10³. The timer rule itself is unchanged.
16. Level lookup: cell y is computed through a double multiplication by 1/2048 before the
    division by 65536 (not a plain integer division as the x axis).
17. Frame update: received lists are emptied from Cube 0x905bd (was 0x4905c9); only the
    sound, particle and mission-record splices are stated (were "each received section").
18. Hit-routine argument list: named only the arguments whose use was read.

**Removed**: the note on the decompiler listing omitting jump-table bodies (not native
behaviour); the "Suggested game check" column of §4 and open point Q-7 (verification
procedures); the closing advice on marking companion points answered; client animation
and draw-duration statements of the worked example; the knock-back escape estimate of the
example (depended on the reversed rule); the banner screen position; "record field 26";
"seen in fire colours"; the online-client record-receipt sentence and the server row
statements on reward items and packet 12 (moved to Q-3).

**Unresolved**: Q-1 to Q-6, Q-8, Q-9 above; plus the *believed* readings marked in place
(state meaning, kind names, static position semantics, network filling of the received
set, `missioncomplete.wav` name, application eligibility test, ray block types, and the
companion-spec pointers marked "not re-verified").
