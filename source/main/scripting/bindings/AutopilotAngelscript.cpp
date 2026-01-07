#include "Application.h"
#include "ScriptEngine.h"
#include "AutoPilot.h"
#include "SimData.h"

#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterAutopilot(asIScriptEngine* engine)
{
    int result = 0;

    Autopilot::RegisterRefCountingObject(engine, "AutopilotClass");
    AutopilotPtr::RegisterRefCountingObjectPtr(engine, "AutopilotClassPtr", "AutopilotClass");

    // enum APHeadingMode
    result = engine->RegisterEnum("APHeadingMode"); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("APHeadingMode", "HEADING_NONE", Autopilot::HEADING_NONE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("APHeadingMode", "HEADING_FIXED", Autopilot::HEADING_FIXED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("APHeadingMode", "HEADING_NAV", Autopilot::HEADING_NAV); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("APHeadingMode", "HEADING_WLV", Autopilot::HEADING_WLV); ROR_ASSERT(result >= 0);

    // enum APAltitudeMode
    result = engine->RegisterEnum("APAltitudeMode"); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("APAltitudeMode", "ALT_NONE", Autopilot::ALT_NONE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("APAltitudeMode", "ALT_FIXED", Autopilot::ALT_FIXED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("APAltitudeMode", "ALT_VS", Autopilot::ALT_VS); ROR_ASSERT(result >= 0);

    // PLEASE maintain same order as in 'gameplay/AutoPilot.h' and 'doc/angelscript/Script2Game/AutopilotClass.h'
    result = engine->RegisterObjectMethod("AutopilotClass", "void disconnect()", asMETHOD(Autopilot, disconnect), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "bool getOperateControls()", asMETHOD(Autopilot, getOperateControls), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "void setOperateControls(bool)", asMETHOD(Autopilot, setOperateControls), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "APHeadingMode toggleHeading(APHeadingMode)", asMETHOD(Autopilot, toggleHeading), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "APAltitudeMode toggleAltitude(APAltitudeMode)", asMETHOD(Autopilot, toggleAlt), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "bool toggleIAS()", asMETHOD(Autopilot, toggleIAS), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "bool toggleGPWS()", asMETHOD(Autopilot, toggleGPWS), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "int adjustHeading(int)", asMETHOD(Autopilot, adjHDG), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "int adjustAltitude(int)", asMETHOD(Autopilot, adjALT), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "int adjustVerticalSpeed(int)", asMETHOD(Autopilot, adjVS), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "int adjustIAS(int)", asMETHOD(Autopilot, adjIAS), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "float getVerticalApproachDeviation()", asMETHOD(Autopilot, GetVerticalApproachDeviation), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "float getHorizontalApproachDeviation()", asMETHOD(Autopilot, GetHorizontalApproachDeviation), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "bool isILSAvailable()", asMETHOD(Autopilot, IsIlsAvailable), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "APHeadingMode getHeadingMode()", asMETHOD(Autopilot, GetHeadingMode), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "int getHeadingValue()", asMETHOD(Autopilot, GetHeadingValue), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "APAltitudeMode getAltitudeMode()", asMETHOD(Autopilot, GetAltMode), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "int getAltitudeValue()", asMETHOD(Autopilot, GetAltValue), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "bool getIASMode()", asMETHOD(Autopilot, GetIasMode), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "int getIASValue()", asMETHOD(Autopilot, GetIasValue), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "bool getGPWSMode()", asMETHOD(Autopilot, GetGpwsMode), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("AutopilotClass", "int getVerticalSpeedValue()", asMETHOD(Autopilot, GetVsValue), asCALL_THISCALL); ROR_ASSERT(result >= 0);
}
