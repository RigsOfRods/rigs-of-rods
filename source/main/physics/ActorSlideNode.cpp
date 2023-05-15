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

/// @file
/// @author Christopher Ritchey
/// @date 10/30/2009

#include "SlideNode.h"

#include "Actor.h"
#include "GameContext.h"

using namespace RoR;

// ug... BAD PERFORMNCE, BAD!!
void Actor::toggleSlideNodeLock()
{
    // for every slide node on this truck
    for (std::vector<SlideNode>::iterator itNode = m_slidenodes.begin(); itNode != m_slidenodes.end(); itNode++)
    {
        std::pair<RailGroup*, Ogre::Real> closest((RailGroup*)NULL, std::numeric_limits<Ogre::Real>::infinity());
        std::pair<RailGroup*, Ogre::Real> current((RailGroup*)NULL, std::numeric_limits<Ogre::Real>::infinity());

        // if neither foreign, nor self attach is set then we cannot change the
        // Rail attachments
        if (!itNode->sn_attach_self && !itNode->sn_attach_foreign)
        {
            continue;
        }

        if (m_slidenodes_locked)
        {
            itNode->AttachToRail(NULL);
            continue;
        }

        // check all the slide rail on all the other trucks :(
        for (ActorPtr& actor : App::GetGameContext()->GetActorManager()->GetActors())
        {
            // make sure this truck is allowed
            if ((this != actor.GetRef() && !itNode->sn_attach_foreign) || (this == actor.GetRef() && !itNode->sn_attach_self))
                continue;

            current = GetClosestRailOnActor(actor, (*itNode));
            if (current.second < closest.second)
                closest = current;
        } // this many

        itNode->AttachToRail(closest.first);
    } // nests

    m_slidenodes_locked = !m_slidenodes_locked;
} // is ugly....

std::pair<RailGroup*, Ogre::Real> Actor::GetClosestRailOnActor(ActorPtr actor, const SlideNode& node)
{
    std::pair<RailGroup*, Ogre::Real> closest((RailGroup*)NULL, std::numeric_limits<Ogre::Real>::infinity());

    RailGroupSegmentID_t segmentId = RAILGROUPSEGMENTID_INVALID;
    Ogre::Real segmentDist = std::numeric_limits<Ogre::Real>::infinity();

    for (RailGroup* railgroup: actor->m_railgroups)
    {
        // find the rail closest to the Node

        segmentId = railgroup->FindClosestSegment(node.GetSlideNodePosition());
        segmentDist = node.getLenTo(actor, railgroup->rg_segments[segmentId]);

        if (segmentDist < node.GetAttachmentDistance() && segmentDist < closest.second)
        {
            closest.first = railgroup;
            closest.second = segmentDist;
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
        it->ResetSlideNode();
    }
}

void Actor::updateSlideNodePositions()
{
    for (std::vector<SlideNode>::iterator it = m_slidenodes.begin(); it != m_slidenodes.end(); ++it)
    {
        it->UpdatePosition();
    }
}
