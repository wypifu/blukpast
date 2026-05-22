#version 450

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec3 inUV;

layout(set = 0, binding = 0) uniform UBO {
    mat4 transform;
    vec4 color;
} ubo;

layout(location = 0) out vec3 vUV;
layout(location = 1) out vec4 vColor;

void main()
{
    gl_Position = ubo.transform * vec4(inPos, 0.0, 1.0);
    vUV    = inUV;
    vColor = ubo.color;
}
