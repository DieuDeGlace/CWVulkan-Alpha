# Alpha ranged audio boundaries

Status: FINAL / STABLE / HELD. This independently worded source-side contract concerns native sound-event boundaries for the listed ranged and charging actions. It does not certify complete combat, audio playback, networking, physics, or runtime parity. Earlier held contracts are unchanged.

## Supported image and evidence

The original image is `binary/Cube.exe`, relative to the assigned `disassembled alpha cube world` evidence directory: 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, PE machine `0x014c`, timestamp `0x51ea955e`, preferred base `0x00400000`, image size `0x003bc000`. All addresses below are RVAs. Native structure offsets describe this 32-bit image, not a required replacement layout.

Evidence consists of individually selected original instructions in `decompiled/client/assembly-functions/0060c510.asm`, `decompiled/client/assembly-functions/00488ee0.asm`, and `decompiled/client/assembly.txt`; original binary dispatch tables and missing instruction ranges; and the original individual function files `00484350.c`, `00465fb0.c`, `004864c0.c`, `00444230.c`, `00445020.c`, `004452a0.c`, `00445410.c`, `004456a0.c`, and `00459660.c`. The original binary was used where computed-dispatch destinations were absent from the assembly export. A textual search of exported call sites alone was not treated as proof of absence.

No reconstruction material, replacement engine, SDK, exporter, extractor, tests, probes, captures, logs, implementation history, inherited specification, or semantic audit was inspected for this assignment. No original file or earlier contract was modified.

## Established boundary of the common-cue claim

The reviewed native routes do not add a universal weapon-lift or swing cue to physical actions 22/23/26, volley actions 50/55/96, magic actions 37–46/89/90/108, or held actions 24/25/27/36/59/63/64. Their own branch sounds, the separate input-start routes described below, and independent projectile/physics events must be kept distinct.

The original action dispatcher at `0x211e04` selects these destinations using tables at `0x222ccc` and `0x222c84`:

| Actions | Native destination |
| --- | --- |
| 22, 23, 26 | `0x21707a` |
| 50, 55, 96 | `0x217634` |
| 37–46, 108 | `0x217a21` |
| 89, 90 | `0x2151f9` |
| 24, 25, 27, 36, 59, 63, 64 | `0x215bb3` |

The sound that can resemble an initial weapon lift, sound 17, is constructed at `0x21241c` through `0x212485` inside the different action branch beginning at `0x212357`. That site requires action clock zero, a successful equipment predicate at `0x044230`, and the surrounding action-specific exclusions. The same equipment predicate recognizes several ranged weapon IDs, including 6, 7, and 8; that fact does not move this sound outside its enclosing action branch. None of the actions in the table dispatches to that branch. Its anticipation sound-15 site at `0x212a2b` through `0x212a45` is likewise branch-specific.

The reviewed ranged destinations finish by rejoining the post-dispatch corridor around `0x211f35` through `0x211f56`; they do not fall through into the sound-17 or shared sound-15 sites above. The later common clock-zero cooldown initialization and clock increment at `0x218507` through `0x218541` contain no sound construction or sound-pitch random draw. Charging growth at `0x21a74d` through `0x21a918` also supplies no separate start/lift sound.

This is a bounded absence result for those action/start/common-update routes. It is not a claim that an entire frame containing a ranged creature is otherwise silent. Earlier interactions, damage, movement, rolling, other creatures, pre-existing projectiles, and downstream playback can produce independent sounds.

## Sound identities and numeric conventions

The native sound resolver at `0x084350`, recorded in `decompiled/client/functions/00484350.c`, supplies these identity mappings. Filenames identify the selected owner content; no audio assets are included here.

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

The ordinary sound record constructor at `0x065fb0` initializes pitch and volume to binary32 one. These fields lie at sound-record offsets `0x10` and `0x14`; the numeric sound is at `0x0c`. The outgoing sound insertion helper is `0x0864c0`. An outgoing sound-event insertion is not proof of when or how an audio device subsequently plays it.

For the randomized binary32 pitches below, the native order is integer random result converted to binary32, multiplied by the stated coefficient, divided by binary32 `32767.0`, then added to the stated base. Preserve those separate operations. The usual coefficients 0.25, 0.5, 1, 1.25, and 2 are exactly representable; the magic firing volume described as 0.9 has binary32 bits `0x3f666666`.

Action 26's launch pitch instead uses binary64 arithmetic: convert the random integer to binary64, multiply by binary64 bits `0x3fe051eb851eb852` (0.51), divide by binary64 bits `0x40dfffc000000000` (32767), add binary64 bits `0x3fe6666660000000` (the promoted binary32 value approximately 0.7), then convert the result to binary32. Replacing this with a wholly binary32 calculation is not established native arithmetic. Evidence: `0x21756c` through `0x217592` and original constants at `0x31e1e8`, `0x302a80`, and `0x31e1f0`.

The scheduling terminology below uses the signed 32-bit creature action clock at `0x6c` before its general increment, the update's signed 32-bit milliseconds, and their sum as the prospective new time. Anticipation and strike durations are the current native results from `0x03caa0` and `0x047310`. Creature action is the byte at `0x68`; stored charge is binary32 at `0x144`; released physical charge is binary32 at `0x13b4`; attack direction begins at `0x160`.

These facts cover ordinary nonnegative updates, finite values, valid state, and arithmetic without overflow. Signed integer quotient comparisons truncate toward zero. None of the periodic tests below is a catch-up loop over every skipped time boundary unless a separately described outer movement subdivision revisits the test.

## Ownership used by the ranged branches

For physical launch, arrow-volley, held-charge sound, and magic actions 37–46/108, the local creature is permitted. An other creature is permitted only when world byte `0xb4` is zero and creature control byte `0x60` is nonzero. “Local” means equality with the creature pointer at world `0xb8`; “other” means inequality. Failure at that branch's ownership gate skips its sound tests and their pitch draws.

Actions 89/90 differ: their pre-anticipation channel sound precedes this ownership restriction. Their release sound is behind it. The distinction is confirmed in the original binary range `0x2151f9` through `0x21548f` and must not be inferred from their visual resemblance to other magic projectiles.

## Physical launch sounds: 22, 23, and 26

Evidence: `0x21707a` through `0x217634`, especially `0x2174d9` through `0x217624`.

A launch qualifies when the old action clock is at most anticipation and the new time is strictly greater than anticipation. Landing exactly on anticipation does not qualify. Action 26 alone can additionally launch when released charge is positive and an update crosses anticipation plus a fixed 300 ms with that same inclusive-old, strict-new convention. The ordinary anticipation crossing has priority; one update spanning both thresholds produces one launch through this branch.

Every successful launch produces one branch sound after the projectile's damage random draw, world-projectile insertion, and outgoing-projectile insertion. That sound consumes one additional pitch random draw. An update that qualifies for neither launch performs neither of those directly coded random draws and produces no launch sound. The branch has no additional zero-direction rejection before normalization, so a positive-direction guard must not be invented solely to gate this audio.

| Launch state | Sound | Pitch | Volume |
| --- | --- | --- | --- |
| Action 22, released charge not positive | 15 | 2 plus random-scaled 0.5 | 1 |
| Action 22, released charge positive | 22 | 1 plus random-scaled 0.5 | 1 |
| Action 23 | 22, including zero charge | 1 plus random-scaled 0.5 | 1 |
| Action 26 | 15 | Binary64 calculation described above | 1 |

Sound position is the creature position used for that launch. The sound-15 event here is an actual launch effect, not an extra common weapon-lift layer. A powered shot may remove buff 10 before the projectile is queued; this does not add an additional pitch draw or branch sound at that removal site.

## Arrow-volley sounds: 50, 55, and 96

Evidence: `0x217634` through `0x217a21`, particularly `0x217758` through `0x217859` and `0x2179ab` through `0x217a11`.

The period is the integer strike duration divided by four. A volley sound requires the old clock to be at least anticipation, strictly below anticipation plus strike duration, and the integer quotients of new and old full action clocks divided by that period to differ. The attack direction must have positive squared length. Those quotient operands are not first reduced by anticipation.

Each qualifying visit inserts one sound 22 at the creature's position, volume one, with pitch one plus random-scaled 0.5. Its pitch draw and outgoing sound insertion precede projectile construction and the later projectile damage random draw. They are the reverse order of the corresponding two draws for physical launches. The anticipation-crossing charge transfer does not itself force a sound: a tick starting before anticipation can transfer charge and still fail this old-time window test.

Within this action branch there is one event per qualifying visit, regardless of how many period boundaries the update crosses. There is no additional initial lift cue, no sound on a rejected direction/window, and no fixed universal exactly-four-sounds counter established by this code. The strike-period division requires a nonzero period in the supported ordinary state.

## Held charge sounds: 24, 25, 27, 36, 59, 63, and 64

The original dispatch destination `0x215bb3` compares the signed integer quotients of new and old action clocks divided by 200. Differing quotients insert one sound 55 at the creature's position. Pitch is one plus binary32 charge multiplied by 0.5; volume is one. This sound consumes no random draw. No anticipation or positive-direction test is added by this branch. Evidence: original binary `0x215bb3` through `0x215c68`.

The charge read for pitch is the value present during action dispatch, before the update's later charge-growth phase. A start at clock zero with a new time below 200 ms does not automatically create this sound. Reaching 200 ms changes the quotient and can produce it, but does not create a separate weapon-lift sound. Crossing several 200 ms boundaries in one update still creates one sound through this branch.

## Magic launch and channel sounds: 37–46 and 108

Evidence: `0x217a21` through `0x218305`.

After the ownership gate, one sound 36 is inserted when the old clock is at most anticipation minus 200 ms and the 200 ms integer quotients differ across the update. It uses creature position, volume one, and pitch one plus random-scaled 0.25. It consumes one pitch draw only when its own test succeeds. This test occurs before projectile scheduling and before the successful projectile's damage draw. A large update can produce both this channel event and a firing event.

Ordinary magic actions use anticipation as their candidate firing time. Actions 45/46 use the last boundary at or before the prospective new time relative to anticipation, with period equal to integer strike duration divided by three and the candidate floored upward to anticipation. Firing requires old clock strictly below anticipation plus strike duration, old clock at most that candidate, new time strictly above it, and positive direction squared length. A candidate exactly equal to the new time does not fire; skipped earlier candidates are not iterated. Each qualifying visit creates one projectile and one firing sound.

The firing sound itself has no pitch random draw. It follows the projectile damage draw, world insertion, any positive-charge spending and clearing, powered-buff-9 removal if applicable, and outgoing projectile insertion. It uses creature position and volume `0x3f666666` (approximately 0.9).

| Constructed projectile element | Firing sound | Fixed pitch |
| --- | --- | --- |
| Element byte 2 | 41 | 1.25 for actions 44, 41, or 42; otherwise 1 |
| Other element | 38 | 2 for actions 38, 39, or 40; otherwise 1 |

The constructor initializes element to zero; class byte `0x140` equal to 3 changes the constructed element to 2 when specialization byte `0x141` is 1, otherwise to 1. Other classes retain the default in this branch. This includes action 108's use of the same firing-audio selection; the action's physical-looking projectile does not by itself select a swing cue. Evidence: `decompiled/client/functions/00459660.c`, `0x21810d` through `0x218128`, and `0x21827d` through `0x2182f5`.

## Separate area-projectile route: 89 and 90

The original action table sends these actions to `0x2151f9`, not to the preceding magic-launch branch. Its wind-up sound test uses old clock strictly below anticipation, together with a changed 200 ms quotient. A qualifying visit inserts sound 36 at creature position, volume one, with pitch one plus random-scaled 0.25. This pitch draw and sound insertion occur before any local/other ownership rejection in this branch. A remote creature rejected for release emission can therefore still contribute this channel sound.

The later release test is ownership-restricted as described above and uses old clock at most anticipation, new time strictly greater than anticipation. A successful release first takes one pitch draw and inserts sound 38, volume one, pitch one plus random-scaled 0.25. Projectile construction and insertion follow that sound. No positive-direction rejection precedes this release sound in the reviewed route. Both action numbers select sound 38 here; action 90's different projectile element does not change this branch's already-created release sound to sound 41.

The directly coded random draws in this route are the optional channel pitch draw, followed by the release pitch draw when release qualifies. There is no projectile damage-variance draw in the reviewed construction after this release sound. Consequently, a large update can take those two pitch draws in that order. At exactly anticipation, the channel test is already closed while the inclusive-old release test remains eligible for the next strictly later time. Evidence: original binary `0x2151f9` through `0x21548f`, including sound insertion sites `0x2152ad` and `0x215368`.

## Input start and release are separate effects

The selected input-start interval in the original controller update is `0x09b620` through `0x09bf4d`. It acts on the controller's player pointer at `0x8006d0`; it is not a loop emitting starts for remote creatures.

The held-start route at `0x09b989`, the ordinary selected-action assignment at `0x09bd83`, and the ranged held-release assignments in `0x09be73` through `0x09bee3` select actions/reset clocks without constructing a sound event or taking a pitch draw there. The held route covers 24/25/27/59/63/64. The ordinary route covers the requested other action numbers whose input-table destination is the default assignment, including 22/26/36/38–44/55/89/90/108. Action 50's movement-start branch and action 96's roll-start branch likewise contain no sound emission in that selection interval.

Input selections 23/37/45/46 enter the special-attack start helper at `0x0456a0`; the selected action depends on equipment/class state. That helper and its selector `0x0452a0` do not issue an initial sound or pitch draw. They can change the clock: for selected actions 37 and 46, the helper's existing-buff-9 path can place the new clock directly at anticipation. This changes which later native channel/fire tests will be eligible; it is not an immediate input-start firing sound. This particular clock-skip list does not include action 45. Ordinary attack selection through `0x045410` and selector `0x045020` similarly does not emit a common lift/swing cue.

There are direct sound-play calls within the input bar's larger selection interval, but they are not a common tail shared by these ranged starts. The calls at `0x09b780` and `0x09b7e5` belong to input actions 101 and 103 and select sounds 93 and 92 respectively, after their action-effect call. Their fixed pitch and volume are one; there is no pitch random draw at those sites. The requested ranged cases dispatch elsewhere. Original input tables are at `0x09d52c`/`0x09d57c`; held-release tables are at `0x09d5e4`/`0x09d610`.

These findings close the named selection/helper routes, not every UI event, keyboard binding, script, item activation, or alternate control entry in the original program.

## Additional in-flight swing audio belongs to the projectile

A type-2 projectile created by action 26 has another native sound route outside action dispatch. Its later update can insert sound 15 at the projectile's current position when the signed integer quotients of its current clock and that clock minus this update's duration, each divided by 200, differ. The current projectile clock at offset `0x68` has already been incremented in the earlier projectile update at `0x220cc1`. The pulse sound uses volume one and pitch one plus random-scaled 0.25, consuming one pitch draw on each qualifying visit. There is no comparison against the creator's current action or a local/other creature gate at this sound site. Evidence: `0x221f1e` through `0x221fcd`.

Crucially, this site is inside the native projectile movement-subdivision loop. The back edge at `0x2223c8` through `0x2223e7` revisits the pulse test without advancing the projectile clock again. A qualifying update can therefore emit more than one such swing sound and consume more than one pitch draw for one projectile, at successive visited positions. The reviewed code does not justify deduplicating those events to one per projectile per update. Collision, survival, and subdivision eligibility determine which visits occur and require a separate physics contract; a rejected/removed projectile need not reach this site at all.

This is in-flight projectile audio, rather than an extra sound at creature action start. A projectile can remain while its creator has moved on to another action, so attributing every sound in that frame to the creator's current action would be incorrect.

## Limits and handoff

The established absence of a common ranged lift layer is bounded to the dispatch, common clock/charge, and named input-start routes above. This document does not close every sound arising through damage handlers, behavior callbacks, scripts, collision branches, environment, other active entities/projectiles, or downstream replication/playback. Independent rolling and movement sounds in the outer update are not evidence for an attack-start cue. Playback mixing, attenuation, voice limits, concurrent event ordering, and the audible meaning of the owner's waveform content were not verified at runtime.

No native runtime session or replacement-engine verification was performed. The complete final document was read back before handoff. FINAL / STABLE / HELD; this source assignment stops editing here.
