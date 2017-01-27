/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2017+     Petr Ohlidal

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

#include <OgreMaterial.h>
#include <OgreTexture.h>

namespace RoR {

/// Uses a single Render-to-Texture target to display 
/// one of vehicle's rear view mirrors - the one closest in the FOV
class LegacyRearViewMirrors
{
public:
     LegacyRearViewMirrors();
    ~LegacyRearViewMirrors() { assert(!m_is_initialized); } /// MUST call Shutdown() first!

    void Init        (Ogre::SceneManager* scene_mgr, Ogre::Camera* main_cam);
    void Update      (Beam* truck, Ogre::Vector3 main_cam_pos, Ogre::Vector3 main_cam_dir, Ogre::Radian cam_fov_y, float cam_aspect_ratio);
    void SetActive   (bool state);
    void Shutdown    (Ogre::SceneManager* scene_mgr);

private:
    Ogre::Camera*        m_mirror_camera;
    Ogre::RenderTexture* m_rtt_target;
    Ogre::MaterialPtr    m_material;
    Ogre::TexturePtr     m_texture;
    // Flags
    bool                 m_is_initialized: 1;
    bool                 m_is_enabled: 1;
};

} // namespace RoR

