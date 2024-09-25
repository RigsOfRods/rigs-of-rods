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

void Console::cVarSetupBuiltins()
{
    App::app_state               = this->cVarCreate("app_state",               "",                                          CVAR_TYPE_INT,     "0"/*(int)AppState::BOOTSTRAP*/);
    App::app_language            = this->cVarCreate("app_language",            "Language",                   CVAR_ARCHIVE,                     "en");
    App::app_country             = this->cVarCreate("app_country",             "Country",                    CVAR_ARCHIVE,                     "us");
    App::app_skip_main_menu      = this->cVarCreate("app_skip_main_menu",      "SkipMainMenu",               CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::app_async_physics       = this->cVarCreate("app_async_physics",       "AsyncPhysics",               CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "true");
    App::app_num_workers         = this->cVarCreate("app_num_workers",         "NumWorkerThreads",           CVAR_ARCHIVE | CVAR_TYPE_INT);
    App::app_screenshot_format   = this->cVarCreate("app_screenshot_format",   "Screenshot Format",          CVAR_ARCHIVE,                     "png");
    App::app_rendersys_override  = this->cVarCreate("app_rendersys_override",  "Render system",              CVAR_ARCHIVE);
    App::app_extra_mod_path      = this->cVarCreate("app_extra_mod_path",      "Extra mod path",             CVAR_ARCHIVE);
    App::app_force_cache_purge   = this->cVarCreate("app_force_cache_purge",   "",                           CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::app_force_cache_update  = this->cVarCreate("app_force_cache_update",  "",                           CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::app_disable_online_api  = this->cVarCreate("app_disable_online_api",  "Disable Online API",         CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::app_config_long_names   = this->cVarCreate("app_config_long_names",   "Config uses long names",     CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "true");
    App::app_custom_scripts      = this->cVarCreate("app_custom_scripts",      "",                           CVAR_ARCHIVE,                     "");
    App::app_recent_scripts      = this->cVarCreate("app_recent_scripts",      "",                           CVAR_ARCHIVE,                     "");

    App::sim_state               = this->cVarCreate("sim_state",               "",                                          CVAR_TYPE_INT,     "0"/*(int)SimState::OFF*/);
    App::sim_terrain_name        = this->cVarCreate("sim_terrain_name",        "",                           0);
    App::sim_terrain_gui_name    = this->cVarCreate("sim_terrain_gui_name",    "",                           0);
    App::sim_spawn_running       = this->cVarCreate("sim_spawn_running",       "Engines spawn running",      CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "true");
    App::sim_replay_enabled      = this->cVarCreate("sim_replay_enabled",      "Replay mode",                CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::sim_replay_length       = this->cVarCreate("sim_replay_length",       "Replay length",              CVAR_ARCHIVE | CVAR_TYPE_INT,     "200");
    App::sim_replay_stepping     = this->cVarCreate("sim_replay_stepping",     "Replay Steps per second",    CVAR_ARCHIVE | CVAR_TYPE_INT,     "1000");
    App::sim_realistic_commands  = this->cVarCreate("sim_realistic_commands",  "Realistic forward commands", CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::sim_races_enabled       = this->cVarCreate("sim_races_enabled",       "Races",                      CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "true");
    App::sim_no_collisions       = this->cVarCreate("sim_no_collisions",       "DisableCollisions",          CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::sim_no_self_collisions  = this->cVarCreate("sim_no_self_collisions",  "DisableSelfCollisions",      CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::sim_gearbox_mode        = this->cVarCreate("sim_gearbox_mode",        "GearboxMode",                CVAR_ARCHIVE | CVAR_TYPE_INT);
    App::sim_soft_reset_mode     = this->cVarCreate("sim_soft_reset_mode",     "",                                          CVAR_TYPE_BOOL,    "false");
    App::sim_quickload_dialog    = this->cVarCreate("sim_quickload_dialog",    "",                           CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "true");
    App::sim_live_repair_interval = this->cVarCreate("sim_live_repair_interval", "",                         CVAR_ARCHIVE | CVAR_TYPE_FLOAT,   "2.f");
    App::sim_tuning_enabled      = this->cVarCreate("sim_tuning_enabled",      "",                           CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "true");

    App::mp_state                = this->cVarCreate("mp_state",                "",                                          CVAR_TYPE_INT,     "0"/*(int)MpState::DISABLED*/);
    App::mp_join_on_startup      = this->cVarCreate("mp_join_on_startup",      "Auto connect",               CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::mp_chat_auto_hide       = this->cVarCreate("mp_chat_auto_hide",       "Auto hide chat",             CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "true");
    App::mp_hide_net_labels      = this->cVarCreate("mp_hide_net_labels",      "Hide net labels",            CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::mp_hide_own_net_label   = this->cVarCreate("mp_hide_own_net_label",   "Hide own net label",         CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "true");
    App::mp_pseudo_collisions    = this->cVarCreate("mp_pseudo_collisions",    "Multiplayer collisions",     CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::mp_server_host          = this->cVarCreate("mp_server_host",          "Server name",                CVAR_ARCHIVE);
    App::mp_server_port          = this->cVarCreate("mp_server_port",          "Server port",                CVAR_ARCHIVE | CVAR_TYPE_INT);
    App::mp_server_password      = this->cVarCreate("mp_server_password",      "Server password",            CVAR_ARCHIVE | CVAR_NO_LOG);
    App::mp_player_name          = this->cVarCreate("mp_player_name",          "Nickname",                   CVAR_ARCHIVE,                     "Player");
    App::mp_player_token         = this->cVarCreate("mp_player_token",         "User Token",                 CVAR_ARCHIVE | CVAR_NO_LOG);
    App::mp_api_url              = this->cVarCreate("mp_api_url",              "Online API URL",             CVAR_ARCHIVE,                     "http://api.rigsofrods.org");
    App::mp_cyclethru_net_actors = this->cVarCreate("mp_cyclethru_net_actors", "",                           CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");

    App::remote_query_url        = this->cVarCreate("remote_query_url",        "",                           CVAR_ARCHIVE,                     "https://v2.api.rigsofrods.org");

    App::diag_auto_spawner_report= this->cVarCreate("diag_auto_spawner_report","AutoActorSpawnerReport",     CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::diag_camera             = this->cVarCreate("diag_camera",             "Camera Debug",               CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::diag_rig_log_node_import= this->cVarCreate("diag_rig_log_node_import","RigImporter_LogAllNodes",    CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::diag_rig_log_node_stats = this->cVarCreate("diag_rig_log_node_stats", "RigImporter_LogNodeStats",   CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::diag_truck_mass         = this->cVarCreate("diag_truck_mass",         "Debug Truck Mass",           CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::diag_envmap             = this->cVarCreate("diag_envmap",             "EnvMapDebug",                CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::diag_videocameras       = this->cVarCreate("diag_videocameras",       "VideoCameraDebug",           CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::diag_preset_terrain     = this->cVarCreate("diag_preset_terrain",     "Preselected Terrain",        CVAR_ARCHIVE);
    App::diag_preset_spawn_pos   = this->cVarCreate("diag_spawn_position",     "",                           CVAR_ARCHIVE);
    App::diag_preset_spawn_rot   = this->cVarCreate("diag_spawn_rotation",     "",                           CVAR_ARCHIVE);
    App::diag_preset_vehicle     = this->cVarCreate("diag_preset_vehicle",     "Preselected Truck",          CVAR_ARCHIVE);
    App::diag_preset_veh_config  = this->cVarCreate("diag_preset_veh_config",  "Preselected TruckConfig",    CVAR_ARCHIVE);
    App::diag_preset_veh_enter   = this->cVarCreate("diag_preset_veh_enter",   "Enter Preselected Truck",    CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::diag_log_console_echo   = this->cVarCreate("diag_log_console_echo",   "Enable Ingame Console",      CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::diag_log_beam_break     = this->cVarCreate("diag_log_beam_break",     "Beam Break Debug",           CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::diag_log_beam_deform    = this->cVarCreate("diag_log_beam_deform",    "Beam Deform Debug",          CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::diag_log_beam_trigger   = this->cVarCreate("diag_log_beam_trigger",   "Trigger Debug",              CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::diag_simple_materials   = this->cVarCreate("diag_simple_materials",    "SimpleMaterials",           CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::diag_warning_texture    = this->cVarCreate("diag_warning_texture",    "Warning texture",            CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::diag_hide_broken_beams  = this->cVarCreate("diag_hide_broken_beams",  "Hide broken beams",          CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::diag_hide_beam_stress   = this->cVarCreate("diag_hide_beam_stress",   "Hide beam stress",           CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "true");
    App::diag_hide_wheel_info    = this->cVarCreate("diag_hide_wheel_info",    "Hide wheel info",            CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "true");
    App::diag_hide_wheels        = this->cVarCreate("diag_hide_wheels",        "Hide wheels",                CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::diag_hide_nodes         = this->cVarCreate("diag_hide_nodes",         "Hide nodes",                 CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::diag_terrn_log_roads    = this->cVarCreate("diag_terrn_log_roads",    "",                           CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::diag_actor_dump         = this->cVarCreate("diag_actor_dump",         "",                           CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::diag_allow_window_resize= this->cVarCreate("diag_allow_window_resize","",                           CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");

    App::sys_process_dir         = this->cVarCreate("sys_process_dir",         "",                           0);
    App::sys_user_dir            = this->cVarCreate("sys_user_dir",            "",                           0);
    App::sys_config_dir          = this->cVarCreate("sys_config_dir",          "Config Root",                0);
    App::sys_cache_dir           = this->cVarCreate("sys_cache_dir",           "Cache Path",                 0);
    App::sys_thumbnails_dir      = this->cVarCreate("sys_thumbnails_dir",      "Thumbnails Path",            0);
    App::sys_logs_dir            = this->cVarCreate("sys_logs_dir",            "Log Path",                   0);
    App::sys_resources_dir       = this->cVarCreate("sys_resources_dir",       "Resources Path",             0);
    App::sys_profiler_dir        = this->cVarCreate("sys_profiler_dir",        "Profiler output dir",        0);
    App::sys_savegames_dir       = this->cVarCreate("sys_savegames_dir",       "",                           0);
    App::sys_screenshot_dir      = this->cVarCreate("sys_screenshot_dir",      "",                           0);
    App::sys_scripts_dir         = this->cVarCreate("sys_scripts_dir",         "",                           0);
    App::sys_projects_dir        = this->cVarCreate("sys_projects_dir",        "",                           0);

    App::cli_server_host         = this->cVarCreate("cli_server_host",         "",                           0);
    App::cli_server_port         = this->cVarCreate("cli_server_port",         "",                                          CVAR_TYPE_INT,     "0");
    App::cli_preset_vehicle      = this->cVarCreate("cli_preset_vehicle",      "",                           0);
    App::cli_preset_veh_config   = this->cVarCreate("cli_preset_veh_config",   "",                           0);
    App::cli_preset_terrain      = this->cVarCreate("cli_preset_terrain",      "",                           0);
    App::cli_preset_spawn_pos    = this->cVarCreate("cli_preset_spawn_pos",    "",                           0);
    App::cli_preset_spawn_rot    = this->cVarCreate("cli_preset_spawn_rot",    "",                           0);
    App::cli_preset_veh_enter    = this->cVarCreate("cli_preset_veh_enter",    "",                                          CVAR_TYPE_BOOL,    "false");
    App::cli_force_cache_update  = this->cVarCreate("cli_force_cache_update",  "",                                          CVAR_TYPE_BOOL,    "false");
    App::cli_resume_autosave     = this->cVarCreate("cli_resume_autosave",     "",                                          CVAR_TYPE_BOOL,    "false");
    App::cli_custom_scripts      = this->cVarCreate("cli_custom_scripts",      "",                           0,                                "");

    App::io_analog_smoothing     = this->cVarCreate("io_analog_smoothing",     "Analog Input Smoothing",     CVAR_ARCHIVE | CVAR_TYPE_FLOAT,   "1.0");
    App::io_analog_sensitivity   = this->cVarCreate("io_analog_sensitivity",   "Analog Input Sensitivity",   CVAR_ARCHIVE | CVAR_TYPE_FLOAT,   "1.0");
    App::io_blink_lock_range     = this->cVarCreate("io_blink_lock_range",     "Blinker Lock Range",         CVAR_ARCHIVE | CVAR_TYPE_FLOAT,   "0.1");
    App::io_ffb_enabled          = this->cVarCreate("io_ffb_enabled",          "Force Feedback",             CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::io_ffb_camera_gain      = this->cVarCreate("io_ffb_camera_gain",      "Force Feedback Camera",      CVAR_ARCHIVE | CVAR_TYPE_FLOAT);   
    App::io_ffb_center_gain      = this->cVarCreate("io_ffb_center_gain",      "Force Feedback Centering",   CVAR_ARCHIVE | CVAR_TYPE_FLOAT);   
    App::io_ffb_master_gain      = this->cVarCreate("io_ffb_master_gain",      "Force Feedback Gain",        CVAR_ARCHIVE | CVAR_TYPE_FLOAT);   
    App::io_ffb_stress_gain      = this->cVarCreate("io_ffb_stress_gain",      "Force Feedback Stress",      CVAR_ARCHIVE | CVAR_TYPE_FLOAT);   
    App::io_input_grab_mode      = this->cVarCreate("io_input_grab_mode",      "Input Grab",                 CVAR_ARCHIVE | CVAR_TYPE_INT,     "1"/*(int)IoInputGrabMode::ALL*/);
    App::io_arcade_controls      = this->cVarCreate("io_arcade_controls",      "ArcadeControls",             CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::io_hydro_coupling       = this->cVarCreate("io_hydro_coupling",  "Keyboard Steering Speed Coupling",CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "true");
    App::io_outgauge_mode        = this->cVarCreate("io_outgauge_mode",        "OutGauge Mode",              CVAR_ARCHIVE | CVAR_TYPE_INT);    // 0 = disabled, 1 = enabled
    App::io_outgauge_ip          = this->cVarCreate("io_outgauge_ip",          "OutGauge IP",                CVAR_ARCHIVE,                     "192.168.1.100");
    App::io_outgauge_port        = this->cVarCreate("io_outgauge_port",        "OutGauge Port",              CVAR_ARCHIVE | CVAR_TYPE_INT,      "1337");
    App::io_outgauge_delay       = this->cVarCreate("io_outgauge_delay",       "OutGauge Delay",             CVAR_ARCHIVE | CVAR_TYPE_FLOAT,    "10.0");
    App::io_outgauge_id          = this->cVarCreate("io_outgauge_id",          "OutGauge ID",                CVAR_ARCHIVE | CVAR_TYPE_INT);
    App::io_discord_rpc          = this->cVarCreate("io_discord_rpc",          "Discord Rich Presence",      CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "true");
    App::io_invert_orbitcam      = this->cVarCreate("io_invert_orbitcam",      "Invert orbit camera",        CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");

    App::audio_master_volume     = this->cVarCreate("audio_master_volume",     "Sound Volume",               CVAR_ARCHIVE | CVAR_TYPE_FLOAT,   "1.0");
    App::audio_enable_creak      = this->cVarCreate("audio_enable_creak",      "Creak Sound",                CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::audio_device_name       = this->cVarCreate("audio_device_name",       "AudioDevice",                CVAR_ARCHIVE);
    App::audio_doppler_factor    = this->cVarCreate("audio_doppler_factor",    "Doppler Factor",             CVAR_ARCHIVE | CVAR_TYPE_FLOAT,   "1.0");
    App::audio_menu_music        = this->cVarCreate("audio_menu_music",        "MainMenuMusic",              CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::audio_enable_efx        = this->cVarCreate("audio_enable_efx",        "Enable OpenAL EFX",          CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "true");
    App::audio_force_efx_preset  = this->cVarCreate("audio_force_efx_preset",  "Enforce OpenAL EFX preset",  0,                                "");

    App::gfx_flares_mode         = this->cVarCreate("gfx_flares_mode",         "Lights",                     CVAR_ARCHIVE | CVAR_TYPE_INT,     "4"/*(int)GfxFlaresMode::ALL_VEHICLES_ALL_LIGHTS*/);
    App::gfx_polygon_mode        = this->cVarCreate("gfx_polygon_mode",        "Polygon mode",                              CVAR_TYPE_INT,     "1"/*(int)Ogre::PM_SOLID*/);
    App::gfx_shadow_type         = this->cVarCreate("gfx_shadow_type",         "Shadow technique",           CVAR_ARCHIVE | CVAR_TYPE_INT,     "1"/*(int)GfxShadowType::PSSM*/);
    App::gfx_extcam_mode         = this->cVarCreate("gfx_extcam_mode",         "External Camera Mode",       CVAR_ARCHIVE | CVAR_TYPE_INT,     "2"/*(int)GfxExtCamMode::PITCHING*/);
    App::gfx_sky_mode            = this->cVarCreate("gfx_sky_mode",            "Sky effects",                CVAR_ARCHIVE | CVAR_TYPE_INT,     "1"/*(int)GfxSkyMode::CAELUM*/);
    App::gfx_texture_filter      = this->cVarCreate("gfx_texture_filter",      "Texture Filtering",          CVAR_ARCHIVE | CVAR_TYPE_INT,     "3"/*(int)GfxTexFilter::ANISOTROPIC*/);
    App::gfx_vegetation_mode     = this->cVarCreate("gfx_vegetation_mode",     "Vegetation",                 CVAR_ARCHIVE | CVAR_TYPE_INT,     "3"/*(int)GfxVegetation::FULL*/);
    App::gfx_sky_time_cycle      = this->cVarCreate("gfx_sky_time_cycle",      "",                                          CVAR_TYPE_BOOL,    "false");
    App::gfx_sky_time_speed      = this->cVarCreate("gfx_sky_time_speed",      "",                                          CVAR_TYPE_INT,     "300");
    App::gfx_water_mode          = this->cVarCreate("gfx_water_mode",          "Water effects",              CVAR_ARCHIVE | CVAR_TYPE_INT,     "3"/*(int)GfxWaterMode::FULL_FAST*/);
    App::gfx_anisotropy          = this->cVarCreate("gfx_anisotropy",          "Anisotropy",                 CVAR_ARCHIVE | CVAR_TYPE_INT,     "4");
    App::gfx_water_waves         = this->cVarCreate("gfx_water_waves",         "Waves",                      CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::gfx_particles_mode      = this->cVarCreate("gfx_particles_mode",      "Particles",                  CVAR_ARCHIVE | CVAR_TYPE_INT);
    App::gfx_enable_videocams    = this->cVarCreate("gfx_enable_videocams",    "gfx_enable_videocams",       CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::gfx_window_videocams    = this->cVarCreate("gfx_window_videocams",    "UseVideocameraWindows",      CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::gfx_surveymap_icons     = this->cVarCreate("gfx_surveymap_icons",     "Overview map icons",         CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "true");
    App::gfx_declutter_map       = this->cVarCreate("gfx_declutter_map",       "Declutter overview map",     CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "true");
    App::gfx_envmap_enabled      = this->cVarCreate("gfx_envmap_enabled",      "Reflections",                CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "true");
    App::gfx_envmap_rate         = this->cVarCreate("gfx_envmap_rate",         "ReflectionUpdateRate",       CVAR_ARCHIVE | CVAR_TYPE_INT,     "1");
    App::gfx_shadow_quality      = this->cVarCreate("gfx_shadow_quality",      "Shadows Quality",            CVAR_ARCHIVE | CVAR_TYPE_INT,     "2");
    App::gfx_skidmarks_mode      = this->cVarCreate("gfx_skidmarks_mode",      "Skidmarks",                  CVAR_ARCHIVE | CVAR_TYPE_INT,     "0");
    App::gfx_sight_range         = this->cVarCreate("gfx_sight_range",         "SightRange",                 CVAR_ARCHIVE | CVAR_TYPE_INT,     "5000");
    App::gfx_camera_height       = this->cVarCreate("gfx_camera_height",       "Static camera height",       CVAR_ARCHIVE | CVAR_TYPE_INT,     "5");
    App::gfx_fov_external        = this->cVarCreate("gfx_fov_external",        "",                                          CVAR_TYPE_INT,     "60");
    App::gfx_fov_external_default= this->cVarCreate("gfx_fov_external_default","FOV External",               CVAR_ARCHIVE | CVAR_TYPE_INT,     "60");
    App::gfx_fov_internal        = this->cVarCreate("gfx_fov_internal",        "",                                          CVAR_TYPE_INT,     "75");
    App::gfx_fov_internal_default= this->cVarCreate("gfx_fov_internal_default","FOV Internal",               CVAR_ARCHIVE | CVAR_TYPE_INT,     "75");
    App::gfx_static_cam_fov_exp  = this->cVarCreate("gfx_static_cam_fov_exp",  "",                           CVAR_ARCHIVE | CVAR_TYPE_FLOAT,   "1.0");
    App::gfx_fixed_cam_tracking  = this->cVarCreate("gfx_fixed_cam_tracking",  "",                           CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::gfx_fps_limit           = this->cVarCreate("gfx_fps_limit",           "FPS-Limiter",                CVAR_ARCHIVE | CVAR_TYPE_INT,     "0");
    App::gfx_speedo_digital      = this->cVarCreate("gfx_speedo_digital",      "DigitalSpeedo",              CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "true");
    App::gfx_speedo_imperial     = this->cVarCreate("gfx_speedo_imperial",     "gfx_speedo_imperial",        CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::gfx_flexbody_cache      = this->cVarCreate("gfx_flexbody_cache",      "Flexbody_UseCache",          CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::gfx_reduce_shadows      = this->cVarCreate("gfx_reduce_shadows",      "Shadow optimizations",       CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "true");
    App::gfx_enable_rtshaders    = this->cVarCreate("gfx_enable_rtshaders",    "Use RTShader System",        CVAR_ARCHIVE | CVAR_TYPE_BOOL,    "false");
    App::gfx_alt_actor_materials = this->cVarCreate("gfx_alt_actor_materials", "Use alternate vehicle materials", CVAR_ARCHIVE | CVAR_TYPE_BOOL, "false");

    App::flexbody_defrag_enabled           = this->cVarCreate("flexbody_defrag_enabled",           "", CVAR_TYPE_BOOL);
    App::flexbody_defrag_const_penalty     = this->cVarCreate("flexbody_defrag_const_penalty",     "", CVAR_TYPE_INT, "7");
    App::flexbody_defrag_prog_up_penalty   = this->cVarCreate("flexbody_defrag_prog_up_penalty",   "", CVAR_TYPE_INT, "3");
    App::flexbody_defrag_prog_down_penalty = this->cVarCreate("flexbody_defrag_prog_down_penalty", "", CVAR_TYPE_INT, "1");
    App::flexbody_defrag_reorder_indices   = this->cVarCreate("flexbody_defrag_reorder_indices",   "", CVAR_TYPE_BOOL, "true");
    App::flexbody_defrag_reorder_texcoords = this->cVarCreate("flexbody_defrag_reorder_texcoords", "", CVAR_TYPE_BOOL, "true");
    App::flexbody_defrag_invert_lookup     = this->cVarCreate("flexbody_defrag_invert_lookup",     "", CVAR_TYPE_BOOL, "true");

    App::ui_show_live_repair_controls      = this->cVarCreate("ui_show_live_repair_controls",      "", CVAR_ARCHIVE | CVAR_TYPE_BOOL, "true");
    App::ui_show_vehicle_buttons           = this->cVarCreate("ui_show_vehicle_buttons", "Show vehicle buttons menu", CVAR_ARCHIVE | CVAR_TYPE_BOOL, "true");
}

CVar* Console::cVarCreate(std::string const& name, std::string const& long_name,
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
        this->cVarAssign(cvar, val);
    }

    m_cvars.insert(std::make_pair(name, cvar));
    m_cvars_longname.insert(std::make_pair(long_name, cvar));

    return cvar;
}

void Console::cVarAssign(CVar* cvar, std::string const& value)
{
    if (cvar->hasFlag(CVAR_TYPE_BOOL | CVAR_TYPE_INT | CVAR_TYPE_FLOAT))
    {
        float val = 0.f;
             if (cvar->hasFlag(CVAR_TYPE_BOOL))  { val = (float)Ogre::StringConverter::parseBool(value); }
        else if (cvar->hasFlag(CVAR_TYPE_INT))   { val = (float)Ogre::StringConverter::parseInt(value);  }
        else if (cvar->hasFlag(CVAR_TYPE_FLOAT)) { val =        Ogre::StringConverter::parseReal(value); }


        cvar->setVal(val);
    }
    else
    {
        cvar->setStr(value);
    }
}

CVar* Console::cVarFind(std::string const& input_name)
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

CVar* Console::cVarSet(std::string const& input_name, std::string const& input_val)
{
    CVar* found = this->cVarFind(input_name);
    if (found)
    {
        this->cVarAssign(found, input_val);
    }

    return nullptr;
}

CVar* Console::cVarGet(std::string const& input_name, int flags)
{
    CVar* found = this->cVarFind(input_name);
    if (found)
    {
        return found;
    }

    return this->cVarCreate(input_name, input_name, flags);
}

void CVar::logUpdate(std::string const& new_val)
{
    if (!Ogre::LogManager::getSingletonPtr())
        return;

    if (this->hasFlag(CVAR_NO_LOG))
        return;

    LogFormat("[RoR|CVar]  %20s:  \"%s\" (was: \"%s\")",
              m_name.c_str(), new_val.c_str(), m_value_str.c_str());
}

std::string CVar::convertStr(float val)
{
    if (this->hasFlag(CVAR_TYPE_BOOL))
    {
        return ((bool)val) ? "Yes" : "No";
    }
    else if (this->hasFlag(CVAR_TYPE_INT))
    {
        return Ogre::StringUtil::format("%d", (int)val);
    }
    else if (this->hasFlag(CVAR_TYPE_FLOAT))
    {
        return std::to_string((float)val);
    }
    else
    {
        return std::to_string(val);
    }
}
