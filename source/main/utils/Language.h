/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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
/// @date    12th of January 2009

#pragma once

#include "RoRPrerequisites.h"
#include "Singleton.h"

#include <OgreUTFString.h>

// three configurations currently supported:
// #define NOLANG            = no language translations at all, removes any special parsing tags
// #define USE_MOFILEREADER  = windows gettext replacement

#ifdef NOLANG
// no language mode
# define _L
# define _LC

#else // NOLANG

// using mofilereader as gettext replacement
#include <moFileReader.h>

#define _L(str) moFileLib::moFileReaderSingleton::GetInstance().Lookup(str).c_str()
#define _LC(ctx,str) moFileLib::moFileReaderSingleton::GetInstance().LookupWithContext(ctx,str).c_str()

class LanguageEngine : public RoRSingleton<LanguageEngine>, public ZeroedMemoryAllocator
{
    friend class RoRSingleton<LanguageEngine>;

public:
    void setup();

    std::vector<std::pair<std::string, std::string>> getLanguages() { return languages; };

private:
    std::vector<std::pair<std::string, std::string>> languages;
};

#endif // NOLANG
