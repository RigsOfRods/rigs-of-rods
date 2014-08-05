//#include <hdrutils.hlsl>

// RGBE mode utilities
// RGB each carry a mantissa, A carries a shared exponent
// The exponent is calculated based on the largest colour channel


float3 decodeRGBE8(in float4 rgbe)
{
    // get exponent (-128 since it can be +ve or -ve)
	float exp = rgbe.a * 255 - 128;
	
	// expand out the rgb value
	return rgbe.rgb * exp2(exp);
}

float4 encodeRGBE8(in float3 rgb)
{
	float4 ret;

    // What is the largest colour channel?
	float highVal = max(rgb.r, max(rgb.g, rgb.b));
	
	// Take the logarithm, clamp it to a whole value
	float exp = ceil(log2(highVal));

    // Divide the components by the shared exponent
	ret.rgb = rgb / exp2(exp);
	
	// Store the shared exponent in the alpha channel
	ret.a = (exp + 128) / 255;

	return ret;
}


static const float4 LUMINENCE_FACTOR  = float4(0.27f, 0.67f, 0.06f, 0.0f);
static const float MIDDLE_GREY = 0.72f;
static const float FUDGE = 0.001f;
static const float L_WHITE = 1.5f;
static const float4 BRIGHT_LIMITER = float4(0.6f, 0.6f, 0.6f, 0.0f);


/** Tone mapping function 
@note Only affects rgb, not a
@param inColour The HDR colour
@param lum The scene lumninence 
@returns Tone mapped colour
*/
float4 toneMap(float4 inColour, float lum)
{
	// From Reinhard et al
	// "Photographic Tone Reproduction for Digital Images"
	
	// Initial luminence scaling (equation 2)
    inColour.rgb *= MIDDLE_GREY / (FUDGE + lum);

	// Control white out (equation 4 nom)
    inColour.rgb *= (1.0f + inColour / L_WHITE);

	// Final mapping (equation 4 denom)
	inColour.rgb /= (1.0f + inColour);
	
	return inColour;

}



/* Downsample a 2x2 area and convert to greyscale
*/
float4 downscale2x2Luminence(
	float2 uv : TEXCOORD0,
	uniform float2 texelSize, // depends on size of source texture
	uniform sampler2D inRTT : register(s0)
    ) : COLOR
{
	
    float4 accum = float4(0.0f, 0.0f, 0.0f, 0.0f);

	float2 texOffset[4] = {
		-0.5, -0.5,
		-0.5,  0.5, 
		 0.5, -0.5,
		 0.5, 0.5 };

	for( int i = 0; i < 4; i++ )
    {
        // Get colour from source
        accum += tex2D(inRTT, uv + texelSize * texOffset[i]);
    }
    
	// Adjust the accumulated amount by lum factor
	// Cannot use float3's here because it generates dependent texture errors because of swizzle
	float lum = dot(accum, LUMINENCE_FACTOR);
	// take average of 4 samples
	lum *= 0.25;
	return lum;

}

/* Downsample a 3x3 area 
 * This shader is used multiple times on different source sizes, so texel size has to be configurable
*/
float4 downscale3x3(
	float2 uv : TEXCOORD0,
	uniform float2 texelSize, // depends on size of source texture
	uniform sampler2D inRTT : register(s0)
    ) : COLOR
{
	
    float4 accum = float4(0.0f, 0.0f, 0.0f, 0.0f);

	float2 texOffset[9] = {
		-1.0, -1.0,
		 0.0, -1.0,
		 1.0, -1.0,
		-1.0,  0.0,
		 0.0,  0.0,
		 1.0,  0.0,
		-1.0,  1.0,
		 0.0,  1.0,
		 1.0,  1.0
	};

	for( int i = 0; i < 9; i++ )
    {
        // Get colour from source
        accum += tex2D(inRTT, uv + texelSize * texOffset[i]);
    }
    
	// take average of 9 samples
	accum *= 0.1111111111111111;
	return accum;

}

/* Downsample a 3x3 area from main RTT and perform a brightness pass
*/
float4 downscale3x3brightpass(
	float2 uv : TEXCOORD0,
	uniform float2 texelSize, // depends on size of source texture
	uniform sampler2D inRTT : register(s0),
	uniform sampler2D inLum : register(s1)
    ) : COLOR
{
	
    float4 accum = float4(0.0f, 0.0f, 0.0f, 0.0f);

	float2 texOffset[9] = {
		-1.0, -1.0,
		 0.0, -1.0,
		 1.0, -1.0,
		-1.0,  0.0,
		 0.0,  0.0,
		 1.0,  0.0,
		-1.0,  1.0,
		 0.0,  1.0,
		 1.0,  1.0
	};

	for( int i = 0; i < 9; i++ )
    {
        // Get colour from source
        accum += tex2D(inRTT, uv + texelSize * texOffset[i]);
    }
    
	// take average of 9 samples
	accum *= 0.1111111111111111;

    // Reduce bright and clamp
    accum = max(float4(0.0f, 0.0f, 0.0f, 1.0f), accum - BRIGHT_LIMITER);

	// Sample the luminence texture
	float4 lum = tex2D(inLum, float2(0.5f, 0.5f));
	
	// Tone map result
	return toneMap(accum, lum.r);

}

/* Gaussian bloom, requires offsets and weights to be provided externally
*/
float4 bloom(
		float2 uv : TEXCOORD0,
		uniform float2 sampleOffsets[15],
		uniform float4 sampleWeights[15],	
		uniform sampler2D inRTT : register(s0)
		) : COLOR
{
    float4 accum = float4(0.0f, 0.0f, 0.0f, 1.0f);
	float2 sampleUV;
    
    for( int i = 0; i < 15; i++ )
    {
        // Sample from adjacent points, 7 each side and central
        sampleUV = uv + sampleOffsets[i];
        accum += sampleWeights[i] * tex2D(inRTT, sampleUV);
    }
    
    return accum;
	
}
		

/* Final scene composition, with tone mapping
*/
float4 finalToneMapping(
	float2 uv : TEXCOORD0,
	uniform sampler2D inRTT : register(s0),
	uniform sampler2D inBloom : register(s1),
	uniform sampler2D inLum : register(s2)
    ) : COLOR
{
	// Get main scene colour
    float4 sceneCol = tex2D(inRTT, uv);

	// Get luminence value
	float4 lum = tex2D(inLum, float2(0.5f, 0.5f));

	// tone map this
	float4 toneMappedSceneCol = toneMap(sceneCol, lum.r);
	
	// Get bloom colour
    float4 bloom = tex2D(inBloom, uv);

	// Add scene & bloom
	return float4(toneMappedSceneCol.rgb + bloom.rgb, 1.0f);
     	
}


