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

#include <Ogre.h>

namespace RoR {

/// @addtogroup Gfx
/// @{

/// A dynamic environment probe; Creates a cubemap with realtime reflections around specified point.
class GfxEnvmap
{
public:

    ~GfxEnvmap();

    bool envmap_show_terrain_objects = true; // When false, all .ODEF objects are hidden when rendering envmap.

    void SetupEnvMap();
    void UpdateEnvMap(Ogre::Vector3 center, GfxActor* gfx_actor, bool full = false);
    void CreateSceneNodes(); //!< Must be invoked repeatedly because we wipe the SceneManager after return to main menu.

    Ogre::RenderTarget* GetRenderTarget(int face) const
    {
        return m_render_targets[face];
    }

private:

    void SetupCameras(); //only needs to be done once
    void AdjustSceneBeforeRender(Ogre::Vector3 center, GfxActor* gfx_actor);
    void RestoreSceneAfterRender(Ogre::Vector3 center, GfxActor* gfx_actor);

    static const unsigned int NUM_FACES = 6;

    std::array<Ogre::Camera*, NUM_FACES> m_cameras;
    std::array<Ogre::SceneNode*, NUM_FACES> m_cameras_snode;
    std::array<Ogre::RenderTarget*, NUM_FACES> m_render_targets;
    Ogre::TexturePtr     m_rtt_texture;
    int                  m_update_round = 0; /// Render targets are updated one-by-one; this is the index of next target to update.
};

/// @} // addtogroup Gfx

} // namespace RoR
