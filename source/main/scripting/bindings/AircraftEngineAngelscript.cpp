#include "Application.h"
#include "ScriptEngine.h"
#include "AeroEngine.h"
#include "SimData.h"

#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterAircraftEngine(asIScriptEngine* engine)
{
    int result = 0;

    AeroEngine::RegisterRefCountingObject(engine, "AircraftEngineClass");
    AeroEnginePtr::RegisterRefCountingObjectPtr(engine, "AircraftEngineClassPtr", "AircraftEngineClass");

    // enum AircraftDataTypes
    result = engine->RegisterEnum("AircraftEngineTypes"); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("AircraftEngineTypes", "AE_UNKNOWN", (int)AeroEngineType::AE_UNKNOWN); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("AircraftEngineTypes", "AE_TURBOJET", (int)AeroEngineType::AE_TURBOJET); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("AircraftEngineTypes", "AE_PROPELLER", (int)AeroEngineType::AE_XPROP); ROR_ASSERT(result >= 0);

    // PLEASE maintain same order as in 'physics/air/AeroEngine.h' and 'doc/angelscript/Script2Game/AircraftEngineClass.h'
    result = engine->RegisterObjectMethod("AircraftEngineClass", "void setThrottle(float)", asMETHOD(AeroEngine, setThrottle), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AircraftEngineClass", "float getThrottle()", asMETHOD(AeroEngine, getThrottle), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AircraftEngineClass", "void toggleReverse()", asMETHOD(AeroEngine, toggleReverse), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AircraftEngineClass", "void setReverse(bool)", asMETHOD(AeroEngine, setReverse), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AircraftEngineClass", "bool getReverse()", asMETHOD(AeroEngine, getReverse), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AircraftEngineClass", "void flipStart()", asMETHOD(AeroEngine, flipStart), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AircraftEngineClass", "float getRPMPercent()", asMETHOD(AeroEngine, getRPMpc), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AircraftEngineClass", "bool isFailed()", asMETHOD(AeroEngine, isFailed), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AircraftEngineClass", "AircraftEngineTypes getType()", asMETHOD(AeroEngine, getType), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AircraftEngineClass", "bool getIgnition()", asMETHOD(AeroEngine, getIgnition), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AircraftEngineClass", "int getFrontNode()", asMETHOD(AeroEngine, GetFrontNode), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AircraftEngineClass", "int getBackNode()", asMETHOD(AeroEngine, GetBackNode), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AircraftEngineClass", "bool getWarmup()", asMETHOD(AeroEngine, getWarmup), asCALL_THISCALL); ROR_ASSERT(result >= 0);
}
