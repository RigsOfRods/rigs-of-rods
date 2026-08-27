/*
 This source file is part of Rigs of Rods
 Copyright 2005-2012 Pierre-Michel Ricordel
 Copyright 2007-2012 Thomas Fischer
 Copyright 2013-2023 Petr Ohlidal

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

/// @file

#include "AngelScriptBindings.h"
#include "Engine.h"

#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterEngineCommon(asIScriptEngine* engine)
{
    int r;

    r = engine->RegisterEnum("autoswitch"); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("autoswitch", "AUTOSWITCH_REAR", Engine::REAR); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("autoswitch", "AUTOSWITCH_NEUTRAL", Engine::NEUTRAL); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("autoswitch", "AUTOSWITCH_DRIVE", Engine::DRIVE); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("autoswitch", "AUTOSWITCH_TWO", Engine::TWO); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("autoswitch", "AUTOSWITCH_ONE", Engine::ONE); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("autoswitch", "AUTOSWITCH_MANUALMODE", Engine::MANUALMODE); ROR_ASSERT(r >= 0);

    r = engine->RegisterEnum("SimGearboxMode"); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("SimGearboxMode", "AUTO", static_cast<int>(SimGearboxMode::AUTO)); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("SimGearboxMode", "SEMI_AUTO", static_cast<int>(SimGearboxMode::SEMI_AUTO)); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("SimGearboxMode", "MANUAL", static_cast<int>(SimGearboxMode::MANUAL)); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("SimGearboxMode", "MANUAL_STICK", static_cast<int>(SimGearboxMode::MANUAL_STICK)); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("SimGearboxMode", "MANUAL_RANGES", static_cast<int>(SimGearboxMode::MANUAL_RANGES)); ROR_ASSERT(r >= 0);
}
