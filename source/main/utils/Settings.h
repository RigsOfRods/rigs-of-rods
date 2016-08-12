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
    @file
    @date   4th of January 2009
    @author Thomas Fischer
*/

#pragma once
#ifndef __Settings_H_
#define __Settings_H_

#include "RoRPrerequisites.h"

#include "Singleton.h"

// some shortcuts to improve code readability
#define SETTINGS          Settings::getSingleton()
#define SSETTING(x, y)    Settings::getSingleton().getSetting(x, y)            //<! get string setting
#define UTFSSETTING(x, y) Settings::getSingleton().getUTFSetting(x, y)         //<! get UTF string setting
#define BSETTING(x, y)    Settings::getSingleton().getBooleanSetting(x, y)     //<! get boolean setting
#define ISETTING(x, y)    Settings::getSingleton().getIntegerSetting(x, y)     //<! get int setting
#define FSETTING(x, y)    Settings::getSingleton().getFloatSetting(x, y)       //<! get float setting

class Settings : public RoRSingleton<Settings>, public ZeroedMemoryAllocator
{
	friend class RoRSingleton<Settings>;

public:

	Ogre::String getSetting(Ogre::String key, Ogre::String defaultValue);
	Ogre::UTFString getUTFSetting(Ogre::UTFString key, Ogre::UTFString defaultValue);
	bool getBooleanSetting(Ogre::String key, bool defaultValue);
	float getFloatSetting(Ogre::String key, float defaultValue);
	int getIntegerSetting(Ogre::String key, int defaultValue);
	
	Ogre::String getSettingScriptSafe(const Ogre::String &key);
	void setSettingScriptSafe(const Ogre::String &key, const Ogre::String &value);

	void setSetting(Ogre::String key, Ogre::String value);
	void setUTFSetting(Ogre::UTFString key, Ogre::UTFString value);
	
	bool setupPaths();
	void loadSettings(Ogre::String configFile, bool overwrite=false);
	void saveSettings();
	void saveSettings(Ogre::String configFile);

	void checkGUID();
	void createGUID();

	int GetFlaresMode(int default_value = 2);
	int GetGearBoxMode(int default_value = 0);

#ifdef USE_ANGELSCRIPT
	// we have to add this to be able to use the class as reference inside scripts
	void addRef(){};
	void release(){};
#endif

	std::map<Ogre::String, Ogre::String> GetSettingMap()
	{
		return settings;
	}
protected:

	Settings();
	~Settings();
	Settings(const Settings&);
	Settings& operator= (const Settings&);
	static Settings* myInstance;

	// members
	// TODO: use wide char / UTFString ...
	typedef std::map<Ogre::String, Ogre::String> settings_map_t;
	settings_map_t settings;

	// methods
	void path_descend(char* path);
	void path_add(char* path, const char* dirname);

	bool get_system_paths(char *program_path, char *user_path);
	int generateBinaryHash();

	// ------------------------------------------------------------
	// Cached config data

	int m_flares_mode; // -1: unknown, -2: default, 0+: mode ID
	int m_gearbox_mode;
};

#endif // __Settings_H_
