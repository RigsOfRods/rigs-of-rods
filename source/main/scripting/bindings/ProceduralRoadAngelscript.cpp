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
/// @author Petr Ohlidal
 
#include "ProceduralManager.h"
#include "ProceduralRoad.h"
#include "ScriptEngine.h"

using namespace RoR;
using namespace AngelScript;

static ProceduralPoint* ProceduralPointFactory()
{
    return new ProceduralPoint();
}

static ProceduralObject* ProceduralObjectFactory()
{
    return new ProceduralObject();
}

static ProceduralRoad* ProceduralRoadFactory()
{
    return new ProceduralRoad();
}

void RoR::RegisterProceduralRoad(asIScriptEngine* engine)
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

    // struct ProceduralPoint (ref)
    ProceduralPoint::RegisterRefCountingObject(engine, "ProceduralPointClass");
    ProceduralPointPtr::RegisterRefCountingObjectPtr(engine, "ProceduralPointClassPtr", "ProceduralPointClass");
    result = engine->RegisterObjectBehaviour("ProceduralPointClass", asBEHAVE_FACTORY, "ProceduralPointClass@+ f()", asFUNCTION(ProceduralPointFactory), asCALL_CDECL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectProperty("ProceduralPointClass", "vector3 position", offsetof(ProceduralPoint, position)); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectProperty("ProceduralPointClass", "quaternion rotation", offsetof(ProceduralPoint, rotation)); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectProperty("ProceduralPointClass", "float width", offsetof(ProceduralPoint, width)); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectProperty("ProceduralPointClass", "float border_width", offsetof(ProceduralPoint, bwidth)); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectProperty("ProceduralPointClass", "float border_height", offsetof(ProceduralPoint, bheight)); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectProperty("ProceduralPointClass", "RoadType type", offsetof(ProceduralPoint, type)); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectProperty("ProceduralPointClass", "int pillar_type", offsetof(ProceduralPoint, pillartype)); ROR_ASSERT(result >= 0);

    // class ProceduralRoad (ref)
    ProceduralRoad::RegisterRefCountingObject(engine, "ProceduralRoadClass");
    ProceduralRoadPtr::RegisterRefCountingObjectPtr(engine, "ProceduralRoadClassPtr", "ProceduralRoadClass");
    result = engine->RegisterObjectBehaviour("ProceduralRoadClass", asBEHAVE_FACTORY, "ProceduralRoadClass@+ f()", asFUNCTION(ProceduralRoadFactory), asCALL_CDECL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralRoadClass", "void addBlock(vector3 pos, quaternion rot, RoadType type, float width, float border_width, float border_height, int pillar_type = 1)", asMETHOD(RoR::ProceduralRoad, addBlock), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralRoadClass", "void addQuad(vector3 p1, vector3 p2, vector3 p3, vector3 p4, TextureFit texfit, vector3 pos, vector3 lastpos, float width, bool flip = false)", asMETHOD(RoR::ProceduralRoad, addQuad), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralRoadClass", "void addCollisionQuad(vector3 p1, vector3 p2, vector3 p3, vector3 p4, const string&in gm_name, bool flip = false)", asMETHODPR(RoR::ProceduralRoad, addCollisionQuad, (Ogre::Vector3, Ogre::Vector3, Ogre::Vector3, Ogre::Vector3, std::string const&, bool), void), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralRoadClass", "void createMesh()", asMETHOD(RoR::ProceduralRoad, createMesh), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralRoadClass", "void finish()", asMETHOD(RoR::ProceduralRoad, finish), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralRoadClass", "void setCollisionEnabled(bool v)", asMETHOD(RoR::ProceduralRoad, setCollisionEnabled), asCALL_THISCALL); ROR_ASSERT(result >= 0);

    // class ProceduralObject (ref)
    ProceduralObject::RegisterRefCountingObject(engine, "ProceduralObjectClass");
    ProceduralObjectPtr::RegisterRefCountingObjectPtr(engine, "ProceduralObjectClassPtr", "ProceduralObjectClass");
    result = engine->RegisterObjectBehaviour("ProceduralObjectClass", asBEHAVE_FACTORY, "ProceduralObjectClass@+ f()", asFUNCTION(ProceduralObjectFactory), asCALL_CDECL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "string getName()", asMETHOD(RoR::ProceduralObject, getName), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "void setName(const string&in)", asMETHOD(RoR::ProceduralObject, setName), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "void addPoint(ProceduralPointClassPtr @)", asMETHOD(RoR::ProceduralObject, addPoint), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "void insertPoint(int pos, ProceduralPointClassPtr @)", asMETHOD(RoR::ProceduralObject, insertPoint), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "void deletePoint(int pos)", asMETHOD(RoR::ProceduralObject, deletePoint), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "ProceduralPointClassPtr @getPoint(int pos)", asMETHOD(RoR::ProceduralObject, getPoint), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "int getNumPoints()", asMETHOD(RoR::ProceduralObject, getNumPoints), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "ProceduralRoadClassPtr @getRoad()", asMETHOD(ProceduralObject, getRoad), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectProperty("ProceduralObjectClass", "int smoothing_num_splits", offsetof(ProceduralObject, smoothing_num_splits)); ROR_ASSERT(result >= 0);

    // class ProceduralManager (ref)
    ProceduralManager::RegisterRefCountingObject(engine, "ProceduralManagerClass");
    ProceduralManagerPtr::RegisterRefCountingObjectPtr(engine, "ProceduralManagerClassPtr", "ProceduralManagerClass");
    result = engine->RegisterObjectMethod("ProceduralManagerClass", "void addObject(ProceduralObjectClassPtr@)", asMETHOD(ProceduralManager, addObject), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("ProceduralManagerClass", "void removeObject(ProceduralObjectClassPtr@)", asMETHOD(ProceduralManager, removeObject), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralManagerClass", "int getNumObjects()", asMETHOD(RoR::ProceduralManager, getNumObjects), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralManagerClass", "ProceduralObjectClassPtr @getObject(int pos)", asMETHOD(ProceduralManager, getObject), asCALL_THISCALL); ROR_ASSERT(result >= 0);
}
