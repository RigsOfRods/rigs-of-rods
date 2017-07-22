/*
    This source file is part of Rigs of Rods
    Copyright 2013-2017 Petr Ohlidal & contributors

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

/** 
    @file   RigEditor_RigProperties.cpp
    @date   09/2014
    @author Petr Ohlidal
*/

#include "RigDef_File.h"
#include "RigEditor_Json.h"
#include "RigEditor_RigProperties.h"

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>

namespace RoR {
namespace RigEditor {

struct RigModuleData
{
    std::string                                module_name;
    std::string                                submesh_groundmodel;

    // LAND VEHICLE window
    std::shared_ptr<RigDef::Engine>            engine;
    std::shared_ptr<RigDef::Engoption>         engoption;

    // STUBS (data only stored and exported, no editing)

    std::vector    <RigDef::Airbrake>          airbrakes;
    std::vector    <RigDef::Animator>          animators;
    std::shared_ptr<RigDef::AntiLockBrakes>    anti_lock_brakes;
    std::vector    <RigDef::Axle>              axles;
    std::shared_ptr<RigDef::Brakes>            brakes;
    std::vector    <RigDef::Camera>            cameras;
    std::vector    <RigDef::CameraRail>        camera_rails;
    std::vector    <RigDef::CollisionBox>      collision_boxes;
    std::shared_ptr<RigDef::CruiseControl>     cruise_control;
    std::vector    <RigDef::Node::Ref>         contacters;
    std::shared_ptr<RigDef::Engturbo>          engturbo;
    std::vector    <RigDef::Exhaust>           exhausts;
    std::vector    <RigDef::Node::Ref>         fixes;
    std::vector    <RigDef::Fusedrag>          fusedrag;
    std::vector    <RigDef::Hook>              hooks;
    std::vector    <RigDef::Lockgroup>         lockgroups;
    std::vector    <RigDef::ManagedMaterial>       managed_mats;
    std::vector    <RigDef::MaterialFlareBinding>  mat_flare_bindings;
    std::vector    <RigDef::NodeCollision>         node_collisions;
    std::vector    <RigDef::Particle>          particles;
    std::vector    <RigDef::Pistonprop>        pistonprops;
    std::vector    <RigDef::Prop>              props;
    std::vector    <RigDef::Ropable>           ropables;
    std::vector    <RigDef::Rotator>           rotators;
    std::vector    <RigDef::Rotator2>          rotators_2;
    std::vector    <RigDef::Screwprop>         screwprops;
    std::vector    <RigDef::SlideNode>         slidenodes;
    std::shared_ptr<RigDef::SlopeBrake>        slope_brake;
    std::vector    <RigDef::SoundSource>       soundsources;
    std::vector    <RigDef::SoundSource2>      soundsources_2;
                    RigDef::SpeedLimiter       speed_limiter;
    std::vector    <RigDef::Submesh>           submeshes;
    std::vector    <RigDef::Tie>               ties;
    std::shared_ptr<RigDef::TorqueCurve>       torque_curve;
    std::shared_ptr<RigDef::TractionControl>   traction_control;
    std::vector    <RigDef::Turbojet>          turbojets;
    std::vector    <RigDef::Turboprop2>        turboprops_2;
    std::vector    <RigDef::VideoCamera>       videocameras;
    std::vector    <RigDef::Wing>              wings;

    std::vector<std::shared_ptr<RigDef::Flexbody>>  flexbodies;

    void BuildFromModule(std::shared_ptr<RigDef::File::Module> m)
    {
        module_name         = m->name;

        // Engine + Engoption
        engine              = m->engine;
        engoption           = m->engoption;
                            
        // Stubs            
        engturbo            = m->engturbo;
        traction_control    = m->traction_control;
        torque_curve        = m->torque_curve;
        speed_limiter       = m->speed_limiter;
        slope_brake         = m->slope_brake;
        submesh_groundmodel = m->submeshes_ground_model_name;
        anti_lock_brakes    = m->anti_lock_brakes;
        brakes              = m->brakes;
        cruise_control      = m->cruise_control;
        
        rotators       .assign( m->rotators      .begin(),   m->rotators       .end() );
        rotators_2     .assign( m->rotators_2    .begin(),   m->rotators_2     .end() );
        wings          .assign( m->wings         .begin(),   m->wings          .end() );
        videocameras   .assign( m->videocameras  .begin(),   m->videocameras   .end() );
        turboprops_2   .assign( m->turboprops_2  .begin(),   m->turboprops_2   .end() );
        pistonprops    .assign( m->pistonprops   .begin(),   m->pistonprops    .end() );
        turbojets      .assign( m->turbojets     .begin(),   m->turbojets      .end() );
        soundsources   .assign( m->soundsources  .begin(),   m->soundsources   .end() );
        soundsources_2 .assign( m->soundsources2 .begin(),   m->soundsources2  .end() );
        slidenodes     .assign( m->slidenodes    .begin(),   m->slidenodes     .end() );
        submeshes      .assign( m->submeshes     .begin(),   m->submeshes      .end() );
        ropables       .assign( m->ropables      .begin(),   m->ropables       .end() );
        particles      .assign( m->particles     .begin(),   m->particles      .end() );
        airbrakes      .assign( m->airbrakes     .begin(),   m->airbrakes      .end() );
        animators      .assign( m->animators     .begin(),   m->animators      .end() );
        axles          .assign( m->axles         .begin(),   m->axles          .end() );
        cameras        .assign( m->cameras       .begin(),   m->cameras        .end() );
        camera_rails   .assign( m->camera_rails  .begin(),   m->camera_rails   .end() );
        collision_boxes.assign( m->collision_boxes.begin(),  m->collision_boxes.end() );
        contacters     .assign( m->contacters    .begin(),   m->contacters     .end() );
        exhausts       .assign( m->exhausts      .begin(),   m->exhausts       .end() );
        fixes          .assign( m->fixes         .begin(),   m->fixes          .end() );
        flexbodies     .assign( m->flexbodies    .begin(),   m->flexbodies     .end() );
        fusedrag       .assign( m->fusedrag      .begin(),   m->fusedrag       .end() );
        hooks          .assign( m->hooks         .begin(),   m->hooks          .end() );
        lockgroups     .assign( m->lockgroups    .begin(),   m->lockgroups     .end() );
        props          .assign( m->props         .begin(),   m->props          .end() );
        screwprops     .assign( m->screwprops    .begin(),   m->screwprops     .end() );
        ties           .assign( m->ties          .begin(),   m->ties           .end() );

        managed_mats      .assign( m->managed_materials      .begin(),  m->managed_materials      .end() );
        mat_flare_bindings.assign( m->material_flare_bindings.begin(),  m->material_flare_bindings.end() );
        node_collisions   .assign( m->node_collisions        .begin(),  m->node_collisions        .end() );
    }

    void ExportToModule(std::shared_ptr<RigDef::File::Module> m)
    {
        m->name                        = module_name;

        // --- Engine + Engoption ---
        m->engine                      = engine;
        m->engoption                   = engoption;
        // ---------- Stubs -----------
        m->engturbo                    = engturbo;
        m->traction_control            = traction_control;
        m->torque_curve                = torque_curve;
        m->speed_limiter               = speed_limiter;
        m->slope_brake                 = slope_brake;
        m->submeshes_ground_model_name = submesh_groundmodel;
        m->anti_lock_brakes            = anti_lock_brakes;
        m->brakes                      = brakes;
        m->cruise_control              = cruise_control;
        
        m->rotators       .assign( rotators       .begin(),   rotators       .end() );
        m->rotators_2     .assign( rotators_2     .begin(),   rotators_2     .end() );
        m->wings          .assign( wings          .begin(),   wings          .end() );
        m->videocameras   .assign( videocameras   .begin(),   videocameras   .end() );
        m->turboprops_2   .assign( turboprops_2   .begin(),   turboprops_2   .end() );
        m->pistonprops    .assign( pistonprops    .begin(),   pistonprops    .end() );
        m->turbojets      .assign( turbojets      .begin(),   turbojets      .end() );
        m->soundsources   .assign( soundsources   .begin(),   soundsources   .end() );
        m->soundsources2  .assign( soundsources_2 .begin(),   soundsources_2 .end() );
        m->slidenodes     .assign( slidenodes     .begin(),   slidenodes     .end() );
        m->submeshes      .assign( submeshes      .begin(),   submeshes      .end() );
        m->ropables       .assign( ropables       .begin(),   ropables       .end() );
        m->particles      .assign( particles      .begin(),   particles      .end() );
        m->airbrakes      .assign( airbrakes      .begin(),   airbrakes      .end() );
        m->animators      .assign( animators      .begin(),   animators      .end() );
        m->axles          .assign( axles          .begin(),   axles          .end() );
        m->cameras        .assign( cameras        .begin(),   cameras        .end() );
        m->camera_rails   .assign( camera_rails   .begin(),   camera_rails   .end() );
        m->collision_boxes.assign( collision_boxes.begin(),   collision_boxes.end() );
        m->contacters     .assign( contacters     .begin(),   contacters     .end() );
        m->exhausts       .assign( exhausts       .begin(),   exhausts       .end() );
        m->fixes          .assign( fixes          .begin(),   fixes          .end() );
        m->flexbodies     .assign( flexbodies     .begin(),   flexbodies     .end() );
        m->fusedrag       .assign( fusedrag       .begin(),   fusedrag       .end() );
        m->hooks          .assign( hooks          .begin(),   hooks          .end() );
        m->lockgroups     .assign( lockgroups     .begin(),   lockgroups     .end() );
        m->props          .assign( props          .begin(),   props          .end() );
        m->screwprops     .assign( screwprops     .begin(),   screwprops     .end() );
        m->ties           .assign( ties           .begin(),   ties           .end() );

        m->managed_materials      .assign( managed_mats      .begin(),  managed_mats      .end() );
        m->material_flare_bindings.assign( mat_flare_bindings.begin(),  mat_flare_bindings.end() );
        m->node_collisions        .assign( node_collisions   .begin(),  node_collisions   .end() );
    }

    void ExportRigModuleToJson(JsonExporter& exporter)
    {
        exporter.SetModule(this->module_name);

        exporter.ExportSubmeshGroundmodelToJson(this->submesh_groundmodel);
        exporter.ExportEngineToJson          (this->engine);
        exporter.ExportEngoptionToJson       (this->engoption);
        exporter.ExportAirbrakesToJson       (this->airbrakes);
        exporter.ExportAnimatorsToJson       (this->animators);
        exporter.ExportAntiLockBrakesToJson  (this->anti_lock_brakes);
        exporter.ExportAxlesToJson           (this->axles);
        exporter.ExportBrakesToJson          (this->brakes);
        exporter.ExportCamerasToJson         (this->cameras);
        exporter.ExportCameraRailsToJson     (this->camera_rails);
        exporter.ExportCollisionBoxesToJson  (this->collision_boxes);
        exporter.ExportCruiseControlToJson   (this->cruise_control);
        exporter.ExportContactersToJson      (this->contacters);
        exporter.ExportEngineToJson          (this->engine);
        exporter.ExportEngoptionToJson       (this->engoption);
        exporter.ExportEngturboToJson        (this->engturbo);
        exporter.ExportExhaustsToJson        (this->exhausts);
        exporter.ExportFixesToJson           (this->fixes);
        exporter.ExportFusedragsToJson       (this->fusedrag);
        exporter.ExportHooksToJson           (this->hooks);
        exporter.ExportLockgroupsToJson      (this->lockgroups);
        exporter.ExportManagedMatsToJson     (this->managed_mats);
        exporter.ExportMatFlareBindingsToJson(this->mat_flare_bindings);
        exporter.ExportNodeCollisionsToJson  (this->node_collisions);
        exporter.ExportParticlesToJson       (this->particles);
        exporter.ExportPistonpropsToJson     (this->pistonprops);
        exporter.ExportPropsToJson           (this->props);
        exporter.ExportRopablesToJson        (this->ropables);
        exporter.ExportRotatorsToJson        (this->rotators);
        exporter.ExportRotators2ToJson       (this->rotators_2);
        exporter.ExportScrewpropsToJson      (this->screwprops);
        exporter.ExportSlideNodesToJson      (this->slidenodes);
        exporter.ExportSlopeBrakeToJson      (this->slope_brake);
        exporter.ExportSoundSourcesToJson    (this->soundsources);
        exporter.ExportSoundSources2ToJson   (this->soundsources_2);
        exporter.ExportSpeedLimiterToJson    (this->speed_limiter);
        exporter.ExportSubmeshesToJson       (this->submeshes);
        exporter.ExportTiesToJson            (this->ties);
        exporter.ExportTorqueCurveToJson     (this->torque_curve);
        exporter.ExportTractionControlToJson (this->traction_control);
        exporter.ExportTurbojetsToJson       (this->turbojets);
        exporter.ExportTurboprops2ToJson     (this->turboprops_2);
        exporter.ExportVideoCamerasToJson    (this->videocameras);
        exporter.ExportWingsToJson           (this->wings);
    }

    void ImportRigModuleFromJson(JsonImporter& importer)
    {
        //importer.ImportSubmeshGroundmodelFromJson(this->submesh_groundmodel);
        importer.ImportAirbrakesFromJson       (this->airbrakes);
        importer.ImportAnimatorsFromJson       (this->animators);
        importer.ImportAntiLockBrakesFromJson  (this->anti_lock_brakes);
        importer.ImportAxlesFromJson           (this->axles);
        importer.ImportBrakesFromJson          (this->brakes);
        importer.ImportCamerasFromJson         (this->cameras);
        importer.ImportCameraRailsFromJson     (this->camera_rails);
        importer.ImportCollisionBoxesFromJson  (this->collision_boxes);
        importer.ImportCruiseControlFromJson   (this->cruise_control);
        importer.ImportContactersFromJson      (this->contacters);
        importer.ImportEngineFromJson          (this->engine);
        importer.ImportEngoptionFromJson       (this->engoption);
        importer.ImportEngturboFromJson        (this->engturbo);
        importer.ImportExhaustsFromJson        (this->exhausts);
        importer.ImportFixesFromJson           (this->fixes);
        importer.ImportFusedragsFromJson       (this->fusedrag);
        importer.ImportHooksFromJson           (this->hooks);
        importer.ImportLockgroupsFromJson      (this->lockgroups);
        importer.ImportManagedMatsFromJson     (this->managed_mats);
        importer.ImportMatFlareBindingsFromJson(this->mat_flare_bindings);
        importer.ImportNodeCollisionsFromJson  (this->node_collisions);
        importer.ImportParticlesFromJson       (this->particles);
        importer.ImportPistonpropsFromJson     (this->pistonprops);
        importer.ImportPropsFromJson           (this->props);
        //importer.ImportRailGroupsFromJson      (this->railgroups);
        //importer.ImportRopablesFromJson        (this->ropables);
        importer.ImportRotatorsFromJson        (this->rotators);
        importer.ImportRotators2FromJson       (this->rotators_2);
        importer.ImportScrewpropsFromJson      (this->screwprops);
        importer.ImportSlideNodesFromJson      (this->slidenodes);
        importer.ImportSlopeBrakeFromJson      (this->slope_brake);
        importer.ImportSoundSourcesFromJson    (this->soundsources);
        importer.ImportSoundSources2FromJson   (this->soundsources_2);
        importer.ImportSpeedLimiterFromJson    (this->speed_limiter);
        importer.ImportSubmeshesFromJson       (this->submeshes);
        importer.ImportTiesFromJson            (this->ties);
        importer.ImportTorqueCurveFromJson     (this->torque_curve);
        importer.ImportTractionControlFromJson (this->traction_control);
        importer.ImportTurbojetsFromJson       (this->turbojets);
        importer.ImportTurboprops2FromJson     (this->turboprops_2);
        //importer.ImportVideoCamerasFromJson    (this->videocameras);
        importer.ImportWingsFromJson           (this->wings);
    }
};


RigProperties::RigProperties():
    m_root_data(nullptr),
    m_hide_in_chooser(false),
    m_forward_commands(false),
    m_import_commands(false),
    m_is_rescuer(false),
    m_disable_default_sounds(false),
    m_globals_dry_mass(0.f), // This is a default
    m_globals_load_mass(0.f), // This is a default
    m_minimass(0.f), // This is a default
    m_enable_advanced_deform(true),
    m_rollon(false)
{}

RigProperties::~RigProperties()
{}

void RigProperties::Import(std::shared_ptr<RigDef::File> def_file)
{
    m_title                      = def_file->name;
    m_guid                       = def_file->guid;
    m_hide_in_chooser            = def_file->hide_in_chooser;
    m_forward_commands           = def_file->forward_commands;
    m_import_commands            = def_file->import_commands;
    m_is_rescuer                 = def_file->rescuer;
    m_rollon                     = def_file->rollon;
    m_minimass                   = def_file->minimum_mass;
    m_enable_advanced_deform     = def_file->enable_advanced_deformation;
    m_disable_default_sounds     = def_file->disable_default_sounds;
    m_slidenodes_connect_instant = def_file->slide_nodes_connect_instantly;
    m_collision_range            = def_file->collision_range;
    m_lockgroup_default_nolock   = def_file->lockgroup_default_nolock;

    auto desc_end = def_file->description.end();
    for (auto itor = def_file->description.begin(); itor != desc_end; ++itor )
    {
        m_description.push_back(*itor);
    }

    auto authors_end = def_file->authors.end();
    for (auto itor = def_file->authors.begin(); itor != authors_end; ++itor )
    {
        m_authors.push_back(*itor);
    }

    RigDef::Fileinfo* fileinfo = def_file->file_info.get();
    if (fileinfo != nullptr)
    {
        m_fileinfo = *fileinfo;
    }

    RigDef::ExtCamera* ext_camera = def_file->root_module->ext_camera.get();
    if (ext_camera != nullptr)
    {
        m_extcamera = *ext_camera;
    }
    
    RigDef::Globals* globals = def_file->root_module->globals.get();
    if (globals != nullptr)
    {
        m_globals_cab_material_name = globals->material_name;
        m_globals_dry_mass          = globals->dry_mass;
        m_globals_load_mass         = globals->cargo_mass;
    }
    
    m_skeleton_settings  = def_file->root_module->skeleton_settings;

    if (m_root_data != nullptr) { delete m_root_data; m_root_data = nullptr; }
    if (m_root_data == nullptr) { m_root_data = new RigModuleData(); }

    m_root_data->BuildFromModule(def_file->root_module);
}

void RigProperties::Export(std::shared_ptr<RigDef::File> def_file)
{
    def_file->name                          = m_title;
    def_file->guid                          = m_guid;
    def_file->hide_in_chooser               = m_hide_in_chooser;
    def_file->forward_commands              = m_forward_commands;
    def_file->import_commands               = m_import_commands;
    def_file->rescuer                       = m_is_rescuer;
    def_file->rollon                        = m_rollon;
    def_file->minimum_mass                  = m_minimass;
    def_file->enable_advanced_deformation   = m_enable_advanced_deform;
    def_file->disable_default_sounds        = m_disable_default_sounds;
    def_file->slide_nodes_connect_instantly = m_slidenodes_connect_instant;
    def_file->collision_range               = m_collision_range;
    def_file->lockgroup_default_nolock      = m_lockgroup_default_nolock;

    def_file->file_info 
        = std::shared_ptr<RigDef::Fileinfo>(new RigDef::Fileinfo(m_fileinfo));
    def_file->root_module->ext_camera 
        = std::shared_ptr<RigDef::ExtCamera>(new RigDef::ExtCamera(m_extcamera));
    def_file->root_module->skeleton_settings = m_skeleton_settings;

    // Globals
    RigDef::Globals globals;
    globals.cargo_mass               = m_globals_load_mass;
    globals.dry_mass                 = m_globals_dry_mass;
    globals.material_name            = m_globals_cab_material_name;
    def_file->root_module->globals   = std::shared_ptr<RigDef::Globals>(new RigDef::Globals(globals));

    // Description
    for (auto itor = m_description.begin(); itor != m_description.end(); ++itor)
    {
        def_file->description.push_back(*itor);
    }

    // Authors
    for (auto itor = m_authors.begin(); itor != m_authors.end(); ++itor)
    {
        def_file->authors.push_back(*itor);
    }

    m_root_data->ExportToModule(def_file->root_module);
}

inline rapidjson::Value StrToJson(std::string const & s) { return rapidjson::Value(rapidjson::StringRef(s.c_str())); }
inline rapidjson::Value StrToJson(std::string & s)       { return rapidjson::Value(rapidjson::StringRef(s.c_str())); }

rapidjson::Value RigProperties::ExportJson(JsonExporter& exporter)
{
    auto& j_alloc = exporter.GetDocument().GetAllocator();

    // Global properties
    rapidjson::Value j_data(rapidjson::kObjectType);
    j_data.AddMember("project_fileformat_version", 1                                             , j_alloc);
    j_data.AddMember("name"                      , StrToJson(m_title)                            , j_alloc);
    j_data.AddMember("guid"                      , StrToJson(m_guid)                             , j_alloc);
    j_data.AddMember("hide_in_chooser"           , m_hide_in_chooser                             , j_alloc);
    j_data.AddMember("forward_commands"          , m_forward_commands                            , j_alloc);
    j_data.AddMember("import_commands"           , m_import_commands                             , j_alloc);
    j_data.AddMember("is_rescuer"                , m_is_rescuer                                  , j_alloc);
    j_data.AddMember("is_rollon"                 , m_rollon                                      , j_alloc);
    j_data.AddMember("minimum_mass"              , m_minimass                                    , j_alloc);
    j_data.AddMember("enable_advanced_deform"    , m_enable_advanced_deform                      , j_alloc);
    j_data.AddMember("disable_default_sounds"    , m_disable_default_sounds                      , j_alloc);
    j_data.AddMember("slidenodes_connect_instant", m_slidenodes_connect_instant                  , j_alloc);
    j_data.AddMember("collision_range"           , m_collision_range                             , j_alloc);
    j_data.AddMember("lockgroup_default_nolock"  , m_lockgroup_default_nolock                    , j_alloc);
    j_data.AddMember("fileinfo_uid"              , StrToJson(m_fileinfo.unique_id)               , j_alloc);
    j_data.AddMember("fileinfo_category_id"      , m_fileinfo.category_id                        , j_alloc);
    j_data.AddMember("fileinfo_version"          , m_fileinfo.file_version                       , j_alloc);
    j_data.AddMember("extcam_mode"               , m_extcamera.mode                              , j_alloc);
    j_data.AddMember("extcam_node_id"            , StrToJson(m_extcamera.node.Str())             , j_alloc);
    j_data.AddMember("visibility_range_meters"   , m_skeleton_settings.visibility_range_meters,    j_alloc);
    j_data.AddMember("beam_thickness_meters"     , m_skeleton_settings.beam_thickness_meters     , j_alloc);
    j_data.AddMember("globals_load_mass"         , m_globals_load_mass                           , j_alloc);
    j_data.AddMember("globals_dry_mass"          , m_globals_dry_mass                            , j_alloc);
    j_data.AddMember("globals_cab_material_name" , StrToJson(m_globals_cab_material_name)        , j_alloc);

    // Description
    std::stringstream desc;
    for (auto itor = m_description.begin(); itor != m_description.end(); ++itor)
    {
        desc << *itor << std::endl;
    }
    j_data.AddMember("description", StrToJson(desc.str()), j_alloc);

    // Authors
    rapidjson::Value author_array(rapidjson::kArrayType);
    for (auto itor = m_authors.begin(); itor != m_authors.end(); ++itor)
    {
        rapidjson::Value author(rapidjson::kObjectType);
        author.AddMember("name",  StrToJson(itor->name), j_alloc);
        author.AddMember("role",  StrToJson(itor->type), j_alloc);
        author.AddMember("email", StrToJson(itor->email), j_alloc);
        if (itor->_has_forum_account)
            author.AddMember("forum_id", itor->forum_account_id, j_alloc);
        
        author_array.PushBack(author, j_alloc);
    }
    j_data.AddMember("authors", author_array, j_alloc);

    exporter.AddRigPropertiesJson(j_data);

    // The root module stubs
    m_root_data->ExportRigModuleToJson(exporter);

    return j_data;
}

void RigProperties::ImportJson(JsonImporter& importer)
{

    rapidjson::Value& j_properties = importer.GetRigPropertiesJson();
    // Global properties
    m_title                                     = j_properties["name"].GetString();
    m_guid                                      = j_properties["guid"].GetString();
    m_hide_in_chooser                           = j_properties["hide_in_chooser"].GetBool();
    m_forward_commands                          = j_properties["forward_commands"].GetBool();
    m_import_commands                           = j_properties["import_commands"].GetBool();
    m_is_rescuer                                = j_properties["is_rescuer"].GetBool();
    m_rollon                                    = j_properties["is_rollon"].GetBool();
    m_minimass                                  = j_properties["minimum_mass"].GetFloat();
    m_enable_advanced_deform                    = j_properties["enable_advanced_deform"].GetBool();
    m_disable_default_sounds                    = j_properties["disable_default_sounds"].GetBool();
    m_slidenodes_connect_instant                = j_properties["slidenodes_connect_instant"].GetBool();
    m_collision_range                           = j_properties["collision_range"].GetFloat();
    m_lockgroup_default_nolock                  = j_properties["lockgroup_default_nolock"].GetBool();
    m_fileinfo.unique_id                        = j_properties["fileinfo_uid"].GetString();
    m_fileinfo.category_id                      = j_properties["fileinfo_category_id"].GetBool();
    m_fileinfo.file_version                     = j_properties["fileinfo_version"].GetBool();
    m_extcamera.mode                            = static_cast<RigDef::ExtCamera::Mode>(j_properties["extcam_mode"].GetInt()); // TODO: check validity for the cast!
    m_extcamera.node                            = JsonImporter::JsonToNodeRef(j_properties["extcam_node"]);
    m_skeleton_settings.visibility_range_meters = j_properties["visibility_range_meters"].GetFloat();
    m_skeleton_settings.beam_thickness_meters   = j_properties["beam_thickness_meters"].GetFloat();
    m_globals_load_mass                         = j_properties["globals_load_mass"].GetFloat();
    m_globals_dry_mass                          = j_properties["globals_dry_mass"].GetFloat();
    m_globals_cab_material_name                 = j_properties["globals_cab_material_name"].GetString();

    m_description.push_back(j_properties["description"].GetString());

    // Authors
    for (auto itor = j_properties["authors"].Begin(); itor != j_properties["authors"].End(); ++itor)
    {
        RigDef::Author author;
        author.name   = (*itor)["name"].GetString();
        author.email  = (*itor)["email"].GetString();
        author.type   = (*itor)["role"].GetString();
        if (itor->HasMember("forum_id"))
        {
            author._has_forum_account = true;
            author.forum_account_id = (*itor)["forum_id"].GetUint();
        }
    }

    m_root_data->ImportRigModuleFromJson(importer);
}

std::shared_ptr<RigDef::Engine>    RigProperties::GetEngine()          { return m_root_data->engine; }
std::shared_ptr<RigDef::Engoption> RigProperties::GetEngoption()       { return m_root_data->engoption; }

void  RigProperties::SetEngine     (std::shared_ptr<RigDef::Engine> engine)        { m_root_data->engine      = engine;     }
void  RigProperties::SetEngoption  (std::shared_ptr<RigDef::Engoption> engoption)  { m_root_data->engoption   = engoption;  }

} // namespace RigEditor
} // namespace RoR
