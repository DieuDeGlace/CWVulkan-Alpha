#include "TerrainLights.h"
#include "creatures/Spirits.h"
#include <bit>
#include <cstdio>
#include <limits>

using namespace terrain_lights;
static int failures = 0;
static void check(bool ok, const char *message) {
    if (!ok) { ++failures; std::printf("FAIL %s\n", message); }
}
static FixedPosition point(int64_t x, int64_t y, int64_t z = 0) {
    return {x * 65536, y * 65536, z * 65536};
}
static Candidate light(int64_t x, int64_t y, float radius, glm::vec3 colour = {1, 1, 1}) {
    Candidate result;
    result.position = point(x, y);
    result.colour = colour;
    result.radius = radius;
    return result;
}
int main() {
    FadingSources timed;
    const FixedPosition precise{(int64_t(1) << 54) + 1, 17, -33};
    timed.append({precise, {1, .5f, .25f}, 20, 0, 400});
    timed.append({point(16, 16), {0, 0, 1}, 16, 0, 800});
    timed.advance(400);
    check(timed.entries().size() == 2 && timed.entries()[0].ageMs == 400 &&
              timed.entries()[0].fadedColour() == glm::vec3(0) &&
              timed.entries()[1].fadedColour() == glm::vec3(0, 0, .5f),
          "duration equality preserves a zero-colour source before later sources");
    check(timed.entries()[0].position == precise && timed.entries()[0].radius == 20,
          "fading retains complete fixed coordinates and the original radius");
    timed.advance(1);
    check(timed.entries().size() == 1 && timed.entries()[0].ageMs == 401 &&
              timed.entries()[0].position == point(16, 16),
          "strictly expired sources are removed while survivor order is retained");
    timed.append({point(16, 16), {0, 0, 1}, 16, 0, 800});
    timed.append({point(16, 16), {0, 0, 1}, 16, 0, 800});
    check(timed.entries().size() == 3 && timed.entries()[1].ageMs == 0 && timed.entries()[2].ageMs == 0,
          "later producer appends do not merge coincident identities or inherit old age");
    timed.advance(399);
    check(timed.entries().size() == 3 && timed.entries()[0].ageMs == 800,
          "mixed source durations retain the later equality boundary");
    timed.advance(1);
    check(timed.entries().size() == 2 && timed.entries()[0].ageMs == 400 && timed.entries()[1].ageMs == 400,
          "deferred expiry keeps both younger duplicates in append order");
    check(FadingSource{{}, {1, .5f, .25f}, 20, -200, 400}.fadedColour() == glm::vec3(1.5f, .75f, .375f),
          "fading has a lower-only clamp rather than an invented upper cap");
    check(FadingSource{{}, {1, 1, 1}, 20, 401, 400}.fadedColour() == glm::vec3(0),
          "render evaluation clamps an ordinary negative fade");
    FadingSources largeAge;
    largeAge.append({{}, {1, 1, 1}, 20, 16777216, 16777217});
    check(largeAge.entries()[0].fadedColour() == glm::vec3(0),
          "age and duration narrow to float separately before division");
    largeAge.advance(1);
    check(largeAge.entries().size() == 1 && largeAge.entries()[0].ageMs == 16777217,
          "integer ages advance beyond binary32's unit precision and preserve equality");
    largeAge.advance(1);
    check(largeAge.entries().empty(), "integer expiry does not stall at the float precision boundary");

    creatures::Item glowing;
    glowing.spirits = 3;
    glowing.cubes = {{0, 0, 0, 128}, {0, 0, 0, 132}, {0, 0, 0, 255}, {0, 0, 0, 130}};
    glm::vec3 average(9);
    check(creatures::averageSpiritColour(glowing, average) &&
              std::bit_cast<uint32_t>(average.r) == 0x3f555556 &&
              std::bit_cast<uint32_t>(average.g) == 0x3f155556 &&
              std::bit_cast<uint32_t>(average.b) == 0x3ec44444,
          "all material bytes >=128 qualify; three ordered sums use one float reciprocal");
    glowing.spirits = 1;
    check(creatures::averageSpiritColour(glowing, average) && average == glm::vec3(1.5f, .75f, .15f),
          "only the descriptor's signed-count prefix contributes to item glow");
    glowing.spirits = -1;
    check(!creatures::averageSpiritColour(glowing, average) && average == glm::vec3(0),
          "nonpositive descriptor count writes zero even with retained decoded storage");
    glowing.spirits = 1;
    glowing.cubes[0].material = 127;
    check(!creatures::averageSpiritColour(glowing, average) && average == glm::vec3(0),
          "ordinary materials below128 do not qualify and leave zero sums");

    const auto fixed = fromBlocks({1.0 / 65536, -1.0 / 65536, .5});
    check(fixed && *fixed == FixedPosition{1, -1, 32768}, "world coordinates retain fixed-point fractions");
    check(!fromBlocks({std::numeric_limits<double>::infinity(), 0, 0}),
          "invalid engine coordinates fail conversion without an overflowing cast");
    check(priority({200, 200, 0}, {}) == 0.f, "each squared component divides before priority addition");
    const FixedPosition reference = point(8396928, 8396928);
    const FixedPosition nearReference{reference[0] + 512, reference[1] + 512, 0};
    check(priority(nearReference, reference) == 1.f / 8192.f,
          "priority subtracts complete large fixed coordinates before narrowing");

    std::vector<Candidate> small(5);
    const float keys[] = {2, 1, 1, 0, 1};
    for (size_t i = 0; i < small.size(); ++i) {
        small[i].position[2] = int64_t(i);
        small[i].priority = keys[i];
    }
    order(small);
    const int64_t expected[] = {3, 1, 2, 4, 0};
    bool stable = true;
    for (size_t i = 0; i < small.size(); ++i) stable &= small[i].position[2] == expected[i];
    check(stable, "small finite-key ties retain source order without a secondary key");
    std::vector<Candidate> partition(33);
    for (size_t i = 0; i < partition.size(); ++i) {
        partition[i].position[2] = int64_t(i);
        partition[i].priority = i == 16 ? 0.f : 1.f;
    }
    order(partition);
    bool witness = true;
    for (size_t i = 0; i < partition.size(); ++i)
        witness &= partition[i].position[2] == int64_t(i == 0 ? 16 : i == 16 ? 0 : i);
    check(witness, "the 33-record native tie witness changes the retained prefix");
    std::vector<Candidate> equal(96);
    for (size_t i = 0; i < equal.size(); ++i) {
        equal[i].position[2] = int64_t(i);
        equal[i].priority = std::bit_cast<float>(i & 1 ? 0x80000000u : 0u);
    }
    order(equal);
    bool unchanged = true;
    for (size_t i = 0; i < equal.size(); ++i) unchanged &= equal[i].position[2] == int64_t(i);
    check(unchanged, "all-equal signed-zero keys retain their sequence above the partition threshold");

    Selector selector;
    const std::array<Chunk, 3> chunks{{{{0, 0}, 7}, {{1, 0}, 8}, {{1, 1}, 9}}};
    std::vector<Candidate> candidates{light(31, 31, 1, {.3f, .5f, .7f})};
    candidates[0].position[2] = 10000 * 65536ll;
    const auto &corner = selector.rebuild(candidates, chunks, {-32768, 65536, 0});
    check(corner[0].count == 1 && corner[1].count == 1 && corner[2].count == 1,
          "inclusive XY rectangles admit corner chunks without a sphere or vertical test");
    check(corner[0].positions[0] == glm::vec4(30.5f, 32.f, 10000.f, 1.f) &&
              corner[1].positions[0] == corner[0].positions[0] &&
              corner[0].colours[0] == glm::vec4(.3f, .5f, .7f, 0),
          "full shifts precede narrowing and the selected position is independent of receiving chunk");

    candidates.clear();
    for (int i = 0; i < 19; ++i) {
        auto candidate = light(16, 16, 0, glm::vec3(float(i)));
        candidate.position[2] = i * 65536ll;
        candidates.push_back(candidate);
    }
    candidates.push_back(light(48, 16, 1, {0, 0, 1}));
    const auto &capped = selector.rebuild(candidates, chunks, {});
    check(capped[0].count == 16 && capped[1].count == 1 && capped[2].count == 0 &&
              capped[0].positions[15].z == 15.f,
          "each chunk retains its first sixteen qualifying candidates rather than a global prefix");
    check(capped[0].positions[0].w == 0.f && capped[0].colours[0] == glm::vec4(0),
          "zero radius and zero colour still occupy selected slots");
    const auto uploaded = shaderPrefix(capped[0]);
    check(uploaded.positions.size() == 10 && uploaded.positions[9].z == 9.f &&
              uploaded.colours[9] == glm::vec4(9, 9, 9, 0),
          "the shader upload copies exactly the first ten resident slots");

    candidates = {light(-1, 0, 0)};
    const auto &negative = selector.rebuild(candidates, chunks, {});
    check(negative[0].count == 1, "negative endpoints use two truncating divisions before world bounds");
    const auto &cleared = selector.rebuild({}, chunks, {});
    bool allZero = true;
    for (const auto &selection : cleared) {
        allZero &= selection.count == 0;
        for (size_t i = 0; i < CAPACITY; ++i)
            allZero &= selection.positions[i] == glm::vec4(0) && selection.colours[i] == glm::vec4(0);
    }
    check(allZero, "rebuild clears every unused position, radius and colour after lights disappear");
    const std::array<Chunk, 1> replaced{{{{0, 0}, 42}}};
    const auto &changed = selector.rebuild(candidates, replaced, {});
    check(changed.size() == 1 && changed[0].mesh == 42 && changed[0].count == 1,
          "geometry changes refresh cached chunk-to-selection indices");
    const std::array<Chunk, 2> outside{{{{-1, 0}, 1}, {{0x80000, 0}, 2}}};
    const auto &rejected = selector.rebuild(candidates, outside, {});
    check(rejected[0].count == 0 && rejected[1].count == 0, "native chunk bounds reject both exterior edges");
    check(selector.rebuild(candidates, {}, {}).empty(), "unloading all geometry removes its light selections");
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
