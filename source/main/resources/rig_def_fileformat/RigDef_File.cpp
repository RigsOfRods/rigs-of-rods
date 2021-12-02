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
/// @date   12/2013

#include "RigDef_File.h"
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

Command2::Command2():
    shorten_rate(0),
    lengthen_rate(0),
    max_contraction(0),
    max_extension(0),
    contract_key(0),
    extend_key(0),
    _format_version(1), /* 1 = 'commands', 2 = 'commands2' */
    affect_engine(1),
    detacher_group(0),
    needs_engine(true),
    plays_sound(true),
    option_i_invisible(false),
    option_r_rope(false),
    option_c_auto_center(false),
    option_f_not_faster(false),
    option_p_1press(false),
    option_o_1press_center(false)
{}

Engoption::Engoption():
    inertia(10.f),
    clutch_force(-1.f),
    shift_time(-1.f),
    clutch_time(-1.f),
    type(ENGINE_TYPE_t_TRUCK),
    post_shift_time(-1.f),
    idle_rpm(-1.f),
    stall_rpm(-1.f),
    max_idle_mixture(-1.f),
    min_idle_mixture(-1.f),
    braking_torque(-1.f)
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

/* -------------------------------------------------------------------------- */
/* File                                                                       */
/* -------------------------------------------------------------------------- */

const char * KeywordToString(Keyword keyword)
{
    // PLEASE maintain alphabetical order!

    switch (keyword)
    {
        case KEYWORD_ADD_ANIMATION:        return "add_animation";
        case KEYWORD_AIRBRAKES:            return "airbrakes";
        case KEYWORD_ANIMATORS:            return "animators";
        case KEYWORD_ANTILOCKBRAKES:       return "antilockbrakes";
        case KEYWORD_AUTHOR:               return "author";
        case KEYWORD_AXLES:                return "axles";
        case KEYWORD_BEAMS:                return "beams";
        case KEYWORD_BRAKES:               return "brakes";
        case KEYWORD_CAB:                  return "cab";
        case KEYWORD_CAMERAS:              return "cameras";
        case KEYWORD_CAMERARAIL:           return "camerarail";
        case KEYWORD_CINECAM:              return "cinecam";
        case KEYWORD_COLLISIONBOXES:       return "collisionboxes";
        case KEYWORD_COMMANDS:             return "commands";
        case KEYWORD_COMMANDS2:            return "commands2";
        case KEYWORD_COMMENT:              return "comment";
        case KEYWORD_CONTACTERS:           return "contacters";
        case KEYWORD_CRUISECONTROL:        return "cruisecontrol";
        case KEYWORD_DESCRIPTION:          return "description";
        case KEYWORD_DETACHER_GROUP:       return "detacher_group";
        case KEYWORD_DISABLEDEFAULTSOUNDS: return "disabledefaultsounds";
        case KEYWORD_ENABLE_ADVANCED_DEFORMATION: return "enable_advanced_deformation";
        case KEYWORD_END:                  return "end";
        case KEYWORD_END_COMMENT:          return "end_comment";
        case KEYWORD_END_DESCRIPTION:      return "end_description";
        case KEYWORD_END_SECTION:          return "end_section";
        case KEYWORD_ENGINE:               return "engine";
        case KEYWORD_ENGOPTION:            return "engoption";
        case KEYWORD_ENGTURBO:             return "engturbo";
        case KEYWORD_EXHAUSTS:             return "exhausts";
        case KEYWORD_EXTCAMERA:            return "extcamera";
        case KEYWORD_FILEINFO:             return "fileinfo";
        case KEYWORD_FILEFORMATVERSION:    return "fileformatversion";
        case KEYWORD_FIXES:                return "fixes";
        case KEYWORD_FLARES:               return "flares";
        case KEYWORD_FLARES2:              return "flares2";
        case KEYWORD_FLEXBODIES:           return "flexbodies";
        case KEYWORD_FLEXBODY_CAMERA_MODE: return "flexbody_camera_mode";
        case KEYWORD_FLEXBODYWHEELS:       return "flexbodywheels";
        case KEYWORD_FORSET:               return "forset";
        case KEYWORD_FORWARDCOMMANDS:      return "forwardcommands";
        case KEYWORD_FUSEDRAG:             return "fusedrag";
        case KEYWORD_GLOBALS:              return "globals";
        case KEYWORD_GUID:                 return "guid";
        case KEYWORD_GUISETTINGS:          return "guisettings";
        case KEYWORD_HELP:                 return "help";
        case KEYWORD_HIDEINCHOOSER:        return "hideinchooser";
        case KEYWORD_HOOKGROUP:            return "hookgroup";
        case KEYWORD_HOOKS:                return "hooks";
        case KEYWORD_HYDROS:               return "hydros";
        case KEYWORD_IMPORTCOMMANDS:       return "importcommands";
        case KEYWORD_INTERAXLES:           return "interaxles";
        case KEYWORD_LOCKGROUPS:           return "lockgroups";
        case KEYWORD_LOCKGROUP_DEFAULT_NOLOCK: return "lockgroup_default_nolock";
        case KEYWORD_MANAGEDMATERIALS:     return "managedmaterials";
        case KEYWORD_MATERIALFLAREBINDINGS: return "materialflarebindings";
        case KEYWORD_MESHWHEELS:           return "meshwheels";
        case KEYWORD_MESHWHEELS2:          return "meshwheels2";
        case KEYWORD_MINIMASS:             return "minimass";
        case KEYWORD_NODES:                return "nodes";
        case KEYWORD_NODES2:               return "nodes2";
        case KEYWORD_PARTICLES:            return "particles";
        case KEYWORD_PISTONPROPS:          return "pistonprops";
        case KEYWORD_PROP_CAMERA_MODE:     return "prop_camera_mode";
        case KEYWORD_PROPS:                return "props";
        case KEYWORD_RAILGROUPS:           return "railgroups";
        case KEYWORD_RESCUER:              return "rescuer";
        case KEYWORD_RIGIDIFIERS:          return "rigidifiers";
        case KEYWORD_ROLLON:               return "rollon";
        case KEYWORD_ROPABLES:             return "ropables";
        case KEYWORD_ROPES:                return "ropes";
        case KEYWORD_ROTATORS:             return "rotators";
        case KEYWORD_ROTATORS2:            return "rotators2";
        case KEYWORD_SCREWPROPS:           return "screwprops";
        case KEYWORD_SECTION:              return "section";
        case KEYWORD_SECTIONCONFIG:        return "sectionconfig";
        case KEYWORD_SET_BEAM_DEFAULTS:    return "set_beam_defaults";
        case KEYWORD_SET_BEAM_DEFAULTS_SCALE: return "set_beam_defaults_scale";
        case KEYWORD_SET_COLLISION_RANGE:  return "set_collision_range";
        case KEYWORD_SET_DEFAULT_MINIMASS: return "set_default_minimass";
        case KEYWORD_SET_INERTIA_DEFAULTS: return "set_inertia_defaults";
        case KEYWORD_SET_MANAGEDMATERIALS_OPTIONS: return "set_managedmaterials_options";
        case KEYWORD_SET_NODE_DEFAULTS:    return "set_node_defaults";
        case KEYWORD_SET_SHADOWS:          return "set_shadows";
        case KEYWORD_SET_SKELETON_SETTINGS: return "set_skeleton_settings";
        case KEYWORD_SHOCKS:               return "shocks";
        case KEYWORD_SHOCKS2:              return "shocks2";
        case KEYWORD_SHOCKS3:              return "shocks3";
        case KEYWORD_SLIDENODE_CONNECT_INSTANTLY: return "slidenode_connect_instantly";
        case KEYWORD_SLIDENODES:           return "slidenodes";
        case KEYWORD_SLOPE_BRAKE:          return "SlopeBrake";
        case KEYWORD_SOUNDSOURCES:         return "soundsources";
        case KEYWORD_SOUNDSOURCES2:        return "soundsources2";
        case KEYWORD_SPEEDLIMITER:         return "speedlimiter";
        case KEYWORD_SUBMESH:              return "submesh";
        case KEYWORD_SUBMESH_GROUNDMODEL:  return "submesh_groundmodel";
        case KEYWORD_TEXCOORDS:            return "texcoords";
        case KEYWORD_TIES:                 return "ties";
        case KEYWORD_TORQUECURVE:          return "torquecurve";
        case KEYWORD_TRACTIONCONTROL:      return "tractioncontrol";
        case KEYWORD_TRANSFERCASE:         return "transfercase";
        case KEYWORD_TRIGGERS:             return "triggers";
        case KEYWORD_TURBOJETS:            return "turbojets";
        case KEYWORD_TURBOPROPS:           return "turboprops";
        case KEYWORD_TURBOPROPS2:          return "turboprops2";
        case KEYWORD_VIDEOCAMERA:          return "videocamera";
        case KEYWORD_WHEELDETACHERS:       return "wheeldetachers";
        case KEYWORD_WHEELS:               return "wheels";
        case KEYWORD_WHEELS2:              return "wheels2";
        case KEYWORD_WINGS:                return "wings";

        default:                           return "";
    }
}

File::Module::Module(Ogre::String const & name):
    name(name)
{}

File::File():
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
    root_module = std::make_shared<File::Module>(ROOT_MODULE_NAME); // Required to exist.
}

} /* namespace RigDef */
