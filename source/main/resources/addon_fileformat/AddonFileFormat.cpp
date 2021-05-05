/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2021 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/// @file   
/// @brief  Addons are AngelScript-powered feature packages for Rigs of Rods.
/// @author Petr Ohlidal, 05/2021

#include "AddonFileFormat.h"

#include "AppContext.h"
#include "ConfigFile.h"
#include "Console.h"
#include "Utils.h"
#include "SimConstants.h"

#include <OgreException.h>

using namespace RoR;
using namespace Ogre;

const std::string   VALUE_NOT_FOUND("@@NotFound!!");

bool AddonParser::LoadAddonFile(AddonDef& def, Ogre::DataStreamPtr &ds)
{
    RoR::ConfigFile file;
    file.load(ds, "\t:=", true);

    // read in the settings
    def.name = file.GetStringEx("Name", "General");
    if (def.name.empty())
    {
        Str<500> msg; msg << "Error in file '" << ds->getName() << "': Addon name is empty";
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_SCRIPT, Console::CONSOLE_SYSTEM_ERROR, msg.ToCStr());
        return false;
    }

    def.category_id          = file.GetInt        ("CategoryID",       "General", 129);
    def.guid                 = file.GetStringEx   ("GUID",             "General");
    def.version              = file.GetInt        ("Version",          "General", 1);

    if (file.HasSection("Authors"))
    {
        for (auto& author: file.getSettings("Authors"))
        {
            String type = RoR::Utils::SanitizeUtf8String(author.first);  // e.g. terrain
            String name = RoR::Utils::SanitizeUtf8String(author.second); // e.g. john doe

            if (!name.empty())
            {
                AuthorInfo author;
                author.type = type;
                author.name = name;
                def.authors.push_back(author);
            }
        }
    }

    if (file.HasSection("Scripts"))
    {
        for (auto& script: file.getSettings("Scripts"))
        {
            Ogre::String as_filename = RoR::Utils::SanitizeUtf8String(script.first);
            def.as_files.push_back(RoR::Utils::TrimStr(as_filename));
        }
    }

    def.as_load_func_name   = file.GetStringEx   ("LoadFunctionName",   "General");
    def.as_unload_func_name = file.GetStringEx   ("UnloadFunctionName", "General");

    return true;
}

