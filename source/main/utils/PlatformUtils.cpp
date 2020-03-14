/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2018 Petr Ohlidal & contributors

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

///    @file   PlatformUtils.cpp
///    @author Petr Ohlidal
///    @date   05/2014


#include "PlatformUtils.h"
#include "Application.h"

#ifdef _MSC_VER
    #include <Windows.h>
    #include <shlobj.h> // SHGetFolderPathW()
#else
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <unistd.h> // readlink()
#endif

#include <OgrePlatform.h>
#include <OgreFileSystem.h>
#include <string>

namespace RoR {

#ifdef _MSC_VER

// -------------------------- File/path utils for MS Windows --------------------------
// Based on research in https://github.com/only-a-ptr/filepaths4rigs

char PATH_SLASH = '\\';

std::wstring MSW_Utf8ToWchar(const char* path) 
{
    if( path == nullptr || path[0] == 0 )
    {
        RoR::Log("[RoR] Internal error: MSW_Utf8ToWchar() received empty input");
        return std::wstring();
    }

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &path[0], -1, NULL, 0); // Doc: https://msdn.microsoft.com/en-us/library/windows/desktop/dd319072(v=vs.85).aspx
    std::wstring out_wstr( size_needed, 0 );
    int raw_result = MultiByteToWideChar(CP_UTF8, 0, &path[0], -1, &out_wstr[0], size_needed);
    if (raw_result <= 0)
    {
        RoR::LogFormat("[RoR] Internal error: MSW_Utf8ToWchar() could not convert UTF-8 to UTF-16; MultiByteToWideChar() returned %d", raw_result);
        return std::wstring();
    }
    return out_wstr;
}

DWORD MSW_GetFileAttrs(const char* path)
{
    if (path == nullptr || path[0] == 0)
    {
        return INVALID_FILE_ATTRIBUTES;
    }

    std::wstring wpath = MSW_Utf8ToWchar(path);
    // Function reference: https://msdn.microsoft.com/en-us/library/windows/desktop/aa364944(v=vs.85).aspx
    // File attribute constants: https://msdn.microsoft.com/en-us/library/windows/desktop/gg258117(v=vs.85).aspx
    return GetFileAttributesW(wpath.c_str());
}

std::string MSW_WcharToUtf8(const wchar_t* wstr) // wstr _must_ be NUL-terminated!
{
    if( wstr == nullptr || wstr[0] == 0 )
    {
        RoR::Log("[RoR] Internal error: MSW_WcharToUtf8() received empty input");
        return std::string();
    }

    const int dst_size = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], -1, nullptr, 0, nullptr, nullptr);
    std::string dst(dst_size, 0); // Construct by length and initial value
    int raw_result = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], -1, &dst[0], dst_size, nullptr, nullptr);
    if (raw_result <= 0)
    {
        RoR::LogFormat("[RoR] Internal error: MSW_WcharToUtf8() could not convert UTF-16 to UTF-8; WideCharToMultiByte() returned %d", raw_result);
        return std::string();
    }
    return dst;
}

bool FileExists(const char *path)
{
    DWORD file_attrs = MSW_GetFileAttrs(path);
    return (file_attrs != INVALID_FILE_ATTRIBUTES && ! (file_attrs & FILE_ATTRIBUTE_DIRECTORY));
}

bool FolderExists(const char* path)
{
    DWORD file_attrs = MSW_GetFileAttrs(path);
    return (file_attrs != INVALID_FILE_ATTRIBUTES && (file_attrs & FILE_ATTRIBUTE_DIRECTORY));
}

void CreateFolder(const char* path)
{
    if (!FolderExists(path))
    {
        std::wstring wpath = MSW_Utf8ToWchar(path);
        CreateDirectoryW(wpath.c_str(), nullptr);
    }
}

std::string GetUserHomeDirectory()
{
    std::wstring out_wstr(MAX_PATH, 0); // Length limit imposed by the function, see https://msdn.microsoft.com/en-us/library/windows/desktop/bb762181(v=vs.85).aspx
    HRESULT hres = SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, &out_wstr[0]);
    if (hres != S_OK)
    {
        RoR::LogFormat("[RoR] Internal error: GetUserHomeDirectory() failed; SHGetFolderPathW() returned %ld", static_cast<long>(hres));
        return std::string();
    }

    return MSW_WcharToUtf8(out_wstr.c_str());
}

std::string GetExecutablePath()
{
    const int BUF_SIZE = 500;
    std::wstring out_wstr(BUF_SIZE, 0);
    DWORD res = GetModuleFileNameW(nullptr, &out_wstr[0], BUF_SIZE);
    if (res <= 0)
    {
        RoR::LogFormat("[RoR] Internal error: GetExecutablePath() failed; GetModuleFileNameW() returned %d", static_cast<int>(res));
        return std::string();
    }

    return MSW_WcharToUtf8(out_wstr.c_str());
}

#else

// -------------------------- File/path utils for Linux/*nix --------------------------

char PATH_SLASH = '/';

bool FileExists(const char *path)
{
    struct stat st;
    return (stat(path, &st) == 0);
}

bool FolderExists(const char* path)
{
    struct stat st;
    return (stat(path, &st) == 0);
}

void CreateFolder(const char* path)
{
    mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

std::string GetUserHomeDirectory()
{
    return getenv("HOME");
}

std::string GetExecutablePath()
{
    const int BUF_SIZE = 500;
    std::string buf_str(BUF_SIZE, 0);
    // Linux or POSIX assumed; http://stackoverflow.com/a/625523
    if (readlink("/proc/self/exe", &buf_str[0], BUF_SIZE-1) == -1)
    {
        RoR::LogFormat("[RoR] Internal error: GetExecutablePath() failed; readlink() sets errno to %d", static_cast<int>(errno));
        return std::string();
    }

    return std::move(buf_str);
}

#endif // _MSC_VER

// -------------------------- File/path common utils --------------------------

std::string GetParentDirectory(const char* src_buff)
{
    const char* start = src_buff;
    size_t count = strlen(src_buff);
    // Trim trailing separator(s)
    for (;;)
    {
        if (count == 0) { return std::string(); }
        if (start[count - 1] != PATH_SLASH) { break; }
        --count;
    }
    // Remove last path entry
    for (;;)
    {
        if (count == 0) { return std::string(); }
        if (start[count - 1] == PATH_SLASH) {break; }
        --count;
    }
    // Trim rear separator(s)
    for (;;)
    {
        if (count == 0) { return std::string(); }
        if (start[count - 1] != PATH_SLASH) { break; }
        --count;
    }
    
    return std::string(start, count);
}

std::time_t GetFileLastModifiedTime(std::string const & path)
{
    Ogre::FileSystemArchiveFactory factory;
    Ogre::Archive* fs_archive = factory.createInstance(path, /*readOnly*/true);
    std::time_t time = fs_archive->getModifiedTime(path);
    factory.destroyInstance(fs_archive);
    return time;
}

std::string PathStrEscape(std::string const& p)
{
    Str<1000> res;
    for (char c: p)
    {
        if (c == '\\')
        {
            res << "\\\\";
        }
        else
        {
            res << c;
        }
    }
    return res.ToCStr();
}

} // namespace RoR
