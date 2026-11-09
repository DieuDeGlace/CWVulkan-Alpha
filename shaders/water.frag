#version 450
// Alpha's native water remains selectable; the default surface reuses the
// Steam engine's original shared refraction/reflection shader.
#extension GL_GOOGLE_include_directive : require
#include "frame.glsl"
#include "water_surface.glsl"
layout(location = 0) in vec4 lit;
layout(location = 1) in vec4 fogInputs;
layout(location = 2) in vec3 shaded;
layout(location = 3) in vec3 worldPosition;
layout(location = 4) in vec3 worldNormal;
layout(location = 0) out vec4 outColour;
void main() {
    if (f.water.z > 0.5) {
        outColour = sharedWaterPixel(worldPosition);
        return;
    }
    // Alpha rendering contract, section 6.4.
    vec3 V = f.camera.xyz - worldPosition;
    vec3 H = normalize(normalize(f.lightDirection.xyz + vec3(0, 0, 2)) + normalize(V));
    float glint = pow(saturate(H.z), 10.0) * lit.a;
    float a = saturate(fogInputs.y + 1.0);
    vec3 colour = lit.b * vec3(0.0, 0.6 - 0.5 * a, 1.0) + glint * f.sky1.rgb;
    vec3 c = mix(colour, f.fog.rgb, saturate(fogInputs.z));
    c = mix(c, skyAt(fogInputs.y), saturate(2.0 * fogInputs.z - 1.0));
    outColour = vec4(c, 0.8 + 0.0001 * dot(V, V));
}