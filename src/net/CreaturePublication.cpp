#include "net/CreaturePublication.h"

namespace net {

bool decodeCreatureUpdate(const std::vector<uint8_t> &payload, CreatureRecord &temporary) {
    return applyDelta(payload, temporary).has_value();
}

bool publishCreatureUpdate(uint64_t contextKey, const std::vector<uint8_t> &payload,
                           const CreaturePublication &services, const CreatureDecoder &decode) {
    CreatureRecord temporary;
    {
        std::lock_guard guard(services.worldLock);
        const CreatureRecord *resident = services.lookup(contextKey);
        if (!resident) return false;
        temporary = *resident;
    }
    if (!decode(payload, temporary)) return false;
    {
        std::lock_guard guard(services.worldLock);
        CreatureRecord *resident = services.lookup(contextKey);
        if (!resident) return false;
        *resident = temporary;
        if (services.afterAssignment) services.afterAssignment(*resident);
    }
    return true;
}

} // namespace net
