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

#pragma once

#include "RigEditor_ForwardDeclarations.h"

#include <OgreMaterialManager.h>
#include <memory>

namespace RoR
{

namespace RigEditor
{

class HighlightBoxesDynamicMesh
{
public:
    HighlightBoxesDynamicMesh():
        m_dynamic_mesh_capacity_boxes(0)
    {}

    void Initialize(
        RigEditor::Main* rig_editor,
        Ogre::SceneManager* ogre_scene_manager, 
        const char* material_name, 
        int initial_capacity_boxes
        );

    void BeginUpdate();
    void EndUpdate();
    void AddBox(Ogre::Vector3 const & extent_max, Ogre::Vector3 const & extent_min, Ogre::ColourValue const & color);
    void AttachToScene(Ogre::SceneNode* parent_scene_node);
    void DetachFromScene();

private:
    void AddLine(Ogre::Vector3 const & start, Ogre::Vector3 const & end, Ogre::ColourValue const & color);
    void CheckAndCreateMaterial(const char* mat_name);
    void AddCorner(
        Ogre::Vector3 const & corner_point, 
        Ogre::Vector3 const & line_lengths, 
        Ogre::Vector3 const & line_orientations, 
        Ogre::ColourValue const & color);

    std::unique_ptr<Ogre::ManualObject> m_dynamic_mesh;
    int                                 m_dynamic_mesh_capacity_boxes;
};

} // namespace RigEditor

} // namespace RoR
