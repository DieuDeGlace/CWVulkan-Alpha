Status: FINAL / STABLE / HELD. Full author readback completed.

# Melee strike case, area heal and beam strike: attack-routine call sites 3 and 4

This independently authored, code-free contract closes the two world-update call sites of the attack routine that the held contract `alpha-attack-routine.md` left open: site 3 (the melee strike case) and site 4 (its strike-variant sibling, which turns out to be the beam case). It covers the action dispatch that reaches them, their timing gates, the target loops, every random draw in order, the damage and push-vector derivations with their callees, the area heal, and the work done after each call. The held contracts `alpha-attack-routine.md` and `alpha-combat-stat-queries.md` were used only to locate routines and for terms; every statement below was re-established from the original images. The Infinite-Cubes reconstruction was not needed for any statement.

Supported originals: `binary/Cube.exe` (client), SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, and `binary/Server.exe` (server), SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`. Both hashes were recomputed before use. Both images have preferred base `0x00400000`. Every address is an RVA. Where one address is given it is the server RVA; inside the world update routine the client RVA is the server RVA plus `0xDA240` (section 10). Callees are written client / server. Evidence paths are relative to `Documents/disassembled alpha cube world`.

## 1. Evidence method and terms

The server instruction listing (`decompiled/server/assembly.txt`) was read for the world update routine (server `0x1322D0`) from the action dispatch at `0x137BC4` to the end of the beam case near `0x13CA40`, and for every callee named below. Constants and jump tables were read from the image bytes. The client listing was compared instruction by instruction (section 10). Statements that a callee makes no draw rest on a transitive walk of `decompiled/server/calls.csv` from that callee.

Terms, as in the held contracts:

- **Striker**: the creature whose action is being processed. **Target**: the creature examined by a loop.
- **t**: the striker's signed 32-bit `+0x6C` (time within the current action). **dt**: the world update's first stack argument, a signed 32-bit frame time; the routine also keeps dt converted to binary32.
- **B**, **C**, **R0**: the wind-up, active-window and total timing queries of the held stat-query contract (client `0x03CAA0` / server `0x007DB0`, `0x047310` / `0x011D60`, `0x03D1A0` / `0x0084B0`), each called with the striker and the argument −1 (R0 takes none). They make no draws.
- **Q**: the attack-speed query (client `0x047700` / server `0x012150`), referenced but not re-specified here. It reads the striker's `+0x70`.
- **Integer division** below always truncates toward zero (signed). "Crossing x" means t ≤ x < t + dt.
- **Draw**, **unit draw**, **scaled unit draw with factor k**: as in the held attack-routine contract (one call through the `rand` import; draw as binary32 times k, divided by 32767).
- **Presentation record**: the 24-byte record of the held contract, appended to output-aggregate list `+0x08`. Its initialiser (`0x065FB0` / `0x0C8530`) sets T1 and T2 (offsets `+0x10`, `+0x14`) to 1; positions come from the raw actor position converted as in that contract. "Record *n*" below means a presentation record with identifier *n*; when no T1 is stated it is 1.
- **Sizes**: an actor's binary32 triple at `+0x80`, `+0x84`, `+0x88` is called size x, y, z. The striker's binary32 triple at `+0x28` is read only at index 2 (`+0x30`), used as an angle in degrees. The striker's binary32 triple at `+0x160` is called the aim vector.
- **Fixed** converts a binary32 to a signed 64-bit position unit: multiply by 65536 in binary32, then truncate (runtime conversion helper). Positions are signed 64-bit triples at actor `+0x10`.
- **The sets**: the striker's key sets at `+0x11AC` (struck set) and `+0x11B4` (deferred set). Clearing uses `0x27E480` / `0x0F8520`; insertion `0x07F000` / `0x130690`; the emptiness test `0x077230` / `0x130540`.
- **Buff entry replace-or-append** (`0x046AF0` / `0x011740`): for buff types 1, 2, 3, 6, 7, 9, 10 and 11, the first existing entry of the same type in the creature's buff list is overwritten with the new 24-byte entry; otherwise (or for other types) the entry is appended. A **buff notice** is the 40-byte record of the held contract (creature key twice, then the entry) appended to aggregate list `+0x58`.

## 2. Dispatch

At `0x137BC4` the striker's action byte minus 1 indexes a byte table at `0x148A8C` (client `0x222CCC`) when it is at most `0x6D`; the byte selects a target from `0x148A44` (client `0x222C84`). Action 0 and actions above `0x6E` go to the common tail. Two targets matter here:

- **Strike case** `0x138117`: actions `0x01`–`0x07`, `0x09`–`0x15`, `0x1E`–`0x21`, `0x33`, `0x36`, `0x39`, `0x3A`, `0x3C`–`0x3E`, `0x41`–`0x46`, `0x48`, `0x4A`–`0x4E`, `0x56`–`0x58`, `0x5B`, `0x5D` and `0x68`. Site 3 lies in this case.
- **Beam case** `0x13BA28`: actions `0x1C`, `0x5E` and `0x5F`. Site 4 lies in this case.

No other jump into either case was found. Both cases end in the **common tail** (`0x137CEF`, entered at `0x137CF5`/`0x137CFB`/`0x137D01`): when world byte `+0xB4` is zero or the striker is world `+0xB8`, dt is added to the striker's `+0x74`; when `+0x74` then exceeds 4000, Q is evaluated, `+0x70` is set to 0, Q is evaluated again, and t becomes the truncation of ((first Q ÷ second Q) × t as binary32). The tail then continues with work outside this contract.

**Common case gate.** Both cases first require world byte `+0xB4` nonzero, or striker byte `+0x60` nonzero, or the striker being world `+0xB8`; otherwise they go to the common tail.

## 3. Strike case before the strike

Steps run in this order every frame. Draws are marked.

**S1. Guard stance.** When striker byte `+0x140` is 4, byte `+0x141` is 1, the action is `0x11`, `0x05` or `0x14`, and t is 0, the striker's signed `+0x128` is set to 600.

**S2. Opening effects.** B is evaluated once here.

- Actions `0x57` and `0x58`. When t < B and t ÷ 200 differs from (t + dt) ÷ 200: record 36 at the striker, T1 = scaled unit draw with factor 0.25, plus 1 (**one draw**). Then, when t − dt < B ≤ t: an effect record (initialiser `0x065ED0` / `0x0C8510`) is built at the striker's position with constant fields (0.25, 1, 20, a vector (0, 0, 2) and 80; not otherwise specified), record 81 is appended at the striker with T1 = 1, and the effect record is appended to aggregate list `+0x10`. No draw.
- Actions `0x1E`–`0x21`. The same 200-step record 36 with **one draw** while t < B. Then, when crossing B: an effect record at the striker's position plus the fixed aim vector, with constant fields (0.25; when striker byte `+0x141` is 1, the value 2 and colour (0, 0.2, 1, 1), otherwise the value 1; 8; a vector (0, 0, 2); 20), then record 41 (when `+0x141` is 1) or 38 at the striker with T1 = 1, then the effect record to list `+0x10`. No draw.
- Every other action of the case. When t is 0, the action is not `0x5B`, and the **two-hand predicate** (`0x044230` / `0x00F2B0`: slot `+0xAA8` has category 3 and subtype 5, 6, 7, 8, 10, 11, 15, 16, 17 or 18) holds: record 17 at the striker, T1 = scaled unit draw with factor 0.1, plus 1 (**one draw**).

**S3. Swing record.** For every action of the case except `0x5B`, when crossing B: record at the striker with T1 = scaled unit draw with factor 0.2, plus 0.9 (**one draw**). The identifier is 16 for action `0x0A`, 48 for `0x0B`, 14 for `0x36`. Otherwise it is 15 and T1 is multiplied by 1.5; when the two-hand predicate holds or the action is `0x44`, `0x5D` or `0x45`, that product is further multiplied by 0.5.

**S4. Mid-swing record.** For actions `0x39`, `0x3C`, `0x3A` and `0x4A`, when t < B + C ÷ 2 ≤ t + dt: record 12 at the striker, T1 = scaled unit draw with factor 0.25, plus 1 (**one draw**).

**S5. Strike window.** Window start and end are computed:

1. start = (C ÷ 2) − 100 + B, raised to B if below it; end = (C ÷ 2) + B, replaced by B + C − 1 when it is at least B + C.
2. For actions `0x44` and `0x45`, start = (C ÷ 3) + B.
3. For action `0x5D`, start = (C ÷ 5) + B and end = B + C.

**S6. Periodic window and the area-heal flag.** A byte local at frame slot −`0x2C70` (the **area-heal flag**) is set to 0. For the **periodic actions** `0x48`, `0x56`, `0x1E`, `0x1F`, `0x20`, `0x21`, `0x0B`, `0x05` and `0x5B`:

1. When t < B, or t ≥ B + C, start and end are both set to B, and the rest of S6 is skipped.
2. The interval I is 250; 1000 for `0x5B`; C ÷ 3 for `0x0B` and `0x05`; C ÷ 6 for `0x1E` and `0x20`; C ÷ 12 for `0x1F` and `0x21`.
3. start = B + ((t − B) ÷ I) × I; end = start + 100.
4. Let N = B + ((t + dt − B) ÷ I) × I. When N differs from start and N < B + C: striker `+0x1314` takes the value of `+0x70`, the area-heal flag is set to 1, and both sets are cleared.
5. The area-heal flag is set to 1 when t + dt > end.
6. Except for action `0x05`: when t − dt < B, or (t + dt − B) ÷ I differs from (t − B) ÷ I, record 15 at the striker with T1 = scaled unit draw with factor 0.25, plus 1 (**one draw**); for action `0x5B` this is followed by record 82 with T1 = scaled unit draw with factor 0.1, plus 0.5 (**one draw**).

So the flag is set only for periodic actions, on a frame that enters a new interval inside the active window, or on which the current 100-unit window ends.

**S7. Wind-up reset.** When t ≤ B: striker `+0x1314` takes the value of `+0x70`; the **deferred-key pass** below runs; then both sets are cleared and striker byte `+0x13B8` is set to 0.

The **deferred-key pass**: for each key in the deferred set, in set order, that is not in the struck set, the creature is looked up (`0x02F000` / `0x005420`). When found, the **counter callback** (`0x03CF20` / `0x008230`) runs on it: when its `+0x128` is nonzero, its byte `+0x140` is 4 and `+0x141` is 1, its `+0x170` is raised by 0.25 (set to 1 above 1) and it receives a type-11 buff entry (value 0, duration 30000) by replace-or-append, with a buff notice. Then a hit record is built by the hit-record initialiser with: attacker key the striker, target key the creature, damage 0, byte `+0x14` = 0, the creature's raw position, type 4. It is appended to the hit list; the hit routine is not called. No draws.

**S8. Action `0x4A`.** When t < B + C ÷ 2 ≤ t + dt: record 82 at the striker, T1 = scaled unit draw with factor 0.1, plus 0.5 (**one draw**).

**S9. Window end.** When t ≥ end: the deferred-key pass runs, then only the deferred set is cleared.

**S10. Whiff reset.** When `+0x13B8` is 0, t + dt ≥ B + C, t < R0 and the striker's `+0x128` is 0: Q is evaluated, `+0x70` is set to 0, Q is evaluated again, and t is rescaled as in the common tail.

**S11. Charge (`+0x13B4`) and crit lock (`+0x13C0`).**

1. For actions `0x1F` and `0x21`: when crossing B, `+0x13B4` is set to 0.1 and `+0x170` is reduced by the action cost (S12) of the action with argument −1, then set to 0 if negative. Nothing else in this step runs for these two actions, so `+0x13C0` is not reset by this case for them.
2. For the other actions, when t is 0: `+0x13C0` is set to 0 and `+0x13B4` to 0. The cost of the action is evaluated; when it is greater than 0 (ordered), it is evaluated again, stored in `+0x13B4` and subtracted from `+0x170`. Otherwise:
   - actions `0x11`, `0x14`, `0x05`: `+0x13B4` takes `+0x170`, and `+0x170` is set to 0. When the striker has a type-11 buff, that buff is replaced by a type-11 entry with value 0 and duration 0 (a buff notice is appended) and `+0x13C0` is set to 1;
   - actions `0x36`, `0x15`, `0x57`, `0x58`: `+0x13B4` is set to 1.
3. Every frame, for every action of the case: when `+0x144` is greater than zero (ordered), `+0x13B4` takes `+0x144`, `+0x170` is reduced by `+0x144`, `+0x13B8` is set to 0, and for actions `0x0B` and `0x05` `+0x13B4` is halved. Then `+0x144` is set to 0, and `+0x170` is set to 0 if negative.
4. **Flag B3** = 1 when `+0x13B4` is greater than 0 (ordered) or the action is `0x0B`; otherwise 0.

**S12. Action cost** (`0x044AE0` / `0x00FB20`), by action: 0.1 for `0x03` and `0x04`; 0.3 for `0x1F`, `0x21`, `0x25`, `0x2B`, `0x2D`, `0x2E` and `0x5F`, except 0 when the striker has a type-9 buff; the held formula for `0x22`; 0 otherwise. No draws.

Other writers of `+0x13B4` outside this case (not part of the strike flow): the beam case's unreachable `0x68` branch (section 8), the cases at `0x13CE3A` and `0x13D3F4`, the action-switch code at `0x13FEC4` and `0x13FF15` (sets 0), and the server routines `0x006400` and `0x0110D0` (set 0, also clearing `+0x13C0`; their client counterparts were not paired). The value consumed from `+0x144` is written, among others, by the held attack routine's block step (E6) in the blocking target.

## 4. The strike

**K1. Gate.** The strike runs on the frames on which start < t ≤ end. Otherwise control goes past the target loop to the common tail. One strike per frame; the struck set stops repeat hits on the same target until it is cleared (S6 step 4 or S7).

**K2. Reach R and strike centre.** Let x be striker size x.

1. R = x × 1.5 and the forward factor F = 1.5.
2. Actions `0x0D`, `0x0E`, `0x02`, `0x01`, `0x09`, `0x04`, `0x03`, `0x07`, `0x06`, `0x12`, `0x13`: R = x × 2 and F = 1.
3. Then, in order: `0x14`, `0x15`: R × 2. `0x44`, `0x46`: F = 0. `0x56`, `0x5D`, `0x68`: R × 2, F = 0. `0x57`, `0x58`: R = 8, F = 0. `0x1E`, `0x20`: R = 3, F = 0. `0x1F`, `0x21`: R = 5, F = 0. `0x5B`, `0x4A`: R = 20, F = 0. `0x48`, `0x4E`, `0x4D`, `0x4C`: R × 1.2, F = 0.
4. Let d = x × F. The angle a is the striker's `+0x30` times 0.0174532924 (binary32). Cosine and sine are the precise double-precision imports applied to a widened to double, each rounded to binary32. The offset is (−sin a × d, cos a × d, 0), formed by a rotation about the vertical axis of the vector (0, d, 0) (`0x024610` / `0x002030`, then `0x088E50` / `0x0DDE00`). The strike centre is the striker's position plus the fixed offset.
5. Actions `0x1E`–`0x21`: the centre is instead the striker's position plus the fixed aim vector.
6. Actions `0x4B`, `0x45`: the fixed value of ((striker size z − x) × 0.5) is **subtracted** from the centre's z.
7. Actions `0x0B`, `0x3D`, `0x36`: the centre is the striker's position, and R is multiplied by 3.

**K3. Block breaking.** Only when world byte `+0xB4` is zero, x > 4 (ordered), and the action is not `0x57`, `0x5B` or `0x4A`.

1. n = the truncation of R, capped at 8. For integer offsets from −n to n, looped over the first axis outermost, then the second, then the third, a cell is examined when n × n (as binary32) exceeds the squared length of the offset; the cell is the world cell at the centre plus the offset in whole units (`0x02F860` / `0x006050`, then the test `0x20AA60` / `0x1306C0`).
2. For each such cell: an effect record goes to aggregate list `+0x10` at the cell, with the cell's three colour bytes each divided by 255, the vector (0, 0, 10), the count 3 and 0.5; the cell is removed (`0x071920` / `0x130470`) and a zone change record goes to aggregate list `+0x18`. No draws.
3. When any cell was removed: record 2 at the striker, T1 = scaled unit draw with factor 0.4, plus 0.5 (**one draw**). A further branch, taken when world `+0xB8` is null (`0x139CC8`), was not audited.

**K4. Strike setup.** The **bonus** is 1, or 0 for actions `0x3C`, `0x0B` and `0x3E`. The local **hit-any flag** is cleared. For action `0x1F` with t ≥ B, when the striker has a type-9 buff, it is replaced by a type-9 entry with value 0 and duration 0 (buff notice appended).

**K5. Target loop.** The loop walks the world's creature container (world `+0x04`) in container order. The striker is not excluded by an explicit test. For each entry:

1. Skip when the creature is null, when its health `+0x16C` is not greater than zero (ordered; NaN continues), or when its key is in the struck set.
2. **Horizontal reach.** r = target size x × 0.7 + R. Let D be the strike centre minus the target's position (64-bit). The test passes when (Dx × Dx) ÷ 65536 + (Dy × Dy) ÷ 65536, each a 64-bit integer product and signed division, is less than the truncation to 64 bits of (r × r × 65536) computed in binary32 (`0x087F80` / `0x01CE90`, `0x1A7B50` / `0x0DADE0`).
3. **Vertical reach.** Dz is the centre's z minus the target's z, converted to binary32 and scaled by 2^−16; its absolute value is taken in double precision and rounded to binary32. The test passes when (target size z × 0.5 + striker size z) + R exceeds that absolute value (ordered).
4. **Geometric test** (`0x19EE90` / `0x0D4D80`, internals not audited; no draws). Its arguments are, in order: the start point (a copy of the striker's raw position, or for actions `0x1E`–`0x21` the strike centre plus (0, 0, fixed R)); a pointer to the target's raw position; the byte 1; and the binary32 200. The loop skips the target when it fails.
5. **Hostility test** (`0x1988A0` / `0x0D18C0`, striker then target; not audited, no draws). When it fails, control goes to the area-heal check (section 6).
6. When the target's signed `+0x128` is nonzero, its key is inserted into the deferred set and the loop continues with the next creature (no area-heal check).
7. Otherwise the target is struck (K6–K8), the post-hit work runs (section 5), and the area-heal check follows.

**K6. Push vector P3.**

1. The target's position minus the striker's position is converted to binary32 (each 64-bit component as in the presentation conversion). z is set to 0. When the squared length exceeds 0.01 (ordered), the vector is normalised: each component times the binary32 reciprocal of the binary32-rounded double square root of ((x² + y²) + z²).
2. z is set to 0.25.
3. **A3** is 1 when striker byte `+0x13C0` is nonzero, with no draw; otherwise it is the crit roll Cr on the striker (**one draw**).
4. **Strength** s (`0x044860` / `0x00F8F0`) of the striker's action is evaluated (section 7.2).
5. The **size factor** g is the double-precision square root of (striker size x ÷ target size x), the quotient binary32, the root rounded to binary32 (`0x024B70` / `0x0024E0`). Each component is multiplied by g, then by s.

**K7. Damage S3.** P is `+0x13B4` read now.

- Actions `0x57`, `0x58`, `0x1E`–`0x21`: m = the damage scale (section 7.1) times the heal power; then **one draw** gives roll = 1.25 − (scaled unit draw with factor 0.5); S3 = (roll × m) × (0 + 1).
- Every other action: m = the damage scale times the attack power (held contract), then multiplied by ((P × P) × 5 + bonus); then **one draw**: S3 = ((1.25 − scaled unit draw with factor 0.5) + 0) × m.
- S3 is multiplied by 2 when A3 is set.

**K8. Call (site 3).** The **first-target flag** is set when the struck set is empty at this point and the striker's signed `+0x12C` is not positive. The target's key is inserted into the struck set, and striker byte `+0x13B8` is set to 1. The attack routine is then called (client `0x21476B` / server `0x13A52B`) with: target; striker; S3; A3; B3; W = `+0x13B4`; a pointer to the striker's position (never read); P3; the output aggregate; the changed-zone collection; argument 11 = 1 for actions `0x1E`–`0x21`, else 0; K = the action byte; a null pointer; and 1.

## 5. Work after site 3

Only when the attack routine returns 1, in this order:

1. **Action `0x1E`.** Roll Cw(0.25) on the striker (**one draw**). When it succeeds and striker byte `+0x141` is 0: a type-9 buff entry (value 0, duration 30000) by replace-or-append, a buff notice, and record 47 at the striker (no draw).
2. **Swing count.** When the first-target flag is set: striker `+0x70` is increased by 1 and `+0x74` is set to 0. (Routine `0x0453B0` / `0x0103A0` is called between them; it only walks the striker's `+0x139C` tree and has no effect.)
3. **Crit charge.** When B3 is set and striker bytes `+0x140` = 4 and `+0x141` = 0: `+0x1190` becomes `+0x1190` + `+0x13B4`, set to 1 if above 1.
4. **Mana gain.** When world byte `+0xB4` is zero or the striker is world `+0xB8`, the hit-any flag is still clear, B3 is clear, and the action is neither `0x1E` nor `0x20`:
   1. **One draw:** g = (1 − scaled unit draw with factor 2) × 0.05 + 0.1.
   2. g becomes (R0 as binary32 ÷ 500) × g.
   3. **One draw:** when that draw is divisible by 8, g is multiplied by 1.5.
   4. `+0x170` becomes `+0x170` + g, set to 1 if above 1.
5. The hit-any flag is set.

When the attack routine returns 0 none of this runs. After the loop, when the hit-any flag is set, R0 is evaluated on the striker unless the item in slot `+0x990` has subtype 13, or slot `+0xAA8` is non-empty while slot `+0x990` is not category 3; the result is discarded, so this has no effect. The case then ends in the common tail.

## 6. Area heal (`0x214BAC` / `0x13A96C`)

After every target that reached K5 step 5 (whether the hostility test failed, or the target was struck, whatever the attack routine returned, and any post-hit work finished), and not after K5 step 6:

1. The check requires the action to be `0x21` or `0x20`, the area-heal flag (S6) to be set, the hostility test (striker, target) to fail when re-evaluated, and striker byte `+0x141` to be 1.
2. **One draw:** roll = 1.25 − scaled unit draw with factor 0.5. The heal power of the striker is then evaluated.
3. Damage = ((heal power × roll) × (0 + 1)) × −0.1.
4. Byte `+0x14` = roll Cr on the striker (**one draw**, after the damage).
5. The record is built by the hit-record initialiser: attacker key the striker, target key the target, the target's raw position. It is appended to the hit list.
6. Record 42 at the striker, T1 = T2 = 1.
7. The hit routine is called with the record when world byte `+0xB4` is zero, or when both the striker and the target are world `+0xB8`.

The target is not inserted into the struck set, so a creature may be healed on every flagged frame of the window. Since the flag depends only on the periodic schedule, the heal is interleaved with strikes in target-loop order within the same frame.

## 7. Callees used to scale damage and push

### 7.1 Damage scale (`0x03EC90` / `0x009D10`)

By action: 0.5 for `0x05`, `0x1A`; 0.1 for `0x15`, `0x48`; 0.4 for `0x1E`, `0x20`; 0.6 for `0x1F`, `0x21`; 2 for `0x25`, `0x2B`, `0x39`, `0x3A`, `0x44`; 0.25 for `0x37`; 1 for every other value. Tables: index `0x009D90` (action − 5, up to `0x43`), targets `0x009D70`; client `0x03ED10`, `0x03ECF0`. No draws, no writes.

### 7.2 Strength (`0x044860` / `0x00F8F0`)

The result is 0 when the creature's byte `+0x60` is 5 or 3. Otherwise, by action (Q evaluated once where it appears):

| Strength | Actions |
| --- | --- |
| 6 ÷ Q | `0x01`, `0x02`, `0x09`, `0x0D`, `0x0E`, `0x0F`, `0x3C`, `0x3D` |
| 3 ÷ Q | `0x03`, `0x04`, `0x06`, `0x07`, `0x12`, `0x13`, `0x3E` |
| 10 ÷ Q | `0x41`, `0x42` |
| 4 | `0x05` |
| 10 | `0x0B`, `0x57` |
| 6 | `0x11`, `0x14` |
| 5 | `0x36` |
| 30 × v + 5 | `0x15`, `0x58`, where v is the action-specific value (`0x03ED60` / `0x009DE0`) for action `0x15` with second argument −1 |
| 0 | `0x1E`–`0x21` |
| 2 ÷ Q | every other value, including 0 and values above `0x58` |

Tables: index `0x00FAC8` (action − 1), targets `0x00FA90`; client `0x044A38`, `0x044A00`. No draws, no writes. Consequently P3 is the zero-scaled vector (each component multiplied by 0) for the heal actions `0x1E`–`0x21`.

### 7.3 Summary of P3

P3 = (unit horizontal direction from striker to target, or the unnormalised difference when its squared length is at most 0.01, with z = 0.25) × sqrt(striker size x ÷ target size x) × strength. Each multiplication is componentwise and rounded separately; the size factor is applied before strength.

## 8. Beam case and site 4

**B1. Before the beam event.** After the common case gate:

- Action `0x1C`: `+0x170` is reduced by dt (binary32) × 0.0002.
- Actions `0x5E`, `0x5F`: when crossing B, `+0x170` is reduced by the action cost; then, when the striker has a type-9 buff, it is replaced by a type-9 entry with value 0 and duration 0 (buff notice).
- Then `+0x170` is set to 0 if negative; in that case the case ends in the common tail.

**B2. Beam timing.** The case ends in the common tail when t + dt < B or t ≥ B + C. A **beam event** occurs when crossing B (all three actions), and, for `0x5F` and `0x1C` only, on frames with B < t < B + C on which (t + dt − B) ÷ 200 differs from (t − B) ÷ 200.

**B3. Beam event.**

1. The direction u is a copy of the aim vector, normalised (as in K6) when its squared length exceeds 0.
2. The origin O is formed by routine `0x046BB0` / `0x011800` (not audited). The length L is routine `0x1A35D0` / `0x0D6730` (not audited, no draws) applied to O, to the aim vector normalised without a zero check by `0x027870` / `0x012670`, and to 200, 0 and 1.
3. Both sets are cleared and the hit-any flag is cleared.
4. When striker bytes `+0x140` = 3 and `+0x141` = 1 and crossing B: a projectile record (initialiser `0x059660` / `0x022890`) whose `+0x48` is heal power × 0.1, placed at O plus the fixed value of the normalised aim vector scaled by L (`0x051510` / `0x0E1520`), is appended to world list `+0x14` and aggregate list `+0x20`. Its other fields are not specified here.
5. **Steps.** For step i = 0, 1, 2, … while L is greater than i (as binary32), the creature container is walked in order; after each walk O is advanced by fixed u.

**B4. Target tests (per step, per creature).** Skip when the creature is null or is the striker; when its key equals the striker's 64-bit `+0x11C0`; when its health is not greater than zero (ordered); when its key is in the struck set; or when its signed `+0x128` is nonzero (no deferred-set insertion here). Then:

1. **Range:** the horizontal test of K5 step 2 with D = O minus the target's position and r = target size x × 0.5 + 1.
2. **Vertical:** the absolute z difference (O minus target, converted as in K5 step 3) must be less than target size z × 0.5 + 1 (ordered).
3. The target's key is inserted into the struck set.
4. **A** = roll Cr on the striker (**one draw**).
5. A horizontal unit vector from striker to target with z = 0.25 is formed as in K6 steps 1–2; it is not used afterwards.
6. **One draw:** m = scaled unit draw with factor 0.05, plus 1. The heal power is then evaluated, and S4 = (heal power × 1.5) × m.
7. S4 is doubled when A is set.
8. The hostility test (striker, target) must pass; otherwise skip.
9. W4 = 0.1 for action `0x5F`, otherwise 0. V4 = (0, 0, 0).

**B5. Call (site 4).** The attack routine is called (client `0x2166C8` / server `0x13C488`) with: target; striker; S4; A; flag B = 1 for action `0x5F`, else 0; W4; a pointer to O (never read); V4; the output aggregate; the changed-zone collection; argument 11 = 1 (the action is `0x5F`, `0x5E` or `0x1C`); K = 0; a null pointer; and 1.

**B6. After site 4.**

1. Only when the routine returns 1 and striker byte `+0x141` is 2: a **drain record** is built by the hit-record initialiser with attacker and target keys both the striker, the striker's raw position, byte `+0x14` = A, and damage = −S4 (sign bit flipped). It is appended to the hit list; the hit routine is not called. When world byte `+0xB4` is zero, the striker's health becomes health − (−S4), and is lowered to the striker's maximum health (`0x044DB0` / `0x00FDA0`, not audited) when it exceeds it.
2. Whatever the return value: when striker bytes `+0x140` = 3 and `+0x141` = 1 and crossing B, another projectile record as in B3 step 4 is placed at O and appended to the same two lists.
3. The hit-any flag is set (for every target that reached the call, whatever it returned).

**B7. After the steps.**

1. When the hit-any flag is set: `+0x70` is increased by 1 (the no-op walk is called) and `+0x74` is set to 0.
2. When the struck set is empty and the action is not `0x68`: an effect record at the striker's position plus the fixed value of the normalised aim vector scaled by L, with constant fields not specified here (no draws), goes to aggregate list `+0x10`; then Q is evaluated, `+0x70` set to 0, Q evaluated again and t rescaled as in the common tail.
3. Action `0x5E`, when crossing B: record 40 at the striker, T1 = scaled unit draw with factor 0.1, plus 1 (**one draw**).
4. The case ends in the common tail.

**B8. Action `0x68`.** The dispatch sends `0x68` to the strike case (section 2), not here, and no other jump into the beam case exists. The beam case's `0x68` branches are therefore not reached from the dispatch: they would set `+0x13B4` from `+0x144` at t = 0, fix L to 10, widen the range radius by the step index, use S4 = ((1.25 − scaled unit draw with factor 0.5) + 0) × ((damage scale of `0x68`, which is 1) × attack power × ((P × P) × 10)) with one more draw, set W4 = P, and set V4 to the aim vector normalised (`0x027870` / `0x012670`, without a zero check) times 10 (`0x0252F0` / `0x0079F0`) with z then set to 5. Whether any routine between the dispatch and those tests could change the action byte was not exhaustively audited. In the strike case, action `0x68` takes the reach rule R × 2, F = 0 (K2), strength 2 ÷ Q, and damage scale 1.

## 9. Draws in order

Strike case, per frame (each only when its condition holds):
1. S2: record 36 (actions `0x57`, `0x58`, `0x1E`–`0x21`) or record 17 (other actions).
2. S3: swing record.
3. S4: mid-swing record 12.
4. S6 step 6: record 15, then record 82 for `0x5B`.
5. S8: record 82 for `0x4A`.
6. K3: block-breaking record 2.
7. Per target, in container order: A3 (unless `+0x13C0` is set), S3, then inside the attack routine its own draws (held contract), then Cw(0.25) for `0x1E` on success, then the two mana draws on the first eligible success of the frame, then the area-heal roll and its Cr.

Beam case, per beam event: per step and target, Cr then the S4 draw (and, for the unreachable `0x68` branch, a second damage draw), then the attack routine's own draws; after the steps, record 40 for `0x5E`.

The deferred-key pass, the counter callback, the action cost, strength, damage scale, size factor, heal and attack power, Q, B, C and R0, the hostility, geometric, reach, raycast and origin routines, the set, list and buff helpers, and the block helpers make no draws and do not reach the attack or hit routine (transitive walk of the server call list). Some list-growth and allocator helpers contain computed calls that the call list does not resolve.

## 10. Client and server

The server range `0x137BC4`–`0x13CB00` and the client range `0x211E04`–`0x216D40` contain 4114 instructions each and agree instruction by instruction after masking absolute addresses. Every branch target inside the range differs by exactly `0xDA240`. Every data constant referenced agrees in value; the only differing data reference is the `rand` import slot (client `0x2FC468`, server `0x1582F4`), both resolving to `rand`. Callees pair consistently (for example `0x044860` / `0x00F8F0`, `0x03EC90` / `0x009D10`, `0x024B70` / `0x0024E0`, `0x027870` / `0x012670`, `0x0252F0` / `0x0079F0`, `0x19EE90` / `0x0D4D80`, `0x1988A0` / `0x0D18C0`, `0x196D30` / `0x0CFD50`, `0x195A60` / `0x0CEA80`). The dispatch, strength, damage-scale and action-cost index tables are byte-identical, and their targets agree as offsets from their routine. These callees were also compared instruction by instruction with no difference apart from addresses: strength, damage scale (client listing omission at `0x03ECB6`–`0x03ECBF` read from the image bytes: it loads 0.25), size factor, `0x027870`, `0x0252F0`, the two-hand predicate, the counter callback, the no-op walk, buff replace-or-append, buff search, normalisation, rotation, matrix application, the reach comparison and the horizontal squared length. No behavioural difference was found.

## 11. Unresolved

- The game meaning of all fields, actions, record identifiers and lists. "Strike", "charge", "crit lock", "guard", "beam", "drain", "mana", "swing count" and "heal" are descriptive labels only. The held attack-routine contract labels `+0x70` "action time"; in this case it behaves as a counter raised per landed swing and reset by S10, B7 and the common tail.
- Internals of: the geometric test `0x19EE90` / `0x0D4D80`; the hostility test `0x1988A0` / `0x0D18C0`; the beam origin `0x046BB0` / `0x011800` and raycast `0x1A35D0` / `0x0D6730`; maximum health `0x044DB0` / `0x00FDA0`; the cell lookup, cell test, cell removal and zone-change helpers; the branch at `0x139CC8`; Q (only referenced).
- The full layouts of the effect records (aggregate `+0x10`), zone change records (`+0x18`) and projectile records (world `+0x14`, aggregate `+0x20`), and how the hit routine treats type-4 records.
- The upstream gating of the per-creature loop that reaches the dispatch, and the container order of world `+0x04`.
- Whether the striker can be its own target in the strike loop (no explicit exclusion; it depends on the untraced hostility and geometric tests).
- Whether the action byte can change between the dispatch and the beam case's `0x68` tests (section 8, B8).
- Division by zero when C is small enough to make the periodic interval I zero (C ÷ 3, ÷ 6 or ÷ 12), and behaviour for zero or NaN sizes in the size factor; not established.
- Computed calls inside list-growth and allocator helpers, not resolved by the call list.
