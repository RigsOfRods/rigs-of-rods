/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2022 Petr Ohlidal

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

#include "AngelScriptBindings.h"
#include "GenericFileFormat.h"

#include <angelscript.h>

using namespace RoR;
using namespace AngelScript;

void RoR::RegisterGenericFileFormatCommon(asIScriptEngine* engine)
{
    // enum TokenType
    engine->RegisterEnum("TokenType");
    engine->RegisterEnumValue("TokenType", "TOKEN_TYPE_NONE", (int)TokenType::NONE);
    engine->RegisterEnumValue("TokenType", "TOKEN_TYPE_LINEBREAK", (int)TokenType::LINEBREAK);
    engine->RegisterEnumValue("TokenType", "TOKEN_TYPE_COMMENT", (int)TokenType::COMMENT);
    engine->RegisterEnumValue("TokenType", "TOKEN_TYPE_STRING", (int)TokenType::STRING);
    engine->RegisterEnumValue("TokenType", "TOKEN_TYPE_FLOAT", (int)TokenType::FLOAT);
    engine->RegisterEnumValue("TokenType", "TOKEN_TYPE_INT", (int)TokenType::INT);
    engine->RegisterEnumValue("TokenType", "TOKEN_TYPE_BOOL", (int)TokenType::BOOL);
    engine->RegisterEnumValue("TokenType", "TOKEN_TYPE_KEYWORD", (int)TokenType::KEYWORD);


    // GenericDocument constants
    engine->RegisterEnum("GenericDocumentOptions");
    engine->RegisterEnumValue("GenericDocumentOptions", "GENERIC_DOCUMENT_OPTION_ALLOW_NAKED_STRINGS", GenericDocument::OPTION_ALLOW_NAKED_STRINGS);
    engine->RegisterEnumValue("GenericDocumentOptions", "GENERIC_DOCUMENT_OPTION_ALLOW_SLASH_COMMENTS", GenericDocument::OPTION_ALLOW_SLASH_COMMENTS);
    engine->RegisterEnumValue("GenericDocumentOptions", "GENERIC_DOCUMENT_OPTION_FIRST_LINE_IS_TITLE", GenericDocument::OPTION_FIRST_LINE_IS_TITLE);
    engine->RegisterEnumValue("GenericDocumentOptions", "GENERIC_DOCUMENT_OPTION_ALLOW_SEPARATOR_COLON", GenericDocument::OPTION_ALLOW_SEPARATOR_COLON);
    engine->RegisterEnumValue("GenericDocumentOptions", "GENERIC_DOCUMENT_OPTION_PARENTHESES_CAPTURE_SPACES", GenericDocument::OPTION_PARENTHESES_CAPTURE_SPACES);
    engine->RegisterEnumValue("GenericDocumentOptions", "GENERIC_DOCUMENT_OPTION_ALLOW_BRACED_KEYWORDS", GenericDocument::OPTION_ALLOW_BRACED_KEYWORDS);
    engine->RegisterEnumValue("GenericDocumentOptions", "GENERIC_DOCUMENT_OPTION_ALLOW_SEPARATOR_EQUALS", GenericDocument::OPTION_ALLOW_SEPARATOR_EQUALS);
    engine->RegisterEnumValue("GenericDocumentOptions", "GENERIC_DOCUMENT_OPTION_ALLOW_HASH_COMMENTS", GenericDocument::OPTION_ALLOW_HASH_COMMENTS);
}
