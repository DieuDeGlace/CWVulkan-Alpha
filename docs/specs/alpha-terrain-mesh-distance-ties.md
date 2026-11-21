# Alpha client terrain-mesh distance ties

**Author status — FINAL / STABLE / HELD**

## Scope and source

This contract gives the exact candidate order for two 40-by-40 terrain-mesh windows in the original Alpha client. It records the order before the worker checks mesh budget, slot state and source-region residency. Those checks determine which candidate actually gets a build.

The reference is the 3,885,568-byte x86 `Cube.exe`, preferred base `0x00400000`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`. Source evidence comprises the original client image, its disassembly, and individual decompilations at the RVAs cited here. The coordinate sequences below were calculated from synthetic integer windows. An independent reading of the ordering behavior and an instruction-by-instruction calculation from the original disassembly produced identical results for every candidate in both tables.

## Inputs and priority

The worker enumerates X outside Y, discards coordinates outside the inclusive interval 0 through 524,287 on either axis, and sorts the surviving entries by increasing signed 32-bit squared distance from the original window center. The 40-by-40 center is the window origin plus 20 on each axis. Clipping leaves that center in place. The comparison uses the distance field alone. Entries with the same distance can exchange positions when the full list is partitioned. No random draw is involved. Evidence is at worker RVAs `0x0691E6–0x069370`, sort entry `0x055D80`, distance comparisons `0x055270` and `0x0563D0`, and short-range ordering `0x054D60`.

- **Full window.** Origin (100, 100), center (120, 120), 1,600 surviving candidates. The results apply to every 40-by-40 window wholly within the coordinate bounds, including one whose origin is exactly (0, 0).
- **Lower X edge.** Origin (−20, 100), center (0, 120), 800 surviving candidates. X offsets relative to the center span 0 through 19. Y offsets span −20 through 19. The same results apply at other Y origins with this same X clipping and all 40 Y coordinates in range.

## Nearest candidates

Coordinates in each cell are (X, Y) offsets from the window center, in exact visit order from left to right. The first six rows cover the entire central five-by-five square in the full window and all its surviving positions at the lower X edge. At distance 1 the full window begins at positive Y, while the clipped window begins at negative Y. Removing the out-of-bounds entries from an already sorted full window would give a different result.

| Squared distance | Full window | Lower X edge |
| ---: | --- | --- |
| 0 | (0, 0) | (0, 0) |
| 1 | (0, 1) · (-1, 0) · (0, -1) · (1, 0) | (0, -1) · (0, 1) · (1, 0) |
| 2 | (-1, -1) · (1, -1) · (-1, 1) · (1, 1) | (1, 1) · (1, -1) |
| 4 | (2, 0) · (0, -2) · (0, 2) · (-2, 0) | (2, 0) · (0, -2) · (0, 2) |
| 5 | (2, -1) · (1, -2) · (1, 2) · (2, 1) · (-1, -2) · (-2, -1) · (-1, 2) · (-2, 1) | (1, 2) · (2, 1) · (2, -1) · (1, -2) |
| 8 | (2, 2) · (-2, -2) · (-2, 2) · (2, -2) | (2, 2) · (2, -2) |
| 9 | (-3, 0) · (0, -3) · (0, 3) · (3, 0) | (3, 0) · (0, -3) · (0, 3) |

## Complete equal-distance ranks

Each character gives one candidate's zero-based rank among all candidates at the same squared distance. Digits 0 through 9 have their usual values and A through N represent 10 through 23. The leftmost character of each full-window row is X offset −20. The leftmost character of the clipped row is X offset 0. Each following character advances X by one. The row header is the Y offset. All X and Y offsets use the original center.

To recover the complete visit order, read candidates by increasing squared distance and then by increasing rank. For example, in the full window the four candidates at distance 1 have ranks 0 at (0, 1), 1 at (−1, 0), 2 at (0, −1) and 3 at (1, 0). When only a few chunks are eligible, the worker selects the first eligible position in this order. Sorting just the available subset can change ties.

| Y offset | Full window, X −20 through 19 | Lower X edge, X 0 through 19 |
| ---: | --- | --- |
| -20 | `0220300010393323300022222832260333312100` | `30000600010000000000` |
| -19 | `327520217315276052401584360F443526170451` | `11516012031102230120` |
| -18 | `3335631441501245A60A24629107436615261402` | `04135111133210121300` |
| -17 | `22025311015633H2C76BB515A3C7634316716333` | `15104061050511031110` |
| -16 | `06000726445364300CC702E13416423A997A3275` | `03171001332214340002` |
| -15 | `351711312216A479457596262B5391F465525571` | `5110051121A301112110` |
| -14 | `2554423A7701A20E384420571C37842426005300` | `01340122610127130100` |
| -13 | `153070BB577050E2D00A4323541242204933E370` | `52102232031304127011` |
| -12 | `052685E707131750754F38305B15606822875553` | `01013032110512630031` |
| -11 | `2234D10D21568B6344B400CB0555F7234F661256` | `23722310603140200112` |
| -10 | `152177667DA0550F2542702117230604005N2676` | `60233431100633303300` |
| -9 | `0778172FB023EF2201B203002707205384704E63` | `11030307202322230000` |
| -8 | `1D600657464A261712020A2535621C675043743G` | `23111042000323250238` |
| -7 | `1A581634307943D286252B5051C0131D4662B23E` | `12004030013103037324` |
| -6 | `145I646077733805417711253222245206205E11` | `00223005321206303820` |
| -5 | `C331367117E1244A171045346376362050857464` | `20012032300220462221` |
| -4 | `02DD517C3576701300553671340E0533232362B7` | `12030305021017110335` |
| -3 | `1C06E2254C431226922410533144026534F4017A` | `13304201320423132223` |
| -2 | `1E57616774145650031411311437775A6324D33D` | `13111232240532435220` |
| -1 | `14D3512C3111C063125021053429F473947434L6` | `01211214132066230293` |
| 0 | `100832091352202520310303123333A107342712` | `02000622022111262622` |
| 1 | `31BA005E42566846037203362137D066B6634237` | `101001250011573227B2` |
| 2 | `304020B4007541166026220437646F315175952F` | `20003113061323524306` |
| 3 | `3B44913776043302B06721718061672066D3323F` | `22215312001332726107` |
| 4 | `1BCF204B42414B272A470426126767066466430F` | `23201027310320323577` |
| 5 | `B22718653654473603277255520505D1239D5077` | `13301313221163703333` |
| 6 | `023K037B4116893177000543561A974424522877` | `13031212130020031733` |
| 7 | `0C457703072E52E3373114054011384965100669` | `01132111152431302033` |
| 8 | `0F222116792737B6273911145100007023B4155A` | `16322153133532332302` |
| 9 | `3110750C6172611346371565305A114553525550` | `02713110413207002212` |
| 10 | `270009354A931639636443075660311023761314` | `03322220211020111222` |
| 11 | `142DE356F0F218341D85273272022432DC358641` | `02661033115016324703` |
| 12 | `26626016153A326A111D25220631015A31F17330` | `27100311041301014303` |
| 13 | `24150211011A17C6E25F8161A791274E3A76I707` | `45014120112435301333` |
| 14 | `16750629521374115A3311850B45F64162116647` | `10252211732012021323` |
| 15 | `242640473723511E77627730516504J044732002` | `30013422022123003232` |
| 16 | `11341835306055467B3611061278364C0H1B2757` | `11002126110323201333` |
| 17 | `30703202707D10G5E04928239654712544241161` | `41030152161622202023` |
| 18 | `142225627744712435103M714060023006344111` | `13314000221322302013` |
| 19 | `1067363347221B55675332C99138J60012434603` | `00441220513020111211` |

For a whole-sequence check, serialize each sorted offset pair as signed decimal X, one comma, signed decimal Y, and a single LF, including the final LF. Use ASCII digits and minus sign, no leading plus signs or zeros, no spaces, and no header. Zero is the single digit 0.

| Window | Pairs | SHA-256 of ordered offset pairs |
| --- | ---: | --- |
| Full | 1,600 | `707A7E8569FDD443BB78E180E4C7311D2B9640ED7D183703A7CD078603F42B62` |
| Lower X edge | 800 | `B866783955323E71FDB83A19E0311A3CA20E4E98D4A016DE0D786F3BFECF8501` |

## Generalization and limits

Ordinary translations preserve the order when the width and surviving offsets relative to the center stay identical. The candidate enumeration and distance key remain the same, and neither absolute coordinates nor the physical cache slot address enters the comparator. Across different clipping patterns or widths, the sorted list length, initial key sequence and sampled pivot positions can all change.

The threshold at RVA `0x055D80` applies to the number of entries in a sort range. A whole input of 32 or fewer keeps the original enumeration order within each distance. In large inputs, even a small central group can have been reordered before the short-range stage. For ranges of 33 through 41 entries, the pivot sampling uses the first, middle and last entries. At 42 or more, it samples three spaced triples and uses their middle values to select the pivot. The spacing is the range length divided by eight and rounded down. The middle sample index is the range length divided by two and rounded down, counting from its first entry. Evidence is at `0x055470`, `0x055270` and `0x0563D0`.

As a width check, a fully in-range width-five window visits the center, then (−1, 0), (0, −1), (0, 1) and (1, 0). As an axis check, a width-40 window at origin (100, −20) visits the center, then (0, 1), (−1, 0) and (1, 0). Swapping X and Y in the lower-X table would fail that check. Exact tables for other clipping patterns and widths are outside this record. Selection timing, dirty-state changes and region arrival remain separate questions.
