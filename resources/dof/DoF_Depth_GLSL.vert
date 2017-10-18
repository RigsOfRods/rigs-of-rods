// "Depth of Field" demo for Ogre
// Copyright (C) 2006  Christian Lindequist Larsen
//
// This code is in the public domain. You may do whatever you want with it.

varying float depth; // in view space

void main()
{
	vec4 viewPos = gl_ModelViewMatrix * gl_Vertex;
	depth = -viewPos.z;

	gl_Position = ftransform(); 
}
