/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2014 Petr Ohlidal

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

/** 
    @file   PlatformUtils.cpp
    @author Petr Ohlidal
    @date   05/2014
*/

#include "PlatformUtils.h"

#include <Ogre.h>

using namespace RoR;

// ================================================================================
// Static variables
// ================================================================================

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    const Ogre::String PlatformUtils::DIRECTORY_SEPARATOR("\\");
#else
    const Ogre::String PlatformUtils::DIRECTORY_SEPARATOR("/");
#endif

// ================================================================================
// Functions
// ================================================================================

bool PlatformUtils::FileExists(const char *path)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    DWORD attributes = GetFileAttributesA(path);
    return (attributes != INVALID_FILE_ATTRIBUTES && ! (attributes & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return (stat(path, &st) == 0);
#endif
}

bool PlatformUtils::FolderExists(const char *path)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    DWORD attributes = GetFileAttributesA(path);
    return (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return (stat(path, &st) == 0);
#endif
}

bool PlatformUtils::FileExists(Ogre::String const & path)
{
    return FileExists(path.c_str());
}

bool PlatformUtils::FolderExists(Ogre::String const & path)
{
    return FolderExists(path.c_str());
}
