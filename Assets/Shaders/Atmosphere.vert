#version 450

// Fullscreen triangle — no vertex input
layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outViewRay;

layout(set = 0, binding = 0) uniform AtmosphereUBO {
    vec4  SunDirection;      // xyz=dir, w=intensity
    vec4  RayleighScatter;   // xyz=coeffs, w=height scale
    vec4  MieParams;         // x=scatter, y=height, z=anisotropy, w=unused
    vec4  PlanetRadii;       // x=planet km, y=atmosphere km
    mat4  InvView;
    mat4  InvProj;
} atmo;

void main()
{
    // Generate fullscreen triangle from vertex index
    vec2 uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
    outUV = uv;

    // Reconstruct view ray
    vec4 clipRay    = vec4(uv * 2.0 - 1.0, 1.0, 1.0);
    vec4 viewRay    = atmo.InvProj * clipRay;
    viewRay.xyz    /= viewRay.w;
    vec4 worldRay   = atmo.InvView * vec4(viewRay.xyz, 0.0);
    outViewRay      = normalize(worldRay.xyz);
}
