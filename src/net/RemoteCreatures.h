#pragma once
// Remote creature smoothing (docs/specs/alpha-remote-creature-interpolation.md). On a connected
// client every listed creature but the local one keeps a live body, advanced each world update
// by the ordinary movement step from its last received record (§3.2), and a drawn pose that
// follows it (§2): exactly while the last receipt smoothed, a copy after a snap. A dead
// creature (health not above 0) runs no update, so its drawn pose stays frozen (§2.5).
#include "creatures/Physics.h"
#include "net/Protocol.h"
#include <array>
#include <map>

namespace net {

class RemoteCreatures {
  public:
    struct Remote {
        creatures::Body live;            // received record, then advanced locally
        glm::vec3 rotation{0};           // live rotation components (record +0x18)
        std::array<int64_t, 3> drawn{};  // drawn position (+0x1350)
        glm::vec3 drawnRotation{0};      // drawn rotation (+0x1374)
        bool smoothing = false;          // the flag +0x1398 (60 or 0)
        float health = 0;
    };
    // §1.3 step 7: a record applied with the receive's snap decision.
    void receive(uint64_t id, const CreatureRecord &record, bool snap);
    // §1.5: creatures no longer listed are gone.
    void retain(const std::map<uint64_t, CreatureRecord> &listed);
    // §3.2 then §2: advance each living creature's live body, then its drawn pose.
    void step(int32_t dtMs, const creatures::BlockType &block, const creatures::ObjectTest &objects);
    const Remote *find(uint64_t id) const;

  private:
    std::map<uint64_t, Remote> list;
};

} // namespace net
