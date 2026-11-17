# Alpha client initial Direct3D9 device request

**FINAL / STABLE / HELD.** This addendum covers the first Direct3D9
device request and its startup result in the x86 `Cube.exe` with
SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
RVAs are relative to image base `0x00400000`. Evidence comes from the
original PE imports and instruction bytes.

Graphics startup begins at RVA `0x0c8720` after creation of the hidden
`Cube` window. It calls `Direct3DCreate9` with SDK version `0x20` at
`0x0c874a`. With a valid factory it calls `IDirect3D9::CreateDevice` at
`0x0c87d0`, using the default adapter ordinal `0`, device type
`D3DDEVTYPE_HAL` (`1`), the hidden window's HWND as the separate
`hFocusWindow` argument, and behavior flags `0x44`
(`D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED`).
The output device pointer goes to client storage at RVA `0x36b1b4`.

The x86 `D3DPRESENT_PARAMETERS` block is 56 bytes. Each field is four
bytes. The client clears the entire block at RVA `0x0c8764` and sends
these values to the first device call.

| Byte offset | Field | Value at the call |
| --- | --- | --- |
| `0x00` | `BackBufferWidth` | Result of `GetSystemMetrics(SM_CXSCREEN)`, metric `0` |
| `0x04` | `BackBufferHeight` | Result of `GetSystemMetrics(SM_CYSCREEN)`, metric `1` |
| `0x08` | `BackBufferFormat` | `0`, `D3DFMT_UNKNOWN` |
| `0x0c` | `BackBufferCount` | `0`, the D3D9 default count of one |
| `0x10` | `MultiSampleType` | `0`, `D3DMULTISAMPLE_NONE` |
| `0x14` | `MultiSampleQuality` | `0` |
| `0x18` | `SwapEffect` | `1`, `D3DSWAPEFFECT_DISCARD` |
| `0x1c` | `hDeviceWindow` | Null. D3D9 uses the separate focus window as the presentation window. |
| `0x20` | `Windowed` | `1` |
| `0x24` | `EnableAutoDepthStencil` | `1` |
| `0x28` | `AutoDepthStencilFormat` | `0x4b`, `D3DFMT_D24S8` |
| `0x2c` | `Flags` | `0` |
| `0x30` | `FullScreen_RefreshRateInHz` | `0` |
| `0x34` | `PresentationInterval` | `0x80000000`, `D3DPRESENT_INTERVAL_IMMEDIATE` |

The width and height are the two primary screen metrics as returned to
the process. The client passes them directly and makes no display mode
or format capability query before this call. In windowed mode the zero
format asks D3D9 to use the current desktop format. The later adapter
mode enumeration uses format `0x16` (`D3DFMT_X8R8G8B8`) after graphics
initialization succeeds. The `options.cfg` read also comes after
successful graphics setup. The first request therefore has no
resolution, fullscreen or multisampling override from that file.

A null factory or any nonzero `CreateDevice` result reports graphics
failure, with no alternative device attempt. With a created device,
startup requires both rendering resource initializers to report success
(call sites `0x0c886f` and `0x0c88ac`, targets `0x28a350` and
`0x047e10`). Both check vertex declaration and shader creation. The first
also checks vertex and index buffer creation. A checked creation failure
reports false. The graphics helper releases the factory on a nonzero
device creation result and releases the device and factory on a
reported render setup failure.

A reported graphics failure takes the same path in the startup caller
at RVA `0x0c8be0`. It shows the window-owned `Cube World` Direct3D
error dialog at `0x0c8bf6` and returns zero at `0x0c8bfc` after
dismissal. The audio initialization call at `0x0c8dc5` and the explicit
window show at `0x0c900f` are reached only on a later successful path.
These findings describe the request and the checked returns. The
resulting surface format and HRESULT for a specific launch remain
dependent on its Windows display and D3D9 environment.
