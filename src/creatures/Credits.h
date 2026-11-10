#pragma once
#include "creatures/Inventory.h"
#include <functional>

namespace creatures {

struct Combatant;

// Complete section-ten record. The consumer does not use victimType for admission.
struct KillRecord {
    uint64_t player = 0, victim = 0;
    int32_t victimType = 0, experience = 0;
    std::array<uint8_t, 24> serialize() const;
    static KillRecord parse(const uint8_t *bytes); // complete 24-byte payload required
};

struct CreditItems {
    ItemRecord &source, &snapshot;
};

struct CreditWorld {
    std::function<Combatant *(uint64_t)> lookup;
    std::function<Combatant *()> controlled;
    uint8_t mode = 0;
    // Required for an admitted mode-zero linked award; both records belong to
    // the credited actor. Storage and actor pointers stay valid during the call.
    std::function<CreditItems(Combatant &)> linkedItems;
};

// Engine observations for later persistence/presentation, never callbacks inside
// the native mutation sequence. A record may award to the same actor twice.
struct CreditResult {
    Combatant *directAward = nullptr, *linkedAward = nullptr;
    Combatant *healthZeroed = nullptr;
};

// Held alpha-fatal-credit-level-consumer.md. Caller supplies world protection,
// current identity and mode; this function neither queues nor deduplicates.
CreditResult receiveCredit(const KillRecord &record, const CreditWorld &world);

} // namespace creatures
