/*
    This source file is part of Rigs of Rods
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

#include "ActorEditor.h"

#include "Application.h"
#include "ContentManager.h"
#include "GUIManager.h"
#include "Language.h"
#include "TruckSerializer.h"

#include <OgreDataStream.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

using namespace RoR;

// static
bool ActorEditor::ReLoadProjectFromDirectory(ProjectEntry* proj)
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

bool ActorEditor::SaveProject(ProjectEntry* proj)
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

bool ActorEditor::ImportSnapshotToProject(std::string const& filename, std::shared_ptr<Truck::File> src_def)
{
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
    m_def = std::make_shared<Truck::File>();
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
    m_def->file_info = std::make_shared<Truck::Fileinfo>();
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

    // Save the snapshot
    ProjectSnapshot snap;
    snap.prs_filename = filename_buf;
    snap.prs_name = filename_buf;
    m_snapshot = &snap; // Temporary, just for the initial save.
    if (!this->SaveSnapshot())
    {
        return false; // Error already reported
    }

    // Persist the snapshot in project
    m_entry->prj_snapshots.push_back(snap);
    m_snapshot = &m_entry->prj_snapshots.back();
    return true;
}

void ActorEditor::ImportModuleToSnapshot(std::shared_ptr<Truck::File::Module> src)
{
    std::shared_ptr<Truck::File::Module> dst = std::make_shared<Truck::File::Module>(src->name);

    dst->help_panel_material_name   = src->help_panel_material_name   ; //Ogre::String                       
    dst->contacter_nodes            = src->contacter_nodes            ; //std::vector<unsigned int>          
    dst->airbrakes                  = src->airbrakes                  ; //std::vector<Airbrake>              
    dst->animators                  = src->animators                  ; //std::vector<Animator>              
    dst->anti_lock_brakes           = src->anti_lock_brakes           ; //std::shared_ptr<AntiLockBrakes>    
    dst->axles                      = src->axles                      ; //std::vector<Axle>                  
    dst->beams                      = src->beams                      ; //std::vector<Beam>
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

    dst->editor_groups              = src->editor_groups              ; //std::vector<EditorGroup>  

    if (src->name == Truck::ROOT_MODULE_NAME)
    {
        m_def->root_module = dst;
    }
    else
    {
        m_def->user_modules.insert(std::make_pair(src->name, dst));
    }
}

bool ActorEditor::SaveSnapshot()
{
    try
    {
        Ogre::ResourceGroupManager& rgm = Ogre::ResourceGroupManager::getSingleton();

        // Open OGRE stream for writing
        const bool overwrite = true;
        Ogre::DataStreamPtr stream = rgm.createResource(m_snapshot->prs_filename, m_entry->prj_rg_name, overwrite);
        if (stream.isNull() || !stream->isWriteable())
        {
            OGRE_EXCEPT(Ogre::Exception::ERR_CANNOT_WRITE_TO_FILE,
                "Stream NULL or not writeable, filename: '" + m_snapshot->prs_filename
                + "', resource group: '" + m_entry->prj_rg_name + "'");
        }

        // Serialize actor to string
        Truck::Serializer serializer(m_def);
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

