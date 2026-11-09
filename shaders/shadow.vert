#version 450
// Depth from the shadow caster's side (Shadows.h). Same inputs as model.vert.
#extension GL_GOOGLE_include_directive : require
#include "frame.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 3) in float inSway;

layout(push_constant) uniform Draw {
    mat4 world;
} draw;

void main() {
    // Leaves cast from where they are drawn, so their shadows sway with them.
    gl_Position = f.lightViewProjection * vec4(swayed((draw.world * vec4(inPosition, 1.0)).xyz, inSway), 1.0);
}
