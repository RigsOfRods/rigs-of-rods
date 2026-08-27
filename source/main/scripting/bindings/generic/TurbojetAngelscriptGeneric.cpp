#include "Application.h"
#include "ScriptEngine.h"
#include "TurboJet.h"
#include "SimData.h"
#include "../autowrapper/aswrappedcall.h"

#include <angelscript.h>

using namespace AngelScript;

static bool TurbojetGetAfterburner(RoR::Turbojet* tj)
{
    return tj->getAfterburner() != 0;
}

void RoR::RegisterTurbojetGeneric(asIScriptEngine* engine)
{
    int result = 0;

    AeroEngine::RegisterRefCountingObject(engine, "TurbojetClass");
    AeroEnginePtr::RegisterRefCountingObjectPtr(engine, "TurbojetClassPtr", "TurbojetClass");

    // PLEASE maintain same order as in 'physics/air/Turbojet.h' and 'doc/angelscript/Script2Game/TurbojetClass.h'
    result = engine->RegisterObjectMethod("TurbojetClass", "float getMaxDryThrust()", WRAP_MFN(Turbojet, getMaxDryThrust), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TurbojetClass", "bool getAfterburner()", WRAP_OBJ_FIRST(TurbojetGetAfterburner), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TurbojetClass", "float getAfterburnerThrust()", WRAP_MFN(Turbojet, getAfterburnThrust), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TurbojetClass", "float getExhaustVelocity()", WRAP_MFN(Turbojet, getExhaustVelocity), asCALL_GENERIC); ROR_ASSERT(result >= 0);
}