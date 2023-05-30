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

/**
    @file   RigDef_Regexes.h
    @author Petr Ohlidal
    @date   12/2013

    @brief Defines regular expressions to verify and pull data from rig-def file. 'E' stands for Expression

    REGEX CRASH COURSE:
                  ^  Start of line
                  $  End of line
                  .  Wildcard - matches any character
                 \.  Escaped special character '.', matches '.'
                     IMPORTANT! \ is also an escape character in C, so it needs to be doubled \\ to use in string constants
         something*  Match something 0 or more times
         something+  Match something 1 or more times
         something?  Match something 0 or 1 times
            (stuff)  Subsection. Can be manipulates as 'something'. Also works as a Capture
                  |  Alternation character. (abc|cba) means abc OR cba.
      [:some_type:]  A POSIX character class. Read this: http://www.boost.org/doc/libs/1_54_0/libs/regex/doc/html/boost_regex/syntax/character_classes.html

    REGEX NAMING CONVENTION:
        E_* ~ Re-usable regexes or macros.
        SECTION_* ~ File sections
        DIRECTIVE_* ~ File directives
        INLINE_SECTION_* ~ Inline sections of file
        [ANYTHING ELSE] ~ Helpers. It's forbidden to use the above reserved prefixes for other purposes.

*/

#pragma once

#include <regex>

namespace RigDef
{

namespace Regexes
{

// -------------------------------------------------------------------------- //
// Repeatedly used bits of regular expressions                                //
// Using #define-s instead of const vars to utilize C-string concatenation    //
// REMEMBER! Do not use captures () - they would mess up result order. Use |  //
// REMEMBER! Expressions using | MUST be enclosed in E_CAPTURE()              //
// -------------------------------------------------------------------------- //

#define E_SLASH "\\/"

#define E_OPTIONAL_SPACE "[[:blank:]]*"

#define E_OR "|"

#define E_TRAILING_WHITESPACE "[[:blank:]]*$"

#define E_LEADING_WHITESPACE "^[[:blank:]]*"

#define E_DELIMITER_SPACE "[[:blank:]]+"

#define E_DELIMITER_CLASSIC "[[:blank:],:|]+" // Separators: space/comma/colon/pipe, see also `IsSeparator()` in RigDef_Parser.cpp. This is what original parser did.

#define E_NODE_ID "[[:alnum:]_-]+"

// --------------------------------------------------------------------------
// Macros                                                                    
// --------------------------------------------------------------------------

/// Encloses expression in 'capture' marks.
#define E_CAPTURE(_REGEXP_) \
    "(" _REGEXP_ ")"

/// Encloses expression in 'capture' marks with ? (zero or one time) mark.
#define E_CAPTURE_OPTIONAL(_REGEXP_) \
    "(" _REGEXP_ ")?"

/// A keyword which should be on it's own line. Used in IDENTIFY_KEYWORD.
#define E_KEYWORD_BLOCK(_NAME_) \
    "(^" _NAME_ "[[:blank:]]*$)?"

/// A keyword which should have values following it. Used in IDENTIFY_KEYWORD.
#define E_KEYWORD_INLINE(_NAME_) \
    "(^" _NAME_ E_DELIMITER_CLASSIC ".*$)?"

/// Actual regex definition macro.
#define DEFINE_REGEX(_NAME_,_REGEXP_) \
    const std::regex _NAME_ = std::regex( _REGEXP_, std::regex::ECMAScript);

#define DEFINE_REGEX_IGNORECASE(_NAME_,_REGEXP_) \
    const std::regex _NAME_ = std::regex( _REGEXP_, std::regex::ECMAScript | std::regex::icase);

// -------------------------------------------------------------------------- //
// Utility regexes                                                            //
// -------------------------------------------------------------------------- //

// IMPORTANT! If you add a value here, you must also modify File::Keywords enum, it relies on positions in this regex
#define IDENTIFY_KEYWORD_REGEX_STRING                             \
    E_KEYWORD_INLINE("add_animation")  /* Position 1 */           \
    E_KEYWORD_BLOCK("airbrakes")       /* Position 2 */           \
    E_KEYWORD_BLOCK("animators")       /* Position 3 etc... */    \
    E_KEYWORD_INLINE("AntiLockBrakes")                            \
    E_KEYWORD_INLINE("author")                                    \
    E_KEYWORD_BLOCK("axles")                                      \
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
    E_KEYWORD_INLINE("default_skin")                              \
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
    E_KEYWORD_BLOCK("flares3")                                    \
    E_KEYWORD_BLOCK("flexbodies")                                 \
    E_KEYWORD_INLINE("flexbody_camera_mode")                      \
    E_KEYWORD_BLOCK("flexbodywheels")                             \
    E_KEYWORD_INLINE("forset")                                    \
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
    E_KEYWORD_BLOCK("scripts")                                    \
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

DEFINE_REGEX(            IDENTIFY_KEYWORD_RESPECT_CASE, IDENTIFY_KEYWORD_REGEX_STRING )
DEFINE_REGEX_IGNORECASE( IDENTIFY_KEYWORD_IGNORE_CASE,  IDENTIFY_KEYWORD_REGEX_STRING )

#define E_2xCAPTURE_TRAILING_COMMENT \
    E_OPTIONAL_SPACE                 \
    E_CAPTURE_OPTIONAL(              \
        E_CAPTURE(                   \
            ";"                      \
            E_OR                     \
            E_SLASH E_SLASH          \
        )                            \
        ".*"                         \
    )                                \
    "$"

// -------------------------------------------------------------------------- //
// Regexes for parsing sections                                               //
// -------------------------------------------------------------------------- //

DEFINE_REGEX( PARSE_ANIMATORS_NUMBERED_KEYWORD,
    E_LEADING_WHITESPACE
    E_CAPTURE( "throttle|rpm|aerotorq|aeropit|aerostatus" )
    E_CAPTURE( "[[:digit:]]" )
    E_TRAILING_WHITESPACE
    );

DEFINE_REGEX( SECTION_AXLES_PROPERTY,
    E_LEADING_WHITESPACE
    E_CAPTURE_OPTIONAL( // #1
        E_OPTIONAL_SPACE
        "w(1|2)" // #2 wheel number
        "\\("
            E_CAPTURE( E_NODE_ID ) // #3 Node 1
            E_DELIMITER_SPACE
            E_CAPTURE( E_NODE_ID ) // #4 Node 2
        "\\)"
        E_OPTIONAL_SPACE
    )
    E_CAPTURE_OPTIONAL( // #5
        E_OPTIONAL_SPACE
        "d\\("
            E_CAPTURE( "[olsv]*" ) // #6 Differential modes
        "\\)"
        E_OPTIONAL_SPACE
    )
    E_2xCAPTURE_TRAILING_COMMENT
    );



// -------------------------------------------------------------------------- //
// Cleanup                                                                    //
// -------------------------------------------------------------------------- //


#undef E_SLASH
#undef E_TRAILING_WHITESPACE
#undef E_LEADING_WHITESPACE
#undef E_DELIMITER_SPACE
#undef E_OPTIONAL_SPACE
#undef E_OR
#undef E_NODE_ID
#undef E_CAPTURE
#undef E_CAPTURE_OPTIONAL
#undef DEFINE_REGEX
#undef E_2xCAPTURE_TRAILING_COMMENT

} // namespace Regexes

} // namespace RigDef
