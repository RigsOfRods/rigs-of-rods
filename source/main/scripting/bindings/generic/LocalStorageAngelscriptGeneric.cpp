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

#include "../../autowrapper/aswrappedcall.h"
#include "LocalStorage.h"
#include "AngelScriptBindings.h"
#include "wrappers/LocalStorageAngelscriptWrappers.h"

#include <angelscript.h>

using namespace AngelScript;
using namespace RoR;
using namespace LocalStorageAngelscriptWrappers;

void RoR::RegisterLocalStorageGeneric(asIScriptEngine *engine)
{
    LocalStorage::RegisterRefCountingObjectGeneric(engine, "LocalStorageClass");
    LocalStoragePtr::RegisterRefCountingObjectPtrGeneric(engine, "LocalStorageClassPtr", "LocalStorageClass");

    int r;
    r = engine->RegisterObjectBehaviour("LocalStorageClass", asBEHAVE_FACTORY, "LocalStorageClass@+ f(string, const string&in = \"common\", const string&in = \"Cache\")", WRAP_FN(LocalStorageFactory), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("LocalStorageClass", "void copyFrom(LocalStorageClassPtr@)", WRAP_MFN(LocalStorage, copyFrom), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void changeSection(const string &in)", WRAP_MFN(LocalStorage, changeSection), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorageClass", "string get(string)",                 WRAP_MFN_PR(LocalStorage, get, (std::string), std::string), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "string getString(string)",           WRAP_MFN_PR(LocalStorage, get, (std::string), std::string), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void set(string, const string &in)", WRAP_MFN_PR(LocalStorage, set, (std::string, const std::string&), void), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void setString(string, const string &in)", WRAP_MFN_PR(LocalStorage, set, (std::string, const std::string&), void), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorageClass", "float getFloat(string)",  WRAP_MFN_PR(LocalStorage, getFloat, (std::string), float), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void set(string, float)", WRAP_MFN_PR(LocalStorage, set, (std::string, const float), void), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void setFloat(string, float)", WRAP_MFN_PR(LocalStorage, set, (std::string, const float), void), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorageClass", "vector3 getVector3(string)",          WRAP_MFN_PR(LocalStorage, getVector3, (std::string), Ogre::Vector3), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void set(string, const vector3 &in)", WRAP_MFN_PR(LocalStorage, set, (std::string, const Ogre::Vector3&), void), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void setVector3(string, const vector3 &in)", WRAP_MFN_PR(LocalStorage, set, (std::string, const Ogre::Vector3&), void), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorageClass", "radian getRadian(string)",           WRAP_MFN_PR(LocalStorage, getRadian, (std::string), Ogre::Radian), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void set(string, const radian &in)", WRAP_MFN_PR(LocalStorage, set, (std::string, const Ogre::Radian&), void), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void setRadian(string, const radian &in)", WRAP_MFN_PR(LocalStorage, set, (std::string, const Ogre::Radian&), void), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorageClass", "degree getDegree(string)",           WRAP_MFN_PR(LocalStorage, getDegree, (std::string), Ogre::Degree), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void set(string, const degree &in)", WRAP_MFN_PR(LocalStorage, set, (std::string, const Ogre::Degree&), void), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void setDegree(string, const degree &in)", WRAP_MFN_PR(LocalStorage, set, (std::string, const Ogre::Degree&), void), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorageClass", "quaternion getQuaternion(string)",       WRAP_MFN_PR(LocalStorage, getQuaternion, (std::string), Ogre::Quaternion), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void set(string, const quaternion &in)", WRAP_MFN_PR(LocalStorage, set, (std::string, const Ogre::Quaternion&), void), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void setQuaternion(string, const quaternion &in)", WRAP_MFN_PR(LocalStorage, set, (std::string, const Ogre::Quaternion&), void), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorageClass", "bool getBool(string)",             WRAP_MFN_PR(LocalStorage, getBool, (std::string), bool), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void set(string, const bool &in)", WRAP_OBJ_FIRST(LocalStorage_setBool), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void setBool(string, const bool &in)", WRAP_OBJ_FIRST(LocalStorage_setBool), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorageClass", "int getInt(string)",     WRAP_MFN_PR(LocalStorage, getInt, (std::string), int), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "int getInteger(string)", WRAP_MFN_PR(LocalStorage, getInt, (std::string), int), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void set(string, int)",  WRAP_MFN_PR(LocalStorage, set, (std::string, const int), void), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void setInt(string, int)",  WRAP_MFN_PR(LocalStorage, set, (std::string, const int), void), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void setInteger(string, int)",  WRAP_MFN_PR(LocalStorage, set, (std::string, const int), void), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorageClass", "void save()",   WRAP_MFN(LocalStorage, saveDict), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "bool reload()", WRAP_MFN(LocalStorage, loadDict), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectMethod("LocalStorageClass", "bool exists(string &in) const", WRAP_OBJ_FIRST(LocalStorage_exists), asCALL_GENERIC); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectMethod("LocalStorageClass", "void delete(string &in)",       WRAP_OBJ_FIRST(LocalStorage_eraseKey), asCALL_GENERIC); ROR_ASSERT( r >= 0 );

}
