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
    else if (cvar->getName() == App::gfx_shadow_quality->getName())
    {
        int quality = Ogre::StringConverter::parseInt(val);
        if (quality < 0) { quality = 0; }
        if (quality > 3) { quality = 3; }
        AssignHelper(App::gfx_shadow_quality, quality);
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


