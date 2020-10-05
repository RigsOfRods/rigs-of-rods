/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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

#include "OgreAngelscript.h"

#include "Application.h"

using namespace Ogre;
using namespace AngelScript;

// helper/wrapper functions first

/***VECTOR3***/
static void Vector3DefaultConstructor(Vector3* self)
{
    new(self) Vector3();
}

static void Vector3CopyConstructor(const Vector3& other, Vector3* self)
{
    new(self) Vector3(other);
}

static void Vector3InitConstructor(float x, float y, float z, Vector3* self)
{
    new(self) Vector3(x, y, z);
}

static void Vector3InitConstructorScaler(float s, Vector3* self)
{
    new(self) Vector3(s, s, s);
}

// not used
static int Vector3Cmp(const Vector3& a, const Vector3& b)
{
    // If a is greater than b, then we return a positive number
    if (a > b)
        return 1;
    // If a is smaller than b, then we return a negative number
    else if (a < b)
        return -1;
    // If a is equal to b, then we return zero
    else if (a == b)
        return 0;
    // Now, what are we supposed to return if none of the above is true???
    else
        return -2; // definitly not this, this is interpreted as 'smaller than'...
}

/***RADIAN***/
static void RadianDefaultConstructor(Radian* self)
{
    new(self) Radian();
}

static void RadianCopyConstructor(const Radian& other, Radian* self)
{
    new(self) Radian(other);
}

static void RadianInitConstructor(float r, Radian* self)
{
    new(self) Radian(r);
}

static int RadianCmp(const Radian& a, const Radian& b)
{
    if (a > b)
        return 1;
    else if (a < b)
        return -1;
    else
        return 0;
}

/***DEGREE***/
static void DegreeDefaultConstructor(Degree* self)
{
    new(self) Degree();
}

static void DegreeCopyConstructor(const Degree& other, Degree* self)
{
    new(self) Degree(other);
}

static void DegreeInitConstructor(float r, Degree* self)
{
    new(self) Degree(r);
}

static int DegreeCmp(const Degree& a, const Degree& b)
{
    if (a > b)
        return 1;
    else if (a < b)
        return -1;
    else
        return 0;
}

/***QUATERNION***/
static void QuaternionDefaultConstructor(Quaternion* self)
{
    new(self) Quaternion();
}

static void QuaternionCopyConstructor(const Quaternion& other, Quaternion* self)
{
    new(self) Quaternion(other.w, other.x, other.y, other.z);
}

static void QuaternionInitConstructor1(const Radian& rfAngle, const Vector3& rkAxis, Quaternion* self)
{
    new(self) Quaternion(rfAngle, rkAxis);
}

static void QuaternionInitConstructor2(float w, float x, float y, float z, Quaternion* self)
{
    new(self) Quaternion(w, x, y, z);
}

static void QuaternionInitConstructor3(const Vector3& xaxis, const Vector3& yaxis, const Vector3& zaxis, Quaternion* self)
{
    new(self) Quaternion(xaxis, yaxis, zaxis);
}

// not used
static void QuaternionInitConstructor5(float val, Quaternion* self)
{
    new(self) Quaternion((Ogre::Real *)&val);
}

static void QuaternionInitConstructorScaler(float s, Quaternion* self)
{
    new(self) Quaternion(s, s, s, s);
}

// main registration method
void registerOgreObjects(AngelScript::asIScriptEngine* engine)
{
    int r;

    // We start by registering some data types, so angelscript knows that they exist

    // Ogre::Degree
    r = engine->RegisterObjectType("degree", sizeof(Degree), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CA | asOBJ_APP_CLASS_ALLFLOATS);
    ROR_ASSERT( r >= 0 );

    // Ogre::Radian
    r = engine->RegisterObjectType("radian", sizeof(Radian), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CA | asOBJ_APP_CLASS_ALLFLOATS);
    ROR_ASSERT( r >= 0 );

    // Ogre::Vector3
    r = engine->RegisterObjectType("vector3", sizeof(Ogre::Vector3), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CA | asOBJ_APP_CLASS_ALLFLOATS);
    ROR_ASSERT( r >= 0 );

    // Ogre::Quaternion
    r = engine->RegisterObjectType("quaternion", sizeof(Quaternion), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CA | asOBJ_APP_CLASS_ALLFLOATS);
    ROR_ASSERT( r >= 0 );

    registerOgreRadian(engine);
    registerOgreDegree(engine);
    registerOgreVector3(engine);
    registerOgreQuaternion(engine);
}

// register Ogre::Vector3
void registerOgreVector3(AngelScript::asIScriptEngine* engine)
{
    int r;

    // Register the object properties
    r = engine->RegisterObjectProperty("vector3", "float x", offsetof(Ogre::Vector3, x));
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectProperty("vector3", "float y", offsetof(Ogre::Vector3, y));
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectProperty("vector3", "float z", offsetof(Ogre::Vector3, z));
    ROR_ASSERT( r >= 0 );

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

    // r = engine->RegisterObjectMethod("vector3", "int opCmp(const vector3 &in) const",        asFUNCTION(Vector3Cmp), asCALL_CDECL_OBJFIRST); ROR_ASSERT( r >= 0 );

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
    r = engine->RegisterObjectMethod("vector3", "float crossProduct(const vector3 &in) const", asMETHOD(Vector3,crossProduct), asCALL_THISCALL);
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

    // Register the object properties
    r = engine->RegisterObjectProperty("quaternion", "float w", offsetof(Quaternion, w));
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectProperty("quaternion", "float x", offsetof(Quaternion, x));
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectProperty("quaternion", "float y", offsetof(Quaternion, y));
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectProperty("quaternion", "float z", offsetof(Quaternion, z));
    ROR_ASSERT( r >= 0 );
    // r = engine->RegisterObjectProperty("quaternion", "float ms_fEpsilon", offsetof(Quaternion, ms_fEpsilon)); ROR_ASSERT( r >= 0 );
    // r = engine->RegisterObjectProperty("quaternion", "quaternion ZERO", offsetof(Quaternion, ZERO)); ROR_ASSERT( r >= 0 );
    // r = engine->RegisterObjectProperty("quaternion", "quaternion IDENTITY", offsetof(Quaternion, IDENTITY)); ROR_ASSERT( r >= 0 );

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
    r = engine->RegisterObjectMethod("quaternion", "radian getRoll(bool) const", asMETHOD(Quaternion,getRoll), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "radian getPitch(bool) const", asMETHOD(Quaternion,getPitch), asCALL_THISCALL);
    ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("quaternion", "radian getYaw(bool) const", asMETHOD(Quaternion,getYaw), asCALL_THISCALL);
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
