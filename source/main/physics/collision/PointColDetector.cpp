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

using namespace Ogre;

PointColDetector::PointColDetector(std::vector < Vector3 > &o_list) :
	object_list(&o_list)
{
	ref_list=new refelem_t[1];
	pointid_list=new pointid_t[1];

	update();
}

PointColDetector::PointColDetector()
{
	ref_list=new refelem_t[1];
	pointid_list=new pointid_t[1];
	object_list_size=-1;
}

PointColDetector::~PointColDetector()
{
	delete [] ref_list;
	delete [] pointid_list;
}

void PointColDetector::reset()
{
	object_list_size=-1;
}

void PointColDetector::update()
{
	if (object_list->size()!=(unsigned int) object_list_size)
	{
		object_list_size = (int)object_list->size();
		update_structures();
	}

	kdtree[0].ref=NULL;
	kdtree[0].begin=0;
	kdtree[0].end=-object_list_size;
	//build_kdtree(0, object_list_size, 0, 0);
}

void PointColDetector::update(Beam* truck)
{
	int contacters_size=0;

	if (truck && truck->state < SLEEPING)
		contacters_size+=truck->free_contacter;

	//If the contacter number has changed, its time to update the kdtree structures
	if (truck->free_contacter!=object_list_size)
	{
		object_list_size = contacters_size;
		update_structures_for_contacters(truck);
	}

	kdtree[0].ref=NULL;
	kdtree[0].begin=0;
	kdtree[0].end=-object_list_size;
}

void PointColDetector::update(Beam** trucks, const int numtrucks)
{
	int t, contacters_size=0;

	//Count the contacters of all trucks
	for (t=0; t<numtrucks; t++)
	{
		if (!trucks[t] || trucks[t]->state >= SLEEPING) continue;

		//Sweep & prune
		trucks[t]->collisionRelevant=false;
		bool skipIt=true;
		for (int j=0; j<numtrucks; j++)
		{
			if (j!=t && trucks[j] && trucks[j]->state < SLEEPING && BeamFactory::getSingleton().truckIntersectionAABB(t, j))
			{
				trucks[t]->collisionRelevant=true;
				skipIt=false;
				break;
			}
		}
		if (skipIt) continue;

		contacters_size+=trucks[t]->free_contacter;
	}

	//If the contacter number has changed, its time to update the kdtree structures
	if (contacters_size!=object_list_size)
	{
		object_list_size = contacters_size;
		update_structures_for_contacters(trucks, numtrucks);
	}

	kdtree[0].ref=NULL;
	kdtree[0].begin=0;
	kdtree[0].end=-object_list_size;
}

void PointColDetector::update_structures()
{
	kdnode_t kdelem={0.0f, 0, 0.0f, NULL, 0.0f, 0};
	hit_list.resize(object_list_size, NULL);

	delete [] ref_list;
	delete [] pointid_list;
	ref_list=new refelem_t[object_list_size];
	pointid_list=new pointid_t[object_list_size];

	kdtree.resize(1<<(int) (ceil(log((float) object_list_size)/log(2.0f))+1), kdelem);
}

void PointColDetector::update_structures_for_contacters(Beam* truck)
{
	kdnode_t kdelem={0.0f, 0, 0.0f, NULL, 0.0f, 0};
	hit_list.resize(object_list_size, NULL);

	delete [] ref_list;
	delete [] pointid_list;
	ref_list=new refelem_t[object_list_size];
	pointid_list=new pointid_t[object_list_size];

	int refi=0;

	//Insert all contacters, into the list of points to consider when building the kdtree
	if (truck && truck->state < SLEEPING)
	{
		for (int i=0;i<truck->free_contacter;++i)
		{
			ref_list[refi].pidref=&pointid_list[refi];
			pointid_list[refi].truckid=truck->trucknum;
			pointid_list[refi].nodeid=truck->contacters[i].nodeid;
			ref_list[refi].point=&(truck->nodes[pointid_list[refi].nodeid].AbsPosition.x);
			refi++;
		}
	}

	kdtree.resize(1<<(int) (ceil(log((float) object_list_size)/log(2.0f))+1), kdelem);
}

void PointColDetector::update_structures_for_contacters(Beam** trucks, const int numtrucks)
{
	kdnode_t kdelem={0.0f, 0, 0.0f, NULL, 0.0f, 0};
	hit_list.resize(object_list_size, NULL);

	delete [] ref_list;
	delete [] pointid_list;
	ref_list=new refelem_t[object_list_size];
	pointid_list=new pointid_t[object_list_size];

	int t, refi=0;

	//Insert all contacters, into the list of points to consider when building the kdtree
	for (t=0; t<numtrucks; t++)
	{
		if (!trucks[t] || !trucks[t]->collisionRelevant || trucks[t]->state >= SLEEPING) continue;

		for (int i=0;i<trucks[t]->free_contacter;++i)
		{
			ref_list[refi].pidref=&pointid_list[refi];
			pointid_list[refi].truckid=t;
			pointid_list[refi].nodeid=trucks[t]->contacters[i].nodeid;
			ref_list[refi].point=&(trucks[t]->nodes[pointid_list[refi].nodeid].AbsPosition.x);
			refi++;
		}
	}

	kdtree.resize(1<<(int) (ceil(log((float) object_list_size)/log(2.0f))+1), kdelem);
}

void PointColDetector::querybb(const Vector3 &bmin, const Vector3 &bmax)
{
	bbmin=bmin;
	bbmax=bmax;

	hit_count=0;
	queryrec(0, 0);
}

void PointColDetector::query(const Vector3 &vec1, const Vector3 &vec2, const Vector3 &vec3, float enlargeBB)
{
	calc_bounding_box(bbmin, bbmax, vec1, vec2, vec3, enlargeBB);

	hit_count=0;
	queryrec(0, 0);
}

void PointColDetector::query(const Vector3 &vec1, const Vector3 &vec2, const float enlargeBB)
{
	calc_bounding_box(bbmin, bbmax, vec1, vec2, enlargeBB);

	hit_count=0;
	queryrec(0, 0);
}

inline void PointColDetector::calc_bounding_box(Vector3 &bmin, Vector3 &bmax, const Vector3 &vec1, const Vector3 &vec2, const Vector3 &vec3, const float enlargeBB)
{
	if (vec1.y < vec2.y) {
		bmin.y= vec1.y; bmax.y= vec2.y;
	} else {
		bmin.y= vec2.y; bmax.y= vec1.y;
	}
	if (vec3.y < bmin.y) {
		bmin.y= vec3.y;
	} else {
		if (vec3.y > bmax.y) bmax.y= vec3.y;
	}
	bmin.y-= enlargeBB;
	bmax.y+= enlargeBB;

	if (vec1.x < vec2.x) {
		bmin.x= vec1.x; bmax.x= vec2.x;
	} else {
		bmin.x= vec2.x; bmax.x= vec1.x;
	}
	if (vec3.x < bmin.x) {
		bmin.x= vec3.x;
	} else {
		if (vec3.x > bmax.x) bmax.x= vec3.x;
	}
	bmin.x-= enlargeBB;
	bmax.x+= enlargeBB;

	if (vec1.z < vec2.z) {
		bmin.z= vec1.z; bmax.z= vec2.z;
	} else {
		bmin.z= vec2.z; bmax.z= vec1.z;
	}
	if (vec3.z < bmin.z) {
		bmin.z= vec3.z;
	} else {
		if (vec3.z > bmax.z) bmax.z= vec3.z;
	}
	bmin.z-= enlargeBB;
	bmax.z+= enlargeBB;
}

inline void PointColDetector::calc_bounding_box(Vector3 &bmin, Vector3 &bmax, const Vector3 &vec1, const Vector3 &vec2, const float enlargeBB)
{
	if (vec1.x < vec2.x) {
		bmin.x= vec1.x; bmax.x=vec2.x;
	} else {
		bmin.x= vec2.x; bmax.x=vec1.x;
	}
	bmin.x-= enlargeBB;
	bmax.x+= enlargeBB;

	if (vec1.y < vec2.y) {
		bmin.y= vec1.y; bmax.y=vec2.y;
	} else {
		bmin.y= vec2.y; bmax.y=vec1.y;
	}
	bmin.y-= enlargeBB;
	bmax.y+= enlargeBB;

	if (vec1.z < vec2.z) {
		bmin.z= vec1.z; bmax.z=vec2.z;
	} else {
		bmin.z= vec2.z; bmax.z=vec1.z;
	}
	bmin.z-= enlargeBB;
	bmax.z+= enlargeBB;
}

inline void PointColDetector::queryrec(int kdindex, int axis)
{
	for (;;)
	{
		if (kdtree[kdindex].end < 0)
		{
			build_kdtree_incr(axis, kdindex);
		}

		if (kdtree[kdindex].ref != NULL)
		{
			float *point = kdtree[kdindex].ref->point;
			if (point[0] >= bbmin.x && point[0] <= bbmax.x
			  && point[1] >= bbmin.y && point[1] <= bbmax.y
			  && point[2] >= bbmin.z && point[2] <= bbmax.z )
			{
				hit_list[hit_count] = kdtree[kdindex].ref->pidref;
				hit_count++;
			}
			return;
		}

		int newaxis;
		int newindex;

		if (bbmax[axis] >= kdtree[kdindex].middle)
		{
			if (bbmin[axis] > kdtree[kdindex].max)
			{
				return;
			}

			newaxis = axis + 1;
			
			if (newaxis == 3)
			{
				newaxis = 0;
			}

			newindex = kdindex+kdindex+1;

			if (bbmin[axis] <= kdtree[kdindex].middle)
			{
				queryrec(newindex, newaxis);
			}

			kdindex = newindex+1;
			axis = newaxis;
		}
		else
		{
			if (bbmax[axis] < kdtree[kdindex].min)
			{
				return;
			}

			kdindex = kdindex + kdindex + 1;
			axis++;

			if (axis == 3)
			{
				axis = 0;
			}
		}
	}
}

void PointColDetector::build_kdtree(int begin, int end, int axis, int index)
{
	int median;
	for (;;)
	{
		int slice_size = end-begin;
		if (slice_size != 1)
		{
			if (slice_size == 2)
			{
				median = begin + 1;

				if (ref_list[begin].point[axis] > ref_list[median].point[axis])
				{
					std::swap(ref_list[begin],ref_list[median]);
				}

				kdtree[index].min = ref_list[begin].point[axis];
				kdtree[index].max = ref_list[median].point[axis];
			}
			else
			{
				median = begin + (slice_size>>1);
				partintwo(begin, median, end, axis, kdtree[index].min, kdtree[index].max);
			}

			kdtree[index].middle = ref_list[median].point[axis];
			kdtree[index].ref = NULL;

			axis++;

			if (axis==3)
			{
				axis=0;
			}

			int newindex = index + index + 1;
			build_kdtree(begin, median, axis, newindex);

			//Tail cutting
			index = newindex+1;
			begin = median;
		}
		else
		{
			kdtree[index].middle = ref_list[begin].point[axis];
			kdtree[index].ref = &ref_list[begin];
			kdtree[index].min = ref_list[begin].point[axis];
			kdtree[index].max = ref_list[begin].point[axis];

			return;
		}
	}
}

void PointColDetector::build_kdtree_incr(int axis, int index)
{
	int end=-kdtree[index].end;
	kdtree[index].end=end;
	int begin=kdtree[index].begin;
	int median;
	int slice_size=end-begin;
	if (slice_size!=1) {
		int newindex=index+index+1;
		if (slice_size==2){
			median=begin+1;
			if (ref_list[begin].point[axis]>ref_list[median].point[axis])
			{
				std::swap(ref_list[begin],ref_list[median]);
			}

			kdtree[index].min=ref_list[begin].point[axis];
			kdtree[index].max=ref_list[median].point[axis];
			kdtree[index].middle=kdtree[index].max;
			kdtree[index].ref=NULL;

			axis++;
			if (axis==3) axis=0;

			kdtree[newindex].ref=&ref_list[begin];
			kdtree[newindex].middle=kdtree[newindex].ref->point[axis];
			kdtree[newindex].min=kdtree[newindex].middle;
			kdtree[newindex].max=kdtree[newindex].middle;
			kdtree[newindex].end=median;
			newindex++;

			kdtree[newindex].ref=&ref_list[median];
			kdtree[newindex].middle=kdtree[newindex].ref->point[axis];
			kdtree[newindex].min=kdtree[newindex].middle;
			kdtree[newindex].max=kdtree[newindex].middle;
			kdtree[newindex].end=end;
			return;
		} else
		{
			median=begin+(slice_size>>1);
			partintwo(begin, median, end, axis, kdtree[index].min, kdtree[index].max);
		}

		kdtree[index].middle=ref_list[median].point[axis];
		kdtree[index].ref=NULL;

		kdtree[newindex].begin=begin;
		kdtree[newindex].end=-median;

		newindex++;
		kdtree[newindex].begin=median;
		kdtree[newindex].end=-end;

	} else
	{
		kdtree[index].ref=&ref_list[begin];
		kdtree[index].middle=kdtree[index].ref->point[axis];
		kdtree[index].min=kdtree[index].middle;
		kdtree[index].max=kdtree[index].middle;
	}
}

void PointColDetector::partintwo(const int start, const int median, const int end, const int axis, float &minex, float &maxex)
{
	int i, j, l, m;
	refelem_t t;
	int k = median;
	l = start;
	m = end - 1;

	float x = ref_list[k].point[axis];
	while (l < m) {
		i = l;
		j = m;
		do {
			while (ref_list[i].point[axis] < x) i++;
			while (x < ref_list[j].point[axis]) j--;

			t = ref_list[i];
			ref_list[i] = ref_list[j];
			ref_list[j] = t;
			i++;
			j--;
		} while (!(j < k || k < i));
		if (j < k) l = i;
		if (k < i) m = j;
		x = ref_list[k].point[axis];
	}

	minex = maxex = x;
	for (i = start; i < median; ++i)
		if (ref_list[i].point[axis] < minex) minex = ref_list[i].point[axis];
	for (i = median+1; i < end; ++i)
		if (ref_list[i].point[axis] > maxex) maxex = ref_list[i].point[axis];
}
