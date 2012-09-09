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

#include <DecalsManager.h>

#include <Hydrax.h>

namespace Hydrax
{
	Decal::Decal(Hydrax *h, const Ogre::String &TextureName, const int& Id)
		: mHydrax(h)
		, mTextureName(TextureName)
		, mId(Id)
		, mRegisteredPass(0)
		, mPosition(Ogre::Vector2(0,0))
		, mSize(Ogre::Vector2(1,1))
		, mOrientation(Ogre::Radian(0))
		, mTransparency(1)
		, mVisible(true)
	{
		mProjector = new Ogre::Frustum();
		mProjector->setProjectionType(Ogre::PT_ORTHOGRAPHIC); 
		
		mSceneNode = mHydrax->getSceneManager()->getRootSceneNode()->createChildSceneNode();
        mSceneNode->attachObject(mProjector);
		mSceneNode->setPosition(Ogre::Vector3(0,0,0));
        mSceneNode->setOrientation(Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::NEGATIVE_UNIT_X)); 

		setPosition(mPosition);
		setSize(mSize);
		setOrientation(mOrientation);
	}

	Decal::~Decal()
	{
		unregister();
	
		mSceneNode->getParentSceneNode()->removeAndDestroyChild(mSceneNode->getName());

		delete mProjector;
	}

	void Decal::registerPass(Ogre::Pass* _Pass)
	{
		unregister();

		_Pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
		_Pass->setCullingMode(Ogre::CULL_NONE);
        _Pass->setDepthBias(1,1);
        _Pass->setLightingEnabled(false);
		_Pass->setDepthWriteEnabled(false);

		Ogre::TextureUnitState *DecalTexture = _Pass->createTextureUnitState(mTextureName);
        DecalTexture->setProjectiveTexturing(true, mProjector);
		DecalTexture->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);
		DecalTexture->setTextureFiltering(Ogre::FO_LINEAR, Ogre::FO_LINEAR, Ogre::FO_NONE);
		DecalTexture->setAlphaOperation(Ogre::LBX_MODULATE, Ogre::LBS_TEXTURE, Ogre::LBS_MANUAL, 1.0, mTransparency); 

		mRegisteredPass = _Pass;
	}

	void Decal::unregister()
	{
		if (mRegisteredPass)
		{
			mRegisteredPass->getParent()->removePass(mRegisteredPass->getIndex());
			mRegisteredPass = static_cast<Ogre::Pass*>(NULL);
		}
	}

	void Decal::setPosition(const Ogre::Vector2& Position)
	{
		mPosition = Position;

		mSceneNode->setPosition(Position.x, 0, Position.y);
	}

	void Decal::setSize(const Ogre::Vector2& Size)
	{
		mSize = Size;

		// This method is only available in the CVS HEAD,
		// if you have problems compiling, just comment the
		// following line:
		mProjector->setOrthoWindow(Size.x, Size.y);
	}

	void Decal::setOrientation(const Ogre::Radian& Orientation)
	{
		mSceneNode->rotate(Ogre::Vector3::UNIT_Z, -mOrientation + Orientation);

		mOrientation = Orientation;
	}

	void Decal::setTransparency(const Ogre::Real& Transparency)
	{
		mTransparency = Transparency;

		if (mRegisteredPass)
		{
			mRegisteredPass->getTextureUnitState(0)
				->setAlphaOperation(Ogre::LBX_MODULATE, Ogre::LBS_TEXTURE, Ogre::LBS_MANUAL, 1.0, mTransparency);
		}
	}

	void Decal::setVisible(const bool& Visible)
	{
		mVisible = Visible;

		unregister();

		mHydrax->getDecalsManager()->_forceToUpdate();
	}

	// --------------------------------------------------------------------

	DecalsManager::DecalsManager(Hydrax *h)
		: mHydrax(h)
		, mNextId(0)
		, mLastUnderwater(false)
		, mWaterStrength(5)
		, mForceToUpdate(false)
	{
	}

	DecalsManager::~DecalsManager()
	{
		for(DecalIt = mDecals.begin(); DecalIt != mDecals.end(); DecalIt++)
        {
			delete (*DecalIt);
		}

		mDecals.clear();
	}

	void DecalsManager::update()
	{
		if (mHydrax->getCamera()->getDerivedPosition()    == mLastPosition &&
			mHydrax->getCamera()->getDerivedOrientation() == mLastOrientation && 
			!mForceToUpdate)
		{
			return;
		}

		if (mForceToUpdate)
		{
			mForceToUpdate = false;
		}

		Ogre::Vector2 DPos;
		Ogre::Real    HHeight = mHydrax->getPosition().y;
		Ogre::Vector2 DSize;
		Ogre::AxisAlignedBox DecalBox;

		if (mLastUnderwater != mHydrax->_isCurrentFrameUnderwater())
		{
			for(DecalIt = mDecals.begin(); DecalIt != mDecals.end(); DecalIt++)
            {
				if (!(*DecalIt)->isVisible())
				{
					continue;
				}

				if ((*DecalIt)->getRegisteredPass())
				{
					(*DecalIt)->unregister();

					if (!mHydrax->_isCurrentFrameUnderwater())
					{
					    (*DecalIt)->registerPass(
				             mHydrax->getMaterialManager()->getMaterial(MaterialManager::MAT_WATER)->
				                 getTechnique(0)->createPass());
					}
					else
					{
						(*DecalIt)->registerPass(
				             mHydrax->getMaterialManager()->getMaterial(MaterialManager::MAT_UNDERWATER)->
				                 getTechnique(0)->createPass());
					}
				}

			}
		}

		for(DecalIt = mDecals.begin(); DecalIt != mDecals.end(); DecalIt++)
        {
			if (!(*DecalIt)->isVisible())
			{
				continue;
			}

			DPos = (*DecalIt)->getPosition();
			DSize = (*DecalIt)->getSize()/2;

			DecalBox = Ogre::AxisAlignedBox(DPos.x - DSize.x,  HHeight - mWaterStrength, DPos.y - DSize.y,
				                            DPos.x + DSize.x,  HHeight + mWaterStrength, DPos.y + DSize.y);

			if (mHydrax->getCamera()->isVisible(DecalBox))
			{
				if (!(*DecalIt)->getRegisteredPass())
				{
					if (!mHydrax->_isCurrentFrameUnderwater())
					{
					    (*DecalIt)->registerPass(
				             mHydrax->getMaterialManager()->getMaterial(MaterialManager::MAT_WATER)->
				                 getTechnique(0)->createPass());
					}
					else
					{
						(*DecalIt)->registerPass(
				             mHydrax->getMaterialManager()->getMaterial(MaterialManager::MAT_UNDERWATER)->
				                 getTechnique(0)->createPass());
					}
				}
			}
			else
			{
				(*DecalIt)->unregister();
			}
		}
		
		mLastPosition    = mHydrax->getCamera()->getDerivedPosition();
		mLastOrientation = mHydrax->getCamera()->getDerivedOrientation();
		mLastUnderwater  = mHydrax->_isCurrentFrameUnderwater();
	}

	Decal* DecalsManager::add(const Ogre::String& TextureName)
	{
		Decal* NewDecal = new Decal(mHydrax, TextureName, mNextId);

		mDecals.push_back(NewDecal);

		if (mHydrax->getMaterialManager()->isCreated())
		{
			NewDecal->registerPass(
				mHydrax->getMaterialManager()->getMaterial(MaterialManager::MAT_WATER)->
				     getTechnique(0)->createPass());
		}

		mNextId++;

		return NewDecal;
	}

	Decal* DecalsManager::get(const int& Id)
	{
		for(DecalIt = mDecals.begin(); DecalIt != mDecals.end(); DecalIt++)
        {
            if((*DecalIt)->getId() == Id)
            {
				return (*DecalIt);
            }
		}

		return static_cast<Decal*>(NULL);
	}

	void DecalsManager::remove(const int& Id)
	{
		for(DecalIt = mDecals.begin(); DecalIt != mDecals.end(); DecalIt++)
        {
            if((*DecalIt)->getId() == Id)
            {
				delete (*DecalIt);
				mDecals.erase(DecalIt);

				return;
            }
		}
	}

	void DecalsManager::removeAll()
	{
		for(DecalIt = mDecals.begin(); DecalIt != mDecals.end(); DecalIt++)
        {
			delete (*DecalIt);
		}

		mDecals.clear();

		mNextId = 0;
	}

	void DecalsManager::registerAll()
	{
		for(DecalIt = mDecals.begin(); DecalIt != mDecals.end(); DecalIt++)
        {
			(*DecalIt)->unregister();

			if (!mHydrax->_isCurrentFrameUnderwater())
			{
				(*DecalIt)->registerPass(
					mHydrax->getMaterialManager()->getMaterial(MaterialManager::MAT_WATER)->
					   getTechnique(0)->createPass());
			}
			else
			{
				(*DecalIt)->registerPass(
					mHydrax->getMaterialManager()->getMaterial(MaterialManager::MAT_UNDERWATER)->
					   getTechnique(0)->createPass());
			}
		}
	}
}
