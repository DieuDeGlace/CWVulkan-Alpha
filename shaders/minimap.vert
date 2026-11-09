#version 450
// The corner map (alpha-ui-screens.md §5.1): the zones' map tiles turned and scaled into a small
// model near the top-right corner, lit with Alpha's sun and sky term, and the markers on it
// (land borders, players) pulled 80 clip units nearer so they draw over the tiles. No fog, no
// shadows.
#extension GL_GOOGLE_include_directive : require
#include "frame.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColour; // rgb: colour; a: baked light and occlusion
layout(location = 3) in float inSway;

layout(push_constant) uniform Draw {
    mat4 world;    // model -> world blocks
    vec4 material; // rgb: material colour; a: 1 tile, 2 creature marker, 3 border marker
} draw;

layout(location = 0) out vec4 lit;

// §5.1.4: the creature markers' own lights.
const vec3 MARKER_FRONT = vec3(1.0), MARKER_BACK = vec3(0.2, 0.3, 0.4), MARKER_AMBIENT = vec3(0.4);
const vec3 MARKER_DIRECTION = vec3(0.0, 0.4472136, 0.8944272); // normalize(0, 0.5, 1)

void main() {
    vec4 world = draw.world * vec4(inPosition, 1.0);
    gl_Position = f.minimap * world;
    float mode = draw.material.a;
    if (mode > 1.5)
        gl_Position.z -= 80.0; // the marker projection (§5.1.3)
    vec3 n = normalize(mat3(draw.world) * inNormal);
    vec3 T;
    if (mode > 1.5 && mode < 2.5) {
        vec3 d = MARKER_DIRECTION;
        T = MARKER_AMBIENT + max(dot(n, d), 0.0) * MARKER_FRONT + max(dot(n, -d), 0.0) * MARKER_BACK;
    } else {
        // The frame's terrain light at material alpha 1 (tile vertices carry alpha 1, §5.1.4).
        vec3 d = f.lightDirection.xyz;
        T = f.ambient.rgb + max(dot(n, d), 0.0) * f.front.rgb + max(dot(n, -d), 0.0) * f.back.rgb;
    }
    vec3 K = clamp(T, 0.0, 1.2) * inColour.rgb * draw.material.rgb * f.minimapTint.rgb;
    lit = vec4(K, 1.0);
}
