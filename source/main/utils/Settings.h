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


/// @file
/// @date   4th of January 2009
/// @author Thomas Fischer
/// @description This is a global configuration hub.
///     Values from both config file and command line are propagated here
///     and converted to GVars - see 'ParseGlobalVarSetting()' for details.
///     Entries without corresponding GVar can be read
///     by ad-hoc by macros like SSETTING(), BSETTING() etc...


#pragma once

#include "RoRPrerequisites.h"

#include "Singleton.h"

namespace RoR {

void ShowCommandLineUsage();
void ShowVersion();

} // namespace RoR

// ---------------------
// Config value strings

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

    void LoadRoRCfg(); // Reads GVars

    void SaveSettings(); // Writes GVars

    /// Process command line arguments into settings.
    void ProcessCommandLine(int argc, char* argv[]);

    /// Process and erase settings which propagate to global vars.
    /// @return True if the value was processed, false if it remains in settings.
    bool ParseGlobalVarSetting(std::string const& name, std::string const& value);

    static bool SetupAllPaths();

protected:

    static Settings* myInstance;
};
