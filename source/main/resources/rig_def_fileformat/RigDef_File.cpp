/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2021 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public RoR::WheelSide version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public RoR::WheelSide for more details.

    You should have received a copy of the GNU General Public RoR::WheelSide
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/// @file
/// @author Petr Ohlidal
/// @date   12/2013

#include "RigDef_File.h"

#include "Actor.h"
#include "CacheSystem.h"
#include "SimConstants.h"

namespace RigDef
{

const char* ROOT_MODULE_NAME = "_Root_"; // Static

/* -------------------------------------------------------------------------- */
/* Sections                                                                   */
/*                                                                            */
/* This is the place to set defaults.                                         */
/* -------------------------------------------------------------------------- */

Airbrake::Airbrake():
    offset(Ogre::Vector3::ZERO),
    width(0),
    height(0),
    max_inclination_angle(0),
    texcoord_x1(0),
    texcoord_x2(0),
    texcoord_y1(0),
    texcoord_y2(0),
    lift_coefficient(1.f) // This is default
{}

AntiLockBrakes::AntiLockBrakes():
    regulation_force(0),
    min_speed(0),
    pulse_per_sec(0),
    attr_is_on(true),
    attr_no_dashboard(false),
    attr_no_toggle(false)
{}

Engturbo::Engturbo() :
tinertiaFactor(1),
nturbos(1),
param1(9999), //used for default values
param2(9999),
param3(9999),
param4(9999),
param5(9999),
param6(9999), 
param7(9999),
param8(9999),
param9(9999),
param10(9999),
param11(9999)
{}

Fusedrag::Fusedrag():
    autocalc(false),
    approximate_width(0),
    area_coefficient(1.f),  // Default
    airfoil_name("NACA0009.afl")
{}

Hook::Hook():
    flag_self_lock (false),
    flag_auto_lock (false),
    flag_no_disable(false),
    flag_no_rope   (false),
    flag_visible   (false),
    option_hook_range(HOOK_RANGE_DEFAULT),
    option_speed_coef(1.0f),
    option_max_force(HOOK_FORCE_DEFAULT),
    option_hookgroup(-1),
    option_lockgroup(NODE_LOCKGROUP_DEFAULT),
    option_timer(HOOK_LOCK_TIMER_DEFAULT),
    option_min_range_meters(0.f)
{}

NodeDefaults::NodeDefaults():
    load_weight(-1.f),
    friction(1),
    volume(1),
    surface(1),
    options(0)
{}

Shock2::Shock2():
    spring_in(0),
    damp_in(0),
    progress_factor_spring_in(0),
    progress_factor_damp_in(0),
    spring_out(0),
    damp_out(0),
    progress_factor_spring_out(0),
    progress_factor_damp_out(0),
    short_bound(0),
    long_bound(0),
    precompression(0),
    options(0),
    detacher_group(0)
{}

Shock3::Shock3():
    spring_in(0),
    damp_in(0),
    damp_in_slow(0),
    split_vel_in(0),
    damp_in_fast(0),
    spring_out(0),
    damp_out(0),
    damp_out_slow(0),
    split_vel_out(0),
    damp_out_fast(0),
    short_bound(0),
    long_bound(0),
    precompression(0),
    options(0),
    detacher_group(0)
{}

TractionControl::TractionControl():
    regulation_force(0),
    wheel_slip(0),
    fade_speed(0),
    pulse_per_sec(0),
    attr_is_on(false),
    attr_no_dashboard(false),
    attr_no_toggle(false)
{}

VideoCamera::VideoCamera():
    offset(Ogre::Vector3::ZERO),
    rotation(Ogre::Vector3::ZERO),
    field_of_view(0),
    texture_width(0),
    texture_height(0),
    min_clip_distance(0),
    max_clip_distance(0),
    camera_role(0),
    camera_mode(0)
{}

void Animation::AddMotorSource(unsigned int source, unsigned int motor)
{
    Animation::MotorSource motor_source;
    motor_source.source = source;
    motor_source.motor = motor;
    this->motor_sources.push_back(motor_source);
}

/*static*/ const char* ManagedMaterial::TypeToStr(ManagedMaterialType type)
{
    switch (type)
    {
    case ManagedMaterialType::FLEXMESH_STANDARD:     return "flexmesh_standard";
    case ManagedMaterialType::FLEXMESH_TRANSPARENT:  return "flexmesh_transparent";
    case ManagedMaterialType::MESH_STANDARD:         return "mesh_standard";
    case ManagedMaterialType::MESH_TRANSPARENT:      return "mesh_transparent";
    default:                                         return "";
    }
}

Document::Module::Module(Ogre::String const & name):
    name(name)
{}

Document::Document():
    hide_in_chooser(false),
    enable_advanced_deformation(false),
    rollon(false),
    forward_commands(false),
    import_commands(false),
    lockgroup_default_nolock(false),
    rescuer(false),
    disable_default_sounds(false),
    slide_nodes_connect_instantly(false)
{
    root_module = std::make_shared<Document::Module>(ROOT_MODULE_NAME); // Required to exist.
}

} /* namespace RigDef */
