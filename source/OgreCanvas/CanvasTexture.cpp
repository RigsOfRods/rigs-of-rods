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

#include "CanvasTexture.h"

#include <core/SkBitmap.h>

using namespace Ogre;
using namespace Canvas;

Canvas::Texture::Texture(const std::string& _textureName, unsigned int _width, unsigned int _height, bool _enableAlpha, int _mipmaps)
: DynamicTexture(_textureName, _width, _height, _enableAlpha, _mipmaps)
{
	mContext = new Context(_width, _height, _enableAlpha);
	mPixelBox = Ogre::PixelBox(_width, _height, 1, mFormat, mContext->getData());
}

Canvas::Texture::~Texture()
{
	delete mContext;
	mContext = NULL;
}

void Canvas::Texture::loadResource(Ogre::Resource* resource)
{
	Ogre::HardwarePixelBufferSharedPtr pixelBuffer = ((Ogre::Texture*) resource)->getBuffer();	
	if (!pixelBuffer.isNull()) 
		uploadTexture();
}

Context* Canvas::Texture::getContext()
{
	return mContext;
}

void Canvas::Texture::uploadTexture()
{
	update(mPixelBox);
}
void Canvas::Texture::createMaterial()
{
	Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().create(mTexture->getName(), "General");

	Pass* pass = material->getTechnique(0)->getPass(0);
	pass->setDepthCheckEnabled(false);
	pass->setDepthWriteEnabled(false);
	pass->setLightingEnabled(false);	
	pass->setSceneBlending(SBT_TRANSPARENT_ALPHA);

	TextureUnitState* texUnit = pass->createTextureUnitState(mTexture->getName());
	texUnit->setTextureFiltering(FO_NONE, FO_NONE, FO_NONE);
}

void Canvas::Texture::deleteMaterial()
{
	Ogre::MaterialManager::getSingleton().remove(mTexture->getName());
}