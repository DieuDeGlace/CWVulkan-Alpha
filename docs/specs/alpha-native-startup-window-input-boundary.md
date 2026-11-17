# Alpha client native startup, window and input boundary

**FINAL / STABLE / HELD.** Static contract for the 32-bit Windows GUI
`Cube.exe`, SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
The archived executable and the owner's installed executable have this same
hash. RVAs below are relative to the preferred image base `0x00400000`.
Evidence is the original PE tables and data and the original instruction bytes
at the cited RVAs. The scope is process loading, the first explicit window show
request, and the immediately relevant focus, input and display reset behavior.

## Before the game entry point

- The executable is PE32, machine `0x014c` (x86), GUI subsystem. Its ordinary
  import directory is at RVA `0x36074c`. The delay import directory is empty.
  The 14 ordinary imports are `zlib1.dll`, `WINMM.dll`, `DINPUT8.dll`,
  `d3d9.dll`, `WS2_32.dll`, `KERNEL32.dll`, `USER32.dll`, `SHELL32.dll`,
  `ole32.dll`, `MSVCP110.dll`, `MSVCR110.dll`, `IPHLPAPI.DLL`,
  `FreeImage.dll`, and `GLU32.dll`. The VC11 C++ and C runtimes are x86
  dependencies. The loader must resolve the ordinary modules and imported
  symbols before game startup; zlib imports include ordinals.
- The archived x86 `MSVCP110.dll` also imports `MSVCR110.dll`; the archived
  x86 `zlib1.dll` also imports `msvcrt.dll`. These companion import tables
  do not establish which files or Windows implementations any host loaded.
  `vccorlib110.dll`, `XAudio2_8.dll`, D3DX and XInput are not ordinary or
  delayed imports of this executable. Audio has a later COM dependency.
- The PE manifest requests `asInvoker` with `uiAccess=false`. The entry at
  RVA `0x28e1e0` runs CRT initialization before game startup at
  `0x0c8ae0` (entry calls it at `0x28e307`). Loader or CRT failure is
  separate from the game's handled failures below.

## Hidden window, graphics and options

1. Startup calls Winsock 2.2 initialization and does not test its result here.
   It registers a Unicode class named `Cube` (RVA `0x0c8b88`). If
   registration succeeds, it requests a top-level Unicode window named
   `Cube` at `(200, 200)`, initially `800 × 600` outer pixels (RVA
   `0x0c8bc8`). Style `0x06cf0000` is an overlapped window with child
   and sibling clipping; it has no visible bit. Extended style and parent
   are zero. Failure to register or create returns without a game dialog.
2. The first graphics device attempt is always windowed. RVA `0x0c8720`
   calls `Direct3DCreate9` with SDK version `0x20`; it requests a HAL
   device on adapter 0 using hardware vertex processing and multithreaded
   mode (behavior flags `0x44`, call at `0x0c87d0`). Initial backbuffer
   dimensions are the primary screen metrics, with no multisampling.
   Presentation uses discard, automatic D24S8 depth/stencil and immediate
   presentation. A null factory, nonzero device creation result, or
   reported render setup failure ends this path; the helper provides no
   alternate device retry. Successful graphics setup precedes display mode
   enumeration and the options read.
3. At RVA `0x0c8daf`, startup reads the relative `options.cfg` through
   the reader at `0x0ce6e0`. This is a whitespace token stream with
   case-sensitive keys and formatted integer values. The literal
   `fullscreen` key is followed by an integer; `0` selects windowed
   presentation at reset, and a nonzero value selects fullscreen.
   `resolutionX`, `resolutionY` and `antiAliasing` also feed the later
   display state. The defaults initialize fullscreen and anti-aliasing to
   zero (RVA `0x0ce660`); startup sets the size fields to the current
   primary screen dimensions before attempting to read the file. Failure
   to open it leaves those defaults and does not itself stop startup.
   Changing these settings cannot change the first device creation attempt.

The later display reset helper, RVA `0x0c8940`, uses the actual client area
in windowed mode and configured dimensions in fullscreen; nonpositive
dimensions skip reset. It is called on `WM_SIZE` and `WM_SETFOCUS` (window
procedure RVA `0x0c83f0`), and when the frame path sees a change in the four
display options (RVA `0x0c85f0`). A nonzero `Reset` return prompts a
100 ms sleep and repeat of the same request (`0x0c8a58`–`0x0c8a7a`).
There is no timeout or fallback in this loop. Persistent failure can block
a window message; it is distinct from the first device creation failure.

## Audio, input and first show

- Audio initializes after reading options and before DirectInput. RVA
  `0x223530` calls `CoInitializeEx` without testing its result here, then
  requests in-process activation of XAudio2.7 class
  `5a508685-a254-4fba-9b82-9a24b00306af` with interface
  `8bcf1f58-9fe7-4583-8ac6-e2adc465c8bb` (GUID data at RVAs
  `0x302d68` and `0x302d78`). A negative result from activation,
  engine initialization or mastering voice creation is fatal to this
  startup path. There is no silent audio fallback. The archived
  `XAudio2_8.dll` exports `XAudio2Create` and no `DllGetClassObject`;
  its presence alone cannot supply the requested in-process COM class.
- RVA `0x0c8e2d` calls `DirectInput8Create` with the application
  instance, version `0x0800` and the Unicode interface IID
  `bf798031-483a-4da2-aa99-5d64ed369700`. A nonzero factory result stops
  startup. On zero, the client creates the system keyboard, sets its
  keyboard data format and acquires it. It creates the system mouse,
  sets the mouse data format, sets cooperative flags `0x0a` (background,
  nonexclusive) on the game window and acquires it. The keyboard has no
  explicit cooperative-level call here. Neither device's setup returns are
  checked or retried (RVAs `0x0c8e53`–`0x0c8ed0`); their failures have no
  dedicated diagnostic branch. A failed device creation can leave the next
  call without a valid interface. This startup path initializes only the
  keyboard and mouse.
- After input setup the client processes the first user argument through
  a `server` or string path, attempts to create relative directory
  `Save`, and calls the main game initialization before showing the
  window (RVAs `0x0c8f06`–`0x0c8ff9`). The traced argument path has no
  windowed switch.
- The first explicit show request is `ShowWindow(SW_SHOWMAXIMIZED)` at
  RVA `0x0c900f`, regardless of the fullscreen option or the show mode
  obtained by CRT startup. It sets a null cursor and requests hiding it,
  calls `UpdateWindow`, requests keyboard focus with `SetFocus`, and
  requests a 1 ms timer period before the message and frame loop.
  No success check or activation retry follows the show or focus request.
- Polled keyboard and mouse state is used only when the polling flag is
  enabled and `GetFocus()` returns the game window (RVA `0x0c91bf`);
  GetDeviceState results are not checked here. This is a thread input
  focus check. The traced window path does not inspect the foreground
  process, shell, desktop, window station or session, switch desktops, or
  force foreground activation. The window and focus requests rely on the
  Windows environment in which the process starts.

The three handled subsystem failures use an ANSI message box owned by the
hidden window with caption `Cube World`. The messages begin `Could not
initialize Direct3D.`, `Could not initialize XAudio2.` or `Could not
initialize DirectInput8.` and each recommends the DirectX End-User Runtime.
Call sites are RVAs `0x0c8bf6`, `0x0c8de0` and `0x0c8e48`; message
data begin at `0x302dd0`, `0x302e70` and `0x302f10`. Dismissal leads to
exit before the explicit show request. The handled window failures and all
three handled subsystem failures return zero to the native CRT exit path
(RVAs `0x0c8bfc`, `0x0c9534`, `0x28e31b`); exit code zero does not
establish that a game window was shown.

The static trace establishes the requests and game branches, not the OS
loader message, module search result, COM registry state, desktop/window
visibility, or driver and device HRESULTs on a particular launch. It does
not establish the cause of any observed failure, a first successful frame,
native controller support, or support for unattended or noninteractive
launches. No host or game files were changed or executed to make this ruling.
