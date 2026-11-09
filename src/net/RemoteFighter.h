#pragma once
// Engine presentation state for a received actor. Native interpolation and
// timing remain separate; every decoded core snapshot still replaces its fields.
#include "creatures/Combat.h"

namespace net {

struct RemoteFighter {
    creatures::Combatant fighter;
    float leftoverMs = 0; // Existing engine fractional frame-duration adapter.
    void refresh(creatures::Combatant snapshot, int32_t elapsedMs);

  private:
    int32_t receivedAction = -1, receivedClock = 0;
};

} // namespace net
