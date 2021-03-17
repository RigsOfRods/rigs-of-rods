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
#include "CameraManager.h"
#include "ChatSystem.h"
#include "Collisions.h"
#include "Console.h"
#include "ContentManager.h"
#include "DiscordRpc.h"
#include "ErrorUtils.h"
#include "GameContext.h"
#include "GfxScene.h"
#include "GUIManager.h"
#include "GUI_DirectionArrow.h"
#include "GUI_FrictionSettings.h"
#include "GUI_LoadingWindow.h"
#include "GUI_MainSelector.h"
#include "GUI_MultiplayerSelector.h"
#include "GUI_SimActorStats.h"
#include "InputEngine.h"
#include "Language.h"
#include "MumbleIntegration.h"
#include "OutGauge.h"
#include "OverlayWrapper.h"
#include "PlatformUtils.h"
#include "RoRVersion.h"
#include "ScriptEngine.h"
#include "Skidmark.h"
#include "SoundScriptManager.h"
#include "TerrainManager.h"
#include "Utils.h"
#include <Overlay/OgreOverlaySystem.h>
#include <ctime>
#include <iomanip>
#include <string>
#include <fstream>

#include <fmt/format.h>

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

        // Process command line params - updates 'cli_*' cvars
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

        App::GetContentManager()->InitContentManager();

        // Set up rendering
        App::CreateGfxScene(); // Creates OGRE SceneManager, needs content manager
        App::GetGfxScene()->GetSceneManager()->addRenderQueueListener(overlay_system);
        App::CreateCameraManager(); // Creates OGRE Camera
        App::GetGfxScene()->GetEnvMap().SetupEnvMap(); // Needs camera

        App::CreateGuiManager(); // Needs scene manager

        App::GetDiscordRpc()->Init();

#ifdef USE_ANGELSCRIPT
        App::CreateScriptEngine();
#endif

        App::GetAppContext()->SetUpInput();

        App::GetGuiManager()->SetUpMenuWallpaper();

        // Add "this is obsolete" marker file to old config location
        App::GetAppContext()->SetUpObsoleteConfMarker();

        App::CreateThreadPool();

        // Load inertia config file
        App::GetGameContext()->GetActorManager()->GetInertiaConfig().LoadDefaultInertiaModels();

        // Load mod cache
        if (App::app_force_cache_purge->GetBool())
        {
            App::GetGameContext()->PushMessage(Message(MSG_APP_MODCACHE_PURGE_REQUESTED));
        }
        else if (App::cli_force_cache_update->GetBool() || App::app_force_cache_update->GetBool())
        {
            App::GetGameContext()->PushMessage(Message(MSG_APP_MODCACHE_UPDATE_REQUESTED));
        }
        else
        {
            App::GetGameContext()->PushMessage(Message(MSG_APP_MODCACHE_LOAD_REQUESTED));
        }

        // Handle game state presets
        if (App::cli_server_host->GetStr() != "" && App::cli_server_port->GetInt() != 0) // Multiplayer, commandline
        {
            App::mp_server_host->SetStr(App::cli_server_host->GetStr());
            App::mp_server_port->SetVal(App::cli_server_port->GetInt());
            App::GetGameContext()->PushMessage(Message(MSG_NET_CONNECT_REQUESTED));
        }
        else if (App::mp_join_on_startup->GetBool()) // Multiplayer, conf file
        {
            App::GetGameContext()->PushMessage(Message(MSG_NET_CONNECT_REQUESTED));
        }
        else // Single player
        {
            if (App::cli_preset_terrain->GetStr() != "") // Terrain, commandline
            {
                App::GetGameContext()->PushMessage(Message(MSG_SIM_LOAD_TERRN_REQUESTED, App::cli_preset_terrain->GetStr()));
            }
            else if (App::diag_preset_terrain->GetStr() != "") // Terrain, conf file
            {
                App::GetGameContext()->PushMessage(Message(MSG_SIM_LOAD_TERRN_REQUESTED, App::diag_preset_terrain->GetStr()));
            }
            else // Main menu
            {
                if (App::cli_resume_autosave->GetBool())
                {
                    if (FileExists(PathCombine(App::sys_savegames_dir->GetStr(), "autosave.sav")))
                    {
                        App::GetGameContext()->PushMessage(RoR::Message(MSG_SIM_LOAD_SAVEGAME_REQUESTED, "autosave.sav"));
                    }
                }
                else if (App::app_skip_main_menu->GetBool())
                {
                    // MainMenu disabled (singleplayer mode) -> go directly to map selector (traditional behavior)
                    RoR::Message m(MSG_GUI_OPEN_SELECTOR_REQUESTED);
                    m.payload = reinterpret_cast<void*>(new LoaderType(LT_Terrain));
                    App::GetGameContext()->PushMessage(m);
                }
                else
                {
                    App::GetGameContext()->PushMessage(Message(MSG_GUI_OPEN_MENU_REQUESTED));
                }
            }
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

        // Hack to properly init DearIMGUI integration - force rendering image
        //  Will be properly fixed under OGRE 2x
        App::GetGuiManager()->GetLoadingWindow()->SetProgress(100, "Hack", /*renderFrame=*/true);
        App::GetGuiManager()->SetVisible_LoadingWindow(false);

        // --------------------------------------------------------------
        // Main rendering and event handling loop
        // --------------------------------------------------------------

        auto start_time = std::chrono::high_resolution_clock::now();

        while (App::app_state->GetEnum<AppState>() != AppState::SHUTDOWN)
        {
            OgreBites::WindowEventUtilities::messagePump();

            // Halt physics (wait for async tasks to finish)
            App::GetGameContext()->GetActorManager()->SyncWithSimThread();

            // Game events
            while (App::GetGameContext()->HasMessages())
            {
                Message m = App::GetGameContext()->PopMessage();
                bool failed_m = false;
                switch (m.type)
                {

                // -- Application events --

                case MSG_APP_SHUTDOWN_REQUESTED:
                    if (App::app_state->GetEnum<AppState>() == AppState::SIMULATION)
                    {
                        App::GetGameContext()->SaveScene("autosave.sav");
                    }
                    App::GetConsole()->SaveConfig(); // RoR.cfg
                    App::GetDiscordRpc()->Shutdown();
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

                case MSG_APP_DISPLAY_FULLSCREEN_REQUESTED:
                    App::GetAppContext()->ActivateFullscreen(true);
                    App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                                                  _L("Display mode changed to fullscreen"));
                    break;

                case MSG_APP_DISPLAY_WINDOWED_REQUESTED:
                    App::GetAppContext()->ActivateFullscreen(false);
                    App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                                                  _L("Display mode changed to windowed"));
                    break;

                case MSG_APP_MODCACHE_LOAD_REQUESTED:
                    if (!App::GetCacheSystem()) // If not already loaded...
                    {
                        App::GetGuiManager()->SetMouseCursorVisibility(GUIManager::MouseCursorVisibility::HIDDEN);
                        App::GetContentManager()->InitModCache(CacheSystem::CacheValidityState::CACHE_STATE_UNKNOWN);
                    }
                    break;

                case MSG_APP_MODCACHE_UPDATE_REQUESTED:
                    if (App::app_state->GetEnum<AppState>() == AppState::MAIN_MENU) // No actors must be spawned; they keep pointers to CacheEntries
                    {
                        RoR::Log("[RoR|ModCache] Cache update requested");
                        App::GetGuiManager()->SetMouseCursorVisibility(GUIManager::MouseCursorVisibility::HIDDEN);
                        App::GetContentManager()->InitModCache(CacheSystem::CacheValidityState::CACHE_NEEDS_UPDATE);
                    }
                    break;

                case MSG_APP_MODCACHE_PURGE_REQUESTED:
                    if (App::app_state->GetEnum<AppState>() == AppState::MAIN_MENU) // No actors must be spawned; they keep pointers to CacheEntries
                    {
                        RoR::Log("[RoR|ModCache] Cache rebuild requested");
                        App::GetGuiManager()->SetMouseCursorVisibility(GUIManager::MouseCursorVisibility::HIDDEN);
                        App::GetContentManager()->InitModCache(CacheSystem::CacheValidityState::CACHE_NEEDS_REBUILD);
                    }
                    break;

                // -- Network events --

                case MSG_NET_CONNECT_REQUESTED:
#if USE_SOCKETW
                    App::GetNetwork()->StartConnecting();
#endif
                    break;

                case MSG_NET_DISCONNECT_REQUESTED:
#if USE_SOCKETW
                    if (App::mp_state->GetEnum<MpState>() == MpState::CONNECTED)
                    {
                        App::GetNetwork()->Disconnect();
                        if (App::app_state->GetEnum<AppState>() == AppState::MAIN_MENU)
                        {
                            App::GetGuiManager()->GetMainSelector()->Close(); // We may get disconnected while still in map selection
                            App::GetGameContext()->PushMessage(Message(MSG_GUI_OPEN_MENU_REQUESTED));
                        }
                    }
#endif // USE_SOCKETW
                    break;

                case MSG_NET_SERVER_KICK:
                    App::GetGameContext()->PushMessage(Message(MSG_NET_DISCONNECT_REQUESTED));
                    App::GetGameContext()->PushMessage(Message(MSG_SIM_UNLOAD_TERRN_REQUESTED));
                    App::GetGameContext()->PushMessage(Message(MSG_GUI_OPEN_MENU_REQUESTED));
                    App::GetGuiManager()->ShowMessageBox(
                        _LC("Network", "Network disconnected"), m.description.c_str());
                    break;

                case MSG_NET_RECV_ERROR:
                    App::GetGameContext()->PushMessage(Message(MSG_NET_DISCONNECT_REQUESTED));
                    App::GetGameContext()->PushMessage(Message(MSG_SIM_UNLOAD_TERRN_REQUESTED));
                    App::GetGameContext()->PushMessage(Message(MSG_GUI_OPEN_MENU_REQUESTED));
                    App::GetGuiManager()->ShowMessageBox(
                        _L("Network fatal error: "), m.description.c_str());
                    break;

                case MSG_NET_CONNECT_STARTED:
                    App::GetGuiManager()->GetLoadingWindow()->SetProgressNetConnect(m.description);
                    App::GetGuiManager()->SetVisible_MultiplayerSelector(false);
                    App::GetGameContext()->PushMessage(Message(MSG_GUI_CLOSE_MENU_REQUESTED));
                    break;

                case MSG_NET_CONNECT_PROGRESS:
                    App::GetGuiManager()->GetLoadingWindow()->SetProgressNetConnect(m.description);
                    break;

                case MSG_NET_CONNECT_SUCCESS:
#if USE_SOCKETW
                    App::GetGuiManager()->GetLoadingWindow()->SetVisible(false);
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
                            RoR::Message m(MSG_GUI_OPEN_SELECTOR_REQUESTED);
                            m.payload = reinterpret_cast<void*>(new LoaderType(LT_Terrain));
                            App::GetGameContext()->PushMessage(m);
                        }
                        else
                        {
                            App::GetGameContext()->PushMessage(Message(MSG_SIM_LOAD_TERRN_REQUESTED, App::diag_preset_terrain->GetStr()));
                        }
                    }
#endif // USE_SOCKETW
                    break;

                case MSG_NET_CONNECT_FAILURE:
#if USE_SOCKETW
                    App::GetGuiManager()->GetLoadingWindow()->SetVisible(false);
                    App::GetNetwork()->StopConnecting();
                    App::GetGameContext()->PushMessage(Message(MSG_NET_DISCONNECT_REQUESTED));
                    App::GetGameContext()->PushMessage(Message(MSG_GUI_OPEN_MENU_REQUESTED));
                    App::GetGuiManager()->ShowMessageBox(
                        _LC("Network", "Multiplayer: connection failed"), m.description.c_str());
                    App::GetGuiManager()->ReflectGameState();
#endif // USE_SOCKETW
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
                    for (Actor* actor: App::GetGameContext()->GetActorManager()->GetActors())
                    {
                        actor->StopAllSounds();
                    }
                    App::sim_state->SetVal((int)SimState::PAUSED);
                    break;

                case MSG_SIM_UNPAUSE_REQUESTED:
                    for (Actor* actor: App::GetGameContext()->GetActorManager()->GetActors())
                    {
                        actor->UnmuteAllSounds();
                    }
                    App::sim_state->SetVal((int)SimState::RUNNING);
                    break;

                case MSG_SIM_LOAD_TERRN_REQUESTED:
                    App::GetGuiManager()->GetLoadingWindow()->SetProgress(5, _L("Loading resources"));
                    App::GetContentManager()->LoadGameplayResources();

                    if (App::GetGameContext()->LoadTerrain(m.description))
                    {
                        App::GetGameContext()->CreatePlayerCharacter();
                        // Spawn preselected vehicle; commandline has precedence
                        if (App::cli_preset_vehicle->GetStr() != "")
                            App::GetGameContext()->SpawnPreselectedActor(App::cli_preset_vehicle->GetStr(), App::cli_preset_veh_config->GetStr()); // Needs character for position
                        else if (App::diag_preset_vehicle->GetStr() != "")
                            App::GetGameContext()->SpawnPreselectedActor(App::diag_preset_vehicle->GetStr(), App::diag_preset_veh_config->GetStr()); // Needs character for position
                        App::GetGameContext()->GetSceneMouse().InitializeVisuals();
                        App::CreateOverlayWrapper();
                        App::GetGuiManager()->GetDirectionArrow()->LoadOverlay();
                        if (App::audio_menu_music->GetBool())
                        {
                            SOUND_KILL(-1, SS_TRIG_MAIN_MENU);
                        }
                        App::GetGfxScene()->GetSceneManager()->setAmbientLight(Ogre::ColourValue(0.3f, 0.3f, 0.3f));
                        App::GetDiscordRpc()->UpdatePresence();
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
                        if (App::io_outgauge_mode->GetInt() > 0)
                        {
                            App::GetOutGauge()->Connect();
                        }
                    }
                    else
                    {
                        if (App::mp_state->GetEnum<MpState>() == MpState::CONNECTED)
                        {
                            App::GetGameContext()->PushMessage(Message(MSG_NET_DISCONNECT_REQUESTED));
                        }
                        else
                        {
                            App::GetGameContext()->PushMessage(Message(MSG_GUI_OPEN_MENU_REQUESTED));
                        }
                        App::GetGuiManager()->SetVisible_LoadingWindow(false);
                        failed_m = true;
                    }
                    break;

                case MSG_SIM_UNLOAD_TERRN_REQUESTED:
                    if (App::sim_state->GetEnum<SimState>() == SimState::EDITOR_MODE)
                    {
                        App::GetSimTerrain()->GetTerrainEditor()->WriteOutputFile();
                    }
                    App::GetGameContext()->SaveScene("autosave.sav");
                    App::GetGameContext()->ChangePlayerActor(nullptr);
                    App::GetGameContext()->GetActorManager()->CleanUpSimulation();
                    App::GetGameContext()->GetCharacterFactory()->DeleteAllCharacters();
                    App::GetGameContext()->GetSceneMouse().DiscardVisuals();
                    App::DestroyOverlayWrapper();
                    App::GetCameraManager()->ResetAllBehaviors();
                    App::GetGuiManager()->SetVisible_LoadingWindow(false);
                    App::sim_state->SetVal((int)SimState::OFF);
                    App::app_state->SetVal((int)AppState::MAIN_MENU);
                    App::GetGuiManager()->ReflectGameState();
                    delete App::GetSimTerrain();
                    App::SetSimTerrain(nullptr);
                    App::GetGfxScene()->ClearScene();
                    App::sim_terrain_name->SetStr("");
                    App::sim_terrain_gui_name->SetStr("");
                    App::GetOutGauge()->Close();
                    break;

                case MSG_SIM_LOAD_SAVEGAME_REQUESTED:
                    {
                        std::string terrn_filename = App::GetGameContext()->ExtractSceneTerrain(m.description);
                        if (terrn_filename == "")
                        {
                            Str<400> msg; msg << _L("Could not read savegame file") << "'" << m.description << "'";
                            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, msg.ToCStr());
                            App::GetGameContext()->PushMessage(Message(MSG_GUI_OPEN_MENU_REQUESTED));
                        }
                        else if (terrn_filename == App::sim_terrain_name->GetStr())
                        {
                            App::GetGameContext()->LoadScene(m.description);
                        }
                        else if (terrn_filename != App::sim_terrain_name->GetStr() && App::mp_state->GetEnum<MpState>() == MpState::CONNECTED)
                        {
                            Str<400> msg; msg << _L("Not loading different terrain while in Multiplayer");
                            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING, msg.ToCStr());
                        }
                        else
                        {
                            if (App::sim_terrain_name->GetStr() != "")
                            {
                                App::GetGameContext()->PushMessage(Message(MSG_SIM_UNLOAD_TERRN_REQUESTED));
                            }

                            RoR::LogFormat("[RoR|Savegame] Loading terrain '%s' ...", terrn_filename.c_str());
                            App::GetGameContext()->PushMessage(Message(MSG_SIM_LOAD_TERRN_REQUESTED, terrn_filename));
                            // Loading terrain may produce actor-spawn requests; the savegame-request must be posted after them.
                            App::GetGameContext()->ChainMessage(Message(MSG_SIM_LOAD_SAVEGAME_REQUESTED, m.description));
                        }
                    }
                    break;

                case MSG_SIM_SPAWN_ACTOR_REQUESTED:
                    if (App::app_state->GetEnum<AppState>() == AppState::SIMULATION)
                    {
                        ActorSpawnRequest* rq = (ActorSpawnRequest*)m.payload;
                        App::GetGameContext()->SpawnActor(*rq);
                        delete rq;
                    }
                    break;

                case MSG_SIM_MODIFY_ACTOR_REQUESTED:
                    if (App::app_state->GetEnum<AppState>() == AppState::SIMULATION)
                    {
                        ActorModifyRequest* rq = (ActorModifyRequest*)m.payload;
                        App::GetGameContext()->ModifyActor(*rq);
                        delete rq;
                    }
                    break;

                case MSG_SIM_DELETE_ACTOR_REQUESTED:
                    if (App::app_state->GetEnum<AppState>() == AppState::SIMULATION)
                    {
                        App::GetGameContext()->DeleteActor((Actor*)m.payload);
                    }
                    break;

                case MSG_SIM_SEAT_PLAYER_REQUESTED:
                    if (App::app_state->GetEnum<AppState>() == AppState::SIMULATION)
                    {
                        App::GetGameContext()->ChangePlayerActor((Actor*)m.payload);
                    }
                    break;

                case MSG_SIM_TELEPORT_PLAYER_REQUESTED:
                    if (App::app_state->GetEnum<AppState>() == AppState::SIMULATION)
                    {
                        Ogre::Vector3* pos = (Ogre::Vector3*)m.payload;
                        App::GetGameContext()->TeleportPlayer(pos->x, pos->z);
                        delete pos;
                    }
                    break;

                // -- GUI events ---

                case MSG_GUI_OPEN_MENU_REQUESTED:
                    if (App::app_state->GetEnum<AppState>() == AppState::MAIN_MENU)
                    {
                        App::GetGuiManager()->SetVisible_GameMainMenu(true);
                    }
                    break;

                case MSG_GUI_CLOSE_MENU_REQUESTED:
                    App::GetGuiManager()->SetVisible_GameMainMenu(false);
                    break;

                case MSG_GUI_OPEN_SELECTOR_REQUESTED:
                    App::GetGuiManager()->GetMainSelector()->Show(*reinterpret_cast<LoaderType*>(m.payload), m.description);
                    delete reinterpret_cast<LoaderType*>(m.payload);
                    break;

                case MSG_GUI_CLOSE_SELECTOR_REQUESTED:
                    App::GetGuiManager()->GetMainSelector()->Close();
                    break;

                // -- Editing events --

                case MSG_EDI_MODIFY_GROUNDMODEL_REQUESTED:
                    {
                        ground_model_t* modified_gm = (ground_model_t*)m.payload;
                        ground_model_t* live_gm = App::GetSimTerrain()->GetCollisions()->getGroundModelByString(modified_gm->name);
                        *live_gm = *modified_gm; // Copy over
                    }
                    break;

                case MSG_EDI_ENTER_TERRN_EDITOR_REQUESTED:
                    if (App::sim_state->GetEnum<SimState>() != SimState::EDITOR_MODE)
                    {
                        App::sim_state->SetVal((int)SimState::EDITOR_MODE);
                        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                                                      _L("Entered terrain editing mode"));
                    }
                    break;

                case MSG_EDI_LEAVE_TERRN_EDITOR_REQUESTED:
                    if (App::sim_state->GetEnum<SimState>() == SimState::EDITOR_MODE)
                    {
                        App::GetSimTerrain()->GetTerrainEditor()->WriteOutputFile();
                        App::GetSimTerrain()->GetTerrainEditor()->ClearSelection();
                        App::sim_state->SetVal((int)SimState::RUNNING);
                        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                                                      _L("Left terrain editing mode"));
                    }
                    break;

                case MSG_EDI_RELOAD_BUNDLE_REQUESTED:
                    {
                        // To reload the bundle, it's resource group must be destroyed and re-created. All actors using it must be deleted.
                        CacheEntry* entry = reinterpret_cast<CacheEntry*>(m.payload);
                        bool all_clear = true;
                        for (Actor* actor: App::GetGameContext()->GetActorManager()->GetActors())
                        {
                            if (actor->GetGfxActor()->GetResourceGroup() == entry->resource_group)
                            {
                                App::GetGameContext()->PushMessage(Message(MSG_SIM_DELETE_ACTOR_REQUESTED, actor));
                                all_clear = false;
                            }
                        }

                        if (all_clear)
                        {
                            // Nobody uses the RG anymore -> destroy and re-create it.
                            App::GetCacheSystem()->ReLoadResource(*entry);
                        }
                        else
                        {
                            // Re-post the same message again so that it's message chain is executed later.
                            App::GetGameContext()->PushMessage(m);
                            failed_m = true;
                        }
                    }
                    break;

                case MSG_EDI_EXPORT_TRUCK_REQUESTED:
                    {
                        ROR_ASSERT(m.payload);
                        CacheEntry* entry = reinterpret_cast<CacheEntry*>(m.payload);
                        ROR_ASSERT(entry->actor_def);
                        if (entry->resource_bundle_type == "FileSystem")
                        {
                            // Export the truck file
                            std::string filename = m.description;
                            if (filename == "")
                            {
                                filename = fmt::format("export_{}", entry->fname);
                            }
                            App::GetGameContext()->GetActorManager()->ExportTruckDocument(
                                entry->actor_def, filename, entry->resource_group);

                            // Update cache - also deletes all spawned actors
                            App::GetGameContext()->PushMessage(
                                Message(MSG_EDI_RELOAD_BUNDLE_REQUESTED, (void*)entry));

                            // Load the new actor, but only after the bundle is fully reloaded (chain messages)
                            RoR::ActorSpawnRequest* request = new ActorSpawnRequest();
                            request->asr_filename = filename;
                            request->asr_origin = ActorSpawnRequest::Origin::USER;
                            App::GetGameContext()->ChainMessage(
                                Message(MSG_SIM_SPAWN_ACTOR_REQUESTED, (void*)request));
                        }
                        else
                        {
                            App::GetConsole()->putMessage(
                                Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_ERROR,
                                _LC("Truck", "Export not supported for ZIP bundles"));
                        }
                    }
                    break;

                default:;
                }

                // Process chained messages
                if (!failed_m)
                {
                    for (Message& chained_msg: m.chain)
                    {
                        App::GetGameContext()->PushMessage(chained_msg);
                    }
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
            const float dt = std::chrono::duration<float>(now - start_time).count();
            start_time = now;

#ifdef USE_SOCKETW
            // Process incoming network traffic
            if (App::mp_state->GetEnum<MpState>() == MpState::CONNECTED)
            {
                std::vector<RoR::NetRecvPacket> packets = App::GetNetwork()->GetIncomingStreamData();
                if (!packets.empty())
                {
                    RoR::ChatSystem::HandleStreamData(packets);
                    if (App::app_state->GetEnum<AppState>() == AppState::SIMULATION)
                    {
                        App::GetGameContext()->GetActorManager()->HandleActorStreamData(packets);
                        App::GetGameContext()->GetCharacterFactory()->handleStreamData(packets); // Update characters last (or else beam coupling might fail)
                    }
                }
            }
#endif // USE_SOCKETW

            // Process input events
            if (dt != 0.f)
            {
                App::GetInputEngine()->Capture();
                App::GetInputEngine()->updateKeyBounces(dt);

                App::GetGameContext()->HandleSavegameHotkeys();
                App::GetGameContext()->UpdateGlobalInputEvents();
                App::GetGuiManager()->UpdateInputEvents(dt);

                if (App::app_state->GetEnum<AppState>() == AppState::SIMULATION)
                {
                    App::GetCameraManager()->UpdateInputEvents(dt);
                    App::GetOverlayWrapper()->update(dt);
                    if (App::sim_state->GetEnum<SimState>() == SimState::EDITOR_MODE)
                    {
                        App::GetGameContext()->UpdateSimInputEvents(dt);
                        App::GetGameContext()->UpdateSkyInputEvents(dt);
                        App::GetSimTerrain()->GetTerrainEditor()->UpdateInputEvents(dt);
                    }
                    else if (App::sim_state->GetEnum<SimState>() == SimState::RUNNING)
                    {
                        App::GetGameContext()->GetCharacterFactory()->Update(dt);
                        if (App::GetCameraManager()->GetCurrentBehavior() != CameraManager::CAMERA_BEHAVIOR_FREE)
                        {
                            App::GetGameContext()->UpdateSimInputEvents(dt);
                            App::GetGameContext()->UpdateSkyInputEvents(dt);
                            if (App::GetGameContext()->GetPlayerActor() &&
                                App::GetGameContext()->GetPlayerActor()->ar_sim_state != Actor::SimState::NETWORKED_OK) // we are in a vehicle
                            {
                                App::GetGameContext()->UpdateCommonInputEvents(dt);
                                if (App::GetGameContext()->GetPlayerActor()->ar_sim_state != Actor::SimState::LOCAL_REPLAY)
                                {
                                    if (App::GetGameContext()->GetPlayerActor()->ar_driveable == TRUCK)
                                    {
                                        App::GetGameContext()->UpdateTruckInputEvents(dt);
                                    }
                                    if (App::GetGameContext()->GetPlayerActor()->ar_driveable == AIRPLANE)
                                    {
                                        App::GetGameContext()->UpdateAirplaneInputEvents(dt);
                                    }
                                    if (App::GetGameContext()->GetPlayerActor()->ar_driveable == BOAT)
                                    {
                                        App::GetGameContext()->UpdateBoatInputEvents(dt);
                                    }
                                }
                            }
                        }
                        else // free cam mode
                        {
                            App::GetGameContext()->UpdateSkyInputEvents(dt);
                        }
                    }
                    App::GetGameContext()->GetRecoveryMode().UpdateInputEvents(dt);
                    App::GetGameContext()->GetActorManager()->UpdateInputEvents(dt);
                }
            }

            // Update OutGauge device
            if (App::io_outgauge_mode->GetInt() > 0)
            {
                App::GetOutGauge()->Update(dt, App::GetGameContext()->GetPlayerActor());
            }

            // Early GUI updates which require halted physics
            App::GetGuiManager()->NewImGuiFrame(dt);
            if (App::app_state->GetEnum<AppState>() == AppState::SIMULATION)
            {
                App::GetGuiManager()->DrawSimulationGui(dt);
                for (auto actor : App::GetGameContext()->GetActorManager()->GetActors())
                {
                    actor->GetGfxActor()->UpdateDebugView();
                }
                if (App::GetGameContext()->GetPlayerActor())
                {
                    App::GetGuiManager()->GetSimActorStats()->UpdateStats(dt, App::GetGameContext()->GetPlayerActor());
                    if (App::GetGuiManager()->IsVisible_FrictionSettings())
                    {
                        App::GetGuiManager()->GetFrictionSettings()->setActiveCol(App::GetGameContext()->GetPlayerActor()->ar_last_fuzzy_ground_model);
                    }
                }
            }

#ifdef USE_MUMBLE
            if (App::GetMumble())
            {
                App::GetMumble()->Update(); // 3d voice over network
            }
#endif // USE_MUMBLE

#ifdef USE_OPENAL
            App::GetSoundScriptManager()->update(dt); // update 3d audio listener position
#endif // USE_OPENAL

#ifdef USE_ANGELSCRIPT
            if (App::app_state->GetEnum<AppState>() == AppState::SIMULATION)
            {
                App::GetScriptEngine()->framestep(dt);
            }
#endif // USE_ANGELSCRIPT

            if (App::io_ffb_enabled->GetBool() &&
                App::sim_state->GetEnum<SimState>() == SimState::RUNNING)
            {
                App::GetAppContext()->GetForceFeedback().Update();
            }

            if (App::sim_state->GetEnum<SimState>() == SimState::RUNNING)
            {
                App::GetGameContext()->GetSceneMouse().UpdateSimulation();
            }

            // Create snapshot of simulation state for Gfx/GUI updates
            if (App::sim_state->GetEnum<SimState>() == SimState::RUNNING)
            {
                App::GetGfxScene()->BufferSimulationData();
            }

            // Advance simulation
            if (App::sim_state->GetEnum<SimState>() == SimState::RUNNING)
            {
                App::GetGameContext()->UpdateActors(); // *** Start new physics tasks. No reading from Actor N/B beyond this point.
            }

            // Scene and GUI updates
            if (App::app_state->GetEnum<AppState>() == AppState::MAIN_MENU)
            {
                App::GetGuiManager()->DrawMainMenuGui();
            }
            else if (App::app_state->GetEnum<AppState>() == AppState::SIMULATION)
            {
                App::GetGfxScene()->UpdateScene(dt); // Draws GUI as well
            }

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
