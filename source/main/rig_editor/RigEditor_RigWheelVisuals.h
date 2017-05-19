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

#pragma once

#include "ConfigFile.h"
#include "GUI_OpenSaveFileDialog.h"
#include "RigDef_Prerequisites.h"
#include "RigEditor_IMain.h"
#include "RigEditor_ForwardDeclarations.h"
#include "RoRPrerequisites.h"

namespace RoR
{

namespace RigEditor
{

class RigWheelVisuals
{
public:
    RigWheelVisuals():
        m_flags(0)
    {}

    void Init(RigEditor::Main* rig_editor);

    // Visibility

    void AttachToScene(Ogre::SceneNode* parent_scene_node);
    void DetachFromScene();

    // Wheel visualization

    /// Updates or rebuilds dyn. mesh as needed.
    void RefreshWheelsDynamicMeshes(
        Ogre::SceneNode* parent_scene_node, 
        RigEditor::Main* rig_editor,
        std::vector<LandVehicleWheel*> & wheels
        );

    // Wheel selection/hover highlights

    void UpdateWheelsSelectionHighlightBoxes(
            std::vector<LandVehicleWheel*> & wheels, 
            RigEditor::Main* rig_editor, 
            Ogre::SceneNode* parent_scene_node
        );
    void UpdateWheelsMouseHoverHighlightBoxes(
            std::vector<LandVehicleWheel*> & wheels, 
            RigEditor::Main* rig_editor, 
            Ogre::SceneNode* parent_scene_node
        );

    BITMASK_PROPERTY(m_flags,  1,   FLAG_IS_HOVER_DIRTY,       IsHoverDirty,       SetIsHoverDirty)
    BITMASK_PROPERTY(m_flags,  2,   FLAG_IS_SELECTION_DIRTY,   IsSelectionDirty,   SetIsSelectionDirty)

protected:

    /// Forces rebuild of dynamic mesh
    void BuildWheelsGeometryDynamicMesh(
        RigEditor::Main* rig_editor,
        std::vector<LandVehicleWheel*> & wheels
        );
    /// Forces update of dynamic mesh
    void UpdateWheelsGeometryDynamicMesh(
        RigEditor::Main* rig_editor, 
        std::vector<LandVehicleWheel*> & wheels
        );

    std::unique_ptr<Ogre::ManualObject>         m_wheels_dynamic_mesh;
    std::unique_ptr<HighlightBoxesDynamicMesh>  m_wheels_selected_dynamic_mesh;
    std::unique_ptr<HighlightBoxesDynamicMesh>  m_wheels_hovered_dynamic_mesh;
    unsigned int                                m_flags;
};

} // namespace RigEditor

} // namespace RoR
