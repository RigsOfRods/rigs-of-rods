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

#include <OgreException.h>

#include "CacheSystem.h"
#include "ContentManager.h"
#include "GUIManager.h"
#include "InputEngine.h"
#include "OgreSubsystem.h"
#include "OverlayWrapper.h"
#include "SceneMouse.h"
#include "MumbleIntegration.h"

namespace RoR {
namespace App {

// ================================================================================
// Global variables
// ================================================================================


// Object instances
static OgreSubsystem*   g_ogre_subsystem;
static ContentManager*  g_content_manager;
static OverlayWrapper*  g_overlay_wrapper;
static SceneMouse*      g_scene_mouse;
static GUIManager*      g_gui_manager;
static Console*         g_console;
static InputEngine*     g_input_engine;
static CacheSystem*     g_cache_system;
static MainMenu*        g_main_menu;
static SimController*   g_sim_controller;
static MumbleIntegration* g_mumble;
static TerrainManager*  g_sim_terrain;

// App
 GVar_AP<AppState>      app_state               ("app_state",               nullptr,                     AppState::BOOTSTRAP,     AppState::MAIN_MENU);
 GVar_A<std::string>    app_language            ("app_language",            "Language",                  "English");
 GVar_A<std::string>    app_locale              ("app_locale",              "Language Short",            "en");
 GVar_A<bool>           app_multithread         ("app_multithread",         "Multi-threading",           true);
 GVar_AP<std::string>   app_screenshot_format   ("app_screenshot_format",   "Screenshot Format",         "jpg",                   "jpg");

// Simulation
 GVar_AP<SimState>      sim_state               ("sim_state",               nullptr,                     SimState::OFF,           SimState::OFF);
 GVar_AP<std::string>   sim_terrain_name        ("sim_terrain_name",        nullptr,                     "",                      "");
 GVar_A<bool>           sim_replay_enabled      ("sim_replay_enabled",      "Replay mode",               false);
 GVar_A<int>            sim_replay_length       ("sim_replay_length",       "Replay length",             200);
 GVar_A<int>            sim_replay_stepping     ("sim_replay_stepping",     "Replay Steps per second",   1000);
 GVar_A<bool>           sim_hires_wheel_col     ("sim_hires_wheel_col",     "HighResWheelNodeCollisions", true);
 GVar_A<bool>           sim_position_storage    ("sim_position_storage",    "Position Storage",          false);
 GVar_AP<SimGearboxMode> sim_gearbox_mode       ("sim_gearbox_mode",        "GearboxMode",               SimGearboxMode::AUTO,    SimGearboxMode::AUTO);

// Multiplayer
 GVar_AP<MpState>       mp_state                ("mp_state",                nullptr,                     MpState::DISABLED,       MpState::DISABLED);
 GVar_A<bool>           mp_join_on_startup      ("mp_join_on_startup",      "Network enable",            false);
 GVar_AP<std::string>   mp_server_host          ("mp_server_host",          "Server name",               "",                      "");
 GVar_A<int>            mp_server_port          ("mp_server_port",          "Server port",               1337);
 GVar_A<std::string>    mp_server_password      ("mp_server_password",      "Server password",           "");
 GVar_AP<std::string>   mp_player_name          ("mp_player_name",          "Nickname",                  "Player",                "Player");
 GVar_AP<std::string>   mp_player_token_hash    ("mp_player_token_hash",    "User Token Hash",           "",                      "");
 GVar_AP<std::string>   mp_portal_url           ("mp_portal_url",           "Multiplayer portal URL",    "http://multiplayer.rigsofrods.org", "http://multiplayer.rigsofrods.org");

// Diagnostic
 GVar_A<bool>           diag_trace_globals      ("diag_trace_globals",      nullptr,                     false); // Don't init to 'true', logger is not ready at startup
 GVar_A<bool>           diag_rig_log_node_import("diag_rig_log_node_import","RigImporter_Debug_TraverseAndLogAllNodes",  false);
 GVar_A<bool>           diag_rig_log_node_stats ("diag_rig_log_node_stats", "RigImporter_PrintNodeStatsToLog",           false);
 GVar_A<bool>           diag_rig_log_messages   ("diag_rig_log_messages",   "RigImporter_PrintMessagesToLog",            false);
 GVar_A<bool>           diag_collisions         ("diag_collisions",         "Debug Collisions",          false);
 GVar_A<bool>           diag_truck_mass         ("diag_truck_mass",         "Debug Truck Mass",          false);
 GVar_A<bool>           diag_envmap             ("diag_envmap",             "EnvMapDebug",               false);
 GVar_A<bool>           diag_videocameras       ("diag_videocameras",       "VideoCameraDebug",          false);
 GVar_APS<std::string>  diag_preset_terrain     ("diag_preset_terrain",     "Preselected Map",           "",                      "",        "");
 GVar_A<std::string>    diag_preset_vehicle     ("diag_preset_vehicle",     "Preselected Truck",         "");
 GVar_A<std::string>    diag_preset_veh_config  ("diag_preset_veh_config",  "Preselected TruckConfig",   "");
 GVar_A<bool>           diag_preset_veh_enter   ("diag_preset_veh_enter",   "Enter Preselected Truck",   false);
 GVar_A<bool>           diag_log_console_echo   ("diag_log_console_echo",   "Enable Ingame Console",     false);
 GVar_A<bool>           diag_log_beam_break     ("diag_log_beam_break",     "Beam Break Debug",          false);
 GVar_A<bool>           diag_log_beam_deform    ("diag_log_beam_deform",    "Beam Deform Debug",         false);
 GVar_A<bool>           diag_log_beam_trigger   ("diag_log_beam_trigger",   "Trigger Debug",             false);
 GVar_A<bool>           diag_dof_effect         ("diag_dof_effect",         "DOFDebug",                  false);
 GVar_AP<std::string>   diag_extra_resource_dir ("diag_extra_resource_dir", "resourceIncludePath",       "",                     "");

// System                                         (all paths are without ending slash!)
 GVar_A<std::string>    sys_process_dir         ("sys_process_dir",         nullptr,                     "");
 GVar_A<std::string>    sys_user_dir            ("sys_user_dir",            nullptr,                     "");
 GVar_A<std::string>    sys_config_dir          ("sys_config_dir",          "Config Root",               "");
 GVar_A<std::string>    sys_cache_dir           ("sys_cache_dir",           "Cache Path",                "");
 GVar_A<std::string>    sys_logs_dir            ("sys_logs_dir",            "Log Path",                  "");
 GVar_A<std::string>    sys_resources_dir       ("sys_resources_dir",       "Resources Path",            "");
 GVar_A<std::string>    sys_profiler_dir        ("sys_profiler_dir",        "Profiler output dir",       "");
 GVar_A<std::string>    sys_screenshot_dir      ("sys_screenshot_dir",      nullptr,                     "");

// Input - Output
 GVar_A<bool>           io_ffb_enabled          ("io_ffb_enabled",          "Force Feedback",            false);
 GVar_A<float>          io_ffb_camera_gain      ("io_ffb_camera_gain",      "Force Feedback Camera",     0.f);
 GVar_A<float>          io_ffb_center_gain      ("io_ffb_center_gain",      "Force Feedback Centering",  0.f);
 GVar_A<float>          io_ffb_master_gain      ("io_ffb_master_gain",      "Force Feedback Gain",       0.f);
 GVar_A<float>          io_ffb_stress_gain      ("io_ffb_stress_gain",      "Force Feedback Stress",     0.f);
 GVar_AP<IoInputGrabMode> io_input_grab_mode    ("io_input_grab_mode",      "Input Grab",                IoInputGrabMode::NONE,   IoInputGrabMode::NONE);
 GVar_A<bool>           io_arcade_controls      ("io_arcade_controls",      "ArcadeControls",            false);
 GVar_A<int>            io_outgauge_mode        ("io_outgauge_mode",        "OutGauge Mode",             0); // 0 = disabled, 1 = enabled
 GVar_A<std::string>    io_outgauge_ip          ("io_outgauge_ip",          "OutGauge IP",               "192.168.1.100");
 GVar_A<int>            io_outgauge_port        ("io_outgauge_port",        "OutGauge Port",             1337);
 GVar_A<float>          io_outgauge_delay       ("io_outgauge_delay",       "OutGauge Delay",            10.f);
 GVar_A<int>            io_outgauge_id          ("io_outgauge_id",          "OutGauge ID",               0);

// Audio
 GVar_A<float>          audio_master_volume     ("audio_master_volume",     "Sound Volume",              0);
 GVar_A<bool>           audio_enable_creak      ("audio_enable_creak",      "Creak Sound",               false);
 GVar_AP<std::string>   audio_device_name       ("audio_device_name",       "AudioDevice",               "",                      "");
 GVar_A<bool>           audio_menu_music        ("audio_menu_music",        "MainMenuMusic",             false);

// Graphics
 GVar_AP<GfxFlaresMode>   gfx_flares_mode      ("gfx_flares_mode",         "Lights",                    GfxFlaresMode::ALL_VEHICLES_HEAD_ONLY, GfxFlaresMode::ALL_VEHICLES_HEAD_ONLY);
 GVar_AP<GfxShadowType>   gfx_shadow_type      ("gfx_shadow_type",         "Shadow technique",          GfxShadowType::NONE,     GfxShadowType::NONE);
 GVar_AP<GfxExtCamMode>   gfx_extcam_mode      ("gfx_extcam_mode",         "External Camera Mode",      GfxExtCamMode::PITCHING, GfxExtCamMode::PITCHING);
 GVar_AP<GfxSkyMode>      gfx_sky_mode         ("gfx_sky_mode",            "Sky effects",               GfxSkyMode::SANDSTORM,   GfxSkyMode::SANDSTORM);
 GVar_AP<GfxTexFilter>    gfx_texture_filter   ("gfx_texture_filter",      "Texture Filtering",         GfxTexFilter::TRILINEAR, GfxTexFilter::TRILINEAR);
 GVar_AP<GfxVegetation>   gfx_vegetation_mode  ("gfx_vegetation_mode",     "Vegetation",                GfxVegetation::NONE,     GfxVegetation::NONE);
 GVar_AP<GfxWaterMode>    gfx_water_mode       ("gfx_water_mode",          "Water effects",             GfxWaterMode::BASIC,     GfxWaterMode::BASIC);
 GVar_A<bool>          gfx_enable_sunburn      ("gfx_enable_sunburn",      "Sunburn",                   false);
 GVar_A<bool>          gfx_water_waves         ("gfx_water_waves",         "Waves",                     false);
 GVar_A<bool>          gfx_minimap_disabled    ("gfx_minimap_disabled",    "disableOverViewMap",        false);
 GVar_A<int>           gfx_particles_mode      ("gfx_particles_mode",      "Particles",                 0);
 GVar_A<bool>          gfx_enable_glow         ("gfx_enable_glow",         "Glow",                      false);
 GVar_A<bool>          gfx_enable_hdr          ("gfx_enable_hdr",          "HDR",                       false);
 GVar_A<bool>          gfx_enable_dof          ("gfx_enable_dof",          "DOF",                       false);
 GVar_A<bool>          gfx_enable_heathaze     ("gfx_enable_heathaze",     "HeatHaze",                  false);
 GVar_A<bool>          gfx_enable_videocams    ("gfx_enable_videocams",    "gfx_enable_videocams",      false);
 GVar_A<bool>          gfx_envmap_enabled      ("gfx_envmap_enabled",      "Envmap",                    false);
 GVar_A<int>           gfx_envmap_rate         ("gfx_envmap_rate",         "EnvmapUpdateRate",          2);
 GVar_A<int>           gfx_skidmarks_mode      ("gfx_skidmarks_mode",      "Skidmarks",                 0);
 GVar_A<float>         gfx_sight_range         ("gfx_sight_range",         "SightRange",                3000.f); // Previously either 2000 or 4500 (inconsistent)
 GVar_APS<float>       gfx_fov_external        ("gfx_fov_external",        "FOV External",              60.f,                      60.f,     60.f);
 GVar_APS<float>       gfx_fov_internal        ("gfx_fov_internal",        "FOV Internal",              75.f,                      75.f,     75.f);
 GVar_A<int>           gfx_fps_limit           ("gfx_fps_limit",           "FPS-Limiter",               0); // 0 = unlimited
 GVar_A<bool>          gfx_speedo_digital      ("gfx_speedo_digital",      "DigitalSpeedo",             false);
 GVar_A<bool>          gfx_speedo_imperial     ("gfx_speedo_imperial",     "gfx_speedo_imperial",       false);
 GVar_A<bool>          gfx_motion_blur         ("gfx_motion_blur",         "Motion blur",               false);

// Instance management
void SetMainMenu       (MainMenu* obj)                { g_main_menu = obj; }
void SetSimController  (SimController* obj)           { g_sim_controller = obj;}
void SetSimTerrain     (TerrainManager* obj)          { g_sim_terrain = obj;}

// Instance access
OgreSubsystem*         GetOgreSubsystem      () { return g_ogre_subsystem; };
ContentManager*        GetContentManager     () { return g_content_manager;}
OverlayWrapper*        GetOverlayWrapper     () { return g_overlay_wrapper;}
SceneMouse*            GetSceneMouse         () { return g_scene_mouse;}
GUIManager*            GetGuiManager         () { return g_gui_manager;}
Console*               GetConsole            () { return g_gui_manager->GetConsole();}
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

void CreateContentManager()
{
    g_content_manager = new ContentManager();
}

void DestroyContentManager()
{
    assert(g_content_manager != nullptr && "DestroyContentManager(): ContentManager never created");
    delete g_content_manager;
    g_content_manager = nullptr;
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
    assert(g_overlay_wrapper != nullptr && "DestroyOverlayWrapper(): OverlayWrapper never created");
    delete g_overlay_wrapper;
    g_overlay_wrapper = nullptr;
}

void CreateSceneMouse()
{
    assert (g_scene_mouse == nullptr);
    g_scene_mouse = new SceneMouse();
}

void DeleteSceneMouse()
{
    assert (g_scene_mouse != nullptr);
    delete g_scene_mouse;
    g_scene_mouse = nullptr;
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

void CreateCacheSystem()
{
    assert(g_cache_system == nullptr);
    g_cache_system = new CacheSystem();
}


void CheckAndCreateMumble()
{
#ifdef USE_MUMBLE // The class is always forward-declared but only defined if USE_MUMBLE is defined
    if (g_mumble == nullptr)
        g_mumble = new MumbleIntegration();
#endif // USE_MUMBLE
}

} // namespace App

const char* ToStr(AppState v)
{
    switch (v)
    {
    case AppState::BOOTSTRAP:           return "BOOTSTRAP";
    case AppState::CHANGE_MAP:          return "CHANGE_MAP";
    case AppState::MAIN_MENU:           return "MAIN_MENU";
    case AppState::PRINT_HELP_EXIT:     return "PRINT_HELP_EXIT";
    case AppState::PRINT_VERSION_EXIT:  return "PRINT_VERSION_EXIT";
    case AppState::SHUTDOWN:            return "SHUTDOWN";
    case AppState::SIMULATION:          return "SIMULATION";
    default:                            return "~invalid~";
    }
}

const char* ToStr(MpState v)
{
    switch (v)
    {
    case MpState::DISABLED:  return "DISABLED";
    case MpState::CONNECTED: return "CONNECTED";
    default:                 return "~invalid~";
    }
}

const char* ToStr(SimState v)
{
    switch (v)
    {
    case SimState::OFF        : return "OFF";
    case SimState::RUNNING    : return "RUNNING";
    case SimState::PAUSED     : return "PAUSED";
    case SimState::SELECTING  : return "SELECTING";
    case SimState::EDITOR_MODE: return "EDITOR_MODE";
    default                   : return "~invalid~";
    }
}

const char* ToStr(SimGearboxMode v)
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

const char* ToStr(GfxFlaresMode v)
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

const char* ToStr(GfxVegetation v)
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

const char* ToStr(GfxWaterMode v)
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

const char* ToStr(GfxSkyMode v)
{
    switch(v)
    {
    case GfxSkyMode::SANDSTORM: return "SANDSTORM";
    case GfxSkyMode::CAELUM   : return "CAELUM";
    case GfxSkyMode::SKYX     : return "SKYX";
    default                   : return "~invalid~";
    }
}

const char* ToStr(IoInputGrabMode v)
{
    switch (v)
    {
    case IoInputGrabMode::NONE   : return "NONE";
    case IoInputGrabMode::ALL    : return "ALL";
    case IoInputGrabMode::DYNAMIC: return "DYNAMIC";
    default                      : return "~invalid~";
    }
}

const char* ToStr(GfxShadowType v)
{
    switch(v)
    {
    case GfxShadowType::NONE   : return "NONE";
    case GfxShadowType::TEXTURE: return "TEXTURE";
    case GfxShadowType::PSSM   : return "PSSM";
    default                    : return "~invalid~";
    }
}

const char* ToStr(GfxTexFilter v)
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

const char* ToStr(GfxExtCamMode v)
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
