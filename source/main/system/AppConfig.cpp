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

#include "Actor.h"
#include "Application.h"
#include "Console.h"
#include "ContentManager.h" // RGN_CONFIG
#include "ErrorUtils.h"
#include "Language.h"
#include "PlatformUtils.h"
#include "Utils.h"

#include <Ogre.h>
#include <sstream>

using namespace Ogre;
using namespace RoR;

#define CONFIG_FILE_NAME "RoR.cfg"

// --------------------------------
// Config file strings and helpers

const char* CONF_GFX_SHADOW_PSSM    = "Parallel-split Shadow Maps";
const char* CONF_GFX_SHADOW_NONE    = "No shadows (fastest)";

const char* CONF_EXTCAM_PITCHING    = "Pitching";
const char* CONF_EXTCAM_STATIC      = "Static";
const char* CONF_EXTCAM_NONE        = "None";

const char* CONF_TEXFILTER_NONE     = "None (fastest)";
const char* CONF_TEXFILTER_BILI     = "Bilinear";
const char* CONF_TEXFILTER_TRILI    = "Trilinear";
const char* CONF_TEXFILTER_ANISO    = "Anisotropic (best looking)";

const char* CONF_VEGET_NONE         = "None (fastest)";
const char* CONF_VEGET_20PERC       = "20%";
const char* CONF_VEGET_50PERC       = "50%";
const char* CONF_VEGET_FULL         = "Full (best looking, slower)";

const char* CONF_GEARBOX_AUTO       = "Automatic shift";
const char* CONF_GEARBOX_SEMIAUTO   = "Manual shift - Auto clutch";
const char* CONF_GEARBOX_MANUAL     = "Fully Manual: sequential shift";
const char* CONF_GEARBOX_MAN_STICK  = "Fully manual: stick shift";
const char* CONF_GEARBOX_MAN_RANGES = "Fully Manual: stick shift with ranges";

const char* CONF_FLARES_NONE        = "None (fastest)";
const char* CONF_FLARES_NO_LIGHT    = "No light sources";
const char* CONF_FLARES_CURR_HEAD   = "Only current vehicle, main lights";
const char* CONF_FLARES_ALL_HEADS   = "All vehicles, main lights";
const char* CONF_FLARES_ALL_LIGHTS  = "All vehicles, all lights";

const char* CONF_WATER_NONE         = "None";
const char* CONF_WATER_BASIC        = "Basic (fastest)";
const char* CONF_WATER_REFLECT      = "Reflection";
const char* CONF_WATER_FULL_FAST    = "Reflection + refraction (speed optimized)";
const char* CONF_WATER_FULL_HQ      = "Reflection + refraction (quality optimized)";
const char* CONF_WATER_HYDRAX       = "Hydrax";

const char* CONF_SKY_CAELUM         = "Caelum (best looking, slower)";
const char* CONF_SKY_SKYX           = "SkyX (best looking, slower)";
const char* CONF_SKY_SANDSTORM      = "Sandstorm (fastest)";

const char* CONF_INPUT_GRAB_DYNAMIC = "Dynamically";
const char* CONF_INPUT_GRAB_NONE    = "None";
const char* CONF_INPUT_GRAB_ALL     = "All";

IoInputGrabMode ParseIoInputGrabMode(std::string const & s)
{
    if (s == CONF_INPUT_GRAB_DYNAMIC) { return RoR::IoInputGrabMode::DYNAMIC ; }
    if (s == CONF_INPUT_GRAB_NONE   ) { return RoR::IoInputGrabMode::NONE    ; }
    else                              { return RoR::IoInputGrabMode::ALL     ; }
}

GfxShadowType ParseGfxShadowType(std::string const & s)
{
    if (s == CONF_GFX_SHADOW_PSSM)    { return GfxShadowType::PSSM    ; }
    else                              { return GfxShadowType::NONE    ; }
}

GfxExtCamMode ParseGfxExtCamMode(std::string const & s)
{
    if (s == CONF_EXTCAM_PITCHING)    { return GfxExtCamMode::PITCHING ; }
    if (s == CONF_EXTCAM_STATIC)      { return GfxExtCamMode::STATIC   ; }
    else                              { return GfxExtCamMode::NONE     ; }
}

GfxTexFilter ParseGfxTexFilter(std::string const & s)
{
    if (s == CONF_TEXFILTER_NONE)     { return GfxTexFilter::NONE        ; }
    if (s == CONF_TEXFILTER_BILI)     { return GfxTexFilter::BILINEAR    ; }
    if (s == CONF_TEXFILTER_TRILI)    { return GfxTexFilter::TRILINEAR   ; }
    if (s == CONF_TEXFILTER_ANISO)    { return GfxTexFilter::ANISOTROPIC ; }
    else                              { return GfxTexFilter::NONE        ; }
}

GfxVegetation ParseGfxVegetation(std::string const & s)
{
    if (s == CONF_VEGET_NONE  )       { return GfxVegetation::NONE    ; }
    if (s == CONF_VEGET_20PERC)       { return GfxVegetation::x20PERC ; }
    if (s == CONF_VEGET_50PERC)       { return GfxVegetation::x50PERC ; }
    if (s == CONF_VEGET_FULL  )       { return GfxVegetation::FULL    ; }
    else                              { return GfxVegetation::NONE    ; }
}

SimGearboxMode ParseSimGearboxMode(std::string const & s)
{
    if (s == CONF_GEARBOX_AUTO      ) { return SimGearboxMode::AUTO          ; }
    if (s == CONF_GEARBOX_SEMIAUTO  ) { return SimGearboxMode::SEMI_AUTO     ; }
    if (s == CONF_GEARBOX_MANUAL    ) { return SimGearboxMode::MANUAL        ; }
    if (s == CONF_GEARBOX_MAN_STICK ) { return SimGearboxMode::MANUAL_STICK  ; }
    if (s == CONF_GEARBOX_MAN_RANGES) { return SimGearboxMode::MANUAL_RANGES ; }
    else                              { return SimGearboxMode::AUTO          ; }
}

GfxFlaresMode ParseGfxFlaresMode(std::string const & s)
{
    if (s == CONF_FLARES_NONE      )  { return GfxFlaresMode::NONE                    ; }
    if (s == CONF_FLARES_NO_LIGHT  )  { return GfxFlaresMode::NO_LIGHTSOURCES         ; }
    if (s == CONF_FLARES_CURR_HEAD )  { return GfxFlaresMode::CURR_VEHICLE_HEAD_ONLY  ; }
    if (s == CONF_FLARES_ALL_HEADS )  { return GfxFlaresMode::ALL_VEHICLES_HEAD_ONLY  ; }
    if (s == CONF_FLARES_ALL_LIGHTS)  { return GfxFlaresMode::ALL_VEHICLES_ALL_LIGHTS ; }
    else                              { return GfxFlaresMode::CURR_VEHICLE_HEAD_ONLY  ; }
}

GfxWaterMode ParseGfxWaterMode(std::string const & s)
{
    if (s == CONF_WATER_NONE     )    { return GfxWaterMode::NONE      ; }
    if (s == CONF_WATER_BASIC    )    { return GfxWaterMode::BASIC     ; }
    if (s == CONF_WATER_REFLECT  )    { return GfxWaterMode::REFLECT   ; }
    if (s == CONF_WATER_FULL_FAST)    { return GfxWaterMode::FULL_FAST ; }
    if (s == CONF_WATER_FULL_HQ  )    { return GfxWaterMode::FULL_HQ   ; }
    if (s == CONF_WATER_HYDRAX   )    { return GfxWaterMode::HYDRAX    ; }
    else                              { return GfxWaterMode::BASIC     ; }
}

GfxSkyMode ParseGfxSkyMode(std::string const & s)
{
    if (s == CONF_SKY_SANDSTORM)      { return GfxSkyMode::SANDSTORM ; }
    if (s == CONF_SKY_CAELUM   )      { return GfxSkyMode::CAELUM    ; }
    if (s == CONF_SKY_SKYX     )      { return GfxSkyMode::SKYX      ; }
    else                              { return GfxSkyMode::SANDSTORM ; } 
}

const char* IoInputGrabModeToStr(IoInputGrabMode v)
{
    switch (v)
    {
    case IoInputGrabMode::DYNAMIC: return CONF_INPUT_GRAB_DYNAMIC;
    case IoInputGrabMode::NONE   : return CONF_INPUT_GRAB_NONE;
    case IoInputGrabMode::ALL    : return CONF_INPUT_GRAB_ALL;
    default                      : return "";
    }
}

const char* GfxShadowTypeToStr(GfxShadowType v)
{
    switch (v)
    {
    case GfxShadowType::PSSM   : return CONF_GFX_SHADOW_PSSM;
    case GfxShadowType::NONE   : return CONF_GFX_SHADOW_NONE;
    default                    : return "";
    }
}

const char* GfxExtCamModeToStr(GfxExtCamMode v)
{
    switch (v)
    {
    case GfxExtCamMode::PITCHING: return CONF_EXTCAM_PITCHING;
    case GfxExtCamMode::STATIC  : return CONF_EXTCAM_STATIC;
    case GfxExtCamMode::NONE    : return CONF_EXTCAM_NONE;
    default                     : return "";
    }
}

const char* GfxTexFilterToStr(GfxTexFilter v)
{
    switch (v)
    {
    case GfxTexFilter::NONE       : return CONF_TEXFILTER_NONE;
    case GfxTexFilter::BILINEAR   : return CONF_TEXFILTER_BILI;
    case GfxTexFilter::TRILINEAR  : return CONF_TEXFILTER_TRILI;
    case GfxTexFilter::ANISOTROPIC: return CONF_TEXFILTER_ANISO;
    default                       : return "";
    }
}

const char* GfxVegetationToStr(GfxVegetation v)
{
    switch (v)
    {
    case GfxVegetation::NONE   : return CONF_VEGET_NONE;
    case GfxVegetation::x20PERC: return CONF_VEGET_20PERC;
    case GfxVegetation::x50PERC: return CONF_VEGET_50PERC;
    case GfxVegetation::FULL   : return CONF_VEGET_FULL;
    default                    : return "";
    }
}

const char* SimGearboxModeToStr(SimGearboxMode v)
{
    switch (v)
    {
    case SimGearboxMode::AUTO         : return CONF_GEARBOX_AUTO;
    case SimGearboxMode::SEMI_AUTO    : return CONF_GEARBOX_SEMIAUTO;
    case SimGearboxMode::MANUAL       : return CONF_GEARBOX_MANUAL;
    case SimGearboxMode::MANUAL_STICK : return CONF_GEARBOX_MAN_STICK;
    case SimGearboxMode::MANUAL_RANGES: return CONF_GEARBOX_MAN_RANGES;
    default                           : return "";
    }
}

const char* GfxFlaresModeToStr(GfxFlaresMode v)
{
    switch(v)
    {
    case GfxFlaresMode::NONE                   : return CONF_FLARES_NONE;
    case GfxFlaresMode::NO_LIGHTSOURCES        : return CONF_FLARES_NO_LIGHT;
    case GfxFlaresMode::CURR_VEHICLE_HEAD_ONLY : return CONF_FLARES_CURR_HEAD;
    case GfxFlaresMode::ALL_VEHICLES_HEAD_ONLY : return CONF_FLARES_ALL_HEADS;
    case GfxFlaresMode::ALL_VEHICLES_ALL_LIGHTS: return CONF_FLARES_ALL_LIGHTS;
    default                                    : return "";
    }
}

const char* GfxWaterModeToStr(GfxWaterMode v)
{
    switch(v)
    {
    case GfxWaterMode::NONE     : return CONF_WATER_NONE;
    case GfxWaterMode::BASIC    : return CONF_WATER_BASIC;
    case GfxWaterMode::REFLECT  : return CONF_WATER_REFLECT;
    case GfxWaterMode::FULL_FAST: return CONF_WATER_FULL_FAST;
    case GfxWaterMode::FULL_HQ  : return CONF_WATER_FULL_HQ;
    case GfxWaterMode::HYDRAX   : return CONF_WATER_HYDRAX;
    default                     : return "";
    }
}

const char* GfxSkyModeToStr(GfxSkyMode v)
{
    switch(v)
    {
    case GfxSkyMode::CAELUM   : return CONF_SKY_CAELUM;
    case GfxSkyMode::SKYX     : return CONF_SKY_SKYX;
    case GfxSkyMode::SANDSTORM: return CONF_SKY_SANDSTORM;
    default                   : return "";
    }
}

// --------------------------------
// Config file parsing

void AssignHelper(CVar* cvar, int val)
{
    Str<25> s; s << val;
    App::GetConsole()->cVarAssign(cvar, s.ToCStr());
}

void ParseHelper(CVar* cvar, std::string const & val)
{
    if (cvar->getName() == App::gfx_envmap_rate->getName())
    {
        int rate = Ogre::StringConverter::parseInt(val);
        if (rate < 0) { rate = 0; }
        if (rate > 6) { rate = 6; }
        AssignHelper(App::gfx_envmap_rate, rate);
    }
    else if (cvar->getName() == App::gfx_shadow_type->getName())
    {
        AssignHelper(App::gfx_shadow_type, (int)ParseGfxShadowType(val));
    }
    else if (cvar->getName() == App::gfx_extcam_mode->getName())
    {
        AssignHelper(App::gfx_extcam_mode, (int)ParseGfxExtCamMode(val));
    }
    else if (cvar->getName() == App::gfx_texture_filter->getName())
    {
        AssignHelper(App::gfx_texture_filter, (int)ParseGfxTexFilter(val));
    }
    else if (cvar->getName() == App::gfx_vegetation_mode->getName())
    {
        AssignHelper(App::gfx_vegetation_mode, (int)ParseGfxVegetation(val));
    }
    else if (cvar->getName() == App::gfx_flares_mode->getName())
    {
        AssignHelper(App::gfx_flares_mode, (int)ParseGfxFlaresMode(val));
    }
    else if (cvar->getName() == App::gfx_water_mode->getName())
    {
        AssignHelper(App::gfx_water_mode, (int)ParseGfxWaterMode(val));
    }
    else if (cvar->getName() == App::gfx_sky_mode->getName())
    {
        AssignHelper(App::gfx_sky_mode, (int)ParseGfxSkyMode(val));
    }
    else if (cvar->getName() == App::sim_gearbox_mode->getName())
    {
        AssignHelper(App::sim_gearbox_mode, (int)ParseSimGearboxMode(val));
    }
    else if (cvar->getName() == App::gfx_fov_external_default->getName() ||
             cvar->getName() == App::gfx_fov_internal_default->getName())
    {
        int fov = Ogre::StringConverter::parseInt(val);
        if (fov >= 10) // FOV shouldn't be below 10
        {
            AssignHelper(cvar, fov);
        }
    }
    else
    {
        App::GetConsole()->cVarAssign(cvar, val);
    }
}

void Console::loadConfig()
{
    Ogre::ConfigFile cfg;
    try
    {
        std::string path = PathCombine(App::sys_config_dir->getStr(), CONFIG_FILE_NAME);
        cfg.load(path, "=:\t", /*trimWhitespace=*/true);

        Ogre::ConfigFile::SettingsIterator i = cfg.getSettingsIterator();
        while (i.hasMoreElements())
        {
            std::string cvar_name = SanitizeUtf8String(i.peekNextKey());
            CVar* cvar = App::GetConsole()->cVarFind(cvar_name);
            if (cvar && !cvar->hasFlag(CVAR_ARCHIVE))
            {
                RoR::LogFormat("[RoR|Settings] CVar '%s' cannot be set from %s (defined without 'archive' flag)", cvar->getName().c_str(), CONFIG_FILE_NAME);
                i.moveNext();
                continue;
            }

            if (!cvar)
            {
                cvar = App::GetConsole()->cVarGet(cvar_name, CVAR_ARCHIVE);
            }

            ParseHelper(cvar, SanitizeUtf8String(i.peekNextValue()));

            i.moveNext();
        }
    }
    catch (Ogre::FileNotFoundException&) {} // Just continue with defaults...
}

void WriteVarsHelper(std::stringstream& f, const char* label, const char* prefix)
{
    f << std::endl << "; " << label << std::endl;

    for (auto& pair: App::GetConsole()->getCVars())
    {
        if (pair.second->hasFlag(CVAR_ARCHIVE) && pair.first.find(prefix) == 0)
        {
            if (App::app_config_long_names->getBool())
            {
                f << pair.second->getLongName() << "=";
            }
            else
            {
                f << pair.second->getName() << "=";
            }

                 if (pair.second->getName() == App::gfx_shadow_type->getName()     ){ f << GfxShadowTypeToStr(App::gfx_shadow_type     ->getEnum<GfxShadowType>()); }
            else if (pair.second->getName() == App::gfx_extcam_mode->getName()     ){ f << GfxExtCamModeToStr(App::gfx_extcam_mode     ->getEnum<GfxExtCamMode>()); }
            else if (pair.second->getName() == App::gfx_texture_filter->getName()  ){ f << GfxTexFilterToStr (App::gfx_texture_filter  ->getEnum<GfxTexFilter>());  }
            else if (pair.second->getName() == App::gfx_vegetation_mode->getName() ){ f << GfxVegetationToStr(App::gfx_vegetation_mode ->getEnum<GfxVegetation>()); }
            else if (pair.second->getName() == App::gfx_flares_mode->getName()     ){ f << GfxFlaresModeToStr(App::gfx_flares_mode     ->getEnum<GfxFlaresMode>()); }
            else if (pair.second->getName() == App::gfx_water_mode->getName()      ){ f << GfxWaterModeToStr (App::gfx_water_mode      ->getEnum<GfxWaterMode>());  }
            else if (pair.second->getName() == App::gfx_sky_mode->getName()        ){ f << GfxSkyModeToStr   (App::gfx_sky_mode        ->getEnum<GfxSkyMode>());    }
            else if (pair.second->getName() == App::sim_gearbox_mode->getName()    ){ f << SimGearboxModeToStr(App::sim_gearbox_mode->getEnum<SimGearboxMode>());    }
            else                                                                    { f << pair.second->getStr(); }

            f << std::endl;
        }
    }    
}

void Console::saveConfig()
{
    std::stringstream f;

    f << "; Rigs of Rods configuration file" << std::endl;
    f << "; -------------------------------" << std::endl;
    
    WriteVarsHelper(f, "Application",   "app_");
    WriteVarsHelper(f, "Multiplayer",   "mp_");
    WriteVarsHelper(f, "Simulation",    "sim_");
    WriteVarsHelper(f, "Input/Output",  "io_");
    WriteVarsHelper(f, "Graphics",      "gfx_");
    WriteVarsHelper(f, "GUI",           "ui_");
    WriteVarsHelper(f, "Audio",         "audio_");
    WriteVarsHelper(f, "Diagnostics",   "diag_");

    try
    {
        Ogre::DataStreamPtr stream
            = Ogre::ResourceGroupManager::getSingleton().createResource(
                CONFIG_FILE_NAME, RGN_CONFIG, /*overwrite=*/true);
        size_t written = stream->write(f.str().c_str(), f.str().length());
        if (written < f.str().length())
        {
            RoR::LogFormat("[RoR] Error writing file '%s' (resource group '%s'), ",
                           "only written %u out of %u bytes!",
                           CONFIG_FILE_NAME, RGN_CONFIG, written, f.str().length());
        }
    }
    catch (std::exception& e)
    {
        RoR::LogFormat("[RoR|Settings] Error writing file '%s' (resource group '%s'), message: '%s'",
                        CONFIG_FILE_NAME, RGN_CONFIG, e.what());
    }
}


