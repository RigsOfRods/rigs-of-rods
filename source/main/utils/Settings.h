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

namespace RoR {
namespace System {

void GetParentDirectory(char* dst_buf, const char* src_buff);
int DetectBasePaths();

} // namespace System

void ShowCommandLineUsage();
void ShowVersion();

} // namespace RoR

// some shortcuts to improve code readability
#define SETTINGS          Settings::getSingleton()
#define SSETTING(x, y)    Settings::getSingleton().getSetting(x, y)            //<! get string setting
#define UTFSSETTING(x, y) Settings::getSingleton().getUTFSetting(x, y)         //<! get UTF string setting
#define BSETTING(x, y)    Settings::getSingleton().getBooleanSetting(x, y)     //<! get boolean setting
#define ISETTING(x, y)    Settings::getSingleton().getIntegerSetting(x, y)     //<! get int setting
#define FSETTING(x, y)    Settings::getSingleton().getFloatSetting(x, y)       //<! get float setting

// ---------------------
// Config value strings

extern const char* CONF_GFX_SHADOW_TEX;
extern const char* CONF_GFX_SHADOW_PSSM;
extern const char* CONF_GFX_SHADOW_NONE;

extern const char* CONF_EXTCAM_PITCHING;
extern const char* CONF_EXTCAM_STATIC;
extern const char* CONF_EXTCAM_NONE;

extern const char* CONF_TEXFILTER_BILI;
extern const char* CONF_TEXFILTER_TRILI;
extern const char* CONF_TEXFILTER_ANISO;

extern const char* CONF_VEGET_NONE;
extern const char* CONF_VEGET_20PERC;
extern const char* CONF_VEGET_50PERC;
extern const char* CONF_VEGET_FULL;

extern const char* CONF_GEARBOX_AUTO;
extern const char* CONF_GEARBOX_SEMIAUTO;
extern const char* CONF_GEARBOX_MANUAL;
extern const char* CONF_GEARBOX_MAN_STICK;
extern const char* CONF_GEARBOX_MAN_RANGES;

extern const char* CONF_FLARES_NONE;
extern const char* CONF_FLARES_NO_LIGHT;
extern const char* CONF_FLARES_CURR_HEAD;
extern const char* CONF_FLARES_ALL_HEADS;
extern const char* CONF_FLARES_ALL_LIGHTS;

extern const char* CONF_WATER_BASIC;
extern const char* CONF_WATER_REFLECT;
extern const char* CONF_WATER_FULL_FAST;
extern const char* CONF_WATER_FULL_HQ;
extern const char* CONF_WATER_HYDRAX;

extern const char* CONF_SKY_CAELUM;
extern const char* CONF_SKY_SKYX;
extern const char* CONF_SKY_SANDSTORM;

extern const char* CONF_INPUT_GRAB_DYNAMIC;
extern const char* CONF_INPUT_GRAB_NONE;
extern const char* CONF_INPUT_GRAB_ALL;

// ---------------------
// Config string -> GVar

void App__SetIoInputGrabMode(std::string const& s);
void App__SetShadowTech(std::string const& s);
void App__SetExtcamMode(std::string const& s);
void App__SetTexFiltering(std::string const& s);
void App__SetVegetationMode(std::string const& s);
void App__SetSimGearboxMode(std::string const& s);
void App__SetGfxFlaresMode(std::string const& s);
void App__SetGfxWaterMode(std::string const& s);
void App__SetGfxSkyMode(std::string const& s);

class Settings : public RoRSingleton<Settings>, public ZeroedMemoryAllocator
{
    friend class RoRSingleton<Settings>;

public:

    Ogre::String getSetting(Ogre::String key, Ogre::String defaultValue);
    Ogre::UTFString getUTFSetting(Ogre::UTFString key, Ogre::UTFString defaultValue);
    bool getBooleanSetting(Ogre::String key, bool defaultValue);
    float getFloatSetting(Ogre::String key, float defaultValue);
    int getIntegerSetting(Ogre::String key, int defaultValue);

    Ogre::String getSettingScriptSafe(const Ogre::String& key);
    void setSettingScriptSafe(const Ogre::String& key, const Ogre::String& value);

    void setSetting(Ogre::String key, Ogre::String value);
    void setUTFSetting(Ogre::UTFString key, Ogre::UTFString value);

    void LoadRoRCfg(); // Reads GVars

    void SaveSettings();

    /// Process command line arguments into settings.
    void ProcessCommandLine(int argc, char* argv[]);

    /// Process and erase settings which propagate to global vars.
    /// @return True if the value was processed, false if it remains in settings.
    bool ParseGlobalVarSetting(std::string const& name, std::string const& value);

    static bool SetupAllPaths();

#ifdef USE_ANGELSCRIPT
    // we have to add this to be able to use the class as reference inside scripts
    void addRef()
    {
    };

    void release()
    {
    };
#endif

    std::map<Ogre::String, Ogre::String> GetSettingMap()
    {
        return settings;
    }

protected:

    // Helpers
    void SetMpNetworkEnable(bool enable);
    void SetGfxFovExternal(float fov);
    void SetGfxFovInternal(float fov);

    static Settings* myInstance;

    // members
    // TODO: use wide char / UTFString ...
    typedef std::map<Ogre::String, Ogre::String> settings_map_t;
    settings_map_t settings;

    // Cached config values
    bool m_network_enable;
    float m_fov_internal;
    float m_fov_external;
};

#endif // __Settings_H_
