/*
This source file is part of Rigs of Rods
Copyright 2009 Lefteris Stamatogiannakis

For more information, see http://www.rigsofrods.org/

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

#pragma once
#ifndef __PointColDetector_H_
#define __PointColDetector_H_

#include "RoRPrerequisites.h"

class PointColDetector : public ZeroedMemoryAllocator
{
public:

	typedef struct _pointid {
		int nodeid;
		int truckid;
	} pointid_t;

	std::vector< Ogre::Vector3 > *object_list;
	std::vector< pointid_t* > hit_list;
	int hit_count;

	PointColDetector();
	~PointColDetector();

	void update(Beam* truck, bool ignorestate = false);
	void update(Beam* truck, Beam** trucks, const int numtrucks, bool ignorestate = false);
	void query(const Ogre::Vector3 &vec1, const Ogre::Vector3 &vec2, const Ogre::Vector3 &vec3, const float enlargeBB=0.0f);

private:

	typedef struct _refelem {
		pointid_t* pidref;
		float* point;
	} refelem_t;

	typedef struct _kdnode {
		float min;
		int end;
		float max;
		refelem_t* ref;
		float middle;
		int begin;
	} kdnode_t;

	int object_list_size;
	std::vector< Beam* > m_trucks;

	std::vector< refelem_t > ref_list;
	std::vector< pointid_t > pointid_list;
	std::vector< kdnode_t > kdtree;

	Ogre::Vector3 bbmin;
	Ogre::Vector3 bbmax;

	void queryrec(int kdindex, int axis);
	void build_kdtree_incr(int axis, int index);
	void partintwo(const int start, const int median, const int end, const int axis, float &minex, float &maxex);
	void update_structures_for_contacters();
};

#endif // __PointColDetector_H_
