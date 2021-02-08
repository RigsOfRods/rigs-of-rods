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

    Ogre::ColourValue GetColourValue(Ogre::String const& key, Ogre::ColourValue const& defaultValue = Ogre::ColourValue())
    {
        return this->GetColourValue(key, Ogre::BLANKSTRING, defaultValue);
    }

    Ogre::ColourValue GetColourValue(Ogre::String const& key, Ogre::String const& section, Ogre::ColourValue const& defaultValue = Ogre::ColourValue());

    float GetFloat(Ogre::String const& key, float defaultValue = 0.f)
    {
        return this->GetFloat(key, Ogre::BLANKSTRING, defaultValue);
    }

    float GetFloat(Ogre::String const& key, Ogre::String const& section, float defaultValue = 0.f);

    bool GetBool(Ogre::String const& key, bool defaultValue = false)
    {
        return this->GetBool(key, Ogre::BLANKSTRING, defaultValue);
    }

    bool GetBool(Ogre::String const& key, Ogre::String const& section, bool defaultValue = false);

    int GetInt(Ogre::String const& key, int defaultValue = 0)
    {
        return this->GetInt(key, Ogre::BLANKSTRING, defaultValue);
    }

    int GetInt(Ogre::String const& key, Ogre::String const& section, int defaultValue = 0);

    Ogre::String GetString(Ogre::String const& key, Ogre::String const& defaultValue = "")
    {
        return this->GetStringEx(key, Ogre::BLANKSTRING, defaultValue);
    }

    Ogre::String GetStringEx(Ogre::String const& key, Ogre::String const& section, Ogre::String const& defaultValue = "");

    void SetString(Ogre::String key, Ogre::String value, Ogre::String section = Ogre::BLANKSTRING);

    bool HasSection(std::string const & name);

private:
    //Block access to Ogre::ConfigFile::getSetting() - not UTF8 safe!
    Ogre::String getSetting(const Ogre::String& key, const Ogre::String& section = Ogre::BLANKSTRING, const Ogre::String& defaultValue = Ogre::BLANKSTRING) const;
};

} // namespace RoR
