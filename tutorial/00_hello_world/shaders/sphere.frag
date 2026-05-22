#version 450

layout(location = 0) in vec3 inNormal;
layout(location = 0) out vec4 outColor;

void main()
{
    vec3 light = normalize(vec3(1.0, 1.0, 1.0));
    float diff = max(dot(normalize(inNormal), light), 0.08);
    outColor = vec4(vec3(0.2, 0.5, 1.0) * diff, 1.0);
}
