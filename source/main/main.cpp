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

#include "Application.h"
#include "AppContext.h"
#include "CacheSystem.h"
#include "ChatSystem.h"
#include "Console.h"
#include "ContentManager.h"
#include "ErrorUtils.h"
#include "GUIManager.h"
#include "GUI_LoadingWindow.h"
#include "GUI_MainSelector.h"
#include "GUI_MultiplayerSelector.h"
#include "InputEngine.h"
#include "Language.h"
#include "MumbleIntegration.h"
#include "OgreSubsystem.h"
#include "PlatformUtils.h"
#include "RoRFrameListener.h"
#include "RoRVersion.h"
#include "ScriptEngine.h"
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

        // User directories
        App::sys_config_dir    ->SetActiveStr(PathCombine(App::sys_user_dir->GetActiveStr(), "config").c_str());
        App::sys_cache_dir     ->SetActiveStr(PathCombine(App::sys_user_dir->GetActiveStr(), "cache").c_str());
        App::sys_savegames_dir ->SetActiveStr(PathCombine(App::sys_user_dir->GetActiveStr(), "savegames").c_str());
        App::sys_screenshot_dir->SetActiveStr(PathCombine(App::sys_user_dir->GetActiveStr(), "screenshots").c_str());

        // Resources dir
        std::string process_dir = PathCombine(App::sys_process_dir->GetActiveStr(), "resources");
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
        if (!FolderExists(process_dir))
        {
            process_dir = "/usr/share/rigsofrods/resources/";
        }
#endif
        if (!FolderExists(process_dir))
        {
            ErrorUtils::ShowError(_L("Startup error"), _L("Resources folder not found. Check if correctly installed."));
            return -1;
        }
        App::sys_resources_dir->SetActiveStr(process_dir);

        App::GetConsole()->LoadConfig(); // RoR.cfg
        App::GetConsole()->ProcessCommandLine(argc, argv);

        if (App::app_state_requested->GetActiveEnum<AppState>() == AppState::PRINT_HELP_EXIT)
        {
            App::GetConsole()->ShowCommandLineUsage();
            return 0;
        }
        if (App::app_state_requested->GetActiveEnum<AppState>() == AppState::PRINT_VERSION_EXIT)
        {
            App::GetConsole()->ShowCommandLineVersion();
            return 0;
        }

        App::CreateOgreSubsystem();

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
        App::GetLanguageEngine()->setup();
#endif // NOLANG
        App::GetConsole()->RegBuiltinCommands(); // Call after localization had been set up

        // Set up rendering
        App::CreateGfxScene(); // Creates OGRE SceneManager
        App::GetGfxScene()->GetSceneManager()->addRenderQueueListener(overlay_system);
        App::CreateCameraManager(); // Creates OGRE Camera
        App::GetGfxScene()->GetEnvMap().SetupEnvMap(); // Needs camera

        App::GetContentManager()->InitContentManager();

        App::CreateGuiManager(); // Needs scene manager

        InitDiscord();

#ifdef USE_ANGELSCRIPT
        App::CreateScriptEngine();
#endif

        App::CreateInputEngine();

        App::GetGuiManager()->SetUpMenuWallpaper();

        AppContext ctx; // Probably not the final location ~ 05/2020 Petr O.

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

        App::CreateThreadPool();

        App::app_state_requested->SetActiveVal((int)AppState::MAIN_MENU);

        if (App::mp_join_on_startup->GetActiveVal<bool>())
        {
            App::mp_state_requested->SetActiveVal((int)RoR::MpState::CONNECTED);
        }
        else if (App::diag_preset_terrain->GetActiveStr() != "")
        {
            App::app_state_requested->SetActiveVal((int)AppState::SIMULATION);
        }
        else if (App::sim_savegame->GetActiveStr() != App::sim_savegame->GetPendingStr())
        {
            App::app_state_requested->SetActiveVal((int)AppState::SIMULATION);
        }

        // --------------------------------------------------------------
        // Main rendering and event handling loop
        // --------------------------------------------------------------

        auto start_time = std::chrono::high_resolution_clock::now();

        while (App::app_state_requested->GetActiveEnum<AppState>() != AppState::SHUTDOWN)
        {
            OgreBites::WindowEventUtilities::messagePump();

            // Enter new app state
            if (App::app_state_requested->GetActiveEnum<AppState>() != App::app_state->GetActiveEnum<AppState>())
            {
                if (App::app_state_requested->GetActiveEnum<AppState>() == AppState::MAIN_MENU)
                {
                    App::app_state->SetActiveVal((int)AppState::MAIN_MENU);
                    App::GetGuiManager()->ReflectGameState();
#ifdef USE_OPENAL
                    if (App::audio_menu_music->GetActiveVal<bool>())
                    {
                        App::GetSoundScriptManager()->createInstance("tracks/main_menu_tune", -1, nullptr);
                        SOUND_START(-1, SS_TRIG_MAIN_MENU);
                    }
#endif // USE_OPENAL

                    if (!App::mp_join_on_startup->GetActiveVal<bool>() && App::app_skip_main_menu->GetActiveVal<bool>())
                    {
                        // MainMenu disabled (singleplayer mode) -> go directly to map selector (traditional behavior)
                        if (App::diag_preset_terrain->GetActiveStr() == "")
                        {
                            App::GetGuiManager()->SetVisible_GameMainMenu(false);
                            App::GetGuiManager()->GetMainSelector()->Show(LT_Terrain);
                        }
                    }
                }
                else if (App::app_state_requested->GetActiveEnum<AppState>() == AppState::SIMULATION)
                {
                    App::app_state->SetActiveVal((int)AppState::SIMULATION);
                    App::GetGuiManager()->ReflectGameState();
                    App::SetSimController(new SimController(&force_feedback, &skidmark_conf));
                    if (App::GetSimController()->SetupGameplayLoop())
                    {
                        App::sim_state->SetActiveVal((int)SimState::RUNNING);
                        App::sim_state_requested->SetActiveVal((int)SimState::RUNNING);
                        App::gfx_fov_external->SetActiveVal(App::gfx_fov_external_default->GetActiveVal<int>());
                        App::gfx_fov_internal->SetActiveVal(App::gfx_fov_internal_default->GetActiveVal<int>());
#ifdef USE_SOCKETW
                        if (App::mp_state->GetActiveEnum<MpState>() == MpState::CONNECTED)
                        {
                            char text[300];
                            std::snprintf(text, 300, _L("Press %s to start chatting"),
                                RoR::App::GetInputEngine()->getKeyForCommand(EV_COMMON_ENTER_CHATMODE).c_str());
                            App::GetConsole()->putMessage(
                                Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, text, "", 5000);
                        }
#endif // USE_SOCKETW
                    }
                    else
                    {
                        delete App::GetSimController();
                        App::SetSimController(nullptr);
                        App::app_state_requested->SetActiveVal((int)AppState::MAIN_MENU);
                    }
                }
            } // App state change block

#ifdef USE_SOCKETW
            // Network events
            NetEventQueue events = App::GetNetwork()->CheckEvents();
            while (!events.empty())
            {
                switch (events.front().type)
                {
                case NetEvent::Type::SERVER_KICK:
                    App::mp_state_requested->SetActiveVal((int)MpState::DISABLED);
                    App::app_state_requested->SetActiveVal((int)AppState::MAIN_MENU);
                    App::GetGuiManager()->ShowMessageBox(
                        _LC("Network", "Network disconnected"), events.front().message.c_str());
                    break;

                case NetEvent::Type::RECV_ERROR:
                    App::mp_state_requested->SetActiveVal((int)MpState::DISABLED);
                    App::app_state_requested->SetActiveVal((int)AppState::MAIN_MENU);
                    App::GetGuiManager()->ShowMessageBox(
                        _L("Network fatal error: "), events.front().message.c_str());
                    break;

                case NetEvent::Type::CONNECT_STARTED:
                    App::GetGuiManager()->SetMpConnectingStatusMsg(events.front().message.c_str());
                    App::GetGuiManager()->SetVisible_GameMainMenu(false);
                    App::GetGuiManager()->SetVisible_MultiplayerSelector(false);
                    break;

                case NetEvent::Type::CONNECT_PROGRESS:
                    App::GetGuiManager()->SetMpConnectingStatusMsg(events.front().message.c_str());
                    break;

                case NetEvent::Type::CONNECT_SUCCESS:
                    App::mp_state->SetActiveVal((int)RoR::MpState::CONNECTED);
                    RoR::ChatSystem::SendStreamSetup();
                    if (!App::GetMumble())
                    {
                        App::CreateMumble();
                    }
                    if (App::GetNetwork()->GetTerrainName() != "any")
                    {
                        App::sim_terrain_name->SetPendingStr(App::GetNetwork()->GetTerrainName());
                        App::app_state_requested->SetActiveVal((int)AppState::SIMULATION);
                    }
                    else
                    {
                        // Connected -> go directly to map selector
                        if (App::diag_preset_terrain->GetActiveStr().empty())
                        {
                            App::GetGuiManager()->GetMainSelector()->Show(LT_Terrain);
                        }
                        else
                        {
                            App::app_state_requested->SetActiveVal((int)AppState::SIMULATION);
                        }
                    }
                    break;

                case NetEvent::Type::CONNECT_FAILURE:
                    App::mp_state_requested->SetActiveVal((int)MpState::DISABLED);
                    App::GetGuiManager()->ShowMessageBox(
                        _LC("Network", "Multiplayer: connection failed"), events.front().message.c_str());
                    App::GetGuiManager()->ReflectGameState();
                    break;


                default:;
                }
                events.pop();
            } // Network connecting events block

            App::GetGuiManager()->GetMpSelector()->CheckAndProcessRefreshResult();

            // Change network state
            if (App::mp_state_requested->GetActiveEnum<MpState>() != App::mp_state->GetActiveEnum<MpState>())
            {
                if (App::mp_state_requested->GetActiveEnum<MpState>() == MpState::CONNECTED &&
                    App::mp_state->GetActiveEnum<MpState>() == MpState::DISABLED)
                {
                    App::GetNetwork()->StartConnecting();
                }
                else if (App::mp_state_requested->GetActiveEnum<MpState>() == MpState::DISABLED)
                {
                    App::GetNetwork()->Disconnect();
                    if (App::app_state->GetActiveEnum<AppState>() == AppState::MAIN_MENU)
                    {
                        App::GetGuiManager()->GetMainSelector()->Close(); // We may get disconnected while still in map selection
                        App::GetGuiManager()->SetVisible_GameMainMenu(true);
                    }
                }
            } // Net state change block
#endif // USE_SOCKETW

            // Check FPS limit
            if (App::gfx_fps_limit->GetActiveVal<int>() > 0)
            {
                const float min_frame_time = 1.0f / Ogre::Math::Clamp(App::gfx_fps_limit->GetActiveVal<int>(), 5, 240);
                float dt = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - start_time).count();
                while (dt < min_frame_time)
                {
                    dt = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - start_time).count();
                }
            } // Check FPS limit block

            // Calculate delta time
            const auto now = std::chrono::high_resolution_clock::now();
            const float dt_sec = std::chrono::duration<float>(now - start_time).count();
            start_time = now;

            // Check simulation state change
            if (App::app_state->GetActiveEnum<AppState>() == AppState::SIMULATION)
            {
                if (App::sim_state_requested->GetActiveEnum<SimState>() != App::sim_state->GetActiveEnum<SimState>())
                {
                    if (App::sim_state->GetActiveEnum<SimState>() == SimState::RUNNING)
                    {
                        if (App::sim_state_requested->GetActiveEnum<SimState>() == SimState::PAUSED)
                        {
                            App::GetSimController()->GetBeamFactory()->MuteAllActors();
                            App::sim_state->SetActiveVal((int)App::sim_state_requested->GetActiveEnum<SimState>());
                        }
                    }
                    else if (App::sim_state->GetActiveEnum<SimState>() == SimState::PAUSED)
                    {
                        if (App::sim_state_requested->GetActiveEnum<SimState>() == SimState::RUNNING)
                        {
                            App::GetSimController()->GetBeamFactory()->UnmuteAllActors();
                            App::sim_state->SetActiveVal((int)App::sim_state_requested->GetActiveEnum<SimState>());
                        }
                    }
                }
            }

            // Prepare for simulation update
            if (App::app_state->GetActiveEnum<AppState>() == AppState::SIMULATION)
            {
                App::GetSimController()->GetBeamFactory()->SyncWithSimThread();
            }

#ifdef USE_SOCKETW
            // Process incoming network traffic
            if (App::mp_state->GetActiveEnum<MpState>() == MpState::CONNECTED)
            {
                std::vector<RoR::NetRecvPacket> packets = App::GetNetwork()->GetIncomingStreamData();
                if (!packets.empty())
                {
                    RoR::ChatSystem::HandleStreamData(packets);
                    if (App::app_state->GetActiveEnum<AppState>() == AppState::SIMULATION && App::GetSimController())
                    {
                        App::GetSimController()->GetBeamFactory()->HandleActorStreamData(packets);
                        App::GetSimController()->GetCharacterFactory()->handleStreamData(packets); // Update characters last (or else beam coupling might fail)
                    }
                }
            }
#endif // USE_SOCKETW

            // Process game events
            App::GetInputEngine()->Capture();
            if (App::app_state->GetActiveEnum<AppState>() == AppState::MAIN_MENU)
            {
                App::GetInputEngine()->updateKeyBounces(dt_sec);

                // Savegame shortcuts
                int slot = -1;
                if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_01, 1.0f))
                {
                    slot = 1;
                }
                if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_02, 1.0f))
                {
                    slot = 2;
                }
                if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_03, 1.0f))
                {
                    slot = 3;
                }
                if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_04, 1.0f))
                {
                    slot = 4;
                }
                if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_05, 1.0f))
                {
                    slot = 5;
                }
                if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_06, 1.0f))
                {
                    slot = 6;
                }
                if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_07, 1.0f))
                {
                    slot = 7;
                }
                if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_08, 1.0f))
                {
                    slot = 8;
                }
                if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_09, 1.0f))
                {
                    slot = 9;
                }
                if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_10, 1.0f))
                {
                    slot = 0;
                }
                if (slot != -1)
                {
                    Ogre::String filename = Ogre::StringUtil::format("quicksave-%d.sav", slot);
                    App::sim_savegame->SetPendingStr(filename);
                    App::app_state_requested->SetActiveVal((int)AppState::SIMULATION);
                }

                // Handle escape key
                if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUIT_GAME))
                {
                    if (App::GetGuiManager()->IsVisible_GameAbout())
                    {
                        App::GetGuiManager()->SetVisible_GameAbout(false);
                    }
                    else if (App::GetGuiManager()->IsVisible_MainSelector())
                    {
                        App::GetGuiManager()->GetMainSelector()->Close();
                    }
                    else if (App::GetGuiManager()->IsVisible_GameSettings())
                    {
                        App::GetGuiManager()->SetVisible_GameSettings(false);
                    }
                    else if (App::GetGuiManager()->IsVisible_MultiplayerSelector())
                    {
                        App::GetGuiManager()->SetVisible_MultiplayerSelector(false);
                    }
                    else
                    {
                        App::app_state_requested->SetActiveVal((int)AppState::SHUTDOWN);
                    }
                }

                // Draw gui
                App::GetGuiManager()->NewImGuiFrame(dt_sec);
                App::GetGuiManager()->DrawMainMenuGui();
            }
            else if (App::app_state->GetActiveEnum<AppState>() == AppState::SIMULATION &&
                     App::app_state_requested->GetActiveEnum<AppState>() == AppState::SIMULATION)
            {
                // Update simulation - inputs, gui, gameplay
                if (dt_sec != 0.f)
                {
                    App::GetGuiManager()->NewImGuiFrame(dt_sec); // For historical reasons, simulation update also draws some GUI
                    App::GetSimController()->UpdateSimulation(dt_sec);
                    if (App::sim_state->GetActiveEnum<SimState>() != RoR::SimState::PAUSED)
                    {
                        App::GetGfxScene()->UpdateScene(dt_sec);
                    }
                }
            }

#ifdef USE_ANGELSCRIPT
            App::GetScriptEngine()->framestep(dt_sec);
#endif

            // Render!
            Ogre::RenderWindow* render_window = RoR::App::GetOgreSubsystem()->GetRenderWindow();
            if (render_window->isClosed())
            {
                App::app_state_requested->SetActiveVal((int)AppState::SHUTDOWN);
            }
            else
            {
                App::GetOgreSubsystem()->GetOgreRoot()->renderOneFrame();
                if (!render_window->isActive() && render_window->isVisible())
                {
                    render_window->update(); // update even when in background !
                }
            } // Render block

            // Update cache if requested
            if (App::app_state->GetActiveEnum<AppState>() == AppState::MAIN_MENU)
            {
                if (App::app_force_cache_udpate->GetActiveVal<bool>() || App::app_force_cache_purge->GetActiveVal<bool>())
                {
                    App::GetGuiManager()->SetVisible_GameSettings(false);
                    App::GetGuiManager()->SetMouseCursorVisibility(GUIManager::MouseCursorVisibility::HIDDEN);
                    App::GetContentManager()->InitModCache();
                    App::GetGuiManager()->SetVisible_GameMainMenu(true);
                }
            } // Update cache block

            // Clean up after previous game state
            if (App::app_state_requested->GetActiveEnum<AppState>() != App::app_state->GetActiveEnum<AppState>())
            {
                if (App::app_state->GetActiveEnum<AppState>() == AppState::SIMULATION)
                {
#ifdef USE_MUMBLE
                    if (App::GetMumble())
                    {
                        App::GetMumble()->SetNonPositionalAudio();
                    }
#endif // USE_MUMBLE
                    if (App::GetSimController())
                    {
                        App::GetSimController()->GetBeamFactory()->SaveScene("autosave.sav");
                        App::GetSimController()->GetBeamFactory()->SyncWithSimThread(); // Wait for background tasks to finish
                        App::GetSimController()->CleanupAfterSimulation();
                        App::sim_state->SetActiveVal((int)SimState::OFF);
                        delete App::GetSimController();
                        App::SetSimController(nullptr);
                    }

                    App::GetGfxScene()->ClearScene();
                    App::sim_terrain_name->SetActiveStr("");
                    App::sim_terrain_gui_name->SetActiveStr("");
                }
            } // Clean up after previous game state block

            App::GetGuiManager()->ApplyGuiCaptureKeyboard();

        } // End of main rendering/input loop

        // --------------------------------------------------------------
        // Shutdown
        // --------------------------------------------------------------

        ShutdownDiscord();

        App::GetConsole()->SaveConfig(); // RoR.cfg

#ifdef USE_SOCKETW
        if (App::mp_state->GetActiveEnum<MpState>() == MpState::CONNECTED)
        {
            App::GetNetwork()->Disconnect();
        }
#endif // USE_SOCKETW

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
