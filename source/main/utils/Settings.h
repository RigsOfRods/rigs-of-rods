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
    @description This is a global configuration hub.
        Values from both config file and command line are propagated here
            and accessed ad-hoc by macros like SSETTING(), BSETTING() etc...
            See 'ProcessCommandLine()' for details.
        NOTE: Since 09/2016, this class is being superceded by GlobalEnvironment.
            See 'ParseGlobalVarSetting()' for details.
*/

#pragma once
#ifndef __Settings_H_
#define __Settings_H_

#include "RoRPrerequisites.h"

#include "Singleton.h"

namespace RoR{
namespace System {

std::string    GetParentDirectory(const char* src_buff);
int            DetectBasePaths();

} // namespace System
} // namespace RoR

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
	
	void loadSettings(Ogre::String configFile, bool overwrite=false);
	void saveSettings();
	void saveSettings(Ogre::String configFile);

    /// Process command line arguments into settings.
    void ProcessCommandLine(int argc, char *argv[]);

    /// Process and erase settings which propagate to global vars.
    /// @return True if the value was processed, false if it remains in settings.
    bool ParseGlobalVarSetting(std::string const & name, std::string const & value);

    static bool SetupAllPaths();

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

	static Settings* myInstance;

	// members
	// TODO: use wide char / UTFString ...
	typedef std::map<Ogre::String, Ogre::String> settings_map_t;
	settings_map_t settings;
};

#endif // __Settings_H_
