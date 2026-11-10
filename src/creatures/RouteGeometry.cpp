#include "creatures/RouteGeometry.h"
#include <bit>
#include <cmath>
#include <immintrin.h>

namespace creatures::route {
namespace {
int64_t add(int64_t a, int64_t b) { return std::bit_cast<int64_t>(uint64_t(a) + uint64_t(b)); }
int64_t subtract(int64_t a, int64_t b) { return std::bit_cast<int64_t>(uint64_t(a) - uint64_t(b)); }
float distanceComponent(int64_t a, int64_t b) { return float(subtract(a, b)) * (1.f / 65536.f); }
int64_t fixed(float value) { return int64_t(value * 65536.f); }
int64_t halfSize(float value) {
    const float half = value * .5f;
    return fixed(half);
}
int32_t grid(int64_t value, bool negativeExtra) {
    const uint32_t quotient = uint32_t(value / 65536);
    return std::bit_cast<int32_t>(quotient - uint32_t(negativeExtra && value < 0));
}
float squared(const Dimensions &value) {
    const float x2 = value[0] * value[0];
    const float y2 = value[1] * value[1];
    const float z2 = value[2] * value[2];
    const float horizontal = y2 + x2;
    return horizontal + z2;
}
float squareRoot(float value, const Scene &scene) {
    return scene.squareRoot ? scene.squareRoot(value) : float(std::sqrt(double(value)));
}
bool skippedSelector(int32_t selector) { return selector == 7 || selector == 6 || selector == 9; }
bool activeState(const Record &record) {
    switch (record.selector) {
    case 1: case 8: case 2: case 3: case 5: return record.state != 0;
    default: return true;
    }
}
Dimensions dimensions(const Record &record) {
    return uint32_t(record.orientation) & 1 ?
        Dimensions{record.size[1], record.size[0], record.size[2]} : record.size;
}
bool overlap(const Subject &subject, const Position &center, const Record &record, const Dimensions &size) {
    for (int axis = 0; axis < 2; axis++) {
        const int64_t recordHalf = halfSize(size[size_t(axis)]);
        const int64_t actorHalf = halfSize(subject.size[size_t(axis)]);
        if (subtract(record.position[size_t(axis)], recordHalf) > add(center[size_t(axis)], actorHalf) ||
            subtract(center[size_t(axis)], actorHalf) >= add(record.position[size_t(axis)], recordHalf))
            return false;
    }
    const int64_t actorHalf = halfSize(subject.size[2]);
    if (record.position[2] > add(center[2], actorHalf))
        return false;
    const int64_t recordHeight = fixed(size[2]);
    return subtract(center[2], actorHalf) < add(record.position[2], recordHeight);
}

uint8_t cell(const Scene &scene, int32_t x, int32_t y, int32_t z) {
    if (x < 0 || y < 0 || x >= 0x1000000 || y >= 0x1000000)
        return 1;
    const auto column = scene.column(x, y);
    if (!column || z < column->base)
        return 1;
    if (z >= column->base + column->count)
        return z > 0 ? 0 : 0x82;
    const size_t index = size_t(z - column->base);
    const uint8_t value = std::to_integer<uint8_t>(column->cells[index * 4 + 3]);
    if ((value & 0x1f) == 0 && z <= 0 && !(value & 0x40))
        return 0x82;
    return value;
}
bool blocking(uint8_t value) { return (value & 0x1f) != 0 && (value & 0x1f) != 2; }

using Cell = std::array<int32_t, 3>;
struct Bounds { Cell lower{}, upper{}; };
Bounds bounds(const Subject &subject, const Position &center, bool raised) {
    Bounds result;
    for (const bool upper : {false, true}) {
        Position half{}, offset{}, corner{};
        for (size_t axis = 0; axis < 3; axis++)
            half[axis] = halfSize(subject.size[axis]);
        if (raised)
            for (size_t axis = 0; axis < 3; axis++)
                offset[axis] = fixed(axis == 2 ? std::bit_cast<float>(uint32_t(0x3f8147ae)) : 0.f);
        for (size_t axis = 0; axis < 3; axis++) {
            const int64_t shifted = add(center[axis], offset[axis]);
            corner[axis] = upper ? add(shifted, half[axis]) : subtract(shifted, half[axis]);
        }
        Cell &output = upper ? result.upper : result.lower;
        if (raised) {
            for (int axis = 2; axis >= 0; axis--)
                output[size_t(axis)] = grid(corner[size_t(axis)], true);
        } else {
            for (size_t axis = 0; axis < 3; axis++)
                output[axis] = grid(corner[axis], false);
        }
    }
    return result;
}
bool terrainBlocked(const Subject &subject, const Position &center, bool raised, const Scene &scene) {
    const Bounds box = bounds(subject, center, raised);
    for (int32_t x = box.lower[0]; x <= box.upper[0]; x++)
        for (int32_t y = box.lower[1]; y <= box.upper[1]; y++) {
            bool blocked = false;
            for (int32_t z = box.lower[2]; z <= box.upper[2]; z++) {
                if (!raised && x != box.lower[0] && x != box.upper[0] &&
                    y != box.lower[1] && y != box.upper[1] && z != box.lower[2] && z != box.upper[2])
                    continue;
                if (blocking(cell(scene, x, y, z))) {
                    if (!raised)
                        return true;
                    blocked = true; // raised scans retain the rest of this Z run
                }
            }
            if (blocked)
                return true;
        }
    return false;
}
bool closeEnough(const Subject &subject, const Position &position, std::span<const int64_t, 3> target,
                 float radius, float radiusSquared, bool equality) {
    const float x = distanceComponent(position[0], target[0]);
    const float y = distanceComponent(position[1], target[1]);
    const float y2 = y * y;
    const float x2 = x * x;
    const float horizontal = y2 + x2;
    if (!(equality ? radiusSquared >= horizontal : radiusSquared > horizontal))
        return false;
    const float z = distanceComponent(position[2], target[2]);
    const float height = subject.size[2] + radius;
    return std::abs(double(z)) < double(height);
}
void raisePosition(const Subject &subject, Position &position) {
    const int64_t half = halfSize(subject.size[2]);
    int64_t feet = subtract(position[2], half);
    if (feet < 0)
        feet = subtract(feet, 65536);
    const int32_t level = std::bit_cast<int32_t>(uint32_t(feet / 65536));
    const int64_t base = std::bit_cast<int64_t>(uint64_t(int64_t(level)) << 16);
    // Closed truncations of binary64 -66191.36 and binary32 66191.359375.
    position[2] = add(add(subtract(base, -66191), half), 66191);
}
} // namespace

bool endpointOverlap(const Subject &subject, const Position &center, const Record &record) {
    return !skippedSelector(record.selector) && activeState(record) && overlap(subject, center, record, dimensions(record));
}
bool recordBlocks(const Subject &subject, const Position &center, int axis,
                  const Dimensions &direction, const Record &record) {
    if (skippedSelector(record.selector) || (subject.excluded && subject.excluded(record.identity)) || !activeState(record))
        return false;
    const Dimensions size = dimensions(record);
    const float raw = float(subtract(center[size_t(axis)], record.position[size_t(axis)]));
    const int64_t directional = int64_t(raw * direction[size_t(axis)]);
    const int64_t zero = fixed(0.f);
    return directional < zero && overlap(subject, center, record, size);
}

int32_t publicationCell(int64_t raw) {
    const double divided = double(raw) / 8.0;
    const int64_t truncated = int64_t(divided);
    return std::bit_cast<int32_t>(uint32_t(truncated / 65536));
}

bool scanRecords(const Subject &subject, const Position &center, int axis, const Dimensions &direction,
                 bool terrainBlocked, const RecordLookup &lookup) {
    const int32_t cx = grid(center[0], false) / 8;
    const int32_t cy = grid(center[1], false) / 8;
    for (int32_t x = cx - 1; x <= cx + 1; x++)
        for (int32_t y = cy - 1; y <= cy + 1; y++) {
            bool found = false;
            if (x >= 0 && y >= 0 && x < 0x200000 && y < 0x200000 && lookup)
                lookup(x, y, [&](const Record &record) {
                    found = recordBlocks(subject, center, axis, direction, record);
                    return !found;
                });
            if (found || terrainBlocked)
                return true; // retained terrain is tested at bucket completion
        }
    return terrainBlocked;
}

void endpointExclusions(const Subject &subject, std::span<const int64_t, 3> endpoint,
                        const RecordLookup &lookup, const std::function<void(uintptr_t)> &insert) {
    Position center;
    for (int axis = 2; axis >= 0; axis--) {
        const size_t index = size_t(axis);
        const int32_t coordinate = grid(endpoint[index], true);
        center[index] = std::bit_cast<int64_t>(uint64_t(int64_t(coordinate)) << 16);
    }
    center[0] = add(center[0], fixed(.5f));
    center[1] = add(center[1], fixed(.5f));
    center[2] = add(center[2], halfSize(subject.size[2]));
    const int32_t cx = grid(center[0], false) / 8;
    const int32_t cy = grid(center[1], false) / 8;
    for (int32_t x = cx - 1; x <= cx + 1; x++)
        for (int32_t y = cy - 1; y <= cy + 1; y++) {
            if (x < 0 || y < 0 || x >= 0x200000 || y >= 0x200000 || !lookup)
                continue;
            lookup(x, y, [&](const Record &record) {
                if (endpointOverlap(subject, center, record))
                    insert(record.identity);
                return true;
            });
        }
}

bool direct(const Subject &subject, std::span<const int64_t, 3> target, float radius, const Scene &scene) {
    Position position{subject.position[0], subject.position[1], subject.position[2]};
    const float radiusSquared = radius * radius;
    if (closeEnough(subject, position, target, radius, radiusSquared, false))
        return true;
    Dimensions budgetDirection{distanceComponent(target[0], position[0]), distanceComponent(target[1], position[1]), 0};
    const float lengthSquared = squared(budgetDirection);
    if (lengthSquared > 2500.f) {
        const float reciprocal = 1.f / squareRoot(lengthSquared, scene);
        for (int axis = 0; axis < 2; axis++) {
            const size_t index = size_t(axis);
            const float normalized = reciprocal * budgetDirection[index];
            budgetDirection[index] = normalized * 50.f;
        }
    }
    const float by2 = budgetDirection[1] * budgetDirection[1];
    const float bx2 = budgetDirection[0] * budgetDirection[0];
    const float horizontal = by2 + bx2;
    const float budgetSquared = horizontal + .5625f;
    const float budgetValue = 1.f + squareRoot(budgetSquared, scene);
    const int32_t budget = _mm_cvttss_si32(_mm_set_ss(budgetValue));
    for (int32_t iteration = 0; iteration < budget; iteration++) {
        if (closeEnough(subject, position, target, radius, radiusSquared, true))
            return true;
        Dimensions direction{distanceComponent(target[0], position[0]), distanceComponent(target[1], position[1]), 0};
        const float amount = squared(direction);
        if (amount > 0) {
            const float reciprocal = 1.f / squareRoot(amount, scene);
            direction[0] *= reciprocal;
            direction[1] *= reciprocal;
        }
        direction[2] = std::bit_cast<float>(uint32_t(0xbf35c28f));
        for (int axis = 0; axis < 3; axis++) {
            const size_t index = size_t(axis);
            const int64_t increment = fixed(direction[index]);
            position[index] = add(position[index], increment);
            const bool terrain = terrainBlocked(subject, position, false, scene);
            const bool record = (!terrain || axis == 2) && scene.spatial(position, axis, direction, terrain);
            if (!terrain && !record)
                continue;
            if (axis != 2 && !record && !(subject.flags & 0x0100) && !terrainBlocked(subject, position, true, scene)) {
                raisePosition(subject, position);
                continue;
            }
            position[index] = subtract(position[index], increment);
        }
    }
    const Dimensions progress{distanceComponent(position[0], subject.position[0]),
                              distanceComponent(position[1], subject.position[1]),
                              distanceComponent(position[2], subject.position[2])};
    return squared(progress) > 625.f;
}

} // namespace creatures::route
