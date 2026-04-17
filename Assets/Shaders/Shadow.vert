#version 450

layout(location = 0) in vec3 inPosition;

layout(push_constant) uniform PushConstants {
    mat4 LightViewProj;
} pc;

void main()
{
    gl_Position = pc.LightViewProj * vec4(inPosition, 1.0);
}
