# Alpha client startup and initial window boundary

Status: FINAL / STABLE / HELD. This contract covers the supported original client's executable dependencies, entry-to-window ordering, direct early exits and selected blocking boundaries. Creation of the native game window and the later request to show it are distinct milestones. Broader game construction, complete graphics initialization and every runtime initializer remain outside scope.

The evidence root is identified relatively as `disassembled alpha cube world`. The supported image is `binary/Cube.exe`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`. It is a PE32 x86 Windows GUI executable, with preferred image base `0x400000`, entry RVA `0x28E1E0` and image size `0x3BC000`. RVAs below are relative to that image. The audit used original executable bytes and individually identified original function/assembly exports, without reconstruction, runtime execution or other contracts.

## Loader dependencies and later activation

The normal PE import directory is at RVA `0x36074C`, with size `0x12C`. The delay-import directory is absent. The executable directly imports the following 14 DLLs; these are normal loader dependencies, including dependencies whose imported functions are called only after window creation.

| Direct import | Evidence relevant to this boundary |
| --- | --- |
| `d3d9.dll` | Imports `Direct3DCreate9` |
| `DINPUT8.dll` | Imports `DirectInput8Create` |
| `MSVCR110.dll` | Imports native C runtime startup, termination, allocation and other services |
| `MSVCP110.dll` | Imports native C++ runtime and stream/container services |
| `zlib1.dll` | Four ordinal imports: 58, 11, 55 and 50 |
| `FreeImage.dll` | Named image-loading and memory/image-access imports |
| `WINMM.dll` | Imports timer/time-period services |
| `WS2_32.dll` | Ten ordinal imports, including the startup route called before class registration |
| `KERNEL32.dll` | Process, file, memory, synchronization and library-loader services |
| `USER32.dll` | Window-class registration, creation, message, display and dialog services |
| `SHELL32.dll` | Imports wide command-line argument parsing |
| `ole32.dll` | Imports COM initialization and object activation |
| `IPHLPAPI.DLL` | Imports adapter-information retrieval |
| `GLU32.dll` | Imports tessellation services |

Normal loading must resolve this image's direct imports before its entry routine can execute. A missing or incompatible direct dependency, an unresolved named export or an unresolved ordinal can therefore prevent the application from reaching its own window-creation call. This is an executable-loader prerequisite, not a failure produced by the later Direct3D or DirectInput admission checks. The supported image is x86; a same-named DLL of incompatible architecture does not satisfy that image's import requirement. Actual DLL search results, forwarded exports, dependencies of these DLLs and their initialization routines were not audited.

Neither an XAudio DLL nor a D3DX DLL appears in this direct import table. The image has imports for `LoadLibraryA`, `LoadLibraryW` and `GetProcAddress`, exposed through native thunks at RVAs `0x28D47C`, `0x28D4E2` and `0x28D494`. Their presence alone does not establish a particular dynamically loaded dependency. The inspected entry/application window boundary contains no explicit request for a named DLL through these APIs. A literal-string inspection of this image finds DLL-name strings for the direct imports, without an additional XAudio/D3DX DLL filename. This does not exclude indirect, constructed-name or dependency-owned loading.

The adjacent audio startup route does make a dynamic COM activation request after game-window creation. At RVA `0x223530` it calls `CoInitializeEx` with null reserved pointer and mode zero, then `CoCreateInstance` with no outer object and class-context value 1. The requested class identifier is `5a508685-a254-4fba-9b82-9a24b00306af`, and the requested interface identifier is `8bcf1f58-9fe7-4583-8ac6-e2adc465c8bb`, stored at image RVAs `0x302D68` and `0x302D78`. The application describes failure of this route as an XAudio2 initialization failure. The executable evidence establishes the COM class/interface request, not the filename or version of the DLL selected by the operating system's COM registration. No specific XAudio DLL filename is established as a direct import or explicit named library request here.

Sources: `binary/Cube.exe`, PE headers, import descriptors/name and ordinal tables, native thunks at RVAs `0x227210`, `0x227216`, `0x28D47C`, `0x28D494` and `0x28D4E2`, audio RVAs `0x223530`–`0x2235A4`, and GUID bytes at `0x302D68`–`0x302D87`; `decompiled/client/functions/00623530.c`.

## Work before the application's window request

The PE entry at RVA `0x28E1E0` first initializes the security cookie and enters native runtime startup. Before calling the application routine, runtime startup obtains the initial show-mode value, serializes initialization using a four-byte ownership word at image RVA `0x36E2E4`, and examines its four-byte initialization state at `0x36E2E8`.

The ownership acquisition retries while another ownership value remains present; the inspected retry at `0x28E217`–`0x28E225` has no timeout or sleep. Reentry with the current ownership value follows a separate admitted route. An initialization state equal to 1 calls the runtime fatal-message exit with numeric reason `0x1F`. For a previously uninitialized state, startup first runs the error-reporting initializer range bounded by RVAs `0x2FC8C8` and `0x2FC8DC`; a nonzero result returns `0xFF` before the application routine. It then runs the other initializer range bounded by `0x2FC62C` and `0x2FC8C4` and records initialization state 2. The image contains four nonnull entries in the former range and 165 in the latter.

Those initializers are a real prerequisite to this application's window call. Their entire transitive work has not been reconstructed. The inspected early runtime callbacks include process/runtime setup, runtime allocation and argument setup, CPU-feature setup and exit-handler registration. In particular, native argument setup at RVA `0x28E195` calls the runtime argument service and invokes the fatal-message exit with reason 8 for a negative result. This contract does not certify all initializer callbacks as nonblocking, free of file access or free of additional library requests.

After initializer processing, startup contains a guarded optional callback through the four-byte slot at image RVA `0x36E2EC`: the call requires a nonnull slot and a successful image-protection check. The slot lies in initially zero-filled image storage; subsequent initializer effects on it are not fully closed. Startup then scans the native command line past the executable portion, preserving quote/multibyte handling, and calls the application routine at RVA `0x0C8AE0` from `0x28E307`. The instance value passed for this supported image is `0x400000`; the previous-instance argument is zero. Detailed runtime exception and termination behavior remains outside this boundary.

The application's first direct startup work records the instance at image RVA `0x36B1C4` and requests Winsock version `0x0202` at `0x0C8B0F`. Its return value is not tested before continuing to class registration. It prepares the class named `Cube`, with window procedure RVA `0x0C83F0`, zero class-style and extra-byte fields, the supplied instance, a null class cursor, background value 6 and the name `Cube` in both class-name and menu-name fields. It requests an icon named `Cube`; that return is assigned into the class description without a separate failure branch.

The supported image's embedded resource directory contains one manifest resource: type 24, identifier 1, language 1033, data RVA `0x370060`, length 381 bytes. The manifest requests execution as the invoking user with UI access disabled and contains no dependent-assembly declaration. This resource inspection establishes no embedded menu or icon resource named `Cube`. The consequences of missing named resources inside the operating system's registration/creation routines are not independently reconstructed here; no extra application-side resource validation is visible in this direct path.

Sources: `binary/Cube.exe`, entry RVAs `0x28E1E0`–`0x28E30C`, initializer pointers at `0x2FC62C`–`0x2FC8DC`, native callbacks `0x28E0DC`–`0x28E1DF`, `0x28D674`–`0x28D6AF`, `0x28E4B8` and `0x28E645`, application RVAs `0x0C8AE0`–`0x0C8B88`, and resource directory/manifest at `0x370000`/`0x370060`; `decompiled/client/functions/0068e1e0.c`; `decompiled/client/functions/004c8ae0.c`.

## Creation, failure exits and later visibility

The application calls `RegisterClassW` at RVA `0x0C8B88`. It tests the returned 16-bit atom for zero. Zero bypasses window creation and reaches the common return at `0x0C9534`; there is no application message box on this branch and no retry accepting an already registered class.

With a nonzero atom, it calls `CreateWindowExW` at `0x0C8BC8`. The requested extended style is zero; class and title are `Cube`; style is `0x06CF0000`; requested position is 200, 200 and dimensions are 800 by 600. Parent window, explicit menu handle and creation parameter are null, and the instance is the stored application instance. The returned 32-bit window handle is stored at image RVA `0x36B1C0`, then tested for null. Null also reaches the common return without the application's DirectX failure dialog. That common return supplies application result zero. The normal native runtime path subsequently passes the application result to process exit.

The initial style does not contain the visible bit `0x10000000`. A successful window handle therefore marks creation before the application's later explicit show request. The immediate next call, at `0x0C8BDB`, is the Direct3D initialization helper at RVA `0x0C8720`. A false byte result invokes a modal failure message box owned by the already created window, then returns application result zero. The diagnostic recommends a DirectX end-user runtime, but that generic diagnostic does not identify which nested graphics prerequisite failed.

That graphics helper requests `Direct3DCreate9` with argument `0x20`. A null result fails. It then requests device creation through the returned object's native method, supplying the already created window handle; a result other than zero fails this stage. Further renderer/font/auxiliary-object initialization also precedes the helper's final success byte. Their complete assets, capabilities and failure requirements are deliberately not reconstructed here. Thus an existing `d3d9.dll` is a loader prerequisite, while successful graphics initialization is a later, stricter prerequisite to the application's show request.

On graphics success, display-mode processing, screen-dimension retrieval and intervening initialization calls precede audio activation at `0x0C8DC5`. A false audio-helper result displays an XAudio2 diagnostic and leaves the normal startup path through cleanup and result zero. The audio helper does not test the `CoInitializeEx` return before the activation request. It rejects a negative COM activation result and negative results from its following audio initialization/mastering operations; those tests are signed failure tests, not the application DirectInput test described next.

The application requests `DirectInput8Create` at `0x0C8E2D`, with version `0x0800` and its original interface identifier. It admits exactly return value zero. Any nonzero value displays a DirectInput8 diagnostic and leaves through cleanup/result zero. On admitted input setup, keyboard/mouse object setup, wide command-line parsing, an attempt to create the `Save` directory and client allocation/construction precede visibility. The client constructor call is at `0x0C8FF9`. Its transitive file, content and initialization requirements are outside this contract. Allocation/construction and subsequent virtual calls are not given a new successful-null-object guarantee here.

The explicit `ShowWindow` request finally occurs at RVA `0x0C900F`, with the created handle and numeric show command 3. Cursor operations follow, then `UpdateWindow` at `0x0C902B`, focus assignment, timer-period setup and the main message/frame loop. These calls establish native ordering; they do not guarantee that any particular pixels were painted or that the window became foreground or visibly rendered on a particular system.

Sources: `binary/Cube.exe`, RVAs `0x0C8B88`–`0x0C8C0C`, `0x0C8D9A`–`0x0C8E53`, `0x0C8FF9`–`0x0C903F`, `0x0C9514`–`0x0C9544`, `0x0C8720`–`0x0C8934`, and `0x28E307`–`0x28E31B`; `decompiled/client/functions/004c8ae0.c`; `decompiled/client/functions/004c8720.c`; `decompiled/client/functions/00623530.c`.

## Blocking boundaries and remaining limits

Before the application window call, the runtime initialization ownership retry has no native timeout in the inspected loop. DLL-loader initialization, runtime callbacks, Winsock initialization and operating-system window operations are synchronous dependencies whose duration and complete failure behavior were not audited. A missing dependency can stop execution before application entry; an early runtime failure can stop it before the application routine; class-registration or creation failure can return without creating a usable game-window handle.

After a handle exists, the three inspected initialization diagnostics call a modal message box before their failure return, so their acknowledgement can delay exit while the main game window has not reached its explicit show request. Native construction and COM/driver operations can also delay that later milestone; no general startup timeout is established.

The registered window procedure routes size/focus-gain messages, numeric kinds 5 and 7, to RVA `0x0C8940`. That helper checks for positive extents and a nonnull graphics device before attempting a reset. An unsuccessful reset is retried after 100 milliseconds of sleep, without an inspected retry limit, until its result becomes zero. This bounds a further synchronous window-message blocking route once a graphics device exists. It does not prove that this reset loop runs during the initial creation call, when device initialization has not yet occurred on the direct application path.

No exact loader error text, dependency search order, registry-selected audio DLL, redistributable installer version, device-driver diagnosis, complete initial-resource requirement or comprehensive reason for an absent visible window follows from this evidence alone. In particular, an absence of a shown game window does not establish that the executable failed before `CreateWindowExW`; several substantial prerequisites occur after handle creation and before `ShowWindow`. This contract establishes their order and audited admission tests without reconstructing broader game initialization.

Sources: `decompiled/client/functions/004c83f0.c`; `decompiled/client/functions/004c8940.c`; `binary/Cube.exe`, entry ownership retry RVAs `0x28E217`–`0x28E225`, application diagnostics `0x0C8BF6`, `0x0C8DE0` and `0x0C8E48`, and window helpers RVAs `0x0C83F0` and `0x0C8940`.
