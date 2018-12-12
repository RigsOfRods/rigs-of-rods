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

#include "CacheSystem.h"
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

#include <Overlay/OgreOverlaySystem.h>
#include <string>

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

    try
    {
        gEnv = &gEnvInstance;

        // ### Detect system paths ###

        // Process directory
        std::string exe_path = RoR::GetExecutablePath();
        if (exe_path.empty())
        {
            ErrorUtils::ShowError(_L("Startup error"), _L("Error while retrieving program directory path"));
            return -1;
        }
        App::sys_process_dir.SetActive(RoR::GetParentDirectory(exe_path.c_str()).c_str());

        // RoR's home directory
        std::string local_userdir = PathCombine(App::sys_process_dir.GetActive(), "config"); // TODO: Think of a better name, this is ambiguious with ~/.rigsofrods/config which stores configfiles! ~ only_a_ptr, 02/2018
        if (FolderExists(local_userdir))
        {
            // It's a portable installation
            App::sys_user_dir.SetActive(local_userdir.c_str());
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
            ror_homedir << user_home << PATH_SLASH << "Rigs of Rods " << ROR_VERSION_STRING_SHORT;
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
            ror_homedir << user_home << PATH_SLASH << ".rigsofrods";
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
            ror_homedir << user_home << PATH_SLASH << "RigsOfRods";
#endif
            CreateFolder(ror_homedir.ToCStr ());
            App::sys_user_dir.SetActive(ror_homedir.ToCStr ());
        }

        // ### Create OGRE default logger early. ###

        std::string logs_dir = PathCombine(App::sys_user_dir.GetActive(), "logs");
        CreateFolder(logs_dir);
        App::sys_logs_dir.SetActive(logs_dir.c_str());

        auto ogre_log_manager = OGRE_NEW Ogre::LogManager();
        std::string rorlog_path = PathCombine(logs_dir, "RoR.log");
        ogre_log_manager->createLog(rorlog_path, true, true);
        App::diag_trace_globals.SetActive(true); // We have logger -> we can trace.

        // ### Setup program paths ###

        if (! Settings::SetupAllPaths()) // Updates globals
        {
            ErrorUtils::ShowError(_L("Startup error"), _L("Resources folder not found. Check if correctly installed."));
            return -1;
        }

        Settings::getSingleton().LoadRoRCfg(); // Main config file - path obtained from GVars
        Settings::getSingleton().ProcessCommandLine(argc, argv);

        bool extract_skeleton = false;
        if (!FolderExists(App::sys_config_dir.GetActive()))
        {
            CreateFolder(App::sys_config_dir.GetActive());
            extract_skeleton = true;
        }
        App::StartOgreSubsystem();

        if (extract_skeleton)
        {
            Ogre::String src_path = PathCombine(App::sys_resources_dir.GetActive(), "skeleton.zip");
            Ogre::ResourceGroupManager::getSingleton().addResourceLocation(src_path, "Zip", "SrcRG");
            Ogre::FileInfoListPtr fl = Ogre::ResourceGroupManager::getSingleton().findResourceFileInfo("SrcRG", "*", true);
            if (fl->empty())
            {
                ErrorUtils::ShowError(_L("Startup error"), _L("Faulty resource folder. Check if correctly installed."));
                return -1;
            }
            Ogre::String dst_path = PathCombine(App::sys_user_dir.GetActive(), "");
            for (auto file : *fl)
            {
                CreateFolder(dst_path + file.filename);
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
                Ogre::String path = file.path + file.filename;
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
        }

        if (App::app_state.GetPending() == AppState::PRINT_HELP_EXIT)
        {
            ShowCommandLineUsage();
            return 0;
        }
        if (App::app_state.GetPending() == AppState::PRINT_VERSION_EXIT)
        {
            ShowVersion();
            return 0;
        }

        Ogre::OverlaySystem* overlay_system = new Ogre::OverlaySystem(); //Overlay init
        Ogre::TextureManager::getSingleton().createManual ("EnvironmentTexture",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_CUBE_MAP, 256, 256, 0,
            Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET);
        Ogre::TextureManager::getSingleton ().createManual ("Refraction",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, 512, 512, 0,
            Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET);
        Ogre::TextureManager::getSingleton ().createManual ("Reflection",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, 512, 512, 0,
            Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET);


        App::CreateContentManager();

        LanguageEngine::getSingleton().setup();

        // Add startup resources
        App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::OGRE_CORE);
        App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::WALLPAPERS);

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

        Ogre::String menu_wallpaper_texture_name = GUIManager::getRandomWallpaperImage();

        App::CreateCacheSystem(); // Reads GVars

        // Initialize "managed materials"
        // These are base materials referenced by user content
        // They must be initialized before any content is loaded, including mod-cache update.
        // Otherwise material links are unresolved and loading ends with an exception
        // TODO: Study Ogre::ResourceLoadingListener and implement smarter solution (not parsing materials on cache refresh!)
        App::GetContentManager()->InitManagedMaterials();

        App::GetContentManager()->OnApplicationStartup();

        App::CreateGuiManagerIfNotExists();

        // Load and show menu wallpaper
        MyGUI::VectorWidgetPtr v = MyGUI::LayoutManager::getInstance().loadLayout("wallpaper.layout");
        MyGUI::Widget* menu_wallpaper_widget = nullptr;
        if (!v.empty())
        {
            MyGUI::Widget* mainw = v.at(0);
            if (mainw)
            {
                MyGUI::ImageBox* img = (MyGUI::ImageBox *)(mainw->getChildAt(0));
                if (img)
                    img->setImageTexture(menu_wallpaper_texture_name);
                menu_wallpaper_widget = mainw;
            }
        }

#ifdef USE_ANGELSCRIPT
        new ScriptEngine(); // Init singleton. TODO: Move under Application
#endif

        App::CreateInputEngine();
        App::GetInputEngine()->setupDefault(App::GetOgreSubsystem()->GetMainHWND());

        App::GetCacheSystem()->Startup();

        RoR::ForceFeedback force_feedback;
#ifdef _WIN32
        if (App::io_ffb_enabled.GetActive()) // Force feedback
        {
            if (App::GetInputEngine()->getForceFeedbackDevice())
            {
                force_feedback.Setup();
            }
            else
            {
                LOG("No force feedback device detected, disabling force feedback");
                App::io_ffb_enabled.SetActive(false);
            }
        }
#endif // _WIN32

#ifdef USE_MPLATFORM
	    m_frame_listener->m_platform = new MPlatform_FD();
	    if (m_frame_listener->m_platform)
	    {
		    m_platform->connect();
	    }
#endif

        RoR::App::GetInputEngine()->windowResized(App::GetOgreSubsystem()->GetRenderWindow());

        MainMenu main_obj;
        SkidmarkConfig skidmark_conf; // Loads 'skidmark.cfg' in constructor

        // ### Main loop (switches application states) ###

        AppState prev_app_state = App::app_state.GetActive();
        App::app_state.SetPending(AppState::MAIN_MENU);

        if (App::mp_join_on_startup.GetActive() == true)
        {
            App::mp_state.SetPending(RoR::MpState::CONNECTED);
        }
        else if (!App::diag_preset_terrain.IsActiveEmpty())
        {
            App::app_state.SetPending(AppState::SIMULATION);
        }

        while (App::app_state.GetPending() != AppState::SHUTDOWN)
        {
            if (App::app_state.GetPending() == AppState::MAIN_MENU)
           
            {
                App::app_state.ApplyPending();

#ifdef USE_OPENAL
                if (App::audio_menu_music.GetActive())
                {
                    SoundScriptManager::getSingleton().createInstance("tracks/main_menu_tune", -1, nullptr);
                    SOUND_START(-1, SS_TRIG_MAIN_MENU);
                }
#endif // USE_OPENAL

                App::GetGuiManager()->ReflectGameState();
                if (!App::mp_join_on_startup.GetActive() && App::app_skip_main_menu.GetActive())
                {
                    // MainMenu disabled (singleplayer mode) -> go directly to map selector (traditional behavior)
                    if (App::diag_preset_terrain.IsActiveEmpty())
                    {
                        App::GetGuiManager()->SetVisible_GameMainMenu(false);
                        App::GetGuiManager()->GetMainSelector()->Show(LT_Terrain);
                    }
                }

                main_obj.EnterMainMenuLoop();
            }
            else if (App::app_state.GetPending() == AppState::SIMULATION)
            {
                { // Enclosing scope for SimController
                    SimController sim_controller(&force_feedback, &skidmark_conf);
                    App::SetSimController(&sim_controller);
                    if (sim_controller.SetupGameplayLoop())
                    {
                        App::app_state.ApplyPending();
                        App::GetGuiManager()->ReflectGameState();
                        App::sim_state.SetActive(SimState::RUNNING);
                        sim_controller.EnterGameplayLoop();
                        App::SetSimController(nullptr);
#ifdef USE_SOCKETW
                        if (App::mp_state.GetActive() == MpState::CONNECTED)
                        {
                            RoR::Networking::Disconnect();
                            App::GetGuiManager()->SetVisible_MpClientList(false);
                        }
#endif // USE_SOCKETW
#ifdef USE_MUMBLE
                        if (App::GetMumble() != nullptr)
                        {
                            App::GetMumble()->SetNonPositionalAudio();
                        }
#endif // USE_MUMBLE
                        menu_wallpaper_widget->setVisible(true);
                    }
                    else
                    {
                        App::app_state.SetPending(AppState::MAIN_MENU);
                    }
                } // Enclosing scope for SimController
                gEnv->sceneManager->clearScene(); // Wipe the scene after SimController was destroyed
            }
            prev_app_state = App::app_state.GetActive();


        } // End of app state loop

        // ========================================================================
        // Cleanup
        // ========================================================================

        Settings::getSingleton().SaveSettings(); // Save RoR.cfg

        App::GetGuiManager()->GetMainSelector()->~MainSelector();

#ifdef USE_SOCKETW
        if (App::mp_state.GetActive() == MpState::CONNECTED)
        {
            RoR::Networking::Disconnect();
        }
#endif //SOCKETW

        //TODO: we should destroy OIS here
        //TODO: we could also try to destroy SoundScriptManager, but we don't care!

#ifdef USE_MPLATFORM
	    if (frame_listener->mplatform != nullptr)
	    {
		    if (frame_listener->mplatform->disconnect())
		    {
			    delete(frame_listener->mplatform);
			    frame_listener->mplatform = nullptr;
		    }
	    }
#endif

        scene_manager->destroyCamera(camera);
        App::GetOgreSubsystem()->GetOgreRoot()->destroySceneManager(scene_manager);

        App::DestroyOverlayWrapper();

        App::DestroyContentManager();
    }
    catch (Ogre::Exception& e)
    {
        ErrorUtils::ShowError(_L("An exception has occured!"), e.getFullDescription());
    }
    catch (std::runtime_error& e)
    {
        ErrorUtils::ShowError(_L("An exception (std::runtime_error) has occured!"), e.what());
    }

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
