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

//created by Thomas Fischer 22th of January 2011

#ifndef __DecalManager_H_
#define __DecalManager_H_

#include "RoRPrerequisites.h"
//#include <math.h>
//

class DecalSpline : public ZeroedMemoryAllocator
{
public:

	DecalSpline(Ogre::SceneNode* parent);
	~DecalSpline();

	int addPoint(Ogre::Vector3 v);
	int showDebugLine(bool enabled);

private:

	Ogre::ManualObject *mo_spline;
	Ogre::SceneNode *mo_spline_node, *snparent;
	Ogre::SimpleSpline *spline;
};

class DecalManager : public ZeroedMemoryAllocator
{
public:

	DecalManager();
	~DecalManager();

	int parseLine(char *line);

private:

	int addTerrainDecal(Ogre::Vector3 position, Ogre::Vector2 size, Ogre::Vector2 numSeg, Ogre::Real rotation, Ogre::String materialname, Ogre::String normalname);
	int addTerrainSplineDecal(Ogre::SimpleSpline *spline, float width, Ogre::Vector2 numSeg, Ogre::Vector2 uvSeg, Ogre::String materialname, float ground_offset, Ogre::String export_fn, bool debug);
	int finishTerrainDecal();

	Ogre::SceneNode *terrain_decals_snode;
	Ogre::StaticGeometry *terrain_decals_sg;
	int terrain_decal_count;

	// parser things
	Ogre::String splinemat, spline_export_fn;
	bool decalSplineMode;
	float spline_width, splinetex_u, splinetex_v, ground_offset;
	int spline_segments_x, spline_segments_y;
};

#endif // __DecalManager_H_
