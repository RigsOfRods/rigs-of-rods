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

// Factories
static GenericDocument* GenericDocumentFactory()
{
    return new GenericDocument();
}

static GenericDocContext* GenericDocContextFactory(GenericDocumentPtr doc)
{
    return new GenericDocContext(doc);
}

void RoR::RegisterGenericFileFormat(asIScriptEngine* engine)
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


    // class GenericDocument
    GenericDocument::RegisterRefCountingObject(engine, "GenericDocumentClass");
    GenericDocumentPtr::RegisterRefCountingObjectPtr(engine, "GenericDocumentClassPtr", "GenericDocumentClass");
    engine->RegisterObjectBehaviour("GenericDocumentClass", asBEHAVE_FACTORY, "GenericDocumentClass@+ f()", asFUNCTION(GenericDocumentFactory), asCALL_CDECL);

    engine->RegisterObjectMethod("GenericDocumentClass", "bool loadFromResource(string,string,int)", asMETHOD(GenericDocument, loadFromResource), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocumentClass", "bool saveToResource(string,string)", asMETHOD(GenericDocument, saveToResource), asCALL_THISCALL);


    // class GenericDocContext
    // (Please maintain the same order as in 'GenericFileFormat.h' and 'doc/*/GenericDocContextClass.h')
    GenericDocContext::RegisterRefCountingObject(engine, "GenericDocContextClass");
    GenericDocContextPtr::RegisterRefCountingObjectPtr(engine, "GenericDocContextClassPtr", "GenericDocContextClass");
    engine->RegisterObjectBehaviour("GenericDocContextClass", asBEHAVE_FACTORY, "GenericDocContextClass@+ f(GenericDocumentClassPtr @)", asFUNCTION(GenericDocContextFactory), asCALL_CDECL);

    engine->RegisterObjectMethod("GenericDocContextClass", "bool moveNext()", asMETHOD(GenericDocContext, moveNext), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocContextClass", "uint getPos()", asMETHOD(GenericDocContext, getPos), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool seekNextLine()", asMETHOD(GenericDocContext, seekNextLine), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocContextClass", "uint countLineArgs()", asMETHOD(GenericDocContext, countLineArgs), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool endOfFile(int offset = 0)", asMETHOD(GenericDocContext, endOfFile), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocContextClass", "TokenType tokenType(int offset = 0)", asMETHOD(GenericDocContext, tokenType), asCALL_THISCALL);

    engine->RegisterObjectMethod("GenericDocContextClass", "string getTokString(int offset = 0)", asMETHOD(GenericDocContext, getTokString), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocContextClass", "float getTokFloat(int offset = 0)", asMETHOD(GenericDocContext, getTokFloat), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocContextClass", "int getTokInt(int offset = 0)", asMETHOD(GenericDocContext, getTokInt), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool getTokBool(int offset = 0)", asMETHOD(GenericDocContext, getTokBool), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocContextClass", "string getTokKeyword(int offset = 0)", asMETHOD(GenericDocContext, getTokKeyword), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocContextClass", "string getTokComment(int offset = 0)", asMETHOD(GenericDocContext, getTokComment), asCALL_THISCALL);
    
    engine->RegisterObjectMethod("GenericDocContextClass", "bool isTokString(int offset = 0)", asMETHOD(GenericDocContext, isTokString), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool isTokFloat(int offset = 0)", asMETHOD(GenericDocContext, isTokFloat), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool isTokInt(int offset = 0)", asMETHOD(GenericDocContext, isTokInt), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool isTokBool(int offset = 0)", asMETHOD(GenericDocContext, isTokBool), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool isTokKeyword(int offset = 0)", asMETHOD(GenericDocContext, isTokKeyword), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool isTokComment(int offset = 0)", asMETHOD(GenericDocContext, isTokComment), asCALL_THISCALL);

    // > Editing functions:
    engine->RegisterObjectMethod("GenericDocContextClass", "bool insertToken(int offset = 0)", asMETHOD(GenericDocContext, insertToken), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool eraseToken(int offset = 0)", asMETHOD(GenericDocContext, eraseToken), asCALL_THISCALL);

    engine->RegisterObjectMethod("GenericDocContextClass", "bool setTokString(int offset, const string &in)", asMETHOD(GenericDocContext, setTokString), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool setTokFloat(int offset, float)", asMETHOD(GenericDocContext, setTokFloat), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool setTokInt(int offset, int)", asMETHOD(GenericDocContext, setTokInt), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool setTokBool(int offset, bool)", asMETHOD(GenericDocContext, setTokBool), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool setTokKeyword(int offset, const string &in)", asMETHOD(GenericDocContext, setTokKeyword), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool setTokComment(int offset, const string &in)", asMETHOD(GenericDocContext, setTokComment), asCALL_THISCALL);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool setTokLineBreak(int offset)", asMETHOD(GenericDocContext, setTokBool), asCALL_THISCALL);

}
