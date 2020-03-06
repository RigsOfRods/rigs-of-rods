/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2016-2020 Petr Ohlidal

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

/// @file
/// @brief  Constants and data structures dedicated exclusively to visualization.
///         For simulation-related data structures, see 'physics/BeamData.h'

#pragma once

#include "BeamData.h"

#include <Ogre.h>
#include <stdint.h>
#include <string>

namespace RoR {

enum VideoCamType
{
    VCTYPE_INVALID,
    VCTYPE_VIDEOCAM,
    VCTYPE_TRACKING_VIDEOCAM,
    VCTYPE_MIRROR,
    VCTYPE_MIRROR_PROP_LEFT,  //!< The classic 'special prop/rear view mirror'
    VCTYPE_MIRROR_PROP_RIGHT, //!< The classic 'special prop/rear view mirror'
};

/// An Ogre::Camera mounted on the actor and rendering into
/// either in-scene texture or external window.
struct VideoCamera
{
    VideoCamType         vcam_type           = VCTYPE_INVALID;
    uint16_t             vcam_node_center    = node_t::INVALID_IDX;
    uint16_t             vcam_node_dir_y     = node_t::INVALID_IDX;
    uint16_t             vcam_node_dir_z     = node_t::INVALID_IDX;
    uint16_t             vcam_node_alt_pos   = node_t::INVALID_IDX;
    uint16_t             vcam_node_lookat    = node_t::INVALID_IDX; //!< Only for VCTYPE_TRACK_CAM
    Ogre::Quaternion     vcam_rotation;
    Ogre::Vector3        vcam_pos_offset     = Ogre::Vector3::ZERO;
    Ogre::MaterialPtr    vcam_material;
    std::string          vcam_off_tex_name;                         //!< Used when videocamera is offline
    Ogre::Camera*        vcam_ogre_camera    = nullptr;
    Ogre::RenderTexture* vcam_render_target  = nullptr;
    Ogre::TexturePtr     vcam_render_tex;
    Ogre::SceneNode*     vcam_debug_node     = nullptr;
    Ogre::RenderWindow*  vcam_render_window  = nullptr;
    Ogre::SceneNode*     vcam_prop_scenenode = nullptr;             //!< Only for VCTYPE_MIRROR_PROP_*
};

} // namespace RoR
