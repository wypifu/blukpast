#version 450

/* Procedural full-screen quad: no vertex buffer needed.
 * Indices 0-5 form two CCW triangles that cover the rect. */

layout(push_constant) uniform PC {
    vec4 rect;    /* x, y, w, h in pixels         */
    vec4 color;   /* rgba                          */
    vec2 screen;  /* viewport width and height     */
} pc;

layout(location = 0) out vec4 vColor;

void main()
{
    vec2 corners[4] = vec2[4](
        vec2(pc.rect.x,              pc.rect.y),
        vec2(pc.rect.x + pc.rect.z,  pc.rect.y),
        vec2(pc.rect.x + pc.rect.z,  pc.rect.y + pc.rect.w),
        vec2(pc.rect.x,              pc.rect.y + pc.rect.w)
    );
    int tri[6] = int[6](0, 2, 1, 0, 3, 2);
    vec2 p = corners[tri[gl_VertexIndex]];
    gl_Position = vec4(p.x / pc.screen.x * 2.0 - 1.0,
                       p.y / pc.screen.y * 2.0 - 1.0,
                       0.0, 1.0);
    vColor = pc.color;
}
