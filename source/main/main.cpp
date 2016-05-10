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
	OPT_IMGPATH,
	OPT_JOINMPSERVER
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
	{ OPT_ADVLOG,         ("-advlog"),   SO_NONE    },
	{ OPT_STATE,          ("-state"),     SO_REQ_SEP    },
	{ OPT_INCLUDEPATH,    ("-includepath"),     SO_REQ_SEP    },
	{ OPT_LOGPATH,        ("-logpath"),       SO_REQ_SEP },
	{ OPT_NOCACHE,        ("-nocache"),       SO_NONE },
	{ OPT_VEHICLEOUT,     ("-vehicleout"),       SO_REQ_SEP },
	{ OPT_IMGPATH,        ("-imgpath"),       SO_REQ_SEP },
	{ OPT_JOINMPSERVER,	  ("-joinserver"),		SO_REQ_CMB },
	
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
			} else if (args.OptionId() == OPT_JOINMPSERVER) {
				String serveragrs = args.OptionArg();
				SETTINGS.setSetting("Network enable", "Yes");
				SETTINGS.setSetting("Server name", serveragrs.substr(0, serveragrs.find(":")));
				SETTINGS.setSetting("Server port", serveragrs.substr(serveragrs.find(":") + 1, serveragrs.length()));
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
