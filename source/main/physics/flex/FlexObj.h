/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2015 Petr Ohlidal

	For more information, see http://www.rigsofrods.org/

	Rigs of Rods is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 3, as
	published by the Free Software Foundation.

	Rigs of Rods is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "RoRPrerequisites.h"

#include "BeamData.h"

#include <OgreVector3.h>
#include <OgreMesh.h>
#include <OgreSubMesh.h>
#include <OgreHardwareBuffer.h>

class FlexObj : public ZeroedMemoryAllocator
{
public:

	FlexObj(
		node_t *nds, 
		int numtexcoords, 
		Ogre::Vector3* texcoords, 
		int numtriangles, 
		int* triangles, 
		int numsubmeshes, 
		int* subtexindex, 
		int* subtriindex, 
		char* texname, 
		char* name, 
		int* subisback, 
		char* backtexname, 
		char* transtexname);

	~FlexObj();

	//find the zeroed id of the node v in the context of the tidx triangle
	int findID(int tidx, int v, int numsubmeshes, int* subtexindex, int* subtriindex);
	//with normals
	Ogre::Vector3 updateVertices();
	//with normals
	Ogre::Vector3 updateShadowVertices();
	Ogre::Vector3 flexit();
	void scale(float factor);

private:

	struct CoVertice_t
	{
		Ogre::Vector3 vertex;
		Ogre::Vector3 normal;
		//Ogre::Vector3 color;
		Ogre::Vector2 texcoord;
	};

	struct posVertice_t
	{
		Ogre::Vector3 vertex;
	};

	struct norVertice_t
	{
		Ogre::Vector3 normal;
		//Ogre::Vector3 color;
		Ogre::Vector2 texcoord;
	};

	Ogre::MeshPtr msh;
	Ogre::SubMesh** subs;
	Ogre::VertexDeclaration* decl;
	Ogre::HardwareVertexBufferSharedPtr vbuf;

	size_t nVertices;
	size_t vbufCount;
	//shadow
	union
	{
		float *shadowposvertices;
		posVertice_t *coshadowposvertices;
	};
	union
	{
		float *shadownorvertices;
		norVertice_t *coshadownorvertices;
	};

	union
	{
		float *vertices;
		CoVertice_t *covertices;
	};
	//nodes
	int *nodeIDs;

	size_t ibufCount;
	unsigned short *faces;
	node_t *nodes;
	int nbrays;

	float *sref;
	int triangleCount;
};
