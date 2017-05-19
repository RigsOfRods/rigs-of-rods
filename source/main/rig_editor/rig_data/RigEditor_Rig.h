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
    @file   RigEditor_Rig.h
    @date   06/2014
    @author Petr Ohlidal
*/

#pragma once

#include <unordered_map>
#include <OgreAxisAlignedBox.h>

#include "RigDef_File.h"
#include "RigDef_Prerequisites.h"
#include "RigEditor_ForwardDeclarations.h"
#include "RigEditor_Types.h"
#include "RoRPrerequisites.h"
#include "RigEditor_Node.h"

namespace RoR
{

namespace RigEditor
{

class RigBuildingReport
{
public:
    struct Message
    {
        enum Level
        {
            LEVEL_ERROR,
            LEVEL_WARNING,
            LEVEL_INFO,
            LEVEL_INVALID = 0xFFFFFFFF
        };

        Message(Level level, Ogre::String const & text, Ogre::String const & module_name, Ogre::String const & section_name):
            level(level),
            text(text),
            module_name(module_name),
            section_name(section_name)
        {}

        Ogre::String section_name;
        Ogre::String module_name;
        Ogre::String text;
        Level level;
    };

    inline void SetCurrentSectionName(Ogre::String const & sec_name) { m_current_section_name = sec_name; }
    inline void ClearCurrentSectionName() { m_current_section_name.clear(); }

    inline void SetCurrentModuleName(Ogre::String const & sec_name) { m_current_module_name = sec_name; }
    inline void ClearCurrentModuleName() { m_current_module_name.clear(); }

    void AddMessage(Message::Level level, Ogre::String const & text)
    {
        m_messages.push_back(Message(level, text, m_current_module_name, m_current_section_name));
    }

    Ogre::String ToString() const;

private:
    Ogre::String m_current_section_name;
    Ogre::String m_current_module_name;
    std::list<Message> m_messages;
};

class Rig
{
public:

    Rig(Config* config);

    ~Rig();

    void UpdateBoundingBox(Ogre::Vector3 const & point);

    void RefreshNodeScreenPosition(Node & node, CameraHandler* camera_handler);

    /**
    * @return True if new node was focused.
    */
    bool RefreshMouseHoveredNode(Vector2int const & mouse_position);

    void RefreshAllNodesScreenPositions(CameraHandler* camera_handler);

    Node* GetMouseHoveredNode() const
    {
        return m_mouse_hovered_node;
    }

    void AttachToScene(Ogre::SceneNode* parent_scene_node);
    void DetachFromScene();

    bool ToggleMouseHoveredNodeSelected();

    void DeselectAllNodes();

    Node& CreateNewNode(Ogre::Vector3 const & position);

    void ClearMouseHoveredNode();

    void TranslateSelectedNodes(Ogre::Vector3 const & offset, CameraHandler* camera_handler);

    /** Deselects all nodes. If none was selected, selects all.
    */
    void DeselectOrSelectAllNodes();

    /**
    * @return True if beam existed and was deleted, false otherwise.
    */
    bool DeleteBeamBetweenNodes(Node* n1, Node* n2);

    void DeleteBeamsBetweenSelectedNodes();

    int DeleteAttachedBeams(Node* node);

    void DeleteSelectedNodes();

    bool DeleteNode(Node* node);

    bool DeleteBeam(Beam* beam);

    void DeleteBeam(std::vector<RigEditor::Beam*>::iterator & delete_itor);

    void Build(
        std::shared_ptr<RigDef::File> rig_def, 
        RigEditor::Main* rig_editor,
        Ogre::SceneNode* parent_scene_node,
        RigBuildingReport* report // = nullptr
    );

    /** Blender-like extrusion; makes linked copies of selected nodes, selects new nodes.
    */
    void ExtrudeSelectedNodes();

    Beam & CreateNewBeam(Node* n1, Node* n2);

    void SelectedNodesCommitPositionUpdates();

    void SelectedNodesCancelPositionUpdates();

    RigProperties* GetProperties();

    std::shared_ptr<RigDef::File> Export();

    void QuerySelectedNodesData(RigAggregateNodesData* result);

    void QuerySelectedBeamsData(RigAggregateBeams2Data* result);

    void UpdateSelectedBeamsList();

    inline unsigned int GetNumSelectedBeams() const { return m_selected_beams.size(); }

    // Update visuals
    void RefreshNodesDynamicMeshes(Ogre::SceneNode* parent_scene_node);
    void RefreshBeamsDynamicMesh();
    void RefreshWheelsDynamicMesh(Ogre::SceneNode* parent_scene_node, RigEditor::Main* rig_editor);
    void CheckAndRefreshWheelsSelectionHighlights(RigEditor::Main* rig_editor, Ogre::SceneNode* parent_scene_node, bool force = false);
    void CheckAndRefreshWheelsMouseHoverHighlights(RigEditor::Main* rig_editor, Ogre::SceneNode* parent_scene_node);
    
    // Node/beam updaters
    void SelectedNodesUpdateAttributes     (const RigAggregateNodesData      *data);
    void SelectedPlainBeamsUpdateAttributes(const RigAggregatePlainBeamsData *data);
    void SelectedMixedBeamsUpdateAttributes(const MixedBeamsAggregateData    *data);
    void SelectedShocksUpdateAttributes    (const RigAggregateShocksData     *data);
    void SelectedShocks2UpdateAttributes   (const RigAggregateShocks2Data    *data);
    void SelectedHydrosUpdateAttributes    (const RigAggregateHydrosData     *data);
    void SelectedCommands2UpdateAttributes (const RigAggregateCommands2Data  *data);

    // WHEELS

    /// @return True if selection was changed.
    bool SetWheelSelected(LandVehicleWheel* wheel, int index, bool state_selected, RigEditor::Main* rig_editor);
    /// @return True if selection will change
    bool ScheduleSetWheelSelected(LandVehicleWheel* wheel, int index, bool state_selected, RigEditor::Main* rig_editor);
    void SetWheelHovered(LandVehicleWheel* wheel, int index, bool state_hovered, RigEditor::Main* rig_editor);
    /// @return True if selection was changed.
    bool SetAllWheelsSelected(bool state_selected, RigEditor::Main* rig_editor);
    bool ScheduleSetAllWheelsSelected(bool state_selected, RigEditor::Main* rig_editor);
    void SetAllWheelsHovered(bool state_hovered, RigEditor::Main* rig_editor);
    void QuerySelectedWheelsData(AllWheelsAggregateData* data);
    /// @return True if any wheel's geometry changed.
    bool UpdateSelectedWheelsData(AllWheelsAggregateData* data);
    /// @return True if selection was changed.
    bool PerformScheduledWheelSelectionUpdates(RigEditor::Main* rig_editor);

    inline std::vector<LandVehicleWheel*>& GetWheels() { return m_wheels; }

private:

    /** Rig building utility function
    */
    Node* FindNode(RigDef::Node::Ref const & node_ref, RigBuildingReport* logger = nullptr);

    bool GetWheelAxisNodes(RigDef::Node::Ref const & a1, RigDef::Node::Ref const & a2, Node*& axis_inner, Node*& axis_outer, RigBuildingReport* report);

    /// Finds all nodes required for wheel. 
    /// @return False if some valid node was not found.
    bool GetWheelDefinitionNodes(
        RigDef::Node::Ref axis1,
        RigDef::Node::Ref axis2,
        RigDef::Node::Ref rigidity,
        RigDef::Node::Ref reference_arm,
        Node*& out_axis_inner, 
        Node*& out_axis_outer,
        Node*& out_rigidity, 
        Node*& out_reference_arm,
        RigBuildingReport* report
        );

    void BuildFromModule(RigDef::File::Module* module, RigBuildingReport* logger = nullptr);

private:

    /* STRUCTURE */

    std::map<std::string, Node>       m_nodes; // Only named nodes are supported
    std::list<Beam>                   m_beams;
    std::list<CineCamera>             m_cinecameras;
    Ogre::AxisAlignedBox              m_aabb;
    unsigned int                      m_highest_node_id;

    std::vector<LandVehicleWheel*>    m_wheels;
    
    /* PROPERTIES */

    std::unique_ptr<RigProperties>    m_properties;

    /* STATE */
    Node*                m_mouse_hovered_node;
    std::list<Beam*>     m_selected_beams;

    /* VISUALS */
    std::unique_ptr<Ogre::ManualObject>         m_beams_dynamic_mesh;
    std::unique_ptr<Ogre::ManualObject>         m_nodes_dynamic_mesh;
    std::unique_ptr<Ogre::ManualObject>         m_nodes_hover_dynamic_mesh;
    std::unique_ptr<Ogre::ManualObject>         m_nodes_selected_dynamic_mesh;
    std::unique_ptr<RigWheelVisuals>            m_wheel_visuals;

    /* UTILITY */
    bool                 m_modified;
    Config*              m_config;
};

} // namespace RigEditor

} // namespace RoR
