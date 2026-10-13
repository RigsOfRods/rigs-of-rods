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
 
#include "../../autowrapper/aswrappedcall.h"
#include "AngelScriptBindings.h"
#include "Application.h"
#include "CacheSystem.h"
#include "ProceduralManager.h"
#include "ScriptEngine.h"
#include "Terrain.h"
#include "TerrainEditor.h"

#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterTerrainGeneric(asIScriptEngine* engine)
{
    int result = 0;

    // NOTE: enum SpecialObjectType is registered in RegisterTerrainCommon()

    Terrain::RegisterRefCountingObjectGeneric(engine, "TerrainClass");
    TerrainPtr::RegisterRefCountingObjectPtrGeneric(engine, "TerrainClassPtr", "TerrainClass");

    // PLEASE maintain same order as in 'terrain/Terrain.cpp' and 'doc/angelscript/TerrainClass.h'

    // > General
    result = engine->RegisterObjectMethod("TerrainClass", "string getTerrainName()", WRAP_MFN(RoR::Terrain, getTerrainName), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("TerrainClass", "string getTerrainFileName()", WRAP_MFN(RoR::Terrain, getTerrainFileName), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TerrainClass", "string getTerrainFileResourceGroup()", WRAP_MFN(RoR::Terrain, getTerrainFileResourceGroup), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TerrainClass", "string getGUID()", WRAP_MFN(RoR::Terrain, getGUID), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("TerrainClass", "int getVersion()", WRAP_MFN(RoR::Terrain, getVersion), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("TerrainClass", "CacheEntryClassPtr @getCacheEntry()", WRAP_MFN(RoR::Terrain, getCacheEntry), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    
    // > Gameplay
    result = engine->RegisterObjectMethod("TerrainClass", "bool isFlat()", WRAP_MFN(RoR::Terrain, isFlat), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("TerrainClass", "float getHeightAt(float x, float z)", WRAP_MFN(RoR::Terrain, getHeightAt), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("TerrainClass", "vector3 getSpawnPos()", WRAP_MFN(RoR::Terrain, getSpawnPos), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("TerrainClass", "degree getSpawnRot()", WRAP_MFN(RoR::Terrain, getSpawnRot), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TerrainClass", "vector3 getMaxTerrainSize()", WRAP_MFN(RoR::Terrain, getMaxTerrainSize), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    // > Subsystems
    result = engine->RegisterObjectMethod("TerrainClass", "void addSurveyMapEntity(const string &in type, const string &in filename, const string &in resource_group, const string &in caption, const vector3 &in pos, float angle, int id)", WRAP_MFN(RoR::Terrain, addSurveyMapEntity), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TerrainClass", "void delSurveyMapEntities(int id)", WRAP_MFN(RoR::Terrain, delSurveyMapEntities), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TerrainClass", "ProceduralManagerClassPtr @getProceduralManager()", WRAP_MFN(RoR::Terrain, getProceduralManager), asCALL_GENERIC); ROR_ASSERT(result >= 0);


    TerrainEditorObject::RegisterRefCountingObjectGeneric(engine, "TerrainEditorObjectClass");
    TerrainEditorObjectPtr::RegisterRefCountingObjectPtrGeneric(engine, "TerrainEditorObjectClassPtr", "TerrainEditorObjectClass");

    result = engine->RegisterObjectMethod("TerrainEditorObjectClass", "const vector3& getPosition()", WRAP_MFN(RoR::TerrainEditorObject, getPosition), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TerrainEditorObjectClass", "const vector3& getRotation()", WRAP_MFN(RoR::TerrainEditorObject, getRotation), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    result = engine->RegisterObjectMethod("TerrainEditorObjectClass", "void setPosition(const vector3&in)", WRAP_MFN(RoR::TerrainEditorObject, setPosition), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TerrainEditorObjectClass", "void setRotation(const vector3&in)", WRAP_MFN(RoR::TerrainEditorObject, setRotation), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    result = engine->RegisterObjectMethod("TerrainEditorObjectClass", "const string& getName()", WRAP_MFN(RoR::TerrainEditorObject, getName), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TerrainEditorObjectClass", "const string& getInstanceName()", WRAP_MFN(RoR::TerrainEditorObject, getInstanceName), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TerrainEditorObjectClass", "const string& getType()", WRAP_MFN(RoR::TerrainEditorObject, getType), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    
    // ~ only for preloaded actors:
    result = engine->RegisterObjectMethod("TerrainEditorObjectClass", "const SpecialObjectType getSpecialObjectType()", WRAP_MFN(RoR::TerrainEditorObject, getSpecialObjectType), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TerrainEditorObjectClass", "void setSpecialObjectType(SpecialObjectType)", WRAP_MFN(RoR::TerrainEditorObject, setSpecialObjectType), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TerrainEditorObjectClass", "const int getActorInstanceId()", WRAP_MFN(RoR::TerrainEditorObject, getActorInstanceId), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TerrainEditorObjectClass", "void setActorInstanceId(int)", WRAP_MFN(RoR::TerrainEditorObject, setActorInstanceId), asCALL_GENERIC); ROR_ASSERT(result >= 0);

}
