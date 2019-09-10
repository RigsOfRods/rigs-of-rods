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

#include "Utils.h"

#include "RoRnet.h"
#include "RoRVersion.h"
#include "SHA1.h"
#include "Application.h"

#include <Ogre.h>

#ifdef USE_DISCORD_RPC
#include <discord_rpc.h>
#endif

#ifndef _WIN32
#   include <iconv.h>
#endif

#ifdef _WIN32
#   include <windows.h> // Sleep()
#endif

using namespace Ogre;

void InitDiscord()
{
#ifdef USE_DISCORD_RPC
    if(RoR::App::io_discord_rpc.GetActive())
    {
        DiscordEventHandlers handlers;
        memset(&handlers, 0, sizeof(handlers));
        handlers.ready = handleDiscordReady;
        handlers.errored = handleDiscordError;

        // Discord_Initialize(const char* applicationId, DiscordEventHandlers* handlers, int autoRegister, const char* optionalSteamId)
        Discord_Initialize("492484203435393035", &handlers, 1, "1234");
    }
#endif
}

#ifdef USE_DISCORD_RPC
void handleDiscordError(int, const char *error)
{
    RoR::LogFormat("Discord Error: %s", error);
}

void handleDiscordReady(const DiscordUser *user)
{
    RoR::LogFormat("Discord Ready: %s", user->username);
}
#endif

void UpdatePresence()
{
#ifdef USE_DISCORD_RPC
    if(RoR::App::io_discord_rpc.GetActive())
    {
        char buffer[256];
        DiscordRichPresence discordPresence;
        memset(&discordPresence, 0, sizeof(discordPresence));
        if (RoR::App::mp_state.GetActive() == RoR::MpState::CONNECTED)
        {
            discordPresence.state = "Playing online";
            sprintf(buffer, "On server: %s:%d  on terrain: %s",
                    RoR::App::mp_server_host.GetActive(),
                    RoR::App::mp_server_port.GetActive(),
                    RoR::App::sim_terrain_gui_name.GetActive());
        }
        else
        {
            discordPresence.state = "Playing Singleplayer";
            sprintf(buffer, "On terrain: %s", RoR::App::sim_terrain_gui_name.GetActive());
        }
        discordPresence.details = buffer;
        Discord_UpdatePresence(&discordPresence);
    }
#endif
}

void ShutdownDiscord()
{
#ifdef USE_DISCORD_RPC
    Discord_Shutdown();
#endif
}

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

String HashFile(const char *szFileName)
{
    unsigned long ulFileSize, ulRest, ulBlocks;
    std::vector<char> uData(2097152);

    if (szFileName == NULL) return "";

    FILE *fIn = fopen(szFileName, "rb");
    if (fIn == NULL) return "";

    fseek(fIn, 0, SEEK_END);
    ulFileSize = (unsigned long)ftell(fIn);
    fseek(fIn, 0, SEEK_SET);

    if (ulFileSize != 0)
    {
        ulBlocks = ulFileSize / static_cast<unsigned long>(uData.size());
        ulRest = ulFileSize % uData.size();
    }
    else
    {
        ulBlocks = 0;
        ulRest = 0;
    }

    uint32_t hash = 0;
    for (unsigned long i = 0; i < ulBlocks; i++)
    {
        size_t result = fread(uData.data(), 1, uData.size(), fIn);
        hash = FastHash(uData.data(), uData.size(), hash);
    }

    if (ulRest != 0)
    {
        size_t result = fread(uData.data(), 1, ulRest, fIn);
        hash = FastHash(uData.data(), ulRest, hash);
    }

    fclose(fIn); fIn = NULL;

    std::stringstream result;
    result << std::hex << hash;
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

void fixRenderWindowIcon(RenderWindow* rw)
{
#ifdef _WIN32
    size_t hWnd = 0;
    rw->getCustomAttribute("WINDOW", &hWnd);

    char buf[MAX_PATH];
    ::GetModuleFileNameA(0, (LPCH)&buf, MAX_PATH);

    HINSTANCE instance = ::GetModuleHandleA(buf);
    HICON hIcon = ::LoadIconA(instance, MAKEINTRESOURCEA(101));
    if (hIcon)
    {
        ::SendMessageA((HWND)hWnd, WM_SETICON, 1, (LPARAM)hIcon);
        ::SendMessageA((HWND)hWnd, WM_SETICON, 0, (LPARAM)hIcon);
    }
#endif // _WIN32
}

UTFString ANSI_TO_UTF(const String source)
{
    return UTFString(RoR::Utils::SanitizeUtf8String(source));
}

std::wstring ANSI_TO_WCHAR(const String source)
{
    return ANSI_TO_UTF(source).asWStr();
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
    sha1.UpdateHash((uint8_t *)input.c_str(), input.length());
    sha1.Final();
    return sha1.ReportHash();
}

} // namespace Utils
} // namespace RoR
