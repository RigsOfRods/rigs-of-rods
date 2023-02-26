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
using namespace RoR;

static LocalStorage* LocalStorageFactory(std::string filename, const std::string& section_name, const std::string& rg_name)
{
    return new LocalStorage(filename, section_name, rg_name);
}

void RoR::RegisterLocalStorage(asIScriptEngine *engine)
{
    LocalStorage::RegisterRefCountingObject(engine, "LocalStorageClass");
    LocalStoragePtr::RegisterRefCountingObjectPtr(engine, "LocalStorageClassPtr", "LocalStorageClass");

    int r;
    r = engine->RegisterObjectBehaviour("LocalStorageClass", asBEHAVE_FACTORY, "LocalStorageClass@+ f(string, const string&in = \"common\", const string&in = \"Cache\")", asFUNCTION(LocalStorageFactory), asCALL_CDECL); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("LocalStorageClass", "void copyFrom(LocalStorageClassPtr@)", asMETHOD(LocalStorage,copyFrom), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void changeSection(const string &in)", asMETHOD(LocalStorage,changeSection), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorageClass", "string get(string)",                 asMETHODPR(LocalStorage,get,(std::string), std::string), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "string getString(string)",           asMETHODPR(LocalStorage,get,(std::string), std::string), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void set(string, const string &in)", asMETHODPR(LocalStorage,set,(std::string, const std::string&),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void setString(string, const string &in)", asMETHODPR(LocalStorage,set,(std::string, const std::string&),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorageClass", "float getFloat(string)",  asMETHODPR(LocalStorage,getFloat,(std::string), float), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void set(string, float)", asMETHODPR(LocalStorage,set,(std::string, const float),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void setFloat(string, float)", asMETHODPR(LocalStorage,set,(std::string, const float),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorageClass", "vector3 getVector3(string)",          asMETHODPR(LocalStorage,getVector3,(std::string), Ogre::Vector3), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void set(string, const vector3 &in)", asMETHODPR(LocalStorage,set,(std::string, const Ogre::Vector3&),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void setVector3(string, const vector3 &in)", asMETHODPR(LocalStorage,set,(std::string, const Ogre::Vector3&),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorageClass", "radian getRadian(string)",           asMETHODPR(LocalStorage,getRadian,(std::string), Ogre::Radian), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void set(string, const radian &in)", asMETHODPR(LocalStorage,set,(std::string, const Ogre::Radian&),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void setRadian(string, const radian &in)", asMETHODPR(LocalStorage,set,(std::string, const Ogre::Radian&),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorageClass", "degree getDegree(string)",           asMETHODPR(LocalStorage,getDegree,(std::string), Ogre::Degree), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void set(string, const degree &in)", asMETHODPR(LocalStorage,set,(std::string, const Ogre::Degree&),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void setDegree(string, const degree &in)", asMETHODPR(LocalStorage,set,(std::string, const Ogre::Degree&),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorageClass", "quaternion getQuaternion(string)",       asMETHODPR(LocalStorage,getQuaternion,(std::string), Ogre::Quaternion), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void set(string, const quaternion &in)", asMETHODPR(LocalStorage,set,(std::string, const Ogre::Quaternion&),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void setQuaternion(string, const quaternion &in)", asMETHODPR(LocalStorage,set,(std::string, const Ogre::Quaternion&),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorageClass", "bool getBool(string)",             asMETHODPR(LocalStorage,getBool,(std::string), bool), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void set(string, const bool &in)", asMETHODPR(LocalStorage,set,(std::string, const bool),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void setBool(string, const bool &in)", asMETHODPR(LocalStorage,set,(std::string, const bool),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorageClass", "int getInt(string)",     asMETHODPR(LocalStorage,getInt,(std::string), int), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "int getInteger(string)", asMETHODPR(LocalStorage,getInt,(std::string), int), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void set(string, int)",  asMETHODPR(LocalStorage,set,(std::string, const int),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void setInt(string, int)",  asMETHODPR(LocalStorage,set,(std::string, const int),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void setInteger(string, int)",  asMETHODPR(LocalStorage,set,(std::string, const int),void), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorageClass", "void save()",   asMETHOD(LocalStorage,saveDict), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "bool reload()", asMETHOD(LocalStorage,loadDict), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorageClass", "bool exists(string &in) const", asMETHOD(LocalStorage,exists), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void delete(string &in)",       asMETHOD(LocalStorage,eraseKey), asCALL_THISCALL); ROR_ASSERT( r >= 0 );

}
