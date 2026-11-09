#include "worldgen/NoiseMath.h"
#include <array>
#include <cmath>

namespace worldgen {
namespace {
constexpr double number(uint64_t bits) { return std::bit_cast<double>(bits); }

// Numeric phase data from the held prose contract, represented independently of
// the native packed layout. A coarse slope and residual preserve cancellation.
struct Phase {
    double slopeResidual, value, valueTail, coarseSlope;
};
constexpr Phase phase(uint64_t residual, uint64_t value, uint64_t tail, uint64_t coarse) {
    return {number(residual), number(value), number(tail), number(coarse)};
}

constexpr std::array<Phase, 17> QUARTER{{
    phase(0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x3ff0000000000000),
    phase(0xbf73b92e176d6d31, 0x3fb917a6bc29b42c, 0xbc3e2718e0000000, 0x3ff0000000000000),
    phase(0xbf93ad06011469fb, 0x3fc8f8b83c69a60b, 0xbc626d19c0000000, 0x3ff0000000000000),
    phase(0xbfa60bea939d225a, 0x3fd294062ed59f06, 0xbc75d28da0000000, 0x3ff0000000000000),
    phase(0xbfb37ca1866b95cf, 0x3fd87de2a6aea963, 0xbc672cede0000000, 0x3ff0000000000000),
    phase(0xbfbe3a6873fa1279, 0x3fde2b5d3806f63b, 0x3c5e0d8920000000, 0x3ff0000000000000),
    phase(0xbfc592675bc57974, 0x3fe1c73b39ae68c8, 0x3c8b25dd20000000, 0x3ff0000000000000),
    phase(0xbfcd0dfe53aba2fd, 0x3fe44cf325091dd6, 0x3c68076a20000000, 0x3ff0000000000000),
    phase(0x3fca827999fcef32, 0x3fe6a09e667f3bcd, 0xbc8bdd3420000000, 0x3fe0000000000000),
    phase(0x3fc133cc94247758, 0x3fe8bc806b151741, 0xbc82c5e120000000, 0x3fe0000000000000),
    phase(0x3fac73b39ae68c87, 0x3fea9b66290ea1a3, 0x3c39f630e0000000, 0x3fe0000000000000),
    phase(0xbf9d4a2c7f909c4e, 0x3fec38b2f180bdb1, 0xbc76e0b180000000, 0x3fe0000000000000),
    phase(0xbfbe087565455a75, 0x3fed906bcf328d46, 0x3c7457e620000000, 0x3fe0000000000000),
    phase(0x3fa4a03176acf82d, 0x3fee9f4156c62dda, 0x3c8760b1e0000000, 0x3fd0000000000000),
    phase(0xbfac1d1f0e5967d5, 0x3fef6297cff75cb0, 0x3c75621720000000, 0x3fd0000000000000),
    phase(0xbf9ba1650f592f50, 0x3fefd88da3d12526, 0xbc887df640000000, 0x3fc0000000000000),
    phase(0x0000000000000000, 0x3ff0000000000000, 0x0000000000000000, 0x0000000000000000)
}};

constexpr std::array<Phase, 64> phases() {
    std::array<Phase, 64> result{};
    for (size_t index = 0; index <= 16; index++)
        result[index] = QUARTER[index];
    for (size_t offset = 1; offset < 16; offset++) {
        const Phase forward = QUARTER[offset], reverse = QUARTER[16 - offset];
        result[16 + offset] = {-reverse.slopeResidual, reverse.value, reverse.valueTail, -reverse.coarseSlope};
        result[32 + offset] = {-forward.slopeResidual, -forward.value, -forward.valueTail, -forward.coarseSlope};
        result[48 + offset] = {reverse.slopeResidual, -reverse.value, -reverse.valueTail, reverse.coarseSlope};
    }
    result[32] = {0, 0, 0, -1};
    result[48] = {0, -1, 0, 0};
    return result;
}
constexpr auto PHASES = phases();
}

double noiseCosine(double angle) {
    // Positive IEEE encodings are ordered by magnitude, including the tiny
    // boundary. One classification also sends nonfinite inputs to the library.
    const uint64_t magnitude = std::bit_cast<uint64_t>(angle) & 0x7fffffffffffffff;
    if (magnitude > 0x400921fb5a7ed197)
        return std::cos(angle);
    if (magnitude < 0x3030000000000000)
        return 1;

    // Each operation in this translation unit rounds separately to binary64.
    // Keep the leading displacement: replacing it with the reduced one changes
    // the highest-degree corrections, even though both are small.
    constexpr double scale = number(0x40245f306dc9c883);
    constexpr double integralOffset = number(0x4338000000000000);
    const double scaled = angle * scale;
    const double integral = (scaled + integralOffset) - integralOffset;
    const auto &sample = PHASES[(unsigned)((int)integral + 16) & 63u];
    const double leading = angle - integral * number(0x3fb921fb54400000);
    const double secondary = integral * number(0x3d90b4611a600000);
    const double reduced = leading - secondary;
    const double reductionTail = integral * number(0x3b63198a2e037073) - ((leading - reduced) - secondary);
    const double slope = sample.slopeResidual + sample.coarseSlope;
    const double square = reduced * reduced, fourth = square * square;

    const double oddLow = number(0x3f81111111111111) * square + number(0xbfc5555555555555);
    const double oddHigh = (number(0x3ec71de3a556c734) * leading) * reduced + number(0xbf2a01a01a01a01a);
    const double evenLow = number(0x3fa5555555555555) * square + number(0xbfe0000000000000);
    const double evenHigh = (number(0x3efa01a01a01a01a) * leading) * reduced + number(0xbf56c16c16c16c17);
    const double odd = (oddLow + oddHigh * fourth) * ((slope * reduced) * square);
    const double even = (evenLow + evenHigh * fourth) * (sample.value * square);

    const double coarseLine = sample.coarseSlope * reduced, fineLine = sample.slopeResidual * reduced;
    const double base = coarseLine + sample.value, main = fineLine + base;
    const double firstError = (sample.value - base) + coarseLine;
    const double secondError = (base - main) + fineLine;
    double correction = reductionTail * (sample.value * reduced - slope) + sample.valueTail;
    correction += firstError;
    correction += secondError;
    correction += odd;
    correction += even;
    return main + correction;
}
}
