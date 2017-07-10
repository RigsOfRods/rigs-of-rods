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
      - [Unified beams]: module/beams, module/beam_groups, module/beam_presets
      - [Unified nodes]: module/nodes, module/node_groups, module/node_presets

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
    cinecam                       
    collisionboxes                modules/../collision_boxes
    commands                      [Unified beams]
    commands2                     [Unified beams]
    comment                       
    contacters                    modules/../contacters
    cruisecontrol                 modules/../cruise_control
    description                   general/description
    disabledefaultsounds          general/disable_default_sounds
    enable_advanced_deformation   general/enable_advanced_deform
    engine                        
    engoption                     
    engturbo                      
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

    void AddModule                   (std::string const & module_name);
    void AddBeamPreset               (RigDef::BeamDefaults* beam_preset);
    void AddNodePreset               (RigDef::NodeDefaults* node_preset);
    void AppendAnimatorOptionJson    (rapidjson::Value& j_array, const char* option, int param=-1);
    void AddRigPropertiesJson        (rapidjson::Value val);
    void ExportNodesToJson           (std::map<std::string, Node>& nodes, std::vector<NodeGroup>& groups);
    void ExportBeamsToJson           (std::list<Beam>& beams, std::vector<BeamGroup>& groups);
    void ExportAirbrakesToJson       (std::vector<RigDef::Airbrake>&airbrakes);
    void ExportAnimatorsToJson       (std::vector<RigDef::Animator>&animators);
    void ExportAntiLockBrakesToJson  (std::shared_ptr<RigDef::AntiLockBrakes>&anti_lock_brakes);
    void ExportAxlesToJson           (std::vector<RigDef::Axle>&axles);
    void ExportBrakesToJson          (std::shared_ptr<RigDef::Brakes>&brakes);
    void ExportCamerasToJson         (std::vector<RigDef::Camera>&cameras);
    void ExportCameraRailsToJson     (std::vector<RigDef::CameraRail>&camera_rails);
    void ExportCollisionBoxesToJson  (std::vector<RigDef::CollisionBox>&collision_boxes);
    void ExportCruiseControlToJson   (std::shared_ptr<RigDef::CruiseControl>&cruise_control);
    void ExportContactersToJson      (std::vector<RigDef::Node::Ref>&contacters);
    void ExportEngturbosToJson       (std::shared_ptr<RigDef::Engturbo>&engturbo);
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
    void ExportRailGroupsToJson      (std::vector<RigDef::RailGroup>&railgroups);
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
    void              NodeRefArrayToJson(rapidjson::Value& dst, std::vector<RigDef::Node::Ref>& nodes);
    void              NodeRangeArrayToJson(rapidjson::Value& j_dst_array, std::vector<RigDef::Node::Range>& ranges);
    void              AddMemberBool(rapidjson::Value& j_container, const char* name, bool value);

    rapidjson::Document m_json_doc;
    std::string         m_cur_module_name;
    std::map<RigDef::BeamDefaults*, std::string> m_beam_presets; ///< Temporary while we use 'RigDef::' for editor presets
    std::map<RigDef::NodeDefaults*, std::string> m_node_presets; ///< Temporary while we use 'RigDef::' for editor presets
}; 

} // namespace RigEditor
} // namespace RoR
