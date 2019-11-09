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

#pragma once

#include "RoRPrerequisites.h"

#include <OIS.h>
#include <OgreTimer.h>

class GUIInputManager
{
    friend class InputEngine;

  public:
    GUIInputManager();
    virtual ~GUIInputManager();

    void setInputViewSize(int _width, int _height);

    void setMousePosition(int _x, int _y);

    float getLastMouseMoveTime()
    {
        return m_last_mousemove_time->getMilliseconds();
    };

    void SupressCursor(bool do_supress);

  protected:
    virtual bool mouseMoved(const OIS::MouseEvent &_arg);
    virtual bool mousePressed(const OIS::MouseEvent &_arg, OIS::MouseButtonID _id);
    virtual bool mouseReleased(const OIS::MouseEvent &_arg, OIS::MouseButtonID _id);
    virtual bool keyPressed(const OIS::KeyEvent &_arg);
    virtual bool keyReleased(const OIS::KeyEvent &_arg);

    void checkPosition();

  private:
    int          mCursorX, mCursorY, width, height;
    bool         m_is_cursor_supressed; ///< True if cursor was manually hidden.
    Ogre::Timer *m_last_mousemove_time;

    void WakeUpGUI();
};
