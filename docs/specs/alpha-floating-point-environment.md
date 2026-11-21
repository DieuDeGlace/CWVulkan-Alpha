# Alpha floating-point environment contract

**FINAL / STABLE / HELD**

Status: FINAL / STABLE / HELD (re-certified 2026-11-20).

Scope: original Alpha client and dedicated-server startup precision requests, the mapped worker launch and entry routes, the floating-point control operations present in both executables, and the relevant control operations in the bundled original CRT binary.

Evidence dates: 2026-11-07 (original audit); 2026-11-20 (re-certification).

## Supported images and evidence boundary

| Image | Size (bytes) | SHA-256 | Preferred base | Entry RVA |
| --- | --- | --- | --- | --- |
| binary/Cube.exe (identical to game-files/Cube.exe) | 3885568 | 84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF | 0x00400000 | 0x28E1E0 |
| binary/Server.exe (identical to game-files/Server.exe) | 1718784 | 1FFBCBCE251E1506A63C9CE18AB0009D5553FCBB9E6202BE2BC3A7D870D34FB1 | 0x00400000 | 0x14B44B |
| game-files/msvcr110.dll | 875472 | B30160E759115E24425B9BCDF606EF6EBCE4657487525EDE7F1AC40B90FF7E49 | 0x10000000 | 0x01013C |

All three are 32-bit x86 PE images. "Client RVA" refers to Cube.exe,
"server RVA" to Server.exe and "DLL RVA" to the separately identified
msvcr110.dll. The presence of this bundled DLL does not prove which
physical DLL an unobserved native process actually loaded.

All relative evidence paths are relative to the original disassembled alpha
cube world directory. Client and server instructions were read from
decompiled/client/assembly.txt and decompiled/server/assembly.txt and
checked against original image bytes: the cited client ranges gave 2253
instruction comparisons and the cited server ranges 560, all with zero byte
mismatches. Because those listings omit some code, every executable and the
DLL were additionally disassembled by linear sweep from their original
bytes; addresses found only that way are identified below.
decompiled/client/external_functions.csv, symbols.csv, function_ranges.csv
and calls.csv were used only as original import, symbol, range and
direct-call exports. PE import, export, TLS and initializer-table facts come
from direct parsing of the original images. No original binary was executed.

No replacement engine, SDK, exporter, test, capture, probe output,
implementation history or reconstruction source was consulted.

## Import identification

Client IAT RVA 0x2FC444 imports MSVCR110.dll!_libm_sse2_cos_precise by
name. Its lookup value 0x003634CA has the high ordinal flag clear and points
to a hint/name record (hint 0x36D) whose name begins two bytes later. An
earlier identification of this import as ordinal 13514 resulted from
treating the name-record RVA as an ordinal; 13514 is 0x34CA, the low half of
that RVA. This concerns import identity only and makes no numerical cosine
equivalence claim.

| Image | IAT RVA | Imported name | Thunk, when used |
| --- | --- | --- | --- |
| Client | 0x2FC3D0 | MSVCR110.dll!_controlfp_s | 0x28E8C4 |
| Client | 0x2FC3B0 | MSVCR110.dll!_initterm_e | 0x28E8A6 |
| Client | 0x2FC3B4 | MSVCR110.dll!_initterm | 0x28E8AC |
| Client | 0x2FC3CC | MSVCR110.dll!_invoke_watson | 0x28E8BE |
| Client | 0x2FC46C | MSVCR110.dll!_beginthread | called through IAT at 0x050EE2 |
| Client | 0x2FC470 | MSVCR110.dll!_endthread | tail jump through IAT at 0x050E5C |
| Client | 0x2FC444 | MSVCR110.dll!_libm_sse2_cos_precise | 0x28D7A4 |
| Client | 0x2FC0C8 | KERNEL32.dll!IsProcessorFeaturePresent | 0x2CF8C2 |
| Client | 0x2FC604 | d3d9.dll!Direct3DCreate9 | 0x227216 |
| Server | 0x1583B0 | MSVCR110.dll!_controlfp_s | 0x14B910 |
| Server | 0x1582B0 | MSVCR110.dll!_initterm_e | 0x14B8F2 |
| Server | 0x1582AC | MSVCR110.dll!_initterm | 0x14B8F8 |
| Server | 0x158294 | MSVCR110.dll!_invoke_watson | 0x14B90A |
| Server | 0x158300 | MSVCR110.dll!_beginthread | called through IAT at 0x0136B2 |
| Server | 0x158304 | MSVCR110.dll!_endthread | tail jump through IAT at 0x01362C |
| Server | 0x1582E4 | MSVCR110.dll!_libm_sse2_cos_precise | — |
| Server | 0x158004 | KERNEL32.dll!IsProcessorFeaturePresent | 0x14B916 |

Neither executable imports _fpreset, _control87, _controlfp,
__control87_2, _clearfp or _statusfp. The client additionally imports the
_libm_sse2_*_precise forms of acos, asin, atan, exp, log, pow, sin, sqrt and
tan; the server imports those of acos, asin, exp, pow, sin and sqrt.
Neither declares a PE TLS directory (both TLS data-directory RVAs are zero),
so neither has its own PE TLS callback table; loaded DLLs keep their own
process and thread callbacks.

[Evidence: binary/Cube.exe and binary/Server.exe, PE import directories,
lookup and IAT records, optional-header TLS entries; listed thunks in the
respective assembly listings.]

## What the client explicitly requests at startup

The client entry RVA 0x28E1E0 passes the C initializer table bounded by data
RVAs 0x2FC8C8 and 0x2FC8DC to _initterm_e (call 0x28E25C). The table holds
five words: zero, then the preferred-base addresses of client initializers
0x28E0DC, 0x28D674, 0x28E4B8 and 0x28E645. The bundled CRT's _initterm_e
(DLL RVA 0x02FA53) visits nonzero entries in increasing address order and
stops on the first nonzero result. The C++ initializer table at data RVAs
0x2FC62C–0x2FC8C4 (165 nonzero entries) is passed to _initterm afterwards
(call 0x28E290). Application entry RVA 0x0C8AE0 is called at 0x28E307.

The first nonzero initializer, 0x28E0DC, unconditionally calls the precision
wrapper RVA 0x28E87E at 0x28E17B. The wrapper calls _controlfp_s at
0x28E88C with a null current-state output pointer, new value 0x00010000 and
mask 0x00030000. A nonzero return would enter _invoke_watson through
0x28E89F. In the bundled CRT this request cannot return nonzero: _controlfp_s
removes abstract bit 0x00080000 from the mask, accepts the request because
the selected new bits contain none of the invalid bits 0xFCF0FCE0, and the
null-output path returns zero after delegating to _control87.

Thus the precision request precedes the remaining C initializers, every C++
static initializer and the application entry, on the primary thread.

The later initializer 0x28E4B8 is the client's own processor-capability
initializer. It sets client data RVA 0x36E2A0 to 0 when
IsProcessorFeaturePresent(10) is false, otherwise to 1, raising it to 2 or 3
from CPUID leaf 1 ECX bits 20 and 28 (0x28E4BB–0x28E531). It changes no
floating-point control word.

[Evidence: binary/Cube.exe initializer tables at data RVAs 0x2FC8C8–0x2FC8DB
and 0x2FC62C–0x2FC8C3; decompiled/client/assembly.txt, client RVAs
0x28E1E0–0x28E31F, 0x28E0DC–0x28E194, 0x28E87E–0x28E89F,
0x28E4B8–0x28E541; game-files/msvcr110.dll, DLL RVAs 0x02FA53–0x02FA74,
0x027F57–0x027F8E and 0x05DCB9–0x05DCE7.]

## What the dedicated server explicitly requests at startup

The server follows the same structure. Entry RVA 0x14B44B reaches
0x14B2E6, which passes the C initializer table bounded by data RVAs
0x15854C and 0x158560 to _initterm_e (call 0x14B34B). Its five words are
zero, then server initializers 0x14B1E2, 0x14A7C2, 0x14B4EC and 0x14B685.
The C++ table 0x158408–0x158548 is passed to _initterm at 0x14B37F, and the
console main entry RVA 0x149C50 is called at 0x14B3DE.

The first nonzero initializer, 0x14B1E2, unconditionally calls server
precision wrapper RVA 0x14B8CB at 0x14B281. That wrapper calls _controlfp_s
at 0x14B8D9 with the same null output, new value 0x00010000 and mask
0x00030000, entering _invoke_watson through 0x14B8EC only on a nonzero
return, which the bundled CRT does not produce for these arguments.
Server initializer 0x14B4EC is the server's capability initializer and
assigns server data RVA 0x184640 by the same rule as the client flag.

[Evidence: binary/Server.exe initializer tables at data RVAs
0x15854C–0x15855F and 0x158408–0x158547; decompiled/server/assembly.txt,
server RVAs 0x14B1E2–0x14B29A, 0x14B2E6–0x14B3E3, 0x14B8CB–0x14B8F1,
0x14B4EC–0x14B575.]

## Meaning of the precision request in the bundled CRT

_control87 (DLL RVA 0x027DF9) reads the current x87 control word at
0x027E00 and converts its supported fields to CRT abstract bits: the six
exception masks (including the denormal mask as abstract 0x00080000),
rounding control, precision control and the infinity-control bit 0x1000. It
replaces only the abstract bits selected by the mask. Only if the abstract
value changed does it compose a new hardware word (DLL RVA 0x0A6F4D) and
load it at 0x05DD39, then re-read it.

In that composition, abstract precision 0x00010000 becomes hardware
precision bits 0x0200 within field 0x0300: a 53-bit significand setting. It
is not a request for 24-bit x87 arithmetic, and it does not give SSE
operations an extended accumulator.

The mask selects neither x87 rounding nor exception masks; those supported
fields are carried from the word that was read. The loaded word is,
however, recomposed only from the supported fields: reserved bit 6 and bits
13–15 are not composed. For example, if the word read is 0x037F, the word
the CRT composes and loads is 0x023F. The request therefore establishes the
53-bit precision field while leaving the preceding rounding and mask choices
unestablished by this call.

The SSE branch of _control87 runs only when the CRT's capability flag at DLL
RVA 0x0C87CC is at least 1 (test at 0x027E8E). That flag is set by the CRT's
own capability initializer (DLL RVA 0x027934) by the same SSE2 / SSE4.2 /
AVX rule as the executables' flags. In the SSE branch the mask is limited to
abstract bits 0x0308031F (0x027E9B), which exclude the precision mask
0x00030000. MXCSR is read at 0x027EA1, but for this request no new MXCSR
rounding mode, exception mask, flush-to-zero setting or denormals-are-zero
setting is selected or applied.

Thus neither MXCSR 0x00001F80 nor a complete x87 control word such as
0x027F is proved to be the ambient state by finding these startup calls.
The precision field is established; the complete words are not.

[Evidence: game-files/msvcr110.dll, DLL RVAs 0x027DF9–0x027F55,
0x05DCEC–0x05DDD6, 0x05DDDB–0x05DEE9, 0x0A6F4D–0x0A6FE3 (precision at
0x0A6FBB–0x0A6FD3), 0x027934–0x0279A0 with 0x05063E–0x050669.]

## CRT DLL process and thread initialization

On process attach, CRT DLL entry 0x01013C reaches the CRT's own startup,
which calls its C-runtime initializer 0x0278AA with argument zero (call at
0x02730B). With zero, the helper at 0x027899 skips the precision request and
executes only FNCLEX at 0x0278A6, clearing pending x87 exception flags. It
then runs CRT-internal initializer tables (0x027918–0x02792F, whose first
nonzero entry is the capability initializer 0x027934, and
0x0278F0–0x027913). No control-word or MXCSR load occurs in that path.

On thread attach (reason 2), dispatcher 0x010095 takes the branch at
0x0100B5, obtains or allocates a 0x3BC-byte per-thread CRT block, and
initializes it through 0x010476. That branch contains no floating-point
control operation, and its common final hook 0x010090 is a return-only stub.

[Evidence: game-files/msvcr110.dll, DLL RVAs 0x01013C–0x01015B,
0x010095–0x01011F, 0x010476–0x010510, 0x010090–0x010094, 0x0272C5–0x02731F,
0x027899–0x0278EC and its initializer tables.]

## Worker launch and entry

The client has exactly one _beginthread call site and the server exactly one;
every worker of each executable is started through its single launcher.

### Client terrain worker

| Boundary | Established original evidence |
| --- | --- |
| Worker object construction | Call at client RVA 0x0635D9 to constructor 0x051100, which stores vtable data RVA 0x3029C4, copies one pointer from its argument to +4 and stores the object's own address at +0x10. |
| Worker start request | Call at 0x0635EC to launcher 0x050E70. The returned thread handle is stored at owner offset 0x8005B4. |
| Launcher | Allocates a new 0x18-byte holder, clones the held callable into it through slot 0 of the callable's vtable (in place when the callable is held inline), and at 0x050EE2 calls _beginthread with callback 0x050E00, zero requested stack size and the new holder as argument. |
| Application callback | 0x050E00 dispatches through vtable slot +8 of the callable at holder +0x10, then destroys it through slot +0x10, frees the holder and tail-jumps to _endthread at 0x050E5C. |
| Terrain dispatch | Vtable 0x3029C4, slot +8 (data RVA 0x3029CC), is client RVA 0x06DBA0. |
| Terrain worker body | 0x06DBA0 adjusts the object reference by 4 and transfers to 0x06A8A0. |
| Generation call | The worker body calls 0x1E4850 at 0x06ACD1. |

The launcher, callback and terrain-worker entry do not invoke a precision
wrapper or issue a floating-point-control load, and they do not save the
creator's control words into the thread argument. The launch is therefore
not evidence that the worker inherits the startup precision request.

### Server workers

Server launcher 0x013640 (called at 0x025E9B, 0x025F94, 0x0288CE, 0x149F9D
and 0x14A01C) has the same holder-and-clone form and calls _beginthread at
0x0136B2 with callback 0x0135D0, zero stack size and the new holder.
Callback 0x0135D0 dispatches through slot +8 of the callable at holder +0x10,
destroys it, frees the holder and tail-jumps to _endthread at 0x01362C.
Neither body contains a floating-point control operation. The individual
server worker bodies were not mapped in this audit.

### Bundled CRT thread path

_beginthread (DLL RVA 0x0775C8) allocates a 0x3BC-byte per-thread block,
initializes it through 0x010476 with the creating thread's locale reference,
stores the callback at +0x54 and argument at +0x58, and calls
KERNEL32.dll!CreateThread through DLL IAT 0x0CD05C with start routine
0x0776EF and the suspended flag, then resumes the thread through
ResumeThread at DLL IAT 0x0CD064. The start routine either installs that
block or copies its callback, argument and handle into an already-present
per-thread block, then reaches wrapper 0x077667, which invokes the callback
at 0x07767F and afterwards calls _endthread (0x0776C4). No _controlfp_s,
_fpreset, FNINIT, FLDCW or LDMXCSR occurs in these bodies.

This establishes what the inspected boundary bodies do. It does not close
Windows thread initialization, other loaded modules' callbacks, or later
work on the worker.

[Evidence: decompiled/client/assembly.txt, client RVAs 0x0635C0–0x0635FA,
0x051100–0x051162, 0x050E70–0x050EFA, 0x050E00–0x050E5C,
0x06DBA0–0x06DBA3, 0x06ACC1–0x06ACDE; binary/Cube.exe vtable words at
0x3029C4–0x3029D7; server RVAs 0x0135D0–0x0136CA; game-files/msvcr110.dll,
DLL RVAs 0x0775C8–0x077665, 0x077667–0x077683 and 0x0776EF–0x077745; DLL
import records for IAT RVAs 0x0CD05C and 0x0CD064.]

## Direct3D 9 device creation on the client primary thread

The application entry 0x0C8AE0 calls 0x0C8720 at 0x0C8BDB. That routine
obtains an IDirect3D9 object through Direct3DCreate9 (call 0x0C874A) and at
0x0C87D0 calls its CreateDevice slot (+0x40) with adapter 0, device type 1,
the stored focus window and behavior flags 0x44. Device Reset (device slot
+0x40) is called at 0x0C8A58 and 0x0C8A75 during device-loss recovery. The
behavior-flag value does not include the floating-point-preserve bit 0x02.

The control words written by the external d3d9.dll module are outside this
evidence set. Under that module's published contract, creating or resetting
a device without the preserve flag may set the calling thread's x87 control
to single precision. The client's 53-bit startup field is therefore not
established as the primary thread's precision after 0x0C87D0. Worker
threads are separate threads and are not covered by this observation.

[Evidence: client RVAs 0x0C8720–0x0C88EF, 0x0C8A42–0x0C8A75, 0x0C8BDB;
IAT 0x2FC604.]

## Existing reset and conditional operations in the bundled CRT

The CRT exports _fpreset at DLL RVA 0x0A6F17. Neither executable imports
it, and the DLL itself contains no direct call or jump to it. Its body
performs FNINIT, calls the CRT precision helper 0x077E12 (the same
null-output 53-bit _controlfp_s request, with _invoke_watson on failure),
and requests MXCSR 0x00001F80 through setter 0x0BD19B. It also clears two
per-thread fields when a per-thread flag selects them.

After FNINIT the hardware word is 0x037F. The precision helper composes and
loads 0x023F. If the processor reports reserved bit 6 as set, as the CRT's
own mask tests assume, the observed word is 0x027F: round to nearest, 53-bit
precision and all x87 exceptions masked. The setter loads MXCSR only when
the CRT capability flag is at least 1. It removes the denormals-are-zero bit
unless DAZ support has been recorded; that bit is not present in 0x1F80.
MXCSR 0x00001F80 selects round to nearest, all SSE exceptions masked, and
neither FTZ nor DAZ. No invocation of _fpreset is established on any mapped
route, so these words are not accepted as observed defaults.

Every _libm_sse2_*_precise export imported by the client or server first
tests the ambient state. Its SSE2 computation is selected only when MXCSR
bits 0x7F80 equal 0x1F80 (all SSE exception masks set and round to nearest;
FTZ and DAZ are not tested) and x87 control bits 0x7F equal 0x7F (all x87
exception masks plus reserved bit 6). Otherwise it uses an x87 fallback.
For _libm_sse2_cos_precise, arguments whose binary64 high 16-bit magnitude
exceeds 0x40F5 (magnitude at least 90112), and non-finite arguments, also
take the fallback. That fallback loads x87 control word 0x027F from DLL data
RVA 0x060344 before FCOS unless the current word already equals 0x027F.

[Evidence: game-files/msvcr110.dll, DLL RVAs 0x0A6F17–0x0A6F4C,
0x077E12–0x077E38, 0x0BD19B–0x0BD210; gate prologues at 0x036F80
(cos), 0x03DCF0 (sin), 0x039840 (pow), 0x03E748 (sqrt), 0x036B28 (atan),
0x0379C0 (exp), 0x038CD0 (log), 0x033155 (acos), 0x034E40 (asin), 0x03E846
(tan); cos range test 0x036FB1–0x0370FE, fallback 0x03711F–0x037137 and
0x032775–0x032793; data word at 0x060344.]

## Control operations and x87/SSE use in the executables

Neither executable contains LDMXCSR, STMXCSR, FNINIT, FINIT, FNCLEX,
FLDENV, FRSTOR or FXRSTOR, by either the exported listing or linear-sweep
disassembly. A linear-sweep decode at client 0x182F10 and 0x2CDEFD and at
server 0x09C244 lies inside data or misaligned padding and is not an
instruction. The genuine x87 control-word operations are:

| Image | Body | Operations |
| --- | --- | --- |
| Client | 0x1654F0 | Two save/select-truncation/restore sequences around 64-bit integer stores: changes at 0x1657E2 and 0x1658E4, restores at 0x1657FB and 0x1658FD. Truncation is selected by setting rounding bits 0x0C00; precision and masks are kept. |
| Client | 0x1FC0D0 | Saves at 0x1FC0DA, selects truncation at 0x1FC11B, restores at 0x1FC128. |
| Client | 0x2E0BD0 | Unless the saved word already has extended precision and nearest rounding (bits 0x0F00 equal 0x0300), loads it with those fields at 0x2E0C1E; an inner truncating conversion at 0x2E0C58/0x2E0C60; outer restore at 0x2E0E8C when it changed the word. |
| Client | 0x2E0EC0 | Corresponding operations: 0x2E0F0E, 0x2E0F48/0x2E0F50, outer restore 0x2E11A1. |
| Client (listing omits) | 0x2E0880 | Corresponding operations: 0x2E08CE, 0x2E0908/0x2E0910, outer restore 0x2E0B43. |
| Client (listing omits) | 0x28D92C | Read only at 0x28D938: tests x87 bits 0x7F before choosing a conversion path. |
| Server | 0x012D00 | Saves at 0x012D0A, selects truncation at 0x012D4B, restores at 0x012D58. |
| Server | 0x0A7E30 | Saves at 0x0A8104 and 0x0A8206, selects truncation at 0x0A8122 and 0x0A8224, restores at 0x0A813B and 0x0A823D. |
| Server | 0x14A92C | Read only at 0x14A938, as client 0x28D92C. |

Reachability by direct calls and jumps: client 0x2E0BD0 is called by both
branches of a packed binary64 sine/cosine routine at 0x2E0390, which selects
its branch from the client capability flag (at least 2 or not). That routine
is reached from 0x2E0150 and then 0x2DFBA0. Client 0x2E0880, 0x2E0EC0 and
0x28D92C, and server 0x14A92C, have no direct call, jump or absolute-pointer
reference in their images.

The client float-to-integer helper 0x28D910, and the server one 0x14A910
(called once, at 0x10EAAC), choose their path from the executable's
capability flag. With a nonzero flag the x87 value is stored as binary64 and
truncated with CVTTSD2SI. With a zero flag, an x87 integer-store path is used
whose correction assumes the ambient x87 rounding is nearest.

Within the client direct-call graph, terrain generation 0x1E4850 reaches
0x28D910 (through 0x1DA280) and 0x1654F0, and the terrain-worker body
0x06A8A0 also reaches 0x1FC0D0. No direct-call path from 0x1E4850 reaches
0x2E0880, 0x2E0BD0 or 0x2E0EC0. Calls through pointers are not included.

In the listing-assigned ranges of 0x1E4850 and 0x1DA280, arithmetic is
scalar or packed SSE. The only x87 instructions are FLD, FILD and FSTP: they
carry float and double return values in ST0 and convert integers. The
integer conversions, for example 0x1DAD80/0x1DAD87 and 0x1E6DD4/0x1E6DE0,
load a 64-bit integer (a zero-extended 32-bit value or a 64-bit value) and
store it as binary64 or binary32. Such stores round under the ambient x87
rounding control, not MXCSR. They can be inexact only for magnitudes above
2^53 (binary64) or 2^24 (binary32).

These ordinary-return restoration sequences are not a persistent setup. They
also prevent treating one x87 precision or rounding setting as invariant
across all native routines. The search is bounded by static decoding; it
does not cover dynamically generated code or exceptional exits.

[Evidence: decompiled/client/assembly.txt and decompiled/server/assembly.txt
at the listed RVAs; linear-sweep disassembly of binary/Cube.exe at client
0x28D910–0x28D9BA and 0x2E0880–0x2E0B55; decompiled/client/calls.csv and
function_ranges.csv for direct-call reachability and ranges.]

## Established facts and held uncertainties

| Control | At the successful startup precision call (client and server) | For the mapped workers |
| --- | --- | --- |
| x87 precision field | Hardware field 0x0200, 53-bit significand; client primary thread may later be changed by Direct3D 9 | No explicit replay or reset at worker entry |
| x87 rounding | Preserved from preceding state | Ambient mode unresolved |
| x87 exception masks | Supported masks preserved | Ambient masks unresolved |
| MXCSR rounding, masks, FTZ, DAZ | No selected change | Ambient values unresolved |
| Complete control words | Not established; the CRT recomposes the x87 word from supported fields | Not established by the launch route |

Child-thread inheritance is not established. Windows thread initialization,
d3d9.dll and other loaded modules are outside the proved boundary, as are
actual DLL search resolution, processor-capability results, later control
changes and exception-path behavior.

Consequently, round to nearest with ties to even, all exceptions masked,
FTZ and DAZ disabled, and matching x87 and SSE rounding remain environmental
assumptions when predicting results; they are not verified universal native
defaults. Results of imported _libm_sse2_*_precise functions also depend on
the gate above. The 53-bit field does not change the binary32 or binary64
widths of SSE arithmetic. An x87 binary32 or binary64 store still has its
own width and rounding boundary.

This document does not complete native numerical or visual acceptance.

## Re-certification (2026-11-20)

Every claim of the 2026-11-07 text was re-checked against Cube.exe,
Server.exe and the bundled msvcr110.dll. Confirmed unchanged: image
identities and entry RVAs; the client import identities and thunks; the
_controlfp_s arguments and wrapper addresses; the abstract-to-hardware
precision mapping; the SSE mask 0x0308031F; the terrain-worker chain
0x0635D9 → 0x051100, 0x0635EC → 0x050E70, 0x050EE2, 0x050E00,
vtable 0x3029C4 slot +8 → 0x06DBA0 → 0x06A8A0 → call 0x1E4850 at 0x06ACD1;
the CRT _beginthread, thread-start and thread-attach bodies; the _fpreset
body; the absence of LDMXCSR/STMXCSR/FNINIT in the client; and the four
previously listed client control-word bodies.

Corrections and additions:

1. The client C initializer table at data RVAs 0x2FC8C8–0x2FC8DB holds five
   words (zero, 0x28E0DC, 0x28D674, 0x28E4B8, 0x28E645), not two meaningful
   words. The precision request is in the first nonzero entry, and 0x28E4B8
   is the client's capability initializer. [client data 0x2FC8C8; 0x28E4B8]
2. For these arguments the bundled _controlfp_s cannot return nonzero
   (invalid-bit validation 0xFCF0FCE0 and a zero-returning null-output path).
   The wrapper's _invoke_watson failure path is therefore unreachable with
   the bundled CRT. _controlfp_s also strips abstract bit 0x00080000 from
   every mask. [DLL 0x027F57–0x027F8E, 0x05DCB9; client 0x28E89A]
3. _control87 recomposes the x87 word from supported fields only, omitting
   reserved bit 6 and bits 13–15, and loads only when the abstract value
   changes. "Carried through" applies to fields, not the raw word: from
   0x037F the loaded word is 0x023F. [DLL 0x027E86–0x027E88, 0x0A6F4D–0x0A6FE3,
   0x05DD2C–0x05DD39]
4. The SSE branch and the MXCSR setter depend on the CRT capability flag at
   DLL data RVA 0x0C87CC. That flag is set by the CRT initializer 0x027934.
   [DLL 0x027E8E, 0x0BD1A7, 0x027934]
5. The _fpreset value 0x027F is the observed form only if reserved bit 6
   reads back as set; the CRT itself composes 0x023F after FNINIT. _fpreset
   is not imported by either executable and has no direct caller in the DLL.
   [DLL 0x0A6F1F–0x0A6F21; executables' import tables]
6. The client listing search was incomplete. Linear-sweep disassembly
   found a fifth x87 control-changing body at client 0x2E0880 (0x2E08CE,
   0x2E0908, 0x2E0910, 0x2E0B43) and a control-word read at 0x28D938, both
   absent from the exported listing. [client 0x2E0880–0x2E0B55, 0x28D92C]
7. Reachability: 0x2E0EC0, 0x2E0880 and 0x28D92C have no direct reference.
   0x2E0BD0 is reached from the packed sine/cosine routine 0x2E0390. The
   precision rule given for 0x2E0BD0 is now exact: extended precision and
   nearest rounding are selected only when bits 0x0F00 are not already
   0x0300. [client 0x2E0390–0x2E0397, 0x2E0504, 0x2E07EE, 0x2E0BF6–0x2E0C1E]
8. Worker launch argument corrected: _beginthread receives a newly allocated
   0x18-byte holder containing a clone of the callable, not the constructed
   object. The callback ends by destroying the clone, freeing the holder
   and tail-jumping to _endthread. Each executable has one _beginthread site.
   [client 0x050E94–0x050EE2, 0x050E19–0x050E5C]
9. CRT thread path detail: the callback and argument are stored in a newly
   allocated 0x3BC-byte block. That block is installed, or copied into an
   existing per-thread block, by the start routine. [DLL 0x0775EC–0x077623,
   0x0776F2–0x07773B]
10. Added the CRT process-attach behavior: _cinit(0) executes FNCLEX only,
    with no precision request, MXCSR or control-word load.
    [DLL 0x02730B, 0x027899–0x0278A9]
11. Added the dedicated server: the same startup precision request
    (0x14B1E2 → 0x14B8CB → _controlfp_s at 0x14B8D9), its initializer tables,
    capability initializer, single worker launcher 0x013640 / callback
    0x0135D0, and control-word bodies 0x012D00, 0x0A7E30 and 0x14A92C.
    [server RVAs listed above]
12. Added the client Direct3D 9 device creation at 0x0C87D0 and Reset calls
    at 0x0C8A58 and 0x0C8A75 with behavior flags 0x44, which omit the
    floating-point-preserve bit. The 53-bit startup field is no longer
    presented as the primary thread's established precision after device
    creation. [client 0x0C8796–0x0C87D0, 0x0C8BDB]
13. Added the environment gate of all imported _libm_sse2_*_precise
    functions (MXCSR bits 0x7F80 = 0x1F80 and x87 bits 0x7F = 0x7F,
    otherwise x87 fallback), the cosine argument-range fallback and the
    fallback's 0x027F load. [DLL 0x036F80–0x036FAB, 0x0370FE–0x037137,
    0x032780–0x03278D, data 0x060344]
14. Added the float-to-integer helpers 0x28D910 (client) and 0x14A910
    (server), with their capability-flag path choice. Added the
    direct-call reachability of 0x28D910, 0x1654F0 and 0x1FC0D0 from the
    terrain route. Replaced the unsupported "SSE terrain arithmetic"
    generalization with the observed instruction mix and the x87
    integer-conversion store boundary. [client 0x28D910–0x28D9BA,
    0x1DACAC, 0x1DAD80, 0x1E6DD4; calls.csv]
15. Replaced the earlier figure of 1339 instruction comparisons with this
    re-certification's 2253 client and 560 server comparisons (zero
    mismatches), plus linear-sweep cross-checks.
16. Removed the statement that the earlier rock-color contract "remains
    unchanged" with a stale ordinal: that held contract
    (alpha-rock-color-precision.md) now itself records the import by name.

Remaining uncertainties:

- The ambient x87 and MXCSR words at process start, on new threads and after
  d3d9.dll device creation or reset are external to these images.
- Whether the processor returns reserved x87 bit 6 as set after the CRT loads
  a composed word is a processor property. The CRT's own 0x7F tests assume it.
- The CRT and executable capability flags depend on the processor and
  operating system that actually run the program.
- Direct-call reachability excludes calls through pointers and virtual slots.
  Whether 0x2E0880, 0x2E0EC0 or 0x2DFBA0 run during terrain work is open.
- The server worker bodies were not mapped to particular subsystems.
- The x87 fallback of the imported mathematical functions is not specified
  numerically here.

Cross-spec notes: alpha-rock-color-precision.md now agrees on the import by
name and refers to held cosine contracts for the cosine gate and fallback.
Those contracts and alpha-generation-worker-floating-point-controls.md are
not cited by this document and were not read in this re-certification, so
any difference from the gate description above is unassessed.

This document is held after complete readback. Further source research
requires a separately assigned scope.
