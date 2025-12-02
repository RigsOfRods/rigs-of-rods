// This file is part of the Caelum project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution.

OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
#include <OgreUnifiedShader.h>

OGRE_UNIFORMS(
    uniform mat4 worldviewproj_matrix;
)

MAIN_PARAMETERS
    IN(vec4 in_pos, POSITION)
    OUT(vec2 out_uv0, TEXCOORD0)
MAIN_DECLARATION
{
    // Use standard transform.
    gl_Position = mul(worldviewproj_matrix, in_pos);

    // Convert to image-space
    vec4 tmp_pos = in_pos;
    tmp_pos.xy = sign(in_pos.xy);
    out_uv0 = (vec2(tmp_pos.x, -tmp_pos.y) + 1.0f) * 0.5f;    
}
