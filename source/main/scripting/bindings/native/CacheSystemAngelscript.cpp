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

#include <angelscript.h>

using namespace AngelScript;
using namespace RoR;

CScriptDictionary* CacheSystemQueryWrapper(CacheSystem* self, CScriptDictionary* dict)
{
    CacheQuery query;
    std::string log_msg = "modcache.query(): ";
    std::string search_expr;
    if (!GetValueFromScriptDict(log_msg, dict, /*required:*/true, "filter_type", "LoaderType", query.cqy_filter_type))
    {
        return nullptr;
    }
    int64_t i64_filter_category_id; // AngelScript's `Dictionary` converts all ints int `int64`
    GetValueFromScriptDict(log_msg, dict, /*required:*/false, "filter_category_id", "int64", i64_filter_category_id);
    query.cqy_filter_category_id = i64_filter_category_id;
    GetValueFromScriptDict(log_msg, dict, /*required:*/false, "filter_guid", "string", query.cqy_filter_guid);
    GetValueFromScriptDict(log_msg, dict, /*required:*/false, "search_expr", "string", search_expr);

    // FIXME: Copypasta of `GUI::MainSelector::UpdateSearchParams()`
    if (search_expr.find(":") == std::string::npos)
    {
        query.cqy_search_method = CacheSearchMethod::FULLTEXT;
        query.cqy_search_string = search_expr;
    }
    else
    {
        Ogre::StringVector v = Ogre::StringUtil::split(search_expr, ":");
        if (v.size() < 2)
        {
            query.cqy_search_method = CacheSearchMethod::NONE;
            query.cqy_search_string = "";
        }
        else if (v[0] == "guid")
        {
            query.cqy_search_method = CacheSearchMethod::GUID;
            query.cqy_search_string = v[1];
        }
        else if (v[0] == "author")
        {
            query.cqy_search_method = CacheSearchMethod::AUTHORS;
            query.cqy_search_string = v[1];
        }
        else if (v[0] == "wheels")
        {
            query.cqy_search_method = CacheSearchMethod::WHEELS;
            query.cqy_search_string = v[1];
        }
        else if (v[0] == "file")
        {
            query.cqy_search_method = CacheSearchMethod::FILENAME;
            query.cqy_search_string = v[1];
        }
        else
        {
            query.cqy_search_method = CacheSearchMethod::NONE;
            query.cqy_search_string = "";
        }
    }
    // END copypasta

    size_t results_count = self->Query(query);

    asITypeInfo* typeinfo_array_entries = App::GetScriptEngine()->getEngine()->GetTypeInfoByDecl("array<CacheEntryClass@>");
    asITypeInfo* typeinfo_array_scores = App::GetScriptEngine()->getEngine()->GetTypeInfoByDecl("array<uint>");

    CScriptArray* results_entries = CScriptArray::Create(typeinfo_array_entries);
    CScriptArray* results_scores = CScriptArray::Create(typeinfo_array_scores);
    for (CacheQueryResult& result: query.cqy_results)
    {
        CacheEntry* entry_ptr = result.cqr_entry.GetRef();
        results_entries->InsertLast(&entry_ptr);
        results_scores->InsertLast(&result.cqr_score);
    }
    
    CScriptDictionary* results_dict = CScriptDictionary::Create(App::GetScriptEngine()->getEngine());
    results_dict->Set("count", (asINT64)results_count);
    results_dict->Set("entries", results_entries, typeinfo_array_entries->GetTypeId());
    results_dict->Set("scores", results_scores, typeinfo_array_scores->GetTypeId());

    return results_dict;
}

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
    result = engine->RegisterEnumValue("LoaderType", "LOADER_TYPE_ADDONPART", LT_AddonPart); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("LoaderType", "LOADER_TYPE_TUNEUP", LT_Tuneup); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("LoaderType", "LOADER_TYPE_ASSETPACK", LT_AssetPack); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("LoaderType", "LOADER_TYPE_DASHBOARD", LT_DashBoard); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("LoaderType", "LOADER_TYPE_GADGET", LT_Gadget); ROR_ASSERT(result >= 0);

    // class CacheEntry, with read-only property access
    // (Please maintain the same order as in 'CacheSystem.h' and 'doc/*/CacheEntryClass.h')
    
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
    result = engine->RegisterObjectMethod("CacheSystemClass", "CacheEntryClassPtr @getEntryByNumber(int)", asMETHOD(CacheSystem,GetEntryByNumber), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CacheSystemClass", "dictionary@ query(dictionary@)", asFUNCTION(CacheSystemQueryWrapper), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);

}
