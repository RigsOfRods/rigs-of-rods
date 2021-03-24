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
#include "TruckParser.h"
#include "SimData.h"
#include "FlexFactory.h"
#include "FlexObj.h"
#include "GfxActor.h"

#include <OgreString.h>
#include <string>

namespace RoR {

/// Processes a Truck::Document data structure (result of parsing a "Truckfile" fileformat) into 'an Actor' - a simulated physical object.
///
/// HISTORY:
///
/// Before v0.4.5, truckfiles were parsed&spawned on-the-fly: RoR's simulation used data structures with arrays of pre-defined sizes
/// (i.e. MAX_NODES, MAX_BEAMS, MAX_* ...) and the spawner (class `SerializedRig`) wrote directly into them while reading data from the truckfile. Gfx elements were also created immediately.
/// As a result, the logic was chaotic: some features broke each other (most notably VideoCameras X MaterialFlares X SkinZips) and the sim. structs often contained parser context variables.
/// Also, the whole system was extremely sensitive to order of definitions in truckfile - often [badly/not] documented, known only by forum/IRC users at the time.
///
/// Since v0.4.5, RoR has `Truck::Parser` which reads truckfile and emits instance of `Truck::Document` - all data from truckfile in memory. `Truck::Document` doesn't preserve the order of definitions,
/// instead it's designed to resolve all order-dependent references to order-independent, see `Truck::SequentialImporter` (resources/rig_def_fileformat/RigDef_SequentialImporter.h) for more info.
/// `ActorSpawner` was created by carefully refactoring old `SerializedRig` described above, so a lot of the dirty logic remained. Elements were still written into constant-size arrays.
///
/// PRESENT (06/2017):
///
/// RoR is being refactored to get rid of the MAX_[BEAMS/NODES/***] limits. Static arrays in `rig_t` are replaced with pointers to dynamically allocated memory.
/// Memory requirements are calculated upfront from `Truck::Document`.
///
/// FUTURE:
///
/// ActorSpawner will work in 2 steps:
///  1. Physics/simulation data are fully prepared. This should be very fast (we can pre-calculate and cache things if needed).
///  2. Graphics/sounds are set up, reading the completed physics/sim data. Graphics are fully managed by `GfxActor`. Similar utility will be added for sound.
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

    ActorSpawner() {}

    struct ActorMemoryRequirements
    {
        ActorMemoryRequirements() { memset(this,0, sizeof(ActorMemoryRequirements)); }

        size_t num_nodes;
        size_t num_beams;
        size_t num_shocks;
        size_t num_rotators;
        size_t num_wings;
        size_t num_airbrakes;
        size_t num_fixes;
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

    void Setup(
        Actor *actor,
        Truck::DocumentPtr file,
        Ogre::SceneNode *parent,
        Ogre::Vector3 const & spawn_position
        );

    Actor *SpawnActor(std::string const& config);


    /**
    * Adds a vehicle module to the validated configuration.
    * @param module_name A module from the validated rig-def file.
    */
    void AddModule(Truck::ModulePtr module)
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
    NodeIdx_t GetNodeIndexOrThrow(Truck::Node::Ref const & id);

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
        Truck::MaterialFlareBinding*  material_flare_def;
        Truck::VideoCamera*           video_camera_def;
        MirrorPropType                 mirror_prop_type;
        Ogre::SceneNode*               mirror_prop_scenenode;
    };

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

    struct WheelVisualsTicket //!< Wheel visuals are queued for processing using this struct
    {
        WheelVisualsTicket(uint16_t wheel_idx, uint16_t node_idx, Truck::Wheel* def):
            wheel_index(wheel_idx), base_node_index(node_idx),
            wheel_def(def), wheel2_def(nullptr), meshwheel_def(nullptr), flexbodywheel_def(nullptr)
        {}

        WheelVisualsTicket(uint16_t wheel_idx, uint16_t node_idx, Truck::Wheel2* def):
            wheel_index(wheel_idx), base_node_index(node_idx),
            wheel_def(nullptr), wheel2_def(def), meshwheel_def(nullptr), flexbodywheel_def(nullptr)
        {}

        WheelVisualsTicket(uint16_t wheel_idx, uint16_t node_idx, Truck::MeshWheel* def, uint16_t axis1, uint16_t axis2):
            wheel_index(wheel_idx), base_node_index(node_idx),
            wheel_def(nullptr), wheel2_def(nullptr), meshwheel_def(def), flexbodywheel_def(nullptr),
            axis_node_1(axis1), axis_node_2(axis2)
        {}

        WheelVisualsTicket(uint16_t wheel_idx, uint16_t node_idx, Truck::FlexBodyWheel* def, uint16_t axis1, uint16_t axis2):
            wheel_index(wheel_idx), base_node_index(node_idx),
            wheel_def(nullptr), wheel2_def(nullptr), meshwheel_def(nullptr), flexbodywheel_def(def),
            axis_node_1(axis1), axis_node_2(axis2)
        {}

        Truck::Wheel*         wheel_def;
        Truck::Wheel2*        wheel2_def;
        Truck::MeshWheel*     meshwheel_def;
        Truck::FlexBodyWheel* flexbodywheel_def;

        uint16_t               wheel_index;
        uint16_t               base_node_index;
        uint16_t               axis_node_1;
        uint16_t               axis_node_2;
    };

/* -------------------------------------------------------------------------- */
/* Processing functions.                                                      */
/* NOTE: Please maintain alphabetical order.                                  */
/* -------------------------------------------------------------------------- */

    /// Keywords 'section/end_section' and 'implicit module' which covers everything else.
    void EvaluateSectionConfig();
    void ValidateSectionConfig();

    /**
    * Section 'airbrakes'.
    */
    void ProcessAirbrake(Truck::Airbrake & def);

    /**
    * Section 'animators'.
    */
    void ProcessAnimator(Truck::Animator & def);

    /**
    * Section 'AntiLockBrakes'.
    */
    void ProcessAntiLockBrakes(Truck::AntiLockBrakes & def);

    /**
    * Sections 'author' in current module.
    */
    void ProcessAuthors();

    /**
    * Section 'axles'.
    */
    void ProcessAxle(Truck::Axle & def);

    /**
    * Section 'beams'. Depends on 'nodes'
    */
    void ProcessBeam(Truck::Beam & def);

    /**
    * Section 'brakes'.
    */
    void ProcessBrakes(Truck::Brakes & def);

    /**
    * Section 'camerarail', depends on 'nodes'.
    */
    void ProcessCameraRail(Truck::CameraRail & def);

    /**
    * Section 'cameras', depends on 'nodes'.
    */
    void ProcessCamera(Truck::Camera & def);

    /**
    * Section 'cinecam', depends on 'nodes'.
    */
    void ProcessCinecam(Truck::Cinecam & def);

    /**
    * Section 'collisionboxes'
    */
    void ProcessCollisionBox(Truck::CollisionBox & def);

    /**
    * Processes sections 'commands' and 'commands2' (unified).
    */
    void ProcessCommand(Truck::Command2 & def);

    /**
    * Section 'contacters'.
    */
    void ProcessContacter(Truck::Node::Ref & node_ref);

    /**
    * Section 'cruisecontrol'.
    */
    void ProcessCruiseControl(Truck::CruiseControl & def);

    /**
    * Section 'engine'.
    */
    void ProcessEngine(Truck::Engine & def);

    /**
    * Section 'engoption'.
    */ 
    void ProcessEngoption();

    /**
    * Section 'engturbo'.
    */
    void ProcessEngturbo();

    /**
    * Section 'exhausts'.
    */
    void ProcessExhaust(Truck::Exhaust & def);

    /**
    * Inline-section 'extcamera'.
    */
    void ProcessExtCamera(Truck::ExtCamera & def);

    /**
    * Section 'fixes'
    */
    void ProcessFixedNode(Truck::Node::Ref node_ref);

    /**
    * Sections 'flares' and 'flares2'.
    */
    void ProcessFlare2(Truck::Flare2 & def);

    /**
    * Section 'flexbodies'.
    */
    void ProcessFlexbody(Truck::Flexbody& ref);

    /**
    * Section 'flexbodywheels'.
    */
    void ProcessFlexBodyWheel(Truck::FlexBodyWheel & def);

    /**
    * Section 'fusedrag'.
    */
    void ProcessFusedrag(Truck::Fusedrag & def);

    /**
    * Section 'gobals'
    */
    void ProcessGlobals(Truck::Globals & def);

    /**
    * Section 'guisettings'.
    */
    void ProcessGuiSettings(Truck::GuiSettings & def);

    void ProcessHelp();

    /**
    * Depends on 'nodes'
    */
    void ProcessHook(Truck::Hook & def);

    void ProcessHydro(Truck::Hydro & def);

    /**
    * Section 'interaxles'.
    */
    void ProcessInterAxle(Truck::InterAxle & def);

    /**
    * Depends on section 'nodes'
    */
    void ProcessLockgroup(Truck::Lockgroup & lockgroup);

    /**
    * Section 'managedmaterials'
    */
    void ProcessManagedMaterial(Truck::ManagedMaterial & def);

    /**
    * Section 'meshwheels'.
    */
    void ProcessMeshWheel(Truck::MeshWheel & meshwheel_def);

    /**
    * Section 'meshwheels2'.
    */
    void ProcessMeshWheel2(Truck::MeshWheel & def);

    /**
    * Section 'minimass'.
    */
    void ProcessMinimassInAnyModule();

    void ProcessNode(Truck::Node & def);

    /**
    * Section 'particles'.
    */
    void ProcessParticle(Truck::Particle & def);

    /**
    * Section 'pistonprops'.
    */
    void ProcessPistonprop(Truck::Pistonprop & def);

    /**
    * Section 'props'.
    */
    void ProcessProp(Truck::Prop & def);

    /**
    * Section 'railgroups'.
    */
    void ProcessRailGroup(Truck::RailGroup & def);

    /**
    * Section 'ropables'.
    */
    void ProcessRopable(Truck::Ropable & def);

    /**
    * Section 'ropes'.
    */
    void ProcessRope(Truck::Rope & def);

    void ProcessRotator(Truck::Rotator & def);

    void ProcessRotator2(Truck::Rotator2 & def);

    void ProcessScrewprop(Truck::Screwprop & def);

    /**
    * Section 'shocks'.
    */
    void ProcessShock(Truck::Shock & def);

    /**
    * Add a shock absorber (section 'shocks2') to the rig.
    */
    void ProcessShock2(Truck::Shock2 & def);

    /**
    * Add a shock absorber (section 'shocks3') to the rig.
    */
    void ProcessShock3(Truck::Shock3 & def);

    /**
    * Section 'slidenodes'. Depends on 'railgroups'
    */
    void ProcessSlidenode(Truck::SlideNode & def);

    /**
    * Section 'SlopeBrake'.
    */
    void ProcessSlopeBrake(Truck::SlopeBrake & def);

    /**
    * Section 'soundsources'.
    */
    void ProcessSoundSource(Truck::SoundSource & def); 

    /**
    * Section 'soundsources2'.
    */
    void ProcessSoundSource2(Truck::SoundSource2 & def); 

    /**
    * Section 'submeshes'.
    */
    void ProcessSubmesh(Truck::Submesh & def);

    /**
    * Section 'ties'.
    */
    void ProcessTie(Truck::Tie & def);

    /**
    * Section 'torquecurve'. Depends on 'engine'.
    */
    void ProcessTorqueCurve(Truck::TorqueCurve & def);

    /**
    * Section 'TractionControl'.
    */
    void ProcessTractionControl(Truck::TractionControl & def);

    /**
    * Section 'transfercase'.
    */
    void ProcessTransferCase(Truck::TransferCase & def);

    void ProcessTrigger(Truck::Trigger & def);

    void ProcessTurbojet(Truck::Turbojet & def);

    /**
    * Sections 'turboprops' and 'turboprops2'
    */
    void ProcessTurboprop2(Truck::Turboprop2 & def);

    /**
    * Section 'wheeldetachers'.
    */
    void ProcessWheelDetacher(Truck::WheelDetacher & def);

    /**
    * Section 'wheels'.
    */
    void ProcessWheel(Truck::Wheel & def);

    /**
    * Section 'wheels2'.
    * @author Pierre-Michel Ricordel
    * @author Thomas Fischer
    */
    void ProcessWheel2(Truck::Wheel2 & def);

    /**
    * Section 'wings'.
    * @author 
    */
    void ProcessWing(Truck::Wing & def);

/* -------------------------------------------------------------------------- */
/* Partial processing functions.                                              */
/* -------------------------------------------------------------------------- */

    void BuildAeroEngine(
        NodeIdx_t ref_node_index,
        NodeIdx_t back_node_index,
        NodeIdx_t blade_1_node_index,
        NodeIdx_t blade_2_node_index,
        NodeIdx_t blade_3_node_index,
        NodeIdx_t blade_4_node_index,
        NodeIdx_t couplenode_index,
        bool is_turboprops,
        Ogre::String const & airfoil,
        float power,
        float pitch
    );

    /**
    * Fetches free beam and sets up defaults.
    */
    beam_t & AddBeam(node_t & node_1, node_t & node_2);
    beam_t & AddBeam(Truck::Node::Ref n1, Truck::Node::Ref n2);

    /**
    * Adds complete wheel (section 'wheels') to the rig.
    * @return wheel index in rig_t::wheels array.
    */
    unsigned int AddWheel(Truck::Wheel & wheel);

    /**
    * Adds wheel from section 'wheels2'.
    * @return wheel index.
    */
    unsigned int AddWheel2(Truck::Wheel2 & wheel_2_def);

    void CreateBeamVisuals(beam_t const& beam, int beam_index, bool visible, std::string material_override="");

    RailGroup *CreateRail(std::vector<Truck::Node::Range> & node_ranges);

    static void AddSoundSource(Actor *vehicle, SoundScriptInstance *sound_script, NodeIdx_t node_index, int type = -2);

    static void AddSoundSourceInstance(Actor *vehicle, Ogre::String const & sound_script_name, int node_index, int type = -2);

/* -------------------------------------------------------------------------- */
/* Limits.                                                                    */
/* -------------------------------------------------------------------------- */

    /**
    * Checks there is still space left in rig_t::ar_custom_particles array.
    * @param count Required number of free slots.
    * @return True if there is space left.
    */
    bool CheckParticleLimit(unsigned int count);
    
    /**
    * Checks there is still space left in rig_t::axles array.
    * @param count Required number of free slots.
    * @return True if there is space left.
    */
    bool CheckAxleLimit(unsigned int count);

    /**
    * Checks there is still space left in rig_t::subtexcoords, rig_t::subcabs and rig_t::subisback arrays.
    * @param count Required number of free slots.
    * @return True if there is space left.
    */
    bool CheckSubmeshLimit(unsigned int count);

    /**
    * Checks there is still space left in rig_t::texcoords array.
    * @param count Required number of free slots.
    * @return True if there is space left.
    */
    bool CheckTexcoordLimit(unsigned int count);

    /**
    * Checks there is still space left in rig_t::cabs array.
    * @param count Required number of free slots.
    * @return True if there is space left.
    */
    bool CheckCabLimit(unsigned int count);

    /**
    * Checks there is still space left in rig_t::ar_camera_rail array.
    * @param count Required number of free slots.
    * @return True if there is space left.
    */
    bool CheckCameraRailLimit(unsigned int count);

    /**
    * Checks there is still space left in rig_t::soundsources array (static version).
    * @param count Required number of free slots.
    * @return True if there is space left.
    */
    static bool CheckSoundScriptLimit(Actor *vehicle, unsigned int count);

    /**
    * Checks there is still space left in rig_t::aeroengines array.
    * @param count Required number of free slots.
    * @return True if there is enough space left.
    */
    bool CheckAeroEngineLimit(unsigned int count);

    /**
    * Checks there is still space left in rig_t::screwprops array.
    * @param count Required number of free slots.
    * @return True if there is enough space left.
    */
    bool CheckScrewpropLimit(unsigned int count);

/* -------------------------------------------------------------------------- */
/* Utility functions.                                                         */
/* -------------------------------------------------------------------------- */

    /**
    * Seeks node.
    * @return Pointer to node, or nullptr if not found.
    */
    node_t* GetBeamNodePointer(Truck::Node::Ref const & node_ref);

    /**
    * Seeks node in both Truck::Document definition and rig_t generated rig.
    * @return Node index or -1 if the node was not found.
    */
    NodeIdx_t FindNodeIndex(Truck::Node::Ref & node_ref, bool silent = false);

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
    * Adds a message to internal log.
    */
    void AddMessage(Message::Type type, Ogre::String const & text);

    void AddExhaust(
        unsigned int emitter_node_idx,
        unsigned int direction_node_idx
    );

    NodeIdx_t ResolveNodeRef(Truck::Node::Ref const & node_ref);

    /**
    * Finds existing node by Node::Ref
    * @return First: Index of existing node; Second: true if node was found.
    */
    std::pair<NodeIdx_t, bool> GetNodeIndex(Truck::Node::Ref const & node_ref, bool quiet = false);

    /**
    * Finds existing node by Node::Ref
    * @return Pointer to node or nullptr if not found.
    */
    node_t* GetNodePointer(Truck::Node::Ref const & node_ref);

    /**
    * Finds existing node by Node::Ref
    * @return Pointer to node
    * @throws Exception If the node isn't found.
    */
    node_t* GetNodePointerOrThrow(Truck::Node::Ref const & node_ref);

    /**
    * Finds existing node by Node::Ref; throws an exception if the node doesn't exist.
    * @return Reference to existing node.
    * @throws Exception If the node isn't found.
    */
    node_t & GetNodeOrThrow(Truck::Node::Ref const & node_ref);

    /**
    * Finds existing pointer by Node::Id
    * @return Ref. to node.
    */
    node_t & GetNode(Truck::Node::Ref & node_ref)
    {
        node_t * node = GetNodePointer(node_ref);
        if (node == nullptr)
        {
            throw Exception(std::string("Failed to retrieve node from reference: ") + node_ref.ToString());
        }
        return * node;
    }

    /**
    * Finds existing node by index.
    * @return Pointer to node or nullptr if not found.
    */
    node_t & GetNode(NodeIdx_t node_index);

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
        Truck::NodeDefaults& node_defaults
    );

    /**
    * Setter.
    */
    void SetCurrentKeyword(Truck::Keyword keyword)
    {
        m_current_keyword = keyword;
    }

    beam_t & GetBeam(unsigned int index);

    /**
    * Parses list of node-ranges into list of individual nodes.
    * @return False if some nodes could not be found and thus the lookup wasn't completed.
    */
    bool CollectNodesFromRanges(
        std::vector<Truck::Node::Range> & node_ranges,
        std::vector<NodeIdx_t> & out_node_indices
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

    beam_t *FindBeamInRig(NodeIdx_t node_a, NodeIdx_t node_b);

    void UpdateCollcabContacterNodes();

    wheel_t::BrakeCombo TranslateBrakingDef(Truck::Wheels::Braking def);

    /**
    * Checks a section only appears in one module and reports a warning if not.
    */
    void CheckSectionSingleModule(
        Ogre::String const & section_name,
        std::list<Truck::ModulePtr> & found_items	
    );

    /**
    * Creates beam pre-configured for use as rim with section 'wheels2'.
    */
    unsigned int AddWheelRimBeam(Truck::Wheel2 & wheel_2_def, node_t *node_1, node_t *node_2);

    /**
    * Creates beam pre-configured for use as tyre with section 'wheels2'.
    */
    unsigned int AddTyreBeam(Truck::Wheel2 & wheel_2_def, node_t *node_1, node_t *node_2);

    /**
    * Creates beam partially configured for use with section 'wheels2'.
    */
    unsigned int _SectionWheels2AddBeam(Truck::Wheel2 & wheel_2_def, node_t *node_1, node_t *node_2);

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

    void CreateGfxActor();

    /**
    * Factory of GfxActor; invoke after all gfx setup was done.
    */
    void FinalizeGfxSetup();

    /**
    * Validator for the rotator reference structure
    */
    void ValidateRotator(int id, int axis1, int axis2, NodeIdx_t *nodes1, NodeIdx_t *nodes2);

    /**
    * Helper for 'SetupNewEntity()' - see it's doc.
    */
    Ogre::MaterialPtr FindOrCreateCustomizedMaterial(std::string orig_name);

    Ogre::MaterialPtr CreateSimpleMaterial(Ogre::ColourValue color);

    Ogre::ParticleSystem* CreateParticleSystem(std::string const & name, std::string const & template_name);

    Truck::MaterialFlareBinding* FindFlareBindingForMaterial(std::string const & material_name); //!< Returns NULL if none found

    Truck::VideoCamera* FindVideoCameraByMaterial(std::string const & material_name); //!< Returns NULL if none found

    void CreateVideoCamera(Truck::VideoCamera* def);
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
        Truck::Wheels::Propulsion propulsion,
        Truck::Wheels::Braking braking,
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
        Truck::Node::Ref const & rigidity_node_id,
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

    void FetchAxisNodes(
        node_t* & axis_node_1, 
        node_t* & axis_node_2, 
        Truck::Node::Ref const & axis_node_1_id,
        Truck::Node::Ref const & axis_node_2_id
    );

    void _ProcessKeyInertia(
        Truck::Inertia & inertia,
        RoR::CmdKeyInertia& contract_key, 
        RoR::CmdKeyInertia& extend_key
    );

    /** 
    * For specified nodes
    */
    void AdjustNodeBuoyancy(node_t & node, Truck::Node & node_def);

    /** 
    * For generated nodes
    */
    void AdjustNodeBuoyancy(node_t & node);

    /**
    * Ported from SerializedRig::loadTruck() [v0.4.0.7]
    */
    void FinalizeRig();

    /**
    * Ported from SerializedRig::SerializedRig() [v0.4.0.7]
    */
    void InitializeRig();

    void CalcMemoryRequirements(ActorMemoryRequirements& req, Truck::Module* module_def);

    void HandleException();

    // Input
    Truck::DocumentPtr               m_file;
    Ogre::Vector3                    m_spawn_position;
    std::string                      m_selected_config;
    std::vector<Truck::ModulePtr>    m_selected_modules;

    // Context
    Truck::ModulePtr                 m_cur_module;
    Truck::Keyword                   m_current_keyword; //!< For error reports
    std::map<std::string, NodeIdx_t> m_node_names;
    Truck::BeamDefaults              m_cur_beam_defaults;
    Truck::NodeDefaults              m_cur_node_defaults;
    Truck::BeamDefaultsScale         m_cur_beam_defaults_scale;
    Truck::Inertia                   m_cur_inertia_defaults;
    Truck::MinimassPreset            m_cur_minimass_preset;
    Truck::DetacherGroupPreset       m_cur_detacher_preset;
    Truck::LockgroupPreset           m_cur_lockgroup_preset;
    Truck::ManagedMatOptions         m_cur_managed_mat_options;
    Truck::EnableAdvancedDeformation m_cur_enable_advanced_deformation;

    // Spawn
    Actor*             m_actor; //!< The output actor.
    
    bool                           m_apply_simple_materials;
    std::string        m_cab_material_name; //!< Original name defined in truckfile/globals.
    std::string                    m_custom_resource_group;
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
    std::vector<RoR::NodeGfx> m_gfx_nodes;
    CustomMaterial::MirrorPropType         m_curr_mirror_prop_type;
    
    std::map<std::string, CustomMaterial>  m_material_substitutions; //!< Maps original material names (shared) to their actor-specific substitutes; There's 1 substitute per 1 material, regardless of user count.
    std::vector<BeamVisualsTicket>         m_beam_visuals_queue; //!< We want to spawn visuals asynchronously in the future
    std::vector<WheelVisualsTicket>        m_wheel_visuals_queue; //!< We want to spawn visuals asynchronously in the future
    std::map<std::string, Ogre::MaterialPtr>  m_managed_materials;
};

} // namespace RoR
