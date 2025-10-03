#include "Application.h"
#include "ScriptEngine.h"
#include "DashBoardManager.h"

#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterDashBoardManager(asIScriptEngine* engine)
{
    int result = 0;

    DashBoardManager::RegisterRefCountingObject(engine, "DashBoardManagerClass");
    DashBoardManagerPtr::RegisterRefCountingObjectPtr(engine, "DashBoardManagerClassPtr", "DashBoardManagerClass");

    // enum DashboardDataTypes
    result = engine->RegisterEnum("DashboardDataTypes"); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("DashboardDataTypes", "DC_BOOL", DC_BOOL); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("DashboardDataTypes", "DC_INT", DC_INT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("DashboardDataTypes", "DC_FLOAT", DC_FLOAT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("DashboardDataTypes", "DC_STRING", DC_CHAR); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("DashboardDataTypes", "DC_INVALID", DC_INVALID); ROR_ASSERT(result >= 0);

    // PLEASE maintain same order as in 'gui/DashBoardManager.cpp' and 'doc/angelscript/Script2Game/DashBoardManagerClass.h'
    result = engine->RegisterObjectMethod("DashBoardManagerClass", "bool getBool(int key)", asMETHOD(DashBoardManager, _getBool), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("DashBoardManagerClass", "float getNumeric(int key)", asMETHOD(DashBoardManager, getNumeric), asCALL_THISCALL); ROR_ASSERT(result >= 0);

    result = engine->RegisterObjectMethod("DashBoardManagerClass", "string getString(int key)", asFUNCTIONPR([](DashBoardManager* self, int key) -> std::string {
        char* s = self->getChar(key);
        return s != NULL ? std::string(s) : ""; }, (DashBoardManager*, int), std::string), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);

    result = engine->RegisterObjectMethod("DashBoardManagerClass", "bool getEnabled(int key)", asMETHOD(DashBoardManager, getEnabled), asCALL_THISCALL); ROR_ASSERT(result >= 0);

    result = engine->RegisterObjectMethod("DashBoardManagerClass", "void setBool(int key, bool value)", asMETHOD(DashBoardManager, setBool), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("DashBoardManagerClass", "void setInt(int key, int value)", asMETHOD(DashBoardManager, setInt), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("DashBoardManagerClass", "void setFloat(int key, float value)", asMETHOD(DashBoardManager, setFloat), asCALL_THISCALL); ROR_ASSERT(result >= 0);

    result = engine->RegisterObjectMethod("DashBoardManagerClass", "void setString(int key, const string& value)", asFUNCTIONPR([](DashBoardManager* self, int key, const std::string& value) {
        self->setChar(key, value.c_str()); }, (DashBoardManager*, int, const std::string&), void), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);

    result = engine->RegisterObjectMethod("DashBoardManagerClass", "void setEnabled(int key, bool value)", asMETHOD(DashBoardManager, setEnabled), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("DashBoardManagerClass", "DashboardDataTypes getDataType(int key)", asMETHOD(DashBoardManager, getDataType), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("DashBoardManagerClass", "int getLinkIDForName(string& key)", asMETHOD(DashBoardManager, getLinkIDForName), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("DashBoardManagerClass", "void updateFeatures()", asMETHOD(DashBoardManager, updateFeatures), asCALL_THISCALL); ROR_ASSERT(result >= 0);
}
