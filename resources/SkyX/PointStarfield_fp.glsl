#version 330 core

in vec4 out_color;
in vec2 out_texcoord;

out vec4 FragColor;

void main()
{
    FragColor = out_color;
    float sqlen = dot(out_texcoord, out_texcoord);

    // A gaussian bell of sorts.
    FragColor.a *= 1.5 * exp(-(sqlen * 8.0));
}
