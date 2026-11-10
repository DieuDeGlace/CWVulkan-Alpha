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
