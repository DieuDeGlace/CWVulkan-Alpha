#pragma once
// The original's angle step (routine 0x023F70; alpha-airship-steering.md §2.1,
// alpha-remote-creature-interpolation.md §2.3): `from` + factor x asin(sin(to - from)), in
// degrees, with the original's binary32/binary64 roundings. Differences within a quarter turn
// are approached directly; larger ones fold (180 degrees does not move). No wrapping.
namespace creatures {

float angleStep(float from, float to, float factor);

} // namespace creatures
