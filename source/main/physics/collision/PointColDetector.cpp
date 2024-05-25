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

void IntraPointColDetector::UpdateIntraPoint(bool contactables)
{
    // Setup for self-collision detection
    // ---------------------------------

    int contacters_size = contactables ? m_actor->ar_num_contactable_nodes : m_actor->ar_num_contacters;

    if (contacters_size != m_object_list_size)
    {
        collision_partners.clear();
        collision_partners.push_back(ColActor(m_actor.GetRef()));
        m_object_list_size = contacters_size;
        update_structures_for_contacters(contactables);
    }
    else
    {
        refresh_node_positions();
    }
}

void InterPointColDetector::UpdateInterPoint(bool ignorestate)
{
    // Find collision partners (actors with collidable nodes)
    // ------------------------------------------------------

    int contacters_size = 0;
    std::vector<ColActor> new_collision_partners;
    for (ActorPtr& actor : App::GetGameContext()->GetActorManager()->GetActors())
    {
        if (actor != m_actor && (ignorestate || actor->ar_update_physics) &&
                m_actor->ar_bounding_box.intersects(actor->ar_bounding_box))
        {
            new_collision_partners.push_back(actor.GetRef());
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

    if (collision_partners != new_collision_partners || contacters_size != m_object_list_size)
    {
        collision_partners = new_collision_partners;
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
    for (ColActor& partner : collision_partners)
    {
        partner.point_pool_start = refi;
        Actor* actor = partner.actor;

        bool is_linked = std::find(m_actor->ar_linked_actors.begin(), m_actor->ar_linked_actors.end(), actor) != m_actor->ar_linked_actors.end();
        bool internal_collision = !ignoreinternal && ((actor == m_actor.GetRef()) || is_linked);
        for (int i = 0; i < actor->ar_num_nodes; i++)
        {
            if (actor->ar_nodes[i].nd_contacter || (!internal_collision && actor->ar_nodes[i].nd_contactable))
            {
                contactable_point_pool[refi].nodenum = static_cast<NodeNum_t>(i);
                contactable_point_pool[refi].nodepos = actor->ar_nodes[i].AbsPosition;
                refi++;
            }
        }
        partner.point_pool_count = refi - partner.point_pool_start;
    }
}

bool PointColDetector::QueryCollisionsWithAllPartners(const Vector3 &vec1, const Vector3 &vec2, const Vector3 &vec3, float enlargeBB)
{
    // Only check for presence of collision, do not record hits.
    // ---------------------------------------------------------
    
    Ogre::AxisAlignedBox collcab_box;
    collcab_box.merge(vec1);
    collcab_box.merge(vec2);
    collcab_box.merge(vec3);
    collcab_box.setMinimum(collcab_box.getMinimum() - enlargeBB);
    collcab_box.setMaximum(collcab_box.getMaximum() + enlargeBB);

    for (const ColActor& partner: collision_partners)
    {
        if (QueryCollisionsWithSinglePartner(collcab_box, partner))
        {
            hit_list.clear();
            return true;
        }
    }
    return false;
}

bool PointColDetector::QueryCollisionsWithSinglePartner(const Ogre::AxisAlignedBox collcab_aabb, const ColActor& partner)
{
    // Record all hits with the partner
    // -------------------------------

    ROR_ASSERT(partner);
    hit_list.clear();

    if (collcab_aabb.intersects(partner.actor->ar_bounding_box))
    {
        for (int i = 0; i < partner.point_pool_count; i++)
        {
            if (collcab_aabb.intersects(contactable_point_pool[partner.point_pool_start + i].nodepos))
            {
                hit_list.push_back(contactable_point_pool[partner.point_pool_start + i].nodenum);
            }
        }
    }
    return hit_list.size() > 0;
}

void PointColDetector::refresh_node_positions()
{
    // Because the `contactable_point_pool` contains cached node positions, we must update it on each tick.
    // ----------------------------------------------------------------------------------

    for (const ColActor& partner: collision_partners)
    {
        Actor* actor = partner.actor;
        for (int i = 0; i < partner.point_pool_count; i++)
        {
            ColPoint& point = contactable_point_pool[partner.point_pool_start + i];
            point.nodepos = actor->ar_nodes[point.nodenum].AbsPosition;
        }
    }
}
