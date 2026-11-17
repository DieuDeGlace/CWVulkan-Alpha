# Alpha ordinary creature link

**FINAL / STABLE / HELD**

Supported image is original Alpha `Cube.exe`, 3,885,568 bytes, SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
Addresses are RVAs from the image at preferred base `0x00400000`. Image
instructions and immediates used here were checked against the original PE.
The scope is the native client, including its offline world turn.

## 1. World lookup and key zero

The ordinary link is the creature's 64-bit key with the low dword at
`+0x11C0` and the high dword at `+0x11C4`. An actor's own ID is at
`+0x08/+0x0C`. The actor turn walks the world creature tree at world `+0x04`
(`0x2104A0–0x2104E6`). The health gate at `0x211AD1–0x211ADB` sends an
ordered nonpositive subject to the next actor. With floating-point exceptions
masked, an unordered comparison takes the living continuation. Both offline and connected
living turns, including received nonlocal actors, reach the shared movement
tail. The mode byte at world `+0xB4` and local-actor pointer at world `+0xB8`
gate earlier work. The local wall-velocity path at `0x21E8A2–0x21E957` and
species-101 angle work at `0x21E95C–0x21E9B2` join before the ordinary lookup.

At `0x21E9B5–0x21E9BE` the turn passes the address of the key as stored,
including all-zero, to `0x02F000`. The lookup takes the signed high dword
first and the unsigned low dword on a tie (`0x077DB0–0x077DE0`). The tree's
end node distinguishes a missing key from an exact key. A missing key or a
node with a null payload yields a null actor. This path makes the same lookup
in both modes. A nonnull payload under key zero proceeds to target health.

- In a fresh client's local registration, the actor and its map node both
  get ID 1 (`0x062005–0x062022`, `0x062205–0x06222C`). This supplies the
  initial local key and leaves zero admission to the other insertion paths.
- In a connected session, the local registration reads an eight-byte
  assigned ID. With the other admission conditions satisfied, zero passes
  the ID check. The resulting node and actor use the same key
  (`0x070104–0x070125`, `0x0701EA–0x070200`, `0x0702C4–0x07032B`).
  Packet 0 reads the leading eight-byte ID and performs lookup, construction
  on a miss, and insertion with that key (`0x06BBB5–0x06BC49`). Packet 1
  takes each eight-byte ID into its temporary record and seen trees, then
  looks up or inserts each final per-ID key in the live map (`0x06C3A0–0x06C44B`,
  `0x06C620–0x06C694`). A zero ID follows these same paths. The packet-local
  actor pointer tests follow lookup or insertion (`0x06BCC5`, `0x06C69A`).
- The offline regional admission passes a supplied descriptor's `+0x48` key
  to construction and insertion as stored (`0x20F948–0x20F94F`,
  `0x20FFD5–0x21000C`). The
  generation bounds of that descriptor key are open in this audit. The
  connection teardown at `0x06A843–0x06A893` can clear the mode byte without
  clearing the creature map in that tail. The mode byte alone supplies no
  zero-key invariant, including across that store. An ensuing offline turn
  consuming a retained key zero, and production of key zero in a fresh stock
  offline world, remain unverified.

## 2. Gates and immediate writes

| Test in order | Result |
| --- | --- |
| Lookup yields a null actor at `0x21E9C5` | Preserve the key and enter the own-motion continuation at `0x21E9E6`. |
| Resolved target health at `+0x16C` is ordered and at most zero at `0x21E9CC` | Clear both dwords of the subject's key at `0x21E9D9/0x21E9DF`, then enter the own-motion continuation. This includes either sign of zero and negative infinity. |
| Resolved health is ordered and positive, or the comparison is unordered | Enter `0x21EAD7`. Positive infinity qualifies. A NaN reaches this branch in normally nontrapping floating-point execution. |
| Admitted target and subject role byte `+0x60` equals 5 | Preserve the key and rejoin own motion at `0x21E9EC`. The target-health clear precedes this role decision. |
| Admitted target and subject role byte differs from 5 | Preserve the key and make the copy below. |

The table gives all admission tests at this site. A map node resolving to
any actor, including the subject itself, supplies the target inputs. The
copy uses the target's live values in the listed order.

1. Replace the subject's live signed 64-bit positions at `+0x10/+0x18/+0x20`
   with the target's live position plus a vertical offset. Subtract the target
   binary32 `+0x1180` from binary32 `1.5`, round that subtraction to binary32,
   multiply by binary32 `65536`, then convert to signed 64-bit with truncation
   toward zero for finite representable inputs. Add that fixed-point offset
   to Z. The X and Y offsets are integer zero (`0x21EAE7–0x21EB40`,
   `0x02C460–0x02C49D`, `0x02C800–0x02C852`).
2. Zero the three binary32 velocity components starting at `+0x34`
   (`0x21EB45–0x21EB6E`).
3. Write the live angle vector at `+0x28` with the componentwise binary32
   sum of `(20, +0, +0)` and the target's live angle vector
   (`0x21EB73–0x21EBAC`, `0x012280–0x0122B4`).
4. Zero the three binary32 motion components starting at `+0x40`
   (`0x21EBB1–0x21EBDA`).
5. Write binary32 `+0x1188` from the target value at the same offset
   multiplied by binary32 `0.5`. Copy the target's four bytes at `+0x118C`
   to the same subject offset (`0x21EBDF–0x21EBFD`).

This copy has no immediate sound, event append or random draw. The success
branch joins at `0x21F000`, past the own-motion and phase work at
`0x21E9E6–0x21EFF8`. The shared later work can still change copied values.
The common tail caps an ordered `+0x1188` above `1.0` and then moves the
field toward zero at `0x21F03B–0x21F079`. The list specifies the immediate
copy writes.

## 3. Movement and riding order

- The local action-106 speed stage can look up `+0x11C8/+0x11CC` at
  `0x219306–0x219319`, before position integration. Per-axis motion can
  update the subject's live position from `0x21B43A/0x21B51B` onward.
  Collision, ground work and the optional wall-velocity change precede
  the ordinary link at `0x21E9B5`. A successful ordinary copy can replace
  live values established by that earlier movement.
- Inside the living per-actor turn, the first active carried-pet copy checks
  current role 5 at `0x21F968`. It follows the pet's separate owner ID
  `+0x198/+0x19C` and requires a resolved owner with live action 106 at
  `0x21F990`. The copy at `0x21FA2F–0x21FB75` follows the ordinary link
  and shared tail. It supplies drawn and live position and angle, takes
  the owner's `+0x1180/+0x1188/+0x118C`, and clears the pet's movement
  vectors. This bypasses the first pass's normal drawn-pose interpolation
  or snap path at `0x21FB80–0x21FD4F` and rejoins at `0x21FD54`.
- After the primary actor loop finishes (`0x21FE21–0x21FE61`), a later
  creature-map sweep starts at `0x220E51`. It checks current role 5 at
  `0x220EEB`, resolves the owner through the same owner key, and tests live
  action 106 at `0x220F1B`. The second copy at `0x220FBE–0x2210FD` repeats
  those drawn and live writes. Neither carried-pet site tests the resolved
  owner's health. The second sweep has no carried subject health gate in
  its role-and-owner branch. Both copies use the owner state available at
  their own sites.
- The pet association at `+0x11C8/+0x11CC` has independent writers
  (`0x20E577/0x20E580`, `0x20E7E3/0x20E7EF`) and independent consumers.
  The explicit nonzero pet-association guard at `0x20E588–0x20E59A`
  belongs to that path. The separate key at `+0x11D0/+0x11D4` has its own
  hit writes at `0x195BF1/0x195BFA`. The later sweep checks it at
  `0x220EAE–0x220EDC` and can clear that pair before the second carried
  copy. Neither pair is the ordinary link.

## 4. Producers, read sites and boundaries

| Traced path | Effect on the ordinary link |
| --- | --- |
| Creature construction at `0x03B690` | Clears both dwords at `0x03B7AA/0x03B7B4`. The actor ID argument is copied to `+0x08/+0x0C` separately at `0x03B6BA–0x03B6CE`. This covers fresh allocations on the traced startup, receive, clone and world-spawn paths. The constructor's separate 256-byte zero fill at `0x03BBDC` starts at creature `+0x1D5C`. |
| Creature reset at `0x046330` | Clears both dwords at `0x046373/0x04637D`. This reset is called by character import at `0x04BE7F`, the local reset path at `0x083F73`, and the pet lifecycle paths at `0x20E5EF`, `0x20E839` and `0x20EADF`. |
| Ordinary health rejection | At `0x21E9D9/0x21E9DF` the stores use the key address formed at `0x21E9B5`. These are writes through the alias to both ordinary dwords. |
| Creature data construction, reset and copy | The state block begins at creature `+0x10`, spans `0x1168` bytes, and ends immediately before creature `+0x1178`. The initializer `0x03C100`, state reset `0x0465C0`, copy constructor `0x066050` and assignment `0x04B040` operate on that block. Its final name field is 16 bytes at block `+0x1158`. The character clones at `0x08238F–0x0823CC` and `0x087612–0x08762B` construct first and copy the state block. Import resets before applying state at `0x04BF93`. These copies leave the ordinary link outside their destination. |
| Connection and packet boundaries | The initial local state receive uses a `0x1168`-byte temporary block (`0x0701A1–0x0701DD`). Packet 0's state reader `0x0CCFA0` and packet 1's `0x0CD3E0` have explicit block-relative fields ending in the `+0x1158` name and adjacent state elements. Applications at `0x06BDFF` and `0x06C96C` use `0x04B040` with creature `+0x10`. Inventory and pose/interpolation writes use their own offsets. Existing ordinary links therefore survive those state applications. New actors start at the constructor's zero. Packet 2 erases selected creature-map nodes (`0x06BE70–0x06BEFA`), changing lookup availability. |
| Adjacent address aliases | The two trees at creature `+0x11AC` and `+0x11B4` use their own headers and allocated nodes, including the initialization at `0x03B74F–0x03B79E`, the destructor aliases at `0x2E2AE1/0x2E2AEF`, and the node insertions at `0x221967/0x2228A3`. The vector at `+0x11A0` occupies 12 bytes. Its copy at `0x21D83D` and nearby readers stay within it. |

The direct displacement audit of executable text found the constructor and
reset writes above. Other actual actor uses of the ordinary displacements
were the lookup alias, pose-side zero checks at `0x013463/0x013701`, the
local-input zero check and lookup at `0x0A6E4E–0x0A6E98`, and ID
comparisons at `0x20F1B1`, `0x21626E`, `0x221642` and `0x2225C0`. The
local-input path explicitly skips its own tree lookup when both dwords are
zero. Its different zero rule applies at that reader.

The traced writers all write zero. Any stock nonzero producer, whole-creature
copy spanning this link, or other computed-address writer remains untraced.
The producer ruling covers the direct displacement sites, observed aliases
and named native boundaries above.
