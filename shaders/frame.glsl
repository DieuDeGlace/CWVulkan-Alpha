// The frame's constants and Cube World Alpha's sky gradient, shared by every shader. Written
// from docs/specs/alpha-rendering.md; the section numbers point there.

layout(set = 0, binding = 0) uniform Frame {
    mat4 viewProjection;    // world blocks -> clip
    mat4 skyViewProjection; // the sky camera: the view's rotation alone, 50 degree lens (§8.4)
    vec4 camera;            // xyz: camera position in world blocks
    vec4 lightDirection;    // xyz: D, from a surface towards the sun (§2.3)
    vec4 front;             // P4, the sun colour (§3.6)
    vec4 back;              // P5 (§3.6)
    vec4 ambient;           // P6 (§3.7)
    vec4 material;          // rgb: material colour; a: sunlight strength S (§4)
    vec4 sky1, sky2, fog;   // P1, P2, P3 (§3)
    vec4 params;            // x: fogScale = 1/F, y: gradient shift (§7.2), z: white, w: shininess
    vec4 sunColour;         // the sun sprite's vertex colour (§8.4)
    vec4 skyRight, skyUp;   // xyz: the view's right and up axes in world space, for sprites
    mat4 lightViewProjection; // world -> shadow map clip space (Shadows.h)
    vec4 shadow;            // x: strength (0 = off), y: map texel in blocks, z: 1 when the moon
                            // casts, w: map radius in blocks
    vec4 shadowFocus;       // xyz: centre of the shadow map
    vec4 moonFront;         // AlphaVulkan's moon (Moon.h): on faces towards it
    vec4 moonBack;          // on faces away from it
    vec4 nightAmbient;      // rgb: the night floor on every face; a: the moon's phase
    vec4 moonSprite;        // xyz: towards the moon; w: how visible its disc is
    vec4 water;             // x, y: near and far plane; z: 0 Alpha look, 1 Omega look; w: seconds
    vec4 underwater;        // the Omega look under water: rgb fog colour, w density (0: off)
    vec4 leaves;            // leaf sway: xy offset now (blocks), z full range, w fade length
    vec4 lightPositions[16]; // §10: xyz position, w radius (0: off)
    vec4 lightColours[16];
    mat4 minimap;           // world blocks -> clip for the corner map (screens §5.1)
    vec4 minimapTint;       // the map's material colour
} f;

struct TerrainLightRow {
    vec4 positions[10];
    vec4 colours[10];
};
layout(std430, set = 0, binding = 5) readonly buffer TerrainLightRows {
    TerrainLightRow terrainLightRows[];
};

vec4 pointLightPosition(int i, uint row) {
    if (row == 0u)
        return f.lightPositions[i];
    return terrainLightRows[row - 1u].positions[i];
}

vec3 pointLightColour(int i, uint row) {
    if (row == 0u)
        return f.lightColours[i].rgb;
    return terrainLightRows[row - 1u].colours[i].rgb;
}

// Held terrain-shader-color-numerics: radius admission is an arithmetic mask
// after the contribution, not a branch around reciprocal/direction operations.
// Degenerate GPU arithmetic remains device-dependent and is not a parity claim.
vec3 pointLightTerm(int i, vec3 world, vec3 n, uint row) {
    vec4 position = pointLightPosition(i, row);
    float radius = position.w;
    vec3 fromLight = world - position.xyz;
    float squaredDistance = dot(fromLight, fromLight);
    vec3 direction = fromLight * inversesqrt(squaredDistance);
    float facing = min(max(dot(n, direction) * -4.0, 0.3), 1.0);
    float inverseRadiusSquared = 1.0 / (radius * radius);
    float fall = 1.0 - min(squaredDistance * inverseRadiusSquared, 1.0);
    float fallSquared = fall * fall;
    return pointLightColour(i, row) * (facing * fallSquared);
}

vec3 pointLights(vec3 world, vec3 n, vec3 directional, float alpha, uint row) {
    vec3 first = pointLightTerm(0, world, n, row) * float(pointLightPosition(0, row).w > 0.0);
    vec3 sum = fma(directional, vec3(alpha), first);
    for (int i = 1; i < 10; i++)
        sum = fma(vec3(float(pointLightPosition(i, row).w > 0.0)), pointLightTerm(i, world, n, row), sum);
    return sum;
}

// AlphaVulkan's leaf sway (Motion.h): leaf corners move by the shared offset, as much as their
// weight says, fading out with distance from the camera.
vec3 swayed(vec3 world, float weight) {
    if (weight <= 0.0)
        return world;
    float d = length(world.xy - f.camera.xy);
    float fade = 1.0 - clamp((d - f.leaves.z) / max(f.leaves.w, 1.0), 0.0, 1.0);
    return world + vec3(f.leaves.xy * weight * fade, 0.0);
}

float saturate(float x) { return clamp(x, 0.0, 1.0); }
// 3x^2 - 2x^3 on an already saturated x.
float smoothCurve(float x) { return x * x * 3.0 - x * x * x * 2.0; }

// §6: the sky gradient for a screen height Y (+1 at the top, -1 at the bottom, shifted with
// the camera's tilt): fog and the horizon colour low down, the zenith colour high up.
vec3 skyAt(float Y) {
    vec3 top = mix(f.sky1.rgb, f.sky2.rgb, smoothCurve(saturate(0.6 * Y)));
    vec3 bottom = 0.2 * f.sky1.rgb + 0.8 * f.fog.rgb;
    return mix(bottom, top, smoothCurve(saturate(2.0 * Y + 1.0)));
}
