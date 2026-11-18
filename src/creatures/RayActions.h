#pragma once
// The shared action family in alpha-dedicated-action-projectile-launch.md,
// alpha-ray-launch-fields-reconciliation.md and alpha-strike-call-sites.md.
#include "creatures/Projectiles.h"

namespace creatures {

inline bool isRayAction(int32_t action) { return action == 28 || action == 94 || action == 95; }

struct RayActionWorld {
    bool connected = false; // native world mode byte
    Combatant *controlled = nullptr;
    std::span<const ShotTarget> targets;
    visibility::Scene sight;
    std::function<bool(Combatant &, const Body &, HitInput)> hit;
    // Append the resident copy before the separate outgoing copy.
    std::function<void(const Projectile &)> launch;
};

struct RayActionEvents {
    std::vector<HitRecord> &hits;
    std::vector<BuffEvent> &buffs;
    std::vector<SoundEvent> &sounds;
    std::vector<Burst> &bursts;
};

// Runs before ordinary action-age progression. Every launch reevaluates its
// class, specialization and age threshold after any preceding hit resolver.
void stepRayAction(Combatant &source, const Body &body, int32_t milliseconds,
                   const RayActionWorld &world, RayActionEvents events,
                   const std::function<int()> &draw);

} // namespace creatures
