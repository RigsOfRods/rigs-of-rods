/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

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
	@file   RigDefRegexes.h
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

#include <boost/regex.hpp>

namespace RigDef
{

namespace Regexes
{

/* -------------------------------------------------------------------------- */
/* Repeatedly used bits of regular expressions                                */
/* Using #define-s instead of const vars to utilize C-string concatenation    */
/* REMEMBER! Do not use captures () - they would mess up result order. Use |  */
/* REMEMBER! Expressions using | MUST be enclosed in E_CAPTURE()              */
/* -------------------------------------------------------------------------- */

/* CHARACTERS */

#define E_BACKSLASH "\\\\" /* Each backslash \ must be escaped in C++ (\\) and then in regex (\\\\) */

#define E_SLASH "\\/"

/* NUMBERS */

#define E_REAL_NUMBER "-?[[:digit:]]*\\.[[:digit:]]*|-?[[:digit:]]+" /* Uses |, MUST be enclosed in E_CAPTURE() */

#define E_DECIMAL_NUMBER "-?[[:digit:]]+"

#define E_POSITIVE_DECIMAL_NUMBER "[[:digit:]]+"

#define E_NEGATIVE_DECIMAL_NUMBER "-[[:digit:]]+"

#define E_MINUS_ONE_REAL "-1\\.[0]*|-1"   /* Uses |, MUST be enclosed in E_CAPTURE() */

/* STRINGS */

#define E_ILLEGAL_TRAILING_STRING "[^[:blank:]]+.*"

#define E_STRING_NO_SPACES "[[:alnum:]" E_BACKSLASH E_SLASH "@.{}()+,;_\\-]+"

#define E_STRING_NO_LEADING_DIGIT "[^[:blank:][:digit:]]+[^[:blank:]]*"

#define E_STRING_ANYTHING_BUT_WHITESPACE "[^[:blank:]]+"

#define E_STRING_ALNUM_COMMAS_USCORES_ONLY "[[:alnum:]_-]+"

#define E_OPTIONAL_SPACE "[[:blank:]]*"

/* DELIMITERS */

#define E_OR "|"

#define E_TRAILING_WHITESPACE "[[:blank:]]*$"

#define E_LEADING_WHITESPACE "^[[:blank:]]*"

#define E_DELIMITER_SPACE "[[:blank:]]+"

#define E_DELIMITER_COMMA "[[:blank:]]*,[[:blank:]]*"

#define E_DELIMITER_COLON "[[:blank:]]*:[[:blank:]]*"

#define E_DELIMITER E_DELIMITER_COMMA E_OR E_DELIMITER_SPACE /* Uses |, MUST be enclosed in E_CAPTURE() */

/* VALUE TYPES */

#define E_BOOLEAN "true|yes|1|false|no|0" /* Uses |, MUST be enclosed in E_CAPTURE() */

#define E_NODE_ID E_STRING_ALNUM_COMMAS_USCORES_ONLY

#define E_NODE_ID_OPTIONAL E_NODE_ID E_OR E_MINUS_ONE_REAL

#define E_INERTIA_FUNCTION E_STRING_ALNUM_COMMAS_USCORES_ONLY

/* -------------------------------------------------------------------------- */
/* Macros                                                                     */
/* -------------------------------------------------------------------------- */

/** Encloses expression in 'capture' marks. */
#define E_CAPTURE(_REGEXP_) \
	"(" _REGEXP_ ")"

/** Encloses expression in 'capture' marks with ? (zero or one time) mark. */
#define E_CAPTURE_OPTIONAL(_REGEXP_) \
	"(" _REGEXP_ ")?"

/** A keyword which should be on it's own line. Used in IDENTIFY_KEYWORD. */
#define E_KEYWORD_BLOCK(_NAME_) \
	"(^" _NAME_ "[[:blank:]]*$)?"

/** A keyword which should have values following it. Used in IDENTIFY_KEYWORD. */
#define E_KEYWORD_INLINE(_NAME_) \
	"(^" _NAME_ E_DELIMITER_SPACE ".*$)?"
	
/** Inline keyword, tolerant version: keyword and values can be delimited by either space or comma */
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

/** Actual regex definition macro. */
#define DEFINE_REGEX(_NAME_,_REGEXP_) \
	const boost::regex _NAME_ = boost::regex( _REGEXP_, boost::regex::extended);

#define DEFINE_REGEX_IGNORECASE(_NAME_,_REGEXP_) \
	const boost::regex _NAME_ = boost::regex( _REGEXP_, boost::regex::extended | boost::regex::icase);

/* -------------------------------------------------------------------------- */
/* Utility regexes                                                            */
/* -------------------------------------------------------------------------- */

/* IMPORTANT! If you add a value here, you must also modify File::Keywords enum, it relies on positions in this regex */
DEFINE_REGEX( IDENTIFY_KEYWORD,
	// E_KEYWORD_BLOCK("advdrag") /* Not supported yet */

	E_KEYWORD_INLINE_TOLERANT("add_animation")  /* Position 1 */
	E_KEYWORD_BLOCK("airbrakes")       /* Position 2 */
	E_KEYWORD_BLOCK("animators")       /* Position 3 etc... */
	E_KEYWORD_INLINE("AntiLockBrakes")
	E_KEYWORD_BLOCK("axles")
	E_KEYWORD_INLINE("author")
	E_KEYWORD_BLOCK("backmesh")
	E_KEYWORD_BLOCK("beams")
	E_KEYWORD_BLOCK("brakes")
	E_KEYWORD_BLOCK("cab")
	E_KEYWORD_BLOCK("camerarail")
	E_KEYWORD_BLOCK("cameras")
	E_KEYWORD_BLOCK("cinecam")
	E_KEYWORD_BLOCK("collisionboxes")
	E_KEYWORD_BLOCK("commands")
	E_KEYWORD_BLOCK("commands2")
	E_KEYWORD_BLOCK("contacters")
	E_KEYWORD_INLINE("cruisecontrol")
	E_KEYWORD_BLOCK("description")
	E_KEYWORD_INLINE("detacher_group")
	E_KEYWORD_BLOCK("disabledefaultsounds")
	E_KEYWORD_BLOCK("enable_advanced_deformation")
	E_KEYWORD_BLOCK("end")
	E_KEYWORD_BLOCK("end_section")
	E_KEYWORD_BLOCK("engine")
	E_KEYWORD_BLOCK("engoption")
	E_KEYWORD_BLOCK("envmap")
	E_KEYWORD_BLOCK("exhausts")
	E_KEYWORD_INLINE("extcamera")
	E_KEYWORD_INLINE("fileformatversion")
	E_KEYWORD_INLINE("fileinfo")
	E_KEYWORD_BLOCK("fixes")
	E_KEYWORD_BLOCK("flares")
	E_KEYWORD_BLOCK("flares2")
	E_KEYWORD_BLOCK("flexbodies")
	E_KEYWORD_INLINE("flexbody_camera_mode")
	E_KEYWORD_BLOCK("flexbodywheels")
	E_KEYWORD_BLOCK("forwardcommands")
	E_KEYWORD_BLOCK("fusedrag")
	E_KEYWORD_BLOCK("globals")
	E_KEYWORD_INLINE("guid")
	E_KEYWORD_BLOCK("guisettings")
	E_KEYWORD_BLOCK("help")
	E_KEYWORD_BLOCK("hideInChooser")
	E_KEYWORD_BLOCK("hookgroup")
	E_KEYWORD_BLOCK("hooks")
	E_KEYWORD_BLOCK("hydros")
	E_KEYWORD_BLOCK("importcommands")
	E_KEYWORD_BLOCK("lockgroups")
	E_KEYWORD_BLOCK("lockgroup_default_nolock")
	E_KEYWORD_BLOCK("managedmaterials")
	E_KEYWORD_BLOCK("materialflarebindings")
	E_KEYWORD_BLOCK("meshwheels")
	E_KEYWORD_BLOCK("meshwheels2")
	E_KEYWORD_BLOCK("minimass")
	E_KEYWORD_BLOCK("nodecollision")
	E_KEYWORD_BLOCK("nodes")
	E_KEYWORD_BLOCK("nodes2")
	E_KEYWORD_BLOCK("particles")
	E_KEYWORD_BLOCK("pistonprops")
	E_KEYWORD_INLINE("prop_camera_mode")
	E_KEYWORD_BLOCK("props")
	E_KEYWORD_BLOCK("railgroups")
	E_KEYWORD_BLOCK("rescuer")
	E_KEYWORD_BLOCK("rigidifiers")
	E_KEYWORD_BLOCK("rollon")
	E_KEYWORD_BLOCK("ropables")
	E_KEYWORD_BLOCK("ropes")
	E_KEYWORD_BLOCK("rotators")
	E_KEYWORD_BLOCK("rotators_2")
	E_KEYWORD_BLOCK("screwprops")
	E_KEYWORD_INLINE("section")
	E_KEYWORD_INLINE("sectionconfig")
	E_KEYWORD_INLINE("set_beam_defaults")
	E_KEYWORD_INLINE("set_beam_defaults_scale")
	E_KEYWORD_INLINE("set_collision_range")
	E_KEYWORD_INLINE("set_inertia_defaults")
	E_KEYWORD_INLINE("set_managedmaterials_options")
	E_KEYWORD_INLINE("set_node_defaults")
	E_KEYWORD_BLOCK("set_shadows")
	E_KEYWORD_INLINE("set_skeleton_settings")
	E_KEYWORD_BLOCK("shocks")
	E_KEYWORD_BLOCK("shocks2")
	E_KEYWORD_BLOCK("slidenode_connect_instantly")
	E_KEYWORD_BLOCK("slidenodes")
	E_KEYWORD_INLINE("SlopeBrake")
	E_KEYWORD_BLOCK("soundsources")
	E_KEYWORD_BLOCK("soundsources2")
	E_KEYWORD_INLINE("speedlimiter")
	//E_KEYWORD_BLOCK("soundsources3") // Not supported yet
	E_KEYWORD_BLOCK("submesh")
	E_KEYWORD_INLINE("submesh_groundmodel")
	E_KEYWORD_BLOCK("texcoords")
	E_KEYWORD_BLOCK("ties")
	E_KEYWORD_BLOCK("torquecurve")
	E_KEYWORD_INLINE("TractionControl")
	E_KEYWORD_BLOCK("triggers")
	E_KEYWORD_BLOCK("turbojets")
	E_KEYWORD_BLOCK("turboprops")
	E_KEYWORD_BLOCK("turboprops2")
	E_KEYWORD_BLOCK("videocamera")
	E_KEYWORD_BLOCK("wheels")
	E_KEYWORD_BLOCK("wheels2")
	E_KEYWORD_BLOCK("wings")
	);

DEFINE_REGEX( CHECK_BLOCK_COMMENT_END, 
	"^end_comment"
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( CHECK_END_DESCRIPTION, 
	"^end_description"
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( IDENTIFY_LINE_TYPE,
	E_CAPTURE_OPTIONAL( "^comment" E_TRAILING_WHITESPACE ) /* Block comment start */
	E_CAPTURE_OPTIONAL( "^[[:blank:]]*;.*$" )              /* Single line comment */
	E_CAPTURE_OPTIONAL( "^[[:blank:]]*" E_SLASH ".*$" )    /* Single line comment (legacy format) */
	E_CAPTURE_OPTIONAL( "^[[:blank:]]*$"  )                /* Blank line */
	);

DEFINE_REGEX( POSITIVE_DECIMAL_NUMBER, E_POSITIVE_DECIMAL_NUMBER );

DEFINE_REGEX( NEGATIVE_DECIMAL_NUMBER, E_NEGATIVE_DECIMAL_NUMBER );

DEFINE_REGEX( DECIMAL_NUMBER, E_DECIMAL_NUMBER );

DEFINE_REGEX( MINUS_ONE_REAL, E_MINUS_ONE_REAL );

DEFINE_REGEX( NODE_NUMBER, 
	E_LEADING_WHITESPACE
	E_DECIMAL_NUMBER
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( NODE_NAME, E_STRING_ALNUM_COMMAS_USCORES_ONLY );

DEFINE_REGEX( NODE_LIST,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID )
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( NODE_ID_OPTIONAL,
	E_LEADING_WHITESPACE
	E_CAPTURE_OPTIONAL( E_POSITIVE_DECIMAL_NUMBER ) /* Numeric Id */
	E_CAPTURE_OPTIONAL( E_MINUS_ONE_REAL ) /* -1 = use default */
	E_CAPTURE_OPTIONAL( E_STRING_ALNUM_COMMAS_USCORES_ONLY ) /* String Id */
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

/* -------------------------------------------------------------------------- */
/* Regexes for parsing directives                                             */
/* -------------------------------------------------------------------------- */

DEFINE_REGEX( DIRECTIVE_ADD_ANIMATION,
	"^add_animation"
	E_CAPTURE( 
		E_CAPTURE( E_DELIMITER_SPACE )
		E_OR 
		E_CAPTURE( E_DELIMITER_COMMA )
	)
	E_CAPTURE( E_REAL_NUMBER ) /* #4 Ratio */
	E_CAPTURE( 
		E_CAPTURE( E_DELIMITER_SPACE )
		E_OR 
		E_CAPTURE( E_DELIMITER_COMMA )
	)
	E_CAPTURE( E_REAL_NUMBER ) /* #8 Lower limit */
	E_CAPTURE( 
		E_CAPTURE( E_DELIMITER_SPACE )
		E_OR 
		E_CAPTURE( E_DELIMITER_COMMA )
	)
	E_CAPTURE( E_REAL_NUMBER ) /* #12 Upper limit */
	E_CAPTURE( 
		E_CAPTURE( E_DELIMITER_SPACE )
		E_OR 
		E_CAPTURE( E_DELIMITER_COMMA )
	)
	E_CAPTURE( ".*$" )         /* #16 All the rest, parsed later */
	);

DEFINE_REGEX( IDENTIFY_ADD_ANIMATION_TOKEN,
	E_LEADING_WHITESPACE
	E_CAPTURE( /* #1 Wrapper */
		E_CAPTURE( "autoanimate" E_OPTIONAL_SPACE ) /* #2 Mode option */
		E_OR
		E_CAPTURE( "noflip" E_OPTIONAL_SPACE )      /* #3 Mode option */
		E_OR
		E_CAPTURE( "bounce" E_OPTIONAL_SPACE )      /* #4 Mode option */
		E_OR
		E_CAPTURE( "eventlock" E_OPTIONAL_SPACE )   /* #5 Mode option */
		E_OR
		E_CAPTURE( /* #6 Mode */
			"mode"   
			E_CAPTURE_OPTIONAL( E_OPTIONAL_SPACE ":" ) /* #7 Check format validity, colon is required */
			E_CAPTURE( ".*" )                          /* #8 Options or invalid text */
		)
		E_OR
		E_CAPTURE( /* #9 Source */
			"source"   
			E_CAPTURE_OPTIONAL( E_OPTIONAL_SPACE ":" ) /* #10 Check format validity, colon is required */
			E_CAPTURE( ".*" )                          /* #11 Options or invalid text */
		)
		E_OR
		E_CAPTURE( /* #12 Event */
			"event"   
			E_CAPTURE_OPTIONAL( E_OPTIONAL_SPACE ":" ) /* #13 Check format validity, colon is required */
			E_CAPTURE( ".*" )                          /* #14 Options or invalid text */
		)
	)
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( IDENTIFY_ADD_ANIMATION_MODE,
	E_LEADING_WHITESPACE
	E_CAPTURE( /* Wrapper */
		E_CAPTURE( "x-rotation" )
		E_OR
		E_CAPTURE( "y-rotation" )
		E_OR
		E_CAPTURE( "z-rotation" )
		E_OR
		E_CAPTURE( "x-offset" )
		E_OR
		E_CAPTURE( "y-offset" )
		E_OR
		E_CAPTURE( "z-offset" )
		E_OR
		E_OPTIONAL_SPACE /* Ignore blank string */
	)
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( IDENTIFY_ADD_ANIMATION_SOURCE,
	E_LEADING_WHITESPACE
	E_CAPTURE( /* #1 Wrapper */
		E_CAPTURE( "airspeed" ) /* #2 */
		E_OR
		E_CAPTURE( "vvi" )
		E_OR
		E_CAPTURE( "altimeter100k" )
		E_OR
		E_CAPTURE( "altimeter10k" )
		E_OR
		E_CAPTURE( "altimeter1k" )
		E_OR
		E_CAPTURE( "aoa" )
		E_OR
		E_CAPTURE( "flap" )
		E_OR
		E_CAPTURE( "airbrake" )
		E_OR
		E_CAPTURE( "roll" ) /* #10 */
		E_OR
		E_CAPTURE( "pitch" ) 
		E_OR
		E_CAPTURE( "brakes" )
		E_OR
		E_CAPTURE( "accel" )
		E_OR
		E_CAPTURE( "clutch" )
		E_OR
		E_CAPTURE( "speedo" )
		E_OR
		E_CAPTURE( "tacho" )
		E_OR
		E_CAPTURE( "turbo" )
		E_OR
		E_CAPTURE( "parking" )
		E_OR
		E_CAPTURE( "shifterman1" )
		E_OR
		E_CAPTURE( "shifterman2" ) /* #20 */
		E_OR
		E_CAPTURE( "sequential" ) 
		E_OR
		E_CAPTURE( "shifterlin" )
		E_OR
		E_CAPTURE( "torque" )
		E_OR
		E_CAPTURE( "heading" )
		E_OR
		E_CAPTURE( "difflock" )
		E_OR
		E_CAPTURE( "rudderboat" )
		E_OR
		E_CAPTURE( "throttleboat" )
		E_OR
		E_CAPTURE( "steeringwheel" )
		E_OR
		E_CAPTURE( "aileron" )
		E_OR
		E_CAPTURE( "elevator" ) /* #30 */
		E_OR
		E_CAPTURE( "rudderair" ) 
		E_OR
		E_CAPTURE( "permanent" )
		/* [Source: event] intentionally left out */
		E_OR
		E_CAPTURE(
			"throttle"
			E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* #34 Motor number */
		)
		E_OR
		E_CAPTURE(
			"rpm"
			E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* #36 Motor number */
		)
		E_OR
		E_CAPTURE(
			"aerotorq"
			E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* #38 Motor number */
		)
		E_OR
		E_CAPTURE(
			"aeropit"
			E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* #40 Motor number */
		)
		E_OR
		E_CAPTURE(
			"aerostatus"
			E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* #42 Motor number */
		)
		E_OR
		E_OPTIONAL_SPACE /* Ignore blank input */
	)
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( DIRECTIVE_DETACHER_GROUP,
	"^detacher_group"
	E_DELIMITER_SPACE
	E_CAPTURE( /* #1 Wrapper */
		E_CAPTURE( E_DECIMAL_NUMBER ) /* #2 Detacher group ID */
		E_OR
		E_CAPTURE( "end" )            /* #3 End of group */
	)
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( DIRECTIVE_FLEXBODY_CAMERA_MODE,
	"^flexbody_camera_mode"
	E_DELIMITER_SPACE
	E_CAPTURE( E_DECIMAL_NUMBER ) /* #1 Mode/cinecam index */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( DIRECTIVE_PROP_CAMERA_MODE,
	"^prop_camera_mode"
	E_DELIMITER_SPACE
	E_CAPTURE( E_DECIMAL_NUMBER ) /* #1 Mode/cinecam index */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( DIRECTIVE_SECTION,
	"^section"
	E_DELIMITER_SPACE
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* #1 Unused (version) */
	E_DELIMITER_SPACE
	E_CAPTURE( E_STRING_NO_SPACES )        /* #2 Module name */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( DIRECTIVE_SET_BEAM_DEFAULTS, 
	"^set_beam_defaults" 
	E_DELIMITER_SPACE
	E_CAPTURE( E_REAL_NUMBER ) /* #1 Spring */
	E_CAPTURE_OPTIONAL(
		E_DELIMITER_COMMA
		E_CAPTURE( E_REAL_NUMBER ) /* #3 Damping */

		E_CAPTURE_OPTIONAL(
			E_DELIMITER_COMMA
			E_CAPTURE( E_REAL_NUMBER ) /* #5 Deformation threshold constant */

			E_CAPTURE_OPTIONAL(
				E_DELIMITER_COMMA
				E_CAPTURE( E_REAL_NUMBER ) /* #7 Breaking threshold constant */

				E_CAPTURE_OPTIONAL(
					E_DELIMITER_COMMA
					E_CAPTURE( E_REAL_NUMBER ) /* #9 Beam diameter */

					E_CAPTURE_OPTIONAL(
						E_DELIMITER_COMMA
						E_CAPTURE( E_STRING_NO_SPACES ) /* #11 Beam material */

						E_CAPTURE_OPTIONAL(
							E_DELIMITER_COMMA
							E_CAPTURE( E_REAL_NUMBER ) /* #13 Plastic deformation coefficient */
						)
					)
				)
			)
		)
	)
	);

DEFINE_REGEX( DIRECTIVE_SET_BEAM_DEFAULTS_SCALE, 
	"^set_beam_defaults_scale" 
	E_DELIMITER_SPACE
	E_CAPTURE( E_REAL_NUMBER ) /* #1 Spring scale */
	E_CAPTURE_OPTIONAL(
		E_DELIMITER_COMMA
		E_CAPTURE( E_REAL_NUMBER ) /* #3 Damping scale */

		E_CAPTURE_OPTIONAL(
			E_DELIMITER_COMMA
			E_CAPTURE( E_REAL_NUMBER ) /* #5 Deformation threshold constant scale */

			E_CAPTURE_OPTIONAL(
				E_DELIMITER_COMMA
				E_CAPTURE( E_REAL_NUMBER ) /* #7 Breaking threshold constant scale */
			)
		)
	)
	);

DEFINE_REGEX( DIRECTIVE_SET_INERTIA_DEFAULTS, 
	"^set_inertia_defaults"
	E_DELIMITER_SPACE
	E_CAPTURE( E_REAL_NUMBER ) /* #1 Start delay OR -1 */
	E_CAPTURE_OPTIONAL( 
		E_CAPTURE( E_DELIMITER_COMMA E_OR E_DELIMITER_SPACE ) /* #3 Delimiter */
		E_CAPTURE( E_REAL_NUMBER ) /* #4 Stop delay */

		E_CAPTURE_OPTIONAL( 
			E_CAPTURE( E_DELIMITER_COMMA E_OR E_DELIMITER_SPACE ) /* #6 Delimiter */
			E_CAPTURE_OPTIONAL( E_INERTIA_FUNCTION ) /* #7 Start function */

			E_CAPTURE_OPTIONAL( 
				E_CAPTURE( E_DELIMITER_COMMA E_OR E_DELIMITER_SPACE ) /* #9 Delimiter */
				E_CAPTURE_OPTIONAL( E_INERTIA_FUNCTION ) /* #10 Stop function */
			)
		)
	)
	E_CAPTURE_OPTIONAL( E_ILLEGAL_TRAILING_STRING ) /* #11 Invalid text */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( DIRECTIVE_SET_MANAGEDMATERIALS_OPTIONS, 
	"^set_managedmaterials_options" 
	E_DELIMITER_SPACE
	E_CAPTURE( E_STRING_ANYTHING_BUT_WHITESPACE ) /* Double-sided (for backwards compatibility, accept anything) */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( DIRECTIVE_SET_NODE_DEFAULTS, 
	"^set_node_defaults" 
	E_DELIMITER_SPACE
	E_CAPTURE( E_REAL_NUMBER ) /* Load weight */
	E_CAPTURE_OPTIONAL(
		E_DELIMITER_COMMA
		E_CAPTURE( E_REAL_NUMBER ) /* #3 Friction */

		E_CAPTURE_OPTIONAL(
			E_DELIMITER_COMMA
			E_CAPTURE( E_REAL_NUMBER ) /* #5 Volume */

			E_CAPTURE_OPTIONAL(
				E_DELIMITER_COMMA
				E_CAPTURE( E_REAL_NUMBER ) /* #7 Surface */

				E_CAPTURE_OPTIONAL(
					E_DELIMITER_COMMA
					E_CAPTURE( "[lnmfxychebpL]*" ) /* #9 Options */
				)
			)
		)
	)
	E_TRAILING_WHITESPACE
	);

/* -------------------------------------------------------------------------- */
/* Regexes for parsing sections                                               */
/* -------------------------------------------------------------------------- */

DEFINE_REGEX( SECTION_AIRBRAKES,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Ref. node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* X axis node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Y axis node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Additional node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* X offset */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Y offset */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Z offset */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Panel width */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Panel height */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Maximum inclination angle */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Texcoord x1 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Texcoord y1 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Texcoord x2 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Texcoord y2 */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_ANIMATORS, 
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Node 1 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Node 2 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Lenghtening factor */
	E_DELIMITER_COMMA
	E_CAPTURE(".*$") /* Options string */
	);

DEFINE_REGEX( PARSE_ANIMATORS_NUMBERED_KEYWORD,
	E_LEADING_WHITESPACE
	E_CAPTURE( "throttle|rpm|aerotorq|aeropit|aerostatus" )
	E_CAPTURE( "[[:digit:]]" )
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( PARSE_ANIMATORS_KEY_COLON_VALUE,
	E_LEADING_WHITESPACE
	E_CAPTURE( "shortlimit|longlimit" )
	E_DELIMITER_COLON
	E_CAPTURE( E_REAL_NUMBER )
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_ANTI_LOCK_BRAKES, 
	E_LEADING_WHITESPACE
	E_CAPTURE( E_REAL_NUMBER ) /* Regulating force */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Min. speed */

	E_CAPTURE_OPTIONAL(
		E_DELIMITER_COMMA
		E_CAPTURE( E_REAL_NUMBER ) /* #4 Pulse/sec */

		E_CAPTURE_OPTIONAL(
			E_DELIMITER_COMMA
			"mode:"
			E_CAPTURE("[[:blank:]ONFDASHTGLEonfdashtgle&]*") /* #6 Mode string */	
		)
	)
	);

/* Case insensitive */
DEFINE_REGEX_IGNORECASE( ANTI_LOCK_BRAKES_MODE,
	E_LEADING_WHITESPACE
	E_CAPTURE( /* Wrapper */
		E_CAPTURE( "on" )
		E_OR
		E_CAPTURE( "off" )
		E_OR
		E_CAPTURE( "nodash" )
		E_OR
		E_CAPTURE( "notoggle" )
	)
	E_TRAILING_WHITESPACE
	); 

DEFINE_REGEX( INLINE_SECTION_AUTHOR, 
	"^author"

	E_CAPTURE_OPTIONAL(
		E_DELIMITER_SPACE
		E_CAPTURE( E_STRING_ANYTHING_BUT_WHITESPACE ) /* #2 Role */

		E_CAPTURE_OPTIONAL( 
			E_DELIMITER_SPACE
			E_CAPTURE( E_DECIMAL_NUMBER )   /* #4 Forum ID */

			E_CAPTURE_OPTIONAL( 
				E_DELIMITER_SPACE
				E_CAPTURE( E_STRING_ANYTHING_BUT_WHITESPACE ) /* #6 Name */
				
				E_CAPTURE_OPTIONAL( 
					E_DELIMITER_SPACE
					E_CAPTURE( E_STRING_ANYTHING_BUT_WHITESPACE ) /* #8 Email */
				)
			)
		)
	)
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_AXLES_PROPERTY,
	E_CAPTURE_OPTIONAL( /* #1 */
		E_LEADING_WHITESPACE
		"w(1|2)" /* #2 wheel number */
		"\\("
			E_CAPTURE( E_NODE_ID ) /* #3 Node 1 */
			E_DELIMITER_SPACE
			E_CAPTURE( E_NODE_ID ) /* #4 Node 2 */
		"\\)"
		E_TRAILING_WHITESPACE
	)
	E_CAPTURE_OPTIONAL( /* #5 */
		E_LEADING_WHITESPACE
		"d\\("
			E_CAPTURE( "[ols]*" ) /* #6 Differential modes */
		"\\)"
		E_TRAILING_WHITESPACE
	)
	);

DEFINE_REGEX( SECTION_BEAMS,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Node 1 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Node 2 */

	E_CAPTURE_OPTIONAL( 
		E_DELIMITER_COMMA
		E_CAPTURE( "[irs]*" ) /* Options */

		E_CAPTURE_OPTIONAL( 
			E_DELIMITER_SPACE 
			E_CAPTURE( E_REAL_NUMBER )
		)
	)
	);

DEFINE_REGEX( SECTION_BRAKES,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_REAL_NUMBER ) /* #1 Default brake force */
	E_CAPTURE_OPTIONAL( /* #2 */
		E_DELIMITER_COMMA 
		E_CAPTURE( E_REAL_NUMBER ) /* #3 Parking brake force */
	)
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_CAMERARAILS,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Node */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_CAMERAS,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Center node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Back node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Left node */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_CINECAM,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_REAL_NUMBER ) /* X */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Y */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Z */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Node 1 ... */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID )
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID )
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID )
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID )
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID )
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* #10 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Node 8 */
	E_CAPTURE_OPTIONAL( 
		E_DELIMITER_COMMA
		E_CAPTURE( E_REAL_NUMBER )    /* Spring */
	
		E_CAPTURE_OPTIONAL( 
			E_DELIMITER_COMMA
			E_CAPTURE( E_REAL_NUMBER )    /* #15 Damping */
		)
	)
	E_CAPTURE_OPTIONAL( E_STRING_ANYTHING_BUT_WHITESPACE ) /* #16 Illegal characters */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_COLLISIONBOXES,
	E_LEADING_WHITESPACE
	E_DELIMITED_LIST( E_NODE_ID, "," )
	E_TRAILING_WHITESPACE
	);

/* Syntax: startDelay, stopDelay, startFunction  stopFunction affectEngine needsEngine */
/* valid separator is comma or space, because the original documentation was confusing and nobody got it right */
#define E_SECTIONS_COMMANDS_COMMANDS2_INERTIA_AFFECT_ENGINE_PART                  \
	E_CAPTURE_OPTIONAL( /* #1 */                                                  \
		E_CAPTURE( E_REAL_NUMBER ) /* #2 Start delay */                           \
		E_CAPTURE_OPTIONAL( /* #3 */                                              \
			E_CAPTURE( E_DELIMITER_SPACE E_OR E_DELIMITER_COMMA )                    \
			E_CAPTURE( E_REAL_NUMBER ) /* #5 Stop delay */                        \
			E_CAPTURE_OPTIONAL( /* #6 */                                          \
				E_CAPTURE( E_DELIMITER_SPACE E_OR E_DELIMITER_COMMA )                \
				E_CAPTURE( E_INERTIA_FUNCTION ) /* #8 Start function */           \
				E_CAPTURE_OPTIONAL( /* #9 */                                      \
					E_CAPTURE( E_DELIMITER_SPACE E_OR E_DELIMITER_COMMA )            \
					E_CAPTURE( E_INERTIA_FUNCTION ) /* #11 Stop function */       \
					E_CAPTURE_OPTIONAL( /* #12 */                                 \
						E_CAPTURE( E_DELIMITER_SPACE E_OR E_DELIMITER_COMMA )        \
						E_CAPTURE( E_DECIMAL_NUMBER ) /* #14 Affect engine */     \
						E_CAPTURE_OPTIONAL( /* #15 */                             \
							E_CAPTURE( E_DELIMITER_SPACE E_OR E_DELIMITER_COMMA )    \
							E_CAPTURE( E_DECIMAL_NUMBER ) /* #17 Needs engine */  \
						)                                                         \
					)                                                             \
				)                                                                 \
			)                                                                     \
		)                                                                         \
	)

DEFINE_REGEX( SECTION_COMMANDS,
	E_CAPTURE( E_NODE_ID ) /* #1 Node 1 */
	E_CAPTURE( E_DELIMITER_SPACE E_OR E_DELIMITER_COMMA )
	E_CAPTURE( E_NODE_ID ) /* #3 Node 2 */
	E_CAPTURE( E_DELIMITER_SPACE E_OR E_DELIMITER_COMMA )
	E_CAPTURE( E_REAL_NUMBER ) /* #5 Rate */
	E_CAPTURE( E_DELIMITER_SPACE E_OR E_DELIMITER_COMMA )
	E_CAPTURE( E_REAL_NUMBER ) /* #7 Min length */
	E_CAPTURE( E_DELIMITER_SPACE E_OR E_DELIMITER_COMMA )
	E_CAPTURE( E_REAL_NUMBER ) /* #9 Max length */
	E_CAPTURE( E_DELIMITER_SPACE E_OR E_DELIMITER_COMMA )
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* #11 Contract key */
	E_CAPTURE( E_DELIMITER_SPACE E_OR E_DELIMITER_COMMA )
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* #13 Expand key */
	E_CAPTURE_OPTIONAL( /* #14 */
		E_CAPTURE( E_DELIMITER_SPACE E_OR E_DELIMITER_COMMA )
		E_CAPTURE( "i|r|n" ) /* #16 Options */
		E_CAPTURE_OPTIONAL( /* #17 */
			E_CAPTURE( E_DELIMITER_SPACE E_OR E_DELIMITER_COMMA ) 
			E_CAPTURE( E_STRING_NO_SPACES ) /* #19 Description */ 
			E_CAPTURE( E_DELIMITER_SPACE E_OR E_DELIMITER_COMMA ) /* #20 */
			E_SECTIONS_COMMANDS_COMMANDS2_INERTIA_AFFECT_ENGINE_PART
		)
	)
	);

DEFINE_REGEX( SECTION_COMMANDS_2,
/*
	Syntax (valid separator is comma or space, because the original documentation was confusing and nobody got it right)
	id1, id2, rateShort, rateLong, short, long, keyS, keyL, options description startDelay, stopDelay, startFunction  stopFunction affectEngine needsEngine
*/
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID )  /* #1 Node 1 */
	E_CAPTURE( E_DELIMITER_SPACE E_OR E_DELIMITER_COMMA )
	E_CAPTURE( E_NODE_ID )  /* #3 Node 2 */
	E_CAPTURE( E_DELIMITER_SPACE E_OR E_DELIMITER_COMMA )
	E_CAPTURE( E_REAL_NUMBER ) /* #5 Contract rate */
	E_CAPTURE( E_DELIMITER_SPACE E_OR E_DELIMITER_COMMA )
	E_CAPTURE( E_REAL_NUMBER ) /* #7 Expand rate */
	E_CAPTURE( E_DELIMITER_SPACE E_OR E_DELIMITER_COMMA )
	E_CAPTURE( E_REAL_NUMBER ) /* #9 Min length */
	E_CAPTURE( E_DELIMITER_SPACE E_OR E_DELIMITER_COMMA )
	E_CAPTURE( E_REAL_NUMBER ) /* #11 Max length */
	E_CAPTURE( E_DELIMITER_SPACE E_OR E_DELIMITER_COMMA )
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER )  /* #13 Contract key */
	E_CAPTURE( E_DELIMITER_SPACE E_OR E_DELIMITER_COMMA )
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER )  /* #15 Expand key */
	E_CAPTURE_OPTIONAL(                     /* #16 */
		E_CAPTURE( E_DELIMITER_SPACE E_OR E_DELIMITER_COMMA )
		E_CAPTURE( "[nircfpo]*" )           /* #18 Options */
		E_CAPTURE_OPTIONAL(                 /* #19 */
			E_CAPTURE( E_DELIMITER_SPACE E_OR E_DELIMITER_COMMA )
			E_CAPTURE( E_STRING_NO_SPACES ) /* #21 Description */
			/*	
				Illegal description with spaces. 
				However, for backwards compatibility, we must silently pass	up to 2 extra strings 
				as long as they don't start with a digit and there's nothing after them.
			*/
			E_CAPTURE_OPTIONAL( E_DELIMITER_SPACE E_STRING_NO_LEADING_DIGIT ) /* #22 Error */
			E_CAPTURE_OPTIONAL( E_DELIMITER_SPACE E_STRING_NO_LEADING_DIGIT ) /* #23 Error */
			E_CAPTURE_OPTIONAL( /* #24 */
				E_CAPTURE( E_DELIMITER_SPACE E_OR E_DELIMITER_COMMA ) /* #25 */
				E_SECTIONS_COMMANDS_COMMANDS2_INERTIA_AFFECT_ENGINE_PART
			)
		)
	)
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( INLINE_SECTION_CRUISECONTROL,
	"^cruisecontrol"
	E_DELIMITER_SPACE
	E_CAPTURE( E_REAL_NUMBER ) /* Min speed */
	E_DELIMITER_COMMA
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* Autobrake on/off */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_ENGINE,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_REAL_NUMBER ) /* #1 Min RPM */
	E_CAPTURE( E_DELIMITER )
	E_CAPTURE( E_REAL_NUMBER ) /* #3 Max RPM */
	E_CAPTURE( E_DELIMITER )
	E_CAPTURE( E_REAL_NUMBER ) /* #5 Torque */
	E_CAPTURE( E_DELIMITER )
	E_CAPTURE( E_REAL_NUMBER ) /* #7 Differential */
	E_CAPTURE( E_DELIMITER )
	E_CAPTURE( E_REAL_NUMBER ) /* #9 Reverse */
	E_CAPTURE( E_DELIMITER )
	E_CAPTURE( E_REAL_NUMBER ) /* #11 Neutral */
	E_CAPTURE( E_DELIMITER )
	E_CAPTURE( E_REAL_NUMBER ) /* #13 1st gear */
	E_CAPTURE_OPTIONAL( 
		E_CAPTURE( E_DELIMITER )
		E_CAPTURE( E_REAL_NUMBER ) /* #16 2nd gear ... */

		E_CAPTURE_OPTIONAL( 
			E_CAPTURE( E_DELIMITER )
			E_CAPTURE( E_REAL_NUMBER ) /* #19 */

			E_CAPTURE_OPTIONAL( 
				E_CAPTURE( E_DELIMITER )
				E_CAPTURE( E_REAL_NUMBER ) /* #22*/

				E_CAPTURE_OPTIONAL( 
					E_CAPTURE( E_DELIMITER )
					E_CAPTURE( E_REAL_NUMBER ) /* #25 */

					E_CAPTURE_OPTIONAL( 
						E_CAPTURE( E_DELIMITER )
						E_CAPTURE( E_REAL_NUMBER ) /* #28 */

						E_CAPTURE_OPTIONAL( 
							E_CAPTURE( E_DELIMITER )
							E_CAPTURE( E_REAL_NUMBER ) /* #31 */

							E_CAPTURE_OPTIONAL( 
								E_CAPTURE( E_DELIMITER )
								E_CAPTURE( E_REAL_NUMBER ) /* #34 */

								E_CAPTURE_OPTIONAL( 
									E_CAPTURE( E_DELIMITER )
									E_CAPTURE( E_REAL_NUMBER ) /* #37 */

									E_CAPTURE_OPTIONAL( 
										E_CAPTURE( E_DELIMITER )
										E_CAPTURE( E_REAL_NUMBER ) /* #40 */

										E_CAPTURE_OPTIONAL( 
											E_CAPTURE( E_DELIMITER )
											E_CAPTURE( E_REAL_NUMBER ) /* #43 */

											E_CAPTURE_OPTIONAL( 
												E_CAPTURE( E_DELIMITER )
												E_CAPTURE( E_REAL_NUMBER ) /* #46 */

												E_CAPTURE_OPTIONAL( 
													E_CAPTURE( E_DELIMITER )
													E_CAPTURE( E_REAL_NUMBER ) /* #49 */

													E_CAPTURE_OPTIONAL( 
														E_CAPTURE( E_DELIMITER )
														E_CAPTURE( E_REAL_NUMBER ) /* #52 */

														E_CAPTURE_OPTIONAL( 
															E_CAPTURE( E_DELIMITER )
															E_CAPTURE( E_REAL_NUMBER ) /* #55 15th gear, max */
														)
													)
												)
											)
										)
									)
								)
							)
						)
					)
				)
			)
		)
	) 
	E_CAPTURE_OPTIONAL( /* #56 Terminator (optional for backwards compatibility) */
		E_CAPTURE( E_DELIMITER )
		E_MINUS_ONE_REAL 
	) 
	E_CAPTURE_OPTIONAL( E_ILLEGAL_TRAILING_STRING ) /* #57 Invalid text */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_ENGOPTION,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_REAL_NUMBER ) /* #1 Inertia */
	E_CAPTURE_OPTIONAL( 
		E_CAPTURE( E_DELIMITER ) 
		E_CAPTURE( "[ct]" ) /* #4 Type */

		E_CAPTURE_OPTIONAL( 
			E_CAPTURE( E_DELIMITER )
			E_CAPTURE( E_REAL_NUMBER ) /* #7 Clutch force */

			E_CAPTURE_OPTIONAL( 
				E_CAPTURE( E_DELIMITER )
				E_CAPTURE( E_REAL_NUMBER ) /* #10 Shift time */

				E_CAPTURE_OPTIONAL( 
					E_CAPTURE( E_DELIMITER )
					E_CAPTURE( E_REAL_NUMBER ) /* #13 Clutch time */

					E_CAPTURE_OPTIONAL( 
						E_CAPTURE( E_DELIMITER )
						E_CAPTURE( E_REAL_NUMBER ) /* #16 Post shift time */

						E_CAPTURE_OPTIONAL( 
							E_CAPTURE( E_DELIMITER )
							E_CAPTURE( E_REAL_NUMBER ) /* #19 Idle RPM */

							E_CAPTURE_OPTIONAL( 
								E_CAPTURE( E_DELIMITER )
								E_CAPTURE( E_REAL_NUMBER ) /* #22 Stall RMP */

								E_CAPTURE_OPTIONAL( 
									E_CAPTURE( E_DELIMITER )
									E_CAPTURE( E_REAL_NUMBER ) /* #25 Max idle mixture */

									E_CAPTURE_OPTIONAL( 
										E_CAPTURE( E_DELIMITER )
										E_CAPTURE_OPTIONAL( E_REAL_NUMBER ) /* #28 Min idle mixture */
									)
								)
							)
						)
					)
				)
			)
		)
	)
	E_CAPTURE_OPTIONAL( E_ILLEGAL_TRAILING_STRING ) /* #29 Illegal text */ 
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_EXHAUSTS,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID )   /* Reference node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID )   /* Direction node */
	E_CAPTURE_OPTIONAL( 
		E_DELIMITER_COMMA 
		E_CAPTURE( E_REAL_NUMBER ) /* #4 Factor */

		E_CAPTURE_OPTIONAL( 
			E_DELIMITER_SPACE
			E_CAPTURE( E_STRING_NO_SPACES ) /* #6 Material name */
		)
	)
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( INLINE_SECTION_EXTCAMERA,
	"^extcamera"
	E_CAPTURE( /* #1 Wrap */
		E_CAPTURE( /* #2 */
			E_DELIMITER_SPACE 
			"classic"
		)
		E_OR
		E_CAPTURE( /* #3 */
			E_DELIMITER_SPACE 
			"cinecam"
		)
		E_OR
		E_CAPTURE( /* #4 */
			E_DELIMITER_SPACE 
			"node"

			E_CAPTURE( /* #5 */
				E_DELIMITER_SPACE
				E_CAPTURE( E_NODE_ID ) /* #6 */
			)
		)
	)
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( INLINE_SECTION_FILE_FORMAT_VERSION, 
	"^fileformatversion"
	E_DELIMITER_SPACE
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* #1 The version */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( INLINE_SECTION_FILEINFO, 
	"^fileinfo"
	E_DELIMITER_SPACE
	E_CAPTURE(
		E_CAPTURE( E_MINUS_ONE_REAL )   /* #2 No UID */
		E_OR
		E_CAPTURE( E_STRING_NO_SPACES ) /* #3 UID */ 
	)
	E_CAPTURE_OPTIONAL( /* #4 Wrapper */
		E_DELIMITER_COMMA
		E_CAPTURE(      /* #5 Wrapper */
			E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* #6 Category */
			E_OR
			E_CAPTURE( E_MINUS_ONE_REAL )          /* #7 No category */
		)

		E_CAPTURE_OPTIONAL( /* #8 Wrapper */
			E_DELIMITER_COMMA
			E_CAPTURE(      /* #9 Wrapper */
				E_CAPTURE( E_DECIMAL_NUMBER ) /* #10 File version, integer */
				E_OR
				E_CAPTURE( E_REAL_NUMBER ) /* #11 File version, float (backwards compatibility) */
			)
		)
	)
	E_TRAILING_WHITESPACE
	);

/* This section accepts both space and comma as valid separators. It checks which one is used, hence the complexity */
DEFINE_REGEX( SECTION_FLARES,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID )      /* Reference node */
	E_CAPTURE( 
		E_CAPTURE( E_DELIMITER_SPACE )
		E_OR 
		E_CAPTURE( E_DELIMITER_COMMA )
	)
	E_CAPTURE( E_REAL_NUMBER )  /* #5 X */
	E_CAPTURE( 
		E_CAPTURE( E_DELIMITER_SPACE )
		E_OR 
		E_CAPTURE( E_DELIMITER_COMMA )
	)
	E_CAPTURE( E_REAL_NUMBER )  /* #9 Y */
	E_CAPTURE( 
		E_CAPTURE( E_DELIMITER_SPACE )
		E_OR 
		E_CAPTURE( E_DELIMITER_COMMA )
	)
	E_CAPTURE( E_REAL_NUMBER )  /* #13 X offset */
	E_CAPTURE( 
		E_CAPTURE( E_DELIMITER_SPACE )
		E_OR 
		E_CAPTURE( E_DELIMITER_COMMA )
	)
	E_CAPTURE( E_REAL_NUMBER )  /* #17 Y offset */
	E_CAPTURE_OPTIONAL(
		E_CAPTURE( 
			E_CAPTURE( E_DELIMITER_SPACE )
			E_OR 
			E_CAPTURE( E_DELIMITER_COMMA )
		)
		E_CAPTURE( "[[:alnum:]]+" ) /* #22 Type flags (for backwards compatibility, accept anything) */

		E_CAPTURE_OPTIONAL(
			E_CAPTURE( 
				E_CAPTURE( E_DELIMITER_SPACE )
				E_OR 
				E_CAPTURE( E_DELIMITER_COMMA )
			)
			E_CAPTURE( E_DECIMAL_NUMBER ) /* #27 Control number */

			E_CAPTURE_OPTIONAL(
				E_CAPTURE( 
					E_CAPTURE( E_DELIMITER_SPACE )
					E_OR 
					E_CAPTURE( E_DELIMITER_COMMA )
				)
				E_CAPTURE( E_DECIMAL_NUMBER ) /* #32 Blink delay */

				E_CAPTURE_OPTIONAL(
					E_CAPTURE( 
						E_CAPTURE( E_DELIMITER_SPACE )
						E_OR 
						E_CAPTURE( E_DELIMITER_COMMA )
					)
					E_CAPTURE( E_REAL_NUMBER ) /* #37 Size */

					E_CAPTURE_OPTIONAL( 
						E_CAPTURE( 
							E_CAPTURE( E_DELIMITER_SPACE )
							E_OR 
							E_CAPTURE( E_DELIMITER_COMMA )
						) 
						E_CAPTURE( E_STRING_NO_SPACES ) /* #42 Material name */ 
					)
				)
			)
		)
	)
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_FLARES_TYPE, "[fblrRu]" );

DEFINE_REGEX( SECTION_FLARES2,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID )      /* Reference node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER )  /* X */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER )  /* Y */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER )  /* #4 X offset */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER )  /* #5 Y offset */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER )  /* #6 Z offset */
	E_CAPTURE_OPTIONAL(
		E_DELIMITER_COMMA
		E_CAPTURE( "[fblrRu]" ) /* #8 Type */

		E_CAPTURE_OPTIONAL(
			E_DELIMITER_COMMA
			E_CAPTURE( E_DECIMAL_NUMBER ) /* #10 Control number */

			E_CAPTURE_OPTIONAL(
				E_DELIMITER_COMMA
				E_CAPTURE( E_DECIMAL_NUMBER ) /* #12 Blink delay */

				E_CAPTURE_OPTIONAL(
					E_DELIMITER_COMMA
					E_CAPTURE( E_REAL_NUMBER ) /* #14 Size */

					E_CAPTURE_OPTIONAL( 
						E_DELIMITER_SPACE 
						E_CAPTURE( E_STRING_NO_SPACES ) /* #16 Material name */ 
					)
				)
			)
		)
	)
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( FLEXBODIES_SUBSECTION_PROPLIKE_LINE,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* X */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Y */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* X offset */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Y offset */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Z offset */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* X rotation */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Y rotation */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Z rotation */
	E_DELIMITER_COMMA
	E_CAPTURE( E_STRING_NO_SPACES ) /* Mesh name */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( FLEXBODIES_SUBSECTION_FORSET_LINE,
	"^forset"
	E_CAPTURE( E_DELIMITER )
	E_CAPTURE( ".*$" ) /* #2 Entire line */
	);

DEFINE_REGEX( FORSET_ELEMENT,
	E_CAPTURE( /* #1 Range with numbered nodes */
		E_LEADING_WHITESPACE
		E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* #2 Range start */
		"[[:blank:]]*-[[:blank:]]*"
		E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* #3 Range end */
		E_TRAILING_WHITESPACE
	)
	E_OR
	E_CAPTURE( /* #4 Solitary node (numbered or named) */
		E_LEADING_WHITESPACE
		E_NODE_ID
		E_TRAILING_WHITESPACE
	)
	);

DEFINE_REGEX( SECTION_FLEXBODYWHEELS,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_REAL_NUMBER ) /* Tire radius */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Rim radius */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Width */
	E_DELIMITER_COMMA
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* Num rays */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Node 1 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Node 2 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Ref. node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* Braked? */
	E_DELIMITER_COMMA
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* Propulsed? */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Reference arm node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Weight */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Tire spring */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Tire damp */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Rim spring */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Rim damp */
	E_DELIMITER_COMMA
	E_CAPTURE( "[lr]" ) /* Rim orientation */
	E_DELIMITER_COMMA
	E_CAPTURE( E_STRING_NO_SPACES ) /* Rim mesh */
	E_DELIMITER_SPACE
	E_CAPTURE( E_STRING_NO_SPACES ) /* Tire mesh */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_FUSEDRAG,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* #1 Node 1 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* #2 Node 2 */
	E_DELIMITER_COMMA
	E_CAPTURE_OPTIONAL( /* #3 */
		E_CAPTURE( E_REAL_NUMBER ) /* #4 Approx. width */
		E_DELIMITER_COMMA
		E_CAPTURE( E_STRING_NO_SPACES ) /* #5 Airfoil name */
		E_TRAILING_WHITESPACE
	)
	E_CAPTURE_OPTIONAL( /* #6 */
		"autocalc"
		E_CAPTURE_OPTIONAL( /* #7 */
			E_DELIMITER_COMMA
			E_CAPTURE( E_REAL_NUMBER ) /* #8 Area coeff. */

			E_CAPTURE_OPTIONAL( /* #9 */
				E_DELIMITER_SPACE
				E_CAPTURE( E_STRING_NO_SPACES ) /* #10 Airfoil name */
			)
		)
		E_TRAILING_WHITESPACE
	)
	);

DEFINE_REGEX( SECTION_GLOBALS,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_REAL_NUMBER ) /* Dry mass */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Cargo mass */
	E_CAPTURE_OPTIONAL( 
		E_DELIMITER_COMMA
		E_CAPTURE( E_STRING_NO_SPACES ) /* Truck submesh material */
	)
	E_CAPTURE_OPTIONAL( E_STRING_ANYTHING_BUT_WHITESPACE ) /* #5 Illegal characters at the end */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_GUID,
	"^guid"
	E_DELIMITER_SPACE
	E_CAPTURE( E_STRING_NO_SPACES )
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_GUISETTINGS,
	E_LEADING_WHITESPACE
	E_CAPTURE( /* #1 */
		E_CAPTURE( /* #2 */
			"tachoMaterial"
			E_DELIMITER_SPACE 
			E_CAPTURE( E_STRING_NO_SPACES ) /* #3 The name of the tachometer face material. */
			E_TRAILING_WHITESPACE
		) 
		E_OR
		E_CAPTURE( 
			"speedoMaterial" 
			E_DELIMITER_SPACE 
			E_CAPTURE( E_STRING_NO_SPACES ) /* #5 The name of the speedometer face material. */
			E_TRAILING_WHITESPACE
		)
		E_OR
		E_CAPTURE( 
			"speedoMax" 
			E_DELIMITER_SPACE 
			E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* #7 The highest number that is on the speedometer. */
			E_TRAILING_WHITESPACE
		)
		E_OR
		E_CAPTURE( 
			"useMaxRPM" 
			E_DELIMITER_SPACE 
			E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* #9 Boolean (0/1): Use max RPM from ENGINE section as tacho-max */
			E_TRAILING_WHITESPACE
		)
		E_OR
		E_CAPTURE( 
			"helpMaterial" 
			E_DELIMITER_SPACE 
			E_CAPTURE( E_STRING_NO_SPACES ) /* #11 A picture that shows command instructions */ 
			E_TRAILING_WHITESPACE
		)
		E_OR
		E_CAPTURE( /* #12 */ 
			"interactiveOverviewMap" /* Enum: off, simple, zoom */
			E_DELIMITER_SPACE 
			E_CAPTURE( /* #13 */
				E_CAPTURE( "off" ) /* #14 */
				E_OR
				E_CAPTURE( "simple" ) /* #15 */
				E_OR
				E_CAPTURE( "zoom" ) /* #16 */
			)
			E_TRAILING_WHITESPACE
		)
		E_OR
		E_CAPTURE( 
			"dashboard"
			E_DELIMITER_SPACE 
			E_CAPTURE( E_STRING_NO_SPACES ) /* #18 MyGUI layout file for dashboard. You can use multiple lines. */
			E_TRAILING_WHITESPACE
		)
		E_OR
		E_CAPTURE( 
			"texturedashboard"
			E_DELIMITER_SPACE 
			E_CAPTURE( E_STRING_NO_SPACES ) /* #20 MyGUI layout file for the RTT dashboard of this truck. You can use multiple lines. */
			E_TRAILING_WHITESPACE
		)
		E_OR
		E_CAPTURE( /* Obsolete, ignored */
			"debugBeams"
			E_CAPTURE_OPTIONAL( E_DELIMITER_SPACE E_BOOLEAN ) 
			E_TRAILING_WHITESPACE
		)
	)
	);

DEFINE_REGEX( SECTION_HELP,
	E_CAPTURE( E_STRING_NO_SPACES ) /* Material name */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_HOOKS,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Node */
	E_DELIMITER_COMMA
	E_CAPTURE( ".*$" ) /* Options string */
	);

DEFINE_REGEX( HOOKS_OPTIONS,
	E_LEADING_WHITESPACE
	E_CAPTURE( /* #1 */
		E_CAPTURE("self-lock") /* #2 This hook can lock to the truck its placed on too */
		E_OR
		E_CAPTURE("auto-lock") /* This hook will lock automatically to valid nodes in range */
		E_OR
		E_CAPTURE("nodisable") /* Linkage beam won't be disabled, but hook will stop pulling node to lock */
		E_OR
		E_CAPTURE("norope")    /* Linkage will act like a beam and not like a rope */
		E_OR
		E_CAPTURE("visible")   /* #6 Linkage will be visible while locking process and locked */
		E_OR
		E_CAPTURE(
				"hookrange"
				E_DELIMITER_COLON
				E_CAPTURE( E_REAL_NUMBER ) /* #8 The range a hook scans for a valid node to lock to */
			)
		E_OR
		E_CAPTURE(
				"maxforce"
				E_DELIMITER_COLON
				E_CAPTURE( E_REAL_NUMBER ) /* #10 The force limit where a locking attempt is canceld */
			)
		E_OR
		E_CAPTURE(
				"hookgroup"
				E_DELIMITER_COLON
				E_CAPTURE( E_DECIMAL_NUMBER ) /* #12 The hookgroup a hook belongs to */
			)
		E_OR
		E_CAPTURE(
				"lockgroup"
				E_DELIMITER_COLON
				E_CAPTURE( E_DECIMAL_NUMBER ) /* #14 The lockgroup a hook belongs */
			)
		E_OR
		E_CAPTURE(
				"timer"
				E_DELIMITER_COLON
				E_CAPTURE( E_REAL_NUMBER ) /* #16 Delay timer for autolocking hooks before they attempt to relock. */
			)
		E_OR
		E_CAPTURE(
				"shortlimit"
				E_DELIMITER_COLON
				E_CAPTURE( E_REAL_NUMBER ) /* #18 Minimum range in meters the hook will pull the node to lock to */
			)
		E_OR
		E_CAPTURE(
				"speedcoef"
				E_DELIMITER_COLON
				E_CAPTURE( E_REAL_NUMBER ) /* #20 The speed a hook pulls the node to lock into locking position */
			)
	)
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_HYDROS,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* #1 Node 1 */
	E_CAPTURE( E_DELIMITER )
	E_CAPTURE( E_NODE_ID ) /* #3 Node 2 */
	E_CAPTURE( E_DELIMITER )
	E_CAPTURE( E_REAL_NUMBER ) /* #5 Lengthening factor */
	E_CAPTURE_OPTIONAL( 
		E_CAPTURE( E_DELIMITER )
		E_CAPTURE( "[areuvxyghis]*" ) /* #8 Flags */

		E_CAPTURE_OPTIONAL(
			E_CAPTURE( E_DELIMITER )
			E_CAPTURE( E_REAL_NUMBER ) /* #11 Inertia: start delay */

			E_CAPTURE_OPTIONAL(
				E_CAPTURE( E_DELIMITER )
				E_CAPTURE( E_REAL_NUMBER ) /* #14 Inertia: stop delay */

				E_CAPTURE_OPTIONAL(
					E_CAPTURE( E_DELIMITER )
					E_CAPTURE( E_STRING_NO_SPACES ) /* #17 Inertia: start function */

					E_CAPTURE_OPTIONAL(
						E_CAPTURE( E_DELIMITER )
						E_CAPTURE( E_STRING_NO_SPACES ) /* #20 Inertia: stop function */
					)
				)
			)
		)
	)
	E_CAPTURE_OPTIONAL( E_ILLEGAL_TRAILING_STRING ) /* #21 Invalid text */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_LOCKGROUPS,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_DECIMAL_NUMBER ) /* Group number */
	E_DELIMITER_COMMA
	E_CAPTURE( /* Node list */
		E_CAPTURE(
			E_NODE_ID
			E_DELIMITER_COMMA		
		) "*"
		E_CAPTURE(
			E_NODE_ID
			E_TRAILING_WHITESPACE
		) 
	) 
	);

DEFINE_REGEX( SECTION_MANAGEDMATERIALS,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_STRING_NO_SPACES ) /* #1 Material name */
	E_CAPTURE( E_DELIMITER )        /* #2 */
	E_CAPTURE(                      /* #3 Type wrapper */
		E_CAPTURE( "mesh_standard" )        /* #4 */
		E_OR
		E_CAPTURE( "mesh_transparent" )     /* #5 */
		E_OR
		E_CAPTURE( "flexmesh_standard" )    /* #6 */
		E_OR
		E_CAPTURE( "flexmesh_transparent" ) /* #7 */
	)
	E_CAPTURE( E_DELIMITER )
	E_CAPTURE( E_STRING_NO_SPACES )         /* #9 Diffuse map filename */
	E_CAPTURE_OPTIONAL( 
		E_CAPTURE( E_DELIMITER )
		E_CAPTURE( E_STRING_NO_SPACES )     /* #12 */
		E_CAPTURE_OPTIONAL( 
			E_CAPTURE( E_DELIMITER )
			E_CAPTURE( E_STRING_NO_SPACES ) /* #15 */
		)
	)
	E_CAPTURE_OPTIONAL( E_ILLEGAL_TRAILING_STRING )
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_MATERIALFLAREBINDINGS,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* #1 Flare number */
	E_CAPTURE_OPTIONAL( "[[:alpha:]]+" )   /* #2 Tolerated characters after flare number (backwards compatibility) */
	E_CAPTURE(                             /* #3 Separator */
		E_DELIMITER_COMMA
		E_OR
		E_DELIMITER_SPACE /* Backwards compatibility */
	)
	E_CAPTURE( E_STRING_NO_SPACES )        /* #4 Material name */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_MESHWHEELS_MESHWHEELS2,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_REAL_NUMBER ) /* Tyre radius */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Rim radius */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Width */
	E_DELIMITER_COMMA
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* Num rays */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Node 1 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Node 2 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Ref. node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* Braked? */
	E_DELIMITER_COMMA
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* Propulsed? */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* #10 Reference arm node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Mass*/
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Tyre spring */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Tyre damping */
	E_DELIMITER_COMMA
	E_CAPTURE( "[lr]" ) /* Rim orientation */
	E_DELIMITER_COMMA
	E_CAPTURE( E_STRING_NO_SPACES ) /* Rim mesh name */
	E_DELIMITER_SPACE
	E_CAPTURE( E_STRING_NO_SPACES ) /* Tyre material name */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_MINIMASS,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_REAL_NUMBER ) /* Min. default mass */
	E_TRAILING_WHITESPACE
	);

/* FIXME: Undocumented by RoR, may not be correct */
DEFINE_REGEX( SECTION_NODECOLLISION,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Radius */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_NODES_N2,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Node id (integer or string) */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* X */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Y */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Z */
	E_CAPTURE_OPTIONAL( 
		E_DELIMITER_COMMA
		E_CAPTURE( "[lnmfxychebpL]*" ) /* #6 Options */

		E_CAPTURE_OPTIONAL( 
			E_DELIMITER_SPACE
			E_CAPTURE( E_REAL_NUMBER ) /* #8 Load weight override */
		)
	)
	E_2xCAPTURE_TRAILING_COMMENT
	);

DEFINE_REGEX( SECTION_PARTICLES,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Emit node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Ref node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_STRING_NO_SPACES ) /* Particle sys. name */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_PISTONPROPS,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Reference node (center of the prop) */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Prop axis node (back of the prop) */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Blade 1 tip node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Blade 2 tip node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* #5 Blade 3 tip node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* #6 Blade 4 tip node */
	E_DELIMITER_COMMA
	E_CAPTURE ( /* #7 Wrapper */
		E_CAPTURE( E_MINUS_ONE_REAL ) /* #8 Couple node unused */
		E_OR
		E_CAPTURE( E_NODE_ID ) /* #9 Couple node */	
	)
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* #10 Power (in kW) */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* #11 Pitch */
	E_DELIMITER_COMMA
	E_CAPTURE( E_STRING_NO_SPACES ) /* #12 Airfoil */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_PROPS,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID )             /* #1 Ref. node */
	E_CAPTURE( E_DELIMITER )
	E_CAPTURE( E_NODE_ID )             /* #3 X axis node */
	E_CAPTURE( E_DELIMITER )
	E_CAPTURE( E_NODE_ID )             /* #5 Y axis node */
	E_CAPTURE( E_DELIMITER )
	E_CAPTURE( E_REAL_NUMBER )         /* #7 X offset */
	E_CAPTURE( E_DELIMITER )
	E_CAPTURE( E_REAL_NUMBER )         /* #9 Y offset */
	E_CAPTURE( E_DELIMITER )
	E_CAPTURE( E_REAL_NUMBER )         /* #11 Z offset */
	E_CAPTURE( E_DELIMITER )
	E_CAPTURE( E_REAL_NUMBER )         /* #13 rot. X */
	E_CAPTURE( E_DELIMITER )
	E_CAPTURE( E_REAL_NUMBER )         /* #15 rot. Y */
	E_CAPTURE( E_DELIMITER )
	E_CAPTURE( E_REAL_NUMBER )         /* #17 rot. Z */
	E_CAPTURE( E_DELIMITER )
	E_CAPTURE( E_STRING_NO_SPACES )    /* #19 Mesh name */
	E_CAPTURE_OPTIONAL( E_DELIMITER )
	E_CAPTURE( ".*$" )                 /* #21 Special mesh options part */
	);

/* IMPORTANT! Result indexes must match values from RigDef::Prop::Special enum */
DEFINE_REGEX( SPECIAL_PROPS,
	E_CAPTURE( "^leftmirror.*$" ) /* #1 */
	E_OR
	E_CAPTURE( "^rightmirror.*$" )
	E_OR
	E_CAPTURE( "^dashboard.*$" )
	E_OR
	E_CAPTURE( "^dashboard-rh.*$" )
	E_OR
	E_CAPTURE( "^spinprop.*$" ) 
	E_OR
	E_CAPTURE( "^pale.*$" ) 
	E_OR
	E_CAPTURE( "^seat.*$" )
	E_OR
	E_CAPTURE( "^seat2.*$" )
	E_OR
	E_CAPTURE( "^beacon.*$" )
	E_OR
	E_CAPTURE( "^redbeacon.*$" )
	E_OR	
	E_CAPTURE( "^lightbar.*$" ) /* #11 */
	);

DEFINE_REGEX( SPECIAL_PROP_DASHBOARD,
	E_LEADING_WHITESPACE
	E_CAPTURE_OPTIONAL( E_STRING_ANYTHING_BUT_WHITESPACE ) /* #1 Mesh name */
	E_CAPTURE_OPTIONAL(
		E_CAPTURE(
			E_DELIMITER_COMMA
			E_OR
			E_DELIMITER_SPACE
		)
		E_CAPTURE( E_REAL_NUMBER ) /* #4 X offset */
	)
	E_CAPTURE_OPTIONAL(
		E_CAPTURE(
			E_DELIMITER_COMMA
			E_OR
			E_DELIMITER_SPACE
		)
		E_CAPTURE( E_REAL_NUMBER ) /* #7 Y offset */
	)
	E_CAPTURE_OPTIONAL(
		E_CAPTURE(
			E_DELIMITER_COMMA
			E_OR
			E_DELIMITER_SPACE
		)
		E_CAPTURE( E_REAL_NUMBER ) /* #10 Z offset */
	)
	E_CAPTURE_OPTIONAL(
		E_CAPTURE(
			E_DELIMITER_COMMA
			E_OR
			E_DELIMITER_SPACE
		)
		E_CAPTURE( E_REAL_NUMBER ) /* #13 Rotation, degrees */
	)
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SPECIAL_PROP_BEACON,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_STRING_NO_SPACES ) /* #1 */
	E_CAPTURE(
		E_DELIMITER_COMMA
		E_OR
		E_DELIMITER_SPACE
	)
	E_CAPTURE( E_REAL_NUMBER ) /* #3 Red */
	E_CAPTURE(
		E_DELIMITER_COMMA
		E_OR
		E_DELIMITER_SPACE
	)
	E_CAPTURE( E_REAL_NUMBER ) /* #5 Green */
	E_CAPTURE(
		E_DELIMITER_COMMA
		E_OR
		E_DELIMITER_SPACE
	)
	E_CAPTURE( E_REAL_NUMBER ) /* #7 Blue */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_RAILGROUPS,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Id */
	E_DELIMITER_COMMA
	E_CAPTURE( "([[:blank:]]*" E_CAPTURE( E_NODE_ID ) "[[:blank:]]*,)*([[:blank:]]*" E_CAPTURE( E_NODE_ID ) "[[:blank:]]*)+$" ) /* Node list */
	);

DEFINE_REGEX( SECTION_ROPABLES,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Node */

	E_CAPTURE_OPTIONAL( 
		E_DELIMITER_COMMA
		E_CAPTURE( E_DECIMAL_NUMBER ) /* Group */
	
		E_CAPTURE_OPTIONAL( 
			E_DELIMITER_COMMA
			E_CAPTURE( "[01]" )
		)
	)
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_ROPES,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Root node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* End node */
	E_CAPTURE_OPTIONAL( 
		E_DELIMITER_COMMA
		E_CAPTURE( "[i]" ) /* Flags */
	)
	E_TRAILING_WHITESPACE
	);

#define E_ROTATORS_R1_R2_COMMON_BASE                                \
	E_LEADING_WHITESPACE                                            \
	E_CAPTURE( E_NODE_ID )                 /* Axis node 1 */        \
	E_DELIMITER_COMMA                                                  \
	E_CAPTURE( E_NODE_ID )                 /* Axis node 2 */        \
	E_DELIMITER_COMMA                                                  \
	E_CAPTURE( E_NODE_ID )                 /* Baseplate node 1 */   \
	E_DELIMITER_COMMA                                                  \
	E_CAPTURE( E_NODE_ID )                 /* Baseplate node 2 */   \
	E_DELIMITER_COMMA                                                  \
	E_CAPTURE( E_NODE_ID )                 /* Baseplate node 3 */   \
	E_DELIMITER_COMMA                                                  \
	E_CAPTURE( E_NODE_ID )                 /* Baseplate node 4 */   \
	E_DELIMITER_COMMA                                                  \
	E_CAPTURE( E_NODE_ID )                 /* Rot. plate node 1 */  \
	E_DELIMITER_COMMA                                                  \
	E_CAPTURE( E_NODE_ID )                 /* Rot. plate node 2 */  \
	E_DELIMITER_COMMA                                                  \
	E_CAPTURE( E_NODE_ID )                 /* Rot. plate node 3 */  \
	E_DELIMITER_COMMA                                                  \
	E_CAPTURE( E_NODE_ID )                 /* Rot. plate node 4 */  \
	E_DELIMITER_COMMA                                                  \
	E_CAPTURE( E_REAL_NUMBER )             /* Rate */               \
	E_DELIMITER_COMMA                                                  \
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* Key left */           \
	E_DELIMITER_COMMA                                                  \
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* #13 Key right */

#define E_ROTATORS_R1_R2_COMMON_INERTIA                                            \
	E_CAPTURE( E_REAL_NUMBER )                           /* #+1 Start delay */     \
	E_CAPTURE_OPTIONAL(                                                            \
		E_DELIMITER_COMMA                                                             \
		E_CAPTURE( E_REAL_NUMBER )                       /* #+3 Stop delay */      \
		E_CAPTURE_OPTIONAL(                                                        \
			E_DELIMITER_COMMA                                                         \
			E_CAPTURE( E_REAL_NUMBER )                   /* #+5 Start fn. */       \
			E_CAPTURE_OPTIONAL(                                                    \
				E_DELIMITER_COMMA                                                     \
				E_CAPTURE( E_INERTIA_FUNCTION )          /* #+7 Stop fn. */        \
				E_CAPTURE_OPTIONAL(                                                \
					E_DELIMITER_COMMA                                                 \
					E_CAPTURE( E_REAL_NUMBER )           /* #+9 Engine coupling */ \
					E_CAPTURE_OPTIONAL(                                            \
						E_DELIMITER_COMMA                                             \
						E_CAPTURE( E_BOOLEAN ) ) ) ) ) ) /* #+11 Needs engine */
					                                                 
	

DEFINE_REGEX( SECTION_ROTATORS,
	E_ROTATORS_R1_R2_COMMON_BASE
	E_CAPTURE_OPTIONAL( /* #14 */
		E_DELIMITER_COMMA
		E_ROTATORS_R1_R2_COMMON_INERTIA
	)
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_ROTATORS2,
	E_ROTATORS_R1_R2_COMMON_BASE
	E_CAPTURE_OPTIONAL( 
		E_DELIMITER_COMMA
		E_CAPTURE( E_REAL_NUMBER )      /* #15 Rotating force */
	
		E_CAPTURE_OPTIONAL( 
			E_DELIMITER_COMMA
			E_CAPTURE( E_REAL_NUMBER )      /* Anti-jitter tolerance */
	
			E_CAPTURE_OPTIONAL( 
				E_DELIMITER_COMMA
				E_CAPTURE( E_STRING_NO_SPACES ) /* Description */
	
				E_CAPTURE_OPTIONAL( /* #20 */
					E_DELIMITER_COMMA   
					E_ROTATORS_R1_R2_COMMON_INERTIA
				)
			)
		)
	)
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_SCREWPROPS,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Prop node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Back node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Top node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Power */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( INLINE_SECTION_SET_SKELETON_DISPLAY,
	"^set_skeleton_settings"
	E_DELIMITER_SPACE
	E_CAPTURE( E_REAL_NUMBER ) /* View distance */
	E_CAPTURE_OPTIONAL( 
		E_DELIMITER_COMMA
		E_CAPTURE( E_REAL_NUMBER ) /* Thickness */
	)
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( INLINE_SECTION_SET_COLLISION_RANGE,
	"^set_collision_range"
	E_DELIMITER_SPACE
	E_CAPTURE( E_REAL_NUMBER ) /* Range */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_SHOCKS,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Node 1 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Node 2 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Spring */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Damping */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Short bound */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Long bound */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* #7 Precompression */
	E_CAPTURE_OPTIONAL( 
		E_DELIMITER_COMMA
		E_CAPTURE( "[^[:blank:],;|]+" ) /* #9 Options */
	)
	E_CAPTURE_OPTIONAL( /* #10 Invalid input */
		E_CAPTURE( E_DELIMITER_COMMA E_OR E_DELIMITER_SPACE )
		E_CAPTURE( E_STRING_ANYTHING_BUT_WHITESPACE )
	)
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_SHOCKS2,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Node 1 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Node 2 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Spring in */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Damping in */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* #5 Spring in progression factor */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Damping in progression factor */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Spring out */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Damping out */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Spring out progression factor */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* #10 Damping out progression factor */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Short bound */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Long bound */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* #13 Precompression */
	E_CAPTURE_OPTIONAL( 
		E_DELIMITER_COMMA
		E_CAPTURE( E_STRING_NO_SPACES ) /* #15 Options */
	)
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_SLIDENODES,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* #1 The sliding node */
	E_DELIMITER_COMMA
	E_CAPTURE( ".*$" ) /* #2 All the rest: [rail nodes list] parameters */
	);

DEFINE_REGEX( SLIDENODES_IDENTIFY_OPTION,
	E_LEADING_WHITESPACE
	E_CAPTURE_OPTIONAL( /* #1 Spring rate entry */
		"[sS]"
		E_CAPTURE( E_REAL_NUMBER ) /* #2 Spring rate value */
	)
	E_CAPTURE_OPTIONAL( /* #3 Break force entry */
		"[bB]"
		E_CAPTURE( E_REAL_NUMBER ) /* #4 Break force value */
	)
	E_CAPTURE_OPTIONAL( /* #5 Tolerance entry */
		"[tT]"
		E_CAPTURE( E_REAL_NUMBER ) /* #6 Tolerance value */
	)
	E_CAPTURE_OPTIONAL( /* #7 Attachment rate entry */
		"[rR]"
		E_CAPTURE( E_REAL_NUMBER ) /* #8 Attachment rate value */
	)
	E_CAPTURE_OPTIONAL( /* #9 Railgroup ID entry */
		"[gG]"
		E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* #10 Railgroup ID value */
	)
	E_CAPTURE_OPTIONAL( /* #11 Max. attachment distance entry */
		"[dD]"
		E_CAPTURE( E_REAL_NUMBER ) /* #12 Max. attachment distance value */
	)
	E_CAPTURE_OPTIONAL( /* #13 Quantity entry */
		"[qQ]"
		E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* #14 Quantity value */
	)
	E_CAPTURE_OPTIONAL( /* #15 Constraints entry */
		"[cC]"
		E_CAPTURE( "[afsn]" ) /* #16 Constraints value */
	)
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( INLINE_SECTION_SLOPE_BRAKE,
	"^SlopeBrake"
	E_CAPTURE_OPTIONAL(
		E_DELIMITER_SPACE
		E_CAPTURE( E_REAL_NUMBER ) /* #2 Regulating force */

		E_CAPTURE_OPTIONAL(
			E_DELIMITER_COMMA
			E_CAPTURE( E_REAL_NUMBER ) /* #4 Attach-angle */

			E_CAPTURE_OPTIONAL(
				E_DELIMITER_COMMA
				E_CAPTURE( E_REAL_NUMBER ) /* #6 Detach-angle */
			)
		)
	)
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_SOUNDSOURCES,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Node which makes the sound */
	E_DELIMITER_COMMA
	E_CAPTURE( E_STRING_NO_SPACES ) /* Sound script name */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_SOUNDSOURCES2,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Node which makes the sound */
	E_DELIMITER_COMMA
	E_CAPTURE( E_STRING_ANYTHING_BUT_WHITESPACE ) /* Mode/Cinecam ID, decimal number (accept anything for backward compatibility) */
	E_DELIMITER_COMMA
	E_CAPTURE( E_STRING_NO_SPACES ) /* Sound script name */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( INLINE_SECTION_SPEEDLIMITER,
	"^speedlimiter"
	E_DELIMITER_SPACE
	E_CAPTURE( E_REAL_NUMBER ) /* Max speed */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SUBMESH_SUBSECTION_TEXCOORDS,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Node ID */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* U coordinate */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* V coordinate */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SUBMESH_SUBSECTION_CAB,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Vertex 1 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Vertex 2 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Vertex 3 */
	E_CAPTURE_OPTIONAL( 
		E_DELIMITER_COMMA
		E_CAPTURE( "[[:alpha:]]*" ) /* Options */
	)
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( INLINE_SECTION_SUBMESH_GROUNDMODEL,
	"^submesh_groundmodel"
	E_DELIMITER_SPACE
	E_CAPTURE( E_STRING_NO_SPACES ) /* Ground type name */	
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_TIES,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Root node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Max. reach length */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Auto shorten rate */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Min length */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Max length */
	E_CAPTURE_OPTIONAL( 
		E_DELIMITER_COMMA
		E_CAPTURE( "[in]" ) /* Options */

		E_CAPTURE_OPTIONAL( 
			E_DELIMITER_COMMA
			E_CAPTURE( E_REAL_NUMBER ) /* Max stress */

			E_CAPTURE_OPTIONAL( 
				E_DELIMITER_COMMA
				E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* Group */
			)
		)
	)
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_TORQUECURVE,
	E_LEADING_WHITESPACE
	E_CAPTURE( /* #1 Whole custom-curve line */
		E_CAPTURE( E_REAL_NUMBER ) /* #2 Power */
		E_DELIMITER_COMMA
		E_CAPTURE( E_REAL_NUMBER ) /* #3 Percentage */
	)
	E_OR
	E_CAPTURE( E_STRING_NO_SPACES ) /* #4 Known function */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_TRACTION_CONTROL,
	"^TractionControl"
	E_DELIMITER_SPACE
	E_CAPTURE( E_REAL_NUMBER ) /* Force */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Wheelslip */
	E_CAPTURE_OPTIONAL( 
		E_DELIMITER_COMMA 
		E_CAPTURE( E_REAL_NUMBER ) /* #4 Fade speed */

		E_CAPTURE_OPTIONAL( 
			E_DELIMITER_COMMA 
			E_CAPTURE( E_REAL_NUMBER ) /* #6 Pulse/sec */

			E_CAPTURE_OPTIONAL(
				E_DELIMITER_COMMA
				"mode[[:blank:]]*:"
				E_CAPTURE("[[:blank:]ONFDASHTGLEonfdashtgle&]*") /* #8 Mode string */	
			)
		)
	)
	E_TRAILING_WHITESPACE
	);

/* Case insensitive */
DEFINE_REGEX_IGNORECASE( TRACTION_CONTROL_MODE,
	E_LEADING_WHITESPACE
	E_CAPTURE_OPTIONAL( "on" )
	E_CAPTURE_OPTIONAL( "off" )
	E_CAPTURE_OPTIONAL( "nodash" )
	E_CAPTURE_OPTIONAL( "notoggle" )
	E_TRAILING_WHITESPACE
	); 

DEFINE_REGEX( SECTION_TRIGGERS,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Node 1 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Node 2 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Contraction trigger limit */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Expansion trigger limit */
	E_DELIMITER_COMMA
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* Contraction trigger key (or engine index with E option) */
	E_DELIMITER_COMMA
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* #6 Expansion trigger key (or function number with E option) */

	E_CAPTURE_OPTIONAL( 
		E_DELIMITER_COMMA
		E_CAPTURE( "[icxbBAshHtE]*" ) /* #8 Options */
	
		E_CAPTURE_OPTIONAL( 
			E_DELIMITER_COMMA
			E_CAPTURE( E_REAL_NUMBER ) /* #10 Boundary timer */
		)
	)
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_TURBOJETS,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Front */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Back */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Side */
	E_DELIMITER_COMMA
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* Is reversable */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Dry thrust */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Wet thrust */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Front diameter */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Rear diameter */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Nozzle length */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_TURBOPROPS,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Reference node (center of the prop) */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Prop axis node (back of the prop) */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Blade 1 tip node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Blade 2 tip node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Blade 3 tip node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Blade 4 tip node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Power of the turbine (in kW) */
	E_DELIMITER_COMMA
	E_CAPTURE( E_STRING_NO_SPACES ) /* Airfoil of the blades */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_VIDEOCAMERA,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Ref. node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* X node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Y node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Alt. ref. node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* alt. orientation node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Offset X */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Offset Y */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Offset Z */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Rot. X */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* #10 Rot. Y */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Rot. Z */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* FOV */
	E_DELIMITER_COMMA
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* Tex. width */
	E_DELIMITER_COMMA
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* Tex. height */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Min. clip dist. */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Max. clip dist. */
	E_DELIMITER_COMMA
	E_CAPTURE( E_DECIMAL_NUMBER ) /* Camera role */
	E_DELIMITER_COMMA
	E_CAPTURE( E_DECIMAL_NUMBER ) /* Camera mode */
	E_DELIMITER_COMMA
	E_CAPTURE( E_STRING_NO_SPACES ) /* Material name */
	E_CAPTURE_OPTIONAL(  /* #20 */
		E_DELIMITER_COMMA 
		E_CAPTURE( E_STRING_NO_SPACES ) /* #21 Camera name */
	)
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_WHEELS,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_REAL_NUMBER ) /* Radius */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Width */
	E_DELIMITER_COMMA
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* Num. rays */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Node 1 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Node 2 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Rigidity node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* Braking */
	E_DELIMITER_COMMA
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* Propulsion */
	E_DELIMITER_COMMA	
	E_CAPTURE( E_NODE_ID ) /* Reference arm node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* #10 Mass */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Spring */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Damping */
	E_DELIMITER_COMMA
	E_CAPTURE( E_STRING_NO_SPACES ) /* Face material */
	E_DELIMITER_SPACE
	E_CAPTURE( E_STRING_NO_SPACES ) /* Band material */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_WHEELS2,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_REAL_NUMBER ) /* Rim radius */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Tyre radius */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Width */
	E_DELIMITER_COMMA
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* Num. rays */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* #5 Node 1 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Node 2 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Rigidity node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* Braking */
	E_DELIMITER_COMMA
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /* Propulsion */
	E_DELIMITER_COMMA	
	E_CAPTURE( E_NODE_ID ) /* #10 Reference arm node */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Mass */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Spring - rim */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Damping - rim */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* Spring - tyre */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* #15 Damping - tyre */
	E_DELIMITER_COMMA
	E_CAPTURE( E_STRING_NO_SPACES ) /* Face material */
	E_DELIMITER_SPACE
	E_CAPTURE( E_STRING_NO_SPACES ) /* Band material */
	E_TRAILING_WHITESPACE
	);

DEFINE_REGEX( SECTION_WINGS,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /* Box node 1 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /*  */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /*  */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /*  */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /*  */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /*  */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /*  */
	E_DELIMITER_COMMA
	E_CAPTURE( E_NODE_ID ) /* Box node 8 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* #9 Tex. coord 1 */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /*  */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /*  */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /*  */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /*  */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /*  */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /*  */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /* #16 Tex. coord 8 */
	E_CAPTURE_OPTIONAL(
		E_DELIMITER_COMMA
		E_CAPTURE( "[nabferSTcdghUVij]" ) /* #18 Surface type */

		E_CAPTURE_OPTIONAL(
			E_DELIMITER_COMMA
			E_CAPTURE( E_REAL_NUMBER ) /* #20 Chord */

			E_CAPTURE_OPTIONAL(
				E_DELIMITER_COMMA
				E_CAPTURE( E_REAL_NUMBER ) /* Min. deflection */

				E_CAPTURE_OPTIONAL(
					E_DELIMITER_COMMA
					E_CAPTURE( E_REAL_NUMBER ) /* Max. deflection */

					E_CAPTURE_OPTIONAL(
						E_DELIMITER_COMMA
						E_CAPTURE( E_STRING_NO_SPACES ) /* Airfoil */

						E_CAPTURE_OPTIONAL(
							E_DELIMITER_SPACE
							E_CAPTURE( E_REAL_NUMBER ) /* #28 Efficacy coef. */
						)
					)
				)
			)
		)
	)
	E_TRAILING_WHITESPACE
	);

#if 0 // TEMPLATE

DEFINE_REGEX( SECTION_,
	E_LEADING_WHITESPACE
	E_CAPTURE( E_NODE_ID ) /*  */
	E_DELIMITER_COMMA
	E_CAPTURE( E_POSITIVE_DECIMAL_NUMBER ) /*  */
	E_DELIMITER_COMMA
	E_CAPTURE( E_STRING_NO_SPACES ) /*  */
	E_DELIMITER_COMMA
	E_CAPTURE( E_REAL_NUMBER ) /*  */
	E_DELIMITER_COMMA
	E_CAPTURE_OPTIONAL( E_DELIMITER_COMMA )
	E_CAPTURE_OPTIONAL( E_DELIMITER_SPACE E_CAPTURE( E_REAL_NUMBER ) E_TRAILING_WHITESPACE )
	);

#endif

/* -------------------------------------------------------------------------- */
/* Cleanup                                                                    */
/* -------------------------------------------------------------------------- */

#undef E_BACKSLASH
#undef E_SLASH
#undef E_REAL_NUMBER
#undef E_DELIMITER_COMMA
#undef E_DELIMITER_COLON
#undef E_TRAILING_WHITESPACE
#undef E_LEADING_WHITESPACE
#undef E_STRING_NO_SPACES
#undef E_STRING_ALNUM_COMMAS_USCORES_ONLY
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