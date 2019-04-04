/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2016-2018 Petr Ohlidal & contributors

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

#include "Differentials.h"
#include "ForwardDeclarations.h"
#include "RigDef_Prerequisites.h"
#include "ThreadPool.h" // class Task

#include <OgreAxisAlignedBox.h>
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

    /// An Ogre::Camera mounted on the actor and rendering into
    /// either in-scene texture or external window.
    struct VideoCamera
    {
        VideoCamera();

        VideoCamType         vcam_type;          // old 'camRole'
        uint16_t             vcam_node_center;   // old 'nref'
        uint16_t             vcam_node_dir_y;    // old 'ny'
        uint16_t             vcam_node_dir_z;    // old 'nz'
        uint16_t             vcam_node_alt_pos;  // old 'camNode'
        uint16_t             vcam_node_lookat;   // old 'lookat' - only present for type=VCTYPE_TRACK_CAM
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

    /// Gfx attributes/state of a softbody node
    struct NodeGfx
    {
        NodeGfx(uint16_t node_idx);

        float      nx_wet_time_sec; //!< 'Wet' means "already out of water, producing dripping particles". Set to -1 when not 'wet'.
        uint16_t   nx_node_idx;

        // Bit flags
        bool       nx_no_particles:1;     //!< User-defined attr; disable all particles
        bool       nx_may_get_wet:1;      //!< Attr; enables water drip and vapour
        bool       nx_is_hot:1;           //!< User-defined attr; emits vapour particles when in contact with water.
        bool       nx_under_water_prev:1; //!< State
        bool       nx_no_sparks:1;        //!< User-defined attr; 

    }; // more to come... ~only_a_ptr, 04/2018

    /// Copy of node simulation state
    struct NodeData
    {
        Ogre::Vector3 AbsPosition;
        bool nd_has_contact:1;
        bool nd_is_wet:1;
    };

    /// Visuals of softbody beam (`beam_t` struct); Partially updated along with SimBuffer
    struct Rod
    {
        // We don't keep pointer to the Ogre::Entity - we rely on the SceneNode keeping it attached all the time.
        Ogre::SceneNode* rod_scenenode;
        uint16_t         rod_beam_index;  //!< Index of the associated `beam_t` instance; assumes Actor has at most 65536 beams (RoR doesn't have a soft limit now, but until v0.4.8 it was 5000 beams).
        uint16_t         rod_diameter_mm; //!< Diameter in millimeters

        // Assumption: Actor has at most 65536 nodes (RoR doesn't have a soft limit right now, but until v0.4.8 it was 1000 nodes).
        uint16_t         rod_node1;         //!< Node index - may change during simulation!
        uint16_t         rod_node2;         //!< Node index - may change during simulation!
        Actor*           rod_target_actor;
        bool             rod_is_visible;
    };

    struct WheelGfx
    {
        WheelGfx(): wx_flex_mesh(nullptr), wx_scenenode(nullptr), wx_is_meshwheel(false) {}

        Flexable*        wx_flex_mesh;
        Ogre::SceneNode* wx_scenenode;
        bool             wx_is_meshwheel;
    };

    struct AirbrakeGfx
    {
        Ogre::MeshPtr    abx_mesh;
        Ogre::SceneNode* abx_scenenode;
        Ogre::Entity*    abx_entity;
        Ogre::Vector3    abx_offset;
        uint16_t         abx_ref_node;
        uint16_t         abx_x_node;
        uint16_t         abx_y_node;
    };

    struct SimBuffer /// Buffered simulation data
    {
        struct ScrewPropSB
        {
            float simbuf_sp_rudder;
            float simbuf_sp_throttle;
        };

        struct CommandKeySB
        {
            float simbuf_cmd_value;
        };

        struct AeroEngineSB
        {
            float simbuf_ae_rpm;
            float simbuf_ae_rpmpc;
            float simbuf_ae_throttle;
            float simbuf_tp_aetorque; //!< Turboprop torque, used by animation "aetorque"
            float simbuf_tp_aepitch;  //!< Turboprop pitch, used by animation "aepitch"
            bool  simbuf_ae_turboprop:1; //!< This is a TurboProp/PistonProp
            bool  simbuf_ae_ignition:1;
            bool  simbuf_ae_failed:1;
        };

        struct AirbrakeSB
        {
            float simbuf_ab_ratio;
        };

        std::unique_ptr<NodeData>   simbuf_nodes;
        Ogre::Vector3               simbuf_pos;
        Ogre::Vector3               simbuf_node0_velo;
        bool                        simbuf_live_local;
        bool                        simbuf_physics_paused;
        float                       simbuf_rotation;
        float                       simbuf_tyre_pressure;
        Ogre::AxisAlignedBox        simbuf_aabb;
        std::string                 simbuf_net_username;
        bool                        simbuf_is_remote;
        int                         simbuf_gear;
        int                         simbuf_autoshift;
        float                       simbuf_wheel_speed;
        float                       simbuf_engine_rpm;
        float                       simbuf_engine_crankfactor;
        float                       simbuf_engine_turbo_psi;
        float                       simbuf_engine_accel;
        bool                        simbuf_beaconlight_active;
        float                       simbuf_hydro_dir_state; // State of steering actuator ('hydro'), for steeringwheel display
        float                       simbuf_hydro_aileron_state;
        float                       simbuf_hydro_elevator_state;
        float                       simbuf_hydro_aero_rudder_state;
        int                         simbuf_cur_cinecam;
        std::vector<ScrewPropSB>    simbuf_screwprops;
        std::vector<CommandKeySB>   simbuf_commandkey;
        std::vector<AeroEngineSB>   simbuf_aeroengines;
        std::vector<AirbrakeSB>     simbuf_airbrakes;
        DiffType                    simbuf_diff_type;
        bool                        simbuf_parking_brake;
        float                       simbuf_brake;
        float                       simbuf_clutch;
        int                         simbuf_aero_flap_state;
        int                         simbuf_airbrake_state;
        float                       simbuf_wing4_aoa;
        bool                        simbuf_headlight_on;
        Ogre::Vector3               simbuf_direction; //!< Output of `Actor::getDirection()`
        int                         simbuf_autopilot_heading;
        bool                        simbuf_autopilot_ils_available;
        float                       simbuf_autopilot_ils_vdev;
        float                       simbuf_autopilot_ils_hdev;
    };

    struct Attributes    //!< Actor visual attributes
    {
        float            xa_speedo_highest_kph;
        bool             xa_speedo_use_engine_max_rpm;
        int              xa_num_gears; //!< Gearbox
        float            xa_engine_max_rpm;
        int              xa_camera0_pos_node;
        int              xa_camera0_roll_node;
        bool             xa_has_autopilot;
    };

    GfxActor(Actor* actor, ActorSpawner* spawner, std::string ogre_resource_group,
        std::vector<NodeGfx>& gfx_nodes, std::vector<prop_t>& props, int driverseat_prop_idx, RoR::Renderdash* renderdash);

    ~GfxActor();

    void                      AddMaterialFlare   (int flare_index, Ogre::MaterialPtr mat);
    void                      SetMaterialFlareOn (int flare_index, bool state_on);
    void                      RegisterCabMaterial(Ogre::MaterialPtr mat, Ogre::MaterialPtr mat_trans);
    void                      RegisterCabMesh    (Ogre::Entity* ent, Ogre::SceneNode* snode, FlexObj* flexobj);
    void                      SetCabLightsActive (bool state_on);
    void                      SetVideoCamState   (VideoCamState state);
    void                      UpdateVideoCameras (float dt_sec);
    void                      UpdateParticles    (float dt_sec);
    void                      AddRod             (int beam_index, int node1_index, int node2_index, const char* material_name, bool visible, float diameter_meters);
    void                      UpdateRods         ();
    void                      SetRodsVisible     (bool visible);
    void                      ScaleActor         (Ogre::Vector3 relpos, float ratio);
    bool                      IsActorLive        () const; //!< Should the visuals be updated for this actor?
    bool                      IsActorInitialized () const  { return m_initialized; } //!< Temporary TODO: Remove once the spawn routine is fixed
    void                      InitializeActor    ()        { m_initialized = true; } //!< Temporary TODO: Remove once the spawn routine is fixed
    void                      UpdateSimDataBuffer(); //!< Copies sim. data from `Actor` to `GfxActor` for later update
    void                      SetWheelVisuals    (uint16_t index, WheelGfx wheel_gfx);
    void                      CalculateDriverPos (Ogre::Vector3& out_pos, Ogre::Quaternion& out_rot);
    void                      UpdateWheelVisuals ();
    void                      FinishWheelUpdates ();
    void                      UpdateFlexbodies   ();
    void                      FinishFlexbodyTasks();
    void                      SetFlexbodyVisible (bool visible);
    void                      SetWheelsVisible   (bool value);
    void                      SetAllMeshesVisible(bool value);
    void                      SetCastShadows     (bool value);
    void                      UpdateDebugView    ();
    void                      ToggleDebugView    ();
    void                      CycleDebugViews    ();
    void                      UpdateCabMesh      ();
    void                      UpdateWingMeshes   ();
    int                       GetActorId         () const;
    int                       GetActorState      () const;
    int                       GetNumFlexbodies   () const { return static_cast<int>(m_flexbodies.size()); }
    void                      ResetFlexbodies    ();
    void                      SetFlexbodiesVisible(bool visible);
    int                       GetActorDriveable  () const;
    void                      RegisterAirbrakes  ();
    void                      UpdateAirbrakes    ();
    void                      UpdateCParticles   ();
    void                      UpdateAeroEngines  ();
    void                      UpdateNetLabels    (float dt);
    void                      SetDebugView       (DebugViewType dv);
    void                      SortFlexbodies     ();
    void                      AddFlexbody        (FlexBody* fb)           { m_flexbodies.push_back(fb); }
    Attributes&               GetAttributes      ()                       { return m_attr; }
    inline Ogre::MaterialPtr& GetCabTransMaterial()                       { return m_cab_mat_visual_trans; }
    inline VideoCamState      GetVideoCamState   () const                 { return m_vidcam_state; }
    inline DebugViewType      GetDebugView       () const                 { return m_debug_view; }
    SimBuffer &               GetSimDataBuffer   ()                       { return m_simbuf; }
    NodeData*                 GetSimNodeBuffer   ()                       { return m_simbuf.simbuf_nodes.get(); }
    void                      SetSurveyMapEntity (SurveyMapEntity* e)     { m_survey_map_entity = e; }
    SurveyMapEntity*          GetSurveyMapEntity ()                       { return m_survey_map_entity; }
    std::set<GfxActor*>       GetLinkedGfxActors ()                       { return m_linked_gfx_actors; }
    Ogre::String              GetResourceGroup   ()                       { return m_custom_resource_group; }
    bool                 HasDriverSeatProp   () const { return m_driverseat_prop_index != -1; }
    void                 UpdateBeaconFlare   (prop_t & prop, float dt, bool is_player_actor);
    void                 UpdateProps         (float dt, bool is_player_actor);
    void                 UpdatePropAnimations(float dt, bool is_player_connected);
    void                 SetPropsVisible     (bool visible);
    void                 SetRenderdashActive (bool active);
    void                 SetBeaconsEnabled   (bool beacon_light_is_active);
    void                 CalcPropAnimation   (const int flag_state, float& cstate, int& div, float timer,
                                              const float lower_limit, const float upper_limit, const float option3);
    void                 UpdateFlares        (float dt_sec, bool is_player);

private:

    static Ogre::Quaternion SpecialGetRotationTo(const Ogre::Vector3& src, const Ogre::Vector3& dest);

    Actor*                      m_actor;

    std::string                 m_custom_resource_group;
    std::vector<FlareMaterial>  m_flare_materials;
    VideoCamState               m_vidcam_state;
    std::vector<VideoCamera>    m_videocameras;
    DebugViewType               m_debug_view;
    DebugViewType               m_last_debug_view;
    std::vector<NodeGfx>        m_gfx_nodes;
    std::vector<AirbrakeGfx>    m_gfx_airbrakes;
    std::vector<prop_t>         m_props;
    std::vector<FlexBody*>      m_flexbodies;
    int                         m_driverseat_prop_index;
    Attributes                  m_attr;
    DustPool*                   m_particles_drip;
    DustPool*                   m_particles_misc; // This is "dust" in RoR::GfxScene; handles dust, vapour and tyre smoke
    DustPool*                   m_particles_splash;
    DustPool*                   m_particles_ripple;
    DustPool*                   m_particles_sparks;
    DustPool*                   m_particles_clump;
    std::vector<Rod>            m_rods;
    std::vector<WheelGfx>       m_wheels;
    Ogre::SceneNode*            m_rods_parent_scenenode;
    RoR::Renderdash*            m_renderdash;
    std::vector<std::shared_ptr<Task>> m_flexwheel_tasks;
    std::vector<std::shared_ptr<Task>> m_flexbody_tasks;
    bool                        m_beaconlight_active;
    float                       m_prop_anim_crankfactor_prev;
    float                       m_prop_anim_shift_timer;
    int                         m_prop_anim_prev_gear;
    std::set<GfxActor*>         m_linked_gfx_actors;

    bool                        m_initialized;

    SimBuffer                   m_simbuf;

    SurveyMapEntity*            m_survey_map_entity;

    // Old cab mesh
    FlexObj*                    m_cab_mesh;
    Ogre::SceneNode*            m_cab_scene_node;
    Ogre::Entity*               m_cab_entity;

    // Cab materials and their features
    Ogre::MaterialPtr           m_cab_mat_visual; ///< Updated in-place from templates
    Ogre::MaterialPtr           m_cab_mat_visual_trans;
    Ogre::MaterialPtr           m_cab_mat_template_plain;
    Ogre::MaterialPtr           m_cab_mat_template_emissive;
};

} // Namespace RoR
