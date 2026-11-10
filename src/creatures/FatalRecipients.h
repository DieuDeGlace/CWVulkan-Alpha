#pragma once
#include "creatures/Credits.h"

namespace creatures {

// Supplied world services keep actor storage valid and visit the current map in
// signed key order. Appends may add live successors; erasing actor storage during
// this traversal is outside the owned-world contract.
struct FatalRecipientWorld {
    using Visit = std::function<void(uint64_t, Combatant *)>;
    std::function<void(const Visit &)> visit;
    CreditWorld credit;
    std::function<void(const KillRecord &)> appendCredit;
    // Engine output observation after the actual consumer, before grants.
    std::function<void(const KillRecord &, const CreditResult &)> consumed;
    // Required on completion: append/apply the first grant before generating
    // the second. This service must not precompute both items before delivery.
    std::function<void(uint64_t, Combatant &)> completionReward;
};

// The incoming target key is independent of target.id. The amount is the
// already prepared binary32 value; conversion belongs to each admitted record.
void applyFatalRecipients(Combatant &target, uint64_t targetKey, float amount, bool completed,
                          const FatalRecipientWorld &world);

} // namespace creatures
