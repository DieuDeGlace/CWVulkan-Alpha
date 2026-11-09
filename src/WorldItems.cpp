#include "WorldItems.h"
#include "worldgen/Random.h"
#include "worldgen/Regions.h"
#include <cmath>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>

std::array<uint8_t, 325> WorldItem::save() const {
    std::array<uint8_t, 325> r{};
    std::memcpy(r.data(), item.data(), 280);
    std::memcpy(r.data() + 280, position, 24);
    std::memcpy(r.data() + 304, &angle, 4);
    std::memcpy(r.data() + 308, &scale, 4);
    r[312] = origin;
    std::memcpy(r.data() + 313, &landingMs, 4);
    std::memcpy(r.data() + 317, &hideMs, 4);
    std::memcpy(r.data() + 321, &day, 4);
    return r;
}

WorldItem WorldItem::load(const uint8_t *r) {
    WorldItem w;
    std::memcpy(w.item.data(), r, 280);
    std::memcpy(w.position, r + 280, 24);
    std::memcpy(&w.angle, r + 304, 4);
    std::memcpy(&w.scale, r + 308, 4);
    w.origin = r[312];
    std::memcpy(&w.landingMs, r + 313, 4);
    std::memcpy(&w.hideMs, r + 317, 4);
    std::memcpy(&w.day, r + 321, 4);
    return w;
}

std::array<uint8_t, 328> WorldItem::record() const {
    const auto s = save();
    std::array<uint8_t, 328> r{};
    std::memcpy(r.data(), s.data(), 313); // through the origin byte
    std::memcpy(r.data() + 0x13c, s.data() + 313, 12); // after three bytes of padding
    return r;
}

WorldItem WorldItem::fromRecord(const uint8_t *r) {
    std::array<uint8_t, 325> s{};
    std::memcpy(s.data(), r, 313);
    std::memcpy(s.data() + 313, r + 0x13c, 12);
    return load(s.data());
}

glm::mat4 worldItemPose(const WorldItem &w, glm::vec3 size, double clockMs, bool forBox) {
    const uint8_t type = creatures::itemType(w.item);
    const glm::vec3 c = size * 0.5f;
    const auto about = [](float degrees, glm::vec3 axis) { return glm::rotate(glm::mat4(1), glm::radians(degrees), axis); };
    const glm::vec3 X(1, 0, 0), Y(0, 1, 0), Z(0, 0, 1);
    glm::mat4 pose;
    if (type == 3 || type == 4 || type == 12 || type == 13) {
        // Standing on the model's x axis, its centre half the model's length above the ground;
        // coins spin (and their box is not turned).
        float a = w.angle;
        if (type >= 12)
            a = forBox ? 0.f : w.angle + 0.2f * (float)std::fmod(clockMs, 1800.0);
        // (Ry(90°) turns +x towards -z, as glm's does: the model's +x points down.)
        pose = about(a, Z) * glm::translate(glm::mat4(1), glm::vec3(0, 0, size.x / 2)) * about(90, Y) *
               glm::translate(glm::mat4(1), -c);
    } else if (type == 25 && !forBox) {
        pose = about(0.02f * (float)std::fmod(clockMs, 18000.0), Z) * glm::translate(glm::mat4(1), glm::vec3(0, 0, size.z)) *
               about(45, X) * about(45, Y) * glm::translate(glm::mat4(1), -c);
    } else {
        pose = about(w.angle, Z) * glm::translate(glm::mat4(1), -glm::vec3(c.x, c.y, 0));
    }
    // §2.4: the tumble while landing (innermost), and the hop.
    glm::mat4 tumble(1);
    glm::vec3 hop(0);
    if (w.landingMs > 0 && !forBox) {
        const float T = (float)w.landingMs;
        tumble = glm::translate(glm::mat4(1), c) * about(T, X) * about(0.7f * T, Y) * glm::translate(glm::mat4(1), -c);
        const float k = (T - 250) / 250;
        hop.z = 5 * (1 - k * k);
    }
    // The spec's model space spans [0, size]; the engine's meshes are centred on x and y.
    return glm::translate(glm::mat4(1), hop) * glm::scale(glm::mat4(1), glm::vec3(w.scale)) * pose * tumble *
           glm::translate(glm::mat4(1), glm::vec3(c.x, c.y, 0));
}

void WorldItems::generated(const worldgen::Zone &zone) {
    const Key key{zone.zx, zone.zy};
    if (lists.count(key))
        return;
    std::vector<WorldItem> &list = lists[key];
    // §4.2, §4.3: structure loot first, then the vegetation pass's items.
    for (const worldgen::ItemDrop &d : zone.drops) {
        WorldItem w;
        w.item = creatures::toRecord(d.item);
        w.position[0] = d.x, w.position[1] = d.y, w.position[2] = d.z;
        w.angle = d.rotation;
        w.scale = d.scale;
        w.origin = d.scale > 0.08f ? 2 : 1;
        list.push_back(w);
    }
    for (const worldgen::GroundItem &g : zone.items) {
        WorldItem w;
        w.item[0] = g.category;
        w.item[1] = g.subtype;
        creatures::setItemLevel(w.item, 1);
        for (int byte = 0; byte < 4; byte++)
            w.item[4 + byte] = (uint8_t)(g.seed >> (8 * byte));
        w.position[0] = (int64_t)(((double)g.x + 0.5) * 65536.0);
        w.position[1] = (int64_t)(((double)g.y + 0.5) * 65536.0);
        w.position[2] = (int64_t)g.z * 65536;
        w.angle = g.rotation;
        w.scale = g.scale == 1 ? 1.f / 14 : g.scale; // (the generator keeps 1 for "the default")
        const bool mushroom = g.category == 11 && g.subtype == 19; // (a ravine item, open points 12 §7.2)
        if (mushroom)
            w.item[0x0d] = 9;
        w.origin = g.category == 25 || mushroom ? 0 : 2;
        list.push_back(w);
    }
}

void WorldItems::restore(int32_t zx, int32_t zy, const std::vector<std::array<uint8_t, 325>> &saved, int32_t today) {
    std::vector<WorldItem> kept;
    for (const auto &bytes : saved) {
        WorldItem w = WorldItem::load(bytes.data());
        if (w.day < 0 || w.day >= today - 3)
            kept.push_back(w);
    }
    std::vector<WorldItem> &list = lists[{zx, zy}];
    if (kept != list) {
        list = std::move(kept);
        changed.insert({zx, zy});
    }
}

const std::vector<WorldItem> *WorldItems::zone(int32_t zx, int32_t zy) const {
    const auto it = lists.find({zx, zy});
    return it == lists.end() ? nullptr : &it->second;
}

void WorldItems::replace(int32_t zx, int32_t zy, std::vector<WorldItem> items) { lists[{zx, zy}] = std::move(items); }

void WorldItems::step(int32_t dt, bool offline, const creatures::BlockType &block, const std::function<int()> &draw,
                      std::vector<creatures::SoundEvent> &sounds) {
    const auto solid = [&](int64_t x, int64_t y, int64_t z) {
        const uint8_t t = block((int32_t)(x >> 16), (int32_t)(y >> 16), (int32_t)(z >> 16)) & 0x1f;
        return t != 0 && t != 2;
    };
    for (auto &[key, list] : lists)
        for (WorldItem &w : list) {
            // Paintings hang on a neighbouring wall; a later test wins.
            if (creatures::itemType(w.item) == 16) {
                int64_t *p = w.position;
                const int64_t one = 65536;
                if (solid(p[0], p[1] - one, p[2]))
                    p[1] = (p[1] / one) * one + one / 5, w.angle = 180;
                if (solid(p[0], p[1] + one, p[2]))
                    p[1] = (p[1] / one) * one + one * 4 / 5, w.angle = 0;
                if (solid(p[0] - one, p[1], p[2]))
                    p[0] = (p[0] / one) * one + one / 5, w.angle = 90;
                if (solid(p[0] + one, p[1], p[2]))
                    p[0] = (p[0] / one) * one + one * 4 / 5, w.angle = 270;
            }
            if (w.hideMs > 0)
                w.hideMs = std::max(0, w.hideMs - dt);
            if (w.landingMs > 0) {
                w.landingMs -= dt;
                if (w.landingMs <= 0) {
                    w.landingMs = 0;
                    if (offline)
                        sounds.push_back({w.blocks(), creatures::itemType(w.item) == 12 ? 58 : 59,
                                          1 + 0.1f * (float)draw() / 32767.f, 1});
                }
            }
        }
}

bool WorldItems::drop(const creatures::ItemRecord &item, glm::dvec3 at, float factor, int32_t day,
                      const creatures::BlockType &block, const std::function<int()> &draw) {
    const int64_t fixed[3] = {(int64_t)(at.x * 65536.0), (int64_t)(at.y * 65536.0), (int64_t)(at.z * 65536.0)};
    return drop(item, fixed, factor, day, block, draw);
}

bool WorldItems::drop(const creatures::ItemRecord &item, const int64_t at[3], float factor, int32_t day,
                      const creatures::BlockType &block, const std::function<int()> &draw) {
    WorldItem w;
    for (int k = 0; k < 3; k++)
        w.position[k] = at[k];
    w.scale = factor / 14;
    w.landingMs = 500 + draw() % 300;
    w.day = day;
    const int32_t bx = (int32_t)(w.position[0] / 65536), by = (int32_t)(w.position[1] / 65536);
    const Key key{worldgen::floorDiv(bx, worldgen::ZONE), worldgen::floorDiv(by, worldgen::ZONE)};
    const auto it = lists.find(key);
    if (it == lists.end())
        return false;
    w.item = item;
    w.angle = (float)draw() * 360.f / 32767.f;
    // It falls through air and water to the first solid block.
    int32_t z = (int32_t)(at[2] >> 16);
    for (int guard = 0; guard < 4096; guard++, z--) {
        const uint8_t t = block(bx, by, z) & 0x1f;
        if (t != 0 && t != 2)
            break;
    }
    w.position[2] = (int64_t)(z + 1) * 65536;
    it->second.push_back(w);
    changed.insert(key);
    return true;
}

std::optional<creatures::ItemRecord> WorldItems::pickUp(int32_t zx, int32_t zy, int32_t index, glm::dvec3 from) {
    const auto it = lists.find({zx, zy});
    if (it == lists.end() || index < 0 || index >= (int32_t)it->second.size())
        return std::nullopt;
    const WorldItem &w = it->second[index];
    const glm::dvec3 d = w.blocks() - from;
    if (glm::dot(d, d) > 16)
        return std::nullopt;
    const creatures::ItemRecord item = w.item;
    it->second.erase(it->second.begin() + index);
    changed.insert({zx, zy});
    return item;
}
