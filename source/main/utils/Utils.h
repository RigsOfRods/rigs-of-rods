/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2016 Petr Ohlidal

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

/**
    @file
    @author Thomas Fischer thomas{AT}thomasfischer{DOT}biz
    @date   9th of August 2009
*/

#pragma once

#include "Application.h"

#include "utf8/checked.h"
#include "utf8/unchecked.h"

#include <MyGUI.h>
#include <OgreUTFString.h>

Ogre::String sha1sum(const char *key, int len);

Ogre::String HashData(const char *key, int len);

Ogre::UTFString tryConvertUTF(const char* buffer);

Ogre::UTFString formatBytes(double bytes);

std::time_t getTimeStamp();

Ogre::String getVersionString(bool multiline = true);

std::wstring ANSI_TO_WCHAR(const Ogre::String source);
Ogre::UTFString ANSI_TO_UTF(const Ogre::String source);

void trimUTFString(Ogre::UTFString& str, bool left = true, bool right = true);

inline MyGUI::UString convertToMyGUIString(char* charstr)
{
    return MyGUI::UString(ANSI_TO_WCHAR(charstr));
}

inline MyGUI::UString convertToMyGUIString(wchar_t* charstr, int len)
{
    return MyGUI::UString(charstr, len);
}

inline MyGUI::UString convertToMyGUIString(std::wstring str)
{
    return MyGUI::UString(str);
}

inline MyGUI::UString convertToMyGUIString(Ogre::UTFString str)
{
    return MyGUI::UString(str.asWStr());
}

inline Ogre::UTFString convertFromMyGUIString(MyGUI::UString str)
{
    return Ogre::UTFString(str.asWStr());
}

inline void replaceString(std::string& str, std::string searchString, std::string replaceString)
{
    std::string::size_type pos = 0;
    while ((pos = str.find(searchString, pos)) != std::string::npos)
    {
        str.replace(pos, searchString.size(), replaceString);
        pos++;
    }
}

Ogre::Real Round(Ogre::Real value, unsigned short ndigits = 0);

namespace RoR {
namespace Utils {
std::string SanitizeUtf8String(std::string const& str_in);
std::string SanitizeUtf8CString(const char* start, const char* end = nullptr);

    inline std::string& TrimStr(std::string& s) { Ogre::StringUtil::trim(s); return s; }
    std::string Sha1Hash(std::string const & data);
}

namespace Color {
const Ogre::UTFString CommandColour = Ogre::UTFString("#00FF00");
const Ogre::UTFString WarningColour = Ogre::UTFString("#FF0000");
const Ogre::UTFString NormalColour = Ogre::UTFString("#FFFFFF");
const Ogre::UTFString WhisperColour = Ogre::UTFString("#FFCC00");
const Ogre::UTFString ScriptCommandColour = Ogre::UTFString("#0099FF");
}

} // namespace RoR
