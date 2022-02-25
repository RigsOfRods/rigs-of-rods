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

#ifdef USE_ANGELSCRIPT

#include <OgreParticleAffector.h>
#include <Ogre.h>

#include "ExtinguishableFireAffectorFactory.h"

namespace Ogre {

/// @addtogroup Gfx
/// @{

/// @addtogroup Particle
/// @{

/** This class defines a ParticleAffector which deflects particles.
@remarks
    This affector (see ParticleAffector) offers a simple (and inaccurate) physical deflection.
    All particles which hit the plane are reflected.
@par
    The plane is defined by a point (plane_point) and the normal (plane_normal).
    In addition it is possible to change the strenght of the recoil by using the bounce parameter.
*/
class FireExtinguisherAffector : public ParticleAffector
{
public:
    /** Command object for bounce (see ParamCommand).*/
    class CmdEffectiveness : public ParamCommand
    {
    public:
        String doGet(const void* target) const;
        void doSet(void* target, const String& val);
    };

    /// Default constructor
    FireExtinguisherAffector(ParticleSystem* psys);

    /** See ParticleAffector. */
    void _affectParticles(ParticleSystem* pSystem, Real timeElapsed);

    /** Sets the bounce value of the deflection. */
    void setEffectiveness(Real effectiveness);

    /** Gets the bounce value of the deflection. */
    Real getEffectiveness(void) const;

    /// Command objects
    static CmdEffectiveness msEffectivenessCmd;

protected:
    // a reference to the extinguishable fire
    ExtinguishableFireAffectorFactory* mEfaf;

    /// effectiveness factor (1 means as effective as water, everything higher is more effective while anything lower is less effective)
    Real mEffectiveness;
};

/// @} // addtogroup Particle
/// @} // addtogroup Gfx

} // namespace Ogre

#endif //USE_ANGELSCRIPT
