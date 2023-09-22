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

#include "Application.h"
#include "SimData.h"



#include <Ogre.h>

namespace RoR {

class SceneMouse
{
public:

    SceneMouse();

    bool mouseMoved(const OgreBites::MouseMotionEvent& _arg);
    bool mousePressed(const OgreBites::MouseButtonEvent& _arg);
    bool mouseReleased(const OgreBites::MouseButtonEvent& _arg);

    void InitializeVisuals();
    void UpdateSimulation();
    void UpdateVisuals();
    void DiscardVisuals();

protected:

    Ogre::ManualObject* pickLine;
    Ogre::SceneNode* pickLineNode;
    int mouseGrabState;

    NodeNum_t minnode = NODENUM_INVALID;
    float mindist;
    ActorPtr grab_truck;
    Ogre::Vector3 lastgrabpos;
    int lastMouseX, lastMouseY;

    // Cached mouse state from OgreBites::InputListener, to minimize code changes
    int m_mouse_x = 0;
    int m_mouse_y = 0;
    // END cached

    void releaseMousePick();
    Ogre::Ray getMouseRay();
    void reset();
};

} // namespace RoR
