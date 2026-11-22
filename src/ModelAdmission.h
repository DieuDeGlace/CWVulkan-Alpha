#pragma once
#include "TerrainLights.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <vector>

namespace terrain_models {
using terrain_lights::FixedPosition;

struct Bound {
    FixedPosition centre;
    float radius;
};

inline std::optional<Bound> decorationBound(FixedPosition position, int32_t width, int32_t height, float scale) {
    const float rise = (float(height) * scale * 0.5f) * 65536.f;
    if (!(double(rise) >= -0x1p63 && double(rise) < 0x1p63))
        return std::nullopt;
    position[2] = std::bit_cast<int64_t>(uint64_t(position[2]) + uint64_t(int64_t(rise)));
    return Bound{position, float(std::max(width, height)) * scale};
}

inline std::optional<Bound> staticBound(FixedPosition position, const std::array<float, 3> &dimensions) {
    const double rise = (double(dimensions[2]) * .5) * 65536.;
    if (!(rise >= -0x1p63 && rise < 0x1p63))
        return std::nullopt;
    position[2] = std::bit_cast<int64_t>(uint64_t(position[2]) + uint64_t(int64_t(rise)));
    return Bound{position, std::max({dimensions[0], dimensions[1], dimensions[2]}) + 1.f};
}

struct SlotBox {
    FixedPosition low{}, high{}, midpoint{};
    SlotBox() = default;
    SlotBox(int x, int y, int lower, int upper)
        : low{int64_t(x)*65536, int64_t(y)*65536, int64_t(lower)*65536},
          high{(int64_t(x)+33)*65536, (int64_t(y)+33)*65536, (int64_t(upper)+1)*65536} {
        for (size_t i = 0; i < 3; ++i)
            midpoint[i] = (low[i] + high[i]) / 2;
    }
};

inline bool horizontalRange(const SlotBox &box, const FixedPosition &reference, float range) {
    const auto term = [](int64_t a, int64_t b) {
        const uint64_t delta = uint64_t(a) - uint64_t(b);
        return std::bit_cast<int64_t>(delta * delta) / 65536;
    };
    const float threshold = (range * range) * 65536.f;
    if (!(double(threshold) >= 0 && double(threshold) < 0x1p63))
        return false;
    const auto x = term(reference[0], box.midpoint[0]), y = term(reference[1], box.midpoint[1]);
    return x + y <= int64_t(threshold);
}

inline bool boxInside(const SlotBox &box, const FixedPosition &shift, const std::array<glm::vec4, 6> &planes) {
    const auto low = terrain_lights::renderPosition(box.low, shift), high = terrain_lights::renderPosition(box.high, shift);
    for (const auto &p : planes) {
        const float x = p.x * (p.x > 0 ? high.x : low.x);
        const float y = p.y * (p.y > 0 ? high.y : low.y);
        const float z = p.z * (p.z > 0 ? high.z : low.z);
        if (((x + y) + z) + p.w < 0)
            return false;
    }
    return true;
}

// The later decoration gate converts the two raw deltas separately to world-unit floats.
inline bool withinDecorationRange(const SlotBox &box, const FixedPosition &eye, float range) {
    const auto delta = [&](size_t axis) {
        const auto raw = std::bit_cast<int64_t>(uint64_t(box.midpoint[axis]) - uint64_t(eye[axis]));
        return float(raw) * (1.f / 65536.f);
    };
    const float x = delta(0), y = delta(1);
    return x * x + y * y <= range * range;
}

template<class Entry> struct RenderSlot {
    Entry *resident = nullptr;
    // No initial numeric value is established. A failed visit leaves the physical slot's last write in place.
    std::optional<float> lastSquared;

    bool admit(bool ownsCoordinate, const FixedPosition &eye, const FixedPosition &shift,
               const std::array<glm::vec4, 6> &planes, float boxRange) {
        if (!resident || !ownsCoordinate || !resident->active.populated() ||
            !horizontalRange(resident->bounds, eye, boxRange) || !boxInside(resident->bounds, shift, planes))
            return false;
        lastSquared = terrain_lights::priority(resident->bounds.midpoint, eye);
        return true;
    }
};

struct Distance {
    float squared;
    float opacity;
    bool deferred;
};

class ModelView {
  public:
    ModelView(FixedPosition camera, FixedPosition shift, const std::array<glm::vec4, 6> &planes, float farDistance)
        : reference(camera), outer(farDistance), inner(outer * .9f), admission(camera, shift, planes, outer) {}

    std::optional<Distance> classify(const Bound &model) const {
        return classify(model, model.centre);
    }
    std::optional<Distance> classify(const Bound &model, const FixedPosition &sortingPoint) const {
        if (!admission.accepts(model.centre, model.radius))
            return std::nullopt;
        const auto delta = [&](int axis) {
            const auto fixed = std::bit_cast<int64_t>(uint64_t(sortingPoint[axis]) - uint64_t(reference[axis]));
            return float(fixed) * (1.f / 65536.f);
        };
        const float x = delta(0), y = delta(1), z = delta(2);
        const float squared = (x * x + y * y) + z * z;
        if (squared <= inner * inner)
            return Distance{squared, 1.f, false};
        const float distance = float(std::sqrt(double(squared)));
        const float fade = 1.f - (distance - inner) / (outer - inner);
        return Distance{squared, fade, true};
    }

  private:
    FixedPosition reference;
    float outer, inner;
    terrain_lights::ViewAdmission admission;
};

// Each of these two native lists uses insertion for a complete input of at most 32 entries.
// The supported Windows comparison sort also reproduces the established 33-slot partition example.
template<class Record, class Compare> void orderByDistance(std::vector<Record> &list, Compare before) {
    if (list.size() > 32) {
        std::sort(list.begin(), list.end(), before);
        return;
    }
    for (auto it = list.begin(); it != list.end(); ++it) {
        const auto first = std::upper_bound(list.begin(), it, *it, before);
        std::rotate(first, it, it + 1);
    }
}

template<class Entry> void orderSlots(std::vector<const RenderSlot<Entry> *> &slots) {
    orderByDistance(slots, [](const auto *a, const auto *b) { return *a->lastSquared < *b->lastSquared; });
}

template<class Record> void orderDeferred(std::vector<Record> &list) {
    orderByDistance(list, [](const Record &a, const Record &b) { return a.squared > b.squared; });
}
} // namespace terrain_models
