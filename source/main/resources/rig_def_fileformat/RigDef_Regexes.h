/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2016 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
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

// CHARACTERS

#define E_BACKSLASH "\\\\" // Each backslash \ must be escaped in C++ (\\) and then in regex (\\\\)

#define E_SLASH "\\/"

// NUMBERS

#define E_DECIMAL_NUMBER "-?[[:digit:]]+"

#define E_POSITIVE_DECIMAL_NUMBER "[[:digit:]]+"

#define E_NEGATIVE_DECIMAL_NUMBER "-[[:digit:]]+"

#define E_REAL_NUMBER_WITH_EXPONENT "-?[[:digit:]]*\\.[[:digit:]]+[eE][-+]?[[:digit:]]+"

#define E_REAL_NUMBER_WITH_EXPONENT_NO_FRACTION "-?[[:digit:]]*[eE][-+]?[[:digit:]]+"

// NOTE: Intentionally accepting format "1." for backwards compatibility, observed in http://www.rigsofrods.org/repository/view/2389
#define E_REAL_NUMBER_SIMPLE "-?[[:digit:]]*\\.[[:digit:]]*"

//NOTE: Uses |, MUST be enclosed in E_CAPTURE()
#define E_REAL_NUMBER \
    E_REAL_NUMBER_WITH_EXPONENT             E_OR \
    E_REAL_NUMBER_WITH_EXPONENT_NO_FRACTION E_OR \
    E_REAL_NUMBER_SIMPLE                    E_OR \
    E_DECIMAL_NUMBER 

#define E_MINUS_ONE_REAL "-1\\.[0]*|-1"   // Uses |, MUST be enclosed in E_CAPTURE()

// STRINGS

#define E_ILLEGAL_TRAILING_STRING "[^[:blank:]]+.*"

#define E_STRING_NO_SPACES "[[:alnum:]" E_BACKSLASH E_SLASH "@.{}()+,;_\\-]+"

#define E_STRING_NO_LEADING_DIGIT "[^[:blank:][:digit:]]+[^[:blank:]]*"

#define E_STRING_ANYTHING_BUT_WHITESPACE "[^[:blank:]]+"

#define E_STRING_ANYTHING_BUT_DELIMITER "[^[:blank:],]+"

#define E_STRING_ALNUM_HYPHENS_USCORES_ONLY "[[:alnum:]_-]+"

#define E_OPTIONAL_SPACE "[[:blank:]]*"

// DELIMITERS

#define E_OR "|"

#define E_TRAILING_WHITESPACE "[[:blank:]]*$"

#define E_LEADING_WHITESPACE "^[[:blank:]]*"

#define E_DELIMITER_SPACE "[[:blank:]]+"

#define E_DELIMITER_COMMA "[[:blank:]]*,[[:blank:]]*"

#define E_DELIMITER_COLON "[[:blank:]]*:[[:blank:]]*"

// Universal delimiter - at least 1 space or comma
// Multiple delimiters in row are merged into one (backwards compatibility)
#define E_DELIMITER "[[:blank:],]+"

// VALUE TYPES

#define E_BOOLEAN "true|yes|1|false|no|0" // Uses |, MUST be enclosed in E_CAPTURE()

#define E_NODE_ID E_STRING_ALNUM_HYPHENS_USCORES_ONLY

#define E_NODE_ID_OPTIONAL E_NODE_ID E_OR E_MINUS_ONE_REAL

#define E_INERTIA_FUNCTION E_STRING_ALNUM_HYPHENS_USCORES_ONLY

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
    "(^" _NAME_ E_DELIMITER_SPACE ".*$)?"
    
/// Inline keyword, tolerant version: keyword and values can be delimited by either space or comma
#define E_KEYWORD_INLINE_TOLERANT(_NAME_) \
    "(^" _NAME_ "[[:blank:],]+" ".*$)?"

#define E_DELIMITED_LIST( _VALUE_, _DELIMITER_ ) \
    E_CAPTURE(                                   \
        E_OPTIONAL_SPACE                         \
        _VALUE_                                  \
        E_OPTIONAL_SPACE                         \
        _DELIMITER_                              \
    ) "*"                                        \
    E_CAPTURE(                                   \
        E_OPTIONAL_SPACE                         \
        _VALUE_                                  \
        E_OPTIONAL_SPACE                         \
    ) "+"

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
    /* E_KEYWORD_BLOCK("advdrag") ~~ Not supported yet */         \
    E_KEYWORD_INLINE_TOLERANT("add_animation")  /* Position 1 */  \
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
    E_KEYWORD_BLOCK("contacters")                                 \
    E_KEYWORD_INLINE("cruisecontrol")                             \
    E_KEYWORD_BLOCK("description")                                \
    E_KEYWORD_INLINE("detacher_group")                            \
    E_KEYWORD_BLOCK("disabledefaultsounds")                       \
    E_KEYWORD_BLOCK("enable_advanced_deformation")                \
    E_KEYWORD_BLOCK("end")                                        \
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
    E_KEYWORD_INLINE("set_inertia_defaults")                      \
    E_KEYWORD_INLINE("set_managedmaterials_options")              \
    E_KEYWORD_INLINE("set_node_defaults")                         \
    E_KEYWORD_BLOCK("set_shadows")                                \
    E_KEYWORD_INLINE("set_skeleton_settings")                     \
    E_KEYWORD_BLOCK("shocks")                                     \
    E_KEYWORD_BLOCK("shocks2")                                    \
    E_KEYWORD_BLOCK("slidenode_connect_instantly")                \
    E_KEYWORD_BLOCK("slidenodes")                                 \
    E_KEYWORD_INLINE("SlopeBrake")                                \
    E_KEYWORD_BLOCK("soundsources")                               \
    E_KEYWORD_BLOCK("soundsources2")                              \
    E_KEYWORD_INLINE("speedlimiter")                              \
    /* E_KEYWORD_BLOCK("soundsources3") ~~ Not supported yet */   \
    E_KEYWORD_BLOCK("submesh")                                    \
    E_KEYWORD_INLINE("submesh_groundmodel")                       \
    E_KEYWORD_BLOCK("texcoords")                                  \
    E_KEYWORD_BLOCK("ties")                                       \
    E_KEYWORD_BLOCK("torquecurve")                                \
    E_KEYWORD_INLINE("TractionControl")                           \
    E_KEYWORD_BLOCK("triggers")                                   \
    E_KEYWORD_BLOCK("turbojets")                                  \
    E_KEYWORD_BLOCK("turboprops")                                 \
    E_KEYWORD_BLOCK("turboprops2")                                \
    E_KEYWORD_BLOCK("videocamera")                                \
    E_KEYWORD_BLOCK("wheels")                                     \
    E_KEYWORD_BLOCK("wheels2")                                    \
    E_KEYWORD_BLOCK("wings")

DEFINE_REGEX(            IDENTIFY_KEYWORD_RESPECT_CASE, IDENTIFY_KEYWORD_REGEX_STRING )
DEFINE_REGEX_IGNORECASE( IDENTIFY_KEYWORD_IGNORE_CASE,  IDENTIFY_KEYWORD_REGEX_STRING )

DEFINE_REGEX( POSITIVE_DECIMAL_NUMBER, E_POSITIVE_DECIMAL_NUMBER );

DEFINE_REGEX( NEGATIVE_DECIMAL_NUMBER, E_NEGATIVE_DECIMAL_NUMBER );

DEFINE_REGEX( DECIMAL_NUMBER, E_DECIMAL_NUMBER );

DEFINE_REGEX( MINUS_ONE_REAL, E_MINUS_ONE_REAL );

DEFINE_REGEX( REAL_NUMBER, E_REAL_NUMBER );

DEFINE_REGEX( NODE_LIST,
    E_LEADING_WHITESPACE
    E_CAPTURE( E_NODE_ID )
    E_CAPTURE_OPTIONAL( E_DELIMITER ) // Tolerate invalid trailing delimiter
    E_TRAILING_WHITESPACE
    );

DEFINE_REGEX( NODE_ID_OPTIONAL,
    E_LEADING_WHITESPACE
    E_CAPTURE_OPTIONAL( E_POSITIVE_DECIMAL_NUMBER ) // Numeric Id
    E_CAPTURE_OPTIONAL( E_MINUS_ONE_REAL ) // -1 = use default
    E_CAPTURE_OPTIONAL( E_STRING_ALNUM_HYPHENS_USCORES_ONLY ) // String Id
    E_CAPTURE_OPTIONAL( E_TRAILING_WHITESPACE )
    );

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
// Regexes for parsing directives                                             //
// -------------------------------------------------------------------------- //

DEFINE_REGEX( DIRECTIVE_SECTION,
    E_LEADING_WHITESPACE
    "[Ss][Ee][Cc][Tt][Ii][Oo][Nn]"
    E_DELIMITER_SPACE
    E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) // #1 Unused (version)
    E_DELIMITER_SPACE
    E_CAPTURE( E_STRING_NO_SPACES )        // #2 Module name
    E_2xCAPTURE_TRAILING_COMMENT
    );

DEFINE_REGEX( DIRECTIVE_SET_BEAM_DEFAULTS_SCALE,
    E_LEADING_WHITESPACE
    "[Ss][Ee][Tt]_[Bb][Ee][Aa][Mm]_[Dd][Ee][Ff][Aa][Uu][Ll][Tt][Ss]_[Ss][Cc][Aa][Ll][Ee]" 
    E_DELIMITER_SPACE
    E_CAPTURE( E_REAL_NUMBER )             // #1 Spring scale
    E_CAPTURE_OPTIONAL(
        E_DELIMITER_COMMA
        E_CAPTURE( E_REAL_NUMBER )         // #3 Damping scale

        E_CAPTURE_OPTIONAL(
            E_DELIMITER_COMMA
            E_CAPTURE( E_REAL_NUMBER )     // #5 Deformation threshold constant scale

            E_CAPTURE_OPTIONAL(
                E_DELIMITER_COMMA
                E_CAPTURE( E_REAL_NUMBER ) // #7 Breaking threshold constant scale
            )
        )
    )
    E_CAPTURE_OPTIONAL( E_ILLEGAL_TRAILING_STRING )
    E_2xCAPTURE_TRAILING_COMMENT
    );

DEFINE_REGEX( DIRECTIVE_SET_INERTIA_DEFAULTS,
    E_LEADING_WHITESPACE
    "[Ss][Ee][Tt]_[Ii][Nn][Ee][Rr][Tt][Ii][Aa]_[Dd][Ee][Ff][Aa][Uu][Ll][Tt][Ss]"
    E_DELIMITER_SPACE
    E_CAPTURE( E_REAL_NUMBER )                                        // #1 Start delay OR -1
    E_CAPTURE_OPTIONAL(
        E_CAPTURE( E_DELIMITER )                                      // #3 Delimiter
        E_CAPTURE( E_REAL_NUMBER )                                    // #4 Stop delay

        E_CAPTURE_OPTIONAL(
            E_CAPTURE( E_DELIMITER )                                  // #6 Delimiter
            E_CAPTURE_OPTIONAL( E_INERTIA_FUNCTION )                  // #7 Start function

            E_CAPTURE_OPTIONAL(
                E_CAPTURE( E_DELIMITER )                              // #9 Delimiter
                E_CAPTURE_OPTIONAL( E_INERTIA_FUNCTION )              // #10 Stop function
            )
        )
    )
    E_CAPTURE_OPTIONAL( E_ILLEGAL_TRAILING_STRING )                   // #11 Invalid text
    E_TRAILING_WHITESPACE
    );

DEFINE_REGEX( DIRECTIVE_SET_MANAGEDMATERIALS_OPTIONS,
    E_LEADING_WHITESPACE
    "[Ss][Ee][Tt]_[Mm][Aa][Nn][Aa][Gg][Ee][Dd][Mm][Aa][Tt][Ee][Rr][Ii][Aa][Ll][Ss]_[Oo][Pp][Tt][Ii][Oo][Nn][Ss]" 
    E_DELIMITER_SPACE
    E_CAPTURE( E_STRING_ANYTHING_BUT_WHITESPACE ) // Double-sided (for backwards compatibility, accept anything)
    E_2xCAPTURE_TRAILING_COMMENT
    );

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
            E_CAPTURE( "[ols]*" ) // #6 Differential modes
        "\\)"
        E_OPTIONAL_SPACE
    )
    E_2xCAPTURE_TRAILING_COMMENT
    );

DEFINE_REGEX( SECTION_BEAMS,
    E_LEADING_WHITESPACE
    E_CAPTURE( E_NODE_ID )                     // #1 Node 1
    E_CAPTURE( E_DELIMITER )
    E_CAPTURE( E_NODE_ID )                     // #3 Node 2

    E_CAPTURE_OPTIONAL(
        E_CAPTURE( E_DELIMITER )               // #5 Delimiter (possibly dangling)

        E_CAPTURE_OPTIONAL(
            E_CAPTURE( E_STRING_ALNUM_HYPHENS_USCORES_ONLY ) // #7 Options

            E_CAPTURE_OPTIONAL(
                E_CAPTURE( E_DELIMITER )       // #9 Delimiter (possibly dangling)

                E_CAPTURE_OPTIONAL(
                    E_CAPTURE( E_REAL_NUMBER ) // #11 User-defined extension break limit
                )
            )
        )
    )
    E_2xCAPTURE_TRAILING_COMMENT
    );

DEFINE_REGEX( SECTION_CAMERARAILS,
    E_LEADING_WHITESPACE
    E_CAPTURE( E_NODE_ID ) // Node
    E_2xCAPTURE_TRAILING_COMMENT
    );

DEFINE_REGEX( SECTION_CINECAM,
    E_LEADING_WHITESPACE
    E_CAPTURE( E_REAL_NUMBER ) // #1 X
    E_CAPTURE( E_DELIMITER )
    E_CAPTURE( E_REAL_NUMBER ) // #3 Y
    E_CAPTURE( E_DELIMITER )
    E_CAPTURE( E_REAL_NUMBER ) // #5 Z
    E_CAPTURE( E_DELIMITER )
    E_CAPTURE( E_NODE_ID )     // #7 Node 1 ...
    E_CAPTURE( E_DELIMITER )
    E_CAPTURE( E_NODE_ID )
    E_CAPTURE( E_DELIMITER )
    E_CAPTURE( E_NODE_ID )
    E_CAPTURE( E_DELIMITER )
    E_CAPTURE( E_NODE_ID )     // #13 Node 4
    E_CAPTURE( E_DELIMITER )
    E_CAPTURE( E_NODE_ID )
    E_CAPTURE( E_DELIMITER )
    E_CAPTURE( E_NODE_ID )
    E_CAPTURE( E_DELIMITER )
    E_CAPTURE( E_NODE_ID )
    E_CAPTURE( E_DELIMITER )
    E_CAPTURE( E_NODE_ID )     // #21 Node 8
    E_CAPTURE_OPTIONAL(
        E_CAPTURE( E_DELIMITER )
        E_CAPTURE( E_REAL_NUMBER )     // #24 Spring

        E_CAPTURE_OPTIONAL(
            E_CAPTURE( E_DELIMITER )
            E_CAPTURE( E_REAL_NUMBER ) // #27 Damping
        )
    )
    E_CAPTURE_OPTIONAL( E_STRING_ANYTHING_BUT_WHITESPACE ) // Illegal characters
    E_TRAILING_WHITESPACE
    );

DEFINE_REGEX( SECTION_COLLISIONBOXES,
    E_LEADING_WHITESPACE
    E_DELIMITED_LIST( E_NODE_ID, "," )
    E_TRAILING_WHITESPACE
    );

DEFINE_REGEX( SECTION_EXHAUSTS,
    E_LEADING_WHITESPACE
    E_CAPTURE( E_NODE_ID )   // Reference node
    E_DELIMITER_COMMA
    E_CAPTURE( E_NODE_ID )   // Direction node
    E_CAPTURE_OPTIONAL(
        E_DELIMITER
        E_CAPTURE( E_REAL_NUMBER ) // #4 Factor

        E_CAPTURE_OPTIONAL( 
            E_DELIMITER
            E_CAPTURE( E_STRING_NO_SPACES ) // #6 Material name
        )
    )
    E_2xCAPTURE_TRAILING_COMMENT
DEFINE_REGEX( INLINE_SECTION_EXTCAMERA,
    E_LEADING_WHITESPACE
    "[Ee][Xx][Tt][Cc][Aa][Mm][Ee][Rr][Aa]"
    E_CAPTURE( // #1 Wrap
        E_CAPTURE( // #2
            E_DELIMITER_SPACE 
            "classic"
        )
        E_OR
        E_CAPTURE( // #3
            E_DELIMITER_SPACE 
            "cinecam"
        )
        E_OR
        E_CAPTURE( // #4
            E_DELIMITER_SPACE 
            "node"

            E_CAPTURE( // #5
                E_DELIMITER_SPACE
                E_CAPTURE( E_NODE_ID ) // #6
            )
        )
    )
    E_TRAILING_WHITESPACE
    );

DEFINE_REGEX( INLINE_SECTION_FILE_FORMAT_VERSION,
    E_LEADING_WHITESPACE
    "[Ff][Ii][Ll][Ee][Ff][Oo][Rr][Mm][Aa][Tt][Vv][Ee][Rr][Ss][Ii][Oo][Nn]"
    E_DELIMITER_SPACE
    E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) // #1 The version
    E_TRAILING_WHITESPACE
    );

DEFINE_REGEX( SECTION_FUSEDRAG,
    E_LEADING_WHITESPACE
    E_CAPTURE( E_NODE_ID )                       // #1 Node 1
    E_CAPTURE( E_DELIMITER )
    E_CAPTURE( E_NODE_ID )                       // #3 Node 2
    E_CAPTURE( E_DELIMITER )
    E_CAPTURE_OPTIONAL(
        E_CAPTURE( E_REAL_NUMBER )               // #6 Approx. width
        E_CAPTURE( E_DELIMITER )
        E_CAPTURE( E_STRING_NO_SPACES )          // #8 Airfoil name
        E_TRAILING_WHITESPACE
    )
    E_CAPTURE_OPTIONAL(                          // #9
        "autocalc"
        E_CAPTURE_OPTIONAL(
            E_CAPTURE( E_DELIMITER )
            E_CAPTURE( E_REAL_NUMBER )           // #12 Area coeff.

            E_CAPTURE_OPTIONAL(
                E_CAPTURE( E_DELIMITER )
                E_CAPTURE( E_STRING_NO_SPACES )  // #15 Airfoil name
            )
        )
        E_TRAILING_WHITESPACE
    )
    );

DEFINE_REGEX( SECTION_GLOBALS,
    E_LEADING_WHITESPACE
    E_CAPTURE( E_REAL_NUMBER ) // Dry mass
    E_DELIMITER_COMMA
    E_CAPTURE( E_REAL_NUMBER ) // Cargo mass
    E_CAPTURE_OPTIONAL( 
        E_DELIMITER_COMMA
        E_CAPTURE( E_STRING_ANYTHING_BUT_DELIMITER ) // Truck submesh material
    )
    E_CAPTURE_OPTIONAL( E_DELIMITER )                      // #5 Illegal characters at the end
    E_CAPTURE_OPTIONAL( E_STRING_ANYTHING_BUT_WHITESPACE ) // #6 Illegal characters at the end
    E_TRAILING_WHITESPACE
    );

DEFINE_REGEX( SECTION_GUID,
    E_LEADING_WHITESPACE
    "[Gg][Uu][Ii][Dd]"
    E_DELIMITER_SPACE
    E_CAPTURE( E_STRING_NO_SPACES )
    E_TRAILING_WHITESPACE
    );

DEFINE_REGEX( SECTION_GUISETTINGS,
    E_LEADING_WHITESPACE
    E_CAPTURE( // #1
        E_CAPTURE( // #2
            "tachoMaterial"
            E_DELIMITER_SPACE 
            E_CAPTURE( E_STRING_NO_SPACES ) // #3 The name of the tachometer face material.
            E_TRAILING_WHITESPACE
        ) 
        E_OR
        E_CAPTURE( 
            "speedoMaterial" 
            E_DELIMITER_SPACE 
            E_CAPTURE( E_STRING_NO_SPACES ) // #5 The name of the speedometer face material.
            E_TRAILING_WHITESPACE
        )
        E_OR
        E_CAPTURE( 
            "speedoMax" 
            E_DELIMITER_SPACE 
            E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) // #7 The highest number that is on the speedometer.
            E_TRAILING_WHITESPACE
        )
        E_OR
        E_CAPTURE( 
            "useMaxRPM" 
            E_DELIMITER_SPACE 
            E_CAPTURE( E_DECIMAL_NUMBER ) // #9 Boolean (0/1): Use max RPM from ENGINE section as tacho-max
            E_TRAILING_WHITESPACE
        )
        E_OR
        E_CAPTURE( 
            "helpMaterial" 
            E_DELIMITER_SPACE 
            E_CAPTURE( E_STRING_NO_SPACES ) // #11 A picture that shows command instructions 
            E_TRAILING_WHITESPACE
        )
        E_OR
        E_CAPTURE( // #12 
            "interactiveOverviewMap" // Enum: off, simple, zoom
            E_DELIMITER_SPACE 
            E_CAPTURE( // #13
                E_CAPTURE( "off" ) // #14
                E_OR
                E_CAPTURE( "simple" ) // #15
                E_OR
                E_CAPTURE( "zoom" ) // #16
            )
            E_TRAILING_WHITESPACE
        )
        E_OR
        E_CAPTURE( 
            "dashboard"
            E_DELIMITER_SPACE 
            E_CAPTURE( E_STRING_NO_SPACES ) // #18 MyGUI layout file for dashboard. You can use multiple lines.
            E_TRAILING_WHITESPACE
        )
        E_OR
        E_CAPTURE( 
            "texturedashboard"
            E_DELIMITER_SPACE 
            E_CAPTURE( E_STRING_NO_SPACES ) // #20 MyGUI layout file for the RTT dashboard of this truck. You can use multiple lines.
            E_TRAILING_WHITESPACE
        )
        E_OR
        E_CAPTURE( // Obsolete, ignored
            "debugBeams"
            E_CAPTURE_OPTIONAL( E_DELIMITER_SPACE E_BOOLEAN ) 
            E_TRAILING_WHITESPACE
        )
    )
    );

DEFINE_REGEX( SECTION_HELP,
    E_CAPTURE( E_STRING_NO_SPACES ) // Material name
    E_TRAILING_WHITESPACE
    );



DEFINE_REGEX( SECTION_LOCKGROUPS,
    E_LEADING_WHITESPACE
    E_CAPTURE( E_DECIMAL_NUMBER ) // #1 Group number
    E_DELIMITER_COMMA
    E_CAPTURE( // #2 Node list
        E_CAPTURE(
            E_NODE_ID
            E_DELIMITER_COMMA
        ) "*"
        E_CAPTURE(
            E_NODE_ID
        )
    )
    E_CAPTURE_OPTIONAL( E_ILLEGAL_TRAILING_STRING ) // #5 Invalid text
    E_TRAILING_WHITESPACE
    );

DEFINE_REGEX( SECTION_MANAGEDMATERIALS,
    E_LEADING_WHITESPACE
    E_CAPTURE( E_STRING_ANYTHING_BUT_DELIMITER ) // #1 Material name
    E_CAPTURE( E_DELIMITER )        // #2
    E_CAPTURE(                      // #3 Type wrapper
        E_CAPTURE( "mesh_standard" )        // #4
        E_OR
        E_CAPTURE( "mesh_transparent" )     // #5
        E_OR
        E_CAPTURE( "flexmesh_standard" )    // #6
        E_OR
        E_CAPTURE( "flexmesh_transparent" ) // #7
    )
    E_CAPTURE( E_DELIMITER )
    E_CAPTURE( E_STRING_NO_SPACES )         // #9 Diffuse map filename
    E_CAPTURE_OPTIONAL(
        E_CAPTURE( E_DELIMITER )
        E_CAPTURE( E_STRING_NO_SPACES )     // #12
        E_CAPTURE_OPTIONAL( 
            E_CAPTURE( E_DELIMITER )
            E_CAPTURE( E_STRING_NO_SPACES ) // #15
        )
    )
    E_CAPTURE_OPTIONAL( E_ILLEGAL_TRAILING_STRING )
    E_TRAILING_WHITESPACE
    );

DEFINE_REGEX( SECTION_MATERIALFLAREBINDINGS,
    E_LEADING_WHITESPACE
    E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) // #1 Flare number
    E_CAPTURE_OPTIONAL( "[[:alpha:]]+" )   // #2 Tolerated characters after flare number (backwards compatibility)
    E_CAPTURE(                             // #3 Separator
        E_DELIMITER_COMMA
        E_OR
        E_DELIMITER_SPACE // Backwards compatibility
    )
    E_CAPTURE( E_STRING_ANYTHING_BUT_DELIMITER )        // #4 Material name
    E_2xCAPTURE_TRAILING_COMMENT
    );

DEFINE_REGEX( SECTION_MINIMASS,
    E_LEADING_WHITESPACE
    E_CAPTURE( E_REAL_NUMBER ) // Min. default mass
    E_2xCAPTURE_TRAILING_COMMENT
    );

// FIXME: Undocumented by RoR, may not be correct
DEFINE_REGEX( SECTION_NODECOLLISION,
    E_LEADING_WHITESPACE
    E_CAPTURE( E_NODE_ID ) // Node
    E_DELIMITER_COMMA
    E_CAPTURE( E_REAL_NUMBER ) // Radius
    E_2xCAPTURE_TRAILING_COMMENT
    );

DEFINE_REGEX( SECTION_PARTICLES,
    E_LEADING_WHITESPACE
    E_CAPTURE( E_NODE_ID ) // Emit node
    E_DELIMITER_COMMA
    E_CAPTURE( E_NODE_ID ) // Ref node
    E_DELIMITER_COMMA
    E_CAPTURE( E_STRING_NO_SPACES ) // Particle sys. name
    E_2xCAPTURE_TRAILING_COMMENT
    );

DEFINE_REGEX( SECTION_PISTONPROPS,
    E_LEADING_WHITESPACE
    E_CAPTURE( E_NODE_ID ) // Reference node (center of the prop)
    E_DELIMITER_COMMA
    E_CAPTURE( E_NODE_ID ) // Prop axis node (back of the prop)
    E_DELIMITER_COMMA
    E_CAPTURE( E_NODE_ID ) // Blade 1 tip node
    E_DELIMITER_COMMA
    E_CAPTURE( E_NODE_ID ) // Blade 2 tip node
    E_DELIMITER_COMMA
    E_CAPTURE( E_NODE_ID ) // #5 Blade 3 tip node
    E_DELIMITER_COMMA
    E_CAPTURE( E_NODE_ID ) // #6 Blade 4 tip node
    E_DELIMITER_COMMA
    E_CAPTURE ( // #7 Wrapper
        E_CAPTURE( E_MINUS_ONE_REAL ) // #8 Couple node unused
        E_OR
        E_CAPTURE( E_NODE_ID ) // #9 Couple node    
    )
    E_DELIMITER_COMMA
    E_CAPTURE( E_REAL_NUMBER ) // #10 Power (in kW)
    E_DELIMITER_COMMA
    E_CAPTURE( E_REAL_NUMBER ) // #11 Pitch
    E_DELIMITER_COMMA
    E_CAPTURE( E_STRING_NO_SPACES ) // #12 Airfoil
    E_2xCAPTURE_TRAILING_COMMENT
    );

DEFINE_REGEX( SECTION_RAILGROUPS,
    E_LEADING_WHITESPACE
    E_CAPTURE( E_NODE_ID ) // Id
    E_DELIMITER_COMMA
    E_CAPTURE( "([[:blank:]]*" E_CAPTURE( E_NODE_ID ) "[[:blank:]]*,)*([[:blank:]]*" E_CAPTURE( E_NODE_ID ) "[[:blank:]]*)+$" ) // Node list
    );

DEFINE_REGEX( SECTION_ROPABLES,
    E_LEADING_WHITESPACE
    E_CAPTURE( E_NODE_ID ) // Node

    E_CAPTURE_OPTIONAL( 
        E_DELIMITER_COMMA
        E_CAPTURE( E_DECIMAL_NUMBER ) // Group
    
        E_CAPTURE_OPTIONAL( 
            E_DELIMITER_COMMA
            E_CAPTURE( "[01]" )
        )
    )
    E_TRAILING_WHITESPACE
    );

DEFINE_REGEX( SECTION_ROPES,
    E_LEADING_WHITESPACE
    E_CAPTURE( E_NODE_ID ) // Root node
    E_DELIMITER_COMMA
    E_CAPTURE( E_NODE_ID ) // End node
    E_CAPTURE_OPTIONAL( 
        E_DELIMITER_COMMA
        E_CAPTURE( E_STRING_NO_SPACES ) // Flags
    )
    E_TRAILING_WHITESPACE
    );

#define E_ROTATORS_R1_R2_COMMON_BASE                                    \
    E_LEADING_WHITESPACE                                                \
    E_CAPTURE( E_NODE_ID )                 /* #1  Axis node 1 */        \
    E_CAPTURE( E_DELIMITER )                                            \
    E_CAPTURE( E_NODE_ID )                 /* #3  Axis node 2 */        \
    E_CAPTURE( E_DELIMITER )                                            \
    E_CAPTURE( E_NODE_ID )                 /* #5  Baseplate node 1 */   \
    E_CAPTURE( E_DELIMITER )                                            \
    E_CAPTURE( E_NODE_ID )                 /* #7  Baseplate node 2 */   \
    E_CAPTURE( E_DELIMITER )                                            \
    E_CAPTURE( E_NODE_ID )                 /* #9  Baseplate node 3 */   \
    E_CAPTURE( E_DELIMITER )                                            \
    E_CAPTURE( E_NODE_ID )                 /* #11 Baseplate node 4 */   \
    E_CAPTURE( E_DELIMITER )                                            \
    E_CAPTURE( E_NODE_ID )                 /* #13 Rot. plate node 1 */  \
    E_CAPTURE( E_DELIMITER )                                            \
    E_CAPTURE( E_NODE_ID )                 /* #15 Rot. plate node 2 */  \
    E_CAPTURE( E_DELIMITER )                                            \
    E_CAPTURE( E_NODE_ID )                 /* #17 Rot. plate node 3 */  \
    E_CAPTURE( E_DELIMITER )                                            \
    E_CAPTURE( E_NODE_ID )                 /* #19 Rot. plate node 4 */  \
    E_CAPTURE( E_DELIMITER )                                            \
    E_CAPTURE( E_REAL_NUMBER )             /* #21 Rate */               \
    E_CAPTURE( E_DELIMITER )                                            \
    E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* #23 Key left */           \
    E_CAPTURE( E_DELIMITER )                                            \
    E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* #25 Key right */

#define E_ROTATORS_R1_R2_COMMON_INERTIA                                      \
    E_CAPTURE( E_STRING_NO_SPACES )              /* #+1 Start delay (real number; accept garbage for backwards compatibility) */     \
    E_CAPTURE_OPTIONAL(                                                      \
        E_CAPTURE( E_DELIMITER )                                             \
        E_CAPTURE( E_REAL_NUMBER )               /* #+4 Stop delay */        \
        E_CAPTURE_OPTIONAL(                                                  \
            E_CAPTURE( E_DELIMITER )                                         \
            E_CAPTURE( E_REAL_NUMBER )           /* #+7 Start fn. */         \
            E_CAPTURE_OPTIONAL(                                              \
                E_CAPTURE( E_DELIMITER )                                     \
                E_CAPTURE( E_INERTIA_FUNCTION )  /* #+10 Stop fn. */         \
                E_CAPTURE_OPTIONAL(                                          \
                    E_CAPTURE( E_DELIMITER )                                 \
                    E_CAPTURE( E_REAL_NUMBER )   /* #+13 Engine coupling */  \
                    E_CAPTURE_OPTIONAL(                                      \
                        E_CAPTURE( E_DELIMITER )                             \
                        E_CAPTURE( E_BOOLEAN )   /* #+16 Needs engine */     \
                    )                                                        \
                )                                                            \
            )                                                                \
        )                                                                    \
    )

DEFINE_REGEX( SECTION_ROTATORS,
    E_ROTATORS_R1_R2_COMMON_BASE
    E_CAPTURE_OPTIONAL(                   // #26
        E_CAPTURE( E_DELIMITER )
        E_ROTATORS_R1_R2_COMMON_INERTIA   // Base #28
    )
    E_TRAILING_WHITESPACE
    );

DEFINE_REGEX( SECTION_ROTATORS2,
    E_ROTATORS_R1_R2_COMMON_BASE
    E_CAPTURE_OPTIONAL(                              // #26
        E_CAPTURE( E_DELIMITER )
        E_CAPTURE( E_REAL_NUMBER )                   // #28 Rotating force

        E_CAPTURE_OPTIONAL( 
            E_CAPTURE( E_DELIMITER )
            E_CAPTURE( E_REAL_NUMBER )               // #31 Anti-jitter tolerance

            E_CAPTURE_OPTIONAL( 
                E_CAPTURE( E_DELIMITER )
                E_CAPTURE( E_STRING_NO_SPACES )      // #34 Description

                E_CAPTURE_OPTIONAL(
                    E_CAPTURE( E_DELIMITER )
                    E_ROTATORS_R1_R2_COMMON_INERTIA  // Base #37
                )
            )
        )
    )
    E_2xCAPTURE_TRAILING_COMMENT
    );

DEFINE_REGEX( SECTION_SCREWPROPS,
    E_LEADING_WHITESPACE
    E_CAPTURE( E_NODE_ID ) // Prop node
    E_DELIMITER_COMMA
    E_CAPTURE( E_NODE_ID ) // Back node
    E_DELIMITER_COMMA
    E_CAPTURE( E_NODE_ID ) // Top node
    E_DELIMITER_COMMA
    E_CAPTURE( E_REAL_NUMBER ) // Power
    E_TRAILING_WHITESPACE
    );

DEFINE_REGEX( INLINE_SECTION_SET_SKELETON_DISPLAY,
    E_LEADING_WHITESPACE
    "[Ss][Ee][Tt]_[Ss][Kk][Ee][Ll][Ee][Tt][Oo][Nn]_[Ss][Ee][Tt][Tt][Ii][Nn][Gg][Ss]"
    E_DELIMITER_SPACE
    E_CAPTURE( E_REAL_NUMBER ) // View distance
    E_CAPTURE_OPTIONAL( 
        E_DELIMITER_COMMA
        E_CAPTURE( E_REAL_NUMBER ) // Thickness
    )
    E_TRAILING_WHITESPACE
    );

DEFINE_REGEX( SECTION_SLIDENODES,
    E_LEADING_WHITESPACE
    E_CAPTURE( E_NODE_ID ) // #1 The sliding node
    E_DELIMITER_COMMA
    E_CAPTURE( ".*$" ) // #2 All the rest: [rail nodes list] parameters
    );

DEFINE_REGEX( SLIDENODES_IDENTIFY_OPTION,
    E_LEADING_WHITESPACE
    E_CAPTURE_OPTIONAL( // #1 Spring rate entry
        "[sS]"
        E_CAPTURE( E_REAL_NUMBER ) // #2 Spring rate value
    )
    E_CAPTURE_OPTIONAL( // #3 Break force entry
        "[bB]"
        E_CAPTURE( E_REAL_NUMBER ) // #4 Break force value
    )
    E_CAPTURE_OPTIONAL( // #5 Tolerance entry
        "[tT]"
        E_CAPTURE( E_REAL_NUMBER ) // #6 Tolerance value
    )
    E_CAPTURE_OPTIONAL( // #7 Attachment rate entry
        "[rR]"
        E_CAPTURE( E_REAL_NUMBER ) // #8 Attachment rate value
    )
    E_CAPTURE_OPTIONAL( // #9 Railgroup ID entry
        "[gG]"
        E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) // #10 Railgroup ID value
    )
    E_CAPTURE_OPTIONAL( // #11 Max. attachment distance entry
        "[dD]"
        E_CAPTURE( E_REAL_NUMBER ) // #12 Max. attachment distance value
    )
    E_CAPTURE_OPTIONAL( // #13 Quantity entry
        "[qQ]"
        E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) // #14 Quantity value
    )
    E_CAPTURE_OPTIONAL( // #15 Constraints entry
        "[cC]"
        E_CAPTURE( E_STRING_NO_SPACES ) // #16 Constraints value
    )
    E_TRAILING_WHITESPACE
    );

DEFINE_REGEX( INLINE_SECTION_SLOPE_BRAKE,
    E_LEADING_WHITESPACE
    "[Ss][Ll][Oo][Pp][Ee][Bb][Rr][Aa][Kk][Ee]"
    E_CAPTURE_OPTIONAL(
        E_DELIMITER_SPACE
        E_CAPTURE( E_REAL_NUMBER ) // #2 Regulating force

        E_CAPTURE_OPTIONAL(
            E_DELIMITER_COMMA
            E_CAPTURE( E_REAL_NUMBER ) // #4 Attach-angle

            E_CAPTURE_OPTIONAL(
                E_DELIMITER_COMMA
                E_CAPTURE( E_REAL_NUMBER ) // #6 Detach-angle
            )
        )
    )
    E_TRAILING_WHITESPACE
    );

DEFINE_REGEX( SECTION_SOUNDSOURCES,
    E_LEADING_WHITESPACE
    E_CAPTURE( E_NODE_ID ) // Node which makes the sound
    E_DELIMITER_COMMA
    E_CAPTURE( E_STRING_NO_SPACES ) // Sound script name
    E_2xCAPTURE_TRAILING_COMMENT
    );

DEFINE_REGEX( SECTION_SOUNDSOURCES2,
    E_LEADING_WHITESPACE

    E_CAPTURE( E_NODE_ID ) // #1 Node which makes the sound
    E_CAPTURE( E_DELIMITER )
    E_CAPTURE( E_STRING_ANYTHING_BUT_DELIMITER ) // #3 Mode/Cinecam ID, decimal number. Accept anything for backward compatibility
    E_CAPTURE( E_DELIMITER )
    E_CAPTURE( E_STRING_NO_SPACES ) // #5 Sound script name

    E_CAPTURE_OPTIONAL( E_ILLEGAL_TRAILING_STRING ) // #6 Invalid text
    E_2xCAPTURE_TRAILING_COMMENT
    );

DEFINE_REGEX( SECTION_TIES,
    E_LEADING_WHITESPACE
    E_CAPTURE( E_NODE_ID )       // #1 Root node
    E_CAPTURE( E_DELIMITER )
    E_CAPTURE( E_REAL_NUMBER )   // #3 Max. reach length
    E_CAPTURE( E_DELIMITER )
    E_CAPTURE( E_REAL_NUMBER )   // #5 Auto shorten rate
    E_CAPTURE( E_DELIMITER )
    E_CAPTURE( E_REAL_NUMBER )   // #7 Min length
    E_CAPTURE( E_DELIMITER )
    E_CAPTURE( E_REAL_NUMBER )   // #9 Max length
    E_CAPTURE_OPTIONAL(
        E_CAPTURE( E_DELIMITER )
        E_CAPTURE( E_STRING_ANYTHING_BUT_DELIMITER )   // #12 Options

        E_CAPTURE_OPTIONAL(
            E_CAPTURE( E_DELIMITER )
            E_CAPTURE( E_REAL_NUMBER )                 // #15 Max stress

            E_CAPTURE_OPTIONAL(
                E_CAPTURE( E_DELIMITER )
                E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) // #18 Group
            )
        )
    )
    E_CAPTURE_OPTIONAL( E_DELIMITER ) // Tolerate trailing delimiter
    E_2xCAPTURE_TRAILING_COMMENT
    );

DEFINE_REGEX( SECTION_TORQUECURVE,
    E_LEADING_WHITESPACE
    E_CAPTURE( // #1 Whole custom-curve line
        E_CAPTURE( E_REAL_NUMBER ) // #2 Power
        E_DELIMITER_COMMA
        E_CAPTURE( E_REAL_NUMBER ) // #3 Percentage
    )
    E_OR
    E_CAPTURE( E_STRING_NO_SPACES ) // #4 Known function
    E_2xCAPTURE_TRAILING_COMMENT
    );

DEFINE_REGEX( SECTION_TRIGGERS,
    E_LEADING_WHITESPACE
    E_CAPTURE( E_NODE_ID ) // Node 1
    E_DELIMITER_COMMA
    E_CAPTURE( E_NODE_ID ) // Node 2
    E_DELIMITER_COMMA
    E_CAPTURE( E_REAL_NUMBER ) // Contraction trigger limit
    E_DELIMITER_COMMA
    E_CAPTURE( E_REAL_NUMBER ) // Extension trigger limit
    E_DELIMITER_COMMA
    E_CAPTURE( E_DECIMAL_NUMBER ) // #5 Contraction trigger action
    E_DELIMITER_COMMA
    E_CAPTURE( E_DECIMAL_NUMBER ) // #6 Extension trigger action

    E_CAPTURE_OPTIONAL(
        E_DELIMITER_COMMA
        E_CAPTURE( E_STRING_ALNUM_HYPHENS_USCORES_ONLY ) // #8 Options

        E_CAPTURE_OPTIONAL(
            E_DELIMITER_COMMA
            E_CAPTURE( E_REAL_NUMBER ) // #10 Boundary timer
        )
    )
    E_2xCAPTURE_TRAILING_COMMENT
    );

DEFINE_REGEX( SECTION_TURBOJETS,
    E_LEADING_WHITESPACE
    E_CAPTURE( E_NODE_ID ) // Front
    E_DELIMITER_COMMA
    E_CAPTURE( E_NODE_ID ) // Back
    E_DELIMITER_COMMA
    E_CAPTURE( E_NODE_ID ) // Side
    E_DELIMITER_COMMA
    E_CAPTURE( E_DECIMAL_NUMBER ) // Is reversable
    E_DELIMITER_COMMA
    E_CAPTURE( E_REAL_NUMBER ) // Dry thrust
    E_DELIMITER_COMMA
    E_CAPTURE( E_REAL_NUMBER ) // Wet thrust
    E_DELIMITER_COMMA
    E_CAPTURE( E_REAL_NUMBER ) // Front diameter
    E_DELIMITER_COMMA
    E_CAPTURE( E_REAL_NUMBER ) // Rear diameter
    E_DELIMITER_COMMA
    E_CAPTURE( E_REAL_NUMBER ) // Nozzle length
    E_2xCAPTURE_TRAILING_COMMENT
    );

DEFINE_REGEX( SECTION_TURBOPROPS,
    E_LEADING_WHITESPACE
    E_CAPTURE( E_NODE_ID ) // Reference node (center of the prop)
    E_DELIMITER_COMMA
    E_CAPTURE( E_NODE_ID ) // Prop axis node (back of the prop)
    E_DELIMITER_COMMA
    E_CAPTURE( E_NODE_ID ) // Blade 1 tip node
    E_DELIMITER_COMMA
    E_CAPTURE( E_NODE_ID ) // Blade 2 tip node
    E_DELIMITER_COMMA
    E_CAPTURE( E_NODE_ID ) // Blade 3 tip node
    E_DELIMITER_COMMA
    E_CAPTURE( E_NODE_ID ) // Blade 4 tip node
    E_DELIMITER_COMMA
    E_CAPTURE( E_REAL_NUMBER ) // Power of the turbine (in kW)
    E_DELIMITER_COMMA
    E_CAPTURE( E_STRING_NO_SPACES ) // Airfoil of the blades
    E_2xCAPTURE_TRAILING_COMMENT
    );

DEFINE_REGEX( SECTION_VIDEOCAMERA,
    E_LEADING_WHITESPACE
    E_CAPTURE( E_NODE_ID ) // Ref. node
    E_DELIMITER_COMMA
    E_CAPTURE( E_NODE_ID ) // X node
    E_DELIMITER_COMMA
    E_CAPTURE( E_NODE_ID ) // Y node
    E_DELIMITER_COMMA
    E_CAPTURE( E_NODE_ID ) // Alt. ref. node
    E_DELIMITER_COMMA
    E_CAPTURE( E_NODE_ID ) // alt. orientation node
    E_DELIMITER_COMMA
    E_CAPTURE( E_REAL_NUMBER ) // Offset X
    E_DELIMITER_COMMA
    E_CAPTURE( E_REAL_NUMBER ) // Offset Y
    E_DELIMITER_COMMA
    E_CAPTURE( E_REAL_NUMBER ) // Offset Z
    E_DELIMITER_COMMA
    E_CAPTURE( E_REAL_NUMBER ) // Rot. X
    E_DELIMITER_COMMA
    E_CAPTURE( E_REAL_NUMBER ) // #10 Rot. Y
    E_DELIMITER_COMMA
    E_CAPTURE( E_REAL_NUMBER ) // Rot. Z
    E_DELIMITER_COMMA
    E_CAPTURE( E_REAL_NUMBER ) // FOV
    E_DELIMITER_COMMA
    E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) // Tex. width
    E_DELIMITER_COMMA
    E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) // Tex. height
    E_DELIMITER_COMMA
    E_CAPTURE( E_REAL_NUMBER ) // Min. clip dist.
    E_DELIMITER_COMMA
    E_CAPTURE( E_REAL_NUMBER ) // Max. clip dist.
    E_DELIMITER_COMMA
    E_CAPTURE( E_DECIMAL_NUMBER ) // Camera role
    E_DELIMITER_COMMA
    E_CAPTURE( E_DECIMAL_NUMBER ) // Camera mode
    E_DELIMITER_COMMA
    E_CAPTURE( E_STRING_NO_SPACES ) // Material name
    E_CAPTURE_OPTIONAL(  // #20
        E_DELIMITER_COMMA 
        E_CAPTURE( E_STRING_NO_SPACES ) // #21 Camera name
    )
    E_2xCAPTURE_TRAILING_COMMENT
    );

// -------------------------------------------------------------------------- //
// Cleanup                                                                    //
// -------------------------------------------------------------------------- //

#undef E_BACKSLASH
#undef E_SLASH
#undef E_REAL_NUMBER
#undef E_DELIMITER_COMMA
#undef E_DELIMITER_COLON
#undef E_TRAILING_WHITESPACE
#undef E_LEADING_WHITESPACE
#undef E_STRING_NO_SPACES
#undef E_STRING_ALNUM_HYPHENS_USCORES_ONLY
#undef E_DECIMAL_NUMBER
#undef E_POSITIVE_DECIMAL_NUMBER
#undef E_NEGATIVE_DECIMAL_NUMBER
#undef E_DELIMITER_SPACE
#undef E_OPTIONAL_SPACE
#undef E_MINUS_ONE_REAL
#undef E_BOOLEAN
#undef E_OR
#undef E_NODE_ID
#undef E_NODE_ID_OPTIONAL
#undef E_INERTIA_FUNCTION

#undef E_CAPTURE
#undef E_CAPTURE_OPTIONAL
#undef E_KEYWORD_BLOCK
#undef E_KEYWORD_INLINE
#undef E_DELIMITED_LIST
#undef DEFINE_REGEX
#undef DEFINE_REGEX_IGNORECASE

#undef E_2xCAPTURE_TRAILING_COMMENT

} // namespace Regexes

} // namespace RigDef
