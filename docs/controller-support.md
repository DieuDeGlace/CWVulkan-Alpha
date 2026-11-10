# Controller support

Controller support is an owner-requested engine extension. It uses GLFW's
standard gamepad mappings, including Xbox-style controls and other devices with
an equivalent mapping. Connect the pad before or during play. The engine keeps
the selected device until it disconnects, then selects another mapped pad if one
is available. A newly connected pad must release any held buttons/triggers before
they can act. Loss of window focus clears controller input.

| Control | Gameplay | Menus | World map |
| --- | --- | --- | --- |
| Left stick | Move/swim | Move cursor | Pan |
| Right stick | Camera | Move cursor | Move cursor |
| A | Jump/swim up; revive while dead | Left click, hold to drag/craft | Left click; release to select |
| B | Dodge | Back/cancel | Back |
| RT / LT | Primary / secondary attack | — | Zoom in / out |
| X / Y | Abilities 1 / 2 | Right / middle click | Right / middle click |
| RB / LB | Abilities 3 / 4 | — | — |
| Right stick press | Pick up targeted item, otherwise interact/talk/ride | — | — |
| Left stick press | Use equipped special item, such as glider/boat | — | — |
| D-pad left | Hold to climb | — | Rotate left |
| D-pad right | Quick item | — | Rotate right |
| D-pad up / down | Camera zoom | Existing wheel action | Zoom |
| Start | Menu | Back/cancel | Back |
| Back / View | Open map | Open map when available | Close map |

Menu A/X/Y remain held until release, including list dragging, crafting and
middle-button bench rotation. The cursor stays within the window. Mouse and
controller holds combine: releasing one does not release a button still held by
the other. Disconnect/focus loss cancels the controller hold without activating
a release action. Changing between gameplay, menus, map and chat suppresses held
buttons/triggers until release, so closing a panel cannot also dodge or attack.

Back cancels chat without sending it, leaves options/skills without applying a
draft, returns through character/world screens and closes inventory panels.
It does not quit from the main menu; select Exit explicitly. Pet recall uses T.
Per-action rebinding and rumble are not implemented.

For character/world names, a world seed or a server address, select the field and
press the right stick to open the on-screen keyboard. D-pad moves between keys;
either stick moves the cursor. A selects, X erases, Y changes case/symbols, and B
cancels the draft. Press the right stick again or select Done to accept the text.
Creating a character/world or connecting still requires the existing screen's
button. To chat, open the menu with Start, then press the right stick. Send queues
the message through the normal chat path. Cancel never submits it.

The physical keyboard can also edit this draft: Enter accepts/sends, Escape
cancels and Backspace erases. Opening/closing the overlay consumes its pointer
holds, so a release cannot select a portal or start an attack underneath. The
overlay is an engine extension; the original screens remain behind it. Physical
controller and complete first-time menu workflows still need acceptance testing.

Configure these optional input settings in `AlphaVulkan.ini`:

```ini
[Controller]
Enabled=1
DeadZone=20
TriggerDeadZone=5
LookSpeed=300
CursorSpeed=900
InvertY=0
```

Dead zones are percentages. Sticks use a radial dead zone and retain analog
travel after rescaling. Trigger dead zones are separate. Speeds control camera
and cursor travel per second; `InvertY` affects the gameplay camera. Keyboard and
mouse bindings remain available alongside the pad.

`GamepadTests` checks supplied device states and interface navigation without
requiring hardware. `GamepadTests --devices` enumerates mapped devices through
the actual GLFW backend. Physical device, reconnect, focus and complete in-game
menu acceptance remain release checks; synthetic tests do not certify those.
