#include "creatures/TargetSelection.h"
#include <bit>
#include <cassert>

namespace creatures::selection {
namespace {
int32_t add(int32_t a, int32_t b) {
    return std::bit_cast<int32_t>(uint32_t(a) + uint32_t(b));
}
void reduce(int32_t &counter, int32_t step) {
    counter = std::bit_cast<int32_t>(uint32_t(counter) - uint32_t(step));
    if (counter < 0)
        counter = 0;
}
float squaredDistance(const Body &subject, const Body &candidate) {
    float delta[3];
    for (size_t axis = 0; axis < 3; axis++) {
        const int64_t difference = std::bit_cast<int64_t>(uint64_t(candidate.position[axis]) -
                                                         uint64_t(subject.position[axis]));
        const float narrowed = float(difference);
        delta[axis] = narrowed * (1.f / 65536.f);
    }
    const float x2 = delta[0] * delta[0], y2 = delta[1] * delta[1], z2 = delta[2] * delta[2];
    const float horizontal = y2 + x2;
    return horizontal + z2;
}
uint64_t preliminaryKey(const Combatant &subject) {
    uint64_t key = 0;
    float best = 0;
    for (const auto &[candidate, value] : subject.relations.threats)
        if (candidate != 0 && value >= best) {
            key = candidate;
            best = value;
        }
    return key;
}
float randomComponent(int32_t draw) {
    const float scaled = float(draw) * 4.f;
    const float fraction = scaled / 32767.f;
    return fraction - 2.f;
}
} // namespace

void TargetSelection::begin() {
    assert(!begun);
    begun = true;
    Combatant &actor = *subject.fighter;
    actor.mode &= ~uint32_t(AIMING);
    const uint8_t action = uint8_t(actor.action);
    if (action != 83 && action != 82 && action != 84 && action != 79 && actor.actionMs > 10000)
        actor.action = 0;
    // The semantic roll view retains the nonzero admission of the full word,
    // including negative values; it is not a positive-time comparison.
    if (actor.dodgeMs != 0 || (uint8_t(actor.action) == 80 && actor.actionMs < 3000)) {
        result.outcome = Outcome::ReturnTrue;
        terminal = true;
        return;
    }
    const float health = actor.health;
    const float quarter = services.maximumHealth() * .25f;
    if (health < quarter && actor.actionMs > services.duration()) {
        const Potion potion = services.firstPotion();
        if (potion.count && potion.item && *potion.count != 0 &&
            itemType(*potion.item) == 1 && itemSubtype(*potion.item) == 1) {
            actor.action = 80;
            actor.actionMs = 0;
            actor.copyHeld(*potion.item);
            *potion.count = std::bit_cast<int32_t>(uint32_t(*potion.count) - 1u);
            if (*potion.count <= 0) {
                *potion.count = 0;
                (*potion.item)[0] = (*potion.item)[1] = 0;
            }
            const float x = randomComponent(services.draw());
            const float y = randomComponent(services.draw());
            subject.body->intent.x = x;
            subject.body->intent.z = 0;
            subject.body->intent.y = y;
            result.outcome = Outcome::ReturnTrue;
            terminal = true;
            return;
        }
    }

    result.target = services.lookup(preliminaryKey(actor)); // zero is an exact lookup too
    if (actor.controller == 5) {
        const Actor linked = services.lookup(actor.ownerId);
        if (linked) {
            result.target = {};
            if (uint8_t(linked.fighter->action) == 106) {
                terminal = true;
                return;
            }
            float best = 0;
            for (const auto &[key, value] : linked.fighter->relations.counts)
                if (value > best && services.lookup(key)) {
                    result.target = services.lookup(key);
                    best = value;
                }
        }
    }
    if (!result.target) {
        const float freshHealth = actor.health;
        const float half = services.maximumHealth() * .5f;
        if (freshHealth < half && actor.controller == 3) {
            actor.action = 80;
            actor.actionMs = 0;
            auto item = actor.heldRecord();
            item[0] = 1;
            item[1] = 4;
            setItemLevel(item, std::bit_cast<int16_t>(uint16_t(actor.level)));
            actor.setHeld(item); // only the two selected words changed above
            subject.body->intent.x = 0;
            subject.body->intent.y = 0;
            subject.body->intent.z = 0;
            result.outcome = Outcome::ReturnTrue;
            terminal = true;
            return;
        }
    }
    reduce(state.specialMs, step);
    pacified = actor.hasBuff(8); // presence only, including expired resident rows
    scan = !result.target && !(actor.controller == 5 && actor.ownerId != 0) && state.cooldownMs <= 0;
    if (scan)
        limit = state.range * state.range;
}

bool TargetSelection::tempting(const Combatant &candidate) const {
    return uint8_t(candidate.action) == 82 && candidate.equipment[12].type == 20 &&
           int32_t(candidate.equipment[12].subtype) == subject.fighter->type;
}

void TargetSelection::consider(Actor candidate) {
    assert(begun && scan && !terminal);
    if (!candidate || candidate.fighter == subject.fighter || candidate.fighter->health <= 0 ||
        candidate.fighter->controller == 5)
        return;
    const Combatant &other = *candidate.fighter;
    float score = squaredDistance(*subject.body, *candidate.body);
    if (other.stealth > 0) {
        const int32_t difference = std::bit_cast<int32_t>(uint32_t(other.level) - uint32_t(subject.fighter->level));
        const float exponent = float(difference);
        const float power = float(services.power(double(exponent)));
        const float hidden = power * other.stealth;
        float factor = 1.f - hidden;
        constexpr float minimum = std::bit_cast<float>(uint32_t(0x3dcccccd));
        if (factor < minimum)
            factor = minimum;
        const float squared = factor * factor;
        score = score / squared;
    }
    if (pacified || !(score < limit))
        return;
    const bool related = services.related(*subject.fighter, other);
    const bool extension = services.extensionAdmission && services.extensionAdmission(candidate);
    if (!related && !(subject.fighter->controller == 5 && subject.fighter->ownerId == 0 && tempting(other)) && !extension)
        return;
    if (!services.visible(subject, candidate))
        return;
    if (services.classified(*subject.fighter) && !tempting(other) && !extension)
        return;
    result.target = candidate;
    limit = score;
    acquired = true;
}

Result TargetSelection::finish() {
    assert(begun);
    if (terminal)
        return result;
    if (acquired) {
        subject.fighter->relations.threats[result.target.fighter->id] = 1.f;
        services.acquired(result.target);
    }
    bool ready = false;
    Combatant &actor = *subject.fighter;
    if (result.target) {
        if (uint8_t(actor.action) == 0) {
            ready = true;
        } else {
            const int32_t recovery = services.timing(Timing::Recovery, -1);
            const int32_t anticipation = services.timing(Timing::Anticipation, -1);
            const int32_t active = services.timing(Timing::Active, -1);
            const int32_t threshold = add(add(recovery / 2, anticipation), active);
            ready = actor.actionMs > threshold;
        }
    }
    actor.mode = ready ? actor.mode | RUN : actor.mode & ~uint32_t(RUN);
    reduce(state.cooldownMs, step);
    if (!result.target)
        state.chaseMs = 0;
    result.outcome = result.target ? Outcome::ContinueWithTarget : Outcome::ReturnFalse;
    terminal = true;
    scan = false;
    return result;
}

} // namespace creatures::selection
