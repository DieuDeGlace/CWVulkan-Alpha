#version 450
// The sky (alpha-rendering.md §6.1): the screen-space gradient alone. The sun and the stars
// are sprites drawn over it (sprite.vert/frag).
#extension GL_GOOGLE_include_directive : require
#include "frame.glsl"

layout(location = 0) in vec2 ndc;
layout(location = 0) out vec4 outColour;

void main() {
    // Vulkan's screen y points down; the gradient's Y points up.
    outColour = vec4(skyAt(-ndc.y + f.params.y), 1.0);
}
