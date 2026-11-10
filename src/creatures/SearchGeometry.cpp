#include "creatures/SearchGeometry.h"
#include <algorithm>
#include <bit>

namespace creatures::search {
namespace {
int64_t add(int64_t a, int64_t b) { return std::bit_cast<int64_t>(uint64_t(a) + uint64_t(b)); }
int64_t subtract(int64_t a, int64_t b) { return std::bit_cast<int64_t>(uint64_t(a) - uint64_t(b)); }
int64_t fixed(float value) { return int64_t(value * 65536.f); }
int64_t half(float value) { return fixed(value * .5f); }
bool blocking(uint8_t value) { return (value & 31) != 0 && (value & 31) != 2; }
int32_t rawCell(int64_t value) {
    if (value < 0)
        value = subtract(value, 65536);
    return std::bit_cast<int32_t>(uint32_t(value / 65536));
}
struct Bounds { Cell lower{}, upper{}; };
Bounds expandedBounds(const route::Position &center, std::span<const float, 3> size) {
    Bounds result;
    for (const bool upper : {false, true}) {
        route::Dimensions halves{};
        route::Position converted{}, corner{};
        for (size_t axis = 0; axis < 3; axis++)
            halves[axis] = size[axis] * .5f;
        for (size_t axis = 0; axis < 3; axis++)
            converted[axis] = fixed(halves[axis]);
        for (size_t axis = 0; axis < 3; axis++)
            corner[axis] = upper ? add(center[axis], converted[axis]) :
                add(subtract(center[axis], converted[axis]), axis == 2 ? 655 : 0);
        Cell &output = upper ? result.upper : result.lower;
        output = coordinate(corner);
        for (int32_t &component : output)
            component += upper ? 1 : -1;
    }
    return result;
}
void terrainFlags(Classification &flags, const Bounds &box, const Cell &at) {
    const bool xi = at[0] > box.lower[0] && at[0] < box.upper[0];
    const bool yi = at[1] > box.lower[1] && at[1] < box.upper[1];
    const bool zi = at[2] > box.lower[2] && at[2] < box.upper[2];
    const bool zl = at[2] == box.lower[2], zu = at[2] == box.upper[2];
    flags.lower |= xi && yi && zl;
    flags.upper |= xi && yi && zu;
    for (size_t side = 0; side < 2; side++) {
        const bool x = at[0] == (side ? box.upper[0] : box.lower[0]);
        const bool y = at[1] == (side ? box.upper[1] : box.lower[1]);
        flags.faceX[side] |= x && yi && zi;
        flags.faceY[side] |= y && xi && zi;
        flags.lowerX[side] |= x && yi && zl;
        flags.lowerY[side] |= y && xi && zl;
        flags.upperX[side] |= x && yi && zu;
        // The native plus-Y upper flag deliberately shares the lower-Z test.
        flags.upperY[side] |= y && xi && (side ? zl : zu);
        for (size_t ySide = 0; ySide < 2; ySide++) {
            const bool corner = x && at[1] == (ySide ? box.upper[1] : box.lower[1]);
            const size_t index = side + 2 * ySide;
            flags.lowerCorner[index] |= corner && zl;
            flags.verticalCorner[index] |= corner && zi;
        }
    }
}

void recordFlags(Classification &flags, const route::Position &center, const Subject &subject, const route::Record &record) {
    if (subject.excluded && subject.excluded(record.identity))
        return;
    const route::Dimensions size = uint32_t(record.orientation) & 1 ?
        route::Dimensions{record.size[1], record.size[0], record.size[2]} : record.size;
    const int64_t recordHalfX = half(size[0]), actorHalfX = half(subject.size[0]);
    const int64_t recordLowX = subtract(record.position[0], recordHalfX);
    const int64_t recordHighX = add(record.position[0], recordHalfX);
    const int64_t actorLowX = subtract(center[0], actorHalfX), actorHighX = add(center[0], actorHalfX);
    const auto overlaps = [&](int64_t xShift, int64_t yShift, int64_t zShift) {
        if (recordLowX > add(actorHighX, xShift) || add(actorLowX, xShift) >= recordHighX)
            return false;
        const int64_t recordHalfY = half(size[1]), actorHalfY = half(subject.size[1]);
        const int64_t y = add(center[1], yShift);
        if (subtract(record.position[1], recordHalfY) > add(y, actorHalfY) ||
            subtract(y, actorHalfY) >= add(record.position[1], recordHalfY))
            return false;
        const int64_t actorHalfZ = half(subject.size[2]);
        const int64_t z = add(center[2], zShift);
        if (record.position[2] > add(z, actorHalfZ))
            return false;
        const int64_t height = fixed(size[2]);
        return subtract(z, actorHalfZ) < add(record.position[2], height);
    };
    // Do not short-circuit using a previously accumulated true flag.
    flags.recordBelow |= overlaps(0, 0, -65536);
    if (size[2] < 3.f && !flags.upper && !flags.upperX[0] && !flags.upperX[1] && !flags.upperY[0] && !flags.upperY[1])
        return;
    switch (record.selector) {
    case 1: case 2: case 8: case 7: case 6: return;
    default: break;
    }
    if (std::equal(record.reference.begin(), record.reference.end(), subject.reference.begin()))
        return;
    flags.recordSide[0] |= overlaps(-65536, 0, 0);
    flags.recordSide[1] |= overlaps(65536, 0, 0);
    flags.recordSide[2] |= overlaps(0, -65536, 0);
    flags.recordSide[3] |= overlaps(0, 65536, 0);
}
}

Classification classify(const Cell &selected, const Subject &subject, const Scene &scene) {
    const double halfHeight = double(subject.size[2]) * .5;
    const int64_t vertical = int64_t(halfHeight * 65536.0);
    route::Position center{};
    for (size_t axis = 0; axis < 3; axis++)
        center[axis] = add(int64_t(selected[axis]) * 65536, axis == 2 ? vertical : 32768);
    const Bounds box = expandedBounds(center, subject.size);
    Classification result;
    const int32_t sampleZ = rawCell(add(center[2], 65));
    const int32_t sampleY = rawCell(center[1]), sampleX = rawCell(center[0]);
    result.sampleBit = (scene.cell(sampleX, sampleY, sampleZ) & 2) != 0;
    for (int32_t x = box.lower[0]; x <= box.upper[0]; x++)
        for (int32_t y = box.lower[1]; y <= box.upper[1]; y++)
            for (int32_t z = box.lower[2]; z <= box.upper[2]; z++) {
                if (x != box.lower[0] && x != box.upper[0] && y != box.lower[1] && y != box.upper[1] &&
                    z != box.lower[2] && z != box.upper[2])
                    continue;
                if (blocking(scene.cell(x, y, z)))
                    terrainFlags(result, box, {x, y, z});
            }
    if (scene.records)
        for (int32_t x = box.lower[0] / 8; x <= box.upper[0] / 8; x++)
            for (int32_t y = box.lower[1] / 8; y <= box.upper[1] / 8; y++)
                if (x >= 0 && y >= 0 && x < 0x200000 && y < 0x200000)
                    scene.records(x, y, [&](const route::Record &record) {
                        recordFlags(result, center, subject, record);
                        return true;
                    });
    return result;
}

} // namespace creatures::search
