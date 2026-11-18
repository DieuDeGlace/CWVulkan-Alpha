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

RouteGeometryTests checks initial/iteration proximity differences, separate
square-root queries, capped work, exact vertical increments and progress success
without actor-position writes. Terrain cases retain missing-column blockage,
swimmer handling, raised-box height replacement and remaining inner-Z queries
after blockage. Record fixtures cover pointer exclusion, selector/state order,
directional truncation, edge equality and negative odd orientation.

Bucket checks cover X-outer/Y-inner scan order, first-record exits and the full
first-list traversal before a retained third-axis terrain result is tested.
Production index cases retain duplicate-valued records, one center membership,
own-chunk and negative boundaries, live state, owner/ordinal metadata, immediate
visitor stop, snapshot replacement and unload. Endpoint overlap uses snapped
centers and accumulates unique identities. A real companion-world fixture uses
the current object state to choose direct steering or the separate search
continuation. Linked reconstruction clears search containers and exclusions.
Search expansion/classification, imported math and original-client gameplay
remain separate acceptance gates.

All 49 CTests and the production Player/controller diagnostic passed for this
geometry checkpoint. Fresh reward, injured-health, dedicated-server and save/
reload flows passed; their five captures and a repeated health capture were
reviewed. Reward state retained level 2/-33 XP/7 Platinum Coins, injured health
remained binary32 50, and reload retained seed 123, nine map tiles, the preview
and 170 edits. Bright ground and negative XP display remain unresolved.

The concurrent health run was unusually slow and accumulated 3,726 world calls
over 17 input frames. An isolated repeat of the unchanged build completed in
16.427 seconds with 133 calls over 17 frames. Both retained health 50 and maximum
20 ms pieces. The stall did not reproduce; contention is a possible explanation,
not an established cause or a general performance acceptance result.

SearchStateTests checks signed lexicographic selection ties, distinct seed
binary32 rounding, strict destination-score replacement, pending-only
improvement, retained destination-score epochs, new/improved Y-minus costs and
same-call cardinal updates controlling diagonals. Limits admit equality and
clear an exceeded count only on the next call. List checks retain missing-parent
partials, self-parent stopping and the count-plus-one lookup guard for cycles.
Setup checks seed-before-endpoint order, snapped centers and unique exclusions.

Classifier cases check the complete expanded boundary query stream, all terrain
flag sites, the plus-Y lower-plane asymmetry, the independent sample bit, negative
conversion boundaries and inverted ranges. Record cases distinguish pointer
exclusion, early below-probe, height, selector, raw tag, zero state, edge equality,
odd orientation and repeated ordered bucket entries. An integrated kernel case
builds a flat-ground route and pops the exact goal without another classification.
The full 50 CTests passed; a later test-only addition of that integrated case
was rebuilt and passed separately. Production Paths/Npcs are unchanged in this
checkpoint, so earlier client captures are not presented as new runtime evidence.
Follower binding, imported math, resident lifetime, native gameplay and server
equivalence remain open.

RouteFollowingTests checks arrival versus empty-list/timeout order, exact timer
threshold, nearest-node tie order, partial and far output, sub-unit directions,
ground-plane/lookahead query order, independent velocity regimes and binary32
recurrence anchors through the four-iteration grouping boundary. Zero, negative
and minimum signed elapsed retain the specified no-blend behavior. Reroot cases
cover the current reference before predecessor lookup, accepted-chain union,
rejected branches, unchanged scores/list and the seventy-key score/tie limit.

Object cases retain forced vertical output, inflated asymmetric overlap,
independence from search exclusions/tags, duplicate live-state observation and
actual resident byte/timer/change publication. Fallback checks distinguish the
three fixed half-unit offsets from actor height. An actual NPC world fixture
checks false/nonempty and false/emptied flags, stun bypass, output retention and
counter wrap after integration with no active path. Existing flat searches,
route-around-wall and companion fixtures now use the production core. Linked
reconstruction also restores the specified raw reference triplet.
All 51 CTests passed for this production binding. The actual Player/controller
diagnostic passed; hardware/controller and original-client acceptance stay open.
Fresh reward, injured-health, dedicated-server and save/reload flows also passed;
all five captures were reviewed. Reward state retained level 2/-33 XP/7 Platinum
Coins, injured health remained binary32 50, and reload retained health 265, seed
123, nine map tiles, 33,792 preview bytes and 170 edits. The health run reported
138 world calls over 17 input frames with maximum 20 ms pieces. Bright ground
and negative XP display remain open; these engine regressions do not certify
original Alpha route behavior or physical controller operation.

DescriptorEquipmentTests checks every early classification return, exact generic
clear masks across complete records, fixed kind/action routes, full role/kind
widths and repeated appends. Random cases distinguish all hand outcomes, negative
remainders, unrecognized classes, the forced-class discarded draw and visible
field publication before later draws. Optional cases retain failed fields and
accessories while checking conditional parity calls and subsequent seed order.
Numeric cases cover the complete kind table, omitted fields, low-byte options,
the hand predicate, boss caps/scales and distinct unsigned/signed configuration
draws. An actual actor adapter retains full inactive payloads and gap bytes.
The seven affected existing suites passed, followed by all 52 CTests on the final
build. The production Player/controller diagnostic also passed. These checks
establish the supplied setup and engine adapters; compact generated descriptor
payload storage and full native generation/activation acceptance remain open.
Fresh reward, injured-health, dedicated-server and save/reload flows passed for
this build; all five captures were reviewed. The reward retained level 2/-33 XP/
7 Platinum Coins; injured health remained binary32 50 with one visit and 139 world
calls over 17 frames. Reload retained health 265, seed 123, nine map tiles, 33,792
preview bytes and 170 edits. Bright ground and negative XP display persist.

InventoryValueTests checks fresh empty groups and level-one item defaults,
ordered deep assignment with zero/negative/minimum counts, retained destination
gaps and all inactive payload bytes, self-assignment and empty-source reset.
Insertion cases distinguish explicit/automatic group routing, internal item word
from entry quantity, zero/negative increments, identity before vacancies, negative
matching counts, first-zero reuse and wrapping count/accumulator arithmetic.
The collection-only replacement test observes count one before each of ten
ordered callbacks while other inventory members survive.

An actual actor activation fixture observes the full transferred value during
the following health stat query. Another fixture refreshes a grouped
descriptor, retaining negative counts, skipping zero/rejected entries and keeping
the generated source immutable before copying the actor value. Legacy compact
rows remain explicit one-unit inputs rather than running automatic normalization.
These tests do not certify later legacy selection/vendor/loot adapters, complete
inventory generation, native allocation/publication timing or server equivalence.
All 53 CTests and the production Player/controller diagnostic passed. Fresh
reward, injured-health, dedicated-server and save/reload flows passed; all five
captures were reviewed. Reward state retained level 2/-33 XP/7 Platinum Coins,
injured health remained binary32 50 with one visit, and reload retained health
265, seed 123, nine map tiles, 33,792 preview bytes and 170 edits. The health run
reported 138 world calls over 17 input frames with maximum 20 ms pieces. Bright
ground, negative XP display and physical-controller acceptance remain open.

The later grouped-consumer checkpoint removes the NPC flat inventory cache and
uses one owned InventoryValue. Actual-world health cases cover a zero row before
a later-group match, multi-unit consumption, negative quantities and minimum
signed decrement wrap. Complete held/remaining item fields survive and the next
matching row stays untouched. Fatal-hook fixtures preserve every ordered count,
then verify two positive marker units, skipped zero/negative/healing rows and
unchanged source quantities. Live/delayed loot and existing region/object ordering
checks use the same counted input type.

UI checks retain all exported groups, zero/negative counts, extra scalars and
destination gaps independently of the older widget list. The full 53 CTests pass
after these connections. Native vendor transactions, periodic and alternate-mode
refresh, identity/lifetime, nested drop publication and original-client behavior
remain open; the counted export is not a complete vendor or loot certification.
Fresh reward, injured-health, dedicated-server and save/reload flows passed, and
a separate vendor-window render passed; all six captures were reviewed. Rewards
retained level 2/-33 XP/7 Platinum Coins, injured health remained binary32 50 with
one visit, and reload retained health 265, seed 123, nine tiles, 33,792 preview
bytes and 170 edits. The health run reported 134 world calls over 17 frames;
the vendor render reported 98 calls over 17 frames, both with maximum 20 ms pieces.
An initial vendor diagnostic invocation omitted the required numeric `--loot`
argument and aborted in argument parsing. The corrected invocation passed on
the unchanged binary. Bright ground, negative XP display and hardware controller
acceptance remain open; the vendor capture establishes rendering only.

VendorTests now exercises the independently supplied transaction services and
the public inventory input path. It covers logical group/entry indices across
zero/negative rows, scroll truncation/range, selection, reverse buyback refresh
and independently owned stale buffer rows. Ordered traces check normal price
re-evaluation, signed funds, capacity refusal, category-zero insertion no-ops,
different displayed/live buyback prices, repeated unaffordable attempts,
quantity re-reads, partial transfers and exact-zero removal. Sale callbacks
observe the distinct bag/direct-slot clear times, wrap and ten-node retention.
Player capacity cases retain raw supplied identity, hand contributions, negative
counts and wrapping totals. Price fixtures cover signed level/rarity inputs,
the first minimum test and the un-clamped second evaluation. The default uses
platform power calls; it does not establish original imported-math accuracy.

The panel now draws the actual current group's rows without creating a flat
stock projection. UI tests purchase zero-quantity stock, sell from a player bag,
switch to the refreshed buyback group and repurchase a live node. Original
widget art, control geometry, negative-count icon color, upstream selection
priority, periodic refresh, cached-pointer lifetime, recipe/interface work,
network reconciliation and native transaction/visual acceptance remain open.

This vendor checkpoint passes all 54 CTests and the production Player/controller
diagnostic. Fresh reward, injured-health, dedicated-server, save/reload and vendor
runs passed; all six captures were reviewed. Reward state remains level 2/-33 XP/
7 Platinum Coins; injured health remains binary32 50 with one visit. Reload
retains health 265, seed 123, nine tiles, 33,792 preview bytes and 170 edits.
The health run reports 154 world calls over 17 input frames, and the vendor run
117 calls over 17 frames, with maximum 20 ms pieces. These are diagnostic samples,
not a performance certification. Bright ground, negative XP display, physical
controller and original-client acceptance remain open.

CreaturePublicationTests uses the real delta codec with supplied resident lookup
and decoder boundaries. Independent mutex probes check both locked lookups,
unlocked decoding and locked post-assignment admission. A decoder-time resident
replacement verifies that final publication resolves the context key again,
copies the complete snapshot and retains fields absent from the payload. A
different wire identity does not redirect it. Zero/high context keys, all sixteen
mode bits, absent mode retention, strict malformed rejection, missing storage at
either lookup and released failure-path locks have bounded checks. Real loopback
NetTests also sets mode 0xA620, sends a name-only update retaining that word and
then publishes a zero word. These checks do not establish an autonomous native
mode producer, full receive/shutdown timing or original-server equivalence.

All 55 CTests pass for this server-publication checkpoint. The fresh production
Player/controller diagnostic and reward, injured-health, dedicated-server and
save/reload runs pass; all five captures were reviewed. Rewards retain level 2/
-33 XP/7 Platinum Coins and injured health remains binary32 50 with one visit.
Save/reload retains health 265, seed 123, nine tiles, 33,792 preview bytes and
170 edits. The health sample reports 137 world calls over 17 input frames with
maximum 20 ms pieces. Bright ground, negative XP display, physical-controller
and original-client acceptance remain open.

ProjectionTests checks exact main-angle/near/far constants, separate signed
dimension conversions before aspect division, sign-bit inversion including
signed zero, one supplied tangent call, narrowing before reciprocal, the complete
physical component layout and all unused positive zeros. Exact finite depth
anchors check the positive-view-Z denominator and zero-to-one relation. Matrix
fixtures distinguish successive sums from reassociation and separately rounded
products from fused cancellation. A public Camera call verifies the RH/Vulkan
adapter and supplied constructor path. Native main-camera input selection,
imported tangent accuracy, original view production and pixel acceptance remain
open; these tests do not turn the engine's existing input policy into parity.

All 56 CTests and the procedural GPU routing checks pass for this projection
checkpoint. The GPU probe checks source routing, growth, clearing, ten-slot
prefixes, timed/cached/sector lifecycle, body/equipment/yaw/action sources and
fallback; representative current/pending frames were reviewed. The production
Player/controller diagnostic and fresh reward, health, network and save/reload
runs also pass, with all five client captures reviewed. Rewards retain level 2/
-33 XP/7 Platinum Coins, injured health remains binary32 50 with one visit and
reload retains health 265, seed 123, nine tiles, 33,792 preview bytes and 170 edits.
The health sample reports 151 world calls over 17 input frames, with maximum
20 ms pieces. Bright ground, negative XP display, physical-controller and native
visual acceptance remain open.

VendorStockTests supplies deterministic draw streams to the three stock bodies.
It checks fixed-item order and four independent optional gates, complete
twenty-four/sixteen-draw nested selectors, saved original recipe category,
opposite general/specialized jewel parity, full versus reduced random fields,
five additional weapon discards and stable removal before a second ordinal pick.
Exact complete-stream counts include 95/725/124 general cases and
3300/1980/3330/2010 specialized cases. An upper-band fixture retains low-word
0x8243 after separately rounded endpoints; band zero still consumes its overridden
draw. Repeated insertion, eligible stacking, first-zero reuse, destination gaps,
other inventory members and every cleared payload cell have checks. The existing
town-generation fixture encounters three vendor descriptors and verifies owned
grouped stock with positive quantities and no flat shadow. These checks establish
bounded supplied-input behavior, not imported RNG state or full town parity.
All 57 CTests pass for this checkpoint. The player/controller diagnostic and
fresh reward, health, network and save/reload runs also pass, with their five
client captures reviewed: level 2/-33 XP/7 Platinum Coins, injured health at
binary32 50 with one visit (144 world calls over 17 input frames, maximum 20 ms
pieces) and reload retaining health 265, seed 123, nine tiles, 33,792 preview
bytes and 170 edits. An additional actual-client walking capture at the seed-123
town (cell 8397824, 8397824, radius 1, noon) exercises production town stock
and renders the plaza, character, HUD and map. Its first run reported 3,225
world calls and 106.3 s of generation while other local jobs were active. Run
back to back on an idle machine, the same executable reported 115 world calls
and 17.5 s, and the previously published executable 118 world calls and 16.8 s,
so the earlier figures reflect machine contention and not this change. These
are single local samples, not a performance benchmark or native comparison.

CameraTests supplies recorded trigonometry, power, clock, voxel and ray inputs
to the two camera kernels. It checks binary32-first fixed conversions and the
wrapped opposite, each row-pair rotation's saved-value mixing and
cosine-before-sine request, rounded products without fused cancellation,
fourth-row translation order, the count-fold weight against an independent
binary64 loop, both clock reads and the y-then-x shake results at zero and
nonzero amplitude, and decay. Construction cases count eleven cosine and nine
sine requests in order, check sign-bit inversion of zero angles, and use
coordinates near 2^40 to distinguish raw subtraction before conversion. Follow
cases cover mouse and wheel bounds including signed zero and wrapped doubling,
the scale route's zero-argument division, smoothing order, the strict snap,
quantized x/y steps with truncation in both directions, all five vertical
action cases, the binary32 raw blend at zero weight, both alternative-anchor
column outcomes, the negative voxel coordinate rule, corner order, original
maximum per ray, strict candidate retention, the unclamped negative candidate
and the final correction's negative, unordered, zero and zero-offset cases.
Production cases check the engine direction, first-frame placement, shake
displacement with decay afterward, and distance shortening under a ceiling
through the engine ray adapter.

All 58 CTests pass for this checkpoint. The player/controller diagnostic and
fresh reward, health, network and save/reload runs also pass, together with the
seed-123 town walking capture; all six client captures were reviewed and keep
their previous framing. Rewards retain level 2/-33 XP/7 Platinum Coins, injured
health remains binary32 50 with one visit (144 world calls over 17 input
frames, maximum 20 ms pieces), and reload retains health 265, seed 123, nine
tiles, 33,792 preview bytes and 170 edits. An earlier batch produced top-down
and from-below town and reload captures. The previously published executable
reproduced the same view under the same desktop conditions: the cursor
callback treated its first sample as a movement from the origin, so a cursor
resting over the new window became a large look input. The callback now uses
the first sample after window creation or a cursor-mode change only as its
reference. This is an engine input adapter correction, not a native mouse
claim. These checks establish supplied-input behavior and engine rendering;
native ray geometry, widget gates, the constructed matrices as the production
view and original visual acceptance remain open.

WorldDropTests drives the shared drop helper and the inventory-unit tail with
supplied draws and block callbacks. It checks the exact scale bits for factor
0.75, the landing remainder, the replacement angle, one draw and no append
without first residency, key coalescing without suppressed records, the
double truncation that keeps a small negative coordinate in zone zero while a
negative zone is absent, immediate scan termination outside the horizontal
range, the vertical start for positive, zero, fractional-negative and exact
negative heights, class masking through values with high bits set, and
field-wise item assignment with zero gaps. Paired runs from one seed isolate the
tail's draws: five per resident unit, four without residency, none for excluded
category-1 subtype-1 units, nonpositive quantities or category zero, and no
units for an actor byte other than one. All 59 CTests pass, and the
player/controller diagnostic, reward, health, network, save/reload and town
runs pass with their six client captures reviewed and unchanged in framing.
These supplied-input checks do not establish native column fallbacks, total
fatal draw counts or original drop placement.

ProjectionTests additionally checks the production lens: without a followed
creature the engine free-camera lens is retained; with one, the mode word's
0x0400 bit selects the two native angle bit patterns with the native near
plane, far 2000 and the separately converted pixel aspect; the main projection
equals the native constructor through the explicit adapter; and frame
constants carry the selected planes and the ordered product. CameraTests now
drives the production camera through the held ray query with supplied voxel
scenes. All 59 CTests pass. The player/controller diagnostic and the reward,
health, network, save/reload and town runs pass; all six client captures were
reviewed and show the wider 50-degree framing with the character, HUD, map and
terrain intact and the camera placed behind the character as before. These are
engine captures under the engine's view adapter, not native pixel comparisons.

CameraTests additionally compares the constructed view with the engine look-at
view for four angle sets at one eye: after mirroring the view x and z axes the
worst component difference stays below 1e-4, both directly and through the
production Camera adapter. Production cases check that shake translates the
constructed view by both half-amplitude components while the eye is unchanged,
and that a render origin changes only the horizontal translation input. All 59
CTests pass. With the constructed view in production, the player/controller
diagnostic and the reward, health, network, save/reload and town runs pass and
all six reviewed client captures match the preceding checkpoint's framing.

WorldDropTests additionally pins the held fatal-loot branch contracts' counts:
four draws for every grade call with the result at most four and the forced
bound, sixteen and twenty-four draws for the two generators with selector
minus one, their fixed categories with the supplied level and grade, one
parity draw plus one full generator run for the wrapped producer with its
category moved aside, and the level adjuster drawing only from level three
upward. All 59 CTests pass. Only tests and documents changed in this
checkpoint; the client executable is unchanged, so no new client runs were
made.

CameraTests now checks the constructed camera values and option defaults, the
first update's snap of the zero anchor to a creature far from the origin
followed by the start of the vertical blend, and settling at the head-height
target after many updates. All 59 CTests pass. The player/controller
diagnostic and the reward, health, network, save/reload and town runs pass;
all six client captures were reviewed and keep the preceding framing, using
the labelled direct-entry distance because these runs skip the front end.
The front-end rotation and creation snaps have no automated run here.
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

WorldDropTests also checks the pet-food subtype list against the held
world-list contract: 45 values in the held order with sum 2750. All 59 CTests
pass. Only a test and documents changed; the client executable is unchanged.

WorldgenTests additionally checks the held template-stamping corrections: all
five guard equipment blocks at level 20 for selector 9, creature yaw and crest
angle from the raw orientation 5, and a lone wall-plant voxel that draws and
clear-writes over a warm column but neither draws nor writes over an
unordered (NaN) column value. All 59 CTests pass. The player/controller
diagnostic and the reward, health, network, save/reload and town runs pass;
all six client captures were reviewed. Local runs now open from the native
local-entry camera (5 blocks, level, third angle 180) and connected runs from
the front-end distance of 10 blocks, so their framing differs from earlier
checkpoints by design. The menu return row and screen transitions have no
automated run.

WorldgenTests also checks the held column-value contract's edge rule: an index
outside 0 to 1023 in the three-by-three window yields one half. All 59 CTests
pass. Only a test and documents changed; the client executable is unchanged.

CameraTests checks that engine yaw and pitch formed from the native angles
reproduce the native view direction within 1e-5 for four angle pairs, including
first angle 180 and third angles beyond one turn. All 59 CTests pass; the
player/controller diagnostic and the reward, health, network, save/reload and
town runs pass, and all six reviewed captures match the preceding batch.

Live water-extension captures at seed 123 (noon, radius 1, walking client with
the local-entry camera), at points along the line from the start toward the
nearest generated sea region: (8402900, 8396150) and (8403600, 8396100) start
under the ocean and render the shared-shader underwater look (teal seabed,
deep-blue fog, kelp, visible surface beyond); (8402300, 8396200) is a coastal
hill above the same ocean; (8408064, 8395776) is a coastal town. These show
underwater rendering and coastal geometry in the real client. They do not show
bank reeds, because this coast is a steep slope into deep water rather than a
shallow bank, and seventeen-frame captures cannot show whether a submerged
player swims or rests on the seabed. Live swimming and reed placement therefore
remain unverified in the client; WaterTests still covers them with synthetic
terrain and the real movement code.

WorldDropTests additionally pins the held fatal-branch table of generator draws
per recipient class: 1, 6, 4, 5 and 4 draws for the weapon generator and 5, 9,
10, 10 and 10 for the armour generator for classes 0 to 4, with an empty weapon
result for class 0. All 59 CTests pass; only a test and documents changed.

CreatureTests' feeding fixture now also checks that the feeding branch records
the feeder as the creature's contact, per the held created-actor combat
contract. All 59 CTests pass; the player/controller diagnostic and the reward,
health, network, save/reload and town runs pass, and all six reviewed captures
match the preceding batch. The other combat corrections have no dedicated test.

The two weapon light sites now read the main-hand and off-hand weapon points
from the previous pose refresh, cached per creature, instead of the main-position
fallback. This reproduces the held contract's one-refresh-stale read.
CreatureLightTests' new weaponPointLights case checks the conversion from pose
trail tips to absolute fixed positions. All 59 CTests pass; the player/controller
diagnostic and the reward, health, network, save/reload and town runs pass, and
all six reviewed captures match the preceding batch. The weapon matrices still
come from the engine pose producer; term-by-term equivalence with the native pose
chain is not verified.
Water extension on generated terrain: the new WaterLiveTests generates seed 123
with and without the extension. At the home region's first lake location
(centre 8389628, 8401925), the native water surfaces gain swimming depth, eleven
bank reeds stand in air beside surface water, and a player-sized body in the
real movement step stays in the swimming state for 150 of 150 steps without
standing. The nearest home-area zone with native walkable wet ground (zone
32801, 32798, a stream bed) converts 3532 wet surfaces to three-deep liquid,
receives 48 bank reeds, and a body there floats at the surface (154.35 to 154.44)
without ground contact. Live client captures at the stream bank (8397224,
8396332) show cattail reeds beside the shared water surface with lily pads. A
capture at the swimming spot (8397217, 8396312) puts the camera just under the
surface, showing a flat saturated-blue underside without a visible player model.
That view is unreviewed against any intended look. The client's own swim state
was not logged. All 60 CTests pass.
Water extension floors: the earlier swim-spot capture's flat saturated-blue
underside was the underwater sky colour seen through missing floor faces. When
the extension deepens water through a column's lowest stored cell, or water
already rests on the implicit cell below a column, the mesher draws no floor,
because implicit cells have no faces. makeWaterSwimmable now stores that floor
cell, using the terrain's gap block, under every water column it handles. The
native profile is unchanged. WaterTests covers a thin column, checking for a
stored bed and a drawn top face. WaterLiveTests now requires no water over an
undrawn floor: it finds 0 at the lake and stream sites, where the native
generator has 0 and 4879. Live recaptures at the stream (Omega and Alpha looks)
show a lit, fogged floor under the water. From a spot 4 blocks along, the
swimming player is visible, tinted by the water. Captures now print the eye and
player positions, contact and health. In the running client the swimming player
reports contact 0x2 (in water), eye 0.33 below the surface, health unchanged. A
deep-lake start point (8389378, 8401922) killed the player in under a second, but
the cause was three creature hits (108 to 140 each) from one attacker in the
lake location, not the water. All 60 CTests pass, the player/controller
diagnostic and the reward, health, network, save/reload and town runs pass, and
all six reviewed captures match the preceding batch. Whether the original
mesher draws floors under natively generated water on implicit cells is
unverified.
Created-actor combat: CreatureTests gains two cases. In the first, a creature
fed by a player, with five bites, no taming progress and two resident kind-7
records, emits exactly two record events naming itself twice. The first is the
last kind-7 record with zero value and time; the second a kind-8 record of
20000 ms from the feeder. The creature ends pacified. In the second, an
owned-flag creature with owner identifier zero still starts feeding (action
110). The fixture keeps the action clock under the selection prefix's 10000 ms
reset. The magic beam aim offset has no dedicated test. All 60 CTests pass,
the player/controller diagnostic and the reward, health, network, save/reload
and town runs pass, and all six reviewed captures match the preceding batch.
Weapon pose chain, term by term against the held weapon-attachment pose
contract (2026-11-12). The pose code's rotations now use the contract's
primitives: binary32 degree-to-radian scaling, binary64 cosine and sine
narrowed, single-axis rotations from the pre-update columns, and axis-angle
rotations with a binary64-rooted axis length. The arm swing is formed in
binary64 with narrowed sine arguments. The dodge roll axis needs a squared
length above 0.01. A carried main-hand weapon is centred and turned -135
degrees about y (a staff -45), so blades hang tip down; carried weapons no
longer except model 840. Bracelet scaling is applied as two scalings, the
off-hand fist turns after its scale, and selector 0x47 holds the off-hand item
at the hands' midpoint 8 higher. A CreatureTests case checks the carried
sword and staff directions at 45 degrees. All 60 CTests pass, the player
diagnostic and the five live runs pass, and the six captures show the sword
hilt-up over the shoulder.

Cast points (2026-11-12), from the held `alpha-combat-cast-points.md`. Bodies
carry a fixed-point follow point, zero at construction, set to the position
after each live creature's step (a carried pet takes its owner's follow point
plus the binary32 height offset, its position likewise in fixed point) and,
as an engine adapter for their place in the actor order, after the update for
live players. During actions 0x1E-0x21 a magic user copies its target's follow
point into its cast point while (clock - step) is at most the anticipation
time, and aims at the held cast point afterwards. The type-118 beam (0x5F)
aims from routine 0x046BB0's offset point whenever that action is current. A
CreatureTests case latches a displaced follow point and checks the held aim.
All 60 CTests pass, the player diagnostic and five live runs pass, and the six
captures match the preceding batch.

Aim triplet (2026-11-12), from the held `alpha-combat-aim-triplet.md`. Creature
shots, beams and close-spell placement now read the creature's own aim (record
field 26) as the behaviour and the per-update path follower left it, in place
of the engine adapter that aimed at a player target's current position. The
path follower's tail-cell vector is that aim (section 2.3 step 6), and beams
start at routine 0x046BB0's sight point. CreatureTests checks that a magic
user's shots fly along its aim while the player sidesteps between behaviour
updates; RouteFollowingTests now reads the path aim from the aim itself. All
60 CTests pass, the player diagnostic and five live runs pass, and the six
captures match the preceding batch.

Far-route approach and path (2026-11-12), from the held
`alpha-combat-approach-path.md` with `alpha-created-actor-combat.md` section
5.2. The existing search core, rebuild and direct walk already matched the
contract; the combat far route now calls them in the contract's order. A new
CreatureTests case lets a melee creature target the player, raises a wall that
hides the player, and checks that the far route requests a path with an arrival
radius of both widths and a goal lowered by half the target's height less 0.1.
After the wall is removed, the direct walk succeeds and the search is cleared.
All 60 CTests pass, the player diagnostic and five live runs pass, and the six
captures match the preceding batch.

Facing and drawn aim (2026-11-12), from the held `alpha-combat-aim-triplet.md`
sections 2.2 and 3.7 to 3.9. A new CreatureTests case turns an aiming creature
60 degrees off its drawn aim. One 16 ms update leaves a gap between 5 and 20
degrees: it turns most of the way, and does not snap. Within 60 more updates it
faces the aim to within 2 degrees. The pets' companion now uses the held direct
walk, and the removed clear-route adapter's test went with it; the direct walk
keeps its own RouteGeometryTests coverage. All 60 CTests pass, the player
diagnostic and five live runs pass, and the six captures match the preceding
batch.

Held Companion for pets and followers (2026-11-12). Pets and behaviour-3
followers now run the held companion routine. The existing pet tests still
pass: the pet follows to within 4 blocks, comes when called and returns after
20 s. A new CreatureTests case puts a five-block wall into the zone's columns
between a pet and its owner. The pet then starts a search toward the owner with
arrival radius 2 and a goal at the owner's x. All 60 CTests pass, the player
diagnostic and five live runs pass, and the six captures match the preceding
batch.

Beam pulses, boomerang and the pet goal (2026-11-12). A new CreatureTests case
sets a creature's beam clock directly and checks four updates:
- the update that crosses the wind-up pulses (the earlier engine missed it);
- an update inside the same 200 ms period does not pulse;
- the next 200 ms boundary pulses;
- nothing pulses once the active window is over.

The walled-pet case now also checks that the search goal lies exactly one block
below the owner and keeps its fraction. The old pet code would have failed
that, because it snapped the goal to a block floor. All 60 CTests pass, the
player diagnostic and five live runs pass, and the six captures match the
preceding batch.

Mod SDK v10, the server's mod events (2026-11-12). ModTests' probe plugin now
receives a join, a leave and a taming. From the join it calls `getPlayerName`
(full length, a terminated cut-down copy, -1 for an unknown id) and `tell`, and
both reach the host. NetTests gives its live server mod events. The two joins
arrive once each, frames run with the world steps, and Alice's "hi" reaches the
chat event. That event's handler then calls back into the server (her name, the
creature list, a tell), and the told line reaches her client from id 0.
All 60 CTests pass, the player diagnostic and five live runs pass, and the six captures match the preceding batch.

Controller aim and flags (2026-11-12). New CreatureTests cases cover the flags block:
- idle under 300 ms sets the face-aim bit and walking, and past 300 ms the bits
  follow the keys;
- Left Shift and the excluded and sustained actions;
- action 0x58 walks without facing the aim;
- Left Ctrl, and the gliding lift.

They also cover the aim:
- free aim with and without Left Shift;
- the pick, with a view along +y: the creature on the screen centre is hovered
  and moves the aim to its depth, while those behind or aside do not;
- a friendly creature is hovered but keeps the ray point;
- a creature behind the player does not move the aim;
- the close-spell freeze.

The player diagnostic now expects the face-aim bit at idle clock 0. All 60
CTests pass, the player diagnostic and five live runs pass, and the six
captures match the preceding batch.

Beam mana and order (2026-11-12). The beam CreatureTests case now gives its
caster mana, and checks these cases:
- the first crossing of 0x5F leaves at most 0.7 mana;
- a first crossing that cannot pay loses its pulse;
- with mana at zero, the 200 ms repeats still pulse;
- action 0x1C with no mana stops, and with mana it pulses.

The ActionPhaseTests ordering fixture now gives its ray actor mana. All 60
CTests pass, the player diagnostic and five live runs pass, and the six
captures match the preceding batch.

Loot draw order (2026-11-12). New CreatureTests cases check that generator P
(weapons) makes 16 draws and generator Q (armour) makes 24 with selector -1.
They also check that a boss's worn-item stage makes its r mod 4 draw: eight
draws in all for the fixture, where the old code made seven. All 60 CTests
pass, the player diagnostic and five live runs pass, and the six captures
match the preceding batch.

Timing tables (2026-11-12). A new data-driven CreatureTests case checks B, C
and D for every action 0..0x69 against the held stat-queries tables, with the
speed scale doubled so that fixed and scaled entries differ. It also checks
B for action 0x25 with subtype 12 in the second hand. All 106 actions match.
All 60 CTests pass, the player diagnostic and five live runs pass, and the
six captures match the preceding batch.

Head pitch, pick reach and the healing stream (2026-11-12). New CreatureTests
cases check:
- the daylight factor at noon and at midnight;
- the pick's reach in light, in darkness and in between;
- the heal-target write before the wind-up end, with Left Shift, and after;
- a player's healing-stream pulse heals a wounded townsperson and pays the
  action cost, and a second update inside the same period does not heal.

FatalContextTests now checks that record +0x190 round-trips as the heal
target. All 60 CTests pass, the player diagnostic and five live runs pass,
and the six captures match the preceding batch.

Attack routine (2026-11-12). Under the contract, a magic hit's presentation
makes no draw, so these tests now expect one draw fewer per spell hit:
PeriodicHitTests' flash case and RangedSchedulingTests' contact, impact,
producer-probability, water and mode-zero cases. CreatureTests now expects
identifier 5 for an unarmed impact, where it used to expect the inferred 7.
All 60 CTests pass, the player diagnostic and five live runs pass, and the six
captures match the preceding batch.

Local player facing (2026-11-12). The shared facing block keeps the existing
NPC facing tests. A first live run showed the player turned 45 degrees toward
the world origin. A temporary debug print traced this to the camera's zero
anchor on the first update. The adapter that holds the aim until the camera is
placed fixes it, and the captures again match the preceding batch. All 60
CTests pass, and the player diagnostic and five live runs pass.

Bomb blast (2026-11-12). The existing bomb CreatureTests (fuse, one blast,
hit and push) pass with the contract's reach tests, victim order and damage.
All 60 CTests pass, the player diagnostic and five live runs pass, and the six
captures match the preceding batch.

Blast removal-sound draw (2026-11-13). A new CreatureTests case runs one bomb
with a shared generator. With the block query answering yes, the explosion's
kept pitch equals the draw taken right after the query, before any victim. With
it answering no, there is no pitch and no draw. Each blast asks once, at the
bomb. All 60 CTests pass, the player diagnostic and five live runs pass, and
the six captures match the preceding batch.
Strike call sites (2026-11-13). New CreatureTests cases cover the periodic window and its
area-heal flag, the deferred-key pass order, the strike centre (forward offset and the 0x45
lowering), the post-hit swing count and mana draws (none after the frame's first success,
none for a charged blow, whose charge feeds the crit charge), the area-heal record's two
draws and amount, and the beam's single S4 draw. All 60 CTests pass, the player diagnostic
and five live runs pass, and the six captures match the preceding batch.
Trap ticks (2026-11-13). The spike-trap CreatureTests case now runs 25 frames of 16 ms after
activation and expects exactly two passes (the 200 ms world-clock quotient), the player hurt
20 at level 5 and thrown 20 away, and the role-1 creature spared. All 60 CTests pass, the
player diagnostic and five live runs pass, and the six captures match the preceding batch.Drawn-height offset (2026-11-13). A CreatureTests case walks a body up and off a one-block
ledge: the step-up lands 1.01 above the bottom's layer (within the frame's later gravity
sub-steps), the step-down exactly on its fixed target, the drawn height stays within the
frame's decay of its previous value, and the decay is bit-exact for both rates and does
nothing at 0 ms. New cases check the shield bar entry 8 and its release to 104 at clock 0,
and a beam record's 35 draws in particle order. All 60 CTests pass, the player diagnostic and
five live runs pass, and the six captures match the preceding batch.
Trap attack routine (2026-11-13). The spike-trap CreatureTests case now resolves each hit
through the attackerless attack routine: one discarded draw per hit, a main record with no
attacker, the 0.5 flash, the type-7 armour selector, and a damage-taken buff of 0.5 halving
a 20 hit to 10. All 60 CTests pass, the player diagnostic and five live runs pass, and the
six captures match the preceding batch.
Hostility test (2026-11-13). A CreatureTests case checks each rule of the relation: identity
(even for role 6), role 6 against everyone, exactly one role 1, role 5 against role 1,
non-1 roles at peace, an exempt role-1 animal against a monster and its loss of exemption
through appearance bit 11, the 0x20 mode bit on either side, and that health is not read.
All 60 CTests pass, the player diagnostic and five live runs pass, and the six captures
match the preceding batch.
Online strike order and area heal (2026-11-13). No new unit case (the path lives in the client
loop); the area-heal record and window flag keep their CreatureTests coverage. All 60 CTests
pass, the player diagnostic and five live runs (including the connected-client flow) pass, and
the six captures match the preceding batch.
Target relation (2026-11-13). New CreatureTests cases pin the relation's distinct rules: role 5
never against role 0 even toggled, the reverse falling to the toggle, role 6 never a target,
one role 1 in either order, and the same creature relating by its toggle only. Relationship
and target-selection tests now use the single exemption predicate. All 60 CTests pass, the
player diagnostic and five live runs pass, and the six captures match the preceding batch.
Strike block breaking (2026-11-14). A CreatureTests case drives a 5.6-wide striker through the
real dispatch: one block query per strike frame at radius 8 and the forward strike centre, a
crater with the pitch equal to the draw right after the query and the sound at the striker
when cells go, and no crater or draw otherwise. The blast test now checks radius 8. All 60
CTests pass, the player diagnostic and five live runs pass, and the six captures match the
preceding batch.
Strike cell helpers (2026-11-14). WorldgenTests: the 955-cell bomb crater and its records are
unchanged through `breakCells`; the first burst is at the unsnapped offset position in the
tested colour; raw z -65536 is tested at -2 and recorded at -1 by the strike, -2 by the
blast; raw z -32768 is recorded at 0 by the strike and -1 by the blast; radius 0 breaks
nothing. All 60 CTests pass, the player diagnostic and five live runs pass, and the six
captures match the preceding batch.
Airships (2026-11-14). CreatureTests: the landing-plot template's identifier, heading, home,
target and destination; no copy with the player 600 blocks away (outside the player zones),
a copy posed at the target with the player 200 blocks away; a box inside the hull collides,
takes the base position z - T(half height) and is carried; pure red voxels never collide; a
body dropped above the deck lands on it, stands, and is carried; a stationary ship leaves
its passenger in place and drops absent keys; the ship is dropped once every player is
beyond 512 blocks. All 60 CTests pass, the player diagnostic and five live runs pass, and the
six captures match the preceding batch (no landing place is in their views).
Airship steering (2026-11-14). CreatureTests: a body sunk 4.2 blocks into a hull is pushed out
through the nearer +x side on one axis only; a keyed creature is placed on the marker voxel a
hair above it and takes the ship's yaw (none without a ship); a ship at home moves from
course 1 to 2, then flies at 10 blocks a second toward the waypoint ahead and above home and
its target moves by velocity x seconds; a ship copied at its target yaw keeps it through the
yaw follow. All 60 CTests pass, the player diagnostic and five live runs pass, and the six
captures match the preceding batch.
In-range combat arithmetic (2026-11-14). No new unit case (the change replaces double
precision by the contract's binary32 forms in the same branches); the existing creature,
pet, combat and selection tests still pass. All 60 CTests pass, the player diagnostic and
five live runs pass, and the six captures match the preceding batch.
Combat helper tables (2026-11-14). CreatureTests pin the bracelet row of B (0x59 wind-up
halved), the two-hander chain 0x39 -> 0x43 -> 0x3A within R0 + 200 and its restart, the
special's parity (0x41 odd, 0x42 even, negative counts included), the 0.3 cost of 0x2E, and
the class (4, 1) roll starting through a lockout while any other special is stopped. All 60
CTests pass, the player diagnostic and five live runs pass, and the six captures match the
preceding batch.
Airship pilot (2026-11-14). A CreatureTests case runs a behaviour-8 pilot: it faces a player 4
blocks away without walking, and beyond 8 blocks stops facing and stays put. All 60 CTests
pass, the player diagnostic and five live runs pass, and the six captures match the preceding
batch.
Remote smoothing (2026-11-14). CreatureTests: the angle step reaches 90, folds 120 to 60, leaves
180 and halves 10 -> 40 at factor 0.5; a snap draws a received creature at its record, a
smoothing receipt leaves the drawn pose and one 16 ms update closes part of a 4-block gap, a
dead creature's drawn pose stays frozen, and unlisted creatures are forgotten. All 60 CTests
pass, the player diagnostic and five live runs (including the connected client) pass, and the
six captures match the preceding batch.
Pose selectors (2026-11-14). CreatureTests: selector 8 with an off-hand model, selector 1
clearing the amplitude, 0x13's strike offsets, 0x30 in phase 3, 0x46 after the strike, 0x47
writing model 2101 with the 1.5 off-hand scale, and the knockdown offset. All 60 CTests pass,
the player diagnostic and five live runs pass, and the six captures show the same scenes with
creatures and the player posed normally.Remote facing (2026-11-14). CreatureTests: a received creature keeps its received head pitch;
with the face-aim bit and action 0 one 16 ms update sets the drawn aim to the aim and turns
the yaw and head pitch toward it. All 60 CTests pass, the player diagnostic and five live
runs (including the connected client) pass, and the six captures match the preceding batch.Pose owner inputs (2026-11-14). CreatureTests: selector 0x30 takes its hand turn from owner
+0x148; selector 0x0F turns the creature angle on swing-count variant 2 and not on variant 1;
the swing count takes the combo through the wind-up and at a pulse boundary of action 0x1E,
not inside a pulse. All 60 CTests pass, the player diagnostic and five live runs pass, and the
six captures match the preceding batch.Provenance batches 1-2 (2026-11-14). New regressions: RelationshipTests (kind 89 with every
bit outside the mask), HitApplicationTests (NaN amount writes nothing; NaN and negative-zero
differences kept), LinkedItemTests (orphaned role 5: health 0, role 3, clock 5000, reverse 0),
ItemIdentityTests (zero hand count swaps; hand count 1 with category 20 swaps). All 60 CTests
pass, the player diagnostic and five live runs pass, and the six captures match the
preceding batch.Provenance batch 3b (2026-11-14). No behaviour change (comment only); all 60 CTests pass.
Provenance batch 3a (2026-11-14). No behaviour change; all 60 CTests pass.
Provenance batch 4 (2026-11-14). WorldgenTests: every cloud placed at home (seed 123) matches the fixed-point height composition. All 60 CTests pass, the player diagnostic and five live runs pass, and the six captures match the preceding batch (clouds unchanged to the eye).
Provenance batch 5 (2026-11-14). WorldgenTests: column writer cases (zero-filled extension, no-op, water substitution, kept cells, downward gap fill) and formation guards (mesa over read water, mound skipping water, mesa over rock). NetTests: x10 and x100 day-clock rates, rollover under acceleration, record action byte. All 60 CTests pass, the player diagnostic and five live runs pass, and the six captures match the preceding batch.
Provenance batch 6b (2026-11-14). CreatureTests: a corner gives wall normal (-1, -1) and the normal resets without a wall; action 54 before P1 + P2 keeps more ground velocity than action 0. The input rewrite needs a window and is covered by the build and the player/controller diagnostic. All 60 CTests pass, the diagnostic and five live runs pass, and the six captures match the preceding batch.
Provenance batch 6a and transport (2026-11-14). WorldgenTests: every seed-123 offset group, float N in [-1, 1], Plant flags default 2, crown size and height grouping over a grid, density cap before bonus. NetTests: pacing alternation and clock wrap, truncating zone index for negative positions, partial inflate (whole, cut, bad header). All 60 CTests pass, the player diagnostic and five live runs (including the connected client) pass, and the captures match the preceding batch.
Provenance network server and saves (2026-11-14). NetTests: strict day wrap and 10,000 ms save boundary, next-zone order and limits, binary32 200-block equality, join record, a real-server zone record with edits saved after unload or shutdown. SaveTests (scratch files only): tolerant character reader, single-key add/delete/save keeping a damaged record's index, native world deletion, damaged world entry kept in place, session clock at exactly 86,400,000. All 60 CTests pass, the player diagnostic and five live runs (including connected client and save/reload) pass.
Provenance open points 7 and 8 (2026-11-14). No behaviour change (comments only); all 60 CTests pass.
Provenance regions and item generator (2026-11-14). WorldgenTests: every sea cell in a 276-cell block is exactly -100 and cell points are slot-centred; the low-edge two-index window and its blend; location lookup at negative columns; armour draws D15-D19 and a negative-argument rarity. All 60 CTests pass.
Provenance projectiles, world items, open points 1 and 2 (2026-11-15). RangedSchedulingTests: weapon set, healer burst for 94 at the ray end, rock element, dodge in the evaded set, still knock-back, 4-block burst hit, boomerang catch. CreatureTests: beam tick clears the evaded set, seat release on stun (remote keeps it), settling for tags 1/2 and vegetation, hop and tumble, mana-cube filter, static approach below one unit and face equalities, z statics after a cell hit. All 60 CTests pass, the player diagnostic and five live runs pass, and the captures match the preceding batch.
Provenance terrain, open points 5, tick ruling (2026-11-15). WorldgenTests: settling at z = -2.5 over water sinks to -3.5 and z = -2.0 stays; Zone::place dry air, z <= 0 water conversion and keep-water rules; placer marker 11 types, out-of-zone creature level, 0x40 suppression. All 60 CTests pass.
Provenance caves, spawn objects, loot rolls, open points 10, formation ruling (2026-11-15). WorldgenTests: full-circle pillar offsets for n = 6, 7, 8; terrace pool cell counts; centredIn truncation. CreatureTests: empty level-1 spawn items, a kept-gear spawn's 34 finishing draws, general-branch chest at spawn level with slot 8 kept, wide re-roll lo + d. All 60 CTests pass; the player diagnostic and five live runs pass on an idle machine (two earlier runs timed out while authors were building concurrently).
Provenance network packets, skills, knock-back/snap ruling (2026-11-15). NetTests: client packet fixture (batch create and snap, own id skipped, non-positive count deletes, packet 3, negative chat count, section 3 not a hit, item before hit), typed masks, duplicate region request answered. UiTests: skill tooltip punctuation, riding speed, sneak cooldown, cooldown values, skill points. IncomingHitTests: kinds 2, 4, 7 default path. All 60 CTests pass; the player diagnostic and five live runs pass (reward flow saves level 2, XP -13, Platinum 7).
Provenance travel, particles, crafting, open points 3, walk-flag ruling (2026-11-15). CreatureTests: wall crash lift and wind with stun 1500, ground crash without lift, blend weight over 16 ms, exact head-pitch decay and faceAfterStep order, climb-hold relax, exhausted climbing fall/hang, explosion window, staff spread and position, collision and light cells, cull radius and leaf reach, collision sub-steps, per-bag over-removal, saurian draw order. Frame order and stun gate need a window and are covered by the build and the player diagnostic. All 60 CTests pass; the player diagnostic and five live runs pass on an idle machine; captures match.
Provenance path search, locations, item names, trees (2026-11-15). CreatureTests: strict stop runs on to the goal, 501 open cells abandon with the goal cell as path, no rounds without a node map. WorldgenTests: out-of-range regions null, candidate ties keep slot order, climate minimum level carried by type-10 and unranked records and their zone entries, conifer top cells in a dry-air zone. ContentTests: tag fallback, '#' in both branches, missing word, negative coin quantity. All 60 CTests pass; the player diagnostic and five live runs pass.
Provenance server simulation and physics/camera (2026-11-15). CreatureTests: schedule persistence, inn rest, step-14 clean-up, threat stripping, silent kill; boxBlock, charge window, slam, diagonal and corner probes, separation push, lens/sitting, dodge cancel, action 81, climb/glider clears, equipment checks, kind 6 water, camera defaults and zoom. NetTests: healing hit with attacker 0. All 60 CTests pass; the player diagnostic (updated for the lantern check) and five live runs pass.
Camera/lens ruling and NPC physics (2026-11-15). CreatureTests: NPC intent ends action 83, skill fallback, equipment flag checks, NPC separation 0.08 blocks over 16 ms, NPC thud only with a local player; RelationshipTests fixture equips a lantern. All 60 CTests pass; the player diagnostic (unchanged) and five live runs pass.
Interaction, open points 4, item services, block edits, melee reach ruling (2026-11-15). UiTests: interaction targets (squared reach, creature prompt, free-pet claim, revive order). CreatureTests: occupant, chest drops three draws per unit, lever partner by descriptor index. All 60 CTests pass; the player diagnostic and five live runs pass.
Talk and vendors, open points 6, location creatures (2026-11-15). WorldgenTests: waypoint raised to water, flagged air, no column, negative-zone cell one block lower. ContentTests: shared-stream state after innkeeper, random, pet-food and explored talk. VendorTests: partner range, follow share, voice blips, CR tokenizer. CreatureTests: type-14 identification fee. All 60 CTests pass; the player diagnostic and five live runs pass.
Creature behaviour, beam flag and strike origin ruling (2026-11-16). CreatureTests: kind-11 critical boundary and draw count, player armour flag 0x20, spawn defaults, voiceless hurt draw, stun step (draw, length, z, sound, boundary), block gauge floor, beam area flags for 28/94/95, strike origin for 0x45/0x4B both signs, type-144 drain under controller 1, random walk timer/action/clock/intent. All 60 CTests pass; the player diagnostic and five live runs pass.
World generation structures (2026-11-16). WorldgenTests: dungeon ring creatures and escorts have radius 150 and ring facings 90/210/270/330. All 60 CTests pass (zone comparisons unchanged); the player diagnostic and five live runs pass.
UI screens, knock-back/taming/charge and corridor rulings (2026-11-16). UiTests: city location keys, item depth keeps camera and lens, hit text colour and rise. HitRecordTests: velocity bit-identical for zero, small, large and NaN. ActionPhaseTests: pending charge for 31, 33 and 1 around the crossing. WorldgenTests: all four corridor third-line cases. All 60 CTests pass; the player diagnostic and five live runs (including town render with the item shader change) pass.
Creature models, missions and rendering with four rulings (2026-11-16). CreatureTests: action 93, chest material 6, key-colour set, type flags, computed boxes, quadruped hand, draw order, shimmer slots, glow, glider model, dodge raise, poses 41/44/49/50, mana cube frame, footsteps. RegionProgressTests: group sizes, route draws and height, cell monster draws, pass region. LookTests: stars, fog target. TerrainLightTests, SectorLightTests, WaterTests, LightingTests: water light rows, item light limit, film quad shading, tints, trail, post-water row. All 60 CTests pass; the player diagnostic and five live runs pass.
Pets and bombs, character screens (2026-11-16). CreatureTests: five placement draws, chain order both directions, id-0 mount filter, kind-7 bite records summing to 1.0, pet return action time, controller-5 pet health, rebuild empties buffs. UiTests: easing share, pick skipped while held, Delete confirmation and withdrawal, release-only and press/release on different buttons, last character Create, widened names, online length 7, world index selection, signed seed, drag re-anchor, yaw rule. SaveTests: in-memory world deletion. All 60 CTests pass; the player diagnostic and five live runs pass.
Materials, wildlife and routes, held-light factor ruling (2026-11-16). CreatureLightTests: off hand 0.5, worked example 2 hands, infinite/NaN ratio gates, critical flash weapon selection and order. TerrainLightTests: averaged colour alpha, material colour for ordinary, gold, copper, fire and wind. CreatureTests: averaged alpha, unloaded route zone. WorldgenTests: companion ring bits including truncation. All 60 CTests pass; the player diagnostic and five live runs pass.
Trails/poses/schedules, Plasma, audio, extractor and six rulings (2026-11-16). CreatureTests: pose-69 arc bits, dt-0 trail rules, unarmed drift, alpha table, mission creatures, seats, ruin group draws and two-point route, exact ring point, SpawnLocation single round, swing crossings, attenuation, ambience draws and chunk gate, channel2 tick, staff release and explosion windows, action 104. UiTests: Plasma matrix, sequence start/evaluation/loop/stop, manacubebar 84 frames. StoreTests and OwnerContentTests: decode, synthetic .cub, store schema and empty stores, headers, reference entries. WorldgenTests: kept-zone disc and implicit blocks. All 60 CTests pass; the player diagnostic and five live runs pass.

## Death-pass timing (2026-11-17)

CreatureTests cover a 5 ms fuse followed by a 7 ms death pass before activation,
an incoming fatal hit at 0 ms, exact 1,000 ms expiry, signed-ID chain order, and pet
return at 20,000 ms before the owner's living turn. HitApplicationTests confirms
the published death clock advances while living action, stun and buff updates
stay inactive. FatalLootTests covers the linked actor's non-hit notification.
OwnerContentTests passes with the actual owned installation, extracts into OS
temporary storage, and checks the original file hashes.

The Windows x64 build and all 60 CTests pass. The player/controller diagnostic
and actual-client reward, injured-health cadence, network, save/reload and town
render flows pass. Town captures cover the same camera and player position across
the two engine checkpoints, with different simulation elapsed times and pixel
output. The town run establishes render completion. Native acceptance and pixel
parity remain open.

## Terrain light and fog reconciliation (2026-11-17)

TerrainLightTests cover the signed low-32 endpoint conversion, including a low
word that represents a negative column. Resident uploads retain full light
positions, apply horizontal translation and still publish ten of sixteen slots.
The Windows x64 build compiles the revised world shader. All 60 CTests pass,
including the live Vulkan water suite. The player/controller diagnostic and five
actual-client flows pass, including the town render. The town frame uses a
different elapsed world time than the prior frame. Native pixel parity remains open.

## Living update timings (2026-11-17)

ActionPhaseTests verify integer cooldown subtraction beyond binary32 unit
precision, zero-entry retention, mana gain at the updated duration boundary,
and buff 10 at and below anticipation and on action 36. ActorOrderTests verify
that a stunned actor retains the exact integer behavior time and that an inert
actor resets its accumulator on an admitted turn. SimulationFrameTests,
DescriptorRefreshTests and the signed actor tests continue to pass.

The Windows x64 build, all 60 CTests, the player/controller diagnostic and five
actual-client flows pass. Original-client RNG histories and runtime acceptance
remain separate verification work.

## Bomb terrain order (2026-11-17)

CreatureTests exercise two bombs that overlap one natural block. The first
removal is visible to the victim callback and the later living turn, and the
second blast produces no extra removal sound. The fixture also checks the raw
centre, primary and broken-cell burst order, and one dated edit. The existing
conditional pitch test checks the shared generator immediately at the terrain
boundary. The Windows x64 build, all 60 CTests, the player/controller diagnostic
and five actual-client smoke flows pass.

## Client input and lens meter (2026-11-17)

SimulationFrameTests cover action 34 at readiness equality, the caster's full
ID, preservation of the roll clock, release timing after the new action write,
and binding 99's mode and integer cooldown. ActionPhaseTests cover skill-scaled
growth, intent clearing before decay, and the buff-3 override before mana gain.
The Windows x64 build, all 60 test suites, the player/controller diagnostic and
five actual-client smoke flows pass. The initial network and zone comparisons
missed time limits. Both passed in standard isolated reruns, with NetTests at
17.61 seconds. Physical controller and original-client acceptance remain open.

## Dedicated packet-6 sender turns (2026-11-17)

ActorOrderTests reverses player storage order and verifies signed visitation,
admission after requests, deletion of a future NPC, and the next surviving
actor's turn. It checks fixed-point bomb placement, exact signed item level,
initial bomb health and the two exact NPC pitches around the bomb's five
appearance draws. CreatureTests checks per-unit chest draw order, stored
timer and angle and the two lever snapshots. The network loopback tests
ordered effects for interleaved heals, a rejected pickup and a drop.

The Windows x64 build, five focused suites and the full 60-suite CTest
run passed. Original-client runtime order and visual acceptance remain open.

## Client packet-6 frame boundaries (2026-11-17)

SimulationFrameTests verifies that the inventory callback's bomb reaches
the first of several world pieces. Later chest, drop and pickup inputs
stay queued until the next frame. The fixture checks their exact draw
count, the order of the sounds and gift, and the new item's landing timer.
CreatureTests verifies a resident's next-piece seat request and the connected
static gate. NetTests verifies the static snapshot preserves the sender's
pending queue and exercises the mixed dedicated request sequence.

The Windows x64 build and all 60 CTest suites passed. The initial focused
loopback run missed its placement deadline. It passed in isolation and in
the full suite. The full run included live Vulkan water and zone comparison.
Native client timing, graphics and physical controller acceptance remain open.

## Runestone releases and local links (2026-11-17)

UiTests verifies that the first release selects without moving the body and
that confirmation writes the fixed zone centre with Z zero, preserves
velocity and clears both map indices. CreatureTests verifies the dedicated
admission gate, equal-distance spatial order across zones, raw position
copy, the two-block lift, the fallback zone and the old-position sound.
The fixture checks that the local link appends no static snapshot and
spends zero random draws.

The Windows x64 build, all five focused suites and the full 60-suite CTest
run passed. Native portal projection and original-client runtime acceptance
remain open.

## Quick-item wheel timing (2026-11-17)

UiTests verifies signed ordinal wrapping across independent candidate counts,
neutral steering admission, repeat testing before elapsed subtraction, two
simultaneous directions, layout endpoints and stack decrement after its use
callback. SimulationFrameTests verifies the ordinary primary start gate and
the mixed queue ending in pet recall and wheel bomb. Its two bombs, chest,
drop and pickup consume exactly 18 draws.

The Windows x64 build, six focused suites and all 60 CTests passed. Original
controller hardware, native input and native visual acceptance remain open.

## Dedicated blueprint placement replay (2026-11-17)

NetTests places a rotated slab with a hollow interior. It verifies the
protected air in the authoritative world and checks the exact traversal
order, coordinates and bytes of all 500 solid records. A second peer
requests the loaded zone once and receives the same records in order.
The initial replay fixture missed its deadline on the placing client.
The final loopback fixture passed with a single request from the second peer.
The Windows x64 build and all 60 CTest suites passed, including the live
Vulkan water and owner-content zone comparison suites.

## Timed consumables (2026-11-17)

ActionPhaseTests checks the local and authority gates, signed elapsed
boundaries, the complete heal payload and the health cap. It checks that
recovery sound 47 precedes sound 44 and that rounding can suppress 47.
The resident fixture verifies one reported heal at the 10,000 ms food
endpoint, the next-step transition to sitting and zero random draws.
The signed-level fixture uses the inventory refusal and quick-use paths
on the same stack and verifies callback-before-decrement.

UiTests checks the fixed recipient-height adjustment, displayed amount,
colour and threshold. SimulationFrameTests retains the mixed-request
order and its 18-draw witness. All seven focused suites passed on the
Windows x64 build.
All 60 CTest suites passed, including network loopback, live Vulkan
water and owner-content zone comparison.

## Melee terrain before target resolution (2026-11-17)

ActorOrderTests runs two overlapping strikes through the live signed actor
walk. The first emits the dated block record and burst, and the second
reads the changed cell. The fixture checks the sound position and pitch
and that the entire pair consumes one removal-sound draw. WorldgenTests
verifies the negative-height water write affects the following probe and
checks the horizontal column boundary. CreatureTests checks both strike
and blast rounding through the shared removal service.

The Windows x64 build, all six focused suites and all 60 CTest suites
passed. Exact native relight and plant pass footprints remain open.

## Placed-object snapshots and zone requests (2026-11-17)

NetTests checks that the receiver retains repeated zone keys, empty lists
and signed placement coordinates. Its held-zone fixture checks replacement,
invalid static indices, terrain-only copies and unloading. Clock tests cover
the strict 1,000 ms threshold, timestamp advancement and signed wrap.
A scripted peer observes two copies of the same live request lists, with
all zone pairs before the region pair in each turn.

Two real peers receive the placed-object list from a dedicated server. The
second peer requests two resident zones, with one repeated and one
out-of-range request interleaved. The replies preserve the resident keys'
exact sequence and replay all 500 stored solid edits in order. The request
emits no section-8 entry. Each copy of a repeated region request supplies
its 64 records.

The Windows x64 build, seven focused suites and all 60 CTest suites passed.
Original-client interoperation and the stock blueprint controls remain open.

## Riding terrain and attachment order (2026-11-17)

ActorOrderTests puts the pet before its rider in signed actor order. The
fixture checks two different piece durations, the full Y span of the first
solid X column, one increment in water, the cap, and zero random draws for
the dry refill. It observes the previous rider position at the first
attachment and the updated position, orientation and landing bob at the
second. A later dismount prevents the second copy.

CreatureTests checks the riding tail after a missing pet, the speed
available after a zero-factor cancellation, and the stationary meter clamp
to positive zero. The first revision refreshed a caller-supplied cast
point before its consumer. The follow-point refresh now occurs after the
caller-advanced actor sweep. The cast-point tests passed on the rerun.

The Windows x64 build, six focused suites and all 60 CTest suites passed,
including network loopback, live Vulkan water and owner-content zone
comparison. Connected pet rendering and native hardware acceptance remain open.

## Connected actor motion and riding (2026-11-17)

ActorOrderTests checks that a received negative-key pet refills and attaches
before the controlled callback, and that the late pass reads the owner's
updated position, rotation and bob. It covers the late copy for a dead pet,
a dismount between passes, a retained smoothing flag and an owner at key
zero whose record and drawn poses differ.

CreatureTests checks snap, interpolation, the frozen unowned dead pose
and facing with the new actor call path. The Windows x64 build and all seven
focused suites passed. The main specification was parsed and both new links
to held contracts resolve.

All 60 CTest suites passed, including dedicated loopback, live Vulkan water
and the owner-content zone comparison. Original-client interoperation and
renderer walk-phase acceptance remain open.

## Creature receipt publication (2026-11-17)

NetTests sends a snap followed by a smooth receipt and verifies each
position and rotation at the frame boundary. It removes the creature
and creates it again at the same key before the frame drains its
receipts. Its pose check uses the recreated snap, the recreated initial
aim and the newest live record.

A five-entry packet-1 fixture interleaves three keys and repeats two
of them. It verifies field retention, one commit per distinct signed
key and a snap decision against the final merged field. The subsequent
packet 2 keeps the key previously seen in packet 0 and prunes the two
keys supplied by packet 1 alone.

The Windows x64 build, seven focused suites and all 60 CTest suites
passed. The HTML specification parsed and all three added contract links
resolve. Exact live-field baselines between locks and original-client
interoperation remain open.

## Live movement baselines (2026-11-17)

NetTests advances a received actor, publishes it under the frame gate,
then receives a health-only packet 0 and a snap-field-only packet 1.
It checks the live position against the distinct drawn position, compares
the six preserved fields byte for byte and verifies the partial batch's
exact snap position and rotation. It also compares the record bytes
outside the motion fields immediately after frame publication.

The older blueprint replay fixture missed its adjacent-zone deadline on
the first run. The spectator now holds the required zone through the
order check after witnessing the distant broadcast. The final Windows x64
build, six focused suites and all 60 CTest suites passed. Revision 4 of the
HTML specification parsed and the three added contract links resolve.

## Connected common timers (2026-11-17)

ActorOrderTests checks the earlier signed actor's state from the controlled
turn, the surviving and expiring type-1 buff boundaries, the positive-lock
clock override, role 6, a negative clock and a second world piece. It also
checks that a frame without a receipt keeps resident state. RemoteFighterTests
checks equal-clock receipt replacement, item gaps and retained actor-local
state. NetTests checks signed clock precision and the partial packet-0 and
packet-1 baselines, including byte equality outside the modeled fields.

The older drop fixture now waits for Alice's destination zone to be held and
selects a living actor with a spawn identifier before testing contact. The
Windows x64 build, all nine focused suites and all 60 CTest suites passed.
Revision 5 of the HTML specification parsed and all 25 links resolved.
The action-specific effects and full original-client timing remain open.

## Connected resource and movement fields (2026-11-17)

ActorOrderTests exercises shield blocking drain, the surviving type-3
meter before type-10 charge, roll cancellation on the last millisecond,
the assassin charge exception, intent-driven sitting cancellation, and
late stun. A zero-duration piece still applies resource ceilings.
RemoteFighterTests checks an exact signed roll value beyond binary32 integer
precision. NetTests checks the additional contact, mode and resource baselines
and byte equality outside the modeled fields.

The Windows x64 build, all nine focused suites, and all 60 CTest suites
passed. Revision 6 parsed and all 26 links resolved. The isolated original
Alpha process did not expose a game window, so native menu, gameplay and
hardware acceptance remain open.

## Connected walk and movement cues (2026-11-17)

ActorOrderTests observes a negative-key actor's landing and roll before
the controlled turn, followed by a ground footfall and a splash from
later actors. It verifies three draws, the blocked landing's exact
position and pitch, the splash visual's low fixed-point bits, and the
phase crossing before the footfall. It also checks the old roll gate
for facing and the copied phases in both pet attachment passes.
CreatureTests and ActorOrderTests cover exact negative support sampling.

The Windows x64 build, nine focused suites and all 60 CTest suites passed.
Revision 7 parsed and all 27 links resolved. Native playback and the
ordinary creature link remain open.

## Connected common preparation and action audio (2026-11-17)

ActorOrderTests checks resource debits before stun interruption in a
negative-key actor, the class-four roll and type-11 replacement, and the
type-9 replacement's strict lower boundary. The same fixture checks the
200 ms sound bucket, deterministic staff and explosion releases, exact
fixed visual positions, and six draws in order across the admitted actions.
The explosion record is present before its shared swing draw.

The Windows x64 build, eight focused suites and all 60 CTest suites passed.
Revision 8 parsed, all 30 links resolved and the three held hashes matched.
Native candidate selection, stored-candidate feedback and original playback
remain open.

## Original Alpha server loopback (2026-11-17)

An isolated copy of the supported original Server.exe was launched with
seed 41 and a fresh save directory. Its console logged client version 3
and a player join. AlphaVulkan reported the connection and adopted seed
41. The captured Vulkan frame showed the Lands of Gami world, player,
HUD and minimap. This test covers original-server startup, the client
handshake and a rendered connected frame. Original-client gameplay,
sustained combat and matched visual acceptance remain open.

## Connected sneaking light (2026-11-17)

ActorOrderTests checks daylight, the stored terrain cell, flagged placements,
streetlights, candles and live signed-order lanterns. It verifies the zone
subtotal cap, the extra light from a dead lantern carrier, the clear by an
earlier actor, a live section-six replacement and unchanged zone membership
when the controlled actor moves mid-piece. The meter calculation reads full
velocity and feeds the same turn's mana gain. The Windows x64 build, nine
focused suites and all 60 CTest suites passed.

## Original dedicated clock interchange (2026-11-17)

The engine's save reader opened the original server's isolated world
database and read day 0 and clock 43,299,980 ms. The engine wrote day 5 and
36,000,000 ms to the clock record in a separate copy. The original server
opened that copy and advertised day 5 and 36,009,230 ms over loopback.
The input database hash was unchanged. This covers the dedicated world's
clock record in both directions. Interactive character, zone and map save
interchange remain open.

## Connected action-1 melee production (2026-11-17)

ActorOrderTests exercises the signed creature walk with local rejection,
protection, guard, ward and ordinary results. It checks the exact order of
16 draws through damage variation, defensive pitch, critical voice and stun.
The local target consumes caller rolls and occupies a struck slot before the
helper rejects it. An accepted first hit increments the combo without a mana
reward draw or local application of target damage. The fixture also checks
the saved window after rescaling, counter expiry at its upper boundary,
idle cleanup and the updated packet baseline.

VisibilityTests exercises the resident object's position, rotation, shape
and state from a section-six record and verifies that a moved closed window
no longer occludes its old position. The Windows x64 build, eight focused
suites and all 60 CTest suites passed. Revision 10 parsed and all 33 links
resolved. The held candidate and cleanup contracts matched their recorded
hashes. Connected output presentation, action-specific candidates beyond
action 1, and original-client combat comparisons remain open.

## Connected hit receipt and presentation (2026-11-17)

IncomingHitTests verifies the full local-target pass runs before the
foreign-source tail is appended. Self echoes affect local state without a
second presentation record. Ward events retain the embedded source, and
foreign record order and opaque bytes survive the merge. The production
Client transport fixture checks exact 64-bit filtering, two hit packets in
frame order, the earlier local-state packet, and the later buff packet.

UiTests checks the label's raw fixed position, live height, local-target
offset, colour and motion. CreatureLightTests checks weapon order, the
resource and auxiliary bytes, exact positions and the 400 ms light. A
separate isolated loopback run sent the Vulkan client a nonlocal record,
a local echo for 49 damage, a missing-source heal of 7, another 4 damage,
and a counter. The local save ended at exactly 4 HP from 50. Processing the
two damage amounts before the heal would leave zero health.

The Windows x64 build, nine focused suites and all 60 CTests passed.
The initial full run exposed a `CreatureTests` stack overflow. The three
ordered bomb and melee-edit fixtures now run in a separate stack frame
and the test passed 30 consecutive runs. Revision 11 parsed and all 34
links resolved. Native hit counters, capture-signal details and matched
original hit presentation remain open.

## Ordinary creature link (2026-11-17)

ActorOrderTests verifies that the connected and offline client paths resolve
key zero. A received actor before the controlled turn copies the earlier live
pose, and an actor after it copies the new pose. The checks cover raw fixed
coordinates, the lift from target bob, angles, cleared movement, copied and
relaxed walk, receipt preservation, missing targets, role five and dead-target
clearing. A zero-key self match with unordered health supplies its completed
live state to the next actor.

The Windows x64 build, nine focused suites and all 60 CTests passed.
Revision 12 parsed and all 35 links resolved. The isolated original server
accepted this build with protocol version 3, assigned player ID 1 and supplied
seed 41. The captured Vulkan frame showed the Lands of Gami world, player,
HUD and minimap. Runtime comparison of ordinary link behavior with an original
client remains open.

## Connected melee actions 2–7 (2026-11-17)

ActorOrderTests checks the ordinary upper bounds, fixed 0.1 cost at zero
for actions 3 and 4, and hit amount, impulse and draw counts for the
remaining ordinary strikes. Action 5 checks the type-11 replacement, half
pending debit, candidate clear before deferred feedback, strict lower
bound and latched critical. Its accepted target uses the jitter and impact
pitch draws. The constant impulse skips the weapon speed query. The nine
focused suites and all 60 CTests passed on Windows x64.

The visibility re-certification covers the fixed conversions and side
faces. Current synthetic ray and connected object-adapter checks passed.
An isolated mod-launcher run started the original 32-bit client and
loaded Direct3D 9, DirectInput and the Visual C++ runtime. It exposed no
targetable game window. The desktop launch produced the same window
outcome. Original-client interactive comparison is still open.

## Offline queued blueprint placement (2026-11-17)

WorldgenTests checks the exact ordered colored records, signed truncation
at world zero, live protection, keep bits, interior air, positional gap fill
and unloaded columns. In the admitted zones the fixture removes unsupported
flagged sprites and terrain plants. The retained lily uses its submerged
fixed height, and the retained reed has its ground-resting flag clear.
NetTests checks separate signed model and rotation bytes at receipt and
serialization, and exercises the existing dedicated placement path.

The production WorldView diagnostic used the owner's wooden floor model.
It verified 600 live cells and matching color records at a signed rotation of
minus three quarter turns. The repeated stamp produced an empty colored
history and a second anchor entry. The remesh completed and the Vulkan
capture showed the placed floor in the generated terrain. The Windows x64
build, nine focused suites and all 60 CTest suites passed. Stock input
activation, native preview and the placement save boundary remain open.

## Connected melee actions 9–21 (2026-11-17)

ActorOrderTests checks all thirteen received actions at their saved strike
windows, including the zero base amount and constant impulse of action 11,
the skill-based action-21 impulse, and the full and half-resource routes.
One fixture observes the action-11 candidate sets before the pulse pitch
draw, then checks critical, damage variation and impact pitch in sequence.
The startup fixture checks the roll before counter feedback, followed by the
type-11 replacement and pending debit. The action-17 fixture sees the new
buff at the end of the resident list before the target's own turn. The
target ages the new entry and keeps its health under connected authority.

The Windows x64 build, nine focused suites and all 60 CTest suites passed.
Revision 15 parsed
and all 44 links resolved. The held contract's full hash and line count
matched. Original-client combat and the zero-period action-11 case remain open.

## Connected staff actions 30–33 (2026-11-17)

ActorOrderTests verifies the attack reader's weapon priority, full item
contributions and both unarmed flags. It checks the defense reader's role-zero
base, retained tier term and four equipment weights. A nonzero aim received
in field 26 moves the test's strike centre into range. Action 30 orders its
fixed magic sound or ward sound before the proc and its type-9 record. The
ordinary outcome uses resistance, and a fully absorbed ward still reaches
the proc. The connected target's health remains under authority.

The action-32 and action-33 fixtures check the rolling ally, self and
controlled ally in signed actor order. Each target uses variation followed
by a fresh critical draw that leaves the signed amount unchanged. The
fixtures distinguish a successor pulse at upper equality, a final pulse
at equality and a final pulse overrun. The Windows x64 build, ten focused
suites and all 60 CTest suites passed. Revision 16 parsed and all 46 links
resolved. Both independent contracts matched their complete hashes and
saved line counts. The original Alpha dedicated server accepted the new
client with protocol version 3 and assigned creature ID 1. The Vulkan
capture confirmed seed 41's Lands of Gami scene, HUD and minimap.
Original-client combat and last-bit math comparisons remain open.

## Connected ranged dispatch and stat re-certification (2026-11-17)

ActorOrderTests supplies independent pitch samples to actions 34, 47, 89,
48 and 90 and observes their signed actor order across the controlled turn.
It checks signed negative quotients, strict anticipation and zero
crossings, case 89's bracelet numerator, and absent hit, effect and buff
output on the scoped nonlocal release routes. The Windows x64 build,
nine focused suites and all 60 CTest suites passed.

Revision 17 parsed and all 48 links resolved. The ranged dispatch and
re-certified critical and haste contracts matched their complete hashes.
The recertification checks the client and dedicated arithmetic against
their original image bytes. Connected projectile receipt and stepping,
native playback and imported power's last-bit match remain open.

## Original Alpha desktop startup (2026-11-17)

An independent x86 diagnostic sent the original initial Direct3D 9
presentation values, including a null device window and the hidden focus
window. On the same 1920 × 1080 adapter the call returned `0x8876086C`
under sandboxed execution and `0x00000000` with normal desktop access.
XAudio2.7 COM activation, DirectInput8 initialization and system mouse
creation all returned zero. Microsoft documents the initialization call
and its return values. [Direct3D 9 CreateDevice](https://learn.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3d9-createdevice)

The exact original `Cube.exe` opened a responsive window in a fresh
isolated directory without imported saves or mods. Accessibility tools
identified the native window and its title bar. Three window capture
attempts timed out before menu pixels were available. The original-client
menu, gameplay and visual acceptance therefore remain open.

## Connected projectile and combat receipt order (2026-11-17)

SimulationFrameTests runs a 41 ms frame in three pieces. Existing shots
advance in all three. Received records retain the carried fields, age,
duplicates and unresolved sources at the resident tail. The next world
piece advances them for the first time. NetTests checks the frame gate
with a real loopback socket, all three packet-4 combat sections in one
drain and the exact local-source selection and order on outbound
packets 7, 8 and 9.

IncomingHitTests replaces a local ward twice from the received buff
list before consuming its first hit. The ward update follows the
complete received list, including an unresolved target, in the frame
aggregate. The local hit callback precedes projectile insertion and
foreign-source hits follow it in wire order. The fixture checks an
equal low word with a different high word and the retained type-6
payload. The Windows x64 build, eight focused suites and all 60 CTests
passed. Revision 19 parsed, all 53 links resolved and all three held
contracts matched their saved hashes and line counts. The production
player/controller diagnostic, five actual-client flows and 36 release
archive digests passed.

Two loopback clients joined the hash-verified original dedicated server
using protocol 3 and seed 41. Both retrieved the same two 112-byte shot
records in send order and with unchanged bytes. Resident admission
dropped both echoes at the sender and retained both at the peer. Native
flight effects and late capture behavior remain open.

## Connected live projectile contact and steering (2026-11-17)

RangedSchedulingTests checks that a resolved nonlocal arrow consumes its
two gain draws and critical draw before the controlled target rejection.
The direct clock still rises when the helper rejects that target. A live
guard fixture observes the guard mutation before the pitch draw and checks
that the hit record does not apply health damage. An ordinary linked
target and an occluded target each skip contact before any draw. The
charged impact retains all eight charge bits and leaves the direct clock
untouched. The beneficial controlled self hit applies at the append site.

The boomerang cases check the live zero-key source, the 800 and 1,100 ms
edges, the full fixed aim and return offset, the repeated catch factor and
the first removal above 1,200 ms. The Windows x64 build, nine focused
suites, all 60 CTests, production player/controller diagnostic, five
actual-client flows and 36 independent package digests passed. Spec
revision 20 parsed with 55 links. Both held contracts matched their full
hashes and saved line counts. The post-move terrain ray and original-client
projectile effects remain open.

An isolated production Vulkan client received remote arrow records at its
live position through a loopback peer. Its next observed player state
carried the exact 5,000 ms slow counter and the same 260 health. The client
rendered its requested frame and disconnected successfully.

## Backward projectile terrain and object ray (2026-11-17)

RangedSchedulingTests compares a solid cell behind the advanced point,
an obstacle ahead, distance equality, a solid point sample and passable
water. An active object side more than one substep away still clips the
first substep because the query uses the full piece distance. The same
side with its state clear leaves the shot in flight. The ray cases have
no solid-point effect or random draw.

VisibilityTests checks the terrain and object bounds, negative and
unordered limits, and the integer-indefinite conversion on an unordered
boundary quotient. The open-air arrow and steering fixtures now start
inside map coverage so they measure displacement independently of the
missing-cell boundary. The Windows x64 build, nine focused suites and all
60 CTests passed. Revision 21 parsed and all 56 links resolved. The
original dedicated flight contract matched its full hash and readback.
The production player/controller diagnostic, all five actual-client flows
and the 36 package digests passed.
Exact solid-cell effects and original-client projectile playback remain
open.

## Projectile solid-cell sampling and burst (2026-11-17)

RangedSchedulingTests advances an arrow from one column into a different
one at exact fixed Z of minus one block. The advanced column's lower
cell is solid and the cell above it is clear. The burst uses all three
original fixed positions after rollback and the distinct RGB of the
restored water cell. It verifies the binary32 color values, vertical
velocity, size, particle count, style, spread and zero producer draws.
The separate ray cases still produce no point burst during rollback.

The Windows x64 build, nine focused suites and all 60 CTests passed.
Revision 22 parsed, all 57 links resolved, and the point-impact ruling
matched its full hash and saved line count. The production player/controller
diagnostic, five actual-client flows and all 36 archive digests passed.
Original-client particle playback and dedicated defensive hit application
remain open.

## Dedicated projectile hit delivery (2026-11-17)

RangedSchedulingTests gives two player targets the same geometry with
opposite signed key order and inserts their views out of order. The first
admitted signed target receives two byte-identical ordinary, guard or
ward records. The duplicate keeps player health unchanged. Guard and
ward retain the gain, critical, proc and pitch draw counts. The direct
contact clock still updates the admitted target.

A two-shot fixture verifies the first arrow's immediate lethal damage
to an NPC, the second arrow's continued flight, and both unchanged
outgoing launch copies. Impact feedback precedes the fatal sound. The
Windows x64 build, ten focused suites and all 60 CTests passed. Revision
23 parsed with 59 resolved links and the held dedicated and point-hit
contracts matched their hashes and saved line counts.

Two clients joined the production dedicated build on loopback, adopted
seed 41, and retrieved both relayed shot records with their full bytes
and order intact. The origin filtered the echoes. The server shut down
cleanly. The production player/controller diagnostic, five actual-client
flows and all 36 archive digests passed. Matched original dedicated hit
playback, full launch admission and staged offline projectile application
remain open.

## Offline terrain history and blueprint persistence (2026-11-17)

SimulationFrameTests checks the 20, 20 and 1 ms pieces, repeated ordered
records in each held zone, signed coordinate routing at world zero and
unloaded-zone rejection. Negative day records survive the wrapped
day-minus-three comparison.

The owned-content diagnostic placed and rendered all 600 wooden floor
cells. The isolated WorldSession file stored and reopened 1,800 ordered
20-byte records from three pieces, with the original seed and day. All
edits replayed at day 20 with their exact color and type. They expired on
day 21, and the restored zone had no anchor records. The capture showed
the placed floor and generated terrain. Native save interchange and
original-client placement remain open.

The Windows x64 build, ten focused suites and all 60 CTests passed.
The production player/controller diagnostic, five actual-client flows
and 36 archive digests passed. Revision 24 parsed with all 60 links and
the held save contract matched its full hash and saved line count.

## Packed terrain vertex light (2026-11-17)

TerrainLightTests samples the actual mesh corner through a held zone.
Uniform intensities 255, 5 and 128 pack to alpha bytes 254, 4 and 128.
A nearest low-light blocker packs to 238 and a far blocker to 251.
The fixture compares the normalized float values read by the Vulkan
vertex layout and checks the three stored RGB values on the all-open
top face. Existing WaterTests and WaterLiveTests also passed.

The Windows x64 build, nine focused suites and all 60 CTests passed.
Spec revision 25 parsed with 61 resolved links. The held native contract
matched its complete hash and line count. The production player/controller
diagnostic, five actual-client flows and 36 package digests passed. The
owned-floor render again showed all 600 placed cells. Its ground remains
bright. Native cache scheduling and matched original-client terrain images
remain open.

## Live terrain palette and camera cell (2026-11-17)

LookTests checks both native sky replacements, independent type-two material
tint and the original fog, front, back and ambient inputs. An owned-content
Vulkan diagnostic checked the production world scene against the live region
samplers. At exactly Z = −1 it read the type-two cell below the boundary.
Just above that plane it read the dry cell. The floor capture matched the
previous checkpoint byte for byte. Its bright ground remains unresolved.

The Windows x64 build and nine focused suites passed. The first parallel
CTest run timed out waiting for an adjacent zone in NetTests. The isolated
NetTests rerun passed in 23.32 s, followed by all 60 CTests at two concurrent
jobs in 97.14 s. The production player/controller diagnostic, five actual
client flows and 36 package digests passed. Revision 26 parsed with all
63 links, and both held native contracts matched their full hashes and
saved line counts. Native mesh-cache lighting and comparable original-client
terrain images remain open.

## Mesh-cache light preparation (2026-11-17)

TerrainLightTests checks repeated frame edits, a different coordinate
in the same cache slot, the signed world boundary, request retention
through invalidation and successful clearance. A newer mark survives
an older worker result. The actual mesher reads the prepared light on
dirty interior chunks, uses four cells at a dirty seam, and waits for a
missing diagonal before preparing a corner.

The owned-content Vulkan diagnostic placed all 600 floor cells and
applied two live batches to a five-cell column while a remesh was pending.
The final capture shows the newest batch's colour. The surrounding
terrain is still bright. The Windows x64 build, nine focused suites and
all 60 CTests passed. The production player/controller diagnostic, five
actual client flows and 36 package digests passed. Revision 27 parsed with
64 links, and the held cache contract matched its full hash and line count.
Original worker timing, writes to resident light and matched native
terrain images remain open.

## Stored light rectangle propagation (2026-11-17)

LightingTests verifies the native corridor values through round sixteen,
the next cell's value of 4, and opaque bytes staying intact. Separate
fixtures check a 200 work byte producing 170 across an area boundary,
a stale bright committed byte producing 4, and an explicit-area lookup
producing zero. The margin retains its original committed byte and
exposes its final work byte to a later call. Flagged stored water
contributes its own work byte. Implicit water contributes 255.

The before-and-after seed 123 area fixture matched all 65,536 column
headers, 233,386 solid records and 231,048 committed clear-cell values.
All three light bytes held the final value in the new recording. One
local diagnostic run took 2.48 s before the change and 0.94 s after it.
The owned-content Vulkan probe verified all three bytes in 244,984 clear
cells on area admission. Both floor captures matched the previous
checkpoint byte for byte, including the bright terrain.

The Windows x64 build, nine focused suites and all 60 CTests passed.
The production player/controller diagnostic, five actual client flows and
36 package digests passed. Revision 28 parsed with 65 links. The held
contract matched its full hash and line count. Mesh and destruction
writeback to resident areas and matched original-client visuals remain open.

## Resident mesh light publication (2026-11-17)

LightingTests publishes two sequential rectangles by a zone seam.
The second reads the work byte written by the first. The earlier
neighbourhood retains both original areas, and later preparations within
the batch reuse their writable copies. The existing placement records
and network acknowledgement survive.

The owned-content Vulkan probe applied queued records to both sides of
a loaded area seam. Their work bytes held at 109 before preparation.
The left and right resident cells committed 183 and 216 respectively
to all three light bytes. An edit that removed the emitter during
preparation preserved the new rock and retried to values of 4 on both
sides. Both earlier snapshots retained their original bytes.

Moving preparation from the frame thread to an ordered background
worker reduced the longest world update in the isolated streaming
probe from 218 ms to 57 ms. Total streaming and seam-edit time in
those runs was 7.39 s and 8.06 s respectively. The x64 build, ten
focused suites and all 60 CTests passed for the background variant.
The production player/controller diagnostic, five actual client flows
and 36 package digests passed. Revision 29 parsed with all 65 links.
Immediate destruction margins and matched original-client runtime
timing and visuals remain open.

## Primary terrain fog and composition (2026-11-17)

LookTests checks the first tick, a transition across unsigned rollover,
negative signed counts, fixed-point subtraction before narrowing, and
the exact binary32 result of a thousand recurrence steps. Zero elapsed
still applies the startup floor. A world mismatch also forces the
floor, and a long positive count reaches the same stable value.
The frame test checks both the current followed angle and the 90-degree
map choice with an unchanged reciprocal fog distance.

The Windows x64 build, ten focused suites and all 60 CTests passed.
The production player/controller diagnostic, five actual client flows
and 36 package digests passed. The terrain Vulkan probe passed the
current point-light routing, cap, lifecycle and fallback checks.
The owned-content town capture showed the terrain and interface with
the updated fog. Revision 30 parsed with all 68 links and verified the
two held presentation contracts and re-certified light lifecycle.
The checked source scope leaves 49 inherited contracts awaiting review.
Native capture comparison and complete world transition acceptance
remain open.

## Combat terrain lighting (2026-11-17)

ActorOrderTests checks that only the first admitted strike spends the
removal sound draw. The light callback observes the removed block after
that draw and receives the full fractional radius. LightingTests checks
the independent signed endpoints, half-open destination, eight-cell
working margin and preserved earlier terrain snapshot.

The owned-content world view verified an immediate cross-area light
commit of 183 and 216 before mesh processing. The margin kept its
committed value of 35 and updated both work bytes to 255. Two runs
passed these checks. Both runs also observed a long streaming update
before the destruction stage, at 2.23 s and 1.36 s respectively.
The cause of those streaming stalls remains open.

The Windows x64 build, ten focused suites and all 60 CTests passed.
The production player/controller diagnostic, five actual client flows
and 36 package digests passed. Revision 31 parsed with all 69 links and
the held combat terrain-lighting contract matched its hash and full
readback. Original-client and dedicated combat playback remain open.

## Feeding progression and late capture (2026-11-17)

ActorOrderTests checks both endpoints of the action-110 crossing and
the exact one-draw pitch. Five bites match each original binary32
progress value. The wire event retains 5,000 ms while the resident
ages in the producer's piece. The threshold changes ownership before
publication. An earlier hit in the aggregate can capture after the
threshold. Wrong upper identity bits, unordered progress and a later
duplicate resident leave the food equipped.

The production Vulkan client joined an isolated loopback peer with a
food item. Received type-six hits changed health from 100 to 97 and
then 95. Partial progress and a threshold with the wrong high owner
word both preserved the food. The live threshold with the matching
owner converted it to species 19, level 3 and experience 287,454,020
while health stayed at 95. The peer's actor record had controller 3
and owner zero.

The Windows x64 build, ten focused suites and all 60 CTests passed.
The production player/controller diagnostic, five actual client flows
and 36 package digests passed. Revision 32 parsed with all 71 links and
both held contracts matched their hashes and line counts. Original
client and dedicated feeding playback remain open.

## Connected objective receipt (2026-11-17)

`IncomingHitTests` verifies that resident buffs precede objective writes,
local hits see the current descriptor and latch, and the complete objective
sequence follows both hit passes. A coordinate of -1 acknowledges region
zero without a cell write. Coordinates -8 and 8,192 do not acknowledge it.
A missing region stays unallocated and still requests contents when first
prepared. Repeated completion records use the final descriptor even after
a kind-zero update. `NetTests` exchanges all 56 bytes of ordered records
under the same frame gate, including the distinct padding bytes.

`UiTests` covers first-page formatting, punctuation, and the four-slot
dialogue restart with an exact zero actor identity. A failed preparation
leaves the existing slot bound. `ContentTests` exercises the real owned
dictionary and verifies the innkeeper bypass and the zero, one and four
draw paths for non-innkeeper speech.

The production Vulkan client requested its controlled actor's region
(512, 512) over isolated loopback. One objective record suppressed further
requests past a full sender interval. The client then applied a completion,
a state-one request for the innkeeper, an absent actor with the same low
word, and a final live Troll objective. Its captured banner displayed
"Defeat the Troll." and the speaker began the localized innkeeper text.
The two accompanying hits reduced health from 100 to 95. Both private
processes exited successfully. Bright terrain is still visible in that
capture. Native dialogue, objective and visual comparison remain open.

The Windows x64 build, ten focused suites and all 60 CTests passed.
The production player/controller diagnostic and the five actual Vulkan
flows passed. Package inspection verified 36 hashes in 37 archive files.
Revision 33 parsed with all 73 links and both held objective contracts
matched their exact hashes and line counts.

## Connected world readiness (2026-11-17)

`NetTests` sends an identical pair followed by two distinct requests.
The repeated pair retains the actor. The new pair removes old actors and
poses while preserving the local key, connection, queued projectile and
objective records, and the connected clock. `SimulationFrameTests` checks
signed display wrap, a skipped world interval and a later input phase.
The next admitted frame keeps only its own interval. `SaveTests` checks
exact fixed coordinates, independent connected discovery archives and
the catalog. It verifies that a connected selection creates no local
terrain database.

The production Vulkan client received requests for seeds 55 and -7 on an
existing loopback connection and activated `online_-7`. It published the
saved fixed X and Y and retained health 93 after the hit within the request
sequence. A duplicate -7 request left it active, and the next hit reduced
health to 89. It asked for the new region and rendered its evening terrain
with health 89. On exit the private character and two catalog entries
reopened, and the target discovery map retained and advanced its count
from 61 to 63. No terrain archive was created. An initial fixture used a
saved height of 450 blocks and took fall damage after proving the cutover.
The final exercise used a near-ground saved position. The original client's
worker timing, unsaved-anchor playback and cross-thread state remain open.

The Windows x64 build, ten focused suites, all 60 CTests, the production
controller diagnostic and five actual client flows passed. The package
verified 36 hashes in 37 files. Revision 34 parsed with all 75 links and
both held source contracts matched their hashes and line counts.

## Required water behavior (2026-11-17)

`WaterTests` and `WaterLiveTests` passed. A player-sized body stayed in
the swimming state for all 150 movement steps at both generated sites,
with zero standing steps. The seed-123 lake shore and wet-ground region
produced 11 and 48 dry-bank reeds. The emitted reeds had adjacent surface
water. The wet-ground site gained swimming depth at all 3,532 sampled
exposed wet surfaces.

The previous and current Vulkan executables loaded isolated INI files with
the older water settings. At the same former wet-ground site, the current
client reported a submerged camera and rendered the blue fog and teal
terrain. The previous client reported a dry camera. The current client
also rendered the shared surface from above. The actual dedicated server
accepted a fresh loopback bind with the old command-line flag present,
exited normally and saved `Water=swimmable-3-v1`. Its profile database
was read back by the test. The shared surface algorithm's source blob
matches the previous checkpoint.

The Windows x64 build, ten focused suites, all 60 CTests, the production
controller diagnostic and five actual client flows passed. Package
inspection checked 36 hashes in 37 files. Spec revision 35 parsed with
all 76 links. Original-client movement and visual comparison remain open.

## Connected spawn recovery (2026-11-18)

`CreatureTests` exercises the inclusive upper corner of the overlap box
and the exact negative lower layer. The first two pieces produce fixed Z
values 6,553 and 72,089, preserve X and Y, and clear the drawn-height
offset. Controller role six and received replicas skip the lift. The
unloaded-zone case keeps ordinary solid movement fallback.

An isolated loopback peer instructed the production Vulkan client to
activate an unsaved seed. The peer first received the exact home X and Y,
zero Z and zero velocity. Once terrain arrived, it observed seven distinct
rising positions and grounded contact near Z 246.08636. Health stayed at
100 throughout. The Vulkan capture showed the actor standing in the
forest with contact `0x11`. Before the publication fix the actor reached
the floor visually but the peer only received zero contact flags.

The Windows x64 build, all 60 CTests in a serial run and the package
inspection passed. The first concurrent focused run missed two periodic
network-hit assertions. `NetTests` passed when rerun alone and within the
full serial run. The package verified 36 hashes in 37 files, and revision
36 parsed all 77 links. The held spawn contract matched its exact hash and
line count. The production player/controller diagnostic and five actual
client flows also passed. Original-client timing and special teleport
cases remain open.

## Offline projectile lifecycle (2026-11-18)

`RangedSchedulingTests` gives two arrows the same collision center.
The first visits and kills the negative-ID NPC in signed actor order.
The second sees that death and applies to the controlled actor immediately.
Each output reaches presentation once. Both launch records keep their
original bytes, the retired shots leave by the end of the piece, and the
next piece advances the one older resident without replaying a launch.
An owner's water burst spends its critical and proc draws in order and
heals on the 999-to-1,000 ms crossing. Existing cases retain the water
spell conversion fields and the dedicated role-zero duplicate records.

An isolated Vulkan probe used the production `Player` and world routines
with owned terrain. In a 20/20/1 ms frame, the player launched an arrow
during the first actor turn. The hit and fatal callback ran after the
object phase moved the fixture target into contact. The target was dead
before the piece returned. An older water burst healed the
player from 20 to 23 in the second piece. The frame retained one unchanged
launch copy and two presentation records. The only surviving resident
finished at age 1,020 ms. A Vulkan capture confirmed that the owned area
rendered successfully. The test supplies actor state and coordinates.

The Windows x64 build, focused ranged tests and all 60 serial CTests
passed. The production player/controller diagnostic and the five live
client flows passed. The package verified 36 hashes in 37 files. Revision
37 parsed all 78 links and the held offline contract matched its exact
hash and line count. Original projectile action and particle playback
remain open.
