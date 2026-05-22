#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inUV;
layout(location = 3) in vec3 inTangent;

layout(set = 0, binding = 0) uniform Scene {
    mat4 view;
    mat4 proj;
    vec4 lightDir;
    vec4 camPos;
    mat4 lightViewProj;
    mat4 reflectViewProj;   /* reflected camera VP — for anchored reflection UV */
} scene;

layout(push_constant) uniform PC {
    mat4  model;
    vec4  color;
    float roughness;
    float metallic;
    float useAlbedoMap;
    float pad1;
} pc;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec4 vColor;
layout(location = 3) out vec2 vUV;
layout(location = 4) out vec4 vLightPos;
layout(location = 5) out vec4 vClipPos;      /* main camera clip pos (screen-space UV) */
layout(location = 6) out vec4 vReflectPos;   /* reflected camera clip pos (world-anchored UV) */

void main()
{
    vec4 worldPos = pc.model * vec4(inPos, 1.0);
    gl_Position   = scene.proj * scene.view * worldPos;
    vWorldPos     = worldPos.xyz;
    vNormal       = normalize(mat3(transpose(inverse(pc.model))) * inNormal);
    vColor        = pc.color;
    vUV           = inUV.xy;
    vLightPos     = scene.lightViewProj   * worldPos;
    vClipPos      = gl_Position;
    vReflectPos   = scene.reflectViewProj * worldPos;
}
