#version 450
// Vertex colour times the atlas (glyphs, the star textures, and a white texel for plain fills).
layout(set = 0, binding = 0) uniform sampler2D atlas;
layout(location = 0) in vec2 vUv;
layout(location = 1) in vec4 vColour;
layout(location = 0) out vec4 outColor;
void main() { outColor = vColour * textureLod(atlas, vUv, 0); }
