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

#include "ChatSystem.h"
#include "ContentManager.h"
#include "ErrorUtils.h"
#include "GUIManager.h"
#include "GUI_LoadingWindow.h"
#include "GUI_MainSelector.h"
#include "GUI_MultiplayerClientList.h"
#include "GUI_MultiplayerSelector.h"
#include "InputEngine.h"
#include "Language.h"
#include "PlatformUtils.h"
#include "RoRFrameListener.h" // SimController
#include "RoRVersion.h"
#include "OverlayWrapper.h"

#ifdef USE_ANGELSCRIPT
#    include "ScriptEngine.h"
#endif

#ifdef _WIN32
#   include <windows.h>
#endif

using namespace RoR;

// --------------------------
// Input handling

bool AppContext::SetUpInput()
{
    App::CreateInputEngine();
    App::GetInputEngine()->SetMouseListener(this);
    App::GetInputEngine()->SetKeyboardListener(this);
    App::GetInputEngine()->SetJoystickListener(this);
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

        if (!handled && App::GetSimController())
        {
            if (!App::GetCameraManager()->mouseMoved(arg))
            {
                App::GetSimController()->GetSceneMouse().mouseMoved(arg);
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

        if (!handled && App::GetSimController())
        {
            App::GetSimController()->GetSceneMouse().mousePressed(arg, _id);
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
            handled = App::GetOverlayWrapper()->mouseReleased(arg, _id); // update the old airplane / autopilot gui

        if (!handled && App::GetSimController())
        {
            App::GetSimController()->GetSceneMouse().mouseReleased(arg, _id);
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
    if (App::sim_state->GetActiveEnum<AppState>() == RoR::AppState::SIMULATION)
    {
        App::GetSimController()->GetBeamFactory()->NotifyActorsWindowResized();
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
    std::string log_filepath = PathCombine(App::sys_logs_dir->GetActiveStr(), "RoR.log");
    std::string cfg_filepath = PathCombine(App::sys_config_dir->GetActiveStr(), "ogre.cfg");
    m_ogre_root = new Ogre::Root("", cfg_filepath, log_filepath);

    // load OGRE plugins manually
#ifdef _DEBUG
    std::string plugins_path = PathCombine(RoR::App::sys_process_dir->GetActiveStr(), "plugins_d.cfg");
#else
	std::string plugins_path = PathCombine(RoR::App::sys_process_dir->GetActiveStr(), "plugins.cfg");
#endif
    try
    {
        Ogre::ConfigFile cfg;
        cfg.load(plugins_path);
        std::string plugin_dir = cfg.getSetting("PluginFolder", /*section=*/"", /*default=*/App::sys_process_dir->GetActiveStr());
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

    const auto rs = m_ogre_root->getRenderSystemByName(App::app_rendersys_override->GetActiveStr());
    if (rs != nullptr && rs != m_ogre_root->getRenderSystem())
    {
        // The user has selected a different render system during the previous session.
        m_ogre_root->setRenderSystem(rs);
        m_ogre_root->saveConfig();
    }
    App::app_rendersys_override->SetActiveStr("");

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
    this->SetRenderWindowIcon(rw);
    return rw;
}

