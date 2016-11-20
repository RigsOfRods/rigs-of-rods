/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013+     Petr Ohlidal & contributors

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

#include "Application.h"
#include "RoRPrerequisites.h"
#include "MainThread.h"
#include "Language.h"
#include "ErrorUtils.h"
#include "Utils.h"
#include "PlatformUtils.h"
#include "Settings.h"

#include <OgreException.h>

#ifdef USE_CURL
#   include <curl/curl.h>
#endif //USE_CURL

GlobalEnvironment *gEnv; // Global instance used throughout the game.

#ifdef __cplusplus
extern "C" {
#endif

int main(int argc, char *argv[])
{
    using namespace RoR;

#ifdef USE_CURL
    curl_global_init(CURL_GLOBAL_ALL); // MUST init before any threads are started
#endif

    try
    {
        gEnv = new GlobalEnvironment(); // Instantiate global environment. TODO: Eliminate gEnv
        App::Init();

        // Detect system paths

        int res = RoR::System::DetectBasePaths(); // Updates globals
        if (res == -1)
        {
            ErrorUtils::ShowError(_L("Startup error"), _L("Error while retrieving program directory path"));
            return -1;
        }
        else if (res == -2)
        {
            ErrorUtils::ShowError(_L("Startup error"), _L("Error while retrieving user directory path"));
            return -1;
        }

        // Create OGRE default logger early.

        App::SetSysLogsDir(App::GetSysUserDir() + PATH_SLASH + "logs");

        auto ogre_log_manager = OGRE_NEW Ogre::LogManager();
        Ogre::String log_filepath = App::GetSysLogsDir() + PATH_SLASH + "RoR.log";
        ogre_log_manager->createLog(log_filepath, true, true);
        App::SetDiagTraceGlobals(true); // We have logger -> we can trace.

        // Setup program paths

        if (! Settings::SetupAllPaths()) // Updates globals
        {
            ErrorUtils::ShowError(_L("Startup error"), _L("Resources folder not found. Check if correctly installed."));
            return -1;
        }

        App::GetSettings().LoadSettings(App::GetSysConfigDir() + PATH_SLASH + "RoR.cfg"); // Main config file

        // Process command-line arguments

#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE //MacOSX adds an extra argument in the form of -psn_0_XXXXXX when the app is double clicked
        RoR::App::GetSettings().ProcessCommandLine(argc, argv);
#endif

        if (App::GetPendingAppState() == App::APP_STATE_PRINT_HELP_EXIT)
        {
            ShowCommandLineUsage();
            return 0;
        }
        if (App::GetPendingAppState() == App::APP_STATE_PRINT_VERSION_EXIT)
        {
            ShowVersion();
            return 0;
        }

#ifdef USE_CRASHRPT
        InstallCrashRpt();
#endif //USE_CRASHRPT

        MainThread main_obj;
        main_obj.Go();
    }
    catch (Ogre::Exception& e)
    {
        ErrorUtils::ShowError(_L("An exception has occured!"), e.getFullDescription());
    }
    catch (std::runtime_error& e)
    {
        ErrorUtils::ShowError(_L("An exception (std::runtime_error) has occured!"), e.what());
    }

#ifdef USE_CRASHRPT
    UninstallCrashRpt();
#endif //USE_CRASHRPT

    return 0;
}

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
{
    return main(__argc, __argv);
}
#endif

#ifdef __cplusplus
}
#endif
