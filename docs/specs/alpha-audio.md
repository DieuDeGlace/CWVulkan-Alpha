# Alpha audio: sound player, listener, ambience, interface sounds, music

Behavioural specification of how the Cube World **Alpha** client (`Cube.exe`) makes sound:
the playback engine, how a sound event (position, id, pitch, volume) becomes audible, the
listener, the ambient bird/cricket/owl calls, every interface-sound call site, the (unused)
music system, and which ids of the 0–100 table are reached at all. Written from the Alpha
decompile and checked instruction by instruction in the assembly where numbers matter.
Addresses are citations only (client unless marked "server").

Related, not repeated here: the id → file table and the play routine's switch
(`alpha-creatures-behaviour.md` §14), the 24-byte sound record of world-update section 4
(same file §6), hit-resolution sounds (same file §5 steps 4–14), footsteps
(`alpha-creatures-models.md` §7.3), landing sound 32, splash 31 and dodge sound 26
(`alpha-physics-camera.md` §2 step 15, §4.2, §5), projectile and staff sounds
(`alpha-projectiles.md` §2, `alpha-particles.md` §4–§5), interface sounds by window
(`alpha-ui-screens.md` §8.1.11, §10; `alpha-skills.md` §7; `alpha-character-screens.md`;
`alpha-crafting.md`; `alpha-item-services.md`; `alpha-talk-vendors.md`), world-item and
server sounds (`alpha-world-items.md`, `alpha-server-simulation.md`), options
`soundVolume` / `musicVolume` (`alpha-ui-screens.md` §9).

Status: the engine, the volume formula, the listener, the ambience and the interface call
sites are complete. The world-step melee swing sounds (§6.2) are new. Items marked
*believed* rest on a single reading; the open points (§8) list game checks.

---

## 0. Conventions

* G = the game object; W = G + 0x2e4 the world (as in `alpha-rendering.md`).
* **L** = the listener position G+0x140 (§2). **P** = the local player's position (local
  player object G+0x8006d0, its position at +0x10).
* *u* = one C-runtime `rand()` / 32767.0, so 0 ≤ *u* ≤ 1.
* "Play(id, position, volume *v*, pitch *p*)" means the play routine 0x484350 (§1.2). Unless
  a value is given, *v* = 1 and *p* = 1.
* World positions are 64-bit fixed point (1/65536 block). Section-4 records carry float
  block positions; they are converted to fixed point (×65536, truncation, 0x42c460) before
  playing.
* Times are milliseconds; time of day *t* is G+0x800440 in [0, 86 400 000).

---

## 1. The sound player

### 1.1 Engine

* The audio API is **XAudio2 2.7** (COM class 5a508685-a254-4fba-9b82-9a24b00306af, created
  with `CoCreateInstance` after `CoInitializeEx`; 0x623530). Initialisation: default flags,
  default processor; one **mastering voice** with default channel count, default sample rate,
  default device (0x623530). No submix voices, no effects, no 3D-audio (X3DAudio) library.
  If initialisation fails, a message box says XAudio2 could not be initialised and the game
  continues without sound (0x4c8ae0).
* The engine object (class `XAudio2Engine`, constructor 0x622da0) is reached through
  G+0x800714. Its virtual methods, in table order: reap finished voices (0x623410), play
  music (0x623980), stream music (0x6240b0), stop music (0x623e60), "music is playing"
  (0x623600), set the two music volumes (0x623a00), **play a sound** (0x623610),
  **get a sound by name** (0x623a60), destructor.
* **Loading.** Sounds load lazily, the first time a name is played, and are cached by name
  for the rest of the session (name → handle map; handle → sound map). The WAV bytes come
  from `data2.db` (key = the file name exactly, case-sensitive); only if `data2.db` could not
  be opened is a loose file of that name read from the working directory (*believed*, the
  fallback branch of 0x623a60). The RIFF file is walked chunk by chunk: the `fmt ` chunk is
  copied as the voice format (PCM 16-bit and IEEE-float 32-bit, 44.1 or 48 kHz, mono or
  stereo all occur), the `data` chunk becomes one buffer marked end-of-stream, no loop. A
  name that is not in the store yields handle 0 and **plays nothing**; it is looked up again
  on every later play.
* **Playing** (0x623610; arguments handle, volume, pan, pitch, an unused 0):
  1. If volume ≤ 0: nothing.
  2. Look the handle up; no sound → nothing.
  3. Create a **new source voice** in the sound's own format, maximum frequency ratio 2.0,
     sending to the mastering voice.
  4. Set the voice volume to *volume* (linear amplitude), submit the buffer, start, set the
     volume again (same value), set the **frequency ratio to the pitch**.
  5. Append the voice to the playing list.
  The **pan argument is ignored**: no output matrix is ever set, so every sound plays with
  XAudio2's default channel mapping — stereo files exactly as authored, mono files the same
  in both front speakers (*believed*: at XAudio2's default mono-to-stereo level).
* **Pitch** is a playback-rate ratio: pitch 1.2 plays 20 % faster and higher. XAudio2 clamps
  the ratio to [1/1024, 2.0] (the maximum given at voice creation), so any pitch above 2 plays
  as **2.0** (e.g. the projectile swing of `alpha-projectiles.md` §2, pitch 2 + 0.5*u*, is
  always exactly 2.0).
* **Voices.** There is **no voice limit and no stealing**: every play makes a new voice, the
  same sound overlaps itself freely, and nothing is deduplicated. Once per frame (0x48b35d)
  the reap method walks the list and destroys every voice whose queued-buffer count is 0
  (finished). Sounds therefore always play to their end; nothing stops them early (not even
  leaving the world).

### 1.2 The play routine (0x484350)

Arguments: id, position (3 × 64-bit fixed point), volume *v*, pitch *p*. In order:

1. **Distance**: Δ = position − **L** per axis (64-bit), each converted to float and ×1/65536
   (blocks); *d*² = Δx² + Δy² + Δz² in float. If *d*² > **10 000** (*d* > 100 blocks):
   return — nothing is loaded or played.
2. *d* = square root of *d*² (computed in double, then used as float).
3. **Falloff**: *a* = (1 − *d* / 100) · *v* — linear, full at the listener, silent at 100
   blocks. There is no minimum radius and no curve.
4. Look up the sound by its file name (the id switch of `alpha-creatures-behaviour.md` §14;
   ids outside 0–100 → no name, and the routine still goes on with handle 0, which plays
   nothing).
5. A pan value is computed from the position and the matrix at G+0x22c … +0x268 (the clip-x
   over clip-w of the absolute world position, × −0.1, clamped to [−1, 1]) and passed on —
   but the engine ignores it (§1.1). An implementation should not reproduce it.
6. Play with **final volume = `soundVolume` · *a* · 0.01** (`soundVolume` is the integer
   option 0…100 at G+0x184, read at every play) and pitch *p*.

So final volume = (soundVolume / 100) · (1 − *d*/100) · *v*. Volumes above 1 would amplify
(XAudio2 allows it); no caller passes *v* > 1.

**Helper 0x484320**: Play(id, **L**, 1, 1) — an interface sound at the listener, which always
plays at full volume (*d* = 0).

### 1.3 Where play requests come from

* **Direct calls** from client code (interface, ambience): §3, §4.
* **Sound records** (world-update section 4, 24 bytes: position as 3 floats in blocks, id,
  pitch, volume; a new record defaults to pitch 1, volume 1, 0x465fb0). The frame update
  walks the update's record list (0x4955f0 … 0x495650) and calls Play(record id, record
  position, record volume, record pitch) for each. Records from the server and records the
  client's own world step queued (footsteps, swings, splashes, landing) are treated
  **identically**: same falloff, same listener, no ownership test. Whether an offline game
  routes its step records through the same loop is `alpha-world-items.md` W-4.
* Right after playing a record with id **81** (`Explosion.wav`) or **82** (`punch2.wav`), the
  loop sets the **camera-shake amplitude G+0x1e8 := 1** — whatever the distance, even beyond
  100 blocks (0x495635 … 0x495642). This answers part of `alpha-physics-camera.md` Q-11.
* The server (`Server.exe`) has no audio code; it only creates records (it shares the world
  step with the client, see §6).

---

## 2. The listener

* Position: **G+0x140**, the **camera** eye position (`alpha-physics-camera.md` camera
  fields), not the player. In third person a sound "at the player" is therefore attenuated by
  the camera distance: a camera 8 blocks behind the player plays player sounds at 0.92.
* Orientation: **none**. No direction, no up vector and no velocity are used; there is no
  stereo placement, no Doppler, no occlusion, no underwater filter, no reverb.

---

## 3. Ambient sounds (ids 94–100)

Played from the frame routine 0x4ac260 (0x4ac761 … 0x4ad41d), once per rendered frame, on the
render thread's C-runtime `rand()` (the same generator the particle code uses; not a world
generator). The loop runs early in the frame, before the map-mode branch, so it also runs
while the world map is open (*believed* from the order of `alpha-rendering.md` §1.3).

**Scope.** For each zone object in the draw list whose camera distance (zone field +0x6c,
blocks, *believed*) is below **4096**, every record in its static-record list (zone object
+0x240: plant records of `alpha-worldgen-vegetation.md` §1.1 and the tree records 60/61 of
`alpha-worldgen-trees.md` §5) is tested. The record's kind is its first field; its position
(x, y, z) the fixed-point position fields.

**Tick test** (per record, per frame). With period *P* (ms):
* *x*ᵦ = trunc(x / 65536), *y*ᵦ = trunc((y + 333·65536) / 65536) (64-bit division truncating
  toward zero; the 333-block offset decorrelates x and y);
* φ = (*x*ᵦ rem *P*) + (*y*ᵦ rem *P*) (C remainders, negative for negative coordinates);
* T = world clock W+0x8000bc (= G+0x8003a0), dt = G+0x8006e8 (the frame's elapsed ms);
* the record **ticks** this frame if trunc((φ + T)/*P*) ≠ trunc((φ + T + dt)/*P*).
On a tick, one `rand()` decides (chance below); if it passes, a second `rand()` gives the
pitch; owls take a third for the file. The sound plays at the record's position through
Play (§1.2), so it is attenuated by distance like any other sound.

| Time of day *t* | Record kind | *P* | Chance per tick | Id | Volume | Pitch |
|---|---|---:|---|---|---:|---|
| 06:00 < *t* < 21:00 | 61 (tree crown top) | 1000 | rand mod 10 = 0 | **94** `bird1.wav` | 0.1 | 0.9 + 0.2*u* |
| 06:00 < *t* < 21:00 | 62 | 1000 | 1/10 | **95** `bird2.wav` | 0.1 | 0.9 + 0.2*u* |
| 06:00 < *t* < 21:00 | 63 | 1000 | 1/10 | **96** `bird3.wav` | 0.1 | 0.9 + 0.2*u* |
| *t* ≤ 06:00 or *t* ≥ 21:00 | 61 | 1000 | 1/10 | **99 + (rand & 1)** `owl1`/`owl2` | 0.1 | 0.5 + 0.2*u* |
| 12:00 < *t* < 24:00 | 0, 1 (flowers) | 5000 | rand mod 70 = 0 | **98** `cricket2.wav` | 0.05 | 0.9 + 0.2*u* |
| 12:00 < *t* < 24:00 | 2, 3, 4 (grasses) | 5000 | 1/70 | **97** `cricket1.wav` | 0.05 | 0.9 + 0.2*u* |

Boundaries are exclusive as written (06:00 = 21 600 000, 21:00 = 75 600 000, 12:00 =
43 200 000). The owl's file bit is the sign-corrected low bit of the third draw (always
0 or 1 here). Kinds 62 and 63 are tested but **no generator makes them** (only 60 and 61
exist, `alpha-worldgen-trees.md` §5), and `bird3.wav` is missing from `data2.db` anyway, so
ids 95 and 96 are never heard (*believed*). Other kinds (60, plants 5–58, statics) make no
sound. Nothing depends on biome or zone beyond which records exist; there is no wind,
water, rain or cave ambience.

Expected rates: one bird call per crown every 10 s on average by day; one owl call per crown
every 10 s at night; one chirp per grass or flower record every 350 s in the afternoon and
evening — audible as a chorus because meadows hold many such records.

---

## 4. Interface and item sounds

All interface sounds go through 0x484350 (or its helper 0x484320). "At **L**" means full
volume; "at **P**" means attenuated by the camera–player distance (§2). Volume 1 and pitch 1
unless given. The table lists every direct interface call site of 0x484350 and 0x484320 in
the client (the remaining direct callers are 0x484320 itself, the record loop of §1.3 and
the ambience of §3); "spec" names where the trigger is described.

| Id | File | Call sites | Where | Trigger (spec) |
|---:|---|---|---|---|
| 83 | menu-open2 | 0x48b33d | L | the "a window holds the input" state turns on (below) |
| 84 | menu-close2 | 0x48b33d | L | that state turns off (below) |
| 85 | menu-select | 0x42b810, 0x42b860, 0x42b8b0, 0x42b910, 0x42b950, 0x42b990, 0x42b9e0, 0x42ba20, 0x42ba60, 0x42bab0 | L | each options arrow (`alpha-character-screens.md`, arrow actions) |
| 85 | menu-select | 0x47b899, 0x47b9d8 | L | a menu row / specialization chosen; a skill raised (`alpha-skills.md` §4, `alpha-ui-screens.md` §3.2) |
| 85 | menu-select | 0x497a97, 0x497ae0 | L | quick-item wheel step left / right (`alpha-ui-screens.md` §5.7) |
| 85 | menu-select | 0x4c6656 (via 0x484320) | L | a row selected in a list widget (0x4c6610; *believed* the crafting/vendor rows of `alpha-crafting.md`) |
| 86 | menu-tab | 0x4c5b91 (via 0x484320) | L | a tab chosen in a tabbed item widget (0x4c5a60; `alpha-ui-screens.md` §8.1) |
| 87 / 88 | menu-grab-item / menu-drop-item | 0x47c548, 0x47c743 (via 0x484320) | L | after a slot/hand swap: 87 if the hand now holds an item, else 88 |
| 88 | menu-drop-item | 0x47ba7e | L | a skill lowered (`alpha-skills.md` §4) |
| 88 | menu-drop-item | 0x47d2aa, 0x47da26, and via 0x484320 at 0x47d1f7, 0x47d324, 0x47d39e, 0x47d418, 0x47d4b8, 0x47d53c, 0x47d5b6, 0x47d630, 0x47d6aa, 0x47d712 | L | an equipment box emptied back into the bag (one site per box; `alpha-ui-screens.md` §8.1) |
| 89 | craft | 0x470c9a | L | a craft completes (`alpha-crafting.md` step 4) |
| 90 | craft-proc | 0x470d11 | L | the craft rolled a bonus (`alpha-crafting.md`) |
| 57 | drop | 0x47dc2c, 0x47dd6b | P | buy / buy back (`alpha-talk-vendors.md`) |
| 58 | drop-coin | 0x47ce74, 0x47d89a | P, **volume 0.75** | sell from bag / from an equipment box |
| 57, 58, 59 | drop, drop-coin, drop-item | records | item position | drop and landing (`alpha-world-items.md` §3, §5; `alpha-server-simulation.md`) |
| 45 | pickup | 0x48ce0c | P | an item arrives in the bag (`alpha-ui-screens.md` §8.1.9) |
| 45 | pickup | records (pitch 1 + 0.1*u*) | creature | world pick-up (`alpha-world-items.md` §4) |
| 46 | disenchant2 | 0x47c801, 0x47cae8, 0x47ccaf | P | disenchant / adapt / reward into bag (`alpha-item-services.md`) |
| 46 | disenchant2 | 0x48d31f | L | *believed* the mission-shown sound of `alpha-missions.md` (that spec says "at the player"; this call uses **L**) |
| 47 | upgrade2 | 0x47d7bc | L | recipe learnt from a formula (`alpha-ui-screens.md` §8.1) |
| 47 | upgrade2 | records | creature | mana/buff gains (`alpha-projectiles.md`, `alpha-pets-bombs.md`) |
| 48 | swirl | records | creature | action 11 (§6.2) |
| 49 | human-voice01 | 0x470a9d, 0x47dbd6, 0x47dd1a, 0x4976e2 | P | refusal: can't carry / not allowed |
| 49 | human-voice01 | 0x47d7e3 | L | recipe already known |
| 49 | human-voice01 | records | P | interaction refusal (`alpha-interaction.md` §5) |
| 50 | human-voice02 | 0x47bf60, 0x47c63f, 0x47cffc, 0x49210e, 0x497e2f | L | refusal: level / class / tab (`alpha-ui-screens.md` §8.1, §5.7) |
| 50 | human-voice02 | 0x4e60d2 | P, pitch **1 + 0.5*u*** | speech-bubble typing, every third character (`alpha-talk-vendors.md`) |
| 28 | cry | — | — | **never played** |
| 29 | levelup2 | 0x493e8f | P | level up (`alpha-ui-screens.md` "LEVEL UP!") |
| 29 | levelup2 | records (server) | player | inn rest (`alpha-talk-vendors.md`) |
| 30 | missioncomplete | 0x48f9be, 0x4907d7 | L | mana cube received; quest banner (`alpha-missions.md`) |
| 54 | lever | 0x497fda, 0x49802f | L | cycling the options of a speech bubble with keys G+0xd / G+0xe — **dead code** (the bubble pointer G+0x800940 is never set, `alpha-talk-vendors.md`) |

**Window open/close rule** (0x48b2d0 … 0x48b346), once per frame: *open* = (G's virtual
"a window holds the input" query, or the flag G+0x800a40). Compared with last frame's value
(a static byte): off → on plays 83, on → off plays 84; then the static := *open*. Opening
one window while another is already open plays nothing; switching between windows in one
frame plays nothing.

Corrections to `alpha-ui-screens.md` §10: 0x39 (57) is the **buy** sound, 0x3a (58) the
**sell** sound at volume 0.75; 0x3b (59) is only the world-item landing sound.

---

## 5. Music and looping sounds

* A complete **music streamer exists but is never used.** Two `Music` objects (each with
  three 640 KB PCM buffers) are created at start-up (0x623530). Each streams an **Ogg
  Vorbis** file opened with `fopen` (decoder 0x6271b0 / read 0x626de0, 16-bit signed little
  endian), refills a finished buffer every frame (0x6240b0) and can loop by seeking back to
  the start (0x626740). The play-music method (0x623980) has **no caller**, the executable
  contains no music file names, and `data2.db` holds only the 93 `.wav` sound effects.
  Alpha therefore plays **no music**.
* The mixer still runs every frame (0x48c3d3 … 0x48c432): music channel A volume =
  `musicVolume` × 0.01; channel B volume relaxes toward `musicVolume` × 0.01 at rate 0.001
  per ms while a window holds the input (§4) and toward 0 at 0.00005 per ms otherwise
  ("relax at rate *r* per ms" as in `alpha-creatures-models.md` §7: value ← target +
  (value − target)·(1 − *r*)^dt). The options screen sets both at once
  (`alpha-ui-screens.md` §9). Inaudible in Alpha; a mod could use the two channels as
  "world" and "menu" music.
* **No looping sound effects**: every buffer is submitted once, without a loop count. There
  is no wind, water, rain, fire or lava loop; the only recurring ambience is §3.

---

## 6. Coverage of ids 0–100

### 6.1 Already specified elsewhere

1–11 and 17 (weapon impacts), 24, 39, 42, 60–69, 91 (`alpha-creatures-behaviour.md` §5);
1, 2 (`alpha-block-edits.md`); 15, 19 (unreachable), 20, 22, 36, 38, 41, 47, 55
(`alpha-projectiles.md`); 36, 81 (`alpha-particles.md`); 23, 26, 31, 32 (`alpha-physics-camera.md`);
32–35 (`alpha-creatures-models.md` §7.3); 27, 29, 57, 58, 59 (`alpha-server-simulation.md`,
`alpha-world-items.md`); 44, 47 (`alpha-pets-bombs.md`); 49, 51, 54 (`alpha-interaction.md`);
92, 93 (`alpha-creatures-behaviour.md` §16.6); interface ids §4; ambience §3.

### 6.2 Newly found: world-step action sounds (client 0x60c510 / server 0x5322d0, shared)

Records at the creature's position (+0x10), default volume 1, for each creature the step
simulates (the step's usual filter, 0x612357 … 0x61237f). "Crossing *X*" means this step's
interval contains *X*: action time ≤ *X* < action time + dt. P1, P2 are the action's phase
durations (`alpha-creatures-behaviour.md` §3.3, 0x43caa0 / 0x447310). "Two-hander" = weapon
type 3 with subtype 5, 6, 7, 8, 10, 11, 15, 16, 17 or 18 (0x444230).

| Id | File | When | Pitch |
|---:|---|---|---|
| 17 | swing-slow | action time = 0 (start) while holding a two-hander, action ≠ 91 and not 30–33, 87, 88 (0x6123f3 … 0x612485) | 1 + 0.1*u* |
| 16 | shield-swing | crossing P1, action **10** (0x612782) | 0.9 + 0.2*u* |
| 48 | swirl | crossing P1, action **11** (0x61298f) | 0.9 + 0.2*u* |
| 14 | smash-jump | crossing P1, action **54** (0x6129bd) | 0.9 + 0.2*u* |
| 15 | swing | crossing P1, any other action except 91 (0x612a3b) | (0.9 + 0.2*u*)·1.5, then ×0.5 if two-hander or action 68, 69 or 93 |
| 12 | slam-ground | crossing P1 + P2/2, actions 57, 58, 60, 74 (0x612b24) | 1 + 0.25*u* |
| 82 | punch2 | crossing P1 + P2/2, action 74 (0x6132af) | 0.5 + 0.1*u* |
| 15 | swing | multi-hit actions (72, 86, 30–33, 11, 91; not 5): each time a new hit interval starts inside P2 (0x612f54 … 0x612fc3) | 1 + 0.25*u* |
| 82 | punch2 | the same moments, action 91 only (0x613016) | 0.5 + 0.1*u* |
| 40 | magic02 | crossing P1, action **94** (0x616b9d) | 1 + 0.1*u* |
| 5 / 16 | hit1 / shield-swing | crossing P1, action **104**: 5 if the step's local flag at frame −0x2b29 is set, else 16 (0x616c35) | 1 + 0.25*u* |
| 37 | channel-hit | on the 200-ms channel tick of `alpha-particles.md` §4 when that same flag is set (36 otherwise) (0x616d02) | 1 + 0.25*u* |

The hit-interval length of the multi-hit rows and the meaning of the local flag are open
(§8, AU-4).

### 6.3 Hurt voices for every creature type (resolves `alpha-creatures-behaviour.md` F-8b)

The critical-hit voice of `alpha-creatures-behaviour.md` §5 step 14 (pitch 1 + 0.1*u*,
volume 1) uses a 120-entry table (0x598296, types 0–119):

| Types | Id | Types | Id |
|---|---:|---|---:|
| 0 | 60 | 11 | 70 |
| 1 | 61 | 12 | 71 |
| 2, 43 | 62 | 15 | 72 |
| 3, 45 | 63 | 16 | 73 |
| 4 | 64 | 13 | 74 |
| 5 | 65 | 14 | 75 |
| 7 | 66 | 119 | 76 |
| 8 | 67 | 46, 108, 109, 114, 115 | 77 |
| 9 | 68 | 48, 51, 87 | 78 |
| 10 | 69 | 37, 38, 39, 40 | 79 |
| | | 96 | 80 |

Every other type (6 and all those not listed) and every type ≥ 120 has **no voice**.

### 6.4 Never played (*believed*)

No client call site, no client or server record writer and no table entry was found for:

| Id | File |
|---:|---|
| 0 | hit.wav |
| 13 | smash-hit2.wav |
| 18 | swing-slow2.wav |
| 21 | punch2.wav (82 is used instead) |
| 25 | shield-slam.wav |
| 28 | cry.wav |
| 43 | lich-scream.wav |
| 52 | spike-trap.wav |
| 53 | fire-trap.wav |
| 56 | magic02.wav (40 is used instead) |
| 95, 96 | bird2.wav, bird3.wav (record kinds 62/63 never generated; `bird3.wav` absent from `data2.db`) |

Ids whose file is missing or misnamed in `data2.db` and so play nothing even when requested:
**38** (`fireball.wav` is stored as `Fireball.wav`; the lookup is an exact SQL `key = ?`
match, so the staff fireball sound of `alpha-projectiles.md` is silent, *believed*) and
**96** (`bird3.wav`).

---

## 7. Worked examples

**Footstep 30 blocks away.** A creature 30 blocks from the camera steps on ordinary ground:
a section-4 record id 32, volume 1, pitch 1 + 0.2*u* (`alpha-creatures-models.md` §7.3),
say *u* = 0.5 → pitch 1.1. *d*² = 900 ≤ 10 000; *d* = 30; *a* = (1 − 30/100)·1 = 0.7;
with `soundVolume` 100 the voice volume is 100 · 0.7 · 0.01 = **0.70**. Pan: **none** —
`step2.wav` (stereo, 32-bit float, 0.28 s) plays as authored in both speakers, at
frequency ratio 1.1 (0.25 s long). With `soundVolume` 60 the volume is 0.42. The same
creature landing hard (id 32, volume 0.5) plays at 0.35; at 101 blocks nothing plays.

**Bird at a tree 60 blocks away, 10:00.** Kind-61 record at the crown top. Each second the
record ticks; with probability 1/10 Play(94, crown, 0.1, 0.9 + 0.2*u*): volume
0.1 · (1 − 0.6) = **0.04**, pitch 0.9–1.1.

**Interface click.** Clicking an options arrow: Play(85, **L**, 1, 1) → *d* = 0, volume =
soundVolume/100.

**Selling while the camera is 6 blocks behind.** Play(58, **P**, 0.75, 1): *d* = 6 →
0.75 · 0.94 = 0.705 at soundVolume 100.

---

## 8. Open points

* **AU-1** Mono-to-stereo level: XAudio2's default matrix for mono sources (both speakers at
  full level, or reduced). *Check*: record the game's output of a mono groan (e.g. id 60)
  and compare left/right peak with the file's peak at known volume.
* **AU-2** Fireball silence (id 38, key case). *Check*: cast the fire staff's fireball in
  the game and listen; or hook 0x623a60 and log the returned handle for `fireball.wav`.
* **AU-3** Zone gate of §3 (zone +0x6c < 4096): confirm the field is the camera distance in
  blocks (log it for nearby zones).
* **AU-4** The hit-interval length of the multi-hit swing rows of §6.2 and the local flag
  that switches 104's sound to 5 and the channel tick to 37. *Check*: hook 0x4864c0 and log
  (id, action, action time) while using cyclone / swirl / staff channels.
* **AU-5** The "never played" list (§6.4) rests on immediate-value scans of all record
  writers; an id computed from a table elsewhere would be missed. *Check*: hook 0x484350
  during a long play session (combat with many creature types, dungeons, traps) and log the
  ids seen.
* **AU-6** Whether the ambience loop of §3 runs while the map is open. *Check*: open the map
  at a meadow in the evening and listen for crickets.
* `alpha-world-items.md` W-4 (offline records through the same loop) remains open.

---

## 9. Implementation notes (non-normative)

* Any audio API is fine. To be 1:1: linear falloff to 100 blocks from the camera, no
  panning, no 3D effects, pitch as a resampling ratio clamped to [1/1024, 2], unlimited
  overlapping one-shot voices, lazy name-cached loading from `data2.db`.
* "Steam-style" stereo placement or a voice cap can be an option, but must default off.
* Keep the play routine the single entry point (id, position, volume, pitch) so mods can
  add ids and files, and keep the two music channels so mods can add music.
