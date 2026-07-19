
// =============================================================================
// This file is adopted from rorserver at commit 4a7109ae2d9a081ccfdad8cc696dc54efe49acb3
// Changes from the original are marked with "//RIGSOFRODS"
// =============================================================================

/*
This file is part of "Rigs of Rods Server" (Relay mode)

Copyright 2007   Pierre-Michel Ricordel
Copyright 2014+  Rigs of Rods Community

"Rigs of Rods Server" is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, either version 3
of the License, or (at your option) any later version.

"Rigs of Rods Server" is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar. If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef USE_ANGELSCRIPT

#include "ServerScriptLogger.h"

#include "Application.h" // RIGSOFRODS: For App::sys_logs_dir
#include "PlatformUtils.h" // RIGSOFRODS: For RoR::PathCombine

using namespace RoR;

static Ogre::Log* sServerLog = nullptr;

void Init()
{
    if (!sServerLog)
        sServerLog = Ogre::LogManager::getSingleton().createLog(RoR::PathCombine(App::sys_logs_dir->getStr(), "RoRServerScript.log"));
}

void Logger::Log(ServerLogLevel level, const char *format, ...)
{
    Init();
    
    // Format the message
    const int BUF_LEN = 4000; // hard limit
    char buffer[BUF_LEN] = {}; // zeroed memory
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, BUF_LEN, format, args);
    va_end(args);
    // Log the message
    sServerLog->logMessage(std::string(buffer));
}

void Logger::Log(ServerLogLevel level, std::string const& msg)
{
    Init();
    
    sServerLog->logMessage(msg);
}

#endif // USE_ANGELSCRIPT