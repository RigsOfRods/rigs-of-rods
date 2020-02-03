/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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

#include "GUIInputManager.h"

#include "Application.h"
#include "GUIManager.h"
#include "GUI_TopMenubar.h"
#include "InputEngine.h"
#include "OverlayWrapper.h"
#include "RoRFrameListener.h" // SimController
#include "SceneMouse.h"

GUIInputManager::GUIInputManager() :
    height(0)
    , m_last_mousemove_time(0)
    , mCursorX(0)
    , mCursorY(0)
    , width(0)
    , m_is_cursor_supressed(false)
{
    m_last_mousemove_time = new Ogre::Timer();
}

GUIInputManager::~GUIInputManager()
{
}

bool GUIInputManager::mouseMoved(const OIS::MouseEvent& _arg)
{
    this->WakeUpGUI();

    RoR::App::GetGuiManager()->GetImGui().InjectMouseMoved(_arg);

    if (RoR::App::sim_state.GetActive() == RoR::SimState::RUNNING && RoR::App::GetGuiManager()->IsVisible_TopMenubar()) // dirty hack to block imgui handled input events
    {
        return true;
    }

    if (RoR::App::sim_state.GetActive() == RoR::SimState::PAUSED)
    {
        MyGUI::InputManager::getInstance().injectMouseMove(mCursorX, mCursorY, _arg.state.Z.abs);
        mCursorX = _arg.state.X.abs;
        mCursorY = _arg.state.Y.abs;
        checkPosition();
        return true;
    }

    // fallback, handle by GUI, then by RoR::SceneMouse
    bool handled = MyGUI::InputManager::getInstance().injectMouseMove(mCursorX, mCursorY, _arg.state.Z.abs);

    if (handled)
    {
        MyGUI::Widget* w = MyGUI::InputManager::getInstance().getMouseFocusWidget();
        // hack for console, we want to use the mouse through that control
        if (w && w->getName().substr(0, 7) == "Console")
            handled = false;
        if (w && w->getUserString("interactive") == "0")
            handled = false;
    }

    if (!handled && RoR::App::GetOverlayWrapper() != nullptr)
    {
        // update the old airplane / autopilot gui
        handled = RoR::App::GetOverlayWrapper()->mouseMoved(_arg);
    }

    if (!handled)
    {
        if (RoR::App::GetSimController() != nullptr) // TODO: Fix this hack. Main menu should not use the same input handler as simulation ~ only_a_ptr, 08/2018
        {
            // not handled by gui
            bool fixed = RoR::App::GetSimController()->GetSceneMouse().mouseMoved(_arg);
            if (fixed)
            {
                // you would really need to "fix" the actual mouse position, see
                // http://www.wreckedgames.com/forum/index.php?topic=1104.0
                return true;
            }
        }
    }

    mCursorX = _arg.state.X.abs;
    mCursorY = _arg.state.Y.abs;

    checkPosition();
    return true;
}

bool GUIInputManager::mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
    this->WakeUpGUI();

    RoR::App::GetGuiManager()->GetImGui().InjectMousePressed(_arg, _id);

    mCursorX = _arg.state.X.abs;
    mCursorY = _arg.state.Y.abs;

    // fallback, handle by GUI, then by RoR::SceneMouse
    bool handled = MyGUI::InputManager::getInstance().injectMousePress(mCursorX, mCursorY, MyGUI::MouseButton::Enum(_id));

    if (handled)
    {
        MyGUI::Widget* w = MyGUI::InputManager::getInstance().getMouseFocusWidget();
        // hack for console, we want to use the mouse through that control
        if (w && w->getName().substr(0, 7) == "Console")
            handled = false;
        if (w && w->getUserString("interactive") == "0")
            handled = false;
    }

    if (!handled && RoR::App::GetOverlayWrapper())
    {
        // update the old airplane / autopilot gui
        handled = RoR::App::GetOverlayWrapper()->mousePressed(_arg, _id);
    }

    // TODO: Fix this hack. Main menu should not use the same input handler as simulation ~ only_a_ptr, 08/2018
    if (!handled && (RoR::App::GetSimController() != nullptr)) 
    {
        RoR::App::GetSimController()->GetSceneMouse().mousePressed(_arg, _id);
    }
    return handled;
}

bool GUIInputManager::mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
    this->WakeUpGUI();

    RoR::App::GetGuiManager()->GetImGui().InjectMouseReleased(_arg, _id);

    // fallback, handle by GUI, then by RoR::SceneMouse
    bool handled = MyGUI::InputManager::getInstance().injectMouseRelease(mCursorX, mCursorY, MyGUI::MouseButton::Enum(_id));

    if (handled)
    {
        MyGUI::Widget* w = MyGUI::InputManager::getInstance().getMouseFocusWidget();
        // hack for console, we want to use the mouse through that control
        if (w && w->getName().substr(0, 7) == "Console")
            handled = false;
        if (w && w->getUserString("interactive") == "0")
            handled = false;
    }

    if (!handled && RoR::App::GetOverlayWrapper())
    {
        // update the old airplane / autopilot gui
        handled = RoR::App::GetOverlayWrapper()->mouseReleased(_arg, _id);
    }

    if (!handled && (RoR::App::GetSimController() != nullptr))
    {
        RoR::App::GetSimController()->GetSceneMouse().mouseReleased(_arg, _id);
    }
    return handled;
}

bool GUIInputManager::keyPressed(const OIS::KeyEvent& _arg)
{
    RoR::App::GetGuiManager()->GetImGui().InjectKeyPressed(_arg);

    bool do_capture = RoR::App::GetGuiManager()->IsGuiCaptureKeyboardRequested() || ImGui::GetIO().WantCaptureKeyboard;
    if (!do_capture)
    {
        RoR::App::GetSimController()->GetSceneMouse().keyPressed(_arg);
    }

    return do_capture;
}

bool GUIInputManager::keyReleased(const OIS::KeyEvent& _arg)
{
    RoR::App::GetGuiManager()->GetImGui().InjectKeyReleased(_arg);

    bool do_capture = RoR::App::GetGuiManager()->IsGuiCaptureKeyboardRequested() || ImGui::GetIO().WantCaptureKeyboard;
    if (!do_capture)
    {
        RoR::App::GetSimController()->GetSceneMouse().keyReleased(_arg);
    }

    // If capturing is requested, still pass release events for already-pressed keys.
    return do_capture && !RoR::App::GetInputEngine()->isKeyDownEffective(_arg.key);
}

void GUIInputManager::setInputViewSize(int _width, int _height)
{
    this->width = _width;
    this->height = _height;

    checkPosition();
}

void GUIInputManager::setMousePosition(int _x, int _y)
{
    mCursorX = _x;
    mCursorY = _y;

    checkPosition();
}

void GUIInputManager::checkPosition()
{
    if (mCursorX < 0)
        mCursorX = 0;
    else if (mCursorX >= this->width)
        mCursorX = this->width - 1;

    if (mCursorY < 0)
        mCursorY = 0;
    else if (mCursorY >= this->height)
        mCursorY = this->height - 1;
}

void GUIInputManager::WakeUpGUI()
{
    m_last_mousemove_time->reset();
    if (!m_is_cursor_supressed)
    {
        RoR::App::GetGuiManager()->SetMouseCursorVisibility(RoR::GUIManager::MouseCursorVisibility::VISIBLE);
    }
}

void GUIInputManager::SupressCursor(bool do_supress)
{
    m_is_cursor_supressed = do_supress;
}
