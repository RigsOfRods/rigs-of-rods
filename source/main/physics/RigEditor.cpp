/*
    This source file is part of Rigs of Rods
    Copyright 2018 Petr Ohlidal & contributors

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

#include "RigEditor.h"

#include "Application.h"
#include "BeamData.h"
#include "Console.h"
#include "ContentManager.h"
#include "GUIManager.h"
#include "Language.h"
#include "RigDef_Serializer.h"
#include "RoRFrameListener.h"
#include "PlatformUtils.h"

#include <OgreDataStream.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <sstream>
#include <string>

using namespace RoR;

// static
bool RigEditor::ReLoadProjectFromDirectory(ProjectEntry* proj)
{
    Ogre::ResourceGroupManager& rgm = Ogre::ResourceGroupManager::getSingleton();

    // Load project file
    Ogre::DataStreamPtr stream = rgm.openResource(PROJECT_FILE, proj->prj_rg_name);
    std::string proj_file_data(stream->size() + 1, '\0');
    stream->read(&proj_file_data[0], stream->size()); // Copy entire project file to buffer
    rapidjson::StringStream j_stream(proj_file_data.c_str());
    rapidjson::Document j_doc;
    j_doc.ParseStream(j_stream);
    if (!j_doc.IsObject() ||
        !j_doc.HasMember("name") || !j_doc["name"].IsString() || 
        !j_doc.HasMember("format_version") || !j_doc["format_version"].IsNumber())
    {
        LogFormat("[RoR|Editor] Loading project directory '%s' failed, JSON invalid", proj->prj_dirname);
        proj->prj_valid = false;
        return false;
    }

    // Update project entry
    proj->prj_name = j_doc["name"].GetString();
    proj->prj_format_version = j_doc["format_version"].GetUint();

    // List actor snapshots
    proj->prj_snapshots.clear();
    Ogre::FileInfoListPtr truckfiles = rgm.findResourceFileInfo(proj->prj_rg_name, "*.truck"); // It's always *.truck in project folders!
    for (Ogre::FileInfo truckfile: *truckfiles)
    {
        ProjectSnapshot snap;
        snap.prs_filename = truckfile.filename;
        snap.prs_name = truckfile.filename; // TODO: load actual truckfile name
        proj->prj_snapshots.push_back(snap);
    }

    return true;
}

bool RigEditor::SaveProject(ProjectEntry* proj)
{
    // Create JSON
    rapidjson::Document j_doc;
    j_doc.SetObject();
    j_doc.AddMember("name", rapidjson::StringRef(proj->prj_name.c_str()), j_doc.GetAllocator());
    j_doc.AddMember("format_version", 1, j_doc.GetAllocator());

    // Write JSON to file
    if (!App::GetContentManager()->SerializeAndWriteJson(PROJECT_FILE, proj->prj_rg_name, j_doc))
    {
        App::GetGuiManager()->ShowMessageBox(_L("Error"), _L("Could not save project file, see RoR.log for details"));
        return false;
    }
    return true;
}

bool RigEditor::ImportSnapshotToProject(std::string const& filename, std::shared_ptr<RigDef::File> src_def)
{
    assert(m_snapshot == -1);
    assert(m_entry);

    // Generate filename (avoid duplicates)
    Str<200> filename_buf;
    static int import_counter = 0;
    bool is_unique = true;
    do
    {
        is_unique = true;
        filename_buf << "imported_" << import_counter++ << "_" << filename;
        for (auto& snap: m_entry->prj_snapshots)
        {
            if (snap.prs_filename == filename_buf.ToCStr())
            {
                is_unique = false;
                break;
            }
        }
    }
    while (!is_unique);

    // Create new blank actor
    m_def = std::make_shared<RigDef::File>();
    m_def->name = "(Imported) " + src_def->name;

    // copy global attributes
    m_def->file_format_version           = src_def->file_format_version;
    m_def->guid                          = src_def->guid; // string
    m_def->hide_in_chooser               = src_def->hide_in_chooser                ;   //bool   
    m_def->enable_advanced_deformation   = src_def->enable_advanced_deformation    ;   //bool   
    m_def->slide_nodes_connect_instantly = src_def->slide_nodes_connect_instantly  ;   //bool   
    m_def->rollon                        = src_def->rollon                         ;   //bool   
    m_def->forward_commands              = src_def->forward_commands               ;   //bool   
    m_def->import_commands               = src_def->import_commands                ;   //bool   
    m_def->lockgroup_default_nolock      = src_def->lockgroup_default_nolock       ;   //bool   
    m_def->rescuer                       = src_def->rescuer                        ;   //bool   
    m_def->disable_default_sounds        = src_def->disable_default_sounds         ;   //bool   
    m_def->name                          = src_def->name                           ;   //String 
    m_def->collision_range               = src_def->collision_range                ;   //float  
    m_def->global_minimass               = src_def->global_minimass                ;   //float  
    m_def->description                   = src_def->description                    ;   // vector<string>
    m_def->authors                       = src_def->authors                        ;   // vector<>
    m_def->file_info = std::make_shared<RigDef::Fileinfo>();
    if (src_def->file_info)
    {
        *(m_def->file_info.get()) = *(src_def->file_info.get()); // Copy the object contents
    }

    // Vehicle modules (caled 'sections' in truckfile doc)
    this->ImportModuleToSnapshot(src_def->root_module);
    for (auto module_itor: src_def->user_modules)
    {
        this->ImportModuleToSnapshot(module_itor.second);
    }

    // Add snapshot to project
    ProjectSnapshot snap;
    snap.prs_filename = filename_buf;
    snap.prs_name = filename_buf;
    m_entry->prj_snapshots.push_back(snap);
    m_snapshot = (int)m_entry->prj_snapshots.size() - 1;

    // Save the snapshot
    if (!this->SaveSnapshot())
    {
        m_entry->prj_snapshots.pop_back();
        m_snapshot = -1;
        return false; // Error already reported
    }

    return true;
}

void RigEditor::ImportModuleToSnapshot(std::shared_ptr<RigDef::File::Module> src)
{
    std::shared_ptr<RigDef::File::Module> dst = std::make_shared<RigDef::File::Module>(src->name);

    dst->help_panel_material_name   = src->help_panel_material_name   ; //Ogre::String                       
    dst->contacter_nodes            = src->contacter_nodes            ; //std::vector<unsigned int>          
    dst->airbrakes                  = src->airbrakes                  ; //std::vector<Airbrake>              
    dst->animators                  = src->animators                  ; //std::vector<Animator>              
    dst->anti_lock_brakes           = src->anti_lock_brakes           ; //std::shared_ptr<AntiLockBrakes>    
    dst->axles                      = src->axles                      ; //std::vector<Axle>                  
    dst->beams                      = src->beams                      ; //std::vector<Beam>
    dst->beam_editor_groups         = src->beam_editor_groups         ; //std::vector<EditorGroup>  
    dst->brakes                     = src->brakes                     ; //std::shared_ptr<Brakes>            
    dst->cameras                    = src->cameras                    ; //std::vector<Camera>                
    dst->camera_rails               = src->camera_rails               ; //std::vector<CameraRail>            
    dst->collision_boxes            = src->collision_boxes            ; //std::vector<CollisionBox>          
    dst->cinecam                    = src->cinecam                    ; //std::vector<Cinecam>               
    dst->commands_2                 = src->commands_2                 ; //std::vector<Command2>              
    dst->cruise_control             = src->cruise_control             ; //std::shared_ptr<CruiseControl>     
    dst->contacters                 = src->contacters                 ; //std::vector<Node::Ref>             
    dst->engine                     = src->engine                     ; //std::shared_ptr<Engine>            
    dst->engoption                  = src->engoption                  ; //std::shared_ptr<Engoption>         
    dst->engturbo                   = src->engturbo                   ; //std::shared_ptr<Engturbo>          
    dst->exhausts                   = src->exhausts                   ; //std::vector<Exhaust>               
    dst->ext_camera                 = src->ext_camera                 ; //std::shared_ptr<ExtCamera>         
    dst->fixes                      = src->fixes                      ; //std::vector<Node::Ref>             
    dst->flares_2                   = src->flares_2                   ; //std::vector<Flare2>                
    dst->flexbodies                 = src->flexbodies                 ; //std::vector<shared_ptr<Flexbody>>  
    dst->flex_body_wheels           = src->flex_body_wheels           ; //std::vector<FlexBodyWheel>         
    dst->fusedrag                   = src->fusedrag                   ; //std::vector<Fusedrag>              
    dst->globals                    = src->globals                    ; //std::shared_ptr<Globals>           
    dst->gui_settings               = src->gui_settings               ; //std::shared_ptr<GuiSettings>       
    dst->hooks                      = src->hooks                      ; //std::vector<Hook>                  
    dst->hydros                     = src->hydros                     ; //std::vector<Hydro>                 
    dst->interaxles                 = src->interaxles                 ; //std::vector<InterAxle>             
    dst->lockgroups                 = src->lockgroups                 ; //std::vector<Lockgroup>             
    dst->managed_materials          = src->managed_materials          ; //std::vector<ManagedMaterial>       
    dst->material_flare_bindings    = src->material_flare_bindings    ; //std::vector<MaterialFlareBinding>  
    dst->mesh_wheels                = src->mesh_wheels                ; //std::vector<MeshWheel>             
    dst->nodes                      = src->nodes                      ; //std::vector<Node>                  
    dst->node_collisions            = src->node_collisions            ; //std::vector<NodeCollision> 
    dst->node_editor_groups         = src->node_editor_groups         ; //std::vector<EditorGroup>  
    dst->particles                  = src->particles                  ; //std::vector<Particle>              
    dst->pistonprops                = src->pistonprops                ; //std::vector<Pistonprop>            
    dst->props                      = src->props                      ; //std::vector<Prop>                  
    dst->railgroups                 = src->railgroups                 ; //std::vector<RailGroup>             
    dst->ropables                   = src->ropables                   ; //std::vector<Ropable>               
    dst->ropes                      = src->ropes                      ; //std::vector<Rope>                  
    dst->rotators                   = src->rotators                   ; //std::vector<Rotator>               
    dst->rotators_2                 = src->rotators_2                 ; //std::vector<Rotator2>              
    dst->screwprops                 = src->screwprops                 ; //std::vector<Screwprop>             
    dst->shocks                     = src->shocks                     ; //std::vector<Shock>                 
    dst->shocks_2                   = src->shocks_2                   ; //std::vector<Shock2>                
    dst->shocks_3                   = src->shocks_3                   ; //std::vector<Shock3>                
    dst->skeleton_settings          = src->skeleton_settings          ; //SkeletonSettings                   
    dst->slidenodes                 = src->slidenodes                 ; //std::vector<SlideNode>             
    dst->slope_brake                = src->slope_brake                ; //std::shared_ptr<SlopeBrake>        
    dst->soundsources               = src->soundsources               ; //std::vector<SoundSource>           
    dst->soundsources2              = src->soundsources2              ; //std::vector<SoundSource2>          
    dst->speed_limiter              = src->speed_limiter              ; //SpeedLimiter                       
    dst->submeshes_ground_model_name= src->submeshes_ground_model_name; //Ogre::String                        
    dst->submeshes                  = src->submeshes                  ; //std::vector<Submesh>                
    dst->ties                       = src->ties                       ; //std::vector<Tie>                    
    dst->torque_curve               = src->torque_curve               ; //std::shared_ptr<TorqueCurve>        
    dst->traction_control           = src->traction_control           ; //std::shared_ptr<TractionControl>    
    dst->transfer_case              = src->transfer_case              ; //std::shared_ptr<TransferCase>       
    dst->triggers                   = src->triggers                   ; //std::vector<Trigger>                
    dst->turbojets                  = src->turbojets                  ; //std::vector<Turbojet>               
    dst->turboprops_2               = src->turboprops_2               ; //std::vector<Turboprop2>             
    dst->videocameras               = src->videocameras               ; //std::vector<VideoCamera>            
    dst->wheeldetachers             = src->wheeldetachers             ; //std::vector<WheelDetacher>          
    dst->wheels                     = src->wheels                     ; //std::vector<Wheel>                  
    dst->wheels_2                   = src->wheels_2                   ; //std::vector<Wheel2>                 
    dst->wings                      = src->wings                      ; //std::vector<Wing>                   

    if (src->name == RigDef::ROOT_MODULE_NAME)
    {
        m_def->root_module = dst;
    }
    else
    {
        m_def->user_modules.insert(std::make_pair(src->name, dst));
    }
}

bool RigEditor::SaveSnapshot()
{
    try
    {
        Ogre::ResourceGroupManager& rgm = Ogre::ResourceGroupManager::getSingleton();

        // Open OGRE stream for writing
        const bool overwrite = true;
        Ogre::DataStreamPtr stream = rgm.createResource(m_entry->prj_snapshots[m_snapshot].prs_filename, m_entry->prj_rg_name, overwrite);
        if (stream.isNull() || !stream->isWriteable())
        {
            OGRE_EXCEPT(Ogre::Exception::ERR_CANNOT_WRITE_TO_FILE,
                "Stream NULL or not writeable, filename: '" + m_entry->prj_snapshots[m_snapshot].prs_filename
                + "', resource group: '" + m_entry->prj_rg_name + "'");
        }

        // Serialize actor to string
        RigDef::Serializer serializer(m_def);
        serializer.Serialize();

        // Flush the string to file
        stream->write(serializer.GetOutput().c_str(), serializer.GetOutput().size());
        stream->close();
        return true;
    }
    catch (Ogre::Exception& oex)
    {
        std::string msg = "Failed to save project snapshot, message:\n" + oex.getFullDescription();
        LogFormat("[RoR] %s", msg.c_str());
        App::GetGuiManager()->ShowMessageBox("Error!", msg.c_str());
        return false;
    }
}

void RigEditor::AddExampleScriptToSnapshot(std::string const& filename)
{
    assert(m_def);
    assert(m_entry);

    Str<50> dst_filename = "framestep_1.as";
    int num = 1;
    while (Ogre::ResourceGroupManager::getSingleton().resourceExists(m_entry->prj_rg_name, dst_filename.ToCStr()))
    {
        dst_filename = "";
        dst_filename << "framestep_" << num++ << ".as";
    }

    try
    {
        // Create empty file for the example script
        Ogre::DataStreamPtr out_stream
            = Ogre::ResourceGroupManager::getSingleton().createResource(
                dst_filename.ToCStr(), m_entry->prj_rg_name);

        // Open bundled demo script
        Ogre::DataStreamPtr demo_stream
            = Ogre::ResourceGroupManager::getSingleton().openResource(
                filename, ContentManager::ResourcePack::SCRIPTS.resource_group_name);

        // Buffer welcome text
        std::stringstream buf;
        buf << "\n// " << _L("This script demonstrates what data are provided for you and how you can visualize them using ImGui.");
        buf << "\n";
        buf << "\n// " << _L("For reference manual of the AngelScript language, visit http://angelcode.com/angelscript/sdk/docs/manual/doc_script.html");
        buf << "\n";
        buf << "\n// " << _L("State of current actor (truck/load...) is in object ActorSimBuffer available via GfxActor. Note Y coordinate means 'UP'.");
        buf << "\n// \t" << _L("vector3  pos                     ~ XYZ position (approximate rotation center)");
        buf << "\n// \t" << _L("vector3  node0_velo              ~ XYZ velocity, measured on node 0");
        buf << "\n// \t" << _L("bool     live_local              ~ Is the actor local (not networked) and actively simulated?");
        buf << "\n// \t" << _L("bool     physics_paused          ~ Is this actor individually paused?");
        buf << "\n// \t" << _L("float    rotation                ~ Rotation along Y axis");
        buf << "\n// \t" << _L("float    tyre_pressure           ~ 0-100");
        buf << "\n// \t" << _L("string   net_username            ~ Multiplayer username");
        buf << "\n// \t" << _L("bool     is_remote               ~ Is this actor remote (networked)?");
        buf << "\n// \t" << _L("int      gear                    ~ Current gearbox gear. 0 is neutral, -1 and lower are reverse, 1 and higher are forward");
        buf << "\n// \t" << _L("int      autoshift               ~ Automatic shift mode, see enum `autoswitch`");
        buf << "\n// \t" << _L("float    wheel_speed             ~ Average wheel speed (m/s)");
        buf << "\n// \t" << _L("float    engine_rpm              ~ Engine - RPM (Revs Per Minute)");
        buf << "\n// \t" << _L("float    engine_crankfactor      ~ Engine - hydraulics power ratio (0 - 5)");
        buf << "\n// \t" << _L("float    engine_turbo_psi        ~ Engine - turbocharger pressure in Psi");
        buf << "\n// \t" << _L("float    engine_accel            ~ ");
        buf << "\n// \t" << _L("float    engine_torque           ~ ");
        buf << "\n// \t" << _L("float    inputshaft_rpm          ~ ");
        buf << "\n// \t" << _L("float    drive_ratio             ~ ");
        buf << "\n// \t" << _L("bool     beaconlight_active      ~ ");
        buf << "\n// \t" << _L("float    hydro_dir_state         ~ ");
        buf << "\n// \t" << _L("float    hydro_aileron_state     ~ ");
        buf << "\n// \t" << _L("float    hydro_elevator_state    ~ ");
        buf << "\n// \t" << _L("float    hydro_aero_rudder_state ~ ");
        buf << "\n// \t" << _L("int      cur_cinecam             ~ ");
        buf << "\n// \t" << _L("bool     parking_brake           ~ ");
        buf << "\n// \t" << _L("float    brake                   ~ ");
        buf << "\n// \t" << _L("float    clutch                  ~ ");
        buf << "\n// \t" << _L("int      aero_flap_state         ~ ");
        buf << "\n// \t" << _L("int      airbrake_state          ~ ");
        buf << "\n// \t" << _L("float    wing4_aoa               ~ ");
        buf << "\n// \t" << _L("bool     headlight_on            ~ ");
        buf << "\n// \t" << _L("vector3  direction               ~ ");
        buf << "\n// \t" << _L("float    top_speed               ~ ");
        buf << "\n// \t" << _L("int      ap_heading_mode         ~ Flight autopilot");
        buf << "\n// \t" << _L("int      ap_heading_value        ~ Flight autopilot");
        buf << "\n// \t" << _L("int      ap_alt_mode             ~ Flight autopilot");
        buf << "\n// \t" << _L("int      ap_alt_value            ~ Flight autopilot");
        buf << "\n// \t" << _L("bool     ap_ias_mode             ~ Flight autopilot");
        buf << "\n// \t" << _L("int      ap_ias_value            ~ Flight autopilot");
        buf << "\n// \t" << _L("bool     ap_gpws_mode            ~ Flight autopilot");
        buf << "\n// \t" << _L("bool     ap_ils_available        ~ Flight autopilot");
        buf << "\n// \t" << _L("float    ap_ils_vdev             ~ Flight autopilot");
        buf << "\n// \t" << _L("float    ap_ils_hdev             ~ Flight autopilot");
        buf << "\n// \t" << _L("int      ap_vs_value             ~ Flight autopilot");
        buf << "\n\n";

        // Buffer localized literals
        Str<1000> path_buf;
        path_buf << App::sys_projects_dir->GetActiveStr()
                 << PATH_SLASH << m_entry->prj_dirname
                 << PATH_SLASH << dst_filename.ToCStr();
            
        buf << "string L_main_welcome = \"" << _L("This window is drawn by script, you can view and edit it: ")
            << PathStrEscape(path_buf.ToCStr()) << "\";";


        // Write out the buffer
        std::string out_str = buf.str();
        out_stream->write(out_str.c_str(), out_str.length());

        // Copy the example code
        const int BUF_LEN = 1000;
        char* demo_buf[BUF_LEN] = {};
        size_t num_read = 0;
        while ((num_read = demo_stream->read(&demo_buf, BUF_LEN)) != 0)
        {
            out_stream->write(demo_buf, num_read);
        }

        // Add script to snapshot
        m_def->root_module->scripts.push_back(RigDef::Script(RigDef::Script::TYPE_FRAMESTEP, dst_filename.ToCStr(), ""));
    }
    catch (Ogre::Exception& oex)
    {
        std::string msg = "Failed to add example script to project, message:" + oex.getFullDescription();
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING, msg);
    }
}

void RigEditor::LoadSnapshot(ProjectEntry* project, int snapshot)
{
    if (m_entry)
    {
        this->CloseProject();
    }

    assert(!m_entry);
    assert(m_snapshot == -1);
    
    assert(project);
    assert(snapshot >= 0 && snapshot < (int)project->prj_snapshots.size());

    m_entry = project;
    m_snapshot = snapshot;

    ActorSpawnRequest rq;
    rq.asr_origin = ActorSpawnRequest::Origin::USER;
    rq.asr_project = project;
    rq.asr_filename = project->prj_snapshots[snapshot].prs_filename;
    App::GetSimController()->QueueActorSpawn(rq);
}

void RigEditor::CloseProject()
{
    assert(m_entry);
    m_entry = nullptr;
    m_snapshot = -1;
    m_def = nullptr;
}

bool RigEditor::CreateProjet(std::string const& dir_name, std::string const& proj_name)
{
    assert(!m_entry);
    assert(m_snapshot == -1);

    ProjectEntry* proj = App::GetContentManager()->CreateNewProject(dir_name, proj_name);
    if (proj)
    {
        m_entry = proj;
        return true;
    }
    else
    {
        return false; // Error already logged
    }
}
