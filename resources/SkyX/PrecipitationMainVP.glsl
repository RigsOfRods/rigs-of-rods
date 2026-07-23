// This file is part of the Caelum project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution.

//\\Backported from OgreUnifiedShader (OGRE14) to plain GLSL
//\\OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
#version 440
//\\#include <OgreUnifiedShader.h>

//\\OGRE_UNIFORMS(
    uniform mat4 worldviewproj_matrix;
//\\)

//\\MAIN_PARAMETERS
    layout (location = 0) in vec4 in_pos; //\\IN(vec4 in_pos, POSITION)
    out vec2 scr_pos; //\\OUT(vec2 out_uv0, TEXCOORD0)
void main()//\\MAIN_DECLARATION
{
    // Use standard transform.
    gl_Position = worldviewproj_matrix * in_pos;//\\ mul(worldviewproj_matrix, in_pos);

    // Convert to image-space
    vec4 tmp_pos = in_pos;
    tmp_pos.xy = sign(in_pos.xy);
    scr_pos = (vec2(tmp_pos.x, -tmp_pos.y) + 1.0f) * 0.5f;    
}
