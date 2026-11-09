#version 450
// Sun, moon and star colours (alpha-rendering.md §8.3, §8.4; the moon is AlphaVulkan's own,
// carried over from CubeVulkan: a pale pixel-art disc with a few darker craters, a faint halo,
// and its phase).
#extension GL_GOOGLE_include_directive : require
#include "frame.glsl"

layout(location = 0) in vec2 corner;
layout(location = 1) flat in int kind;
layout(location = 0) out vec4 outColour;

// The moon is drawn on a coarse grid so it reads as pixel art, like the blocks around it.
const float CELLS = 24; // grid cells across the moon's square
const float DISC = 0.6; // the disc's radius (the square is -1..1)
const vec3 MOON = vec3(0.90, 0.93, 1.0);
const vec3 CRATER = vec3(0.66, 0.70, 0.84);
// Crater spots on the disc: xy centre, z radius (disc units, -1..1 across the disc).
const vec3 CRATERS[5] = vec3[](vec3(-0.35, 0.30, 0.26), vec3(0.30, 0.42, 0.16), vec3(0.20, -0.30, 0.30),
                               vec3(-0.42, -0.35, 0.14), vec3(0.52, -0.02, 0.12));

vec4 moonColour() {
    // The centre of this pixel's grid cell, in square units (-1..1).
    vec2 cell = (floor((corner * 0.5 + 0.5) * CELLS) + 0.5) / CELLS * 2.0 - 1.0;
    float r = length(cell);
    if (r > DISC) {
        // A faint cool halo round the disc.
        float halo = 1.0 - clamp((r - DISC) / (1.0 - DISC), 0.0, 1.0);
        return vec4(0.55, 0.65, 0.9, 0.18 * halo * halo);
    }
    vec2 p = cell / DISC; // on the disc, -1..1
    vec3 c = MOON;
    for (int i = 0; i < 5; i++)
        if (length(p - CRATERS[i].xy) < CRATERS[i].z)
            c = CRATER;
    c *= 1.0 - 0.12 * smoothstep(0.75, 1.0, length(p)); // a slightly darker rim

    // The phase: the lit part grows from the right (waxing) to full, then shrinks to the left
    // (waning). The dark part keeps a dim earthshine so the disc never vanishes.
    float phase = f.nightAmbient.a;
    float edge = cos(phase * 6.2831853) * sqrt(max(1.0 - p.y * p.y, 0.0));
    bool litSide = phase < 0.5 ? p.x > edge : p.x < -edge;
    if (!litSide)
        c = c * 0.18 + vec3(0.02, 0.03, 0.06);
    return vec4(c, 1.0);
}

void main() {
    if (kind == 0) {
        // The sun texture: q = clamp(1.05 - 1.05 r / 128, 0, 1)^4 with r in texels from the
        // middle of a 256-texel square; white with a yellow rim, q as its alpha. Modulated by
        // the sprite's vertex colour.
        float r = length(corner) * 128.0;
        float q = pow(saturate(1.05 - 1.05 * 0.0078125 * r), 4.0);
        outColour = vec4(1.0, 1.0, (100.0 + 155.0 * q) / 255.0, q) * f.sunColour;
    } else if (kind == 1) {
        vec4 m = moonColour();
        outColour = vec4(m.rgb, m.a * f.moonSprite.w);
    } else {
        // A star is a fan of four triangles: opaque white middle, clear corners. Across each
        // triangle that is 1 - the larger of |x| and |y|.
        outColour = vec4(1.0, 1.0, 1.0, 1.0 - max(abs(corner.x), abs(corner.y)));
    }
}
