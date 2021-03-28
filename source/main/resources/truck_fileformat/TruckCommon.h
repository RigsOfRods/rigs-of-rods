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
/// @brief Presets in truck format. All values are initialized to -1 which means empty value.

#pragma once

namespace RoR {
namespace Truck {

/// A keyword which should be on it's own line.
#define E_KEYWORD_BLOCK(_NAME_)     "(^" _NAME_ "[[:blank:]]*$)?"

/// A keyword which should have values following it.
#define E_KEYWORD_INLINE(_NAME_)     "(^" _NAME_ ".*$)?"

// IMPORTANT! If you add a value here, you must also modify Keywords enum, it relies on positions in this regex
const std::string IDENTIFY_KEYWORD_REGEX_STRING
(
    E_KEYWORD_INLINE("add_animation")  /* Position 1 */           \
    E_KEYWORD_BLOCK("airbrakes")       /* Position 2 */           \
    E_KEYWORD_BLOCK("animators")       /* Position 3 etc... */    \
    E_KEYWORD_INLINE("AntiLockBrakes")                            \
    E_KEYWORD_BLOCK("axles")                                      \
    E_KEYWORD_INLINE("author")                                    \
    E_KEYWORD_BLOCK("backmesh")                                   \
    E_KEYWORD_BLOCK("beams")                                      \
    E_KEYWORD_BLOCK("brakes")                                     \
    E_KEYWORD_BLOCK("cab")                                        \
    E_KEYWORD_BLOCK("camerarail")                                 \
    E_KEYWORD_BLOCK("cameras")                                    \
    E_KEYWORD_BLOCK("cinecam")                                    \
    E_KEYWORD_BLOCK("collisionboxes")                             \
    E_KEYWORD_BLOCK("commands")                                   \
    E_KEYWORD_BLOCK("commands2")                                  \
    E_KEYWORD_BLOCK("comment")                                    \
    E_KEYWORD_BLOCK("contacters")                                 \
    E_KEYWORD_INLINE("cruisecontrol")                             \
    E_KEYWORD_BLOCK("description")                                \
    E_KEYWORD_INLINE("detacher_group")                            \
    E_KEYWORD_BLOCK("disabledefaultsounds")                       \
    E_KEYWORD_BLOCK("enable_advanced_deformation")                \
    E_KEYWORD_BLOCK("end")                                        \
    E_KEYWORD_BLOCK("end_comment")                                \
    E_KEYWORD_BLOCK("end_description")                            \
    E_KEYWORD_BLOCK("end_section")                                \
    E_KEYWORD_BLOCK("engine")                                     \
    E_KEYWORD_BLOCK("engoption")                                  \
    E_KEYWORD_BLOCK("engturbo")                                   \
    E_KEYWORD_BLOCK("envmap")                                     \
    E_KEYWORD_BLOCK("exhausts")                                   \
    E_KEYWORD_INLINE("extcamera")                                 \
    E_KEYWORD_INLINE("fileformatversion")                         \
    E_KEYWORD_INLINE("fileinfo")                                  \
    E_KEYWORD_BLOCK("fixes")                                      \
    E_KEYWORD_BLOCK("flares")                                     \
    E_KEYWORD_BLOCK("flares2")                                    \
    E_KEYWORD_BLOCK("flexbodies")                                 \
    E_KEYWORD_INLINE("flexbody_camera_mode")                      \
    E_KEYWORD_BLOCK("flexbodywheels")                             \
    E_KEYWORD_BLOCK("forset")                                     \
    E_KEYWORD_BLOCK("forwardcommands")                            \
    E_KEYWORD_BLOCK("fusedrag")                                   \
    E_KEYWORD_BLOCK("globals")                                    \
    E_KEYWORD_INLINE("guid")                                      \
    E_KEYWORD_BLOCK("guisettings")                                \
    E_KEYWORD_BLOCK("help")                                       \
    E_KEYWORD_BLOCK("hideInChooser")                              \
    E_KEYWORD_BLOCK("hookgroup")                                  \
    E_KEYWORD_BLOCK("hooks")                                      \
    E_KEYWORD_BLOCK("hydros")                                     \
    E_KEYWORD_BLOCK("importcommands")                             \
    E_KEYWORD_BLOCK("interaxles")                                 \
    E_KEYWORD_BLOCK("lockgroups")                                 \
    E_KEYWORD_BLOCK("lockgroup_default_nolock")                   \
    E_KEYWORD_BLOCK("managedmaterials")                           \
    E_KEYWORD_BLOCK("materialflarebindings")                      \
    E_KEYWORD_BLOCK("meshwheels")                                 \
    E_KEYWORD_BLOCK("meshwheels2")                                \
    E_KEYWORD_BLOCK("minimass")                                   \
    E_KEYWORD_BLOCK("nodecollision")                              \
    E_KEYWORD_BLOCK("nodes")                                      \
    E_KEYWORD_BLOCK("nodes2")                                     \
    E_KEYWORD_BLOCK("particles")                                  \
    E_KEYWORD_BLOCK("pistonprops")                                \
    E_KEYWORD_INLINE("prop_camera_mode")                          \
    E_KEYWORD_BLOCK("props")                                      \
    E_KEYWORD_BLOCK("railgroups")                                 \
    E_KEYWORD_BLOCK("rescuer")                                    \
    E_KEYWORD_BLOCK("rigidifiers")                                \
    E_KEYWORD_BLOCK("rollon")                                     \
    E_KEYWORD_BLOCK("ropables")                                   \
    E_KEYWORD_BLOCK("ropes")                                      \
    E_KEYWORD_BLOCK("rotators")                                   \
    E_KEYWORD_BLOCK("rotators2")                                  \
    E_KEYWORD_BLOCK("screwprops")                                 \
    E_KEYWORD_INLINE("section")                                   \
    E_KEYWORD_INLINE("sectionconfig")                             \
    E_KEYWORD_INLINE("set_beam_defaults")                         \
    E_KEYWORD_INLINE("set_beam_defaults_scale")                   \
    E_KEYWORD_INLINE("set_collision_range")                       \
    E_KEYWORD_INLINE("set_default_minimass")                      \
    E_KEYWORD_INLINE("set_inertia_defaults")                      \
    E_KEYWORD_INLINE("set_managedmaterials_options")              \
    E_KEYWORD_INLINE("set_node_defaults")                         \
    E_KEYWORD_BLOCK("set_shadows")                                \
    E_KEYWORD_INLINE("set_skeleton_settings")                     \
    E_KEYWORD_BLOCK("shocks")                                     \
    E_KEYWORD_BLOCK("shocks2")                                    \
    E_KEYWORD_BLOCK("shocks3")                                    \
    E_KEYWORD_BLOCK("slidenode_connect_instantly")                \
    E_KEYWORD_BLOCK("slidenodes")                                 \
    E_KEYWORD_INLINE("SlopeBrake")                                \
    E_KEYWORD_BLOCK("soundsources")                               \
    E_KEYWORD_BLOCK("soundsources2")                              \
    E_KEYWORD_INLINE("speedlimiter")                              \
    E_KEYWORD_BLOCK("submesh")                                    \
    E_KEYWORD_INLINE("submesh_groundmodel")                       \
    E_KEYWORD_BLOCK("texcoords")                                  \
    E_KEYWORD_BLOCK("ties")                                       \
    E_KEYWORD_BLOCK("torquecurve")                                \
    E_KEYWORD_INLINE("TractionControl")                           \
    E_KEYWORD_BLOCK("transfercase")                               \
    E_KEYWORD_BLOCK("triggers")                                   \
    E_KEYWORD_BLOCK("turbojets")                                  \
    E_KEYWORD_BLOCK("turboprops")                                 \
    E_KEYWORD_BLOCK("turboprops2")                                \
    E_KEYWORD_BLOCK("videocamera")                                \
    E_KEYWORD_BLOCK("wheeldetachers")                             \
    E_KEYWORD_BLOCK("wheels")                                     \
    E_KEYWORD_BLOCK("wheels2")                                    \
    E_KEYWORD_BLOCK("wings")
);

#undef E_KEYWORD_BLOCK
#undef E_KEYWORD_INLINE
#undef E_KEYWORD_INLINE_TOLERANT

// --------------------------------
// Enums

// IMPORTANT! If you add a value here, you must also modify Keywords enum, it relies on positions in this regex
enum Keyword
{
    KEYWORD_NONE = 0,

    KEYWORD_ADD_ANIMATION = 1,
    KEYWORD_AIRBRAKES,
    KEYWORD_ANIMATORS,
    KEYWORD_ANTI_LOCK_BRAKES,
    KEYWORD_AXLES,
    KEYWORD_AUTHOR,
    KEYWORD_BACKMESH,
    KEYWORD_BEAMS,
    KEYWORD_BRAKES,
    KEYWORD_CAB,
    KEYWORD_CAMERARAIL,
    KEYWORD_CAMERAS,
    KEYWORD_CINECAM,
    KEYWORD_COLLISIONBOXES,
    KEYWORD_COMMANDS,
    KEYWORD_COMMANDS2,
    KEYWORD_COMMENT,
    KEYWORD_CONTACTERS,
    KEYWORD_CRUISECONTROL,
    KEYWORD_DESCRIPTION,
    KEYWORD_DETACHER_GROUP,
    KEYWORD_DISABLEDEFAULTSOUNDS,
    KEYWORD_ENABLE_ADVANCED_DEFORMATION,
    KEYWORD_END,
    KEYWORD_END_COMMENT,
    KEYWORD_END_DESCRIPTION,
    KEYWORD_END_SECTION,
    KEYWORD_ENGINE,
    KEYWORD_ENGOPTION,
    KEYWORD_ENGTURBO,
    KEYWORD_ENVMAP,
    KEYWORD_EXHAUSTS,
    KEYWORD_EXTCAMERA,
    KEYWORD_FILEFORMATVERSION,
    KEYWORD_FILEINFO,
    KEYWORD_FIXES,
    KEYWORD_FLARES,
    KEYWORD_FLARES2,
    KEYWORD_FLEXBODIES,
    KEYWORD_FLEXBODY_CAMERA_MODE,
    KEYWORD_FLEXBODYWHEELS,
    KEYWORD_FORSET,
    KEYWORD_FORWARDCOMMANDS,
    KEYWORD_FUSEDRAG,
    KEYWORD_GLOBALS,
    KEYWORD_GUID,
    KEYWORD_GUISETTINGS,
    KEYWORD_HELP,
    KEYWORD_HIDE_IN_CHOOSER,
    KEYWORD_HOOKGROUP, // obsolete, ignored
    KEYWORD_HOOKS,
    KEYWORD_HYDROS,
    KEYWORD_IMPORTCOMMANDS,
    KEYWORD_INTERAXLES,
    KEYWORD_LOCKGROUPS,
    KEYWORD_LOCKGROUP_DEFAULT_NOLOCK,
    KEYWORD_MANAGEDMATERIALS,
    KEYWORD_MATERIALFLAREBINDINGS,
    KEYWORD_MESHWHEELS,
    KEYWORD_MESHWHEELS2,
    KEYWORD_MINIMASS,
    KEYWORD_NODECOLLISION,
    KEYWORD_NODES,
    KEYWORD_NODES2,
    KEYWORD_PARTICLES,
    KEYWORD_PISTONPROPS,
    KEYWORD_PROP_CAMERA_MODE,
    KEYWORD_PROPS,
    KEYWORD_RAILGROUPS,
    KEYWORD_RESCUER,
    KEYWORD_RIGIDIFIERS,
    KEYWORD_ROLLON,
    KEYWORD_ROPABLES,
    KEYWORD_ROPES,
    KEYWORD_ROTATORS,
    KEYWORD_ROTATORS2,
    KEYWORD_SCREWPROPS,
    KEYWORD_SECTION,
    KEYWORD_SECTIONCONFIG,
    KEYWORD_SET_BEAM_DEFAULTS,
    KEYWORD_SET_BEAM_DEFAULTS_SCALE,
    KEYWORD_SET_COLLISION_RANGE,
    KEYWORD_SET_DEFAULT_MINIMASS,
    KEYWORD_SET_INERTIA_DEFAULTS,
    KEYWORD_SET_MANAGEDMATERIALS_OPTIONS,
    KEYWORD_SET_NODE_DEFAULTS,
    KEYWORD_SET_SHADOWS,
    KEYWORD_SET_SKELETON_SETTINGS,
    KEYWORD_SHOCKS,
    KEYWORD_SHOCKS2,
    KEYWORD_SHOCKS3,
    KEYWORD_SLIDENODE_CONNECT_INSTANT,
    KEYWORD_SLIDENODES,
    KEYWORD_SLOPE_BRAKE,
    KEYWORD_SOUNDSOURCES,
    KEYWORD_SOUNDSOURCES2,
    KEYWORD_SPEEDLIMITER,
    KEYWORD_SUBMESH,
    KEYWORD_SUBMESH_GROUNDMODEL,
    KEYWORD_TEXCOORDS,
    KEYWORD_TIES,
    KEYWORD_TORQUECURVE,
    KEYWORD_TRACTION_CONTROL,
    KEYWORD_TRANSFER_CASE,
    KEYWORD_TRIGGERS,
    KEYWORD_TURBOJETS,
    KEYWORD_TURBOPROPS,
    KEYWORD_TURBOPROPS2,
    KEYWORD_VIDEOCAMERA,
    KEYWORD_WHEELDETACHERS,
    KEYWORD_WHEELS,
    KEYWORD_WHEELS2,
    KEYWORD_WINGS,
};

enum class DifferentialType: char
{
    DIFF_o_OPEN    = 'o',
    DIFF_l_LOCKED  = 'l',
    DIFF_s_SPLIT   = 's',
    DIFF_v_VISCOUS = 'v'
};

/** Syntax-sugar struct to hold enums */
struct Wheels
{
    enum Braking
    {
        BRAKING_NO                = 0,
        BRAKING_YES               = 1,
        BRAKING_DIRECTIONAL_LEFT  = 2,
        BRAKING_DIRECTIONAL_RIGHT = 3,
        BRAKING_ONLY_FOOT         = 4,

        BRAKING_INVALID           = 0xFFFFFFFF
    };

    enum Propulsion
    {
        PROPULSION_NONE     = 0,
        PROPULSION_FORWARD  = 1,
        PROPULSION_BACKWARD = 2,

        PROPULSION_INVALID  = 0xFFFFFFFF
    };
};


// --------------------------------
// Data structures

/// A line (any type) in truck file
struct Element
{
    int _num_args = -1;
};

/// Sequential ordering of elements
/// Each entry has `section` other than KEYWORD_NONE.
/// Optionally, it may reference a data array associated with the keyword.
struct SeqSection
{
    SeqSection(Keyword k, int idx): section(k), index(idx) {}

    Keyword section = KEYWORD_NONE;
    int index = -1;
};

/// Shared by 'set_default_inertia' and optional params in some sections ('hydros/commands/rotators...')
struct BaseInertia
{
    float start_delay_factor = -1.f;
    float stop_delay_factor = -1.f;
    std::string start_function = "-1";
    std::string stop_function = "-1";
};

/// optional params in some sections ('hydros/commands/rotators...')
struct OptionalInertia: public BaseInertia
{
};

} // namespace Truck
} // namespace RoR
