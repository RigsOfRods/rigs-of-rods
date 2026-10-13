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
/// @author Petr Ohlidal
/// @date   01-2022
 
#include "../../autowrapper/aswrappedcall.h"
#include "AngelScriptBindings.h"
#include "InputEngine.h"
#include "ScriptEngine.h"

#include <OIS.h>

using namespace AngelScript;
using namespace RoR;
using namespace OIS;

void RoR::RegisterInputEngineGeneric(asIScriptEngine* engine)
{
    // Please maintain the same order as in 'InputEngine.cpp' and 'docs/*/InputEngineClass.h'
    // --------------------------------------------------------------------------------------

    int result = 0;
    // NOTE: object type InputEngineClass and all enums are registered in RegisterInputEngineCommon()

    // > Input processing
    result = engine->RegisterObjectMethod("InputEngineClass", "void setEventSimulatedValue(inputEvents ev, float value)", WRAP_MFN(InputEngine, setEventSimulatedValue), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("InputEngineClass", "void setEventStatusSupressed(inputEvents ev, bool supressed)", WRAP_MFN(InputEngine, setEventStatusSupressed), asCALL_GENERIC); ROR_ASSERT(result>=0);
    
    // > Event info
    result = engine->RegisterObjectMethod("InputEngineClass", "string getEventCommand(inputEvents ev)", WRAP_MFN(InputEngine, getEventCommand), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("InputEngineClass", "string getEventCommandTrimmed(inputEvents ev)", WRAP_MFN(InputEngine, getEventCommandTrimmed), asCALL_GENERIC); ROR_ASSERT(result>=0);

    // > Event states
    result = engine->RegisterObjectMethod("InputEngineClass", "float getEventValue(inputEvents, bool = false, inputSourceType = inputSourceType::IST_ANY)", WRAP_MFN(InputEngine, getEventValue), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("InputEngineClass", "bool getEventBoolValue(inputEvents ev)", WRAP_MFN(InputEngine, getEventBoolValue), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("InputEngineClass", "bool getEventBoolValueBounce(inputEvents ev, float time = 0.2f)", WRAP_MFN(InputEngine, getEventBoolValueBounce), asCALL_GENERIC); ROR_ASSERT(result>=0);    
    result = engine->RegisterObjectMethod("InputEngineClass", "bool isKeyDownEffective(keyCodes keycode)", WRAP_MFN(InputEngine, isKeyDownEffective), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("InputEngineClass", "bool isKeyDownValueBounce(keyCodes keycode, float time = 0.2f)", WRAP_MFN(InputEngine, isKeyDownValueBounce), asCALL_GENERIC); ROR_ASSERT(result>=0);    
    
    // Direct input device states
    result = engine->RegisterObjectMethod("InputEngineClass", "bool isKeyDown(keyCodes keycode)", WRAP_MFN(InputEngine, isKeyDown), asCALL_GENERIC); ROR_ASSERT(result>=0);
}
