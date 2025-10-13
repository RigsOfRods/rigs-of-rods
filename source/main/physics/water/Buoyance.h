/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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

#pragma once

#include "Application.h"

namespace RoR {

/// @addtogroup Physics
/// @{

struct BuoyCachedNode
{
    BuoyCachedNode(NodeNum_t _nodenum): nodenum(_nodenum) {}

    // node_t fields
    Ogre::Vector3 AbsPosition = Ogre::Vector3::ZERO;
    Ogre::Vector3 Velocity = Ogre::Vector3::ZERO;
    Ogre::Vector3 Forces = Ogre::Vector3::ZERO;
    // additional fields
    NodeNum_t nodenum = NODENUM_INVALID;
};

class Buoyance
{
public:

    Buoyance(DustPool* splash, DustPool* ripple);
    ~Buoyance();

    void computeNodeForce(BuoyCachedNode *a, BuoyCachedNode *b, BuoyCachedNode *c, bool doUpdate, int type);

    enum { BUOY_NORMAL, BUOY_DRAGONLY, BUOY_DRAGLESS };

    bool sink;

    /// try adding the node to internal list (each node is only listed once).
    /// @return new or existing cached node ID.
    BuoyCachedNodeID_t cacheBuoycabNode(node_t* n);

    std::vector<BuoyCachedNode> buoy_cached_nodes;

private:

    //compute tetrahedron volume
    inline float computeVolume(Ogre::Vector3 o, Ogre::Vector3 a, Ogre::Vector3 b, Ogre::Vector3 c);

    //compute pressure and drag force on a submerged triangle
    Ogre::Vector3 computePressureForceSub(Ogre::Vector3 a, Ogre::Vector3 b, Ogre::Vector3 c, Ogre::Vector3 vel, int type);
    
    //compute pressure and drag forces on a random triangle
    Ogre::Vector3 computePressureForce(Ogre::Vector3 a, Ogre::Vector3 b, Ogre::Vector3 c, Ogre::Vector3 vel, int type);
    
    DustPool *splashp, *ripplep;
    bool update;

    
};

/// @} // addtogroup Physics

} // namespace RoRs

