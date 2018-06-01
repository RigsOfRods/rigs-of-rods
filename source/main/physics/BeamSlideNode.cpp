/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2009      Christopher Ritchey

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

/// @file BeamSlideNode.cpp
/// @author Christopher Ritchey
/// @date 10/30/2009

#include "SlideNode.h"

#include "Application.h"
#include "Beam.h"
#include "BeamFactory.h"
#include "RoRFrameListener.h"

// ug... BAD PERFORMNCE, BAD!!
void Actor::ToggleSlideNodeLock()
{
    unsigned int num_slots = static_cast<unsigned int>(RoR::App::GetSimController()->GetBeamFactory()->GetNumUsedActorSlots());
    int player_actor_id = RoR::App::GetSimController()->GetBeamFactory()->GetPlayerActorId();

    // for every slide node on this truck
    for (std::vector<SlideNode>::iterator itNode = m_slidenodes.begin(); itNode != m_slidenodes.end(); itNode++)
    {
        std::pair<RailGroup*, Ogre::Real> closest((RailGroup*)NULL, std::numeric_limits<Ogre::Real>::infinity());
        std::pair<RailGroup*, Ogre::Real> current((RailGroup*)NULL, std::numeric_limits<Ogre::Real>::infinity());

        // if neither foreign, nor self attach is set then we cannot change the
        // Rail attachments
        if (!itNode->getAttachRule(ATTACH_ALL))
            continue;
        if (m_slidenodes_locked)
        {
            itNode->attachToRail(NULL);
            continue;
        }

        // check all the slide rail on all the other trucks :(
        for (unsigned int i = 0; i < num_slots; ++i)
        {
            // make sure this truck is allowed
            if (!((player_actor_id != i && itNode->getAttachRule(ATTACH_FOREIGN)) ||
                (player_actor_id == i && itNode->getAttachRule(ATTACH_SELF))))
                continue;

            current = GetClosestRailOnActor(RoR::App::GetSimController()->GetActorById(i), (*itNode));
            if (current.second < closest.second)
                closest = current;
        } // this many

        itNode->attachToRail(closest.first);
    } // nests

    m_slidenodes_locked = !m_slidenodes_locked;
} // is ugly....

std::pair<RailGroup*, Ogre::Real> Actor::GetClosestRailOnActor(Actor* actor, const SlideNode& node)
{
    std::pair<RailGroup*, Ogre::Real> closest((RailGroup*)NULL, std::numeric_limits<Ogre::Real>::infinity());
    Rail* curRail = NULL;
    Ogre::Real lenToCurRail = std::numeric_limits<Ogre::Real>::infinity();

    for (std::vector<RailGroup*>::iterator itGroup = actor->m_railgroups.begin();
         itGroup != actor->m_railgroups.end();
         itGroup++)
    {
        // find the rail closest to the Node
        curRail = SlideNode::getClosestRailAll((*itGroup), node.getNodePosition());
        lenToCurRail = node.getLenTo(curRail);
#if 0
        // if this rail closer than the previous one keep, also check that this
        // slide node is not already attached to this rail, now how to do that
        // without looping through the entire slide node array AGAIN...
        // Only for use with a single slide node attaching to multiple rails,
        // which currently is not implemented
        // oh well git'r done... :P
        for (std::vector< SlideNode >::iterator itNode = m_slidenodes.begin(); itNode != m_slidenodes.end(); itNode++)
        {
        // check if node is already hooked up to
            if ( node.getNodeID() == itNode->getNodeID() &&
                node.getRailID() == (*itGroup)->getID() )
                continue;
        }
#endif
        if (lenToCurRail < node.getAttachmentDistance() && lenToCurRail < closest.second)
        {
            closest.first = (*itGroup);
            closest.second = lenToCurRail;
        }
    }

    return closest;
}

// SlideNode Utility functions /////////////////////////////////////////////////

void Actor::updateSlideNodeForces(const Ogre::Real dt)
{
    for (std::vector<SlideNode>::iterator it = m_slidenodes.begin(); it != m_slidenodes.end(); ++it)
    {
        it->UpdatePosition();
        it->UpdateForces(dt);
    }
}

void Actor::resetSlideNodePositions()
{
    if (m_slidenodes.empty())
        return;
    for (std::vector<SlideNode>::iterator it = m_slidenodes.begin(); it != m_slidenodes.end(); ++it)
    {
        it->ResetPositions();
    }
}

void Actor::resetSlideNodes()
{
    for (std::vector<SlideNode>::iterator it = m_slidenodes.begin(); it != m_slidenodes.end(); ++it)
    {
        it->reset();
    }
}

void Actor::updateSlideNodePositions()
{
    for (std::vector<SlideNode>::iterator it = m_slidenodes.begin(); it != m_slidenodes.end(); ++it)
    {
        it->UpdatePosition();
    }
}
