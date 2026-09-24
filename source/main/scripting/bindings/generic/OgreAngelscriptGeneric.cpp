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

// forward declarations, defined below
static void registerOgreVector3Generic(AngelScript::asIScriptEngine* engine);

// main registration method
void RoR::RegisterOgreObjectsGeneric(AngelScript::asIScriptEngine* engine)
{
    int r;

    // NOTE: data types, enums and object properties are registered in RegisterOgreObjectsCommon()
    registerOgreVector3Generic(engine);
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

