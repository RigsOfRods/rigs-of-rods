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

#include "RacingAI.h"
#include "AngelScriptBindings.h"
#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterRacingAi(asIScriptEngine *engine)
{
    int result;

    // NOTE: enums Ai_events, Ai_values are shared with VehicleAI

    // (ref object) RacingAI
    RacingAI::RegisterRefCountingObject(engine, "RacingAIClass");
    RacingAIPtr::RegisterRefCountingObjectPtr(engine, "RacingAIClassPtr", "RacingAIClass");

    // PLEASE maintain the same order as in 'RacingAI.h' and 'doc/../RacingAIClass.h'
    result = engine->RegisterObjectMethod("RacingAIClass", "void setActive(bool)", asMETHOD(RacingAI, SetActive), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("RacingAIClass", "bool isActive()", asMETHOD(RacingAI, IsActive), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("RacingAIClass", "void addWaypoint(const string &in id, const vector3 &in pos)", asMETHOD(RacingAI, AddWaypoint), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("RacingAIClass", "void addWaypoints(dictionary &in d)", asMETHOD(RacingAI, AddWaypoints), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("RacingAIClass", "void addEvent(const string &in id,int ev)", asMETHOD(RacingAI, AddEvent), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("RacingAIClass", "void setValueAtWaypoint(const string &in id, int value_id, float value)", asMETHOD(RacingAI, SetValueAtWaypoint), asCALL_THISCALL); ROR_ASSERT(result >= 0);

}
