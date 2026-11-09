#include "creatures/Particles.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace creatures {

namespace {

constexpr double PI = 3.141592653589793;

// §0: a float distance in blocks as fixed point, truncated toward zero.
int64_t fixed(float blocks) { return (int64_t)((double)blocks * 65536.0); }
int32_t cellOf(int64_t v) { return (int32_t)std::floor((double)v / 65536.0); }

} // namespace

Burst Burst::at(glm::dvec3 blocks, glm::vec3 velocity, glm::vec4 colour, float size, int32_t count, int32_t style) {
    Burst b;
    for (int k = 0; k < 3; k++)
        b.position[k] = (int64_t)(blocks[k] * 65536.0);
    b.velocity = velocity;
    b.colour = colour;
    b.size = size;
    b.count = count;
    b.style = style;
    return b;
}

std::array<uint8_t, 72> Burst::serialize() const {
    std::array<uint8_t, 72> r{};
    std::memcpy(&r[0x00], position, 24);
    std::memcpy(&r[0x18], &velocity, 12);
    std::memcpy(&r[0x24], &colour, 16);
    std::memcpy(&r[0x34], &size, 4);
    std::memcpy(&r[0x38], &count, 4);
    std::memcpy(&r[0x3c], &style, 4);
    std::memcpy(&r[0x40], &spread, 4);
    return r;
}

Burst Burst::parse(const uint8_t *r) {
    Burst b;
    std::memcpy(b.position, &r[0x00], 24);
    std::memcpy(&b.velocity, &r[0x18], 12);
    std::memcpy(&b.colour, &r[0x24], 16);
    std::memcpy(&b.size, &r[0x34], 4);
    std::memcpy(&b.count, &r[0x38], 4);
    std::memcpy(&b.style, &r[0x3c], 4);
    std::memcpy(&b.spread, &r[0x40], 4);
    return b;
}

void Particles::expand(const Burst &burst, const std::function<int()> &draw) {
    const auto u = [&] { return (float)draw() / 32767.f; };
    // ±a, drawn for z, then y, then x.
    const auto jitter = [&](float a) {
        glm::vec3 v;
        v.z = (u() - 0.5f) * 2 * a;
        v.y = (u() - 0.5f) * 2 * a;
        v.x = (u() - 0.5f) * 2 * a;
        return v;
    };
    for (int32_t k = 0; k < burst.count; k++) {
        Particle p;
        p.style = burst.style;
        std::copy(burst.position, burst.position + 3, p.position);
        p.velocity = burst.velocity + jitter(burst.spread);
        p.spin = jitter(50);
        p.colour = burst.colour;
        if (burst.style == FIRE)
            p.colour = {1, u(), 0, 1};
        p.size = burst.size;
        list.push_back(p);
    }
}

void Particles::update(int32_t dtMs, const BlockType &block) {
    if (list.size() > CAP) // §3.2: as Alpha does, the newest go
        list.resize(CAP);
    const float s = (float)dtMs * 0.001f;
    for (Particle &p : list) {
        p.ageMs += dtMs;
        if (p.ageMs < 0 || p.style == STILL)
            continue;
        if (p.style != LEAF)
            p.velocity.z -= 30 * s;
        p.rotation += p.spin * s;
        if (!p.collides) {
            for (int k = 0; k < 3; k++)
                p.position[k] += fixed(p.velocity[k] * s);
            p.rotation += p.spin * s; // a second time (as Alpha does)
            continue;
        }
        // §4 step 5: up to 10 sub-steps, each axis on its own; a blocked axis loses its speed,
        // and the floor stops the particle dead.
        const glm::vec3 d = p.velocity * s;
        const int steps = std::min((int)glm::length(d) + 1, 10);
        const glm::vec3 e = d / (float)steps;
        const auto blocks = [&] {
            const uint8_t type = block(cellOf(p.position[0]), cellOf(p.position[1]), cellOf(p.position[2])) & 0x1f;
            if (type == 0 || type == 2)
                return false;
            return !(p.style == LEAF && (type == 7 || type == 8));
        };
        for (int n = 0; n < steps; n++)
            for (int i = 0; i < 3; i++) {
                const int64_t move = fixed(e[i]);
                p.position[i] += move;
                if (!blocks())
                    continue;
                p.position[i] -= move;
                p.velocity[i] = 0;
                if (i == 2)
                    p.velocity = glm::vec3(0), p.spin = glm::vec3(0);
            }
    }
    list.erase(std::remove_if(list.begin(), list.end(), [](const Particle &p) { return p.ageMs > p.lifetimeMs; }),
               list.end());
}

Burst bombBurst(glm::dvec3 at) { return Burst::at(at, {0, 0, 20}, {1, 0.2f, 0.5f, 1}, 0.25f, 20, FIRE); }

Burst blockBurst(glm::ivec3 cell, glm::vec3 rgb) {
    // At the cell's corner, half a block off its centre (as Alpha does).
    return Burst::at(glm::dvec3(cell), {0, 0, 10}, glm::vec4(rgb / 255.f, 1), 0.5f, 3, LIT);
}

Burst splashBurst(glm::dvec3 at) { return Burst::at(at, {0, 0, 10}, {0.2f, 0.7f, 1, 1}, 0.4f, 15, LIT); }

Burst arrowBurst(glm::dvec3 at, glm::vec3 rgb) {
    return Burst::at(at, {0, 0, 10}, glm::vec4(rgb / 255.f, 1), 0.1f, 4, LIT);
}

std::vector<FlameCube> flames(double t, float radius, float size, int count, glm::vec3 base, glm::vec3 tip, float twist) {
    std::vector<FlameCube> out;
    const double clock = std::max(t, 0.0);
    for (int i = 0; i < count; i++) {
        // A 2 s rise, staggered by 443 ms per flame, swelling in the middle and pulsing at 2 Hz.
        const float f = (float)(std::fmod(clock + 443.0 * i, 2000.0) / 2000.0);
        const float c = (float)std::cos(13.0 * i + 2 * PI * std::fmod(clock, 500.0) / 500.0);
        const float q = 2 * f - 1;
        // On a circle that turns once in 10 s, rising to twice its radius.
        double a = 2 * PI * (i / 10.0 + std::fmod(clock, 10000.0) * 0.0001);
        float across = radius;
        if (twist >= 0) // the swirl: a cone that turns as it rises
            a += 4 * PI * twist * f * f, across = radius * f;
        FlameCube k;
        k.edge = (0.5f * c + 1) * size * (1 - q * q);
        k.colour = base * (1 - f) + tip * f;
        k.position = {across * (float)std::cos(a), across * (float)std::sin(a), 2 * radius * f};
        k.turn = {f * 40, f * 30, f * 10};
        out.push_back(k);
    }
    return out;
}

} // namespace creatures
