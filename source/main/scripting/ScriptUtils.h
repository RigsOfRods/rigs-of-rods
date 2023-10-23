/*
    This source file is part of Rigs of Rods
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

#pragma once

#ifdef USE_ANGELSCRIPT

#include <angelscript.h>
#include "scriptdictionary/scriptdictionary.h"
#include "scriptarray/scriptarray.h"
#include "scriptbuilder/scriptbuilder.h"

#include <map>
#include <string>
#include <vector>

namespace RoR {

/// @addtogroup Scripting
/// @{

template <typename T>
AngelScript::CScriptArray* VectorToScriptArray(std::vector<T>& vec, const std::string& decl)
{
    std::string arraydecl = fmt::format("array<{}>", decl);
    AngelScript::asITypeInfo* typeinfo = App::GetScriptEngine()->getEngine()->GetTypeInfoByDecl(arraydecl.c_str());
    AngelScript::CScriptArray* arr = AngelScript::CScriptArray::Create(typeinfo, vec.size());

    for (AngelScript::asUINT i = 0; i < arr->GetSize(); i++)
    {
        // Set the value of each element
        arr->SetValue(i, &vec[i]);
    }

    return arr;
}

template <typename T, typename U>
AngelScript::CScriptArray* MapToScriptArray(std::map<T, U>& map, const std::string& decl)
{
    std::string arraydecl = fmt::format("array<{}>", decl);
    AngelScript::asITypeInfo* typeinfo = App::GetScriptEngine()->getEngine()->GetTypeInfoByDecl(arraydecl.c_str());
    AngelScript::CScriptArray* arr = AngelScript::CScriptArray::Create(typeinfo, map.size());

    for (auto itor = map.begin(); itor != map.end(); itor++)
    {
        // Set the value of each element
        AngelScript::asUINT pos = static_cast<AngelScript::asUINT>(std::distance(map.begin(), itor));
        arr->SetValue(pos, &itor->second);
    }

    return arr;
}

template<typename T>
bool GetValueFromScriptDict(const std::string& log_msg, AngelScript::CScriptDictionary* dict, bool required, std::string const& key, const char* decl, T & out_value)
{
    if (!dict)
    {
        // Dict is NULL
        if (required)
        {
            App::GetScriptEngine()->SLOG(fmt::format("{}: ERROR, no parameters; '{}' is required.", log_msg, key));
        }
        return false;
    }
    auto itor = dict->find(key);
    if (itor == dict->end())
    {
        // Key not found
        if (required)
        {
            App::GetScriptEngine()->SLOG(fmt::format("{}: ERROR, required parameter '{}' not found.", log_msg, key));
        }
        return false;
    }

    const int expected_typeid = App::GetScriptEngine()->getEngine()->GetTypeIdByDecl(decl);
    const int actual_typeid = itor.GetTypeId();
    if (actual_typeid != expected_typeid)
    {
        // Wrong type
        if (required)
        {
            App::GetScriptEngine()->SLOG(fmt::format("{}: ERROR, required parameter '{}' must be a {}, instead got {}.",
                log_msg, key, decl, App::GetScriptEngine()->getEngine()->GetTypeDeclaration(actual_typeid)));
        }
        return false;
    }

    return itor.GetValue(&out_value, actual_typeid); // Error will be logged to Angelscript.log
}

/// @}   //addtogroup Scripting

} // namespace RoR

#endif // USE_ANGELSCRIPT
