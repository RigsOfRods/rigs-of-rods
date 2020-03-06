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

enum PropAnimFlag
{
    PROP_ANIM_FLAG_AIRSPEED      = BITMASK(1),
    PROP_ANIM_FLAG_VVI           = BITMASK(2),
    PROP_ANIM_FLAG_ALTIMETER     = BITMASK(3),
    PROP_ANIM_FLAG_AOA           = BITMASK(4),
    PROP_ANIM_FLAG_FLAP          = BITMASK(5),
    PROP_ANIM_FLAG_AIRBRAKE      = BITMASK(6),
    PROP_ANIM_FLAG_ROLL          = BITMASK(7),
    PROP_ANIM_FLAG_PITCH         = BITMASK(8),
    PROP_ANIM_FLAG_THROTTLE      = BITMASK(9),
    PROP_ANIM_FLAG_RPM           = BITMASK(10),
    PROP_ANIM_FLAG_ACCEL         = BITMASK(11),
    PROP_ANIM_FLAG_BRAKE         = BITMASK(12),
    PROP_ANIM_FLAG_CLUTCH        = BITMASK(13),
    PROP_ANIM_FLAG_TACHO         = BITMASK(14),
    PROP_ANIM_FLAG_SPEEDO        = BITMASK(15),
    PROP_ANIM_FLAG_PBRAKE        = BITMASK(16),
    PROP_ANIM_FLAG_TURBO         = BITMASK(17),
    PROP_ANIM_FLAG_SHIFTER       = BITMASK(18),
    PROP_ANIM_FLAG_AETORQUE      = BITMASK(19),
    PROP_ANIM_FLAG_AEPITCH       = BITMASK(20),
    PROP_ANIM_FLAG_AESTATUS      = BITMASK(21),
    PROP_ANIM_FLAG_TORQUE        = BITMASK(22),
    PROP_ANIM_FLAG_HEADING       = BITMASK(23),
    PROP_ANIM_FLAG_DIFFLOCK      = BITMASK(24),
    PROP_ANIM_FLAG_STEERING      = BITMASK(25),
    PROP_ANIM_FLAG_EVENT         = BITMASK(26),
    PROP_ANIM_FLAG_AILERONS      = BITMASK(27),
    PROP_ANIM_FLAG_ARUDDER       = BITMASK(28),
    PROP_ANIM_FLAG_BRUDDER       = BITMASK(29),
    PROP_ANIM_FLAG_BTHROTTLE     = BITMASK(30),
    PROP_ANIM_FLAG_PERMANENT     = BITMASK(31),
    PROP_ANIM_FLAG_ELEVATORS     = BITMASK(32),
};

enum PropAnimMode
{
    PROP_ANIM_MODE_ROTA_X        = BITMASK(1),
    PROP_ANIM_MODE_ROTA_Y        = BITMASK(2),
    PROP_ANIM_MODE_ROTA_Z        = BITMASK(3),
    PROP_ANIM_MODE_OFFSET_X      = BITMASK(4),
    PROP_ANIM_MODE_OFFSET_Y      = BITMASK(5),
    PROP_ANIM_MODE_OFFSET_Z      = BITMASK(6),
    PROP_ANIM_MODE_AUTOANIMATE   = BITMASK(7),
    PROP_ANIM_MODE_NOFLIP        = BITMASK(8),
    PROP_ANIM_MODE_BOUNCE        = BITMASK(9),
};

inline PropAnimFlag operator|=(PropAnimFlag& dst, PropAnimFlag const& arg) { dst = static_cast<PropAnimFlag>(dst|arg); return dst; }
inline PropAnimMode operator|=(PropAnimMode& dst, PropAnimMode const& arg) { dst = static_cast<PropAnimMode>(dst|arg); return dst; }

struct PropAnim
{
    float        animratio    = 0;  //!< A coefficient for the animation, prop degree if used with mode: rotation and propoffset if used with mode: offset.
    PropAnimFlag animFlags    = {};
    PropAnimMode animMode     = {};
    float        animOpt3     = 0;  //!< Various purposes
    float        animOpt5     = 0;
    int          animKey      = 0;
    int          animKeyState = 0;
    int          lastanimKS   = 0;
    float        lower_limit  = 0;  //!< The lower limit for the animation
    float        upper_limit  = 0;  //!< The upper limit for the animation
};

/// A mesh attached to vehicle frame via 3 nodes
struct Prop
{
    Prop() { memset(this, 0, sizeof(*this)); }

    int noderef;
    int nodex;
    int nodey;
    float offsetx;
    float offsety;
    float offsetz;
    float rotaX;
    float rotaY;
    float rotaZ;
    float orgoffsetX;
    float orgoffsetY;
    float orgoffsetZ;
    Ogre::Quaternion rot;
    Ogre::SceneNode *scene_node; //!< The pivot scene node (parented to root-node).
    Ogre::SceneNode *wheel; //!< Special prop: custom steering wheel for dashboard
    Ogre::Vector3 wheelpos; //!< Special prop: custom steering wheel for dashboard
    int mirror;             //<! Special prop: rear view mirror {0 = disabled, -1 = right, 1 = left}
    char beacontype;        //<! Special prop: beacon {0 = none, 'b' = user-specified, 'r' = red, 'p' = police lightbar, 'L'/'R'/'w' - aircraft wings}

    // formerly named "bbs"
    Ogre::BillboardSet *beacon_flares_billboard_system[4];

    // formerly named bbsnode
    Ogre::SceneNode *beacon_flare_billboard_scene_node[4];

    // formerly named "light"
    Ogre::Light *beacon_light[4];

    // formerly named "brate"
    float beacon_light_rotation_rate[4]; //<! Radians per second
    
    // formerly named "bpos"
    float beacon_light_rotation_angle[4]; //<! Radians

    Ogre::Real wheelrotdegree;
    int cameramode; //!< Visibility control {-2 = always, -1 = 3rdPerson only, 0+ = cinecam index}
    MeshObject *mo;
    MeshObject *wheelmo;

    std::vector<PropAnim> pp_animations;

    int  pp_aero_engine_idx;          //!< Special - a turboprop/pistonprop reference
    bool pp_aero_propeller_blade:1;   //!< Special - single blade mesh
    bool pp_aero_propeller_spin:1;    //!< Special - blurred spinning propeller effect
};

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
