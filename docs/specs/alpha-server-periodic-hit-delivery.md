# Dedicated Alpha server delivery of type-4 periodic hits

**FINAL / STABLE / HELD**

The dedicated Alpha server can tick a resident type-4 buff on a living role-zero player. When the tick produces only an ordinary hit, **two category-0 records** enter that player's outgoing hit list and **two** enter each connected peer's hit list. They come from one server hit calculation. A previously produced incoming hit follows a different path and contributes **one** record per connection when it names such a player.

## Evidence

The original `binary/Server.exe` is 1,718,784 bytes, SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`. The original `binary/Cube.exe` is 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`. Both use preferred image base `0x00400000`. Addresses below are RVAs in the named image. Role zero means actor control byte `+0x60` equals zero. Counts refer to ordered hit records queued for a connection with the player still present and living. Sound, particle and buff records have separate lists.

## How the player reaches the server tick

Server startup constructs a world with mode zero at `0x149CB7–0x149CBD`. The constructor writes that mode to world `+0xB4` and initializes the distinguished actor pointer at `+0xB8` to null at `0x0C867C–0x0C8682`. Player setup writes role zero at `0x025A19`. The dedicated main loop gathers pending hit and buff events, updates this world with elapsed milliseconds and distributes its output at `0x14A05A–0x14A0D0`.

The server processes pending hit events before pending buffs in this update. For a buff naming a present target it applies the supplied resident record at `0x132BE0–0x132C17`. The resident setter at `0x011740` appends type 4 to the target's list. Type 4 is outside the setter's replace-by-type cases. The actor pass follows in the same update. It iterates the world actors at `0x136253–0x1362AC`. A living role-zero actor passes the health gate and skips an AI update at `0x137891–0x13793B`. That branch rejoins actor processing. For example, an idle role-zero player reaches the resident traversal through `0x137BC4–0x137BCE`, `0x13BC80` and `0x137D01`.

The server subtracts elapsed time at `0x137EF0–0x137F23`. Nonpositive new remaining time queues expiry and bypasses the hit. A surviving type 4 in mode zero makes one hit attempt if the signed 400-ms quotients of the old and new remaining times differ, with division truncated toward zero. Even a crossing of several intervals produces one attempt from that entry. The call at `0x13E19A` uses the current actor as target, the resident magnitude, and the result of looking up the resident source ID. Thus a newly installed entry can tick in this same pass if it meets these conditions. Missing source alone does not prevent an ordinary hit.

## Counts and order for a server-produced tick

| Outcome for one entry and one update | Player's queued hit categories | Each connected peer's queued hit categories |
| --- | --- | --- |
| No qualifying crossing, expired entry, or hit rejected before any record | None | None |
| Ordinary hit only | 0, 0 | 0, 0 |
| Category-5 mitigation, then the remainder stops the hit | 5, 5 | 5, 5 |
| Category-5 mitigation followed by an ordinary hit | 5, 5, 0, 0 | 5, 5, 0, 0 |

The hit producer appends a category-5 record at `0x0D041A` and calls local application at `0x0D04FD`. It tests the remainder afterward at `0x0D0502–0x0D052B`. The ordinary record is appended at `0x0D16A7–0x0D16D2` and locally applied at `0x0D16F0`. On each application, the consumer resolves the target. A living role-zero target and null distinguished actor select `0x0CEB1D–0x0CEB38`, which appends a copy of the same record to that same outgoing accumulator and returns. Consequently, the ordinary and mitigation records each appear as an adjacent pair within the hit list. This consumer branch returns before its HP and resident type-6 mutation paths. Category 3 requires target role 1 at `0x0CFEE1` and cannot supply another outcome for the role-zero target above.

## Already-produced incoming hits and recipient selection

A hit already present in the dedicated world's input list enters local application once at `0x132B40–0x132B60`. The mode-zero route at `0x132AFD–0x132B10` skips the alternate bulk-list operation. This traversal does not call the periodic hit producer. For the same living role-zero target, the consumer's early append therefore contributes **one record of the incoming category**, with no second producer append. If incoming hits and a periodic hit occur in the same update, the admitted incoming copies precede the later actor-pass records in the hit list.

At `0x027330–0x027392` the distributor visits every nonnull connection and appends the world's entire hit list to that connection's own hit list. The range-copy helper `0x020EF0` visits entries in order and appends each once. This copy has no distance, sender or target exclusion for hit records. Thus the player, an originating connection when different, and every other present peer each receive the same ordered contribution to their outgoing lists. At `0x024902–0x024911` the connection worker snapshots its queued events, then clears the consumed hit list.

## Client distinction and limits

In `Cube.exe`, the periodic producer at `0x2183DA` runs only in mode zero. Its ordinary append and application are at `0x198677–0x1986D0`. With the ordinary local player set as the world's distinguished actor, the null-pointer forwarding branch at `0x195AFD–0x195B18` is unavailable. The ordinary producer then contributes one ordinary record and proceeds through the client's local application gates. A count from that local collector alone cannot determine a dedicated recipient's count.

These findings establish static game-update and per-connection ordering for these two original images. They do not establish packet segmentation, connection success, receipt time, visual effects, recipient-side HP changes per copy, or runtime acceptance. The table counts the specified entry in isolation. Other resident entries, world events and already-produced hits can contribute their own records.
