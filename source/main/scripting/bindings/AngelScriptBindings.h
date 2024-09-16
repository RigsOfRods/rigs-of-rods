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

/// @addtogroup Scripting
/// @{

/// defined in ActorAngelscript.cpp
void RegisterActor(AngelScript::asIScriptEngine* engine);

/// defined in VehicleAiAngelscript.cpp
void RegisterVehicleAi(AngelScript::asIScriptEngine* engine);

/// Registers RoR::InputEngine, defined in InputEngineAngelscript.cpp
void RegisterInputEngine(AngelScript::asIScriptEngine* engine);

/// Registers RoR::Console, defined in ConsoleAngelscript.cpp
void RegisterConsole(AngelScript::asIScriptEngine* engine);

/// Registers RoR::LocalStorage, defined in LocalStorageAngelscript.cpp
void RegisterLocalStorage(AngelScript::asIScriptEngine* engine);

/// Registers RoR::GameScript, defined in GameScriptAngelscript.cpp
void RegisterGameScript(AngelScript::asIScriptEngine* engine);

/// Registers enum scriptEvents, defined in ScriptEventsAngelscript.cpp
void RegisterScriptEvents(AngelScript::asIScriptEngine* engine);

/// defined in ImGuiAngelscript.cpp
void RegisterImGuiBindings(AngelScript::asIScriptEngine* engine);

// This function will register the following objects with the scriptengine:
//    - Ogre::Vector3
//    - Ogre::Vector2
//    - Ogre::Radian
//    - Ogre::Degree
//    - Ogre::Quaternion
//    - Ogre::ColourValue
/// defined in OgreAngelscript.cpp
void RegisterOgreObjects(AngelScript::asIScriptEngine* engine);

/// Registers RoR::Terrain, defined in TerrainAngelscript.cpp
void RegisterTerrain(AngelScript::asIScriptEngine* engine);

/// defined in ProceduralRoadAngelscript.cpp
void RegisterProceduralRoad(AngelScript::asIScriptEngine* engine);

/// defined in GenericFileFormatAngelscript.cpp
void RegisterGenericFileFormat(AngelScript::asIScriptEngine* engine);

/// Registers enum MsgType, defined in MsgQueueAngelscript.cpp
void RegisterMessageQueue(AngelScript::asIScriptEngine* engine);

/// defined in SoundScriptAngelscript.cpp
void RegisterSoundScript(AngelScript::asIScriptEngine* engine);

/// defined in CacheSystemAngelscript.cpp
void RegisterCacheSystem(AngelScript::asIScriptEngine* engine);

/// Register class EngineSim and related enums, defined in EngineSimAngelscript.cpp
void RegisterEngineSim(AngelScript::asIScriptEngine* engine);


/// @}   //addtogroup Scripting

} // namespace RoR
