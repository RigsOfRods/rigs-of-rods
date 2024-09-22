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

#include <TextureManager.h>

#include <Hydrax.h>

namespace Hydrax
{
	TextureManager::TextureManager(Hydrax *h)
		: mCreated(false)
		, mHydrax(h)
	{
		for (int k = 0; k < 1; k++)
		{
			mTextures[k].reset();
		}

		mTextureNames[0] = "HydraxNormalMap";
	}

	TextureManager::~TextureManager()
	{
		remove();
	}

	void TextureManager::create(const Size &Size)
	{
		remove();

		for (int k = 0; k < 1; k++)
		{
		     _createTexture(mTextures[k], mTextureNames[k], Size);
		}

		mHydrax->getMaterialManager()->reload(MaterialManager::MAT_WATER);

		mCreated = true;
	}

	void TextureManager::remove()
	{
		if (!mCreated)
		{
			return;
		}

		for (int k = 0; k < 1; k++)
		{
		     Ogre::TextureManager::getSingleton().remove(mTextureNames[k]);
			 mTextures[k].reset();
		}

		mCreated = false;
	}

	bool TextureManager::_updateNormalMap(Image &Image)
	{
		if (!mCreated)
		{
			HydraxLOG("Error in TextureManager::_updateNormalMap, create() does not called.");

			return false;
		}

		if (Image.getType() != Image::TYPE_RGB)
		{
			HydraxLOG("Error in TextureManager::_updateNormalMap, Image type isn't correct.");

			return false;
		}

		Ogre::TexturePtr &Texture = getTexture(TEX_NORMAL_ID);

		Size ImageSize = Image.getSize();

		if (Texture->getWidth()  != ImageSize.Width ||
			Texture->getHeight() != ImageSize.Height)
		{
			HydraxLOG("Error in TextureManager::update, Update size doesn't correspond to " + getTextureName(TEX_NORMAL_ID) + " texture size");

			return false;
		}

		Ogre::HardwarePixelBufferSharedPtr pixelBuffer = Texture->getBuffer();

        pixelBuffer->lock(Ogre::HardwareBuffer::HBL_NORMAL);
        const Ogre::PixelBox& pixelBox = pixelBuffer->getCurrentLock();

        Ogre::uint8* pDest = static_cast<Ogre::uint8*>(pixelBox.data);

        int x, y;

        for (x = 0; x < ImageSize.Width; x++)
        {
            for (y = 0; y < ImageSize.Height; y++)
            {
                *pDest++ =   Image.getValue(x,y,2); // B
                *pDest++ =   Image.getValue(x,y,1); // G
                *pDest++ =   Image.getValue(x,y,0); // R
				*pDest++ =   255;                   // A
            }
        }

        pixelBuffer->unlock();

		return true;
	}

	bool TextureManager::_createTexture(Ogre::TexturePtr &Texture, const Ogre::String &Name, const Size &Size)
	{
		try
		{
			Ogre::TextureManager::getSingleton().remove(Name);

			Texture = Ogre::TextureManager::getSingleton().
				createManual(Name,
				Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
				Ogre::TEX_TYPE_2D,
				Size.Width, Size.Height,
				0,
				Ogre::PF_BYTE_BGRA,
				Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);

			Texture->load();
		}
		catch (Ogre::Exception &e)
		{
			HydraxLOG(e.getFullDescription());

			return false;
		}

		return true;
	}
}
