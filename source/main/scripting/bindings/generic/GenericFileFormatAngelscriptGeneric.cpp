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

#include "../../autowrapper/aswrappedcall.h"
#include "AngelScriptBindings.h"
#include "GenericFileFormat.h"
#include "wrappers/GenericFileFormatAngelscriptWrappers.h"

using namespace RoR;
using namespace AngelScript;
using namespace GenericFileFormatAngelscriptWrappers;

void RoR::RegisterGenericFileFormatGeneric(asIScriptEngine* engine)
{
    // NOTE: enums TokenType and GenericDocumentOptions are registered in RegisterGenericFileFormatCommon()

    // class GenericDocument
    GenericDocument::RegisterRefCountingObjectGeneric(engine, "GenericDocumentClass");
    GenericDocumentPtr::RegisterRefCountingObjectPtrGeneric(engine, "GenericDocumentClassPtr", "GenericDocumentClass");
    engine->RegisterObjectBehaviour("GenericDocumentClass", asBEHAVE_FACTORY, "GenericDocumentClass@+ f()", WRAP_FN(GenericDocumentFactory), asCALL_GENERIC);

    engine->RegisterObjectMethod("GenericDocumentClass", "bool loadFromResource(string,string,int)", WRAP_MFN(GenericDocument, loadFromResource), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocumentClass", "bool saveToResource(string,string)", WRAP_MFN(GenericDocument, saveToResource), asCALL_GENERIC);


    // class GenericDocContext
    // (Please maintain the same order as in 'GenericFileFormat.h' and 'doc/*/GenericDocContextClass.h')
    GenericDocContext::RegisterRefCountingObjectGeneric(engine, "GenericDocContextClass");
    GenericDocContextPtr::RegisterRefCountingObjectPtrGeneric(engine, "GenericDocContextClassPtr", "GenericDocContextClass");
    engine->RegisterObjectBehaviour("GenericDocContextClass", asBEHAVE_FACTORY, "GenericDocContextClass@+ f(GenericDocumentClassPtr @)", WRAP_FN(GenericDocContextFactory), asCALL_GENERIC);

    engine->RegisterObjectMethod("GenericDocContextClass", "bool moveNext()", WRAP_MFN(GenericDocContext, moveNext), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "uint getPos()", WRAP_MFN(GenericDocContext, getPos), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool seekNextLine()", WRAP_MFN(GenericDocContext, seekNextLine), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "uint countLineArgs()", WRAP_MFN(GenericDocContext, countLineArgs), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool endOfFile(int offset = 0)", WRAP_MFN(GenericDocContext, endOfFile), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "TokenType tokenType(int offset = 0)", WRAP_MFN(GenericDocContext, tokenType), asCALL_GENERIC);

    engine->RegisterObjectMethod("GenericDocContextClass", "string getTokString(int offset = 0)", WRAP_MFN(GenericDocContext, getTokString), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "float getTokFloat(int offset = 0)", WRAP_MFN(GenericDocContext, getTokFloat), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "int getTokInt(int offset = 0)", WRAP_MFN(GenericDocContext, getTokInt), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool getTokBool(int offset = 0)", WRAP_MFN(GenericDocContext, getTokBool), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "string getTokKeyword(int offset = 0)", WRAP_MFN(GenericDocContext, getTokKeyword), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "string getTokComment(int offset = 0)", WRAP_MFN(GenericDocContext, getTokComment), asCALL_GENERIC);
    
    engine->RegisterObjectMethod("GenericDocContextClass", "bool isTokString(int offset = 0)", WRAP_MFN(GenericDocContext, isTokString), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool isTokFloat(int offset = 0)", WRAP_MFN(GenericDocContext, isTokFloat), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool isTokInt(int offset = 0)", WRAP_MFN(GenericDocContext, isTokInt), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool isTokBool(int offset = 0)", WRAP_MFN(GenericDocContext, isTokBool), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool isTokKeyword(int offset = 0)", WRAP_MFN(GenericDocContext, isTokKeyword), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool isTokComment(int offset = 0)", WRAP_MFN(GenericDocContext, isTokComment), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool isTokLineBreak(int offset = 0)", WRAP_MFN(GenericDocContext, isTokLineBreak), asCALL_GENERIC);

    // > Editing functions:
    engine->RegisterObjectMethod("GenericDocContextClass", "void appendTokens(int count)", WRAP_MFN(GenericDocContext, appendTokens), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool insertToken(int offset = 0)", WRAP_MFN(GenericDocContext, insertToken), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool eraseToken(int offset = 0)", WRAP_MFN(GenericDocContext, eraseToken), asCALL_GENERIC);

    engine->RegisterObjectMethod("GenericDocContextClass", "void appendTokString(const string &in)", WRAP_MFN(GenericDocContext, appendTokString), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "void appendTokFloat(float)", WRAP_MFN(GenericDocContext, appendTokFloat), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "void appendTokInt(int)", WRAP_MFN(GenericDocContext, appendTokInt), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "void appendTokBool(bool)", WRAP_MFN(GenericDocContext, appendTokBool), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "void appendTokKeyword(const string &in)", WRAP_MFN(GenericDocContext, appendTokKeyword), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "void appendTokComment(const string &in)", WRAP_MFN(GenericDocContext, appendTokComment), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "void appendTokLineBreak()", WRAP_MFN(GenericDocContext, appendTokLineBreak), asCALL_GENERIC);

    engine->RegisterObjectMethod("GenericDocContextClass", "bool setTokString(int offset, const string &in)", WRAP_MFN(GenericDocContext, setTokString), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool setTokFloat(int offset, float)", WRAP_MFN(GenericDocContext, setTokFloat), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool setTokInt(int offset, int)", WRAP_MFN(GenericDocContext, setTokInt), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool setTokBool(int offset, bool)", WRAP_MFN(GenericDocContext, setTokBool), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool setTokKeyword(int offset, const string &in)", WRAP_MFN(GenericDocContext, setTokKeyword), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool setTokComment(int offset, const string &in)", WRAP_MFN(GenericDocContext, setTokComment), asCALL_GENERIC);
    engine->RegisterObjectMethod("GenericDocContextClass", "bool setTokLineBreak(int offset)", WRAP_MFN(GenericDocContext, setTokLineBreak), asCALL_GENERIC);

}
