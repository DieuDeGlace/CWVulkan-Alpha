#pragma once
// Terrain light selection from the held chunk-record and origin contracts.
// Source collection, camera/anchor following and general tie parity stay separate.
#include <glm/glm.hpp>
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace terrain_lights {

constexpr size_t CAPACITY = 16, UPLOADED = 10;
using FixedPosition = std::array<int64_t, 3>;

struct Candidate {
    FixedPosition position{};
    glm::vec3 colour{0};
    float radius = 0;
    float priority = 0;
};

struct Chunk {
    glm::ivec2 coordinate{0};
    uint32_t mesh = 0;
    bool operator==(const Chunk &) const = default;
};

struct Selection {
    uint32_t mesh = 0, count = 0;
    std::array<glm::vec4, CAPACITY> positions{}, colours{};
};

struct alignas(16) ShaderLights {
    std::array<glm::vec4, UPLOADED> positions{}, colours{};
};
static_assert(sizeof(ShaderLights) == 320);

std::optional<FixedPosition> fromBlocks(glm::dvec3 position);
glm::vec3 renderPosition(const FixedPosition &position, const FixedPosition &shift);
float priority(const FixedPosition &position, const FixedPosition &reference);
void order(std::vector<Candidate> &candidates);
ShaderLights shaderPrefix(const Selection &selection);

struct FadingSource {
    FixedPosition position{};
    glm::vec3 colour{0};
    float radius = 20;
    int32_t ageMs = 0, durationMs = 400;
    glm::vec3 fadedColour() const;
};

class FadingSources {
  public:
    void append(const FadingSource &source) { sources.push_back(source); }
    // Integer age update and deferred expiry preserve survivor/append order.
    void advance(int32_t elapsedMs);
    std::span<const FadingSource> entries() const { return sources; }

  private:
    std::vector<FadingSource> sources;
};

class Selector {
  public:
    // Inputs are in shared priority order. Active chunk coordinates must be unique.
    // Geometry indexing and output capacity are retained between frame rebuilds.
    const std::vector<Selection> &rebuild(std::span<const Candidate> ordered, std::span<const Chunk> chunks,
                                          const FixedPosition &shift);

  private:
    std::vector<Chunk> cachedChunks;
    std::vector<std::pair<uint64_t, size_t>> index;
    std::vector<Selection> selections;
    glm::ivec2 first{0}, last{-1};
};

} // namespace terrain_lights
