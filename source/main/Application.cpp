/*
    This source file is part of Rigs of Rods
    Copyright 2013-2016 Petr Ohlidal

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

/** 
    @file   Application.cpp
    @author Petr Ohlidal
    @date   05/2014
*/

#include "Application.h"

#include <OgreException.h>

#include "CacheSystem.h"
#include "ContentManager.h"
#include "GUIManager.h"
#include "InputEngine.h"
#include "OgreSubsystem.h"
#include "OverlayWrapper.h"
#include "SceneMouse.h"

#define STR_CREF  std::string const &

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

// App
static int              g_app_state_active;      ///< Current state
static int              g_app_state_pending;     ///< Requested state change
static std::string      g_app_language;          ///< Config: STR Language
static std::string      g_app_locale;            ///< Config: STR Language Short
static bool             g_app_multithread;       ///< Config: STR Multi-threading
static std::string      g_app_screenshot_format; ///< Config: STR Screenshot Format

// Simulation
static int              g_sim_state_active;      ///< Current state
static int              g_sim_state_pending;     ///< Requested state change
static std::string      g_sim_active_terrain;
static std::string      g_sim_next_terrain;
static bool             g_sim_replay_enabled;    ///< Config: BOOL Replay mode            
static int              g_sim_replay_length;     ///< Config: INT  Replay length          
static int              g_sim_replay_stepping;   ///< Config: INT  Replay Steps per second
static bool             g_sim_position_storage;  ///< Config: BOOL Position Storage       
static int              g_sim_gearbox_mode;      ///< Config: STR  GearboxMode

// Multiplayer
static int              g_mp_state_active;       ///< Current state
static int              g_mp_state_pending;      ///< Requested state change
static std::string      g_mp_server_host;        ///< Config: STR Server name
static int              g_mp_server_port;        ///< Config: INT Server port
static std::string      g_mp_server_password;    ///< Config: STR Server password
static std::string      g_mp_player_name;        ///< Config: STR Nickname
static std::string      g_mp_portal_url;

// Diagnostic
static bool             g_diag_trace_globals;
static bool             g_diag_rig_log_node_import;    ///< Config: BOOL RigImporter_Debug_TraverseAndLogAllNodes
static bool             g_diag_rig_log_node_stats;     ///< Config: BOOL RigImporter_PrintNodeStatsToLog
static bool             g_diag_rig_log_messages;       ///< Config: BOOL RigImporter_PrintMessagesToLog
static bool             g_diag_collisions;             ///< Config: BOOL Debug Collisions
static bool             g_diag_truck_mass;             ///< Config: BOOL Debug Truck Mass
static bool             g_diag_envmap;                 ///< Config: BOOL EnvMapDebug
static bool             g_diag_videocameras;           ///< Config: BOOL VideoCameraDebug
static std::string      g_diag_preselected_terrain;    ///< Config: STR  Preselected Map
static std::string      g_diag_preselected_vehicle;    ///< Config: STR  Preselected Truck  
static std::string      g_diag_preselected_veh_config; ///< Config: STR  Preselected TruckConfig
static bool             g_diag_preselected_veh_enter;  ///< Config: STR  Enter Preselected Truck

// System
static std::string      g_sys_process_dir;       ///< No ending slash.
static std::string      g_sys_user_dir;          ///< No ending slash.
static std::string      g_sys_config_dir;        ///< No ending slash. Config: STR Config Root
static std::string      g_sys_cache_dir;         ///< No ending slash. Config: STR Cache Path
static std::string      g_sys_logs_dir;          ///< No ending slash. Config: STR Log Path
static std::string      g_sys_resources_dir;     ///< No ending slash. Config: STR Resources Path
static std::string      g_sys_profiler_dir;      ///< No ending slash. Config: STR Profiler output dir
static std::string      g_sys_screenshot_dir;    ///< No ending slash.

// Input - Output
static bool             g_io_ffback_enabled;     ///< Config: BOOL  Force Feedback
static float            g_io_ffback_camera_gain; ///< Config: FLOAT Force Feedback Camera
static float            g_io_ffback_center_gain; ///< Config: FLOAT Force Feedback Centering
static float            g_io_ffback_master_gain; ///< Config: FLOAT Force Feedback Gain
static float            g_io_ffback_stress_gain; ///< Config: FLOAT Force Feedback Stress
static int              g_io_input_grab_mode;    ///< Config: BOOL  Input Grab
static bool             g_io_arcade_controls;    ///< Config: BOOL  ArcadeControls
static int              g_io_outgauge_mode;      ///< Config: INT   OutGauge Mode
static std::string      g_io_outgauge_ip;        ///< Config: STR   OutGauge IP
static int              g_io_outgauge_port;      ///< Config: INT   OutGauge Port
static float            g_io_outgauge_delay;     ///< Config: FLOAT OutGauge Delay
static int              g_io_outgauge_id;        ///< Config: INT   OutGauge ID

// Audio
static float            g_audio_master_volume;   ///< Config: FLOAT Sound Volume
static bool             g_audio_enable_creak;    ///< Config: BOOL  Creak Sound
static std::string      g_audio_device_name;     ///< Config: STR   AudioDevice
static bool             g_audio_menu_music;      ///< Config: BOOL  MainMenuMusic 

// Graphics
static int              g_gfx_flares_mode;       ///< Config: STR   Lights
static int              g_gfx_shadow_type;       ///< Config: STR   Shadow technique
static int              g_gfx_extcam_mode;       ///< Config: STR   External Camera Mode
static int              g_gfx_sky_mode;          ///< Config: STR   Sky effects
static int              g_gfx_texture_filter;    ///< Config: STR   Texture Filtering
static int              g_gfx_vegetation_mode;   ///< Config: STR   Vegetation
static int              g_gfx_water_mode;        ///< Config: STR   Water effects
static bool             g_gfx_enable_sunburn;    ///< Config: BOOL  Sunburn
static bool             g_gfx_water_waves;       ///< Config: BOOL  Waves
static bool             g_gfx_minimap_disabled;  ///< Config: BOOL  disableOverViewMap
static int              g_gfx_particles_mode;    ///< Config: BOOL  Particles
static bool             g_gfx_enable_glow;       ///< Config: BOOL  Glow
static bool             g_gfx_enable_hdr;        ///< Config: BOOL  HDR
static bool             g_gfx_enable_heathaze;   ///< Config: BOOL  HeatHaze
static bool             g_gfx_enable_videocams;  ///< Config: BOOL  gfx_enable_videocams
static bool             g_gfx_envmap_enabled;    ///< Config: BOOL  Envmap
static int              g_gfx_envmap_rate;       ///< Config: INT   EnvmapUpdateRate
static int              g_gfx_skidmarks_mode;    ///< Config: BOOL  Skidmarks
static float            g_gfx_sight_range;       ///< Config: FLOAT SightRange
static float            g_gfx_fov_external;      ///< Config: FLOAT FOV External
static float            g_gfx_fov_internal;      ///< Config: FLOAT FOV Internal
static int              g_gfx_fps_limit;         ///< Config: INT   FPS-Limiter

// ================================================================================
// Access functions
// ================================================================================


// Helpers (forward decl.)
typedef const char* (*EnumToStringFn)(int);

const char* SimGearboxModeToString(int v);
const char* GfxFlaresModeToString (int v);
const char* IoInputGrabModeToStr  (int v);
const char* GfxVegetationModeToStr(int v);
const char* GfxWaterModeToString  (int v);
const char* GfxSkyModeToString    (int v);
const char* GfxShadowModeToStr    (int v);
const char* GfxTexFilterToStr     (int v);
const char* AppStateToStr         (int v);
const char* SimStateToStr         (int v);
const char* MpStateToStr          (int v);

void SetVarStr      (std::string&     var, const char* var_name, STR_CREF        new_value);
void SetVarInt      (int&             var, const char* var_name, int             new_value);
void SetVarEnum     (int&             var, const char* var_name, int             new_value,   EnumToStringFn to_str_fn );
void SetVarBool     (bool&            var, const char* var_name, bool            new_value);
void SetVarFloat    (float&           var, const char* var_name, float           new_value);

// Getters
State           GetActiveAppState          () { return (State)g_app_state_active;      }
State           GetPendingAppState         () { return (State)g_app_state_pending;     }
STR_CREF        GetSimActiveTerrain        () { return g_sim_active_terrain;    }
STR_CREF        GetSimNextTerrain          () { return g_sim_next_terrain;      }
SimState        GetActiveSimState          () { return (SimState)g_sim_state_active;   }
SimState        GetPendingSimState         () { return (SimState)g_sim_state_pending;  }
MpState         GetActiveMpState           () { return (MpState)g_mp_state_active;     }
MpState         GetPendingMpState          () { return (MpState)g_mp_state_pending;    }
STR_CREF        GetMpServerHost            () { return g_mp_server_host;        }
STR_CREF        GetMpServerPassword        () { return g_mp_server_password;    }
int             GetMpServerPort            () { return g_mp_server_port;        }
STR_CREF        GetMpPlayerName            () { return g_mp_player_name;        }
STR_CREF        GetMpPortalUrl             () { return g_mp_portal_url;         }
bool            GetDiagTraceGlobals        () { return g_diag_trace_globals;    }
STR_CREF        GetSysProcessDir           () { return g_sys_process_dir;       }
STR_CREF        GetSysUserDir              () { return g_sys_user_dir;          }
STR_CREF        GetSysConfigDir            () { return g_sys_config_dir;        }
STR_CREF        GetSysCacheDir             () { return g_sys_cache_dir;         }
STR_CREF        GetSysLogsDir              () { return g_sys_logs_dir;          }
STR_CREF        GetSysResourcesDir         () { return g_sys_resources_dir;     }
bool            GetIoFFbackEnabled         () { return g_io_ffback_enabled;     }
float           GetIoFFbackCameraGain      () { return g_io_ffback_camera_gain; }
float           GetIoFFbackCenterGain      () { return g_io_ffback_center_gain; }
float           GetIoFFbackMasterGain      () { return g_io_ffback_master_gain; }
float           GetIoFFbackStressGain      () { return g_io_ffback_stress_gain; }
GfxShadowType   GetGfxShadowType           () { return (GfxShadowType)g_gfx_shadow_type;     }
GfxExtCamMode   GetGfxExternCamMode        () { return (GfxExtCamMode)g_gfx_extcam_mode;     }
GfxTexFilter    GetGfxTexFiltering         () { return (GfxTexFilter) g_gfx_texture_filter;  }
GfxVegetation   GetGfxVegetationMode       () { return (GfxVegetation)g_gfx_vegetation_mode; }
bool            GetGfxEnableSunburn        () { return g_gfx_enable_sunburn;       }
bool            GetGfxWaterUseWaves        () { return g_gfx_water_waves;          }
int             GetGfxParticlesMode        () { return g_gfx_particles_mode;       }
bool            GetGfxEnableGlow           () { return g_gfx_enable_glow;          }
bool            GetGfxEnableHdr            () { return g_gfx_enable_hdr;           }
bool            GetGfxEnableVideocams      () { return g_gfx_enable_videocams;     }
bool            GetGfxUseHeathaze          () { return g_gfx_enable_heathaze;      }
bool            GetGfxEnvmapEnabled        () { return g_gfx_envmap_enabled;       }
int             GetGfxEnvmapRate           () { return g_gfx_envmap_rate;          }
int             GetGfxSkidmarksMode        () { return g_gfx_skidmarks_mode;       }
bool            GetGfxMinimapDisabled      () { return g_gfx_minimap_disabled;     }
bool            GetDiagRigLogNodeImport    () { return g_diag_rig_log_node_import; }
bool            GetDiagRigLogNodeStats     () { return g_diag_rig_log_node_stats ; }
bool            GetDiagRigLogMessages      () { return g_diag_rig_log_messages   ; }
bool            GetDiagCollisions          () { return g_diag_collisions         ; }
bool            GetDiagTruckMass           () { return g_diag_truck_mass         ; }
bool            GetDiagEnvmap              () { return g_diag_envmap             ; }
STR_CREF        GetAppLanguage             () { return g_app_language;             }
STR_CREF        GetAppLocale               () { return g_app_locale;               }
bool            GetAppMultithread          () { return g_app_multithread;          }
STR_CREF        GetAppScreenshotFormat     () { return g_app_screenshot_format;    }
IoInputGrabMode GetIoInputGrabMode         () { return (IoInputGrabMode)g_io_input_grab_mode ; }
bool            GetIoArcadeControls        () { return g_io_arcade_controls;       }
float           GetAudioMasterVolume       () { return g_audio_master_volume;      }
bool            GetAudioEnableCreak        () { return g_audio_enable_creak;       }
STR_CREF        GetAudioDeviceName         () { return g_audio_device_name;        }
bool            GetAudioMenuMusic          () { return g_audio_menu_music;         }
bool            GetSimReplayEnabled        () { return g_sim_replay_enabled;       }
int             GetSimReplayLength         () { return g_sim_replay_length;        }
int             GetSimReplayStepping       () { return g_sim_replay_stepping;      }
bool            GetSimPositionStorage      () { return g_sim_position_storage;     }
SimGearboxMode  GetSimGearboxMode          () { return (SimGearboxMode)g_sim_gearbox_mode; }
GfxFlaresMode   GetGfxFlaresMode           () { return (GfxFlaresMode)g_gfx_flares_mode;   }
STR_CREF        GetSysScreenshotDir        () { return g_sys_screenshot_dir;       }
int             GetIoOutGaugeMode          () { return g_io_outgauge_mode;         }
STR_CREF        GetIoOutGaugeIp            () { return g_io_outgauge_ip;           }
int             GetIoOutGaugePort          () { return g_io_outgauge_port;         }
float           GetIoOutGaugeDelay         () { return g_io_outgauge_delay;        }
int             GetIoOutGaugeId            () { return g_io_outgauge_id;           }
GfxSkyMode      GetGfxSkyMode              () { return (GfxSkyMode)g_gfx_sky_mode; }
GfxWaterMode    GetGfxWaterMode            () { return (GfxWaterMode)g_gfx_water_mode; }
float           GetGfxSightRange           () { return g_gfx_sight_range;          }
float           GetGfxFovExternal          () { return g_gfx_fov_external;         }
float           GetGfxFovInternal          () { return g_gfx_fov_internal;         }
int             GetGfxFpsLimit             () { return g_gfx_fps_limit;            }
bool            GetDiagVideoCameras        () { return g_diag_videocameras;        }
STR_CREF        GetDiagPreselectedTerrain  () { return g_diag_preselected_terrain; }
STR_CREF        GetDiagPreselectedVehicle  () { return g_diag_preselected_vehicle; }
STR_CREF        GetDiagPreselectedVehConfig() { return g_diag_preselected_veh_config;  }
bool            GetDiagPreselectedVehEnter () { return g_diag_preselected_veh_enter;   }

// Setters
void SetActiveAppState       (State           v) { SetVarEnum    (g_app_state_active     , "app_state_active"     , (int)v, AppStateToStr); }
void SetPendingAppState      (State           v) { SetVarEnum    (g_app_state_pending    , "app_state_pending"    , (int)v, AppStateToStr); }
void SetSimActiveTerrain     (STR_CREF        v) { SetVarStr     (g_sim_active_terrain   , "sim_active_terrain"   , v); }
void SetSimNextTerrain       (STR_CREF        v) { SetVarStr     (g_sim_next_terrain     , "sim_next_terrain"     , v); }
void SetActiveSimState       (SimState        v) { SetVarEnum    (g_sim_state_active     , "sim_state_active"     , (int)v, SimStateToStr); }
void SetPendingSimState      (SimState        v) { SetVarEnum    (g_sim_state_pending    , "sim_state_pending"    , (int)v, SimStateToStr); }
void SetActiveMpState        (MpState         v) { SetVarEnum    (g_mp_state_active      , "mp_state_active"      , (int)v, MpStateToStr ); }
void SetPendingMpState       (MpState         v) { SetVarEnum    (g_mp_state_pending     , "mp_state_pending"     , (int)v, MpStateToStr ); }
void SetMpServerHost         (STR_CREF        v) { SetVarStr     (g_mp_server_host       , "mp_server_host"       , v); }
void SetMpServerPassword     (STR_CREF        v) { SetVarStr     (g_mp_server_password   , "mp_server_password"   , v); }
void SetMpServerPort         (int             v) { SetVarInt     (g_mp_server_port       , "mp_server_port"       , v); }
void SetMpPlayerName         (STR_CREF        v) { SetVarStr     (g_mp_player_name       , "mp_player_name"       , v); }
void SetMpPortalUrl          (STR_CREF        v) { SetVarStr     (g_mp_portal_url        , "mp_portal_url"        , v); }
void SetDiagTraceGlobals     (bool            v) { SetVarBool    (g_diag_trace_globals   , "diag_trace_globals"   , v); }
void SetSysProcessDir        (STR_CREF        v) { SetVarStr     (g_sys_process_dir      , "sys_process_dir"      , v); }
void SetSysUserDir           (STR_CREF        v) { SetVarStr     (g_sys_user_dir         , "sys_user_dir"         , v); }
void SetSysConfigDir         (STR_CREF        v) { SetVarStr     (g_sys_config_dir       , "sys_config_dir"       , v); }
void SetSysCacheDir          (STR_CREF        v) { SetVarStr     (g_sys_cache_dir        , "sys_cache_dir"        , v); }
void SetSysLogsDir           (STR_CREF        v) { SetVarStr     (g_sys_logs_dir         , "sys_logs_dir"         , v); }
void SetSysResourcesDir      (STR_CREF        v) { SetVarStr     (g_sys_resources_dir    , "sys_resources_dir"    , v); }
void SetIoFFbackEnabled      (bool            v) { SetVarBool    (g_io_ffback_enabled    , "io_ffback_enabled"    , v); }
void SetIoFFbackCameraGain   (float           v) { SetVarFloat   (g_io_ffback_camera_gain, "io_ffback_camera_gain", v); }
void SetIoFFbackCenterGain   (float           v) { SetVarFloat   (g_io_ffback_center_gain, "io_ffback_center_gain", v); }
void SetIoFFbackMasterGain   (float           v) { SetVarFloat   (g_io_ffback_master_gain, "io_ffback_master_gain", v); }
void SetIoFFbackStressGain   (float           v) { SetVarFloat   (g_io_ffback_stress_gain, "io_ffback_stress_gain", v); }
void SetGfxShadowType        (GfxShadowType   v) { SetVarEnum    (g_gfx_shadow_type      , "gfx_shadow_mode"      , (int)v, GfxShadowModeToStr); }
void SetGfxExternCamMode     (GfxExtCamMode   v) { SetVarInt     (g_gfx_extcam_mode      , "gfx_extcam_mode"      , (int)v); }
void SetGfxTexFiltering      (GfxTexFilter    v) { SetVarEnum    (g_gfx_texture_filter   , "gfx_texture_filter"   , (int)v, GfxTexFilterToStr ); }
void SetGfxVegetationMode    (GfxVegetation   v) { SetVarEnum    (g_gfx_vegetation_mode  , "gfx_vegetation_mode"  , (int)v, GfxVegetationModeToStr); }
void SetGfxEnableSunburn     (bool            v) { SetVarBool    (g_gfx_enable_sunburn   , "gfx_enable_sunburn"   , v); }
void SetGfxWaterUseWaves     (bool            v) { SetVarBool    (g_gfx_water_waves      , "gfx_water_waves"      , v); }
void SetGfxEnableGlow        (bool            v) { SetVarBool    (g_gfx_enable_glow      , "gfx_enable_glow"      , v); }
void SetGfxEnableHdr         (bool            v) { SetVarBool    (g_gfx_enable_hdr       , "gfx_enable_hdr"       , v); }
void SetGfxEnableVideocams   (bool            v) { SetVarBool    (g_gfx_enable_videocams , "gfx_enable_videocams" , v); }
void SetGfxUseHeathaze       (bool            v) { SetVarBool    (g_gfx_enable_heathaze  , "gfx_enable_heathaze"  , v); }
void SetGfxEnvmapEnabled     (bool            v) { SetVarBool    (g_gfx_envmap_enabled   , "gfx_envmap_enabled"   , v); }
void SetGfxEnvmapRate        (int             v) { SetVarInt     (g_gfx_envmap_rate      , "gfx_envmap_rate"      , v); }
void SetGfxSkidmarksMode     (int             v) { SetVarInt     (g_gfx_skidmarks_mode   , "gfx_skidmarks_mode"   , v); }
void SetGfxParticlesMode     (int             v) { SetVarInt     (g_gfx_particles_mode   , "gfx_particles_mode"   , v); }
void SetGfxMinimapDisabled   (bool            v) { SetVarBool    (g_gfx_minimap_disabled , "gfx_minimap_disabled" , v); }
void SetDiagRigLogNodeImport (bool            v) { SetVarBool    (g_diag_rig_log_node_import  , "diag_rig_log_node_import"  , v); }
void SetDiagRigLogNodeStats  (bool            v) { SetVarBool    (g_diag_rig_log_node_stats   , "diag_rig_log_node_stats"   , v); }
void SetDiagRigLogMessages   (bool            v) { SetVarBool    (g_diag_rig_log_messages     , "diag_rig_log_messages"     , v); }
void SetDiagCollisions       (bool            v) { SetVarBool    (g_diag_collisions      , "diag_collisions"           , v); }
void SetDiagTruckMass        (bool            v) { SetVarBool    (g_diag_truck_mass      , "diag_truck_mass"           , v); }
void SetDiagEnvmap           (bool            v) { SetVarBool    (g_diag_envmap          , "diag_envmap"               , v); }
void SetAppLanguage          (STR_CREF        v) { SetVarStr     (g_app_language         , "app_language"              , v); }
void SetAppLocale            (STR_CREF        v) { SetVarStr     (g_app_locale           , "app_locale"                , v); }
void SetAppMultithread       (bool            v) { SetVarBool    (g_app_multithread      , "app_multithread"           , v); }
void SetAppScreenshotFormat  (STR_CREF        v) { SetVarStr     (g_app_screenshot_format, "app_screenshot_format"     , v); }
void SetIoInputGrabMode      (IoInputGrabMode v) { SetVarEnum    (g_io_input_grab_mode   , "io_input_grab_mode",    (int)v, IoInputGrabModeToStr); }
void SetIoArcadeControls     (bool            v) { SetVarBool    (g_io_arcade_controls   , "io_arcade_controls"        , v); }
void SetAudioMasterVolume    (float           v) { SetVarFloat   (g_audio_master_volume  , "audio_master_volume"       , v); }
void SetAudioEnableCreak     (bool            v) { SetVarBool    (g_audio_enable_creak   , "audio_enable_creak"        , v); }
void SetAudioDeviceName      (STR_CREF        v) { SetVarStr     (g_audio_device_name    , "audio_device_name"         , v); }
void SetAudioMenuMusic       (bool            v) { SetVarBool    (g_audio_menu_music     , "audio_menu_music"          , v); }
void SetSimReplayEnabled     (bool            v) { SetVarBool    (g_sim_replay_enabled   , "sim_replay_enabled"        , v); }
void SetSimReplayLength      (int             v) { SetVarInt     (g_sim_replay_length    , "sim_replay_length"         , v); }
void SetSimReplayStepping    (int             v) { SetVarInt     (g_sim_replay_stepping  , "sim_replay_stepping"       , v); }
void SetSimPositionStorage   (bool            v) { SetVarBool    (g_sim_position_storage , "sim_position_storage"      , v); }
void SetSimGearboxMode       (SimGearboxMode  v) { SetVarEnum    (g_sim_gearbox_mode     , "sim_gearbox_mode",      (int)v, SimGearboxModeToString); }
void SetGfxFlaresMode        (GfxFlaresMode   v) { SetVarEnum    (g_gfx_flares_mode      , "gfx_flares_mode",       (int)v, GfxFlaresModeToString ); }
void SetSysScreenshotDir     (STR_CREF        v) { SetVarStr     (g_sys_screenshot_dir   , "sys_screenshot_dir"        , v); }
void SetIoOutGaugeMode       (int             v) { SetVarInt     (g_io_outgauge_mode     , "io_outgauge_mode"          , v); }
void SetIoOutGaugeIp         (STR_CREF        v) { SetVarStr     (g_io_outgauge_ip       , "io_outgauge_ip"            , v); }
void SetIoOutGaugePort       (int             v) { SetVarInt     (g_io_outgauge_port     , "io_outgauge_port"          , v); }
void SetIoOutGaugeDelay      (float           v) { SetVarFloat   (g_io_outgauge_delay    , "io_outgauge_delay"         , v); }
void SetIoOutGaugeId         (int             v) { SetVarInt     (g_io_outgauge_id       , "io_outgauge_id"            , v); }
void SetGfxSkyMode           (GfxSkyMode      v) { SetVarEnum    (g_gfx_sky_mode         , "gfx_sky_mode",          (int)v, GfxSkyModeToString   ); }
void SetGfxWaterMode         (GfxWaterMode    v) { SetVarEnum    (g_gfx_water_mode       , "gfx_water_mode",        (int)v, GfxWaterModeToString ); }
void SetGfxSightRange        (float           v) { SetVarFloat   (g_gfx_sight_range      , "gfx_sight_range"           , v); }
void SetGfxFovExternal       (float           v) { SetVarFloat   (g_gfx_fov_external     , "gfx_fov_external"          , v); }
void SetGfxFovInternal       (float           v) { SetVarFloat   (g_gfx_fov_internal     , "gfx_fov_internal"          , v); }
void SetGfxFpsLimit          (int             v) { SetVarInt     (g_gfx_fps_limit        , "gfx_fps_limit"             , v); }
void SetDiagVideoCameras     (bool            v) { SetVarBool    (g_diag_videocameras    , "diag_videocamera"          , v); }
void SetDiagPreselectedTerrain  (STR_CREF     v) { SetVarStr     (g_diag_preselected_terrain   , "diag_preselected_terrain"   , v); }
void SetDiagPreselectedVehicle  (STR_CREF     v) { SetVarStr     (g_diag_preselected_vehicle   , "diag_preselected_vehicle"   , v); }
void SetDiagPreselectedVehConfig(STR_CREF     v) { SetVarStr     (g_diag_preselected_veh_config, "diag_preselected_veh_config", v); }
void SetDiagPreselectedVehEnter (bool         v) { SetVarBool    (g_diag_preselected_veh_enter , "diag_preselected_veh_enter" , v); }

// Instance access
OgreSubsystem*         GetOgreSubsystem      () { return g_ogre_subsystem; };
Settings&              GetSettings           () { return Settings::getSingleton(); } // Temporary solution
ContentManager*        GetContentManager     () { return g_content_manager;}
OverlayWrapper*        GetOverlayWrapper     () { return g_overlay_wrapper;}
SceneMouse*            GetSceneMouse         () { return g_scene_mouse;}
GUIManager*            GetGuiManager         () { return g_gui_manager;}
Console*               GetConsole            () { return g_gui_manager->GetConsole();}
InputEngine*           GetInputEngine        () { return g_input_engine;}
CacheSystem*           GetCacheSystem        () { return g_cache_system;}
MainMenu*              GetMainMenu           () { return g_main_menu;}

// Instance management
void SetMainMenu(MainMenu* obj) { g_main_menu = obj; }

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

void Init()
{
    g_app_state_active     = APP_STATE_BOOTSTRAP;
    g_app_state_pending    = APP_STATE_MAIN_MENU;
    g_app_language         = "English";
    g_app_locale           = "en";
    g_app_screenshot_format= "jpg";
    g_app_multithread      = true;

    g_mp_state_active      = MP_STATE_DISABLED;
    g_mp_state_pending     = MP_STATE_NONE;
    g_mp_player_name       = "Anonymous";
    g_mp_portal_url        = "http://multiplayer.rigsofrods.org";

    g_sim_state_active     = SIM_STATE_NONE;
    g_sim_state_pending    = SIM_STATE_NONE;
    g_sim_replay_stepping  = 240;
    g_sim_replay_length    = 1000;

    g_diag_trace_globals   = false; // Don't init to 'true', logger is not ready at startup.

    g_gfx_shadow_type      = GFX_SHADOW_TYPE_NONE;
    g_gfx_extcam_mode      = GFX_EXTCAM_MODE_PITCHING;
    g_gfx_texture_filter   = GFX_TEXFILTER_TRILINEAR;
    g_gfx_vegetation_mode  = GFX_VEGETATION_NONE;
    g_gfx_flares_mode      = GFX_FLARES_ALL_VEHICLES_HEAD_ONLY;
    g_gfx_water_mode       = GFX_WATER_BASIC;
    g_gfx_sky_mode         = GFX_SKY_SANDSTORM;
    g_gfx_sight_range      = 3000.f; // Previously either 2000 or 4500 (inconsistent)
    g_gfx_fov_external     = 60.f;
    g_gfx_fov_internal     = 75.f;
    g_gfx_fps_limit        = 0; // Unlimited
    g_gfx_enable_videocams = true;

    g_io_outgauge_ip       = "192.168.1.100";
    g_io_outgauge_port     = 1337;
    g_io_outgauge_delay    = 10.f;
    g_io_outgauge_mode     = 0; // 0 = disabled, 1 = enabled
    g_io_outgauge_id       = 0;
}


// ================================================================================
// Private helper functions
// ================================================================================


void LogVarUpdate(const char* name, const char* old_value, const char* new_value)
{
    if (g_diag_trace_globals && (strcmp(old_value, new_value) != 0))
    {
        char log[1000] = "";
        snprintf(log, 1000, "[RoR|Globals] Updating \"%s\": [%s] => [%s]", name, old_value, new_value);
        LOG(log);
    }
}

void SetVarStr (std::string& var, const char* var_name, std::string const & new_value)
{
    LogVarUpdate(var_name, var.c_str(), new_value.c_str());
    var = new_value;
}

void SetVarInt (int& var, const char* var_name, int new_value)
{
    if (g_diag_trace_globals && (var != new_value))
    {
        char log[1000] = "";
        snprintf(log, 1000, "[RoR|Globals] Updating \"%s\": [%d] => [%d]", var_name, var, new_value);
        LOG(log);
    }
    var = new_value;
}

void SetVarEnum (int& var, const char* var_name, int new_value, EnumToStringFn enum_to_str_fn)
{
    LogVarUpdate(var_name, (*enum_to_str_fn)(var), (*enum_to_str_fn)(new_value));
    var = new_value;
}

void SetVarBool (bool& var, const char* var_name, bool new_value)
{
    LogVarUpdate(var_name, (var ? "True" : "False"), (new_value ? "True" : "False"));
    var = new_value;
}

void SetVarFloat(float& var, const char* var_name, float new_value)
{
    if (g_diag_trace_globals && (var != new_value))
    {
        char log[1000] = "";
        snprintf(log, 1000, "[RoR|Globals] Updating \"%s\": [%f] => [%f]", var_name, var, new_value);
        LOG(log);
    }
    var = new_value;
}

const char* AppStateToStr(int v)
{
    switch ((State)v)
    {
    case App::APP_STATE_NONE:                return "NONE";
    case App::APP_STATE_BOOTSTRAP:           return "BOOTSTRAP";
    case App::APP_STATE_CHANGE_MAP:          return "CHANGE_MAP";
    case App::APP_STATE_MAIN_MENU:           return "MAIN_MENU";
    case App::APP_STATE_PRINT_HELP_EXIT:     return "PRINT_HELP_EXIT";
    case App::APP_STATE_PRINT_VERSION_EXIT:  return "PRINT_VERSION_EXIT";
    case App::APP_STATE_SHUTDOWN:            return "SHUTDOWN";
    case App::APP_STATE_SIMULATION:          return "SIMULATION";
    default:                                 return "~invalid~";
    }
}

const char* MpStateToStr(int v)
{
    switch ((MpState)v)
    {
    case App::MP_STATE_NONE:      return "NONE";
    case App::MP_STATE_DISABLED:  return "DISABLED";
    case App::MP_STATE_CONNECTED: return "CONNECTED";
    default:                      return "~invalid~";
    }
}

const char* SimStateToStr(int v)
{
    switch ((SimState)v)
    {
    case App::SIM_STATE_NONE       : return "NONE";
    case App::SIM_STATE_RUNNING    : return "RUNNING";
    case App::SIM_STATE_PAUSED     : return "PAUSED";
    case App::SIM_STATE_SELECTING  : return "SELECTING";
    case App::SIM_STATE_EDITOR_MODE: return "EDITOR_MODE";
    default                        : return "~invalid~";
    }
}

const char* SimGearboxModeToString(int v)
{
    switch ((SimGearboxMode)v)
    {
    case SIM_GEARBOX_AUTO         : return "AUTO";
    case SIM_GEARBOX_SEMI_AUTO    : return "SEMI_AUTO";
    case SIM_GEARBOX_MANUAL       : return "MANUAL";
    case SIM_GEARBOX_MANUAL_STICK : return "MANUAL_STICK";
    case SIM_GEARBOX_MANUAL_RANGES: return "MANUAL_RANGES";
    default                       : return "~invalid~";
    }
}

const char* GfxFlaresModeToString(int v)
{
    switch ((GfxFlaresMode)v)
    {
    case GFX_FLARES_NONE                   : return "NONE"                   ;
    case GFX_FLARES_NO_LIGHTSOURCES        : return "NO_LIGHTSOURCES"        ;
    case GFX_FLARES_CURR_VEHICLE_HEAD_ONLY : return "CURR_VEHICLE_HEAD_ONLY" ;
    case GFX_FLARES_ALL_VEHICLES_HEAD_ONLY : return "ALL_VEHICLES_HEAD_ONLY" ;
    case GFX_FLARES_ALL_VEHICLES_ALL_LIGHTS: return "ALL_VEHICLES_ALL_LIGHTS";
    default                                : return "~invalid~";
    }
}

const char* GfxVegetationModeToStr (int v)
{
    switch((GfxWaterMode)v)
    {
    case GFX_VEGETATION_NONE   : return "NONE";
    case GFX_VEGETATION_20PERC : return "20%";
    case GFX_VEGETATION_50PERC : return "50%";
    case GFX_VEGETATION_FULL   : return "FULL";
    default                    : return "~invalid~";
    }
}

const char* GfxWaterModeToString (int v)
{
    switch((GfxWaterMode)v)
    {
    case GFX_WATER_NONE      : return "NONE";
    case GFX_WATER_BASIC     : return "BASIC";
    case GFX_WATER_REFLECT   : return "REFLECT";
    case GFX_WATER_FULL_FAST : return "FULL_FAST";
    case GFX_WATER_FULL_HQ   : return "FULL_HQ";
    case GFX_WATER_HYDRAX    : return "HYDRAX";
    default                  : return "~invalid~";
    }
}

const char* GfxSkyModeToString (int v)
{
    switch((GfxSkyMode)v)
    {
    case GFX_SKY_SANDSTORM: return "SANDSTORM";
    case GFX_SKY_CAELUM   : return "CAELUM";
    case GFX_SKY_SKYX     : return "SKYX";
    default               : return "~invalid~";
    }
}

const char* IoInputGrabModeToStr(int v)
{
    switch ((IoInputGrabMode)v)
    {
    case INPUT_GRAB_NONE   : return "NONE"   ;
    case INPUT_GRAB_ALL    : return "ALL"    ;
    case INPUT_GRAB_DYNAMIC: return "DYNAMIC";
    default                : return "~invalid~";
    }
}

const char* GfxShadowModeToStr(int v)
{
    switch((GfxShadowType)v)
    {
    case GFX_SHADOW_TYPE_NONE   : return "NONE";
    case GFX_SHADOW_TYPE_TEXTURE: return "TEXTURE";
    case GFX_SHADOW_TYPE_PSSM   : return "PSSM";
    default                     : return "~invalid~";
    }
}

const char* GfxTexFilterToStr(int v)
{
    switch ((GfxTexFilter)v)
    {
    case GFX_TEXFILTER_NONE       : return "NONE";
    case GFX_TEXFILTER_BILINEAR   : return "BILINEAR";
    case GFX_TEXFILTER_TRILINEAR  : return "TRILINEAR";
    case GFX_TEXFILTER_ANISOTROPIC: return "ANISOTROPIC";
    default                       : return "~invalid~";
    }
}

} // namespace Application

} // namespace RoR
