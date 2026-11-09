# Kind-89 boundary of the original relationship predicate

This contract concerns only original client predicate RVA 0x044680 in `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, image base 0x00400000. Evidence paths below are relative to `Documents/disassembled alpha cube world`. The complete predicate was checked directly in the supported image and against `decompiled/client/assembly.txt`, RVA 0x044680–0x044759. `decompiled/client/functions/00444680.c` was a navigation aid. No other contract or replacement implementation is evidence.

The input is the object pointer supplied in ECX. The predicate first reads the unsigned 16-bit word at object+0x7E. Any set bit in mask 0x1A00 immediately selects a false result, before reading the kind field. The relevant individual bits are 0x0200, 0x0800 and 0x1000: respectively word bits 9, 11 and 12 when the least significant bit is bit zero. If all three are clear, the predicate reads the complete 32-bit kind at object+0x64 and compares it against its admitted constants. The kind requested here is decimal 89, whose hexadecimal value is 0x59. It matches the final kind comparison at RVA 0x04474F and selects true. [Evidence: supported `binary/Cube.exe`; `decompiled/client/assembly.txt`, RVA 0x044680–0x04468F and 0x04474F–0x044759.]

For kind exactly decimal 89, these are the results with all unlisted flag bits zero:

| Word at object+0x7E | Relevant bit condition | Predicate result in AL | Complete EAX on return |
| --- | --- | --- | --- |
| 0x0000 | All mask bits clear | 1, true | 0x00000001 |
| 0x0200 | Bit 9 set | 0, false | 0x00000200 |
| 0x0800 | Bit 11 set | 0, false | 0x00000800 |
| 0x1000 | Bit 12 set | 0, false | 0x00001000 |
| 0x1A00 | All three mask bits set | 0, false | 0x00001A00 |

Any combination containing at least one of those three bits has the same false predicate result. Other bits in the 16-bit word do not alter the mask decision. For kind 89 with none of the three bits set, other flag bits therefore leave the result true. Bytes beyond the 16-bit read at +0x7E/+0x7F are not part of this flag input. The kind comparison tests the whole 32-bit value; merely having low byte 0x59 does not establish kind 89. [Evidence: supported `binary/Cube.exe`; `decompiled/client/assembly.txt`, RVA 0x044680–0x04468F, 0x04474F–0x044759.]

The boolean result is the low byte AL. The true exit writes AL = 1, and the false exit clears AL. Neither exit normalizes all 32 bits of EAX. In the early flag rejection, EAX still contains the zero-extended input flag word except for its cleared low byte; this explains the nonzero complete EAX values in the false rows. The inspected relationship callers preserve or compare AL, rather than interpreting complete EAX as a boolean. [Evidence: `decompiled/client/assembly.txt`, RVA 0x044754–0x044759; caller sequences RVA 0x196CD7–0x196CE7, 0x196CF7–0x196D07, 0x1988D1–0x1988E1 and 0x1988F1–0x198901.]

This predicate performs no memory writes, calls, random draws or floating-point operations. Its result does not depend on a clock, health, map order, identifiers or an ambient floating-point control mode. There is no null-pointer admission check before the flag read. The result assumes readable input storage and the stated field values. This narrow contract does not establish the producers or gameplay meaning of the flag bits, caller-level relationship admission, synchronization of concurrent writes, any actor role name, or a server counterpart. It also does not supply the predicate's complete behavior for kinds other than exactly decimal 89.
