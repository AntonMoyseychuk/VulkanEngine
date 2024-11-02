#version 450

layout(location = 0) in vec4 fs_in_color;

layout(location = 0) out vec4 fs_out_color;


void main()
{
#if defined(_DEBUG)
    fs_out_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
#else
    fs_out_color = fs_in_color;
#endif
}