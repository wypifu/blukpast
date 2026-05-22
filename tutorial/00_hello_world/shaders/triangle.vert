#version 450
const vec2 pos[3] = vec2[](vec2(-0.5, 0.5), vec2(0.5, 0.5), vec2(0.0, -0.5));
const vec4 col[3] = vec4[](vec4(1.0, 0.0, 0.0, 1.0), vec4(0.0, 1.0, 0.0, 1.0), vec4(0.0, 0.0, 1.0, 1.0));
layout(location = 0) out vec4 fragColor;
void main() {
    gl_Position = vec4(pos[gl_VertexIndex], 0.0, 1.0);
    fragColor = col[gl_VertexIndex];
}
