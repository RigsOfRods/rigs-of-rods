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

    result = engine->RegisterEnumValue("scriptEvents", "SE_EVENTBOX_ENTER", SE_EVENTBOX_ENTER); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_EVENTBOX_EXIT", SE_EVENTBOX_EXIT); ROR_ASSERT(result >= 0);

    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_ENTER", SE_TRUCK_ENTER); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_EXIT", SE_TRUCK_EXIT); ROR_ASSERT(result>=0);

    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_ENGINE_DIED", SE_TRUCK_ENGINE_DIED); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_ENGINE_FIRE", SE_TRUCK_ENGINE_FIRE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_TOUCHED_WATER", SE_TRUCK_TOUCHED_WATER); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_LIGHT_TOGGLE", SE_TRUCK_LIGHT_TOGGLE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_TIE_TOGGLE", SE_TRUCK_TIE_TOGGLE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_PARKINGBRAKE_TOGGLE", SE_TRUCK_PARKINGBRAKE_TOGGLE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_BEACONS_TOGGLE", SE_TRUCK_BEACONS_TOGGLE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_CPARTICLES_TOGGLE", SE_TRUCK_CPARTICLES_TOGGLE); ROR_ASSERT(result>=0);

    result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_NEW_TRUCK", SE_GENERIC_NEW_TRUCK); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_DELETED_TRUCK", SE_GENERIC_DELETED_TRUCK); ROR_ASSERT(result>=0);

    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_RESET", SE_TRUCK_RESET); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_TELEPORT", SE_TRUCK_TELEPORT); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_MOUSE_GRAB", SE_TRUCK_MOUSE_GRAB); ROR_ASSERT(result>=0);

    result = engine->RegisterEnumValue("scriptEvents", "SE_ANGELSCRIPT_MANIPULATIONS", SE_ANGELSCRIPT_MANIPULATIONS); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_ANGELSCRIPT_MSGCALLBACK", SE_ANGELSCRIPT_MSGCALLBACK); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_ANGELSCRIPT_LINECALLBACK", SE_ANGELSCRIPT_LINECALLBACK); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_ANGELSCRIPT_EXCEPTIONCALLBACK", SE_ANGELSCRIPT_EXCEPTIONCALLBACK); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_ANGELSCRIPT_THREAD_STATUS", SE_ANGELSCRIPT_THREAD_STATUS); ROR_ASSERT(result>=0);

    result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_MESSAGEBOX_CLICK", SE_GENERIC_MESSAGEBOX_CLICK); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_EXCEPTION_CAUGHT", SE_GENERIC_EXCEPTION_CAUGHT); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_MODCACHE_ACTIVITY", SE_GENERIC_MODCACHE_ACTIVITY); ROR_ASSERT(result>=0);

    result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_TRUCK_LINKING_CHANGED", SE_GENERIC_TRUCK_LINKING_CHANGED); ROR_ASSERT(result>=0);

    result = engine->RegisterEnumValue("scriptEvents", "SE_ALL_EVENTS", SE_ALL_EVENTS); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_NO_EVENTS", SE_NO_EVENTS); ROR_ASSERT(result>=0);

    // enum angelScriptManipulationType
    result = engine->RegisterEnum("angelScriptManipulationType"); ROR_ASSERT(result>=0);

    result = engine->RegisterEnumValue("angelScriptManipulationType", "ASMANIP_CONSOLE_SNIPPET_EXECUTED", ASMANIP_CONSOLE_SNIPPET_EXECUTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("angelScriptManipulationType", "ASMANIP_SCRIPT_LOADED", ASMANIP_SCRIPT_LOADED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("angelScriptManipulationType", "ASMANIP_SCRIPT_UNLOADING", ASMANIP_SCRIPT_UNLOADING); ROR_ASSERT(result >= 0);

    // enum angelScriptThreadStatus
    result = engine->RegisterEnum("angelScriptThreadStatus"); ROR_ASSERT(result>=0);

    result = engine->RegisterEnumValue("angelScriptThreadStatus", "ASTHREADSTATUS_NONE", ASTHREADSTATUS_NONE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("angelScriptThreadStatus", "ASTHREADSTATUS_CURLSTRING_PROGRESS", ASTHREADSTATUS_CURLSTRING_PROGRESS); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("angelScriptThreadStatus", "ASTHREADSTATUS_CURLSTRING_SUCCESS", ASTHREADSTATUS_CURLSTRING_SUCCESS ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("angelScriptThreadStatus", "ASTHREADSTATUS_CURLSTRING_FAILURE", ASTHREADSTATUS_CURLSTRING_FAILURE ); ROR_ASSERT(result >= 0);

    // enum modCacheActivityType
    result = engine->RegisterEnum("modCacheActivityType"); ROR_ASSERT(result>=0);

    result = engine->RegisterEnumValue("modCacheActivityType", "MODCACHEACTIVITY_ENTRY_ADDED", MODCACHEACTIVITY_ENTRY_ADDED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("modCacheActivityType", "MODCACHEACTIVITY_ENTRY_DELETED", MODCACHEACTIVITY_ENTRY_DELETED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("modCacheActivityType", "MODCACHEACTIVITY_BUNDLE_LOADED", MODCACHEACTIVITY_BUNDLE_LOADED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("modCacheActivityType", "MODCACHEACTIVITY_BUNDLE_RELOADED", MODCACHEACTIVITY_BUNDLE_RELOADED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("modCacheActivityType", "MODCACHEACTIVITY_BUNDLE_UNLOADED", MODCACHEACTIVITY_BUNDLE_UNLOADED); ROR_ASSERT(result >= 0);
}
