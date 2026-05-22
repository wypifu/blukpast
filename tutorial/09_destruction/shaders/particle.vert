#version 450

layout(location = 0) in vec4 inPos; /* xyz=world pos, w=lifetime */
layout(location = 1) in vec4 inVel; /* xyz=velocity  (unused here) */

/* Only view+proj are read; the buffer may contain more fields (SceneUBORefl). */
layout(set = 0, binding = 0) uniform Scene {
    mat4 view;
    mat4 proj;
} scene;

layout(push_constant) uniform PC {
    vec4 color;
};

layout(location = 0) out float vLifetime;
layout(location = 1) out vec3  vColor;

void main()
{
    vLifetime = inPos.w;
    vColor    = color.rgb;

    if(inPos.w <= 0.0) {
        gl_Position  = vec4(10.0, 10.0, 10.0, 1.0); /* off-screen */
        gl_PointSize = 0.0;
        return;
    }

    gl_Position  = scene.proj * scene.view * vec4(inPos.xyz, 1.0);
    gl_PointSize = max(2.0, 6.0 * inPos.w);
}
