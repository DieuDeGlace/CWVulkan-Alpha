#pragma once
// Client world-call boundary, separate from rendering and device input.
// Supplied identity admission is an engine adapter; this does not infer native
// pause/transition flags. The caller prepares one aggregate before this boundary.
#include <cstdint>
#include <functional>
#include <bit>

namespace simulation {
// Called after this world's elapsed-counter increment. The second quotient
// deliberately looks one supplied piece ahead, not back to the former counter.
inline bool clockWriteDue(uint32_t currentCounter, int32_t elapsed) {
    return std::bit_cast<int32_t>(currentCounter) / 10000 !=
           std::bit_cast<int32_t>(currentCounter + uint32_t(elapsed)) / 10000;
}

template <class Aggregate, class WorldCall, class Input>
void clientFrame(int32_t elapsed, bool admitted, Aggregate &events, WorldCall &&worldCall, Input &&input) {
    if (admitted) {
        while (elapsed > 20) {
            std::invoke(worldCall, 20, events);
            elapsed -= 20;
        }
        // Exact multiples end with 20, while zero/negative remain real calls.
        std::invoke(worldCall, elapsed, events);
    }
    std::invoke(input, events);
}
} // namespace simulation
