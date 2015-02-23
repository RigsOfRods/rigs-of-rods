/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

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
#ifdef USE_MYGUI
#ifndef __GUI_InputManager_H_
#define __GUI_InputManager_H_

#include "RoRPrerequisites.h"
#include "RigEditor_ForwardDeclarations.h"

#include <OIS.h>
#include <OgreTimer.h>

class GUIInputManager
{
	friend class InputEngine;
	friend class RoR::RigEditor::InputHandler;

public:

    GUIInputManager();
    virtual ~GUIInputManager();

    void setInputViewSize(int _width, int _height);

    void setMousePosition(int _x, int _y);

	float getLastMouseMoveTime() { return lastMouseMoveTime->getMilliseconds(); };

protected:

    virtual bool mouseMoved(const OIS::MouseEvent& _arg);
    virtual bool mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
    virtual bool mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
    virtual bool keyPressed(const OIS::KeyEvent& _arg);
    virtual bool keyReleased(const OIS::KeyEvent& _arg);

    void checkPosition();

private:

	Ogre::Timer *lastMouseMoveTime;
    int mCursorX, mCursorY, width, height;
	
	void activateGUI();
};

#endif // __GUI_InputManager_H_
#endif // USE_MYGUI
