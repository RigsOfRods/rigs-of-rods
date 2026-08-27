#include "Application.h"
#include "ScriptEngine.h"
#include "AeroEngine.h"
#include "SimData.h"
#include "../../autowrapper/aswrappedcall.h"

#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterAircraftEngineGeneric(asIScriptEngine* engine)
{
    int result = 0;

    AeroEngine::RegisterRefCountingObject(engine, "AircraftEngineClass");
    AeroEnginePtr::RegisterRefCountingObjectPtr(engine, "AircraftEngineClassPtr", "AircraftEngineClass");

    // PLEASE maintain same order as in 'physics/air/AeroEngine.h' and 'doc/angelscript/Script2Game/AircraftEngineClass.h'
    result = engine->RegisterObjectMethod("AircraftEngineClass", "void setThrottle(float)", WRAP_MFN(AeroEngine, setThrottle), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AircraftEngineClass", "float getThrottle()", WRAP_MFN(AeroEngine, getThrottle), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AircraftEngineClass", "void toggleReverse()", WRAP_MFN(AeroEngine, toggleReverse), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AircraftEngineClass", "void setReverse(bool)", WRAP_MFN(AeroEngine, setReverse), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AircraftEngineClass", "bool getReverse()", WRAP_MFN(AeroEngine, getReverse), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AircraftEngineClass", "void flipStart()", WRAP_MFN(AeroEngine, flipStart), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AircraftEngineClass", "float getRPMPercent()", WRAP_MFN(AeroEngine, getRPMpc), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AircraftEngineClass", "bool isFailed()", WRAP_MFN(AeroEngine, isFailed), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AircraftEngineClass", "AircraftEngineTypes getType()", WRAP_MFN(AeroEngine, getType), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AircraftEngineClass", "bool getIgnition()", WRAP_MFN(AeroEngine, getIgnition), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AircraftEngineClass", "int getFrontNode()", WRAP_MFN(AeroEngine, GetFrontNode), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AircraftEngineClass", "int getBackNode()", WRAP_MFN(AeroEngine, GetBackNode), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AircraftEngineClass", "bool getWarmup()", WRAP_MFN(AeroEngine, getWarmup), asCALL_GENERIC); ROR_ASSERT(result >= 0);
}
