#pragma once
#include "creatures/Combat.h"
#include "creatures/Physics.h"
#include <functional>

namespace creatures::selection {

struct Actor {
    Combatant *fighter = nullptr;
    Body *body = nullptr;
    explicit operator bool() const { return fighter != nullptr; }
};

// Engine behavior storage supplied to the bounded native prefix. The range's
// producer and the later meanings of these counters are separate dependencies.
struct State {
    int32_t &specialMs, &chaseMs, &cooldownMs;
    float range;
};

struct Potion {
    int32_t *count = nullptr;
    ItemRecord *item = nullptr;
};

enum class Timing { Recovery, Anticipation, Active };
struct Services {
    std::function<Actor(uint64_t)> lookup;
    std::function<float()> maximumHealth;
    std::function<int32_t()> duration;
    std::function<int32_t(Timing, int32_t)> timing;
    // First nonzero-count type-1/subtype-1 row, in forward bag/slot order.
    std::function<Potion()> firstPotion;
    std::function<int32_t()> draw;
    std::function<double(double)> power; // base is 1.5; argument is the widened float exponent
    std::function<bool(const Combatant &, const Combatant &)> related;
    std::function<bool(const Combatant &)> classified;
    std::function<bool(Actor, Actor)> visible;
    // Completes relationship publication/fanout after the subject's map store.
    std::function<void(Actor)> acquired;
    // Explicit engine extension, bound by the production quest-ally adapter.
    std::function<bool(Actor)> extensionAdmission;
};

enum class Outcome { ReturnFalse, ReturnTrue, ContinueWithTarget };
struct Result {
    Outcome outcome = Outcome::ReturnFalse;
    Actor target;
};

// Held alpha-behavior-target-selection.md. Call begin once, then consider each
// live world entry in ascending signed-key order when scanning(), then finish.
// No candidate collection is copied. Nonnull actors require usable bodies;
// required services and their containers stay valid throughout the operation.
// ContinueWithTarget names the handoff to later behavior, not a native return.
class TargetSelection {
  public:
    TargetSelection(Actor subject, State state, int32_t step, const Services &services)
        : subject(subject), state(state), step(step), services(services) {}
    void begin();
    bool scanning() const { return scan; }
    void consider(Actor candidate);
    Result finish();

  private:
    bool tempting(const Combatant &candidate) const;
    Actor subject;
    State state;
    int32_t step;
    const Services &services;
    Result result;
    bool begun = false, terminal = false, scan = false, pacified = false, acquired = false;
    float limit = 0;
};

} // namespace creatures::selection
