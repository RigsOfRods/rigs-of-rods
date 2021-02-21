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

namespace Truck
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



DEFINE_REGEX( POSITIVE_DECIMAL_NUMBER, E_POSITIVE_DECIMAL_NUMBER );

DEFINE_REGEX( NEGATIVE_DECIMAL_NUMBER, E_NEGATIVE_DECIMAL_NUMBER );

DEFINE_REGEX( DECIMAL_NUMBER, E_DECIMAL_NUMBER );

DEFINE_REGEX( MINUS_ONE_REAL, E_MINUS_ONE_REAL );

DEFINE_REGEX( REAL_NUMBER, E_REAL_NUMBER );

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

DEFINE_REGEX( SECTION_COLLISIONBOXES,
    E_LEADING_WHITESPACE
    E_DELIMITED_LIST( E_NODE_ID, "," )
    E_TRAILING_WHITESPACE
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

} // namespace Truck
