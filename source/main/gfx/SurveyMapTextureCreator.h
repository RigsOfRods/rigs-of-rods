/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "Application.h"
#include <OgreRenderTargetListener.h>

namespace RoR {

class SurveyMapTextureCreator : public Ogre::RenderTargetListener
{
public:

    SurveyMapTextureCreator(Ogre::Real terrain_height);
    ~SurveyMapTextureCreator();

    bool init(int res, int fsaa);
    void update(Ogre::Vector2 center, Ogre::Vector2 size);

    Ogre::TexturePtr GetTexture() const { return mTexture; }

protected:

    void preRenderTargetUpdate(const Ogre::RenderTargetEvent &evt);
    void postRenderTargetUpdate(const Ogre::RenderTargetEvent &evt);

    Ogre::Real mTerrainHeight;
    Ogre::String mTextureName;

    Ogre::Camera* mCamera;
    Ogre::TexturePtr mTexture;
    Ogre::RenderTarget* mRttTex;
};

} // namespace RoR
