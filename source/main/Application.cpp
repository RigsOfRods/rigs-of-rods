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

#include "CacheSystem.h"
#include "Console.h"
#include "ContentManager.h"
#include "GUIManager.h"
#include "InputEngine.h"
#include "OgreSubsystem.h"
#include "OverlayWrapper.h"
#include "MumbleIntegration.h"

namespace RoR {
namespace App {

// ================================================================================
// Global variables
// ================================================================================


// Object instances
static OgreSubsystem*   g_ogre_subsystem;
static Console          g_console;
static ContentManager   g_content_manager;
static OverlayWrapper*  g_overlay_wrapper;
static GUIManager*      g_gui_manager;
static InputEngine*     g_input_engine;
static CacheSystem*     g_cache_system;
static MainMenu*        g_main_menu;
static SimController*   g_sim_controller;
static MumbleIntegration* g_mumble;
static TerrainManager*  g_sim_terrain;

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
CVar* app_force_cache_udpate;
CVar* app_disable_online_api;
CVar* app_config_long_names;


// Simulation
 CVar* sim_state;
 CVar* sim_terrain_name;
 CVar* sim_terrain_gui_name;
 CVar* sim_savegame;
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
CVar* diag_import_grp_loose;

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
CVar* sys_projects_dir;

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
CVar* gfx_fov_internal;
CVar* gfx_static_cam_fov_exp;
CVar* gfx_fixed_cam_tracking;
CVar* gfx_fps_limit;
CVar* gfx_speedo_digital;
CVar* gfx_speedo_imperial;
CVar* gfx_flexbody_lods;
CVar* gfx_flexbody_cache;
CVar* gfx_reduce_shadows;
CVar* gfx_enable_rtshaders;
CVar* gfx_classic_shaders;

// Instance management
void SetMainMenu       (MainMenu* obj)                { g_main_menu = obj; }
void SetSimController  (SimController* obj)           { g_sim_controller = obj;}
void SetSimTerrain     (TerrainManager* obj)          { g_sim_terrain = obj;}
void SetCacheSystem    (CacheSystem* obj)             { g_cache_system = obj; }

// Instance access
OgreSubsystem*         GetOgreSubsystem      () { return g_ogre_subsystem; };
ContentManager*        GetContentManager     () { return &g_content_manager;}
OverlayWrapper*        GetOverlayWrapper     () { return g_overlay_wrapper;}
GUIManager*            GetGuiManager         () { return g_gui_manager;}
Console*               GetConsole            () { return &g_console;}
InputEngine*           GetInputEngine        () { return g_input_engine;}
CacheSystem*           GetCacheSystem        () { return g_cache_system;}
MainMenu*              GetMainMenu           () { return g_main_menu;}
SimController*         GetSimController      () { return g_sim_controller; }
MumbleIntegration*     GetMumble             () { return g_mumble; }
TerrainManager*        GetSimTerrain         () { return g_sim_terrain; }

void StartOgreSubsystem()
{
    g_ogre_subsystem = new OgreSubsystem();
    if (g_ogre_subsystem == nullptr)
    {
        throw std::runtime_error("[RoR] Failed to create OgreSubsystem");
    }

    if (! g_ogre_subsystem->StartOgre("", ""))
    {
        throw std::runtime_error("[RoR] Failed to start up OGRE 3D engine");
    }
}

void ShutdownOgreSubsystem()
{
    assert(g_ogre_subsystem != nullptr && "ShutdownOgreSubsystem(): Ogre subsystem was not started");
    delete g_ogre_subsystem;
    g_ogre_subsystem = nullptr;
}

void CreateOverlayWrapper()
{
    g_overlay_wrapper = new OverlayWrapper();
    if (g_overlay_wrapper == nullptr)
    {
        throw std::runtime_error("[RoR] Failed to create OverlayWrapper");
    }
}

void DestroyOverlayWrapper()
{
    delete g_overlay_wrapper; // deleting nullptr does nothing
    g_overlay_wrapper = nullptr;
}

void CreateGuiManagerIfNotExists()
{
    if (g_gui_manager == nullptr)
    {
        g_gui_manager = new GUIManager();
    }
}

void DeleteGuiManagerIfExists()
{
    if (g_gui_manager != nullptr)
    {
        delete g_gui_manager;
        g_gui_manager = nullptr;
    }
}

void CreateInputEngine()
{
    assert(g_input_engine == nullptr);
    g_input_engine = new InputEngine();
}


void CheckAndCreateMumble()
{
#ifdef USE_MUMBLE // The class is always forward-declared but only defined if USE_MUMBLE is defined
    if (g_mumble == nullptr)
        g_mumble = new MumbleIntegration();
#endif // USE_MUMBLE
}

} // namespace App

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

// --------------------- Config enums ---------------------

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
