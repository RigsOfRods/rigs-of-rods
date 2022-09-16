/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2022 Petr Ohlidal

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

#pragma once

#include "ForwardDeclarations.h"

#include <Ogre.h>
#include <vector>

namespace RoR {
namespace GUI {

/// Diagnostic view for static terrain collisions and script event-boxes.
class CollisionsDebug
{
public:
    void SetVisible(bool v) { m_is_visible = v; }
    bool IsVisible() const { return m_is_visible; }
    bool IsHovered() const { return IsVisible() && m_is_hovered; }

    void Draw();
    void CleanUp();

private:
    void AddCollisionBoxDebugMesh(collision_box_t const& coll_box);
    void DrawCollisionBoxDebugText(collision_box_t const& coll_box);
    void DrawLabelAtWorldPos(std::string const& caption, Ogre::Vector3 const& world_pos);
    Ogre::Vector3 GetCollBoxWorldPos(collision_box_t const& coll_box);

    std::vector<Ogre::SceneNode*> m_collision_boxes;
    bool                          m_draw_collision_boxes = false;

    bool m_is_visible = false;
    bool m_is_hovered = false;
};

} // namespace GUI
} // namespace RoR
