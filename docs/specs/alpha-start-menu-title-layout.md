# Alpha start menu title layout

**FINAL / STABLE / HELD**

## Evidence and scope

This contract covers the native windowed start menu and the title scene node named `cubeworld`. Evidence paths are relative to the original Alpha evidence directory. Executable addresses are RVAs and asset addresses are file offsets.

- `binary/Cube.exe` is a 32-bit x86 PE with preferred base `0x400000`, timestamp `0x51EA955E`, image size `0x3BC000`, and SHA-256 `84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF`.
- `game-files/start.plx` has 1,534,507 bytes and SHA-256 `BE1EFE6B5A1CB6B6984495994B10EB830A7F187583EAF44B7D0EAE6504059C53`. The supplied `game-files/options.cfg` selects windowed mode.

## Viewport and update

Startup requests an overlapped window with an outer rectangle of 800 by 600 at screen position (200, 200), then requests that the window be shown maximized. The client is already constructed when that show request is made. For ordinary windowed layout, width and height come from the client rectangle reported by Windows. The maximized window's active drawable aspect therefore depends on the window environment. The dimensions of the restored outer rectangle and the configured 1920 by 1080 are separate quantities. [`binary/Cube.exe` `0x0C8BA3–0x0C8BCD`, `0x0C8FF9–0x0C9015`, `0x0C8958–0x0C8997`]

Both the size message and the focus-acquired message lead to the reset helper. Positive client dimensions are required. The helper updates the GUI extent at origin (0, 0), runs device reset, stores the width and height at client `+0x11C` and `+0x120`, and invokes the client layout callback. The frame loop also runs this helper before rendering when the tracked display settings change. The GUI projection takes its extent from the GUI object's `+0x10C` and `+0x110`. Its orthographic X and Y coefficients are 2 divided by width and negative 2 divided by height, with the origin at the upper left. [`binary/Cube.exe` `0x0C8421–0x0C842F`, jump table `0x0C85A4`, `0x0C863C–0x0C86EC`, `0x0C89C1–0x0C89CD`, `0x0C8A58–0x0C8AA7`, `0x259D70`, `0x288C62–0x288D2B`]

## Title placement, scale, and clipping

The client loads the asset into the existing start scene at client `+0x800874`. The asset loader uses that existing object for the first serialized node. The title is a named child of that root, resolved once and retained at client `+0x800878`. The publisher logo is a separate child at `+0x80087C`. [`binary/Cube.exe` `0x05C2FF–0x05C37A`, `0x28462A–0x28465F`; `game-files/start.plx` `0x175006`, `0x175130`, `0x17637E`]

The title's authored transform has an XY deformation of about 1.954037 on each axis (binary32 `0x3FFA1DE2`) and zero rotation in the initial record. The authored pivot is approximately (276.26282, 247.9013), with binary32 values `0x438A21A4` and `0x4377E6BC`. The authored translation is approximately (523.7373, 552.0988) before the layout callback writes its viewport-dependent placement. [`game-files/start.plx` `0x16E10D–0x16E1F0`, `0x175130`; `binary/Cube.exe` `0x286FF0`]

On layout, the target anchor is half the client width and half the client height, using integer division before binary32 conversion. The callback subtracts the active pivot from those two coordinates, stores the result in the active translation, and immediately refreshes the transform. The refresh adds the pivot back before applying the deformation about that pivot. In the title's initial transform, a one-unit offset from the pivot becomes approximately 1.954037 GUI units on either axis. The viewport dimensions change the anchor and projection extent. The title's authored scale has no aspect-ratio multiplier or fit-to-window adjustment in this path. [`binary/Cube.exe` `0x082B91–0x082BFC`, `0x278600–0x27873A`, `0x278A8E–0x278ADC`, `0x278DFF–0x278EDD`]

The title node's own clipping attribute is zero in both stored slots. One nested group within the title has initial clipping enabled, with stored values 1 and 0. The GUI projection still bounds the complete draw by the client viewport. Since viewport changes leave the title scale fixed, transformed content can extend outside a narrower or shorter viewport. The title node's authored rectangle alone spans about 1,034.7 by 444.6 GUI units after deformation. Its rectangle extends about 506.1 units left and 528.6 units right of the anchor, and about 444.6 units upward. These figures describe the rectangle associated with the node. The painted title bounds depend on descendant geometry, the nested clipping group, opacity, and raster sampling. [`binary/Cube.exe` `0x28359D–0x2835A9`, `0x288C62–0x288D2B`; `game-files/start.plx` `0x273B4–0x273EF`, `0x1728C6`, `0x1729B6`, `0x175130`, `0x175CDE`]

## Frame visibility and limits

The windowed frame loop calls the client update before the render callback. While the start scene is active and the title object exists, the title's selected display slot is set to 1 when the Options panel is closed and to 0 when it is open. The panel is client `+0x800A00`. Selecting Options sets its selected display slot to 1, which makes the next title update hide the title. Opening the panel leaves the publisher logo under the start scene with independent visibility. The publisher logo has a separate layout anchor, 20 client units inward from both the right and bottom edges. The renderer saves and restores the start scene's active state around a separate GUI pass. [`binary/Cube.exe` `0x0C862A–0x0C8636`, `0x0C86E4–0x0C86EC`, `0x088F4E–0x088FC2`, `0x07B6B8–0x07B70F`, `0x183698`, `0x31A1F4`, `0x082C5C–0x082CC4`, `0x0AD48E–0x0AD5E0`]

These are static findings. The exact client extent and event sequence before the first presented frame depend on Windows, including monitor work area, DPI handling, and startup messages. Exact opaque pixel bounds, the nested mask's painted footprint, and subpixel edge placement remain unresolved. The character creation actor and camera are outside this contract.
