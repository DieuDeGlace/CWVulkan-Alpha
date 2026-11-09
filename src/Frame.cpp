#include "Frame.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

PointLight pointLightAt(glm::dvec3 absolute, glm::vec3 relative, float radius, glm::vec3 colour) {
    return {relative, radius, colour, terrain_lights::fromBlocks(absolute)};
}

glm::vec3 Camera::forward() const {
    const float yaw = glm::radians(yawDegrees), pitch = glm::radians(pitchDegrees);
    return {std::cos(pitch) * std::sin(yaw), std::cos(pitch) * std::cos(yaw), -std::sin(pitch)};
}

glm::mat4 Camera::view() const { return glm::lookAt(eye, eye + forward(), glm::vec3(0, 0, 1)); }

glm::mat4 Camera::projection(float nearPlane, float farPlane, float lens) const {
    glm::mat4 p = glm::perspective(glm::radians(lens), aspect, nearPlane, farPlane);
    p[1][1] *= -1; // Vulkan's clip space has y pointing down
    return p;
}

std::array<glm::vec4, 6> Camera::viewPlanes() const {
    const glm::mat4 rows = glm::transpose(projection(NEAR_PLANE, FAR_PLANE, lensDegrees) * view());
    std::array<glm::vec4, 6> planes{rows[3] + rows[0], rows[3] - rows[0], rows[3] + rows[1],
                                    rows[3] - rows[1], rows[2], rows[3] - rows[2]};
    for (auto &plane : planes)
        plane /= glm::length(glm::vec3(plane));
    return planes;
}

FrameConstants makeFrame(const Camera &camera, const Scene &scene, const ShadowSettings &shadowSettings,
                         const MoonSettings &moonSettings) {
    const float L = alpha::paletteDaylight(scene.timeOfDayMs);
    const float S = alpha::sunlight(scene.timeOfDayMs);
    const alpha::Palette palette = alpha::palette(L, scene.zone, scene.underwater);
    const glm::mat4 view = camera.view();

    FrameConstants c{};
    constexpr float NEAR_PLANE = Camera::NEAR_PLANE, FAR_PLANE = Camera::FAR_PLANE;
    c.viewProjection = camera.projection(NEAR_PLANE, FAR_PLANE, camera.lensDegrees) * view;
    // §8.4: the sky camera keeps the view's rotation, sits at the origin, and has its own lens.
    glm::mat4 rotation = view;
    rotation[3] = glm::vec4(0, 0, 0, 1);
    c.skyViewProjection = camera.projection(0.1f, 2000.f, alpha::SKY_LENS_DEGREES) * rotation;
    c.camera = glm::vec4(camera.eye, 1);
    c.lightDirection = glm::vec4(alpha::sunDirection(scene.timeOfDayMs), 0);
    c.front = glm::vec4(palette.front, 1);
    c.back = glm::vec4(palette.back, 1);
    c.ambient = glm::vec4(palette.ambient, 1);
    c.material = alpha::material(S, scene.underwater);
    c.sky1 = glm::vec4(palette.sky1, 1);
    c.sky2 = glm::vec4(palette.sky2, 1);
    c.fog = glm::vec4(palette.fog, 1);
    // §7.2: the gradient follows the camera's angle from straight up (90 = level).
    const float zenith = 90 + camera.pitchDegrees;
    c.params = glm::vec4(scene.fogDistance > 0 ? 1 / scene.fogDistance : 0, alpha::fogGradientTranslation(zenith), 0, 0);
    c.sunColour = alpha::sunSpriteColour(S);
    // The view's right and up axes are the first two rows of its rotation.
    c.skyRight = glm::vec4(view[0][0], view[1][0], view[2][0], 0);
    c.skyUp = glm::vec4(view[0][1], view[1][1], view[2][1], 0);

    // The caster follows the stepped clock, so its direction moves a game minute at a time.
    const moon::ShadowCaster caster =
        moon::shadowCaster(shadows::steppedTime(scene.timeOfDayMs), shadowSettings.strength, moonSettings);
    c.lightViewProjection =
        shadows::lightMatrix(scene.focus, caster.direction, shadowSettings.radius, shadowSettings.resolution);
    const float texel = 2 * shadowSettings.radius / (float)shadowSettings.resolution;
    c.shadow = glm::vec4(shadowSettings.enabled && !scene.underwater ? caster.strength : 0, texel,
                         caster.moon ? 1 : 0, shadowSettings.radius);
    c.shadowFocus = glm::vec4(scene.focus, 1);

    const moon::Light moonLight = moon::light(scene.timeOfDayMs, moonSettings);
    c.moonFront = glm::vec4(moonLight.front, 0);
    c.moonBack = glm::vec4(moonLight.back, 0);
    c.nightAmbient = glm::vec4(moonLight.ambient, moon::phase(scene.day));
    const float disc = moonSettings.enabled && !scene.underwater ? moon::discVisibility(scene.timeOfDayMs) : 0;
    c.moonSprite = glm::vec4(moon::direction(scene.timeOfDayMs), disc);
    c.water = glm::vec4(NEAR_PLANE, FAR_PLANE, scene.water == WaterLook::Omega ? 1.f : 0.f,
                        (float)std::fmod(scene.seconds, 3600.0));
    if (scene.underwater && scene.water == WaterLook::Omega) {
        // The Omega look under water (references: water-underwater-reference.webp): the ground
        // turns teal instead of Alpha's blue, and deep blue fog closes in within a few dozen
        // blocks. It dims with the daylight, never quite to black.
        c.material = glm::vec4(0.45f, 0.82f, 0.85f, S);
        c.underwater = glm::vec4(glm::vec3(0.02f, 0.12f, 0.45f) * std::max(S, 0.15f), 0.045f);
    }
    c.leaves = glm::vec4(scene.leafSway, scene.leafRange, scene.leafFade);
    for (int i = 0; i < FRAME_LIGHTS; i++) {
        const bool on = i < (int)scene.lights.size();
        c.lightPositions[i] = on ? glm::vec4(scene.lights[i].position, scene.lights[i].radius) : glm::vec4(0);
        c.lightColours[i] = on ? glm::vec4(scene.lights[i].colour, 0) : glm::vec4(0);
    }
    return c;
}

glm::mat4 minimapMatrix(glm::vec3 pivot, float yawDegrees, float s, float W, float H) {
    return mapMatrix(pivot, yawDegrees, 120, s, W, H, {W - 240, 170});
}

glm::mat4 mapMatrix(glm::vec3 pivot, float yawDegrees, float tilt, float s, float W, float H, glm::vec2 centre) {
    // Map units are 8 blocks, centred on the player (§5.1.1).
    const glm::mat4 toMap = glm::scale(glm::mat4(1), glm::vec3(1.f / 8)) * glm::translate(glm::mat4(1), -pivot);
    // §5.1.3 view: about z by -psi, about x by -120 degrees, times s, then 300 deep. Alpha's
    // camera yaw psi = 0 looks along -y; ours looks along +y and turns towards +x.
    const float psi = glm::radians(180.f - yawDegrees);
    const glm::mat4 turnZ = glm::rotate(glm::mat4(1), -psi, glm::vec3(0, 0, 1));
    const glm::mat4 turnX = glm::rotate(glm::mat4(1), glm::radians(-tilt), glm::vec3(1, 0, 0));
    glm::mat4 view = glm::translate(glm::mat4(1), glm::vec3(0, 0, 300)) * glm::scale(glm::mat4(1), glm::vec3(s)) * turnX * turnZ;
    // §5.1.3 projection, written for column vectors; Vulkan's clip y points down.
    const float ys = 1.f / std::tan(glm::radians(22.5f)), xs = -ys / (W / H);
    // (y0 counts up from the bottom, as Direct3D's NDC: the corner map's 170 px from the top.)
    const float ox = 2 * (centre.x - W / 2) / W, oy = 2 * ((H - centre.y) - H / 2) / H;
    glm::mat4 P(0);
    P[0][0] = xs;
    P[1][1] = ys;
    P[2][0] = ox, P[2][1] = oy, P[2][2] = 0.9001f, P[2][3] = 1;
    P[3][2] = -0.10001f;
    glm::mat4 flipY(1);
    flipY[1][1] = -1;
    return flipY * P * view * toMap;
}

bool drawStars(const Scene &scene) {
    return alpha::starsVisible(alpha::sunlight(scene.timeOfDayMs), scene.underwater);
}
