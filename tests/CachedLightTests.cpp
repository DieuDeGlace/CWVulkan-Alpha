#include "CachedLights.h"
#include "worldgen/Zone.h"
#include <bit>
#include <cstdio>
#include <limits>

using namespace terrain_lights;
static int failures = 0;
static void check(bool ok, const char *message) {
    if (!ok) { ++failures; std::printf("FAIL %s\n", message); }
}
static FixedPosition point(int64_t x, int64_t y = 0, int64_t z = 0) { return {x * 65536, y * 65536, z * 65536}; }
static CachedSource source(FixedPosition at, int32_t kind = 14, uint32_t flags = 1) {
    return {at, {1, .5f, .25f}, kind, flags};
}
static std::vector<Candidate> gather(const CachedSources &cache, float reach = 100, float day = 0, int32_t clock = 0) {
    std::vector<Candidate> out;
    cache.append(out, ViewAdmission({}, {}, {}, reach), clock, day);
    return out;
}

static void membership() {
    std::vector<worldgen::Sprite> sprites;
    const int64_t edge = 32 * 65536;
    for (int64_t x : {int64_t(-1), int64_t(0), edge - 1, edge, -edge + 1, -edge}) {
        worldgen::Sprite sprite;
        sprite.x = x;
        sprite.y = -1;
        sprite.z = x + 7;
        sprite.kind = 13;
        sprite.flags = 0;
        sprite.tint[0] = .25f;
        sprites.push_back(sprite);
    }
    const auto zero = chunkSources(sprites, {0, 0});
    check(zero.size() == 4 && zero[0].position[0] == -1 && zero[1].position[0] == 0 &&
              zero[2].position[0] == edge - 1 && zero[3].position[0] == -edge + 1,
          "double scaling then two truncations retain chunk-zero order across positive/negative fractional edges");
    check(zero.size() == 4 && zero[0].position[2] == 6 && zero[0].colour == glm::vec3(.25f, 0, 0) &&
              zero[0].kind == 13 && zero[0].flags == 0,
          "worker selection preserves raw position/color/kind and does not prefilter flag-clear sources");
    check(chunkSources(sprites, {1, 0}).size() == 1 && chunkSources(sprites, {-1, 0}).size() == 1 &&
              chunkSources(sprites, {0, 1}).empty(),
          "both coordinates participate with exact next-chunk boundaries");
    sprites.front().x = std::numeric_limits<int64_t>::max();
    check(chunkSources(sprites, {0, 0}).size() == 3, "far engine coordinates do not overflow a narrowed chunk index");
}

static void publication() {
    CachedSources cache;
    const auto first = source(point(1));
    cache.stage({0, 0}, {first});
    check(gather(cache).empty(), "staged sources are not visible before publication");
    check(!cache.refresh({0, 0}, false, 1) && !cache.refresh({0, 0}, true, 0) && gather(cache).empty(),
          "a cache miss and nonzero pending geometry are both required for refresh");
    check(cache.refresh({0, 0}, true, 1) && gather(cache).size() == 1,
          "admitted refresh publishes the pending list");
    check(cache.refresh({0, 0}, true, 1) && gather(cache).size() == 1,
          "publication copies rather than consumes the pending list");
    const auto replacement = source(point(2));
    cache.stage({0, 0}, {replacement, replacement});
    cache.refresh({0, 0}, false, 7);
    const auto old = gather(cache);
    check(old.size() == 1 && old[0].position == first.position,
          "staging and a blocked refresh retain the old current list");
    cache.refresh({0, 0}, true, 2);
    const auto replaced = gather(cache);
    check(replaced.size() == 2 && replaced[0].position == replacement.position &&
              replaced[1].position == replacement.position,
          "an admitted replacement preserves duplicate positions and supplied order");
    cache.stage({0, 0}, {});
    cache.refresh({0, 0}, true, 0);
    check(gather(cache).size() == 2, "zero geometry does not clear current sources even with an empty pending list");
    cache.refresh({0, 0}, true, 1);
    check(gather(cache).empty(), "an admitted empty pending list clears the current list");
    cache.stage({0, 0}, {first});
    cache.refresh({0, 0}, true, 1);
    cache.stage({0, 0}, {replacement});
    cache.reset({0, 0});
    check(!cache.refresh({0, 0}, true, 1) && gather(cache).empty(), "reset removes current and pending state together");
}

static void arithmetic() {
    CachedSources cache;
    auto zero = source(point(2));
    zero.colour = glm::vec3(0);
    cache.stage({0, 0}, {source(point(1), 14, 0x100), source(point(8)),
                         source(point(1), 13), zero, source(point(3))});
    cache.refresh({0, 0}, true, 1);
    const auto lights = gather(cache, 4, 1);
    check(lights.size() == 3, "flag-clear and distance-rejected nodes do not contribute; zero RGB still contributes");
    if (lights.size() == 3) {
        check(std::bit_cast<uint32_t>(lights[0].radius) == 0x416fe4cf &&
                  std::bit_cast<uint32_t>(lights[1].radius) == 0x416800b9 &&
                  std::bit_cast<uint32_t>(lights[2].radius) == 0x4170518e,
              "only admitted sources advance the deterministic 33/66/99 cosine phase");
        check(lights[0].colour == glm::vec3(.5f, .25f, .125f) && lights[1].colour == glm::vec3(0) &&
                  lights[2].colour == glm::vec3(1, .5f, .25f),
              "only kind13 receives one-minus-half-day scaling; other RGB is copied unchanged");
    }
    const auto bright = gather(cache, 4, -2, 1000), inverted = gather(cache, 4, 3, 16777217);
    check(bright.size() == 3 && bright[0].colour == glm::vec3(2, 1, .5f) &&
              std::bit_cast<uint32_t>(bright[0].radius) == 0x417470df,
          "day scaling is not clamped and clock conversion precedes the phase addition");
    check(inverted.size() == 3 && inverted[0].colour == glm::vec3(-.5f, -.25f, -.125f) &&
              std::bit_cast<uint32_t>(inverted[0].radius) == 0x4177efb3,
          "negative day-scaled RGB survives and a large clock narrows to float before multiplication");
    cache.stage({0, 0}, {source(point(1))});
    cache.refresh({0, 0}, true, 1);
    cache.stage({1, 0}, {source(point(40, 8))});
    cache.refresh({1, 0}, true, 1);
    cache.stage({0, 1}, {source(point(8, 40))});
    cache.refresh({0, 1}, true, 1);
    cache.stage({-1, 0}, {source(point(1))});
    cache.refresh({-1, 0}, true, 1);
    cache.stage({0x80000, 0}, {source(point(1))});
    cache.refresh({0x80000, 0}, true, 1);
    const auto ordered = gather(cache);
    check(ordered.size() == 3 && ordered[0].position == point(1) && ordered[1].position == point(8, 40) &&
              ordered[2].position == point(40, 8) && ordered[0].radius == ordered[1].radius &&
              ordered[1].radius == ordered[2].radius,
          "valid chunk keys visit X then Y, skip out-of-world keys and restart their admitted phase");
    std::vector<Candidate> prefixed{{point(99), {}, 1, 0}};
    cache.append(prefixed, ViewAdmission({}, {}, {}, 100), 0, 0);
    check(prefixed.size() == 4 && prefixed.front().position == point(99), "cached collection appends without replacing preceding records");
}

int main() {
    membership();
    publication();
    arithmetic();
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
