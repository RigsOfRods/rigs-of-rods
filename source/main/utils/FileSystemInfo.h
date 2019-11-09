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

/*
    This file was ported from MyGUI project (MIT licensed)
    https://github.com/MyGUI/mygui
    http://mygui.info/
*/

/*!
    @file
    @author		Albert Semenov
    @date		09/2009
    @module
*/

#pragma once

#include <MyGUI.h>
#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
    #include <io.h>
    #include <windows.h>
#else
    #include <dirent.h>
    #include <unistd.h>
#endif

#include <string>
#include <vector>

namespace RoR
{

    namespace FileSystem
    {

        struct FileInfo
        {
            FileInfo(const std::wstring &_name, bool _folder) : name(_name), folder(_folder)
            {
            }

            std::wstring name;
            bool         folder;
        };

        typedef std::vector<FileInfo> VectorFileInfo;

        inline bool isAbsolutePath(const wchar_t *path)
        {
#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
            if (IsCharAlphaW(path[0]) && path[1] == ':') return true;
#endif
            return path[0] == '/' || path[0] == '\\';
        }

        inline std::wstring concatenatePath(const std::wstring &_base, const std::wstring &_name)
        {
            if (_base.empty() || isAbsolutePath(_name.c_str()))
                return _name;
            else
#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
                return _base + L'\\' + _name;
#else
                return _base + L'/' + _name;
#endif
        }

        inline bool isReservedDir(const wchar_t *_fn)
        {
            // if "."
            return (_fn[0] == '.' && _fn[1] == 0);
        }

        inline bool isParentDir(const wchar_t *_fn)
        {
            // if ".."
            return (_fn[0] == '.' && _fn[1] == '.' && _fn[2] == 0);
        }

        inline void getSystemFileList(VectorFileInfo &_result, const std::wstring &_folder, const std::wstring &_mask)
        {
#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
            // FIXME add optional parameter?
            bool ms_IgnoreHidden = true;

            long                lHandle, res;
            struct _wfinddata_t tagData;

            // pattern can contain a directory name, separate it from mask
            size_t       pos = _mask.find_last_of(L"/\\");
            std::wstring directory;
            if (pos != _mask.npos) directory = _mask.substr(0, pos);

            std::wstring full_mask = concatenatePath(_folder, _mask);

            lHandle = _wfindfirst(full_mask.c_str(), &tagData);
            res     = 0;
            while (lHandle != -1 && res != -1)
            {
                if ((!ms_IgnoreHidden || (tagData.attrib & _A_HIDDEN) == 0) && (!isReservedDir(tagData.name)))
                { _result.push_back(FileInfo(concatenatePath(directory, tagData.name), (tagData.attrib & _A_SUBDIR) != 0)); }
                res = _wfindnext(lHandle, &tagData);
            }
            // Close if we found any files
            if (lHandle != -1) _findclose(lHandle);
#else
            DIR *dir = opendir(MyGUI::UString(_folder).asUTF8_c_str());
            struct dirent *dp;

            if (dir == NULL)
            { /* opendir() failed */
            }

            rewinddir(dir);

            while ((dp = readdir(dir)) != NULL)
            {
                if (!isReservedDir(MyGUI::UString(dp->d_name).asWStr_c_str()))
                    _result.push_back(FileInfo(MyGUI::UString(dp->d_name).asWStr(), (dp->d_type == DT_DIR)));
            }

            closedir(dir);
#endif
        }

        inline std::wstring getSystemCurrentFolder()
        {
#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
            wchar_t buff[MAX_PATH + 1];
            ::GetCurrentDirectoryW(MAX_PATH, buff);
            return buff;
#else
            char buff[PATH_MAX + 1];
            return getcwd(buff, PATH_MAX) ? MyGUI::UString(buff).asWStr() : std::wstring();
#endif
        }

        typedef std::vector<std::wstring> VectorWString;

        inline void scanFolder(VectorWString &_result, const std::wstring &_folder, bool _recursive, const std::wstring &_mask,
                               bool _fullpath)
        {
            std::wstring folder = _folder;
            if (!folder.empty()) folder += L"/";

            VectorFileInfo result;
            getSystemFileList(result, folder, _mask);

            for (VectorFileInfo::const_iterator item = result.begin(); item != result.end(); ++item)
            {
                if (item->folder) continue;

                if (_fullpath)
                    _result.push_back(folder + item->name);
                else
                    _result.push_back(item->name);
            }

            if (_recursive)
            {
                getSystemFileList(result, folder, L"*");

                for (VectorFileInfo::const_iterator item = result.begin(); item != result.end(); ++item)
                {
                    if (!item->folder || item->name == L".." || item->name == L".") continue;
                    scanFolder(_result, folder + item->name, _recursive, _mask, _fullpath);
                }
            }
        }

    } // namespace FileSystem

} // namespace RoR
