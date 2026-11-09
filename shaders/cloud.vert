#version 450
// Cube World Alpha's world vertex shader with the cloud pass's constants (alpha-rendering.md §4
// cloud row, §5): the sun colour lights both sides, the zone's ambient fills in, and where the
// light falls short the colour goes towards the sky's average (the darkness colour). The fog
// reaches 1.5 times as far as the world's.
#extension GL_GOOGLE_include_directive : require
#include "frame.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColour;

layout(push_constant) uniform Draw {
    mat4 world;
} draw;

layout(location = 0) out vec4 lit;
layout(location = 1) out vec4 fogInputs;

const float CLOUD_FOG_REACH = 1.5;

void main() {
    vec4 world = draw.world * vec4(inPosition, 1.0);
    vec4 clip = f.viewProjection * world;
    gl_Position = clip;
    fogInputs.x = clip.x / clip.w;
    fogInputs.y = -clip.y / clip.w + f.params.y;
    vec3 toCamera = f.camera.xyz - world.xyz;
    float scale = f.params.x / CLOUD_FOG_REACH;
    fogInputs.z = dot(toCamera, toCamera) * scale * scale;
    fogInputs.w = dot(toCamera.xy, toCamera.xy) * scale * scale;

    vec3 n = normalize(mat3(draw.world) * inNormal);
    vec3 d = f.lightDirection.xyz;
    vec3 T = f.ambient.rgb + max(dot(n, d), 0.0) * f.front.rgb + max(dot(n, -d), 0.0) * f.front.rgb;
    T *= f.material.a * inColour.a;
    T = clamp(T, 0.0, 1.2);
    vec3 K = T * inColour.rgb * f.material.rgb;
    vec3 darkness = (f.sky1.rgb + f.sky2.rgb) * 0.5;
    K = darkness * (1.0 - T) + K;
    lit = vec4(K, inColour.a);
}
