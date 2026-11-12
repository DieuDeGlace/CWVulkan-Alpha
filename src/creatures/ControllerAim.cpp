#include "creatures/ControllerAim.h"
#include "creatures/Npcs.h"
#include <algorithm>
#include <bit>
#include <cmath>

namespace creatures::controller_aim {

namespace {

using namespace camera_view;

int64_t wrapAdd(int64_t a, int64_t b) { return std::bit_cast<int64_t>(uint64_t(a) + uint64_t(b)); }
int64_t wrapSub(int64_t a, int64_t b) { return std::bit_cast<int64_t>(uint64_t(a) - uint64_t(b)); }

// Precise normalization: binary64 square root of the binary32 squared length.
glm::vec3 normalized(glm::vec3 v) {
    const float squared = v.x * v.x + v.y * v.y + v.z * v.z;
    const float length = float(std::sqrt(double(squared)));
    return {v.x / length, v.y / length, v.z / length};
}

// A point through the stored rows: x*row0 + y*row1 + z*row2 + row3.
glm::vec3 transformPoint(const Matrix &m, glm::vec3 p) {
    glm::vec3 out;
    for (int k = 0; k < 3; k++)
        out[k] = p.x * m[0][size_t(k)] + p.y * m[1][size_t(k)] + p.z * m[2][size_t(k)] + m[3][size_t(k)];
    return out;
}

// Fixed self dot product, each square divided by 65,536, then the float conversion.
float squaredDistance(const Fixed3 &a, const Fixed3 &b) {
    int64_t sum = 0;
    for (size_t k = 0; k < 3; k++) {
        const uint64_t d = uint64_t(wrapSub(a[k], b[k]));
        sum = wrapAdd(sum, std::bit_cast<int64_t>(d * d) / 65536);
    }
    return float(sum) * (1.f / 65536.f);
}

bool oneOf(int32_t action, std::initializer_list<int32_t> list) {
    return std::find(list.begin(), list.end(), action) != list.end();
}

} // namespace

AimPoint aimPoint(const CameraState &camera, const Fixed3 &playerPosition, const Combatant &player,
                  std::span<const Candidate> candidates, const visibility::Scene &scene) {
    const glm::vec3 V = normalized(camera.forward);
    AimPoint out;
    // §3.1: 100 units along V from the follow anchor, or where the 60-unit ray stops.
    out.point = add(camera.anchor, toFixed(V * 100.f));
    const float L = visibility::trace(camera.anchor, {V.x, V.y, V.z}, 60.f, 0, 1, scene);
    if (L >= 0.f) // (an unordered result counts as less)
        out.point = add(camera.anchor, toFixed(V * L));

    // §3.2: the creatures whose box straddles the screen centre.
    const float R0 = float(duration(player, player.action));
    float hovered = -1.f, admitted = -1.f;
    for (const Candidate &k : candidates) {
        if (!k.fighter || k.fighter->id == player.id)
            continue;
        if (player.action == 0x22 && player.actionMs < R0 && (k.fighter->controller == 6 || k.fighter->controller == 1))
            continue;
        if (!visibility::visible(camera.eye, k.point, 0, 200.f, scene))
            continue;
        const glm::vec3 centre = toFloat(subtract(k.point, camera.origin));
        const glm::vec3 half = (k.size + 1.f) * 0.5f;
        bool negX = false, negY = false, posX = false, posY = false;
        for (int a = 0; a < 2; a++)
            for (int b = 0; b < 2; b++)
                for (int c = 0; c < 2; c++) {
                    const glm::vec3 corner(a ? centre.x + half.x : centre.x - half.x, b ? centre.y + half.y : centre.y - half.y,
                                           c ? centre.z + half.z : centre.z - half.z);
                    const glm::vec3 v = transformPoint(camera.view, corner);
                    if (!(v.z > 0.f))
                        continue;
                    negX |= v.x < 0.f, negY |= v.y < 0.f, posX |= v.x > 0.f, posY |= v.y > 0.f;
                }
        if (!(negX && negY && posX && posY))
            continue;
        const float d2 = squaredDistance(camera.eye, k.point);
        if (hovered < 0.f || d2 < hovered) {
            hovered = d2;
            out.hovered = k.fighter->id;
        }
        if ((hostile(player, *k.fighter) || hostile(*k.fighter, player)) && (admitted < 0.f || d2 < admitted))
            admitted = d2;
    }
    // The override: the nearest admitted creature's depth from the eye, if in front of the player.
    if (!(admitted < 0.f) && player.action != 0x31) {
        const float r = float(std::sqrt(double(admitted)));
        const Fixed3 Q = add(camera.eye, toFixed(V * r));
        const glm::vec3 toQ = toFloat(subtract(Q, playerPosition));
        if (V.x * toQ.x + V.y * toQ.y + V.z * toQ.z > 0.f)
            out.point = Q;
    }
    return out;
}

void freezeSpellPoint(Fixed3 &point, Fixed3 &held, const Combatant &player, int32_t elapsedMs) {
    if (player.action < 0x1E || player.action > 0x21)
        return;
    if (!(player.actionMs - float(elapsedMs) > float(phases(player, player.action).windUp)))
        held = point;
    else
        point = held;
}

glm::vec3 freeAim(bool shift, float yawDegrees, const Fixed3 &point, const Fixed3 &playerPosition) {
    if (shift) {
        const float y = yawDegrees * 0.017453292f;
        const float c = float(std::cos(double(y))), s = float(std::sin(double(y)));
        return {-s, c, 0.f};
    }
    return toFloat(subtract(point, playerPosition));
}

uint32_t flags(uint32_t mode, const Combatant &player, const FlagInputs &in) {
    mode &= ~uint32_t(AIMING);
    const bool fast = in.gliding ? in.glideLift : !in.shift;
    mode = fast ? mode | RUN : mode & ~uint32_t(RUN);
    const int32_t action = player.action;
    const bool excluded = oneOf(action, {0x6D, 0x53, 0x6A, 0x6B, 0x69, 0x52, 0x22, 0x54, 0x50, 0x51, 0x4F, 0x0B});
    const bool sustained = oneOf(action, {0x18, 0x19, 0x1B, 0x08, 0x24, 0x40, 0x3B, 0x3F, 0x1C, 0x23});
    const float R0 = float(duration(player, action));
    if (!in.shift && !excluded && (player.actionMs < R0 + 200.f || sustained)) {
        if (!oneOf(action, {0x58, 0x57, 0x59, 0x5C}))
            mode |= AIMING;
        mode &= ~uint32_t(RUN);
    }
    return in.ctrl ? mode | CLIMB : mode & ~uint32_t(CLIMB);
}

} // namespace creatures::controller_aim
