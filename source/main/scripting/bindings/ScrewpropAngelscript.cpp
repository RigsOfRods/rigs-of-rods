#include "Application.h"
#include "ScriptEngine.h"
#include "ScrewProp.h"
#include "SimData.h"

#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterScrewprop(asIScriptEngine* engine)
{
    int result = 0;

    Screwprop::RegisterRefCountingObject(engine, "ScrewpropClass");
    ScrewpropPtr::RegisterRefCountingObjectPtr(engine, "ScrewpropClassPtr", "ScrewpropClass");

    // PLEASE maintain same order as in 'physics/water/Screwprop.h' and 'doc/angelscript/Script2Game/ScrewpropClass.h'
    result = engine->RegisterObjectMethod("ScrewpropClass", "void setThrottle(float)", asMETHOD(Screwprop, setThrottle), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ScrewpropClass", "void setRudder(float)", asMETHOD(Screwprop, setRudder), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ScrewpropClass", "void setMaxPower(float)", asMETHOD(Screwprop, setMaxPower), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ScrewpropClass", "float getThrottle()", asMETHOD(Screwprop, getThrottle), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ScrewpropClass", "float getRudder()", asMETHOD(Screwprop, getRudder), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ScrewpropClass", "float getMaxPower()", asMETHOD(Screwprop, getMaxPower), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ScrewpropClass", "bool getReverse()", asMETHOD(Screwprop, getReverse), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ScrewpropClass", "void toggleReverse()", asMETHOD(Screwprop, toggleReverse), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ScrewpropClass", "int getRefNode()", asMETHOD(Screwprop, GetRefNode), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ScrewpropClass", "int getBackNode()", asMETHOD(Screwprop, GetBackNode), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ScrewpropClass", "int getUpNode()", asMETHOD(Screwprop, GetUpNode), asCALL_THISCALL); ROR_ASSERT(result >= 0);
}
