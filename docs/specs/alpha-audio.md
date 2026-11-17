# Alpha audio: sound player, listener, ambience, interface sounds, music

Status: FINAL / STABLE / HELD (re-certified 2026-11-16 against the original Alpha evidence)

Behavioural specification of how the Cube World **Alpha** client (`Cube.exe`, SHA-256
84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf) makes sound: the playback
engine, how a sound event (position, id, pitch, volume) becomes audible, the listener, the
ambient bird/cricket/owl calls, every interface-sound call site, the (unused) music system,
and which ids of the 0–100 table are reached. Written from the Alpha decompile and checked
in the assembly where numbers matter. Addresses are citations only (client unless marked
"server"). Items marked *believed* rest on a single reading; §8 lists the open points.

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

---

## 0. Conventions

* G = the game object; W = G + 0x2e4 the world (as in `alpha-rendering.md`).
* **L** = the listener position G+0x140 (§2). **P** = the local player's position (local
  player object G+0x8006d0, its position at +0x10).
* *r* = one C-runtime `rand()` result (0 … 32767, MSVCR110). "*a* + *k*·*u*" is shorthand
  for a random pitch; every site described in this file evaluates it in single precision
  in this order: convert *r* to float, multiply by *k*, divide by 32767.0, add *a*. (So *u*
  = *r*/32767 only up to that rounding order.)
* "Play(id, position, volume *v*, pitch *p*)" means the play routine 0x484350 (§1.2). Unless
  a value is given, *v* = 1 and *p* = 1.
* World positions are 64-bit fixed point (1/65536 block). Section-4 records carry float
  block positions; they are converted to fixed point (×65536, truncation toward zero,
  0x42c460) before playing.
* Times are milliseconds; time of day *t* is G+0x800440 in [0, 86 400 000).

---

## 1. The sound player

### 1.1 Engine

* The audio API is **XAudio2 2.7**, created as a COM object (`CoInitializeEx`, then
  `CoCreateInstance` of class 5a508685-a254-4fba-9b82-9a24b00306af, interface
  8bcf1f58-9fe7-4583-8ac6-e2adc465c8bb; 0x623530). It therefore depends on the DirectX
  End-User Runtime; the `XAudio2_8.dll` present in the installation is not referenced by
  `Cube.exe`. Initialisation: default flags, default (any) processor; one **mastering
  voice** with default channel count, default sample rate, default device. No submix
  voices, no effects, no 3D-audio (X3DAudio) library.
* **Initialisation failure** (0x4c8ae0): a message box "Could not initialize XAudio2 …"
  is shown and the program then **shuts down**; there is no soundless mode.
* Right after a successful initialisation the music volumes are set once (§5).
* The engine object (constructor 0x622da0) is reached through G+0x800714. Its virtual
  methods, in table order (0x71e23c): reap finished voices (0x623410), play music
  (0x623980), stream music (0x6240b0), stop music (0x623e60), "music is playing"
  (0x623600), set the two music volumes (0x623a00), **play a sound** (0x623610),
  **get a sound by name** (0x623a60), destructor.
* **The sound store.** The constructor opens `data2.db` (SQLite, opened with create
  permission; if the file is new the table `blobs(key TEXT PRIMARY KEY, value BLOB)` is
  created, 0x4497b0). Lookup is `SELECT value FROM blobs WHERE key = ?` with the file name
  as key — an exact, case-sensitive match.
* **Loading** (0x623a60). Sounds load lazily, the first time a name is played, and are
  cached by name for the rest of the session (name → handle; handle → sound; handles are
  1, 2, 3 … in load order). Ids that share a file name (§6.5) share one cached sound.
  * The WAV bytes come from the store. A loose file of that name in the working directory
    is read only when the engine has no store object at all (0x623a60, first branch) —
    which happens only if allocating that object failed. A missing `data2.db` therefore
    does **not** fall back to loose files: an empty store is created and no sound plays
    (*believed*: the open routine 0x525590 is SQLite's default open).
  * The RIFF data is walked chunk by chunk from the start; the `RIFF` header is entered,
    every other chunk is skipped by its stated size (no pad-byte alignment). The first
    four bytes after the header must be `WAVE`, otherwise nothing is cached. The `fmt `
    chunk is copied into a zeroed 40-byte format block; the `data` chunk becomes one
    buffer flagged end-of-stream, played once, no loop. In the shipped store: 34 files PCM
    16-bit stereo 44.1 kHz, 25 PCM 16-bit mono 44.1 kHz, 1 PCM 16-bit stereo 44.1 kHz
    with an 18-byte format, 2 PCM 16-bit stereo 48 kHz (`owl1.wav`, `owl2.wav`), 31
    IEEE-float 32-bit stereo 44.1 kHz (most of these carry a leading `Fake` chunk, and
    `Explosion.wav` a leading `bext` chunk, both skipped).
  * A name that is not in the store yields handle 0 and **plays nothing**; nothing is
    cached, so it is looked up again on every later play.
* **Playing** (0x623610; arguments handle, volume, pan, pitch, an unused 0):
  1. If volume ≤ 0: nothing.
  2. Look the handle up; no sound → nothing.
  3. Create a **new source voice** in the sound's own format, maximum frequency ratio 2.0,
     no callback, default send (the mastering voice).
  4. Set the voice volume to *volume* (linear amplitude), submit the buffer, start, set the
     volume again (same value), set the **frequency ratio to the pitch**.
  5. Append the voice to the playing list.
  The **pan argument is ignored**: no output matrix or channel volumes are ever set, so
  every sound plays with XAudio2's default channel mapping — stereo files exactly as
  authored, mono files on both front speakers at XAudio2's default mono level (§8 AU-1).
* **Pitch** is a playback-rate ratio: pitch 1.2 plays 20 % faster and higher. XAudio2 clamps
  the ratio to [1/1024, 2.0] (the maximum given at voice creation), so any requested pitch
  above 2 plays as **2.0**.
* **Voices.** There is **no voice limit and no stealing**: every play makes a new voice, the
  same sound overlaps itself freely, and nothing is deduplicated. Once per frame (0x48b35d)
  the reap method queries every voice in the list and destroys each whose queued-buffer
  count is 0 (finished). Sounds therefore always play to their end; nothing stops them
  early (not even leaving the world).

### 1.2 The play routine (0x484350)

Arguments: id, position (3 × 64-bit fixed point), volume *v*, pitch *p*. In order:

1. **Distance**: Δ = position − **L** per axis (64-bit integer subtraction); each Δ is
   converted to the nearest float and multiplied by 1/65536 (blocks). *d*² = (Δy² + Δx²) +
   Δz² in single precision. If *d*² > **10 000**: return — nothing is loaded or played
   (*d*² = 10 000 exactly continues and ends with volume 0, i.e. silence).
2. *d* = square root of *d*² taken in double precision, rounded to float.
3. **Falloff**: *a* = (1 − *d* / 100) · *v*, in single precision — linear, full at the
   listener, silent at 100 blocks. There is no minimum radius and no curve.
4. Look up the sound by its file name (the id switch, `alpha-creatures-behaviour.md` §14;
   ids outside 0–100 → no name, handle 0, which plays nothing).
5. A pan value is computed (−0.1 × the sound's x in view space through the world-to-view
   matrix G+0x22c … +0x268, divided by its w, clamped to [−1, 1]) and passed on; the engine
   discards it (§1.1), so it has no audible effect.
6. Play with **final volume = (`soundVolume` · *a*) · 0.01** (`soundVolume` is the integer
   option 0…100 at G+0x184, converted to float, read at every play) and pitch *p*.

So final volume = (soundVolume / 100) · (1 − *d*/100) · *v*. Volumes above 1 would amplify;
no caller found passes *v* > 1 (*believed*).

**Helper 0x484320**: Play(id, **L**, 1, 1) — an interface sound at the listener, which always
plays at full volume (*d* = 0).

### 1.3 Where play requests come from

* **Direct calls** from client code (interface, ambience): §3, §4.
* **Sound records** (world-update section 4, 24 bytes: position as 3 floats in blocks at
  +0, +4, +8; id +0xc; pitch +0x10; volume +0x14; a new record defaults to pitch 1, volume
  1, 0x465fb0). A record's position is the source position converted to float blocks
  (0x42c4a0), so far from the origin it carries single-precision rounding. The frame update
  walks the update's record list (0x4955f0 … 0x495650) and calls Play(record id, record
  position, record volume, record pitch) for each. Records from the server and records the
  client's own world step queued (footsteps, swings, splashes, landing) are treated
  identically: same falloff, same listener, no ownership test. Whether an offline game
  routes its step records through the same loop is `alpha-world-items.md` W-4.
* Right after playing a record with id **81** (`Explosion.wav`) or **82** (`punch2.wav`), the
  loop sets the **camera-shake amplitude G+0x1e8 := 1** — whatever the distance, even beyond
  100 blocks (0x495635 … 0x495642).
* The server (`Server.exe`) has no audio code (no audio import, no sound file names); it only
  creates records (it shares the world step with the client, see §6).

---

## 2. The listener

* Position: **G+0x140**, the **camera** position (`alpha-physics-camera.md` §7.3, camera
  fields), not the player. In third person a sound "at the player" is therefore attenuated
  by the camera distance: a camera 8 blocks behind the player plays player sounds at 0.92.
* Orientation: **none**. No direction, no up vector and no velocity are used; there is no
  stereo placement, no Doppler, no occlusion, no underwater filter, no reverb.

---

## 3. Ambient sounds (ids 94–100)

Played from the render routine 0x4ac260 (0x4ac761 … 0x4ad41d), once per rendered frame,
drawing from the C-runtime `rand()` of the thread running that routine (the same CRT
generator the other client `rand()` users draw from; MSVCR110 keeps one seed per thread).

**Scope** (0x4ac523 … 0x4ad472). The routine visits every slot of the client's zone grid
(the square window G+0x2ac / G+0x2b0, width G+0x2dc, slots of 0x268 bytes at G+0x2e0,
coordinates 0 ≤ x, y < 0x80000), holding the slot's lock (+0x250) while it works. For a
populated slot whose zone coordinates (+0x18, +0x1c) match the grid position and which
passes the view test 0x47f3c0, the slot's distance field **+0x6c** is refreshed to the
camera-to-zone distance in blocks (float, from zone point +0x50 and **L**). Then every record
of the slot's static-record list (+0x240: plant records of `alpha-worldgen-vegetation.md`
§1.1 and the tree records 60/61 of `alpha-worldgen-trees.md` §5) is tested, provided the
slot's +0x6c is **below 4096**. A slot that did not pass the refresh keeps its previous
+0x6c (*believed* consequence: zones out of view keep their last distance). The record's
kind is its first field (+0 of the record); its position (x, y, z) the fixed-point position
fields.

**Tick test** (per record, per frame). With period *P* (1000 or 5000 ms):
* *x*ᵦ = trunc(x / 65536), *y*ᵦ = trunc((y + 333·65536) / 65536) (64-bit division truncating
  toward zero; the low 32 bits are used from here on);
* φ = (*x*ᵦ rem *P*) + (*y*ᵦ rem *P*) (C remainders, negative for negative coordinates);
* T = world clock W+0x8000bc (= G+0x8003a0), dt = G+0x8006e8 (the frame's elapsed ms);
* the record **ticks** this frame if trunc((φ + T)/*P*) ≠ trunc((φ + T + dt)/*P*) (32-bit
  signed division truncating toward zero).

**Random draws on a tick, in order**: (1) one `rand()` for the chance; if it fails, nothing
more is drawn; (2) one `rand()` for the pitch; (3) owls only: one `rand()` for the file.
Records that do not tick draw nothing. The sound plays at the record's position through
Play (§1.2), so it is attenuated by distance like any other sound.

| Time of day *t* | Record kind | *P* | Chance per tick | Id | Volume | Pitch |
|---|---|---:|---|---|---:|---|
| 06:00 < *t* < 21:00 | 61 (tree crown top) | 1000 | *r* mod 10 = 0 | **94** `bird1.wav` | 0.1 | 0.9 + 0.2*u* |
| 06:00 < *t* < 21:00 | 62 | 1000 | *r* mod 10 = 0 | **95** `bird2.wav` | 0.1 | 0.9 + 0.2*u* |
| 06:00 < *t* < 21:00 | 63 | 1000 | *r* mod 10 = 0 | **96** `bird3.wav` | 0.1 | 0.9 + 0.2*u* |
| *t* ≤ 06:00 or *t* ≥ 21:00 | 61 | 1000 | *r* mod 10 = 0 | **99 + (*r*₃ mod 2)** `owl1`/`owl2` | 0.1 | 0.5 + 0.2*u* |
| 12:00 < *t* < 24:00 | 0, 1 (flowers) | 5000 | *r* mod 70 = 0 | **98** `cricket2.wav` | 0.05 | 0.9 + 0.2*u* |
| 12:00 < *t* < 24:00 | 2, 3, 4 (grasses) | 5000 | *r* mod 70 = 0 | **97** `cricket1.wav` | 0.05 | 0.9 + 0.2*u* |

Decision order per record (0x4acac5 … 0x4ad127): if 06:00 < *t* < 21:00, kinds 61, 62, 63
take the bird rows and are done; otherwise (and for every other kind) kind 61 by night
takes the owl row and is done; every remaining record falls through to the cricket test.
Boundaries are exclusive as written (06:00 = 21 600 000, 21:00 = 75 600 000, 12:00 =
43 200 000, 24:00 = 86 400 000). Kinds 62 and 63 are tested but no generator makes them
(only 60 and 61 exist, `alpha-worldgen-trees.md` §5), and `bird3.wav` is missing from
`data2.db` anyway, so ids 95 and 96 are never heard (*believed*). Other kinds (60, plants
5–58, statics) make no sound. Nothing depends on biome or zone beyond which records exist;
there is no wind, water, rain or cave ambience.

Expected rates: one bird call per crown every 10 s on average by day; one owl call per crown
every 10 s at night; one chirp per grass or flower record every 350 s in the afternoon and
evening — audible as a chorus because meadows hold many such records.

---

## 4. Interface and item sounds

All interface sounds go through 0x484350 (or its helper 0x484320). "At **L**" means full
volume; "at **P**" means attenuated by the camera–player distance (§2). Volume 1 and pitch 1
unless given. The table lists every direct interface call site of 0x484350 and 0x484320 in
the client (the remaining direct callers are 0x484320 itself, the record loop of §1.3 and
the ambience call 0x4ad408 of §3; 63 call sites in all); "spec" names where the trigger is
described.

| Id | File | Call sites | Where | Trigger (spec) |
|---:|---|---|---|---|
| 83 | menu-open2 | 0x48b33d | L | the "a window holds the input" state turns on (below) |
| 84 | menu-close2 | 0x48b33d | L | that state turns off (below) |
| 85 | menu-select | 0x42b834, 0x42b884, 0x42b8d4, 0x42b934, 0x42b974, 0x42b9b4, 0x42ba04, 0x42ba44, 0x42ba84, 0x42bad4 (routines 0x42b810 … 0x42bab0) | L | each options arrow (`alpha-character-screens.md`, arrow actions) |
| 85 | menu-select | 0x47b899, 0x47b9d8 | L | a menu row / specialization chosen; a skill raised (`alpha-skills.md` §4, `alpha-ui-screens.md` §3.2) |
| 85 | menu-select | 0x497a97, 0x497ae0 | L | quick-item wheel step left / right (`alpha-ui-screens.md` §5.7) |
| 85 | menu-select | 0x4c6656 (via 0x484320) | L | a row selected in a list widget (0x4c6610; *believed* the crafting/vendor rows of `alpha-crafting.md`) |
| 86 | menu-tab | 0x4c5b91 (via 0x484320) | L | a tab chosen in a tabbed item widget (0x4c5a60; `alpha-ui-screens.md` §8.1) |
| 87 / 88 | menu-grab-item / menu-drop-item | 0x47c548 (direct), 0x47c743 (via 0x484320) | L | after a slot/hand swap: 87 if the hand (player +0x11e8) now holds an item, else 88 |
| 88 | menu-drop-item | 0x47ba7e | L | a skill lowered (`alpha-skills.md` §4) |
| 88 | menu-drop-item | 0x47d2aa, 0x47da26, and via 0x484320 at 0x47d1f7, 0x47d324, 0x47d39e, 0x47d418, 0x47d4b8, 0x47d53c, 0x47d5b6, 0x47d630, 0x47d6aa, 0x47d712 | L | an equipment box emptied back into the bag (one site per box; `alpha-ui-screens.md` §8.1) |
| 89 | craft | 0x470c9a | L | a craft completes (`alpha-crafting.md` step 4) |
| 90 | craft-proc | 0x470d11 | L | the craft rolled a bonus (`alpha-crafting.md`) |
| 57 | drop | 0x47dc2c, 0x47dd6b | P | buy back / buy (money −= price at 0x47dd45; `alpha-talk-vendors.md`) |
| 58 | drop-coin | 0x47ce74, 0x47d89a | P, **volume 0.75** | sell from bag / from an equipment box (money += price) |
| 57, 58, 59 | drop, drop-coin, drop-item | records | item position | drop and landing (`alpha-world-items.md` §3, §5; `alpha-server-simulation.md`) |
| 45 | pickup | 0x48ce0c | P | an item arrives in the bag (`alpha-ui-screens.md` §8.1.9) |
| 45 | pickup | records (pitch 1 + 0.1*u*) | creature | world pick-up (`alpha-world-items.md` §4) |
| 46 | disenchant2 | 0x47c801, 0x47cae8, 0x47ccaf | P | disenchant / adapt / reward into bag (`alpha-item-services.md`) |
| 46 | disenchant2 | 0x48d31f | L | a mission objective shown (`alpha-missions.md` §8.2 step 4) |
| 47 | upgrade2 | 0x47d7bc | L | recipe learnt from a formula (`alpha-ui-screens.md` §8.1) |
| 47 | upgrade2 | records | creature | mana/buff gains (`alpha-projectiles.md`, `alpha-pets-bombs.md`) |
| 48 | swirl | records | creature | action 11 (§6.2) |
| 49 | human-voice01 | 0x470a9d, 0x47dbd6, 0x47dd1a, 0x4976e2 | P | refusal: can't carry / not allowed |
| 49 | human-voice01 | 0x47d7e3 | L | recipe already known |
| 49 | human-voice01 | records | P | interaction refusal (`alpha-interaction.md` §5) |
| 50 | human-voice02 | 0x47bf60, 0x47c63f, 0x47cffc, 0x49210e, 0x497e2f | L | refusal: level / class / tab (`alpha-ui-screens.md` §8.1, §5.7) |
| 50 | human-voice02 | 0x4e60d2 | P, pitch **1 + 0.5*u*** | speech-bubble typing (below; `alpha-talk-vendors.md`) |
| 28 | cry | — | — | **never played** (and absent from `data2.db`) |
| 29 | levelup2 | 0x493e8f | P | level up (`alpha-ui-screens.md` "LEVEL UP!") |
| 29 | levelup2 | records (server) | player | inn rest (`alpha-talk-vendors.md`) |
| 30 | missioncomplete | 0x48f9be, 0x4907d7 | L | mana cube received; quest banner (`alpha-missions.md`) |
| 54 | lever | 0x497fda, 0x49802f | L | cycling the options of a speech bubble with keys G+0xd / G+0xe — **dead code** (the bubble pointer G+0x800940 is never set, `alpha-talk-vendors.md`) |

**Window open/close rule** (0x48b2d0 … 0x48b346), once per frame: *open* = (G's virtual
"a window holds the input" query, or the byte G+0x800a40 ≠ 0). Compared with last frame's
value (the static byte 0x76b0c0): off → on plays 83, on → off plays 84; then the static :=
*open*. Opening one window while another is already open plays nothing; switching between
windows in one frame plays nothing.

**Speech-bubble typing** (0x4e5f90): each time the bubble is drawn, if trunc(previous
revealed count / 3) ≠ trunc(new revealed count / 3) (bubble +0x174 versus +0x1d0), one
`rand()` is drawn and Play(50, **P**, 1, 1 + 0.5*u*) is called — at most one sound per draw
however many characters appeared.

Corrections to `alpha-ui-screens.md` §10 (recorded in the re-certification below): 57 is
the **buy** sound, 58 the **sell** sound at volume 0.75; 59 is only the world-item landing
sound; 88 at 0x47ba7e is the skill-lowered sound.

---

## 5. Music and looping sounds

* A complete **music streamer exists but is never used.** Two `Music` objects (each with
  three 640 KB PCM buffers) are created at start-up (0x623530). Each streams an Ogg Vorbis
  file opened with `fopen` (decoder open 0x6271b0 with file callbacks, read 0x626de0 as
  16-bit signed little-endian), refills a finished buffer every frame (0x6240b0) and can
  loop by seeking back to the start (0x626740). The play-music method (0x623980) has **no
  caller**, the executable contains no music file names, and `data2.db` holds only the 93
  `.wav` sound effects. Alpha therefore plays **no music**.
* The two music voices' volumes are nevertheless driven (method 0x623a00, arguments A, B):
  * at start-up, after XAudio2 initialises (0x4c8ae0): A = `musicVolume` × 0.01, B = 0;
  * when the options screen applies settings (0x46f390): A = `musicVolume` × 0.01, B = 0;
  * every frame (0x48c385 … 0x48c432): A = `musicVolume` (G+0x188) × 0.01; B = a stored
    value (0x76b0cc) relaxed toward `musicVolume` × 0.01 at rate 0.001 per ms while a window
    holds the input (§4) and toward 0 at rate 0.00005 per ms otherwise. Relaxing: *f* =
    1 − (1 − *r*)ⁿ with *n* the frame's integer ms (computed by *n* repeated steps, double
    precision, rounded to float; *n* ≤ 0 gives *f* = 0), value := (1 − *f*)·value + *f*·target
    (0x4aba20, 0x4ac150). The per-frame setting overrides the B = 0 of the other two.
  All of this is inaudible in Alpha.
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

### 6.2 World-step action sounds (client 0x60c510 region / server 0x5322d0, shared)

Records at the creature's position (+0x10), default volume 1, for each creature the step
simulates (the step's usual filter, 0x612357 … 0x61237f). Each sound's `rand()` is drawn at
the moment its record is created, in the step's order; rows listed for the same moment are
created in the order listed. Let *τ* = action time (+0x6c) before this step's advance and
dt the step length. Two crossing forms occur:
* "**at P1**" = *τ* ≤ P1 < *τ* + dt;
* "**at X (late form)**" = *τ* < X ≤ *τ* + dt.
P1, P2 are the action's phase durations (`alpha-creatures-behaviour.md` §3.3, 0x43caa0 /
0x447310); P2/2 truncates toward zero. "Two-hander" = weapon type 3 with subtype 5, 6, 7, 8,
10, 11, 15, 16, 17 or 18 (0x444230).

| Id | File | When | Pitch |
|---:|---|---|---|
| 17 | swing-slow | *τ* = 0 while holding a two-hander, action ≠ 91 and not 30–33, 87, 88 (0x6123b4 … 0x612485) | 1 + 0.1*u* |
| 36 | channel2 | actions 30–33, 87, 88, while *τ* < P1: when trunc(*τ*/200) ≠ trunc((*τ* + dt)/200) (0x612495 … 0x612545, 0x61279c … 0x61284c; also `alpha-particles.md` §4) | 1 + 0.25*u* |
| 81 | Explosion | actions 87, 88: *τ* − dt < P1 ≤ *τ* (0x612859 … 0x61294f; no draw; camera shake §1.3) | 1 |
| 16 | shield-swing | at P1, action **10** (0x612782) | 0.9 + 0.2*u* |
| 48 | swirl | at P1, action **11** (0x61298f) | 0.9 + 0.2*u* |
| 14 | smash-jump | at P1, action **54** (0x6129bd) | 0.9 + 0.2*u* |
| 15 | swing | at P1, any other action except 91 (0x612a3b) | (0.9 + 0.2*u*)·1.5, then ×0.5 if two-hander or action 68, 69 or 93 |
| 12 | slam-ground | at P1 + P2/2 (late form), actions 57, 58, 60, 74 (0x612b24) | 1 + 0.25*u* |
| 15 | swing | multi-hit actions 72, 86, 30–33, 11, 91 (not 5), while P1 ≤ *τ* < P1 + P2: when *τ* − dt < P1, or trunc((*τ* + dt − P1)/*I*) ≠ trunc((*τ* − P1)/*I*) (0x612ce4 … 0x612fc3) | 1 + 0.25*u* |
| 82 | punch2 | the same moments, action 91 only, after the swing (0x613016) | 0.5 + 0.1*u* |
| 82 | punch2 | at P1 + P2/2 (late form), action 74, after its slam-ground (0x6132af) | 0.5 + 0.1*u* |
| 40 | magic02 | at P1, action **94** (0x616b9d) | 1 + 0.1*u* |
| 5 / 16 | hit1 / shield-swing | at P1, action **104**: 5 if the beam struck something this step, else 16 (0x616c35) | 1 + 0.25*u* |
| 37 / 36 | channel-hit / channel2 | beam actions (*believed* those of `alpha-particles.md` §4) other than 94, on steps where trunc(*τ*/200) ≠ trunc((*τ* + dt)/200), except the step on which action 104 plays its at-P1 sound: 37 if the beam struck something this step, else 36 (0x616c80 … 0x616d25; `alpha-particles.md` §4) | 1 + 0.25*u* |

The multi-hit interval *I* (0x612d27 … 0x612ddb, integer, truncating): 250 ms by default;
1000 ms for action 91; P2/3 for action 11 (and 5, which makes no sound here); P2/6 for
actions 30 and 32; P2/12 for actions 31 and 33. "The beam struck something" is a per-step
flag cleared at 0x616033 and set at 0x616918 right after a target is added to the beam's
hit lists (*believed* meaning, from that placement).

### 6.3 Hurt voices for every creature type

The critical-hit voice of `alpha-creatures-behaviour.md` §5 step 14 (pitch 1 + 0.1*u*,
volume 1) uses a 120-entry table (0x598296, types 0–119). The pitch `rand()` is drawn
**before** the table lookup (0x598258), so it is consumed even for types without a voice.

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
| 95, 96 | bird2.wav, bird3.wav (record kinds 62/63 never generated) |

Names the id table requests that are **absent from `data2.db`** (exact-key lookup, §1.1), so
those ids play nothing even when requested: **28** `cry.wav`, **38** `fireball.wav` (the
store holds `Fireball.wav`; the staff fireball sound of `alpha-projectiles.md` is
therefore silent) and **96** `bird3.wav`. The store's only key not requested by the table is
`Fireball.wav`.

### 6.5 Ids sharing a file

1 and 20 (`blade1.wav`); 8, 21 and 82 (`punch2.wav`); 40 and 56 (`magic02.wav`); 60 and 62
(`male-groan.wav`); 61 and 63 (`female-groan.wav`). Sharing ids use the same cached sound.

---

## 7. Worked examples

**Footstep 30 blocks away.** A creature 30 blocks from the camera steps on ordinary ground:
a section-4 record id 32, volume 1, pitch 1 + 0.2*u* (`alpha-creatures-models.md` §7.3),
say *u* = 0.5 → pitch 1.1. *d*² = 900 ≤ 10 000; *d* = 30; *a* = (1 − 30/100)·1 = 0.7;
with `soundVolume` 100 the voice volume is 100 · 0.7 · 0.01 = **0.70**. Pan: **none** —
`step2.wav` (stereo, 32-bit float, 0.28 s) plays as authored in both speakers, at
frequency ratio 1.1 (0.26 s long). With `soundVolume` 60 the volume is 0.42. The same
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

* **AU-1** XAudio2 2.7's default output level for mono sources on a stereo (or wider)
  mastering voice is a property of the runtime, not of the game; its exact value is not
  established here.
* **AU-3** The zone distance gate of §3: +0x6c is the camera-to-zone distance in blocks,
  refreshed only for visible, matching slots; which point of the zone +0x50 denotes, and the
  exact view test 0x47f3c0, are not established.
* **AU-5** The "never played" list (§6.4) rests on immediate-value scans of all record
  writers; an id computed from a table elsewhere would be missed.
* **AU-6** Whether the ambience loop of §3 also runs while the world map is open (the order
  of the render routine's map branch, `alpha-rendering.md` §1.3, is not re-traced here).
* **AU-7** The thread arrangement of the C-runtime `rand()` users (render routine versus the
  client world step) and hence whether ambience and step sounds share one seed.
* `alpha-world-items.md` W-4 (offline records through the same loop) remains open.

---

## Re-certification (2026-11-16)

Checked against `Cube.exe` (SHA-256 above) assembly and decompile, `Server.exe` imports and
strings, and the `data2.db` key list and WAV headers. Non-normative advice and test
procedures were removed (former §9 and the "Check" items of §8); "engine" in this file
means only the game's own audio object (§1.1).

Corrections:

1. **Reversal — XAudio2 failure.** Initialisation failure shows the message box and then
   jumps to program shutdown; the game does not continue without sound (0x4c8ae0, branch to
   the shutdown label after the message; the whole game set-up lives in the success branch).
2. **Reversal — loose-file fallback.** Loose files are read only when the engine has no store
   object (allocation failure), not when `data2.db` cannot be opened; the store is opened by
   SQLite with creation and an empty `blobs` table is created if needed (0x622da0, 0x4497b0,
   0x623a60 branch on engine +0x2c).
3. **`cry.wav` (id 28) is absent from `data2.db`**; the missing-file list now holds 28, 38,
   96 (store key list versus the id switch of 0x484350).
4. **Random pitch evaluation order** stated exactly: float(*r*) × *k* ÷ 32767 + *a* in single
   precision (0x4acc22 … 0x4acc45, 0x61243c … 0x61246b, 0x4e6091 … 0x4e60ba, 0x598258 …
   0x598278).
5. **Half-P2 crossings use the late form** *τ* < X ≤ *τ* + dt, unlike the P1 crossings
   *τ* ≤ P1 < *τ* + dt (0x612a85 … 0x612add, 0x613200 … 0x61325a versus 0x6126f3 … 0x612708).
6. **Multi-hit swing interval resolved** (former AU-4): *I* = 250 / 1000 (action 91) /
   P2/3 (11, 5) / P2/6 (30, 32) / P2/12 (31, 33), and the trigger includes the first step in
   P2 (*τ* − dt < P1) (0x612d27 … 0x612f56).
7. **Beam flag meaning** (former AU-4): the flag that selects 5 vs 16 and 37 vs 36 is set
   when the beam strikes a target in this step (0x616033, 0x616918); the 200-ms beam tick is
   skipped for action 94 and on the step where action 104 plays its at-P1 sound (0x616bc7 …
   0x616c80).
8. **Omission filled — rows 36 and 81** for actions 30–33 / 87, 88 in §6.2 (0x612495 …
   0x61294f), with the Explosion record's camera shake.
9. **Random-draw order — hurt voice**: the pitch `rand()` precedes the type table, so it is
   consumed for voiceless types (0x598258 before 0x59828d).
10. **Random-draw order — ambience** made explicit (chance, pitch, owl file; no draw without a
    tick) and the per-record decision order stated (0x4acac5 … 0x4ad127).
11. **Ambience scope**: the loop visits the zone grid slots (not a draw list); +0x6c is the
    camera-to-zone distance refreshed only for visible matching slots; the slot lock is held
    (0x4ac523 … 0x4ac737, 0x4ad442 … 0x4ad472). Former AU-3 narrowed.
12. **Music volumes**: start-up and the options screen set A = `musicVolume`·0.01 and
    **B = 0** (not both to the music volume); the relax step is iterated over integer ms
    (0x4c8ae0, 0x46f3eb … 0x46f413, 0x4aba20, 0x4ac150).
13. **Pan formula** restated as view-space x / w through the world-to-view matrix (not clip
    space); still discarded (0x484350 tail, `alpha-physics-camera.md` §7.3 item 10).
14. Distance sum order ((Δy² + Δx²) + Δz²) and the boundary *d*² = 10 000 (plays at volume 0)
    stated (0x4845a2 … 0x4845e9).
15. 0x47c548 is a direct call of 0x484350; only 0x47c743 goes through the helper. Options-arrow
    call addresses given as the actual call instructions.
16. The 46 sound at 0x48d31f is no longer marked as conflicting with `alpha-missions.md`,
    which now also places it at G+0x140.
17. Speech-bubble typing rule stated (at most one sound per draw, old/3 ≠ new/3, 0x4e5f90).
18. Stored-format inventory of `data2.db` corrected and the `Fake`/`bext` leading chunks noted;
    ids sharing files listed (§6.5).
19. The installed `XAudio2_8.dll` is not referenced by `Cube.exe` (no such import or name).

Confirmed unchanged: XAudio2 2.7 CLSID/IID (0x702d68, 0x702d78), voice creation with maximum
ratio 2.0, volume set twice and frequency ratio = pitch (0x623610), no output matrix, reaping
by queued-buffer count (0x623410), distance cut-off 10 000, falloff (1 − *d*/100)·*v*, final
volume factor 0.01·`soundVolume` (0x484350), listener G+0x140, camera shake on 81/82
(0x495635), all ambience constants (periods 1000/5000, 333-block offset, chances 1/10 and
1/70, volumes 0.1/0.05, pitch bases 0.9/0.5 and span 0.2, the 4096 gate), every §4 call site
id/position/volume, the hurt-voice table (0x5987c4 index, 0x59876c targets), and the §6.2
ids, actions and pitch ranges other than those corrected above.

Cross-spec conflicts (other specs not edited):

* `alpha-ui-screens.md` §10 says nearer sounds are "attenuated with distance and **panned**".
  This spec: the pan value is computed but the engine never applies it (0x623610 sets no
  output matrix or channel volumes; the pan argument is unused).
* `alpha-ui-screens.md` §10 table: 0x58 at 0x47ba7e is described as "an item is put down";
  this spec reads it as the skill-lowered sound (0x47ba5c decrements a skill point before
  the call). Its row "0x39 / 0x3a / 0x3b buying, selling, dropping": this spec has 0x39 buy,
  0x3a sell (volume 0.75) and 0x3b only as a world-item record sound (call sites in §4).

Remaining uncertainties: AU-1, AU-3 (partly), AU-5, AU-6, AU-7 and `alpha-world-items.md`
W-4 (§8); the meaning of the beam flag and of slots keeping a stale distance (*believed*);
SQLite default-open behaviour for a missing `data2.db` (*believed*).
