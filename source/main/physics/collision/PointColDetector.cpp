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
#include "PointColDetector.h"

#include "Beam.h"
#include "BeamFactory.h"

// Microsoft Visual Studio 2010 doesn't have std::log2
// Version macros: http://stackoverflow.com/a/70630
#if defined(WIN32) && defined(_MSC_VER) && _MSC_VER >= 1600
    double mini_log2(double n)
    {
        return log(n) / log(2.f);
    }
#   define LOG2(n) (mini_log2(n))
#else
#   define LOG2(n) (std::log2(n))
#endif

using namespace Ogre;

PointColDetector::PointColDetector()
   	: object_list_size(-1)
{
	std::vector< refelem_t > ref_list(1);
	std::vector< pointid_t > pointid_list(1);
}

PointColDetector::~PointColDetector() {
}

void PointColDetector::update(Beam* truck) {
	int contacters_size = 0;

	if (truck && truck->state < SLEEPING) {
		contacters_size += truck->free_contacter;
	}

	//If the contacter number has changed, its time to update the kdtree structures
	if (contacters_size != object_list_size) {
		object_list_size = contacters_size;
		update_structures_for_contacters(truck);
	}

	kdtree[0].ref = NULL;
	kdtree[0].begin = 0;
	kdtree[0].end = -object_list_size;
}

void PointColDetector::update(Beam* truck, Beam** trucks, const int numtrucks) {
	int contacters_size = 0;

	truck->collisionRelevant = false;
	if (truck->state < SLEEPING) {
		//Sweep & prune
		for (int t = 0; t < numtrucks; t++) {
			if (t != truck->trucknum && trucks[t] && trucks[t]->state < SLEEPING && BeamFactory::getSingleton().truckIntersectionAABB(t, truck->trucknum)) {
				truck->collisionRelevant = true;
				contacters_size += trucks[t]->free_contacter;
			}
		}
	}

	//If the contacter number has changed, its time to update the kdtree structures
	if (contacters_size != object_list_size) {
		object_list_size = contacters_size;
		update_structures_for_contacters(truck, trucks, numtrucks);
	}

	kdtree[0].ref = NULL;
	kdtree[0].begin = 0;
	kdtree[0].end = -object_list_size;
}

void PointColDetector::update_structures_for_contacters(Beam* truck) {
	kdnode_t kdelem = {0.0f, 0, 0.0f, NULL, 0.0f, 0};
	hit_list.resize(object_list_size, NULL);
	int exp_factor = std::max(0, (int) ceil(LOG2(object_list_size)) + 1);

	ref_list.clear();
	pointid_list.clear();

	ref_list.resize(object_list_size);
	pointid_list.resize(object_list_size);

	int refi = 0;

	//Insert all contacters, into the list of points to consider when building the kdtree
	if (truck && truck->state < SLEEPING) {
		for (int i = 0;i < truck->free_contacter; ++i) {
			ref_list[refi].pidref = &pointid_list[refi];
			pointid_list[refi].truckid = truck->trucknum;
			pointid_list[refi].nodeid = truck->contacters[i].nodeid;
			ref_list[refi].point = &(truck->nodes[pointid_list[refi].nodeid].AbsPosition.x);
			refi++;
		}
	}

	kdtree.resize(pow(2.f, exp_factor), kdelem);
}

void PointColDetector::update_structures_for_contacters(Beam* truck, Beam** trucks, const int numtrucks) {
	kdnode_t kdelem = {0.0f, 0, 0.0f, NULL, 0.0f, 0};
	hit_list.resize(object_list_size, NULL);
	int exp_factor = std::max(0, (int) ceil(LOG2(object_list_size)) + 1);

	ref_list.clear();
	pointid_list.clear();

	ref_list.resize(object_list_size);
	pointid_list.resize(object_list_size);

	int refi = 0;

	//Insert all contacters, into the list of points to consider when building the kdtree
	for (int t = 0; t < numtrucks; t++) {
		if (t == truck->trucknum || !trucks[t] || trucks[t]->state >= SLEEPING || !BeamFactory::getSingleton().truckIntersectionAABB(t, truck->trucknum)) {
			continue;
		}

		for (int i = 0; i < trucks[t]->free_contacter; ++i) {
			ref_list[refi].pidref = &pointid_list[refi];
			pointid_list[refi].truckid = t;
			pointid_list[refi].nodeid = trucks[t]->contacters[i].nodeid;
			ref_list[refi].point = &(trucks[t]->nodes[pointid_list[refi].nodeid].AbsPosition.x);
			refi++;
		}
	}

	kdtree.resize(pow(2.f, exp_factor), kdelem);
}

void PointColDetector::query(const Vector3 &vec1, const Vector3 &vec2, const Vector3 &vec3, float enlargeBB) {
	calc_bounding_box(bbmin, bbmax, vec1, vec2, vec3, enlargeBB);

	hit_count = 0;
	queryrec(0, 0);
}

void PointColDetector::calc_bounding_box(Vector3 &bmin, Vector3 &bmax, const Vector3 &vec1, const Vector3 &vec2, const Vector3 &vec3, const float enlargeBB) {
	if (vec1.y < vec2.y) {
		bmin.y = vec1.y;
		bmax.y = vec2.y;
	}
	else {
		bmin.y = vec2.y;
		bmax.y = vec1.y;
	}

	if (vec3.y < bmin.y) {
		bmin.y = vec3.y;
	}
	else if (vec3.y > bmax.y) {
		bmax.y = vec3.y;
	}

	bmin.y -= enlargeBB;
	bmax.y += enlargeBB;

	if (vec1.x < vec2.x) {
		bmin.x= vec1.x;
		bmax.x= vec2.x;
	}
	else {
		bmin.x= vec2.x;
		bmax.x= vec1.x;
	}

	if (vec3.x < bmin.x) {
		bmin.x= vec3.x;
	}
	else if (vec3.x > bmax.x) {
		bmax.x= vec3.x;
	}

	bmin.x-= enlargeBB;
	bmax.x+= enlargeBB;

	if (vec1.z < vec2.z) {
		bmin.z= vec1.z;
		bmax.z= vec2.z;
	}
	else {
		bmin.z= vec2.z;
		bmax.z= vec1.z;
	}

	if (vec3.z < bmin.z) {
		bmin.z= vec3.z;
	}
	else if (vec3.z > bmax.z) {
		bmax.z= vec3.z;
	}

	bmin.z-= enlargeBB;
	bmax.z+= enlargeBB;
}

void PointColDetector::queryrec(int kdindex, int axis) {
	for (;;) {
		if (kdtree[kdindex].end < 0) {
			build_kdtree_incr(axis, kdindex);
		}

		if (kdtree[kdindex].ref != NULL) {
			float *point = kdtree[kdindex].ref->point;
			if (point[0] >= bbmin.x && point[0] <= bbmax.x && point[1] >= bbmin.y && point[1] <= bbmax.y && point[2] >= bbmin.z && point[2] <= bbmax.z ) {
				hit_list[hit_count] = kdtree[kdindex].ref->pidref;
				hit_count++;
			}
			return;
		}

		int newaxis;
		int newindex;

		if (bbmax[axis] >= kdtree[kdindex].middle) {
			if (bbmin[axis] > kdtree[kdindex].max) {
				return;
			}

			newaxis = axis + 1;

			if (newaxis >= 3) {
				newaxis = 0;
			}

			newindex = kdindex + kdindex + 1;

			if (bbmin[axis] <= kdtree[kdindex].middle) {
				queryrec(newindex, newaxis);
			}

			kdindex = newindex + 1;
			axis = newaxis;
		}
		else {
			if (bbmax[axis] < kdtree[kdindex].min) {
				return;
			}

			kdindex = 2 * kdindex + 1;
			axis++;

			if (axis >= 3) {
				axis = 0;
			}
		}
	}
}

void PointColDetector::build_kdtree_incr(int axis, int index)
{
	int end = -kdtree[index].end;
	kdtree[index].end = end;
	int begin = kdtree[index].begin;
	int median;
	int slice_size = end - begin;
	if (slice_size != 1) {
		int newindex=index+index+1;
		if (slice_size == 2) {
			median = begin+1;
			if (ref_list[begin].point[axis] > ref_list[median].point[axis]) {
				std::swap(ref_list[begin], ref_list[median]);
			}

			kdtree[index].min = ref_list[begin].point[axis];
			kdtree[index].max = ref_list[median].point[axis];
			kdtree[index].middle = kdtree[index].max;
			kdtree[index].ref = NULL;

			axis++;
			if (axis >= 3) {
				axis = 0;
			}

			kdtree[newindex].ref = &ref_list[begin];
			kdtree[newindex].middle = kdtree[newindex].ref->point[axis];
			kdtree[newindex].min = kdtree[newindex].middle;
			kdtree[newindex].max = kdtree[newindex].middle;
			kdtree[newindex].end = median;
			newindex++;

			kdtree[newindex].ref = &ref_list[median];
			kdtree[newindex].middle = kdtree[newindex].ref->point[axis];
			kdtree[newindex].min = kdtree[newindex].middle;
			kdtree[newindex].max = kdtree[newindex].middle;
			kdtree[newindex].end = end;
			return;
		}
		else {
			median = begin + (slice_size / 2);
			partintwo(begin, median, end, axis, kdtree[index].min, kdtree[index].max);
		}

		kdtree[index].middle = ref_list[median].point[axis];
		kdtree[index].ref = NULL;

		kdtree[newindex].begin = begin;
		kdtree[newindex].end = -median;

		newindex++;
		kdtree[newindex].begin = median;
		kdtree[newindex].end = -end;

	}
	else {
		kdtree[index].ref = &ref_list[begin];
		kdtree[index].middle = kdtree[index].ref->point[axis];
		kdtree[index].min = kdtree[index].middle;
		kdtree[index].max = kdtree[index].middle;
	}
}

void PointColDetector::partintwo(const int start, const int median, const int end, const int axis, float &minex, float &maxex)
{
	int i, j, l, m;
	int k = median;
	l = start;
	m = end - 1;

	float x = ref_list[k].point[axis];
	while (l < m) {
		i = l;
		j = m;
		while (!(j < k || k < i)) {
			while (ref_list[i].point[axis] < x) {
				i++;
			}
			while (x < ref_list[j].point[axis]) {
				j--;
			}

			std::swap(ref_list[i], ref_list[j]);
			i++;
			j--;
		}
		if (j < k) {
			l = i;
		}
		if (k < i) {
			m = j;
		}
		x = ref_list[k].point[axis];
	}

	minex = x;
	maxex = x;
	for (int i = start; i < median; ++i) {
		minex = std::min(ref_list[i].point[axis], minex);
	}
	for (int i = median+1; i < end; ++i) {
		maxex = std::max(maxex, ref_list[i].point[axis]);
	}
}
