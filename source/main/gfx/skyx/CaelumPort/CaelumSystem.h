// This file is part of the Caelum project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution.

#ifndef CAELUMPORT__CAELUM_SYSTEM_H
#define CAELUMPORT__CAELUM_SYSTEM_H

#include "CaelumPort/CaelumPrerequisites.h"
#include "CaelumPort/UniversalClock.h"
#include "CaelumPort/PointStarfield.h"
#include "CaelumPort/SkyLight.h"
#include "CaelumPort/Sun.h"
#include "CaelumPort/Moon.h"
#include "skyx/Controller.h"

namespace CaelumPort
{
    /** RIGSOFRODS: Trimmed down version of CaelumSystem; Features:
    * - Astronomical calculations using julian calendar
    * - Directional and ambient scene lighting (CAELUM_COMPONENT_SUN/MOON)
    * - Point starfield rendering (CAELUM_COMPONENT_POINT_STARFIELD)
    * - NOTE: Precipitation was ported separately earlier, see '../Precipitation'
    * Code changes:
    * - namespace changed to CaelumPort
    * - Added proper grouping scene node
    * - Ad-hoc changes are marked with 'RIGSOFRODS:' comments
     */
    class  CaelumSystem:
            public Ogre::FrameListener,
            public Ogre::RenderTargetListener,
            public SkyX::Controller
    {
    private:
        /// Root of the Ogre engine.
        Ogre::Root *mOgreRoot;

        /// Scene manager.
        Ogre::SceneManager *mSceneMgr;

        /// Caelum scene node for camera-bound elements (most).
        Ogre::SceneNode* mCaelumCameraNode = nullptr;

        /// RIGSOFRODS: CaelumPort's topmost scene node for nicer inspector view
        Ogre::SceneNode* mCaelumportGroupingNode = nullptr;

        /// Automatically move the camera node.
        bool mAutoMoveCameraNode;

        /// Automatically call this->notifyCameraChanged.
        bool mAutoNotifyCameraChanged;

        /// Automatically attach compositors to viewports
        bool mAutoAttachViewportsToComponents;

        /// Automatically set the viewport colour to black.
        bool mAutoViewportBackground;

        /// Fog Mode caelum should use
        Ogre::FogMode mManageSceneFogMode;

        /// Distances for linear fog mode
        Ogre::Real    mManageSceneFogFromDistance, mManageSceneFogToDistance;

        Real mGlobalFogDensityMultiplier;
        Ogre::ColourValue mGlobalFogColourMultiplier;

        Real mSceneFogDensityMultiplier;
        Ogre::ColourValue mSceneFogColourMultiplier;

        Real mGroundFogDensityMultiplier;
        Ogre::ColourValue mGroundFogColourMultiplier;

        /// Flag for managing scene ambient light.
        bool mManageAmbientLight;

        /// Minimum ambient light; only useful if mManageAmbientLight
        Ogre::ColourValue mMinimumAmbientLight;

        /// If only one light source should enabled at a time.
        bool mEnsureSingleLightSource;

        /// Ensure only one of the light sources casts shadows.
        bool mEnsureSingleShadowSource;

        /// The sky gradients image (for lookups).
        std::unique_ptr<Ogre::Image> mSkyGradientsImage;

        /// The sun gradients image (for lookups).
        std::unique_ptr<Ogre::Image> mSunColoursImage;

        /// Observer Latitude (on the earth).
        Ogre::Degree mObserverLatitude;
        /// Observer Longitude (on the earth).
        Ogre::Degree mObserverLongitude;

        static const Ogre::Vector3 makeDirection (
                Ogre::Degree azimuth, Ogre::Degree altitude);
        
        // References to sub-components
        std::unique_ptr<UniversalClock> mUniversalClock;
        std::unique_ptr<BaseSkyLight> mSun;
        std::unique_ptr<Moon> mMoon;
        std::unique_ptr<PointStarfield> mPointStarfield;

        // RIGSOFRODS: Cached values to be retrieved by SkyX controller functions.
        Ogre::Vector3 mCachedSunDir = Ogre::Vector3::ZERO;
        Ogre::Vector3 mCachedMoonDir = Ogre::Vector3::ZERO;
        float mCachedMoonPhase = 0.0f;

    public:
        typedef std::set<Ogre::Viewport*> AttachedViewportSet;

    private:
        AttachedViewportSet mAttachedViewports;

    public:

        // RIGSOFRODS: Removed component flags.

        static const String DEFAULT_SKY_GRADIENTS_IMAGE;
        static const String DEFAULT_SUN_COLOURS_IMAGE;
    
        /** Constructor.
         *  Registers itself in the Ogre engine and initialises the system.
         *
         *  @param root The Ogre root.
         *  @param scene The Ogre scene manager.
         *  @param componentsToCreate Default components for @see autoConfigure.
         */
        CaelumSystem (
                Ogre::Root *root, 
                Ogre::SceneManager *sceneMgr,
                Ogre::SceneNode* groupingSceneNode);

        /** Revert everything to defaults.
         *
         *  This function will delete all subcomponents and revert everything
         *  to default values (the values which are also set on construction).
         */
        void clear ();

        /** Create some default component with resonable default settings.
         *  This will always call clear() before creating components.
         */
        void autoConfigure ();

        /** Destructor.
         */
        ~CaelumSystem ();

        /** Update the whole system manually.
         *  You have to call this yourself if you don't register CaelumSystem
         *  as an ogre frame listener. Otherwise it's called automatically.
         *
         *  @param timeSinceLastFrame: Time passed since last frame.
         */
        void updateSubcomponents (Real timeSinceLastFrame);

        /** Notify subcomponents of camera changes.
         *  This function must be called after camera changes but before
         *  rendering with that camera. If multiple cameras are used it must
         *  be called for each camera before the camera is rendered with.
         *
         *  This function will move caelum's camera node to the camera
         *  position, but only if getAutoMoveCameraNode.
         *  It will also call CameraBoundElement::notifyCameraChanged
         */
        void notifyCameraChanged(Ogre::Camera* cam);

        /** Get the scene manager for this caelum system.
         *  This is set in the constructor. CaelumSystem can't exist without a valid scene manager.
         */
        inline Ogre::SceneManager* getSceneMgr() const { return mSceneMgr; }

        /// Gets root scene node for camera-bound elements
        inline Ogre::SceneNode* getCaelumCameraNode(void) const { return mCaelumCameraNode; }

        /// Gets root scene node for ground-bound elements.
        inline Ogre::SceneNode* getCaelumportGroundNode(void) const { return mCaelumportGroupingNode; }

        /** If true; listen to preViewportUpdate and automatically notifyCameraChanged();
         *
         *  This is on by default; but does not work with compositors.
         *
         *  You must attach CaelumSystem as a RenderTargetListener manually for
         *  this to work; as in version 0.3.
         */
        inline void setAutoNotifyCameraChanged(bool value) { mAutoNotifyCameraChanged = value; }
        /// @see setAutoNotifyCameraChanged
        inline bool getAutoNotifyCameraChanged() const { return mAutoNotifyCameraChanged; }

        /** If true; automatically attach viewports to subcomponents.
         *
         *  Some subcomponents use compositors and those compositors need to
         *  be attached to individual viewports. By default CaelumSystem will
         *  try take to take care of that automatically.
         *
         *  This property allows you to disable that behaviour. If set to false
         *  you must call functions like
         *  PrecipitationController::createViewportInstance manually.
         *
         *  @see attachViewport detachViewport
         */
        inline void setAutoAttachViewportsToComponents(bool value) { mAutoAttachViewportsToComponents = value; }
        /// @see setAutoAttachViewportsToComponents.
        inline bool getAutoAttachViewportsToComponents() const { return mAutoAttachViewportsToComponents; }

        /** If true (default); automatically move the camera node in notifyCameraChanged.
         *  If disable you get full control of the camera node; and in theory
         *  you can attach it to the scene graph however you please.
         */
        inline void setAutoMoveCameraNode(bool value) { mAutoMoveCameraNode = value; }
        /// @see setAutoMoveCameraNode
        inline bool getAutoMoveCameraNode() { return mAutoMoveCameraNode; }

        /** If true; automatically set the viewport color to black.
         *  Caelum's domes relies on the viewport background being black.
         *  There's generally no reason to disable this and it's on by default.
         */
        inline void setAutoViewportBackground(bool value) { mAutoViewportBackground = value; }
        /// @see setAutoViewportBackground
        inline bool getAutoViewportBackground() const { return mAutoViewportBackground; }

        /// Get the observer's longitude. East is positive, west is negative.
        inline const Ogre::Degree getObserverLongitude () const { return mObserverLongitude; }

        /// Set the observer's longitude. East is positive, west is negative.
        inline void setObserverLongitude (Ogre::Degree value) { mObserverLongitude = value; }

        /// Get the observer's latitude. North is positive, south is negative.
        inline const Ogre::Degree getObserverLatitude () const { return mObserverLatitude; }

        /// Set the observer's latitude. North is positive, south is negative.
        inline void setObserverLatitude (Ogre::Degree value) { mObserverLatitude = value; }

        inline LongReal getJulianDay () const { return mUniversalClock->getJulianDay (); }
        inline void setJulianDay (LongReal value) { mUniversalClock->setJulianDay (value); }
        inline Real getTimeScale () const { return mUniversalClock->getTimeScale (); }
        inline void setTimeScale (Real value) { mUniversalClock->setTimeScale (value); }

    public:
        /** Attach CaelumSystem to a viewport.
         *  You should call this for every new viewport looking at the scene
         *  where CaelumSystem is created.
         *
         *  If the viewport is already attached then nothing happens.
         *
         *  If getAutoAttachViewportsToComponents() this will add Caelum's compositors.
         */
        void attachViewport (Ogre::Viewport* rt);

        /** Reverse of @see attachViewport.
         *  You need to call this when you destroy a viewport.
         *
         *  If the viewport is not already attached nothing happens.
         */
        void detachViewport (Ogre::Viewport* rt);

        /** Check if one particular viewport is attached.
         */
        bool isViewportAttached (Ogre::Viewport* vp) const;

        /** Detach from all viewports.
         */
        void detachAllViewports ();

        /// Get a reference to the set of attached viewports.
        const AttachedViewportSet& _getAttachedViewportSet () { return mAttachedViewports; }

    protected:
        // Do the work behind attach/detach viewport.
        void attachViewportImpl (Ogre::Viewport* rt);
        void detachViewportImpl (Ogre::Viewport* rt);
        
    public:
        /// Gets the universal clock.
        inline UniversalClock *getUniversalClock () const { return mUniversalClock.get(); }



        /// Gets the current sun, or null if disabled.
        inline BaseSkyLight* getSun () const { return mSun.get (); }
        /// Set the sun, or null to disable.
        void setSun (BaseSkyLight* obj);

        /// Gets the current moon, or null if disabled.
        inline Moon* getMoon () const { return mMoon.get (); }
        /// Set the moon, or null to disable.
        void setMoon (Moon* obj);



        /// Gets the current point starfield, or null if disabled.
        inline PointStarfield* getPointStarfield () const { return mPointStarfield.get (); }
        /// Set image starfield, or null to disable.
        void setPointStarfield (PointStarfield* obj);




 
        /** Enables/disables Caelum managing standard Ogre::Scene fog.
            This makes CaelumSystem control standard Ogre::Scene fogging. It
            will use EXP2 fog with density from SkyColourModel.

            Fog density multipliers are used; final scene fog density is:
            SceneMultiplier * GlobalMultiplier * SkyColourModel.GetFogDensity

            When this is set to false it also disables all scene fog (but you
            control it afterwards).

            @param value New value
         */
        void setManageSceneFog (Ogre::FogMode f);
        void disableFogMangement();

        /** Tells if Caelum is managing the fog or not.
            @return The value set in setManageSceneFog.
         */
        Ogre::FogMode getManageSceneFog () const;

        void setManageSceneFogStart (Ogre::Real from);
        void setManageSceneFogEnd (Ogre::Real to);
        Ogre::Real getManageSceneFogStart() const;
        Ogre::Real getManageSceneFogEnd() const;

        /** Multiplier for scene fog density (default 1).
            This is an additional multiplier for Ogre::Scene fog density.
            This has no effect if getManageSceneFog is false.

            Final scene fog density is:
            SceneMultiplier * GlobalMultiplier * SkyColourModel.GetFogDensity
         */
        void setSceneFogDensityMultiplier (Real value);

        /** Get the value set by setSceneFogDensityMultiplier.
         */
        Real getSceneFogDensityMultiplier () const;

        /** Set an additional multiplier for fog colour as it comes from SkyColourModel.
         *  This is 0.7 by default; to be compatible with previous versions.
         */
        inline void setSceneFogColourMultiplier (const Ogre::ColourValue& value) { mSceneFogColourMultiplier = value; }

        /// See setSceneFogColourMultiplier.
        inline const Ogre::ColourValue getSceneFogColourMultiplier () const { return mSceneFogColourMultiplier; }

        /** Multiplier for ground fog density (default 1).
         *  This is an additional multiplier for Caelum::GroundFog DepthComposer ground fog density.
         *
         *  Final ground fog density is:
         *  GroundFogMultipler * GlobalMultiplier * SkyColourModel.GetFogDensity
         */
        void setGroundFogDensityMultiplier (Real value);

        /** Get the value set by setGroundFogDensityMultiplier.
         */
        Real getGroundFogDensityMultiplier () const;

        /** Set an additional multiplier for ground fog colour as it comes from SkyColourModel.
         *  This is OgreColour::White by default; which has no effect.
         */
        inline void setGroundFogColourMultiplier (const Ogre::ColourValue& value) { mGroundFogColourMultiplier = value; }

        /// See setGroundFogColourMultiplier.
        inline const Ogre::ColourValue getGroundFogColourMultiplier () const { return mGroundFogColourMultiplier; }

        /** Multiplier for global fog density (default 1).
         *  This is an additional multiplier for fog density as received from
         *  SkyColourModel. There are other multipliers you can tweak for
         *  individual kinds of fog; but this is what you should change from
         *  whatever "game logic" you might have.
         */
        void setGlobalFogDensityMultiplier (Real value);

        /** Get the value set by setSceneFogDensityMultiplier.
         */
        Real getGlobalFogDensityMultiplier () const;

        /** Set an additional multiplier for fog colour.
         *  This will also affect stuff like clouds or precipitation. Careful!
         *  This is OgreColour::White by default; which has no effect.
         */
        inline void setGlobalFogColourMultiplier (const Ogre::ColourValue& value) { mGlobalFogColourMultiplier = value; }

        /// See setGlobalFogColourMultiplier.
        inline const Ogre::ColourValue getGlobalFogColourMultiplier () const { return mGlobalFogColourMultiplier; }

        /** Set this to true to have CaelumSystem manage the scene's ambient light.
         *  The colour and AmbientMultiplier of the sun and moon are used.
         *  This is false by default.
         */
        inline void setManageAmbientLight (bool value) { mManageAmbientLight = value; }

        /// Check if CaelumSystem is managing ambient lighting.
        inline bool getManageAmbientLight () const { return mManageAmbientLight; }

        /** Set the minimum value for scene ambient lighting,
         *  This is only used if getManageAmbientLight() is true.
         *  By default this value is Ogre::ColourValue::Black, so it has no effect.
         */
        inline void setMinimumAmbientLight (const Ogre::ColourValue &value) { mMinimumAmbientLight = value; }

        /// @see setMinimumAmbientLight
        inline const Ogre::ColourValue getMinimumAmbientLight () const { return mMinimumAmbientLight; }

        /** Ensure only one of caelum's light sources is active at a time (the brightest).
         *  This uses SkyLight::setForceDisable to disable low-intensity lightsources.
         *  Their contribution to ambient lighting is not affected.
         *  This implies a single shadow caster.
         *  This is disabled by default; and you can tweak light disabling by yourself.
         */
        inline void setEnsureSingleLightSource (bool value) { mEnsureSingleLightSource = value; }

        /// See setEnsureSingleLightSource
        inline bool getEnsureSingleLightSource () const { return mEnsureSingleLightSource; }

        /** Ensure only one of caelum's light sources casts shadows (the brightest).
         *  Disabled by default.
         */
        inline void setEnsureSingleShadowSource (bool value) { mEnsureSingleShadowSource = value; }

        /// See setEnsureSingleShadowSource
        inline bool getEnsureSingleShadowSource () const { return mEnsureSingleShadowSource; }

        /** Gets the fog colour for a certain daytime.
            @param time The current time.
            @param sunDir The sun direction.
            @return The fog colour.
         */
        Ogre::ColourValue getFogColour (Real time, const Ogre::Vector3 &sunDir);

        /** Gets the fog density for a certain daytime.
            @param time The current time.
            @param sunDir The sun direction.
            @return The fog density.
         */
        Real getFogDensity (Real time, const Ogre::Vector3 &sunDir);

        /** Get the colour of the sun sphere.
         *  This colour is used to draw the sun sphere in the sky.
         *  @return The colour of the sun.
         */
        Ogre::ColourValue getSunSphereColour (Real time, const Ogre::Vector3 &sunDir);

        /** Gets the colour of sun light.
         *  This color is used to illuminate the scene.
         *  @return The colour of the sun's light
         */
        Ogre::ColourValue getSunLightColour (Real time, const Ogre::Vector3 &sunDir);

        /// Gets the colour of moon's body.
        Ogre::ColourValue getMoonBodyColour (const Ogre::Vector3 &moonDir);

        /// Gets the colour of moon's light.
        Ogre::ColourValue getMoonLightColour (const Ogre::Vector3 &moonDir);

        /// Set the sun gradients image.
        void setSkyGradientsImage (const Ogre::String &filename = DEFAULT_SKY_GRADIENTS_IMAGE);

        /// Set the sun colours image.
        /// Sun colour is taken from this image.
        void setSunColoursImage (const Ogre::String &filename = DEFAULT_SUN_COLOURS_IMAGE);

        /** Get the sun's direction at a certain time.
         *  @param jday astronomical julian day.
         *  @see UniversalClock for julian day calculations.
         */
        const Ogre::Vector3 getSunDirection (LongReal jday);

        /** Get the moon's direction at a certain time.
         *  @param jday astronomical julian day.
         */
        const Ogre::Vector3 getMoonDirection (LongReal jday);

        /** Function to get the phase of the moon
         *  @param jday Julian day
         *  @return the phase of the moon; ranging from 0(full moon) to 1(again full moon).
         */
        Ogre::Real getMoonPhase (LongReal jday);

        /** Get the ecliptic's north pole direction at a certain time.
         *  Useful as Moon's north polar axis points within 1.5 degrees of the north ecliptic pole.
         *  @param jday astronomical julian day.
         */
        const Ogre::Vector3 getEclipticNorthPoleDirection (LongReal jday);
        
    private:
        /** Handle FrameListener::frameStarted to call updateSubcomponents every frame.
         *  If you don't register CaelumSystem as a an ogre frame listener you have to
         *  call updateSubcomponents yourself.
         */
        virtual bool frameStarted (const Ogre::FrameEvent &e);

        /** Event trigger called just before rendering a viewport in a render target Caelum is attached to.
            Useful to make objects follow every camera that renders a viewport in a certain render target.
         */
        virtual void preViewportUpdate (const Ogre::RenderTargetViewportEvent &e);

        /** Free all subcomponents, but not CaelumSystem itself. Can be called multiple times.
         *  @param everything To destroy things that can't be rebuilt.
         */
        void destroySubcomponents (bool everything);

    public: // SkyX::Controller implementation
        virtual Ogre::Vector3 getSunDirection() const override { return -mCachedSunDir; }
        virtual Ogre::Vector3 getMoonDirection() const override { return -mCachedMoonDir; }
        virtual float getMoonPhase() const override { return (mCachedMoonPhase+1.f)/2.f; }
    };
}

#endif // CAELUM__CAELUM_SYSTEM_H
