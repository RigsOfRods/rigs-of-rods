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

#include "ForwardDeclarations.h"
#include "ThreadPool.h" // class Task

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
        // Members intentionally have same names as in `node_t`
        Ogre::Vector3 AbsPosition;
    };

    /// Visuals of softbody beam (`beam_t` struct)
    struct Rod
    {
        // We don't keep pointer to the Ogre::Entity - we rely on the SceneNode keeping it attached all the time.
        Ogre::SceneNode* rod_scenenode;
        uint16_t         rod_beam_index;  //!< Index of the associated `beam_t` instance; assumes Actor has at most 65536 beams (RoR doesn't have a soft limit now, but until v0.4.8 it was 5000 beams).
        uint16_t         rod_diameter_mm; //!< Diameter in millimeters
        uint16_t         rod_node1;       //!< Node index - assumes the Actor has at most 65536 nodes (RoR doesn't have a soft limit now, but until v0.4.8 it was 1000 nodes).
        uint16_t         rod_node2;       //!< Node index - assumes the Actor has at most 65536 nodes (RoR doesn't have a soft limit now, but until v0.4.8 it was 1000 nodes).
    };

    struct WheelGfx
    {
        WheelGfx(): wx_flex_mesh(nullptr), wx_scenenode(nullptr), wx_is_meshwheel(false) {}

        Flexable*        wx_flex_mesh;
        Ogre::SceneNode* wx_scenenode;
        bool             wx_is_meshwheel;
    };

    struct SimBuffer /// Buffered simulation data
    {
        std::unique_ptr<NodeData>   simbuf_nodes;
        Ogre::Vector3               simbuf_pos;
        Ogre::Vector3               simbuf_node0_velo;
        bool                        simbuf_live_local;
    };

    GfxActor(Actor* actor, std::string ogre_resource_group, std::vector<NodeGfx>& gfx_nodes);

    ~GfxActor();

    void                      AddMaterialFlare   (int flare_index, Ogre::MaterialPtr mat);
    void                      SetMaterialFlareOn (int flare_index, bool state_on);
    void                      RegisterCabMaterial(Ogre::MaterialPtr mat, Ogre::MaterialPtr mat_trans);
    void                      SetCabLightsActive (bool state_on);
    void                      SetVideoCamState   (VideoCamState state);
    void                      UpdateVideoCameras (float dt_sec);
    void                      UpdateParticles    (float dt_sec);
    void                      AddRod             (int beam_index, int node1_index, int node2_index, const char* material_name, bool visible, float diameter_meters);
    void                      UpdateRods         ();
    void                      SetRodsVisible     (bool visible);
    void                      ScaleActor         (float ratio);
    bool                      IsActorLive        () const; //!< Should the visuals be updated for this actor?
    void                      UpdateSimDataBuffer(); //!< Copies sim. data from `Actor` to `GfxActor` for later update
    void                      SetWheelVisuals    (uint16_t index, WheelGfx wheel_gfx);
    void                      UpdateWheelVisuals ();
    void                      FinishWheelUpdates ();
    void                      SetWheelsVisible   (bool value);
    void                      UpdateDebugView    ();
    void                      CycleDebugViews    ();
    void                      UpdateCabMesh      ();
    inline void               SetDebugView       (DebugViewType dv)       { m_debug_view = dv; }
    inline Ogre::MaterialPtr& GetCabTransMaterial()                       { return m_cab_mat_visual_trans; }
    inline VideoCamState      GetVideoCamState   () const                 { return m_vidcam_state; }
    inline DebugViewType      GetDebugView       () const                 { return m_debug_view; }
    SimBuffer &               GetSimDataBuffer   ()                       { return m_simbuf; }
    NodeData*                 GetSimNodeBuffer   ()                       { return m_simbuf.simbuf_nodes.get(); }

private:

    static Ogre::Quaternion SpecialGetRotationTo(const Ogre::Vector3& src, const Ogre::Vector3& dest);

    Actor*                      m_actor;

    std::string                 m_custom_resource_group; ///< Stores OGRE resources individual to this actor
    std::vector<FlareMaterial>  m_flare_materials;
    VideoCamState               m_vidcam_state;
    std::vector<VideoCamera>    m_videocameras;
    DebugViewType               m_debug_view;
    std::vector<NodeGfx>        m_gfx_nodes;
    DustPool*                   m_particles_drip;
    DustPool*                   m_particles_misc; // This is "dust" in RoR::GfxScene; handles dust, vapour and tyre smoke
    DustPool*                   m_particles_splash;
    DustPool*                   m_particles_ripple;
    DustPool*                   m_particles_sparks;
    DustPool*                   m_particles_clump;
    std::vector<Rod>            m_rods;
    std::vector<WheelGfx>       m_wheels;
    Ogre::SceneNode*            m_rods_parent_scenenode;
    std::vector<std::shared_ptr<Task>> m_flexwheel_tasks;

    SimBuffer                   m_simbuf;

    // Cab materials and their features
    Ogre::MaterialPtr           m_cab_mat_visual; ///< Updated in-place from templates
    Ogre::MaterialPtr           m_cab_mat_visual_trans;
    Ogre::MaterialPtr           m_cab_mat_template_plain;
    Ogre::MaterialPtr           m_cab_mat_template_emissive;
};

} // Namespace RoR
