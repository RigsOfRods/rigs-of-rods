/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2022 Petr Ohlidal

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

#pragma once

#include <angelscript.h>

/// @file

namespace RoR {

typedef void (*ASBindingRegisterFunc_t)(AngelScript::asIScriptEngine*);

void RegisterAngelScriptBinding(const char* bindingName, AngelScript::asIScriptEngine* engine, ASBindingRegisterFunc_t native, ASBindingRegisterFunc_t generic);

/*
=== NOTE ===
Register*Common() methods are meant for binding things that are common to both
native and generic bindings (e.g. enums, constants, etc).

Register*Native() methods are meant for registering native bindings (which use a native calling convention such as cdecl or thiscall).

Register*Generic() methods are meant for registering generic bindings (which use the asCALL_GENERIC calling convention).
*/


/// @addtogroup Scripting
/// @{

/// defined in ActorAngelscript.cpp
void RegisterActorCommon(AngelScript::asIScriptEngine* engine);
void RegisterActorNative(AngelScript::asIScriptEngine* engine);
void RegisterActorGeneric(AngelScript::asIScriptEngine* engine);
inline void RegisterActor(AngelScript::asIScriptEngine* engine)
{
    RegisterActorCommon(engine);
    RegisterAngelScriptBinding("Actor", engine, &RegisterActorNative, &RegisterActorGeneric);
}

/// defined in VehicleAiAngelscript.cpp
void RegisterVehicleAiCommon(AngelScript::asIScriptEngine* engine);
void RegisterVehicleAiNative(AngelScript::asIScriptEngine* engine);
void RegisterVehicleAiGeneric(AngelScript::asIScriptEngine* engine);
inline void RegisterVehicleAi(AngelScript::asIScriptEngine* engine)
{
    RegisterVehicleAiCommon(engine);
    RegisterAngelScriptBinding("VehicleAi", engine, &RegisterVehicleAiNative, &RegisterVehicleAiGeneric);
}

/// Registers RoR::InputEngine, defined in InputEngineAngelscript.cpp
void RegisterInputEngineNative(AngelScript::asIScriptEngine* engine);
inline void RegisterInputEngine(AngelScript::asIScriptEngine* engine)
{
    RegisterAngelScriptBinding("InputEngine", engine, &RegisterInputEngineNative, nullptr);
}

/// Registers RoR::Console, defined in ConsoleAngelscript.cpp
void RegisterConsoleNative(AngelScript::asIScriptEngine* engine);
inline void RegisterConsole(AngelScript::asIScriptEngine* engine)
{
    RegisterAngelScriptBinding("Console", engine, &RegisterConsoleNative, nullptr);
}

/// Registers RoR::LocalStorage, defined in LocalStorageAngelscript.cpp
void RegisterLocalStorageNative(AngelScript::asIScriptEngine* engine);
inline void RegisterLocalStorage(AngelScript::asIScriptEngine* engine)
{
    RegisterAngelScriptBinding("LocalStorage", engine, &RegisterLocalStorageNative, nullptr);
}

/// Registers RoR::GameScript, defined in GameScriptAngelscript.cpp
void RegisterGameScriptCommon(AngelScript::asIScriptEngine* engine);
void RegisterGameScriptNative(AngelScript::asIScriptEngine* engine);
void RegisterGameScriptGeneric(AngelScript::asIScriptEngine* engine);
inline void RegisterGameScript(AngelScript::asIScriptEngine* engine)
{
    RegisterGameScriptCommon(engine);
    RegisterAngelScriptBinding("GameScript", engine, &RegisterGameScriptNative, &RegisterGameScriptGeneric);
}

/// Registers enum scriptEvents, defined in ScriptEventsAngelscript.cpp
void RegisterScriptEvents(AngelScript::asIScriptEngine* engine);

/// defined in ImGuiAngelscript.cpp
void RegisterImGuiNativeBindings(AngelScript::asIScriptEngine* engine);
inline void RegisterImGuiBindings(AngelScript::asIScriptEngine* engine)
{
    RegisterAngelScriptBinding("ImGui", engine, &RegisterImGuiNativeBindings, nullptr);
}

// This function will register the following objects with the scriptengine:
//    - Ogre::Vector3
//    - Ogre::Vector2
//    - Ogre::Radian
//    - Ogre::Degree
//    - Ogre::Quaternion
//    - Ogre::ColourValue
/// defined in OgreAngelscript.cpp
void RegisterOgreObjectsNative(AngelScript::asIScriptEngine* engine);
inline void RegisterOgreObjects(AngelScript::asIScriptEngine* engine)
{
    RegisterAngelScriptBinding("Ogre", engine, &RegisterOgreObjectsNative, nullptr);
}

/// Registers RoR::Terrain, defined in TerrainAngelscript.cpp
void RegisterTerrainNative(AngelScript::asIScriptEngine* engine);
inline void RegisterTerrain(AngelScript::asIScriptEngine* engine)
{
    RegisterAngelScriptBinding("Terrain", engine, &RegisterTerrainNative, nullptr);
}

/// defined in ProceduralRoadAngelscript.cpp
void RegisterProceduralRoadNative(AngelScript::asIScriptEngine* engine);
inline void RegisterProceduralRoad(AngelScript::asIScriptEngine* engine)
{
    RegisterAngelScriptBinding("ProceduralRoad", engine, &RegisterProceduralRoadNative, nullptr);
}

/// defined in GenericFileFormatAngelscript.cpp
void RegisterGenericFileFormatNative(AngelScript::asIScriptEngine* engine);
inline void RegisterGenericFileFormat(AngelScript::asIScriptEngine* engine)
{
    RegisterAngelScriptBinding("GenericFileFormat", engine, &RegisterGenericFileFormatNative, nullptr);
}

/// Registers enum MsgType, defined in MsgQueueAngelscript.cpp
void RegisterMessageQueue(AngelScript::asIScriptEngine* engine);

/// defined in SoundScriptAngelscript.cpp
void RegisterSoundScriptNative(AngelScript::asIScriptEngine* engine);
inline void RegisterSoundScript(AngelScript::asIScriptEngine* engine)
{
    RegisterAngelScriptBinding("SoundScript", engine, &RegisterSoundScriptNative, nullptr);
}

/// defined in CacheSystemAngelscript.cpp
void RegisterCacheSystemNative(AngelScript::asIScriptEngine* engine);
inline void RegisterCacheSystem(AngelScript::asIScriptEngine* engine)
{
    RegisterAngelScriptBinding("CacheSystem", engine, &RegisterCacheSystemNative, nullptr);
}

/// Register class Engine and related enums, defined in EngineAngelscript.cpp
void RegisterEngineCommon(AngelScript::asIScriptEngine* engine);
void RegisterEngineNative(AngelScript::asIScriptEngine* engine);
void RegisterEngineGeneric(AngelScript::asIScriptEngine* engine);
inline void RegisterEngine(AngelScript::asIScriptEngine* engine)
{
    RegisterEngineCommon(engine);
    RegisterAngelScriptBinding("Engine", engine, &RegisterEngineNative, RegisterEngineGeneric);
}

// Register class DashBoardManager. Defined in DashBoardManagerAngelscript.cpp
void RegisterDashBoardManagerCommon(AngelScript::asIScriptEngine* engine);
void RegisterDashBoardManagerNative(AngelScript::asIScriptEngine* engine);
void RegisterDashBoardManagerGeneric(AngelScript::asIScriptEngine* engine);
inline void RegisterDashBoardManager(AngelScript::asIScriptEngine* engine)
{
    RegisterDashBoardManagerCommon(engine);
    RegisterAngelScriptBinding("DashBoardManager", engine, &RegisterDashBoardManagerNative, RegisterDashBoardManagerGeneric);
}

// Register class AircraftEngineClass. Defined in AircraftEngineAngelscript.cpp
void RegisterAircraftEngineCommon(AngelScript::asIScriptEngine* engine);
void RegisterAircraftEngineNative(AngelScript::asIScriptEngine* engine);
void RegisterAircraftEngineGeneric(AngelScript::asIScriptEngine* engine);
inline void RegisterAircraftEngine(AngelScript::asIScriptEngine* engine)
{
    RegisterAircraftEngineCommon(engine);
    RegisterAngelScriptBinding("AircraftEngine", engine, &RegisterAircraftEngineNative, &RegisterAircraftEngineGeneric);
}

// Register class TurbopropClass. Defined in TurbopropAngelscript.cpp
void RegisterTurbopropNative(AngelScript::asIScriptEngine* engine);
void RegisterTurbopropGeneric(AngelScript::asIScriptEngine* engine);
inline void RegisterTurboprop(AngelScript::asIScriptEngine* engine)
{
    RegisterAngelScriptBinding("Turboprop", engine, &RegisterTurbopropNative, &RegisterTurbopropGeneric);
}

// Register class TurbojetClass. Defined in TurbojetAngelscript.cpp
void RegisterTurbojetNative(AngelScript::asIScriptEngine* engine);
void RegisterTurbojetGeneric(AngelScript::asIScriptEngine* engine);
inline void RegisterTurbojet(AngelScript::asIScriptEngine* engine)
{
    RegisterAngelScriptBinding("Turbojet", engine, &RegisterTurbojetNative, &RegisterTurbojetGeneric);
}

// Register class AutopilotClass. Defined in AutopilotAngelscript.cpp
void RegisterAutopilotCommon(AngelScript::asIScriptEngine* engine);
void RegisterAutopilotNative(AngelScript::asIScriptEngine* engine);
void RegisterAutopilotGeneric(AngelScript::asIScriptEngine* engine);
inline void RegisterAutopilot(AngelScript::asIScriptEngine* engine)
{
    RegisterAutopilotCommon(engine);
    RegisterAngelScriptBinding("Autopilot", engine, &RegisterAutopilotNative, &RegisterAutopilotGeneric);
}

// Register class ScrewpropClass. Defined in ScrewpropAngelscript.cpp
void RegisterScrewpropNative(AngelScript::asIScriptEngine* engine);
void RegisterScrewpropGeneric(AngelScript::asIScriptEngine* engine);
inline void RegisterScrewprop(AngelScript::asIScriptEngine* engine)
{
    RegisterAngelScriptBinding("Screwprop", engine, &RegisterScrewpropNative, &RegisterScrewpropGeneric);
}


/// @}   //addtogroup Scripting

} // namespace RoR
