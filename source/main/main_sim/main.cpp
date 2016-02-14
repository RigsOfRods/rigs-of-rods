/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

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

#include "RoRPrerequisites.h"
#include "MainThread.h"
#include "Language.h"
#include "ErrorUtils.h"
#include "Utils.h"
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
	// Now add these two files to the error report

	// logs
	crAddFile2((SSETTING("Log Path", "") + "RoR.log").c_str(), "RoR.log", "Rigs of Rods Log", CR_AF_FILE_MUST_EXIST);
	crAddFile2((SSETTING("Log Path", "") + "mygui.log").c_str(), "mygui.log", "Rigs of Rods GUI Log", CR_AF_FILE_MUST_EXIST);
	crAddFile2((SSETTING("Log Path", "") + "configlog.txt").c_str(), "configlog.txt", "Rigs of Rods Configurator Log", CR_AF_FILE_MUST_EXIST);
	//crAddFile2((SSETTING("Program Path", "") + "wizard.log").c_str(), "wizard.log", "Rigs of Rods Installer Log", CR_AF_FILE_MUST_EXIST);

	// cache
	crAddFile2((SSETTING("Cache Path", "") + "mods.cache").c_str(), "mods.cache", "Rigs of Rods Cache File", CR_AF_FILE_MUST_EXIST);

	// configs
	crAddFile2((SSETTING("Config Root", "") + "ground_models.cfg").c_str(), "ground_models.cfg", "Rigs of Rods Ground Configuration", CR_AF_FILE_MUST_EXIST);
	crAddFile2((SSETTING("Config Root", "") + "input.map").c_str(), "input.map", "Rigs of Rods Input Configuration", CR_AF_FILE_MUST_EXIST);
	crAddFile2((SSETTING("Config Root", "") + "ogre.cfg").c_str(), "ogre.cfg", "Rigs of Rods Renderer Configuration", CR_AF_FILE_MUST_EXIST);
	crAddFile2((SSETTING("Config Root", "") + "RoR.cfg").c_str(), "RoR.cfg", "Rigs of Rods Configuration", CR_AF_FILE_MUST_EXIST);

	crAddProperty("Version", ROR_VERSION_STRING);
	crAddProperty("protocol_version", RORNET_VERSION);
	crAddProperty("build_date", __DATE__);
	crAddProperty("build_time", __TIME__);

	crAddProperty("System_GUID", SSETTING("GUID", "None").c_str());
	crAddProperty("Multiplayer", (BSETTING("Network enable", "No")) ? "1" : "0");

	crAddScreenshot2(CR_AS_MAIN_WINDOW, 0);

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
	info.pszEmailTo = "error-report@rigsofrods.com";

	char tmp[512]="";
	sprintf(tmp, "http://api.rigsofrods.com/crashreport/?version=%s_%s", __DATE__, __TIME__);
	for (unsigned int i=0;i<strnlen(tmp, 512);i++)
	{
		if (tmp[i] == ' ')
			tmp[i] = '_';
	}

	info.pszUrl = tmp;
	info.uPriorities[CR_HTTP]  = 3;  // Try HTTP the first
	info.uPriorities[CR_SMTP]  = 2;  // Try SMTP the second
	info.uPriorities[CR_SMAPI] = 1; // Try Simple MAPI the last
	info.dwFlags = CR_INST_ALL_POSSIBLE_HANDLERS; // Install all available exception handlers
	info.pszPrivacyPolicyURL = "http://wiki.rigsofrods.com/pages/Crash_Report_Privacy_Policy"; // URL for the Privacy Policy link

	crSetCrashCallback(CrashCallback, NULL);

	int nInstResult = crInstall(&info);
	if (nInstResult!=0)
	{
		// Something goes wrong!
		TCHAR szErrorMsg[512];
		szErrorMsg[0]=0;

		crGetLastErrorMsg(szErrorMsg, 512);
		printf("%s\n", szErrorMsg);


		ErrorUtils::ShowError(_L("Exception handling registration problem"), String(szErrorMsg));

		assert(nInstResult==0);
	}
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

// simpleopt by http://code.jellycan.com/simpleopt/
// license: MIT
#include "SimpleOpt.h"

// option identifiers
enum {
	OPT_HELP,
	OPT_MAP,
	OPT_TRUCK,
	OPT_SETUP,
	OPT_CMD,
	OPT_WDIR,
	OPT_ETM,
	OPT_CONFIG,
	OPT_VER,
	OPT_CHECKCACHE,
	OPT_TRUCKCONFIG,
	OPT_ENTERTRUCK,
	OPT_BENCH,
	OPT_STREAMCACHEGEN,
	OPT_BENCHNUM,
	OPT_USERPATH,
	OPT_BENCHPOS,
	OPT_BENCHPOSERR,
	OPT_NOCRASHCRPT,
	OPT_STATE,
	OPT_INCLUDEPATH,
	OPT_LOGPATH,
	OPT_ADVLOG,
	OPT_REPOMODE,
	OPT_VEHICLEOUT,
	OPT_NOCACHE,
	OPT_IMGPATH
};

// option array
CSimpleOpt::SOption cmdline_options[] = {
	{ OPT_MAP,            ("-map"),         SO_REQ_SEP },
	{ OPT_MAP,            ("-terrain"),     SO_REQ_SEP },
	{ OPT_TRUCK,          ("-truck"),       SO_REQ_SEP },
	{ OPT_ENTERTRUCK,     ("-enter"),       SO_NONE },
	{ OPT_CMD,            ("-cmd"),         SO_REQ_SEP },
	{ OPT_WDIR,           ("-wd"),          SO_REQ_SEP },
	{ OPT_SETUP,          ("-setup"),       SO_NONE    },
	{ OPT_CONFIG,         ("-config"),      SO_NONE    },
	{ OPT_TRUCKCONFIG,    ("-truckconfig"), SO_REQ_SEP    },
	{ OPT_HELP,           ("--help"),       SO_NONE    },
	{ OPT_HELP,           ("-help"),        SO_NONE    },
	{ OPT_CHECKCACHE,     ("-checkcache"),  SO_NONE    },
	{ OPT_VER,            ("-version"),     SO_NONE    },
	{ OPT_USERPATH,       ("-userpath"),   SO_REQ_SEP    },
	{ OPT_BENCH,          ("-benchmark"),   SO_REQ_SEP    },
	{ OPT_BENCHPOS,       ("-benchmark-final-position"),   SO_REQ_SEP    },
	{ OPT_BENCHPOSERR,    ("-benchmark-final-position-error"),   SO_REQ_SEP    },
	{ OPT_BENCHNUM,       ("-benchmarktrucks"),       SO_REQ_SEP },
	{ OPT_BENCHNUM,       ("-benchmark-trucks"),       SO_REQ_SEP },
	{ OPT_STREAMCACHEGEN, ("-streamcachegen"),   SO_NONE    },
	{ OPT_NOCRASHCRPT,    ("-nocrashrpt"),   SO_NONE    },
	{ OPT_ADVLOG,         ("-advlog"),   SO_NONE    },
	{ OPT_STATE,          ("-state"),     SO_REQ_SEP    },
	{ OPT_INCLUDEPATH,    ("-includepath"),     SO_REQ_SEP    },
	{ OPT_LOGPATH,        ("-logpath"),       SO_REQ_SEP },
	{ OPT_NOCACHE,        ("-nocache"),       SO_NONE },
	{ OPT_VEHICLEOUT,     ("-vehicleout"),       SO_REQ_SEP },
	{ OPT_IMGPATH,        ("-imgpath"),       SO_REQ_SEP },

SO_END_OF_OPTIONS
};

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
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	//start working dir is highly unpredictable in MacOSX (typically you start in "/"!)
	//oh, thats quite hacked - thomas
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

	RoR::MainThread main_thread_object;

//MacOSX adds an extra argument in the for of -psn_0_XXXXXX when the app is double clicked
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE
	CSimpleOpt args(argc, argv, cmdline_options);
	while (args.Next()) {
		if (args.LastError() == SO_SUCCESS) {
			if (args.OptionId() == OPT_HELP) {
				showUsage();
				return 0;
			} else if (args.OptionId() == OPT_TRUCK) {
				SETTINGS.setSetting("Preselected Truck", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_TRUCKCONFIG) {
				SETTINGS.setSetting("Preselected TruckConfig", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_MAP) {
				SETTINGS.setSetting("Preselected Map", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_CMD) {
				SETTINGS.setSetting("cmdline CMD", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_BENCH) {
				SETTINGS.setSetting("Benchmark", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_BENCHNUM) {
				SETTINGS.setSetting("BenchmarkTrucks", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_BENCHPOS) {
				SETTINGS.setSetting("BenchmarkFinalPosition", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_BENCHPOSERR) {
				SETTINGS.setSetting("BenchmarkFinalPositionError", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_NOCRASHCRPT) {
				SETTINGS.setSetting("NoCrashRpt", "Yes");
			} else if (args.OptionId() == OPT_USERPATH) {
				SETTINGS.setSetting("userpath", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_CONFIG) {
				SETTINGS.setSetting("configfile", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_IMGPATH) {
				SETTINGS.setSetting("OPT_IMGPATH", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_WDIR) {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
				SetCurrentDirectory(args.OptionArg());
#endif
			} else if (args.OptionId() == OPT_STATE) {
				SETTINGS.setSetting("StartState", args.OptionArg());
			} else if (args.OptionId() == OPT_NOCACHE) {
				SETTINGS.setSetting("NOCACHE", "Yes");
			} else if (args.OptionId() == OPT_LOGPATH) {
				SETTINGS.setSetting("Enforce Log Path", args.OptionArg());
			} else if (args.OptionId() == OPT_ADVLOG) {
				SETTINGS.setSetting("Advanced Logging", "Yes");
			} else if (args.OptionId() == OPT_INCLUDEPATH) {
				SETTINGS.setSetting("resourceIncludePath", args.OptionArg());
			} else if (args.OptionId() == OPT_STREAMCACHEGEN) {
				SETTINGS.setSetting("streamCacheGenerationOnly", "Yes");
			} else if (args.OptionId() == OPT_CHECKCACHE) {
				// just regen cache and exit
				SETTINGS.setSetting("regen-cache-only", "Yes");
			} else if (args.OptionId() == OPT_ENTERTRUCK) {
				SETTINGS.setSetting("Enter Preselected Truck", "Yes");
			} else if (args.OptionId() == OPT_SETUP) {
				SETTINGS.setSetting("USE_OGRE_CONFIG", "Yes");
			} else if (args.OptionId() == OPT_VEHICLEOUT) {
				SETTINGS.setSetting("vehicleOutputFile", args.OptionArg());
			} else if (args.OptionId() == OPT_VER) {
				showVersion();
				return 0;
			}
		}
		else
		{
			showUsage();
			return 1;
		}
	}
#endif

#ifdef USE_CRASHRPT
	if (!BSETTING("NoCrashRpt", "No"))
		install_crashrpt();

	//test_crashrpt();
#endif //USE_CRASHRPT

	try
	{
		main_thread_object.Go();
	}
	catch (Ogre::Exception& e)
	{
		String url = "http://wiki.rigsofrods.com/index.php?title=Error_" + TOSTRING(e.getNumber())+"#"+e.getSource();
		ErrorUtils::ShowOgreWebError(_L("An exception has occured!"), e.getFullDescription(), url);
	}
	catch (std::runtime_error& e)
	{
		ErrorUtils::ShowError(_L("An exception (std::runtime_error) has occured!"), e.what());
	}

#ifdef USE_CRASHRPT
	if (!BSETTING("NoCrashRpt", "No"))
		uninstall_crashrpt();
#endif //USE_CRASHRPT

	// show errors before we give up
	ErrorUtils::ShowStoredOgreWebErrors();

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
