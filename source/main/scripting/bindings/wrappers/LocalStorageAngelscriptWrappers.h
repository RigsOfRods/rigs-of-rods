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
/// Helper/wrapper functions shared by native (LocalStorageAngelscript.cpp)
/// and generic (LocalStorageAngelscriptGeneric.cpp) LocalStorage bindings.
/// Native bindings register these directly (asCALL_CDECL/asCALL_CDECL_OBJFIRST),
/// generic bindings wrap them using WRAP_FN()/WRAP_OBJ_FIRST().

#pragma once

#include "LocalStorage.h"

#include <string>

namespace LocalStorageAngelscriptWrappers {

using namespace RoR;

static LocalStorage* LocalStorageFactory(std::string filename, const std::string& section_name, const std::string& rg_name)
{
    return new LocalStorage(filename, section_name, rg_name);
}

// The script declares `const bool &in` - the C++ `set()` takes `bool` by value.
static void LocalStorage_setBool(LocalStorage* self, std::string key, const bool& value)
{
    self->set(key, value);
}

// The script declares `string &in` - the C++ `exists()` takes `std::string` by value.
static bool LocalStorage_exists(LocalStorage* self, const std::string& key)
{
    return self->exists(key);
}

// The script declares `string &in` - the C++ `eraseKey()` takes `std::string` by value.
static void LocalStorage_eraseKey(LocalStorage* self, const std::string& key)
{
    self->eraseKey(key);
}

} // namespace LocalStorageAngelscriptWrappers
