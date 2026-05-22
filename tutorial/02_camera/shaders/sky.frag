#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D skyTex;

void main()
{
    outColor = texture(skyTex, vUV);
}
