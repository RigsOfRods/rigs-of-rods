#include "Application.h"
#include "ScriptEngine.h"
#include "ScrewProp.h"
#include "SimData.h"
#include "../../autowrapper/aswrappedcall.h"

#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterScrewpropGeneric(asIScriptEngine* engine)
{
    int result = 0;

    Screwprop::RegisterRefCountingObject(engine, "ScrewpropClass");
    ScrewpropPtr::RegisterRefCountingObjectPtr(engine, "ScrewpropClassPtr", "ScrewpropClass");

    // PLEASE maintain same order as in 'physics/water/Screwprop.h' and 'doc/angelscript/Script2Game/ScrewpropClass.h'
    result = engine->RegisterObjectMethod("ScrewpropClass", "void setThrottle(float)", WRAP_MFN(Screwprop, setThrottle), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ScrewpropClass", "void setRudder(float)", WRAP_MFN(Screwprop, setRudder), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ScrewpropClass", "float getThrottle()", WRAP_MFN(Screwprop, getThrottle), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ScrewpropClass", "float getRudder()", WRAP_MFN(Screwprop, getRudder), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ScrewpropClass", "float getMaxPower()", WRAP_MFN(Screwprop, getMaxPower), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ScrewpropClass", "bool getReverse()", WRAP_MFN(Screwprop, getReverse), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ScrewpropClass", "void toggleReverse()", WRAP_MFN(Screwprop, toggleReverse), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ScrewpropClass", "int getRefNode()", WRAP_MFN(Screwprop, GetRefNode), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ScrewpropClass", "int getBackNode()", WRAP_MFN(Screwprop, GetBackNode), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("ScrewpropClass", "int getUpNode()", WRAP_MFN(Screwprop, GetUpNode), asCALL_GENERIC); ROR_ASSERT(result >= 0);
}
