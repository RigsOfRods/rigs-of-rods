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
static std::string GenericDocReader_getTokString(GenericDocReader* reader, uint32_t offset)
{
    const char* val = reader->getTokString(offset);
    return (val) ? val : "";
}

static std::string GenericDocReader_getTokKeyword(GenericDocReader* reader, uint32_t offset)
{
    const char* val = reader->getTokKeyword(offset);
    return (val) ? val : "";
}

static std::string GenericDocReader_getTokComment(GenericDocReader* reader, uint32_t offset)
{
    const char* val = reader->getTokComment(offset);
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
    engine->RegisterObjectBehaviour("GenericDocumentClass", asBEHAVE_FACTORY, "GenericDocumentClass@+ f()", asFUNCTION(GenericDocumentFactory), asCALL_CDECL);

    engine->RegisterObjectMethod("GenericDocumentClass", "bool loadFromResource(string,string,int)", asMETHOD(GenericDocument, loadFromResource), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocumentClass", "bool saveToResource(string,string)", asMETHOD(GenericDocument, saveToResource), asCALL_THISCALL);


    // class GenericDocReader
    GenericDocReader::RegisterRefCountingObject(engine, "GenericDocReaderClass");
    GenericDocReaderPtr::RegisterRefCountingObjectPtr(engine, "GenericDocReaderClassPtr", "GenericDocReaderClass");
    engine->RegisterObjectBehaviour("GenericDocReaderClass", asBEHAVE_FACTORY, "GenericDocReaderClass@+ f(GenericDocumentClassPtr @)", asFUNCTION(GenericDocReaderFactory), asCALL_CDECL);

    engine->RegisterObjectMethod("GenericDocReaderClass", "bool moveNext()", asMETHOD(GenericDocReader, moveNext), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocReaderClass", "uint getPos()", asMETHOD(GenericDocReader, getPos), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocReaderClass", "bool seekNextLine()", asMETHOD(GenericDocReader, seekNextLine), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocReaderClass", "uint countLineArgs()", asMETHOD(GenericDocReader, countLineArgs), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocReaderClass", "bool endOfFile(int offset = 0)", asMETHOD(GenericDocReader, endOfFile), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocReaderClass", "TokenType tokenType(int offset = 0)", asMETHOD(GenericDocReader, tokenType), asCALL_THISCALL);

    engine->RegisterObjectMethod("GenericDocReaderClass", "string getTokString(int offset = 0)", asFUNCTION(GenericDocReader_getTokString), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("GenericDocReaderClass", "float getTokFloat(int offset = 0)", asMETHOD(GenericDocReader, getTokFloat), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocReaderClass", "bool getTokBool(int offset = 0)", asMETHOD(GenericDocReader, getTokBool), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocReaderClass", "string getTokKeyword(int offset = 0)", asFUNCTION(GenericDocReader_getTokKeyword), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("GenericDocReaderClass", "string getTokComment(int offset = 0)", asFUNCTION(GenericDocReader_getTokComment), asCALL_CDECL_OBJFIRST);
    
    engine->RegisterObjectMethod("GenericDocReaderClass", "bool isTokString(int offset = 0)", asMETHOD(GenericDocReader, isTokString), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocReaderClass", "bool isTokFloat(int offset = 0)", asMETHOD(GenericDocReader, isTokFloat), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocReaderClass", "bool isTokBool(int offset = 0)", asMETHOD(GenericDocReader, isTokBool), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocReaderClass", "bool isTokKeyword(int offset = 0)", asMETHOD(GenericDocReader, isTokKeyword), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocReaderClass", "bool isTokComment(int offset = 0)", asMETHOD(GenericDocReader, isTokComment), asCALL_THISCALL);
}
