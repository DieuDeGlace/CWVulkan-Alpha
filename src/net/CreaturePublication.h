#pragma once
#include "net/Protocol.h"
#include <functional>
#include <mutex>

namespace net {

struct CreaturePublication {
    std::mutex &worldLock;
    // Both lookups run under worldLock and use the context key, never wire ID.
    // The returned pointer need only remain valid for that locked interval.
    std::function<CreatureRecord *(uint64_t)> lookup;
    // Separate engine admission policy, after assignment and before release.
    std::function<void(CreatureRecord &)> afterAssignment;
};
using CreatureDecoder = std::function<bool(const std::vector<uint8_t> &, CreatureRecord &)>;

bool decodeCreatureUpdate(const std::vector<uint8_t> &payload, CreatureRecord &temporary);
// Snapshot and final assignment have separate lock intervals. Decode operates
// only on the temporary. Missing storage and failed decoding are rejected by
// engine policy; the original's malformed/null behavior is not certified.
bool publishCreatureUpdate(uint64_t contextKey, const std::vector<uint8_t> &payload,
                           const CreaturePublication &services,
                           const CreatureDecoder &decode = decodeCreatureUpdate);

} // namespace net
