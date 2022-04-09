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

#include "Language.h"

#include "Application.h"
#include "Console.h"
#include "PlatformUtils.h"

#include <OgreFileSystem.h>

using namespace Ogre;
using namespace RoR;

std::pair<std::string, std::string> extractLang(const std::string& info)
{
    String lang_long  = "";
    String lang_short = "";
    for (const auto& line : StringUtil::split(info, "\n"))
    {
        auto token = StringUtil::split(line);
        if (token[0] == "Language-Team:")
            lang_long = token[1];
        else if (token[0] == "Language:")
            lang_short = token[1];
    }
    return {lang_long, lang_short};
}

bool loadMoFile(std::string const& dir, std::string const& filename, bool ignoreMissing)
{
    try
    {
        Ogre::FileSystemArchiveFactory factory;
        Ogre::Archive* archive = factory.createInstance(dir);
        if (ignoreMissing && !archive->exists(filename))
        {
            return false;
        }
        Ogre::DataStreamPtr stream = archive->open(filename);
        moFileLib::moFileReader::eErrorCode mo_result =
            moFileLib::moFileReaderSingleton::GetInstance().ParseData(stream->getAsString());
        stream->close();
        factory.destroyInstance(archive);
        return mo_result == moFileLib::moFileReader::EC_SUCCESS;
    }
    catch (Ogre::Exception& ex)
    {
        App::GetConsole()->putMessage(
            Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
            fmt::format("Could not load language file {}/{}, message: {}", dir, filename, ex.getFullDescription()));
        return false;
    }
}

void LanguageEngine::setup()
{
    // load language, must happen after initializing Settings class and Ogre Root!

    languages = { {"English", "en"} };
    auto& moFileReader = moFileLib::moFileReaderSingleton::GetInstance();

    String base_path = PathCombine(App::sys_process_dir->getStr(), "languages");
    ResourceGroupManager::getSingleton().addResourceLocation(base_path, "FileSystem", "LngRG");
    FileInfoListPtr fl = ResourceGroupManager::getSingleton().findResourceFileInfo("LngRG", "*", true);
    if (!fl->empty())
    {
        for (const auto& file : *fl)
        {
            String locale_path = PathCombine(base_path, file.filename);
            if (loadMoFile(locale_path, "ror.mo", /*ignoreMissing:*/true)) // Logs error to console.
            {
                String info = moFileLib::moFileReaderSingleton::GetInstance().Lookup("");
                languages.push_back(extractLang(info));
            }
        }
        std::sort(languages.begin() + 1, languages.end());
        moFileReader.ClearTable();
    }
    ResourceGroupManager::getSingleton().destroyResourceGroup("LngRG");

    String locale_path = PathCombine(base_path, App::app_language->getStr().substr(0, 2));

    if (loadMoFile(locale_path, "ror.mo", /*ignoreMissing:*/false)) // Logs error to console.
    {
        String info = moFileLib::moFileReaderSingleton::GetInstance().Lookup("");
        App::app_language->setStr(extractLang(info).second);
        RoR::LogFormat("[RoR|App] Language '%s' successfully loaded", App::app_language->getStr().c_str());
    }
}
