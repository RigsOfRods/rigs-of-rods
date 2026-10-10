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

#include "../../autowrapper/aswrappedcall.h"
#include "AngelScriptBindings.h"
#include "ProceduralManager.h"
#include "ProceduralRoad.h"
#include "ScriptEngine.h"
#include "wrappers/ProceduralRoadAngelscriptWrappers.h"

#include <angelscript.h>

using namespace RoR;
using namespace AngelScript;
using namespace ProceduralRoadAngelscriptWrappers;

// We need to manually define a wrap function for addQuad()
// since aswrappedcall.h has no support for methods with more than 8 parameters.
static void ProceduralRoad_addQuad_Generic(asIScriptGeneric* gen)
{
    ProceduralRoad* obj = static_cast<ProceduralRoad*>(gen->GetObject());
    obj->addQuad(
        *static_cast<Ogre::Vector3*>(gen->GetArgObject(0)), // p1
        *static_cast<Ogre::Vector3*>(gen->GetArgObject(1)), // p2
        *static_cast<Ogre::Vector3*>(gen->GetArgObject(2)), // p3
        *static_cast<Ogre::Vector3*>(gen->GetArgObject(3)), // p4
        static_cast<TextureFit>(gen->GetArgDWord(4)),       // texfit
        *static_cast<Ogre::Vector3*>(gen->GetArgObject(5)), // pos
        *static_cast<Ogre::Vector3*>(gen->GetArgObject(6)), // lastpos
        gen->GetArgFloat(7),                                // width
        gen->GetArgByte(8) != 0);                           // flip
}

void RoR::RegisterProceduralRoadGeneric(asIScriptEngine* engine)
{
    int result = 0;

    // NOTE: enums RoadType and TextureFit are registered in RegisterProceduralRoadCommon()

    // struct ProceduralPoint (ref)
    ProceduralPoint::RegisterRefCountingObject(engine, "ProceduralPointClass");
    ProceduralPointPtr::RegisterRefCountingObjectPtr(engine, "ProceduralPointClassPtr", "ProceduralPointClass");
    result = engine->RegisterObjectBehaviour("ProceduralPointClass", asBEHAVE_FACTORY, "ProceduralPointClass@+ f()", WRAP_FN(ProceduralPointFactory), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    //get:
    result = engine->RegisterObjectMethod("ProceduralPointClass", "vector3& get_position() property", WRAP_OBJ_FIRST(ProceduralPoint_get_position), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralPointClass", "quaternion& get_rotation() property", WRAP_OBJ_FIRST(ProceduralPoint_get_rotation), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralPointClass", "float get_width() property", WRAP_OBJ_FIRST(ProceduralPoint_get_width), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralPointClass", "float get_border_width() property", WRAP_OBJ_FIRST(ProceduralPoint_get_border_width), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralPointClass", "float get_border_height() property", WRAP_OBJ_FIRST(ProceduralPoint_get_border_height), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralPointClass", "RoadType get_type() property", WRAP_OBJ_FIRST(ProceduralPoint_get_type), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralPointClass", "int get_pillar_type() property", WRAP_OBJ_FIRST(ProceduralPoint_get_pillar_type), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    //set:
    result = engine->RegisterObjectMethod("ProceduralPointClass", "void set_position(const vector3& in pos) property", WRAP_OBJ_FIRST(ProceduralPoint_set_position), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralPointClass", "void set_rotation(const quaternion& in rot) property", WRAP_OBJ_FIRST(ProceduralPoint_set_rotation), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralPointClass", "void set_width(float width) property", WRAP_OBJ_FIRST(ProceduralPoint_set_width), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralPointClass", "void set_border_width(float bwidth) property", WRAP_OBJ_FIRST(ProceduralPoint_set_border_width), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralPointClass", "void set_border_height(float bheight) property", WRAP_OBJ_FIRST(ProceduralPoint_set_border_height), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralPointClass", "void set_type(RoadType type) property", WRAP_OBJ_FIRST(ProceduralPoint_set_type), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralPointClass", "void set_pillar_type(int type) property", WRAP_OBJ_FIRST(ProceduralPoint_set_pillar_type), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    // class ProceduralRoad (ref)
    ProceduralRoad::RegisterRefCountingObject(engine, "ProceduralRoadClass");
    ProceduralRoadPtr::RegisterRefCountingObjectPtr(engine, "ProceduralRoadClassPtr", "ProceduralRoadClass");
    result = engine->RegisterObjectBehaviour("ProceduralRoadClass", asBEHAVE_FACTORY, "ProceduralRoadClass@+ f()", WRAP_FN(ProceduralRoadFactory), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralRoadClass", "void addBlock(vector3 pos, quaternion rot, RoadType type, float width, float border_width, float border_height, int pillar_type = 1)", WRAP_MFN(RoR::ProceduralRoad, addBlock), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralRoadClass", "void addQuad(vector3 p1, vector3 p2, vector3 p3, vector3 p4, TextureFit texfit, vector3 pos, vector3 lastpos, float width, bool flip = false)", asFUNCTION(ProceduralRoad_addQuad_Generic), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralRoadClass", "void addCollisionQuad(vector3 p1, vector3 p2, vector3 p3, vector3 p4, const string&in gm_name, bool flip = false)", WRAP_MFN_PR(RoR::ProceduralRoad, addCollisionQuad, (Ogre::Vector3, Ogre::Vector3, Ogre::Vector3, Ogre::Vector3, std::string const&, bool), void), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralRoadClass", "void finish()", WRAP_OBJ_FIRST(ProceduralRoad_finish), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralRoadClass", "void setCollisionEnabled(bool v)", WRAP_MFN(RoR::ProceduralRoad, setCollisionEnabled), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    // class ProceduralObject (ref)
    ProceduralObject::RegisterRefCountingObject(engine, "ProceduralObjectClass");
    ProceduralObjectPtr::RegisterRefCountingObjectPtr(engine, "ProceduralObjectClassPtr", "ProceduralObjectClass");
    result = engine->RegisterObjectBehaviour("ProceduralObjectClass", asBEHAVE_FACTORY, "ProceduralObjectClass@+ f()", WRAP_FN(ProceduralObjectFactory), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "string getName()", WRAP_MFN(RoR::ProceduralObject, getName), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "void setName(const string&in)", WRAP_MFN(RoR::ProceduralObject, setName), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "void addPoint(ProceduralPointClassPtr @)", WRAP_MFN(RoR::ProceduralObject, addPoint), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "void insertPoint(int pos, ProceduralPointClassPtr @)", WRAP_MFN(RoR::ProceduralObject, insertPoint), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "void deletePoint(int pos)", WRAP_MFN(RoR::ProceduralObject, deletePoint), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "ProceduralPointClassPtr @getPoint(int pos)", WRAP_MFN(RoR::ProceduralObject, getPoint), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "int getNumPoints()", WRAP_MFN(RoR::ProceduralObject, getNumPoints), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "ProceduralRoadClassPtr @getRoad()", WRAP_MFN(ProceduralObject, getRoad), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "int get_smoothing_num_splits() property", WRAP_OBJ_FIRST(ProceduralObject_get_smoothing_num_splits), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "void set_smoothing_num_splits(int) property", WRAP_OBJ_FIRST(ProceduralObject_set_smoothing_num_splits), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "bool get_collision_enabled() property", WRAP_OBJ_FIRST(ProceduralObject_get_collision_enabled), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "void set_collision_enabled(bool) property", WRAP_OBJ_FIRST(ProceduralObject_set_collision_enabled), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "string get_custom_material() property", asFUNCTION(ProceduralObject_get_custom_material), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "void set_custom_material(const string&in) property", WRAP_OBJ_FIRST(ProceduralObject_set_custom_material), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "string get_custom_groundmodel() property", WRAP_OBJ_FIRST(ProceduralObject_get_custom_groundmodel), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralObjectClass", "void set_custom_groundmodel(const string&in) property", WRAP_OBJ_FIRST(ProceduralObject_set_custom_groundmodel), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    // class ProceduralManager (ref)
    ProceduralManager::RegisterRefCountingObject(engine, "ProceduralManagerClass");
    ProceduralManagerPtr::RegisterRefCountingObjectPtr(engine, "ProceduralManagerClassPtr", "ProceduralManagerClass");
    result = engine->RegisterObjectMethod("ProceduralManagerClass", "void addObject(ProceduralObjectClassPtr@)", WRAP_MFN(ProceduralManager, addObject), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("ProceduralManagerClass", "void removeObject(ProceduralObjectClassPtr@)", WRAP_MFN(ProceduralManager, removeObject), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralManagerClass", "int getNumObjects()", WRAP_MFN(RoR::ProceduralManager, getNumObjects), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralManagerClass", "ProceduralObjectClassPtr @getObject(int pos)", WRAP_MFN(ProceduralManager, getObject), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralManagerClass", "void rebuildObjectMesh(ProceduralObjectClassPtr@)", WRAP_MFN(ProceduralManager, rebuildObjectMesh), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ProceduralManagerClass", "void deleteObjectMesh(ProceduralObjectClassPtr@)", WRAP_MFN(ProceduralManager, deleteObjectMesh), asCALL_GENERIC); ROR_ASSERT(result >= 0);
}
