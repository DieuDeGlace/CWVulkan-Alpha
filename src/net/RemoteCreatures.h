#pragma once
// Remote creature smoothing (docs/specs/alpha-remote-creature-interpolation.md). On a connected
// client every listed creature but the local one keeps a live body, advanced each world update
// by the movement step from its last received record (§3.2), and a separate drawn pose (§2).
#include "creatures/ActorMap.h"
#include "creatures/Physics.h"
#include "net/Protocol.h"
#include "net/RemoteFighter.h"
#include <map>

namespace creatures {
struct NpcFrame;
}

namespace net {

class RemoteCreatures {
  public:
    struct Remote {
        creatures::Body live;            // locally advanced record; follow is the drawn position
        glm::vec3 drawnRotation{0};      // drawn rotation (+0x1374)
        float headPitch = 0;             // +0x58, received, then turned by the facing block
        glm::vec3 drawnLook{0};          // drawn aim; follows the received aim (+0x160)
        bool smoothing = false;          // the flag +0x1398 (60 or 0)
        bool attached = false;
        bool needsLiveSnapshot = false;
    };
    struct Controlled {
        creatures::Combatant &fighter;
        creatures::Body &body;
    };
    struct StepEffects {
        creatures::NpcFrame &frame;
        const std::function<int()> &draw;
    };
    // §1.3 step 7: apply the receipt's drawn pose in message order. After
    // draining receipts, publish the latest live record once for each actor.
    void receive(const CreatureReceipt &receipt);
    void updateLive(uint64_t id, const CreatureRecord &record, RemoteFighter &state);
    // Store the locally advanced live fields that are also in the wire record.
    void writeLive(uint64_t id, CreatureRecord &record, const RemoteFighter &state) const;
    // §1.5: creatures no longer listed are gone.
    void retain(const std::map<uint64_t, CreatureRecord> &listed);
    // §3.2 then §2, one world piece. Visit the controlled and received actors
    // by signed key. Each received actor uses its live fighter and body.
    void step(creatures::ActorMap<RemoteFighter> &fighters, int32_t dtMs, const creatures::BlockType &block,
              const creatures::ObjectTest &objects, const Controlled *controlled = nullptr,
              const std::function<void()> &advanceControlled = {}, const StepEffects *effects = nullptr);
    // Riding actor step §4: revisit attachments after the actor and object passes.
    void finishStep(creatures::ActorMap<RemoteFighter> &fighters, const Controlled *controlled = nullptr);
    const Remote *find(uint64_t id) const;

  private:
    bool attach(Remote &pet, const creatures::Combatant &fighter, const creatures::ActorMap<RemoteFighter> &fighters,
                const Controlled *controlled);
    std::map<uint64_t, Remote> list;
};

} // namespace net
