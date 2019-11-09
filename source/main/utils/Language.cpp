/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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
/// @date   12th of January 2009

#ifndef NOLANG

    #include "Language.h"

    #include "Application.h"
    #include "PlatformUtils.h"

using namespace Ogre;
using namespace RoR;

std::pair<std::string, std::string> extractLang(std::string info)
{
    String lang_long  = "";
    String lang_short = "";
    for (auto line : StringUtil::split(info, "\n"))
    {
        auto token = StringUtil::split(line);
        if (token[0] == "Language-Team:")
            lang_long = token[1];
        else if (token[0] == "Language:")
            lang_short = token[1];
    }
    return {lang_long, lang_short};
}

void LanguageEngine::setup()
{
    // load language, must happen after initializing Settings class and Ogre Root!

    languages          = {{"English", "en"}};
    auto &moFileReader = moFileLib::moFileReaderSingleton::GetInstance();

    String base_path = PathCombine(App::sys_process_dir.GetActive(), "languages");
    ResourceGroupManager::getSingleton().addResourceLocation(base_path, "FileSystem", "LngRG");
    FileInfoListPtr fl = ResourceGroupManager::getSingleton().findResourceFileInfo("LngRG", "*", true);
    if (!fl->empty())
    {
        for (auto file : *fl)
        {
            String locale_path = PathCombine(base_path, file.filename);
            String lang_path   = PathCombine(locale_path, "LC_MESSAGES");
            String mo_path     = PathCombine(lang_path, "ror.mo");
            if (moFileReader.ReadFile(mo_path.c_str()) == moFileLib::moFileReader::EC_SUCCESS)
            {
                String info = moFileLib::moFileReaderSingleton::GetInstance().Lookup("");
                languages.push_back(extractLang(info));
            }
        }
        std::sort(languages.begin() + 1, languages.end());
        moFileReader.ClearTable();
    }
    ResourceGroupManager::getSingleton().destroyResourceGroup("LngRG");

    String language_short = App::app_language.GetActive();
    String locale_path    = PathCombine(base_path, language_short.substr(0, 2));
    String lang_path      = PathCombine(locale_path, "LC_MESSAGES");
    String mo_path        = PathCombine(lang_path, "ror.mo");

    if (moFileReader.ReadFile(mo_path.c_str()) == moFileLib::moFileReader::EC_SUCCESS)
    {
        String info = moFileLib::moFileReaderSingleton::GetInstance().Lookup("");
        App::app_language.SetActive(extractLang(info).second.c_str());
        RoR::LogFormat("[RoR|App] Loading language file '%s'", mo_path.c_str());
    }
    else
    {
        RoR::LogFormat("[RoR|App] Error loading language file: '%s'", mo_path.c_str());
        return;
    }

    RoR::Log("[RoR|App] Language successfully loaded");
}
#endif // NOLANG
