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

#pragma warning(disable:4267)

#include "RttManager.h"

#include "Hydrax.h"

#define _def_Hydrax_Reflection_Rtt_Name        "HydraxReflectionMap"
#define _def_Hydrax_Refraction_Rtt_Name        "HydraxRefractionMap"
#define _def_Hydrax_Depth_Rtt_Name             "HydraxDepthMap"
#define _def_Hydrax_Depth_Reflection_Rtt_Name  "HydraxDepthReflectionMap"
#define _def_Hydrax_API_Rtt_Name               "HydraxAPIMap"
#define _def_Hydrax_GPU_Normal_Map_Rtt_Name    "HydraxNormalMap"

namespace Hydrax
{
	RttManager::RttManager(Hydrax *h)
		: mHydrax(h)
		, mPlanesSceneNode(0)
		, mReflectionDisplacementError(0.5f)
	{
		Ogre::String RttNames[6] = 
		    {_def_Hydrax_Reflection_Rtt_Name,
			 _def_Hydrax_Refraction_Rtt_Name,
			 _def_Hydrax_Depth_Rtt_Name,
			 _def_Hydrax_Depth_Reflection_Rtt_Name,
			 _def_Hydrax_API_Rtt_Name,
		     _def_Hydrax_GPU_Normal_Map_Rtt_Name};

		for (int k = 0; k < 6; k++)
		{
			mPlanes[k] = static_cast<Ogre::MovablePlane*>(NULL);
			mTextures[k].reset();
			mRttOptions[k].Name  = RttNames[k];
			mRttOptions[k].Size_ = Size(0);
			mRttOptions[k].NumberOfChannels_ = NOC_3;
			mRttOptions[k].BitsPerChannel_ = BPC_8;
		}

		mReflectionListener.mRttManager = this;
		mReflectionListener.mCReflectionQueueListener.mRttManager = this;
		mReflectionListener.mCReflectionQueueListener.mActive = false;
		mRefractionListener.mRttManager = this;
		mDepthListener.mRttManager = this;
		mDepthReflectionListener.mRttManager = this;
		mDepthReflectionListener.mRttManager = this;
		mGPUNormalMapListener.mRttManager = this;
	}

	RttManager::~RttManager()
	{
		removeAll();
	}

	void RttManager::initialize(const RttType& Rtt)
	{
		if (!mPlanesSceneNode)
		{
			mPlanesSceneNode = mHydrax->getSceneManager()->getRootSceneNode()->createChildSceneNode();
			mHydrax->getSceneManager()->addRenderQueueListener(&mReflectionListener.mCReflectionQueueListener);
		}

		switch(Rtt)
		{
		    case RTT_REFLECTION:
			{
				const Ogre::Vector3& WC = mHydrax->getWaterColor();

				bool RenderSkyBox = !mHydrax->_isCurrentFrameUnderwater();

				_initializeRtt(Rtt, Ogre::Vector3::UNIT_Y, Ogre::ColourValue(WC.x, WC.y, WC.z), RenderSkyBox, &mReflectionListener);
			}
			break;

			case RTT_REFRACTION:
			{
				const Ogre::Vector3& WC = mHydrax->getWaterColor();

				bool RenderSkyBox = mHydrax->_isCurrentFrameUnderwater();

				_initializeRtt(Rtt, Ogre::Vector3::NEGATIVE_UNIT_Y, Ogre::ColourValue(WC.x, WC.y, WC.z), RenderSkyBox, &mRefractionListener);
			}
			break;

			case RTT_DEPTH:
			{
				_initializeRtt(Rtt, Ogre::Vector3::NEGATIVE_UNIT_Y, Ogre::ColourValue::Black, false, &mDepthListener, "HydraxDepth", false);
			}
			break;

			case RTT_DEPTH_REFLECTION:
			{
				_initializeRtt(Rtt, Ogre::Vector3::NEGATIVE_UNIT_Y, Ogre::ColourValue::Black, false, &mDepthReflectionListener, "HydraxDepth", false);
			}
			break;

			case RTT_GPU_NORMAL_MAP:
			{
				_initializeRtt(Rtt, Ogre::Vector3::NEGATIVE_UNIT_Y, Ogre::ColourValue(0.5,1,0.5), false, &mGPUNormalMapListener, "", false);
			}
			break;
		}
	}

	void RttManager::remove(const RttType& Rtt)
	{
		Ogre::TexturePtr &Tex = mTextures[Rtt];

		if (Tex)
		{
		    Ogre::RenderTarget* RT = Tex->getBuffer()->getRenderTarget();
            RT->removeAllListeners();
            RT->removeAllViewports();

			Ogre::TextureManager::getSingleton().remove(mRttOptions[Rtt].Name);
			Ogre::MeshManager::getSingleton().remove(mRttOptions[Rtt].Name + "ClipPlane");

			Tex.reset();
			delete mPlanes[Rtt];
			mPlanes[Rtt] = static_cast<Ogre::MovablePlane*>(NULL);
		}

		// Check it to avoid any possible problem(texture initializated by createTextureUnit(Name..))
		if (Ogre::TextureManager::getSingleton().resourceExists(mRttOptions[Rtt].Name))
		{
			Ogre::TextureManager::getSingleton().remove(mRttOptions[Rtt].Name);
		}
	}

	void RttManager::removeAll()
	{
		for (int k = 0; k < 6; k++)
		{
			remove(static_cast<RttType>(k));
		}

	    if (mPlanesSceneNode)
		{
			mPlanesSceneNode->detachAllObjects();
            mPlanesSceneNode->getParentSceneNode()->removeAndDestroyChild(mPlanesSceneNode);
			mPlanesSceneNode = 0;
		}
	}

	void RttManager::_initializeRtt(const RttType& Rtt,                        const Ogre::Vector3& PlaneNormal, 
			                        const Ogre::ColourValue& BackgroundColour, const bool& RenderSky, 
									Ogre::RenderTargetListener* RTListener,    const Ogre::String& MaterialScheme, 
									const bool& ShadowsEnabled)
	{
		remove(Rtt);

		mPlanes[Rtt] = new Ogre::MovablePlane(mRttOptions[Rtt].Name + "Plane");
		
		mPlanes[Rtt]->d = 0;
		mPlanes[Rtt]->normal = PlaneNormal;

		Ogre::MeshManager::getSingleton().
			createPlane(mRttOptions[Rtt].Name + "ClipPlane", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                        *mPlanes[Rtt],
                        mHydrax->getMesh()->getSize().Width,mHydrax->getMesh()->getSize().Height, // PG grid.. <-- look into
                        10,10, true, 1,5,5, Ogre::Vector3::UNIT_Z);

		mPlanes[Rtt]->setCastShadows(false);
		mPlanesSceneNode->attachObject(mPlanes[Rtt]);

		Size TSize = mRttOptions[Rtt].Size_;

		if (TSize.Width == 0 || TSize.Height == 0)
		{
			TSize.Width = mHydrax->getViewport()->getActualWidth();
			TSize.Height = mHydrax->getViewport()->getActualHeight();
		}

		mTextures[Rtt] = Ogre::TextureManager::getSingleton()
			.createManual(mRttOptions[Rtt].Name,
                          Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                          Ogre::TEX_TYPE_2D,
                          TSize.Width, 
						  TSize.Height, 
						  0,
						  getPixelFormat(Rtt),
                          Ogre::TU_RENDERTARGET);

        Ogre::RenderTarget* RT_Texture = mTextures[Rtt]->getBuffer()->getRenderTarget();
            
        Ogre::Viewport *RT_Texture_Viewport = RT_Texture->addViewport(mHydrax->getCamera());
        RT_Texture_Viewport->setClearEveryFrame(true);
        RT_Texture_Viewport->setBackgroundColour(BackgroundColour);
        RT_Texture_Viewport->setOverlaysEnabled(false);
		RT_Texture_Viewport->setShadowsEnabled(ShadowsEnabled);
		if (MaterialScheme != "")
		{
            RT_Texture_Viewport->setMaterialScheme(MaterialScheme);
		}
        RT_Texture_Viewport->setSkiesEnabled(RenderSky);

        RT_Texture->addListener(RTListener);
	}

	void RttManager::setTextureSize(const RttType& Rtt, const Size& S)
	{
		mRttOptions[static_cast<int>(Rtt)].Size_ = S;

		if (getTexture(Rtt))
		{
			initialize(Rtt);

			mHydrax->getMaterialManager()->reload(MaterialManager::MAT_WATER);

			if (mHydrax->isComponent(HYDRAX_COMPONENT_UNDERWATER))
			{
				mHydrax->getMaterialManager()->reload(MaterialManager::MAT_UNDERWATER);

				if (mHydrax->_isCurrentFrameUnderwater())
				{
					mHydrax->getMaterialManager()->reload(MaterialManager::MAT_UNDERWATER_COMPOSITOR);
				}
			}
		}
	}

	void RttManager::setTexturesSize(const Size& S)
	{
		bool ReloadMaterialsNeeded = false;

		for(int k = 0; k < 5; k++)
		{
			mRttOptions[static_cast<RttType>(k)].Size_ = S;

            Ogre::TexturePtr& Tex = mTextures[static_cast<RttType>(k)];
			if (Tex)
			{
				initialize(static_cast<RttType>(k));

				ReloadMaterialsNeeded = true;
			}
		}

		if (ReloadMaterialsNeeded)
		{
			mHydrax->getMaterialManager()->reload(MaterialManager::MAT_WATER);

			if (mHydrax->isComponent(HYDRAX_COMPONENT_UNDERWATER))
			{
				mHydrax->getMaterialManager()->reload(MaterialManager::MAT_UNDERWATER);

				if (mHydrax->_isCurrentFrameUnderwater())
				{
					mHydrax->getMaterialManager()->reload(MaterialManager::MAT_UNDERWATER_COMPOSITOR);
				}
			}
		}
	}

	const Ogre::PixelFormat RttManager::getPixelFormat(const RttType& Rtt) const
	{ 
		switch (mRttOptions[Rtt].NumberOfChannels_)
		{
			// One channel (R)
		    case NOC_1 :
			{
				switch (mRttOptions[Rtt].BitsPerChannel_)
				{
					// 8 bits
				    case BPC_8 :
					{
						return Ogre::PF_L8;
					}
					break;

					// 16 bits
				    case BPC_16 :
					{
						return Ogre::PF_FLOAT16_R;
					}
					break;

					// 32 bits
				    case BPC_32 :
					{
						return Ogre::PF_FLOAT32_R;
					}
					break;
				}
			}
			break;

			// Two channels (RG)
		    case NOC_2 :
			{
				switch (mRttOptions[Rtt].BitsPerChannel_)
				{
					// 8 bits
				    case BPC_8 :
					{
						//...
					}
					break;

					// 16 bits
				    case BPC_16 :
					{
						return Ogre::PF_FLOAT16_GR;
					}
					break;

					// 32 bits
				    case BPC_32 :
					{
						return Ogre::PF_FLOAT32_GR;
					}
					break;
				}
			}
			break;

			// Three channels (RGB)
		    case NOC_3 :
			{
				switch (mRttOptions[Rtt].BitsPerChannel_)
				{
					// 8 bits
				    case BPC_8 :
					{
						return Ogre::PF_B8G8R8;
					}
					break;

					// 16 bits
				    case BPC_16 :
					{
						return Ogre::PF_FLOAT16_RGB;
					}
					break;

					// 32 bits
				    case BPC_32 :
					{
						return Ogre::PF_FLOAT32_RGB;
					}
					break;
				}
			}
			break;

			// Four channels (RGBA)
		    case NOC_4 :
			{
				switch (mRttOptions[Rtt].BitsPerChannel_)
				{
					// 8 bits
				    case BPC_8 :
					{
						return Ogre::PF_B8G8R8A8;
					}
					break;

					// 16 bits
				    case BPC_16 :
					{
						return Ogre::PF_FLOAT16_RGBA;
					}
					break;

					// 32 bits
				    case BPC_32 :
					{
						return Ogre::PF_FLOAT32_RGBA;
					}
					break;
				}
			}
			break;

		}
	
		// >= than needed
		return Ogre::PF_FLOAT32_RGBA;
	}

	/// -------------------------------------------------------------------------------

	void RttManager::CReflectionListener::preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
    {
		Hydrax *mHydrax = mRttManager->mHydrax;

		mCReflectionQueueListener.mActive = true;

        mHydrax->getMesh()->getEntity()->setVisible(false);
        
		if (mHydrax->_isCurrentFrameUnderwater())
		{
            mRttManager->mPlanes[RTT_REFLECTION]->getParentNode()->translate(0,-mHydrax->getPlanesError(),0);
		}
		else
		{
			mRttManager->mPlanes[RTT_REFLECTION]->getParentNode()->translate(0,+mHydrax->getPlanesError(),0);
		}

        bool IsInUnderwaterError = false;

		// Underwater
		if ( mHydrax->_isCurrentFrameUnderwater() &&
			(mHydrax->getCamera()->getDerivedPosition().y > mRttManager->mPlanes[RTT_REFLECTION]->getParentNode()->getPosition().y))
		{
			mCameraPlaneDiff = 0;
			IsInUnderwaterError = true;
		}
		// Overwater
		else if ((!mHydrax->_isCurrentFrameUnderwater()) &&
			     (mHydrax->getCamera()->getDerivedPosition().y < mRttManager->mPlanes[RTT_REFLECTION]->getParentNode()->getPosition().y))
		{
			mCameraPlaneDiff = mRttManager->mPlanes[RTT_REFLECTION]->getParentNode()->getPosition().y-mHydrax->getCamera()->getDerivedPosition().y+mRttManager->mReflectionDisplacementError;
			mRttManager->mPlanes[RTT_REFLECTION]->getParentNode()->translate(0,-mCameraPlaneDiff,0);
		}
		else
		{
			mCameraPlaneDiff = 0;
		}

		if (mHydrax->isComponent(HYDRAX_COMPONENT_UNDERWATER))
		{
			if (mHydrax->_isCurrentFrameUnderwater())
			{
			    mRttManager->mPlanes[RTT_REFLECTION]->normal = -mRttManager->mPlanes[RTT_REFLECTION]->normal;
			}
		}

        mHydrax->getCamera()->enableReflection(mRttManager->mPlanes[RTT_REFLECTION]);

        if (IsInUnderwaterError)
		{
			mCReflectionQueueListener.mActive = false;
		}
		else
		{ 
			mHydrax->getCamera()->enableCustomNearClipPlane(mRttManager->mPlanes[RTT_REFLECTION]);
		}
    }

    void RttManager::CReflectionListener::postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
    {
		Hydrax *mHydrax = mRttManager->mHydrax;

        mHydrax->getMesh()->getEntity()->setVisible(true);

		if (mCameraPlaneDiff != 0)
		{
			mRttManager->mPlanes[RTT_REFLECTION]->getParentNode()->translate(0,mCameraPlaneDiff,0);
		}

        if (mHydrax->_isCurrentFrameUnderwater())
		{
            mRttManager->mPlanes[RTT_REFLECTION]->getParentNode()->translate(0,mHydrax->getPlanesError(),0);
		}
		else
		{
			mRttManager->mPlanes[RTT_REFLECTION]->getParentNode()->translate(0,-mHydrax->getPlanesError(),0);
		}

        mHydrax->getCamera()->disableReflection();
        mHydrax->getCamera()->disableCustomNearClipPlane();

		if (mHydrax->isComponent(HYDRAX_COMPONENT_UNDERWATER))
		{
			if (mHydrax->_isCurrentFrameUnderwater())
			{
			    mRttManager->mPlanes[RTT_REFLECTION]->normal = -mRttManager->mPlanes[RTT_REFLECTION]->normal;
			}
		}

		mCReflectionQueueListener.mActive = false;
    }

	void RttManager::CReflectionListener::CReflectionQueueListener::renderQueueStarted(Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool

		&skipThisInvocation)
	{
		if ((queueGroupId == Ogre::RENDER_QUEUE_SKIES_EARLY || queueGroupId == Ogre::RENDER_QUEUE_SKIES_LATE || queueGroupId ==

			(Ogre::RENDER_QUEUE_SKIES_EARLY + 2))
			&& mActive)
		{
			mRttManager->mHydrax->getCamera()->disableCustomNearClipPlane();
			Ogre::Root::getSingleton().getRenderSystem()->_setProjectionMatrix(mRttManager->mHydrax->getCamera()->getProjectionMatrix());
		}
	}

	void RttManager::CReflectionListener::CReflectionQueueListener::renderQueueEnded(Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool

		&skipThisInvocation)
	{
		if ((queueGroupId == Ogre::RENDER_QUEUE_SKIES_EARLY || queueGroupId == Ogre::RENDER_QUEUE_SKIES_LATE || queueGroupId ==

			(Ogre::RENDER_QUEUE_SKIES_EARLY + 2))
			&& mActive)
		{
			mRttManager->mHydrax->getCamera()->enableCustomNearClipPlane(mRttManager->mPlanes[RTT_REFLECTION]);
			Ogre::Root::getSingleton().getRenderSystem()->_setProjectionMatrix(mRttManager->mHydrax->getCamera()->getProjectionMatrix());
		}
	}

    void RttManager::CRefractionListener::preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
    {
		Hydrax *mHydrax = mRttManager->mHydrax;

        mHydrax->getMesh()->getEntity()->setVisible(false);
		
		if (Ogre::Math::Abs(mHydrax->getPosition().y - mHydrax->getCamera()->getDerivedPosition().y) > mHydrax->getPlanesError())
		{
			if (mHydrax->_isCurrentFrameUnderwater())
		    {
				mRttManager->mPlanes[RTT_REFRACTION]->normal = -mRttManager->mPlanes[RTT_REFRACTION]->normal;
                mRttManager->mPlanes[RTT_REFRACTION]->getParentNode()->translate(0,-mHydrax->getPlanesError(),0);
		    }
		    else
		    {
			    mRttManager->mPlanes[RTT_REFRACTION]->getParentNode()->translate(0,mHydrax->getPlanesError(),0);
		    }   

            mHydrax->getCamera()->enableCustomNearClipPlane(mRttManager->mPlanes[RTT_REFRACTION]);
		}
    }

	void RttManager::CRefractionListener::postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
    {
		Hydrax *mHydrax = mRttManager->mHydrax;

        mHydrax->getMesh()->getEntity()->setVisible(true);

		if (Ogre::Math::Abs(mHydrax->getPosition().y - mHydrax->getCamera()->getDerivedPosition().y) > mHydrax->getPlanesError())
		{
			if (mHydrax->_isCurrentFrameUnderwater())
		    {
				mRttManager->mPlanes[RTT_REFRACTION]->normal = -mRttManager->mPlanes[RTT_REFRACTION]->normal;
                mRttManager->mPlanes[RTT_REFRACTION]->getParentNode()->translate(0,+mHydrax->getPlanesError(),0);
		    }
		    else
		    {
			    mRttManager->mPlanes[RTT_REFRACTION]->getParentNode()->translate(0,-mHydrax->getPlanesError(),0);
		    }   

            mHydrax->getCamera()->disableCustomNearClipPlane();
		}
    }

    void RttManager::CDepthListener::preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
    {
		Hydrax *mHydrax = mRttManager->mHydrax;

        Ogre::SceneManager::MovableObjectIterator EntityIterator = 
			mHydrax->getSceneManager()->getMovableObjectIterator("Entity");
        Ogre::Entity* CurrentEntity;
		unsigned int k;

        while (EntityIterator.hasMoreElements())
        {
            CurrentEntity = static_cast<Ogre::Entity*>(EntityIterator.peekNextValue());

			for(k = 0; k < CurrentEntity->getNumSubEntities(); k++)
			{
				mMaterials.push(CurrentEntity->getSubEntity(k)->getMaterialName());

			    CurrentEntity->getSubEntity(k)->setMaterialName(mHydrax->getMaterialManager()->getMaterial(MaterialManager::MAT_DEPTH)->getName());
			}

            EntityIterator.moveNext();
        }

		if (Ogre::Math::Abs(mHydrax->getPosition().y - mHydrax->getCamera()->getDerivedPosition().y) > mHydrax->getPlanesError())
		{
			if (mHydrax->_isCurrentFrameUnderwater())
		    {
				mRttManager->mPlanes[RTT_DEPTH]->normal = -mRttManager->mPlanes[RTT_DEPTH]->normal;
                mRttManager->mPlanes[RTT_DEPTH]->getParentNode()->translate(0,-mHydrax->getPlanesError(),0);
		    }
		    else
		    {
			    mRttManager->mPlanes[RTT_DEPTH]->getParentNode()->translate(0,mHydrax->getPlanesError(),0);
				mHydrax->getCamera()->enableCustomNearClipPlane(mRttManager->mPlanes[RTT_DEPTH]);
		    }   
		}

		if (mHydrax->_isCurrentFrameUnderwater())
		{
            mHydrax->getMesh()->getEntity()->setVisible(true);
			mHydrax->getMesh()->getEntity()->setMaterialName(
				mHydrax->getMaterialManager()->getMaterial(MaterialManager::MAT_SIMPLE_RED)->
				     getName());
			mHydrax->getMesh()->getEntity()->setRenderQueueGroup(Ogre::RENDER_QUEUE_SKIES_EARLY);
			mHydrax->getGodRaysManager()->setVisible(true);
		}
		else
		{
			mHydrax->getMesh()->getEntity()->setVisible(false);
		}
    }

    void RttManager::CDepthListener::postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
    {
		Hydrax *mHydrax = mRttManager->mHydrax;

        Ogre::SceneManager::MovableObjectIterator EntityIterator = 
			mHydrax->getSceneManager()->getMovableObjectIterator("Entity");
        Ogre::Entity* CurrentEntity;
		unsigned int k;

        while (EntityIterator.hasMoreElements())
        {
			CurrentEntity = static_cast<Ogre::Entity*>(EntityIterator.peekNextValue());

			for(k = 0; k < CurrentEntity->getNumSubEntities(); k++)
			{
			    CurrentEntity->getSubEntity(k)->setMaterialName(mMaterials.front());

				mMaterials.pop();
			}

            EntityIterator.moveNext();
        }

        mHydrax->getMesh()->getEntity()->setVisible(true);
		mHydrax->getGodRaysManager()->setVisible(false);
		mHydrax->getMesh()->getEntity()->setRenderQueueGroup(Ogre::RENDER_QUEUE_1);

        if (Ogre::Math::Abs(mHydrax->getPosition().y - mHydrax->getCamera()->getDerivedPosition().y) > mHydrax->getPlanesError())
		{
			if (mHydrax->_isCurrentFrameUnderwater())
		    {
				mRttManager->mPlanes[RTT_DEPTH]->normal = -mRttManager->mPlanes[RTT_DEPTH]->normal;
                mRttManager->mPlanes[RTT_DEPTH]->getParentNode()->translate(0,+mHydrax->getPlanesError(),0);
		    }
		    else
		    {
			    mRttManager->mPlanes[RTT_DEPTH]->getParentNode()->translate(0,-mHydrax->getPlanesError(),0);
		    }   

            mHydrax->getCamera()->disableCustomNearClipPlane();
		}
    }

	void RttManager::CDepthReflectionListener::preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
    {
		Hydrax *mHydrax = mRttManager->mHydrax;

		mHydrax->getMesh()->getEntity()->setVisible(false);

        Ogre::SceneManager::MovableObjectIterator EntityIterator = 
			mHydrax->getSceneManager()->getMovableObjectIterator("Entity");
        Ogre::Entity* CurrentEntity;
		unsigned int k;

        while (EntityIterator.hasMoreElements())
        {
            CurrentEntity = static_cast<Ogre::Entity*>(EntityIterator.peekNextValue());

			for(k = 0; k < CurrentEntity->getNumSubEntities(); k++)
			{
				mMaterials.push(CurrentEntity->getSubEntity(k)->getMaterialName());

			    CurrentEntity->getSubEntity(k)->setMaterialName(mHydrax->getMaterialManager()->getMaterial(MaterialManager::MAT_DEPTH)->getName());
			}

            EntityIterator.moveNext();
        }

        mRttManager->mPlanes[RTT_DEPTH_REFLECTION]->getParentNode()->translate(0,-mHydrax->getPlanesError(),0);
		
        bool IsInUnderwaterError = false;

		if (mHydrax->getCamera()->getDerivedPosition().y > mRttManager->mPlanes[RTT_DEPTH_REFLECTION]->getParentNode()->getPosition().y)
		{
			mCameraPlaneDiff = 0;
			IsInUnderwaterError = true;
		}
		else
		{
			mCameraPlaneDiff = 0;
		}

        mHydrax->getCamera()->enableReflection(mRttManager->mPlanes[RTT_DEPTH_REFLECTION]);

		if (!IsInUnderwaterError)
		{
            mHydrax->getCamera()->enableCustomNearClipPlane(mRttManager->mPlanes[RTT_DEPTH_REFLECTION]);
		}
    }

	void RttManager::CDepthReflectionListener::postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
    {
		Hydrax *mHydrax = mRttManager->mHydrax;

        Ogre::SceneManager::MovableObjectIterator EntityIterator = 
			mHydrax->getSceneManager()->getMovableObjectIterator("Entity");
        Ogre::Entity* CurrentEntity;
		unsigned int k;

        while (EntityIterator.hasMoreElements())
        {
			CurrentEntity = static_cast<Ogre::Entity*>(EntityIterator.peekNextValue());

			for(k = 0; k < CurrentEntity->getNumSubEntities(); k++)
			{
			    CurrentEntity->getSubEntity(k)->setMaterialName(mMaterials.front());

				mMaterials.pop();
			}

            EntityIterator.moveNext();
        }

        mHydrax->getMesh()->getEntity()->setVisible(true);

		if (mCameraPlaneDiff != 0)
		{
			mRttManager->mPlanes[RTT_DEPTH_REFLECTION]->getParentNode()->translate(0,mCameraPlaneDiff,0);
		}
		
        mRttManager->mPlanes[RTT_DEPTH_REFLECTION]->getParentNode()->translate(0,mHydrax->getPlanesError(),0);

        mHydrax->getCamera()->disableReflection();
        mHydrax->getCamera()->disableCustomNearClipPlane();
    }

	void RttManager::CGPUNormalMapListener::preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
    {
		mOldMaterialName = mRttManager->mHydrax->getMesh()->getMaterialName();
		mRttManager->mHydrax->getMesh()->setMaterialName(mRttManager->mHydrax->getGPUNormalMapManager()->getNormalMapMaterial()->getName());

		Ogre::SceneManager *mSceneMgr = mRttManager->mHydrax->getSceneManager();

		// Render only the Hydrax mesh(Ogre::RENDER_QUEUE_1)
		mSceneMgr->clearSpecialCaseRenderQueues();
        mSceneMgr->addSpecialCaseRenderQueue(Ogre::RENDER_QUEUE_1);
        mSceneMgr->setSpecialCaseRenderQueueMode(Ogre::SceneManager::SCRQM_INCLUDE);
    }

	void RttManager::CGPUNormalMapListener::postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
    {
		mRttManager->mHydrax->getMesh()->setMaterialName(mOldMaterialName);

		Ogre::SceneManager *mSceneMgr = mRttManager->mHydrax->getSceneManager();

		// Render all
		mSceneMgr->clearSpecialCaseRenderQueues();
		mSceneMgr->setSpecialCaseRenderQueueMode(Ogre::SceneManager::SCRQM_EXCLUDE); 
    }
}
