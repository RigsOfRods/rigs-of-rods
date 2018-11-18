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

/** 
    @file   ErrorUtils.cpp
    @author Thomas Fischer
    @date   3rd October 2009
*/

#include "ErrorUtils.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h> // for ShellExecuteW
#define _L
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#include "Language.h"
#endif

#ifndef NOOGRE
#endif //NOOGRE

using namespace Ogre;

int ErrorUtils::ShowError(Ogre::UTFString title, Ogre::UTFString err)
{
    Ogre::UTFString infoText = _L("An internal error occured in Rigs of Rods.\n\nTechnical details below: \n\n");
    return ErrorUtils::ShowMsgBox(_L("FATAL ERROR"), infoText + err, 0);
}

int ErrorUtils::ShowInfo(Ogre::UTFString title, Ogre::UTFString err)
{
    return ErrorUtils::ShowMsgBox(title, err, 1);
}

int ErrorUtils::ShowMsgBox(Ogre::UTFString title, Ogre::UTFString err, int type)
{
    // we might call the ErrorUtils::ShowMsgBox without having ogre created yet!
    //LOG("message box: " + title + ": " + err);
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    int mtype = MB_ICONERROR;
    if (type == 1)
        mtype = MB_ICONINFORMATION;
    MessageBoxW(NULL, err.asWStr_c_str(), title.asWStr_c_str(), MB_OK | mtype | MB_TOPMOST);
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	printf("\n\n%s: %s\n\n", title.asUTF8_c_str(), err.asUTF8_c_str());
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	printf("\n\n%s: %s\n\n", title.asUTF8_c_str(), err.asUTF8_c_str());
    //CFOptionFlags flgs;
    //CFUserNotificationDisplayAlert(0, kCFUserNotificationStopAlertLevel, NULL, NULL, NULL, T("A network error occured"), T("Bad server port."), NULL, NULL, NULL, &flgs);
#endif
    return 0;
}
