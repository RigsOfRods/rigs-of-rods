/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2026 Petr Ohlidal

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
/// @author Thomas Fischer
/// @date   15th of May 2011

#include "OgreScriptBuilder.h"

#include "Application.h"
#include "Actor.h"
#include "Console.h"
#include "ContentManager.h"
#include "Utils.h"

#include <string>

using namespace RoR;

int OgreScriptBuilder::LoadScriptSection(const char* full_path_cstr)
{
    // Get filename - required to retrieve file from OGRe's resource system.
    //  This function received filename in older AngelScript versions, but now receives full path
    //      (reconstructed wrong by CScriptBuilder because it doesn't know about OGRE's ZIP files).
    //  TODO: Refactor the entire script building logic 
    //      - create fully RoR-custom builder instead of hacked stock CScriptBuilder + our overload. ~ ohlidalp, 08/2017

    std::string full_path(full_path_cstr);
    std::string filename;
    size_t slash_pos = full_path.rfind('/'); // AngelScript always uses forward slashes in paths.
    if (slash_pos != std::string::npos)
    {
        filename = full_path.substr(slash_pos+1);
    }
    else
    {
        filename = full_path;
    }

    // Check for _DEVEL.as version first if the CVar is enabled
    std::string devel_filename;
    std::string basename, ext;
    Ogre::StringUtil::splitBaseFilename(filename, basename, ext);
    if (ext == "as" && App::diag_load_devel_scripts->getBool())
    {
        devel_filename = basename + "_DEVEL.as";
    }

    Ogre::DataStreamPtr ds;
    std::string actual_filename = filename;
    
    // Try to load _DEVEL version first if applicable
    // Note we use 'throwOnFailure=false' to avoid spamming the log with 'Not found' errors. ~ ohlidalp, 01/2026
    if (!devel_filename.empty())
    {
        try
        {
            ds = Ogre::ResourceGroupManager::getSingleton().openResource(
                devel_filename,
                Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                /*resourceBeingLoaded: */nullptr,
                /*throwOnFailure:*/false);

            if (ds)
            {
                actual_filename = devel_filename;
                App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                    fmt::format("Using development version of script: '{}'", devel_filename));
            }
        }
        catch (Ogre::Exception&)
        {
            // _DEVEL version not found, fall back to regular version
            ds.reset();
        }
    }

    // If _DEVEL version wasn't found or not applicable, load the regular version
    
    if (!ds)
    {
        try
        {
            ds = Ogre::ResourceGroupManager::getSingleton().openResource(filename, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

                     //TODO: do not use `AUTODETECT_RESOURCE_GROUP_NAME`, use specific group, lookups are slow!
                     //see also https://github.com/OGRECave/ogre/blob/master/Docs/1.10-Notes.md#resourcemanager-strict-mode ~ ohlidalp, 08/2017
        }
        catch (Ogre::Exception& e)
        {
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, e.getDescription());
            return -2;
        }
    }
    
    // In some cases (i.e. when fed a full path with '/'-s on Windows), `openResource()` will silently return NULL for datastream. ~ ohlidalp, 08/2017
    if (!ds)
    {
        LOG("[RoR|Scripting] Failed to load file '"+filename+"', reason unknown.");
        return -1;
    }

    const std::string& code = ds->getAsString();
    hash = RoR::Sha1Hash(code);

    return ProcessScriptSection(code.c_str(), static_cast<unsigned int>(code.length()), actual_filename.c_str(), 0);
}
