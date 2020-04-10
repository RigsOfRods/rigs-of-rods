/*
    This source file is part of Rigs of Rods
    Copyright 2013-2017 Petr Ohlidal

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
 GVarEnum_AP<AppState>    app_state               ("app_state",               nullptr,                     AppState::BOOTSTRAP,     AppState::MAIN_MENU);
 GVarStr_A<50>            app_language            ("app_language",            "Language",                  "en");
 GVarStr_A<50>            app_country             ("app_country",             "Country",                   "us");
 GVarPod_A<bool>          app_skip_main_menu      ("app_skip_main_menu",      "SkipMainMenu",              false);
 GVarPod_APS<bool>        app_async_physics       ("app_async_physics",       "AsyncPhysics",              true,
         true,      true);
 GVarPod_APS<int>         app_num_workers         ("app_num_workers",         "NumWorkerThreads",          0,
         0,         0);
 GVarStr_AP<50>           app_screenshot_format   ("app_screenshot_format",   "Screenshot Format",         "png",                   "png");
 GVarStr_A<100>           app_rendersys_override  ("app_rendersys_override",  "Render system",             "");
 GVarStr_A<300>           app_extra_mod_path      ("app_extra_mod_path",      "Extra mod path",            "");
 GVarPod_A<bool>          app_force_cache_purge   ("app_force_cache_purge",   nullptr,                     false);
 GVarPod_A<bool>          app_force_cache_udpate  ("app_force_cache_udpate",  nullptr,                     false);
 GVarPod_A<bool>          app_disable_online_api  ("app_disable_online_api",  "Disable Online API",        false);


// Simulation
 GVarEnum_AP<SimState>    sim_state               ("sim_state",               nullptr,                     SimState::OFF,           SimState::OFF);
 GVarStr_AP<200>          sim_terrain_name        ("sim_terrain_name",        nullptr,                     "",                      "");
 GVarStr_AP<300>          sim_terrain_gui_name    ("sim_terrain_gui_name",    nullptr,                     "",                      "");
 GVarStr_A<100>           sim_savegame            ("sim_savegame",            nullptr,                     "");
 GVarPod_A<bool>          sim_load_savegame       ("sim_load_savegame",       nullptr,                     false);
 GVarPod_A<bool>          sim_spawn_running       ("sim_spawn_running",       "Engines spawn running",     true);
 GVarPod_A<bool>          sim_replay_enabled      ("sim_replay_enabled",      "Replay mode",               false);
 GVarPod_A<int>           sim_replay_length       ("sim_replay_length",       "Replay length",             200);
 GVarPod_A<int>           sim_replay_stepping     ("sim_replay_stepping",     "Replay Steps per second",   1000);
 GVarPod_A<bool>          sim_realistic_commands  ("sim_realistic_commands",  "Realistic forward commands",false);
 GVarPod_A<bool>          sim_races_enabled       ("sim_races_enabled",       "Races",                     true);
 GVarPod_A<bool>          sim_no_collisions       ("sim_no_collisions",       "DisableCollisions",         false);
 GVarPod_A<bool>          sim_no_self_collisions  ("sim_no_self_collisions",  "DisableSelfCollisions",     false);
 GVarEnum_AP<SimGearboxMode> sim_gearbox_mode     ("sim_gearbox_mode",        "GearboxMode",               SimGearboxMode::AUTO,    SimGearboxMode::AUTO);

// Multiplayer
 GVarEnum_AP<MpState>     mp_state                ("mp_state",                nullptr,                     MpState::DISABLED,       MpState::DISABLED);
 GVarPod_APS<bool>        mp_join_on_startup      ("mp_join_on_startup",      "Auto connect",              false,                   false,      false); 
 GVarPod_A<bool>          mp_chat_auto_hide       ("mp_chat_auto_hide",       "Auto hide chat",            true);
 GVarPod_A<bool>          mp_hide_net_labels      ("mp_hide_net_labels",      "Hide net labels",           false);
 GVarPod_A<bool>          mp_hide_own_net_label   ("mp_hide_own_net_label",   "Hide own net label",        true);
 GVarPod_A<bool>          mp_pseudo_collisions    ("mp_pseudo_collisions",    "Multiplayer collisions",    false);
 GVarStr_AP<200>          mp_server_host          ("mp_server_host",          "Server name",               "",                      "");
 GVarPod_A<int>           mp_server_port          ("mp_server_port",          "Server port",               0);
 GVarStr_APS<100>         mp_server_password      ("mp_server_password",      "Server password",           "",                      "",         "");
 GVarStr_AP<100>          mp_player_name          ("mp_player_name",          "Nickname",                  "Player",                "Player");
 GVarStr_AP<100>          mp_player_token         ("mp_player_token",         "User Token",                "",                      "");
 GVarStr_A<100>           mp_api_url              ("mp_api_url",              "Online API URL",            "http://api.rigsofrods.org");

// Diagnostic
 GVarPod_A<bool>          diag_auto_spawner_report("diag_auto_spawner_report","AutoActorSpawnerReport",    false);
 GVarPod_A<bool>          diag_camera             ("diag_camera",            "Camera Debug",               false);
 GVarPod_A<bool>          diag_trace_globals      ("diag_trace_globals",      nullptr,                     false); // Don't init to 'true', logger is not ready at startup
 GVarPod_A<bool>          diag_rig_log_node_import("diag_rig_log_node_import","RigImporter_Debug_TraverseAndLogAllNodes",  false);
 GVarPod_A<bool>          diag_rig_log_node_stats ("diag_rig_log_node_stats", "RigImporter_PrintNodeStatsToLog",           false);
 GVarPod_A<bool>          diag_rig_log_messages   ("diag_rig_log_messages",   "RigImporter_PrintMessagesToLog",            false);
 GVarPod_A<bool>          diag_collisions         ("diag_collisions",         "Debug Collisions",          false);
 GVarPod_A<bool>          diag_truck_mass         ("diag_truck_mass",         "Debug Truck Mass",          false);
 GVarPod_A<bool>          diag_envmap             ("diag_envmap",             "EnvMapDebug",               false);
 GVarPod_A<bool>          diag_videocameras       ("diag_videocameras",       "VideoCameraDebug",          false);
 GVarStr_APS<100>         diag_preset_terrain     ("diag_preset_terrain",     "Preselected Terrain",       "",                      "",        "");
 GVarStr_A<100>           diag_preset_spawn_pos   ("diag_spawn_position",     nullptr,                     "");
 GVarStr_A<100>           diag_preset_spawn_rot   ("diag_spawn_rotation",     nullptr,                     "");
 GVarStr_APS<100>         diag_preset_vehicle     ("diag_preset_vehicle",     "Preselected Truck",         "",                      "",        "");
 GVarStr_A<100>           diag_preset_veh_config  ("diag_preset_veh_config",  "Preselected TruckConfig",   "");
 GVarPod_APS<bool>        diag_preset_veh_enter   ("diag_preset_veh_enter",   "Enter Preselected Truck",   false,                   false,     false);
 GVarPod_A<bool>          diag_log_console_echo   ("diag_log_console_echo",   "Enable Ingame Console",     false);
 GVarPod_A<bool>          diag_log_beam_break     ("diag_log_beam_break",     "Beam Break Debug",          false);
 GVarPod_A<bool>          diag_log_beam_deform    ("diag_log_beam_deform",    "Beam Deform Debug",         false);
 GVarPod_A<bool>          diag_log_beam_trigger   ("diag_log_beam_trigger",   "Trigger Debug",             false);
 GVarPod_A<bool>          diag_simple_materials   ("diag_simple_materials",    "SimpleMaterials",          false);
 GVarPod_A<bool>          diag_warning_texture    ("diag_warning_texture",    "Warning texture",           false);
 GVarPod_A<bool>          diag_hide_broken_beams  ("diag_hide_broken_beams",  "Hide broken beams",         false);
 GVarPod_A<bool>          diag_hide_beam_stress   ("diag_hide_beam_stress",   "Hide beam stress",          true);
 GVarPod_A<bool>          diag_hide_wheel_info    ("diag_hide_wheel_info",    "Hide wheel info",           true);
 GVarPod_A<bool>          diag_hide_wheels        ("diag_hide_wheels",        "Hide wheels",               false);
 GVarPod_A<bool>          diag_hide_nodes         ("diag_hide_nodes",         "Hide nodes",                false);
 GVarPod_A<float>         diag_physics_dt         ("diag_physics_dt",          "PhysicsTimeStep",          0.0005f);

// System                                         (all paths are without ending slash!)
 GVarStr_A<300>           sys_process_dir         ("sys_process_dir",         nullptr,                     "");
 GVarStr_A<300>           sys_user_dir            ("sys_user_dir",            nullptr,                     "");
 GVarStr_A<300>           sys_config_dir          ("sys_config_dir",          "Config Root",               "");
 GVarStr_A<300>           sys_cache_dir           ("sys_cache_dir",           "Cache Path",                "");
 GVarStr_A<300>           sys_logs_dir            ("sys_logs_dir",            "Log Path",                  "");
 GVarStr_A<300>           sys_resources_dir       ("sys_resources_dir",       "Resources Path",            "");
 GVarStr_A<300>           sys_profiler_dir        ("sys_profiler_dir",        "Profiler output dir",       "");
 GVarStr_A<300>           sys_savegames_dir       ("sys_savegames_dir",       nullptr,                     "");
 GVarStr_A<300>           sys_screenshot_dir      ("sys_screenshot_dir",      nullptr,                     "");

// Input - Output
 GVarPod_A<float>         io_analog_smoothing     ("io_analog_smoothing",     "Analog Input Smoothing",    1.f);
 GVarPod_A<float>         io_analog_sensitivity   ("io_analog_sensitivity",   "Analog Input Sensitivity",  1.f);
 GVarPod_A<bool>          io_ffb_enabled          ("io_ffb_enabled",          "Force Feedback",            false);
 GVarPod_A<float>         io_ffb_camera_gain      ("io_ffb_camera_gain",      "Force Feedback Camera",     0.f);
 GVarPod_A<float>         io_ffb_center_gain      ("io_ffb_center_gain",      "Force Feedback Centering",  0.f);
 GVarPod_A<float>         io_ffb_master_gain      ("io_ffb_master_gain",      "Force Feedback Gain",       0.f);
 GVarPod_A<float>             io_ffb_stress_gain  ("io_ffb_stress_gain",      "Force Feedback Stress",     0.f);
 GVarEnum_AP<IoInputGrabMode> io_input_grab_mode  ("io_input_grab_mode",      "Input Grab",                IoInputGrabMode::ALL,   IoInputGrabMode::ALL);
 GVarPod_A<bool>              io_arcade_controls  ("io_arcade_controls",      "ArcadeControls",            false);
 GVarPod_A<bool>              io_hydro_coupling  ("io_hydro_coupling",        "Keyboard Steering Speed Coupling",            true);
 GVarPod_A<int>           io_outgauge_mode        ("io_outgauge_mode",        "OutGauge Mode",             0); // 0 = disabled, 1 = enabled
 GVarStr_A<50>            io_outgauge_ip          ("io_outgauge_ip",          "OutGauge IP",               "192.168.1.100");
 GVarPod_A<int>           io_outgauge_port        ("io_outgauge_port",        "OutGauge Port",             1337);
 GVarPod_A<float>         io_outgauge_delay       ("io_outgauge_delay",       "OutGauge Delay",            10.f);
 GVarPod_A<int>           io_outgauge_id          ("io_outgauge_id",          "OutGauge ID",               0);
 GVarPod_A<bool>          io_discord_rpc          ("io_discord_rpc",          "Discord Rich Presence",     true);

// Audio
 GVarPod_A<float>         audio_master_volume     ("audio_master_volume",     "Sound Volume",              1);
 GVarPod_A<bool>          audio_enable_creak      ("audio_enable_creak",      "Creak Sound",               false);
 GVarStr_AP<100>          audio_device_name       ("audio_device_name",       "AudioDevice",               "",                      "");
 GVarPod_A<bool>          audio_menu_music        ("audio_menu_music",        "MainMenuMusic",             false);

// Graphics
 GVarEnum_AP<GfxFlaresMode>  gfx_flares_mode      ("gfx_flares_mode",         "Lights",                    GfxFlaresMode::ALL_VEHICLES_ALL_LIGHTS, GfxFlaresMode::ALL_VEHICLES_ALL_LIGHTS);
 GVarEnum_AP<GfxShadowType>  gfx_shadow_type      ("gfx_shadow_type",         "Shadow technique",          GfxShadowType::PSSM,     GfxShadowType::PSSM);
 GVarEnum_AP<GfxExtCamMode>  gfx_extcam_mode      ("gfx_extcam_mode",         "External Camera Mode",      GfxExtCamMode::PITCHING, GfxExtCamMode::PITCHING);
 GVarEnum_AP<GfxSkyMode>     gfx_sky_mode         ("gfx_sky_mode",            "Sky effects",               GfxSkyMode::CAELUM,   GfxSkyMode::CAELUM);
 GVarEnum_AP<GfxTexFilter>   gfx_texture_filter   ("gfx_texture_filter",      "Texture Filtering",         GfxTexFilter::ANISOTROPIC, GfxTexFilter::ANISOTROPIC);
 GVarEnum_AP<GfxVegetation>  gfx_vegetation_mode  ("gfx_vegetation_mode",     "Vegetation",                GfxVegetation::FULL,     GfxVegetation::FULL);
 GVarEnum_AP<GfxWaterMode>   gfx_water_mode       ("gfx_water_mode",          "Water effects",             GfxWaterMode::FULL_FAST,     GfxWaterMode::FULL_FAST);
 GVarPod_A<int>           gfx_anisotropy          ("gfx_anisotropy",          "Anisotropy",                4);
 GVarPod_A<bool>          gfx_water_waves         ("gfx_water_waves",         "Waves",                     false);
 GVarPod_A<int>           gfx_particles_mode      ("gfx_particles_mode",      "Particles",                 0);
 GVarPod_A<bool>          gfx_enable_videocams    ("gfx_enable_videocams",    "gfx_enable_videocams",      false);
 GVarPod_A<bool>          gfx_window_videocams    ("gfx_window_videocams",    "UseVideocameraWindows",     false);
 GVarPod_APS<bool>        gfx_surveymap_icons     ("gfx_surveymap_icons",     "Overview map icons",        true,                    true,   true);
 GVarPod_A<bool>          gfx_declutter_map       ("gfx_declutter_map",       "Declutter overview map",    true);
 GVarPod_A<bool>          gfx_envmap_enabled      ("gfx_envmap_enabled",      "Reflections",               true);
 GVarPod_A<int>           gfx_envmap_rate         ("gfx_envmap_rate",         "ReflectionUpdateRate",      1);
 GVarPod_A<int>           gfx_shadow_quality      ("gfx_shadow_quality",      "Shadows Quality",           2);
 GVarPod_A<int>           gfx_skidmarks_mode      ("gfx_skidmarks_mode",      "Skidmarks",                 0);
 GVarPod_A<int>           gfx_sight_range         ("gfx_sight_range",         "SightRange",                5000); // Previously either 2000 or 4500 (inconsistent)
 GVarPod_A<int>           gfx_camera_height       ("gfx_camera_height",       "Static camera height",      5);
 GVarPod_APS<int>         gfx_fov_external        ("gfx_fov_external",        "FOV External",              60,                      60,     60);
 GVarPod_APS<int>         gfx_fov_internal        ("gfx_fov_internal",        "FOV Internal",              75,                      75,     75);
 GVarPod_A<float>         gfx_static_cam_fov_exp  ("gfx_static_cam_fov_exp",  nullptr,                     1.f);
 GVarPod_A<bool>          gfx_fixed_cam_tracking  ("gfx_fixed_cam_tracking",  nullptr,                     false);
 GVarPod_A<int>           gfx_fps_limit           ("gfx_fps_limit",           "FPS-Limiter",               0); // 0 = unlimited
 GVarPod_A<bool>          gfx_speedo_digital      ("gfx_speedo_digital",      "DigitalSpeedo",             true);
 GVarPod_A<bool>          gfx_speedo_imperial     ("gfx_speedo_imperial",     "gfx_speedo_imperial",       false);
 GVarPod_A<bool>          gfx_flexbody_lods       ("gfx_flexbody_lods",       "Flexbody_EnableLODs",       false);
 GVarPod_A<bool>          gfx_flexbody_cache      ("gfx_flexbody_cache",      "Flexbody_UseCache",         false);
 GVarPod_A<bool>          gfx_reduce_shadows      ("gfx_reduce_shadows",      "Shadow optimizations",      true);
 GVarPod_A<bool>          gfx_enable_rtshaders    ("gfx_enable_rtshaders",    "Use RTShader System",       false);
 GVarPod_A<bool>          gfx_classic_shaders     ("gfx_classic_shaders",      "Classic material shaders",       false);

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

const char* EnumToStr(AppState v)
{
    switch (v)
    {
    case AppState::BOOTSTRAP:           return "BOOTSTRAP";
    case AppState::MAIN_MENU:           return "MAIN_MENU";
    case AppState::PRINT_HELP_EXIT:     return "PRINT_HELP_EXIT";
    case AppState::PRINT_VERSION_EXIT:  return "PRINT_VERSION_EXIT";
    case AppState::SHUTDOWN:            return "SHUTDOWN";
    case AppState::SIMULATION:          return "SIMULATION";
    default:                            return "~invalid~";
    }
}

const char* EnumToStr(MpState v)
{
    switch (v)
    {
    case MpState::DISABLED:  return "DISABLED";
    case MpState::CONNECTED: return "CONNECTED";
    default:                 return "~invalid~";
    }
}

const char* EnumToStr(SimState v)
{
    switch (v)
    {
    case SimState::OFF        : return "OFF";
    case SimState::RUNNING    : return "RUNNING";
    case SimState::PAUSED     : return "PAUSED";
    case SimState::EDITOR_MODE: return "EDITOR_MODE";
    default                   : return "~invalid~";
    }
}

const char* EnumToStr(SimGearboxMode v)
{
    switch (v)
    {
    case SimGearboxMode::AUTO         : return "AUTO";
    case SimGearboxMode::SEMI_AUTO    : return "SEMI_AUTO";
    case SimGearboxMode::MANUAL       : return "MANUAL";
    case SimGearboxMode::MANUAL_STICK : return "MANUAL_STICK";
    case SimGearboxMode::MANUAL_RANGES: return "MANUAL_RANGES";
    default                           : return "~invalid~";
    }
}

const char* EnumToStr(GfxFlaresMode v)
{
    switch (v)
    {
    case GfxFlaresMode::NONE                   : return "NONE";
    case GfxFlaresMode::NO_LIGHTSOURCES        : return "NO_LIGHTSOURCES";
    case GfxFlaresMode::CURR_VEHICLE_HEAD_ONLY : return "CURR_VEHICLE_HEAD_ONLY";
    case GfxFlaresMode::ALL_VEHICLES_HEAD_ONLY : return "ALL_VEHICLES_HEAD_ONLY";
    case GfxFlaresMode::ALL_VEHICLES_ALL_LIGHTS: return "ALL_VEHICLES_ALL_LIGHTS";
    default                                    : return "~invalid~";
    }
}

const char* EnumToStr(GfxVegetation v)
{
    switch(v)
    {
    case GfxVegetation::NONE    : return "NONE";
    case GfxVegetation::x20PERC : return "20%";
    case GfxVegetation::x50PERC : return "50%";
    case GfxVegetation::FULL    : return "FULL";
    default                     : return "~invalid~";
    }
}

const char* EnumToStr(GfxWaterMode v)
{
    switch(v)
    {
    case GfxWaterMode::NONE      : return "NONE";
    case GfxWaterMode::BASIC     : return "BASIC";
    case GfxWaterMode::REFLECT   : return "REFLECT";
    case GfxWaterMode::FULL_FAST : return "FULL_FAST";
    case GfxWaterMode::FULL_HQ   : return "FULL_HQ";
    case GfxWaterMode::HYDRAX    : return "HYDRAX";
    default                      : return "~invalid~";
    }
}

const char* EnumToStr(GfxSkyMode v)
{
    switch(v)
    {
    case GfxSkyMode::SANDSTORM: return "SANDSTORM";
    case GfxSkyMode::CAELUM   : return "CAELUM";
    case GfxSkyMode::SKYX     : return "SKYX";
    default                   : return "~invalid~";
    }
}

const char* EnumToStr(IoInputGrabMode v)
{
    switch (v)
    {
    case IoInputGrabMode::NONE   : return "NONE";
    case IoInputGrabMode::ALL    : return "ALL";
    case IoInputGrabMode::DYNAMIC: return "DYNAMIC";
    default                      : return "~invalid~";
    }
}

const char* EnumToStr(GfxShadowType v)
{
    switch(v)
    {
    case GfxShadowType::NONE   : return "NONE";
    case GfxShadowType::PSSM   : return "PSSM";
    default                    : return "~invalid~";
    }
}

const char* EnumToStr(GfxTexFilter v)
{
    switch (v)
    {
    case GfxTexFilter::NONE       : return "NONE";
    case GfxTexFilter::BILINEAR   : return "BILINEAR";
    case GfxTexFilter::TRILINEAR  : return "TRILINEAR";
    case GfxTexFilter::ANISOTROPIC: return "ANISOTROPIC";
    default                       : return "~invalid~";
    }
}

const char* EnumToStr(GfxExtCamMode v)
{
    switch (v)
    {
    case GfxExtCamMode::NONE:     return "NONE";
    case GfxExtCamMode::STATIC:   return "STATIC";
    case GfxExtCamMode::PITCHING: return "PITCHING";
    default:                      return "~invalid~";
    }
}

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

const char* GVarBase::LOG_FMT_S = "[RoR|GVar]  %20s:  %s(), new: \"%s\", old: \"%s\"";
const char* GVarBase::LOG_FMT_D = "[RoR|GVar]  %20s:  %s(), new: \"%d\", old: \"%d\"";
const char* GVarBase::LOG_FMT_F = "[RoR|GVar]  %20s:  %s(), new: \"%f\", old: \"%f\"";

void GVarBase::LogFormat(const char* format, ...) const
{
    if (! App::diag_trace_globals.GetActive())
    {
        return;
    }

    char buffer[2000] = {};

    va_list args;
    va_start(args, format);
        vsprintf(buffer, format, args);
    va_end(args);

    RoR::Log(buffer);
}

} // namespace RoR
