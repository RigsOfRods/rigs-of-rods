/*
--------------------------------------------------------------------------------
This source file is part of Hydrax.
Visit ---

Copyright (C) 2008 Xavier Vergu�n Gonz�lez <xavierverguin@hotmail.com>
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

#include "Enums.h"
#include "Help.h"
#include "Prerequisites.h"

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
            virtual void preRenderTargetUpdate(const RttType &Rtt){};

            /** Funtion that is called after the Rtt will render
                @param Rtt Rtt type
                @remarks We've to override it
            */
            virtual void postRenderTargetUpdate(const RttType &Rtt){};
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
        void initialize(const RttType &Rtt);

        /** Removes a RTT
            @param Rtt Rtt to remove
         */
        void remove(const RttType &Rtt);

        /** Remove all RttManager resources
            @remarks After calling removeAll(), calling initialize(...) is allowed.
         */
        void removeAll();

        /** Get RTT texture name
            @param Rtt Rtt type
            @return Rtt texture name
         */
        inline const Ogre::String &getRttName(RttType Rtt) const
        {
            return mRttOptions[static_cast<int>(Rtt)].Name;
        }

        /** Get Rtt texture
            @param Rtt Rtt type
            @return Rtt texture
         */
        inline Ogre::TexturePtr getTexture(RttType Rtt)
        {
            return mTextures[static_cast<int>(Rtt)];
        }

        /** Get planes scene node
            @return Planes scene node
         */
        inline Ogre::SceneNode *getPlanesSceneNode()
        {
            return mPlanesSceneNode;
        }

        /** Get Rtt plane
            @param Rtt Rtt type
            @return Rtt plane
         */
        inline Ogre::MovablePlane *getPlane(RttType Rtt)
        {
            return mPlanes[static_cast<int>(Rtt)];
        }

        /** Get Rtt texture size
            @param Rtt Rtt type
            @return Rtt texture size
         */
        inline const Size &getTextureSize(const RttType &Rtt) const
        {
            return mRttOptions[static_cast<int>(Rtt)].Size_;
        }

        /** Set Rtt texture size
            @param Rtt Rtt type
            @param S New texture size (0,0 -> get main viewport size)
         */
        void setTextureSize(const RttType &Rtt, const Size &S);

        /** Set Rtt textures size
            @param S New texture size (0,0 -> get main viewport size)
         */
        void setTexturesSize(const Size &S);

        /** Get pixel format
            @param Rtt Rtt type
            @return Rtt pixel format
         */
        const Ogre::PixelFormat getPixelFormat(const RttType &Rtt) const;

        /** Set number of channels
            @Rtt Rtt type
            @NOC Number of channels
         */
        inline void setNumberOfChannels(const RttType &Rtt, const NumberOfChannels &NOC)
        {
            mRttOptions[Rtt].NumberOfChannels_ = NOC;

            if (!getTexture(Rtt).isNull()) { initialize(Rtt); }
        }

        /** Get number of channels
            @Rtt Rtt type
            @return Number of channels
         */
        inline const NumberOfChannels &getNumberOfChannels(const RttType &Rtt) const
        {
            return mRttOptions[Rtt].NumberOfChannels_;
        }

        /** Set bits per channel
            @Rtt Rtt type
            @BPC Bits per channel
         */
        inline void setBitsPerChannel(const RttType &Rtt, const BitsPerChannel &BPC)
        {
            mRttOptions[Rtt].BitsPerChannel_ = BPC;

            if (!getTexture(Rtt).isNull()) { initialize(Rtt); }
        }

        /** Get bits per channels
            @Rtt Rtt type
            @return Bits per channel
         */
        inline const BitsPerChannel &getBitsPerChannel(const RttType &Rtt) const
        {
            return mRttOptions[Rtt].BitsPerChannel_;
        }

        /** Get Rtt options
            @Rtt Rtt type
            @return RttOptions
         */
        inline const RttOptions &getRttOptions(const RttType &Rtt) const
        {
            return mRttOptions[Rtt];
        }

        /** Set reflection displacement error
            @param ReflectionDisplacementError Range [0.05, ~2], increase if you experiment
                   reflection issues when the camera is near to the water.
         */
        void setReflectionDisplacementError(const Ogre::Real &ReflectionDisplacementError)
        {
            mReflectionDisplacementError = ReflectionDisplacementError;
        }

        /** Get reflection displacement error
            @return Reflection displacement error
         */
        inline const Ogre::Real &getReflectionDisplacementError() const
        {
            return mReflectionDisplacementError;
        }

        /** Set disable reflection custom near clip plane render queues
            @param DisableReflectionCustomNearClipPlaneRenderQueues Disable reflection custom near clip plane render queues
         */
        inline void setDisableReflectionCustomNearCliplPlaneRenderQueues(
            const std::vector<Ogre::RenderQueueGroupID> &DisableReflectionCustomNearClipPlaneRenderQueues)
        {
            mDisableReflectionCustomNearClipPlaneRenderQueues = DisableReflectionCustomNearClipPlaneRenderQueues;
        }

        /** Get disable reflection custom near clip plane render queues
            @return Disable reflection custom near clip plane render queues
         */
        inline const std::vector<Ogre::RenderQueueGroupID> &getDisableReflectionCustomNearClipPlaneRenderQueues()
        {
            return mDisableReflectionCustomNearClipPlaneRenderQueues;
        }

        /** Add Rtt listener
            @param l Rtt listener
         */
        inline void addRttListener(RttListener *l)
        {
            mRttListeners.push_back(l);
        }

        /** Remove Rtt listener
            @param l Rtt listener to be removed
            @param releaseMemory delete Rtt listener pointer?
         */
        void removeRttListener(RttListener *l, const bool &releaseMemory = true);

        /** Remove all Rtt listeners
            @param releaseMemory delete Rtt listeners pointers?
         */
        void removeAllRttListeners(const bool &releaseMemory = true);

      private:
        /** RttManager::CRefractionListener class
         */
        class CRefractionListener : public Ogre::RenderTargetListener
        {
          public:
            /// Rtt manager pointer
            RttManager *mRttManager;

            /// Camera.y - Plane.y difference
            Ogre::Real mCameraPlaneDiff;

            /** Funtion that is called before the Rtt will render
                @param evt Ogre RenderTargetEvent
                @remarks We've to override it
             */
            void preRenderTargetUpdate(const Ogre::RenderTargetEvent &evt);

            /** Funtion that is called after the Rtt will render
            @param evt Ogre RenderTargetEvent
            @remarks We've to override it
            */
            void postRenderTargetUpdate(const Ogre::RenderTargetEvent &evt);
        };

        /** RttManager::CReflectionListener class
         */
        class CReflectionListener : public Ogre::RenderTargetListener
        {
          public:
            /** RttManager::CReflectionListener::CReflectionQueueListener class
                Used for avoid near clip plane clipping during the reflection Rtt
             */
            class CReflectionQueueListener : public Ogre::RenderQueueListener
            {
              public:
                /** Called at the start of the queue
                 */
                void renderQueueStarted(Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool &skipThisInvocation);

                /** Called on the end of the queue
                 */
                void renderQueueEnded(Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool &skipThisInvocation);

                /// Rtt manager pointer
                RttManager *mRttManager;
                /// Is the reflection Rtt active?
                bool mActive;
            };

            /// CReflectionQueueListener
            CReflectionQueueListener mCReflectionQueueListener;

            /// Rtt manager pointer
            RttManager *mRttManager;

            /// Camera.y - Plane.y difference
            Ogre::Real mCameraPlaneDiff;

            /** Funtion that is called before the Rtt will render
                @param evt Ogre RenderTargetEvent
                @remarks We've to override it
             */
            void preRenderTargetUpdate(const Ogre::RenderTargetEvent &evt);

            /** Funtion that is called after the Rtt will render
                @param evt Ogre RenderTargetEvent
                @remarks We've to override it
             */
            void postRenderTargetUpdate(const Ogre::RenderTargetEvent &evt);
        };

        /** RttManager::CDepthListener class
         */
        class CDepthListener : public Ogre::RenderTargetListener
        {
          public:
            /// Rtt manager pointer
            RttManager *mRttManager;

            /// std::string to store entity's original materials name
            std::queue<std::string> mMaterials;

            /// Camera.y - Plane.y difference
            Ogre::Real mCameraPlaneDiff;

            /** Funtion that is called before the Rtt will render
                @param evt Ogre RenderTargetEvent
                @remarks We've to override it
             */
            void preRenderTargetUpdate(const Ogre::RenderTargetEvent &evt);

            /** Funtion that is called after the Rtt will render
                @param evt Ogre RenderTargetEvent
                @remarks We've to override it
             */
            void postRenderTargetUpdate(const Ogre::RenderTargetEvent &evt);
        };

        /** RttManager::CDepthReflectionListener class
         */
        class CDepthReflectionListener : public Ogre::RenderTargetListener
        {
          public:
            /// Rtt manager pointer
            RttManager *mRttManager;

            /// std::string to store entity's original materials name
            std::queue<std::string> mMaterials;

            /// Camera.y - Plane.y difference
            Ogre::Real mCameraPlaneDiff;

            /** Funtion that is called before the Rtt will render
                @param evt Ogre RenderTargetEvent
                @remarks We've to override it
             */
            void preRenderTargetUpdate(const Ogre::RenderTargetEvent &evt);

            /** Funtion that is called after the Rtt will render
                @param evt Ogre RenderTargetEvent
                @remarks We've to override it
             */
            void postRenderTargetUpdate(const Ogre::RenderTargetEvent &evt);
        };

        /** RttManager::CGPUNormalMapListener class
         */
        class CGPUNormalMapListener : public Ogre::RenderTargetListener
        {
          public:
            /// Rtt manager pointer
            RttManager *mRttManager;

            /// Old Hydrax mesh material name
            Ogre::String mOldMaterialName;

            /** Funtion that is called before the Rtt will render
                @param evt Ogre RenderTargetEvent
                @remarks We've to override it
             */
            void preRenderTargetUpdate(const Ogre::RenderTargetEvent &evt);

            /** Funtion that is called after the Rtt will render
                @param evt Ogre RenderTargetEvent
                @remarks We've to override it
             */
            void postRenderTargetUpdate(const Ogre::RenderTargetEvent &evt);
        };

        /** Initialize the Rtt
            @param Rtt Rtt type
            @param PlaneNormal Rtt plane normal
            @param BackgroundColour Background colour
            @param RenderSky True for sky rendering and false for not
            @param RTListener Render target listener
            @param MaterialScheme Ogre::String("") for none
            @param ShadowsEnabled Enable shadows in this RTT
         */
        void _initializeRtt(const RttType &Rtt, const Ogre::Vector3 &PlaneNormal, const Ogre::ColourValue &BackgroundColour,
                            const bool &RenderSky, Ogre::RenderTargetListener *RTListener,
                            const Ogre::String &MaterialScheme = "", const bool &ShadowsEnabled = true);

        /** Find a render queue in a render queue list
            @param l Render queue list
            @param rq Render queue to find
            @return true If the render queue to find is in the list, false if not
         */
        inline const bool _isRenderQueueInList(const std::vector<Ogre::RenderQueueGroupID> &l, const Ogre::RenderQueueGroupID &rq)
        {
            for (std::vector<Ogre::RenderQueueGroupID>::const_iterator i = l.begin(); i != l.end(); i++)
            {
                if ((*i) == rq) { return true; }
            }

            return false;
        }

        /** Invoke Rtt Listeners
            @param Rtt Rtt type
            @param pre true for Pre render target update, false for Post render target update
        */
        void _invokeRttListeners(const RttType &Rtt, const bool &pre);

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
        std::vector<RttListener *> mRttListeners;

        /// Reflection displacement error, range [0.01, ~2]
        Ogre::Real mReflectionDisplacementError;
    };
}; // namespace Hydrax

#endif
