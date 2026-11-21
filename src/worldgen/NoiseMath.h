#pragma once
#include <bit>
#include <cstdint>

namespace worldgen {
inline constexpr double NOISE_ANGLE = std::bit_cast<double>(uint64_t{0x400921fb5a7ed197});
// Bounded binary64 contract in alpha-noise-cosine-numeric-contract.md. Requires
// nearest rounding and gradual underflow; no contraction/reassociation. The
// engine noise arguments lie in [-NOISE_ANGLE, NOISE_ANGLE]. Native routes larger
// magnitudes below 90112 through the same relations (uncontracted results); from
// 90112, infinities and NaNs reach its unspecified x87 fallback, for which the
// engine uses the platform library without an Alpha-equivalence claim.
//
// Environment (alpha-floating-point-environment.md): native neither sets nor
// establishes MXCSR or the x87 word on its workers, so nearest ties-to-even, all
// exceptions masked and FTZ/DAZ off remain assumptions. The engine never changes
// the floating-point environment: x64 threads start with MXCSR 0x1F80, which
// selects exactly the assumed state and the cosine's SSE2 gate. Native x87
// integer loads/stores (64-bit integer to binary32/binary64) round once under the
// x87 rounding control; under that nearest assumption they equal the engine's
// SSE conversions, and x87 precision control (including any Direct3D 9 change on
// the client primary thread) cannot affect FILD/FSTP or SSE arithmetic.
double noiseCosine(double angle);
}
