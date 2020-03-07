/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2014-2018 Petr Ohlidal & contributors

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
#include "RoRPrerequisites.h"
#include "RigDef_Parser.h"
#include "BeamData.h"
#include "FlexFactory.h"
#include "FlexObj.h"
#include "GfxActor.h"

#include <OgreString.h>
#include <string>

/// Processes a RigDef::File data structure (result of parsing a "Truckfile" fileformat) into 'an Actor' - a simulated physical object.
///
/// HISTORY:
///
/// Before v0.4.5, truckfiles were parsed&spawned on-the-fly: RoR's simulation used data structures with arrays of pre-defined sizes
/// (i.e. MAX_NODES, MAX_BEAMS, MAX_* ...) and the spawner (class `SerializedRig`) wrote directly into them while reading data from the truckfile. Gfx elements were also created immediately.
/// As a result, the logic was chaotic: some features broke each other (most notably VideoCameras X MaterialFlares X SkinZips) and the sim. structs often contained parser context variables.
/// Also, the whole system was extremely sensitive to order of definitions in truckfile - often [badly/not] documented, known only by forum/IRC users at the time.
///
/// Since v0.4.5, RoR has `RigDef::Parser` which reads truckfile and emits instance of `RigDef::File` - all data from truckfile in memory. `RigDef::File` doesn't preserve the order of definitions,
/// instead it's designed to resolve all order-dependent references to order-independent, see `RigDef::SequentialImporter` (resources/rig_def_fileformat/RigDef_SequentialImporter.h) for more info.
/// `ActorSpawner` was created by carefully refactoring old `SerializedRig` described above, so a lot of the dirty logic remained. Elements were still written into constant-size arrays.
///
/// PRESENT (06/2017):
///
/// RoR is being refactored to get rid of the MAX_[BEAMS/NODES/***] limits. Static arrays in `rig_t` are replaced with pointers to dynamically allocated memory.
/// Memory requirements are calculated upfront from `RigDef::File`.
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
        std::shared_ptr<RigDef::File> file,
        Ogre::SceneNode *parent,
        Ogre::Vector3 const & spawn_position
        );

    Actor *SpawnActor();

    /**
    * Adds a vehicle module to the validated configuration.
    * @param module_name A module from the validated rig-def file.
    */
    bool AddModule(Ogre::String const & module_name);

    /**
    * Adds a vehicle module to the validated configuration.
    * @param module_name A module from the validated rig-def file.
    */
    void AddModule(std::shared_ptr<RigDef::File::Module> module)
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
    unsigned int GetNodeIndexOrThrow(RigDef::Node::Ref const & id);

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
            wheel_def(nullptr), wheel2_def(nullptr), meshwheel_def(def), flexbodywheel_def(nullptr),
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

    /**
    * Section 'airbrakes'.
    */
    void ProcessAirbrake(RigDef::Airbrake & def);

    /**
    * Section 'animators'.
    */
    void ProcessAnimator(RigDef::Animator & def);

    /**
    * Section 'AntiLockBrakes'.
    */
    void ProcessAntiLockBrakes(RigDef::AntiLockBrakes & def);

    /**
    * Section 'author' in root module.
    */
    void ProcessAuthors();

    /**
    * Section 'axles'.
    */
    void ProcessAxle(RigDef::Axle & def);

    /**
    * Section 'beams'. Depends on 'nodes'
    */
    void ProcessBeam(RigDef::Beam & def);

    /**
    * Section 'brakes' in any module.
    */
    void ProcessBrakes(RigDef::Brakes & def);

    /**
    * Section 'camerarail', depends on 'nodes'.
    */
    void ProcessCameraRail(RigDef::CameraRail & def);

    /**
    * Section 'cameras', depends on 'nodes'.
    */
    void ProcessCamera(RigDef::Camera & def);

    /**
    * Section 'cinecam', depends on 'nodes'.
    */
    void ProcessCinecam(RigDef::Cinecam & def);

    /**
    * Section 'collisionboxes'
    */
    void ProcessCollisionBox(RigDef::CollisionBox & def);

    /**
    * Processes sections 'commands' and 'commands2' (unified).
    */
    void ProcessCommand(RigDef::Command2 & def);

    /**
    * Section 'contacters'.
    */
    void ProcessContacter(RigDef::Node::Ref & node_ref);

    /**
    * Section 'cruisecontrol' in any module.
    */
    void ProcessCruiseControl(RigDef::CruiseControl & def);

    /**
    * Section 'engine' in any module.
    */
    void ProcessEngine(RigDef::Engine & def);

    /**
    * Section 'engoption' in any module.
    */ 
    void ProcessEngoption(RigDef::Engoption & def);

    /**
    * Section 'engturbo' in any module.
    */
    void ProcessEngturbo(RigDef::Engturbo & def);

    /**
    * Section 'exhausts'.
    */
    void ProcessExhaust(RigDef::Exhaust & def);

    /**
    * Inline-section 'extcamera'.
    */
    void ProcessExtCamera(RigDef::ExtCamera & def);

    /**
    * Section 'fixes'
    */
    void ProcessFixedNode(RigDef::Node::Ref node_ref);

    /**
    * Sections 'flares' and 'flares2'.
    */
    void ProcessFlare2(RigDef::Flare2 & def);

    /**
    * Section 'flexbodies'.
    */
    void ProcessFlexbody(std::shared_ptr<RigDef::Flexbody> def);

    /**
    * Section 'flexbodywheels'.
    */
    void ProcessFlexBodyWheel(RigDef::FlexBodyWheel & def);

    /**
    * Section 'fusedrag'.
    */
    void ProcessFusedrag(RigDef::Fusedrag & def);

    /**
    * Section 'gobals' in any module
    */
    void ProcessGlobals(RigDef::Globals & def);

    /**
    * Section 'guisettings'.
    */
    void ProcessGuiSettings(RigDef::GuiSettings & def);

    void ProcessHelp();

    /**
    * Depends on 'nodes'
    */
    void ProcessHook(RigDef::Hook & def);

    void ProcessHydro(RigDef::Hydro & def);

    /**
    * Section 'interaxles'.
    */
    void ProcessInterAxle(RigDef::InterAxle & def);

    /**
    * Depends on section 'nodes'
    */
    void ProcessLockgroup(RigDef::Lockgroup & lockgroup);

    /**
    * Section 'managedmaterials'
    */
    void ProcessManagedMaterial(RigDef::ManagedMaterial & def);

    /**
    * Section 'meshwheels'.
    */
    void ProcessMeshWheel(RigDef::MeshWheel & meshwheel_def);

    /**
    * Section 'meshwheels2'.
    */
    void ProcessMeshWheel2(RigDef::MeshWheel & def);

    /**
    * Section 'minimass'.
    */
    void ProcessMinimassInAnyModule();

    void ProcessNode(RigDef::Node & def);

    /**
    * Section 'particles'.
    */
    void ProcessParticle(RigDef::Particle & def);

    /**
    * Section 'pistonprops'.
    */
    void ProcessPistonprop(RigDef::Pistonprop & def);

    /**
    * Section 'props'.
    */
    void ProcessProp(RigDef::Prop & def);

    /**
    * Section 'railgroups'.
    */
    void ProcessRailGroup(RigDef::RailGroup & def);

    /**
    * Section 'ropables'.
    */
    void ProcessRopable(RigDef::Ropable & def);

    /**
    * Section 'ropes'.
    */
    void ProcessRope(RigDef::Rope & def);

    void ProcessRotator(RigDef::Rotator & def);

    void ProcessRotator2(RigDef::Rotator2 & def);

    void ProcessScrewprop(RigDef::Screwprop & def);

    void ProcessScript(RigDef::Script & def);

    /**
    * Section 'shocks'.
    */
    void ProcessShock(RigDef::Shock & def);

    /**
    * Add a shock absorber (section 'shocks2') to the rig.
    */
    void ProcessShock2(RigDef::Shock2 & def);

    /**
    * Add a shock absorber (section 'shocks3') to the rig.
    */
    void ProcessShock3(RigDef::Shock3 & def);

    /**
    * Section 'slidenodes'. Depends on 'railgroups'
    */
    void ProcessSlidenode(RigDef::SlideNode & def);

    /**
    * Section 'SlopeBrake' in any module.
    */
    void ProcessSlopeBrake(RigDef::SlopeBrake & def);

    /**
    * Section 'soundsources'.
    */
    void ProcessSoundSource(RigDef::SoundSource & def); 

    /**
    * Section 'soundsources2'.
    */
    void ProcessSoundSource2(RigDef::SoundSource2 & def); 

    /**
    * Section 'submeshes'.
    */
    void ProcessSubmesh(RigDef::Submesh & def);

    /**
    * Section 'ties'.
    */
    void ProcessTie(RigDef::Tie & def);

    /**
    * Section 'torquecurve' in any module. Depends on 'engine'.
    */
    void ProcessTorqueCurve(RigDef::TorqueCurve & def);

    /**
    * Section 'TractionControl' in any module.
    */
    void ProcessTractionControl(RigDef::TractionControl & def);

    /**
    * Section 'transfercase'.
    */
    void ProcessTransferCase(RigDef::TransferCase & def);

    void ProcessTrigger(RigDef::Trigger & def);

    void ProcessTurbojet(RigDef::Turbojet & def);

    /**
    * Sections 'turboprops' and 'turboprops2'
    */
    void ProcessTurboprop2(RigDef::Turboprop2 & def);

    /**
    * Section 'wheeldetachers' in all modules.
    */
    void ProcessWheelDetacher(RigDef::WheelDetacher & def);

    /**
    * Section 'wheels' in all modules.
    */
    void ProcessWheel(RigDef::Wheel & def);

    /**
    * Section 'wheels2' in all modules.
    * @author Pierre-Michel Ricordel
    * @author Thomas Fischer
    */
    void ProcessWheel2(RigDef::Wheel2 & def);

    /**
    * Section 'wings'.
    * @author 
    */
    void ProcessWing(RigDef::Wing & def);

/* -------------------------------------------------------------------------- */
/* Partial processing functions.                                              */
/* -------------------------------------------------------------------------- */

    void BuildAerialEngine(
        int ref_node_index,
        int back_node_index,
        int blade_1_node_index,
        int blade_2_node_index,
        int blade_3_node_index,
        int blade_4_node_index,
        int couplenode_index,
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

    static void AddSoundSource(Actor *vehicle, SoundScriptInstance *sound_script, int node_index, int type = -2);

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
    node_t* GetBeamNodePointer(RigDef::Node::Ref const & node_ref);

    /**
    * Seeks node in both RigDef::File definition and rig_t generated rig.
    * @return Node index or -1 if the node was not found.
    */
    int FindNodeIndex(RigDef::Node::Ref & node_ref, bool silent = false);

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
    * Finds existing node by Node::Ref
    * @return First: Index of existing node; Second: true if node was found.
    */
    std::pair<unsigned int, bool> GetNodeIndex(RigDef::Node::Ref const & node_ref, bool quiet = false);

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
    * Finds existing node by Node::Ref; throws an exception if the node doesn't exist.
    * @return Reference to existing node.
    * @throws Exception If the node isn't found.
    */
    node_t & GetNodeOrThrow(RigDef::Node::Ref const & node_ref);

    /**
    * Finds existing pointer by Node::Id
    * @return Ref. to node.
    */
    node_t & GetNode(RigDef::Node::Ref & node_ref)
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
    node_t & GetNode(unsigned int node_index);

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
    void SetCurrentKeyword(RigDef::File::Keyword keyword)
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
        std::vector<unsigned int> & out_node_indices
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

    beam_t *FindBeamInRig(unsigned int node_a, unsigned int node_b);

    void SetBeamDeformationThreshold(beam_t & beam, std::shared_ptr<RigDef::BeamDefaults> beam_defaults);

    void UpdateCollcabContacterNodes();

    wheel_t::BrakeCombo TranslateBrakingDef(RigDef::Wheels::Braking def);

    /**
    * Checks a section only appears in one module and reports a warning if not.
    */
    void CheckSectionSingleModule(
        Ogre::String const & section_name,
        std::list<std::shared_ptr<RigDef::File::Module>> & found_items	
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

    void CreateGfxActor();

    /**
    * Factory of GfxActor; invoke after all gfx setup was done.
    */
    void FinalizeGfxSetup();

    /**
    * Validator for the rotator reference structure
    */
    void ValidateRotator(int id, int axis1, int axis2, int *nodes1, int *nodes2);

    /**
    * Helper for 'SetupNewEntity()' - see it's doc.
    */
    Ogre::MaterialPtr FindOrCreateCustomizedMaterial(std::string orig_name);

    Ogre::MaterialPtr CreateSimpleMaterial(Ogre::ColourValue color);

    Ogre::ParticleSystem* CreateParticleSystem(std::string const & name, std::string const & template_name);

    RigDef::MaterialFlareBinding* FindFlareBindingForMaterial(std::string const & material_name); ///< Returns NULL if none found

    RigDef::VideoCamera* FindVideoCameraByMaterial(std::string const & material_name); ///< Returns NULL if none found

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
        RigDef::Wheels::Propulsion propulsion,
        RigDef::Wheels::Braking braking,
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
        int type = BEAM_NORMAL /* Anonymous enum in BeamData.h */
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
        RigDef::Node::Ref const & axis_node_1_id,
        RigDef::Node::Ref const & axis_node_2_id
    );

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

    void CalcMemoryRequirements(ActorMemoryRequirements& req, RigDef::File::Module* module_def);

    void HandleException();

    // Spawn
    Actor*             m_actor; //!< The output actor.
    Ogre::Vector3      m_spawn_position;
    bool               m_apply_simple_materials;
    std::string        m_cab_material_name; ///< Original name defined in truckfile/globals.
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
    RigDef::File::Keyword     m_current_keyword; //!< For error reports
    std::vector<RoR::NodeGfx> m_gfx_nodes;
    CustomMaterial::MirrorPropType         m_curr_mirror_prop_type;
    std::shared_ptr<RigDef::File>          m_file; //!< The parsed input file.
    std::map<Ogre::String, unsigned int>   m_named_nodes;
    std::map<std::string, CustomMaterial>  m_material_substitutions; //!< Maps original material names (shared) to their actor-specific substitutes; There's 1 substitute per 1 material, regardless of user count.
    std::vector<BeamVisualsTicket>         m_beam_visuals_queue; //!< We want to spawn visuals asynchronously in the future
    std::vector<WheelVisualsTicket>        m_wheel_visuals_queue; //!< We want to spawn visuals asynchronously in the future
    std::map<std::string, Ogre::MaterialPtr>  m_managed_materials;
    std::list<std::shared_ptr<RigDef::File::Module>>  m_selected_modules;

};
