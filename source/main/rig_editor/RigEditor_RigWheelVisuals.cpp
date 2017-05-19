/*
    This source file is part of Rigs of Rods
    Copyright 2013-2016 Petr Ohlidal & contributors

    For more information, see http://www.rigsofrods.com/

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

/** 
    @file   
    @date   03/2015
    @author Petr Ohlidal
*/

#include "RigEditor_RigWheelVisuals.h"

#include "RigEditor_Config.h"
#include "RigEditor_HighlightBoxesDynamicMesh.h"
#include "RigEditor_LandVehicleWheel.h"
#include "RigEditor_Main.h"

#include <OgreTechnique.h>
#include <OgreSceneManager.h>
#include <OgreManualObject.h>
#include <vector>

using namespace RoR;
using namespace RigEditor;

void RigWheelVisuals::Init(RigEditor::Main* rig_editor)
{
    // WHEELS SELECTION + HOVER MESHES
    HighlightBoxesDynamicMesh* dyna_mesh = new HighlightBoxesDynamicMesh();
    dyna_mesh->Initialize(rig_editor, rig_editor->GetOgreSceneManager(), "rig-editor-wheels-selected-dynamic-mesh", 10);
    m_wheels_selected_dynamic_mesh = std::unique_ptr<HighlightBoxesDynamicMesh>(dyna_mesh);

    dyna_mesh = new HighlightBoxesDynamicMesh();
    dyna_mesh->Initialize(rig_editor, rig_editor->GetOgreSceneManager(), "rig-editor-wheels-hovered-dynamic-mesh",  2);
    m_wheels_hovered_dynamic_mesh = std::unique_ptr<HighlightBoxesDynamicMesh>(dyna_mesh);
}

void RigWheelVisuals::BuildWheelsGeometryDynamicMesh(
    RigEditor::Main* rig_editor,
    std::vector<LandVehicleWheel*> & wheels
    )
{
    assert(rig_editor != nullptr);

    // Prepare material
    if (! Ogre::MaterialManager::getSingleton().resourceExists("rig-editor-skeleton-wheels-material"))
    {
        Ogre::MaterialPtr node_mat = static_cast<Ogre::MaterialPtr>(
            Ogre::MaterialManager::getSingleton().create("rig-editor-skeleton-wheels-material", 
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME)
        );

        node_mat->getTechnique(0)->getPass(0)->createTextureUnitState();
        node_mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureFiltering(Ogre::TFO_ANISOTROPIC);
        node_mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureAnisotropy(3);
        node_mat->setLightingEnabled(false);
        node_mat->setReceiveShadows(false);
    }

    // Analyze
    int wheels_vertex_count = 0;
    int wheels_index_count = 0;
    auto wheels_end = wheels.end();
    for (auto wheels_itor = wheels.begin(); wheels_itor != wheels_end; ++wheels_itor)
    {
        wheels_index_count += (*wheels_itor)->GetEdges().size();
        wheels_vertex_count = wheels_index_count * 2; // Vertices are generated per edge
    }

    // Create mesh
    assert(m_wheels_dynamic_mesh.get() == nullptr);
    m_wheels_dynamic_mesh = std::unique_ptr<Ogre::ManualObject>(
            rig_editor->GetOgreSceneManager()->createManualObject()
        );
    m_wheels_dynamic_mesh->estimateVertexCount(wheels_vertex_count);
    m_wheels_dynamic_mesh->estimateIndexCount(wheels_index_count);
    m_wheels_dynamic_mesh->setCastShadows(false);
    m_wheels_dynamic_mesh->setDynamic(true);
    m_wheels_dynamic_mesh->setRenderingDistance(300);

    // Init
    m_wheels_dynamic_mesh->begin("rig-editor-skeleton-wheels-material", Ogre::RenderOperation::OT_LINE_LIST);

    // Process
    for (auto wheels_itor = wheels.begin(); wheels_itor != wheels_end; ++wheels_itor)
    {
        auto & vertices = (*wheels_itor)->GetVertices();
        auto & edges = (*wheels_itor)->GetEdges();

        auto edge_end = edges.end();
        for (auto edge_itor = edges.begin(); edge_itor != edge_end; ++edge_itor)
        {
            LandVehicleWheel::Edge edge = *edge_itor;
            Ogre::ColourValue color = LandVehicleWheel::Edge::GetColorValue(edge.type, rig_editor->GetConfig());

            m_wheels_dynamic_mesh->position(vertices[edge.indices[0]]);
            m_wheels_dynamic_mesh->colour(color);
            m_wheels_dynamic_mesh->position(vertices[edge.indices[1]]);
            m_wheels_dynamic_mesh->colour(color);
        }
    }
    
    // Finalize
    m_wheels_dynamic_mesh->end();
}

void RigWheelVisuals::RefreshWheelsDynamicMeshes(
    Ogre::SceneNode* parent_scene_node, 
    RigEditor::Main* rig_editor,
    std::vector<LandVehicleWheel*> & wheels
    )
{
    assert(parent_scene_node != nullptr);
    assert(rig_editor != nullptr);
    
    bool is_any_selected = false;
    bool is_any_hovered = false;
    bool is_any_geometry_dirty = false;
    bool is_any_state_dirty = false;
    bool has_ray_count_changed = false;

    if (m_wheels_dynamic_mesh.get() == nullptr)
    {
        is_any_state_dirty = true;
        has_ray_count_changed = true;
    }

    auto itor_end = wheels.end();
    for (auto itor = wheels.begin(); itor != itor_end; ++itor)
    {
        LandVehicleWheel* wheel = *itor;
        is_any_selected       = is_any_selected || wheel->IsSelected();
        is_any_hovered        = is_any_hovered || wheel->IsHovered();
        is_any_state_dirty    = is_any_state_dirty || wheel->IsSelectionStateDirty();
        has_ray_count_changed = has_ray_count_changed || wheel->HasRayCountChanged();

        if (wheel->IsGeometryDirty())
        {
            is_any_geometry_dirty = true;
            wheel->ReGenerateMeshData();
        }
    }

    if (is_any_geometry_dirty)
    {
        if (has_ray_count_changed)
        {
            // Rebuild the wheels dynamic mesh
            bool attach_to_scene = false;
            if (m_wheels_dynamic_mesh.get() != nullptr)
            {
                attach_to_scene = m_wheels_dynamic_mesh->isInScene();
                m_wheels_dynamic_mesh->detachFromParent();
                m_wheels_dynamic_mesh.reset();
            }
            this->BuildWheelsGeometryDynamicMesh(rig_editor, wheels);
            if (attach_to_scene)
            {
                parent_scene_node->attachObject(m_wheels_dynamic_mesh.get());
            }
        }
        else
        {
            // Update existing mesh
            this->UpdateWheelsGeometryDynamicMesh(rig_editor, wheels);
        }
    }
}

void RigWheelVisuals::UpdateWheelsGeometryDynamicMesh(
        RigEditor::Main* rig_editor,
        std::vector<LandVehicleWheel*> & wheels
        )
{
    assert(m_wheels_dynamic_mesh.get() != nullptr);
    // Init
    m_wheels_dynamic_mesh->beginUpdate(0);

    // Process
    auto wheels_end = wheels.end();
    for (auto wheels_itor = wheels.begin(); wheels_itor != wheels_end; ++wheels_itor)
    {
        auto & vertices = (*wheels_itor)->GetVertices();
        auto & edges = (*wheels_itor)->GetEdges();

        auto edge_end = edges.end();
        for (auto edge_itor = edges.begin(); edge_itor != edge_end; ++edge_itor)
        {
            LandVehicleWheel::Edge edge = *edge_itor;
            Ogre::ColourValue color = LandVehicleWheel::Edge::GetColorValue(edge.type, rig_editor->GetConfig());

            m_wheels_dynamic_mesh->position(vertices[edge.indices[0]]);
            m_wheels_dynamic_mesh->colour(color);
            m_wheels_dynamic_mesh->position(vertices[edge.indices[1]]);
            m_wheels_dynamic_mesh->colour(color);
        }
    }
    
    // Finalize
    m_wheels_dynamic_mesh->end();
}

void RigWheelVisuals::AttachToScene(Ogre::SceneNode* parent_scene_node)
{
    assert(parent_scene_node != nullptr);
    if (m_wheels_dynamic_mesh.get() != nullptr)
    {
        parent_scene_node->attachObject(m_wheels_dynamic_mesh.get());
    }
}

void RigWheelVisuals::DetachFromScene()
{
    if (m_wheels_dynamic_mesh.get() != nullptr)
    {
        m_wheels_dynamic_mesh->detachFromParent();
    }
}

void RigWheelVisuals::UpdateWheelsSelectionHighlightBoxes(
        std::vector<LandVehicleWheel*> & wheels, 
        RigEditor::Main* rig_editor,
        Ogre::SceneNode* parent_scene_node
        )
{
    // Check if any wheel is selected
    auto end = wheels.end();
    bool selected_found = false;
    for (auto itor = wheels.begin(); itor != end; ++itor)
    {
        LandVehicleWheel* wheel = *itor;
        if (wheel->IsSelected())
        {
            selected_found = true;
            break;
        }
    }
    if (!selected_found)
    {
        m_wheels_selected_dynamic_mesh->DetachFromScene();
        this->SetIsSelectionDirty(false);
        return;
    }

    // Update selection highlight
    float box_padding = rig_editor->GetConfig()->wheels_selection_highlight_boxes_padding;
    m_wheels_selected_dynamic_mesh->BeginUpdate();
    for (auto itor = wheels.begin(); itor != end; ++itor)
    {
        LandVehicleWheel* wheel = *itor;
        if (wheel->IsSelected())
        {
            m_wheels_selected_dynamic_mesh->AddBox(
                wheel->GetAabbEdgesMax(box_padding), 
                wheel->GetAabbEdgesMin(box_padding), 
                rig_editor->GetConfig()->wheels_selection_highlight_boxes_color);
        }
    }
    m_wheels_selected_dynamic_mesh->EndUpdate();
    m_wheels_selected_dynamic_mesh->AttachToScene(parent_scene_node);
    this->SetIsSelectionDirty(false);
}

void RigWheelVisuals::UpdateWheelsMouseHoverHighlightBoxes(
        std::vector<LandVehicleWheel*> & wheels, 
        RigEditor::Main* rig_editor, 
        Ogre::SceneNode* parent_scene_node
        )
{
    // Check if any wheel is hovered
    auto end = wheels.end();
    bool hovered_found = false;
    for (auto itor = wheels.begin(); itor != end; ++itor)
    {
        LandVehicleWheel* wheel = *itor;
        if (wheel->IsHovered())
        {
            hovered_found = true;
            break;
        }
    }
    if (!hovered_found)
    {
        m_wheels_hovered_dynamic_mesh->DetachFromScene();
        this->SetIsHoverDirty(false);
        return;
    }

    // Update hover highlight
    float box_padding = rig_editor->GetConfig()->wheels_hover_highlight_boxes_padding;
    m_wheels_hovered_dynamic_mesh->BeginUpdate();
    for (auto itor = wheels.begin(); itor != end; ++itor)
    {
        LandVehicleWheel* wheel = *itor;
        if (wheel->IsHovered())
        {
            m_wheels_hovered_dynamic_mesh->AddBox(
                wheel->GetAabbEdgesMax(box_padding), 
                wheel->GetAabbEdgesMin(box_padding), 
                rig_editor->GetConfig()->wheels_hover_highlight_boxes_color);
        }
    }
    m_wheels_hovered_dynamic_mesh->EndUpdate();
    m_wheels_hovered_dynamic_mesh->AttachToScene(parent_scene_node);
    this->SetIsHoverDirty(false);
}
