#include "creatures/CameraFollow.h"
#include "creatures/CameraView.h"
#include "creatures/FollowCamera.h"
#include <bit>
#include <cmath>
#include <cstdio>
#include <limits>
#include <string>
#include <vector>

using camera_view::Fixed3;
using camera_view::Matrix;

static int failures = 0;
static void check(bool value, const char *message) {
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", message); ++failures; }
}
static uint32_t bits(float value) { return std::bit_cast<uint32_t>(value); }
static float fromBits(uint32_t value) { return std::bit_cast<float>(value); }
constexpr int64_t UNIT = 65536;

static camera_view::Trig fixedTrig(double cosine, double sine, std::string *order = nullptr, std::vector<double> *arguments = nullptr) {
    return {[=](double angle) { if (order) *order += 'c'; if (arguments) arguments->push_back(angle); return cosine; },
            [=](double angle) { if (order) *order += 's'; if (arguments) arguments->push_back(angle); return sine; }};
}
static bool rowIs(const Matrix &matrix, int row, float a, float b, float c, float d) {
    return matrix[row][0] == a && matrix[row][1] == b && matrix[row][2] == c && matrix[row][3] == d;
}

static void conversions() {
    const glm::vec3 narrowed = camera_view::toFloat({UNIT, -UNIT / 2, 16777217});
    check(narrowed.x == 1.f && narrowed.y == -.5f && narrowed.z == 256.f,
          "fixed components round to binary32 before the 1/65536 product");
    check(camera_view::toFixed(1.5f) == 98304 && camera_view::toFixed(-1.5f) == -98304 &&
          camera_view::toFixed(fromBits(0x3727c5ac)) == 0 && camera_view::toFixed(-fromBits(0x3727c5ac)) == 0,
          "float components scale in binary32 and truncate toward zero");
    const Fixed3 reversed = camera_view::opposite({UNIT, -3, int64_t(1) << 47});
    check(reversed[0] == -UNIT && reversed[1] == 3 && reversed[2] == -(int64_t(1) << 47),
          "ordinary opposite is exact negation");
    check(camera_view::opposite({int64_t(1) << 48, 0, 0})[0] == 0,
          "opposite keeps the wrapped product before signed division, not overflow-free negation");
    const int64_t lowest = std::numeric_limits<int64_t>::min();
    check(camera_view::negate(5) == -5 && camera_view::negate(lowest) == lowest, "raw negation is two's complement with wrap");
    const Fixed3 wrapped = camera_view::subtract({lowest, 0, 0}, {1, 0, 0});
    check(wrapped[0] == std::numeric_limits<int64_t>::max(), "fixed subtraction keeps the low 64 bits");
}

static void rotationsAndTranslations() {
    std::string order;
    std::vector<double> arguments;
    const auto quarter = fixedTrig(0, 1, &order, &arguments);
    Matrix matrix = camera_view::identity();
    matrix[3] = {5, 6, 7, 1};
    camera_view::rotateRows12(matrix, 90.f, quarter);
    check(rowIs(matrix, 0, 1, 0, 0, 0) && rowIs(matrix, 1, 0, 0, 1, 0) && rowIs(matrix, 2, 0, -1, 0, 0) && rowIs(matrix, 3, 5, 6, 7, 1),
          "component-zero rotation mixes rows one and two from saved values and retains the others");
    const double expected = double(90.f * fromBits(0x3c8efa35));
    check(order == "cs" && arguments.size() == 2 && arguments[0] == expected && arguments[1] == expected,
          "degrees take one binary32 factor, widen, then request cosine before sine");
    matrix = camera_view::identity();
    camera_view::rotateRows02(matrix, 90.f, quarter);
    check(rowIs(matrix, 0, 0, 0, -1, 0) && rowIs(matrix, 1, 0, 1, 0, 0) && rowIs(matrix, 2, 1, 0, 0, 0),
          "component-one rotation subtracts the sine term from row zero and adds it to row two");
    matrix = camera_view::identity();
    camera_view::rotateRows01(matrix, 90.f, quarter);
    check(rowIs(matrix, 0, 0, 1, 0, 0) && rowIs(matrix, 1, -1, 0, 0, 0) && rowIs(matrix, 2, 0, 0, 1, 0),
          "component-two rotation mixes rows zero and one");

    // Rounded products: 0x1.000002p0 * 0x1.fffffcp-1 rounds to one, so the sum
    // with -1 is exactly zero unless a fused or wider product is used.
    matrix = camera_view::identity();
    matrix[0] = {0x1.000002p0f, 0, 0, 0};
    matrix[1] = {-1.f, 0, 0, 0};
    camera_view::rotateRows01(matrix, 0.f, fixedTrig(0x1.fffffcp-1, 1));
    check(bits(matrix[0][0]) == 0, "rotation products round to binary32 before their sum");

    matrix = camera_view::identity();
    matrix[0][0] = 1; matrix[1][0] = 1; matrix[2][0] = 1; matrix[3][0] = 4;
    camera_view::translate(matrix, {1e20f, -1e20f, 3.f});
    check(matrix[3][0] == 7.f && rowIs(matrix, 0, 1, 0, 0, 0), "translation adds x and y terms, then z, then the old fourth-row value");
    matrix = camera_view::identity();
    matrix[2] = {9, 9, 9, 9};
    matrix[3] = {1, 2, 3, 4};
    camera_view::translate(matrix, 2.f, 3.f);
    check(rowIs(matrix, 3, 3, 5, 3, 4), "two-coordinate translation has no third-row contribution");
    matrix = camera_view::identity();
    matrix[0] = {1e20f, 0, 0, 0}; matrix[1] = {-1e20f, 0, 0, 0}; matrix[2] = {3, 4, 5, 0}; matrix[3] = {100, 100, 100, 1};
    const glm::vec3 turned = camera_view::transformDirection(matrix, {1, 1, 1});
    check(turned.x == 3.f && turned.y == 4.f && turned.z == 5.f, "direction transform adds the first two products before the third and no translation");
}

static void weightsAndShake() {
    check(bits(camera_view::weight(0, .5f)) == 0 && bits(camera_view::weight(-5, .5f)) == 0, "nonpositive counts give zero weight");
    check(camera_view::weight(1, .5f) == .5f && camera_view::weight(2, .5f) == .75f && camera_view::weight(3, .5f) == .875f,
          "weight is a count-fold approach to one");
    const float small = fromBits(0x3ba3d70a);
    double wide = 0;
    for (int i = 0; i < 16; ++i)
        wide = wide + (1.0 - wide) * double(small);
    check(bits(camera_view::weight(16, small)) == bits(float(wide)), "the approach stays binary64 and narrows only once");
    check(camera_view::weight(std::numeric_limits<int32_t>::max(), .5f) == 1.f, "a saturated approach ends at its fixed point");
    check(camera_view::blend(10.f, 20.f, .25f) == 12.5f, "blend adds the complement product and the target product");

    int reads = 0;
    std::vector<double> arguments;
    const camera_view::Trig trig{[&](double angle) { arguments.push_back(angle); return arguments.size() == 1 ? .5 : .25; },
                                 [](double) { return 0.; }};
    const auto clock = [&] { return ++reads == 1 ? 100 : 200; };
    const camera_view::Shake moved = camera_view::shake(2.f, clock, trig);
    check(reads == 2 && arguments.size() == 2 && arguments[0] == double(100.f * fromBits(0x3d23d70a)) &&
          arguments[1] == double(200.f * fromBits(0x3cf5c28f)),
          "shake reads the clock twice and passes raw clock products to cosine without a degree factor");
    check(moved.y == .5f && moved.x == .25f, "the first shake result is y and the second is x, both times half the amplitude");
    reads = 0; arguments.clear();
    const camera_view::Shake still = camera_view::shake(0.f, clock, trig);
    check(reads == 2 && arguments.size() == 2 && still.x == 0 && still.y == 0, "zero amplitude skips neither clock read nor cosine");
    check(camera_view::decayedAmplitude(1.f, 0) == 1.f && bits(camera_view::decayedAmplitude(1.f, 1)) == bits(1.f - small),
          "amplitude decays toward zero with the frame-duration weight");
}

static void finalConstruction() {
    std::string order;
    std::vector<double> arguments;
    camera_view::Inputs inputs;
    const int64_t far = int64_t(1) << 40;
    inputs.eye = {far + 3 * UNIT, 2 * UNIT, 7 * UNIT};
    inputs.anchor = {far + 5 * UNIT, 6 * UNIT, 99 * UNIT};
    inputs.angles = {0.f, 0.f, 0.f};
    int reads = 0;
    const auto views = camera_view::construct(inputs, [&] { return ++reads; }, fixedTrig(1, 0, &order, &arguments));
    check(order == "cscscs" "cscscs" "cc" "cscscs" && reads == 2, "the direct route requests eleven cosines and nine sines in order");
    bool companionPositive = true, viewsNegative = true;
    for (int i = 0; i < 6; ++i) companionPositive &= !std::signbit(arguments[i]);
    for (int i = 6; i < 12; ++i) viewsNegative &= std::signbit(arguments[i]);
    for (int i = 14; i < 20; ++i) viewsNegative &= std::signbit(arguments[i]);
    check(companionPositive && viewsNegative, "both views flip each angle's sign bit, including zero; the companion does not");
    const glm::vec3 eye = camera_view::toFloat(inputs.eye);
    check(rowIs(views.companion, 3, eye.x, eye.y, eye.z, 1), "the companion translates by the converted eye");
    const glm::vec3 away = camera_view::toFloat(camera_view::opposite(inputs.eye));
    check(rowIs(views.absolute, 3, away.x, away.y, away.z, 1), "the absolute view translates by the converted opposite eye");
    check(views.rebaseX == -inputs.anchor[0] && views.rebaseY == -inputs.anchor[1], "rebasing publishes the opposite anchor x and y");
    check(rowIs(views.rebased, 3, 2.f, 4.f, -7.f, 1),
          "the rebased view subtracts raw coordinates before conversion and does not rebase the vertical origin");

    inputs.amplitude = 2.f;
    const auto shaken = camera_view::construct(inputs, [] { return 0; }, fixedTrig(1, 0));
    check(rowIs(shaken.rebased, 3, 3.f, 5.f, -7.f, 1) && rowIs(shaken.absolute, 3, away.x, away.y, away.z, 1),
          "shake translates only the rebased view and survives its rotations");
}

static void inputTargets() {
    camera_follow::State state;
    state.targetAngles = {120.f, 3.5f, 0.f};
    camera_follow::mouseTargets(state, 50, 0, 4.f, 8.f);
    const float rate = 50.f * fromBits(0x3ba3d70a);
    check(bits(state.targetAngles[0]) == bits(120.f + rate * 8.f) && bits(state.targetAngles[2]) == bits(0.f - rate * 4.f) &&
          state.targetAngles[1] == 3.5f, "mouse input multiplies the sensitivity product first and never writes the second target");
    camera_follow::mouseTargets(state, 50, 7, 0.f, 8.f);
    check(state.targetAngles[0] == 120.f, "any nonzero direction discriminator subtracts");
    camera_follow::mouseTargets(state, 50, 0, 0.f, 1e6f);
    check(state.targetAngles[0] == 180.f, "the first target stops at 180");
    camera_follow::mouseTargets(state, 50, 0, 0.f, -1e6f);
    check(bits(state.targetAngles[0]) == 0, "a negative first target becomes positive zero");
    state.targetAngles[0] = -0.f;
    camera_follow::mouseTargets(state, 0, 0, 0.f, 0.f);
    check(bits(state.targetAngles[0]) == 0x80000000u || bits(state.targetAngles[0]) == 0, "zero-valued targets pass the strict bounds");

    check(camera_follow::wheelMessage(-120) == -1 && camera_follow::wheelMessage(360) == 1 && camera_follow::wheelMessage(0) == 0,
          "the message route keeps only the wheel sign");
    state.targetDistance = 6.f;
    camera_follow::wheel(state, 1, false);
    check(state.targetDistance == 4.f, "one notch moves the target distance by two");
    camera_follow::wheel(state, -9, false);
    check(state.targetDistance == 14.f, "the target distance stops at fourteen");
    camera_follow::wheel(state, 100, false);
    check(bits(state.targetDistance) == 0, "a negative target distance becomes positive zero");
    camera_follow::wheel(state, 0x40000000, false);
    check(state.targetDistance == 14.f, "the doubled argument wraps to a signed value before conversion");
    state.targetScale = 1.f;
    camera_follow::wheel(state, -1, true);
    check(bits(state.targetScale) == 0x3f666666, "a negative argument multiplies the target scale");
    state.targetScale = 1.f;
    camera_follow::wheel(state, 0, true);
    check(bits(state.targetScale) == bits(1.f / fromBits(0x3f666666)), "a zero argument still divides the target scale");
    state.targetScale = 10.f;
    camera_follow::wheel(state, 1, true);
    check(state.targetScale == 10.f, "the target scale stops at ten");
    state.targetScale = fromBits(0x3c23d70a);
    camera_follow::wheel(state, -1, true);
    check(bits(state.targetScale) == 0x3c23d70a, "the target scale stops at its lower bound");
}

static void smoothing() {
    camera_follow::State state;
    state.targetAngles = {10.f, 20.f, 30.f};
    state.distance = 6.f; state.targetDistance = 10.f;
    state.scale = 1.f; state.targetScale = 3.f;
    int calls = 0;
    const camera_follow::Power half = [&](double base, double exponent) {
        ++calls;
        check(base == double(fromBits(0x3ba3d70a)) && exponent == double(100.f * fromBits(0x3c23d70a)),
              "the power helper receives the widened binary32 base and option product");
        return .5;
    };
    camera_follow::smooth(state, 1, 100, half);
    const float slow = fromBits(0x3b83126f);
    check(calls == 1 && state.angles[0] == 5.f && state.angles[1] == 10.f && state.angles[2] == 15.f,
          "all three angles blend with the power result as their coefficient");
    check(bits(state.distance) == bits(camera_view::blend(6.f, 10.f, slow)) && bits(state.scale) == bits(camera_view::blend(1.f, 3.f, slow)),
          "distance and scale blend with their own coefficient and the same count");
    const camera_follow::State before = state;
    camera_follow::smooth(state, 0, 100, half);
    check(state.angles == before.angles && state.distance == before.distance && state.scale == before.scale,
          "a zero count leaves ordinary finite values unchanged");
    const glm::vec3 level = camera_follow::offsetDirection({90.f, 0.f, 0.f}, camera_view::platformTrig());
    const glm::vec3 turned = camera_follow::offsetDirection({90.f, 0.f, 90.f}, camera_view::platformTrig());
    const glm::vec3 down = camera_follow::offsetDirection({180.f, 0.f, 37.f}, camera_view::platformTrig());
    check(std::abs(level.x) < 1e-6f && std::abs(level.y + 1) < 1e-6f && std::abs(level.z) < 1e-6f &&
          std::abs(turned.x - 1) < 1e-6f && std::abs(turned.y) < 1e-6f && std::abs(down.z + 1) < 1e-6f,
          "the offset direction is the oriented third axis for the current angles");
}

static void anchorFollowing() {
    camera_follow::Subject subject;
    subject.position = {0, 0, 0};
    camera_follow::State state;
    state.anchor = {51 * UNIT, 0, 0};
    camera_follow::followAnchor(state, subject, 0);
    check(state.anchor == subject.position, "an anchor strictly beyond fifty units snaps to the whole creature position");
    state.anchor = {50 * UNIT, 0, 0};
    subject.extent = {0, 0, -1.f}; // cancels the ordinary half-unit so the target is zero
    camera_follow::followAnchor(state, subject, 0);
    check(state.anchor[0] == 50 * UNIT, "exactly fifty units does not snap, and a zero count does not move x or y");

    subject = {};
    subject.position = {10 * UNIT, -10 * UNIT, 0};
    state.anchor = {0, 0, 0};
    camera_follow::followAnchor(state, subject, 1);
    check(state.anchor[0] == 16380 && state.anchor[1] == -16380,
          "x and y add the raw difference times the truncated fixed weight, divided toward zero");
    state.anchor = {0, 0, 0};
    subject.position = {-1, 1, 0};
    camera_follow::followAnchor(state, subject, 1);
    check(state.anchor[0] == 0 && state.anchor[1] == 0, "small quantized steps truncate toward zero in both directions");

    subject = {};
    subject.position = {0, 0, 100 * UNIT};
    subject.verticalOffset = .25f;
    subject.extent = {2.f, 1.f, 3.f};
    check(camera_follow::verticalTarget(subject, false) == 6668288, "ordinary actions add half the third extent and one half");
    subject.action = 0x53;
    check(camera_follow::verticalTarget(subject, false) == 6602752, "actions 0x53 and 0x54 add half the first extent and no constant");
    subject.action = 0x54;
    check(camera_follow::verticalTarget(subject, false) == 6602752, "action 0x54 shares the first-extent target");
    subject.action = 0x6a;
    check(camera_follow::verticalTarget(subject, false) == 6733824, "actions 0x6a and 0x6b add one and a half");
    subject.action = 0x6b;
    check(camera_follow::verticalTarget(subject, false) == 6733824 && camera_follow::verticalTarget(subject, true) == 6537216,
          "the override keeps only the position minus its offset");

    subject.action = 0;
    state.anchor = {7, 9, 0};
    camera_follow::followAnchor(state, subject, std::numeric_limits<int32_t>::max());
    check(state.anchor[0] == 0 && state.anchor[1] == 0 && state.anchor[2] == 6668288, "a saturated count reaches the creature and its vertical target");
    state.anchor = {0, 0, (int64_t(1) << 40) + 1};
    subject.position = {0, 0, int64_t(1) << 40};
    subject.verticalOffset = 0; subject.extent = {0, 0, -1.f};
    camera_follow::followAnchor(state, subject, 0);
    check(state.anchor[2] == int64_t(1) << 40, "the vertical blend passes the old raw value through binary32 even at zero weight");

    camera_follow::Alternative alternative;
    alternative.anchor = {3.5f, -2.5f, 7.f};
    int seenX = 0, seenY = 0;
    alternative.column = [&](int32_t x, int32_t y) { seenX = x; seenY = y; return std::optional<camera_follow::Column>({10, 5}); };
    camera_follow::AnchorGates gates;
    gates.alternative = &alternative;
    state.anchor = {};
    subject = {};
    camera_follow::followAnchor(state, subject, 1000, gates);
    check(seenX == 3 && seenY == -2 && state.anchor[0] == 229376 && state.anchor[1] == -163840 && state.anchor[2] == 65 * UNIT,
          "the alternative anchor truncates its column coordinates toward zero and adds fifty above the column sum");
    alternative.column = [](int32_t, int32_t) { return std::optional<camera_follow::Column>(); };
    camera_follow::followAnchor(state, subject, 1000, gates);
    check(state.anchor[2] == 57 * UNIT, "a missing column keeps the converted vertical value before adding fifty");
}

static void voxelsAndEye() {
    using camera_follow::voxelCoordinate;
    check(voxelCoordinate(0) == 0 && voxelCoordinate(65535) == 0 && voxelCoordinate(65536) == 1 && voxelCoordinate(-1) == -1 &&
          voxelCoordinate(-65536) == -2 && voxelCoordinate(-65537) == -2,
          "negative raw coordinates subtract one unit before truncation, including exact integers");
    using camera_follow::blocks;
    check(!blocks(0) && !blocks(2) && blocks(1) && !blocks(0x20) && !blocks(0x22) && blocks(0x3f),
          "only the low five classification bits decide blocking");

    camera_follow::State state;
    state.anchor = {0, 10 * UNIT, 0};
    state.distance = 4.f; state.targetDistance = 9.f;
    const glm::vec3 direction(0, 1, 0);
    std::vector<Fixed3> samples;
    std::vector<float> maxima;
    camera_follow::Queries queries;
    bool corners = true;
    queries.blocked = [&](const Fixed3 &at) { if (corners) samples.push_back(at); return corners; };
    queries.ray = [&](const Fixed3 &, glm::vec3 along, float maximum) {
        check(along == direction, "corner rays use the supplied direction");
        maxima.push_back(maximum);
        if (maxima.size() == 8) corners = false; // the centre is free afterwards
        return maxima.size() == 4 ? 1. : maxima.size() == 6 ? 3. : maxima.size() == 7 ? -1. : 0.;
    };
    camera_follow::placeEye(state, {0, 0, 0}, direction, queries);
    bool ordered = samples.size() == 8, original = maxima.size() == 8;
    for (size_t i = 0; ordered && i < 8; ++i) {
        const int64_t q = UNIT / 4;
        ordered = samples[i] == Fixed3{i & 4 ? q : -q, 6 * UNIT + (i & 2 ? q : -q), i & 1 ? q : -q};
    }
    for (float maximum : maxima) original &= maximum == 4.f;
    check(ordered, "eight quarter-unit corners are sampled round the first eye with the third axis fastest");
    check(original, "every corner ray receives the original distance, with no early exit");
    check(state.distance == 1.f && state.retainedDistance == 1.f && state.targetDistance == 9.f && state.eye == Fixed3{0, 9 * UNIT, 0},
          "the shortest candidate is retained and the eye recomputed without changing the target distance");

    state.distance = 4.f;
    corners = true; samples.clear(); maxima.clear();
    queries.ray = [&](const Fixed3 &, glm::vec3, float maximum) { maxima.push_back(maximum); if (maxima.size() == 8) corners = false; return 10.; };
    camera_follow::placeEye(state, {0, 0, 0}, direction, queries);
    check(state.distance == -6.f && state.eye == Fixed3{0, 16 * UNIT, 0}, "the caller does not clamp a candidate below zero");

    state.distance = 4.f;
    std::vector<float> centre;
    glm::vec3 centreDirection(0);
    Fixed3 centreStart{9, 9, 9};
    double reached = 2.5;
    queries.blocked = [](const Fixed3 &at) { return at[0] == 0 && at[2] == 0; }; // only the centre line
    queries.ray = [&](const Fixed3 &start, glm::vec3 along, float maximum) {
        centreStart = start; centreDirection = along; centre.push_back(maximum);
        return reached;
    };
    const Fixed3 subject{0, 0, 0};
    camera_follow::placeEye(state, subject, direction, queries);
    check(centre.size() == 1 && centre[0] == 100.f && centreStart == subject && centreDirection == direction,
          "a blocked centre casts one ray from the creature along the normalized offset with maximum 100");
    check(state.eye == Fixed3{0, 163840, 0} && state.distance == 4.f && state.retainedDistance == 4.f,
          "the final correction moves only the eye");
    reached = -1;
    camera_follow::placeEye(state, subject, direction, queries);
    check(state.eye == Fixed3{0, 6 * UNIT, 0}, "a negative final result keeps the recomputed eye");
    reached = std::numeric_limits<double>::quiet_NaN();
    camera_follow::placeEye(state, subject, direction, queries);
    check(state.eye == Fixed3{0, 6 * UNIT, 0}, "an unordered final result keeps the recomputed eye");
    reached = 0;
    camera_follow::placeEye(state, subject, direction, queries);
    check(state.eye == subject, "a zero final result is admitted");
    centre.clear();
    state.anchor = {0, 4 * UNIT, 0};
    camera_follow::placeEye(state, subject, direction, queries);
    check(centre.empty() && state.eye == subject, "a zero squared offset reaches no final ray");
}

static void productionCamera() {
    creatures::Body body;
    body.position[0] = 5 * UNIT; body.position[1] = -3 * UNIT; body.position[2] = 20 * UNIT;
    const creatures::visibility::Scene open{[](int32_t, int32_t, int32_t) { return uint8_t(0); }, {}};
    creatures::FollowCamera camera;
    camera.update(body, 16, open, [] { return 0; });
    const glm::vec3 direction = camera.direction();
    const double pitch = glm::radians(double(camera.pitch)), yaw = glm::radians(double(camera.yaw));
    check(std::abs(direction.x - std::sin(yaw) * std::sin(pitch)) < 1e-6 && std::abs(direction.y + std::cos(yaw) * std::sin(pitch)) < 1e-6 &&
          std::abs(direction.z - std::cos(pitch)) < 1e-6, "production direction agrees with the oriented third axis");
    const glm::dvec3 head(5, -3, 20 + 2.16f * .5 + .5);
    const glm::dvec3 expected = head - glm::dvec3(direction) * double(camera.distance);
    check(glm::length(camera.position() - expected) < 1e-3 && std::abs(camera.distance - 6.f) < 1e-5f,
          "the first open-air frame sits the retained distance behind the head-height anchor");

    creatures::FollowCamera shaken = camera, steady = camera;
    shaken.shake = 1.f;
    shaken.update(body, 16, open, [] { return 0; });
    steady.update(body, 16, open, [] { return 0; });
    const glm::dvec3 moved = shaken.position() - steady.position();
    check(std::abs(glm::length(moved) - std::sqrt(.5)) < 1e-4 && std::abs(glm::dot(moved, glm::dvec3(direction))) < 1e-4,
          "shake displaces the engine eye across the view by both half-amplitude components");
    check(bits(shaken.shake) == bits(camera_view::decayedAmplitude(1.f, 16)), "the amplitude decays after the frame's view offset");

    const creatures::visibility::Scene solid{[](int32_t, int32_t, int32_t z) { return uint8_t(z >= 23 ? 1 : 0); }, {}};
    creatures::FollowCamera pressed;
    pressed.pitch = pressed.targetPitch = 170.f; // looking down, so the camera rises into the ceiling
    for (int i = 0; i < 4; ++i)
        pressed.update(body, 16, solid, [] { return 0; });
    check(pressed.distance >= 0.f && pressed.distance < 6.f && pressed.position().z < 23.3,
          "blocked corners shorten the production distance through the held ray query");
}

int main() {
    conversions(); rotationsAndTranslations(); weightsAndShake(); finalConstruction();
    inputTargets(); smoothing(); anchorFollowing(); voxelsAndEye(); productionCamera();
    if (!failures) std::puts("PASS: camera conversions, view construction, input targets, smoothing, anchor following, corner sampling and production adapter");
    return failures ? 1 : 0;
}
