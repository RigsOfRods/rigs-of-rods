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
    @file   RigEditor_Rig.cpp
    @date   08/2014
    @author Petr Ohlidal
*/

#include "RigEditor_Rig.h"

#include "RigDef_File.h"
#include "RigEditor_Beam.h"
#include "RigEditor_CineCamera.h"
#include "RigEditor_CameraHandler.h"
#include "RigEditor_Config.h"
#include "RigEditor_FlexBodyWheel.h"
#include "RigEditor_HighlightBoxesDynamicMesh.h"
#include "RigEditor_Main.h"
#include "RigEditor_MeshWheel.h"
#include "RigEditor_MeshWheel2.h"
#include "RigEditor_Node.h"
#include "RigEditor_RigProperties.h"
#include "RigEditor_RigElementsAggregateData.h"
#include "RigEditor_RigWheelsAggregateData.h"
#include "RigEditor_RigWheelVisuals.h"

#include <OgreManualObject.h>
#include <OgreMaterialManager.h>
#include <OgreTechnique.h>
#include <OgreManualObject.h>
#include <OgreSceneManager.h>
#include <OgreRenderOperation.h>
#include <sstream>

using namespace RoR;
using namespace RoR::RigEditor;

Ogre::String RigBuildingReport::ToString() const
{
    std::stringstream s;
    auto end = m_messages.end();
    auto itor = m_messages.begin();
    for (; itor != end; ++itor)
    {
        switch (itor->level)
        {
        case Message::LEVEL_ERROR:
            s << "ERROR ";
            break;
        case Message::LEVEL_INFO:
            s << "INFO ";
            break;
        case Message::LEVEL_WARNING:
            s << "WARNING ";
            break;
        default:
            assert(false && "Wrong message type");
            break;
        }
        s << "(module: " << itor->module_name << ", section: " << itor->section_name << "): " << itor->text << std::endl;
    }
    return s.str();
}

// ============================================================================

Rig::Rig(Config* config):
    m_mouse_hovered_node(nullptr),
    m_aabb(Ogre::AxisAlignedBox::BOX_NULL),
    m_config(config),
    m_modified(false)
{}

Rig::~Rig()
{
    // NOTE: visuals deleted by std::unique_ptr

    // == Clear structure ==

    // Beams
    auto beams_end = m_beams.end();
    for (auto beam_itor = m_beams.begin(); beam_itor != beams_end; ++beam_itor)
    {
        beam_itor->DeleteDefinition();
    }
    m_beams.clear();

    // Nodes
    m_nodes.clear();
}

Node* Rig::FindNode(RigDef::Node::Ref const & node_ref, RigBuildingReport* logger /* = nullptr */ )
{
    // Find node
    auto result = m_nodes.find(node_ref.Str());
    if (result == m_nodes.end())
    {
        // Node not found
        if (logger != nullptr)
        {
            logger->AddMessage(RigBuildingReport::Message::LEVEL_WARNING, std::string("Node not found: ") + node_ref.ToString());
        }
        return nullptr;
    }
    return & result->second;
}


void Rig::Build(
        std::shared_ptr<RigDef::File> rig_def,
        RigEditor::Main* rig_editor,
        Ogre::SceneNode* parent_scene_node,
        RigBuildingReport* report // = nullptr
    )
{
    assert(parent_scene_node != nullptr);
    assert(rig_editor != nullptr);

    RigEditor::Config & config = *rig_editor->GetConfig();
    RigDef::File::Module* module = rig_def->root_module.get();
    if (report != nullptr)
    {
        report->SetCurrentModuleName(module->name);
    }

    m_properties = std::unique_ptr<RigProperties>(new RigProperties());
    m_properties->Import(rig_def);
    
    // ##### Process nodes (section "nodes") #####

    int node_index = 0;
    if (report != nullptr)
    {
        report->SetCurrentSectionName("nodes");
    }
    for (auto node_itor = module->nodes.begin(); node_itor != module->nodes.end(); ++node_itor)
    {
        bool done = false;
        RigDef::Node & node_def = *node_itor;
        RigDef::Node::Id & node_id = node_itor->id;
        while (true)
        {
            auto result = m_nodes.insert( std::make_pair(node_id.Str(), Node(node_def)) );
            if (result.second == true)
            {
                // Update bounding box
                m_aabb.merge(node_itor->position);

                break;
            }
            else if (report != nullptr)
            {
                std::stringstream msg;
                msg << "Duplicate node ID: " << node_id.ToString();
                node_id.SetStr(node_id.Str() + "-dup");
                msg << ", changed to: " << node_id.ToString();
                report->AddMessage(RigBuildingReport::Message::LEVEL_WARNING, msg.str());
                m_modified = true;
            }
        }
        node_index++;
    }
    m_highest_node_id = m_nodes.size();

    // ##### Process beams (section "beams") #####

    std::vector<RigDef::Beam> unlinked_beams_to_retry; // Linked to invalid nodes, will retry after other sections (esp. wheels) were processed
    // TODO: Implement the retry.
    if (report != nullptr)
    {
        report->SetCurrentSectionName("beams");
    }
    auto beam_itor_end = module->beams.end();
    for (auto beam_itor = module->beams.begin(); beam_itor != beam_itor_end; ++beam_itor)
    {
        Node* nodes[] = {
            FindNode(beam_itor->nodes[0], report),
            FindNode(beam_itor->nodes[1], report)
        };

        if (nodes[0] == nullptr || nodes[1] == nullptr)
        {
            continue; // Error already logged by FindNode()
        }
        
        // Colorize beams
        unsigned int beam_options = beam_itor->options;
        Ogre::ColourValue const & color =
            (BITMASK_IS_1(beam_options, RigDef::Beam::OPTION_i_INVISIBLE))
            ?	m_config->beam_invisible_color
            :	(BITMASK_IS_1(beam_options, RigDef::Beam::OPTION_r_ROPE))
                ?	m_config->beam_rope_color
                :	(BITMASK_IS_1(beam_options, RigDef::Beam::OPTION_s_SUPPORT))
                    ?	m_config->beam_support_color
                    :	m_config->beam_generic_color;
            
        // Save
        RigDef::Beam* def_ptr = new RigDef::Beam(*beam_itor);
        Beam beam(static_cast<void*>(def_ptr), Beam::TYPE_PLAIN, nodes[0], nodes[1]);
        m_beams.push_back(beam);
        m_beams.back().SetColor(color);
        nodes[0]->m_linked_beams.push_back(&m_beams.back());
        nodes[1]->m_linked_beams.push_back(&m_beams.back());
    }

    // ##### Process steering hydros (section "hydros") #####

    auto hydro_itor_end = module->hydros.end();
    if (report != nullptr)
    {
        report->SetCurrentSectionName("hydros");
    }
    for (auto hydro_itor = module->hydros.begin(); hydro_itor != hydro_itor_end; ++hydro_itor)
    {
        Node* nodes[] = {
            FindNode(hydro_itor->nodes[0], report),
            FindNode(hydro_itor->nodes[1], report)
        };

        if (nodes[0] == nullptr || nodes[1] == nullptr)
        {
            continue; // Error already logged by FindNode()
        }
            
        // Save
        auto def_ptr = new RigDef::Hydro(*hydro_itor);
        Beam beam(static_cast<void*>(def_ptr), Beam::TYPE_STEERING_HYDRO, nodes[0], nodes[1]);
        m_beams.push_back(beam);
        m_beams.back().SetColor(m_config->steering_hydro_beam_color_rgb);
        nodes[0]->m_linked_beams.push_back(&m_beams.back());
        nodes[1]->m_linked_beams.push_back(&m_beams.back());
    }

    // ##### Process command-hydros (section "commands[N]") #####

    auto command_itor_end = module->commands_2.end();
    if (report != nullptr)
    {
        report->SetCurrentSectionName("commands2");
    }
    for (auto command_itor = module->commands_2.begin(); command_itor != command_itor_end; ++command_itor)
    {
        Node* nodes[] = {
            FindNode(command_itor->nodes[0], report),
            FindNode(command_itor->nodes[1], report)
        };

        if (nodes[0] == nullptr || nodes[1] == nullptr)
        {
            continue; // Error already logged by FindNode()
        }
            
        // Save
        auto def_ptr = new RigDef::Command2(*command_itor);
        Beam beam(static_cast<void*>(def_ptr), Beam::TYPE_COMMAND_HYDRO, nodes[0], nodes[1]);
        m_beams.push_back(beam);
        m_beams.back().SetColor(m_config->command_hydro_beam_color_rgb);
        nodes[0]->m_linked_beams.push_back(&m_beams.back());
        nodes[1]->m_linked_beams.push_back(&m_beams.back());
    }

    // ##### Process section "shocks" #####

    auto shock_itor_end = module->shocks.end();
    if (report != nullptr)
    {
        report->SetCurrentSectionName("shocks");
    }
    for (auto shock_itor = module->shocks.begin(); shock_itor != shock_itor_end; ++shock_itor)
    {
        Node* nodes[] = {
            FindNode(shock_itor->nodes[0], report),
            FindNode(shock_itor->nodes[1], report)
        };

        if (nodes[0] == nullptr || nodes[1] == nullptr)
        {
            continue; // Error already logged by FindNode()
        }
            
        // Save
        auto def_ptr = new RigDef::Shock(*shock_itor);
        Beam beam(static_cast<void*>(def_ptr), Beam::TYPE_SHOCK_ABSORBER, nodes[0], nodes[1]);
        m_beams.push_back(beam);
        m_beams.back().SetColor(m_config->shock_absorber_beam_color_rgb);
        nodes[0]->m_linked_beams.push_back(&m_beams.back());
        nodes[1]->m_linked_beams.push_back(&m_beams.back());
    }

    // ##### Process section "shocks2" #####

    auto shock2_itor_end = module->shocks_2.end();
    if (report != nullptr)
    {
        report->SetCurrentSectionName("shocks2");
    }
    for (auto shock2_itor = module->shocks_2.begin(); shock2_itor != shock2_itor_end; ++shock2_itor)
    {
        Node* nodes[] = {
            FindNode(shock2_itor->nodes[0], report),
            FindNode(shock2_itor->nodes[1], report)
        };

        if (nodes[0] == nullptr || nodes[1] == nullptr)
        {
            continue; // Error already logged by FindNode()
        }
            
        // Save
        auto def_ptr = new RigDef::Shock2(*shock2_itor);
        Beam beam(static_cast<void*>(def_ptr), Beam::TYPE_SHOCK_ABSORBER_2, nodes[0], nodes[1]);
        m_beams.push_back(beam);
        m_beams.back().SetColor(m_config->shock_absorber_2_beam_color_rgb);
        nodes[0]->m_linked_beams.push_back(&m_beams.back());
        nodes[1]->m_linked_beams.push_back(&m_beams.back());
    }

    // ##### Process cine cameras (section "cinecam") #####
    auto cinecam_itor_end = module->cinecam.end();
    if (report != nullptr)
    {
        report->SetCurrentSectionName("cinecam");
    }
    for (auto cinecam_itor = module->cinecam.begin(); cinecam_itor != cinecam_itor_end; ++cinecam_itor)
    {
        RigDef::Cinecam & cinecam_def = *cinecam_itor;
        m_cinecameras.push_back(CineCamera(cinecam_def));
        CineCamera & editor_cinecam = m_cinecameras.back();

        // === Cinecam node ===
        RigDef::Node node_def;
        node_def.position = cinecam_def.position;
        node_def.id.SetNum(m_highest_node_id++);
        auto result = m_nodes.insert( std::make_pair(node_def.id.Str(), Node(node_def)) );
        if (result.second == false)
        {
            // Insert failed
            if (report != nullptr)
            {
                report->AddMessage(RigBuildingReport::Message::LEVEL_ERROR, "FATAL: Failed to insert cinecam node.");
            }
            continue;
        }
        // Node created OK
        Node & cinecam_node = result.first->second;

        // Set node type
        BITMASK_SET_1(cinecam_node.m_flags, RigEditor::Node::Flags::SOURCE_CINECAM);

        // Update bounding box
        m_aabb.merge(node_def.position);

        // === Cinecam beams ===
        Node* nodes[] = {
            FindNode(cinecam_def.nodes[0], report),
            FindNode(cinecam_def.nodes[1], report),
            FindNode(cinecam_def.nodes[2], report),
            FindNode(cinecam_def.nodes[3], report),
            FindNode(cinecam_def.nodes[4], report),
            FindNode(cinecam_def.nodes[5], report),
            FindNode(cinecam_def.nodes[6], report),
            FindNode(cinecam_def.nodes[7], report),
        };
        
        // All nodes found?
        if (!nodes[0] | !nodes[1] | !nodes[2] | !nodes[3] | !nodes[4] | !nodes[5] | !nodes[6] | !nodes[7])
        {
            // Nope
            if (report != nullptr)
            {
                std::stringstream msg;
                msg << "Some cinecam nodes were not found (";
                msg <<   "0=" << (nodes[0] ? "ok":"NULL");
                msg << ", 1=" << (nodes[1] ? "ok":"NULL");
                msg << ", 2=" << (nodes[2] ? "ok":"NULL");
                msg << ", 3=" << (nodes[3] ? "ok":"NULL");
                msg << ", 4=" << (nodes[4] ? "ok":"NULL");
                msg << ", 5=" << (nodes[5] ? "ok":"NULL");
                msg << ", 6=" << (nodes[6] ? "ok":"NULL");
                msg << ", 7=" << (nodes[7] ? "ok":"NULL");
                msg << ")";
                report->AddMessage(RigBuildingReport::Message::LEVEL_ERROR, msg.str());
            }
            continue;
        }

        // Create beams
        void* cinecam_void_ptr = static_cast<void*>(&editor_cinecam);
        for (int i = 0; i < 8; ++i)
        {
            m_beams.push_back(Beam(cinecam_void_ptr, Beam::TYPE_CINECAM, &cinecam_node, nodes[i]));
            Beam & beam = m_beams.back();
            beam.SetColor(m_config->cinecam_beam_color_rgb);
            cinecam_node.m_linked_beams.push_back(&beam);
            nodes[i]->m_linked_beams.push_back(&beam);
        }
    }
    
    BuildFromModule(module, report);

    // ========================================================================
    // Generating visual meshes
    // ========================================================================

    // ##### CREATE MESH OF BEAMS #####

    /* Prepare material */
    static const Ogre::String beams_mat_name("rig-editor-skeleton-material");
    if (! Ogre::MaterialManager::getSingleton().resourceExists(beams_mat_name))
    {
        Ogre::MaterialPtr mat = static_cast<Ogre::MaterialPtr>(
            Ogre::MaterialManager::getSingleton().create(beams_mat_name, 
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME)
        );

        mat->getTechnique(0)->getPass(0)->createTextureUnitState();
        mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureFiltering(Ogre::TFO_ANISOTROPIC);
        mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureAnisotropy(3);
        mat->setLightingEnabled(false);
        mat->setReceiveShadows(false);
    }

    /* Create mesh */
    m_beams_dynamic_mesh = std::unique_ptr<Ogre::ManualObject>(rig_editor->GetOgreSceneManager()->createManualObject());
    m_beams_dynamic_mesh->estimateIndexCount(m_beams.size() * 2);
    m_beams_dynamic_mesh->estimateVertexCount(m_nodes.size());
    m_beams_dynamic_mesh->setCastShadows(false);
    m_beams_dynamic_mesh->setDynamic(true);
    m_beams_dynamic_mesh->setRenderingDistance(300);

    /* Init */
    m_beams_dynamic_mesh->begin(beams_mat_name, Ogre::RenderOperation::OT_LINE_LIST);

    /* Process beams */
    for (auto itor = m_beams.begin(); itor != m_beams.end(); ++itor)
    {
        Beam & beam = *itor;
        m_beams_dynamic_mesh->position(beam.GetNodeA()->GetPosition());
        m_beams_dynamic_mesh->colour(beam.GetColor());
        m_beams_dynamic_mesh->position(beam.GetNodeB()->GetPosition());
        m_beams_dynamic_mesh->colour(beam.GetColor());
    }

    /* Finalize */
    m_beams_dynamic_mesh->end();

    // ##### CREATE MESH OF NODES #####

    /* Prepare material */
    static const Ogre::String nodes_material_name("rig-editor-skeleton-nodes-material");
    if (! Ogre::MaterialManager::getSingleton().resourceExists(nodes_material_name))
    {
        Ogre::MaterialPtr node_mat = static_cast<Ogre::MaterialPtr>(
            Ogre::MaterialManager::getSingleton().create(nodes_material_name, 
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME)
        );

        node_mat->getTechnique(0)->getPass(0)->createTextureUnitState();
        node_mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureFiltering(Ogre::TFO_ANISOTROPIC);
        node_mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureAnisotropy(3);
        node_mat->setLightingEnabled(false);
        node_mat->setReceiveShadows(false);
        node_mat->setPointSize(m_config->node_generic_point_size);
    }

    /* Create mesh */
    m_nodes_dynamic_mesh = std::unique_ptr<Ogre::ManualObject>(
            rig_editor->GetOgreSceneManager()->createManualObject()
        );
    m_nodes_dynamic_mesh->estimateVertexCount(m_nodes.size());
    m_nodes_dynamic_mesh->setCastShadows(false);
    m_nodes_dynamic_mesh->setDynamic(true);
    m_nodes_dynamic_mesh->setRenderingDistance(300);

    /* Init */
    m_nodes_dynamic_mesh->begin(nodes_material_name, Ogre::RenderOperation::OT_POINT_LIST);

    /* Process nodes */
    for (auto itor = m_nodes.begin(); itor != m_nodes.end(); itor++)
    {
        m_nodes_dynamic_mesh->position((*itor).second.GetPosition());
        m_nodes_dynamic_mesh->colour(m_config->node_generic_color);
    }

    /* Finalize */
    m_nodes_dynamic_mesh->end();

    /* CREATE MESH OF HOVERED NODES */

    /* Prepare material */
    if (! Ogre::MaterialManager::getSingleton().resourceExists("rig-editor-skeleton-nodes-hover-material"))
    {
        Ogre::MaterialPtr node_mat = static_cast<Ogre::MaterialPtr>(
            Ogre::MaterialManager::getSingleton().create("rig-editor-skeleton-nodes-hover-material", 
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME)
        );

        node_mat->getTechnique(0)->getPass(0)->createTextureUnitState();
        node_mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureFiltering(Ogre::TFO_ANISOTROPIC);
        node_mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureAnisotropy(3);
        node_mat->setLightingEnabled(false);
        node_mat->setReceiveShadows(false);
        node_mat->setPointSize(m_config->node_hover_point_size);
    }

    /* Create mesh */
    m_nodes_hover_dynamic_mesh = std::unique_ptr<Ogre::ManualObject>(
            rig_editor->GetOgreSceneManager()->createManualObject()
        );
    m_nodes_hover_dynamic_mesh->estimateVertexCount(10);
    m_nodes_hover_dynamic_mesh->setCastShadows(false);
    m_nodes_hover_dynamic_mesh->setDynamic(true);
    m_nodes_hover_dynamic_mesh->setRenderingDistance(300);

    /* Init */
    m_nodes_hover_dynamic_mesh->begin("rig-editor-skeleton-nodes-hover-material", Ogre::RenderOperation::OT_POINT_LIST);

    /* Fill some dummy vertices (to properly initialise the object) */
    m_nodes_hover_dynamic_mesh->position(Ogre::Vector3::UNIT_X);
    m_nodes_hover_dynamic_mesh->colour(Ogre::ColourValue::Red);
    m_nodes_hover_dynamic_mesh->position(Ogre::Vector3::UNIT_Y);
    m_nodes_hover_dynamic_mesh->colour(Ogre::ColourValue::Green);
    m_nodes_hover_dynamic_mesh->position(Ogre::Vector3::UNIT_Z);
    m_nodes_hover_dynamic_mesh->colour(Ogre::ColourValue::Blue);

    /* Finalize */
    m_nodes_hover_dynamic_mesh->end();

    /* CREATE MESH OF SELECTED NODES */

    /* Prepare material */
    static const Ogre::String nodes_selected_mat_name("rig-editor-skeleton-nodes-selected-material");
    if (! Ogre::MaterialManager::getSingleton().resourceExists(nodes_selected_mat_name))
    {
        Ogre::MaterialPtr node_mat = static_cast<Ogre::MaterialPtr>(
            Ogre::MaterialManager::getSingleton().create(
                nodes_selected_mat_name, 
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
            )
        );

        node_mat->getTechnique(0)->getPass(0)->createTextureUnitState();
        node_mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureFiltering(Ogre::TFO_ANISOTROPIC);
        node_mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureAnisotropy(3);
        node_mat->setLightingEnabled(false);
        node_mat->setReceiveShadows(false);
        node_mat->setPointSize(m_config->node_selected_point_size);
    }

    /* Create mesh */
    m_nodes_selected_dynamic_mesh = std::unique_ptr<Ogre::ManualObject>(
            rig_editor->GetOgreSceneManager()->createManualObject()
        );
    m_nodes_selected_dynamic_mesh->estimateVertexCount(10);
    m_nodes_selected_dynamic_mesh->setCastShadows(false);
    m_nodes_selected_dynamic_mesh->setDynamic(true);
    m_nodes_selected_dynamic_mesh->setRenderingDistance(300);

    /* Init */
    m_nodes_selected_dynamic_mesh->begin(nodes_selected_mat_name, Ogre::RenderOperation::OT_POINT_LIST);

    /* Fill some dummy vertices (to properly initialise the object) */
    m_nodes_selected_dynamic_mesh->position(Ogre::Vector3::UNIT_X);
    m_nodes_selected_dynamic_mesh->colour(Ogre::ColourValue::Red);
    m_nodes_selected_dynamic_mesh->position(Ogre::Vector3::UNIT_Y);
    m_nodes_selected_dynamic_mesh->colour(Ogre::ColourValue::Green);
    m_nodes_selected_dynamic_mesh->position(Ogre::Vector3::UNIT_Z);
    m_nodes_selected_dynamic_mesh->colour(Ogre::ColourValue::Blue);

    /* Finalize */
    m_nodes_selected_dynamic_mesh->end();

    // WHEEL VISUALS

    m_wheel_visuals = std::unique_ptr<RigWheelVisuals>(new RigWheelVisuals());
    m_wheel_visuals->Init(rig_editor);
    m_wheel_visuals->RefreshWheelsDynamicMeshes(parent_scene_node, rig_editor, m_wheels);
}

bool Rig::GetWheelAxisNodes(RigDef::Node::Ref const & a1, RigDef::Node::Ref const & a2, Node*& axis_inner, Node*& axis_outer, RigBuildingReport* report)
{
    axis_inner = FindNode(a1, report);
    axis_outer = FindNode(a2, report);
    if (axis_inner == nullptr || axis_outer == nullptr)
    {
        if (report != nullptr)
        {
            report->AddMessage(RigBuildingReport::Message::LEVEL_ERROR, "Some axis nodes were not found");
        }
        return false;
    }
    if (axis_inner->GetPosition().z > axis_outer->GetPosition().z)
    {
        Node* swap = axis_inner;
        axis_inner = axis_outer;
        axis_outer = swap;
    }
    return true;
}

bool Rig::GetWheelDefinitionNodes(
    RigDef::Node::Ref axis1,
    RigDef::Node::Ref axis2,
    RigDef::Node::Ref rigidity,
    RigDef::Node::Ref reference_arm,
    Node*& out_axis_inner, 
    Node*& out_axis_outer,
    Node*& out_rigidity, 
    Node*& out_reference_arm,
    RigBuildingReport* report
    )
{
    this->GetWheelAxisNodes(axis1, axis2, out_axis_inner, out_axis_outer, report);
    // Rigidity node (leave NULL if undefined)
    Node* rigidity_ptr = nullptr;
    if (rigidity.IsValidAnyState())
    {
        rigidity_ptr = FindNode(rigidity, report);
        if (rigidity_ptr == nullptr)
        {
            if (report != nullptr)
            {
                report->AddMessage(
                    RigBuildingReport::Message::LEVEL_ERROR, 
                    Ogre::String("Failed to find rigidity node: ") + rigidity.ToString());
            }
            return false;
        }
        out_rigidity = rigidity_ptr;
    }
    // Reference arm node (set to inner axis node if undefined)
    Node* ref_arm_ptr = out_axis_inner;
    if (reference_arm.IsValidAnyState())
    {
        ref_arm_ptr = FindNode(reference_arm, report);
        if (ref_arm_ptr == nullptr)
        {
            if (report != nullptr)
            {
                report->AddMessage(
                    RigBuildingReport::Message::LEVEL_ERROR, 
                    Ogre::String("Failed to find reference arm node: ") + reference_arm.ToString());
            }
            return false;
        }
        out_reference_arm = ref_arm_ptr;
    }
    return true;
}

void Rig::BuildFromModule(RigDef::File::Module* module, RigBuildingReport* report /*=nullptr*/)
{
    // FLEXBODYWHEELS
    if (report != nullptr)
    {
        report->SetCurrentSectionName("flexbodywheels");
    }
    auto flexbodywheels_end = module->flex_body_wheels.end();
    for (auto itor = module->flex_body_wheels.begin(); itor != flexbodywheels_end; ++itor)
    {
        Node* axis_inner = nullptr;
        Node* axis_outer = nullptr;
        Node* rigidity = nullptr;
        Node* reference_arm = nullptr;
        RigDef::FlexBodyWheel & def = *itor;
        bool nodes_found = GetWheelDefinitionNodes(
            def.nodes[0], def.nodes[1], def.rigidity_node, def.reference_arm_node, // Input
            axis_inner, axis_outer, rigidity, reference_arm, // Output
            report);
        if (!nodes_found)
        {
            continue; // Error already logged
        }

        auto wheel = new FlexBodyWheel(def, axis_inner, axis_outer, rigidity, reference_arm);
        wheel->SetGeometryIsDirty(true);
        m_wheels.push_back(wheel);
    }
    
    // MESHWHEELS_2
    if (report != nullptr)
    {
        report->SetCurrentSectionName("meshwheels2");
    }
    auto meshwheels2_end = module->mesh_wheels.end();
    for (auto itor = module->mesh_wheels.begin(); itor != meshwheels2_end; ++itor)
    {
        if (!itor->_is_meshwheel2) { continue; }

        Node* axis_inner = nullptr;
        Node* axis_outer = nullptr;
        Node* rigidity = nullptr;
        Node* reference_arm = nullptr;
        RigDef::MeshWheel & def = *itor;
        bool nodes_found = GetWheelDefinitionNodes(
            def.nodes[0], def.nodes[1], def.rigidity_node, def.reference_arm_node, // Input
            axis_inner, axis_outer, rigidity, reference_arm, // Output
            report);
        if (!nodes_found)
        {
            continue; // Error already logged
        }
        auto meshwheel2 = new MeshWheel2(def, axis_inner, axis_outer, rigidity, reference_arm);
        meshwheel2->SetGeometryIsDirty(true);
        m_wheels.push_back(meshwheel2);
    }
    
    if (report != nullptr)
    {
        report->ClearCurrentSectionName();
    }
}

void Rig::RefreshNodeScreenPosition(Node & node, CameraHandler* camera_handler)
{
    Vector2int screen_pos(-1, -1);
    bool node_visible = camera_handler->ConvertWorldToScreenPosition(node.GetPosition(), screen_pos);
    if (node_visible)
    {
        node.SetScreenPosition(screen_pos);
    }
}

void Rig::RefreshAllNodesScreenPositions(CameraHandler* camera_handler)
{
    for (auto itor = m_nodes.begin(); itor != m_nodes.end(); ++itor)
    {
        RefreshNodeScreenPosition(itor->second, camera_handler);
    }
}

bool Rig::RefreshMouseHoveredNode(Vector2int const & mouse_position)
{
    /*
    Lookup method: for each node
        - Compute mouse offset
        - Convert it's components to abs()
        - Pick the larger component as 'distance'
        - Compare 'distance' to previous node

    Finally, compare result node's 'distance' to configured box.
    */

    auto itor = m_nodes.begin();
    Node* result = m_mouse_hovered_node;
    if (result == nullptr)
    {
        result = &itor->second;
        ++itor;
    }
    Vector2int node_mouse_offset = result->GetScreenPosition() - mouse_position;
    node_mouse_offset.x *= (node_mouse_offset.x < 0) ? -1 : 1; // Absolute value
    node_mouse_offset.y *= (node_mouse_offset.y < 0) ? -1 : 1; // Absolute value
    int result_mouse_distance = (node_mouse_offset.x > node_mouse_offset.y) ? node_mouse_offset.x : node_mouse_offset.y;
    
    auto itor_end = m_nodes.end();

    for ( ; itor != itor_end ; ++itor)
    {
        node_mouse_offset = itor->second.GetScreenPosition() - mouse_position;
        node_mouse_offset.x *= (node_mouse_offset.x < 0) ? -1 : 1; // Absolute value
        node_mouse_offset.y *= (node_mouse_offset.y < 0) ? -1 : 1; // Absolute value
        int current_mouse_distance = (node_mouse_offset.x > node_mouse_offset.y) ? node_mouse_offset.x : node_mouse_offset.y;
        if (current_mouse_distance < result_mouse_distance)
        {
            result_mouse_distance = current_mouse_distance;
            result = &itor->second;
        }
    }

    if (result_mouse_distance > m_config->node_mouse_box_halfsize_px)
    {
        result = nullptr;
    }

    bool ret_val = (m_mouse_hovered_node != result);
    m_mouse_hovered_node = result;
    return ret_val;
}

void Rig::RefreshNodesDynamicMeshes(Ogre::SceneNode* parent_scene_node)
{
    /* ========== Nodes (plain + selected) ========== */

    m_nodes_dynamic_mesh->beginUpdate(0);
    m_nodes_selected_dynamic_mesh->beginUpdate(0);
    for (auto itor = m_nodes.begin(); itor != m_nodes.end(); ++itor)
    {
        // DEBUG (Windows, VC10)
        // Conditional breakpoint: "if the node IDs match"
        //
        // Current node:
        //     Definition:
        //         ((*(((*current_node).m_def_module).px)).nodes)._Myfirst[(*(current_node)).m_def_index]
        //     ID:
        //         ((((*(((*current_node).m_def_module).px)).nodes)._Myfirst[(*(current_node)).m_def_index]).id).m_id_num
        // 
        // Closest node:
        //     Definition:
        //         ((*(((*((*(this)).m_node_closest_to_mouse)).m_def_module).px)).nodes)._Myfirst[(*((*(this)).m_node_closest_to_mouse)).m_def_index]
        //     ID:
        //         ((((*(((*((*(this)).m_node_closest_to_mouse)).m_def_module).px)).nodes)._Myfirst[(*((*(this)).m_node_closest_to_mouse)).m_def_index]).id).m_id_num

        RigEditor::Node* current_node = &itor->second;
        if (current_node != m_mouse_hovered_node)
        {
            if (current_node->IsSelected())
            {
                m_nodes_selected_dynamic_mesh->position(current_node->GetPosition());
                m_nodes_selected_dynamic_mesh->colour(m_config->node_selected_color);
            }
            else
            {
                m_nodes_dynamic_mesh->position(current_node->GetPosition());
                m_nodes_dynamic_mesh->colour(m_config->node_generic_color);
            }
        }
    }
    m_nodes_dynamic_mesh->end();
    m_nodes_selected_dynamic_mesh->end();

    /* ========== Hover nodes ========== */

    if (m_mouse_hovered_node != nullptr)
    {
        m_nodes_hover_dynamic_mesh->beginUpdate(0);
        m_nodes_hover_dynamic_mesh->position(m_mouse_hovered_node->GetPosition());
        m_nodes_hover_dynamic_mesh->colour(m_config->node_hover_color);
        m_nodes_hover_dynamic_mesh->end();
        
        if (! m_nodes_hover_dynamic_mesh->isAttached())
        {
            parent_scene_node->attachObject(m_nodes_hover_dynamic_mesh.get());
        }
    }
    else
    {
        if (m_nodes_hover_dynamic_mesh->isAttached())
        {
            m_nodes_hover_dynamic_mesh->detachFromParent();
        }
    }
}

void Rig::AttachToScene(Ogre::SceneNode* parent_scene_node)
{
    assert(parent_scene_node != nullptr);

    parent_scene_node->attachObject(m_beams_dynamic_mesh.get());
    parent_scene_node->attachObject(m_nodes_dynamic_mesh.get());
    parent_scene_node->attachObject(m_nodes_hover_dynamic_mesh.get());
    parent_scene_node->attachObject(m_nodes_selected_dynamic_mesh.get());

    m_wheel_visuals->AttachToScene(parent_scene_node);
}

void Rig::DetachFromScene()
{
    m_beams_dynamic_mesh->detachFromParent();
    m_nodes_dynamic_mesh->detachFromParent();
    m_nodes_hover_dynamic_mesh->detachFromParent();
    m_nodes_selected_dynamic_mesh->detachFromParent();
}

void Rig::DeselectAllNodes()
{
    for (auto itor = m_nodes.begin(); itor != m_nodes.end(); ++itor)
    {
        itor->second.SetSelected(false);
    }
}

bool Rig::ToggleMouseHoveredNodeSelected()
{
    if (m_mouse_hovered_node == nullptr)
    {
        return false;
    }
    m_mouse_hovered_node->SetSelected(! m_mouse_hovered_node->IsSelected());
    return true;
}

Node& Rig::CreateNewNode(Ogre::Vector3 const & position)
{
    RigDef::Node node_def;
    m_highest_node_id++;
    node_def.id.SetNum(m_highest_node_id);
    node_def.position = position;
    auto result = m_nodes.insert( std::make_pair(node_def.id.Str(), Node(node_def)) );
    if (result.second == true)
    {
        m_aabb.merge(position); // Update bounding box
    }
    return result.first->second;
}

void Rig::ClearMouseHoveredNode()
{
    m_mouse_hovered_node = nullptr;
}

void Rig::RefreshWheelsDynamicMesh(Ogre::SceneNode* parent_scene_node, RigEditor::Main* rig_editor)
{
    m_wheel_visuals->RefreshWheelsDynamicMeshes(parent_scene_node, rig_editor, m_wheels);
}

void Rig::TranslateSelectedNodes(Ogre::Vector3 const & offset, CameraHandler* camera_handler)
{
    for (auto itor = m_nodes.begin(); itor != m_nodes.end(); ++itor)
    {
        Node & node = itor->second;
        if (node.IsSelected())
        {
            node.Translate(offset);
            RefreshNodeScreenPosition(node, camera_handler);
        }
    }
}

void Rig::RefreshBeamsDynamicMesh()
{
    m_beams_dynamic_mesh->beginUpdate(0);

    for (auto itor = m_beams.begin(); itor != m_beams.end(); ++itor)
    {
        Beam & beam = *itor;
        m_beams_dynamic_mesh->position(beam.GetNodeA()->GetPosition());
        m_beams_dynamic_mesh->colour(beam.GetColor());
        m_beams_dynamic_mesh->position(beam.GetNodeB()->GetPosition());
        m_beams_dynamic_mesh->colour(beam.GetColor());
    }

    m_beams_dynamic_mesh->end();
}

void Rig::DeselectOrSelectAllNodes()
{
    int num_deselected = 0;
    for (auto itor = m_nodes.begin(); itor != m_nodes.end(); ++itor)
    {
        if (itor->second.IsSelected())
        {
            itor->second.SetSelected(false);
            num_deselected++;
        }
    }

    if (num_deselected == 0)
    {
        for (auto itor = m_nodes.begin(); itor != m_nodes.end(); ++itor)
        {
            itor->second.SetSelected(true);
        }
    }
}

bool Rig::DeleteBeamBetweenNodes(Node* n1, Node* n2)
{
    // Search & destroy!
    for (auto itor1 = n1->m_linked_beams.begin(); itor1 != n1->m_linked_beams.end(); ++itor1)
    {
        Beam* beam1 = *itor1;
        for (auto itor2 = n2->m_linked_beams.begin(); itor2 != n2->m_linked_beams.end(); ++itor2)
        {
            Beam* beam2 = *itor2;
            if (beam1 == beam2)
            {
                // NOTE: Invalidated iterators don't matter, we're returning right after.
                n1->m_linked_beams.erase(itor1);
                n2->m_linked_beams.erase(itor2);
                DeleteBeam(beam1);
                return true; // Beam found and erased
            }
        }
    }
    return false; // Beam not found
}

bool Rig::DeleteBeam(RigEditor::Beam* beam_to_delete)
{
    for (auto itor = m_beams.begin(); itor != m_beams.end(); ++itor)
    {
        Beam* current_beam = &*itor;
        if (beam_to_delete == current_beam)
        {
            m_beams.erase(itor); // Invalidated iterator doesn't matter, we're returning.
            return true; // Found and erased
        }
    }
    return false; // Not found
}

void Rig::DeleteBeamsBetweenSelectedNodes()
{
    std::set<Beam*> beam_set;
    for (auto node_itor = m_nodes.begin(); node_itor != m_nodes.end(); ++node_itor)
    {
        Node* node = &node_itor->second;
        if (node->IsSelected())
        {
            auto beam_itor     = node->m_linked_beams.begin();
            auto beam_itor_end = node->m_linked_beams.end();
            while (beam_itor != beam_itor_end)
            {
                Beam* beam = *beam_itor;
                auto result_itor = beam_set.insert(beam);
                if (result_itor.second == false)
                {
                    // Already in set => is between 2 selected nodes => delete.

                    // Unlink from opposite node
                    Node* opposite_node = (beam->GetNodeA() == node) ? beam->GetNodeB() : beam->GetNodeA();
                    opposite_node->UnlinkBeam(beam);

                    // Unlink from current node
                    beam_itor = node->m_linked_beams.erase(beam_itor); // Erase + advance iterator

                    DeleteBeam(beam); // Delete from rig
                    beam_set.erase(result_itor.first); // Delete from set
                }
                else
                {
                    ++beam_itor; // Advance iterator
                }
            }
        }
    }
}

int Rig::DeleteAttachedBeams(Node* node)
{
    int num_deleted = node->m_linked_beams.size();
    while (node->m_linked_beams.size() > 0)
    {
        Beam* beam = node->m_linked_beams.back();

        // Unlink from opposite node
        Node* opposite_node = (beam->GetNodeA() == node) ? beam->GetNodeB() : beam->GetNodeA();
        opposite_node->UnlinkBeam(beam);

        // Unlink from current node
        node->m_linked_beams.pop_back();

        DeleteBeam(beam);
    }
    return num_deleted;
}

void Rig::DeleteSelectedNodes()
{
    auto itor     = m_nodes.begin();
    auto itor_end = m_nodes.end();
    while (itor != itor_end)
    {
        Node* node = &itor->second;
        if (node->IsSelected())
        {
            DeleteAttachedBeams(node);
            if (m_mouse_hovered_node == node)
            {
                m_mouse_hovered_node = nullptr;
            }
            itor = m_nodes.erase(itor); // Erase the node, advance iterator
        }
        else
        {
            ++itor; // Just advance iterator
        }
    }
}

bool Rig::DeleteNode(Node* node_to_delete)
{
    // Search & destroy!
    for (auto itor = m_nodes.begin(); itor != m_nodes.end(); ++itor)
    {
        Node* node = &itor->second;
        if (node->IsSelected())
        {
            DeleteAttachedBeams(node);
            if (m_mouse_hovered_node == node)
            {
                m_mouse_hovered_node = nullptr;
            }
            itor = m_nodes.erase(itor); // Erase the node, advance iterator
        }
        else
        {
            ++itor; // Just advance iterator
        }
    }
    return false; // Node not found
}

void Rig::ExtrudeSelectedNodes()
{
    // Iterate through existing nodes (don't include newly generated nodes)
    auto node_itor = m_nodes.begin();
    int num_nodes = m_nodes.size();
    for (int i = 0; i < num_nodes; ++i)
    {
        Node* node = &node_itor->second;
        if (node->IsSelected())
        {
            node->SetSelected(false);
            Node & new_node = CreateNewNode(node->GetPosition());
            new_node.SetSelected(true);
            CreateNewBeam(node, &new_node);
        }

        ++node_itor;
    }
}

RigEditor::Beam & Rig::CreateNewBeam(Node* n1, Node* n2)
{
    RigDef::Beam* beam_def = new RigDef::Beam();
    BITMASK_SET_1(beam_def->options, RigDef::Beam::OPTION_i_INVISIBLE);
    RigEditor::Beam beam(static_cast<void*>(beam_def), Beam::TYPE_PLAIN, n1, n2);
    beam.SetColor(m_config->beam_invisible_color);
    m_beams.push_back(beam);
    Beam & beam_ref = m_beams.back();
    n1->m_linked_beams.push_back(&beam_ref);
    n2->m_linked_beams.push_back(&beam_ref);
    return beam_ref;
}

void Rig::SelectedNodesCommitPositionUpdates()
{
    auto end_itor = m_nodes.end();
    for (auto itor = m_nodes.begin(); itor != end_itor; ++itor)
    {
        Node & node = itor->second;
        if (node.IsSelected())
        {
            node.SetDefinitionPosition(node.GetPosition());
        }
    }
}

void Rig::SelectedNodesCancelPositionUpdates()
{
    auto end_itor = m_nodes.end();
    for (auto itor = m_nodes.begin(); itor != end_itor; ++itor)
    {
        Node & node = itor->second;
        if (node.IsSelected())
        {
            node.SetPosition(node.GetDefinitionPosition());
        }
    }
}

RigProperties* Rig::GetProperties()
{
    return m_properties.get();
}

std::shared_ptr<RigDef::File> Rig::Export()
{
    using namespace RigDef;

    // Allocate
    auto def = std::shared_ptr<File>(new RigDef::File());
    auto module = std::shared_ptr<File::Module>(new File::Module("_Root_"));
    def->root_module = module;

    // Fill node data
    for(auto itor = m_nodes.begin(); itor != m_nodes.end(); ++itor)
    {
        module->nodes.push_back(itor->second.m_definition); // Copy definition
    }

    // Fill beam data
    for (auto itor = m_beams.begin(); itor != m_beams.end(); ++itor)
    {
        void* def = itor->m_source;
        Beam::Type type = itor->GetType();
        switch (type)
        {
        case Beam::TYPE_PLAIN: 
            module->beams.push_back(* static_cast<RigDef::Beam*>(def)); // Copy definition
            break;
        case Beam::TYPE_COMMAND_HYDRO:
            module->commands_2.push_back(* static_cast<RigDef::Command2*>(def)); // Copy definition
            break;
        case Beam::TYPE_SHOCK_ABSORBER:
            module->shocks.push_back(* static_cast<RigDef::Shock*>(def)); // Copy definition
            break;
        case Beam::TYPE_SHOCK_ABSORBER_2:
            module->shocks_2.push_back(* static_cast<RigDef::Shock2*>(def)); // Copy definition
            break;
        case Beam::TYPE_STEERING_HYDRO:
            module->hydros.push_back(* static_cast<RigDef::Hydro*>(def)); // Copy definition
            break;
        case Beam::TYPE_CINECAM:
            break; // Generated beam; do nothing
        default:
            // This really shouldn't happen
            throw std::runtime_error("INTERNAL ERROR: Rig::Export(): Unknown Beam::Type encountered");
        }
        
    }

    // Export 'properties'
    m_properties->Export(def);

    auto output_module = def->root_module;

    // EXPORT WHEELS
    auto wheels_end = m_wheels.end();
    for (auto itor = m_wheels.begin(); itor != wheels_end; ++itor)
    {
        LandVehicleWheel* wheel = *itor;
        switch (wheel->GetType())
        {
        case LandVehicleWheel::TYPE_MESHWHEEL_2:
            {
                RigEditor::MeshWheel2* mw2 = static_cast<RigEditor::MeshWheel2*>(wheel);
                output_module->mesh_wheels.push_back(mw2->GetDefinition()); // Copy definition
                break;
            }
        
        case LandVehicleWheel::TYPE_MESHWHEEL:
            {
                RigEditor::MeshWheel* mw = static_cast<RigEditor::MeshWheel*>(wheel);
                output_module->mesh_wheels.push_back(mw->GetDefinition()); // Copy definition
                break;
            }
        
        case LandVehicleWheel::TYPE_FLEXBODYWHEEL:
            {
                RigEditor::FlexBodyWheel* fbw = static_cast<RigEditor::FlexBodyWheel*>(wheel);
                output_module->flex_body_wheels.push_back(fbw->GetDefinition()); // Copy definition
                break;
            }

        default:
            // This really shouldn't happen
            throw std::runtime_error("INTERNAL ERROR: Rig::Export(): Unknown LandVehicleWheel::Type encountered");
            break;
        }
        
    }

    // Return
    return def;
}

void Rig::QuerySelectedNodesData(RigAggregateNodesData* out_result)
{
    RigAggregateNodesData result; // Local, for performance
    auto itor_end = m_nodes.end();
    for (auto itor = m_nodes.begin(); itor != itor_end; ++itor)
    {
        RigEditor::Node & node = itor->second;
        if (node.IsSelected())
        {
            ++result.num_selected;
            if (result.num_selected == 1)
            {
                result.node_name         = node.GetId().ToString();
                result.detacher_group_id = node.GetDefinitionDetacherGroup();
                result.load_weight       = node.GetDefinitionLoadWeight();
                
                unsigned int node_flags = node.GetDefinitionFlags();
                result.SetFlag_m( BITMASK_IS_1( node_flags, RigDef::Node::OPTION_m_NO_MOUSE_GRAB     ) );
                result.SetFlag_f( BITMASK_IS_1( node_flags, RigDef::Node::OPTION_f_NO_SPARKS         ) );
                result.SetFlag_x( BITMASK_IS_1( node_flags, RigDef::Node::OPTION_x_EXHAUST_POINT     ) );
                result.SetFlag_y( BITMASK_IS_1( node_flags, RigDef::Node::OPTION_y_EXHAUST_DIRECTION ) );
                result.SetFlag_c( BITMASK_IS_1( node_flags, RigDef::Node::OPTION_c_NO_GROUND_CONTACT ) );
                result.SetFlag_h( BITMASK_IS_1( node_flags, RigDef::Node::OPTION_h_HOOK_POINT        ) );
                result.SetFlag_b( BITMASK_IS_1( node_flags, RigDef::Node::OPTION_b_EXTRA_BUOYANCY    ) );
                result.SetFlag_p( BITMASK_IS_1( node_flags, RigDef::Node::OPTION_p_NO_PARTICLES      ) );
                result.SetFlag_L( BITMASK_IS_1( node_flags, RigDef::Node::OPTION_L_LOG               ) );
                result.SetFlag_l( BITMASK_IS_1( node_flags, RigDef::Node::OPTION_l_LOAD_WEIGHT       ) );
                
                // TODO: Implement presets
            }
            else
            {
                if (result.num_selected == 2)
                {
                    result.node_name = "";
                }

                if (result.IsDetacherGroupUniform()) { result.SetDetacherGroupIsUniform(node.GetDefinitionDetacherGroup() == result.detacher_group_id); }
                if (result.IsLoadWeightUniform())    { result.SetLoadWeightIsUniform   (node.GetDefinitionLoadWeight()    == result.load_weight      ); }

                unsigned int node_flags = node.GetDefinitionFlags();
                result.SetFlagIsUniform_m( result.IsFlagUniform_m() && (result.HasFlag_m() == BITMASK_IS_1( node_flags, RigDef::Node::OPTION_m_NO_MOUSE_GRAB     )));
                result.SetFlagIsUniform_f( result.IsFlagUniform_f() && (result.HasFlag_f() == BITMASK_IS_1( node_flags, RigDef::Node::OPTION_f_NO_SPARKS         )));
                result.SetFlagIsUniform_x( result.IsFlagUniform_x() && (result.HasFlag_x() == BITMASK_IS_1( node_flags, RigDef::Node::OPTION_x_EXHAUST_POINT     )));
                result.SetFlagIsUniform_y( result.IsFlagUniform_y() && (result.HasFlag_y() == BITMASK_IS_1( node_flags, RigDef::Node::OPTION_y_EXHAUST_DIRECTION )));
                result.SetFlagIsUniform_c( result.IsFlagUniform_c() && (result.HasFlag_c() == BITMASK_IS_1( node_flags, RigDef::Node::OPTION_c_NO_GROUND_CONTACT )));
                result.SetFlagIsUniform_h( result.IsFlagUniform_h() && (result.HasFlag_h() == BITMASK_IS_1( node_flags, RigDef::Node::OPTION_h_HOOK_POINT        )));
                result.SetFlagIsUniform_b( result.IsFlagUniform_b() && (result.HasFlag_b() == BITMASK_IS_1( node_flags, RigDef::Node::OPTION_b_EXTRA_BUOYANCY    )));
                result.SetFlagIsUniform_p( result.IsFlagUniform_p() && (result.HasFlag_p() == BITMASK_IS_1( node_flags, RigDef::Node::OPTION_p_NO_PARTICLES      )));
                result.SetFlagIsUniform_L( result.IsFlagUniform_L() && (result.HasFlag_L() == BITMASK_IS_1( node_flags, RigDef::Node::OPTION_L_LOG               )));
                result.SetFlagIsUniform_l( result.IsFlagUniform_l() && (result.HasFlag_l() == BITMASK_IS_1( node_flags, RigDef::Node::OPTION_l_LOAD_WEIGHT       )));
                
                // TODO: Implement presets
            }
        }
    }
    *out_result = result;
}

void Rig::QuerySelectedBeamsData(RigAggregateBeams2Data* out_result)
{
    UpdateSelectedBeamsList();
    RigAggregateBeams2Data result; // Local, for performance
    auto itor_end = m_selected_beams.end();
    for (auto itor = m_selected_beams.begin(); itor != itor_end; ++itor)
    {
        Beam* beam = *itor;
        if (beam->GetType() == Beam::TYPE_PLAIN)
        {
            auto & res = result.plain_beams;
            auto * def = beam->GetDefinitionPlainBeam();
            assert(def != nullptr);

            if (res.num_selected == 0)
            {
                res.SetFlag_i(def->HasFlag_i_Invisible());
                res.SetFlag_r(def->HasFlag_r_Rope());
                res.SetFlag_s(def->HasFlag_s_Support());
                res.detacher_group = def->detacher_group;
                res.extension_break_limit = def->extension_break_limit;
            }
            else
            {
                // Flags
                res.SetFlagUniform_i( res.IsFlagUniform_i() && (res.HasFlag_i() == def->HasFlag_i_Invisible()));				
                res.SetFlagUniform_r( res.IsFlagUniform_r() && (res.HasFlag_r() == def->HasFlag_r_Rope()));				
                res.SetFlagUniform_s( res.IsFlagUniform_s() && (res.HasFlag_s() == def->HasFlag_s_Support()));

                // Numbers
                res.SetDetacherGroupIsUniform(      res.IsDetacherGroupUniform()       && (res.detacher_group        == def->detacher_group));											   
                res.SetExtensionBreakLimitIsUniform(res.IsExtensionBreakLimitUniform() && (res.extension_break_limit == def->extension_break_limit));
            }
            ++res.num_selected;
        }
        else if (beam->GetType() == Beam::TYPE_STEERING_HYDRO)
        {
            auto & res = result.hydros;
            auto * def = beam->GetDefinitionHydro();
            assert(def != nullptr);

            if (res.num_selected == 0)
            {
                // Flags (1)                        // Flags (2)                        // Flags (3)
                res.SetFlag_a(def->HasFlag_a());    res.SetFlag_e(def->HasFlag_e());    res.SetFlag_g(def->HasFlag_g());
                res.SetFlag_h(def->HasFlag_h());    res.SetFlag_i(def->HasFlag_i());    res.SetFlag_r(def->HasFlag_r());
                res.SetFlag_s(def->HasFlag_s());    res.SetFlag_u(def->HasFlag_u());    res.SetFlag_v(def->HasFlag_v());
                res.SetFlag_x(def->HasFlag_x());    res.SetFlag_y(def->HasFlag_y());

                res.detacher_group          = def->detacher_group;
                res.extension_factor        = def->lenghtening_factor;
                res.inertia_start_delay     = def->inertia.start_delay_factor;
                res.inertia_stop_delay      = def->inertia.stop_delay_factor;
                res.inertia_start_function  = def->inertia.start_function;
                res.inertia_stop_function   = def->inertia.stop_function;
            }
            else
            {
                // Update uniformity
                res.SetFlagIsUniform_a( res.IsFlagUniform_a() && (res.HasFlag_a() == def->HasFlag_a()) );
                res.SetFlagIsUniform_e( res.IsFlagUniform_e() && (res.HasFlag_e() == def->HasFlag_e()) );
                res.SetFlagIsUniform_g( res.IsFlagUniform_g() && (res.HasFlag_g() == def->HasFlag_g()) );
                res.SetFlagIsUniform_h( res.IsFlagUniform_h() && (res.HasFlag_h() == def->HasFlag_h()) );
                res.SetFlagIsUniform_i( res.IsFlagUniform_i() && (res.HasFlag_i() == def->HasFlag_i()) );
                res.SetFlagIsUniform_r( res.IsFlagUniform_r() && (res.HasFlag_r() == def->HasFlag_r()) );
                res.SetFlagIsUniform_s( res.IsFlagUniform_s() && (res.HasFlag_s() == def->HasFlag_s()) );
                res.SetFlagIsUniform_u( res.IsFlagUniform_u() && (res.HasFlag_u() == def->HasFlag_u()) );
                res.SetFlagIsUniform_v( res.IsFlagUniform_v() && (res.HasFlag_v() == def->HasFlag_v()) );
                res.SetFlagIsUniform_x( res.IsFlagUniform_x() && (res.HasFlag_x() == def->HasFlag_x()) );
                res.SetFlagIsUniform_y( res.IsFlagUniform_y() && (res.HasFlag_y() == def->HasFlag_y()) );
                
                res.SetDetacherGroupIsUniform(        res.IsDetacherGroupUniform()        && (res.detacher_group         == def->detacher_group));
                res.SetExtensionFactorIsUniform(      res.IsExtensionFactorUniform()      && (res.extension_factor       == def->lenghtening_factor));
                res.SetInertiaStartDelayIsUniform(    res.IsInertiaStartDelayUniform()    && res.inertia_start_delay     == def->inertia.start_delay_factor);
                res.SetInertiaStopDelayIsUniform(     res.IsInertiaStopDelayUniform()     && (res.inertia_stop_delay     == def->inertia.stop_delay_factor));
                res.SetInertiaStartFunctionIsUniform( res.IsInertiaStartFunctionUniform() && (res.inertia_start_function == def->inertia.start_function));
                res.SetInertiaStopFunctionIsUniform(  res.IsInertiaStopFunctionUniform()  && (res.inertia_stop_function  == def->inertia.stop_function));
            }
            ++res.num_selected;
        }
        else if (beam->GetType() == Beam::TYPE_COMMAND_HYDRO)
        {
            auto & res = result.commands2;
            auto * def = beam->GetDefinitionCommand2();
            assert(def != nullptr);

            if (res.num_selected == 0)
            {
                res.SetFlag_c(def->option_c_auto_center   );
                res.SetFlag_f(def->option_f_not_faster    );
                res.SetFlag_i(def->option_i_invisible     );
                res.SetFlag_o(def->option_o_1press_center );
                res.SetFlag_p(def->option_p_1press        );
                res.SetFlag_r(def->option_r_rope          );

                res.detacher_group          = def->detacher_group;
                res.contraction_rate        = def->shorten_rate;
                res.extension_rate          = def->lengthen_rate;
                res.contract_key            = def->contract_key;
                res.extend_key              = def->extend_key;
                res.description             = def->description;
                res.max_contraction         = def->max_contraction;
                res.max_extension           = def->max_extension;
                res.inertia_start_delay     = def->inertia.start_delay_factor;
                res.inertia_stop_delay      = def->inertia.stop_delay_factor;
                res.inertia_start_function  = def->inertia.start_function;
                res.inertia_stop_function   = def->inertia.stop_function;
                res.affect_engine           = def->affect_engine;
                res.SetBoolNeedsEngine(def->needs_engine);
            }
            else
            {
                if (res.IsFlagUniform_c()) { res.SetFlagIsUniform_c(res.HasFlag_c() == def->option_c_auto_center   ); }
                if (res.IsFlagUniform_f()) { res.SetFlagIsUniform_f(res.HasFlag_f() == def->option_f_not_faster    ); }
                if (res.IsFlagUniform_i()) { res.SetFlagIsUniform_i(res.HasFlag_i() == def->option_i_invisible     ); }
                if (res.IsFlagUniform_o()) { res.SetFlagIsUniform_o(res.HasFlag_o() == def->option_o_1press_center ); }
                if (res.IsFlagUniform_p()) { res.SetFlagIsUniform_p(res.HasFlag_p() == def->option_p_1press        ); }
                if (res.IsFlagUniform_r()) { res.SetFlagIsUniform_r(res.HasFlag_r() == def->option_r_rope          ); }

                if (res.IsDetacherGroupUniform())        { res.SetDetacherGroupIsUniform(       res.detacher_group         == def->detacher_group); }
                if (res.IsContractionRateUniform())      { res.SetContractionRateIsUniform(     res.contraction_rate       == def->shorten_rate); }
                if (res.IsExtensionRateUniform())        { res.SetExtensionRateIsUniform(       res.extension_rate         == def->lengthen_rate); }
                if (res.IsContractKeyUniform())          { res.SetContractKeyIsUniform(         res.contract_key           == def->contract_key); }
                if (res.IsExtendKeyUniform())            { res.SetExtendKeyIsUniform(           res.extend_key             == def->extend_key); }
                if (res.IsDescriptionUniform())          { res.SetDescriptionIsUniform(         res.description            == def->description); }
                if (res.IsContractionLimitUniform())     { res.SetContractionLimitIsUniform(    res.max_contraction        == def->max_contraction); }
                if (res.IsExtensionLimitUniform())       { res.SetExtensionLimitIsUniform(      res.max_extension          == def->max_extension); }
                if (res.IsInertiaStartDelayUniform())    { res.SetInertiaStartDelayIsUniform(   res.inertia_start_delay    == def->inertia.start_delay_factor); }
                if (res.IsInertiaStopDelayUniform())     { res.SetInertiaStopDelayIsUniform(    res.inertia_stop_delay     == def->inertia.stop_delay_factor); }
                if (res.IsInertiaStartFunctionUniform()) { res.SetInertiaStartFunctionIsUniform(res.inertia_start_function == def->inertia.start_function); }
                if (res.IsInertiaStopFunctionUniform())  { res.SetInertiaStopFunctionIsUniform( res.inertia_stop_function  == def->inertia.stop_function); }
                if (res.IsAffectEngineUniform())         { res.SetAffectEngineIsUniform(        res.affect_engine          == def->affect_engine); }
                if (res.IsNeedsEngineUniform())          { res.SetNeedsEngineIsUniform(         res.GetBoolNeedsEngine()   == def->needs_engine); }
            }
            ++res.num_selected;
        }
        else if (beam->GetType() == Beam::TYPE_SHOCK_ABSORBER)
        {
            auto & res = result.shocks;
            auto * def = beam->GetDefinitionShock();
            assert(def != nullptr);

            if (res.num_selected == 0)
            {
                res.spring_rate       = def->spring_rate;
                res.damping           = def->damping;
                res.contraction_limit = def->short_bound;
                res.extension_limit   = def->long_bound;
                res.precompression    = def->precompression;
                res.detacher_group    = def->detacher_group;

                res.SetFlag_i(def->HasOption_i_Invisible());
                res.SetFlag_L(def->HasOption_L_ActiveLeft());
                res.SetFlag_R(def->HasOption_R_ActiveRight());
                res.SetFlag_m(def->HasOption_m_Metric());
            }
            else
            {
                if (res.IsSpringUniform())           { res.SetSpringIsUniform(          res.spring_rate       == def->spring_rate); }
                if (res.IsDampingUniform())          { res.SetDampingIsUniform(         res.damping           == def->damping); }
                if (res.IsContractionLimitUniform()) { res.SetContractionLimitIsUniform(res.contraction_limit == def->short_bound); }
                if (res.IsExtensionLimitUniform())   { res.SetExtensionLimitIsUniform(  res.extension_limit   == def->long_bound); }
                if (res.IsPrecompressionUniform())   { res.SetPrecompressionIsUniform(  res.precompression    == def->precompression); }
                if (res.IsDetacherGroupUniform())    { res.SetDetacherGroupIsUniform(   res.detacher_group    == def->detacher_group); }

                if (res.IsFlagUniform_i()) { res.SetFlagIsUniform_i(res.HasFlag_i() == def->HasOption_i_Invisible()); }
                if (res.IsFlagUniform_L()) { res.SetFlagIsUniform_L(res.HasFlag_L() == def->HasOption_L_ActiveLeft()); }
                if (res.IsFlagUniform_R()) { res.SetFlagIsUniform_R(res.HasFlag_R() == def->HasOption_R_ActiveRight()); }
                if (res.IsFlagUniform_m()) { res.SetFlagIsUniform_m(res.HasFlag_m() == def->HasOption_m_Metric()); }
            }
            ++res.num_selected;
        }
        else if (beam->GetType() == Beam::TYPE_SHOCK_ABSORBER_2)
        {
            auto & res = result.shocks2;
            auto * def = beam->GetDefinitionShock2();
            assert(def != nullptr);

            if (res.num_selected == 0)
            {
                res.spring_in_rate      = def->spring_in;
                res.spring_out_rate     = def->spring_out;
                res.spring_in_progress  = def->progress_factor_spring_in;
                res.spring_out_progress = def->progress_factor_spring_out;
                res.damp_in_rate        = def->damp_in;
                res.damp_out_rate       = def->damp_out;
                res.damp_in_progress    = def->progress_factor_damp_in;
                res.damp_out_progress   = def->progress_factor_damp_out;
                res.contraction_limit   = def->short_bound;
                res.extension_limit     = def->long_bound;
                res.precompression      = def->precompression;
                res.detacher_group      = def->detacher_group;

                res.SetFlag_i(def->HasOption_i_Invisible());
                res.SetFlag_M(def->HasOption_M_AbsoluteMetric());
                res.SetFlag_s(def->HasOption_s_SoftBumpBounds());
                res.SetFlag_m(def->HasOption_m_Metric());
            }
            else
            {
                if (res.IsSpringInRateUniform())     { res.SetSpringInRateIsUniform(    res.spring_in_rate      == def->spring_in); }
                if (res.IsSpringOutRateUniform())    { res.SetSpringOutRateIsUniform(   res.spring_out_rate     == def->spring_out); }
                if (res.IsSpringInProgressUniform()) { res.SetSpringInProgressIsUniform(res.spring_in_progress  == def->progress_factor_spring_in); }
                if (res.IsDampInProgressUniform())   { res.SetDampInProgressIsUniform(  res.spring_out_progress == def->progress_factor_spring_out); }
                if (res.IsDampInRateUniform())       { res.SetDampInRateIsUniform(      res.damp_in_rate        == def->damp_in); }
                if (res.IsDampOutRateUniform())      { res.SetDampOutRateIsUniform(     res.damp_out_rate       == def->damp_out); }
                if (res.IsDampInProgressUniform())   { res.SetDampInProgressIsUniform(  res.damp_in_progress    == def->progress_factor_damp_in); }
                if (res.IsDampOutProgressUniform())  { res.SetDampOutProgressIsUniform( res.damp_out_progress   == def->progress_factor_damp_out); }
                if (res.IsContractionLimitUniform()) { res.SetContractionLimitIsUniform(res.contraction_limit   == def->short_bound); }
                if (res.IsExtensionLimitUniform())   { res.SetExtensionLimitIsUniform(  res.extension_limit     == def->long_bound); }
                if (res.IsPrecompressionUniform())   { res.SetPrecompressionIsUniform(  res.precompression      == def->precompression); }
                if (res.IsDetacherGroupUniform())    { res.SetDetacherGroupIsUniform(   res.detacher_group      == def->detacher_group); }

                if (res.IsFlagUniform_i()) { res.SetFlagIsUniform_i(res.HasFlag_i() == def->HasOption_i_Invisible()); }
                if (res.IsFlagUniform_M()) { res.SetFlagIsUniform_M(res.HasFlag_M() == def->HasOption_M_AbsoluteMetric()); }
                if (res.IsFlagUniform_s()) { res.SetFlagIsUniform_s(res.HasFlag_s() == def->HasOption_s_SoftBumpBounds()); }
                if (res.IsFlagUniform_m()) { res.SetFlagIsUniform_m(res.HasFlag_m() == def->HasOption_m_Metric()); }
            }
            ++res.num_selected;
        }
    }
    *out_result = result;
}

void Rig::UpdateSelectedBeamsList()
{
    // Selected beams are those between 2 selected nodes

    m_selected_beams.clear();
    std::set<Beam*> beam_set;
    for (auto node_itor = m_nodes.begin(); node_itor != m_nodes.end(); ++node_itor)
    {
        Node* node = &node_itor->second;
        if (node->IsSelected())
        {
            auto beam_itor     = node->m_linked_beams.begin();
            auto beam_itor_end = node->m_linked_beams.end();
            while (beam_itor != beam_itor_end)
            {
                Beam* beam = *beam_itor;
                auto result_itor = beam_set.insert(beam);
                if (result_itor.second == false)
                {
                    // Already in set => is between 2 selected nodes
                    m_selected_beams.push_back(*result_itor.first);
                }
                ++beam_itor;
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Node/beam updaters
// ----------------------------------------------------------------------------

void Rig::SelectedNodesUpdateAttributes(const RigAggregateNodesData* data)
{
    auto itor_end = m_nodes.end();
    for (auto itor = m_nodes.begin(); itor != itor_end; ++itor)
    {
        RigEditor::Node& node = itor->second;
        if (node.IsSelected())
        {
            if (data->IsDetacherGroupUniform()) { node.SetDefinitionDetacherGroup(data->detacher_group_id); }
            if (data->IsLoadWeightUniform())    { node.SetDefinitionLoadWeight(data->load_weight); }

            unsigned int node_flags = node.GetDefinitionFlags();
            if (data->IsFlagUniform_m()) { Bitmask_SetBool( data->HasFlag_m(), node_flags, RigDef::Node::OPTION_m_NO_MOUSE_GRAB     ); }
            if (data->IsFlagUniform_f()) { Bitmask_SetBool( data->HasFlag_f(), node_flags, RigDef::Node::OPTION_f_NO_SPARKS         ); }
            if (data->IsFlagUniform_x()) { Bitmask_SetBool( data->HasFlag_x(), node_flags, RigDef::Node::OPTION_x_EXHAUST_POINT     ); }
            if (data->IsFlagUniform_y()) { Bitmask_SetBool( data->HasFlag_y(), node_flags, RigDef::Node::OPTION_y_EXHAUST_DIRECTION ); }
            if (data->IsFlagUniform_c()) { Bitmask_SetBool( data->HasFlag_c(), node_flags, RigDef::Node::OPTION_c_NO_GROUND_CONTACT ); }
            if (data->IsFlagUniform_h()) { Bitmask_SetBool( data->HasFlag_h(), node_flags, RigDef::Node::OPTION_h_HOOK_POINT        ); }
            if (data->IsFlagUniform_b()) { Bitmask_SetBool( data->HasFlag_b(), node_flags, RigDef::Node::OPTION_b_EXTRA_BUOYANCY    ); }
            if (data->IsFlagUniform_p()) { Bitmask_SetBool( data->HasFlag_p(), node_flags, RigDef::Node::OPTION_p_NO_PARTICLES      ); }
            if (data->IsFlagUniform_L()) { Bitmask_SetBool( data->HasFlag_L(), node_flags, RigDef::Node::OPTION_L_LOG               ); }
            if (data->IsFlagUniform_l()) { Bitmask_SetBool( data->HasFlag_l(), node_flags, RigDef::Node::OPTION_l_LOAD_WEIGHT       ); }
            node.SetDefinitionFlags(node_flags);
        }
    }
}

void Rig::SelectedPlainBeamsUpdateAttributes(const RigAggregatePlainBeamsData* data)
{
    auto itor_end = m_selected_beams.end();
    for (auto itor = m_selected_beams.begin(); itor != itor_end; ++itor)
    {
        Beam* beam = *itor;
        if (beam->GetType() == Beam::TYPE_PLAIN)
        {
            RigDef::Beam* def = beam->GetDefinitionPlainBeam();
            
            if (data->IsFlagUniform_i()) { def->SetFlag_i_Invisible( data->HasFlag_i()); }
            if (data->IsFlagUniform_s()) { def->SetFlag_s_Support(   data->HasFlag_s()); }
            if (data->IsFlagUniform_r()) { def->SetFlag_r_Rope(      data->HasFlag_r()); }
            
            if (data->IsDetacherGroupUniform())       { def->detacher_group        = data->detacher_group; }
            if (data->IsExtensionBreakLimitUniform()) { def->extension_break_limit = data->extension_break_limit; }
        }
    }
}

void Rig::SelectedMixedBeamsUpdateAttributes(const MixedBeamsAggregateData *data)
{
    auto itor_end = m_selected_beams.end();
    for (auto itor = m_selected_beams.begin(); itor != itor_end; ++itor)
    {
        Beam* beam = *itor;
        if (beam->GetType() == Beam::TYPE_PLAIN)
        {
            RigDef::Beam* def = beam->GetDefinitionPlainBeam();
            
            if (data->IsDetacherGroupUniform()) { def->detacher_group = data->detacher_group; }
        }
        else if (beam->GetType() == Beam::TYPE_SHOCK_ABSORBER_2)
        {
            RigDef::Shock2* shock = beam->GetDefinitionShock2();
                                                    
            if (data->IsDetacherGroupUniform()) { shock->detacher_group = data->detacher_group; }
        }
        else if (beam->GetType() == Beam::TYPE_STEERING_HYDRO)
        {
            RigDef::Hydro* def = beam->GetDefinitionHydro();

            if (data->IsDetacherGroupUniform()) { def->detacher_group = data->detacher_group; }
        }
        else if (beam->GetType() == Beam::TYPE_COMMAND_HYDRO)
        {
            RigDef::Command2* def = beam->GetDefinitionCommand2();
            
            if (data->IsDetacherGroupUniform()) { def->detacher_group = data->detacher_group; } 
        }
        else if (beam->GetType() == Beam::TYPE_SHOCK_ABSORBER)
        {
            RigDef::Shock* shock = beam->GetDefinitionShock();
            
            if (data->IsDetacherGroupUniform()) { shock->detacher_group = data->detacher_group; }
        }
    }
}

void Rig::SelectedShocks2UpdateAttributes(const RigAggregateShocks2Data* data)
{
    auto itor_end = m_selected_beams.end();
    for (auto itor = m_selected_beams.begin(); itor != itor_end; ++itor)
    {
        Beam* beam = *itor;
        if (beam->GetType() == Beam::TYPE_SHOCK_ABSORBER_2)
        {
            RigDef::Shock2* shock = beam->GetDefinitionShock2();
            
            if (data->IsFlagUniform_i()) { shock->SetOption_i_Invisible     ( data->HasFlag_i()); }
            if (data->IsFlagUniform_M()) { shock->SetOption_M_AbsoluteMetric( data->HasFlag_M()); }
            if (data->IsFlagUniform_s()) { shock->SetOption_s_SoftBumpBounds( data->HasFlag_s()); }
            if (data->IsFlagUniform_m()) { shock->SetOption_m_Metric        ( data->HasFlag_m()); }

            if (data->IsSpringInRateUniform())      { shock->spring_in    = data->spring_in_rate; }
            if (data->IsSpringOutRateUniform())     { shock->spring_out   = data->spring_out_rate; }
            if (data->IsDampInRateUniform())        { shock->damp_in      = data->damp_in_rate; }
            if (data->IsDampOutRateUniform())       { shock->damp_out     = data->damp_out_rate; }
            if (data->IsSpringInProgressUniform())  { shock->progress_factor_spring_in  = data->spring_in_progress; }
            if (data->IsSpringOutProgressUniform()) { shock->progress_factor_spring_out = data->spring_out_progress; }
            if (data->IsDampInProgressUniform())    { shock->progress_factor_damp_in    = data->damp_in_progress; }
            if (data->IsDampOutProgressUniform())   { shock->progress_factor_damp_out   = data->damp_out_progress; }
                                                    
            if (data->IsDetacherGroupUniform())     { shock->detacher_group = data->detacher_group; }
            if (data->IsExtensionLimitUniform())    { shock->long_bound     = data->extension_limit; }
            if (data->IsContractionLimitUniform())  { shock->short_bound    = data->contraction_limit; }
            if (data->IsPrecompressionUniform())    { shock->precompression = data->precompression; }
        }
    }
}

void Rig::SelectedHydrosUpdateAttributes(const RigAggregateHydrosData* data)
{
    auto itor_end = m_selected_beams.end();
    for (auto itor = m_selected_beams.begin(); itor != itor_end; ++itor)
    {
        Beam* beam = *itor;
        if (beam->GetType() == Beam::TYPE_STEERING_HYDRO)
        {
            RigDef::Hydro* def = beam->GetDefinitionHydro();
            
            if (data->IsFlagUniform_a()) { if (!def->HasFlag_a()) { def->AddFlag(RigDef::Hydro::OPTION_a_INPUT_AILERON);             } }
            if (data->IsFlagUniform_e()) { if (!def->HasFlag_e()) { def->AddFlag(RigDef::Hydro::OPTION_e_INPUT_ELEVATOR);            } }
            if (data->IsFlagUniform_g()) { if (!def->HasFlag_g()) { def->AddFlag(RigDef::Hydro::OPTION_g_INPUT_ELEVATOR_RUDDER);     } }
            if (data->IsFlagUniform_h()) { if (!def->HasFlag_h()) { def->AddFlag(RigDef::Hydro::OPTION_h_INPUT_InvELEVATOR_RUDDER);  } }
            if (data->IsFlagUniform_i()) { if (!def->HasFlag_i()) { def->AddFlag(RigDef::Hydro::OPTION_i_INVISIBLE);                 } }
            if (data->IsFlagUniform_r()) { if (!def->HasFlag_r()) { def->AddFlag(RigDef::Hydro::OPTION_r_INPUT_RUDDER);              } }
            if (data->IsFlagUniform_s()) { if (!def->HasFlag_s()) { def->AddFlag(RigDef::Hydro::OPTION_s_DISABLE_ON_HIGH_SPEED);     } }
            if (data->IsFlagUniform_u()) { if (!def->HasFlag_u()) { def->AddFlag(RigDef::Hydro::OPTION_u_INPUT_AILERON_ELEVATOR);    } }
            if (data->IsFlagUniform_v()) { if (!def->HasFlag_v()) { def->AddFlag(RigDef::Hydro::OPTION_v_INPUT_InvAILERON_ELEVATOR); } }
            if (data->IsFlagUniform_x()) { if (!def->HasFlag_x()) { def->AddFlag(RigDef::Hydro::OPTION_x_INPUT_AILERON_RUDDER);      } }
            if (data->IsFlagUniform_y()) { if (!def->HasFlag_y()) { def->AddFlag(RigDef::Hydro::OPTION_y_INPUT_InvAILERON_RUDDER);   } }
                
            if (data->IsDetacherGroupUniform())       { def->detacher_group             = data->detacher_group;        }
            if (data->IsExtensionFactorUniform())     { def->lenghtening_factor         = data->extension_factor;      }
            if (data->IsInertiaStartDelayUniform())   { def->inertia.start_delay_factor = data->inertia_start_delay;   }
            if (data->IsInertiaStopDelayUniform())    { def->inertia.stop_delay_factor  = data->inertia_stop_delay;    }
            if (data->IsInertiaStartFunctionUniform()){ def->inertia.start_function	    = data->inertia_start_function;}
            if (data->IsInertiaStopFunctionUniform()) { def->inertia.stop_function	    = data->inertia_stop_function; }
        }
    }
}

void Rig::SelectedCommands2UpdateAttributes(const RigAggregateCommands2Data*   data)
{
    auto itor_end = m_selected_beams.end();
    for (auto itor = m_selected_beams.begin(); itor != itor_end; ++itor)
    {
        Beam* beam = *itor;
        if (beam->GetType() == Beam::TYPE_COMMAND_HYDRO)
        {
            RigDef::Command2* def = beam->GetDefinitionCommand2();

            if (data->IsFlagUniform_c()) { def->option_c_auto_center   = data->HasFlag_c(); }
            if (data->IsFlagUniform_f()) { def->option_f_not_faster    = data->HasFlag_f(); }
            if (data->IsFlagUniform_i()) { def->option_i_invisible     = data->HasFlag_i(); }
            if (data->IsFlagUniform_o()) { def->option_o_1press_center = data->HasFlag_o(); }
            if (data->IsFlagUniform_p()) { def->option_p_1press        = data->HasFlag_p(); }
            if (data->IsFlagUniform_r()) { def->option_r_rope          = data->HasFlag_r(); }

            if (data->IsDetacherGroupUniform())        { def->detacher_group              = data->detacher_group        ; } 
            if (data->IsContractionRateUniform())      { def->shorten_rate                = data->contraction_rate      ; } 
            if (data->IsExtensionRateUniform())        { def->lengthen_rate               = data->extension_rate        ; } 
            if (data->IsContractKeyUniform())          { def->contract_key                = data->contract_key          ; } 
            if (data->IsExtendKeyUniform())            { def->extend_key                  = data->extend_key            ; } 
            if (data->IsDescriptionUniform())          { def->description                 = data->description           ; } 
            if (data->IsContractionLimitUniform())     { def->max_contraction             = data->max_contraction       ; } 
            if (data->IsExtensionLimitUniform())       { def->max_extension               = data->max_extension         ; } 
            if (data->IsInertiaStartDelayUniform())    { def->inertia.start_delay_factor  = data->inertia_start_delay   ; } 
            if (data->IsInertiaStopDelayUniform())     { def->inertia.stop_delay_factor   = data->inertia_stop_delay    ; } 
            if (data->IsInertiaStartFunctionUniform()) { def->inertia.start_function      = data->inertia_start_function; } 
            if (data->IsInertiaStopFunctionUniform())  { def->inertia.stop_function       = data->inertia_stop_function ; } 
            if (data->IsAffectEngineUniform())         { def->affect_engine               = data->affect_engine         ; } 
            if (data->IsNeedsEngineUniform())          { def->needs_engine                = data->GetBoolNeedsEngine()  ; } 
        }
    }
}

void Rig::SelectedShocksUpdateAttributes(const RigAggregateShocksData* data)
{
    auto itor_end = m_selected_beams.end();
    for (auto itor = m_selected_beams.begin(); itor != itor_end; ++itor)
    {
        Beam* beam = *itor;
        if (beam->GetType() == Beam::TYPE_SHOCK_ABSORBER)
        {
            RigDef::Shock* shock = beam->GetDefinitionShock();
            
            if (data->IsFlagUniform_i()) { shock->SetOption_i_Invisible(  data->HasFlag_i()); }
            if (data->IsFlagUniform_L()) { shock->SetOption_L_ActiveLeft( data->HasFlag_L()); }
            if (data->IsFlagUniform_R()) { shock->SetOption_R_ActiveRight(data->HasFlag_R()); }
            if (data->IsFlagUniform_m()) { shock->SetOption_m_Metric(     data->HasFlag_m()); }

            if (data->IsSpringUniform())           { shock->spring_rate    = data->spring_rate; }
            if (data->IsDampingUniform())          { shock->damping        = data->damping; }
            if (data->IsDetacherGroupUniform())    { shock->detacher_group = data->detacher_group; }
            if (data->IsExtensionLimitUniform())   { shock->long_bound     = data->extension_limit; }
            if (data->IsContractionLimitUniform()) { shock->short_bound    = data->contraction_limit; }
            if (data->IsPrecompressionUniform())   { shock->precompression = data->precompression; }
        }
    }
}

void Rig::CheckAndRefreshWheelsSelectionHighlights(RigEditor::Main* rig_editor, Ogre::SceneNode* parent_scene_node, bool force)
{
    if (m_wheel_visuals->IsSelectionDirty() || force)
    {
        m_wheel_visuals->UpdateWheelsSelectionHighlightBoxes(m_wheels, rig_editor, parent_scene_node);	
    }
}

void Rig::CheckAndRefreshWheelsMouseHoverHighlights(RigEditor::Main* rig_editor, Ogre::SceneNode* parent_scene_node)
{
    if (m_wheel_visuals->IsHoverDirty())
    {
        m_wheel_visuals->UpdateWheelsMouseHoverHighlightBoxes(m_wheels, rig_editor, parent_scene_node);
    }
}

bool Rig::SetWheelSelected(LandVehicleWheel* wheel, int index, bool state_selected, RigEditor::Main* rig_editor)
{
    if (wheel->IsSelected() == state_selected)
    {
        return false; // No change
    }
    wheel->SetIsSelected(state_selected);
    m_wheel_visuals->SetIsSelectionDirty(true);
    return true;
}

bool Rig::ScheduleSetWheelSelected(LandVehicleWheel* wheel, int index, bool state_selected, RigEditor::Main* rig_editor)
{
    if (state_selected)
    {
        if (!wheel->IsScheduledForSelect() && !wheel->IsSelected())
        {
            wheel->SetIsScheduledForDeselect(false);
            wheel->SetIsScheduledForSelect(true);
            return true;
        }
    }
    else
    {
        if (!wheel->IsScheduledForDeselect() && wheel->IsSelected())
        {
            wheel->SetIsScheduledForSelect(false);
            wheel->SetIsScheduledForDeselect(true);
            return true;
        }
    }
    return false;
}

bool Rig::PerformScheduledWheelSelectionUpdates(RigEditor::Main* rig_editor)
{
    bool any_change = false;
    auto end = m_wheels.end();
    auto itor = m_wheels.begin();
    for (; itor != end; ++itor)
    {
        LandVehicleWheel* wheel = *itor;
        if (wheel->IsScheduledForSelect() && !wheel->IsSelected())
        {
            any_change = true;
            wheel->SetIsSelected(true);
            wheel->SetIsScheduledForSelect(false);
            wheel->SetIsScheduledForDeselect(false);
        }
        else if (wheel->IsScheduledForDeselect() && wheel->IsSelected())
        {
            any_change = true;
            wheel->SetIsSelected(false);
            wheel->SetIsScheduledForSelect(false);
            wheel->SetIsScheduledForDeselect(false);
        }
    }
    if (any_change)
    {
        m_wheel_visuals->SetIsSelectionDirty(true);
    }
    return any_change;
}

void Rig::SetWheelHovered(LandVehicleWheel* wheel, int index, bool state_hovered, RigEditor::Main* rig_editor)
{
    if (wheel->IsHovered() == state_hovered)
    {
        return; // No change
    }
    // Update hover state and visuals
    wheel->SetIsHovered(state_hovered);
    m_wheel_visuals->SetIsHoverDirty(true);
}

bool Rig::SetAllWheelsSelected(bool state_selected, RigEditor::Main* rig_editor)
{
    bool anything_changed = false;
    auto end = m_wheels.end();
    for (auto itor = m_wheels.begin(); itor != end; ++itor)
    {
        LandVehicleWheel* wheel = *itor;
        if (wheel->IsSelected() != state_selected)
        {
            anything_changed = true;
            wheel->SetIsSelected(state_selected);
        }
    }
    if (anything_changed)
    {
        m_wheel_visuals->SetIsSelectionDirty(true);
        return true;
    }
    return false;
}

bool Rig::ScheduleSetAllWheelsSelected(bool state_selected, RigEditor::Main* rig_editor)
{
    bool anything_changed = false;
    auto end = m_wheels.end();
    auto itor = m_wheels.begin();
    for (; itor != end; ++itor)
    {
        LandVehicleWheel* wheel = *itor;
        anything_changed = anything_changed || ScheduleSetWheelSelected(wheel, -1, state_selected, rig_editor);
    }
    return anything_changed;
}

void Rig::SetAllWheelsHovered(bool state_hovered, RigEditor::Main* rig_editor)
{
    bool anything_changed = false;
    auto end = m_wheels.end();
    for (auto itor = m_wheels.begin(); itor != end; ++itor)
    {
        LandVehicleWheel* wheel = *itor;
        if (wheel->IsHovered() != state_hovered)
        {
            anything_changed = true;
            wheel->SetIsHovered(state_hovered);
        }
    }
    if (anything_changed)
    {
        m_wheel_visuals->SetIsHoverDirty(true);
    }
}

void Rig::QuerySelectedWheelsData(AllWheelsAggregateData* out_data)
{
    assert(out_data != nullptr);
    out_data->Reset();
    auto end = m_wheels.end();
    auto itor = m_wheels.begin();
    for (; itor != end; ++itor)
    {
        LandVehicleWheel* wheel = *itor;
        if (wheel->IsSelected())
        {
            out_data->AddWheel(wheel);
        }
    }
}

bool Rig::UpdateSelectedWheelsData(AllWheelsAggregateData* data)
{
    assert(data != nullptr);
    bool is_geometry_dirty = false;
    std::for_each(m_wheels.begin(), m_wheels.end(), [data, &is_geometry_dirty](LandVehicleWheel* wheel)
    {
        if (wheel->IsSelected())
        {
            wheel->Update(data);
            is_geometry_dirty = is_geometry_dirty || wheel->IsGeometryDirty();
        }
    });
    return is_geometry_dirty;
}

// ----------------------------------------------------------------------------
// EOF
// ----------------------------------------------------------------------------
