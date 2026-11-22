#pragma once
// The constants one frame is drawn with: the camera and Cube World Alpha's look for the time of
// day (alpha-rendering.md §2-§4, §7, §8). The layout matches `Frame` in shaders/frame.glsl.
#include "AlphaLook.h"
#include "Moon.h"
#include "Projection.h"
#include "Shadows.h"
#include "TerrainLights.h"
#include <glm/glm.hpp>
#include <array>
#include <vector>
#include <optional>

struct Camera {
    static constexpr float NEAR_PLANE = .1f, FAR_PLANE = 4000.f;
    glm::vec3 eye{0};
    float yawDegrees = 0;   // 0 looks along +y; positive turns towards +x
    float pitchDegrees = 0; // positive looks down
    float lensDegrees = 45; // vertical field of view
    float aspect = 16.f / 9.f;

    glm::vec3 forward() const;
    glm::mat4 view() const;
    glm::mat4 projection(float nearPlane, float farPlane, float lens) const;
    // The main renderer's lens from the held projection-input contract: the
    // followed creature's two-byte mode word selects the angle, and signed
    // pixel dimensions give the aspect. Without it the engine lens above is used.
    struct MainLens {
        uint16_t modeWord;
        int32_t width, height;
    };
    std::optional<MainLens> mainLens;
    // The constructed native view as four stored rows, already relative to the
    // render origin. view() applies the explicit right-handed adapter, which
    // mirrors the view x and z axes by sign-bit inversion.
    std::optional<std::array<std::array<float, 4>, 4>> nativeView;
    glm::ivec2 renderOrigin{0}; // absolute horizontal block coordinates of the rendered origin
    projection::Inputs mainInputs() const;
    // The main projection through the explicit right-handed Vulkan adapter.
    glm::mat4 mainProjection() const;
    // Raw extraction applied to the adapted engine camera matrices. Production
    // angle/near/far selection, native following and references remain separate.
    std::array<glm::vec4, 6> lightAdmissionPlanes() const;
};

// Rendering §10: a point light (street lights, torches, campfires, glowing items), in
// render-origin blocks.
struct PointLight {
    glm::vec3 position{0};
    float radius = 0;
    glm::vec3 colour{0};
    std::optional<terrain_lights::FixedPosition> fixedPosition;
};
PointLight pointLightAt(glm::dvec3 absolute, glm::vec3 relative, float radius, glm::vec3 colour);
// Global fallback for model draws; terrain keeps separate per-chunk selections.
constexpr int FRAME_LIGHTS = 16;

// Where a world draw falls in the frame (alpha-rendering.md §1.3, rulings §16):
//   Opaque       the objects pass (statics, items, opaque creatures), depth writes on
//   Ghost        a motion-trail ghost: in the objects pass, blended, depth writes off
//   Effect       particles after the objects, depth writes off (§1.3 step 15)
//   Translucent  a creature body queued until after the water (§1.3 step 18)
enum class DrawLayer : uint8_t { Opaque, Ghost, Effect, Translucent };

// Rulings §13: the point lights draws after the water see. No upload follows the water
// loop, so they keep the last drawn water sub-mesh's chunk row, or the frame list (row 0)
// when no water was drawn. `drawnRow` is that last water draw's row, if any.
inline uint32_t postWaterLightRow(std::optional<uint32_t> drawnRow) { return drawnRow.value_or(0); }

struct FrameConstants {
    glm::mat4 viewProjection;
    glm::mat4 skyViewProjection;
    glm::vec4 camera;
    glm::vec4 lightDirection;
    glm::vec4 front, back, ambient;
    glm::vec4 material;
    glm::vec4 sky1, sky2, fog;
    glm::vec4 params; // fogScale, gradient shift, white, shininess
    glm::vec4 sunColour;
    glm::vec4 skyRight, skyUp;
    glm::mat4 lightViewProjection;
    glm::vec4 shadow; // strength (0 = off), texel in blocks, 1 when the moon casts, radius
    glm::vec4 shadowFocus;
    glm::vec4 moonFront, moonBack; // Moon.h colours, already scaled by its strength
    glm::vec4 nightAmbient;        // rgb: the night floor; a: the moon's phase
    glm::vec4 moonSprite;          // xyz: towards the moon; w: how visible its disc is
    glm::vec4 water;               // near, far, wrapped water wave phases
    glm::vec4 underwater;          // submerged fog colour and flag
    glm::vec4 leaves;              // leaf sway: offset now (xy), full range, fade length
    glm::vec4 lightPositions[FRAME_LIGHTS]; // xyz: position; w: radius (0: off)
    glm::vec4 lightColours[FRAME_LIGHTS];
    glm::mat4 minimap;     // world blocks -> clip for the corner map (screens §5.1)
    glm::vec4 minimapTint; // its material colour
};

struct Scene {
    double timeOfDayMs = 12 * 3600000.0;
    alpha::ZoneFactors zone;
    bool underwater = false;
    float fogDistance = 640; // F (§7.1); 640 blocks is the default render radius
    std::optional<float> currentFogAngle; // the followed camera's current first rotation, in degrees
    bool mapFog = false;
    glm::vec3 focus{0};      // what the shadow map is centred on (the player, in play)
    int day = 0;             // world day, for the moon's phase
    double seconds = 0;      // monotonic water clock in seconds
    glm::vec2 leafSway{0};   // this frame's leaf offset (motion::leafOffset)
    float leafRange = 96, leafFade = 32;
    std::vector<PointLight> lights; // collected, then jointly ordered before frame construction
};

// Screens §5.1.3: blocks (render origin) to clip space for the corner map, centred at pixel
// (W - 240, 170), turned with the camera yaw, seen from 30 degrees above. `scale` is the view
// scale s (0.8 for 3 x 3 zones); `pivot` is the player's position.
glm::mat4 minimapMatrix(glm::vec3 pivot, float cameraYawDegrees, float scale, float width, float height);
// The same map renderer's matrices in general (screens §5.1.3, §5.2.3): the miniature tilted by
// `tiltDegrees` (120 on the corner map) and centred at pixel `centre`.
glm::mat4 mapMatrix(glm::vec3 pivot, float cameraYawDegrees, float tiltDegrees, float scale, float width, float height,
                    glm::vec2 centre);

FrameConstants makeFrame(const Camera &camera, const Scene &scene, const ShadowSettings &shadows,
                         const MoonSettings &moonSettings);
// Stars are drawn while S < 0.5 and the camera is dry (§8.3).
bool drawStars(const Scene &scene);
