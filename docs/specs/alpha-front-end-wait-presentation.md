# Alpha front-end wait presentation

**Author status — FINAL / STABLE / HELD**

## Scope and evidence

This record covers the original Windows Alpha client's local terrain wait frame during character creation, world creation and entry into a selected world. The ordinary clear is opaque blue. The client draws the words `Please wait...` at the middle of the client area. The visible controls depend on which form was active when the wait began.

Addresses below are RVAs in the original 32-bit `Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`. Evidence comprises that image, its client assembly listing, original routine decompilations, string references and the name table in the owner's original `resource1.dat`. That font file is 19,388 bytes, SHA-256 `f2d473a996288f7d3d0f4a4540bbb71608593bfe40965efa7b02e8c18cc45785`.

## Position and lettering

The client constructs two text shapes. The first is attached to the UI root as a node named `wait`, held at client offset 0x800898. It contains the literal English words with an uppercase P, a space and three ordinary periods. The text has opaque white fill, opaque black stroke, pixel size 12, stroke radius 3, line spacing 3 and alignment flags 1. The second shape is a child of the first. It receives the same text, size, line spacing, font and alignment, and opaque white fill. Its stroke radius retains the text-shape default of zero. The two shapes supply the outlined lettering and its white face at the same location. The construction path supplies no separate shadow offset or animation of the dots. See 0x05B555–0x05B6A5, 0x05B6F0–0x05B7C1 and the text-shape defaults at 0x263240. The original property reader identifies the color, stroke, size, spacing and flag fields at 0x286108–0x286480.

Both shapes request `resource1.dat`. This is a TrueType font whose internal family name is `resource1` and style name is `Bold`. The native text-shape refresh attempts `arial.ttf` if the requested font cannot be resolved. Drawing the text requires a resolved font. See 0x264770 and 0x263DA0.

The layout callback sets the wait node's origin from the current client width and height. It divides each positive integer dimension by two, discards the remainder and uses the resulting pair as floating-point UI coordinates. An 801 by 601 client therefore uses the origin (400, 300). Flag 1 horizontally centers the measured line around that origin. The vertical alignment bits are clear, so the y coordinate remains the normal text origin. The rasterized letter bounds need not have their vertical midpoint there. See 0x082D2C–0x082D7A, 0x25C040 and 0x25DDFE–0x25DEBA.

## Background and draw order

At the beginning of the renderer, 0x0AC312–0x0AC343 clears the full Direct3D target and depth buffer. With the ordinary zero value of client byte 0x8006E6, the packed color is 0xFF0070FF, or RGB (0, 112, 255) at full opacity. The alternate nonzero branch uses opaque white. The constructor initializes the byte to zero at 0x05A000. The clear depth is 1.0. The wait branch has no dedicated background image, dimmer, progress bar or world draw.

After processing available terrain transfers, the renderer checks the wait node at 0x0AD47E. When visible, it suppresses the title, character creation, character picker, world picker and world creation roots for this draw. It also hides the gameplay HUD. It draws the remaining UI tree at 0x0AD5C0, restores the five saved front-end visibility values, ends the scene and returns to the caller, which presents at 0x0C86FF. The HUD is left hidden on this path. The normal terrain, sky and character-preview scene lies beyond this early return. The ordinary UI setup at 0x288B60 can clear depth and stencil under its own flag. Its clear mask leaves the color target intact. Uncovered pixels consequently retain the frame's initial clear color.

## Other controls

| Local state during the wait | Visibility evidence |
| --- | --- |
| Character or world creation | The form and its children disappear for the draw. The shared `Back` button is independently attached to the UI root. Its own visibility is enabled while either creation form is active and the renderer leaves it in the UI traversal. Its nominal size is 200 by 40 and its layout position is (20, 20). Its existing callback returns from character creation to the character picker, or from world creation to the world picker. See 0x05CC95–0x05CD10, 0x083000–0x08301D, 0x089372–0x089425 and 0x0814F0. |
| Duplicate name on a creation form | A separate root-level validation message can also remain visible. The update clears it first and enables it on a duplicate character or world name. It uses the horizontal midpoint and the client height minus 200. This message is independent of the hidden form descendants. See 0x05B91A–0x05B961, 0x082D7D–0x082DCB, 0x089692–0x0896A4, 0x0898BA–0x0898FE and 0x089C51–0x089C87. |
| Entry after submitting a local world | Once all the selection and creation menus are closed, the menu-visibility test turns the shared Back button off. The wait remains eligible while the requested world differs from the current one or the smoothed fog distance is below 8.0. The button and the wait message have separate visibility tests at 0x089372–0x08950C. |
| Cursor | The game attaches `cursor.plx` to its own root-level node at 0x061E34–0x061E92. The update applies the cursor-mode decision to this node and places it at the UI cursor position at 0x08CBBA–0x08CC0D. Either creation form selects cursor mode. The wait node alone does not. A creation-form wait therefore permits the software cursor, while an ordinary world-entry wait with all menus closed and no other cursor-mode condition does not enable it. See 0x07F1D0. |

The renderer suppresses specific roots and traverses the ordinary UI tree. Other independently active UI elements can therefore change the visible result. The source establishes the default clear, text settings, placement and control conditions. It does not establish a captured pixel footprint, the player's cursor position, monitor scaling or the duration of any particular wait.
