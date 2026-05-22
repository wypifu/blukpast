#version 450

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec4 vColor;
layout(location = 3) in vec2 vUV;
layout(location = 4) in vec4 vLightPos;

layout(set = 0, binding = 0) uniform Scene {
    mat4 view;
    mat4 proj;
    vec4 lightDir;
    vec4 camPos;
    mat4 lightViewProj;
} scene;

layout(set = 0, binding = 1) uniform sampler2D        albedoTex;
layout(set = 0, binding = 2) uniform sampler2DShadow  shadowMap;

layout(push_constant) uniform PC {
    mat4  model;
    vec4  color;
    float roughness;
    float metallic;
    float useAlbedoMap;
    float pad1;
} pc;

layout(location = 0) out vec4 outColor;

const float PI              = 3.14159265359;
const float UV_TILE         = 2.0;
const float SHADOW_BIAS     = 0.0015;
const float SHADOW_MAP_SIZE = 4096.0;

/* 3×3 PCF using hardware bilinear comparison */
float shadowPCF(vec4 lightPos)
{
    vec3 proj  = lightPos.xyz / lightPos.w;
    proj.xy    = proj.xy * 0.5 + 0.5;

    if(proj.z >= 1.0 || proj.x <= 0.0 || proj.x >= 1.0 ||
                         proj.y <= 0.0 || proj.y >= 1.0)
        return 1.0;

    float shadow = 0.0;
    float texel  = 1.0 / SHADOW_MAP_SIZE;
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            vec2 off = vec2(float(x), float(y)) * texel;
            shadow  += texture(shadowMap, vec3(proj.xy + off, proj.z - SHADOW_BIAS));
        }
    }
    return shadow / 9.0;
}

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

void main()
{
    vec3 albedo = (pc.useAlbedoMap > 0.5)
                ? texture(albedoTex, vUV * UV_TILE).rgb
                : vColor.rgb;

    float roughness = max(pc.roughness, 0.04);
    float metallic  = pc.metallic;

    vec3 N  = normalize(vNormal);
    vec3 V  = normalize(scene.camPos.xyz - vWorldPos);
    vec3 L  = normalize(-scene.lightDir.xyz);
    vec3 H  = normalize(V + L);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3  F   = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3  specular = (NDF * G * F)
                   / (4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001);
    vec3  kD       = (vec3(1.0) - F) * (1.0 - metallic);
    float NdL      = max(dot(N, L), 0.0);

    float shadow = shadowPCF(vLightPos);

    vec3 Lo      = (kD * albedo / PI + specular) * vec3(4.0) * NdL * shadow;
    vec3 ambient = vec3(0.15) * albedo;
    vec3 color   = ambient + Lo;

    color    = color / (color + vec3(1.0));
    color    = pow(color, vec3(1.0 / 2.2));
    outColor = vec4(color, 1.0);
}
