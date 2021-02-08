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

/// @file
/// @author Petr Ohlidal
/// @date   05/2020
/// @brief  System integration layer; inspired by OgreBites::ApplicationContext.

#pragma once

#include "Application.h"
#include "ForceFeedback.h"

#include <OgreWindowEventUtilities.h>
#include <Ogre.h>
#include <OIS.h>

namespace RoR {

/// Central setup and event handler for input/windowing/rendering.
/// Inspired by OgreBites::ApplicationContext (OGRE 1.x).
/// Rougly matches Demo::GraphicsSystem and Demo::SdlInputHandler (OGRE 2.x).
class AppContext: public Ogre::WindowEventListener,
                  public OIS::MouseListener,
                  public OIS::KeyListener,
                  public OIS::JoyStickListener
{
public:
    // Startup (in order)
    bool                 SetUpProgramPaths();
    void                 SetUpLogging();
    bool                 SetUpResourcesDir();
    bool                 SetUpRendering();
    bool                 SetUpConfigSkeleton();
    bool                 SetUpInput();

    // Rendering
    Ogre::Window*        CreateCustomRenderWindow(std::string const& name, int width, int height);
    void                 CaptureScreenshot();
    void                 ActivateFullscreen(bool val);

    // Getters
    Ogre::Root*          GetOgreRoot() { return m_ogre_root; }
    Ogre::Window*        GetRenderWindow() { return m_render_window; }
    RoR::ForceFeedback&  GetForceFeedback() { return m_force_feedback; }

private:
    // OgreBites::WindowEventListener
    virtual void         windowResized(Ogre::Window* rw) override;
    virtual void         windowFocusChange(Ogre::Window* rw) override;

    // OIS::MouseListener
    virtual bool         mouseMoved(const OIS::MouseEvent& arg) override;
    virtual bool         mousePressed(const OIS::MouseEvent& arg, OIS::MouseButtonID id) override;
    virtual bool         mouseReleased(const OIS::MouseEvent& arg, OIS::MouseButtonID id) override;

    // OIS::KeyListener
    virtual bool         keyPressed(const OIS::KeyEvent& arg) override;
    virtual bool         keyReleased(const OIS::KeyEvent& arg) override;

    // OIS::JoyStickListener
    virtual bool         buttonPressed(const OIS::JoyStickEvent& arg, int button) override;
    virtual bool         buttonReleased(const OIS::JoyStickEvent& arg, int button) override;
    virtual bool         axisMoved(const OIS::JoyStickEvent& arg, int axis) override;
    virtual bool         sliderMoved(const OIS::JoyStickEvent& arg, int) override;
    virtual bool         povMoved(const OIS::JoyStickEvent& arg, int) override;

    // Rendering and window management
    void                 SetRenderWindowIcon(Ogre::Window* rw);

    // Variables

    Ogre::Root*          m_ogre_root     = nullptr;
    Ogre::Window*        m_render_window = nullptr;

    std::time_t          m_prev_screenshot_time;
    int                  m_prev_screenshot_index = 1;

    RoR::ForceFeedback   m_force_feedback;
};

} // namespace RoR
