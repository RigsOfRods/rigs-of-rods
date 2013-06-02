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
#ifndef __FlexMesh_H_
#define __FlexMesh_H_

#include "RoRPrerequisites.h"

#include "BeamData.h"
#include "Flexable.h"
#include "Ogre.h"

class FlexMesh: public Flexable
{
public:

	FlexMesh(char* name, node_t *nds, int n1, int n2, int nstart, int nrays, char* texface, char* texband, bool rimmed=false, float rimratio=1.0);

	Ogre::Vector3 updateVertices();
	Ogre::Vector3 updateShadowVertices();

	// Flexable
	void flexitCompute();
	Ogre::Vector3 flexitFinal();

	void setVisible(bool visible);

private:

	typedef struct
	{
		Ogre::Vector3 vertex;
		Ogre::Vector3 normal;
		//	Ogre::Vector3 color;
		Ogre::Vector2 texcoord;
	} CoVertice_t;

	typedef struct
	{
		Ogre::Vector3 vertex;
	} posVertice_t;

	typedef struct
	{
		Ogre::Vector3 normal;
		//	Ogre::Vector3 color;
		Ogre::Vector2 texcoord;
	} norVertice_t;

	Ogre::MeshPtr msh;
	Ogre::SubMesh* subface;
	Ogre::SubMesh* subband;
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

	size_t bandibufCount;
	size_t faceibufCount;
	unsigned short *facefaces;
	unsigned short *bandfaces;
	node_t *nodes;
	int nbrays;
	bool is_rimmed;
	float rim_ratio;
};

#endif // __FlexMesh_H_

