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
#include "ThreadPool.h"

namespace RoR {
namespace App {

// ------------------------------------------------------------------------------------------------
// Global variables
// ------------------------------------------------------------------------------------------------

// Object instances
static AppContext           g_app_context;
static CacheSystem*         g_cache_system;
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
static TerrainManager*      g_sim_terrain;
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

// Diagnostic
CVar* diag_auto_spawner_report;
CVar* diag_camera;
CVar* diag_rig_log_node_import;
CVar* diag_rig_log_node_stats;
CVar* diag_collisions;
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
CVar* diag_physics_dt;
CVar* diag_terrn_log_roads;
CVar* diag_import_grp_loose;
CVar* diag_actor_dump;

// System
CVar* sys_process_dir;
CVar* sys_user_dir;
CVar* sys_config_dir;
CVar* sys_cache_dir;
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
CVar* gfx_shadow_type;
CVar* gfx_extcam_mode;
CVar* gfx_sky_mode;
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
CVar* gfx_classic_shaders;

// Instance management
void SetSimTerrain     (TerrainManager* obj)          { g_sim_terrain = obj;}
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
TerrainManager*        GetSimTerrain         () { return g_sim_terrain; }
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
// Global definitions and enumerations
// ------------------------------------------------------------------------------------------------

const char* CONF_GFX_SHADOW_PSSM    = "Parallel-split Shadow Maps";
const char* CONF_GFX_SHADOW_NONE    = "No shadows (fastest)";

const char* CONF_EXTCAM_PITCHING    = "Pitching";
const char* CONF_EXTCAM_STATIC      = "Static";
const char* CONF_EXTCAM_NONE        = "None";

const char* CONF_TEXFILTER_NONE     = "None (fastest)";
const char* CONF_TEXFILTER_BILI     = "Bilinear";
const char* CONF_TEXFILTER_TRILI    = "Trilinear";
const char* CONF_TEXFILTER_ANISO    = "Anisotropic (best looking)";

const char* CONF_VEGET_NONE         = "None (fastest)";
const char* CONF_VEGET_20PERC       = "20%";
const char* CONF_VEGET_50PERC       = "50%";
const char* CONF_VEGET_FULL         = "Full (best looking, slower)";

const char* CONF_GEARBOX_AUTO       = "Automatic shift";
const char* CONF_GEARBOX_SEMIAUTO   = "Manual shift - Auto clutch";
const char* CONF_GEARBOX_MANUAL     = "Fully Manual: sequential shift";
const char* CONF_GEARBOX_MAN_STICK  = "Fully manual: stick shift";
const char* CONF_GEARBOX_MAN_RANGES = "Fully Manual: stick shift with ranges";

const char* CONF_FLARES_NONE        = "None (fastest)";
const char* CONF_FLARES_NO_LIGHT    = "No light sources";
const char* CONF_FLARES_CURR_HEAD   = "Only current vehicle, main lights";
const char* CONF_FLARES_ALL_HEADS   = "All vehicles, main lights";
const char* CONF_FLARES_ALL_LIGHTS  = "All vehicles, all lights";

const char* CONF_WATER_NONE         = "None";
const char* CONF_WATER_BASIC        = "Basic (fastest)";
const char* CONF_WATER_REFLECT      = "Reflection";
const char* CONF_WATER_FULL_FAST    = "Reflection + refraction (speed optimized)";
const char* CONF_WATER_FULL_HQ      = "Reflection + refraction (quality optimized)";
const char* CONF_WATER_HYDRAX       = "Hydrax";

const char* CONF_SKY_CAELUM         = "Caelum (best looking, slower)";
const char* CONF_SKY_SKYX           = "SkyX (best looking, slower)";
const char* CONF_SKY_SANDSTORM      = "Sandstorm (fastest)";

const char* CONF_INPUT_GRAB_DYNAMIC = "Dynamically";
const char* CONF_INPUT_GRAB_NONE    = "None";
const char* CONF_INPUT_GRAB_ALL     = "All";

IoInputGrabMode ParseIoInputGrabMode(std::string const & s)
{
    if (s == CONF_INPUT_GRAB_DYNAMIC) { return RoR::IoInputGrabMode::DYNAMIC ; }
    if (s == CONF_INPUT_GRAB_NONE   ) { return RoR::IoInputGrabMode::NONE    ; }
    else                              { return RoR::IoInputGrabMode::ALL     ; }
}

GfxShadowType ParseGfxShadowType(std::string const & s)
{
    if (s == CONF_GFX_SHADOW_PSSM)    { return GfxShadowType::PSSM    ; }
    else                              { return GfxShadowType::NONE    ; }
}

GfxExtCamMode ParseGfxExtCamMode(std::string const & s)
{
    if (s == CONF_EXTCAM_PITCHING)    { return GfxExtCamMode::PITCHING ; }
    if (s == CONF_EXTCAM_STATIC)      { return GfxExtCamMode::STATIC   ; }
    else                              { return GfxExtCamMode::NONE     ; }
}

GfxTexFilter ParseGfxTexFilter(std::string const & s)
{
    if (s == CONF_TEXFILTER_NONE)     { return GfxTexFilter::NONE        ; }
    if (s == CONF_TEXFILTER_BILI)     { return GfxTexFilter::BILINEAR    ; }
    if (s == CONF_TEXFILTER_TRILI)    { return GfxTexFilter::TRILINEAR   ; }
    if (s == CONF_TEXFILTER_ANISO)    { return GfxTexFilter::ANISOTROPIC ; }
    else                              { return GfxTexFilter::NONE        ; }
}

GfxVegetation ParseGfxVegetation(std::string const & s)
{
    if (s == CONF_VEGET_NONE  )       { return GfxVegetation::NONE    ; }
    if (s == CONF_VEGET_20PERC)       { return GfxVegetation::x20PERC ; }
    if (s == CONF_VEGET_50PERC)       { return GfxVegetation::x50PERC ; }
    if (s == CONF_VEGET_FULL  )       { return GfxVegetation::FULL    ; }
    else                              { return GfxVegetation::NONE    ; }
}

SimGearboxMode ParseSimGearboxMode(std::string const & s)
{
    if (s == CONF_GEARBOX_AUTO      ) { return SimGearboxMode::AUTO          ; }
    if (s == CONF_GEARBOX_SEMIAUTO  ) { return SimGearboxMode::SEMI_AUTO     ; }
    if (s == CONF_GEARBOX_MANUAL    ) { return SimGearboxMode::MANUAL        ; }
    if (s == CONF_GEARBOX_MAN_STICK ) { return SimGearboxMode::MANUAL_STICK  ; }
    if (s == CONF_GEARBOX_MAN_RANGES) { return SimGearboxMode::MANUAL_RANGES ; }
    else                              { return SimGearboxMode::AUTO          ; }
}

GfxFlaresMode ParseGfxFlaresMode(std::string const & s)
{
    if (s == CONF_FLARES_NONE      )  { return GfxFlaresMode::NONE                    ; }
    if (s == CONF_FLARES_NO_LIGHT  )  { return GfxFlaresMode::NO_LIGHTSOURCES         ; }
    if (s == CONF_FLARES_CURR_HEAD )  { return GfxFlaresMode::CURR_VEHICLE_HEAD_ONLY  ; }
    if (s == CONF_FLARES_ALL_HEADS )  { return GfxFlaresMode::ALL_VEHICLES_HEAD_ONLY  ; }
    if (s == CONF_FLARES_ALL_LIGHTS)  { return GfxFlaresMode::ALL_VEHICLES_ALL_LIGHTS ; }
    else                              { return GfxFlaresMode::CURR_VEHICLE_HEAD_ONLY  ; }
}

GfxWaterMode ParseGfxWaterMode(std::string const & s)
{
    if (s == CONF_WATER_NONE     )    { return GfxWaterMode::NONE      ; }
    if (s == CONF_WATER_BASIC    )    { return GfxWaterMode::BASIC     ; }
    if (s == CONF_WATER_REFLECT  )    { return GfxWaterMode::REFLECT   ; }
    if (s == CONF_WATER_FULL_FAST)    { return GfxWaterMode::FULL_FAST ; }
    if (s == CONF_WATER_FULL_HQ  )    { return GfxWaterMode::FULL_HQ   ; }
    if (s == CONF_WATER_HYDRAX   )    { return GfxWaterMode::HYDRAX    ; }
    else                              { return GfxWaterMode::BASIC     ; }
}

GfxSkyMode ParseGfxSkyMode(std::string const & s)
{
    if (s == CONF_SKY_SANDSTORM)      { return GfxSkyMode::SANDSTORM ; }
    if (s == CONF_SKY_CAELUM   )      { return GfxSkyMode::CAELUM    ; }
    if (s == CONF_SKY_SKYX     )      { return GfxSkyMode::SKYX      ; }
    else                              { return GfxSkyMode::SANDSTORM ; } 
}

const char* IoInputGrabModeToStr(IoInputGrabMode v)
{
    switch (v)
    {
    case IoInputGrabMode::DYNAMIC: return CONF_INPUT_GRAB_DYNAMIC;
    case IoInputGrabMode::NONE   : return CONF_INPUT_GRAB_NONE;
    case IoInputGrabMode::ALL    : return CONF_INPUT_GRAB_ALL;
    default                      : return "";
    }
}

const char* GfxShadowTypeToStr(GfxShadowType v)
{
    switch (v)
    {
    case GfxShadowType::PSSM   : return CONF_GFX_SHADOW_PSSM;
    case GfxShadowType::NONE   : return CONF_GFX_SHADOW_NONE;
    default                    : return "";
    }
}

const char* GfxExtCamModeToStr(GfxExtCamMode v)
{
    switch (v)
    {
    case GfxExtCamMode::PITCHING: return CONF_EXTCAM_PITCHING;
    case GfxExtCamMode::STATIC  : return CONF_EXTCAM_STATIC;
    case GfxExtCamMode::NONE    : return CONF_EXTCAM_NONE;
    default                     : return "";
    }
}

const char* GfxTexFilterToStr(GfxTexFilter v)
{
    switch (v)
    {
    case GfxTexFilter::NONE       : return CONF_TEXFILTER_NONE;
    case GfxTexFilter::BILINEAR   : return CONF_TEXFILTER_BILI;
    case GfxTexFilter::TRILINEAR  : return CONF_TEXFILTER_TRILI;
    case GfxTexFilter::ANISOTROPIC: return CONF_TEXFILTER_ANISO;
    default                       : return "";
    }
}

const char* GfxVegetationToStr(GfxVegetation v)
{
    switch (v)
    {
    case GfxVegetation::NONE   : return CONF_VEGET_NONE;
    case GfxVegetation::x20PERC: return CONF_VEGET_20PERC;
    case GfxVegetation::x50PERC: return CONF_VEGET_50PERC;
    case GfxVegetation::FULL   : return CONF_VEGET_FULL;
    default                    : return "";
    }
}

const char* SimGearboxModeToStr(SimGearboxMode v)
{
    switch (v)
    {
    case SimGearboxMode::AUTO         : return CONF_GEARBOX_AUTO;
    case SimGearboxMode::SEMI_AUTO    : return CONF_GEARBOX_SEMIAUTO;
    case SimGearboxMode::MANUAL       : return CONF_GEARBOX_MANUAL;
    case SimGearboxMode::MANUAL_STICK : return CONF_GEARBOX_MAN_STICK;
    case SimGearboxMode::MANUAL_RANGES: return CONF_GEARBOX_MAN_RANGES;
    default                           : return "";
    }
}

const char* GfxFlaresModeToStr(GfxFlaresMode v)
{
    switch(v)
    {
    case GfxFlaresMode::NONE                   : return CONF_FLARES_NONE;
    case GfxFlaresMode::NO_LIGHTSOURCES        : return CONF_FLARES_NO_LIGHT;
    case GfxFlaresMode::CURR_VEHICLE_HEAD_ONLY : return CONF_FLARES_CURR_HEAD;
    case GfxFlaresMode::ALL_VEHICLES_HEAD_ONLY : return CONF_FLARES_ALL_HEADS;
    case GfxFlaresMode::ALL_VEHICLES_ALL_LIGHTS: return CONF_FLARES_ALL_LIGHTS;
    default                                    : return "";
    }
}

const char* GfxWaterModeToStr(GfxWaterMode v)
{
    switch(v)
    {
    case GfxWaterMode::BASIC    : return CONF_WATER_BASIC;
    case GfxWaterMode::REFLECT  : return CONF_WATER_REFLECT;
    case GfxWaterMode::FULL_FAST: return CONF_WATER_FULL_FAST;
    case GfxWaterMode::FULL_HQ  : return CONF_WATER_FULL_HQ;
    case GfxWaterMode::HYDRAX   : return CONF_WATER_HYDRAX;
    default                     : return "";
    }
}

const char* GfxSkyModeToStr(GfxSkyMode v)
{
    switch(v)
    {
    case GfxSkyMode::CAELUM   : return CONF_SKY_CAELUM;
    case GfxSkyMode::SKYX     : return CONF_SKY_SKYX;
    case GfxSkyMode::SANDSTORM: return CONF_SKY_SANDSTORM;
    default                   : return "";
    }
}

} // namespace RoR
