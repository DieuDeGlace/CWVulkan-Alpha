#version 450
// The Steam engine's revised refraction and solar-glint surface.
#extension GL_GOOGLE_include_directive : require
#include "frame.glsl"
#include "shadow_share.glsl"
#include "water_surface.glsl"
layout(location = 3) in vec3 worldPosition;
layout(location = 4) in vec3 worldNormal;
layout(location = 0) out vec4 outColour;
void main() {
    outColour = sharedWaterPixel(worldPosition, worldNormal);
}
