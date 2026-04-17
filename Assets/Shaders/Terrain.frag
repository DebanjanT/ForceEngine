#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inViewPos;

layout(location = 0) out vec4 outColor;

// Camera
layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 View; mat4 Proj; vec4 CamPos;
} camera;

// Shadow cascades
layout(set = 1, binding = 0) uniform CascadeUBO {
    mat4 ViewProj[4];
    vec4 SplitDepths;
} cascades;
layout(set = 1, binding = 1) uniform sampler2DShadow shadowMap0;
layout(set = 1, binding = 2) uniform sampler2DShadow shadowMap1;
layout(set = 1, binding = 3) uniform sampler2DShadow shadowMap2;
layout(set = 1, binding = 4) uniform sampler2DShadow shadowMap3;

// Blend maps (layers 0-3 in map0, 4-7 in map1)
layout(set = 2, binding = 0) uniform sampler2D blendMap0;
layout(set = 2, binding = 1) uniform sampler2D blendMap1;

// Material layer textures (up to 8 layers, each 2 slots: albedo + normal)
layout(set = 3, binding = 0) uniform sampler2D layer0Albedo;
layout(set = 3, binding = 1) uniform sampler2D layer1Albedo;
layout(set = 3, binding = 2) uniform sampler2D layer2Albedo;
layout(set = 3, binding = 3) uniform sampler2D layer3Albedo;
layout(set = 3, binding = 4) uniform sampler2D layer4Albedo;
layout(set = 3, binding = 5) uniform sampler2D layer5Albedo;
layout(set = 3, binding = 6) uniform sampler2D layer6Albedo;
layout(set = 3, binding = 7) uniform sampler2D layer7Albedo;

layout(set = 4, binding = 0) uniform LayerParams {
    vec4 Tiling[8]; // x=tiling scale per layer
} layerParams;

// Directional light
layout(set = 5, binding = 0) uniform DirectionalLight {
    vec4 Direction;
    vec4 Color;      // w = intensity
    uint CascadeCount;
    float ShadowBias;
} dirLight;

// DDGI probes
layout(set = 6, binding = 0) uniform sampler2D ddgiIrradiance;
layout(set = 6, binding = 1) uniform sampler2D ddgiDepth;
layout(set = 6, binding = 2) uniform DDGIProbeUBO {
    vec4  GridOriginSpacing;
    ivec4 GridDims;
    vec4  Params;
} ddgi;

float SampleShadow(int cascade, vec3 worldPos)
{
    vec4 sc = cascades.ViewProj[cascade] * vec4(worldPos, 1.0);
    sc.xyz /= sc.w;
    sc.xy = sc.xy * 0.5 + 0.5;
    if (sc.x < 0 || sc.x > 1 || sc.y < 0 || sc.y > 1) return 1.0;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap0, 0);
    // 3x3 PCF
    for (int dy = -1; dy <= 1; dy++)
    for (int dx = -1; dx <= 1; dx++)
    {
        vec2 offset = vec2(dx, dy) * texelSize;
        float s = 0.0;
        if      (cascade == 0) s = texture(shadowMap0, vec3(sc.xy + offset, sc.z - dirLight.ShadowBias));
        else if (cascade == 1) s = texture(shadowMap1, vec3(sc.xy + offset, sc.z - dirLight.ShadowBias));
        else if (cascade == 2) s = texture(shadowMap2, vec3(sc.xy + offset, sc.z - dirLight.ShadowBias));
        else                   s = texture(shadowMap3, vec3(sc.xy + offset, sc.z - dirLight.ShadowBias));
        shadow += s;
    }
    return shadow / 9.0;
}

vec3 SampleTerrainAlbedo(vec2 uv)
{
    vec4 blend0 = texture(blendMap0, inUV);
    vec4 blend1 = texture(blendMap1, inUV);

    vec3 col = vec3(0.0);
    col += blend0.r * texture(layer0Albedo, uv * layerParams.Tiling[0].x).rgb;
    col += blend0.g * texture(layer1Albedo, uv * layerParams.Tiling[1].x).rgb;
    col += blend0.b * texture(layer2Albedo, uv * layerParams.Tiling[2].x).rgb;
    col += blend0.a * texture(layer3Albedo, uv * layerParams.Tiling[3].x).rgb;
    col += blend1.r * texture(layer4Albedo, uv * layerParams.Tiling[4].x).rgb;
    col += blend1.g * texture(layer5Albedo, uv * layerParams.Tiling[5].x).rgb;
    col += blend1.b * texture(layer6Albedo, uv * layerParams.Tiling[6].x).rgb;
    col += blend1.a * texture(layer7Albedo, uv * layerParams.Tiling[7].x).rgb;
    return col;
}

void main()
{
    vec3 N = normalize(inNormal);
    vec3 L = normalize(-dirLight.Direction.xyz);

    // Cascade selection
    float depth = abs(inViewPos.z);
    int cascade = int(dirLight.CascadeCount) - 1;
    for (int i = 0; i < int(dirLight.CascadeCount) - 1; i++)
    {
        if (depth < cascades.SplitDepths[i]) { cascade = i; break; }
    }
    float shadow = SampleShadow(cascade, inWorldPos);

    // Albedo from layer blend
    vec3 albedo = SampleTerrainAlbedo(inUV);

    // Simple diffuse + ambient
    float NdotL   = max(dot(N, L), 0.0);
    vec3  sunColor = dirLight.Color.rgb * dirLight.Color.w;
    vec3  diffuse  = albedo * sunColor * NdotL * shadow;
    vec3  ambient  = albedo * 0.08; // DDGI would replace this

    outColor = vec4(diffuse + ambient, 1.0);
}
