# Alpha floating-point environment contract

Status: FINAL / STABLE / HELD  
Scope: original Alpha client startup precision request, the mapped terrain-worker launch and entry, and relevant control operations in the bundled original CRT binary.  
Evidence date: 2026-11-07.

## Supported images and evidence boundary

The original client is binary/Cube.exe, SHA-256
84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF.
It is a 32-bit x86 PE image with preferred base 0x00400000 and entry RVA
0x28E1E0.

The additional original binary inspected is game-files/msvcr110.dll,
875472 bytes, SHA-256
B30160E759115E24425B9BCDF606EF6EBCE4657487525EDE7F1AC40B90FF7E49.
Its preferred base is 0x10000000 and entry RVA is 0x01013C.
DLL RVAs below always refer to this separately identified DLL, not to Cube.exe.
The presence of this bundled copy does not prove which physical DLL an
unobserved native process actually loaded.

All relative evidence paths are relative to the original disassembled alpha
cube world directory. Client instructions were read from
decompiled/client/assembly.txt and checked against original binary bytes.
The selected startup and worker bodies, including the terrain-worker loop,
provided 1339 instruction comparisons with zero byte mismatches.
decompiled/client/external_functions.csv and decompiled/client/symbols.csv
were used only as original import/symbol exports. DLL findings come from
static PE import/export parsing and disassembly of game-files/msvcr110.dll.
Neither original binary was executed.

No replacement engine, SDK, exporter, test, capture, probe output,
implementation history, reconstruction source or other author's combined
notes were consulted. This document does not reopen the earlier held
rock-color contract.

## Correction of the earlier import identification

The earlier rock-color precision contract's assertion that the cosine helper
was imported as MSVCR110.dll ordinal 13514 is superseded by this original-image
finding: client IAT RVA 0x2FC444 imports MSVCR110.dll!_libm_sse2_cos_precise
by name.

The original lookup value is 0x003634CA. Its high ordinal flag is clear;
it points to an import hint/name record, whose name begins two bytes later.
The earlier tooling incorrectly treated this name-record RVA as an ordinal
because of a signed comparison against the high-bit threshold. This
correction concerns the import identity; it supplies no numerical cosine
equivalence claim.

The same original PE records identify the following relevant imports:

| Client IAT RVA | Imported name | Client thunk, when used |
| --- | --- | --- |
| 0x2FC3D0 | MSVCR110.dll!_controlfp_s | 0x28E8C4 |
| 0x2FC3B0 | MSVCR110.dll!_initterm_e | 0x28E8A6 |
| 0x2FC3B4 | MSVCR110.dll!_initterm | 0x28E8AC |
| 0x2FC46C | MSVCR110.dll!_beginthread | called through IAT |
| 0x2FC470 | MSVCR110.dll!_endthread | called through IAT or wrapper |
| 0x2FC444 | MSVCR110.dll!_libm_sse2_cos_precise | 0x28D7A4 |

[Evidence: binary/Cube.exe, PE import directory and indicated IAT/lookup
records; decompiled/client/external_functions.csv, matching imported names;
decompiled/client/assembly.txt, indicated thunks.]

## What the executable explicitly requests at startup

The normal native startup path at client entry RVA 0x28E1E0 passes the
initializer table bounded by data RVAs 0x2FC8C8 and 0x2FC8DC to _initterm_e.
The table's first word is zero and its next word is the preferred-base
address of client initializer RVA 0x28E0DC. The bundled CRT's _initterm_e
visits nonzero entries in increasing pointer order and stops on a nonzero
initializer result.

That client initializer reaches a floating-point precision wrapper through
the call at RVA 0x28E17B. The wrapper is RVA 0x28E87E and invokes
_controlfp_s at call RVA 0x28E88C with a null current-state output pointer,
new-value bits 0x00010000, and mask bits 0x00030000. A nonzero return
enters its failure path rather than returning normally. This request occurs
before the startup path calls the application entry at client RVA 0x0C8AE0.

[Evidence: binary/Cube.exe, initializer table data RVAs 0x2FC8C8–0x2FC8DB;
decompiled/client/assembly.txt, client RVAs 0x28E252–0x28E265,
0x28E0DC–0x28E194, 0x28E87E–0x28E89F and 0x28E2FE–0x28E307;
game-files/msvcr110.dll, _initterm_e export RVA 0x02FA53–0x02FA74.]

Static inspection of the bundled CRT resolves the meaning of the numeric
request without relying on a replacement SDK header. Its _controlfp_s
export is DLL RVA 0x027F57. The normal null-output path delegates to
_control87 at DLL RVA 0x027DF9. That routine reads the current x87 control
word, converts its supported controls to the CRT's abstract bits, replaces
only the abstract bits selected by the mask, converts back, and loads the
resulting x87 control word if a change is needed.

In that conversion, abstract precision value 0x00010000 selects hardware
x87 precision bits 0x0200 within hardware mask 0x0300: a 53-bit
significand setting. It is not a request for 24-bit x87 arithmetic, and
it is not an instruction to give SSE operations an extended accumulator.

The request's mask does not select x87 rounding bits or exception masks.
Their supported settings are carried through from the previously read
control word. The successful request therefore establishes the 53-bit
precision field at this checkpoint, while leaving the previously active
rounding and mask choices unestablished by this call.

[Evidence: game-files/msvcr110.dll, DLL RVAs 0x027F57–0x027F8E,
0x027DF9–0x027E8E, cold update path 0x05DD2C–0x05DDD6, and abstract-to-x87
conversion 0x0A6F4D–0x0A6FE3, particularly 0x0A6FBB–0x0A6FD3.]

The CRT processes SSE controls separately. Its SSE-update mask is restricted
to abstract bits 0x0308031F, which exclude the requested precision mask
0x00030000. For this request, the SSE branch therefore does not select a
new MXCSR rounding mode, exception mask, flush-to-zero setting or
denormals-are-zero setting. It reads the current supported SSE controls
and has no selected change to apply.

Thus neither MXCSR 0x00001F80 nor a complete x87 control word such as
0x027F is proved to be the ambient state merely by finding this startup
call. The precision field is established; the complete words are not.

[Evidence: game-files/msvcr110.dll, DLL RVAs 0x027E8E–0x027F37 and
the update path beginning 0x05DE1F. The read occurs at 0x027EA1;
the SSE mask restriction is at 0x027E9B.]

The executable's PE TLS-directory RVA is zero. It does not declare its
own PE TLS callback table. This does not remove the separate process
and thread callbacks of loaded DLLs or other indirect initialization.
[Evidence: binary/Cube.exe, optional-header TLS data-directory entry.]

## Terrain-worker launch and entry

One original worker route is tied directly to terrain generation by its
native call to client RVA 0x1E4850. Its creation is distinguishable from
the adjacent workers through the following original addresses:

| Boundary | Established original evidence |
| --- | --- |
| Worker object construction | Call at client RVA 0x0635D9 uses constructor RVA 0x051100. |
| Worker start request | Call at client RVA 0x0635EC uses launcher RVA 0x050E70. The returned handle is stored at owner offset 0x8005B4. |
| CRT launcher | Client RVA 0x050EE2 calls _beginthread with callback RVA 0x050E00, zero requested stack size, and the constructed object as its argument. |
| Application callback | Client RVA 0x050E00 dispatches through virtual slot +8 of the held object. |
| Terrain dispatch | The relevant vtable is data RVA 0x3029C4; its +8 entry is client RVA 0x06DBA0. |
| Terrain worker body | Client RVA 0x06DBA0 adjusts the object reference and transfers to client RVA 0x06A8A0. |
| Generation call | The worker body calls client RVA 0x1E4850 at client RVA 0x06ACD1. |

[Evidence: decompiled/client/assembly.txt, client RVAs 0x0635C0–0x0635FA,
0x051100–0x051162, 0x050E70–0x050EFA, 0x050E00–0x050E52,
0x06DBA0–0x06DBA3, 0x06A8A0–0x06B73B, and 0x06ACC1–0x06ACDE;
binary/Cube.exe, data RVA 0x3029C4 and following vtable entries.]

The mapped client callback and terrain-worker entry do not invoke the
startup precision wrapper or issue an explicit floating-point-control
load. They do not save the creator's hardware control words into the
thread argument. The launch call alone is therefore not evidence that
the worker executes the executable's earlier precision request.

In the bundled CRT, _beginthread is exported at DLL RVA 0x0775C8.
It records the application callback and argument at offsets 0x54 and
0x58 of CRT thread state, then calls KERNEL32.dll!CreateThread through
DLL IAT RVA 0x0CD05C. It supplies CRT thread-start RVA 0x0776EF
and creates the thread suspended, then resumes it through
KERNEL32.dll!ResumeThread at DLL IAT RVA 0x0CD064.

The CRT thread start associates or reuses its per-thread CRT state and
reaches a callback wrapper at DLL RVA 0x077667. That wrapper obtains
the stored callback and invokes it at DLL RVA 0x07767F. There is no
explicit _controlfp_s, _fpreset, FNINIT or MXCSR-setting operation
in these immediate launch/start/callback bodies.

[Evidence: game-files/msvcr110.dll, exports and DLL RVAs
0x0775C8–0x077665, 0x0776EF–0x077745 and 0x077667–0x077683;
PE import records for DLL IAT RVAs 0x0CD05C and 0x0CD064.]

The inspected CRT DLL entry also has a distinct thread-attach branch:
DLL entry RVA 0x01013C reaches dispatcher RVA 0x010095; reason value
2 selects the branch at DLL RVA 0x0100B5. It obtains or allocates
per-thread CRT data and initializes that data through DLL RVA 0x010476.
The branch contains no explicit floating-point-control reset, and its
common final hook at DLL RVA 0x010090 is a return-only stub.

This establishes what those original boundary bodies do. It does not
close the behavior of Windows thread initialization, every helper or
imported module callback, or all later work done on the worker.

[Evidence: game-files/msvcr110.dll, DLL RVAs 0x01013C–0x01015B,
0x010095–0x01011F, 0x010476–0x010510 and 0x010090–0x010094.]

## Existing reset and temporary-control operations

The bundled CRT does provide _fpreset at DLL RVA 0x0A6F17. Its body
performs FNINIT, calls a precision helper that makes the same 53-bit
request, and requests MXCSR 0x00001F80 through a capability-gated
setter. If this reset executes successfully on the supported SSE path,
its normalized x87 control word is 0x027F: round to nearest, 53-bit
precision and all x87 exceptions masked. MXCSR 0x00001F80 selects
round to nearest, all SSE exceptions masked, and neither FTZ nor DAZ.
Its status-reset behavior is distinct from merely changing precision.

This is a definition of an available reset operation. No invocation of
that reset has been established in the mapped terrain-worker entry
route, so these words are not accepted as observed worker defaults.
The MXCSR setter also accounts for SSE capability and DAZ support;
the existence of the literal does not bypass those conditions.

[Evidence: game-files/msvcr110.dll, DLL RVAs 0x0A6F17–0x0A6F4C,
precision helper 0x077E12–0x077E38, and MXCSR setter
0x0BD19B–0x0BD210.]

A search of the original client's exported instruction listing found no
LDMXCSR, STMXCSR or FNINIT instruction. It did find local x87
control-word changes in four original bodies:

- Client RVA 0x1654F0 has save/change/restore sequences around integer
  conversions, with changes at 0x1657E2 and 0x1658E4 and corresponding
  restores at 0x1657FB and 0x1658FD. The change selects truncation by
  setting hardware rounding bits 0x0C00.
- Client RVA 0x1FC0D0 similarly saves the old word, selects truncation
  at 0x1FC11B and restores the saved word at 0x1FC128.
- Client RVA 0x2E0BD0 can temporarily select extended x87 precision
  and nearest rounding, also uses an inner truncating conversion, and
  restores the saved outer word at 0x2E0E8C when it changed it.
- Client RVA 0x2E0EC0 has the corresponding temporary and restoring
  operations, with its outer restore at 0x2E11A1.

These ordinary-return restoration sequences are not a persistent
worker-entry setup. Their existence also prevents treating one x87
precision setting as an invariant for every internal native routine.
The listing search is bounded by decoded listing coverage; it is not
an exhaustive claim about all possible dynamically reached code or
exceptional exits.

[Evidence: decompiled/client/assembly.txt, client RVAs
0x1657C4–0x1657FB, 0x1658C6–0x1658FD, 0x1FC0DA–0x1FC128,
0x2E0BF6–0x2E0C60, 0x2E0E81–0x2E0E90,
0x2E0EE6–0x2E0F50 and 0x2E1196–0x2E11A5.]

## Established facts and held uncertainties

| Control | Established at the successful executable startup precision call | Established for the mapped terrain worker |
| --- | --- | --- |
| x87 precision field | Hardware field 0x0200, meaning 53-bit significand precision | No explicit replay or reset established at worker entry |
| x87 rounding | Preserved from preceding state | Exact ambient mode remains unresolved |
| x87 exception masks | Supported mask choices preserved from preceding state | Exact ambient masks remain unresolved |
| MXCSR rounding | No selected change from the precision-only request | Exact ambient mode remains unresolved |
| MXCSR exception masks | No selected change from that request | Exact ambient masks remain unresolved |
| FTZ and DAZ | No selected change from that request | Exact ambient bits remain unresolved |
| Complete control words | Not established by the precision-only call | Not established by the mapped launch route |

This audit establishes a real startup precision request and an original
terrain-worker boundary. It does not establish that child threads inherit
or independently receive any particular creator state. Windows thread
initialization and other loaded modules are outside the proved boundary.
Nor does the static bundled-DLL analysis establish an actual process's
DLL search resolution, processor capability result, later control changes,
or exception-path behavior.

Consequently round-to-nearest, ties-to-even, all exceptions masked,
FTZ/DAZ disabled, and matching x87/SSE rounding directions remain
environmental assumptions when used to predict terrain results; they are
not promoted here to verified universal native defaults. The confirmed
53-bit startup field does not change the instruction-defined binary32
or binary64 widths of SSE terrain arithmetic. An x87 binary32 store
still has its own store-width and rounding boundary.

The earlier held rock-color file remains unchanged. This standalone
contract supersedes its stale import-ordinal identification and adds
the source-supported startup and worker facts above. It does not
complete native numerical or visual acceptance.

This document is held after complete readback. Further source research
requires a separately assigned scope.
