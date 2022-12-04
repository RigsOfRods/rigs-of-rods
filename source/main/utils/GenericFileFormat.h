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

struct Document
{
    static const BitMask_t OPTION_ALLOW_NAKED_STRINGS = BITMASK(1); //!< Allow strings without quotes, for backwards compatibility.
    static const BitMask_t OPTION_ALLOW_SLASH_COMMENTS = BITMASK(2); //!< Allow comments starting with `//`. 

    virtual ~Document() {};

    std::vector<char> string_pool; // Data of COMMENT/KEYWORD/STRING tokens; NUL-terminated strings.
    std::vector<Token> tokens;
    
    virtual void Load(Ogre::DataStreamPtr datastream, BitMask_t options = 0);
    virtual void Save(Ogre::DataStreamPtr datastream);
};

struct Reader
{
    Reader(Document& d) : doc(d) {}
    virtual ~Reader() {};

    Document& doc;
    size_t token_pos = 0;
    size_t line_num = 0;

    bool MoveNext() { token_pos++; return EndOfFile(); }
    bool SeekNextLine();
    size_t CountLineArgs();
    bool EndOfFile(size_t offset = 0) const { return token_pos + offset >= doc.tokens.size(); }

    TokenType GetType(size_t offset = 0) const { return !EndOfFile(offset) ? doc.tokens[token_pos + offset].type : TokenType::NONE; }
    const char* GetStringData(size_t offset = 0) const { return !EndOfFile(offset) ? (doc.string_pool.data() + (size_t)doc.tokens[token_pos + offset].data) : nullptr; }
    float GetFloatData(size_t offset = 0) const { return !EndOfFile(offset) ? doc.tokens[token_pos + offset].data : 0.f; }

    const char* GetArgString(size_t offset = 0) const { ROR_ASSERT(IsArgString(offset)); return GetStringData(offset); }
    float GetArgFloat(size_t offset = 0) const { ROR_ASSERT(IsArgFloat(offset)); return GetFloatData(offset); }
    bool GetArgBool(size_t offset = 0) const { ROR_ASSERT(IsArgBool(offset)); return GetFloatData(offset) == 1.f; }
    const char* GetArgKeyword(size_t offset = 0) const { ROR_ASSERT(IsArgKeyword(offset)); return GetStringData(offset); }

    bool IsArgString(size_t offset = 0) const { return GetType(offset) == TokenType::STRING; };
    bool IsArgFloat(size_t offset = 0) const { return GetType(offset) == TokenType::NUMBER; };
    bool IsArgBool(size_t offset = 0) const { return GetType(offset) == TokenType::BOOL; };
    bool IsArgKeyword(size_t offset = 0) const { return GetType(offset) == TokenType::KEYWORD; };

};

} // namespace RoR