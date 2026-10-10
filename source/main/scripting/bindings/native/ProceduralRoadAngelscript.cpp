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

#include "AngelScriptBindings.h"
#include "ProceduralManager.h"
#include "ProceduralRoad.h"
#include "ScriptEngine.h"
#include "wrappers/ProceduralRoadAngelscriptWrappers.h"

using namespace RoR;
using namespace AngelScript;
using namespace ProceduralRoadAngelscriptWrappers;

void RoR::RegisterProceduralRoadNative(asIScriptEngine* engine)
{
    int result = 0;

    // NOTE: enums RoadType and TextureFit are registered in RegisterProceduralRoadCommon()

    // struct ProceduralPoint (ref)
    ProceduralPoint::RegisterRefCountingObject(engine, "ProceduralPointClass");
    ProceduralPointPtr::RegisterRefCountingObjectPtr(engine, "ProceduralPointClassPtr", "ProceduralPointClass");
    result = engine->RegisterObjectBehaviour("ProceduralPointClass", asBEHAVE_FACTORY, "ProceduralPointClass@+ f()", asFUNCTION(ProceduralPointFactory), asCALL_CDECL); ROR_ASSERT(result >= 0);
    //get:
    result = engine->RegisterObjectMethod("ProceduralPointClass", "vector3& get_position() property", asFUNCTION(ProceduralPoint_get_position), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralPointClass", "quaternion& get_rotation() property", asFUNCTION(ProceduralPoint_get_rotation), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralPointClass", "float get_width() property", asFUNCTION(ProceduralPoint_get_width), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralPointClass", "float get_border_width() property", asFUNCTION(ProceduralPoint_get_border_width), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralPointClass", "float get_border_height() property", asFUNCTION(ProceduralPoint_get_border_height), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralPointClass", "RoadType get_type() property", asFUNCTION(ProceduralPoint_get_type), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralPointClass", "int get_pillar_type() property", asFUNCTION(ProceduralPoint_get_pillar_type), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    //set:
    result = engine->RegisterObjectMethod("ProceduralPointClass", "void set_position(const vector3& in pos) property", asFUNCTION(ProceduralPoint_set_position), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralPointClass", "void set_rotation(const quaternion& in rot) property", asFUNCTION(ProceduralPoint_set_rotation), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralPointClass", "void set_width(float width) property", asFUNCTION(ProceduralPoint_set_width), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralPointClass", "void set_border_width(float bwidth) property", asFUNCTION(ProceduralPoint_set_border_width), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralPointClass", "void set_border_height(float bheight) property", asFUNCTION(ProceduralPoint_set_border_height), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralPointClass", "void set_type(RoadType type) property", asFUNCTION(ProceduralPoint_set_type), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralPointClass", "void set_pillar_type(int type) property", asFUNCTION(ProceduralPoint_set_pillar_type), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);

    // class ProceduralRoad (ref)
    ProceduralRoad::RegisterRefCountingObject(engine, "ProceduralRoadClass");
    ProceduralRoadPtr::RegisterRefCountingObjectPtr(engine, "ProceduralRoadClassPtr", "ProceduralRoadClass");
    result = engine->RegisterObjectBehaviour("ProceduralRoadClass", asBEHAVE_FACTORY, "ProceduralRoadClass@+ f(const string&in groundmodelName = \"\")", asFUNCTION(ProceduralRoadFactory), asCALL_CDECL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralRoadClass", "void addBlock(vector3 pos, quaternion rot, RoadType type, float width, float border_width, float border_height, int pillar_type = 1)", asMETHOD(RoR::ProceduralRoad, addBlock), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralRoadClass", "void addQuad(vector3 p1, vector3 p2, vector3 p3, vector3 p4, TextureFit texfit, vector3 pos, vector3 lastpos, float width, bool flip = false)", asMETHOD(RoR::ProceduralRoad, addQuad), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralRoadClass", "void addCollisionQuad(vector3 p1, vector3 p2, vector3 p3, vector3 p4, const string&in gm_name, bool flip = false)", asMETHODPR(RoR::ProceduralRoad, addCollisionQuad, (Ogre::Vector3, Ogre::Vector3, Ogre::Vector3, Ogre::Vector3, std::string const&, bool), void), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralRoadClass", "void finish(Ogre::SceneNode& groupingSceneNode, const string&in materialName = \"\")", asMETHOD(RoR::ProceduralRoad, finish), asCALL_THISCALL); ROR_ASSERT(result >= 0);
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
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "int get_smoothing_num_splits() property", asFUNCTION(ProceduralObject_get_smoothing_num_splits), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "void set_smoothing_num_splits(int) property", asFUNCTION(ProceduralObject_set_smoothing_num_splits), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "bool get_collision_enabled() property", asFUNCTION(ProceduralObject_get_collision_enabled), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "void set_collision_enabled(bool) property", asFUNCTION(ProceduralObject_set_collision_enabled), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "string get_custom_material() property", asFUNCTION(ProceduralObject_get_custom_material), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "void set_custom_material(const string&in) property", asFUNCTION(ProceduralObject_set_custom_material), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "string get_custom_groundmodel() property", asFUNCTION(ProceduralObject_get_custom_groundmodel), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "void set_custom_groundmodel(const string&in) property", asFUNCTION(ProceduralObject_set_custom_groundmodel), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);

    // class ProceduralManager (ref)
    ProceduralManager::RegisterRefCountingObject(engine, "ProceduralManagerClass");
    ProceduralManagerPtr::RegisterRefCountingObjectPtr(engine, "ProceduralManagerClassPtr", "ProceduralManagerClass");
    result = engine->RegisterObjectMethod("ProceduralManagerClass", "void addObject(ProceduralObjectClassPtr@)", asMETHOD(ProceduralManager, addObject), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("ProceduralManagerClass", "void removeObject(ProceduralObjectClassPtr@)", asMETHOD(ProceduralManager, removeObject), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralManagerClass", "int getNumObjects()", asMETHOD(RoR::ProceduralManager, getNumObjects), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralManagerClass", "ProceduralObjectClassPtr @getObject(int pos)", asMETHOD(ProceduralManager, getObject), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralManagerClass", "void rebuildObjectMesh(ProceduralObjectClassPtr@)", asMETHOD(ProceduralManager, rebuildObjectMesh), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralManagerClass", "void deleteObjectMesh(ProceduralObjectClassPtr@)", asMETHOD(ProceduralManager, deleteObjectMesh), asCALL_THISCALL); ROR_ASSERT(result >= 0);
}
