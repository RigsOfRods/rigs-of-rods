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

/// @file
/// Helper/wrapper functions shared by native (CacheSystemAngelscript.cpp)
/// and generic (CacheSystemAngelscriptGeneric.cpp) CacheSystem bindings.
/// Native bindings register these directly (asCALL_CDECL_OBJFIRST),
/// generic bindings wrap them using WRAP_OBJ_FIRST().

#pragma once

#include "Actor.h"
#include "CacheSystem.h"
#include "ScriptEngine.h"
#include "ScriptUtils.h"

#include <angelscript.h>

namespace CacheSystemAngelscriptWrappers {

using namespace AngelScript;
using namespace RoR;

// class CacheEntry - read-only property access
// (Please maintain the same order as in 'CacheSystem.h' and 'doc/*/CacheEntryClass.h')
static const std::string& CacheEntry_get_fpath(CacheEntry* self) { return self->fpath; }
static const std::string& CacheEntry_get_fname(CacheEntry* self) { return self->fname; }
static const std::string& CacheEntry_get_fext(CacheEntry* self) { return self->fext; }
static const std::string& CacheEntry_get_dname(CacheEntry* self) { return self->dname; }
static int CacheEntry_get_categoryid(CacheEntry* self) { return self->categoryid; }
static const std::string& CacheEntry_get_categoryname(CacheEntry* self) { return self->categoryname; }
static const std::string& CacheEntry_get_resource_bundle_type(CacheEntry* self) { return self->resource_bundle_type; }
static const std::string& CacheEntry_get_resource_bundle_path(CacheEntry* self) { return self->resource_bundle_path; }
static int CacheEntry_get_number(CacheEntry* self) { return self->number; }
static bool CacheEntry_get_deleted(CacheEntry* self) { return self->deleted; }
static const std::string& CacheEntry_get_filecachename(CacheEntry* self) { return self->filecachename; }
static const std::string& CacheEntry_get_resource_group(CacheEntry* self) { return self->resource_group; }

// class CacheSystem
static CScriptDictionary* CacheSystemQueryWrapper(CacheSystem* self, CScriptDictionary* dict)
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

} // namespace CacheSystemAngelscriptWrappers
