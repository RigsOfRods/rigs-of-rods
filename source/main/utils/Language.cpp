/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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
/// @author Thomas Fischer (thomas{AT}thomasfischer{DOT}biz)
/// @date   12th of January 2009

#ifndef NOLANG

#include "Language.h"

#include "Application.h"
#include "PlatformUtils.h"

using namespace Ogre;
using namespace RoR;

void LanguageEngine::setup()
{
    // load language, must happen after initializing Settings class and Ogre Root!
    // also it must happen after loading all basic resources!

    Str<500> mo_path;
    mo_path << App::sys_process_dir.GetActive() << PATH_SLASH << "languages" << PATH_SLASH;
    mo_path << App::app_locale.GetActive()[0] << App::app_locale.GetActive()[1]; // Only first 2 chars are important
    mo_path << PATH_SLASH << "LC_MESSAGES";

    // Load a .mo-File.
    std::string rormo_path = PathCombine(mo_path.ToCStr(), "ror.mo");
    if (moFileLib::moFileReaderSingleton::GetInstance ().ReadFile(rormo_path.c_str ()) == moFileLib::moFileReader::EC_SUCCESS)
    {
        RoR::LogFormat ("[RoR|App] Loading language file '%s'", rormo_path.c_str ());
    }
    else
    {
        RoR::LogFormat ("[RoR|App] Error loading language file: '%s'", rormo_path.c_str ());
        return;
    }

    RoR::Log("[RoR|App] Language successfully loaded");
}
#endif //NOLANG
