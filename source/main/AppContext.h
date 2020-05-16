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
/// @brief  Central handler for input/windowing/rendering/gameplay events.

#pragma once

#include "Application.h"

#include <Bites/OgreWindowEventUtilities.h>
#include <Ogre.h>
#include <OIS.h>

namespace RoR {

/// Central handler for input/windowing/rendering/gameplay events.
class AppContext: public OgreBites::WindowEventListener,
                  public OIS::MouseListener,
                  public OIS::KeyListener,
                  public OIS::JoyStickListener
{
public:
    AppContext();

private:
    // OgreBites::WindowEventListener
    virtual void windowResized(Ogre::RenderWindow* rw) override;
    virtual void windowFocusChange(Ogre::RenderWindow* rw) override;

    // OIS::MouseListener
    virtual bool mouseMoved(const OIS::MouseEvent& arg) override;
    virtual bool mousePressed(const OIS::MouseEvent& arg, OIS::MouseButtonID id) override;
    virtual bool mouseReleased(const OIS::MouseEvent& arg, OIS::MouseButtonID id) override;

    // OIS::KeyListener
    virtual bool keyPressed(const OIS::KeyEvent& arg) override;
    virtual bool keyReleased(const OIS::KeyEvent& arg) override;

    // OIS::JoyStickListener
    virtual bool buttonPressed(const OIS::JoyStickEvent& arg, int button) override;
    virtual bool buttonReleased(const OIS::JoyStickEvent& arg, int button) override;
    virtual bool axisMoved(const OIS::JoyStickEvent& arg, int axis) override;
    virtual bool sliderMoved(const OIS::JoyStickEvent& arg, int) override;
    virtual bool povMoved(const OIS::JoyStickEvent& arg, int) override;
    
};

} // namespace RoR
