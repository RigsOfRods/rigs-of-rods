#include "Application.h"
#include "ScriptEngine.h"
#include "AutoPilot.h"
#include "SimData.h"
#include "../../autowrapper/aswrappedcall.h"

#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterAutopilotGeneric(asIScriptEngine* engine)
{
    int result = 0;

    Autopilot::RegisterRefCountingObject(engine, "AutopilotClass");
    AutopilotPtr::RegisterRefCountingObjectPtr(engine, "AutopilotClassPtr", "AutopilotClass");

    // PLEASE maintain same order as in 'gameplay/AutoPilot.h' and 'doc/angelscript/Script2Game/AutopilotClass.h'
    result = engine->RegisterObjectMethod("AutopilotClass", "void disconnect()", WRAP_MFN(Autopilot, disconnect), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "bool getForceDisabled()", WRAP_MFN(Autopilot, getForceDisabled), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "void setForceDisabled(bool)", WRAP_MFN(Autopilot, setForceDisabled), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "APHeadingMode toggleHeading(APHeadingMode)", WRAP_MFN(Autopilot, toggleHeading), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "APAltitudeMode toggleAltitude(APAltitudeMode)", WRAP_MFN(Autopilot, toggleAlt), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "bool toggleIAS()", WRAP_MFN(Autopilot, toggleIAS), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "bool toggleGPWS()", WRAP_MFN(Autopilot, toggleGPWS), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "int adjustHeading(int)", WRAP_MFN(Autopilot, adjHDG), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "int adjustAltitude(int)", WRAP_MFN(Autopilot, adjALT), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "int adjustVerticalSpeed(int)", WRAP_MFN(Autopilot, adjVS), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "int adjustIAS(int)", WRAP_MFN(Autopilot, adjIAS), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "float getVerticalApproachDeviation()", WRAP_MFN(Autopilot, GetVerticalApproachDeviation), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "float getHorizontalApproachDeviation()", WRAP_MFN(Autopilot, GetHorizontalApproachDeviation), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "bool isILSAvailable()", WRAP_MFN(Autopilot, IsIlsAvailable), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "APHeadingMode getHeadingMode()", WRAP_MFN(Autopilot, GetHeadingMode), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "int getHeadingValue()", WRAP_MFN(Autopilot, GetHeadingValue), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "APAltitudeMode getAltitudeMode()", WRAP_MFN(Autopilot, GetAltMode), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "int getAltitudeValue()", WRAP_MFN(Autopilot, GetAltValue), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "bool getIASMode()", WRAP_MFN(Autopilot, GetIasMode), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "int getIASValue()", WRAP_MFN(Autopilot, GetIasValue), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "bool getGPWSMode()", WRAP_MFN(Autopilot, GetGpwsMode), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "int getVerticalSpeedValue()", WRAP_MFN(Autopilot, GetVsValue), asCALL_GENERIC); ROR_ASSERT(result >= 0);
}
