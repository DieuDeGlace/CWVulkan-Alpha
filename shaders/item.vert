#version 450
// An item model in the interface (alpha-ui-screens.md §8.1.6): the model in Alpha's camera
// space, projected with a 45 degree lens and shifted so its centre lands on its screen point,
// lit by the directional lights in force after the corner map (§5.1.4 note). No fog, no
// shadows, no point lights.

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColour; // rgb: colour; a: baked light and occlusion
layout(location = 3) in float inSway;

layout(push_constant) uniform Draw {
    mat4 camera; // model -> camera space (x right, y up, z ahead); its unused bottom row carries
                 // a colour (rgb; all zero: white), for attached cubes
    vec4 lens;   // x and y scale, NDC shift
} draw;

layout(location = 0) out vec4 lit;

const vec3 FRONT = vec3(1.0), BACK = vec3(0.2, 0.3, 0.4), AMBIENT = vec3(0.4);
const vec3 DIRECTION = vec3(0.5, 0.4, -0.6) / 0.877496; // normalised

const float NEAR = 0.1, FAR = 1000.0;

void main() {
    vec3 tint = vec3(draw.camera[0][3], draw.camera[1][3], draw.camera[2][3]);
    if (tint == vec3(0.0))
        tint = vec3(1.0);
    mat4 affine = draw.camera;
    affine[0][3] = 0.0;
    affine[1][3] = 0.0;
    affine[2][3] = 0.0;
    vec4 p = affine * vec4(inPosition, 1.0);
    gl_Position = vec4(p.x * draw.lens.x + draw.lens.z * p.z, p.y * draw.lens.y + draw.lens.w * p.z,
                       (p.z - NEAR) * FAR / (FAR - NEAR), p.z);
    vec3 n = normalize(mat3(affine) * inNormal);
    vec3 T = AMBIENT + max(dot(n, DIRECTION), 0.0) * FRONT + max(dot(n, -DIRECTION), 0.0) * BACK;
    lit = vec4(clamp(T, 0.0, 1.2) * inColour.rgb * tint, 1.0);
}
