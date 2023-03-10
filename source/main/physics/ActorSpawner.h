/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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
/// @brief  Vehicle spawning logic.
/// @author Petr Ohlidal
/// @date   12/2013


#pragma once

#include "ActorSpawnContext.h"
#include "Application.h"
#include "CacheSystem.h"
#include "RigDef_File.h"
#include "RigDef_Parser.h"
#include "SimData.h"
#include "FlexFactory.h"
#include "FlexObj.h"
#include "GfxActor.h"

#include <OgreString.h>
#include <string>

namespace RoR {

/// @addtogroup Physics
/// @{

/// Processes a RigDef::Document (parsed from 'truck' file format) into a simulated gameplay object (Actor).
class ActorSpawner
{
    friend class RoR::FlexFactory; // Needs to use `ComposeName()` and `SetupNewEntity()`

public:

    enum struct Message
    {
        TYPE_INFO,
        TYPE_WARNING,
        TYPE_ERROR,
    };

    /// @name Processing
    /// @{
    void                           ProcessNewActor(ActorPtr actor, ActorSpawnRequest rq, RigDef::DocumentPtr def);
    static void                    SetupDefaultSoundSources(ActorPtr const& actor);
    /// @}

    /// @name Utility
    /// @{
    ActorPtr                       GetActor() { return m_actor; }
    void                           FillCacheConfigInfo(RigDef::DocumentPtr document, std::string config, CacheActorConfigInfo& info);
    std::string                    GetSubmeshGroundmodelName();
    /// @}

private:

    struct CustomMaterial
    {
        enum class MirrorPropType
        {
            MPROP_NONE,
            MPROP_LEFT,
            MPROP_RIGHT,
        };

        CustomMaterial(){}

        CustomMaterial(Ogre::MaterialPtr& mat):
            material(mat)
        {}

        Ogre::MaterialPtr              material;
        RigDef::DataPos_t              material_flare_pos = RigDef::DATAPOS_INVALID;
        RigDef::DataPos_t              video_camera_pos = RigDef::DATAPOS_INVALID;
        MirrorPropType                 mirror_prop_type = MirrorPropType::MPROP_NONE;
        Ogre::SceneNode*               mirror_prop_scenenode = nullptr;
    };

    struct Exception: public std::runtime_error
    {
        Exception(Ogre::String const & message):runtime_error(message) {}
    };

    /// @name Processing actor elements
    /// @{
    // PLEASE maintain alphabetical order
    void ProcessAddAnimation(RigDef::DataPos_t pos);
    void ProcessAirbrake(RigDef::DataPos_t pos);
    void ProcessAnimator(RigDef::DataPos_t pos);
    void ProcessAntiLockBrakes(RigDef::DataPos_t pos);
    void ProcessAuthor(RigDef::DataPos_t pos);
    void ProcessAxle(RigDef::DataPos_t pos);
    void ProcessBeam(RigDef::DataPos_t pos);
    void ProcessBeamDefaults(RigDef::DataPos_t pos);
    void ProcessBeamDefaultsScale(RigDef::DataPos_t pos);
    void ProcessBrakes(RigDef::DataPos_t pos);
    void ProcessCab(RigDef::DataPos_t pos);
    void ProcessCameraRail(RigDef::DataPos_t pos);
    void ProcessCamera(RigDef::DataPos_t pos);
    void ProcessCinecam(RigDef::DataPos_t pos);
    void ProcessCollisionBox(RigDef::DataPos_t pos);
    void ProcessCollisionRange(RigDef::DataPos_t pos);
    void ProcessCommand(RigDef::DataPos_t pos);
    void ProcessCommand2(RigDef::DataPos_t pos);
    void ProcessContacter(RigDef::DataPos_t pos);
    void ProcessCruiseControl(RigDef::DataPos_t pos);
    void ProcessDescription(RigDef::DataPos_t pos);
    void ProcessDetacherGroup(RigDef::DataPos_t pos);
    void ProcessEndSection();
    void ProcessEngine(RigDef::DataPos_t pos);
    void ProcessEngoption(RigDef::DataPos_t pos);
    void ProcessEngturbo(RigDef::DataPos_t pos);
    void ProcessExhaust(RigDef::DataPos_t pos);
    void ProcessExtCamera(RigDef::DataPos_t pos);
    void ProcessFixes(RigDef::DataPos_t pos);
    void ProcessFlare(RigDef::DataPos_t pos);
    void ProcessFlare2(RigDef::DataPos_t pos);
    void ProcessFlare3(RigDef::DataPos_t pos);
    void ProcessFlexbody(RigDef::DataPos_t pos);
    void ProcessFlexbodyCameraMode(RigDef::DataPos_t pos);
    void ProcessFlexBodyWheel(RigDef::DataPos_t pos);
    void ProcessForset(RigDef::DataPos_t pos);
    void ProcessFusedrag(RigDef::DataPos_t pos);
    void ProcessGlobals(RigDef::DataPos_t pos);
    void ProcessGuiSettings(RigDef::DataPos_t pos);
    void ProcessHelp(RigDef::DataPos_t pos);
    void ProcessHook(RigDef::DataPos_t pos);
    void ProcessHydro(RigDef::DataPos_t pos);
    void ProcessInertiaDefaults(RigDef::DataPos_t pos);
    void ProcessInterAxle(RigDef::DataPos_t pos);
    void ProcessLockgroup(RigDef::DataPos_t pos);
    void ProcessLockgroupDefaultNolock();
    void ProcessManagedMaterial(RigDef::DataPos_t pos);
    void ProcessManagedMatOptions(RigDef::DataPos_t pos);
    void ProcessMeshWheel(RigDef::DataPos_t pos);
    void ProcessMeshWheel2(RigDef::DataPos_t pos);
    void ProcessMinimass(RigDef::DataPos_t pos);
    void ProcessNode(RigDef::DataPos_t pos);
    void ProcessNode2(RigDef::DataPos_t pos);
    void ProcessNodeDefaults(RigDef::DataPos_t pos);
    void ProcessParticle(RigDef::DataPos_t pos);
    void ProcessPistonprop(RigDef::DataPos_t pos);
    void ProcessProp(RigDef::DataPos_t pos);
    void ProcessPropCameraMode(RigDef::DataPos_t pos);
    void ProcessRailGroup(RigDef::DataPos_t pos);
    void ProcessRopable(RigDef::DataPos_t pos);
    void ProcessRope(RigDef::DataPos_t pos);
    void ProcessRotator(RigDef::DataPos_t pos);
    void ProcessRotator2(RigDef::DataPos_t pos);
    void ProcessScrewprop(RigDef::DataPos_t pos);
    void ProcessSection(RigDef::DataPos_t pos);
    void ProcessShock(RigDef::DataPos_t pos);
    void ProcessShock2(RigDef::DataPos_t pos);
    void ProcessShock3(RigDef::DataPos_t pos);
    void ProcessSkeletonSettings(RigDef::DataPos_t pos);
    void ProcessSlidenode(RigDef::DataPos_t pos);
    void ProcessSlidenodeConnectInstantly();
    void ProcessSoundSource(RigDef::DataPos_t pos);
    void ProcessSoundSource2(RigDef::DataPos_t pos);
    void ProcessSpeedLimiter(RigDef::DataPos_t pos);
    void ProcessSubmesh();
    void ProcessSubmeshGroundModel(RigDef::DataPos_t pos);
    void ProcessTexcoord(RigDef::DataPos_t pos);
    void ProcessTie(RigDef::DataPos_t pos);
    void ProcessTorqueCurve(RigDef::DataPos_t pos);
    void ProcessTractionControl(RigDef::DataPos_t pos);
    void ProcessTransferCase(RigDef::DataPos_t pos);
    void ProcessTrigger(RigDef::DataPos_t pos);
    void ProcessTurbojet(RigDef::DataPos_t pos);
    void ProcessTurboprop(RigDef::DataPos_t pos);
    void ProcessTurboprop2(RigDef::DataPos_t pos);
    void ProcessWheelDetacher(RigDef::DataPos_t pos);
    void ProcessWheel(RigDef::DataPos_t pos);
    void ProcessWheel2(RigDef::DataPos_t pos);
    void ProcessWing(RigDef::DataPos_t pos);
    /// @}

    /// @name Actor building functions
    /// @{

    void BuildAeroEngine(
        NodeNum_t ref_node_index,
        NodeNum_t back_node_index,
        NodeNum_t blade_1_node_index,
        NodeNum_t blade_2_node_index,
        NodeNum_t blade_3_node_index,
        NodeNum_t blade_4_node_index,
        NodeNum_t couplenode_index,
        bool is_turboprops,
        Ogre::String const & airfoil,
        float power,
        float pitch);

    void _ProcessSimpleInertia(RoR::SimpleInertia& obj);

    void _ProcessKeyInertia(
        RigDef::Inertia & inertia, 
        RoR::CmdKeyInertia& contract_key, 
        RoR::CmdKeyInertia& extend_key);

    /// 'wheels', 'meshwheels'
    void BuildWheelBeams(
        unsigned int num_rays,
        unsigned int base_node_index,
        node_t *axis_node_1,
        node_t *axis_node_2,
        float tyre_spring,
        float tyre_damping,
        float rim_spring,
        float rim_damping,
        RigDef::NodeRef_t const & rigidity_node_id,
        float max_extension = 0.f);

    /// 'wheels', 'meshwheels', 'meshwheels2'
    unsigned int AddWheelBeam(
        node_t *node_1,
        node_t *node_2,
        float spring,
        float damping,
        float max_contraction = -1.f,
        float max_extension = -1.f,
        BeamType type = BEAM_NORMAL);

    /// Sets up wheel and builds nodes for sections 'wheels', 'meshwheels' and 'meshwheels2'.
    /// @param wheel_width Width of the wheel (used in section 'wheels'). Use negative value to calculate width from axis beam.
    /// @return Wheel index.
    unsigned int BuildWheelObjectAndNodes(
        unsigned int num_rays,
        node_t *axis_node_1,
        node_t *axis_node_2,
        node_t *reference_arm_node,
        unsigned int reserve_nodes,
        unsigned int reserve_beams,
        float wheel_radius,
        RigDef::WheelPropulsion propulsion,
        RigDef::WheelBraking braking,
        float wheel_mass,
        float wheel_width = -1.f);

    void                          AddNode(RigDef::NodesCommon& def, std::string const& node_name, NodeNum_t node_number);
    void                          InitNode(node_t & node, Ogre::Vector3 const & position);
    void                          InitNode(unsigned int node_index, Ogre::Vector3 const & position);
    beam_t&                       AddBeam(node_t & node_1, node_t & node_2);
    beam_t&                       AddBeam(RigDef::NodeRef_t n1, RigDef::NodeRef_t n2);
    unsigned int                  AddWheels2RimBeam(RigDef::DataPos_t pos, node_t *node_1, node_t *node_2);
    unsigned int                  AddWheels2TyreBeam(RigDef::DataPos_t pos, node_t *node_1, node_t *node_2);
    unsigned int                  AddWheels2Beam(RigDef::DataPos_t pos, node_t *node_1, node_t *node_2);
    unsigned int                  AddWheel(RigDef::DataPos_t pos); //!< @return wheel index in rig_t::wheels array.
    unsigned int                  AddWheel2(RigDef::DataPos_t pos); //!< @return wheel index in rig_t::wheels array.
    void                          AddCommand(RigDef::CommandCommon& def, float shorten_rate, float lenghten_rate);
    void                          AddExhaust(NodeNum_t emitter_node_idx, NodeNum_t direction_node_idx);
    RailGroup*                    CreateRail(std::vector<RigDef::NodeRange> & node_ranges);
    void                          InitializeRig(CacheActorConfigInfo& config_info);
    void                          FinalizeRig();
    /// @}

    /// @name Actor building utilities
    /// @{
    void                          UpdateCollcabContacterNodes();
    wheel_t::BrakeCombo           TranslateBrakingDef(RigDef::WheelBraking def);
    void                          WashCalculator();
    void                          AdjustNodeBuoyancy(node_t & node, RigDef::NodesCommon & node_def);
    void                          AdjustNodeBuoyancy(node_t & node);
    void                          InitBeam(beam_t & beam, node_t *node_1, node_t *node_2);
    void                          CalculateBeamLength(beam_t & beam);
    void                          SetBeamSpring(beam_t & beam, float spring);
    void                          SetBeamDamping(beam_t & beam, float damping);
    void                          ValidateRotator(int id, int axis1, int axis2, NodeNum_t *nodes1, NodeNum_t *nodes2);
    bool                          ValidateTrigger(RigDef::Trigger& def);
    bool                          ShouldProcessSection(RigDef::DataPos_t pos);
    
    /// Creates name containing actor ID token, i.e. "Object_1@Actor_2"
    std::string                   ComposeName(const char* base, int number);

    /// Finds wheel with given axle nodes and returns it's index.
    /// @param _out_axle_wheel Index of the found wheel.
    /// @return True if wheel was found, false if not.
    bool                          AssignWheelToAxle(int & _out_axle_wheel, node_t *axis_node_1, node_t *axis_node_2);
    
    // GetFree*(): Gets a free slot; checks limits, sets it's array position and updates 'free_node' index.
    node_t&                       GetFreeNode();
    beam_t&                       GetFreeBeam();
    node_t&                       GetAndInitFreeNode(Ogre::Vector3 const & position);
    beam_t&                       GetAndInitFreeBeam(node_t & node_1, node_t & node_2);
    shock_t&                      GetFreeShock();

    void ProcessFlareCommon(RigDef::Flare2& def);    
    void ProcessBackmesh();

    float ComputeWingArea(
        Ogre::Vector3 const & ref, 
        Ogre::Vector3 const & x, 
        Ogre::Vector3 const & y, 
        Ogre::Vector3 const & aref);

    void ResolveNodeRanges(
        std::vector<NodeNum_t> & out_nodes,
        std::vector<RigDef::NodeRange> & in_ranges
    );

    void FetchAxisNodes(
        node_t* & axis_node_1, 
        node_t* & axis_node_2, 
        RigDef::NodeRef_t const & axis_node_1_id,
        RigDef::NodeRef_t const & axis_node_2_id
    );

    /// @name Traversal
    /// @{
    NodeNum_t                     ResolveNodeRef(RigDef::NodeRef_t const & node_ref);
    node_t*                       GetNodePointer(RigDef::NodeRef_t const & node_ref);
    node_t*                       GetNodePointerOrThrow(RigDef::NodeRef_t const & node_ref);
    beam_t&                       GetBeam(unsigned int index);
    beam_t*                       FindBeamInRig(NodeNum_t node_a, NodeNum_t node_b);
    NodeNum_t                     GetNodeIndexOrThrow(RigDef::NodeRef_t const & id);
    /// @}

    /// @name Limit checks
    /// @{
    bool                          CheckParticleLimit(unsigned int count);
    bool                          CheckAxleLimit(unsigned int count);
    bool                          CheckSubmeshLimit(unsigned int count);
    bool                          CheckTexcoordLimit(unsigned int count);
    bool                          CheckCabLimit(unsigned int count);
    bool                          CheckCameraRailLimit(unsigned int count);
    static bool                   CheckSoundScriptLimit(ActorPtr const& vehicle, unsigned int count);
    bool                          CheckAeroEngineLimit(unsigned int count);
    bool                          CheckScrewpropLimit(unsigned int count);
    /// @}

    /// @name Visual setup
    /// @{
    void                          CreateBeamVisuals(beam_t const& beam, int beam_index, bool visible, std::string material_override="");
    void                          CreateWheelSkidmarks(unsigned int wheel_index);
    void                          FinalizeGfxSetup();
    Ogre::MaterialPtr             FindOrCreateCustomizedMaterial(std::string orig_name);
    Ogre::MaterialPtr             CreateSimpleMaterial(Ogre::ColourValue color);
    Ogre::ParticleSystem*         CreateParticleSystem(std::string const & name, std::string const & template_name);
    RigDef::DataPos_t             FindFlareBindingForMaterial(std::string const & material_name); //!< Returns DATAPOS_INVALID if none found
    RigDef::DataPos_t             FindVideoCameraByMaterial(std::string const & material_name); //!< Returns DATAPOS_INVALID if none found
    void                          CreateVideoCamera(RigDef::DataPos_t pos);
    void                          CreateMirrorPropVideoCam(Ogre::MaterialPtr custom_mat, CustomMaterial::MirrorPropType type, Ogre::SceneNode* prop_scenenode);
    void                          SetupNewEntity(Ogre::Entity* e, Ogre::ColourValue simple_color); //!< Full texture and material setup
    Ogre::MaterialPtr             InstantiateManagedMaterial(Ogre::String const & source_name, Ogre::String const & clone_name);
    void                          CreateCabVisual();
    void                          CloseCurrentSubmesh(CabSubmesh::BackmeshType type);
    void                          CreateMaterialFlare(int flare_index, Ogre::MaterialPtr mat);
    void                          BuildFlexbody(RigDef::DataPos_t flexbodies_data_pos, RigDef::DataPos_t forset_data_pos);

    /// @param rim_ratio Percentual size of the rim.
    void CreateWheelVisuals(
        unsigned int wheel_index, 
        unsigned int node_base_index,
        unsigned int def_num_rays,
        Ogre::String const & rim_material_name,
        Ogre::String const & band_material_name,
        bool separate_rim,
        float rim_ratio = 1.f
    );

    void CreateFlexBodyWheelVisuals(
        unsigned int wheel_index, 
        unsigned int node_base_index,
        NodeNum_t axis_node_1,
        NodeNum_t axis_node_2,
        RigDef::FlexBodyWheel& def);

    //  (sections 'meshwheels', 'meshwheels2').
    void BuildMeshWheelVisuals(
        unsigned int wheel_index,
        unsigned int base_node_index,
        unsigned int axis_node_1_index,
        unsigned int axis_node_2_index,
        unsigned int num_rays,
        Ogre::String mesh_name,
        Ogre::String material_name,
        float rim_radius,
        bool rim_reverse);
    /// @}

    /// @name Audio setup
    /// @{
    static void                   AddSoundSource(ActorPtr const& vehicle, SoundScriptInstance *sound_script, NodeNum_t node_index, int type = -2);
    static void                   AddSoundSourceInstance(ActorPtr const& vehicle, Ogre::String const & sound_script_name, int node_index, int type = -2);
    /// @}
    
    /// Maintenance
    /// @{
    void                          AddMessage(Message type, Ogre::String const & text);
    void                          SetCurrentKeyword(RigDef::Keyword keyword) { m_current_keyword = keyword; }
    void                          HandleException();
    /// @}

    /// @name Settings
    /// @{
    ActorPtr                 m_actor;
    RigDef::DocumentPtr      m_document;
    std::string              m_selected_config;
    Ogre::Vector3            m_spawn_position;
    bool                     m_apply_simple_materials;
    std::string              m_custom_resource_group;
    bool                     m_generate_wing_position_lights;
    /// @}
    
    /// @name State
    /// @{
    RigDef::DataPos_t        m_pending_flexbody = RigDef::DATAPOS_INVALID; //!< set by 'flexbody', reset by 'forset'
    FlexBody*                m_last_flexbody = nullptr;
    ActorSpawnContext        m_state;
    bool                     m_skipping_section = false;
    std::string              m_cab_material_name; //!< Original name defined in truckfile/globals.
    std::string              m_help_material_name; //!< `help`, or `guisettings`
    float                    m_wing_area;
    int                      m_airplane_left_light;
    int                      m_airplane_right_light;
    int                      m_first_wing_index;
    std::vector<CabTexcoord>       m_oldstyle_cab_texcoords;
    std::vector<CabSubmesh>        m_oldstyle_cab_submeshes;    
    RigDef::Keyword          m_current_keyword; //!< For error reports    
    std::map<std::string, NodeNum_t> m_node_names;
    /// @}
    
    /// @name Visuals
    /// @{
    RoR::FlexFactory                          m_flex_factory;
    std::map<std::string, CustomMaterial>     m_material_substitutions; //!< Maps original material names (shared) to their actor-specific substitutes; There's 1 substitute per 1 material, regardless of user count.
    std::map<std::string, Ogre::MaterialPtr>  m_managed_materials;
    Ogre::MaterialPtr                         m_placeholder_managedmat;
    Ogre::SceneNode*                          m_particles_parent_scenenode = nullptr;
    Ogre::MaterialPtr                         m_cab_trans_material;
    Ogre::MaterialPtr                         m_simple_material_base;
    RoR::Renderdash*                          m_oldstyle_renderdash;
    CustomMaterial::MirrorPropType            m_curr_mirror_prop_type;
    Ogre::SceneNode*                          m_curr_mirror_prop_scenenode;
    int                                       m_driverseat_prop_index = -1;
    /// @}
};

/// @} // addtogroup Physics

} // namespace RoR
