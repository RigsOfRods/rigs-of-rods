/*
    This source file is part of Rigs of Rods
    Copyright 2013-2021 Petr Ohlidal

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

#include "ProjectManager.h"

#include "Application.h"
#include "Console.h"
#include "ContentManager.h"
#include "GameContext.h"
#include "GUIManager.h"
#include "Language.h"
#include "PlatformUtils.h"
#include "TruckSerializer.h"

#include <OgreDataStream.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

#include <efsw/efsw.hpp>
#include <fmt/core.h>

using namespace RoR;

// --------------------------------
// Project list management

void ProjectManager::ReScanProjects()
{
    // Initialize projects dir
    if (!Ogre::ResourceGroupManager::getSingleton().resourceGroupExists(RGN_PROJECTS))
    {
        Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
            App::sys_projects_dir->GetStr(), "FileSystem", RGN_PROJECTS, /*recursive=*/false, /*readOnly=*/false);
    }

    // Mark all entries "out of sync"
    for (ProjectPtr& project: m_projects)
    {
        project->prj_sync = ProjectSyncState::UNKNOWN;
    }

    // List project directories and check if entries exist.
    Ogre::FileInfoListPtr files = Ogre::ResourceGroupManager::getSingleton().findResourceFileInfo(RGN_PROJECTS, "*", /*only_dirs:*/true);
    for (Ogre::FileInfo proj_dir: *files)
    {
        try
        {
            // Find or create project entry
            ProjectPtr project = this->FindProjectByDirName(proj_dir.filename);
            if (!project)
            {
                project = this->RegisterProjectDir(proj_dir.filename);
            }

            // (Re)Load the entry
            this->ReScanProjectDir(project);
        }
        catch (Ogre::Exception& oex)
        {
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_PROJECT, Console::CONSOLE_SYSTEM_ERROR,
                                          fmt::format("Problem scanning directory '{}': {}", proj_dir.filename, oex.getFullDescription()));
        }
    }

    // Delete entries which are still out of sync (directory missing or corrupted)
    this->PruneInvalidProjects();

#if USE_EFSW
    this->StartWatchingFileSystem();
#endif // USE_ESFW
}

ProjectPtr ProjectManager::RegisterProjectDir(std::string const& dirname)
{
    // create entry
    ProjectPtr project = std::make_shared<Project>();
    project->prj_dirname = dirname;
    project->prj_rg_name = fmt::format("project {}", dirname);
    m_projects.push_back(project);

    // create resource group
    std::string path = PathCombine(App::sys_projects_dir->GetStr(), dirname);
    Ogre::ResourceGroupManager::getSingleton().addResourceLocation(path, "FileSystem", project->prj_rg_name, /*recursive=*/false, /*readOnly=*/false);
    Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup(project->prj_rg_name);

    return project;
}

void ProjectManager::ReScanProjectDir(ProjectPtr proj)
{
    this->ReLoadResources(proj);

    // List actor snapshots (shared_ptr cleans up the old data)
    proj->prj_trucks = Ogre::ResourceGroupManager::getSingleton().findResourceNames(proj->prj_rg_name, "*.truck"); // It's always *.truck in project folders!
    proj->prj_sync = ProjectSyncState::OK;
}

ProjectPtr ProjectManager::FindProjectByDirName(std::string const& dirname)
{
    for (ProjectPtr& project: m_projects)
    {
        if (project->prj_dirname == dirname)
        {
            return project;
        }
    }
    return nullptr;
}

void ProjectManager::PruneInvalidProjects()
{
    for (auto itor = m_projects.begin(); itor != m_projects.end(); ++itor)
    {
        if (itor->get()->prj_sync != ProjectSyncState::OK)
        {
            Ogre::ResourceGroupManager::getSingleton().destroyResourceGroup(itor->get()->prj_rg_name);
            itor = m_projects.erase(itor);
        }
    }
}

// --------------------------------
// Project handling

ProjectPtr ProjectManager::CreateNewProject(std::string const& dir_name)
{
    try
    {
        // Create project directory
        Str<150> dir_name_buf(dir_name);
        Str<300> dir_path;
        dir_path << App::sys_projects_dir->GetStr() << PATH_SLASH << dir_name;
        while (RoR::FolderExists(dir_path.ToCStr()))
        {
            static int dir_counter = 2;
            dir_name_buf.Clear() << dir_name << dir_counter++;
            dir_path.Clear() << App::sys_projects_dir->GetStr() << PATH_SLASH << dir_name_buf;
        }
#if USE_EFSW
        this->StartWatchingFileSystem(); // Supress while manipulating files
#endif

        RoR::CreateFolder(dir_path.ToCStr());

#if USE_EFSW
        this->StartWatchingFileSystem(); // Resume
#endif

        // Create project entry
        return this->RegisterProjectDir(dir_name_buf.ToCStr());
    }
    catch (Ogre::Exception& oex)
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_PROJECT, Console::CONSOLE_SYSTEM_ERROR,
                                      fmt::format("Could not creat new project: {}", oex.getFullDescription()));
        return nullptr;
    }
}

bool ProjectManager::ImportTruckToProject(std::string const& src_filename, Truck::DocumentPtr src_def, CacheEntry* entry)
{
    ROR_ASSERT(m_active_project);

    std::string filename = this->MakeFilenameUniqueInProject(src_filename);

    // Create new blank actor
    m_active_truck_filename = filename;
    m_active_truck_def = std::make_shared<Truck::Document>();
    m_active_truck_def->name = "(Imported) " + src_def->name;

    // copy global attributes
    m_active_truck_def->file_format_version           = src_def->file_format_version;
    m_active_truck_def->guid                          = src_def->guid;
    m_active_truck_def->hide_in_chooser               = src_def->hide_in_chooser;
    m_active_truck_def->enable_advanced_deformation   = src_def->enable_advanced_deformation;
    m_active_truck_def->slide_nodes_connect_instantly = src_def->slide_nodes_connect_instantly;
    m_active_truck_def->rollon                        = src_def->rollon;
    m_active_truck_def->forward_commands              = src_def->forward_commands;
    m_active_truck_def->import_commands               = src_def->import_commands;
    m_active_truck_def->lockgroup_default_nolock      = src_def->lockgroup_default_nolock;
    m_active_truck_def->rescuer                       = src_def->rescuer;
    m_active_truck_def->disable_default_sounds        = src_def->disable_default_sounds;
    m_active_truck_def->name                          = src_def->name;
    m_active_truck_def->collision_range               = src_def->collision_range;
    m_active_truck_def->global_minimass               = src_def->global_minimass;
    m_active_truck_def->description                   = src_def->description;
    m_active_truck_def->authors                       = src_def->authors;
    m_active_truck_def->file_info = std::make_shared<Truck::Fileinfo>();
    if (src_def->file_info)
    {
        *(m_active_truck_def->file_info.get()) = *(src_def->file_info.get()); // Copy the object contents
    }

    // Vehicle modules (caled 'sections' in truckfile doc)
    this->ImportModuleToTruck(src_def->root_module);
    for (auto module_itor: src_def->user_modules)
    {
        this->ImportModuleToTruck(module_itor.second);
    }

#if USE_EFSW
        this->StartWatchingFileSystem(); // Supress while manipulating files
#endif

    // Save the truck file
    if (!this->SaveTruck())
    {
        return false; // Error already reported
    }

    // Import resources (use temporary RG, the existing one contains builtins, too)
    Ogre::ResourceGroupManager::getSingleton().addResourceLocation(entry->resource_bundle_path, entry->resource_bundle_type, RGN_TEMP);
    Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup(RGN_TEMP);
    Ogre::StringVectorPtr files = Ogre::ResourceGroupManager::getSingleton().findResourceNames(RGN_TEMP, "*", /*dirs=*/false);
    for (size_t i = 0; i < files->size(); ++i)
    {
        Ogre::String basename, ext;
        Ogre::StringUtil::splitBaseFilename(files->at(i), basename, ext);
        if (ext != "truck" && ext != "load" && ext != "car" && ext != "machine" &&
            ext != "airplane" && ext != "boat" && ext != "trailer" && ext != "train")
        {
            CopyResourceFile(files->at(i), m_active_project->prj_rg_name, files->at(i), RGN_TEMP);
        }
    }
    Ogre::ResourceGroupManager::getSingleton().destroyResourceGroup(RGN_TEMP);

#if USE_EFSW
        this->StartWatchingFileSystem(); // Resume
#endif

    // Update truck list in the project
    this->ReScanProjectDir(m_active_project);

    return true;
}

void ProjectManager::ImportModuleToTruck(Truck::ModulePtr src)
{
    Truck::ModulePtr dst = std::make_shared<Truck::Module>(src->name);

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
        m_active_truck_def->root_module = dst;
    }
    else
    {
        m_active_truck_def->user_modules.insert(std::make_pair(src->name, dst));
    }
}

bool ProjectManager::SaveTruck()
{
    try
    {
        Ogre::ResourceGroupManager& rgm = Ogre::ResourceGroupManager::getSingleton();

        // Open OGRE stream for writing
        const bool overwrite = true;
        Ogre::DataStreamPtr stream = rgm.createResource(m_active_truck_filename, m_active_project->prj_rg_name, overwrite);
        if (stream.isNull() || !stream->isWriteable())
        {
            OGRE_EXCEPT(Ogre::Exception::ERR_CANNOT_WRITE_TO_FILE,
                "Stream NULL or not writeable, filename: '" + m_active_truck_filename
                + "', resource group: '" + m_active_project->prj_rg_name + "'");
        }

        // Serialize actor to string
        Truck::Serializer serializer(m_active_truck_def);
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

void ProjectManager::ReLoadResources(ProjectPtr project)
{
    Ogre::ResourceGroupManager::getSingleton().unloadResourceGroup(project->prj_rg_name);
    Ogre::ResourceGroupManager::getSingleton().loadResourceGroup(project->prj_rg_name);
}

void ProjectManager::SetActiveProject(ProjectPtr project, std::string const& filename)
{
    m_active_project = project;
    m_active_truck_filename = filename;


}

std::string ProjectManager::MakeFilenameUniqueInProject(std::string const& src_filename)
{
    ROR_ASSERT(m_active_project);

    int count = 1;
    std::string filename = src_filename;
    while (Ogre::ResourceGroupManager::getSingleton().resourceExists(m_active_project->prj_rg_name, filename))
    {
        filename = fmt::format("({}){}", ++count, src_filename);
    }
    return filename;
}

// --------------------------------
// File change monitoring

#if USE_EFSW
void ProjectManager::handleFileAction( efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename )
{
    // ## IMPORTANT: This runs on EFSW's monitoring thread!

    if (dir == App::sys_projects_dir->GetStr())
    {
        // Activity happens in projects root directory
        if (!FolderExists(PathCombine(App::sys_projects_dir->GetStr(), filename)))
        {
            return; // We only care about subdirectories
        }

        if (action == efsw::Action::Add)
        {
            ProjectFsEvent* fs_event = new ProjectFsEvent();
            fs_event->pfe_action = ProjectFsAction::PROJECT_ADDED;
            fs_event->pfe_project_dir = filename;
            App::GetGameContext()->PushMessage(Message(MSG_EDI_PROJECT_FILESYSTEM_EVENT, (void*)fs_event));
        }
        else if (action == efsw::Action::Moved)
        {
            ProjectFsEvent* fs_event = new ProjectFsEvent();
            fs_event->pfe_action = ProjectFsAction::PROJECT_RENAMED;
            fs_event->pfe_project_dir = oldFilename;
            fs_event->pfe_new_name = filename;
            App::GetGameContext()->PushMessage(Message(MSG_EDI_PROJECT_FILESYSTEM_EVENT, (void*)fs_event));
        }
        else if (action == efsw::Action::Delete)
        {
            ProjectFsEvent* fs_event = new ProjectFsEvent();
            fs_event->pfe_action = ProjectFsAction::PROJECT_DELETED;
            fs_event->pfe_project_dir = filename;
            App::GetGameContext()->PushMessage(Message(MSG_EDI_PROJECT_FILESYSTEM_EVENT, (void*)fs_event));
        }
    }
    else
    {
        std::string slash = {PATH_SLASH};
        Ogre::StringVector dirs = Ogre::StringUtil::split(dir, slash);
        std::string project_dir = dirs.at(dirs.size() - 1);
        if (project_dir == "")
            project_dir = dirs.at(dirs.size() - 2);

        std::string base, ext;
        Ogre::StringUtil::splitBaseFilename(filename, base, ext);
        const bool is_truck = ext == "truck";

        if (action == efsw::Action::Add)
        {
            ProjectFsEvent* fs_event = new ProjectFsEvent();
            fs_event->pfe_action = is_truck ? ProjectFsAction::TRUCK_ADDED : ProjectFsAction::RESOURCE_ADDED;
            fs_event->pfe_project_dir = project_dir;
            fs_event->pfe_filename = filename;
            App::GetGameContext()->PushMessage(Message(MSG_EDI_PROJECT_FILESYSTEM_EVENT, (void*)fs_event));
        }
        else if (action == efsw::Action::Modified)
        {
            ProjectFsEvent* fs_event = new ProjectFsEvent();
            fs_event->pfe_action = is_truck ? ProjectFsAction::TRUCK_MODIFIED : ProjectFsAction::RESOURCE_MODIFIED;
            fs_event->pfe_project_dir = project_dir;
            fs_event->pfe_filename = filename;
            App::GetGameContext()->PushMessage(Message(MSG_EDI_PROJECT_FILESYSTEM_EVENT, (void*)fs_event));
        }
        else if (action == efsw::Action::Moved)
        {
            ProjectFsEvent* fs_event = new ProjectFsEvent();
            fs_event->pfe_action = is_truck ? ProjectFsAction::TRUCK_RENAMED : ProjectFsAction::RESOURCE_RENAMED;
            fs_event->pfe_project_dir = project_dir;
            fs_event->pfe_filename = oldFilename;
            fs_event->pfe_new_name = filename;
            App::GetGameContext()->PushMessage(Message(MSG_EDI_PROJECT_FILESYSTEM_EVENT, (void*)fs_event));
        }
        else if (action == efsw::Action::Delete)
        {
            ProjectFsEvent* fs_event = new ProjectFsEvent();
            fs_event->pfe_action = is_truck ? ProjectFsAction::TRUCK_DELETED : ProjectFsAction::RESOURCE_DELETED;
            fs_event->pfe_project_dir = project_dir;
            fs_event->pfe_filename = filename;
            App::GetGameContext()->PushMessage(Message(MSG_EDI_PROJECT_FILESYSTEM_EVENT, (void*)fs_event));
        }
    }
}

void ProjectManager::HandleFileSystemEvent(ProjectFsEvent* fs_event)
{
    ProjectPtr project;

    switch (fs_event->pfe_action)
    {
    case ProjectFsAction::PROJECT_ADDED:
        this->RegisterProjectDir(fs_event->pfe_project_dir);
        break;

    case ProjectFsAction::PROJECT_RENAMED:
        project = this->FindProjectByDirName(fs_event->pfe_project_dir);
        if (project)
        {
            // Update list entry
            project->prj_dirname = fs_event->pfe_new_name;
            this->ReLoadResources(project);

            // Reload actors
            for (Actor* actor: App::GetGameContext()->GetActorManager()->GetActors())
            {
                if (actor->GetProject() == project)
                {
                    ActorModifyRequest* request = new ActorModifyRequest();
                    request->amr_type = ActorModifyRequest::Type::RELOAD;
                    App::GetGameContext()->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)request));
                }
            }
        }
        break;

    case ProjectFsAction::PROJECT_DELETED:
        project = this->FindProjectByDirName(fs_event->pfe_project_dir);
        if (project)
        {
            // Unlist the project
            project->prj_sync = ProjectSyncState::DELETED;
            this->PruneInvalidProjects();

            // Destroy actors
            for (Actor* actor: App::GetGameContext()->GetActorManager()->GetActors())
            {
                if (actor->GetProject() == project)
                {
                    App::GetGameContext()->PushMessage(Message(MSG_SIM_DELETE_ACTOR_REQUESTED, (void*)actor));
                }
            }
        }
        break;

    case ProjectFsAction::TRUCK_ADDED:
        project = this->FindProjectByDirName(fs_event->pfe_project_dir);
        if (!project)
        {
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_PROJECT, Console::CONSOLE_SYSTEM_WARNING,
                                          fmt::format(_L("Spurious notification about added file '{}/{}' - no such project"),
                                                      fs_event->pfe_project_dir, fs_event->pfe_filename));
            return;
        }

        this->ReScanProjectDir(project);
        break;

    case ProjectFsAction::TRUCK_MODIFIED:
        project = this->FindProjectByDirName(fs_event->pfe_project_dir);
        if (!project)
        {
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_PROJECT, Console::CONSOLE_SYSTEM_WARNING,
                                          fmt::format(_L("Spurious notification about modified file '{}/{}' - no such project"),
                                                      fs_event->pfe_project_dir, fs_event->pfe_filename));
            return;
        }

        for (Actor* actor: App::GetGameContext()->GetActorManager()->GetActors())
        {
            if (actor->GetProject() == project && actor->GetActorFileName() == fs_event->pfe_filename)
            {
                ActorModifyRequest* request = new ActorModifyRequest();
                request->amr_type = ActorModifyRequest::Type::RELOAD;
                App::GetGameContext()->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)request));
            }
        }
        break;

    case ProjectFsAction::TRUCK_RENAMED:
        project = this->FindProjectByDirName(fs_event->pfe_project_dir);
        if (!project)
        {
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_PROJECT, Console::CONSOLE_SYSTEM_WARNING,
                                          fmt::format(_L("Spurious notification about renamed file '{}/{} => {}' - no such project"),
                                                      fs_event->pfe_project_dir, fs_event->pfe_filename, fs_event->pfe_new_name));
            return;
        }

        this->ReScanProjectDir(project);

        for (Actor* actor: App::GetGameContext()->GetActorManager()->GetActors())
        {
            if (actor->GetProject() == project && actor->GetActorFileName() == fs_event->pfe_filename)
            {
                // Inject new name
                actor->ar_filename = fs_event->pfe_new_name;
                // Reload actor from new file name
                ActorModifyRequest* request = new ActorModifyRequest();
                request->amr_type = ActorModifyRequest::Type::RELOAD;
                App::GetGameContext()->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)request));
            }
        }
        break;

    case ProjectFsAction::TRUCK_DELETED:
        project = this->FindProjectByDirName(fs_event->pfe_project_dir);
        if (!project)
        {
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_PROJECT, Console::CONSOLE_SYSTEM_WARNING,
                                          fmt::format(_L("Spurious notification about deleted file '{}/{}' - no such project"),
                                                      fs_event->pfe_project_dir, fs_event->pfe_filename));
            return;
        }

        this->ReScanProjectDir(project);
        break;

    case ProjectFsAction::RESOURCE_ADDED:
    case ProjectFsAction::RESOURCE_MODIFIED:
    case ProjectFsAction::RESOURCE_RENAMED:
    case ProjectFsAction::RESOURCE_DELETED:
        project = this->FindProjectByDirName(fs_event->pfe_project_dir);
        if (!project)
        {
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_PROJECT, Console::CONSOLE_SYSTEM_WARNING,
                                          fmt::format(_L("Spurious notification about resource file '{}/{}' - no such project"),
                                                      fs_event->pfe_project_dir, fs_event->pfe_filename));
            return;
        }

        // Reload all actors, just to be sure
        for (Actor* actor: App::GetGameContext()->GetActorManager()->GetActors())
        {
            if (actor->GetProject() == project)
            {
                ActorModifyRequest* request = new ActorModifyRequest();
                request->amr_type = ActorModifyRequest::Type::RELOAD;
                App::GetGameContext()->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)request));
            }
        }
        break;

    default:
        break;
    }
}

void ProjectManager::StartWatchingFileSystem()
{
    if (!m_watcher)
    {
        m_watcher = std::make_unique<efsw::FileWatcher>();
    }

    // Recursively watches ROR_HOME/projects
    m_watch_id = m_watcher->addWatch(App::sys_projects_dir->GetStr(), this, /*recursive=*/true);
    if (m_watch_id > 0)
    {
        m_watcher->watch();
    }
    else
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_PROJECT, Console::CONSOLE_SYSTEM_ERROR,
                                        fmt::format(_L("Could not set up filesystem watching for '{}', error code '{}'"),
                                                    App::sys_projects_dir->GetStr(), m_watch_id));
        m_watch_id = 0;
    }
}

void ProjectManager::StopWatchingFileSystem()
{
    if (m_watcher && m_watch_id > 0)
    {
        m_watcher->removeWatch(m_watch_id);
        m_watch_id = 0;
    }
}

#endif // USE_EFSW

