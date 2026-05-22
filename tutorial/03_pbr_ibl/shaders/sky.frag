#version 450

layout(location = 0) in vec3 vDir;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform samplerCube skyCube;

void main()
{
    outColor = texture(skyCube, vDir);
}
