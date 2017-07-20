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
#include "RigEditor_CineCamera.h"
#include "RigEditor_Json.h"
#include "RigEditor_Node.h"
#include "RigEditor_LandVehicleWheel.h"
#include "RigEditor_FlexBodyWheel.h"
#include "RigEditor_MeshWheel.h"
#include "RigEditor_MeshWheel2.h"
#include "RigEditor_Rig.h"

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/filereadstream.h>

#include <sstream>

// Notes on RapidJSON
//    - The only way to add array items is 'PushBack()'. The 'operator[]' is only for reading.

namespace RoR {
namespace RigEditor {

JsonExporter::JsonExporter():
    m_json_doc(rapidjson::kObjectType),
    m_cur_module_name("_ROOT_") // TODO: Add constant
{
    this->GetOrCreateMember(m_json_doc, "modules");
}

void JsonExporter::SetModule (std::string const & module_name)
{
    m_cur_module_name = module_name;
}

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

void JsonExporter::SavePresetsToJson()
{
    rapidjson::Value& j_module = this->GetModuleJson();
    auto& j_alloc = m_json_doc.GetAllocator();

    // TODO: Inertia presets

    // Node presets (aka 'set_node_defaults' in truckfile)
    rapidjson::Value& j_presets = this->GetOrCreateMember(j_module, "node_presets");

    for (auto& preset_pair: m_node_presets)
    {
        auto& preset = preset_pair.first;
        rapidjson::Value j_preset(rapidjson::kObjectType);

        j_preset.AddMember("load_weight", preset->load_weight, j_alloc);
        j_preset.AddMember("friction",    preset->friction,    j_alloc);
        j_preset.AddMember("volume",      preset->volume,      j_alloc);
        j_preset.AddMember("surface",     preset->surface,     j_alloc);

        this->AddMemberBool(j_preset, "option_n_mouse_grab",         (0 != (preset->options & RigDef::Node::OPTION_n_MOUSE_GRAB        )));
        this->AddMemberBool(j_preset, "option_m_no_mouse_grab",      (0 != (preset->options & RigDef::Node::OPTION_m_NO_MOUSE_GRAB     )));
        this->AddMemberBool(j_preset, "option_f_no_sparks",          (0 != (preset->options & RigDef::Node::OPTION_f_NO_SPARKS         )));
        this->AddMemberBool(j_preset, "option_x_exhaust_point",      (0 != (preset->options & RigDef::Node::OPTION_x_EXHAUST_POINT     )));
        this->AddMemberBool(j_preset, "option_y_exhaust_direction",  (0 != (preset->options & RigDef::Node::OPTION_y_EXHAUST_DIRECTION )));
        this->AddMemberBool(j_preset, "option_c_no_ground_contact",  (0 != (preset->options & RigDef::Node::OPTION_c_NO_GROUND_CONTACT )));
        this->AddMemberBool(j_preset, "option_h_hook_point",         (0 != (preset->options & RigDef::Node::OPTION_h_HOOK_POINT        )));
        this->AddMemberBool(j_preset, "option_e_terrain_edit_point", (0 != (preset->options & RigDef::Node::OPTION_e_TERRAIN_EDIT_POINT)));
        this->AddMemberBool(j_preset, "option_b_extra_buoyancy",     (0 != (preset->options & RigDef::Node::OPTION_b_EXTRA_BUOYANCY    )));
        this->AddMemberBool(j_preset, "option_p_no_particles",       (0 != (preset->options & RigDef::Node::OPTION_p_NO_PARTICLES      )));
        this->AddMemberBool(j_preset, "option_L_log",                (0 != (preset->options & RigDef::Node::OPTION_L_LOG               )));
        this->AddMemberBool(j_preset, "option_l_load_weight",        (0 != (preset->options & RigDef::Node::OPTION_l_LOAD_WEIGHT       )));

        j_presets.AddMember(this->StrToJson(preset_pair.second), j_preset, j_alloc);
    }

    // Beam presets (aka 'set_beam_defaults' in truckfile)
    rapidjson::Value& j_beam_presets = this->GetOrCreateMember(j_module, "beam_presets");
    for (auto& entry: m_beam_presets)
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

        j_beam_presets.AddMember(this->StrToJson(entry.second), rapidjson::kObjectType, j_alloc);
    }
}

void JsonExporter::ExportNodesToJson(std::map<std::string, Node>& nodes, std::vector<NodeGroup>& groups)
{
    rapidjson::Value& j_module = this->GetModuleJson();
    auto& j_alloc = m_json_doc.GetAllocator();

    // GROUPS
    rapidjson::Value& j_groups = this->GetOrCreateMember(j_module, "node_groups");

    for (size_t i = 0; i < groups.size(); ++i)
    {
        rapidjson::Value j_grp(rapidjson::kObjectType);
        j_grp.AddMember("name", this->StrToJson(groups[i].name), j_alloc);
        j_grp.AddMember("color", this->RgbaToJson(groups[i].color), j_alloc);
        
        j_groups.AddMember(rapidjson::Value(i), j_grp, j_alloc);
    }

    // NODES
    rapidjson::Value& j_nodes = this->GetOrCreateMember(j_module, "nodes");

    for (auto& node_entry: nodes)
    {
        Node& node = node_entry.second;
        rapidjson::Value j_node(rapidjson::kObjectType);
        j_node.AddMember("position",          this->Vector3ToJson(node.GetDefinitionPosition()),     j_alloc);
        j_node.AddMember("group_id",          node.GetEditorGroupId(),                               j_alloc);
        j_node.AddMember("detacher_group_id", node.GetDefinitionDetacherGroup(),                     j_alloc);
        j_node.AddMember("load_weight_set",   node.GetDefinitionLoadWeightActive(),                  j_alloc);
        j_node.AddMember("load_weight",       node.GetDefinitionLoadWeight(),                        j_alloc);
        j_node.AddMember("preset_id",         this->NodePresetToJson(node_entry.second.GetPreset()), j_alloc);
        
        unsigned flags = node.GetDefinitionFlags();
        this->AddMemberBool(j_node, "option_n_mouse_grab",         (0 != (flags & RigDef::Node::OPTION_n_MOUSE_GRAB        )));
        this->AddMemberBool(j_node, "option_m_no_mouse_grab",      (0 != (flags & RigDef::Node::OPTION_m_NO_MOUSE_GRAB     )));
        this->AddMemberBool(j_node, "option_f_no_sparks",          (0 != (flags & RigDef::Node::OPTION_f_NO_SPARKS         )));
        this->AddMemberBool(j_node, "option_x_exhaust_point",      (0 != (flags & RigDef::Node::OPTION_x_EXHAUST_POINT     )));
        this->AddMemberBool(j_node, "option_y_exhaust_direction",  (0 != (flags & RigDef::Node::OPTION_y_EXHAUST_DIRECTION )));
        this->AddMemberBool(j_node, "option_c_no_ground_contact",  (0 != (flags & RigDef::Node::OPTION_c_NO_GROUND_CONTACT )));
        this->AddMemberBool(j_node, "option_h_hook_point",         (0 != (flags & RigDef::Node::OPTION_h_HOOK_POINT        )));
        this->AddMemberBool(j_node, "option_e_terrain_edit_point", (0 != (flags & RigDef::Node::OPTION_e_TERRAIN_EDIT_POINT)));
        this->AddMemberBool(j_node, "option_b_extra_buoyancy",     (0 != (flags & RigDef::Node::OPTION_b_EXTRA_BUOYANCY    )));
        this->AddMemberBool(j_node, "option_p_no_particles",       (0 != (flags & RigDef::Node::OPTION_p_NO_PARTICLES      )));
        this->AddMemberBool(j_node, "option_L_log",                (0 != (flags & RigDef::Node::OPTION_L_LOG               )));
        this->AddMemberBool(j_node, "option_l_load_weight",        (0 != (flags & RigDef::Node::OPTION_l_LOAD_WEIGHT       )));

        j_nodes.AddMember(this->NodeToJson(node), j_node, j_alloc);
    }
}

void JsonExporter::AddBeamPreset(RigDef::BeamDefaults* beam_preset)
{
    if (beam_preset == nullptr)
        return;

    std::stringstream buf;
    buf << "beam_preset_" << m_beam_presets.size();
    m_beam_presets.insert(std::make_pair(beam_preset, buf.str()));
}

void JsonExporter::AddNodePreset(RigDef::NodeDefaults* preset)
{
    if (preset == nullptr)
        return;

    std::stringstream buf;
    buf << "node_preset_" << m_node_presets.size();
    m_node_presets.insert(std::make_pair(preset, buf.str()));
}

void JsonExporter::ExportBeamsToJson(std::list<Beam>& beams, std::vector<BeamGroup>& groups)
{
    rapidjson::Value& j_module = this->GetModuleJson();
    auto& j_alloc = m_json_doc.GetAllocator();

    // GROUPS
    rapidjson::Value& j_beam_groups = this->GetOrCreateMember(j_module, "beam_groups", rapidjson::kArrayType);
    for (size_t i = 0; i < groups.size(); ++i)
    {
        rapidjson::Value j_grp(rapidjson::kObjectType);
        j_grp.AddMember("name", this->StrToJson(groups[i].rebg_name), j_alloc);
        j_grp.AddMember("color", this->RgbaToJson(groups[i].rebg_color), j_alloc);
        j_beam_groups.AddMember(rapidjson::Value(i), j_grp, j_alloc);
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
            j_beam.AddMember("preset_id",             this->BeamPresetToJson(def->defaults), j_alloc);
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
            j_beam.AddMember("preset_id",               this->BeamPresetToJson(def->beam_defaults), j_alloc);
        }
        else if (beam.GetType() == Beam::TYPE_STEERING_HYDRO)
        {
            // TODO: Inertia!!
            RigDef::Hydro* def = beam.GetDefinitionHydro();
            j_beam.AddMember("type", "hydro",   j_alloc);
            j_beam.AddMember("extend_factor", def->lenghtening_factor,   j_alloc);
            j_beam.AddMember("options", this->StrToJson(def->options),   j_alloc);
            j_beam.AddMember("detacher_group", def->detacher_group,   j_alloc);
            j_beam.AddMember("preset_id",               this->BeamPresetToJson(def->beam_defaults), j_alloc);
        }
        else if (beam.GetType() == Beam::TYPE_ROPE)
        {
            RigDef::Rope* def = beam.GetDefinitionRope();
            j_beam.AddMember("type", "rope",    j_alloc);
            j_beam.AddMember("invisible",      def->invisible,         j_alloc);
            j_beam.AddMember("detacher_group", def->detacher_group,    j_alloc);
            j_beam.AddMember("preset_id",               this->BeamPresetToJson(def->beam_defaults), j_alloc);
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
            j_beam.AddMember("preset_id",               this->BeamPresetToJson(def->beam_defaults), j_alloc);
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
            j_beam.AddMember("preset_id",               this->BeamPresetToJson(def->beam_defaults), j_alloc);
        }
        else if (beam.GetType() == Beam::TYPE_TRIGGER)
        {
            j_beam.AddMember("type", "trigger", j_alloc);
            RigDef::Trigger* def = beam.GetDefinitionTrigger();

            j_beam.AddMember("contraction_trigger_limit", def->contraction_trigger_limit , j_alloc); // float       
            j_beam.AddMember("expansion_trigger_limit"  , def->expansion_trigger_limit   , j_alloc); // float       
            j_beam.AddMember("options"                  , def->options                   , j_alloc); // unsigned int
            j_beam.AddMember("boundary_timer"           , def->boundary_timer            , j_alloc); // float       
            j_beam.AddMember("detacher_group"           , def->detacher_group            , j_alloc); // int         
            j_beam.AddMember("shortbound_trigger_action", def->shortbound_trigger_action , j_alloc); // int         
            j_beam.AddMember("longbound_trigger_action" , def->longbound_trigger_action  , j_alloc); // int         
            j_beam.AddMember("preset_id",               this->BeamPresetToJson(def->beam_defaults), j_alloc);
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

void JsonExporter::AddRigPropertiesJson(rapidjson::Value& val)
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
    auto& j_list = this->GetOrCreateMember(j_module, "animators", rapidjson::kArrayType);
    for (auto& def: animators)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);

        j_def.AddMember("node_a", this->NodeToJson(def.nodes[0]), j_alloc);
        j_def.AddMember("node_b", this->NodeToJson(def.nodes[1]), j_alloc);
        j_def.AddMember("extend_factor", def.lenghtening_factor, j_alloc);
        j_def.AddMember("short_limit", def.short_limit, j_alloc);
        j_def.AddMember("long_limit", def.long_limit, j_alloc);
        j_def.AddMember("detacher_group", def.detacher_group, j_alloc);
        j_def.AddMember("beam_preset", this->BeamPresetToJson(def.beam_defaults),      j_alloc);

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
        j_list.PushBack(j_def, j_alloc);
    }
}

void JsonExporter::ExportAntiLockBrakesToJson(std::shared_ptr<RigDef::AntiLockBrakes>&alb_def)
{
    if (alb_def.get() == nullptr)
        return; // Nothing to do

    auto& j_module = this->GetModuleJson();
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_def = this->GetOrCreateMember(j_module, "anti_lock_brakes", rapidjson::kObjectType);

    j_def.AddMember("regulation_force" , alb_def->regulation_force , j_alloc);
    j_def.AddMember("min_speed"        , alb_def->min_speed        , j_alloc);
    j_def.AddMember("pulse_per_sec"    , alb_def->pulse_per_sec    , j_alloc);
    j_def.AddMember("attr_is_on"       , alb_def->attr_is_on       , j_alloc);
    j_def.AddMember("attr_no_dashboard", alb_def->attr_no_dashboard, j_alloc);
    j_def.AddMember("attr_no_toggle"   , alb_def->attr_no_toggle   , j_alloc);
}

void JsonExporter::ExportAxlesToJson(std::vector<RigDef::Axle>&axles)
{
    auto& j_module = this->GetModuleJson();
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_axles = this->GetOrCreateMember(j_module, "axles", rapidjson::kArrayType);
    for (auto& def: axles)
    {
        rapidjson::Value j_def(rapidjson::kObjectType);
        j_def.AddMember("wheel_a_node_a", this->NodeToJson(def.wheels[0][0]), j_alloc);
        j_def.AddMember("wheel_a_node_b", this->NodeToJson(def.wheels[0][1]), j_alloc);
        j_def.AddMember("wheel_b_node_a", this->NodeToJson(def.wheels[1][0]), j_alloc);
        j_def.AddMember("wheel_b_node_b", this->NodeToJson(def.wheels[1][1]), j_alloc);

        rapidjson::Value j_options(rapidjson::kArrayType);
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

        j_def.AddMember("options", j_options, j_alloc);
        j_axles.PushBack(j_def, j_alloc);
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

void JsonExporter::ExportEngturboToJson(std::shared_ptr<RigDef::Engturbo>&def)
{
    if (def.get() == nullptr)
        return; // Nothing to do.

    auto& j_alloc = m_json_doc.GetAllocator();
    rapidjson::Value j_def(rapidjson::kObjectType);

    j_def.AddMember("version",    def->version,        j_alloc);
    j_def.AddMember("inertia",    def->tinertiaFactor, j_alloc);
    j_def.AddMember("num_turbos", def->nturbos,        j_alloc);
    j_def.AddMember("param01",    def->param1,         j_alloc);
    j_def.AddMember("param02",    def->param2,         j_alloc);
    j_def.AddMember("param03",    def->param3,         j_alloc);
    j_def.AddMember("param04",    def->param4,         j_alloc);
    j_def.AddMember("param05",    def->param5,         j_alloc);
    j_def.AddMember("param06",    def->param6,         j_alloc);
    j_def.AddMember("param07",    def->param7,         j_alloc);
    j_def.AddMember("param08",    def->param8,         j_alloc);
    j_def.AddMember("param09",    def->param9,         j_alloc);
    j_def.AddMember("param10",    def->param10,        j_alloc);
    j_def.AddMember("param11",    def->param11,        j_alloc);

    // Persist
    this->GetModuleJson().AddMember("engturbo", j_def, j_alloc);
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
        j_def.AddMember("nodes", j_nodes, j_alloc);

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

        j_def.AddMember("mode_id",           def.mode,                               j_alloc);
        j_def.AddMember("cinecam_index",     def.cinecam_index,                      j_alloc);
        j_def.AddMember("node",              this->NodeToJson(def.node),             j_alloc);
        j_def.AddMember("sound_script_name", this->StrToJson(def.sound_script_name), j_alloc);

        j_list.PushBack(j_def, j_alloc);
    }
}

void JsonExporter::ExportCinecamToJson(RigEditor::CineCamera& editor_cam)
{
    auto& def = editor_cam.m_definition;
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_list = this->GetOrCreateMember(this->GetModuleJson(), "cinecam", rapidjson::kArrayType);

    rapidjson::Value j_nodes(rapidjson::kArrayType);
    for (int i = 0; i < 8; ++i)
    {
        j_nodes.PushBack(this->NodeToJson(def.nodes[i]), j_alloc);
    }

    rapidjson::Value j_def(rapidjson::kObjectType);
    j_def.AddMember("position",        this->Vector3ToJson(def.position),              j_alloc);
    j_def.AddMember("spring",          def.spring,                                     j_alloc);
    j_def.AddMember("damping",         def.damping,                                    j_alloc);
    j_def.AddMember("beam_preset",     this->BeamPresetToJson(def.beam_defaults),      j_alloc);
    j_def.AddMember("node_preset",     this->NodePresetToJson(def.node_defaults),      j_alloc);

    j_list.PushBack(j_def, j_alloc);
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

    this->AddBeamPreset(preset_sptr.get());

    auto& search_itor = m_beam_presets.find(preset_sptr.get());
    if (search_itor == m_beam_presets.end())
    {
        RoR::Log("ERROR - RigEditor::JsonExporter::BeamPresetToJson() - preset not found");
        return rapidjson::Value(); // Null
    }

    return rapidjson::Value(rapidjson::StringRef(search_itor->second.c_str()));
}

rapidjson::Value JsonExporter::NodePresetToJson(RigDef::NodeDefaults* preset)
{
    if (preset == nullptr)
        return rapidjson::Value(); // Null

    this->AddNodePreset(preset);

    auto& search_itor = m_node_presets.find(preset);
    if (search_itor == m_node_presets.end())
    {
        LOG("ERROR - RigEditor::JsonExporter::NodePresetToJson() - preset not found");
        return rapidjson::Value(); // Null
    }

    return rapidjson::Value(rapidjson::StringRef(search_itor->second.c_str()));
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

void JsonExporter::AddWheel (LandVehicleWheel* wheel)
{
    auto& j_alloc = m_json_doc.GetAllocator();
    auto& j_wheels = this->GetOrCreateMember(this->GetModuleJson(), "wheels", rapidjson::kArrayType);
    rapidjson::Value j_def(rapidjson::kObjectType);

    switch (wheel->GetType())
    {
    case LandVehicleWheel::Type::TYPE_FLEXBODYWHEEL:
    {
        auto* fb_wheel = static_cast<FlexBodyWheel*>(wheel);
        RigDef::FlexBodyWheel& def = fb_wheel->GetDefinition();
        this->AddNodePreset(def.node_defaults.get());
        this->AddBeamPreset(def.beam_defaults.get());
        j_def.AddMember("type",              "FlexBodyWheel",                           j_alloc);

        // ----------- RigDef::BaseWheel -------------
        j_def.AddMember("width",             def.width,                                 j_alloc);
        j_def.AddMember("num_rays",          def.num_rays,                              j_alloc);
        j_def.AddMember("node_1",            this->NodeToJson(def.nodes[0]),            j_alloc);
        j_def.AddMember("node_2",            this->NodeToJson(def.nodes[1]),            j_alloc);
        j_def.AddMember("rigidity_node",     this->NodeToJson(def.rigidity_node),       j_alloc);
        j_def.AddMember("reference_arm_node",this->NodeToJson(def.reference_arm_node),  j_alloc);
        j_def.AddMember("braking_type_id",   static_cast<int>(def.braking),             j_alloc);
        j_def.AddMember("propulsion_type_id",static_cast<int>(def.propulsion),          j_alloc);
        j_def.AddMember("mass",              def.mass,                                  j_alloc);
        // ------------- RigDef::BaseWheel2 --------------
        j_def.AddMember("rim_radius",        def.rim_radius,                            j_alloc);
        j_def.AddMember("tyre_radius",       def.tyre_radius,                           j_alloc);
        j_def.AddMember("tyre_springiness",  def.tyre_springiness,                      j_alloc);
        j_def.AddMember("tyre_damping",      def.tyre_damping,                          j_alloc);
        // ------------- RigDef::FlexBodyWheel ------------
        j_def.AddMember("side",              static_cast<char>(def.side),               j_alloc);
        j_def.AddMember("rim_springiness",   def.rim_springiness,                       j_alloc);
        j_def.AddMember("rim_damping",       def.rim_damping,                           j_alloc);
        j_def.AddMember("rim_mesh_name",     this->StrToJson(def.rim_mesh_name ),       j_alloc);
        j_def.AddMember("tyre_mesh_name",    this->StrToJson(def.tyre_mesh_name),       j_alloc);

        j_wheels.PushBack(j_def, j_alloc);
        return;
    }

    case LandVehicleWheel::Type::TYPE_MESHWHEEL:
    case LandVehicleWheel::Type::TYPE_MESHWHEEL_2:
    {
        MeshWheel* mw = static_cast<MeshWheel*>(wheel);
        RigDef::MeshWheel& def = mw->GetDefinition();
        this->AddNodePreset(def.node_defaults.get());
        this->AddBeamPreset(def.beam_defaults.get());
        if (wheel->GetType() == LandVehicleWheel::Type::TYPE_MESHWHEEL)
            j_def.AddMember("type",          "MeshWheel",              j_alloc);
        else
            j_def.AddMember("type",          "MeshWheel2",             j_alloc);

        // ----------- RigDef::BaseWheel -------------
        j_def.AddMember("width",             def.width,                                 j_alloc);
        j_def.AddMember("num_rays",          def.num_rays,                              j_alloc);
        j_def.AddMember("node_1",            this->NodeToJson(def.nodes[0]),            j_alloc);
        j_def.AddMember("node_2",            this->NodeToJson(def.nodes[1]),            j_alloc);
        j_def.AddMember("rigidity_node",     this->NodeToJson(def.rigidity_node),       j_alloc);
        j_def.AddMember("reference_arm_node",this->NodeToJson(def.reference_arm_node),  j_alloc);
        j_def.AddMember("braking_type_id",   static_cast<int>(def.braking),             j_alloc);
        j_def.AddMember("propulsion_type_id",static_cast<int>(def.propulsion),          j_alloc);
        j_def.AddMember("mass",              def.mass,                                  j_alloc);
        // ------------- RigDef::MeshWheel (unified) --------------
        j_def.AddMember("rim_radius",        def.rim_radius,                            j_alloc);
        j_def.AddMember("tyre_radius",       def.tyre_radius,                           j_alloc);
        j_def.AddMember("spring",            def.spring,                                j_alloc);
        j_def.AddMember("damping",           def.damping,                               j_alloc);
        j_def.AddMember("side",              static_cast<char>(def.side),               j_alloc);
        j_def.AddMember("mesh_name",         this->StrToJson(def.mesh_name ),           j_alloc);
        j_def.AddMember("material_name",     this->StrToJson(def.material_name),        j_alloc);

        j_wheels.PushBack(j_def, j_alloc);
        return;
    }

    // TODO: Support 'wheels' and 'wheels2'!

    default:;
    }
    auto& j_list = this->GetOrCreateMember(this->GetModuleJson(), "meshwheels", rapidjson::kArrayType);
}

void JsonExporter::ExportEngineToJson (std::shared_ptr<RigDef::Engine>&    def)
{
    if (def.get() == nullptr)
        return; // Nothing to do.

    auto& j_alloc = m_json_doc.GetAllocator();
    rapidjson::Value j_def(rapidjson::kObjectType);

    j_def.AddMember("shift_down_rpm",     def->shift_down_rpm,     j_alloc);
    j_def.AddMember("shift_up_rpm",       def->shift_up_rpm,       j_alloc);
    j_def.AddMember("torque",             def->torque,             j_alloc);
    j_def.AddMember("global_gear_ratio",  def->global_gear_ratio,  j_alloc);
    j_def.AddMember("reverse_gear_ratio", def->reverse_gear_ratio, j_alloc);
    j_def.AddMember("neutral_gear_ratio", def->neutral_gear_ratio, j_alloc);

    // Gears
    rapidjson::Value j_gears(rapidjson::kArrayType);
    for (float gear: def->gear_ratios)
    {
        j_gears.PushBack(gear, j_alloc);
    }
    j_def.AddMember("gear_ratios", j_gears, j_alloc);

    // Persist
    this->GetModuleJson().AddMember("engine", j_def, j_alloc);
}

void JsonExporter::ExportEngoptionToJson (std::shared_ptr<RigDef::Engoption>& def)
{
    if (def.get() == nullptr)
        return; // Nothing to do.

    auto& j_alloc = m_json_doc.GetAllocator();
    rapidjson::Value j_def(rapidjson::kObjectType);

    j_def.AddMember("inertia",          def->inertia,                  j_alloc);
    j_def.AddMember("type_id",          static_cast<char>(def->type),  j_alloc);
    j_def.AddMember("clutch_force",     def->clutch_force,             j_alloc);
    j_def.AddMember("shift_time",       def->shift_time,               j_alloc);
    j_def.AddMember("clutch_time",      def->clutch_time,              j_alloc);
    j_def.AddMember("post_shift_time",  def->post_shift_time,          j_alloc);
    j_def.AddMember("idle_rpm",         def->idle_rpm,                 j_alloc);
    j_def.AddMember("stall_rpm",        def->stall_rpm,                j_alloc);
    j_def.AddMember("max_idle_mixture", def->max_idle_mixture,         j_alloc);
    j_def.AddMember("min_idle_mixture", def->min_idle_mixture,         j_alloc);

    this->GetModuleJson().AddMember("engoption", j_def, j_alloc);
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

rapidjson::Value JsonExporter::StrToJson(std::string const & s)
{
    if (s.empty())
        return rapidjson::Value(); // Null
    else
        return rapidjson::Value(rapidjson::StringRef(s.c_str()));
}

void JsonExporter::AddMemberBool(rapidjson::Value& j_container, const char* name, bool value)
{
    j_container.AddMember(rapidjson::StringRef(name), value, m_json_doc.GetAllocator());
}

// -------------------------------- Importer ------------------------------------

JsonImporter::JsonImporter():
    m_rig(nullptr),
    m_module_name("_ROOT_")
{
}

void JsonImporter::LoadRigProjectJson(MyGUI::UString const & src_path)
{
    if (!this->LoadJsonFile(src_path))
        return;

    if (!m_json_doc.HasMember("modules") || !m_json_doc["modules"].IsObject())
    {
        RoR::LogFormat("[RoR|RigEditor] Json document '%s' has no modules!", src_path.asUTF8_c_str());
        return;
    }

    rapidjson::Value& j_modules = m_json_doc["modules"];
    if (!j_modules.HasMember("_ROOT_") || j_modules["_ROOT_"].IsObject())
    {
        RoR::LogFormat("[RoR|RigEditor] Json document '%s' has no root module!", src_path.asUTF8_c_str());
        return;
    }

    // Root module
    this->LoadPresetsFromJson();

    // TODO: other modules
}

bool JsonImporter::LoadJsonFile(MyGUI::UString const & src_path)
{
    FILE* file = nullptr;
    errno_t fopen_result = 0;
#ifdef _WIN32
    static const char* fopen_name = "_wfopen_s()";
    // Binary mode recommended by RapidJSON tutorial: http://rapidjson.org/md_doc_stream.html#FileReadStream
    fopen_result = _wfopen_s(&file, src_path.asWStr_c_str(), L"wb");
#else
    static const char* fopen_name = "fopen_s()";
    fopen_s(&file, src_path.asUTF8_c_str(), "w");
#endif
    if ((fopen_result != 0) || (file == nullptr))
    {
        std::stringstream msg;
        msg << "[RoR|RigEditor] Failed to load JSON project file (path: "<< src_path << ")";
        if (fopen_result != 0)
        {
            msg<<" Tech details: function ["<<fopen_name<<"] returned ["<<fopen_result<<"]";
        }
        LOG(msg.str());
        return false; // TODO: Notify the user!
    }

    char readBuffer[65536];
    rapidjson::FileReadStream is(file, readBuffer, sizeof(readBuffer));

    m_json_doc.ParseStream(is);

    fclose(file); // TODO: check error
    return true;
}

RigDef::Node::Ref    JsonImporter::JsonToNodeRef(rapidjson::Value& j_val) // static helper
{
    const char* name = nullptr;
    int number = 0;
    if (j_val.IsString())
    {
        int flags = RigDef::Node::Ref::REGULAR_STATE_IS_NAMED | RigDef::Node::Ref::REGULAR_STATE_IS_VALID;
        return RigDef::Node::Ref(j_val.GetString(), 0, flags, 0);
    }
    return RigDef::Node::Ref(); // Defaults to invalid reference
}

void JsonImporter::SetCurrentModule(const char* module_name)
{
    if (!m_json_doc["modules"].HasMember(rapidjson::StringRef(module_name)))
        return;

    m_module_name = module_name;
}

rapidjson::Value& JsonImporter::GetModuleJson()
{
    const char* module_cstr = rapidjson::StringRef(m_module_name.c_str());
    return m_json_doc["modules"][module_cstr];
}

void JsonImporter::ImportEngineFromJson(std::shared_ptr<RigDef::Engine>&    def)
{
    rapidjson::Value& j_module = this->GetModuleJson();
    if (!j_module.HasMember("engine") || !j_module["engine"].IsObject())
        return;

    rapidjson::Value& j_def = j_module["engine"];
    def = std::make_shared<RigDef::Engine>();

    def->shift_down_rpm    = j_def["shift_down_rpm"]    .GetFloat();
    def->shift_up_rpm      = j_def["shift_up_rpm"]      .GetFloat();
    def->torque            = j_def["torque"]            .GetFloat();
    def->global_gear_ratio = j_def["global_gear_ratio"] .GetFloat();
    def->reverse_gear_ratio= j_def["reverse_gear_ratio"].GetFloat();
    def->neutral_gear_ratio= j_def["neutral_gear_ratio"].GetFloat();

    // Gears
    auto itor = j_def["gear_ratios"].Begin();
    auto endi = j_def["gear_ratios"].End();
    for (; itor != endi; ++itor)
    {
        def->gear_ratios.push_back(itor->GetFloat());
    }
}

void JsonImporter::ImportEngoptionFromJson(std::shared_ptr<RigDef::Engoption>& def)
{
    rapidjson::Value& j_module = this->GetModuleJson();
    if (!j_module.HasMember("engoption") || !j_module["engoption"].IsObject())
        return;

    rapidjson::Value& j_def = j_module["engoption"];
    def = std::make_shared<RigDef::Engoption>();

    def->inertia           = j_def["inertia"]            .GetFloat();
    def->type              = static_cast<RigDef::Engoption::EngineType>(j_def["type_id"].GetInt()); // TODO: validate!
    def->clutch_force      = j_def["clutch_force"]       .GetFloat();
    def->shift_time        = j_def["shift_time"]         .GetFloat();
    def->clutch_time       = j_def["clutch_time"]        .GetFloat();
    def->post_shift_time   = j_def["post_shift_time"]    .GetFloat();
    def->idle_rpm          = j_def["idle_rpm"]           .GetFloat();
    def->stall_rpm         = j_def["stall_rpm"]          .GetFloat();
    def->max_idle_mixture  = j_def["max_idle_mixture"]   .GetFloat();
    def->min_idle_mixture  = j_def["min_idle_mixture"]   .GetFloat();
}

void JsonImporter::ImportEngturboFromJson(std::shared_ptr<RigDef::Engturbo>& def)
{
    rapidjson::Value& j_module = this->GetModuleJson();
    if (!j_module.HasMember("engturbo") || !j_module["engturbo"].IsObject())
        return;

    rapidjson::Value& j_def = j_module["engturbo"];
    def = std::make_shared<RigDef::Engturbo>();

    def->version        =j_def["version"    ].GetInt();
    def->tinertiaFactor =j_def["inertia"    ].GetFloat();
    def->nturbos        =j_def["num_turbos" ].GetInt();
    def->param1         =j_def["param01"    ].GetFloat();
    def->param2         =j_def["param02"    ].GetFloat();
    def->param3         =j_def["param03"    ].GetFloat();
    def->param4         =j_def["param04"    ].GetFloat();
    def->param5         =j_def["param05"    ].GetFloat();
    def->param6         =j_def["param06"    ].GetFloat();
    def->param7         =j_def["param07"    ].GetFloat();
    def->param8         =j_def["param08"    ].GetFloat();
    def->param9         =j_def["param09"    ].GetFloat();
    def->param10        =j_def["param10"    ].GetFloat();
    def->param11        =j_def["param11"    ].GetFloat();
}

void JsonImporter::ImportTorqueCurveFromJson(std::shared_ptr<RigDef::TorqueCurve>&torque_curve)
{
    rapidjson::Value& j_module = this->GetModuleJson();
    if (!j_module.HasMember("torquecurve"))
        return;

    rapidjson::Value& j_curve = j_module["torquecurve"];

    if (j_curve.IsString)
    {
        torque_curve = std::make_shared<RigDef::TorqueCurve>();
        torque_curve->predefined_func_name = j_curve.GetString();
    }
    else if (j_curve.IsArray())
    {
        torque_curve = std::make_shared<RigDef::TorqueCurve>();

        for (auto itor = j_curve.Begin(); itor != j_curve.End(); ++itor)
        {
            RigDef::TorqueCurve::Sample sample;
            sample.power = (*itor)["power"].GetFloat();
            sample.torque_percent = (*itor)["torque_percent"].GetFloat();
            torque_curve->samples.push_back(sample);
        }
    }
    else
    {
        RoR::LogFormat("[RoR|RigEditor] Invalid 'torquecurve' JSON type.");
    }
}

void JsonImporter::LoadPresetsFromJson()
{
    rapidjson::Value& j_module = this->GetModuleJson();

    // TODO: Inertia presets

    // Node presets (aka 'set_node_defaults' in truckfile)
    if (j_module.HasMember("node_presets") || j_module["node_presets"].IsObject())
    {
        auto itor = j_module["node_presets"].MemberBegin();
        auto endi = j_module["node_presets"].MemberEnd();
        for (; itor != endi; ++itor)
        {
            auto preset = std::make_shared<RigDef::NodeDefaults>();
            rapidjson::Value& j_preset = itor->value;

            preset->load_weight = j_preset["load_weight"].GetFloat();
            preset->friction    = j_preset["friction"]   .GetFloat();
            preset->volume      = j_preset["volume"]     .GetFloat();
            preset->surface     = j_preset["surface"]    .GetFloat();

            if (j_preset["option_n_mouse_grab"]         .GetBool()) { preset->options |= RigDef::Node::OPTION_n_MOUSE_GRAB        ; }
            if (j_preset["option_m_no_mouse_grab"]      .GetBool()) { preset->options |= RigDef::Node::OPTION_m_NO_MOUSE_GRAB     ; }
            if (j_preset["option_f_no_sparks"]          .GetBool()) { preset->options |= RigDef::Node::OPTION_f_NO_SPARKS         ; }
            if (j_preset["option_x_exhaust_point"]      .GetBool()) { preset->options |= RigDef::Node::OPTION_x_EXHAUST_POINT     ; }
            if (j_preset["option_y_exhaust_direction"]  .GetBool()) { preset->options |= RigDef::Node::OPTION_y_EXHAUST_DIRECTION ; }
            if (j_preset["option_c_no_ground_contact"]  .GetBool()) { preset->options |= RigDef::Node::OPTION_c_NO_GROUND_CONTACT ; }
            if (j_preset["option_h_hook_point"]         .GetBool()) { preset->options |= RigDef::Node::OPTION_h_HOOK_POINT        ; }
            if (j_preset["option_e_terrain_edit_point"] .GetBool()) { preset->options |= RigDef::Node::OPTION_e_TERRAIN_EDIT_POINT; }
            if (j_preset["option_b_extra_buoyancy"]     .GetBool()) { preset->options |= RigDef::Node::OPTION_b_EXTRA_BUOYANCY    ; }
            if (j_preset["option_p_no_particles"]       .GetBool()) { preset->options |= RigDef::Node::OPTION_p_NO_PARTICLES      ; }
            if (j_preset["option_L_log"]                .GetBool()) { preset->options |= RigDef::Node::OPTION_L_LOG               ; }
            if (j_preset["option_l_load_weight"]        .GetBool()) { preset->options |= RigDef::Node::OPTION_l_LOAD_WEIGHT       ; }

            std::pair<std::string, std::shared_ptr<RigDef::NodeDefaults>> entry = std::make_pair(std::string(itor->name.GetString()), preset);
            m_node_presets.insert(entry);
        }
    }

    // Beam presets (aka 'set_beam_defaults' in truckfile)
    if (j_module.HasMember("beam_presets") || j_module["beam_presets"].IsObject())
    {
        auto itor = j_module["beam_presets"].MemberBegin();
        auto endi = j_module["beam_presets"].MemberEnd();
        for (; itor != endi; ++itor)
        {
            auto preset = std::make_shared<RigDef::BeamDefaults>();
            rapidjson::Value& j_preset = itor->value;

            preset->springiness                          = j_preset["spring"]                 .GetFloat();
            preset->damping_constant                     = j_preset["damp"]                   .GetFloat();
            preset->deformation_threshold                = j_preset["deform_threshold"]       .GetFloat();
            preset->breaking_threshold                   = j_preset["break_threshold"]        .GetFloat();
            preset->visual_beam_diameter                 = j_preset["visual_diameter"]        .GetFloat();
            preset->plastic_deform_coef                  = j_preset["plastic_deform"]         .GetFloat();
            preset->_enable_advanced_deformation         = j_preset["enable_adv_deform"]      .GetBool();
            preset->_is_plastic_deform_coef_user_defined = j_preset["user_set_plastic_deform"].GetBool();
            preset->scale.springiness                    = j_preset["spring_scale"]           .GetFloat();
            preset->scale.damping_constant               = j_preset["damp_scale"]             .GetFloat();
            preset->scale.deformation_threshold_constant = j_preset["deform_thresh_scale"]    .GetFloat();
            preset->scale.breaking_threshold_constant    = j_preset["break_thresh_scale"]     .GetFloat();
            preset->beam_material_name                   = j_preset["material_name"]          .GetString();

            m_beam_presets.insert(std::make_pair(itor->name.GetString(), preset));
        }
    }
}

Ogre::ColourValue JsonImporter::JsonToRgba(rapidjson::Value& j_color)
{
    if (!j_color.IsObject())
        return Ogre::ColourValue();

    return Ogre::ColourValue(j_color["r"].GetFloat(), j_color["g"].GetFloat(), j_color["b"].GetFloat(), j_color["a"].GetFloat());
}

Ogre::Vector3 JsonImporter::JsonToVector3(rapidjson::Value& j_val)
{
    if (!j_val.IsObject())
        return Ogre::Vector3();

    return Ogre::Vector3(j_val["x"].GetFloat(), j_val["y"].GetFloat(), j_val["z"].GetFloat());
}

std::shared_ptr<RigDef::NodeDefaults>  JsonImporter::ResolveNodePreset(rapidjson::Value& j_preset_id)
{
    if (!j_preset_id.IsString())
    {
        return nullptr;
    }

    auto found_itor = m_node_presets.find(j_preset_id.GetString());
    if (found_itor == m_node_presets.end())
    {
        return nullptr;
    }

    return found_itor->second;
}

std::shared_ptr<RigDef::BeamDefaults>  JsonImporter::ResolveBeamPreset(rapidjson::Value& j_preset_id)
{
    if (!j_preset_id.IsString())
    {
        return nullptr;
    }

    auto found_itor = m_beam_presets.find(j_preset_id.GetString());
    if (found_itor == m_beam_presets.end())
    {
        return nullptr;
    }

    return found_itor->second;
}

void JsonImporter::ImportNodesFromJson(std::map<std::string, Node>& nodes, std::vector<NodeGroup>& groups)
{
    // GROUPS
    rapidjson::Value& j_module = this->GetModuleJson();
    if (j_module.HasMember("node_groups") && j_module["node_groups"].IsArray())
    {
        auto itor = j_module["node_groups"].Begin();
        auto endi = j_module["node_groups"].End();
        for (; itor != endi; ++itor)
        {
            NodeGroup grp;
            grp.color = this->JsonToRgba((*itor)["color"]);
            grp.name = (*itor)["name"].GetString();
        }
    }

    if (j_module.HasMember("nodes") && j_module["nodes"].IsObject())
    {
        auto itor = j_module["nodes"].MemberBegin();
        auto endi = j_module["nodes"].MemberEnd();
        for (; itor != endi; ++itor)
        {
            rapidjson::Value& j_node = itor->value;
            Node node;
            node.m_nodegroup_id                              = j_node["group_id"].GetInt();
            node.m_definition.detacher_group                 = j_node["detacher_group_id"].GetInt();
            node.m_definition._has_load_weight_override      = j_node["load_weight_set"].GetBool();
            node.m_definition.load_weight_override           = j_node["load_weight"].GetFloat();
            node.m_definition.position                       = this->JsonToVector3(j_node["position"]);
            node.m_definition.node_defaults                  = this->ResolveNodePreset(j_node["preset_id"]);

            unsigned flags = 0u;
            if (j_node["option_n_mouse_grab"]        .GetBool()) { flags |= RigDef::Node::OPTION_n_MOUSE_GRAB        ; }
            if (j_node["option_m_no_mouse_grab"]     .GetBool()) { flags |= RigDef::Node::OPTION_m_NO_MOUSE_GRAB     ; }
            if (j_node["option_f_no_sparks"]         .GetBool()) { flags |= RigDef::Node::OPTION_f_NO_SPARKS         ; }
            if (j_node["option_x_exhaust_point"]     .GetBool()) { flags |= RigDef::Node::OPTION_x_EXHAUST_POINT     ; }
            if (j_node["option_y_exhaust_direction"] .GetBool()) { flags |= RigDef::Node::OPTION_y_EXHAUST_DIRECTION ; }
            if (j_node["option_c_no_ground_contact"] .GetBool()) { flags |= RigDef::Node::OPTION_c_NO_GROUND_CONTACT ; }
            if (j_node["option_h_hook_point"]        .GetBool()) { flags |= RigDef::Node::OPTION_h_HOOK_POINT        ; }
            if (j_node["option_e_terrain_edit_point"].GetBool()) { flags |= RigDef::Node::OPTION_e_TERRAIN_EDIT_POINT; }
            if (j_node["option_b_extra_buoyancy"]    .GetBool()) { flags |= RigDef::Node::OPTION_b_EXTRA_BUOYANCY    ; }
            if (j_node["option_p_no_particles"]      .GetBool()) { flags |= RigDef::Node::OPTION_p_NO_PARTICLES      ; }
            if (j_node["option_L_log"]               .GetBool()) { flags |= RigDef::Node::OPTION_L_LOG               ; }
            if (j_node["option_l_load_weight"]       .GetBool()) { flags |= RigDef::Node::OPTION_l_LOAD_WEIGHT       ; }
            node.m_definition.options = flags;

            nodes.insert(std::make_pair(itor->name.GetString(), node));
        }
    }
}

rapidjson::Value& JsonImporter::GetRigPropertiesJson()
{
    return m_json_doc["general"];
}

RigEditor::Node* JsonImporter::ResolveNode(rapidjson::Value& j_node_id)
{
    if (!j_node_id.IsString())
    {
        return nullptr; // Bad argument!
    }

    auto found_itor = m_rig->m_nodes.find(j_node_id.GetString());
    if (found_itor == m_rig->m_nodes.end())
    {
        return nullptr; // Not found!
    }

    return &found_itor->second;
}

bool JsonImporter::FetchArrayFromModule(rapidjson::Value::ValueIterator& start_itor, rapidjson::Value::ValueIterator& end_itor, const char* name)
{
    rapidjson::Value& j_module = this->GetModuleJson();
    if (!j_module.HasMember(name) || !j_module[name].IsArray())
    {
        return false;
    }

    start_itor = j_module[name].Begin();
    end_itor   = j_module[name].End();
    return true;
}

#define ITERATE_MODULE_ARRAY(_NAME_, _BODY_) {             \
    rapidjson::Value::ValueIterator itor, endi;            \
    if (!this->FetchArrayFromModule(itor, endi, (_NAME_))) \
    {                                                      \
        return;                                            \
    }                                                      \
    for (; itor != endi; ++itor)                           \
    {                                                      \
        auto& j_def = *itor;                               \
        { _BODY_ }                                         \
    }                                                      \
}

void JsonImporter::ImportBeamsFromJson(std::list<Beam>& beams, std::vector<BeamGroup>& groups)
{
    rapidjson::Value& j_module = this->GetModuleJson();
    if (!j_module.HasMember("beams") || !j_module["beams"].IsArray())
    {
        return;
    }

    auto itor = j_module["beams"].Begin();
    auto endi = j_module["beams"].End();
    for (; itor != endi; ++itor)
    {
        rapidjson::Value& j_beam = *itor;
        Node* node_a = this->ResolveNode(j_beam["node_a"]);
        Node* node_b = this->ResolveNode(j_beam["node_b"]);

        const char* type_str = j_beam["type"].GetString();
        if (Equals(type_str, "plain"))
        {
            auto* def = new RigDef::Beam();
            def->SetFlag_i_Invisible(j_beam["option_i_invisible"      ].GetBool())   ;
            def->SetFlag_r_Rope     (j_beam["option_r_rope"           ].GetBool())   ;
            def->SetFlag_s_Support  (j_beam["option_s_support"        ].GetBool())   ;

            def->extension_break_limit      = j_beam["extension_break_limit"   ].GetFloat();
            def->_has_extension_break_limit = j_beam["has_extens_beak_limit"   ].GetFloat();
            def->detacher_group             = j_beam["detacher_group"          ].GetInt();
            def->editor_group_id            = j_beam["group_id"                ].GetInt();
            def->defaults                   = this->ResolveBeamPreset(j_beam["preset_id"]);
            beams.push_back(Beam(def, Beam::Type::TYPE_PLAIN, node_a, node_b));
        }
        else if (Equals(type_str, "command")) // Unified 'command' and 'command2' in truckfile
        {
            auto* def = new RigDef::Command2(); // Unified 'command' and 'command2' in truckfile
            def->shorten_rate           = j_beam["shorten_rate"            ].GetFloat();
            def->lengthen_rate          = j_beam["lengthen_rate"           ].GetFloat();
            def->max_contraction        = j_beam["max_contraction"         ].GetFloat();
            def->max_extension          = j_beam["max_extension"           ].GetFloat();
            def->contract_key           = j_beam["contract_key"            ].GetUint();
            def->extend_key             = j_beam["extend_key"              ].GetUint();
            def->description            = j_beam["description"             ].GetString();
            def->affect_engine          = j_beam["affect_engine"           ].GetFloat();
            def->needs_engine           = j_beam["needs_engine"            ].GetBool();
            def->plays_sound            = j_beam["plays_sound"             ].GetFloat();
            def->detacher_group         = j_beam["detacher_group"          ].GetInt();
            def->option_i_invisible     = j_beam["option_i_invisible"      ].GetFloat();
            def->option_r_rope          = j_beam["option_r_rope"           ].GetFloat();
            def->option_c_auto_center   = j_beam["option_c_auto_center"    ].GetFloat();
            def->option_f_not_faster    = j_beam["option_f_not_faster"     ].GetFloat();
            def->option_p_1press        = j_beam["option_p_1press"         ].GetFloat();
            def->option_o_1press_center = j_beam["option_o_1press_center"  ].GetFloat();
            def->beam_defaults          = this->ResolveBeamPreset(j_beam["preset_id"]);
            beams.push_back(Beam(def, Beam::Type::TYPE_COMMAND_HYDRO, node_a, node_b));
        }
        else if (Equals(type_str, "hydro"))
        {
            auto* def = new RigDef::Hydro();
            def->options                = j_beam["options"                 ].GetString();
            def->detacher_group         = j_beam["detacher_group"          ].GetInt();
            def->lenghtening_factor     = j_beam["extend_factor"           ].GetFloat();
            def->beam_defaults          = this->ResolveBeamPreset(j_beam["preset_id"]);
            beams.push_back(Beam(def, Beam::Type::TYPE_COMMAND_HYDRO, node_a, node_b));
        }
        else if (Equals(type_str, "rope"))
        {
            auto* def = new RigDef::Rope();
            def->detacher_group         = j_beam["detacher_group"          ].GetInt();
            def->invisible              = j_beam["invisible"               ].GetBool();
            def->beam_defaults          = this->ResolveBeamPreset(j_beam["preset_id"]);
            beams.push_back(Beam(def, Beam::Type::TYPE_ROPE, node_a, node_b));
        }
        else if (Equals(type_str, "shock"))
        {
            auto* def = new RigDef::Shock();
            def->SetOption_i_Invisible  (j_beam["option_i_invisible"   ].GetBool());
            def->SetOption_L_ActiveLeft (j_beam["option_L_active_left" ].GetBool());
            def->SetOption_R_ActiveRight(j_beam["option_R_active_right"].GetBool());
            def->SetOption_m_Metric     (j_beam["option_m_metric"      ].GetBool());

            def->detacher_group            = j_beam["detacher_group"].GetInt();
            def->spring_rate               = j_beam["spring_rate"   ].GetFloat();
            def->damping                   = j_beam["damping"       ].GetFloat();
            def->short_bound               = j_beam["short_bound"   ].GetFloat();
            def->long_bound                = j_beam["long_bound"    ].GetFloat();
            def->precompression            = j_beam["precompression"].GetFloat();
            def->beam_defaults             = this->ResolveBeamPreset(j_beam["preset_id"]);
            beams.push_back(Beam(def, Beam::Type::TYPE_SHOCK_ABSORBER, node_a, node_b));
        }
        else if (Equals(type_str, "shock2"))
        {
            auto* def = new RigDef::Shock2();
            def->SetOption_i_Invisible      (j_beam["option_i_invisible" ].GetBool());
            def->SetOption_M_AbsoluteMetric (j_beam["option_M_abs_metric"].GetBool());
            def->SetOption_m_Metric         (j_beam["option_m_metric"    ].GetBool());
            def->SetOption_s_SoftBumpBounds (j_beam["option_s_soft_bump" ].GetBool());

            def->spring_in                  = j_beam["type"                      ].GetFloat();
            def->damp_in                    = j_beam["spring_in"                 ].GetFloat();
            def->progress_factor_spring_in  = j_beam["damp_in"                   ].GetFloat();
            def->progress_factor_damp_in    = j_beam["progress_factor_spring_in" ].GetFloat();
            def->spring_out                 = j_beam["progress_factor_damp_in"   ].GetFloat();
            def->damp_out                   = j_beam["spring_out"                ].GetFloat();
            def->progress_factor_spring_out = j_beam["damp_out"                  ].GetFloat();
            def->progress_factor_damp_out   = j_beam["progress_factor_spring_out"].GetFloat();
            def->short_bound                = j_beam["progress_factor_damp_out"  ].GetFloat();
            def->long_bound                 = j_beam["short_bound"               ].GetFloat();
            def->precompression             = j_beam["long_bound"                ].GetFloat();
            def->detacher_group             = j_beam["precompression"            ].GetFloat();
            def->detacher_group             = j_beam["detacher_group"].GetInt();
            def->beam_defaults              = this->ResolveBeamPreset(j_beam["preset_id"]);
            beams.push_back(Beam(def, Beam::Type::TYPE_SHOCK_ABSORBER_2, node_a, node_b));
        }
        else if (Equals(type_str, "trigger"))
        {
            auto* def = new RigDef::Trigger();
            def->contraction_trigger_limit = j_beam["contraction_trigger_limit"].GetFloat();
            def->expansion_trigger_limit   = j_beam["expansion_trigger_limit"  ].GetFloat();
            def->options                   = j_beam["options"                  ].GetUint();
            def->boundary_timer            = j_beam["boundary_timer"           ].GetFloat();
            def->detacher_group            = j_beam["detacher_group"           ].GetInt();
            def->shortbound_trigger_action = j_beam["shortbound_trigger_action"].GetInt();
            def->longbound_trigger_action  = j_beam["longbound_trigger_action" ].GetInt();
            def->beam_defaults             = this->ResolveBeamPreset(j_beam["preset_id"]);
            beams.push_back(Beam(def, Beam::Type::TYPE_TRIGGER, node_a, node_b));
        }
    }

    // Groups
    if (!j_module.HasMember("beam_groups") || !j_module["beam_groups"].IsArray())
    {
        return;
    }

    auto grp_itor = j_module["beam_groups"].Begin();
    auto grp_endi = j_module["beam_groups"].End();
    for (; grp_itor != grp_endi; ++grp_itor)
    {
        BeamGroup grp;
        rapidjson::Value& j_group = *itor;
        grp.rebg_color = this->JsonToRgba(j_group["color"]);
        grp.rebg_name = j_group["name"].GetString();
        groups.push_back(grp);
    }
}

void JsonImporter::ImportCinecamFromJson(std::list<CineCamera>& cams)
{
    ITERATE_MODULE_ARRAY("cinecam",
    {
        RigDef::Cinecam def;

        def.spring         = j_def["spring" ].GetFloat();
        def.damping        = j_def["damping"].GetFloat();
        def.position       = this->JsonToVector3(j_def["position"]);
        def.node_defaults  = this->ResolveNodePreset(j_def["node_preset"]);
        def.beam_defaults  = this->ResolveBeamPreset(j_def["beam_preset"]);

        cams.push_back(CineCamera(def));
    })
}

RigDef::Node::Ref JsonImporter::JsonToNodeRef(rapidjson::Value& j_node_id)
{
    if (!j_node_id.IsString() || j_node_id.GetStringLength() == 0)
        return RigDef::Node::Ref(); // Invalid ref

    unsigned node_flags = RigDef::Node::Ref::REGULAR_STATE_IS_NAMED | RigDef::Node::Ref::REGULAR_STATE_IS_VALID;
    return RigDef::Node::Ref(j_node_id.GetString(), 999999, node_flags, 999999);
}

void JsonImporter::ImportAirbrakesFromJson(std::vector<RigDef::Airbrake>& airbrakes)
{
    ITERATE_MODULE_ARRAY("airbrakes",
    {
        RigDef::Airbrake def;

        // Nodes
        def.reference_node  = this->JsonToNodeRef(j_def["reference_node"]);
        def.x_axis_node     = this->JsonToNodeRef(j_def["x_axis_node"   ]);
        def.y_axis_node     = this->JsonToNodeRef(j_def["y_axis_node"   ]);
        def.aditional_node  = this->JsonToNodeRef(j_def["aditional_node"]);
        // Vector3
        def.offset = this->JsonToVector3(j_def["offset"]);
        // Floats
        def.width                 = j_def["width"                ].GetFloat();
        def.height                = j_def["height"               ].GetFloat();
        def.max_inclination_angle = j_def["max_inclination_angle"].GetFloat();
        def.texcoord_x1           = j_def["texcoord_x1"          ].GetFloat();
        def.texcoord_x2           = j_def["texcoord_x2"          ].GetFloat();
        def.texcoord_y1           = j_def["texcoord_y1"          ].GetFloat();
        def.texcoord_y2           = j_def["texcoord_y2"          ].GetFloat();
        def.lift_coefficient      = j_def["lift_coefficient"     ].GetFloat();

        airbrakes.push_back(def);
    })
}

void JsonImporter::ImportWingsFromJson(std::vector<RigDef::Wing>& wings)
{
    ITERATE_MODULE_ARRAY("wings",
    {
        RigDef::Wing def;

        def.airfoil         = j_def["airfoil_name"].GetString();
        def.chord_point     = j_def["chord_point"   ].GetFloat();
        def.min_deflection  = j_def["min_deflection"].GetFloat();
        def.max_deflection  = j_def["max_deflection"].GetFloat();
        def.efficacy_coef   = j_def["efficacy_coef" ].GetFloat();
        def.control_surface = static_cast<RigDef::Wing::Control>(j_def["control_surface"].GetInt()); // TODO: is 'control_surface' a string or int?

        for (size_t i = 0; i < 8; ++i)
        {
            def.nodes[i] = this->JsonToNodeRef(j_def["nodes"][i]);
            def.tex_coords[i] = j_def["tex_coords"][i].GetFloat();
        }

        wings.push_back(def);
    })
}

void JsonImporter::ImportFusedragsFromJson(std::vector<RigDef::Fusedrag>& fusedrag)
{
    ITERATE_MODULE_ARRAY("fusedrag",
    {
        RigDef::Fusedrag def;

        def.approximate_width = j_def["approx_width"].GetFloat();
        def.area_coefficient  = j_def["area_coef"   ].GetFloat();
        def.autocalc          = j_def["autocalc"    ].GetBool();
        def.airfoil_name      = j_def["airfoil_name"].GetString();
        def.front_node        = this->JsonToNodeRef(j_def["front_node"]);
        def.rear_node         = this->JsonToNodeRef(j_def["rear_node" ]);

        fusedrag.push_back(def);
    })
}

void JsonImporter::ImportAnimatorsFromJson(std::vector<RigDef::Animator>& animators)
{
    ITERATE_MODULE_ARRAY("animators",
    {
        RigDef::Animator def;

        def.nodes[0] = this->JsonToNodeRef(j_def["node_a"]);
        def.nodes[1] = this->JsonToNodeRef(j_def["node_b"]);

        def.lenghtening_factor = j_def["extend_factor"].GetFloat();
        def.short_limit        = j_def["short_limit"  ].GetFloat();
        def.long_limit         = j_def["long_limit"   ].GetFloat();
        def.detacher_group     = j_def["detacher_group"].GetInt();
        def.beam_defaults      = this->ResolveBeamPreset(j_def["beam_preset"]);

        auto opt_itor = j_def["options"].Begin();
        auto opt_endi = j_def["options"].End();
        for (; opt_itor != opt_endi; ++opt_itor)
        {
            rapidjson::Value& entry = *opt_itor;
            if (entry.IsString())
            {
                const char* opt_name = entry.GetString();
                     if (Equals(opt_name, "visible"          )) { def.flags |= RigDef::Animator::OPTION_VISIBLE;          }
                else if (Equals(opt_name, "invisible"        )) { def.flags |= RigDef::Animator::OPTION_INVISIBLE;        }
                else if (Equals(opt_name, "airspeed"         )) { def.flags |= RigDef::Animator::OPTION_AIRSPEED;         }
                else if (Equals(opt_name, "vertical_velocity")) { def.flags |= RigDef::Animator::OPTION_VERTICAL_VELOCITY;}
                else if (Equals(opt_name, "angle_of_attack"  )) { def.flags |= RigDef::Animator::OPTION_ANGLE_OF_ATTACK;  }
                else if (Equals(opt_name, "flap"             )) { def.flags |= RigDef::Animator::OPTION_FLAP;             }
                else if (Equals(opt_name, "air_brake"        )) { def.flags |= RigDef::Animator::OPTION_AIR_BRAKE;        }
                else if (Equals(opt_name, "roll"             )) { def.flags |= RigDef::Animator::OPTION_ROLL;             }
                else if (Equals(opt_name, "pitch"            )) { def.flags |= RigDef::Animator::OPTION_PITCH;            }
                else if (Equals(opt_name, "brakes"           )) { def.flags |= RigDef::Animator::OPTION_BRAKES;           }
                else if (Equals(opt_name, "accel"            )) { def.flags |= RigDef::Animator::OPTION_ACCEL;            }
                else if (Equals(opt_name, "clutch"           )) { def.flags |= RigDef::Animator::OPTION_CLUTCH;           }
                else if (Equals(opt_name, "speedo"           )) { def.flags |= RigDef::Animator::OPTION_SPEEDO;           }
                else if (Equals(opt_name, "tacho"            )) { def.flags |= RigDef::Animator::OPTION_TACHO;            }
                else if (Equals(opt_name, "turbo"            )) { def.flags |= RigDef::Animator::OPTION_TURBO;            }
                else if (Equals(opt_name, "parking"          )) { def.flags |= RigDef::Animator::OPTION_PARKING;          }
                else if (Equals(opt_name, "shift_left_right" )) { def.flags |= RigDef::Animator::OPTION_SHIFT_LEFT_RIGHT; }
                else if (Equals(opt_name, "shift_back_forth" )) { def.flags |= RigDef::Animator::OPTION_SHIFT_BACK_FORTH; }
                else if (Equals(opt_name, "sequential_shift" )) { def.flags |= RigDef::Animator::OPTION_SEQUENTIAL_SHIFT; }
                else if (Equals(opt_name, "gear_select"      )) { def.flags |= RigDef::Animator::OPTION_GEAR_SELECT;      }
                else if (Equals(opt_name, "torque"           )) { def.flags |= RigDef::Animator::OPTION_TORQUE;           }
                else if (Equals(opt_name, "difflock"         )) { def.flags |= RigDef::Animator::OPTION_DIFFLOCK;         }
                else if (Equals(opt_name, "boat_rudder"      )) { def.flags |= RigDef::Animator::OPTION_BOAT_RUDDER;      }
                else if (Equals(opt_name, "boat_throttle"    )) { def.flags |= RigDef::Animator::OPTION_BOAT_THROTTLE;    }
                else if (Equals(opt_name, "short_limit"      )) { def.flags |= RigDef::Animator::OPTION_SHORT_LIMIT;      }
                else if (Equals(opt_name, "long_limit"       )) { def.flags |= RigDef::Animator::OPTION_LONG_LIMIT;       }
            }
            else if (entry.IsObject())
            {
                const char* opt_name = entry["option"].GetString();
                int         opt_param = entry["param"].GetInt();

                     if (Equals(opt_name, "aero_throttle")) { def.aero_animator.flags |= RigDef::AeroAnimator::OPTION_THROTTLE; def.aero_animator.motor = opt_param; }
                else if (Equals(opt_name, "aero_rpm"     )) { def.aero_animator.flags |= RigDef::AeroAnimator::OPTION_RPM;      def.aero_animator.motor = opt_param; }
                else if (Equals(opt_name, "aero_torque"  )) { def.aero_animator.flags |= RigDef::AeroAnimator::OPTION_TORQUE;   def.aero_animator.motor = opt_param; }
                else if (Equals(opt_name, "aero_pitch"   )) { def.aero_animator.flags |= RigDef::AeroAnimator::OPTION_PITCH;    def.aero_animator.motor = opt_param; }
                else if (Equals(opt_name, "aero_status"  )) { def.aero_animator.flags |= RigDef::AeroAnimator::OPTION_STATUS;   def.aero_animator.motor = opt_param; }

                else if (Equals(opt_name, "altimeter") && opt_param == 100) { def.flags |= RigDef::Animator::OPTION_ALTIMETER_100K; }
                else if (Equals(opt_name, "altimeter") && opt_param == 10)  { def.flags |= RigDef::Animator::OPTION_ALTIMETER_10K;  }
                else if (Equals(opt_name, "altimeter") && opt_param == 1)   { def.flags |= RigDef::Animator::OPTION_ALTIMETER_1K;   }
            }
            else
            {
                RoR::LogFormat("[RoR|RigEditor] Error loading JSON project: invalid animator option, type: %d", static_cast<int>(entry.GetType()));
            }
        }
        animators.push_back(def);
    })
}

void JsonImporter::ImportAntiLockBrakesFromJson(std::shared_ptr<RigDef::AntiLockBrakes>& alb_def)
{
    rapidjson::Value& j_module = this->GetModuleJson();
    if (!j_module.HasMember("anti_lock_brakes") || !j_module["anti_lock_brakes"].IsObject())
    {
        return; // AntiLockBrakes not defined
    }

    rapidjson::Value& j_def = j_module["anti_lock_brakes"];
    alb_def = std::make_shared<RigDef::AntiLockBrakes>();

    alb_def->regulation_force  = j_def["regulation_force" ].GetFloat();
    alb_def->min_speed         = j_def["min_speed"        ].GetUint();
    alb_def->pulse_per_sec     = j_def["pulse_per_sec"    ].GetFloat();
    alb_def->attr_is_on        = j_def["attr_is_on"       ].GetBool();
    alb_def->attr_no_dashboard = j_def["attr_no_dashboard"].GetBool();
    alb_def->attr_no_toggle    = j_def["attr_no_toggle"   ].GetBool();
}

void JsonImporter::ImportAxlesFromJson(std::vector<RigDef::Axle>&axles)
{
    ITERATE_MODULE_ARRAY("axles",
    {
        RigDef::Axle def;

        def.wheels[0][0] = this->JsonToNodeRef(j_def["wheel_a_node_a"]);
        def.wheels[0][1] = this->JsonToNodeRef(j_def["wheel_a_node_b"]);
        def.wheels[1][0] = this->JsonToNodeRef(j_def["wheel_b_node_a"]);
        def.wheels[1][1] = this->JsonToNodeRef(j_def["wheel_b_node_b"]);

        auto opt_itor = j_def["options"].Begin();
        auto opt_endi = j_def["options"].End();
        for (; opt_itor != opt_endi; ++opt_itor)
        {
                 if (Equals((*opt_itor).GetString(), "open"  )) { def.options.push_back(RigDef::Axle::OPTION_o_OPEN  ); }
            else if (Equals((*opt_itor).GetString(), "locked")) { def.options.push_back(RigDef::Axle::OPTION_l_LOCKED); }
            else if (Equals((*opt_itor).GetString(), "split" )) { def.options.push_back(RigDef::Axle::OPTION_s_SPLIT ); }
        }

        axles.push_back(def);
    })
}

void JsonImporter::ImportCamerasFromJson(std::vector<RigDef::Camera>& cameras)
{
    rapidjson::Value& j_module = this->GetModuleJson();
    if (!j_module.HasMember("cameras") || !j_module["cameras"].IsArray())
    {
        return;
    }

    auto cam_itor = j_module["axles"].Begin();
    auto cam_endi = j_module["axles"].End();
    for (; cam_itor != cam_endi; ++cam_itor)
    {
        auto& j_def = *cam_itor;
        RigDef::Camera def;
        def.center_node = this->JsonToNodeRef(j_def["center_node"]);
        def.back_node   = this->JsonToNodeRef(j_def["back_node"  ]);
        def.left_node   = this->JsonToNodeRef(j_def["left_node"  ]);
        cameras.push_back(def);
    }
}

void JsonImporter::ImportBrakesFromJson(std::shared_ptr<RigDef::Brakes>& brakes)
{
    rapidjson::Value& j_module = this->GetModuleJson();
    brakes = std::make_shared<RigDef::Brakes>();

    if (j_module.HasMember("brake_force"))
    {
        brakes->default_braking_force = j_module["brake_force"].GetFloat();
    }
    if (j_module.HasMember("parking_brake_force"))
    {
        brakes->parking_brake_force = j_module["parking_brake_force"].GetFloat();
    }
}

void JsonImporter::ImportCameraRailsFromJson(std::vector<RigDef::CameraRail>& camera_rails)
{
    rapidjson::Value& j_module = this->GetModuleJson();
    if (!j_module.HasMember("camera_rails") || !j_module["camera_rails"].IsArray())
    {
        return;
    }

    auto rail_itor = j_module["camera_rails"].Begin();
    auto rail_endi = j_module["camera_rails"].End();
    for (; rail_itor != rail_endi; ++rail_itor)
    {
        RigDef::CameraRail def;

        auto node_itor = rail_itor->Begin();
        auto node_endi = rail_itor->End();
        for (; node_itor != node_endi; ++node_itor)
        {
            def.nodes.push_back(this->JsonToNodeRef(*node_itor));
        }

        camera_rails.push_back(def);
    }
}

void JsonImporter::ImportCollisionBoxesFromJson(std::vector<RigDef::CollisionBox>&collision_boxes)
{
    rapidjson::Value& j_module = this->GetModuleJson();
    if (!j_module.HasMember("collision_boxes") || !j_module["collision_boxes"].IsArray())
    {
        return;
    }

    auto box_itor = j_module["collision_boxes"].Begin();
    auto box_endi = j_module["collision_boxes"].End();
    for (; box_itor != box_endi; ++box_itor)
    {
        RigDef::CollisionBox def;
        auto node_itor = box_itor->Begin();
        auto node_endi = box_itor->End();
        for (; node_itor != node_endi; ++node_itor)
        {
            def.nodes.push_back(this->JsonToNodeRef(*node_itor));
        }
        collision_boxes.push_back(def);
    }
}

void JsonImporter::ImportCruiseControlFromJson(std::shared_ptr<RigDef::CruiseControl>&cruise_control)
{
    rapidjson::Value& j_module = this->GetModuleJson();
    if (!j_module.HasMember("cruise_control") || !j_module["cruise_control"].IsObject())
    {
        return; // cruise control not defined
    }

    auto& j_def = j_module["cruise_control"];

    cruise_control = std::make_shared<RigDef::CruiseControl>();
    cruise_control->min_speed = j_def["min_speed"].GetFloat();
    cruise_control->autobrake = j_def["autobrake"].GetInt();
}

void JsonImporter::ImportContactersFromJson(std::vector<RigDef::Node::Ref>&contacters)
{
    ITERATE_MODULE_ARRAY("contacters",
    {
        contacters.push_back(this->JsonToNodeRef(j_def));
    })
}

void JsonImporter::ImportExhaustsFromJson(std::vector<RigDef::Exhaust>& exhausts)
{
    ITERATE_MODULE_ARRAY("exhausts",
    {
        RigDef::Exhaust def;
        def.reference_node = this->JsonToNodeRef(j_def["reference_node"]);
        def.direction_node = this->JsonToNodeRef(j_def["direction_node"]);
        def.material_name = j_def["material_name"].GetString();
        exhausts.push_back(def);
    })
}

void JsonImporter::ImportFixesFromJson(std::vector<RigDef::Node::Ref>& fixes)
{
    ITERATE_MODULE_ARRAY("fixes",
    {
        fixes.push_back(this->JsonToNodeRef(j_def));
    })
}

void JsonImporter::ImportHooksFromJson(std::vector<RigDef::Hook>&hooks)
{
    ITERATE_MODULE_ARRAY("hooks",
    {
        RigDef::Hook def;

        def.SetHasOptionSelfLock      (j_def["flag_self_lock"    ].GetBool());
        def.SetHasOptionAutoLock      (j_def["flag_auto_lock"    ].GetBool());
        def.SetHasOptionNoDisable     (j_def["flag_no_disable"   ].GetBool());
        def.SetHasOptionNoRope        (j_def["flag_no_rope"      ].GetBool());
        def.SetHasOptionVisible       (j_def["flag_visible"      ].GetBool());
        def.node                     = this->JsonToNodeRef(j_def["node"]);
        def.option_hook_range        = j_def["option_hook_range" ].GetFloat();
        def.option_speed_coef        = j_def["option_speed_coef" ].GetFloat();
        def.option_max_force         = j_def["option_max_force"  ].GetFloat();
        def.option_hookgroup         = j_def["option_hookgroup"  ].GetInt();
        def.option_lockgroup         = j_def["option_lockgroup"  ].GetInt();
        def.option_timer             = j_def["option_timer"      ].GetFloat();
        def.option_min_range_meters  = j_def["option_min_range_m"].GetFloat();

        hooks.push_back(def);
    })
}

void JsonImporter::ImportLockgroupsFromJson(std::vector<RigDef::Lockgroup>&lockgroups)
{
    rapidjson::Value::ValueIterator lg_itor, lg_endi;
    if (!this->FetchArrayFromModule(lg_itor, lg_endi, "lockgroups"))
        return;

    for (; lg_itor != lg_endi; ++lg_itor)
    {
        RigDef::Lockgroup def;
        auto& j_def = *lg_itor;

        def.number = j_def["number"].GetInt();
        auto node_itor = j_def["nodes"].Begin();
        auto node_endi = j_def["nodes"].End();
        for (; node_itor != node_endi; ++node_itor)
        {
            def.nodes.push_back(this->JsonToNodeRef(*node_itor));
        }
    }
}

void JsonImporter::ImportManagedMatsFromJson(std::vector<RigDef::ManagedMaterial>& managed_mats)
{
    ITERATE_MODULE_ARRAY("managed_materials",
    {
        RigDef::ManagedMaterial def;
        def.name                = j_def["name"].GetString();
        def.diffuse_map         = j_def["diffuse_map"        ].GetString();
        def.damaged_diffuse_map = j_def["damaged_diffuse_map"].GetString();
        def.specular_map        = j_def["specular_map"       ].GetString();
        def.options.double_sided= j_def["is_doublesided"     ].GetBool();
        def.type = static_cast<RigDef::ManagedMaterial::Type>(j_def["type_id"].GetInt()); // TODO: validate!!
        managed_mats.push_back(def);
    })
}

void JsonImporter::ImportMatFlareBindingsFromJson(std::vector<RigDef::MaterialFlareBinding>& mat_flare_bindings)
{
    ITERATE_MODULE_ARRAY("mat_flare_bindings",
    {
        RigDef::MaterialFlareBinding def;
        def.flare_number = j_def["flare_number"].GetUint();
        def.material_name= j_def["material_name"].GetString();
        mat_flare_bindings.push_back(def);
    })
}

void JsonImporter::ImportNodeCollisionsFromJson(std::vector<RigDef::NodeCollision>& node_collisions)
{
    ITERATE_MODULE_ARRAY("node_collisions",
    {
        RigDef::NodeCollision def;
        def.node = this->JsonToNodeRef(j_def["node"]);
        def.radius = j_def["radius"].GetFloat();
        node_collisions.push_back(def);
    })
}

void JsonImporter::ImportParticlesFromJson(std::vector<RigDef::Particle>& particles)
{
    ITERATE_MODULE_ARRAY("particles",
    {
        RigDef::Particle def;
        def.particle_system_name = j_def["particle_system_name"].GetString();
        def.emitter_node         = this->JsonToNodeRef(j_def["emitter_node"  ]);
        def.reference_node       = this->JsonToNodeRef(j_def["reference_node"]);
        particles.push_back(def);
    })
}

void JsonImporter::ImportPistonpropsFromJson(std::vector<RigDef::Pistonprop>& pistonprops)
{
    ITERATE_MODULE_ARRAY("pistonprops",
    {
        RigDef::Pistonprop def;

        def.reference_node     = this->JsonToNodeRef(j_def["reference_node"  ]);
        def.axis_node          = this->JsonToNodeRef(j_def["axis_node"       ]);
        def.blade_tip_nodes[0] = this->JsonToNodeRef(j_def["blade_tip_node_1"]);
        def.blade_tip_nodes[1] = this->JsonToNodeRef(j_def["blade_tip_node_2"]);
        def.blade_tip_nodes[2] = this->JsonToNodeRef(j_def["blade_tip_node_3"]);
        def.blade_tip_nodes[3] = this->JsonToNodeRef(j_def["blade_tip_node_4"]);
        def.couple_node        = this->JsonToNodeRef(j_def["couple_node"     ]);
        def.airfoil            = j_def["airfoil"         ].GetString();
        def.turbine_power_kW   = j_def["turbine_power_kW"].GetFloat();
        def.pitch              = j_def["pitch"           ].GetFloat();
    })
}

void JsonImporter::ImportPropsFromJson(std::vector<RigDef::Prop>&props)
{
    ITERATE_MODULE_ARRAY("pistonprops",
    {
        RigDef::Prop def;
        // Special prop - dashboard
        def.special_prop_dashboard.offset         = this->JsonToVector3(j_def["special_dash/offset"]);
        def.special_prop_dashboard.mesh_name      = j_def["special_dash/mesh_name"].GetString();
        def.special_prop_dashboard._offset_is_set = j_def["special_dash/has_offset"].GetBool();
        def.special_prop_dashboard.rotation_angle = j_def["special_dash/rot_angle"].GetFloat();

        // Special - beacon
        def.special_prop_beacon.flare_material_name = j_def["special_beacon/flare_material_name"].GetString();
        def.special_prop_beacon.color               = this->JsonToRgba(j_def["special_beacon/color"]);

        // Common
        def.reference_node                 = this->JsonToNodeRef(j_def["reference_node"]);
        def.x_axis_node                    = this->JsonToNodeRef(j_def["x_axis_node"   ]);
        def.y_axis_node                    = this->JsonToNodeRef(j_def["y_axis_node"   ]);
        def.offset                         = this->JsonToVector3(j_def["offset"  ]);
        def.rotation                       = this->JsonToVector3(j_def["rotation"]);
        def.mesh_name                      = j_def["mesh_name"].GetString();
        def.camera_settings.mode           = static_cast<RigDef::CameraSettings::Mode>(j_def["camera_mode"].GetInt()); // TODO: validate
        def.camera_settings.cinecam_index  = j_def["camera_cinecam_index"].GetUint();
        def.special                        = static_cast<RigDef::Prop::Special>(j_def["special"].GetInt()); // TODO: validate

        // ++ TODO ++ Animations ++
    })
}

} // namespace RigEditor
} // namespace RoR
