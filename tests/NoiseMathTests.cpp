// Exact source-contract anchors, including adjacent reduction/cancellation values.
#include "worldgen/NoiseMath.h"
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

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
    if (argc == 2 && std::strcmp(argv[1], "--bench") == 0)
        benchmark();
    std::printf(failures ? "%d FAILED\n" : "all exact anchors passed\n", failures);
    return failures ? 1 : 0;
}
