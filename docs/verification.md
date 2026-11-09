# Verification

Configure with a 64-bit toolchain, build, then run the existing checks through
CTest. Content, save and UI checks can use an explicit owned installation:

```bat
cmake -S . -B build -DALPHAVULKAN_TEST_GAME_PATH="path to Cube World Alpha"
cmake --build build
ctest --test-dir build --output-on-failure
```

Without a supplied installation, checks use normal discovery. Some report that
owner checks were skipped when content is unavailable; a passing synthetic-only
run is not evidence that owner-content checks ran. Test scratch files belong in
the build directory. The owner's installation is opened read-only.

For native generator recordings, use:

```bat
build\ZoneCompare.exe --strict path-to-recording.avzd
build\ZoneCompare.exe --strict --edges path-to-recording.avzd
```

Strict mode fails on any mismatch in the fields the tool compares, including
stored air/water light, counts, and unexpected trailing bytes. It also accepts
region, spawn and contents recordings. The original reporting mode remains
available without `--strict`; its successful exit means a recording was readable,
not that it matched. Flags apply regardless of their position in the arguments.

The contents comparison currently covers sprite kind/position, the listed static
fields, and item category/subtype/position. It does not establish full record or
visual parity. `--edges` regenerates neighboring zones; it does not reconstruct
an arbitrary native zone admission history.

New spawn recordings use AVSP version 2, adding the explicitly recorded native
generation-prefix count after the full count. `--strict --prefix file.avsp` compares
that prefix while still validating the complete file, including its later suffix.
It rejects legacy version 1 recordings because their boundary was not recorded.
Without `--prefix`, comparison still checks the full list. This isolates an observed
list boundary; it does not establish that retained prefix records never mutate.
Old spawn-count differences cannot be relabeled generation errors from counts alone.

ZoneCompareTests generates a diagnostic recording, checks it, then alters solid
color and clear-cell light independently. Both must fail strict comparison while
remaining readable in reporting mode. Truncated, missing and trailing data,
unknown options and an empty input list must also fail. This validates the
comparator's rejection behavior, not native generator parity. Generated fixtures
and native recordings stay outside source and runtime packages.

OwnerContentTests checks a standard SHA-256 vector, unsupported/incomplete installs,
configuration preservation and, when an owner installation is supplied, every asset,
private extraction, output rejection and unchanged source-file hashes.
EngineModuleTests loads a compiled module without a DLL/folder, exercises an actual
generation pass, checks ordering/duplicate rejection and verifies shutdown/reload.
ModTests continues to exercise the separate TimeLapse DLL and content override path.
GenerationProfileTests checks new-world pinning, matching reloads, legacy native
acceptance and rejection of changed settings without modifying world/list files.
It uses Unicode, literal-percent and hash characters in the save path to exercise
the shared immutable SQLite URI encoder used by both store and save readers.

NoiseMathTests uses exact binary64 anchors from the held bounded-cosine numeric
contract, including adjacent reduction labels and cancellation near half pi.
`NoiseMathTests --bench` also reports local throughput and bit differences from
the platform library; that library is not a native oracle. The contract assumes
nearest rounding and gradual underflow and excludes the original x87 fallback.
Runtime controls, the remaining color byte and native visual acceptance stay open.
A private headless black-box check subsequently called the identified owner's
32-bit math runtime on 81,966 signed bounded inputs and compared recorded binary64
results with the 64-bit engine kernel: zero bit mismatches. The isolated recorder
explicitly selected x87 control 0x027f and MXCSR 0x1f80. This verifies those inputs
under that environment; actual terrain-worker controls and the remaining color
byte are still unresolved. The private runtime copy and observations are excluded
from source and packages.

ActionPhaseTests exercises common lock, combo timeout, cooldown, buff-expiration,
stun and resource boundaries, including deferred removal of shields depleted by a
later periodic callback. It also exercises local melee/beam dispatch before the
common tail and carries those events through later NPC simulation without a second
attack. RangedSchedulingTests covers real NPC dispatch plus anticipation, charge,
audio and random-draw boundaries. Role/rank resistance examples check the fixed
stat relation's distinct base override and common flag bonus, alongside ordered
equipment additions. Native power-library precision remains a separate limit.
Large-clock examples preserve 16,777,217 ms in integer state and on the wire.
Incoming-hit examples use the actual NPC facade to distinguish resolved and
unresolved sources, ID zero, and class/weapon threat precedence.
Primary-input checks distinguish admitted busy attempts from action replacement,
exercise negative roll locks and raw equipment subtypes, and retain the strict
combo boundary. Common-hit checks use exact binary32 charge anchors, fixed-point
differences at large coordinates and across signed wrap, ordered item additions,
and critical/stat/variation call order. Runtime hit construction uses raw Body
coordinates; imported square-root status/control equivalence remains unverified.
Common initializer checks cover inclusive anticipation clearing, the zero-lock
missed-combo gate before dispatch, distinct mana and recorded-charge amounts,
charge crossing equality, pending overwrite and negative-zero preservation.
BuffTests exercises opaque record/event bytes, independent embedded/header source
IDs, first-match replacement/consumption, signed integer expiry and actual NPC
hit-before-buff ordering. NetTests uses a controlled loopback peer to exercise
packet-8 transmission and compressed world-update buff reception for nonlocal
targets. These checks do not cover every native producer or network admission.
Selector-17 examples check the explicit full-width argument independently of the
attacker's action, post-multiplier periodic amount, append behavior, source zero,
record/event semantic payload and one-time forwarding through actual melee
dispatch. The native producer leaves alignment bytes unspecified. Fresh engine
records use deterministic zero alignment; received records preserve opaque bytes.
No bit-for-bit native claim is made for uninitialized producer storage.
HitRecordTests checks the complete 72-byte payload, signed timers, raw fixed-point
positions, raw flag bytes and opaque storage through the typed network wrapper.
NetTests also sends and receives actual hit packets and compressed world hits.
Response-only deltas use presence bit four, preserve the other motion vectors,
and compare signed zero numerically.
HitApplicationTests exercises category-specific mutation order, protected-target
admission after the scalar prefix, independent response/velocity ownership,
positive-health role predicates, negative healing, both timers and every resident
shield. It verifies exact scalar-reduction/response-decay anchors and consumes a
stunning hit through real physics. These are bounded synthetic checks, separate
from original-runtime gameplay comparison. The local dispatch fixture explicitly
keeps damage above mitigation and starts with valid health, so its ordering check
cannot pass merely because an unrelated maximum-health clamp lowered health.
Generic-hit checks also apply a shield record before later main-record work and
verify the resulting action cancellation, status gate and random-draw count. An
actual NPC melee dispatch exercises that same immediate-consumption path. Dead
targets consume no beam-presentation draw. Deferred projectile/local-player
application and complete world-phase timing remain separate integration work.
Health-admission checks retain the native one-or-two maximum-health queries,
including a fresh second result, signed-zero preservation and ordered/unordered
comparisons. The actual NPC facade skips behavior, timers and resident countdown
for rejected health. The earlier request/input stages and separate death-world
processing remain outside that bounded phase check.
SourceAuxiliaryTests exercises action/item priority, an empty selected item's
contribution to the divisor, ratios above one, signed counter conversion/caps,
negative contribution cancellation and uncapped source healing. It checks the
self record's raw position and semantic fields, emission-before-health order,
health-before-main order, and actual NPC/projectile local delivery policy.
NetTests also exercises real-server duplicate self records and unresolved foreign
source IDs on a role-zero relay. Native server routing includes the sender and
does not suppress self echoes; this is distinct from avoiding a second local
application of an effect performed directly by the local producer. Complete
native incoming-category and end-to-end gameplay acceptance remain open.
IncomingHitTests separately checks received categories 0/3, 1 and 5: level-only
meter arithmetic, raw critical-byte nonmultiplication, timer merge, unchanged
motion/action fields, resource/divisor order, every-shield updates and controlled
event headers, fresh health-limit queries and sequential zero-health rejection.
Native incoming mutation is distinct from generic world application. Network
snapshots now use a shared frame gate: the controlled peer checks late own-state
publication before hit sends, hit-before-buff wire order and deferred incoming
publication while a frame is held. Received creature updates separately publish
insertion/baseline and decoded assignment. First partial updates retain specified
constructor properties; successive partial updates preserve received opaque bytes.
Fresh unspecified native storage uses deterministic engine zero without a native
byte-equality claim. Remaining categories, complete native frame timing and
original-runtime gameplay acceptance stay open.
These checks do not establish full combat parity:
production periodic-hit integration, complete death/loot/threat behavior and native gameplay
acceptance remain open.

The held opaque-output trace establishes no RGB gamma encode in the native
terrain pixel program, but leaves complete sRGB-write, callback, target and display
history unresolved. It does not authorize a guessed output correction. Rendered
engine captures remain diagnostics until compared with an original runtime under
known state and matching scene conditions.

LookTests now retains exact CPU anchors for float clock conversion before the
time curve, stored spatial palette coefficients, positive-only Q admission and
two-product overrides including extrapolation above one. Approximate checks reject
nonfinite outputs. Vertex and fragment operation updates compiled and produced a
walking-client capture; bright ground remains. GPU reciprocal/NaN behavior,
interpolation, native combined matrices, spatial inputs, light selection and full
output transfer are not established by those CPU checks or that capture.

`tools/VerifyPackage.ps1 -Archive <package.zip> -BuildDirectory <build-folder>`
checks the current client/server/setup identities and hashes every ZIP entry
directly against its manifest. It also requires the setup/SDK/player docs, rejects
duplicate or escaping paths, and excludes original assets/private diagnostics.
The fresh development archive passed, and a private copy with a modified README
was rejected for its hash mismatch. Package integrity is separate from beta
acceptance and provenance review.

The menu checkpoint rendered and visually inspected twelve diagnostic states:
the escape row, inventory, options, skills, character list/creation, world
list/creation, adaptation, vendor, crafting and map. These captures use a limited
loaded world; the map's surrounding unloaded area is not a full-world comparison.
Bright terrain and HUD overlap with large panels remain visual review items.
UiTests also drives public screen input APIs through character creation/duplicate
rejection, existing-character selection, separate world name/seed input, invalid
seed rejection, multiplayer list/connection requests and option draft/button
dispatch. It writes no owner saves and makes no external connection. These are
renderer and component checks, not completed interactive save/game/network flows.
The owner reported the isolated original reaching its menu after their runtime
installation; computer use independently observed that game window but could not
capture its pixels. Native visual/menu/gameplay acceptance remains open.

SpatialScalarTests exercises wrapped/truncating windows, sample-read order and
reloads, boolean membership row skipping, missing samples, zero-weight NaNs and
the SSE invalid-conversion flag. Live cache checks preserve scalar bits across a
block write and reject wrong/out-of-domain hints. Local actual-world query timing
improved from a 35.975 ms median to 30.034 ms per 100,000 queries with the same
checksum; this is a local throughput sample, not an FPS or native timing claim.
After scalar integration all fourteen retained native region recordings compare
exactly. The full 209-zone audit still has the same one blue-byte discrepancy in
87,447,092 solid blocks and retained stored-light differences; it is not an exact
whole-corpus pass.

TerrainLightTests covers wrapped fixed-point priorities, bounded native sort
witnesses, inclusive XY membership, negative truncation, per-chunk sixteen-slot
limits, positive-zero clearing and fixed ten-slot uploads. A separate private
original-runtime recording matches the actual engine sort on 50,000 finite-key
cases. Larger mixed-tie permutations and exceptional keys are not universally
certified by those samples.

Run `TerrainLightRenderProbe <output-folder>` explicitly on a Vulkan-capable
desktop for procedural GPU verification. It uses no owner content and is excluded
from player packages and automatic CTest. Red/blue quads check separate chunk
rows, a larger subsequent upload checks buffer reallocation/descriptor replacement,
empty rows check clearing, disabled first-ten slots check the absence of active
compaction, and removing rows restores the green global model fallback. These
checks pass on the local GPU, independently of native pixel acceptance.

Saved-character entry now synchronizes the observed recipe level after restoring
the saved list. It does not reroll randomized grants for the loaded level; later
observed level mismatches publish the tracker before granting once. This uses the
separate held recipe-entry contract and does not certify the full recipe table or
frame scheduling. Startup map persistence is installed before initial generation,
so tiles completed during world construction can be written to the map database.
An isolated actual-client smoke created a synthetic character and saved world,
removed 170 terrain blocks, exited and reentered using conflicting requested seed
and coordinates. Both phases preserved the character payload and recipes, retained
the saved seed/location and all edits, advanced the saved clock, decoded nine map
tiles and retained the world's preview. Both captured frames showed the crater.
This verifies an engine save/reload path; interactive menu flows and original-game
save interoperability remain separate acceptance gates.
The final run also visually confirmed the saved character's name in both HUD
captures. An explicit `--name` remains an engine display/network-name override.
