#version 120

uniform vec4 lightDiffuse;
uniform vec4 lightSpecular;
uniform float exponent;
uniform vec4 ambient;

uniform sampler2D Diffuse_Map;
uniform sampler2D Specular_Map;
uniform sampler2D Dmg_Diffuse_Map;

varying vec4 oPos;
varying vec4 oCols;
varying vec3 oNorm;
varying vec4 oLightPos;
varying vec3 oEyePos;
varying vec2 oUv;

vec4 ror_lit(float ndotl, float ndoth, float m)
{
    float ambientTerm  = 1.0;
    float diffuseTerm  = max(ndotl, 0.0);
    float specularTerm = (ndotl > 0.0) ? pow(max(ndoth, 0.0), m) : 0.0;
    return vec4(ambientTerm, diffuseTerm, specularTerm, 1.0);
}

void main()
{
    vec3 n = normalize(oNorm);
    vec3 eyeDir = normalize(oEyePos - oPos.xyz);
    vec3 lightDir = normalize(oLightPos.xyz - (oPos * oLightPos.w).xyz);
    vec3 halfAngle = normalize(lightDir + eyeDir);

    float ndotl = dot(lightDir, n);
    float ndoth = dot(halfAngle, n);
    vec4 litTerm = ror_lit(ndotl, ndoth, exponent);

    vec4 textColour = texture2D(Diffuse_Map, oUv) * (1.0 - oCols.a);
    textColour = textColour + texture2D(Dmg_Diffuse_Map, oUv) * oCols.a;
    textColour = textColour * (1.0 - oCols.b / 3.0);

    vec4 specColour = texture2D(Specular_Map, oUv) + oCols.b / 3.0 - oCols.a / 2.0;

    vec4 outColour = mix(lightDiffuse * textColour * litTerm.y + textColour * ambient,
                         lightSpecular * litTerm.z,
                         specColour);
    outColour.a = texture2D(Diffuse_Map, oUv).a;

    gl_FragColor = outColour;
}
