/*
    This source file is part of Rigs of Rods
    Copyright 2022 Petr Ohlidal

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
/// @brief Generic text file parser
/// 
/// Syntax:
///  - Lines starting with semicolon (;) (ignoring leading whitespace) are comments
///  - Separators are space, tabulator or comma (,)
///  - All strings must be in double quotes (except if OPTION_ALLOW_NAKED_STRINGS is used).
///  - If the first argument on line is an unqoted string, it's considered KEYWORD token type.
///  - Reserved keywords are 'true' and 'false' for the BOOL token type.
/// 
/// Remarks:
///  - Strings cannot be multiline. Linebreak within string ends the string.
///  - KEYWORD tokens cannot start with a digit or special character.

#include "RefCountingObject.h"

#include <vector>
#include <string>

#include <Ogre.h>

namespace RoR {

enum class TokenType
{
    NONE,
    LINEBREAK,    // Input: LF (CR is ignored); Output: platform-specific.
    COMMENT,      // Line starting with ; (skipping whitespace). Data: offset in string pool.
    STRING,       // Quoted string. Data: offset in string pool.
    NUMBER,       // Float.
    BOOL,         // Lowercase 'true'/'false'. Data: 1.0 for true, 0.0 for false.
    KEYWORD,      // Unquoted string at start of line (skipping whitespace). Data: offset in string pool.
};

struct Token
{
    TokenType type;
    float     data;
};

struct GenericDocument: public RefCountingObject<GenericDocument>
{
    static const BitMask_t OPTION_ALLOW_NAKED_STRINGS = BITMASK(1); //!< Allow strings without quotes, for backwards compatibility.
    static const BitMask_t OPTION_ALLOW_SLASH_COMMENTS = BITMASK(2); //!< Allow comments starting with `//`. 
    static const BitMask_t OPTION_FIRST_LINE_IS_TITLE = BITMASK(3); //!< First non-empty & non-comment line is a naked string with spaces.
    static const BitMask_t OPTION_ALLOW_SEPARATOR_COLON = BITMASK(4); //!< Allow ':' as separator between tokens.
    static const BitMask_t OPTION_PARENTHESES_CAPTURE_SPACES = BITMASK(5); //!< If non-empty NAKED string encounters '(', following spaces will be captured until matching ')' is found.

    virtual ~GenericDocument() {};

    std::vector<char> string_pool; // Data of COMMENT/KEYWORD/STRING tokens; NUL-terminated strings.
    std::vector<Token> tokens;
    
    virtual void LoadFromDataStream(Ogre::DataStreamPtr datastream, BitMask_t options = 0);
    virtual void SaveToDataStream(Ogre::DataStreamPtr datastream);

    virtual bool LoadFromResource(std::string resource_name, std::string resource_group_name, BitMask_t options = 0);
    virtual bool SaveToResource(std::string resource_name, std::string resource_group_name);
};

typedef RefCountingObjectPtr<GenericDocument> GenericDocumentPtr;

struct GenericDocReader: public RefCountingObject<GenericDocument>
{
    GenericDocReader(GenericDocumentPtr d) : doc(d) {}
    virtual ~GenericDocReader() {};

    GenericDocumentPtr doc;
    uint32_t token_pos = 0;
    uint32_t line_num = 0;

    bool MoveNext() { token_pos++; return EndOfFile(); }
    uint32_t GetPos() const { return token_pos; }
    bool SeekNextLine();
    int CountLineArgs();
    bool EndOfFile(int offset = 0) const { return token_pos + offset >= doc->tokens.size(); }

    TokenType GetTokType(int offset = 0) const { return !EndOfFile(offset) ? doc->tokens[token_pos + offset].type : TokenType::NONE; }
    const char* GetStringData(int offset = 0) const { return !EndOfFile(offset) ? (doc->string_pool.data() + (uint32_t)doc->tokens[token_pos + offset].data) : nullptr; }
    float GetFloatData(int offset = 0) const { return !EndOfFile(offset) ? doc->tokens[token_pos + offset].data : 0.f; }

    const char* GetTokString(int offset = 0) const { ROR_ASSERT(IsTokString(offset)); return GetStringData(offset); }
    float GetTokFloat(int offset = 0) const { ROR_ASSERT(IsTokFloat(offset)); return GetFloatData(offset); }
    bool GetTokBool(int offset = 0) const { ROR_ASSERT(IsTokBool(offset)); return GetFloatData(offset) == 1.f; }
    const char* GetTokKeyword(int offset = 0) const { ROR_ASSERT(IsTokKeyword(offset)); return GetStringData(offset); }
    const char* GetTokComment(int offset = 0) const { ROR_ASSERT(IsTokComment(offset)); return GetStringData(offset); }

    bool IsTokString(int offset = 0) const { return GetTokType(offset) == TokenType::STRING; };
    bool IsTokFloat(int offset = 0) const { return GetTokType(offset) == TokenType::NUMBER; };
    bool IsTokBool(int offset = 0) const { return GetTokType(offset) == TokenType::BOOL; };
    bool IsTokKeyword(int offset = 0) const { return GetTokType(offset) == TokenType::KEYWORD; };
    bool IsTokComment(int offset = 0) const { return GetTokType(offset) == TokenType::COMMENT; };
};

typedef RefCountingObjectPtr<GenericDocReader> GenericDocReaderPtr;

} // namespace RoR