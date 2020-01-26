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
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "RoRPrerequisites.h"
#include "ConfigFile.h"

#include <OgreConfigFile.h>
#include <OgreDataStream.h>
#include <OgreException.h>
#include <OgreString.h>

#include <cstdio>

/// TODO: Nonsense middle layer, only used by scripting/LocalStorage, to be removed in favor of JSON ~ 01/2020 only_a_ptr
class ImprovedConfigFile : public RoR::ConfigFile
{
public:
    ImprovedConfigFile() : separators("="), filename()
    {
        ConfigFile();
    }

    ~ImprovedConfigFile()
    {
    }

    void loadImprovedCfg()
    {
        ConfigFile::load(this->filename, this->resource_group_name, this->separators, /*trimWhitespace*/true);
    }

    bool hasSetting(Ogre::String key, Ogre::String section = "")
    {
        return (mSettingsPtr.find(section) != mSettingsPtr.end() && mSettingsPtr[section]->find(key) != mSettingsPtr[section]->end());
    }

    bool saveImprovedCfg()
    {
        Ogre::DataStreamPtr stream
            = Ogre::ResourceGroupManager::getSingleton().createResource(
                this->filename, this->resource_group_name, /*overwrite=*/true);

        const size_t BUF_LEN = 2000;
        char buf[BUF_LEN];
        SettingsBySection::iterator secIt;
        for (secIt = mSettingsPtr.begin(); secIt != mSettingsPtr.end(); secIt++)
        {
            if (secIt->first.size() > 0)
            {
                int num_chars = std::snprintf(buf, BUF_LEN, "[%s]\n", secIt->first.c_str());
                stream->write(buf, num_chars);
            }

            SettingsMultiMap::iterator setIt;
            for (setIt = secIt->second->begin(); setIt != secIt->second->end(); setIt++)
            {
                int num_chars = std::snprintf(buf, BUF_LEN, "%s%c%s\n", setIt->first.c_str(), separators[0], setIt->second.c_str());
                stream->write(buf, num_chars);
            }
        }
        return true;
    }

    void setSetting(Ogre::String key, Ogre::String value, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        SettingsMultiMap* set = mSettingsPtr[section];
        if (!set)
        {
            // new section
            set = new SettingsMultiMap();
            mSettingsPtr[section] = set;
        }
        if (set->count(key))
        // known key, delete old first
            set->erase(key);
        // add key
        set->insert(std::multimap<Ogre::String, Ogre::String>::value_type(key, value));
    }

    // type specific implementations
    Ogre::Radian getSettingRadian(Ogre::String key, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        return Ogre::StringConverter::parseAngle(GetStringEx(key, section));
    }

    void setSetting(Ogre::String key, Ogre::Radian value, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        setSetting(key, TOSTRING(value), section);
    }

    bool getSettingBool(Ogre::String key, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        return Ogre::StringConverter::parseBool(GetStringEx(key, section));
    }

    void setSetting(Ogre::String key, bool value, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        setSetting(key, TOSTRING(value), section);
    }

    Ogre::Real getSettingReal(Ogre::String key, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        return Ogre::StringConverter::parseReal(GetStringEx(key, section));
    }

    void setSetting(Ogre::String key, Ogre::Real value, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        setSetting(key, TOSTRING(value), section);
    }

    int getSettingInt(Ogre::String key, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        return Ogre::StringConverter::parseInt(GetStringEx(key, section));
    }

    void setSetting(Ogre::String key, int value, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        setSetting(key, TOSTRING(value), section);
    }

    unsigned int getSettingUnsignedInt(Ogre::String key, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        return Ogre::StringConverter::parseUnsignedInt(GetStringEx(key, section));
    }

    void setSetting(Ogre::String key, unsigned int value, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        setSetting(key, TOSTRING(value), section);
    }

    long getSettingLong(Ogre::String key, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        return Ogre::StringConverter::parseLong(GetStringEx(key, section));
    }

    void setSetting(Ogre::String key, long value, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        setSetting(key, TOSTRING(value), section);
    }

    unsigned long getSettingUnsignedLong(Ogre::String key, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        return Ogre::StringConverter::parseUnsignedLong(GetStringEx(key, section));
    }

    void setSetting(Ogre::String key, unsigned long value, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        setSetting(key, TOSTRING(value), section);
    }

    Ogre::Vector3 getSettingVector3(Ogre::String key, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        return Ogre::StringConverter::parseVector3(GetStringEx(key, section));
    }

    void setSetting(Ogre::String key, Ogre::Vector3 value, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        setSetting(key, TOSTRING(value), section);
    }

    Ogre::Matrix3 getSettingMatrix3(Ogre::String key, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        return Ogre::StringConverter::parseMatrix3(GetStringEx(key, section));
    }

    void setSetting(Ogre::String key, Ogre::Matrix3 value, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        setSetting(key, TOSTRING(value), section);
    }

    Ogre::Matrix4 getSettingMatrix4(Ogre::String key, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        return Ogre::StringConverter::parseMatrix4(GetStringEx(key, section));
    }

    void setSetting(Ogre::String key, Ogre::Matrix4 value, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        setSetting(key, TOSTRING(value), section);
    }

    Ogre::Quaternion getSettingQuaternion(Ogre::String key, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        return Ogre::StringConverter::parseQuaternion(GetStringEx(key, section));
    }

    void setSetting(Ogre::String key, Ogre::Quaternion value, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        setSetting(key, TOSTRING(value), section);
    }

    Ogre::ColourValue getSettingColorValue(Ogre::String key, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        return Ogre::StringConverter::parseColourValue(GetStringEx(key, section));
    }

    void setSetting(Ogre::String key, Ogre::ColourValue value, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        setSetting(key, TOSTRING(value), section);
    }

    Ogre::StringVector getSettingStringVector(Ogre::String key, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        return Ogre::StringConverter::parseStringVector(GetStringEx(key, section));
    }

    void setSetting(Ogre::String key, Ogre::StringVector value, Ogre::String section = Ogre::StringUtil::BLANK)
    {
        setSetting(key, TOSTRING(value), section);
    }

protected:
    Ogre::String separators;
    Ogre::String filename;
    Ogre::String resource_group_name;
};
