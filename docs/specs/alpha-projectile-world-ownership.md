# Alpha world mode, controlled-creature pointer, and first projectile-source admission

Status: FINAL / STABLE / HELD. This is an independently worded, bounded source-side behavioral contract. It does not certify a replacement implementation or runtime acceptance.

The evidence root is `disassembled alpha cube world`. The supported original images are:

| Image | Size | SHA-256 |
| --- | --- | --- |
| `binary/Cube.exe` | 3,885,568 bytes | `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf` |
| `binary/Server.exe` | 1,718,784 bytes | `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1` |

Both use preferred image base `0x00400000`. All instruction/routine addresses below are RVAs, with the image identified when server evidence is used. Object offsets are explicitly marked. Evidence consists only of individually identified original routines, original assembly and supported executable bytes. No other contract, reconstruction, implementation, runtime capture, test, or implementation history supplies facts here.

The established interpretation is narrower than a universal client/server Boolean: world `+0xB4` is a byte switched on during the client's outbound connection attempt, while both initial local-client simulation and native server startup construct it as zero. The pointer at world `+0xB8` is the client's locally controlled creature in the verified client assignment paths; native server construction sets it null. A null projectile hit source is not itself a rejection in the first later client candidate path.

## Field identity and initialization

World `+0xB4` is one byte. World `+0xB8` is a native 32-bit pointer. Relevant projectile fields are kind discriminator `+0x60`, a 32-bit quantity; element `+0x64` and flag `+0x5C`, one byte each; and signed 32-bit clock `+0x68`. Candidate creature `+0x16C` is binary32 health, `+0x128` is a 32-bit lock, and `+0x60` is a one-byte control discriminator.

The client world constructor at `0x18EB00` copies the low byte of its second explicit stack argument into world `+0xB4`, then writes null to `+0xB8`. It does not normalize the mode argument to 0/1. A later point in the same constructor writes null to `+0xB8` again. The native client application constructor calls this world constructor with mode argument zero.

The client embeds that world at application offset `+0x2E4`. Consequently application `+0x398` aliases world `+0xB4`, and application `+0x39C` aliases world `+0xB8`. These aliases are necessary to identify the connection transitions; a search only for the literal world displacement would miss them.

The original server world constructor at server RVA `0x0C8570` has the same relevant initialization: copy its supplied byte into `+0xB4`, write null to `+0xB8`, and write null to `+0xB8` again later. Server startup at `0x149C50` allocates the world and calls that constructor with both explicit arguments zero. The server then supplies this world to its network/session object and later invokes its world update at `0x1322D0` from startup-loop site `0x14A0AE`. This establishes a native running-server startup path with mode zero and no constructor-assigned local-creature pointer. It does not establish that every server caller preserves those values forever.

Sources: client `decompiled/client/functions/0058eb00.c`; `binary/Cube.exe`, byte copy/null write `0x18EC09–0x18EC12`, later null write `0x193A9D`, client constructor call `0x059F31–0x059F57` with object address application `+0x2E4`; matching `decompiled/client/assembly.txt`. Server `decompiled/server/functions/004c8570.c`, `00549c50.c`; `binary/Server.exe`, initialization `0x0C8679–0x0C8682`, later null write `0x0CD50D`, constructor caller `0x149CB7–0x149CC4`, update caller `0x14A097–0x14A0AE`; matching `decompiled/server/assembly.txt`.

## Meaning and verified assignments of `+0xB8`

The client setter at `0x087E90` copies its supplied four-byte argument directly to world `+0xB8`. It performs no null, ownership, creature-control, or world-membership check. In the observed application-construction caller, the argument is the pointer stored at application `+0x8006D0`, and the setter's object is application `+0x2E4`. The preceding construction region also places that same creature pointer in the native world creature mapping under 64-bit identifier 1.

The same application `+0x8006D0` pointer is used by a native primary-input call site at `0x09BF70–0x09BF82`. This connects the pointer assignment to the client's controlled creature, rather than inferring its role from the field name or a single combat comparison.

The network-connection routine later has a separate direct assignment to application `+0x39C`. In the accepted-connection branch with initial record kind 0 and a nonnegative signed 64-bit supplied creature identifier, the client resolves or creates its controlled creature, updates application `+0x8006D0`, applies the received data and local auxiliary values, and then copies the resulting pointer into application `+0x39C`. Thus world `+0xB8` is refreshed to the current controlled creature in that branch. The assignment occurs after the relevant creature replacement/data work, while the world critical section is held, and before the subsequent region refresh and worker startup.

This assignment is conditional. The audited routine does not establish that every accepted response, invalid identifier, short receive, allocation failure, or early return produces a new valid pointer. Native startup and connection assignments support “client's controlled creature” for these paths, not a universal promise that `+0xB8` is always nonnull, valid, or the only owned creature.

The inspected disconnect routine has no direct assignment clearing world `+0xB8`; its direct mode/socket changes are described below. This does not certify the old pointer's validity through all called cleanup operations. No nonnull assignment to the server world's `+0xB8` was established in the bounded server constructor/startup/update-caller trace. Server player admission, all server worker bodies, world destruction, bulk copies and every possible pointer alias remain unclosed.

Sources: `decompiled/client/functions/00487e90.c`, `0046fc50.c`, `004719f0.c`; `binary/Cube.exe`, initial mapping and setter caller `0x0621EF–0x06222C`, primary-input pointer use `0x09BF69–0x09BF87`, connection pointer write `0x070402–0x070408`, and the original connection region at `0x06FC50`; matching original assembly. The server limits refer to the original server sources and ranges cited above.

## Client network-mode transitions at `+0xB4`

The connection routine at client `0x06FC50` receives a destination string. An empty string returns before its mode write. With a nonempty string, it writes byte 1 to application `+0x398` before creating the socket, resolving the address, or calling the native connection API. Therefore mode 1 can describe an attempt in progress and is not proof of a successful socket or completed handshake. The observed destination is IPv4 TCP with native port constant 12345; broader connection options are outside this contract.

The routine closes the socket, clears application socket slot `+0x8006CC` to zero, and restores mode zero on its established connection-failure cleanup routes. Direct reset sites are `0x06FE47`, `0x06FEE9`, `0x07018C`, `0x070745`, and `0x070931`. These include the native failed-connect and protocol/error/rejection cleanup regions. The full classification of every receive result, early return, handshake value and exception is not closed here; the presence of these resets must not be expanded into a claim that every failure or return resets the mode.

The accepted connection branch leaves the earlier mode-1 value in place. It eventually marks application byte `+0x800585` active and starts its two network workers. That application byte is separate from world `+0xB4`. The mode byte is not set only when `+0x800585` is marked active.

The transmit-worker routine at `0x069C10` has a cleanup tail guarded by nonzero application `+0x800585`. In that tail it closes the application socket, writes socket zero, then writes mode zero at `0x06A869`. This can precede the main application's full disconnect handling. A main-update caller checks for active `+0x800585` with socket zero and invokes the disconnect routine.

The disconnect routine at `0x0719F0` does nothing through its main cleanup body when application `+0x800585` is zero. With it nonzero, the routine clears that active byte. If the socket slot is nonzero it closes the socket, clears the slot, and resets mode zero at `0x071A5B`. It then waits for the recorded network workers, enters the world critical section, and writes mode zero again at `0x071AAD` regardless of the earlier socket test. The critical-section wrappers at `0x201CB0` and `0x201E90` operate on world offset `+0x8000C0`; the wrappers themselves do not assign the mode or controlled-creature pointer.

These native paths justify describing `+0xB4` as a client network-mode byte in this contract. They do not justify equating nonzero with “server,” equating zero with “single player only,” or assuming it is synchronized with socket validity, worker state, or connection success at every intermediate point. No in-place transition of the server world's mode to 1 is established by the audited server path.

| Verified path or state boundary | Established mode/pointer relationship |
| --- | --- |
| Initial client world construction | Mode copied from zero argument; pointer initially null. |
| Client controlled-creature installation during construction | Mode remains zero in the inspected path; pointer receives application `+0x8006D0`. |
| Outbound connection attempt with nonempty destination | Mode is set to 1 before socket creation; the previous pointer has not yet been replaced by this write. |
| Verified received-creature replacement branch | Mode has been set to 1; pointer is assigned the current application controlled-creature pointer. |
| Verified connection-failure cleanup | Mode reset to zero at the listed native sites; these mode writes do not themselves clear the pointer. |
| Guarded worker/disconnect cleanup | Mode reset to zero under the described guards; pointer lifetime through every dependency is unresolved. |
| Native server startup | Mode argument zero; pointer null from construction. No later nonnull pointer assignment was established in the bounded startup trace. |

Sources: `decompiled/client/functions/0046fc50.c`, `00469c10.c`, `004719f0.c`, `00601cb0.c`, `00601e90.c`; `binary/Cube.exe`, connection admission/mode write `0x06FC97–0x06FCB3`, five direct reset sites above, worker cleanup `0x06A843–0x06A869`, main-update disconnect trigger `0x088F2A–0x088F45`, disconnect body `0x0719F0–0x071AB4`; matching original assembly. Source identities and startup are separately anchored above. Literal displacement matches in unrelated native objects were not treated as writes to this world.

## Null hit-source admission in the first later projectile candidate path

This section concerns client call site `0x221E8C` to generic hit `0x196D30`, not just the later impact candidate scan. “Hit source” is the source pointer returned by the lookup at `0x22156C`, retained separately from an earlier cached source lookup at `0x221476`/`0x221486`. The lookups use the projectile's first eight bytes as its identifier. Their equivalence under every possible lifetime transition is not assumed.

At the first candidate admission boundary, a candidate must be nonnull. The caller rejects an ordered nonpositive binary32 candidate health at `+0x16C` before inspecting the hit source. An unordered health comparison does not take that particular rejection branch; later non-finite behavior is not established.

After those candidate checks, kind 3 with element byte 2 proceeds directly to geometric testing, bypassing the source-dependent tests even if hit source is nonnull. For every other kind/element combination, a null hit source also proceeds directly to the same geometric testing. It is not a rejection. Only a nonnull source through that latter route enables the source-self comparison, relationship predicate at `0x1988A0`, and comparison against the source-associated 64-bit identifier at source `+0x11C0`/`+0x11C4`.

All these routes still require the native geometric/visibility predicates and later timing/discriminator tests. A nonzero candidate lock at `+0x128` prevents this first attempt regardless of whether hit source is null; the nonnull case additionally performs native bookkeeping on source `+0x11B4`. Null source supplies no exception to that candidate lock.

For flag-zero kinds 0, 1, and 4, the first attempt's immediate timing selector permits progress without another source-existence condition. With a null hit source, the optional source-resource operation and critical helper are skipped, and the critical Boolean remains false. The later generic-call argument formation contains no source-null rejection. Provided the candidate/geometry/lock gates pass and element byte is not 2, **these ordinary kinds can reach `0x221E8C` with stack argument 2 equal to null**. This is positive caller admission, not a claim that the callee accepts the hit or applies damage.

Kind 3 with element other than 2 can likewise pass the source-dependent admission boundary with a null source, but still needs its clock-quotient change at the native 1000-unit boundary before attempting this call. Kind 2's candidate boundary also does not reject null, but an earlier projectile-loop gate requires a source lookup to succeed for kind 2 before this region; its later attempt additionally requires clock strictly greater than 300 and a change at a 100-unit quotient boundary. This document does not infer end-to-end kind-2 null-source reachability from the later boundary alone, or assume a source cannot change between the separate lookups.

The source-independent kind-3/element-2 bypass has two important limits. It does not bypass the initial candidate/health checks, geometry, candidate lock, or kind-3 attempt timing. Also, element 2 subsequently chooses a separate event route and never reaches this first generic hit call. Its separate event route requires the cached source to exist, a false result from predicate `0x196CA0`, and candidate control byte other than 6. Therefore the early kind-3/element-2 bypass does not establish a null-source generic hit, a guaranteed event, or a successful heal. Those later event semantics are outside this contract.

| First-path case | Established source-dependent admission and generic-call limit |
| --- | --- |
| Flag-zero kind 0/1/4, hit source null, element other than 2 | Source-dependent filters are skipped; the generic hit call is reachable subject to the remaining candidate and geometry/lock gates. |
| Kind 3, element other than 2, hit source null | Source-dependent filters are skipped; generic call also requires the kind-3 timing boundary. |
| Kind 3, element 2, hit source nonnull | Source/self/relationship/associated-identifier filters are bypassed; later element choice bypasses generic hit. |
| Kind 3, element 2, hit source null | Same early geometric admission; later element choice still bypasses generic hit. Cached-source-dependent event admission remains separate. |
| Kind 2, hit source null at this candidate boundary | This boundary admits it past source filters, but earlier source and timing gates remain dependencies; no unconditional whole-loop reachability claim is made. |

Sources: `binary/Cube.exe`, separate lookups `0x221465–0x22148B` and `0x221561–0x22157A`, candidate/source boundary `0x2215E0–0x221659`, geometry/lock tail `0x221918–0x22196C`, ordinary timing admission `0x221971–0x221A1F`, source-optional resource/critical processing `0x221A3E–0x221B82`, element decision `0x221D03–0x221E28`, null-preserving call argument formation `0x221E28–0x221E8C`, earlier kind-2 source gate `0x220AE2–0x220AF8`; matching `decompiled/client/assembly.txt` and original `decompiled/client/functions/0042f000.c`.

## Effect of the world fields at this first call boundary

The first candidate source-admission test described above has no requirement that a nonnull source equal world `+0xB8`, and it does not turn a null source into a rejection when world mode is nonzero. Source-dependent relationship filtering is a separate native predicate, whose complete gameplay rules are not reconstructed here.

The nearby owner-buff probability producer uses the earlier cached source. Its source must exist and either equal world `+0xB8`, or have nonzero creature control byte while world mode is zero. A null cached source skips that producer; it does not skip the generic hit call for an otherwise admitted non-element-2 candidate. A null hit source alone does not prove that the earlier cached lookup was null.

Inside generic hit, a nonzero world mode together with candidate equal to world `+0xB8` causes an early false return. This is a target-side callee gate after the caller has reached the call; it is not a null-source admission gate. The callee also rejects ordered nonpositive candidate health. Its source-null continuation, all other protection/damage/event effects, and ultimate hit success remain unresolved here.

The known client assignments explain why equality with `+0xB8` selects its controlled creature in these paths, while the server's constructor-null pointer supplies no comparable distinguished local creature. The field values alone do not establish an ownership model for every subsystem or imply which machine must author every combat result.

Sources: `binary/Cube.exe`, first candidate/source gate `0x22160C–0x221659`, cached-source producer gate `0x221B8A–0x221BBD`, first call `0x221E8C`, generic-hit health/mode gates `0x196D73–0x196DB6` and false return `0x197014–0x197016`; `decompiled/client/functions/00596d30.c` and corresponding original assembly. Complete network protocol validation, unknown callers, indirect/bulk field writes, client/server world lifetime, geometry, source lifetime, and complete hit resolution remain explicit dependencies.
