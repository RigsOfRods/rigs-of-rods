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
/// @author Petr Ohlidal
/// @date   12-2022

#include "AngelScriptBindings.h"
#include "GenericFileFormat.h"

using namespace RoR;
using namespace AngelScript;

// Wrappers
static std::string GenericDocReader_GetTokString(GenericDocReader* reader, uint32_t offset)
{
    const char* val = reader->GetTokString(offset);
    return (val) ? val : "";
}

static std::string GenericDocReader_GetTokKeyword(GenericDocReader* reader, uint32_t offset)
{
    const char* val = reader->GetTokKeyword(offset);
    return (val) ? val : "";
}

static std::string GenericDocReader_GetTokComment(GenericDocReader* reader, uint32_t offset)
{
    const char* val = reader->GetTokComment(offset);
    return (val) ? val : "";
}

// Factories
static GenericDocument* GenericDocumentFactory()
{
    return new GenericDocument();
}

static GenericDocReader* GenericDocReaderFactory(GenericDocumentPtr doc)
{
    return new GenericDocReader(doc);
}

void RoR::RegisterGenericFileFormat(asIScriptEngine* engine)
{
    // enum TokenType
    engine->RegisterEnum("TokenType");
    engine->RegisterEnumValue("TokenType", "TOKEN_TYPE_NONE", (int)TokenType::NONE);
    engine->RegisterEnumValue("TokenType", "TOKEN_TYPE_LINEBREAK", (int)TokenType::LINEBREAK);
    engine->RegisterEnumValue("TokenType", "TOKEN_TYPE_COMMENT", (int)TokenType::COMMENT);
    engine->RegisterEnumValue("TokenType", "TOKEN_TYPE_STRING", (int)TokenType::STRING);
    engine->RegisterEnumValue("TokenType", "TOKEN_TYPE_NUMBER", (int)TokenType::NUMBER);
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


    // class GenericDocument
    GenericDocument::RegisterRefCountingObject(engine, "GenericDocumentClass");
    GenericDocumentPtr::RegisterRefCountingObjectPtr(engine, "GenericDocumentClassPtr", "GenericDocumentClass");
    engine->RegisterObjectBehaviour("GenericDocumentClass", asBEHAVE_FACTORY, "GenericDocumentClass@ f()", asFUNCTION(GenericDocumentFactory), asCALL_CDECL);

    engine->RegisterObjectMethod("GenericDocumentClass", "bool LoadFromResource(string,string,int)", asMETHOD(GenericDocument, LoadFromResource), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocumentClass", "bool SaveToResource(string,string)", asMETHOD(GenericDocument, SaveToResource), asCALL_THISCALL);


    // class GenericDocReader
    GenericDocReader::RegisterRefCountingObject(engine, "GenericDocReaderClass");
    GenericDocReaderPtr::RegisterRefCountingObjectPtr(engine, "GenericDocReaderClassPtr", "GenericDocReaderClass");
    engine->RegisterObjectBehaviour("GenericDocReaderClass", asBEHAVE_FACTORY, "GenericDocReaderClass@ f(GenericDocumentClassPtr @)", asFUNCTION(GenericDocReaderFactory), asCALL_CDECL);

    engine->RegisterObjectMethod("GenericDocReaderClass", "bool MoveNext()", asMETHOD(GenericDocReader, MoveNext), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocReaderClass", "uint GetPos()", asMETHOD(GenericDocReader, GetPos), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocReaderClass", "bool SeekNextLine()", asMETHOD(GenericDocReader, SeekNextLine), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocReaderClass", "uint CountLineArgs()", asMETHOD(GenericDocReader, CountLineArgs), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocReaderClass", "bool EndOfFile(int offset = 0)", asMETHOD(GenericDocReader, EndOfFile), asCALL_THISCALL);

    engine->RegisterObjectMethod("GenericDocReaderClass", "TokenType GetTokType(int offset = 0)", asMETHOD(GenericDocReader, GetTokType), asCALL_THISCALL);

    engine->RegisterObjectMethod("GenericDocReaderClass", "string GetTokString(int offset = 0)", asFUNCTION(GenericDocReader_GetTokString), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("GenericDocReaderClass", "float GetTokFloat(int offset = 0)", asMETHOD(GenericDocReader, GetTokFloat), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocReaderClass", "bool GetTokBool(int offset = 0)", asMETHOD(GenericDocReader, GetTokBool), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocReaderClass", "string GetTokKeyword(int offset = 0)", asFUNCTION(GenericDocReader_GetTokKeyword), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("GenericDocReaderClass", "string GetTokComment(int offset = 0)", asFUNCTION(GenericDocReader_GetTokComment), asCALL_CDECL_OBJFIRST);
    
    engine->RegisterObjectMethod("GenericDocReaderClass", "bool IsTokString(int offset = 0)", asMETHOD(GenericDocReader, IsTokString), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocReaderClass", "bool IsTokFloat(int offset = 0)", asMETHOD(GenericDocReader, IsTokFloat), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocReaderClass", "bool IsTokBool(int offset = 0)", asMETHOD(GenericDocReader, IsTokBool), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocReaderClass", "bool IsTokKeyword(int offset = 0)", asMETHOD(GenericDocReader, IsTokKeyword), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocReaderClass", "bool IsTokComment(int offset = 0)", asMETHOD(GenericDocReader, IsTokComment), asCALL_THISCALL);
}
