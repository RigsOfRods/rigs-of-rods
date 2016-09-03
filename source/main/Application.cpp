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
#include "Console.h"
#include "ContentManager.h"
#include "GUIManager.h"
#include "InputEngine.h"
#include "OgreSubsystem.h"
#include "OverlayWrapper.h"
#include "SceneMouse.h"

namespace RoR {
namespace Application {

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
State                   g_app_state_active;      ///< Current state
State                   g_app_state_pending;     ///< Requested state change

// Simulation
std::string             g_sim_terrain_active;
std::string             g_sim_terrain_pending;   // Replaces old SSETTING("Preselected Map") See 'Settings.h'

// Multiplayer
MpState                 g_mp_state_active;       ///< Current state
MpState                 g_mp_state_pending;      ///< Requested state change
std::string             g_mp_server_host;        // Replaces old SSETTING("Server name")     See 'Settings.h'
int                     g_mp_server_port;        // Replaces old ISETTING("Server port")     See 'Settings.h'
std::string             g_mp_server_password;    // Replaces old SSETTING("Server password") See 'Settings.h'
std::string             g_mp_player_name;        // Replaces old SSETTING("Nickname")        See 'Settings.h'

// Diagnostic
bool                    g_diag_trace_globals;

// ================================================================================
// Access functions
// ================================================================================

// Helpers (forward decl.)
void SetVarStr      (std::string&          var, const char* var_name, std::string const &  new_value);
void SetVarInt      (int&                  var, const char* var_name, int                  new_value);
void SetVarBool     (bool&                 var, const char* var_name, bool                 new_value);
void SetVarAppState (Application::State&   var, const char* var_name, Application::State   new_value);
void SetVarMpState  (Application::MpState& var, const char* var_name, Application::MpState new_value);

// Getters
State         GetActiveAppState()     { return g_app_state_active;      }
State         GetPendingAppState()    { return g_app_state_pending;     }
std::string   GetActiveTerrain()      { return g_sim_terrain_active;    }
std::string   GetPendingTerrain()     { return g_sim_terrain_pending;   }
MpState       GetActiveMpState()      { return g_mp_state_active;       }
MpState       GetPendingMpState()     { return g_mp_state_pending;      }
std::string   GetMpServerHost()       { return g_mp_server_host;        }
std::string   GetMpServerPassword()   { return g_mp_server_password;    }
int           GetMpServerPort()       { return g_mp_server_port;        }
std::string   GetMpPlayerName()       { return g_mp_player_name;        }
bool          GetDiagTraceGlobals()   { return g_diag_trace_globals;    }

// Setters
void SetActiveAppState    (State               v) { SetVarAppState(g_app_state_active     , "app_state_active"     , v); }
void SetPendingAppState   (State               v) { SetVarAppState(g_app_state_pending    , "app_state_pending"    , v); }
void SetActiveTerrain     (std::string const & v) { SetVarStr     (g_sim_terrain_active   , "sim_terrain_active"   , v); }
void SetPendingTerrain    (std::string const & v) { SetVarStr     (g_sim_terrain_pending  , "sim_terrain_pending"  , v); }
void SetActiveMpState     (MpState             v) { SetVarMpState (g_mp_state_active      , "mp_state_active"      , v); }
void SetPendingMpState    (MpState             v) { SetVarMpState (g_mp_state_pending     , "mp_state_pending"     , v); }
void SetMpServerHost      (std::string const & v) { SetVarStr     (g_mp_server_host       , "mp_server_host"       , v); }
void SetMpServerPassword  (std::string const & v) { SetVarStr     (g_mp_server_password   , "mp_server_password"   , v); }
void SetMpServerPort      (int         const & v) { SetVarInt     (g_mp_server_port       , "mp_server_port"       , v); }
void SetMpPlayerName      (std::string const & v) { SetVarStr     (g_mp_player_name       , "mp_player_name"       , v); }
void SetDiagTraceGlobals  (bool                v) { SetVarBool    (g_diag_trace_globals   , "diag_trace_globals"   , v); }

// Instance access
OgreSubsystem*         GetOgreSubsystem      (){ return g_ogre_subsystem; };
Settings&              GetSettings           (){ return Settings::getSingleton(); } // Temporary solution
ContentManager*        GetContentManager     (){ return g_content_manager;}
OverlayWrapper*        GetOverlayWrapper     (){ return g_overlay_wrapper;}
SceneMouse*            GetSceneMouse         (){ return g_scene_mouse;}
GUIManager*            GetGuiManager         (){ return g_gui_manager;}
GuiManagerInterface*   GetGuiManagerInterface(){ return static_cast<GuiManagerInterface*>(g_gui_manager);}
Console*               GetConsole            (){ return g_console;}
InputEngine*           GetInputEngine        (){ return g_input_engine;}
CacheSystem*           GetCacheSystem        (){ return g_cache_system;}
MainThread*            GetMainThreadLogic    (){ return g_main_thread_logic;}

// Instance management
void SetMainThreadLogic(MainThread* obj) { g_main_thread_logic = obj; }

void StartOgreSubsystem()
{
	g_ogre_subsystem = new OgreSubsystem();
	if (g_ogre_subsystem == nullptr)
	{
		throw std::runtime_error("[RoR] Failed to create OgreSubsystem");
	}

	if (! g_ogre_subsystem->StartOgre("RoR", "", ""))
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

void CreateConsoleIfNotExists()
{
	if (g_console == nullptr)
	{
		g_console = new Console();
	}
}

void DeleteConsoleIfExists()
{
	if (g_console != nullptr)
	{
		delete g_console;
		g_console = nullptr;
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

    g_diag_trace_globals   = false; // Don't init to 'true', logger is not ready at startup.
}

// ================================================================================
// Private helper functions
// ================================================================================

void LogVarUpdate(const char* name, std::string const & old_value, std::string const & new_value)
{
    if (g_diag_trace_globals && (old_value != new_value))
    {
        char log[1000] = "";
        snprintf(log, 1000, "[RoR|Globals] Updating \"%s\": [%s] => [%s]", name, old_value.c_str(), new_value.c_str());
        LOG(log);
    }
}

void SetVarStr (std::string& var, const char* var_name, std::string const & new_value)
{
    LogVarUpdate(var_name, var, new_value);
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

const char* AppStateToStr(Application::State s)
{
    switch (s)
    {
    case Application::APP_STATE_NONE:                return "NONE";
    case Application::APP_STATE_BOOTSTRAP:           return "BOOTSTRAP";
    case Application::APP_STATE_CHANGE_MAP:          return "CHANGE_MAP";
    case Application::APP_STATE_MAIN_MENU:           return "MAIN_MENU";
    case Application::APP_STATE_PRINT_HELP_EXIT:     return "PRINT_HELP_EXIT";
    case Application::APP_STATE_PRINT_VERSION_EXIT:  return "PRINT_VERSION_EXIT";
    case Application::APP_STATE_SHUTDOWN:            return "SHUTDOWN";
    case Application::APP_STATE_SIMULATION:          return "SIMULATION";
    default:                                         return "~invalid~";
    }
}

const char* MpStateToStr(Application::MpState s)
{
    switch (s)
    {
    case Application::MP_STATE_NONE: return "NONE";
    case Application::MP_STATE_DISABLED: return "DISABLED";
    case Application::MP_STATE_CONNECTED: return "CONNECTED";
    default: return "~invalid~";
    }
}

void SetVarAppState (Application::State& var, const char* var_name, Application::State new_value)
{
    LogVarUpdate(var_name, AppStateToStr(var), AppStateToStr(new_value));
    var = new_value;
}

void SetVarMpState (Application::MpState& var, const char* var_name, Application::MpState new_value)
{
    LogVarUpdate(var_name, MpStateToStr(var), MpStateToStr(new_value));
    var = new_value;
}

} // namespace Application

} // namespace RoR
