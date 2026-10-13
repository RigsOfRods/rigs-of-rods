#include "Application.h"
#include "ScriptEngine.h"
#include "DashBoardManager.h"

#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterDashBoardManagerCommon(asIScriptEngine* engine)
{
    int result = 0;

    // enum DashboardDataTypes
    result = engine->RegisterEnum("DashboardDataTypes"); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("DashboardDataTypes", "DC_BOOL", DC_BOOL); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("DashboardDataTypes", "DC_INT", DC_INT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("DashboardDataTypes", "DC_FLOAT", DC_FLOAT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("DashboardDataTypes", "DC_STRING", DC_CHAR); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("DashboardDataTypes", "DC_INVALID", DC_INVALID); ROR_ASSERT(result >= 0);
}
