/*
    This source file is part of Rigs of Rods

    Copyright 2009 Lefteris Stamatogiannakis

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

#include "PointColDetector.h"

#include "Beam.h"
#include "RoRFrameListener.h"

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
    : m_object_list_size(-1)
{
}

PointColDetector::~PointColDetector()
{
}

void PointColDetector::UpdateIntraPoint(Actor* actor, bool ignorestate)
{
    int contacters_size = 0;

    if (actor && (ignorestate || actor->ar_sim_state < Actor::SimState::LOCAL_SLEEPING))
    {
        m_actors.resize(1, actor);
        contacters_size += actor->ar_num_contacters;
    }
    else
    {
        m_actors.clear();
    }

    if (contacters_size != m_object_list_size)
    {
        m_object_list_size = contacters_size;
        update_structures_for_contacters();
    }

    m_kdtree[0].ref = NULL;
    m_kdtree[0].begin = 0;
    m_kdtree[0].end = -m_object_list_size;
}

void PointColDetector::UpdateInterPoint(Actor* truck, bool ignorestate)
{
    bool update_required = false;
    int contacters_size = 0;

    if (truck && (ignorestate || truck->ar_update_physics))
    {
        auto all_actors = RoR::App::GetSimController()->GetActors();
        truck->ar_collision_relevant = false;
        m_actors.resize(all_actors.size());
        for (int t = 0; t < all_actors.size(); t++)
        {
            if (all_actors[t] != truck && (ignorestate || all_actors[t]->ar_update_physics) && truck->ar_bounding_box.intersects(all_actors[t]->ar_bounding_box))
            {
                update_required = update_required || (m_actors[t] != all_actors[t]);
                m_actors[t] = all_actors[t];
                truck->ar_collision_relevant = true;
                contacters_size += all_actors[t]->ar_num_contacters;
                if (truck->ar_nodes[0].Velocity.squaredDistance(all_actors[t]->ar_nodes[0].Velocity) > 25)
                {
                    for (int i=0; i<truck->ar_num_collcabs; i++)
                    {
                        truck->ar_intra_collcabrate[i].rate = 0;
                        truck->ar_inter_collcabrate[i].rate = 0;
                    }
                    for (int i=0; i<all_actors[t]->ar_num_collcabs; i++)
                    {
                        all_actors[t]->ar_intra_collcabrate[i].rate = 0;
                        all_actors[t]->ar_inter_collcabrate[i].rate = 0;
                    }
                }
            }
            else
            {
                m_actors[t] = 0;
            }
        }
    }
    else
    {
        m_actors.clear();
    }

    if (update_required || contacters_size != m_object_list_size)
    {
        m_object_list_size = contacters_size;
        update_structures_for_contacters();
    }

    m_kdtree[0].ref = NULL;
    m_kdtree[0].begin = 0;
    m_kdtree[0].end = -m_object_list_size;
}

void PointColDetector::update_structures_for_contacters()
{
    kdnode_t kdelem = {0.0f, 0, 0.0f, NULL, 0.0f, 0};
    hit_list.resize(m_object_list_size, NULL);
    int exp_factor = std::max(0, (int) ceil(LOG2(m_object_list_size)) + 1);

    m_ref_list.clear();
    m_pointid_list.clear();

    m_ref_list.resize(m_object_list_size);
    m_pointid_list.resize(m_object_list_size);

    int refi = 0;

    //Insert all contacters, into the list of points to consider when building the kdtree
    for (auto actor : m_actors)
    {
        if (actor)
        {
            for (int i = 0; i < actor->ar_num_contacters; ++i)
            {
                m_ref_list[refi].pidref = &m_pointid_list[refi];
                m_pointid_list[refi].actor_id = actor->ar_vector_index;
                m_pointid_list[refi].node_id = actor->ar_contacters[i];
                m_ref_list[refi].point = &(actor->ar_nodes[m_pointid_list[refi].node_id].AbsPosition.x);
                refi++;
            }
        }
    }

    m_kdtree.resize(pow(2.f, exp_factor), kdelem);
}

void PointColDetector::query(const Vector3 &vec1, const Vector3 &vec2, const Vector3 &vec3, float enlargeBB)
{
    Vector3 enlarge = Vector3(enlargeBB, enlargeBB, enlargeBB);

    m_bbmin = vec1;

    m_bbmin.x = std::min(vec2.x, m_bbmin.x);
    m_bbmin.x = std::min(vec3.x, m_bbmin.x);

    m_bbmin.y = std::min(vec2.y, m_bbmin.y);
    m_bbmin.y = std::min(vec3.y, m_bbmin.y);

    m_bbmin.z = std::min(vec2.z, m_bbmin.z);
    m_bbmin.z = std::min(vec3.z, m_bbmin.z);

    m_bbmin -= enlarge;

    m_bbmax = vec1;

    m_bbmax.x = std::max(m_bbmax.x, vec2.x);
    m_bbmax.x = std::max(m_bbmax.x, vec3.x);

    m_bbmax.y = std::max(m_bbmax.y, vec2.y);
    m_bbmax.y = std::max(m_bbmax.y, vec3.y);

    m_bbmax.z = std::max(m_bbmax.z, vec2.z);
    m_bbmax.z = std::max(m_bbmax.z, vec3.z);

    m_bbmax += enlarge;

    hit_count = 0;
    queryrec(0, 0);
}

void PointColDetector::queryrec(int kdindex, int axis)
{
    for (;;)
    {
        if (m_kdtree[kdindex].end < 0)
        {
            build_kdtree_incr(axis, kdindex);
        }

        if (m_kdtree[kdindex].ref != NULL)
        {
            float *point = m_kdtree[kdindex].ref->point;
            if (point[0] >= m_bbmin.x && point[0] <= m_bbmax.x && point[1] >= m_bbmin.y && point[1] <= m_bbmax.y && point[2] >= m_bbmin.z && point[2] <= m_bbmax.z )
            {
                hit_list[hit_count] = m_kdtree[kdindex].ref->pidref;
                hit_count++;
            }
            return;
        }

        int newaxis;
        int newindex;

        if (m_bbmax[axis] >= m_kdtree[kdindex].middle)
        {
            if (m_bbmin[axis] > m_kdtree[kdindex].max)
            {
                return;
            }

            newaxis = axis + 1;

            if (newaxis >= 3) {
                newaxis = 0;
            }

            newindex = kdindex + kdindex + 1;

            if (m_bbmin[axis] <= m_kdtree[kdindex].middle)
            {
                queryrec(newindex, newaxis);
            }

            kdindex = newindex + 1;
            axis = newaxis;
        }
        else
        {
            if (m_bbmax[axis] < m_kdtree[kdindex].min)
            {
                return;
            }

            kdindex = 2 * kdindex + 1;
            axis++;

            if (axis >= 3)
            {
                axis = 0;
            }
        }
    }
}

void PointColDetector::build_kdtree_incr(int axis, int index)
{
    int end = -m_kdtree[index].end;
    m_kdtree[index].end = end;
    int begin = m_kdtree[index].begin;
    int median;
    int slice_size = end - begin;
    if (slice_size != 1)
    {
        int newindex=index+index+1;
        if (slice_size == 2)
        {
            median = begin+1;
            if (m_ref_list[begin].point[axis] > m_ref_list[median].point[axis])
            {
                std::swap(m_ref_list[begin], m_ref_list[median]);
            }

            m_kdtree[index].min = m_ref_list[begin].point[axis];
            m_kdtree[index].max = m_ref_list[median].point[axis];
            m_kdtree[index].middle = m_kdtree[index].max;
            m_kdtree[index].ref = NULL;

            axis++;
            if (axis >= 3)
            {
                axis = 0;
            }

            m_kdtree[newindex].ref = &m_ref_list[begin];
            m_kdtree[newindex].middle = m_kdtree[newindex].ref->point[axis];
            m_kdtree[newindex].min = m_kdtree[newindex].middle;
            m_kdtree[newindex].max = m_kdtree[newindex].middle;
            m_kdtree[newindex].end = median;
            newindex++;

            m_kdtree[newindex].ref = &m_ref_list[median];
            m_kdtree[newindex].middle = m_kdtree[newindex].ref->point[axis];
            m_kdtree[newindex].min = m_kdtree[newindex].middle;
            m_kdtree[newindex].max = m_kdtree[newindex].middle;
            m_kdtree[newindex].end = end;
            return;
        }
        else
        {
            median = begin + (slice_size / 2);
            partintwo(begin, median, end, axis, m_kdtree[index].min, m_kdtree[index].max);
        }

        m_kdtree[index].middle = m_ref_list[median].point[axis];
        m_kdtree[index].ref = NULL;

        m_kdtree[newindex].begin = begin;
        m_kdtree[newindex].end = -median;

        newindex++;
        m_kdtree[newindex].begin = median;
        m_kdtree[newindex].end = -end;

    }
    else
    {
        m_kdtree[index].ref = &m_ref_list[begin];
        m_kdtree[index].middle = m_kdtree[index].ref->point[axis];
        m_kdtree[index].min = m_kdtree[index].middle;
        m_kdtree[index].max = m_kdtree[index].middle;
    }
}

void PointColDetector::partintwo(const int start, const int median, const int end, const int axis, float &minex, float &maxex)
{
    int i, j, l, m;
    int k = median;
    l = start;
    m = end - 1;

    float x = m_ref_list[k].point[axis];
    while (l < m)
    {
        i = l;
        j = m;
        while (!(j < k || k < i))
        {
            while (m_ref_list[i].point[axis] < x)
            {
                i++;
            }
            while (x < m_ref_list[j].point[axis])
            {
                j--;
            }

            std::swap(m_ref_list[i], m_ref_list[j]);
            i++;
            j--;
        }
        if (j < k)
        {
            l = i;
        }
        if (k < i)
        {
            m = j;
        }
        x = m_ref_list[k].point[axis];
    }

    minex = x;
    maxex = x;
    for (int i = start; i < median; ++i)
    {
        minex = std::min(m_ref_list[i].point[axis], minex);
    }
    for (int i = median+1; i < end; ++i)
    {
        maxex = std::max(maxex, m_ref_list[i].point[axis]);
    }
}
