/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2021 Petr Ohlidal

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

#include "Actor.h"
#include "SimConstants.h"

namespace RigDef
{

const DataPos_t DATAPOS_INVALID = -1;
const NodeRef_t NODEREF_INVALID = "";

// --------------------------------
// Element definitions

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

const char * KeywordToString(Keyword keyword)
{
    /* NOTE: Maintain alphabetical order! */

    switch (keyword)
    {
        case Keyword::ADD_ANIMATION:        return "add_animation";
        case Keyword::AIRBRAKES:            return "airbrakes";
        case Keyword::ANIMATORS:            return "animators";
        case Keyword::ANTILOCKBRAKES:       return "antilockbrakes";
        case Keyword::AUTHOR:               return "author";
        case Keyword::AXLES:                return "axles";
        case Keyword::BEAMS:                return "beams";
        case Keyword::BRAKES:               return "brakes";
        case Keyword::CAB:                  return "cab";
        case Keyword::CAMERAS:              return "cameras";
        case Keyword::CAMERARAIL:           return "camerarail";
        case Keyword::CINECAM:              return "cinecam";
        case Keyword::COLLISIONBOXES:       return "collisionboxes";
        case Keyword::COMMANDS:             return "commands";
        case Keyword::COMMANDS2:            return "commands2";
        case Keyword::COMMENT:              return "comment";
        case Keyword::CONTACTERS:           return "contacters";
        case Keyword::CRUISECONTROL:        return "cruisecontrol";
        case Keyword::DESCRIPTION:          return "description";
        case Keyword::DETACHER_GROUP:       return "detacher_group";
        case Keyword::DISABLEDEFAULTSOUNDS: return "disabledefaultsounds";
        case Keyword::ENABLE_ADVANCED_DEFORMATION: return "enable_advanced_deformation";
        case Keyword::END:                  return "end";
        case Keyword::END_COMMENT:          return "end_comment";
        case Keyword::END_DESCRIPTION:      return "end_description";
        case Keyword::END_SECTION:          return "end_section";
        case Keyword::ENGINE:               return "engine";
        case Keyword::ENGOPTION:            return "engoption";
        case Keyword::ENGTURBO:             return "engturbo";
        case Keyword::EXHAUSTS:             return "exhausts";
        case Keyword::EXTCAMERA:            return "extcamera";
        case Keyword::FILEINFO:             return "fileinfo";
        case Keyword::FILEFORMATVERSION:    return "fileformatversion";
        case Keyword::FIXES:                return "fixes";
        case Keyword::FLARES:               return "flares";
        case Keyword::FLARES2:              return "flares2";
        case Keyword::FLEXBODIES:           return "flexbodies";
        case Keyword::FLEXBODY_CAMERA_MODE: return "flexbody_camera_mode";
        case Keyword::FLEXBODYWHEELS:       return "flexbodywheels";
        case Keyword::FORSET:               return "forset";
        case Keyword::FORWARDCOMMANDS:      return "forwardcommands";
        case Keyword::FUSEDRAG:             return "fusedrag";
        case Keyword::GLOBALS:              return "globals";
        case Keyword::GUID:                 return "guid";
        case Keyword::GUISETTINGS:          return "guisettings";
        case Keyword::HELP:                 return "help";
        case Keyword::HIDEINCHOOSER:        return "hideinchooser";
        case Keyword::HOOKGROUP:            return "hookgroup";
        case Keyword::HOOKS:                return "hooks";
        case Keyword::HYDROS:               return "hydros";
        case Keyword::IMPORTCOMMANDS:       return "importcommands";
        case Keyword::INTERAXLES:           return "interaxles";
        case Keyword::LOCKGROUPS:           return "lockgroups";
        case Keyword::LOCKGROUP_DEFAULT_NOLOCK: return "lockgroup_default_nolock";
        case Keyword::MANAGEDMATERIALS:     return "managedmaterials";
        case Keyword::MATERIALFLAREBINDINGS: return "materialflarebindings";
        case Keyword::MESHWHEELS:           return "meshwheels";
        case Keyword::MESHWHEELS2:          return "meshwheels2";
        case Keyword::MINIMASS:             return "minimass";
        case Keyword::NODES:                return "nodes";
        case Keyword::NODES2:               return "nodes2";
        case Keyword::PARTICLES:            return "particles";
        case Keyword::PISTONPROPS:          return "pistonprops";
        case Keyword::PROP_CAMERA_MODE:     return "prop_camera_mode";
        case Keyword::PROPS:                return "props";
        case Keyword::RAILGROUPS:           return "railgroups";
        case Keyword::RESCUER:              return "rescuer";
        case Keyword::RIGIDIFIERS:          return "rigidifiers";
        case Keyword::ROLLON:               return "rollon";
        case Keyword::ROPABLES:             return "ropables";
        case Keyword::ROPES:                return "ropes";
        case Keyword::ROTATORS:             return "rotators";
        case Keyword::ROTATORS2:            return "rotators2";
        case Keyword::SCREWPROPS:           return "screwprops";
        case Keyword::SECTION:              return "section";
        case Keyword::SECTIONCONFIG:        return "sectionconfig";
        case Keyword::SET_BEAM_DEFAULTS:    return "set_beam_defaults";
        case Keyword::SET_BEAM_DEFAULTS_SCALE: return "set_beam_defaults_scale";
        case Keyword::SET_COLLISION_RANGE:  return "set_collision_range";
        case Keyword::SET_DEFAULT_MINIMASS: return "set_default_minimass";
        case Keyword::SET_INERTIA_DEFAULTS: return "set_inertia_defaults";
        case Keyword::SET_MANAGEDMATERIALS_OPTIONS: return "set_managedmaterials_options";
        case Keyword::SET_NODE_DEFAULTS:    return "set_node_defaults";
        case Keyword::SET_SHADOWS:          return "set_shadows";
        case Keyword::SET_SKELETON_SETTINGS: return "set_skeleton_settings";
        case Keyword::SHOCKS:               return "shocks";
        case Keyword::SHOCKS2:              return "shocks2";
        case Keyword::SHOCKS3:              return "shocks3";
        case Keyword::SLIDENODE_CONNECT_INSTANTLY: return "slidenode_connect_instantly";
        case Keyword::SLIDENODES:           return "slidenodes";
        case Keyword::SLOPE_BRAKE:          return "SlopeBrake";
        case Keyword::SOUNDSOURCES:         return "soundsources";
        case Keyword::SOUNDSOURCES2:        return "soundsources2";
        case Keyword::SPEEDLIMITER:         return "speedlimiter";
        case Keyword::SUBMESH:              return "submesh";
        case Keyword::SUBMESH_GROUNDMODEL:  return "submesh_groundmodel";
        case Keyword::TEXCOORDS:            return "texcoords";
        case Keyword::TIES:                 return "ties";
        case Keyword::TORQUECURVE:          return "torquecurve";
        case Keyword::TRACTIONCONTROL:      return "tractioncontrol";
        case Keyword::TRANSFERCASE:         return "transfercase";
        case Keyword::TRIGGERS:             return "triggers";
        case Keyword::TURBOJETS:            return "turbojets";
        case Keyword::TURBOPROPS:           return "turboprops";
        case Keyword::TURBOPROPS2:          return "turboprops2";
        case Keyword::VIDEOCAMERA:          return "videocamera";
        case Keyword::WHEELDETACHERS:       return "wheeldetachers";
        case Keyword::WHEELS:               return "wheels";
        case Keyword::WHEELS2:              return "wheels2";
        case Keyword::WINGS:                return "wings";

        default:                             return "~Unknown~";
    }
}

bool Document::HasKeyword(Keyword _keyword)
{
    return std::find_if(lines.begin(), lines.end(),
        [_keyword](RigDef::Line const& l){ return l.keyword == _keyword; }) != lines.end();
}

} /* namespace RigDef */
