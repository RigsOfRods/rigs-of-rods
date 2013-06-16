/*
This source file is part of Rigs of Rods
Copyright 2009 Lefteris Stamatogiannakis

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

	PointColDetector(std::vector< Ogre::Vector3 > &o_list);
	PointColDetector();
	~PointColDetector();

	void reset();
	void update();
	void update(Beam* truck);
	void update(Beam** trucks, const int numtrucks);
	void update_structures();
	void update_structures_for_contacters(Beam* truck);
	void update_structures_for_contacters(Beam** trucks, const int numtrucks);
	void querybb(const Ogre::Vector3 &bmin, const Ogre::Vector3 &bmax);
	void query(const Ogre::Vector3 &vec1, const Ogre::Vector3 &vec2, const Ogre::Vector3 &vec3, float enlargeBB=0.0f);
	void query(const Ogre::Vector3 &vec1, const Ogre::Vector3 &vec2, const float enlargeBB=0.0f);
	inline void calc_bounding_box(Ogre::Vector3 &bmin, Ogre::Vector3 &bmax, const Ogre::Vector3 &vec1, const Ogre::Vector3 &vec2, const Ogre::Vector3 &vec3, const float enlargeBB=0.0f);
	inline void calc_bounding_box(Ogre::Vector3 &bmin, Ogre::Vector3 &bmax, const Ogre::Vector3 &vec1, const Ogre::Vector3 &vec2, const float enlargeBB=0.0f);

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
	refelem_t *ref_list;
	pointid_t *pointid_list;
	std::vector< kdnode_t > kdtree;
	Ogre::Vector3 bbmin;
	Ogre::Vector3 bbmax;
	inline void queryrec(int kdindex, int axis);
	void build_kdtree(int begin, int end, int axis, int index);
	void build_kdtree_incr(int axis, int index);
	void partintwo(const int start, const int median, const int end, const int axis, float &minex, float &maxex);
};

#endif // __PointColDetector_H_
