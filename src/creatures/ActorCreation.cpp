#include "creatures/ActorCreation.h"
#include <algorithm>
#include <bit>
#include <utility>

namespace creatures::creation {
namespace {
void generatedAppearance(uint32_t kind, Appearance &looks, const Services &services) {
    for (int draw = 0; draw < 5; draw++)
        services.draw(); // the closed common producer prefix, including discarded material
    looks.box = {std::bit_cast<float>(uint32_t(0x3f75c290)),
                 std::bit_cast<float>(uint32_t(0x3f75c290)),
                 std::bit_cast<float>(uint32_t(0x400a3d71))};
    if (kind == 0x25) {
        looks.models[4] = 0x095f;
        looks.bodyOffset = {0, 0, 0};
        looks.bodyScale = 1.5f;
        looks.box = glm::vec3(std::bit_cast<float>(uint32_t(0x3f4ccccd)));
        looks.flags |= 0x0029;
    }
}
} // namespace

Result create(Actor current, const Configuration &configuration, int32_t step,
              const World &world, const Services &services) {
    const Combatant &creator = *current.fighter;
    if (uint8_t(creator.action) != 92 ||
        !((world.mode == 0 && creator.controller != 0) || world.designated == current.fighter))
        return {};
    const int32_t first = services.anticipation(-1);
    if (creator.actionMs > first)
        return {};
    const int32_t second = services.anticipation(-1);
    const int32_t after = std::bit_cast<int32_t>(uint32_t(creator.actionMs) + uint32_t(step));
    if (after <= second)
        return {};

    int32_t linked = 0;
    uint64_t smallest = 0;
    world.visit([&](uint64_t key, const Combatant *actor) {
        if (!actor)
            return;
        if (actor->ownerId == creator.id)
            linked++;
        if (ActorKeyLess{}(key, smallest))
            smallest = key;
    });
    if (linked > 2)
        return {};

    Created result;
    result.fighter.id = smallest - uint64_t(1);
    result.fighter.characterClass = result.fighter.specialization = 0;
    result.fighter.hitStun = -3000;
    result.fighter.controller = creator.controller == 0 ? 3 : 1;
    uint32_t kind = 0x25;
    if (!configuration.kinds.empty()) {
        const uint32_t draw = uint32_t(services.draw());
        kind = uint32_t(configuration.kinds[draw % configuration.kinds.size()]);
    }
    if (configuration.choice == 1 || configuration.choice == 2) {
        Actor provider = current;
        if (configuration.choice == 2) {
            const Actor selected = world.lookup(creator.relations.contact);
            if (selected.fighter)
                provider = selected;
        }
        if (!provider.looks)
            return {Outcome::MissingAppearanceState, {}};
        result.fighter.type = provider.fighter->type;
        result.fighter.copyEquipmentFrom(*provider.fighter);
        result.looks = *provider.looks; // all semantic appearance fields; no native gap5 claim
    } else {
        if (kind != 0x25 && kind <= 0x9b)
            return {Outcome::AppearancePending, {}};
        result.fighter.type = std::bit_cast<int32_t>(kind);
        generatedAppearance(kind, result.looks, services);
    }
    result.looks.flags = uint16_t((result.looks.flags & ~uint16_t(0x0200)) | 0x0800);
    result.fighter.appearanceFlags = result.looks.flags;
    result.fighter.level = creator.level;
    result.fighter.health = services.maximumHealth(creator);
    std::copy_n(current.body->position, 3, result.body.position);
    result.body.size = result.looks.box;
    result.setup.companionKey = creator.id;
    result.fighter.ownerId = creator.id; // installed after the ordered setup state
    return {Outcome::Created, std::move(result)};
}

} // namespace creatures::creation
