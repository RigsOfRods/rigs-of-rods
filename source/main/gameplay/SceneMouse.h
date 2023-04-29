/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2023 Petr Ohlidal

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
#include <OIS.h>

#include <OIS.h>
#include <Ogre.h>

namespace RoR {

/// @addtogroup Gameplay
/// @{

class SceneMouse
{
public:

    SceneMouse();

    bool mouseMoved(const OIS::MouseEvent& _arg);
    bool mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
    bool mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);

    void InitializeVisuals();
    void UpdateSimulation();
    void UpdateVisuals();
    void DiscardVisuals();

protected:

    Ogre::ManualObject* pickLine;
    Ogre::SceneNode* pickLineNode;
    int mouseGrabState;

    ActorPtr grab_truck; // grabbed node truck
    Ogre::Vector3 lastgrabpos;

    /// @name Mouse node selection with animated highlights: 1. closest node 2. surrounding nodes (animated by distance)
    /// @{
    
    const float HIGHLIGHT_SPHERE_SIZE = 1.f; //!< in meters
    const float GRAB_SPHERE_SIZE = 0.1f; //!< in meters
    const ImVec4 MINNODE_COLOR = ImVec4(1.f, 0.8f, 0.3f, 1.f);
    const float MINNODE_RADIUS = 2;
    const ImVec4 HIGHLIGHTED_NODE_COLOR = ImVec4(0.7f, 1.f, 0.4f, 1.f);
    const float HIGHLIGHTED_NODE_RADIUS_MAX = 10; //!< in pixels
    const float HIGHLIGHTED_NODE_RADIUS_MIN = 0.5; //!< in pixels
    
    struct HighlightedNode
    {
        float distance;
        NodeNum_t nodenum;
    };

    // the grabbable node
    NodeNum_t minnode = NODENUM_INVALID;
    float mindist;
    ActorPtr mintruck;

    // surrounding nodes
    std::vector<HighlightedNode> highlightedNodes;
    float highlightedNodesTopDistance;
    ActorPtr highlightedTruck;

    /// @}

    void releaseMousePick();
    Ogre::Ray getMouseRay();
    void reset();
    void updateMouseHighlights(ActorPtr actor);
    void drawMouseHighlights();
};

/// @}

} // namespace RoR
