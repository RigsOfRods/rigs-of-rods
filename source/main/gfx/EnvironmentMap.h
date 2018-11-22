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

#include "ForwardDeclarations.h"

#include <OgreVector3.h>
#include <OgreTexture.h>

namespace RoR {

/// A dynamic environment probe; Creates a cubemap with realtime reflections around specified point.
class GfxEnvmap
{
public:

    GfxEnvmap();
    ~GfxEnvmap();

    void SetupEnvMap();
    void UpdateEnvMap(Ogre::Vector3 center, Actor* beam = 0);

private:

    static const unsigned int NUM_FACES = 6;

    void InitEnvMap(Ogre::Vector3 center);

    Ogre::Camera*        m_cameras[NUM_FACES];
    Ogre::RenderTarget*  m_render_targets[NUM_FACES];
    Ogre::TexturePtr     m_rtt_texture;
    bool                 m_is_initialized;
    int                  m_update_round; /// Render targets are updated one-by-one; this is the index of next target to update.
};

} // namespace RoR
