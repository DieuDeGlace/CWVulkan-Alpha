#include "creatures/Visibility.h"
#include <algorithm>
#include <bit>
#include <cmath>

namespace creatures::visibility {
namespace {
constexpr float fixedScale = 65536.f;
constexpr float inverseFixed = 1.f / 65536.f;
constexpr float eligibleSquared = std::bit_cast<float>(uint32_t(0x358637bd));

int64_t add(int64_t a, int64_t b) {
    return std::bit_cast<int64_t>(uint64_t(a) + uint64_t(b));
}
int64_t subtract(int64_t a, int64_t b) {
    return std::bit_cast<int64_t>(uint64_t(a) - uint64_t(b));
}
int32_t stepCoordinate(int32_t value, bool positive) {
    return std::bit_cast<int32_t>(uint32_t(value) + (positive ? 1u : uint32_t(-1)));
}
int32_t truncatedCoordinate(int64_t value) {
    return std::bit_cast<int32_t>(uint32_t(value / 65536));
}
Position displacement(const Direction &direction, float parameter) {
    const Direction scaled{direction[0] * parameter, direction[1] * parameter, direction[2] * parameter};
    Position result;
    for (size_t axis = 0; axis < 3; axis++) {
        const float fixed = scaled[axis] * fixedScale;
        result[axis] = int64_t(fixed); // finite, representable ordinary conversion
    }
    return result;
}
int64_t halfExtent(float dimension) {
    const float half = dimension * .5f;
    const float fixed = half * fixedScale;
    return int64_t(fixed);
}

struct FaceQuery {
    const Position &start;
    const Direction &direction;
    float &bound;

    void face(const Record &record, const Direction &dimensions, size_t axis, int64_t plane) const {
        const float difference = float(subtract(plane, start[axis]));
        const float blocks = difference * inverseFixed;
        const float parameter = blocks / direction[axis];
        if (!(parameter >= 0 && parameter < bound))
            return;
        const Position delta = displacement(direction, parameter);
        const size_t transverse = 1 - axis;
        const int64_t across = add(start[transverse], delta[transverse]);
        const int64_t height = add(start[2], delta[2]);
        const int64_t extent = halfExtent(dimensions[transverse]);
        if (across < subtract(record.position[transverse], extent) || height < record.position[2] ||
            across >= add(record.position[transverse], extent))
            return;
        const float fixedHeight = dimensions[2] * fixedScale;
        if (height >= add(record.position[2], int64_t(fixedHeight)))
            return;
        bound = parameter;
    }

    void record(const Record &record) const {
        if (!(record.kind == 3 || record.kind == 4 || record.kind == 5 || record.kind == 8) || record.state == 0)
            return;
        Direction dimensions = record.dimensions;
        if (uint32_t(record.selector) & 1u)
            std::swap(dimensions[0], dimensions[1]);
        for (size_t axis = 0; axis < 2; axis++) {
            if (direction[axis] == 0)
                continue;
            const int64_t extent = halfExtent(dimensions[axis]);
            face(record, dimensions, axis, subtract(record.position[axis], extent));
            face(record, dimensions, axis, add(record.position[axis], extent));
        }
    }
};
} // namespace

float trace(const Position &start, const Direction &direction, float maximum,
            uint8_t requestedCategory, uint8_t additionalGeometry, const Scene &scene) {
    std::array<int32_t, 3> voxel;
    for (size_t axis = 0; axis < 3; axis++) {
        voxel[axis] = truncatedCoordinate(start[axis]);
        if (start[axis] < 0)
            voxel[axis] = stepCoordinate(voxel[axis], false);
    }
    float parameter = 0;
    int advances = 0;
    while (parameter < maximum) {
        uint8_t type = 1; // initialized native missing/outside fallback category
        if (voxel[0] >= 0 && voxel[1] >= 0 && voxel[0] < 0x1000000 && voxel[1] < 0x1000000)
            type = scene.voxel(voxel[0], voxel[1], voxel[2]) & 31;
        const uint8_t category = type == 0 || type == 2 ? 0 : 1;
        if (category != requestedCategory || advances > 500)
            break;
        advances++;
        const Position delta = displacement(direction, parameter);
        float increment = 10.f;
        size_t chosen = 0;
        for (size_t axis = 0; axis < 3; axis++) {
            const float squared = direction[axis] * direction[axis];
            if (squared < eligibleSquared)
                continue;
            const int32_t edge = direction[axis] > 0 ? stepCoordinate(voxel[axis], true) : voxel[axis];
            const int64_t boundary = int64_t(edge) * 65536;
            const int64_t current = add(start[axis], delta[axis]);
            const float distance = float(subtract(boundary, current));
            const float quotient = distance / direction[axis];
            const float quantized = float(int64_t(quotient));
            const float candidate = quantized * inverseFixed;
            if (candidate < increment) {
                increment = candidate;
                chosen = axis;
            }
        }
        voxel[chosen] = stepCoordinate(voxel[chosen], direction[chosen] > 0);
        parameter = parameter + increment;
    }
    if (parameter > maximum)
        parameter = maximum;

    if (additionalGeometry && scene.records) {
        const int32_t startX = truncatedCoordinate(start[0]) / 256;
        const int32_t startY = truncatedCoordinate(start[1]) / 256;
        const int32_t endX = voxel[0] / 256, endY = voxel[1] / 256;
        const FaceQuery query{start, direction, parameter};
        const RecordVisitor visit = [&query](const Record &record) { query.record(record); };
        for (int32_t x = std::min(startX, endX); x <= std::max(startX, endX); x++)
            for (int32_t y = std::min(startY, endY); y <= std::max(startY, endY); y++)
                if (x >= 0 && y >= 0 && x <= 65535 && y <= 65535)
                    scene.records(x, y, visit);
    }
    return parameter;
}

bool visible(const Position &start, const Position &end, const Scene &scene) { return visible(start, end, 1, 200.f, scene); }

bool visible(const Position &start, const Position &end, uint8_t option, float maximum, const Scene &scene) {
    Direction direction;
    for (size_t axis = 0; axis < 3; axis++) {
        const float difference = float(subtract(end[axis], start[axis]));
        direction[axis] = difference * inverseFixed;
    }
    const float x2 = direction[0] * direction[0], y2 = direction[1] * direction[1], z2 = direction[2] * direction[2];
    const float horizontal = y2 + x2;
    const float squared = horizontal + z2;
    if (squared < std::bit_cast<float>(uint32_t(0x38d1b717)))
        return true;
    if (squared > maximum * maximum)
        return false;
    const float length = float(std::sqrt(double(squared)));
    for (float &component : direction)
        component = component / length;
    const float reached = trace(start, direction, length, 0, option, scene);
    const float threshold = length - std::bit_cast<float>(uint32_t(0x3c23d70a));
    return reached >= threshold;
}

} // namespace creatures::visibility
