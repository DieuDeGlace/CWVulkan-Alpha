# Cursor on the Alpha local creation wait screen

Author status **FINAL / STABLE / HELD**.

On the ordinary terrain-wait frame while the new-character or new-world form is logically open, the client draws its software hand pointer. The ordinary pointer has an empty action caption. Windows receives a separate request to hide the system pointer when the game window opens.

## Why the pointer survives the wait draw

Opening the new-character form sets the display at client +0x800880. Opening the new-world form sets +0x800890. The update determines cursor mode from the logically active forms and other input modes. Either creation form enables it. The update writes that result to the cursor root at +0x8008A8 and positions the cursor from the current interface pointer coordinates. The wait node at +0x800898 has its own display state. See RVAs 0x083F54–0x083F66, 0x084210–0x084223, 0x07F1D0, 0x080DD0 and 0x08CBBA–0x08CC0D.

During rendering the wait branch temporarily hides the creation form roots, draws the interface, then restores the saved form visibility. The cursor belongs to the interface independently of those forms. The wait branch draws it using the visibility established during update. It does not recompute cursor mode with the forms hidden. Consequently the wait flag by itself makes no promise of a hand on other routes. For example, submitting the world form clears +0x800890, and a later update requires another qualifying mode to display the pointer. See RVAs 0x082530, 0x0894F5–0x08950C and 0x0AD47E–0x0AD67A.

## Shape, caption and Windows pointer

The constructor attaches the original `cursor.plx` resource to the cursor root. Its serialized node tree has a mesh named `Mesh` and two text shapes named `info`. The display records for the root and these nodes have visibility enabled in both stored entries. The mesh has a raised-finger hand silhouette with white fill, black stroke and a stored stroke width of 2. Its 22 control positions span approximately 23 by 23 local units before the stroke. These are artwork units. The final pixel bounds depend on the interface transform. The constructor sets flag mask 0x02 recursively on the cursor subtree. The drawing traversal instead checks the display visibility and excludes flag mask 0x04. See RVAs 0x061E29–0x061E9A, 0x06EB90 and 0x232910, and the original resource's mesh, text, display and node records.

The text shapes contain the authored word `ACTION` in their first stored string entry. During update, the client sends an empty string through the cursor subtree. The setter changes text shapes only, so the hand remains drawable. Later contextual branches can supply other text, including a teleport action under its map conditions. Ordinary creation with no such context leaves both captions empty. The wait message is separate from the cursor text. See RVAs 0x08B3A5–0x08B3D3, 0x08B3EF–0x08B6C1 and 0x236AD0.

The host registers its window class with a null cursor. After showing the window it sets the current Windows cursor to null and calls the Windows cursor-display API once with false. These are the image's only direct calls through those two imports. The ordinary wait branch has no Windows pointer visibility change. This evidence describes the client area's intended presentation. It does not establish pointer behavior outside that area or during external focus changes.

## Evidence

The original 32-bit Alpha `Cube.exe` has SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`. RVAs are relative to base 0x00400000. The Windows host is at RVA 0x0C8AE0, with the window-show and two cursor calls at RVAs 0x0C900F, 0x0C9017 and 0x0C901F. The original `cursor.plx` is 11,609 bytes with SHA-256 `a82d79abd362a0b0c2117dfa75f92ab37dc133e96ba7915041ecd090ebc429b9`. Resource field names and values were checked against the original image and resource. No presentation timing or measured screen pixels are assigned here.
