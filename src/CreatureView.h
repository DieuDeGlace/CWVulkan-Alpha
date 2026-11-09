#pragma once
// Draws creatures (docs/specs/alpha-creatures-models.md): the part models from the content
// stores, by model-table slot, and each creature's posed parts as draw items. Mirrored parts
// and hair-tinted parts get meshes of their own, made once.
#include "DataStore.h"
#include "Renderer.h"
#include "creatures/Particles.h"
#include "creatures/Pose.h"
#include <map>
#include <tuple>

class CreatureView {
  public:
    CreatureView(Renderer &renderer, const Content &content) : renderer(renderer), content(content) {}

    // A model's size in voxels (zero when the slot has no model).
    glm::vec3 voxels(uint16_t slot);
    // A model's voxels (null when the slot has no model), for picking cells.
    const CubModel *voxelModel(uint16_t slot);
    // A white one-voxel cube (attached spirit cubes, item services §3.7).
    MeshId cubeMesh();
    // A model's mesh as it is stored (0 when the slot has no model).
    MeshId plainMesh(uint16_t slot) { return mesh(slot, false, 0xffffff); }
    // Adds the creature's parts at `position` (render space, the centre of its collision box).
    // With `effects`, the creature's weapon trails go there (trails §1.3). True when a foot
    // came down this frame (models §7.3).
    bool draw(std::vector<DrawItem> &items, const creatures::Appearance &a, creatures::PoseState &state,
              const creatures::Motion &motion, glm::vec3 position, float dtMs,
              std::vector<EffectVertex> *effects = nullptr);
    // The creature's posed parts as meshes and their matrices (voxel mesh -> blocks from the
    // centre of the box), for drawing it outside the world (the character panels).
    std::vector<std::pair<MeshId, glm::mat4>> parts(const creatures::Appearance &a, creatures::PoseState &state,
                                                    const creatures::Motion &motion, float dtMs);

    // Particles §7.2: a dead creature at `at` (world blocks) crumbles: about half of each drawn
    // model's surface voxels become particles that wait up to a second, then fly outward from
    // the model's centre, fall and come to rest. `draw` is the C runtime's rand().
    void shatter(const creatures::Appearance &a, creatures::PoseState &state, const creatures::Motion &motion,
                 glm::dvec3 at, const std::function<int()> &draw, std::vector<creatures::Particle> &out);

  private:
    struct Model {
        ModelMesh mesh;
        glm::vec3 size{0};
        CubModel cub;
        bool loaded = false;
    };
    const Model &model(uint16_t slot);
    MeshId mesh(uint16_t slot, bool mirrored, uint32_t tint);

    Renderer &renderer;
    const Content &content;
    std::map<uint16_t, Model> models;
    std::map<std::tuple<uint16_t, bool, uint32_t>, MeshId> meshes;
    MeshId cube = 0;
};
