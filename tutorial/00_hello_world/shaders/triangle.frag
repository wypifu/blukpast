#version 450
layout(push_constant) uniform PC { float time; } pc;
layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 color;

void main() {
    float p = fract(pc.time * 0.2) * 3.0;
    vec3 c = fragColor.rgb;
    vec3 col;
    if (p < 1.0)
        col = mix(c, c.gbr, smoothstep(0.0, 1.0, p));
    else if (p < 2.0)
        col = mix(c.gbr, c.brg, smoothstep(0.0, 1.0, p - 1.0));
    else
        col = mix(c.brg, c, smoothstep(0.0, 1.0, p - 2.0));
    color = vec4(col, 1.0);
}
