#include "creatures/SearchState.h"
#include <algorithm>
#include <bit>
#include <cstdlib>
#include <immintrin.h>

namespace creatures::search {
namespace {
int32_t add(int32_t a, int32_t b) { return std::bit_cast<int32_t>(uint32_t(a) + uint32_t(b)); }
constexpr std::array<Cell, 10> offsets{{
    {-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0},
    {-1, -1, 0}, {1, -1, 0}, {-1, 1, 0}, {1, 1, 0}, {0, 0, -1}, {0, 0, 1}}};
}

bool Classification::allows(size_t n, const std::array<bool, 4> &updated) const {
    switch (n) {
    case 0:
        return !faceX[0] && !recordSide[0] &&
            (lower || lowerX[0] || faceY[0] || faceY[1] || verticalCorner[0] || verticalCorner[2] || sampleBit);
    case 1:
        return !faceX[1] && !recordSide[1] &&
            (lower || recordBelow || lowerX[1] || faceY[0] || faceY[1] || verticalCorner[1] || verticalCorner[3] || sampleBit);
    case 2:
        return !faceY[0] && !recordSide[2] &&
            (lower || recordBelow || lowerY[0] || faceX[0] || faceX[1] || verticalCorner[0] || verticalCorner[1] || sampleBit);
    case 3:
        return !faceY[1] && !recordSide[3] &&
            (lower || recordBelow || lowerY[1] || faceX[0] || faceX[1] || verticalCorner[2] || verticalCorner[3] || sampleBit);
    case 4: case 5: case 6: case 7: {
        const size_t corner = n - 4;
        return updated[corner & 1] && updated[2 + (corner >> 1)] && !verticalCorner[corner] &&
            (lower || recordBelow || lowerCorner[corner] || sampleBit);
    }
    case 8: return !lower && !recordBelow;
    case 9: return !upper && (faceX[0] || faceX[1] || faceY[0] || faceY[1] || sampleBit);
    default: return false;
    }
}

Cell coordinate(std::span<const int64_t, 3> raw) {
    Cell result{};
    for (int axis = 2; axis >= 0; axis--) {
        const int64_t value = raw[size_t(axis)];
        result[size_t(axis)] = std::bit_cast<int32_t>(uint32_t(value / 65536) - uint32_t(value < 0));
    }
    return result;
}

int32_t destinationScore(const Cell &from, const Cell &to) {
    // The held ordinary domain excludes INT_MIN differences and score overflow.
    const int32_t x = std::abs(from[0] - to[0]), y = std::abs(from[1] - to[1]);
    const int32_t z = std::abs(from[2] - to[2]);
    return 2 * (10 * std::max(x, y) + 4 * std::min(x, y) + 10 * z);
}

void State::clear() {
    records.clear();
    pending.clear();
    path.clear();
    remembered.reset();
    exclusions.clear();
}

void State::publish(const Cell &key, const Node &value) {
    records.insert_or_assign(key, value);
    pending.insert(key);
    if (!remembered || value.destination < records.at(*remembered).destination)
        remembered = key;
}

void State::setup(std::span<const float, 3> size, const route::RecordLookup &lookup) {
    clear();
    anchor = coordinate(start);
    const Cell seed = coordinate(start);
    counter = 0;
    const int32_t integer = destinationScore(seed, coordinate(goal));
    const int32_t rounded = _mm_cvttss_si32(_mm_cvtepi32_ps(_mm_cvtsi32_si128(integer)));
    publish(seed, {1, rounded, rounded, seed});
    exclusions.clear();
    const uint16_t unusedFlags = 0;
    const route::Subject subject{start, size, unusedFlags, {}};
    const auto insert = [&](uintptr_t identity) { exclusions.insert(identity); };
    route::endpointExclusions(subject, start, lookup, insert);
    route::endpointExclusions(subject, goal, lookup, insert);
}

bool State::step(const ClassificationLookup &classify) {
    if (records.empty())
        return false;
    if (pending.size() > 500 || records.size() > 20000) {
        clear();
        return false;
    }
    const Cell destination = coordinate(goal);
    if (pending.empty())
        return false;
    auto selected = records.end();
    for (const Cell &key : pending) {
        const auto found = records.find(key);
        if (found != records.end() && (selected == records.end() || found->second.selection < selected->second.selection))
            selected = found;
    }
    // Well-formed pending/map storage is a precondition, not a repair policy.
    const Cell here = selected->first;
    const Node current = selected->second;
    pending.erase(here);
    if (here == destination)
        return false;
    const Classification geometry = classify(here);
    std::array<bool, 4> updated{};
    for (size_t n = 0; n < offsets.size(); n++) {
        if (!geometry.allows(n, updated))
            continue;
        Cell key{};
        for (size_t axis = 0; axis < 3; axis++)
            key[axis] = add(here[axis], offsets[n][axis]);
        const auto found = records.find(key);
        const bool fresh = found == records.end();
        int32_t cost;
        if (n == 8) cost = 0;
        else if (n == 9) cost = geometry.lower ? 0 : 40;
        else if (n >= 4) cost = geometry.lower ? 14 : 24;
        else cost = geometry.lower ? 10 : (n == 2 && fresh ? 30 : 20);
        const int32_t accumulated = add(current.accumulated, cost);
        if (!fresh && (!pending.contains(key) || accumulated >= found->second.accumulated))
            continue;
        const int32_t score = fresh ? destinationScore(key, coordinate(goal)) : found->second.destination;
        publish(key, {accumulated, score, add(accumulated, score), here});
        if (n < updated.size())
            updated[n] = true;
    }
    return true;
}

void State::rebuild() {
    const Cell first = coordinate(start);
    const Cell destination = coordinate(goal);
    path.clear();
    Cell at = remembered.value_or(destination);
    path.push_back(at);
    for (size_t attempts = 0; at != first && attempts <= records.size(); attempts++) {
        const auto found = records.find(at);
        if (found == records.end() || found->second.parent == at)
            break;
        at = found->second.parent;
        path.push_back(at);
    }
    std::reverse(path.begin(), path.end());
}

void State::reroot(const Cell &nearest) {
    anchor = nearest;
    for (size_t axis = 0; axis < 3; axis++)
        start[axis] = int64_t(nearest[axis]) * 65536;
    std::set<Cell> retained;
    const size_t attempts = std::min(pending.size(), size_t(70));
    for (size_t i = 0; i < attempts; i++) {
        int32_t score = -1;
        auto choice = pending.end();
        for (auto it = pending.begin(); it != pending.end(); ++it) {
            const auto found = records.find(*it);
            if (found != records.end() && (score < 0 || found->second.destination < score)) {
                score = found->second.destination;
                choice = it;
            }
        }
        if (score >= 0) {
            retained.insert(*choice);
            pending.erase(choice);
        }
    }
    pending = std::move(retained);
    std::map<Cell, Node> replacement;
    const auto referenceRecord = records.find(nearest);
    for (auto it = pending.begin(); it != pending.end();) {
        std::vector<std::pair<Cell, Node>> chain;
        auto at = records.find(*it);
        while (at != records.end() && at != referenceRecord) {
            chain.push_back(*at);
            const Cell parent = at->second.parent;
            if (parent == at->first) {
                at = records.end();
                break;
            }
            at = records.find(parent);
        }
        if (at == records.end()) {
            it = pending.erase(it);
            continue;
        }
        for (const auto &[key, value] : chain)
            replacement.insert_or_assign(key, value);
        ++it;
    }
    Node root = records.at(nearest);
    root.parent = nearest;
    replacement.insert_or_assign(nearest, root);
    records = std::move(replacement);
    remembered.reset();
    for (const auto &[key, value] : records)
        if (!remembered || value.destination < records.at(*remembered).destination)
            remembered = key;
}

} // namespace creatures::search
