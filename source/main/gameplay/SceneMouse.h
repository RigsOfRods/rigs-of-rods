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
    void UpdateInputEvents();
    void SetMouseAffector(AffectorID_t id) { grab_affectorid = id; }

protected:

    int mouseGrabState;
    AffectorID_t grab_affectorid = AFFECTORID_INVALID;
    ActorPtr grab_truck; // grabbed node truck
    Ogre::Vector3 lastgrabpos;

    /// @name Mouse node (or pinned force) selection with animated highlights: 1. closest node 2. surrounding nodes (animated by distance)
    /// @{
    
    const float HIGHLIGHT_SPHERE_SIZE = 1.f; //!< in meters
    const float GRAB_SPHERE_SIZE = 0.1f; //!< in meters
    const float FORCE_NEWTONS_TO_LINE_LENGTH_RATIO = 15.5f;
    const float FORCE_UNPIN_SPHERE_SIZE = 0.2; //!< in meters
    // Colors and scales are defined in GUI Theme, see GUIManager.h

    struct HighlightedNode
    {
        float distance;
        NodeNum_t nodenum;
    };

    // the grabbable node
    NodeNum_t minnode = NODENUM_INVALID;
    float mindist;
    ActorPtr mintruck;

    // the affector node
    NodeNum_t aff_nodes_minnode = NODENUM_INVALID;
    AffectorID_t aff_nodes_minaffector = AFFECTORID_INVALID;
    float aff_nodes_mindist;
    ActorPtr aff_nodes_mintruck;

    // the PINNED affector's pin position
    AffectorID_t aff_pins_minaffector = AFFECTORID_INVALID;
    float aff_pins_mindist;
    ActorPtr aff_pins_mintruck;

    // surrounding nodes
    std::vector<HighlightedNode> highlightedNodes;
    float highlightedNodesTopDistance;
    ActorPtr highlightedTruck;

    /// @}

    void activateMousePick();
    void releaseMousePick();
    Ogre::Ray getMouseRay();
    void reset();
    void updateMouseNodeHighlights(ActorPtr& actor);
    void updateMouseEffectHighlights(ActorPtr& actor);
    void drawMouseHighlights();
    void drawNodeEffects();
};

/// @}

} // namespace RoR
