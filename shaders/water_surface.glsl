// Steam GPL revised water surface, shaders/omega_water.frag. Upstream manifest in docs/water-extension.md.
layout(set = 0, binding = 3) uniform sampler2D opaqueColour;
layout(set = 0, binding = 2) uniform sampler2D opaqueDepth;
layout(std140, set = 0, binding = 4) uniform Water {
    vec4 planesAndWaves; // near, far, world-origin/time phase offsets
    vec4 body;           // scattering RGB and brightness
    vec4 screen;         // inverse width/height, submerged flag, unused
    vec4 fog;            // world fog RGB and range
} water;

float linearDepth(float depth) {
    return water.planesAndWaves.x * water.planesAndWaves.y /
           (water.planesAndWaves.y - depth * (water.planesAndWaves.y - water.planesAndWaves.x));
}
vec3 revisedNormal(vec3 vWorld, vec3 vNormal, vec3 view) {
    float strength = 1. - smoothstep(40., 120., length(view));
    if (vNormal.z <= .5 || strength <= 0.) return normalize(vNormal);
    const vec2 first = vec2(.65, .31), second = vec2(-.27, .72);
    float a = dot(vWorld.xy, first) + water.planesAndWaves.z;
    float b = dot(vWorld.xy, second) + water.planesAndWaves.w;
    vec2 slope = .12 * cos(a) * first + .11 * cos(b) * second
               + .05 * cos(2.*a+b) * (2.*first+second)
               + .05 * cos(a-2.*b) * (first-2.*second)
               + .025 * cos(3.*a+2.*b) * (3.*first+2.*second);
    return normalize(vec3(-strength * slope, 1));
}
vec4 sharedWaterPixel(vec3 vWorld, vec3 vNormal) {
    vec2 uv = gl_FragCoord.xy * water.screen.xy;
    vec3 view = f.camera.xyz - vWorld;
    vec3 toEye = normalize(view);
    vec3 normal = revisedNormal(vWorld, vNormal, view);
    if (dot(normal, toEye) < 0) normal = -normal;
    vec2 margin = water.screen.xy * .5;
    vec2 refractedUv = clamp(uv + normal.xy * .006, margin, 1 - margin);
    float sceneDepth = linearDepth(texture(opaqueDepth, refractedUv).r);
    float surfaceDepth = (f.viewProjection * vec4(vWorld, 1)).w;
    // A thin surface film still needs a visible blue body over bright grass.
    float thickness = clamp(sceneDepth - surfaceDepth, .3, 100);
    vec3 transmission = exp(-vec3(.60, .45, 1.) * thickness);
    vec3 colour = texture(opaqueColour, refractedUv).rgb * transmission +
                  water.body.rgb * water.body.w * (1 - transmission);
    if (vNormal.z > .5 && water.screen.z < .5) {
        vec3 sun = f.lightDirection.xyz;
        float alignment = clamp(dot(reflect(-toEye, normal), sun), 0., 1.);
        float glint = min(3. * pow(alignment, 300.), 1.) * water.body.w * clamp(5. * sun.z, 0., 1.);
        if (glint > .001) {
            float visibility = 1.;
            if (f.shadow.z < .5 && f.shadow.x > 0.)
                visibility = mix(1., sunlitShare(vWorld, vec3(0, 0, 1)), f.shadow.x);
            colour += vec3(glint * visibility);
        }
    }
    float haze = smoothstep(water.fog.w * .6, max(water.fog.w, .001), length(view));
    colour = mix(colour, water.fog.rgb, haze);
    // Refraction already includes the opaque scene; a second alpha blend would
    // count that background twice. The nearest water surface still writes depth.
    return vec4(clamp(colour, 0, 1), 1);
}
