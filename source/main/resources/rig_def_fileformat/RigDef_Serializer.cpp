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
/// @date   10/2014

#include "RigDef_Serializer.h"

#include "Actor.h"
#include "GUIManager.h"
#include "RigDef_File.h"

#include <fstream>
#include <OgreStringConverter.h>
#include <iomanip>
#include <unordered_map>

using namespace RoR;
using namespace RigDef;
using namespace std;

Serializer::Serializer(RigDef::DocumentPtr def_file):
    m_rig_def(def_file),
    m_node_id_width(5),
    m_float_precision(6),
    m_inertia_function_width(10),
    m_bool_width(5), // strlen("false") == 5
    m_command_key_width(2),
    m_float_width(10)
{}

void Serializer::Serialize()
{
    // Prepare output
    m_stream.precision(m_float_precision); // Permanent

    // Write banner
    m_stream
        << "; ---------------------------------------------------------------------------- ;" << endl
        << "; Project: Rigs of Rods (http://www.rigsofrods.org)                            ;" << endl
        << "; File format: https://docs.rigsofrods.org/vehicle-creation/fileformat-truck   ;" << endl
        << "; ---------------------------------------------------------------------------- ;" << endl
        << endl;

    // Write name
    m_stream << m_rig_def->name << endl << endl;

    // Select source
    Document::Module* source_module = m_rig_def->root_module.get();

    // Write individual elements

    // About
    ProcessDescription(source_module);
    ProcessAuthors(source_module);
    ProcessFileinfo(source_module);
    ProcessGuid(source_module);
    WriteFlags();

    // Modules (aka 'sectionconfig')
    this->SerializeModule(m_rig_def->root_module);
    for (auto module_itor: m_rig_def->user_modules)
    {
        this->SerializeModule(module_itor.second);
    }

    // Finalize
    m_stream << "end" << endl;
}

void Serializer::SerializeModule(std::shared_ptr<RigDef::Document::Module> m)
{
    RigDef::Document::Module* source_module = m.get();

    if (m->name != RigDef::ROOT_MODULE_NAME)
    {
        m_stream << "section -1 " << m->name << endl;
    }

    ProcessManagedMaterialsAndOptions(source_module);
    ProcessGlobals(source_module);

    // Structure
    ProcessNodes(source_module);
    ProcessBeams(source_module);
    ProcessCameras(source_module);
    ProcessShocks(source_module);
    ProcessShocks2(source_module);
    ProcessHydros(source_module);
    ProcessCommands2(source_module);
    ProcessSlideNodes(source_module);
    ProcessTies(source_module);
    ProcessRopes(source_module);
    ProcessFixes(source_module);

    // Wheels
    ProcessMeshWheels(source_module);
    ProcessMeshWheels2(source_module);
    ProcessWheels(source_module);
    ProcessWheels2(source_module);
    ProcessFlexBodyWheels(source_module);

    // Driving
    ProcessEngine(source_module);
    ProcessEngoption(source_module);
    ProcessBrakes(source_module);
    ProcessAntiLockBrakes(source_module);
    ProcessTractionControl(source_module);
    ProcessTorqueCurve(source_module);
    ProcessCruiseControl(source_module);
    ProcessSpeedLimiter(source_module);
    ProcessAxles(source_module);
    ProcessTransferCase(source_module);
    ProcessInterAxles(source_module);

    // Features
    ProcessCinecam(source_module);
    ProcessAnimators(source_module);
    ProcessContacters(source_module);
    ProcessTriggers(source_module);
    ProcessLockgroups(source_module);
    ProcessHooks(source_module);
    ProcessRailGroups(source_module);
    ProcessRopables(source_module);
    ProcessParticles(source_module);
    ProcessCollisionBoxes(source_module);
    // TODO: detacher_group
    ProcessFlares2(source_module);
    ProcessMaterialFlareBindings(source_module);
    ProcessPropsAndAnimations(source_module);
    ProcessSubmesh(source_module);
    ProcessSubmeshGroundmodel(source_module);
    ProcessExhausts(source_module);
    ProcessGuiSettings(source_module);
    ProcessSetSkeletonSettings(source_module);
    ProcessVideocamera(source_module);
    ProcessExtCamera(source_module);
    ProcessSoundsources(source_module);
    ProcessSoundsources2(source_module);

    // Aerial
    ProcessWings(source_module);
    ProcessAirbrakes(source_module);
    ProcessTurboprops(source_module);
    ProcessFusedrag(source_module);
    ProcessPistonprops(source_module);
    ProcessTurbojets(source_module);

    // Marine
    ProcessScrewprops(source_module);

    if (m->name != RigDef::ROOT_MODULE_NAME)
    {
        m_stream << "endsection" << endl;
    }
}

void Serializer::ProcessPistonprops(Document::Module* module)
{
    if (module->pistonprops.empty())
    {
        return;
    }
    m_stream << "pistonprops" << endl;
    auto end_itor = module->pistonprops.end();
    for (auto itor = module->pistonprops.begin(); itor != end_itor; ++itor)
    {
        auto & def = *itor;

        m_stream << "\n\t" << setw(m_node_id_width) << def.reference_node.Str()
            << ", " << setw(m_node_id_width) << def.axis_node.Str()
            << ", " << setw(m_node_id_width) << def.blade_tip_nodes[0].Str()
            << ", " << setw(m_node_id_width) << def.blade_tip_nodes[1].Str()
            << ", " << setw(m_node_id_width) << def.blade_tip_nodes[2].Str()
            << ", " << setw(m_node_id_width) << def.blade_tip_nodes[3].Str()
            << ", " << setw(m_node_id_width) << (def.couple_node.IsValidAnyState() ? def.couple_node.Str() : "-1")
            << ", " << setw(m_float_width) << def.turbine_power_kW
            << ", " << setw(m_float_width) << def.pitch
            << ", " << def.airfoil;
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessTurbojets(Document::Module* module)
{
    if (module->turbojets.empty())
    {
        return;
    }
    m_stream << "turbojets" << endl;
    auto end_itor = module->turbojets.end();
    for (auto itor = module->turbojets.begin(); itor != end_itor; ++itor)
    {
        auto & def = *itor;

        m_stream << "\n\t" << setw(m_node_id_width) << def.front_node.Str()
            << ", " << setw(m_node_id_width) << def.back_node.Str()
            << ", " << setw(m_node_id_width) << def.side_node.Str()
            << ", " << setw(2) << def.is_reversable
            << ", " << setw(m_float_width) << def.dry_thrust
            << ", " << setw(m_float_width) << def.wet_thrust
            << ", " << setw(m_float_width) << def.front_diameter
            << ", " << setw(m_float_width) << def.back_diameter
            << ", " << setw(m_float_width) << def.nozzle_length;
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessScrewprops(Document::Module* module)
{
    if (module->screwprops.empty())
    {
        return;
    }
    m_stream << "screwprops" << endl;
    auto end_itor = module->screwprops.end();
    for (auto itor = module->screwprops.begin(); itor != end_itor; ++itor)
    {
        auto & def = *itor;

        m_stream << "\n\t" << setw(m_node_id_width) << def.prop_node.Str()
            << ", " << setw(m_node_id_width) << def.back_node.Str()
            << ", " << setw(m_node_id_width) << def.top_node.Str()
            << ", " << setw(m_float_width) << def.power;
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessFusedrag(Document::Module* module)
{
    m_stream << "fusedrag" << endl;
    for (RigDef::Fusedrag& def: module->fusedrag)
    {
        m_stream << "\n\t"
            << setw(m_node_id_width) << def.front_node.Str() << ", "
            << setw(m_node_id_width) << def.rear_node.Str() << ", ";
        if (def.autocalc)
        {
            m_stream << "autocalc, " << setw(m_float_width) << def.area_coefficient;
        }
        else
        {
            m_stream << setw(m_float_width) << def.approximate_width;
        }
        m_stream << ", " << def.airfoil_name;
    }

    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessTurboprops(Document::Module* module)
{
    if (module->turboprops2.empty())
    {
        return;
    }
    m_stream << "turboprops" << endl;
    auto end_itor = module->turboprops2.end();
    for (auto itor = module->turboprops2.begin(); itor != end_itor; ++itor)
    {
        RigDef::Turboprop2 & def = *itor;

        m_stream << "\n\t" << setw(m_node_id_width) << def.reference_node.Str()
            << ", " << setw(m_node_id_width) << def.axis_node.Str()
            << ", " << setw(m_node_id_width) << def.blade_tip_nodes[0].Str()
            << ", " << setw(m_node_id_width) << def.blade_tip_nodes[1].Str()
            << ", " << setw(m_node_id_width) << def.blade_tip_nodes[2].Str()
            << ", " << setw(m_node_id_width) << def.blade_tip_nodes[3].Str()
            << ", " << setw(m_float_width) << def.turbine_power_kW
            << ", " << def.airfoil;
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessAirbrakes(Document::Module* module)
{
    if (module->airbrakes.empty())
    {
        return;
    }
    m_stream << "airbrakes" << endl;
    auto end_itor = module->airbrakes.end();
    for (auto itor = module->airbrakes.begin(); itor != end_itor; ++itor)
    {
        RigDef::Airbrake & def = *itor;

        m_stream << "\n\t" << setw(m_node_id_width) << def.reference_node.Str()
            << ", " << setw(m_node_id_width) << def.x_axis_node.Str()
            << ", " << setw(m_node_id_width) << def.y_axis_node.Str()
            << ", " << setw(m_node_id_width) << (def.aditional_node.IsValidAnyState() ? def.aditional_node.Str() : "-1")
            << ", " << setw(m_float_width) << def.offset.x
            << ", " << setw(m_float_width) << def.offset.y
            << ", " << setw(m_float_width) << def.offset.z
            << ", " << setw(m_float_width) << def.width
            << ", " << setw(m_float_width) << def.height
            << ", " << setw(m_float_width) << def.max_inclination_angle
            << ", " << setw(m_float_width) << def.texcoord_x1
            << ", " << setw(m_float_width) << def.texcoord_y1
            << ", " << setw(m_float_width) << def.texcoord_x2
            << ", " << setw(m_float_width) << def.texcoord_y2
            << ", " << setw(m_float_width) << def.lift_coefficient;
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessWings(Document::Module* module)
{
    if (module->wings.empty())
    {
        return;
    }
    m_stream << "wings" << endl;
    auto end_itor = module->wings.end();
    for (auto itor = module->wings.begin(); itor != end_itor; ++itor)
    {
        RigDef::Wing & def = *itor;

        m_stream << "\n\t" << setw(m_node_id_width) << def.nodes[0].Str()
            << ", " << setw(m_node_id_width) << def.nodes[1].Str()
            << ", " << setw(m_node_id_width) << def.nodes[2].Str()
            << ", " << setw(m_node_id_width) << def.nodes[3].Str()
            << ", " << setw(m_node_id_width) << def.nodes[4].Str()
            << ", " << setw(m_node_id_width) << def.nodes[5].Str()
            << ", " << setw(m_node_id_width) << def.nodes[6].Str()
            << ", " << setw(m_node_id_width) << def.nodes[7].Str()
            << ", " << setw(m_float_width) << def.tex_coords[0]
            << ", " << setw(m_float_width) << def.tex_coords[1]
            << ", " << setw(m_float_width) << def.tex_coords[2]
            << ", " << setw(m_float_width) << def.tex_coords[3]
            << ", " << setw(m_float_width) << def.tex_coords[4]
            << ", " << setw(m_float_width) << def.tex_coords[5]
            << ", " << setw(m_float_width) << def.tex_coords[6]
            << ", " << setw(m_float_width) << def.tex_coords[7]
            << ", " << (char)def.control_surface
            << ", " << setw(m_float_width) << def.chord_point
            << ", " << setw(m_float_width) << def.min_deflection
            << ", " << setw(m_float_width) << def.max_deflection
            << ", " << def.airfoil
            << ", " << def.efficacy_coef;
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessSoundsources(Document::Module* module)
{
    if (module->soundsources.empty())
    {
        return;
    }
    m_stream << "soundsources" << endl;
    auto end_itor = module->soundsources.end();
    for (auto itor = module->soundsources.begin(); itor != end_itor; ++itor)
    {
        RigDef::SoundSource & def = *itor;

        m_stream << "\n\t" << setw(m_node_id_width) << def.node.Str() << ", " << def.sound_script_name;
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessSoundsources2(Document::Module* module)
{
    if (module->soundsources2.empty())
    {
        return;
    }
    m_stream << "soundsources2" << endl;
    auto end_itor = module->soundsources2.end();
    for (auto itor = module->soundsources2.begin(); itor != end_itor; ++itor)
    {
        RigDef::SoundSource2 & def = *itor;

        m_stream << "\n\t" << setw(m_node_id_width) << def.node.Str() 
            << ", " << setw(2) << def.mode
            << ", " << def.sound_script_name;
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessExtCamera(Document::Module* module)
{
    if (module->extcamera.size() == 0)
    {
        return;
    }
    RigDef::ExtCamera* def = &module->extcamera[0];
    m_stream << "extcamera ";

    switch (def->mode)
    {
    case ExtCameraMode::NODE:
        m_stream << "node " << def->node.Str();
        break;
    case ExtCameraMode::CINECAM:
        m_stream << "cinecam";
        break;
    case ExtCameraMode::CLASSIC:
    default:
        m_stream << "classic";
        break;
    }
        m_stream << "\n\n";
}

void Serializer::ProcessVideocamera(Document::Module* module)
{
    if (module->videocameras.empty())
    {
        return;
    }
    m_stream << "videocamera" << endl;
    auto end_itor = module->videocameras.end();
    for (auto itor = module->videocameras.begin(); itor != end_itor; ++itor)
    {
        RigDef::VideoCamera & def = *itor;

        m_stream << "\n\t" << setw(m_node_id_width) << def.reference_node.Str()
            << ", " << setw(m_node_id_width) << def.left_node.Str()
            << ", " << setw(m_node_id_width) << def.bottom_node.Str()
            << ", " << setw(m_node_id_width) << (def.alt_reference_node.IsValidAnyState() ? def.alt_reference_node.Str() : "-1")
            << ", " << setw(m_node_id_width) << (def.alt_orientation_node.IsValidAnyState() ? def.alt_orientation_node.Str() : "-1")
            << ", " << setw(m_float_width) << def.offset.x
            << ", " << setw(m_float_width) << def.offset.y
            << ", " << setw(m_float_width) << def.offset.z
            << ", " << setw(m_float_width) << def.rotation.x
            << ", " << setw(m_float_width) << def.rotation.y
            << ", " << setw(m_float_width) << def.rotation.z
            << ", " << setw(m_float_width) << def.field_of_view
            << ", " << setw(4) << def.texture_width
            << ", " << setw(4) << def.texture_height
            << ", " << setw(m_float_width) << def.min_clip_distance
            << ", " << setw(m_float_width) << def.max_clip_distance
            << ", " << setw(3) << def.camera_role
            << ", " << setw(3) << def.camera_mode
            << ", " << def.material_name;
        if (!def.camera_name.empty())
        {
            m_stream << ", " << def.camera_name;
        }
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessSetSkeletonSettings(Document::Module* module)
{
    if (module->set_skeleton_settings.empty())
    {
        return;
    }

    RigDef::SkeletonSettings& def = module->set_skeleton_settings[module->set_skeleton_settings.size() - 1];
    m_stream << "set_skeleton_settings " << def.visibility_range_meters << ", " << def.beam_thickness_meters << "\n\n";
}

void Serializer::ProcessGuiSettings(Document::Module* module)
{
    if (module->guisettings.empty())
    {
        return;
    }
    m_stream << "guisettings";
    for (GuiSettings& gs: module->guisettings)
    {
        m_stream << "\n\t" << gs.key << "=" << gs.value;
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessExhausts(Document::Module* module)
{
    if (module->exhausts.empty())
    {
        return;
    }
    m_stream << "exhausts" << endl;
    auto end_itor = module->exhausts.end();
    for (auto itor = module->exhausts.begin(); itor != end_itor; ++itor)
    {
        RigDef::Exhaust & def = *itor;

        m_stream << "\n\t" << setw(m_node_id_width) << def.reference_node.Str()
            << ", " << setw(m_node_id_width) << def.direction_node.Str()
            << ", " << def.particle_name;
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessSubmeshGroundmodel(Document::Module* module)
{
    if (module->submesh_groundmodel.empty())
    {
        return;
    }

    m_stream << "submesh_groundmodel " << module->submesh_groundmodel.back() << endl << endl; // Empty line
}

void Serializer::ProcessSubmesh(Document::Module* module)
{
    if (module->submeshes.empty())
    {
        return;
    }
    m_stream << "submesh" << endl;
    auto end_itor = module->submeshes.end();
    for (auto itor = module->submeshes.begin(); itor != end_itor; ++itor)
    {
        RigDef::Submesh & def = *itor;

        if (def.texcoords.size() > 0)
        {
            m_stream << "\n\ttexcoords";
            auto texcoord_end = def.texcoords.end();
            for (auto texcoord_itor = def.texcoords.begin(); texcoord_itor != texcoord_end; ++texcoord_itor)
            {
                m_stream << "\n\t" << setw(m_node_id_width) << texcoord_itor->node.Str() << ", " << texcoord_itor->u << ", " << texcoord_itor->v;
            }
        }
        if (def.cab_triangles.size() > 0)
        {
            m_stream << "\n\tcab";
            auto cab_end = def.cab_triangles.end();
            for (auto cab_itor = def.cab_triangles.begin(); cab_itor != cab_end; ++cab_itor)
            {
                m_stream << "\n\t" << setw(m_node_id_width) << cab_itor->nodes[0].Str() 
                    << ", " << setw(m_node_id_width) << cab_itor->nodes[1].Str() 
                    << ", " << setw(m_node_id_width) << cab_itor->nodes[2].Str()
                    << ", n";

                // Options
                if (BITMASK_IS_1(cab_itor->options, Cab::OPTION_c_CONTACT             )) m_stream << (char)CabOption::c_CONTACT;
                if (BITMASK_IS_1(cab_itor->options, Cab::OPTION_b_BUOYANT             )) m_stream << (char)CabOption::b_BUOYANT;
                if (BITMASK_IS_1(cab_itor->options, Cab::OPTION_p_10xTOUGHER          )) m_stream << (char)CabOption::p_10xTOUGHER;
                if (BITMASK_IS_1(cab_itor->options, Cab::OPTION_u_INVULNERABLE        )) m_stream << (char)CabOption::u_INVULNERABLE;
                if (BITMASK_IS_1(cab_itor->options, Cab::OPTION_s_BUOYANT_NO_DRAG     )) m_stream << (char)CabOption::s_BUOYANT_NO_DRAG;
                if (BITMASK_IS_1(cab_itor->options, Cab::OPTION_r_BUOYANT_ONLY_DRAG   )) m_stream << (char)CabOption::r_BUOYANT_ONLY_DRAG;
                if (BITMASK_IS_1(cab_itor->options, Cab::OPTION_D_CONTACT_BUOYANT     )) m_stream << (char)CabOption::D_CONTACT_BUOYANT;
                if (BITMASK_IS_1(cab_itor->options, Cab::OPTION_F_10xTOUGHER_BUOYANT  )) m_stream << (char)CabOption::F_10xTOUGHER_BUOYANT;
                if (BITMASK_IS_1(cab_itor->options, Cab::OPTION_S_INVULNERABLE_BUOYANT)) m_stream << (char)CabOption::S_INVULNERABLE_BUOYANT;
            }
        }
        if (def.backmesh)
        {
            m_stream << "\n\tbackmesh";
        }
    }
    m_stream << endl << endl; // Empty line
}

inline void PropAnimFlag(std::stringstream& out, int flags, bool& join, unsigned int mask, const char* name, char joiner = '|')
{
    if (flags & mask)
    {
        if (join)
        {
            out << joiner;
        }
        out << name;
        join = true;
    }
}

void Serializer::ProcessDirectiveAddAnimation(RigDef::Animation & anim)
{
    m_stream << "\n\tadd_animation " 
        << setw(m_float_width) << anim.ratio       << ", "
        << setw(m_float_width) << anim.lower_limit << ", "
        << setw(m_float_width) << anim.upper_limit << ", "
        << "source: ";
            
    // Source flags
    bool join = false;
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_AIRSPEED         , "airspeed");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_VERTICAL_VELOCITY, "vvi");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_ALTIMETER_100K   , "altimeter100k");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_ALTIMETER_10K    , "altimeter10k");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_ALTIMETER_1K     , "altimeter1k");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_ANGLE_OF_ATTACK  , "aoa");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_FLAP             , "flap");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_AIR_BRAKE        , "airbrake");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_ROLL             , "roll");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_PITCH            , "pitch");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_BRAKES           , "brakes");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_ACCEL            , "accel");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_CLUTCH           , "clutch");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_SPEEDO           , "speedo");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_TACHO            , "tacho");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_TURBO            , "turbo");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_PARKING          , "parking");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_SHIFT_LEFT_RIGHT , "shifterman1");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_SHIFT_BACK_FORTH , "shifterman2");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_SEQUENTIAL_SHIFT , "sequential");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_SHIFTERLIN       , "shifterlin");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_TORQUE           , "torque");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_HEADING          , "heading");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_DIFFLOCK         , "difflock");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_BOAT_RUDDER      , "rudderboat");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_BOAT_THROTTLE    , "throttleboat");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_STEERING_WHEEL   , "steeringwheel");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_AILERON          , "aileron");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_ELEVATOR         , "elevator");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_AIR_RUDDER       , "rudderair");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_PERMANENT        , "permanent");
    PropAnimFlag(m_stream, anim.source, join, Animation::SOURCE_EVENT            , "event");
            
    m_stream << ", mode: ";
    join = false;
    PropAnimFlag(m_stream, anim.mode, join, Animation::MODE_ROTATION_X  , "x-rotation");
    PropAnimFlag(m_stream, anim.mode, join, Animation::MODE_ROTATION_Y  , "y-rotation");
    PropAnimFlag(m_stream, anim.mode, join, Animation::MODE_ROTATION_Z  , "z-rotation");
    PropAnimFlag(m_stream, anim.mode, join, Animation::MODE_OFFSET_X    , "x-offset");
    PropAnimFlag(m_stream, anim.mode, join, Animation::MODE_OFFSET_Y    , "y-offset");
    PropAnimFlag(m_stream, anim.mode, join, Animation::MODE_OFFSET_Z    , "z-offset");

    // Solo flags
    PropAnimFlag(m_stream, anim.mode, join, Animation::MODE_AUTO_ANIMATE, " autoanimate", ',');
    PropAnimFlag(m_stream, anim.mode, join, Animation::MODE_NO_FLIP     , " noflip"     , ',');
    PropAnimFlag(m_stream, anim.mode, join, Animation::MODE_BOUNCE      , " bounce"     , ',');
    PropAnimFlag(m_stream, anim.mode, join, Animation::MODE_EVENT_LOCK  , " eventlock"  , ',');
            
    if (BITMASK_IS_1(anim.source, RigDef::Animation::SOURCE_EVENT))
    {
        m_stream << ", event: " << anim.event_name;
    }
}

void Serializer::ProcessFlexbodies(Document::Module* module)
{
    if (module->flexbodies.empty())
    {
        return;
    }
    m_stream << "flexbodies" << endl;
    auto end_itor = module->flexbodies.end();
    for (auto itor = module->flexbodies.begin(); itor != end_itor; ++itor)
    {
        RigDef::Flexbody* def = &*itor;

        // Prop-like line
        m_stream << "\n\t" << setw(m_node_id_width) << def->reference_node.Str()
            << ", " << setw(m_node_id_width) << def->x_axis_node.Str()
            << ", " << setw(m_node_id_width) << def->y_axis_node.Str()
            << ", " << setw(m_float_width) << def->offset.x
            << ", " << setw(m_float_width) << def->offset.y
            << ", " << setw(m_float_width) << def->offset.z
            << ", " << setw(m_float_width) << def->rotation.x 
            << ", " << setw(m_float_width) << def->rotation.y
            << ", " << setw(m_float_width) << def->rotation.z
            << ", " << def->mesh_name;

        // Forset line
        m_stream << "\n\t\tforset (node list)";
        auto forset_end = def->node_list.end();
        auto forset_itor = def->node_list.begin();
        bool first = true;
        for (; forset_itor != forset_end; ++forset_itor)
        {
            if (!first)
            {
                m_stream << ", ";
            }
            m_stream << forset_itor->Str();
            first = false;
        }

        // Animations
        auto anim_end = def->animations.end();
        for (auto anim_itor = def->animations.begin(); anim_itor != anim_end; ++anim_itor)
        {
            ProcessDirectiveAddAnimation(*anim_itor);
        }
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessPropsAndAnimations(Document::Module* module)
{
    if (module->props.empty())
    {
        return;
    }
    m_stream << "props" << endl;
    auto end_itor = module->props.end();
    for (auto itor = module->props.begin(); itor != end_itor; ++itor)
    {
        RigDef::Prop & def = *itor;

        m_stream << "\n\t" << setw(m_node_id_width) << def.reference_node.Str()
            << ", " << setw(m_node_id_width) << def.x_axis_node.Str()
            << ", " << setw(m_node_id_width) << def.y_axis_node.Str()
            << ", " << setw(m_float_width) << def.offset.x
            << ", " << setw(m_float_width) << def.offset.y
            << ", " << setw(m_float_width) << def.offset.z
            << ", " << setw(m_float_width) << def.rotation.x 
            << ", " << setw(m_float_width) << def.rotation.y
            << ", " << setw(m_float_width) << def.rotation.z
            << ", ";

        // Special props
        if (def.special == SpecialProp::BEACON)
        {
            m_stream << def.mesh_name 
                << " " << def.special_prop_beacon.flare_material_name
                << " " << def.special_prop_beacon.color.r
                << ", " << def.special_prop_beacon.color.g
                << ", " << def.special_prop_beacon.color.b;
        }
        else if (def.special == SpecialProp::DASHBOARD_LEFT || def.special == SpecialProp::DASHBOARD_RIGHT)
        {
            m_stream << " " << def.special_prop_dashboard.mesh_name
                << " " << def.special_prop_dashboard.offset.x
                << ", " << def.special_prop_dashboard.offset.y
                << ", " << def.special_prop_dashboard.offset.z
                << ", " << def.special_prop_dashboard.rotation_angle;
        }
        else
        {
            m_stream << def.mesh_name;
        }

        // Animations
        auto anim_end = def.animations.end();
        for (auto anim_itor = def.animations.begin(); anim_itor != anim_end; ++anim_itor)
        {
            ProcessDirectiveAddAnimation(*anim_itor);
        }
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessMaterialFlareBindings(Document::Module* module)
{
    if (module->materialflarebindings.empty())
    {
        return;
    }
    m_stream << "materialflarebindings" << endl;
    auto end_itor = module->materialflarebindings.end();
    for (auto itor = module->materialflarebindings.begin(); itor != end_itor; ++itor)
    {
        RigDef::MaterialFlareBinding & def = *itor;
        m_stream << "\n\t" << def.flare_number << ", " << def.material_name;
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessFlares2(Document::Module* module)
{
    if (module->flares2.empty())
    {
        return;
    }
    m_stream << "flares2" << endl;
    auto end_itor = module->flares2.end();
    for (auto itor = module->flares2.begin(); itor != end_itor; ++itor)
    {
        RigDef::Flare2 & def = *itor;

        m_stream << "\n\t" << setw(m_node_id_width) << def.reference_node.Str()
            << ", " << setw(m_node_id_width) << def.node_axis_x.Str()
            << ", " << setw(m_node_id_width) << def.node_axis_y.Str()
            << ", " << setw(m_float_width) << def.offset.x
            << ", " << setw(m_float_width) << def.offset.y
            << ", " << setw(m_float_width) << def.offset.z
            << ", " << (char)def.type
            << ", " << setw(2) << def.control_number
            << ", " << setw(4) << def.blink_delay_milis
            << ", " << setw(m_float_width) << def.size
            << ", " << def.material_name;
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessManagedMaterialsAndOptions(Document::Module* module)
{
    if (module->managedmaterials.empty())
    {
        return;
    }

    // Calc column widths
    size_t name_w = 0, type_w = 0, tex1_w = 0, tex2_w = 1, tex3_w = 1; // Handle space for '-' empty tex marker
    for (ManagedMaterial& mm: module->managedmaterials)
    {
        name_w = std::max(name_w, mm.name.length());
        type_w = std::max(type_w, strlen(RigDef::ManagedMaterial::TypeToStr(mm.type)));
        tex1_w = std::max(tex1_w, mm.diffuse_map.size());
        tex2_w = std::max(tex2_w, mm.damaged_diffuse_map.length());
        tex3_w = std::max(tex3_w, mm.specular_map.length());
    }

    m_stream << "managedmaterials" << std::left << endl; // Set left alignment. WARNING - PERSISTENT!
    bool first = true;
    ManagedMaterialsOptions mm_options;
    for (ManagedMaterial& def: module->managedmaterials)
    {
        if (first || (mm_options.double_sided != def.options.double_sided))
        {
            mm_options.double_sided = def.options.double_sided;
            m_stream << "\n\tset_managedmaterials_options " << (int) mm_options.double_sided;
        }

        m_stream << "\n\t"
                 << setw(name_w) << def.name << " "
                 << setw(type_w) << RigDef::ManagedMaterial::TypeToStr(def.type) << " "
                 << setw(tex1_w) << def.diffuse_map << " ";

        // Damage diffuse map - empty column if not applicable
        if (def.type == ManagedMaterialType::FLEXMESH_STANDARD || def.type == ManagedMaterialType::FLEXMESH_TRANSPARENT)
        {
            m_stream << setw(tex2_w) << (def.damaged_diffuse_map.empty() ? "-" : def.damaged_diffuse_map) << " ";
        }
        else
        {
            m_stream << setw(tex2_w) << "" << " ";
        }
        
        m_stream << setw(tex3_w) << (def.specular_map.empty() ? "-" : def.specular_map);

        first = false;
    }
    m_stream << std::right; // Reset to default alignment
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessCollisionBoxes(Document::Module* module)
{
    if (module->collisionboxes.empty())
    {
        return;
    }
    m_stream << "collisionboxes" << endl;
    auto end_itor = module->collisionboxes.end();
    for (auto itor = module->collisionboxes.begin(); itor != end_itor; ++itor)
    {
        RigDef::CollisionBox & def = *itor;

        auto nodes_end = def.nodes.end();
        auto node_itor = def.nodes.begin();
        m_stream << node_itor->Str();
        ++node_itor;
        for (; node_itor != nodes_end; ++node_itor)
        {
            m_stream << ", " << node_itor->Str();
        }
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessAxles(Document::Module* module)
{
    if (module->axles.empty())
    {
        return;
    }
    m_stream << "axles" << endl;
    auto end_itor = module->axles.end();
    for (auto itor = module->axles.begin(); itor != end_itor; ++itor)
    {
        RigDef::Axle & def = *itor;

        m_stream << "\n\t"
            << "w1(" << def.wheels[0][0].Str() << " " << def.wheels[0][1].Str() << "), "
            << "w2(" << def.wheels[1][0].Str() << " " << def.wheels[1][1].Str() << ")";
        if (! def.options.empty())
        {
            m_stream << ", d(";
            auto end = def.options.end();
            for (auto itor = def.options.begin(); itor != end; ++itor)
            {
                m_stream << (char)*itor;
            }
            m_stream << ")";
        }
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessInterAxles(Document::Module* module)
{
    if (module->interaxles.empty())
    {
        return;
    }
    m_stream << "interaxles" << endl;
    auto end_itor = module->interaxles.end();
    for (auto itor = module->interaxles.begin(); itor != end_itor; ++itor)
    {
        RigDef::InterAxle & def = *itor;

        m_stream << "\n\t"
            << def.a1 << ", "
            << def.a2;
        if (! def.options.empty())
        {
            m_stream << ", d(";
            auto end = def.options.end();
            for (auto itor = def.options.begin(); itor != end; ++itor)
            {
                m_stream << (char)*itor;
            }
            m_stream << ")";
        }
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessTransferCase(Document::Module* module)
{
    if (module->transfercase.size() == 0)
    {
        return;
    }
    TransferCase& def = module->transfercase[module->transfercase.size() - 1];

    m_stream << "transfercase\t" 
        << def.a1 << ", "
        << def.a2 << ", "
        << def.has_2wd << ", "
        << def.has_2wd_lo;
        for (float gear_ratio : def.gear_ratios)
        {
            m_stream << ", " << gear_ratio;
        }
        m_stream << endl << endl;
}

void Serializer::ProcessCruiseControl(Document::Module* module)
{
    if (module->cruisecontrol.size() == 0)
    {
        return;
    }

    RigDef::CruiseControl& cruisecontrol = module->cruisecontrol[module->cruisecontrol.size() - 1];
    
    m_stream << "cruisecontrol " 
        << cruisecontrol.min_speed << ", " 
        << (int) cruisecontrol.autobrake
        << endl << endl;
    
}

void Serializer::ProcessSpeedLimiter(Document::Module* module)
{
    if (module->speedlimiter.size() == 0)
    {
        return;
    }
    m_stream << "speedlimiter " 
        << module->speedlimiter[module->speedlimiter.size() - 1].max_speed
        << endl << endl;
}

void Serializer::ProcessTorqueCurve(Document::Module* module)
{
    if (module->torquecurve.size() == 0)
    {
        return;
    }
    m_stream << "torquecurve" << endl;
    if (module->torquecurve[0].predefined_func_name.empty())
    {
        auto itor_end = module->torquecurve[0].samples.end();
        auto itor = module->torquecurve[0].samples.begin();
        for (; itor != itor_end; ++itor)
        {
            m_stream << "\n\t" << itor->power << ", " << itor->torque_percent;
        }
    }
    else
    {
        m_stream << "\n\t" << module->torquecurve[0].predefined_func_name;
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessParticles(Document::Module* module)
{
    if (module->particles.empty())
    {
        return;
    }
    m_stream << "particles" << endl;
    auto end_itor = module->particles.end();
    for (auto itor = module->particles.begin(); itor != end_itor; ++itor)
    {
        RigDef::Particle & def = *itor;

        m_stream << "\n\t" 
            << setw(m_node_id_width) << def.emitter_node.Str() << ", "
            << setw(m_node_id_width) << def.reference_node.Str() << ", "
            << def.particle_system_name;
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessRopables(Document::Module* module)
{
    if (module->ropables.empty())
    {
        return;
    }
    m_stream << "ropables" << endl;
    auto end_itor = module->ropables.end();
    for (auto itor = module->ropables.begin(); itor != end_itor; ++itor)
    {
        RigDef::Ropable & def = *itor;

        m_stream << "\n\t" << setw(m_node_id_width) << def.node.Str()
            << ", " << def.group
            << ", " << (int) def.has_multilock;
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessTies(Document::Module* module)
{
    if (module->ties.empty())
    {
        return;
    }
    m_stream << "ties" << endl;
    auto end_itor = module->ties.end();
    for (auto itor = module->ties.begin(); itor != end_itor; ++itor)
    {
        RigDef::Tie & def = *itor;

        m_stream << "\n\t" << setw(m_node_id_width) << def.root_node.Str()
            << ", " << setw(m_float_width) << def.max_reach_length
            << ", " << setw(m_float_width) << def.auto_shorten_rate
            << ", " << setw(m_float_width) << def.min_length
            << ", " << setw(m_float_width) << def.max_length
            << ", " << (BITMASK_IS_1(def.options, Tie::OPTION_i_INVISIBLE) ? "i" : "n")
            << ", " << (BITMASK_IS_1(def.options, Tie::OPTION_s_DISABLE_SELF_LOCK) ? "s" : "")
            << ", " << setw(m_float_width) << def.max_stress
            << ", " << def.group;
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessFixes(Document::Module* module)
{
    if (module->fixes.empty())
    {
        return;
    }
    m_stream << "fixes" << endl;
    auto end_itor = module->fixes.end();
    for (auto itor = module->fixes.begin(); itor != end_itor; ++itor)
    {
        m_stream << "\n\t" << setw(m_node_id_width) << itor->Str();
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessRopes(Document::Module* module)
{
    if (module->ropes.empty())
    {
        return;
    }
    m_stream << "ropes" << endl;
    auto end_itor = module->ropes.end();
    bool first = true;
    BeamDefaults* beam_defaults = nullptr;
    for (auto itor = module->ropes.begin(); itor != end_itor; ++itor)
    {
        RigDef::Rope & def = *itor;

        if (first || (def.beam_defaults.get() != beam_defaults))
        {
            ProcessBeamDefaults(def.beam_defaults.get());
        }

        m_stream << "\n\t" 
            << setw(m_node_id_width) << def.root_node.Str() << ", "
            << setw(m_node_id_width) << def.end_node.Str();
        if (def.invisible)
        {
            m_stream << ", i";
        }
        first = false;
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessRailGroups(Document::Module* module)
{
    if (module->railgroups.empty())
    {
        return;
    }
    m_stream << "railgroups" << endl << endl;
    auto end_itor = module->railgroups.end();
    for (auto itor = module->railgroups.begin(); itor != end_itor; ++itor)
    {
        RigDef::RailGroup & def = *itor;

        m_stream << "\n\t" << def.id;
        auto node_end = def.node_list.end();
        for (auto node_itor = def.node_list.begin(); node_itor != node_end; ++node_itor)
        {
            m_stream << ", " << node_itor->start.Str();
            if (node_itor->IsRange())
            {
                m_stream << " - " << node_itor->end.Str();
            }
        }
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessSlideNodes(Document::Module* module)
{
    if (module->slidenodes.empty())
    {
        return;
    }
    m_stream << "slidenodes" << endl << endl;
    auto end_itor = module->slidenodes.end();
    for (auto itor = module->slidenodes.begin(); itor != end_itor; ++itor)
    {
        RigDef::SlideNode & def = *itor;

        m_stream << "\n\t" << setw(m_node_id_width) << def.slide_node.Str();

        // Define rail - either list of nodes, or raigroup ID
        if (!def.rail_node_ranges.empty())
        {
            auto end = def.rail_node_ranges.end();
            auto itor = def.rail_node_ranges.begin();
            for (; itor != end; ++itor)
            {
                m_stream << ", " << itor->start.Str();
                if (itor->IsRange())
                {
                    m_stream << " - " << itor->end.Str();
                }
            }
        }
        else
        {
            m_stream << ", g" << def.railgroup_id;
        }

        // Optional args
        if (def._spring_rate_set)      { m_stream << ", s" << def.spring_rate; }
        if (def._break_force_set)      { m_stream << ", b" << def.break_force; }
        if (def._tolerance_set)        { m_stream << ", t" << def.tolerance; }
        if (def._attachment_rate_set)  { m_stream << ", r" << def.attachment_rate; }
        if (def._max_attach_dist_set)  { m_stream << ", d" << def.max_attach_dist; }

        // Constraint flags (cX)
             if (BITMASK_IS_1(def.constraint_flags, SlideNode::CONSTRAINT_ATTACH_ALL    )) { m_stream << ", ca"; }
        else if (BITMASK_IS_1(def.constraint_flags, SlideNode::CONSTRAINT_ATTACH_FOREIGN)) { m_stream << ", cf"; }
        else if (BITMASK_IS_1(def.constraint_flags, SlideNode::CONSTRAINT_ATTACH_SELF   )) { m_stream << ", cs"; }
        else if (BITMASK_IS_1(def.constraint_flags, SlideNode::CONSTRAINT_ATTACH_NONE   )) { m_stream << ", cn"; }
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessHooks(Document::Module* module)
{
    if (module->hooks.empty())
    {
        return;
    }
    m_stream << "hooks" << endl << endl;
    auto end_itor = module->hooks.end();
    for (auto itor = module->hooks.begin(); itor != end_itor; ++itor)
    {
        RigDef::Hook & def = *itor;

        m_stream << "\n\t" << setw(m_node_id_width) << def.node.Str();

        // Boolean options
        if (def.flag_auto_lock)  { m_stream << ", auto-lock"; }
        if (def.flag_no_disable) { m_stream << ", nodisable"; }
        if (def.flag_no_rope)    { m_stream << ", norope";    }
        if (def.flag_self_lock)  { m_stream << ", self-lock"; }
        if (def.flag_visible)    { m_stream << ", visible";   }

        // Key-value options
        m_stream 
            << ", hookrange: "  << setw(m_float_width) << def.option_hook_range
            << ", speedcoef: "  << setw(m_float_width) << def.option_speed_coef
            << ", maxforce: "   << setw(m_float_width) << def.option_max_force
            << ", hookgroup: "  << setw(4) << def.option_hookgroup
            << ", lockgroup: "  << setw(4) << def.option_lockgroup
            << ", timer: "      << setw(m_float_width) << def.option_timer
            << ", shortlimit: " << setw(m_float_width) << def.option_min_range_meters;
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessLockgroups(Document::Module* module)
{
    if (module->lockgroups.empty())
    {
        return;
    }
    m_stream << "lockgroups" << endl << endl;
    auto end_itor = module->lockgroups.end();
    for (auto itor = module->lockgroups.begin(); itor != end_itor; ++itor)
    {
        RigDef::Lockgroup & def = *itor;

        m_stream << "\n\t" << def.number;
        auto nodes_end = def.nodes.end();
        for (auto nodes_itor = def.nodes.begin(); nodes_itor != nodes_end; ++nodes_itor)
        {
            m_stream << ", " << nodes_itor->Str();
        }
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessTriggers(Document::Module* module)
{
    if (module->triggers.empty())
    {
        return;
    }
    m_stream << "animators" << endl << endl;
    auto end_itor = module->triggers.end();
    for (auto itor = module->triggers.begin(); itor != end_itor; ++itor)
    {
        RigDef::Trigger & def = *itor;

        m_stream << "\n\t"
            << setw(m_node_id_width) << def.nodes[0].Str()       << ", "
            << setw(m_node_id_width) << def.nodes[1].Str()       << ", "
            << setw(m_float_width) << def.contraction_trigger_limit << ", "
            << setw(m_float_width) << def.expansion_trigger_limit   << ", "
            << setw(4) << def.shortbound_trigger_action << ", "
            << setw(4) << def.longbound_trigger_action  << ", ";

        if (BITMASK_IS_1(def.options, Trigger::OPTION_i_INVISIBLE           )) { m_stream << (char)TriggerOption::i_INVISIBLE; }
        if (BITMASK_IS_1(def.options, Trigger::OPTION_c_COMMAND_STYLE       )) { m_stream << (char)TriggerOption::c_COMMAND_STYLE; }
        if (BITMASK_IS_1(def.options, Trigger::OPTION_x_START_DISABLED      )) { m_stream << (char)TriggerOption::x_START_DISABLED; }
        if (BITMASK_IS_1(def.options, Trigger::OPTION_b_KEY_BLOCKER         )) { m_stream << (char)TriggerOption::b_KEY_BLOCKER; }
        if (BITMASK_IS_1(def.options, Trigger::OPTION_B_TRIGGER_BLOCKER     )) { m_stream << (char)TriggerOption::B_TRIGGER_BLOCKER; }
        if (BITMASK_IS_1(def.options, Trigger::OPTION_A_INV_TRIGGER_BLOCKER )) { m_stream << (char)TriggerOption::A_INV_TRIGGER_BLOCKER; }
        if (BITMASK_IS_1(def.options, Trigger::OPTION_s_CMD_NUM_SWITCH      )) { m_stream << (char)TriggerOption::s_CMD_NUM_SWITCH; }
        if (BITMASK_IS_1(def.options, Trigger::OPTION_h_UNLOCKS_HOOK_GROUP  )) { m_stream << (char)TriggerOption::h_UNLOCKS_HOOK_GROUP; }
        if (BITMASK_IS_1(def.options, Trigger::OPTION_H_LOCKS_HOOK_GROUP    )) { m_stream << (char)TriggerOption::H_LOCKS_HOOK_GROUP; }
        if (BITMASK_IS_1(def.options, Trigger::OPTION_t_CONTINUOUS          )) { m_stream << (char)TriggerOption::t_CONTINUOUS; }
        if (BITMASK_IS_1(def.options, Trigger::OPTION_E_ENGINE_TRIGGER      )) { m_stream << (char)TriggerOption::E_ENGINE_TRIGGER; }

        m_stream << " " << def.boundary_timer;
    }
    m_stream << endl << endl; // Empty line
}

#define ANIMATOR_ADD_FLAG(DEF_VAR, AND_VAR, BITMASK_CONST, NAME_STR) \
    if (AND_VAR) { m_stream << " | "; } \
    if (BITMASK_IS_1((DEF_VAR).flags, RigDef::Animator::BITMASK_CONST)) { \
        AND_VAR = true; \
        m_stream << NAME_STR; \
    }

#define ANIMATOR_ADD_AERIAL_FLAG(DEF_VAR, AND_VAR, BITMASK_CONST, NAME_STR) \
    if (AND_VAR) { m_stream << " | "; } \
    if (BITMASK_IS_1((DEF_VAR).aero_animator.flags, RigDef::AeroAnimator::BITMASK_CONST)) { \
        AND_VAR = true; \
        m_stream << NAME_STR << DEF_VAR.aero_animator.engine_idx + 1; \
    }

#define ANIMATOR_ADD_LIMIT(DEF_VAR, AND_VAR, BITMASK_CONST, NAME_STR, VALUE) \
    if (AND_VAR) { m_stream << " | "; } \
    if (BITMASK_IS_1((DEF_VAR).aero_animator.flags, RigDef::Animator::BITMASK_CONST)) { \
        AND_VAR = true; \
        m_stream << NAME_STR << ": " << VALUE; \
    }

void Serializer::ProcessAnimators(Document::Module* module)
{
    if (module->animators.empty())
    {
        return;
    }
    m_stream << "animators" << endl << endl;
    auto end_itor = module->animators.end();
    for (auto itor = module->animators.begin(); itor != end_itor; ++itor)
    {
        RigDef::Animator & def = *itor;

        m_stream << "\t"
            << setw(m_node_id_width) << def.nodes[0].Str() << ", "
            << setw(m_node_id_width) << def.nodes[1].Str() << ", "
            << setw(m_float_width) << def.lenghtening_factor << ", ";

        // Options
        bool bAnd = false;
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_VISIBLE          , "vis")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_INVISIBLE        , "inv")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_AIRSPEED         , "airspeed")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_VERTICAL_VELOCITY, "vvi")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_ALTIMETER_100K   , "altimeter100k")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_ALTIMETER_10K    , "altimeter10k")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_ALTIMETER_1K     , "altimeter1k")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_ANGLE_OF_ATTACK  , "aoa")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_FLAP             , "flap")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_AIR_BRAKE        , "airbrake")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_ROLL             , "roll")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_PITCH            , "pitch")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_BRAKES           , "brakes")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_ACCEL            , "accel")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_CLUTCH           , "clutch")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_SPEEDO           , "speedo")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_TACHO            , "tacho")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_TURBO            , "turbo")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_PARKING          , "parking")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_SHIFT_LEFT_RIGHT , "shifterman1")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_SHIFT_BACK_FORTH , "shifterman2")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_SEQUENTIAL_SHIFT , "sequential")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_GEAR_SELECT      , "shifterlin")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_TORQUE           , "torque")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_DIFFLOCK         , "difflock")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_BOAT_RUDDER      , "rudderboat")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_BOAT_THROTTLE    , "throttleboat")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_SHORT_LIMIT      , "shortlimit")
        ANIMATOR_ADD_FLAG(def, bAnd, OPTION_LONG_LIMIT       , "longlimit")

        ANIMATOR_ADD_AERIAL_FLAG(def, bAnd, OPTION_THROTTLE , "throttle")
        ANIMATOR_ADD_AERIAL_FLAG(def, bAnd, OPTION_RPM      , "rpm")
        ANIMATOR_ADD_AERIAL_FLAG(def, bAnd, OPTION_TORQUE   , "aerotorq")
        ANIMATOR_ADD_AERIAL_FLAG(def, bAnd, OPTION_PITCH    , "aeropit")
        ANIMATOR_ADD_AERIAL_FLAG(def, bAnd, OPTION_STATUS   , "aerostatus")

        ANIMATOR_ADD_LIMIT(def, bAnd, OPTION_SHORT_LIMIT    , "shortlimit", def.short_limit)
        ANIMATOR_ADD_LIMIT(def, bAnd, OPTION_LONG_LIMIT     , "longlimit",  def.long_limit)
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessContacters(Document::Module* module)
{
    if (module->contacters.empty())
    {
        return;
    }
    m_stream << "contacters" << endl << endl;

    for (RigDef::Node::Ref& node: module->contacters)
    {
        m_stream << setw(m_node_id_width) << node.Str() << endl;
    }

    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessRotators(Document::Module* module)
{
    if (module->rotators.empty())
    {
        return;
    }
    m_stream << "rotators" << endl << endl;
    auto end_itor = module->rotators.end();
    for (auto itor = module->rotators.begin(); itor != end_itor; ++itor)
    {
        Rotator & def = *itor;

        // Axis nodes
        m_stream
             << setw(m_node_id_width) << def.axis_nodes[0].Str() << ", "
             << setw(m_node_id_width) << def.axis_nodes[1].Str() << ", ";

        // Baseplate nodes
        for (int i = 0; i < 4; ++i)
        {
            m_stream << setw(m_node_id_width) << def.base_plate_nodes[i].Str() << ", ";
        }

        // Rotating plate nodes
        for (int i = 0; i < 4; ++i)
        {
            m_stream << setw(m_node_id_width) << def.rotating_plate_nodes[i].Str() << ", ";
        }
        
        // Attributes
        m_stream << setw(m_float_width) << def.rate << ", "
                 << setw(4) << def.spin_left_key << ", "
                 << setw(4) << def.spin_right_key << ", ";

        // Inertia
        m_stream 
            << setw(m_float_width) << def.inertia.start_delay_factor << ", "
            << setw(m_float_width) << def.inertia.stop_delay_factor  << ", "
            << def.inertia.start_function     << ", "
            << def.inertia.stop_function      << ", "
            << def.engine_coupling            << ", "
            << (def.needs_engine ? "true" : "false");
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessRotators2(Document::Module* module)
{
    if (module->rotators2.empty())
    {
        return;
    }

    // Calc column widths
    size_t desc_w = 0, startf_w = 0, stopf_w = 0;
    for (RigDef::Rotator2& def: module->rotators2)
    {
        desc_w   = std::max(desc_w,   def.description.length());
        startf_w = std::max(startf_w, def.inertia.start_function.length());
        stopf_w  = std::max(stopf_w,  def.inertia.stop_function.length());
    }

    // Write data
    m_stream << "rotators2" << endl << endl;
    for (RigDef::Rotator2& def: module->rotators2)
    {
        // Axis nodes
        m_stream
             << setw(m_node_id_width)<< def.axis_nodes[0].Str() << ", "
             << setw(m_node_id_width)<< def.axis_nodes[1].Str() << ", ";

        // Baseplate nodes
        for (int i = 0; i < 4; ++i)
        {
            m_stream << setw(m_node_id_width) << def.base_plate_nodes[i].Str() << ", ";
        }

        // Rotating plate nodes
        for (int i = 0; i < 4; ++i)
        {
            m_stream << setw(m_node_id_width) << def.rotating_plate_nodes[i].Str() << ", ";
        }
        
        // Attributes
        m_stream 
            << setw(m_float_width) << def.rate << ", " 
            << def.spin_left_key   << ", " 
            << def.spin_right_key  << ", "
            << setw(m_float_width) << def.rotating_force << ", "
            << setw(m_float_width) << def.tolerance << ", "
            << setw(desc_w) << def.description << ", ";

        // Inertia
        m_stream 
            << setw(m_float_width) << def.inertia.start_delay_factor << ", "
            << setw(m_float_width) << def.inertia.stop_delay_factor  << ", "
            << setw(startf_w)      << def.inertia.start_function  << ", "
            << setw(stopf_w)       << def.inertia.stop_function  << ", "
            << setw(m_float_width) << def.engine_coupling        << ", "
            << (def.needs_engine ? "true" : "false");
    }
    m_stream << endl << endl; // Empty line
}

void Serializer::ProcessFlexBodyWheels(Document::Module* module)
{
    if (module->flexbodywheels.empty())
    {
        return;
    }

    this->ResetPresets();

    m_stream << "flexbodywheels" << endl << endl;
    auto end_itor = module->flexbodywheels.end();
    for (auto itor = module->flexbodywheels.begin(); itor != end_itor; ++itor)
    {
        this->UpdatePresets(itor->beam_defaults.get(), itor->node_defaults.get(), nullptr);

        m_stream << "\t"
            << setw(m_float_width)   << itor->tyre_radius                   << ", "
            << setw(m_float_width)   << itor->rim_radius                    << ", "
            << setw(m_float_width)   << itor->width                         << ", "
            << setw(3)               << itor->num_rays                      << ", "
            << setw(m_node_id_width) << itor->nodes[0].Str()           << ", "
            << setw(m_node_id_width) << itor->nodes[1].Str()           << ", "
            << setw(m_node_id_width) << RigidityNodeToStr(itor->rigidity_node) << ", "
            << setw(3)               << (int)itor->braking                  << ", "
            << setw(3)               << (int)itor->propulsion               << ", "
            << setw(m_node_id_width) << itor->reference_arm_node.Str() << ", "
            << setw(m_float_width)   << itor->mass                          << ", "
            << setw(m_float_width)   << itor->tyre_springiness              << ", "
            << setw(m_float_width)   << itor->tyre_damping                  << ", "
            << setw(m_float_width)   << itor->rim_springiness               << ", "
            << setw(m_float_width)   << itor->rim_damping                   << ", "
                                     << (static_cast<char>(itor->side))     << ", "
                                     << itor->rim_mesh_name                 << " " // Separator = space!
                                     << itor->tyre_mesh_name
                                     << endl;
    }

    m_stream << endl; // Empty line
}

void Serializer::ProcessTractionControl(Document::Module* module)
{
    if (module->tractioncontrol.size() == 0) { return; }

    RigDef::TractionControl& def = module->tractioncontrol[module->tractioncontrol.size() - 1];

    m_stream << "TractionControl "
        << setw(m_float_width) << def.regulation_force << ", "
        << setw(m_float_width) << def.wheel_slip << ", "
        << setw(m_float_width) << def.fade_speed << ", "
        << setw(m_float_width) << def.pulse_per_sec << ", "
        << "mode: " << (def.attr_is_on ? "ON" : "OFF");

    // Modes
    if (def.attr_no_dashboard) { m_stream << " & NODASH ";   }
    if (def.attr_no_toggle)    { m_stream << " & NOTOGGLE "; }

    m_stream << endl << endl;  // Empty line
}

void Serializer::ProcessBrakes(Document::Module* module)
{
    if (module->brakes.size() == 0) { return; }

    Brakes& brakes = module->brakes[module->brakes.size() - 1];

    m_stream << "brakes\n\t" 
        << setw(m_float_width) << brakes.default_braking_force << ", "
        << setw(m_float_width) << brakes.parking_brake_force;

    m_stream << endl << endl;  // Empty line
}

void Serializer::ProcessAntiLockBrakes(Document::Module* module)
{
    if (module->antilockbrakes.size() == 0) { return; }

    RigDef::AntiLockBrakes* alb = &module->antilockbrakes[module->antilockbrakes.size() - 1];

    m_stream << "AntiLockBrakes "
        << setw(m_float_width) << alb->regulation_force << ", "
        << setw(m_float_width) << alb->min_speed << ", "
        << setw(m_float_width) << alb->pulse_per_sec << ", "
        << "mode: " << (alb->attr_is_on ? "ON" : "OFF");
    // Modes
    if (alb->attr_no_dashboard) { m_stream << " & NODASH ";   }
    if (alb->attr_no_toggle)    { m_stream << " & NOTOGGLE "; }

    m_stream << endl << endl;  // Empty line
}

void Serializer::ProcessEngine(Document::Module* module)
{
    if (module->engine.size() == 0)
    {
        return;
    }

    Engine& engine = module->engine[module->engine.size() - 1];
    
    m_stream << "engine" 
        "\n;\t"
        "ShiftDownRPM,"
        " ShiftUpRPM,"
        "     Torque,"
        " GlobalGear,"
        " ReverseGear,"
        " NeutralGear,"
        " Forward gears...\n\t"
        << setw(12)   << engine.shift_down_rpm                << ", "
        << setw(10)   << engine.shift_up_rpm                  << ", "
        << setw(10)   << engine.torque                        << ", "
        << setw(10)   << engine.global_gear_ratio             << ", "
        << setw(11)   << engine.reverse_gear_ratio            << ", "
        << setw(11)   << engine.neutral_gear_ratio;
    
    auto itor = engine.gear_ratios.begin();
    auto end  = engine.gear_ratios.end();
    for (; itor != end; ++itor)
    {
        m_stream << ", " << *itor;
    }
    m_stream << ", -1.0" /*terminator*/ << endl << endl;
}

void Serializer::ProcessEngoption(Document::Module* module)
{
    if (module->engoption.size() == 0)
    {
        return;
    }

    Engoption& engoption = module->engoption[module->engoption.size() - 1];
    
    m_stream << "engoption"
        "\n;\t"
        "EngInertia,"
        " EngineType,"
        " ClutchForce,"
        " ShiftTime,"
        " ClutchTime,"
        " PostShiftTime,"
        " StallRPM,"
        " IdleRPM,"
        " MaxIdleMixture,"
        " MinIdleMixture"
        " BrakingForce"
        "\n\t" 
        << setw(10)   << engoption.inertia           << ", "
        << setw(10)   << (char)engoption.type        << ", "
        << setw(11)   << engoption.clutch_force      << ", "
        << setw( 9)   << engoption.shift_time        << ", "
        << setw(10)   << engoption.clutch_time       << ", "
        << setw(13)   << engoption.post_shift_time   << ", "
        << setw( 8)   << engoption.stall_rpm         << ", "
        << setw( 7)   << engoption.idle_rpm          << ", "
        << setw(14)   << engoption.max_idle_mixture  << ", "
        << setw(14)   << engoption.min_idle_mixture  << ", "
        << setw(15)   << engoption.braking_torque;
    
    m_stream << endl << endl;  // Empty line
}

void Serializer::ProcessHelp(Document::Module* module)
{
    if (module->help.size() == 0)
    {
        return;
    }
    m_stream << "help\n\t" << module->help[module->help.size() - 1].material << endl << endl;
}

void Serializer::ProcessWheels2(Document::Module* module)
{
    if (module->wheels2.empty())
    {
        return;
    }

    this->ResetPresets();
    m_stream << "wheels2" << endl << endl;
    auto end_itor = module->wheels2.end();
    for (auto itor = module->wheels2.begin(); itor != end_itor; ++itor)
    {
        this->UpdatePresets(itor->beam_defaults.get(), itor->node_defaults.get(), nullptr);

        m_stream << "\t"
            << setw(m_float_width)   << itor->tyre_radius                   << ", "
            << setw(m_float_width)   << itor->rim_radius                    << ", "
            << setw(m_float_width)   << itor->width                         << ", "
            << setw(3)               << itor->num_rays                      << ", "
            << setw(m_node_id_width) << itor->nodes[0].Str()           << ", "
            << setw(m_node_id_width) << itor->nodes[1].Str()           << ", "
            << setw(m_node_id_width) << RigidityNodeToStr(itor->rigidity_node) << ", "
            << setw(3)               << (int)itor->braking                  << ", "
            << setw(3)               << (int)itor->propulsion               << ", "
            << setw(m_node_id_width) << itor->reference_arm_node.Str() << ", "
            << setw(m_float_width)   << itor->mass                          << ", "
            << setw(m_float_width)   << itor->rim_springiness               << ", "
            << setw(m_float_width)   << itor->rim_damping                   << ", "
            << setw(m_float_width)   << itor->tyre_springiness              << ", "
            << setw(m_float_width)   << itor->tyre_damping                  << ", "
                                     << itor->face_material_name            << " " // Separator = space!
                                     << itor->band_material_name            << " " // Separator = space!
                                     ;
        m_stream << endl;
    }

    m_stream << endl; // Empty line
}

void Serializer::ProcessWheels(Document::Module* module)
{
    if (module->wheels.empty())
    {
        return;
    }

    this->ResetPresets();
    m_stream << "wheels" << endl << endl;
    auto end_itor = module->wheels.end();
    for (auto itor = module->wheels.begin(); itor != end_itor; ++itor)
    {
        this->UpdatePresets(itor->beam_defaults.get(), itor->node_defaults.get(), nullptr);

        m_stream << "\t"
            << setw(m_float_width)   << itor->radius                        << ", "
            << setw(m_float_width)   << itor->width                         << ", "
            << setw(3)               << itor->num_rays                      << ", "
            << setw(m_node_id_width) << itor->nodes[0].Str()                << ", "
            << setw(m_node_id_width) << itor->nodes[1].Str()                << ", "
            << setw(m_node_id_width) << RigidityNodeToStr(itor->rigidity_node) << ", "
            << setw(3)               << (int)itor->braking                  << ", "
            << setw(3)               << (int)itor->propulsion               << ", "
            << setw(m_node_id_width) << itor->reference_arm_node.Str()      << ", "
            << setw(m_float_width)   << itor->mass                          << ", "
            << setw(m_float_width)   << itor->springiness                   << ", "
            << setw(m_float_width)   << itor->damping                       << ", "
                                     << itor->face_material_name            << " " // Separator = space!
                                     << itor->band_material_name            << " " // Separator = space!
                                     ;
        m_stream << endl;
    }

    m_stream << endl; // Empty line
}

void Serializer::ExportBaseMeshWheel(BaseMeshWheel& def)
{
    this->UpdatePresets(def.beam_defaults.get(), def.node_defaults.get(), nullptr);

    m_stream << "\t"
    << setw(m_float_width)   << def.tyre_radius                   << ", "
    << setw(m_float_width)   << def.rim_radius                    << ", "
    << setw(m_float_width)   << def.width                         << ", "
    << setw(3)               << def.num_rays                      << ", "
    << setw(m_node_id_width) << def.nodes[0].ToString()           << ", "
    << setw(m_node_id_width) << def.nodes[1].ToString()           << ", "
    << setw(m_node_id_width) << def.rigidity_node.ToString()      << ", "
    << setw(3)               << (int)def.braking                  << ", "
    << setw(3)               << (int)def.propulsion               << ", "
    << setw(m_node_id_width) << def.reference_arm_node.ToString() << ", "
    << setw(m_float_width)   << def.mass                          << ", "
    << setw(m_float_width)   << def.spring                        << ", "
    << setw(m_float_width)   << def.damping                       << ", "
                                << (static_cast<char>(def.side))     << ", "
                                << def.mesh_name                     << " " // Separator = space!
                                << def.material_name;
    m_stream << endl;    
}

void Serializer::ProcessMeshWheels(Document::Module* module)
{
    if (module->meshwheels.empty()) { return; }
    
    this->ResetPresets();

    m_stream << "meshwheels" << "\n\n";

    for (MeshWheel& def: module->meshwheels)
    {
        this->ExportBaseMeshWheel(def);
    }

    m_stream << endl; // Empty line

}

void Serializer::ProcessMeshWheels2(Document::Module* module)
{
    if (module->meshwheels2.empty()) { return; }
    
    this->ResetPresets();

    m_stream << "meshwheels2" << "\n\n";

    for (MeshWheel2& def: module->meshwheels2)
    {
        this->ExportBaseMeshWheel(def);
    }

    m_stream << endl; // Empty line
}

void Serializer::ProcessCameras(Document::Module* module)
{
    if (module->cameras.empty())
    {
        return;
    }

    m_stream << "cameras\n\n";
    for (auto& camera: module->cameras)
    {
        m_stream
            << setw(m_node_id_width) << camera.center_node.Str() << ", "
            << setw(m_node_id_width) << camera.back_node.Str()   << ", "
            << setw(m_node_id_width) << camera.left_node.Str()   << "\n";
    }
    m_stream << "\n";
}

void Serializer::ProcessCinecam(Document::Module* module)
{
    if (module->cinecam.empty())
    {
        return;
    }

    m_stream << "cinecam" << endl << endl;

    for (auto itor = module->cinecam.begin(); itor != module->cinecam.end(); ++itor)
    {
        m_stream << "\t"
            << setw(m_float_width) << itor->position.x << ", "
            << setw(m_float_width) << itor->position.y << ", "
            << setw(m_float_width) << itor->position.z << ", ";
        m_stream
            << setw(m_node_id_width) << itor->nodes[0].Str() << ", "
            << setw(m_node_id_width) << itor->nodes[1].Str() << ", "
            << setw(m_node_id_width) << itor->nodes[2].Str() << ", "
            << setw(m_node_id_width) << itor->nodes[3].Str() << ", "
            << setw(m_node_id_width) << itor->nodes[4].Str() << ", "
            << setw(m_node_id_width) << itor->nodes[5].Str() << ", "
            << setw(m_node_id_width) << itor->nodes[6].Str() << ", "
            << setw(m_node_id_width) << itor->nodes[7].Str() << ", ";
        m_stream
            << setw(m_float_width) << itor->spring << ", "
            << setw(m_float_width) << itor->damping
            << endl;
    }

    m_stream << endl; // Empty line
}

void Serializer::ProcessBeams(Document::Module* module)
{
    if (module->beams.empty())
    {
        return;
    }

    // Group beams by presets
    std::map< BeamDefaults*, std::vector<Beam*> > beams_by_preset;
    auto itor_end = module->beams.end();
    for (auto itor = module->beams.begin(); itor != itor_end; ++itor)
    {
        Beam & beam = *itor;
        BeamDefaults* preset = beam.defaults.get();

        // Ensure preset is in map
        auto found_itor = beams_by_preset.find(preset);
        if (found_itor == beams_by_preset.end())
        {
            // Preset not in map, insert it and add beam.
            std::vector<Beam*> list;
            list.reserve(100);
            list.push_back(&beam);
            beams_by_preset.insert(std::make_pair(preset, list));
        }
        else
        {
            // Preset in map, just add beam.
            found_itor->second.push_back(&beam);
        }
    }

    // Write beams to file
    m_stream << "beams" << endl << endl;
    auto preset_itor_end = beams_by_preset.end();
    for (auto preset_itor = beams_by_preset.begin(); preset_itor != preset_itor_end; ++preset_itor)
    {
        // Write preset
        BeamDefaults* preset = preset_itor->first;
        ProcessBeamDefaults(preset);

        // Write beams
        std::vector<Beam*> & beam_list = preset_itor->second;
        auto beam_itor_end = beam_list.end();
        for (auto beam_itor = beam_list.begin(); beam_itor != beam_itor_end; ++beam_itor)
        {
            Beam & beam = *(*beam_itor);
            ProcessBeam(beam);
        }
    }

    // Empty line
    m_stream << endl;
}

void Serializer::ProcessShocks(Document::Module* module)
{
    if (module->shocks.empty())
    {
        return;
    }

    // Group beams by presets
    std::map< BeamDefaults*, std::vector<Shock*> > shocks_by_preset;
    auto itor_end = module->shocks.end(); 
    for (auto itor = module->shocks.begin(); itor != itor_end; ++itor)
    {
        Shock & shock = *itor;
        BeamDefaults* preset = shock.beam_defaults.get();

        // Ensure preset is in map
        auto found_itor = shocks_by_preset.find(preset);
        if (found_itor == shocks_by_preset.end())
        {
            // Preset not in map, insert it and add shock.
            std::vector<Shock*> list;
            list.reserve(100);
            list.push_back(&shock);
            shocks_by_preset.insert(std::make_pair(preset, list));
        }
        else
        {
            // Preset in map, just add shock.
            found_itor->second.push_back(&shock);
        }
    }

    // Write shocks to file
    m_stream << "shocks" << endl << endl;
    auto preset_itor_end = shocks_by_preset.end();
    for (auto preset_itor = shocks_by_preset.begin(); preset_itor != preset_itor_end; ++preset_itor)
    {
        // Write preset
        BeamDefaults* preset = preset_itor->first;
        ProcessBeamDefaults(preset);

        // Write shocks
        auto shock_list = preset_itor->second;
        auto shock_itor_end = shock_list.end();
        for (auto shock_itor = shock_list.begin(); shock_itor != shock_itor_end; ++shock_itor)
        {
            Shock & shock = *(*shock_itor);
            ProcessShock(shock);
        }
    }

    // Empty line
    m_stream << endl;
}

void Serializer::ProcessShocks2(Document::Module* module)
{
    if (module->shocks2.empty())
    {
        return;
    }

    // Group beams by presets
    std::map< BeamDefaults*, std::vector<Shock2*> > shocks_by_preset;
    auto itor_end = module->shocks2.end(); 
    for (auto itor = module->shocks2.begin(); itor != itor_end; ++itor)
    {
        Shock2 & shock = *itor;
        BeamDefaults* preset = shock.beam_defaults.get();

        // Ensure preset is in map
        auto found_itor = shocks_by_preset.find(preset);
        if (found_itor == shocks_by_preset.end())
        {
            // Preset not in map, insert it and add shock.
            std::vector<Shock2*> list;
            list.reserve(100);
            list.push_back(&shock);
            shocks_by_preset.insert(std::make_pair(preset, list));
        }
        else
        {
            // Preset in map, just add shock.
            found_itor->second.push_back(&shock);
        }
    }

    // Write shocks to file
    m_stream << "shocks2" << endl << endl;
    auto preset_itor_end = shocks_by_preset.end();
    for (auto preset_itor = shocks_by_preset.begin(); preset_itor != preset_itor_end; ++preset_itor)
    {
        // Write preset
        BeamDefaults* preset = preset_itor->first;
        ProcessBeamDefaults(preset);

        // Write shocks
        auto shock_list = preset_itor->second;
        auto shock_itor_end = shock_list.end();
        for (auto shock_itor = shock_list.begin(); shock_itor != shock_itor_end; ++shock_itor)
        {
            Shock2 & shock = *(*shock_itor);
            ProcessShock2(shock);
        }
    }

    // Empty line
    m_stream << endl;
}

void Serializer::ProcessShocks3(Document::Module* module)
{
    if (module->shocks3.empty())
    {
        return;
    }

    // Group beams by presets
    std::map< BeamDefaults*, std::vector<Shock3*> > shocks_by_preset;
    auto itor_end = module->shocks3.end(); 
    for (auto itor = module->shocks3.begin(); itor != itor_end; ++itor)
    {
        Shock3 & shock = *itor;
        BeamDefaults* preset = shock.beam_defaults.get();

        // Ensure preset is in map
        auto found_itor = shocks_by_preset.find(preset);
        if (found_itor == shocks_by_preset.end())
        {
            // Preset not in map, insert it and add shock.
            std::vector<Shock3*> list;
            list.reserve(100);
            list.push_back(&shock);
            shocks_by_preset.insert(std::make_pair(preset, list));
        }
        else
        {
            // Preset in map, just add shock.
            found_itor->second.push_back(&shock);
        }
    }

    // Write shocks to file
    m_stream << "shocks3" << endl << endl;
    auto preset_itor_end = shocks_by_preset.end();
    for (auto preset_itor = shocks_by_preset.begin(); preset_itor != preset_itor_end; ++preset_itor)
    {
        // Write preset
        BeamDefaults* preset = preset_itor->first;
        ProcessBeamDefaults(preset);

        // Write shocks
        auto shock_list = preset_itor->second;
        auto shock_itor_end = shock_list.end();
        for (auto shock_itor = shock_list.begin(); shock_itor != shock_itor_end; ++shock_itor)
        {
            Shock3 & shock = *(*shock_itor);
            ProcessShock3(shock);
        }
    }

    // Empty line
    m_stream << endl;
}

void Serializer::ProcessHydros(Document::Module* module)
{
    if (module->hydros.empty())
    {
        return;
    }

    // Group by presets
    std::map< BeamDefaults*, std::vector<Hydro*> > grouped_by_preset;
    auto itor_end = module->hydros.end(); 
    for (auto itor = module->hydros.begin(); itor != itor_end; ++itor)
    {
        Hydro & hydro = *itor;
        BeamDefaults* preset = hydro.beam_defaults.get();

        // Ensure preset is in map
        auto found_itor = grouped_by_preset.find(preset);
        if (found_itor == grouped_by_preset.end())
        {
            // Preset not in map, insert it and add hydro.
            std::vector<Hydro*> list;
            list.reserve(100);
            list.push_back(&hydro);
            grouped_by_preset.insert(std::make_pair(preset, list));
        }
        else
        {
            // Preset in map, just add hydro.
            found_itor->second.push_back(&hydro);
        }
    }

    // Write hydros to file
    m_stream << "hydros" << endl << endl;
    auto preset_itor_end = grouped_by_preset.end();
    for (auto preset_itor = grouped_by_preset.begin(); preset_itor != preset_itor_end; ++preset_itor)
    {
        // Write preset
        BeamDefaults* preset = preset_itor->first;
        ProcessBeamDefaults(preset);

        // Write hydros
        auto hydro_list = preset_itor->second;
        auto hydro_itor_end = hydro_list.end();
        for (auto hydro_itor = hydro_list.begin(); hydro_itor != hydro_itor_end; ++hydro_itor)
        {
            Hydro & hydro = *(*hydro_itor);
            ProcessHydro(hydro);
        }
    }

    // Empty line
    m_stream << endl << endl;
}

void Serializer::ProcessCommands2(Document::Module* module)
{
    if (module->commands2.empty())
    {
        return;
    }

    // Group by presets and _format_version
    std::map< BeamDefaults*, std::vector<Command2*> > commands_by_preset;
    auto itor_end = module->commands2.end(); 
    for (auto itor = module->commands2.begin(); itor != itor_end; ++itor)
    {
        Command2 & command = *itor;
        BeamDefaults* preset = command.beam_defaults.get();

        // Ensure preset is in map
        auto found_itor = commands_by_preset.find(preset);
        if (found_itor == commands_by_preset.end())
        {
            // Preset not in map, insert it and add command.
            std::vector<Command2*> list;
            list.reserve(100);
            list.push_back(&command);
            commands_by_preset.insert(std::make_pair(preset, list));
        }
        else
        {
            // Preset in map, just add command.
            found_itor->second.push_back(&command);
        }
    }

    // Write section "commands2" to file (commands from section "commands" are converted)
    m_stream << "commands2" << endl << endl;
    auto preset_itor_end = commands_by_preset.end();
    for (auto preset_itor = commands_by_preset.begin(); preset_itor != preset_itor_end; ++preset_itor)
    {
        // Write preset
        BeamDefaults* preset = preset_itor->first;
        ProcessBeamDefaults(preset);

        // Write hydros
        auto command_list = preset_itor->second;
        auto command_itor_end = command_list.end();
        for (auto command_itor = command_list.begin(); command_itor != command_itor_end; ++command_itor)
        {
            Command2 & command = *(*command_itor);
            ProcessCommand2(command);
        }
    }

    // Empty line
    m_stream << endl;
}

void Serializer::ProcessCommand2(Command2 & def)
{
    m_stream << "\t"
        << std::setw(m_node_id_width) << def.nodes[0].Str()    << ", "
        << std::setw(m_node_id_width) << def.nodes[1].Str()    << ", ";

    m_stream << std::setw(m_float_width) << def.shorten_rate        << ", ";
    m_stream << std::setw(m_float_width) << def.lengthen_rate       << ", ";
    m_stream << std::setw(m_float_width) << def.max_contraction     << ", "; // So-called 'shortbound'
    m_stream << std::setw(m_float_width) << def.max_extension       << ", "; // So-called 'longbound'
    m_stream << std::setw(m_command_key_width) << def.contract_key  << ", ";
    m_stream << std::setw(m_command_key_width) << def.extend_key    << ", ";

    // Options
    bool dummy = true;
    if (def.option_c_auto_center)   { m_stream << "c"; dummy = false; }
    if (def.option_f_not_faster)    { m_stream << "f"; dummy = false; }
    if (def.option_i_invisible)     { m_stream << "i"; dummy = false; }
    if (def.option_o_1press_center) { m_stream << "o"; dummy = false; }
    if (def.option_p_1press)        { m_stream << "p"; dummy = false; }
    if (def.option_r_rope)          { m_stream << "r"; dummy = false; }
    if (dummy)                      { m_stream << "n"; } // Placeholder, does nothing

    m_stream << ", ";

    // Description
    m_stream << (def.description.length() > 0 ? def.description : "_");
    
    // Inertia
    if (def.inertia.start_function != "" && def.inertia.stop_function != "")
    {
        m_stream << ", ";
        m_stream << std::setw(m_float_width) << def.inertia.start_delay_factor        << ", ";
        m_stream << std::setw(m_float_width) << def.inertia.stop_delay_factor         << ", ";
        m_stream << std::setw(m_inertia_function_width) << def.inertia.start_function << ", ";
        m_stream << std::setw(m_inertia_function_width) << def.inertia.stop_function  << ", ";
        m_stream << std::setw(m_float_width) << def.affect_engine                     << ", ";
        m_stream << std::setw(m_bool_width) << (def.needs_engine ? "true" : "false");
    }
    m_stream << endl;
}

void Serializer::ProcessHydro(Hydro & def)
{
    m_stream << "\t"
        << std::setw(m_node_id_width) << def.nodes[0].Str() << ", "
        << std::setw(m_node_id_width) << def.nodes[1].Str() << ", ";

    m_stream << std::setw(m_float_width) << def.lenghtening_factor      << ", ";

    // Options
    if (BITMASK_IS_1(def.options, Hydro::OPTION_j_INVISIBLE                )) m_stream << (char)HydroOption::j_INVISIBLE                ;
    if (BITMASK_IS_1(def.options, Hydro::OPTION_s_DISABLE_ON_HIGH_SPEED    )) m_stream << (char)HydroOption::s_DISABLE_ON_HIGH_SPEED    ;
    if (BITMASK_IS_1(def.options, Hydro::OPTION_a_INPUT_AILERON            )) m_stream << (char)HydroOption::a_INPUT_AILERON            ;
    if (BITMASK_IS_1(def.options, Hydro::OPTION_r_INPUT_RUDDER             )) m_stream << (char)HydroOption::r_INPUT_RUDDER             ;
    if (BITMASK_IS_1(def.options, Hydro::OPTION_e_INPUT_ELEVATOR           )) m_stream << (char)HydroOption::e_INPUT_ELEVATOR           ;
    if (BITMASK_IS_1(def.options, Hydro::OPTION_u_INPUT_AILERON_ELEVATOR   )) m_stream << (char)HydroOption::u_INPUT_AILERON_ELEVATOR   ;
    if (BITMASK_IS_1(def.options, Hydro::OPTION_v_INPUT_InvAILERON_ELEVATOR)) m_stream << (char)HydroOption::v_INPUT_InvAILERON_ELEVATOR;
    if (BITMASK_IS_1(def.options, Hydro::OPTION_x_INPUT_AILERON_RUDDER     )) m_stream << (char)HydroOption::x_INPUT_AILERON_RUDDER     ;
    if (BITMASK_IS_1(def.options, Hydro::OPTION_y_INPUT_InvAILERON_RUDDER  )) m_stream << (char)HydroOption::y_INPUT_InvAILERON_RUDDER  ;
    if (BITMASK_IS_1(def.options, Hydro::OPTION_g_INPUT_ELEVATOR_RUDDER    )) m_stream << (char)HydroOption::g_INPUT_ELEVATOR_RUDDER    ;
    if (BITMASK_IS_1(def.options, Hydro::OPTION_h_INPUT_InvELEVATOR_RUDDER )) m_stream << (char)HydroOption::h_INPUT_InvELEVATOR_RUDDER ;
    if (BITMASK_IS_1(def.options, Hydro::OPTION_n_INPUT_NORMAL             )) m_stream << (char)HydroOption::n_INPUT_NORMAL;
    if (def.options == 0) m_stream << (char)HydroOption::n_INPUT_NORMAL;
    m_stream << ", ";

    // Inertia
    Inertia & inertia = def.inertia;
    m_stream << std::setw(m_float_width) << inertia.start_delay_factor  << ", ";
    m_stream << std::setw(m_float_width) << inertia.stop_delay_factor;
    if (!inertia.start_function.empty())
    {
        m_stream << ", " << std::setw(m_inertia_function_width) << inertia.start_function;
        if (!inertia.stop_function.empty())
        {
            m_stream << ", " << std::setw(m_inertia_function_width) << inertia.stop_function;
        }
    }
    m_stream << endl;
}

void Serializer::ProcessShock(Shock & def)
{
    m_stream << "\t"
        << std::setw(m_node_id_width) << def.nodes[0].Str() << ", "
        << std::setw(m_node_id_width) << def.nodes[1].Str() << ", ";
    m_stream << std::setw(m_float_width) << def.spring_rate      << ", ";
    m_stream << std::setw(m_float_width) << def.damping          << ", ";
    m_stream << std::setw(m_float_width) << def.short_bound      << ", ";
    m_stream << std::setw(m_float_width) << def.long_bound       << ", ";
    m_stream << std::setw(m_float_width) << def.precompression   << ", ";

    // Options
    if (def.options == 0)
    {
        m_stream << "n"; // Placeholder
    }
    else
    {
        if (BITMASK_IS_1(def.options, Shock::OPTION_i_INVISIBLE))
        {
            m_stream << "i";
        }
        if (BITMASK_IS_1(def.options, Shock::OPTION_m_METRIC))
        {
            m_stream << "m";
        }
        if (BITMASK_IS_1(def.options, Shock::OPTION_L_ACTIVE_LEFT))
        {
            m_stream << "L";
        }
        if (BITMASK_IS_1(def.options, Shock::OPTION_R_ACTIVE_RIGHT))
        {
            m_stream << "R";
        }
    }

    // Empty line
    m_stream << endl;
}

void Serializer::ProcessShock2(Shock2 & def)
{
    m_stream << "\t"
        << std::setw(m_node_id_width) << def.nodes[0].Str() << ", "
        << std::setw(m_node_id_width) << def.nodes[1].Str() << ", ";

    m_stream << std::setw(m_float_width) << def.spring_in                  << ", ";
    m_stream << std::setw(m_float_width) << def.damp_in                    << ", ";
    m_stream << std::setw(m_float_width) << def.progress_factor_spring_in  << ", ";
    m_stream << std::setw(m_float_width) << def.progress_factor_damp_in    << ", ";

    m_stream << std::setw(m_float_width) << def.spring_out                 << ", ";
    m_stream << std::setw(m_float_width) << def.damp_out                   << ", ";
    m_stream << std::setw(m_float_width) << def.progress_factor_spring_out << ", ";
    m_stream << std::setw(m_float_width) << def.progress_factor_damp_out   << ", ";

    m_stream << std::setw(m_float_width) << def.short_bound                << ", ";
    m_stream << std::setw(m_float_width) << def.long_bound                 << ", ";
    m_stream << std::setw(m_float_width) << def.precompression             << ", ";

    // Options
    if (def.options != 0)
    {
        m_stream << "n"; // Placeholder
    }
    else
    {
        if (BITMASK_IS_1(def.options, Shock2::OPTION_i_INVISIBLE))
        {
            m_stream << "i";
        }
        if (BITMASK_IS_1(def.options, Shock2::OPTION_m_METRIC))
        {
            m_stream << "m";
        }
        if (BITMASK_IS_1(def.options, Shock2::OPTION_M_ABSOLUTE_METRIC))
        {
            m_stream << "M";
        }
        if (BITMASK_IS_1(def.options, Shock2::OPTION_s_SOFT_BUMP_BOUNDS))
        {
            m_stream << "s";
        }
    }

    // Empty line
    m_stream << endl;
}

void Serializer::ProcessShock3(Shock3 & def)
{
    m_stream << "\t"
        << std::setw(m_node_id_width) << def.nodes[0].Str() << ", "
        << std::setw(m_node_id_width) << def.nodes[1].Str() << ", ";

    m_stream << std::setw(m_float_width) << def.spring_in                  << ", ";
    m_stream << std::setw(m_float_width) << def.damp_in                    << ", ";
    m_stream << std::setw(m_float_width) << def.damp_in_slow               << ", ";
    m_stream << std::setw(m_float_width) << def.split_vel_in               << ", ";
    m_stream << std::setw(m_float_width) << def.damp_in_fast               << ", ";

    m_stream << std::setw(m_float_width) << def.spring_out                 << ", ";
    m_stream << std::setw(m_float_width) << def.damp_out                   << ", ";
    m_stream << std::setw(m_float_width) << def.damp_out_slow              << ", ";
    m_stream << std::setw(m_float_width) << def.split_vel_out              << ", ";
    m_stream << std::setw(m_float_width) << def.damp_out_fast              << ", ";

    m_stream << std::setw(m_float_width) << def.short_bound                << ", ";
    m_stream << std::setw(m_float_width) << def.long_bound                 << ", ";
    m_stream << std::setw(m_float_width) << def.precompression             << ", ";

    // Options
    if (def.options != 0)
    {
        m_stream << "n"; // Placeholder
    }
    else
    {
        if (BITMASK_IS_1(def.options, Shock3::OPTION_i_INVISIBLE))
        {
            m_stream << "i";
        }
        if (BITMASK_IS_1(def.options, Shock3::OPTION_m_METRIC))
        {
            m_stream << "m";
        }
        if (BITMASK_IS_1(def.options, Shock3::OPTION_M_ABSOLUTE_METRIC))
        {
            m_stream << "M";
        }
    }

    // Empty line
    m_stream << endl;
}

void Serializer::ProcessBeamDefaults(BeamDefaults* beam_defaults)
{
    if (beam_defaults == nullptr)
    {
        // reset everything to defaults
        m_stream << "\tset_beam_defaults       -1, -1, -1, -1, -1, -1, -1" << endl;
        m_stream << "\tset_beam_defaults_scale -1, -1, -1, -1" << endl;
        return;
    }

    m_stream << "\tset_beam_defaults       " // Extra spaces to align with "set_beam_defaults_scale"
        << beam_defaults->springiness           << ", "
        << beam_defaults->damping_constant      << ", "
        << beam_defaults->deformation_threshold << ", "
        << beam_defaults->breaking_threshold    << ", "
        << beam_defaults->visual_beam_diameter  << ", "
        << beam_defaults->beam_material_name    << ", "
        << beam_defaults->plastic_deform_coef 
        << endl;

    BeamDefaultsScale & scale = beam_defaults->scale;
    m_stream << "\tset_beam_defaults_scale "
        << scale.springiness << ", "
        << scale.damping_constant << ", "
        << scale.deformation_threshold_constant << ", "
        << scale.breaking_threshold_constant 
        << endl;
}

void Serializer::ProcessBeam(Beam & beam)
{
    m_stream << "\t"
        << std::setw(m_node_id_width) << beam.nodes[0].Str() << ", "
        << std::setw(m_node_id_width) << beam.nodes[1].Str() << ", ";

    // Options
    if (beam.options == 0u)
    {
        m_stream << "n"; // Placeholder
    }
    else
    {
        if (BITMASK_IS_1(beam.options, Beam::OPTION_i_INVISIBLE))
        {
            m_stream << "i";
        }
        if (BITMASK_IS_1(beam.options, Beam::OPTION_r_ROPE))
        {
            m_stream << "r";
        }
        if (BITMASK_IS_1(beam.options, Beam::OPTION_s_SUPPORT))
        {
            m_stream << "s";
        }
    }

    if (beam._has_extension_break_limit)
    {
        m_stream << ", " << beam.extension_break_limit;
    }
    
    m_stream << endl;
}

void Serializer::ProcessNodes(Document::Module* module)
{
    if (module->nodes.empty())
    {
        return;
    }

    // Group nodes by presets + find node-zero
    // TODO: Handle minimass presets!
    size_t num_named = 0;
    std::unordered_map< NodeDefaults*, std::vector<Node*> > nodes_by_presets;
    Node* node_zero = nullptr;
    auto itor_end = module->nodes.end(); 
    for (auto itor = module->nodes.begin(); itor != itor_end; ++itor)
    {
        Node & node = *itor;
        if (node.id.IsTypeNamed())
        {
            ++num_named;
        }

        // Check zero node
        if (node.id.IsValid() && node.id.Str() == "0" && node.id.Num() == 0)
        {
            if (node_zero != nullptr)
            {
                RoR::Str<200> msg;
                msg << "Multiple nodes '0' found when serializing actor '" << m_rig_def->name << "'";
                RoR::LogFormat("[RoR] Warning: '%s'", msg.ToCStr());
                RoR::App::GetGuiManager()->ShowMessageBox("Warning!", msg.ToCStr());
            }
            node_zero = &node;
            continue;
        }

        NodeDefaults* preset = node.node_defaults.get();

        // Ensure preset is in map
        auto found_itor = nodes_by_presets.find(preset);
        if (found_itor == nodes_by_presets.end())
        {
            // Preset not in map, insert it and add node.
            std::vector<Node*> list;
            list.reserve(100);
            list.push_back(&node);
            nodes_by_presets.insert(std::make_pair(preset, list));
        }
        else
        {
            // Preset in map, just add node.
            found_itor->second.push_back(&node);
        }
    }

    // == Write nodes to file ==
    m_stream << "nodes" << endl << endl;

    // Node zero first
    if (node_zero == nullptr)
    {
        RoR::Str<200> msg;
        msg << "N '0' not found when serializing actor '" << m_rig_def->name << "'";
        RoR::LogFormat("[RoR] Warning: '%s'", msg.ToCStr());
        RoR::App::GetGuiManager()->ShowMessageBox("Warning!", msg.ToCStr());
    }
    this->ProcessNodeDefaults(node_zero->node_defaults.get());
    this->ProcessNode(*node_zero);

    // Numbered nodes
    for (auto preset_pair: nodes_by_presets)
    {
        this->ProcessNodeDefaults(preset_pair.first);

        for (RigDef::Node* node: preset_pair.second)
        {
            if (node->id.IsTypeNumbered())
            {
                this->ProcessNode(*node);
            }
        }
    }

    // Named nodes
    if (num_named > 0)
    {
        m_stream << endl << endl << "nodes2" << endl;
        for (auto preset_pair: nodes_by_presets)
        {
            this->ProcessNodeDefaults(preset_pair.first);

            for (RigDef::Node* node: preset_pair.second)
            {
                if (node->id.IsTypeNamed())
                {
                    this->ProcessNode(*node);
                }
            }
        }
    }

    // Empty line
    m_stream << endl;
}

void Serializer::ProcessNodeDefaults(NodeDefaults* node_defaults)
{
    m_stream << "\tset_node_defaults ";
    if (node_defaults == nullptr)
    {
        m_stream << "-1, -1, -1, -1, n" << endl; // reset all to defaults
        return;
    }

    m_stream
        << node_defaults->load_weight << ", "
        << node_defaults->friction << ", "
        << node_defaults->volume << ", "
        << node_defaults->surface << ", ";

    ProcessNodeOptions(node_defaults->options);

    m_stream << endl;
}

void Serializer::ProcessNodeOptions(unsigned int options)
{
    if (options == 0)
    {
        m_stream << 'n'; // Placeholder (OPTION_n_MOUSE_GRAB constant is obsolete and will be removed)
        return;
    }

    if (options & Node::OPTION_m_NO_MOUSE_GRAB     ) { m_stream << 'm'; }
    if (options & Node::OPTION_f_NO_SPARKS         ) { m_stream << 'f'; }
    if (options & Node::OPTION_x_EXHAUST_POINT     ) { m_stream << 'x'; }
    if (options & Node::OPTION_y_EXHAUST_DIRECTION ) { m_stream << 'y'; }
    if (options & Node::OPTION_c_NO_GROUND_CONTACT ) { m_stream << 'c'; }
    if (options & Node::OPTION_h_HOOK_POINT        ) { m_stream << 'h'; }
    if (options & Node::OPTION_e_TERRAIN_EDIT_POINT) { m_stream << 'e'; }
    if (options & Node::OPTION_b_EXTRA_BUOYANCY    ) { m_stream << 'b'; }
    if (options & Node::OPTION_p_NO_PARTICLES      ) { m_stream << 'p'; }
    if (options & Node::OPTION_L_LOG               ) { m_stream << 'L'; }
    if (options & Node::OPTION_l_LOAD_WEIGHT       ) { m_stream << 'l'; }
}

void Serializer::ProcessDefaultMinimass(DefaultMinimass* def)
{
    m_stream << "\tset_default_minimass ";
    if (def != nullptr)
    {
        m_stream << def->min_mass_Kg << endl;
    }
    else
    {
        m_stream << "-1" << endl; // reset to default
    }
}

void Serializer::ProcessNode(Node & node)
{
    m_stream 
        << "\t" 
        << std::setw(m_node_id_width) << node.id.Str() << ", " 
        << std::setw(m_float_width) << node.position.x << ", " 
        << std::setw(m_float_width) << node.position.y << ", " 
        << std::setw(m_float_width) << node.position.z << ", ";
    
    ProcessNodeOptions(node.options);
    
    // Load mass
    if (node._has_load_weight_override)
    {
        m_stream << " " << node.load_weight_override;
    }
    m_stream << endl;
}

void Serializer::WriteFlags()
{
    if (m_rig_def->enable_advanced_deformation)
    {
        m_stream << "enable_advanced_deformation" << endl << endl;
    }
    if (m_rig_def->hide_in_chooser)
    {
        m_stream << "hideInChooser" << endl << endl;
    }
    if (m_rig_def->slide_nodes_connect_instantly)
    {
        m_stream << "slidenode_connect_instantly" << endl << endl;
    }
    if (m_rig_def->lockgroup_default_nolock)
    {
        m_stream << "lockgroup_default_nolock" << endl << endl;
    }
    if (m_rig_def->rollon)
    {
        m_stream << "rollon" << endl << endl;
    }
    if (m_rig_def->rescuer)
    {
        m_stream << "rescuer" << endl << endl;
    }
    if (m_rig_def->disable_default_sounds)
    {
        m_stream << "disabledefaultsounds" << endl << endl;
    }
    if (m_rig_def->forward_commands)
    {
        m_stream << "forwardcommands" << endl << endl;
    }
    if (m_rig_def->import_commands)
    {
        m_stream << "importcommands" << endl << endl;
    }
}

void Serializer::ProcessFileinfo(Document::Module* module)
{
    if (module->fileinfo.size() > 0)
    {
        Fileinfo& data = module->fileinfo[module->fileinfo.size() - 1];

        m_stream << "fileinfo ";
        m_stream << data.unique_id;
        m_stream << ", " << data.category_id;
        m_stream << ", " << data.file_version;

        m_stream << endl << endl;
    }
}

void Serializer::ProcessGuid(Document::Module* module)
{
    if (! module->guid.empty())
    {
        m_stream << "guid " << module->guid[module->guid.size() - 1].guid << endl << endl;
    }
}

void Serializer::ProcessDescription(Document::Module* module)
{
    if (module->description.size() != 0)
    {
        m_stream << "description";
        for (auto itor = module->description.begin(); itor != module->description.end(); ++itor)
        {
            std::string line = *itor;
            m_stream << endl << line;
        }
        m_stream << endl << "end_description" << endl << endl;
    }
}

void Serializer::ProcessAuthors(Document::Module* module)
{
    for (auto itor = module->author.begin(); itor != module->author.end(); ++itor)
    {
        Author & def = *itor;
        m_stream << "author " << def.type << " ";
        if (def._has_forum_account)
        {
            m_stream << def.forum_account_id << " ";
        }
        else
        {
            m_stream << "-1 ";
        }
        m_stream  << def.name << " " << def.email << endl;
    }
    m_stream << endl;
}

void Serializer::ProcessGlobals(Document::Module* module)
{
    if (module->globals.size() == 0)
    {
        return;
    }

    m_stream << "globals\n\t"
        << module->globals[0].dry_mass << ", "
        << module->globals[0].cargo_mass;
    if (!module->globals[0].material_name.empty())
    {
        m_stream << ", " << module->globals[0].material_name;
    }
    m_stream << endl << endl;
}

void Serializer::ResetPresets()
{
    m_current_node_defaults = nullptr;
    m_current_beam_defaults = nullptr;
    m_current_default_minimass = nullptr;
}

void Serializer::UpdatePresets(BeamDefaults* beam_defaults, NodeDefaults* node_defaults, DefaultMinimass* default_minimass)
{
    if (m_current_node_defaults != node_defaults)
    {
        m_current_node_defaults = node_defaults;
        this->ProcessNodeDefaults(node_defaults);
    }

    if (m_current_beam_defaults != beam_defaults)
    {
        m_current_beam_defaults = beam_defaults;
        this->ProcessBeamDefaults(beam_defaults);
    }

    if (m_current_default_minimass != default_minimass)
    {
        m_current_default_minimass = default_minimass;
        this->ProcessDefaultMinimass(default_minimass);
    }
}

