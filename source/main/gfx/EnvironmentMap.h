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
#include <OgreOverlay.h>

namespace RoR {

/// @addtogroup Gfx
/// @{

/// A dynamic environment probe; Creates a cubemap with realtime reflections around specified point.
class GfxEnvmap
{
public:

    ~GfxEnvmap();

    // Public envmap live options - they reload automatically every frame
    bool envmap_show_terrain_objects = true;
    float envmap_camera_nearclip_distance = 0.1f;
    float envmap_camera_farclip_distance = 0.0f; // (0 = infinite)
    float evmap_diag_overlay_pos_x = -0.8f;
    float evmap_diag_overlay_pos_y = -0.5f;
    float evmap_diag_overlay_scale = 0.35f;

    void SetupEnvMap();
    void UpdateEnvMap(Ogre::Vector3 center, GfxActor* gfx_actor, bool full = false);

    Ogre::RenderTarget* GetRenderTarget(int face) const
    {
        return m_render_targets[face];
    }

private:

    void SetupCameras(); //only needs to be done once
    void SetupDebugOverlay(); // only needs to be done once
    void SetDebugOverlayVisible(bool show);
    void AdjustSceneBeforeRender(Ogre::Vector3 center, GfxActor* gfx_actor);
    void RestoreSceneAfterRender(Ogre::Vector3 center, GfxActor* gfx_actor);

    static const unsigned int NUM_FACES = 6;

    std::array<Ogre::Camera*, NUM_FACES> m_cameras;
    std::array<Ogre::RenderTarget*, NUM_FACES> m_render_targets;
    Ogre::TexturePtr     m_rtt_texture;
    int                  m_update_round = 0; /// Render targets are updated one-by-one; this is the index of next target to update.
    Ogre::Overlay*       m_debug_overlay = nullptr;
    Ogre::SceneNode*     m_debug_overlay_snode = nullptr;
};

/// @} // addtogroup Gfx

} // namespace RoR
