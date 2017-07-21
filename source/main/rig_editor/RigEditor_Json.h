/*
    This source file is part of Rigs of Rods
    Copyright 2013-2016 Petr Ohlidal & contributors

    For more information, see http://www.rigsofrods.com/

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
/// @date   05/2017
/// @author Petr Ohlidal
/// @brief  Saving/loading of JSON-based RigEditor projects

/*
    TRUCKFILE SUPPORT CHECKLIST:
    ----------------------------

    Legend:
      - [Unified beams]: modules/../beams, modules/../beam_groups, modules/../beam_presets
      - [Unified nodes]: modules/../nodes, modules/../node_groups, modules/../node_presets

    TRUCKFILE KEYWORD           | JSON handling

    airbrakes                     modules/../airbrakes
    animators                     modules/../animators
    AntiLockBrakes                modules/../anti_lock_brakes
    author                        general/authors [array]
    axles                         modules/../axles
    beams                         [Unified beams]
    brakes                        modules/../brake_force, modules/../parking_brake_force
    camerarail                    modules/../camera_rails
    cameras                       modules/../cameras
    cinecam                       modules/../cinecam
    collisionboxes                modules/../collision_boxes
    commands                      [Unified beams]
    commands2                     [Unified beams]
    comment                       
    contacters                    modules/../contacters
    cruisecontrol                 modules/../cruise_control
    description                   general/description
    disabledefaultsounds          general/disable_default_sounds
    enable_advanced_deformation   general/enable_advanced_deform
    engine                        modules/../engine
    engoption                     modules/../engoption
    engturbo                      modules/../engturbo
    exhausts                      modules/../exhausts
    extcamera                     general/extcam_mode, general/extcam_node_id
    forwardcommands               general/forward_commands
    fileformatversion             
    fileinfo                      general/fileinfo_uid, general/fileinfo_category_id, general/fileinfo_version
    fixes                         modules/../fixes
    flares                        
    flares2                       
    flexbodies                    
    flexbodywheels                
    fusedrag                      modules/../fusedrag
    globals                       general/globals_load_mass, general/globals_dry_mass, general/globals_cab_material_name
    guid                          general/guid
    guisettings                   
    help                          
    hideInChooser                 general/hide_in_chooser
    hookgroup                     
    hooks                         modules/../hooks
    hydros                        [Unified beams]
    importcommands                general/import_commands
    lockgroups                    modules/../lockgroups
    lockgroup_default_nolock      general/lockgroup_default_nolock
    managedmaterials              modules/../managed_materials
    materialflarebindings         modules/../mat_flare_bindings
    meshwheels                    
    meshwheels2                   
    minimass                      general/minimum_mass
    nodecollision                 modules/../node_collisions
    nodes                         [Unified nodes]
    nodes2                        [Unified nodes]
    particles                     modules/../particles
    pistonprops                   modules/../pistonprops
    props                         modules/../props -- TODO: animations
    railgroups                    
    rescuer                       general/is_rescuer
    rigidifiers                   
    rollon                        general/is_rollon
    ropables                      
    ropes                         [Unified beams]
    rotators                      modules/../rotators
    rotators2                     modules/../rotators_2
    screwprops                    modules/../screwprops
    sectionconfig                 
    section                       
    set_collision_range           general/collision_range
    set_shadows                   
    set_skeleton_settings         general/visibility_range_meters, general/beam_thickness_meters
    shocks                        [Unified beams]
    shocks2                       [Unified beams]
    slidenode_connect_instantly   general/slidenodes_connect_instant
    slidenodes                    modules/../slidenodes
    SlopeBrake                    modules/../slope_brake
    soundsources                  modules/../sound_sources
    soundsources2                 modules/../sound_sources_2
    speedlimiter                  modules/../speed_limiter
    submesh                       
    submesh_groundmodel           modules/../submesh_groundmodel_name
    ties                          
    <title>                       general/title
    torquecurve                   modules/../torque_curve
    TractionControl               modules/../traction_control
    triggers                      
    turbojets                     modules/../turbojets
    turboprops                    modules/../turboprops_2 (unified turboprops)
    turboprops2                   modules/../turboprops_2 (unified turboprops)
    videocamera                   
    wheels                        
    wheels2                       
    wings                         
*/

#pragma once

#include "RigEditor_Beam.h"
#include "RigEditor_Node.h"
#include "RigEditor_ForwardDeclarations.h"

#include <MyGUI_UString.h>
#include <OgreColourValue.h>
#include <OgreVector3.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <string>
#include <vector>

namespace RoR {
namespace RigEditor {

class JsonExporter
{
public:
    JsonExporter();

    void SetModule                   (std::string const & module_name);
    void AddBeamPreset               (RigDef::BeamDefaults* beam_preset);
    void AddNodePreset               (RigDef::NodeDefaults* node_preset);
    void AddWheel                    (LandVehicleWheel* wheel);
    void AppendAnimatorOptionJson    (rapidjson::Value& j_array, const char* option, int param=-1);
    void AddRigPropertiesJson        (rapidjson::Value& val);
    void ExportNodesToJson           (std::map<std::string, Node>& nodes, std::vector<NodeGroup>& groups);
    void ExportBeamsToJson           (std::list<Beam>& beams, std::vector<BeamGroup>& groups);
    void ExportAirbrakesToJson       (std::vector<RigDef::Airbrake>&airbrakes);
    void ExportAnimatorsToJson       (std::vector<RigDef::Animator>&animators);
    void ExportAntiLockBrakesToJson  (std::shared_ptr<RigDef::AntiLockBrakes>&anti_lock_brakes);
    void ExportAxlesToJson           (std::vector<RigDef::Axle>&axles);
    void ExportBrakesToJson          (std::shared_ptr<RigDef::Brakes>&brakes);
    void ExportCamerasToJson         (std::vector<RigDef::Camera>&cameras);
    void ExportCameraRailsToJson     (std::vector<RigDef::CameraRail>&camera_rails);
    void ExportCinecamToJson         (RigEditor::CineCamera& editor_cam);
    void ExportCollisionBoxesToJson  (std::vector<RigDef::CollisionBox>&collision_boxes);
    void ExportCruiseControlToJson   (std::shared_ptr<RigDef::CruiseControl>&cruise_control);
    void ExportContactersToJson      (std::vector<RigDef::Node::Ref>&contacters);
    void ExportEngineToJson          (std::shared_ptr<RigDef::Engine>&    def);
    void ExportEngoptionToJson       (std::shared_ptr<RigDef::Engoption>& def);
    void ExportEngturboToJson        (std::shared_ptr<RigDef::Engturbo>&engturbo);
    void ExportExhaustsToJson        (std::vector<RigDef::Exhaust>&exhausts);
    void ExportFixesToJson           (std::vector<RigDef::Node::Ref>&fixes);
    void ExportFusedragsToJson       (std::vector<RigDef::Fusedrag>&fusedrag);
    void ExportHooksToJson           (std::vector<RigDef::Hook>&hooks);
    void ExportLockgroupsToJson      (std::vector<RigDef::Lockgroup>&lockgroups);
    void ExportManagedMatsToJson     (std::vector<RigDef::ManagedMaterial>&managed_mats);
    void ExportMatFlareBindingsToJson(std::vector<RigDef::MaterialFlareBinding>& mat_flare_bindings);
    void ExportNodeCollisionsToJson  (std::vector<RigDef::NodeCollision>&node_collisions);
    void ExportParticlesToJson       (std::vector<RigDef::Particle>&particles);
    void ExportPistonpropsToJson     (std::vector<RigDef::Pistonprop>&pistonprops);
    void ExportPropsToJson           (std::vector<RigDef::Prop>&props);
    void ExportRailGroupsToJson      (std::list<RigEditor::NodeGroup>& editor_railgroups);
    void ExportRopablesToJson        (std::vector<RigDef::Ropable>&ropables);
    void ExportRotatorsToJson        (std::vector<RigDef::Rotator>&rotators);
    void ExportRotators2ToJson       (std::vector<RigDef::Rotator2>&rotators_2);
    void ExportScrewpropsToJson      (std::vector<RigDef::Screwprop>&screwprops);
    void ExportSlideNodesToJson      (std::vector<RigDef::SlideNode>&slidenodes);
    void ExportSlopeBrakeToJson      (std::shared_ptr<RigDef::SlopeBrake>&slope_brake);
    void ExportSoundSourcesToJson    (std::vector<RigDef::SoundSource>&soundsources);
    void ExportSoundSources2ToJson   (std::vector<RigDef::SoundSource2>&soundsources_2);
    void ExportSpeedLimiterToJson    (RigDef::SpeedLimiter speed_limiter);
    void ExportSubmeshesToJson       (std::vector<RigDef::Submesh>&submeshes);
    void ExportTiesToJson            (std::vector<RigDef::Tie>&ties);
    void ExportTorqueCurveToJson     (std::shared_ptr<RigDef::TorqueCurve>&torque_curve);
    void ExportTractionControlToJson (std::shared_ptr<RigDef::TractionControl>&traction_control);
    void ExportTurbojetsToJson       (std::vector<RigDef::Turbojet>&turbojets);
    void ExportTurboprops2ToJson     (std::vector<RigDef::Turboprop2>&turboprops_2);
    void ExportVideoCamerasToJson    (std::vector<RigDef::VideoCamera>&videocameras);
    void ExportWingsToJson           (std::vector<RigDef::Wing>&wings);
    void ExportSubmeshGroundmodelToJson(std::string const & submesh_groundmodel_name);
    void SavePresetsToJson           ();

    void SaveRigProjectJsonFile(MyGUI::UString const & out_path);

    inline rapidjson::Document& GetDocument() { return m_json_doc; }

    // Conversion helpers
           rapidjson::Value StrToJson(std::string const & s);
    inline rapidjson::Value NodeToJson(RigDef::Node::Ref const & node_ref) { return this->StrToJson(node_ref.Str()); }
    inline rapidjson::Value NodeToJson(RigEditor::Node & node)             { return this->StrToJson(node.GetId().Str()); }
    inline rapidjson::Value NodeToJson(RigEditor::Node * node)             { return this->StrToJson(node->GetId().Str()); }

private:
    rapidjson::Value& GetModuleJson();
    rapidjson::Value& GetOrCreateMember(rapidjson::Value& container, const char* name, rapidjson::Type j_type = rapidjson::kObjectType);
    rapidjson::Value  RgbaToJson(Ogre::ColourValue color);
    rapidjson::Value  Vector3ToJson(Ogre::Vector3 pos);
    rapidjson::Value  BeamPresetToJson(std::shared_ptr<RigDef::BeamDefaults>& preset);
    rapidjson::Value  NodePresetToJson(RigDef::NodeDefaults* preset);
    inline rapidjson::Value  NodePresetToJson(std::shared_ptr<RigDef::NodeDefaults>& preset_sptr)                { return this->NodePresetToJson(preset_sptr.get()); }
    void              NodeRefArrayToJson(rapidjson::Value& dst, std::vector<RigDef::Node::Ref>& nodes);
    void              NodeRangeArrayToJson(rapidjson::Value& j_dst_array, std::vector<RigDef::Node::Range>& ranges);
    void              AddMemberBool(rapidjson::Value& j_container, const char* name, bool value);
    void              EditorNodePtrArrayToJson(rapidjson::Value& j_array, std::vector<Node*>& nodes);

    rapidjson::Document m_json_doc;
    std::string         m_cur_module_name;
    // We need to assign unique numbers to presets; let's create std::map<> of {instance ->ID} mappings.
    // TODO: implement properly in Editor; we should not rely on RigDef::NodeDefaults
    std::map<RigDef::BeamDefaults*, std::string> m_beam_presets; ///< Temporary while we use 'RigDef::' for editor presets
    std::map<RigDef::NodeDefaults*, std::string> m_node_presets; ///< Temporary while we use 'RigDef::' for editor presets
};

class JsonImporter
{
public:
    JsonImporter();
    void   LoadRigProjectJson            (MyGUI::UString const & src_path);
    void   LoadPresetsFromJson           ();
    void   SetCurrentModule              (const char* module_name);
    void   ImportNodesFromJson           (std::map<std::string, Node>& nodes, std::vector<NodeGroup>& groups);
    void   ImportBeamsFromJson           (std::list<Beam>& beams, std::vector<BeamGroup>& groups);
    void   ImportAirbrakesFromJson       (std::vector<RigDef::Airbrake>&airbrakes);
    void   ImportAnimatorsFromJson       (std::vector<RigDef::Animator>&animators);
    void   ImportAntiLockBrakesFromJson  (std::shared_ptr<RigDef::AntiLockBrakes>&anti_lock_brakes);
    void   ImportAxlesFromJson           (std::vector<RigDef::Axle>&axles);
    void   ImportBrakesFromJson          (std::shared_ptr<RigDef::Brakes>&brakes);
    void   ImportCamerasFromJson         (std::vector<RigDef::Camera>&cameras);
    void   ImportCameraRailsFromJson     (std::vector<RigDef::CameraRail>&camera_rails);
    void   ImportCinecamFromJson         (std::list<RigEditor::CineCamera>& editor_cam);
    void   ImportCollisionBoxesFromJson  (std::vector<RigDef::CollisionBox>&collision_boxes);
    void   ImportCruiseControlFromJson   (std::shared_ptr<RigDef::CruiseControl>&cruise_control);
    void   ImportContactersFromJson      (std::vector<RigDef::Node::Ref>&contacters);
    void   ImportEngineFromJson          (std::shared_ptr<RigDef::Engine>&      def);
    void   ImportEngoptionFromJson       (std::shared_ptr<RigDef::Engoption>&   def);
    void   ImportEngturboFromJson        (std::shared_ptr<RigDef::Engturbo>&    engturbo);
    void   ImportExhaustsFromJson        (std::vector<RigDef::Exhaust>&         exhausts);
    void   ImportFixesFromJson           (std::vector<RigDef::Node::Ref>&       fixes);
    void   ImportFusedragsFromJson       (std::vector<RigDef::Fusedrag>&        fusedrag);
    void   ImportHooksFromJson           (std::vector<RigDef::Hook>&            hooks);
    void   ImportLockgroupsFromJson      (std::vector<RigDef::Lockgroup>&       lockgroups);
    void   ImportManagedMatsFromJson     (std::vector<RigDef::ManagedMaterial>& managed_mats);
    void   ImportMatFlareBindingsFromJson(std::vector<RigDef::MaterialFlareBinding>& mat_flare_bindings);
    void   ImportNodeCollisionsFromJson  (std::vector<RigDef::NodeCollision>&   node_collisions);
    void   ImportParticlesFromJson       (std::vector<RigDef::Particle>&        particles);
    void   ImportPistonpropsFromJson     (std::vector<RigDef::Pistonprop>&      pistonprops);
    void   ImportPropsFromJson           (std::vector<RigDef::Prop>&            props);
    void   ImportRailGroupsFromJson      (std::vector<RigDef::RailGroup>&       railgroups);
    void   ImportRopablesFromJson        (std::vector<RigDef::Ropable>&         ropables);
    void   ImportRotatorsFromJson        (std::vector<RigDef::Rotator>&         rotators);
    void   ImportRotators2FromJson       (std::vector<RigDef::Rotator2>&        rotators_2);
    void   ImportScrewpropsFromJson      (std::vector<RigDef::Screwprop>&       screwprops);
    void   ImportSlideNodesFromJson      (std::vector<RigDef::SlideNode>&       slidenodes);
    void   ImportSlopeBrakeFromJson      (std::shared_ptr<RigDef::SlopeBrake>&  slope_brake);
    void   ImportSoundSourcesFromJson    (std::vector<RigDef::SoundSource>&     soundsources);
    void   ImportSoundSources2FromJson   (std::vector<RigDef::SoundSource2>&    soundsources_2);
    void   ImportSpeedLimiterFromJson    (RigDef::SpeedLimiter& speed_limiter);
    void   ImportSubmeshesFromJson       (std::vector<RigDef::Submesh>&submeshes);
    void   ImportTiesFromJson            (std::vector<RigDef::Tie>&ties);
    void   ImportTorqueCurveFromJson     (std::shared_ptr<RigDef::TorqueCurve>&torque_curve);
    void   ImportTractionControlFromJson (std::shared_ptr<RigDef::TractionControl>&traction_control);
    void   ImportTurbojetsFromJson       (std::vector<RigDef::Turbojet>&turbojets);
    void   ImportTurboprops2FromJson     (std::vector<RigDef::Turboprop2>&turboprops_2);
    void   ImportVideoCamerasFromJson    (std::vector<RigDef::VideoCamera>&videocameras);
    void   ImportWingsFromJson           (std::vector<RigDef::Wing>&wings);
    void   ImportSubmeshGroundmodelFromJson(std::string const & submesh_groundmodel_name);
    rapidjson::Value&  GetRigPropertiesJson();

    static RigDef::Node::Ref    JsonToNodeRef(rapidjson::Value& j_val);

private:
    bool                 FetchArrayFromModule(rapidjson::Value::ValueIterator& start_itor, rapidjson::Value::ValueIterator& end_itor, const char* name);
    bool                 LoadJsonFile(MyGUI::UString const & src_path); ///< @return true on success
    rapidjson::Value&    GetModuleJson();
    Ogre::ColourValue    JsonToRgba(rapidjson::Value& j_val);
    Ogre::Vector3        JsonToVector3(rapidjson::Value& j_val);
    std::shared_ptr<RigDef::NodeDefaults>   ResolveNodePreset(rapidjson::Value& j_preset_id);
    std::shared_ptr<RigDef::BeamDefaults>   ResolveBeamPreset(rapidjson::Value& j_preset_id);
    RigEditor::Node*                        ResolveNode(rapidjson::Value& j_node_id);
    RigDef::Node::Ref                       JsonToNodeRef(rapidjson::Value& j_node_id);
    void                                    JsonToNodeRefArray(std::vector<RigDef::Node::Ref>& vec, rapidjson::Value& j_node_id_array);

    rapidjson::Document m_json_doc;
    Rig*                m_rig;
    std::string         m_module_name;

    // We need to restore element->preset mappings based on assigned names.
    // TODO: implement properly in Editor; we should not rely on RigDef::NodeDefaults
    std::map<std::string, std::shared_ptr<RigDef::BeamDefaults>> m_beam_presets; ///< Temporary while we use 'RigDef::' for editor presets
    std::map<std::string, std::shared_ptr<RigDef::NodeDefaults>> m_node_presets; ///< Temporary while we use 'RigDef::' for editor presets
};

} // namespace RigEditor
} // namespace RoR
