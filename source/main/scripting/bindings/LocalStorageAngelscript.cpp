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

#include "LocalStorage.h"
#include "AngelScriptBindings.h"
#include <angelscript.h>

using namespace AngelScript;

void scriptLocalStorageFactory_Generic(asIScriptGeneric *gen)
{
    std::string filename	= **(std::string**)gen->GetAddressOfArg(0);
    std::string sectionname = **(std::string**)gen->GetAddressOfArg(1);
    
    *(LocalStorage**)gen->GetAddressOfReturnLocation() = new LocalStorage(gen->GetEngine(), filename, sectionname);
}

void scriptLocalStorageFactory2_Generic(asIScriptGeneric *gen)
{
    std::string filename	= **(std::string**)gen->GetAddressOfArg(0);	
    *(LocalStorage**)gen->GetAddressOfReturnLocation() = new LocalStorage(gen->GetEngine(), filename, "common");
}

void scriptLocalStorageFactory3_Generic(asIScriptGeneric *gen)
{	
    *(LocalStorage**)gen->GetAddressOfReturnLocation() = new LocalStorage(gen->GetEngine());
}


void RoR::RegisterLocalStorage(asIScriptEngine *engine)
{
    int r;

    r = engine->RegisterObjectType("LocalStorage", sizeof(LocalStorage), asOBJ_REF | asOBJ_GC); ROR_ASSERT( r >= 0 );
    // Use the generic interface to construct the object since we need the engine pointer, we could also have retrieved the engine pointer from the active context
    r = engine->RegisterObjectBehaviour("LocalStorage", asBEHAVE_FACTORY, "LocalStorage@ f(const string &in, const string &in)", asFUNCTION(scriptLocalStorageFactory_Generic), asCALL_GENERIC); ROR_ASSERT( r>= 0 );
    r = engine->RegisterObjectBehaviour("LocalStorage", asBEHAVE_FACTORY, "LocalStorage@ f(const string &in)", asFUNCTION(scriptLocalStorageFactory2_Generic), asCALL_GENERIC); ROR_ASSERT( r>= 0 );
    r = engine->RegisterObjectBehaviour("LocalStorage", asBEHAVE_FACTORY, "LocalStorage@ f()", asFUNCTION(scriptLocalStorageFactory3_Generic), asCALL_GENERIC); ROR_ASSERT( r>= 0 );
    r = engine->RegisterObjectBehaviour("LocalStorage", asBEHAVE_ADDREF, "void f()",  asMETHOD(LocalStorage,AddRef), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("LocalStorage", asBEHAVE_RELEASE, "void f()", asMETHOD(LocalStorage,Release), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorage", "LocalStorage &opAssign(LocalStorage &in)", asMETHODPR(LocalStorage, operator=, (LocalStorage &), LocalStorage&), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorage", "void changeSection(const string &in)",     asMETHODPR(LocalStorage,changeSection,(const std::string&), void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorage", "string get(string &in)",                 asMETHODPR(LocalStorage,get,(std::string&), std::string), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorage", "string getString(string &in)",           asMETHODPR(LocalStorage,get,(std::string&), std::string), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorage", "void set(string &in, const string &in)", asMETHODPR(LocalStorage,set,(std::string&, const std::string&),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorage", "void setString(string &in, const string &in)", asMETHODPR(LocalStorage,set,(std::string&, const std::string&),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorage", "float getFloat(string &in)",  asMETHODPR(LocalStorage,getFloat,(std::string&), float), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorage", "void set(string &in, float)", asMETHODPR(LocalStorage,set,(std::string&, const float),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorage", "void setFloat(string &in, float)", asMETHODPR(LocalStorage,set,(std::string&, const float),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorage", "vector3 getVector3(string &in)",          asMETHODPR(LocalStorage,getVector3,(std::string&), Ogre::Vector3), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorage", "void set(string &in, const vector3 &in)", asMETHODPR(LocalStorage,set,(std::string&, const Ogre::Vector3&),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorage", "void setVector3(string &in, const vector3 &in)", asMETHODPR(LocalStorage,set,(std::string&, const Ogre::Vector3&),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorage", "radian getRadian(string &in)",           asMETHODPR(LocalStorage,getRadian,(std::string&), Ogre::Radian), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorage", "void set(string &in, const radian &in)", asMETHODPR(LocalStorage,set,(std::string&, const Ogre::Radian&),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorage", "void setRadian(string &in, const radian &in)", asMETHODPR(LocalStorage,set,(std::string&, const Ogre::Radian&),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorage", "degree getDegree(string &in)",           asMETHODPR(LocalStorage,getDegree,(std::string&), Ogre::Degree), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorage", "void set(string &in, const degree &in)", asMETHODPR(LocalStorage,set,(std::string&, const Ogre::Degree&),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorage", "void setDegree(string &in, const degree &in)", asMETHODPR(LocalStorage,set,(std::string&, const Ogre::Degree&),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorage", "quaternion getQuaternion(string &in)",       asMETHODPR(LocalStorage,getQuaternion,(std::string&), Ogre::Quaternion), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorage", "void set(string &in, const quaternion &in)", asMETHODPR(LocalStorage,set,(std::string&, const Ogre::Quaternion&),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorage", "void setQuaternion(string &in, const quaternion &in)", asMETHODPR(LocalStorage,set,(std::string&, const Ogre::Quaternion&),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorage", "bool getBool(string &in)",             asMETHODPR(LocalStorage,getBool,(std::string&), bool), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorage", "void set(string &in, const bool &in)", asMETHODPR(LocalStorage,set,(std::string&, const bool),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorage", "void setBool(string &in, const bool &in)", asMETHODPR(LocalStorage,set,(std::string&, const bool),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorage", "int getInt(string &in)",     asMETHODPR(LocalStorage,getInt,(std::string&), int), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorage", "int getInteger(string &in)", asMETHODPR(LocalStorage,getInt,(std::string&), int), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorage", "void set(string &in, int)",  asMETHODPR(LocalStorage,set,(std::string&, const int),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorage", "void setInt(string &in, int)",  asMETHODPR(LocalStorage,set,(std::string&, const int),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorage", "void setInteger(string &in, int)",  asMETHODPR(LocalStorage,set,(std::string&, const int),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorage", "void save()",   asMETHOD(LocalStorage,saveDict), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorage", "bool reload()", asMETHOD(LocalStorage,loadDict), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorage", "bool exists(string &in) const", asMETHOD(LocalStorage,exists), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorage", "void delete(string &in)",       asMETHOD(LocalStorage,eraseKey), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

    // Register GC behaviours
    r = engine->RegisterObjectBehaviour("LocalStorage", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(LocalStorage,GetRefCount), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("LocalStorage", asBEHAVE_SETGCFLAG, "void f()",  asMETHOD(LocalStorage,SetGCFlag), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("LocalStorage", asBEHAVE_GETGCFLAG, "bool f()",  asMETHOD(LocalStorage,GetGCFlag), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("LocalStorage", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(LocalStorage,EnumReferences), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour("LocalStorage", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(LocalStorage,ReleaseAllReferences), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

}
