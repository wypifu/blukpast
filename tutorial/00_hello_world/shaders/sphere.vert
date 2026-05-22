#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 outNormal;

void main()
{
    // scale to 0.35, offset to right half of screen
    vec3 p = vec3(inPos.x * 0.35 + 0.55, inPos.y * 0.35, inPos.z);
    gl_Position = vec4(p, 1.0);
    outNormal = inNormal;
}
