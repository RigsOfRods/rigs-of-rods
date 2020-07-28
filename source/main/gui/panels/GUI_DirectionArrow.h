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

/// @file Race direction arrow and text info (using OGRE Overlay)

#pragma once

#include "ForwardDeclarations.h"

#include <Ogre.h>
#include <Overlay/OgreOverlay.h>
#include <Overlay/OgreTextAreaOverlayElement.h>

namespace RoR {
namespace GUI {

class DirectionArrow
{
public:
    void LoadOverlay(); //!< Must be called after meshes+overlays were loaded
    void CreateArrow(); //!< Must be called again after OGRE scenemanager is cleared.
    void Update(RoR::GfxActor* player);
    bool IsVisible() { return m_overlay->isVisible(); }

private:
    Ogre::SceneNode*              m_node = nullptr;
    Ogre::Overlay*                m_overlay = nullptr;
    Ogre::TextAreaOverlayElement* m_text = nullptr;
    Ogre::TextAreaOverlayElement* m_distance_text = nullptr;
};

} // namespace GUI
} // namespace RoR
