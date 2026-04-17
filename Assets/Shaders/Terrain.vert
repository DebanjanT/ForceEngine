#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 View;
    mat4 Proj;
    vec4 CamPos;
} camera;

layout(push_constant) uniform TerrainPC {
    vec4 ChunkOriginAndSize; // xyz=origin, w=chunkSize
} pc;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;
layout(location = 3) out vec3 outViewPos;

void main()
{
    vec4 worldPos = vec4(inPosition, 1.0);
    outWorldPos   = worldPos.xyz;
    outNormal     = inNormal;
    outUV         = inUV;
    outViewPos    = vec3(camera.View * worldPos);
    gl_Position   = camera.Proj * camera.View * worldPos;
}
