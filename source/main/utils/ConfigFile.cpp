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
#include "Utils.h"

#include <OgreConfigFile.h>
#include <OgreString.h>
#include <OgreStringConverter.h>

using namespace RoR;

float ConfigFile::GetFloat(Ogre::String const& key, Ogre::String const& section, float defaultValue)
{
    return Ogre::StringConverter::parseReal(Ogre::ConfigFile::getSetting(key, section), defaultValue);
}

Ogre::ColourValue ConfigFile::GetColourValue(Ogre::String const& key, Ogre::String const& section, Ogre::ColourValue const& defaultValue)
{
    return Ogre::StringConverter::parseColourValue(Ogre::ConfigFile::getSetting(key, section), defaultValue);
}

int ConfigFile::GetInt(Ogre::String const& key, Ogre::String const& section, int defaultValue)
{
    return Ogre::StringConverter::parseInt(Ogre::ConfigFile::getSetting(key, section), defaultValue);
}

bool ConfigFile::GetBool(Ogre::String const& key, Ogre::String const& section, bool defaultValue)
{
    return Ogre::StringConverter::parseBool(Ogre::ConfigFile::getSetting(key, section), defaultValue);
}

Ogre::String ConfigFile::GetStringEx(Ogre::String const& key, Ogre::String const& section, Ogre::String const& defaultValue)
{
    auto setting = Ogre::ConfigFile::getSetting(key, section);
    if (setting.empty())
    {
        return defaultValue;
    }
    return RoR::Utils::SanitizeUtf8String(setting);
}

void ConfigFile::SetString(Ogre::String key, Ogre::String value, Ogre::String section /* = Ogre::StringUtil::BLANK */)
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
