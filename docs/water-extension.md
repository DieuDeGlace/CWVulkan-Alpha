# Shared water and swimmable shallows

The default runtime profile uses the same original surface algorithm as the
Steam engine's `shaders/omega_water.frag`. The Alpha adapter changes only its
bindings, frame names and entry point. Opaque color and depth are copied before
water, so refraction samples a stable background and the nearest water surface
can write depth. The shared shader supplies absorption, bank reflections,
ripples, surface sheen and distance haze. Alpha's original visual style remains
selectable through `[Water] Style=Alpha`.

The independent [native material contract](specs/alpha-water-material-contract.md)
establishes that material 2 is liquid, while material 3 is solid wet ground with
an additional water-stream top face. The user's extension makes exposed wet
ground into material-2 volume and lowers natural shallow beds to provide three
blocks of water. Water keeps its original surface height. Existing cavities and
protected non-wet construction stop the lowering operation. These are explicit
engine policies, not native Alpha behavior.

The native generator used by oracle tools keeps the extension off. The client
and dedicated server enable it for their runtime terrain, before lighting and
map generation. `[Water] Swimmable=0` selects native terrain in the client for
comparisons; the dedicated server accepts `--native-water` for the same purpose.
Saved block edits are applied afterward. New single-player and dedicated-server
worlds record `av_generation_profile` in their engine-owned world database. A
different water policy or ordered mod ID/version list is rejected before loading
that world. Unmarked legacy worlds are treated as native terrain; start a new
world for swimmable shallows rather than silently reusing their edits and map.
Original saves and game content are not rewritten by this extension.

Reeds are placed on dry natural ground directly beside surface water, with clear
space above. Their deterministic scatter uses world coordinates and the seed,
without consuming the generator's native random stream. Reed caches refresh
when neighboring zones arrive, leave, or receive edits, including across zone
seams. Native reed records are retained for oracle comparisons but replaced in
the extension's scenery view. Water lilies retain their existing placement.

Original Alpha clients and servers generate their own native wet-ground shape.
Protocol connectivity alone does not establish terrain or movement agreement
with the swimmable extension. Original-client interoperability with the altered
terrain remains an acceptance gate; use matching runtime profiles for engine
peers. The save guard records settings and declared mod versions; it does not
fingerprint arbitrary module/content changes under the same ID/version. Automatic
network profile negotiation and broader mod-version migration remain open.

WaterTests exercises the real movement code in a converted synthetic pool,
retained surface height, lowered bed, dry neighbors, protected geometry,
cavities, repeat application and bank placement across admitted zone boundaries.
GPU captures separately check the shared shader. Neither proves full native
gameplay or rendering parity.
