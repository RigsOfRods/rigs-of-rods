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

#include "Actor.h"
#include "ActorManager.h"
#include "GameContext.h"

using namespace Ogre;
using namespace RoR;

void PointColDetector::UpdateIntraPoint(bool contactables)
{
    int contacters_size = contactables ? m_actor->ar_num_contactable_nodes : m_actor->ar_num_contacters;

    if (contacters_size != m_object_list_size)
    {
        m_collision_partners.clear();
        m_collision_partners.push_back(m_actor->ar_instance_id);
        m_object_list_size = contacters_size;
        update_structures_for_contacters(contactables);
    }

    m_kdtree[0].ref = NULL;
    m_kdtree[0].begin = 0;
    m_kdtree[0].end = -m_object_list_size;
}

void PointColDetector::UpdateInterPoint(bool ignorestate)
{
    int contacters_size = 0;
    std::vector<ActorInstanceID_t> collision_partners;
    for (ActorPtr& actor : App::GetGameContext()->GetActorManager()->GetActors())
    {
        if (actor != m_actor && (ignorestate || actor->ar_update_physics) &&
                m_actor->ar_bounding_box.intersects(actor->ar_bounding_box))
        {
            collision_partners.push_back(actor->ar_instance_id);
            bool is_linked = std::find(m_actor->ar_linked_actors.begin(), m_actor->ar_linked_actors.end(), actor) != m_actor->ar_linked_actors.end();
            contacters_size += is_linked ? actor->ar_num_contacters : actor->ar_num_contactable_nodes;
            if (m_actor->ar_nodes[0].Velocity.squaredDistance(actor->ar_nodes[0].Velocity) > 16)
            {
                for (int i = 0; i < m_actor->ar_num_collcabs; i++)
                {
                    m_actor->ar_intra_collcabrate[i].rate = 0;
                    m_actor->ar_inter_collcabrate[i].rate = 0;
                }
                for (int i = 0; i < actor->ar_num_collcabs; i++)
                {
                    actor->ar_intra_collcabrate[i].rate = 0;
                    actor->ar_inter_collcabrate[i].rate = 0;
                }
            }
        }
    }

    m_actor->ar_collision_relevant = (contacters_size > 0);

    if (collision_partners != m_collision_partners || contacters_size != m_object_list_size)
    {
        m_collision_partners = collision_partners;
        m_object_list_size = contacters_size;
        update_structures_for_contacters(false);
    }

    m_kdtree[0].ref = NULL;
    m_kdtree[0].begin = 0;
    m_kdtree[0].end = -m_object_list_size;
}

void PointColDetector::update_structures_for_contacters(bool ignoreinternal)
{
    m_ref_list.resize(m_object_list_size);
    m_pointid_list.resize(m_object_list_size);

    // Insert all contacters into the list of points to consider when building the kdtree
    int refi = 0;
    for (ActorInstanceID_t actorid : m_collision_partners)
    {
        // We cannot use `ActorPtr` here because reference counting isn't thread-safe
        // and this code is also invoked from physics worker threads.
        Actor* actor = App::GetGameContext()->GetActorManager()->WorkerThreadGetActorByIdUnsafe(actorid);

        bool is_linked = std::find(m_actor->ar_linked_actors.begin(), m_actor->ar_linked_actors.end(), actor) != m_actor->ar_linked_actors.end();
        bool internal_collision = !ignoreinternal && ((actorid == m_actor->ar_instance_id) || is_linked);
        for (NodeNum_t i = 0; i < static_cast<NodeNum_t>(actor->ar_nodes.size()); i++)
        {
            if (actor->ar_nodes[i].nd_contacter || (!internal_collision && actor->ar_nodes[i].nd_contactable))
            {
                m_pointid_list[refi].pd_actorid = actorid;
                m_pointid_list[refi].pd_nodenum = i;
                m_ref_list[refi].pidref = &m_pointid_list[refi];
                m_ref_list[refi].point = actor->ar_nodes[i].AbsPosition.ptr();
                refi++;
            }
        }
    }

    m_kdtree.resize(std::max(1.0, std::pow(2, std::ceil(std::log2(m_object_list_size)) + 1)));
}

void PointColDetector::query(const Vector3 &vec1, const Vector3 &vec2, const Vector3 &vec3, float enlargeBB)
{
    m_bbmin = vec1;

    m_bbmin.x = std::min(vec2.x, m_bbmin.x);
    m_bbmin.x = std::min(vec3.x, m_bbmin.x);

    m_bbmin.y = std::min(vec2.y, m_bbmin.y);
    m_bbmin.y = std::min(vec3.y, m_bbmin.y);

    m_bbmin.z = std::min(vec2.z, m_bbmin.z);
    m_bbmin.z = std::min(vec3.z, m_bbmin.z);

    m_bbmin -= enlargeBB;

    m_bbmax = vec1;

    m_bbmax.x = std::max(m_bbmax.x, vec2.x);
    m_bbmax.x = std::max(m_bbmax.x, vec3.x);

    m_bbmax.y = std::max(m_bbmax.y, vec2.y);
    m_bbmax.y = std::max(m_bbmax.y, vec3.y);

    m_bbmax.z = std::max(m_bbmax.z, vec2.z);
    m_bbmax.z = std::max(m_bbmax.z, vec3.z);

    m_bbmax += enlargeBB;

    hit_list.clear();
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
            const float *point = m_kdtree[kdindex].ref->point;
            if (point[0] >= m_bbmin.x && point[0] <= m_bbmax.x &&
                point[1] >= m_bbmin.y && point[1] <= m_bbmax.y &&
                point[2] >= m_bbmin.z && point[2] <= m_bbmax.z)
            {
                hit_list.push_back(m_kdtree[kdindex].ref->pidref);
            }
            return;
        }

        if (m_bbmax[axis] >= m_kdtree[kdindex].middle)
        {
            if (m_bbmin[axis] > m_kdtree[kdindex].max)
            {
                return;
            }

            int newaxis = axis + 1;

            if (newaxis >= 3)
            {
                newaxis = 0;
            }

            int newindex = kdindex + kdindex + 1;

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
