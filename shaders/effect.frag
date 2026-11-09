#version 450
// The effect triangles' colour as it comes (see effect.vert).
layout(location = 0) in vec4 colour;
layout(location = 0) out vec4 outColour;

void main() {
    outColour = colour;
}
