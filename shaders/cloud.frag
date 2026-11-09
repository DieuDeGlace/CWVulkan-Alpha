#version 450
// Cube World Alpha's cloud pixel shader (alpha-rendering.md §6.3): clouds fade into the sky with
// distance, more sharply than the ground does.
#extension GL_GOOGLE_include_directive : require
#include "frame.glsl"

layout(location = 0) in vec4 lit;
layout(location = 1) in vec4 fogInputs;
layout(location = 0) out vec4 outColour;

void main() {
    float fade = saturate(fogInputs.z);
    float f5 = fade * fade * fade * fade * fade;
    float g = 1.0 - (1.0 - f5) * (1.0 - f5);
    outColour = vec4(mix(lit.rgb, skyAt(fogInputs.y), g), 1.0);
}
