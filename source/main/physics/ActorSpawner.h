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

    struct Message  // TODO: remove, use console API directly
    {
        enum Type
        {
            TYPE_INFO,
            TYPE_WARNING,
            TYPE_ERROR,
            TYPE_INTERNAL_ERROR,

            TYPE_INVALID = 0xFFFFFFFF
        };
    };

    class Exception: public std::runtime_error
    {
    public:

        Exception(Ogre::String const & message):
            runtime_error(message)
        {}

    };

    void ProcessNewActor(Actor *actor, ActorSpawnRequest rq, RigDef::DocumentPtr def);

    /**
    * Adds a vehicle module to the validated configuration.
    * @param module_name A module from the validated rig-def file.
    */
    bool AddModule(Ogre::String const & module_name);

    /**
    * Adds a vehicle module to the validated configuration.
    * @param module_name A module from the validated rig-def file.
    */
    void AddModule(std::shared_ptr<RigDef::Document::Module> module)
    {
        m_selected_modules.push_back(module);
    }

    Actor *GetActor()
    {
        return m_actor;
    }

    ActorMemoryRequirements const& GetMemoryRequirements()
    {
        return m_memory_requirements;
    }

    /**
    * Finds and clones given material. Reports errors.
    * @return NULL Ogre::MaterialPtr on error.
    */
    Ogre::MaterialPtr InstantiateManagedMaterial(Ogre::String const & source_name, Ogre::String const & clone_name);

    /**
    * Finds existing node by Node::Ref; throws an exception if the node doesn't exist.
    * @return Index of existing node
    * @throws Exception If the node isn't found.
    */
    NodeNum_t GetNodeIndexOrThrow(RigDef::Node::Ref const & id);

    static void SetupDefaultSoundSources(Actor *vehicle);

    static void ComposeName(RoR::Str<100>& str, const char* type, int number, int actor_id);

    std::string GetSubmeshGroundmodelName();

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

    // TODO: This "ticket system" is a joke - remove it until resource loading is actually async!
    struct BeamVisualsTicket //!< Visuals are queued for processing using this struct
    {
        BeamVisualsTicket(int idx, float diam, const char* mtr=nullptr, bool vis=true):
            beam_index(idx), diameter(diam), material_name(mtr), visible(vis)
        {}

        int beam_index;
        std::string material_name; // TODO: how does std::string behave when parent struct is re-allocated within std::vector?  ;)
        float diameter;
        bool visible; // Some beams are spawned as hidden (ties, hooks) and displayed only when activated
    };

    // TODO: This "ticket system" is a joke - remove it until resource loading is actually async!
    struct WheelVisualsTicket //!< Wheel visuals are queued for processing using this struct
    {
        WheelVisualsTicket(uint16_t wheel_idx, uint16_t node_idx, RigDef::Wheel* def):
            wheel_index(wheel_idx), base_node_index(node_idx),
            wheel_def(def), wheel2_def(nullptr), meshwheel_def(nullptr), flexbodywheel_def(nullptr)
        {}

        WheelVisualsTicket(uint16_t wheel_idx, uint16_t node_idx, RigDef::Wheel2* def):
            wheel_index(wheel_idx), base_node_index(node_idx),
            wheel_def(nullptr), wheel2_def(def), meshwheel_def(nullptr), flexbodywheel_def(nullptr)
        {}

        WheelVisualsTicket(uint16_t wheel_idx, uint16_t node_idx, RigDef::MeshWheel* def, uint16_t axis1, uint16_t axis2):
            wheel_index(wheel_idx), base_node_index(node_idx),
            wheel_def(nullptr), wheel2_def(nullptr), meshwheel_def(def), meshwheel2_def(nullptr), flexbodywheel_def(nullptr),
            axis_node_1(axis1), axis_node_2(axis2)
        {}
        WheelVisualsTicket(uint16_t wheel_idx, uint16_t node_idx, RigDef::MeshWheel2* def, uint16_t axis1, uint16_t axis2):
            wheel_index(wheel_idx), base_node_index(node_idx),
            wheel_def(nullptr), wheel2_def(nullptr), meshwheel_def(nullptr), meshwheel2_def(def), flexbodywheel_def(nullptr),
            axis_node_1(axis1), axis_node_2(axis2)
        {}

        WheelVisualsTicket(uint16_t wheel_idx, uint16_t node_idx, RigDef::FlexBodyWheel* def, uint16_t axis1, uint16_t axis2):
            wheel_index(wheel_idx), base_node_index(node_idx),
            wheel_def(nullptr), wheel2_def(nullptr), meshwheel_def(nullptr), flexbodywheel_def(def),
            axis_node_1(axis1), axis_node_2(axis2)
        {}

        RigDef::Wheel*         wheel_def;
        RigDef::Wheel2*        wheel2_def;
        RigDef::MeshWheel*     meshwheel_def;
        RigDef::MeshWheel2*    meshwheel2_def;
        RigDef::FlexBodyWheel* flexbodywheel_def;

        uint16_t               wheel_index;
        uint16_t               base_node_index;
        uint16_t               axis_node_1;
        uint16_t               axis_node_2;
    };

/* -------------------------------------------------------------------------- */
/* Processing functions.                                                      */
/* NOTE: Please maintain alphabetical order.                                  */
/* -------------------------------------------------------------------------- */

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

/* -------------------------------------------------------------------------- */
/* Partial processing functions.                                              */
/* -------------------------------------------------------------------------- */

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
        float pitch
    );

    /**
    * Fetches free beam and sets up defaults.
    */
    beam_t & AddBeam(
        node_t & node_1, 
        node_t & node_2, 
        std::shared_ptr<RigDef::BeamDefaults> & defaults,
        int detacher_group
    );

    /**
    * Adds complete wheel (section 'wheels') to the rig.
    * @return wheel index in rig_t::wheels array.
    */
    unsigned int AddWheel(RigDef::Wheel & wheel);

    /**
    * Adds wheel from section 'wheels2'.
    * @return wheel index.
    */
    unsigned int AddWheel2(RigDef::Wheel2 & wheel_2_def);

    void CreateBeamVisuals(beam_t const& beam, int beam_index, bool visible, std::shared_ptr<RigDef::BeamDefaults> const& beam_defaults, std::string material_override="");

    RailGroup *CreateRail(std::vector<RigDef::Node::Range> & node_ranges);

    static void AddSoundSource(Actor *vehicle, SoundScriptInstance *sound_script, NodeNum_t node_index, int type = -2);

    static void AddSoundSourceInstance(Actor *vehicle, Ogre::String const & sound_script_name, int node_index, int type = -2);

/* -------------------------------------------------------------------------- */
/* Limits.                                                                    */
/* -------------------------------------------------------------------------- */

    bool CheckParticleLimit(unsigned int count);
    bool CheckAxleLimit(unsigned int count);
    bool CheckSubmeshLimit(unsigned int count);
    bool CheckTexcoordLimit(unsigned int count);
    bool CheckCabLimit(unsigned int count);
    bool CheckCameraRailLimit(unsigned int count);
    static bool CheckSoundScriptLimit(Actor *vehicle, unsigned int count);
    bool CheckAeroEngineLimit(unsigned int count);
    bool CheckScrewpropLimit(unsigned int count);

/* -------------------------------------------------------------------------- */
/* Utility functions.                                                         */
/* -------------------------------------------------------------------------- */

    /**
    * Seeks node.
    * @return Pointer to node, or nullptr if not found.
    */
    node_t* GetBeamNodePointer(RigDef::Node::Ref const & node_ref);

    /**
    * Seeks node in both RigDef::Document definition and rig_t generated rig.
    * @return Node index or -1 if the node was not found.
    */
    NodeNum_t FindNodeIndex(RigDef::Node::Ref & node_ref, bool silent = false);

    /**
    * Finds wheel with given axle nodes and returns it's index.
    * @param _out_axle_wheel Index of the found wheel.
    * @return True if wheel was found, false if not.
    */
    bool AssignWheelToAxle(int & _out_axle_wheel, node_t *axis_node_1, node_t *axis_node_2);

    float ComputeWingArea(
        Ogre::Vector3 const & ref, 
        Ogre::Vector3 const & x, 
        Ogre::Vector3 const & y, 
        Ogre::Vector3 const & aref
    );

    /**
    * Adds a node to the rig.
    * @return First: node index, second: True if the node was inserted, false if duplicate.
    */
    std::pair<unsigned int, bool> AddNode(RigDef::Node::Id & id);

    /**
    * Adds a message to internal log.
    */
    void AddMessage(Message::Type type, Ogre::String const & text);

    void AddExhaust(
        unsigned int emitter_node_idx,
        unsigned int direction_node_idx
    );

    /**
    * Finds existing node by Node::Ref. Returns NODENUM_INVALID if not found.
    */
    NodeNum_t ResolveNodeRef(RigDef::Node::Ref const & node_ref);

    /**
    * Finds existing node by Node::Ref
    * @return Pointer to node or nullptr if not found.
    */
    node_t* GetNodePointer(RigDef::Node::Ref const & node_ref);

    /**
    * Finds existing node by Node::Ref
    * @return Pointer to node
    * @throws Exception If the node isn't found.
    */
    node_t* GetNodePointerOrThrow(RigDef::Node::Ref const & node_ref);

    /**
    * Sets up defaults & position of a node.
    */
    void InitNode(node_t & node, Ogre::Vector3 const & position);

    /**
    * Sets up defaults & position of a node.
    */
    void InitNode(unsigned int node_index, Ogre::Vector3 const & position);

    /**
    * Sets up defaults & position of a node.
    */
    void InitNode(
        node_t & node, 
        Ogre::Vector3 const & position,
        std::shared_ptr<RigDef::NodeDefaults> node_defaults
    );

    /**
    * Setter.
    */
    void SetCurrentKeyword(RigDef::Keyword keyword)
    {
        m_current_keyword = keyword;
    }

    beam_t & GetBeam(unsigned int index);

    /**
    * Parses list of node-ranges into list of individual nodes.
    * @return False if some nodes could not be found and thus the lookup wasn't completed.
    */
    bool CollectNodesFromRanges(
        std::vector<RigDef::Node::Range> & node_ranges,
        std::vector<NodeNum_t> & out_node_indices
    );

    /**
    * Gets a free node slot; checks limits, sets it's array position and updates 'free_node' index.
    * @return A reference to node slot.
    */
    node_t & GetFreeNode();

    /**
    * Gets a free beam slot; checks limits, sets it's array position and updates 'free_beam' index.
    * @return A reference to beam slot.
    */
    beam_t & GetFreeBeam();

    /**
    * Gets a free beam slot; Sets up defaults & position of a node.
    * @return A reference to node slot.
    */
    node_t & GetAndInitFreeNode(Ogre::Vector3 const & position);

    /**
    * Gets a free beam slot; checks limits, sets it's array position and updates 'rig_t::free_beam' index.
    * @return A reference to beam slot.
    */
    beam_t & GetAndInitFreeBeam(node_t & node_1, node_t & node_2);

    shock_t & GetFreeShock();

    /**
    * Sets up nodes & length of a beam.
    */
    void InitBeam(beam_t & beam, node_t *node_1, node_t *node_2);

    void CalculateBeamLength(beam_t & beam);

    void SetBeamStrength(beam_t & beam, float strength);

    void SetBeamSpring(beam_t & beam, float spring);

    void SetBeamDamping(beam_t & beam, float damping);

    beam_t *FindBeamInRig(NodeNum_t node_a, NodeNum_t node_b);

    void SetBeamDeformationThreshold(beam_t & beam, std::shared_ptr<RigDef::BeamDefaults> beam_defaults);

    void UpdateCollcabContacterNodes();

    wheel_t::BrakeCombo TranslateBrakingDef(RigDef::WheelBraking def);

    /**
    * Checks a section only appears in one module and reports a warning if not.
    */
    void CheckSectionSingleModule(
        Ogre::String const & section_name,
        std::list<std::shared_ptr<RigDef::Document::Module>> & found_items	
    );

    /**
    * Creates beam pre-configured for use as rim with section 'wheels2'.
    */
    unsigned int AddWheelRimBeam(RigDef::Wheel2 & wheel_2_def, node_t *node_1, node_t *node_2);

    /**
    * Creates beam pre-configured for use as tyre with section 'wheels2'.
    */
    unsigned int AddTyreBeam(RigDef::Wheel2 & wheel_2_def, node_t *node_1, node_t *node_2);

    /**
    * Creates beam partially configured for use with section 'wheels2'.
    */
    unsigned int _SectionWheels2AddBeam(RigDef::Wheel2 & wheel_2_def, node_t *node_1, node_t *node_2);

    /**
    * Builds complete wheel visuals (sections 'wheels', 'wheels2').
    * @param rim_ratio Percentual size of the rim.
    */
    void CreateWheelVisuals(
        unsigned int wheel_index, 
        unsigned int node_base_index,
        unsigned int def_num_rays,
        Ogre::String const & rim_material_name,
        Ogre::String const & band_material_name,
        bool separate_rim,
        float rim_ratio = 1.f
    );

    void CreateWheelSkidmarks(unsigned int wheel_index);

    /**
    * Performs full material setup for a new entity.
    * RULE: Each actor must have it's own material instances (a lookup table is kept for OrigName->CustomName)
    *
    * Setup routine:
    *
    *   1. If "SimpleMaterials" (plain color surfaces denoting component type) are enabled in config file, 
    *          material is generated (not saved to lookup table) and processing ends.
    *   2. If the material name is 'mirror', it's a special prop - rear view mirror.
    *          material is generated, added to lookup table under generated name (special case) and processing ends.
    *   3. If the material is a 'videocamera' of any subtype, material is created, added to lookup table and processing ends.
    *   4  'materialflarebindngs' are resolved -> binding is persisted in lookup table.
    *   5  SkinZIP _material replacements_ are queried. If match is found, it's added to lookup table and processing ends.
    *   6. ManagedMaterials are queried. If match is found, it's added to lookup table and processing ends.
    *   7. Orig. material is cloned to create substitute.
    *   8. SkinZIP _texture replacements_ are queried. If match is found, substitute material is updated.
    *   9. Material is added to lookup table, processing ends.
    */
    void SetupNewEntity(Ogre::Entity* e, Ogre::ColourValue simple_color);

    /**
    * Factory of GfxActor; invoke after all gfx setup was done.
    */
    void FinalizeGfxSetup();

    /**
    * Validator for the rotator reference structure
    */
    void ValidateRotator(int id, int axis1, int axis2, NodeNum_t *nodes1, NodeNum_t *nodes2);

    /**
    * Helper for 'SetupNewEntity()' - see it's doc.
    */
    Ogre::MaterialPtr FindOrCreateCustomizedMaterial(std::string orig_name);

    Ogre::MaterialPtr CreateSimpleMaterial(Ogre::ColourValue color);

    Ogre::ParticleSystem* CreateParticleSystem(std::string const & name, std::string const & template_name);

    RigDef::MaterialFlareBinding* FindFlareBindingForMaterial(std::string const & material_name); //!< Returns NULL if none found

    RigDef::VideoCamera* FindVideoCameraByMaterial(std::string const & material_name); //!< Returns NULL if none found

    void CreateVideoCamera(RigDef::VideoCamera* def);
    void CreateMirrorPropVideoCam(Ogre::MaterialPtr custom_mat, CustomMaterial::MirrorPropType type, Ogre::SceneNode* prop_scenenode);

    /**
    * Creates name containing actor ID token, i.e. "Object_1@Actor_2"
    */
    std::string ComposeName(const char* base, int number);

    /**
    * Sets up wheel and builds nodes for sections 'wheels', 'meshwheels' and 'meshwheels2'.
    * @param wheel_width Width of the wheel (used in section 'wheels'). Use negative value to calculate width from axis beam.
    * @return Wheel index.
    */
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
        std::shared_ptr<RigDef::NodeDefaults> node_defaults,
        float wheel_mass,
        float wheel_width = -1.f
    );

    /**
    * Adds beams to wheels from 'wheels', 'meshwheels'
    */
    void BuildWheelBeams(
        unsigned int num_rays,
        unsigned int base_node_index,
        node_t *axis_node_1,
        node_t *axis_node_2,
        float tyre_spring,
        float tyre_damping,
        float rim_spring,
        float rim_damping,
        std::shared_ptr<RigDef::BeamDefaults> beam_defaults,
        RigDef::Node::Ref const & rigidity_node_id,
        float max_extension = 0.f
    );

    /**
    * Creates beam for wheels 'wheels', 'meshwheels', 'meshwheels2'
    */
    unsigned int AddWheelBeam(
        node_t *node_1,
        node_t *node_2,
        float spring,
        float damping,
        std::shared_ptr<RigDef::BeamDefaults> beam_defaults,
        float max_contraction = -1.f,
        float max_extension = -1.f,
        BeamType type = BEAM_NORMAL
    );

    /**
    * Builds wheel visuals (sections 'meshwheels', 'meshwheels2').
    */
    void BuildMeshWheelVisuals(
        unsigned int wheel_index,
        unsigned int base_node_index,
        unsigned int axis_node_1_index,
        unsigned int axis_node_2_index,
        unsigned int num_rays,
        Ogre::String mesh_name,
        Ogre::String material_name,
        float rim_radius,
        bool rim_reverse	
    );

    /**
    * From SerializedRig::wash_calculator()
    */
    void WashCalculator();

    void _ProcessKeyInertia(
        RigDef::Inertia & inertia, 
        RigDef::Inertia & inertia_defaults, 
        RoR::CmdKeyInertia& contract_key, 
        RoR::CmdKeyInertia& extend_key
    );

    /** 
    * For specified nodes
    */
    void AdjustNodeBuoyancy(node_t & node, RigDef::Node & node_def, std::shared_ptr<RigDef::NodeDefaults> defaults);

    /** 
    * For generated nodes
    */
    void AdjustNodeBuoyancy(node_t & node, std::shared_ptr<RigDef::NodeDefaults> defaults);

    /**
    * Ported from SerializedRig::loadTruck() [v0.4.0.7]
    */
    void FinalizeRig();

    /**
    * Ported from SerializedRig::SerializedRig() [v0.4.0.7]
    */
    void InitializeRig();

    void CalcMemoryRequirements(ActorMemoryRequirements& req, RigDef::Document::Module* module_def);

    void HandleException();

    struct ActorSpawnState
    {
        // Minimum node mass
        float       global_minimass = DEFAULT_MINIMASS;   //!< 'minimass' - used where 'set_default_minimass' is not applied.
    };

    // Spawn
    Actor*             m_actor; //!< The output actor.
    ActorSpawnState    m_state;
    Ogre::Vector3      m_spawn_position;
    bool               m_apply_simple_materials;
    std::string        m_cab_material_name; //!< Original name defined in truckfile/globals.
    std::string        m_custom_resource_group;
    std::string        m_help_material_name;
    float              m_wing_area;
    int                m_airplane_left_light;
    int                m_airplane_right_light;
    RoR::FlexFactory   m_flex_factory;
    Ogre::MaterialPtr  m_placeholder_managedmat;
    Ogre::SceneNode*   m_particles_parent_scenenode;
    Ogre::MaterialPtr  m_cab_trans_material;
    Ogre::MaterialPtr  m_simple_material_base;
    RoR::Renderdash*   m_oldstyle_renderdash;
    float              m_fuse_z_min;
    float              m_fuse_z_max;
    float              m_fuse_y_min;
    float              m_fuse_y_max;
    bool               m_generate_wing_position_lights;
    int                m_first_wing_index;
    Ogre::SceneNode*   m_curr_mirror_prop_scenenode;
    std::vector<RoR::Prop>    m_props;
    int                       m_driverseat_prop_index;
    std::vector<CabTexcoord>  m_oldstyle_cab_texcoords;
    std::vector<CabSubmesh>   m_oldstyle_cab_submeshes;
    ActorMemoryRequirements   m_memory_requirements;
    RigDef::Keyword     m_current_keyword; //!< For error reports
    CustomMaterial::MirrorPropType         m_curr_mirror_prop_type;
    RigDef::DocumentPtr          m_file; //!< The parsed input file.
    std::map<Ogre::String, unsigned int>   m_named_nodes;
    std::map<std::string, CustomMaterial>  m_material_substitutions; //!< Maps original material names (shared) to their actor-specific substitutes; There's 1 substitute per 1 material, regardless of user count.
    std::vector<BeamVisualsTicket>         m_beam_visuals_queue; //!< We want to spawn visuals asynchronously in the future
    std::vector<WheelVisualsTicket>        m_wheel_visuals_queue; //!< We want to spawn visuals asynchronously in the future
    std::map<std::string, Ogre::MaterialPtr>  m_managed_materials;
    std::list<std::shared_ptr<RigDef::Document::Module>>  m_selected_modules;

};

/// @} // addtogroup Physics

} // namespace RoR
