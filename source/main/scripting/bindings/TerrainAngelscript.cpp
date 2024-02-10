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
/// @author Petr Ohlidal
 
#include "ScriptEngine.h"
#include "Terrain.h"

#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterTerrain(asIScriptEngine* engine)
{
    Terrain::RegisterRefCountingObject(engine, "TerrainClass");
    TerrainPtr::RegisterRefCountingObjectPtr(engine, "TerrainClassPtr", "TerrainClass");

    int result = 0;

    // > General
    result = engine->RegisterObjectMethod("TerrainClass", "string getTerrainName()", asMETHOD(RoR::Terrain,getTerrainName), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("TerrainClass", "string getTerrainFileName()", asMETHOD(RoR::Terrain, getTerrainFileName), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TerrainClass", "string getTerrainFileResourceGroup()", asMETHOD(RoR::Terrain, getTerrainFileResourceGroup), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TerrainClass", "string getGUID()", asMETHOD(RoR::Terrain,getGUID), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("TerrainClass", "int getVersion()", asMETHOD(RoR::Terrain,getVersion), asCALL_THISCALL); ROR_ASSERT(result>=0);
    
    // > Landscape
    result = engine->RegisterObjectMethod("TerrainClass", "bool isFlat()", asMETHOD(RoR::Terrain,isFlat), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("TerrainClass", "float getHeightAt(float x, float z)", asMETHOD(RoR::Terrain,GetHeightAt), asCALL_THISCALL); ROR_ASSERT(result>=0);
    
    // > Gameplay
    result = engine->RegisterObjectMethod("TerrainClass", "vector3 getSpawnPos()", asMETHOD(RoR::Terrain,getSpawnPos), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("TerrainClass", "void addSurveyMapEntity(const string &in type, const string &in filename, const string &in resource_group, const string &in caption, const vector3 &in pos, float angle, int id)", asMETHOD(RoR::Terrain, addSurveyMapEntity), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TerrainClass", "void delSurveyMapEntities(int id)", asMETHOD(RoR::Terrain, delSurveyMapEntities), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    
    // > Subsystems
    result = engine->RegisterObjectMethod("TerrainClass", "ProceduralManagerClassPtr @getProceduralManager()", asMETHOD(RoR::Terrain, getProceduralManager), asCALL_THISCALL); ROR_ASSERT(result >= 0);
}
