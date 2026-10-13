#include "Application.h"
#include "ScriptEngine.h"
#include "AeroEngine.h"
#include "SimData.h"

#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterAircraftEngineCommon(asIScriptEngine* engine)
{
    int result = 0;

    // enum AircraftDataTypes
    result = engine->RegisterEnum("AircraftEngineTypes"); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("AircraftEngineTypes", "AE_UNKNOWN", (int)AeroEngineType::AE_UNKNOWN); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("AircraftEngineTypes", "AE_TURBOJET", (int)AeroEngineType::AE_TURBOJET); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("AircraftEngineTypes", "AE_PROPELLER", (int)AeroEngineType::AE_XPROP); ROR_ASSERT(result >= 0);
}
