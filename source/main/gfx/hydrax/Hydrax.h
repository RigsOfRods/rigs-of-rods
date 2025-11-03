/*
--------------------------------------------------------------------------------
This source file is part of Hydrax.
Visit ---

Copyright (C) 2008 Xavier Verguín González <xavierverguin@hotmail.com>
                                           <xavyiy@gmail.com>

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/lesser.txt.
--------------------------------------------------------------------------------
*/

#ifndef _Hydrax_Hydrax_H_
#define _Hydrax_Hydrax_H_

#include "Prerequisites.h"
#include "Application.h"

#include "Enums.h"
#include "Help.h"
#include "Mesh.h"
#include "Image.h"
#include "MaterialManager.h"
#include "RttManager.h"
#include "TextureManager.h"
#include "GodRaysManager.h"
#include "DecalsManager.h"
#include "GPUNormalMapManager.h"
#include "CfgFileManager.h"
#include "Module.h"

/// @addtogroup Gfx
/// @{

/// @addtogroup Hydrax
/// @{

namespace Hydrax
{
    /** Main Hydrax class.
	    Hydrax is a plugin for the Ogre3D engine whose aim is rendering realistic water scenes.
		Do not use two instances of the Hydrax class.
     */
    class Hydrax
    {
    public:
        /** Constructor
            @param sm Ogre SceneManager pointer
            @param c Ogre Camera pointer
			@param v Ogre Main window viewport pointer
         */
		Hydrax(Ogre::SceneManager *sm, Ogre::Camera *c, Ogre::Viewport *v);

        /** Destructor
         */
        ~Hydrax();

        /** Create all resources according with current Hydrax components and
		    add Hydrax to the scene.
            @remarks Call when all params are set
         */
        void create();

		/** Remove hydrax, you can call this method to remove Hydrax from the scene
		    or release (secondary) Hydrax memory, call create() to return Hydrax to the scene.
		 */
		void remove();

        /** Call every frame
            @todo Add listener interface
         */
        void update(const Ogre::Real &timeSinceLastFrame);

        /** Returns if the especified component is active
            @param Component Component that we want to check
         */
        bool isComponent(const HydraxComponent &Component);

        /** Set Hydrax components
            @param Components Components
            @remarks It can be called after create(), Components will be updated
         */
        void setComponents(const HydraxComponent &Components);

		/** Set Hydrax module
		    @param Module Hydrax module
			@param DeleteOldModule Delete, if exists, the old module
			@remark Module will be set before call create()
		 */
		void setModule(Module::Module* Module, const bool& DeleteOldModule = true);

        /** Set polygon mode (Solid, Wireframe, Points)
            @param PM Polygon mode
         */
		void setPolygonMode(const Ogre::PolygonMode& PM);

		/** Set shader mode
		    @param ShaderMode Shader mode
		 */
		void setShaderMode(const MaterialManager::ShaderMode &ShaderMode);

        /** Set water position
            @param Position Water position
         */
        void setPosition(const Ogre::Vector3 &Position);

		/** Rotate water and planes
		    @param q const Ogre::Quaternion&
		*/
		void rotate(const Ogre::Quaternion &q);

		/** Save hydrax config to file
		    @param File File name
			@param Path File path
			@return false if an error has been ocurred(Check the log file in this case).
			@remarks If module isn't set, module/noise options won't be saved.
		 */
		inline const bool saveCfg(const Ogre::String &File, const Ogre::String& Path = "") const
		{
			return mCfgFileManager->save(File, Path);
		}

		/** Load config from file
		    @param File File name
			@return false if an error has been ocurred(Check the log file in this case).
			@remarks The file must be registred in Hydrax resource group.
			         If module isn't set, or module isn't the same from
			         config file, module options won't be loaded.
		 */
		inline const bool loadCfg(const Ogre::String &File) const
		{
			return mCfgFileManager->load(File);
		}

        /** Set clip planes error
            @param PlanesError Clip planes error
         */
        void setPlanesError(const Ogre::Real &PlanesError);

        /** Set water strength GPU param
            @param Strength Water strength GPU param
         */
        void _setStrength(const Ogre::Real &Strength);

        /** Set full reflection distance
            @param FullReflectionDistance Full reflection distance
         */
        void setFullReflectionDistance(const Ogre::Real &FullReflectionDistance);

        void setGlobalTransparency(const Ogre::Real &GlobalTransparencyDistance);

        void setWaterColor(const Ogre::Vector3 &WaterColor);

        /** Set normal distortion
            @param NormalDistortion use low value, like 0.025
         */
        void setNormalDistortion(const Ogre::Real &NormalDistortion);

        /** Set sun position
            @param SunPosition Sun position
         */
        void setSunPosition(const Ogre::Vector3 &SunPosition);

        /** Set sun strength
            @param SunStrength Sun strength
         */
        void setSunStrength(const Ogre::Real &SunStrength);

        /** Set sun area
            @param SunArea Sun area
         */
        void setSunArea(const Ogre::Real &SunArea);

        /** Set sun color
            @param SunColor Sun color
         */
        void setSunColor(const Ogre::Vector3 &SunColor);

        /** Set foam max distance
            @param FoamMaxDistance Foam max distance
         */
        void setFoamMaxDistance(const Ogre::Real &FoamMaxDistance);

        /** Set foam scale
            @param FoamScale Foam scale
         */
        void setFoamScale(const Ogre::Real &FoamScale);

        /** Set foam start
            @param FoamStart Foam start
         */
        void setFoamStart(const Ogre::Real &FoamStart);

        /** Set foam transparency
            @param FoamTransparency Foam transparency
         */
        void setFoamTransparency(const Ogre::Real &FoamTransparency);

        /** Set depth limit
            @param DepthLimit Depth limit
         */
        void setDepthLimit(const Ogre::Real &DepthLimit);

        /** Set distance limit underwater (occlusion)
            @param DistLimit Distance limit
         */
        void setDistLimit(const Ogre::Real &DistLimit);

        /** Set smooth power
            @param SmoothPower Smooth power
            @remarks Less values more transition distance, hight values short transition values, 1-50 range(aprox.)
         */
        void setSmoothPower(const Ogre::Real &SmoothPower);

        /** Set caustics scale
            @param CausticsScale Caustics scale
         */
        void setCausticsScale(const Ogre::Real &CausticsScale);

        /** Set caustics power
            @param CausticsPower Caustics power
         */
        void setCausticsPower(const Ogre::Real &CausticsPower);

        /** Set caustics end
            @param CausticsEnd Caustics end
         */
        void setCausticsEnd(const Ogre::Real &CausticsEnd);

		/** Set god rays exposure
		    @param GodRaysExposure God rays exposure
		 */
		void setGodRaysExposure(const Ogre::Vector3 &GodRaysExposure)
		{
			mGodRaysExposure = GodRaysExposure;
		}

		/** Set god rays intensity
		    @param GodRaysIntensity God rays intensity
		 */
		void setGodRaysIntensity(const Ogre::Real &GodRaysIntensity)
		{
			mGodRaysIntensity = GodRaysIntensity;
		}

		/** Set the y-displacement under the water needed to change between underwater and overwater mode
			@param UnderwaterCameraSwitchDelta Underwater camera switch delta factor
			@remarks Useful to get a nice underwater-overwater transition, it depends of the world scale
		 */
		inline void setUnderwaterCameraSwitchDelta(const Ogre::Real& UnderwaterCameraSwitchDelta)
		{
		    mUnderwaterCameraSwitchDelta = UnderwaterCameraSwitchDelta;
		}

        /** Has create() already called?
            @return true is yes, false if not
         */
        inline const bool& isCreated() const
        {
            return mCreated;
        }

		/** Show/Hide hydrax water
		    @param Visible true to show, false to hide
			@remarks Resources aren't going to be realeased(Use remove() for this),
			         only RTT's are going to be stopped.
		 */
		void setVisible(const bool& Visible);

		/** Is hydrax water visible?
		    @return true if yes, false if not
		 */
		inline const bool& isVisible() const
		{
			return mVisible;
		}

		/** Get rendering camera
		    @return Ogre::Camera pointer
		 */
		inline Ogre::Camera* getCamera()
		{
			return mCamera;
		}

		/** Get main window viewport
		    @return Ogre::Viewport pointer
		 */
		inline Ogre::Viewport* getViewport()
		{
			return mViewport;
		}

		/** Get scene manager
		    @return Ogre::SceneManager pointer
		 */
		inline Ogre::SceneManager* getSceneManager()
		{
			return mSceneManager;
		}

		/** Get Hydrax::Mesh
		    @return Hydrax::Mesh pointer
		 */
		inline Mesh* getMesh()
		{
			return mMesh;
		}

		/** Get Hydrax::MaterialManager
		    @return Hydrax::MaterialManager pointer
		 */
		inline MaterialManager* getMaterialManager()
		{
			return mMaterialManager;
		}

		/** Get Hydrax::RttManager
		    @return Hydrax::RttManager pointer
		 */
		inline RttManager* getRttManager()
		{
			return mRttManager;
		}

		/** Get Hydrax::TextureManager
		    @return Hydrax::TextureManager pointer
		 */
		inline TextureManager* getTextureManager()
		{
			return mTextureManager;
		}

		/** Get Hydrax::GodRaysManager
		    @return Hydrax::GodRaysManager pointer
		 */
		inline GodRaysManager* getGodRaysManager()
		{
			return mGodRaysManager;
		}

		/** Get Hydrax::DecalsManager
		    @return Hydrax::DecalsManager pointer
		 */
		inline DecalsManager* getDecalsManager()
		{
			return mDecalsManager;
		}

		/** Get Hydrax::GPUNormalMapManager
		    @return Hydrax::GPUNormalMapManager pointer
	     */
		inline GPUNormalMapManager* getGPUNormalMapManager()
		{
			return mGPUNormalMapManager;
		}

		/** Get Hydrax::CfgFileManager
		    @return Hydrax::CfgFileManager pointer
	     */
		inline CfgFileManager* getCfgFileManager()
		{
			return mCfgFileManager;
		}

		/** Get our Hydrax::Module::Module
		    @return Hydrax::Module::Module pointer or NULL if Module isn't set.
		 */
		inline Module::Module* getModule()
		{
			return mModule;
		}

        /** Get hydrax components selected
            @return Hydrax components
         */
        inline const HydraxComponent& getComponents() const
        {
            return mComponents;
        }

        /** Get current polygon mode
            @return Current polygon mode
         */
		inline const Ogre::PolygonMode& getPolygonMode() const
        {
            return mPolygonMode;
        }

		/** Get current shader mode
		    @return Current shader mode
		 */
		inline const MaterialManager::ShaderMode& getShaderMode() const
		{
			return mShaderMode;
		}

        /** Get water position
            @return Water position
         */
        inline const Ogre::Vector3& getPosition() const
        {
            return mPosition;
        }

		/** Get current clip planes error
		    @return Current clip planes error
		 */
		inline const Ogre::Real& getPlanesError() const
		{
			return mPlanesError;
		}

		/** Get the current heigth at a especified world-space point
		    @param Position X/Z World position
			@return Heigth at the given position in y-World coordinates, if it's outside of the water return -1
		 */
		inline float getHeigth(const Ogre::Vector2 &Position)
		{
			if (mModule)
			{
				return mModule->getHeigth(Position);
			}

			return -1;
		}

		/** Get the current heigth at a especified world-space point
		    @param Position X/(Y)/Z World position
			@return Heigth at the given position in y-World coordinates, if it's outside of the water return -1
		 */
		inline float getHeigth(const Ogre::Vector3 &Position)
		{
			return getHeigth(Ogre::Vector2(Position.x, Position.z));
		}

        /** Get full reflection distance
            @return Hydrax water full reflection distance
         */
        inline const Ogre::Real& getFullReflectionDistance() const
        {
            return mFullReflectionDistance;
        }

        /** Get global transparency
            @return Hydrax water global transparency
         */
        inline const Ogre::Real& getGlobalTransparency() const
        {
            return mGlobalTransparency;
        }

        /** Get sun position
            @return Sun position
         */
        inline const Ogre::Vector3& getSunPosition() const
        {
            return mSunPosition;
        }

		/** Get water color
            @return Water color
         */
        inline const Ogre::Vector3& getWaterColor() const
        {
            return mWaterColor;
        }

        /** Get normal distortion
            @return Hydrax normal distortion
         */
        inline const Ogre::Real& getNormalDistortion() const
        {
            return mNormalDistortion;
        }

        /** Get water strength
            @return Hydrax water strength
         */
        inline const Ogre::Real& getSunStrength() const
        {
            return mSunStrength;
        }

        /** Get sun area
            @return Sun area
         */
        inline const Ogre::Real& getSunArea() const
        {
            return mSunArea;
        }

        /** Get sun color
            @return Sun color
         */
        inline const Ogre::Vector3& getSunColor() const
        {
            return mSunColor;
        }

        /** Get foam max distance
            @return Foam max distance
         */
        inline const Ogre::Real& getFoamMaxDistance() const
        {
            return mFoamMaxDistance;
        }

        /** Get foam scale
            @return Foam scale
         */
        inline const Ogre::Real& getFoamScale() const
        {
            return mFoamScale;
        }

        /** Get foam start
            @return Foam start
         */
        inline const Ogre::Real& getFoamStart() const
        {
            return mFoamStart;
        }

        /** Get foam transparency
            @return Foam scale
         */
        inline const Ogre::Real& getFoamTransparency() const
        {
            return mFoamTransparency;
        }

        /** Get depth limit
            @return Depth limit
         */
        inline const Ogre::Real& getDepthLimit() const
        {
            return mDepthLimit;
        }

        /** Get distance limit (viewable underwater)
            @return Distance limit
         */
        inline const Ogre::Real& getDistLimit() const
        {
            return mDistLimit;
        }

        /** Get smooth power
            @return Smooth power
         */
        inline const Ogre::Real& getSmoothPower() const
        {
            return mSmoothPower;
        }

		/** Get caustics scale
            @return Caustics scale
         */
        inline const Ogre::Real& getCausticsScale() const
        {
            return mCausticsScale;
        }

        /** Get caustics power
            @return Caustics power
         */
        inline const Ogre::Real& getCausticsPower() const
        {
            return mCausticsPower;
        }

        /** Get caustics end
            @return Caustics end
         */
        inline const Ogre::Real& getCausticsEnd() const
        {
            return mCausticsEnd;
        }

		/** Get God rays exposure factors
		    @return God rays exposure factors
		 */
		inline const Ogre::Vector3& getGodRaysExposure() const
		{
			return mGodRaysExposure;
		}

		/** Get God rays intensity
		    @return God rays intensity
		 */
		inline const Ogre::Real& getGodRaysIntensity() const
		{
			return mGodRaysIntensity;
		}

		/** Get the y-displacement under the water needed to change between underwater and overwater mode
			@return Underwater camera switch delta
			@remarks Useful to get a nice underwater-overwater transition, it depends of the world scale
		 */
		inline const Ogre::Real& getUnderwaterCameraSwitchDelta() const
		{
			return mUnderwaterCameraSwitchDelta;
		}

		/** Is current frame underwater?
		    @return true If yes, false if not
		 */
		inline const bool& _isCurrentFrameUnderwater() const
		{
			return mCurrentFrameUnderwater;
		}

    private:

        /** Device listener
	     */
	    class DeviceListener : public Ogre::RenderSystem::Listener
	    {
		public:
			/// Hydrax manager pointer
            Hydrax* mHydrax;

			/** Event occurred
			    @param eventName Name of the event
				@param parameters Ogre::NameValuePairList pointer
			 */
			void eventOccurred(const Ogre::String& eventName, const Ogre::NameValuePairList *parameters);
		};

        /** Update normal map textures
         */
        void _updateNM();

		/** setVisible() helper funtion
		 */
		void _checkVisible();

		/** Check for underwater effects
		    @param timeSinceLastFrame Time since last frame
		 */
		void _checkUnderwater(const Ogre::Real& timeSinceLastFrame);

        /// Has create() already called?
        bool mCreated;

		/// Is hydrax water visible?
		bool mVisible;

        /// Hydrax components
        HydraxComponent mComponents;
		/// Current shader mode
		MaterialManager::ShaderMode mShaderMode;

		/// Device listener
		DeviceListener mDeviceListener;

		/// Polygon mode (Solid, Wireframe, Points)
		Ogre::PolygonMode mPolygonMode;
        /// Water position
        Ogre::Vector3 mPosition;
        /// Planes error, y axis clipplanes displacement
        Ogre::Real mPlanesError;

        /// Full reflection distance param
        Ogre::Real mFullReflectionDistance;
        /// Global transparency param
        Ogre::Real mGlobalTransparency;
		/// Water color param
        Ogre::Vector3 mWaterColor;
        /// Normal distortion param
        Ogre::Real mNormalDistortion;

        /// Sun position
        Ogre::Vector3 mSunPosition;
        /// Sun strength param
        Ogre::Real mSunStrength;
        /// Sun area
        Ogre::Real mSunArea;
        /// Sun color
        Ogre::Vector3 mSunColor;

        /// Foam max distance param
        Ogre::Real mFoamMaxDistance;
        /// Foam scale param
        Ogre::Real mFoamScale;
        /// Foam start param
        Ogre::Real mFoamStart;
        /// Foam transparency param
        Ogre::Real mFoamTransparency;

        /// Depth limit param
        Ogre::Real mDepthLimit;
		/// Distance limit param (viewable underwater)
		Ogre::Real mDistLimit;

        /// Smooth power param
        Ogre::Real mSmoothPower;

        /// Caustics scale param
        Ogre::Real mCausticsScale;
        /// Caustics power
        Ogre::Real mCausticsPower;
        /// Caustics end
        Ogre::Real mCausticsEnd;

		/// God rays exposure factors
		Ogre::Vector3 mGodRaysExposure;
		/// God rays intensity
		Ogre::Real mGodRaysIntensity;

		// Delta-displacement in Y-AXIS before changing to underwater mode
		Ogre::Real mUnderwaterCameraSwitchDelta;

		/// Is current frame underwater?
		bool mCurrentFrameUnderwater;

        /// Our Hydrax::Mesh pointer
        Mesh *mMesh;
		/// Our Hydrax::MaterialManager
		MaterialManager *mMaterialManager;
		/// Our Hydrax::RttManager
		RttManager *mRttManager;
		/// Our Hydrax::TextureManager pointer
		TextureManager *mTextureManager;
		/// Our Hydrax::GodRaysManager pointer
		GodRaysManager *mGodRaysManager;
		/// Our Hydrax::DecalsManager pointer
		DecalsManager *mDecalsManager;
		/// Our Hydrax::GPUNormalMapManager pointer
		GPUNormalMapManager *mGPUNormalMapManager;
		/// Our Hydrax::CfgFileManager pointer
		CfgFileManager *mCfgFileManager;
		/// Our Hydrax::Module::Module pointer
		Module::Module *mModule;

        /// Pointer to Ogre::SceneManager
        Ogre::SceneManager *mSceneManager;
        /// Pointer to Ogre::Camera
        Ogre::Camera *mCamera;
		/// Pointer to main window viewport
		Ogre::Viewport *mViewport;
    };
}

/// @} // addtogroup Hydrax
/// @} // addtogroup Gfx

#endif
