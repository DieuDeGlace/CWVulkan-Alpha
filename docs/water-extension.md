# Shared water and swimmable shallows

The runtime uses the same original surface algorithm as the
Steam engine's `shaders/omega_water.frag`. The Alpha adapter changes only its
bindings, frame names and entry point. The renderer copies opaque color and
depth before water. Refraction samples that background and the nearest surface
writes depth. The shader supplies absorption, bank reflections, ripples,
surface sheen and distance haze.

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
