/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __FlexMeshWheel_H__
#define __FlexMeshWheel_H__

#include "RoRPrerequisites.h"

#include "BeamData.h"
#include "FlexMesh.h"
#include "MaterialFunctionMapper.h"
#include "Ogre.h"

class FlexMeshWheel: public Flexable
{
public:

	FlexMeshWheel(char* name, node_t *nds, int n1, int n2, int nstart, int nrays, char* meshname, char* texband, float rimradius, bool rimreverse, MaterialFunctionMapper *mfm, Skin *usedSkin, MaterialReplacer *mr);

	Ogre::Entity *getRimEntity() { return rimEnt; };

	Ogre::Vector3 updateVertices();
	Ogre::Vector3 updateShadowVertices();

	// Flexable
	bool flexitPrepare(Beam* b);
	void flexitCompute();
	Ogre::Vector3 flexitFinal();

	void setVisible(bool visible);

private:

	MaterialReplacer *mr;
	
	typedef struct
	{
		Ogre::Vector3 vertex;
		Ogre::Vector3 normal;
		//Ogre::Vector3 color;
		Ogre::Vector2 texcoord;
	} CoVertice_t;

	typedef struct
	{
		Ogre::Vector3 vertex;
	} posVertice_t;

	typedef struct
	{
		Ogre::Vector3 normal;
		//Ogre::Vector3 color;
		Ogre::Vector2 texcoord;
	} norVertice_t;

	Ogre::MeshPtr msh;
	Ogre::SubMesh* sub;
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
	//int *nodeIDs;
	int id0;
	int id1;
	int idstart;

	size_t ibufCount;
	unsigned short *faces;
	node_t *nodes;
	int nbrays;
	float rim_radius;
	Ogre::SceneNode *rnode;
	float normy;
	bool revrim;
	Ogre::Entity *rimEnt;
};

#endif // __FlexMeshWheel_H__
