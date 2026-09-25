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

#include "../../autowrapper/aswrappedcall.h"
#include "AngelScriptBindings.h"
#include "Application.h"
#include "ScriptEngine.h"
#include "ScriptUtils.h"
#include "wrappers/OgreAngelscriptWrappers.h"

#include <angelscript.h>

#include <OgreRenderOperation.h>

#include <Overlay/OgreOverlaySystem.h>
#include <Overlay/OgreOverlayManager.h>
#include <Overlay/OgreOverlay.h>
#include <Overlay/OgreOverlayContainer.h>

using namespace Ogre;
using namespace AngelScript;
using namespace RoR;
using namespace OgreAngelscriptWrappers;

// Generic-only helpers:
// The autowrapper reads each argument as its exact C++ type, so C++ `size_t` params
// can't be bound directly to 32-bit script params (`int`/`uint`) and C++ `const T&`
// return values can't be bound to by-value script returns. These adapters convert.

static float Vector3OpIndex(const Vector3& self, int i)
{
    return self[i];
}

static Vector3 Vector3OpAddUnary(const Vector3& self)
{
    return +self;
}

static float Vector2OpIndex(const Vector2& self, int i)
{
    return self[i];
}

static Vector2 Vector2OpAddUnary(const Vector2& self)
{
    return +self;
}

static float QuaternionOpIndex(const Quaternion& self, int i)
{
    return self[i];
}

static Radian& RadianAssignFloat(Radian& self, float f)
{
    return self = f;
}

static Radian RadianOpAddUnary(const Radian& self)
{
    return +self;
}

static Degree& DegreeAssignFloat(Degree& self, float f)
{
    return self = f;
}

static Degree DegreeOpAddUnary(const Degree& self)
{
    return +self;
}

static ColourValue PixelBoxGetColourAt(const PixelBox& self, asUINT x, asUINT y, asUINT z)
{
    return self.getColourAt(x, y, z);
}

static void PixelBoxSetColourAt(PixelBox& self, const ColourValue& c, asUINT x, asUINT y, asUINT z)
{
    self.setColourAt(c, x, y, z);
}

static ColourValue ImageGetColourAt(const Image& self, asUINT x, asUINT y, asUINT z)
{
    return self.getColourAt(x, y, z);
}

static void ImageSetColourAt(Image& self, const ColourValue& c, asUINT x, asUINT y, asUINT z)
{
    self.setColourAt(c, x, y, z);
}

static PixelBox ImageGetPixelBox(const Image& self, asUINT face, asUINT mipmap)
{
    return self.getPixelBox(face, mipmap);
}

static void AnimationStateCreateBlendMask(AnimationState* self, asUINT blendMaskSizeHint, float initialWeight)
{
    self->createBlendMask(blendMaskSizeHint, initialWeight);
}

static void AnimationStateSetBlendMaskEntry(AnimationState* self, asUINT boneHandle, float weight)
{
    self->setBlendMaskEntry(boneHandle, weight);
}

static float AnimationStateGetBlendMaskEntry(AnimationState* self, asUINT boneHandle)
{
    return self->getBlendMaskEntry(boneHandle);
}

// forward declarations, defined below
static void registerOgreVector3Generic(AngelScript::asIScriptEngine* engine);
static void registerOgreVector2Generic(AngelScript::asIScriptEngine* engine);
static void registerOgreRadianGeneric(AngelScript::asIScriptEngine* engine);
static void registerOgreDegreeGeneric(AngelScript::asIScriptEngine* engine);
static void registerOgreQuaternionGeneric(AngelScript::asIScriptEngine* engine);
static void registerOgreColourValueGeneric(AngelScript::asIScriptEngine* engine);
static void registerOgreBoxGeneric(AngelScript::asIScriptEngine* engine);

static void registerOgreMovableObjectGeneric(AngelScript::asIScriptEngine* engine);
static void registerOgreAnimationStateGeneric(AngelScript::asIScriptEngine* engine);
static void registerOgreAnimationStateSetGeneric(AngelScript::asIScriptEngine* engine);
static void registerOgreTextureGeneric(AngelScript::asIScriptEngine* engine);
static void registerOgreTextureManagerGeneric(AngelScript::asIScriptEngine* engine);
static void registerOgreHardwarePixelBufferGeneric(AngelScript::asIScriptEngine* engine);
static void registerOgrePixelBoxGeneric(AngelScript::asIScriptEngine* engine);
static void registerOgreImageGeneric(AngelScript::asIScriptEngine* engine);
static void registerOgreMeshManagerGeneric(AngelScript::asIScriptEngine* engine);
static void registerOgreMeshGeneric(AngelScript::asIScriptEngine* engine);
static void registerOgreSubMeshGeneric(AngelScript::asIScriptEngine* engine);
static void registerOgreMaterialManagerGeneric(AngelScript::asIScriptEngine* engine);
static void registerOgreMaterialGeneric(AngelScript::asIScriptEngine* engine);
static void registerOgreTechniqueGeneric(AngelScript::asIScriptEngine* engine);
static void registerOgrePassGeneric(AngelScript::asIScriptEngine* engine);
static void registerOgreTextureUnitStateGeneric(AngelScript::asIScriptEngine* engine);
static void registerOgreTimerGeneric(AngelScript::asIScriptEngine* engine);
static void registerOgreGpuProgramParametersGeneric(AngelScript::asIScriptEngine* engine);
// main registration method
void RoR::RegisterOgreObjectsGeneric(AngelScript::asIScriptEngine* engine)
{
    int r;

    // NOTE: data types, enums and object properties are registered in RegisterOgreObjectsCommon()

    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    // dictionary/array view types, also under namespace `Ogre`

    AnimationStateDict::RegisterReadonlyScriptDictViewGeneric(engine, "AnimationStateDict", "AnimationState");
    SubMeshArray::RegisterReadonlyScriptArrayViewGeneric(engine, "SubMeshArray", "SubMesh");
    TechniqueArray::RegisterReadonlyScriptArrayViewGeneric(engine, "TechniqueArray", "Technique");
    PassArray::RegisterReadonlyScriptArrayViewGeneric(engine, "PassArray", "Pass");
    TextureUnitStateArray::RegisterReadonlyScriptArrayViewGeneric(engine, "TextureUnitStateArray", "TextureUnitState");

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);

    // Now we register the object methods

    registerOgreRadianGeneric(engine);
    registerOgreDegreeGeneric(engine);
    registerOgreVector3Generic(engine);
    registerOgreVector2Generic(engine);
    registerOgreQuaternionGeneric(engine);
    registerOgreColourValueGeneric(engine);
    registerOgreBoxGeneric(engine);
    registerOgreMovableObjectGeneric(engine);
    registerOgreAnimationStateGeneric(engine);
    registerOgreAnimationStateSetGeneric(engine);
    registerOgreTextureGeneric(engine);
    registerOgreTextureManagerGeneric(engine);
    registerOgreHardwarePixelBufferGeneric(engine);
    registerOgrePixelBoxGeneric(engine);
    registerOgreImageGeneric(engine);
    registerOgreMeshGeneric(engine);
    registerOgreSubMeshGeneric(engine);
    registerOgreMeshManagerGeneric(engine);
    registerOgreMaterialGeneric(engine);
    registerOgreTechniqueGeneric(engine);
    registerOgrePassGeneric(engine);
    registerOgreTextureUnitStateGeneric(engine);
    registerOgreMaterialManagerGeneric(engine);
    registerOgreTimerGeneric(engine);
    registerOgreGpuProgramParametersGeneric(engine);
}

// register Ogre::Vector3
static void registerOgreVector3Generic(AngelScript::asIScriptEngine* engine)
{
    int r;

    // Register the object constructors
    r = engine->RegisterObjectBehaviour("vector3", asBEHAVE_CONSTRUCT, "void f()", WRAP_OBJ_LAST(Vector3DefaultConstructor), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("vector3", asBEHAVE_CONSTRUCT, "void f(float, float, float)", WRAP_OBJ_LAST(Vector3InitConstructor), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("vector3", asBEHAVE_CONSTRUCT, "void f(const vector3 &in)", WRAP_OBJ_LAST(Vector3CopyConstructor), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("vector3", asBEHAVE_CONSTRUCT, "void f(float)", WRAP_OBJ_LAST(Vector3InitConstructorScaler), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    // Register the object operators
    r = engine->RegisterObjectMethod("vector3", "float opIndex(int) const", WRAP_OBJ_FIRST(Vector3OpIndex), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 &f(const vector3 &in)", WRAP_MFN_PR(Vector3, operator =, (const Vector3 &), Vector3&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "bool opEquals(const vector3 &in) const", WRAP_MFN_PR(Vector3, operator==,(const Vector3&) const, bool), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector3", "vector3 opAdd(const vector3 &in) const", WRAP_MFN_PR(Vector3, operator+,(const Vector3&) const, Vector3), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 opSub(const vector3 &in) const", WRAP_MFN_PR(Vector3, operator-,(const Vector3&) const, Vector3), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector3", "vector3 opMul(float) const", WRAP_MFN_PR(Vector3, operator*,(const float) const, Vector3), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 opMul(const vector3 &in) const", WRAP_MFN_PR(Vector3, operator*,(const Vector3&) const, Vector3), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 opDiv(float) const", WRAP_MFN_PR(Vector3, operator/,(const float) const, Vector3), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 opDiv(const vector3 &in) const", WRAP_MFN_PR(Vector3, operator/,(const Vector3&) const, Vector3), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector3", "vector3 opAdd() const", WRAP_OBJ_FIRST(Vector3OpAddUnary), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 opSub() const", WRAP_MFN_PR(Vector3, operator-,() const, Vector3), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector3", "vector3 &opAddAssign(const vector3 &in)", WRAP_MFN_PR(Vector3,operator+=,(const Vector3 &),Vector3&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 &opAddAssign(float)", WRAP_MFN_PR(Vector3,operator+=,(const float),Vector3&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector3", "vector3 &opSubAssign(const vector3 &in)", WRAP_MFN_PR(Vector3,operator-=,(const Vector3 &),Vector3&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 &opSubAssign(float)", WRAP_MFN_PR(Vector3,operator-=,(const float),Vector3&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector3", "vector3 &opMulAssign(const vector3 &in)", WRAP_MFN_PR(Vector3,operator*=,(const Vector3 &),Vector3&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 &opMulAssign(float)", WRAP_MFN_PR(Vector3,operator*=,(const float),Vector3&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector3", "vector3 &opDivAssign(const vector3 &in)", WRAP_MFN_PR(Vector3,operator/=,(const Vector3 &),Vector3&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 &opDivAssign(float)", WRAP_MFN_PR(Vector3,operator/=,(const float),Vector3&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    // Register the object methods
    r = engine->RegisterObjectMethod("vector3", "float length() const", WRAP_MFN(Vector3,length), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "float squaredLength() const", WRAP_MFN(Vector3,squaredLength), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector3", "float distance(const vector3 &in) const", WRAP_MFN(Vector3,distance), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "float squaredDistance(const vector3 &in) const", WRAP_MFN(Vector3,squaredDistance), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector3", "float dotProduct(const vector3 &in) const", WRAP_MFN(Vector3,dotProduct), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "float absDotProduct(const vector3 &in) const", WRAP_MFN(Vector3,absDotProduct), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector3", "float normalise()", WRAP_MFN(Vector3,normalise), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 crossProduct(const vector3 &in) const", WRAP_MFN(Vector3,crossProduct), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 midPoint(const vector3 &in) const", WRAP_MFN(Vector3,midPoint), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "void makeFloor(const vector3 &in)", WRAP_MFN(Vector3,makeFloor), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "void makeCeil(const vector3 &in)", WRAP_MFN(Vector3,makeCeil), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 perpendicular() const", WRAP_MFN(Vector3,perpendicular), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 randomDeviant(const radian &in, const vector3 &in) const", WRAP_MFN(Vector3,randomDeviant), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "radian angleBetween(const vector3 &in)", WRAP_MFN(Vector3,angleBetween), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "quaternion getRotationTo(const vector3 &in, const vector3 &in) const", WRAP_MFN(Vector3,getRotationTo), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "bool isZeroLength() const", WRAP_MFN(Vector3,isZeroLength), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 normalisedCopy() const", WRAP_MFN(Vector3,normalisedCopy), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "vector3 reflect(const vector3 &in) const", WRAP_MFN(Vector3,reflect), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector3", "bool positionEquals(const vector3 &in, float) const", WRAP_MFN(Vector3,positionEquals), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "bool positionCloses(const vector3 &in, float) const", WRAP_MFN(Vector3,positionCloses), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector3", "bool directionEquals(const vector3 &in, radian &in) const", WRAP_MFN(Vector3,directionEquals), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector3", "bool isNaN() const", WRAP_MFN(Vector3,isNaN), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
}

// register Ogre::Vector2
static void registerOgreVector2Generic(AngelScript::asIScriptEngine* engine)
{
    int r;

    // Register the object constructors
    r = engine->RegisterObjectBehaviour("vector2", asBEHAVE_CONSTRUCT, "void f()", WRAP_OBJ_LAST(Vector2DefaultConstructor), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("vector2", asBEHAVE_CONSTRUCT, "void f(float, float)", WRAP_OBJ_LAST(Vector2InitConstructor), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("vector2", asBEHAVE_CONSTRUCT, "void f(const vector2 &in)", WRAP_OBJ_LAST(Vector2CopyConstructor), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("vector2", asBEHAVE_CONSTRUCT, "void f(float)", WRAP_OBJ_LAST(Vector2InitConstructorScaler), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    // Register the object operators
    r = engine->RegisterObjectMethod("vector2", "float opIndex(int) const", WRAP_OBJ_FIRST(Vector2OpIndex), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 &f(const vector2 &in)", WRAP_MFN_PR(Vector2, operator =, (const Vector2 &), Vector2&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "bool opEquals(const vector2 &in) const", WRAP_MFN_PR(Vector2, operator==,(const Vector2&) const, bool), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector2", "vector2 opAdd(const vector2 &in) const", WRAP_MFN_PR(Vector2, operator+,(const Vector2&) const, Vector2), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 opSub(const vector2 &in) const", WRAP_MFN_PR(Vector2, operator-,(const Vector2&) const, Vector2), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector2", "vector2 opMul(float) const", WRAP_MFN_PR(Vector2, operator*,(const float) const, Vector2), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 opMul(const vector2 &in) const", WRAP_MFN_PR(Vector2, operator*,(const Vector2&) const, Vector2), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 opDiv(float) const", WRAP_MFN_PR(Vector2, operator/,(const float) const, Vector2), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 opDiv(const vector2 &in) const", WRAP_MFN_PR(Vector2, operator/,(const Vector2&) const, Vector2), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector2", "vector2 opAdd() const", WRAP_OBJ_FIRST(Vector2OpAddUnary), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 opSub() const", WRAP_MFN_PR(Vector2, operator-,() const, Vector2), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector2", "vector2 &opAddAssign(const vector2 &in)", WRAP_MFN_PR(Vector2,operator+=,(const Vector2 &),Vector2&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 &opAddAssign(float)", WRAP_MFN_PR(Vector2,operator+=,(const float),Vector2&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector2", "vector2 &opSubAssign(const vector2 &in)", WRAP_MFN_PR(Vector2,operator-=,(const Vector2 &),Vector2&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 &opSubAssign(float)", WRAP_MFN_PR(Vector2,operator-=,(const float),Vector2&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector2", "vector2 &opMulAssign(const vector2 &in)", WRAP_MFN_PR(Vector2,operator*=,(const Vector2 &),Vector2&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 &opMulAssign(float)", WRAP_MFN_PR(Vector2,operator*=,(const float),Vector2&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector2", "vector2 &opDivAssign(const vector2 &in)", WRAP_MFN_PR(Vector2,operator/=,(const Vector2 &),Vector2&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 &opDivAssign(float)", WRAP_MFN_PR(Vector2,operator/=,(const float),Vector2&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    // Register the object methods

    r = engine->RegisterObjectMethod("vector2", "float length() const", WRAP_MFN(Vector2,length), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "float squaredLength() const", WRAP_MFN(Vector2,squaredLength), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector2", "float distance(const vector2 &in) const", WRAP_MFN(Vector2,distance), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "float squaredDistance(const vector2 &in) const", WRAP_MFN(Vector2,squaredDistance), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector2", "float dotProduct(const vector2 &in) const", WRAP_MFN(Vector2,dotProduct), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector2", "float normalise()", WRAP_MFN(Vector2,normalise), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "float crossProduct(const vector2 &in) const", WRAP_MFN(Vector2,crossProduct), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 midPoint(const vector2 &in) const", WRAP_MFN(Vector2,midPoint), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "void makeFloor(const vector2 &in)", WRAP_MFN(Vector2,makeFloor), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "void makeCeil(const vector2 &in)", WRAP_MFN(Vector2,makeCeil), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 perpendicular() const", WRAP_MFN(Vector2,perpendicular), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 randomDeviant(const radian &in, const vector2 &in) const", WRAP_MFN(Vector2,randomDeviant), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "radian angleBetween(const vector2 &in)", WRAP_MFN(Vector2,angleBetween), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "bool isZeroLength() const", WRAP_MFN(Vector2,isZeroLength), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 normalisedCopy() const", WRAP_MFN(Vector2,normalisedCopy), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("vector2", "vector2 reflect(const vector2 &in) const", WRAP_MFN(Vector2,reflect), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector2", "bool positionEquals(const vector2 &in, float) const", WRAP_MFN(Vector2,positionEquals), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("vector2", "bool isNaN() const", WRAP_MFN(Vector2,isNaN), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
}

static void registerOgreRadianGeneric(AngelScript::asIScriptEngine* engine)
{
    int r;

    // Register the object constructors
    r = engine->RegisterObjectBehaviour("radian", asBEHAVE_CONSTRUCT, "void f()", WRAP_OBJ_LAST(RadianDefaultConstructor), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("radian", asBEHAVE_CONSTRUCT, "void f(float)", WRAP_OBJ_LAST(RadianInitConstructor), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("radian", asBEHAVE_CONSTRUCT, "void f(const radian &in)", WRAP_OBJ_LAST(RadianCopyConstructor), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    // Register the object operators
    r = engine->RegisterObjectMethod("radian", "radian &opAssign(const radian &in)", WRAP_MFN_PR(Radian, operator =, (const Radian &), Radian&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("radian", "radian &opAssign(const float)", WRAP_OBJ_FIRST(RadianAssignFloat), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("radian", "radian &opAssign(const degree &in)", WRAP_MFN_PR(Radian, operator =, (const Degree &), Radian&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("radian", "radian opAdd() const", WRAP_OBJ_FIRST(RadianOpAddUnary), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("radian", "radian opAdd(const radian &in) const", WRAP_MFN_PR(Radian, operator+,(const Radian&) const, Radian), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("radian", "radian opAdd(const degree &in) const", WRAP_MFN_PR(Radian, operator+,(const Degree&) const, Radian), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("radian", "radian &opAddAssign(const radian &in)", WRAP_MFN_PR(Radian,operator+=,(const Radian &),Radian&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("radian", "radian &opAddAssign(const degree &in)", WRAP_MFN_PR(Radian,operator+=,(const Degree &),Radian&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("radian", "radian opSub() const", WRAP_MFN_PR(Radian, operator-,() const, Radian), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("radian", "radian opSub(const radian &in) const", WRAP_MFN_PR(Radian, operator-,(const Radian&) const, Radian), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("radian", "radian opSub(const degree &in) const", WRAP_MFN_PR(Radian, operator-,(const Degree&) const, Radian), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("radian", "radian &opSubAssign(const radian &in)", WRAP_MFN_PR(Radian,operator-=,(const Radian &),Radian&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("radian", "radian &opSubAssign(const degree &in)", WRAP_MFN_PR(Radian,operator-=,(const Degree &),Radian&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("radian", "radian opMul(float) const", WRAP_MFN_PR(Radian, operator*,(float) const, Radian), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("radian", "radian opMul(const radian &in) const", WRAP_MFN_PR(Radian, operator*,(const Radian&) const, Radian), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("radian", "radian &opMulAssign(float)", WRAP_MFN_PR(Radian,operator*=,(float),Radian&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("radian", "radian opDiv(float) const", WRAP_MFN_PR(Radian, operator/,(float) const, Radian), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    // NOTE: mirrors the native binding, which maps opDivAssign to operator*= (!)
    r = engine->RegisterObjectMethod("radian", "radian &opDivAssign(float)", WRAP_MFN_PR(Radian,operator*=,(float),Radian&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("radian", "int opCmp(const radian &in) const", WRAP_OBJ_FIRST(RadianCmp), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("radian", "bool opEquals(const radian &in) const", WRAP_MFN_PR(Radian, operator==,(const Radian&) const, bool), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    // Register the object methods
    r = engine->RegisterObjectMethod("radian", "float valueDegrees() const", WRAP_MFN(Radian,valueDegrees), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("radian", "float valueRadians() const", WRAP_MFN(Radian,valueRadians), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("radian", "float valueAngleUnits() const", WRAP_MFN(Radian,valueAngleUnits), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
}

static void registerOgreDegreeGeneric(AngelScript::asIScriptEngine* engine)
{
    int r;

    // Register the object constructors
    r = engine->RegisterObjectBehaviour("degree", asBEHAVE_CONSTRUCT, "void f()", WRAP_OBJ_LAST(DegreeDefaultConstructor), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("degree", asBEHAVE_CONSTRUCT, "void f(float)", WRAP_OBJ_LAST(DegreeInitConstructor), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("degree", asBEHAVE_CONSTRUCT, "void f(const degree &in)", WRAP_OBJ_LAST(DegreeCopyConstructor), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    // Register the object operators
    r = engine->RegisterObjectMethod("degree", "degree &opAssign(const degree &in)", WRAP_MFN_PR(Degree, operator =, (const Degree &), Degree&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("degree", "degree &opAssign(float)", WRAP_OBJ_FIRST(DegreeAssignFloat), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("degree", "degree &opAssign(const radian &in)", WRAP_MFN_PR(Degree, operator =, (const Radian &), Degree&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("degree", "degree opAdd() const", WRAP_OBJ_FIRST(DegreeOpAddUnary), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("degree", "degree opAdd(const degree &in) const", WRAP_MFN_PR(Degree, operator+,(const Degree&) const, Degree), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("degree", "degree opAdd(const radian &in) const", WRAP_MFN_PR(Degree, operator+,(const Radian&) const, Degree), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("degree", "degree &opAddAssign(const degree &in)", WRAP_MFN_PR(Degree,operator+=,(const Degree &),Degree&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("degree", "degree &opAddAssign(const radian &in)", WRAP_MFN_PR(Degree,operator+=,(const Radian &),Degree&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("degree", "degree opSub() const", WRAP_MFN_PR(Degree, operator-,() const, Degree), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("degree", "degree opSub(const degree &in) const", WRAP_MFN_PR(Degree, operator-,(const Degree&) const, Degree), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("degree", "degree opSub(const radian &in) const", WRAP_MFN_PR(Degree, operator-,(const Radian&) const, Degree), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("degree", "degree &opSubAssign(const degree &in)", WRAP_MFN_PR(Degree,operator-=,(const Degree &),Degree&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("degree", "degree &opSubAssign(const radian &in)", WRAP_MFN_PR(Degree,operator-=,(const Radian &),Degree&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("degree", "degree opMul(float) const", WRAP_MFN_PR(Degree, operator*,(float) const, Degree), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("degree", "degree opMul(const degree &in) const", WRAP_MFN_PR(Degree, operator*,(const Degree&) const, Degree), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("degree", "degree &opMulAssign(float)", WRAP_MFN_PR(Degree,operator*=,(float),Degree&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("degree", "degree opDiv(float) const", WRAP_MFN_PR(Degree, operator/,(float) const, Degree), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    // NOTE: mirrors the native binding, which maps opDivAssign to operator*= (!)
    r = engine->RegisterObjectMethod("degree", "degree &opDivAssign(float)", WRAP_MFN_PR(Degree,operator*=,(float),Degree&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("degree", "int opCmp(const degree &in) const", WRAP_OBJ_FIRST(DegreeCmp), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("degree", "bool opEquals(const degree &in) const", WRAP_MFN_PR(Degree, operator==,(const Degree&) const, bool), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    // Register the object methods
    r = engine->RegisterObjectMethod("degree", "float valueRadians() const", WRAP_MFN(Degree,valueRadians), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("degree", "float valueDegrees() const", WRAP_MFN(Degree,valueDegrees), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("degree", "float valueAngleUnits() const", WRAP_MFN(Degree,valueAngleUnits), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
}

static void registerOgreQuaternionGeneric(AngelScript::asIScriptEngine* engine)
{
    int r;

    // Register the object constructors
    r = engine->RegisterObjectBehaviour("quaternion", asBEHAVE_CONSTRUCT, "void f()", WRAP_OBJ_LAST(QuaternionDefaultConstructor), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("quaternion", asBEHAVE_CONSTRUCT, "void f(const radian &in, const vector3 &in)", WRAP_OBJ_LAST(QuaternionInitConstructor1), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("quaternion", asBEHAVE_CONSTRUCT, "void f(float, float, float, float)", WRAP_OBJ_LAST(QuaternionInitConstructor2), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("quaternion", asBEHAVE_CONSTRUCT, "void f(const vector3 &in, const vector3 &in, const vector3 &in)", WRAP_OBJ_LAST(QuaternionInitConstructor3), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("quaternion", asBEHAVE_CONSTRUCT, "void f(float)", WRAP_OBJ_LAST(QuaternionInitConstructorScaler), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("quaternion", asBEHAVE_CONSTRUCT, "void f(const quaternion &in)", WRAP_OBJ_LAST(QuaternionCopyConstructor), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    // Register the object operators
    r = engine->RegisterObjectMethod("quaternion", "float opIndex(int) const", WRAP_OBJ_FIRST(QuaternionOpIndex), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "quaternion &opAssign(const quaternion &in)", WRAP_MFN_PR(Quaternion, operator =, (const Quaternion &), Quaternion&), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "quaternion opAdd(const quaternion &in) const", WRAP_MFN_PR(Quaternion, operator+,(const Quaternion&) const, Quaternion), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "quaternion opSub(const quaternion &in) const", WRAP_MFN_PR(Quaternion, operator-,(const Quaternion&) const, Quaternion), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "quaternion opMul(const quaternion &in) const", WRAP_MFN_PR(Quaternion, operator*,(const Quaternion&) const, Quaternion), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "quaternion opMul(float) const", WRAP_MFN_PR(Quaternion, operator*,(float) const, Quaternion), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "quaternion opSub() const", WRAP_MFN_PR(Quaternion, operator-,() const, Quaternion), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "bool opEquals(const quaternion &in) const", WRAP_MFN_PR(Quaternion, operator==,(const Quaternion&) const, bool), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "vector3 opMul(const vector3 &in) const", WRAP_MFN_PR(Quaternion, operator*,(const Vector3&) const, Vector3), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    // Register the object methods
    r = engine->RegisterObjectMethod("quaternion", "float Dot(const quaternion &in) const", WRAP_MFN(Quaternion,Dot), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "float Norm() const", WRAP_MFN(Quaternion,Norm), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "float normalise()", WRAP_MFN(Quaternion,normalise), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "quaternion Inverse() const", WRAP_MFN(Quaternion,Inverse), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "quaternion UnitInverse() const", WRAP_MFN(Quaternion,UnitInverse), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "quaternion Exp() const", WRAP_MFN(Quaternion,Exp), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "quaternion Log() const", WRAP_MFN(Quaternion,Log), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "radian getRoll(bool reprojectAxis = true) const", WRAP_MFN(Quaternion,getRoll), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "radian getPitch(bool reprojectAxis = true) const", WRAP_MFN(Quaternion,getPitch), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "radian getYaw(bool reprojectAxis = true) const", WRAP_MFN(Quaternion,getYaw), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "bool equals(const quaternion &in, const radian &in) const", WRAP_MFN(Quaternion,equals), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "bool isNaN() const", WRAP_MFN(Quaternion,isNaN), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    // Register some static methods
    r = engine->RegisterGlobalFunction("quaternion Slerp(float, const quaternion &in, const quaternion &in, bool &in)", WRAP_FN_PR(Quaternion::Slerp,(Real fT, const Quaternion&, const Quaternion&, bool), Quaternion), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterGlobalFunction("quaternion SlerpExtraSpins(float, const quaternion &in, const quaternion &in, int &in)", WRAP_FN(Quaternion::SlerpExtraSpins), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterGlobalFunction("void Intermediate(const quaternion &in, const quaternion &in, const quaternion &in, const quaternion &in, const quaternion &in)", WRAP_FN(Quaternion::Intermediate), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterGlobalFunction("quaternion Squad(float, const quaternion &in, const quaternion &in, const quaternion &in, const quaternion &in, bool &in)", WRAP_FN(Quaternion::Squad), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterGlobalFunction("quaternion nlerp(float, const quaternion &in, const quaternion &in, bool &in)", WRAP_FN(Quaternion::nlerp), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
}

static void registerOgreColourValueGeneric(AngelScript::asIScriptEngine* engine)
{
    int r;

    // Register the object constructors
    r = engine->RegisterObjectBehaviour("color", asBEHAVE_CONSTRUCT, "void f()", WRAP_OBJ_LAST(ColourValueDefaultConstructor), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("color", asBEHAVE_CONSTRUCT, "void f(float r, float g, float b, float a)", WRAP_OBJ_LAST(ColourValueInitConstructor), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("color", asBEHAVE_CONSTRUCT, "void f(const color &other)", WRAP_OBJ_LAST(ColourValueCopyConstructor), asCALL_GENERIC);
}

static void registerOgreBoxGeneric(AngelScript::asIScriptEngine* engine)
{
    int r;

    // Register the object constructors
    r = engine->RegisterObjectBehaviour("box", asBEHAVE_CONSTRUCT, "void f()", WRAP_OBJ_LAST(BoxDefaultConstructor), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("box", asBEHAVE_CONSTRUCT, "void f(uint32 l, uint32 t, uint32 r, uint32 b)", WRAP_OBJ_LAST(BoxInitConstructor), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("box", asBEHAVE_CONSTRUCT, "void f(const box &other)", WRAP_OBJ_LAST(BoxCopyConstructor), asCALL_GENERIC);

    // Register other behaviors
    r = engine->RegisterObjectBehaviour("box", asBEHAVE_DESTRUCT, "void f()", WRAP_OBJ_LAST(BoxDestructor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("box", "box& opAssign(const box&in)", WRAP_OBJ_LAST(BoxAssignOperator), asCALL_GENERIC); ROR_ASSERT(r >= 0);
}

static void registerOgreTextureGeneric(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectBehaviour("TexturePtr", asBEHAVE_CONSTRUCT, "void f()", WRAP_OBJ_LAST(TexturePtrDefaultConstructor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("TexturePtr", asBEHAVE_CONSTRUCT, "void f(const TexturePtr&in)", WRAP_OBJ_LAST(TexturePtrCopyConstructor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("TexturePtr", asBEHAVE_DESTRUCT, "void f()", WRAP_OBJ_LAST(TexturePtrDestructor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("TexturePtr", "TexturePtr& opAssign(const TexturePtr&in)", WRAP_OBJ_LAST(TexturePtrAssignOperator), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("TexturePtr", "bool isNull()", WRAP_OBJ_LAST(TexturePtrIsNull), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    // Wrappers are inevitable, see https://www.gamedev.net/forums/topic/540419-custom-smartpointers-and-angelscript-/
    r = engine->RegisterObjectMethod("TexturePtr", "string getName() const", WRAP_OBJ_FIRST(TexturePtrGetName), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("TexturePtr", "uint getWidth()", WRAP_OBJ_FIRST(TexturePtrGetWidth), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("TexturePtr", "uint getHeight()", WRAP_OBJ_FIRST(TexturePtrGetHeight), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("TexturePtr", "uint getNumMipmaps()", WRAP_OBJ_FIRST(TexturePtrGetNumMipmaps), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("TexturePtr", "HardwarePixelBufferPtr getBuffer(uint, uint)", WRAP_OBJ_FIRST(TexturePtrGetBuffer), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

static void registerOgreTextureManagerGeneric(AngelScript::asIScriptEngine * engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    // Convenience wrapper to omit optional parameters
    r = engine->RegisterObjectMethod("TextureManager", "TexturePtr load(const string&in file, const string&in rg)", WRAP_OBJ_FIRST(TextureManagerLoad), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace("Ogre::TextureManager"); ROR_ASSERT(r >= 0);
    r = engine->RegisterGlobalFunction("TextureManager& getSingleton()", WRAP_FN(TextureManager::getSingleton), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

template <typename T>
static void registerOgreMovableObjectBaseGeneric(AngelScript::asIScriptEngine* engine, const char* obj)
{
    int r;
    r = engine->RegisterObjectMethod(obj, "string __getUniqueName() const", WRAP_OBJ_LAST(MovableObjectGetUniqueNameMixin), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod(obj, "const string& getName() const", WRAP_MFN(MovableObject, getName), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "const string& getMovableType() const", WRAP_MFN(MovableObject, getMovableType), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod(obj, "Node@ getParentNode()", WRAP_MFN(MovableObject, getParentNode), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "SceneNode@ getParentSceneNode()", WRAP_MFN(MovableObject, getParentSceneNode), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "bool isParentTagPoint() const", WRAP_MFN(MovableObject, isParentTagPoint), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "bool isAttached() const", WRAP_MFN(MovableObject, isAttached), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "void detachFromParent()", WRAP_MFN(MovableObject, detachFromParent), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod(obj, "bool isInScene() const", WRAP_MFN(MovableObject, isInScene), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "float getBoundingRadius() const", WRAP_MFN(MovableObject, getBoundingRadius), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod(obj, "void setVisible(bool visible)", WRAP_MFN(MovableObject, setVisible), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "bool getVisible() const", WRAP_MFN(MovableObject, getVisible), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "bool isVisible() const", WRAP_MFN(MovableObject, isVisible), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod(obj, "void setRenderingDistance(float dist)", WRAP_MFN(MovableObject, setRenderingDistance), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "float getRenderingDistance() const", WRAP_MFN(MovableObject, getRenderingDistance), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod(obj, "void setRenderingMinPixelSize(float pixelSize)", WRAP_MFN(MovableObject, setRenderingMinPixelSize), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "float getRenderingMinPixelSize() const", WRAP_MFN(MovableObject, getRenderingMinPixelSize), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod(obj, "void setCastShadows(bool enabled)", WRAP_MFN(MovableObject, setCastShadows), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod(obj, "bool getCastShadows() const", WRAP_MFN(MovableObject, getCastShadows), asCALL_GENERIC); ROR_ASSERT(r >= 0);
}

static void registerOgreMovableObjectGeneric(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    registerOgreMovableObjectBaseGeneric<MovableObject>(engine, "MovableObject");

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

static void registerOgreAnimationStateGeneric(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    // Register the getters and setters
    r = engine->RegisterObjectMethod("AnimationState", "const string& getAnimationName() const", WRAP_MFN(AnimationState, getAnimationName), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "float getTimePosition() const", WRAP_MFN(AnimationState, getTimePosition), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "void setTimePosition(float)", WRAP_MFN(AnimationState, setTimePosition), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "float getLength() const", WRAP_MFN(AnimationState, getLength), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "void setLength(float)", WRAP_MFN(AnimationState, setLength), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "float getWeight() const", WRAP_MFN(AnimationState, getWeight), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "void setWeight(float)", WRAP_MFN(AnimationState, setWeight), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "void addTime(float)", WRAP_MFN(AnimationState, addTime), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "bool hasEnded() const", WRAP_MFN(AnimationState, hasEnded), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "bool getEnabled() const", WRAP_MFN(AnimationState, getEnabled), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "void setEnabled(bool)", WRAP_MFN(AnimationState, setEnabled), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "void setLoop(bool)", WRAP_MFN(AnimationState, setLoop), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "bool getLoop() const", WRAP_MFN(AnimationState, getLoop), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "AnimationStateSet@ getParent()", WRAP_MFN(AnimationState, getParent), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "void createBlendMask(uint blendMaskSizeHint, float initialWeight = 1.0f)", WRAP_OBJ_FIRST(AnimationStateCreateBlendMask), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "void destroyBlendMask()", WRAP_MFN(AnimationState, destroyBlendMask), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "bool hasBlendMask() const", WRAP_MFN(AnimationState, hasBlendMask), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "void setBlendMaskEntry(uint boneHandle, float weight)", WRAP_OBJ_FIRST(AnimationStateSetBlendMaskEntry), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationState", "float getBlendMaskEntry(uint boneHandle)", WRAP_OBJ_FIRST(AnimationStateGetBlendMaskEntry), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

static void registerOgreAnimationStateSetGeneric(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("AnimationStateSet", "AnimationState@ createAnimationState(const string& in, float, float, float = 1.0f, bool = false)", WRAP_MFN(AnimationStateSet, createAnimationState), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationStateSet", "AnimationState@ getAnimationState(const string& in) const", WRAP_MFN(AnimationStateSet, getAnimationState), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationStateSet", "bool hasAnimationState(const string& in) const", WRAP_MFN(AnimationStateSet, hasAnimationState), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationStateSet", "void removeAnimationState(const string& in)", WRAP_MFN(AnimationStateSet, removeAnimationState), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationStateSet", "void removeAllAnimationStates()", WRAP_MFN(AnimationStateSet, removeAllAnimationStates), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("AnimationStateSet", "AnimationStateDict@ getAnimationStates()", WRAP_OBJ_LAST(AnimationStateSetGetAnimationStates), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

static void registerOgreHardwarePixelBufferGeneric(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectBehaviour("HardwarePixelBufferPtr", asBEHAVE_CONSTRUCT, "void f()", WRAP_OBJ_LAST(HardwarePixelBufferPtrDefaultConstructor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("HardwarePixelBufferPtr", asBEHAVE_CONSTRUCT, "void f(const HardwarePixelBufferPtr&in)", WRAP_OBJ_LAST(HardwarePixelBufferPtrCopyConstructor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("HardwarePixelBufferPtr", asBEHAVE_DESTRUCT, "void f()", WRAP_OBJ_LAST(HardwarePixelBufferPtrDestructor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("HardwarePixelBufferPtr", "HardwarePixelBufferPtr& opAssign(const HardwarePixelBufferPtr&in)", WRAP_OBJ_LAST(HardwarePixelBufferPtrAssignOperator), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("HardwarePixelBufferPtr", "const PixelBox& getCurrentLock()", WRAP_OBJ_FIRST(HardwarePixelBufferPtrGetCurrentLock), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("HardwarePixelBufferPtr", "const PixelBox& lock(const box& lockbox, HardwareBufferLockOptions opt)", WRAP_OBJ_FIRST(HardwarePixelBufferPtrLock), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("HardwarePixelBufferPtr", "uint getWidth()", WRAP_OBJ_FIRST(HardwarePixelBufferPtrGetWidth), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("HardwarePixelBufferPtr", "uint getHeight()", WRAP_OBJ_FIRST(HardwarePixelBufferPtrGetHeight), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("HardwarePixelBufferPtr", "void blitFromMemory(const PixelBox& src, const box& dst)", WRAP_OBJ_FIRST(HardwarePixelBufferPtrBlitFromMemory), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("HardwarePixelBufferPtr", "void blitToMemory(const box& src, const PixelBox& dst)", WRAP_OBJ_FIRST(HardwarePixelBufferPtrBlitToMemory), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("HardwarePixelBufferPtr", "void unlock()", WRAP_OBJ_FIRST(HardwarePixelBufferPtrUnlock), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    engine->SetDefaultNamespace("");
}

static void registerOgrePixelBoxGeneric(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectBehaviour("PixelBox", asBEHAVE_CONSTRUCT, "void f()", WRAP_OBJ_LAST(PixelBoxDefaultConstructor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("PixelBox", asBEHAVE_CONSTRUCT, "void f(const PixelBox&in)", WRAP_OBJ_LAST(PixelBoxCopyConstructor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("PixelBox", asBEHAVE_DESTRUCT, "void f()", WRAP_OBJ_LAST(PixelBoxDestructor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("PixelBox", "PixelBox& opAssign(const PixelBox&in)", WRAP_OBJ_LAST(PixelBoxAssignOperator), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("PixelBox", "color getColourAt(uint32 x, uint32 y, uint32 z)", WRAP_OBJ_FIRST(PixelBoxGetColourAt), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("PixelBox", "void setColourAt(const color& c, uint32 x, uint32 y, uint32 z)", WRAP_OBJ_FIRST(PixelBoxSetColourAt), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    // Inherited methods must have wrappers - see AngelScript doc
    r = engine->RegisterObjectMethod("PixelBox", "uint getWidth()",  WRAP_OBJ_FIRST(PixelBoxGetWidth), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("PixelBox", "uint getHeight()", WRAP_OBJ_FIRST(PixelBoxGetHeight), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("PixelBox", "uint getDepth()",  WRAP_OBJ_FIRST(PixelBoxGetDepth), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

static void registerOgreImageGeneric(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectBehaviour("Image", asBEHAVE_CONSTRUCT, "void f()", WRAP_OBJ_LAST(ImageDefaultConstructor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("Image", asBEHAVE_CONSTRUCT, "void f(const Image&in)", WRAP_OBJ_LAST(ImageCopyConstructor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("Image", asBEHAVE_DESTRUCT, "void f()", WRAP_OBJ_LAST(ImageDestructor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Image", "Image& opAssign(const Image&in)", WRAP_OBJ_LAST(ImageAssignOperator), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("Image", "color getColourAt(uint32 x, uint32 y, uint32 z)", WRAP_OBJ_FIRST(ImageGetColourAt), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Image", "void setColourAt(const color& c, uint32 x, uint32 y, uint32 z)", WRAP_OBJ_FIRST(ImageSetColourAt), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Image", "Image& flipAroundX()", WRAP_MFN(Ogre::Image, flipAroundX), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Image", "Image& flipAroundY()", WRAP_MFN(Ogre::Image, flipAroundY), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Image", "uint getNumMipmaps()", WRAP_MFN(Ogre::Image, getNumMipmaps), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Image", "uint getNumFaces()", WRAP_MFN(Ogre::Image, getNumFaces), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Image", "PixelBox getPixelBox(uint face, uint mipmap)", WRAP_OBJ_FIRST(ImageGetPixelBox), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Image", "uint getSize()", WRAP_MFN(Ogre::Image, getSize), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Image", "uint getWidth()", WRAP_MFN(Ogre::Image, getWidth), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Image", "uint getHeight()", WRAP_MFN(Ogre::Image, getHeight), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Image", "void resize(uint16 width, uint16 height, ImageFilter filter)", WRAP_MFN(Ogre::Image, resize), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

static void registerOgreSubMeshGeneric(AngelScript::asIScriptEngine* engine)
{
    engine->SetDefaultNamespace("Ogre");

    // Register the SubMesh class
    engine->RegisterObjectMethod("SubMesh", "const string& getMaterialName()", WRAP_MFN(Ogre::SubMesh, getMaterialName), asCALL_GENERIC);
    engine->RegisterObjectMethod("SubMesh", "void setMaterialName(const string&in, const string&in)", WRAP_MFN(Ogre::SubMesh, setMaterialName), asCALL_GENERIC);

    // > Vertex buffer
    engine->RegisterObjectMethod("SubMesh", "array<vector3>@ __getVertexPositions()", WRAP_OBJ_FIRST(SubMesh__getVertexPositions), asCALL_GENERIC);
    engine->RegisterObjectMethod("SubMesh", "array<vector2>@ __getVertexTexcoords(uint index)", WRAP_OBJ_FIRST(SubMesh__getVertexTexcoords), asCALL_GENERIC);

    // > Index buffer
    engine->RegisterObjectMethod("SubMesh", "array<uint16>@ __getIndexBuffer16bit()", WRAP_OBJ_FIRST(SubMesh__getIndexBuffer16bit), asCALL_GENERIC);
    engine->RegisterObjectMethod("SubMesh", "array<uint>@ __getIndexBuffer32bit()", WRAP_OBJ_FIRST(SubMesh__getIndexBuffer32bit), asCALL_GENERIC);
    engine->RegisterObjectMethod("SubMesh", "IndexType __getIndexType()", WRAP_OBJ_FIRST(SubMesh__getIndexType), asCALL_GENERIC);

    engine->SetDefaultNamespace("");
}

static void registerOgreMeshGeneric(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectBehaviour("MeshPtr", asBEHAVE_CONSTRUCT, "void f()", WRAP_OBJ_LAST(MeshPtrDefaultConstructor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("MeshPtr", asBEHAVE_CONSTRUCT, "void f(const MeshPtr&in)", WRAP_OBJ_LAST(MeshPtrCopyConstructor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("MeshPtr", asBEHAVE_DESTRUCT, "void f()", WRAP_OBJ_LAST(MeshPtrDestructor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("MeshPtr", "MeshPtr& opAssign(const MeshPtr&in)", WRAP_OBJ_LAST(MeshPtrAssignOperator), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("MeshPtr", "bool isNull()", WRAP_OBJ_LAST(MeshPtrIsNull), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    // Wrappers are inevitable, see https://www.gamedev.net/forums/topic/540419-custom-smartpointers-and-angelscript-/
    r = engine->RegisterObjectMethod("MeshPtr", "SubMeshArray@ getSubMeshes()", WRAP_OBJ_FIRST(MeshPtrGetSubmeshes), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("MeshPtr", "string getName()", WRAP_OBJ_FIRST(MeshPtrGetName), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("MeshPtr", "SubMesh@ createSubMesh(const string&in name)", WRAP_OBJ_FIRST(MeshPtrCreateSubMesh), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("MeshPtr", "void destroySubMesh(const string&in name)", WRAP_OBJ_FIRST(MeshPtrDestroySubMesh), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

static void registerOgreMeshManagerGeneric(AngelScript::asIScriptEngine * engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("MeshManager", "MeshPtr load(const string&in file, const string&in rg)", WRAP_OBJ_FIRST(MeshManagerLoad), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("MeshManager", "void remove(const string&in file, const string&in rg)", WRAP_OBJ_FIRST(MeshManagerRemove), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace("Ogre::MeshManager"); ROR_ASSERT(r >= 0);
    r = engine->RegisterGlobalFunction("MeshManager& getSingleton()", WRAP_FN(MeshManager::getSingleton), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

static void registerOgreMaterialManagerGeneric(AngelScript::asIScriptEngine * engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("MaterialManager", "MaterialPtr getByName(const string&in file, const string&in rg)", WRAP_OBJ_FIRST(MaterialManagerGetByName), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("MaterialManager", "MaterialPtr create(const string&in file, const string&in rg)", WRAP_OBJ_FIRST(MaterialManagerCreate), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace("Ogre::MaterialManager"); ROR_ASSERT(r >= 0);
    r = engine->RegisterGlobalFunction("MaterialManager& getSingleton()", WRAP_FN(MaterialManager::getSingleton), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

static void registerOgreMaterialGeneric(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectBehaviour("MaterialPtr", asBEHAVE_CONSTRUCT, "void f()", WRAP_OBJ_LAST(MaterialPtrDefaultConstructor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("MaterialPtr", asBEHAVE_CONSTRUCT, "void f(const MaterialPtr&in)", WRAP_OBJ_LAST(MaterialPtrCopyConstructor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("MaterialPtr", asBEHAVE_DESTRUCT, "void f()", WRAP_OBJ_LAST(MaterialPtrDestructor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("MaterialPtr", "MaterialPtr& opAssign(const MaterialPtr&in)", WRAP_OBJ_LAST(MaterialPtrAssignOperator), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("MaterialPtr", "bool isNull()", WRAP_OBJ_LAST(MaterialPtrIsNull), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    // Wrappers are inevitable, see https://www.gamedev.net/forums/topic/540419-custom-smartpointers-and-angelscript-/
    r = engine->RegisterObjectMethod("MaterialPtr", "TechniqueArray@ getTechniques()", WRAP_OBJ_FIRST(MaterialPtrGetTechniques), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("MaterialPtr", "string getName()", WRAP_OBJ_FIRST(MaterialPtrGetName), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("MaterialPtr", "Technique@ createTechnique()", WRAP_OBJ_FIRST(MaterialPtrCreateTechnique), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("MaterialPtr", "void removeTechnique(uint16 index)", WRAP_OBJ_FIRST(MaterialPtrRemoveTechnique), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

static void registerOgreTechniqueGeneric(AngelScript::asIScriptEngine* engine)
{
    engine->SetDefaultNamespace("Ogre");

    engine->RegisterObjectMethod("Technique", "PassArray @getPasses()", WRAP_OBJ_FIRST(TechniqueGetPasses), asCALL_GENERIC);
    engine->RegisterObjectMethod("Technique", "Pass @createPass()", WRAP_MFN(Ogre::Technique, createPass), asCALL_GENERIC);
    engine->RegisterObjectMethod("Technique", "void removePass(uint16 index)", WRAP_MFN(Ogre::Technique, removePass), asCALL_GENERIC);
    engine->RegisterObjectMethod("Technique", "const string& getName() const", WRAP_MFN(Ogre::Technique, getName), asCALL_GENERIC);

    engine->SetDefaultNamespace("");
}

static void registerOgrePassGeneric(AngelScript::asIScriptEngine* engine)
{
    int r = 0;
    engine->SetDefaultNamespace("Ogre");

    engine->RegisterObjectMethod("Pass", "const string& getName() const", WRAP_MFN(Ogre::Pass, getName), asCALL_GENERIC);
    engine->RegisterObjectMethod("Pass", "TextureUnitStateArray @getTextureUnitStates()", WRAP_OBJ_FIRST(PassGetTextureUnitStates), asCALL_GENERIC);
    engine->RegisterObjectMethod("Pass", "void removeTextureUnitState(uint16 index)", WRAP_MFN(Ogre::Pass, removeTextureUnitState), asCALL_GENERIC);

    r = engine->RegisterObjectMethod("Pass", "GpuProgramParametersPtr getVertexProgramParameters()", WRAP_OBJ_FIRST(PassGetVertexProgramParameters), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Pass", "GpuProgramParametersPtr getFragmentProgramParameters()", WRAP_OBJ_FIRST(PassGetFragmentProgramParameters), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Pass", "GpuProgramParametersPtr getGeometryProgramParameters()", WRAP_OBJ_FIRST(PassGetGeometryProgramParameters), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Pass", "GpuProgramParametersPtr getTessellationHullProgramParameters()", WRAP_OBJ_FIRST(PassGetTessellationHullProgramParameters), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Pass", "GpuProgramParametersPtr getTessellationDomainProgramParameters()", WRAP_OBJ_FIRST(PassGetTessellationDomainProgramParameters), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Pass", "GpuProgramParametersPtr getComputeProgramParameters()", WRAP_OBJ_FIRST(PassGetComputeProgramParameters), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    engine->RegisterObjectMethod("Pass", "void setVertexProgramParameters(GpuProgramParametersPtr)", WRAP_MFN(Ogre::Pass, setVertexProgramParameters), asCALL_GENERIC);
    engine->RegisterObjectMethod("Pass", "void setFragmentProgramParameters(GpuProgramParametersPtr)", WRAP_MFN(Ogre::Pass, setFragmentProgramParameters), asCALL_GENERIC);
    engine->RegisterObjectMethod("Pass", "void setGeometryProgramParameters(GpuProgramParametersPtr)", WRAP_MFN(Ogre::Pass, setGeometryProgramParameters), asCALL_GENERIC);
    engine->RegisterObjectMethod("Pass", "void setTessellationHullProgramParameters(GpuProgramParametersPtr)", WRAP_MFN(Ogre::Pass, setTessellationHullProgramParameters), asCALL_GENERIC);
    engine->RegisterObjectMethod("Pass", "void setTessellationDomainProgramParameters(GpuProgramParametersPtr)", WRAP_MFN(Ogre::Pass, setTessellationDomainProgramParameters), asCALL_GENERIC);
    engine->RegisterObjectMethod("Pass", "void setComputeProgramParameters(GpuProgramParametersPtr)", WRAP_MFN(Ogre::Pass, setComputeProgramParameters), asCALL_GENERIC);

    engine->SetDefaultNamespace("");
}

static void registerOgreTextureUnitStateGeneric(AngelScript::asIScriptEngine* engine)
{
    engine->SetDefaultNamespace("Ogre");

    engine->RegisterObjectMethod("TextureUnitState", "const string& getName() const", WRAP_MFN(Ogre::TextureUnitState, getName), asCALL_GENERIC);
    engine->RegisterObjectMethod("TextureUnitState", "void setTexture(const TexturePtr&in)", WRAP_MFN_PR(Ogre::TextureUnitState, setTexture, (const TexturePtr&), void), asCALL_GENERIC);
    engine->RegisterObjectMethod("TextureUnitState", "const TexturePtr& _getTexturePtr() const", WRAP_MFN_PR(Ogre::TextureUnitState, _getTexturePtr, (void) const, const TexturePtr&), asCALL_GENERIC);

    engine->SetDefaultNamespace("");
}

static void registerOgreTimerGeneric(AngelScript::asIScriptEngine* engine)
{
    int r;
    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectBehaviour("Timer", asBEHAVE_CONSTRUCT, "void f()", WRAP_OBJ_LAST(TimerDefaultConstructor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("Timer", asBEHAVE_CONSTRUCT, "void f(const Timer&in)", WRAP_OBJ_LAST(TimerCopyConstructor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("Timer", asBEHAVE_DESTRUCT, "void f()", WRAP_OBJ_LAST(TimerDefaultDestructor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Timer", "Timer& opAssign(const Timer&in)", WRAP_OBJ_LAST(TimerAssignOperator), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("Timer", "void reset()", WRAP_MFN(Timer, reset), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Timer", "uint getMilliseconds()", WRAP_MFN(Timer, getMilliseconds), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Timer", "uint getMicroseconds()", WRAP_MFN(Timer, getMicroseconds), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Timer", "uint getMillisecondsCPU()", WRAP_MFN(Timer, getMillisecondsCPU), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("Timer", "uint getMicrosecondsCPU()", WRAP_MFN(Timer, getMicrosecondsCPU), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

static void registerOgreGpuProgramParametersGeneric(AngelScript::asIScriptEngine* engine)
{
    int r;

    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    // Note: The `*SharedPtr` is a deprecated alias of `*Ptr` in OGRE 14, but it's not yet present in the version we use.
    r = engine->RegisterObjectBehaviour("GpuProgramParametersPtr", asBEHAVE_CONSTRUCT, "void f()", WRAP_OBJ_LAST(GpuProgramParametersPtrDefaultConstructor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("GpuProgramParametersPtr", asBEHAVE_CONSTRUCT, "void f(const GpuProgramParametersPtr&in)", WRAP_OBJ_LAST(GpuProgramParametersPtrCopyConstructor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour("GpuProgramParametersPtr", asBEHAVE_DESTRUCT, "void f()", WRAP_OBJ_LAST(GpuProgramParametersPtrDestructor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "GpuProgramParametersPtr& opAssign(const GpuProgramParametersPtr&in)", WRAP_OBJ_LAST(GpuProgramParametersPtrAssignOperator), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "bool isNull()", WRAP_OBJ_LAST(GpuProgramParametersPtrIsNull), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    // Wrappers are inevitable, see https://www.gamedev.net/forums/topic/540419-custom-smartpointers-and-angelscript-/

    // > setConstant (scalar)
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setConstant(uint index, float val)", WRAP_OBJ_FIRST(GpuProgramParametersPtrSetConstantFloat), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setConstant(uint index, const vector3& val)", WRAP_OBJ_FIRST(GpuProgramParametersPtrSetConstantVector3), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setConstant(uint index, const vector2& val)", WRAP_OBJ_FIRST(GpuProgramParametersPtrSetConstantVector2), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setConstant(uint index, const color& val)", WRAP_OBJ_FIRST(GpuProgramParametersPtrSetConstantColourValue), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    // > setConstant (vector)
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setConstant(uint index, const array<float>@ vals)", WRAP_OBJ_FIRST(GpuProgramParametersPtrSetConstantFloatArray), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setConstant(uint index, const array<vector3>@ vals)", WRAP_OBJ_FIRST(GpuProgramParametersPtrSetConstantVector3Array), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setConstant(uint index, const array<vector2>@ vals)", WRAP_OBJ_FIRST(GpuProgramParametersPtrSetConstantVector2Array), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setConstant(uint index, const array<color>@ vals)", WRAP_OBJ_FIRST(GpuProgramParametersPtrSetConstantColourValueArray), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    // > setNamedConstant (scalar)
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setNamedConstant(const string&in name, float val)", WRAP_OBJ_FIRST(GpuProgramParametersPtrSetNamedConstantFloat), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setNamedConstant(const string&in name, const vector3& val)", WRAP_OBJ_FIRST(GpuProgramParametersPtrSetNamedConstantVector3), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setNamedConstant(const string&in name, const vector2& val)", WRAP_OBJ_FIRST(GpuProgramParametersPtrSetNamedConstantVector2), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setNamedConstant(const string&in name, const color& val)", WRAP_OBJ_FIRST(GpuProgramParametersPtrSetNamedConstantColourValue), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    // > setNamedConstant (vector)
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setNamedConstant(const string&in name, const array<float>@ vals)", WRAP_OBJ_FIRST(GpuProgramParametersPtrSetNamedConstantFloatArray), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setNamedConstant(const string&in name, const array<vector3>@ vals)", WRAP_OBJ_FIRST(GpuProgramParametersPtrSetNamedConstantVector3Array), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setNamedConstant(const string&in name, const array<vector2>@ vals)", WRAP_OBJ_FIRST(GpuProgramParametersPtrSetNamedConstantVector2Array), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "void setNamedConstant(const string&in name, const array<color>@ vals)", WRAP_OBJ_FIRST(GpuProgramParametersPtrSetNamedConstantColourValueArray), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("GpuProgramParametersPtr", "array<string>@ __getNamedConstants()", WRAP_OBJ_FIRST(GpuProgramParametersPtr__getNamedConstants), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);
}

