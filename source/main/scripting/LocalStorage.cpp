/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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

#include "Actor.h"
#include "Application.h"
#include "ContentManager.h"
#include "PlatformUtils.h"

using namespace RoR;


/* class that implements the localStorage interface for the scripts */
LocalStorage::LocalStorage(std::string fileName_in, const std::string& sectionName_in, const std::string &resource_group = RGN_CACHE)
{
    // inversed logic, better use a whitelist instead of a blacklist, so you are on the safe side ;) - tdev
    std::string allowedChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-";
    for (std::string::iterator it = fileName_in.begin() ; it < fileName_in.end() ; ++it){
        if ( allowedChars.find(*it) == std::string::npos )
            *it = '_';
    }

    sectionName = sectionName_in.substr(0, sectionName_in.find(".", 0));
    
    m_filename = fileName_in + ".asdata";
    m_resource_group = resource_group;
    this->separators = "=";
    loadDict();
    
    saved = true;
}

LocalStorage::~LocalStorage()
{
    // save everything
    saveDict();
}

void LocalStorage::copyFrom(LocalStoragePtr other)
{
    m_filename = other->getFilename();
    sectionName = other->getSection();
    SettingsBySection::iterator secIt;
    SettingsBySection osettings = other->getSettings();
    for (secIt = osettings.begin(); secIt!=osettings.end(); secIt++)
    {
        SettingsMultiMap::iterator setIt;
        for (setIt = secIt->second->begin(); setIt!=secIt->second->end(); setIt++)
        {
            setSetting(setIt->first, setIt->second, secIt->first);
        }
    }
}

void LocalStorage::changeSection(const std::string &section)
{
    sectionName = section.substr(0, section.find(".", 0));
}

// getters and setters
std::string LocalStorage::get(std::string key)
{
    std::string sec;
    parseKey(key, sec);
    return getString(key, sec);
}

void LocalStorage::set(std::string key, const std::string &value)
{
    std::string sec;
    parseKey(key, sec);
    setSetting(key, value, sec);
    saved = false;
}

int LocalStorage::getInt(std::string key)
{
    std::string sec;
    parseKey(key, sec);
    return getSettingInt(key, sec);
}

void LocalStorage::set(std::string key, const int value)
{
    std::string sec;
    parseKey(key, sec);
    setSetting(key, value, sec);
    saved = false;
}

float LocalStorage::getFloat(std::string key)
{
    std::string sec;
    parseKey(key, sec);
    return getSettingReal(key, sec);
}

void LocalStorage::set(std::string key, const float value)
{
    std::string sec;
    parseKey(key, sec);
    setSetting(key, value, sec);
    saved = false;
}

bool LocalStorage::getBool(std::string key)
{
    std::string sec;
    parseKey(key, sec);
    return getSettingBool(key, sec);
}

void LocalStorage::set(std::string key, const bool value)
{
    std::string sec;
    parseKey(key, sec);
    setSetting(key, value, sec);
    saved = false;
}

Ogre::Vector3 LocalStorage::getVector3(std::string key)
{
    std::string sec;
    parseKey(key, sec);
    return getSettingVector3(key, sec);
}

void LocalStorage::set(std::string key, const Ogre::Vector3 &value)
{
    std::string sec;
    parseKey(key, sec);
    setSetting(key, value, sec);
    saved = false;
}

Ogre::Quaternion LocalStorage::getQuaternion(std::string key)
{
    std::string sec;
    parseKey(key, sec);
    return getSettingQuaternion(key, sec);
}

void LocalStorage::set(std::string key, const Ogre::Quaternion &value)
{
    std::string sec;
    parseKey(key, sec);
    setSetting(key, value, sec);
    saved = false;
}

Ogre::Radian LocalStorage::getRadian(std::string key)
{
    std::string sec;
    parseKey(key, sec);
    return getSettingRadian(key, sec);
}

void LocalStorage::set(std::string key, const Ogre::Radian &value)
{
    std::string sec;
    parseKey(key, sec);
    setSetting(key, value, sec);
    saved = false;
}

Ogre::Degree LocalStorage::getDegree(std::string key)
{
    std::string sec;
    parseKey(key, sec);
    return Ogre::Degree(getSettingRadian(key, sec));
}

void LocalStorage::set(std::string key, const Ogre::Degree &value)
{
    std::string sec;
    parseKey(key, sec);
    setSetting(key, Ogre::Radian(value), sec);
    saved = false;
}

void LocalStorage::saveDict()
{
    if (this->saved)
    {
        return;
    }

    try
    {
        this->saveImprovedCfg(m_filename, RGN_CACHE);
        this->saved = true;
    }
    catch (std::exception& e)
    {
        RoR::LogFormat("[RoR|Scripting|LocalStorage]"
                       "Error saving file '%s' (resource group '%s'), message: '%s'",
                        m_filename.c_str(), RGN_CACHE, e.what());
    }
}

bool LocalStorage::loadDict()
{
    try
    {
        this->loadImprovedCfg(m_filename, RGN_CACHE);
    }
    catch (std::exception& e)
    {
        RoR::LogFormat("[RoR|Scripting|LocalStorage]"
                       "Error reading file '%s' (resource group '%s'), message: '%s'",
                        m_filename.c_str(), RGN_CACHE, e.what());
        return false;
    }
    saved = true;
    return true;
}

void LocalStorage::eraseKey(std::string key)
{
    std::string sec;
    parseKey(key, sec);
    if (mSettingsPtr.find(sec) != mSettingsPtr.end() && mSettingsPtr[sec]->find(key) != mSettingsPtr[sec]->end())
        if (mSettingsPtr[sec]->erase(key) > 0)
            saved = false;
}

void LocalStorage::deleteAll()
{
    // SLOG("This feature is not available yet");
}

void LocalStorage::parseKey(std::string& key, std::string &section)
{
    size_t dot = key.find(".", 0);
    if ( dot != std::string::npos )
    {
        section = key.substr(0, dot);

        if ( !section.length() )
            section = sectionName;

        key.erase(0, dot+1);
    }
    else
        section = sectionName;
}

bool LocalStorage::exists(std::string key)
{
    std::string sec;
    parseKey(key, sec);
    return hasSetting(key, sec);
}

