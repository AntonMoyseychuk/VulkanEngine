#version 450

layout(location = 0) out vec4 vs_out_color;


struct Vertex
{
    vec4 position;
    vec4 color;
};


Vertex vertices[3] = Vertex[3](
    Vertex(vec4( 0.0f, -0.5f, 0.0f, 1.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)),
    Vertex(vec4( 0.5f,  0.5f, 0.0f, 1.0f), vec4(0.0f, 1.0f, 0.0f, 1.0f)),
    Vertex(vec4(-0.5f,  0.5f, 0.0f, 1.0f), vec4(0.0f, 0.0f, 1.0f, 1.0f))
);


void main()
{
#if defined(_DEBUG)
    vs_out_color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
#else
    vs_out_color = vertices[gl_VertexIndex].color;
#endif
    gl_Position =  vertices[gl_VertexIndex].position;
}