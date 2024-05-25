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
    collision_partners.clear();
    this->AddCollisionPartner(m_actor.GetRef(), contacters_size, contactables);
}

void InterPointColDetector::UpdateInterPoint(bool ignorestate)
{
    // Find collision partners (actors with collidable nodes)
    // ------------------------------------------------------

    int total_contacters_size = 0;
    collision_partners.clear();
    for (ActorPtr& actor : App::GetGameContext()->GetActorManager()->GetActors())
    {
        if (actor != m_actor && (ignorestate || actor->ar_update_physics) &&
                m_actor->ar_bounding_box.intersects(actor->ar_bounding_box))
        {
            bool is_linked = std::find(m_actor->ar_linked_actors.begin(), m_actor->ar_linked_actors.end(), actor) != m_actor->ar_linked_actors.end();
            int contacters_size = is_linked ? actor->ar_num_contacters : actor->ar_num_contactable_nodes;
            this->AddCollisionPartner(actor.GetRef(), contacters_size, false);
            total_contacters_size += contacters_size;
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

    m_actor->ar_collision_relevant = (total_contacters_size > 0);
}

void PointColDetector::AddCollisionPartner(Actor* actor, int contacters_size, bool ignoreinternal)
{
    // Partner = actor whose nodes will collide with `m_actor`'s collcabs
    // ------------------------------------------------------------------

    ColActor partner(actor);
    bool is_linked = std::find(m_actor->ar_linked_actors.begin(), m_actor->ar_linked_actors.end(), partner.actor) != m_actor->ar_linked_actors.end();
    partner.internal_collision = !ignoreinternal && ((partner.actor == m_actor.GetRef()) || is_linked);
    partner.contacters_size = contacters_size;
    collision_partners.push_back(partner);
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
        for (int i = 0; i < partner.actor->ar_num_nodes; i++)
        {
            const node_t& node = partner.actor->ar_nodes[i];
            if (node.nd_contacter || (!partner.internal_collision && node.nd_contactable))
            {
                if (collcab_aabb.intersects(node.AbsPosition))
                {
                    hit_list.push_back((NodeNum_t)i);
                }
            }
        }
    }
    return hit_list.size() > 0;
}

