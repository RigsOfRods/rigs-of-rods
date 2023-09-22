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
class ExtinguishableFireAffector : public ParticleAffector
{
public:
    /** Command object for middle point (see ParamCommand).*/
    class CmdMiddlePoint : public ParamCommand
    {
    public:
        String doGet(const void* target) const;
        void doSet(void* target, const String& val);
    };

    /** Command object for intensity (see ParamCommand).*/
    class CmdIntensity : public ParamCommand
    {
    public:
        String doGet(const void* target) const;
        void doSet(void* target, const String& val);
    };

    /** Command object for maximum intensity (see ParamCommand).*/
    class CmdMaxIntensity : public ParamCommand
    {
    public:
        String doGet(const void* target) const;
        void doSet(void* target, const String& val);
    };

    /** Command object for intensity growth (see ParamCommand).*/
    class CmdIntensityGrowth : public ParamCommand
    {
    public:
        String doGet(const void* target) const;
        void doSet(void* target, const String& val);
    };

    /** Command object for radius (see ParamCommand).*/
    class CmdRadius : public ParamCommand
    {
    public:
        String doGet(const void* target) const;
        void doSet(void* target, const String& val);
    };

    /// Default constructor
    ExtinguishableFireAffector(ParticleSystem* psys);

    /// Destructor
    ~ExtinguishableFireAffector();

    /** See ParticleAffector. */
    void _affectParticles(ParticleSystem* pSystem, Real timeElapsed);

    /** Sets the sphere middle point. */
    void setMiddlePoint(const Vector3& pos);

    /** Gets the sphere middle point. */
    Vector3 getMiddlePoint(void) const;

    /** Sets the intensity. */
    void setIntensity(Real intensity);

    /** Gets the intensity */
    Real getIntensity(void) const;

    /** Sets the maximum intensity. */
    void setMaxIntensity(Real intensity);

    /** Gets the maximum intensity */
    Real getMaxIntensity(void) const;

    /** Sets the intensity growth rate. */
    void setIntensityGrowth(Real intensity);

    /** Gets the intensity growth rate*/
    Real getIntensityGrowth(void) const;

    /** Sets the radius for the interaction with the FireExtiniguisher affector. */
    void setRadius(Real radius);

    /** Gets the radius for the interaction with the FireExtiniguisher affector. */
    Real getRadius(void) const;

    bool isTemplate() const;
    Vector3 getAbsoluteMiddlePoint(void) const;
    Real reduceIntensity(Real amount);
    ParticleSystem* getParticleSystem() { return mPsys; };

    /** Sets the instance name of the parent object for this particle system */
    void setInstanceName(String iname) { objectInstanceName = iname; };
    String getInstanceName() { return objectInstanceName; };

    /// Command objects
    static CmdMiddlePoint msMiddlePointCmd;
    static CmdIntensity msIntensityCmd;
    static CmdMaxIntensity msMaxIntensityCmd;
    static CmdIntensityGrowth msIntensityGrowthCmd;
    static CmdRadius msRadiusCmd;

protected:
    /// sphere middle point
    Vector3 mMiddlePoint;

    /// radius of the sphere
    Real mRadius;

    /// Intensity of the fire
    Real mIntensity;

    ParticleSystem* mPsys;

    Real originalIntensity;
    bool updateIntensityRequired;
    Vector2 originalDimensions;
    bool firstFrame;
    Real mIntensityGrowth;
    Real mMaxIntensity;

    /// The instance name of the parent object
    String objectInstanceName;
};

} // namespace Ogre

#endif // USE_ANGELSCRIPT
