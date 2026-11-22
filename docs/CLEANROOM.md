# AlphaVulkan clean-room process

AlphaVulkan is an independent engine that loads content from the owner's Cube
World Alpha installation. Native code research and implementation use separate
agent contexts.

1. A source-side researcher reads the standalone author policy before inspecting
   scoped original Alpha evidence. Infinite-Cubes/alpha may supply leads, which
   must be confirmed against the original Alpha image. The researcher writes
   independently worded behavioral prose: inputs, outputs, timing, decision and
   random-draw order, constants, field widths and evidence limits. No source,
   transcribed pseudocode, invented reconstruction names or copied structure.
2. After the researcher reports FINAL / STABLE / HELD, the implementer reads the
   complete contract and writes engine code using project-owned sources only.
   The implementer does not open native code or reconstruction sources.
3. Verification compares runtime behavior and owner data with the contract.
   Synthetic tests establish bounded engine behavior; native captures establish
   only the behavior they actually record. Neither proves complete parity.

Source-side role rules are in [CLEANROOM_AUTHOR.md](CLEANROOM_AUTHOR.md). Do not
send this combined process/history document to a source-side author.

## Continuation provenance

The 2026-11-07 continuation copied project sources from
Documents/Claude-workfiles/AlphaVulkan into this chat's workspace without copying
its builds, saves, recordings or distribution. Claude's original files remain
untouched. Inherited implementations and specifications require a final
provenance audit; their presence is not certification of clean-room authorship.

Keep assets, data stores, captures, exports and saves local and outside releases.
Before a final release, audit source/specification handoffs, uncertain inherited
contributions, package contents and broader gameplay, rendering, save and network
comparisons against the original Alpha client and server.

## Water and lighting continuation

Independent source-only contexts supplied the held
`alpha-light-propagation-lifecycle.md`, `alpha-ranged-action-scheduling.md` and
`alpha-water-material-contract.md` contracts. The implementer read the complete
final prose and project-owned implementation only. One earlier combat-author
context stopped after encountering unvetted audit metadata, wrote no contract,
and was replaced by a fresh author restricted to original native evidence.

Lighting boundary changes use the finalized lifecycle contract. The shared water
surface reuses the Steam engine's original GPL shader and snapshot design; it
does not use an original-game or third-party reconstruction shader. Swimmable
shallow terrain and bank reeds implement explicit user extensions, described in
[water-extension.md](water-extension.md). Synthetic tests and local owner-data
captures remain distinct from broader native acceptance and provenance audits.

Fresh source-only authors also supplied held zone-spawn lifecycle and rock-color
precision contracts. Both were read completely before use. AVSP version 2 records
the explicit native generation-prefix boundary; older count differences remain
unclassified where their capture lifecycle was not recorded. The precision
contract establishes binary32 channel arithmetic and truncating byte conversion,
with ambient controls and the imported mathematical helper still unresolved.
The later held `alpha-floating-point-environment.md` was read completely and
supersedes the earlier rock contract's stale import-ordinal identification:
the client imports `_libm_sse2_cos_precise` by name. It establishes the executable's
53-bit x87 startup precision request, while leaving the terrain worker's full
ambient controls unresolved. Do not reuse the superseded ordinal claim.
The two later cosine contracts were each held and read completely. The numeric
contract supplies complete bounded binary64 phase data and rounding relations
for the identified original runtime under its stated controls. NoiseMath is an
independent scalar implementation of those mathematical facts, without native
register layouts or code. Exact static anchors pass; benchmark/library comparisons
are performance diagnostics, not native runtime observations. The retained
blue-byte discrepancy remains open; this change did not remove it in its zone.
The implementer later ran a private black-box observation through the scalar ABI
already specified by the held contract, using an isolated, hash-verified copy of
the owner's original runtime. All 81,966 tested results matched the engine bits
under explicitly selected controls. No native code was read for this probe and no
probe, implementation or result was sent to the isolated source authors. This
sampled runtime evidence does not establish the running terrain worker's controls.

The owner-content setup tool and source-module SDK are independently designed
engine features. They consume existing engine loaders/APIs and public operating
system interfaces, without a new native-code implementation handoff.

## Creature-update continuation

The source-only authors subsequently supplied held common-update, ranged-audio,
periodic-buff hit and periodic-hit application/fixed-stat contracts. Each complete
final artifact was read before implementation. The common phase and ranged-audio
changes have bounded synthetic checks; broader native fight acceptance remains
open. The periodic contracts distinguish motion
velocity from the stored response vector and retain unknown semantic labels.
They do not certify fatal death/loot/threat paths or original-server equivalence.
The later held hit-contact/threat/fatal-lifecycle contract was also read in full.
Its bounded source resolution and threat-contribution rules are implemented;
complete propagation, protected-target admission, death/world effects and nested
loot integration remain open. The contract's explicit missing dependencies remain
acceptance limits, and do not establish original-server equivalence.

Further isolated authors supplied held buff record/replication, server admission,
client buff-action, primary-input, common hit-input and common-processing-state
contracts. Each complete artifact was read before its corresponding implementation.
These establish bounded record layouts, source identities, timing, state writes,
resource arithmetic and draw order. They do not certify full producer coverage,
generic hit/death integration, imported math status equivalence or native combat
acceptance. Separate terrain-color and opaque-output contracts preserve unknown
effective output state; no guessed gamma transform follows from those unknowns.
The separate selector-17 production contract was held and fully read before adding
its explicit selector and generated buff-event route. It identifies unspecified
native producer padding; deterministic engine initialization is recorded as such.

Later isolated source authors supplied held projectile call/ownership, water
contact, critical/haste numerics, hit record production, presentation draws,
vector adjustment, scalar reduction, primitive application, response consumption
and client buff-transmission contracts. Each was read completely before use.
The client frame/world-call ordering contract was also held and fully read; it
does not establish every input route, pause state or world-transition condition.
Record codecs preserve received opaque bytes while fresh producer storage is
deterministically initialized. Primitive application and response checks establish
bounded target mutations and motion consumption. Complete world relationship,
fatal/loot processing and native frame timing
remain separate open work; these handoffs do not certify full combat parity.
The later world-creature health-admission and generic source/self auxiliary
contracts were also held and read completely. The former now supplies the bounded
maximum-query/admission boundary. Bounded source/self item selection, arithmetic,
counter writes and direct health mutation now use the auxiliary contract, with
local delivery policy separate from the serialized payload. The independently
held client hit-transmission and server hit-routing contracts were fully read
before routing changes. Their native self-echo behavior is retained; local direct
producer delivery is distinct from receiving a record over the network. Complete
incoming category consumers and broader network acceptance remain open.
The later incoming-hit mutation contract was held and read in full before adding
the distinct controlled-creature receive phase. Numeric categories retain their
caller-specific behavior; the world applier is not reused as the network consumer.
Its bounded tests do not close every category, whole-frame publication order,
fatal world behavior or original-client/server gameplay acceptance.

The later receiver-publication-locks and network-creature-state-defaults contracts
were held and read completely before adjusting the receive worker. New remote
actors start from specified fixed semantic properties; existing actors keep prior
received state. Insertion/baseline copy and final assignment use separate frame
lock intervals, with decoding and receiver-private presence tracking between them.
The native constructor leaves some codec storage and padding unspecified. Fresh
engine storage is deliberately zeroed, while received opaque values survive later
partial deltas; no native equality claim applies to those fresh unspecified bytes.
Loopback regressions cover partial first updates, retained state and frame exclusion.
Native interpolation, complete transition semantics and wider timing remain open.

The original-only client-startup-window-boundary contract was subsequently held
and fully read. It distinguishes loader dependencies, window creation and the later
show request without identifying a redistributable installer from an absent window.
Its source author received no implementation or runtime observations. The owner's
reported native menu and the separately observed isolated game window establish
a startup checkpoint, not completed interactive or visual acceptance.

The complete held terrain-shader-color-numerics contract was then read before
updating CPU clock/palette widths and stored constants, vertex light accumulation
and ordinary fragment fog/oscillation grouping. The implementation uses the
engine's existing mesh and orthonormal camera interfaces; original matrix
construction, spatial producers, light selection and device precision remain
bounded dependencies. A successful engine capture is separate from native pixel
acceptance, and the bright-ground observation remains unresolved. No gamma or
display correction follows from the incomplete output-transfer evidence. Existing
moon/shadow/water extensions remain distinct from the native shader account.

Later spatial-scalar, cache-publication, chunk-point-light, sort-order/ABI and
renderer-origin contracts were held and fully read before implementation. The
scalar implementation preserves wrapped windows, repeated sample passes and
ordered binary32 accumulation while reusing one engine read lock. Fixed-point
light priorities and inclusive per-chunk membership use a separately structured
engine selector, not original record layouts. Per-draw storage rows route each
chunk's fixed ten-vector prefix to the existing shader arithmetic.

A private black-box recorder called the original sort through its held ABI in a
separate process with a verified isolated executable image. It inspected no
native instructions, patched nothing and did not run the game's entrypoint.
All 50,000 recorded finite-key examples matched the actual engine candidate sort,
including semantic payload movement. Neither the recorder nor its results were
sent to source authors. General large mixed ties and exceptional keys remain
bounded. Procedural red/blue GPU quads separately verify engine routing, buffer
growth, clearing and the fixed ten-slot prefix; they are not native pixel evidence.
Original source gathering/lifetime, renderer anchor following, worker controls
and native visual acceptance remain open.

The later recipe-grant entry-boundary contract was held and read completely before
correcting loaded-character recipe observation. Saved recipe lists are restored
without regenerating the loaded level's randomized grants; the observed-level
tracker synchronizes after load. Later mismatches publish the new observed level
before one grant invocation, without a catch-up loop. The unsaved diagnostic
character's initial grant is separate from real character creation. Full recipe
content tables, update scheduling and wider entry failure paths remain bounded.

The complete held terrain-light-source-lifecycle contract was reread before the
timed-light and item-color changes. Engine-owned fading records retain canonical
fixed-point positions and integer ages, advance before gathering, retain duration
equality and erase strictly expired entries without changing survivor order.
Event flashes use their raw hit position; later bomb presentation appends a source
from the main position after that frame's collection. Item color respects the
signed descriptor count and qualifying bytes at least 128, including the specified
grey contribution above 131, with ordered sums and reciprocal multiplication.
Malformed descriptor counts remain bounded by owned decoded storage. Native
clock/mode scheduling, complete producer eligibility/culling and attachment
freshness are still open; the GLFW integer-duration adapter is engine behavior.

The held light-source lifecycle contract also supplies the conditional source
admission helper and sector collection rules. Admission compares quantized fixed
squared distance before testing supplied planes in the specified arithmetic
order; culling radius does not enlarge the distance limit. Sector collection
uses the controlled main position, visits existing keys in X/Y order and processes
statics before current live item state. Generated items are no longer collected
both from a stale zone cache and the live list, and model drawing does not decide
whether their lights exist. Upstream projection/view/reference inputs and cached
chunk source publication remain separate dependencies from this bounded path.

The independent source-admission plane-production contract was then held and read
completely before replacing the engine's normalized frustum shortcut. Given P and
V, the kernel computes all four scalar binary32 products for every P*V element,
adds in order, and extracts the six raw row combinations without normalization.
The lower depth plane uses w+z even with zero-to-one projection coefficients.
The implementation preserves specified coefficient operand order and constructs a
completed engine value; it does not claim the original renderer's incremental
shared-state publication or early-query scheduling. Horizontal shifts precede
float conversion while Z remains unchanged. Native projection input selection,
imported tangent accuracy, view following/shake and R/A evolution remain open;
the current camera supplies engine matrices to this bounded arithmetic kernel.

The same held source-lifecycle contract supplies the four equipment light sites.
The engine now uses ordered-positive health admission, the finite nonzero counter
ratio without a denominator floor or ratio clamp, independent main/off-hand and
shoulder/chest category gates, and average-times-ratio before the final color
factor. The helper accepts distinct fixed-point weapon attachments and main
position; the production local/NPC adapter still supplies an explicit main-position
fallback for the two unverified weapon attachments. Kind-1 projectile lights now
precede fading and creature sources and retain their raw coordinates.

The same fully reread contract supplies the mutually exclusive kind117 and
kind135..138 sources, followed by the independent appearance-bit0200 source.
Their supplied main/rendered coordinates, constant RGB, height and positive-health
gates are implemented separately from the equipment ratio. The received semantic
fighter view now retains its already stored appearance flags; appearance-only
deltas update that view without changing other fields. Local, NPC and received
actors share the bounded body/equipment collector. Production displayed positions
remain the engine's draw adapter, and the two weapon positions retain the stated
fallback. Other creature sites, native attachment producers, interpolation,
cached-source publication and complete container ordering remain dependencies.

The fully read held lifecycle and plane-production contracts also bound cached
sprite lights. Worker selection retains the supplied sprite order after the
specified binary64 scaling and integer chunk division. Pending/current engine
lists publish only through explicit cache-miss and nonzero-geometry gates, copy
without consuming pending data and reset together. Collection advances its
33-step cosine phase only for admitted flag-bit1 entries, retains zero RGB and
applies the half-day color factor only to kind13. Cached collection precedes the
engine's new plane publication and dynamic sources. Mesh jobs transfer source
state from the same immutable snapshot as geometry.

Native scan/key and geometry-count producers, sector source lifetimes, worker
metadata outside this light read and surrounding presentation/RNG guards remain
unclosed. Loaded chunk keys and mesh readiness are explicit engine adapters.
Initial planes are deterministic engine zero, and early queries use the last
completed engine publication without claiming native initialization or age. The
integer GLFW clock and platform cosine also retain their stated equivalence limits.

A private black-box observation calls only the named cosine export through the
previously held scalar ABI in a verified isolated original runtime. On 32,896
sampled signed-clock/admitted-phase cases, both narrowed cosine and actual engine
candidate-radius bits match. This uses selected floating controls, not a proof of
the running renderer's controls or all possible inputs. The recorder, original
runtime and observations remain private and were not sent to any source author.

The separate original-only attachment-producer contract was then held and fully
read. It establishes copied references, conditional model-point writes, later
adjustments and light reads before identified pose-refresh calls. Complete pose
matrices, action/history branches, initial float-attachment validity and whole-frame
freshness remain unresolved. This does not justify replacing the explicit weapon
fallback with an assumed current-frame attachment.

The fully reread receiver-publication and constructor-default contracts establish
that decoded noncontrolled core state is assigned independently of an action-clock
change. The engine presentation adapter now refreshes every semantic snapshot,
retains its separately received resident buffs and preserves the existing local
clock reset/advance policy. That policy remains an engine interpolation adapter.
The existing two-byte wire mode property now transfers through the semantic
fighter view/setter without changing adjacent opaque bytes or the wire format.
This repairs engine data flow and does not certify native interpolation or wider
frame scheduling.

The new original-only yaw-light gate and scalar-trig ABI contracts were each held
and read completely before use. The remaining three independent creature gates
and the later action-light gate now preserve their category/bit/presence rules,
separate per-admission yaw evaluations and fixed conversion before integer
placement. Conditional action lights query the stat once, multiply by the stored
creature scalar, divide and truncate to a signed integer cutoff; unconditional
actions skip that query. Color retains its separate cosine narrowing and ordered
products. The existing attack-speed helper retains the specified repeated ratio
query in its unclamped branch; no other ratio helper was changed.

All fourteen creature sites described by the held lifecycle contract are now
represented for supplied state. Production still uses the engine displayed
position/yaw adapter and the documented main-position fallback for weapon float
attachments. Native angle following, complete caller/frame admission, container
order, attachment freshness, imported math/controls and original visual acceptance
remain open. This collector coverage does not establish complete light parity.

A private observation uses the newly held scalar ABI and a hash-verified isolated
original runtime for 33,018 bounded yaw/height/clock cases. Narrowed trig values
match, and engine point/vector placement and color agree with the finite relations
using those observed math scalars. No original actor, pose or transform helper is
called, and no original implementation is inspected. The recorder and results
remain private and were not shared with source authors. Selected controls and
bounded inputs limit this evidence.

The original-only periodic producer-state addendum was held and read completely
before correcting the three direct flash-scalar stores, repeated mitigation-ratio
queries and status-exponent rounding. Production countdown now uses the existing
record producer with the periodic call's explicit flags and zero discriminator.
Each target record is emitted before immediate application or the no-local,
role-zero relay. The engine resolves the source again at application, including
identifier zero, rather than retaining the producer's source-pointer assumption.
Payload-only shield changes remain inside the resident scan; cleanup still uses
the nodes collected at their own visits.

This connects local-player, NPC and dedicated-world player countdowns. The
separately stored engine physics contact is supplied to both periodic producer
gates, and the engine SDK may adjust the controlled local application copy before
it is applied. Neither that SDK extension nor engine identity/contact adapters
establish original frame scheduling. Complete relationship propagation,
protected-player state, fatal callback effects, loot and native combat acceptance
remain open; production integration does not certify those downstream paths.

The separate held item-presentation-vector contract was read completely before
replacing the actor-position direction adapter. Emission now uses the supplied
three-component vector's ordered binary32 norm, binary64 square root narrowed to
binary32, raw fixed target position and selected-item/record order. Signed record
counts, all unsigned qualifying material bytes, grey defaults and the post-color
variant retain their supplied-input relations. The engine's existing Burst fields
carry those values; unverified downstream interpretations and imported-math/control
equivalence are not inferred from the producer contract.

The independently held outer world-call phase contract was also read completely.
It establishes preliminary scans, queued inputs, actors, later projectile work and
final publication within each invocation, including retained aggregate revisits.
It does not turn the existing physics-only split into whole-world timing parity.

That same complete contract supplies the local preliminary health phase. The
controlled engine actor now receives its role-zero/action-84/ordered-positive
health gate before input and common action processing. Integer elapsed converts
to binary32 before seconds and fraction scaling; the first maximum feeds the
addition, a fresh second maximum feeds comparison and only ordered excess queries
the third stored maximum. Zero and negative elapsed retain these boundaries.
The former unconditional post-action heal has been removed. This is a local
health correction; the surrounding world scan, clock policy, aggregate reuse,
whole-world subdivision, full frame input order and original-server behavior
remain separate gates.

The new original-only action-input-phase and world-clock-input contracts were
held and read completely as part of this frame migration. The client now performs
consecutive world calls of at most twenty integer milliseconds using retained
engine output collectors, followed by one input phase. Common actor work, NPCs,
periodic buffs, item/static timers, projectiles and clock arithmetic share those
pieces. Device polling and UI/crafting work are not repeated per piece. Output
views consume newly appended engine notifications without removing the retained
records; this does not assert that every native aggregate consumer uses such a
view. The separately identified native rewrite/revisit behavior remains open.

The preliminary health addition precedes each clock step. The clock uses wrapped
four-byte arithmetic, the supplied acceleration predicate and nonempty-world
gate, and a strict signed day-end comparison. The persistence predicate compares
the incremented counter with that counter plus the supplied piece, as specified;
it is not a wall-clock autosave interval. A new complete socket-clock publication
is applied once at a frame boundary under the existing engine gate, rather than
replaying the cached pair each frame. Identical later packets remain new input.
The engine rejects incomplete socket payloads and requires a complete stored
clock record; the original EOF and partial-reset behavior is not certified by
those validation policies.

Configured action execution is separate from the existing preset producer. The
held-slot walk queries readiness for every held nonzero binding, applies the
index-one restriction, retains primary suppression on rejected attempts, and
performs the two post-primary release cleanups without resetting the clock.
Full-width binding/cooldown keys remain distinct from the low-byte action write.
Full movement/device mapping, all outer UI/transition gates and original
interactive acceptance remain open. Received
actor presentation keeps its stated engine clock adapter, now with the shared
integer duration and per-world-call resident countdown. Preview participants and
explicit unsaved --hour captures also remain identified engine adapters.

Two further source-only contracts are held and were read completely: action
binding production and creature-map key order. The client now refreshes concrete
two/five-entry bindings before world calls and shares that selection with later
input and HUD reads. Fixed engine storage replaces native vector allocation;
zero-masked entries retain their indices. The heavy primary chain returns to 57
at and beyond the action-67 continuation boundary. Ordinary-primary fallback
still selects from current actor state. Neither decimal 28 nor 29 is emitted by
the closed native preset producer; configured consumer checks do not imply a
vanilla producer. Complete display/cooldown-map lifecycle, stat dependencies,
outer admission and native interactive acceptance remain open.

The signed-key contract now also supplies the engine actor traversal. The local
client callback runs at its key among NPCs; protected received-state countdowns
merge with the local callback using the same signed order. NPC storage preserves
references on append, and the reusable ordered index walks live successors.
An in-pass insertion before the current key is not revisited; a later insertion
can be visited. Generated in-pass keys include mapped player entries in their
minimum scan and use defined low-64-bit decrement. Existing entry nodes are
retained across refreshes, without a per-piece sort of the creature storage.
Offline local and NPC projectiles share the ordered output collector.

Engine producers currently assign entry keys from actor IDs at each invocation's
index refresh. This adapter does not establish arbitrary native key/own-ID alias
lifetime across calls, every original creation route, lookup consumers or null
publication. Structural removals remain outside the valid live walk; arbitrary
callback removal of its current entry is unsupported. Dedicated-world player
buff scheduling, remaining controlled hit delivery and received core interpolation
retain their existing adapters; the client contract does not certify
original-server ordering. Full target-scan,
record, fatal/loot and world-transition behavior and native gameplay acceptance
remain open. Outside a live pass, the existing NPC-only generated-ID policy is
retained with defined integer wrap; it is not a claim about all native producers.

Two later original-only world-hit target-admission and fatal-effects contracts
were each held and read completely. The previously held lifecycle contract was
also fully reread before the shared world-delivery change. Melee, beam and bomb
emissions now resolve their target and source at application, use entry keys
inside the live actor pass, and apply controlled target state synchronously.
The SDK changes only the local application copy; outgoing producer payloads stay
unchanged and applied hits are not queued for replay. A world without a controlled
actor still relays living role-zero targets. The socket-hit consumer and its
later caller phase remain separate.

Primitive category four now retains its earlier vector/lock effects, applies the
nonzero-roll/class-four/specialization-one resource and resident-eleven helper,
then emits the self event and returns before ordinary stun/health. Other positive
categories reaching health use the supplied admission predicate, without the
former named-category whitelist. Resident overwrite uses the separately held
record policy; fresh producer padding remains deterministic engine storage.
These changes do not complete contact propagation, protection-state producers,
negative-source-free behavior, full synchronous fatal/world/loot effects or native
combat acceptance. Exceptional health remains outside the ordinary finite-input
claim. The fatal-effects handoff records further work; it is not a completion
claim for the engine's later death and loot adapters.

Further source-only relationship-delivery, fatal-context and kind-89 boundary
contracts were held and read completely. Resident contact identifiers and
binary32 count/threat maps now belong to every semantic combatant, with signed
key ordering. The world caller performs the positive prefix, direct bookkeeping
and ordered nearby fanout before resolving contributor protection. Healing uses
its separate argument direction, membership test and later fanout. NPC AI and
pet targeting read these same maps; core snapshots preserve resident state.
The separate kind-89 check confirms the earlier hexadecimal set's inclusion of
decimal 89 and supersedes its omission from the later decimal list. Earlier held
artifacts remain unchanged.

Relationship outputs retain their complete 24-byte extent, including four opaque
bytes. Fresh engine producers initialize those bytes deterministically; native
fresh values remain unspecified. Received bytes survive decode and queue copies.
Server publication sends the ordered records to each peer. Client publication
uses the frame gate, and frame assignment runs after world calls before action
input. It requires both IDs to resolve, skips equal IDs, repeats the first lookup
and assigns only that actor's threat entry. Distinct IDs aliasing one pointer are
accepted; health, role, ownership and value sign do not add receiver gates.

These are bounded provided-state and complete-record changes. Engine key
production across calls, upstream classification/protection flags, full map
lifetime, malformed-data policies, source-free healing and remaining world modes
retain their explicit limits. Existing AI acquisition, pet reset and dedicated
player scheduling remain identified adapters. Full synchronous fatal effects,
object associations, regional/credit/loot work and shared random-stream timing
are still open; the retained corpse adapter is not that native continuation.
The additional held object-association and random-boundary contracts were also
read completely. They retain the missing positive selector-five association
producer and distinguish scoped shared import calls from a proven runtime state
model or complete draw history. Their inclusion records the next fatal/loot
dependencies; no completed implementation follows merely from those artifacts.

The separate forward-link zero-gates contract was also held and read completely.
Fatal-context implementation now copies the cached spatial coordinates and
descriptor ordinal through their core state group, separately from body motion.
The reverse identifier travels with core state; the forward identifier survives
core replacement. Ordinary activation assigns descriptor origin, while engine
quest creation retains the independent default context. Link recovery copies a
child's own ID; lookup still uses entry keys. The pet-category route bypasses
numeric-zero forward lookups, while other categories can resolve key zero and
directly zero its health, including self, unless resident selector seven retains
the link. The later supplied-mode refresh and designated item copy have their
own distinct zero rules. First fatal presentation now precedes the retained
corpse adapter and does not replay its sound.

The engine still supplies its selected PlayerViews, named-session admission,
NPC-only rebuild helper and item equality/cache adapter. These do not establish
the complete native selected-role-zero collection, upstream world-count meaning,
all reset/configuration fields or alias lifetime. Existing feeding grace remains
an explicit engine adapter beside resident-seven retention until its full native
producer is connected. Reverse-ID zero on fresh engine storage is deterministic
engine initialization. Full object association, synchronous loot/linked death,
regional progress, credit and shared random history remain open. No native code,
runtime probe or implementation finding was shared across the author boundary.

The held random-stream boundary contract now supplies a shared-call identity
requirement at the scoped gameplay sites. The client routes actions, NPCs,
presentation, world items, missions and loot through a single owned generator;
the Player adapter uses the same reference. The dedicated server adopts that
engine design under its existing lock, without inferring a native server
counterpart from the client evidence. Reseeding an actor subsystem changes the
supplied stream for its later users, with no caller-side restoration. Explicit
terrain-generation and standalone-tool generators remain separate; the required
engine quest extension retains its existing derived client stream.

This change reuses the engine's existing linear-congruential generator and an
explicit seed-one gameplay startup policy. The held import-identity evidence
does not prove that recurrence, original loaded DLL identity, default seed,
thread state or world-entry history. Existing descriptor activation/mission
seed admission and timing, complete nested draws and replay persistence remain
open. No native deterministic-sequence claim follows from shared engine routing.

Fatal application now invokes the actual world loot service before its direct
nonzero-link health store. The live input adapter preserves raw positions and
inventory records without an additional Combatant/map snapshot. Later corpse
outputs identify an already completed loot call and do not replay it; non-hit
death adapters remain explicit. The inherited equipment-to-loot adapter and
nested generators retain their separate completeness/provenance limits. Native
object handling belongs before this loot stage and is still missing; regional
progress, relationship cleanup, credit, final backing writes and aggregate
presentation order are also open. These bounded changes do not complete the
native fatal continuation or certify gameplay equivalence.

The implementer subsequently compared the engine Random class with the owner's
isolated original 32-bit runtime through the named `rand`/`srand` ABI already
established by the held import contract. File hashes matched the installation;
no native instructions, reconstruction code or runtime internals were read.
All 2,097,344 sampled returns matched, including interleaved histories on eight
fresh threads. Six observed seed prefixes now supply x64 regression anchors.
The probe, runtime and observations stay private and were not sent to source
authors. This bounded runtime evidence supplements the source-side import
identity contract; it does not rewrite that immutable artifact or establish
the game's loaded DLL, startup seeds, call/thread assignment or complete replay
history. Details and limits are in [verification.md](verification.md).

The already held fatal-effects and object-association contracts now supply the
bounded pre-loot object stage. The engine retains a separate descriptor
association with default minus one, without deriving it from the object's array
index. Supplied cached coordinates use the documented admission interval; the
first matching kind-five object with nonzero state changes synchronously,
emits its narrowed-position sound and appends a snapshot using the static-array
index. Full engine copies retain association metadata; state-word reception and
state changes leave it separate. The common prefix's uninterpreted word at
offset 0x38 is retained without assigning semantics. Fresh snapshot padding
remains deterministic engine initialization, not a native all-bytes claim.

The local client and dedicated world connect that stage before loot. Ordered
immediate snapshots precede the server's retained final-zone publication adapter;
an identical final snapshot is omitted unless a complete zone was requested.
That publication policy does not infer original-server equivalence. The native
positive-association producer, complete received geometry/prefix publication,
descriptor replacement/lifetime and wider native acceptance remain open. The
existing held artifacts are unchanged; ongoing source-only research receives
no engine, test or runtime findings.

The additional original-only object-producer/snapshot contract was subsequently
held and read in full. It confirms the common-state word's zero construction
and full-width copying, while retaining the missing first positive-association
writer. The native receive reader consumes an 88-byte body but inserts its keys
and common fields through field-wise copying, which skips alignment gaps. The
engine raw snapshot queue deliberately retains received padding; that is an
engine storage policy, not evidence that native queue padding is identical.
Complete native geometry replacement, malformed-record behavior and lifecycle
publication remain separate from this bounded state/word implementation.

The later original-only category-thirteen inventory/counter and fatal-credit
consumer contracts were each held and read completely. Category 13 decimal is
Platinum Coins, with a signed two-byte quantity credited to a separate four-byte
balance. It is not experience. This corrects the inherited mission, quest,
pickup and online-grant interpretation; older held artifacts remain unchanged.
Those engine routes now use the common inventory receipt path. Automatic group
admission precedes the currency credit, including zero and negative quantities.
The displayed Platinum balance and adaptation payment comparison use its signed
interpretation. Adaptation retains separate comparison/debit price queries and
post-debit item updates. The owner's required rebalanced price remains an
always-on engine extension, rather than the native exponential price curve.

The separately bounded level helper now retains binary32 conversion before
subtraction, low-four-byte XP/debit/level arithmetic, the positive-level entry
gate and one fresh health maximum after each level increment. Imported stat/math
precision, ambient controls and exceptional later level wrap remain separate
acceptance limits. The synchronous regional fatal continuation is still open.
Safe selection invalidation and received opaque-byte retention remain identified
engine policies. No source author received engine, runtime or test findings.

The same fully read credit-consumer contract subsequently supplied a shared
typed record and provided-state consumer. It performs both initial lookups before
admission, designated-victim cleanup with repeated current lookups, then separate
direct and linked awards. Supplied raw item storage receives the post-level XP
and low level word; the snapshot copy preserves its five unassigned alignment
bytes. Engine semantic item views update those same two fields without an extra
parse, allocation or random draw. Actor/item storage must remain valid throughout
the call; null relookup handling is an engine guard, not a native lifetime claim.

The network worker now retains complete ordered records, including foreign
recipients, rather than summing own-player XP. The client invokes the consumer
after its incoming hits and persists remote cleanup health without replacing
other core bytes. Its existing online mode/key production, receive protection,
presentation/SDK notifications and core-refresh policy remain engine adapters.
Mode-zero linked effects have supplied-state coverage; the synchronous fatal
producer and its raw item-cache, regional, recipient/grant and final descriptor
integration remain open. No new native source or runtime observation was used.

The complete held fatal-region/credit-order contract was reread before moving
regional state into fatal application. Cached coordinates use signed truncation
before the regional bounds and allocated-block checks. Contributor selection
uses signed key order, ordered-positive values and repeated role-zero lookup;
zero keys and dead contributors remain eligible. Supported selections retain
wrapped counts and immediate snapshots, while the default selection emits none.
The descriptor marker retains its raw byte through activation. Engine metadata
beside the 56-byte snapshot supports the still separate later reward adapter;
fresh snapshot padding remains deterministic engine initialization.

The additional original-only descriptor-tail lifecycle contract was held and
read completely before refining disposal. It identifies the ordinary concrete
Spawn descriptor, its retained-prefix boundary and descriptor-owned destruction.
That fatal route does not delete its separately instantiated creature or draw
gameplay randomness. The engine clears owned extra descriptor values, retains
the prefix boundary/vector capacity and keeps live actors. Explicit rebuilding
removes engine actors matched by its still-retained descriptor IDs, without a
role or association filter; an already empty tail removes nobody. Native
pointer-layout bytes, complete actor destruction, uninspected
writers, behavior-child ownership and wider lifetime/unload remain bounded.

Client and dedicated-world bindings append snapshots before completion disposal,
and corpse outputs mark already completed regional admission to prevent replay.
The server's engine publication policy preserves immediate order before final
changed/requested values, comparing only the last snapshot of each cell. This
does not certify an original-server counterpart. Presentation, completion reward
eligibility/generation, corpse XP, recipient cleanup/credit and final backing
stores retain their explicit integration limits. Source authors received no
engine, test or runtime material.

The additional original-only linked-item-storage and inventory-equality-consumer
contracts were FINAL/STABLE/HELD and read completely before the inventory changes.
The common equality now compares supplied header/count bits and only the first
four bytes of active body entries. Equal nonpositive counts skip the body;
different signed counts remain different. Positive counts beyond owned capacity
compare unequal as an explicit engine malformed-input policy, rather than a
claim that the original predicate validates its storage boundary.

Selected-field assignment copies every body entry while retaining five
destination alignment bytes. Ordinary receipt preserves local-only quantity
normalization, category permission, zero/negative candidate counts and low32
arithmetic. Unknown category-twelve materials retain ordinary storage handling.
The examined hand and linked-equipment routes retain their distinct comparison
gates and old-item storage order. Existing sound/UI admission, other equipment
routes, carry limits, raw pet-cache production and native interactive acceptance
remain bounded separately. The shared credit snapshot uses the same assignment
service. These changes do not certify the full linked-item maintenance lifecycle;
source authors received no implementation or runtime findings.

The already held fatal-effects and regional-order contracts were fully reread
before correcting the common credit preparation boundary. The amount now retains
wrapped level-plus-byte-tier input, separate binary32 steps, ordered minimum,
kind/flag multipliers and final zero replacement. It is prepared after linked
health and before regional callbacks. An optional engine-owned value carries
that float to the still later corpse adapter, where integer conversion occurs;
it is not an inferred native actor field. Live admission and pet reconstruction
clear the bridge. The old helper delegates to the shared arithmetic while
retaining its separate boss-argument adapter. Synchronous per-recipient cleanup,
credit consumption, completion grants and final backing writes remain open.
Selected-control numeric/status checks and synthetic callback mutations do not
establish the original game's ambient controls or native gameplay acceptance.

The finalized linked-item storage contract was reread completely before adding
actor-owned current and retained item records. Only the current slot twelve is
core state; remote core assignment and the dedicated engine connection bridge
retain their actor's snapshot and comparison word. Remote actor assignment also
retains the five existing source alignment bytes through selected-field copying.
The network record remains a distinct raw 280-byte payload. Constructors establish level
one, while the scoped reconstruction reset clears the current source and retains
comparison state. Raw source transport preserves all 280 bytes; selected snapshot
assignment retains its five destination padding bytes. Fresh padding remains
deterministic engine initialization.

The item view now retains product/flags and writes represented body fields without
discarding inactive raw backing. Owned inventory, descriptor activation and the
codec supply the current source. The selected role-zero maintenance path uses
the common field equality, publishes even without a usable link and does so before
missing-child construction. Other categories retain comparison state; the later
controlled level/XP mirror does not acknowledge it. The shared credit stamp
updates the source and snapshot without changing the comparison word, and the
mode-zero consumer can use that real actor-owned storage directly.

The named-world admission, PlayerView collection, NPC-only reconstruction and
broader configuration/name/reset producers remain explicit engine adapters.
Complete native packet/callback lifetime, generation provenance and server
equivalence are not inferred. The retained later-corpse sharing route uses the
common stamp but retains its older eligibility until synchronous fatal migration.
This storage checkpoint does not certify the remaining recipient/grant sequence.

The already finalized fatal-effects, regional-order and credit-consumer contracts
were fully reread before connecting the recipient continuation. A separately
structured service walks the current signed-key actor map, performs contact and
deferred threat erasure for every nonnull role, then tests current target-map
membership for each role-zero credit. Incoming target keys remain separate from
actor IDs. Appending each complete record precedes the actual shared consumer;
its local mutations precede that recipient's completion grants and the next
entry. Counts, health, contribution value sign and key zero add no producer gate.
Aliases and later map insertions retain their live traversal effects.

The first Platinum grant is appended and locally applied before the gear
selection draw, generation, second append and local application. The existing
engine gear generators remain separately bounded dependencies. Final backing
admission uses current cached coordinates/index and retained descriptor storage,
after every recipient, then writes the engine countdown/day state. Original
producer identity, complete descriptor lifetime and timer consumers are not
inferred from that engine binding.

World hits now invoke this sequence synchronously. Dedicated-world publication
retains ordered typed credit/grant records without claiming an original-server
counterpart. Controlled inventory bridges keep raw/semantic item storage coherent;
later UI/SDK notifications do not award again. Corpse bookkeeping suppresses hit
credit/pet-sharing, progress rewards and backing replay, including rejected backing
indices. Non-hit corpse behavior, broader actor selection/key lifetime, native
reset/configuration and generator provenance retain their explicit limits. A new
original-only backing-producer audit remains separate from this implementation;
no engine, test or runtime findings were supplied to its author.

That backing-producer audit was subsequently FINAL/STABLE/HELD and read in full.
It confirms the activation triple's full-width cell coordinates and actual
descriptor pointer-array ordinal, including skipped descriptors, and its default,
reset and copy boundaries. The existing engine activation and provided backing
binding agree with those scoped facts. Spatial-record member 0x178 remains
separate: the audited constructors/builders initialize minus one and copies
preserve it; its first nondefault producer is still unresolved. No ordinal is
invented for that spatial member. Short-reader publication, descriptor editing
and broader generation/lifetime stay open under the contract's explicit bounds.

The subsequent spatial-association producer contract was FINAL/STABLE/HELD and
read completely. Its audit covers the three large generators' direct output
families, record copying/relocation, active-byte restoration and separate ordinal
reference publication. Those scoped paths retain association minus one. The
first nondefault writer and complete transitive generation remain unresolved;
the engine does not assign an invented spatial ordinal from this negative result.
No engine, runtime or test findings were supplied to that source-only author.

Mod input fingerprints and owned content-layer lifetimes are independently
designed engine features. They use project source declarations, public filesystem
operations and the operating system's SHA-256 API, without original-game source
facts. Their bounded compatibility checks do not establish full dependency,
cross-peer, migration or native gameplay equivalence.

Controller state, context routing, combined pointer buttons and controller-only
back navigation are independently designed owner-requested engine extensions.
They use project-owned input/UI code and the pinned GLFW public interface, with
no new original-game source handoff. Supplied-state tests and device discovery
remain separate from physical-controller acceptance and original input/UI parity.

The on-screen keyboard is another independently designed engine extension. Its
draft, field binding and modal pointer lifetime use project-owned interfaces;
accepting a name does not create or save a character/world or connect a server.
Chat uses the existing engine submission adapter. Its own overlay renders last
through the engine's UI painter and owned fonts. Provided-state tests and local
Vulkan captures establish bounded behavior/layout only; they supply no original
keyboard/polling equivalence or physical-controller acceptance claim.

The additional original-only protection-state producer contract was subsequently
FINAL/STABLE/HELD and read completely. It distinguishes the raw appearance word
from the separate two-byte mode field, supplies their scoped initialization,
copy/reset/stream boundaries and identifies concrete descriptor flag producers.
The engine now treats combat mode as canonical between its input, behavior and
physics phases, transferring the complete word instead of letting a stale body
replace copied flags. Pet reconstruction resets its body view alongside the
fresh combat view. Required quest captive changes publish both views directly.
This synchronization is independently structured engine state management; it
does not reconstruct native object layout. The first gameplay-specific bit-0x20
writer, complete generator/caller coverage, world mode/designated-pointer
transitions and native acceptance remain separate gates. The constructor's raw
role/kind defaults do not redefine the engine's semantic controlled-player view.
No implementation, diagnostic or runtime result was supplied to the source author.

The later original-only world actor-creation contract was FINAL/STABLE/HELD and
read completely. It establishes action 92's creation branch, its live count/key
scan, source identities, direct fields, semantic copy/default boundaries, direct
appearance draw counts and ordered setup publication. Other appearance outputs,
future virtual behavior and exceptional lifetime remain bounded. Its presence
does not claim production implementation of that creation branch. No engine,
verification or runtime material was supplied to its source-side author.

The dedicated engine world now supplies the decoded complete mode word to its
temporary player body before linked maintenance. This repairs an engine binding
using the already held raw-word and linked-admission boundaries; the real peer
regression does not establish an original-server counterpart.

The later original-only created-actor behavior and target-selection contracts
were each FINAL/STABLE/HELD and read completely. They establish ordered child
updates, bounded child state/draw relations, the selection prefix's identities,
live traversal, numeric boundaries and relationship publication. The broad
first-child continuation, second-child terrain path, reference-coordinate
producers and visibility geometry remain bounded. These research artifacts do
not certify production implementation of the complete created-actor behavior.

The later original-only visibility-ray contract was FINAL/STABLE/HELD and read
completely before implementation. An independently structured scalar query
retains its fixed-coordinate conversions, binary32 quantization, one-axis tie
order, category bytes, traversal cap and optional four-side-face pass. The
bounded visibility caller uses the held distance/normalization relation.
Production NPC callers use the existing engine position adapter; static metadata
comes from held zones with the current live state byte in original record order.
This does not close raw caller-position producers, original spatial-record
ownership, replacement/concurrency, every collision caller or native acceptance.
No implementation, tests or runtime observations were supplied to the source-side
author.

Complete equipment backing now extends the earlier slot-twelve storage to all
thirteen slots. Twelve additional owned records reuse the existing last-slot
source without duplicating it. Raw imports retain received bytes; semantic item
edits overlay represented fields on that backing. Core refresh and equipment
assignment preserve each destination's five alignment bytes while transferring
all body entries, independently of the signed active count. Fresh engine gaps
remain deterministic zero, without an original padding-equality claim.

Owned inventory, current network state and NPC descriptor backing supply these
records. The existing NPC gear view and level adapter remain separately bounded;
this storage change does not certify full activation/generation provenance.
The loot input adapter now carries complete current equipment instead of a
limited header reconstruction. Nested generation, loot admission and broader
native gameplay remain separate gates. No implementation or runtime findings
were supplied to any source-side author.

The standalone held item now uses the same complete backing and selected-field
copy distinction as equipment. The previously held constructor/default and item
copy contracts were fully reread. Imports and the existing raw wire group retain
all 280 bytes; local consumable assignment and remote core refresh retain the
destination's five omitted-copy bytes. Fresh engine storage uses the specified
level-one properties with deterministic zero gaps. This storage bridge does not
close the behavior-selection prefix or full native consumable/input acceptance.

The previously held original-only target-selection contract was fully reread
before the next implementation. A phased engine selector performs entry and
inventory work, receives candidates from the live signed world map, then
publishes a fallback choice before readiness. It retains key-zero lookup,
preliminary/link identity distinctions, integer counter wrap, binary32 distance
and stealth arithmetic, selected versus partial item writes and ordered draws.
Production NPC behavior uses this service and its existing relationship fanout;
quest-ally admission remains an always-bound intentional engine extension.

Stat/phase providers, the platform power function, initial range/identity
producers, the flat one-unit descriptor inventory and later action/navigation
remain explicit engine adapters or open dependencies. This does not implement
the complete created-actor sequence or action-92 creation. Synthetic origin-key
behavior is distinguished from ordinary routing fixtures. Neither the supplied
state tests nor engine captures certify original-client/server gameplay.
The source-side author received no implementation, test or runtime material.

The world actor-creation and created-behavior contracts were fully reread before
the next implementation. An independent factory retains the action-92 admission,
two threshold queries, live link/key scan, candidate-draw precedence, complete
copy alternatives, current-actor level/health/position sources and fresh state.
Closed generation covers kind 0x25 and unsigned kinds beyond the original table.
Other generation outputs report an explicit engine capability boundary; this is
not a native rejection rule. The completed record enters the owned live map last,
with its ordered setup and owner link, before the creator's common clock advance.

Two later original-only identity-publication and companion-navigation contracts
were HELD and read completely. Normal initial/connected identity publication is
kept distinct from key-zero lookup, pointer exclusions, aliases and other ID
producers. Companion caller arithmetic, terrain query order, partial writes and
work budgets use a separate scalar service. Broad geometry and path expansion
still use explicit owned adapters. The third child retains its counter, ordered
unnormalized draws and action-clock preservation rather than reusing the older
generic wander adapter.

The reference triple is retained as raw core field 40. The inspected activation
producer copies the effective descriptor coordinates; the inspected body reset
preserves it. Copy/stream behavior does not establish a universal initializer.
Fresh engine reference storage is deterministic zero, including action-92 births,
without claiming that native allocation initializes it. Current position and
reference state remain separate. Configuration/kind producers, arbitrary generated
appearances, network-client creation/authority, broader geometry and complete
native lifetime/gameplay remain gates. No implementation or runtime findings
were supplied to either source-side author task.

The held identity contract also supplies the bounded connection-scalar fix.
The engine starts its client identity at literal 1, adopts a complete received
nonnegative ID and retains the prior value for a signed-negative ID. Disconnect
does not restore literal 1. Real loopback regressions distinguish getter, sender
and remote-key classification across reconnects, including adopted zero. This
does not certify the complete native actor replacement, saved-state restoration,
pointer alias lifetime or incomplete-message handling.

Two later original-only configuration and candidate-appearance contracts were
FINAL/STABLE/HELD and read completely before their implementation. Their SHA-256
identities are respectively
`43e27073883dca4302360eb3139712769f36245c9f1a5898c6e2e9de7b4a8760` and
`fe2179bfe2698dd5f557a7e98e5805b9f0a73200e8e959b395f4f8a723f0bd29`.
The author received no implementation, tests, captures, logs or runtime findings.

Descriptor configuration now keeps its full-width choice and ordered candidate
kinds separately from action entries. Descriptor zero/empty and fresh actor
one/empty are distinct; activation performs a value copy, and inspected body
replacement preserves resident configuration. The bounded builder retains the
full descriptor role for its gates, earlier kind-117 append, later action draw,
separate choice draw and six item-seed writes. Repeated setup appends duplicates.
Unrelated equipment/stat branches remain inherited. Native refresh modifies the
descriptor before actor construction; the existing mission regear adapter does
not establish that complete producer or lifetime relation.

Fresh action-92 appearance generation additionally covers full kinds 17 and 96.
The five discarded prefix draws precede exact field writes and caller flag
publication. Independent full-extent fixtures exclude byte 5, whose native value
is unspecified. This closes those supplied generation paths, without claiming
arbitrary reused-buffer initialization, other kinds, rendering interpretation,
upstream generator distribution or a native server counterpart.

The subsequent original-only descriptor activation refresh contract was HELD,
read completely and hash-verified before implementation:
`b08ee5ac970969d3b561248d3abe22c644abfd5a3e9d3791e6506eb264b4558f`.
No engine, tests, runtime observations or implementation history were supplied to
its author. A separate caller kernel borrows the context fields, retains full
role/selector widths, seed and setup order, post-setup flag reads, nine signed
item-level adjustments and ordered count/predicate/rarity work.

The containing zone's integer pair supplies a truncating context lookup, with
resident-parent admission and no allocation. The existing regional payload
provides the corresponding raw fields. The engine caches that reference per
descriptor loop and stores refreshed descriptors in world-owned overlays;
generated snapshots remain immutable. Snapshot replacement drops the overlay as
an explicit engine ownership policy, without certifying native unload lifetime.
The old post-construction mission regear path has been removed.

Window and distance kernels retain distinct clock queries, fixed subtraction,
binary32 conversion/squaring and strict radius admission. Production clock and
identifier sources remain existing engine adapters. Finished descriptor imports
retain zero hand-level words rather than substituting the actor level. General
equipment/stat setup, flat one-unit inventory generation, full native grouped
transfer, pointer publication and complete activation/concurrency remain gates.
The engine's dedicated-server binding is not a native server equivalence claim.

The original-only direct-route geometry, path-state order and resident bucket
contracts were HELD and read completely before the next geometry implementation.
Their SHA-256 identities are respectively
`5593a1d5d13fa41155c2308c77e73ce9a4bcf152052c90589dee978e6a4d6ac3`,
`28dbc38804c48e5d9e6d0197fa5090de0755feb47ce81d2797d942ecc68641c2` and
`a2c44ca0237434dff31079f45c5084dd3ab6e0696e342d6e58854e833d205e57`.
Neither author received engine, tests, captures, runtime or implementation history.

A separate route kernel simulates only a private fixed-coordinate triplet. It
retains binary32 conversion, separate square-root boundaries, distinct initial
and iteration proximity gates, the three-axis budget, terrain boundary scans,
raised-box scans and the specified non-surface-derived height replacement. True
can mean sufficient progress without target arrival. Record consumers preserve
selector/state gates, pointer-identity exclusion, directional truncation and
asymmetric overlap. The production square root is a bounded platform adapter;
the native imported routine's internals and ambient controls remain separate.

An immutable-source index stores one admitted center-selected bucket membership
per record and retains source ordinal order. It uses the producer's binary64/
integer conversion chain and own-chunk bounds, independently of footprint or
state. Cached indices use compact offsets; queries obtain the current state and
stable engine record identity. This preserves the ordinary final membership
relation without reproducing original allocation or publication machinery.
Individual relocation, pointer reuse, concurrent publication and unload lifetime
remain open. Bucket owner/ordinal metadata does not establish record +0x178.

The held path contract additionally supplies snapped endpoint overlap and a
unique exclusion union. This checkpoint uses those setup/reset relations; its
score, selection and expansion relations await the separate search implementation.
The existing search/classification adapter is retained explicitly. Original
pointer width is represented by engine spatial identity, independently of actor
IDs. No native server or whole-navigation equivalence follows from this binding.

The subsequent original-only selected-coordinate classification contract was
FINAL/STABLE/HELD and read completely before implementing SearchState and
SearchGeometry. Its SHA-256 is
`7c22e89698413dd5ed8513337e8c49dfdc41dc6a57400dae70f812a6102d612a`.
The already held path-state contract supplies container/score/list transitions.
Neither author received engine, tests, captures, logs or runtime findings.

The independently owned kernel uses coordinate values and standard containers,
without reproducing native record allocation layouts. It preserves the seed's
distinct score conversion, signed tie order, cached scores, asymmetric costs,
same-call cardinal updates and partial predecessor publication. Its classifier
retains the complete boundary query order, independent sample bit, unusual plus-Y
flag, early record-below probe and pointer/tag distinctions. Synthetic isolated
and combined cases establish these supplied-input relations. Production route
consumer integration is a separate next step; no native gameplay, performance,
concurrency, imported runtime or server equivalence is certified by these tests.

Three further original-only contracts were FINAL/STABLE/HELD, read completely
and hash-verified before the production route checkpoint:
`alpha-path-following-update.md` has SHA-256
`2a4159bfb75c18802d99ffd3dfabd701b54370ad688379dd03ed93f6c443b1f5`,
`alpha-path-world-fallback.md` has
`1406c39173ba493dd2bb7e03e4ee1c6f7b708a3de80f403b637751d021d79b21`, and
`alpha-path-reroot-reference.md` has
`4659e668fc4e7e943810e29b2fcff8207bc30d55ca95d2ba9152287a72fc7395`.
The short last contract also persists an original-evidence prose clarification
received before implementation of the reference acceptance test. The author
received no engine, tests, captures, logs, runtime or implementation history.

The independently structured route consumer retains integer distance decisions,
lookahead query order, binary32 normalization and exact elapsed recurrences.
Rerooting owns replacement coordinate values while preserving scores and the
separate list. Shared endpoint storage removes the prior companion/search
duplication. Production NPCs use resident columns and ordered record identities;
selector-1/2 stores update current state and timer synchronously. Engine change
tracking additionally publishes those mutations through existing save/network
mechanisms; it is not a claimed native event produced by the helper.

The world adapter rebuilds after true and retains the specified false-result
flags and distinct tail-derived triplet. Fresh storage for that new output is
engine zero; its unspecified native initialization and wider consumers are not
inferred. Integer counter addition follows physical integration. The existing
authoritative client/server ownership policy and older block-coordinate endpoint
callers remain adapters. Imported math, broader native scheduling, object
ownership/concurrency, native gameplay and server equivalence remain open.

The original-only descriptor-equipment contract was FINAL/STABLE/HELD, read
completely and hash-verified before replacing the inherited setup projection.
`alpha-descriptor-equipment-setup.md` has SHA-256
`621e13d977210c1a2fc7c95cf1e609ab850590c3788814da3664abe03195b495`.
Its author received no engine, tests, captures, logs or runtime observations.

A borrowed-value kernel now operates on full item records, ordered action and
candidate collections, full kind/role inputs and explicit scalar state. It keeps
unlisted bytes, ordered header/level writes, distinct fixed/random admissions,
the discarded forced-class draw, conditional optional draws and six seed stores.
Numeric setup preserves low-byte argument admission, retained multipliers, the
complete supplied table, hand predicate and separate boss arithmetic/draws.
Actor setup projects complete equipment backing through this kernel. The existing
generator still stores compact item headers; its adapter preserves all represented
fields without claiming original descriptor payload storage or constructor values.
Completed adapter publication does not certify native pointer-visible mutation
timing, arbitrary producer coverage, imported RNG, concurrency or server behavior.

The original-only descriptor-inventory transfer contract was FINAL/STABLE/HELD,
read completely and hash-verified before the grouped-value implementation.
`alpha-descriptor-inventory-transfer.md` has SHA-256
`f95f7e64dc225a94b782ba61beed0533dc1f48612a1f89d987d345d4c00e4e23`.
Its author received no engine, tests, captures, logs or runtime observations.

InventoryValue owns ordinary C++ groups and counted full item values. Semantic
assignment retains destination gaps while transferring every entry count and
payload, then the separate scalar/item/accumulators. Constructor gaps are engine
zero, distinct from the native unspecified bytes. Insertion retains explicit
routing, counted-type normalization, accumulator-only returns, identity priority
and wrapping arithmetic. The bounded collection replacement exposes ten ordered
generator calls after each count-one write without adding generator semantics.

Full descriptor input uses cold immutable ownership; refresh publishes a new
owned descriptor value without changing a generated source. Actor assignment
occurs after equipment/configuration and before the amount query. Legacy compact
generation remains a one-unit bridge, and later flat NPC consumers remain open.
This is supplied-value/transfer evidence, not complete native inventory storage,
generation, UI, allocation, concurrent publication or server acceptance.

The subsequent original-only actor-inventory consumer contract was HELD, read
completely and hash-verified before its production connection:
`alpha-actor-inventory-consumers.md`, SHA-256
`b68e27f8bdfdd6985e389b4d7bd516c7da3b064ae3fe294bcac9d7c4018f1bc8`.
Its author received no engine, tests, captures, logs or runtime observations.

One grouped NPC value now supplies real mutable health-item quantities, complete
vendor-buffer copies and counted fatal inputs. These routes keep their distinct
admissions: nonzero quantities for the health scan, all rows for export, and
positive quantities/nonzero categories for fatal units, with healing items skipped
before their scatter work. Engine event snapshots and the value-only widget view
are explicitly separate from the original allocation and presentation services.
The pre-equipment engine activation callback precedes the later inventory copy;
the amount query follows that copy. Native pointer-visible construction, alias
and lifetime guarantees, widgets/transactions, full generation, nested drop
publication and server equivalence remain open.

Two further original-only provenance audits were HELD, read completely and
hash-verified: `alpha-mode-twenty-production.md` has SHA-256
`349555670c6e1c9e89111502fa94b72e6c38d314b0421f0d7ad88ea1fb9c65db`, and
`alpha-spatial-record-association-production.md` has
`079705c0eb15e79e6663f99be3f97e37cc8dc71f0c98e8c6c8437fddddbf3365`.
They establish bounded zero/default, full-width copy, reader and lifetime facts.
They do not establish a first positive local mode-0x20 writer or a first
nonnegative spatial association/loading source. Existing raw values remain
preserved; no producer or descriptor-ordinal assignment is invented from these
negative findings. Neither author received implementation or runtime feedback.

The original-only vendor widget/transaction contract was HELD, read completely
and SHA-256 verified before the independent row/transfer implementation:
`alpha-vendor-widget-transactions.md`,
`41134b15e2792966695478dea9ebbd0c6da5ea225756d26dd551b638335a171d`.
The author received no engine, tests, captures, logs or runtime observations.
The engine separates grouped widget state from ordered services for price,
capacity, insertion, feedback and refresh. Production code uses those services;
it does not flatten stock or infer positive-count admission for vendor rows.
Player capacity compares the supplied item and wraps the complete matching
count sum. Price arithmetic uses separate binary32 operations and the original
two-evaluation boundary; platform `pow` remains an imported-math approximation
boundary rather than a native-runtime accuracy claim.

The production exporter resolves the selected actor ID on explicit refresh to
avoid retaining pointers into an engine vector. Its online compact descriptor
bridge also remains explicit. These are engine ownership adapters, not proof of
the original cached-pointer identity or timing. Panel art, negative-count icon
color, full control geometry, upstream selector priority, periodic refresh,
recipe/interface continuation, allocation/concurrency, network reconciliation
and original-client transaction/visual acceptance remain bounded dependencies.

The separate server origins audit was also HELD, read completely and verified:
`alpha-server-mode-association-origins.md`,
`5c331886f32ea40b40d5146a7140f1fe164c43ae9652a8280381e57bf111198f`.
It independently establishes server receiver identity, raw mode publication and
spatial default/copy boundaries. It still does not establish the upstream first
mode-0x20 selection or a first nonnegative spatial association. Its presence is
not certification of the engine's receive-lock sequence or whole server.

The later server publication implementation uses that completely read origins
contract for separate snapshot/decode/assignment intervals. A supplied lookup
resolves the context key during each locked interval; the wire identity does not
select the resident. Production binds this to the engine's connection map.
The decoder works only on an owned temporary. Strict malformed-payload rejection,
safe missing-key rejection and the inherited post-assignment player-controller
admission are identified as engine policies; the native local sequence does not
establish those guarantees or that controller write. No new autonomous mode-0x20
producer or spatial association is inferred. Full framing/decompression,
connection admission, map lifetime, shutdown and native server equivalence remain
separate boundaries.

The original-only projection-input contract was HELD, read completely and
hash-verified before implementation: `alpha-projection-input-production.md`,
SHA-256 `bce52cffb167fcb2887b4901d5afa4c83518670f0eb629af0fcbab196e073a07`.
Its author received no implementation, tests, captures, logs or runtime feedback.
An independent scalar constructor preserves native input widths, exact angle
constants, the half-angle binary32 store before a binary64 tangent call, the
narrowed tangent reciprocal and separate depth product/divisions. A separate
matrix product uses four rounded products and successive additions.

Production Camera uses this constructor with the existing engine's supplied
angle/aspect/near/far choices and an explicit coordinate adapter for its RH view
and Vulkan clip Y. Frame and sky compositions use the ordered matrix product.
The native main-input API exposes the two established fixed radian inputs
and signed dimension ratio, but upstream receiver/camera selection is still a
separate gate. In particular, the current engine camera's 45-degree/4000-far
policy is not certified by that native main-input contract. Platform tangent
accuracy, full control state, camera/view/origin production, every render pass,
original device publication and shader/rasterizer precision remain unresolved.

The three original-only stock contracts were HELD, read completely and verified
before the new shared generator:
`alpha-vendor-stock-generation.md` has SHA-256
`5ce6e4f4194d7fbd6ef43efc43805f7d9cc83a6aa311d8f6cd79530655ea4b9d`;
`alpha-specialized-vendor-stock.md` has
`956fb14290a454de69d77675e465fcd790188cd0492afe16683f978631712e46`;
and the independently inspected server artifact `alpha-server-vendor-stock.md`
has `767095943f53dc57f8072e66d5b4da0899aed24385d763bdec0464c9492d2333`.
The author received no engine, tests, captures, logs or runtime feedback. Client
and server facts were established separately before their matching bounded
generation relations were combined in one engine kernel.

The independent implementation stores compact candidate attributes, consumes
every specified draw, and materializes complete semantic items only for insertion.
The established unused payload fields are zero for every candidate; no random
draw or selected field is omitted by this representation. Full destination item
copying still follows InventoryValue rules. Candidate allocation/address,
exception/concurrency and original container publication are separate limits.
The fixed-pet selector rejects negative out-of-domain remainders as engine policy.
No imported random distribution, global stream or floating-control claim follows.

Town publishes a cold owned grouped value, replacing its previous flat stock
adapter. Its upstream house/position/admission remains an engine generator
boundary. In particular, the inherited house-subtype-5/class-0x83 caller still
selects the weapon route; these contracts establish the bodies reached for
0x80/0x81/0x82 and do not certify that additional caller. Wider stock persistence,
refresh, ownership and native client/server acceptance remain open.

The two original-only camera contracts were FINAL/STABLE/HELD, read completely
and hash-verified before the camera kernels:
`alpha-camera-view-production.md` has SHA-256
`0f4cc8b708c61bd55ef73e4c6d1b0480924f461c15e3a5196e49b1020524e1a4` and
`alpha-camera-follow-collision.md` has
`a16961fefc82c061f281182e8de7bd91d29b406f4c753bf79a9cb3615b58cfb3`.
Their authors received no engine, tests, captures, logs or runtime feedback.
The implementation continued in a different implementer tool after the earlier
implementer's context ended; the same separation applies, and that implementer
opened no original code, reconstruction source or source-author workspace.

CameraView is an independent scalar implementation of the supplied-input view
relations: binary32-first fixed conversions, the wrapped-product opposite,
three degree-angle row-pair rotations with cosine requested before sine,
fourth-row translations, the count-fold binary64 weight, two separate clock
reads for the shake and decay after construction. It builds the companion,
absolute and horizontally rebased matrices with raw subtraction before float
conversion. CameraFollow separately holds the mouse and wheel targets,
per-update smoothing, the strict fifty-unit snap, quantized x/y following, the
action-dependent vertical target and its binary32 raw blend, the alternative
anchor, eight quarter-unit corner samples and the final centre correction.

The contract does not state the index convention of the direction helper. The
engine reads it as the same stored-row convention the contract gives for
translation, which also reproduces the inherited view direction; this is an
interpretive bound, not an independently verified native fact. The alternative
anchor's "fixed integer fifty" is read as fifty coordinate units.

Production FollowCamera now uses these kernels for targets, smoothing, anchor
following, corner sampling and shake. Several inputs remain explicit engine
adapters: the starting angles and distance, the first-frame anchor, the block
ray march standing in for the unresolved native ray query (the distance to the
first change of blocked state), the engine world clock supplying the shake, and
the displacement of the right-handed engine eye in place of a view-coordinate
translation. The widget gates are unbound, so production takes the ordinary
route; the scale route, the second angle and the alternative anchor have
supplied-state coverage only. The engine still renders through its own
right-handed view and adapted projection: the three constructed native
matrices, their rebasing members and native field-of-view selection are not yet
the production view. Imported trigonometry, power and square root use platform
adapters. Ray geometry, widget/device producers, initial and reset camera
state, pointer lifetime and native visual acceptance remain open.

The original-only fatal inventory-unit world-drop contract was FINAL/STABLE/HELD,
hash-verified and read completely before the drop-helper correction:
`alpha-fatal-world-drop-production.md` has SHA-256
`ba7e3f568e422a877ab5b1ea3611a4e2282b8e3d05565f5a551199116dbf90b5`. Its author
received no engine, tests, captures, logs or runtime feedback. The inherited
unit loop already matched its admission, exclusion, per-unit draw order and
unchanged source quantities. The shared drop helper now forms its scale with one
binary32 product from the constructor scalar, truncates each raw coordinate
toward zero twice for the first residency lookup with the 0..65535 admission,
assigns the item by selected fields after that lookup, starts the vertical scan
one step lower for every negative raw height, and ends the scan at once for
horizontal integers outside the world range. Draw order, the early return
without an item copy or angle draw, append to the first lookup's list and key
publication after the append were already in place.

The engine's zone lists stand in for the native pointer grid, its day value for
the uninterpreted receiver snapshot, and its supplied block callback for the
column service and its three fallback records. The native scan has no length
limit; the engine keeps a bounded guard for arbitrary callbacks. Fresh record
gaps are deterministic engine zero. The helper is shared by other engine drop
callers whose own native routes this contract does not examine. Earlier fatal
loot branches, total draw counts, collection retention, later dropped-item
simulation, pickup and transport remain open.

Source-side research resumed in two new isolated author contexts started by the
implementer at the owner's instruction. Each author received only the project
requirements file, the author policy and individually named finished contracts
to locate routines, with original evidence as the sole source of facts and the
separately authorized reconstruction available as leads. Their reports to the
implementer carry only a path, hash, line count, status and generically named
open points. `alpha-camera-ray-geometry.md` was FINAL/STABLE/HELD with SHA-256
`c935b2997e30839a64400be03cdadc6448dc09897dfd4d3e00689065d6fd75fd` and was
hash-verified and read completely before use. Its author reported that the
reconstruction was not consulted. No engine, test, capture, log or runtime
finding was sent to either author.

That contract describes the same query the engine already implements from the
earlier held visibility-ray contract, and adds the two camera call shapes: both
pass option values one and one, so the walk continues through blocking cells
and side faces of spatial records may shorten the result. The production
camera now calls that existing query for its corner samples and final centre
correction, replacing the engine's own block march, and no longer bounds the
resulting distance. A final call that starts in a non-blocking cell returns
zero, which places the eye at the followed creature. The camera's separate
voxel test still uses the engine block reader. The same author's held addendum
`alpha-voxel-fallback-record-initialization.md`, SHA-256
`6b9fa857c480d9866157f5d974dde901f7016c1590c32d9df045f2f2af8f87b8`, was requested
with a source-side question only, hash-verified and read completely. It
corrects the ray contract's statement that the fallback records stay zero and
governs where the two differ: missing, out-of-range and below-column cells are
blocking, and cells at or above the column top are non-blocking, for both the
query and the caller's separate lookup. The engine's existing query and block
reader already classify those cases that way, so no arithmetic changed.
Writers through register-held addresses remain unaudited at the source side.

The fully reread projection-input contract (SHA-256
`bce52cffb167fcb2887b4901d5afa4c83518670f0eb629af0fcbab196e073a07`) now also
binds the production lens. While a creature is followed, the main projection
uses the contract's inputs: the creature's two-byte mode word selects the
angle by its 0x0400 bit, signed pixel dimensions convert separately before the
aspect division and sign-bit inversion, and the planes are the native near and
2000. The mode word is identified with that contract's tested member through
the existing record layout, in which the action byte and position sit at the
corresponding offsets. Free cameras, model viewers and diagnostic probes with
no followed creature keep the engine's 45-degree, far-4000 lens. The sky pass
keeps its separate lens. The engine still renders through its right-handed
view and the explicit projection adapter; the constructed native view
matrices are not yet the production view.

The second resumed author's contract, `alpha-class-83-descriptor-production.md`,
was FINAL/STABLE/HELD with SHA-256
`5e46ce8aee74fed70c109a600d594e1d59a169178911552bfedd611db7dd3366`, and was
hash-verified and read completely. Its author reported that the reconstruction
was not consulted and received no engine, test or runtime material. It
establishes independently in both images that no class-value dispatcher exists:
a plot-kind discriminator selects the stock body in a fixed comparison order,
the class byte is a constant written in the same branch, and the branch that
writes 0x83 calls the same stock body as the branch that writes 0x81.

This closes the earlier caveat about the inherited town caller. The existing
engine shopkeeper path already requires both nonempty collections, draws the
position index with an unsigned remainder, draws the kind as remainder two plus
two, assigns the settlement level and the class constant, and only then
generates stock, with 0x84 and 0x88 producing none and 0x83 sharing the 0x81
route. No engine arithmetic changed. The engine's house subtype stands in for
the native discriminator, whose upstream plot selection, the unread scalar
arguments, draws inside construction and position services, and the second,
stock-less template-stamping producer of class 0x83 remain open. That second
producer is not implemented as a native claim here.

The already held camera view-production contract now supplies the production
view while a creature is followed. The follow camera constructs the rebased
view each frame from its fixed eye, current angles, pre-decay amplitude and two
clock reads, so shake is the contract's view-coordinate translation and no
longer an engine displacement of the eye. The engine render origin takes the
place of the followed anchor as the horizontal rebasing origin, because engine
geometry is positioned relative to that origin with absolute heights; this is
an explicit engine adapter, and the contract's publication of the opposite
anchor members has no engine consumer. The renderer applies an explicit
right-handed adapter that mirrors the view x and z axes by sign-bit inversion.
A supplied-state comparison shows that this adapted view agrees with the
engine's previous look-at view for the same eye and angles within binary32
rounding, which also supports the stored-row reading recorded earlier. Free
cameras, model viewers and probes keep the look-at view. The view's remaining
engine consumers (sky rotation, light admission planes, frame constants) read
the adapted matrix; yaw and pitch scalars kept for the minimap, fog gradient
and sound still come from the engine direction adapter with its pitch bound.
The companion and absolute matrices are constructed but not consumed.

A further isolated author supplied `alpha-fatal-loot-branches.md`, FINAL/STABLE/
HELD with SHA-256
`85aa0329e185a699e226a797cfad1920c470013b1e0e8ee1b78ef917d20fa629`, covering the
fatal-loot helper from entry to the inventory-unit branch in both images. It
was hash-verified and read completely. Its author reported that the
reconstruction was not consulted and received no engine, test or runtime
material. The implementer noticed that one generator's stated draw total did
not equal the sum of that section's own itemised draws and put that question,
about the document alone, back to the author. The held recount note
`alpha-fatal-loot-branches-generator-recount.md`, SHA-256
`2563c20fd13fb1a1feb95d854b6a4197013220188f3fb1ab938f9cd685b8d339`, was read
completely: the itemised order is correct, the total is sixteen, and the note
governs where the two differ.

Compared branch by branch, the inherited loot implementation already agrees
with these contracts: admission and the discarded draw; the worn-item
candidate order, gate, selection between the offset and angle draws and unit
scale; the generated-item gate, level, generator choice, grade, level
adjustment and optional second item; the kind table with its scales and
parity-selected rows; the wrapped item with placement draws ahead of its grade
and generator draws; the value curve, amount, split and three denominations;
the tiered item with its score bounds, always-made second draw and unreduced
stored draw; and both generators' candidate order and fixed fields. No engine
arithmetic changed. This is independent confirmation of previously inherited
logic within the contracts' bounds, not a provenance certificate for its
original authorship. The 45-entry table behind the world-list producer, the
imported power routine, item gap bytes, the actor-field meanings and total
draw counts for a fatal actor remain open.

A further isolated author supplied `alpha-camera-initial-state.md`, FINAL/
STABLE/HELD with SHA-256
`6965b84b417adf0f78cb2a9748beecee7f0f89f6ee4d6d008329b54106ecfd6a`. It was
hash-verified and read completely; its author reported that the reconstruction
was not consulted. The follow camera now starts from the constructed values:
first angle 120 degrees for current and target, both distances zero, the
anchor zero and the option defaults 50, 80 and 0. The engine's first-frame
anchor placement is removed: the first update's strict distance snap copies
the creature position and the ordinary vertical blend then raises the anchor.
The front-end rotation assigns target distance 10 and first angle 100 and
advances the third target angle by the frame duration converted to binary32
and multiplied by the contract's constant before the old value is added.
The already implemented creation-screen snaps and the shake assignment for
record values 0x51 and 0x52 agree with the contract.

Engine limits remain explicit. Runs that skip the front end never pass the
screens that move the zero distance, so they begin at an engine distance of
six blocks as a labelled direct-entry adapter. The front-end rotation now assigns its targets
in the input phase immediately before the camera update, so that update's
smoothing sees them, as the contract orders it; its screen states stand in for
the native widget properties; and only the two creation snaps are bound. The third button handler, the menu-selection
route that restores the constructed distance and angle, the members that
construction never stores, the constructed eye and the options screen's own
bounds are not implemented as native claims.

Process note. The implementer and the source-side agents were given the same
session temporary folder. Source agents left extracts of original listings
there; the implementer listed that folder by file name only, opened none of
those files and deleted them once every source agent had finished. For a few
minutes during the last author's run the same folder also held a short
implementer diagnostic that named two engine generator functions. That author
reported that it opened no file it had not created, apart from seeing a few
lines of another source agent's header-reading helper in an error message, and
that it saw no engine material. Later source agents are to be given a separate
working folder.

A third round of isolated authors ran with separate scratch folders, apart
from the implementer's temporary folder. All three were interrupted by a usage
limit and resumed under unchanged briefs. `alpha-world-item-list-population.md`
was FINAL/STABLE/HELD with SHA-256
`237ff15b2fbdc159f70b0ee27645798b41a93606349b25dde6e2e038b92f5cb1`, was
hash-verified and read completely; its author reported that the
reconstruction was not consulted. It establishes in both images that the world
constructor fills the list read by the pet-food producer with one unconditional
run of 45 fixed values, that nothing changes it afterwards, and that the
producer picks by an unsigned remainder of one draw. The inherited 45-entry
table in the loot code holds the same values in the same order and is used
with the same reduction, so no engine code changed. The client's second reader
of the list, which passes a whole element to an unexamined routine, is not
implemented.

`alpha-front-end-widget-states.md`, FINAL/STABLE/HELD with SHA-256
`7f37c47da67b8aa1ee81a0220c18fb92527fe0562ca5727dda19f1cb5fa6fd4c`, was
hash-verified and read completely; its author reported that the reconstruction
was not consulted. It identifies the nine front-end screens by the literal
strings they are built with, their creation-time visibility, every located
transition and the owners of the three camera-snapping button handlers. The
engine's rotation screens (title, character list, world list, connection)
match the contract's admissions. The world-list Select and the Create World
button both snap the camera to distance 5, first angle 90 and third angle 180
on the way into a local world; a server connection has no snap. Because the
engine restarts into a world, the direct-entry state now reproduces that
native local-entry snap instead of the earlier six-block adapter, and a
connection starts at the front-end targets of distance 10 and first angle 100
as a labelled adapter for the lost smoothing state. The in-game menu's return
row now resets both distances to zero and the first and third angle targets to
120 and 0, leaving the current angles. The loading overlay's admission gate,
first-frame visibility defaults, the options screen and the in-game menu
key toggles are not bound as native claims.

`alpha-template-stamping-markers.md`, FINAL/STABLE/HELD with SHA-256
`501d46a358424decd714931072f1180f294a5b08360211a3a76869a1aba68355`, was
hash-verified and read completely; its author reported that the reconstruction
was not consulted. Compared marker by marker, the inherited prefab placer
already matches its traversal, crop table, column scan, pre-pass, protection
test, selector admission, all fifteen markers' kinds, positions, sizes,
draw order and clear writes, the box finder and the ordinary-cell rule. Three
differences were corrected: all five guard equipment blocks for selectors 9 to
12 now take level 20, not only the weapon and chest; the wall-plant, crest and
sign angles and the creature yaw use the raw orientation argument rather than
its remainder; and the column-value test is ordered, so a NaN value skips the
wall-plant and flower-box markers. Engine zone lists stand in for the native
feature, object and descriptor collections, the column warmth stands in for the
column value, negative orientations remain reduced, and the behaviour object,
the column-value fallback service and the uninitialised object-entry field are
not modelled.

A fourth author round ran with separate scratch folders. Two of its three
authors reported that an automated safety classifier stopped their contract
writes; no file was produced for the front-end visibility/key topic or the
client's second world-list reader, and neither was retried or re-delegated.
During that round the evidence root's two original executables stopped being
visible in its binary folder; authors that had already verified them used
hash-identical copies elsewhere under the evidence root. The implementer did
not modify the evidence root.

`alpha-column-value-production.md`, FINAL/STABLE/HELD with SHA-256
`a6fc18649ce7ba8d3177318fa2f2d1dd709955666092819a7e442c0c07fec2b1`, was
hash-verified and read completely; its author reported that the reconstruction
was not consulted. It establishes that the column value read by the stamping
markers is the distance-weighted region-point value produced by the fallback
service, written once per column by the sector generator. The inherited engine
region generator and its blend already match the contract's seed, both climate
paths, the fire branch, position draws and snap, height noise, sea handling,
window, jitter, distances, weights and the one-half fallback, and the engine
value it calls warmth is that point value. No engine arithmetic changed. The
contract's other readers, the neighbouring column members and the point-cleanup
rule are not implemented as native claims.

The follow camera now hands its own angles to the world view instead of the
engine deriving yaw and pitch back from the view direction with a plus/minus
89-degree pitch bound. The engine scalars used by the minimap, the fog gradient
and sound are the third angle mirrored about 180 degrees and the first angle
less 90, so the fog gradient's zenith angle equals the native first angle,
including straight up and straight down. The free-flight camera keeps its own
turning bound. This removes an engine adapter; it is not a new native claim.

Round five took its topics from the open roadmap items at the owner's
direction. `alpha-fatal-branch-order.md`, FINAL/STABLE/HELD with SHA-256
`a1db6ed7e7dbf0d1ee7caca901bda81f0872f35394fcf8a36436fd386afaf368`, was
hash-verified and read completely; its author reported that the reconstruction
was not consulted. It gives the complete ordered fatal branch of hit
application in both images. Compared step by step, the engine's synchronous
fatal sequence already follows it: health zeroing, the first presentation and
state reset, the pre-loot object stage, loot, the direct linked-actor health
store without a recursive fatal sequence, the credit value, regional progress,
the per-actor cleanup with credit and completion rewards, and the final backing
write. The completion reward's Platinum amount and its single parity draw
choosing between the two generators with the recipient's class as selector also
match, as do the generators' per-class draw counts and the empty out-of-range
weapon pick. No engine arithmetic changed. The unresolved virtual disposal
routines, the unattributed upstream callers and runtime values of the steering
world fields remain open.

`alpha-weapon-attachment-poses.md`, FINAL/STABLE/HELD with SHA-256
`05006c22dad53e57cfc42ca2cad7a66697c3e8e20d461eef3f37b2ad3e7515dd`, was
hash-verified and read completely. Its author reported consulting the
authorized reconstruction for leads on member layout and order, each confirmed
against the original before use. It closes the default-selector chain that
produces both weapon attachment points (creature, body, hand and weapon
matrices, the carried and held grip branches, the unconditional reference
shift and the series updates), establishes that construction leaves both points
uninitialised, and shows that the renderer's light reads see the previous
refresh's points while the later geometry reads see the current one. The
per-selector pose evaluator is bounded but not closed. No engine change follows
yet: production keeps its documented main-position fallback for the two weapon
lights until the engine's pose producer is mapped onto the contract's creature
fields and the one-frame light staleness is reproduced.

`alpha-created-actor-combat.md`, FINAL/STABLE/HELD with SHA-256
`23c4173adbce5f724407479cbde020474fd07f1f62cd609e6b426bf47f521f05`, was
hash-verified and read completely; its author reported that the reconstruction
was not consulted. It closes the combat continuation that a creature runs after
target selection, in both images. The inherited engine combat update already
follows most of it: the approach-range rules and their order, the candidate
draw, the far route's give-up reset and pursuit speed, the big creatures'
action lists and the saurian's part-dependent list, the dragon and
claw-and-jaw hops, the ranged pair rule and the melee charges. Seven
differences were corrected. The near route now clears the path in its common
prefix. The feeding branch records the feeder as the creature's contact before
its clock test. Magic users and non-magic dragons at or beyond the sixty-block
band now do nothing further instead of falling through to later classes. The
ranged and melee one-in-ten dodge draws are now made only while the dodge
lockout is zero, and their attack fallbacks also require a zero lockout. The
melee class now does nothing beyond its reach of three times the summed widths.
The engine still writes a facing yaw in this update, which the contract says
the native continuation does not; the native facing producer is elsewhere and
not yet closed, so that write remains an engine adapter. The busy indicator,
the kind-7 record handling, the magic class's aim adjustments, the 118 offset
aim, the far route's direct-approach and path-loop details and every helper
the contract leaves unaudited remain open.

The weapon light sites no longer use the main-position fallback. The engine pose
producer already keeps, for each creature, the point at the top centre of each
drawn weapon model through that weapon's full matrix: the counterpart of the
held weapon-attachment contract's main-hand and off-hand points, which are the
transform of the top centre of the weapon's box through the matrix built after
its grip translation (engine meshes are already centred, so no grip offset is
added). After each creature's lights are collected, the frame stores those two
points as absolute fixed positions; the weapon light sites read the stored
points, so lights see the previous refresh's points as the contract establishes
and later geometry sees the current ones. A creature never refreshed keeps the
main position as an explicit adapter for the contract's uninitialised storage.
The weapon matrices themselves still come from the engine pose producer, whose
equivalence with the contract's creature, body, hand and grip chain has not
been verified term by term; the per-selector pose evaluator, the reference
shift's accumulation for weapon-less refreshes and the series update branches
remain open.

Created-actor combat open items, rechecked against the same held contract
(SHA-256 23c4173a..., hash re-verified and read completely). The busy
indicator of section 2 was already in place in target selection's finish
phase: bit 0x40 set when the action is zero or the clock exceeds the halved
first duration plus the second plus the third, and the child cooldown
reduced. The tables behind those duration queries stay unaudited. The
section 6.2 special-state branch now admits an owned-flag creature whose
owner identifier is zero, as the contract's either/or condition states. After
five bites it applies the last kind-7 record with its value and time zeroed
(when one is present), then a kind-8 record with time 20000 and the feeder as
source. Each application is also an output event naming the creature twice.
The native buffer's unwritten padding bytes are stale stack contents, which the
engine replaces with zero. The engine taming fields and the legacy
continuation target cache remain explicit adapters for the kind-7 producer,
which is not closed. Section 6.4(e) step 4 is now in place: a magic user
whose action is 0x1C, at a binary32 squared separation above 25, offsets the
first two stored aim values by 1.5 times the binary32-rounded binary64 cosine
and sine of the clock times 0.005. The class's 60-block band now uses the
contract's binary32 squared separation. The stored aim is the engine body's
field-26 look vector; shot direction still comes from the engine adapter that
aims at a player target, because the contract leaves the aim triplet's
consumers open. Still open: the section 6.4(e) step 3 cast-point swap (its
S +0x1320 and T +0x1350 producers are unclosed), the type-118 beam's offset
point when bit 0x04 is set, and the far route's direct-approach helper and
path loop.
Weapon pose chain (2026-11-12): implemented from the held
`alpha-weapon-attachment-poses.md` only (sections 2, 3, 6, 8 and 9). Still
open there: the per-selector evaluator, the owner +0x58 rotations of the hand
and held-weapon matrices (no engine field mapping), the selector 0x33/0x34
swing factor (helper 0x0C6B80 unclosed), the lean condition's native inputs,
and the blend's repeat count in place of the engine's elapsed-time weight.

Cast points (2026-11-12): implemented from the held `alpha-combat-cast-points.md`
(SHA-256 d8bf7afa...dacd) and `alpha-created-actor-combat.md` sections 6.4(b)
and 6.4(e) only. The engine's cast point starts at zero where native storage
is unestablished; the client controller's local-actor latch, airship riders and
the second carried-pet pass are not implemented. Round 6 also held
`alpha-combat-aim-triplet.md` (fef0ed13...da22) and `alpha-combat-approach-path.md`
(a480a566...d1b6); the latter's author made a final wording edit after the full
readback and before the status line.

Aim triplet (2026-11-12): implemented from the held `alpha-combat-aim-triplet.md`
sections 2.3, 3.1 (origin and direction), 3.3 and 3.4 only. Still open there:
the beam's pulse timing, sampling and skip rules against section 3.1, the
drawn-aim smoothing and facing of sections 3.7 and 3.8, the speed limit of 3.9,
the look-at behaviour's distance-8 rule, boomerang steering by the owner's aim,
and the client controller's aim writers.

Far-route approach and path (2026-11-12): compared the held
`alpha-combat-approach-path.md` (SHA-256 a480a566...d1b6, read whole) with the
engine's existing search state, classifier, rebuild and companion direct walk;
they already match sections 1 to 7 term by term. The combat far route
(`alpha-created-actor-combat.md` section 5.2) now uses them: binary32 squared
separation against the squared reach (equality near), contract normalization
and the 40/80 speed rule, the direct walk only when the target is visible
(cleared path when it succeeds and contact bit 0x20 is clear), the threat entry
created at zero before scaling, a request only when the path list is empty and
the target touches something (raw endpoints lowered by half height less 0.1,
radius of both widths), and the strict 10-round loop with the 50-node and
squared-width exits. The older floating-point clear-route adapter is no longer
used by combat (the pets caller moved later the same day; see below). The far
route's interim facing write was removed with the facing work below. Section 9's follower
leads remain unspecified.

Facing and drawn aim (2026-11-12): implemented from the held
`alpha-combat-aim-triplet.md` sections 2.2, 3.7, 3.8 and 3.9. Behaviours no
longer set the yaw directly. After movement, an aiming creature (face-aim bit,
no stun) turns its yaw toward the drawn aim's heading at rate 0.1 through the
quarter-turn angle interpolation, and its head pitch toward the drawn aim's
tilt at rate 0.005. Otherwise, with no lockout, a horizontal speed above 0.02
and velocity along the movement, the yaw is set to the velocity heading. The
drawn aim then approaches the aim by the rate-0.025 share, or copies it when
the action or clock is zero or the clock is past the recovery time. The look-at
behaviour now uses binary32 squared separation below 64 and clears the bit
first. The speed limit normalizes precisely. Engine choices recorded: "blended
over the step" uses the held camera weight recurrence (one approach per
millisecond); the native "driven" list is open, so the engine's action-72
motion stands in for it; the drawn aim and head pitch start at zero, and head
pitch has no pose consumer yet. The pets' companion now uses the held direct
walk; the floating-point clear-route adapter and its test were removed.

Companion for pets and followers (2026-11-12): the pets specification (section
10.2) and the creatures behaviour specification (section 13.5) both name
Companion as client 0x43ACE0. That is RVA 0x03ACE0, the routine covered by the
held `alpha-created-companion-navigation.md`, whose discriminator-5 healing
prefix is the pet case. Pets (owner key) and behaviour-3 followers (follow key)
therefore now run the same held companion as created actors. The older engine
pet companion, with its distance thresholds, healing, goal scan and adapter path
search, is deleted.

Beam pulses and boomerang (2026-11-12), from the held `alpha-combat-aim-triplet.md`
sections 3.1 and 3.6:
- **Pulse timing:** a creature beam (actions 0x1C and 0x5F) is gated to clock
  plus step at least B and clock below B + C, with C the engine's active phase.
  It pulses first on the update that crosses B, then whenever the truncating
  quotient by 200 of the clock minus B changes.
- **Ray:** the direction is the precisely normalized aim. The length now comes
  from the held ray routine (`visibility::trace`, flags 0 and 1, maximum 200)
  instead of the engine's quarter-block march.
- **Sampling:** samples step in fixed point from the sight point. The skip
  rules follow the contract, with health strictly positive and any nonzero
  lockout skipping. The hit tests use the fixed-point horizontal sum and the
  binary32 vertical test.
- **Draws and admission:** every creature that passes the geometry gets the
  direct draw before admission. Hostility stands in for admission routine
  0x1988A0.
- **Sparks:** the no-hit sparks start at the creature's position plus the aim
  times the length.
- **Boomerang:** steering uses fixed-point differences, with the owner's aim as
  a fixed offset.
- **Drawn aim:** the drawn aim's recovery threshold is corrected to R0, which is
  the engine's action duration, the same as `actionOver`.

Not implemented: beam mana handling, the creature-map iteration order, the
id at +0x11C0, the clock rescale by query 0x047700, the water-cloud shot, and
the +0x74 reset.

### Round 7 contracts and the controller aim (2026-11-12)

Round 7's source authors held three contracts, each read whole and hash-verified
before use:

- `alpha-controller-aim-writers.md` (SHA-256 2DAAF0AC...7E25);
- `alpha-combat-beam-leftovers.md` (SHA-256 509516200A...E6AD);
- `alpha-fatal-disposal-callers.md` (SHA-256 BE8E01FA...9CB).

The new `creatures/ControllerAim` module implements the controller aim contract,
and the player's input phase calls it after the frame's world calls and before
the attack input, as in the original update:

- **Aim ray:** 100 units along the unshaken camera forward from the follow
  anchor, or the end of a 60-unit ray from that anchor.
- **Creature pick:** the nearest admitted creature whose size+1 box straddles
  the screen centre, taken at its depth from the eye when in front of the
  player. The hovered creature is kept as well.
- **Close spell:** actions 0x1E-0x21 hold their aim point once clock minus
  elapsed passes the wind-up.
- **Free control:** with Left Shift the aim is the forward of the yaw. Otherwise
  it is the float offset to the aim point, written to both the record aim and
  the body's look.
- **Flags block:** the face-aim bit is cleared, then set again for
  non-excluded actions under R0+200 and for the sustained list, with bit 0x40
  cleared alongside; Left Ctrl sets bit 0x01. The whole block is skipped while
  stunned.

Engine adapters, named in the module header:

- the pick's visibility maximum is 200 (the original derives it from a column
  query);
- the creature +0x1180 adjustment is zero;
- the projection is treated as sign-preserving;
- hostility stands in for admission.

Not implemented: the target-id write for action 0x22, and the head-pitch pose
sites.

### Beam leftovers (2026-11-12)

The held `alpha-combat-beam-leftovers.md` (SHA-256 509516200A...E6AD) was read
whole. The beam case for actions 0x1C and 0x5F now does the following:

- **Mana before the clock window:** action 0x1C drains step times 0.0002.
  Action 0x5F pays 0.3 at its first crossing, or nothing while a kind-9 effect
  is present, which it then resets and announces.
- **Negative mana:** the mana is set to zero and the rest of the case is
  skipped for that update. Action 0x5F's clock-0 deduction in the action
  update is removed.
- **Creature order:** creatures are visited in ascending signed id.
- **Draws per geometric hit:** the critical draw comes before the direct draw,
  and the critical decided there feeds the hit.
- **Clouds:** class pair (3, 1) places clouds through the engine's healing
  burst, which matches the record's radius 4, element 2 and 0.1-power
  amount. One cloud goes at the sight point plus the aim times the length on
  the first crossing, and one at the sample point for each admitted creature.
  The strike's own healer burst no longer fires for beams.
- **Empty loop:** the combo resets, and the clock is rescaled by the engine
  speed query before and after the reset. An admitted pulse zeroes the combo
  timer.
- **Driven flag:** it now follows the contract's eight setters instead of the
  action-72 adapter.
- **Climbing:** creatures in the climbing state skip the facing block.

Still adapters or open:

- `attackSpeed` stands in for query 0x047700. Its equipment term is not the
  contract's unclosed item query.
- The effect record's 35-draw particle presentation is not modelled; the
  engine sparks stand in.
- Actions 0x5E and 0x68, and the riding id at +0x11C0 (it has no producer in
  either image), are not handled.

### Fatal disposal and callers (2026-11-12)

The held `alpha-fatal-disposal-callers.md` (SHA-256 BE8E01FA...9CB) was read
whole and checked against the engine's loot and fatal code. The engine already
matched it on these points:

- the ground drop's draws and its ground search;
- loot stages L1, L3, L4, L5 and L6, including the 16 draws of generator P
  (weapons) and the 24 of generator Q (armour) with selector -1;
- level variation and its ceiling;
- F9's tail disposal, which leaves actors in place, and F11's
  out-of-range no-op.

Two fixes:

- **L2:** the worn-item stage now makes its r mod 4 draw even for a boss. The
  engine had skipped it.
- **Coins:** the coin value truncates out-of-range or NaN values to 0x80000000.

Still open, as in the contract: the attack routine's choice between miss,
block and absorption, the heal power routine, and indirect writes to world
+0xB4.

### Combat stat queries (2026-11-12)

The held `alpha-combat-stat-queries.md` (SHA-256 F45EF002...134A) was read whole.
The engine already matched it on these points:

- the attack-speed query (`attackSpeed`) and its per-item term (`itemSpeed`);
- the crit base and crit chance (`criticalChance`), and the per-item crit term
  (`itemCritical`);
- the action-specific value and the mana-cost query (`abilityValue`,
  `manaCost`);
- the power query and its weapon list (`magicBase`, `weaponsOf`);
- the weapon power multipliers (`itemDamage`);
- the B, C and D timing tables for every action 0..0x69 (`phases`).

One fix: the ray case's clouds take power x 0.1 through the new `rayCloud`.
Before, they used the projectile helper's weapon damage. Still open: whether
the platform's binary64 pow matches MSVCR110's precise pow for non-integer
exponents.

### Head pitch, pick reach and the healing stream (2026-11-12)

The held `alpha-head-pitch-and-pick.md` (SHA-256 E0994DB4...03A2) was read
whole. Implemented:

- **Head pitch:** NPC head pitch now reaches the pose. The head turns by
  (pitch + head angle) about x. The arm matrix turns by twice the pitch,
  except while climbing or for pose selectors 2, 3 and 0x25. Held weapons add
  half the pitch about x.
- **Pick reach:** the pick's reach comes from the block light at the
  creature's position and the daylight of the world clock: 256 in light,
  falling to 25.6 in darkness.
- **Pick box:** its corners are built in fixed point (point + offset +
  rebasing), x outermost and z innermost, and the comparisons are ordered.
- **Healing-stream target:** the controller writes the healing stream's
  target, record +0x190: the nearest straddling creature while action 0x22 is
  before its wind-up end, or the player itself with Left Shift.
  `Combatant::healTarget` carries it, and the creature codec reads and writes
  it.
- **Healing stream:** the stream (world case 0x214C32) is new gameplay:
  - a wind-up sound every 100 ms;
  - eight pulses over the active window;
  - each pulse pays the action cost, and is skipped when the mana falls short;
  - each pulse heals the target id, or the caster, by power x -2 x roll, then
    makes the critical draw;
  - the old clock-0 mana deduction is removed.

Adapters:

- the drawn X rotation and the +0x1180 height lag are zero, because the
  engine keeps neither;
- the light byte is the engine's propagated light;
- the projection is treated as a centred perspective;
- the third id pair takes the player-distance minimum over the straddling
  creatures, the contract's reading.

The local player has no head pitch yet; its facing is not the shared facing
block.

### Attack routine (2026-11-12)

The held `alpha-attack-routine.md` (SHA-256 853FC768...8A50) was read whole and
checked against `resolveHit`. The engine already matched it on these points:

- the entry and ground gates, and the attackerless draw;
- the beam's effect record;
- the miss (immune) and block paths and their presentation draws;
- absorption from the combo ratio;
- the item-extension effects 0x80-0x83 with the attacker's direct heal;
- the hit-flash stores and the type-1 buff product;
- the kind-0x11 buff and the critical voice table with its unconditional draw;
- the stun roll and its duration;
- the action-0x1C marker.

Fixes:

- **Block drain divisor (`damage`):** it is now m x (A x m + bonus), with the
  weapon bonuses summed first.
- **Mitigation clamp:** it keeps negative zero and NaN.
- **Impact presentation:** a magic (resistance) hit's presentation (39 or 42)
  makes no draw. The weapon sound is gated on the kind argument (not 0x1C)
  rather than on the attacker's action. Its selector is the striking list's
  first subtype: -1 when the list is empty, which gives identifier 5. The
  engine's inferred bare-hands case is gone.
- **Draw rounding:** each presentation draw now uses the contract's order,
  (draw x factor) / 32767 + offset.
- **Striking list:** `strikingWeapons` exposes it (0x03DF60).

The healing-stream (beam heal) case was implemented in the previous
checkpoint. The area heal, site 1 (traps) and the call sites' own damage
derivations remain open.

### Local player facing (2026-11-12)

The held aim contract (§3.7, §3.8), with the beam-leftovers contract §7 (every
live creature, the local player included, reaches the facing and drawn-aim
blocks), now drives the local player too. The NPC block was moved into
`faceAfterStep`, shared by the NPC step and `Player::advance`. It covers the
driven list, the action-0x48 spin yaw, the climbing bypass, the turn toward
the drawn aim with head pitch, the velocity heading and the drawn-aim
smoothing.

The player's own climbing turn toward the wall (travel §6.4) stays in place of
the unclosed +0x11A0 rule. The player's head pitch reaches the pose.

Engine adapter: the controller writes no aim until the follow camera has been
placed once. The contract leaves the first update after construction open; an
unplaced zero anchor would aim at the world origin.

### Hazards, blast and ticks (2026-11-12)

The held `alpha-hazards-blast-ticks.md` (SHA-256 136BC339...4A96) was read
whole.

**Site 5 (type-4 buff ticks):** `tickBuffs` and `periodicHit` already
matched it:
- the 400 ms quotient of the buff's own remaining duration;
- expiry without a tick;
- the type-1 stun clear;
- resistance mitigation, no block, a zero vector and the source lookup.

**Site 2 (bomb blast):** `Npcs::blast` now follows it:
- victims in ascending signed id;
- dodgers skipped (+0x128), and NaN health passes;
- the fixed-point horizontal reach, trunc(((0.7 size x + 8)^2) x 65536);
- the binary32 vertical test, ((victim height x 0.5) + bomb height) + 8 > |dz|;
- the knock-back vector (presentation-unit direction, z 0, normalised above
  0.01, z 0.25, x 10);
- one doubling draw per victim;
- D = attack power (`strikeBase`) x 25, doubled.

The bomb's role 6 makes the admission test pass for every victim, as before.

The block-removal sound draw (§4.2) now precedes the victims. The engine
adapter `Npcs::blastRemovesBlocks` asks, read-only, whether the 8-block
natural sphere holds a removable block. When it does, the pitch
0.5 + 0.4 x draw/32767 is drawn before the first victim and kept on the
explosion record. The client and server still remove the blocks after the
step and play the sound with that kept pitch. The removal itself running after
the step is an engine ordering adapter; it makes no draws.

Still open:
- Site 1's zone traps (type 7, generation routine 0x100300 / client 0x1CC510).
  No engine generation pass maps to that routine yet, and adding the trap
  draws would change world generation.

## Strike call sites (round 10, held 2026-11-13)

Held contract `docs/specs/alpha-strike-call-sites.md` (SHA-256 8849C420...43E7), written by
a fresh source author after the round 9 author was stopped, read whole by the implementer.

Implemented:
- `strikeWindow` now returns the window and the area-heal flag (S5-S9) with integer clock
  arithmetic. The periodic interval clears both sets on entering a new interval; the wind-up
  and the window end run the deferred-key pass. A zero interval never strikes (engine guard;
  the contract leaves division by zero open).
- `strikeGeometry` / `strikeReaches` (K2, K5): the precise-trig rotated forward offset in
  fixed point, the aim centre of 0x1E-0x21 and their raised start point, action 0x45/0x4B
  centres lowered (the engine had raised them), the fixed-point horizontal and binary32
  vertical reach tests. The geometric test takes option 1 and maximum 200.
- `Npcs::strike` visits the creatures in ascending signed id. Order per target: health
  (NaN continues), struck set, reach, geometric test, admission, then a dodging creature
  (+0x128 nonzero) is deferred instead of struck. Admission keeps the engine's sides as the
  stand-in for the unaudited hostility routine 0x1988A0.
- The deferred-key pass leaves keys in `Combatant::counterPending`; `Npcs::counterPass` runs
  the counter callback (`counterDodge`, shared with the type-4 hit application) and appends a
  damage-0 type-4 record without the hit routine. Online, the client sends the type-4 record
  and the target's owner runs the counter.
- `afterStrike` (§5): action 0x1E's Cw(0.25) kind-9 effect and record 47, the swing count on
  the first target (struck set empty and not stunned), the pair (4, 0) crit charge on every
  charged success, and mana on the frame's first uncharged success with the contract's
  rounding. Action 0x1F's kind-9 reset moved from the action update to the strike (K4).
- Area heal (§6) for 0x20/0x21 of class pair (_, 1) on flagged frames, after every target
  that reached admission: roll, heal power x -0.1, then the critical draw; record 42; the hit
  routine. Role-6 targets are left out (engine stand-in, as the projectile heal does).
- Beam (B4-B6): the second per-target draw is S4 = (heal power x 1.5) x (draw x 0.05 / 32767
  + 1), so `beamInput` no longer draws a third time; class pair (_, 2) drains (a self record
  of -S4 and a direct health gain up to the maximum); the swing count follows every target
  that reached the call.
- The healer swing's extra healing burst (older open-points inference) is removed: the
  contract's post-hit work places no cloud.

Still open: the true hostility routine (target sets), block breaking at K3 (the engine's
crater pass from the block-edits contract stands; its sound draw is still taken after the
step), the 0x1314 field, the online client's area heal and geometric test.
## Trap generation and trap ticks (round 10, held 2026-11-13)

Held contract `docs/specs/alpha-trap-generation.md` (SHA-256 3E0195FE...4199), read whole.
It identifies server 0x100300 / client 0x1CC510 as the dungeon generation pass. The engine's
dungeon builder (`worldgen/Dungeon.cpp`, `furnish`) already produces the type-7 objects at
the contract's point: after the openings and the floor fill, gated by the entrance kind, the
vertical-step flag and the large-layout byte, with the mod-40 gate draw, the mod-4 rotation
draw and the mod-4000 phase draw, then straight to the arches. No generation change was
needed; the earlier note that no engine pass mapped to 0x100300 was wrong.

With the traps known to exist, `StaticStates::traps` now follows hazards-blast-ticks §3:
- the firing tick is the 200 ms quotient of the world clock (+0x8000BC), kept by the traps
  as a 32-bit counter advanced by each frame's time (the engine had used 100 ms of the time
  of day);
- creatures are visited in ascending signed id, the range test first, then role 1;
- P1 and D use the contract's binary32 order (normalisation by the rounded double root,
  x 20, z 5, type 8 x 2; 2^((L - 1) x 0.25) as a double power rounded, x 10, then x 5).

Still open: the hits still bypass the attack routine (attackerless path with its E2 draw,
the type-6 resistance / type-7/8 armour mitigation and the hit routine's draws); zone
selection near role-0 creatures; the contract's level-at-position routine (the engine's
level adapter stands in).
## Drawn-height offset, shield release and beam particles (round 10, held 2026-11-13)

Held contract `docs/specs/alpha-beam-height-leftovers.md` (SHA-256 B0961250...0B86), read
whole. The engine's landing offset (`Body::bob`, subtracted from the drawn height by the
renderer and the follow camera) plays the role of creature +0x1180. Now per §1:
- step-up snap: the new z is 65536 x layer(z - T(h)) + T(h) + 66191, with layer the
  truncating quotient minus one for negatives; the offset becomes F(new - (old - T(offset)));
- step-down snap: layer(z - T(h) - T(1.2)) on the terrain grid, same lift and offset rule;
  the ground probe one layer at z - T(h) - T(0.2) uses the same layer rule;
- a static-object hit withdraws step-up and step-down for the rest of the update;
- decay once per millisecond of the step in binary32, 0.01 below zero, 0.02 otherwise
  (NaN included; the engine had used a power form and sent NaN to 0.01);
- the unstuck lift still sets the offset to 0.

Already in the engine and matching: the player's bar entry 1 = 8 with a shield, its release
to 0x68 at clock 0 with no further test (§2.2); NPCs' 0x68 from the charged block (§2.3); the
N = 0 NaN path (§3); the particle consumer's seven draws per particle (r1..r6, u), 35 for a
beam record, with received records after local ones (§4).

Engine extensions / not modelled: the world-object base term b of §1.2 (no world-object voxel
models, so b = 0); the attachment copy and the interpolation zeroing of §1.6 (no attachment
or remote-interpolation counter in the engine). Contract note: §1.3 condition 5 reads both
"0.0 > d.z is false" and "d.z <= 0.0; a NaN fails"; the engine keeps d.z <= 0.

### Trap hits through the attack routine (2026-11-13)

From the held `alpha-attack-routine.md` (§3 E1/E2, §4 M6-M12, §6 site 1): each trap hit now
calls the engine's attack routine (`resolveHit`) with no attacker, at the trap, in visiting
order, with damage D, flags A and B clear, the push P1 as V, the mitigation selector (type 6)
and the blockable flag. The attackerless path makes one discarded draw, sets the target's
+0x1184 (the engine's `hitFlash`) to 0.5, skips E3-M5 (so no armour or resistance applies;
the earlier note expecting type-6 resistance / type-7/8 armour mitigation was wrong),
multiplies the damage by the target's type-1 (damage-taken) buffs and emits a main record
with attacker key 0, no stun and no action marker. The server resolves a connected player on
its record snapshot with its buffs; its client applies the hit.

Engine stand-ins still open: the hit routine runs at the engine's existing delivery points
(the offline player at once, creatures in their next step) instead of inside the trap loop;
zone selection near role-0 creatures; the level-at-position routine.

## Hostility test (round 11, held 2026-11-13)

Held contract `docs/specs/alpha-hostility-test.md` (SHA-256 B10605EA...3884), read whole.
`creatures::hostilityTest(source, other)` implements §2.4 in order: the same object never
(identity, not id); either role 6 always; exactly one role 1 always; two role-1 creatures
when their exemption (the §2.3 predicate, the engine's existing `peacefulAnimal`, whose list
and appearance mask match) differs; otherwise mode bit 0x20 of either. It reads no health,
makes no draws and is symmetric. Wired at the callers of §2.7:
- the melee strike loop and its area-heal check (the engine-side sides stand-in is gone;
  role-6 objects are now struck by everyone and town NPCs no longer strike players);
- the beam target test (was the behaviour-side `hostile` in both directions);
- the nearby-hostile contact flag for lunges, for players and creatures;
- both projectile collision loops (replacing the local `canDamage` copy, now with identity);
- the client's target choice (`ControllerAim`, local player first);
- the online client's strike path (role 6 no longer excluded);
- the bomb blast loop (equivalent: the bomb is role 6), matched to caller 0x20F192 by its
  struck set and reach test only.

Engine extension kept and identified: a creature's swing also lands on its quest battle's
other side (required engine quests). The behaviour-side relation (`hostile`, 0x196CA0) stays
for target choice; the write-up identifies it as a distinct sibling but does not specify it.
Still open: the geometric test's ray-march second phase; the online area heal.

### Online strike path: order, line of sight and area heal (2026-11-13)

From the held strike-call-sites and hostility contracts: the online client's strike loop now
visits the listed creatures in ascending signed id and follows K5 in order (health with NaN
passing, struck set, reach, the geometric test with option 1 and maximum 200, the hostility
test, dodge deferral, the strike). A non-hostile target goes to the §6 area-heal check, which
is also re-evaluated after each struck target: actions 0x20/0x21 of class pair (_, 1) on a
flagged frame send the area-heal record (its two draws) as this client's hit and play
sound 42. The online client had skipped the geometric test and the heal.

## Target relation (round 12, held 2026-11-13)

Held contract `docs/specs/alpha-target-relation.md` (SHA-256 BCF310E5...81EC), read whole.
The engine's `hostile(a, b)` already implemented §5 exactly (role 5 against role 0 never,
role 6 never, one role 1, differing exemption, toggle bit; no identity rule), and its four
callers already matched §7: target choice (`TargetSelection::consider`, including the pet
food exception and the chooser-exemption acceptance), the heal-threat walk in hit
application, threat propagation within 8 units at 0.5, and the healing projectile branch
(owner role 1 halves it, role 6 excluded). No behaviour change. Cleanup: the duplicate
exemption predicate `harmless` is removed in favour of `peacefulAnimal` (the same 32-race
list and 0x1A00 mask), and `hostile` is documented as the target relation.

### Strike block breaking, K3 (2026-11-14)

From the held `alpha-strike-call-sites.md` K1-K3: block breaking now runs inside the strike on
each strike frame (the K1 window), after the reach and centre (K2) and before the setup and
target loop. A striker wider than 4 (body size x, ordered), not in action 0x57, 0x5B or 0x4A,
asks the engine adapter `removesBlocks(centre, n)` with the exact strike centre and
n = min(trunc(R), 8); when any cell goes, the sound-2 pitch (scaled unit draw x 0.4, plus 0.5)
is drawn there, before every target draw, and the sound plays at the striker. Main and the
server remove the cells and emit the per-cell bursts after the step. Replaced: the older
block-edits §9 path (its own reach table, which disagreed with K2 for actions 0x48/0x4C-0x4E,
fired at the hit moment, drew the pitch after the creature step and played at the crater);
`impactReach` and `craterRadius` are removed. The blast adapter is the same `removesBlocks`
with radius 8. Still open: the cell test 0x20AA60 (the natural-block flag stands in), the
cell lookup's rounding for negative coordinates, and the branch at 0x139CC8.

## Strike cell helpers (round 13, held 2026-11-14)

Held contract `docs/specs/alpha-strike-cell-helpers.md` (SHA-256 CF2CD012...4CEC), read whole.
`naturalSphere` is replaced by `breakCells(centre raw, radius, rounding, day, read)`, shared
by the strike (K3) and the blast:
- offsets x outermost, z innermost, kept while radius² (binary32) exceeds i² + j² + k²;
- the tested cell is looked up with floor-adjusted division (§2.1: an exact negative multiple
  of 65536 lands one block lower); the test is flag 0x20 only (§3). `Zone::read` already
  supplies the lookup's fallbacks (grey below, water/air above, water for an unflagged empty
  cell at z <= 0), none of which carries the flag;
- the removal record carries the air value and the day (§5) at the strike's truncating or
  the blast's floor-adjusted indices (§5.3); applying it already writes what §4 writes
  (water with the keep flag at z <= 0, air above unless kept);
- each burst starts at the unsnapped cell position with the tested cell's colour (§8.1
  step 6); the engine had used the cell corner and re-read the colour at the record cell.
Main and the server share one `breakAround` helper per program. The plant pass (§7.2)
already matches `dropFloating` (same fallback rule) but runs over the whole touched zone
rather than the rectangle. Not modelled: the server-only light pass when world +0xB8 is
null (§7.3; engine lighting is the client's own), the blast's critical section, and the
zone change vectors' network/save use (unresolved in the contract).

## World objects: airships (round 13, held 2026-11-14)

Held contract `docs/specs/alpha-world-objects.md` (SHA-256 3F3557F9...856C), read whole; a
first author context safety-stopped before writing and a fresh one wrote it. The list is the
original's airships (model 0xA03, `airship.cub`). Implemented:
- **Templates (§2.1):** `worldgen::airshipTemplate` computes the identifier K(zx, zy, n), the
  heading r x 90, the home 19 blocks out along (r + 2) pi / 2 and 66 above the plot top, the
  target 100 back along the heading and 100 up, and the destination (+50, +50, +20), in the
  contract's binary32/binary64 order. `Town` adds one after each landing-place prefab when
  any of the eight region cells round the zone's own has positive elevation (+0x18 of the
  region record, the engine's `RegionCell::elevation`). Engine identification: the
  contract's zone x * 256 + A + Px is the town builder's plot centre `cx(p)` (plot origin
  plus half its size), r is `Plot::face` (+0x14) and H is `Plot::hi` (+0x04).
- **List (§2.3, §2.4):** `creatures::Airships` drops ships whose target is 512 blocks or more
  from every player, then copies each template of the player zones (3 x 3 round each player)
  once a player is within 512 blocks, posed at the target. Offline (unconnected) only on the
  client; the server runs it for its connected players. The original server never sends
  ships, so connected clients have none.
- **Collision (§3, §4):** after the terrain and static tests of an axis, a body with an
  obstacle list tests each ship within 64 blocks (squared length 4096) in its model frame
  (binary32 rotation by -yaw, half-size offset), voxel by voxel; every hit ship takes the
  body's key and sets the sub-step base (ship z - T(half height)). A z hit stands the body
  (landing flag). The step-up attempt also needs the raised box (1.01 added in the model
  frame) clear of every ship, and its snap is relative to the sub-step base. Pure red voxels
  are emptied when the model is set.
- **Frame update (§6.2, §6.3):** target += T(seconds x velocity); the pose follows by the
  per-millisecond factor; carried creatures follow the pose change (position, follow point,
  yaw) and leave the set when gone or 64 blocks from the target.
- **Drawing (§7):** the client draws each ship at its current pose turned by +yaw with its
  grid centred on the position.
Not modelled (open; a steering author round is running): the course steering of §6.1 (ships
keep the template's zero velocity, so they hover at their target), the yaw-follow helper,
the companion spawn and creature placement of §6.4, the unstuck routine's ship part, the
out-of-grid shared cell writes of §5 (treated as empty), and the yaw field at +0x137C. The
server cannot carry players (their bodies are their clients'), so their keys leave the set.

### Airship steering, yaw follow, placement and push-out (round 14, held 2026-11-14)

Held contract `docs/specs/alpha-airship-steering.md` (SHA-256 CF69EB99...97E4), read whole.
- **Steering (§1)**, unconnected worlds only (the client offline, the server): courses
  1 -> 2 -> 3 -> 0 -> 1. Course 1 heads home (velocity = offset within 10 blocks, else 10
  blocks a second); courses 2 and 0 fly at 10 blocks a second to home +- 50 along the heading
  and 20 up; course 3 approaches the destination horizontally (30 a second beyond 30 blocks,
  per-millisecond approach at 0.001) and holds 100 to 150 blocks above the column's stored
  top, its target yaw following the horizontal velocity; courses 0-2 turn the target yaw to
  the heading at 0.005. All in the contract's binary32/binary64 order.
- **Angle helper and yaw follow (§2):** a + asin(sin(b - a)) in degrees, then the current yaw
  approaches it at 0.01 per millisecond, every world.
- **Companion (§4):** the town builder now appends the companion spawn record (radius 512,
  role 3, kind 4, class byte 0x88, the location's level, the ship's target and yaw, key
  +0x10E0 = the template identifier) before the template, so the zone's finishing pass and
  spawn identifiers include it as in the original (this also restores that pass's draws for
  towns with landing places). Activation is the engine's spawn pass. Its single-element
  behaviour object is untraced; the engine's default behaviour stands in.
- **Placement (§3):** after its movement each creature keyed to a listed ship is set on the
  model's first pure red voxel (x, y, z loader order) turned by +yaw about the grid centre,
  half its height plus 0.001 above it, when more than one block away, and takes the ship's
  yaw.
- **Push-out (§5):** after the terrain unstick, per ship in key order, a box overlapping its
  voxels moves to the nearest free of the four horizontal escapes (1.05 / 0.05 margins),
  turned back to the world by +yaw.
- Carrying now moves the facing used for strikes (`Npc::yaw`, the player's facing).
Still open: the finishing step's effect on the companion (boss/size bits), the behaviour
object, the health gate of the unstuck routine (the engine body has no health), and the
world values read by the spawn pass's window test.

### Creature in-range combat: exact arithmetic (2026-11-14)

Audit of `Npcs::combat`'s near route against the held `alpha-created-actor-combat.md` §6:
the class order (a)-(i) and their decisions already matched (the dragon class is tested
before the magic class but excludes magic users, which is equivalent), and `actionOver`
is the R0 test. The arithmetic now follows the contract instead of double precision:
- d² is the binary32 square of the separation conversion D (§3 step 9, §6.3), used by every
  band (64, 3600, the melee reach (S + T) x 3) and D is the stored aim;
- the golem smash leaps at the 64-bit doubled T - S converted like D (§6.4(c));
- the dragon and claws-and-jaws lunges normalise D by the reciprocal of the rounded double
  root before x 10, and their height tests compare the raw signed height with 524288 /
  131072 (§6.4(f), (g));
- the evasion uses D's horizontal part with the same reciprocal, the perpendicular as
  (u2, -u1) and z 0 + 5 (§6.4(h)).
Open (round 15 author running): the helper tables 0x044270, 0x03CAA0, 0x047310, 0x03D1A0,
0x045410 and 0x0456A0 (the engine's primary/secondary attack starters stand in for the last
two), and the busy indicator bit 0x0040 of §2.

### Combat helper tables (round 15, held 2026-11-14)

Held contract `docs/specs/alpha-combat-helper-tables.md` (SHA-256 F2938586...A55B), read
whole. It closes the six helpers the created-actor combat contract left bounded. Every part
already matched the engine, so no behaviour change was needed:
- A/B/C (0x044270, 0x03CAA0, 0x047310) are the engine's `phases` recovery, wind-up and
  active terms (spot-checked code by code, including B's bracelet row for 0x25/0x2B/0x59 and
  the fixed rows for 0x5B, 0x5D, 0x5F, 0x65, 0x69); R0 (0x03D1A0) is `duration`.
- The busy indicator (A/2 + B + C) is `TargetSelection`'s readiness threshold.
- The melee chooser 0x045020 is `primaryAttack` (all 15 rows and the R0 + 200 combo rule);
  the special chooser 0x0452A0 is `secondaryAttack` (7 rows, parity of +0x70);
  the cost query 0x044AE0 is `manaCost`.
- Starter 0x045410 is `startPrimaryAttack` (lockout, action 0x30, R0 minus A when the count is
  nonzero except actions 0x1E/0x20, inclusive clock test); starter 0x0456A0 is
  `startSecondaryAttack` (no exclusion, class (4, 1) exemption for 0x05/0x11/0x14, mana
  cost, the kind-9 wind-up preset, lockout cleared unless exempt).
New regression cases pin the less obvious rows. Open (contract): the client-only callers of
the helpers and the meaning of the counters they read.

### Airship pilot behaviour (round 16, held 2026-11-14)

Held contract `docs/specs/alpha-airship-pilot-behaviour.md` (SHA-256 8729395A...1436), read
whole. The pilot's behaviour object is a sequential behaviour holding one look-at-player
element (the ordinary one). The engine adds behaviour 8 for it: `lookAtPlayer` alone, and
when it declines the sequence zeroes the movement input. `lookAtPlayer` now disqualifies a
NaN-health player (the original's ordered >= 0 test). Already matching: the 32 ms behaviour
cadence and stun gate, and the finishing step for kind 4 (five appearance draws; it cannot
set the boss or the 0x1000 flag) plus the follow-on equipment step's class-0x88 draw
(`DescriptorEquipment::setup`). So the pilot is never a boss and never doubled in size.

## Remote creature smoothing (round 16, held 2026-11-14)

Held contract `docs/specs/alpha-remote-creature-interpolation.md` (SHA-256 24F15463...9551),
read whole. (Process note from its author: one recursive search of the reconstruction's alpha
folder returned file names, some in third-party subfolders; no third-party contents were
opened. This concerns the source side only.) Implemented on the connected client:
- `Client` records each applied creature record as a receipt with the original's decision:
  snap for a new creature, a changed 24-byte snap field (record +0x1B0) or old health with
  0.0 >= health; otherwise smooth (§1.3, §1.4).
- `net::RemoteCreatures` keeps per creature a live body (every received field, §1.6), advanced
  each world update by the ordinary movement step (§3.2; the unstuck routine is skipped for
  received creatures, `Body::replica`), and a drawn pose: a copy while the flag is 0, else the
  position closes (d x T(f)) / 65536 per axis plus a second binary32 vertical step, with
  f the per-millisecond 0.015 approach, and each rotation component takes the angle step
  (§2.1-§2.3). Dead creatures run no update (§2.5). Unlisted creatures are dropped (§1.5).
- Remote creatures are drawn, lit and shattered at the drawn pose and drawn yaw.
- The angle step 0x023F70 is now one exact shared routine (`creatures::angleStep`) used by
  creature facing, airships and this smoothing; the earlier facing helper rounded the radians
  and the sine difference differently.
Not modelled: the drawn aim (§2.4), the type-1 batch message (the engine protocol sends type
0 and end-of-update), and the gated server-only spans for remote creatures beyond the unstuck.

## Pose selectors (round 16, held 2026-11-14)

Held contracts, each read whole: `docs/specs/alpha-pose-selectors-01-18.md` (SHA-256
448DC82B...D969), `-19-24.md` (BED44C9D...C1F2), `-25-30.md` (7FEB0CCB...D004) and
`-31-49.md` (0F5A4CCF...7F56). The selectors 0x19-0x30 were researched in two narrower
rounds after the first author stopped. `src/creatures/Pose.cpp` now evaluates every selector
case from these contracts:
- A mapping block names each contract term after its engine field (hand, foot, arm, torso,
  head, tail and shoulder offsets, the lift, the auxiliary scalar, the turn flag, the blend
  rate and the main and off-hand model slots).
- Clock comparisons against the wind-up, active and recovery bounds are integer, as in the
  original. Shared helpers express the repeated approach, orbit and rest formulas once.
- Arm swing is scaled by 0.1 for 0x33/0x34; the rest-hand formula, the arm mid-points and the
  knockdown offset (x/y order) follow the contracts; sitting is gated on clock < R0; a
  cleared selector zeroes the pose clock; a new off-hand scale field carries 0x47's 1.5.
- Where the earlier general pose contract disagreed, the selector contracts win: 0x15 reads
  no mount, 0x3A reads the mount, 0x42/0x43/0x47 write the model slots (0x47 model 2101).
Open (commented in the source): the owner hand-turn targets (+0x148/+0x154) and the
+0x1314 spin variants, 0x3A's mount terms, the consumers of 0x29's head and tail values, the
foot spread factor, +0x40 read as intent in 0x36, and the quadruped rest height.
### Remote facing and drawn aim (2026-11-14)

From the held `alpha-remote-creature-interpolation.md` (§2.4, §3.1, §3.2) and
`alpha-combat-aim-triplet.md` (§3.7, §3.8): no client gate in the creature loop excludes a
received creature from the facing block (0x21F29A-0x21F62B) or the drawn-aim store
(0x21FD54), so `net::RemoteCreatures` now runs the shared `faceAfterStep` after each
received creature's movement step. The receipt lands the mode word (face-aim bit), dodge
time, aim (record field 26), head pitch (field 5) and the decoded fighter on the live state;
the facing turns the live yaw and head pitch from the previous drawn aim, then the drawn aim
moves toward the aim and is replaced by it between actions. Remote creatures are drawn with
that head pitch. Engine choices: a new creature's drawn aim starts at its aim; the received
action clock stands for the clock before the step (whether a remote clock advances locally
is not closed). Still open: the driven-flag list, the attachment copy path.
## Pose owner inputs (round 17, held 2026-11-14)

Held contract `docs/specs/alpha-pose-owner-inputs.md` (SHA-256 14B1999F...EEB3), read whole.
- Owner +0x148 / +0x154 (record fields 24, 25) are now `Combatant::handTurnLate` /
  `handTurnEarly`, decoded and encoded with the record. Constructed creatures keep zero (the
  contract finds no nonzero local writer); received creatures carry the sender's values.
  Selectors 7, 0x0F, 0x10, 0x30 and 0x31 now take their hand-turn targets from them.
- Owner +0x1314 is `Combatant::swingCount`, client-local. `updateSwingCount` holds the
  world update's two writers: a pulsing action (the strike pulses' set and periods) crossing a
  pulse boundary inside its active phase takes the combo; a clock not past the wind-up takes
  the combo. It runs for NPCs and the local player before the action effects, and for received
  fighters on each refresh (keeping the value across snapshots; a new clock counts as a zero
  step, an engine choice). Selectors 0x0F/0x10 (variants 2/1) and 0x1E/0x1F (variant 2) now
  take their spin terms from half the swing count modulo 3.
- Already matching: owner +0x144 (charge) and +0x13B4 (spent power) writers as used by the
  held strike and attack contracts.
Open: the traced-path gates before the swing-count block (0x212CB2), resets A/B in the engine,
the blend repeat count, models entries 840 and 2101.
Repair: comments in `Protocol.h` and `RemoteCreatures.*` (pushed in dfdbfa1) had been
double-encoded by a PowerShell 5.1 read; they are restored to UTF-8 (text unchanged).
## Provenance re-certification, batches 1-2 (2026-11-14)

Twelve inherited contracts (class C of `docs/provenance-audit.md`) were checked for
suitability on the clean side (behavioural prose only), re-certified by fresh source-only
authors against the original images (no reconstruction consulted; 45 corrections recorded in
each document's re-certification section) and then read whole by the implementer:
- `alpha-relationship-kind-89-admission.md` (3518E683...1CE6)
- `alpha-hit-scalar-reduction-order.md` (C65BD469...3E5E)
- `alpha-forward-link-zero-gates.md` (9CA7413C...DF1F)
- `alpha-fatal-object-descriptor-association.md` (9B30688A...A082)
- `alpha-inventory-equality-consumers.md` (9A5ADC42...A9AE)
- `alpha-data-stores.md` (1BA38C37...219C)
- `alpha-category13-inventory-counter.md` (7516D24D...BE15)
- `alpha-generic-hit-vector-adjustment.md` (8045574D...2628)
- `alpha-yaw-trig-scalar-abi.md` (53188905...BF30)
- `alpha-fatal-context-field-producers.md` (AAD508BA...A9F4)
- `alpha-world-creature-health-admission.md` (2B7DAAE1...2A02)
- `alpha-linked-item-storage-producers.md` (639AF16A...6E95)

Engine changes:
- The hit reduction's action-84 test compares the action byte (hit-scalar contract).
- The orphaned role-5 pet also takes action clock 5000 (forward-link contract).
- `makePet`'s owner helper performs its own lookup; only a found owner caps the level,
  writes the reverse identifier, rescales the first multiplier and takes the forward link.
  The pet reset leaves the reverse identifier zero; creation writes it itself.
- Taming (offline and online kind-6) sets the tamer's forward link to the tamed creature
  (fatal-context frame assignment); the engine's own taming admission stands in for the
  native selector-7 record test.
Kept deviations: the data stores open read-only and immutable (the owner installation must
stay untouched); the adaptation price fix (owner extension). Not modelled: the inventory UI
byte +0x29 gate on the fallback swap; the sound request parameters; the selector-1 world-byte
exit; the subtype 0x18 -> 0x19 writer; the 0xE38-byte item-block clear.
## Provenance re-certification, batch 3b (2026-11-14)

Four inherited rendering contracts were re-certified by a fresh source-only author against
the original image and its embedded shader programs (21 corrections; no reconstruction
consulted) and read whole by the implementer:
- `alpha-light-admission-plane-production.md` (059CAC28...4D5C)
- `alpha-hit-item-presentation-vector.md` (D1952744...9564)
- `alpha-opaque-output-transfer-state.md` (0B989F9D...CFEE)
- `alpha-terrain-color-pipeline-public.md` (3A28AC82...046F)

The engine already matched all four, including the one reversed claim: the ordinary opaque
terrain's dark fill is (0, 0, 0, 1); the sky average belongs to a preceding pass (the engine's
sky-average darkness exists only in the cloud pass). Only a comment changed
(`WorldView.cpp`: the tint condition is the voxel-type-2 test, with no held "submerged"
meaning). Not modelled: the early admission read before plane publication and the renderer's
critical section; the 80-byte node and list-length guard of the item-effect append; D3D9
device/reset formats, GUI render surfaces and gamma ramps; the previous-value retention when
the fog distance is not positive. Open engine question: a device without B8G8R8A8_UNORM falls
back to its first format, which could be sRGB; the contract authorises neither choice.
## Provenance re-certification, batch 3a (2026-11-14)

Four inherited contracts were re-certified by a fresh source-only author against the
original images (12 corrections; no reconstruction consulted) and read whole by the
implementer:
- `alpha-creature-light-attachment-producers.md` (12A6F25F...5233)
- `alpha-client-buff-event-transmission.md` (C60C344B...F8D7)
- `alpha-spatial-association-producers.md` (0BF3B6D5...67F2)
- `alpha-fatal-object-association-producers.md` (1374AB45...3B13)

The engine already matched them (buff residents: types 1, 2, 3, 6, 7, 9, 10, 11 replace all 24
bytes, every other type appends; the selector-17 producer's second-creature source; the
remote-creature blend toward the same creature's live state; the spatial association never
written; the fatal snapshot list). Not modelled: the native P/reference adjustment order over
the four attachment series (the engine keeps trail points relative to the displayed position);
the model-point transform and collection argument; the server's double target lookup, list
node size and packet-9-after-packet-8 loop order; apply replacing immutable zone fields.
## Provenance re-certification, batch 4 (2026-11-14)

Five inherited contracts were re-certified by a fresh source-only author against the
original image (16 corrections; no reconstruction consulted) and read whole by the
implementer:
- `alpha-terrain-light-source-lifecycle.md` (041392BD...331E)
- `alpha-worldgen-clouds.md` (7A47B72F...B1B9)
- `alpha-periodic-hit-producer-state.md` (C0BAC1F0...9DB2)
- `alpha-backing-reference-producers.md` (DCBABA05...6290)
- `alpha-creature-yaw-light-gates.md` (EC18275D...3099)

Engine changes:
- Cloud height (`Clouds.cpp`): the base H + 100 M and the lift 50 N + 170 are each made fixed
  (binary32 x 65536, truncated); the base is clamped at zero in fixed point and the lift added
  as an integer. Placement moves by under 1/65536 block; which cells hold a cloud is unchanged.
- The dying kind-144 creature's fading light needs the shared light view test
  (`WorldView::lightViewAccepts`) with culling radius +0x88, tripled for action 106. The
  native distance argument at this site is unresolved; the engine supplies the terrain
  gather's distance (fog + 22.4). Out-of-view bomb deaths no longer light terrain.
Not modelled: the Sleep(5) of the quarter-duration branch (a native timing side effect); the
two presentation draws near 0x0AD3B9; the per-frame retry of the kind-144 trigger byte (the
engine stays one-shot per death); the online client's kind-144 flash; the flash joining the
same frame's candidates (the original appends after the gather); cloud gates sampled at the
camera column with floor (native: the controlled creature, truncation); the cloud render-state
setup. Engine extension kept: optional cloud drift.
## Provenance re-certification, batch 5 (2026-11-14)

Six inherited contracts were re-certified by fresh source-only authors against the original
images (34 corrections, one reversal: the hit applier's fourth argument is an object used only
by death loot as the drop container, not a binary32; no reconstruction consulted) and read
whole by a fresh implementer:
- `alpha-server-buff-update-admission.md` (A5807A9E...2BE6)
- `alpha-hit-response-vector-consumer.md` (F89F3714...D009)
- `alpha-client-buff-action-boundaries.md` (8C910875...A098)
- `alpha-hit-application-mutations.md` (C91D1B73...DDA1)
- `alpha-hit-threat-death-lifecycle.md` (26479873...E484)
- `alpha-open-points-9.md` (2E1D4AF7...60C5)

Engine changes:
- Server day clock (`net::advanceServerClock`): before the world step the server scans live
  role-zero creatures (connections and controller-0 NPCs) for action 0x54 with health > 0; when
  one is found the time of day advances x100, else x10. The engine's day rollover is kept.
- Column extension (`Zone::place`): new cells of an upward extension are zero-filled (type 0,
  colour 0, 0, 0) instead of white air; only stored colour bytes of air change.
Already matching: the hit applier's prelude, gates, category bodies, healing traversal and
fatal join; the death-loot draw order, tables and parity generator choice; the formation guards
and call-site draws; the AI accumulator, interval counter, type-9/10/11 producers and
consumers; the knock-back subdivision count's binary32 rounding; the secondary-action
admission (selector pure, so the clock-first order is observably equal).
Not modelled: the position-list distance test of the formation pass (its list is unidentified);
the type-6 gap fill of the native writer (the engine fills implicit solid; formations never
write below the column bottom); the positive-timer state helper and cooldown refresh; the
native socket reader's error/close index behaviour (the engine decodes whole buffers).
## Provenance re-certification, batch 6b (2026-11-14)

Two inherited contracts were re-certified by a fresh source-only author against the original
image (21 corrections, five reversals in player control; implementer advice, logging
procedures and replacement diagnosis removed; no reconstruction consulted) and read whole by
the implementer:
- `alpha-player-control.md` (A8FEC1FB...B6AF)
- `alpha-open-points-11.md` (1EEEA981...0328)

Engine changes (player movement feel changes):
- Input (`Player.cpp`): each held key (forward, back, left, right), then the analogue pair
  when non-zero, takes the first applicable rule: climb, dodge, swim, tired glider, walk. With
  the dodge key held and enough stamina, an input that cannot dodge does nothing (no steering in
  the air with the dodge key held). A key dodge goes along that key's unit direction; a stick
  dodge uses the unnormalised stick vector. The analogue pair swims even in action 107. Action
  52 ends after the movement rules. The gamepad stick (owner extension) is the analogue pair.
- Wall normals (`Physics.cpp`): reset each step; all four axis probes are tested in order, each
  writing one component (a corner gives (-1, -1)).
- Ground friction is withheld in action 48, in action 54 until P1 + P2, and in actions 6, 7, 10
  and 17-20 until P1.
- The scalar relaxation helper (bank, swim pitch, boat) and boat x/y friction step per
  millisecond with rounding instead of a pow shortcut.
Already matching: regeneration only in action 84 for kind 0; the local day clock x100/x10; the
1.1-block unstick lift once; absent zones not solid; default and water substitution blocks;
20 ms sub-steps; frame-dependent jump boost; the collision box bytes.
Not modelled: the target lock (the engine aims by pick), input acting on a separate mounted
creature, the negative-health unstick skip (health never goes negative), diagonal probes, the
input-call interface gates. Open: block-index conversion at exact negative multiples (physics
contract) and the climbing lateral sign convention (probe frame unresolved).
## Provenance re-certification, batch 6a and network transport (2026-11-14)

Four inherited contracts were re-certified by fresh source-only authors against the original
images (42 corrections; reversals in vegetation ground cover and five in the transport;
implementer advice and the "what a compatible implementation must do" sections removed; no
reconstruction consulted) and read whole by the implementer:
- `alpha-worldgen-random.md` (2DD98F1E...9264)
- `alpha-worldgen-vegetation.md` (16905CC1...3F04)
- `alpha-house-furniture.md` (24048CF4...87EA)
- `alpha-net-transport.md` (7C9727E1...CC3B)

World-generation changes:
- Plant flags start at 2; flagged kinds OR in 4 (6); reeds stay exactly 4 (bank reeds remain the
  owner extension). The renderer tests bit 4 only, so the picture is unchanged.
- Tree crown size and height use the native binary32 grouping (`treeCrownSize`,
  `treeHeight`); sizes can differ by one in rare last-bit cases.
- Tree density re-reads moisture at the tree's current position and caps at 1 before the bonus;
  for warmth below 0.3 the kind is always 1 or 2. Kept or skipped trees and changed kinds shift
  every later draw of the zone's stream.
Network changes (wire-compatible with original peers):
- Sender pacing (`SenderPacing`): each pass sleeps what is left of 20 ms since the stored stamp,
  then stores the clock read before the sleep, so passes alternate (about two per 20 ms); one
  stamp per client image and one shared by every server sender thread.
- The server sender waits for the receiver thread to end before freeing the connection.
- Per-player zone index truncates toward zero (`serverZoneOf`).
- A packet-0 stream that fails to inflate keeps its partial output (`zlibInflatePartial`);
  engine choices: a stalled stream stops and the output size is bounded.
Engine choices kept and labelled: client socket errors go offline and shut the socket; unknown
packet ids end the connection; a 5 s handshake timeout, a thread per join and a reachable
"server full" reply; console quit on Q or q; the seed packet read in the connect routine; size
bounds. Already matching: generator streams, seed offsets, noise vectors, temperate ground
cover, plant and ground-item centring, lily height, house furniture rules. Not modelled: client
world re-initialisation on a seed or name change; mid-field close semantics; the once-per-second
zone request resend.
## Provenance re-certification, network server and saves (2026-11-14)

Two inherited contracts were re-certified by fresh source-only authors against the original
images (34 corrections; reversals: the server's fallback seed follows srand(123) and at least 77
draws, its "server full" reply is unreachable, zones are saved and unloaded by the generation
thread, and appearance and creature type are derived on load from the style bytes; replacement
advice removed; owner save files were not opened) and read whole by a fresh implementer:
- `alpha-net-server.md` (BB684E10...FA2A)
- `alpha-saves.md` (DD42AA73...5BEC)

Server changes: the day rolls over only above 86,400,000; `time` is saved at 10,000 ms
boundaries of a step accumulator and not at shutdown; the step marks zones modified and the
generation thread saves a zone when it unloads it (once per second, if modified or holding
edits), with every held zone saved at shutdown; the main loop replaces the generation request
list each pass and the generation thread builds at most one zone per pass (nearest first,
squared zone distance below 9, never outside 0..65535); shutdown order simulation, generation,
connections, zone save; the join record spawns at z 0 with the two item records at 0x980 and
0xA98; "Player <slot> joined." uses the zero-based slot; players and NPCs are listed together in
id order under the one-new-creature-per-tick permission (NPC radius in binary32, skipped only
above 40,000); burst and sound sections keep records strictly below 40,000 (sounds were
previously unfiltered); a server.cfg without a number is no longer overwritten.
Save changes: CREATE TABLE runs on every open with failure ignored; the write error is printed
only when the INSERT cannot be prepared; the character reader is truncation-tolerant and the
writer always writes 11 skill levels; character and world lists keep one record per key, so an
unreadable record no longer shifts indices (fixing a slot-overwrite bug); single-key character
save, add and delete in the native order; native world deletion (remaining entries rewritten
under the selected key); the start position uses only the per-world table; the session clock
rolls over only above 86,400,000.
Engine choices kept and labelled: the 5 s handshake timeout, a reachable "server full" reply,
Q or q quit, a thread per handshake, unknown packets close, packet size bounds, join health
100, the 1.2/1.2/2.7 box, worlds matched by name, a fresh session clock, the read-only open
mode and entry caps; the unseeded first rand() as fallback seed (native value unresolved).
Not modelled: the join inventory and type-44 appearance; region unloading and region-record
saves; the second world table; the 60 s character autosave; client zone saves only at unload;
the map's discovered counter; the race-7 hair colour copy (the generic appearance routine
forces 255 for types 13 and 14).
## Provenance re-certification, open points 7 and 8 (2026-11-14)

Two inherited dungeon-generation contracts were re-certified by fresh source-only authors
against the original images (14 corrections, no rule reversed; recording comparisons and
implementer remarks removed; no reconstruction consulted) and read whole by the implementer:
- `alpha-open-points-7.md` (9EC97116...A59D)
- `alpha-open-points-8.md` (F9051468...4A37)

The dungeon builder already matched both (link tables per s, corridor and line rules with the
stair flag 0x01 on the lower cell, cell pass order and draw costs, ceiling, walls, torch and
furniture tables, arch conditions and rotations, boss order). Only comments changed in
`Dungeon.cpp`. Open: the native arch stamping's draw count is unresolved (the engine's mode-0
placement draws none; if the original draws there, every later dungeon draw shifts); the
furniture style is the zone's byte +0x79 natively, which the engine takes as v (kinds only, no
draws).
## Provenance re-certification, regions and item generator (2026-11-14)

Two inherited contracts were re-certified by fresh source-only authors against the original
images (20 corrections; reversals in regions: sea elevation exactly -100, truncating index and
window divisions, type 14 inside the queue pass; verification vectors removed; no
reconstruction consulted) and read whole by a fresh implementer:
- `alpha-worldgen-regions.md` (99AB8D3E...9C86)
- `alpha-items-generator.md` (37B95AC2...4A5B)

The engine already matched: cell creation, the sea elevation in effect, displacement and owner
rules, the truncating blend window (two indices near the low world edge), region preparation
(slot and queue passes, type 14, 15, 11/12 snaps, height clamps, counter order), the item
candidates, materials, parities and draw counts. Changes, none affecting generated worlds: the
cell snap and owner search use the truncating forms; the location lookup's slot index
truncates (negative columns outside the world read slot 0); the rarity roll keeps the signed
remainder for a negative argument (no caller passes one); divide-fault guards are commented as
engine choices.
Open discrepancy for the locations re-certification: the regions contract says the slot pass
stores level 1 at +0x24, while the engine applies a climate-based minimum (1/10/15/20) taken
from the not yet re-certified locations contract; zone levels depend on it, so it is unchanged.
## Provenance re-certification, projectiles, world items, open points 1 and 2 (2026-11-15)

Four inherited contracts were re-certified by fresh source-only authors against the original
images (69 corrections; reversals: the projectile landed flag resets every sub-step and a
shooterless projectile skips the eligibility tests; the no-attacker hit path leaves velocity
untouched and pushes through the application vector; world-item tag bit 0 lowers generated
records onto the ground; low non-exempt statics do block sideways path moves; verification
material and implementer remarks removed; no reconstruction consulted) and read whole by a
fresh implementer:
- `alpha-projectiles.md` (A76F2F03...99DD)
- `alpha-open-points-1.md` (2DF6B799...5030)
- `alpha-world-items.md` (6EBB945D...3613)
- `alpha-open-points-2.md` (11067776...5195)

Engine changes:
- Projectiles: the action-dependent weapon set (bow/crossbow alone; action 96 off hand then
  main hand; otherwise main hand plus an off-hand bracelet) and 2^g in binary64; bow and salvo
  products in the contract's order with the record's strength; the type-101 spell offset and the
  class-3 element on the thrown rock; the healer burst only for action 94 at the end of a native
  200-block ray from the attack origin (`Npcs::attackOrigin`); time rescale ratio first;
  16.16 boomerang catch; kind-3 sub-steps computed normally with a creature scan each; still
  projectile knock-back (0, 0, 3c); destroy, landed and wall flags reset per sub-step; world-unit
  overlap with a strict upper bound; native line of sight and flight ray; no eligibility tests
  without a shooter; contact and burst dodges recorded in the shooter's evaded set; burst
  half-size 5 with shooter-only exemptions; a landed hit advances the combo and resets its timer.
- Beam ticks clear the evaded set; trap level lookup uses the raw position with the y cell via a
  binary64 multiply by 1/2048 (`World::levelAtFx`); seat release on hit-stun without touching
  the action, local worlds only.
- World items: tumble while T != 0, hop while T > 0; generated records with tag bit 0 clear settle
  block by block onto solid ground (world generation); the mana-cube counter filter.
- Physics: on z, statics are tested after a cell hit unless already standing on an object; the
  static approach test truncates (fixed difference x d) and boxes use <= on lower and < on upper
  faces (creature-static collision changes; path search already matched).
Random-draw order: the native ray and line of sight, the 5-block burst, multi-sub-step kind-3
scans and per-sub-step landed flags can change which creatures consume critical, damage and
proc draws.
Open conflicts between held contracts, sent to a source-side ruling: the trap pass period
(100 ms in open points 1, 200 ms in hazards-blast-ticks) and the beam tick (100 ms of t - D in
open points 1, first crossing plus 200 ms in aim-triplet); the engine keeps 200 ms meanwhile.
Not modelled: the 89/90 channel sound for remote creatures, the +0x11C0 exclusion, field 18,
statics in the projectile ray, the action-104 beam entry and water-mage visual, seat re-pin to
actions 83/84, physics 3x3 index cells.
## Provenance re-certification, terrain and open points 5; tick-period ruling (2026-11-15)

Two inherited contracts were re-certified by fresh source-only authors (22 corrections;
reversals of two former open-question readings about stored water keep marks and dry air;
verification vectors removed; no reconstruction consulted), and a new source-side ruling settled
the conflicts reported in the previous batch; all three were read whole by the implementer:
- `alpha-worldgen-terrain.md` (85542660...6975)
- `alpha-open-points-5.md` (79648F2B...BE98)
- `alpha-tick-period-rulings.md` (EED29498...5B62)

Rulings: the trap gate for kinds 6 and 7 is trunc(C + dt)/200 against trunc(C)/200 with C the
world clock after the step's advance (`alpha-hazards-blast-ticks.md` §3 is right; the 100 ms
divisor in `alpha-open-points-1.md` is wrong). Actions 28, 94 and 95 pulse once at the wind-up
crossing, and 28 and 95 repeat each time trunc((t - D)/200) changes (`alpha-combat-aim-triplet.md`
§3.1 is right; open points 1's 100 ms period, missing crossing tick and separate action-104
entry are wrong; action 104 dispatches to a different case, so the triplet's claim that 0x68
shares the beam case is unsupported). The engine already followed the 200 ms rules; where
`alpha-open-points-1.md` disagrees, the ruling document governs.
Engine: the terrain contract already matched (the 0x3ECCCCCC divisor is now written by its bits;
the elevation window uses the truncating form, identical for valid coordinates); the item
settling pass converts with the native fixed-to-block rule (exact negative multiples of 65536 one
block lower; generated ground items have whole-block z and do not move). World generation, block
colours and draw order are unchanged. Not modelled: the terrain pass's P and grass counters and
running min/max of zs (uses untraced); the mana cube's unwritten angle.
## Provenance re-certification, caves, spawn objects, loot rolls, open points 10 (2026-11-15)

Four inherited contracts were re-certified by fresh source-only authors (43 corrections; one
reversal of a former open-question reading; recording comparisons, notes for a replacement and
test vectors removed; no reconstruction consulted), and a new source-side ruling settled two
points the implementer raised; all five were read whole by a fresh implementer:
- `alpha-worldgen-caves.md` (A9408400...C172)
- `alpha-spawn-objects.md` (74201FDE...654E)
- `alpha-loot-rolls.md` (129F3C13...51D8)
- `alpha-open-points-10.md` (EA64C0E2...B9C1)
- `alpha-rulings-formation-angle-box.md` (46278FCC...064C)

Ruling: the kind-4 site's formation angle is 2i·pi/n (loop multiplier stepping by 2, times pi,
over n, rounded to binary32; sine then cosine), superseding the caves contract's "i·pi/n"; the
appearance box is written as 0x3F75C290, 0x3F75C290, 0x400A3D71 (0.96 one ulp above nearest),
which the engine already used.
Engine changes: empty spawn items carry level 1 (levels below 2 never re-roll, so draw counts are
unchanged); the type-9 guardian's z is the centre column's bottom; the centre-in-zone tests
truncate (`centredIn`); the E.kind = 5 dungeon call is added (no writer known, fresh worlds
unchanged); single-creature followers only when the entry's second list is non-empty (no current
set is empty). The kind-4 formation angle stays a full circle (the implementer's half-circle
change was reverted after the ruling). Already matching: terrace pools and their skipped draws,
the camp routine, ravine and boulder passes, the equipment generator's branches and slot order,
the finishing pass, faction and ring draws, loot slots, recipe parity, re-roll wrap, dungeon
glow rates and carve order. Not modelled: constructor write order, inventory +0x1094/+0x1098 and
the +0xF7C item, the 0x1DC6E0 branch choice, home placement with a null model, the leader's
+0x48 argument. The type-9 guardian z has no test (no such location near home).
## Provenance re-certification, network packets and skills; knock-back/snap ruling (2026-11-15)

Two inherited contracts were re-certified by fresh source-only authors (32 corrections; two
reversals in the packets: world-update section 3 is only appended to a client-side list, and the
"event 40" relay in section 12 is withdrawn; replacement advice, test vectors and recorded values
removed; no reconstruction consulted), and a new ruling settled two conflicts the implementer
raised; all three were read whole by a fresh implementer:
- `alpha-net-packets.md` (9E49926D...A52C)
- `alpha-skills.md` (CAD7F796...1533)
- `alpha-rulings-knockback-snap.md` (51D80D29...0B10)

Ruling: a kind-1 hit's vector is copied into velocity (not added; never written to +0x4C), and
the display snap tests the OLD health (0.0 >= old, NaN does not snap); the packets contract's
§5.1 and §2.3 are wrong there. The engine already did both.
Network changes (wire-affecting): the creature delta compares typed fields (item and appearance
equality), so uncompared bytes set no mask bit; packet 1 (creature batch) and packet 3 (static
objects) are read instead of disconnecting; send order 0, 10, 6, 7, 8, 9, 11, 12; only section 2
feeds hit handling (section 3 goes to bursts); a non-positive chat count is ignored; the NPC
radius applies only to non-zero controllers; duplicate zone and region requests are each
answered; hit kinds other than 1 and 5 take the damage path; a type-13 item in section 9 adds ten
times its field 0x10 to the local player's experience, and a type-25 one sets action 0x6D;
/name, /namepet, /connect and /disconnect match the exact first word.
Skills (visible): skill 9 is positioned and clickable but not drawn; @ridingspeed shows 0 % below
1; ":", "/", ")" are punctuation only by exact equality; word widths truncate; the cube term is
c/4 signed; the start screen closes the skill window and inventory; non-zero movement intent
closes the inventory, crafting and skill windows.
Kept engine choices: unknown packet ids disconnect; length bounds; merged zone item lists;
per-edit chunk remesh. Not modelled: world flag 0xB8; type-25 effects on other creatures; the
further section-9 routine; packet-3 storage; the window-close nodes and second sound source.
The private reward-flow probe expectation was updated: the two platinum gifts add 20 experience
(XP -13 instead of -33), as the held packets contract requires.
## Provenance re-certification, travel, particles, crafting, open points 3; walk-flag ruling (2026-11-15)

Four inherited contracts were re-certified by fresh source-only authors (76 corrections;
reversals: exhausted climbing falls with a key held and hangs without one, the particle cull
and leaf radii follow the view/fog distance, crafting over-removal works per bag, action 104's
beam branch is unreachable and a charged throw stores its charge at P1; implementer advice,
game-check procedures and the notes for a replacement removed; no reconstruction consulted), and
a ruling settled the conflicts the implementer raised; all five were read whole by the implementer:
- `alpha-travel.md` (DF3C8719...9B64)
- `alpha-particles.md` (3F1398F3...C1F9)
- `alpha-crafting.md` (195FF715...4595)
- `alpha-open-points-3.md` (478AD903...BAD9)
- `alpha-rulings-walk-flag-pitch.md` (EF0F037C...C579)

Ruling: the walk-flag clause at 0x098318 runs only with Left Shift NOT held and clock < R0 + 200
(or a sustained action), setting 0x04 (except actions 0x57/0x58/0x59/0x5C) and clearing 0x40,
also while gliding: `alpha-controller-aim-writers.md` §4 is right and `alpha-travel.md` §2.2
step 3 is wrong. The head-pitch decay (pitch x float(exp(float(step) x -0.01f))) runs every step for
every creature first, then the aim blend; the climb-hold path skips both. The tick-period ruling
and open points 3 agree (its P1 is the ruling's D).
Engine changes (movement feel and visuals): player input runs before the mode-flag update and both
are skipped during hit-stun; exhausted climbing follows from that order; blend weight iterated in
binary64 and noise pi as the double 3.1415927; a wall crash keeps lift and wind with stun 1500;
fall stun raised to at least 1500; the head-pitch decay as ruled, with the climb-hold test on the
glider flag; G toggles only subtype 0 (glider) and 1 (boat), and the glider folds on another
Special subtype; the pet under its rider copies its whole orientation. Particles: cull radius
0.2 x (fog distance + 22.4) through the shared view test (`WorldView::viewAdmission`) and leaf
reach 0.2 x fog distance + 32 horizontally (the believed view/fog floats map to the engine's fog
distance); staff release spread 8 at the fixed-point aim position; the fire explosion window
T - dt < D <= T; collision cells floor (one lower at exact negatives), light cells truncate;
collision sub-step d x (1/m); shatter voxel order x, y, z; a dead creature failing the view test
loses its once-flag without shattering; the action-28 burst at the target's record position.
Crafting: the level-change notice and sound 0x1D play at the player on any level change; unlisted
stations 0 and 2 always present; an ingredient-free listed key passes the button check; the row
count needs a non-zero type (the adaptation price fix extension is unchanged). Open points 3:
saurian model draws head, body, hand, tail (random-draw order); the unreachable action-104 sound
16 removed; player action flames subtract the landing bob.
Not modelled: obj +0x144 and several pet orientation fields; the preview panel's own lights; the
start-up tab icon snapshot; P+0x1D40; the disenchant path; the native single spent-power field
(the engine keeps separate ranged and melee fields); action 104 in the default handler.
## Provenance re-certification, path search, locations, item names, trees (2026-11-15)

Four inherited contracts were re-certified by fresh source-only authors (53 corrections;
reversals in path search: the search round skips ignored statics, and the follower result is
explicit; replacement advice, test vectors, dictionary display text and checking procedures
removed; no reconstruction consulted) and read whole by a fresh implementer:
- `alpha-path-search.md` (6AA57674...2DC9)
- `alpha-worldgen-locations.md` (B1D8A5A8...72BB)
- `alpha-item-names.md` (0BD58670...9688)
- `alpha-worldgen-trees.md` (8A4292D9...E27E)

The locations re-certification settled the open discrepancy from the regions batch: RVA 0x1EE4B0
stores a climate minimum level (1/10/15/20) at +0x24 in the slot pass for owned non-town records;
the queue pass overwrites ranked records; the rest carry it into zone-table levels. The engine
already did this, so the regions contract's "level 1" holds only for skipped slots and the town.
Engine changes: NPC pathing - the shared walk/spawn/interaction loop tests the node map once and
stops strictly inside r; SpawnLocation runs the usual loop after its extra round (up to 11
expansions in the starting update); WalkPath starts a search only when its path list is empty
(an abandoned search waits for the 3000 ms re-plan). Locations - candidate lists of 32 or fewer
keep slot order on ties (`sortCandidates`). Item names - a matched adjective tag without "@"
falls back to the entry's first child, "#" replaced in both branches (English names unchanged);
the healing gate for actions 80/81 uses the native condition (the +0xB4 flag read as "not a
network replica", an engine reading) with signed truncating ticks. Trees - the two conifer top
cells are written once per spoke without the dry-air read (same final blocks); centres and the
kind-5 reads use the native truncating and read conversions (identical for non-negative play
coordinates). Not modelled: the WalkPath radius source and Cube.exe path counterparts; the
long-list tie order; region +0x04; the second healing effect, the 400 ms effect spawn and the
" receives " form for others; the native record node layout and palm stamp internals.
## Provenance re-certification, server simulation and physics/camera (2026-11-15)

Two inherited contracts were re-certified by fresh source-only authors (59 corrections; reversals:
the region unload routines' roles, NPC AI only with the online flag clear, all four wall probes
run, the action-54 charge speed window, the step zeroing intent and velocity in attachment
branches, the map zoom direction and the fallback camera ray; replacement advice and procedures
removed; no reconstruction consulted) and read whole by two fresh implementers working on
separate files:
- `alpha-server-simulation.md` (E1197055...7389)
- `alpha-physics-camera.md` (3CF792FF...92DF)

Server changes (multiplayer): a healing item raises the server's copy of the player's health and
its hit carries attacker 0; the inn searches by home spawn in table order (flag 0x80 ends it) and
rest deletes only non-pet NPCs of loaded spawns and resets their countdowns; a silent kill of a
mission target uses its home zone's map cell, needs a mission, zeroes the death timer and runs no
death routine; schedule entries overwrite a persistent active position; despawned and deleted
creatures are stripped from every threat and hit-counter map; pet revival and the packet-6 pet
reset clear targets; a new step makes everyone forget dead targets; world items count down only in
active zones; unloading drops a zone's spawn state and, with no player within fewer than 3 zones,
saves a region's cells and its "missions generated" flag; fallback mission rewards go to every
connected player. Active zones are visited in ascending (zx, zy) order (an engine choice for the
native memory-address order).
Physics and camera changes (movement feel): box block indexes truncate then go one lower when
negative; truncating wall-probe columns with the facing-gated corner probes; the action-54 charge
speed while time < P1 + X; ground slam needs time < duration; intent clears the lens/sitting mode
and ends actions 83/84; action 81 then 83 with S = 0; dodge cancels actions (not 50/96 in their
duration); climbing and the ledge shove test the glider flag; a running action clears climb and
glider; static-object hits take the full axis response; fall damage at the moved position in
fixed-point stages; the stuck flag needs stun <= 0; kind 6 never in water; boat friction first;
end-of-step glider and lantern equipment checks, bob decay and the dodge sound for any creature;
`separationPush` with fixed-point overlap and the unenlarged scale for the player. Camera
defaults, zoom, smoothing and focus already matched.
Conflicts reported for rulings: the camera's corner offset (contract +/-0.5, camera-follow contract
+/-0.25), the meaning of mode 0x400 (sitting versus lens), and the map's pan rule.
Not modelled: the attachment branches; 3x3 static sectors; the listed-screen focus override and
per-screen orbit; NPC-side copies of the new step outputs (to follow); the second per-region block;
spawn activation gating for unnamed worlds.
The private player diagnostic expects the lantern bit cleared without a lantern equipped.
## Camera, lens and pan ruling; NPC physics follow-up; clean-room page (2026-11-15)

A fresh source-only author settled the three conflicts the physics/camera batch reported, in
`alpha-rulings-camera-lens-pan.md` (3DFB4512...143C), read whole by a fresh implementer:
camera collision corners are +/-0.25 block from the first eye point (the camera-follow contract is
right); mode bit 0x400 is the lens (30 degree field of view, zoom crosshair, a pose override and a
skill-scaled meter), cleared by intent, with no native setter, and is not "sitting" (native sitting
is action 83); the world map pans by (dx, -dy, 0) x 4/s rotated by yaw while the middle button is
held, and the half-rate drag rotates the customization-bench preview (both already matched).
Engine changes: the 0x400 identifiers are renamed to the lens meaning and the dead sitting field
removed; the lens pose override is drawn for the player, NPCs and remote creatures (quest
prisoners now show it); the HUD shows the zoom crosshair while the bit is set; the stealth meter
reads the bit; the engine setters (ability 99, the engine-quest prisoner) are labelled engine
choices. NPC physics: `separationPush` with the unenlarged scale, the skill fallback, glider and
lantern equipment flags, the step's action changes copied back (intent ends 83/84, 81 to 83, dodge
cancel), the spin yaw, and the thud (one draw, sound 32) and roll (sound 26) for NPCs while a local
player exists - a random-draw-order change. Not modelled: the game byte +0x14 crosshair gate; the
meter's action-99 skill term and action-79 branch.
`docs/clean-room-specification.html` is a standalone page summarising the clean-room process,
interoperability facts, functional requirements, references and conformance tests; where it and
the repository documents differ, the documents win.
## Interaction, open points 4, item services, block edits; melee reach ruling (2026-11-15)

Fresh source-only authors re-certified `alpha-interaction.md` (C36D72FC...096F), `alpha-open-points-4.md` (89D5C8CC...D8F5),
`alpha-item-services.md` (D8DF28CE...74E7) and `alpha-block-edits.md` (D08F1B86...DE7B); a ruling author settled the one
conflict between held contracts in `alpha-rulings-melee-reach.md` (CD2114A8...703C): a melee creature beyond
reach takes the common exit only (the created-actor combat contract is right; the open-points-4
claim is not), which the engine already matched. A fresh implementer read all five whole.
Engine changes: interaction targeting uses one pass test with squared reach (4 blocks, not 16),
a creature prompt and the free-pet claim; the revive prompt comes first, talk needs look mode,
riding needs the owner and the revive press skips the use block; chest drops take three draws per
unit (y, x, angle) through `WorldItems::dropAt` - a random-draw-order change; a lever finds its
partner by descriptor index; static-state steps place blocks through a callback with a uniform
draw scale; the runestone uses draw() % 4; the ranged 25-to-55 charge needs a main-hand item; the
customization bench uses the native click priority, a 3D grid ray limited to 1000 steps and the
cursor label; item text takes a name line; the middle button sells or unequips.
Not modelled: talk/vendor, open-points-6 and location-creature corrections (re-certified, to
follow in the next implementer batch).
The clean-room page gains a heading style for its sub-sections.
## Talk and vendors, open points 6, location creatures (2026-11-15)

Fresh source-only authors re-certified `alpha-talk-vendors.md` (F9CE6289...4CEF), `alpha-open-points-6.md` (590CD98B...2022)
and `alpha-location-creatures.md` (F12F3502...3434); a fresh implementer read all three whole.
Engine changes: the talk partner is cleared before the talk press at 5 blocks (squared distance
25) and the window keeps its last copy (a vanished partner is also cleared - engine policy);
speech bubbles advance before the partner check and follow after it; each voice blip takes one
shared-stream draw (pitch 1 + 0.5r/32767, sound 0x32 at the speaker) - a random-draw-order
change; NPC talk reseeds the shared world stream with the id (the innkeeper neither reseeds nor
draws, explored talk reseeds without a draw); the map mark runs only while a new bubble's text is
built, never for the innkeeper, with the 0..65535 guard and truncating division; the follow share
iterates in double; empty page segments are skipped; carriage return is no longer a tokenizer
delimiter (speech text normalises CR LF to LF - engine policy while the parser question is open);
the sale coin sound plays at 0.75. Location waypoints read their block cells through the
negative-coordinate block rule (one block lower for negative z); camp fire and prop records take
state 1. All twenty open-points-6 corrections were already matched (kind-11 critical draw,
action-30 and beam draws, class 0x88 draw, velocity rescale never runs, actions 51/75 stun).
Not modelled: the event-record talk restart (no producer); the map-cell existence test and zone
summary copy; raw static-record bytes; the +0x11c0 beam exclusion and action 94 (no native
producer); online clients running the melee block for every creature.
Conflicts reported for a ruling: the beam area flag for action 95 (open points 6 against the
strike call sites and attack routine) and the strike origin z sign for actions 69 and 75.
## Creature behaviour; beam flag and strike origin ruling (2026-11-16)

A fresh source-only author re-certified `alpha-creatures-behaviour.md` (B1E2181A...5DD8); a ruling author
settled the two conflicts the talk/open-points-6 batch reported in
`alpha-rulings-beam-flag-strike-origin.md` (DF4859D6...453F): the beam sets the area flag for action 95 (area
value 0.1) and the strike origin for actions 69 and 75 subtracts half of (bz - bx) in fixed point
(the strike call-site contract is right on both; open points 6 is not), which the engine already
matched. A fresh implementer read both whole.
Engine changes: armour flag 0x20 adds the unmultiplied tier base for players too; spawn records
default the +0x60/+0x64 pair to -1; the type-144 health drain applies under any controller;
random walk uses the shared created-actor wander (fixed-point leash, unnormalised x-then-y intent
draws, timer draw, action 0 with the clock kept) - a random-draw-order change; the taming bite
counter is the combo counter (the separate bite count is removed); a buff's cooldown read creates
a missing entry at 0. The other corrections were already matched.
Not modelled: the unresolved items U-1 to U-12; +0x13E0 and spawn +0x6C/+0x70; uninitialised
record bytes stay zero.
Conflicts reported for a ruling: the knock-back velocity rescale in hit resolution (creature
behaviour against the attack routine), the taming success branch (against created-actor combat
and pets/bombs), and where pending-charge finalisation runs (against common action processing).
## World generation structures (2026-11-16)

A fresh source-only author re-certified `alpha-worldgen-structures.md` (D5C7DD6B...0DBB); a fresh implementer
read it whole. Most corrections were already matched (stair cells on the lower level, line
direction from room A to room B, the v = 5 shaft heights, face-specific carve noise, kind-18 ruin
models, vendor material parity, storey eligibility, settle-test reads). Engine changes: dungeon
ring creatures and escorts take +0x08 = 150; the unreachable kind-8 plinth builds five centred
steps (896 draws); the ruin centre-zone test divides toward zero. Generated worlds are unchanged
(all zone comparisons pass).
Not modelled: the escort route copy (patrol waypoints unresolved), the boss helper object, the
house grid +0x18/+0x1c fields (no reader), and items tagged unconfirmed.
Conflict reported for a ruling: the along-x third corridor line test (structures "x and y
exchanged" against the explicit extents in open points 7).
## UI screens; knock-back, taming and charge ruling; corridor third-line ruling (2026-11-16)

A fresh source-only author re-certified `alpha-ui-screens.md` (9DB638FD...FC7F); ruling authors settled the
creature-behaviour conflicts in `alpha-rulings-knockback-taming-charge.md` (0110F496...0E2D) (the knock-back
rescale never runs; the in-range combo branch pacifies and taming happens in the action-110 effect;
pending-charge finalisation runs on every route) and the corridor conflict in
`alpha-rulings-corridor-third-line.md` (1B10F83E...48E2) (open points 7 is right: three of the four along-x
third-line bounds use x extents). The engine already matched both rulings. A fresh implementer
read all three whole.
Engine changes: location keys cover variants 0-9 for eleven types (every town style reads "N City");
the item-model depth offset is applied after projection (tooltip model 86.9 px); hit text takes the
+0x44 flag (yellow for flagged damage to others) and the native rise rule; the land-name banner
alpha is read before the frame's decrease; creature bars skip creatures linked to state 0x6a, rise
by half the height minus the bob and move states 0x6a/0x6b two blocks along the camera axis; map
creature markers follow the native collection rules (skulls, town shop signs from scale 1.6,
max(r, 1) x 256 range, offline NPCs) farthest-first; the map marker tile is 4x4x4 with the bottom
layer filled; the map wheel multiplies by 0.9 or divides; the window-open sound check includes map
mode and options; side windows use integer halves; a bagless tab scrolls n = 1; Enter opens chat
whenever the key gate passes. The corridor third-line test is a named function with tests.
Not modelled: the code-made land-name and TIME/TEMP labels (visibility open); the 100 ms incoming
item queue (it would move the pick-up pitch draw); duplicate-name messages from the update routine;
the Direct3D device reset; the quick-item wheel. Engine adapter retained: taming progress is kept
outside a kind-7 buff record.
## Creature models, missions, rendering; mission rewards, pose, clouds/lights and shimmer rulings (2026-11-16)

Fresh source-only authors re-certified `alpha-creatures-models.md` (7699EB2D...1051), `alpha-missions.md`
(564B160C...1C21) and `alpha-rendering.md` (C0D58BEB...8716). Ruling authors settled the conflicts they
raised: `alpha-rulings-mission-rewards.md` (121E72DD...9D33) (every player is rewarded for a hostile
victim, in ascending creature id order; mission progress is gated only by the remote flag, after loot; the daily pass runs for
the region owning the player's position); `alpha-rulings-pose-49-50-mount.md` (2EE4F64E...CB89)
(pose 49 copies the off rest into the main rest; pose 50's 40q lean reaches the feet; only selector 0x3A reads the mount);
`alpha-rulings-rendering-clouds-lights.md` (AE0CBBE1...2173) (cloud, star, projection,
point-light selection and per-draw light sets; opaque creatures before water); `alpha-rulings-shimmer-phase.md`
(871DC71A...2F1A) (the shimmer uses cosine; the terrain numerics contract wins). Fresh implementers read
each contract whole.
Process note: a first clouds/lights ruling author was assigned `sky-and-clouds.md` (class E, contains engine status notes).
That ruling was withdrawn unpublished and the ruling was re-derived by a fresh author who did not open that file;
`sky-and-clouds.md` is withheld from source authors.
Engine changes: creature part draw order (glider/boat/mana cube first, arms, quivers, weapons, left hand, lantern, right hand),
the lantern draws inside the left-hand step, weapon glow, key-colour set, computed boxes in single precision, shimmer slots,
quadruped hand height, dodge and glide hand raise, shortest-arc angle steps, action 93 with an empty off hand gives pose 49,
pose 41 visibility and curls, pose 44/73 step 5, pose 50 foot lean, footsteps (ground and non-climbing only, at the creature);
the daily mission pass uses the owner region, route points draw y then x from the zone-centre column, zone-pass draw order,
every player rewarded on the non-hit death path, kind-1 tag distance from the monster area; frame clear 0xFF0070FF, one
projection for the frame, star precision, cloud gate at the player, cloud shade and rotation, opaque creatures drawn before
water and translucent bodies after it, effect tints, five-ghost motion trail, fog distance to chunk centres, lit-water
substitution, shaded type-3 film quad, particles without depth writes, 128-block item light limit, water lit by its own chunk
and post-water draws by the last water row.
Not modelled: riding and mount parts, the plaster field, the highlight colour source, far statics, a queued creature's own item
models (the whole translucent creature is deferred), ghost pose ageing at dt 0, dropping records for unloaded regions.
Engine adapters retained: fog window origin and start value, the water chunk visiting order, the free-camera cloud gate. The
owner's shared water shader, swimmable water, reeds, gamepad, price fixes and quests are unchanged.
## Pets and bombs; character screens (2026-11-16)

Fresh source-only authors re-certified `alpha-pets-bombs.md` (31CC4211...2AAC) and `alpha-character-screens.md`
(5673CB9C...DF21). The creature-behaviour "pet at combo 5" wording they flagged is already governed by
`alpha-rulings-knockback-taming-charge.md` (taming happens in the action-110 effect). Fresh implementers read each whole
(the pets-and-bombs implementer read revision F1E81BFB...0AF; its author then moved only the status line into the header,
giving the recorded revision).
Engine changes: the taming adapter is removed: each bite applies a real kind-7 buff (announced in section 12; the failure
re-issue is reachable; a rebuild empties the buff list through a shared creature reset); the fifth bite builds the pet without
re-running the appearance generator; bite signals carry the creature position, the pitch grouping is 0.9 + 0.2(d/32767) and the
target is the creature's own target field; new pet health uses controller 5; the kind-1 heal is not clamped; the T-key distance
is single precision; the dead-creature pass walks ascending signed id, explodes inside the walk and removes corpses after it;
the explosion gate is type 144 alone; the bomb filter skips the mount id (0); a returned pet's action time takes its death timer.
Front screens: list buttons are hidden while the left button is held, scrolling follows the stored-y bounds, pull, coast and
pick steps with friction every frame; buttons act on the left release (style arrows, palette and edit focus on the press); any
press withdraws the Delete confirmation and resets drag smoothing; Create clears the Name caption; panel figures turn and walk
only while hovered (per figure, asin ease back); the active character is drawn on its last world's panel; seeds are signed
32-bit; online worlds have names of length 7 or more; Delete World never changes the selection and Delete stays shown; names
are compared widened; the easing share is dt double steps rounded to single; Apply runs on every arrow, palette click, hover
change and Create, and Create saves the last Apply's record.
Not modelled: the native stage-10 placement of the dead-creature pass (the engine runs it at the end of the same step, one
step early); pet names; the online kind-7 conversion test; the bomb hit set; knock-back on bombs; per-frame figure record
fields; world-panel model placement (C-8); the "Please wait..." state; separate character and world name edits; hover
caption highlight. Engine choices for open points: delivery uses press-time visibility (C-9, keeps controller clicks working),
seed overflow wraps (C-6), wide units of 0x80 and above narrow to '?' (C-2). The controller drives the same press/release
path; price fixes, quests, water and reeds are unchanged.
## Materials, wildlife and routes; held-light factor ruling (2026-11-16)

A fresh source-only author re-certified `alpha-materials-wildlife-routes.md` (0784812E...A63F); a ruling
author settled its conflict with the terrain light-source lifecycle contract in `alpha-rulings-held-light-factor.md`
(8D26C2DF...A56F) (held-spirit lights in slot order 7, 6, 5, 2 with factors 0.3, 0.5, 0.1, 0.1;
ratio product first; all skipped only when the ratio is exactly zero; each block needs a positive averaged alpha; the
lifecycle contract's off-hand 0.3 is overruled). A fresh implementer read both whole.
Engine changes: the off-hand held light uses 0.5; the zero-ratio and alpha gates follow the ruling (negative and NaN ratios
pass); the averaged spirit colour carries alpha through a material colour routine (s·0.5 + 1, gold red copied); lying items of
type 0 draw no attached cubes and each cube takes the item's lit colour; the critical-hit flash walks the action weapon list;
wildlife camp distance in single precision; Lc from the creature's own position and the tier gate needs v and v squared
positive; the companion ring uses double trig of the widened angle, one rounding and truncation toward zero; WalkPath
advances its index modulo the point count, tests arrival by single-precision squared distance and clears a path whose goal
zone is not loaded.
Not modelled: equipped-item cubes still use the spirit colour times glow and only type-3 items; the WalkPath box fit stays on
the engine block adapter (MW-8); ring trig uses platform double functions; the native Lc lookup equivalence is unconfirmed;
held-light hand positions use the engine weapon-point fallback (MW-1). Engine policy: a non-finite held-light colour (zero
divisor) omits that source.
## Trails, poses and schedules; interface Plasma; audio; content extractor; six rulings (2026-11-16)

Fresh source-only authors re-certified `alpha-trails-poses-schedules.md` (302AE51F...06AD),
`alpha-ui-plasma.md` (F22B9FD5...73D2), `alpha-audio.md` (B95F12A6...37FB) and `alpha-extractor.md`
(23DDA955...6B23); the extractor contract gained a native content-store part (file identification, blob schema,
store roles, scrambling decode, payload formats, start-up checks) and lost its dump-tool advice. Ruling authors settled the
conflicts they raised: `alpha-rulings-pose-69-arc.md` (06CBBACD...4AF5) (an open 16-point arc of
168.75 degrees, not a circle); `alpha-rulings-manacubebar-sequence.md` (AF0E4BB4...E20E) (the
fourth cube plays `manacubes` once and all four icons stay lit while it runs); `alpha-rulings-audio-ui-sounds.md`
(062C2766...178A) (no voice is panned; 0x58 is the skill-lowered sound; buy/sell/landing sites);
`alpha-rulings-zone-fields.md` (B7313316...76A2) (ambience walks chunk records gated at 64 blocks by squared
distance; zone vectors +0x04 sprites, +0x0c statics, +0x18 spawns, +0x24 location objects);
`alpha-rulings-spawn-rounds-ruin-routes.md` (E0B1CBF5...A080) (SpawnLocation runs one round on
an entry change; ruin group creatures walk to a point of a drawn house; ring route points are exact; open-plot ruin creatures
get the activation default); `alpha-rulings-channel2-explosion-window.md` (DA620D4A...77D3)
(the particles' duration is P1; the explosion uses the late crossing; action 104 never plays 5/16). Fresh implementers read
each contract whole.
Record correction: the pushed commit 32d121b carries the Recert24 subject line by mistake; its content is the materials,
wildlife and routes batch recorded above.
Engine changes: each creature draw is its own pose call (ghosts and translucent colour at dt 0), opaque trails show the same
frame's ageing, the chain is inert at dt 0, unarmed hands drift point 0, the pose-69 arc follows the ruling, trail alpha in
single precision; mission creatures run Combat and WalkPath only; house test and path start truncate; SpawnLocation's single
entry round and 11-test ground fit; ruin group two-point routes in native draw order; fixed-point exact route points.
Interface nodes use the full pivoted 4x4 matrix; frame 0 is the current value; sequences start, clamp, wrap and stop as
specified and advance once per update; `show`/`shine` play once; mana cube pickups show both messages and the sound, and
the bar plays `manacubes` on every fourth cube. Sound volume, 100-block falloff and the d squared boundary follow the
contract; random pitches in single precision; swing crossings, multi-hit intervals, channel2 ticks, staff release and
explosion windows follow the ruling; ambience walks a chunk grid with the 64-block squared gate and native draw order; buy,
sell and adapt sounds play at the player and dropping plays nothing. The setup tool and content loader check native headers,
the blob schema, store roles and counts and reference sizes and hashes, never create files in the installation, reject empty
stores and require db.dat; .cub loading follows the payload rules with marker points; zones are kept in a squared-distance
disc below 16 and cleaned up after 1000 ms.
Not modelled: the trail view test, outer pose callers and trail blend state; the combo-points `hit` sequence, edge fringes and
blur layers; music; a few refusal and pickup sounds still play at the listener; the db.dat content rule, dict_de selection,
the .plx internals and spawn save words. Engine adapters: silent audio when the backend fails (native exits); hidden interface
nodes advance (root update flag inferred); unloaded zones keep the engine's own block type (native reads grey type 0x01 and
would lift creatures there); the dedicated server keeps its own zone unload distance.

## World-step death pass (2026-11-16)

The clean team read the complete held `alpha-pets-bombs.md`
(`31cc42112587c8aa9fdb65461d3b3b5968b9ffe211165bd0418ddcf2cf4d2aac`) and
`alpha-server-simulation.md` (`e11970558b2e7500f0d3f6adfc334be3d7ad3ca0d828539d2dbb9e0662587389`).
This resolves the stage placement gap recorded in the earlier pets and bombs entry.
Spawn schedules and countdowns advance before despawn. The signed-ID death pass
follows pet maintenance and precedes both activation and the living turns. An
incoming fatal hit can enter the current pass. A bomb that runs out of fuse time
during its turn enters the following pass. The pass reads the published action
clock for explosion admission, advances it with that step's integer duration and
removes eligible corpses after the walk. Returning pets copy the owner's fixed
position and clear their threat state and the owner's hit counters before living turns.

The engine suppresses duplicate presentation, loot and credit on repeated
zero-duration calls. Its non-hit death notifications remain an adapter for deaths
outside the synchronous fatal-hit path. Client and server still batch explosion
voxel edits after the creature walk. Bomb item requests still enter the outer
client and server paths ahead of the requesting player's turn.

## Terrain point lights and shader numerics (2026-11-17)

A fresh source-only author re-certified `alpha-terrain-chunk-point-lights.md`
(`61f806721de76968b7f0160d330032241a9d9633986cb07a601fa4a01c2b4996`) and
`alpha-terrain-shader-color-numerics.md`
(`dfbb9588d86f209b28550990a62b9a457af58223fdc1c26cc7081cd1ad0bc217`). The clean team
verified the hashes, read both held contracts in full and reconciled the engine.

Chunk selection divides signed fixed endpoints into columns, keeps the signed low
32 bits, then divides into chunks. Resident positions apply only horizontal
translation and keep the original fixed coordinates through conversion. Each light
supplies a shared position and colour across its receiving chunks. Selection keeps
16 resident slots and uploads the first 10. The cached-source radius phase and
sector-object culling already use the held operation order and positions.

The ordinary world shader now uses a single clip-W reciprocal for its two fog
coordinates and a fused vertical translation. The CPU palette keeps the observed
zero product and addition for the warm/wet blue term. The existing paths use the
ten-light accumulation and the specified order of the cubic, haze and sky blends.
Release checks still cover broader equal-key sort witnesses, untraced source and
device state, Vulkan transform and interpolation differences, and native raster
and display output. Explicit lighting extensions continue through their own settings.

## Living phases, frame calls and shared randomness (2026-11-17)

A fresh source-only author re-certified `alpha-creature-update-phases.md`
(`ee9014f2b73a0414e81551ae2f4c164564485e7cf558d89bde1b97f1cc87ed3a`),
`alpha-client-frame-world-step-order.md`
(`06e3b9cd4c29085a0daf0b6e2a9104a9e4c78b0f88da48965d4f3360a791dc65`) and
`alpha-world-random-stream-boundaries.md`
(`8669fc47147ad7f479113574b01eec67ce77d2a9179434986065719e5fa23a4a`). The clean team
verified the hashes and read the held contracts in full.

The engine stores cooldown and behavior accumulators as signed integer milliseconds.
The common resource and charge gates compare signed action clocks directly, and the
charge path reads attack speed only when its gain is admitted. The grouped charge
already confines buff 10's assignment to anticipation or later. Combo charge
follows the held cap and repeat-read order. The frame splitter already uses one
output aggregate, issues the final signed piece including zero and negative calls,
and reaches the normal primary-input pass after all admitted pieces.

Descriptor activation uses the shared seed and generator before the signed actor
walk. The re-certified inventory covers 47 direct world draw sites, including
the 33 newly traced sites in action dispatch. Action-specific call equivalence,
transitive draw counts, the original clock-scaling and admission producers, the
action-105 input mapping, and charge placement relative to late physics still need
verification and reconciliation. The GLFW elapsed adapter and synthesized client
input remain engine paths.

## Bomb terrain edits during the death pass (2026-11-17)

The clean team used the held `alpha-pets-bombs.md` §4, `alpha-block-edits.md` §8
and `alpha-strike-cell-helpers.md` §8. The offline client and dedicated server
apply a bomb's cell edits from its raw fixed-point centre inside the death pass.
The primary burst precedes the ordered broken-cell bursts. The edited terrain
is visible during victim application and to later blasts and actors. The sound
draw follows the edit and is spent only when the blast finds removable cells.

The engine applies each blast's edits by zone, retains that step's active spawn
descriptors across terrain snapshot replacement, and uses its existing relight
and remesh paths. The earlier deferred bomb-terrain gap is closed. Mixed packet-6
request timing, deferred melee craters, and native relight and visual acceptance
remain open.

## Client action input and binding 99 (2026-11-17)

A fresh source-only author re-certified `alpha-client-action-input-phase.md`
(`65e68e7da488757669daf2dc7094d135e2a9dcb6d63a116a86092b61cbd453d2`) and
`alpha-client-action-binding-producer.md`
(`9ab940a949cd76357170e0fdbdedfc8a16cf7bf217556ab20c89830deed6bdb5`). A separate
source-only ruling author resolved the setter conflict in `alpha-rulings-action-99-lens.md`
(`327d398623c17cd33d0dd38da32f74e97ceddcc908f8b8d7ca626df0c4e89faf`). The clean
team verified all three hashes and read the held texts in full.

The engine already captures the concrete bindings before world pieces, visits
the configured slots in order, and reselects the ordinary primary from current
state. An admitted action 34 now stores the caster's full ID immediately. The
held release cases publish the replacement action before querying its clock.
Binding 99 sets the actor and movement-body lens bit before the cooldown. The
new ruling supersedes the earlier missing-setter finding. Resident intent clears
the bit before ordinary meter growth, which uses skill slot 8. Buff 3 forces
the meter to one before mana reads it.

The engine retains an approximation for action 79's separate meter branch.
Action 105's timing is present. Its physical input producer and position effect,
pet-food transfers at action entry, full native device/UI admission, crosshair
publication timing and hardware controller acceptance remain open.

## Dedicated packet-6 sender turns (2026-11-17)

An isolated source author finalized [the sender-turn contract](specs/alpha-packet6-sender-turn.md)
(`3918a2529fcffa2972ade6b1245e89a3e81da2df20461b1768b481c3d33c05e5`).
A fresh source-only author resolved the bomb constructor conflict in
[the bomb-default ruling](specs/alpha-rulings-packet6-bomb-default.md)
(`0ac4d4ecf8cad43c9d64e96048ec433635d57602e355033e1c9ede28f507570e`).
The clean team verified both hashes and read both held documents in full.
The ruling establishes 500 live health, the full signed item level and five
appearance draws. It supersedes the sender-turn contract's initial health value.

The dedicated world consumes each live player's whole mixed FIFO at its signed
actor key, before the health clamp and resident buff countdown. NPC removal
rebinds surviving actor entries in place. A freshly placed bomb retains the
sender's raw position and first receives a fuse turn in a later sweep under
ordinary ID allocation. Inn rest changes the current clock, day and surviving
actors before subsequent requests. The always-on quest rewards use an
extension-owned random stream initialized from the server seed.

World-item timers and static state advance after incoming hits and buffs and
before spawn timers and the actor walk. Pickup commits the gift and the pitch
draw before erasing the indexed item. Chest units use Y, X and discarded angle
draws followed by the shared drop helper's timer and conditional angle. Drops
use the live actor centre and emit their attempt sound. Static requests capture
ordered section-six snapshots. The server retains its supplemental changed-zone
and requested-zone snapshots for its existing replication path.

Active zones retain the engine's zone-key order. Trap target snapshots, remote
player movement ownership and blueprint terrain finalization retain engine
adapters. Inherited native-client playback and the new offline sender-turn path
remain separate reconciliation and acceptance work.

## Client packet-6 append and result phases (2026-11-17)

An isolated source author finalized [the client queue contract](specs/alpha-packet6-client-queue.md)
(`8c9e544b59f227f40fae27d5b8add45155362b152e46ce18d0dade8d0776ee2f`).
The clean team verified the hash and read the entire held contract. The
bomb-default ruling and sender-turn contract govern the five initializer
draws and shared authority handlers. The fixed species branch itself adds
zero draws.

The inventory mouse path delivers its consumable uses before the world
pieces. Supported frame producers append after the pieces in Q, drop,
talk, generic static, pickup and pet order. Offline requests share one
mixed FIFO and consume at the controlled actor's signed key. A resident's
furniture behavior leaves its static request for that resident's next piece
on both authority hosts. Ordinary consumable use continues to start
actions 80 and 81. Ordinary local pickup stores the item before the pitch
draw and erasure, then grant handling and world sound playback run once
after the pieces. Newly dropped timers first count down in a subsequent piece. Changed zone
lists are saved after the piece that changed them.

The connected client samples permitted pending static actions at the
controlled actor's turn and retains one-request-per-iteration transport.
The engine publishes NPC home triples and the local mana-cube counter.
Online seats follow an authoritative occupancy update. The engine's
initial quick-use selection uses the first eligible item while the full
selection wheel remains open work. The queued local-only runestone branch,
stock blueprint activation, blueprint placement in offline play, full
native input admission and original-client runtime acceptance remain open.

## Runestone map release and queued links (2026-11-17)

An isolated source author finalized [the placement and runestone contract](specs/alpha-packet6-placement-producers.md)
(`4bd8666a716c82e9fe89a1506ff6284e6e80f098f0447e76c2f18fc2975ba99d`).
The clean team verified its hash and read the complete held text. The
stock runestone action opens the map without an interaction record.
The second matching release now writes the main-thread player's raw
fixed position at the selected zone centre with Z zero, then clears
the map selection. Motion is preserved. This follows the held signed
32-bit block calculation. The existing map traversal uses X then Y
ordering and the specified inclusive lower and exclusive upper hit edges.

Queued kind 3 uses the separate local runestone branch from the held
sender-turn contract. It uses the zone of the first closest static in
spatial bucket order, with the origin-zone fallback, and takes the link
from the requested stone. It emits sound 47 at the old position before
copying the linked position with two blocks of lift. It consumes no
random draws. Dedicated and resident callers omit the local context.

The placement contract establishes the mode check, the live record
selection and the right-button rotation path. Stock activation and
selection producers, offline blueprint placement, full map projection
and original-client runtime acceptance remain open.

## Connected melee actions 2–7 (2026-11-17)

The implementer read and verified the two complete held documents.

- [Connected melee actions 2–7](specs/alpha-connected-melee-actions-2-through-7.md), 293 physical lines, `5a9bea098c93e721ccc4a0ce2dc656acc9965b848f6230f8588f69d05d702b3c`.
- [Connected melee scalar ruling](specs/alpha-rulings-connected-melee-scalar-selectors.md), 128 physical lines, `249b1be7ec07d0fafb97dbd7c4f15d34c7f3427ad63a57ab37070a2cef77c02e`.

A fresh isolated author adjudicated the conflicting early cost, amount
and impulse entries in sections 3–4. The ruling governs those selectors.
The original tables give actions 3 and 4 a fixed 0.1 cost, action 5 an
amount coefficient of one-half, and impulse six over live speed for
actions 1 and 2, three over live speed for 3, 4, 6 and 7, and constant
four for action 5. Roles three and five receive zero impulse. The
shared scalar functions supply those same values. Divided impulse reads
weapon speed once. The constant and zero branches skip that read.

Received actions 2–7 now run the live signed candidate walk. Action 5
uses its saved thirds of the active duration. A projected crossing to a
new pulse clears struck and deferred before feedback, preserving the
hit latch and the combo used for the original clock decision. The startup
type-11 replacement, pending resource and latched critical shape the
same candidate's draw count. The scoped replica helper emits target
results and resident effects without applying target damage. The
remaining action families, independent readers for the stock hit totals
and original-client combat acceptance remain open.

## Visibility geometry re-certification (2026-11-17)

A fresh isolated author independently re-certified [the visibility ray contract](specs/alpha-visibility-ray-geometry.md).
The implementer read all 84 physical lines and verified SHA-256
`b9ad1b0a9033068681ca982873f6c0b102bdf2ae2293b8e2c7d54f8c08a4ea10`.
The re-certification checks the scalar conversions, initialized missing
cell, held spatial grid, and four side faces. The shared finite-input
ray kernel already uses the specified step and face order. Current world
storage supplies cell and object callbacks. Native unloaded-column and
loader equivalence, and the original server's ray path, remain open.

## Offline queued blueprint placement (2026-11-17)

The isolated author finalized [the stock placement contract](specs/alpha-stock-blueprint-placement.md).
The implementer read all 252 physical lines and verified SHA-256
`a49cd4a2d877da5315166f0f9a3478543608386a1c46192d14935731871d3f24`.
The original local writer audit covers the mode and signed selection
fields, the conditional R producer and the offline world's kind-7 arm.
The stock route into enabled mode and the local model-selection writer
remain untraced.

The offline actor turn resolves the cached owned model with the live
selection, rotation, aim and position. Placement uses the shared stencil
arithmetic with signed fixed-point conversion and truncation after adding
the anchor. It reads earlier writes in the same stencil, writes protected
interior air, retains the original keep bit, and emits only the admitted
colored records. Unsupported ground-resting plants and sprites are removed across
the loaded unrotated rectangle. The anchor zone retains the model and
position entry. Each colored record enters the existing edit history for
its own loaded zone. The terrain and light renderer use the published
copies. The verified path runs with a local actor. Native preview activation,
lighting without a local actor, placed-object persistence and original-client
acceptance remain open.

## Quick-item wheel and ordered confirmation (2026-11-17)

An isolated source author finalized [the quick-wheel contract](specs/alpha-packet6-quick-wheel.md)
(`9aab793438e4a2ee8bdc84c49d3bac95bf9938b77e51c0b709e8599dff4f1fbf`).
The clean team verified the hash and read all 199 lines. Keyboard messages
toggle the wheel. The early scan admits Q from the retained ordinal and
only advances its edge history on an eligible pass. The late wheel handles
held movement closure, neutral steering admission and the two 200 ms
repeat clocks. An index below zero wraps against the early scan before
the late selection, including when Q removed a candidate in the same frame.

Q and R/E use the same inventory and consumable paths. The late R/E
confirmation follows drop, talk, static, pickup and pet producers. The
bomb enqueue precedes its stack decrement. The wheel emits fixed interface
sounds with no game random draws. The open wheel suppresses the ordinary
primary start for the frame, including a frame that confirms and closes it.

The existing selector template and item-model renderer draw the late
selection and ring. The ring uses the raw ordinal with the held mixed
precision easing and projection arithmetic. The engine retains the last
finite angle when the render list is empty. Keyboard navigation follows
the traced focused route. The existing controller quick and interact
actions reach the corresponding use sites. Exact modal widget equivalence,
native pixels and physical controller acceptance remain open.

## Dedicated blueprint placement writes (2026-11-17)

Using held `alpha-block-edits.md` §6.3
(SHA-256 `d08f1b86f5d5259a4005b5863158e96f88f648db17a123ef2fc34de2c965de7b`), the server applies the prepared
blueprint writes in model traversal order within each affected zone. A
single copy and relight handles the solids and protected interior air.
The section-one and saved edit lists include the reported solid cells
in the same order. Interior air is available to later world reads
without a section-one record. The placed-object entry follows the
terrain writes.

The engine retains its affected-zone batching, lighting and floating
sprite cleanup. Matching the original footprint passes, the offline
placement path and the stock mode and selection producers remain open.

## Timed consumable actions (2026-11-17)

The isolated source author finalized `alpha-timed-consumables.md`. The
implementer read the whole contract and verified SHA-256
`584e4b408304e36c4eae764c8f982f2b3379260f20b18b0c3e5e90db742b74d3`.
Both images use signed 200 ms effect buckets and 400 ms sound buckets.
This supersedes the earlier cadence in `alpha-pets-bombs.md` §6.

The common actor update emits a signed-source heal record before adding
and capping health. The secondary accumulator uses the rounded sum for
its recovery sound test. Recovery sound 47 precedes recurring sound 44.
Client presentation consumes the local heal record without reapplying it
or queuing packet 7. Dedicated NPC output uses the existing hit and sound
sections. The movement and pose code share the same item duration. Quick
use follows the signed level check while inventory presses retain their
transformed level check. The immediate kind-1 handler remains a separate
request path.

NPC potion admission uses two draws in X/Y order. The timed actor
effects use zero draws. Exact item attachment transforms, native frame
presentation timing and original multiplayer acceptance remain open.

## Melee terrain inside the strike (2026-11-17)

The implementer reread held `alpha-strike-call-sites.md` K3 and
`alpha-strike-cell-helpers.md` §§2–8. Their SHA-256 hashes are
`8849c4207df74650504619231b7d4aba87515606262945cd446796bd512643e7`
and `cf2cd012585a9bdb004de964d2087166d43b782fbcd45b6bac3bff4ca5e74cec`.

Offline and dedicated melee strikes apply their terrain edits before target
resolution. The shared removal service receives the raw centre and the
strike's truncating edit coordinates. When a cell qualifies, the strike
appends its bursts and draws sound 2 from the shared stream before walking
targets. Later overlapping strikes see the edited terrain at their own turns.
At negative heights, a strike can test a cell and fill the one above it with
water. The ordered probe takes that water fill into account within the same
column. The native horizontal column bounds apply to both strike and blast.

The engine applies a single batch to each touched zone. Exact native relight
and plant pass footprints remain open.

## Placed-object snapshots and zone requests (2026-11-17)

The isolated source author finalized [the blueprint delivery contract](specs/alpha-blueprint-zone-delivery.md).
The implementer read all 244 lines and verified SHA-256
`f5bc972a487e774996eeb23caf01f067db531cb9603b6ffbcc61894994a2f0ee`.
Dedicated placements publish the whole anchor-zone list to every peer.
The connected frame applies item, block-edit, static and placed-object
records after the world pieces. A section-8 entry replaces the held
zone's list, including when empty, and updates no terrain. An absent zone
discards the entry. Valid sections 6, 7 and 8 each satisfy the held zone's
request latch. Unloading a zone discards its latch and placed objects.

The ordinary connected client considers only the controlled actor's current
zone. The raw signed coordinate, low-word narrowing and final division
are shared with the dedicated server. The sender takes copies of the live
lists after its gameplay batch. The signed wrapping clock admits a request
turn above 1,000 ms, advances even for an empty turn and emits packet 11
before packet 12. The server appends each requested snapshot in receive
order after the corresponding world outputs. Duplicate requests receive
complete replies. Packet 11 supplies an empty item list for a resident
zone with no items and leaves section 8 to the world output.

The engine uses its frame lock and steady clock for transport. The current
region request persists until its 64 cells arrive and refreshes while the
map is open. Exact native packet-12 production remains open. The current
client connects before loading zones and has no hosted mode or in-place
rebind. Stock blueprint activation and selection, offline placement,
native lighting footprints and original-client timing remain open.

## Pet riding in actor order (2026-11-17)

The isolated source author finalized [the riding actor contract](specs/alpha-riding-actor-step.md).
The implementer read all 202 lines and verified SHA-256
`758a29cbd9bfde54bb32d8bf2a939e9dcda9e7e3c672d8eed3e9c149c84909c0`.
The existing Ride prompt starts action 106 on a rising R sample.
At the controlled actor's turn, the current linked actor's type and ordered,
nonnegative health determine admission to the riding speed multiplier.
After entering the speed branch, the piece's drain, meter clamp and climb
test still run if validation clears the action. Cancellation at this stage
retains the action clock. The engine resolves a link once under its actor
protection for the native lookups with no intervening mutations.

Resident role-five actors refill the resolved owner's meter during physics.
One refill can follow either a water mark or a visited type-3 ground cell.
The dry probe completes the Y span and stops at its first solid X column.
Each increment uses the piece's binary32 seconds. The actor sweep preserves
the refill's order relative to the owner's drain. The shared resident
implementation covers the offline and dedicated worlds.

The two resident attachment passes use live owner state at the displayed
position boundary and again after the actor loop. They copy record position,
follow position, body orientation and landing bob, and clear velocity and
intent. The latter pass rechecks the owner action and reaches nonliving
pets still present in the actor map. Display-rotation smoothing and the
walk phase and amplitude caches remain rendering differences. Connected
remote refill and attachment, the dormant mount-ID path and native
interactive acceptance remain open.

## Connected actor motion and riding (2026-11-17)

Read the held [remote interpolation contract](specs/alpha-remote-creature-interpolation.md)
and [riding actor contract](specs/alpha-riding-actor-step.md) in full. Verified
SHA-256 `24f15463d3d297d05b196b0e23538c0ed0320c1a0dfe29bb7cb9a574cd369551`
and `758a29cbd9bfde54bb32d8bf2a939e9dcda9e7e3c672d8eed3e9c149c84909c0`.

The connected world piece advances the controlled and received actors in one
signed-key pass. Each living received actor ticks its resident buffs and runs
terrain movement before its drawn-pose pass. A role-five actor refills the live
owner's meter during its terrain probe. The first attachment replaces normal
copy or interpolation, and the late pass after world objects rechecks the live
owner, including for a dead received pet. It copies the live and drawn positions
and rotations, copies the landing bob, and clears velocity and intent. A zero
reverse key can resolve a supplied actor. Facing reads the previous drawn aim
and the aim refresh follows the pose write.

The connected client reuses one smoothing weight per piece and the existing
receipt snap decisions. The render, aim and interaction paths consume the new
positions. The connected pet HUD uses the linked role-five actor that belongs
to the controlled player. The controlled display pose uses its direct body
pose, and remote facing retains the received clock for its timing.

Walk phase and amplitude still live in the renderer. The precise remote
action clock and all movement inputs, frame publication of multiple receipt
poses, the dormant mount-ID path and original-client acceptance remain open.

## Creature receipt publication (2026-11-17)

The isolated source author finalized [the creature receipt contract](specs/alpha-creature-receipt-order.md).
The implementer read all 145 physical lines and verified SHA-256
`894b3ea606c54f188154b4ee05cce2ff2a281c06c3320fc5fde1d1c1fba0a03b`.

Packet 1 takes its temporary record snapshot at the count boundary, folds
each repeated key's fields into the same record in wire order, then prunes
unlisted creatures and commits once per distinct key in signed order.
Its snap decision uses the final merged record and the held pre-assignment
health and snap field. Packet 2 uses only the persistent set of successful
packet-0 keys, which survives packet 1. The engine maps received records by
their own identifier and retains its known mask-width validation.

Each snap publishes the position and rotation of that receipt. The frame
drains these in order and applies the newest live record once per actor with
pending receipts. If the same key returns as a new creature, the body,
fighter and cached presentation state start a new lifetime. The engine
seeds its drawn-aim adapter from the creation receipt. The native
write-only saved pose and container semantics, exact live-field baselines
between frame and receiver snapshots, and original-client timing remain
open.

## Live movement baselines for creature receipt (2026-11-17)

The held [remote interpolation contract](specs/alpha-remote-creature-interpolation.md)
§§1.3–1.6 and §3.2 and [creature receipt order](specs/alpha-creature-receipt-order.md)
§§1 and 3 establish that the receiver starts from the live record under its
initial lock. Packet 1 takes the same kind of baseline before reading its
field stream. A field omitted by its packet keeps the baseline value.

Before releasing the frame gate, the connected client publishes the
locally advanced position, orientation, velocity, intent, response and
head pitch to the receiver's live record. A partial snap can therefore
take its pose from that advanced baseline. Each receive retains its
separate temporary copy through decoding. The publication writes the
six motion fields and leaves the other fields to their current producers.
Native timing and complete non-motion live fields remain open.

## Connected common timers (2026-11-17)

Read the held [living-creature update phases](specs/alpha-creature-update-phases.md)
and [remote interpolation](specs/alpha-remote-creature-interpolation.md) in full.
Their verified SHA-256 values are
`ee9014f2b73a0414e81551ae2f4c164564485e7cf558d89bde1b97f1cc87ed3a`
and `24f15463d3d297d05b196b0e23538c0ed0320c1a0dfe29bb7cb9a574cd369551`.

The frame applies a core snapshot to a resident actor when a receipt is
pending. The core-field copy preserves the destination's item gaps and
actor-resident state. Each received creature runs the common preliminary
timers and clock logic at its signed living turn. A surviving type-1 buff
can clear stun before the signed subtraction. Only a positive result clears
the action and clock. Role 6 skips general clock progression, and a negative
clock waits while its action stays active. A zero clock initializes its
cooldown after that turn's cooldown and buff decay. The received turn skips
combo timeout and world-authoritative type-4 hits.

Drawn aim reads the advanced clock and the renderer uses the live actor's
velocity and health. The next packet baseline includes the action, its clock,
stun, action lock, both movement timers, recovery and capped health as well
as the previously published motion fields. Per-action effects, resource and
movement inputs, the native movement-driven facing flag and native timing
remain open.

## Connected resources and movement fields (2026-11-17)

The isolated author finalized [the connected creature turn](specs/alpha-connected-actor-turn.md).
The implementer read all 462 physical lines and verified SHA-256
`fba882762680c0eebbd31fd768d561728f83d27337e5c841579c4959d3ce8e5c`.
The held [physics contract](specs/alpha-physics-camera.md) supplies field 6,
the signed roll, and contact and motion inputs. Section 6 of the new contract
confirms that drawn aim reads the late live action clock.

Received bodies now use the action clock and phase lengths as signed integers,
the received roll timer and modes, equipment checks, skills and prior contact.
Action 54's vertical override precedes mana and blocking. Intent then clears
the lens bit and can end sitting without clearing the clock. The ordinary lens
meter updates before charge. The roll gate uses the still-current timer and
applies the class/specialization exemptions. Charge is capped to the resulting
mana. Late stun clears charge and the glider and climb toggles. Contact, modes,
roll, mana, blocking and charge now join the receiver's movement baseline.

Action 79 still uses the existing meter approximation while its ordered native
lighting inputs remain unmodeled. Connected pre-clock action preparation,
target outcomes, exact movement-driven facing, the sound and visual event
sites and their draws remain separate work. Movement shares the existing body
and collision adapters. Original-client interactive acceptance remains open.

## Connected walk and movement cues (2026-11-17)

The isolated author finalized [the connected walk contract](specs/alpha-connected-walk-phases.md).
The implementer read all 316 physical lines and verified SHA-256
`04a38ca199a5a88799f30f4a22b1e9f804300c8466fe15ffe83d6e4fa298b098`.
Sections 2–6 close the action-driven facing flag, ordinary walk phase and
amplitude, the late equipment and relaxation rules, the support probe and
the roll snapshot. The [connected creature turn](specs/alpha-connected-actor-turn.md)
supplies the blocked-landing and splash sites and their local-pointer gates.

The movement step produces the phase and amplitude before the connected
renderer reads them. It saves the blocked landing position and uses the old
roll for footfalls and facing. The movement output uses the shared random
stream in signed actor order. A landing consumes one draw, a splash appends
its visual before one draw, and a ground footfall consumes one draw. The
roll cue follows facing and consumes no draw. Carried pets copy the current
owner's walk phase and amplitude in both existing attachment passes.

The separate ordinary link at creature offset `+0x11C0`, broader action
sounds, the resident renderer's remaining timing, and native sound playback
acceptance remain open. A received box with a missing X dimension uses a
finite engine divisor for its pose.

## Connected common preparation and action audio (2026-11-17)

The isolated author finalized and read back the following contracts. The
implementer read each complete file and verified its SHA-256.

- [Common action processing](specs/alpha-common-action-processing-state.md), 140 physical lines, `c9b766cecf21490200846d4b2526039f0c20df14214160b884d96aa3e06029da`.
- [Connected action audio and preparation](specs/alpha-connected-action-audio.md), 314 physical lines, `67117f9a23e81064b3b3487b17f4ec7d4ca100436eff97ede6a58029dc20f502`.
- [Sound 36 cadence ruling](specs/alpha-rulings-channel36-cadence.md), 65 physical lines, `cb3c5457f146bd2ed872558e3d9a26bfaf81d68b80a852141e51e7b0f4b8f7ad`.

The cadence ruling settles the earlier conflicting divisor at 200 ms for
both client modes. The connected actor appends its opening, shared swing,
repeat and midpoint sounds at its own signed turn. The shared audio path
uses signed clocks and truncating bucket arithmetic. Staff and explosion
records preserve their fixed position and are appended after the release
sound and before later swing draws. The engine initializes unassigned FIRE
colour bytes to zero and uses the existing consumer palette.

The received actor uses the existing common preparation for ordinary and
pending spending, action-31 and 33 crossing debits, and the action-5/17/20
type-11 replacement and class-four roll. The valid action-31 saved interval
can also produce the type-9 replacement before ordinary buff expiration.
The frame applies the usual local-source filter when promoting these buff
records to transport. Full candidate traversal, hit outcomes, stored
candidate cleanup and its ordering against all cues, and original-client
playback remain open.

## Connected action-79 light inputs (2026-11-17)

The isolated author finalized [the sneaking light contract](specs/alpha-sneak-light-inputs.md).
The implementer read all 282 physical lines and verified SHA-256
`e104f8c8ccabd1f0dd5e5eab329341a15fef7a2400039d0ec48fd30a74caa4ae`.
The connected piece fixes zone membership from the three-by-three
neighborhoods of controller-zero bodies. It keeps the admitted zones in
resident address order. Zone lights visit flagged model placements, current
type-50 objects and category-18 items in their respective collection orders.
A complete section-six snapshot supplies both its current type and its
fixed position for this sample. Section-seven lists supply the current
candles. Living and dead lantern-flag carriers contribute in live signed
actor order after the zone cap, including the subject when flagged.

The connected controlled and received actors use the current light and
full live velocity for action 79. The result passes through the shared
type-3 meter override and final mana gain. The implementation uses its own
resident zone addresses and the existing world cell and material adapters.
Offline and dedicated action-79 meter inputs, broader section-six placement
consumers, and matched native lighting and resource comparisons remain open.

## Connected action-1 melee production (2026-11-17)

The isolated author finalized and read back each contract. The implementer
read both complete files and verified their SHA-256 values.

- [Connected melee candidates](specs/alpha-connected-melee-candidates.md), 451 physical lines, `bb60a4bc5b05292744f36d4b814883c85c0320779858391778ecd3d895f8e76a`.
- [Connected combo and idle cleanup](specs/alpha-connected-combo-cleanup.md), 87 physical lines, `d0fc5bcd2066343ba0ce619f48587a8e3760d95bc22e52a04e031899e91fbf13`.

The received action-1 branch evaluates its saved clock bounds after common
preparation and before general clock progression. The candidate walk uses
resident bodies, signed map order, fixed reach, world visibility, hostility
and live roll. The local creature is rejected after the caller rolls and
struck insertion. The shared resolver provides defensive and ordinary records,
equipment effects and audio. Live guard, flash and source changes occur in the
producer. The branch leaves target damage, record stun and knockback to the
authority path. On success it updates eligible combo and lens state without
the local-owner mana draws. Combo and its timer join the packet baseline.

Anticipation and the action-1 upper bound can send deferred feedback before
the hit window. The post-clock idle cleanup runs for every admitted controller
kind and clears both sets while keeping the hit latch. Counter benefit uses
the live target, including the controlled creature. The cooldown-tree walk
between combo writes has no observable effect and is omitted. The engine uses
its existing stat, item presentation and world visibility adapters. The
visibility adapter reads received static type, position, rotation, dimensions
and state when present. Geometry outside the engine's fixed conversion range
is skipped. Other action-specific candidates, output consumers and native
client acceptance remain open.

## Connected hit receipt and presentation (2026-11-17)

The isolated author finalized [the hit consumption contract](specs/alpha-connected-hit-consumption.md).
The implementer read all 303 physical lines and verified SHA-256
`d70901d54aebc7cfe7e41a04a4d577e01bb6d0a5a78deaaf1761a636449c16f1`.
The frame collects controlled melee and projectile hits alongside received
actors' generated hit records. After the world pieces, the local-target pass
applies all relevant receipts in order, including self-source echoes. A second
pass appends foreign-source receipts after produced records. It preserves the
engine's existing pet-capture handling and permits the SDK's local damage hook
to update the amount used for state and presentation.

Floating text requires a resolved target and a local endpoint. Position uses
the raw record and live target height, including the fixed local Z adjustment.
Lights use the resolved source's current striking weapons for the nonzero
resource byte and append the independent auxiliary light when its byte is set.
The sender receives only complete-ID local-source hits and buff events from
the frame, with serial order within each collection. Connected incoming buffs
apply after hit state processing and before presentation. Native hit totals and
the transient aim pointer are not modeled here. Original-client combat and
light comparisons, the embedded host and the capture-signal details stay open.

## Ordinary creature link (2026-11-17)

The isolated author finalized [the ordinary link contract](specs/alpha-ordinary-creature-link.md).
The implementer read all 147 physical lines and verified SHA-256
`08d2f8f584f351dc01662ae4b66acfb1598c7f1a126d4f7165cb30b0f394b80b`.
The client movement tail resolves the resident ordinary link in the live
creature map, including numeric zero. A missing actor preserves the key. A
resolved nonpositive target clears it even for a role-five subject. A living
or unordered target supplies its fixed position with the documented vertical
offset, shifted angles and half walk amplitude. The copy clears velocity and
intent, skips ordinary walk, and keeps the later relaxation and riding passes.

Connected and offline client bodies use the same movement operation. The
controlled player and offline NPCs render the copied walk state when attached.
Construction and actor reset default the identifier to zero, and the core
packet copy preserves it. The dedicated server keeps its existing movement
route. Stock nonzero producers, fresh offline production of actor key zero
and original-client runtime acceptance remain open.

## Connected melee actions 9–21 (2026-11-17)

The isolated author finalized [the connected actions 9–21 contract](specs/alpha-connected-melee-actions-9-through-21.md).
The implementer read all 425 physical lines and verified SHA-256
`43206c115392d0d53c59cff91d00ba8032edc68bfbad9253f9cd7cb196f84900`.
The received living turn now admits these actions to the signed candidate
walk and keeps the saved early-clock bounds through combo retiming. Action 11
resets its candidate sets before the pulse pitch draw and deferred feedback.
It keeps its prior window for the rest of the turn and spends the full pending
amount before halving the recorded resource. The critical probability is
sampled before its draw. A zero amount still follows the applicable draws.

Class-four specialization-one actions 17 and 20 write the startup roll before
the early sounds and deferred feedback. The common hit resolver appends the
action-17 timed buff after damage multipliers and publishes the returned buff
record before the next actor. The connected actor's own buff pass ages the
resident effect. Finite nonlocal living actors with positive stat divisors are
covered. Original-client combat, the zero-period case, other action families
and the wider stat and presentation acceptance gates remain open.

## Connected staff actions 30–33 (2026-11-17)

The isolated author finalized [the connected staff contract](specs/alpha-connected-melee-actions-30-through-33.md).
The implementer read all 393 physical lines and verified SHA-256
`49c71dcdbf245c9c15383fe18eaaa34ba14b65b03d958fc441cd364024cf814e`.
A separate source author finalized [the stat ruling](specs/alpha-rulings-connected-melee-30-through-33-stats.md).
The implementer read all 216 physical lines and verified SHA-256
`5ad1be59e12ca5a59ac781fecbc195de26423a663ec8fa423cfc45ba7e47a168`.
The ruling identifies the attack reader at RVA `0x046E80` for both hostile
and healing amounts and defense at `0x0467A0` for continuing hostile hits.
The shared stat helpers provide the held rank, equipment order and factors.
The rank-zero term reduces to its exact unit multiplier.

The four actions save their 100 ms pulse bounds before retiming. A projected
successor clears both ID lists before the pulse cue. Action 31 consumes the
first resident type 9 before geometry and candidate handling. Field 26 supplies
the live aim to both geometry and release visuals, and baseline hit sounds
use the native binary32 conversion of the target's fixed position. The
hostile route keeps hit-helper and action-30 proc draws in order and bypasses
local mana rewards. The alternate route rechecks the relationship and emits
its signed heal after a variation draw and an independent critical draw. It
keeps the target's local health intact in this nonlocal turn. Nonfinite state,
original-client combat and imported power's last-bit runtime match remain open.

## Connected ranged dispatch and stat re-certification (2026-11-17)

The isolated author finalized [the ranged dispatch contract](specs/alpha-connected-ranged-dispatch.md).
The implementer read all 256 physical lines and verified SHA-256
`7f114f58e86a2487be5242c31e244846067f5930bcad27948873cd7c1dfcc14a`.
The received turn handles actions 34, 89 and 90 at their pre-ownership
windup site. A change in the signed absolute clock's 200 ms quotient
consumes one pitch draw if the earlier clock precedes anticipation.
Actions 47 and 48 retain their strict zero crossing. The reviewed
nonlocal path leaves the scoped projectile releases and targeted healing
to their owner. Later projectile receipt and world stepping remain open.

An independent source author re-certified [the critical and haste numerics](specs/alpha-critical-and-haste-stat-numerics.md).
The implementer read all 341 physical lines and verified SHA-256
`444bcdfd2701de6eca333bd707c6d37adaca5e57168195754cb2136fd62cab48`.
The current stat helpers agree with the signed raw item level, equipment
order, per-item cutoff and finite speed rules in both original images.
The shared phase helper reuses the pure speed result across scaled legs
of an unchanged actor state. Critical rolls still sample the probability
before the draw. Imported power runtime behavior, exceptional arithmetic
and the full action schedules remain open.

## Original client desktop startup (2026-11-17)

The isolated author finalized [the native startup contract](specs/alpha-native-startup-window-input-boundary.md)
and [the first device request](specs/alpha-native-initial-direct3d9-device.md).
The implementer read their complete 131 and 65 physical lines and verified
SHA-256 `ca67ff1c8e230ceeee711bf5d85d8339b55cb7ce7b0d36fbb1fc031ab739b692`
and `e0a4670c79f6013a9b9afc4045204c8ee050eeafb4099206f527c33bb816e5fb`, respectively.
The original constructs its first windowed Direct3D 9 device before reading
the display options and shows the window only after graphics, XAudio2.7,
input and game initialization. A handled startup failure can exit with zero.

The private x86 diagnostic matched the initial presentation parameters. Its
Direct3D 9 request returned `0x8876086C` under sandboxed execution and zero
with normal desktop access. XAudio2.7 activation, the DirectInput8 factory
and system mouse creation returned zero. A fresh isolated copy of the exact
original executable opened a responsive native window in a normal visible
desktop launch. The window capture tools timed out. Original menu contents,
interactive gameplay and matched image comparisons remain open.

## Connected projectile receipt and relay (2026-11-17)

The isolated author finalized [the projectile receipt and step contract](specs/alpha-connected-projectile-receipt-step.md).
The implementer read all 458 physical lines and verified SHA-256
`3ba7abff8d03e89a6ca97e904fe2c5a50c466c17278c864b6bf83d78fe05d756`.
The controlled connected frame appends pending projectile records after all
world pieces and local-target hit effects. It filters the full local source ID.
Duplicate and unresolved remote records keep their place at the resident
tail, with age, velocity, position and carried words intact. The typed
copy preserves the named fields, while raw transport also retains padding.

The separate [receipt ruling](specs/alpha-rulings-connected-client-receipt-order.md)
corrects the buff address from the earlier hit contract. The implementer read
all 205 physical lines and verified SHA-256
`2d6addb53663f2930344e4adf6af7d02f3d8b64257f412b7433a983a89790b40`.
The client drains the three receipt lists together. Received buffs update
residents before either hit pass and all are retained in the aggregate. The
local-target pass sees those residents and appends its own ward notices after
the received buff records. The second pass appends every foreign-source hit
after projectile insertion. Presentation retains the existing pet adapter.
The intervening object receipt, elapsed-mode flag and stock late capture
effects remain open.

Local launches enter the resident list during the actor's turn and the shared
frame collector in the same order. Late promotion uses the exact source ID
for shots as for hits and buffs. Dedicated pending relays accept complete
records, preserve their wire payload and follow produced shots in each peer
queue. Dedicated relays leave the resident projectile list to local producers.
The exact original dedicated server relayed two ordered 112-byte records to
both connected test clients, preserving the bytes. Only the originating
client's resident list rejected them. Live target resolution, connected
collision, original-client effects and the embedded-host path remain open.

A separate author re-certified [the world-call boundary](specs/alpha-world-call-phase-boundary.md).
The implementer read all 442 physical lines and verified SHA-256
`9e78f9a1c530369cfed4c12d2ec0236ac016291d35c8bbd04ea71ce53a2e3ea8`.
It confirms the client piece sequence and the single dedicated call, and
traces the queue order and two separate zone snapshots. The client retains
one aggregate across pieces. Its earlier terrain records can contribute to
more than one later history pass. That repeated append, the dedicated
drains' separate cutoffs and later frame effects remain follow-up work.

## Connected live projectile contact and steering (2026-11-17)

The isolated author finalized [the steering and impact ruling](specs/alpha-rulings-projectile-steering-and-impact.md)
and [the separate ray and variant ruling](specs/alpha-rulings-projectile-ray-and-variant.md).
The implementer read all 228 and 292 physical lines and verified the respective
SHA-256 hashes
`e37f8d536e3945e290bf539cba994e35af24cf6c56365ec00282f8fee30de1e2` and
`8b218f05331077716f2c15af817cd9b9d138c3036773e59a9aa79bbff7916377`.
The connected world path builds one signed-ordered list of live actor and
body references after each piece's actor turns. Flight uses their current
health, guard, roll, ordinary link and actual position. The list reuses its
storage between pieces. Direct contact queries the current visibility records.

Generic hit generation sees the live connected target and suppresses its
apply stage after the caller's draws. Direct contact still raises the signed
slow counter after the helper returns or bypasses the local target. Resolved
sources can retime and advance their combo regardless of connection ownership.
The variant-two contact uses its separate relationship test and applies a
controlled self heal at the append site. The collected hit remains available
for presentation and send.
Impact carries the raw charge byte and strength.

Type-two outbound and return branches use full fixed differences and the
catch performs two signed, low-64-bit products. The component arithmetic
retains separate binary32 stages. The post-move terrain ray direction,
placed-object clipping, dedicated hit application and original-client flight
comparison are follow-up work.

## Backward projectile terrain and object ray (2026-11-17)

The author independently traced [dedicated resident flight](specs/alpha-dedicated-projectile-step.md).
The implementer read all 404 physical lines and verified SHA-256
`fba0b426b53f06fc04ad537b08c053d5f70b0d2d126c53c635719b7dd04e333e`.
Its fixed ray, boundary constants, side geometry and rollback match the
original client. The shared projectile step now queries from the advanced
position along negated reciprocal-length velocity. The bound uses the full
world piece duration even with several flight substeps. Both client and
dedicated callers supply their live visibility records.

The fixed traversal retains strict distance comparison and reconstructs
the original signed integer indefinite for an invalid boundary conversion.
The point test still owns the arrow's cell-colored effect. The backward
ray rolls the step back without an effect or draw. Signed point sampling
at the negative boundary and the complete solid-cell effect fields need
separate verification. The server's live hit delivery and double-copy
role-zero branch are documented in the held original contract and remain
implementation work.

## Projectile solid-cell sampling and particles (2026-11-17)

The isolated author verified the two native point queries and effect fields
in [the projectile point impact ruling](specs/alpha-rulings-projectile-point-impact.md).
The implementer read all 208 physical lines and verified SHA-256
`6e3ef17af324d74ab0b6019d3235f5269adef48a2015ad797e487fa78ed4f804`.
The advanced point uses signed fixed integer division with the native
negative-axis bias, including exact negative planes. A missing or
out-of-range column supplies the grey solid fallback. Present columns
use their stored cells and upper and lower fallbacks.

An arrow whose advanced point is solid rolls back, reads color from the
restored cell, and appends the burst there in projectile visit order. The
raw fixed point, per-component binary32 color division, velocity, 0.1
size, four-particle field, style and spread survive into the frame's
collector. The ray's shorter-distance path emits no burst. Dedicated
defensive hit duplication is independently documented in the same ruling.
Native particle playback remains open.

## Dedicated projectile hit delivery (2026-11-17)

The verified dedicated flight and defensive output rulings establish when
the server applies each projectile hit and how those results enter the
collector. The dedicated engine now runs flight while the post-actor
index is live. It consumes each produced hit at its emission site. A
qualifying role-zero delivery appends the same record a second time before
the next result. Guard and ward take this path, along with ordinary and
beneficial records. An NPC's local apply can change health, immediately
removing a later living contact when the first hit is fatal.

The producer's feedback uses the same frame buffers as immediate apply.
Impact sounds precede the effects caused by applying the record. Updated
player guard, resources, clock and combo persist in the resident wire
baseline until later player packets replace those fields. Produced
launch records are copied before the resident visit and retain original
age. Packet 9 input continues through the separate verified raw relay.
Matched original-client and dedicated runtime flight and full launch
admission remain open.

## Offline terrain history and blueprint persistence (2026-11-17)

The isolated author finished [the stock writer and persistence audit](specs/alpha-stock-blueprint-writers-and-persistence.md).
The implementer verified all 238 physical lines and SHA-256
`05039a958404a4aec9633426bad5e2581f11304de09d8e8ca181da0e5bfd909b`.
This contract establishes the empty initial build mode and the queued
kind-seven consumer. The original input, settings and transfer paths
yielded no verified nonzero activation writer or fresh model selection.

Offline world producers apply terrain changes at their original actor or
effect boundary and append their edits to the frame list. Each world
piece's tail visits that complete list and appends each record to its
current loaded zone with signed division. Later pieces can append prior
records again without repeating their terrain or request effects. The
zone save retains the sequence, and restore uses the signed day-minus-three
cutoff with 32-bit wrapping. A fresh zone's anchor list stays empty through
the load. An isolated owned-floor diagnostic saved and reopened all 1,800
20-byte records from a three-piece frame and verified the live 600-cell
layout, day-plus-three replay and day-plus-four expiry. Original-client
save and placement interchange remain open.

## Packed terrain vertex light (2026-11-17)

The isolated author finalized [the native terrain vertex light contract](specs/alpha-native-terrain-vertex-light-alpha.md).
The implementer read all 193 physical lines and verified SHA-256
`b3eab091ccaab5f175b28c76b3285dc9cbe6188aaf5db646e207af2d7eb1dade`.
It traces stored light through the ordered 36-cell sample, separate
open-weight RGB contribution, native byte packing and indexed draw.
The shared chunk mesher now accumulates in X/Y/Z order independently of
face winding. Each color lane is truncated to the packed byte and
normalized for the existing Vulkan float vertex input. This applies to
both terrain streams before the existing surface shaders.

The native reference cases produce top-face alpha bytes 254, 4, 128,
238 and 251 for uniform full light, uniform intensity five, uniform
intensity 128, a nearest blocker and a far blocker. These compare the
actual mesh output rather than an isolated approximation of the sampler.
The held native cache-dependent padding and observed bright ground still
need comparison and further verification.

## Terrain environment palette inputs (2026-11-17)

The isolated author finalized [the native palette](specs/alpha-native-terrain-environment-palette.md)
and [the primary terrain and fixed render boundary](specs/alpha-native-fixed-coordinates-terrain-light.md).
The implementer verified both complete readbacks, 251 and 109 physical lines
respectively, and SHA-256 hashes
`370a60d749040120be673236c05901cc84e9b31c23ee46bbc9e18192d084a1b1` and
`9c40c5dc112b965e5a58d1faeef568cfb654cd9ddb4704a4f06d9cd93ec7a59d`.

The world scene now reads its palette factors directly from the live
regional queries at the truncated horizontal camera reference. The sea
share is read when the warm, wet sky branch uses it. The separate
camera-cell type query matches the original whole coordinate conversion and
horizontal bounds, including the lower cell at an exact negative plane.
Its result selects the existing sky replacement and material tint. The
held contracts also trace the terrain direction normalization, fixed
horizontal rebasing and separate regional point-light registers.
Full native camera setup, cache updates and image comparisons remain open.

## Mesh-cache light preparation (2026-11-17)

The isolated author finalized [the mesh-cache lighting contract](specs/alpha-native-mesh-cache-light-padding.md).
The implementer verified all 157 physical lines and SHA-256
`cb36974145f2b4cbbac60bf12a18144335e0fc8f6f34341b7d723c2523677486`.
The contract traces the matching-slot mark, literal request byte,
invalidation, clearance and the distinct four- and sixteen-cell pads.
Its separate resident storage and destruction-margin rules remain
implementation work.

At the world-view update, live changed columns mark matching coordinates
in the 40-by-40 request cache. The signed neighbourhood can end marking
at a world boundary. Duplicate records coalesce and the request survives
invalidated coordinates. A completed light preparation can clear its
request. The engine workers use a frame sequence to preserve a newer mark
that arrived after an earlier terrain snapshot. Geometry can stream with
stored light while required neighbours load. A corner is queued again
when its diagonal becomes available.

The mesh uses a four-cell pad for a pending request, a sixteen-cell pad
at an ordinary area seam and stored values elsewhere. Snapshot
preparation still feeds only the generated mesh. Edited areas still
receive the existing area-wide relight. Native resident work and
commit publication, destruction rectangles and original worker timing
remain open.

## Stored light rectangle propagation (2026-11-17)

The isolated author finalized [the light rectangle contract](specs/alpha-native-light-rectangle-propagation.md).
The implementer verified all 156 physical lines and SHA-256
`0b988277a5d87e92d01101981b514ba2d34e058ac28fcdc549bf0f8aa4afd9b7`.
The original client's instruction trace matched the seed, sixteen rounds
and destination writes in 31 synthetic calls. It checked the quotient
for all 256 neighbor maxima.

Area generation and mesh preparation share the same stored-cell
propagation. It reads a one-cell halo using the previous working byte,
seeds the raw tags, and uses successive whole rounds. Commit stores
the final work bytes throughout the bounded area and the committed
byte inside the destination. Effective implicit water supplies full
light during a neighbor read. Stored flagged water contributes its
actual working byte. Solid cell bytes and material flags survive.
Area generation writes the clear cells in the newly allocated area's
own columns before publication. The generated diagnostic recording
retains the actual four-byte records in the existing container.

The renderer still uses mesh preparation from immutable snapshots.
Cross-area resident mesh writes, live destruction margins and exact
worker dispatch remain open.

## Resident mesh light publication (2026-11-17)

The mesh preparation rectangles in the held cache and propagation
rulings now update resident terrain before mesh dispatch. A background
worker prepares each ordered batch and copies each touched area once.
Each rectangle sees the work bytes left by the preceding preparations.
Before publishing the copies, the world view verifies that the source
areas are still current. An edit or unload prompts a fresh preparation.
Older snapshots keep their original geometry and light. Each mesh job
also receives the prepared light from its own rectangle. Repeated
requests before dispatch share a snapshot.

The connected client's queued edits keep the incoming cell bytes until
admitted mesh preparation. Save restore, blueprint placement and the
existing local destruction paths still use the previous area-wide
preparation. Native immediate destruction margins, original worker
dispatch timing and matched visuals remain open.

## Primary terrain fog and composition (2026-11-17)

The isolated authors finalized [the fog upload state](specs/alpha-primary-terrain-fog-upload-state.md)
and [terrain RGB and composition](specs/alpha-native-terrain-rgb-and-composition.md).
The implementer read all 81 and 206 physical lines and verified SHA-256
`b52ed291351a0d705e1e2b2d038dc64389a912c665f8939cb8f48dd711de2a57` and
`1f7785758067ea4897f0882bffd23861de65a5dbd8b52e91407880069cf42e58`.
The RGB contract checks the two original terrain shaders, their binding
and direct presentation route. Its arithmetic matched 192 vertex and
192 pixel instruction cases. Device precision, inherited render state
and final display processing remain open.

The world view now checks published geometry in the existing chunk
window and subtracts fixed camera coordinates before narrowing each
horizontal difference. Main applies signed elapsed time from low
32-bit GLFW millisecond samples. The fog update uses the same rounded
weight and blend as the camera. The stored distance starts at zero
and takes the original positive floor. A mismatch between the connected
requested seed and active world seed forces that floor. The complete
native pair of requested seed and name and its adoption remain open.
The frame uses the followed camera's actual current angle and selects
90 degrees in the live map. Viewers keep their ordinary camera angle.

The separate source re-certification of [the light lifecycle](specs/alpha-light-propagation-lifecycle.md)
retains the supported original rules and clarifies allocation,
caller-selected byte reads, mesh marking and immediate destruction.
The implementer read all 228 physical lines and verified SHA-256
`d0df53880aba9b4b4cdcd5006a687bc17020d32a53cb3cfe37122215fe50cb0c`.
Original view-window placement, complete input admission and matched
native visuals remain open.

## Combat terrain lighting (2026-11-17)

The isolated author finalized [the combat terrain-lighting contract](specs/alpha-combat-terrain-lighting.md)
from both matched original images. The implementer read all 182 physical
lines and verified SHA-256
`85816cca3ef5ae005ccabbb72da4a5f8e574baeb48f49bcea5965d7d2e477311`.
The ruling distinguishes the capped integer removal scan, signed fixed
light bounds and retained binary32 strike radius. It confirms the
protected clear and shallow-water storage rules.

The Bomb and strike turns run the immediate light callback after an
accepted removal and its original pitch draw. The dedicated caller
writes the bounded resident light before continuing the actor turn.
The stand-alone client uses the same immediate operation when no local
creature is selected. A controlled client defers light to admitted mesh
requests, and the connected client's variable scan is gated out.
Immediate publication writes all three bytes to the destination and
only working bytes to its eight-cell margin. Original runtime playback
and further action and network acceptance remain open.

## Feeding progression and late capture (2026-11-17)

The isolated author finalized [taming and capture](specs/alpha-taming-progress-and-capture.md)
and [the connected receipt supplement](specs/alpha-connected-receipt-supplement.md).
The implementer read all 261 and 284 physical lines and verified SHA-256
`5757945384ee93c044d22301728e174a337d099ea93e6ce89a6735f41ad41511` and
`07a4c300f7a56d5653ae36bf96bc227f995611ee78b44ffdc771d39967c9c172`.

Action 110 admits the bite when the old signed age is strictly before
its period and the end reaches it. The pitch uses the one existing
draw, multiplying before division. The world emits a zero-amount hit
and kind-seven resident progress. At threshold the creature changes
ownership before publishing its full duration. The world piece then
ages the live resident.

Received type-six hits reach ordinary local health and stun processing.
The late hit pass resolves the live source after resident updates.
Capture requires food and the first kind-seven resident with ordered
strength of at least one and the full controlled identity. It writes
the default pet record with the source species, level and experience,
clears the action and links the actor. The offline producer's mod
notification remains at its existing event boundary. Native play,
all offering controls and the separate objective receipt remain open.

## Connected objective writes and late effects (2026-11-17)

The isolated author finalized [state-one interaction](specs/alpha-objective-state-one-late-interaction.md)
and [objective formatting and requests](specs/alpha-objective-receipt-formatting-map-supplement.md).
The implementer read all 102 and 185 physical lines and verified exact
SHA-256 `b4d4a00c03fd16c0ee44f2110b3e5339fae8b1abd21e027aa869d198d1fa5010`
and `3854bf0a5094a7f2f187d334d7e6c991654d5e11209442def8f3f1056716b75d`.

The connected client takes section 13 with its ordered buffs, hits and
projectiles under the frame gate. After buffs it updates present cells
and clears present terrain region request flags using the independent
signed quotient. The live descriptor omits the two padding bytes. After
both hit passes it copies the complete original 56-byte records into the
frame including all misses and duplicates. The late walk gates on each
record's own kind and state. State two resolves the cell again, uses its
current descriptor and first localized page, restarts `shine`, assigns
`objective` and plays sound 30.

State one resolves the full identity, uses an owned speech slot or the
last free one, and prepares the non-innkeeper's seeded text before the
map mark. Map-only actors use their backing region and the controlled
level gate. The speech mode and target come from the received creature
record. The engine stores map discovery sparsely and uses prepared zone
summaries or restored map regions as backing. Native map allocation and
descriptor lifetime still need verification. The ordinary controlled
socket path derives the region from the actor position, with an independent
resident gate and the existing strict sender interval. Embedded host,
local objective producers and native UI playback remain open.

## Connected world readiness and activation (2026-11-17)

The implementer fully read the isolated authors' [elapsed and readiness](specs/alpha-client-elapsed-adjustment-and-world-readiness.md)
and [world change lifecycle](specs/alpha-client-world-change-lifecycle.md) contracts.
The first has 218 physical lines and SHA-256
`d66fb2d16afa20b24e2a7dc0d5d79b2d2f296d0b17ae0062c9e53a5a1776f89a`.
The second has 172 physical lines and SHA-256
`846f93597fd0f39f017987e0738754f7f6ee1e5b1393f3427ed2da2de92d7743`.

Packet 15 publishes one requested pair. An identical pair retains actor
lifetimes. A new one discards received actors and their pending poses.
In-flight preparation drains only previous-world worker jobs. The frame
samples the latest pair under the existing gate, retires the old terrain,
region, mesh and discovery state, and installs the current seed. The same
controlled actor retains health, skills and inventory, and its position
and mirror are set from the matching saved visit or the native home
anchor. Velocity is cleared. The client reopens the named map archive
and uses the exact pair in the local world catalog. It preserves the
network clock and packet-4 lists and active projectiles. World generation
uses the existing ordered seed offsets and asynchronous zone loading.

Both world call paths now use the signed low-32 display tick difference
and compare both identity fields. Receipt work, input and presentation
continue while world calls skip. Fog uses the same display difference and
the identity admission. GLFW sampling, in-flight worker completion and
renderer resource retirement are engine adapters. Connected startup still
uses the initial handshake seed. Native worker contention and cadence,
disconnect races, complete discovery map allocation and shared CRT state
across native threads remain open. No stock elapsed-mode control was
verified.

## Required water behavior (2026-11-17)

The [water extension](water-extension.md) records the owner's intentional
changes to terrain and rendering. Both production entry points always
use the existing swimmable generation profile. Wet surfaces are liquid
and reeds grow on dry banks beside surface water. Native comparison
tools use the native generator.

The production water pass calls the shared Steam fragment algorithm
directly. It reads the opaque background once, writes the resulting
color and retains depth writes. The `swimmable-3-v1` save profile pins
the generation policy. The engine checks for incompatible profiles before
opening a save.

## Connected spawn recovery and support publication (2026-11-17)

The isolated author finalized the [connected spawn recovery](specs/alpha-client-connected-spawn-recovery.md)
contract and separately confirmed the gate for controller role six. The
implementer read all 89 physical lines of the final ruling and verified
SHA-256 `3bb62127332ae2ce46626ba1a0201e17cc3d0f5b349d972b8734b2a7c8c55cad`.

The controlled actor starts an unsaved activation at the exact home fixed
coordinates and zero Z. The ordinary collision box uses the binary32
half extent and signed lower-cell rule. Its first solid overlap permits
one upward correction and clears the drawn-height offset. An unloaded
zone skips the lift and continues through ordinary solid movement fallback.
The controlled creature snapshot now copies the actual contact flags from
that update. The source investigation covers connected local terrain and
leaves special teleport, object correction and original timing open.

## Offline projectile lifecycle (2026-11-18)

The isolated author finalized the [standalone projectile contract](specs/alpha-client-offline-projectile-lifecycle.md).
The implementer read all 163 physical lines and verified SHA-256
`660e2b92ad1a5492fad5d65c92f0d27c953dab5f81acdb3ed853aac7e6d24143`.
The final contract reads the water conversion constant from each exact
original executable and confirms separate binary32 multiplication by five
and addition of five. The existing shared solver matches that arithmetic.

The offline client uses the authoritative projectile walk after the actor
and world object passes. Each piece appends only its own launches to the
resident list while the frame collector retains their original values.
Both controlled and other actors participate in the same signed map order.
Generic and beneficial contact append and apply in place, so later shots
read current health and defenses. Hit presentation takes each output once.
Expiry erases staged residents after the walk, and a converted water spell
begins its burst sampling on the next piece. Native flight and particle
playback, full launch admission and unusual defenses are still open.

## Windowed start menu and gameplay visibility (2026-11-18)

The isolated author finalized the [start menu title contract](specs/alpha-start-menu-title-layout.md).
The implementer read all 32 physical lines and verified SHA-256
`8ef325440ec337c1a67c4e533729b7187f75b98027f7a3a22e1cb3c2146412d6`.
Windowed start mode requests maximization. The title uses integer halves
of the drawable viewport inside its authored transform. Its fixed size
can crop at smaller extents. The Options panel hides the title and start
menu and leaves the independently anchored publisher mark visible.

The front-end screens honor the in-game root boundary from the held
character-screens and corner-map contracts. The gameplay HUD, quick slot,
gameplay panels and corner map are hidden on those screens. The background
world still renders. The engine keeps its configured restored window size.
Original pixel comparisons, native startup message timing and the complete
creation preview state remain open.

## Character creation actor and camera (2026-11-18)

The isolated author finalized [creator admission](specs/alpha-character-creation-preview-entry.md)
and [creator facing and camera height](specs/alpha-character-creation-facing-and-camera-height.md).
The implementer read all 29 and 25 physical lines and verified SHA-256
`85c561cdf02ae3e7d48ba04e9897cd847a63bf0b85959f6689415ca472551f8d` and
`0cc665c27fcf93185df43b736e18ae53eecf6d0f76c48b8340b5d8a8e8a7e22c`.

Start mode initializes the existing controlled actor with offline ID 1.
The title, character list, world list and connection screen suppress
scene actor draws while their world and actor state continue. Character
creation reuses the actor, resets its motion and its positive-Y facing,
and renders it through the world actor path. The unsaved preview takes
the selected appearance, class and generated equipment. Preview grants
keep the existing draw schedule and reuse the result of the existing kit
generator. The preview applies its stats and kit only in unsaved sessions.

Creation entry assigns the current and target orbit before the frame's
camera calculation. It selects the held vertical route that subtracts
the live drawn-height correction and preserves collision shortening and
the prior focus. Complete native actor reset, saved selection and return-to-title lifetime,
and original first-present playback are still open.

## GPU buffer-memory pages (2026-11-18)

Profiling project-owned Vulkan code localized streaming stalls to repeated
mesh-buffer allocation. The implementation now binds each buffer to an
aligned, disjoint range within a 16 MiB page of the selected memory type.
Host-visible pages are mapped coherently. GPU-visible buffer lifetime
continues through the existing frame fence. Canceled pending uploads
return their ranges immediately. Free ranges coalesce, and each memory
type and mapping mode can retain one empty page for reuse. Individual
allocation handles buffers larger than a page and page-allocation fallback.

This is an engine resource-management adapter. Verification compares frame
bytes and exercises buffer reuse and the simulation contracts. Page ranges
follow Vulkan's buffer binding size, alignment and memory
type requirements. The current ordinary buffers have no external-memory or
sparse flags. Images own their allocations. The implementation uses
project-owned code and the Vulkan specification. [Buffer binding](https://docs.vulkan.org/refpages/latest/refpages/source/vkBindBufferMemory.html),
[dedicated allocation requirements](https://docs.vulkan.org/refpages/latest/refpages/source/VkMemoryDedicatedRequirements.html).

## Menu camera focus producer (2026-11-18)

The isolated author finalized the [menu focus producer](specs/alpha-menu-camera-focus-producer.md).
The implementer read all 41 physical lines and verified SHA-256
`b661448b622ec98b920e51a65a167b8a75154702f9d8e9864b6af0164dd4dd63`.
The production camera now uses the existing alternative-anchor solver on
the title, character list, world list and connection screens. Every frame
converts the reset home anchor and samples its current resident column.
The signed base-plus-count sum supplies Z before the exact 50-block lift.
A missing column supplies the stored anchor Z. This height is independent
of the controlled actor's position and motion.

Start mode initializes the actor at exact fixed home X/Y and zero Z, and
ordinary world motion recovers it as terrain arrives. Its default seed
uses the Windows millisecond timer. Explicit diagnostic seeds retain
their supplied value. The camera uses the live resident extent on every
frame, including frames without a world piece. Native first-present
height, refresh-thread timing, alias writes and complete world selection
and save handoff remain open.

## Shared ray action launches (2026-11-18)

The isolated author finalized the dedicated action-family launch contract and
then reconciled its launch-specific fields against both original images.
The implementer read all three complete held documents and verified their
physical line counts and SHA-256 values.

- [Dedicated launch family](specs/alpha-dedicated-action-projectile-launch.md),
  180 lines, `7b15407b86271523b8d95d1ac846d5d8565e7b34d450a8b8e7ad5a6524d1bad7`.
- [Launch field reconciliation](specs/alpha-ray-launch-fields-reconciliation.md),
  65 lines, `9d1bcb0987177d9eef3f5c747b7743a6334db13fddd8c67d26e0fa84ed52ee65`.
- [Post-hit timing](specs/alpha-ray-post-hit-timing-evaluation.md),
  85 lines, `da118c42a9e3c7b69b09d7fe2d3537c95a0815fdbbce8407ccaf4cc26d37d492`.

The reconciliation supersedes the first document's strength-field value.
Both launch sites explicitly overwrite that field with zero, preserving the
constructor's scale value of one. The prior held beam record agrees.
The timing supplement establishes fresh threshold and current-age reads at
each secondary gate and the late action-94 sound gate. The empty-ray rescale
precedes the action-28/95 sound-grid calculation.

One shared ray dispatcher now serves actions 28, 94 and 95 in dedicated,
offline and connected play. Its source gate precedes resource work. It keeps
the native signed age arithmetic, first-crossing and repeat rules, signed
actor order, ordinary-link exclusion and struck-set lifetime. Each admitted
resolver completes before the secondary cloud gate, including false returns.
Every cloud evaluates its magic amount afresh and preserves its raw fixed
center. Connected specialization-two drains emit their self record without
the mode-zero direct health mutation.

Production client and server world calls bind their resident projectile list
during actor work. Each launch appends its resident value before its outgoing
copy, and the flight tail advances those values once after objects. Magic
launches insert residents before positive-charge spending and proc reset,
then append their outgoing copy before the firing sound. Existing standalone
callers retain the unbound launch-range import at the flight boundary.
Native combat playback, full upstream eligibility, rate and amount formulas,
unusual numeric inputs and inherited provenance gates remain open.

## Native generation-worker controls (2026-11-18)

The isolated source author finalized the [generation-worker FP contract](specs/alpha-generation-worker-floating-point-controls.md).
The implementer read all 132 physical lines and verified SHA-256
`1e6d5b9bde409ebb12c17ab49fc63bd2135bc396b739caf05c6a9c418c3ad324`.
The static contract traces the process precision request and the checked
thread-creation route. It retains operating-system initialization and other
loaded-library effects as runtime dependencies.

A private black-box observer ran two fresh, hash-verified original client
copies on the normal desktop. Hardware execution breakpoints used only
observation addresses from the held contract. Debug events suspended the
threads for WOW64 context reads. Context writes were limited to hardware
debug registers and resume bookkeeping. The floating-point fields were read through the documented Windows
context and debugger APIs. The observer used the held RVAs and process
module metadata.
[WOW64 thread context](https://learn.microsoft.com/en-us/windows/win32/api/wow64apiset/nf-wow64apiset-wow64getthreadcontext),
[debug event suspension](https://learn.microsoft.com/en-us/windows/win32/debug/writing-the-debugger-s-main-loop).

Both runs loaded the bundled runtime from their own isolated directory.
The client and loaded file matched the held hashes before and after each
run. Each generation thread was observed at application callback entry,
worker entry, and three consecutive pre-root-call and root-entry pairs.
The legacy and extended context views agreed on x87 control word 0x027f.
MXCSR control fields were 0x1f80 at every sampled boundary. Later sticky
status flags differed between runs while those control fields stayed fixed.

These observations establish 53-bit x87 precision, nearest rounding and
masked exceptions at the sampled client-worker boundaries. SSE nearest
rounding, masked exceptions and disabled flush-to-zero and denormals-are-zero
were observed at those boundaries. Complete nested-call persistence,
dedicated-worker observations, other runtime/driver combinations and the
retained one-byte rock-color discrepancy remain open. The observation
perturbs scheduling through debugger stops and supplies no frame-timing
or original-menu acceptance claim. Probe files, native copies and logs
remain private and excluded from source and packages.

## Rock cover roughness order (2026-11-20)

Isolated source-only authors finalized three observation contracts. The
implementer read every physical line of each and verified its SHA-256:
[zone-generator boundaries](specs/alpha-zone-generator-observation-boundaries.md),
38 lines, `c75e45d276db79e7ede3d9a92edf6193c6a261f4458a05e0babc61eac6e783ce`;
[initial column-color boundaries](specs/alpha-column-initial-color-observation-boundaries.md),
57 lines, `0dbb426212d44712627d4832f5b49da1c566fa9173e6faf0b9fa911ed312b88a`;
[surface-cover boundaries](specs/alpha-surface-cover-observation-boundaries.md),
116 lines, `599c909fffe6bc683f94670a5d9b886604b82e0083011ab79574e7c0fccdbc7c`.
They name observation points only. No runtime value or implementation detail
was sent to any source context.

A private black-box observer ran a fresh, hash-verified original dedicated
server and a project-owned loopback peer. The peer now presents a player
record (controller byte 0), because the held server-simulation contract
generates zones only around player creatures. Earlier runs left the default
controller byte and generated only the home zone. Hardware execution
breakpoints at held addresses read the target column of seed 41, zone
(32804, 32800), column (8398044, 8397004).

The stored initial pair matched the engine. Every rock, extra and lower-mix
vector matched bit for bit. The first mix weight, which is the rock cover,
was 0x3f0c21e0 in the original and 0x3f0c21e4 in the engine. The surface-cover
points then showed v1, v2 and T equal and the mountain factor P different
(0x3ed956c4 against 0x3ed956c9). Inside the mountain factor, every stage up to
the shaping-field products matched. The difference came from the roughness
term of `alpha-worldgen-terrain.md` section 3.3 step 3, held as t²·0.05. The
engine evaluated 0.05·t·t, which rounds one binary32 unit higher at this
column. The engine now squares first. P, cover and the ravine floor then match,
and the retained floor cell is (189, 179, 178, type 6) as in the original.
WorldgenTests pins the roughness, mountain factor and floor bytes. The full
209-zone native corpus is recorded in verification.md. Stored-light
differences, native pixel acceptance and the remaining open gates are
unchanged. Probe files, native copies and logs remain private.

## Class B re-certification, first batch (2026-11-21)

Fresh source-only authors re-certified eleven held-but-unpinned contracts
against the original images and appended dated re-certification sections.
A ruling author settled the two conflicts raised by the creature-defaults
work. Fresh implementers read each document whole and verified its SHA-256:

| Contract | Lines | SHA-256 |
| --- | ---: | --- |
| [buff record replication](specs/alpha-buff-record-replication-recovered.md) | 582 | `17e8faed6b6ddf2d9fb0cc1a8a4bdac6b13bf5dd4b981b8dd716fffbea021cc8` |
| [rock-color precision](specs/alpha-rock-color-precision.md) | 494 | `a1d8d87a3c099df4f459bf943f3a3611da310215d402304e07158f7a2ca21495` |
| [noise cosine numeric contract](specs/alpha-noise-cosine-numeric-contract.md) | 239 | `3e0af7bb3e5a9a23ca31d969bc4da83290c5002ac021c606292cde8b620f1e76` |
| [noise cosine boundary](specs/alpha-noise-cosine-boundary.md) | 143 | `bd41baeef0f19b19bfcc3150b66a45bde113a74b20184b04e2e5f810adc54df0` |
| [floating-point environment](specs/alpha-floating-point-environment.md) | 498 | `dfdf97f8eb9c5409a6f314022c46ed1fb5628fbc1472eb4a404c6c2678191b5d` |
| [common hit input construction](specs/alpha-common-hit-input-construction.md) | 225 | `f884af5129e7aecf69edcf088425e96215e5f953fb0f8aa1e90e856bb17b4712` |
| [client primary start boundary](specs/alpha-client-primary-start-boundary.md) | 220 | `a8cf02e79299ae3395d6c25081638919799dde71859c94a4d5f64732f1d8def7` |
| [world actor creation](specs/alpha-world-actor-creation.md) | 231 | `fac528b4a335b5a2b425a44b0a6065fef06119134c2dc6b0491571db9dad8790` |
| [fatal region credit order](specs/alpha-fatal-region-credit-order.md) | 201 | `bc909ce0511bf627cdec4abaea3b5db3b16a6232c21ac6626d7e298ee698fb19` |
| [ranged audio boundaries](specs/alpha-ranged-audio-boundaries.md) | 179 | `8da9c3f97895509665430cc48f9501c594ae550a7d2116b4f91eeee44ef24282` |
| [network creature state defaults](specs/alpha-network-creature-state-defaults.md) | 194 | `c5832ceec180be8bf9e4aceefa8131063a98741d3b9b5406650f40852a9476e6` |
| [creature codec and join ruling](specs/alpha-rulings-creature-codec-and-join.md) | 231 | `b547516267c7f0bee47ddc4ed03a0dfd8ce5438ad8ed1b143419c2f9855b8bc8` |

The floating-point environment implementer read revision
`e0cd31f8423fa6ef7f7f137fbef7598f0f57c9b73c62183b5657ca45d849a703`; its author
then only removed trailing spaces from two header lines, adding a blank line
after each, which gives the recorded revision.

The ruling keeps 48 creature codec groups, as the packet contract states,
and confirms the defaults contract's join stores. It supersedes the 1.2,
1.2, 2.7 join box and the initializer-default claim of `alpha-net-server.md`
section 3.1: the join runs the appearance generator last, which makes five
discarded CRT draws for type 44 and leaves height bits 0x400A3D71.

Engine changes: type-1 buff clears end at minus the update interval; server
packet-8 events start zeroed and keep a partial event at end of stream;
signed buff-list counts; resident list limit. Roughness, rock-color and
cosine class routing follow the held order and widths, with a bit-exact
rock-color reference test; generated terrain is unchanged. Common hit input
queries strength before the size ratio and follows the held operand order.
Magic-user secondary selection draws only for ordered mana above 0.5. The
five common appearance draws build the native temporary descriptor. Fatal
credit and the completion reward use native truncating conversion. The
dedicated-server join starts from construction defaults and follows the
ruling's write order. The floating-point and ranged-audio contracts needed
no code change; the engine's default MXCSR and nearest integer conversion
are now tested.

Not modelled: world-mode buff clears (no engine path), the native
truncated-count loop, the x87 cosine fallback above 90112 (the engine uses
the library cosine), the server spawn position source and join health
value, and the open points each contract lists. Engine choices: zeroed
alignment and outer bytes in server packet-8 events and deterministic
zeroing of unspecified record bytes.

## Class B re-certification, second batch (2026-11-21)

Fresh source-only authors re-certified three more held-but-unpinned
contracts. Fresh implementers read each whole and verified its SHA-256:

| Contract | Lines | SHA-256 |
| --- | ---: | --- |
| [ranged action scheduling](specs/alpha-ranged-action-scheduling.md) | 183 | `3dd2add88b5252e0f0c8c7dba847b51b7531284cd22c9264417650a9d4961dce` |
| [maximum health numeric contract](specs/alpha-maximum-health-numeric-contract-recovered.md) | 183 | `e48545ff24b9ea9fd82d284c4bf167f118da580ccaadd08f42b36da52e3b24fd` |
| [client receiver publication locks](specs/alpha-client-receiver-publication-locks.md) | 161 | `715648c8a49e71ef71501b67794f6310d03d5e16135f4c08dc9ae2af2e0478a1` |

Engine changes: the dedicated-server join stores health from the real
maximum-health query (100 for the join state, as before); the item level
factor follows the held operand order. Magic projectile speed tests the
main-hand weapon identifier byte alone; the magic firing sound is chosen by
class and specialization; the AI low-mana test compares in double. The
client receiver returns to its loop for unhandled kinds without reading a
payload, treats only a socket error as a stop for in-message reads, keeps a
partial value at end of stream, publishes no batch interval for the first
end-of-batch message, and holds 1 ms timer resolution while receiving.

Engine adapters: a closed stream at the next header ends the receiver; a
failed creature allocation throws instead of dereferencing a null pointer;
a short decoded kind-0 buffer stops the receiver; unread value bytes are
zero. Not modelled: the dedicated server's own healing caller, kind-1 and
kind-3 lock intervals, zero-direction shot normalization and the open
points each contract lists.

Record correction: in checkpoint c538c4a the batch-73 sentence of
provenance-audit.md lost its code-span backticks and carried control
characters in place of the leading letters of the names. This checkpoint
restores that sentence; the table rows and counts were unaffected.

## Class B re-certification, third batch (2026-11-21)

Fresh source-only authors re-certified three more held-but-unpinned
contracts. Fresh implementers read each whole and verified its SHA-256:

| Contract | Lines | SHA-256 |
| --- | ---: | --- |
| [spatial scalars 0x1EF040 and 0x1F0720](specs/alpha-spatial-scalars-1ef040-1f0720.md) | 168 | `6db93a4dcc20bc17cc906deeba7373bbb332b25554a9869a28e20a71faf0696b` |
| [created actor behavior](specs/alpha-created-actor-behavior.md) | 182 | `73250f0f10475b3cf453178d1a1b11d94f371a1e7b741b11a68f54e5a30f9a0d` |
| [created companion navigation](specs/alpha-created-companion-navigation.md) | 151 | `36fd84bb0eafd718338c9e9eb5d9157f1d65c5533050dc6d902aaa2c1ccf53db` |

The spatial-scalar engine already matched every correction; its constants,
truncating caller division and reference pairing are now named and pinned
by bit-exact tests against an independent restatement of the contract.
Engine changes: an accepted created-actor special writes only the interval
20000, the action and a zero clock; the path-expansion step takes the first
pending key as its default choice and always removes the choice (this also
removes an end-iterator dereference); the creature reset clears the motion
triplets, sets the start cell to all ones and the arrival radius to zero.

Not modelled: actor +0x13E0 (no engine field, reader untraced), the
constructor discriminator default, world byte +0xB4 (no replica NPC world),
record eviction, and the helper internals and later first-child dispatch
each contract leaves open.

## Dedicated NPC publication and fatal-credit consumption (2026-11-21)

A separate source-only author rechecked the original client and froze the
fatal-credit consumer contract. The implementer read all 102 lines and verified
SHA-256 `9733222497603ae99210d070e9aeba5f1640a31d8a86c6ad6b64b26a28d6bcac`.
Cleanup reloads the designated creature before clearing its contact. A direct
award writes experience to the admitted creature, then reloads the designated
creature for level processing. The corpse reward adapter and the level
threshold use the existing signed truncation helper. The two credit callers
retain their existing record and random-draw order.

The clean-team publisher read the held creature-state defaults, codec/join
ruling, server simulation, motion and fatal-context contracts. Each NPC
snapshot starts from a shared native constructor template and overlays the
live actor fields. The network copy preserves the backing descriptor, raw
home position, pitch, bank, head pitch and contact. Reset pets carry the
unassigned dialogue coordinates until a producer supplies a destination.
NPC names and the third dialogue word for all possible descriptors remain
open. Native packet capture is a separate acceptance step.

## Owned save import (2026-11-21)

This AlphaSetup feature uses the project's existing save codecs and SQLite.
It stages the original list databases, per-world state and maps in a fresh
directory. Once validation succeeds, each staged world receives the unmodded
swimmable-water profile and the directory is published. The ordinary profile
check continues to protect established engine worlds. Catalog rows and
unrecognized blob metadata travel with their databases.

The clean team designed the import and its filesystem handling independently.
Original saves are read-only inputs to verification. Runtime interchange in
the reverse direction requires separate original-client evidence.

## Terrain mesh source residency (2026-11-21)

A separate source author finalized `alpha-client-terrain-mesh-dispatch.md`.
The clean team read all 50 lines and recorded SHA-256
`6da3401a77468922723c60e63cafea1ee4ab69e97ec8c50273e3232dd23fb4c2`.
The engine checks the required zone references before preparing light or
geometry. It rechecks them before accepting asynchronous work. An arriving
zone reconsiders affected side and corner chunks. A dirty chunk keeps its
published mesh while waiting for a replacement. The contract also describes
the worker cadence, sorting and vertex allowance. Those require separate
implementation and verification.

## Terrain mesh distance order and camera origin (2026-11-21)

The source author independently finalized the following two contracts. The
clean team read both in full before implementing the ordering and conversion.

- `alpha-terrain-mesh-distance-ties.md`, 94 lines, SHA-256
  `dd5d1c17f5aad2743a7dac94586967651fe16de2be7eef1805b8336692fffc44`.
- `alpha-terrain-mesh-window-origin.md`, 57 lines, SHA-256
  `781a5c4f9b83278179fab0c88968263ae7a08f49fe6326576c11c966b94d3619`.

The engine derives the window from the adjusted fixed-point player camera
and clips each candidate before ordering the complete window. It filters
that order to queued requests. Fog and ambience read the published origin.
Preload and free-camera modes use the engine camera until a player eye is
available. Work beyond the native window in wider engine views remains an
extension. First native publication timing, worker pacing and vertex-budget
eviction require separate evidence or implementation.

## Original client startup prerequisites (2026-11-21)

The source author traced startup independently, including dependencies,
initialization order, file failure paths and when the window is shown.
The clean team read all 51 lines of `alpha-client-menu-startup-prerequisites.md`
and verified SHA-256
`b0a161c4e8b1b3f3232065b2daeadb6803278f2a0aee2e184c939ed3e19db820`.
The verifier separately compared isolated native launches and queried
the availability of the two system DirectInput devices in a 32-bit process.

## Terrain mesh worker cadence (2026-11-21)

The clean implementation follows the held mesh-dispatch contract recorded
in batch 77. A dedicated below-normal-priority worker completes at most one
eligible chunk's CPU lighting and geometry per pass. It requests five
milliseconds of sleep after each pass. The render thread accepts completed work
and owns Vulkan uploads and retirement. Region replacement, edits and world
transitions invalidate superseded work. A prior drawable mesh remains in use
while its replacement is being built. Elapsed wall time depends on the work
and Windows scheduling. Native buffer transfer and budget eviction are
separate checkpoints.

## Terrain mesh allowance and active groups (2026-11-21)

The source author independently traced the builder's face emission, primary
record count and the distinction between pending and active mesh groups.
The clean team read the complete held records before using them.

- `alpha-terrain-mesh-vertex-accounting.md`, 65 lines, SHA-256
  `eab5747e0c846bfdc572286d9a60c6801bf38de5457b4ac40ea234b29c0b8405`.
- `alpha-fog-mesh-slot-population.md`, 47 lines, SHA-256
  `4ddfe050109d198d1aa4732cfc7cb9cc80cc7ebae2b7c4ad35e3ca7f7e0a5558`.

Each successful terrain or liquid quad contributes four primary records.
The worker scans the complete clipped window, adds the slot's current
count after any build and evicts subsequent slots once the total exceeds
the allowance. Reusing a physical slot clears its previous owner. A
matching empty rebuild charges zero and can retain its active geometry.
World reset and budget eviction release that geometry. Extension chunks
beyond the native window are accounted for separately. Matching renderer
transfer of pending groups is a separate checkpoint.

## Terrain face direction order (2026-11-21)

The direction sequence follows the held vertex-accounting contract in batch
80. The clean implementation reorders its existing face descriptions. Their
corner geometry, winding, liquid indices, shading and sway use the existing
paths. The directional visit order and the number of charged vertices are
verified independently.

## Offline character placement on world entry (2026-11-21)

The clean team used the held menu camera and controlled actor contract
from batch 69. A selected character enters at its saved position for the
world's seed and name. When that entry is absent, ordinary local selection
uses the reset home anchor. The actor copies all three signed coordinates
to its drawable position and clears velocity before motion. Terrain and
camera setup use a separate view of the coordinates. Offline character
saving records the body's fixed position directly.

## Terrain fog and renderer transfer (2026-11-21)

The source author traced the physical dirty-key census and clipped renderer
visit independently. The clean team read all 47 lines of
`alpha-terrain-mesh-group-transfer-window.md` and verified SHA-256
`c5b45cf4a14d4994d5d4844e5f052d24656be15273400a4f3738b8eddea458fb`.
This supplements the held fog population and vertex accounting records.

Fog samples active geometry before renderer transfer. A nonzero dirty
timestamp anywhere in the physical cache can delay acceptance for another
slot sharing that timestamp. The renderer visits valid window coordinates
in X-major order and can accept pending geometry for an offscreen slot.
Drawing still checks the coordinate and visibility. Vulkan stores its
terrain and liquid streams by material. Their nonempty state supplies the
fog predicate. The native vertical bands and their exact lower bound are
documented in the contract.

## Local front-end terrain readiness (2026-11-21)

The clean team read all three final source-author records before using
their findings. The front-end terrain readiness record has 39 lines,
SHA-256 `868f6742d9fb7a05cb10a9e9b4b7ca602ffbf1825ebe5ce0f48eb37782ffd708`.
The wait presentation has 34 lines,
SHA-256 `9036b34cf8ee9507fb0aec07ab3620ae4bc2dfade7cab4cc20aa927af4e311c5`.
The creation cursor supplement has 23 lines,
SHA-256 `f1ff918ae2f027d7bd53da63afc85d6f3fd9e6ab8a8db161d471ade8725a45a0`.

Ordinary local front-end entry allows terrain generation to overlap
display and input. The selected world, visible screens and current
smoothed fog distance determine whether the scene or wait UI appears.
The renderer can accept terrain on either path. A creation wait retains
its independently visible controls. The renderer's window and GPU
lifetime management are engine implementation choices. The original
records establish no fixed elapsed time for the first visible scene.

## Resident terrain publication and object lifetimes (2026-11-21)

The independent source contract establishes that lighting and mesh
publication preserve the resident zone's spawn descriptors and indexed
objects. The clean team read the complete 31-line held lifetime record,
SHA-256 `b6b0d66524c0d4bec6fd831e34f5c3051c0476639e3b98a384e222074f98a720`.
The contract also describes one decoration light value written before
meshing copies the decoration. Matching that value remains open.

The engine gives a zone's resident records an identity independent of
its immutable terrain snapshots. Snapshot publication during relighting
or a terrain edit preserves record ownership. Full zone replacement or
departure ends it. The renderer can reuse a writable area across
publications in the same batch and takes a new copy when another reader
holds the current version. These ownership and allocation choices are
specific to the replacement engine.

## What the resident column recordings establish (2026-11-21)

The clean team read the full 35-line held source contract on live column
observation, SHA-256 `7201e8ca2afd1b2628038499c36af138ebe65a822f3f66b559d79c90353d8b01`.
It supersedes the inherited extractor description where that description treats
resident column bytes as unlit or mesh work as read-only. Generation lights a
new zone before publication. Mesh work can subsequently write the same
resident columns. Main simulation also has a conditional lighting route.
The original routes use two distinct critical sections, with conflicting
nesting orders during zone and region departure.

The project recorder holds one of those sections across its reads. Historical
AVZD files preserve the observed samples. They lack the writer phase, camera
sequence and mesh scheduling information required to reproduce a particular
frame. The project changed measurement labels and the committed water byte
comparison. The native locking and recording routes stayed as previously used.

## Streaming radius at world entry (2026-11-21)

The CLI loading radius and the conversion from the options menu's
render distance to the engine streaming radius are engine controls.
A private instrumented run found that startup applied two different
values, depending on whether terrain had already been loaded. The same
source recordings and native mesh-budget contracts apply. This adjustment
changes which zones the engine requests when a radius is explicitly set.

## Addressed item result presentation (2026-11-21)

The clean team read the complete 41-line held contract, SHA-256
`36e06dd434f8d62a3867df267b2c103fb06bdbe55f81e38c1b068909779b826b`.
The source author independently established the dedicated section's
audience, the two frame-time lookups and the UI, sound and progression
order. The earlier mixed sender-turn FIFO contract still governs request
consumption and random draws. The client retains each addressed result
through delivery to its game frame.

## Active terrain decoration lighting (2026-11-21)

Two original-only contracts were held and read in full before integration.
The 46-line sampling and rendering record has SHA-256
`aa081b1acb7a6e4238368dee126bf17be5fd02f99fd9900b583ea5e79a9e72e6`.
The 70-line model and distance record has SHA-256
`f407aaa690ecf112d0affde75bc4872c804ee1b2df883237cfe10a65125b0f8b`.
The latter supplies the ordinary model lookup keys. Its slot bounds,
distance gates, sorting and final render state have a separate delivery
and verification scope. The clean implementation retains immutable
records for each resident chunk and updates the Vulkan draw material
when the active mesh changes. The engine keeps bank reeds on their
owner-requested placement path.

## Initial decoration lighting and the final static index (2026-11-21)

The clean team read the complete 38-line original-only correction.
SHA-256 `36141cd0bd622d9ac28f1bd3e1c45b28eef6f165f54e150df98abe1cf2834f0e`.
Generation refreshes the existing decoration records after column lighting.
The following pass appends references to original static ordinals in their
eight-block buckets. This corrects the earlier interpretation that the pass
generated more decorations. The engine now samples ordinary plants, sprites,
leaf sources and song positions before it publishes a fresh zone. It also
resamples the engine's restored terrain after applying saved block changes.
The original's live restore timing remains open.

## Decoration support after client terrain edits (2026-11-21)

The clean team used the held resident-zone contract covering the client
batch, its affected bounds, the support flag and positive saved-edit count.
Its SHA-256 is `b6b0d66524c0d4bec6fd831e34f5c3051c0476639e3b98a384e222074f98a720`.
The check uses the edited terrain and includes resident areas between the
batch endpoints. The engine compacts its typed decoration collections and
retains earlier snapshots for readers. Indexed statics and spawn records
keep their identities. Original-client playback across a live removal and
restore remains open.

## Revised shared water surface (2026-11-21)

The Alpha adapter uses the owner's project-owned GPL Steam engine shader
at commit 41ae7d1be330b171a846845f4000f4b22a51dd42. Its shader blob is
`aa23b6b16a14336886fc2202eeaab263f22f618f`, also present on the verified
Steam main revision 354d3349f9f91d79ad0d08d2966d5437a3aab2e2. The shader's
SHA-256 is `4d72d1c975bf3b3344fc50446203b9b99e5b6ea9a12e84e7d38a1c99f9a0ca52`.
The clean team also read the complete Steam project records for the later
appearance (SHA-256 `3d5e81ca56d5767a019b0ed068a4afc580ce68c0e799dfda5e06ce32df0bc5c5`)
and wave offsets (`c1ac07764a504ba3675c37c0434314f45f83efdf90aa6e99d4f02c99a7d59271`).
The visual treatment, swimmable shallows and bank reeds are owner extensions.

## Temporal anti-aliasing stability (2026-11-21)

At the owner's request, the Alpha renderer uses the owner's original
Infinite Cubes Vulkan tests temporal resolve and camera-history calculations.
The source revision is `71611c67acc7e82862b150d1142fe2d9aeda4b37`. The
shader's Git blob is `c8ec11f66ff7bf98484d4e9d0bf58baf987ed77e`, also
present on the verified main commit `1b001b697c50466589a53c3cc85189ad5d6d46d8`.
The GLSL adapter uses Alpha's positive-height Vulkan viewport and local
terrain coordinates. It keeps resolved color on the output grid, tests
previous jittered depth separately, and translates the previous view
when the terrain origin moves. Eight centered Halton samples have an
overall scale of 0.60 pixels by default. This filter is an engine extension.
[Owner's temporal resolve](https://github.com/DieuDeGlace/infinite-steam-vulkan-tests/blob/71611c67acc7e82862b150d1142fe2d9aeda4b37/release/src/gfx/shaders/TemporalResolveShader.hlsl)

## Character entry, recipe grants and progress presentation (2026-11-21)

The clean team read both finalized original-only records. The reviewed
223-line recipe contract replaces the earlier 85-line version under the
same name. SHA-256 `3eb14ec476affc27ded0b4c31b1fd52e9ffee03807333ed927acc574d3c2325d`.
The separate 100-line progress-presentation contract has SHA-256
`53d040c77da41242c34e719b96bd437a4b833a075783871d4366d8b497bc2b20`. Creation, selected-character loading, local and
accepted network entry, and the observation after credit processing were
implemented from those records. The engine's explicit unsaved diagnostic
character has its own starter grant. Original update cadence, visible
text and device playback remain open.

## Bag and crafting scroll controls (2026-11-21)

The clean team read the complete 147-line source-author contract and the
earlier recipient presentation record. The scroll contract has SHA-256
`f94078c8e7b660ae32d986bb60e77736bfd7f942fcd1d0822e4029e4effb764d`. Its measurements and event order govern each list independently.
The controller cursor maps engine input onto the same movement events.
Original skin transforms and the initial drawing of an unplaced child
remain open.

## Terrain slots, scenery distance and chest assembly (2026-11-21)

The clean team read each complete held source-author contract below before
implementing from it. The indexed model table follows the held
`alpha-interaction.md` section 4.2.

- `alpha-terrain-box-range-and-static-fade.md`: 105 lines, SHA-256 `c41d587cc3ab65815416b7e938879e1eb55d08cd0fc0b80bff4ea282272fc085`.
- `alpha-terrain-slot-bounds-and-static-model-admission.md`: 177 lines, SHA-256 `a443642e37f537ca2b489dd98a3c2b139f41a226438e5c03490ddbbde4f37b84`.
- `alpha-terrain-slot-sort-key-and-pass-order.md`: 143 lines, SHA-256 `655a78c0d372fa05555fc0cbfba3b023509f6f6f9e740e82133ec942c024fcf8`.
- `alpha-indexed-static-kind-10-base-and-lid-transforms.md`: 134 lines, SHA-256 `b165ce011922d4a4d09580a5f680b2768b9c1cb4e9f46fdf39a0b6bfc78d32dc`.

The clean team requested the chest contract after an engine capture showed
a detached lid. Its author did not see that capture and worked only from
the original executable and content store.
The implementation places both parts in original grid-corner coordinates.
Per-band native draw identity, long equal-key ties, timed slot effects and
original framebuffer comparison remain open.

## Terrain-slot ambience (2026-11-22)

The clean team read the complete 138-line held source-author contract
`alpha-terrain-slot-presort-ambient-audio.md`, SHA-256 `754acf4e39d33c69421f5ff1790988b5357acf698484cee421c4fa986f8677e3`. It governs where ambient calls are decided,
the retained slot key, the table, the draw order and the hearing cutoff.
It replaces the earlier engine adapter built on `alpha-rulings-zone-fields.md`
ruling 1, which remains a held record for the facts it states. The clock
producers, the frame increment's source and the native random stream's
history outside this branch remain open.
