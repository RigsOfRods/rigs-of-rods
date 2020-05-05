/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-2014 Torus Knot Software Ltd
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

//-----------------------------------------------------------------------------
// Program Name: SGXLib_NormalMapLighting
// Program Desc: Normal map lighting functions.
// Program Type: Vertex/Pixel shader
// Language: GLSL
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void SGX_ConstructTBNMatrix(in vec3 vNormal,
				   in vec3 vTangent,
				   out mat3 vOut)
{
	vec3 vBinormal = cross(vNormal, vTangent);

	vOut[0][0] = vTangent.x;
	vOut[1][0] = vTangent.y;
	vOut[2][0] = vTangent.z;

	vOut[0][1] = vBinormal.x;
	vOut[1][1] = vBinormal.y;
	vOut[2][1] = vBinormal.z;

	vOut[0][2] = vNormal.x;
	vOut[1][2] = vNormal.y;
	vOut[2][2] = vNormal.z;
}

//-----------------------------------------------------------------------------
void SGX_Generate_Parallax_Texcoord(in sampler2D normalHeightMap,
						in vec2 texCoord,
						in vec3 eyeVec,
						in vec2 scaleBias,
						out vec2 newTexCoord)
{
	eyeVec = normalize(eyeVec);
	float height = texture2D(normalHeightMap, texCoord).a;
	float displacement = (height * scaleBias.x) + scaleBias.y;
	vec3 scaledEyeDir = eyeVec * displacement;
	newTexCoord = (scaledEyeDir  + vec3(texCoord, 1.0)).xy;
}