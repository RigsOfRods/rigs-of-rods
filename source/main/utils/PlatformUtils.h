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

///  @file   PlatformUtils.h
///  @author Petr Ohlidal
///  @date   05/2014
///  @brief  Platform-specific utilities. We use narrow UTF-8 encoded strings as paths. Inspired by http://utf8everywhere.org/ manifesto.
///  Code based on https://github.com/only-a-ptr/filepaths4rigs

#pragma once

#include <string>
#include <ctime>

namespace RoR {

extern char PATH_SLASH;

bool FileExists(const char* path);   //!< Path must be UTF-8 encoded.
bool FolderExists(const char* path); //!< Path must be UTF-8 encoded.
void CreateFolder(const char* path); //!< Path must be UTF-8 encoded.

inline bool FileExists(std::string const& path)   { return FileExists(path.c_str()); }
inline bool FolderExists(std::string const& path) { return FolderExists(path.c_str()); }
inline void CreateFolder(std::string const& path) { CreateFolder(path.c_str()); }

inline std::string PathCombine(std::string a, std::string b) { return a + PATH_SLASH + b; };

std::string GetUserHomeDirectory(); //!< Returns UTF-8 path or empty string on error
std::string GetExecutablePath(); //!< Returns UTF-8 path or empty string on error
std::string GetParentDirectory(const char* path); //!< Returns UTF-8 path without trailing slash.

std::time_t GetFileLastModifiedTime(std::string const & path);

} // namespace RoR
