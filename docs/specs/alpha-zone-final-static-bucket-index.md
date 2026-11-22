# Final static-object indexing during Alpha zone generation

**Source author status — FINAL / STABLE / HELD**

## Evidence and finding

The original 32-bit Cube.exe has SHA-256 84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF and length 3,885,568 bytes. RVAs below use its preferred base of 0x400000. The evidence is the owner's original image, disassembly and the matching native function listings.

The final loop after zone lighting populates a spatial index of the zone's existing static objects. Every admitted static contributes one reference to one horizontal bucket. The zone's decoration list receives zero new descriptors from this loop. The distinction is visible at the two native destinations. Earlier in the same generator, RVA 0x1EC0AC–0x1EC0DD explicitly appends a type-22 decoration to the list at zone offset 4. The final append at RVA 0x1EE02D instead targets an eight-byte list header in the bucket array reached through zone offset 0xAC.

## Order within generation

The newly generated zone has its X and Y keys at offsets 0x60 and 0x64. Each accepted key is in the range 0 through 65,535. A request finding an existing zone returns that pointer early. The following sequence belongs to new-zone generation. RVAs 0x1E4889–0x1E48C2, 0x1E4952–0x1E495F and 0x1E49BB–0x1E49DA.

1. The generator calls the lighting routine. It then traverses the decoration list already present at zone offset 4. For each descriptor it converts all three fixed position components to integer voxel coordinates, samples light with lane selector zero and the current zone, and stores the returned value as a float at descriptor offset 0x28. This pass updates records in place. RVAs 0x1EDE65–0x1EDF61, 0x02F570 and 0x0718B0.
2. The generator visits the static-object vector at zone offset 0x0C in increasing index order, starting at zero. Its records are 0x188 bytes. The loop checks the vector length after each visit. It reads each static's X and Y position and attempts one spatial-index insertion. The static's type, Z, yaw, dimensions and lighting do not select or suppress an insertion here. RVAs 0x1EDF67–0x1EE048, 0x04B560 and 0x04E890.
3. After the loop the generator stores the zone pointer in the containing region's zone slot and returns it. There is no intervening decoration append or second decoration-light pass before publication. RVAs 0x1EE04E–0x1EE098.

## Bucket and payload

The coordinate source is the original static's signed 64-bit fixed-point X at offset 8 and Y at offset 0x10. One world block spans 65,536 fixed-point increments. Each axis takes the native division-by-eight route, converting the double quotient to a signed 64-bit integer. The following signed integer division by 65,536 truncates toward zero. Over the supported nonnegative world range the resulting coordinate is the containing eight-block bucket. The native sequence also explains the behavior immediately below zero. For example, minus one fixed-point increment gives bucket zero, whereas minus eight whole blocks gives bucket minus one and fails the global range check. No model-center offset, orientation, footprint or vertical coordinate enters this calculation. RVAs 0x1EDF83–0x1EDFE1, 0x0120B0, 0x069060, 0x28D946 and 0x02F570.

Both computed bucket coordinates must lie between zero and 2,097,151 inclusive. With the generating zone supplied, each axis must also be at least 32 times that zone's key and strictly below that base plus 32. A static assigned outside the zone's bucket range is skipped. The loop does not redirect it to a neighboring zone. On acceptance, the row is the low five bits of bucket Y and the column the low five bits of bucket X. Columns vary fastest in the array of 1,024 eight-byte list headers. The zone constructor initializes that array. RVAs 0x02F640–0x02F712, 0x2240F0 and 0x1EDFE1–0x1EDFF0.

The new node has two linkage words followed by four 32-bit payload words, in this order.

| Payload offset | Value |
| ---: | --- |
| 0 | Pointer to the existing static record at the visited vector index |
| 4 | Generating zone X key |
| 8 | Generating zone Y key |
| 0x0C | Zero-based index of the static in that zone's vector |

The append inserts at the bucket's tail and increments its count. Thus statics sharing one bucket retain their original vector order. The node references the original object and carries its addressable identity. It contains no copied decoration position, decoration model ID, scale, tint, lighting sample or randomized attributes. RVAs 0x1EDFF0–0x1EE048, 0x02C500, 0x1C38A0, 0x1F4B50–0x1F4B90 and 0x208A20. As an independent consumer check, the main update looks up a nearby bucket, reads the payload's first word as a pointer, then reads that static's position at offset 8 and its dimensions at offset 0x24. RVAs 0x098F51–0x099012.

## Limits

This establishes the final generation pass through publication and distinguishes it from the preceding decoration-light refresh. Other generators and earlier branches may create decorative models or statics. The final spatial index alone establishes no visual emission rule for those paths. Dynamic editing, subsequent index maintenance and pointer lifetime beyond this generation pass remain outside this contract.
