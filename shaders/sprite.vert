#version 450
// The sun, the moon and the stars (alpha-rendering.md §8.3, §8.4; the moon is AlphaVulkan's
// own, Moon.h): squares around a point 100 blocks out, made in the sky camera's view space and
// then put through its lens. Instance 0 is the sun, 1 the moon, the others stars.
#extension GL_GOOGLE_include_directive : require
#include "frame.glsl"

layout(location = 0) in vec4 inSprite; // xyz: point on the sky sphere; w: half-size

layout(location = 0) out vec2 corner; // -1..1 across the square
layout(location = 1) flat out int kind; // 0 sun, 1 moon, 2 star

const vec2 CORNERS[6] = vec2[](vec2(-1, -1), vec2(1, -1), vec2(1, 1), vec2(-1, -1), vec2(1, 1), vec2(-1, 1));

void main() {
    corner = CORNERS[gl_VertexIndex];
    kind = min(gl_InstanceIndex, 2);
    // The sky camera matrix is lens x rotation, so a square in the view's own plane is made by
    // moving the corners along the view's right and up axes in world space.
    vec4 centre = f.skyViewProjection * vec4(inSprite.xyz, 1.0);
    vec4 right = f.skyViewProjection * vec4(f.skyRight.xyz, 0.0);
    vec4 up = f.skyViewProjection * vec4(f.skyUp.xyz, 0.0);
    gl_Position = centre + (corner.x * right + corner.y * up) * inSprite.w;
}
