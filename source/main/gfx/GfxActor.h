/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2016-2017 Petr Ohlidal & contributors

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
/// @author Petr Ohlidal
/// @date   2017/04
/// @brief  Manager for all visuals belonging to a single actor.

#pragma once

#include "ForwardDeclarations.h"

#include <OgreColourValue.h>
#include <OgreMaterial.h>
#include <OgreQuaternion.h>
#include <OgreTexture.h>
#include <OgreVector3.h>
#include <string>
#include <vector>

namespace RoR
{

class GfxActor
{
    friend class ::ActorSpawner; // The factory

public:

    struct FlareMaterial
    {
        int               flare_index;
        Ogre::MaterialPtr mat_instance;
        Ogre::ColourValue emissive_color;
    };

    enum class VideoCamState
    {
        VCSTATE_INVALID,
        VCSTATE_DISABLED,
        VCSTATE_ENABLED_OFFLINE,
        VCSTATE_ENABLED_ONLINE,
    };

    enum class VideoCamType
    {
        VCTYPE_INVALID,
        VCTYPE_VIDEOCAM,
        VCTYPE_TRACKING_VIDEOCAM,
        VCTYPE_MIRROR,
        VCTYPE_MIRROR_PROP_LEFT, ///< The classic 'special prop/rear view mirror'
        VCTYPE_MIRROR_PROP_RIGHT, ///< The classic 'special prop/rear view mirror'
    };

    /// An Ogre::Camera mounted on the actor and rendering into
    /// either in-scene texture or external window.
    struct VideoCamera
    {
        VideoCamera();

        VideoCamType         vcam_type;          // old 'camRole'
        node_t*              vcam_node_center;   // old 'nref'
        node_t*              vcam_node_dir_y;    // old 'ny'
        node_t*              vcam_node_dir_z;    // old 'nz'
        node_t*              vcam_node_alt_pos;  // old 'camNode'
        node_t*              vcam_node_lookat;   // old 'lookat' - only present for type=VCTYPE_TRACK_CAM
        Ogre::Quaternion     vcam_rotation;      // old 'rotation'
        Ogre::Vector3        vcam_pos_offset;    // old 'offset'
        Ogre::MaterialPtr    vcam_material;      // old 'mat' + 'materialName'
        std::string          vcam_off_tex_name;  // old 'disabledTexture' - used when videocamera is offline.
        Ogre::Camera*        vcam_ogre_camera;   // old 'mVidCam'
        Ogre::RenderTexture* vcam_render_target; // old 'rttTex'
        Ogre::TexturePtr     vcam_render_tex;
        Ogre::SceneNode*     vcam_debug_node;    // old 'debugNode'
        Ogre::RenderWindow*  vcam_render_window; // old 'rwMirror'
        Ogre::SceneNode*     vcam_prop_scenenode; // Only for type=MIRROR_PROP_*
    };

    GfxActor(Actor* actor, std::string ogre_resource_group):
        m_actor(actor),
        m_custom_resource_group(ogre_resource_group),
        m_vidcam_state(VideoCamState::VCSTATE_ENABLED_ONLINE)
    {}

    ~GfxActor();

    void                 AddMaterialFlare    (int flare_index, Ogre::MaterialPtr mat);
    void                 SetMaterialFlareOn  (int flare_index, bool state_on);
    void                 RegisterCabMaterial (Ogre::MaterialPtr mat, Ogre::MaterialPtr mat_trans);
    void                 SetCabLightsActive  (bool state_on);
    Ogre::MaterialPtr&   GetCabTransMaterial () { return m_cab_mat_visual_trans; }
    void                 SetVideoCamState    (VideoCamState state);
    inline VideoCamState GetVideoCamState    () const { return m_vidcam_state; }
    void                 UpdateVideoCameras  (float dt_sec);

private:

    Actor*                      m_actor;
    std::string                 m_custom_resource_group; ///< Stores OGRE resources individual to this actor
    std::vector<FlareMaterial>  m_flare_materials;
    VideoCamState               m_vidcam_state;
    std::vector<VideoCamera>    m_videocameras;

    // Cab materials and their features
    Ogre::MaterialPtr           m_cab_mat_visual; ///< Updated in-place from templates
    Ogre::MaterialPtr           m_cab_mat_visual_trans;
    Ogre::MaterialPtr           m_cab_mat_template_plain;
    Ogre::MaterialPtr           m_cab_mat_template_emissive;
};

} // Namespace RoR
