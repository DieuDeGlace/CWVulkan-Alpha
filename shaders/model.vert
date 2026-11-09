#version 450
// Cube World Alpha's world vertex shader (alpha-rendering.md §5): sun, back and ambient light
// scaled by the sunlight strength and the vertex's own light, then the fog inputs. On top of
// Alpha's light it adds AlphaVulkan's moon (Moon.h), and it works out the colour with the
// shadow caster's direct light taken away (Shadows.h); the pixel shader picks between the two.
#extension GL_GOOGLE_include_directive : require
#include "frame.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColour; // rgb: colour; a: baked light and occlusion (§9)
layout(location = 3) in float inSway;  // leaf sway weight (0 for everything but leaves)

layout(push_constant) uniform Draw {
    mat4 world;    // model -> world blocks
    vec4 material; // this draw's material: rgb times the colour, a times the sun and sky term
    vec4 extra;    // x: shininess (materials §1.2: only its sign matters)
} draw;

layout(location = 0) centroid out vec4 lit; // floating-point colour, without a final colour clamp
layout(location = 1) out vec4 fogInputs; // x, y: screen position (y shifted); z, w: fog distances
layout(location = 2) out vec3 shaded;    // the colour without the caster's direct light
layout(location = 3) out vec3 worldPosition;
layout(location = 4) out vec3 worldNormal;
layout(location = 5) flat out float shine;
layout(location = 6) out vec2 doubledViewNormal;

const vec3 LUMA = vec3(0.299, 0.587, 0.114);
// How far moonlight takes colours towards grey, so a moonlit world reads blue rather than as a
// dim day.
const float MOON_GREY = 0.55;

void main() {
    vec4 world = vec4(swayed((draw.world * vec4(inPosition, 1.0)).xyz, inSway), 1.0);
    vec4 clip = f.viewProjection * world;
    gl_Position = clip;

    // §5.2: screen height for the gradient, divided per vertex. Vulkan's clip y points down.
    fogInputs.x = clip.x / clip.w;
    fogInputs.y = -clip.y / clip.w + f.params.y;
    // §5.4: squared, scaled distances to the camera, all and horizontal only.
    vec3 toCamera = f.camera.xyz - world.xyz;
    float scale2 = f.params.x * f.params.x;
    fogInputs.z = dot(toCamera, toCamera) * scale2;
    fogInputs.w = dot(toCamera.xy, toCamera.xy) * scale2;

    // §5.6: Alpha's sun and sky term, as its three parts, scaled by S and the vertex light.
    vec3 n = normalize(mat3(draw.world) * inNormal);
    vec3 d = f.lightDirection.xyz;
    float baked = inColour.a;
    float scale = f.material.a * draw.material.a * baked;
    float frontFactor = max(dot(n, d), 0.0);
    float backFactor = max(dot(n, -d), 0.0);
    vec3 directional = fma(vec3(frontFactor), f.front.rgb, f.ambient.rgb);
    directional = fma(vec3(backFactor), f.back.rgb, directional);
    vec3 sun = (frontFactor * f.front.rgb) * scale;

    // The moon, opposite the sun, scaled by the vertex light too so caves stay dark.
    vec3 moonDirect = max(dot(n, -d), 0.0) * f.moonFront.rgb * baked;
    vec3 moonFill = (max(dot(n, d), 0.0) * f.moonBack.rgb + f.nightAmbient.rgb) * baked;

    // §5.7: the point lights, not scaled by the vertex light.
    vec3 T = pointLights(world.xyz, n, directional, scale, uint(draw.extra.y));

    // In shadow the caster's direct light goes (the sun by day, the moon by night), scaled by
    // the vertex light so baked shade is not darkened twice (shadows.md §4).
    float shadowAmount = f.shadow.x * baked;
    bool moonCasts = f.shadow.z > 0.5;
    vec3 Tshaded = moonCasts ? T : T - sun * shadowAmount;
    vec3 moonShaded = moonCasts ? moonFill + moonDirect * (1.0 - shadowAmount) : moonFill + moonDirect;

    // §5.8, §5.9: clamp, colour, darkness (black here); moonlight on greyed colour; hit flash.
    vec3 material = f.material.rgb * draw.material.rgb;
    vec3 colour = inColour.rgb * material;
    vec3 moonColour = mix(colour, vec3(dot(colour, LUMA)), MOON_GREY);
    precise vec3 vertexProduct = clamp(T, 0.0, 1.2) * inColour.rgb;
    precise vec3 shadedProduct = clamp(Tshaded, 0.0, 1.2) * inColour.rgb;
    vec3 K = vertexProduct * material + (moonFill + moonDirect) * moonColour;
    vec3 Kshaded = shadedProduct * material + moonShaded * moonColour;
    lit = vec4(fma(vec3(f.params.z), 1.0 - K, K), inColour.a);
    shaded = fma(vec3(f.params.z), 1.0 - Kshaded, Kshaded);
    worldPosition = world.xyz;
    worldNormal = n;
    shine = draw.extra.x;
    // The engine's camera is an orthonormal view rotation. These are the first
    // two view-normal components; upstream matrix equivalence remains open.
    vec3 viewDepth = cross(f.skyRight.xyz, f.skyUp.xyz);
    vec3 viewNormal = normalize(vec3(dot(f.skyRight.xyz, n), dot(f.skyUp.xyz, n), dot(viewDepth, n)));
    doubledViewNormal = 2.0 * viewNormal.xy;
}
