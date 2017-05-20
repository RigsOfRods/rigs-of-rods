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
    @file   PlatformUtils.cpp
    @author Petr Ohlidal
    @date   05/2014
*/

#include "PlatformUtils.h"

#ifdef USE_CRASHRPT
#   include "RoRVersion.h"
#   include "ErrorUtils.h"
#   include "Application.h"
#   include "Language.h"
#   include "rornet.h"
#endif

#include <Ogre.h>

namespace RoR {

// ================================================================================
// Static variables
// ================================================================================

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
const Ogre::String PlatformUtils::DIRECTORY_SEPARATOR("\\");
#else
    const Ogre::String PlatformUtils::DIRECTORY_SEPARATOR("/");
#endif

// ================================================================================
// Functions
// ================================================================================

bool PlatformUtils::FileExists(const char* path)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    DWORD attributes = GetFileAttributesA(path);
    return (attributes != INVALID_FILE_ATTRIBUTES && ! (attributes & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return (stat(path, &st) == 0);
#endif
}

bool PlatformUtils::FolderExists(const char* path)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    DWORD attributes = GetFileAttributesA(path);
    return (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return (stat(path, &st) == 0);
#endif
}

void PlatformUtils::CreateFolder(const char* path)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    CreateDirectory(path, NULL);
#else
    mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
}

bool PlatformUtils::FileExists(Ogre::String const& path)
{
    return FileExists(path.c_str());
}

bool PlatformUtils::FolderExists(Ogre::String const& path)
{
    return FolderExists(path.c_str());
}

void PlatformUtils::CreateFolder(Ogre::String const& path)
{
    CreateFolder(path.c_str());
}

// ================================================================================
// CrashRpt integration
// ================================================================================

#ifdef USE_CRASHRPT

// Define the crash callback
int CALLBACK CrashRptCallback(CR_CRASH_CALLBACK_INFO* pInfo)
{
    // Return CR_CB_DODEFAULT to generate error report
    return CR_CB_DODEFAULT;
}

void InstallCrashRpt()
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

    crSetCrashCallback(CrashRptCallback, NULL);

    int nInstResult = crInstall(&info);
    if (nInstResult != 0)
    {
        // Something goes wrong!
        TCHAR szErrorMsg[512];
        szErrorMsg[0] = 0;

        crGetLastErrorMsg(szErrorMsg, 512);
        printf("%s\n", szErrorMsg);


        ErrorUtils::ShowError(_L("Exception handling registration problem"), Ogre::String(szErrorMsg));

        assert(nInstResult == 0);
    }

    // logs
    crAddFile2((std::string(RoR::App::sys_logs_dir.GetActive()) + PATH_SLASH + "RoR.log").c_str(), "RoR.log", "Rigs of Rods Log", CR_AF_FILE_MUST_EXIST);
    crAddFile2((std::string(RoR::App::sys_logs_dir.GetActive()) + PATH_SLASH + "mygui.log").c_str(), "mygui.log", "Rigs of Rods GUI Log", CR_AF_FILE_MUST_EXIST);
    crAddFile2((std::string(RoR::App::sys_logs_dir.GetActive()) + PATH_SLASH + "configlog.txt").c_str(), "configlog.txt", "Rigs of Rods Configurator Log", CR_AF_FILE_MUST_EXIST);

    // cache
    crAddFile2((std::string(App::sys_cache_dir.GetActive()) + PATH_SLASH + "mods.cache").c_str(), "mods.cache", "Rigs of Rods Cache File", CR_AF_FILE_MUST_EXIST);

    // configs
    crAddFile2((std::string(RoR::App::sys_config_dir.GetActive()) + PATH_SLASH + "ground_models.cfg").c_str(), "ground_models.cfg", "Rigs of Rods Ground Configuration", CR_AF_FILE_MUST_EXIST);
    crAddFile2((std::string(RoR::App::sys_config_dir.GetActive()) + PATH_SLASH + "input.map").c_str(), "input.map", "Rigs of Rods Input Configuration", CR_AF_FILE_MUST_EXIST);
    crAddFile2((std::string(RoR::App::sys_config_dir.GetActive()) + PATH_SLASH + "ogre.cfg").c_str(), "ogre.cfg", "Rigs of Rods Renderer Configuration", CR_AF_FILE_MUST_EXIST);
    crAddFile2((std::string(RoR::App::sys_config_dir.GetActive()) + PATH_SLASH + "RoR.cfg").c_str(), "RoR.cfg", "Rigs of Rods Configuration", CR_AF_FILE_MUST_EXIST);

    crAddProperty("Version", ROR_VERSION_STRING);
    crAddProperty("protocol_version", RORNET_VERSION);
    crAddProperty("build_date", ROR_BUILD_DATE);
    crAddProperty("build_time", ROR_BUILD_TIME);

    crAddProperty("System_GUID", SSETTING("GUID", "None").c_str());
    crAddProperty("Multiplayer", RoR::App::mp_state.GetActive() ? "1" : "0");

    crAddScreenshot2(CR_AS_MAIN_WINDOW, 0);
}

void UninstallCrashRpt()
{
    crUninstall();
}

#endif // USE_CRASHRPT

} // namespace RoR
