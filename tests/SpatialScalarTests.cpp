#include "worldgen/Regions.h"
#include "worldgen/SpatialScalars.h"
#include "worldgen/Terrain.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cfenv>
#include <cmath>
#include <cstdio>
#include <map>
#include <string_view>
#include <vector>

using namespace worldgen;
static int failures = 0;
static void check(bool ok, const char *message) {
    if (!ok) {
        ++failures;
        std::printf("FAIL %s\n", message);
    }
}

struct Sample {
    int32_t x, y;
    float value = 1.f;
    bool selected = false;
};

struct Fixture {
    std::map<std::pair<int32_t, int32_t>, Sample> samples;
    std::vector<std::pair<int32_t, int32_t>> reads;
    Fixture() {
        const auto query = scalars::distort(16383, 16383);
        for (int32_t x = 0; x < 2; ++x)
            for (int32_t y = 0; y < 2; ++y)
                samples[{x, y}] = {query.x, query.y};
    }
    const Sample *read(int32_t x, int32_t y) {
        reads.emplace_back(x, y);
        const auto it = samples.find({x, y});
        return x >= 0 && y >= 0 && x < 1024 && y < 1024 && it != samples.end() ? &it->second : nullptr;
    }
};

static int benchmark() {
    World world(123);
    world.makeCells(512, 512);
    std::array<double, 5> times;
    for (double &elapsed : times) {
        double checksum = 0;
        const auto start = std::chrono::steady_clock::now();
        for (int32_t i = 0; i < 25000; ++i) {
            const int32_t x = HOME + (i * 37 % 30000) - 15000;
            const int32_t y = HOME + (i * 71 % 30000) - 15000;
            checksum += world.warmth(x, y);
            checksum += world.moisture(x, y);
            checksum += world.seaShare(x, y);
            checksum += world.fireShare(x, y);
        }
        elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        std::printf("100000 queries: %.3f ms, checksum %.17g\n", elapsed, checksum);
    }
    std::sort(times.begin(), times.end());
    std::printf("median %.3f ms\n", times[2]);
    return 0;
}

int main(int argc, char **argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--benchmark")
        return benchmark();
    const auto boundary = scalars::window(16383, 16383);
    check(boundary.firstX == 0 && boundary.lastX == 1 && boundary.firstY == 0 && boundary.lastY == 1,
          "negative adjusted endpoint divides toward zero");
    const auto wrapped = scalars::window(INT32_MAX, INT32_MAX);
    check(wrapped.firstX > wrapped.lastX && wrapped.firstY > wrapped.lastY,
          "endpoint overflow wraps before signed division");
    check(scalars::truncate(2147483648.f) == INT32_MIN && scalars::truncate(-2147483648.f) == INT32_MIN &&
              scalars::truncate(std::numeric_limits<float>::quiet_NaN()) == INT32_MIN,
          "masked conversion results retain the integer-indefinite bits");
    check(scalars::distance(100000, 0, {0, 0}) == INT32_MIN,
          "distance narrows through float before signed truncation without saturation");
#if defined(_M_X64) || defined(_M_IX86) || defined(__SSE2__)
    std::fenv_t originalEnvironment;
    std::fegetenv(&originalEnvironment);
    std::feclearexcept(FE_ALL_EXCEPT);
    const int32_t validMinimum = scalars::truncate(-2147483648.f);
    check(validMinimum == INT32_MIN && !std::fetestexcept(FE_INVALID),
          "representable minimum converts without inventing an invalid exception");
    const int32_t invalidMinimum = scalars::truncate(2147483648.f);
    check(invalidMinimum == INT32_MIN && std::fetestexcept(FE_INVALID),
          "out-of-range conversion retains scalar-SSE invalid status");
    std::fesetenv(&originalEnvironment);
#endif

    auto value = [](const Sample &sample) { return sample.value; };
    auto selected = [](const Sample &sample) { return sample.selected; };
    Fixture fixture;
    auto read = [&](int32_t x, int32_t y) { return fixture.read(x, y); };
    auto result = scalars::average(16383, 16383, .5f, read, value);
    const std::vector<std::pair<int32_t, int32_t>> pass{{0, 0}, {0, 1}, {1, 0}, {1, 1}};
    auto twice = pass;
    twice.insert(twice.end(), pass.begin(), pass.end());
    check(result.positiveTotal && result.value == 1.f && fixture.reads == twice,
          "first and second full passes reload samples in row-major order");
    for (auto &[key, sample] : fixture.samples) sample.value = 2.f;
    check(scalars::average(16383, 16383, .5f, read, value).value == 2.f,
          "scalar average is not capped to one");
    fixture.samples.erase({1, 1});
    fixture.reads.clear();
    result = scalars::average(16383, 16383, .5f, read, value);
    check(!result.positiveTotal && result.value == .5f && fixture.reads == pass,
          "a missing candidate returns the fallback rather than a partial average");

    fixture = Fixture{};
    fixture.samples[{0, 0}].selected = fixture.samples[{1, 0}].selected = true;
    fixture.samples.erase({0, 1});
    result = scalars::fraction(16383, 16383, read, selected);
    check(result.value == 0.f && !result.positiveTotal &&
              fixture.reads == std::vector<std::pair<int32_t, int32_t>>{{0, 0}, {1, 0}, {0, 0}, {0, 1}},
          "membership skips each row tail, but the later full pass rejects missing entries");
    fixture = Fixture{};
    result = scalars::fraction(16383, 16383, read, selected);
    check(result.value == 0.f && !result.positiveTotal && fixture.reads == pass,
          "no selected sample returns zero at the membership boundary");
    fixture = Fixture{};
    fixture.samples[{0, 0}].selected = fixture.samples[{1, 1}].selected = true;
    result = scalars::fraction(16383, 16383, read, selected);
    check(result.positiveTotal && result.value == .5f && fixture.reads.size() == 11,
          "selected weight is reduced separately from total weight across three ordered passes");

    fixture = Fixture{};
    fixture.samples[{0, 1}].x += 2000;
    fixture.samples[{0, 1}].value = std::numeric_limits<float>::quiet_NaN();
    result = scalars::average(16383, 16383, .5f, read, value);
    check(result.positiveTotal && std::isnan(result.value),
          "a zero-weight scalar product still evaluates an unfiltered NaN sample");
    result = scalars::average(INT32_MAX, INT32_MAX, .5f, read, value);
    check(!result.positiveTotal && result.value == .5f, "wrapped empty window retains the scalar fallback");
    {
        World empty(123);
        Terrain terrain(empty);
        Zone cache;
        const auto fresh = terrain.climate(3, 7, &cache);
        check(fresh.moisture == 0.f && fresh.warmth == 0.f && fresh.fire == 0.f,
              "a live matching cache returns constructor zeros without a validity heuristic");
        Column &column = cache.column(3, 7);
        column.moisture = .5f;
        column.warmth = std::bit_cast<float>(0x80000000u);
        column.fire = std::bit_cast<float>(0x7fc12345u);
        cache.place(3, 7, 1, {1, 2, 3, ROCK});
        const auto stored = terrain.climate(3, 7, &cache);
        check(stored.moisture == .5f && std::bit_cast<uint32_t>(stored.warmth) == 0x80000000u &&
                  std::bit_cast<uint32_t>(stored.fire) == 0x7fc12345u,
              "cache hits preserve half, signed zero and NaN payloads through a block write");
        const auto wrongHint = terrain.climate(300, 300, &cache);
        check(wrongHint.moisture == .5f && wrongHint.warmth == .5f && wrongHint.fire == 0.f,
              "a mismatched hint computes producer fallbacks without filling the cache");
        cache.zx = -1;
        cache.column(-1, 7).moisture = .75f;
        check(terrain.climate(-1, 7, &cache).moisture == .5f,
              "negative coordinates cannot expose a matching hinted cache");
        cache.zx = 65536;
        cache.column(16777216, 7).moisture = .75f;
        check(terrain.climate(16777216, 7, &cache).moisture == .5f,
              "the exclusive world limit rejects a hinted cache before reading its scalar");
    }
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
