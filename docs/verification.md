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
It now checks SHA-256 standard empty/short/million-byte vectors, changed DLL and
content bytes, content-key changes and path-independent fingerprints. A synthetic
SDK DLL reads its own override during startup, declines or succeeds from that
input, and observes the layer through shutdown. Failed loads and shutdown leave
no owned layer, while another registration of the same folder remains intact.
GenerationProfileTests checks new-world pinning, matching reloads, legacy native
acceptance and rejection of changed settings without modifying world/list files.
It uses Unicode, literal-percent and hash characters in the save path to exercise
the shared immutable SQLite URI encoder used by both store and save readers.
An actual content mod retains its declared version while its bytes change: the
real WorldSession boundary rejects it without changing either save database.
Restoring those bytes reopens the same world with its original seed and clock.
EngineModuleTests also checks the generated source-input identity. A separate
private build check configures a real external module, changes a declared header
without a version bump and observes Ninja's automatic reconfiguration and changed
identity. Normalized line endings and a relocated module preserve that identity.
These are bounded engine compatibility checks; undeclared dependencies, wider
engine/build settings, migrations and cross-peer negotiation remain open.
All 42 CTests passed after this compatibility change. Fresh reward, injured-health,
loopback and both save/reload runs passed, with all five captures reviewed. Saved
reward state retained level 2, -33 XP and 7 Platinum Coins; injured health retained
exactly binary32 50. Reload retained the character, seed, nine map tiles, preview
and 170 block edits. The package passed 35 hashes and 36 entry checks. Known bright
ground and broader native UI/gameplay/visual acceptance remain unresolved.

GamepadTests uses the same provided-state reader path as the GLFW adapter. It
checks stable device selection, attach/replacement held-input suppression,
disconnect/disable/focus neutrality, radial/trigger dead zones, malformed axes,
the shared gameplay mapping and release rearming across menu/map/text contexts.
Actual Interface back navigation cancels chat, drafts and pending front requests;
map back preserves an underlying inventory. Mixed mouse/controller tests check
both release orders, one logical press/release and cancellation without a click.
The client uses the shared gameplay mapping for input and all six HUD slots,
combined UI holds for dragging/crafting and fresh context after callbacks.
These are engine extensions. `GamepadTests --devices` exercises real GLFW device
discovery; the local run found zero mapped pads. Physical controller, reconnect,
focus and complete gameplay/menu acceptance remain open. The later text-entry
checkpoint below does not close hardware or per-action rebinding gates.
All 43 CTests passed for this controller integration. Fresh actual-client reward,
injured-health, loopback and both save/reload checks passed; all five captures were
reviewed. Reward state retained level 2, -33 XP and 7 Platinum Coins; injured health
retained exactly binary32 50. Reload retained the character, seed, nine map tiles,
preview and 170 edits. These runs had no mapped controller and therefore establish
client regression evidence, not physical controller acceptance. Known bright
ground and broader native UI/gameplay/visual gates remain open.

ControllerKeyboardTests checks draft editing/commands, shifted symbols, bounded
focus navigation and key hit regions at three sizes. Actual Interface checks
cover name/seed/server commits, cancel, stale-target invalidation and no automatic
create/connect request. Chat sends once through the existing local queue and
cancel sends nothing. The old non-ASCII transport adapter remains explicit.
Pointer tests also consume both sources at a modal handoff so an old release
cannot activate a new layer. Main routes keyboard/controller edits to the draft,
masks underlying held UI input and supplies the combined mouse state to gameplay.

`ControllerKeyboardRenderProbe <owned Alpha folder> <new output folder>` explicitly
renders the keyboard using owned fonts/interfaces through the real Vulkan UI
pipeline. It refuses an existing output folder and is excluded from CTest and
player packages. Six reviewed captures cover character input, shifted symbols,
seed, server, chat and a smaller window. These establish bounded engine layout
evidence, not physical-controller or original Alpha pixel acceptance.
All 44 CTests passed for this text-entry checkpoint. Fresh actual-client reward,
injured-health, loopback and both save/reload flows passed, and their five captures
were reviewed. Reward state retained level 2, -33 XP and 7 Platinum Coins; injured
health retained exactly binary32 50. Reload retained the character, seed, nine map
tiles, preview and 170 edits. These regression runs do not close physical
controller, complete first-time workflows or broader native acceptance gates.

Mode preservation regressions run a real NPC world update with copied raw flags,
replace the core word, encode the updated actor and exercise consecutive stunned
pieces. They retain both bytes while applying only the relevant movement changes;
a dead rejected update changes neither view. Existing pet and quest tests also
check reconstruction reset and immediate captive create/free publication.

`PlayerStateProbe <owned Alpha folder>` runs the production Player input and
actor code with an actual Vulkan-backed world and supplied controller samples.
It checks raw flag preservation, a later core replacement, controller climbing,
stun cleanup across pieces and dead admission. The local run passed. This explicit
diagnostic reads owned content without saves, networking or an original process;
it is excluded from CTest and player packages. A supplied pad establishes this
engine input path, while physical controller and native behavior acceptance
remain open. First native bit-0x20 production, complete reset/generator coverage
and designated-pointer/world transition lifetime are still unresolved.
All 44 CTests passed for this mode-preservation checkpoint. Fresh actual-client
reward, injured-health, loopback and both save/reload phases passed; all five
captures were reviewed. Reward state retained level 2, -33 XP and 7 Platinum
Coins, injured health remained binary32 50, and reload retained seed 123, nine
map tiles, the preview and 170 edits. Known bright ground and broader native
visual/gameplay acceptance remain open.

An additional real two-client NetTests regression equips a pet while the owner
publishes a full gliding mode word. It acknowledges that word through the second
peer and observes advancing server clock publications without a pet appearing.
Clearing only glider must then create that same pet while retaining unrelated
high-byte flags. This failed before the decoded mode was copied into the server's
body view, establishing that the fixture reaches the actual admission path.
The fixed focused checks and all 44 CTests passed. A fresh actual client connected
to the dedicated server, rendered its reviewed capture and exited successfully;
the test server shut down normally. Native server/gameplay equivalence remains
separate from this bounded engine check.

Complete equipment regressions cover all thirteen source records with negative,
zero, ordinary, full and oversized signed body counts. The real codec retains
every raw byte, while semantic copies preserve each destination's five gaps.
An inactive nonlinked byte changes only the existing equipment delta group and
survives actual partial decoding and core refresh. Direct semantic edits retain
unrepresented backing; self-copy and resident comparison state are also checked.
Live and delayed loot inputs retain the same complete equipment record.

The extended PlayerStateProbe imports complete records, runs production input
and physics, then checks the actual outgoing codec for every slot. This is
bounded engine data-flow evidence, separate from complete creation, generation,
loot and native gameplay acceptance.
All 44 CTests and the extended production Player diagnostic passed. Fresh reward,
injured-health, loopback and both save/reload phases passed with all five captures
reviewed. Saved reward state retained level 2, -33 XP and 7 Platinum Coins;
injured health remained binary32 50. Reload retained seed 123, nine map tiles,
the preview and 170 edits. Known bright ground and broader native visual and
gameplay gates remain open.

VisibilityTests checks low-five-bit categories and raw option bytes, exact
negative fixed coordinates, tied crossings, integer-quantized parameters,
endpoint equality, default-axis behavior and the 501-advance limit. Spatial
fixtures cover ordered side faces, current state admission, odd extent swaps,
half-open boundaries, vertical/inside starts and the retained overshoot rectangle.
An actual NPC acquisition test binds StaticStates: the active window blocks
sight, and changing only its current state admits the target.

All 45 CTests passed. Fresh reward, injured-health, dedicated-server connection
and save/reload flows also passed; all five client captures were reviewed.
Reward state retained level 2, -33 XP and 7 Platinum Coins, injured health stayed
binary32 50, and reload retained seed 123, nine map tiles, the preview and 170
edits. These checks establish bounded engine behavior. Raw caller-position
producers, spatial-record lifetime, broader native collision/gameplay and the
known bright-ground rendering issue remain open.

Standalone held-item regressions cover full raw import/output, negative and
oversized component counts, inactive entries, selected-field assignment,
self-copy and the five destination gaps during actual core refresh. A held-only
edit changes existing wire field 43; later health-only decoding retains that
item. The production Player diagnostic also carries a complete held record
through real input, physics and outgoing serialization. All 45 CTests passed,
along with a fresh dedicated-server connection and reviewed client capture.
Complete native consumable behavior and target selection remain separate gates.

TargetSelectionTests checks entry clock/roll gates, strict health/duration
boundaries, negative and wrapped potion counts, complete held copies before the
two draws, partial friendly writes, preliminary key-zero/self/dead targets,
linked overrides and repeated lookups. Signed live scan ties, rejected nearer
actors, binary32 distance ties, stealth/pacification order, wrapped level
exponents, full-kind food comparison and ordered readiness queries also have
provided-state regressions. ContinueWithTarget explicitly denotes the later
behavior boundary rather than a native success return.

Production NPC fixtures cover typed subject-before-neighbor fanout, zero and
nonzero threat values, distance-64 exclusion, full mode publication, required
quest allies and in-place consumption of the flat inventory adapter. A supplied
origin spawn demonstrates its key-zero preliminary self lookup. Independent
route, schedule, sitting and fighting fixtures use the neighboring zone while
retaining their relative geometry. The lifecycle fixture checks active-threat
retention before ending that relationship to check noncombat despawn.

All 46 CTests passed. Fresh reward, injured-health, dedicated-server and both
save/reload phases passed with all five captures reviewed. Retained state was
level 2/-33 XP/7 Platinum Coins, injured health binary32 50, and saved seed 123
with nine map tiles, the preview and 170 edits. Native identity/range producers,
stat/phase/power equivalence, grouped inventory, later behavior and full native
acceptance remain open; bright-ground rendering is still unresolved.

ActorCreationTests covers action/pointer/mode admission, distinct anticipation
queries, strict crossings and wrapped clocks, live ownership counts and signed
map-key allocation. Copy fixtures retain all thirteen item payloads, selected
appearance flags and independent creator level/health/position sources. Fresh
state, ordered setup, discarded candidate draws, closed generation outputs and
explicit unsupported-generation results are checked separately. Actual NPC and
designated offline-player dispatch create without updating the new predecessor
again in its birth pass.

The same target checks raw reference field 40 through actual delta/core refresh,
descriptor activation and fresh-reference policy. LinkedItemTests verifies its
preservation through real reconstruction. Wander fixtures check counter admission,
unnormalized draw order and retained action clocks. Companion fixtures distinguish
fixed/float distance gates, ordered health queries, both 21-query terrain limits,
repeated turning-point sampling, partial writes on failure, retained setup and
the initial-plus-ten work budget. Supplied geometry/path callbacks establish the
caller relation; the production owned search remains a bounded adapter.

All 47 CTests and the production Player/controller diagnostic passed. Fresh
reward, injured-health, dedicated-server and save/reload flows passed; all five
captures were reviewed. Reward state retained level 2/-33 XP/7 Platinum Coins,
injured health stayed binary32 50, and reload retained seed 123, nine map tiles,
the preview and 170 edits. Configuration producers, remaining generated
appearances, universal reference initialization, broad geometry and online/native
acceptance remain open. Bright-ground rendering is still unresolved.

NetTests also exercises real reconnects with replies 7, -1, 0, the minimum signed
64-bit value and 5. It checks initial identity 1, nonnegative adoption, retained
identity after negative replies and disconnect, actual outgoing IDs and distinct
remote-key classification. Before the fix, nine new assertions failed while
the existing checks passed; all pass after the fix. All 47 CTests and a fresh
connected-client/server smoke passed. The rendered client frame was reviewed.
Full native actor replacement, saved-state restoration, alias lifetime and
incomplete transport behavior remain separate acceptance gates.

Extended ActorCreationTests distinguishes descriptor and actor defaults, checks
candidate/action separation, duplicate appends on repeated setup, full-width role
gates, early returns, the separate action/choice draws and all six seed writes.
Actual finishing and activation paths retain full choice values and independent
candidate allocations. LinkedItemTests also verifies resident configuration
through actual body reconstruction. The action-phase fixture isolates its AI
decision counter so an independent behavior decision cannot replace its supplied
action before dispatch.

Fresh candidate appearances for kinds 17 and 96 are compared byte by byte with
the independent full-extent contract, excluding unspecified byte 5. Boundary
random values still spend all six selection/prefix draws, copy alternatives
spend only selection, and full-width kind classification is checked separately.
Actual NPC dispatch covers the three reachable generated kinds 17, 37 and 96
with independently initialized newborn configuration. Native descriptor refresh,
upstream distribution, other kinds, rendering and online authority stay open.

All 47 CTests and the production Player/controller diagnostic passed for this
configuration/appearance checkpoint. Fresh reward, injured-health, dedicated
server and save/reload flows passed; all five captures were reviewed. Rewards
retained level 2/-33 XP/7 Platinum Coins, injured health remained binary32 50,
and reload retained seed 123, nine map tiles, the preview and 170 edits. Bright
ground and broader native visual/gameplay acceptance remain unresolved.

DescriptorRefreshTests exercises optional refresh gates, exact role 6 versus
full values with the same low byte, wrapping seed input, setup/flag order and
preservation of existing target flags. Item fixtures cover the nine-slot order,
signed level anchors, zero/one draw boundaries, grouped zero/negative counts,
predicate rejection, low-word copies, byte wrapping and the final rarity clamp.
Lookup checks include negative truncation, both domain ends, missing parents and
borrowed field rereads. Activation checks distinguish equal-window bypass from
two clock queries and expose fixed-to-binary32 radius-boundary quantization.

Actual NPC activation observes refreshed level, inventory and flags before its
extension hook, preserves zero hand words, reuses descriptor mutations on a later
activation and drops the overlay when its shared generation snapshot is replaced.
The containing cell supplies context even when the descriptor position belongs
to another cell. Original snapshots remain unchanged. Full native grouped
production, general gear setup, timing/identity sources and pointer lifetime stay
open; these supplied-state checks do not certify original-client gameplay.

All 48 CTests and the production Player/controller diagnostic passed for this
refresh checkpoint. Fresh reward, injured-health, dedicated-server and save/reload
flows passed; all five captures were reviewed. Rewards retained level 2/-33 XP/
7 Platinum Coins, injured health stayed binary32 50, and reload retained seed
123, nine map tiles, the preview and 170 edits. Known bright ground, the negative
XP display and broader native visual/gameplay acceptance remain open.

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
Preliminary local health examples check the exact role/action/ordered-life gates,
including NaN rejection, zero-duration comparison and a fresh third maximum on
excess. Observable stat queries establish addition-before-comparison ordering.
An exact binary32 anchor retains elapsed/seconds/fraction grouping; a negative
duration is not silently skipped or lower-clamped. A common-phase integration
case sees this health addition before a periodic hit cancels action 84.
SimulationFrameTests exercises exact signed piece sequences, shared aggregate
identity and later input, including skipped supplied world admission. A real
41-ms subsystem fixture advances common actors, NPC periodic damage, items,
statics, projectiles and clock through 20/20/1, preserves one hit notification,
and starts the requested primary only afterward. It checks post-world instant
buff duration, retained output lists/new-output views, signed clock/wrap/save
boundaries, configured full-width cooldown keys and action-only release cleanup.
Binding examples cover empty construction, two/five-entry refresh, signed skill
masking, specialization bytes 0/1/2/255, raw subtype precedence and strict heavy
continuation equality. A 41-ms integration changes class, equipment and skills
during world calls, then executes the earlier binding/count; the next refresh
adopts the changed state. Ordinary-primary fallback independently selects from
current equipment. These are supplied-state engine checks, not native input/UI
acceptance or a proof of all stat and cooldown-map dependencies.
All 31 CTests passed for this binding checkpoint. An isolated current-client run
rendered the selected ability bar, HUD and world, then saved exactly 50 health
and exited successfully. The capture was reviewed; its known bright ground is
still unresolved. This does not certify original-game visual or menu parity.
NetTests also checks clock publication waits for the frame gate, drains once,
and accepts an identical later packet as fresh input.

ActorOrderTests checks complete signed-key anchors across both word boundaries,
distinct key/own-ID values, duplicate and null mappings, stable entry identity,
and live insertion/removal before successor advance. Production NPC fixtures
place the controlled callback before, between and after NPC keys, including zero,
and observe actual resident countdowns on either side. A dead NPC remains through
the actor boundary without ticking. Actual in-pass bomb appends include the
controlled entry in generated-key selection, preserve existing references, defer
earlier keys, visit a signed-minimum-wrapped later key and tick on the next pass.
Protected received-state examples check the same controlled/remote merge. These
checks do not establish native creation admission, arbitrary key/ID aliases,
fatal cleanup, target-scan ordering or original-server behavior.
All 32 CTests passed. Fresh isolated client checks retained exactly 50 health,
connected through the loopback server and shut both processes down cleanly.
Save/reload preserved the character, seed 123 despite conflicting reload inputs,
advancing clock, nine map tiles, preview and 170 block edits. All four captures
were reviewed. World pieces stayed at most 20 ms; known bright ground and native
interactive acceptance remain open. Later controlled hit delivery remains a
separate engine adapter from the actor scheduling verified here.

WorldHitDeliveryTests exercises target/source relookup, zero and self keys,
missing and dead sources, living-target admission and delivery-only role-zero
worlds. An actual earlier-key NPC melee hit mutates the controlled target before
its later common phase, calls the SDK once, preserves the producer payload and
queues no replay. A separate live-pass case changes both objects' own IDs while
application still resolves the original entry keys and enforces source-role
admission after the positive-hit prefix. Category-four cases cover negative,
zero and positive roll locks, exact class/specialization admission, motion/lock
effects before its early return, first-match resident replacement, independent
source zero and self event IDs. Remaining positive categories use ordinary
health admission. Complete relationship/protection/fatal effects and native
gameplay acceptance are still open.
The world-hit delivery checkpoint passed all 33 CTests. Fresh local and connected
client smoke checks exited successfully; the loopback server also shut down
cleanly. Both captures were reviewed and retained their known bright-ground
observation. The isolated injured character again saved exactly binary32 50
health. These engine checks do not certify native combat, UI or visual parity.

RelationshipTests covers contact keys and binary32 count saturation, contribution
rounding order, signed notification order and strict eight-unit distance at raw
positions beyond binary64 integer precision. Dead candidates, negative zero,
existing negative/unordered values and unchanged output views have separate
cases. Protection examples retain earlier interruption/contact effects after
rejection and exercise new zero-amount contributions, dead distant role-zero
contributors, missing keys and nonpositive values. Healing tests distinguish
argument direction, mere key membership, live propagation and category exits.
The independently checked kind-89 predicate cases cover all three mask bits.

Complete relationship records round-trip all 24 bytes. Receiver examples check
target/source/target lookup order, zero/equal/missing/null IDs, distinct aliases,
dead actors and last-assignment wins without changing count/contact state.
NetTests blocks publication with the actual frame gate, verifies ordered opaque
bytes through section eleven, then exercises a real server contact that publishes
the same complete record to both clients. Remote core refresh preserves resident
relation values and float bits. These checks retain native actor-lifetime,
classification producers, malformed-input and broader gameplay acceptance gates.
All 34 CTests passed for this relationship checkpoint. Fresh isolated client,
loopback and save/reload checks exited successfully. Reviewed captures retained
the world, character, HUD, nine map tiles and 170 edited blocks; the injured
character saved exactly binary32 50. The known bright-ground observation persists.

FatalContextTests covers signed cached-origin defaults, explicit core groups,
unrelated-delta retention, reverse ownership and forward-link exclusion from
core refresh. It checks descriptor origin across movement, separate engine quest
origin, zero-owner child construction and child reset without parent reset.
Zero-link cases distinguish skipped category maintenance, unconditional key-zero
health stores (including self), resident-seven retention before later expiration,
pet-category creation despite a mapped/recovered zero ID, signed player keys and
allocation wrap. The later designated-actor copy preserves low signed level bits
and raw experience even when category maintenance was skipped. Immediate fatal
sound placement retains raw-integer-to-binary32 narrowing, the clock resets
without changing the action byte, and later ordinary/pet corpse adapters avoid
replaying the sound. These are bounded engine regressions, not full native pet,
fatal/loot, world-context or gameplay acceptance.
All 35 CTests passed for this checkpoint. Fresh isolated health, loopback and
save/reload runs exited successfully, and all four captures were reviewed.
The injured character retained exactly binary32 50 health; reload preserved
the character, seed 123 despite conflicting arguments, nine map tiles, preview
data and 170 block edits. The loopback server shut down cleanly. The known
bright-ground rendering observation remains open.

FatalLootTests exercises actual actor appearance producers sharing one supplied
stream, an intervening presentation draw and a reseed visible to another actor
subsystem without restoration. Live loot input preserves raw integer positions
and full inventory bytes without copying resident relationship maps. A real
drop-service callback sees completed fatal presentation and timer reset while
the linked actor is still alive; drops already exist when application returns,
followed by a direct linked-health store without recursive fatal work. Further
cases distinguish zero/missing/self links and repeated dead-target rejection.
An actual two-target melee pass publishes the first target's drops before the
second target, continues the same stream and emits later death records that
cannot replay loot or consume its draws again. These checks establish bounded
engine routing and ordering, not the native runtime recurrence, startup/thread
state, complete seed history, nested generators or full fatal lifecycle.
All 36 CTests passed for this checkpoint. Fresh isolated health, loopback and
save/reload runs also passed, with all four captures reviewed. The injured
character saved exactly binary32 50; the two save phases retained character
data, seed 123 despite conflicting reload inputs, nine tiles, preview data and
170 block edits. The loopback server shut down cleanly. These runtime checks
retain the known bright-ground and broader native gameplay/visual gates.

A subsequent private black-box experiment loaded the isolated owned 32-bit
`MSVCR110.dll` (SHA-256
`b30160e759115e24425b9bcdf606ef6ebce4657487525ede7f1ac40b90ff7e49`),
after verifying it matched the installation's file. It used only the already
held named `rand`/`srand` API, inspected no native instructions and compared the
actual engine Random class with 2,097,344 returned integers: zero mismatches.
The sample includes 65,536 seeded eight-draw prefixes, a 1,048,576-draw chain,
six explicit seed heads and the fresh main-thread head. Eight fresh threads
each checked sixteen initial results, then 8,192 seeded eight-draw prefixes
with interleaved scheduling; all matched their independent reference streams.
FatalLootTests retains the six observed prefixes as x64 regression anchors.
The runtime copy, probe and diagnostics remain private. This characterizes
the tested API/file and histories; the running game's loaded-module identity,
thread assignments, earlier calls, descriptor seeding and full draw history
remain separate acceptance gates. Seed-one engine startup is still a policy.

FatalObjectTests covers independent association defaults/copies, cached-coordinate
bounds, negative matching ordinals and first-match selection across kind,
association and state gates. Snapshot checks distinguish the static-array index
from the descriptor ordinal and retain the changed timer plus uninterpreted
common-state word. Exact sound checks narrow large fixed-point positions to
binary32 before scaling. Timer cases cover equality, negative inputs and defined
32-bit wrap before the signed lower clamp. Frame views drain once and reset
retains capacity. Publication preserves immediate order, suppresses an identical
final record, retains later changes and responds fully to requested zones.
A production hit fixture observes first death presentation, object state/sound/
snapshot, loot and direct linked-health store in that order. These are bounded
provided-state checks; they do not prove a native positive-association producer,
full static-prefix geometry reception, padding equality or gameplay acceptance.
NetTests also sends two actual object snapshots through the publication helper
and a compressed loopback world update. The client's frame gate blocks queue
publication, then all 88 received bytes, order, uninterpreted state-word bits and
supplied padding survive one drain. Incomplete retained engine storage is omitted
instead of manufacturing zero records; that malformed-state policy is separate
from native container validity.
All 37 CTests passed for this checkpoint. Fresh client health, loopback and both
save/reload phases passed, and all four captures were reviewed. The injured
character retained exactly binary32 50 health; reload preserved seed 123 despite
conflicting inputs, character data, nine map tiles, preview data and 170 edits.
The loopback server shut down cleanly. The known bright-ground observation and
broader native gameplay/UI/visual acceptance remain open.

Platinum reward checks distinguish category-thirteen inventory grants from
section-ten kill experience. CreatureTests covers raw signed two-byte positive,
zero and negative quantities, four-byte balance wrap, unusual item metadata,
unchanged input and automatic group admission before currency credit. NetTests
sends complete grants through an actual loopback peer, blocks publication with
the frame gate, then checks retained bytes, order and a single drain. Applying
quantities 9, 0 and -7 to a balance of 5 produces 7 Platinum; the independent
kill record contributes exactly 17 XP, with no item-to-XP conversion.

UiTests exercises the public inventory input route with a selected adaptation
item: exact payment at the required corrected price, insufficient funds,
signed-negative balance, already-adapted rejection and zero-price adaptation.
Copper stays separate, while level/flag changes, sound and equipment notification
follow admitted payment. These checks retain the owner's always-on price
extension; they do not certify the original imported price-library arithmetic.
CreatureTests separately observes the level helper's debit/increment/health-query
order, equality and zero-amount entry, positive-level entry gate and defined
four-byte addition wrap. Synchronous fatal production and complete native incoming/
fatal caller timing remain separate acceptance gates.
All 37 CTests passed for this reward checkpoint. An isolated actual client
received the three Platinum grants and independent kill credit, displayed
7 Platinum Coins and 17/50 XP, and saved exactly those separate values with
health still binary32 50. The capture was reviewed. Fresh injured-health,
client/server loopback and both save/reload phases also passed; all five captures
were reviewed. Reload retained seed 123 despite conflicting arguments, character
data, nine map tiles, preview data and 170 block edits. These engine checks retain
the known bright-ground observation and native gameplay/UI/visual gates.

CreditTests checks all 24 record bytes, unconditional first/second lookup order,
missing/null/zero/equal/alias keys and pointer-based admission independently of
actor own IDs and the unused type member. Designated cleanup retains repeated
lookups, health-before-map-clear ordering, a fresh forward key, contact clearing
on a missing link and unchanged count/forward-link state. Direct and mode-zero
linked awards remain independent, with ordered-positive linked health, signed
wrapped additions, post-level item stamps and five retained snapshot alignment
bytes. Zero awards still process level/item boundaries; repeated calls are not
deduplicated. A `+50, -50` sequence levels before the debit instead of behaving
like an accumulated zero.

NetTests sends foreign, zero and self credit IDs, signed amounts and unused type
bits through an actual compressed update. The frame gate blocks publication;
the queue then retains record order and drains once. Credit cleanup changes only
the received health bytes and does not create an absent actor. A later position
delta retains that health, while an explicit health delta can replace it. These
checks retain the engine's existing key, online-mode and core-publication
adapters; the full synchronous fatal producer and native caller acceptance stay
separate.
All 38 CTests passed for this typed-credit checkpoint. An isolated actual client
rejected XP awards with a missing victim, a role-zero victim and a foreign
recipient, then processed `+50, -50, +17` against the retained role-one victim.
It saved level 2, -33 XP and 7 Platinum Coins, with health refilled by the admitted
level increment. The reviewed inventory/HUD capture shows those separate values.
Fresh injured-health, multiplayer and both save/reload phases also passed; all
five captures were reviewed. The injured character retained exactly binary32 50,
and reload retained the character, seed, nine map tiles, preview and 170 edits.
These checks do not establish full native fatal, menu or visual acceptance.

RegionProgressTests checks truncation before regional coordinate bounds, allocated
block/record admission, signed contributor traversal, repeated lookup, key-zero
and dead contributors, marker/flag independence and all supported/default
selectors. Counting examples retain negative goals, signed wrap, incomplete
state-one events and immutable earlier snapshots. Cell traversal retains X/Y
order, both world edges and fresh coordinate reads after a supplied callback.
Frame views drain once and retain capacity. Publication keeps intermediate
snapshots and later changes, including a return to an earlier value; explicit
cell requests keep their final response.

An actual world-hit fixture observes fatal presentation/loot, linked health and
regional state before return, then checks the corpse's no-replay marker. Actual
descriptor activation retains the raw marker byte. A separate disposal fixture
retains prefix and tail actors with no draws. Rebuilding an already disposed tail
retains those actors; rebuilding existing descriptors matches their IDs even
after taming, while retaining independent actors and the prefix. These are independent engine storage/lifecycle checks,
not a claim about arbitrary native descriptor writers or allocator reentry.

NetTests carries two actual regional progress snapshots through section thirteen
and the frame gate, retaining order and all received opaque bytes for one drain.
Its real-server fixture restores a synthetic mission/monster, activates the
descriptor, kills its mapped actor through the real client hit route, observes
the same completed snapshot at both peers and reads the saved completed state.
The retained reward adapter emits Platinum Coins followed by gear once. That
adapter's broader recipient and random timing still require fatal integration.
All 39 CTests passed after the descriptor-ownership correction. Fresh actual
client reward, injured-health, multiplayer and both save/reload phases passed;
all five captures were reviewed. Rewards retained level 2, -33 XP and 7 Platinum
Coins; the injured character retained exactly binary32 50. Reload preserved the
character, seed 123 despite conflicting inputs, nine map tiles, preview and 170
block edits. The known bright-ground and native gameplay/UI/visual gates remain.

ItemIdentityTests exercises each item byte in both operand directions, separating
compared header/count fields and active four-byte entry prefixes from padding,
entry trailing bytes and inactive entries. It checks unequal negative/raw counts,
the full 32-entry boundary and the engine's explicit rejection of larger positive
counts. Assignment checks all 275 selected bytes, all five preserved destination
bytes, inactive-body copying, source immutability and self-copy.

Inventory fixtures cover an immediately matching empty row, a later negative
match winning over a remembered empty row, local-only quantity normalization,
zero/negative quantities, signed wrap and subtype fourteen's level exemption.
Unknown material coins use ordinary stacking while known materials and Platinum
retain separate counters. Hand fixtures check raw comparison, zero-row merging,
wrapped counts and field-wise fallback swaps. Linked-equipment fixtures check
the no-consumption equality case, distinct signed count gates, old-item storage
before decrement and correct index reuse after bag growth. CreditTests retains
its existing linked-credit snapshot checks through the shared assignment service.
These fixtures establish engine behavior under the held contracts; complete UI
admission, pet-cache production and native interactive acceptance remain open.

All 40 CTests passed for the item-identity checkpoint. Fresh actual-client
reward, injured-health, multiplayer and both save/reload phases passed, and all
five captures were reviewed. The reward character retained level 2, -33 XP and
7 Platinum Coins; idle injured health retained exactly binary32 50. Save/reload
retained seed 123 despite conflicting reload arguments, nine map tiles, the
33,792-byte preview and 170 block edits. The verified player package contains
35 hashed files in 36 archive entries and excludes owner/private content. These
checks leave the documented native UI, gameplay, visual and provenance gates open.

Additional CreditTests anchors check exact prepared-credit bits for minimum,
fractional, byte-tier, negative-rank and signed-wrap inputs, every specified
kind multiplier and neighboring exclusions. Flag tests retain the independence
of regional admission, multiply the fractional value before truncation and force
zero without a second minimum. Under explicitly selected masked, nearest-rounding
controls, forced-zero output still exposes the earlier division's status; those
controls are restored after the check and are not inferred for the running game.

The actual world-hit regional fixture changes the victim's level during loot,
observes the prepared fractional float after linked health, then changes the
level again during regional processing. The later adapter uses the earlier float
for its integer record, retains no-replay behavior and clears the bridge on a
subsequent live update. Full recipient cleanup/credit/reward timing, every native
storage producer and native gameplay acceptance remain separate gates.

The credit-preparation checkpoint passed all 40 CTests and fresh actual-client
reward, injured-health, multiplayer and both save/reload checks. All five
captures were reviewed. Rewards retained level 2, -33 XP and 7 Platinum Coins;
injured health retained exactly binary32 50. Reload retained the character,
seed, map tiles, preview and edits. Its current player package passed all 35
hashes and 36 archive-entry checks. The known bright ground and broader native
UI/gameplay/visual acceptance gates remain open.

LinkedItemTests checks source/snapshot construction, raw-word comparison,
selected-field publication and ignored/compared body bytes. Actual codec examples
round-trip all 280 current-item bytes across zero, negative, ordinary, full and
oversized signed counts. Received actor assignment copies every selected source
byte, retains its five existing alignment bytes and preserves that actor's
separate snapshot/word. A real mode-zero credit uses the default
actor storage, stamps post-level XP/level and preserves all selected/inactive
bytes without requiring a pet item category or updating the comparison word.

Actual Npcs maintenance fixtures cover publication and missing-child construction
for a dead role-zero owner, source reset versus retained
snapshot, ignored changes without reconstruction, compared coordinates and raw
word changes that reconstruct, non-pet state retention, creation rejection and
non-role-zero exclusion. A controlled late-mirror fixture skips maintenance but
still updates the source, leaving the snapshot/word unchanged. These fixtures
exercise engine-owned storage and the held bounded rules; the documented
selection/configuration/name, full fatal and native acceptance gates remain open.

After the actor-assignment alignment correction, all 41 CTests passed. Fresh
actual-client rewards, injured health, multiplayer and both save/reload phases
passed, with all five captures reviewed. Rewards retained level 2, -33 XP and
7 Platinum Coins; injured health retained exactly binary32 50. Reload preserved
the character, seed, nine map tiles, preview and 170 edits. The current player
package passed all 35 hashes and 36 archive-entry checks. These checks do not
close the known bright ground or native UI/gameplay/visual/provenance gates.

FatalRecipientTests covers signed-key order, null entries and cleanup for all
nonnull roles, with counts and unrelated threats retained. Supplied-state cases
admit dead/key-zero recipients and zero/negative/unordered contribution values
by membership alone. Actual credit consumption occurs between append and grant.
Early aliases of the victim remove later membership; earlier consumers can change
the current link/kind, add future entries and affect later eligibility. Repeated
recipient pointers remain distinct visits, and zero credit still runs the actual
level helper. The complete 288-byte grant codec preserves opaque bytes. New
frame views drain grants/applied-credit metadata once and retain capacity.

Actual retained-descriptor tests exercise immediate engine backing countdown/day
effects, completion-tail disposal, invalid indices, missing cells and coordinate
bounds. The world-hit regional fixture observes post-loot preparation, immediate
credit and the key-zero linked alias, first local Platinum before reward RNG,
gear after that receipt and final backing after all grants. Corpse and subsequent
live-update checks exercise no replay and lifecycle reset. The real server
fixture observes completion rewards at both peers, identical ordered credit
payloads, one killer credit and saved regional state. These checks retain the
documented generator/producer/lifetime and native acceptance limits.

All 42 CTests passed for the synchronous-recipient checkpoint, including the
expanded two-peer server fixture. Fresh actual-client reward, injured-health,
multiplayer and both save/reload checks passed; all five captures were reviewed.
The reward character retained level 2, -33 XP and 7 Platinum Coins. Injured health
retained exactly binary32 50; reload preserved the character, seed, nine map tiles,
preview and 170 edits. The player package passed all 35 hashes and 36 entry checks.
The known bright ground and native UI/gameplay/visual/provenance gates remain open.

With an isolated 41-ms minimum frame setting, the actual engine client reported
138 world calls across 17 input frames and a maximum piece of 20 ms. Its injured
character retained exactly 50 health on exit and the capture was viewed. A fresh
connected-client run reported 96 calls across 17 input frames, also at most
20 ms, then both client and loopback server exited successfully. These are engine
runtime checks, not original-client timing or gameplay acceptance.
The subsequent actual-client save/reload check retained seed 123 despite
conflicting reload arguments, character payload, nine map tiles, preview data
and 170 edited blocks. Both captures were viewed; each reported a maximum world
piece of 20 ms. Bright-ground/output-state acceptance remains unresolved.
A fresh actual-client run used a new isolated synthetic character injured to
50 health. The viewed idle-world capture showed 50 health, and the character
saved on exit retained exactly binary32 50 (`0x42480000`) with its world visit.
This checks removal of passive healing in the engine client, independently of
the helper tests; it is not an original-client gameplay comparison.
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
PeriodicHitTests exercises production countdown and the actual NPC/dedicated-world
entry points: one call across multiple surviving buckets, expiration without a
hit, dead/self/zero/missing source resolution, source-owned mitigation, exact
periodic flags and no action-17 reinsertion. It covers every-shield mutation,
expired-but-resident multipliers, visit-dependent cleanup, status cancellation
before the next producer gate, separate response/velocity writes, local SDK
application once and replica suppression. Direct producer checks distinguish the
beam flash minimum, ordinary presentation flag and null-source overwrite.
Item-effect checks cover full argument-vector normalization, the zero/underflow
gate, raw fixed positions, signed record limits, selected-item/record order,
unsigned material defaults, color arithmetic and both flag variants. The actual
periodic scheduler also emits these effects with its zero initial impulse.
NetTests sends a real packet-8 damage buff to the running loopback server and
checks the server-generated hit reaches both connected clients, including the
target, with matching category, timer and vector.

These synthetic and loopback checks do not establish full combat parity.
Whole-world frame splitting, complete contact/threat/protected-target state,
fatal callback effects, death/loot ordering and native gameplay acceptance remain
open.

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
Additional timed-source frames pass a duration-equality zero color and a half-faded
blue contribution through the real terrain shader, then verify black output after
expiry. No original game content is used by this diagnostic.

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

Timed-light checks cover strict expiry versus duration equality, zero-color
residency, ordered nonmerging appends, mixed durations, fixed positions beyond
binary64 unit precision and integer age advancement beyond binary32 unit
precision. They separately check lower-only fade clamping and conversion of age
and duration to float before division. Item glow checks use exact RGB anchors for
mixed bytes 128/132/255, signed descriptor counts, prefix selection and zero
results for no qualifying entries. These checks bound source arithmetic and
storage; native main-loop time sampling and complete producer admission remain
separate from the engine's GLFW duration adapter.

SectorLightTests checks integer distance-limit equality, per-component fixed
quantization, radius-independent distance admission, all six supplied planes,
ordered cancellation and shifting before float narrowing. Source fixtures cover
the different static test positions/radii, zero daytime RGB, item height/hide
gates and truncating sector endpoints. A real generated WorldItems list supplies
one light per item, and replacement/hiding/unloading remove stale contributions
while preserving sector and list order. Procedural GPU frames also exercise the
actual collector, then hide an item and unload the sector. Their expected light
contributions clear independently of the green global fallback. Camera/reference
production and complete native source coverage remain acceptance dependencies.

Plane-production fixtures check P*V order, sequential four-term binary32 rounding,
raw unequal plane scales, structural-zero multiplications under the test floating
environment and the w+z lower-depth bound. Supplied-plane tests also retain either
sign of zero, ordered-negative rejection and unordered completion, and verify that
only horizontal fixed-point shifts apply. Those checks establish the kernel for
the supplied matrices and ambient test environment. Native projection inputs,
view/reference following, partial publication, imported math and scheduling remain
separate acceptance gates; mesh clipping still uses the engine's Vulkan frustum.

The raw-plane checkpoint passes all 26 CTests and the procedural GPU routing,
growth, clearing, timed-source and sector-lifecycle checks. A walking night client
capture also renders terrain, character and HUD. These engine checks do not close
native camera construction or original-client visual acceptance.

CreatureLightTests checks all four equipment sites, distinct supplied hand
anchors, coincident shoulder/chest sources, raw positions beyond binary64 integer
precision, exact color rounding, ratios above one and below zero, category/count
rejection and ordered-positive health admission. Exceptional ratios use an
explicit engine omission policy outside the held ordinary numeric contract.
Procedural GPU frames pass shoulder/chest sources through the actual terrain
selector and shader, then clear their contributions when the counters reach zero.
The production hand-position fallback and missing native source sites remain open;
these checks do not certify native attachments or full source collection.
The equipment-light checkpoint passes all 27 CTests, the extended procedural GPU
checks and a walking night client capture with terrain, character and HUD.

Additional CreatureLightTests cover the five kind-specific body sources, exact
main-position height, distinct rendered inputs, independent coincident flag
sources, role-specific RGB and ordered-positive health. A received-record fixture
checks flag decoding without record mutation, then applies an appearance-only
wire delta and removes only the flag source. Procedural GPU frames deliberately
give main/rendered sites different positions to expose substitutions, and clear
both terrain rows when their actors become nonpositive. Native position producers
and original-client source/visual acceptance remain separate gates.
The body/received-source checkpoint passes all 27 CTests and the extended GPU
checks; a walking night client capture also renders terrain, character and HUD.

CachedLightTests covers fractional and negative chunk boundaries, preserved input
order/flags, staged invisibility, both publication gates, retained pending lists,
duplicate sources, replacement, empty refresh and reset. Admission tests verify
that flag-clear and rejected records do not advance the phase, zero RGB does,
each chunk restarts its phase, keys visit X/Y order and invalid world keys skip.
Exact anchors exercise clock narrowing, the ordered radius arithmetic and
unclamped kind13 color scaling. These cosine anchors characterize the local
platform adapter rather than defining all original runtime results.

The extended procedural GPU probe checks pending black output, red/blue current
sources with kind13 half-day scaling, unchanged contributions after blocked
refresh and black output after reset. The full 28 CTests pass. A private original
runtime observation separately matches narrowed cosine and actual CachedSources
candidate radii in all 32,896 sampled signed-clock/phase cases under selected
controls. Native worker controls, source publication scheduling, scan/key
construction and matched original-client scenes remain open.
A separate actual-client night capture at the generated town center renders its
streets, buildings and local lighting; the walking capture also shows character,
HUD and map. These captures verify engine rendering, not matched native pixels.

RemoteFighterTests changes health, equipment, counters and flags while the received
action and clock stay equal, then checks that presentation adopts those fields
while its local clock advances. Resident buff bytes survive without consumption
or expiration. Further checks cover action/clock resets, death, defined counter
wrap, two-byte mode width, untouched adjacent padding, mode-only partial deltas
and retention through health-only deltas. All 29 CTests pass. An actual loopback
client/server smoke also connects, renders a capture and shuts both test processes
down successfully. Native interpolation and original-client/server acceptance
remain open; the known bright-ground rendering observation persists.

Extended CreatureLightTests covers all four remaining gates, raw subtype checks
without invented category gates, independent resident-presence admission,
unsigned-byte lantern extrema, unclamped colors and nonconsumption. Placement
anchors retain large raw bases, rounded ninety-degree trig and negative-height
truncation. Action cases check all listed memberships, strict integer equality,
negative/fractional/large cutoffs, no stat query for unconditional/unsupported
actions, one query per conditional case and the default helper's equipment order.
The extended GPU probe renders lantern/action sources in separate terrain rows,
then clears both after their gates reject. All 29 CTests pass for this checkpoint.
Native displayed-position/angle production, frame timing and matched native
scenes remain separate from these supplied-input checks.

A private recorder makes three original scalar trig calls for each of 33,018
bounded yaw/height/clock cases under selected floating controls. The observed
narrowed trig bits match the platform adapter; actual engine collectors match
the supplied-input point/vector placement and color relations using those
observations. This executes no
original creature, pose or transform helper and proves no whole-game equivalence.
The current town night capture also renders successfully.
