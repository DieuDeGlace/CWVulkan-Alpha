# Lifetime and use of the generated zone item count

FINAL / STABLE / HELD

The original 32-bit Windows Alpha client, Cube.exe, is 3,885,568 bytes with SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`. Addresses are RVAs from image base `0x00400000`. The count has a verified consequence. For each qualifying group of at least eight generated items, the generator appends one type-9 record to the resident zone.

## Counting and order

An already-resident zone returns at the lookup in `0x1E494C–0x1E4959`. On the construction path, the caller creates a fresh, empty ordered map at `0x1ED873–0x1ED879`, just before traversing the generated item vector at zone `+0x30`. The map belongs to this invocation. Its nodes are separate from both the zone and its item records.

The key consists of exactly three **unsigned bytes** from the embedded item, in this order. They are category at `+0x00`, subtype at `+0x01`, and the item field at `+0x0D`. Each is widened independently to a 32-bit integer. A byte of `0xFF` therefore contributes 255. Other item fields, origin, scale, position, day and item identity do not distinguish keys. The map first compares category, then subtype, then the third field. Evidence includes `0x1ED8BF–0x1ED908`, `0x1C3820`, `0x1C3840`, `0x1C0150` and `0x1C3FE0`.

For every resident element in vector order, a new key starts with a 32-bit count of zero and the client increments the entry once. An existing key receives another increment. The increment at `0x1ED90D` precedes the test of origin bit 0 at `0x1ED915`. Items that bypass settling and items that descend both count once. No item is generated, removed or relocated in response to the tally while this loop runs. Settling makes its own decision from origin and terrain. Evidence includes `0x1C3DE0` and `0x1ED8B4–0x1EDA48`.

## The decision after settling

After all items have been visited, the client enumerates the map once in ascending key order. It tests category first. Categories **1 and 11** proceed to a signed comparison of the count against **8**, inclusive. Every other category and every qualifying-category group with seven or fewer entries produces no record from this pass. Each group passing both tests appends exactly one record to zone collection `+0x48`, after any records already there. Evidence includes `0x1EDA4E–0x1EDB71`.

The appended record is 320 bytes. Its first four bytes hold discriminator 9. A default embedded item begins at record `+0x04`. The three key bytes are copied into record offsets `+0x04`, `+0x05` and `+0x11`. The three signed fixed-point coordinates at `+0x128`, `+0x130` and `+0x138` locate the center of the zone horizontally and zero vertically. In block units each horizontal coordinate is its zone coordinate times 256 plus 128. One block is 65,536 fixed-position units. The record carries neither the numerical count nor a member item's position. Evidence includes `0x1E49BB–0x1E4A07`, `0x1C39E0`, `0x1EDABB–0x1EDB38`, `0x012080`, `0x1F4BA0` and `0x1C3950`.

For example, eight category-11 items with matching subtype and third byte produce one type-9 record. Sixteen with the same key still produce one. Seven with one third-byte value and one with another produce none. Eight in each of two distinct qualifying keys produce two in key order. Eight category-25 items with one key produce none.

## What survives

The generator uses a separately initialized map and a different threshold for the later type-10 append at `0x1EDB7D–0x1EDD0E`. That pass does not read the item counters. The ordinary saved-zone item overlay runs later at `0x1EDE46`. It can replace the resident item list after the type-9 records were formed. It neither recounts the saved items nor retracts or creates a type-9 record on that path. The original item-count map is destroyed at `0x1EE06C–0x1EE072` before the generator returns. No reference to its keys, counts or nodes is exported.

The type-9 copy remains in the zone's separate resident collection. Ordinary zone serialization at `0x04E340` and row decoding at `0x04CDB0` omit that collection. Zone teardown frees its storage at `0x2245BD–0x2245DB`. A later fresh construction counts its own generated items before applying its saved row. Another resident zone has its own storage.

## Limits on downstream use

The verified downstream decision is the category-and-count threshold and its resident record. It occurs after item generation and settling for this zone. The examined local-map references, direct record-append and copy calls, ordinary save/load paths and zone teardown show no transfer of the tally to later generation or another zone. Searches of the original image for the record vector's native 320-byte indexing and traversal operations located its append and reallocation helpers. No further reader of the derived type-9 records was established for gameplay, inter-zone decisions, map display or other UI. Indirect access to the resident collection remains unresolved, so the evidence supports neither a player-visible purpose nor a claim that the derived records can never be read.
