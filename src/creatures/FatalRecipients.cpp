#include "creatures/FatalRecipients.h"
#include "creatures/Combat.h"

namespace creatures {

void applyFatalRecipients(Combatant &target, uint64_t targetKey, float amount, bool completed,
                          const FatalRecipientWorld &world) {
    world.visit([&](uint64_t key, Combatant *recipient) {
        if (!recipient)
            return;
        const uint64_t linkedKey = target.linkedId; // earlier recipients can change this value
        if (recipient->relations.contact == targetKey || recipient->relations.contact == linkedKey)
            recipient->relations.contact = 0;

        auto &threats = recipient->relations.threats;
        // A unique-key map can contain at most these two identities. Finish the
        // signed-order scan before erasing, without a per-recipient allocation.
        std::array<decltype(threats.begin()), 2> removed;
        size_t count = 0;
        for (auto entry = threats.begin(); entry != threats.end(); ++entry)
            if (entry->first == targetKey || entry->first == linkedKey)
                removed[count++] = entry;
        for (size_t i = 0; i < count; i++)
            threats.erase(removed[i]);

        if (target.controller != 1 || recipient->controller != 0)
            return;
        // Membership is tested now, after this recipient's cleanup and every
        // earlier local consumer. Do not snapshot/filter contribution values.
        if (target.relations.threats.contains(key)) {
            const KillRecord record{key, targetKey, target.type, (int32_t)amount};
            world.appendCredit(record);
            const auto result = receiveCredit(record, world.credit);
            if (world.consumed)
                world.consumed(record, result);
        }
        if (completed)
            world.completionReward(key, *recipient);
    });
}

} // namespace creatures
