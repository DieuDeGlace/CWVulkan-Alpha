#version 450
// The interface (alpha-ui-plasma.md §5): flat triangles in window pixels, y down, drawn over
// the finished frame.
layout(push_constant) uniform Screen { vec4 size; } screen; // (width, height, -, -)
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec4 inColour;
layout(location = 0) out vec2 vUv;
layout(location = 1) out vec4 vColour;
void main() {
    vUv = inUv;
    vColour = inColour;
    gl_Position = vec4(inPosition / screen.size.xy * 2 - 1, 0, 1);
}
