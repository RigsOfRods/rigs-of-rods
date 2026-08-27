#include "Application.h"
#include "ScriptEngine.h"
#include "DashBoardManager.h"
#include "../../autowrapper/aswrappedcall.h"

#include <angelscript.h>

using namespace AngelScript;

static std::string DashBoardManagerGetString(RoR::DashBoardManager* self, int key)
{
    char* s = self->getChar(key);
    return s != nullptr ? std::string(s) : "";
}

static void DashBoardManagerSetString(RoR::DashBoardManager* self, int key, const std::string& value)
{
    self->setChar(key, value.c_str());
}

void RoR::RegisterDashBoardManagerGeneric(asIScriptEngine* engine)
{
    int result = 0;

    DashBoardManager::RegisterRefCountingObject(engine, "DashBoardManagerClass");
    DashBoardManagerPtr::RegisterRefCountingObjectPtr(engine, "DashBoardManagerClassPtr", "DashBoardManagerClass");

    // PLEASE maintain same order as in 'gui/DashBoardManager.cpp' and 'doc/angelscript/Script2Game/DashBoardManagerClass.h'
    result = engine->RegisterObjectMethod("DashBoardManagerClass", "bool getBool(int key)", WRAP_MFN(DashBoardManager, _getBool), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("DashBoardManagerClass", "float getNumeric(int key)", WRAP_MFN(DashBoardManager, getNumeric), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    result = engine->RegisterObjectMethod("DashBoardManagerClass", "string getString(int key)", WRAP_OBJ_FIRST_PR(DashBoardManagerGetString, (DashBoardManager*, int), std::string), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    result = engine->RegisterObjectMethod("DashBoardManagerClass", "bool getEnabled(int key)", WRAP_MFN(DashBoardManager, getEnabled), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    result = engine->RegisterObjectMethod("DashBoardManagerClass", "void setBool(int key, bool value)", WRAP_MFN(DashBoardManager, setBool), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("DashBoardManagerClass", "void setInt(int key, int value)", WRAP_MFN(DashBoardManager, setInt), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("DashBoardManagerClass", "void setFloat(int key, float value)", WRAP_MFN(DashBoardManager, setFloat), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    result = engine->RegisterObjectMethod("DashBoardManagerClass", "void setString(int key, const string&in value)", WRAP_OBJ_FIRST_PR(DashBoardManagerSetString, (DashBoardManager*, int, const std::string&), void), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    result = engine->RegisterObjectMethod("DashBoardManagerClass", "void setEnabled(int key, bool value)", WRAP_MFN(DashBoardManager, setEnabled), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("DashBoardManagerClass", "DashboardDataTypes getDataType(int key)", WRAP_MFN(DashBoardManager, getDataType), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("DashBoardManagerClass", "int getLinkIDForName(const string&in key)", WRAP_MFN(DashBoardManager, getLinkIDForName), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("DashBoardManagerClass", "void updateFeatures()", WRAP_MFN(DashBoardManager, updateFeatures), asCALL_GENERIC); ROR_ASSERT(result >= 0);
}
