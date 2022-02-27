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

#include <OgreParticleAffectorFactory.h>
#include <OgreIteratorWrapper.h>

#include "ExtinguishableFireAffector.h"

#include <vector>

namespace Ogre {

/// @addtogroup Gfx
/// @{

/// @addtogroup Particle
/// @{

/** Factory class for DeflectorPlaneAffector. */
class ExtinguishableFireAffectorFactory : public ParticleAffectorFactory
{
    /** See ParticleAffectorFactory */
    String getName() const { return "ExtinguishableFire"; }

    /** See ParticleAffectorFactory */
    Ogre::ParticleAffector* createAffector(Ogre::ParticleSystem* psys)
    {
        Ogre::ParticleAffector* p = OGRE_NEW ExtinguishableFireAffector(psys);
        mAffectors.push_back(p);
        return p;
    }

public:

    typedef VectorIterator<std::vector<ParticleAffector*>> affectorIterator;

    /** Allow external access to the mFactories iterator */
    affectorIterator getAffectorIterator() { return affectorIterator(mAffectors.begin(), mAffectors.end()); }
};

/// @} // addtogroup Particle
/// @} // addtogroup Gfx

} // namespace Ogre
