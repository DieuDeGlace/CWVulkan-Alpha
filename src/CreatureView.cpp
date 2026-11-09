#include "CreatureView.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

const CreatureView::Model &CreatureView::model(uint16_t slot) {
    Model &m = models[slot];
    if (m.loaded)
        return m;
    m.loaded = true;
    if (slot < creatures::MODEL_SLOTS && creatures::MODEL_KEYS[slot])
        if (const auto bytes = content.read(std::string(creatures::MODEL_KEYS[slot]) + ".cub")) {
            CubModel cub;
            if (cub.load(bytes->data(), bytes->size())) {
                // §5.1: hair and eye layers carry key colours. (The spec's other key-colour
                // models, staffs, wands and bracelets, come with equipment.)
                const std::string key = creatures::MODEL_KEYS[slot];
                const bool keys = key.find("-hair-") != std::string::npos || key.rfind("-eyes") == key.size() - 5;
                m.mesh = cub.mesh(glm::vec3(0), keys);
                m.size = glm::vec3(cub.sx, cub.sy, cub.sz);
                m.cub = std::move(cub);
            }
        }
    return m;
}

glm::vec3 CreatureView::voxels(uint16_t slot) { return model(slot).size; }

const CubModel *CreatureView::voxelModel(uint16_t slot) {
    const Model &m = model(slot);
    return m.cub.sx > 0 ? &m.cub : nullptr;
}

MeshId CreatureView::cubeMesh() {
    if (!cube) {
        CubModel one;
        one.sx = one.sy = one.sz = 1;
        one.rgb = {255, 255, 255};
        cube = renderer.addMesh(one.mesh());
    }
    return cube;
}

MeshId CreatureView::mesh(uint16_t slot, bool mirrored, uint32_t tint) {
    const auto key = std::make_tuple(slot, mirrored, tint);
    if (const auto it = meshes.find(key); it != meshes.end())
        return it->second;
    ModelMesh copy = model(slot).mesh;
    if (mirrored) {
        // The mirror image in x, with the winding turned so faces still face out.
        for (ModelVertex &v : copy.vertices) {
            v.position.x = -v.position.x;
            v.normal.x = -v.normal.x;
        }
        for (size_t i = 0; i + 2 < copy.indices.size(); i += 3)
            std::swap(copy.indices[i + 1], copy.indices[i + 2]);
    }
    if (tint != 0xffffff) { // §5: the hair layer takes the hair colour
        const glm::vec4 c((tint >> 16 & 255) / 255.f, (tint >> 8 & 255) / 255.f, (tint & 255) / 255.f, 1);
        for (ModelVertex &v : copy.vertices)
            v.colour *= c;
    }
    const MeshId id = copy.indices.empty() ? 0 : renderer.addMesh(copy);
    meshes[key] = id;
    return id;
}

void CreatureView::shatter(const creatures::Appearance &a, creatures::PoseState &state,
                           const creatures::Motion &motion, glm::dvec3 at, const std::function<int()> &draw,
                           std::vector<creatures::Particle> &out) {
    const auto posed = creatures::poseParts(state, a, motion, 0, [&](uint16_t slot) { return voxels(slot); });
    const glm::vec3 hair = glm::vec3(a.hairColour[0], a.hairColour[1], a.hairColour[2]) / 255.f;
    for (const creatures::PartDraw &p : posed) {
        const CubModel *m = p.cube ? nullptr : voxelModel(p.model);
        if (!m)
            continue;
        // The part's frame from voxel corners (the mesh is centred in x and y), mirrored as drawn.
        glm::mat4 M = p.matrix;
        if (p.mirrored)
            M = glm::scale(M, glm::vec3(-1, 1, 1));
        M = glm::translate(M, glm::vec3(-m->sx / 2.f, -m->sy / 2.f, 0));
        const glm::vec3 centre = glm::vec3(M * glm::vec4(m->sx / 2.f, m->sy / 2.f, m->sz / 2.f, 1));
        glm::mat4 turn = M;
        turn[3] = glm::vec4(0, 0, 0, 1);
        // Hair layers leave their key colours out (models §5.1).
        const auto filled = [&](int x, int y, int z) {
            if (!m->solid(x, y, z))
                return false;
            if (!p.hairTint)
                return true;
            const glm::u8vec3 c = m->colour(x, y, z);
            const int bright = (c.r == 255) + (c.g == 255) + (c.b == 255), dark = (c.r == 0) + (c.g == 0) + (c.b == 0);
            return !(bright == 1 && dark == 2);
        };
        const glm::vec3 tint = p.hairTint ? hair : glm::vec3(1);
        for (int z = 0; z < m->sz; z++)
            for (int y = 0; y < m->sy; y++)
                for (int x = 0; x < m->sx; x++) {
                    if (!filled(x, y, z))
                        continue;
                    if (filled(x - 1, y, z) && filled(x + 1, y, z) && filled(x, y - 1, z) && filled(x, y + 1, z) &&
                        filled(x, y, z - 1) && filled(x, y, z + 1))
                        continue; // inside: only the surface crumbles
                    if ((draw() & 1) != 0)
                        continue;
                    creatures::Particle k;
                    k.style = creatures::LIT;
                    k.ageMs = -(draw() % 1000);
                    const glm::vec3 here = glm::vec3(M * glm::vec4(x, y, z, 1));
                    const glm::dvec3 world = at + glm::dvec3(here);
                    for (int i = 0; i < 3; i++)
                        k.position[i] = (int64_t)(world[i] * 65536.0);
                    k.velocity = 2.f * (here - centre);
                    k.spin.x = 100 * ((float)draw() / 32767.f - 0.5f);
                    k.spin.y = 100 * ((float)draw() / 32767.f - 0.5f);
                    k.spin.z = 100 * ((float)draw() / 32767.f - 0.5f);
                    k.colour = glm::vec4(glm::vec3(m->colour(x, y, z)) / 255.f * tint, 1);
                    k.extra = turn;
                    k.collides = true;
                    k.lifetimeMs = 5000;
                    out.push_back(k);
                }
    }
}

std::vector<std::pair<MeshId, glm::mat4>> CreatureView::parts(const creatures::Appearance &a, creatures::PoseState &state,
                                                              const creatures::Motion &motion, float dtMs) {
    creatures::stepWalk(state, a, motion, dtMs);
    const auto posed = creatures::poseParts(state, a, motion, dtMs, [&](uint16_t slot) { return voxels(slot); });
    const uint32_t hair = (uint32_t)a.hairColour[0] << 16 | (uint32_t)a.hairColour[1] << 8 | a.hairColour[2];
    std::vector<std::pair<MeshId, glm::mat4>> out;
    for (const creatures::PartDraw &p : posed)
        if (!p.cube)
            if (const MeshId id = mesh(p.model, p.mirrored, p.hairTint ? hair : 0xffffff))
                out.push_back({id, p.matrix});
    return out;
}

bool CreatureView::draw(std::vector<DrawItem> &items, const creatures::Appearance &a, creatures::PoseState &state,
                        const creatures::Motion &motion, glm::vec3 position, float dtMs,
                        std::vector<EffectVertex> *effects) {
    // Trails §1.3: the trail drawn this frame is the one aged in the last.
    if (effects && motion.fighter) {
        std::vector<creatures::TrailCorner> corners;
        creatures::trailTriangles(state, *motion.fighter, position, corners);
        for (const creatures::TrailCorner &k : corners)
            effects->push_back({k.position, 0x00ffffffu | (uint32_t)k.alpha << 24});
    }
    creatures::Motion at = motion;
    at.height = position.z;
    const bool footstep = creatures::stepWalk(state, a, at, dtMs);
    const auto parts = creatures::poseParts(state, a, at, dtMs, [&](uint16_t slot) { return voxels(slot); });
    const uint32_t hair = (uint32_t)a.hairColour[0] << 16 | (uint32_t)a.hairColour[1] << 8 | a.hairColour[2];
    const glm::mat4 place = glm::translate(glm::mat4(1), position);
    for (const creatures::PartDraw &p : parts)
        if (p.cube)
            items.push_back({cubeMesh(), place * p.matrix, true, Shading::World, glm::vec4(p.colour, 1), p.shine});
        else if (const MeshId id = mesh(p.model, p.mirrored, p.hairTint ? hair : 0xffffff))
            items.push_back({id, place * p.matrix, true, Shading::World, glm::vec4(1), p.shine});
    return footstep;
}
