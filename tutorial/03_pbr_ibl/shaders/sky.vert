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

/* Vertex world-space direction — used directly as the cubemap lookup vector. */
layout(location = 0) out vec3 vDir;

void main()
{
    /* Strip translation from view so the sky box never moves with the camera. */
    mat4 viewNoTrans  = scene.view;
    viewNoTrans[3]    = vec4(0.0, 0.0, 0.0, 1.0);

    /* Scale box to 200 units so it always encloses the scene. */
    vec4 pos          = scene.proj * viewNoTrans * vec4(inPos * 200.0, 1.0);

    /* Force depth = 1.0 (far plane) so the sky box renders behind everything. */
    gl_Position       = pos.xyww;

    /* Model-space position == world-space direction for a centred unit cube. */
    vDir              = inPos;
}
