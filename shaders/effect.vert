#version 450
// Blended effect triangles in the world (alpha-trails-poses-schedules.md §1.3: Alpha draws them
// with the fixed-function pipeline, lighting and texturing off, so each corner is just its
// colour).
#extension GL_GOOGLE_include_directive : require
#include "frame.glsl"

layout(location = 0) in vec3 inPosition; // world blocks
layout(location = 1) in vec4 inColour;

layout(location = 0) out vec4 colour;

void main() {
    colour = inColour;
    gl_Position = f.viewProjection * vec4(inPosition, 1.0);
}
