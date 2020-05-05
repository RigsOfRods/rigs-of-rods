/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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

#include "RoRPrerequisites.h"

#include "Application.h"
#include "CacheSystem.h"
#include "Console.h"
#include "ContentManager.h"
#include "ErrorUtils.h"
#include "GUIManager.h"
#include "GUI_MainSelector.h"
#include "InputEngine.h"
#include "Language.h"
#include "MainMenu.h"
#include "MumbleIntegration.h"
#include "OgreSubsystem.h"
#include "PlatformUtils.h"
#include "RoRFrameListener.h"
#include "RoRVersion.h"
#include "Scripting.h"
#include "Settings.h"
#include "Skidmark.h"
#include "SoundScriptManager.h"
#include "Utils.h"

#include <Overlay/OgreOverlaySystem.h>
#include <ctime>
#include <iomanip>
#include <string>
#include <fstream>

#ifdef USE_CURL
#   include <curl/curl.h>
#endif //USE_CURL

GlobalEnvironment* gEnv;         // Global pointer used throughout the game. Declared in "RoRPrerequisites.h". TODO: Eliminate
GlobalEnvironment  gEnvInstance; // The actual instance

#ifdef __cplusplus
extern "C" {
#endif

int main(int argc, char *argv[])
{
    using namespace RoR;

#ifdef USE_CURL
    curl_global_init(CURL_GLOBAL_ALL); // MUST init before any threads are started
#endif

#ifndef _DEBUG
    try
    {
#endif

        App::GetConsole()->CVarSetupBuiltins();

        gEnv = &gEnvInstance;

        // ### Detect system paths ###

        // Process directory
        std::string exe_path = RoR::GetExecutablePath();
        if (exe_path.empty())
        {
            ErrorUtils::ShowError(_L("Startup error"), _L("Error while retrieving program directory path"));
            return -1;
        }
        App::sys_process_dir->SetActiveStr(RoR::GetParentDirectory(exe_path.c_str()).c_str());

        // RoR's home directory
        std::string local_userdir = PathCombine(App::sys_process_dir->GetActiveStr(), "config"); // TODO: Think of a better name, this is ambiguious with ~/.rigsofrods/config which stores configfiles! ~ only_a_ptr, 02/2018
        if (FolderExists(local_userdir))
        {
            // It's a portable installation
            App::sys_user_dir->SetActiveStr(local_userdir.c_str());
        }
        else
        {
            // Default location - user's home directory
            std::string user_home = RoR::GetUserHomeDirectory();
            if (user_home.empty())
            {
                ErrorUtils::ShowError(_L("Startup error"), _L("Error while retrieving user directory path"));
                return -1;
            }
            RoR::Str<500> ror_homedir;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
            ror_homedir << user_home << PATH_SLASH << "My Games";
            CreateFolder(ror_homedir.ToCStr());
            ror_homedir << PATH_SLASH << "Rigs of Rods";
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
            char* env_SNAP = getenv("SNAP_USER_COMMON");
            if(env_SNAP)
                ror_homedir = env_SNAP;
            else
                ror_homedir << user_home << PATH_SLASH << ".rigsofrods";
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
            ror_homedir << user_home << PATH_SLASH << "RigsOfRods";
#endif
            CreateFolder(ror_homedir.ToCStr ());
            App::sys_user_dir->SetActiveStr(ror_homedir.ToCStr ());
        }

        // ### Create OGRE default logger early. ###

        std::string logs_dir = PathCombine(App::sys_user_dir->GetActiveStr(), "logs");
        CreateFolder(logs_dir);
        App::sys_logs_dir->SetActiveStr(logs_dir.c_str());

        auto ogre_log_manager = OGRE_NEW Ogre::LogManager();
        std::string rorlog_path = PathCombine(logs_dir, "RoR.log");
        Ogre::Log* rorlog = ogre_log_manager->createLog(rorlog_path, true, true);
        rorlog->stream() << "[RoR] Rigs of Rods (www.rigsofrods.org) version " << ROR_VERSION_STRING;
        std::time_t t = std::time(nullptr);
        rorlog->stream() << "[RoR] Current date: " << std::put_time(std::localtime(&t), "%Y-%m-%d");
        rorlog->addListener(App::GetConsole());  // Allow console to intercept log messages

        if (! Settings::SetupAllPaths()) // Updates globals
        {
            ErrorUtils::ShowError(_L("Startup error"), _L("Resources folder not found. Check if correctly installed."));
            return -1;
        }

        Settings::getSingleton().LoadRoRCfg(); // Main config file - path obtained from GVars
        if (!FolderExists(App::sys_projects_dir->GetActiveStr()))
        {
            CreateFolder(App::sys_projects_dir->GetActiveStr());
        }

        Settings::getSingleton().ProcessCommandLine(argc, argv);

        if (App::app_state->GetPendingEnum<AppState>() == AppState::PRINT_HELP_EXIT)
        {
            ShowCommandLineUsage();
            return 0;
        }
        if (App::app_state->GetPendingEnum<AppState>() == AppState::PRINT_VERSION_EXIT)
        {
            ShowVersion();
            return 0;
        }

        App::StartOgreSubsystem();

        Ogre::String src_path = PathCombine(App::sys_resources_dir->GetActiveStr(), "skeleton.zip");
        Ogre::ResourceGroupManager::getSingleton().addResourceLocation(src_path, "Zip", "SrcRG");
        Ogre::FileInfoListPtr fl = Ogre::ResourceGroupManager::getSingleton().findResourceFileInfo("SrcRG", "*", true);
        if (fl->empty())
        {
            ErrorUtils::ShowError(_L("Startup error"), _L("Faulty resource folder. Check if correctly installed."));
            return -1;
        }
        Ogre::String dst_path = PathCombine(App::sys_user_dir->GetActiveStr(), "");
        for (auto file : *fl)
        {
            CreateFolder(dst_path + file.basename);
        }
        fl = Ogre::ResourceGroupManager::getSingleton().findResourceFileInfo("SrcRG", "*");
        if (fl->empty())
        {
            ErrorUtils::ShowError(_L("Startup error"), _L("Faulty resource folder. Check if correctly installed."));
            return -1;
        }
        Ogre::ResourceGroupManager::getSingleton().addResourceLocation(dst_path, "FileSystem", "DstRG", false, false);
        for (auto file : *fl)
        {
            if (file.uncompressedSize == 0)
                continue;
            Ogre::String path = file.path + file.basename;
            if (!Ogre::ResourceGroupManager::getSingleton().findResourceFileInfo("DstRG", path)->empty())
                continue;
            Ogre::DataStreamPtr src_ds = Ogre::ResourceGroupManager::getSingleton().openResource(path, "SrcRG");
            Ogre::DataStreamPtr dst_ds = Ogre::ResourceGroupManager::getSingleton().createResource(path, "DstRG");
            std::vector<char> buf(src_ds->size());
            size_t read = src_ds->read(buf.data(), src_ds->size());
            if (read > 0)
            {
                dst_ds->write(buf.data(), read);
            }
        }
        Ogre::ResourceGroupManager::getSingleton().destroyResourceGroup("SrcRG");
        Ogre::ResourceGroupManager::getSingleton().destroyResourceGroup("DstRG");

        Ogre::OverlaySystem* overlay_system = new Ogre::OverlaySystem(); //Overlay init

        Ogre::ConfigOptionMap ropts = App::GetOgreSubsystem()->GetOgreRoot()->getRenderSystem()->getConfigOptions();
        int resolution = Ogre::StringConverter::parseInt(Ogre::StringUtil::split(ropts["Video Mode"].currentValue, " x ")[0], 1024);
        int fsaa = 2 * (Ogre::StringConverter::parseInt(ropts["FSAA"].currentValue, 0) / 4);
        int res = std::pow(2, std::floor(std::log2(resolution)));

        Ogre::TextureManager::getSingleton().createManual ("EnvironmentTexture",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_CUBE_MAP, res / 4, res / 4, 0,
            Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET, 0, false, fsaa);
        Ogre::TextureManager::getSingleton ().createManual ("Refraction",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, res / 2, res / 2, 0,
            Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET, 0, false, fsaa);
        Ogre::TextureManager::getSingleton ().createManual ("Reflection",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, res / 2, res / 2, 0,
            Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET, 0, false, fsaa);

        if (!App::diag_warning_texture->GetActiveVal<bool>())
        {
            // We overwrite the default warning texture (yellow stripes) with something unobtrusive
            Ogre::uchar data[3] = {0};
            Ogre::PixelBox pixels(1, 1, 1, Ogre::PF_BYTE_RGB, &data);
            Ogre::TextureManager::getSingleton()._getWarningTexture()->getBuffer()->blitFromMemory(pixels);
        }

        App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::FLAGS);
        App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::FONTS);
        App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::ICONS);
        App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::OGRE_CORE);
        App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::WALLPAPERS);

#ifndef NOLANG
        LanguageEngine::getSingleton().setup();
#endif // NOLANG
        App::GetConsole()->RegBuiltinCommands(); // Call after localization had been set up

        // Setup rendering (menu + simulation)
        Ogre::SceneManager* scene_manager = App::GetOgreSubsystem()->GetOgreRoot()->createSceneManager(Ogre::ST_EXTERIOR_CLOSE, "main_scene_manager");
        gEnv->sceneManager = scene_manager;
        if (overlay_system)
        {
            scene_manager->addRenderQueueListener(overlay_system);
        }

        Ogre::Camera* camera = scene_manager->createCamera("PlayerCam");
        camera->setPosition(Ogre::Vector3(128, 25, 128)); // Position it at 500 in Z direction
        camera->lookAt(Ogre::Vector3(0, 0, -300)); // Look back along -Z
        camera->setNearClipDistance(0.5);
        camera->setFarClipDistance(1000.0 * 1.733);
        camera->setFOVy(Ogre::Degree(60));
        camera->setAutoAspectRatio(true);
        App::GetOgreSubsystem()->GetViewport()->setCamera(camera);
        gEnv->mainCamera = camera;

        App::GetContentManager()->InitContentManager();

        App::CreateGuiManagerIfNotExists();
        scene_manager->addRenderQueueListener(&App::GetGuiManager()->GetImGui());

        InitDiscord();

#ifdef USE_ANGELSCRIPT
        new ScriptEngine(); // Init singleton. TODO: Move under Application
#endif

        App::CreateInputEngine();
        App::GetInputEngine()->setupDefault(App::GetOgreSubsystem()->GetMainHWND());

        RoR::App::GetInputEngine()->windowResized(App::GetOgreSubsystem()->GetRenderWindow());
        App::GetGuiManager()->SetUpMenuWallpaper();
        MainMenu main_obj;
        App::GetOgreSubsystem()->GetOgreRoot()->addFrameListener(&main_obj); // HACK until OGRE 1.12 migration; We need a frame listener to display 'progress' window ~ only_a_ptr, 10/2019

        App::GetContentManager()->InitModCache();

        RoR::ForceFeedback force_feedback;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        if (App::io_ffb_enabled->GetActiveVal<bool>()) // Force feedback
        {
            if (App::GetInputEngine()->getForceFeedbackDevice())
            {
                force_feedback.Setup();
            }
            else
            {
                LOG("No force feedback device detected, disabling force feedback");
                App::io_ffb_enabled->SetActiveVal(false);
            }
        }

        Ogre::String old_ror_homedir = Ogre::StringUtil::format("%s\\Rigs of Rods 0.4", RoR::GetUserHomeDirectory().c_str());
        if(FolderExists(old_ror_homedir))
        {
            if (!FileExists(Ogre::StringUtil::format("%s\\OBSOLETE_FOLDER.txt", old_ror_homedir.c_str())))
            {
                Ogre::String obsoleteMessage = Ogre::StringUtil::format("This folder is obsolete, please move your mods to  %s", App::sys_user_dir->GetActiveStr());
                try
                {
                    Ogre::ResourceGroupManager::getSingleton().addResourceLocation(old_ror_homedir, "FileSystem", "homedir", false, false);
                    Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().createResource("OBSOLETE_FOLDER.txt", "homedir");
                    stream->write(obsoleteMessage.c_str(), obsoleteMessage.length());
                    Ogre::ResourceGroupManager::getSingleton().destroyResourceGroup("homedir");
                }
                catch (std::exception & e)
                {
                    RoR::LogFormat("Error writing to %s, message: '%s'", old_ror_homedir.c_str(), e.what());
                }
                Ogre::String message = Ogre::StringUtil::format(
                    "Welcome to Rigs of Rods %s\nPlease note that the mods folder has moved to:\n\"%s\"\nPlease move your mods to the new folder to continue using them",
                    ROR_VERSION_STRING_SHORT,
                    App::sys_user_dir->GetActiveStr()
                );

                RoR::App::GetGuiManager()->ShowMessageBox("Obsolete folder detected", message.c_str());
            }
        }
#endif // OGRE_PLATFORM_WIN32

        SkidmarkConfig skidmark_conf; // Loads 'skidmark.cfg' in constructor

        // ### Main loop (switches application states) ###

        App::app_state->SetPendingVal((int)AppState::MAIN_MENU);

        if (App::mp_join_on_startup->GetActiveVal<bool>())
        {
            App::mp_state->SetPendingVal((int)RoR::MpState::CONNECTED);
        }
        else if (App::diag_preset_terrain->GetActiveStr() != "")
        {
            App::app_state->SetPendingVal((int)AppState::SIMULATION);
        }
        else if (App::sim_savegame->GetActiveStr() != App::sim_savegame->GetPendingStr())
        {
            App::app_state->SetPendingVal((int)AppState::SIMULATION);
        }

        while (App::app_state->GetPendingEnum<AppState>() != AppState::SHUTDOWN)
        {
            if (App::app_state->GetPendingEnum<AppState>() == AppState::MAIN_MENU)

            {
                App::app_state->ApplyPending();

#ifdef USE_OPENAL
                if (App::audio_menu_music->GetActiveVal<bool>())
                {
                    SoundScriptManager::getSingleton().createInstance("tracks/main_menu_tune", -1, nullptr);
                    SOUND_START(-1, SS_TRIG_MAIN_MENU);
                }
#endif // USE_OPENAL

                App::GetGuiManager()->ReflectGameState();
                if (!App::mp_join_on_startup->GetActiveVal<bool>() && App::app_skip_main_menu->GetActiveVal<bool>())
                {
                    // MainMenu disabled (singleplayer mode) -> go directly to map selector (traditional behavior)
                    if (App::diag_preset_terrain->GetActiveStr() == "")
                    {
                        App::GetGuiManager()->SetVisible_GameMainMenu(false);
                        App::GetGuiManager()->GetMainSelector()->Show(LT_Terrain);
                    }
                }

                main_obj.EnterMainMenuLoop();
            }
            else if (App::app_state->GetPendingEnum<AppState>() == AppState::SIMULATION)
            {
                { // Enclosing scope for SimController
                    SimController sim_controller(&force_feedback, &skidmark_conf);
                    App::SetSimController(&sim_controller);
                    if (sim_controller.SetupGameplayLoop())
                    {
                        App::app_state->ApplyPending();
                        App::GetOgreSubsystem()->GetOgreRoot()->removeFrameListener(&main_obj);     // HACK until OGRE 1.12 migration; We need a frame listener to display loading window ~ only_a_ptr, 10/2019
                        App::GetGuiManager()->ReflectGameState();
                        App::sim_state->SetActiveVal((int)SimState::RUNNING);
                        sim_controller.EnterGameplayLoop();
                        App::SetSimController(nullptr);
                        App::GetMainMenu()->LeaveMultiplayerServer();
#ifdef USE_MUMBLE
                        if (App::GetMumble() != nullptr)
                        {
                            App::GetMumble()->SetNonPositionalAudio();
                        }
#endif // USE_MUMBLE
                    }
                    else
                    {
                        App::app_state->SetPendingVal((int)AppState::MAIN_MENU);
                    }
                } // Enclosing scope for SimController
                gEnv->sceneManager->clearScene(); // Wipe the scene after SimController was destroyed
                App::sim_terrain_name->SetActiveStr("");
                App::sim_terrain_gui_name->SetActiveStr("");
                App::GetOgreSubsystem()->GetOgreRoot()->addFrameListener(&main_obj); // HACK until OGRE 1.12 migration; Needed for GUI display, must be done ASAP ~ only_a_ptr, 10/2019
            }


        } // End of app state loop

        // ========================================================================
        // Cleanup
        // ========================================================================

        ShutdownDiscord();

        Settings::getSingleton().SaveSettings(); // Save RoR.cfg

        App::GetGuiManager()->GetMainSelector()->~MainSelector();

        App::GetMainMenu()->LeaveMultiplayerServer();

        //TODO: we should destroy OIS here
        //TODO: we could also try to destroy SoundScriptManager, but we don't care!

        scene_manager->destroyCamera(camera);
        App::GetOgreSubsystem()->GetOgreRoot()->destroySceneManager(scene_manager);

        App::DestroyOverlayWrapper();
#ifndef _DEBUG
    }
    catch (Ogre::Exception& e)
    {
        LOG(e.getFullDescription());
        ErrorUtils::ShowError(_L("An exception has occured!"), e.getFullDescription());
    }
    catch (std::runtime_error& e)
    {
        LOG(e.what());
        ErrorUtils::ShowError(_L("An exception (std::runtime_error) has occured!"), e.what());
    }
#endif

    return 0;
}

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
{
    return main(__argc, __argv);
}
#endif

#ifdef __cplusplus
}
#endif
