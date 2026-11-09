# Third-party notices

AlphaVulkan's own engine source is GPL-3.0-only (root `LICENSE`). The components below keep
their own licenses and notices.

Cube World is the property of its owners. This is an unofficial project. No game content is
licensed by this document or included in the source.

`shaders/taa.frag` is ported from DieuDeGlace's MIT-licensed TemporalAA mod (by way of
CubeVulkan, which shares this repository owner). The moon (`src/Moon.*`, the moon disc in
`shaders/sprite.frag`) follows the rules of DieuDeGlace's MIT-licensed MoonandShadow mod, also
by way of CubeVulkan; its notice is `licenses/MoonandShadow-MIT.txt`.
The cloud drift in `src/Motion.*` follows the rule of the AlphaCWWorldMotion mod (MIT, the
user's own; notice in `licenses/AlphaCWWorldMotion-MIT.txt`).
The quest icons (`assets/quests/quest-icons.png`, built into the program) are DieuDeGlace's
own MIT-licensed picture, shared with CubeVulkan; notice in `licenses/QuestIcons-MIT.txt`.

**Credit: Cube World Adaption Rebalance by coremaze (Chris Miuchiz).** The default adaption
cost (`rebalancedAdaptionCost` in `src/Loot.cpp`, setting `[Gameplay] AdaptionCost`) uses the
cost curve of that mod, https://github.com/coremaze/Cube-World-Adaption-Rebalance. The
repository carries no license, so none of its code is included: the rule is written anew here
from its description, and the credit for it is theirs.

The build fetches these pinned open-source dependencies:

| Dependency | Pinned revision | License notice |
| --- | --- | --- |
| [Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers) | v1.4.304 | `licenses/Vulkan-Headers-LICENSE.md`, `Vulkan-Headers-Apache-2.0.txt`, `Vulkan-Headers-MIT.txt` |
| [volk](https://github.com/zeux/volk) | 1.4.304 | `licenses/volk-MIT.txt` |
| [GLM](https://github.com/g-truc/glm) | 1.0.1 | `licenses/glm-copying.txt` (MIT or Happy Bunny) |
| [miniaudio](https://github.com/mackron/miniaudio) | 0.11.21 | `licenses/miniaudio-LICENSE.txt` (public domain or MIT) |
| [stb](https://github.com/nothings/stb) | a7d4c2175630e1ea5b6f8dd00ab96fed0b97b96d | `licenses/stb-LICENSE.txt` (MIT or public domain) |
| [GLFW](https://github.com/glfw/glfw) | 3.4 | `licenses/glfw-LICENSE.txt` (zlib/libpng) |
| [SQLite](https://www.sqlite.org) | 3.46.1 (amalgamation) | public domain (https://www.sqlite.org/copyright.html) |
| [zlib](https://zlib.net) | 1.3.1 | `licenses/zlib-LICENSE.txt` (zlib License) |
| [glslang](https://github.com/KhronosGroup/glslang) | 15.0.0 | `licenses/glslang-LICENSE.txt` (multiple notices) |
| [SMAA](https://github.com/iryoku/smaa) | vendored files in `third_party/smaa` | `third_party/smaa/LICENSE.txt` (MIT) |
