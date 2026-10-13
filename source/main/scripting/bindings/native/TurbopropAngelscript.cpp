#include "Application.h"
#include "ScriptEngine.h"
#include "TurboProp.h"
#include "SimData.h"

#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterTurbopropNative(asIScriptEngine* engine)
{
    int result = 0;

    AeroEngine::RegisterRefCountingObject(engine, "TurbopropClass");
    AeroEnginePtr::RegisterRefCountingObjectPtr(engine, "TurbopropClassPtr", "TurbopropClass");

    // PLEASE maintain same order as in 'physics/air/Turboprop.h' and 'doc/angelscript/Script2Game/TurbopropClass.h'
    result = engine->RegisterObjectMethod("TurbopropClass", "float getPropellerPitch()", asMETHOD(Turboprop, getPropellerPitch), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TurbopropClass", "float getPropellerIndicatedTorque()", asMETHOD(Turboprop, getPropellerIndicatedTorque), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TurbopropClass", "float getPropellerMaxTorque()", asMETHOD(Turboprop, getPropellerMaxTorque), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TurbopropClass", "float getPropellerMaxPower()", asMETHOD(Turboprop, getMaxPower), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TurbopropClass", "bool isPistonProp()", asMETHOD(Turboprop, isPistonProp), asCALL_THISCALL); ROR_ASSERT(result >= 0);
}
