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
/// Helper/wrapper functions shared by native (GenericFileFormatAngelscript.cpp)
/// and generic (GenericFileFormatAngelscriptGeneric.cpp) GenericFileFormat bindings.
/// Native bindings register these directly (asCALL_CDECL),
/// generic bindings wrap them using WRAP_FN().

#pragma once

#include "GenericFileFormat.h"

namespace GenericFileFormatAngelscriptWrappers {

using namespace RoR;

// Factories
static GenericDocument* GenericDocumentFactory()
{
    return new GenericDocument();
}

static GenericDocContext* GenericDocContextFactory(GenericDocumentPtr doc)
{
    return new GenericDocContext(doc);
}

} // namespace GenericFileFormatAngelscriptWrappers
