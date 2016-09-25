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
static MainThread*      g_main_thread_logic;

// App state
static State            g_app_state_active;      ///< Current state
static State            g_app_state_pending;     ///< Requested state change

// Simulation
static std::string      g_sim_terrain_active;
static std::string      g_sim_terrain_pending;   // Replaces old SSETTING("Preselected Map") See 'Settings.h'
static SimState         g_sim_state_active;      ///< Current state
static SimState         g_sim_state_pending;     ///< Requested state change

// Multiplayer
static MpState          g_mp_state_active;       ///< Current state
static MpState          g_mp_state_pending;      ///< Requested state change
static std::string      g_mp_server_host;        // Replaces old SSETTING("Server name")     See 'Settings.h'
static int              g_mp_server_port;        // Replaces old ISETTING("Server port")     See 'Settings.h'
static std::string      g_mp_server_password;    // Replaces old SSETTING("Server password") See 'Settings.h'
static std::string      g_mp_player_name;        // Replaces old SSETTING("Nickname")        See 'Settings.h'

// Diagnostic
static bool             g_diag_trace_globals;

// System
static std::string      g_sys_process_dir;       ///< No ending slash.
static std::string      g_sys_user_dir;          ///< No ending slash.
static std::string      g_sys_config_dir;        ///< No ending slash. Replaces settings["Config Root"]
static std::string      g_sys_cache_dir;         ///< No ending slash. Replaces settings["Cache Path"] 
static std::string      g_sys_logs_dir;          ///< No ending slash. Replaces settings["Log Path"]
static std::string      g_sys_resources_dir;     ///< No ending slash. Replaces settings["Resources Path"]

// Input
static bool             g_input_ff_enabled;      ///< Config: BSETTING Force Feedback
static float            g_input_ff_camera;       ///< Config: FSETTING Force Feedback Camera
static float            g_input_ff_centering;    ///< Config: FSETTING Force Feedback Centering
static float            g_input_ff_gain;         ///< Config: FSETTING Force Feedback Gain
static float            g_input_ff_stress;       ///< Config: FSETTING Force Feedback Stress

// Graphics
static int              g_gfx_shadow_type;       ///< Config: STR   Shadow technique
static int              g_gfx_extcam_mode;       ///< Config: STR   External Camera Mode
  //   std::string      g_gfx_sky_effects;       ///< Config: STR   Sky effects
static int              g_gfx_texture_filter;    ///< Config: STR   Texture Filtering
static int              g_gfx_vegetation_mode;   ///< Config: STR   Vegetation
  //   std::string      g_gfx_water_mode;        ///< Config: STR   Water effects
static bool             g_gfx_enable_sunburn;    ///< Config: BOOL  Sunburn
static bool             g_gfx_water_waves;       ///< Config: BOOL  Waves
static int              g_gfx_minimap_mode;      ///< Config: BOOL  disableOverViewMap
static int              g_gfx_particles_mode;    ///< Config: BOOL  Particles
static bool             g_gfx_enable_glow;       ///< Config: BOOL  Glow
static bool             g_gfx_enable_hdr;        ///< Config: BOOL  HDR
static bool             g_gfx_enable_heathaze;   ///< Config: BOOL  HeatHaze
static int              g_gfx_envmap_mode;       ///< Config: BOOL  Envmap & INT EnvmapUpdateRate
static int              g_gfx_skidmarks_mode;    ///< Config: BOOL  Skidmarks
  //   float            g_gfx_sight_range;       ///< Config: FLOAT SightRange
  //   float            g_gfx_fov_external;      ///< Config: FLOAT FOV External
  //   float            g_gfx_fov_internal;      ///< Config: FLOAT FOV Internal
  //   int              g_gfx_fps_limit;         ///< Config: INT   FPS-Limiter


// ================================================================================
// Access functions
// ================================================================================


// Helpers (forward decl.)
void SetVarStr      (std::string&     var, const char* var_name, STR_CREF        new_value);
void SetVarInt      (int&             var, const char* var_name, int             new_value);
void SetVarBool     (bool&            var, const char* var_name, bool            new_value);
void SetVarFloat    (float&           var, const char* var_name, float           new_value);
void SetVarAppState (App::State&      var, const char* var_name, App::State      new_value);
void SetVarMpState  (App::MpState&    var, const char* var_name, App::MpState    new_value);
void SetVarSimState (App::SimState&   var, const char* var_name, App::SimState   new_value);

// Getters
State           GetActiveAppState()     { return g_app_state_active;      }
State           GetPendingAppState()    { return g_app_state_pending;     }
STR_CREF        GetActiveTerrain()      { return g_sim_terrain_active;    }
STR_CREF        GetPendingTerrain()     { return g_sim_terrain_pending;   }
SimState        GetActiveSimState()     { return g_sim_state_active;      }
SimState        GetPendingSimState()    { return g_sim_state_pending;     }
MpState         GetActiveMpState()      { return g_mp_state_active;       }
MpState         GetPendingMpState()     { return g_mp_state_pending;      }
STR_CREF        GetMpServerHost()       { return g_mp_server_host;        }
STR_CREF        GetMpServerPassword()   { return g_mp_server_password;    }
int             GetMpServerPort()       { return g_mp_server_port;        }
STR_CREF        GetMpPlayerName()       { return g_mp_player_name;        }
bool            GetDiagTraceGlobals()   { return g_diag_trace_globals;    }
STR_CREF        GetSysProcessDir()      { return g_sys_process_dir;       }
STR_CREF        GetSysUserDir()         { return g_sys_user_dir;          }
STR_CREF        GetSysConfigDir()       { return g_sys_config_dir;        }
STR_CREF        GetSysCacheDir()        { return g_sys_cache_dir;         }
STR_CREF        GetSysLogsDir()         { return g_sys_logs_dir;          }
STR_CREF        GetSysResourcesDir()    { return g_sys_resources_dir;     }
bool            GetInputFFEnabled()     { return g_input_ff_enabled;      }
float           GetInputFFCamera()      { return g_input_ff_camera;       }
float           GetInputFFCentering()   { return g_input_ff_centering;    }
float           GetInputFFGain()        { return g_input_ff_gain;         }
float           GetInputFFStress()      { return g_input_ff_stress;       }
GfxShadowType   GetGfxShadowType()      { return (GfxShadowType)g_gfx_shadow_type; }
GfxExtCamMode   GetGfxExternCamMode()   { return (GfxExtCamMode)g_gfx_extcam_mode; }
GfxTexFilter    GetGfxTexFiltering()    { return (GfxTexFilter) g_gfx_texture_filter; }
GfxVegetation   GetGfxVegetationMode()  { return (GfxVegetation)g_gfx_vegetation_mode; }
bool            GetGfxEnableSunburn()   { return g_gfx_enable_sunburn ; }
bool            GetGfxWaterUseWaves()   { return g_gfx_water_waves    ; }
int             GetGfxMinimapMode  ()   { return g_gfx_minimap_mode   ; }
int             GetGfxParticlesMode()   { return g_gfx_particles_mode ; }
bool            GetGfxEnableGlow   ()   { return g_gfx_enable_glow    ; }
bool            GetGfxEnableHdr    ()   { return g_gfx_enable_hdr     ; }
bool            GetGfxUseHeathaze  ()   { return g_gfx_enable_heathaze; }
int             GetGfxEnvmapMode   ()   { return g_gfx_envmap_mode    ; }
int             GetGfxSkidmarksMode()   { return g_gfx_skidmarks_mode ; }

// Setters
void SetActiveAppState    (State    v) { SetVarAppState(g_app_state_active     , "app_state_active"     , v); }
void SetPendingAppState   (State    v) { SetVarAppState(g_app_state_pending    , "app_state_pending"    , v); }
void SetActiveTerrain     (STR_CREF v) { SetVarStr     (g_sim_terrain_active   , "sim_terrain_active"   , v); }
void SetPendingTerrain    (STR_CREF v) { SetVarStr     (g_sim_terrain_pending  , "sim_terrain_pending"  , v); }
void SetActiveSimState    (SimState v) { SetVarSimState(g_sim_state_active     , "sim_state_active"     , v); }
void SetPendingSimState   (SimState v) { SetVarSimState(g_sim_state_pending    , "sim_state_pending"    , v); }
void SetActiveMpState     (MpState  v) { SetVarMpState (g_mp_state_active      , "mp_state_active"      , v); }
void SetPendingMpState    (MpState  v) { SetVarMpState (g_mp_state_pending     , "mp_state_pending"     , v); }
void SetMpServerHost      (STR_CREF v) { SetVarStr     (g_mp_server_host       , "mp_server_host"       , v); }
void SetMpServerPassword  (STR_CREF v) { SetVarStr     (g_mp_server_password   , "mp_server_password"   , v); }
void SetMpServerPort      (int      v) { SetVarInt     (g_mp_server_port       , "mp_server_port"       , v); }
void SetMpPlayerName      (STR_CREF v) { SetVarStr     (g_mp_player_name       , "mp_player_name"       , v); }
void SetDiagTraceGlobals  (bool     v) { SetVarBool    (g_diag_trace_globals   , "diag_trace_globals"   , v); }
void SetSysProcessDir     (STR_CREF v) { SetVarStr     (g_sys_process_dir      , "sys_process_dir"      , v); }
void SetSysUserDir        (STR_CREF v) { SetVarStr     (g_sys_user_dir         , "sys_user_dir"         , v); }
void SetSysConfigDir      (STR_CREF v) { SetVarStr     (g_sys_config_dir       , "sys_config_dir"       , v); }
void SetSysCacheDir       (STR_CREF v) { SetVarStr     (g_sys_cache_dir        , "sys_cache_dir"        , v); }
void SetSysLogsDir        (STR_CREF v) { SetVarStr     (g_sys_logs_dir         , "sys_logs_dir"         , v); }
void SetSysResourcesDir   (STR_CREF v) { SetVarStr     (g_sys_resources_dir    , "sys_resources_dir"    , v); }
void SetInputFFEnabled    (bool     v) { SetVarBool    (g_input_ff_enabled     , "input_ff_enabled"     , v); }
void SetInputFFCamera     (float    v) { SetVarFloat   (g_input_ff_camera      , "input_ff_camera"      , v); }
void SetInputFFCentering  (float    v) { SetVarFloat   (g_input_ff_centering   , "input_ff_centering"   , v); }
void SetInputFFGain       (float    v) { SetVarFloat   (g_input_ff_gain        , "input_ff_gain"        , v); }
void SetInputFFStress     (float    v) { SetVarFloat   (g_input_ff_stress      , "input_ff_stress"      , v); }
void SetGfxShadowType     (GfxShadowType  v) { SetVarInt     (g_gfx_shadow_type      , "gfx_shadow_mode"      , (int)v); }
void SetGfxExternCamMode  (GfxExtCamMode  v) { SetVarInt     (g_gfx_extcam_mode      , "gfx_extcam_mode"      , (int)v); }
void SetGfxTexFiltering   (GfxTexFilter   v) { SetVarInt     (g_gfx_texture_filter   , "gfx_texture_filter"   , (int)v); }
void SetGfxVegetationMode (GfxVegetation  v) { SetVarInt     (g_gfx_vegetation_mode  , "gfx_vegetation_mode"  , (int)v); }
void SetGfxEnableSunburn  (bool           v) { SetVarBool    (g_gfx_enable_sunburn   , "gfx_enable_sunburn"   , v); }
void SetGfxWaterUseWaves  (bool           v) { SetVarBool    (g_gfx_water_waves      , "gfx_water_waves"      , v); }
void SetGfxEnableGlow     (bool           v) { SetVarBool    (g_gfx_enable_glow      , "gfx_enable_glow"      , v); }
void SetGfxEnableHdr      (bool           v) { SetVarBool    (g_gfx_enable_hdr       , "gfx_enable_hdr"       , v); }
void SetGfxUseHeathaze    (bool           v) { SetVarBool    (g_gfx_enable_heathaze  , "gfx_enable_heathaze"  , v); }
void SetGfxEnvmapMode     (int            v) { SetVarInt     (g_gfx_envmap_mode      , "gfx_envmap_mode"      , v); }
void SetGfxSkidmarksMode  (int            v) { SetVarInt     (g_gfx_skidmarks_mode   , "gfx_skidmarks_mode"   , v); }
void SetGfxMinimapMode    (int            v) { SetVarInt     (g_gfx_minimap_mode     , "gfx_minimap_mode"     , v); }
void SetGfxParticlesMode  (int            v) { SetVarInt     (g_gfx_particles_mode   , "gfx_particles_mode"   , v); }

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
MainThread*            GetMainThreadLogic    () { return g_main_thread_logic;}

// Instance management
void SetMainThreadLogic(MainThread* obj) { g_main_thread_logic = obj; }

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

    g_mp_state_active      = MP_STATE_DISABLED;
    g_mp_state_pending     = MP_STATE_NONE;
    g_mp_player_name       = "Anonymous";

    g_sim_state_active     = SIM_STATE_NONE;
    g_sim_state_pending    = SIM_STATE_NONE;

    g_diag_trace_globals   = false; // Don't init to 'true', logger is not ready at startup.

    g_gfx_shadow_type      = GFX_SHADOW_TYPE_PSSM;
    g_gfx_extcam_mode      = GFX_EXTCAM_MODE_PITCHING;
    g_gfx_texture_filter   = GFX_TEXFILTER_TRILINEAR;
    g_gfx_vegetation_mode  = GFX_VEGETATION_NONE;
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

const char* AppStateToStr(App::State s)
{
    switch (s)
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

const char* MpStateToStr(App::MpState s)
{
    switch (s)
    {
    case App::MP_STATE_NONE:      return "NONE";
    case App::MP_STATE_DISABLED:  return "DISABLED";
    case App::MP_STATE_CONNECTED: return "CONNECTED";
    default:                      return "~invalid~";
    }
}

const char* SimStateToStr(App::SimState s)
{
    switch (s)
    {
    case App::SIM_STATE_NONE       : return "NONE";
    case App::SIM_STATE_RUNNING    : return "RUNNING";
    case App::SIM_STATE_PAUSED     : return "PAUSED";
    case App::SIM_STATE_SELECTING  : return "SELECTING";
    case App::SIM_STATE_EDITOR_MODE: return "EDITOR_MODE";
    default                        : return "~invalid~";
    }
}

void SetVarAppState (App::State& var, const char* var_name, App::State new_value)
{
    LogVarUpdate(var_name, AppStateToStr(var), AppStateToStr(new_value));
    var = new_value;
}

void SetVarMpState (App::MpState& var, const char* var_name, App::MpState new_value)
{
    LogVarUpdate(var_name, MpStateToStr(var), MpStateToStr(new_value));
    var = new_value;
}

void SetVarSimState (App::SimState& var, const char* var_name, App::SimState new_value)
{
    LogVarUpdate(var_name, SimStateToStr(var), SimStateToStr(new_value));
    var = new_value;
}

} // namespace Application

} // namespace RoR
