/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

    For more information see http://www.rigsofrods.org/

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

/// @file   Application.cpp
/// @author Petr Ohlidal
/// @date   05/2014

#include "Application.h"

#include "AppContext.h"
#include "CacheSystem.h"
#include "CameraManager.h"
#include "Console.h"
#include "ContentManager.h"
#include "DiscordRpc.h"
#include "GameContext.h"
#include "GfxScene.h"
#include "GUIManager.h"
#include "InputEngine.h"
#include "Language.h"
#include "OutGauge.h"
#include "OverlayWrapper.h"
#include "MumbleIntegration.h"
#include "Network.h"
#include "ScriptEngine.h"
#include "SoundScriptManager.h"
#include "Terrain.h"
#include "ThreadPool.h"

namespace RoR {
namespace App {

// ------------------------------------------------------------------------------------------------
// Global variables
// ------------------------------------------------------------------------------------------------

// Object instances
static AppContext           g_app_context;
static CacheSystem          g_cache_system;
static CameraManager*       g_camera_manager;
static Console              g_console;
static ContentManager       g_content_manager;
static DiscordRpc           g_discord_rpc;
static GameContext          g_game_context;
static GfxScene             g_gfx_scene;
static GUIManager*          g_gui_manager;
static InputEngine*         g_input_engine;
static LanguageEngine       g_language_engine;
static MumbleIntegration*   g_mumble;
static OverlayWrapper*      g_overlay_wrapper;
static OutGauge             g_out_gauge;
static ScriptEngine*        g_script_engine;
static SoundScriptManager*  g_sound_script_manager;
static Terrain*             g_sim_terrain;
static ThreadPool*          g_thread_pool;
#if USE_SOCKETW
    static Network          g_network;
#endif

// App
CVar* app_state;
CVar* app_language;
CVar* app_country;
CVar* app_skip_main_menu;
CVar* app_async_physics;
CVar* app_num_workers;
CVar* app_screenshot_format;
CVar* app_rendersys_override;
CVar* app_extra_mod_path;
CVar* app_force_cache_purge;
CVar* app_force_cache_update;
CVar* app_disable_online_api;
CVar* app_config_long_names;
CVar* app_custom_scripts;
CVar* app_recent_scripts;

// Simulation
CVar* sim_state;
CVar* sim_terrain_name;
CVar* sim_terrain_gui_name;
CVar* sim_load_savegame;
CVar* sim_spawn_running;
CVar* sim_replay_enabled;
CVar* sim_replay_length;
CVar* sim_replay_stepping;
CVar* sim_realistic_commands;
CVar* sim_races_enabled;
CVar* sim_no_collisions;
CVar* sim_no_self_collisions;
CVar* sim_gearbox_mode;
CVar* sim_soft_reset_mode;
CVar* sim_quickload_dialog;
CVar* sim_live_repair_interval;
CVar* sim_tuning_enabled;

// Multiplayer
CVar* mp_state;
CVar* mp_join_on_startup;
CVar* mp_chat_auto_hide;
CVar* mp_hide_net_labels;
CVar* mp_hide_own_net_label;
CVar* mp_pseudo_collisions;
CVar* mp_server_host;
CVar* mp_server_port;
CVar* mp_server_password;
CVar* mp_player_name;
CVar* mp_player_token;
CVar* mp_api_url;
CVar* mp_cyclethru_net_actors;

// New remote API
CVar* remote_query_url;

// Diagnostic
CVar* diag_auto_spawner_report;
CVar* diag_camera;
CVar* diag_rig_log_node_import;
CVar* diag_rig_log_node_stats;
CVar* diag_truck_mass;
CVar* diag_envmap;
CVar* diag_videocameras;
CVar* diag_preset_terrain;
CVar* diag_preset_spawn_pos;
CVar* diag_preset_spawn_rot;
CVar* diag_preset_vehicle;
CVar* diag_preset_veh_config;
CVar* diag_preset_veh_enter;
CVar* diag_log_console_echo;
CVar* diag_log_beam_break;
CVar* diag_log_beam_deform;
CVar* diag_log_beam_trigger;
CVar* diag_simple_materials;
CVar* diag_warning_texture;
CVar* diag_hide_broken_beams;
CVar* diag_hide_beam_stress;
CVar* diag_hide_wheel_info;
CVar* diag_hide_wheels;
CVar* diag_hide_nodes;
CVar* diag_terrn_log_roads;
CVar* diag_actor_dump;
CVar* diag_allow_window_resize;

// System
CVar* sys_process_dir;
CVar* sys_user_dir;
CVar* sys_config_dir;
CVar* sys_cache_dir;
CVar* sys_thumbnails_dir;
CVar* sys_logs_dir;
CVar* sys_resources_dir;
CVar* sys_profiler_dir;
CVar* sys_savegames_dir;
CVar* sys_screenshot_dir;
CVar* sys_scripts_dir;
CVar* sys_projects_dir;

// OS command line
CVar* cli_server_host;
CVar* cli_server_port;
CVar* cli_preset_vehicle;
CVar* cli_preset_veh_config;
CVar* cli_preset_terrain;
CVar* cli_preset_spawn_pos;
CVar* cli_preset_spawn_rot;
CVar* cli_preset_veh_enter;
CVar* cli_force_cache_update;
CVar* cli_resume_autosave;
CVar* cli_custom_scripts;

// Input - Output
CVar* io_analog_smoothing;
CVar* io_analog_sensitivity;
CVar* io_blink_lock_range;
CVar* io_ffb_enabled;
CVar* io_ffb_camera_gain;
CVar* io_ffb_center_gain;
CVar* io_ffb_master_gain;
CVar* io_ffb_stress_gain;
CVar* io_input_grab_mode;
CVar* io_arcade_controls;
CVar* io_hydro_coupling;
CVar* io_outgauge_mode;
CVar* io_outgauge_ip;
CVar* io_outgauge_port;
CVar* io_outgauge_delay;
CVar* io_outgauge_id;
CVar* io_discord_rpc;
CVar* io_invert_orbitcam;
              
// Audio
CVar* audio_master_volume;
CVar* audio_enable_creak;
CVar* audio_device_name;
CVar* audio_menu_music;

// Graphics
CVar* gfx_flares_mode;
CVar* gfx_polygon_mode;
CVar* gfx_shadow_type;
CVar* gfx_extcam_mode;
CVar* gfx_sky_mode;
CVar* gfx_sky_time_cycle;
CVar* gfx_sky_time_speed;
CVar* gfx_texture_filter;
CVar* gfx_vegetation_mode;
CVar* gfx_water_mode;
CVar* gfx_anisotropy;
CVar* gfx_water_waves;
CVar* gfx_particles_mode;
CVar* gfx_enable_videocams;
CVar* gfx_window_videocams;
CVar* gfx_surveymap_icons;
CVar* gfx_declutter_map;
CVar* gfx_envmap_enabled;
CVar* gfx_envmap_rate;
CVar* gfx_shadow_quality;
CVar* gfx_skidmarks_mode;
CVar* gfx_sight_range;
CVar* gfx_camera_height;
CVar* gfx_fov_external;
CVar* gfx_fov_external_default;
CVar* gfx_fov_internal;
CVar* gfx_fov_internal_default;
CVar* gfx_static_cam_fov_exp;
CVar* gfx_fixed_cam_tracking;
CVar* gfx_fps_limit;
CVar* gfx_speedo_digital;
CVar* gfx_speedo_imperial;
CVar* gfx_flexbody_cache;
CVar* gfx_reduce_shadows;
CVar* gfx_enable_rtshaders;
CVar* gfx_alt_actor_materials;

// Flexbodies
CVar* flexbody_defrag_enabled;
CVar* flexbody_defrag_const_penalty;
CVar* flexbody_defrag_prog_up_penalty;
CVar* flexbody_defrag_prog_down_penalty;
CVar* flexbody_defrag_reorder_indices;
CVar* flexbody_defrag_reorder_texcoords;
CVar* flexbody_defrag_invert_lookup;

// GUI
CVar* ui_show_live_repair_controls;
CVar* ui_show_vehicle_buttons;

// Instance access
AppContext*            GetAppContext         () { return &g_app_context; };
ContentManager*        GetContentManager     () { return &g_content_manager;}
OverlayWrapper*        GetOverlayWrapper     () { return g_overlay_wrapper;}
GUIManager*            GetGuiManager         () { return g_gui_manager;}
Console*               GetConsole            () { return &g_console;}
InputEngine*           GetInputEngine        () { return g_input_engine;}
CacheSystem*           GetCacheSystem        () { return &g_cache_system;}
MumbleIntegration*     GetMumble             () { return g_mumble; }
ThreadPool*            GetThreadPool         () { return g_thread_pool; }
CameraManager*         GetCameraManager      () { return g_camera_manager; }
GfxScene*              GetGfxScene           () { return &g_gfx_scene; }
SoundScriptManager*    GetSoundScriptManager () { return g_sound_script_manager; }
LanguageEngine*        GetLanguageEngine     () { return &g_language_engine; }
ScriptEngine*          GetScriptEngine       () { return g_script_engine; }
GameContext*           GetGameContext        () { return &g_game_context; }
OutGauge*              GetOutGauge           () { return &g_out_gauge; }
DiscordRpc*            GetDiscordRpc         () { return &g_discord_rpc; }
#if USE_SOCKETW
    Network*           GetNetwork            () { return &g_network; }
#else
    Network*           GetNetwork            () { return nullptr; }
#endif

// Factories
void CreateOverlayWrapper()
{
    ROR_ASSERT(!g_overlay_wrapper);
    g_overlay_wrapper = new OverlayWrapper();
}

void CreateGuiManager()
{
    ROR_ASSERT(!g_gui_manager);
    g_gui_manager = new GUIManager();
}

void CreateInputEngine()
{
    ROR_ASSERT(!g_input_engine);
    g_input_engine = new InputEngine();
}

void CreateMumble()
{
#ifdef USE_MUMBLE // The class is always forward-declared but only defined if USE_MUMBLE is defined
    ROR_ASSERT(!g_mumble);
    g_mumble = new MumbleIntegration();
#endif // USE_MUMBLE
}

void CreateThreadPool()
{
    ROR_ASSERT(g_thread_pool == nullptr);
    g_thread_pool = ThreadPool::DetectNumWorkersAndCreate();
}

void CreateCameraManager()
{
    ROR_ASSERT(!g_camera_manager);
    g_camera_manager = new CameraManager();
}

void CreateGfxScene()
{
    ROR_ASSERT(!g_gfx_scene.GetSceneManager());
    g_gfx_scene.Init();
}

void CreateSoundScriptManager()
{
#if USE_OPENAL
    ROR_ASSERT(!g_sound_script_manager);
    g_sound_script_manager = new SoundScriptManager();
#endif
}

void CreateScriptEngine()
{
#if USE_ANGELSCRIPT
    ROR_ASSERT(!g_script_engine);
    g_script_engine = new ScriptEngine();
#endif
}

// Cleanup
void DestroyOverlayWrapper()
{
    delete g_overlay_wrapper;
    g_overlay_wrapper = nullptr;
}

void DestroyInputEngine()
{
    delete g_input_engine;
    g_input_engine = nullptr;
}

} // namespace App

// ------------------------------------------------------------------------------------------------
// Global exception handling
// ------------------------------------------------------------------------------------------------

void HandleGenericException(const std::string& from, BitMask_t flags)
{
    try
    {
        throw; // rethrow
    }
    catch (Ogre::Exception& oex)
    {
        if (flags & HANDLEGENERICEXCEPTION_CONSOLE)
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, fmt::format("{}: {}", from, oex.what()));
        else if (flags & HANDLEGENERICEXCEPTION_LOGFILE)
            LOG(fmt::format("{}: {}", from, oex.getDescription()));
#ifdef USE_ANGELSCRIPT
        if (flags & HANDLEGENERICEXCEPTION_SCRIPTEVENT)
            App::GetScriptEngine()->forwardExceptionAsScriptEvent(from);
#endif
    }
    catch (std::exception& stex)
    {
        if (flags & HANDLEGENERICEXCEPTION_CONSOLE)
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, fmt::format("{}: {}", from, stex.what()));
        else if (flags & HANDLEGENERICEXCEPTION_LOGFILE)
            LOG(fmt::format("{}: {}", from, stex.what()));
#ifdef USE_ANGELSCRIPT
        if (flags & HANDLEGENERICEXCEPTION_SCRIPTEVENT)
            App::GetScriptEngine()->forwardExceptionAsScriptEvent(from);
#endif
    }
    catch (...)
    {
        if (flags & HANDLEGENERICEXCEPTION_CONSOLE)
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, fmt::format("{}: Unknown exception", from));
        else if (flags & HANDLEGENERICEXCEPTION_LOGFILE)
            LOG(fmt::format("{}: Unknown exception", from));
#ifdef USE_ANGELSCRIPT
        if (flags & HANDLEGENERICEXCEPTION_SCRIPTEVENT)
            App::GetScriptEngine()->forwardExceptionAsScriptEvent(from);
#endif
    }
}

void HandleMsgQueueException(MsgType from)
{
    HandleGenericException(MsgTypeToString(from), HANDLEGENERICEXCEPTION_CONSOLE | HANDLEGENERICEXCEPTION_SCRIPTEVENT);
}

// ------------------------------------------------------------------------------------------------
// Global logging
// ------------------------------------------------------------------------------------------------

void Log(const char* msg)
{
    Ogre::LogManager::getSingleton().logMessage(msg);
}

void LogFormat(const char* format, ...)
{
    char buffer[2000] = {};

    va_list args;
    va_start(args, format);
        vsprintf(buffer, format, args);
    va_end(args);

    RoR::Log(buffer);
}

// ------------------------------------------------------------------------------------------------
// Global enums
// ------------------------------------------------------------------------------------------------

std::string ToLocalizedString(SimGearboxMode e)
{
    switch (e)
    {
    case SimGearboxMode::AUTO:          return _LC("SimGearboxMode", "Automatic shift");
    case SimGearboxMode::SEMI_AUTO:     return _LC("SimGearboxMode", "Manual shift with auto clutch");
    case SimGearboxMode::MANUAL:        return _LC("SimGearboxMode", "Fully manual: sequential shift");
    case SimGearboxMode::MANUAL_STICK:  return _LC("SimGearboxMode", "Fully manual: stick shift");
    case SimGearboxMode::MANUAL_RANGES: return _LC("SimGearboxMode", "Fully manual: stick shift with ranges");
    default:                            return "";
    }
}

std::string ToLocalizedString(GfxFlaresMode e)
{
    switch (e)
    {
    case GfxFlaresMode::NONE:                    return _LC("GfxFlaresMode", "None (fastest)");
    case GfxFlaresMode::NO_LIGHTSOURCES:         return _LC("GfxFlaresMode", "No light sources");
    case GfxFlaresMode::CURR_VEHICLE_HEAD_ONLY:  return _LC("GfxFlaresMode", "Only current vehicle, main lights");
    case GfxFlaresMode::ALL_VEHICLES_HEAD_ONLY:  return _LC("GfxFlaresMode", "All vehicles, main lights");
    case GfxFlaresMode::ALL_VEHICLES_ALL_LIGHTS: return _LC("GfxFlaresMode", "All vehicles, all lights");
    default:                                     return "";
    }
}

std::string ToLocalizedString(GfxShadowType e)
{
    switch (e)
    {
    case GfxShadowType::NONE:         return _LC("GfxShadowType", "Disabled");
    case GfxShadowType::PSSM:         return _LC("GfxShadowType", "PSSM");
    default:                          return "";
    }
}

std::string ToLocalizedString(GfxSkyMode e)
{
    switch (e)
    {
    case GfxSkyMode::SANDSTORM: return _LC("GfxSkyMode", "Sandstorm (fastest)");
    case GfxSkyMode::CAELUM:    return _LC("GfxSkyMode", "Caelum (best looking, slower)");
    case GfxSkyMode::SKYX:      return _LC("GfxSkyMode", "SkyX (best looking, slower)");
    default:                    return "";
    }
}

std::string ToLocalizedString(GfxTexFilter e)
{
    switch (e)
    {
    case GfxTexFilter::NONE:         return _LC("GfxTexFilter", "None");
    case GfxTexFilter::BILINEAR:     return _LC("GfxTexFilter", "Bilinear");
    case GfxTexFilter::TRILINEAR:    return _LC("GfxTexFilter", "Trilinear");
    case GfxTexFilter::ANISOTROPIC:  return _LC("GfxTexFilter", "Anisotropic");
    default:                         return "";
    }
}

std::string ToLocalizedString(GfxVegetation e)
{
    switch (e)
    {
    case GfxVegetation::NONE:        return _LC("GfxVegetation", "None");
    case GfxVegetation::x20PERC:     return _LC("GfxVegetation", "20%");
    case GfxVegetation::x50PERC:     return _LC("GfxVegetation", "50%");
    case GfxVegetation::FULL:        return _LC("GfxVegetation", "Full");
    default:                         return "";
    }
}

std::string ToLocalizedString(GfxWaterMode e)
{
    switch (e)
    {
    case GfxWaterMode::NONE:      return _LC("GfxWaterMode", "None");
    case GfxWaterMode::BASIC:     return _LC("GfxWaterMode", "Basic (fastest)");
    case GfxWaterMode::REFLECT:   return _LC("GfxWaterMode", "Reflection");
    case GfxWaterMode::FULL_FAST: return _LC("GfxWaterMode", "Reflection + refraction (speed optimized)");
    case GfxWaterMode::FULL_HQ:   return _LC("GfxWaterMode", "Reflection + refraction (quality optimized)");
    case GfxWaterMode::HYDRAX:    return _LC("GfxWaterMode", "HydraX");
    default:                      return "";
    }
}

std::string ToLocalizedString(GfxExtCamMode e)
{
    switch (e)
    {
    case GfxExtCamMode::NONE:      return _LC("GfxExtCamMode", "None");
    case GfxExtCamMode::STATIC:    return _LC("GfxExtCamMode", "Static");
    case GfxExtCamMode::PITCHING:  return _LC("GfxExtCamMode", "Pitching");
    default:                       return "";
    }
}

std::string ToLocalizedString(IoInputGrabMode e)
{
    switch (e)
    {
    case IoInputGrabMode::NONE:      return _LC("IoInputGrabMode", "None");
    case IoInputGrabMode::ALL:       return _LC("IoInputGrabMode", "All");
    case IoInputGrabMode::DYNAMIC:   return _LC("IoInputGrabMode", "Dynamic");
    default:                         return "";
    }
}

std::string ToLocalizedString(SimResetMode e)
{
    switch (e)
    {
    case SimResetMode::HARD: return _LC("SimResetMode", "Hard");
    case SimResetMode::SOFT: return _LC("SimResetMode", "Soft");
    default:                 return "";
    }
}

const char* MsgTypeToString(MsgType type)
{
    switch (type)
    {
    case MSG_APP_SHUTDOWN_REQUESTED           : return "MSG_APP_SHUTDOWN_REQUESTED";
    case MSG_APP_SCREENSHOT_REQUESTED         : return "MSG_APP_SCREENSHOT_REQUESTED";
    case MSG_APP_DISPLAY_FULLSCREEN_REQUESTED : return "MSG_APP_DISPLAY_FULLSCREEN_REQUESTED";
    case MSG_APP_DISPLAY_WINDOWED_REQUESTED   : return "MSG_APP_DISPLAY_WINDOWED_REQUESTED";
    case MSG_APP_MODCACHE_LOAD_REQUESTED      : return "MSG_APP_MODCACHE_LOAD_REQUESTED";
    case MSG_APP_MODCACHE_UPDATE_REQUESTED    : return "MSG_APP_MODCACHE_UPDATE_REQUESTED";
    case MSG_APP_MODCACHE_PURGE_REQUESTED     : return "MSG_APP_MODCACHE_PURGE_REQUESTED";
    case MSG_APP_LOAD_SCRIPT_REQUESTED        : return "MSG_APP_LOAD_SCRIPT_REQUESTED";  
    case MSG_APP_UNLOAD_SCRIPT_REQUESTED      : return "MSG_APP_UNLOAD_SCRIPT_REQUESTED";
    case MSG_APP_SCRIPT_THREAD_STATUS         : return "MSG_APP_SCRIPT_THREAD_STATUS";   
    case MSG_APP_REINIT_INPUT_REQUESTED       : return "MSG_APP_REINIT_INPUT_REQUESTED";

    case MSG_NET_CONNECT_REQUESTED            : return "MSG_NET_CONNECT_REQUESTED";
    case MSG_NET_CONNECT_STARTED              : return "MSG_NET_CONNECT_STARTED";
    case MSG_NET_CONNECT_PROGRESS             : return "MSG_NET_CONNECT_PROGRESS";
    case MSG_NET_CONNECT_SUCCESS              : return "MSG_NET_CONNECT_SUCCESS";
    case MSG_NET_CONNECT_FAILURE              : return "MSG_NET_CONNECT_FAILURE";
    case MSG_NET_SERVER_KICK                  : return "MSG_NET_SERVER_KICK";
    case MSG_NET_DISCONNECT_REQUESTED         : return "MSG_NET_DISCONNECT_REQUESTED";
    case MSG_NET_USER_DISCONNECT              : return "MSG_NET_USER_DISCONNECT";
    case MSG_NET_RECV_ERROR                   : return "MSG_NET_RECV_ERROR";
    case MSG_NET_REFRESH_SERVERLIST_SUCCESS   : return "MSG_NET_REFRESH_SERVERLIST_SUCCESS";
    case MSG_NET_REFRESH_SERVERLIST_FAILURE   : return "MSG_NET_REFRESH_SERVERLIST_FAILURE";
    case MSG_NET_REFRESH_REPOLIST_SUCCESS     : return "MSG_NET_REFRESH_REPOLIST_SUCCESS";
    case MSG_NET_OPEN_RESOURCE_SUCCESS        : return "MSG_NET_OPEN_RESOURCE_SUCCESS";
    case MSG_NET_REFRESH_REPOLIST_FAILURE     : return "MSG_NET_REFRESH_REPOLIST_FAILURE";
    case MSG_NET_FETCH_AI_PRESETS_SUCCESS     : return "MSG_NET_FETCH_AI_PRESETS_SUCCESS";
    case MSG_NET_FETCH_AI_PRESETS_FAILURE     : return "MSG_NET_FETCH_AI_PRESETS_FAILURE";

    case MSG_SIM_PAUSE_REQUESTED              : return "MSG_SIM_PAUSE_REQUESTED";
    case MSG_SIM_UNPAUSE_REQUESTED            : return "MSG_SIM_UNPAUSE_REQUESTED";
    case MSG_SIM_LOAD_TERRN_REQUESTED         : return "MSG_SIM_LOAD_TERRN_REQUESTED";
    case MSG_SIM_LOAD_SAVEGAME_REQUESTED      : return "MSG_SIM_LOAD_SAVEGAME_REQUESTED";
    case MSG_SIM_UNLOAD_TERRN_REQUESTED       : return "MSG_SIM_UNLOAD_TERRN_REQUESTED";
    case MSG_SIM_SPAWN_ACTOR_REQUESTED        : return "MSG_SIM_SPAWN_ACTOR_REQUESTED";
    case MSG_SIM_MODIFY_ACTOR_REQUESTED       : return "MSG_SIM_MODIFY_ACTOR_REQUESTED";
    case MSG_SIM_DELETE_ACTOR_REQUESTED       : return "MSG_SIM_DELETE_ACTOR_REQUESTED";
    case MSG_SIM_SEAT_PLAYER_REQUESTED        : return "MSG_SIM_SEAT_PLAYER_REQUESTED";
    case MSG_SIM_TELEPORT_PLAYER_REQUESTED    : return "MSG_SIM_TELEPORT_PLAYER_REQUESTED";
    case MSG_SIM_HIDE_NET_ACTOR_REQUESTED     : return "MSG_SIM_HIDE_NET_ACTOR_REQUESTED";
    case MSG_SIM_UNHIDE_NET_ACTOR_REQUESTED   : return "MSG_SIM_UNHIDE_NET_ACTOR_REQUESTED";
    case MSG_SIM_SCRIPT_EVENT_TRIGGERED       : return "MSG_SIM_SCRIPT_EVENT_TRIGGERED";
    case MSG_SIM_SCRIPT_CALLBACK_QUEUED       : return "MSG_SIM_SCRIPT_CALLBACK_QUEUED";
    case MSG_SIM_ACTOR_LINKING_REQUESTED      : return "MSG_SIM_ACTOR_LINKING_REQUESTED";
    case MSG_SIM_ADD_FREEFORCE_REQUESTED      : return "MSG_SIM_ADD_FREEFORCE_REQUESTED";
    case MSG_SIM_MODIFY_FREEFORCE_REQUESTED   : return "MSG_SIM_MODIFY_FREEFORCE_REQUESTED";
    case MSG_SIM_REMOVE_FREEFORCE_REQUESTED   : return "MSG_SIM_REMOVE_FREEFORCE_REQUESTED";

    case MSG_GUI_OPEN_MENU_REQUESTED          : return "MSG_GUI_OPEN_MENU_REQUESTED";
    case MSG_GUI_CLOSE_MENU_REQUESTED         : return "MSG_GUI_CLOSE_MENU_REQUESTED";
    case MSG_GUI_OPEN_SELECTOR_REQUESTED      : return "MSG_GUI_OPEN_SELECTOR_REQUESTED";
    case MSG_GUI_CLOSE_SELECTOR_REQUESTED     : return "MSG_GUI_CLOSE_SELECTOR_REQUESTED";
    case MSG_GUI_MP_CLIENTS_REFRESH           : return "MSG_GUI_MP_CLIENTS_REFRESH";
    case MSG_GUI_SHOW_MESSAGE_BOX_REQUESTED   : return "MSG_GUI_SHOW_MESSAGE_BOX_REQUESTED";
    case MSG_GUI_HIDE_MESSAGE_BOX_REQUESTED   : return "MSG_GUI_HIDE_MESSAGE_BOX_REQUESTED";
    case MSG_GUI_DOWNLOAD_PROGRESS            : return "MSG_GUI_DOWNLOAD_PROGRESS";
    case MSG_GUI_DOWNLOAD_FINISHED            : return "MSG_GUI_DOWNLOAD_FINISHED";
    case MSG_GUI_REFRESH_TUNING_MENU_REQUESTED: return "MSG_GUI_REFRESH_TUNING_MENU_REQUESTED";

    case MSG_EDI_MODIFY_GROUNDMODEL_REQUESTED : return "MSG_EDI_MODIFY_GROUNDMODEL_REQUESTED";
    case MSG_EDI_ENTER_TERRN_EDITOR_REQUESTED : return "MSG_EDI_ENTER_TERRN_EDITOR_REQUESTED";
    case MSG_EDI_LEAVE_TERRN_EDITOR_REQUESTED : return "MSG_EDI_LEAVE_TERRN_EDITOR_REQUESTED";
    case MSG_EDI_LOAD_BUNDLE_REQUESTED        : return "MSG_EDI_LOAD_BUNDLE_REQUESTED";
    case MSG_EDI_RELOAD_BUNDLE_REQUESTED      : return "MSG_EDI_RELOAD_BUNDLE_REQUESTED";
    case MSG_EDI_UNLOAD_BUNDLE_REQUESTED      : return "MSG_EDI_UNLOAD_BUNDLE_REQUESTED";
    case MSG_EDI_CREATE_PROJECT_REQUESTED     : return "MSG_EDI_CREATE_PROJECT_REQUESTED";
    case MSG_EDI_MODIFY_PROJECT_REQUESTED     : return "MSG_EDI_MODIFY_PROJECT_REQUESTED";
    case MSG_EDI_DELETE_PROJECT_REQUESTED     : return "MSG_EDI_DELETE_PROJECT_REQUESTED";

    default: return "";
    }
}

} // namespace RoR

namespace RigDef
{

const char * KeywordToString(RigDef::Keyword keyword)
{
    // PLEASE maintain alphabetical order!

    switch (keyword)
    {
        case Keyword::ADD_ANIMATION:        return "add_animation";
        case Keyword::AIRBRAKES:            return "airbrakes";
        case Keyword::ANIMATORS:            return "animators";
        case Keyword::ANTILOCKBRAKES:       return "antilockbrakes";
        case Keyword::AUTHOR:               return "author";
        case Keyword::AXLES:                return "axles";
        case Keyword::BEAMS:                return "beams";
        case Keyword::BRAKES:               return "brakes";
        case Keyword::CAB:                  return "cab";
        case Keyword::CAMERAS:              return "cameras";
        case Keyword::CAMERARAIL:           return "camerarail";
        case Keyword::CINECAM:              return "cinecam";
        case Keyword::COLLISIONBOXES:       return "collisionboxes";
        case Keyword::COMMANDS:             return "commands";
        case Keyword::COMMANDS2:            return "commands2";
        case Keyword::COMMENT:              return "comment";
        case Keyword::CONTACTERS:           return "contacters";
        case Keyword::CRUISECONTROL:        return "cruisecontrol";
        case Keyword::DESCRIPTION:          return "description";
        case Keyword::DETACHER_GROUP:       return "detacher_group";
        case Keyword::DISABLEDEFAULTSOUNDS: return "disabledefaultsounds";
        case Keyword::ENABLE_ADVANCED_DEFORMATION: return "enable_advanced_deformation";
        case Keyword::END:                  return "end";
        case Keyword::END_COMMENT:          return "end_comment";
        case Keyword::END_DESCRIPTION:      return "end_description";
        case Keyword::END_SECTION:          return "end_section";
        case Keyword::ENGINE:               return "engine";
        case Keyword::ENGOPTION:            return "engoption";
        case Keyword::ENGTURBO:             return "engturbo";
        case Keyword::EXHAUSTS:             return "exhausts";
        case Keyword::EXTCAMERA:            return "extcamera";
        case Keyword::FILEINFO:             return "fileinfo";
        case Keyword::FILEFORMATVERSION:    return "fileformatversion";
        case Keyword::FIXES:                return "fixes";
        case Keyword::FLARES:               return "flares";
        case Keyword::FLARES2:              return "flares2";
        case Keyword::FLEXBODIES:           return "flexbodies";
        case Keyword::FLEXBODY_CAMERA_MODE: return "flexbody_camera_mode";
        case Keyword::FLEXBODYWHEELS:       return "flexbodywheels";
        case Keyword::FORSET:               return "forset";
        case Keyword::FORWARDCOMMANDS:      return "forwardcommands";
        case Keyword::FUSEDRAG:             return "fusedrag";
        case Keyword::GLOBALS:              return "globals";
        case Keyword::GUID:                 return "guid";
        case Keyword::GUISETTINGS:          return "guisettings";
        case Keyword::HELP:                 return "help";
        case Keyword::HIDEINCHOOSER:        return "hideinchooser";
        case Keyword::HOOKGROUP:            return "hookgroup";
        case Keyword::HOOKS:                return "hooks";
        case Keyword::HYDROS:               return "hydros";
        case Keyword::IMPORTCOMMANDS:       return "importcommands";
        case Keyword::INTERAXLES:           return "interaxles";
        case Keyword::LOCKGROUPS:           return "lockgroups";
        case Keyword::LOCKGROUP_DEFAULT_NOLOCK: return "lockgroup_default_nolock";
        case Keyword::MANAGEDMATERIALS:     return "managedmaterials";
        case Keyword::MATERIALFLAREBINDINGS: return "materialflarebindings";
        case Keyword::MESHWHEELS:           return "meshwheels";
        case Keyword::MESHWHEELS2:          return "meshwheels2";
        case Keyword::MINIMASS:             return "minimass";
        case Keyword::NODES:                return "nodes";
        case Keyword::NODES2:               return "nodes2";
        case Keyword::PARTICLES:            return "particles";
        case Keyword::PISTONPROPS:          return "pistonprops";
        case Keyword::PROP_CAMERA_MODE:     return "prop_camera_mode";
        case Keyword::PROPS:                return "props";
        case Keyword::RAILGROUPS:           return "railgroups";
        case Keyword::RESCUER:              return "rescuer";
        case Keyword::RIGIDIFIERS:          return "rigidifiers";
        case Keyword::ROLLON:               return "rollon";
        case Keyword::ROPABLES:             return "ropables";
        case Keyword::ROPES:                return "ropes";
        case Keyword::ROTATORS:             return "rotators";
        case Keyword::ROTATORS2:            return "rotators2";
        case Keyword::SCREWPROPS:           return "screwprops";
        case Keyword::SECTION:              return "section";
        case Keyword::SECTIONCONFIG:        return "sectionconfig";
        case Keyword::SET_BEAM_DEFAULTS:    return "set_beam_defaults";
        case Keyword::SET_BEAM_DEFAULTS_SCALE: return "set_beam_defaults_scale";
        case Keyword::SET_COLLISION_RANGE:  return "set_collision_range";
        case Keyword::SET_DEFAULT_MINIMASS: return "set_default_minimass";
        case Keyword::SET_INERTIA_DEFAULTS: return "set_inertia_defaults";
        case Keyword::SET_MANAGEDMATERIALS_OPTIONS: return "set_managedmaterials_options";
        case Keyword::SET_NODE_DEFAULTS:    return "set_node_defaults";
        case Keyword::SET_SHADOWS:          return "set_shadows";
        case Keyword::SET_SKELETON_SETTINGS: return "set_skeleton_settings";
        case Keyword::SHOCKS:               return "shocks";
        case Keyword::SHOCKS2:              return "shocks2";
        case Keyword::SHOCKS3:              return "shocks3";
        case Keyword::SLIDENODE_CONNECT_INSTANTLY: return "slidenode_connect_instantly";
        case Keyword::SLIDENODES:           return "slidenodes";
        case Keyword::SLOPE_BRAKE:          return "SlopeBrake";
        case Keyword::SOUNDSOURCES:         return "soundsources";
        case Keyword::SOUNDSOURCES2:        return "soundsources2";
        case Keyword::SPEEDLIMITER:         return "speedlimiter";
        case Keyword::SUBMESH:              return "submesh";
        case Keyword::SUBMESH_GROUNDMODEL:  return "submesh_groundmodel";
        case Keyword::TEXCOORDS:            return "texcoords";
        case Keyword::TIES:                 return "ties";
        case Keyword::TORQUECURVE:          return "torquecurve";
        case Keyword::TRACTIONCONTROL:      return "tractioncontrol";
        case Keyword::TRANSFERCASE:         return "transfercase";
        case Keyword::TRIGGERS:             return "triggers";
        case Keyword::TURBOJETS:            return "turbojets";
        case Keyword::TURBOPROPS:           return "turboprops";
        case Keyword::TURBOPROPS2:          return "turboprops2";
        case Keyword::VIDEOCAMERA:          return "videocamera";
        case Keyword::WHEELDETACHERS:       return "wheeldetachers";
        case Keyword::WHEELS:               return "wheels";
        case Keyword::WHEELS2:              return "wheels2";
        case Keyword::WINGS:                return "wings";

        default:                           return "";
    }
}

} // namespace RigDef
