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
/// @author Petr Ohlidal, 2022

#pragma once

#include "Collisions.h"
#include "ForwardDeclarations.h"

#include <Ogre.h>
#include <vector>
#include "imgui.h"

namespace RoR {
namespace GUI {

/// Diagnostic view for static terrain collisions and script event-boxes.
class CollisionsDebug
{
public:
    const ImVec4 COLOR_EVENTBOX = ImVec4(181/255.f, 51/255.f, 64/255.f, 1.f);
    const ImVec4 COLOR_COLLMESH = ImVec4(209/255.f, 109/255.f, 44/255.f, 1.f);
    const float  WIDTH_DRAWDIST = 75.f;
    const float  DEFAULT_DRAWDIST = 200.f;

    void SetVisible(bool v);
    bool IsVisible() const { return m_is_visible; }
    bool IsHovered() const { return IsVisible() && m_is_hovered; }

    void Draw();
    void CleanUp();

    // EVENTBOX
    void SetDrawEventBoxes(bool val);
    void AddCollisionBoxDebugMesh(collision_box_t const& coll_box);
    void DrawCollisionBoxDebugText(collision_box_t const& coll_box);
    Ogre::Vector3 GetCollBoxWorldPos(collision_box_t const& coll_box);
    void ClearEventBoxVisuals();

    // COLLMESH
    void SetDrawCollisionMeshes(bool val);
    void AddCollisionMeshDebugMesh(collision_mesh_t const& coll_mesh);
    void DrawCollisionMeshDebugText(collision_mesh_t const& coll_mesh);
    void ClearCollisionMeshVisuals();

    // CELL
    void SetDrawCollisionCells(bool val);
    void ClearCollisionCellVisuals();

private:

    void DrawLabelAtWorldPos(std::string const& caption, Ogre::Vector3 const& world_pos, ImVec4 const& text_color);
    void GenerateCellDebugMaterials();

    // EVENTBOX
    std::vector<Ogre::SceneNode*> m_collision_boxes;
    bool m_draw_collision_boxes = false;
    float m_collision_box_draw_distance = DEFAULT_DRAWDIST;

    // COLLMESH
    std::vector<Ogre::SceneNode*> m_collision_meshes;
    bool m_draw_collision_meshes = false;
    float m_collision_mesh_draw_distance = DEFAULT_DRAWDIST;

    // CELL
    std::vector<Ogre::SceneNode*> m_collision_cells;
    Ogre::SceneNode* m_collision_grid_root = nullptr;
    bool m_draw_collision_cells = false;
    float m_collision_cell_draw_distance = DEFAULT_DRAWDIST;
    int m_cell_generator_distance_limit = 50; // meters from character, to reduce memory load.

    bool m_draw_labels = true;
    bool m_labels_draw_types = true;
    bool m_labels_draw_sources = true;
    bool m_is_visible = false;
    bool m_is_hovered = false;
};

} // namespace GUI
} // namespace RoR
