#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_texcoord;

uniform mat4 worldviewproj_matrix;

// These params are in clipspace; not pixels
uniform float mag_scale;
uniform float mag0_size;
uniform float min_size;
uniform float max_size;
uniform float render_target_flipping;

// width/height
uniform float aspect_ratio;

out vec2 out_texcoord;
out vec4 out_color;

void main()
{
    vec4 in_color = vec4(1.0, 1.0, 1.0, 1.0);
    gl_Position = worldviewproj_matrix * vec4(in_position, 1.0);
    out_texcoord = in_texcoord.xy;

    float magnitude = in_texcoord.z;
    float size = exp(mag_scale * magnitude) * mag0_size;

    // Fade below minSize.
    float fade = clamp(size / min_size, 0.0, 1.0);
    out_color = vec4(in_color.rgb, fade * fade);

    // Clamp size to range.
    size = clamp(size, min_size, max_size);

    // Splat the billboard on the screen.
    gl_Position.xy +=
            gl_Position.w *
            in_texcoord.xy *
            vec2(size, size * aspect_ratio * render_target_flipping);
}
