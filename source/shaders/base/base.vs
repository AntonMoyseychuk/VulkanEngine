#version 450

layout(location = 0) out vec4 vs_out_color;


vec2 g_positions[3] = vec2[](
    vec2(0.0f, -0.5f),
    vec2(-0.0f, 0.5f),
    vec2(0.5f, 0.5f)
);


vec3 g_colors[3] = vec3[](
    vec3(1.0f, 0.0f, 0.0f),
    vec3(0.0f, 1.0f, 0.0f),
    vec3(0.0f, 0.0f, 1.0f)
);


void main()
{
    vs_out_color = vec4(g_colors[gl_VertexIndex], 1.0f);
    gl_Position = vec4(g_positions[gl_VertexIndex], 0.0f, 1.0f);
}