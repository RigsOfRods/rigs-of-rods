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

#pragma once

#include "RefCountingObject.h"
#include "RefCountingObjectPtr.h"
#include "BitFlags.h"
#include "Application.h"

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
    FLOAT,
    INT,
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
    static const BitMask_t OPTION_ALLOW_BRACED_KEYWORDS = BITMASK(6); //!< Allow INI-like '[keyword]' tokens.
    static const BitMask_t OPTION_ALLOW_SEPARATOR_EQUALS = BITMASK(7); //!< Allow '=' as separator between tokens.
    static const BitMask_t OPTION_ALLOW_HASH_COMMENTS = BITMASK(8); //!< Allow comments starting with `#`. 

    virtual ~GenericDocument() {};

    std::vector<char> string_pool; // Data of COMMENT/KEYWORD/STRING tokens; NUL-terminated strings.
    std::vector<Token> tokens;
    
    virtual void loadFromDataStream(Ogre::DataStreamPtr datastream, BitMask_t options = 0);
    virtual void saveToDataStream(Ogre::DataStreamPtr datastream);

    virtual bool loadFromResource(std::string resource_name, std::string resource_group_name, BitMask_t options = 0);
    virtual bool saveToResource(std::string resource_name, std::string resource_group_name);
};

typedef RefCountingObjectPtr<GenericDocument> GenericDocumentPtr;

struct GenericDocContext: public RefCountingObject<GenericDocContext>
{
    GenericDocContext(GenericDocumentPtr d) : doc(d)
    {
        ROR_ASSERT(doc != nullptr);
        if (doc == nullptr && AngelScript::asGetActiveContext() != nullptr)
        {
            AngelScript::asGetActiveContext()->SetException("Cannot create GenericDocContextClass from null GenericDocument!");
        }
    }
    virtual ~GenericDocContext() {};

    GenericDocumentPtr doc;
    uint32_t token_pos = 0;

    // PLEASE maintain the same order as in 'bindings/GenericFileFormatAngelscript.cpp' and 'doc/*/GenericDocContextClass.h'

    bool moveNext() { token_pos++; return endOfFile(); }
    uint32_t getPos() const { return token_pos; }
    bool seekNextLine();
    int countLineArgs();
    bool endOfFile(int offset = 0) const { return token_pos + offset >= doc->tokens.size(); }
    TokenType tokenType(int offset = 0) const { return !endOfFile(offset) ? doc->tokens[token_pos + offset].type : TokenType::NONE; }

    std::string getTokString(int offset = 0) const { ROR_ASSERT(isTokString(offset)); return getStringData(offset); }
    float getTokFloat(int offset = 0) const { ROR_ASSERT(isTokFloat(offset)); return getFloatData(offset); }
    int getTokInt(int offset = 0) const { ROR_ASSERT(isTokInt(offset)); return (int)getFloatData(offset); }
    float getTokNumeric(int offset = 0) const { ROR_ASSERT(isTokNumeric(offset)); return getFloatData(offset); }
    bool getTokBool(int offset = 0) const { ROR_ASSERT(isTokBool(offset)); return getFloatData(offset) == 1.f; }
    std::string getTokKeyword(int offset = 0) const { ROR_ASSERT(isTokKeyword(offset)); return getStringData(offset); }
    std::string getTokComment(int offset = 0) const { ROR_ASSERT(isTokComment(offset)); return getStringData(offset); }

    bool isTokString(int offset = 0) const { return tokenType(offset) == TokenType::STRING; }
    bool isTokFloat(int offset = 0) const { return tokenType(offset) == TokenType::FLOAT; }
    bool isTokInt(int offset = 0) const { return tokenType(offset) == TokenType::INT; }
    bool isTokBool(int offset = 0) const { return tokenType(offset) == TokenType::BOOL; }
    bool isTokKeyword(int offset = 0) const { return tokenType(offset) == TokenType::KEYWORD; }
    bool isTokComment(int offset = 0) const { return tokenType(offset) == TokenType::COMMENT; }
    bool isTokLineBreak(int offset = 0) const { return tokenType(offset) == TokenType::LINEBREAK; }
    bool isTokNumeric(int offset = 0) const { return isTokInt(offset) || isTokFloat(offset); }

    // Editing functions:

    bool insertToken(int offset = 0); //!< Inserts `TokenType::NONE`; @return false if offset is beyond EOF
    bool eraseToken(int offset = 0); //!< @return false if offset is beyond EOF

    bool setTokString(int offset, const std::string& str) { return setStringData(offset, TokenType::STRING, str); }
    bool setTokFloat(int offset, float val) { return setFloatData(offset, TokenType::FLOAT, val); }
    bool setTokInt(int offset, int val) { return setFloatData(offset, TokenType::INT, val); }
    bool setTokBool(int offset, bool val) { return setFloatData(offset, TokenType::BOOL, val); }
    bool setTokKeyword(int offset, const std::string& str) { return setStringData(offset, TokenType::KEYWORD, str); }
    bool setTokComment(int offset, const std::string& str) { return setStringData(offset, TokenType::COMMENT, str); }
    bool setTokLineBreak(int offset) { return setFloatData(offset, TokenType::LINEBREAK, 0.f); }
        
    // Not exported to script:

    const char* getStringData(int offset = 0) const { return !endOfFile(offset) ? (doc->string_pool.data() + (uint32_t)doc->tokens[token_pos + offset].data) : ""; }
    float getFloatData(int offset = 0) const { return !endOfFile(offset) ? doc->tokens[token_pos + offset].data : 0.f; }
    bool setStringData(int offset, TokenType type, const std::string& data); //!< @return false if offset is beyond EOF
    bool setFloatData(int offset, TokenType type, float data); //!< @return false if offset is beyond EOF
};

typedef RefCountingObjectPtr<GenericDocContext> GenericDocContextPtr;

} // namespace RoR