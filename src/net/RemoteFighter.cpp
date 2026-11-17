#include "net/RemoteFighter.h"
#include "net/Protocol.h"

namespace net {

void RemoteFighter::receive(uint64_t id, const CreatureRecord &record) {
    record.updateFighter(fighter);
    fighter.id = id;
}

} // namespace net
