/*
	Copyright (c) 2010 ASTRE Henri (http://www.visual-experiments.com)

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
*/

#include "DynamicTexture.h"

#include <OgreHardwarePixelBuffer.h>
#include <OgreTextureManager.h>

using namespace Ogre;

DynamicTexture::DynamicTexture(const std::string& _textureName, unsigned int _width, unsigned int _height, bool _enableAlpha, int _mipmaps)
{
	mName    = _textureName;
	mWidth   = _width;
	mHeight  = _height;
	mFormat  = _enableAlpha ? Ogre::PF_A8B8G8R8: Ogre::PF_B8G8R8;
	mMipMaps = _mipmaps;

	createTexture();
}
DynamicTexture::~DynamicTexture()
{
	Ogre::TextureManager::getSingleton().remove(mTexture->getName());
	mTexture.setNull();
}

void DynamicTexture::update(const Ogre::PixelBox& _box)
{
	HardwarePixelBufferSharedPtr pixelBuffer = mTexture->getBuffer();
	pixelBuffer->blitFromMemory(_box);
}

void DynamicTexture::createTexture(void)
{
	mTexture = TextureManager::getSingleton().createManual(
		mName, 
		ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, 
		TEX_TYPE_2D, 
		mWidth, 
		mHeight, 
		0, //mMipMaps, 
		mFormat, 
		TU_DYNAMIC_WRITE_ONLY_DISCARDABLE,
		this);
}