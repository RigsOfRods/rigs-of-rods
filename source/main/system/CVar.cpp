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
#include "Console.h"

#include <Ogre.h>

using namespace RoR;

void CVar::LogStr(const char* op, std::string const& old_val, std::string const& new_val)
{
    if (!Ogre::LogManager::getSingletonPtr())
        return;

    Str<100> flags_str;
    if (m_flags & CVAR_AUTO_APPLY) { flags_str << " [autoapply]"; }
    if (m_flags & CVAR_AUTO_STORE) { flags_str << " [autostore]"; }
    LogFormat(CVAR_LOG_FMT, m_name.c_str(), op, new_val.c_str(), old_val.c_str(), flags_str.ToCStr());
}

void CVar::LogVal(const char* op, float old_val, float new_val)
{
    if (!Ogre::LogManager::getSingletonPtr())
        return;

    this->LogStr(op, Val::ConvertStr(old_val, m_flags), Val::ConvertStr(new_val, m_flags));
}

void Console::CVarSetupBuiltins()
{
    App::app_state               = this->CVarCreate("app_state",               "",                                                                CVAR_TYPE_INT,     "0"/*(int)AppState::BOOTSTRAP*/);
    App::app_language            = this->CVarCreate("app_language",            "Language",                   CVAR_ALLOW_STORE | CVAR_AUTO_APPLY,                     "en");          
    App::app_country             = this->CVarCreate("app_country",             "Country",                    CVAR_ALLOW_STORE | CVAR_AUTO_APPLY,                     "us");
    App::app_skip_main_menu      = this->CVarCreate("app_skip_main_menu",      "SkipMainMenu",               CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::app_async_physics       = this->CVarCreate("app_async_physics",       "AsyncPhysics",               CVAR_ALLOW_STORE                   | CVAR_TYPE_BOOL,    "true");
    App::app_num_workers         = this->CVarCreate("app_num_workers",         "NumWorkerThreads",           CVAR_ALLOW_STORE);
    App::app_screenshot_format   = this->CVarCreate("app_screenshot_format",   "Screenshot Format",          CVAR_ALLOW_STORE,                                       "png");
    App::app_rendersys_override  = this->CVarCreate("app_rendersys_override",  "Render system",              CVAR_ALLOW_STORE | CVAR_AUTO_APPLY);   
    App::app_extra_mod_path      = this->CVarCreate("app_extra_mod_path",      "Extra mod path",             CVAR_ALLOW_STORE | CVAR_AUTO_APPLY);   
    App::app_force_cache_purge   = this->CVarCreate("app_force_cache_purge",   "",                           CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::app_force_cache_udpate  = this->CVarCreate("app_force_cache_udpate",  "",                           CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::app_disable_online_api  = this->CVarCreate("app_disable_online_api",  "Disable Online API",         CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::app_config_long_names   = this->CVarCreate("app_config_long_names",   "Config uses long names",     CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "true");

    App::sim_state               = this->CVarCreate("sim_state",               "",                                                                CVAR_TYPE_INT);
    App::sim_terrain_name        = this->CVarCreate("sim_terrain_name",        "",                           0);
    App::sim_terrain_gui_name    = this->CVarCreate("sim_terrain_gui_name",    "",                                              CVAR_AUTO_APPLY);
    App::sim_savegame            = this->CVarCreate("sim_savegame",            "",                           0);
    App::sim_spawn_running       = this->CVarCreate("sim_spawn_running",       "Engines spawn running",      CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "true");        
    App::sim_replay_enabled      = this->CVarCreate("sim_replay_enabled",      "Replay mode",                CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::sim_replay_length       = this->CVarCreate("sim_replay_length",       "Replay length",              CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_INT,     "200");                                                        
    App::sim_replay_stepping     = this->CVarCreate("sim_replay_stepping",     "Replay Steps per second",    CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_INT,     "1000");                                                       
    App::sim_realistic_commands  = this->CVarCreate("sim_realistic_commands",  "Realistic forward commands", CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::sim_races_enabled       = this->CVarCreate("sim_races_enabled",       "Races",                      CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "true");
    App::sim_no_collisions       = this->CVarCreate("sim_no_collisions",       "DisableCollisions",          CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::sim_no_self_collisions  = this->CVarCreate("sim_no_self_collisions",  "DisableSelfCollisions",      CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::sim_gearbox_mode        = this->CVarCreate("sim_gearbox_mode",        "GearboxMode",                CVAR_ALLOW_STORE                   | CVAR_TYPE_INT);

    App::mp_state                = this->CVarCreate("mp_state",                "",                                                                CVAR_TYPE_INT,     "0"/*(int)MpState::DISABLED*/);
    App::mp_join_on_startup      = this->CVarCreate("mp_join_on_startup",      "Auto connect",               CVAR_ALLOW_STORE                   | CVAR_TYPE_BOOL,    "false");       
    App::mp_chat_auto_hide       = this->CVarCreate("mp_chat_auto_hide",       "Auto hide chat",             CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "true");
    App::mp_hide_net_labels      = this->CVarCreate("mp_hide_net_labels",      "Hide net labels",            CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::mp_hide_own_net_label   = this->CVarCreate("mp_hide_own_net_label",   "Hide own net label",         CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "true");
    App::mp_pseudo_collisions    = this->CVarCreate("mp_pseudo_collisions",    "Multiplayer collisions",     CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::mp_server_host          = this->CVarCreate("mp_server_host",          "Server name",                CVAR_ALLOW_STORE);
    App::mp_server_port          = this->CVarCreate("mp_server_port",          "Server port",                CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_INT);
    App::mp_server_password      = this->CVarCreate("mp_server_password",      "Server password",            CVAR_ALLOW_STORE);       
    App::mp_player_name          = this->CVarCreate("mp_player_name",          "Nickname",                   CVAR_ALLOW_STORE,                                       "Player");
    App::mp_player_token         = this->CVarCreate("mp_player_token",         "User Token",                 CVAR_ALLOW_STORE);          
    App::mp_api_url              = this->CVarCreate("mp_api_url",              "Online API URL",             CVAR_ALLOW_STORE | CVAR_AUTO_APPLY,                     "http://api.rigsofrods.org");

    App::diag_auto_spawner_report= this->CVarCreate("diag_auto_spawner_report","AutoActorSpawnerReport",     CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::diag_camera             = this->CVarCreate("diag_camera",             "Camera Debug",               CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::diag_rig_log_node_import= this->CVarCreate("diag_rig_log_node_import","RigImporter_LogAllNodes",    CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::diag_rig_log_node_stats = this->CVarCreate("diag_rig_log_node_stats", "RigImporter_LogNodeStats",   CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::diag_collisions         = this->CVarCreate("diag_collisions",         "Debug Collisions",           CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::diag_truck_mass         = this->CVarCreate("diag_truck_mass",         "Debug Truck Mass",           CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::diag_envmap             = this->CVarCreate("diag_envmap",             "EnvMapDebug",                CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::diag_videocameras       = this->CVarCreate("diag_videocameras",       "VideoCameraDebug",           CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::diag_preset_terrain     = this->CVarCreate("diag_preset_terrain",     "Preselected Terrain",        CVAR_ALLOW_STORE);       
    App::diag_preset_spawn_pos   = this->CVarCreate("diag_spawn_position",     "",                           CVAR_ALLOW_STORE);         
    App::diag_preset_spawn_rot   = this->CVarCreate("diag_spawn_rotation",     "",                           CVAR_ALLOW_STORE | CVAR_AUTO_APPLY);
    App::diag_preset_vehicle     = this->CVarCreate("diag_preset_vehicle",     "Preselected Truck",          CVAR_ALLOW_STORE);
    App::diag_preset_veh_config  = this->CVarCreate("diag_preset_veh_config",  "Preselected TruckConfig",    CVAR_ALLOW_STORE | CVAR_AUTO_APPLY);
    App::diag_preset_veh_enter   = this->CVarCreate("diag_preset_veh_enter",   "Enter Preselected Truck",    CVAR_ALLOW_STORE                   | CVAR_TYPE_BOOL,    "false");
    App::diag_log_console_echo   = this->CVarCreate("diag_log_console_echo",   "Enable Ingame Console",      CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::diag_log_beam_break     = this->CVarCreate("diag_log_beam_break",     "Beam Break Debug",           CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::diag_log_beam_deform    = this->CVarCreate("diag_log_beam_deform",    "Beam Deform Debug",          CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::diag_log_beam_trigger   = this->CVarCreate("diag_log_beam_trigger",   "Trigger Debug",              CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::diag_simple_materials   = this->CVarCreate("diag_simple_materials",    "SimpleMaterials",           CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::diag_warning_texture    = this->CVarCreate("diag_warning_texture",    "Warning texture",            CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::diag_hide_broken_beams  = this->CVarCreate("diag_hide_broken_beams",  "Hide broken beams",          CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::diag_hide_beam_stress   = this->CVarCreate("diag_hide_beam_stress",   "Hide beam stress",           CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "true");
    App::diag_hide_wheel_info    = this->CVarCreate("diag_hide_wheel_info",    "Hide wheel info",            CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "true");
    App::diag_hide_wheels        = this->CVarCreate("diag_hide_wheels",        "Hide wheels",                CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::diag_hide_nodes         = this->CVarCreate("diag_hide_nodes",         "Hide nodes",                 CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::diag_physics_dt         = this->CVarCreate("diag_physics_dt",          "PhysicsTimeStep",           CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_FLOAT,   "0.0005");             

    App::sys_process_dir         = this->CVarCreate("sys_process_dir",         "",                                              CVAR_AUTO_APPLY);
    App::sys_user_dir            = this->CVarCreate("sys_user_dir",            "",                                              CVAR_AUTO_APPLY);
    App::sys_config_dir          = this->CVarCreate("sys_config_dir",          "Config Root",                                   CVAR_AUTO_APPLY);
    App::sys_cache_dir           = this->CVarCreate("sys_cache_dir",           "Cache Path",                                    CVAR_AUTO_APPLY);
    App::sys_logs_dir            = this->CVarCreate("sys_logs_dir",            "Log Path",                                      CVAR_AUTO_APPLY);
    App::sys_resources_dir       = this->CVarCreate("sys_resources_dir",       "Resources Path",                                CVAR_AUTO_APPLY);
    App::sys_profiler_dir        = this->CVarCreate("sys_profiler_dir",        "Profiler output dir",                           CVAR_AUTO_APPLY);
    App::sys_savegames_dir       = this->CVarCreate("sys_savegames_dir",       "",                                              CVAR_AUTO_APPLY);
    App::sys_screenshot_dir      = this->CVarCreate("sys_screenshot_dir",      "",                                              CVAR_AUTO_APPLY);

    App::io_analog_smoothing     = this->CVarCreate("io_analog_smoothing",     "Analog Input Smoothing",     CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_FLOAT,   "1.0");
    App::io_analog_sensitivity   = this->CVarCreate("io_analog_sensitivity",   "Analog Input Sensitivity",   CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_FLOAT,   "1.0");
    App::io_blink_lock_range     = this->CVarCreate("io_blink_lock_range",     "Blinker Lock Range",         CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_FLOAT,   "0.1");
    App::io_ffb_enabled          = this->CVarCreate("io_ffb_enabled",          "Force Feedback",             CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::io_ffb_camera_gain      = this->CVarCreate("io_ffb_camera_gain",      "Force Feedback Camera",      CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_FLOAT);   
    App::io_ffb_center_gain      = this->CVarCreate("io_ffb_center_gain",      "Force Feedback Centering",   CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_FLOAT);   
    App::io_ffb_master_gain      = this->CVarCreate("io_ffb_master_gain",      "Force Feedback Gain",        CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_FLOAT);   
    App::io_ffb_stress_gain      = this->CVarCreate("io_ffb_stress_gain",      "Force Feedback Stress",      CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_FLOAT);   
    App::io_input_grab_mode      = this->CVarCreate("io_input_grab_mode",      "Input Grab",                 CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_INT,     "1"/*(int)IoInputGrabMode::ALL*/);
    App::io_arcade_controls      = this->CVarCreate("io_arcade_controls",      "ArcadeControls",             CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::io_hydro_coupling       = this->CVarCreate("io_hydro_coupling",  "Keyboard Steering Speed Coupling",CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "true");
    App::io_outgauge_mode        = this->CVarCreate("io_outgauge_mode",        "OutGauge Mode",              CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_INT);    // 0 = disabled, 1 = enabled
    App::io_outgauge_ip          = this->CVarCreate("io_outgauge_ip",          "OutGauge IP",                CVAR_ALLOW_STORE | CVAR_AUTO_APPLY,                     "192.168.1.100");
    App::io_outgauge_port        = this->CVarCreate("io_outgauge_port",        "OutGauge Port",              CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_INT,      "1337");
    App::io_outgauge_delay       = this->CVarCreate("io_outgauge_delay",       "OutGauge Delay",             CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_FLOAT,    "10.0");
    App::io_outgauge_id          = this->CVarCreate("io_outgauge_id",          "OutGauge ID",                CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_INT);
    App::io_discord_rpc          = this->CVarCreate("io_discord_rpc",          "Discord Rich Presence",      CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "true");

    App::audio_master_volume     = this->CVarCreate("audio_master_volume",     "Sound Volume",               CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_FLOAT,   "1.0");
    App::audio_enable_creak      = this->CVarCreate("audio_enable_creak",      "Creak Sound",                CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::audio_device_name       = this->CVarCreate("audio_device_name",       "AudioDevice",                CVAR_ALLOW_STORE);       
    App::audio_menu_music        = this->CVarCreate("audio_menu_music",        "MainMenuMusic",              CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");

    App::gfx_flares_mode         = this->CVarCreate("gfx_flares_mode",         "Lights",                     CVAR_ALLOW_STORE                   | CVAR_TYPE_INT,     "4"/*(int)GfxFlaresMode::ALL_VEHICLES_ALL_LIGHTS*/);
    App::gfx_shadow_type         = this->CVarCreate("gfx_shadow_type",         "Shadow technique",           CVAR_ALLOW_STORE                   | CVAR_TYPE_INT,     "1"/*(int)GfxShadowType::PSSM*/);
    App::gfx_extcam_mode         = this->CVarCreate("gfx_extcam_mode",         "External Camera Mode",       CVAR_ALLOW_STORE                   | CVAR_TYPE_INT,     "2"/*(int)GfxExtCamMode::PITCHING*/);
    App::gfx_sky_mode            = this->CVarCreate("gfx_sky_mode",            "Sky effects",                CVAR_ALLOW_STORE                   | CVAR_TYPE_INT,     "1"/*(int)GfxSkyMode::CAELUM*/);
    App::gfx_texture_filter      = this->CVarCreate("gfx_texture_filter",      "Texture Filtering",          CVAR_ALLOW_STORE                   | CVAR_TYPE_INT,     "3"/*(int)GfxTexFilter::ANISOTROPIC*/);
    App::gfx_vegetation_mode     = this->CVarCreate("gfx_vegetation_mode",     "Vegetation",                 CVAR_ALLOW_STORE                   | CVAR_TYPE_INT,     "3"/*(int)GfxVegetation::FULL*/);
    App::gfx_water_mode          = this->CVarCreate("gfx_water_mode",          "Water effects",              CVAR_ALLOW_STORE                   | CVAR_TYPE_INT,     "3"/*(int)GfxWaterMode::FULL_FAST*/);
    App::gfx_anisotropy          = this->CVarCreate("gfx_anisotropy",          "Anisotropy",                 CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_INT,     "4");
    App::gfx_water_waves         = this->CVarCreate("gfx_water_waves",         "Waves",                      CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::gfx_particles_mode      = this->CVarCreate("gfx_particles_mode",      "Particles",                  CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_INT);
    App::gfx_enable_videocams    = this->CVarCreate("gfx_enable_videocams",    "gfx_enable_videocams",       CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::gfx_window_videocams    = this->CVarCreate("gfx_window_videocams",    "UseVideocameraWindows",      CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::gfx_surveymap_icons     = this->CVarCreate("gfx_surveymap_icons",     "Overview map icons",         CVAR_ALLOW_STORE                   | CVAR_TYPE_BOOL,    "true");
    App::gfx_declutter_map       = this->CVarCreate("gfx_declutter_map",       "Declutter overview map",     CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "true");
    App::gfx_envmap_enabled      = this->CVarCreate("gfx_envmap_enabled",      "Reflections",                CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "true");
    App::gfx_envmap_rate         = this->CVarCreate("gfx_envmap_rate",         "ReflectionUpdateRate",       CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_INT,     "1");         
    App::gfx_shadow_quality      = this->CVarCreate("gfx_shadow_quality",      "Shadows Quality",            CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_INT,     "2");         
    App::gfx_skidmarks_mode      = this->CVarCreate("gfx_skidmarks_mode",      "Skidmarks",                  CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_INT,     "0");         
    App::gfx_sight_range         = this->CVarCreate("gfx_sight_range",         "SightRange",                 CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_INT,     "5000");      
    App::gfx_camera_height       = this->CVarCreate("gfx_camera_height",       "Static camera height",       CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_INT,     "5");         
    App::gfx_fov_external        = this->CVarCreate("gfx_fov_external",        "FOV External",               CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_INT,     "60");
    App::gfx_fov_internal        = this->CVarCreate("gfx_fov_internal",        "FOV Internal",               CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_INT,     "75");
    App::gfx_static_cam_fov_exp  = this->CVarCreate("gfx_static_cam_fov_exp",  "",                           CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_FLOAT,   "1.0");
    App::gfx_fixed_cam_tracking  = this->CVarCreate("gfx_fixed_cam_tracking",  "",                           CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::gfx_fps_limit           = this->CVarCreate("gfx_fps_limit",           "FPS-Limiter",                CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_INT,     "0");        
    App::gfx_speedo_digital      = this->CVarCreate("gfx_speedo_digital",      "DigitalSpeedo",              CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "true");
    App::gfx_speedo_imperial     = this->CVarCreate("gfx_speedo_imperial",     "gfx_speedo_imperial",        CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::gfx_flexbody_lods       = this->CVarCreate("gfx_flexbody_lods",       "Flexbody_EnableLODs",        CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::gfx_flexbody_cache      = this->CVarCreate("gfx_flexbody_cache",      "Flexbody_UseCache",          CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::gfx_reduce_shadows      = this->CVarCreate("gfx_reduce_shadows",      "Shadow optimizations",       CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "true");
    App::gfx_enable_rtshaders    = this->CVarCreate("gfx_enable_rtshaders",    "Use RTShader System",        CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
    App::gfx_classic_shaders     = this->CVarCreate("gfx_classic_shaders",     "Classic material shaders",   CVAR_ALLOW_STORE | CVAR_AUTO_APPLY | CVAR_TYPE_BOOL,    "false");
}

CVar* Console::CVarCreate(std::string const& name, std::string const& long_name,
    int flags, std::string const& val/* = std::string()*/)
{
    CVar* cvar = nullptr;
    if (long_name == "")
    {
        cvar = new CVar(name, name, flags);
    }
    else
    {
        cvar = new CVar(name, long_name, flags);
    }

    if (!val.empty())
    {
        this->CVarAssign(cvar, val, /*force_active = */true);
    }

    m_cvars.insert(std::make_pair(name, cvar));
    m_cvars_longname.insert(std::make_pair(long_name, cvar));

    return cvar;
}

void Console::CVarAssign(CVar* cvar, std::string const& value, bool force_active/* = false*/)
{
    if (cvar->HasFlags(CVAR_TYPE_BOOL))
    {
        if (force_active)
            cvar->SetActiveVal(Ogre::StringConverter::parseBool(value));
        else
            cvar->SetPendingVal(Ogre::StringConverter::parseBool(value));
    }
    else if (cvar->HasFlags(CVAR_TYPE_INT))
    {
        if (force_active)
            cvar->SetActiveVal(Ogre::StringConverter::parseInt(value));
        else
            cvar->SetPendingVal(Ogre::StringConverter::parseInt(value));
    }
    else if (cvar->HasFlags(CVAR_TYPE_FLOAT))
    {
        if (force_active)
            cvar->SetActiveVal(Ogre::StringConverter::parseReal(value));
        else
            cvar->SetPendingVal(Ogre::StringConverter::parseReal(value));
    }
    else
    {
        if (force_active)
            cvar->SetActiveStr(value);
        else
            cvar->SetPendingStr(value);
    }
}

CVar* Console::CVarFind(std::string const& input_name)
{
    CVarPtrMap::const_iterator itor = m_cvars.find(input_name);
    if (itor != m_cvars.end())
    {
        return itor->second;
    }

    itor = m_cvars_longname.find(input_name);
    if (itor != m_cvars_longname.end())
    {
        return itor->second;
    }

    return nullptr;
}

CVar* Console::CVarSet(std::string const& input_name, std::string const& input_val, bool force_active/* = false*/)
{
    CVar* found = this->CVarFind(input_name);
    if (found)
    {
        this->CVarAssign(found, input_val, force_active);
    }

    return nullptr;
}

CVar* Console::CVarGet(std::string const& input_name, int flags)
{
    CVar* found = this->CVarFind(input_name);
    if (found)
    {
        return found;
    }

    return this->CVarCreate(input_name, input_name, flags);
}

