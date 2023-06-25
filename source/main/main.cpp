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

#include "Actor.h"
#include "AppContext.h"
#include "Application.h"
#include "CacheSystem.h"
#include "CameraManager.h"
#include "ChatSystem.h"
#include "Collisions.h"
#include "Console.h"
#include "ContentManager.h"
#include "DiscordRpc.h"
#include "ErrorUtils.h"
#include "GUI_DirectionArrow.h"
#include "GUI_FrictionSettings.h"
#include "GUI_GameControls.h"
#include "GUI_LoadingWindow.h"
#include "GUI_MainSelector.h"
#include "GUI_MessageBox.h"
#include "GUI_MultiplayerClientList.h"
#include "GUI_MultiplayerSelector.h"
#include "GUI_RepositorySelector.h"
#include "GUI_SimActorStats.h"
#include "GUI_SurveyMap.h"
#include "GUIManager.h"
#include "GUIUtils.h"
#include "GameContext.h"
#include "GfxScene.h"
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
#include "Terrain.h"
#include "Utils.h"
#include <Overlay/OgreOverlaySystem.h>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <string>

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
        App::GetConsole()->cVarSetupBuiltins();

        // Record main thread ID for checks
        App::GetAppContext()->SetUpThreads();

        // Update cvars 'sys_process_dir', 'sys_user_dir'
        if (!App::GetAppContext()->SetUpProgramPaths())
        {
            return -1; // Error already displayed
        }

        // Create OGRE default logger early
        App::GetAppContext()->SetUpLogging();

        // User directories
        App::sys_config_dir    ->setStr(PathCombine(App::sys_user_dir->getStr(), "config"));
        App::sys_cache_dir     ->setStr(PathCombine(App::sys_user_dir->getStr(), "cache"));
        App::sys_thumbnails_dir->setStr(PathCombine(App::sys_user_dir->getStr(), "thumbnails"));
        App::sys_savegames_dir ->setStr(PathCombine(App::sys_user_dir->getStr(), "savegames"));
        App::sys_screenshot_dir->setStr(PathCombine(App::sys_user_dir->getStr(), "screenshots"));

        // Load RoR.cfg - updates cvars
        App::GetConsole()->loadConfig();

        // Process command line params - updates 'cli_*' cvars
        App::GetConsole()->processCommandLine(argc, argv);

        if (App::app_state->getEnum<AppState>() == AppState::PRINT_HELP_EXIT)
        {
            App::GetConsole()->showCommandLineUsage();
            return 0;
        }
        if (App::app_state->getEnum<AppState>() == AppState::PRINT_VERSION_EXIT)
        {
            App::GetConsole()->showCommandLineVersion();
            return 0;
        }

        // Find resources dir, update cvar 'sys_resources_dir'
        if (!App::GetAppContext()->SetUpResourcesDir())
        {
            return -1; // Error already displayed
        }
#ifdef USE_REMOTERY
        Remotery* rmt;
        rmt_CreateGlobalInstance(&rmt);
#endif
        // Make sure config directory exists - to save 'ogre.cfg'
        CreateFolder(App::sys_config_dir->getStr());

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

        if (!App::diag_warning_texture->getBool())
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
        App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::SCRIPTS);

#ifndef NOLANG
        App::GetLanguageEngine()->setup();
#endif // NOLANG
        App::GetConsole()->regBuiltinCommands(); // Call after localization had been set up

        App::GetContentManager()->InitContentManager();

        // Set up rendering
        App::CreateGfxScene(); // Creates OGRE SceneManager, needs content manager
        App::GetGfxScene()->GetSceneManager()->addRenderQueueListener(overlay_system);
        App::CreateCameraManager(); // Creates OGRE Camera
        App::GetGfxScene()->GetEnvMap().SetupEnvMap(); // Needs camera

        App::CreateGuiManager(); // Needs scene manager

        App::GetDiscordRpc()->Init();

        App::GetAppContext()->SetUpInput();

#ifdef USE_ANGELSCRIPT
        App::CreateScriptEngine();
#endif

        App::GetGuiManager()->SetUpMenuWallpaper();

        // Add "this is obsolete" marker file to old config location
        App::GetAppContext()->SetUpObsoleteConfMarker();

        App::CreateThreadPool();

        // Load inertia config file
        App::GetGameContext()->GetActorManager()->GetInertiaConfig().LoadDefaultInertiaModels();

        // Load mod cache
        if (App::app_force_cache_purge->getBool())
        {
            App::GetGameContext()->PushMessage(Message(MSG_APP_MODCACHE_PURGE_REQUESTED));
        }
        else if (App::cli_force_cache_update->getBool() || App::app_force_cache_update->getBool())
        {
            App::GetGameContext()->PushMessage(Message(MSG_APP_MODCACHE_UPDATE_REQUESTED));
        }
        else
        {
            App::GetGameContext()->PushMessage(Message(MSG_APP_MODCACHE_LOAD_REQUESTED));
        }

        // Load startup scripts (console, then RoR.cfg)
        if (App::cli_custom_scripts->getStr() != "")
        {
            Ogre::StringVector script_names = Ogre::StringUtil::split(App::cli_custom_scripts->getStr(), ",");
            for (Ogre::String const& scriptname: script_names)
            {
                LOG(fmt::format("Loading startup script '{}' (from command line)", scriptname));
                App::GetScriptEngine()->loadScript(scriptname, ScriptCategory::CUSTOM);
                // errors are logged by OGRE & AngelScript
            }
        }
        if (App::app_custom_scripts->getStr() != "")
        {
            Ogre::StringVector script_names = Ogre::StringUtil::split(App::app_custom_scripts->getStr(), ",");
            for (Ogre::String const& scriptname: script_names)
            {
                LOG(fmt::format("Loading startup script '{}' (from config file)", scriptname));
                App::GetScriptEngine()->loadScript(scriptname, ScriptCategory::CUSTOM);
                // errors are logged by OGRE & AngelScript
            }
        }

        // Handle game state presets
        if (App::cli_server_host->getStr() != "" && App::cli_server_port->getInt() != 0) // Multiplayer, commandline
        {
            App::mp_server_host->setStr(App::cli_server_host->getStr());
            App::mp_server_port->setVal(App::cli_server_port->getInt());
            App::GetGameContext()->PushMessage(Message(MSG_NET_CONNECT_REQUESTED));
        }
        else if (App::mp_join_on_startup->getBool()) // Multiplayer, conf file
        {
            App::GetGameContext()->PushMessage(Message(MSG_NET_CONNECT_REQUESTED));
        }
        else // Single player
        {
            if (App::cli_preset_terrain->getStr() != "") // Terrain, commandline
            {
                App::GetGameContext()->PushMessage(Message(MSG_SIM_LOAD_TERRN_REQUESTED, App::cli_preset_terrain->getStr()));
            }
            else if (App::diag_preset_terrain->getStr() != "") // Terrain, conf file
            {
                App::GetGameContext()->PushMessage(Message(MSG_SIM_LOAD_TERRN_REQUESTED, App::diag_preset_terrain->getStr()));
            }
            else // Main menu
            {
                if (App::cli_resume_autosave->getBool())
                {
                    if (FileExists(PathCombine(App::sys_savegames_dir->getStr(), "autosave.sav")))
                    {
                        App::GetGameContext()->PushMessage(RoR::Message(MSG_SIM_LOAD_SAVEGAME_REQUESTED, "autosave.sav"));
                    }
                }
                else if (App::app_skip_main_menu->getBool())
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

        App::app_state->setVal((int)AppState::MAIN_MENU);
        App::GetGuiManager()->MenuWallpaper->show();

#ifdef USE_OPENAL
        if (App::audio_menu_music->getBool())
        {
            App::GetSoundScriptManager()->createInstance("tracks/main_menu_tune", -1);
            SOUND_START(-1, SS_TRIG_MAIN_MENU);
        }
#endif // USE_OPENAL

        // Hack to properly init DearIMGUI integration - force rendering image
        //  Will be properly fixed under OGRE 2x
        App::GetGuiManager()->LoadingWindow.SetProgress(100, "Hack", /*renderFrame=*/true);
        App::GetGuiManager()->LoadingWindow.SetVisible(false);

        // --------------------------------------------------------------
        // Main rendering and event handling loop
        // --------------------------------------------------------------

        auto start_time = std::chrono::high_resolution_clock::now();

        while (App::app_state->getEnum<AppState>() != AppState::SHUTDOWN)
        {
            rmt_ScopedCPUSample(MainLoop, 0);

            rmt_BeginCPUSample(MainLoop_window_messages, 0);
            OgreBites::WindowEventUtilities::messagePump();
            rmt_EndCPUSample(MainLoop_window_messages);

            rmt_BeginCPUSample(MainLoop_sync_with_simthread, 0);
            // Halt physics (wait for async tasks to finish)
            if (App::app_state->getEnum<AppState>() == AppState::SIMULATION)
            {
                App::GetGameContext()->GetActorManager()->SyncWithSimThread();
            }
            rmt_EndCPUSample(MainLoop_sync_with_simthread);

            // Game events
            rmt_BeginCPUSample(MainLoop_message_queue, 0);
            while (App::GetGameContext()->HasMessages())
            {
                Message m = App::GetGameContext()->PopMessage();
                bool failed_m = false;
                switch (m.type)
                {

                // -- Application events --

                case MSG_APP_SHUTDOWN_REQUESTED:
                    if (App::app_state->getEnum<AppState>() == AppState::SIMULATION)
                    {
                        App::GetGameContext()->SaveScene("autosave.sav");
                    }
                    App::GetConsole()->saveConfig(); // RoR.cfg
                    App::GetDiscordRpc()->Shutdown();
#ifdef USE_SOCKETW
                    if (App::mp_state->getEnum<MpState>() == MpState::CONNECTED)
                    {
                        App::GetNetwork()->Disconnect();
                    }
#endif // USE_SOCKETW
                    App::app_state->setVal((int)AppState::SHUTDOWN);
                    App::GetScriptEngine()->setEventsEnabled(false); // Hack to enable fast shutdown without cleanup.
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
                        App::GetContentManager()->InitModCache(CacheValidity::UNKNOWN);
                    }
                    break;

                case MSG_APP_MODCACHE_UPDATE_REQUESTED:
                    if (App::app_state->getEnum<AppState>() == AppState::MAIN_MENU) // No actors must be spawned; they keep pointers to CacheEntries
                    {
                        RoR::Log("[RoR|ModCache] Cache update requested");
                        App::GetGuiManager()->SetMouseCursorVisibility(GUIManager::MouseCursorVisibility::HIDDEN);
                        App::GetContentManager()->InitModCache(CacheValidity::NEEDS_UPDATE);
                    }
                    break;

                case MSG_APP_MODCACHE_PURGE_REQUESTED:
                    if (App::app_state->getEnum<AppState>() == AppState::MAIN_MENU) // No actors must be spawned; they keep pointers to CacheEntries
                    {
                        RoR::Log("[RoR|ModCache] Cache rebuild requested");
                        App::GetGuiManager()->SetMouseCursorVisibility(GUIManager::MouseCursorVisibility::HIDDEN);
                        App::GetContentManager()->InitModCache(CacheValidity::NEEDS_REBUILD);
                    }
                    break;

                case MSG_APP_LOAD_SCRIPT_REQUESTED:
                {
                    LoadScriptRequest* request = static_cast<LoadScriptRequest*>(m.payload);
                    ActorPtr actor = App::GetGameContext()->GetActorManager()->GetActorById(request->lsr_associated_actor);
                    App::GetScriptEngine()->loadScript(request->lsr_filename, request->lsr_category, actor);
                    delete request;
                    break;
                }

                case MSG_APP_UNLOAD_SCRIPT_REQUESTED:
                {
                    ScriptUnitId_t* id = static_cast<ScriptUnitId_t*>(m.payload);
                    App::GetScriptEngine()->unloadScript(*id);
                    delete id;
                    break;
                }

                // -- Network events --

                case MSG_NET_CONNECT_REQUESTED:
#if USE_SOCKETW
                    App::GetNetwork()->StartConnecting();
#endif
                    break;

                case MSG_NET_DISCONNECT_REQUESTED:
#if USE_SOCKETW
                    if (App::mp_state->getEnum<MpState>() == MpState::CONNECTED)
                    {
                        App::GetNetwork()->Disconnect();
                        if (App::app_state->getEnum<AppState>() == AppState::MAIN_MENU)
                        {
                            App::GetGuiManager()->MainSelector.Close(); // We may get disconnected while still in map selection
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
                    App::GetGuiManager()->LoadingWindow.SetProgressNetConnect(m.description);
                    App::GetGuiManager()->MultiplayerSelector.SetVisible(false);
                    App::GetGameContext()->PushMessage(Message(MSG_GUI_CLOSE_MENU_REQUESTED));
                    break;

                case MSG_NET_CONNECT_PROGRESS:
                    App::GetGuiManager()->LoadingWindow.SetProgressNetConnect(m.description);
                    break;

                case MSG_NET_CONNECT_SUCCESS:
#if USE_SOCKETW
                    App::GetGuiManager()->LoadingWindow.SetVisible(false);
                    App::GetNetwork()->StopConnecting();
                    App::mp_state->setVal((int)RoR::MpState::CONNECTED);
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
                        if (App::diag_preset_terrain->getStr().empty())
                        {
                            RoR::Message m(MSG_GUI_OPEN_SELECTOR_REQUESTED);
                            m.payload = reinterpret_cast<void*>(new LoaderType(LT_Terrain));
                            App::GetGameContext()->PushMessage(m);
                        }
                        else
                        {
                            App::GetGameContext()->PushMessage(Message(MSG_SIM_LOAD_TERRN_REQUESTED, App::diag_preset_terrain->getStr()));
                        }
                    }
#endif // USE_SOCKETW
                    break;

                case MSG_NET_CONNECT_FAILURE:
#if USE_SOCKETW
                    App::GetGuiManager()->LoadingWindow.SetVisible(false);
                    App::GetNetwork()->StopConnecting();
                    App::GetGameContext()->PushMessage(Message(MSG_NET_DISCONNECT_REQUESTED));
                    App::GetGameContext()->PushMessage(Message(MSG_GUI_OPEN_MENU_REQUESTED));
                    App::GetGuiManager()->ShowMessageBox(
                        _LC("Network", "Multiplayer: connection failed"), m.description.c_str());
#endif // USE_SOCKETW
                    break;

                case MSG_NET_REFRESH_SERVERLIST_SUCCESS:
                {
                    GUI::MpServerInfoVec* data = static_cast<GUI::MpServerInfoVec*>(m.payload);
                    App::GetGuiManager()->MultiplayerSelector.UpdateServerlist(data);
                    delete data;
                    break;
                }

                case MSG_NET_REFRESH_SERVERLIST_FAILURE:
                {
                    CurlFailInfo* failinfo = static_cast<CurlFailInfo*>(m.payload);
                    App::GetGuiManager()->MultiplayerSelector.DisplayRefreshFailed(failinfo);
                    delete failinfo;
                    break;
                }

                case MSG_NET_REFRESH_REPOLIST_SUCCESS:
                {
                    GUI::ResourcesCollection* data = static_cast<GUI::ResourcesCollection*>(m.payload);
                    App::GetGuiManager()->RepositorySelector.UpdateResources(data);
                    delete data;
                    break;
                }

                case MSG_NET_OPEN_RESOURCE_SUCCESS:
                {
                    GUI::ResourcesCollection* data = static_cast<GUI::ResourcesCollection*>(m.payload);
                    App::GetGuiManager()->RepositorySelector.UpdateFiles(data);
                    delete data;
                    break;
                }

                case MSG_NET_REFRESH_REPOLIST_FAILURE:
                {
                    CurlFailInfo* failinfo = static_cast<CurlFailInfo*>(m.payload);
                    App::GetGuiManager()->RepositorySelector.ShowError(failinfo);
                    delete failinfo;
                    break;
                }

                case MSG_NET_REFRESH_AI_PRESETS:
                    App::GetGuiManager()->TopMenubar.Refresh(m.description);
                    break;

                // -- Gameplay events --

                case MSG_SIM_PAUSE_REQUESTED:
                    for (ActorPtr& actor: App::GetGameContext()->GetActorManager()->GetActors())
                    {
                        actor->muteAllSounds();
                    }
                    App::sim_state->setVal((int)SimState::PAUSED);
                    break;

                case MSG_SIM_UNPAUSE_REQUESTED:
                    for (ActorPtr& actor: App::GetGameContext()->GetActorManager()->GetActors())
                    {
                        actor->unmuteAllSounds();
                    }
                    App::sim_state->setVal((int)SimState::RUNNING);
                    break;

                case MSG_SIM_LOAD_TERRN_REQUESTED:
                    App::GetGuiManager()->SetMouseCursorVisibility(GUIManager::MouseCursorVisibility::HIDDEN);
                    App::GetGuiManager()->LoadingWindow.SetProgress(5, _L("Loading resources"));
                    App::GetContentManager()->LoadGameplayResources();

                    if (App::GetGameContext()->LoadTerrain(m.description))
                    {
                        App::GetGameContext()->CreatePlayerCharacter();
                        // Spawn preselected vehicle; commandline has precedence
                        if (App::cli_preset_vehicle->getStr() != "")
                            App::GetGameContext()->SpawnPreselectedActor(App::cli_preset_vehicle->getStr(), App::cli_preset_veh_config->getStr()); // Needs character for position
                        else if (App::diag_preset_vehicle->getStr() != "")
                            App::GetGameContext()->SpawnPreselectedActor(App::diag_preset_vehicle->getStr(), App::diag_preset_veh_config->getStr()); // Needs character for position
                        App::GetGameContext()->GetSceneMouse().InitializeVisuals();
                        App::CreateOverlayWrapper();
                        App::GetGuiManager()->DirectionArrow.LoadOverlay();
                        if (App::audio_menu_music->getBool())
                        {
                            SOUND_KILL(-1, SS_TRIG_MAIN_MENU);
                        }
                        App::GetGfxScene()->GetSceneManager()->setAmbientLight(Ogre::ColourValue(0.3f, 0.3f, 0.3f));
                        App::GetDiscordRpc()->UpdatePresence();
                        App::sim_state->setVal((int)SimState::RUNNING);
                        App::app_state->setVal((int)AppState::SIMULATION);
                        App::GetGuiManager()->GameMainMenu .SetVisible(false);
                        App::GetGuiManager()->MenuWallpaper->hide();
                        App::GetGuiManager()->LoadingWindow.SetVisible(false);
                        App::GetGuiManager()->SetMouseCursorVisibility(GUIManager::MouseCursorVisibility::VISIBLE);
                        App::gfx_fov_external->setVal(App::gfx_fov_external_default->getInt());
                        App::gfx_fov_internal->setVal(App::gfx_fov_internal_default->getInt());
#ifdef USE_SOCKETW
                        if (App::mp_state->getEnum<MpState>() == MpState::CONNECTED)
                        {
                            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                                                                  fmt::format(_LC("ChatBox", "Press {} to start chatting"),
                                               App::GetInputEngine()->getEventCommandTrimmed(EV_COMMON_ENTER_CHATMODE)), "lightbulb.png");
                        }
#endif // USE_SOCKETW
                        if (App::io_outgauge_mode->getInt() > 0)
                        {
                            App::GetOutGauge()->Connect();
                        }
                    }
                    else
                    {
                        if (App::mp_state->getEnum<MpState>() == MpState::CONNECTED)
                        {
                            App::GetGameContext()->PushMessage(Message(MSG_NET_DISCONNECT_REQUESTED));
                        }
                        else
                        {
                            App::GetGameContext()->PushMessage(Message(MSG_GUI_OPEN_MENU_REQUESTED));
                        }
                        App::GetGuiManager()->LoadingWindow.SetVisible(false);
                        failed_m = true;
                    }
                    break;

                case MSG_SIM_UNLOAD_TERRN_REQUESTED:
                    if (App::sim_state->getEnum<SimState>() == SimState::EDITOR_MODE)
                    {
                        App::GetGameContext()->GetTerrain()->GetTerrainEditor()->WriteOutputFile();
                    }
                    App::GetGameContext()->SaveScene("autosave.sav");
                    App::GetGameContext()->ChangePlayerActor(nullptr);
                    App::GetGameContext()->GetActorManager()->CleanUpSimulation();
                    App::GetGameContext()->GetCharacterFactory()->DeleteAllCharacters();
                    App::GetGameContext()->GetSceneMouse().DiscardVisuals();
                    App::DestroyOverlayWrapper();
                    App::GetCameraManager()->ResetAllBehaviors();
                    App::GetGuiManager()->CollisionsDebug.CleanUp();
                    App::GetGuiManager()->MainSelector.Close();
                    App::GetGuiManager()->LoadingWindow.SetVisible(false);
                    App::GetGuiManager()->MenuWallpaper->show();
                    App::GetGuiManager()->SurveyMap.ai_waypoints.clear();
                    App::sim_state->setVal((int)SimState::OFF);
                    App::app_state->setVal((int)AppState::MAIN_MENU);
                    App::GetGameContext()->UnloadTerrain();
                    App::GetGfxScene()->ClearScene();
                    App::sim_terrain_name->setStr("");
                    App::sim_terrain_gui_name->setStr("");
                    App::GetOutGauge()->Close();
                    App::GetSoundScriptManager()->setCamera(/*position:*/Ogre::Vector3::ZERO, /*direction:*/Ogre::Vector3::ZERO, /*up:*/Ogre::Vector3::UNIT_Y, /*velocity:*/Ogre::Vector3::ZERO);
                    break;

                case MSG_SIM_LOAD_SAVEGAME_REQUESTED:
                {
                    std::string terrn_filename = App::GetGameContext()->ExtractSceneTerrain(m.description);
                    if (terrn_filename == "")
                    {
                        Str<400> msg; msg << _L("Could not read savegame file") << "'" << m.description << "'";
                        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, msg.ToCStr());
                        if (App::app_state->getEnum<AppState>() == AppState::MAIN_MENU)
                        {
                            App::GetGameContext()->PushMessage(Message(MSG_GUI_OPEN_MENU_REQUESTED));
                        }
                    }
                    else if (terrn_filename == App::sim_terrain_name->getStr())
                    {
                        App::GetGameContext()->LoadScene(m.description);
                    }
                    else if (terrn_filename != App::sim_terrain_name->getStr() && App::mp_state->getEnum<MpState>() == MpState::CONNECTED)
                    {
                        Str<400> msg; msg << _L("Error while loading scene: Terrain mismatch");
                        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, msg.ToCStr());
                    }
                    else
                    {
                        if (App::sim_terrain_name->getStr() != "")
                        {
                            App::GetGameContext()->PushMessage(Message(MSG_SIM_UNLOAD_TERRN_REQUESTED));
                        }

                        RoR::LogFormat("[RoR|Savegame] Loading terrain '%s' ...", terrn_filename.c_str());
                        App::GetGameContext()->PushMessage(Message(MSG_SIM_LOAD_TERRN_REQUESTED, terrn_filename));
                        // Loading terrain may produce actor-spawn requests; the savegame-request must be posted after them.
                        App::GetGameContext()->ChainMessage(Message(MSG_SIM_LOAD_SAVEGAME_REQUESTED, m.description));
                    }
                    break;
                }

                case MSG_SIM_SPAWN_ACTOR_REQUESTED:
                {
                    ActorSpawnRequest* rq = static_cast<ActorSpawnRequest*>(m.payload);
                    if (App::app_state->getEnum<AppState>() == AppState::SIMULATION)
                    {
                        App::GetGameContext()->SpawnActor(*rq);
                    }
                    delete rq;
                    break;
                }

                case MSG_SIM_MODIFY_ACTOR_REQUESTED:
                {
                    ActorModifyRequest* rq = static_cast<ActorModifyRequest*>(m.payload);
                    if (App::app_state->getEnum<AppState>() == AppState::SIMULATION)
                    {
                        App::GetGameContext()->ModifyActor(*rq);
                    }
                    delete rq;
                    break;
                }

                case MSG_SIM_DELETE_ACTOR_REQUESTED:
                {
                    ActorPtr* actor_ptr = static_cast<ActorPtr*>(m.payload);
                    ROR_ASSERT(actor_ptr);
                    if (App::app_state->getEnum<AppState>() == AppState::SIMULATION)
                    {
                        App::GetGameContext()->DeleteActor(*actor_ptr);
                    }
                    delete actor_ptr;
                    break;
                }

                case MSG_SIM_SEAT_PLAYER_REQUESTED:
                {
                    ActorPtr* actor_ptr = static_cast<ActorPtr*>(m.payload);
                    ROR_ASSERT(actor_ptr); // Even if leaving vehicle, the pointer must be valid.
                    if (App::app_state->getEnum<AppState>() == AppState::SIMULATION)
                    {
                        App::GetGameContext()->ChangePlayerActor(*actor_ptr);
                    }
                    delete actor_ptr;
                    break;
                }

                case MSG_SIM_TELEPORT_PLAYER_REQUESTED:
                {
                    Ogre::Vector3* pos = static_cast<Ogre::Vector3*>(m.payload);
                    if (App::app_state->getEnum<AppState>() == AppState::SIMULATION)
                    {
                        App::GetGameContext()->TeleportPlayer(pos->x, pos->z);
                    }
                    delete pos;
                    break;
                }

                case MSG_SIM_HIDE_NET_ACTOR_REQUESTED:
                {
                    ActorPtr* actor_ptr = static_cast<ActorPtr*>(m.payload);
                    ROR_ASSERT(actor_ptr);
                    if ((App::mp_state->getEnum<MpState>() == MpState::CONNECTED) &&
                        ((*actor_ptr)->ar_state == ActorState::NETWORKED_OK))
                    {
                        ActorPtr actor = *actor_ptr;
                        actor->ar_state = ActorState::NETWORKED_HIDDEN; // Stop net. updates
                        App::GetGfxScene()->RemoveGfxActor(actor->GetGfxActor()); // Remove visuals (also stops updating SimBuffer)
                        actor->GetGfxActor()->GetSimDataBuffer().simbuf_actor_state = ActorState::NETWORKED_HIDDEN; // Hack - manually propagate the new state to SimBuffer so Character can reflect it.
                        actor->GetGfxActor()->SetAllMeshesVisible(false);
                        actor->GetGfxActor()->SetCastShadows(false);
                        actor->muteAllSounds(); // Stop sounds
                        actor->forceAllFlaresOff();
                        actor->setSmokeEnabled(false);
                    }
                    delete actor_ptr;
                    break;
                }

                case MSG_SIM_UNHIDE_NET_ACTOR_REQUESTED:
                {
                    ActorPtr* actor_ptr = static_cast<ActorPtr*>(m.payload);
                    ROR_ASSERT(actor_ptr);
                    if (App::mp_state->getEnum<MpState>() == MpState::CONNECTED &&
                        ((*actor_ptr)->ar_state == ActorState::NETWORKED_HIDDEN))
                    {
                        ActorPtr actor = *actor_ptr;
                        actor->ar_state = ActorState::NETWORKED_OK; // Resume net. updates
                        App::GetGfxScene()->RegisterGfxActor(actor->GetGfxActor()); // Restore visuals (also resumes updating SimBuffer)
                        actor->GetGfxActor()->SetAllMeshesVisible(true);
                        actor->GetGfxActor()->SetCastShadows(true);
                        actor->unmuteAllSounds(); // Unmute sounds
                        actor->setSmokeEnabled(true);
                    }
                    delete actor_ptr;
                    break;
                }

                case MSG_SIM_SCRIPT_EVENT_TRIGGERED:
                {
                    ScriptEventArgs* args = static_cast<ScriptEventArgs*>(m.payload);
                    App::GetScriptEngine()->triggerEvent(args->type, args->arg1, args->arg2ex, args->arg3ex, args->arg4ex, args->arg5ex, args->arg6ex, args->arg7ex, args->arg8ex);
                    delete args;
                    break;
                }

                case MSG_SIM_SCRIPT_CALLBACK_QUEUED:
                {
                    ScriptCallbackArgs* args = static_cast<ScriptCallbackArgs*>(m.payload);
                    App::GetScriptEngine()->envokeCallback(args->eventsource->es_script_handler, args->eventsource, args->node);
                    delete args;
                    break;
                }

                case MSG_SIM_ACTOR_LINKING_REQUESTED:
                {
                    // Estabilishing a physics linkage between 2 actors modifies a global linkage table
                    // and triggers immediate update of every actor's linkage tables,
                    // so it has to be done sequentially on main thread.
                    // ---------------------------------------------------------------------------------
                    ActorLinkingRequest* request = static_cast<ActorLinkingRequest*>(m.payload);
                    ActorPtr actor = App::GetGameContext()->GetActorManager()->GetActorById(request->alr_actor_instance_id);
                    if (actor)
                    {
                        switch (request->alr_type)
                        {
                        case ActorLinkingRequestType::HOOK_ACTION:
                            actor->hookToggle(request->alr_hook_group, request->alr_hook_action, request->alr_hook_mousenode);
                            if (request->alr_hook_action == MOUSE_HOOK_TOGGLE)
                            {
                                TRIGGER_EVENT_ASYNC(SE_TRUCK_MOUSE_GRAB, request->alr_actor_instance_id);
                            }
                            break;

                        case ActorLinkingRequestType::TIE_ACTION:
                            actor->tieToggle(request->alr_tie_group);
                            break;

                        case ActorLinkingRequestType::ROPE_ACTION:
                            actor->ropeToggle(request->alr_rope_group);
                            break;

                        case ActorLinkingRequestType::SLIDENODE_ACTION:
                            actor->toggleSlideNodeLock();
                            break;
                        }
                        
                    }
                    delete request;
                    break;
                }

                // -- GUI events ---

                case MSG_GUI_OPEN_MENU_REQUESTED:
                    App::GetGuiManager()->GameMainMenu.SetVisible(true);
                    break;

                case MSG_GUI_CLOSE_MENU_REQUESTED:
                    App::GetGuiManager()->GameMainMenu.SetVisible(false);
                    break;

                case MSG_GUI_OPEN_SELECTOR_REQUESTED:
                {
                    LoaderType* type = static_cast<LoaderType*>(m.payload);
                    App::GetGuiManager()->MainSelector.Show(*type, m.description);
                    delete type;
                    break;
                }

                case MSG_GUI_CLOSE_SELECTOR_REQUESTED:
                    App::GetGuiManager()->MainSelector.Close();
                    break;

                case MSG_GUI_MP_CLIENTS_REFRESH:
                    App::GetGuiManager()->MpClientList.UpdateClients();
                    break;

                case MSG_GUI_SHOW_MESSAGE_BOX_REQUESTED:
                {
                    GUI::MessageBoxConfig* conf = static_cast<GUI::MessageBoxConfig*>(m.payload);
                    App::GetGuiManager()->ShowMessageBox(*conf);
                    delete conf;
                    break;
                }

                case MSG_GUI_DOWNLOAD_PROGRESS:
                {
                    App::GetGameContext()->PushMessage(Message(MSG_GUI_CLOSE_MENU_REQUESTED));
                    int* percentage = static_cast<int*>(m.payload);
                    App::GetGuiManager()->LoadingWindow.SetProgress(*percentage, m.description, false);
                    delete percentage;
                    break;
                }

                case MSG_GUI_DOWNLOAD_FINISHED:
                    App::GetGuiManager()->LoadingWindow.SetVisible(false);
                    App::GetGuiManager()->RepositorySelector.SetVisible(true);
                    App::GetGuiManager()->RepositorySelector.DownloadFinished();
                    break;

                // -- Editing events --

                case MSG_EDI_MODIFY_GROUNDMODEL_REQUESTED:
                {
                    ground_model_t* modified_gm = static_cast<ground_model_t*>(m.payload);
                    ground_model_t* live_gm = App::GetGameContext()->GetTerrain()->GetCollisions()->getGroundModelByString(modified_gm->name);
                    *live_gm = *modified_gm; // Copy over
                    //DO NOT `delete` the payload - it's a weak pointer, the data are owned by `RoR::Collisions`; See `enum MsgType` in file 'Application.h'.
                    break;
                }

                case MSG_EDI_ENTER_TERRN_EDITOR_REQUESTED:
                    if (App::sim_state->getEnum<SimState>() != SimState::EDITOR_MODE)
                    {
                        App::sim_state->setVal((int)SimState::EDITOR_MODE);
                        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                                                      _L("Entered terrain editing mode"));
                        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                                                      fmt::format(_L("Press {} or middle mouse click to select an object"),
                                   App::GetInputEngine()->getEventCommandTrimmed(EV_COMMON_ENTER_OR_EXIT_TRUCK)), "lightbulb.png");
                    }
                    break;

                case MSG_EDI_LEAVE_TERRN_EDITOR_REQUESTED:
                    if (App::sim_state->getEnum<SimState>() == SimState::EDITOR_MODE)
                    {
                        App::GetGameContext()->GetTerrain()->GetTerrainEditor()->WriteOutputFile();
                        App::GetGameContext()->GetTerrain()->GetTerrainEditor()->ClearSelection();
                        App::sim_state->setVal((int)SimState::RUNNING);
                        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                                                      _L("Left terrain editing mode"));
                    }
                    break;

                case MSG_EDI_RELOAD_BUNDLE_REQUESTED:
                {
                    // To reload the bundle, it's resource group must be destroyed and re-created. All actors using it must be deleted.
                    CacheEntry* entry = static_cast<CacheEntry*>(m.payload);
                    bool all_clear = true;
                    for (ActorPtr& actor: App::GetGameContext()->GetActorManager()->GetActors())
                    {
                        if (actor->GetGfxActor()->GetResourceGroup() == entry->resource_group)
                        {
                            App::GetGameContext()->PushMessage(Message(MSG_SIM_DELETE_ACTOR_REQUESTED, static_cast<void*>(new ActorPtr(actor))));
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
                    //DO NOT `delete` the payload - it's a weak pointer, data are owned by `RoR::CacheSystem`; See `enum MsgType` in file 'Application.h'.
                    break;
                }

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
            rmt_EndCPUSample(MainLoop_message_queue);

            // Check FPS limit
            if (App::gfx_fps_limit->getInt() > 0)
            {
                const float min_frame_time = 1.0f / Ogre::Math::Clamp(App::gfx_fps_limit->getInt(), 5, 240);
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
            if (App::mp_state->getEnum<MpState>() == MpState::CONNECTED)
            {
                std::vector<RoR::NetRecvPacket> packets = App::GetNetwork()->GetIncomingStreamData();

                if (!packets.empty())
                {
                    RoR::ChatSystem::HandleStreamData(packets);
                    if (App::app_state->getEnum<AppState>() == AppState::SIMULATION)
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

                if (!App::GetGuiManager()->GameControls.IsInteractiveKeyBindingActive())
                {
                    if (!App::GetGuiManager()->MainSelector.IsVisible() && !App::GetGuiManager()->MultiplayerSelector.IsVisible() &&
                        !App::GetGuiManager()->GameSettings.IsVisible() && !App::GetGuiManager()->GameControls.IsVisible() &&
                        !App::GetGuiManager()->GameAbout.IsVisible() && !App::GetGuiManager()->RepositorySelector.IsVisible())
                    {
                        App::GetGameContext()->HandleSavegameHotkeys();
                    }
                    App::GetGameContext()->UpdateGlobalInputEvents();
                    App::GetGuiManager()->UpdateInputEvents(dt);

                    if (App::app_state->getEnum<AppState>() == AppState::SIMULATION)
                    {
                        if (App::sim_state->getEnum<SimState>() == SimState::EDITOR_MODE)
                        {
                            App::GetGameContext()->UpdateSkyInputEvents(dt);
                            App::GetGameContext()->GetTerrain()->GetTerrainEditor()->UpdateInputEvents(dt);
                        }
                        else
                        {
                            App::GetGameContext()->GetCharacterFactory()->Update(dt); // Character MUST be updated before CameraManager, otherwise camera position is always 1 frame behind the character position, causing stuttering.
                        }
                        App::GetCameraManager()->UpdateInputEvents(dt);
                        App::GetOverlayWrapper()->update(dt);
                        App::GetGameContext()->GetRepairMode().UpdateInputEvents(dt);
                        App::GetGameContext()->GetActorManager()->UpdateInputEvents(dt);
                        if (App::sim_state->getEnum<SimState>() == SimState::RUNNING)
                        {
                            if (App::GetCameraManager()->GetCurrentBehavior() != CameraManager::CAMERA_BEHAVIOR_FREE)
                            {
                                App::GetGameContext()->UpdateSimInputEvents(dt);
                                App::GetGameContext()->UpdateSkyInputEvents(dt);
                                if (App::GetGameContext()->GetPlayerActor() &&
                                    App::GetGameContext()->GetPlayerActor()->ar_state != ActorState::NETWORKED_OK) // we are in a vehicle
                                {
                                    App::GetGameContext()->UpdateCommonInputEvents(dt);
                                    if (App::GetGameContext()->GetPlayerActor()->ar_state != ActorState::LOCAL_REPLAY)
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
                    } // app state SIMULATION
                } // interactive key binding mode
            } // dt != 0

            // Update OutGauge device
            if (App::io_outgauge_mode->getInt() > 0)
            {
                App::GetOutGauge()->Update(dt, App::GetGameContext()->GetPlayerActor());
            }

            // Early GUI updates which require halted physics
            App::GetGuiManager()->NewImGuiFrame(dt);
            if (App::app_state->getEnum<AppState>() == AppState::SIMULATION)
            {
                App::GetGuiManager()->DrawSimulationGui(dt);
                for (ActorPtr& actor : App::GetGameContext()->GetActorManager()->GetActors())
                {
                    actor->GetGfxActor()->UpdateDebugView();
                }
                if (App::GetGameContext()->GetPlayerActor())
                {
                    App::GetGuiManager()->SimActorStats.UpdateStats(dt, App::GetGameContext()->GetPlayerActor());
                    if (App::GetGuiManager()->FrictionSettings.IsVisible())
                    {
                        App::GetGuiManager()->FrictionSettings.setActiveCol(App::GetGameContext()->GetPlayerActor()->ar_last_fuzzy_ground_model);
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
            App::GetScriptEngine()->framestep(dt);
#endif // USE_ANGELSCRIPT

            if (App::io_ffb_enabled->getBool() &&
                App::sim_state->getEnum<SimState>() == SimState::RUNNING)
            {
                App::GetAppContext()->GetForceFeedback().Update();
            }

            if (App::sim_state->getEnum<SimState>() == SimState::RUNNING)
            {
                App::GetGameContext()->GetSceneMouse().UpdateSimulation();
            }

            // Create snapshot of simulation state for Gfx/GUI updates
            if (App::sim_state->getEnum<SimState>() == SimState::RUNNING ||   // Obviously
                App::sim_state->getEnum<SimState>() == SimState::EDITOR_MODE) // Needed for character movement
            {
                App::GetGfxScene()->BufferSimulationData();
            }

            // Advance simulation
            if (App::sim_state->getEnum<SimState>() == SimState::RUNNING)
            {
                App::GetGameContext()->UpdateActors(); // *** Start new physics tasks. No reading from Actor N/B beyond this point.
            }

            // Scene and GUI updates
            if (App::app_state->getEnum<AppState>() == AppState::MAIN_MENU)
            {
                App::GetGuiManager()->DrawMainMenuGui();
            }
            else if (App::app_state->getEnum<AppState>() == AppState::SIMULATION)
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

    rmt_DestroyGlobalInstance(rmt);

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
