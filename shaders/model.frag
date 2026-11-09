#version 450
// Cube World Alpha's world pixel shader (alpha-rendering.md §6.2): haze towards the fog colour
// by distance, then the sky over the far ground, and the shimmer of shiny items. Before that,
// AlphaVulkan's own shadows choose between the lit and the shaded colour (Shadows.h).
#extension GL_GOOGLE_include_directive : require
#include "frame.glsl"

layout(set = 0, binding = 1) uniform sampler2DShadow shadowMap;

layout(location = 0) centroid in vec4 lit;
layout(location = 1) in vec4 fogInputs;
layout(location = 2) in vec3 shaded;
layout(location = 3) in vec3 worldPosition;
layout(location = 4) in vec3 worldNormal;
layout(location = 5) flat in float shine;
layout(location = 6) in vec2 doubledViewNormal;
layout(location = 0) out vec4 outColour;

// Bounded ordinary terrain program from the held shader numeric contract.
// The dot-plus-add expresses the native cubic grouping; device precision is
// still conditional and is not asserted to match a particular D3D9 GPU bitwise.
float terrainCurve(float x) {
    precise float squared = x * x;
    precise float threeSquared = squared * 3.0;
    return dot(vec2(squared), vec2(-x)) + threeSquared;
}

vec3 terrainSky(float vertical) {
    float upperWeight = terrainCurve(saturate(vertical * 0.6));
    vec3 upper = fma(vec3(upperWeight), f.sky2.rgb - f.sky1.rgb, f.sky1.rgb);
    float lowerWeight = terrainCurve(saturate(fma(vertical, 2.0, 1.0)));
    vec3 lower = fma(f.sky1.rgb, vec3(0.2), f.fog.rgb * 0.8);
    return mix(lower, upper, lowerWeight);
}

// How much of the caster's light reaches this point: 1 lit, 0 in full shadow.
float sunlitShare() {
    if (f.shadow.x <= 0.0)
        return 1.0;
    // Look up a point moved one texel out along the face normal: on axis-aligned voxel faces
    // this keeps a face from shadowing itself without lifting shadows off their casters.
    float texel = f.shadow.y;
    vec4 clip = f.lightViewProjection * vec4(worldPosition + normalize(worldNormal) * texel, 1.0);
    vec3 ndc = clip.xyz / clip.w;
    vec2 uv = ndc.xy * 0.5 + 0.5;
    if (any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0))) || ndc.z > 1.0)
        return 1.0;
    // A 3 x 3 filtered lookup: a soft edge about a quarter block wide, closer to the soft
    // baked shade of Alpha than a one-texel step.
    float reference = ndc.z - 0.5 * texel / (2.0 * 420.0); // half a texel of depth bias
    vec2 step = 1.0 / vec2(textureSize(shadowMap, 0));
    float sum = 0.0;
    for (int y = -1; y <= 1; y++)
        for (int x = -1; x <= 1; x++)
            sum += texture(shadowMap, vec3(uv + vec2(x, y) * step * 1.5, reference));
    float share = sum / 9.0;
    // Fade out over the last fifth of the map, so its edge is never seen.
    float distanceShare = length(worldPosition.xy - f.shadowFocus.xy) / f.shadow.w;
    return mix(share, 1.0, smoothstep(0.8, 1.0, distanceShare));
}

void main() {
    vec3 c = mix(shaded, lit.rgb, sunlitShare());

    float Y = fogInputs.y;
    vec3 sky = terrainSky(Y);
    float far = terrainCurve(saturate((fogInputs.w - 0.6) * 2.5));
    float haze = saturate(fma(1.5, fogInputs.z, -0.025));
    float clear = 1.0 - haze;
    haze = fma(clear, -clear, 1.0) * 0.75;
    c = mix(c, f.fog.rgb, haze);
    c = mix(c, sky, far * far);
    if (f.underwater.w > 0.0) // the Omega look under water: deep blue fog
        c = mix(c, f.underwater.rgb, 1.0 - exp(-length(f.camera.xyz - worldPosition) * f.underwater.w));

    // Ordinary native selection uses the sign, after computing the cosine path.
    float phase = dot(vec2(10.0, 5.0), fogInputs.yx) + doubledViewNormal.y;
    phase += doubledViewNormal.x;
    float turn = fma(phase, uintBitsToFloat(0x3e22f983u), 0.5);
    float angle = fma(fract(turn), uintBitsToFloat(0x40c90fdbu), uintBitsToFloat(0xc0490fdbu));
    float oscillation = fma(cos(angle), 0.3, 1.0);
    c = f.params.w > 0.0 || shine > 0.0 ? c * oscillation : c;
    outColour = vec4(c, 1.0);
}
