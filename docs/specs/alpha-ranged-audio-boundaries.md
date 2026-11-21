# Alpha ranged audio boundaries

**FINAL / STABLE / HELD**

Status: independently worded source-side contract for native sound-event boundaries of the listed ranged and charging actions. It does not certify complete combat, audio playback, networking, physics, or runtime parity. Re-certified against original evidence on 2026-11-21; see the final section. Other held contracts are unchanged.

## Supported image and evidence

The original image is `binary/Cube.exe`, relative to the assigned `disassembled alpha cube world` evidence directory: 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, PE machine `0x014c`, timestamp `0x51ea955e`, preferred base `0x00400000`, image size `0x003bc000`. All addresses below are RVAs. Native structure offsets describe this 32-bit image, not a required replacement layout.

Evidence consists of individually selected original instructions in `decompiled/client/assembly.txt` (and the per-function exports `0060c510.asm` and `00488ee0.asm`); original binary bytes for dispatch tables and for instruction ranges missing from the export (notably `0x2151f9`–`0x21548f`, `0x215bb3`–`0x215c68`, and the input-table destinations `0x09b6e9`–`0x09bd43` and `0x09be00`–`0x09bf4d`); the original call list `decompiled/client/calls.csv`; and the original function files `00484350.c`, `00465fb0.c`, `004864c0.c`, `00444230.c`, `004456a0.c`, `00459660.c`, and `00424860.c`. A textual search of exported call sites alone was not treated as proof of absence.

No reconstruction material, replacement engine, SDK, exporter, tests, probes, captures, logs, implementation history, or other specification was inspected. No original file was modified.

## Established boundary of the common-cue claim

The reviewed native routes do not add a universal weapon-lift or swing cue to physical actions 22/23/26, volley actions 50/55/96, magic actions 37–46/89/90/108, or held actions 24/25/27/36/59/63/64. Their own branch sounds, the separate input-start routes described below, and independent projectile/physics events must be kept distinct.

The action dispatcher at `0x211e04` reads the action byte, subtracts one, sends values above `0x6d` to the default destination, and otherwise selects a destination through the byte table at `0x222ccc` and the address table at `0x222c84`:

| Actions | Native destination |
| --- | --- |
| 22, 23, 26 | `0x21707a` |
| 50, 55, 96 | `0x217634` |
| 37–46, 108 | `0x217a21` |
| 89, 90 | `0x2151f9` |
| 24, 25, 27, 36, 59, 63, 64 | `0x215bb3` |

The sound that can resemble an initial weapon lift, sound 17, is constructed at `0x21241c` through `0x212485` inside the different action branch beginning at `0x212357` (actions 1–7, 9–21, 30–33, 51, 54, 57, 58, 60–62, 65–70, 72, 74–78, 86–88, 91, 93, 104). That site requires action clock zero, a true result from the equipment predicate at `0x044230`, and the surrounding action-specific exclusions (actions 30–33, 87, 88 and 91 bypass it). Its pitch is one plus random-scaled 0.1 (binary32 bits `0x3dcccccd`). The predicate is true when equipment category byte `0xaa8` equals 3 and subtype byte `0xaa9` is one of 5, 6, 7, 8, 10, 11, 15, 16, 17, or 18; that list includes 6, 7 and 8 but does not move this sound outside its enclosing action branch. None of the actions in the table dispatches to that branch. Its sound-15 site at `0x212a2b` through `0x212a45` is likewise branch-specific. That branch's own ownership gate differs from the ranged gate below (it also admits every creature when world byte `0xb4` is nonzero); this does not affect the ranged branches.

The reviewed ranged destinations finish by jumping to the post-dispatch corridor at `0x211f35`, `0x211f3b`, or `0x211f41`; they do not fall through into the sound-17 or shared sound-15 sites above. The later common clock-zero cooldown initialization and clock increment at `0x218507` through `0x218541` contain no sound construction or random draw. Charging growth at `0x21a74d` through `0x21a918` runs after dispatch and supplies no start/lift sound.

This is a bounded absence result for those action/start/common-update routes. It is not a claim that an entire frame containing a ranged creature is otherwise silent. Earlier interactions, damage, movement, rolling, other creatures, pre-existing projectiles, and downstream playback can produce independent sounds.

## Sound identities and numeric conventions

The native sound-play routine at `0x084350` (recorded in `decompiled/client/functions/00484350.c`) maps numeric sounds to content filenames. Filenames identify the selected owner content; no audio assets are included here.

| Numeric sound | Native content filename |
| --- | --- |
| 15 | `swing.wav` |
| 16 | `shield-swing.wav` |
| 17 | `swing-slow.wav` |
| 18 | `swing-slow2.wav` |
| 22 | `salvo2.wav` |
| 36 | `channel2.wav` |
| 38 | `fireball.wav` |
| 41 | `watersplash.wav` |
| 55 | `charge2.wav` |
| 92 | `manashield.wav` |
| 93 | `bulwark.wav` |

The sound record constructor at `0x065fb0` initializes the binary32 fields at record offsets `0x10` (pitch) and `0x14` (volume) to one; the signed 32-bit numeric sound is at `0x0c`, and the position is copied in from the given source. The outgoing sound-event insertion helper is `0x0864c0`, which appends the record to a list. An outgoing insertion is not proof of when or how an audio device subsequently plays it.

For the randomized binary32 pitches below, the native order is: C runtime `rand` result converted to binary32, multiplied by the stated coefficient, divided by binary32 `32767.0` (bits `0x46fffe00`), then added to the stated base. Preserve those separate operations. The binary32 constants are read from 0.1 at `0x2fcb7c` (bits `0x3dcccccd`), 0.25 at `0x345d98`, 0.5 at `0x345da0`, 1 at `0x345dc0`, 2 at `0x345e10`, 1.25 at `0x2fcdd0`, zero at `0x345d84`, and 32767 at `0x2fd338`; the magic firing volume described as 0.9 has binary32 bits `0x3f666666`.

Action 26's launch pitch uses binary64 arithmetic: convert the random integer to binary64, multiply by binary64 bits `0x3fe051eb851eb852` (0.51), divide by binary64 bits `0x40dfffc000000000` (32767), add binary64 bits `0x3fe6666660000000` (the promoted binary32 value approximately 0.7), then convert the result to binary32 with the current rounding mode. A wholly binary32 calculation is not native arithmetic. Evidence: `0x21756c` through `0x217592` and constants at `0x31e1e8`, `0x302a80`, and `0x31e1f0`.

Scheduling terms: "old clock" is the signed 32-bit creature action clock at offset `0x6c` before its general increment; "update" is the update's signed 32-bit milliseconds (the same value is in effect throughout dispatch); "new time" is their sum. Anticipation and strike durations are the current results of `0x03caa0` and `0x047310` for the creature. Creature action is the byte at `0x68`; stored charge is binary32 at `0x144`; released physical charge is binary32 at `0x13b4`; attack direction is the three-binary32 vector at `0x160`, and "positive direction" means its squared length (`0x024860`) compares strictly greater than binary32 zero.

These facts cover ordinary nonnegative updates, finite values, valid state, and arithmetic without overflow. Signed integer quotients truncate toward zero (including the divisions by 200, by 3 and by 4 below). None of the periodic tests below is a catch-up loop over skipped boundaries unless a separately described outer subdivision revisits the test.

## Ownership used by the ranged branches

For physical launch (`0x21707a`), arrow volley (`0x217634`), held charge (`0x215bb3`), and magic actions 37–46/108 (`0x217a21`), the local creature is always permitted. Another creature is permitted only when world byte `0xb4` is zero and creature control byte `0x60` is nonzero. "Local" means equality with the creature pointer at world `0xb8`. Failure skips all of that branch's tests, effects and random draws and continues at `0x211f41` (the held branch reaches `0x211f41` through the default destination `0x215ec0`, which does nothing else).

Actions 89/90 differ: their channel sound precedes this ownership gate, and only their release is behind it. Evidence: `0x2151f9` through `0x21548f`.

## Physical launch sounds: 22, 23, and 26

Evidence: `0x21707a` through `0x217634`, especially `0x2170c7` through `0x21714c` and `0x2174d9` through `0x217624`.

When the old clock is at most anticipation and the new time is strictly greater than anticipation, the branch first transfers stored charge: released charge at `0x13b4` takes the stored charge value and stored charge is cleared. The launch test that immediately follows uses the same convention, so this transfer always precedes an ordinary launch in the same update. Landing exactly on anticipation does not qualify. Action 26 alone can otherwise launch when released charge is strictly positive and the update crosses anticipation plus 300 ms with the same inclusive-old, strict-new convention. The ordinary crossing is tested first; one update produces at most one launch through this branch.

A launch is "powered" when released charge is strictly greater than zero, and action 23 is always powered (including zero charge). A powered launch removes buff 10 if present (`0x21733f`–`0x2173e3`); that removal adds no sound or random draw.

Every launch takes, in order: one damage random draw (`0x2174d9`), world-projectile insertion, outgoing-projectile insertion, then one pitch random draw and one outgoing sound insertion. An update with no launch takes neither draw and produces no launch sound. No direction test precedes the launch.

| Launch state | Sound | Pitch | Volume |
| --- | --- | --- | --- |
| Action 22, not powered | 15 | 2 plus random-scaled 0.5 | 1 |
| Action 22, powered | 22 | 1 plus random-scaled 0.5 | 1 |
| Action 23 | 22 | 1 plus random-scaled 0.5 | 1 |
| Action 26 | 15 | Binary64 calculation above | 1 |

Sound position is the creature position (creature offset `0x10`). This sound-15 event is the launch effect itself, not an extra common weapon-lift layer. Action 26 constructs a projectile of type 2 (projectile record offset `0x60`, `0x2173f7`); actions 22/23 construct type 0.

## Arrow-volley sounds: 50, 55, and 96

Evidence: `0x217634` through `0x217a21`, particularly `0x217758` through `0x217859` and `0x2179ab` through `0x217a11`.

On the anticipation crossing (old at most anticipation, new strictly greater) the branch transfers stored charge to released charge, spends it, and removes buff 10 if present; none of this produces a sound or draw.

The period is the integer strike duration divided by four. A volley visit requires, in this order: old clock at least anticipation; old clock strictly below anticipation plus strike duration; differing integer quotients of new time and old clock divided by the period (operands not reduced by anticipation); positive direction. A visit failing any test produces nothing further.

Each qualifying visit inserts one sound 22 at the creature position, volume one, pitch one plus random-scaled 0.5. Its pitch draw and sound insertion precede projectile construction and the projectile's damage draw, the reverse of the physical-launch order. A tick starting before anticipation can transfer charge yet fail the old-time window. There is one event per qualifying visit regardless of how many period boundaries the update crosses, no initial lift cue, and no exactly-four counter. The supported state has a nonzero period.

## Held charge sounds: 24, 25, 27, 36, 59, 63, and 64

Evidence: original bytes `0x215bb3` through `0x215c68`. After the ownership gate, the branch compares the signed integer quotients of new time and old clock divided by 200. If they differ, it inserts one sound 55 at the creature position, volume one, pitch equal to stored charge multiplied by binary32 0.5 then plus binary32 one. No random draw, anticipation test, or direction test occurs. Otherwise nothing happens.

The charge read is the value present during dispatch, before the later charge-growth phase. A start at clock zero with a new time below 200 ms creates no sound. Reaching exactly 200 ms changes the quotient and creates the sound, but no separate weapon-lift sound. Crossing several 200 ms boundaries in one update still creates one sound.

## Magic launch and channel sounds: 37–46 and 108

Evidence: `0x217a21` through `0x218305`.

After the ownership gate, one sound 36 is inserted when the old clock is at most anticipation minus 200 ms and the integer quotients of new time and old clock divided by 200 differ. It uses creature position, volume one, and pitch one plus random-scaled 0.25 (`0x217ab2`), taking one draw only when its test succeeds. It precedes the anticipation-crossing mana deduction (no sound) and firing. A large update can produce both this channel event and a firing event.

The candidate firing time is anticipation for ordinary magic actions. For actions 45/46 it is anticipation plus (period × integer quotient of (new time − anticipation) divided by period), with period equal to integer strike duration divided by three, then raised to anticipation if below it. Firing requires old clock strictly below anticipation plus strike duration, old clock at most the candidate, new time strictly above it, and positive direction. A candidate equal to the new time does not fire; skipped candidates are not iterated. Each qualifying visit creates one projectile and one firing sound.

The firing sound has no pitch draw. It follows, in order: the single projectile damage draw, world-projectile insertion, spending and clearing of positive stored charge, removal of buff 9 when present for actions 37, 43, 45 or 46 only (`0x218128`–`0x21824c`; not for action 108), and outgoing-projectile insertion. It uses creature position and volume `0x3f666666` (approximately 0.9).

| Constructed projectile element | Firing sound | Fixed pitch |
| --- | --- | --- |
| Element byte 2 | 41 | 1.25 for actions 41, 42 or 44; otherwise 1 |
| Other element | 38 | 2 for actions 38, 39 or 40; otherwise 1 |

The projectile constructor (`0x059660`) initializes element (projectile record offset `0x64`) to zero. Class byte `0x140` equal to 3 changes it to 2 when specialization byte `0x141` is 1, otherwise to 1; other classes keep zero. Action 108 (projectile type 4) uses the same element and firing-audio selection; its physical-looking projectile does not select a swing cue. Evidence: `0x21810d` through `0x218128` and `0x21827d` through `0x2182f5`.

## Separate area-projectile route: 89 and 90

The action table sends these actions to `0x2151f9`. Its channel test requires old clock strictly below anticipation and differing 200 ms quotients of new time and old clock. A qualifying visit inserts sound 36 at creature position, volume one, pitch one plus random-scaled 0.25 (`0x215266`–`0x2152ad`). This draw and insertion precede the ownership gate, so a creature later rejected for release still contributes the channel sound.

After the ownership gate (failure continues at `0x211f35`), release requires old clock at most anticipation and new time strictly greater. A release first takes one pitch draw and inserts sound 38, volume one, pitch one plus random-scaled 0.25 (`0x215321`–`0x215368`), then constructs a type-3 projectile with element 1 for action 89 and 2 for action 90 (`0x2153dd`–`0x215416`) and inserts it into the world and outgoing lists. No direction test precedes release, and both actions use sound 38.

No damage-variance draw occurs in this route. A large update therefore takes at most the channel draw followed by the release draw. At exactly anticipation, the channel test is closed while the release test remains eligible for the next strictly later time.

## Input start and release are separate effects

The selected input loop in the controller update is `0x09b620` through `0x09bf4d`. It iterates input-bar slots and acts on the player creature pointer stored at offset `0x8006d0` of the controlling game object; it does not start actions for remote creatures.

Start selections use the byte table `0x09d57c` and address table `0x09d52c` (index = action − 3, actions 3–103; others use the default). The held-start route `0x09b989` (actions 8, 24, 25, 27, 59, 63, 64) assigns the action and resets clock and the `0x128` field when the action differs from the current one. The default route `0x09bd43`/`0x09bd83` (including 22, 26, 36, 38–44, 55, 89, 90 and 108) checks that available mana at `0x170` is not below the action cost, then assigns the action and resets clock and `0x128`. Action 50's start (`0x09baf4`) sets movement and a 50-keyed timer entry without assigning the action; action 96's start (`0x09bc76`) assigns action 96, resets the clock and sets `0x128` to 600. None of these sites constructs a sound or takes a random draw.

Held releases use byte table `0x09d610` and address table `0x09d5e4` (index = action − 8). Ranged releases at `0x09be73` through `0x09bec7` assign 24→23, 25→55, 27→26 and 36→37, each with clock zero; 59, 63 and 64 release at `0x09be00` through `0x09be57` to non-ranged actions. No release site constructs a sound or takes a draw.

Selections 3, 4, 23, 31, 33, 37, 45, 46, 65, 66, 94 and 95 enter the special-attack start helper `0x0456a0`; the selected action comes from selector `0x0452a0` and depends on equipment/class state. Neither issues a sound or random draw. When the selected action is 31, 33, 37, 46 or 95 and buff 9 is present, the helper sets the new clock to that action's anticipation instead of zero; action 45 is not in this list. This changes which later channel/fire tests are eligible (for example the magic channel test cannot succeed from that clock); it is not an immediate firing sound. Ordinary attack selection through `0x045410` and selector `0x045020` likewise emits no sound.

Direct sound calls exist in this interval but are not a common ranged tail: input actions 101 and 103 call `0x084350` at `0x09b780` and `0x09b7e5` with sounds 93 and 92 respectively, at the player position, after their action-effect call, with both binary32 arguments one and no random draw.

These findings close the named selection/helper routes, not every UI event, keyboard binding, script, item activation, or alternate control entry.

## Additional in-flight swing audio belongs to the projectile

A type-2 projectile (created by action 26) has another sound route outside action dispatch. In the projectile update, its clock at projectile offset `0x68` is first advanced by the update milliseconds (`0x220cc1`). Later, at `0x221f1e` through `0x221fcd`, when the signed integer quotients of that clock and of the clock minus the update, each divided by 200, differ, it inserts sound 15 at the projectile position (projectile offset `0x18`), volume one, pitch one plus random-scaled 0.25, taking one draw. There is no test of the creator's current action and no local/other gate at this site.

The site lies inside the projectile movement-subdivision loop; the back edge at `0x2223c8` through `0x2223e7` revisits it without advancing the clock. Because the quotient test depends only on the clock and update, it gives the same result on every visit in that update: a qualifying update emits one sound and one draw per visit that reaches the site, at successive positions, not one per projectile per update. Collision, survival, and subdivision count determine which visits occur and belong to a separate physics contract; a removed projectile need not reach the site.

This is in-flight projectile audio, not an extra sound at creature action start. A projectile can remain while its creator has moved on to another action.

## Limits

The absence of a common ranged lift layer is bounded to the dispatch, common clock/charge, and named input routes above. This document does not close sounds from damage handlers, behaviour callbacks, scripts, collision branches, environment, other entities/projectiles, or downstream replication/playback. Playback mixing, attenuation, voice limits, concurrent event ordering, and audible meaning were not verified at runtime. No native runtime session was performed.

## Re-certification (2026-11-21)

All dispatch, input and release tables were decoded from original image bytes; missing export ranges were decoded from original bytes; every sound id, constant, comparison direction and draw position above was rechecked. Corrections:

1. Equipment predicate `0x044230` is true for category byte `0xaa8` = 3 with subtype `0xaa9` in {5, 6, 7, 8, 10, 11, 15, 16, 17, 18}; the earlier "ranged weapon IDs" wording was an interpretation. Sound-17 pitch (1 plus random-scaled 0.1) and the bypassing actions 30–33/87/88/91 were added. Evidence `0x2123bd`–`0x212473`.
2. The sound-17 branch's ownership gate admits every creature when world byte `0xb4` is nonzero, unlike the ranged gate. Evidence `0x212357`–`0x212372`.
3. `0x084350` is the sound-play routine called directly with id, position and two binary32 ones; it was described only as a resolver. Sounds 92 (`manashield.wav`) and 93 (`bulwark.wav`) added. Evidence `0x09b760`–`0x09b780`, `0x09b7c5`–`0x09b7e5`, `00484350.c`.
4. `0x8006d0` is an offset in the controlling game object, not an absolute pointer address. Evidence `0x09b640`.
5. Physical launch: the charge transfer from `0x144` to `0x13b4` happens in the same update immediately before the ordinary launch test; "powered" (strictly positive released charge, or action 23) also gates buff-10 removal. Evidence `0x2170c7`–`0x21714c`, `0x2172d8`–`0x2172f2`, `0x21733f`.
6. Magic buff-9 removal applies only to actions 37, 43, 45 and 46; action 108's earlier flag is overwritten. Evidence `0x218128`–`0x218149`.
7. Held-branch ownership failure goes through `0x215ec0` before rejoining `0x211f41`, and the held pitch order is charge × 0.5 then + 1. Evidence `0x215bce`, `0x215c24`–`0x215c40`.
8. Projectile pulse: the quotient test is the same on every subdivision visit, so a qualifying update emits once per reached visit; position is projectile offset `0x18`; type 2 is assigned only for action 26. Evidence `0x221f33`–`0x221f7c`, `0x2173f7`.
9. Held-release mapping added: 24→23, 25→55, 27→26, 36→37 with clock zero; 59/63/64 releases lie at `0x09be00`–`0x09be57`, outside the earlier cited range, and also emit no sound.
10. Area route: type-3 projectile with element 1 for action 89 and 2 for action 90; ownership failure continues at `0x211f35`. Evidence `0x2152cd`, `0x21537e`, `0x2153dd`–`0x215416`.
11. Special-attack helper: its input set is 3, 4, 23, 31, 33, 37, 45, 46, 65, 66, 94, 95, and the buff-9 clock skip also covers 31, 33 and 95. Evidence tables `0x09d57c`/`0x09d52c`, `004456a0.c`.
12. Default input start requires mana `0x170` not below the action cost; action 50's start does not assign action 50; action 96 sets `0x128` to 600. Evidence `0x09bd43`–`0x09bdb5`, `0x09baf4`–`0x09bc1c`, `0x09bc76`–`0x09bd3e`.

Remaining uncertainties:

- Random-draw absence in callees (damage, buff, insertion and helper routines) rests on the original call list plus inspected listings; computed or virtual calls not resolved there are not excluded.
- The helper's buff-9 clock value is computed inline per action; its exact equality to `0x03caa0` for every listed action was not independently proven.
- The binary64-to-binary32 conversion in action 26 assumes the default rounding mode.
- Subdivision count, collision exits and projectile survival were not analysed.

No conflicts with other held specifications were examined or found; none were read for this assignment.
