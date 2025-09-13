#version 450
#extension GL_EXT_nonuniform_qualifier : enable
layout(local_size_x = 8, local_size_y = 8) in;

layout(set = 3, binding = 0, rgba16) uniform image2D inputImageRGBA16[];

layout(push_constant) uniform PushConsts
{
    uint uniformBufferIndex;
} pushConsts;

// std140-friendly packing: keep 16-byte alignment
layout(set = 0, binding = 0) uniform UniformBufferObject
{
    // Use a full 16B row to match HLSL packing easily: x = imageID, yzw = reserved
    uvec4 imageInfo;

    // sunParams0: x=sunNDC.x, y=sunNDC.y, z=sunRadiusNDC, w=time(seconds)
    vec4 sunParams0;
    // sunColor: HDR radiance (pre-tonemap)
    vec4 sunColor;
} uniformBufferObjects[];

vec3 acesFilmicToneMapCurve(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;

    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// ---- Helper noise functions (global scope) ----
float hash(vec2 q){ return fract(sin(dot(q, vec2(127.1, 311.7))) * 43758.5453); }
float noise(vec2 q){
    vec2 i = floor(q);
    vec2 f = fract(q);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d2 = hash(i + vec2(1.0, 1.0));
    vec2 u2 = f * f * (3.0 - 2.0 * f);
    return mix(mix(a, b, u2.x), mix(c, d2, u2.x), u2.y);
}
float fbm(vec2 q){
    float v = 0.0;
    float a2 = 0.5;
    mat2 m = mat2(1.6, 1.2, -1.2, 1.6);
    for(int i=0;i<4;i++){
        v += a2 * noise(q);
        q = m * q + 3.7;
        a2 *= 0.5;
    }
    return v;
}

void main()
{
    uint imageID = uniformBufferObjects[pushConsts.uniformBufferIndex].imageInfo.x;
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    // Start from black to avoid relying on rasterized content
    vec4 color = vec4(0.0);

    // --- Sun + Sky (before tone mapping) ---
    ivec2 size = imageSize(inputImageRGBA16[imageID]);
    vec2 uv = (vec2(pixel) + vec2(0.5)) / vec2(size);           // [0,1]
    // Flip Y so uv.y=1 is top; keep top-oriented logic
    vec2 uvFlip = vec2(uv.x, 1.0 - uv.y);
    vec2 ndc = uvFlip * 2.0 - 1.0;                                   // [-1,1]

    vec2 sunNDC = uniformBufferObjects[pushConsts.uniformBufferIndex].sunParams0.xy;
    float sunRadius = uniformBufferObjects[pushConsts.uniformBufferIndex].sunParams0.z; // in NDC units

    // Keep sun circular under non-square aspect
    float aspect = float(size.x) / float(size.y);
    vec2 ndcA = vec2(ndc.x * aspect, ndc.y);
    vec2 sunA = vec2(sunNDC.x * aspect, sunNDC.y);
    float d = length(ndcA - sunA);

    vec3 sunRadiance = uniformBufferObjects[pushConsts.uniformBufferIndex].sunColor.xyz;

    // Procedural sunset sky with horizon and clouds
    // Horizon blend
    float horizonY = 0.18;
    float horizonBlend = smoothstep(horizonY - 0.01, horizonY + 0.01, uvFlip.y);

    // Ground (below horizon) and warm sky (above)
    vec3 ground = vec3(0.03, 0.025, 0.02);
    vec3 skyLow = vec3(0.02, 0.02, 0.05);
    vec3 skyWarm = vec3(0.9, 0.45, 0.18);
    float warmTop = smoothstep(0.55, 1.0, uvFlip.y);
    vec3 skyGrad = mix(skyLow, skyWarm, warmTop);

    // Simple 2D noise and clouds (animated to the right)
    float tSec = uniformBufferObjects[pushConsts.uniformBufferIndex].sunParams0.w;
    vec2 p = uvFlip * 1.8 + vec2(tSec * 0.02, 0.0);

    float clouds = smoothstep(0.55, 0.75, fbm(p)); // thresholded soft clouds

    // Base sky with horizon
    vec3 sky = mix(ground, skyGrad, horizonBlend);

    // Add global warm glow near the sun
    float wideWidth = max(sunRadius * 3.0, 1e-5);
    float tWide = clamp(d / wideWidth, 0.0, 1.0);
    float wideHalo = exp(-tWide * tWide * 2.0);
    sky += sunRadiance * 0.15 * wideHalo;

    // Lighten clouds (only above horizon)
    vec3 cloudLight = vec3(0.7) + normalize(sunRadiance) * 0.3;
    sky = mix(sky, sky + cloudLight * 0.25, clouds * horizonBlend);

    // (removed duplicate wide halo block)

    // Start from sky background
    color.rgb = sky;

    // Sun disk + rim feather
    float feather = sunRadius * 0.2;
    float edge = 1.0 - smoothstep(sunRadius - feather, sunRadius, d);

    // Near-rim halo
    float haloWidth = max(sunRadius * 0.6, 1e-5);
    float t = max(d - sunRadius, 0.0) / haloWidth; // 0 at rim, 1 at rim + haloWidth
    float halo = exp(-t * t * 3.0);               // gaussian-like falloff
    color.rgb += sunRadiance * edge;          // sun disk
    color.rgb += sunRadiance * 0.5 * halo;    // halo glow

    // Tone map to display-referred
    imageStore(inputImageRGBA16[imageID], pixel, vec4(acesFilmicToneMapCurve(color.xyz), 1.0));
}