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

#ifdef USE_ANGELSCRIPT

#include "FireExtinguisherAffector.h"

#include "Application.h"

#include <OgreParticleSystem.h>
#include <OgreParticle.h>
#include <OgreStringConverter.h>
#include <OgreParticleSystemManager.h>
#include <OgreParticle.h>

#include "RoRPrerequisites.h"

#include "ExtinguishableFireAffectorFactory.h"


namespace Ogre {

// Instantiate statics
FireExtinguisherAffector::CmdEffectiveness FireExtinguisherAffector::msEffectivenessCmd;

//-----------------------------------------------------------------------
FireExtinguisherAffector::FireExtinguisherAffector(ParticleSystem* psys)
    : ParticleAffector(psys)
{
    mType = "FireExtinguisher";

    // defaults
    mEffectiveness = 1.0;
    mEfaf = 0;

    // Set up parameters
    if (createParamDictionary("FireExtinguisherAffector"))
    {
        addBaseParameters();
        // Add extra paramaters
        ParamDictionary* dict = getParamDictionary();
        dict->addParameter(ParameterDef("effectiveness",
            "How effective is your fire extinguisher? Anything higher than one is more effective than water while everything lower than one is less effective than water.",
            PT_REAL), &msEffectivenessCmd);
    }

    // get fire affector factory
    ParticleSystemManager::ParticleAffectorFactoryIterator pafi = ParticleSystemManager::getSingleton().getAffectorFactoryIterator();
    ParticleAffectorFactory *paf;

    while(pafi.hasMoreElements())
    {
        paf = pafi.getNext();
        if ( paf->getName() == "ExtinguishableFire" )
        {
            mEfaf = (ExtinguishableFireAffectorFactory *)paf;
            break;
        }	
    }
    if (!mEfaf) LOG("ERROR: Couldn't find an ExtinguishableFireAffectorFactory instance. Was it registered in the content manager?");
}
//-----------------------------------------------------------------------
void FireExtinguisherAffector::_affectParticles(ParticleSystem* pSystem, Real timeElapsed)
{
    ExtinguishableFireAffectorFactory::affectorIterator affIt = mEfaf->getAffectorIterator();
    ExtinguishableFireAffector* fire;

    while(affIt.hasMoreElements())
    {
        fire = (ExtinguishableFireAffector*)affIt.getNext();

        if (fire->isTemplate())
            continue;

        Real squaredRadius = Math::Pow(fire->getRadius(), 2);
        Vector3 middlePoint = fire->getAbsoluteMiddlePoint();

        ParticleIterator pi = pSystem->_getIterator();
        Particle *p;
        int fireHits = 0;
        while (!pi.end())
        {
            p = pi.getNext();

            if ( middlePoint.squaredDistance(p->mPosition) < squaredRadius )
            {
                // This particle is inside the fire, dispose of it in the next update
                p->mTimeToLive = 0;
                ++fireHits;
            }
        }
        if (fireHits>0)
        {
            Real intensity = fire->reduceIntensity(fireHits*mEffectiveness);
            if (intensity<0) delete fire->getParticleSystem();
        }
    }
}
//-----------------------------------------------------------------------
void FireExtinguisherAffector::setEffectiveness(Real effectiveness)
{
    mEffectiveness = effectiveness;
}
//-----------------------------------------------------------------------
Real FireExtinguisherAffector::getEffectiveness(void) const
{
    return mEffectiveness;
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
// Command objects
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
String FireExtinguisherAffector::CmdEffectiveness::doGet(const void* target) const
{
    return StringConverter::toString(
        static_cast<const FireExtinguisherAffector*>(target)->getEffectiveness() );

}
void FireExtinguisherAffector::CmdEffectiveness::doSet(void* target, const String& val)
{
    static_cast<FireExtinguisherAffector*>(target)->setEffectiveness(
        StringConverter::parseReal(val));
}

} // namespace Ogre


#endif //USE_ANGELSCRIPT

