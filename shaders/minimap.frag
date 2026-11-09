#version 450
// The corner map's pixels: the lit colour as it is (alpha-ui-screens.md §5.1).
layout(location = 0) in vec4 lit;
layout(location = 0) out vec4 outColour;

void main() { outColour = lit; }
