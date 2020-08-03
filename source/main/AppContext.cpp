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
#include "GUIManager.h"
#include "GUI_LoadingWindow.h"
#include "GUI_MainSelector.h"
#include "GUI_MultiplayerClientList.h"
#include "GUI_MultiplayerSelector.h"
#include "InputEngine.h"
#include "Language.h"
#include "RoRFrameListener.h" // SimController

#include "OgreSubsystem.h"
#include "OverlayWrapper.h"

#ifdef USE_ANGELSCRIPT
#    include "ScriptEngine.h"
#endif

using namespace RoR;

AppContext::AppContext()
{
    App::GetInputEngine()->SetMouseListener(this);
    App::GetInputEngine()->SetKeyboardListener(this);
    App::GetInputEngine()->SetJoystickListener(this);
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
