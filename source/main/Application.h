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
	@file   Application.h
	@author Petr Ohlidal
	@date   05/2014
	@brief  Central state/object manager and communications hub.
*/

#pragma once

#include "RoRPrerequisites.h"
#include "Settings.h"

#define STR       std::string
#define STR_CREF  std::string const &

namespace RoR {
namespace App {

enum State
{
    APP_STATE_NONE,               ///< Only valid for GVar 'app_state_pending'. Means no change is requested.
    APP_STATE_BOOTSTRAP,          ///< Initial state
    APP_STATE_MAIN_MENU,
    APP_STATE_CHANGE_MAP,         ///< Enter main menu & immediatelly launch singleplayer map selector.
    APP_STATE_SIMULATION,
    APP_STATE_SHUTDOWN,
    APP_STATE_PRINT_HELP_EXIT,
    APP_STATE_PRINT_VERSION_EXIT,
};

enum MpState
{
    MP_STATE_NONE,      ///< Only valid for GVar 'app_state_pending'. Means no change is requested.
    MP_STATE_DISABLED,  ///< Not connected for whatever reason.
    MP_STATE_CONNECTED,
};

enum SimState
{
    SIM_STATE_NONE,
    SIM_STATE_RUNNING,
    SIM_STATE_PAUSED,
    SIM_STATE_SELECTING,  ///< The selector GUI window is displayed.
    SIM_STATE_EDITOR_MODE ///< Hacky, but whatever... added by Ulteq, 2016
};

enum GfxShadowType
{
    GFX_SHADOW_TYPE_NONE,
    GFX_SHADOW_TYPE_TEXTURE,
    GFX_SHADOW_TYPE_PSSM
};

enum GfxExtCamMode
{
    GFX_EXTCAM_MODE_NONE,
    GFX_EXTCAM_MODE_STATIC,
    GFX_EXTCAM_MODE_PITCHING,
};

enum GfxTexFilter
{
    GFX_TEXFILTER_NONE,
    GFX_TEXFILTER_BILINEAR,
    GFX_TEXFILTER_TRILINEAR,
    GFX_TEXFILTER_ANISOTROPIC,
}

void Init();

// Getters
OgreSubsystem*       GetOgreSubsystem();
Settings&            GetSettings();
ContentManager*      GetContentManager();
OverlayWrapper*      GetOverlayWrapper();
SceneMouse*          GetSceneMouse();
GUIManager*          GetGuiManager();
Console*             GetConsole();
InputEngine*         GetInputEngine();
CacheSystem*         GetCacheSystem();
MainThread*          GetMainThreadLogic();

State                GetActiveAppState();
State                GetPendingAppState();
STR_CREF             GetActiveTerrain();
STR_CREF             GetPendingTerrain();
MpState              GetActiveMpState();
MpState              GetPendingMpState();
SimState             GetActiveSimState();
SimState             GetPendingSimState();
STR_CREF             GetMpServerHost();
STR_CREF             GetMpServerPassword();
int                  GetMpServerPort();
STR_CREF             GetMpPlayerName();
bool                 GetDiagTraceGlobals();
STR_CREF             GetSysProcessDir();
STR_CREF             GetSysUserDir();
STR_CREF             GetSysConfigDir();
STR_CREF             GetSysCacheDir();
STR_CREF             GetSysLogsDir();
STR_CREF             GetSysResourcesDir();
bool                 GetInputFFEnabled();
float                GetInputFFCamera();
float                GetInputFFCentering();
float                GetInputFFGain();
float                GetInputFFStress();
GfxShadowType        GetGfxShadowType();
GfxExtCamMode        GetGfxExternCamMode();
GfxTexFilter         GetGfxTexFiltering();

// Setters
void SetActiveAppState   (State    v);
void SetPendingAppState  (State    v);
void SetActiveTerrain    (STR_CREF v);
void SetPendingTerrain   (STR_CREF v);
void SetActiveMpState    (MpState  v);
void SetPendingMpState   (MpState  v);
void SetActiveSimState   (SimState v);
void SetPendingSimState  (SimState v);
void SetMpServerHost     (STR_CREF v);
void SetMpServerPassword (STR_CREF v);
void SetMpServerPort     (int      v);
void SetMpPlayerName     (STR_CREF v);
void SetDiagTraceGlobals (bool     v);
void SetSysProcessDir    (STR_CREF v);
void SetSysUserDir       (STR_CREF v);
void SetSysConfigDir     (STR_CREF v);
void SetSysCacheDir      (STR_CREF v);
void SetSysLogsDir       (STR_CREF v);
void SetSysResourcesDir  (STR_CREF v);
void SetInputFFEnabled   (bool     v);
void SetInputFFCamera    (float    v);
void SetInputFFCentering (float    v);
void SetInputFFGain      (float    v);
void SetInputFFStress    (float    v);
void SetGfxShadowType    (GfxShadowType v);
void SetGfxExternCamMode (GfxExtCamMode v);
void SetGfxTexFiltering  (GfxTexFilter  v);

void SetMainThreadLogic  (MainThread* obj);

// Factories
void StartOgreSubsystem();
void ShutdownOgreSubsystem();
void CreateContentManager();
void DestroyContentManager();
void CreateOverlayWrapper();
void DestroyOverlayWrapper();
void CreateSceneMouse();
void DeleteSceneMouse();
void CreateGuiManagerIfNotExists();
void DeleteGuiManagerIfExists();
void CreateInputEngine();
void CreateCacheSystem();

} // namespace Application
} // namespace RoR
