/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009 Pierre-Michel Ricordel
Copyright 2007,2008,2009 Thomas Fischer

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
#include "platform.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	#include "wx/msw/private.h"
	#include "wx/msw/registry.h"
	#include <shlobj.h>
#endif // OGRE_PLATFORM

#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/msgdlg.h>

#include "ConfigManager.h"
#include "wthread.h"
#include "wsyncdownload.h"
#include "utils.h"
#include "installerlog.h"
#include "SHA1.h"
#include "wizard.h"
#include "RoRVersion.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	#include <shlobj.h> // for the special path functions
	#include "symlink.h"
	#include <shellapi.h> // for executing the binaries
#endif //OGRE_PLATFORM

#include "boost/filesystem.hpp"

//icons must be 50x50
#include "unknown.xpm"
#include "mainpack.xpm"
#include "extrapack.xpm"
#include <string>

ConfigManager *ConfigManager::instance = 0;

ConfigManager::ConfigManager() : currVersion()
{
	ConfigManager::instance = this;
	installPath = wxString();
	dlerror=0;
	enforceUpdate=false;
}

int ConfigManager::getCurrentVersionInfo()
{
	// get the most recent version information
	WsyncDownload *wsdl = new WsyncDownload();
	std::vector< std::vector< std::string > > list;
	char tmp[256] = "";
	sprintf(tmp, "/%s/%s/%s", ROR_VERSION_STRING_SHORT, INSTALLER_PLATFORM, WSYNC_VERSION_INFO);
	int res = wsdl->downloadConfigFile(0, WSYNC_MAIN_SERVER, std::string(tmp), list);
	delete(wsdl);
	if(!res && list.size()>0 && list[0].size()>0)
	{
		currVersion = list[0][0];
		return 0;
	} else
	{
		currVersion = std::string(" unknown "); // spaces intentionally
	}
	return 1;
}

#ifdef WIN32
void wxStringToTCHAR(TCHAR *tCharString, wxString &myString)
{
	const wxChar* myStringChars = myString.c_str();
	for (unsigned int i = 0; i < myString.Len(); i++) {
		tCharString[i] = myStringChars [i];
	}
	tCharString[myString.Len()] = _T('\0');
}

#endif //WIN32
std::string ConfigManager::readVersionInfo()
{
#ifdef WIN32
	// http://stackoverflow.com/questions/940707/how-do-i-programatically-get-the-version-of-a-dll-or-exe

	wxString rorpath = getInstallationPath() + wxT("RoR.exe");

	char buffer[4096]="";
	TCHAR pszFilePath[MAX_PATH];
	
	wxStringToTCHAR(pszFilePath, rorpath);

	DWORD               dwSize              = 0;
	BYTE                *pbVersionInfo      = NULL;
	VS_FIXEDFILEINFO    *pFileInfo          = NULL;
	UINT                puLenFileInfo       = 0;

	// get the version info for the file requested
	dwSize = GetFileVersionInfoSize( pszFilePath, NULL );
	if ( dwSize == 0 )
	{
		//wxMessageBox(wxString::Format("Error in GetFileVersionInfoSize: %d\n", GetLastError()),wxT("GetFileVersionInfoSize Error"),0);
		return "unknown";
	}

	pbVersionInfo = new BYTE[ dwSize ];

	if ( !GetFileVersionInfo( pszFilePath, 0, dwSize, pbVersionInfo ) )
	{
		//printf( "Error in GetFileVersionInfo: %d\n", GetLastError() );
		//delete[] pbVersionInfo;
		return "unknown";
	}

	if ( !VerQueryValue( pbVersionInfo, TEXT("\\"), (LPVOID*) &pFileInfo, &puLenFileInfo ) )
	{
		//printf( "Error in VerQueryValue: %d\n", GetLastError() );
		//delete[] pbVersionInfo;
		return "unknown";
	}

	if (pFileInfo->dwSignature == 0xfeef04bd)
	{
		int major = HIWORD(pFileInfo->dwFileVersionMS);
		int minor = LOWORD(pFileInfo->dwFileVersionMS);
		int patch = HIWORD(pFileInfo->dwFileVersionLS);
		int rev   = LOWORD(pFileInfo->dwFileVersionLS);
		sprintf(buffer, "%d.%d.%d", major, minor, patch);
		return std::string(buffer);
	}
#endif // WIN32
	return "unknown";
}

wxString ConfigManager::getInstallationPath()
{
	wxString path = getExecutableBasePath();
	path += wxT("\\");

	if(!enforceUpdate)
	{
		// check if RoR.exe exists
		bool exists = wxFileExists(path+wxT("RoR.exe"));
		if(!exists)
		{
			wxMessageBox(wxT("No RoR.exe found, please install the game before trying to update"),wxT("Update Error"),0);
			exit(1);
		}
	}
	return path;
}

bool ConfigManager::getUserPathExists()
{
	return wxFileName::DirExists(getUserPath());
}

wxString ConfigManager::getUserPath()
{
#ifdef WIN32
	WCHAR Wuser_path[1024];

	if (SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, Wuser_path)!=S_OK)
	{
		return wxString();
	}
	GetShortPathName(Wuser_path, Wuser_path, 512); //this is legal
	wxFileName tfn=wxFileName(Wuser_path, wxEmptyString);
	// TODO: fix hardcoded value here
	tfn.AppendDir(wxT("Rigs of Rods ") + wxString(ROR_VERSION_STRING_SHORT));
	return tfn.GetPath();
#endif // WIN32
}

void ConfigManager::associateViewerFileTypes(std::string type)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	// add the icon
	wxRegKey *pRegKey = new wxRegKey(wxT("HKEY_CLASSES_ROOT\\" + type + "\\DefaultIcon"));
	if(!pRegKey->Exists())
		pRegKey->Create();

	pRegKey->SetValue("", installPath + "\\RoRViewer.exe,0");

	// add the action
	pRegKey = new wxRegKey(wxT("HKEY_CLASSES_ROOT\\" + type + "\\shell\\open\\command"));
	if(!pRegKey->Exists())
		pRegKey->Create();

	pRegKey->SetValue("", installPath + "\\RoRViewer.exe \"%1\" ");
#else
	// TODO: implement
#endif //OGRE_PLATFORM
}
int ConfigManager::associateFileTypes()
{
	associateViewerFileTypes(".mesh");
	return 0;
}

void ConfigManager::setPersistentConfig(wxString name, wxString value)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	wxRegKey *pRegKey = new wxRegKey(wxT("HKEY_LOCAL_MACHINE\\Software\\RigsOfRods\\config"));
	if(!pRegKey->Exists())
		pRegKey->Create();
	pRegKey->SetValue(name, value);
#else
	// TODO: implement
#endif //OGRE_PLATFORM
}

wxString ConfigManager::getPersistentConfig(wxString name)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	wxRegKey *pRegKey = new wxRegKey(wxT("HKEY_LOCAL_MACHINE\\Software\\RigsOfRods\\config"));
	if(!pRegKey->Exists())
		return wxT("");

	if(!pRegKey->HasValue(name))
		return wxT("");

	wxString result;
	pRegKey->QueryValue(name, result);
	return result;
#else
	// TODO: implement
	return wxString();
#endif //OGRE_PLATFORM
}

void ConfigManager::updateUserConfigFile(std::string filename, boost::filesystem::path iPath, boost::filesystem::path uPath)
{
	boost::filesystem::path fPath = iPath / filename;
	boost::filesystem::path dfPath = uPath / filename;
	if(boost::filesystem::exists(dfPath))
	{
		WsyncDownload::tryRemoveFile(dfPath);
	}
	LOG("updating file: %s ... ", fPath.string().c_str());

	bool ok = boost::filesystem::is_regular_file(fPath);
	if(ok)
	{
		try
		{
			boost::filesystem::copy_file(fPath, dfPath);
			ok=true;
		} catch(...)
		{
			ok=false;
		}
	}

	LOG("%s\n", ok?"ok":"error");
}

void ConfigManager::updateUserConfigs()
{
	LOG("==== updating user configs ... \n");

	boost::filesystem::path iPath = boost::filesystem::path (conv(getInstallationPath()));
	iPath = iPath / std::string("skeleton") / std::string("config");

	boost::filesystem::path uPath;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	LPWSTR wuser_path = new wchar_t[1024];
	if (!SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, wuser_path)!=S_OK)
	{
		GetShortPathName(wuser_path, wuser_path, 512); //this is legal
		uPath = wstrtostr(std::wstring(wuser_path));
		uPath = uPath / (std::string("Rigs of Rods ") + wxT(ROR_VERSION_STRING_SHORT)) / std::string("config");;
	}
#else
	// XXX : TODO
#endif // OGRE_PLATFORM

	// check directories

	LOG("installation path: %s ... ", iPath.string().c_str());
	bool ok = boost::filesystem::is_directory(iPath);
	LOG("%s\n", ok?"ok":"error");

	LOG("user path: %s ... ", uPath.string().c_str());
	ok = boost::filesystem::is_directory(uPath);
	LOG("%s\n", ok?"ok":"error");

	updateUserConfigFile(std::string("categories.cfg"), iPath, uPath);
	updateUserConfigFile(std::string("ground_models.cfg"), iPath, uPath);
	updateUserConfigFile(std::string("torque_models.cfg"), iPath, uPath);
	updateUserConfigFile(std::string("wavefield.cfg"), iPath, uPath);
}

void ConfigManager::installRuntime()
{
	// obsolete
	wxMessageBox(wxT("Will now install DirectX. Please click ok to continue"), wxT("directx"), wxICON_INFORMATION | wxOK);
	executeBinary(wxT("dxwebsetup.exe"));
	wxMessageBox(wxT("Please wait until the DirectX installation is done and click ok to continue"), wxT("directx"), wxICON_INFORMATION | wxOK);
	wxMessageBox(wxT("Will now install the Visual Studio runtime. Please click ok to continue."), wxT("runtime"), wxICON_INFORMATION | wxOK);
	executeBinary(wxT("msiexec.exe"), wxT("runas"), wxT("/i \"") + CONFIG->getInstallationPath() + wxT("\\VCCRT4.msi\""), wxT("cwd"), false);
}

void ConfigManager::startConfigurator()
{
	executeBinary(wxT("rorconfig.exe"), wxT("runas"), wxT(""), CONFIG->getInstallationPath());
}

void ConfigManager::viewManual()
{
	executeBinary(wxT("Things_you_can_do_in_Rigs_of_Rods.pdf"), wxT("open"));
	executeBinary(wxT("keysheet.pdf"), wxT("open"));
}

void ConfigManager::viewChangelog()
{
	wxString changeLogURL = wxT(CHANGELOGURL) + conv(currVersion);
	wxLaunchDefaultBrowser(changeLogURL);
}

void ConfigManager::executeBinary(wxString filename, wxString action, wxString parameters, wxString cwDir, bool prependCWD)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	char path[2048]= "";
	if(prependCWD)
		filename = CONFIG->getInstallationPath() + "\\" + filename;

	// now construct struct that has the required starting info
	SHELLEXECUTEINFO sei = { sizeof(sei) };
	sei.cbSize = sizeof(SHELLEXECUTEINFOA);
	sei.fMask = 0;
	sei.hwnd = NULL;
	sei.lpVerb = action;
	sei.lpFile = filename;
	sei.lpParameters = parameters;
	if(cwDir == wxT("cwd"))
		sei.lpDirectory = CONFIG->getInstallationPath();
	else
		sei.lpDirectory = cwDir;

	sei.nShow = SW_NORMAL;
	ShellExecuteEx(&sei);
#endif //OGRE_PLATFORM
}

void ConfigManager::createProgramLinks(bool desktop, bool startmenu)
{
	// XXX: TODO: ADD LINUX code!
	// create shortcuts
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	wxString startmenuDir, desktopDir, workingDirectory;

	// XXX: ADD PROPER ERROR HANDLING
	// CSIDL_COMMON_PROGRAMS = start menu for all users
	// CSIDL_PROGRAMS = start menu for current user only
	if(!SHGetSpecialFolderPath(0, wxStringBuffer(startmenuDir, MAX_PATH), CSIDL_COMMON_PROGRAMS, FALSE))
		return;

	// same with CSIDL_COMMON_DESKTOPDIRECTORY and CSIDL_DESKTOP
	if(!SHGetSpecialFolderPath(0, wxStringBuffer(desktopDir, MAX_PATH), CSIDL_DESKTOP, FALSE))
		return;

	workingDirectory = CONFIG->getInstallationPath();
	if(workingDirectory.size() > 3 && workingDirectory.substr(workingDirectory.size()-1,1) != wxT("\\"))
	{
		workingDirectory += wxT("\\");
	}
	// ensure our directory in the start menu exists
	startmenuDir += wxT("\\Rigs of Rods");
	if (!wxDir::Exists(startmenuDir)) wxFileName::Mkdir(startmenuDir);

	// the actual linking
	if(desktop)
		createShortcut(workingDirectory + wxT("rorconfig.exe"), workingDirectory, desktopDir + wxT("\\Rigs of Rods.lnk"), wxT("start Rigs of Rods"));

	if(startmenu)
	{
		createShortcut(workingDirectory + wxT("RoR.exe"), workingDirectory, startmenuDir + wxT("\\Rigs of Rods.lnk"), wxT("start Rigs of Rods"));
		createShortcut(workingDirectory + wxT("rorconfig.exe"), workingDirectory, startmenuDir + wxT("\\Configurator.lnk"), wxT("start Rigs of Rods Configuration Program (required upon first start)"));
		createShortcut(workingDirectory + wxT("servergui.exe"), workingDirectory, startmenuDir + wxT("\\Multiplayer Server.lnk"), wxT("start Rigs of Rods multiplayer server"));
		createShortcut(workingDirectory + wxT("Things_you_can_do_in_Rigs_of_Rods.pdf"), workingDirectory, startmenuDir + wxT("\\Manual.lnk"), wxT("open the RoR Manual"));
		createShortcut(workingDirectory + wxT("keysheet.pdf"), workingDirectory, startmenuDir + wxT("\\Keysheet.lnk"), wxT("open the RoR Key Overview"));
		createShortcut(workingDirectory + wxT("updater.exe"), workingDirectory, startmenuDir + wxT("\\Updater.lnk"), wxT("open the Updater with which you can update or uninstall RoR."));
	}
#endif // OGRE_PLATFORM
}

// small wrapper that converts wxString to std::string and remvoes the file if already existing
int ConfigManager::createShortcut(wxString linkTarget, wxString workingDirectory, wxString linkFile, wxString linkDescription)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	if(wxFileExists(linkFile)) wxRemoveFile(linkFile);
	if(!wxFileExists(linkTarget)) return 1;
	return createLink(conv(linkTarget), conv(workingDirectory), conv(linkFile), conv(linkDescription));
#endif //OGRE_PLATFORM
	return 1;
}

std::string ConfigManager::getOwnHash()
{
	// now hash ourself and validate our installer version, to be sure we are using the latest installer
	std::string myPath = conv(getExecutablePath());

	CSHA1 sha1;
	bool res = sha1.HashFile(const_cast<char*>(myPath.c_str()));
	if(!res) return std::string("");
	sha1.Final();
	char resultHash[256] = "";
	sha1.ReportHash(resultHash, CSHA1::REPORT_HEX_SHORT);
	return std::string(resultHash);
}

void ConfigManager::checkForNewUpdater()
{
	std::string ourHash = getOwnHash();

    char platform_str[256]="";
#ifdef WIN32
   sprintf(platform_str, "windows");
#else
   sprintf(platform_str, "linux");
#endif

    char url_tmp[256]="";
	sprintf(url_tmp, API_CHINSTALLER, ourHash.c_str(), platform_str, ROR_VERSION_STRING_SHORT);

	WsyncDownload *wsdl = new WsyncDownload();
	LOG("checking for updates...\n");
	wsdl->setDownloadMessage(_T("checking for updates"));
	std::vector< std::vector< std::string > > list;
	int res = wsdl->downloadConfigFile(1, API_SERVER, std::string(url_tmp), list, true);
	if(!res && list.size()>0 && list[0].size()>0)
	{
		LOG("update check result: %s\n", list[0][0].c_str());
		if(list[0][0] == std::string("ok"))
		{
			// no updates
		} else if(list[0][0] == std::string("update") && list[0].size() > 2)
		{
			// yay, an update
			LOG("new update available: %s %s\n", list[0][1].c_str(), list[0][2].c_str());
			wsdl->setDownloadMessage(_T("downloading installer update"));

			// rename ourself, so we can replace ourself
			LOG("renaming self...\n");
			std::string myPath = conv(getExecutablePath());

			// removing old, otherwise renaming trows exception
			if(boost::filesystem::exists(myPath+std::string(".old")))
				boost::filesystem::remove(myPath+std::string(".old"));

			boost::filesystem::rename(myPath, myPath+std::string(".old"));

			LOG("downloading update...\n");
			int res = wsdl->downloadFile(0, myPath, list[0][1], list[0][2], 0, 0, true);
			if(!res)
			{
				wxMessageBox(_T("Installer was updated, will restart the installer now!"));

				// now start the new installer and quit ourselfs
				LOG("update downloaded, starting now...\n");
				executeBinary(conv(myPath), wxT("runas"), wxT(""), wxT("cwd"), false);
				exit(1);
			} else
			{
				LOG("update download failed with error %d\n", res);
			}
		}
	}
	delete(wsdl);
}

void path_descend(char* path)
{
	// WINDOWS ONLY
	char dirsep='\\';
	char* pt1=path;
	while(*pt1)
	{
		// normalize
		if(*pt1 == '/') *pt1 = dirsep;
		pt1++;
	}
	char* pt=path+strlen(path)-1;
	if (pt>=path && *pt==dirsep) pt--;
	while (pt>=path && *pt!=dirsep) pt--;
	if (pt>=path) *(pt+1)=0;
}

wxString ConfigManager::getExecutableBasePath()
{
	char tmp[1025] = "";
	strncpy(tmp, getExecutablePath().mb_str(), 1024);
	path_descend(tmp);
	return wxString(tmp, wxConvUTF8);
}

wxString ConfigManager::getExecutablePath()
{
    static bool found = false;
    static wxString path;

    if (found)
        return path;
    else
    {
#ifdef __WXMSW__

        TCHAR buf[512];
        *buf = '\0';
        GetModuleFileName(NULL, buf, 511);
        path = buf;

#elif defined(__WXMAC__)

        ProcessInfoRec processinfo;
        ProcessSerialNumber procno ;
        FSSpec fsSpec;

        procno.highLongOfPSN = NULL ;
        procno.lowLongOfPSN = kCurrentProcess ;
        processinfo.processInfoLength = sizeof(ProcessInfoRec);
        processinfo.processName = NULL;
        processinfo.processAppSpec = &fsSpec;

        GetProcessInformation( &procno , &processinfo ) ;
        path = wxMacFSSpec2MacFilename(&fsSpec);
#else
        wxString argv0 = wxTheApp->argv[0];

        if (wxIsAbsolutePath(argv0))
            path = argv0;
        else
        {
            wxPathList pathlist;
            pathlist.AddEnvList(wxT("PATH"));
            path = pathlist.FindAbsoluteValidPath(argv0);
        }

        wxFileName filename(path);
        filename.Normalize();
        path = filename.GetFullPath();
#endif
        found = true;
        return path;
    }
}

