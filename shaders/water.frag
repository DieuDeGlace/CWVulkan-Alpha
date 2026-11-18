#version 450
// The Steam engine's shared refraction and reflection surface.
#extension GL_GOOGLE_include_directive : require
#include "frame.glsl"
#include "water_surface.glsl"
layout(location = 3) in vec3 worldPosition;
layout(location = 0) out vec4 outColour;
void main() {
    outColour = sharedWaterPixel(worldPosition);
}
