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

#include "AngelScriptBindings.h"
#include "Application.h"
#include "CacheSystem.h"

#include <angelscript.h>

using namespace AngelScript;
using namespace RoR;

void RoR::RegisterCacheSystemCommon(asIScriptEngine *engine)
{
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

    // class CacheSystem (non-counted reference type)
    result = engine->RegisterObjectType("CacheSystemClass", sizeof(CacheSystem), asOBJ_REF | asOBJ_NOCOUNT); ROR_ASSERT(result>=0);
}
