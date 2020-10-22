/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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

#include "AppContext.h"

#include "AdvancedScreen.h"
#include "Actor.h"
#include "CameraManager.h"
#include "ChatSystem.h"
#include "Console.h"
#include "ContentManager.h"
#include "DashBoardManager.h"
#include "ErrorUtils.h"
#include "GameContext.h"
#include "GUIManager.h"
#include "GUI_LoadingWindow.h"
#include "GUI_MainSelector.h"
#include "GUI_MultiplayerClientList.h"
#include "GUI_MultiplayerSelector.h"
#include "InputEngine.h"
#include "Language.h"
#include "PlatformUtils.h"
#include "RoRVersion.h"
#include "OverlayWrapper.h"

#ifdef USE_ANGELSCRIPT
#    include "ScriptEngine.h"
#endif

#ifdef _WIN32
#   include <windows.h>
#endif

#include <iomanip>
#include <sstream>
#include <string>
#include <ctime>

using namespace RoR;

// --------------------------
// Input handling

bool AppContext::SetUpInput()
{
    App::CreateInputEngine();
    App::GetInputEngine()->SetMouseListener(this);
    App::GetInputEngine()->SetKeyboardListener(this);
    App::GetInputEngine()->SetJoystickListener(this);

    if (App::io_ffb_enabled->GetBool())
    {
        m_force_feedback.Setup();
    }
    return true;
}

bool AppContext::mouseMoved(const OIS::MouseEvent& arg) // overrides OIS::MouseListener
{
    App::GetGuiManager()->WakeUpGUI();
    App::GetGuiManager()->GetImGui().InjectMouseMoved(arg);

    if (!ImGui::GetIO().WantCaptureMouse) // true if mouse is over any window
    {
        bool handled = false;
        if (App::GetOverlayWrapper())
        {
            handled = App::GetOverlayWrapper()->mouseMoved(arg); // update the old airplane / autopilot gui
        }
        if (!handled)
        {
            if (!App::GetCameraManager()->mouseMoved(arg))
            {
                App::GetGameContext()->GetSceneMouse().mouseMoved(arg);
            }
        }
    }

    return true;
}

bool AppContext::mousePressed(const OIS::MouseEvent& arg, OIS::MouseButtonID _id) // overrides OIS::MouseListener
{
    App::GetGuiManager()->WakeUpGUI();
    App::GetGuiManager()->GetImGui().InjectMousePressed(arg, _id);

    if (!ImGui::GetIO().WantCaptureMouse) // true if mouse is over any window
    {
        bool handled = false;
        if (App::GetOverlayWrapper())
        {
            handled = App::GetOverlayWrapper()->mousePressed(arg, _id); // update the old airplane / autopilot gui
        }

        if (!handled && App::app_state->GetEnum<AppState>() == AppState::SIMULATION)
        {
            App::GetGameContext()->GetSceneMouse().mousePressed(arg, _id);
            App::GetCameraManager()->mousePressed(arg, _id);
        }
    }
    else
    {
        App::GetInputEngine()->ProcessMouseEvent(arg);
    }

    return true;
}

bool AppContext::mouseReleased(const OIS::MouseEvent& arg, OIS::MouseButtonID _id) // overrides OIS::MouseListener
{
    App::GetGuiManager()->WakeUpGUI();
    App::GetGuiManager()->GetImGui().InjectMouseReleased(arg, _id);

    if (!ImGui::GetIO().WantCaptureMouse) // true if mouse is over any window
    {
        bool handled = false;
        if (App::GetOverlayWrapper())
        {
            handled = App::GetOverlayWrapper()->mouseReleased(arg, _id); // update the old airplane / autopilot gui
        }
        if (!handled && App::app_state->GetEnum<AppState>() == AppState::SIMULATION)
        {
            App::GetGameContext()->GetSceneMouse().mouseReleased(arg, _id);
        }
    }
    else
    {
        App::GetInputEngine()->ProcessMouseEvent(arg);
    }

    return true;
}

bool AppContext::keyPressed(const OIS::KeyEvent& arg)
{
    App::GetGuiManager()->GetImGui().InjectKeyPressed(arg);

    if (!App::GetGuiManager()->IsGuiCaptureKeyboardRequested() &&
        !ImGui::GetIO().WantCaptureKeyboard)
    {
        App::GetInputEngine()->ProcessKeyPress(arg);
    }

    return true;
}

bool AppContext::keyReleased(const OIS::KeyEvent& arg)
{
    App::GetGuiManager()->GetImGui().InjectKeyReleased(arg);

    if (!App::GetGuiManager()->IsGuiCaptureKeyboardRequested() &&
        !ImGui::GetIO().WantCaptureKeyboard)
    {
        App::GetInputEngine()->ProcessKeyRelease(arg);
    }
    else if (App::GetInputEngine()->isKeyDownEffective(arg.key))
    {
        // If capturing is requested, still pass release events for already-pressed keys.
        App::GetInputEngine()->ProcessKeyRelease(arg);
    }

    return true;
}

bool AppContext::buttonPressed(const OIS::JoyStickEvent& arg, int)  { App::GetInputEngine()->ProcessJoystickEvent(arg); return true; }
bool AppContext::buttonReleased(const OIS::JoyStickEvent& arg, int) { App::GetInputEngine()->ProcessJoystickEvent(arg); return true; }
bool AppContext::axisMoved(const OIS::JoyStickEvent& arg, int)      { App::GetInputEngine()->ProcessJoystickEvent(arg); return true; }
bool AppContext::sliderMoved(const OIS::JoyStickEvent& arg, int)    { App::GetInputEngine()->ProcessJoystickEvent(arg); return true; }
bool AppContext::povMoved(const OIS::JoyStickEvent& arg, int)       { App::GetInputEngine()->ProcessJoystickEvent(arg); return true; }

void AppContext::windowResized(Ogre::RenderWindow* rw)
{
    App::GetInputEngine()->windowResized(rw); // Update mouse area
    App::GetOverlayWrapper()->windowResized();
    if (App::sim_state->GetEnum<AppState>() == RoR::AppState::SIMULATION)
    {
        for (Actor* actor: App::GetGameContext()->GetActorManager()->GetActors())
        {
            actor->ar_dashboard->windowResized();
        }
    }
}

void AppContext::windowFocusChange(Ogre::RenderWindow* rw)
{
    App::GetInputEngine()->resetKeys();
}

// --------------------------
// Rendering

void AppContext::SetRenderWindowIcon(Ogre::RenderWindow* rw)
{
#ifdef _WIN32
    size_t hWnd = 0;
    rw->getCustomAttribute("WINDOW", &hWnd);

    char buf[MAX_PATH];
    ::GetModuleFileNameA(0, (LPCH)&buf, MAX_PATH);

    HINSTANCE instance = ::GetModuleHandleA(buf);
    HICON hIcon = ::LoadIconA(instance, MAKEINTRESOURCEA(101));
    if (hIcon)
    {
        ::SendMessageA((HWND)hWnd, WM_SETICON, 1, (LPARAM)hIcon);
        ::SendMessageA((HWND)hWnd, WM_SETICON, 0, (LPARAM)hIcon);
    }
#endif // _WIN32
}

bool AppContext::SetUpRendering()
{
    // Create 'OGRE root' facade
    std::string log_filepath = PathCombine(App::sys_logs_dir->GetStr(), "RoR.log");
    std::string cfg_filepath = PathCombine(App::sys_config_dir->GetStr(), "ogre.cfg");
    m_ogre_root = new Ogre::Root("", cfg_filepath, log_filepath);

    // load OGRE plugins manually
	std::string plugins_path = PathCombine(RoR::App::sys_process_dir->GetStr(), "plugins.cfg");
    try
    {
        Ogre::ConfigFile cfg;
        cfg.load(plugins_path);
        std::string plugin_dir = cfg.getSetting("PluginFolder", /*section=*/"", /*default=*/App::sys_process_dir->GetStr());
        Ogre::StringVector plugins = cfg.getMultiSetting("Plugin");
        for (Ogre::String plugin_filename: plugins)
        {
            try
            {
                m_ogre_root->loadPlugin(PathCombine(plugin_dir, plugin_filename));
            }
            catch (Ogre::Exception&) {} // Logged by OGRE
        }
    }
    catch (Ogre::Exception& e)
    {
        ErrorUtils::ShowError (_L("Startup error"), e.getFullDescription());
        return false;
    }

    // Load renderer configuration
    if (!m_ogre_root->restoreConfig())
    {
        const auto render_systems = App::GetAppContext()->GetOgreRoot()->getAvailableRenderers();
        if (!render_systems.empty())
            m_ogre_root->setRenderSystem(render_systems.front());
        else
            ErrorUtils::ShowError (_L("Startup error"), _L("No render system plugin available. Check your plugins.cfg"));
    }

    const auto rs = m_ogre_root->getRenderSystemByName(App::app_rendersys_override->GetStr());
    if (rs != nullptr && rs != m_ogre_root->getRenderSystem())
    {
        // The user has selected a different render system during the previous session.
        m_ogre_root->setRenderSystem(rs);
        m_ogre_root->saveConfig();
    }
    App::app_rendersys_override->SetStr("");

    // Start the renderer
    m_ogre_root->initialise(/*createWindow=*/false);

    // Configure the render window
    Ogre::ConfigOptionMap ropts = m_ogre_root->getRenderSystem()->getConfigOptions();
    Ogre::NameValuePairList miscParams;
    miscParams["FSAA"] = ropts["FSAA"].currentValue;
    miscParams["vsync"] = ropts["VSync"].currentValue;
    miscParams["gamma"] = ropts["sRGB Gamma Conversion"].currentValue;
    miscParams["border"] = "fixed";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    const auto rd = ropts["Rendering Device"];
    const auto it = std::find(rd.possibleValues.begin(), rd.possibleValues.end(), rd.currentValue);
    const int idx = std::distance(rd.possibleValues.begin(), it);
    miscParams["monitorIndex"] = Ogre::StringConverter::toString(idx);
    miscParams["windowProc"] = Ogre::StringConverter::toString((size_t)OgreBites::WindowEventUtilities::_WndProc);
#endif

    // Validate rendering resolution
    Ogre::uint32 width, height;
    std::istringstream mode (ropts["Video Mode"].currentValue);
    Ogre::String token;
    mode >> width;
    mode >> token; // 'x' as seperator between width and height
    mode >> height;
    
    if(width < 800) width = 800;
    if(height < 600) height = 600;

    // Create render window
    m_render_window = Ogre::Root::getSingleton().createRenderWindow (
        "Rigs of Rods version " + Ogre::String (ROR_VERSION_STRING),
        width, height, ropts["Full Screen"].currentValue == "Yes", &miscParams);
    OgreBites::WindowEventUtilities::_addRenderWindow(m_render_window);

    this->SetRenderWindowIcon(m_render_window);
    m_render_window->setActive(true);

    // Create viewport (without camera)
    m_viewport = m_render_window->addViewport(/*camera=*/nullptr);
    m_viewport->setBackgroundColour(Ogre::ColourValue::Black);

    return true;
}

Ogre::RenderWindow* AppContext::CreateCustomRenderWindow(std::string const& window_name, int width, int height)
{
    Ogre::NameValuePairList misc;
    Ogre::ConfigOptionMap ropts = m_ogre_root->getRenderSystem()->getConfigOptions();
    misc["FSAA"] = Ogre::StringConverter::parseInt(ropts["FSAA"].currentValue, 0);

    Ogre::RenderWindow* rw = Ogre::Root::getSingleton().createRenderWindow(window_name, width, height, false, &misc);
    return rw;
}

void AppContext::CaptureScreenshot()
{
    const std::time_t time = std::time(nullptr);
    const int index = (time == m_prev_screenshot_time) ? m_prev_screenshot_index+1 : 1;

    std::stringstream stamp;
    stamp << std::put_time(std::localtime(&time), "%Y-%m-%d_%H-%M-%S") << "_" << index
          << "." << App::app_screenshot_format->GetStr();
    std::string path = PathCombine(App::sys_screenshot_dir->GetStr(), "screenshot_") + stamp.str();

    if (App::app_screenshot_format->GetStr() == "png")
    {
        AdvancedScreen png(m_render_window, path);

        png.addData("User_NickName", App::mp_player_name->GetStr());
        png.addData("User_Language", App::app_language->GetStr());

        if (App::app_state->GetEnum<AppState>() == AppState::SIMULATION && 
            App::GetGameContext()->GetPlayerActor())
        {
            png.addData("Truck_file", App::GetGameContext()->GetPlayerActor()->ar_filename);
            png.addData("Truck_name", App::GetGameContext()->GetPlayerActor()->GetActorDesignName());
        }
        if (App::GetSimTerrain())
        {
            png.addData("Terrn_file", App::sim_terrain_name->GetStr());
            png.addData("Terrn_name", App::sim_terrain_gui_name->GetStr());
        }
        if (App::mp_state->GetEnum<MpState>() == MpState::CONNECTED)
        {
            png.addData("MP_ServerName", App::mp_server_host->GetStr());
        }

        png.write();
    }
    else
    {
        m_render_window->writeContentsToFile(path);
    }

    App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                                  _L("Screenshot:") + stamp.str());

    m_prev_screenshot_time = time;
    m_prev_screenshot_index = index;
}

void AppContext::ActivateFullscreen(bool val)
{
    if (!val && !m_windowed_fix)
    {
        // Workaround OGRE glitch - badly aligned viewport after first full->window switch
        // Observed on Win10/OpenGL (GeForce GTX 660)
        m_render_window->setFullscreen(false, m_render_window->getWidth()-1, m_render_window->getHeight()-1);    
        m_render_window->setFullscreen(false, m_render_window->getWidth()+1, m_render_window->getHeight()+1);    
    }
    else
    {
        m_render_window->setFullscreen(val, m_render_window->getWidth(), m_render_window->getHeight());
    }
}

// --------------------------
// Program paths and logging

bool AppContext::SetUpProgramPaths()
{
    // Process directory
    std::string exe_path = RoR::GetExecutablePath();
    if (exe_path.empty())
    {
        ErrorUtils::ShowError(_L("Startup error"), _L("Error while retrieving program directory path"));
        return false;
    }
    App::sys_process_dir->SetStr(RoR::GetParentDirectory(exe_path.c_str()).c_str());

    // RoR's home directory
    std::string local_userdir = PathCombine(App::sys_process_dir->GetStr(), "config"); // TODO: Think of a better name, this is ambiguious with ~/.rigsofrods/config which stores configfiles! ~ only_a_ptr, 02/2018
    if (FolderExists(local_userdir))
    {
        // It's a portable installation
        App::sys_user_dir->SetStr(local_userdir.c_str());
    }
    else
    {
        // Default location - user's home directory
        std::string user_home = RoR::GetUserHomeDirectory();
        if (user_home.empty())
        {
            ErrorUtils::ShowError(_L("Startup error"), _L("Error while retrieving user directory path"));
            return false;
        }
        RoR::Str<500> ror_homedir;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        ror_homedir << user_home << PATH_SLASH << "My Games";
        CreateFolder(ror_homedir.ToCStr());
        ror_homedir << PATH_SLASH << "Rigs of Rods";
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
        char* env_SNAP = getenv("SNAP_USER_COMMON");
        if(env_SNAP)
            ror_homedir = env_SNAP;
        else
            ror_homedir << user_home << PATH_SLASH << ".rigsofrods";
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
        ror_homedir << user_home << PATH_SLASH << "RigsOfRods";
#endif
        CreateFolder(ror_homedir.ToCStr ());
        App::sys_user_dir->SetStr(ror_homedir.ToCStr ());
    }

    return true;
}

void AppContext::SetUpLogging()
{
    std::string logs_dir = PathCombine(App::sys_user_dir->GetStr(), "logs");
    CreateFolder(logs_dir);
    App::sys_logs_dir->SetStr(logs_dir.c_str());

    auto ogre_log_manager = OGRE_NEW Ogre::LogManager();
    std::string rorlog_path = PathCombine(logs_dir, "RoR.log");
    Ogre::Log* rorlog = ogre_log_manager->createLog(rorlog_path, true, true);
    rorlog->stream() << "[RoR] Rigs of Rods (www.rigsofrods.org) version " << ROR_VERSION_STRING;
    std::time_t t = std::time(nullptr);
    rorlog->stream() << "[RoR] Current date: " << std::put_time(std::localtime(&t), "%Y-%m-%d");

    rorlog->addListener(App::GetConsole());  // Allow console to intercept log messages
}

bool AppContext::SetUpResourcesDir()
{
    std::string process_dir = PathCombine(App::sys_process_dir->GetStr(), "resources");
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
    if (!FolderExists(process_dir))
    {
        process_dir = "/usr/share/rigsofrods/resources/";
    }
#endif
    if (!FolderExists(process_dir))
    {
        ErrorUtils::ShowError(_L("Startup error"), _L("Resources folder not found. Check if correctly installed."));
        return false;
    }
    App::sys_resources_dir->SetStr(process_dir);
    return true;
}

bool AppContext::SetUpConfigSkeleton()
{
    Ogre::String src_path = PathCombine(App::sys_resources_dir->GetStr(), "skeleton.zip");
    Ogre::ResourceGroupManager::getSingleton().addResourceLocation(src_path, "Zip", "SrcRG");
    Ogre::FileInfoListPtr fl = Ogre::ResourceGroupManager::getSingleton().findResourceFileInfo("SrcRG", "*", true);
    if (fl->empty())
    {
        ErrorUtils::ShowError(_L("Startup error"), _L("Faulty resource folder. Check if correctly installed."));
        return false;
    }
    Ogre::String dst_path = PathCombine(App::sys_user_dir->GetStr(), "");
    for (auto file : *fl)
    {
        CreateFolder(dst_path + file.basename);
    }
    fl = Ogre::ResourceGroupManager::getSingleton().findResourceFileInfo("SrcRG", "*");
    if (fl->empty())
    {
        ErrorUtils::ShowError(_L("Startup error"), _L("Faulty resource folder. Check if correctly installed."));
        return false;
    }
    Ogre::ResourceGroupManager::getSingleton().addResourceLocation(dst_path, "FileSystem", "DstRG", false, false);
    for (auto file : *fl)
    {
        if (file.uncompressedSize == 0)
            continue;
        Ogre::String path = file.path + file.basename;
        if (!Ogre::ResourceGroupManager::getSingleton().findResourceFileInfo("DstRG", path)->empty())
            continue;
        Ogre::DataStreamPtr src_ds = Ogre::ResourceGroupManager::getSingleton().openResource(path, "SrcRG");
        Ogre::DataStreamPtr dst_ds = Ogre::ResourceGroupManager::getSingleton().createResource(path, "DstRG");
        std::vector<char> buf(src_ds->size());
        size_t read = src_ds->read(buf.data(), src_ds->size());
        if (read > 0)
        {
            dst_ds->write(buf.data(), read);
        }
    }
    Ogre::ResourceGroupManager::getSingleton().destroyResourceGroup("SrcRG");
    Ogre::ResourceGroupManager::getSingleton().destroyResourceGroup("DstRG");

    return true;
}

void AppContext::SetUpObsoleteConfMarker()
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    Ogre::String old_ror_homedir = Ogre::StringUtil::format("%s\\Rigs of Rods 0.4", RoR::GetUserHomeDirectory().c_str());
    if(FolderExists(old_ror_homedir))
    {
        if (!FileExists(Ogre::StringUtil::format("%s\\OBSOLETE_FOLDER.txt", old_ror_homedir.c_str())))
        {
            Ogre::String obsoleteMessage = Ogre::StringUtil::format("This folder is obsolete, please move your mods to  %s", App::sys_user_dir->GetStr());
            try
            {
                Ogre::ResourceGroupManager::getSingleton().addResourceLocation(old_ror_homedir, "FileSystem", "homedir", false, false);
                Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().createResource("OBSOLETE_FOLDER.txt", "homedir");
                stream->write(obsoleteMessage.c_str(), obsoleteMessage.length());
                Ogre::ResourceGroupManager::getSingleton().destroyResourceGroup("homedir");
            }
            catch (std::exception & e)
            {
                RoR::LogFormat("Error writing to %s, message: '%s'", old_ror_homedir.c_str(), e.what());
            }
            Ogre::String message = Ogre::StringUtil::format(
                "Welcome to Rigs of Rods %s\nPlease note that the mods folder has moved to:\n\"%s\"\nPlease move your mods to the new folder to continue using them",
                ROR_VERSION_STRING_SHORT,
                App::sys_user_dir->GetStr()
            );

            RoR::App::GetGuiManager()->ShowMessageBox("Obsolete folder detected", message.c_str());
        }
    }
#endif // OGRE_PLATFORM_WIN32
}
