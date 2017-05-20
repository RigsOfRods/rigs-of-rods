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

// three configurations currently supported:
// #define NOLANG            = no language translations at all, removes any special parsing tags
// #define USE_MOFILEREADER  = windows gettext replacement
// #define !USE_MOFILEREADER = using linux gettext

# define U(str) Ogre::UTFString(L##str)

#ifdef NOLANG
// no language mode
// used when building with wxwidgets for example (as they ship their own i18n)
# define _L(str) ANSI_TO_UTF(str)

#else // NOLANG

//#include "Ogre.h"
#ifdef USE_MOFILEREADER
// using mofilereader as gettext replacement
# include "moFileReader.h"
# define _L(str) LanguageEngine::getSingleton().lookUp(str).c_str()
#else
// gettext

#ifdef _WIN32
#error please use MOFILEREADER (enable in cmake) when compiling for windows
#endif // _WIN32

# include <libintl.h>
# include <locale.h>
# define _L(str) gettext(str)
#endif //MOFILEREADER

#define MOFILENAME "ror"

class LanguageEngine : public RoRSingleton<LanguageEngine>, public ZeroedMemoryAllocator
{
    friend class RoRSingleton<LanguageEngine>;

public:
    void setup();
    void postSetup();
    Ogre::UTFString lookUp(Ogre::String name);

    Ogre::String getMyGUIFontConfigFilename();

protected:
    LanguageEngine();
    ~LanguageEngine();
    LanguageEngine(const LanguageEngine&);
    LanguageEngine& operator=(const LanguageEngine&);
    Ogre::String myguiConfigFilename;
    static LanguageEngine* myInstance;
    bool working;
#ifdef USE_MOFILEREADER
    moFileLib::moFileReader* reader;
#endif // MOFILEREADER
    void setupCodeRanges(Ogre::String codeRangesFilename, Ogre::String codeRangesGroupname);
};

#endif // NOLANG

