#version 450

layout(location = 0) in float vLifetime;
layout(location = 1) in vec3  vColor;

layout(location = 0) out vec4 outColor;

void main()
{
    if(vLifetime <= 0.0) discard;

    vec2  coord = gl_PointCoord * 2.0 - 1.0;
    float r2    = dot(coord, coord);
    if(r2 > 1.0) discard;

    float r     = sqrt(r2);
    float alpha = vLifetime * (1.0 - r * 0.8);

    /* hot core: brighten toward center */
    vec3 col = mix(min(vColor * 2.0, vec3(1.0)), vColor, r);

    outColor = vec4(col, alpha);
}
