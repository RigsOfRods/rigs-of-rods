/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2023 Petr Ohlidal

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

#include "Actor.h"
#include "AngelScriptBindings.h"
#include "CacheSystem.h"
#include "ScriptEngine.h"
#include "ScriptUtils.h"
#include "wrappers/CacheSystemAngelscriptWrappers.h"

#include <angelscript.h>

using namespace AngelScript;
using namespace RoR;
using namespace CacheSystemAngelscriptWrappers;

void RoR::RegisterCacheSystemNative(asIScriptEngine *engine)
{
    CacheEntry::RegisterRefCountingObject(engine, "CacheEntryClass");
    CacheEntryPtr::RegisterRefCountingObjectPtr(engine, "CacheEntryClassPtr", "CacheEntryClass");
    
    int result;

    // NOTE: enum LoaderType and object type CacheSystemClass are registered in RegisterCacheSystemCommon()

    // class CacheEntry, with read-only property access
    // (Please maintain the same order as in 'CacheSystem.h' and 'doc/*/CacheEntryClass.h')
    
    result = engine->RegisterObjectMethod("CacheEntryClass", "const string& get_fpath() const property", asFUNCTION(CacheEntry_get_fpath), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheEntryClass", "const string& get_fname() const property", asFUNCTION(CacheEntry_get_fname), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheEntryClass", "const string& get_fext() const property", asFUNCTION(CacheEntry_get_fext), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheEntryClass", "const string& get_dname() const property", asFUNCTION(CacheEntry_get_dname), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheEntryClass", "int           get_categoryid() const property", asFUNCTION(CacheEntry_get_categoryid), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheEntryClass", "const string& get_categoryname() const property", asFUNCTION(CacheEntry_get_categoryname), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheEntryClass", "const string& get_resource_bundle_type() const property", asFUNCTION(CacheEntry_get_resource_bundle_type), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheEntryClass", "const string& get_resource_bundle_path() const property", asFUNCTION(CacheEntry_get_resource_bundle_path), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheEntryClass", "int           get_number() const property", asFUNCTION(CacheEntry_get_number), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheEntryClass", "bool          get_deleted() const property", asFUNCTION(CacheEntry_get_deleted), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheEntryClass", "const string& get_filecachename() const property", asFUNCTION(CacheEntry_get_filecachename), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheEntryClass", "const string& get_resource_group() const property", asFUNCTION(CacheEntry_get_resource_group), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);

    // class CacheSystem
    result = engine->RegisterObjectMethod("CacheSystemClass", "CacheEntryClassPtr @findEntryByFilename(LoaderType, bool, const string &in)", asMETHOD(CacheSystem,FindEntryByFilename), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheSystemClass", "CacheEntryClassPtr @getEntryByNumber(int)", asMETHOD(CacheSystem,GetEntryByNumber), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheSystemClass", "dictionary@ query(dictionary@+)", asFUNCTION(CacheSystemQueryWrapper), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);

}
