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

#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterCacheSystem(asIScriptEngine *engine)
{
    CacheEntry::RegisterRefCountingObject(engine, "CacheEntryClass");
    CacheEntryPtr::RegisterRefCountingObjectPtr(engine, "CacheEntryClassPtr", "CacheEntryClass");
    
    int result;
    
    // enum LoaderType, which is a primary filter for modcache queries
    result = engine->RegisterEnum("LoaderType"); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("LoaderType", "LOADER_TYPE_NONE", LT_None); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("LoaderType", "LOADER_TYPE_TERRAIN", LT_Terrain); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("LoaderType", "LOADER_TYPE_VEHICLE", LT_Vehicle); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("LoaderType", "LOADER_TYPE_TRUCK", LT_Truck); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("LoaderType", "LOADER_TYPE_CAR", LT_Car); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("LoaderType", "LOADER_TYPE_BOAT", LT_Boat); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("LoaderType", "LOADER_TYPE_AIRPLANE", LT_Airplane); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("LoaderType", "LOADER_TYPE_TRAILER", LT_Trailer); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("LoaderType", "LOADER_TYPE_TRAIN", LT_Train); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("LoaderType", "LOADER_TYPE_LOAD", LT_Load); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("LoaderType", "LOADER_TYPE_EXTENSION", LT_Extension); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("LoaderType", "LOADER_TYPE_SKIN", LT_Skin); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("LoaderType", "LOADER_TYPE_ALLBEAM", LT_AllBeam); ROR_ASSERT(result >= 0);

    // class CacheEntry, with read-only property access
    // (Please maintain the same order as in 'CacheSystem.h' and 'doc/*/CacheSystemClass.h')
    
    result = engine->RegisterObjectMethod("CacheEntryClass", "const string& get_fpath() const property", asFUNCTIONPR([](CacheEntry* self) -> const std::string& {return self->fpath;}, (CacheEntry*), const std::string&), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheEntryClass", "const string& get_fname() const property", asFUNCTIONPR([](CacheEntry* self) -> const std::string& {return self->fname;}, (CacheEntry*), const std::string&), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheEntryClass", "const string& get_fext() const property", asFUNCTIONPR([](CacheEntry* self) -> const std::string& {return self->fext;}, (CacheEntry*), const std::string&), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheEntryClass", "const string& get_dname() const property", asFUNCTIONPR([](CacheEntry* self) -> const std::string& {return self->dname;}, (CacheEntry*), const std::string&), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheEntryClass", "int           get_categoryid() const property", asFUNCTIONPR([](CacheEntry* self) -> int {return self->categoryid;}, (CacheEntry*), int), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheEntryClass", "const string& get_categoryname() const property", asFUNCTIONPR([](CacheEntry* self) -> const std::string& {return self->categoryname;}, (CacheEntry*), const std::string&), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheEntryClass", "const string& get_resource_bundle_type() const property", asFUNCTIONPR([](CacheEntry* self) -> const std::string& {return self->resource_bundle_type;}, (CacheEntry*), const std::string&), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheEntryClass", "const string& get_resource_bundle_path() const property", asFUNCTIONPR([](CacheEntry* self) -> const std::string& {return self->resource_bundle_path;}, (CacheEntry*), const std::string&), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheEntryClass", "int           get_number() const property", asFUNCTIONPR([](CacheEntry* self) -> int {return self->number;}, (CacheEntry*), int), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheEntryClass", "bool          get_deleted() const property", asFUNCTIONPR([](CacheEntry* self) -> bool {return self->deleted;}, (CacheEntry*), bool), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheEntryClass", "const string& get_filecachename() const property", asFUNCTIONPR([](CacheEntry* self) -> const std::string& {return self->filecachename;}, (CacheEntry*), const std::string&), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheEntryClass", "const string& get_resource_group() const property", asFUNCTIONPR([](CacheEntry* self) -> const std::string& {return self->resource_group;}, (CacheEntry*), const std::string&), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);

    // class CacheSystem (non-counted reference type)
    result = engine->RegisterObjectType("CacheSystemClass", sizeof(CacheSystem), asOBJ_REF | asOBJ_NOCOUNT); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheSystemClass", "CacheEntryClassPtr @findEntryByFilename(LoaderType, bool, const string &in)", asMETHOD(CacheSystem,FindEntryByFilename), asCALL_THISCALL); ROR_ASSERT(result>=0);

}
