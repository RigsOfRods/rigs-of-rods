/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2024 Petr Ohlidal

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
/// @author Thomas Fischer
/// @date   31th of July 2009

#include "Application.h"
#include "AngelScriptBindings.h"
#include "ScriptEngine.h"
#include "ScriptUtils.h"
#include "wrappers/OgreAngelscriptWrappers.h"

// AS addons start
#include "scriptstdstring/scriptstdstring.h"
#include "scriptmath/scriptmath.h"
#include "scriptany/scriptany.h"
#include "scriptarray/scriptarray.h"
#include "scripthelper/scripthelper.h"
// AS addons end

#include <OgreRenderOperation.h>

#include <Overlay/OgreOverlaySystem.h>
#include <Overlay/OgreOverlayManager.h>
#include <Overlay/OgreOverlay.h>
#include <Overlay/OgreOverlayContainer.h>

using namespace Ogre;
using namespace AngelScript;
using namespace RoR;

/***NODE***/
typedef CReadonlyScriptArrayView<Ogre::Node*> ChildNodeArray;

static ChildNodeArray* NodeGetChildren(Ogre::Node* self)
{
    return new ChildNodeArray(self->getChildren());
}

static std::string NodeGetUniqueNameMixin(Ogre::Node* self)
{
    // Node names are optional and largely unused by RoR, so always append the memory address (libfmt adds the '0x' prefix)
    return fmt::format("\"{}\" ({})", self->getName(), static_cast<void*>(self));
}

/***SCENENODE***/
typedef CReadonlyScriptArrayView<Ogre::MovableObject*> MovableObjectArray;

static MovableObjectArray* SceneNodeGetAttachedObjects(SceneNode* self)
{
    return new MovableObjectArray(self->getAttachedObjects());
}

/***ROOT***/
typedef CReadonlyScriptDictView<SceneManager*> SceneManagerInstanceDict;

static SceneManagerInstanceDict* RootGetSceneManagers(Root* self)
{
    return new SceneManagerInstanceDict(self->getSceneManagers());
}

/***SCENEMANAGER***/

static AngelScript::CScriptArray* SceneManager__getMovableObjectsByType(SceneManager* self, const std::string& typeName)
{
    if (!self)
    {
        App::GetScriptEngine()->SLOG("SceneManager::__getMovableObjectsByType(): SceneManager is null");
        return nullptr;
    }

    try
    {
        // Get the iterator
        SceneManager::MovableObjectIterator it = self->getMovableObjectIterator(typeName);

        // Create script array for MovableObjects
        AngelScript::asITypeInfo* typeinfo = App::GetScriptEngine()->getEngine()->GetTypeInfoByDecl("array<Ogre::MovableObject@>");
        if (!typeinfo)
        {
            App::GetScriptEngine()->SLOG("SceneManager::__getMovableObjectsByType(): Failed to get array type info");
            return nullptr;
        }

        AngelScript::CScriptArray* arr = AngelScript::CScriptArray::Create(typeinfo);

        // Iterate through all movable objects of this type
        while (it.hasMoreElements())
        {
            Ogre::MovableObject* obj = it.getNext();
            if (obj)
            {
                arr->InsertLast(&obj);
            }
        }

        return arr;
    }
    catch (...)
    {
        App::GetScriptEngine()->forwardExceptionAsScriptEvent("SceneManager::__getMovableObjectsByType()");
        return nullptr;
    }
}
using namespace OgreAngelscriptWrappers;

// forward declarations, defined below
void registerOgreVector3(AngelScript::asIScriptEngine* engine);
void registerOgreVector2(AngelScript::asIScriptEngine* engine);
void registerOgreRadian(AngelScript::asIScriptEngine* engine);
void registerOgreDegree(AngelScript::asIScriptEngine* engine);
void registerOgreQuaternion(AngelScript::asIScriptEngine* engine);
void registerOgreOverlay(AngelScript::asIScriptEngine* engine);
void registerOgreColourValue(AngelScript::asIScriptEngine* engine);
void registerOgreBox(AngelScript::asIScriptEngine* engine);

void registerOgreMovableObject(AngelScript::asIScriptEngine* engine);
void registerOgreEntity(AngelScript::asIScriptEngine* engine);
void registerOgreSubEntity(AngelScript::asIScriptEngine* engine);
void registerOgreNode(AngelScript::asIScriptEngine* engine);
void registerOgreSceneNode(AngelScript::asIScriptEngine* engine);
void registerOgreSceneManager(AngelScript::asIScriptEngine* engine);
void registerOgreRoot(AngelScript::asIScriptEngine* engine);
void registerOgreAnimationState(AngelScript::asIScriptEngine* engine);
void registerOgreAnimationStateSet(AngelScript::asIScriptEngine* engine);
void registerOgreTexture(AngelScript::asIScriptEngine* engine);
void registerOgreTextureManager(AngelScript::asIScriptEngine* engine);
void registerOgreManualObject(AngelScript::asIScriptEngine* engine);
void registerOgreHardwarePixelBuffer(AngelScript::asIScriptEngine* engine);
void registerOgrePixelBox(AngelScript::asIScriptEngine* engine);
void registerOgreImage(AngelScript::asIScriptEngine* engine);
void registerOgreMeshManager(AngelScript::asIScriptEngine* engine);
void registerOgreMesh(AngelScript::asIScriptEngine* engine);
void registerOgreSubMesh(AngelScript::asIScriptEngine* engine);
void registerOgreMaterialManager(AngelScript::asIScriptEngine* engine);
void registerOgreMaterial(AngelScript::asIScriptEngine* engine);
void registerOgreTechnique(AngelScript::asIScriptEngine* engine);
void registerOgrePass(AngelScript::asIScriptEngine* engine);
void registerOgreTextureUnitState(AngelScript::asIScriptEngine* engine);
void registerOgreTimer(AngelScript::asIScriptEngine* engine);
void registerOgreGpuProgramParameters(AngelScript::asIScriptEngine* engine);
void registerOgreLight(AngelScript::asIScriptEngine* engine);

// main registration method
void RoR::RegisterOgreObjectsNative(AngelScript::asIScriptEngine* engine)
{
    int r;


    // More data types - the low-level scene API, under namespace `Ogre`

    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectType("Node", sizeof(Node), asOBJ_REF | asOBJ_NOCOUNT);
    ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectType("SceneNode", sizeof(SceneNode), asOBJ_REF | asOBJ_NOCOUNT);
    ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectType("SceneManager", sizeof(SceneManager), asOBJ_REF | asOBJ_NOCOUNT);
    ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectType("Root", sizeof(Root), asOBJ_REF | asOBJ_NOCOUNT);
    ROR_ASSERT(r >= 0);

    // dictionary/array view types, also under namespace `Ogre`

    SceneManagerInstanceDict::RegisterReadonlyScriptDictView(engine, "SceneManagerInstanceDict", "SceneManager");
    MovableObjectArray::RegisterReadonlyScriptArrayView(engine, "MovableObjectArray", "MovableObject");
    ChildNodeArray::RegisterReadonlyScriptArrayView(engine, "ChildNodeArray", "Node");
    AnimationStateDict::RegisterReadonlyScriptDictView(engine, "AnimationStateDict", "AnimationState");
    SubMeshArray::RegisterReadonlyScriptArrayView(engine, "SubMeshArray", "SubMesh");
    TechniqueArray::RegisterReadonlyScriptArrayView(engine, "TechniqueArray", "Technique");
    PassArray::RegisterReadonlyScriptArrayView(engine, "PassArray", "Pass");
    TextureUnitStateArray::RegisterReadonlyScriptArrayView(engine, "TextureUnitStateArray", "TextureUnitState");
    SubEntityArray::RegisterReadonlyScriptArrayView(engine, "SubEntityArray", "SubEntity");

    // enums, also under namespace `Ogre`

    r = engine->RegisterEnum("TransformSpace"); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("TransformSpace", "TS_LOCAL", Node::TS_LOCAL); ROR_ASSERT(r >= 0); // Transform is relative to the local space
    r = engine->RegisterEnumValue("TransformSpace", "TS_PARENT", Node::TS_PARENT); ROR_ASSERT(r >= 0); // Transform is relative to the space of the parent node
    r = engine->RegisterEnumValue("TransformSpace", "TS_WORLD", Node::TS_WORLD); ROR_ASSERT(r >= 0); // Transform is relative to world space

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);

    // Now we register the object properties and methods

    registerOgreRadian(engine);
    registerOgreDegree(engine);
    registerOgreVector3(engine);
    registerOgreVector2(engine);
    registerOgreQuaternion(engine);
    registerOgreColourValue(engine);
    registerOgreBox(engine);

    registerOgreNode(engine);
    registerOgreMovableObject(engine);
    registerOgreEntity(engine);
    registerOgreSubEntity(engine);
    registerOgreSceneNode(engine);
    registerOgreSceneManager(engine);
    registerOgreRoot(engine);
    registerOgreAnimationState(engine);
    registerOgreAnimationStateSet(engine);
    registerOgreTexture(engine);
    registerOgreTextureManager(engine);
    registerOgreOverlay(engine);
    registerOgreManualObject(engine);
    registerOgreHardwarePixelBuffer(engine);
    registerOgrePixelBox(engine);
    registerOgreImage(engine);
    registerOgreMesh(engine);
    registerOgreSubMesh(engine);
    registerOgreMeshManager(engine);
    registerOgreMaterial(engine);
    registerOgreTechnique(engine);
    registerOgrePass(engine);
    registerOgreTextureUnitState(engine);
    registerOgreMaterialManager(engine);
    registerOgreTimer(engine);
    registerOgreGpuProgramParameters(engine);
    registerOgreLight(engine);

    // To estabilish class hierarchy in AngelScript you need to register the reference cast operators opCast and opImplCast.

    // - `SceneNode` derives from `Node`
    r = engine->RegisterObjectMethod("Ogre::Node", "Ogre::SceneNode@ opCast()", asFUNCTION((ScriptRefCastNoCount<Ogre::Node, Ogre::SceneNode>)), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectMethod("Ogre::SceneNode", "Ogre::Node@ opImplCast()", asFUNCTION((ScriptRefCastNoCount<Ogre::SceneNode, Ogre::Node>)), asCALL_CDECL_OBJLAST); assert(r >= 0);
    // - `Entity` derives from `MovableObject`
    r = engine->RegisterObjectMethod("Ogre::MovableObject", "Ogre::Entity@ opCast()", asFUNCTION((ScriptRefCastNoCount<Ogre::MovableObject, Ogre::Entity>)), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectMethod("Ogre::Entity", "Ogre::MovableObject@ opImplCast()", asFUNCTION((ScriptRefCastNoCount<Ogre::Entity, Ogre::MovableObject>)), asCALL_CDECL_OBJLAST); assert(r >= 0);
    // - `ManualObject` derives from `MovableObject`
    r = engine->RegisterObjectMethod("Ogre::MovableObject", "Ogre::ManualObject@ opCast()", asFUNCTION((ScriptRefCastNoCount<Ogre::MovableObject, Ogre::ManualObject>)), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectMethod("Ogre::ManualObject", "Ogre::MovableObject@ opImplCast()", asFUNCTION((ScriptRefCastNoCount<Ogre::ManualObject, Ogre::MovableObject>)), asCALL_CDECL_OBJLAST); assert(r >= 0);
    // - `Light` derives from `MovableObject`
    r = engine->RegisterObjectMethod("Ogre::MovableObject", "Ogre::Light@ opCast()", asFUNCTION((ScriptRefCastNoCount<Ogre::MovableObject, Ogre::Light>)), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectMethod("Ogre::Light", "Ogre::MovableObject@ opImplCast()", asFUNCTION((ScriptRefCastNoCount<Ogre::Light, Ogre::MovableObject>)), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Also register the const overloads so the cast works also when the handle is read only
 
    // - `SceneNode` derives from `Node`
    r = engine->RegisterObjectMethod("Ogre::Node", "const Ogre::SceneNode@ opCast() const", asFUNCTION((ScriptRefCastNoCount<Ogre::Node, Ogre::SceneNode>)), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectMethod("Ogre::SceneNode", "const Ogre::Node@ opImplCast() const", asFUNCTION((ScriptRefCastNoCount<Ogre::SceneNode, Ogre::Node>)), asCALL_CDECL_OBJLAST); assert(r >= 0);
    // - `Entity` derives from `MovableObject`
    r = engine->RegisterObjectMethod("Ogre::MovableObject", "const Ogre::Entity@ opCast() const", asFUNCTION((ScriptRefCastNoCount<Ogre::MovableObject, Ogre::Entity>)), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectMethod("Ogre::Entity", "const Ogre::MovableObject@ opImplCast() const", asFUNCTION((ScriptRefCastNoCount<Ogre::Entity, Ogre::MovableObject>)), asCALL_CDECL_OBJLAST); assert(r >= 0);
    // - `ManualObject` derives from `MovableObject`
    r = engine->RegisterObjectMethod("Ogre::MovableObject", "const Ogre::ManualObject@ opCast() const", asFUNCTION((ScriptRefCastNoCount<Ogre::MovableObject, Ogre::ManualObject>)), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectMethod("Ogre::ManualObject", "const Ogre::MovableObject@ opImplCast() const", asFUNCTION((ScriptRefCastNoCount<Ogre::ManualObject, Ogre::MovableObject>)), asCALL_CDECL_OBJLAST); assert(r >= 0);
    // - `Light` derives from `MovableObject`
    r = engine->RegisterObjectMethod("Ogre::MovableObject", "const Ogre::Light@ opCast() const", asFUNCTION((ScriptRefCastNoCount<Ogre::MovableObject, Ogre::Light>)), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectMethod("Ogre::Light", "const Ogre::MovableObject@ opImplCast() const", asFUNCTION((ScriptRefCastNoCount<Ogre::Light, Ogre::MovableObject>)), asCALL_CDECL_OBJLAST); assert(r >= 0);
}

// register Ogre::Vector3
void registerOgreVector3(AngelScript::asIScriptEngine* engine)
{
    int r;

    // Register the object constructors
    r = engine->RegisterObjectBehaviour("vector3", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Vector3DefaultConstructor), asCALL_CDECL_OBJLAST);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("vector3", asBEHAVE_CONSTRUCT, "void f(float, float, float)", asFUNCTION(Vector3InitConstructor), asCALL_CDECL_OBJLAST);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("vector3", asBEHAVE_CONSTRUCT, "void f(const vector3 &in)", asFUNCTION(Vector3CopyConstructor), asCALL_CDECL_OBJLAST);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("vector3", asBEHAVE_CONSTRUCT, "void f(float)", asFUNCTION(Vector3InitConstructorScaler), asCALL_CDECL_OBJLAST);
    ROR_ASSERT( r >= 0 );

    // Register the object operators
    r = engine->RegisterObjectMethod("vector3", "float opIndex(int) const", asMETHODPR(Vector3, operator[], (size_t) const, float), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 &f(const vector3 &in)", asMETHODPR(Vector3, operator =, (const Vector3 &), Vector3&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "bool opEquals(const vector3 &in) const", asMETHODPR(Vector3, operator==,(const Vector3&) const, bool), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector3", "vector3 opAdd(const vector3 &in) const", asMETHODPR(Vector3, operator+,(const Vector3&) const, Vector3), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 opSub(const vector3 &in) const", asMETHODPR(Vector3, operator-,(const Vector3&) const, Vector3), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector3", "vector3 opMul(float) const", asMETHODPR(Vector3, operator*,(const float) const, Vector3), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 opMul(const vector3 &in) const", asMETHODPR(Vector3, operator*,(const Vector3&) const, Vector3), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 opDiv(float) const", asMETHODPR(Vector3, operator/,(const float) const, Vector3), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 opDiv(const vector3 &in) const", asMETHODPR(Vector3, operator/,(const Vector3&) const, Vector3), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector3", "vector3 opAdd() const", asMETHODPR(Vector3, operator+,() const, const Vector3&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 opSub() const", asMETHODPR(Vector3, operator-,() const, Vector3), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    //r = engine->RegisterObjectMethod("vector3", "vector3 opMul(float, const vector3 &in)", asMETHODPR(Vector3, operator*,(const float, const Vector3&), Vector3), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector3", "vector3 &opAddAssign(const vector3 &in)", asMETHODPR(Vector3,operator+=,(const Vector3 &),Vector3&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 &opAddAssign(float)", asMETHODPR(Vector3,operator+=,(const float),Vector3&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector3", "vector3 &opSubAssign(const vector3 &in)", asMETHODPR(Vector3,operator-=,(const Vector3 &),Vector3&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 &opSubAssign(float)", asMETHODPR(Vector3,operator-=,(const float),Vector3&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector3", "vector3 &opMulAssign(const vector3 &in)", asMETHODPR(Vector3,operator*=,(const Vector3 &),Vector3&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 &opMulAssign(float)", asMETHODPR(Vector3,operator*=,(const float),Vector3&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    //r = engine->RegisterObjectMethod("vector3", "vector3& operator @= ( const vector3& rkVector f( const Vector3& rkVector )", asMETHOD(Ogre::Vector3, f), asCALL_THISCALL); ROR_ASSERT(r>=0);

    r = engine->RegisterObjectMethod("vector3", "vector3 &opDivAssign(const vector3 &in)", asMETHODPR(Vector3,operator/=,(const Vector3 &),Vector3&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 &opDivAssign(float)", asMETHODPR(Vector3,operator/=,(const float),Vector3&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    // Register the object methods
    // r = engine->RegisterObjectMethod("vector3", "void swap(vector3 &inout)",  asMETHOD(Vector3,swap), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector3", "float length() const", asMETHOD(Vector3,length), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "float squaredLength() const", asMETHOD(Vector3,squaredLength), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector3", "float distance(const vector3 &in) const", asMETHOD(Vector3,distance), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "float squaredDistance(const vector3 &in) const", asMETHOD(Vector3,squaredDistance), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector3", "float dotProduct(const vector3 &in) const", asMETHOD(Vector3,dotProduct), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "float absDotProduct(const vector3 &in) const", asMETHOD(Vector3,absDotProduct), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector3", "float normalise()", asMETHOD(Vector3,normalise), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 crossProduct(const vector3 &in) const", asMETHOD(Vector3,crossProduct), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 midPoint(const vector3 &in) const", asMETHOD(Vector3,midPoint), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "void makeFloor(const vector3 &in)", asMETHOD(Vector3,makeFloor), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "void makeCeil(const vector3 &in)", asMETHOD(Vector3,makeCeil), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 perpendicular() const", asMETHOD(Vector3,perpendicular), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 randomDeviant(const radian &in, const vector3 &in) const", asMETHOD(Vector3,randomDeviant), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "radian angleBetween(const vector3 &in)", asMETHOD(Vector3,angleBetween), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "quaternion getRotationTo(const vector3 &in, const vector3 &in) const", asMETHOD(Vector3,getRotationTo), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "bool isZeroLength() const", asMETHOD(Vector3,isZeroLength), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 normalisedCopy() const", asMETHOD(Vector3,normalisedCopy), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 reflect(const vector3 &in) const", asMETHOD(Vector3,reflect), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector3", "bool positionEquals(const vector3 &in, float) const", asMETHOD(Vector3,positionEquals), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "bool positionCloses(const vector3 &in, float) const", asMETHOD(Vector3,positionCloses), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "bool directionEquals(const vector3 &in, radian &in) const", asMETHOD(Vector3,directionEquals), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector3", "bool isNaN() const", asMETHOD(Vector3,isNaN), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
}


// register Ogre::Vector2
void registerOgreVector2(AngelScript::asIScriptEngine* engine)
{
    int r;

    // Register the object constructors
    r = engine->RegisterObjectBehaviour("vector2", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Vector2DefaultConstructor), asCALL_CDECL_OBJLAST);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("vector2", asBEHAVE_CONSTRUCT, "void f(float, float)", asFUNCTION(Vector2InitConstructor), asCALL_CDECL_OBJLAST);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("vector2", asBEHAVE_CONSTRUCT, "void f(const vector2 &in)", asFUNCTION(Vector2CopyConstructor), asCALL_CDECL_OBJLAST);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("vector2", asBEHAVE_CONSTRUCT, "void f(float)", asFUNCTION(Vector2InitConstructorScaler), asCALL_CDECL_OBJLAST);
    ROR_ASSERT( r >= 0 );

    // Register the object operators
    r = engine->RegisterObjectMethod("vector2", "float opIndex(int) const", asMETHODPR(Vector2, operator[], (size_t) const, float), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 &f(const vector2 &in)", asMETHODPR(Vector2, operator =, (const Vector2 &), Vector2&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "bool opEquals(const vector2 &in) const", asMETHODPR(Vector2, operator==,(const Vector2&) const, bool), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector2", "vector2 opAdd(const vector2 &in) const", asMETHODPR(Vector2, operator+,(const Vector2&) const, Vector2), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 opSub(const vector2 &in) const", asMETHODPR(Vector2, operator-,(const Vector2&) const, Vector2), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector2", "vector2 opMul(float) const", asMETHODPR(Vector2, operator*,(const float) const, Vector2), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 opMul(const vector2 &in) const", asMETHODPR(Vector2, operator*,(const Vector2&) const, Vector2), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 opDiv(float) const", asMETHODPR(Vector2, operator/,(const float) const, Vector2), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 opDiv(const vector2 &in) const", asMETHODPR(Vector2, operator/,(const Vector2&) const, Vector2), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector2", "vector2 opAdd() const", asMETHODPR(Vector2, operator+,() const, const Vector2&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 opSub() const", asMETHODPR(Vector2, operator-,() const, Vector2), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector2", "vector2 &opAddAssign(const vector2 &in)", asMETHODPR(Vector2,operator+=,(const Vector2 &),Vector2&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 &opAddAssign(float)", asMETHODPR(Vector2,operator+=,(const float),Vector2&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector2", "vector2 &opSubAssign(const vector2 &in)", asMETHODPR(Vector2,operator-=,(const Vector2 &),Vector2&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 &opSubAssign(float)", asMETHODPR(Vector2,operator-=,(const float),Vector2&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector2", "vector2 &opMulAssign(const vector2 &in)", asMETHODPR(Vector2,operator*=,(const Vector2 &),Vector2&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 &opMulAssign(float)", asMETHODPR(Vector2,operator*=,(const float),Vector2&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector2", "vector2 &opDivAssign(const vector2 &in)", asMETHODPR(Vector2,operator/=,(const Vector2 &),Vector2&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 &opDivAssign(float)", asMETHODPR(Vector2,operator/=,(const float),Vector2&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    // Register the object methods

    r = engine->RegisterObjectMethod("vector2", "float length() const", asMETHOD(Vector2,length), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "float squaredLength() const", asMETHOD(Vector2,squaredLength), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector2", "float distance(const vector2 &in) const", asMETHOD(Vector2,distance), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "float squaredDistance(const vector2 &in) const", asMETHOD(Vector2,squaredDistance), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector2", "float dotProduct(const vector2 &in) const", asMETHOD(Vector2,dotProduct), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector2", "float normalise()", asMETHOD(Vector2,normalise), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "float crossProduct(const vector2 &in) const", asMETHOD(Vector2,crossProduct), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 midPoint(const vector2 &in) const", asMETHOD(Vector2,midPoint), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "void makeFloor(const vector2 &in)", asMETHOD(Vector2,makeFloor), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "void makeCeil(const vector2 &in)", asMETHOD(Vector2,makeCeil), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 perpendicular() const", asMETHOD(Vector2,perpendicular), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 randomDeviant(const radian &in, const vector2 &in) const", asMETHOD(Vector2,randomDeviant), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "radian angleBetween(const vector2 &in)", asMETHOD(Vector2,angleBetween), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "bool isZeroLength() const", asMETHOD(Vector2,isZeroLength), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 normalisedCopy() const", asMETHOD(Vector2,normalisedCopy), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 reflect(const vector2 &in) const", asMETHOD(Vector2,reflect), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector2", "bool positionEquals(const vector2 &in, float) const", asMETHOD(Vector2,positionEquals), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector2", "bool isNaN() const", asMETHOD(Vector2,isNaN), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
}

void registerOgreRadian(AngelScript::asIScriptEngine* engine)
{
    int r;

    // Register the object constructors
    r = engine->RegisterObjectBehaviour("radian", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(RadianDefaultConstructor), asCALL_CDECL_OBJLAST);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("radian", asBEHAVE_CONSTRUCT, "void f(float)", asFUNCTION(RadianInitConstructor), asCALL_CDECL_OBJLAST);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("radian", asBEHAVE_CONSTRUCT, "void f(const radian &in)", asFUNCTION(RadianCopyConstructor), asCALL_CDECL_OBJLAST);
    ROR_ASSERT( r >= 0 );

  // Disabled during port to latest AngelScript because `asBEHAVE_IMPLICIT_VALUE_CAST` is deprecated.
  // TODO: Research and restore it ~ only_a_ptr, 08/2017
  //  // Register other object behaviours
  //  r = engine->RegisterObjectBehaviour("radian", asBEHAVE_IMPLICIT_VALUE_CAST, "float f() const", asMETHOD(Radian,valueRadians), asCALL_THISCALL);
  //  ROR_ASSERT( r >= 0 );
  //  r = engine->RegisterObjectBehaviour("radian", asBEHAVE_IMPLICIT_VALUE_CAST, "double f() const", asMETHOD(Radian,valueRadians), asCALL_THISCALL);
  //  ROR_ASSERT( r >= 0 );

    // Register the object operators
    r = engine->RegisterObjectMethod("radian", "radian &opAssign(const radian &in)", asMETHODPR(Radian, operator =, (const Radian &), Radian&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("radian", "radian &opAssign(const float)", asMETHODPR(Radian, operator =, (const float &), Radian&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("radian", "radian &opAssign(const degree &in)", asMETHODPR(Radian, operator =, (const Degree &), Radian&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("radian", "radian opAdd() const", asMETHODPR(Radian, operator+,() const, const Radian&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("radian", "radian opAdd(const radian &in) const", asMETHODPR(Radian, operator+,(const Radian&) const, Radian), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("radian", "radian opAdd(const degree &in) const", asMETHODPR(Radian, operator+,(const Degree&) const, Radian), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("radian", "radian &opAddAssign(const radian &in)", asMETHODPR(Radian,operator+=,(const Radian &),Radian&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("radian", "radian &opAddAssign(const degree &in)", asMETHODPR(Radian,operator+=,(const Degree &),Radian&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("radian", "radian opSub() const", asMETHODPR(Radian, operator-,() const, Radian), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("radian", "radian opSub(const radian &in) const", asMETHODPR(Radian, operator-,(const Radian&) const, Radian), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("radian", "radian opSub(const degree &in) const", asMETHODPR(Radian, operator-,(const Degree&) const, Radian), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("radian", "radian &opSubAssign(const radian &in)", asMETHODPR(Radian,operator-=,(const Radian &),Radian&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("radian", "radian &opSubAssign(const degree &in)", asMETHODPR(Radian,operator-=,(const Degree &),Radian&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("radian", "radian opMul(float) const", asMETHODPR(Radian, operator*,(float) const, Radian), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("radian", "radian opMul(const radian &in) const", asMETHODPR(Radian, operator*,(const Radian&) const, Radian), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("radian", "radian &opMulAssign(float)", asMETHODPR(Radian,operator*=,(float),Radian&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("radian", "radian opDiv(float) const", asMETHODPR(Radian, operator/,(float) const, Radian), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("radian", "radian &opDivAssign(float)", asMETHODPR(Radian,operator*=,(float),Radian&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("radian", "int opCmp(const radian &in) const", asFUNCTION(RadianCmp), asCALL_CDECL_OBJFIRST);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("radian", "bool opEquals(const radian &in) const", asMETHODPR(Radian, operator==,(const Radian&) const, bool), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    // Register the object methods
    r = engine->RegisterObjectMethod("radian", "float valueDegrees() const", asMETHOD(Radian,valueDegrees), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("radian", "float valueRadians() const", asMETHOD(Radian,valueRadians), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("radian", "float valueAngleUnits() const", asMETHOD(Radian,valueAngleUnits), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
}

void registerOgreDegree(AngelScript::asIScriptEngine* engine)
{
    int r;

    // Register the object constructors
    r = engine->RegisterObjectBehaviour("degree", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(DegreeDefaultConstructor), asCALL_CDECL_OBJLAST);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("degree", asBEHAVE_CONSTRUCT, "void f(float)", asFUNCTION(DegreeInitConstructor), asCALL_CDECL_OBJLAST);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("degree", asBEHAVE_CONSTRUCT, "void f(const degree &in)", asFUNCTION(DegreeCopyConstructor), asCALL_CDECL_OBJLAST);
    ROR_ASSERT( r >= 0 );

  // Disabled during port to latest AngelScript because `asBEHAVE_IMPLICIT_VALUE_CAST` is deprecated.
  // TODO: Research and restore it ~ only_a_ptr, 08/2017
  //  // Register other object behaviours
  //  r = engine->RegisterObjectBehaviour("degree", asBEHAVE_IMPLICIT_VALUE_CAST, "float f() const", asMETHOD(Degree,valueDegrees), asCALL_THISCALL);
  //  ROR_ASSERT( r >= 0 );
  //  r = engine->RegisterObjectBehaviour("degree", asBEHAVE_IMPLICIT_VALUE_CAST, "double f() const", asMETHOD(Degree,valueDegrees), asCALL_THISCALL);
  //  ROR_ASSERT( r >= 0 );

    // Register the object operators
    r = engine->RegisterObjectMethod("degree", "degree &opAssign(const degree &in)", asMETHODPR(Degree, operator =, (const Degree &), Degree&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("degree", "degree &opAssign(float)", asMETHODPR(Degree, operator =, (const float &), Degree&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("degree", "degree &opAssign(const radian &in)", asMETHODPR(Degree, operator =, (const Radian &), Degree&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("degree", "degree opAdd() const", asMETHODPR(Degree, operator+,() const, const Degree&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("degree", "degree opAdd(const degree &in) const", asMETHODPR(Degree, operator+,(const Degree&) const, Degree), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("degree", "degree opAdd(const radian &in) const", asMETHODPR(Degree, operator+,(const Radian&) const, Degree), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("degree", "degree &opAddAssign(const degree &in)", asMETHODPR(Degree,operator+=,(const Degree &),Degree&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("degree", "degree &opAddAssign(const radian &in)", asMETHODPR(Degree,operator+=,(const Radian &),Degree&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("degree", "degree opSub() const", asMETHODPR(Degree, operator-,() const, Degree), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("degree", "degree opSub(const degree &in) const", asMETHODPR(Degree, operator-,(const Degree&) const, Degree), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("degree", "degree opSub(const radian &in) const", asMETHODPR(Degree, operator-,(const Radian&) const, Degree), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("degree", "degree &opSubAssign(const degree &in)", asMETHODPR(Degree,operator-=,(const Degree &),Degree&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("degree", "degree &opSubAssign(const radian &in)", asMETHODPR(Degree,operator-=,(const Radian &),Degree&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("degree", "degree opMul(float) const", asMETHODPR(Degree, operator*,(float) const, Degree), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("degree", "degree opMul(const degree &in) const", asMETHODPR(Degree, operator*,(const Degree&) const, Degree), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("degree", "degree &opMulAssign(float)", asMETHODPR(Degree,operator*=,(float),Degree&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("degree", "degree opDiv(float) const", asMETHODPR(Degree, operator/,(float) const, Degree), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("degree", "degree &opDivAssign(float)", asMETHODPR(Degree,operator*=,(float),Degree&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("degree", "int opCmp(const degree &in) const", asFUNCTION(DegreeCmp), asCALL_CDECL_OBJFIRST);
    ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("degree", "bool opEquals(const degree &in) const", asMETHODPR(Degree, operator==,(const Degree&) const, bool), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    // Register the object methods
    r = engine->RegisterObjectMethod("degree", "float valueRadians() const", asMETHOD(Degree,valueRadians), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("degree", "float valueDegrees() const", asMETHOD(Degree,valueDegrees), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("degree", "float valueAngleUnits() const", asMETHOD(Degree,valueAngleUnits), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
}

void registerOgreQuaternion(AngelScript::asIScriptEngine* engine)
{
    int r;

    // Register the object constructors
    r = engine->RegisterObjectBehaviour("quaternion", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(QuaternionDefaultConstructor), asCALL_CDECL_OBJLAST);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("quaternion", asBEHAVE_CONSTRUCT, "void f(const radian &in, const vector3 &in)", asFUNCTION(QuaternionInitConstructor1), asCALL_CDECL_OBJLAST);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("quaternion", asBEHAVE_CONSTRUCT, "void f(float, float, float, float)", asFUNCTION(QuaternionInitConstructor2), asCALL_CDECL_OBJLAST);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("quaternion", asBEHAVE_CONSTRUCT, "void f(const vector3 &in, const vector3 &in, const vector3 &in)", asFUNCTION(QuaternionInitConstructor3), asCALL_CDECL_OBJLAST);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("quaternion", asBEHAVE_CONSTRUCT, "void f(float)", asFUNCTION(QuaternionInitConstructorScaler), asCALL_CDECL_OBJLAST);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("quaternion", asBEHAVE_CONSTRUCT, "void f(const quaternion &in)", asFUNCTION(QuaternionCopyConstructor), asCALL_CDECL_OBJLAST);
    ROR_ASSERT( r >= 0 );

    // Register the object operators
    r = engine->RegisterObjectMethod("quaternion", "float opIndex(int) const", asMETHODPR(Quaternion, operator[], (size_t) const, float), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "quaternion &opAssign(const quaternion &in)", asMETHODPR(Quaternion, operator =, (const Quaternion &), Quaternion&), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "quaternion opAdd(const quaternion &in) const", asMETHODPR(Quaternion, operator+,(const Quaternion&) const, Quaternion), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "quaternion opSub(const quaternion &in) const", asMETHODPR(Quaternion, operator-,(const Quaternion&) const, Quaternion), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "quaternion opMul(const quaternion &in) const", asMETHODPR(Quaternion, operator*,(const Quaternion&) const, Quaternion), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "quaternion opMul(float) const", asMETHODPR(Quaternion, operator*,(float) const, Quaternion), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "quaternion opSub() const", asMETHODPR(Quaternion, operator-,() const, Quaternion), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "bool opEquals(const quaternion &in) const", asMETHODPR(Quaternion, operator==,(const Quaternion&) const, bool), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "vector3 opMul(const vector3 &in) const", asMETHODPR(Quaternion, operator*,(const Vector3&) const, Vector3), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    // Register the object methods
    r = engine->RegisterObjectMethod("quaternion", "float Dot(const quaternion &in) const", asMETHOD(Quaternion,Dot), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "float Norm() const", asMETHOD(Quaternion,Norm), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "float normalise()", asMETHOD(Quaternion,normalise), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "quaternion Inverse() const", asMETHOD(Quaternion,Inverse), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "quaternion UnitInverse() const", asMETHOD(Quaternion,UnitInverse), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "quaternion Exp() const", asMETHOD(Quaternion,Exp), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "quaternion Log() const", asMETHOD(Quaternion,Log), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "radian getRoll(bool reprojectAxis = true) const", asMETHOD(Quaternion,getRoll), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "radian getPitch(bool reprojectAxis = true) const", asMETHOD(Quaternion,getPitch), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "radian getYaw(bool reprojectAxis = true) const", asMETHOD(Quaternion,getYaw), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "bool equals(const quaternion &in, const radian &in) const", asMETHOD(Quaternion,equals), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "bool isNaN() const", asMETHOD(Quaternion,isNaN), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );

    // Register some static methods
    r = engine->RegisterGlobalFunction("quaternion Slerp(float, const quaternion &in, const quaternion &in, bool &in)", asFUNCTIONPR(Quaternion::Slerp,(Real fT, const Quaternion&, const Quaternion&, bool), Quaternion), asCALL_CDECL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterGlobalFunction("quaternion SlerpExtraSpins(float, const quaternion &in, const quaternion &in, int &in)", asFUNCTION(Quaternion::SlerpExtraSpins), asCALL_CDECL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterGlobalFunction("void Intermediate(const quaternion &in, const quaternion &in, const quaternion &in, const quaternion &in, const quaternion &in)", asFUNCTION(Quaternion::Intermediate), asCALL_CDECL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterGlobalFunction("quaternion Squad(float, const quaternion &in, const quaternion &in, const quaternion &in, const quaternion &in, bool &in)", asFUNCTION(Quaternion::Squad), asCALL_CDECL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterGlobalFunction("quaternion nlerp(float, const quaternion &in, const quaternion &in, bool &in)", asFUNCTION(Quaternion::nlerp), asCALL_CDECL);
    ROR_ASSERT( r >= 0 );
}

void registerOgreColourValue(AngelScript::asIScriptEngine* engine)
{
    int r;

    // Register the object constructors
    r = engine->RegisterObjectBehaviour("color", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ColourValueDefaultConstructor), asCALL_CDECL_OBJLAST);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("color", asBEHAVE_CONSTRUCT, "void f(float r, float g, float b, float a)", asFUNCTION(ColourValueInitConstructor), asCALL_CDECL_OBJLAST);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("color", asBEHAVE_CONSTRUCT, "void f(const color &other)", asFUNCTION(QuaternionCopyConstructor), asCALL_CDECL_OBJLAST);
}

void registerOgreBox(AngelScript::asIScriptEngine* engine)
{
    int r;

    // Register the object constructors
    r = engine->RegisterObjectBehaviour("box", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(BoxDefaultConstructor), asCALL_CDECL_OBJLAST);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("box", asBEHAVE_CONSTRUCT, "void f(uint32 l, uint32 t, uint32 r, uint32 b)", asFUNCTION(BoxInitConstructor), asCALL_CDECL_OBJLAST);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("box", asBEHAVE_CONSTRUCT, "void f(const box &other)", asFUNCTION(BoxCopyConstructor), asCALL_CDECL_OBJLAST);

    // Register other behaviors
    r = engine->RegisterObjectBehaviour("box", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(BoxDestructor), asCALL_CDECL_OBJLAST);
    ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("box", "box& opAssign(const box&in)", asFUNCTION(BoxAssignOperator), asCALL_CDECL_OBJLAST);
    ROR_ASSERT(r >= 0);
}

void registerOgreTexture(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectBehaviour("TexturePtr", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(TexturePtrDefaultConstructor), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("TexturePtr", asBEHAVE_CONSTRUCT, "void f(const TexturePtr&in)", asFUNCTION(TexturePtrCopyConstructor), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("TexturePtr", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(TexturePtrDestructor), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("TexturePtr", "TexturePtr& opAssign(const TexturePtr&in)", asFUNCTION(TexturePtrAssignOperator), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("TexturePtr", "bool isNull()", asFUNCTION(TexturePtrIsNull), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);

    // Wrappers are inevitable, see https://www.gamedev.net/forums/topic/540419-custom-smartpointers-and-angelscript-/
    r = engine->RegisterObjectMethod("TexturePtr", "string getName() const", asFUNCTION(TexturePtrGetName), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("TexturePtr", "uint getWidth()", asFUNCTION(TexturePtrGetWidth), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("TexturePtr", "uint getHeight()", asFUNCTION(TexturePtrGetHeight), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("TexturePtr", "uint getNumMipmaps()", asFUNCTION(TexturePtrGetNumMipmaps), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("TexturePtr", "HardwarePixelBufferPtr getBuffer(uint, uint)", asFUNCTION(TexturePtrGetBuffer), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

void registerOgreTextureManager(AngelScript::asIScriptEngine * engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    // Convenience wrapper to omit optional parameters
    r = engine->RegisterObjectMethod("TextureManager", "TexturePtr load(const string&in file, const string&in rg)", asFUNCTION(TextureManagerLoad), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace("Ogre::TextureManager"); ROR_ASSERT(r >= 0);
    r = engine->RegisterGlobalFunction("TextureManager& getSingleton()", asFUNCTION(TextureManager::getSingleton), asCALL_CDECL); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

template <typename T>
void registerOgreMovableObjectBase(AngelScript::asIScriptEngine* engine, const char* obj)
{
    int r;
    r = engine->RegisterObjectMethod(obj, "string __getUniqueName() const", asFUNCTION(MovableObjectGetUniqueNameMixin), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod(obj, "const string& getName() const", asMETHOD(MovableObject, getName), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "const string& getMovableType() const", asMETHOD(MovableObject, getMovableType), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod(obj, "Node@ getParentNode()", asMETHOD(MovableObject, getParentNode), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "SceneNode@ getParentSceneNode()", asMETHOD(MovableObject, getParentSceneNode), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "bool isParentTagPoint() const", asMETHOD(MovableObject, isParentTagPoint), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "bool isAttached() const", asMETHOD(MovableObject, isAttached), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "void detachFromParent()", asMETHOD(MovableObject, detachFromParent), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod(obj, "bool isInScene() const", asMETHOD(MovableObject, isInScene), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "float getBoundingRadius() const", asMETHOD(MovableObject, getBoundingRadius), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod(obj, "void setVisible(bool visible)", asMETHOD(MovableObject, setVisible), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "bool getVisible() const", asMETHOD(MovableObject, getVisible), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "bool isVisible() const", asMETHOD(MovableObject, isVisible), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod(obj, "void setRenderingDistance(float dist)", asMETHOD(MovableObject, setRenderingDistance), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "float getRenderingDistance() const", asMETHOD(MovableObject, getRenderingDistance), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod(obj, "void setRenderingMinPixelSize(float pixelSize)", asMETHOD(MovableObject, setRenderingMinPixelSize), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "float getRenderingMinPixelSize() const", asMETHOD(MovableObject, getRenderingMinPixelSize), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod(obj, "void setCastShadows(bool enabled)", asMETHOD(MovableObject, setCastShadows), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "bool getCastShadows() const", asMETHOD(MovableObject, getCastShadows), asCALL_THISCALL); ROR_ASSERT(r >= 0);
}

void registerOgreMovableObject(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    registerOgreMovableObjectBase<MovableObject>(engine, "MovableObject");

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

void registerOgreEntity(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("Entity", "void setMaterialName(const string &in name, const string &in rg = \"OgreAutodetect\")", asMETHOD(Entity, setMaterialName), asCALL_THISCALL);  ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Entity", "AnimationState @getAnimationState(const string &in) const", asMETHOD(Entity, getAnimationState), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Entity", "AnimationStateSet @getAllAnimationStates()", asMETHOD(Entity, getAllAnimationStates), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Entity", "void setDisplaySkeleton(bool)", asMETHOD(Entity, setDisplaySkeleton), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Entity", "bool getDisplaySkeleton() const", asMETHOD(Entity, getDisplaySkeleton), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Entity", "uint64 getNumManualLodLevels() const", asMETHOD(Entity, getNumManualLodLevels), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Entity", "uint16 getCurrentLodIndex()", asMETHOD(Entity, getCurrentLodIndex), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Entity", "Entity @getManualLodLevel(uint64) const", asMETHOD(Entity, getManualLodLevel), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Entity", "void setMeshLodBias(float, uint16, uint16)", asMETHOD(Entity, setMeshLodBias), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Entity", "void setMaterialLodBias(float, uint16, uint16)", asMETHOD(Entity, setMaterialLodBias), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Entity", "SubEntityArray @getSubEntities() const", asFUNCTION(EntityGetSubEntities), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Entity", "const MeshPtr& getMesh() const", asMETHOD(Entity, getMesh), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    
    registerOgreMovableObjectBase<MovableObject>(engine, "Entity");

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

void registerOgreSubEntity(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("SubEntity", "const MaterialPtr& getMaterial() const", asMETHOD(SubEntity, getMaterial), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SubEntity", "void setMaterial(const MaterialPtr&in)", asMETHOD(SubEntity, setMaterial), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SubEntity", "SubMesh@ getSubMesh()", asMETHOD(SubEntity, getSubMesh), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

template <typename T>
void registerOgreNodeBase(AngelScript::asIScriptEngine* engine, const char* obj)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod(obj, "const vector3& getPosition() const", asMETHOD(T, getPosition), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "void setPosition(const vector3 &in)", asMETHODPR(T, setPosition, (const Vector3&), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod(obj, "const vector3& getScale() const", asMETHOD(T, getScale), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "void setScale(const vector3 &in)", asMETHODPR(T, setScale, (const Ogre::Vector3&), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod(obj, "const string& getName() const", asMETHOD(T, getName), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "Node@ getParent()", asMETHOD(T, getParent), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod(obj, "const quaternion& getOrientation() const", asMETHOD(T, getOrientation), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "void setOrientation(const quaternion&in)", asFUNCTIONPR([](T* self, const Ogre::Quaternion& q){
        self->setOrientation(q);
        }, (T*, const Quaternion&), void), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod(obj, "string __getUniqueName() const", asFUNCTION(NodeGetUniqueNameMixin), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    
    // Not const because we don't want all elements to be const (this isn't the case with raw pointers in C++).
    r = engine->RegisterObjectMethod(obj, "ChildNodeArray@ getChildren()", asFUNCTION(NodeGetChildren), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

void registerOgreNode(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    registerOgreNodeBase<Node>(engine, "Node");

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

void registerOgreSceneNode(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("SceneNode", "void attachObject(MovableObject@ obj)", asMETHODPR(SceneNode, attachObject, (MovableObject*), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "MovableObject@ getAttachedObject(const string& in)", asMETHODPR(SceneNode, getAttachedObject, (const String&), MovableObject*), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "MovableObject@ detachObject(uint16)", asMETHODPR(SceneNode, detachObject, (uint16), MovableObject*), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "void detachObject(MovableObject@ obj)", asMETHODPR(SceneNode, detachObject, (MovableObject*), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "MovableObject@ detachObject(const string& in)", asMETHODPR(SceneNode, detachObject, (const String&), MovableObject*), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "void detachAllObjects()", asMETHOD(SceneNode, detachAllObjects), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "bool isInSceneGraph() const", asMETHOD(SceneNode, isInSceneGraph), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "SceneManager@ getCreator() const", asMETHOD(SceneNode, getCreator), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "void removeAndDestroyChild(const string& in)", asMETHODPR(SceneNode, removeAndDestroyChild, (const String&), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "void removeAndDestroyChild(uint16)", asMETHODPR(SceneNode, removeAndDestroyChild, (uint16), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "void removeAndDestroyChild(SceneNode@)", asMETHODPR(SceneNode, removeAndDestroyChild, (uint16), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "void removeAndDestroyAllChildren()", asMETHOD(SceneNode, removeAndDestroyAllChildren), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "void showBoundingBox(bool bShow)", asMETHOD(SceneNode, showBoundingBox), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "void hideBoundingBox(bool bHide)", asMETHOD(SceneNode, hideBoundingBox), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "bool getShowBoundingBox() const", asMETHOD(SceneNode, getShowBoundingBox), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "SceneNode@ createChildSceneNode(const string& in name, const vector3& in translate = vector3(0.f, 0.f, 0.f), const quaternion& in rotate = quaternion())", asMETHODPR(SceneNode, createChildSceneNode, (const String&, const Vector3&, const Quaternion&), SceneNode*), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "void setFixedYawAxis(bool useFixed, const vector3& in fixedAxis = vector3(0.f, 1.f, 0.f))", asMETHOD(SceneNode, setFixedYawAxis), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "void yaw(const radian& in angle, TransformSpace relativeTo = Ogre::TS_LOCAL)", asMETHOD(SceneNode, yaw), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "void setDirection(const vector3& in vec, TransformSpace relativeTo = Ogre::TS_LOCAL, const vector3& in localDirectionVector = vector3(0.f, 0.f, -1.f))", asMETHODPR(SceneNode, setDirection, (const Vector3&, Node::TransformSpace, const Vector3&), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "SceneNode@ getParentSceneNode() const", asMETHOD(SceneNode, getParentSceneNode), asCALL_THISCALL); ROR_ASSERT(r >= 0);    
    // Not const because we don't want all elements to be const (this isn't the case with raw pointers in C++).
    r = engine->RegisterObjectMethod("SceneNode", "MovableObjectArray@ getAttachedObjects()", asFUNCTION(SceneNodeGetAttachedObjects), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "void lookAt(const vector3 &in, TransformSpace, const vector3 &in = vector3(0,0,-1))", asMETHODPR(SceneNode, lookAt, (const Vector3&, Node::TransformSpace, const Vector3&), void), asCALL_THISCALL);
    r = engine->RegisterObjectMethod("SceneNode", "void setAutoTracking(bool, SceneNode@, const vector3 &in = vector3(0,0,-1), const vector3 &in = vector3())", asMETHODPR(SceneNode, setAutoTracking, (bool, SceneNode* const, const Vector3&, const Vector3&), void), asCALL_THISCALL);
    r = engine->RegisterObjectMethod("SceneNode", "SceneNode@ getAutoTrackTarget()", asMETHODPR(SceneNode, getAutoTrackTarget, (), SceneNode*), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "const vector3& getAutoTrackOffset()", asMETHODPR(SceneNode, getAutoTrackOffset, (), const Vector3&), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "const vector3& getAutoTrackLocalDirection()", asMETHODPR(SceneNode, getAutoTrackLocalDirection, (), const Vector3&), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "SceneNode@ getParentSceneNode()", asMETHODPR(SceneNode, getParentSceneNode, () const, SceneNode*), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "void setVisible(bool, bool cascade = true)", asMETHODPR(SceneNode, setVisible, (bool, bool), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "void flipVisibility(bool = true)", asMETHODPR(SceneNode, flipVisibility, (bool), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneNode", "void setDebugDisplayEnabled(bool, bool cascade = true)", asMETHODPR(SceneNode, setDebugDisplayEnabled, (bool, bool), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    registerOgreNodeBase<SceneNode>(engine, "SceneNode");

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

void registerOgreSceneManager(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("SceneManager", "const string& getName() const", asMETHOD(SceneManager, getName), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("SceneManager", "array<MovableObject@>@ __getMovableObjectsByType(const string&in typeName)", asFUNCTION(SceneManager__getMovableObjectsByType), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    // Entities
    r = engine->RegisterObjectMethod("SceneManager", "Entity@ createEntity(const string&in ent_name, const string &in mesh_name, const string &in mesh_rg = \"OgreAutodetect\")", asFUNCTIONPR([](Ogre::SceneManager* self, const std::string& entityName, const std::string& meshName, const std::string& meshRG) -> Ogre::Entity* {
        try { return self->createEntity(entityName, meshName, meshRG); }
        catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::SceneManager::createEntity()"); return nullptr; }
    }, (Ogre::SceneManager* , const std::string& , const std::string& , const std::string& ), Ogre::Entity*), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneManager", "void destroyEntity(Entity@)", asMETHODPR(SceneManager, destroyEntity, (Entity*), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneManager", "void destroyEntity(const string &in)", asMETHODPR(SceneManager, destroyEntity, (const Ogre::String&), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    // Scene nodes
    r = engine->RegisterObjectMethod("SceneManager", "SceneNode@ getRootSceneNode()", asMETHOD(SceneManager, getRootSceneNode), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneManager", "void destroySceneNode(SceneNode@)", asMETHODPR(SceneManager, destroySceneNode, (SceneNode*), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneManager", "void destroySceneNode(const string &in)", asMETHODPR(SceneManager, destroySceneNode, (const Ogre::String&), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    // Ambient light
    r = engine->RegisterObjectMethod("SceneManager", "const color& getAmbientLight() const", asMETHOD(SceneManager, getAmbientLight), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneManager", "void setAmbientLight(const color &in)", asMETHODPR(SceneManager, setAmbientLight, (const ColourValue&), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    // ManualObject:
    r = engine->RegisterObjectMethod("SceneManager", "ManualObject@ createManualObject(const string &in)", asMETHODPR(SceneManager, createManualObject, (const Ogre::String&), Ogre::ManualObject*), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneManager", "ManualObject@ getManualObject(const string &in)", asMETHODPR(SceneManager, getManualObject, (const Ogre::String&) const, Ogre::ManualObject*), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneManager", "ManualObject@ destroyManualObject(const string &in)", asMETHODPR(SceneManager, destroyManualObject, (const Ogre::String&), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("SceneManager", "void destroyManualObject(ManualObject@)", asMETHODPR(SceneManager, destroyManualObject, (Ogre::ManualObject*), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

void registerOgreRoot(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);
    
    r = engine->RegisterObjectMethod("Root", "SceneManagerInstanceDict@ getSceneManagers()", asFUNCTION(RootGetSceneManagers), asCALL_CDECL_OBJLAST);

    r = engine->SetDefaultNamespace("Ogre::Root");
    r = engine->RegisterGlobalFunction("Root& getSingleton()", asFUNCTION(Root::getSingleton), asCALL_CDECL);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

void registerOgreAnimationState(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    // Register the getters and setters
    r = engine->RegisterObjectMethod("AnimationState", "const string& getAnimationName() const", asMETHOD(AnimationState, getAnimationName), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "float getTimePosition() const", asMETHOD(AnimationState, getTimePosition), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "void setTimePosition(float)", asMETHOD(AnimationState, setTimePosition), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "float getLength() const", asMETHOD(AnimationState, getLength), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "void setLength(float)", asMETHOD(AnimationState, setLength), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "float getWeight() const", asMETHOD(AnimationState, getWeight), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "void setWeight(float)", asMETHOD(AnimationState, setWeight), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "void addTime(float)", asMETHOD(AnimationState, addTime), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "bool hasEnded() const", asMETHOD(AnimationState, hasEnded), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "bool getEnabled() const", asMETHOD(AnimationState, getEnabled), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "void setEnabled(bool)", asMETHOD(AnimationState, setEnabled), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "void setLoop(bool)", asMETHOD(AnimationState, setLoop), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "bool getLoop() const", asMETHOD(AnimationState, getLoop), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "AnimationStateSet@ getParent()", asMETHOD(AnimationState, getParent), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "void createBlendMask(uint blendMaskSizeHint, float initialWeight = 1.0f)", asMETHOD(AnimationState, createBlendMask), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "void destroyBlendMask()", asMETHOD(AnimationState, destroyBlendMask), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "bool hasBlendMask() const", asMETHOD(AnimationState, hasBlendMask), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "void setBlendMaskEntry(uint boneHandle, float weight)", asMETHOD(AnimationState, setBlendMaskEntry), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "float getBlendMaskEntry(uint boneHandle)", asMETHOD(AnimationState, getBlendMaskEntry), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

void registerOgreAnimationStateSet(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("AnimationStateSet", "AnimationState@ createAnimationState(const string& in, float, float, float = 1.0f, bool = false)", asMETHOD(AnimationStateSet, createAnimationState), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationStateSet", "AnimationState@ getAnimationState(const string& in) const", asMETHOD(AnimationStateSet, getAnimationState), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationStateSet", "bool hasAnimationState(const string& in) const", asMETHOD(AnimationStateSet, hasAnimationState), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationStateSet", "void removeAnimationState(const string& in)", asMETHOD(AnimationStateSet, removeAnimationState), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationStateSet", "void removeAllAnimationStates()", asMETHOD(AnimationStateSet, removeAllAnimationStates), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationStateSet", "AnimationStateDict@ getAnimationStates()", asFUNCTION(AnimationStateSetGetAnimationStates), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

AngelScript::CScriptArray* get2DElementsHelper(Ogre::Overlay* self)
{
    try { 
        const Ogre::Overlay::OverlayContainerList& ocList = self->get2DElements();
        AngelScript::asITypeInfo* typeinfo = App::GetScriptEngine()->getEngine()->GetTypeInfoByDecl("array<Ogre::OverlayElement@>");
        AngelScript::CScriptArray* arr = AngelScript::CScriptArray::Create(typeinfo);
        for (OverlayContainer* oc: ocList)
        {
            OverlayElement* elem = static_cast<Ogre::OverlayElement*>(oc);
            arr->InsertLast(&elem); // TORN HAIR HERE!! Don't forget to pass ref-types as pointer-to-pointer!!
        }
        return arr;
    }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::Overlay::get2DElements()"); return (CScriptArray*)nullptr; }
}

AngelScript::CScriptArray* getElementTemplatesHelper(Ogre::OverlayManager* self)
{
    try {
        auto iterable = self->getTemplateIterator();
        // we must cast on the go (unlike get2DElements() this actually returns list of Ogre::OverlayElement*), see ATTENTION! below.
        AngelScript::asITypeInfo* typeinfo = App::GetScriptEngine()->getEngine()->GetTypeInfoByDecl("array<Ogre::OverlayElement@>");
        AngelScript::CScriptArray* arr = AngelScript::CScriptArray::Create(typeinfo);
        for (auto& elem_pair: iterable) {
            OverlayElement* elem = static_cast<Ogre::OverlayElement*>(elem_pair.second);
            arr->InsertLast(&elem);  // TORN HAIR HERE!! Don't forget to pass ref-types as pointer-to-pointer!!
        }
        return arr; }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::OverlayManager::getTemplates()"); return (CScriptArray*)nullptr; }
}

void registerOgreOverlay(AngelScript::asIScriptEngine* engine)
{
    engine->SetDefaultNamespace("Ogre");

    // Register the GuiMetricsMode enum
    engine->RegisterEnum("GuiMetricsMode");
    engine->RegisterEnumValue("GuiMetricsMode", "GMM_PIXELS", Ogre::GMM_PIXELS);
    engine->RegisterEnumValue("GuiMetricsMode", "GMM_RELATIVE", Ogre::GMM_RELATIVE);
    engine->RegisterEnumValue("GuiMetricsMode", "GMM_RELATIVE_ASPECT_ADJUSTED", Ogre::GMM_RELATIVE_ASPECT_ADJUSTED);

    // Register the GuiHorizontalAlignment enum
    engine->RegisterEnum("GuiHorizontalAlignment");
    engine->RegisterEnumValue("GuiHorizontalAlignment", "GHA_LEFT", Ogre::GHA_LEFT);
    engine->RegisterEnumValue("GuiHorizontalAlignment", "GHA_CENTER", Ogre::GHA_CENTER);
    engine->RegisterEnumValue("GuiHorizontalAlignment", "GHA_RIGHT", Ogre::GHA_RIGHT);


    // Register the OverlayElement class 
    // ATTENTION! only instances of `OverlayContainer` (is-a `OverlayElement`) can actually be bound to overlays and retrieved from overlays
    //            - see `Ogre::Overlay::add2D()`, `Ogre::Overlay::get2DElements()` and APIDOC on `OverlayContainer`.
    //            This forces us to toss `dynamic_cast<>`s around :(
    // (order roughly matches OgreOverlayElement.h)
    engine->RegisterObjectType("OverlayElement", 0, asOBJ_REF | asOBJ_NOCOUNT);
    engine->RegisterObjectMethod("OverlayElement", "const string& getName() const", asMETHOD(Ogre::OverlayElement, getName), asCALL_THISCALL);
    // > visibility
    engine->RegisterObjectMethod("OverlayElement", "void show()", asMETHOD(Ogre::OverlayElement, show), asCALL_THISCALL);
    engine->RegisterObjectMethod("OverlayElement", "void hide()", asMETHOD(Ogre::OverlayElement, hide), asCALL_THISCALL);
    engine->RegisterObjectMethod("OverlayElement", "bool isVisible() const", asMETHOD(Ogre::OverlayElement, isVisible), asCALL_THISCALL);
    // > positioning
    engine->RegisterObjectMethod("OverlayElement", "void setPosition(float, float)", asMETHOD(Ogre::OverlayElement, setPosition), asCALL_THISCALL);
    engine->RegisterObjectMethod("OverlayElement", "void setDimensions(float, float)", asMETHOD(Ogre::OverlayElement, setDimensions), asCALL_THISCALL);
    engine->RegisterObjectMethod("OverlayElement", "float getLeft() const", asMETHOD(Ogre::OverlayElement, getLeft), asCALL_THISCALL);
    engine->RegisterObjectMethod("OverlayElement", "float getTop() const", asMETHOD(Ogre::OverlayElement, getTop), asCALL_THISCALL);
    engine->RegisterObjectMethod("OverlayElement", "float getWidth() const", asMETHOD(Ogre::OverlayElement, getWidth), asCALL_THISCALL);
    engine->RegisterObjectMethod("OverlayElement", "float getHeight() const", asMETHOD(Ogre::OverlayElement, getHeight), asCALL_THISCALL);
    engine->RegisterObjectMethod("OverlayElement", "void setLeft(float)", asMETHOD(Ogre::OverlayElement, setLeft), asCALL_THISCALL);
    engine->RegisterObjectMethod("OverlayElement", "void setTop(float)", asMETHOD(Ogre::OverlayElement, setTop), asCALL_THISCALL);
    engine->RegisterObjectMethod("OverlayElement", "void setWidth(float)", asMETHOD(Ogre::OverlayElement, setWidth), asCALL_THISCALL);
    engine->RegisterObjectMethod("OverlayElement", "void setHeight(float)", asMETHOD(Ogre::OverlayElement, setHeight), asCALL_THISCALL);
    // > material
    engine->RegisterObjectMethod("OverlayElement", "const string& getMaterialName() const", asMETHOD(Ogre::OverlayElement, getMaterialName), asCALL_THISCALL);
    engine->RegisterObjectMethod("OverlayElement", "void setMaterialName(const string&in, const string&in)", asMETHOD(Ogre::OverlayElement, setMaterialName), asCALL_THISCALL);
    // > caption
    engine->RegisterObjectMethod("OverlayElement", "void setCaption(const string&in)", asMETHOD(Ogre::OverlayElement, setCaption), asCALL_THISCALL);
    engine->RegisterObjectMethod("OverlayElement", "const string& getCaption() const", asMETHOD(Ogre::OverlayElement, getCaption), asCALL_THISCALL);
    // > color
    engine->RegisterObjectMethod("OverlayElement", "void setColour(const color&in)", asMETHOD(Ogre::OverlayElement, setColour), asCALL_THISCALL);
    engine->RegisterObjectMethod("OverlayElement", "const color& getColour() const", asMETHOD(Ogre::OverlayElement, getColour), asCALL_THISCALL);
    // > GuiMetricsMode
    engine->RegisterObjectMethod("OverlayElement", "GuiMetricsMode getMetricsMode() const", asMETHOD(Ogre::OverlayElement, getMetricsMode), asCALL_THISCALL);
    engine->RegisterObjectMethod("OverlayElement", "void setMetricsMode(GuiMetricsMode)", asMETHOD(Ogre::OverlayElement, setMetricsMode), asCALL_THISCALL);
    // > GuiHorizontalAlignment
    engine->RegisterObjectMethod("OverlayElement", "GuiHorizontalAlignment getHorizontalAlignment() const", asMETHOD(Ogre::OverlayElement, getHorizontalAlignment), asCALL_THISCALL);
    engine->RegisterObjectMethod("OverlayElement", "void setHorizontalAlignment(GuiHorizontalAlignment)", asMETHOD(Ogre::OverlayElement, setHorizontalAlignment), asCALL_THISCALL);


    // Register the Overlay class
    // (order roughly matches OgreOverlay.h)
    engine->RegisterObjectType("Overlay", 0, asOBJ_REF | asOBJ_NOCOUNT);
    engine->RegisterObjectMethod("Overlay", "const string& getName() const", asMETHOD(Ogre::Overlay, getName), asCALL_THISCALL);
    // > z-order
    engine->RegisterObjectMethod("Overlay", "void setZOrder(uint16)", asMETHOD(Ogre::Overlay, setZOrder), asCALL_THISCALL);
    engine->RegisterObjectMethod("Overlay", "uint16 getZOrder()", asMETHOD(Ogre::Overlay, getZOrder), asCALL_THISCALL);
    // > visibility
    engine->RegisterObjectMethod("Overlay", "bool isVisible() const", asMETHODPR(Ogre::Overlay, isVisible, () const, bool), asCALL_THISCALL);
    engine->RegisterObjectMethod("Overlay", "void show()", asMETHODPR(Ogre::Overlay, show, (), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("Overlay", "void hide()", asMETHODPR(Ogre::Overlay, hide, (), void), asCALL_THISCALL);
    // > 2D elements
    engine->RegisterObjectMethod("Overlay", "void add2D(OverlayElement@)", asFUNCTIONPR([](Ogre::Overlay* self, Ogre::OverlayElement* elem) {
        try { self->add2D(dynamic_cast<Ogre::OverlayContainer*>(elem)); }
        catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::Overlay::add2D()"); } }, (Ogre::Overlay* , Ogre::OverlayElement* ), void), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("Overlay", "void remove2D(OverlayElement@)", asFUNCTIONPR([](Ogre::Overlay* self, Ogre::OverlayElement* elem) {
        try { self->remove2D(dynamic_cast<Ogre::OverlayContainer*>(elem)); }
        catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::Overlay::remove2D()"); } }, (Ogre::Overlay* , Ogre::OverlayElement* ), void), asCALL_CDECL_OBJFIRST);
    // > scrolling
    engine->RegisterObjectMethod("Overlay", "void setScroll(float, float)", asMETHOD(Ogre::Overlay, setScroll), asCALL_THISCALL);
    engine->RegisterObjectMethod("Overlay", "float getScrollX() const", asMETHOD(Ogre::Overlay, getScrollX), asCALL_THISCALL);
    engine->RegisterObjectMethod("Overlay", "float getScrollY() const", asMETHOD(Ogre::Overlay, getScrollY), asCALL_THISCALL);
    engine->RegisterObjectMethod("Overlay", "void scroll(float, float)", asMETHOD(Ogre::Overlay, scroll), asCALL_THISCALL);
    // > rotating
    engine->RegisterObjectMethod("Overlay", "void setRotate(const radian&in)", asMETHOD(Ogre::Overlay, setRotate), asCALL_THISCALL);
    engine->RegisterObjectMethod("Overlay", "const radian& getRotate() const", asMETHOD(Ogre::Overlay, getRotate), asCALL_THISCALL);
    engine->RegisterObjectMethod("Overlay", "void rotate(const radian&in)", asMETHOD(Ogre::Overlay, rotate), asCALL_THISCALL);
    // > scaling
    engine->RegisterObjectMethod("Overlay", "void setScale(float, float)", asMETHOD(Ogre::Overlay, setScale), asCALL_THISCALL);
    engine->RegisterObjectMethod("Overlay", "float getScaleX() const", asMETHOD(Ogre::Overlay, getScaleX), asCALL_THISCALL);
    engine->RegisterObjectMethod("Overlay", "float getScaleY() const", asMETHOD(Ogre::Overlay, getScaleY), asCALL_THISCALL);
    // > 2D elements
    engine->RegisterObjectMethod("Overlay", "array<OverlayElement@>@ get2DElements()", asFUNCTION(get2DElementsHelper), asCALL_CDECL_OBJFIRST);


    // Register the OverlayManager class
    // (order roughly matches OgreOverlayManager.h)
    engine->RegisterObjectType("OverlayManager", 0, asOBJ_REF | asOBJ_NOCOUNT);
    // > overlay management
    engine->RegisterObjectMethod("OverlayManager", "Overlay@ create(const string&in)", asFUNCTIONPR([](Ogre::OverlayManager* self, const std::string& name) {
        try {return self->create(name);}
        catch(...) {App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::OverlayManager::create()"); return (Ogre::Overlay*)nullptr;}}, (Ogre::OverlayManager*, const std::string&), Ogre::Overlay*), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("OverlayManager", "Overlay@ getByName(const string&in)", asFUNCTIONPR([](Ogre::OverlayManager* self, const std::string& name) {
        try {return self->getByName(name);} // Doesn't seem to throw, but just to be sure...
        catch(...) {App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::OverlayManager::getByName()"); return (Ogre::Overlay*)nullptr;}}, (Ogre::OverlayManager*, const std::string&), Ogre::Overlay*), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("OverlayManager", "void destroy(const string&in)", asFUNCTIONPR([](Ogre::OverlayManager* self, const std::string& name) {
        try {return self->destroy(name);}
        catch(...) {App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::OverlayManager::destroy(string)"); }}, (Ogre::OverlayManager*, const std::string&), void), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("OverlayManager", "void destroy(Overlay@)", asFUNCTIONPR([](Ogre::OverlayManager* self, Ogre::Overlay* ov) {
        try {return self->destroy(ov);}
        catch(...) {App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::OverlayManager::destroy(Overlay@)"); }}, (Ogre::OverlayManager*, Ogre::Overlay*), void), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("OverlayManager", "void destroyAll()", asFUNCTIONPR([](Ogre::OverlayManager* self) {
        try {return self->destroyAll();}
        catch(...) {App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::OverlayManager::destroyAll()"); }}, (Ogre::OverlayManager*), void), asCALL_CDECL_OBJFIRST);
    //    NOTE: we have `getOverlays()` instead of `getOverlayIterator()`
    engine->RegisterObjectMethod("OverlayManager", "array<Overlay@>@ getOverlays()", asFUNCTIONPR([](Ogre::OverlayManager* self) {
        try {auto iterable = self->getOverlayIterator();
            return IterableMapToScriptArray(iterable.begin(), iterable.end(), "Ogre::Overlay@"); }
        catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::OverlayManager::getOverlays()"); return (CScriptArray*)nullptr; } }, (Ogre::OverlayManager*), CScriptArray*), asCALL_CDECL_OBJFIRST);
    // > viewport info
    engine->RegisterObjectMethod("OverlayManager", "float getViewportHeight() const", asMETHOD(Ogre::OverlayManager, getViewportHeight), asCALL_THISCALL);
    engine->RegisterObjectMethod("OverlayManager", "float getViewportWidth() const", asMETHOD(Ogre::OverlayManager, getViewportWidth), asCALL_THISCALL);
    // > overlay element management
    engine->RegisterObjectMethod("OverlayManager", "OverlayElement@ createOverlayElement(const string&in, const string&in, bool=false)", asFUNCTIONPR([](Ogre::OverlayManager* self, const std::string& type, const std::string& name, bool isTemplate) {
        try {return dynamic_cast<Ogre::OverlayElement*>(self->createOverlayElement(type,name,isTemplate));}
        catch(...) {App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::OverlayManager::createOverlayElement()"); return (Ogre::OverlayElement*)nullptr;}}, (Ogre::OverlayManager*, const std::string&, const std::string&, bool), Ogre::OverlayElement*), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("OverlayManager", "OverlayElement@ getOverlayElement(const string&in) const", asFUNCTIONPR([](Ogre::OverlayManager* self, const std::string& name) {
        try {return dynamic_cast<Ogre::OverlayElement*>(self->getOverlayElement(name));}
        catch(...) {App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::OverlayManager::getOverlayElement()"); return (Ogre::OverlayElement*)nullptr;}}, (Ogre::OverlayManager*, const std::string&), Ogre::OverlayElement*), asCALL_CDECL_OBJFIRST);    
    engine->RegisterObjectMethod("OverlayManager", "bool hasOverlayElement(const string&in) const", asMETHOD(Ogre::OverlayManager, hasOverlayElement), asCALL_THISCALL);
    engine->RegisterObjectMethod("OverlayManager", "void destroyOverlayElement(const string&in, bool isTemplate=false) const", asFUNCTIONPR([](Ogre::OverlayManager* self, const std::string& name, bool isTemplate) {
        try { self->destroyOverlayElement(name, isTemplate);}
        catch(...) {App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::OverlayManager::destroyOverlayElement(string)"); }}, (Ogre::OverlayManager*, const std::string&, bool), void), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("OverlayManager", "void destroyOverlayElement(OverlayElement@, bool isTemplate=false) const", asFUNCTIONPR([](Ogre::OverlayManager* self, Ogre::OverlayElement* oe, bool isTemplate) {
        try { self->destroyOverlayElement(oe, isTemplate);}
        catch(...) {App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::OverlayManager::destroyOverlayElement(OverlayElement@)"); }}, (Ogre::OverlayManager*, Ogre::OverlayElement*, bool), void), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("OverlayManager", "void destroyAllOverlayElements(bool isTemplate=false) const", asFUNCTIONPR([](Ogre::OverlayManager* self, bool isTemplate) {
        try { self->destroyAllOverlayElements(isTemplate);}
        catch(...) {App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::OverlayManager::destroyAllOverlayElements()"); }}, (Ogre::OverlayManager*, bool), void), asCALL_CDECL_OBJFIRST);
    // > template management
    engine->RegisterObjectMethod("OverlayManager", "OverlayElement@ createOverlayElementFromTemplate(const string&in, const string&in, const string&in, bool=false)", asFUNCTIONPR([](Ogre::OverlayManager* self,  const std::string& templateName, const std::string& typeName, const std::string& instanceName, bool isTemplate) {
        try {return dynamic_cast<Ogre::OverlayElement*>(self->createOverlayElementFromTemplate(templateName, typeName, instanceName, isTemplate));}
        catch(...) {App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::OverlayManager::createOverlayElementFromTemplate()"); return (Ogre::OverlayElement*)nullptr;}}, (Ogre::OverlayManager*, const std::string&, const std::string&, const std::string&, bool), Ogre::OverlayElement*), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("OverlayManager", "OverlayElement@ cloneOverlayElementFromTemplate(const string&in, const string&in)", asFUNCTIONPR([](Ogre::OverlayManager* self,  const std::string& templateName, const std::string& instanceName) {
        try {return dynamic_cast<Ogre::OverlayElement*>(self->cloneOverlayElementFromTemplate(templateName, instanceName));}
        catch(...) {App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::OverlayManager::cloneOverlayElementFromTemplate()"); return (Ogre::OverlayElement*)nullptr;}}, (Ogre::OverlayManager*, const std::string&, const std::string&), Ogre::OverlayElement*), asCALL_CDECL_OBJFIRST);
    //    NOTE: we have `getTemplates()` instead of `getTemplateIterator()`
    engine->RegisterObjectMethod("OverlayManager", "array<OverlayElement@>@ getTemplates()", asFUNCTION(getElementTemplatesHelper), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("OverlayManager", "bool isTemplate(const string&in)", asFUNCTIONPR([](Ogre::OverlayManager* self, const std::string& name) {
        try {return self->isTemplate(name);}
        catch(...) {App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::OverlayManager::isTemplate()"); return false;}}, (Ogre::OverlayManager*, const std::string&), bool), asCALL_CDECL_OBJFIRST);


    engine->SetDefaultNamespace("Ogre::OverlayManager");
    engine->RegisterGlobalFunction("OverlayManager& getSingleton()", asFUNCTION(OverlayManager::getSingleton), asCALL_CDECL);

    engine->SetDefaultNamespace("");

}

void registerOgreManualObject(AngelScript::asIScriptEngine* engine)
{
    engine->SetDefaultNamespace("Ogre");

    // Register the ManualObject class
    engine->RegisterObjectMethod("ManualObject", "void begin(const string&in, RenderOperation, const string&in)", asMETHODPR(Ogre::ManualObject, begin, (const String&, Ogre::RenderOperation::OperationType, const String&), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("ManualObject", "void beginUpdate()", asMETHOD(Ogre::ManualObject, beginUpdate), asCALL_THISCALL);
    engine->RegisterObjectMethod("ManualObject", "void position(const vector3&in)", asMETHODPR(Ogre::ManualObject, position, (const Ogre::Vector3&), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("ManualObject", "void normal(const vector3&in)", asMETHODPR(Ogre::ManualObject, normal, (const Ogre::Vector3&), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("ManualObject", "void textureCoord(float, float)", asMETHODPR(Ogre::ManualObject, textureCoord, (float, float), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("ManualObject", "void textureCoord(const vector2&in)", asMETHODPR(Ogre::ManualObject, textureCoord, (const Ogre::Vector2&), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("ManualObject", "void textureCoord(const vector3&in)", asMETHODPR(Ogre::ManualObject, textureCoord, (const Ogre::Vector3&), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("ManualObject", "void colour(const color&in)", asMETHODPR(Ogre::ManualObject, colour, (const Ogre::ColourValue&), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("ManualObject", "void index(uint32)", asMETHOD(Ogre::ManualObject, index), asCALL_THISCALL);
    engine->RegisterObjectMethod("ManualObject", "void end()", asMETHOD(Ogre::ManualObject, end), asCALL_THISCALL);

    engine->RegisterObjectMethod("ManualObject", "uint getCurrentVertexCount()", asMETHOD(Ogre::ManualObject, getCurrentVertexCount), asCALL_THISCALL);
    engine->RegisterObjectMethod("ManualObject", "void getCurrentIndexCount()", asMETHOD(Ogre::ManualObject, getCurrentIndexCount), asCALL_THISCALL);

    engine->RegisterObjectMethod("ManualObject", "MeshPtr convertToMesh(const string&in name, const string&in group = 'General')", asMETHOD(Ogre::ManualObject, convertToMesh), asCALL_THISCALL);
    
    engine->SetDefaultNamespace("");
}

void registerOgreHardwarePixelBuffer(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectBehaviour("HardwarePixelBufferPtr", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(HardwarePixelBufferPtrDefaultConstructor), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("HardwarePixelBufferPtr", asBEHAVE_CONSTRUCT, "void f(const HardwarePixelBufferPtr&in)", asFUNCTION(HardwarePixelBufferPtrCopyConstructor), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("HardwarePixelBufferPtr", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(HardwarePixelBufferPtrDestructor), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("HardwarePixelBufferPtr", "HardwarePixelBufferPtr& opAssign(const HardwarePixelBufferPtr&in)", asFUNCTION(HardwarePixelBufferPtrAssignOperator), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    
    r = engine->RegisterObjectMethod("HardwarePixelBufferPtr", "const PixelBox& getCurrentLock()", asFUNCTION(HardwarePixelBufferPtrGetCurrentLock), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("HardwarePixelBufferPtr", "const PixelBox& lock(const box& lockbox, HardwareBufferLockOptions opt)", asFUNCTION(HardwarePixelBufferPtrLock), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);   

    r = engine->RegisterObjectMethod("HardwarePixelBufferPtr", "uint getWidth()", asFUNCTION(HardwarePixelBufferPtrGetWidth), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("HardwarePixelBufferPtr", "uint getHeight()", asFUNCTION(HardwarePixelBufferPtrGetHeight), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("HardwarePixelBufferPtr", "void blitFromMemory(const PixelBox& src, const box& dst)", asFUNCTION(HardwarePixelBufferPtrBlitFromMemory), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("HardwarePixelBufferPtr", "void blitToMemory(const box& src, const PixelBox& dst)", asFUNCTION(HardwarePixelBufferPtrBlitToMemory), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("HardwarePixelBufferPtr", "void unlock()", asFUNCTION(HardwarePixelBufferPtrUnlock), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    engine->SetDefaultNamespace("");
}

void registerOgrePixelBox(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectBehaviour("PixelBox", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(PixelBoxDefaultConstructor), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("PixelBox", asBEHAVE_CONSTRUCT, "void f(const PixelBox&in)", asFUNCTION(PixelBoxCopyConstructor), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("PixelBox", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(PixelBoxDestructor), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("PixelBox", "PixelBox& opAssign(const PixelBox&in)", asFUNCTION(PixelBoxAssignOperator), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("PixelBox", "color getColourAt(uint32 x, uint32 y, uint32 z)", asMETHOD(PixelBox, getColourAt), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("PixelBox", "void setColourAt(const color& c, uint32 x, uint32 y, uint32 z)", asMETHOD(PixelBox, setColourAt), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    // Inherited methods must have wrappers - see AngelScript doc
    r = engine->RegisterObjectMethod("PixelBox", "uint getWidth()",  asFUNCTION(PixelBoxGetWidth), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("PixelBox", "uint getHeight()", asFUNCTION(PixelBoxGetHeight), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("PixelBox", "uint getDepth()",  asFUNCTION(PixelBoxGetDepth), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

void registerOgreImage(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectBehaviour("Image", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ImageDefaultConstructor), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("Image", asBEHAVE_CONSTRUCT, "void f(const Image&in)", asFUNCTION(ImageCopyConstructor), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("Image", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(ImageDestructor), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Image", "Image& opAssign(const Image&in)", asFUNCTION(ImageAssignOperator), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("Image", "color getColourAt(uint32 x, uint32 y, uint32 z)", asMETHOD(Image, getColourAt), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Image", "void setColourAt(const color& c, uint32 x, uint32 y, uint32 z)", asMETHOD(Image, setColourAt), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Image", "Image& flipAroundX()", asMETHOD(Ogre::Image, flipAroundX), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Image", "Image& flipAroundY()", asMETHOD(Ogre::Image, flipAroundY), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Image", "uint getNumMipmaps()", asMETHOD(Ogre::Image, getNumMipmaps), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Image", "uint getNumFaces()", asMETHOD(Ogre::Image, getNumFaces), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Image", "PixelBox getPixelBox(uint face, uint mipmap)", asMETHOD(Ogre::Image, getPixelBox), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Image", "uint getSize()", asMETHOD(Ogre::Image, getSize), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Image", "uint getWidth()", asMETHOD(Ogre::Image, getWidth), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Image", "uint getHeight()", asMETHOD(Ogre::Image, getHeight), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Image", "void resize(uint16 width, uint16 height, ImageFilter filter)", asMETHOD(Ogre::Image, resize), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

void registerOgreSubMesh(AngelScript::asIScriptEngine* engine)
{
    engine->SetDefaultNamespace("Ogre");

    // Register the SubMesh class
    engine->RegisterObjectMethod("SubMesh", "const string& getMaterialName()", asMETHOD(Ogre::SubMesh, getMaterialName), asCALL_THISCALL);
    engine->RegisterObjectMethod("SubMesh", "void setMaterialName(const string&in, const string&in)", asMETHOD(Ogre::SubMesh, setMaterialName), asCALL_THISCALL);

    // > Vertex buffer
    engine->RegisterObjectMethod("SubMesh", "array<vector3>@ __getVertexPositions()", asFUNCTION(SubMesh__getVertexPositions), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("SubMesh", "array<vector2>@ __getVertexTexcoords(uint index)", asFUNCTION(SubMesh__getVertexTexcoords), asCALL_CDECL_OBJFIRST);

    // > Index buffer
    engine->RegisterObjectMethod("SubMesh", "array<uint16>@ __getIndexBuffer16bit()", asFUNCTION(SubMesh__getIndexBuffer16bit), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("SubMesh", "array<uint>@ __getIndexBuffer32bit()", asFUNCTION(SubMesh__getIndexBuffer32bit), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("SubMesh", "IndexType __getIndexType()", asFUNCTION(SubMesh__getIndexType), asCALL_CDECL_OBJFIRST);
    
    engine->SetDefaultNamespace("");
}

void registerOgreMesh(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectBehaviour("MeshPtr", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(MeshPtrDefaultConstructor), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("MeshPtr", asBEHAVE_CONSTRUCT, "void f(const MeshPtr&in)", asFUNCTION(MeshPtrCopyConstructor), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("MeshPtr", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(MeshPtrDestructor), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("MeshPtr", "MeshPtr& opAssign(const MeshPtr&in)", asFUNCTION(MeshPtrAssignOperator), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("MeshPtr", "bool isNull()", asFUNCTION(MeshPtrIsNull), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);

    // Wrappers are inevitable, see https://www.gamedev.net/forums/topic/540419-custom-smartpointers-and-angelscript-/
    r = engine->RegisterObjectMethod("MeshPtr", "SubMeshArray@ getSubMeshes()", asFUNCTION(MeshPtrGetSubmeshes), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("MeshPtr", "string getName()", asFUNCTION(MeshPtrGetName), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("MeshPtr", "SubMesh@ createSubMesh(const string&in name)", asFUNCTION(MeshPtrCreateSubMesh), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("MeshPtr", "void destroySubMesh(const string&in name)", asFUNCTION(MeshPtrDestroySubMesh), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

void registerOgreMeshManager(AngelScript::asIScriptEngine * engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("MeshManager", "MeshPtr load(const string&in file, const string&in rg)", asFUNCTION(MeshManagerLoad), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("MeshManager", "void remove(const string&in file, const string&in rg)", asFUNCTION(MeshManagerRemove), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace("Ogre::MeshManager"); ROR_ASSERT(r >= 0);
    r = engine->RegisterGlobalFunction("MeshManager& getSingleton()", asFUNCTION(MeshManager::getSingleton), asCALL_CDECL); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

void registerOgreMaterialManager(AngelScript::asIScriptEngine * engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("MaterialManager", "MaterialPtr getByName(const string&in file, const string&in rg)", asFUNCTION(MaterialManagerGetByName), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("MaterialManager", "MaterialPtr create(const string&in file, const string&in rg)", asFUNCTION(MaterialManagerCreate), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace("Ogre::MaterialManager"); ROR_ASSERT(r >= 0);
    r = engine->RegisterGlobalFunction("MaterialManager& getSingleton()", asFUNCTION(MaterialManager::getSingleton), asCALL_CDECL); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

void registerOgreMaterial(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectBehaviour("MaterialPtr", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(MaterialPtrDefaultConstructor), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("MaterialPtr", asBEHAVE_CONSTRUCT, "void f(const MaterialPtr&in)", asFUNCTION(MaterialPtrCopyConstructor), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("MaterialPtr", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(MaterialPtrDestructor), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("MaterialPtr", "MaterialPtr& opAssign(const MaterialPtr&in)", asFUNCTION(MaterialPtrAssignOperator), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("MaterialPtr", "bool isNull()", asFUNCTION(MaterialPtrIsNull), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);

    // Wrappers are inevitable, see https://www.gamedev.net/forums/topic/540419-custom-smartpointers-and-angelscript-/
    r = engine->RegisterObjectMethod("MaterialPtr", "TechniqueArray@ getTechniques()", asFUNCTION(MaterialPtrGetTechniques), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);
    
    r = engine->RegisterObjectMethod("MaterialPtr", "string getName()", asFUNCTION(MaterialPtrGetName), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("MaterialPtr", "Technique@ createTechnique()", asFUNCTION(MaterialPtrCreateTechnique), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("MaterialPtr", "void removeTechnique(uint16 index)", asFUNCTION(MaterialPtrRemoveTechnique), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

void registerOgreTechnique(AngelScript::asIScriptEngine* engine)
{
    engine->SetDefaultNamespace("Ogre");

    engine->RegisterObjectMethod("Technique", "PassArray @getPasses()", asFUNCTION(TechniqueGetPasses), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("Technique", "Pass @createPass()", asMETHOD(Ogre::Technique, createPass), asCALL_THISCALL);
    engine->RegisterObjectMethod("Technique", "void removePass(uint16 index)", asMETHOD(Ogre::Technique, removePass), asCALL_THISCALL);
    engine->RegisterObjectMethod("Technique", "const string& getName() const", asMETHOD(Ogre::Technique, getName), asCALL_THISCALL);

    engine->SetDefaultNamespace("");
}

void registerOgrePass(AngelScript::asIScriptEngine* engine)
{
    int r = 0;
    engine->SetDefaultNamespace("Ogre");

    engine->RegisterObjectMethod("Pass", "const string& getName() const", asMETHOD(Ogre::Pass, getName), asCALL_THISCALL);
    engine->RegisterObjectMethod("Pass", "TextureUnitStateArray @getTextureUnitStates()", asFUNCTION(PassGetTextureUnitStates), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("Pass", "void removeTextureUnitState(uint16 index)", asMETHOD(Ogre::Pass, removeTextureUnitState), asCALL_THISCALL);
    
    r = engine->RegisterObjectMethod("Pass", "GpuProgramParametersPtr getVertexProgramParameters()", asFUNCTION(PassGetVertexProgramParameters), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("Pass", "GpuProgramParametersPtr getFragmentProgramParameters()", asFUNCTION(PassGetFragmentProgramParameters), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("Pass", "GpuProgramParametersPtr getGeometryProgramParameters()", asFUNCTION(PassGetGeometryProgramParameters), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("Pass", "GpuProgramParametersPtr getTessellationHullProgramParameters()", asFUNCTION(PassGetTessellationHullProgramParameters), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("Pass", "GpuProgramParametersPtr getTessellationDomainProgramParameters()", asFUNCTION(PassGetTessellationDomainProgramParameters), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("Pass", "GpuProgramParametersPtr getComputeProgramParameters()", asFUNCTION(PassGetComputeProgramParameters), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    engine->RegisterObjectMethod("Pass", "void setVertexProgramParameters(GpuProgramParametersPtr)", asMETHOD(Ogre::Pass, setVertexProgramParameters), asCALL_THISCALL);
    engine->RegisterObjectMethod("Pass", "void setFragmentProgramParameters(GpuProgramParametersPtr)", asMETHOD(Ogre::Pass, setFragmentProgramParameters), asCALL_THISCALL);
    engine->RegisterObjectMethod("Pass", "void setGeometryProgramParameters(GpuProgramParametersPtr)", asMETHOD(Ogre::Pass, setGeometryProgramParameters), asCALL_THISCALL);
    engine->RegisterObjectMethod("Pass", "void setTessellationHullProgramParameters(GpuProgramParametersPtr)", asMETHOD(Ogre::Pass, setTessellationHullProgramParameters), asCALL_THISCALL);
    engine->RegisterObjectMethod("Pass", "void setTessellationDomainProgramParameters(GpuProgramParametersPtr)", asMETHOD(Ogre::Pass, setTessellationDomainProgramParameters), asCALL_THISCALL);
    engine->RegisterObjectMethod("Pass", "void setComputeProgramParameters(GpuProgramParametersPtr)", asMETHOD(Ogre::Pass, setComputeProgramParameters), asCALL_THISCALL);

    engine->SetDefaultNamespace("");
}

void registerOgreTextureUnitState(AngelScript::asIScriptEngine* engine)
{
    engine->SetDefaultNamespace("Ogre");

    engine->RegisterObjectMethod("TextureUnitState", "const string& getName() const", asMETHOD(Ogre::TextureUnitState, getName), asCALL_THISCALL);
    engine->RegisterObjectMethod("TextureUnitState", "void setTexture(const TexturePtr&in)", asMETHODPR(Ogre::TextureUnitState, setTexture, (const TexturePtr&), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("TextureUnitState", "const TexturePtr& _getTexturePtr() const", asMETHODPR(Ogre::TextureUnitState, _getTexturePtr, (void) const, const TexturePtr&), asCALL_THISCALL);

    engine->SetDefaultNamespace("");
}

void registerOgreTimer(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectBehaviour("Timer", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(TimerDefaultConstructor), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("Timer", asBEHAVE_CONSTRUCT, "void f(const Timer&in)", asFUNCTION(TimerCopyConstructor), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("Timer", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(TimerDefaultDestructor), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Timer", "Timer& opAssign(const Timer&in)", asFUNCTION(TimerAssignOperator), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("Timer", "void reset()", asMETHOD(Timer, reset), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Timer", "uint getMilliseconds()", asMETHOD(Timer, getMilliseconds), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Timer", "uint getMicroseconds()", asMETHOD(Timer, getMicroseconds), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Timer", "uint getMillisecondsCPU()", asMETHOD(Timer, getMillisecondsCPU), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Timer", "uint getMicrosecondsCPU()", asMETHOD(Timer, getMicrosecondsCPU), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

void registerOgreGpuProgramParameters(AngelScript::asIScriptEngine* engine)
{
    int r;
    
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    // Note: The `*SharedPtr` is a deprecated alias of `*Ptr` in OGRE 14, but it's not yet present in the version we use.
    r = engine->RegisterObjectBehaviour("GpuProgramParametersPtr", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(GpuProgramParametersPtrDefaultConstructor), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("GpuProgramParametersPtr", asBEHAVE_CONSTRUCT, "void f(const GpuProgramParametersPtr&in)", asFUNCTION(GpuProgramParametersPtrCopyConstructor), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("GpuProgramParametersPtr", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(GpuProgramParametersPtrDestructor), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "GpuProgramParametersPtr& opAssign(const GpuProgramParametersPtr&in)", asFUNCTION(GpuProgramParametersPtrAssignOperator), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "bool isNull()", asFUNCTION(GpuProgramParametersPtrIsNull), asCALL_CDECL_OBJLAST); ROR_ASSERT(r >= 0);

    // Wrappers are inevitable, see https://www.gamedev.net/forums/topic/540419-custom-smartpointers-and-angelscript-/

    // > setConstant (scalar)
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setConstant(uint index, float val)", asFUNCTION(GpuProgramParametersPtrSetConstantFloat), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setConstant(uint index, const vector3& val)", asFUNCTION(GpuProgramParametersPtrSetConstantVector3), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setConstant(uint index, const vector2& val)", asFUNCTION(GpuProgramParametersPtrSetConstantVector2), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setConstant(uint index, const color& val)", asFUNCTION(GpuProgramParametersPtrSetConstantColourValue), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    // > setConstant (vector)
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setConstant(uint index, const array<float>@ vals)", asFUNCTION(GpuProgramParametersPtrSetConstantFloatArray), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setConstant(uint index, const array<vector3>@ vals)", asFUNCTION(GpuProgramParametersPtrSetConstantVector3Array), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setConstant(uint index, const array<vector2>@ vals)", asFUNCTION(GpuProgramParametersPtrSetConstantVector2Array), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setConstant(uint index, const array<color>@ vals)", asFUNCTION(GpuProgramParametersPtrSetConstantColourValueArray), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    // > setNamedConstant (scalar)
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setNamedConstant(const string&in name, float val)", asFUNCTION(GpuProgramParametersPtrSetNamedConstantFloat), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setNamedConstant(const string&in name, const vector3& val)", asFUNCTION(GpuProgramParametersPtrSetNamedConstantVector3), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setNamedConstant(const string&in name, const vector2& val)", asFUNCTION(GpuProgramParametersPtrSetNamedConstantVector2), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setNamedConstant(const string&in name, const color& val)", asFUNCTION(GpuProgramParametersPtrSetNamedConstantColourValue), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    // > setNamedConstant (vector)
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setNamedConstant(const string&in name, const array<float>@ vals)", asFUNCTION(GpuProgramParametersPtrSetNamedConstantFloatArray), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setNamedConstant(const string&in name, const array<vector3>@ vals)", asFUNCTION(GpuProgramParametersPtrSetNamedConstantVector3Array), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setNamedConstant(const string&in name, const array<vector2>@ vals)", asFUNCTION(GpuProgramParametersPtrSetNamedConstantVector2Array), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setNamedConstant(const string&in name, const array<color>@ vals)", asFUNCTION(GpuProgramParametersPtrSetNamedConstantColourValueArray), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "array<string>@ __getNamedConstants()", asFUNCTION(GpuProgramParametersPtr__getNamedConstants), asCALL_CDECL_OBJFIRST); ROR_ASSERT(r >= 0);


    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

void registerOgreLight(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    // Type control
    r = engine->RegisterObjectMethod("Light", "void setType(LightTypes type)", asMETHOD(Ogre::Light, setType), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Light", "LightTypes getType() const", asMETHOD(Ogre::Light, getType), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    // Color
    r = engine->RegisterObjectMethod("Light", "void setDiffuseColour(float r, float g, float b)", asMETHODPR(Ogre::Light, setDiffuseColour, (float, float, float), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Light", "void setDiffuseColour(const color&in)", asMETHODPR(Ogre::Light, setDiffuseColour, (const Ogre::ColourValue&), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Light", "const color& getDiffuseColour() const", asMETHOD(Ogre::Light, getDiffuseColour), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("Light", "void setSpecularColour(float r, float g, float b)", asMETHODPR(Ogre::Light, setSpecularColour, (float, float, float), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Light", "void setSpecularColour(const color&in)", asMETHODPR(Ogre::Light, setSpecularColour, (const Ogre::ColourValue&), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Light", "const color& getSpecularColour() const", asMETHOD(Ogre::Light, getSpecularColour), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    // Attenuation
    r = engine->RegisterObjectMethod("Light", "void setAttenuation(float range, float constant, float linear, float quadratic)", asMETHOD(Ogre::Light, setAttenuation), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Light", "float getAttenuationRange() const", asMETHOD(Ogre::Light, getAttenuationRange), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Light", "float getAttenuationConstant() const", asMETHOD(Ogre::Light, getAttenuationConstant), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Light", "float getAttenuationLinear() const", asMETHOD(Ogre::Light, getAttenuationLinear), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Light", "float getAttenuationQuadric() const", asMETHOD(Ogre::Light, getAttenuationQuadric), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    // Position & Direction
    r = engine->RegisterObjectMethod("Light", "void setPosition(float x, float y, float z)", asMETHODPR(Ogre::Light, setPosition, (float, float, float), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Light", "void setPosition(const vector3&in)", asMETHODPR(Ogre::Light, setPosition, (const Ogre::Vector3&), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Light", "const vector3& getPosition() const", asMETHOD(Ogre::Light, getPosition), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("Light", "void setDirection(float x, float y, float z)", asMETHODPR(Ogre::Light, setDirection, (float, float, float), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Light", "void setDirection(const vector3&in)", asMETHODPR(Ogre::Light, setDirection, (const Ogre::Vector3&), void), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Light", "const vector3& getDirection() const", asMETHOD(Ogre::Light, getDirection), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    // Spotlight parameters
    r = engine->RegisterObjectMethod("Light", "void setSpotlightRange(const radian&in innerAngle, const radian&in outerAngle, float falloff = 1.0f)", asMETHOD(Ogre::Light, setSpotlightRange), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Light", "const radian& getSpotlightInnerAngle() const", asMETHOD(Ogre::Light, getSpotlightInnerAngle), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Light", "const radian& getSpotlightOuterAngle() const", asMETHOD(Ogre::Light, getSpotlightOuterAngle), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Light", "float getSpotlightFalloff() const", asMETHOD(Ogre::Light, getSpotlightFalloff), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    // Power scale
    r = engine->RegisterObjectMethod("Light", "void setPowerScale(float power)", asMETHOD(Ogre::Light, setPowerScale), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Light", "float getPowerScale() const", asMETHOD(Ogre::Light, getPowerScale), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    // Derived direction (useful for attached lights)
    r = engine->RegisterObjectMethod("Light", "const vector3& getDerivedPosition() const", asMETHOD(Ogre::Light, getDerivedPosition), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Light", "const vector3& getDerivedDirection() const", asMETHOD(Ogre::Light, getDerivedDirection), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    // Inherit MovableObject methods
    registerOgreMovableObjectBase<Ogre::Light>(engine, "Light");

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}
