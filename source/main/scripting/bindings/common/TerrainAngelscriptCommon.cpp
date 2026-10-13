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

/// @file

#include "AngelScriptBindings.h"
#include "Application.h"

#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterTerrainCommon(asIScriptEngine* engine)
{
    int result = 0;

    result = engine->RegisterEnum("SpecialObjectType"); ROR_ASSERT(result >= 0);

    result = engine->RegisterEnumValue("SpecialObjectType", "SPECIAL_OBJECT_NONE", (int)TObjSpecialObject::NONE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("SpecialObjectType", "SPECIAL_OBJECT_TRUCK", (int)TObjSpecialObject::TRUCK); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("SpecialObjectType", "SPECIAL_OBJECT_LOAD", (int)TObjSpecialObject::LOAD); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("SpecialObjectType", "SPECIAL_OBJECT_MACHINE", (int)TObjSpecialObject::MACHINE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("SpecialObjectType", "SPECIAL_OBJECT_BOAT", (int)TObjSpecialObject::BOAT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("SpecialObjectType", "SPECIAL_OBJECT_TRUCK2", (int)TObjSpecialObject::TRUCK2); ROR_ASSERT(result >= 0);
}
