#include "Application.h"
#include "ScriptEngine.h"
#include "AutoPilot.h"
#include "SimData.h"

#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterAutopilotCommon(asIScriptEngine* engine)
{
    int result = 0;

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
}
