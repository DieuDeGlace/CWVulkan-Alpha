// Exact source-contract anchors, including adjacent reduction/cancellation values.
#include "worldgen/NoiseMath.h"
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>
#if defined(_M_X64) || defined(__SSE2__)
#include <xmmintrin.h>
#endif

namespace {
struct Anchor { uint64_t input, result; };
constexpr Anchor ANCHORS[] = {
    {0x0000000000000000, 0x3ff0000000000000},
    {0x302fffffffffffff, 0x3ff0000000000000},
    {0x3030000000000000, 0x3ff0000000000000},
    {0x3030000000000001, 0x3ff0000000000000},
    {0x3e50000000000000, 0x3fefffffffffffff},
    {0x3fa921fb54442d17, 0x3feff621e3796d7e},
    {0x3fa921fb54442d18, 0x3feff621e3796d7e},
    {0x3fa921fb54442d19, 0x3feff621e3796d7e},
    {0x3fe0000000000000, 0x3fec1528065b7d50},
    {0x3ff0000000000000, 0x3fe14a280fb5068c},
    {0x3fe921fb5a7ed197, 0x3fe6a09e6217a6a9},
    {0x3ff9eb0b2ee64e80, 0xbfa91f65f10dd7f3},
    {0x3ff9eb0b2ee64e81, 0xbfa91f65f10dd813},
    {0x3ff9eb0b2ee64e82, 0xbfa91f65f10dd833},
    {0x4008bd7366f31c62, 0xbfeff621e3796d7e},
    {0x4008bd7366f31c63, 0xbfeff621e3796d7e},
    {0x4008bd7366f31c64, 0xbfeff621e3796d7e},
    {0x3ff921fb54442d17, 0x3cb469898cc51702},
    {0x3ff921fb54442d18, 0x3c91a62633145c07},
    {0x3ff921fb54442d19, 0xbca72cece675d1fd},
    {0x3ff921fb5a7ed196, 0xbe58ea91f6e59d9c},
    {0x3ff921fb5a7ed197, 0xbe58ea91fae59d9c},
    {0x3ff921fb5a7ed198, 0xbe58ea91fee59d9c},
    {0x400921fb5a7ed197, 0xbfeffffffffffff6},
};

void benchmark() {
    std::array<double, 8192> input{};
    uint32_t random = 41;
    for (double &angle : input) {
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        angle = ((double)random / 4294967295.0 * 2 - 1) * worldgen::NOISE_ANGLE;
    }
    const auto run = [&](const char *name, double (*function)(double)) {
        const auto start = std::chrono::steady_clock::now();
        double sum = 0;
        constexpr int rounds = 1024;
        for (int round = 0; round < rounds; round++)
            for (double angle : input)
                sum += function(angle);
        const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        std::printf("%s: %.2f ns/call; checksum %.17g\n", name, seconds * 1e9 / (input.size() * rounds), sum);
    };
    size_t different = 0;
    for (double angle : input)
        different += std::bit_cast<uint64_t>(std::cos(angle)) != std::bit_cast<uint64_t>(worldgen::noiseCosine(angle));
    std::printf("Platform/contract bit differences on benchmark inputs: %zu of %zu (not a native oracle).\n", different, input.size());
    for (int repeat = 0; repeat < 3; repeat++) {
        run("platform cosine", [](double value) { return std::cos(value); });
        run("contract cosine", worldgen::noiseCosine);
    }
}
}

int main(int argc, char **argv) {
    int failures = 0;
    for (const auto &anchor : ANCHORS)
        for (uint64_t sign : {uint64_t{0}, uint64_t{0x8000000000000000}}) {
            const uint64_t input = anchor.input ^ sign;
            const auto actual = std::bit_cast<uint64_t>(worldgen::noiseCosine(std::bit_cast<double>(input)));
            if (actual != anchor.result) {
                std::printf("FAIL input %016llx: got %016llx, expected %016llx\n",
                            (unsigned long long)input, (unsigned long long)actual, (unsigned long long)anchor.result);
                failures++;
            }
        }
    // Class routing from the sign-cleared upper 16 bits (re-certified boundary
    // corrections 2-4): every tiny input, subnormals included, gives exactly one.
    for (uint64_t input : {uint64_t{1}, uint64_t{0x000fffffffffffff}, uint64_t{0x0010000000000000},
                           uint64_t{0x1234567890abcdef}, uint64_t{0x302fffffffffffff}})
        for (uint64_t sign : {uint64_t{0}, uint64_t{0x8000000000000000}}) {
            const auto actual = std::bit_cast<uint64_t>(worldgen::noiseCosine(std::bit_cast<double>(input ^ sign)));
            if (actual != 0x3ff0000000000000) {
                std::printf("FAIL tiny input %016llx: got %016llx\n", (unsigned long long)(input ^ sign),
                            (unsigned long long)actual);
                failures++;
            }
        }
    // Magnitudes above the angle constant but with upper bits <= 0x40F5 use the
    // regular relations (uncontracted values, so only a sanity bound is checked);
    // from 0x40F6 the x87 fallback is unspecified and the engine uses the library.
    for (uint64_t input : {uint64_t{0x400921fb5a7ed198}, uint64_t{0x4010000000000000}, uint64_t{0x40c3880000000000},
                           uint64_t{0x40f5ffffffffffff}}) {
        const double angle = std::bit_cast<double>(input);
        const double actual = worldgen::noiseCosine(angle);
        if (!(std::fabs(actual - std::cos(angle)) <= 1e-12)) {
            std::printf("FAIL regular-class input %016llx: got %.17g\n", (unsigned long long)input, actual);
            failures++;
        }
    }
    for (uint64_t input : {uint64_t{0x40f6000000000000}, uint64_t{0x4100000000000000}, uint64_t{0x7ff0000000000000},
                           uint64_t{0x7ff8000000000000}}) {
        const double angle = std::bit_cast<double>(input);
        const double actual = worldgen::noiseCosine(angle), library = std::cos(angle);
        if (std::bit_cast<uint64_t>(actual) != std::bit_cast<uint64_t>(library) &&
            !(std::isnan(actual) && std::isnan(library))) {
            std::printf("FAIL fallback-class input %016llx: got %.17g\n", (unsigned long long)input, actual);
            failures++;
        }
    }
    // alpha-floating-point-environment.md: the contracts assume nearest rounding,
    // masked exceptions and no FTZ/DAZ on every generation thread. The engine
    // changes no control word, so each new thread must start in that state
    // (control bits of MXCSR, status flags ignored).
#if defined(_M_X64) || defined(__SSE2__)
    const auto environmentOk = [] { return (_mm_getcsr() & 0xffc0u) == 0x1f80u; };
    bool workerOk = false;
    std::thread([&] { workerOk = environmentOk(); }).join();
    if (!environmentOk() || !workerOk) {
        std::printf("FAIL floating-point environment: MXCSR %08x, worker %s\n", _mm_getcsr(), workerOk ? "ok" : "bad");
        failures++;
    }
#endif
    // Integer-to-float stores round once to nearest, ties to even, above 2^24/2^53.
    volatile int64_t wide[] = {16777217, 16777219, (int64_t{1} << 53) + 1, (int64_t{1} << 53) + 3};
    if ((float)wide[0] != 16777216.f || (float)wide[1] != 16777220.f ||
        (double)wide[2] != 9007199254740992.0 || (double)wide[3] != 9007199254740996.0) {
        std::printf("FAIL integer conversions do not round to nearest even\n");
        failures++;
    }
    if (argc == 2 && std::strcmp(argv[1], "--bench") == 0)
        benchmark();
    std::printf(failures ? "%d FAILED\n" : "all exact anchors passed\n", failures);
    return failures ? 1 : 0;
}
