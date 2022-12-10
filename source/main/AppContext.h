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

#include <Bites/OgreApplicationContext.h>
#include <Bites/OgreWindowEventUtilities.h>
#include <Bites/OgreInput.h>
#include <Ogre.h>


namespace RoR {

/// @addtogroup Application
/// @{

/// Central setup and event handler for input/windowing/rendering.
/// Inspired by OgreBites::ApplicationContext.
class AppContext: public OgreBites::WindowEventListener,
                  public OgreBites::InputListener,
                  public OgreBites::ApplicationContext
{
public:
    // Startup (in order)
    void                 SetUpThreads();
    bool                 SetUpProgramPaths();
    void                 SetUpLogging();
    bool                 SetUpResourcesDir();
    bool                 SetUpRendering();
    bool                 SetUpConfigSkeleton();
    bool                 SetUpInput();
    void                 SetUpObsoleteConfMarker();

    // Rendering
    Ogre::RenderWindow*  CreateCustomRenderWindow(std::string const& name, int width, int height);
    void                 CaptureScreenshot();
    void                 ActivateFullscreen(bool val);

    // Getters
    Ogre::Root*          GetOgreRoot() { return m_ogre_root; }
    Ogre::Viewport*      GetViewport() { return m_viewport; }
    Ogre::RenderWindow*  GetRenderWindow() { return m_render_window; }
    RoR::ForceFeedback&  GetForceFeedback() { return m_force_feedback; }
    std::thread::id      GetMainThreadID() { return m_mainthread_id; }

private:
    // OgreBites::WindowEventListener
    virtual void         windowResized(Ogre::RenderWindow* rw) override;
    virtual void         windowFocusChange(Ogre::RenderWindow* rw) override;

    // OgreBites::InputListener
    virtual bool         mouseMoved(const OgreBites::MouseMotionEvent& arg) override;
    virtual bool         mouseWheelRolled(const OgreBites::MouseWheelEvent& evt) override;
    virtual bool         mousePressed(const OgreBites::MouseButtonEvent& arg) override;
    virtual bool         mouseReleased(const OgreBites::MouseButtonEvent& arg) override;
    virtual bool         keyPressed(const OgreBites::KeyboardEvent& arg) override;
    virtual bool         keyReleased(const OgreBites::KeyboardEvent& arg) override;
    virtual bool         buttonPressed(const OgreBites::ButtonEvent& arg) override;
    virtual bool         buttonReleased(const OgreBites::ButtonEvent& arg) override;
    virtual bool         axisMoved(const OgreBites::AxisEvent& arg) override;

    // Rendering and window management
    void                 SetRenderWindowIcon(Ogre::RenderWindow* rw);

    // Variables

    Ogre::Root*          m_ogre_root     = nullptr;
    Ogre::RenderWindow*  m_render_window = nullptr;
    Ogre::Viewport*      m_viewport      = nullptr;
    bool                 m_windowed_fix = false; //!< Workaround OGRE glitch when switching from fullscreen.

    std::time_t          m_prev_screenshot_time;
    int                  m_prev_screenshot_index = 1;

    RoR::ForceFeedback   m_force_feedback;

    std::thread::id      m_mainthread_id;
};

/// @} // addtogroup Application

} // namespace RoR
