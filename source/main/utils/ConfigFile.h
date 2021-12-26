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
    @file   ConfigFile.h
    @date   06/2014
    @author Petr Ohlidal
*/

#pragma once

#include <OgreConfigFile.h>
#include <OgreColourValue.h>
#include <OgreString.h>

namespace RoR {

/// Adds direct parsing of custom types.
class ConfigFile: public Ogre::ConfigFile
{
public:

    Ogre::ColourValue getColourValue(Ogre::String const& key, Ogre::ColourValue const& defaultValue = Ogre::ColourValue())
    {
        return this->getColourValue(key, Ogre::StringUtil::BLANK, defaultValue);
    }

    Ogre::ColourValue getColourValue(Ogre::String const& key, Ogre::String const& section, Ogre::ColourValue const& defaultValue = Ogre::ColourValue());

    Ogre::Vector3 getVector3(Ogre::String const& key, Ogre::String const& section, Ogre::Vector3 const& defaultValue = Ogre::Vector3::ZERO);

    float getFloat(Ogre::String const& key, float defaultValue = 0.f)
    {
        return this->getFloat(key, Ogre::StringUtil::BLANK, defaultValue);
    }

    float getFloat(Ogre::String const& key, Ogre::String const& section, float defaultValue = 0.f);

    bool getBool(Ogre::String const& key, bool defaultValue = false)
    {
        return this->getBool(key, Ogre::StringUtil::BLANK, defaultValue);
    }

    bool getBool(Ogre::String const& key, Ogre::String const& section, bool defaultValue = false);

    int getInt(Ogre::String const& key, int defaultValue = 0)
    {
        return this->getInt(key, Ogre::StringUtil::BLANK, defaultValue);
    }

    int getInt(Ogre::String const& key, Ogre::String const& section, int defaultValue = 0);

    Ogre::String getString(Ogre::String const& key, Ogre::String const& section, Ogre::String const& defaultValue = "");

    void SetString(Ogre::String key, Ogre::String value, Ogre::String section = Ogre::StringUtil::BLANK);

    bool HasSection(std::string const & name);

private:
    //Block access to Ogre::ConfigFile::getSetting() - not UTF8 safe!
    Ogre::String getSetting(Ogre::String, Ogre::String);
    Ogre::String getSetting(Ogre::String, Ogre::String, Ogre::String);
};

} // namespace RoR
