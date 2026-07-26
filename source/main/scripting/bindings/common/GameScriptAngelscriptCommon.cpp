/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2024 Petr Ohlidal

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

#include "AngelScriptBindings.h"
#include "GameScript.h"
#include "ScriptEngine.h"

#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterGameScriptCommon(asIScriptEngine *engine)
{
    int result;

    // enum ScriptCategory ~ for `game.pushMessage(MSG_APP_LOAD_SCRIPT_REQUESTED ...)`
    result = engine->RegisterEnum("ScriptCategory"); ROR_ASSERT(result >= 0);

    result = engine->RegisterEnumValue("ScriptCategory", "SCRIPT_CATEGORY_INVALID", static_cast<int>(ScriptCategory::INVALID)); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptCategory", "SCRIPT_CATEGORY_ACTOR", static_cast<int>(ScriptCategory::ACTOR)); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptCategory", "SCRIPT_CATEGORY_TERRAIN", static_cast<int>(ScriptCategory::TERRAIN)); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptCategory", "SCRIPT_CATEGORY_CUSTOM", static_cast<int>(ScriptCategory::CUSTOM)); ROR_ASSERT(result >= 0);

    // `enum ScriptRetCode` ~ Common return codes for script manipulation funcs (add/get/delete | funcs/variables)
    result = engine->RegisterEnum("ScriptRetCode"); ROR_ASSERT(result >= 0);

    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_ERROR", SCRIPTRETCODE_AS_ERROR ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_CONTEXT_ACTIVE", SCRIPTRETCODE_AS_CONTEXT_ACTIVE ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_CONTEXT_NOT_FINISHED", SCRIPTRETCODE_AS_CONTEXT_NOT_FINISHED ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_CONTEXT_NOT_PREPARED", SCRIPTRETCODE_AS_CONTEXT_NOT_PREPARED ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_INVALID_ARG", SCRIPTRETCODE_AS_INVALID_ARG ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_NO_FUNCTION", SCRIPTRETCODE_AS_NO_FUNCTION ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_NOT_SUPPORTED", SCRIPTRETCODE_AS_NOT_SUPPORTED ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_INVALID_NAME", SCRIPTRETCODE_AS_INVALID_NAME ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_NAME_TAKEN", SCRIPTRETCODE_AS_NAME_TAKEN ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_INVALID_DECLARATION", SCRIPTRETCODE_AS_INVALID_DECLARATION ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_INVALID_OBJECT", SCRIPTRETCODE_AS_INVALID_OBJECT ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_INVALID_TYPE", SCRIPTRETCODE_AS_INVALID_TYPE ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_ALREADY_REGISTERED", SCRIPTRETCODE_AS_ALREADY_REGISTERED ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_MULTIPLE_FUNCTIONS", SCRIPTRETCODE_AS_MULTIPLE_FUNCTIONS ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_NO_MODULE", SCRIPTRETCODE_AS_NO_MODULE ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_NO_GLOBAL_VAR", SCRIPTRETCODE_AS_NO_GLOBAL_VAR ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_INVALID_CONFIGURATION", SCRIPTRETCODE_AS_INVALID_CONFIGURATION ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_INVALID_INTERFACE", SCRIPTRETCODE_AS_INVALID_INTERFACE ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_CANT_BIND_ALL_FUNCTIONS", SCRIPTRETCODE_AS_CANT_BIND_ALL_FUNCTIONS ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_LOWER_ARRAY_DIMENSION_NOT_REGISTERED", SCRIPTRETCODE_AS_LOWER_ARRAY_DIMENSION_NOT_REGISTERED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_WRONG_CONFIG_GROUP", SCRIPTRETCODE_AS_WRONG_CONFIG_GROUP ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_CONFIG_GROUP_IS_IN_USE", SCRIPTRETCODE_AS_CONFIG_GROUP_IS_IN_USE ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_ILLEGAL_BEHAVIOUR_FOR_TYPE", SCRIPTRETCODE_AS_ILLEGAL_BEHAVIOUR_FOR_TYPE ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_WRONG_CALLING_CONV", SCRIPTRETCODE_AS_WRONG_CALLING_CONV ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_BUILD_IN_PROGRESS", SCRIPTRETCODE_AS_BUILD_IN_PROGRESS ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_INIT_GLOBAL_VARS_FAILED", SCRIPTRETCODE_AS_INIT_GLOBAL_VARS_FAILED ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_OUT_OF_MEMORY", SCRIPTRETCODE_AS_OUT_OF_MEMORY ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_AS_MODULE_IS_IN_USE", SCRIPTRETCODE_AS_MODULE_IS_IN_USE ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_UNSPECIFIED_ERROR", SCRIPTRETCODE_UNSPECIFIED_ERROR); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_ENGINE_NOT_CREATED", SCRIPTRETCODE_ENGINE_NOT_CREATED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_CONTEXT_NOT_CREATED", SCRIPTRETCODE_CONTEXT_NOT_CREATED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_SCRIPTUNIT_NOT_EXISTS", SCRIPTRETCODE_SCRIPTUNIT_NOT_EXISTS); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_SCRIPTUNIT_NO_MODULE", SCRIPTRETCODE_SCRIPTUNIT_NO_MODULE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ScriptRetCode", "SCRIPTRETCODE_FUNCTION_NOT_EXISTS", SCRIPTRETCODE_FUNCTION_NOT_EXISTS); ROR_ASSERT(result >= 0);
}
