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

#ifdef ROR_USE_OGRE_1_9
#	include <Overlay/OgreOverlayManager.h>
#	include <Overlay/OgreOverlay.h>
#else
#	include <OgreOverlayManager.h>
#	include <OgreOverlayElement.h>
#	include <OgreTextAreaOverlayElement.h>
#endif

#include <MyGUI.h>
#include <MyGUI_IFont.h>
#include <MyGUI_FontData.h>
#include <MyGUI_FontManager.h>

#include "Application.h"
#include "Settings.h"

#include <Ogre.h>

using namespace Ogre;
using namespace RoR;

LanguageEngine::LanguageEngine() : working(false), myguiConfigFilename("MyGUI_FontsEnglish.xml")
{
}

LanguageEngine::~LanguageEngine()
{
}

Ogre::String LanguageEngine::getMyGUIFontConfigFilename()
{
    // this is a fallback to the default English Resource if the language specific file was not found
    String group = "";
    try
    {
        group = ResourceGroupManager::getSingleton().findGroupContainingResource(myguiConfigFilename);
    }
    catch (...)
    {
    }
    if (group == "")
        return String("MyGUI_FontsEnglish.xml");

    return myguiConfigFilename;
}

void LanguageEngine::setup()
{
#ifdef USE_MOFILEREADER
    // load language, must happen after initializing Settings class and Ogre Root!
    // also it must happen after loading all basic resources!
    reader = new moFileLib::moFileReader();

    String language = App::GetAppLanguage();
    String language_short = App::GetAppLocale().substr(0, 2); // only first two characters are important

    // Load a .mo-File.
    LOG("*** Loading Language ***");
    std::string lang_dir = App::GetSysProcessDir() + PATH_SLASH + "languages" + PATH_SLASH;
    String langfile = lang_dir + language_short + String("/LC_MESSAGES/ror.mo");
    if (reader->ReadFile(langfile.c_str()) != moFileLib::moFileReader::EC_SUCCESS)
    {
        LOG("* error loading language file " + langfile);
        return;
    }
    working = true;

    // add resource path
    ResourceGroupManager::getSingleton().addResourceLocation(lang_dir + language_short + String("/LC_MESSAGES"), "FileSystem", "LanguageFolder");

    ResourceGroupManager::getSingleton().initialiseResourceGroup("LanguageFolder");

    // now load the code ranges
    // be aware, that this approach only works if we load just one language, and not multiple
    setupCodeRanges("code_range.txt", "LanguageFolder");
#else
    // init gettext
    bindtextdomain("ror","languages");
    textdomain("ror");

    char *curr_locale = setlocale(LC_ALL,NULL);
    if (curr_locale)
        LOG("system locale is: " + String(curr_locale));
    else
        LOG("unable to read system locale!");

    String language_short = App::GetAppLocale().substr(0, 2); // only first two characters are important
    if (!language_short.empty())
    {
        LOG("setting new locale to " + language_short);
        char *newlocale = setlocale(LC_ALL, language_short.c_str());
        if (newlocale)
            LOG("new locale is: " + String(newlocale));
        else
            LOG("error setting new locale");
    } else
    {
        LOG("not changing locale, using system locale");
    }

#endif // USE_MOFILEREADER
    LOG("* Language successfully loaded");
}

void LanguageEngine::postSetup()
{
    // set some overlay used fonts to the new font config
    String newfont = "CyberbitEnglish";
    const char* overlays[] = {"Core/CurrFps", "Core/AverageFps", "Core/WorstFps", "Core/BestFps", "Core/NumTris", "Core/DebugText", "Core/CurrMemory", "Core/MemoryText", "Core/LoadPanel/Description", "Core/LoadPanel/Comment", 0};
    for (int i = 0; overlays[i] != 0; i++)
    {
        try
        {
            Ogre::TextAreaOverlayElement* ot = (Ogre::TextAreaOverlayElement *)OverlayManager::getSingleton().getOverlayElement(overlays[i]);
            if (ot)
                ot->setFontName(newfont);
        }
        catch (...)
        {
        }
    }
}

Ogre::UTFString LanguageEngine::lookUp(Ogre::String name)
{
#ifdef USE_MOFILEREADER
    if (working)
        return reader->Lookup(name.c_str());
    return name;
#else
    return UTFString(gettext(name.c_str()));
#endif //MOFILEREADER
}

void LanguageEngine::setupCodeRanges(String codeRangesFilename, String codeRangesGroupname)
{
    // not using the default mygui font config
    myguiConfigFilename = "MyGUI_FontConfig.xml";
}
#endif //NOLANG
