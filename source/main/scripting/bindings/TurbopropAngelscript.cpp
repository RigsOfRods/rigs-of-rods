#include "Application.h"
#include "ScriptEngine.h"
#include "TurboProp.h"
#include "SimData.h"

#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterTurboprop(asIScriptEngine* engine)
{
    int result = 0;

    AeroEngine::RegisterRefCountingObject(engine, "TurbopropClass");
    AeroEnginePtr::RegisterRefCountingObjectPtr(engine, "TurbopropClassPtr", "TurbopropClass");

    // PLEASE maintain same order as in 'physics/air/Turboprop.h' and 'doc/angelscript/Script2Game/TurbopropClass.h'
    result = engine->RegisterObjectMethod("TurbopropClass", "float getPropellerPitch()", asFUNCTIONPR([](Turboprop* self) -> float {
        return self->pitch; }, (Turboprop*), float), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TurbopropClass", "float getPropellerIndicatedTorque()", asFUNCTIONPR([](Turboprop* self) -> float {
        return self->indicated_torque; }, (Turboprop*), float), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TurbopropClass", "float getPropellerMaxTorque()", asFUNCTIONPR([](Turboprop* self) -> float {
        return self->max_torque; }, (Turboprop*), float), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TurbopropClass", "float getPropellerMaxPower()", asFUNCTIONPR([](Turboprop* self) -> float {
        return self->getMaxPower(); }, (Turboprop*), float), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("TurbopropClass", "bool isPistonProp()", asFUNCTIONPR([](Turboprop* self) -> bool {
        return self->is_piston; }, (Turboprop*), bool), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);

    result = engine->RegisterObjectMethod("TurbopropClass", "void setMaxPower(float)", asMETHOD(Turboprop, setMaxPower), asCALL_THISCALL); ROR_ASSERT(result >= 0);
}
