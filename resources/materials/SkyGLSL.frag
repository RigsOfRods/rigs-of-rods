// Shader ported from https://www.shadertoy.com/view/4sKGWt
// License: BSD
// by Morgan McGuire, @CasualEffects

uniform float iTime;
uniform float iResolution_x;
uniform float iResolution_y;

uniform float sky_yaw;
uniform float sky_pitch;
uniform float sky_roll;
uniform float sky_fov;
uniform float sun_size;
uniform float cloud_density;
uniform float sky_light;

varying vec4 projectionCoord;

float hash(vec2 p) { return fract(1e4 * sin(17.0 * p.x + p.y * 0.1) * (0.1 + abs(sin(p.y * 13.0 + p.x)))); }

float noise(vec2 x) {
    float speed = iTime * 0.2;

    vec2 i = floor(x+speed), f = fract(x+speed);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

float fbm(vec2 p) {
    const mat2 m2 = mat2(0.8, -0.6, 0.6, 0.8);
    
    float f = 0.5000 * noise(p); p = m2 * p * 2.02;
    f += 0.2500 * noise(p); p = m2 * p * 2.03;
    f += 0.1250 * noise(p); p = m2 * p * 2.01;
    f += 0.0625 * noise(p);

    return f / 0.9375;
}

vec3 render(in vec3 light, in vec3 ro, in vec3 rd) {
    // Sky with haze
    vec3 col = vec3(0.3, 0.55, 0.8) * (1.0 - 0.8 * rd.y) * sky_light;

    if (rd.y >= 0.0)
    {
        // Sun
        float sundot = clamp(dot(rd, light), 0.0, 1.0);
        col += sun_size * 0.25 * vec3(1.0, 0.7, 0.4) * pow(sundot, 8.0);
        col += sun_size * 0.75 * vec3(1.0, 0.8, 0.5) * pow(sundot, 64.0);

        // Clouds
        col = mix(col, vec3(1.0, 0.95, 1.0), 0.5 * smoothstep(0.5-cloud_density, 0.8-cloud_density, fbm((ro.xz + rd.xz * (250000.0 - ro.y) / rd.y) * 0.000008)));
    }

    // Horizon/atmospheric perspective
    col = mix(col, vec3(0.7, 0.75, 0.8), pow(1.0 - max(abs(rd.y), 0.0), 8.0));
    
    return col;
}

void main(void)
{
    float verticalFieldOfView = sky_fov * 3.1415927 / 180.0;

    vec3 cameraOrigin = vec3(-iTime, sin(iTime) + 2.1, 0.0);

    vec3 ro = cameraOrigin;
    vec3 rd = normalize(vec3(projectionCoord.xy * vec2(iResolution_x, iResolution_y) / 2.0, iResolution_y * 0.5 / -tan(verticalFieldOfView * 0.5)));
    
    mat3 yawMatrix = mat3(cos(sky_yaw), 0.0, -sin(sky_yaw),
                  0.0, 1.0, 0.0,
                  sin(sky_yaw), 0.0, cos(sky_yaw));
    
    mat3 pitchMatrix = mat3(1.0, 0.0, 0.0,
                  0.0, cos(sky_pitch), sin(sky_pitch),
                  0.0, -sin(sky_pitch), cos(sky_pitch));

    mat3 rollMatrix = mat3(cos(sky_roll), sin(sky_roll), 0.0,
                  -sin(sky_roll), cos(sky_roll), 0.0,
                  0.0, 0.0, 1.0);

    rd = yawMatrix * pitchMatrix * rollMatrix * rd;

    vec3 light = normalize(vec3(-0.5, 0.3 - sin(0.15), -0.3));

    vec3 col = render(light, ro, rd);
 
    // Gamma encode
    col = pow(col, vec3(0.4545));

    gl_FragColor = vec4( col, 1.0 );
}