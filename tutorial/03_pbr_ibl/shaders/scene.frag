#version 450

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec4 vColor;
layout(location = 3) in vec2 vUV;

layout(set = 0, binding = 0) uniform Scene {
    mat4 view;
    mat4 proj;
    vec4 lightDir;
    vec4 camPos;
} scene;

/* IBL resources — precomputed on the CPU at startup.
 *
 * irradianceMap : 32×32 cubemap, cosine-weighted average sky radiance per
 *                 direction.  Sampled with the surface normal N for diffuse.
 *
 * skyCube       : 256×256 sky cubemap used as a simple reflection probe for
 *                 specular.  A production pipeline would use a multi-mip
 *                 prefiltered environment map here.
 *
 * brdfLut       : 128×128 split-sum BRDF LUT (R=scale, G=bias).
 *
 * albedoMap     : optional 2-D albedo texture (e.g. checkerboard for the
 *                 ground).  Activated via pc.useAlbedoMap > 0.5.
 */
layout(set = 0, binding = 1) uniform samplerCube irradianceMap;
layout(set = 0, binding = 2) uniform samplerCube skyCube;
layout(set = 0, binding = 3) uniform sampler2D   brdfLut;
layout(set = 0, binding = 4) uniform sampler2D   albedoMap;

layout(push_constant) uniform PC {
    mat4  model;
    vec4  color;
    float roughness;
    float metallic;
    float useAlbedoMap;  /* > 0.5 → sample albedoMap instead of color */
    float pad1;
} pc;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

/* IBL ambient strength relative to direct lighting.
 * The procedural sky is quite bright (gradient to white); this constant
 * keeps the indirect contribution physically plausible alongside the sun. */
const float IBL_SCALE = 0.4;

/* UV tiling for the albedo map on the ground plane (2 checker squares per
 * texture repeat × 10 repeats across 20 world units = 1 unit per square). */
const float UV_TILE = 5.0;   /* 4 squares/tex × 5 repeats = 20 squares across 20 world units */

/* ---- GGX direct-lighting helpers ---- */

float DistributionGGX(vec3 N, vec3 H, float rough)
{
    float a   = rough * rough;
    float a2  = a * a;
    float NdH = max(dot(N, H), 0.0);
    float d   = NdH * NdH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

float GeometrySchlick(float NdV, float rough)
{
    float k = (rough + 1.0) * (rough + 1.0) / 8.0;
    return NdV / (NdV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float rough)
{
    return GeometrySchlick(max(dot(N, V), 0.0), rough)
         * GeometrySchlick(max(dot(N, L), 0.0), rough);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

/* Roughness-attenuated Fresnel for IBL — prevents unrealistic grazing
 * highlights on rough surfaces. */
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0)
              * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    /* Albedo: flat color or tiled texture (e.g. checkerboard ground). */
    vec3 albedo = (pc.useAlbedoMap > 0.5)
                ? texture(albedoMap, vUV * UV_TILE).rgb
                : vColor.rgb;

    float roughness = max(pc.roughness, 0.04);
    float metallic  = pc.metallic;

    vec3 N  = normalize(vNormal);
    vec3 V  = normalize(scene.camPos.xyz - vWorldPos);
    vec3 R  = reflect(-V, N);
    vec3 L  = normalize(-scene.lightDir.xyz);
    vec3 H  = normalize(V + L);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    /* ------------------------------------------------------------------ */
    /* Direct lighting (PBR Cook-Torrance)                                 */
    /* ------------------------------------------------------------------ */
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3  F   = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3  specDirect = (NDF * G * F)
                     / (4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001);
    vec3  kD_direct  = (vec3(1.0) - F) * (1.0 - metallic);
    float NdL        = max(dot(N, L), 0.0);
    vec3  Lo         = (kD_direct * albedo / PI + specDirect) * vec3(3.5) * NdL;

    /* ------------------------------------------------------------------ */
    /* IBL ambient                                                         */
    /* ------------------------------------------------------------------ */
    vec3 kS = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    /* Diffuse: irradiance map stores E(N)/π so diffuse = kD·albedo·irradiance. */
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse    = kD * albedo * irradiance;

    /* Specular: sky cubemap as reflection probe; blend toward irradiance for
     * rough surfaces (approximates a prefiltered environment map). */
    vec3 prefilteredColor = mix(texture(skyCube, R).rgb, irradiance, roughness * roughness);
    vec2 brdf    = texture(brdfLut, vec2(clamp(dot(N, V), 0.0, 1.0), roughness)).rg;
    vec3 specIbl = prefilteredColor * (kS * brdf.x + brdf.y);

    vec3 ambient = (diffuse + specIbl) * IBL_SCALE;

    /* ------------------------------------------------------------------ */
    /* Combine + Reinhard tone-map + gamma                                 */
    /* ------------------------------------------------------------------ */
    vec3 color = ambient + Lo;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(color, 1.0);
}
