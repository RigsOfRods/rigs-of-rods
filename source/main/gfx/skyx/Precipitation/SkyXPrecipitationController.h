// This file is part of the Caelum project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution.

#pragma once

#include "SkyXFastGpuParamRef.h"

namespace SkyX // Ported to SkyX by ohlidalp
{
	struct PrecipitationPresetParams
	{
		Ogre::ColourValue Colour;
        float Speed;
		std::string Name;
	};

	enum PrecipitationType
	{
		PRECTYPE_DRIZZLE		= 0,
		PRECTYPE_RAIN			= 1,
		PRECTYPE_SNOW			= 2,
		PRECTYPE_SNOWGRAINS	    = 3,
		PRECTYPE_ICECRYSTALS	= 4,
		PRECTYPE_ICEPELLETS	    = 5,
		PRECTYPE_HAIL			= 6,
		PRECTYPE_SMALLHAIL		= 7,

		PRECTYPE_CUSTOM		    = 8,
	};

    /** Compositor-based precipitation controller.
     *  This class will add and control precipitation controllers to viewports.
     *
     *  Compositors clone the composing materials. This controller will
     *  register itself as a compositor listener and change the material in notifyMaterialSetup.
     */
	class PrecipitationController
	{
	private:
        friend class PrecipitationInstance;

		Ogre::SceneManager *mSceneMgr;
		Ogre::Vector3 mWindSpeed;
		float mIntensity;
		float mSpeed;
		Ogre::ColourValue mColour;
		PrecipitationType mPresetType;
		std::string mTextureName;
        Ogre::Vector3 mCameraSpeedScale;
        const Ogre::Vector3 mFallingDirection = Ogre::Vector3::NEGATIVE_UNIT_Y;

        float mAutoDisableThreshold;
        bool mHardDisableCompositor;

		Ogre::ColourValue mSceneColour;
		float mInternalTime;

        // Only meant for the instance ctl in auto-camera-speed mode.
        float mSecondsSinceLastFrame;
        inline float getSecondsSinceLastFrame() { return mSecondsSinceLastFrame; }

	public:
        static const std::string COMPOSITOR_NAME; // resource name
        static const std::string MATERIAL_NAME;

		static bool isPresetTypeValid (PrecipitationType value);
		static const PrecipitationPresetParams& getPresetParams (PrecipitationType value);
		void setParams (const PrecipitationPresetParams& params);
        void setPresetType (PrecipitationType value);

        /** Get the preset type.
         *  Will return PRECIPITATION_CUSTOM if you modify parameters manually
         *  after setPresetType.
         */
        PrecipitationType getPresetType () const;

		/// @name Configuration parameters (use SkyX config manager)
        /// @{
        
		void setTextureName(const std::string& textureName);
		const std::string getTextureName() const;

		void setColour(const Ogre::ColourValue& color);
		const Ogre::ColourValue getColour() const;

		void setSpeed(float value);
		float getSpeed() const;

		void setIntensity(float value);
		float getIntensity() const;

		void setWindSpeed(const Ogre::Vector3 &value);
		const Ogre::Vector3 getWindSpeed() const;

		/// Set manual camera speed for all viewports.
		void setManualCameraSpeed(const Ogre::Vector3 &value);

        /// Set auto camera speed everywhere.o
		void setAutoCameraSpeed();

        /** Automatically disable compositors when intensity is low.
         *  A negative value always enable the compositor.
         *  @note: Ogre::CompositorInstance allocates/frees resources when
         *         enabling or disabling. That is expensive.
         */
        inline void setAutoDisableThreshold (float value) { mAutoDisableThreshold = value; }
        inline float getAutoDisableThreshold () const { return mAutoDisableThreshold; }

        /** Automatically scale camera speed.
         *
         *  This is multiplied per-component with camera speed; manual or
         *  automatic. It's most useful for automatic camera speed to control 
         *  how much of an effect moving the camera has on rain drop directions.
         *
         *  The components should probably be equal.
         *
         *  Default in Ogre::Vector3::UNIT_SCALE
         */
        inline void setCameraSpeedScale (const Ogre::Vector3& value) {
            mCameraSpeedScale = value;
        }
        inline const Ogre::Vector3 getCameraSpeedScale () const {
            return mCameraSpeedScale;
        }

        /** Set an equal camera speed scale in all dimensions.
         */
        inline void setCameraSpeedScale (float value) {
            setCameraSpeedScale(Ogre::Vector3::UNIT_SCALE * value);
        }

        /// @}

        /** Update the the precipitation controller.
         *  @param secondsSinceLastFrame Number of secods since the last frame.
         */
		void update(float secondsSinceLastFrame, Ogre::ColourValue colour);

		PrecipitationController(
				Ogre::SceneManager *sceneMgr);
		~PrecipitationController();

    public:
        typedef std::map<Ogre::Viewport*, PrecipitationInstance*> ViewportInstanceMap;
        ViewportInstanceMap mViewportInstanceMap;

    public:
        /// Add precipitation to a certain viewport.
        PrecipitationInstance* createViewportInstance(Ogre::Viewport* viewport);

        /// Remove precipitation from a certain viewport.
        void destroyViewportInstance(Ogre::Viewport* viewport);

        /// Get per-viewport instance, or null if not created yet.
        PrecipitationInstance* getViewportInstance(Ogre::Viewport* viewport);

        /// Remove from all attached viewports; clean up.
        void destroyAllViewportInstances();
	};

    /** Per-viewport instance of precipitation.
     *  This will create and control an ogre::CompositorInstance.
     */
    class PrecipitationInstance: private Ogre::CompositorInstance::Listener
    {
    private:
        friend class PrecipitationController;

        PrecipitationController* mParent;
        Ogre::Viewport* mViewport;
        Ogre::CompositorInstance* mCompInst;
        Ogre::Camera* mLastCamera;
        Ogre::Vector3 mLastCameraPosition;
        Ogre::Vector3 mCameraSpeed;
		bool mAutoCameraSpeed;

        virtual void notifyMaterialSetup(uint pass_id, Ogre::MaterialPtr &mat);
        virtual void notifyMaterialRender(uint pass_id, Ogre::MaterialPtr &mat);

        /// Called to enforce parameters on a composing material
        /// Called from notifyMaterialRender.
    	void _updateMaterialParams(
                const Ogre::MaterialPtr& mat,
                const Ogre::Camera* cam,
                const Ogre::Vector3& camSpeed);

        // Add remove the compositors. Do nothing if already added/removed
        void createCompositor ();
        void destroyCompositor ();

        // Check if the compositor should be enabled
        bool shouldBeEnabled () const;

        /// Called from the parent's update.
        void _update();

    public:
        inline Ogre::Viewport* getViewport() const { return mViewport; }
        inline PrecipitationController* getParent() const { return mParent; }
        inline Ogre::CompositorInstance* getCompositorInstance() const { return mCompInst; }

        /// Check if camera speed is automatically calculated (default true).
        bool getAutoCameraSpeed();

        /** Set camera speed to automatic calculation.
         *
         *  @warning: This runs into difficult precission issues. It is
         *  better to use setManualCameraSpeed.
         */
        void setAutoCameraSpeed();

        /// Set manual camera speed; disables automatic calculation.
        void setManualCameraSpeed(const Ogre::Vector3& value);

        /// Get current camera speed. Doesn't include CameraSpeedScale.
        const Ogre::Vector3 getCameraSpeed();

        PrecipitationInstance(PrecipitationController* parent, Ogre::Viewport* view);
        virtual ~PrecipitationInstance();

    private:
        struct Params {
            void setup(Ogre::GpuProgramParametersSharedPtr fpParams);

            Ogre::GpuProgramParametersSharedPtr fpParams;
            FastGpuParamRef precColor;
            FastGpuParamRef intensity;
            FastGpuParamRef dropSpeed;
            FastGpuParamRef corner1;
            FastGpuParamRef corner2;
            FastGpuParamRef corner3;
            FastGpuParamRef corner4;
            FastGpuParamRef deltaX;
            FastGpuParamRef deltaY;
        } mParams;

    };
}

