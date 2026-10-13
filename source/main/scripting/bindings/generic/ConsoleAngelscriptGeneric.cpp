/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2022 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#include "../../autowrapper/aswrappedcall.h"
#include "Console.h"
#include "AngelScriptBindings.h"
#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterConsoleGeneric(asIScriptEngine *engine)
{
    int result;

    // NOTE: object types CVarClass/ConsoleClass and enum CVarFlags are registered in RegisterConsoleCommon()

    // class CVar
    result = engine->RegisterObjectMethod("CVarClass", "const string& getName()", WRAP_MFN(CVar, getName), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CVarClass", "const string& getStr()", WRAP_MFN(CVar, getStr), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CVarClass", "int getInt()", WRAP_MFN(CVar, getInt), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CVarClass", "float getFloat()", WRAP_MFN(CVar, getFloat), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CVarClass", "bool getBool()", WRAP_MFN(CVar, getBool), asCALL_GENERIC); ROR_ASSERT(result>=0);

    // class Console
    result = engine->RegisterObjectMethod("ConsoleClass", "CVarClass @cVarCreate(const string &in, const string &in, int, const string &in)", WRAP_MFN(Console, cVarCreate), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("ConsoleClass", "CVarClass @cVarFind(const string &in)", WRAP_MFN(Console, cVarFind), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("ConsoleClass", "CVarClass @cVarGet(const string &in, int)", WRAP_MFN(Console, cVarGet), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("ConsoleClass", "CVarClass @cVarSet(const string &in, const string &in)", WRAP_MFN(Console, cVarSet), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("ConsoleClass", "void cVarAssign(CVarClass@, const string &in)", WRAP_MFN(Console, cVarAssign), asCALL_GENERIC); ROR_ASSERT(result>=0);
}
