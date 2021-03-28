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

class Buoyance
{
public:

    Buoyance(DustPool* splash, DustPool* ripple);
    ~Buoyance();

    void computeNodeForce(node_t *a, node_t *b, node_t *c, bool doUpdate, int type);

    enum BuoyType { BUOY_NORMAL, BUOY_DRAGONLY, BUOY_DRAGLESS };

    bool sink;

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

} // namespace RoRs

