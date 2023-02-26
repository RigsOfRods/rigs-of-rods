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

#include "VehicleAI.h"
#include "AngelScriptBindings.h"
#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterVehicleAi(asIScriptEngine *engine)
{
    int result;

    // enum aiEvents
    result = engine->RegisterEnum("aiEvents"); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("aiEvents", "AI_LIGHTSTOGGLE", AI_LIGHTSTOGGLE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("aiEvents", "AI_WAIT_SECONDS", AI_WAIT_SECONDS); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("aiEvents", "AI_BEACONSTOGGLE", AI_BEACONSTOGGLE); ROR_ASSERT(result >= 0);

    // enum aiEvents
    result = engine->RegisterEnum("AiValues"); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("AiValues", "AI_SPEED", AI_SPEED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("AiValues", "AI_POWER", AI_POWER); ROR_ASSERT(result >= 0);

    // (ref object) VehicleAI
    VehicleAI::RegisterRefCountingObject(engine, "VehicleAIClass");
    VehicleAIPtr::RegisterRefCountingObjectPtr(engine, "VehicleAIClassPtr", "VehicleAIClass");

    // PLEASE maintain the same order as in 'VehicleAI.h' and 'doc/../VehicleAIClass.h'
    result = engine->RegisterObjectMethod("VehicleAIClass", "void setActive(bool)", asMETHOD(VehicleAI, setActive), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("VehicleAIClass", "bool isActive()", asMETHOD(VehicleAI, isActive), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("VehicleAIClass", "void addWaypoint(const string &in id, const vector3 &in pos)", asMETHOD(VehicleAI, addWaypoint), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("VehicleAIClass", "void addWaypoints(dictionary &in d)", asMETHOD(VehicleAI, addWaypoints), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("VehicleAIClass", "void addEvent(const string &in id,int ev)", asMETHOD(VehicleAI, addEvent), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("VehicleAIClass", "void setValueAtWaypoint(const string &in id, int value_id, float value)", asMETHOD(VehicleAI, setValueAtWaypoint), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("VehicleAIClass", "vector3 getTranslation(int offset, uint wp)", AngelScript::asMETHOD(VehicleAI, getTranslation), asCALL_THISCALL); ROR_ASSERT(result >= 0);

}
