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


/// @file   Application.h
/// @author Petr Ohlidal
/// @date   05/2014
/// @brief  Central state/object manager and communications hub.

#pragma once

#include "CVar.h"
#include "ForwardDeclarations.h"
#include "Str.h"

#include <string>

namespace RoR {

// ------------------------------------------------------------------------------------------------
// Global definitions and enumerations
// ------------------------------------------------------------------------------------------------

enum class AppState
{
    BOOTSTRAP,          ///< Initial state
    MAIN_MENU,
    SIMULATION,
    SHUTDOWN,
    PRINT_HELP_EXIT,
    PRINT_VERSION_EXIT,
};

enum class MpState
{
    DISABLED,  ///< Not connected for whatever reason.
    CONNECTING,
    CONNECTED,
};

enum class SimState
{
    OFF,
    RUNNING,
    PAUSED,
    EDITOR_MODE ///< Hacky, but whatever... added by Ulteq, 2016
};

enum class SimGearboxMode
{
    AUTO,          ///< Automatic shift
    SEMI_AUTO,     ///< Manual shift - Auto clutch
    MANUAL,        ///< Fully Manual: sequential shift
    MANUAL_STICK,  ///< Fully manual: stick shift
    MANUAL_RANGES, ///< Fully Manual: stick shift with ranges
};
SimGearboxMode ParseSimGearboxMode(std::string const & s);
const char* SimGearboxModeToStr(SimGearboxMode v);

enum class GfxShadowType
{
    NONE,
    PSSM
};
GfxShadowType ParseGfxShadowType(std::string const & s);
const char* GfxShadowTypeToStr(GfxShadowType v);

enum class GfxExtCamMode
{
    NONE,
    STATIC,
    PITCHING,
};
GfxExtCamMode ParseGfxExtCamMode(std::string const & s);
const char* GfxExtCamModeToStr(GfxExtCamMode v);

enum class GfxTexFilter
{
    NONE,
    BILINEAR,
    TRILINEAR,
    ANISOTROPIC,
};
GfxTexFilter ParseGfxTexFilter(std::string const & s);
const char* GfxTexFilterToStr(GfxTexFilter v);

enum class GfxVegetation
{
    NONE,
    x20PERC,
    x50PERC,
    FULL,
};
GfxVegetation ParseGfxVegetation(std::string const & s);
const char* GfxVegetationToStr(GfxVegetation v);

enum class GfxFlaresMode
{
    NONE,                    ///< None (fastest)
    NO_LIGHTSOURCES,         ///< No light sources
    CURR_VEHICLE_HEAD_ONLY,  ///< Only current vehicle, main lights
    ALL_VEHICLES_HEAD_ONLY,  ///< All vehicles, main lights
    ALL_VEHICLES_ALL_LIGHTS, ///< All vehicles, all lights
};
GfxFlaresMode ParseGfxFlaresMode(std::string const & s);
const char* GfxFlaresModeToStr(GfxFlaresMode v);

enum class GfxWaterMode
{
    NONE,       ///< None
    BASIC,      ///< Basic (fastest)
    REFLECT,    ///< Reflection
    FULL_FAST,  ///< Reflection + refraction (speed optimized)
    FULL_HQ,    ///< Reflection + refraction (quality optimized)
    HYDRAX,     ///< HydraX
};
GfxWaterMode ParseGfxWaterMode(std::string const & s);
const char* GfxWaterModeToStr(GfxWaterMode v);

enum class GfxSkyMode
{
    SANDSTORM,  ///< Sandstorm (fastest)
    CAELUM,     ///< Caelum (best looking, slower)
    SKYX,       ///< SkyX (best looking, slower)
};
GfxSkyMode ParseGfxSkyMode(std::string const & s);
const char* GfxSkyModeToStr(GfxSkyMode v);

enum class IoInputGrabMode
{
    NONE,
    ALL,
    DYNAMIC,
};
IoInputGrabMode ParseIoInputGrabMode(std::string const & s);
const char* IoInputGrabModeToStr(IoInputGrabMode v);

// ------------------------------------------------------------------------------------------------
// Global variables
// ------------------------------------------------------------------------------------------------

namespace App {

// App
extern CVar* app_state;
extern CVar* app_language;
extern CVar* app_country;
extern CVar* app_skip_main_menu;
extern CVar* app_async_physics;
extern CVar* app_num_workers;
extern CVar* app_screenshot_format;
extern CVar* app_rendersys_override;
extern CVar* app_extra_mod_path;
extern CVar* app_force_cache_purge;
extern CVar* app_force_cache_udpate;
extern CVar* app_disable_online_api;
extern CVar* app_config_long_names;

// Simulation
extern CVar* sim_state;
extern CVar* sim_terrain_name;
extern CVar* sim_terrain_gui_name;
extern CVar* sim_savegame;
extern CVar* sim_spawn_running;
extern CVar* sim_replay_enabled;
extern CVar* sim_replay_length;
extern CVar* sim_replay_stepping;
extern CVar* sim_realistic_commands;
extern CVar* sim_races_enabled;
extern CVar* sim_no_collisions;
extern CVar* sim_no_self_collisions;
extern CVar* sim_gearbox_mode;

// Multiplayer
extern CVar* mp_state;
extern CVar* mp_join_on_startup;
extern CVar* mp_chat_auto_hide;
extern CVar* mp_hide_net_labels;
extern CVar* mp_hide_own_net_label;
extern CVar* mp_pseudo_collisions;
extern CVar* mp_server_host;
extern CVar* mp_server_port;
extern CVar* mp_server_password;
extern CVar* mp_player_name;
extern CVar* mp_player_token;
extern CVar* mp_api_url;

// Diagnostic
extern CVar* diag_auto_spawner_report;
extern CVar* diag_camera;
extern CVar* diag_trace_globals;
extern CVar* diag_rig_log_node_import;
extern CVar* diag_rig_log_node_stats;
extern CVar* diag_collisions;
extern CVar* diag_truck_mass;
extern CVar* diag_envmap;
extern CVar* diag_videocameras;
extern CVar* diag_preset_terrain;
extern CVar* diag_preset_spawn_pos;
extern CVar* diag_preset_spawn_rot;
extern CVar* diag_preset_vehicle;
extern CVar* diag_preset_veh_config;
extern CVar* diag_preset_veh_enter;
extern CVar* diag_log_console_echo;
extern CVar* diag_log_beam_break;
extern CVar* diag_log_beam_deform;
extern CVar* diag_log_beam_trigger;
extern CVar* diag_simple_materials;
extern CVar* diag_warning_texture;
extern CVar* diag_hide_broken_beams;
extern CVar* diag_hide_beam_stress;
extern CVar* diag_hide_wheel_info;
extern CVar* diag_hide_wheels;
extern CVar* diag_hide_nodes;
extern CVar* diag_physics_dt;
extern CVar* diag_import_grp_loose;

// System
extern CVar* sys_process_dir;
extern CVar* sys_user_dir;
extern CVar* sys_config_dir;
extern CVar* sys_cache_dir;
extern CVar* sys_logs_dir;
extern CVar* sys_resources_dir;
extern CVar* sys_profiler_dir;
extern CVar* sys_savegames_dir;
extern CVar* sys_screenshot_dir;
extern CVar* sys_projects_dir;

// Input - Output
extern CVar* io_analog_smoothing;
extern CVar* io_analog_sensitivity;
extern CVar* io_blink_lock_range;
extern CVar* io_ffb_enabled;
extern CVar* io_ffb_camera_gain;
extern CVar* io_ffb_center_gain;
extern CVar* io_ffb_master_gain;
extern CVar* io_ffb_stress_gain;
extern CVar* io_input_grab_mode;
extern CVar* io_arcade_controls;
extern CVar* io_hydro_coupling;
extern CVar* io_outgauge_mode;
extern CVar* io_outgauge_ip;
extern CVar* io_outgauge_port;
extern CVar* io_outgauge_delay;
extern CVar* io_outgauge_id;
extern CVar* io_discord_rpc;

// Audio
extern CVar* audio_master_volume;
extern CVar* audio_enable_creak;
extern CVar* audio_device_name;
extern CVar* audio_menu_music;

// Graphics
extern CVar* gfx_flares_mode;
extern CVar* gfx_shadow_type;
extern CVar* gfx_extcam_mode;
extern CVar* gfx_sky_mode;
extern CVar* gfx_texture_filter;
extern CVar* gfx_vegetation_mode;
extern CVar* gfx_water_mode;
extern CVar* gfx_anisotropy;
extern CVar* gfx_water_waves;
extern CVar* gfx_particles_mode;
extern CVar* gfx_enable_videocams;
extern CVar* gfx_window_videocams;
extern CVar* gfx_surveymap_icons;
extern CVar* gfx_declutter_map;
extern CVar* gfx_envmap_enabled;
extern CVar* gfx_envmap_rate;
extern CVar* gfx_shadow_quality;
extern CVar* gfx_skidmarks_mode;
extern CVar* gfx_sight_range;
extern CVar* gfx_camera_height;
extern CVar* gfx_fov_external;
extern CVar* gfx_fov_internal;
extern CVar* gfx_static_cam_fov_exp;
extern CVar* gfx_fixed_cam_tracking;
extern CVar* gfx_fps_limit;
extern CVar* gfx_speedo_digital;
extern CVar* gfx_speedo_imperial;
extern CVar* gfx_flexbody_lods;
extern CVar* gfx_flexbody_cache;
extern CVar* gfx_reduce_shadows;
extern CVar* gfx_enable_rtshaders;
extern CVar* gfx_classic_shaders;

// ------------------------------------------------------------------------------------------------
// Global objects
// ------------------------------------------------------------------------------------------------

// Getters
OgreSubsystem*       GetOgreSubsystem();
ContentManager*      GetContentManager();
OverlayWrapper*      GetOverlayWrapper();
GUIManager*          GetGuiManager();
Console*             GetConsole();
InputEngine*         GetInputEngine();
CacheSystem*         GetCacheSystem();
MainMenu*            GetMainMenu();
SimController*       GetSimController();
MumbleIntegration*   GetMumble();
TerrainManager*      GetSimTerrain();

// Factories
void StartOgreSubsystem();
void ShutdownOgreSubsystem();
void CreateOverlayWrapper();
void DestroyOverlayWrapper();
void CreateGuiManagerIfNotExists();
void DeleteGuiManagerIfExists();
void CreateInputEngine();
void CheckAndCreateMumble();

// Setters
void SetMainMenu             (MainMenu*          obj);
void SetSimController        (SimController*     obj);
void SetSimTerrain           (TerrainManager*    obj);
void SetCacheSystem          (CacheSystem*       obj);

} // namespace App

// ------------------------------------------------------------------------------------------------
// Logging
// ------------------------------------------------------------------------------------------------

void          Log(const char* msg);               ///< The ultimate, application-wide logging function. Adds a line (any length) in 'RoR.log' file.
void          LogFormat(const char* format, ...); ///< Improved logging utility. Uses fixed 2Kb buffer.

} // namespace RoR

inline void   LOG(const char* msg)           { RoR::Log(msg); }         ///< Legacy alias - formerly a macro
inline void   LOG(std::string const & msg)   { RoR::Log(msg.c_str()); } ///< Legacy alias - formerly a macro

