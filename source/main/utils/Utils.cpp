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

#include "Utils.h"

#include "RoRnet.h"
#include "RoRVersion.h"
#include "SHA1.h"
#include "Application.h"

#include <Ogre.h>

#ifndef _WIN32
#   include <iconv.h>
#endif

#ifdef _WIN32
#   include <windows.h> // Sleep()
#endif

using namespace Ogre;
using namespace RoR;

String sha1sum(const char *key, int len)
{
    RoR::CSHA1 sha1;
    sha1.UpdateHash((uint8_t *)key, len);
    sha1.Final();
    return sha1.ReportHash();
}

String HashData(const char *key, int len)
{
    std::stringstream result;
    result << std::hex << FastHash(key, len);
    return result.str();
}

UTFString tryConvertUTF(const char* buffer)
{
    std::string str_in(buffer);
    return UTFString(RoR::Utils::SanitizeUtf8String(str_in));
}

UTFString formatBytes(double bytes)
{
    wchar_t tmp[128] = L"";
    const wchar_t* si_prefix[] = {L"B", L"KB", L"MB", L"GB", L"TB", L"EB", L"ZB", L"YB"};
    int base = 1024;
    int c = bytes > 0 ? std::min((int)(log(bytes) / log((float)base)), (int)sizeof(si_prefix) - 1) : 0;
    swprintf(tmp, 128, L"%1.2f %ls", bytes / pow((float)base, c), si_prefix[c]);
    return UTFString(tmp);
}

std::time_t getTimeStamp()
{
    return time(NULL); //this will overflow in 2038
}

String getVersionString(bool multiline)
{
    char tmp[1024] = "";
    if (multiline)
    {
        sprintf(tmp, "Rigs of Rods\n"
            " version: %s\n"
            " protocol version: %s\n"
            " build time: %s, %s\n"
            , ROR_VERSION_STRING, RORNET_VERSION, ROR_BUILD_DATE, ROR_BUILD_TIME);
    }
    else
    {
        sprintf(tmp, "Rigs of Rods version %s, protocol version: %s, build time: %s, %s", ROR_VERSION_STRING, RORNET_VERSION, ROR_BUILD_DATE, ROR_BUILD_TIME);
    }

    return String(tmp);
}

UTFString ANSI_TO_UTF(const String source)
{
    return UTFString(RoR::Utils::SanitizeUtf8String(source));
}

void trimUTFString(UTFString& str, bool left, bool right)
{
    static const String delims = " \t\r";
    if (right)
        str.erase(str.find_last_not_of(delims) + 1); // trim right
    if (left)
        str.erase(0, str.find_first_not_of(delims)); // trim left
}

Real Round(Real value, unsigned short ndigits /* = 0 */)
{
    Real f = 1.0f;

    while (ndigits--)
        f = f * 10.0f;

    value *= f;

    if (value >= 0.0f)
        value = std::floor(value + 0.5f);
    else
        value = std::ceil(value - 0.5f);

    value /= f;

    return value;
}

Real Round(Real value, int valueN, unsigned short ndigits /* = 0 */)
{
    Real f = 1.0f;

    while (ndigits--)
        f = f * 10.0f;

    value *= f;

    if (value >= 0.0f)
        value = std::floor(value + valueN);
    else
        value = std::ceil(value - valueN);

    value /= f;

    return value;
}

namespace RoR {
namespace Utils {

std::string SanitizeUtf8String(std::string const& str_in)
{
    // Cloned from UTFCPP tutorial: http://utfcpp.sourceforge.net/#fixinvalid
    std::string str_out;
    utf8::replace_invalid(str_in.begin(), str_in.end(), std::back_inserter(str_out));
    return str_out;
}

std::string SanitizeUtf8CString(const char* start, const char* end /* = nullptr */)
{
    if (end == nullptr)
    {
        end = (start + strlen(start));
    }

    // Cloned from UTFCPP tutorial: http://utfcpp.sourceforge.net/#fixinvalid
    std::string str_out;
    utf8::replace_invalid(start, end, std::back_inserter(str_out));
    return str_out;
}

std::string Sha1Hash(std::string const & input)
{
    RoR::CSHA1 sha1;
    sha1.UpdateHash((uint8_t *)input.c_str(), (int)input.length());
    sha1.Final();
    return sha1.ReportHash();
}

} // namespace Utils
} // namespace RoR
