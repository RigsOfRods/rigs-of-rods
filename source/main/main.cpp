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
	along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Application.h"
#include "RoRPrerequisites.h"
#include "MainThread.h"
#include "Language.h"
#include "ErrorUtils.h"
#include "Utils.h"
#include "PlatformUtils.h"
#include "Settings.h"
#include "rornet.h"
#include "RoRVersion.h"

#include <OgreException.h>

using namespace Ogre;

#ifdef USE_CRASHRPT
// see http://crashrpt.sourceforge.net/
#include "crashrpt.h"

// Define the crash callback
int CALLBACK CrashCallback(CR_CRASH_CALLBACK_INFO* pInfo)
{
	// Return CR_CB_DODEFAULT to generate error report
	return CR_CB_DODEFAULT;
}

void install_crashrpt()
{
	// Install CrashRpt support
	CR_INSTALL_INFO info;
	memset(&info, 0, sizeof(CR_INSTALL_INFO));
	info.cb = sizeof(CR_INSTALL_INFO);
	info.pszAppName = "Rigs of Rods";
	info.pszAppVersion = ROR_VERSION_STRING;
	info.pszEmailSubject = "Error Report for Rigs of Rods";
	info.pszEmailTo = "error-report@rigsofrods.org";
	info.pszUrl = "http://crashfix.rigsofrods.org/index.php/crashReport/uploadExternal";
	info.uPriorities[CR_HTTP] = 1;                      // Use HTTP.
	info.uPriorities[CR_SMTP] = CR_NEGATIVE_PRIORITY;   // Not user SMTP.
	info.uPriorities[CR_SMAPI] = CR_NEGATIVE_PRIORITY;  // Not use Simple MAPI.  
	info.dwFlags = CR_INST_AUTO_THREAD_HANDLERS; // Install the per-thread exception handlers automatically
	info.dwFlags |= CR_INST_ALL_POSSIBLE_HANDLERS; // Install all available exception handlers
	info.dwFlags |= CR_INST_SHOW_ADDITIONAL_INFO_FIELDS; // Makes "Your E-mail" and "Describe what you were doing when the problem occurred" fields of Error Report dialog always visible.
	info.pszPrivacyPolicyURL = "http://docs.rigsofrods.org/legal/crash-report-privacy-policy/"; // URL for the Privacy Policy link

	crSetCrashCallback(CrashCallback, NULL);

	int nInstResult = crInstall(&info);
	if (nInstResult != 0)
	{
		// Something goes wrong!
		TCHAR szErrorMsg[512];
		szErrorMsg[0] = 0;

		crGetLastErrorMsg(szErrorMsg, 512);
		printf("%s\n", szErrorMsg);


		ErrorUtils::ShowError(_L("Exception handling registration problem"), String(szErrorMsg));

		assert(nInstResult == 0);
	}

	// logs
	crAddFile2((RoR::App::GetSysLogsDir() + PATH_SLASH + "RoR.log").c_str(), "RoR.log", "Rigs of Rods Log", CR_AF_FILE_MUST_EXIST);
	crAddFile2((RoR::App::GetSysLogsDir() + PATH_SLASH + "mygui.log").c_str(), "mygui.log", "Rigs of Rods GUI Log", CR_AF_FILE_MUST_EXIST);
	crAddFile2((RoR::App::GetSysLogsDir() + PATH_SLASH + "configlog.txt").c_str(), "configlog.txt", "Rigs of Rods Configurator Log", CR_AF_FILE_MUST_EXIST);

	// cache
	crAddFile2((RoR::App::GetSysCacheDir() + PATH_SLASH + "mods.cache").c_str(), "mods.cache", "Rigs of Rods Cache File", CR_AF_FILE_MUST_EXIST);

	// configs
	crAddFile2((RoR::App::GetSysConfigDir() + PATH_SLASH + "ground_models.cfg").c_str(), "ground_models.cfg", "Rigs of Rods Ground Configuration", CR_AF_FILE_MUST_EXIST);
	crAddFile2((RoR::App::GetSysConfigDir() + PATH_SLASH + "input.map").c_str(), "input.map", "Rigs of Rods Input Configuration", CR_AF_FILE_MUST_EXIST);
	crAddFile2((RoR::App::GetSysConfigDir() + PATH_SLASH + "ogre.cfg").c_str(), "ogre.cfg", "Rigs of Rods Renderer Configuration", CR_AF_FILE_MUST_EXIST);
	crAddFile2((RoR::App::GetSysConfigDir() + PATH_SLASH + "RoR.cfg").c_str(), "RoR.cfg", "Rigs of Rods Configuration", CR_AF_FILE_MUST_EXIST);

	crAddProperty("Version", ROR_VERSION_STRING);
	crAddProperty("protocol_version", RORNET_VERSION);
	crAddProperty("build_date", __DATE__);
	crAddProperty("build_time", __TIME__);

	crAddProperty("System_GUID", SSETTING("GUID", "None").c_str());
	crAddProperty("Multiplayer", RoR::App::GetActiveMpState() ? "1" : "0");

	crAddScreenshot2(CR_AS_MAIN_WINDOW, 0);
}

void uninstall_crashrpt()
{
	crUninstall();
}

void test_crashrpt()
{
	//generate a null pointer exception.
	crEmulateCrash(CR_SEH_EXCEPTION);
}
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif //WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX // required to stop windows.h messing up std::min
#endif //NOMINMAX
#include "windows.h"
#include "ShellAPI.h"
#endif //OGRE_PLATFORM_WIN32

#ifdef USE_CURL
#   include <curl/curl.h>
#endif //USE_CURL

// Global instance of GlobalEnvironment used throughout the game.
GlobalEnvironment *gEnv; 

#ifdef __cplusplus
extern "C" {
#endif

void showUsage()
{
	ErrorUtils::ShowInfo(_L("Command Line Arguments"), _L("--help (this)\n-map <map> (loads map on startup)\n-truck <truck> (loads truck on startup)\n-setup shows the ogre configurator\n-version shows the version information\n-enter enters the selected truck\n-userpath <path> sets the user directory\nFor example: RoR.exe -map oahu -truck semi"));
}

void showVersion()
{
	ErrorUtils::ShowInfo(_L("Version Information"), getVersionString());
#ifdef __GNUC__
	printf(" * built with gcc %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif //__GNUC__
}

int main(int argc, char *argv[])
{
    using namespace RoR;

#ifdef USE_CURL
    curl_global_init(CURL_GLOBAL_ALL); // MUST init before any threads are started
#endif

    try
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
        //start working dir is highly unpredictable in MacOSX (typically you start in "/"!)
        //oh, thats quite hacked - thomas
        // TODO: Is this even required anymore? ~only_a_ptr, 09/2016
        char str[256];
        strcpy(str, argv[0]);
        char *pt=str+strlen(str);
        while (*pt!='/') pt--;
        *pt=0;
        chdir(str);
        chdir("../../..");
        getwd(str);
        printf("GETWD=%s\n", str);
#endif

        gEnv = new GlobalEnvironment(); // Instantiate global environment. TODO: Eliminate gEnv
        App::Init();
        
    // ----------------------------------
    // Detect system paths
    
    int res = RoR::System::DetectBasePaths(); // Updates globals
    if (res == -1)
    {
        ErrorUtils::ShowError(_L("Startup error"), _L("Error while retrieving program space path"));
        return -1;
    }
    else if (res == -2)
    {
        ErrorUtils::ShowError(_L("Startup error"), _L("Error while retrieving user space path"));
        return -1;
    }

    // --------------------------------
    // Create OGRE default logger early.

    App::SetSysLogsDir(App::GetSysUserDir() + PATH_SLASH + "logs");
    
    auto ogre_log_manager = OGRE_NEW LogManager();
    Ogre::String log_filepath = App::GetSysLogsDir() + PATH_SLASH + "RoR.log";
    ogre_log_manager->createLog(log_filepath, true, true);
    App::SetDiagTraceGlobals(true); // We have logger -> we can trace.
    
    // --------------------------------
    // Setup program paths

    if (! Settings::SetupAllPaths()) // Updates globals
    {
        ErrorUtils::ShowError(_L("Startup error"), _L("Resources folder not found. Check if correctly installed."));
        return -1;
    }

	App::GetSettings().LoadSettings(App::GetSysConfigDir() + PATH_SLASH + "RoR.cfg"); // Main config file

    // -------------------------------
    // Process command-line arguments
    
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE //MacOSX adds an extra argument in the form of -psn_0_XXXXXX when the app is double clicked
        RoR::App::GetSettings().ProcessCommandLine(argc, argv);
#endif

        if (App::GetPendingAppState() == App::APP_STATE_PRINT_HELP_EXIT)
        {
            showUsage();
            return 0;
        }
        else if (App::GetPendingAppState() == App::APP_STATE_PRINT_VERSION_EXIT)
        {
            showVersion();
            return 0;
        }

#ifdef USE_CRASHRPT
		install_crashrpt();
		//test_crashrpt();
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
	uninstall_crashrpt();
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
