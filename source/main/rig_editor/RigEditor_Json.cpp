/*
    This source file is part of Rigs of Rods
    Copyright 2013-2017 Petr Ohlidal & contributors

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


/// @file
/// @date   05/2017
/// @author Petr Ohlidal

#include "Application.h"
#include "RigEditor_Json.h"
#include "RigEditor_Node.h"

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/filewritestream.h>

namespace RoR {
namespace RigEditor {

rapidjson::Value& JsonExporter::GetModuleJson()
{
    const char* module_name = m_cur_module_name.c_str();
    if (!m_json_doc["modules"].HasMember(module_name))
    {
        m_json_doc["modules"].AddMember(rapidjson::StringRef(module_name), rapidjson::Value(rapidjson::kObjectType), m_json_doc.GetAllocator());
    }

    return m_json_doc["modules"][module_name];
}

rapidjson::Value& JsonExporter::GetOrCreateMember(
    rapidjson::Value& j_container, 
    const char* name, 
    rapidjson::Type j_type //= rapidjson::kObjectType
)
{
    if (!j_container.HasMember(name))
    {
        j_container.AddMember(rapidjson::StringRef(name), rapidjson::Value(j_type), m_json_doc.GetAllocator());
    }
    return j_container[name];
}

void JsonExporter::ExportNodesToJson(std::map<std::string, Node>& nodes, std::vector<NodeGroup>& groups)
{
    rapidjson::Value& j_module = this->GetModuleJson();
    auto& j_alloc = m_json_doc.GetAllocator();

    rapidjson::Value& j_softbody = this->GetOrCreateMember(j_module, "softbody");

    // PRESETS (aka 'set_node_defaults' in truckfile)
    // We need to assign unique numbers to presets; let's create std::map<> of {instance ->ID} mappings.
    // TODO: implement properly in Editor; we can't rely on RigDef::NodeDefaults
    std::map<RigDef::NodeDefaults*, size_t> presets;
    for (auto& name_node_pair: nodes)
    {
        presets.insert(std::make_pair(name_node_pair.second.GetPreset(), presets.size()));
    }

    rapidjson::Value& j_presets = this->GetOrCreateMember(j_softbody, "node_presets");

    for (auto& preset_pair: presets)
    {
        auto& preset = preset_pair.first;
        rapidjson::Value j_preset(rapidjson::kObjectType);

        j_preset.AddMember("load_weight", preset->load_weight, j_alloc);
        j_preset.AddMember("friction",    preset->friction,    j_alloc);
        j_preset.AddMember("volume",      preset->volume,      j_alloc);
        j_preset.AddMember("surface",     preset->surface,     j_alloc);

        j_preset.AddMember("option_n_mouse_grab"        , (preset->options & RigDef::Node::OPTION_n_MOUSE_GRAB        ), j_alloc);
        j_preset.AddMember("option_m_no_mouse_grab"     , (preset->options & RigDef::Node::OPTION_m_NO_MOUSE_GRAB     ), j_alloc);
        j_preset.AddMember("option_f_no_sparks"         , (preset->options & RigDef::Node::OPTION_f_NO_SPARKS         ), j_alloc);
        j_preset.AddMember("option_x_exhaust_point"     , (preset->options & RigDef::Node::OPTION_x_EXHAUST_POINT     ), j_alloc);
        j_preset.AddMember("option_y_exhaust_direction" , (preset->options & RigDef::Node::OPTION_y_EXHAUST_DIRECTION ), j_alloc);
        j_preset.AddMember("option_c_no_ground_contact" , (preset->options & RigDef::Node::OPTION_c_NO_GROUND_CONTACT ), j_alloc);
        j_preset.AddMember("option_h_hook_point"        , (preset->options & RigDef::Node::OPTION_h_HOOK_POINT        ), j_alloc);
        j_preset.AddMember("option_e_terrain_edit_point", (preset->options & RigDef::Node::OPTION_e_TERRAIN_EDIT_POINT), j_alloc);
        j_preset.AddMember("option_b_extra_buoyancy"    , (preset->options & RigDef::Node::OPTION_b_EXTRA_BUOYANCY    ), j_alloc);
        j_preset.AddMember("option_p_no_particles"      , (preset->options & RigDef::Node::OPTION_p_NO_PARTICLES      ), j_alloc);
        j_preset.AddMember("option_L_log"               , (preset->options & RigDef::Node::OPTION_L_LOG               ), j_alloc);
        j_preset.AddMember("option_l_load_weight"       , (preset->options & RigDef::Node::OPTION_l_LOAD_WEIGHT       ), j_alloc);

        j_presets.AddMember(rapidjson::Value(preset_pair.second), j_preset, j_alloc);
    }

    // GROUPS
    rapidjson::Value& j_groups = this->GetOrCreateMember(j_softbody, "node_groups");

    for (size_t i = 0; i < groups.size(); ++i)
    {
        rapidjson::Value j_grp(rapidjson::kObjectType);
        j_grp.AddMember("name", this->StrToJson(groups[i].reng_name), j_alloc);
        j_grp.AddMember("color", this->RgbaToJson(groups[i].reng_color), j_alloc);
        
        j_groups.AddMember(rapidjson::Value(i), j_grp, j_alloc);
    }

    // NODES
    rapidjson::Value& j_nodes = this->GetOrCreateMember(j_softbody, "nodes");

    for (auto& node_entry: nodes)
    {
        Node& node = node_entry.second;
        rapidjson::Value j_node(rapidjson::kObjectType);
        j_node.AddMember("position",          this->Vector3ToJson(node.GetDefinitionPosition()), j_alloc);
        j_node.AddMember("group_id",          node.GetEditorGroupId(),                j_alloc);
        j_node.AddMember("detacher_group_id", node.GetDefinitionDetacherGroup(),      j_alloc);
        j_node.AddMember("load_weight_set",   node.GetDefinitionLoadWeightActive(),   j_alloc);
        j_node.AddMember("load_weight",       node.GetDefinitionLoadWeight(),         j_alloc);
        j_node.AddMember("preset_id",         presets.find(node.GetPreset())->second, j_alloc);
        
        unsigned flags = node.GetDefinitionFlags();
        j_node.AddMember("option_n_mouse_grab"        , (flags & RigDef::Node::OPTION_n_MOUSE_GRAB        ), j_alloc);
        j_node.AddMember("option_m_no_mouse_grab"     , (flags & RigDef::Node::OPTION_m_NO_MOUSE_GRAB     ), j_alloc);
        j_node.AddMember("option_f_no_sparks"         , (flags & RigDef::Node::OPTION_f_NO_SPARKS         ), j_alloc);
        j_node.AddMember("option_x_exhaust_point"     , (flags & RigDef::Node::OPTION_x_EXHAUST_POINT     ), j_alloc);
        j_node.AddMember("option_y_exhaust_direction" , (flags & RigDef::Node::OPTION_y_EXHAUST_DIRECTION ), j_alloc);
        j_node.AddMember("option_c_no_ground_contact" , (flags & RigDef::Node::OPTION_c_NO_GROUND_CONTACT ), j_alloc);
        j_node.AddMember("option_h_hook_point"        , (flags & RigDef::Node::OPTION_h_HOOK_POINT        ), j_alloc);
        j_node.AddMember("option_e_terrain_edit_point", (flags & RigDef::Node::OPTION_e_TERRAIN_EDIT_POINT), j_alloc);
        j_node.AddMember("option_b_extra_buoyancy"    , (flags & RigDef::Node::OPTION_b_EXTRA_BUOYANCY    ), j_alloc);
        j_node.AddMember("option_p_no_particles"      , (flags & RigDef::Node::OPTION_p_NO_PARTICLES      ), j_alloc);
        j_node.AddMember("option_L_log"               , (flags & RigDef::Node::OPTION_L_LOG               ), j_alloc);
        j_node.AddMember("option_l_load_weight"       , (flags & RigDef::Node::OPTION_l_LOAD_WEIGHT       ), j_alloc);

        j_nodes.AddMember(this->NodeToJson(node), j_node, j_alloc);
    }
}

void JsonExporter::AddBeamPreset(RigDef::BeamDefaults* beam_preset)
{
    m_beam_preset_map.insert(std::make_pair(beam_preset, m_beam_preset_map.size()));
}

void JsonExporter::ExportBeamsToJson(std::list<Beam>& beams, std::vector<BeamGroup>& groups)
{
    rapidjson::Value& j_module = this->GetModuleJson();
    auto& j_alloc = m_json_doc.GetAllocator();

    // GROUPS
    rapidjson::Value& j_beam_groups = this->GetOrCreateMember(j_module, "beam_groups");
    for (size_t i = 0; i < groups.size(); ++i)
    {
        rapidjson::Value j_grp(rapidjson::kObjectType);
        j_grp.AddMember("name", this->StrToJson(groups[i].rebg_name), j_alloc);
        j_grp.AddMember("color", this->RgbaToJson(groups[i].rebg_color), j_alloc);
        j_beam_groups.AddMember(rapidjson::Value(i), j_grp, j_alloc);
    }

    // PRESETS
    // We need list of unique presets with numeric IDs -> let's use std::map
    for (Beam& beam: beams)
    {
        m_beam_preset_map.insert(std::make_pair(beam.GetPreset(), m_beam_preset_map.size()));
    }
    rapidjson::Value& j_beam_presets = this->GetOrCreateMember(j_module, "beam_presets");
    for (auto& entry: m_beam_preset_map)
    {
        rapidjson::Value j_preset(rapidjson::kObjectType);

        j_preset.AddMember("spring",                  entry.first->springiness                         , j_alloc);
        j_preset.AddMember("damp",                    entry.first->damping_constant                    , j_alloc);
        j_preset.AddMember("deform_threshold",        entry.first->deformation_threshold               , j_alloc);
        j_preset.AddMember("break_threshold",         entry.first->breaking_threshold                  , j_alloc);
        j_preset.AddMember("visual_diameter",         entry.first->visual_beam_diameter                , j_alloc);
        j_preset.AddMember("material_name",           this->StrToJson(entry.first->beam_material_name) , j_alloc);
        j_preset.AddMember("plastic_deform",          entry.first->plastic_deform_coef                 , j_alloc);
        j_preset.AddMember("enable_adv_deform",       entry.first->_enable_advanced_deformation        , j_alloc);
        j_preset.AddMember("user_set_plastic_deform", entry.first->_is_plastic_deform_coef_user_defined, j_alloc);
        j_preset.AddMember("spring_scale",            entry.first->scale.springiness                   , j_alloc);
        j_preset.AddMember("damp_scale",              entry.first->scale.damping_constant              , j_alloc);
        j_preset.AddMember("deform_thresh_scale",     entry.first->scale.deformation_threshold_constant, j_alloc);
        j_preset.AddMember("break_thresh_scale",      entry.first->scale.breaking_threshold_constant   , j_alloc);

        j_beam_presets.AddMember(rapidjson::Value(entry.second), rapidjson::kObjectType, j_alloc);
    }
    
    // BEAMS
    rapidjson::Value& j_beams = this->GetOrCreateMember(j_module, "beams", rapidjson::kArrayType);
    for (Beam& beam : beams)
    {
        if (beam.GetType() == Beam::TYPE_CINECAM)
            continue; // Generated beam -> skip

        rapidjson::Value j_beam(rapidjson::kObjectType);
        j_beam.AddMember("node_a", this->NodeToJson(beam.GetNodeA()), j_alloc);
        j_beam.AddMember("node_b", this->NodeToJson(beam.GetNodeB()), j_alloc);

        // Type-specific fields
        if (beam.GetType() == Beam::TYPE_PLAIN)
        {
            RigDef::Beam* def = beam.GetDefinitionPlainBeam();
            j_beam.AddMember("type",                  "plain",                           j_alloc);
            j_beam.AddMember("extension_break_limit", def->extension_break_limit,        j_alloc);
            j_beam.AddMember("has_extens_beak_limit", def->_has_extension_break_limit,   j_alloc);
            j_beam.AddMember("detacher_group",        def->detacher_group,               j_alloc);
            j_beam.AddMember("group_id",              def->editor_group_id,              j_alloc);
            j_beam.AddMember("option_i_invisible",    def->HasFlag_i_Invisible(),        j_alloc);
            j_beam.AddMember("option_r_rope",         def->HasFlag_r_Rope     (),        j_alloc);
            j_beam.AddMember("option_s_support",      def->HasFlag_s_Support  (),        j_alloc);
        }
        else if (beam.GetType() == Beam::TYPE_COMMAND_HYDRO)
        {
            RigDef::Command2* def = beam.GetDefinitionCommand2();
            // TODO: std::shared_ptr<Inertia> inertia_defaults;
            // TODO: Inertia inertia;
            j_beam.AddMember("type", "command", j_alloc);
            j_beam.AddMember("shorten_rate",            def->shorten_rate           , j_alloc);
            j_beam.AddMember("lengthen_rate",           def->lengthen_rate          , j_alloc);
            j_beam.AddMember("max_contraction",         def->max_contraction        , j_alloc);
            j_beam.AddMember("max_extension",           def->max_extension          , j_alloc);
            j_beam.AddMember("contract_key",            def->contract_key           , j_alloc);
            j_beam.AddMember("extend_key",              def->extend_key             , j_alloc);
            j_beam.AddMember("description",             this->StrToJson(def->description), j_alloc);
            j_beam.AddMember("affect_engine",           def->affect_engine          , j_alloc);
            j_beam.AddMember("needs_engine",            def->needs_engine           , j_alloc);
            j_beam.AddMember("plays_sound",             def->plays_sound            , j_alloc);
            j_beam.AddMember("detacher_group",          def->detacher_group         , j_alloc);
            j_beam.AddMember("option_i_invisible",      def->option_i_invisible     , j_alloc);
            j_beam.AddMember("option_r_rope",           def->option_r_rope          , j_alloc);
            j_beam.AddMember("option_c_auto_center",    def->option_c_auto_center   , j_alloc);
            j_beam.AddMember("option_f_not_faster",     def->option_f_not_faster    , j_alloc);
            j_beam.AddMember("option_p_1press",         def->option_p_1press        , j_alloc);
            j_beam.AddMember("option_o_1press_center",  def->option_o_1press_center , j_alloc);
        }
        else if (beam.GetType() == Beam::TYPE_STEERING_HYDRO)
        {
            // TODO: Inertia!!
            RigDef::Hydro* def = beam.GetDefinitionHydro();
            j_beam.AddMember("type", "hydro",   j_alloc);
            j_beam.AddMember("extend_factor", def->lenghtening_factor,   j_alloc);
            j_beam.AddMember("options", this->StrToJson(def->options),   j_alloc);
            j_beam.AddMember("detacher_group", def->detacher_group,   j_alloc);
        }
        else if (beam.GetType() == Beam::TYPE_ROPE)
        {
            RigDef::Rope* def = beam.GetDefinitionRope();
            j_beam.AddMember("type", "rope",    j_alloc);
            j_beam.AddMember("invisible",      def->invisible,         j_alloc);
            j_beam.AddMember("detacher_group", def->detacher_group,    j_alloc);
        }
        else if (beam.GetType() == Beam::TYPE_SHOCK_ABSORBER)
        {
            RigDef::Shock* def = beam.GetDefinitionShock();
            j_beam.AddMember("type", "shock",   j_alloc);
            j_beam.AddMember("option_i_invisible",    def->HasOption_i_Invisible(),     j_alloc);
            j_beam.AddMember("option_L_active_left",  def->HasOption_L_ActiveLeft(),    j_alloc);
            j_beam.AddMember("option_R_active_right", def->HasOption_R_ActiveRight(),   j_alloc);
            j_beam.AddMember("option_m_metric",       def->HasOption_m_Metric(),        j_alloc);
            j_beam.AddMember("detacher_group",        def->detacher_group,              j_alloc);
            j_beam.AddMember("spring_rate",           def->spring_rate   ,              j_alloc);
            j_beam.AddMember("damping",               def->damping       ,              j_alloc);
            j_beam.AddMember("short_bound",           def->short_bound   ,              j_alloc);
            j_beam.AddMember("long_bound",            def->long_bound    ,              j_alloc);
            j_beam.AddMember("precompression",        def->precompression,              j_alloc);
        }
        else if (beam.GetType() == Beam::TYPE_SHOCK_ABSORBER_2)
        {
            RigDef::Shock2* def = beam.GetDefinitionShock2();
            j_beam.AddMember("type",                       "shock2",                        j_alloc);
            j_beam.AddMember("spring_in",                  def->spring_in                 , j_alloc);
            j_beam.AddMember("damp_in",                    def->damp_in                   , j_alloc);
            j_beam.AddMember("progress_factor_spring_in",  def->progress_factor_spring_in , j_alloc);
            j_beam.AddMember("progress_factor_damp_in",    def->progress_factor_damp_in   , j_alloc);
            j_beam.AddMember("spring_out",                 def->spring_out                , j_alloc);
            j_beam.AddMember("damp_out",                   def->damp_out                  , j_alloc);
            j_beam.AddMember("progress_factor_spring_out", def->progress_factor_spring_out, j_alloc);
            j_beam.AddMember("progress_factor_damp_out",   def->progress_factor_damp_out  , j_alloc);
            j_beam.AddMember("short_bound",                def->short_bound               , j_alloc);
            j_beam.AddMember("long_bound",                 def->long_bound                , j_alloc);
            j_beam.AddMember("precompression",             def->precompression            , j_alloc);
            j_beam.AddMember("detacher_group",             def->detacher_group            , j_alloc);

            j_beam.AddMember("option_i_invisible",  def->HasOption_i_Invisible(),      j_alloc);
            j_beam.AddMember("option_M_abs_metric", def->HasOption_M_AbsoluteMetric(), j_alloc);
            j_beam.AddMember("option_m_metric",     def->HasOption_m_Metric(),         j_alloc);
            j_beam.AddMember("option_s_soft_bump",  def->HasOption_s_SoftBumpBounds(), j_alloc);
        }
        else if (beam.GetType() == Beam::TYPE_TRIGGER)
        {
            j_beam.AddMember("type", "trigger", j_alloc);
            // TODO
        }

        j_beams.PushBack(j_beam, j_alloc);
    }
}

rapidjson::Value JsonExporter::RgbaToJson(Ogre::ColourValue color)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    rapidjson::Value j_color(rapidjson::kObjectType);
    j_color.AddMember("r", color.r, j_alloc);
    j_color.AddMember("g", color.g, j_alloc);
    j_color.AddMember("b", color.b, j_alloc);
    j_color.AddMember("a", color.a, j_alloc);
    return j_color;
}

rapidjson::Value JsonExporter::Vector3ToJson(Ogre::Vector3 pos)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    rapidjson::Value j_pos(rapidjson::kObjectType);
    j_pos.AddMember("x", pos.x, j_alloc);
    j_pos.AddMember("y", pos.y, j_alloc);
    j_pos.AddMember("z", pos.z, j_alloc);
    return j_pos;
}

void JsonExporter::AddRigPropertiesJson(rapidjson::Value val)
{
    if (!m_json_doc.HasMember("general"))
    {
        m_json_doc.AddMember("general", val, m_json_doc.GetAllocator());
    }
    else
    {
        m_json_doc["general"] = val;
    }
}

void JsonExporter::ExportAirbrakesToJson(std::vector<RigDef::Airbrake>&airbrakes)
{
    auto& j_module = this->GetModuleJson();
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_airbrakes = this->GetOrCreateMember(j_module, "airbrakes", rapidjson::kArrayType);
    for (auto& def: airbrakes)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);

        j_def.AddMember("reference_node"       , this->NodeToJson(def.reference_node), j_alloc); // Node::Ref
        j_def.AddMember("x_axis_node"          , this->NodeToJson(def.x_axis_node)   , j_alloc); // Node::Ref
        j_def.AddMember("y_axis_node"          , this->NodeToJson(def.y_axis_node)   , j_alloc); // Node::Ref
        j_def.AddMember("aditional_node"       , this->NodeToJson(def.aditional_node), j_alloc); // Node::Ref
        j_def.AddMember("offset"               , this->Vector3ToJson(def.offset)     , j_alloc); // Ogre::Vector3
        j_def.AddMember("width"                , def.width                           , j_alloc); // float
        j_def.AddMember("height"               , def.height                          , j_alloc); // float
        j_def.AddMember("max_inclination_angle", def.max_inclination_angle           , j_alloc); // float
        j_def.AddMember("texcoord_x1"          , def.texcoord_x1                     , j_alloc); // float
        j_def.AddMember("texcoord_x2"          , def.texcoord_x2                     , j_alloc); // float
        j_def.AddMember("texcoord_y1"          , def.texcoord_y1                     , j_alloc); // float
        j_def.AddMember("texcoord_y2"          , def.texcoord_y2                     , j_alloc); // float
        j_def.AddMember("lift_coefficient"     , def.lift_coefficient                , j_alloc); // float

        j_airbrakes.PushBack(rapidjson::kObjectType, j_alloc);
    }
}

void JsonExporter::AppendAnimatorOptionJson(rapidjson::Value& j_array, const char* option, int param /*=-1*/)
{
    if (param != -1)
    {
        rapidjson::Value j_option(rapidjson::kObjectType);
        j_option.AddMember("option", rapidjson::StringRef(option), m_json_doc.GetAllocator());
        j_option.AddMember("param", param, m_json_doc.GetAllocator());
        j_array.PushBack(j_option, m_json_doc.GetAllocator());
    }
    else
    {
        j_array.PushBack(rapidjson::StringRef(option), m_json_doc.GetAllocator());
    }
}

void JsonExporter::ExportAnimatorsToJson(std::vector<RigDef::Animator>&animators)
{
    auto& j_module = this->GetModuleJson();
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_airbrakes = this->GetOrCreateMember(j_module, "animators", rapidjson::kArrayType);
    for (auto& def: animators)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);

        j_def.AddMember("node_a", this->NodeToJson(def.nodes[0]), j_alloc);
        j_def.AddMember("node_b", this->NodeToJson(def.nodes[1]), j_alloc);
        j_def.AddMember("extend_factor", def.lenghtening_factor, j_alloc);
        j_def.AddMember("short_limit", def.short_limit, j_alloc);
        j_def.AddMember("long_limit", def.long_limit, j_alloc);
        j_def.AddMember("detacher_group", def.detacher_group, j_alloc);

        rapidjson::Value j_options(rapidjson::kObjectType);

        // Options + param
        if (def.aero_animator.flags & RigDef::AeroAnimator::OPTION_THROTTLE)
            this->AppendAnimatorOptionJson(j_options, "aero_throttle", static_cast<int>(def.aero_animator.motor));

        if (def.aero_animator.flags & RigDef::AeroAnimator::OPTION_RPM     )
            this->AppendAnimatorOptionJson(j_options, "aero_rpm", static_cast<int>(def.aero_animator.motor));

        if (def.aero_animator.flags & RigDef::AeroAnimator::OPTION_TORQUE  )
            this->AppendAnimatorOptionJson(j_options, "aero_torque", static_cast<int>(def.aero_animator.motor));

        if (def.aero_animator.flags & RigDef::AeroAnimator::OPTION_PITCH   )
            this->AppendAnimatorOptionJson(j_options, "aero_pitch", static_cast<int>(def.aero_animator.motor));

        if (def.aero_animator.flags & RigDef::AeroAnimator::OPTION_STATUS  )
            this->AppendAnimatorOptionJson(j_options, "aero_status", static_cast<int>(def.aero_animator.motor));

        if (def.flags & RigDef::Animator::OPTION_ALTIMETER_100K)
            this->AppendAnimatorOptionJson(j_options, "altimeter", 100);

        if (def.flags & RigDef::Animator::OPTION_ALTIMETER_10K)
            this->AppendAnimatorOptionJson(j_options, "altimeter", 10);

        if (def.flags & RigDef::Animator::OPTION_ALTIMETER_1K)
            this->AppendAnimatorOptionJson(j_options, "altimeter", 1);

        // options without param
        if (def.flags & RigDef::Animator::OPTION_VISIBLE          ) this->AppendAnimatorOptionJson(j_options, "visible"          );
        if (def.flags & RigDef::Animator::OPTION_INVISIBLE        ) this->AppendAnimatorOptionJson(j_options, "invisible"        );
        if (def.flags & RigDef::Animator::OPTION_AIRSPEED         ) this->AppendAnimatorOptionJson(j_options, "airspeed"         );
        if (def.flags & RigDef::Animator::OPTION_VERTICAL_VELOCITY) this->AppendAnimatorOptionJson(j_options, "vertical_velocity");
        if (def.flags & RigDef::Animator::OPTION_ANGLE_OF_ATTACK  ) this->AppendAnimatorOptionJson(j_options, "angle_of_attack"  );
        if (def.flags & RigDef::Animator::OPTION_FLAP             ) this->AppendAnimatorOptionJson(j_options, "flap"             );
        if (def.flags & RigDef::Animator::OPTION_AIR_BRAKE        ) this->AppendAnimatorOptionJson(j_options, "air_brake"        );
        if (def.flags & RigDef::Animator::OPTION_ROLL             ) this->AppendAnimatorOptionJson(j_options, "roll"             );
        if (def.flags & RigDef::Animator::OPTION_PITCH            ) this->AppendAnimatorOptionJson(j_options, "pitch"            );
        if (def.flags & RigDef::Animator::OPTION_BRAKES           ) this->AppendAnimatorOptionJson(j_options, "brakes"           );
        if (def.flags & RigDef::Animator::OPTION_ACCEL            ) this->AppendAnimatorOptionJson(j_options, "accel"            );
        if (def.flags & RigDef::Animator::OPTION_CLUTCH           ) this->AppendAnimatorOptionJson(j_options, "clutch"           );
        if (def.flags & RigDef::Animator::OPTION_SPEEDO           ) this->AppendAnimatorOptionJson(j_options, "speedo"           );
        if (def.flags & RigDef::Animator::OPTION_TACHO            ) this->AppendAnimatorOptionJson(j_options, "tacho"            );
        if (def.flags & RigDef::Animator::OPTION_TURBO            ) this->AppendAnimatorOptionJson(j_options, "turbo"            );
        if (def.flags & RigDef::Animator::OPTION_PARKING          ) this->AppendAnimatorOptionJson(j_options, "parking"          );
        if (def.flags & RigDef::Animator::OPTION_SHIFT_LEFT_RIGHT ) this->AppendAnimatorOptionJson(j_options, "shift_left_right" );
        if (def.flags & RigDef::Animator::OPTION_SHIFT_BACK_FORTH ) this->AppendAnimatorOptionJson(j_options, "shift_back_forth" );
        if (def.flags & RigDef::Animator::OPTION_SEQUENTIAL_SHIFT ) this->AppendAnimatorOptionJson(j_options, "sequential_shift" );
        if (def.flags & RigDef::Animator::OPTION_GEAR_SELECT      ) this->AppendAnimatorOptionJson(j_options, "gear_select"      );
        if (def.flags & RigDef::Animator::OPTION_TORQUE           ) this->AppendAnimatorOptionJson(j_options, "torque"           );
        if (def.flags & RigDef::Animator::OPTION_DIFFLOCK         ) this->AppendAnimatorOptionJson(j_options, "difflock"         );
        if (def.flags & RigDef::Animator::OPTION_BOAT_RUDDER      ) this->AppendAnimatorOptionJson(j_options, "boat_rudder"      );
        if (def.flags & RigDef::Animator::OPTION_BOAT_THROTTLE    ) this->AppendAnimatorOptionJson(j_options, "boat_throttle"    );
        if (def.flags & RigDef::Animator::OPTION_SHORT_LIMIT      ) this->AppendAnimatorOptionJson(j_options, "short_limit"      );
        if (def.flags & RigDef::Animator::OPTION_LONG_LIMIT       ) this->AppendAnimatorOptionJson(j_options, "long_limit"       );

        j_def.AddMember("options", j_options, j_alloc);
        j_airbrakes.PushBack(j_def, j_alloc);
    }
}

void JsonExporter::ExportAntiLockBrakesToJson(std::shared_ptr<RigDef::AntiLockBrakes>&alb_def)
{
    if (alb_def.get() == nullptr)
        return; // Nothing to do

    auto& j_module = this->GetModuleJson();
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_airbrakes = this->GetOrCreateMember(j_module, "anti_lock_brakes", rapidjson::kArrayType);

    j_airbrakes.AddMember("regulation_force" , alb_def->regulation_force , j_alloc);
    j_airbrakes.AddMember("min_speed"        , alb_def->min_speed        , j_alloc);
    j_airbrakes.AddMember("pulse_per_sec"    , alb_def->pulse_per_sec    , j_alloc);
    j_airbrakes.AddMember("attr_is_on"       , alb_def->attr_is_on       , j_alloc);
    j_airbrakes.AddMember("attr_no_dashboard", alb_def->attr_no_dashboard, j_alloc);
    j_airbrakes.AddMember("attr_no_toggle"   , alb_def->attr_no_toggle   , j_alloc);
}

void JsonExporter::ExportAxlesToJson(std::vector<RigDef::Axle>&axles)
{
    auto& j_module = this->GetModuleJson();
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_axles = this->GetOrCreateMember(j_module, "axles", rapidjson::kArrayType);
    for (auto& def: axles)
    {
        auto& j_def = j_axles.PushBack(rapidjson::kObjectType, j_alloc);
        j_def.AddMember("wheel_a_node_a", this->NodeToJson(def.wheels[0][0]), j_alloc);
        j_def.AddMember("wheel_a_node_b", this->NodeToJson(def.wheels[0][1]), j_alloc);
        j_def.AddMember("wheel_b_node_a", this->NodeToJson(def.wheels[1][0]), j_alloc);
        j_def.AddMember("wheel_b_node_b", this->NodeToJson(def.wheels[1][1]), j_alloc);

        auto& j_options = j_def.AddMember("options", rapidjson::kArrayType, j_alloc);
        for (char c: def.options)
        {
            switch (c)
            {
            case RigDef::Axle::OPTION_o_OPEN:   j_options.PushBack("open"  , j_alloc); break;
            case RigDef::Axle::OPTION_l_LOCKED: j_options.PushBack("locked", j_alloc); break;
            case RigDef::Axle::OPTION_s_SPLIT:  j_options.PushBack("split" , j_alloc); break;
            default:;
            }
        }
    }
}

void JsonExporter::ExportBrakesToJson(std::shared_ptr<RigDef::Brakes>&brakes)
{
    if (brakes.get() == nullptr)
        return; // Nothing to do

    auto& j_module = this->GetModuleJson();
    auto& j_alloc = m_json_doc.GetAllocator();

    j_module.AddMember("brake_force",         brakes->default_braking_force, j_alloc);
    j_module.AddMember("parking_brake_force", brakes->parking_brake_force,   j_alloc);
}

void JsonExporter::ExportCamerasToJson(std::vector<RigDef::Camera>&cameras)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_cams = this->GetOrCreateMember(this->GetModuleJson(), "cameras", rapidjson::kArrayType);
    for (RigDef::Camera& def: cameras)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);
        j_def.AddMember("center_node", this->NodeToJson(def.center_node), j_alloc);
        j_def.AddMember("back_node",   this->NodeToJson(def.back_node),   j_alloc);
        j_def.AddMember("left_node",   this->NodeToJson(def.left_node),   j_alloc);

        j_cams.PushBack(j_def, j_alloc);
    }
}

void JsonExporter::ExportCameraRailsToJson(std::vector<RigDef::CameraRail>&camera_rails)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_camrails = this->GetOrCreateMember(this->GetModuleJson(), "camera_rails", rapidjson::kArrayType);
    for (RigDef::CameraRail& def: camera_rails)
    {
        rapidjson::Value j_nodes(rapidjson::kArrayType);
        this->NodeRefArrayToJson(j_nodes, def.nodes);
        j_camrails.PushBack(j_nodes, j_alloc);
    }
}

void JsonExporter::NodeRefArrayToJson(rapidjson::Value& j_dst, std::vector<RigDef::Node::Ref>& nodes)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    for (RigDef::Node::Ref& node: nodes)
    {
        j_dst.PushBack(rapidjson::StringRef(node.Str().c_str()), j_alloc);
    }
}

void JsonExporter::ExportCollisionBoxesToJson(std::vector<RigDef::CollisionBox>&collision_boxes)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_colboxes = this->GetOrCreateMember(this->GetModuleJson(), "collision_boxes", rapidjson::kArrayType);
    for (RigDef::CollisionBox& def: collision_boxes)
    {
        rapidjson::Value j_nodes(rapidjson::kArrayType);
        this->NodeRefArrayToJson(j_nodes, def.nodes);
        j_colboxes.PushBack(j_nodes, j_alloc);
    }
}

void JsonExporter::ExportCruiseControlToJson(std::shared_ptr<RigDef::CruiseControl>&cruise_control)
{
    if (cruise_control.get() == nullptr)
        return; // Nothing to do

    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_def = this->GetOrCreateMember(this->GetModuleJson(), "cruise_control", rapidjson::kArrayType);
    j_def.AddMember("min_speed", cruise_control->min_speed, j_alloc);
    j_def.AddMember("autobrake", cruise_control->autobrake, j_alloc);
}

void JsonExporter::ExportContactersToJson(std::vector<RigDef::Node::Ref>&contacters)
{
    rapidjson::Value j_nodes(rapidjson::kArrayType);
    this->NodeRefArrayToJson(j_nodes, contacters);
    this->GetModuleJson().AddMember("contacters", j_nodes, m_json_doc.GetAllocator());
}

void JsonExporter::ExportEngturbosToJson(std::shared_ptr<RigDef::Engturbo>&engturbo)
{
}

void JsonExporter::ExportExhaustsToJson(std::vector<RigDef::Exhaust>&exhausts)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_exhausts = this->GetOrCreateMember(this->GetModuleJson(), "exhausts", rapidjson::kArrayType);
    for (RigDef::Exhaust& def: exhausts)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);
        j_def.AddMember("reference_node",  this->NodeToJson(def.reference_node), j_alloc);
        j_def.AddMember("direction_node",  this->NodeToJson(def.direction_node), j_alloc);
        j_def.AddMember("material_name",   this->StrToJson(def.material_name),        j_alloc);

        j_exhausts.PushBack(j_def, j_alloc);
    }
}

void JsonExporter::ExportFixesToJson(std::vector<RigDef::Node::Ref>&fixes)
{
    rapidjson::Value j_nodes(rapidjson::kArrayType);
    this->NodeRefArrayToJson(j_nodes, fixes);
    this->GetModuleJson().AddMember("fixes", j_nodes, m_json_doc.GetAllocator());
}

void JsonExporter::ExportFusedragsToJson(std::vector<RigDef::Fusedrag>&fusedrag)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_fuselist = this->GetOrCreateMember(this->GetModuleJson(), "fusedrag", rapidjson::kArrayType);
    for (RigDef::Fusedrag& def: fusedrag)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);
        j_def.AddMember("autocalc",     def.autocalc,          j_alloc); // bool
        j_def.AddMember("front_node",   this->NodeToJson(def.front_node),  j_alloc); // Node::Ref
        j_def.AddMember("rear_node",    this->NodeToJson(def.rear_node ),   j_alloc); // Node::Ref
        j_def.AddMember("approx_width", def.approximate_width, j_alloc); // float
        j_def.AddMember("airfoil_name", this->StrToJson(def.airfoil_name),      j_alloc); // Ogre::String
        j_def.AddMember("area_coef",    def.area_coefficient,  j_alloc); // float

        j_fuselist.PushBack(j_def, j_alloc);
    }
}

void JsonExporter::ExportHooksToJson(std::vector<RigDef::Hook>&hooks)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_hooklist = this->GetOrCreateMember(this->GetModuleJson(), "hooks", rapidjson::kArrayType);
    for (RigDef::Hook& def: hooks)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);

        j_def.AddMember("flag_self_lock",     def.HasOptionSelfLock (),    j_alloc);
        j_def.AddMember("flag_auto_lock",     def.HasOptionAutoLock (),    j_alloc);
        j_def.AddMember("flag_no_disable",    def.HasOptionNoDisable(),    j_alloc);
        j_def.AddMember("flag_no_rope",       def.HasOptionNoRope   (),    j_alloc);
        j_def.AddMember("flag_visible",       def.HasOptionVisible  (),    j_alloc);
        j_def.AddMember("node",               this->NodeToJson(def.node)             , j_alloc);
        j_def.AddMember("option_hook_range",  def.option_hook_range      , j_alloc);
        j_def.AddMember("option_speed_coef",  def.option_speed_coef      , j_alloc);
        j_def.AddMember("option_max_force",   def.option_max_force       , j_alloc);
        j_def.AddMember("option_hookgroup",   def.option_hookgroup       , j_alloc);
        j_def.AddMember("option_lockgroup",   def.option_lockgroup       , j_alloc);
        j_def.AddMember("option_timer",       def.option_timer           , j_alloc);
        j_def.AddMember("option_min_range_m", def.option_min_range_meters, j_alloc);

        j_hooklist.PushBack(j_def, j_alloc);
    }
}

void JsonExporter::ExportLockgroupsToJson(std::vector<RigDef::Lockgroup>&lockgroups)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_list = this->GetOrCreateMember(this->GetModuleJson(), "lockgroups", rapidjson::kArrayType);
    for (RigDef::Lockgroup& def: lockgroups)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);
        j_def.AddMember("number", def.number, j_alloc);

        rapidjson::Value j_nodes(rapidjson::kArrayType);
        this->NodeRefArrayToJson(j_nodes, def.nodes);
        j_def.PushBack(j_nodes, j_alloc);

        j_list.PushBack(j_def, j_alloc);
    }
}

void JsonExporter::ExportManagedMatsToJson(std::vector<RigDef::ManagedMaterial>&managed_mats)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_list = this->GetOrCreateMember(this->GetModuleJson(), "managed_materials", rapidjson::kArrayType);
    for (RigDef::ManagedMaterial& def: managed_mats)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);

        j_def.AddMember("name",                this->StrToJson(def.name),                    j_alloc);
        j_def.AddMember("type_id",             static_cast<int>(def.type),  j_alloc);
        j_def.AddMember("is_doublesided",      def.options.double_sided,    j_alloc);
        j_def.AddMember("diffuse_map",         this->StrToJson(def.diffuse_map),             j_alloc);
        j_def.AddMember("damaged_diffuse_map", this->StrToJson(def.damaged_diffuse_map),     j_alloc);
        j_def.AddMember("specular_map",        this->StrToJson(def.specular_map),            j_alloc);

        j_list.PushBack(j_def, j_alloc);
    }
}

void JsonExporter::ExportMatFlareBindingsToJson(std::vector<RigDef::MaterialFlareBinding>& mat_flare_bindings)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_list = this->GetOrCreateMember(this->GetModuleJson(), "mat_flare_bindings", rapidjson::kArrayType);
    for (RigDef::MaterialFlareBinding& def: mat_flare_bindings)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);

        j_def.AddMember("flare_number",  def.flare_number,   j_alloc);
        j_def.AddMember("material_name", this->StrToJson(def.material_name),  j_alloc);

        j_list.PushBack(j_def, j_alloc);
    }
}

void JsonExporter::ExportNodeCollisionsToJson(std::vector<RigDef::NodeCollision>& node_collisions)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_list = this->GetOrCreateMember(this->GetModuleJson(), "node_collisions", rapidjson::kArrayType);
    for (RigDef::NodeCollision& def: node_collisions)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);

        j_def.AddMember("node",   this->NodeToJson(def.node),   j_alloc);
        j_def.AddMember("radius", def.radius,       j_alloc);

        j_list.PushBack(j_def, j_alloc);
    }
}

void JsonExporter::ExportParticlesToJson(std::vector<RigDef::Particle>& particles)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_list = this->GetOrCreateMember(this->GetModuleJson(), "particles", rapidjson::kArrayType);
    for (RigDef::Particle& def: particles)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);

        j_def.AddMember("emitter_node",     this->NodeToJson(def.emitter_node),   j_alloc);
        j_def.AddMember("reference_node",   this->NodeToJson(def.reference_node),   j_alloc);
        j_def.AddMember("particle_system_name", this->StrToJson(def.particle_system_name),       j_alloc);

        j_list.PushBack(j_def, j_alloc);
    }
}

void JsonExporter::ExportPistonpropsToJson(std::vector<RigDef::Pistonprop>& pistonprops)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_list = this->GetOrCreateMember(this->GetModuleJson(), "pistonprops", rapidjson::kArrayType);
    for (RigDef::Pistonprop& def: pistonprops)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);

        j_def.AddMember("reference_node",   this->NodeToJson(def.reference_node)    , j_alloc);
        j_def.AddMember("axis_node",        this->NodeToJson(def.axis_node)         , j_alloc);
        j_def.AddMember("blade_tip_node_1", this->NodeToJson(def.blade_tip_nodes[0]), j_alloc);
        j_def.AddMember("blade_tip_node_2", this->NodeToJson(def.blade_tip_nodes[1]), j_alloc);
        j_def.AddMember("blade_tip_node_3", this->NodeToJson(def.blade_tip_nodes[2]), j_alloc);
        j_def.AddMember("blade_tip_node_4", this->NodeToJson(def.blade_tip_nodes[3]), j_alloc);
        j_def.AddMember("couple_node",      this->NodeToJson(def.couple_node)       , j_alloc);
        j_def.AddMember("turbine_power_kW", def.turbine_power_kW                    , j_alloc);
        j_def.AddMember("pitch",            def.pitch                               , j_alloc);
        j_def.AddMember("airfoil",          this->StrToJson(def.airfoil)                             , j_alloc);

        j_list.PushBack(j_def, j_alloc);
    }
}

void JsonExporter::ExportPropsToJson(std::vector<RigDef::Prop>&props)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_list = this->GetOrCreateMember(this->GetModuleJson(), "props", rapidjson::kArrayType);
    for (RigDef::Prop& def: props)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);

        // Special - dashboard
        j_def.AddMember("special_dash/offset", this->Vector3ToJson(def.special_prop_dashboard.offset), j_alloc);
        j_def.AddMember("special_dash/has_offset", def.special_prop_dashboard._offset_is_set, j_alloc);
        j_def.AddMember("special_dash/rot_angle", def.special_prop_dashboard.rotation_angle, j_alloc);
        j_def.AddMember("special_dash/mesh_name", this->StrToJson(def.special_prop_dashboard.mesh_name), j_alloc);

        // Special - beacon
        j_def.AddMember("special_beacon/flare_material_name", this->StrToJson(def.special_prop_beacon.flare_material_name),     j_alloc);
        j_def.AddMember("special_beacon/color",               this->RgbaToJson(def.special_prop_beacon.color), j_alloc);

        j_def.AddMember("reference_node",       this->NodeToJson(def.reference_node),       j_alloc);
        j_def.AddMember("x_axis_node",          this->NodeToJson(def.x_axis_node),          j_alloc);
        j_def.AddMember("y_axis_node",          this->NodeToJson(def.y_axis_node),          j_alloc);
        j_def.AddMember("offset",               this->Vector3ToJson(def.offset),            j_alloc);
        j_def.AddMember("rotation",             this->Vector3ToJson(def.rotation),          j_alloc);
        j_def.AddMember("mesh_name",            this->StrToJson(def.mesh_name),             j_alloc);
        j_def.AddMember("camera_mode",          static_cast<int>(def.camera_settings.mode), j_alloc);
        j_def.AddMember("camera_cinecam_index", def.camera_settings.cinecam_index,          j_alloc);
        j_def.AddMember("special",              static_cast<int>(def.special),              j_alloc);

        rapidjson::Value j_animlist(rapidjson::kArrayType);
        for (auto& anim: def.animations)
        {
            rapidjson::Value j_anim(rapidjson::kArrayType);
            //j_def.AddMember("", def., j_alloc); -- TODO
        }
        j_def.AddMember("animations", j_animlist, j_alloc);

        j_list.PushBack(j_def, j_alloc);
    }
}

void JsonExporter::ExportRailGroupsToJson(std::vector<RigDef::RailGroup>&railgroups)
{
}

void JsonExporter::ExportRopablesToJson(std::vector<RigDef::Ropable>&ropables)
{
}

void JsonExporter::ExportRotatorsToJson(std::vector<RigDef::Rotator>&rotators)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_list = this->GetOrCreateMember(this->GetModuleJson(), "rotators", rapidjson::kArrayType);
    for (RigDef::Rotator& def: rotators)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);
        // TODO: inertia

        j_def.AddMember("rate",              def.rate,                                      j_alloc);
        j_def.AddMember("spin_left_key",     def.spin_left_key,                             j_alloc);
        j_def.AddMember("spin_right_key",    def.spin_right_key,                            j_alloc);
        j_def.AddMember("engine_coupling",   def.engine_coupling,                           j_alloc);
        j_def.AddMember("needs_engine",      def.needs_engine,                              j_alloc);

        // Nodes
        j_def.AddMember("axis_node_a",       this->NodeToJson(def.axis_nodes[0]),           j_alloc);
        j_def.AddMember("axis_node_b",       this->NodeToJson(def.axis_nodes[1]),           j_alloc);

        j_def.AddMember("base_plate_node_1", this->NodeToJson(def.base_plate_nodes[0]),     j_alloc);
        j_def.AddMember("base_plate_node_2", this->NodeToJson(def.base_plate_nodes[1]),     j_alloc);
        j_def.AddMember("base_plate_node_3", this->NodeToJson(def.base_plate_nodes[2]),     j_alloc);
        j_def.AddMember("base_plate_node_4", this->NodeToJson(def.base_plate_nodes[3]),     j_alloc);

        j_def.AddMember("rot_plate_node_1",  this->NodeToJson(def.rotating_plate_nodes[0]), j_alloc);
        j_def.AddMember("rot_plate_node_2",  this->NodeToJson(def.rotating_plate_nodes[1]), j_alloc);
        j_def.AddMember("rot_plate_node_3",  this->NodeToJson(def.rotating_plate_nodes[2]), j_alloc);
        j_def.AddMember("rot_plate_node_4",  this->NodeToJson(def.rotating_plate_nodes[3]), j_alloc);

        j_list.PushBack(j_def, j_alloc);
    }
}

void JsonExporter::ExportRotators2ToJson(std::vector<RigDef::Rotator2>&rotators_2)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_list = this->GetOrCreateMember(this->GetModuleJson(), "rotators_2", rapidjson::kArrayType);
    for (RigDef::Rotator2& def: rotators_2)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);
        // TODO: inertia

        j_def.AddMember("rate",              def.rate,                                      j_alloc);
        j_def.AddMember("spin_left_key",     def.spin_left_key,                             j_alloc);
        j_def.AddMember("spin_right_key",    def.spin_right_key,                            j_alloc);
        j_def.AddMember("engine_coupling",   def.engine_coupling,                           j_alloc);
        j_def.AddMember("needs_engine",      def.needs_engine,                              j_alloc);

        // Nodes
        j_def.AddMember("axis_node_a",       this->NodeToJson(def.axis_nodes[0]),           j_alloc);
        j_def.AddMember("axis_node_b",       this->NodeToJson(def.axis_nodes[1]),           j_alloc);

        j_def.AddMember("base_plate_node_1", this->NodeToJson(def.base_plate_nodes[0]),     j_alloc);
        j_def.AddMember("base_plate_node_2", this->NodeToJson(def.base_plate_nodes[1]),     j_alloc);
        j_def.AddMember("base_plate_node_3", this->NodeToJson(def.base_plate_nodes[2]),     j_alloc);
        j_def.AddMember("base_plate_node_4", this->NodeToJson(def.base_plate_nodes[3]),     j_alloc);

        j_def.AddMember("rot_plate_node_1",  this->NodeToJson(def.rotating_plate_nodes[0]), j_alloc);
        j_def.AddMember("rot_plate_node_2",  this->NodeToJson(def.rotating_plate_nodes[1]), j_alloc);
        j_def.AddMember("rot_plate_node_3",  this->NodeToJson(def.rotating_plate_nodes[2]), j_alloc);
        j_def.AddMember("rot_plate_node_4",  this->NodeToJson(def.rotating_plate_nodes[3]), j_alloc);

        // Rotators2 extras
        j_def.AddMember("rotating_force",    def.rotating_force,                            j_alloc);
        j_def.AddMember("tolerance",         def.tolerance,                                 j_alloc);
        j_def.AddMember("description",       this->StrToJson(def.description),              j_alloc);

        j_list.PushBack(j_def, j_alloc);
    }
}

void JsonExporter::ExportScrewpropsToJson(std::vector<RigDef::Screwprop>&screwprops)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_list = this->GetOrCreateMember(this->GetModuleJson(), "screwprops", rapidjson::kArrayType);
    for (RigDef::Screwprop& def: screwprops)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);

        j_def.AddMember("prop_node", this->NodeToJson(def.prop_node),   j_alloc);
        j_def.AddMember("back_node", this->NodeToJson(def.back_node),   j_alloc);
        j_def.AddMember("top_node",  this->NodeToJson(def.top_node ),   j_alloc);
        j_def.AddMember("power",     def.power,                         j_alloc);

        j_list.PushBack(j_def, j_alloc);
    }
}

void JsonExporter::NodeRangeArrayToJson(rapidjson::Value& j_dst_array, std::vector<RigDef::Node::Range>& ranges)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    for (RigDef::Node::Range& range: ranges)
    {
        if (range.IsRange())
        {
            rapidjson::Value j_entry(rapidjson::kArrayType);
            j_entry.PushBack(this->NodeToJson(range.start), j_alloc);
            j_entry.PushBack(this->NodeToJson(range.end),   j_alloc);
            j_dst_array.PushBack(j_entry, j_alloc);
        }
        else
        {
            j_dst_array.PushBack(this->NodeToJson(range.start), j_alloc);
        }
    }
}

void JsonExporter::ExportSlideNodesToJson(std::vector<RigDef::SlideNode>&slidenodes)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_list = this->GetOrCreateMember(this->GetModuleJson(), "slidenodes", rapidjson::kArrayType);
    for (RigDef::SlideNode& def: slidenodes)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);

        // Node ranges
        rapidjson::Value j_ranges(rapidjson::kArrayType);
        this->NodeRangeArrayToJson(j_ranges, def.rail_node_ranges);
        j_def.AddMember("rail_node_ranges", j_ranges, j_alloc);

        // Constraints
        j_def.AddMember("constraint_attach_all",       def.HasConstraint_a_AttachAll    (),   j_alloc);
        j_def.AddMember("constraint_attach_foreign",   def.HasConstraint_f_AttachForeign(),   j_alloc);
        j_def.AddMember("constraint_attach_self",      def.HasConstraint_s_AttachSelf   (),   j_alloc);
        j_def.AddMember("constraint_attach_none",      def.HasConstraint_n_AttachNone   (),   j_alloc);
        // Args
        j_def.AddMember("slide_node",              this->NodeToJson(def.slide_node)        ,   j_alloc);
        j_def.AddMember("spring_rate",             def.spring_rate             ,   j_alloc);
        j_def.AddMember("break_force",             def.break_force             ,   j_alloc);
        j_def.AddMember("tolerance",               def.tolerance               ,   j_alloc);
        j_def.AddMember("railgroup_id",            def.railgroup_id            ,   j_alloc);
        j_def.AddMember("_railgroup_id_set",       def._railgroup_id_set       ,   j_alloc);
        j_def.AddMember("attachment_rate",         def.attachment_rate         ,   j_alloc);
        j_def.AddMember("max_attachment_distance", def.max_attachment_distance ,   j_alloc);
        j_def.AddMember("_break_force_set",        def._break_force_set        ,   j_alloc);

        j_list.PushBack(j_def, j_alloc);
    }
}

void JsonExporter::ExportSlopeBrakeToJson(std::shared_ptr<RigDef::SlopeBrake>&slope_brake)
{
    if (slope_brake.get() == nullptr)
        return; // Nothing to do

    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_def = this->GetOrCreateMember(this->GetModuleJson(), "slope_brake", rapidjson::kObjectType);
    j_def.AddMember("regulating_force", slope_brake->regulating_force, j_alloc);
    j_def.AddMember("attach_angle",     slope_brake->attach_angle,     j_alloc);
    j_def.AddMember("release_angle",    slope_brake->release_angle,    j_alloc);
}

void JsonExporter::ExportSoundSourcesToJson(std::vector<RigDef::SoundSource>&soundsources)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_list = this->GetOrCreateMember(this->GetModuleJson(), "sound_sources", rapidjson::kArrayType);
    for (RigDef::SoundSource& def: soundsources)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);

        j_def.AddMember("node",   this->NodeToJson(def.node),   j_alloc);
        j_def.AddMember("sound_script_name",   this->StrToJson(def.sound_script_name),   j_alloc);

        j_list.PushBack(j_def, j_alloc);
    }
}

void JsonExporter::ExportSoundSources2ToJson(std::vector<RigDef::SoundSource2>&soundsources_2)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_list = this->GetOrCreateMember(this->GetModuleJson(), "sound_sources_2", rapidjson::kArrayType);
    for (RigDef::SoundSource2& def: soundsources_2)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);

        j_def.AddMember("mode_id",           def.mode,              j_alloc);
        j_def.AddMember("cinecam_index",     def.cinecam_index,     j_alloc);
        j_def.AddMember("node",              this->NodeToJson(def.node),        j_alloc);
        j_def.AddMember("sound_script_name", this->StrToJson(def.sound_script_name), j_alloc);

        j_list.PushBack(j_def, j_alloc);
    }
}

void JsonExporter::ExportSpeedLimiterToJson(RigDef::SpeedLimiter speed_limiter)
{
    if (!speed_limiter.is_enabled)
        return; // Nothing to do

    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_def = this->GetOrCreateMember(this->GetModuleJson(), "speed_limiter", rapidjson::kObjectType);
    j_def.AddMember("is_enabled", speed_limiter.is_enabled, j_alloc);
    j_def.AddMember("max_speed",     speed_limiter.max_speed,     j_alloc);
}

void JsonExporter::ExportSubmeshesToJson(std::vector<RigDef::Submesh>&submeshes)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_list = this->GetOrCreateMember(this->GetModuleJson(), "submeshes", rapidjson::kArrayType);
    for (RigDef::Submesh& def: submeshes)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);
            // Texcoords
        rapidjson::Value j_texcoord_array(rapidjson::kArrayType);
        for (RigDef::Texcoord& texcoord: def.texcoords)
        {

            rapidjson::Value j_texcoord(rapidjson::kObjectType);
            j_texcoord.AddMember("node",this->NodeToJson(texcoord.node),        j_alloc);
            j_def.AddMember("u", texcoord.u,                     j_alloc);
            j_def.AddMember("v", texcoord.v,                     j_alloc);

            j_texcoord_array.PushBack(j_texcoord, j_alloc);
        }
        j_def.AddMember("texcoords",   j_texcoord_array,     j_alloc);

                    // cabs
        rapidjson::Value j_cab_array(rapidjson::kArrayType);
        for (RigDef::Cab& cab: def.cab_triangles)
        {

            rapidjson::Value j_cab(rapidjson::kObjectType);
            j_cab.AddMember("node_a",this->NodeToJson(cab.nodes[0]),        j_alloc);
            j_cab.AddMember("node_b",this->NodeToJson(cab.nodes[1]),        j_alloc);
            j_cab.AddMember("node_c",this->NodeToJson(cab.nodes[2]),        j_alloc);

            j_cab.AddMember("option_c_contact", (cab.options & RigDef::Cab::OPTION_c_CONTACT          ),j_alloc);
            j_cab.AddMember("option_b_buoyant", (cab.options & RigDef::Cab::OPTION_b_BUOYANT          ),j_alloc);
            j_cab.AddMember("option_p_10xtougher", (cab.options & RigDef::Cab::OPTION_p_10xTOUGHER       ),j_alloc);
            j_cab.AddMember("option_u_invulnerable", (cab.options & RigDef::Cab::OPTION_u_INVULNERABLE     ),j_alloc);
            j_cab.AddMember("option_s_buoyant_no_drag", (cab.options & RigDef::Cab::OPTION_s_BUOYANT_NO_DRAG  ),j_alloc);
            j_cab.AddMember("option_r_buoyant_only_drag", (cab.options & RigDef::Cab::OPTION_r_BUOYANT_ONLY_DRAG),j_alloc);

            j_cab_array.PushBack(j_cab, j_alloc);
        }
        j_def.AddMember("cab_triangles",   j_cab_array,     j_alloc);

        j_def.AddMember("has_backmesh", def.backmesh,    j_alloc);

        j_list.PushBack(j_def, j_alloc);
    }
}

void JsonExporter::ExportTiesToJson(std::vector<RigDef::Tie>&ties)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_list = this->GetOrCreateMember(this->GetModuleJson(), "ties", rapidjson::kArrayType);
    for (RigDef::Tie& def: ties)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);

        j_def.AddMember("root_node",              this->NodeToJson(def.root_node),     j_alloc);

        j_def.AddMember("max_reach_length",       def.max_reach_length  ,              j_alloc);
        j_def.AddMember("auto_shorten_rate",      def.auto_shorten_rate ,              j_alloc);
        j_def.AddMember("min_length",             def.min_length        ,              j_alloc);
        j_def.AddMember("max_length",             def.max_length        ,              j_alloc);
        j_def.AddMember("is_invisible",           def.is_invisible      ,              j_alloc);
        j_def.AddMember("max_stress",             def.max_stress        ,              j_alloc);
        j_def.AddMember("detacher_group",         def.detacher_group    ,              j_alloc);
        j_def.AddMember("group",                  def.group             ,              j_alloc);
        j_def.AddMember("beam_preset", this->BeamPresetToJson(def.beam_defaults),      j_alloc);

        j_list.PushBack(j_def, j_alloc);
    }
}

rapidjson::Value JsonExporter::BeamPresetToJson(std::shared_ptr<RigDef::BeamDefaults>& preset_sptr)
{
    if (preset_sptr.get() == nullptr)
        return rapidjson::Value(); // Null

    auto& search_itor = m_beam_preset_map.find(preset_sptr.get());
    if (search_itor == m_beam_preset_map.end())
    {
        RoR::Log("ERROR - RigEditor::JsonExporter::BeamPresetToJson() - preset not found");
        return rapidjson::Value(); // Null
    }

    return rapidjson::Value(search_itor->second);
}

void JsonExporter::ExportTorqueCurveToJson(std::shared_ptr<RigDef::TorqueCurve>&torque_curve)
{
    if (torque_curve.get() == nullptr)
        return; // Nothing to do

    auto& j_module = this->GetModuleJson();
    auto& j_alloc = m_json_doc.GetAllocator();

    if (!torque_curve->predefined_func_name.empty())
    {
        // Predefined function is used
        j_module.AddMember("torque_curve", this->StrToJson(torque_curve->predefined_func_name), j_alloc);
    }
    else
    {
        // Samples are specified
        auto& j_samples = this->GetOrCreateMember(j_module, "torque_curve", rapidjson::kArrayType);
        for (auto& sample: torque_curve->samples)
        {
            rapidjson::Value j_sample(rapidjson::kArrayType);
            j_sample.AddMember("power", sample.power, j_alloc);
            j_sample.AddMember("torque_percent", sample.torque_percent, j_alloc);
            j_samples.PushBack(j_sample, j_alloc);
        }
    }
}

void JsonExporter::ExportTractionControlToJson(std::shared_ptr<RigDef::TractionControl>&tc_ptr)
{
    if (tc_ptr.get() == nullptr)
        return; // Nothing to do

    auto& j_module = this->GetModuleJson();
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_tc = this->GetOrCreateMember(j_module, "traction_control", rapidjson::kArrayType);

    j_tc.AddMember("regulation_force",  tc_ptr->regulation_force,  j_alloc);
    j_tc.AddMember("fade_speed",        tc_ptr->fade_speed,        j_alloc);
    j_tc.AddMember("pulse_per_sec",     tc_ptr->pulse_per_sec,     j_alloc);
    j_tc.AddMember("attr_is_on",        tc_ptr->attr_is_on,        j_alloc);
    j_tc.AddMember("attr_no_dashboard", tc_ptr->attr_no_dashboard, j_alloc);
    j_tc.AddMember("attr_no_toggle",    tc_ptr->attr_no_toggle,    j_alloc);
    j_tc.AddMember("wheel_slip",        tc_ptr->wheel_slip,        j_alloc);

}

void JsonExporter::ExportTurbojetsToJson(std::vector<RigDef::Turbojet>&turbojets)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_list = this->GetOrCreateMember(this->GetModuleJson(), "turbojets", rapidjson::kArrayType);
    for (RigDef::Turbojet& def: turbojets)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);

        j_def.AddMember("reference_node",  this->NodeToJson(def.front_node),  j_alloc);
        j_def.AddMember("axis_node",       this->NodeToJson(def.back_node),   j_alloc);
        j_def.AddMember("side_node",       this->NodeToJson(def.side_node),   j_alloc);


        j_def.AddMember("is_reversable",   def.is_reversable,                 j_alloc);
        j_def.AddMember("dry_thrust",      def.dry_thrust,                    j_alloc);
        j_def.AddMember("wet_thrust",      def.wet_thrust,                    j_alloc);
        j_def.AddMember("front_diameter",  def.front_diameter,                j_alloc);
        j_def.AddMember("back_diameter",   def.back_diameter,                 j_alloc);
        j_def.AddMember("nozzle_length",   def.nozzle_length,                 j_alloc);

        j_list.PushBack(j_def, j_alloc);
    }
}

void JsonExporter::ExportTurboprops2ToJson(std::vector<RigDef::Turboprop2>&turboprops_2)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_list = this->GetOrCreateMember(this->GetModuleJson(), "turboprops_2", rapidjson::kArrayType);
    for (RigDef::Turboprop2& def: turboprops_2)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);

        j_def.AddMember("reference_node",    this->NodeToJson(def.reference_node),      j_alloc);
        j_def.AddMember("axis_node",         this->NodeToJson(def.axis_node),           j_alloc);
        j_def.AddMember("blade_tip_node_1",  this->NodeToJson(def.blade_tip_nodes[0]),  j_alloc);
        j_def.AddMember("blade_tip_node_2",  this->NodeToJson(def.blade_tip_nodes[1]),  j_alloc);
        j_def.AddMember("blade_tip_node_3",  this->NodeToJson(def.blade_tip_nodes[2]),  j_alloc);
        j_def.AddMember("blade_tip_node_4",  this->NodeToJson(def.blade_tip_nodes[3]),  j_alloc);
        j_def.AddMember("couple_node",       this->NodeToJson(def.couple_node),         j_alloc);
        j_def.AddMember("turbine_power_kW",  def.turbine_power_kW,                      j_alloc);
        j_def.AddMember("_format_version",   def._format_version,                       j_alloc);
        j_def.AddMember("airfoil",           this->StrToJson(def.airfoil),              j_alloc);

        j_list.PushBack(j_def, j_alloc);
    }
}

void JsonExporter::ExportVideoCamerasToJson(std::vector<RigDef::VideoCamera>&videocameras)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_list = this->GetOrCreateMember(this->GetModuleJson(), "video_cameras", rapidjson::kArrayType);
    for (RigDef::VideoCamera& def: videocameras)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);
        // nodes
        j_def.AddMember("reference_node",        this->NodeToJson(def.reference_node),        j_alloc);
        j_def.AddMember("left_node",             this->NodeToJson(def.left_node),             j_alloc);
        j_def.AddMember("bottom_node",           this->NodeToJson(def.bottom_node),           j_alloc);
        j_def.AddMember("alt_reference_node",    this->NodeToJson(def.alt_reference_node),    j_alloc);
        j_def.AddMember("alt_orientation_node",  this->NodeToJson(def.alt_orientation_node),  j_alloc);
        // vector3
        j_def.AddMember("offset",                this->Vector3ToJson(def.offset),             j_alloc);
        j_def.AddMember("rotation",              this->Vector3ToJson(def.rotation),           j_alloc);
        // string
        j_def.AddMember("material_name",         this->StrToJson(def.material_name),          j_alloc);
        j_def.AddMember("camera_name",           this->StrToJson(def.camera_name),            j_alloc);
        // other
        j_def.AddMember("field_of_view",         def.field_of_view,                           j_alloc);
        j_def.AddMember("texture_width",         def.texture_width,                           j_alloc);
        j_def.AddMember("texture_height",        def.texture_height,                          j_alloc);
        j_def.AddMember("min_clip_distance",     def.min_clip_distance,                       j_alloc);
        j_def.AddMember("max_clip_distance",     def.max_clip_distance,                       j_alloc);
        j_def.AddMember("camera_role",           def.camera_role,                             j_alloc);
        j_def.AddMember("camera_mode",           def.camera_mode,                             j_alloc);

        j_list.PushBack(j_def, j_alloc);
    }
}

void JsonExporter::ExportWingsToJson(std::vector<RigDef::Wing>&wings)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_list = this->GetOrCreateMember(this->GetModuleJson(), "wings", rapidjson::kArrayType);
    for (RigDef::Wing& def: wings)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);

        rapidjson::Value j_control(static_cast<char>(def.control_surface));
        rapidjson::Value j_node_array(rapidjson::kArrayType);
        rapidjson::Value j_uv_array(rapidjson::kArrayType);
        for (size_t i = 0; i < 8; ++i)
        {
            j_node_array.PushBack(this->NodeToJson(def.nodes[i]), j_alloc);
            j_uv_array.PushBack(def.tex_coords[i], j_alloc);
        }
        j_def.AddMember("nodes",            j_node_array,                  j_alloc);
        j_def.AddMember("tex_coords",       j_uv_array,                    j_alloc);
        j_def.AddMember("control_surface",  j_control,                     j_alloc);
        j_def.AddMember("airfoil_name",     this->StrToJson(def.airfoil),  j_alloc);
        j_def.AddMember("chord_point",      def.chord_point,               j_alloc);
        j_def.AddMember("min_deflection",   def.min_deflection,            j_alloc);
        j_def.AddMember("max_deflection",   def.max_deflection,            j_alloc);
        j_def.AddMember("efficacy_coef",    def.efficacy_coef,             j_alloc);

        j_list.PushBack(j_def, j_alloc);
    }
}

void JsonExporter::ExportSubmeshGroundmodelToJson(std::string const & submesh_groundmodel_name)
{
    auto& j_module = this->GetModuleJson();
    j_module.AddMember("submesh_groundmodel_name", this->StrToJson(submesh_groundmodel_name), m_json_doc.GetAllocator());
}

void JsonExporter::SaveRigProjectJsonFile(MyGUI::UString const & out_path)
{
    FILE* file = nullptr;
    errno_t fopen_result = 0;
#ifdef _WIN32
    static const char* fopen_name = "_wfopen_s()";
    // Binary mode recommended by RapidJSON tutorial: http://rapidjson.org/md_doc_stream.html#FileWriteStream
    fopen_result = _wfopen_s(&file, out_path.asWStr_c_str(), L"wb");
#else
    static const char* fopen_name = "fopen_s()";
    fopen_s(&file, out_path.asUTF8_c_str(), "w");
#endif
    if ((fopen_result != 0) || (file == nullptr))
    {
        std::stringstream msg;
        msg << "[RoR|RigEditor] Failed to save JSON project file (path: "<< out_path << ")";
        if (fopen_result != 0)
        {
            msg<<" Tech details: function ["<<fopen_name<<"] returned ["<<fopen_result<<"]";
        }
        LOG(msg.str());
        return; // TODO: Notify the user!
    }

    char* buffer = new char[100000]; // 100kb
    rapidjson::FileWriteStream j_out_stream(file, buffer, sizeof(buffer));
    rapidjson::PrettyWriter<rapidjson::FileWriteStream> j_writer(j_out_stream);
    m_json_doc.Accept(j_writer);
    fclose(file);
    delete buffer;
}

} // namespace RigEditor
} // namespace RoR
