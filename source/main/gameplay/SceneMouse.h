/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2014 Petr Ohlidal

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

/// @file   SceneMouse.h
/// @author Thomas Fischer <thomas@thomasfischer.biz>
/// @date   11th of March 2011
/// @brief  Mouse interaction with 3D scene.

#pragma once

#include "RoRPrerequisites.h"

namespace RoR {

class SceneMouse
{
public:

    SceneMouse();

    bool mouseMoved(const OIS::MouseEvent& _arg);
    bool mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
    bool mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
    bool keyPressed(const OIS::KeyEvent& _arg);
    bool keyReleased(const OIS::KeyEvent& _arg);

    void InitializeVisuals();
    void UpdateSimulation();
    void UpdateVisuals();
    void DiscardVisuals();

protected:

    Ogre::ManualObject* pickLine;
    Ogre::SceneNode* pickLineNode;
    int mouseGrabState;

    int minnode;
    float mindist;
    Actor* grab_truck;
    Ogre::Vector3 lastgrabpos;
    int lastMouseX, lastMouseY;

    void releaseMousePick();
    Ogre::Ray getMouseRay();
};

} // namespace RoR
