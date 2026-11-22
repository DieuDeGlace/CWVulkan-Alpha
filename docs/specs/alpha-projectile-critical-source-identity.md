# Alpha client projectile critical-source identity

## Evidence

This ruling covers the direct and area projectile critical decisions in the owner's original 32-bit Alpha client, `Cube.exe`. The PE image base is `0x00400000`, its timestamp field is `0x51EA955E`, and its SHA-256 is `84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF`. All addresses below are relative to that image base. The evidence is the original executable's instructions and numeric constant data.

## Resolving the projectile source

All three lookup sites use the same world and the address of the projectile's first field. The map search at RVA `0x077D90` compares both 32-bit halves of this 64-bit source identifier. The lookup at `0x02F000` returns null for the map's end marker and otherwise returns the actor pointer stored in the matching entry.

1. **Earlier saved actor.** The block at `0x221465` initializes this value to null. The lookup at `0x221476` tests presence. A null result leaves the saved value null. A non-null result triggers another lookup at `0x221486`, whose return value is saved at `0x22148B` even if it is null. The initial presence result itself is never saved as the actor.
2. **Later saved actor.** The lookup at `0x22156C` runs independently of the earlier presence result. Its return value is retained at `0x221578`–`0x22157A` before candidate processing. The direct path reloads it at `0x221A42`. The path leading to the area traversal restores it at `0x2222DF`.

## Critical, buff and hit recipients

| Decision or use | Direct contact | Area contact |
| --- | --- | --- |
| Critical chance | **Later saved actor.** The flag starts false at `0x221B50`. The null check at `0x221B58` skips the call on absence. The call at `0x221B5E` uses the later actor as its receiver. | **Earlier saved actor.** The path loads it at `0x2228AD` and starts the flag false. The null check at `0x2228C1` skips the call on absence. The call at `0x2228C7` uses the earlier actor as its receiver. |
| Conditional source buff | The earlier actor is tested at `0x221B96`. Subject to the intervening gates, this actor receives the chance check at `0x221BC7` and buff type 10 at `0x221C27`. | The earlier actor is tested at `0x22290C`. Subject to the intervening gates, this actor receives the chance check at `0x222943` and buff type 9 at `0x22299A`. |
| Ordinary hit processing | The later actor is passed as the hit source at `0x221E85`. The direct candidate is passed as the target at `0x221E86`. | The later actor is passed as the hit source at `0x222AB0`. The area candidate is passed as the target at `0x222AB1`. |

Each critical call reaches the same actor routine at `0x0444A0`. A null receiver at the applicable check leaves the flag false and consumes no random draw for that critical call. A true result doubles the projectile's damage field at offset `0x48` for this contact. The direct and area multiplications are at `0x221B7A` and `0x2228FC`. The buff checks follow the critical decision on each path. Both buff records carry the native duration value 30000.

Both ordinary hit calls reach `0x196D30` at `0x221E8C` and `0x222AB8`. The direct subtype-2 branch has a separate record path. When that path produces a record, it copies the source identifier directly from the projectile and the target identifier from the candidate at `0x221D9A`–`0x221DBD`. Its critical byte still comes from the direct decision at `0x221B50`.

## Limits

With an unchanged identifier and a stable mapping to a live actor, the earlier and later values identify the same actor. With a consistently missing entry, both values are null. The lookup and use sites establish the distinction between the saved values. Whether a binding can change between those reads during native play, and what guarantees the lifetime of a returned actor throughout processing, remain unresolved. Chance outcomes, the order of different contacts and the circumstances that admit a contact are separate from this identity ruling.
