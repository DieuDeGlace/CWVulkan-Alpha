# Light on active terrain decorations in the Alpha client

Author status **FINAL / STABLE / HELD**.

The copied decoration light controls the ambient and directional illumination of an ordinary decoration model. A decoration flagged as a light source gets full strength for that part of the calculation and can contribute a separate colored point light. Model opacity has its own distance control. All decoration offsets below are relative to the payload, after the two list links.

## Chunk membership and light sampling

The builder looks up the resident zone for the chunk and examines that zone's decoration list. X, Y and Z are signed 64-bit coordinates with 65,536 units per voxel. For chunk membership it converts each horizontal component to double, multiplies by exactly 1/32, truncates toward zero to a signed integer, and then divides by 65,536 with signed truncation toward zero. Both resulting coordinates must match the requested chunk. Within the valid world's coordinate range this gives 2,097,152 fixed units per chunk. The intermediate double conversion matters if extrapolating to arbitrary 64-bit inputs. See RVAs 0x0A0CD2–0x0A0DB4, 0x28D946 and 0x28D9E0.

For a match, the voxel sample divides the original X, Y and Z by 65,536 independently using signed integer division. There is no subtraction of one voxel, rounding to nearest, or floor adjustment. Examples for one horizontal component are given below. They describe conversion before the resident-zone and world-bounds checks.

| Signed fixed coordinate | Chunk component | Sampled voxel component |
| ---: | ---: | ---: |
| −2,097,152 | −1 | −32 |
| −2,097,151 | 0 | −31 |
| −65,536 | 0 | −1 |
| −1 | 0 | 0 |
| 65,535 | 0 | 0 |
| 65,536 | 0 | 1 |
| 2,097,151 | 0 | 31 |
| 2,097,152 | 1 | 32 |

The sampler accepts horizontal voxel components from zero through 0xFFFFFF. An invalid horizontal location, missing column or elevation below its stored range yields the solid fallback and sampled light zero. Above the stored range the fallback is air for positive elevation and water otherwise, both with first byte 255. In stored air at elevation zero or below, a clear 0x40 flag also selects that water fallback. From the effective voxel, type 13 yields 255, types 0 and 2 yield the larger of the first byte and 5, and all remaining types yield zero. The result is stored as a float at +0x28 before copying. An in-range chunk key therefore does not imply an in-range voxel sample. See RVAs 0x0A0DBA–0x0A0E59, 0x02F7E0, 0x0347A0 and 0x2F9720–0x2F979D.

## Model selection and submission

The mesh worker copies the record into pending slot decorations at +0x248. The renderer copies them into the slot's active list at +0x240. The payload includes its model ID at +0x00, position at +0x08, scale at +0x20, rotation at +0x24, sampled light at +0x28, three light-source color floats at +0x2C through +0x34, and flags at +0x38. The copy preserves those fields. See RVAs 0x059AB0, 0x0A114A and 0x0AC631–0x0AC646.

The ordinary model pass visits selected terrain slots. Slot selection tests active terrain groups, the slot's chunk coordinates and its bounds against the view. The model pass checks the active group count again and tests horizontal slot distance against the square of the outer model distance plus 32, with the addition made before squaring. For a candidate model it checks a scaled bound against camera range and the six view planes. It uses three-dimensional squared distance to a point raised by half the scaled model height for the near/deferred decision. Equality at the inner threshold draws immediately. Eligible farther models are queued with their payload, distance and ordinal for the later pass. Both call RVA 0x0BD160. See RVAs 0x0AC665–0x0AC737, 0x0B1BD7–0x0B1E66, 0x0B25AF–0x0B2806 and 0x0BA4A4–0x0BA503.

The model ID must be nonnegative and in range with a populated model entry. ID 18 is explicitly skipped. ID 60 follows a timed, randomized effect path and bypasses the ordinary model draw. The constructor prepares 64 slots and supplies models for IDs 0–17 and 19–58. The same light rule therefore covers flowers, grasses, corals, reeds, water lilies, rocks, signs and other populated decorations when drawn. Examples include grass at ID 2, lantern at 13, torch at 14 and reed at 22. These are decoration IDs, separate from voxel types. See RVAs 0x062545–0x062BF9, 0x1174D4–0x118547, 0x0B1E91–0x0B212F and 0x0B25AF–0x0B25EA.

## Material color, daytime and emitted light

The renderer starts with a white material tint. If the camera's effective voxel is water, the tint becomes 0.4 red, 0.5 green and 1.0 blue. Its fourth component in both cases is a daytime factor. Over the ordinary 86,400,000-millisecond day, the renderer scales the time to the interval −1 to +1, takes the fourth power, subtracts that result from one, then raises the difference to the fifth power. The factor is zero at the day endpoints and one at midday. The power calls use double precision with single-precision intermediates. See RVAs 0x0AC3E1–0x0AC44A and 0x0AD684–0x0AD755.

For each model the draw routine copies the incoming tint's first three components. With flag mask 0x01 clear, it divides the sampled light by 255 and multiplies by the incoming fourth component, in that order, using single precision. With mask 0x01 set it writes exactly 1.0 to the fourth component regardless of the sample or time. The camera tint still applies. Flag mask 0x04 controls a model transform independently of this lighting calculation. The draw routine sets distance fade independently to one through the inner distance, then decreases it linearly toward zero at the outer distance. Actual indexed geometry submission also requires model vertex and index buffers. See RVAs 0x0BD160, 0x0BE63C–0x0BE725, 0x047FB0 and 0x0E6DF0.

The four components go to vertex-shader register 32, named `materialColor` in the original shader. Its fourth component multiplies the model vertex's own fourth color component and scales the ambient, front-directional and back-directional terms. Point-light contributions are added separately. The shader clamps accumulated illumination by channel to 0 through 1.2, applies vertex RGB and the material RGB tint, then applies the darkness-color and white-mix terms. Vertex output alpha receives the original vertex color's fourth component directly. The copied decoration light can therefore alter visible RGB without serving as model opacity. See RVAs 0x047E10, 0x048280 and the embedded vertex shader at 0x2FD928, particularly 0x2FDDF4, 0x2FDF04–0x2FDF52, 0x2FEA4C–0x2FEAD4 and 0x2FEB34.

The earlier active-list pass independently tests mask 0x01 and checks the source's position against view distance and planes with radius 16. A qualifying source contributes its own three color floats. For decoration ID 13, the lantern, each color channel is multiplied by one minus half the daytime factor. The lantern's emitted color thus reaches half strength at midday while its model uses the same full-strength fourth material component as other flagged sources. This source-color path does not read the copied light. See RVAs 0x0AC761–0x0AC791 and 0x0ACA1D–0x0ACAA6.

## Evidence boundary

These findings cover active terrain decoration payloads and the traced Alpha shader and model passes. Live creatures, dropped items, indexed interactable objects and other rendering programs have separate inputs. The final pixel depends on model vertex data, illumination, fog and the active render pass. No screen measurement or exact frame of publication is inferred. The original 32-bit Alpha `Cube.exe` is 3,885,568 bytes with SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`. RVAs are relative to image base 0x00400000.
