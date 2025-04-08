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

#ifndef _Hydrax_RttManager_H_
#define _Hydrax_RttManager_H_

#include "Prerequisites.h"

#include "Enums.h"
#include "Help.h"

/// @addtogroup Gfx
/// @{

/// @addtogroup Hydrax
/// @{

namespace Hydrax
{
	class Hydrax;

	/** Rtt's manager class
	 */
	class RttManager
	{
	public:
		/** Rtt enumeration
		 */
		enum RttType
		{
			RTT_REFLECTION       = 0,
			RTT_REFRACTION       = 1,
			RTT_DEPTH            = 2,
			RTT_DEPTH_REFLECTION = 3,
			RTT_DEPTH_AIP        = 4,
			RTT_GPU_NORMAL_MAP   = 5
		};

		/** Bits per channel
		 */
		enum BitsPerChannel
		{
			BPC_8  = 8,
			BPC_16 = 16,
			BPC_32 = 32
		};

		/** Number of channels
		 */
		enum NumberOfChannels
		{
			NOC_1 = 1,
			NOC_2 = 2,
			NOC_3 = 3,
			NOC_4 = 4
		};

		/** Rtt options struct
		 */
		struct RttOptions
		{
			/// Texture names
		    Ogre::String Name;
			/// Size; Size(0,0) to get main viewport size
			Size Size_;
			/// Number of channels
			NumberOfChannels NumberOfChannels_;
			/// Bits per channel
			BitsPerChannel BitsPerChannel_;
		};

		/** Rtt Listener class
		 */
		class RttListener
		{
		public:
			/** Funtion that is called before the Rtt will render
			    @param Rtt Rtt type
			    @remarks We've to override it
			 */
			virtual void preRenderTargetUpdate(const RttType& Rtt){};

			/** Funtion that is called after the Rtt will render
				@param Rtt Rtt type
				@remarks We've to override it
			*/
			virtual void postRenderTargetUpdate(const RttType& Rtt){};
		};

		/** Constructor
		    @param h Hydrax parent pointer
	     */
		RttManager(Hydrax *h);

		/** Destructor
		 */
	    ~RttManager();

		/** Initialize a RTT
		    @param Rtt Rtt to initialize
			@remarks If the RTT is already created, it will be recreated.
		 */
		void initialize(const RttType& Rtt);

		/** Removes a RTT
		    @param Rtt Rtt to remove
		 */
		void remove(const RttType& Rtt);

		/** Remove all RttManager resources
		    @remarks After calling removeAll(), calling initialize(...) is allowed.
		 */
		void removeAll();

		/** Get RTT texture name
		 */
		inline const Ogre::String& getRttName(RttType Rtt) const
		{
			return mRttOptions[static_cast<int>(Rtt)].Name;
		}

		/** Get Rtt texture
		 */
		inline Ogre::TexturePtr getTexture(RttType Rtt)
		{
			return mTextures[static_cast<int>(Rtt)];
		}

		inline Ogre::SceneNode* getPlanesSceneNode()
		{
			return mPlanesSceneNode;
		}

		/** Get Rtt plane
		 */
		inline Ogre::MovablePlane* getPlane(RttType Rtt)
		{
			return mPlanes[static_cast<int>(Rtt)];
		}

		inline const Size& getTextureSize(const RttType& Rtt) const
		{
			return mRttOptions[static_cast<int>(Rtt)].Size_;
		}

		/** Set Rtt texture size
			@param S New texture size (0,0 -> get main viewport size)
		 */
		void setTextureSize(const RttType& Rtt, const Size& S);

		/** Set Rtt textures size
			@param S New texture size (0,0 -> get main viewport size)
		 */
		void setTexturesSize(const Size& S);

		const Ogre::PixelFormat getPixelFormat(const RttType& Rtt) const;

		inline void setNumberOfChannels(const RttType& Rtt, const NumberOfChannels& NOC)
		{
			mRttOptions[Rtt].NumberOfChannels_ = NOC;

			if (getTexture(Rtt))
			{
				initialize(Rtt);
			}
		}

		inline const NumberOfChannels& getNumberOfChannels(const RttType& Rtt) const
		{
			return mRttOptions[Rtt].NumberOfChannels_;
		}

		inline void setBitsPerChannel(const RttType& Rtt, const BitsPerChannel& BPC)
		{
			mRttOptions[Rtt].BitsPerChannel_ = BPC;

			if (getTexture(Rtt))
			{
				initialize(Rtt);
			}
		}

		inline const BitsPerChannel& getBitsPerChannel(const RttType& Rtt) const
		{
			return mRttOptions[Rtt].BitsPerChannel_;
		}

		inline const RttOptions& getRttOptions(const RttType& Rtt) const
		{
			return mRttOptions[Rtt];
		}

		/** Range [0.05, ~2], increase if you experience reflection issues when the camera is near to the water.
	     */
		void setReflectionDisplacementError(const Ogre::Real& ReflectionDisplacementError)
		{
			mReflectionDisplacementError = ReflectionDisplacementError;
		}

		inline const Ogre::Real& getReflectionDisplacementError() const
		{
			return mReflectionDisplacementError;
		}

		inline void setDisableReflectionCustomNearCliplPlaneRenderQueues(const std::vector<Ogre::RenderQueueGroupID>& DisableReflectionCustomNearClipPlaneRenderQueues)
		{
			mDisableReflectionCustomNearClipPlaneRenderQueues = DisableReflectionCustomNearClipPlaneRenderQueues;
		}

		inline const std::vector<Ogre::RenderQueueGroupID>& getDisableReflectionCustomNearClipPlaneRenderQueues()
		{
			return mDisableReflectionCustomNearClipPlaneRenderQueues;
		}

		inline void addRttListener(RttListener *l)
		{
			mRttListeners.push_back(l);
		}

	private:
		/** RttManager::CRefractionListener class
		 */
		class CRefractionListener : public Ogre::RenderTargetListener
		{
		public:
			/// Rtt manager pointer
			RttManager* mRttManager;

			/// Camera.y - Plane.y difference
			Ogre::Real mCameraPlaneDiff;

			void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt) override;
			void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt) override;
		};

		/** RttManager::CReflectionListener class
		 */
		class CReflectionListener : public Ogre::RenderTargetListener
		{
		public:
			/** Used for avoid near clip plane clipping during the reflection Rtt
			 */
			class CReflectionQueueListener : public Ogre::RenderQueueListener
			{
			public:
				void renderQueueStarted(Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool &skipThisInvocation) override;
				void renderQueueEnded(Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool &skipThisInvocation) override;

			    RttManager* mRttManager;
				/// Is the reflection Rtt active?
				bool mActive;
			};

			/// CReflectionQueueListener
			CReflectionQueueListener mCReflectionQueueListener;

			/// Rtt manager pointer
			RttManager* mRttManager;

			/// Camera.y - Plane.y difference
			Ogre::Real mCameraPlaneDiff;

			void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt) override;
			void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt) override;
		};

		class CDepthListener : public Ogre::RenderTargetListener
		{
		public:
			/// Rtt manager pointer
			RttManager* mRttManager;

			/// std::string to store entity's original materials name
			std::queue<std::string> mMaterials;

			/// Camera.y - Plane.y difference
			Ogre::Real mCameraPlaneDiff;

			void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt) override;
			void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt) override;
		};

		class CDepthReflectionListener : public Ogre::RenderTargetListener
		{
		public:
			/// Rtt manager pointer
			RttManager* mRttManager;

			/// std::string to store entity's original materials name
			std::queue<std::string> mMaterials;

			/// Camera.y - Plane.y difference
			Ogre::Real mCameraPlaneDiff;

			void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt) override;
			void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt) override;
		};

		class CGPUNormalMapListener : public Ogre::RenderTargetListener
		{
		public:
			/// Rtt manager pointer
			RttManager* mRttManager;

			/// Old Hydrax mesh material name
			Ogre::String mOldMaterialName;

			void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt) override;
			void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt) override;
		};


		void _initializeRtt(const RttType& Rtt,                        const Ogre::Vector3& RttPlaneNormal,
			                const Ogre::ColourValue& BackgroundColour, const bool& RenderSky,
							Ogre::RenderTargetListener* RTListener,    const Ogre::String& MaterialScheme = "",
							const bool& ShadowsEnabled = true);

		inline const bool _isRenderQueueInList(const std::vector<Ogre::RenderQueueGroupID>& l, const Ogre::RenderQueueGroupID& rq)
		{
			for (std::vector<Ogre::RenderQueueGroupID>::const_iterator i = l.begin(); i != l.end(); i++)
			{
				if ((*i) == rq)
				{
					return true;
				}
			}

			return false;
		}

		/// Hydrax parent pointer
		Hydrax *mHydrax;

        /// SceneNode to attach our planes
        Ogre::SceneNode *mPlanesSceneNode;
		/// Rtt planes vector
		Ogre::MovablePlane *mPlanes[6];

		/// Rtt textures vector
		Ogre::TexturePtr mTextures[6];
		/// Rtt options
		RttOptions mRttOptions[6];

		/// Rtt listener objects
        CReflectionListener      mReflectionListener;
		CRefractionListener      mRefractionListener;
		CDepthListener           mDepthListener;
		CDepthReflectionListener mDepthReflectionListener;
		CGPUNormalMapListener    mGPUNormalMapListener;

		/// Render queues to exclude of the reflection custom near clip plane
		std::vector<Ogre::RenderQueueGroupID> mDisableReflectionCustomNearClipPlaneRenderQueues;
		/// Rtt listeners
		std::vector<RttListener*> mRttListeners;

		/// Reflection displacement error, range [0.01, ~2]
		Ogre::Real mReflectionDisplacementError;
	};
};

/// @} // addtogroup Hydrax
/// @} // addtogroup Gfx

#endif
