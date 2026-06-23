// Vertex Shader - Shader Model 3.0
float4x4 worldviewproj_matrix;

// These params are in clipspace; not pixels
float mag_scale;
float mag0_size;
float min_size;
float max_size;
float render_target_flipping;

// width/height
float aspect_ratio;

struct VS_INPUT
{
    float3 position : POSITION;
    float3 texcoord : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    
    float4 in_color = float4(1.0, 1.0, 1.0, 1.0);
    output.position = mul(float4(input.position, 1.0), worldviewproj_matrix);
    output.texcoord = input.texcoord.xy;

    float magnitude = input.texcoord.z;
    float size = exp(mag_scale * magnitude) * mag0_size;

    // Fade below minSize.
    float fade = saturate(size / min_size);
    output.color = float4(in_color.rgb, fade * fade);

    // Clamp size to range.
    size = clamp(size, min_size, max_size);

    // Splat the billboard on the screen.
    output.position.xy +=
            output.position.w *
            input.texcoord.xy *
            float2(size, size * aspect_ratio * render_target_flipping);
    
    return output;
}