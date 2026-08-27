#include "Application.h"
#include "ScriptEngine.h"
#include "TurboProp.h"
#include "SimData.h"
#include "../../autowrapper/aswrappedcall.h"

#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterTurbopropGeneric(asIScriptEngine* engine)
{
    int result = 0;

    AeroEngine::RegisterRefCountingObject(engine, "TurbopropClass");
    AeroEnginePtr::RegisterRefCountingObjectPtr(engine, "TurbopropClassPtr", "TurbopropClass");

    // PLEASE maintain same order as in 'physics/air/Turboprop.h' and 'doc/angelscript/Script2Game/TurbopropClass.h'
    result = engine->RegisterObjectMethod("TurbopropClass", "float getPropellerPitch()", WRAP_MFN(Turboprop, getPropellerPitch), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TurbopropClass", "float getPropellerIndicatedTorque()", WRAP_MFN(Turboprop, getPropellerIndicatedTorque), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TurbopropClass", "float getPropellerMaxTorque()", WRAP_MFN(Turboprop, getPropellerMaxTorque), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TurbopropClass", "float getPropellerMaxPower()", WRAP_MFN(Turboprop, getMaxPower), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TurbopropClass", "bool isPistonProp()", WRAP_MFN(Turboprop, isPistonProp), asCALL_GENERIC); ROR_ASSERT(result >= 0);
}
