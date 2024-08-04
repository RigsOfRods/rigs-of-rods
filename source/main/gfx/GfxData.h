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

/*
    @file
    @brief  Constants and data structures dedicated exclusively to visualization.
            For simulation-related data structures, see 'physics/SimData.h'
*/

#pragma once

#include "MeshObject.h"
#include "SimData.h"

#include <Ogre.h>
#include <stdint.h>
#include <string>

namespace RoR {

/// @addtogroup Gfx
/// @{


static const BitMask64_t    PROP_ANIM_FLAG_AIRSPEED      = BITMASK64(1);
static const BitMask64_t    PROP_ANIM_FLAG_VVI           = BITMASK64(2);
static const BitMask64_t    PROP_ANIM_FLAG_ALTIMETER     = BITMASK64(3);
static const BitMask64_t    PROP_ANIM_FLAG_AOA           = BITMASK64(4);
static const BitMask64_t    PROP_ANIM_FLAG_FLAP          = BITMASK64(5);
static const BitMask64_t    PROP_ANIM_FLAG_AIRBRAKE      = BITMASK64(6);
static const BitMask64_t    PROP_ANIM_FLAG_ROLL          = BITMASK64(7);
static const BitMask64_t    PROP_ANIM_FLAG_PITCH         = BITMASK64(8);
static const BitMask64_t    PROP_ANIM_FLAG_THROTTLE      = BITMASK64(9);
static const BitMask64_t    PROP_ANIM_FLAG_RPM           = BITMASK64(10);
static const BitMask64_t    PROP_ANIM_FLAG_ACCEL         = BITMASK64(11);
static const BitMask64_t    PROP_ANIM_FLAG_BRAKE         = BITMASK64(12);
static const BitMask64_t    PROP_ANIM_FLAG_CLUTCH        = BITMASK64(13);
static const BitMask64_t    PROP_ANIM_FLAG_TACHO         = BITMASK64(14);
static const BitMask64_t    PROP_ANIM_FLAG_SPEEDO        = BITMASK64(15);
static const BitMask64_t    PROP_ANIM_FLAG_PBRAKE        = BITMASK64(16);
static const BitMask64_t    PROP_ANIM_FLAG_TURBO         = BITMASK64(17);
static const BitMask64_t    PROP_ANIM_FLAG_SHIFTER       = BITMASK64(18);
static const BitMask64_t    PROP_ANIM_FLAG_AETORQUE      = BITMASK64(19);
static const BitMask64_t    PROP_ANIM_FLAG_AEPITCH       = BITMASK64(20);
static const BitMask64_t    PROP_ANIM_FLAG_AESTATUS      = BITMASK64(21);
static const BitMask64_t    PROP_ANIM_FLAG_TORQUE        = BITMASK64(22);
static const BitMask64_t    PROP_ANIM_FLAG_HEADING       = BITMASK64(23);
static const BitMask64_t    PROP_ANIM_FLAG_DIFFLOCK      = BITMASK64(24);
static const BitMask64_t    PROP_ANIM_FLAG_STEERING      = BITMASK64(25);
static const BitMask64_t    PROP_ANIM_FLAG_EVENT         = BITMASK64(26);
static const BitMask64_t    PROP_ANIM_FLAG_AILERONS      = BITMASK64(27);
static const BitMask64_t    PROP_ANIM_FLAG_ARUDDER       = BITMASK64(28);
static const BitMask64_t    PROP_ANIM_FLAG_BRUDDER       = BITMASK64(29);
static const BitMask64_t    PROP_ANIM_FLAG_BTHROTTLE     = BITMASK64(30);
static const BitMask64_t    PROP_ANIM_FLAG_PERMANENT     = BITMASK64(31);
static const BitMask64_t    PROP_ANIM_FLAG_ELEVATORS     = BITMASK64(32);
static const BitMask64_t    PROP_ANIM_FLAG_DASHBOARD     = BITMASK64(33); //!< Used with dashboard system inputs, see `enum DashData` in file DashBoardManager.h
static const BitMask64_t    PROP_ANIM_FLAG_SIGNALSTALK   = BITMASK64(34); //!< Turn indicator stalk position (-1=left, 0=off, 1=right)

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

inline PropAnimMode operator|=(PropAnimMode& dst, PropAnimMode const& arg) { dst = static_cast<PropAnimMode>(dst|arg); return dst; }

enum class VideoCamState
{
    VCSTATE_INVALID,
    VCSTATE_DISABLED,
    VCSTATE_ENABLED_OFFLINE,
    VCSTATE_ENABLED_ONLINE,
};

enum class DebugViewType
{
    DEBUGVIEW_NONE,
    DEBUGVIEW_SKELETON,
    DEBUGVIEW_NODES,
    DEBUGVIEW_BEAMS,
    DEBUGVIEW_WHEELS,
    DEBUGVIEW_SHOCKS,
    DEBUGVIEW_ROTATORS,
    DEBUGVIEW_SLIDENODES,
    DEBUGVIEW_SUBMESH,
};

/// Used by rig-def/addonpart/tuneup formats to specify wheel rim mesh orientation.
enum class WheelSide: char
{
    INVALID   = 'n',
    RIGHT     = 'r',
    LEFT      = 'l'
};

// Dynamic visibility control (value 0 and higher is cinecam index) - common to 'props' and 'flexbodies'
typedef int CameraMode_t;
static CameraMode_t CAMERA_MODE_ALWAYS_HIDDEN = -3;
static CameraMode_t CAMERA_MODE_ALWAYS_VISIBLE = -2;
static CameraMode_t CAMERA_MODE_3RDPERSON_ONLY = -1;

enum ShifterPropAnim
{
    SHIFTER_INVALID = 0,
    SHIFTERMAN1 = 1,
    SHIFTERMAN2 = 2,
    SHIFTERSEQ = 3,
    SHIFTERLIN = 4
};

struct PropAnim
{
    float        animratio    = 0;  //!< A coefficient for the animation, prop degree if used with mode: rotation and propoffset if used with mode: offset.
    BitMask64_t  animFlags    = 0ull;
    PropAnimMode animMode     = {};

    /// MULTIPURPOSE
    /// * SHIFTER type `ShifterPropAnim` (1 = shifterman1, 2 = shifterman2, 3 = shifterseq, 4 = shifterlin)
    /// * AEROENGINE number (starting from 1), applies to: rpm + throttle + torque ( turboprop ) + pitch ( turboprop ) + status +  fire
    /// * ALTIMETER type (1 = 100k limited, 2 = 10k oscillating, 3 = 1k oscillating)
    /// * DASHBOARD input (see `enum DashData` in file DashBoardManager.h)
    float        animOpt3     = 0;
    float        animOpt5     = 0;
    float        lower_limit  = 0;  //!< The lower limit for the animation
    float        upper_limit  = 0;  //!< The upper limit for the animation

    // Only for SHIFTER
    float        shifterSmooth = 0.f;
    float        shifterStep = 0.f;
    float        shifterTarget = 0.f;
};

/// A mesh attached to vehicle frame via 3 nodes
struct Prop
{
    PropID_t              pp_id                   = PROPID_INVALID;
    NodeNum_t             pp_node_ref             = NODENUM_INVALID;
    NodeNum_t             pp_node_x               = NODENUM_INVALID;
    NodeNum_t             pp_node_y               = NODENUM_INVALID;
    Ogre::Vector3         pp_offset               = Ogre::Vector3::ZERO;
    Ogre::Vector3         pp_offset_orig          = Ogre::Vector3::ZERO; //!< Used with ANIM_FLAG_OFFSET*
    Ogre::Vector3         pp_rota                 = Ogre::Vector3::ZERO;
    Ogre::Quaternion      pp_rot                  = Ogre::Quaternion::IDENTITY;
    Ogre::SceneNode*      pp_scene_node           = nullptr;             //!< The pivot scene node (parented to root-node).
    MeshObject*           pp_mesh_obj             = nullptr;             //!< Optional; NULL if removed via tuneup/addonpart
    std::string           pp_media[2];                                   //!< Redundant, for Tuning UI. Media1 = prop mesh name, Media2 = steeringwheel mesh/beaconprop flare mat.
    std::vector<PropAnim> pp_animations;

    /// @name Visibility control (same as flexbody - see file FlexBody.h)
    /// @{
    CameraMode_t          pp_camera_mode_active = CAMERA_MODE_ALWAYS_VISIBLE; //!< Dynamic visibility mode {0 and higher = cinecam index}
    CameraMode_t          pp_camera_mode_orig = CAMERA_MODE_ALWAYS_VISIBLE;   //!< Dynamic visibility mode {0 and higher = cinecam index}
    /// @}

    // Special prop - steering wheel
    MeshObject*           pp_wheel_mesh_obj       = nullptr;
    Ogre::Vector3         pp_wheel_pos            = Ogre::Vector3::ZERO;
    Ogre::SceneNode*      pp_wheel_scene_node     = nullptr;
    float                 pp_wheel_rot_degree     = 0;
    
    // Special prop - beacon
    char                  pp_beacon_type          = 0;                   //!< Special prop: beacon {0 = none, 'b' = user-specified, 'r' = red, 'p' = police lightbar, 'L'/'R'/'w' - aircraft wings}
    Ogre::BillboardSet*   pp_beacon_bbs[4]        = {};
    Ogre::SceneNode*      pp_beacon_scene_node[4] = {};
    Ogre::Light*          pp_beacon_light[4]      = {};
    float                 pp_beacon_rot_rate[4]   = {};                  //!< Radians per second
    float                 pp_beacon_rot_angle[4]  = {};                  //!< Radians
    
    // Special prop - aero engine
    int                   pp_aero_engine_idx      = -1;                  //!< Special - a turboprop/pistonprop reference
    bool                  pp_aero_propeller_blade = false;               //!< Special - single blade mesh
    bool                  pp_aero_propeller_spin  = false;               //!< Special - blurred spinning propeller effect

    void setPropMeshesVisible(bool visible)
    {
        if (pp_mesh_obj)
            pp_mesh_obj->setVisible(visible);
        if (pp_wheel_mesh_obj)
            pp_wheel_mesh_obj->setVisible(visible);
        if (pp_beacon_scene_node[0])
            pp_beacon_scene_node[0]->setVisible(visible);
        if (pp_beacon_scene_node[1])
            pp_beacon_scene_node[1]->setVisible(visible);
        if (pp_beacon_scene_node[2])
            pp_beacon_scene_node[2]->setVisible(visible);
        if (pp_beacon_scene_node[3])
            pp_beacon_scene_node[3]->setVisible(visible);
    }
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
    NodeNum_t            vcam_node_center    = NODENUM_INVALID;
    NodeNum_t            vcam_node_dir_y     = NODENUM_INVALID;
    NodeNum_t            vcam_node_dir_z     = NODENUM_INVALID;
    NodeNum_t            vcam_node_alt_pos   = NODENUM_INVALID;
    NodeNum_t            vcam_node_lookat    = NODENUM_INVALID; //!< Only for VCTYPE_TRACK_CAM
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

/// Gfx attributes/state of a softbody node
struct NodeGfx
{
    NodeGfx(NodeNum_t node_idx):
        nx_node_idx(node_idx),
        nx_no_particles(false), // Bitfields can't be initialized in-class :(
        nx_may_get_wet(false),
        nx_is_hot(false),
        nx_no_sparks(true),
        nx_under_water_prev(false)
    {}

    float      nx_wet_time_sec = -1; //!< 'Wet' means "already out of water, producing dripping particles". Set to -1 when not 'wet'.
    NodeNum_t  nx_node_idx = NODENUM_INVALID;

    // Bit flags
    bool       nx_no_particles:1;     //!< User-defined attr; disable all particles
    bool       nx_may_get_wet:1;      //!< Attr; enables water drip and vapour
    bool       nx_is_hot:1;           //!< User-defined attr; emits vapour particles when in contact with water.
    bool       nx_under_water_prev:1; //!< State
    bool       nx_no_sparks:1;        //!< User-defined attr; 

};

/// Visuals of softbody beam (`beam_t` struct); Partially updated along with SimBuffer
struct BeamGfx
{
    BeamGfx();
    ~BeamGfx();

    // We don't keep pointer to the Ogre::Entity - we rely on the SceneNode keeping it attached all the time.
    Ogre::SceneNode* rod_scenenode       = nullptr;
    uint16_t         rod_beam_index      = 0;
    uint16_t         rod_diameter_mm     = 0;                    //!< Diameter in millimeters

    NodeNum_t        rod_node1           = NODENUM_INVALID;  //!< Node index - may change during simulation!
    NodeNum_t        rod_node2           = NODENUM_INVALID;  //!< Node index - may change during simulation!
    ActorPtr         rod_target_actor;
    bool             rod_is_visible      = false;
};

struct WheelGfx
{
    WheelID_t          wx_wheel_id         = WHEELID_INVALID;
    Flexable*          wx_flex_mesh        = nullptr;
    Ogre::SceneNode*   wx_scenenode        = nullptr;
    WheelSide          wx_side             = WheelSide::INVALID;
    std::string        wx_rim_mesh_name;                         //!< Redundant, for Tuning UI. Only for 'meshwheels[2]' and 'flexbodywheels'
};

struct AirbrakeGfx
{
    Ogre::MeshPtr    abx_mesh;
    Ogre::SceneNode* abx_scenenode;
    Ogre::Entity*    abx_entity;
    Ogre::Vector3    abx_offset;
    NodeNum_t        abx_ref_node = NODENUM_INVALID;
    NodeNum_t        abx_x_node   = NODENUM_INVALID;
    NodeNum_t        abx_y_node   = NODENUM_INVALID;
};

struct FlareMaterial // materialflares
{
    int               flare_index;
    Ogre::MaterialPtr mat_instance;
    Ogre::ColourValue emissive_color;
};

struct Exhaust
{
    NodeNum_t emitterNode   = NODENUM_INVALID;
    NodeNum_t directionNode = NODENUM_INVALID;
    Ogre::SceneNode *smokeNode = nullptr;
    Ogre::ParticleSystem* smoker = nullptr; //!< This remains `nullptr` if removed via `addonpart_unwanted_exhaust` or Tuning UI.
    std::string particleSystemName; //!< Name in .particle file ~ for display in Tuning UI.
};

struct CParticle
{
    NodeNum_t emitterNode   = NODENUM_INVALID;
    NodeNum_t directionNode = NODENUM_INVALID;
    Ogre::SceneNode *snode = nullptr;
    Ogre::ParticleSystem* psys = nullptr;
};

/// @} // addtogroup Gfx

} // namespace RoR
