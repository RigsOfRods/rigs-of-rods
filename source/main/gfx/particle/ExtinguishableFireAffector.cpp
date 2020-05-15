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

#include "ExtinguishableFireAffector.h"

#include "Application.h"
#include "ScriptEngine.h"

#include <OgreParticleSystem.h>
#include <OgreParticle.h>
#include <OgreStringConverter.h>
#include <OgreSceneNode.h>

using namespace RoR;

namespace Ogre {

// Instantiate statics
ExtinguishableFireAffector::CmdMiddlePoint ExtinguishableFireAffector::msMiddlePointCmd;
ExtinguishableFireAffector::CmdIntensity ExtinguishableFireAffector::msIntensityCmd;
ExtinguishableFireAffector::CmdMaxIntensity ExtinguishableFireAffector::msMaxIntensityCmd;
ExtinguishableFireAffector::CmdIntensityGrowth ExtinguishableFireAffector::msIntensityGrowthCmd;
ExtinguishableFireAffector::CmdRadius ExtinguishableFireAffector::msRadiusCmd;

//-----------------------------------------------------------------------
ExtinguishableFireAffector::ExtinguishableFireAffector(ParticleSystem* psys)
    : ParticleAffector(psys), mPsys(psys)
{
    mType = "ExtinguishableFire";

    // defaults
    mMiddlePoint = Vector3::ZERO;
    mRadius = 1.0;
    mIntensity = 3000.0;
    originalIntensity = mIntensity;
    mIntensityGrowth = 10;
    mMaxIntensity = 4000.0;
    updateIntensityRequired = false;
    firstFrame = true;
        
    // Set up parameters
    if (createParamDictionary("ExtinguishableFireAffector"))
    {
        addBaseParameters();
        // Add extra paramaters
        ParamDictionary* dict = getParamDictionary();
        dict->addParameter(ParameterDef("middle_point",
            "The middle point of the sphere on which water particles will interact.",
            PT_VECTOR3), &msMiddlePointCmd);
        dict->addParameter(ParameterDef("intensity",
            "The amount of water particles that a fire can withstand before being extinguished.",
            PT_REAL), &msIntensityCmd);
        dict->addParameter(ParameterDef("max_intensity",
            "The maximum intensity the fire can grow to.",
            PT_REAL), &msMaxIntensityCmd);
        dict->addParameter(ParameterDef("intensity_growth",
            "The amount by which the intensity of the fire grows per second.",
            PT_REAL), &msMaxIntensityCmd);
        dict->addParameter(ParameterDef("radius",
            "The radius of the sphere.",
            PT_REAL), &msRadiusCmd);
    }

    // predefine objectInstanceName
    objectInstanceName = "unknown";

    App::GetScriptEngine()->fireEvent(objectInstanceName, mIntensity);
}
//-----------------------------------------------------------------------
ExtinguishableFireAffector::~ExtinguishableFireAffector()
{
    App::GetScriptEngine()->fireEvent(objectInstanceName, mIntensity);
}
//-----------------------------------------------------------------------
void ExtinguishableFireAffector::_affectParticles(ParticleSystem* pSystem, Real timeElapsed)
{
    if (firstFrame)
    {
        originalDimensions = Vector2(pSystem->getDefaultWidth(), pSystem->getDefaultHeight());
        originalIntensity = mIntensity;
        firstFrame = false;
    }

    if (mIntensity < 0)
    {
        // The fire is extinguished (normally, this particleSystem should get deleted by the fire extinguisher, before this can happen)
        mPsys->removeAllEmitters();
    }
    else
    {
        if (!updateIntensityRequired && mIntensity<mMaxIntensity) {
            // No water is hitting the fire at the moment, so let's increase the fire intensity a little bit
            mIntensity += mIntensityGrowth*timeElapsed;
            updateIntensityRequired = true;
        }

        if (updateIntensityRequired)
        {
            // update the fire
            mPsys->setDefaultDimensions(originalDimensions.x*mIntensity/originalIntensity, originalDimensions.y*mIntensity/originalIntensity);
            updateIntensityRequired = false;
            App::GetScriptEngine()->fireEvent(objectInstanceName, mIntensity);
        }
    }
}
//-----------------------------------------------------------------------
void ExtinguishableFireAffector::setMiddlePoint(const Vector3& pos)
{
    mMiddlePoint = pos;
}
//-----------------------------------------------------------------------
void ExtinguishableFireAffector::setIntensity(Real intensity)
{
    mIntensity = intensity;
}
//-----------------------------------------------------------------------
void ExtinguishableFireAffector::setMaxIntensity(Real intensity)
{
    mMaxIntensity = intensity;
}
//-----------------------------------------------------------------------
void ExtinguishableFireAffector::setIntensityGrowth(Real intensity)
{
    mIntensityGrowth = intensity;
}
//-----------------------------------------------------------------------
void ExtinguishableFireAffector::setRadius(Real radius)
{
    mRadius = radius;
}

//-----------------------------------------------------------------------
Vector3 ExtinguishableFireAffector::getMiddlePoint(void) const
{
    return mMiddlePoint;		
}
//-----------------------------------------------------------------------
Vector3 ExtinguishableFireAffector::getAbsoluteMiddlePoint(void) const
{
    Node* node = mPsys->getParentNode();
    if (!node) return mMiddlePoint;
    return node->convertLocalToWorldPosition(mMiddlePoint);;
}
//-----------------------------------------------------------------------
Real ExtinguishableFireAffector::getIntensity(void) const
{
    return mIntensity;
}
//-----------------------------------------------------------------------
Real ExtinguishableFireAffector::getMaxIntensity(void) const
{
    return mMaxIntensity;
}
//-----------------------------------------------------------------------
Real ExtinguishableFireAffector::getIntensityGrowth(void) const
{
    return mIntensityGrowth;
}
//-----------------------------------------------------------------------
Real ExtinguishableFireAffector::getRadius(void) const
{
    return mRadius;
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
// Command objects
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
String ExtinguishableFireAffector::CmdMiddlePoint::doGet(const void* target) const
{
    return StringConverter::toString(
        static_cast<const ExtinguishableFireAffector*>(target)->getMiddlePoint() );
}
void ExtinguishableFireAffector::CmdMiddlePoint::doSet(void* target, const String& val)
{
    static_cast<ExtinguishableFireAffector*>(target)->setMiddlePoint(
        StringConverter::parseVector3(val));
}
//-----------------------------------------------------------------------
String ExtinguishableFireAffector::CmdIntensity::doGet(const void* target) const
{
    return StringConverter::toString(
        static_cast<const ExtinguishableFireAffector*>(target)->getIntensity() );

}
void ExtinguishableFireAffector::CmdIntensity::doSet(void* target, const String& val)
{
    static_cast<ExtinguishableFireAffector*>(target)->setIntensity(
        StringConverter::parseReal(val));
}
//-----------------------------------------------------------------------
String ExtinguishableFireAffector::CmdMaxIntensity::doGet(const void* target) const
{
    return StringConverter::toString(
        static_cast<const ExtinguishableFireAffector*>(target)->getMaxIntensity() );

}
void ExtinguishableFireAffector::CmdMaxIntensity::doSet(void* target, const String& val)
{
    static_cast<ExtinguishableFireAffector*>(target)->setMaxIntensity(
        StringConverter::parseReal(val));
}
//-----------------------------------------------------------------------
String ExtinguishableFireAffector::CmdIntensityGrowth::doGet(const void* target) const
{
    return StringConverter::toString(
        static_cast<const ExtinguishableFireAffector*>(target)->getIntensityGrowth() );

}
void ExtinguishableFireAffector::CmdIntensityGrowth::doSet(void* target, const String& val)
{
    static_cast<ExtinguishableFireAffector*>(target)->setIntensityGrowth(
        StringConverter::parseReal(val));
}
//-----------------------------------------------------------------------
String ExtinguishableFireAffector::CmdRadius::doGet(const void* target) const
{
    return StringConverter::toString(
        static_cast<const ExtinguishableFireAffector*>(target)->getRadius() );

}
void ExtinguishableFireAffector::CmdRadius::doSet(void* target, const String& val)
{
    static_cast<ExtinguishableFireAffector*>(target)->setRadius(
        StringConverter::parseReal(val));
}
//------------------------------------------------------------------------
bool ExtinguishableFireAffector::isTemplate() const
{
    return !(mPsys->getParentNode());
}
Real ExtinguishableFireAffector::reduceIntensity(Real amount)
{
    mIntensity -= amount;
    updateIntensityRequired = true;
    return mIntensity;
}

}

#endif // USE_ANGELSCRIPT
