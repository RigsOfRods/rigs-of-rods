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
#ifndef __FlexBody_H__
#define __FlexBody_H__

#include "RoRPrerequisites.h"

#include "BeamData.h"
#include "Flexable.h"
#include "MaterialFunctionMapper.h"
#include "Ogre.h"

class FlexBody : public Flexable
{
public:

	FlexBody(node_t *nds, int numnodes, char* meshname, char* uname, int ref, int nx, int ny, Ogre::Vector3 offset, Ogre::Quaternion rot, char* setdef, MaterialFunctionMapper *mfm, Skin *usedSkin, bool forceNoShadows, MaterialReplacer *mr);

	void addinterval(int from, int to);
	bool isinset(int n);
	void printMeshInfo(Ogre::Mesh* mesh);
	void reset();
	void updateBlend();
	void writeBlend();
	Ogre::SceneNode *getSceneNode() { return snode; };

	void setEnabled(bool e);

	void setCameraMode(int mode) { cameramode = mode; };
	int getCameraMode() { return cameramode; };

	// Flexable
	bool flexitPrepare(Beam* b);
	void flexitCompute();
	Ogre::Vector3 flexitFinal();

	void setVisible(bool visible);

private:

	MaterialReplacer *mr;

	typedef struct
	{
		int from;
		int to;
	} interval_t;

	typedef struct
	{
		int ref;
		int nx;
		int ny;
		int nz;
		Ogre::Vector3 coords;
	} Locator_t;

	static const int MAX_SET_INTERVALS = 256;

	node_t *nodes;
	int numnodes;
	size_t vertex_count;
	Ogre::Vector3* vertices;
	Ogre::Vector3* dstpos;
	Ogre::Vector3* srcnormals;
	Ogre::Vector3* dstnormals;
	Ogre::ARGB* srccolors;
	Locator_t *locs; //1 loc per vertex

	int cref;
	int cx;
	int cy;
	Ogre::Vector3 coffset;
	Ogre::SceneNode *snode;

	interval_t nodeset[MAX_SET_INTERVALS];
	int freenodeset;

	int sharedcount;
	Ogre::HardwareVertexBufferSharedPtr sharedpbuf;
	Ogre::HardwareVertexBufferSharedPtr sharednbuf;
	Ogre::HardwareVertexBufferSharedPtr sharedcbuf;
	int numsubmeshbuf;
	int *submeshnums;
	int *subnodecounts;
	Ogre::HardwareVertexBufferSharedPtr subpbufs[16];//positions
	Ogre::HardwareVertexBufferSharedPtr subnbufs[16];//normals
	Ogre::HardwareVertexBufferSharedPtr subcbufs[16];//colors

	bool enabled;

	int cameramode;

	bool hasshared;
	bool hasshadows;
	bool hastangents;
	bool hastexture;
	bool hasblend;
	bool faulty;

	Ogre::MeshPtr msh;
};

#endif // __FlexBody_H__
