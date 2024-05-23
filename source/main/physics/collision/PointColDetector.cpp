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
    else
    {
        refresh_node_positions();
    }
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
    else
    {
        refresh_node_positions();
    }
}

void PointColDetector::update_structures_for_contacters(bool ignoreinternal)
{
    // Insert all contacters into the list of points to consider for collision.
    // ------------------------------------------------------------------------

    contactable_point_pool.resize(m_object_list_size);

    ColPointID_t refi = 0;
    for (ActorInstanceID_t actorid : m_collision_partners)
    {
        const ActorPtr& actor = App::GetGameContext()->GetActorManager()->GetActorById(actorid);

        bool is_linked = std::find(m_actor->ar_linked_actors.begin(), m_actor->ar_linked_actors.end(), actor) != m_actor->ar_linked_actors.end();
        bool internal_collision = !ignoreinternal && ((actorid == m_actor->ar_instance_id) || is_linked);
        for (int i = 0; i < actor->ar_num_nodes; i++)
        {
            if (actor->ar_nodes[i].nd_contacter || (!internal_collision && actor->ar_nodes[i].nd_contactable))
            {
                contactable_point_pool[refi].actorid = actor->ar_instance_id;
                contactable_point_pool[refi].nodenum = static_cast<NodeNum_t>(i);
                contactable_point_pool[refi].nodepos = actor->ar_nodes[i].AbsPosition;
                refi++;
            }
        }
    }
}

void PointColDetector::query(const Vector3 &vec1, const Vector3 &vec2, const Vector3 &vec3, float enlargeBB)
{
    // Create a bounding box of the collcab and do a brute force search
    // ----------------------------------------------------------------
    
    Ogre::AxisAlignedBox collcab_box;
    collcab_box.merge(vec1);
    collcab_box.merge(vec2);
    collcab_box.merge(vec3);
    collcab_box.setMinimum(collcab_box.getMinimum() - enlargeBB);
    collcab_box.setMaximum(collcab_box.getMaximum() + enlargeBB);

    hit_list.clear();
    for (ColPointID_t i = 0; i < m_object_list_size; i++)
    {
        if (collcab_box.intersects(contactable_point_pool[i].nodepos))
        {
            hit_list.push_back(i);
        }
    }
}

void PointColDetector::refresh_node_positions()
{
    // Because the `contactable_point_pool` contains cached node positions, we must update it on each tick.
    // ----------------------------------------------------------------------------------

    ActorInstanceID_t cur_actorid = ACTORINSTANCEID_INVALID;
    ActorPtr cur_actor = nullptr;

    for (ColPoint& pointid: contactable_point_pool)
    {
        if (pointid.actorid != cur_actorid)
        {
            cur_actorid = pointid.actorid;
            cur_actor = App::GetGameContext()->GetActorManager()->GetActorById(cur_actorid);
        }
        pointid.nodepos = cur_actor->ar_nodes[pointid.nodenum].AbsPosition;
    }
}
