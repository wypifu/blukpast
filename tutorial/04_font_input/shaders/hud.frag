#version 450

layout(location = 0) in vec3 vUV;
layout(location = 1) in vec4 vColor;

layout(set = 0, binding = 1) uniform sampler2DArray fontAtlas;

layout(location = 0) out vec4 outColor;

void main()
{
    float alpha = texture(fontAtlas, vUV).r;
    outColor = vec4(vColor.rgb, vColor.a * alpha);
}
