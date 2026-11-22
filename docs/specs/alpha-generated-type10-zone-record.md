# Generated type-10 zone records in the Alpha client

FINAL / STABLE / HELD

The source is the original 32-bit Windows Alpha client, Cube.exe, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`. Addresses are RVAs from image base `0x00400000`. A freshly generated zone receives one type-10 record for each qualifying group of five or more spawn entries. The group uses two fields. Only the second field survives in the output.

## Which spawns count

After completing the type-9 item pass, the generator creates a fresh local ordered map at `0x1EDB7D–0x1EDB83`. It visits the zone's existing pointer vector at `+0x18` in order, dereferencing one spawn per entry. The pointed-to native type is identified as `cube::Spawn` by its constructor and executable type information. A generated spawn can be traced through construction, field assignment and insertion at `0x1ADB20` and `0x1EADB9–0x1EAE65`. The pass counts spawn entries even if several have the same position. It does not require a live creature.

A spawn contributes exactly once when its full 32-bit field at `+0x44` equals **86,400,000** (`0x05265C00`). All other values skip the increment. This field is the upper endpoint of the spawn's time-of-day window. When the endpoints differ, the actor update compares the world clock to spawn `+0x40` and `+0x44`. The generator's count checks only the upper endpoint. It does not test the lower endpoint, the current clock, the saved day or whether this is an active spawning time. One producer explicitly assigns a nonzero lower endpoint of 21,600,000 and an upper endpoint of 86,400,000. Evidence includes `0x1EB838–0x1EB85E`, `0x1EDBC0–0x1EDBFB`, `0x20FBC1–0x20FBEB`, `0x071910` and the day-clock boundary at `0x20C6A5–0x20C6D8`.

The exact key is the ordered pair of **signed 32-bit** spawn fields at `+0x28` and `+0x2C`. At creature creation the client copies the low byte of the first field to the actor's role byte at `+0x60` and the full second field to its type code at `+0x64`. The generator compares all 32 bits of both key fields. Thus a first field of 257 forms a different group from 1, despite the matching low byte. Each group starts at zero. Every admitted vector entry adds one to its 32-bit count, with no per-pointer or per-position deduplication. Evidence includes `0x057EA0`, `0x194890`, `0x1C3D60`, `0x1EDBD6–0x1EDBF9` and `0x21002B–0x210034`.

## Emission order and payload

The generator finishes counting the entire list before testing any group. It visits keys in ascending signed order, first by `+0x28` and then by `+0x2C`. Only first values **1, 5 and 6** qualify. There is no filter on the second value. A signed count of **5 or greater** produces exactly one record for that key. Each append follows records already in the zone, including any earlier type-9 records. Evidence includes `0x1EDC34–0x1EDCAC` and `0x1EDD04–0x1EDD47`.

| Resident record offset | Value |
| --- | --- |
| `+0x00` | Four-byte discriminator 10. |
| `+0x04` | Constructor-default embedded item. The type-10 pass supplies no item from a spawn. |
| `+0x11C` | Constructor value −1. |
| `+0x120` | The full second key field from spawn `+0x2C`. Its original 32-bit value is preserved. |
| `+0x128`, `+0x130`, `+0x138` | Signed 64-bit world X, Y and Z. Each horizontal block coordinate is 256 times the corresponding zone coordinate plus 128. Z is zero. One block equals 65,536 fixed-position units. |

The client appends the 320-byte record to zone vector `+0x48`. The first key field, count, time window and individual spawn positions are absent from its payload. Five matching entries emit one record and ten still emit one. If groups (1, 17) and (5, 17) each reach five, they append two records with identical defined payload fields and the same zone-center position. Four of each produce none. Evidence includes `0x1E49BB–0x1E4A07`, `0x012080`, `0x1C39E0`, `0x1EDCAD–0x1EDD0E`, `0x1F4BA0` and `0x1C3950`.

## Lifetime and save overlay

Emission finishes before the client records the size of the spawn list at zone `+0xA0`, calls the later population routine at `0x1EDD64`, and conditionally reads the ordinary saved-zone row at `0x1EDE46`. That decoder can overlay spawn `+0x38` and `+0x3C` by index on a matching count. It does not restore either key field, the upper endpoint or the type-10 records. Neither it nor the remaining generator tail repeats this count. The local map is destroyed at `0x1EE061–0x1EE067`.

The emitted copies remain resident through the ordinary overlay. The zone writer at `0x04E340` omits their vector, and teardown releases it at `0x2245BD–0x2245DB`. The verified effect is the generated resident record. No subsequent gameplay, inter-zone or UI reader of type 10 was established. The meanings of individual actor type codes and indirect readers of this vector remain open.
