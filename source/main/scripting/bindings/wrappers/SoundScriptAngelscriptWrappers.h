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
/// Helper/wrapper functions shared by native (SoundScriptAngelscript.cpp)
/// and generic (SoundScriptAngelscriptGeneric.cpp) SoundScript bindings.
/// Native bindings register these directly (asCALL_CDECL_OBJFIRST),
/// generic bindings wrap them using WRAP_OBJ_FIRST().

#pragma once

#include "Sound.h"
#include "SoundScriptManager.h"

namespace SoundScriptAngelscriptWrappers {

using namespace RoR;

// The C++ getters return `Ptr&` (a reference), but the script declares `Ptr@`,
// which for these handle-types (asOBJ_ASHANDLE value types) means a value returned by value.
// These wrappers return a copy so that the C++ signature matches the script declaration.

static SoundScriptTemplatePtr SoundScriptInstance_getTemplate(SoundScriptInstance* self)
{
    return self->getTemplate();
}

static SoundPtr SoundScriptInstance_getStartSound(SoundScriptInstance* self)
{
    return self->getStartSound();
}

static SoundPtr SoundScriptInstance_getStopSound(SoundScriptInstance* self)
{
    return self->getStopSound();
}

static SoundPtr SoundScriptInstance_getSound(SoundScriptInstance* self, int pos)
{
    return self->getSound(pos);
}

} // namespace SoundScriptAngelscriptWrappers
