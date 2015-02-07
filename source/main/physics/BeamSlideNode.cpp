/**
 @file BeamSlideNode.cpp
 @author Christopher Ritchey
 @date 10/30/2009

This source file is part of Rigs of Rods
Copyright 2009 Christopher Ritchey

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
#include "SlideNode.h"

#include "Beam.h"
#include "BeamFactory.h"

// ug... BAD PERFORMNCE, BAD!!
void Beam::toggleSlideNodeLock()
{
	int trucksnum = BeamFactory::getSingleton().getTruckCount();
	int curTruck = BeamFactory::getSingleton().getCurrentTruckNumber();

	// for every slide node on this truck
	for (SlideNode* node : mSlideNodes)
	{
		std::pair<RailGroup*, Ogre::Real> closest((RailGroup*)nullptr, std::numeric_limits<Ogre::Real>::infinity());
		std::pair<RailGroup*, Ogre::Real> current((RailGroup*)nullptr, std::numeric_limits<Ogre::Real>::infinity());
		
		// if neither foreign, nor self attach is set then we cannot change the
		// Rail attachments
		if ( !node->getAttachRule( ATTACH_ALL ) ) continue;
		if ( SlideNodesLocked )
		{
			node->attachToRail( nullptr );
			continue;
		}
		
		// check all the slide rail on all the other trucks :(
		for ( unsigned int i = 0; i < (unsigned int) trucksnum; ++i)
		{
			// make sure this truck is allowed
			if (!((curTruck != i && node->getAttachRule(ATTACH_FOREIGN) ) ||
				  (curTruck == i && node->getAttachRule(ATTACH_SELF) ) ) )
				continue;
			
			current = getClosestRailOnTruck( BeamFactory::getSingleton().getTruck(i), *node );
			if ( current.second < closest.second ) closest = current;
			
		} // this many
		
		node->attachToRail(closest.first);
	} // nests
	
	SlideNodesLocked = !SlideNodesLocked;
} // is ugly....

std::pair<RailGroup*, Ogre::Real> Beam::getClosestRailOnTruck( Beam* truck, const SlideNode& node)
{
	std::pair<RailGroup*, Ogre::Real> closest((RailGroup*)nullptr, std::numeric_limits<Ogre::Real>::infinity());
	Rail* curRail = nullptr;
	Ogre::Real lenToCurRail = std::numeric_limits<Ogre::Real>::infinity();

	for (RailGroup* group : truck->mRailGroups)
	{
		// find the rail closest to the Node
		curRail = SlideNode::getClosestRailAll( group, node.getNodePosition() );
		lenToCurRail = node.getLenTo( curRail );
#if 0
		// if this rail closer than the previous one keep, also check that this
		// slide node is not already attached to this rail, now how to do that
		// without looping through the entire slide node array AGAIN...
		// Only for use with a single slide node attaching to multiple rails,
		// which currently is not implemented
		// oh well git'r done... :P
		for (auto& slideNode : mSlideNodes)
		{
			// check if node is already hooked up to
			if ( node.getNodeID() == slideNode.getNodeID() &&
				node.getRailID() == group->getID() )
				continue;
		}
#endif
		if ( lenToCurRail < node.getAttachmentDistance() && lenToCurRail < closest.second )
		{
			closest.first = group;
			closest.second = lenToCurRail;
		}
	}
	
	return closest;
}

// SlideNode Utility functions /////////////////////////////////////////////////
void Beam::resetSlideNodePositions()
{
	for (SlideNode* slideNode : mSlideNodes)
	{
		slideNode->ResetPositions();
	}	
}

void Beam::resetSlideNodes()
{
	for (SlideNode* slideNode : mSlideNodes)
	{
		slideNode->reset();
	}
}

void Beam::updateSlideNodePositions()
{
	for (SlideNode* slideNode : mSlideNodes)
	{
		slideNode->UpdatePosition();
	}
}
