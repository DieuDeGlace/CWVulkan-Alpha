#version 450
// Owner's Infinite Cubes Vulkan tests temporal resolve, 71611c67, adapted to Alpha's
// positive-height viewport and GLSL bindings. History is stored on the output grid.
layout(set = 0, binding = 0) uniform sampler2D scene;
layout(set = 0, binding = 1) uniform sampler2D history;
layout(set = 0, binding = 2) uniform sampler2D depthTex;
layout(set = 0, binding = 3) uniform sampler2D previousDepth;
layout(push_constant) uniform Resolve {
    vec4 a;              // weight, valid, jitterX, jitterY (pixels)
    vec4 b;              // blur strength, edge weight, unused, unused
    mat4 clipToPrevious; // current unjittered clip -> previous unjittered clip
    vec4 previousJitter; // xy: last frame's jitter in pixels
} pc;
layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 outColor;

float maxChannel(vec3 v) { return max(v.r, max(v.g, v.b)); }

vec3 historyAt(vec2 uv, vec2 size) {
    vec2 position = uv * size, center = floor(position - .5) + .5, f = position - center;
    vec2 w0 = f * (-.5 + f * (1 - .5 * f)), w1 = 1 + f * f * (-2.5 + 1.5 * f), w2 = f * (.5 + f * (2 - 1.5 * f)),
         w3 = f * f * (-.5 + .5 * f);
    vec2 w12 = w1 + w2, t0 = (center - 1) / size, t3 = (center + 2) / size, t12 = (center + w2 / w12) / size;
    vec3 sum = textureLod(history, vec2(t12.x, t0.y), 0).rgb * (w12.x * w0.y) +
               textureLod(history, vec2(t0.x, t12.y), 0).rgb * (w0.x * w12.y) +
               textureLod(history, t12, 0).rgb * (w12.x * w12.y) +
               textureLod(history, vec2(t3.x, t12.y), 0).rgb * (w3.x * w12.y) +
               textureLod(history, vec2(t12.x, t3.y), 0).rgb * (w12.x * w3.y);
    float total = w12.x * w0.y + w0.x * w12.y + w12.x * w12.y + w3.x * w12.y + w12.x * w3.y;
    return max(sum / total, vec3(0));
}

void main() {
    ivec2 size = textureSize(scene, 0);
    vec2 invSize = 1. / vec2(size);
    vec2 outputUV = gl_FragCoord.xy * invSize;
    vec2 uv = outputUV + pc.a.zw * invSize;
    vec4 current = textureLod(scene, uv, 0);
    if (pc.a.y < .5) {
        outColor = current;
        return;
    }

    vec3 lo = current.rgb, hi = current.rgb, axisSum = vec3(0);
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            if (x == 0 && y == 0) continue;
            vec3 neighbour = textureLod(scene, uv + vec2(x, y) * invSize, 0).rgb;
            lo = min(lo, neighbour);
            hi = max(hi, neighbour);
            if (abs(x) + abs(y) == 1) axisSum += neighbour;
        }
    }

    float depth = texelFetch(depthTex, ivec2(gl_FragCoord.xy), 0).r;
    vec2 sourceUV = outputUV - pc.a.zw * invSize;
    vec4 previous = pc.clipToPrevious * vec4(sourceUV * 2 - 1, depth, 1);
    vec3 previousNdc = previous.xyz / max(previous.w, 1e-8);
    vec2 previousUV = previousNdc.xy * .5 + .5;
    vec2 motion = previousUV - sourceUV;
    vec2 historyUV = outputUV + motion;
    vec2 depthUV = previousUV + pc.previousJitter.xy * invSize;
    bool inside = previous.w > 0 && all(greaterThanEqual(min(historyUV, depthUV), vec2(0))) &&
                  all(lessThanEqual(max(historyUV, depthUV), vec2(1)));
    ivec2 base = ivec2(floor(clamp(depthUV, 0, 1) * vec2(size) - .5));
    float tolerance = .0002 + min(.005, 2 * (abs(dFdx(previousNdc.z)) + abs(dFdy(previousNdc.z))));
    bool sameSurface = false;
    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 2; ++x) {
            float d = texelFetch(previousDepth, clamp(base + ivec2(x, y), ivec2(0), size - 1), 0).r;
            sameSurface = sameSurface || (depth >= .99999 ? d >= .99999 :
                                          d < .99999 && abs(d - previousNdc.z) <= tolerance);
        }
    }

    vec3 raw = historyAt(clamp(historyUV, 0, 1), vec2(size));
    vec3 delta = raw - current.rgb;
    vec3 room = mix(current.rgb - lo, hi - current.rgb, greaterThan(delta, vec3(0)));
    vec3 limits = clamp((room + 1e-5) / max(abs(delta), vec3(1e-5)), 0, 1);
    vec3 clipped = current.rgb + delta * min(limits.x, min(limits.y, limits.z));
    float confidence = 1 - smoothstep(.02, .12, maxChannel(abs(raw - clipped)));
    float edge = smoothstep(.04, .20, maxChannel(hi - lo));
    float still = 1 - smoothstep(.05, 1., length(motion * vec2(size)));
    float edgeWeight = mix(pc.a.x, pc.b.y, edge);
    float accumulation = inside && sameSurface ? mix(edgeWeight, .94, still) * confidence : 0.;
    vec3 filtered = mix(current.rgb, (current.rgb * 4 + axisSum) / 8, pc.b.x);
    float currentWeight = (1 - accumulation) / (1 + maxChannel(filtered));
    float historyWeight = accumulation / (1 + maxChannel(clipped));
    outColor = vec4((filtered * currentWeight + clipped * historyWeight) / (currentWeight + historyWeight), current.a);
}
