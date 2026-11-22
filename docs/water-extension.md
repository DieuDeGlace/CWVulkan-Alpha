# Shared water and swimmable shallows

Alpha uses the revised shared surface from the owner's GPL Steam engine. Five
traveling waves perturb the top-face normal per pixel. Their strength falls
from full at 40 blocks to zero at 120. Per-channel transmission refracts the
opaque scene beneath the water. An above-water top face can reflect the sun
when its alignment, height and shadow visibility permit. The revised surface
retains the nearest water depth for later draws. Underwater, Alpha's sampled
sunlight dims the blue horizon, zenith and fog. Visibility is capped at 48
blocks and retains any shorter scene range. The surface and scene use the
same camera water-volume test.

The render origin and monotonically increasing seconds supply the two phases
in double precision before each is wrapped to a turn. A world point keeps its
ripple position as the local rendering coordinates change. Solar brightness
continues to follow game time. This is the owner's water appearance extension.

### Shared shader source

| GPL Steam source | Pinned identity |
| --- | --- |
| Repository revision | `41ae7d1be330b171a846845f4000f4b22a51dd42` |
| `shaders/omega_water.frag` Git blob | `aa23b6b16a14336886fc2202eeaab263f22f618f` |
| Raw shader SHA-256 | `4d72d1c975bf3b3344fc50446203b9b99e5b6ea9a12e84e7d38a1c99f9a0ca52` |

The Alpha adapter selects the revised branch of that shader and uses its
existing frame bindings, face normals and solar shadow map. The optical
inputs and origin projection follow the same revision's `src/RendererWater.cpp`
and `src/WaterLook.cpp`. The surface needs only Alpha's opaque color and
depth snapshots. The upstream style selector, the older reflection branch
and Steam color grading apply to other rendering modes. [Pinned upstream shader](https://github.com/DieuDeGlace/CWVulkan/blob/41ae7d1be330b171a846845f4000f4b22a51dd42/shaders/omega_water.frag)

The independent [native material contract](specs/alpha-water-material-contract.md)
establishes that material 2 is liquid, while material 3 is solid wet ground with
an additional water-stream top face. The user's extension makes exposed wet
ground into material-2 volume and lowers natural shallow beds to provide three
blocks of water. Water keeps its original surface height. Existing cavities and
protected non-wet construction stop the lowering operation. These are explicit
engine extensions requested by the owner.

The client and dedicated server always convert swimmable terrain before
lighting and map generation. Native terrain comparisons use the diagnostic
generator. The engine applies saved block edits afterward. New single-player
and dedicated-server worlds record `av_generation_profile` in their own world
database. The profile records the water policy and ordered modules and plugins,
including their declared inputs. The engine checks a conflicting profile
read-only before opening the world. To play an original unmarked world, use
AlphaSetup's save import. It adds the swimmable profile to the new copy.
Existing engine profiles keep their water and mod requirements.
[Owner content setup](owner-content.md)

Reeds are placed on dry natural ground directly beside surface water, with clear
space above. Their deterministic scatter uses world coordinates and the seed,
without consuming the generator's native random stream. Reed caches refresh
when neighboring zones arrive, leave, or receive edits, including across zone
seams. Native reed records are retained for oracle comparisons but replaced in
the extension's scenery view. Water lilies retain their existing placement.

Original Alpha clients and servers generate their own native wet-ground shape.
Protocol connectivity alone does not establish terrain or movement agreement
with the swimmable extension. Original-client interoperability with the altered
terrain remains an acceptance gate. Use matching runtime profiles for engine
peers. The save guard fingerprints declared source-module inputs and runtime
plugin binaries and content. Arbitrary inputs outside those declarations,
automatic network profile negotiation and broader migrations remain open.

WaterTests exercises the real movement code in a converted synthetic pool,
retained surface height, lowered bed, dry neighbors, protected geometry,
cavities, repeat application and bank placement across admitted zone boundaries.
`WaterLiveTests` adds generated lake and stream fixtures. GPU captures check
the shared shader. Native gameplay and rendering comparisons remain open.
