// This file is part of the Caelum project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution.

#include "CaelumPort/CaelumSystem.h"
#include "CaelumPort/Astronomy.h"
#include "CaelumPort/CaelumExceptions.h"
#include "CaelumPort/InternalUtilities.h"
#include <functional>

using namespace Ogre;

namespace CaelumPort
{
    const String CaelumSystem::DEFAULT_SKY_GRADIENTS_IMAGE = "EarthClearSky2.png";
    const String CaelumSystem::DEFAULT_SUN_COLOURS_IMAGE = "SunGradient.png";

    CaelumSystem::CaelumSystem
    (
        Ogre::Root *root,
        Ogre::SceneManager *sceneMgr,
                Ogre::SceneNode* groupingSceneNode
    ):
        mOgreRoot (root),
        mSceneMgr (sceneMgr),
        mCleanup (false)
    {
        LogManager::getSingleton().logMessage ("CaelumPort: Initialising Caelum system...");
        //LogManager::getSingleton().logMessage ("CaelumPort: CaelumSystem* at d" +
        //        StringConverter::toString (reinterpret_cast<uint>(this)));

        mCaelumportGroupingNode = groupingSceneNode;
        mCaelumCameraNode = groupingSceneNode->createChildSceneNode("CaelumCameraNode");
        mUniversalClock.reset(new UniversalClock ());

        // If the "Caelum" resource group does not exist; create it.
        // This resource group is never released; which may be bad.
        // What does ogre do for it's own runtime resources?
        Ogre::StringVector groups = ResourceGroupManager::getSingleton ().getResourceGroups ();
        if (std::find (groups.begin(), groups.end(), CaelumPort::RESOURCE_GROUP_NAME) == groups.end()) {
            LogManager::getSingleton ().logMessage (
                    "CaelumPort: Creating required internal resource group \'" + RESOURCE_GROUP_NAME + "\'");
            ResourceGroupManager::getSingleton ().createResourceGroup (CaelumPort::RESOURCE_GROUP_NAME);
        }

        // Autoconfigure. Calls clear first to set defaults.
        autoConfigure ();
    }

    void CaelumSystem::destroySubcomponents (bool destroyEverything)
    {
        // Destroy sub-components
        setSun (0);
        setPointStarfield (0);
        setMoon (0);
        mSkyGradientsImage.reset ();
        mSunColoursImage.reset ();

        // These things can't be rebuilt.
        if (destroyEverything) {
            LogManager::getSingleton ().logMessage("CaelumPort: Delete UniversalClock");
            mUniversalClock.reset ();
            mSceneMgr->destroySceneNode(mCaelumCameraNode);
            // RIGSOFRODS: Don't destroy the grouping node; it's owned by SkyX manager.
        }
    }

    CaelumSystem::~CaelumSystem () {
        destroySubcomponents (true);
        LogManager::getSingleton ().logMessage ("CaelumPort: CaelumSystem destroyed.");
    }

    void CaelumSystem::clear()
    {
        // Destroy all subcomponents first.
        destroySubcomponents (false);

        // Some "magical" behaviour.
        mAutoMoveCameraNode = true;
        mAutoNotifyCameraChanged = true;
        mAutoAttachViewportsToComponents = true;
        mAutoViewportBackground = true;

        // Default lookups.
        setSkyGradientsImage(DEFAULT_SKY_GRADIENTS_IMAGE);
        setSunColoursImage(DEFAULT_SUN_COLOURS_IMAGE);

        // Fog defaults.
        setManageSceneFog (Ogre::FOG_NONE);
        setManageSceneFogStart(900);
        setManageSceneFogEnd(1000);
        mGlobalFogDensityMultiplier = 1;
        mGlobalFogColourMultiplier = Ogre::ColourValue(1.0, 1.0, 1.0, 1.0);
        mSceneFogDensityMultiplier = 1;
        mSceneFogColourMultiplier = Ogre::ColourValue(0.7, 0.7, 0.7, 0.7);
        mGroundFogDensityMultiplier = 1;
        mGroundFogColourMultiplier = Ogre::ColourValue(1.0, 1.0, 1.0, 1.0);

        // Ambient lighting.
        setManageAmbientLight (true);
        setMinimumAmbientLight (Ogre::ColourValue (0.1, 0.1, 0.3));
        mEnsureSingleLightSource = false;
        mEnsureSingleShadowSource = false;

        // Observer time & position. J2000 is midday.
        mObserverLatitude = Ogre::Degree(45);
        mObserverLongitude = Ogre::Degree(0);
        mUniversalClock->setJulianDay (Astronomy::J2000);
    }

    void CaelumSystem::autoConfigure
    (
    )
    {
        // Clear everything; revert to default.
        clear();

        LogManager::getSingleton ().logMessage ("CaelumPort: Creating caelum sub-components.");

        // Init sun
        try {
            this->setSun (new SpriteSun (mSceneMgr, getCaelumCameraNode ()));
            this->getSun ()->setAmbientMultiplier (Ogre::ColourValue (0.5, 0.5, 0.5));
            this->getSun ()->setDiffuseMultiplier (Ogre::ColourValue (3, 3, 2.7));
            this->getSun ()->setSpecularMultiplier (Ogre::ColourValue (5, 5, 5));

            this->getSun ()->setAutoDisable (true);
            this->getSun ()->setAutoDisableThreshold (0.05);
        } catch (CaelumPort::UnsupportedException& ex) {
            LogManager::getSingleton ().logMessage (
                    "CaelumPort: Failed to initialize sun: " + ex.getDescription());
        }

        // Init moon
        try {
            this->setMoon (new Moon (mSceneMgr, getCaelumCameraNode ()));
            this->getMoon ()->setAutoDisable (true);
            this->getMoon ()->setAutoDisableThreshold (0.05);
        } catch (CaelumPort::UnsupportedException& ex) {
            LogManager::getSingleton ().logMessage (
                    "CaelumPort: Failed to initialize moon: " + ex.getDescription());
        }

        // Create point starfield
        try {
            this->setPointStarfield (new PointStarfield (mSceneMgr, getCaelumCameraNode ()));
        } catch (CaelumPort::UnsupportedException& ex) {
            LogManager::getSingleton ().logMessage (
                    "CaelumPort: Failed to initialize starfield: " + ex.getDescription());
        }

        LogManager::getSingleton ().logMessage ("CaelumPort: DONE initializing");
    }

    void CaelumSystem::shutdown (const bool cleanup) {
        LogManager::getSingleton ().logMessage ("CaelumPort: Shutting down Caelum system...");

        destroySubcomponents (true);

        if (cleanup) {
            mOgreRoot->removeFrameListener (this);
            delete this;
        } else {
            // We'll delete later. Make sure we're registered as a frame listener, or we'd leak.
            mOgreRoot->addFrameListener(this);
            mCleanup = true;
        }
    }

    void CaelumSystem::attachViewportImpl (Ogre::Viewport* vp)
    {
        
    }

    void CaelumSystem::detachViewportImpl (Ogre::Viewport* vp)
    {
        
    }

    void CaelumSystem::attachViewport (Ogre::Viewport* vp)
    {
        bool found = !mAttachedViewports.insert (vp).second;
        if (!found) {
            attachViewportImpl (vp);
        }
    }

    void CaelumSystem::detachViewport (Ogre::Viewport* vp)
    {
        std::set<Viewport*>::size_type erase_result = mAttachedViewports.erase(vp);
        assert(erase_result == 0 || erase_result == 1);
        bool found = erase_result == 1;
        if (found) {
            detachViewportImpl (vp);
        }
    }

    void CaelumSystem::detachAllViewports ()
    {
        std::set<Viewport*>::const_iterator it = mAttachedViewports.begin(), end = mAttachedViewports.end();
        for (; it != end; ++it) {
            detachViewportImpl (*it);
        }
        mAttachedViewports.clear();
    }

    bool CaelumSystem::isViewportAttached (Ogre::Viewport* vp) const {
        return mAttachedViewports.find (vp) != mAttachedViewports.end();
    }



    void CaelumSystem::setSun (BaseSkyLight* obj) {
        mSun.reset (obj);
    }

    void CaelumSystem::setMoon (Moon* obj) {
        mMoon.reset (obj);
    }


    void CaelumSystem::setPointStarfield (PointStarfield* obj)
    {
        mPointStarfield.reset(obj);
    }


    void CaelumSystem::preViewportUpdate (const Ogre::RenderTargetViewportEvent &e) {
        Ogre::Viewport *viewport = e.source;
        Ogre::Camera *camera = viewport->getCamera ();

        if (getAutoViewportBackground ()) {
            viewport->setBackgroundColour (Ogre::ColourValue::Black);
        }
        if (getAutoNotifyCameraChanged ()) {
            this->notifyCameraChanged (camera);
        }
    }

    void CaelumSystem::notifyCameraChanged(Ogre::Camera* cam)
    {
        // Move camera node.
        if (getAutoMoveCameraNode ()) {
            mCaelumCameraNode->setPosition (cam->getDerivedPosition());
            mCaelumCameraNode->_update (true, true);
        }



        if (getSun ()) {
            getSun ()->notifyCameraChanged (cam);
        }

        if (getMoon ()) {
            getMoon ()->notifyCameraChanged (cam);
        }



        if (getPointStarfield ()) {
            getPointStarfield ()->notifyCameraChanged (cam);
        }


    }

    bool CaelumSystem::frameStarted (const Ogre::FrameEvent &e) {
        if (mCleanup) {
            // Delayed destruction.
            mOgreRoot->removeFrameListener (this);
            delete this;
            return true;
        }

        updateSubcomponents(e.timeSinceLastFrame);

        return true;
    }

    void CaelumSystem::updateSubcomponents (Real timeSinceLastFrame)
    {
        /*
        LogManager::getSingleton().getDefaultLog()->logMessage(
                "CaelumSystem::updateSubcomponents: " +
                StringConverter::toString (timeSinceLastFrame, 10));
        */

        mUniversalClock->update (timeSinceLastFrame);

        // Timing variables
        LongReal julDay = mUniversalClock->getJulianDay ();
        LongReal relDayTime = fmod(julDay, 1);
        Real secondDiff = timeSinceLastFrame * mUniversalClock->getTimeScale ();

        // Get astronomical parameters.
        Ogre::Vector3 sunDir = getSunDirection(julDay);
        Ogre::Vector3 moonDir = getMoonDirection(julDay);
        Real moonPhase = getMoonPhase(julDay);

        // RIGSOFRODS: for updating SkyX subsystems via Controller interface.
        mCachedSunDir = sunDir;
        mCachedMoonDir = moonDir;
        mCachedMoonPhase = moonPhase;

        // Get parameters from sky colour model.
        Real fogDensity = getFogDensity (relDayTime, sunDir);
        Ogre::ColourValue fogColour = getFogColour (relDayTime, sunDir);
        Ogre::ColourValue sunLightColour = getSunLightColour (relDayTime, sunDir);
        Ogre::ColourValue sunSphereColour = getSunSphereColour (relDayTime, sunDir);
        Ogre::ColourValue moonLightColour = getMoonLightColour (moonDir);
        Ogre::ColourValue moonBodyColour = getMoonBodyColour (moonDir);

        fogDensity *= mGlobalFogDensityMultiplier;
        fogColour = fogColour * mGlobalFogColourMultiplier;



        // Update point starfield
        if (getPointStarfield ()) {
            getPointStarfield ()->setObserverLatitude (getObserverLatitude ());
            getPointStarfield ()->setObserverLongitude (getObserverLongitude ());
            getPointStarfield ()->update (julDay);
        }



        // Update scene fog.
        if (mManageSceneFogMode != Ogre::FOG_NONE) {
            mSceneMgr->setFog (mManageSceneFogMode,
                    fogColour * mSceneFogColourMultiplier,
                    fogDensity * mSceneFogDensityMultiplier,
                    mManageSceneFogFromDistance,
                    mManageSceneFogToDistance);
        }



        // Choose between sun and moon (should be done before updating)
        if (getSun() && getMoon ()) {
            Ogre::Real moonBrightness = moonLightColour.r + moonLightColour.g + moonLightColour.b + moonLightColour.a;
            Ogre::Real sunBrightness = sunLightColour.r + sunLightColour.g + sunLightColour.b + sunLightColour.a;
            bool sunBrighterThanMoon = (sunBrightness > moonBrightness);

            if (getEnsureSingleLightSource ()) {
                getMoon()->setForceDisable (sunBrighterThanMoon);
                getSun()->setForceDisable (!sunBrighterThanMoon);
            }
            if (getEnsureSingleShadowSource ()) {
                getMoon()->getMainLight ()->setCastShadows (!sunBrighterThanMoon);
                getSun()->getMainLight ()->setCastShadows (sunBrighterThanMoon);
            }
        }

        // Update sun
        if (getSun ()) {
            mSun->update (sunDir, sunLightColour, sunSphereColour);
        }

        // Update moon.
        if (getMoon ()) {
            mMoon->update (
                    moonDir,
                    moonLightColour,
                    moonBodyColour);
        }



        // Update ambient lighting.
        if (getManageAmbientLight ()) {
            Ogre::ColourValue ambient = Ogre::ColourValue::Black;
            if (getMoon ()) {
                ambient += getMoon ()->getLightColour () * getMoon ()->getAmbientMultiplier ();
            }
            if (getSun ()) {
                ambient += getSun ()->getLightColour () * getSun ()->getAmbientMultiplier ();
            }
            ambient.r = std::max(ambient.r, mMinimumAmbientLight.r);
            ambient.g = std::max(ambient.g, mMinimumAmbientLight.g);
            ambient.b = std::max(ambient.b, mMinimumAmbientLight.b);
            ambient.a = std::max(ambient.a, mMinimumAmbientLight.a);
            // Debug ambient factos (ick).
            /*
            LogManager::getSingleton().logMessage (
                        "Sun is " + StringConverter::toString(sunLightColour) + "\n"
                        "Moon is " + StringConverter::toString(moonLightColour) + "\n"
                        "Ambient is " + StringConverter::toString(ambient) + "\n"
                        );
             */
            mSceneMgr->setAmbientLight (ambient);
        }
    }

    void CaelumSystem::setManageSceneFog (Ogre::FogMode v) {
        mManageSceneFogMode = v;

        // Prevent having some stale values around.
        // also important: we need to initialize this before using any terrain
        mSceneMgr->setFog (mManageSceneFogMode);
    }

    void CaelumSystem::disableFogMangement()
    {
        // NO RESET
        mManageSceneFogMode = Ogre::FOG_NONE;
    }

    Ogre::FogMode CaelumSystem::getManageSceneFog () const {
        return mManageSceneFogMode;
    }

    void CaelumSystem::setManageSceneFogStart (Ogre::Real from) {
        mManageSceneFogFromDistance = from;
    }

    Ogre::Real CaelumSystem::getManageSceneFogStart () const {
        return mManageSceneFogFromDistance;
    }

    void CaelumSystem::setManageSceneFogEnd (Ogre::Real from) {
        mManageSceneFogToDistance = from;
    }

    Ogre::Real CaelumSystem::getManageSceneFogEnd () const {
        return mManageSceneFogToDistance;
    }

    void CaelumSystem::setSceneFogDensityMultiplier (Real value) {
        mSceneFogDensityMultiplier = value;
    }

    Real CaelumSystem::getSceneFogDensityMultiplier () const {
        return mSceneFogDensityMultiplier;
    }

    void CaelumSystem::setGroundFogDensityMultiplier (Real value) {
        mGroundFogDensityMultiplier = value;
    }

    Real CaelumSystem::getGroundFogDensityMultiplier () const {
        return mGroundFogDensityMultiplier;
    }

    void CaelumSystem::setGlobalFogDensityMultiplier (Real value) {
        mGlobalFogDensityMultiplier = value;
    }

    Real CaelumSystem::getGlobalFogDensityMultiplier () const {
        return mGlobalFogDensityMultiplier;
    }

    void CaelumSystem::setSkyGradientsImage (const Ogre::String &filename) {
        mSkyGradientsImage.reset(new Ogre::Image ());
        mSkyGradientsImage->load (filename, RESOURCE_GROUP_NAME);
    }

    void CaelumSystem::setSunColoursImage (const Ogre::String &filename) {
        mSunColoursImage.reset(new Ogre::Image ());
        mSunColoursImage->load (filename, RESOURCE_GROUP_NAME);
    }

    Ogre::ColourValue CaelumSystem::getFogColour (Real time, const Ogre::Vector3 &sunDir) {
        if (!mSkyGradientsImage.get()) {
            return Ogre::ColourValue::Black;
        }

        Real elevation = sunDir.dotProduct (Ogre::Vector3::UNIT_Y) * 0.5 + 0.5;
        Ogre::ColourValue col = InternalUtilities::getInterpolatedColour (elevation, 1, mSkyGradientsImage.get(), false);
        return col;
    }

    Real CaelumSystem::getFogDensity (Real time, const Ogre::Vector3 &sunDir)
    {
        if (!mSkyGradientsImage.get()) {
            return 0;
        }

        Real elevation = sunDir.dotProduct (Ogre::Vector3::UNIT_Y) * 0.5 + 0.5;
        Ogre::ColourValue col = InternalUtilities::getInterpolatedColour (elevation, 1, mSkyGradientsImage.get(), false);
        return col.a;
    }

    Ogre::ColourValue CaelumSystem::getSunSphereColour (Real time, const Ogre::Vector3 &sunDir)
    {
        if (!mSunColoursImage.get()) {
            return Ogre::ColourValue::White;
        }

        Real elevation = sunDir.dotProduct (Ogre::Vector3::UNIT_Y);
        elevation = elevation * 2 + 0.4;
        return InternalUtilities::getInterpolatedColour (elevation, 1, mSunColoursImage.get(), false);
    }

    Ogre::ColourValue CaelumSystem::getSunLightColour (Real time, const Ogre::Vector3 &sunDir)
    {
        if (!mSkyGradientsImage.get()) {
            exit(-1);
            return Ogre::ColourValue::White;
        }
        Real elevation = sunDir.dotProduct (Ogre::Vector3::UNIT_Y) * 0.5 + 0.5;

        // Hack: return averaged sky colours.
        // Don't use an alpha value for lights, this can cause nasty problems.
        Ogre::ColourValue col = InternalUtilities::getInterpolatedColour (elevation, elevation, mSkyGradientsImage.get(), false);
        Real val = (col.r + col.g + col.b) / 3;
        col = Ogre::ColourValue(val, val, val, 1.0);
        assert(Ogre::Math::RealEqual(col.a, 1));
        return col;
    }

    Ogre::ColourValue CaelumSystem::getMoonBodyColour (const Ogre::Vector3 &moonDir) {
        return Ogre::ColourValue::White;
    }

    Ogre::ColourValue CaelumSystem::getMoonLightColour (const Ogre::Vector3 &moonDir)
    {
        if (!mSkyGradientsImage.get()) {
            return Ogre::ColourValue::Blue;
        }
        // Scaled version of getSunLightColor
        Real elevation = moonDir.dotProduct (Ogre::Vector3::UNIT_Y) * 0.5 + 0.5;
        Ogre::ColourValue col = InternalUtilities::getInterpolatedColour (elevation, elevation, mSkyGradientsImage.get(), false);
        Real val = (col.r + col.g + col.b) / 3;
        col = Ogre::ColourValue(val / 2.5f, val / 2.5f, val / 2.5f, 1.0);
        assert(Ogre::Math::RealEqual(col.a, 1));
        return col;
    }

    const Ogre::Vector3 CaelumSystem::makeDirection (
            Ogre::Degree azimuth, Ogre::Degree altitude)
    {
        Ogre::Vector3 res;
        res.z = -Ogre::Math::Cos (azimuth) * Ogre::Math::Cos (altitude);  // North
        res.x =  Ogre::Math::Sin (azimuth) * Ogre::Math::Cos (altitude);  // East
        res.y = -Ogre::Math::Sin (altitude); // Zenith
        return res;
    }

    const Ogre::Vector3 CaelumSystem::getSunDirection (LongReal jday)
    {
        Ogre::Degree azimuth, altitude;
        {
            ScopedHighPrecissionFloatSwitch precissionSwitch;

            Astronomy::getHorizontalSunPosition(jday,
                    getObserverLongitude(), getObserverLatitude(),
                    azimuth, altitude);
        }
        Ogre::Vector3 res = makeDirection(azimuth, altitude);

        return res;
    }

    const Ogre::Vector3 CaelumSystem::getEclipticNorthPoleDirection (LongReal jday)
    {
        Ogre::Degree azimuth, altitude;
        {
            ScopedHighPrecissionFloatSwitch precissionSwitch;

            Astronomy::getHorizontalNorthEclipticPolePosition(jday,
                    getObserverLongitude (), getObserverLatitude (),
                    azimuth, altitude);
        }
        Ogre::Vector3 res = -makeDirection(azimuth, altitude);

        return res;
    }

    const Ogre::Vector3 CaelumSystem::getMoonDirection (LongReal jday)
    {
        Ogre::Degree azimuth, altitude;
        {
            ScopedHighPrecissionFloatSwitch precissionSwitch;

            Astronomy::getHorizontalMoonPosition(jday,
                    getObserverLongitude (), getObserverLatitude (),
                    azimuth, altitude);
        }
        Ogre::Vector3 res = makeDirection(azimuth, altitude);

        return res;
    }

    Ogre::Real CaelumSystem::getMoonPhase (LongReal jday)
    {
        // Calculates julian days since June 04, 1993 20:31 (full moon)
        // and divides by the time between lunations (synodic month)
        // Formula derived from http://scienceworld.wolfram.com/astronomy/Lunation.html
        LongReal T = (jday - 2449143.3552943347L) / 29.53058867L;

        T -= floor(T);  // [0..1)
        return T;
    }


}
