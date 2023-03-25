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

// Proof of concept: a read-only Dictionary view of a `std::map`-like container. 
//    Assumed to always use `std::string` for key.
//    Not using RefCountingObject because this is only for use in the script, not C++
//    Adopted from 'scriptdictionary.cpp' bundled with AngelScript SDK
template<typename T>
class CReadonlyScriptDictView
{
public:

    CReadonlyScriptDictView(const std::map<std::string, T>& map) : m_map(map), m_refcount(1) {}

    bool Exists(const std::string& key) const
    {
        return (m_map.find(key) != m_map.end());
    }

    bool IsEmpty() const
    {
        if (m_map.size() == 0)
            return true;

        return false;
    }

    AngelScript::asUINT GetSize() const
    {
        return AngelScript::asUINT(m_map.size());
    }

    T OpIndex(const std::string& key) const
    {
        auto it = m_map.find(key);
        if (it != m_map.end())
            return it->second;
        else
            return T{};
    }

    AngelScript::CScriptArray* GetKeys() const
    {
        // Create the array object
        AngelScript::CScriptArray* array = AngelScript::CScriptArray::Create(AngelScript::asGetActiveContext()->GetEngine()->GetTypeInfoByDecl("array<string>"), AngelScript::asUINT(m_map.size()));
        long current = -1;
        for (auto it = m_map.begin(); it != m_map.end(); it++)
        {
            current++;
            *(std::string*)array->At(current) = it->first;
        }

        return array;
    }

    static void RegisterReadonlyScriptDictView(AngelScript::asIScriptEngine* engine, const char* decl, const char* value_decl)
    {
        using namespace AngelScript;

        int r;

        r = engine->RegisterObjectType(decl, sizeof(CReadonlyScriptDictView<T>), asOBJ_REF); ROR_ASSERT(r >= 0);
        r = engine->RegisterObjectBehaviour(decl, asBEHAVE_ADDREF, "void f()", asMETHOD(CReadonlyScriptDictView<T>, AddRef), asCALL_THISCALL); ROR_ASSERT(r >= 0);
        r = engine->RegisterObjectBehaviour(decl, asBEHAVE_RELEASE, "void f()", asMETHOD(CReadonlyScriptDictView <T>, Release), asCALL_THISCALL); ROR_ASSERT(r >= 0);

        r = engine->RegisterObjectMethod(decl, "bool exists(const string &in) const", asMETHOD(CReadonlyScriptDictView<T>, Exists), asCALL_THISCALL); ROR_ASSERT(r >= 0);
        r = engine->RegisterObjectMethod(decl, "bool isEmpty() const", asMETHOD(CReadonlyScriptDictView<T>, IsEmpty), asCALL_THISCALL); ROR_ASSERT(r >= 0);
        r = engine->RegisterObjectMethod(decl, "uint getSize() const", asMETHOD(CReadonlyScriptDictView<T>, GetSize), asCALL_THISCALL); ROR_ASSERT(r >= 0);
        r = engine->RegisterObjectMethod(decl, "array<string> @getKeys() const", asMETHOD(CReadonlyScriptDictView<T>, GetKeys), asCALL_THISCALL); assert(r >= 0);

        std::string opIndexDecl = fmt::format("{}@ opIndex(const string &in)", value_decl);
        r = engine->RegisterObjectMethod(decl, opIndexDecl.c_str(), asMETHOD(CReadonlyScriptDictView<T>, OpIndex), asCALL_THISCALL); ROR_ASSERT(r >= 0);
        std::string opIndexConstDecl = fmt::format("const {}@ opIndex(const string &in) const", value_decl);
        r = engine->RegisterObjectMethod(decl, opIndexConstDecl.c_str(), asMETHOD(CReadonlyScriptDictView<T>, OpIndex), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    }

    // Angelscript refcounting
    void AddRef() { m_refcount++; }
    void Release() { m_refcount--; if (m_refcount == 0) { delete this; /* Comit suicide! This is legit in C++ and AngelScript relies on it. */ } }
private:
    const std::map<std::string, T>& m_map;
    int m_refcount;
};

template <typename T>
class CReadonlyScriptArrayView
{
public:

    CReadonlyScriptArrayView(const std::vector<T>& vec) : m_vec(vec), m_refcount(1) {}

    bool IsEmpty() const { return m_vec.empty(); }
    unsigned GetSize() const { return m_vec.size(); }
    T OpIndex(unsigned pos) { return m_vec.at(pos); }

    static void RegisterReadonlyScriptArrayView(AngelScript::asIScriptEngine* engine, const char* decl, const char* value_decl)
    {
        using namespace AngelScript;

        int r;

        r = engine->RegisterObjectType(decl, sizeof(CReadonlyScriptArrayView<T>), asOBJ_REF); ROR_ASSERT(r >= 0);
        r = engine->RegisterObjectBehaviour(decl, asBEHAVE_ADDREF, "void f()", asMETHOD(CReadonlyScriptArrayView<T>, AddRef), asCALL_THISCALL); ROR_ASSERT(r >= 0);
        r = engine->RegisterObjectBehaviour(decl, asBEHAVE_RELEASE, "void f()", asMETHOD(CReadonlyScriptArrayView <T>, Release), asCALL_THISCALL); ROR_ASSERT(r >= 0);

        r = engine->RegisterObjectMethod(decl, "bool isEmpty() const", asMETHOD(CReadonlyScriptArrayView<T>, IsEmpty), asCALL_THISCALL); ROR_ASSERT(r >= 0);
        r = engine->RegisterObjectMethod(decl, "uint length() const", asMETHOD(CReadonlyScriptArrayView<T>, GetSize), asCALL_THISCALL); ROR_ASSERT(r >= 0);
        
        std::string opIndexDecl = fmt::format("{}@ opIndex(uint pos)", value_decl);
        r = engine->RegisterObjectMethod(decl, opIndexDecl.c_str(), asMETHOD(CReadonlyScriptArrayView<T>, OpIndex), asCALL_THISCALL); ROR_ASSERT(r >= 0);
        std::string opIndexConstDecl = fmt::format("const {}@ opIndex(uint pos) const", value_decl);
        r = engine->RegisterObjectMethod(decl, opIndexConstDecl.c_str(), asMETHOD(CReadonlyScriptArrayView<T>, OpIndex), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    }

    // Angelscript refcounting
    void AddRef() { m_refcount++; }
    void Release() { m_refcount--; if (m_refcount == 0) { delete this; /* Comit suicide! This is legit in C++ and AngelScript relies on it. */ } }
private:
    const std::vector<T>& m_vec;
    int m_refcount;
};

// Example opCast behaviour
template<class A, class B>
B* ScriptRefCast(A* a)
{
    // If the handle already is a null handle, then just return the null handle
    if (!a) return 0;

    // Now try to dynamically cast the pointer to the wanted type
    B* b = dynamic_cast<B*>(a);
    if (b != 0)
    {
        // Since the cast was made, we need to increase the ref counter for the returned handle
        b->AddRef();
    }
    return b;
}

// Example opCast behaviour - for objects with asOBJ_NOCOUNT
template<class A, class B>
B* ScriptRefCastNoCount(A* a)
{
    // If the handle already is a null handle, then just return the null handle
    if (!a) return 0;

    // Now try to dynamically cast the pointer to the wanted type
    return dynamic_cast<B*>(a);
}

/// @}   //addtogroup Scripting

} // namespace RoR

#endif // USE_ANGELSCRIPT
