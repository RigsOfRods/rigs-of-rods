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
#include "GameContext.h"
#include "GUIManager.h"
#include "GUI_LoadingWindow.h"
#include "GUI_MainSelector.h"
#include "GUI_MultiplayerSelector.h"
#include "InputEngine.h"
#include "Language.h"
#include "MumbleIntegration.h"
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

        // Create cvars, set default values
        App::GetConsole()->CVarSetupBuiltins();

        // Update cvars 'sys_process_dir', 'sys_user_dir'
        if (!App::GetAppContext()->SetUpProgramPaths())
        {
            return -1; // Error already displayed
        }

        // Create OGRE default logger early
        App::GetAppContext()->SetUpLogging();

        // User directories
        App::sys_config_dir    ->SetStr(PathCombine(App::sys_user_dir->GetStr(), "config"));
        App::sys_cache_dir     ->SetStr(PathCombine(App::sys_user_dir->GetStr(), "cache"));
        App::sys_savegames_dir ->SetStr(PathCombine(App::sys_user_dir->GetStr(), "savegames"));
        App::sys_screenshot_dir->SetStr(PathCombine(App::sys_user_dir->GetStr(), "screenshots"));

        // Load RoR.cfg - updates cvars
        App::GetConsole()->LoadConfig();

        // Process command line params - updates cvars
        App::GetConsole()->ProcessCommandLine(argc, argv);

        if (App::app_state->GetEnum<AppState>() == AppState::PRINT_HELP_EXIT)
        {
            App::GetConsole()->ShowCommandLineUsage();
            return 0;
        }
        if (App::app_state->GetEnum<AppState>() == AppState::PRINT_VERSION_EXIT)
        {
            App::GetConsole()->ShowCommandLineVersion();
            return 0;
        }

        // Find resources dir, update cvar 'sys_resources_dir'
        if (!App::GetAppContext()->SetUpResourcesDir())
        {
            return -1; // Error already displayed
        }

        // Make sure config directory exists - to save 'ogre.cfg'
        CreateFolder(App::sys_config_dir->GetStr());

        // Load and start OGRE renderer, uses config directory
        if (!App::GetAppContext()->SetUpRendering())
        {
            return -1; // Error already displayed
        }

        Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);

        // Deploy base config files from 'skeleton.zip'
        if (!App::GetAppContext()->SetUpConfigSkeleton())
        {
            return -1; // Error already displayed
        }

        Ogre::OverlaySystem* overlay_system = new Ogre::OverlaySystem(); //Overlay init

        Ogre::ConfigOptionMap ropts = App::GetAppContext()->GetOgreRoot()->getRenderSystem()->getConfigOptions();
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

        if (!App::diag_warning_texture->GetBool())
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

        App::GetAppContext()->SetUpInput();

        App::GetGuiManager()->SetUpMenuWallpaper();

        App::GetContentManager()->InitModCache();

        RoR::ForceFeedback force_feedback;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        if (App::io_ffb_enabled->GetBool()) // Force feedback
        {
            if (App::GetInputEngine()->getForceFeedbackDevice())
            {
                force_feedback.Setup();
            }
            else
            {
                LOG("No force feedback device detected, disabling force feedback");
                App::io_ffb_enabled->SetVal(false);
            }
        }
#endif // OGRE_PLATFORM_WIN32

        // Add "this is obsolete" marker file to old config location
        App::GetAppContext()->SetUpObsoleteConfMarker();

        SkidmarkConfig skidmark_conf; // Loads 'skidmark.cfg' in constructor

        App::CreateThreadPool();

        if (App::mp_join_on_startup->GetBool())
        {
            App::GetGameContext()->PushMessage(Message(MSG_NET_CONNECT_REQUESTED));
        }
        else if (App::diag_preset_terrain->GetStr() != "")
        {
            App::GetGameContext()->PushMessage(Message(MSG_SIM_LOAD_TERRN_REQUESTED, App::diag_preset_terrain->GetStr()));
        }
        else if (!App::mp_join_on_startup->GetBool() && App::app_skip_main_menu->GetBool())
        {
            // MainMenu disabled (singleplayer mode) -> go directly to map selector (traditional behavior)
            App::GetGuiManager()->SetVisible_GameMainMenu(false);
            App::GetGuiManager()->GetMainSelector()->Show(LT_Terrain);
        }

        App::app_state->SetVal((int)AppState::MAIN_MENU);
        App::GetGuiManager()->ReflectGameState();

#ifdef USE_OPENAL
        if (App::audio_menu_music->GetBool())
        {
            App::GetSoundScriptManager()->createInstance("tracks/main_menu_tune", -1, nullptr);
            SOUND_START(-1, SS_TRIG_MAIN_MENU);
        }
#endif // USE_OPENAL

        // --------------------------------------------------------------
        // Main rendering and event handling loop
        // --------------------------------------------------------------

        auto start_time = std::chrono::high_resolution_clock::now();

        while (App::app_state->GetEnum<AppState>() != AppState::SHUTDOWN)
        {
            OgreBites::WindowEventUtilities::messagePump();

            // Game events
            while (App::GetGameContext()->HasMessages())
            {
                Message m = App::GetGameContext()->PopMessage();
                switch (m.type)
                {

                // -- Application events --

                case MSG_APP_SHUTDOWN_REQUESTED:
                    App::GetConsole()->SaveConfig(); // RoR.cfg
                    ShutdownDiscord();
#ifdef USE_SOCKETW
                    if (App::mp_state->GetEnum<MpState>() == MpState::CONNECTED)
                    {
                        App::GetNetwork()->Disconnect();
                    }
#endif // USE_SOCKETW
                    App::app_state->SetVal((int)AppState::SHUTDOWN);
                    break;

                case MSG_APP_SCREENSHOT_REQUESTED:
                    App::GetGuiManager()->SetMouseCursorVisibility(GUIManager::MouseCursorVisibility::HIDDEN);
                    App::GetAppContext()->CaptureScreenshot();
                    App::GetGuiManager()->SetMouseCursorVisibility(GUIManager::MouseCursorVisibility::VISIBLE);
                    break;

                // -- Network events --

                case MSG_NET_CONNECT_REQUESTED:
                    App::GetNetwork()->StartConnecting();
                    break;

                case MSG_NET_DISCONNECT_REQUESTED:
                    App::GetNetwork()->Disconnect();
                    if (App::app_state->GetEnum<AppState>() == AppState::MAIN_MENU)
                    {
                        App::GetGuiManager()->GetMainSelector()->Close(); // We may get disconnected while still in map selection
                        App::GetGuiManager()->SetVisible_GameMainMenu(true);
                    }
                    break;

                case MSG_NET_SERVER_KICK:
                    App::GetGameContext()->PushMessage(Message(MSG_NET_DISCONNECT_REQUESTED));
                    App::GetGameContext()->PushMessage(Message(MSG_SIM_UNLOAD_TERRN_REQUESTED));
                    App::GetGuiManager()->ShowMessageBox(
                        _LC("Network", "Network disconnected"), m.description.c_str());
                    break;

                case MSG_NET_RECV_ERROR:
                    App::GetGameContext()->PushMessage(Message(MSG_NET_DISCONNECT_REQUESTED));
                    App::GetGameContext()->PushMessage(Message(MSG_SIM_UNLOAD_TERRN_REQUESTED));
                    App::GetGuiManager()->ShowMessageBox(
                        _L("Network fatal error: "), m.description.c_str());
                    break;

                case MSG_NET_CONNECT_STARTED:
                    App::GetGuiManager()->SetMpConnectingStatusMsg(m.description.c_str());
                    App::GetGuiManager()->SetVisible_GameMainMenu(false);
                    App::GetGuiManager()->SetVisible_MultiplayerSelector(false);
                    break;

                case MSG_NET_CONNECT_PROGRESS:
                    App::GetGuiManager()->SetMpConnectingStatusMsg(m.description.c_str());
                    break;

                case MSG_NET_CONNECT_SUCCESS:
                    App::GetNetwork()->StopConnecting();
                    App::mp_state->SetVal((int)RoR::MpState::CONNECTED);
                    RoR::ChatSystem::SendStreamSetup();
                    if (!App::GetMumble())
                    {
                        App::CreateMumble();
                    }
                    if (App::GetNetwork()->GetTerrainName() != "any")
                    {
                        App::GetGameContext()->PushMessage(Message(MSG_SIM_LOAD_TERRN_REQUESTED, App::GetNetwork()->GetTerrainName()));
                    }
                    else
                    {
                        // Connected -> go directly to map selector
                        if (App::diag_preset_terrain->GetStr().empty())
                        {
                            App::GetGuiManager()->GetMainSelector()->Show(LT_Terrain);
                        }
                        else
                        {
                            App::GetGameContext()->PushMessage(Message(MSG_SIM_LOAD_TERRN_REQUESTED, App::diag_preset_terrain->GetStr()));
                        }
                    }
                    break;

                case MSG_NET_CONNECT_FAILURE:
                    App::GetNetwork()->StopConnecting();
                    App::GetGameContext()->PushMessage(Message(MSG_NET_DISCONNECT_REQUESTED));
                    App::GetGuiManager()->ShowMessageBox(
                        _LC("Network", "Multiplayer: connection failed"), m.description.c_str());
                    App::GetGuiManager()->ReflectGameState();
                    break;

                case MSG_NET_REFRESH_SERVERLIST_SUCCESS:
                    App::GetGuiManager()->GetMpSelector()->UpdateServerlist((GUI::MpServerInfoVec*)m.payload);
                    delete (GUI::MpServerInfoVec*)m.payload;
                    break;

                case MSG_NET_REFRESH_SERVERLIST_FAILURE:
                    App::GetGuiManager()->GetMpSelector()->DisplayRefreshFailed(m.description);
                    break;

                // -- Gameplay events --

                case MSG_SIM_PAUSE_REQUESTED:
                    App::GetSimController()->GetBeamFactory()->MuteAllActors();
                    App::sim_state->SetVal((int)SimState::PAUSED);
                    break;

                case MSG_SIM_UNPAUSE_REQUESTED:
                    App::GetSimController()->GetBeamFactory()->UnmuteAllActors();
                    App::sim_state->SetVal((int)SimState::RUNNING);
                    break;

                case MSG_SIM_LOAD_TERRN_REQUESTED:
                    App::SetSimController(new SimController(&force_feedback, &skidmark_conf));
                    App::GetGuiManager()->GetLoadingWindow()->setProgress(5, _L("Loading resources"));
                    App::GetContentManager()->LoadGameplayResources();

                    if (App::GetSimController()->LoadTerrain(m.description) &&
                        App::GetSimController()->SetupGameplayLoop())
                    {
                        App::sim_state->SetVal((int)SimState::RUNNING);
                        App::app_state->SetVal((int)AppState::SIMULATION);
                        App::GetGuiManager()->ReflectGameState();
                        App::GetGuiManager()->SetVisible_LoadingWindow(false);
                        App::gfx_fov_external->SetVal(App::gfx_fov_external_default->GetInt());
                        App::gfx_fov_internal->SetVal(App::gfx_fov_internal_default->GetInt());
#ifdef USE_SOCKETW
                        if (App::mp_state->GetEnum<MpState>() == MpState::CONNECTED)
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
                        App::GetGameContext()->PushMessage(Message(MSG_SIM_UNLOAD_TERRN_REQUESTED));
                    }
                    break;

                case MSG_SIM_UNLOAD_TERRN_REQUESTED:
#ifdef USE_MUMBLE
                    if (App::GetMumble())
                    {
                        App::GetMumble()->SetNonPositionalAudio();
                    }
#endif // USE_MUMBLE
                    ROR_ASSERT (App::GetSimController());                    
                    App::GetSimController()->GetBeamFactory()->SaveScene("autosave.sav");
                    App::GetSimController()->GetBeamFactory()->SyncWithSimThread(); // Wait for background tasks to finish
                    App::GetSimController()->CleanupAfterSimulation();
                    App::sim_state->SetVal((int)SimState::OFF);
                    App::app_state->SetVal((int)AppState::MAIN_MENU);
                    App::GetGuiManager()->ReflectGameState();
                    delete App::GetSimController();
                    App::SetSimController(nullptr);
                    App::GetGfxScene()->ClearScene();
                    App::sim_terrain_name->SetStr("");
                    App::sim_terrain_gui_name->SetStr("");
                    break;

                case MSG_SIM_LOAD_SAVEGAME_REQUESTED:
                    {
                        std::string terrn_filename = ActorManager::ExtractTerrainFilename(m.description);
                        if (terrn_filename == "")
                        {
                            LogFormat("[RoR|Savegame] Could not load '%s'", m.description.c_str());
                            App::GetGuiManager()->SetVisible_GameMainMenu(true);
                        }
                        else if (terrn_filename == App::sim_terrain_name->GetStr())
                        {
                            App::GetSimController()->GetBeamFactory()->LoadScene(m.description);
                        }
                        else
                        {
                            RoR::LogFormat("[RoR|Savegame] Loading terrain '%s' ...", terrn_filename.c_str());
                            App::GetGameContext()->PushMessage(Message(MSG_SIM_LOAD_TERRN_REQUESTED, terrn_filename));
                            App::GetGameContext()->PushMessage(Message(MSG_SIM_LOAD_SAVEGAME_REQUESTED, m.description));
                        }
                    }
                    break;

                default:;
                }
            } // Game events block


            // Check FPS limit
            if (App::gfx_fps_limit->GetInt() > 0)
            {
                const float min_frame_time = 1.0f / Ogre::Math::Clamp(App::gfx_fps_limit->GetInt(), 5, 240);
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

            // Prepare for simulation update
            if (App::app_state->GetEnum<AppState>() == AppState::SIMULATION)
            {
                App::GetSimController()->GetBeamFactory()->SyncWithSimThread();
            }

#ifdef USE_SOCKETW
            // Process incoming network traffic
            if (App::mp_state->GetEnum<MpState>() == MpState::CONNECTED)
            {
                std::vector<RoR::NetRecvPacket> packets = App::GetNetwork()->GetIncomingStreamData();
                if (!packets.empty())
                {
                    RoR::ChatSystem::HandleStreamData(packets);
                    if (App::app_state->GetEnum<AppState>() == AppState::SIMULATION && App::GetSimController())
                    {
                        App::GetSimController()->GetBeamFactory()->HandleActorStreamData(packets);
                        App::GetSimController()->GetCharacterFactory()->handleStreamData(packets); // Update characters last (or else beam coupling might fail)
                    }
                }
            }
#endif // USE_SOCKETW

            // Process game events
            App::GetInputEngine()->Capture();
            if (App::app_state->GetEnum<AppState>() == AppState::MAIN_MENU)
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
                    App::GetGameContext()->PushMessage(Message(MSG_SIM_LOAD_SAVEGAME_REQUESTED, filename));
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
                        App::GetGameContext()->PushMessage(Message(MSG_APP_SHUTDOWN_REQUESTED));
                    }
                }

                // Draw gui
                App::GetGuiManager()->NewImGuiFrame(dt_sec);
                App::GetGuiManager()->DrawMainMenuGui();
            }
            else if (App::app_state->GetEnum<AppState>() == AppState::SIMULATION)
            {
                // Update simulation - inputs, gui, gameplay
                if (dt_sec != 0.f)
                {
                    App::GetGuiManager()->NewImGuiFrame(dt_sec); // For historical reasons, simulation update also draws some GUI
                    App::GetSimController()->UpdateSimulation(dt_sec);
                    if (App::sim_state->GetEnum<SimState>() != RoR::SimState::PAUSED)
                    {
                        App::GetGfxScene()->UpdateScene(dt_sec);
                    }
                }
            }

#ifdef USE_ANGELSCRIPT
            App::GetScriptEngine()->framestep(dt_sec);
#endif

            // Render!
            Ogre::RenderWindow* render_window = RoR::App::GetAppContext()->GetRenderWindow();
            if (render_window->isClosed())
            {
                App::GetGameContext()->PushMessage(Message(MSG_APP_SHUTDOWN_REQUESTED));
            }
            else
            {
                App::GetAppContext()->GetOgreRoot()->renderOneFrame();
                if (!render_window->isActive() && render_window->isVisible())
                {
                    render_window->update(); // update even when in background !
                }
            } // Render block

            // Update cache if requested
            if (App::app_state->GetEnum<AppState>() == AppState::MAIN_MENU)
            {
                if (App::app_force_cache_udpate->GetBool() || App::app_force_cache_purge->GetBool())
                {
                    App::GetGuiManager()->SetVisible_GameSettings(false);
                    App::GetGuiManager()->SetMouseCursorVisibility(GUIManager::MouseCursorVisibility::HIDDEN);
                    App::GetContentManager()->InitModCache();
                    App::GetGuiManager()->SetVisible_GameMainMenu(true);
                }
            } // Update cache block

            App::GetGuiManager()->ApplyGuiCaptureKeyboard();

        } // End of main rendering/input loop

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
