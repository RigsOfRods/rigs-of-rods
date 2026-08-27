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

#pragma once

#include "AngelScriptBindings.h"
#include "Console.h"

namespace RoR {

void RegisterAngelScriptBinding(const char* bindingName, AngelScript::asIScriptEngine* engine, ASBindingRegisterFunc_t native, ASBindingRegisterFunc_t generic)
{
    // If diag_angelscript_generic_bind_test is true, we will try to register generic bindings, or fall back
    // to native bindings if not available. For testing purposes only!
    bool max_portability = strstr(AngelScript::asGetLibraryOptions(), "AS_MAX_PORTABILITY") || App::diag_angelscript_generic_bind_test->getBool();
    ASBindingRegisterFunc_t register_func = max_portability ? generic : native;
    if (max_portability && !generic && App::diag_angelscript_generic_bind_test->getBool())
    {
        max_portability = false;
        register_func = native;
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_LOG, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("[AngelScriptBindings] Warning: Generic bindings not available for {}, falling back to native bindings", bindingName));
    }

    if (register_func)
    {
        register_func(engine);
    }
    else if (max_portability) // Max portability is enabled, no generic binding func specified
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_LOG, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("[AngelScriptBindings] Warning: Generic bindings not available for {}", bindingName));
    }
    else // Max portability is disabled, no native binding func specified
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_LOG, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("[AngelScriptBindings] Warning: Native bindings not available for {}", bindingName));
    }
}

} // namespace RoR
