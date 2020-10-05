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
#include "GfxData.h"
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

    struct SimBuffer /// Buffered simulation data
    {
        SimBuffer();

        struct NodeSB
        {
            Ogre::Vector3 AbsPosition; // classic name
            bool nd_has_contact:1;
            bool nd_is_wet:1;
        };

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
            float simbuf_tj_ab_thrust; //! Turbojet afterburner
            float simbuf_tj_exhaust_velo; //! Turbojet
            bool  simbuf_ae_turboprop:1; //!< This is a TurboProp/PistonProp
            bool  simbuf_ae_ignition:1;
            bool  simbuf_ae_failed:1;
            bool  simbuf_tj_afterburn:1; //! Turbojet afterburner
        };

        struct AirbrakeSB
        {
            float simbuf_ab_ratio;
        };

        std::unique_ptr<NodeSB>     simbuf_nodes;
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
        float                       simbuf_engine_torque;
        float                       simbuf_inputshaft_rpm; // Land vehicle only
        float                       simbuf_drive_ratio; // Land vehicle only
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
        float                       simbuf_top_speed;
        // Autopilot
        int                         simbuf_ap_heading_mode;
        int                         simbuf_ap_heading_value;
        int                         simbuf_ap_alt_mode;
        int                         simbuf_ap_alt_value;
        bool                        simbuf_ap_ias_mode;
        int                         simbuf_ap_ias_value;
        bool                        simbuf_ap_gpws_mode;
        bool                        simbuf_ap_ils_available;
        float                       simbuf_ap_ils_vdev;
        float                       simbuf_ap_ils_hdev;
        int                         simbuf_ap_vs_value;
    };

    struct Attributes    //!< Actor visual attributes
    {
        float            xa_speedo_highest_kph;
        bool             xa_speedo_use_engine_max_rpm;
        int              xa_num_gears; //!< Gearbox
        float            xa_engine_max_rpm;
        int              xa_camera0_pos_node;
        int              xa_camera0_roll_node;
        int              xa_driveable;
        bool             xa_has_autopilot;
        bool             xa_has_engine;
        Ogre::MaterialPtr xa_help_mat;
        Ogre::TexturePtr  xa_help_tex;
    };

    GfxActor(Actor* actor, ActorSpawner* spawner, std::string ogre_resource_group,
        std::vector<NodeGfx>& gfx_nodes, std::vector<Prop>& props, int driverseat_prop_idx, RoR::Renderdash* renderdash);

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
    SimBuffer::NodeSB*        GetSimNodeBuffer   ()                       { return m_simbuf.simbuf_nodes.get(); }
    std::set<GfxActor*>       GetLinkedGfxActors ()                       { return m_linked_gfx_actors; }
    Ogre::String              GetResourceGroup   ()                       { return m_custom_resource_group; }
    std::string               FetchActorDesignName() const;
    int                       FetchNumBeams      () const ;
    int                       FetchNumNodes      () const ;
    int                       FetchNumWheelNodes () const ;
    bool                 HasDriverSeatProp   () const { return m_driverseat_prop_index != -1; }
    void                 UpdateBeaconFlare   (Prop & prop, float dt, bool is_player_actor);
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
    std::vector<Prop>           m_props;
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
