/*
    This source file is part of Rigs of Rods
    Copyright 2022 Petr Ohlidal

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
#include "ProceduralRoad.h"

#include <angelscript.h>

using namespace AngelScript;
using namespace RoR;

void RoR::RegisterProceduralRoadCommon(asIScriptEngine* engine)
{
    int result = 0;

    // enum RoadType
    result = engine->RegisterEnum("RoadType"); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("RoadType", "ROAD_AUTOMATIC", (int)RoadType::ROAD_AUTOMATIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("RoadType", "ROAD_FLAT", (int)RoadType::ROAD_FLAT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("RoadType", "ROAD_LEFT", (int)RoadType::ROAD_LEFT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("RoadType", "ROAD_RIGHT", (int)RoadType::ROAD_RIGHT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("RoadType", "ROAD_BOTH", (int)RoadType::ROAD_BOTH); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("RoadType", "ROAD_BRIDGE", (int)RoadType::ROAD_BRIDGE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("RoadType", "ROAD_MONORAIL", (int)RoadType::ROAD_MONORAIL); ROR_ASSERT(result >= 0);

    // enum TextureFit
    result = engine->RegisterEnum("TextureFit"); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("TextureFit", "TEXFIT_NONE", (int)TextureFit::TEXFIT_NONE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("TextureFit", "TEXFIT_BRICKWALL", (int)TextureFit::TEXFIT_BRICKWALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("TextureFit", "TEXFIT_ROADS1", (int)TextureFit::TEXFIT_ROADS1); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("TextureFit", "TEXFIT_ROADS2", (int)TextureFit::TEXFIT_ROADS2); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("TextureFit", "TEXFIT_ROAD", (int)TextureFit::TEXFIT_ROAD); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("TextureFit", "TEXFIT_ROADS3", (int)TextureFit::TEXFIT_ROADS3); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("TextureFit", "TEXFIT_ROADS4", (int)TextureFit::TEXFIT_ROADS4); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("TextureFit", "TEXFIT_CONCRETEWALL", (int)TextureFit::TEXFIT_CONCRETEWALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("TextureFit", "TEXFIT_CONCRETEWALLI", (int)TextureFit::TEXFIT_CONCRETEWALLI); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("TextureFit", "TEXFIT_CONCRETETOP", (int)TextureFit::TEXFIT_CONCRETETOP); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("TextureFit", "TEXFIT_CONCRETEUNDER", (int)TextureFit::TEXFIT_CONCRETEUNDER); ROR_ASSERT(result >= 0);
}
