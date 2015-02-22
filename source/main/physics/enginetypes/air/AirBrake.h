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
#ifndef __AIRBRAKE_H_
#define __AIRBRAKE_H_

#include "RoRPrerequisites.h"

#include <OgreMesh.h>

class Airbrake : public ZeroedMemoryAllocator
{
	friend class RigInspector; // Debug utility class

private:

	typedef struct
	{
		Ogre::Vector3 vertex;
		Ogre::Vector3 normal;
		Ogre::Vector2 texcoord;
	} CoVertice_t;

	Ogre::MeshPtr msh;
	Ogre::SceneNode *snode;
	node_t *noderef;
	node_t *nodex;
	node_t *nodey;
	node_t *nodea;
	Ogre::Vector3 offset;
	float ratio;
	float maxangle;
	float area;

public:
	Airbrake(char* basename, int num, node_t *ndref, node_t *ndx, node_t *ndy, node_t *nda, Ogre::Vector3 pos, float width, float length, float maxang, char* texname, float tx1, float tx2, float tx3, float tx4, float lift_coef);

	void updatePosition(float amount);
	void applyForce();
};

#endif // __AIRBRAKE_H_
