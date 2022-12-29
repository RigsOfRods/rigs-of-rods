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
static AppContext       g_app_context;
static Console          g_console;
static ContentManager   g_content_manager;
static OverlayWrapper*  g_overlay_wrapper;
static GUIManager*      g_gui_manager;
static InputEngine*     g_input_engine;
static CacheSystem*     g_cache_system;
static MumbleIntegration* g_mumble;
static ThreadPool*      g_thread_pool;
static CameraManager*   g_camera_manager;
static GfxScene         g_gfx_scene;
static SoundScriptManager* g_sound_script_manager;
static LanguageEngine   g_language_engine;
static ScriptEngine*    g_script_engine;
static Network          g_network;
static GameContext      g_game_context;
static OutGauge         g_out_gauge;
static DiscordRpc       g_discord_rpc;

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

// Instance management
void SetCacheSystem    (CacheSystem* obj)             { g_cache_system = obj; }

// Instance access
AppContext*            GetAppContext         () { return &g_app_context; };
ContentManager*        GetContentManager     () { return &g_content_manager;}
OverlayWrapper*        GetOverlayWrapper     () { return g_overlay_wrapper;}
GUIManager*            GetGuiManager         () { return g_gui_manager;}
Console*               GetConsole            () { return &g_console;}
InputEngine*           GetInputEngine        () { return g_input_engine;}
CacheSystem*           GetCacheSystem        () { return g_cache_system;}
MumbleIntegration*     GetMumble             () { return g_mumble; }
ThreadPool*            GetThreadPool         () { return g_thread_pool; }
CameraManager*         GetCameraManager      () { return g_camera_manager; }
GfxScene*              GetGfxScene           () { return &g_gfx_scene; }
SoundScriptManager*    GetSoundScriptManager () { return g_sound_script_manager; }
LanguageEngine*        GetLanguageEngine     () { return &g_language_engine; }
ScriptEngine*          GetScriptEngine       () { return g_script_engine; }
Network*               GetNetwork            () { return &g_network; }
GameContext*           GetGameContext        () { return &g_game_context; }
OutGauge*              GetOutGauge           () { return &g_out_gauge; }
DiscordRpc*            GetDiscordRpc         () { return &g_discord_rpc; }

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
    ROR_ASSERT(!g_sound_script_manager);
    g_sound_script_manager = new SoundScriptManager();
}

void CreateScriptEngine()
{
    ROR_ASSERT(!g_script_engine);
    g_script_engine = new ScriptEngine();
}

// Cleanup
void DestroyOverlayWrapper()
{
    delete g_overlay_wrapper;
    g_overlay_wrapper = nullptr;
}

} // namespace App

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

} // namespace RoR
