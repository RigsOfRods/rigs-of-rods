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

#include "ScriptEvents.h"
#include "AngelScriptBindings.h"
#include "Application.h"
#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterScriptEvents(asIScriptEngine *engine)
{
    int result;

    // enum scriptEvents
    result = engine->RegisterEnum("scriptEvents"); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_COLLISION_BOX_ENTER", SE_COLLISION_BOX_ENTER); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_COLLISION_BOX_LEAVE", SE_COLLISION_BOX_LEAVE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_ENTER", SE_TRUCK_ENTER); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_EXIT", SE_TRUCK_EXIT); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_ENGINE_DIED", SE_TRUCK_ENGINE_DIED); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_ENGINE_FIRE", SE_TRUCK_ENGINE_FIRE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_TOUCHED_WATER", SE_TRUCK_TOUCHED_WATER); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_BEAM_BROKE", SE_TRUCK_BEAM_BROKE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_LOCKED", SE_TRUCK_LOCKED); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_UNLOCKED", SE_TRUCK_UNLOCKED); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_LIGHT_TOGGLE", SE_TRUCK_LIGHT_TOGGLE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_SKELETON_TOGGLE", SE_TRUCK_SKELETON_TOGGLE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_TIE_TOGGLE", SE_TRUCK_TIE_TOGGLE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_PARKINGBREAK_TOGGLE", SE_TRUCK_PARKINGBREAK_TOGGLE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_RESET", SE_TRUCK_RESET); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_TELEPORT", SE_TRUCK_TELEPORT); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_BEACONS_TOGGLE", SE_TRUCK_BEACONS_TOGGLE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_CPARTICLES_TOGGLE", SE_TRUCK_CPARTICLES_TOGGLE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_GROUND_CONTACT_CHANGED", SE_TRUCK_GROUND_CONTACT_CHANGED); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_NEW_TRUCK", SE_GENERIC_NEW_TRUCK); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_DELETED_TRUCK", SE_GENERIC_DELETED_TRUCK); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_TELEPORT", SE_TRUCK_TELEPORT); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_MOUSE_GRAB", SE_TRUCK_MOUSE_GRAB); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_ANGELSCRIPT_MANIPULATIONS", SE_ANGELSCRIPT_MANIPULATIONS); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_MESSAGEBOX_CLICK", SE_GENERIC_MESSAGEBOX_CLICK); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_ALL_EVENTS", SE_ALL_EVENTS); ROR_ASSERT(result>=0);
    
}
