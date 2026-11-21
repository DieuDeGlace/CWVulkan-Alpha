#include "worldgen/Regions.h"
#include "worldgen/SpatialScalars.h"
#include "worldgen/Terrain.h"
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cfenv>
#include <cmath>
#include <cstdio>
#include <map>
#include <optional>
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

// Independent restatement of alpha-spatial-scalars-1ef040-1f0720.md, written from
// the contract text, for bit comparison with the engine.
namespace contract {

template <class T, class Bits> T from(Bits bits) { return std::bit_cast<T>(bits); }

static_assert(std::bit_cast<uint64_t>(0.0005) == 0x3F40624DD2F1A9FCull);
static_assert(std::bit_cast<uint64_t>(3423.0) == 0x40AABE0000000000ull);
static_assert(std::bit_cast<uint64_t>(23421.0) == 0x40D6DF4000000000ull);
static_assert(std::bit_cast<uint32_t>(3.f) == 0x40400000u && std::bit_cast<uint32_t>(256.f) == 0x43800000u);
static_assert(std::bit_cast<uint32_t>(1.f / 65536.f) == 0x37800000u);
static_assert(std::bit_cast<uint32_t>(scalars::WEIGHT_SCALE) == 0x350637BDu);
static_assert(std::bit_cast<uint32_t>(scalars::WEIGHT_ONE) == 0x3F800000u);
static_assert(std::bit_cast<uint64_t>(1.4) == 0x3FF6666666666666ull);
static_assert(std::bit_cast<uint64_t>(843.0) == 0x408A580000000000ull);
static_assert(std::bit_cast<uint64_t>(4.0) == 0x4010000000000000ull);
static_assert(std::bit_cast<uint32_t>(30.f) == 0x41F00000u);
static_assert(std::bit_cast<uint32_t>(0.4f) == 0x3ECCCCCDu && std::bit_cast<uint32_t>(0.3f) == 0x3E99999Au);
static_assert(std::bit_cast<uint32_t>(32767.f) == 0x46FFFE00u);
static_assert(std::bit_cast<uint64_t>(0.4) == 0x3FD999999999999Aull);
static_assert(std::bit_cast<uint64_t>(32767.0) == 0x40DFFFC000000000ull);
static_assert(std::bit_cast<uint64_t>(0.4000000059604645) == 0x3FD99999A0000000ull);
static_assert(std::bit_cast<uint32_t>(0.1f) == 0x3DCCCCCDu && std::bit_cast<uint32_t>(0.9f) == 0x3F666666u);
static_assert(std::bit_cast<uint32_t>(0.2f) == 0x3E4CCCCDu && std::bit_cast<uint32_t>(0.8f) == 0x3F4CCCCDu);
static_assert(std::bit_cast<uint32_t>(float(HOME)) == 0x4B002080u);

struct Record {
    int32_t x = 0, y = 0;
    bool selected = false; // +0x08 == 1
    float first = 0, second = 0; // +0x0C, +0x10
    int32_t raw = 0, signedField = 0; // +0x14, +0x18
    uint32_t draws = 0;
};

// Creator 0x1D7A70 for a missing entry, reference floats at their initializer value.
std::optional<Record> create(int32_t S, int32_t i, int32_t j) {
    if (i < 0 || i > 1023 || j < 0 || j > 1023)
        return std::nullopt;
    const float referenceFirst = from<float>(0x4B002080u), referenceSecond = from<float>(0x4B002080u);
    const auto index = [](float reference) { return scalars::truncate(reference) / 256 / 64; };
    const bool special = i == index(referenceFirst) && j == index(referenceSecond);
    Random r(uint32_t(i) + 4234u + 1024u * uint32_t(j) + 3u * uint32_t(S));
    Record c;
    c.raw = int32_t(uint32_t(i) + 1024u * uint32_t(j) + uint32_t(S));
    const auto centralFirst = [&] {
        return float(r.draw()) * from<float>(0x3ECCCCCDu) / from<float>(0x46FFFE00u) + from<float>(0x3E99999Au);
    };
    const auto centralSecond = [&] {
        return float(double(r.draw()) * from<double>(0x3FD999999999999Aull) / from<double>(0x40DFFFC000000000ull) +
                     from<double>(0x3FD99999A0000000ull));
    };
    const auto alternate = [&](bool high) {
        const float v = float(r.draw()) * from<float>(0x3DCCCCCDu) / from<float>(0x46FFFE00u);
        return high ? v + from<float>(0x3F666666u) : v;
    };
    if (r.draw() % 2 == 0 || special) {
        c.first = centralFirst();
        c.second = centralSecond();
        if (!special && r.draw() % 10 == 0) {
            c.selected = true;
            c.second = float(r.draw()) * from<float>(0x3E4CCCCDu) / 32767.f + from<float>(0x3F4CCCCDu);
            c.first = float(r.draw()) * from<float>(0x3F000000u) / 32767.f;
        }
    } else {
        c.first = alternate(r.draw() % 2 != 0);
        c.second = alternate(r.draw() % 2 != 0);
    }
    if (special) {
        c.y = scalars::truncate(referenceSecond);
        c.x = scalars::truncate(referenceFirst);
    } else {
        c.y = r.draw() % 15360 + 512 + 16384 * j;
        c.x = r.draw() % 15360 + 512 + 16384 * i;
    }
    const auto quantize = [](int32_t v) { return int32_t(uint32_t(v < 0 ? v + 2047 : v) & ~2047u) + 1024; };
    c.y = quantize(c.y);
    c.x = quantize(c.x);
    const double seed = double(S);
    const float n1 = noise(double(i) * 1.4 + seed, double(j) * 1.4 + seed + 843.0);
    float value = (n1 + 1.f) * 100.f - 70.f;
    const float n2 = noise(double(i) * 4.0 + seed, double(j) * 4.0 + seed + 843.0);
    value = value + n2 * 30.f;
    c.signedField = scalars::truncate(value);
    if (c.signedField <= 0) {
        c.selected = false;
        if (special) {
            c.signedField = r.draw() % 50 + 20;
        } else {
            c.signedField = std::max(scalars::wrapSubtract(c.signedField, 100), -100);
            if (r.draw() % 2 == 0) {
                c.first = alternate(true);
                c.second = alternate(true);
            } else {
                c.first = centralFirst();
                c.second = centralSecond();
            }
        }
    }
    c.draws = r.count();
    return c;
}

// Query 0x1EF040 (byte == 1) or 0x1F0720 (signed field < 0) over engine-resident records.
float query(const World &world, int32_t X, int32_t Y, bool bySignedField) {
    const auto selected = [&](const RegionCell &c) { return bySignedField ? c.elevation < 0 : c.fire; };
    const auto lower = [](int32_t c) { return int32_t(uint32_t(c) - 16384u) / 16384; };
    const auto upper = [](int32_t c) { return int32_t(uint32_t(c) + 16384u) / 16384; };
    const int32_t r0 = lower(X), r1 = upper(X), c0 = lower(Y), c1 = upper(Y);
    if (r0 > r1)
        return 0.f;
    const auto admitted = [&](int32_t row, int32_t column) -> const RegionCell * {
        if (row < 0 || column < 0 || int32_t(uint32_t(row) << 10) + 0x10002F >= 0x20002F || column >= 1024)
            return nullptr;
        return world.cell(row, column);
    };
    bool found = false;
    for (int32_t row = r0; row <= r1; ++row)
        for (int32_t column = c0; column <= c1; ++column) {
            const RegionCell *c = admitted(row, column);
            if (!c)
                return 0.f;
            if (selected(*c)) {
                found = true;
                break;
            }
        }
    if (!found)
        return 0.f;
    const float dx = (noise(double(Y) * 0.0005, 3423.0) * 3.f) * 256.f;
    const float dy = (noise(double(X) * 0.0005, 23421.0) * 3.f) * 256.f;
    const int32_t qx = scalars::truncate(float(X) + dx), qy = scalars::truncate(float(Y) + dy);
    const auto distance = [&](const RegionCell &c) {
        const int64_t ex = (int64_t(c.x) << 16) - (int64_t(qx) << 16);
        const int64_t ey = (int64_t(c.y) << 16) - (int64_t(qy) << 16);
        const float fx = float(ex) * (1.f / 65536.f), fy = float(ey) * (1.f / 65536.f);
        return scalars::truncate(fy * fy + fx * fx);
    };
    int32_t minimum = 0;
    const RegionCell *nearest = nullptr;
    for (int32_t row = r0; row <= r1; ++row)
        for (int32_t column = c0; column <= c1; ++column) {
            const RegionCell *c = admitted(row, column);
            if (!c)
                return 0.f;
            const int32_t d = distance(*c);
            if (!nearest || d < minimum)
                minimum = d, nearest = c;
        }
    if (!nearest)
        return 0.f;
    float total = 0.f, part = 0.f;
    for (int32_t row = r0; row <= r1; ++row)
        for (int32_t column = c0; column <= c1; ++column) {
            const RegionCell *c = admitted(row, column);
            if (!c)
                return 0.f;
            float s = float(scalars::wrapSubtract(distance(*c), minimum)) * from<float>(0x350637BDu);
            if (s > from<float>(0x3F800000u))
                s = from<float>(0x3F800000u);
            const float weight = from<float>(0x3F800000u) - s;
            total = total + weight;
            if (selected(*c))
                part = part + weight;
        }
    return total > 0.f ? part / total : 0.f;
}

} // namespace contract

static void contractTests() {
    // Caller 0x19D640: signed 64-bit division toward zero, low 32 bits.
    check(scalars::queryCoordinate(-1) == 0 && scalars::queryCoordinate(-65535) == 0 &&
              scalars::queryCoordinate(-65536) == -1 && scalars::queryCoordinate(-131071) == -1 &&
              scalars::queryCoordinate(65535) == 0 && scalars::queryCoordinate(int64_t(HOME) * 65536 + 65535) == HOME,
          "caller division truncates toward zero, so small negative components give zero");
    check(scalars::queryCoordinate(int64_t(1) << 47) == INT32_MIN &&
              scalars::queryCoordinate(-(int64_t(1) << 47) - 65536) == INT32_MAX,
          "caller keeps the low 32 bits of the 64-bit quotient");

    // Shift and 1/65536 scale cancel: one rounding of the exact integer difference.
    {
        const int32_t values[] = {INT32_MIN, INT32_MIN + 1, -16777217, -16777216, -46341, -32512, -1, 0, 1, 1024,
                                  32512, 46340, 16777217, 33554433, INT32_MAX - 1, INT32_MAX};
        bool exact = true;
        for (int32_t a : values)
            for (int32_t b : values) {
                const float d = float(int64_t(a) - int64_t(b));
                exact = exact && scalars::distance(a, 0, {b, 0}) == scalars::truncate(0.f * 0.f + d * d) &&
                        scalars::distance(0, a, {0, b}) == scalars::truncate(d * d + 0.f * 0.f);
            }
        check(exact, "distance equals the exact difference rounded once to binary32, squared and summed");
    }

    // Distortion stays within [-768, 768].
    {
        bool bounded = true;
        for (int32_t k = -200000; k <= 200000; k += 997) {
            const int32_t x = HOME + 37 * k, y = HOME - 53 * k;
            const glm::ivec2 q = scalars::distort(x, y);
            const float dx = (noise(double(y) * 0.0005, 3423.0) * 3.f) * 256.f;
            const float dy = (noise(double(x) * 0.0005, 23421.0) * 3.f) * 256.f;
            bounded = bounded && dx >= -768.f && dx <= 768.f && dy >= -768.f && dy <= 768.f &&
                      q.x == scalars::truncate(float(x) + dx) && q.y == scalars::truncate(float(y) + dy);
        }
        check(bounded, "distortion follows the stated chain and stays within 768");
    }

    // An empty second-index interval performs no record work and returns zero.
    {
        Fixture fixture;
        auto read = [&](int32_t x, int32_t y) { return fixture.read(x, y); };
        const auto result = scalars::fraction(16383, INT32_MAX, read, [](const Sample &) { return true; });
        check(result.value == 0.f && !result.positiveTotal && fixture.reads.empty(),
              "an empty column interval reads nothing");
    }

    // Creator: records, null returns, existing entries, special reference.
    for (const uint32_t seed : {0u, 123u, 4000000000u}) {
        World world(seed);
        const int32_t S = world.A();
        bool matched = true, drawsKnown = true;
        int ordinarySelected = 0, specialChecked = 0;
        const auto compare = [&](int32_t i, int32_t j) {
            const RegionCell *c = world.makeCell(i, j);
            const auto expected = contract::create(S, i, j);
            if (!expected) {
                matched = matched && !c;
                return;
            }
            matched = matched && c && c->x == expected->x && c->y == expected->y && c->fire == expected->selected &&
                      std::bit_cast<uint32_t>(c->moisture) == std::bit_cast<uint32_t>(expected->first) &&
                      std::bit_cast<uint32_t>(c->warmth) == std::bit_cast<uint32_t>(expected->second) &&
                      c->id == expected->raw && c->elevation == expected->signedField && world.makeCell(i, j) == c;
            const uint32_t d = expected->draws;
            drawsKnown = drawsKnown && (d == 3 || d == 4 || (d >= 6 && d <= 11)); // the draw-total table
            ordinarySelected += expected->selected;
            if (i == 512 && j == 512)
                specialChecked += expected->x == 8397824 && expected->y == 8397824 && !expected->selected &&
                                  (expected->draws == 3 || expected->draws == 4) && expected->signedField > 0;
        };
        for (const int32_t i : {-1, 0, 1, 511, 512, 513, 1022, 1023, 1024})
            for (const int32_t j : {-1, 0, 1, 511, 512, 513, 1022, 1023, 1024})
                compare(i, j);
        for (int32_t i = 400; i < 440; ++i)
            for (int32_t j = 600; j < 640; ++j)
                compare(i, j);
        check(matched, "creator records match the contract bit for bit; invalid indices give null");
        check(drawsKnown && specialChecked == 1, "special reference (512, 512) quantizes to 8397824 on both axes");
        check(ordinarySelected > 0, "the sample includes byte-selected records");

        // Both queries against the contract restatement over resident records.
        bool same = true;
        for (int32_t k = 0; k < 2000; ++k) {
            const int32_t X = 400 * 16384 + 16384 + (k * 7919) % (37 * 16384);
            const int32_t Y = 600 * 16384 + 16384 + (k * 104729) % (37 * 16384);
            for (const bool bySigned : {false, true}) {
                const float engine = bySigned ? world.seaShare(X, Y) : world.fireShare(X, Y);
                same = same && std::bit_cast<uint32_t>(engine) ==
                                   std::bit_cast<uint32_t>(contract::query(world, X, Y, bySigned));
            }
        }
        check(same, "both selected-fraction queries match the contract bit for bit");
        check(world.fireShare(-1, 600 * 16384) == 0.f && world.seaShare(16383, -16385) == 0.f,
              "a negative candidate index fails admission and returns zero");
    }

    // Fill producer: after its creator calls it reseeds, so preexisting records do not
    // change its later draws.
    {
        const auto height = [](int32_t, int32_t) { return 40.f; };
        World fresh(77), warmed(77);
        warmed.makeCell(300, 300);
        warmed.makeCell(302, 299);
        fresh.prepareRegion(301, 300, height);
        warmed.prepareRegion(301, 300, height);
        const Region *a = fresh.region(301, 300), *b = warmed.region(301, 300);
        bool equal = a && b && a->draws == b->draws && a->tier == b->tier && a->style == b->style;
        for (size_t k = 0; equal && k < a->records.size(); ++k)
            equal = a->records[k].x == b->records[k].x && a->records[k].y == b->records[k].y &&
                    a->records[k].type == b->records[k].type && a->records[k].serial == b->records[k].serial;
        check(equal, "producer draws after its creator calls restart from its own seed");
    }
}

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
    contractTests();
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
