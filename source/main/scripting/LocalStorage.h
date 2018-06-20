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

#pragma once

#include "RoRPrerequisites.h"
#include <angelscript.h>
#include "ImprovedConfigFile.h"

void registerLocalStorage(AngelScript::asIScriptEngine* engine);
void scriptLocalStorageFactory_Generic(AngelScript::asIScriptGeneric* gen);
void scriptLocalStorageFactory2_Generic(AngelScript::asIScriptGeneric* gen);
void scriptLocalStorageFactory3_Generic(AngelScript::asIScriptGeneric* gen);

/**
 *  @brief A class that allows scripts to store data persistently
 */
class LocalStorage : public ImprovedConfigFile
{
public:
    // Memory management
    void AddRef() const;
    void Release() const;

    LocalStorage(AngelScript::asIScriptEngine* engine, std::string fileName_in, const std::string& sectionName_in);
    LocalStorage(AngelScript::asIScriptEngine* engine_in);
    ~LocalStorage();

    LocalStorage& operator =(LocalStorage& other);
    void changeSection(const std::string& section);

    std::string get(std::string& key);
    void set(std::string& key, const std::string& value);

    int getInt(std::string& key);
    void set(std::string& key, const int value);

    float getFloat(std::string& key);
    void set(std::string& key, const float value);

    bool getBool(std::string& key);
    void set(std::string& key, const bool value);

    Ogre::Vector3 getVector3(std::string& key);
    void set(std::string& key, const Ogre::Vector3& value);

    Ogre::Quaternion getQuaternion(std::string& key);
    void set(std::string& key, const Ogre::Quaternion& value);

    Ogre::Radian getRadian(std::string& key);
    void set(std::string& key, const Ogre::Radian& value);

    Ogre::Degree getDegree(std::string& key);
    void set(std::string& key, const Ogre::Degree& value);

    void saveDict();
    // int extendDict();
    bool loadDict();

    // removes a key and its associated value
    void eraseKey(std::string& key);

    // Returns true if the key is set
    bool exists(std::string& key);

    // Deletes all keys
    void deleteAll();

    // parses a key
    void parseKey(std::string& key, std::string& section);

    // Garbage collections behaviours
    int GetRefCount();
    void SetGCFlag();
    bool GetGCFlag();
    void EnumReferences(AngelScript::asIScriptEngine* engine);
    void ReleaseAllReferences(AngelScript::asIScriptEngine* engine);

    SettingsBySection getSettings() { return mSettingsPtr; }
    std::string getFilename() { return filename; }
    std::string getSection() { return sectionName; }

protected:
    bool saved;
    std::string sectionName;

    // Our properties
    AngelScript::asIScriptEngine* engine;
    static int refCount;
    bool cgflag;
};

