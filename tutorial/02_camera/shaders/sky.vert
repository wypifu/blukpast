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
} scene;

layout(location = 0) out vec2 vUV;

void main()
{
    /* Sphere poles are on ±Y (world up/down). No rotation needed. */
    mat4 viewNoTrans  = scene.view;
    viewNoTrans[3]    = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 pos          = scene.proj * viewNoTrans * vec4(inPos * 200.0, 1.0);
    gl_Position       = pos.xyww;
    vUV               = inUV.xy;
}
