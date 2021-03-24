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
    @file   
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

namespace RoR {
namespace Truck {

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

/// Actual regex definition macro.
#define DEFINE_REGEX(_NAME_,_REGEXP_) \
    const std::regex _NAME_ = std::regex( _REGEXP_, std::regex::ECMAScript);

// -------------------------------------------------------------------------- //
// Utility regexes                                                            //
// -------------------------------------------------------------------------- //

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

} // namespace Truck
} // namespace RoR
