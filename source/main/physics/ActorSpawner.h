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

#include "Application.h"
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
///
/// HISTORY:
///
/// Before v0.4.5, truckfiles were parsed&spawned on-the-fly: RoR's simulation used data structures with arrays of pre-defined sizes
/// (i.e. MAX_NODES, MAX_BEAMS, MAX_* ...) and the spawner (class `SerializedRig`) wrote directly into them while reading data from the truckfile. Gfx elements were also created immediately.
/// As a result, the logic was chaotic: some features broke each other (most notably VideoCameras X MaterialFlares X SkinZips) and the sim. structs often contained parser context variables.
/// Also, the whole system was extremely sensitive to order of definitions in truckfile - often [badly/not] documented, known only by forum/IRC users at the time.
///
/// Since v0.4.5, RoR has `RigDef::Parser` which reads truckfile and emits instance of `RigDef::Document` - all data from truckfile in memory. `RigDef::Document` doesn't preserve the order of definitions,
/// instead it's designed to resolve all order-dependent references to order-independent, see `RigDef::SequentialImporter` (resources/rig_def_fileformat/RigDef_SequentialImporter.h) for more info.
/// `ActorSpawner` was created by carefully refactoring old `SerializedRig` described above, so a lot of the dirty logic remained. Elements were still written into constant-size arrays.
///
/// PRESENT (06/2017):
///
/// RoR is being refactored to get rid of the MAX_[BEAMS/NODES/***] limits. Static arrays in `rig_t` are replaced with pointers to dynamically allocated memory.
/// Memory requirements are calculated upfront from `RigDef::Document`.
///
/// CONVENTIONS:
///
/// * Functions "Process*(Definition & def)"                 Transform elements of truckfile to rig structures.
/// * Functions "FindAndProcess*(Definition & def)"          Find and process an element which should be unique in the current actor configuration.
/// * Functions "Add*()", "Create*()" or "Build*()"          Add partial structures to the actor.
/// * Functions Other functions are utilities.
///
/// @author Petr Ohlidal
class ActorSpawner
{
    friend class RoR::FlexFactory; // Needs to use `ComposeName()` and `SetupNewEntity()`

public:

    struct ActorMemoryRequirements
    {
        size_t num_nodes     = 0;
        size_t num_beams     = 0;
        size_t num_shocks    = 0;
        size_t num_rotators  = 0;
        size_t num_wings     = 0;
        size_t num_airbrakes = 0;
        size_t num_fixes     = 0;
        // ... more to come ...
    };

    enum struct Message
    {
        TYPE_INFO,
        TYPE_WARNING,
        TYPE_ERROR,
    };

    /// @name Processing
    /// @{
    void                           ConfigureSections(Ogre::String const & sectionconfig, RigDef::DocumentPtr def);
    void                           ProcessNewActor(ActorPtr actor, ActorSpawnRequest rq, RigDef::DocumentPtr def);
    static void                    SetupDefaultSoundSources(ActorPtr const& actor);
    /// @}

    /// @name Utility
    /// @{
    ActorPtr                       GetActor() { return m_actor; }
    ActorMemoryRequirements const& GetMemoryRequirements() { return m_memory_requirements; }
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

        CustomMaterial():
            material_flare_def(nullptr),
            video_camera_def(nullptr),
            mirror_prop_type(MirrorPropType::MPROP_NONE),
            mirror_prop_scenenode(nullptr)
        {}

        CustomMaterial(Ogre::MaterialPtr& mat):
            material(mat),
            material_flare_def(nullptr),
            video_camera_def(nullptr),
            mirror_prop_type(MirrorPropType::MPROP_NONE),
            mirror_prop_scenenode(nullptr)
        {}

        Ogre::MaterialPtr              material;
        RigDef::MaterialFlareBinding*  material_flare_def;
        RigDef::VideoCamera*           video_camera_def;
        MirrorPropType                 mirror_prop_type;
        Ogre::SceneNode*               mirror_prop_scenenode;
    };

    struct Exception: public std::runtime_error
    {
        Exception(Ogre::String const & message):runtime_error(message) {}
    };

    /// @name Processing actor elements
    /// @{
    // PLEASE maintain alphabetical order
    void ProcessAirbrake(RigDef::Airbrake & def);
    void ProcessAnimator(RigDef::Animator & def);
    void ProcessAntiLockBrakes(RigDef::AntiLockBrakes & def);
    void ProcessAuthor(RigDef::Author & def);
    void ProcessAxle(RigDef::Axle & def);
    void ProcessBeam(RigDef::Beam & def);
    void ProcessBrakes(RigDef::Brakes & def);
    void ProcessCameraRail(RigDef::CameraRail & def);
    void ProcessCamera(RigDef::Camera & def);
    void ProcessCinecam(RigDef::Cinecam & def);
    void ProcessCollisionBox(RigDef::CollisionBox & def);
    void ProcessCollisionRange(RigDef::CollisionRange & def);
    void ProcessCommand(RigDef::Command2 & def);
    void ProcessContacter(RigDef::Node::Ref & node_ref);
    void ProcessCruiseControl(RigDef::CruiseControl & def);
    void ProcessDescription(Ogre::String const& line);
    void ProcessEngine(RigDef::Engine & def);
    void ProcessEngoption(RigDef::Engoption & def);
    void ProcessEngturbo(RigDef::Engturbo & def);
    void ProcessExhaust(RigDef::Exhaust & def);
    void ProcessExtCamera(RigDef::ExtCamera & def);
    void ProcessFixedNode(RigDef::Node::Ref node_ref); // 'fixes'
    void ProcessFlare2(RigDef::Flare2 & def);
    void ProcessFlare3(RigDef::Flare3 & def);
    void ProcessFlexbody(RigDef::Flexbody& def);
    void ProcessFlexBodyWheel(RigDef::FlexBodyWheel & def);
    void ProcessFusedrag(RigDef::Fusedrag & def);
    void ProcessGlobals(RigDef::Globals & def);
    void ProcessGuiSettings(RigDef::GuiSettings & def);
    void ProcessHelp(RigDef::Help & def);
    void ProcessHook(RigDef::Hook & def);
    void ProcessHydro(RigDef::Hydro & def);
    void ProcessInterAxle(RigDef::InterAxle & def);
    void ProcessLockgroup(RigDef::Lockgroup & lockgroup);
    void ProcessManagedMaterial(RigDef::ManagedMaterial & def);
    void ProcessMeshWheel(RigDef::MeshWheel & def);
    void ProcessMeshWheel2(RigDef::MeshWheel2 & def);
    void ProcessMinimass(RigDef::Minimass & def);
    void ProcessNode(RigDef::Node & def);
    void ProcessParticle(RigDef::Particle & def);
    void ProcessPistonprop(RigDef::Pistonprop & def);
    void ProcessProp(RigDef::Prop & def);
    void ProcessRailGroup(RigDef::RailGroup & def);
    void ProcessRopable(RigDef::Ropable & def);
    void ProcessRope(RigDef::Rope & def);
    void ProcessRotator(RigDef::Rotator & def);
    void ProcessRotator2(RigDef::Rotator2 & def);
    void ProcessScrewprop(RigDef::Screwprop & def);
    void ProcessShock(RigDef::Shock & def);
    void ProcessShock2(RigDef::Shock2 & def);
    void ProcessShock3(RigDef::Shock3 & def);
    void ProcessSlidenode(RigDef::SlideNode & def);
    void ProcessSoundSource(RigDef::SoundSource & def);
    void ProcessSoundSource2(RigDef::SoundSource2 & def); 
    void ProcessSpeedLimiter(RigDef::SpeedLimiter& def);
    void ProcessSubmesh(RigDef::Submesh & def);
    void ProcessTie(RigDef::Tie & def);
    void ProcessTorqueCurve(RigDef::TorqueCurve & def);
    void ProcessTractionControl(RigDef::TractionControl & def);
    void ProcessTransferCase(RigDef::TransferCase & def);
    void ProcessTrigger(RigDef::Trigger & def);
    void ProcessTurbojet(RigDef::Turbojet & def);
    void ProcessTurboprop2(RigDef::Turboprop2 & def);
    void ProcessWheelDetacher(RigDef::WheelDetacher & def);
    void ProcessWheel(RigDef::Wheel & def);
    void ProcessWheel2(RigDef::Wheel2 & def);
    void ProcessWing(RigDef::Wing & def);
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

    void _ProcessSimpleInertia(RigDef::Inertia & def, RoR::SimpleInertia& obj);

    void _ProcessKeyInertia(
        RigDef::Inertia & inertia, 
        RigDef::Inertia & inertia_defaults, 
        RoR::CmdKeyInertia& contract_key, 
        RoR::CmdKeyInertia& extend_key);

    /// 'wheels', 'meshwheels'
    void BuildWheelBeams(
        unsigned int num_rays,
        NodeNum_t base_node_index,
        NodeNum_t axis_node_1,
        NodeNum_t axis_node_2,
        float tyre_spring,
        float tyre_damping,
        float rim_spring,
        float rim_damping,
        std::shared_ptr<RigDef::BeamDefaults> beam_defaults,
        RigDef::Node::Ref const & rigidity_node_id,
        float max_extension = 0.f);

    /// 'wheels', 'meshwheels', 'meshwheels2'
    BeamID_t AddWheelBeam(
        NodeNum_t node_1,
        NodeNum_t node_2,
        float spring,
        float damping,
        std::shared_ptr<RigDef::BeamDefaults> beam_defaults,
        float max_contraction = -1.f,
        float max_extension = -1.f,
        BeamType type = BEAM_NORMAL);

    /// Sets up wheel and builds nodes for sections 'wheels', 'meshwheels' and 'meshwheels2'.
    /// @param wheel_width Width of the wheel (used in section 'wheels'). Use negative value to calculate width from axis beam.
    /// @return Wheel index.
    WheelID_t BuildWheelObjectAndNodes(
        unsigned int num_rays,
        NodeNum_t axis_node_1,
        NodeNum_t axis_node_2,
        NodeNum_t reference_arm_node,
        unsigned int reserve_nodes,
        unsigned int reserve_beams,
        float wheel_radius,
        RigDef::WheelPropulsion propulsion,
        RigDef::WheelBraking braking,
        std::shared_ptr<RigDef::NodeDefaults> node_defaults,
        float wheel_mass,
        float wheel_width = -1.f);

    NodeNum_t                     AddNode(RigDef::Node::Id & id); //!< Returns NODENUM_INVALID if the ID couldn't be assigned.
    void                          InitNode(node_t & node, Ogre::Vector3 const & position);
    void                          InitNode(unsigned int node_index, Ogre::Vector3 const & position);
    void                          InitNode(node_t & node, Ogre::Vector3 const & position, std::shared_ptr<RigDef::NodeDefaults> node_defaults);
    beam_t&                       AddBeam(node_t & node_1, node_t & node_2, std::shared_ptr<RigDef::BeamDefaults> & defaults, int detacher_group);
    BeamID_t                      AddWheelRimBeam(RigDef::Wheel2 & wheel_2_def, node_t *node_1, node_t *node_2);
    BeamID_t                      AddTyreBeam(RigDef::Wheel2 & wheel_2_def, node_t *node_1, node_t *node_2);
    BeamID_t                      _SectionWheels2AddBeam(RigDef::Wheel2 & wheel_2_def, node_t *node_1, node_t *node_2);
    unsigned int                  AddWheel(RigDef::Wheel & wheel);
    unsigned int                  AddWheel2(RigDef::Wheel2 & wheel_2_def); // 'wheels2'
    void                          AddExhaust(NodeNum_t emitter_node_idx, NodeNum_t direction_node_idx);
    RailGroup*                    CreateRail(std::vector<RigDef::Node::Range> & node_ranges);
    void                          InitializeRig();
    void                          FinalizeRig();
    /// @}

    /// @name Actor building utilities
    /// @{
    void                          CalcMemoryRequirements(ActorMemoryRequirements& req, RigDef::Document::Module* module_def);    
    void                          UpdateCollcabContacterNodes();
    wheel_t::BrakeCombo           TranslateBrakingDef(RigDef::WheelBraking def);
    void                          WashCalculator();
    void                          AdjustNodeBuoyancy(node_t & node, RigDef::Node & node_def, std::shared_ptr<RigDef::NodeDefaults> defaults); //!< For user-defined nodes
    void                          AdjustNodeBuoyancy(node_t & node, std::shared_ptr<RigDef::NodeDefaults> defaults); //!< For generated nodes
    void                          InitBeam(beam_t & beam, node_t *node_1, node_t *node_2);
    void                          CalculateBeamLength(beam_t & beam);
    void                          SetBeamStrength(beam_t & beam, float strength);
    void                          SetBeamSpring(beam_t & beam, float spring);
    void                          SetBeamDamping(beam_t & beam, float damping);
    void                          SetBeamDeformationThreshold(beam_t & beam, std::shared_ptr<RigDef::BeamDefaults> beam_defaults);
    void                          ValidateRotator(int id, int axis1, int axis2, NodeNum_t *nodes1, NodeNum_t *nodes2);

    /// Creates name containing actor ID token, i.e. "Object_1@Actor_2"
    std::string                   ComposeName(const char* base, int number);

    /// Finds wheel with given axle nodes and returns it's WheelID_t index (or WHEELID_INVALID if such wheel wasn't found).
    WheelID_t                     AssignWheelToAxle(NodeNum_t axis_node_1, NodeNum_t axis_node_2);

    // GetFree*(): Gets a free slot; checks limits, sets it's array position and updates 'free_node' index.
    node_t&                       AddNode();
    beam_t&                       GetFreeBeam();
    shock_t&                      GetFreeShock();

    float ComputeWingArea(
        Ogre::Vector3 const & ref, 
        Ogre::Vector3 const & x, 
        Ogre::Vector3 const & y, 
        Ogre::Vector3 const & aref);

    /// Parses list of node-ranges into list of individual nodes.
    /// @return False if some nodes could not be found and thus the lookup wasn't completed.
    bool CollectNodesFromRanges(
        std::vector<RigDef::Node::Range> & node_ranges,
        std::vector<NodeNum_t> & out_node_indices);
    /// @}

    /// @name Traversal
    /// @{
    node_t*                       GetBeamNodePointer(RigDef::Node::Ref const & node_ref);
    NodeNum_t                     FindNodeIndex(RigDef::Node::Ref & node_ref, bool silent = false);
    NodeNum_t                     ResolveNodeRef(RigDef::Node::Ref const & node_ref);
    node_t*                       GetNodePointer(RigDef::Node::Ref const & node_ref);
    node_t*                       GetNodePointerOrThrow(RigDef::Node::Ref const & node_ref);
    beam_t&                       GetBeam(unsigned int index);
    beam_t*                       FindBeamInRig(NodeNum_t node_a, NodeNum_t node_b);
    NodeNum_t                     GetNodeIndexOrThrow(RigDef::Node::Ref const & id);
    /// @}

    /// @name Limit checks
    /// @{
    bool                          CheckParticleLimit(unsigned int count);
    bool                          CheckAxleLimit(unsigned int count);
    bool                          CheckSubmeshLimit(unsigned int count);
    bool                          CheckTexcoordLimit(unsigned int count);
    bool                          CheckCabLimit(unsigned int count);
    bool                          CheckCameraRailLimit(unsigned int count);
    bool                          CheckAeroEngineLimit(unsigned int count);
    bool                          CheckScrewpropLimit(unsigned int count);
    /// @}

    /// @name Visual setup
    /// @{
    void                          CreateBeamVisuals(beam_t const& beam, int beam_index, bool visible, std::shared_ptr<RigDef::BeamDefaults> const& beam_defaults, std::string material_override="");
    void                          CreateWheelSkidmarks(WheelID_t wheel_index);
    void                          FinalizeGfxSetup();
    Ogre::MaterialPtr             FindOrCreateCustomizedMaterial(std::string orig_name);
    Ogre::MaterialPtr             CreateSimpleMaterial(Ogre::ColourValue color);
    Ogre::ParticleSystem*         CreateParticleSystem(std::string const & name, std::string const & template_name);
    RigDef::MaterialFlareBinding* FindFlareBindingForMaterial(std::string const & material_name); //!< Returns NULL if none found
    RigDef::VideoCamera*          FindVideoCameraByMaterial(std::string const & material_name); //!< Returns NULL if none found
    void                          CreateVideoCamera(RigDef::VideoCamera* def);
    void                          CreateMirrorPropVideoCam(Ogre::MaterialPtr custom_mat, CustomMaterial::MirrorPropType type, Ogre::SceneNode* prop_scenenode);
    void                          SetupNewEntity(Ogre::Entity* e, Ogre::ColourValue simple_color); //!< Full texture and material setup
    Ogre::MaterialPtr             InstantiateManagedMaterial(Ogre::String const & source_name, Ogre::String const & clone_name);
    void                          CreateCabVisual();
    void                          CreateMaterialFlare(int flare_index, Ogre::MaterialPtr mat);

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
    static void                   AddSoundSource(ActorPtr const& vehicle, SoundScriptInstancePtr sound_script, NodeNum_t node_index, int type = -2);
    static void                   AddSoundSourceInstance(ActorPtr const& vehicle, Ogre::String const & sound_script_name, int node_index, int type = -2);
    /// @}

    /// Maintenance
    /// @{
    void                          AddMessage(Message type, Ogre::String const & text);
    void                          SetCurrentKeyword(RigDef::Keyword keyword) { m_current_keyword = keyword; }
    void                          HandleException();
    /// @}  

    struct ActorSpawnState
    {
        // Minimum node mass
        float       global_minimass = DEFAULT_MINIMASS;   //!< 'minimass' - used where 'set_default_minimass' is not applied.
    };

    /// @name Settings
    /// @{
    ActorPtr                 m_actor;
    RigDef::DocumentPtr      m_file;
    std::list<std::shared_ptr<RigDef::Document::Module>>
                             m_selected_modules;
    Ogre::Vector3            m_spawn_position;
    bool                     m_apply_simple_materials;
    std::string              m_custom_resource_group;
    bool                     m_generate_wing_position_lights;
    ActorMemoryRequirements  m_memory_requirements;
    /// @}

    /// @name State
    /// @{
    ActorSpawnState                m_state;
    std::string                    m_cab_material_name; //!< Original name defined in truckfile/globals.    
    std::string                    m_help_material_name;
    float                          m_wing_area;
    NodeNum_t                      m_airplane_left_light  = NODENUM_INVALID;
    NodeNum_t                      m_airplane_right_light = NODENUM_INVALID;
    float                          m_fuse_z_min;
    float                          m_fuse_z_max;
    float                          m_fuse_y_min;
    float                          m_fuse_y_max;    
    WingID_t                       m_first_wing_index = WINGID_INVALID;
    std::vector<CabTexcoord>       m_oldstyle_cab_texcoords;
    std::vector<CabSubmesh>        m_oldstyle_cab_submeshes;    
    RigDef::Keyword                m_current_keyword = RigDef::Keyword::INVALID; //!< For error reports    
    std::map<Ogre::String, NodeNum_t> m_named_nodes;
    int                            m_num_cameras_processed = 0; //!< Camera at index 0 is main camera and already exists, do not insert another
    /// @}

    /// @name Visuals
    /// @{
    RoR::FlexFactory                          m_flex_factory;
    std::map<std::string, CustomMaterial>     m_material_substitutions; //!< Maps original material names (shared) to their actor-specific substitutes; There's 1 substitute per 1 material, regardless of user count.
    std::map<std::string, Ogre::MaterialPtr>  m_managed_materials;
    Ogre::MaterialPtr                         m_placeholder_managedmat;
    Ogre::SceneNode*                          m_particles_parent_scenenode;
    Ogre::MaterialPtr                         m_cab_trans_material;
    Ogre::MaterialPtr                         m_simple_material_base;
    RoR::Renderdash*                          m_oldstyle_renderdash;
    CustomMaterial::MirrorPropType            m_curr_mirror_prop_type;
    Ogre::SceneNode*                          m_curr_mirror_prop_scenenode;
    /// @}
};

/// @} // addtogroup Physics

} // namespace RoR
