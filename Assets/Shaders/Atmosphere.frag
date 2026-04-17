#version 450

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inViewRay;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform AtmosphereUBO {
    vec4  SunDirection;    // xyz=dir (toward sun), w=intensity
    vec4  RayleighScatter; // xyz=coeffs (per km), w=scale height km
    vec4  MieParams;       // x=scatter, y=scale height, z=anisotropy g, w=unused
    vec4  PlanetRadii;     // x=planet km, y=atmosphere km
    mat4  InvView;
    mat4  InvProj;
} atmo;

const float PI = 3.14159265358979;
const int   NUM_STEPS       = 16;
const int   NUM_LIGHT_STEPS = 8;

// Ray-sphere intersection: returns t0,t1 (t < 0 = no hit)
vec2 raySphere(vec3 ro, vec3 rd, float radius)
{
    float b = dot(ro, rd);
    float c = dot(ro, ro) - radius * radius;
    float d = b * b - c;
    if (d < 0.0) return vec2(1e5, -1e5);
    d = sqrt(d);
    return vec2(-b - d, -b + d);
}

float rayleighPhase(float cosTheta)
{
    return (3.0 / (16.0 * PI)) * (1.0 + cosTheta * cosTheta);
}

float miePhase(float cosTheta, float g)
{
    float g2 = g * g;
    return (3.0 / (8.0 * PI)) * ((1.0 - g2) * (1.0 + cosTheta * cosTheta))
           / ((2.0 + g2) * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5));
}

vec3 transmittance(vec3 pos, vec3 dir, float pR, float pH_r, float pH_m, vec3 betaR, float betaM)
{
    vec2 t = raySphere(pos, dir, pR + atmo.PlanetRadii.y - atmo.PlanetRadii.x);
    float tMax = max(t.y, 0.0);
    float ds   = tMax / float(NUM_LIGHT_STEPS);
    vec3 optDepth = vec3(0.0);
    for (int i = 0; i < NUM_LIGHT_STEPS; i++)
    {
        vec3 s  = pos + dir * (i + 0.5) * ds;
        float h = max(length(s) - atmo.PlanetRadii.x, 0.0);
        optDepth.xy += vec2(exp(-h / pH_r), exp(-h / pH_m)) * ds;
    }
    return exp(-(betaR * optDepth.x + betaM * optDepth.y));
}

void main()
{
    vec3 rd      = normalize(inViewRay);
    vec3 sunDir  = normalize(atmo.SunDirection.xyz);
    float sunInt = atmo.SunDirection.w;

    float pR  = atmo.PlanetRadii.x;      // planet radius km
    float pA  = atmo.PlanetRadii.y;      // atmosphere top km
    float pH_r = atmo.RayleighScatter.w; // Rayleigh scale height
    float pH_m = atmo.MieParams.y;       // Mie scale height
    float g    = atmo.MieParams.z;
    vec3  betaR = atmo.RayleighScatter.xyz;
    float betaM = atmo.MieParams.x;

    // Camera at surface level
    vec3 ro = vec3(0.0, pR + 0.001, 0.0);

    vec2 tAtm = raySphere(ro, rd, pA);
    if (tAtm.y < 0.0) { outColor = vec4(0, 0, 0, 1); return; }

    float tMin = max(tAtm.x, 0.0);
    float tMax = tAtm.y;

    // Hit planet?
    vec2 tPlanet = raySphere(ro, rd, pR);
    if (tPlanet.x > 0.0) tMax = min(tMax, tPlanet.x);

    float ds = (tMax - tMin) / float(NUM_STEPS);
    float cosTheta = dot(rd, sunDir);

    vec3 scatterR = vec3(0.0), scatterM = vec3(0.0);
    vec3 optDepthR = vec3(0.0), optDepthM = vec3(0.0);

    for (int i = 0; i < NUM_STEPS; i++)
    {
        vec3  s = ro + rd * (tMin + (i + 0.5) * ds);
        float h = max(length(s) - pR, 0.0);
        float hr = exp(-h / pH_r) * ds;
        float hm = exp(-h / pH_m) * ds;
        optDepthR += hr;
        optDepthM += hm;

        vec3 trans = transmittance(s, sunDir, pR, pH_r, pH_m, betaR, betaM);
        vec3 attenuation = exp(-(betaR * optDepthR.x + betaM * optDepthM.x));
        scatterR += trans * attenuation * hr;
        scatterM += trans * attenuation * hm;
    }

    vec3 sky = sunInt * (scatterR * betaR * rayleighPhase(cosTheta)
                       + scatterM * betaM * miePhase(cosTheta, g));

    // Tone-map (simple Reinhard)
    sky = sky / (sky + vec3(1.0));
    sky = pow(sky, vec3(1.0 / 2.2));

    outColor = vec4(sky, 1.0);
}
