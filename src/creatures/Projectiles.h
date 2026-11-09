#pragma once
// Arrows, bolts, boomerangs, spells, thrown rocks and stationary bursts
// (docs/specs/alpha-projectiles.md): the 112-byte shoot record, the actions that launch them,
// the charge ranged attacks build, and each step's flight, hits and end.
#include "creatures/Abilities.h"
#include "creatures/Combat.h"
#include "creatures/Physics.h"
#include <array>
#include <functional>
#include <vector>

namespace creatures {

enum ProjectileKind : int32_t { ARROW = 0, SPELL = 1, BOOMERANG_SHOT = 2, BURST = 3, ROCK = 4 };

// §1: the shoot record, both the network message (packet 9, world-update section 5) and the
// live projectile.
struct Projectile {
    uint64_t shooter = UINT64_MAX;
    std::array<int32_t, 3> home{-1, -1, 0}; // the shooter's record field 39 (not read here)
    int64_t position[3] = {0, 0, 0};        // 1/65536 blocks
    glm::vec3 velocity{0};                  // blocks per second
    float damage = 0;
    float radius = 0.5f;   // half the hit cube, blocks
    float scale = 1;       // the model is scaled by scale / 14
    float strength = 1;    // knock-back strength and area value
    float cost = 0;        // mana or charge spent at launch (not read)
    uint8_t special = 0;   // bursts instead of hitting directly (§6.4)
    int32_t kind = ARROW;
    uint8_t element = 0;   // 0 none, 1 fire, 2 water
    int32_t age = 0;       // ms
    int32_t slow = 0;      // ms of slow for a hit target (no Alpha creator sets it)

    glm::dvec3 blocks() const {
        return glm::dvec3((double)position[0], (double)position[1], (double)position[2]) / 65536.0;
    }
    std::array<uint8_t, 112> serialize() const;
    static Projectile parse(const uint8_t *record);
};

// What launches and flights produced besides projectiles.
struct ShotEffects {
    std::vector<HitEmission> hits; // ordered payloads plus local application policy
    std::vector<SoundEvent> sounds;
    std::vector<BuffEvent> buffs;    // procs gained or spent ("event 40" records)
    std::vector<Burst> bursts;       // particles §5.8, §5.9
    // Particles §5.7: arrows that struck a block (where, and the block), for their bursts in
    // the block's colour, which the caller knows.
    struct Struck {
        glm::dvec3 at{0};
        glm::ivec3 cell{0};
    };
    std::vector<Struck> struck;
};

// §2.1: the charge (field 23) of the actions that build it, `beforeMs` being the action time
// before this step. c.actionMs is the already advanced clock. The 200 ms
// charging sound samples charge before this call's gain.
void stepCharge(Combatant &c, int32_t beforeMs, int32_t dtMs, glm::dvec3 at, ShotEffects &out);
void chargeSound(const Combatant &c, int32_t beforeMs, int32_t dtMs, glm::dvec3 at, ShotEffects &out);
void advanceCharge(Combatant &c, float dtMs);

// Audio §6.2: the swing sounds of a creature's action this step (`beforeMs` as above), at `at`.
// (The multi-hit swings and action 104's flag are open, audio AU-4.)
void actionSounds(const Combatant &c, float beforeMs, float dtMs, glm::dvec3 at, const std::function<int()> &draw,
                  std::vector<SoundEvent> &out);

// §2.3-§2.6: the projectiles a creature's action makes this step (`beforeMs` as above; `aim`
// is record field 26, from the creature to what it aims at). `block` finds how far a healer's
// swing reaches (§2.6).
void launch(Combatant &c, const Body &body, glm::vec3 aim, float beforeMs, float dtMs, const std::function<int()> &draw,
            std::vector<Projectile> &shots, ShotEffects &out, const BlockType &block = {});

// §2.6: a healer's (class 3, specialization 1) healing burst at `at` (blocks): kind 3, element
// 2, radius 4.
Projectile healingBurst(const Combatant &healer, glm::dvec3 at);

// The actions whose hits travel as projectiles (§2) rather than strike at the hit moment.
bool shootsProjectile(int32_t action);

// A creature projectiles can hit.
struct ShotTarget {
    Combatant *fighter = nullptr;
    const Body *body = nullptr;
};

struct ProjectileContext {
    bool networkMode = false;
    const Combatant *controlled = nullptr;
};

class Projectiles {
  public:
    std::vector<Projectile> list;

    // Mutable source lookup is independent of authority: resource writes and
    // buff production have different network/controlled-creature gates.
    void step(int32_t dtMs, const std::vector<ShotTarget> &creatures, const std::function<Combatant *(uint64_t)> &lookup,
              const BlockType &block, const std::function<int()> &draw, ShotEffects &out,
              ProjectileContext context = {});
};

} // namespace creatures
