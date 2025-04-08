/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2016 Petr Ohlidal

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

/** 
    @file   ConfigFile.cpp
    @date   06/2014
    @author Petr Ohlidal
*/

#include "ConfigFile.h"

#include "Application.h"
#include "Console.h"
#include "Utils.h"

#include <OgreConfigFile.h>
#include <OgreString.h>
#include <OgreStringConverter.h>

using namespace RoR;

float ConfigFile::getFloat(Ogre::String const& key, Ogre::String const& section, float defaultValue)
{
    return Ogre::StringConverter::parseReal(Ogre::ConfigFile::getSetting(key, section), defaultValue);
}

Ogre::ColourValue ConfigFile::getColourValue(Ogre::String const& key, Ogre::String const& section, Ogre::ColourValue const& defaultValue)
{
    Ogre::ColourValue result;
    Ogre::String value = Ogre::ConfigFile::getSetting(key, section);
    if (Ogre::StringConverter::parse(value, result))
    {
        return result;
    }
    else
    {
        this->logMessage(
            fmt::format("Could not parse '{}/{}' ({}) as color, format must be 'R G B A' or 'R G B', falling back to '{}'",
                section, key, value, Ogre::StringConverter::toString(defaultValue)));
        return defaultValue;
    }
}

Ogre::Vector3 ConfigFile::getVector3(Ogre::String const& key, Ogre::String const& section, Ogre::Vector3 const& defaultValue)
{
    Ogre::Vector3 result;
    Ogre::String value = Ogre::ConfigFile::getSetting(key, section);
    if (Ogre::StringConverter::parse(value, result))
    {
        return result;
    }
    else
    {
        this->logMessage(
            fmt::format("Could not parse '{}/{}' ({}) as vector3, format must be 'X Y Z', falling back to '{}'",
                section, key, value, Ogre::StringConverter::toString(defaultValue)));
        return defaultValue;
    }
}

int ConfigFile::getInt(Ogre::String const& key, Ogre::String const& section, int defaultValue)
{
    return Ogre::StringConverter::parseInt(Ogre::ConfigFile::getSetting(key, section), defaultValue);
}

bool ConfigFile::getBool(Ogre::String const& key, Ogre::String const& section, bool defaultValue)
{
    return Ogre::StringConverter::parseBool(Ogre::ConfigFile::getSetting(key, section), defaultValue);
}

Ogre::String ConfigFile::getString(Ogre::String const& key, Ogre::String const& section, Ogre::String const& defaultValue)
{
    auto setting = Ogre::ConfigFile::getSetting(key, section);
    if (setting.empty())
    {
        return defaultValue;
    }
    return SanitizeUtf8String(setting);
}

void ConfigFile::SetString(Ogre::String key, Ogre::String value, Ogre::String section /* = Ogre::BLANKSTRING */)
{
    SettingsMultiMap* set = mSettingsPtr[section];
    if (!set)
    {
        // new section
        set = new SettingsMultiMap();
        mSettingsPtr[section] = set;
    }
    if (set->count(key))
    {
        // known key, delete old first
        set->erase(key);
    }
    // add key
    set->insert(std::multimap<Ogre::String, Ogre::String>::value_type(key, value));
}

bool ConfigFile::HasSection(std::string const & name)
{
    // This is the only way to check existence of section
    //  without either an OGRE exception being logged or using deprecated API.
    return this->getSettingsBySection().find(name) != this->getSettingsBySection().end();
}

bool ConfigFile::HasSetting(std::string const& name, std::string const& key)
{
    auto section = this->getSettingsBySection().find(name);
    if (section == this->getSettingsBySection().end())
    {
        return false;
    }
    return section->second.find(key) != section->second.end();
}

void ConfigFile::logMessage(std::string const & msg)
{
    // If filename is specified, log "filename: message", otherwise just "message".
    LOG(fmt::format("{}{}{}", m_log_filename, (m_log_filename == "" ? "" : ": "), msg));
}
