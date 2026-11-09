#include "SectorLights.h"
#include "Frame.h"
#include "WorldItems.h"
#include "worldgen/Zone.h"
#include <cmath>
#include <cstdio>
#include <limits>

using namespace terrain_lights;
static int failures = 0;
static void check(bool ok, const char *message) {
    if (!ok) { ++failures; std::printf("FAIL %s\n", message); }
}
static FixedPosition point(int64_t x, int64_t y, int64_t z = 0) { return {x * 65536, y * 65536, z * 65536}; }

static void admission() {
    const std::array<glm::vec4, 6> noPlanes{};
    ViewAdmission unit({}, {}, noPlanes, 1);
    check(unit.accepts(point(1, 0), 0) && unit.accepts({65536, 255, 0}, 0),
          "fixed squared-distance equality and per-component truncation survive");
    check(!unit.accepts({65536, 256, 0}, 100) && !unit.accepts(point(2, 0), 100),
          "culling radius does not enlarge the quantized distance limit");
    check(ViewAdmission({}, {}, noPlanes, 0).accepts({200, 200, 0}, 0),
          "distance sums separately quantized products rather than floating Euclidean distance");
    check(!ViewAdmission({}, {}, noPlanes, std::numeric_limits<float>::infinity()).accepts({}, 0),
          "invalid engine distance bounds fail without an overflowing cast");

    std::array<glm::vec4, 6> planes{{{1, 0, 0, 1}, {-1, 0, 0, 1}, {0, 1, 0, 1},
                                     {0, -1, 0, 1}, {0, 0, 1, 1}, {0, 0, -1, 1}}};
    const ViewAdmission box({}, {}, planes, 10);
    check(box.accepts(point(2, 0), 1) && !box.accepts({2 * 65536 + 1, 0, 0}, 1),
          "plane equality survives while the next fixed-point unit fails");
    check(!box.accepts(point(0, 0, 3), 1), "the last supplied plane participates in admission");
    planes = {};
    planes[0] = {2, 0, 0, 0};
    check(!ViewAdmission({}, {}, planes, 10).accepts(point(-1, 0), 1),
          "the supplied radius is added directly without an invented plane-normal factor");
    planes[0] = {1, 1, 1, -.5f};
    const auto cancellation = point(0, 1, -16777216);
    check(!ViewAdmission(cancellation, point(16777216, 0), planes, 0).accepts(cancellation, 0),
          "plane products retain XY-then-Z-then-constant grouping");
    const FixedPosition precise{(int64_t(1) << 54) + 1, 0, 0};
    planes[0] = {1, 0, 0, -1.f / 131072};
    check(ViewAdmission(precise, {-(int64_t(1) << 54), 0, 0}, planes, 0).accepts(precise, 0),
          "full shifted fixed coordinates precede float narrowing for plane tests");
    planes[0] = {0, 0, 1, -10};
    check(!ViewAdmission(point(0, 0, 5), point(0, 0, 100), planes, 0).accepts(point(0, 0, 5), 0),
          "plane conversion applies horizontal shifts only and leaves raw Z unchanged");

    Camera camera;
    camera.aspect = 1;
    const auto cameraPlanes = camera.lightAdmissionPlanes();
    check(glm::length(glm::vec3(cameraPlanes[0])) > 2.f &&
              glm::length(glm::vec3(cameraPlanes[5])) < .001f,
          "camera adapter preserves unequal raw plane scales");
    const ViewAdmission cameraView({}, {}, cameraPlanes, 100);
    check(cameraView.accepts(point(0, 10), 0) && !cameraView.accepts(point(0, -10), 0),
          "engine Vulkan camera adapter admits in front and rejects behind");
    const auto belowNear = fromBlocks({0, .075, 0});
    check(belowNear && cameraView.accepts(*belowNear, 0),
          "source lower-depth plane uses w+z rather than the Vulkan geometry near plane");
}

static void planeProduction() {
    glm::mat4 projection(1), view(1);
    projection[0][0] = 2;
    projection[1][1] = 3;
    projection[2][2] = 4;
    view[3][0] = 5;
    const auto scaled = admissionPlanes(projection, view);
    check(scaled[0] == glm::vec4(2, 0, 0, 11) && scaled[1] == glm::vec4(-2, 0, 0, -9) &&
              scaled[2] == glm::vec4(0, 3, 0, 1) && scaled[3] == glm::vec4(0, -3, 0, 1) &&
              scaled[4] == glm::vec4(0, 0, 4, 1) && scaled[5] == glm::vec4(0, 0, -4, 1),
          "P*V column-vector product retains raw row3 +/- rows0/1/2 in consumer order");
    projection = glm::mat4(0);
    for (int column = 0; column < 4; ++column)
        projection[column][0] = 1;
    view = glm::mat4(0);
    view[0] = {16777216, 1, -16777216, -1};
    const auto cancellation = admissionPlanes(projection, view);
    check(cancellation[0].x == -1 && cancellation[1].x == 1,
          "four products accumulate sequentially rather than as paired sums");
    projection = glm::mat4(1);
    view = glm::mat4(1);
    view[0][0] = std::numeric_limits<float>::infinity();
    check(std::isnan(admissionPlanes(projection, view)[0].x),
          "the complete product retains structural-zero multiplications");
    std::array<glm::vec4, 6> unordered{};
    unordered[0].w = std::numeric_limits<float>::quiet_NaN();
    check(ViewAdmission({}, {}, unordered, 1).accepts({}, 0),
          "unordered plane results do not take the ordered-negative rejection branch");
    unordered[0].w = -0.f;
    check(ViewAdmission({}, {}, unordered, 1).accepts({}, 0), "either sign of plane equality survives");
}

static void sourceRules() {
    const auto negative = sectorRange(point(-1, -1), 0);
    check(negative.first == glm::ivec2(0) && negative.last == glm::ivec2(0),
          "negative sector endpoints truncate toward zero");
    const auto fractional = sectorRange({256 * 65536 - 1, 0, 0}, .5f);
    check(fractional.first.x == 0 && fractional.last.x == 0,
          "controlled fixed position discards its fraction before endpoint offsets");
    const auto wrapped = sectorRange({((int64_t(1) << 32) + 512) * 65536, 0, 0}, 0);
    check(wrapped.first.x == 2 && wrapped.last.x == 2, "column reduction keeps the low signed32 bits before float conversion");
    const auto inclusive = sectorRange(point(256, 256), 256);
    check(inclusive.contains(0, 0) && inclusive.contains(2, 2) && !inclusive.contains(3, 2),
          "sector rectangle includes both endpoint sectors");
    check(!sectorRange({}, std::numeric_limits<float>::infinity()).contains(0, 0),
          "invalid engine endpoint conversion produces an empty range");

    std::array<glm::vec4, 6> plane{};
    plane[0] = {0, 0, 1, 0};
    const ViewAdmission view({}, {}, plane, 100);
    const auto lamp = staticSource(50, point(4, 4, -40), 50, 0, view);
    const auto torch = staticSource(51, point(4, 4, -40), 50, 0, view);
    const auto fire = staticSource(65, point(4, 4, -25), 0, 1, view);
    check(lamp && lamp->position == point(4, 4, 10) && lamp->radius == 20,
          "kind50 shifts its position before the view test");
    check(!torch, "kind51 rejects its base before applying the height offset");
    check(fire && fire->radius == 20 && fire->colour == glm::vec3(0),
          "kind65 culls at radius32, emits at20, and retains zero daytime colour");
    check(!staticSource(49, {}, 1, 0, view), "other static kinds do not invent sector lights");
    const auto mushroom = itemSource(11, 19, {1, -2, 7}, 0);
    check(mushroom && mushroom->position == FixedPosition{1, -2, 7 + 3 * 65536} &&
              mushroom->radius == 16 && mushroom->colour == glm::vec3(.1f, .3f, .3f),
          "item light adds exactly three fixed world units without moving X/Y");
    const auto candleA = itemSource(18, 0, {}, 0), candleB = itemSource(18, 1, {}, 0);
    check(candleA && candleB && candleA->colour == glm::vec3(.6f, .6f, .2f) &&
              candleB->colour == glm::vec3(0, .6f, .2f), "both candle subtypes keep their specified RGB");
    check(!itemSource(18, 2, {}, 0) && !itemSource(11, 19, {}, -1) && !itemSource(11, 19, {}, 1),
          "unsupported items and either sign of nonzero hide timer reject emission");
}

static WorldItem item(int x, int y, uint8_t type = 18, uint8_t subtype = 0) {
    WorldItem result;
    result.item[0] = type;
    result.item[1] = subtype;
    result.position[0] = int64_t(x) * 65536;
    result.position[1] = int64_t(y) * 65536;
    return result;
}
static void collection() {
    SectorSources sources;
    WorldItems items;
    worldgen::Zone zone;
    worldgen::StaticObject lamp;
    lamp.kind = 50;
    lamp.x = lamp.y = 8 * 65536;
    lamp.scale[2] = 2;
    zone.statics.push_back(lamp);
    zone.items.push_back({8, 8, 0, 11, 19});
    sources.update(zone);
    items.generated(zone);
    const ViewAdmission view({}, {}, {}, 1000);
    std::vector<Candidate> collected{{point(0, 0, 99), {1, 0, 0}, 1, 0}};
    sources.append(collected, items, view, {}, 0);
    check(collected.size() == 3 && collected[0].position == point(0, 0, 99) &&
              collected[1].position == point(8, 8, 2) &&
              collected[2].position == FixedPosition{8 * 65536 + 32768, 8 * 65536 + 32768, 3 * 65536},
          "real generated items contribute once from live state, after earlier groups and statics");
    auto hidden = item(10, 10);
    hidden.hideMs = 1;
    items.replace(0, 0, {hidden});
    collected.clear();
    sources.append(collected, items, view, {}, 0);
    check(collected.size() == 1, "live item replacement/hiding cannot leave stale generated light");

    items.replace(0, 0, {item(10, 10), item(11, 10, 18, 1)});
    zone.zy = 1;
    zone.statics[0].x = 4 * 65536;
    zone.statics[0].y = 260 * 65536;
    sources.update(zone);
    items.replace(0, 1, {item(4, 260)});
    zone.zx = 1;
    zone.zy = 0;
    zone.statics.clear();
    sources.update(zone);
    items.replace(1, 0, {item(260, 4)});
    items.replace(0, 2, {item(2, 2)}); // no corresponding loaded sector
    collected.clear();
    sources.append(collected, items, view, {}, 1);
    check(collected.size() == 6 && collected[0].colour == glm::vec3(0) &&
              collected[1].position == point(10, 10, 3) && collected[2].position == point(11, 10, 3) &&
              collected[3].position == point(4, 260, 2) && collected[4].position == point(4, 260, 3) &&
              collected[5].position == point(260, 4, 3),
          "loaded sectors visit X then Y, static then live item, including zero-colour and empty-static sectors");
    sources.remove(0, 1);
    collected.clear();
    sources.append(collected, items, view, {}, 0);
    check(collected.size() == 4, "unloaded sectors suppress both static and retained item sources");
    collected.clear();
    sources.append(collected, items, view, point(4096, 4096), 0);
    check(collected.empty(), "controlled position determines sector gathering independently of the view reference");
}

int main() {
    admission();
    planeProduction();
    sourceRules();
    collection();
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
