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
    @author Petr Ohlidal
    @date   03/2015
*/

#include "RigEditor_HighlightBoxesDynamicMesh.h"
#include "RigEditor_Main.h"

#include <OgreMaterialManager.h>
#include <OgreMaterial.h>
#include <OgreTechnique.h>
#include <OgrePass.h>
#include <OgreManualObject.h>
#include <OgreSceneNode.h>
#include <OgreSceneManager.h>

#include <memory>

using namespace RoR;
using namespace RigEditor;

void HighlightBoxesDynamicMesh::CheckAndCreateMaterial(const char* mat_name)
{
    if (Ogre::MaterialManager::getSingleton().resourceExists(mat_name))
    {
        return;
    }
    Ogre::MaterialPtr mat = static_cast<Ogre::MaterialPtr>(
        Ogre::MaterialManager::getSingleton().create(
            mat_name, 
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
        )
    );

    mat->getTechnique(0)->getPass(0)->createTextureUnitState();
    mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureFiltering(Ogre::TFO_ANISOTROPIC);
    mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureAnisotropy(3);
    mat->setLightingEnabled(false);
    mat->setReceiveShadows(false);
}

void HighlightBoxesDynamicMesh::Initialize(
    RigEditor::Main* rig_editor,
    Ogre::SceneManager* ogre_scene_manager, 
    const char* material_name,
    int initial_capacity_boxes
)
{
    assert(material_name != nullptr);
    assert(ogre_scene_manager != nullptr);
    m_dynamic_mesh_capacity_boxes = initial_capacity_boxes;

    this->CheckAndCreateMaterial(material_name);

    m_dynamic_mesh = std::unique_ptr<Ogre::ManualObject>(
            rig_editor->GetOgreSceneManager()->createManualObject()
        );

    const int lines_per_box = 24; // 8 edges, 3 lines each

    // Setup
    m_dynamic_mesh->estimateVertexCount(initial_capacity_boxes * lines_per_box * 2);
    m_dynamic_mesh->setCastShadows(false);
    m_dynamic_mesh->setDynamic(true);
    m_dynamic_mesh->setRenderingDistance(300);

    // Initialize with dummy geometry
    m_dynamic_mesh->begin(material_name, Ogre::RenderOperation::OT_LINE_LIST);
    const int initial_num_lines = initial_capacity_boxes * lines_per_box;
    for (int i = 0; i < initial_num_lines; ++i)
    {
        m_dynamic_mesh->position(Ogre::Vector3::UNIT_X);
        m_dynamic_mesh->colour(Ogre::ColourValue::Red);
        m_dynamic_mesh->position(Ogre::Vector3::UNIT_Y);
        m_dynamic_mesh->colour(Ogre::ColourValue::Green);
    }
    m_dynamic_mesh->end();
}

void HighlightBoxesDynamicMesh::BeginUpdate()
{
    m_dynamic_mesh->beginUpdate(0);
}

void HighlightBoxesDynamicMesh::EndUpdate()
{
    m_dynamic_mesh->end();
}

void HighlightBoxesDynamicMesh::AddLine(Ogre::Vector3 const & start, Ogre::Vector3 const & end, Ogre::ColourValue const & color)
{
    m_dynamic_mesh->position(start);
    m_dynamic_mesh->colour(color);
    m_dynamic_mesh->position(end);
    m_dynamic_mesh->colour(color);
}

void HighlightBoxesDynamicMesh::AddCorner(
    Ogre::Vector3 const & corner_point, 
    Ogre::Vector3 const & line_lengths, 
    Ogre::Vector3 const & line_orientations, 
    Ogre::ColourValue const & color)
{
    float end_x = corner_point.x + (line_lengths.x * line_orientations.x);
    float end_y = corner_point.y + (line_lengths.y * line_orientations.y);
    float end_z = corner_point.z + (line_lengths.z * line_orientations.z);

    AddLine(corner_point, Ogre::Vector3(end_x,          corner_point.y, corner_point.z), color);
    AddLine(corner_point, Ogre::Vector3(corner_point.x, end_y,          corner_point.z), color);
    AddLine(corner_point, Ogre::Vector3(corner_point.x, corner_point.y, end_z),          color);
}

void HighlightBoxesDynamicMesh::AddBox(
        Ogre::Vector3 const & extent_max, 
        Ogre::Vector3 const & extent_min, 
        Ogre::ColourValue const & color)
{
    using namespace Ogre;

    Ogre::Vector3 line_lengths;
    line_lengths.x = (extent_max.x - extent_min.x) / 4;
    line_lengths.y = (extent_max.y - extent_min.y) / 4;
    line_lengths.z = (extent_max.z - extent_min.z) / 4;

    // XYZ min
    this->AddCorner(extent_min, line_lengths, Vector3(1,1,1), color);
    // XY min, Z max
    this->AddCorner(Vector3(extent_min.x, extent_min.y, extent_max.z), line_lengths, Vector3(1,1,-1), color);
    // XZ min, Y max
    this->AddCorner(Vector3(extent_min.x, extent_max.y, extent_min.z), line_lengths, Vector3(1,-1,1), color);
    // YZ min, X max
    this->AddCorner(Vector3(extent_max.x, extent_min.y, extent_min.z), line_lengths, Vector3(-1,1,1), color);
    // XYZ max
    this->AddCorner(extent_max, line_lengths, Vector3(-1, -1, -1), color);
    // XY max, Z min
    this->AddCorner(Vector3(extent_max.x, extent_max.y, extent_min.z), line_lengths, Vector3(-1,-1,1), color);
    // XZ max, Y min
    this->AddCorner(Vector3(extent_max.x, extent_min.y, extent_max.z), line_lengths, Vector3(-1,1,-1), color);
    // YZ max, X min
    this->AddCorner(Vector3(extent_min.x, extent_max.y, extent_max.z), line_lengths, Vector3(1,-1,-1), color);
}

void HighlightBoxesDynamicMesh::DetachFromScene()
{
    if (m_dynamic_mesh->isInScene())
    {
        m_dynamic_mesh->detachFromParent();
    }
}

void HighlightBoxesDynamicMesh::AttachToScene(Ogre::SceneNode* parent_scene_node)
{
    assert(parent_scene_node != nullptr);
    if (!m_dynamic_mesh->isInScene())
    {
        parent_scene_node->attachObject(m_dynamic_mesh.get());
    }
}
