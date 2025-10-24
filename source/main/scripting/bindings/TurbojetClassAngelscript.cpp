#include "Application.h"
#include "ScriptEngine.h"
#include "TurboJet.h"
#include "SimData.h"

#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterTurbojetClass(asIScriptEngine* engine)
{
    int result = 0;

    AeroEngine::RegisterRefCountingObject(engine, "TurbojetClass");
    AeroEnginePtr::RegisterRefCountingObjectPtr(engine, "TurbojetClassPtr", "TurbojetClass");

    // PLEASE maintain same order as in 'physics/air/Turbojet.h' and 'doc/angelscript/Script2Game/TurbojetClass.h'
    result = engine->RegisterObjectMethod("TurbojetClass", "float getMaxDryThrust()", asMETHOD(Turbojet, getMaxDryThrust), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TurbojetClass", "bool getAfterburner()", asFUNCTIONPR([](Turbojet* self) -> bool {
        return self->getAfterburner() != 0; }, (Turbojet*), bool), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TurbojetClass", "float getAfterburnerThrust()", asMETHOD(Turbojet, getAfterburnThrust), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TurbojetClass", "float getExhaustVelocity()", asMETHOD(Turbojet, getExhaustVelocity), asCALL_THISCALL); ROR_ASSERT(result >= 0);
}
