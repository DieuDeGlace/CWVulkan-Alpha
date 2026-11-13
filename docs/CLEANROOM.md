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