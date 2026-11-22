layout(set = 0, binding = 1) uniform sampler2DShadow shadowMap;

// How much of the caster's light reaches this point: 1 lit, 0 in full shadow.
float sunlitShare(vec3 worldPosition, vec3 worldNormal) {
    if (f.shadow.x <= 0.0)
        return 1.0;
    // Look up a point moved one texel out along the face normal: on axis-aligned voxel faces
    // this keeps a face from shadowing itself without lifting shadows off their casters.
    float texel = f.shadow.y;
    vec4 clip = f.lightViewProjection * vec4(worldPosition + normalize(worldNormal) * texel, 1.0);
    vec3 ndc = clip.xyz / clip.w;
    vec2 uv = ndc.xy * 0.5 + 0.5;
    if (any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0))) || ndc.z > 1.0)
        return 1.0;
    // A 3 x 3 filtered lookup: a soft edge about a quarter block wide, closer to the soft
    // baked shade of Alpha than a one-texel step.
    float reference = ndc.z - 0.5 * texel / (2.0 * 420.0); // half a texel of depth bias
    vec2 step = 1.0 / vec2(textureSize(shadowMap, 0));
    float sum = 0.0;
    for (int y = -1; y <= 1; y++)
        for (int x = -1; x <= 1; x++)
            sum += texture(shadowMap, vec3(uv + vec2(x, y) * step * 1.5, reference));
    float share = sum / 9.0;
    // Fade out over the last fifth of the map, so its edge is never seen.
    float distanceShare = length(worldPosition.xy - f.shadowFocus.xy) / f.shadow.w;
    return mix(share, 1.0, smoothstep(0.8, 1.0, distanceShare));
}
