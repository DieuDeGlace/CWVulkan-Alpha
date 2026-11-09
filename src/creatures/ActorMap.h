#pragma once
#include <cstdint>
#include <map>

namespace creatures {

// Compare the complete key as signed two's-complement, without converting or
// interpreting the creature's own ID. Flipping the sign bit preserves all bits.
struct ActorKeyLess {
    constexpr bool operator()(uint64_t left, uint64_t right) const {
        constexpr uint64_t sign = uint64_t(1) << 63;
        return (left ^ sign) < (right ^ sign);
    }
};

template<class Value>
using ActorMap = std::map<uint64_t, Value, ActorKeyLess>;

// Merge a controlled entry with a protected received-state collection. Its
// membership remains fixed under the receive frame gate throughout this walk.
template<class Value, class Controlled, class Received>
void visitReceivedActors(ActorMap<Value> &actors, uint64_t controlledKey,
                         Controlled &&controlled, Received &&received) {
    bool visited = false;
    for (auto &[key, actor] : actors) {
        if (!visited && ActorKeyLess{}(controlledKey, key)) {
            controlled();
            visited = true;
        }
        received(actor);
    }
    if (!visited)
        controlled();
}

} // namespace creatures
