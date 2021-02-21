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

/// @file
/// @author Petr Ohlidal
/// @date   04/2015

#include <iomanip>
#include "TruckSequentialImporter.h"

#include "Application.h"
#include "Console.h"
#include "TruckParser.h"

using namespace Truck;

void SequentialImporter::Init(bool enabled)
{
    m_enabled = enabled;
    m_all_nodes.clear();
    m_all_nodes.reserve(1000);
    m_current_keyword = KEYWORD_INVALID;
    m_current_module.reset();

    m_num_numbered_nodes       = 0;
    m_num_named_nodes          = 0;
    m_num_cinecam_nodes        = 0;
    m_num_wheels_nodes         = 0;
    m_num_wheels2_nodes        = 0;
    m_num_meshwheels_nodes     = 0;
    m_num_meshwheels2_nodes    = 0;
    m_num_flexbodywheels_nodes = 0;
}

void SequentialImporter::GenerateNodesForWheel(Keyword generated_from, int num_rays, bool has_rigidity_node)
{
    // Old parser logic:
    // Note: Axis nodes are A/B, where B has bigger Z coordinate.
    //
    // Section        | Function()    | Generated nodes
    // -------------------------------------------------------------------------------------------
    // wheels         | addWheel()    | Tyre nodes: A, B, A, B... (num_rays*2) 
    // wheels2        | addWheel2()   | Rim nodes: A, B, A, B... (num_rays*2), Tyre nodes: A, B, A, B... (num_rays*2)
    // meshwheels     | addWheel()    | Tyre nodes: A, B, A, B... (num_rays*2)
    // meshwheels2    | addWheel()    | Tyre nodes: A, B, A, B... (num_rays*2)
    // flexbodywheels | addWheel3()   | Rim nodes: A, B, A, B... (num_rays*2), Tyre nodes: A, B, A, B... (num_rays*2)

    NodeMapEntry::OriginDetail detail;
    if (generated_from == KEYWORD_FLEXBODYWHEELS || generated_from == KEYWORD_WHEELS2)
    {
        // Rim nodes
        for (int i = 0; i < num_rays*2; ++i)
        {
            detail = (i % 2 == 0) ? NodeMapEntry::DETAIL_WHEEL_RIM_A : NodeMapEntry::DETAIL_WHEEL_RIM_B;
            this->AddGeneratedNode(generated_from, detail);
        }
    }
    // Tyre nodes
    for (int i = 0; i < num_rays*2; ++i)
    {
        detail = (i % 2 == 0) ? NodeMapEntry::DETAIL_WHEEL_TYRE_A : NodeMapEntry::DETAIL_WHEEL_TYRE_B;
        this->AddGeneratedNode(generated_from, detail);
    }
}

void SequentialImporter::Process(Truck::DocumentPtr def)
{
    this->ProcessModule(def->root_module);

    auto end  = def->user_modules.end();
    auto itor = def->user_modules.begin();
    for (;itor != end; ++itor)
    {
        this->ProcessModule((*itor).second);
    }

    if (RoR::App::diag_rig_log_node_stats->GetBool())
    {
        this->AddMessage(Message::TYPE_INFO, this->GetNodeStatistics());
    }
    if (RoR::App::diag_rig_log_node_import->GetBool())
    {
        this->AddMessage(Message::TYPE_INFO, this->IterateAndPrintAllNodes());
    }
}

void SequentialImporter::AddMessage(Message::Type msg_type, std::string text)
{
    RoR::Str<2000> txt;
    txt << text << "(";
    if (m_current_module) // Not present while adding nodes, only while resolving
    {
        txt << "sectionconfig: " << m_current_module->name << ", ";
    }
    txt << "keyword: " << Document::KeywordToString(m_current_keyword) << ")";

    RoR::Console::MessageType cm_type;
    switch (msg_type)
    {
    case Message::TYPE_FATAL_ERROR:
        cm_type = RoR::Console::MessageType::CONSOLE_SYSTEM_ERROR;
        break;

    case Message::TYPE_ERROR:
    case Message::TYPE_WARNING:
        cm_type = RoR::Console::MessageType::CONSOLE_SYSTEM_WARNING;
        break;

    default:
        cm_type = RoR::Console::MessageType::CONSOLE_SYSTEM_NOTICE;
        break;
    }

    RoR::App::GetConsole()->putMessage(RoR::Console::CONSOLE_MSGTYPE_ACTOR, cm_type, txt.ToCStr());
}

unsigned SequentialImporter::GetNodeArrayOffset(Keyword keyword)
{
    // Reverse order of elements in nodes-array
    // "break" keyword is omitted - control falls through!
    unsigned out_offset = 0;
    switch (keyword)
    {
    case KEYWORD_FLEXBODYWHEELS: out_offset += m_num_meshwheels2_nodes; // NO break! 
    case KEYWORD_MESHWHEELS2:    out_offset += m_num_meshwheels_nodes;  // NO break!
    case KEYWORD_MESHWHEELS:     out_offset += m_num_wheels2_nodes;     // NO break!
    case KEYWORD_WHEELS2:        out_offset += m_num_wheels_nodes;      // NO break!
    case KEYWORD_WHEELS:         out_offset += m_num_cinecam_nodes;     // NO break!
    case KEYWORD_CINECAM:        out_offset += m_num_named_nodes;       // NO break!
    case KEYWORD_NODES2:         out_offset += m_num_numbered_nodes;    // NO break!
    case KEYWORD_NODES:          break; // Starts at 0
    default: break;
    }
    return out_offset;
}

Node::Ref SequentialImporter::ResolveNodeByIndex(unsigned int index_in, unsigned int def_line_number)
{
    if (index_in >= m_all_nodes.size())
    {
        std::stringstream msg;
        msg << "Cannot resolve node by index [" << index_in << "], node is not defined, highest available number is: " << m_all_nodes.size() - 1;
        this->AddMessage(Message::TYPE_ERROR, msg.str());
        return Node::Ref(); // Invalid reference
    }
    NodeMapEntry node_entry = m_all_nodes[index_in];
    unsigned index_out = this->GetNodeArrayOffset(node_entry.origin_keyword) + node_entry.node_sub_index;
    if (index_out != index_in)
    {
        std::stringstream msg;
            msg << "Node resolved by index.\n\tSource: [" << index_in << "]\n\tResult: [" << index_out << "]";
        if (node_entry.origin_keyword != KEYWORD_NODES && node_entry.origin_keyword != KEYWORD_NODES2)
        {
            msg << " (generated by: " << Document::KeywordToString(node_entry.origin_keyword) 
                << ", sub-index: " << node_entry.node_sub_index << ")";
        }
        this->AddMessage(Message::TYPE_INFO, msg.str());
    }
    else
    {
        ++m_num_resolved_to_self; // The happy statistic -> nodes which resolved to the same index.
    }
    return Node::Ref(TOSTRING(index_out), index_out, Node::Ref::IMPORT_STATE_IS_VALID | Node::Ref::IMPORT_STATE_IS_RESOLVED_NUMBERED, def_line_number);
}

Node::Ref SequentialImporter::ResolveNode(Node::Ref const & noderef_in)
{
    ++m_total_resolved;
    if (!noderef_in.GetImportState_IsValid())
    {
        return noderef_in;
    }

    // Work like legacy function SerializedRig::parse_node_number()
    // If node may be named, try resolving it as named first
    // If that fails, try numbered

    if (noderef_in.GetImportState_MustCheckNamedFirst())
    {
        auto result = m_named_nodes.find(noderef_in.Str());
        if (result != m_named_nodes.end())
        {
            return Node::Ref(noderef_in.Str(), 0, Node::Ref::IMPORT_STATE_IS_VALID | Node::Ref::IMPORT_STATE_IS_RESOLVED_NAMED, noderef_in.GetLineNumber());
        }
    }
    if (noderef_in.Num() >= m_all_nodes.size())
    {
        // Return exactly what SerializedRig::parse_node_number() would return on this error, for compatibility.
        // This definitely isn't valid, but it behaves exactly as RoR 0.38, so it's perfectly IMPORT_VALID! :D
        Node::Ref out_ref("0", 0, Node::Ref::IMPORT_STATE_IS_VALID | Node::Ref::IMPORT_STATE_IS_RESOLVED_NUMBERED, noderef_in.GetLineNumber());
        std::stringstream msg;
        msg << "Cannot resolve " << noderef_in.ToString() << " - not a named node, and index is not defined (highest is: "
            << m_all_nodes.size() - 1 << "). For backwards compatibility, converting to: " << out_ref.ToString();
        this->AddMessage(Message::TYPE_ERROR, msg.str());
        return out_ref; // Invalid
    }
    auto entry = m_all_nodes[noderef_in.Num()];
    if (entry.node_id.IsTypeNamed())
    {
        Node::Ref out_ref(entry.node_id.Str(), 0, Node::Ref::IMPORT_STATE_IS_VALID | Node::Ref::IMPORT_STATE_IS_RESOLVED_NAMED, noderef_in.GetLineNumber());
        /* TODO: make optional (debug) or remove
        std::stringstream msg;
        msg << "Node resolved\n\tSource: " << noderef_in.ToString() << "\n\tResult: " << out_ref.ToString();
        this->AddMessage(Message::TYPE_INFO, msg.str());
        */
        return out_ref;
    }
    else if (entry.node_id.IsTypeNumbered())
    {
        unsigned out_index = this->GetNodeArrayOffset(entry.origin_keyword) + entry.node_sub_index;
        Node::Ref out_ref(TOSTRING(out_index), out_index, Node::Ref::IMPORT_STATE_IS_VALID | Node::Ref::IMPORT_STATE_IS_RESOLVED_NUMBERED, noderef_in.GetLineNumber());
        /* TODO: make optional (debug) or remove
        std::stringstream msg;
        msg << "Node resolved\n\tSource: " << noderef_in.ToString() << "\n\tResult: " << out_ref.ToString()
            << "\n\tOrigin: " << Truck::Document::KeywordToString(entry.origin_keyword) << " SubIndex: " << entry.node_sub_index;
        this->AddMessage(Message::TYPE_INFO, msg.str());
        */
        return out_ref;
    }
    else
    {
        std::stringstream msg;
        msg << "Cannot resolve " << noderef_in.ToString() << " - found node is not valid";
        this->AddMessage(Message::TYPE_ERROR, msg.str());
        return Node::Ref(); // Invalid
    }
}

void SequentialImporter::ResolveFlexbodyForset(std::vector<Node::Range>& in_ranges, std::vector<Node::Ref>& out_nodes)
{
    auto range_itor = in_ranges.begin();
    auto range_end  = in_ranges.end();
    for ( ; range_itor != range_end; ++range_itor)
    {
        Node::Range& range = *range_itor;
        if (!range.IsRange())
        {
            if (!range.start.GetImportState_IsValid())
            {
                std::stringstream msg;
                msg << "ResolveFlexbodyForset(): Skipping node because it's marked INVALID:" << range.start.ToString();
                this->AddMessage(Message::TYPE_ERROR, msg.str());
            }
            else
            {
                Node::Ref node_ref = this->ResolveNodeByIndex(range.start.Num(), range.start.GetLineNumber()); // Forset nodes are numbered-only
                // Invalid nodes are simply thrown away, for backwards compatibility 
                // (FlexBody loops through existing nodes first, and then evaluates if they are in the SET)
                if (node_ref.IsValidAnyState())
                {
                    out_nodes.push_back(node_ref);
                }
                else
                {
                    std::stringstream msg;
                    msg << "ResolveFlexbodyForset(): Stand-alone node [" << range.start.ToString() << "] resolved invalid, removing from FORSET";
                    this->AddMessage(Message::TYPE_WARNING, msg.str());
                }
            }
        }
        else // It's a range
        {
            if (!range.start.GetImportState_IsValid() || !range.end.GetImportState_IsValid())
            {
                std::stringstream msg;
                msg << "ResolveFlexbodyForset(): Skipping range because some nodes are marked INVALID, start: [" << range.start.ToString() << "], end: [" << range.end.ToString() << "]";
                this->AddMessage(Message::TYPE_ERROR, msg.str());
            }
            else if (range.start.GetImportState_IsResolvedNamed() || range.end.GetImportState_IsResolvedNamed())
            {
                std::stringstream msg;
                msg << "ResolveFlexbodyForset(): Some nodes in range are already resolved as named, unable to process, start: [" << range.start.ToString() << "], end: [" << range.end.ToString() << "]";
                this->AddMessage(Message::TYPE_ERROR, msg.str());
            }
            else
            {
                unsigned int end_index = range.end.Num();
                unsigned int line_num = range.start.GetLineNumber();
                for (unsigned int i = range.start.Num(); i <= end_index; ++i)
                {
                    Node::Ref node_ref = this->ResolveNodeByIndex(i, line_num); // Forset nodes are numbered-only
                    // Invalid nodes are simply thrown away, for backwards compatibility 
                    // (FlexBody loops through existing nodes first, and then evaluates if they are in the SET -> invalid nodes are silently ignored)
                    if (node_ref.IsValidAnyState())
                    {
                        out_nodes.push_back(node_ref);
                    }
                    else
                    {
                        std::stringstream msg;
                        msg << "ResolveFlexbodyForset(): Node ["<<i<<"] from range [" << range.start.ToString() << " - " << range.end.ToString() << "] resolved invalid, removing from FORSET";
                        this->AddMessage(Message::TYPE_WARNING, msg.str());
                    }
                }
            }
        }
    }
}

void SequentialImporter::ResolveNodeRanges(std::vector<Node::Range>& ranges)
{
    auto in_ranges = ranges; // Copy vector
    ranges.clear();

    auto range_itor = in_ranges.begin();
    auto range_end  = in_ranges.end();
    for ( ; range_itor != range_end; ++range_itor)
    {
        Node::Range& range = *range_itor;
        if (!range.IsRange())
        {
            ranges.push_back(Node::Range(this->ResolveNode(range.start)));
        }
        else if (!range.start.GetImportState_IsValid() || !range.end.GetImportState_IsValid())
        {
            std::stringstream msg;
            msg << "Some nodes in range are invalid, start: [" << range.start.ToString() << "], end: [" << range.end.ToString() << "]";
            this->AddMessage(Message::TYPE_ERROR, msg.str());
        }
        else if (range.start.GetImportState_IsResolvedNamed() || range.end.GetImportState_IsResolvedNamed())
        {
            std::stringstream msg;
            msg << "Some nodes in range are already resolved as named, unable to process, start: [" << range.start.ToString() << "], end: [" << range.end.ToString() << "]";
            this->AddMessage(Message::TYPE_ERROR, msg.str());
        }
        else
        {
            unsigned int end_index = range.end.Num();
            for (unsigned int i = range.start.Num(); i < end_index; ++i)
            {
                ranges.push_back(Node::Range(this->ResolveNodeByIndex(i, range.start.GetLineNumber())));
            }
        }
    }
}

#define RESOLVE_OPTIONAL_SECTION(KEYWORD, PTRNAME, BLOCK) \
{                                                         \
    m_current_keyword = KEYWORD;                          \
    if (PTRNAME)                                          \
    {                                                     \
        BLOCK                                             \
    }                                                     \
    m_current_keyword = KEYWORD_INVALID;            \
}

#define FOR_EACH(KEYWORD, VECTOR, VARNAME, BLOCK) \
{                                                 \
    m_current_keyword = KEYWORD;                  \
    auto itor_ = VECTOR.begin();                  \
    auto end_ = VECTOR.end();                     \
    for (; itor_ != end_; ++itor_)                \
    {                                             \
        auto& VARNAME = *itor_;                   \
        BLOCK                                     \
    }                                             \
    m_current_keyword = KEYWORD_INVALID;    \
}

#define RESOLVE(VARNAME) VARNAME = this->ResolveNode(VARNAME);

#define RESOLVE_VECTOR(VECTORNAME)                             \
{                                                              \
    auto end = VECTORNAME.end();                               \
    for ( auto itor = VECTORNAME.begin(); itor != end; ++itor) \
    {                                                          \
        RESOLVE(*itor);                                        \
    }                                                          \
}

// Support for named nodes in parsers [version 0.39, version 0.4.0.7]
// airbrakes = yes,yes
// axles = NO, NO

void SequentialImporter::ProcessModule(Truck::ModulePtr module)
{
    m_current_module = module;

    FOR_EACH (KEYWORD_AIRBRAKES, module->airbrakes, airbrake,
    {
        RESOLVE(airbrake.reference_node);
        RESOLVE(airbrake.x_axis_node   );
        RESOLVE(airbrake.y_axis_node   );
        RESOLVE(airbrake.aditional_node);
    });

    FOR_EACH (KEYWORD_AXLES, module->axles, axle,
    {
        RESOLVE(axle.wheels[0][0]);
        RESOLVE(axle.wheels[0][1]);
        RESOLVE(axle.wheels[1][0]);
        RESOLVE(axle.wheels[1][1]);
    });

    FOR_EACH (KEYWORD_BEAMS, module->beams, beam,
    {
        RESOLVE(beam.nodes[0]);
        RESOLVE(beam.nodes[1]);
    });

    FOR_EACH (KEYWORD_CAMERAS, module->cameras, camera,
    {
        RESOLVE(camera.center_node);
        RESOLVE(camera.back_node  );
        RESOLVE(camera.left_node  );
    });

    FOR_EACH (KEYWORD_CAMERARAIL, module->camera_rails, rail,
    {
        RESOLVE_VECTOR(rail.nodes);
    });

    FOR_EACH (KEYWORD_CINECAM, module->cinecam, cinecam,
    {
        RESOLVE(cinecam.nodes[0]);
        RESOLVE(cinecam.nodes[1]);
        RESOLVE(cinecam.nodes[2]);
        RESOLVE(cinecam.nodes[3]);
        RESOLVE(cinecam.nodes[4]);
        RESOLVE(cinecam.nodes[5]);
        RESOLVE(cinecam.nodes[6]);
        RESOLVE(cinecam.nodes[7]);
    });

    FOR_EACH (KEYWORD_COLLISIONBOXES, module->collision_boxes, box,
    {
        RESOLVE_VECTOR(box.nodes);
    });

    FOR_EACH (KEYWORD_COMMANDS2, module->commands_2, command,
    {
        RESOLVE(command.nodes[0]);
        RESOLVE(command.nodes[1]);
    });

    FOR_EACH (KEYWORD_CONTACTERS, module->contacters, contacter_node,
    {
        RESOLVE(contacter_node);
    });

    FOR_EACH (KEYWORD_EXHAUSTS, module->exhausts, exhaust,
    {
        RESOLVE(exhaust.reference_node);
        RESOLVE(exhaust.direction_node);
    });

    RESOLVE_OPTIONAL_SECTION (KEYWORD_EXTCAMERA, module->ext_camera,
    {
        RESOLVE(module->ext_camera->node);
    });

    FOR_EACH (KEYWORD_FIXES, module->fixes, fixed_node,
    {
        RESOLVE(fixed_node);
    });

    FOR_EACH (KEYWORD_FLARES2, module->flares_2, flare2,
    {
        RESOLVE(flare2.reference_node);
        RESOLVE(flare2.node_axis_x   );
        RESOLVE(flare2.node_axis_y   );
    });

    FOR_EACH (KEYWORD_FLEXBODIES, module->flexbodies, flexbody,
    {
        RESOLVE(flexbody->reference_node);
        RESOLVE(flexbody->x_axis_node   );
        RESOLVE(flexbody->y_axis_node   );

        ResolveFlexbodyForset(flexbody->node_list_to_import, flexbody->node_list);
        flexbody->node_list_to_import.clear();
    });

    FOR_EACH (KEYWORD_FLEXBODYWHEELS, module->flex_body_wheels, flexbodywheel,
    {
        RESOLVE(flexbodywheel.nodes[0]          );
        RESOLVE(flexbodywheel.nodes[1]          );
        RESOLVE(flexbodywheel.rigidity_node     );
        RESOLVE(flexbodywheel.reference_arm_node);
    });

    FOR_EACH (KEYWORD_FUSEDRAG, module->fusedrag, fusedrag,
    {
        RESOLVE(fusedrag.front_node);
        RESOLVE(fusedrag.rear_node);
    });

    FOR_EACH (KEYWORD_HOOKS, module->hooks, hook,
    {
        RESOLVE(hook.node);
    });

    FOR_EACH (KEYWORD_HYDROS, module->hydros, hydro,
    {
        RESOLVE(hydro.nodes[0]);
        RESOLVE(hydro.nodes[1]);
    });

    FOR_EACH (KEYWORD_MESHWHEELS, module->mesh_wheels, meshwheel, // Also `meshwheels2`
    {
        RESOLVE(meshwheel.nodes[0]          );
        RESOLVE(meshwheel.nodes[1]          );
        RESOLVE(meshwheel.rigidity_node     );
        RESOLVE(meshwheel.reference_arm_node);
    });

    FOR_EACH (KEYWORD_PARTICLES, module->particles, particle,
    {
        RESOLVE(particle.emitter_node);
        RESOLVE(particle.reference_node);
    });

    FOR_EACH (KEYWORD_PROPS, module->props, prop,
    {
        RESOLVE(prop.reference_node);
        RESOLVE(prop.x_axis_node   );
        RESOLVE(prop.y_axis_node   );
    });

    FOR_EACH (KEYWORD_RAILGROUPS, module->railgroups, railgroup,
    {
        ResolveNodeRanges(railgroup.node_list);
    });

    FOR_EACH (KEYWORD_ROPABLES, module->ropables, ropable,
    {
        RESOLVE(ropable.node);
    });

    FOR_EACH (KEYWORD_ROPES, module->ropes, rope,
    {
        RESOLVE(rope.root_node);
        RESOLVE(rope.end_node );
    });

    FOR_EACH (KEYWORD_ROTATORS, module->rotators, rotator,
    {
        RESOLVE(rotator.axis_nodes[0]          );
        RESOLVE(rotator.axis_nodes[1]          );
        RESOLVE(rotator.base_plate_nodes[0]    );
        RESOLVE(rotator.base_plate_nodes[1]    );
        RESOLVE(rotator.base_plate_nodes[2]    );
        RESOLVE(rotator.base_plate_nodes[3]    );
        RESOLVE(rotator.rotating_plate_nodes[0]);
        RESOLVE(rotator.rotating_plate_nodes[1]);
        RESOLVE(rotator.rotating_plate_nodes[2]);
        RESOLVE(rotator.rotating_plate_nodes[3]);
    });

    FOR_EACH (KEYWORD_ROTATORS2, module->rotators_2, rotator2,
    {
        RESOLVE(rotator2.axis_nodes[0]          );
        RESOLVE(rotator2.axis_nodes[1]          );
        RESOLVE(rotator2.base_plate_nodes[0]    );
        RESOLVE(rotator2.base_plate_nodes[1]    );
        RESOLVE(rotator2.base_plate_nodes[2]    );
        RESOLVE(rotator2.base_plate_nodes[3]    );
        RESOLVE(rotator2.rotating_plate_nodes[0]);
        RESOLVE(rotator2.rotating_plate_nodes[1]);
        RESOLVE(rotator2.rotating_plate_nodes[2]);
        RESOLVE(rotator2.rotating_plate_nodes[3]);
    });

    FOR_EACH (KEYWORD_SCREWPROPS, module->screwprops, screwprop,
    {
        RESOLVE(screwprop.prop_node);
        RESOLVE(screwprop.back_node);
        RESOLVE(screwprop.top_node );
    });

    FOR_EACH (KEYWORD_SHOCKS, module->shocks, shock,
    {
        RESOLVE(shock.nodes[0]);
        RESOLVE(shock.nodes[1]);
    });

    FOR_EACH (KEYWORD_SHOCKS2, module->shocks_2, shock2,
    {
        RESOLVE(shock2.nodes[0]);
        RESOLVE(shock2.nodes[1]);
    });

    FOR_EACH (KEYWORD_SHOCKS3, module->shocks_3, shock3,
    {
        RESOLVE(shock3.nodes[0]);
        RESOLVE(shock3.nodes[1]);
    });

    FOR_EACH (KEYWORD_SLIDENODES, module->slidenodes, slidenode,
    {
        RESOLVE(slidenode.slide_node);

        ResolveNodeRanges(slidenode.rail_node_ranges);
    });

    FOR_EACH (KEYWORD_SOUNDSOURCES, module->soundsources, soundsource,
    {
        RESOLVE(soundsource.node);
    });

    FOR_EACH (KEYWORD_SOUNDSOURCES2, module->soundsources2, soundsource2,
    {
        RESOLVE(soundsource2.node);
    });

    FOR_EACH (KEYWORD_SUBMESH, module->submeshes, submesh,
    {
        m_current_keyword = KEYWORD_TEXCOORDS;
        auto texcoord_itor = submesh.texcoords.begin();
        auto texcoord_end  = submesh.texcoords.end();
        for (; texcoord_itor != texcoord_end; ++texcoord_itor)
        {
            RESOLVE(texcoord_itor->node);
        }

        m_current_keyword = KEYWORD_CAB;
        auto cab_itor = submesh.cab_triangles.begin();
        auto cab_end  = submesh.cab_triangles.end();
        for (; cab_itor != cab_end; ++cab_itor)
        {
            RESOLVE(cab_itor->nodes[0]);
            RESOLVE(cab_itor->nodes[1]);
            RESOLVE(cab_itor->nodes[2]);
        }
    });

    FOR_EACH (KEYWORD_TIES, module->ties, tie,
    {
        RESOLVE(tie.root_node);
    });

    FOR_EACH (KEYWORD_TRIGGERS, module->triggers, trigger,
    {
        RESOLVE(trigger.nodes[0]);
        RESOLVE(trigger.nodes[1]);
    });

    FOR_EACH (KEYWORD_TURBOJETS, module->turbojets, turbojet,
    {
        RESOLVE(turbojet.front_node);
        RESOLVE(turbojet.back_node );
        RESOLVE(turbojet.side_node );
    });

    FOR_EACH (KEYWORD_TURBOPROPS2, module->turboprops_2, turboprop2,
    {
        RESOLVE(turboprop2.reference_node    );
        RESOLVE(turboprop2.axis_node         );
        RESOLVE(turboprop2.blade_tip_nodes[0]);
        RESOLVE(turboprop2.blade_tip_nodes[1]);
        RESOLVE(turboprop2.blade_tip_nodes[2]);
        RESOLVE(turboprop2.blade_tip_nodes[3]);
    });

    FOR_EACH (KEYWORD_WHEELS, module->wheels, wheel,
    {
        RESOLVE(wheel.nodes[0]          );
        RESOLVE(wheel.nodes[1]          );
        RESOLVE(wheel.rigidity_node     );
        RESOLVE(wheel.reference_arm_node);
    });

    FOR_EACH (KEYWORD_WHEELS2, module->wheels_2, wheel2,
    {
        RESOLVE(wheel2.nodes[0]          );
        RESOLVE(wheel2.nodes[1]          );
        RESOLVE(wheel2.rigidity_node     );
        RESOLVE(wheel2.reference_arm_node);
    });

    FOR_EACH (KEYWORD_VIDEOCAMERA, module->videocameras, videocamera,
    {
        RESOLVE(videocamera.reference_node      );
        RESOLVE(videocamera.left_node           );
        RESOLVE(videocamera.bottom_node         );
        RESOLVE(videocamera.alt_reference_node  );
        RESOLVE(videocamera.alt_orientation_node);
    });

    FOR_EACH (KEYWORD_WINGS, module->wings, wing,
    {
        RESOLVE(wing.nodes[0]);
        RESOLVE(wing.nodes[1]);
        RESOLVE(wing.nodes[2]);
        RESOLVE(wing.nodes[3]);
        RESOLVE(wing.nodes[4]);
        RESOLVE(wing.nodes[5]);
        RESOLVE(wing.nodes[6]);
        RESOLVE(wing.nodes[7]);
    });

    m_current_module.reset();
}

bool SequentialImporter::AddNumberedNode(unsigned int number)
{
    if (number != m_all_nodes.size()) // Check node sync, like legacy parser did.
    {
        std::stringstream msg;
        msg << "Lost sync in node numbers, got numbered node [" << number << "], expected [" << m_all_nodes.size() << "]. Ignoring node.";
        this->AddMessage(Message::TYPE_FATAL_ERROR, msg.str());
        return false; 
    }
    m_all_nodes.push_back( NodeMapEntry(KEYWORD_NODES, Node::Id(number), m_num_numbered_nodes) );
    ++m_num_numbered_nodes;
    return true;
}

bool SequentialImporter::AddNamedNode(std::string const & name)
{
    auto node_entry = NodeMapEntry(KEYWORD_NODES2, Node::Id(name), m_num_named_nodes);
    auto result = m_named_nodes.insert(std::make_pair(name, node_entry));
    if (!result.second)
    {
        std::stringstream msg;
        msg << "Duplicate node name [" << name << "]. Ignoring node.";
        this->AddMessage(Message::TYPE_FATAL_ERROR, msg.str());
        return false;
    }
    m_all_nodes.push_back(node_entry);
    ++m_num_named_nodes;
    return true;
}

void SequentialImporter::AddGeneratedNode(Keyword generated_from, NodeMapEntry::OriginDetail detail /* = NodeMapEntry::DETAIL_UNDEFINED*/ )
{
    unsigned int new_number = static_cast<int>(m_all_nodes.size());
    unsigned int node_sub_index = 0;
    switch (generated_from)
    {
    case KEYWORD_CINECAM:        node_sub_index = m_num_cinecam_nodes       ++; break;
    case KEYWORD_WHEELS:         node_sub_index = m_num_wheels_nodes        ++; break;
    case KEYWORD_WHEELS2:        node_sub_index = m_num_wheels2_nodes       ++; break;
    case KEYWORD_MESHWHEELS:     node_sub_index = m_num_meshwheels_nodes    ++; break;
    case KEYWORD_MESHWHEELS2:    node_sub_index = m_num_meshwheels2_nodes   ++; break;
    case KEYWORD_FLEXBODYWHEELS: node_sub_index = m_num_flexbodywheels_nodes++; break;
    default: break;
    }
    m_all_nodes.push_back( NodeMapEntry(generated_from, Node::Id(new_number), node_sub_index, detail));
}

#define STAT_LINE(STREAM, TITLE, COUNT_VAR, KEYWORD)       \
{                                                        \
    unsigned offset = this->GetNodeArrayOffset(KEYWORD); \
    STREAM << "\n\t" << TITLE << std::setw(4) << COUNT_VAR; \
    if (COUNT_VAR != 0) { STREAM << " (after conversion: start index = " << std::setw(4) << offset << ", end index = " << (offset + COUNT_VAR) - 1 << ")"; } \
}

std::string SequentialImporter::GetNodeStatistics()
{
    std::stringstream msg;
    msg << "~~~ Node statistics: ~~~"
        << "\n\tTotal: " << m_all_nodes.size();
        STAT_LINE(msg, "         nodes: ", m_num_numbered_nodes      , KEYWORD_NODES         );
        STAT_LINE(msg, "        nodes2: ", m_num_named_nodes         , KEYWORD_NODES2        );
        STAT_LINE(msg, "       cinecam: ", m_num_cinecam_nodes       , KEYWORD_CINECAM       );
        STAT_LINE(msg, "        wheels: ", m_num_wheels_nodes        , KEYWORD_WHEELS        );
        STAT_LINE(msg, "       wheels2: ", m_num_wheels2_nodes       , KEYWORD_WHEELS2       );
        STAT_LINE(msg, "    meshwheels: ", m_num_meshwheels_nodes    , KEYWORD_MESHWHEELS    );
        STAT_LINE(msg, "   meshwheels2: ", m_num_meshwheels2_nodes   , KEYWORD_MESHWHEELS2   );
        STAT_LINE(msg, "flexbodywheels: ", m_num_flexbodywheels_nodes, KEYWORD_FLEXBODYWHEELS);

    msg << "\nResolved " << m_total_resolved << " nodes ("
        << m_num_resolved_to_self << " resolved without change)";
    return msg.str();
}

std::string SequentialImporter::IterateAndPrintAllNodes()
{
    std::stringstream report;
    report << "~~~ Iterating all nodes, in order as defined (total: " << m_all_nodes.size() << ") ~~~\n";
    auto itor = m_all_nodes.begin();
    auto end  = m_all_nodes.end();
    int index = 0;
    for (; itor != end; ++itor)
    {
        NodeMapEntry& entry = *itor;
        // Resolve type specifics
        bool is_wheel = false;
        switch (entry.origin_keyword)
        {
        case KEYWORD_WHEELS:
        case KEYWORD_WHEELS2:
        case KEYWORD_MESHWHEELS:
        case KEYWORD_MESHWHEELS2:
        case KEYWORD_FLEXBODYWHEELS:
            is_wheel = true;
            break;
        default:
            break;
        }
        // Add line entry
        report << "\n\t" << std::setw(3) << index << ": " << entry.node_id.ToString() 
            << " (from=" << Document::KeywordToString(entry.origin_keyword)
            << ", sub-index=" << entry.node_sub_index;
        if (is_wheel)
        {
            switch (entry.origin_detail)
            {
                case NodeMapEntry::DETAIL_WHEEL_RIM_A:  report << "[rim,  A]"; break;
                case NodeMapEntry::DETAIL_WHEEL_RIM_B:  report << "[rim,  B]"; break;
                case NodeMapEntry::DETAIL_WHEEL_TYRE_A: report << "[tyre, A]"; break;
                case NodeMapEntry::DETAIL_WHEEL_TYRE_B: report << "[tyre, B]"; break;
                default: break;
            }
        }
        report << ")";
        ++index;
    }
    return report.str();
}
