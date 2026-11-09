#pragma once
#include <bit>
#include <cstdint>

namespace worldgen {
inline constexpr double NOISE_ANGLE = std::bit_cast<double>(uint64_t{0x400921fb5a7ed197});
// Bounded binary64 contract in alpha-noise-cosine-numeric-contract.md. Requires
// nearest rounding and gradual underflow; no contraction/reassociation. The
// engine noise arguments lie in [-NOISE_ANGLE, NOISE_ANGLE]. Other inputs use the
// platform library without an Alpha-equivalence claim.
double noiseCosine(double angle);
}
